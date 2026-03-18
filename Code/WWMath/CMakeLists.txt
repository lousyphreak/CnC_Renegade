file(GLOB WWMATH_SOURCES CONFIGURE_DEPENDS "*.cpp")
file(GLOB WWMATH_HEADERS CONFIGURE_DEPENDS "*.h")

add_library(WWMath STATIC ${WWMATH_SOURCES} ${WWMATH_HEADERS})
renegade_configure_target(WWMath)

target_include_directories(WWMath
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${PROJECT_SOURCE_DIR}/Code/wwlib"
        "${PROJECT_SOURCE_DIR}/Code/wwdebug"
    "${PROJECT_SOURCE_DIR}/Code/wwsaveload"
)

target_link_libraries(WWMath PRIVATE wwlib)

if(NOT WIN32)
    target_compile_definitions(WWMath PUBLIC _UNIX)
endif()
