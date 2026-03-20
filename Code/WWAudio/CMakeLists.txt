file(GLOB WWAUDIO_HEADERS CONFIGURE_DEPENDS "*.h")

add_library(WWAudio STATIC
    AudioSaveLoad_null.cpp
    wwaudio_null.cpp
    ${WWAUDIO_HEADERS}
)
renegade_configure_target(WWAudio)

target_include_directories(WWAudio
    PUBLIC
        "${PROJECT_SOURCE_DIR}/Code/compat"
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

if(NOT WIN32)
    target_compile_definitions(WWAudio PUBLIC _UNIX WWAUDIO_USE_NULL_BACKEND=1)
endif()
