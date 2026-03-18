cmake_minimum_required(VERSION 3.25)

project(Renegade VERSION 0.1.0 LANGUAGES C CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(RenegadeOptions)
include(RenegadeCompilerSettings)
include(RenegadeTargets)

set(RENEGADE_SDL3_VERSION "release-3.4.2")
set(RENEGADE_SDL3_SUBMODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/external/SDL")
set(RENEGADE_BOOTSTRAP_PLATFORM "Linux x64")

if(NOT EXISTS "${RENEGADE_SDL3_SUBMODULE_PATH}/CMakeLists.txt")
    message(FATAL_ERROR
        "SDL3 submodule was not found at ${RENEGADE_SDL3_SUBMODULE_PATH}. "
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
        message(STATUS "Renegade bootstrap target: Linux x64")
    else()
        message(WARNING "Initial bring-up is validated on Linux x64. Current processor: ${CMAKE_SYSTEM_PROCESSOR}")
    endif()
else()
    message(STATUS "Configuring for ${CMAKE_SYSTEM_NAME}. The first validated bootstrap target remains Linux x64.")
endif()

set(SDL_INSTALL OFF CACHE BOOL "" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
set(SDL_TESTS OFF CACHE BOOL "" FORCE)
set(SDL_STATIC OFF CACHE BOOL "" FORCE)
set(SDL_SHARED ON CACHE BOOL "" FORCE)

add_subdirectory("${RENEGADE_SDL3_SUBMODULE_PATH}" external/SDL EXCLUDE_FROM_ALL)
add_subdirectory(Code)
