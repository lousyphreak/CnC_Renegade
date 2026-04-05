#include "sdlmixer_audio_utils.h"

#include "SoundBuffer.h"

#include "always.h"
#include "ffactory.h"
#include "rawfile.h"
#include "WWAudio.h"
#include "wwdebug.h"

#include <SDL3/SDL.h>
#include <SDL3_mixer/SDL_mixer.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace
{
struct BackendProvider
{
    const char *name;
};

struct BackendSample
{
    explicit BackendSample(bool want_3d) : is_3d(want_3d) {}

    bool is_3d = false;
    MIX_Track *track = nullptr;
    MIX_Audio *audio = nullptr;
    std::array<uintptr_t, 8> user_data{};
    SDL_AudioSpec format{};
    int duration_ms = 0;
    int volume = 127;
    int pan = 64;
    U32 loop_count = 1;
    int playback_rate = 0;
    int original_rate = 0;
    bool paused = false;
    float position[3] = { 0.0f, 0.0f, 0.0f };
    float min_distance = 1.0f;
    float max_distance = 100.0f;
    float effects_level = 0.0f;
};

struct BackendStream
{
    BackendSample *sample_owner = nullptr;
    MIX_Track *track = nullptr;
    std::array<uintptr_t, 8> user_data{};
    std::string name;
    MIX_Audio *audio = nullptr;
    SDL_AudioSpec format{};
    int duration_ms = 0;
    int volume = 127;
    int pan = 64;
    U32 loop_count = 1;
    int playback_rate = 0;
    int original_rate = 0;
    bool paused = false;
};

struct CallbackIOState
{
    uintptr_t file_handle = 0;
    Sint64 size = -1;
};

std::recursive_mutex g_audio_mutex;
RENEGADE_MILES_DIG_DRIVER *g_driver = nullptr;
MIX_Mixer *g_mixer = nullptr;
BackendProvider g_provider{ "SDL_mixer" };
std::string g_last_error;
AIL_FILE_OPEN_CALLBACK g_file_open = nullptr;
AIL_FILE_CLOSE_CALLBACK g_file_close = nullptr;
AIL_FILE_SEEK_CALLBACK g_file_seek = nullptr;
AIL_FILE_READ_CALLBACK g_file_read = nullptr;

void Set_Last_Error(const char *message)
{
    g_last_error = message != nullptr ? message : "unknown SDL_mixer audio error";
}

void Set_Last_Error_From_SDL(void)
{
    Set_Last_Error(SDL_GetError());
}

bool Ensure_SDL_Audio(void)
{
    if ((SDL_WasInit(SDL_INIT_AUDIO) & SDL_INIT_AUDIO) == 0) {
        if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) {
            Set_Last_Error_From_SDL();
            return false;
        }
    }

    if (!MIX_Init()) {
        Set_Last_Error_From_SDL();
        return false;
    }

    return true;
}

SDL_AudioFormat Get_SDL_Format(int bits)
{
    return (bits <= 8) ? SDL_AUDIO_U8 : SDL_AUDIO_S16LE;
}

bool Ensure_Mixer(LPWAVEFORMAT format = nullptr)
{
    if (g_mixer != nullptr) {
        return true;
    }

    if (!Ensure_SDL_Audio()) {
        return false;
    }

    SDL_AudioSpec spec{};
    spec.format = Get_SDL_Format(format != nullptr ? format->nBlockAlign * 8 / std::max<int>(format->nChannels, 1) : 16);
    spec.channels = static_cast<int>(format != nullptr ? format->nChannels : 2);
    spec.freq = static_cast<int>(format != nullptr ? format->nSamplesPerSec : 44100);

    g_mixer = MIX_CreateMixerDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec);
    if (g_mixer == nullptr) {
        Set_Last_Error_From_SDL();
        return false;
    }

    MIX_SetMixerGain(g_mixer, 1.0f);
    return true;
}

void Destroy_Mixer(void)
{
    if (g_mixer != nullptr) {
        MIX_DestroyMixer(g_mixer);
        g_mixer = nullptr;
    }
}

void Destroy_Sample_Audio(BackendSample *sample)
{
    if (sample == nullptr) {
        return;
    }

    if (sample->track != nullptr) {
        MIX_StopTrack(sample->track, 0);
        MIX_SetTrackAudio(sample->track, nullptr);
    }

    if (sample->audio != nullptr) {
        MIX_DestroyAudio(sample->audio);
        sample->audio = nullptr;
    }

    sample->format = SDL_AudioSpec{};
    sample->duration_ms = 0;
    sample->playback_rate = 0;
    sample->original_rate = 0;
    sample->paused = false;
}

void Destroy_Sample(BackendSample *sample)
{
    if (sample == nullptr) {
        return;
    }

    Destroy_Sample_Audio(sample);

    if (sample->track != nullptr) {
        MIX_DestroyTrack(sample->track);
        sample->track = nullptr;
    }

    delete sample;
}

void Destroy_Stream(BackendStream *stream)
{
    if (stream == nullptr) {
        return;
    }

    if (stream->track != nullptr) {
        MIX_StopTrack(stream->track, 0);
        MIX_DestroyTrack(stream->track);
        stream->track = nullptr;
    }

    if (stream->audio != nullptr) {
        MIX_DestroyAudio(stream->audio);
        stream->audio = nullptr;
    }

    delete stream;
}

float Clamp01(float value)
{
    return std::clamp(value, 0.0f, 1.0f);
}

