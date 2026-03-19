#include "WWAudio.h"

#include "AudibleSound.h"
#include "LogicalListener.h"
#include "LogicalSound.h"
#include "refcount.h"

WWAudioClass *WWAudioClass::_theInstance = NULL;
HANDLE WWAudioClass::_TimerSyncEvent = NULL;

WWAudioClass::WWAudioClass(bool)
    : m_PlaybackRate(44100),
      m_PlaybackBits(16),
      m_PlaybackStereo(true),
      m_MusicVolume(DEF_MUSIC_VOL),
      m_SoundVolume(DEF_SFX_VOL),
      m_RealMusicVolume(DEF_MUSIC_VOL),
      m_RealSoundVolume(DEF_SFX_VOL),
      m_DialogVolume(DEF_DIALOG_VOL),
      m_CinematicVolume(DEF_CINEMATIC_VOL),
      m_Max2DSamples(DEF_2D_SAMPLE_COUNT),
      m_Max3DSamples(DEF_3D_SAMPLE_COUNT),
      m_Max2DBufferSize(DEF_MAX_2D_BUFFER_SIZE),
      m_Max3DBufferSize(DEF_MAX_3D_BUFFER_SIZE),
      m_UpdateTimer(-1),
      m_IsMusicEnabled(true),
      m_IsDialogEnabled(true),
      m_IsCinematicSoundEnabled(true),
      m_AreSoundEffectsEnabled(true),
      m_AreNewSoundsEnabled(true),
      m_FileFactory(NULL),
      m_BackgroundMusic(NULL),
      m_CachedIsMusicEnabled(true),
      m_CachedIsDialogEnabled(true),
      m_CachedIsCinematicSoundEnabled(true),
      m_CachedAreSoundEffectsEnabled(true),
      m_SoundScene(NULL),
      m_CurrPage(PAGE_PRIMARY),
      m_Driver2D(NULL),
      m_Driver3D(NULL),
      m_Driver3DPseudo(NULL),
      m_ReverbFilter(NULL),
      m_SpeakerType(0),
      m_MaxCacheSize(DEF_CACHE_SIZE * 1024),
      m_CurrentCacheSize(0),
      m_EffectsLevel(0.0f),
      m_ReverbRoomType(ENVIRONMENT_GENERIC),
      m_NonDialogFadeTime(DEF_FADE_TIME),
      m_FadeType(FADE_NONE),
      m_FadeTimer(0.0f),
      AudioIni(NULL),
      m_ForceDisable(true)
{
    _theInstance = this;
}

WWAudioClass::~WWAudioClass(void)
{
    if (_theInstance == this) {
        _theInstance = NULL;
    }
}

void WWAudioClass::Initialize(bool stereo, int bits, int hertz)
{
    m_PlaybackStereo = stereo;
    m_PlaybackBits = bits;
    m_PlaybackRate = hertz;
}

void WWAudioClass::Initialize(const char *)
{
}

void WWAudioClass::Shutdown(void)
{
}

WWAudioClass::DRIVER_TYPE_2D WWAudioClass::Open_2D_Device(LPWAVEFORMAT format)
{
    if (format != NULL) {
        m_PlaybackRate = format->nSamplesPerSec;
        m_PlaybackStereo = (format->nChannels > 1);
    }
    return DRIVER2D_ERROR;
}

WWAudioClass::DRIVER_TYPE_2D WWAudioClass::Open_2D_Device(bool stereo, int bits, int hertz)
{
    m_PlaybackStereo = stereo;
    m_PlaybackBits = bits;
    m_PlaybackRate = hertz;
    return DRIVER2D_ERROR;
}

bool WWAudioClass::Close_2D_Device(void)
{
    return true;
}

int WWAudioClass::Find_3D_Device(DRIVER_TYPE_3D)
{
    return -1;
}

bool WWAudioClass::Select_3D_Device(int)
{
    return false;
}

bool WWAudioClass::Select_3D_Device(const char *, HPROVIDER)
{
    return false;
}

bool WWAudioClass::Select_3D_Device(DRIVER_TYPE_3D)
{
    return false;
}

bool WWAudioClass::Select_3D_Device(const char *)
{
    return false;
}

bool WWAudioClass::Close_3D_Device(void)
{
    return true;
}

void WWAudioClass::Set_Speaker_Type(int speaker_type)
{
    m_SpeakerType = speaker_type;
}

int WWAudioClass::Get_Speaker_Type(void) const
{
    return m_SpeakerType;
}

