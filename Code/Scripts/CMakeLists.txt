file(GLOB SCRIPTS_SOURCES CONFIGURE_DEPENDS "*.cpp")
file(GLOB SCRIPTS_HEADERS CONFIGURE_DEPENDS "*.h")

list(APPEND SCRIPTS_SOURCES
    "${PROJECT_SOURCE_DIR}/Code/wwlib/slnode.cpp"
)

add_library(Scripts SHARED ${SCRIPTS_SOURCES} ${SCRIPTS_HEADERS})
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

target_link_libraries(Scripts PRIVATE wwlib WWMath wwdebug SDL3::SDL3)

target_compile_definitions(Scripts PRIVATE BUILDING_DLL=1)

if(MSVC)
    target_compile_options(Scripts PRIVATE /Zc:strictStrings-)
endif()

set_target_properties(Scripts PROPERTIES
    OUTPUT_NAME "Scripts"
    PREFIX ""
)