void Apply_Pan_To_Gains(int pan, MIX_StereoGains &gains)
{
    const float normalized_pan = Clamp01(static_cast<float>(pan) / 127.0f);
    gains.left = Clamp01(1.0f - normalized_pan);
    gains.right = Clamp01(normalized_pan);
}

void Apply_3D_Gains(BackendSample *sample, MIX_StereoGains &gains, float &gain)
{
    if (sample == nullptr) {
        return;
    }

    const float x = sample->position[0];
    const float y = sample->position[1];
    const float z = sample->position[2];
    const float distance = std::sqrt((x * x) + (y * y) + (z * z));

    const float min_distance = std::max(sample->min_distance, 0.01f);
    const float max_distance = std::max(sample->max_distance, min_distance + 0.01f);

    float distance_gain = 1.0f;
    if (distance > min_distance) {
        distance_gain = 1.0f - ((distance - min_distance) / (max_distance - min_distance));
        distance_gain = Clamp01(distance_gain);
    }

    gain *= distance_gain;

    const float pan = std::clamp((x / std::max(distance, 1.0f)), -1.0f, 1.0f);
    gains.left = Clamp01(1.0f - std::max(pan, 0.0f));
    gains.right = Clamp01(1.0f + std::min(pan, 0.0f));
}

void Apply_Sample_State(BackendSample *sample)
{
    if ((sample == nullptr) || (sample->track == nullptr)) {
        return;
    }

    float gain = Clamp01(static_cast<float>(sample->volume) / 127.0f);
    MIX_StereoGains stereo{};
    Apply_Pan_To_Gains(sample->pan, stereo);

    if (sample->is_3d) {
        Apply_3D_Gains(sample, stereo, gain);
    }

    MIX_SetTrackGain(sample->track, gain);
    MIX_SetTrackStereo(sample->track, &stereo);

    if ((sample->original_rate > 0) && (sample->playback_rate > 0)) {
        MIX_SetTrackFrequencyRatio(sample->track, static_cast<float>(sample->playback_rate) / static_cast<float>(sample->original_rate));
    }
}

void Apply_Stream_State(BackendStream *stream)
{
    if ((stream == nullptr) || (stream->track == nullptr)) {
        return;
    }

    MIX_StereoGains stereo{};
    Apply_Pan_To_Gains(stream->pan, stereo);
    MIX_SetTrackGain(stream->track, Clamp01(static_cast<float>(stream->volume) / 127.0f));
    MIX_SetTrackStereo(stream->track, &stereo);

    if ((stream->original_rate > 0) && (stream->playback_rate > 0)) {
        MIX_SetTrackFrequencyRatio(stream->track, static_cast<float>(stream->playback_rate) / static_cast<float>(stream->original_rate));
    }
}

Sint64 Track_Position_To_MS(MIX_Track *track)
{
    if (track == nullptr) {
        return 0;
    }

    const Sint64 frames = MIX_GetTrackPlaybackPosition(track);
    if (frames < 0) {
        return 0;
    }

    const Sint64 ms = MIX_TrackFramesToMS(track, frames);
    return (ms >= 0) ? ms : 0;
}

Sint64 Track_Length_To_MS(MIX_Track *track)
{
    if (track == nullptr) {
        return 0;
    }

    const Sint64 current = MIX_GetTrackPlaybackPosition(track);
    const Sint64 remaining = MIX_GetTrackRemaining(track);
    if ((current < 0) || (remaining < 0)) {
        return 0;
    }

    const Sint64 length = MIX_TrackFramesToMS(track, current + remaining);
    return (length >= 0) ? length : 0;
}

int SDL_Audio_Bits(SDL_AudioFormat format)
{
    return SDL_AUDIO_BITSIZE(format);
}

U32 Bytes_Per_Frame(const SDL_AudioSpec &spec)
{
    return static_cast<U32>((std::max(spec.channels, 1) * std::max(SDL_Audio_Bits(spec.format), 8)) / 8);
}

U32 Frames_To_Bytes(Sint64 frames, const SDL_AudioSpec &spec)
{
    return static_cast<U32>(std::max<Sint64>(frames, 0) * Bytes_Per_Frame(spec));
}

Sint64 Bytes_To_Frames(U32 bytes, const SDL_AudioSpec &spec)
{
    const U32 bytes_per_frame = Bytes_Per_Frame(spec);
    return (bytes_per_frame > 0) ? (bytes / bytes_per_frame) : 0;
}

MIX_Audio *Load_Audio_No_Copy(const void *data, size_t bytes)
{
    if (!Ensure_Mixer()) {
        return nullptr;
    }

    MIX_Audio *audio = MIX_LoadAudioNoCopy(g_mixer, data, bytes, false);
    if (audio == nullptr) {
        Set_Last_Error_From_SDL();
    }

    return audio;
}

bool Populate_Audio_Metadata(MIX_Audio *audio, SDL_AudioSpec *format, int *duration_ms)
{
    if (audio == nullptr) {
        return false;
    }

    if ((format != nullptr) && !MIX_GetAudioFormat(audio, format)) {
        Set_Last_Error_From_SDL();
        return false;
    }

    if (duration_ms != nullptr) {
        const Sint64 frames = MIX_GetAudioDuration(audio);
        if (frames >= 0) {
            const Sint64 ms = MIX_AudioFramesToMS(audio, frames);
            *duration_ms = static_cast<int>(std::max<Sint64>(ms, 0));
        } else {
            *duration_ms = 0;
        }
    }

    return true;
}

