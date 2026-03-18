file(GLOB WWLIB_BOOTSTRAP_HEADERS CONFIGURE_DEPENDS "*.h")

set(RENEGADE_WITH_LEGACY_IMAGEHLP_STACKTRACE OFF)
if(WIN32 AND RENEGADE_WITH_WIN32_STACKTRACE AND MSVC AND CMAKE_SIZEOF_VOID_P EQUAL 4)
    set(RENEGADE_WITH_LEGACY_IMAGEHLP_STACKTRACE ON)
endif()

set(WWLIB_SOURCES
    cpudetect_stub.cpp
    systimer.cpp
)

if(RENEGADE_WITH_LEGACY_IMAGEHLP_STACKTRACE)
    list(APPEND WWLIB_SOURCES Except.cpp)
else()
    list(APPEND WWLIB_SOURCES except_stub.cpp)
endif()

if(WIN32)
    list(APPEND WWLIB_SOURCES
        mutex.cpp
        thread.cpp
    )
else()
    list(APPEND WWLIB_SOURCES
        mutex_posix.cpp
        thread_posix.cpp
    )
endif()

add_library(wwlib STATIC ${WWLIB_SOURCES} ${WWLIB_BOOTSTRAP_HEADERS})
renegade_configure_target(wwlib)

target_include_directories(wwlib
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${PROJECT_SOURCE_DIR}/Code/WWMath"
        "${PROJECT_SOURCE_DIR}/Code/wwdebug"
        "${PROJECT_SOURCE_DIR}/Code/wwutil"
)

if(NOT WIN32)
    target_compile_definitions(wwlib PUBLIC _UNIX)
endif()
