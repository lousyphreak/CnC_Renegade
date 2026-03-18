set(WWUTIL_SOURCES
    mathutil.cpp
    miscutil.cpp
)

set(RENEGADE_WITH_LEGACY_IMAGEHLP_STACKTRACE OFF)
if(WIN32 AND RENEGADE_WITH_WIN32_STACKTRACE AND MSVC AND CMAKE_SIZEOF_VOID_P EQUAL 4)
    set(RENEGADE_WITH_LEGACY_IMAGEHLP_STACKTRACE ON)
endif()

if(RENEGADE_WITH_LEGACY_IMAGEHLP_STACKTRACE)
    list(APPEND WWUTIL_SOURCES stackdump.cpp)
else()
    list(APPEND WWUTIL_SOURCES stackdump_stub.cpp)
endif()

file(GLOB WWUTIL_HEADERS CONFIGURE_DEPENDS "*.h")

add_library(wwutil STATIC ${WWUTIL_SOURCES} ${WWUTIL_HEADERS})
renegade_configure_target(wwutil)

target_include_directories(wwutil
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${PROJECT_SOURCE_DIR}/Code/wwlib"
        "${PROJECT_SOURCE_DIR}/Code/wwdebug"
        "${PROJECT_SOURCE_DIR}/Code/WWMath"
)

target_link_libraries(wwutil PRIVATE wwlib wwdebug WWMath)

if(NOT WIN32)
    target_compile_definitions(wwutil PUBLIC _UNIX)
endif()
