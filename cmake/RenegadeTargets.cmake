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
        RENEGADE_WITH_BGFX_RENDERER=$<BOOL:${RENEGADE_WITH_BGFX_RENDERER}>
        RENEGADE_WITH_SCRIPT_DLL=$<BOOL:${RENEGADE_WITH_SCRIPT_DLL}>
        RENEGADE_WITH_BANDTEST=$<BOOL:${RENEGADE_WITH_BANDTEST}>
        RENEGADE_WITH_SCONTROL=$<BOOL:${RENEGADE_WITH_SCONTROL}>
)

set(_renegade_requested_sanitizers)
if(RENEGADE_ENABLE_ASAN)
    list(APPEND _renegade_requested_sanitizers address)
endif()
if(RENEGADE_ENABLE_UBSAN)
    list(APPEND _renegade_requested_sanitizers undefined)
endif()
if(RENEGADE_ENABLE_TSAN)
    list(APPEND _renegade_requested_sanitizers thread)
endif()

if(RENEGADE_ENABLE_ASAN AND RENEGADE_ENABLE_TSAN)
    message(FATAL_ERROR
        "RENEGADE_ENABLE_ASAN and RENEGADE_ENABLE_TSAN cannot be enabled together. "
        "Use a separate build tree if you want to run ThreadSanitizer.")
endif()

if(_renegade_requested_sanitizers)
    if(MSVC)
        message(FATAL_ERROR "Renegade sanitizer options currently require Clang or GCC style -fsanitize support.")
    endif()

    if(NOT CMAKE_C_COMPILER_ID MATCHES "^(Clang|GNU)$")
        message(FATAL_ERROR "Renegade sanitizer options require a Clang or GNU C compiler.")
    endif()

    if(NOT CMAKE_CXX_COMPILER_ID MATCHES "^(Clang|GNU)$")
        message(FATAL_ERROR "Renegade sanitizer options require a Clang or GNU C++ compiler.")
    endif()

    string(JOIN "," _renegade_sanitizer_kinds ${_renegade_requested_sanitizers})
    set(_renegade_sanitizer_flag "-fsanitize=${_renegade_sanitizer_kinds}")

    target_compile_options(renegade_project_options
        INTERFACE
            ${_renegade_sanitizer_flag}
            -fno-omit-frame-pointer
    )
    target_link_options(renegade_project_options
        INTERFACE
            ${_renegade_sanitizer_flag}
    )

    message(STATUS "Renegade sanitizers enabled: ${_renegade_sanitizer_kinds}")
endif()

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