bool WWAudioClass::Load_From_Registry(const char *)
{
    return false;
}

bool WWAudioClass::Load_From_Registry(const char *, StringClass &, bool &, int &, int &, bool &, bool &, bool &, bool &, float &, float &, float &, float &, int &)
{
    return false;
}

bool WWAudioClass::Save_To_Registry(const char *)
{
    return true;
}

bool WWAudioClass::Save_To_Registry(const char *, const StringClass &, bool, int, int, bool, bool, bool, bool, float, float, float, float, int)
{
    return true;
}

void WWAudioClass::Load_Default_Volume(int &defaultmusicvolume, int &defaultsoundvolume, int &defaultdialogvolume, int &defaultcinematicvolume)
{
    defaultmusicvolume = 100;
    defaultsoundvolume = 100;
    defaultdialogvolume = 100;
    defaultcinematicvolume = 100;
}

bool WWAudioClass::Set_Max_2D_Sample_Count(int count)
{
    m_Max2DSamples = count;
    return true;
}

int WWAudioClass::Get_Max_2D_Sample_Count(void) const
{
    return m_Max2DSamples;
}

int WWAudioClass::Get_Avail_2D_Sample_Count(void) const
{
    return 0;
}

bool WWAudioClass::Set_Max_3D_Sample_Count(int count)
{
    m_Max3DSamples = count;
    return true;
}

int WWAudioClass::Get_Max_3D_Sample_Count(void) const
{
    return m_Max3DSamples;
}

int WWAudioClass::Get_Avail_3D_Sample_Count(void) const
{
    return 0;
}

void WWAudioClass::Set_Reverb_Room_Type(int type)
{
    m_ReverbRoomType = type;
}

void WWAudioClass::Set_Sound_Effects_Volume(float volume)
{
    m_SoundVolume = volume;
    m_RealSoundVolume = volume;
}

void WWAudioClass::Set_Music_Volume(float volume)
{
    m_MusicVolume = volume;
    m_RealMusicVolume = volume;
}

void WWAudioClass::Set_Dialog_Volume(float volume)
{
    m_DialogVolume = volume;
}

void WWAudioClass::Set_Cinematic_Volume(float volume)
{
    m_CinematicVolume = volume;
}

void WWAudioClass::Allow_Sound_Effects(bool onoff)
{
    m_AreSoundEffectsEnabled = onoff;
}

void WWAudioClass::Allow_Music(bool onoff)
{
    m_IsMusicEnabled = onoff;
}

void WWAudioClass::Allow_Dialog(bool onoff)
{
    m_IsDialogEnabled = onoff;
}

void WWAudioClass::Allow_Cinematic_Sound(bool onoff)
{
    m_IsCinematicSoundEnabled = onoff;
}

void WWAudioClass::Temp_Disable_Audio(bool onoff)
{
    m_ForceDisable = onoff;
}

void WWAudioClass::On_Frame_Update(unsigned int)
{
}

void WWAudioClass::Register_EOS_Callback(LPFNEOSCALLBACK, DWORD)
{
}

void WWAudioClass::UnRegister_EOS_Callback(LPFNEOSCALLBACK)
{
}

void WWAudioClass::Register_Text_Callback(LPFNTEXTCALLBACK, DWORD)
{
}

void WWAudioClass::UnRegister_Text_Callback(LPFNTEXTCALLBACK)
{
}

void WWAudioClass::Fire_Text_Callback(AudibleSoundClass *, const StringClass &)
{
}

bool WWAudioClass::Is_Sound_Cached(const char *)
{
    return false;
}

AudibleSoundClass * WWAudioClass::Create_Sound_Effect(FileClass &, const char *)
{
    return NULL;
}

AudibleSoundClass * WWAudioClass::Create_Sound_Effect(const char *)
{
    return NULL;
}

AudibleSoundClass * WWAudioClass::Create_Sound_Effect(const char *, unsigned char *, unsigned long)
{
    return NULL;
}

Sound3DClass * WWAudioClass::Create_3D_Sound(FileClass &, const char *, int)
{
    return NULL;
}

Sound3DClass * WWAudioClass::Create_3D_Sound(const char *, int)
{
    return NULL;
}

Sound3DClass * WWAudioClass::Create_3D_Sound(const char *, unsigned char *, unsigned long, int)
{
    return NULL;
}

void WWAudioClass::Set_Background_Music(const char *filename)
{
    m_BackgroundMusicName = (filename != NULL) ? filename : "";
}

