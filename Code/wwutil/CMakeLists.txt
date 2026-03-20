set(WWUTIL_SOURCES
    mathutil.cpp
    miscutil.cpp
    stackdump.cpp
)

file(GLOB WWUTIL_HEADERS CONFIGURE_DEPENDS "*.h")

add_library(wwutil STATIC ${WWUTIL_SOURCES} ${WWUTIL_HEADERS})
renegade_configure_target(wwutil)

target_include_directories(wwutil
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_link_libraries(wwutil PRIVATE SDL3::SDL3)
