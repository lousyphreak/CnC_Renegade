cmake_minimum_required(VERSION 3.25)

project(Renegade VERSION 0.1.0 LANGUAGES C CXX)

include(FetchContent)

if(POLICY CMP0169)
    cmake_policy(SET CMP0169 OLD)
endif()

# temporary disable all warnings until the code is cleaned up
add_definitions(-w)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(RenegadeOptions)
include(RenegadeCompilerSettings)
include(RenegadeTargets)

set(RENEGADE_SDL3_VERSION "release-3.4.2")
set(RENEGADE_SDL3_SUBMODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/external/SDL")
set(RENEGADE_SDL3_MIXER_SUBMODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/external/SDL_mixer")
set(RENEGADE_BOOTSTRAP_PLATFORM "Linux x64")
set(RENEGADE_BGFX_CMAKE_TAG "v1.142.9197-527" CACHE STRING "Pinned bgfx.cmake release tag")

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

FetchContent_Declare(
    bgfx_cmake
    GIT_REPOSITORY https://github.com/bkaradzic/bgfx.cmake.git
    GIT_TAG ${RENEGADE_BGFX_CMAKE_TAG}
    GIT_SHALLOW TRUE
    GIT_PROGRESS TRUE
    GIT_SUBMODULES_RECURSE TRUE
)
FetchContent_GetProperties(bgfx_cmake)
if(NOT bgfx_cmake_POPULATED)
    FetchContent_Populate(bgfx_cmake)
endif()

set(RENEGADE_BGFX_TOOL_UTILS "${bgfx_cmake_SOURCE_DIR}/cmake/bgfxToolUtils.cmake")
file(READ "${RENEGADE_BGFX_TOOL_UTILS}" _renegade_bgfx_tool_utils)
string(REPLACE "$<IF:$<CONFIG:Debug>:0,3>" "$<IF:$<CONFIG:Debug>,0,3>" _renegade_bgfx_tool_utils_patched "${_renegade_bgfx_tool_utils}")
string(REPLACE
    "set(multiValueArgs SHADERS INCLUDE_DIRS DEFINES)"
    "set(multiValueArgs SHADERS INCLUDE_DIRS DEFINES DEPENDS)"
    _renegade_bgfx_tool_utils_patched
    "${_renegade_bgfx_tool_utils_patched}"
)
string(REPLACE
    "DEPENDS \${ARGS_VARYING_DEF}"
    "DEPENDS \${ARGS_VARYING_DEF} \${ARGS_DEPENDS}"
    _renegade_bgfx_tool_utils_patched
    "${_renegade_bgfx_tool_utils_patched}"
)
if(NOT _renegade_bgfx_tool_utils STREQUAL _renegade_bgfx_tool_utils_patched)
    file(WRITE "${RENEGADE_BGFX_TOOL_UTILS}" "${_renegade_bgfx_tool_utils_patched}")
endif()

set(RENEGADE_BGFX_BIMG_CMAKE "${bgfx_cmake_SOURCE_DIR}/cmake/bimg/CMakeLists.txt")
file(READ "${RENEGADE_BGFX_BIMG_CMAKE}" _renegade_bgfx_bimg_cmake)
string(REPLACE
    "include(bimg_decode.cmake)\ninclude(bimg_encode.cmake)\n\nif(BGFX_BUILD_TOOLS_TEXTURE)\n\tinclude(texturec.cmake)\nendif()"
    "include(bimg_decode.cmake)\n\nif(BGFX_BUILD_TOOLS_TEXTURE)\n\tinclude(bimg_encode.cmake)\n\tinclude(texturec.cmake)\nendif()"
    _renegade_bgfx_bimg_cmake_patched
    "${_renegade_bgfx_bimg_cmake}"
)
if(NOT _renegade_bgfx_bimg_cmake STREQUAL _renegade_bgfx_bimg_cmake_patched)
    file(WRITE "${RENEGADE_BGFX_BIMG_CMAKE}" "${_renegade_bgfx_bimg_cmake_patched}")
endif()

add_subdirectory("${bgfx_cmake_SOURCE_DIR}" "${bgfx_cmake_BINARY_DIR}")

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

include("${bgfx_cmake_SOURCE_DIR}/cmake/bgfxToolUtils.cmake")

add_subdirectory(Code)
