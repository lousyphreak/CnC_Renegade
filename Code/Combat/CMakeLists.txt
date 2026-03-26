file(GLOB COMBAT_SOURCES CONFIGURE_DEPENDS "*.cpp")
file(GLOB COMBAT_HEADERS CONFIGURE_DEPENDS "*.h")

list(REMOVE_ITEM COMBAT_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/debug_bootstrap_stub.cpp"
)

add_library(Combat STATIC ${COMBAT_SOURCES} ${COMBAT_HEADERS})
renegade_configure_target(Combat)

set(COMBAT_COMPAT_INCLUDE_DIRS)
if(NOT WIN32)
    list(APPEND COMBAT_COMPAT_INCLUDE_DIRS "${PROJECT_SOURCE_DIR}/Code/compat")
endif()

target_include_directories(Combat
    PUBLIC
        ${COMBAT_COMPAT_INCLUDE_DIRS}
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${PROJECT_SOURCE_DIR}/Code/wwlib"
        "${PROJECT_SOURCE_DIR}/Code/wwdebug"
        "${PROJECT_SOURCE_DIR}/Code/WWMath"
        "${PROJECT_SOURCE_DIR}/Code/wwutil"
        "${PROJECT_SOURCE_DIR}/Code/wwsaveload"
        "${PROJECT_SOURCE_DIR}/Code/wwbitpack"
        "${PROJECT_SOURCE_DIR}/Code/wwtranslatedb"
        "${PROJECT_SOURCE_DIR}/Code/wwui"
        "${PROJECT_SOURCE_DIR}/Code/wwnet"
        "${PROJECT_SOURCE_DIR}/Code/WWAudio"
        "${PROJECT_SOURCE_DIR}/Code/wwphys"
        "${PROJECT_SOURCE_DIR}/Code/ww3d2"
)

target_link_libraries(Combat PRIVATE ww3d2 wwlib wwdebug WWMath wwutil wwsaveload wwbitpack wwtranslatedb wwui wwnet WWAudio wwphys BinkMovie)

if(MSVC)
    target_compile_options(Combat PRIVATE /Zc:strictStrings-)
endif()

if(NOT WIN32)
    target_compile_definitions(Combat PUBLIC _UNIX)
endif()
