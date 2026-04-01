#pragma once

#if defined(__has_include)
#  if __has_include("renegade_build_config.generated.h")
#    include "renegade_build_config.generated.h"
#  endif
#endif

#ifndef RENEGADE_BOOTSTRAP_PLATFORM
#  define RENEGADE_BOOTSTRAP_PLATFORM "unconfigured"
#endif

#ifndef RENEGADE_SDL3_VERSION
#  define RENEGADE_SDL3_VERSION "unknown"
#endif

#ifndef RENEGADE_WITH_SDL3
#  define RENEGADE_WITH_SDL3 0
#endif

#ifndef RENEGADE_WITH_X86_ASM
#  define RENEGADE_WITH_X86_ASM 0
#endif

#ifndef RENEGADE_WITH_UMBRA
#  define RENEGADE_WITH_UMBRA 0
#endif

#ifndef RENEGADE_WITH_BINK
#  define RENEGADE_WITH_BINK 0
#endif

#ifndef RENEGADE_WITH_MILES
#  define RENEGADE_WITH_MILES 0
#endif

#ifndef RENEGADE_WITH_GAMESPY
#  define RENEGADE_WITH_GAMESPY 0
#endif

#ifndef RENEGADE_WITH_LEGACY_WOL
#  define RENEGADE_WITH_LEGACY_WOL 0
#endif

#ifndef RENEGADE_WITH_DX8_RENDERER
#  define RENEGADE_WITH_DX8_RENDERER 0
#endif

#ifndef RENEGADE_WITH_BGFX_RENDERER
#  define RENEGADE_WITH_BGFX_RENDERER 0
#endif

#ifndef RENEGADE_WITH_SCRIPT_DLL
#  define RENEGADE_WITH_SCRIPT_DLL 0
#endif

#ifndef RENEGADE_WITH_BANDTEST
#  define RENEGADE_WITH_BANDTEST 0
#endif

#ifndef RENEGADE_WITH_SCONTROL
#  define RENEGADE_WITH_SCONTROL 0
#endif