bool Load_Sample_Audio(BackendSample *sample, const void *data, size_t bytes)
{
    if ((sample == nullptr) || (data == nullptr) || (bytes == 0)) {
        Set_Last_Error("invalid sample data");
        return false;
    }

    if (!Ensure_Mixer()) {
        return false;
    }

    if (sample->track == nullptr) {
        sample->track = MIX_CreateTrack(g_mixer);
        if (sample->track == nullptr) {
            Set_Last_Error_From_SDL();
            return false;
        }
    }

    Destroy_Sample_Audio(sample);

    sample->audio = Load_Audio_No_Copy(data, bytes);
    if (sample->audio == nullptr) {
        return false;
    }

    if (!Populate_Audio_Metadata(sample->audio, &sample->format, &sample->duration_ms)) {
        Destroy_Sample_Audio(sample);
        return false;
    }

    sample->original_rate = sample->format.freq;
    sample->playback_rate = sample->original_rate;

    if (!MIX_SetTrackAudio(sample->track, sample->audio)) {
        Set_Last_Error_From_SDL();
        Destroy_Sample_Audio(sample);
        return false;
    }

    Apply_Sample_State(sample);
    return true;
}

SDL_IOStream *Create_IO_From_Callbacks(const char *filename)
{
    if ((g_file_open == nullptr) || (g_file_close == nullptr) || (g_file_seek == nullptr) || (g_file_read == nullptr)) {
        Set_Last_Error("SDL_mixer file callbacks are not configured");
        return nullptr;
    }

    auto *state = new CallbackIOState;
    if (!g_file_open(filename, &state->file_handle)) {
        delete state;
        Set_Last_Error("Unable to open audio stream through file callbacks");
        return nullptr;
    }

    {
        const S32 current = g_file_seek(state->file_handle, 0, AIL_FILE_SEEK_CURRENT);
        const S32 end = g_file_seek(state->file_handle, 0, AIL_FILE_SEEK_END);
        if ((current >= 0) && (end >= 0)) {
            state->size = end;
        }
        if (current >= 0) {
            g_file_seek(state->file_handle, current, AIL_FILE_SEEK_BEGIN);
        }
    }

    SDL_IOStreamInterface iface;
    SDL_INIT_INTERFACE(&iface);
    iface.size = [](void *userdata) -> Sint64 {
        auto *state = static_cast<CallbackIOState *>(userdata);
        if (state->size >= 0) {
            return state->size;
        }
        const S32 current = g_file_seek(state->file_handle, 0, AIL_FILE_SEEK_CURRENT);
        const S32 end = g_file_seek(state->file_handle, 0, AIL_FILE_SEEK_END);
        if (current >= 0) {
            g_file_seek(state->file_handle, current, AIL_FILE_SEEK_BEGIN);
        }
        return end;
    };
    iface.seek = [](void *userdata, Sint64 offset, SDL_IOWhence whence) -> Sint64 {
        auto *state = static_cast<CallbackIOState *>(userdata);
        U32 seek_type = AIL_FILE_SEEK_BEGIN;
        if (whence == SDL_IO_SEEK_CUR) {
            seek_type = AIL_FILE_SEEK_CURRENT;
        } else if (whence == SDL_IO_SEEK_END) {
            seek_type = AIL_FILE_SEEK_END;
        }
        return g_file_seek(state->file_handle, static_cast<S32>(offset), seek_type);
    };
    iface.read = [](void *userdata, void *ptr, size_t size, SDL_IOStatus *status) -> size_t {
        auto *state = static_cast<CallbackIOState *>(userdata);
        const U32 bytes = g_file_read(state->file_handle, ptr, static_cast<U32>(size));
        if (status != nullptr && bytes < size) {
            const S32 current = g_file_seek(state->file_handle, 0, AIL_FILE_SEEK_CURRENT);
            const bool at_eof = (bytes == 0) || ((current >= 0) && (state->size >= 0) && (current >= state->size));
            *status = at_eof ? SDL_IO_STATUS_EOF : SDL_IO_STATUS_ERROR;
        }
        return bytes;
    };
    iface.write = [](void *, const void *, size_t, SDL_IOStatus *status) -> size_t {
        if (status != nullptr) {
            *status = SDL_IO_STATUS_READONLY;
        }
        return 0;
    };
    iface.flush = [](void *, SDL_IOStatus *status) -> bool {
        if (status != nullptr) {
            *status = SDL_IO_STATUS_READY;
        }
        return true;
    };
    iface.close = [](void *userdata) -> bool {
        auto *state = static_cast<CallbackIOState *>(userdata);
        g_file_close(state->file_handle);
        delete state;
        return true;
    };

    SDL_IOStream *stream = SDL_OpenIO(&iface, state);
    if (stream == nullptr) {
        g_file_close(state->file_handle);
        delete state;
        Set_Last_Error_From_SDL();
    }

    return stream;
}

bool Populate_Stream_Metadata(const char *filename, BackendStream *stream)
{
    if ((filename == nullptr) || (stream == nullptr)) {
        return false;
    }

    SDL_IOStream *io = Create_IO_From_Callbacks(filename);
    if (io == nullptr) {
        return false;
    }

    MIX_Audio *audio = MIX_LoadAudio_IO(g_mixer, io, false, true);
    if (audio == nullptr) {
        Set_Last_Error_From_SDL();
        return false;
    }

    const bool ok = Populate_Audio_Metadata(audio, &stream->format, &stream->duration_ms);
    if (ok) {
        stream->original_rate = stream->format.freq;
        stream->playback_rate = stream->original_rate;
    }
    MIX_DestroyAudio(audio);
    return ok;
}

