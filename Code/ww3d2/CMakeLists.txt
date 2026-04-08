file(GLOB WW3D2_SOURCES CONFIGURE_DEPENDS "*.cpp")
file(GLOB WW3D2_HEADERS CONFIGURE_DEPENDS "*.h")

if(NOT WIN32)
    list(REMOVE_ITEM WW3D2_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/FramGrab.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/framgrab.cpp"
    )
endif()

list(REMOVE_ITEM WW3D2_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/dx8caps.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/dx8fvf.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/dx8indexbuffer.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/dx8polygonrenderer.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/dx8renderer.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/dx8rendererdebugger.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/dx8texman.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/dx8vertexbuffer.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/dx8wrapper.cpp"
)

add_library(ww3d2 STATIC ${WW3D2_SOURCES} ${WW3D2_HEADERS})
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

target_link_libraries(ww3d2
    PRIVATE
        bgfx
        wwlib
        wwdebug
        WWMath
        wwutil
        wwsaveload
        wwtranslatedb
)
