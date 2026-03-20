file(GLOB WWLIB_BOOTSTRAP_HEADERS CONFIGURE_DEPENDS "*.h")

set(WWLIB_SOURCES
    LaunchWeb.cpp
    FastAllocator.cpp
    except_stub.cpp
    multilist.cpp
    mutex.cpp
    msgloop.cpp
    refcount.cpp
    registry.cpp
    random.cpp
    sha.cpp
    slnode.cpp
    srandom.cpp
    stimer.cpp
    surface.cpp
    systimer.cpp
    thread.cpp
    widestring.cpp
    wwstring.cpp
    xsurface.cpp
)

add_library(wwlib STATIC ${WWLIB_SOURCES} ${WWLIB_BOOTSTRAP_HEADERS})
renegade_configure_target(wwlib)

target_include_directories(wwlib
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_link_libraries(wwlib PRIVATE SDL3::SDL3)