bool Fallback_Stream_To_Loaded_Audio(BackendStream *stream)
{
    if ((stream == nullptr) || (stream->track == nullptr) || stream->name.empty()) {
        return false;
    }

    SDL_IOStream *io = Create_IO_From_Callbacks(stream->name.c_str());
    if (io == nullptr) {
        return false;
    }

    MIX_Audio *audio = MIX_LoadAudio_IO(g_mixer, io, false, true);
    if (audio == nullptr) {
        Set_Last_Error_From_SDL();
        return false;
    }

    if (!MIX_SetTrackAudio(stream->track, audio)) {
        MIX_DestroyAudio(audio);
        Set_Last_Error_From_SDL();
        return false;
    }

    if (stream->audio != nullptr) {
        MIX_DestroyAudio(stream->audio);
    }
    stream->audio = audio;

    if (Populate_Audio_Metadata(audio, &stream->format, &stream->duration_ms)) {
        stream->original_rate = stream->format.freq;
        stream->playback_rate = stream->original_rate;
    }

    Apply_Stream_State(stream);
    WWDEBUG_SAY(("WWAudio: Falling back to preloaded track input for %s\r\n", stream->name.c_str()));
    return true;
}

bool Parse_Wave_Info(const void *data, size_t data_len, AILSOUNDINFO *info, uint32_t *duration_ms)
{
    if ((data == nullptr) || (data_len < 44) || (info == nullptr)) {
        return false;
    }

    const auto *bytes = static_cast<const Uint8 *>(data);
    if ((std::memcmp(bytes, "RIFF", 4) != 0) || (std::memcmp(bytes + 8, "WAVE", 4) != 0)) {
        return false;
    }

    auto read_le16 = [](const Uint8 *src) -> Uint16 {
        Uint16 value = 0;
        std::memcpy(&value, src, sizeof(value));
        return SDL_Swap16LE(value);
    };
    auto read_le32 = [](const Uint8 *src) -> Uint32 {
        Uint32 value = 0;
        std::memcpy(&value, src, sizeof(value));
        return SDL_Swap32LE(value);
    };

    Uint16 format_tag = 0;
    Uint16 channels = 0;
    Uint32 sample_rate = 0;
    Uint16 bits_per_sample = 0;
    Uint32 data_size = 0;

    size_t offset = 12;
    while (offset + 8 <= data_len) {
        const Uint32 chunk_size = read_le32(bytes + offset + 4);
        const size_t chunk_data_offset = offset + 8;
        if (chunk_data_offset + chunk_size > data_len) {
            break;
        }

        if (std::memcmp(bytes + offset, "fmt ", 4) == 0 && chunk_size >= 16) {
            format_tag = read_le16(bytes + chunk_data_offset + 0);
            channels = read_le16(bytes + chunk_data_offset + 2);
            sample_rate = read_le32(bytes + chunk_data_offset + 4);
            bits_per_sample = read_le16(bytes + chunk_data_offset + 14);
        } else if (std::memcmp(bytes + offset, "data", 4) == 0) {
            data_size = chunk_size;
        }

        offset = chunk_data_offset + chunk_size + (chunk_size & 1u);
    }

    if ((channels == 0) || (sample_rate == 0) || (bits_per_sample == 0)) {
        return false;
    }

    info->format = format_tag;
    info->rate = static_cast<S32>(sample_rate);
    info->bits = static_cast<S32>(bits_per_sample);
    info->channels = static_cast<S32>(channels);
    info->data_len = data_size;
    if ((duration_ms != nullptr) && (data_size > 0) && (bits_per_sample > 0) && (channels > 0)) {
        const float bytes_per_second = static_cast<float>((sample_rate * channels * bits_per_sample) / 8);
        *duration_ms = (bytes_per_second > 0.0f) ? static_cast<uint32_t>((static_cast<float>(data_size) / bytes_per_second) * 1000.0f) : 0;
    }

    return true;
}
} // namespace

bool WWAudio_Get_Audio_Info_From_Memory(const void *data, size_t data_len, AILSOUNDINFO *info, uint32_t *duration_ms)
{
    std::lock_guard<std::recursive_mutex> guard(g_audio_mutex);

    if ((data == nullptr) || (data_len == 0) || (info == nullptr)) {
        return false;
    }

    std::memset(info, 0, sizeof(*info));
    if (duration_ms != nullptr) {
        *duration_ms = 0;
    }

    if (Parse_Wave_Info(data, data_len, info, duration_ms)) {
        return true;
    }

    MIX_Audio *audio = nullptr;
    if (Ensure_SDL_Audio()) {
        audio = MIX_LoadAudioNoCopy(g_mixer, data, data_len, false);
    }

    if (audio == nullptr) {
        return false;
    }

    SDL_AudioSpec format{};
    if (!Populate_Audio_Metadata(audio, &format, reinterpret_cast<int *>(duration_ms))) {
        MIX_DestroyAudio(audio);
        return false;
    }

    info->format = WAVE_FORMAT_IMA_ADPCM;
    info->rate = format.freq;
    info->bits = SDL_Audio_Bits(format.format);
    info->channels = format.channels;
    MIX_DestroyAudio(audio);
    return true;
}

void AIL_lock(void)
{
    g_audio_mutex.lock();
}

void AIL_unlock(void)
{
    g_audio_mutex.unlock();
}

void AIL_startup(void)
{
    std::lock_guard<std::recursive_mutex> guard(g_audio_mutex);
    Ensure_SDL_Audio();
}

