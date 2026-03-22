file(GLOB SCONTROL_HEADERS CONFIGURE_DEPENDS "*.h")

set(SCONTROL_SOURCES
    servercontrol.cpp
    servercontrolsocket.cpp
)

add_library(SControl STATIC ${SCONTROL_SOURCES} ${SCONTROL_HEADERS})
renegade_configure_target(SControl)

target_include_directories(SControl
    PUBLIC
        "${PROJECT_SOURCE_DIR}/Code"
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${PROJECT_SOURCE_DIR}/Code/wwlib"
        "${PROJECT_SOURCE_DIR}/Code/wwdebug"
        "${PROJECT_SOURCE_DIR}/Code/wwnet"
)

target_link_libraries(SControl PRIVATE wwlib wwdebug wwnet SDL3::SDL3)

if(NOT WIN32)
    target_compile_definitions(SControl PUBLIC _UNIX)
endif()