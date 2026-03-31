file(GLOB WWLIB_BOOTSTRAP_HEADERS CONFIGURE_DEPENDS "*.h")

set(WWLIB_SOURCES
    LaunchWeb.cpp
    FastAllocator.cpp
    b64pipe.cpp
    b64straw.cpp
    base64.cpp
    buff.cpp
    bufffile.cpp
    chunkio.cpp
    crc.cpp
    cstraw.cpp
    data.cpp
    except_stub.cpp
    ffactory.cpp
    gcd_lcm.cpp
    hash.cpp
    ini.cpp
    int.cpp
    lzo.cpp
    lzo1x_c.cpp
    lzo1x_d.cpp
    jshell.cpp
    lzopipe.cpp
    lzostraw.cpp
    mixfile.cpp
    mono.cpp
    multilist.cpp
    mpmath.cpp
    mutex.cpp
    msgloop.cpp
    nstrdup.cpp
    pipe.cpp
    pk.cpp
    refcount.cpp
    realcrc.cpp
    registry.cpp
    rawfile.cpp
    readline.cpp
    random.cpp
    rndstrng.cpp
    sha.cpp
    slnode.cpp
    srandom.cpp
    stimer.cpp
    straw.cpp
    surface.cpp
    systimer.cpp
    textfile.cpp
    thread.cpp
    TARGA.CPP
    trim.cpp
    vector.cpp
    win.cpp
    wwfile.cpp
    widestring.cpp
    wwstring.cpp
    load.cpp
    xpipe.cpp
    xsurface.cpp
    xstraw.cpp
)

if(MSVC AND CMAKE_SIZEOF_VOID_P EQUAL 4 AND RENEGADE_WITH_X86_ASM)
    list(APPEND WWLIB_SOURCES lcw.cpp)
else()
    list(APPEND WWLIB_SOURCES lcw_portable.cpp)
endif()

add_library(wwlib STATIC ${WWLIB_SOURCES} ${WWLIB_BOOTSTRAP_HEADERS})
renegade_configure_target(wwlib)

target_include_directories(wwlib
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}"
    "${PROJECT_SOURCE_DIR}/Code/wwdebug"
)

target_link_libraries(wwlib PUBLIC SDL3::SDL3)
