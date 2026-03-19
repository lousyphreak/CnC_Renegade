file(GLOB SCRIPTS_SOURCES CONFIGURE_DEPENDS "*.cpp")
file(GLOB SCRIPTS_HEADERS CONFIGURE_DEPENDS "*.h")

add_library(Scripts SHARED ${SCRIPTS_SOURCES} ${SCRIPTS_HEADERS})
renegade_configure_target(Scripts)

target_include_directories(Scripts
    PUBLIC
        "${PROJECT_SOURCE_DIR}/Code/compat"
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${PROJECT_SOURCE_DIR}/Code/wwlib"
        "${PROJECT_SOURCE_DIR}/Code/WWMath"
)

target_compile_definitions(Scripts PRIVATE BUILDING_DLL=1)

set_target_properties(Scripts PROPERTIES
    OUTPUT_NAME "Scripts"
    PREFIX ""
)

if(NOT WIN32)
    target_compile_definitions(Scripts PUBLIC _UNIX)
endif()
