set(WWDEBUG_SOURCES
    wwdebug.cpp
    wwmemlog.cpp
    wwperfmon.cpp
    wwprofile.cpp
)

file(GLOB WWDEBUG_HEADERS CONFIGURE_DEPENDS "*.h")

add_library(wwdebug STATIC ${WWDEBUG_SOURCES} ${WWDEBUG_HEADERS})
renegade_configure_target(wwdebug)

target_include_directories(wwdebug
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_link_libraries(wwdebug PRIVATE SDL3::SDL3)
