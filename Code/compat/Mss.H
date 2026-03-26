#pragma once

#include "windows.h"
#include "bittype.h"

#ifdef _WIN32
#include <mmreg.h>
#endif

using U32 = uint32;
using S32 = sint32;

using HPROVIDER = void *;

struct RENEGADE_MILES_DIG_DRIVER {
    BOOL emulated_ds;
};

using HDIGDRIVER = RENEGADE_MILES_DIG_DRIVER *;
using HSAMPLE = void *;
using H3DSAMPLE = void *;
using HSTREAM = void *;
using H3DPOBJECT = void *;
using HTIMER = sint32;

#ifndef _WIN32
struct WAVEFORMAT {
    WORD wFormatTag;
    WORD nChannels;
    DWORD nSamplesPerSec;
    DWORD nAvgBytesPerSec;
    WORD nBlockAlign;
};

struct PCMWAVEFORMAT {
    WAVEFORMAT wf;
    WORD wBitsPerSample;
};

#endif

using LPWAVEFORMAT = WAVEFORMAT *;
using LPCWAVEFORMAT = const WAVEFORMAT *;

struct AILSOUNDINFO {
    U32 format;
    const void * data_ptr;
    U32 data_len;
    S32 rate;
    S32 bits;
    S32 channels;
    S32 samples;
    U32 block_size;
    const void * initial_ptr;
};

#ifndef AILCALLBACK
#define AILCALLBACK __stdcall
#endif

#ifndef WAVE_FORMAT_PCM
#define WAVE_FORMAT_PCM 1
#endif

#ifndef AIL_NO_ERROR
#define AIL_NO_ERROR 0
#endif

#ifndef AIL_LOCK_PROTECTION
#define AIL_LOCK_PROTECTION 0
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

inline void AIL_lock()
{
}

inline void AIL_unlock()
{
}

inline void AIL_set_3D_position(H3DSAMPLE, float, float, float)
{
}

inline void AIL_set_3D_orientation(H3DSAMPLE, float, float, float, float, float, float)
{
}

inline void AIL_set_3D_velocity_vector(H3DSAMPLE, float, float, float, float)
{
}

inline void AIL_set_3D_velocity_vector(H3DSAMPLE, float, float, float)
{
}

inline void AIL_set_3D_sample_distances(H3DSAMPLE, float, float, int)
{
}

inline void AIL_set_3D_sample_distances(H3DSAMPLE, float, float)
{
}

inline void AIL_set_3D_sample_effects_level(H3DSAMPLE, float)
{
}
