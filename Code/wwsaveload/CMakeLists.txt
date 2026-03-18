file(GLOB WWSAVELOAD_SOURCES CONFIGURE_DEPENDS "*.cpp")
file(GLOB WWSAVELOAD_HEADERS CONFIGURE_DEPENDS "*.h")

add_library(wwsaveload STATIC ${WWSAVELOAD_SOURCES} ${WWSAVELOAD_HEADERS})
renegade_configure_target(wwsaveload)

target_include_directories(wwsaveload
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${PROJECT_SOURCE_DIR}/Code/wwlib"
        "${PROJECT_SOURCE_DIR}/Code/wwdebug"
        "${PROJECT_SOURCE_DIR}/Code/WWMath"
        "${PROJECT_SOURCE_DIR}/Code/wwutil"
)

target_link_libraries(wwsaveload PRIVATE wwlib wwdebug WWMath wwutil)

if(NOT WIN32)
    target_compile_definitions(wwsaveload PUBLIC _UNIX)
endif()
