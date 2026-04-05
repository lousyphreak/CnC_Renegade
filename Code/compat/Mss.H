#pragma once

#include "windows.h"
#include "bittype.h"

#ifdef _WIN32
#include <mmreg.h>
#endif

#include <cstddef>
#include <cstdint>

using U32 = uint32_t;
using S32 = int32_t;
using HPROVIDER = void *;
using HPROENUM = int;

struct RENEGADE_MILES_DIG_DRIVER {
	int32_t emulated_ds;
};

using HDIGDRIVER = RENEGADE_MILES_DIG_DRIVER *;
using HSAMPLE = void *;
using H3DSAMPLE = void *;
using HSTREAM = void *;
using H3DPOBJECT = void *;
using HTIMER = int32_t;

#ifndef _WIN32
struct WAVEFORMAT {
	uint16_t wFormatTag;
	uint16_t nChannels;
	uint32_t nSamplesPerSec;
	uint32_t nAvgBytesPerSec;
	uint16_t nBlockAlign;
};

struct PCMWAVEFORMAT {
	WAVEFORMAT wf;
	uint16_t wBitsPerSample;
};
#endif

using LPWAVEFORMAT = WAVEFORMAT *;
using LPCWAVEFORMAT = const WAVEFORMAT *;

struct AILSOUNDINFO {
	U32 format;
	const void *data_ptr;
	U32 data_len;
	S32 rate;
	S32 bits;
	S32 channels;
	S32 samples;
	U32 block_size;
	const void *initial_ptr;
};

#ifndef AILCALLBACK
#define AILCALLBACK __stdcall
#endif

#ifndef WAVE_FORMAT_PCM
#define WAVE_FORMAT_PCM 1
#endif

#ifndef WAVE_FORMAT_IMA_ADPCM
#define WAVE_FORMAT_IMA_ADPCM 0x0011
#endif

#ifndef AIL_NO_ERROR
#define AIL_NO_ERROR 0
#endif

#ifndef AIL_LOCK_PROTECTION
#define AIL_LOCK_PROTECTION 0
#endif

#ifndef AIL_3D_2_SPEAKER
#define AIL_3D_2_SPEAKER 0
#endif

#ifndef AIL_3D_HEADPHONE
#define AIL_3D_HEADPHONE 1
#endif

#ifndef AIL_3D_SURROUND
#define AIL_3D_SURROUND 2
#endif

#ifndef AIL_3D_4_SPEAKER
#define AIL_3D_4_SPEAKER 3
#endif

#ifndef DIG_USE_WAVEOUT
#define DIG_USE_WAVEOUT 0
#endif

#ifndef ENVIRONMENT_GENERIC
#define ENVIRONMENT_GENERIC 0
#endif

#ifndef NO
#define NO 0
#endif

#ifndef DP_FILTER
#define DP_FILTER 0
#endif

#ifndef HPROENUM_FIRST
#define HPROENUM_FIRST 0
#endif

#ifndef M3D_NOERR
#define M3D_NOERR 0
#endif

#ifndef AIL_FILE_SEEK_BEGIN
#define AIL_FILE_SEEK_BEGIN 0
#endif

#ifndef AIL_FILE_SEEK_CURRENT
#define AIL_FILE_SEEK_CURRENT 1
#endif

#ifndef AIL_FILE_SEEK_END
#define AIL_FILE_SEEK_END 2
#endif

using AIL_FILE_OPEN_CALLBACK = U32 (AILCALLBACK *)(char const *filename, uintptr_t *file_handle);
using AIL_FILE_CLOSE_CALLBACK = void (AILCALLBACK *)(uintptr_t file_handle);
using AIL_FILE_SEEK_CALLBACK = S32 (AILCALLBACK *)(uintptr_t file_handle, S32 offset, U32 type);
using AIL_FILE_READ_CALLBACK = U32 (AILCALLBACK *)(uintptr_t file_handle, void *buffer, U32 bytes);

#if defined(WWAUDIO_USE_SDL_MIXER_BACKEND)
void AIL_lock(void);
void AIL_unlock(void);

void AIL_startup(void);
void AIL_shutdown(void);
S32 AIL_set_preference(U32 preference, S32 value);
S32 AIL_waveOutOpen(HDIGDRIVER *driver, void *, U32, LPWAVEFORMAT format);
void AIL_waveOutClose(HDIGDRIVER driver);