void AIL_shutdown(void)
{
    std::lock_guard<std::recursive_mutex> guard(g_audio_mutex);
    Destroy_Mixer();
    MIX_Quit();
}

S32 AIL_set_preference(U32, S32)
{
    return AIL_NO_ERROR;
}

S32 AIL_waveOutOpen(HDIGDRIVER *driver, void *, U32, LPWAVEFORMAT format)
{
    std::lock_guard<std::recursive_mutex> guard(g_audio_mutex);

    Destroy_Mixer();
    if (!Ensure_Mixer(format)) {
        return -1;
    }

    delete g_driver;
    g_driver = new RENEGADE_MILES_DIG_DRIVER{};
    g_driver->emulated_ds = FALSE;
    if (driver != nullptr) {
        *driver = g_driver;
    }
    return AIL_NO_ERROR;
}

void AIL_waveOutClose(HDIGDRIVER driver)
{
    std::lock_guard<std::recursive_mutex> guard(g_audio_mutex);
    if (driver == g_driver) {
        delete g_driver;
        g_driver = nullptr;
    }
    Destroy_Mixer();
}

HSAMPLE AIL_allocate_sample_handle(HDIGDRIVER)
{
    std::lock_guard<std::recursive_mutex> guard(g_audio_mutex);
    if (!Ensure_Mixer()) {
        return nullptr;
    }
    return new BackendSample(false);
}

void AIL_release_sample_handle(HSAMPLE sample)
{
    std::lock_guard<std::recursive_mutex> guard(g_audio_mutex);
    Destroy_Sample(static_cast<BackendSample *>(sample));
}

void AIL_init_sample(HSAMPLE sample)
{
    std::lock_guard<std::recursive_mutex> guard(g_audio_mutex);
    Destroy_Sample_Audio(static_cast<BackendSample *>(sample));
}

void AIL_set_named_sample_file(HSAMPLE sample, char *, void const *data, U32 bytes, U32)
{
    std::lock_guard<std::recursive_mutex> guard(g_audio_mutex);
    Load_Sample_Audio(static_cast<BackendSample *>(sample), data, bytes);
}

void AIL_start_sample(HSAMPLE sample)
{
    std::lock_guard<std::recursive_mutex> guard(g_audio_mutex);
    auto *backend = static_cast<BackendSample *>(sample);
    if ((backend == nullptr) || (backend->track == nullptr)) {
        return;
    }

    SDL_PropertiesID props = SDL_CreateProperties();
    SDL_SetNumberProperty(props, MIX_PROP_PLAY_LOOPS_NUMBER, backend->loop_count == 0 ? -1 : static_cast<Sint64>(std::max<int>(static_cast<int>(backend->loop_count) - 1, 0)));
    MIX_PlayTrack(backend->track, props);
    SDL_DestroyProperties(props);
    backend->paused = false;
    Apply_Sample_State(backend);
}

void AIL_stop_sample(HSAMPLE sample)
{
    std::lock_guard<std::recursive_mutex> guard(g_audio_mutex);
    auto *backend = static_cast<BackendSample *>(sample);
    if ((backend != nullptr) && (backend->track != nullptr)) {
        MIX_StopTrack(backend->track, 0);
        backend->paused = false;
    }
}

void AIL_resume_sample(HSAMPLE sample)
{
    std::lock_guard<std::recursive_mutex> guard(g_audio_mutex);
    auto *backend = static_cast<BackendSample *>(sample);
    if ((backend != nullptr) && (backend->track != nullptr)) {
        MIX_ResumeTrack(backend->track);
        backend->paused = false;
    }
}

void AIL_end_sample(HSAMPLE sample)
{
    std::lock_guard<std::recursive_mutex> guard(g_audio_mutex);
    Destroy_Sample_Audio(static_cast<BackendSample *>(sample));
}

void AIL_set_sample_pan(HSAMPLE sample, S32 pan)
{
    std::lock_guard<std::recursive_mutex> guard(g_audio_mutex);
    auto *backend = static_cast<BackendSample *>(sample);
    if (backend != nullptr) {
        backend->pan = pan;
        Apply_Sample_State(backend);
    }
}

S32 AIL_sample_pan(HSAMPLE sample)
{
    std::lock_guard<std::recursive_mutex> guard(g_audio_mutex);
    auto *backend = static_cast<BackendSample *>(sample);
    return (backend != nullptr) ? backend->pan : 64;
}

void AIL_set_sample_volume(HSAMPLE sample, S32 volume)
{
    std::lock_guard<std::recursive_mutex> guard(g_audio_mutex);
    auto *backend = static_cast<BackendSample *>(sample);
    if (backend != nullptr) {
        backend->volume = volume;
        Apply_Sample_State(backend);
    }
}

S32 AIL_sample_volume(HSAMPLE sample)
{
    std::lock_guard<std::recursive_mutex> guard(g_audio_mutex);
    auto *backend = static_cast<BackendSample *>(sample);
    return (backend != nullptr) ? backend->volume : 0;
}

void AIL_set_sample_loop_count(HSAMPLE sample, U32 count)
{
    std::lock_guard<std::recursive_mutex> guard(g_audio_mutex);
    auto *backend = static_cast<BackendSample *>(sample);
    if (backend != nullptr) {
        backend->loop_count = count;
    }
}

U32 AIL_sample_loop_count(HSAMPLE sample)
{
    std::lock_guard<std::recursive_mutex> guard(g_audio_mutex);
    auto *backend = static_cast<BackendSample *>(sample);
    return (backend != nullptr) ? backend->loop_count : 0;
}

