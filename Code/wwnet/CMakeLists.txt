file(GLOB WWNET_SOURCES CONFIGURE_DEPENDS "*.cpp")
file(GLOB WWNET_HEADERS CONFIGURE_DEPENDS "*.h")

add_library(wwnet STATIC ${WWNET_SOURCES} ${WWNET_HEADERS})
renegade_configure_target(wwnet)

target_include_directories(wwnet
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${PROJECT_SOURCE_DIR}/Code/wwlib"
        "${PROJECT_SOURCE_DIR}/Code/wwdebug"
        "${PROJECT_SOURCE_DIR}/Code/WWMath"
        "${PROJECT_SOURCE_DIR}/Code/wwutil"
        "${PROJECT_SOURCE_DIR}/Code/wwsaveload"
        "${PROJECT_SOURCE_DIR}/Code/wwbitpack"
        "${PROJECT_SOURCE_DIR}/Code/Combat"
        "${PROJECT_SOURCE_DIR}/Code/Commando"
)

target_link_libraries(wwnet PRIVATE wwlib wwdebug WWMath wwutil wwsaveload wwbitpack)

if(WIN32)
    target_link_libraries(wwnet PUBLIC ws2_32)
endif()

if(NOT WIN32)
    target_compile_definitions(wwnet PUBLIC _UNIX)
endif()
