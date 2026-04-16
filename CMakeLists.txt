cmake_minimum_required(VERSION 3.25)

project(Renegade VERSION 0.1.0 LANGUAGES C CXX)

include(FetchContent)

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
set(SDL_STATIC OFF CACHE BOOL "" FORCE)
set(SDL_SHARED ON CACHE BOOL "" FORCE)

add_subdirectory("${RENEGADE_SDL3_SUBMODULE_PATH}" external/SDL EXCLUDE_FROM_ALL)

if(RENEGADE_AUDIO_BACKEND STREQUAL "SDL_MIXER")
    set(SDLMIXER_SAMPLES OFF CACHE BOOL "" FORCE)
    set(SDLMIXER_TESTS OFF CACHE BOOL "" FORCE)
    set(SDLMIXER_INSTALL OFF CACHE BOOL "" FORCE)
    set(SDLMIXER_VENDORED OFF CACHE BOOL "" FORCE)
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
FetchContent_MakeAvailable(bgfx_cmake)

set(RENEGADE_BGFX_TOOL_UTILS "${bgfx_cmake_SOURCE_DIR}/cmake/bgfxToolUtils.cmake")
file(READ "${RENEGADE_BGFX_TOOL_UTILS}" _renegade_bgfx_tool_utils)
string(REPLACE "$<IF:$<CONFIG:Debug>:0,3>" "$<IF:$<CONFIG:Debug>,0,3>" _renegade_bgfx_tool_utils_patched "${_renegade_bgfx_tool_utils}")
if(NOT _renegade_bgfx_tool_utils STREQUAL _renegade_bgfx_tool_utils_patched)
    file(WRITE "${RENEGADE_BGFX_TOOL_UTILS}" "${_renegade_bgfx_tool_utils_patched}")
endif()

if(TARGET glsl-optimizer)
    target_compile_definitions(glsl-optimizer PRIVATE _GNU_SOURCE)
endif()
if(TARGET fcpp)
    target_compile_definitions(fcpp PRIVATE _GNU_SOURCE)
endif()

include("${bgfx_cmake_SOURCE_DIR}/cmake/bgfxToolUtils.cmake")

add_subdirectory(Code)
