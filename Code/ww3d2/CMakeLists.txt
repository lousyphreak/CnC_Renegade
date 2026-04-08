file(GLOB WW3D2_SOURCES CONFIGURE_DEPENDS "*.cpp")
file(GLOB WW3D2_HEADERS CONFIGURE_DEPENDS "*.h")
file(GLOB WW3D2_SHADER_SOURCES CONFIGURE_DEPENDS "shaders/*.sc")

if(NOT WIN32)
    list(REMOVE_ITEM WW3D2_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/FramGrab.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/framgrab.cpp"
    )
endif()

list(REMOVE_ITEM WW3D2_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/dx8caps.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/dx8indexbuffer.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/dx8vertexbuffer.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/dx8wrapper.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/bgfxmeshstubs.cpp"
)

add_library(ww3d2 STATIC ${WW3D2_SOURCES} ${WW3D2_HEADERS})
renegade_configure_target(ww3d2)

set(WW3D2_SHADER_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated/bgfx-shaders")
set(WW3D2_SHADER_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/shaders")
set(WW3D2_BGFX_SHADER_INCLUDE_DIR "${bgfx_cmake_SOURCE_DIR}/bgfx/src")
set(WW3D2_SHADER_VARYING_DEF "${WW3D2_SHADER_INCLUDE_DIR}/varying.def.sc")

bgfx_compile_shaders(
    TYPE VERTEX
    SHADERS "${WW3D2_SHADER_INCLUDE_DIR}/vs_fixed_function.sc"
    VARYING_DEF "${WW3D2_SHADER_VARYING_DEF}"
    OUTPUT_DIR "${WW3D2_SHADER_OUTPUT_DIR}"
    OUT_FILES_VAR WW3D2_VERTEX_SHADER_OUTPUTS
    INCLUDE_DIRS "${WW3D2_BGFX_SHADER_INCLUDE_DIR}" "${WW3D2_SHADER_INCLUDE_DIR}"
)

bgfx_compile_shaders(
    TYPE FRAGMENT
    SHADERS "${WW3D2_SHADER_INCLUDE_DIR}/fs_fixed_function.sc"
    VARYING_DEF "${WW3D2_SHADER_VARYING_DEF}"
    OUTPUT_DIR "${WW3D2_SHADER_OUTPUT_DIR}"
    OUT_FILES_VAR WW3D2_FRAGMENT_SHADER_OUTPUTS
    INCLUDE_DIRS "${WW3D2_BGFX_SHADER_INCLUDE_DIR}" "${WW3D2_SHADER_INCLUDE_DIR}"
)

add_custom_target(
    ww3d2-shaders
    DEPENDS
        ${WW3D2_VERTEX_SHADER_OUTPUTS}
        ${WW3D2_FRAGMENT_SHADER_OUTPUTS}
        ${WW3D2_SHADER_SOURCES}
)

add_dependencies(ww3d2 ww3d2-shaders)

set(WW3D2_COMPAT_INCLUDE_DIRS)
if(NOT WIN32)
    list(APPEND WW3D2_COMPAT_INCLUDE_DIRS "${PROJECT_SOURCE_DIR}/Code/compat")
endif()

target_include_directories(ww3d2
    PUBLIC
        ${WW3D2_COMPAT_INCLUDE_DIRS}
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${PROJECT_SOURCE_DIR}/external/stb"
        "${PROJECT_SOURCE_DIR}/Code/wwlib"
        "${PROJECT_SOURCE_DIR}/Code/wwdebug"
        "${PROJECT_SOURCE_DIR}/Code/WWMath"
        "${PROJECT_SOURCE_DIR}/Code/wwutil"
        "${PROJECT_SOURCE_DIR}/Code/wwsaveload"
        "${PROJECT_SOURCE_DIR}/Code/wwtranslatedb"
        "${PROJECT_SOURCE_DIR}/Code/WWAudio"
)

target_link_libraries(ww3d2
    PUBLIC
        bgfx
    PRIVATE
        wwlib
        wwdebug
        WWMath
        wwutil
        wwsaveload
        wwtranslatedb
)

target_compile_definitions(ww3d2
    PRIVATE
        RENEGADE_WW3D2_SHADER_DIR="${WW3D2_SHADER_OUTPUT_DIR}"
)
