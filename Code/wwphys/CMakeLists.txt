file(GLOB WWPHYS_SOURCES CONFIGURE_DEPENDS "*.cpp")
file(GLOB WWPHYS_HEADERS CONFIGURE_DEPENDS "*.h")

if(NOT RENEGADE_WITH_UMBRA)
    list(REMOVE_ITEM WWPHYS_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/umbrasupport.cpp"
    )
endif()

list(REMOVE_ITEM WWPHYS_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/PathfindSectorBuilder.cpp"
)

add_library(wwphys STATIC ${WWPHYS_SOURCES} ${WWPHYS_HEADERS})
renegade_configure_target(wwphys)

target_include_directories(wwphys
    PUBLIC
        "${PROJECT_SOURCE_DIR}/Code/compat"
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${PROJECT_SOURCE_DIR}/Code/wwlib"
        "${PROJECT_SOURCE_DIR}/Code/wwdebug"
        "${PROJECT_SOURCE_DIR}/Code/WWMath"
        "${PROJECT_SOURCE_DIR}/Code/wwutil"
        "${PROJECT_SOURCE_DIR}/Code/wwsaveload"
        "${PROJECT_SOURCE_DIR}/Code/wwbitpack"
        "${PROJECT_SOURCE_DIR}/Code/wwtranslatedb"
        "${PROJECT_SOURCE_DIR}/Code/ww3d2"
)

target_link_libraries(wwphys PRIVATE ww3d2 wwlib wwdebug WWMath wwutil wwsaveload wwbitpack wwtranslatedb)
