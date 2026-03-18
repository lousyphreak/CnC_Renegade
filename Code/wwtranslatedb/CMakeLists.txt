file(GLOB WWTRANSLATEDB_SOURCES CONFIGURE_DEPENDS "*.cpp")
file(GLOB WWTRANSLATEDB_HEADERS CONFIGURE_DEPENDS "*.h")

add_library(wwtranslatedb STATIC ${WWTRANSLATEDB_SOURCES} ${WWTRANSLATEDB_HEADERS})
renegade_configure_target(wwtranslatedb)

target_include_directories(wwtranslatedb
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${PROJECT_SOURCE_DIR}/Code/wwlib"
        "${PROJECT_SOURCE_DIR}/Code/wwdebug"
        "${PROJECT_SOURCE_DIR}/Code/WWMath"
        "${PROJECT_SOURCE_DIR}/Code/wwutil"
        "${PROJECT_SOURCE_DIR}/Code/wwsaveload"
)

target_link_libraries(wwtranslatedb PRIVATE wwlib wwdebug WWMath wwutil wwsaveload)

if(NOT WIN32)
    target_compile_definitions(wwtranslatedb PUBLIC _UNIX)
endif()