HSAMPLE AIL_allocate_sample_handle(HDIGDRIVER driver);
void AIL_release_sample_handle(HSAMPLE sample);
void AIL_init_sample(HSAMPLE sample);
void AIL_set_named_sample_file(HSAMPLE sample, char *filename, void const *data, U32 bytes, U32 block);
void AIL_start_sample(HSAMPLE sample);
void AIL_stop_sample(HSAMPLE sample);
void AIL_resume_sample(HSAMPLE sample);
void AIL_end_sample(HSAMPLE sample);
void AIL_set_sample_pan(HSAMPLE sample, S32 pan);
S32 AIL_sample_pan(HSAMPLE sample);
void AIL_set_sample_volume(HSAMPLE sample, S32 volume);
S32 AIL_sample_volume(HSAMPLE sample);
void AIL_set_sample_loop_count(HSAMPLE sample, U32 count);
U32 AIL_sample_loop_count(HSAMPLE sample);
void AIL_set_sample_ms_position(HSAMPLE sample, U32 ms);
void AIL_sample_ms_position(HSAMPLE sample, S32 *len, S32 *pos);
void AIL_set_sample_user_data(HSAMPLE sample, S32 index, uintptr_t value);
uintptr_t AIL_sample_user_data(HSAMPLE sample, S32 index);
S32 AIL_sample_playback_rate(HSAMPLE sample);
void AIL_set_sample_playback_rate(HSAMPLE sample, S32 rate);
void AIL_set_sample_processor(HSAMPLE sample, U32 pipeline, HPROVIDER filter);
void AIL_set_filter_sample_preference(HSAMPLE sample, char const *name, void const *value);

H3DSAMPLE AIL_allocate_3D_sample_handle(HPROVIDER provider);
void AIL_release_3D_sample_handle(H3DSAMPLE sample);
U32 AIL_set_3D_sample_file(H3DSAMPLE sample, void const *data, U32 bytes);
void AIL_start_3D_sample(H3DSAMPLE sample);
void AIL_stop_3D_sample(H3DSAMPLE sample);
void AIL_resume_3D_sample(H3DSAMPLE sample);
void AIL_end_3D_sample(H3DSAMPLE sample);
void AIL_set_3D_sample_volume(H3DSAMPLE sample, S32 volume);
S32 AIL_3D_sample_volume(H3DSAMPLE sample);
void AIL_set_3D_sample_loop_count(H3DSAMPLE sample, U32 count);
U32 AIL_3D_sample_loop_count(H3DSAMPLE sample);
void AIL_set_3D_sample_offset(H3DSAMPLE sample, U32 bytes);
U32 AIL_3D_sample_offset(H3DSAMPLE sample);
U32 AIL_3D_sample_length(H3DSAMPLE sample);
void AIL_set_3D_object_user_data(H3DSAMPLE sample, S32 index, uintptr_t value);
uintptr_t AIL_3D_object_user_data(H3DSAMPLE sample, S32 index);
S32 AIL_3D_sample_playback_rate(H3DSAMPLE sample);
void AIL_set_3D_sample_playback_rate(H3DSAMPLE sample, S32 rate);
void AIL_set_3D_position(H3DSAMPLE sample, float x, float y, float z);
void AIL_set_3D_orientation(H3DSAMPLE sample, float face_x, float face_y, float face_z, float up_x, float up_y, float up_z);
void AIL_set_3D_velocity_vector(H3DSAMPLE sample, float x, float y, float z, float magnitude);
void AIL_set_3D_velocity_vector(H3DSAMPLE sample, float x, float y, float z);
void AIL_set_3D_sample_distances(H3DSAMPLE sample, float max_distance, float min_distance, int auto_update);
void AIL_set_3D_sample_distances(H3DSAMPLE sample, float max_distance, float min_distance);
void AIL_set_3D_sample_effects_level(H3DSAMPLE sample, float effects_level);