void AIL_set_sample_ms_position(HSAMPLE sample, U32 ms)
{
    std::lock_guard<std::recursive_mutex> guard(g_audio_mutex);
    auto *backend = static_cast<BackendSample *>(sample);
    if ((backend != nullptr) && (backend->track != nullptr)) {
        const Sint64 frames = MIX_TrackMSToFrames(backend->track, ms);
        if (frames >= 0) {
            MIX_SetTrackPlaybackPosition(backend->track, frames);
        }
    }
}

void AIL_sample_ms_position(HSAMPLE sample, S32 *len, S32 *pos)
{
    std::lock_guard<std::recursive_mutex> guard(g_audio_mutex);
    auto *backend = static_cast<BackendSample *>(sample);
    if ((backend == nullptr) || (backend->track == nullptr)) {
        return;
    }

    if (len != nullptr) {
        *len = static_cast<S32>(Track_Length_To_MS(backend->track));
    }
    if (pos != nullptr) {
        *pos = static_cast<S32>(Track_Position_To_MS(backend->track));
    }
}

void AIL_set_sample_user_data(HSAMPLE sample, S32 index, uintptr_t value)
{
    std::lock_guard<std::recursive_mutex> guard(g_audio_mutex);
    auto *backend = static_cast<BackendSample *>(sample);
    if ((backend != nullptr) && (index >= 0) && (index < static_cast<S32>(backend->user_data.size()))) {
        backend->user_data[index] = value;
    }
}

uintptr_t AIL_sample_user_data(HSAMPLE sample, S32 index)
{
    std::lock_guard<std::recursive_mutex> guard(g_audio_mutex);
    auto *backend = static_cast<BackendSample *>(sample);
    if ((backend != nullptr) && (index >= 0) && (index < static_cast<S32>(backend->user_data.size()))) {
        return backend->user_data[index];
    }
    return 0;
}

S32 AIL_sample_playback_rate(HSAMPLE sample)
{
    std::lock_guard<std::recursive_mutex> guard(g_audio_mutex);
    auto *backend = static_cast<BackendSample *>(sample);
    return (backend != nullptr) ? backend->playback_rate : 0;
}

void AIL_set_sample_playback_rate(HSAMPLE sample, S32 rate)
{
    std::lock_guard<std::recursive_mutex> guard(g_audio_mutex);
    auto *backend = static_cast<BackendSample *>(sample);
    if (backend != nullptr) {
        backend->playback_rate = rate;
        Apply_Sample_State(backend);
    }
}

void AIL_set_sample_processor(HSAMPLE, U32, HPROVIDER)
{
}

void AIL_set_filter_sample_preference(HSAMPLE, char const *, void const *)
{
}

H3DSAMPLE AIL_allocate_3D_sample_handle(HPROVIDER)
{
    std::lock_guard<std::recursive_mutex> guard(g_audio_mutex);
    if (!Ensure_Mixer()) {
        return nullptr;
    }
    return new BackendSample(true);
}

void AIL_release_3D_sample_handle(H3DSAMPLE sample)
{
    std::lock_guard<std::recursive_mutex> guard(g_audio_mutex);
    Destroy_Sample(static_cast<BackendSample *>(sample));
}

U32 AIL_set_3D_sample_file(H3DSAMPLE sample, void const *data, U32 bytes)
{
    std::lock_guard<std::recursive_mutex> guard(g_audio_mutex);
    return Load_Sample_Audio(static_cast<BackendSample *>(sample), data, bytes) ? 1u : 0u;
}

void AIL_start_3D_sample(H3DSAMPLE sample)
{
    AIL_start_sample(sample);
}

void AIL_stop_3D_sample(H3DSAMPLE sample)
{
    AIL_stop_sample(sample);
}

void AIL_resume_3D_sample(H3DSAMPLE sample)
{
    AIL_resume_sample(sample);
}

void AIL_end_3D_sample(H3DSAMPLE sample)
{
    AIL_end_sample(sample);
}

void AIL_set_3D_sample_volume(H3DSAMPLE sample, S32 volume)
{
    AIL_set_sample_volume(sample, volume);
}

S32 AIL_3D_sample_volume(H3DSAMPLE sample)
{
    return AIL_sample_volume(sample);
}

void AIL_set_3D_sample_loop_count(H3DSAMPLE sample, U32 count)
{
    AIL_set_sample_loop_count(sample, count);
}

U32 AIL_3D_sample_loop_count(H3DSAMPLE sample)
{
    return AIL_sample_loop_count(sample);
}

void AIL_set_3D_sample_offset(H3DSAMPLE sample, U32 bytes)
{
    std::lock_guard<std::recursive_mutex> guard(g_audio_mutex);
    auto *backend = static_cast<BackendSample *>(sample);
    if ((backend != nullptr) && (backend->track != nullptr)) {
        MIX_SetTrackPlaybackPosition(backend->track, Bytes_To_Frames(bytes, backend->format));
    }
}

U32 AIL_3D_sample_offset(H3DSAMPLE sample)
{
    std::lock_guard<std::recursive_mutex> guard(g_audio_mutex);
    auto *backend = static_cast<BackendSample *>(sample);
    if ((backend == nullptr) || (backend->track == nullptr)) {
        return 0;
    }
    return Frames_To_Bytes(MIX_GetTrackPlaybackPosition(backend->track), backend->format);
}

