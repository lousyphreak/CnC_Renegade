file(GLOB BINKMOVIE_HEADERS CONFIGURE_DEPENDS "*.h")

if(RENEGADE_WITH_BINK)
    add_library(BinkMovie STATIC
        BINKMovie.cpp
        subtitle.cpp
        subtitlemanager.cpp
        subtitleparser.cpp
        ${BINKMOVIE_HEADERS}
    )
else()
    add_library(BinkMovie STATIC
        binkmovie_stub.cpp
        ${BINKMOVIE_HEADERS}
    )
endif()

renegade_configure_target(BinkMovie)

target_include_directories(BinkMovie
    PUBLIC
        "${PROJECT_SOURCE_DIR}/Code/compat"
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${PROJECT_SOURCE_DIR}/Code/wwlib"
        "${PROJECT_SOURCE_DIR}/Code/wwdebug"
        "${PROJECT_SOURCE_DIR}/Code/WWMath"
        "${PROJECT_SOURCE_DIR}/Code/ww3d2"
)

target_link_libraries(BinkMovie PRIVATE wwlib wwdebug WWMath)

if(NOT WIN32)
    target_compile_definitions(BinkMovie PUBLIC _UNIX)
endif()
