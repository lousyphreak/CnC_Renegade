file(GLOB WW3D2_HEADERS CONFIGURE_DEPENDS "*.h")

set(WW3D2_SOURCES
    aabtree.cpp
    aabtreebuilder.cpp
    assetstatus.cpp
    bitmaphandler.cpp
    boxrobj_headless.cpp
    bwrender.cpp
    camera.cpp
    coltest.cpp
    decalmsh_headless.cpp
    hanim.cpp
    hanimmgr.cpp
    hcanim.cpp
    hmorphanim.cpp
    hrawanim.cpp
    htree.cpp
    htreemgr.cpp
    lightenvironment.cpp
    light.cpp
    mapper.cpp
    matpass.cpp
    matrixmapper.cpp
    mesh.cpp
    meshgeometry.cpp
    meshmatdesc.cpp
    meshmdlio.cpp
    meshmdl.cpp
    matinfo.cpp
    motchan.cpp
    line3d.cpp
    particle_renderers_headless.cpp
    part_buf.cpp
    part_emt.cpp
    part_ldr.cpp
    predlod.cpp
    projector.cpp
    pivot.cpp
    render2d.cpp
    render2dsentence_headless.cpp
    rendobj.cpp
    rinfo.cpp
    ringobj.cpp
    renderobjectrecycler.cpp
    scene.cpp
    segline.cpp
    soundrobj.cpp
    sphereobj.cpp
    statistics_headless.cpp
    assetmgr_headless.cpp
    animatedsoundmgr.cpp
    dazzle_headless.cpp
    decalsys_headless.cpp
    dx8rendererdebugger.cpp
    dx8renderer_headless.cpp
    font3d.cpp
    shader_headless.cpp
    shattersystem_headless.cpp
    sortingrenderer_headless.cpp
    surfaceclass_headless.cpp
    texture_headless.cpp
    textureloader_headless.cpp
    texturethumbnail_headless.cpp
    texproject_headless.cpp
    vertmaterial_headless.cpp
    visrasterizer.cpp
    w3d_util.cpp
    ww3d_headless.cpp
    ww3dformat_headless.cpp
)

set(WW3D2_BGFX_SHADER_SOURCES)
if(RENEGADE_WITH_BGFX_RENDERER)
    list(APPEND WW3D2_SOURCES dx8wrapper.cpp)

    set(WW3D2_BGFX_SHADER_DIR "${CMAKE_CURRENT_SOURCE_DIR}/shaders")
    set(WW3D2_BGFX_GENERATED_SHADER_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated/shaders")

    bgfx_compile_shaders(
        TYPE VERTEX
        SHADERS "${WW3D2_BGFX_SHADER_DIR}/vs_bootstrap.sc"
        VARYING_DEF "${WW3D2_BGFX_SHADER_DIR}/varying.def.sc"
        OUTPUT_DIR "${WW3D2_BGFX_GENERATED_SHADER_DIR}"
        INCLUDE_DIRS "${bgfx_cmake_SOURCE_DIR}/bgfx/src"
        AS_HEADERS
    )
    bgfx_compile_shaders(
        TYPE FRAGMENT
        SHADERS "${WW3D2_BGFX_SHADER_DIR}/fs_bootstrap.sc"
        VARYING_DEF "${WW3D2_BGFX_SHADER_DIR}/varying.def.sc"
        OUTPUT_DIR "${WW3D2_BGFX_GENERATED_SHADER_DIR}"
        INCLUDE_DIRS "${bgfx_cmake_SOURCE_DIR}/bgfx/src"
        AS_HEADERS
    )

    list(APPEND WW3D2_BGFX_SHADER_SOURCES
        "${WW3D2_BGFX_SHADER_DIR}/varying.def.sc"
        "${WW3D2_BGFX_SHADER_DIR}/vs_bootstrap.sc"
        "${WW3D2_BGFX_SHADER_DIR}/fs_bootstrap.sc"
    )
endif()

add_library(ww3d2 STATIC ${WW3D2_SOURCES} ${WW3D2_HEADERS} ${WW3D2_BGFX_SHADER_SOURCES})
renegade_configure_target(ww3d2)

set(WW3D2_COMPAT_INCLUDE_DIRS)
if(NOT WIN32)
    list(APPEND WW3D2_COMPAT_INCLUDE_DIRS "${PROJECT_SOURCE_DIR}/Code/compat")
endif()

target_include_directories(ww3d2
    PUBLIC
        ${WW3D2_COMPAT_INCLUDE_DIRS}
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${PROJECT_SOURCE_DIR}/Code/wwlib"
        "${PROJECT_SOURCE_DIR}/Code/wwdebug"
        "${PROJECT_SOURCE_DIR}/Code/WWMath"
        "${PROJECT_SOURCE_DIR}/Code/wwutil"
        "${PROJECT_SOURCE_DIR}/Code/wwsaveload"
        "${PROJECT_SOURCE_DIR}/Code/wwtranslatedb"
        "${PROJECT_SOURCE_DIR}/Code/WWAudio"
)

if(RENEGADE_WITH_BGFX_RENDERER)
    target_include_directories(ww3d2 PRIVATE "${WW3D2_BGFX_GENERATED_SHADER_DIR}")
endif()

target_link_libraries(ww3d2 PRIVATE wwlib wwdebug WWMath wwutil wwsaveload wwtranslatedb WWAudio SDL3::SDL3)

if(RENEGADE_WITH_BGFX_RENDERER)
    if(TARGET bgfx::bgfx)
        target_link_libraries(ww3d2 PRIVATE bgfx::bgfx)
    elseif(TARGET bgfx)
        target_link_libraries(ww3d2 PRIVATE bgfx)
    else()
        message(FATAL_ERROR "RENEGADE_WITH_BGFX_RENDERER is enabled, but no bgfx target was exported by bgfx.cmake.")
    endif()
endif()

if(NOT WIN32)
    target_compile_definitions(ww3d2 PUBLIC _UNIX)
endif()