U32 AIL_3D_sample_length(H3DSAMPLE sample)
{
    std::lock_guard<std::recursive_mutex> guard(g_audio_mutex);
    auto *backend = static_cast<BackendSample *>(sample);
    if ((backend == nullptr) || (backend->track == nullptr)) {
        return 0;
    }
    return Frames_To_Bytes(MIX_GetTrackPlaybackPosition(backend->track) + MIX_GetTrackRemaining(backend->track), backend->format);
}

void AIL_set_3D_object_user_data(H3DSAMPLE sample, S32 index, uintptr_t value)
{
    AIL_set_sample_user_data(sample, index, value);
}

uintptr_t AIL_3D_object_user_data(H3DSAMPLE sample, S32 index)
{
    return AIL_sample_user_data(sample, index);
}

S32 AIL_3D_sample_playback_rate(H3DSAMPLE sample)
{
    return AIL_sample_playback_rate(sample);
}

void AIL_set_3D_sample_playback_rate(H3DSAMPLE sample, S32 rate)
{
    AIL_set_sample_playback_rate(sample, rate);
}

void AIL_set_3D_position(H3DSAMPLE sample, float x, float y, float z)
{
    std::lock_guard<std::recursive_mutex> guard(g_audio_mutex);
    auto *backend = static_cast<BackendSample *>(sample);
    if (backend != nullptr) {
        backend->position[0] = x;
        backend->position[1] = y;
        backend->position[2] = z;
        Apply_Sample_State(backend);
    }
}

void AIL_set_3D_orientation(H3DSAMPLE, float, float, float, float, float, float)
{
}

void AIL_set_3D_velocity_vector(H3DSAMPLE, float, float, float, float)
{
}

void AIL_set_3D_velocity_vector(H3DSAMPLE, float, float, float)
{
}

void AIL_set_3D_sample_distances(H3DSAMPLE sample, float max_distance, float min_distance, int)
{
    std::lock_guard<std::recursive_mutex> guard(g_audio_mutex);
    auto *backend = static_cast<BackendSample *>(sample);
    if (backend != nullptr) {
        backend->max_distance = max_distance;
        backend->min_distance = min_distance;
        Apply_Sample_State(backend);
    }
}

void AIL_set_3D_sample_distances(H3DSAMPLE sample, float max_distance, float min_distance)
{
    AIL_set_3D_sample_distances(sample, max_distance, min_distance, 0);
}

void AIL_set_3D_sample_effects_level(H3DSAMPLE sample, float effects_level)
{
    std::lock_guard<std::recursive_mutex> guard(g_audio_mutex);
    auto *backend = static_cast<BackendSample *>(sample);
    if (backend != nullptr) {
        backend->effects_level = effects_level;
    }
}

HSTREAM AIL_open_stream_by_sample(HDIGDRIVER, HSAMPLE sample, char const *filename, U32)
{
    std::lock_guard<std::recursive_mutex> guard(g_audio_mutex);
    if (!Ensure_Mixer()) {
        return nullptr;
    }

    auto *stream = new BackendStream;
    stream->sample_owner = static_cast<BackendSample *>(sample);
    stream->name = (filename != nullptr) ? filename : "";
    stream->track = MIX_CreateTrack(g_mixer);
    if (stream->track == nullptr) {
        Set_Last_Error_From_SDL();
        delete stream;
        return nullptr;
    }

    SDL_IOStream *io = Create_IO_From_Callbacks(filename);
    if (io == nullptr) {
        Destroy_Stream(stream);
        return nullptr;
    }

    if (!MIX_SetTrackIOStream(stream->track, io, true)) {
        Set_Last_Error_From_SDL();
        Destroy_Stream(stream);
        return nullptr;
    }

    Populate_Stream_Metadata(filename, stream);
    Apply_Stream_State(stream);
    return stream;
}

void AIL_start_stream(HSTREAM stream)
{
    std::lock_guard<std::recursive_mutex> guard(g_audio_mutex);
    auto *backend = static_cast<BackendStream *>(stream);
    if ((backend == nullptr) || (backend->track == nullptr)) {
        return;
    }

    SDL_PropertiesID props = SDL_CreateProperties();
    SDL_SetNumberProperty(props, MIX_PROP_PLAY_LOOPS_NUMBER, backend->loop_count == 0 ? -1 : static_cast<Sint64>(std::max<int>(static_cast<int>(backend->loop_count) - 1, 0)));
    if (!MIX_PlayTrack(backend->track, props)) {
        Set_Last_Error_From_SDL();
        if (!Fallback_Stream_To_Loaded_Audio(backend) || !MIX_PlayTrack(backend->track, props)) {
            Set_Last_Error_From_SDL();
            WWDEBUG_SAY(("WWAudio: Failed to start streamed track %s (%s)\r\n", backend->name.c_str(), g_last_error.c_str()));
            SDL_DestroyProperties(props);
            return;
        }
    }
    SDL_DestroyProperties(props);
    backend->paused = false;
    Apply_Stream_State(backend);
}

void AIL_pause_stream(HSTREAM stream, S32 onoff)
{
    std::lock_guard<std::recursive_mutex> guard(g_audio_mutex);
    auto *backend = static_cast<BackendStream *>(stream);
    if ((backend == nullptr) || (backend->track == nullptr)) {
        return;
    }

    if (onoff) {
        MIX_PauseTrack(backend->track);
        backend->paused = true;
    } else {
        MIX_ResumeTrack(backend->track);
        backend->paused = false;
    }
}

void AIL_close_stream(HSTREAM stream)
{
    std::lock_guard<std::recursive_mutex> guard(g_audio_mutex);
    Destroy_Stream(static_cast<BackendStream *>(stream));
}

