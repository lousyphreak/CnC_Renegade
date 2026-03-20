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
    PRIVATE
        "${RENEGADE_SDL3_SUBMODULE_PATH}/include"
)
