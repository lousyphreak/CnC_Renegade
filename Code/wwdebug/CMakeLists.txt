set(WWDEBUG_SOURCES
    wwdebug.cpp
    wwprofile.cpp
)

if(WIN32)
    list(APPEND WWDEBUG_SOURCES wwmemlog.cpp)
else()
    list(APPEND WWDEBUG_SOURCES wwmemlog_stub.cpp)
endif()

file(GLOB WWDEBUG_HEADERS CONFIGURE_DEPENDS "*.h")

add_library(wwdebug STATIC ${WWDEBUG_SOURCES} ${WWDEBUG_HEADERS})
renegade_configure_target(wwdebug)

target_include_directories(wwdebug
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${PROJECT_SOURCE_DIR}/Code/wwlib"
        "${PROJECT_SOURCE_DIR}/Code/WWMath"
)

target_link_libraries(wwdebug PRIVATE wwlib WWMath)

if(NOT WIN32)
    target_compile_definitions(wwdebug PUBLIC _UNIX)
endif()