void AIL_set_stream_pan(HSTREAM stream, S32 pan)
{
    std::lock_guard<std::recursive_mutex> guard(g_audio_mutex);
    auto *backend = static_cast<BackendStream *>(stream);
    if (backend != nullptr) {
        backend->pan = pan;
        Apply_Stream_State(backend);
    }
}

S32 AIL_stream_pan(HSTREAM stream)
{
    std::lock_guard<std::recursive_mutex> guard(g_audio_mutex);
    auto *backend = static_cast<BackendStream *>(stream);
    return (backend != nullptr) ? backend->pan : 64;
}

void AIL_set_stream_volume(HSTREAM stream, S32 volume)
{
    std::lock_guard<std::recursive_mutex> guard(g_audio_mutex);
    auto *backend = static_cast<BackendStream *>(stream);
    if (backend != nullptr) {
        backend->volume = volume;
        Apply_Stream_State(backend);
    }
}

S32 AIL_stream_volume(HSTREAM stream)
{
    std::lock_guard<std::recursive_mutex> guard(g_audio_mutex);
    auto *backend = static_cast<BackendStream *>(stream);
    return (backend != nullptr) ? backend->volume : 0;
}

void AIL_set_stream_loop_block(HSTREAM, S32, S32)
{
}

void AIL_set_stream_loop_count(HSTREAM stream, U32 count)
{
    std::lock_guard<std::recursive_mutex> guard(g_audio_mutex);
    auto *backend = static_cast<BackendStream *>(stream);
    if (backend != nullptr) {
        backend->loop_count = count;
    }
}

U32 AIL_stream_loop_count(HSTREAM stream)
{
    std::lock_guard<std::recursive_mutex> guard(g_audio_mutex);
    auto *backend = static_cast<BackendStream *>(stream);
    return (backend != nullptr) ? backend->loop_count : 0;
}

void AIL_set_stream_ms_position(HSTREAM stream, U32 ms)
{
    std::lock_guard<std::recursive_mutex> guard(g_audio_mutex);
    auto *backend = static_cast<BackendStream *>(stream);
    if ((backend != nullptr) && (backend->track != nullptr)) {
        const Sint64 frames = MIX_TrackMSToFrames(backend->track, ms);
        if (frames >= 0) {
            MIX_SetTrackPlaybackPosition(backend->track, frames);
        }
    }
}

void AIL_stream_ms_position(HSTREAM stream, S32 *len, S32 *pos)
{
    std::lock_guard<std::recursive_mutex> guard(g_audio_mutex);
    auto *backend = static_cast<BackendStream *>(stream);
    if ((backend == nullptr) || (backend->track == nullptr)) {
        return;
    }

    if (len != nullptr) {
        *len = static_cast<S32>(backend->duration_ms > 0 ? backend->duration_ms : Track_Length_To_MS(backend->track));
    }
    if (pos != nullptr) {
        *pos = static_cast<S32>(Track_Position_To_MS(backend->track));
    }
}

S32 AIL_stream_playback_rate(HSTREAM stream)
{
    std::lock_guard<std::recursive_mutex> guard(g_audio_mutex);
    auto *backend = static_cast<BackendStream *>(stream);
    return (backend != nullptr) ? backend->playback_rate : 0;
}

void AIL_set_stream_playback_rate(HSTREAM stream, S32 rate)
{
    std::lock_guard<std::recursive_mutex> guard(g_audio_mutex);
    auto *backend = static_cast<BackendStream *>(stream);
    if (backend != nullptr) {
        backend->playback_rate = rate;
        Apply_Stream_State(backend);
    }
}

H3DPOBJECT AIL_3D_open_listener(HPROVIDER provider)
{
    return provider;
}

S32 AIL_enumerate_3D_providers(HPROENUM *next, HPROVIDER *provider, char **name)
{
    if ((next == nullptr) || (provider == nullptr) || (name == nullptr) || (*next != HPROENUM_FIRST)) {
        return 0;
    }

    *provider = &g_provider;
    *name = const_cast<char *>(g_provider.name);
    *next = 1;
    return 1;
}

S32 AIL_open_3D_provider(HPROVIDER)
{
    std::lock_guard<std::recursive_mutex> guard(g_audio_mutex);
    return Ensure_Mixer() ? M3D_NOERR : -1;
}

void AIL_close_3D_provider(HPROVIDER)
{
}

void AIL_set_3D_speaker_type(HPROVIDER, S32)
{
}

S32 AIL_enumerate_filters(HPROENUM *, HPROVIDER *, char **)
{
    return 0;
}

char *AIL_last_error(void)
{
    return const_cast<char *>(g_last_error.c_str());
}

void AIL_set_file_callbacks(AIL_FILE_OPEN_CALLBACK open_cb, AIL_FILE_CLOSE_CALLBACK close_cb, AIL_FILE_SEEK_CALLBACK seek_cb, AIL_FILE_READ_CALLBACK read_cb)
{
    std::lock_guard<std::recursive_mutex> guard(g_audio_mutex);
    g_file_open = open_cb;
    g_file_close = close_cb;
    g_file_seek = seek_cb;
    g_file_read = read_cb;
}

HTIMER AIL_allocate_timer_handle(void)
{
    return -1;
}

void AIL_start_timer(HTIMER)
{
}

void AIL_stop_timer(HTIMER)
{
}

void AIL_release_timer_handle(HTIMER)
{
}

U32 AIL_WAV_info(void const *, AILSOUNDINFO *)
{
    return 0;
}
