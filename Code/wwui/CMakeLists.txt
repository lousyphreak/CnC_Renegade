file(GLOB WWUI_SOURCES CONFIGURE_DEPENDS "*.cpp")
file(GLOB WWUI_HEADERS CONFIGURE_DEPENDS "*.h")

if(NOT WIN32)
    list(REMOVE_ITEM WWUI_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/IMECandidate.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/IMEManager.cpp"
    )
endif()

add_library(wwui STATIC ${WWUI_SOURCES} ${WWUI_HEADERS})
renegade_configure_target(wwui)

target_include_directories(wwui
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${PROJECT_SOURCE_DIR}/Code/wwlib"
        "${PROJECT_SOURCE_DIR}/Code/wwdebug"
        "${PROJECT_SOURCE_DIR}/Code/WWMath"
        "${PROJECT_SOURCE_DIR}/Code/wwutil"
        "${PROJECT_SOURCE_DIR}/Code/wwsaveload"
        "${PROJECT_SOURCE_DIR}/Code/wwtranslatedb"
        "${PROJECT_SOURCE_DIR}/Code/ww3d2"
        "${PROJECT_SOURCE_DIR}/Code/WWAudio"
)

target_link_libraries(wwui PRIVATE ww3d2 wwlib wwdebug WWMath wwutil wwsaveload wwtranslatedb)

if(NOT WIN32)
    target_compile_definitions(wwui PUBLIC _UNIX)
endif()
