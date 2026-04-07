file(GLOB WWAUDIO_HEADERS CONFIGURE_DEPENDS "*.h")

set(WWAUDIO_COMMON_SOURCES
    AudioEvents.cpp
    SoundSceneObj.cpp
    Threads_compat.cpp
    ${WWAUDIO_HEADERS}
)

set(WWAUDIO_NULL_SOURCES
    AudibleSound.cpp
    AudioSaveLoad_null.cpp
    Listener.cpp
    LogicalListener.cpp
    Sound3D.cpp
    wwaudio_null.cpp
)

set(WWAUDIO_SDLMIXER_SOURCES
    AudioSaveLoad.cpp
    AudibleSound.cpp
    FilteredSound.cpp
    Listener.cpp
    listenerhandle.cpp
    LogicalListener.cpp
    LogicalSound.cpp
    Sound3D.cpp
    SoundBuffer.cpp
    SoundPseudo3D.cpp
    SoundScene.cpp
    Utils.cpp
    sound2dhandle.cpp
    sound3dhandle.cpp
    soundhandle.cpp
    soundstreamhandle.cpp
    WWAudio.cpp
    sdlmixer_mss.cpp
)

if(RENEGADE_AUDIO_BACKEND STREQUAL "SDL_MIXER")
    set(WWAUDIO_SOURCES ${WWAUDIO_COMMON_SOURCES} ${WWAUDIO_SDLMIXER_SOURCES})
else()
    set(WWAUDIO_SOURCES ${WWAUDIO_COMMON_SOURCES} ${WWAUDIO_NULL_SOURCES})
endif()

add_library(WWAudio STATIC ${WWAUDIO_SOURCES})
renegade_configure_target(WWAudio)

set(WWAUDIO_COMPAT_INCLUDE_DIRS)
if(NOT WIN32)
    list(APPEND WWAUDIO_COMPAT_INCLUDE_DIRS "${PROJECT_SOURCE_DIR}/Code/compat")
endif()

target_include_directories(WWAudio
    PUBLIC
        ${WWAUDIO_COMPAT_INCLUDE_DIRS}
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${PROJECT_SOURCE_DIR}/Code/wwlib"
        "${PROJECT_SOURCE_DIR}/Code/wwdebug"
        "${PROJECT_SOURCE_DIR}/Code/WWMath"
        "${PROJECT_SOURCE_DIR}/Code/wwutil"
        "${PROJECT_SOURCE_DIR}/Code/wwsaveload"
        "${PROJECT_SOURCE_DIR}/Code/wwtranslatedb"
        "${PROJECT_SOURCE_DIR}/Code/ww3d2"
)

target_link_libraries(WWAudio PRIVATE wwlib wwdebug WWMath wwutil wwsaveload wwtranslatedb)

if(RENEGADE_AUDIO_BACKEND STREQUAL "SDL_MIXER")
    target_compile_definitions(WWAudio PUBLIC WWAUDIO_USE_NULL_BACKEND=0 WWAUDIO_USE_SDL_MIXER_BACKEND=1)
    target_link_libraries(WWAudio PRIVATE SDL3::SDL3 SDL3_mixer::SDL3_mixer)
else()
    target_compile_definitions(WWAudio PUBLIC WWAUDIO_USE_NULL_BACKEND=1)
endif()

if(NOT WIN32)
    target_compile_definitions(WWAudio PUBLIC _UNIX)
endif()
