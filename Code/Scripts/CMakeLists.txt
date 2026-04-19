file(GLOB SCRIPTS_SOURCES CONFIGURE_DEPENDS "*.cpp")
file(GLOB SCRIPTS_HEADERS CONFIGURE_DEPENDS "*.h")

list(APPEND SCRIPTS_SOURCES
    "${PROJECT_SOURCE_DIR}/Code/wwlib/slnode.cpp"
)

list(REMOVE_ITEM SCRIPTS_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/strtrim.cpp"
)

add_library(Scripts OBJECT ${SCRIPTS_SOURCES} ${SCRIPTS_HEADERS})
renegade_configure_target(Scripts)

set(SCRIPTS_COMPAT_INCLUDE_DIRS)
if(NOT WIN32)
    list(APPEND SCRIPTS_COMPAT_INCLUDE_DIRS "${PROJECT_SOURCE_DIR}/Code/compat")
endif()

target_include_directories(Scripts
    PUBLIC
        ${SCRIPTS_COMPAT_INCLUDE_DIRS}
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${PROJECT_SOURCE_DIR}/Code/wwlib"
    "${PROJECT_SOURCE_DIR}/Code/wwdebug"
        "${PROJECT_SOURCE_DIR}/Code/WWMath"
)

if(MSVC)
    target_compile_options(Scripts PRIVATE /Zc:strictStrings-)
endif()
