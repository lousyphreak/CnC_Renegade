cmake_minimum_required(VERSION 3.25)

project(Renegade VERSION 0.1.0 LANGUAGES C CXX)

# temporary disable all warnings until the code is cleaned up
add_definitions(-w)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(RenegadeOptions)
include(RenegadeCompilerSettings)
include(RenegadeTargets)

set(RENEGADE_SDL3_VERSION "release-3.4.2")
set(RENEGADE_SDL3_SUBMODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/external/SDL")
set(RENEGADE_SDL3_MIXER_SUBMODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/external/SDL_mixer")
set(RENEGADE_BGFX_CMAKE_SUBMODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/external/bgfx.cmake")
set(RENEGADE_BOOTSTRAP_PLATFORM "Linux x64")

if(NOT EXISTS "${RENEGADE_SDL3_SUBMODULE_PATH}/CMakeLists.txt")
    message(FATAL_ERROR
        "SDL3 submodule was not found at ${RENEGADE_SDL3_SUBMODULE_PATH}. "
        "Run 'git submodule update --init --recursive' before configuring.")
endif()

if(NOT RENEGADE_AUDIO_BACKEND STREQUAL "SDL_MIXER" AND NOT RENEGADE_AUDIO_BACKEND STREQUAL "NULL")
    message(FATAL_ERROR "Unsupported RENEGADE_AUDIO_BACKEND='${RENEGADE_AUDIO_BACKEND}'. Use SDL_MIXER or NULL.")
endif()

if(RENEGADE_AUDIO_BACKEND STREQUAL "SDL_MIXER" AND NOT EXISTS "${RENEGADE_SDL3_MIXER_SUBMODULE_PATH}/CMakeLists.txt")
    message(FATAL_ERROR
        "SDL_mixer submodule was not found at ${RENEGADE_SDL3_MIXER_SUBMODULE_PATH}. "
        "Run 'git submodule update --init --recursive' before configuring.")
endif()

if(NOT EXISTS "${RENEGADE_BGFX_CMAKE_SUBMODULE_PATH}/CMakeLists.txt"
    OR NOT EXISTS "${RENEGADE_BGFX_CMAKE_SUBMODULE_PATH}/bgfx/src/bgfx.cpp"
    OR NOT EXISTS "${RENEGADE_BGFX_CMAKE_SUBMODULE_PATH}/bimg/src/image.cpp"
    OR NOT EXISTS "${RENEGADE_BGFX_CMAKE_SUBMODULE_PATH}/bx/include/bx/bx.h")
    message(FATAL_ERROR
        "bgfx.cmake submodule was not found at ${RENEGADE_BGFX_CMAKE_SUBMODULE_PATH}, "
        "or its nested submodules are missing. "
        "Run 'git submodule update --init --recursive' before configuring.")
endif()

file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/generated")
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/Code/renegade_build_config.h.in"
    "${CMAKE_CURRENT_BINARY_DIR}/generated/renegade_build_config.generated.h"
    @ONLY
)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86_64|amd64)$")
        message(STATUS "Renegade runtime target: Linux x64")
    else()
        message(WARNING "Initial bring-up is validated on Linux x64. Current processor: ${CMAKE_SYSTEM_PROCESSOR}")
    endif()
else()
    message(STATUS "Configuring for ${CMAKE_SYSTEM_NAME}. The first validated runtime target remains Linux x64.")
endif()

set(SDL_INSTALL OFF CACHE BOOL "" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
set(SDL_TESTS OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_DEPS_SHARED OFF CACHE BOOL "" FORCE)

foreach(_renegade_sdl_dep_option IN ITEMS
    SDL_ALSA_SHARED
    SDL_JACK_SHARED
    SDL_PIPEWIRE_SHARED
    SDL_PULSEAUDIO_SHARED
    SDL_SNDIO_SHARED
    SDL_X11_SHARED
    SDL_FRIBIDI_SHARED
    SDL_LIBTHAI_SHARED
    SDL_WAYLAND_SHARED
    SDL_WAYLAND_LIBDECOR_SHARED
    SDL_KMSDRM_SHARED
    SDL_HIDAPI_LIBUSB_SHARED
)
    set(${_renegade_sdl_dep_option} OFF CACHE BOOL "" FORCE)
endforeach()

if(EMSCRIPTEN)
    set(SDL_PTHREADS OFF CACHE BOOL "" FORCE)
    set(SDL_PTHREADS_SEM OFF CACHE BOOL "" FORCE)
endif()

add_subdirectory("${RENEGADE_SDL3_SUBMODULE_PATH}" external/SDL EXCLUDE_FROM_ALL)
target_link_libraries(renegade_project_options INTERFACE SDL3::SDL3)

if(RENEGADE_AUDIO_BACKEND STREQUAL "SDL_MIXER")
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    set(SDLMIXER_SAMPLES OFF CACHE BOOL "" FORCE)
    set(SDLMIXER_TESTS OFF CACHE BOOL "" FORCE)
    set(SDLMIXER_INSTALL OFF CACHE BOOL "" FORCE)
    set(SDLMIXER_VENDORED OFF CACHE BOOL "" FORCE)
    set(SDLMIXER_DEPS_SHARED OFF CACHE BOOL "" FORCE)
    foreach(_renegade_sdlmixer_dep_option IN ITEMS
        SDLMIXER_FLAC_LIBFLAC_SHARED
        SDLMIXER_GME_SHARED
        SDLMIXER_MOD_XMP_SHARED
        SDLMIXER_MP3_MPG123_SHARED
        SDLMIXER_MIDI_FLUIDSYNTH_SHARED
        SDLMIXER_OPUS_SHARED
        SDLMIXER_VORBIS_VORBISFILE_SHARED
        SDLMIXER_VORBIS_TREMOR_SHARED
        SDLMIXER_WAVPACK_SHARED
    )
        set(${_renegade_sdlmixer_dep_option} OFF CACHE BOOL "" FORCE)
    endforeach()
    add_subdirectory("${RENEGADE_SDL3_MIXER_SUBMODULE_PATH}" external/SDL_mixer EXCLUDE_FROM_ALL)
endif()

set(BGFX_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BGFX_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(BGFX_INSTALL OFF CACHE BOOL "" FORCE)
set(BGFX_CUSTOM_TARGETS OFF CACHE BOOL "" FORCE)
set(BGFX_BUILD_TOOLS ON CACHE BOOL "" FORCE)
set(BGFX_BUILD_TOOLS_SHADER ON CACHE BOOL "" FORCE)
set(BGFX_BUILD_TOOLS_BIN2C OFF CACHE BOOL "" FORCE)
set(BGFX_BUILD_TOOLS_GEOMETRY OFF CACHE BOOL "" FORCE)
set(BGFX_BUILD_TOOLS_TEXTURE OFF CACHE BOOL "" FORCE)
set(BGFX_CONFIG_MAX_TEXTURES 65535)
set(BGFX_CONFIG_MAX_VERTEX_BUFFERS 65535)
set(BGFX_CONFIG_MAX_INDEX_BUFFERS 65535)

get_filename_component(RENEGADE_BGFX_CMAKE_SOURCE_ROOT "${RENEGADE_BGFX_CMAKE_SUBMODULE_PATH}" REALPATH)
set(bgfx_cmake_SOURCE_DIR "${RENEGADE_BGFX_CMAKE_SOURCE_ROOT}")
set(bgfx_cmake_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/bgfx.cmake-build")
set(BX_DIR "${RENEGADE_BGFX_CMAKE_SOURCE_ROOT}/bx" CACHE STRING "Location of bx." FORCE)
set(BIMG_DIR "${RENEGADE_BGFX_CMAKE_SOURCE_ROOT}/bimg" CACHE STRING "Location of bimg." FORCE)
set(BGFX_DIR "${RENEGADE_BGFX_CMAKE_SOURCE_ROOT}/bgfx" CACHE STRING "Location of bgfx." FORCE)

add_subdirectory("${bgfx_cmake_SOURCE_DIR}" "${bgfx_cmake_BINARY_DIR}")

if(NOT BGFX_BUILD_TOOLS_TEXTURE AND TARGET bimg_encode)
    set_target_properties(bimg_encode PROPERTIES
        EXCLUDE_FROM_ALL TRUE
        EXCLUDE_FROM_DEFAULT_BUILD TRUE
    )
endif()

if(TARGET glsl-optimizer)
    target_compile_definitions(glsl-optimizer PRIVATE _GNU_SOURCE)
endif()
if(TARGET fcpp)
    target_compile_definitions(fcpp PRIVATE _GNU_SOURCE)
endif()
if(EMSCRIPTEN AND TARGET shaderc)
    target_link_options(shaderc PRIVATE
        "SHELL:-sNODERAWFS=1"
        "SHELL:-sSTACK_SIZE=1048576"
    )
endif()

include(RenegadeBgfxShaders)

add_subdirectory(Code)
