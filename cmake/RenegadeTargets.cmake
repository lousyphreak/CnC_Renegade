include_guard(GLOBAL)

add_library(renegade_project_options INTERFACE)
add_library(renegade::project_options ALIAS renegade_project_options)

target_compile_features(renegade_project_options INTERFACE cxx_std_17)
target_include_directories(renegade_project_options
    INTERFACE
        "${PROJECT_SOURCE_DIR}/Code"
        "${PROJECT_BINARY_DIR}/generated"
)
target_compile_definitions(renegade_project_options
    INTERFACE
        RENEGADE_WITH_SDL3=$<BOOL:${RENEGADE_WITH_SDL3}>
        RENEGADE_WITH_X86_ASM=$<BOOL:${RENEGADE_WITH_X86_ASM}>
        RENEGADE_WITH_UMBRA=$<BOOL:${RENEGADE_WITH_UMBRA}>
        RENEGADE_WITH_BINK=$<BOOL:${RENEGADE_WITH_BINK}>
        RENEGADE_WITH_MILES=$<BOOL:${RENEGADE_WITH_MILES}>
        RENEGADE_WITH_GAMESPY=$<BOOL:${RENEGADE_WITH_GAMESPY}>
        RENEGADE_WITH_LEGACY_WOL=$<BOOL:${RENEGADE_WITH_LEGACY_WOL}>
        RENEGADE_WITH_DX8_RENDERER=$<BOOL:${RENEGADE_WITH_DX8_RENDERER}>
        RENEGADE_WITH_DIRECTINPUT=$<BOOL:${RENEGADE_WITH_DIRECTINPUT}>
        RENEGADE_WITH_SCRIPT_DLL=$<BOOL:${RENEGADE_WITH_SCRIPT_DLL}>
        RENEGADE_WITH_BANDTEST=$<BOOL:${RENEGADE_WITH_BANDTEST}>
        RENEGADE_WITH_SCONTROL=$<BOOL:${RENEGADE_WITH_SCONTROL}>
)

if(MSVC)
    target_compile_options(renegade_project_options INTERFACE /W4 /permissive- /EHsc /bigobj)
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_compile_options(renegade_project_options INTERFACE -Wall -Wextra -Wpedantic -fpermissive)
else()
    target_compile_options(renegade_project_options INTERFACE -Wall -Wextra -Wpedantic)
endif()

function(renegade_configure_target target_name)
    if(NOT TARGET "${target_name}")
        message(FATAL_ERROR "renegade_configure_target called for missing target: ${target_name}")
    endif()

    target_link_libraries("${target_name}" PRIVATE renegade::project_options)
    set_target_properties("${target_name}" PROPERTIES FOLDER "Renegade")
endfunction()
