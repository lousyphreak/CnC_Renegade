file(GLOB WWBITPACK_SOURCES CONFIGURE_DEPENDS "*.cpp")
file(GLOB WWBITPACK_HEADERS CONFIGURE_DEPENDS "*.h")

add_library(wwbitpack STATIC ${WWBITPACK_SOURCES} ${WWBITPACK_HEADERS})
renegade_configure_target(wwbitpack)

target_include_directories(wwbitpack
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${PROJECT_SOURCE_DIR}/external/SDL/include"
)
