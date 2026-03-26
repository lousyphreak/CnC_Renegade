file(GLOB WWAUDIO_HEADERS CONFIGURE_DEPENDS "*.h")

add_library(WWAudio STATIC
    AudioEvents.cpp
    AudibleSound.cpp
    AudioSaveLoad_null.cpp
    Listener.cpp
    Sound3D.cpp
    SoundSceneObj.cpp
    Threads_headless.cpp
    wwaudio_null.cpp
    ${WWAUDIO_HEADERS}
)
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

if(NOT RENEGADE_WITH_MILES)
    target_compile_definitions(WWAudio PUBLIC WWAUDIO_USE_NULL_BACKEND=1)
endif()

if(NOT WIN32)
    target_compile_definitions(WWAudio PUBLIC _UNIX)
endif()