HSTREAM AIL_open_stream_by_sample(HDIGDRIVER driver, HSAMPLE sample, char const *filename, U32 stream_mem);
void AIL_start_stream(HSTREAM stream);
void AIL_pause_stream(HSTREAM stream, S32 onoff);
void AIL_close_stream(HSTREAM stream);
void AIL_set_stream_pan(HSTREAM stream, S32 pan);
S32 AIL_stream_pan(HSTREAM stream);
void AIL_set_stream_volume(HSTREAM stream, S32 volume);
S32 AIL_stream_volume(HSTREAM stream);
void AIL_set_stream_loop_block(HSTREAM stream, S32 start, S32 end);
void AIL_set_stream_loop_count(HSTREAM stream, U32 count);
U32 AIL_stream_loop_count(HSTREAM stream);
void AIL_set_stream_ms_position(HSTREAM stream, U32 ms);
void AIL_stream_ms_position(HSTREAM stream, S32 *len, S32 *pos);
S32 AIL_stream_playback_rate(HSTREAM stream);
void AIL_set_stream_playback_rate(HSTREAM stream, S32 rate);

H3DPOBJECT AIL_3D_open_listener(HPROVIDER provider);
S32 AIL_enumerate_3D_providers(HPROENUM *next, HPROVIDER *provider, char **name);
S32 AIL_open_3D_provider(HPROVIDER provider);
void AIL_close_3D_provider(HPROVIDER provider);
void AIL_set_3D_speaker_type(HPROVIDER provider, S32 speaker_type);
S32 AIL_enumerate_filters(HPROENUM *next, HPROVIDER *provider, char **name);
char *AIL_last_error(void);

