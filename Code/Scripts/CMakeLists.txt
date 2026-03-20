file(GLOB SCRIPTS_SOURCES CONFIGURE_DEPENDS "*.cpp")
file(GLOB SCRIPTS_HEADERS CONFIGURE_DEPENDS "*.h")

list(APPEND SCRIPTS_SOURCES
    "${PROJECT_SOURCE_DIR}/Code/wwlib/slnode.cpp"
)

add_library(Scripts SHARED ${SCRIPTS_SOURCES} ${SCRIPTS_HEADERS})
renegade_configure_target(Scripts)

target_include_directories(Scripts
    PUBLIC
        "${PROJECT_SOURCE_DIR}/Code/compat"
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${PROJECT_SOURCE_DIR}/Code/wwlib"
    "${PROJECT_SOURCE_DIR}/Code/wwdebug"
        "${PROJECT_SOURCE_DIR}/Code/WWMath"
)

target_link_libraries(Scripts PRIVATE wwlib WWMath wwdebug SDL3::SDL3)

target_compile_definitions(Scripts PRIVATE BUILDING_DLL=1)

set_target_properties(Scripts PROPERTIES
    OUTPUT_NAME "Scripts"
    PREFIX ""
)