void WWAudioClass::Fade_Background_Music(const char *filename, int, int)
{
    Set_Background_Music(filename);
}

LogicalSoundClass * WWAudioClass::Create_Logical_Sound(void)
{
    return NULL;
}

LogicalListenerClass * WWAudioClass::Create_Logical_Listener(void)
{
    return NULL;
}

void WWAudioClass::Add_Logical_Type(int id, LPCTSTR display_name)
{
    m_LogicalTypes.Add(LOGICAL_TYPE_STRUCT(id, display_name));
}

void WWAudioClass::Reset_Logical_Types(void)
{
    m_LogicalTypes.Delete_All();
}

int WWAudioClass::Get_Logical_Type(int index, StringClass &name)
{
    if (index < 0 || index >= m_LogicalTypes.Count()) {
        return -1;
    }

    name = m_LogicalTypes[index].display_name;
    return m_LogicalTypes[index].id;
}

int WWAudioClass::Create_Instant_Sound(int, const Matrix3D &, RefCountClass *, uint32, int)
{
    return 0;
}

int WWAudioClass::Create_Instant_Sound(const char *, const Matrix3D &, RefCountClass *, uint32, int)
{
    return 0;
}

AudibleSoundClass * WWAudioClass::Create_Continuous_Sound(int, RefCountClass *, uint32, int)
{
    return NULL;
}

AudibleSoundClass * WWAudioClass::Create_Continuous_Sound(const char *, RefCountClass *, uint32, int)
{
    return NULL;
}

AudibleSoundClass * WWAudioClass::Create_Sound(int, RefCountClass *, uint32, int)
{
    return NULL;
}

AudibleSoundClass * WWAudioClass::Create_Sound(const char *, RefCountClass *, uint32, int)
{
    return NULL;
}

SoundSceneObjClass * WWAudioClass::Find_Sound_Object(uint32)
{
    return NULL;
}

void WWAudioClass::Flush_Cache(void)
{
    m_CurrentCacheSize = 0;
}

bool WWAudioClass::Simple_Play_2D_Sound_Effect(const char *, float, float)
{
    return false;
}

bool WWAudioClass::Simple_Play_2D_Sound_Effect(FileClass &, float, float)
{
    return false;
}

bool WWAudioClass::Add_To_Playlist(AudibleSoundClass *)
{
    return false;
}

bool WWAudioClass::Remove_From_Playlist(AudibleSoundClass *)
{
    return false;
}

AudibleSoundClass * WWAudioClass::Get_Playlist_Entry(int index) const
{
    return (index >= 0 && index < m_Playlist[m_CurrPage].Count()) ? m_Playlist[m_CurrPage][index] : NULL;
}

void WWAudioClass::Flush_Playlist(void)
{
    Flush_Playlist(m_CurrPage);
}

void WWAudioClass::Flush_Playlist(SOUND_PAGE page)
{
    m_Playlist[page].Delete_All();
}

bool WWAudioClass::Is_Sound_In_Playlist(AudibleSoundClass *)
{
    return false;
}

bool WWAudioClass::Acquire_Virtual_Channel(AudibleSoundClass *, int)
{
    return false;
}

void WWAudioClass::Release_Virtual_Channel(AudibleSoundClass *, int)
{
}

void WWAudioClass::Set_Active_Sound_Page(SOUND_PAGE page)
{
    m_CurrPage = page;
}

void WWAudioClass::Push_Active_Sound_Page(SOUND_PAGE page)
{
    m_PageStack.Add(m_CurrPage);
    m_CurrPage = page;
}

void WWAudioClass::Pop_Active_Sound_Page(void)
{
    if (m_PageStack.Count() > 0) {
        m_CurrPage = m_PageStack[m_PageStack.Count() - 1];
        m_PageStack.Delete(m_PageStack.Count() - 1);
    }
}

void WWAudioClass::Fade_Non_Dialog_In(void)
{
}

void WWAudioClass::Fade_Non_Dialog_Out(void)
{
}

float WWAudioClass::Get_Digital_CPU_Percent(void) const
{
    return 0.0f;
}

bool WWAudioClass::Is_Disabled(void) const
{
    return true;
}

AudibleSoundClass * WWAudioClass::Peek_2D_Sample(int)
{
    return NULL;
}

AudibleSoundClass * WWAudioClass::Peek_3D_Sample(int)
{
    return NULL;
}

void WWAudioClass::Free_Completed_Sounds(void)
{
    m_CompletedSounds.Delete_All();
}