void AIL_set_file_callbacks(AIL_FILE_OPEN_CALLBACK open_cb, AIL_FILE_CLOSE_CALLBACK close_cb, AIL_FILE_SEEK_CALLBACK seek_cb, AIL_FILE_READ_CALLBACK read_cb);
HTIMER AIL_allocate_timer_handle(void);
void AIL_start_timer(HTIMER timer);
void AIL_stop_timer(HTIMER timer);
void AIL_release_timer_handle(HTIMER timer);
U32 AIL_WAV_info(void const *data, AILSOUNDINFO *info);
#else
inline void AIL_lock(void) {}
inline void AIL_unlock(void) {}
inline void AIL_startup(void) {}
inline void AIL_shutdown(void) {}
inline S32 AIL_set_preference(U32, S32) { return AIL_NO_ERROR; }
inline S32 AIL_waveOutOpen(HDIGDRIVER *, void *, U32, LPWAVEFORMAT) { return AIL_NO_ERROR; }
inline void AIL_waveOutClose(HDIGDRIVER) {}
inline HSAMPLE AIL_allocate_sample_handle(HDIGDRIVER) { return nullptr; }
inline void AIL_release_sample_handle(HSAMPLE) {}
inline void AIL_init_sample(HSAMPLE) {}
inline void AIL_set_named_sample_file(HSAMPLE, char *, void const *, U32, U32) {}
inline void AIL_start_sample(HSAMPLE) {}
inline void AIL_stop_sample(HSAMPLE) {}
inline void AIL_resume_sample(HSAMPLE) {}
inline void AIL_end_sample(HSAMPLE) {}
inline void AIL_set_sample_pan(HSAMPLE, S32) {}
inline S32 AIL_sample_pan(HSAMPLE) { return 64; }
inline void AIL_set_sample_volume(HSAMPLE, S32) {}
inline S32 AIL_sample_volume(HSAMPLE) { return 0; }
inline void AIL_set_sample_loop_count(HSAMPLE, U32) {}
inline U32 AIL_sample_loop_count(HSAMPLE) { return 0; }
inline void AIL_set_sample_ms_position(HSAMPLE, U32) {}
inline void AIL_sample_ms_position(HSAMPLE, S32 *, S32 *) {}
inline void AIL_set_sample_user_data(HSAMPLE, S32, uintptr_t) {}
inline uintptr_t AIL_sample_user_data(HSAMPLE, S32) { return 0; }
inline S32 AIL_sample_playback_rate(HSAMPLE) { return 0; }
inline void AIL_set_sample_playback_rate(HSAMPLE, S32) {}
inline void AIL_set_sample_processor(HSAMPLE, U32, HPROVIDER) {}
inline void AIL_set_filter_sample_preference(HSAMPLE, char const *, void const *) {}
inline H3DSAMPLE AIL_allocate_3D_sample_handle(HPROVIDER) { return nullptr; }
inline void AIL_release_3D_sample_handle(H3DSAMPLE) {}
inline U32 AIL_set_3D_sample_file(H3DSAMPLE, void const *, U32) { return 0; }
inline void AIL_start_3D_sample(H3DSAMPLE) {}
inline void AIL_stop_3D_sample(H3DSAMPLE) {}
inline void AIL_resume_3D_sample(H3DSAMPLE) {}
inline void AIL_end_3D_sample(H3DSAMPLE) {}
inline void AIL_set_3D_sample_volume(H3DSAMPLE, S32) {}
inline S32 AIL_3D_sample_volume(H3DSAMPLE) { return 0; }
inline void AIL_set_3D_sample_loop_count(H3DSAMPLE, U32) {}
inline U32 AIL_3D_sample_loop_count(H3DSAMPLE) { return 0; }
inline void AIL_set_3D_sample_offset(H3DSAMPLE, U32) {}
inline U32 AIL_3D_sample_offset(H3DSAMPLE) { return 0; }
inline U32 AIL_3D_sample_length(H3DSAMPLE) { return 0; }
inline void AIL_set_3D_object_user_data(H3DSAMPLE, S32, uintptr_t) {}
inline uintptr_t AIL_3D_object_user_data(H3DSAMPLE, S32) { return 0; }
inline S32 AIL_3D_sample_playback_rate(H3DSAMPLE) { return 0; }
inline void AIL_set_3D_sample_playback_rate(H3DSAMPLE, S32) {}
inline void AIL_set_3D_position(H3DSAMPLE, float, float, float) {}
inline void AIL_set_3D_orientation(H3DSAMPLE, float, float, float, float, float, float) {}
inline void AIL_set_3D_velocity_vector(H3DSAMPLE, float, float, float, float) {}
inline void AIL_set_3D_velocity_vector(H3DSAMPLE, float, float, float) {}
inline void AIL_set_3D_sample_distances(H3DSAMPLE, float, float, int) {}
inline void AIL_set_3D_sample_distances(H3DSAMPLE, float, float) {}
inline void AIL_set_3D_sample_effects_level(H3DSAMPLE, float) {}
inline HSTREAM AIL_open_stream_by_sample(HDIGDRIVER, HSAMPLE, char const *, U32) { return nullptr; }
inline void AIL_start_stream(HSTREAM) {}
inline void AIL_pause_stream(HSTREAM, S32) {}
inline void AIL_close_stream(HSTREAM) {}
inline void AIL_set_stream_pan(HSTREAM, S32) {}
inline S32 AIL_stream_pan(HSTREAM) { return 64; }
inline void AIL_set_stream_volume(HSTREAM, S32) {}
inline S32 AIL_stream_volume(HSTREAM) { return 0; }
inline void AIL_set_stream_loop_block(HSTREAM, S32, S32) {}
inline void AIL_set_stream_loop_count(HSTREAM, U32) {}
inline U32 AIL_stream_loop_count(HSTREAM) { return 0; }
inline void AIL_set_stream_ms_position(HSTREAM, U32) {}
inline void AIL_stream_ms_position(HSTREAM, S32 *, S32 *) {}
inline S32 AIL_stream_playback_rate(HSTREAM) { return 0; }
inline void AIL_set_stream_playback_rate(HSTREAM, S32) {}
inline H3DPOBJECT AIL_3D_open_listener(HPROVIDER) { return nullptr; }
inline S32 AIL_enumerate_3D_providers(HPROENUM *, HPROVIDER *, char **) { return 0; }
inline S32 AIL_open_3D_provider(HPROVIDER) { return M3D_NOERR; }
inline void AIL_close_3D_provider(HPROVIDER) {}
inline void AIL_set_3D_speaker_type(HPROVIDER, S32) {}
inline S32 AIL_enumerate_filters(HPROENUM *, HPROVIDER *, char **) { return 0; }
inline char *AIL_last_error(void) { return const_cast<char *>("null backend"); }
inline void AIL_set_file_callbacks(AIL_FILE_OPEN_CALLBACK, AIL_FILE_CLOSE_CALLBACK, AIL_FILE_SEEK_CALLBACK, AIL_FILE_READ_CALLBACK) {}
inline HTIMER AIL_allocate_timer_handle(void) { return -1; }
inline void AIL_start_timer(HTIMER) {}
inline void AIL_stop_timer(HTIMER) {}
inline void AIL_release_timer_handle(HTIMER) {}
inline U32 AIL_WAV_info(void const *, AILSOUNDINFO *) { return 0; }
#endif
