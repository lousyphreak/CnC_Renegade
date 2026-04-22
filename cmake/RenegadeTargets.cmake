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
    target_compile_options(renegade_project_options INTERFACE -Wall -Wextra -Wpedantic -fpermissive -DWWDEBUG=1)
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

function(renegade_configure_emscripten_target target_name)
    if(NOT EMSCRIPTEN)
        return()
    endif()

    if(NOT TARGET "${target_name}")
        message(FATAL_ERROR "renegade_configure_emscripten_target called for missing target: ${target_name}")
    endif()

    set_target_properties("${target_name}" PROPERTIES SUFFIX ".html")

    math(EXPR _renegade_emscripten_initial_memory_bytes "${RENEGADE_EMSCRIPTEN_INITIAL_MEMORY_MB} * 1024 * 1024")

    target_link_options("${target_name}" PRIVATE
        "SHELL:-sFORCE_FILESYSTEM=1"
        "SHELL:-sINITIAL_MEMORY=${_renegade_emscripten_initial_memory_bytes}"
        "SHELL:-sMIN_WEBGL_VERSION=2"
        "SHELL:-sMAX_WEBGL_VERSION=2"
        "SHELL:-sFULL_ES3=1"
    )

    if(RENEGADE_EMSCRIPTEN_ALLOW_MEMORY_GROWTH)
        math(EXPR _renegade_emscripten_maximum_memory_bytes "${RENEGADE_EMSCRIPTEN_MAXIMUM_MEMORY_MB} * 1024 * 1024")

        if(_renegade_emscripten_initial_memory_bytes GREATER_EQUAL _renegade_emscripten_maximum_memory_bytes)
            message(FATAL_ERROR
                "RENEGADE_EMSCRIPTEN_INITIAL_MEMORY_MB (${RENEGADE_EMSCRIPTEN_INITIAL_MEMORY_MB}) must be smaller than "
                "RENEGADE_EMSCRIPTEN_MAXIMUM_MEMORY_MB (${RENEGADE_EMSCRIPTEN_MAXIMUM_MEMORY_MB}) when memory growth is enabled.")
        endif()

        target_link_options("${target_name}" PRIVATE
            "SHELL:-sALLOW_MEMORY_GROWTH=1"
            "SHELL:-sMAXIMUM_MEMORY=${_renegade_emscripten_maximum_memory_bytes}"
        )
    endif()

    set(_renegade_emscripten_ww3d2_shader_dir "${PROJECT_BINARY_DIR}/Code/ww3d2/generated/bgfx-shaders")
    target_link_options("${target_name}" PRIVATE
        "SHELL:--preload-file ${_renegade_emscripten_ww3d2_shader_dir}@/generated/bgfx-shaders"
    )

    set(EMSCRIPTEN_SHELL_FILE "${CMAKE_SOURCE_DIR}/shell.html")
    target_link_options("${target_name}" PRIVATE "--shell-file" "${EMSCRIPTEN_SHELL_FILE}")
    set_property(TARGET "${target_name}" APPEND PROPERTY LINK_DEPENDS "${EMSCRIPTEN_SHELL_FILE}")

    if(NOT RENEGADE_EMSCRIPTEN_PACKAGE_GAME_DATA)
        return()
    endif()

    if(NOT IS_DIRECTORY "${RENEGADE_EMSCRIPTEN_DATA_ROOT}")
        message(FATAL_ERROR
            "RENEGADE_EMSCRIPTEN_DATA_ROOT='${RENEGADE_EMSCRIPTEN_DATA_ROOT}' does not exist. "
            "Point it at a shipped Renegade data tree before configuring an Emscripten build.")
    endif()

    if(NOT IS_DIRECTORY "${RENEGADE_EMSCRIPTEN_DATA_ROOT}/Data")
        message(FATAL_ERROR
            "RENEGADE_EMSCRIPTEN_DATA_ROOT='${RENEGADE_EMSCRIPTEN_DATA_ROOT}' is missing the Data directory expected by the runtime.")
    endif()

    target_link_options("${target_name}" PRIVATE
        "SHELL:--preload-file ${RENEGADE_EMSCRIPTEN_DATA_ROOT}/Data@/Data"
    )

    foreach(_renegade_emscripten_optional_dir IN ITEMS HTML Internet)
        if(IS_DIRECTORY "${RENEGADE_EMSCRIPTEN_DATA_ROOT}/${_renegade_emscripten_optional_dir}")
            target_link_options("${target_name}" PRIVATE
                "SHELL:--preload-file ${RENEGADE_EMSCRIPTEN_DATA_ROOT}/${_renegade_emscripten_optional_dir}@/${_renegade_emscripten_optional_dir}"
            )
        endif()
    endforeach()

    file(GLOB _renegade_emscripten_root_files
        LIST_DIRECTORIES false
        RELATIVE "${RENEGADE_EMSCRIPTEN_DATA_ROOT}"
        "${RENEGADE_EMSCRIPTEN_DATA_ROOT}/*"
    )

    foreach(_renegade_emscripten_root_file IN LISTS _renegade_emscripten_root_files)
        get_filename_component(_renegade_emscripten_root_ext "${_renegade_emscripten_root_file}" EXT)
        string(TOLOWER "${_renegade_emscripten_root_ext}" _renegade_emscripten_root_ext_lower)

        if(_renegade_emscripten_root_ext_lower MATCHES "^\\.(exe|dll|asi|m3d|bmp|ico|doc|xml|vdf)$")
            continue()
        endif()

        target_link_options("${target_name}" PRIVATE
            "SHELL:--preload-file ${RENEGADE_EMSCRIPTEN_DATA_ROOT}/${_renegade_emscripten_root_file}@/${_renegade_emscripten_root_file}"
        )
    endforeach()

    foreach(_renegade_emscripten_runtime_dialog_source IN ITEMS
        "Code/Commando/chat.rc"
        "Code/Commando/resource.h"
        "Code/Commando/dialogresource.h"
        "Code/Combat/string_ids.h")
        if(NOT EXISTS "${PROJECT_SOURCE_DIR}/${_renegade_emscripten_runtime_dialog_source}")
            message(FATAL_ERROR
                "Missing runtime dialog source dependency for Emscripten: "
                "${PROJECT_SOURCE_DIR}/${_renegade_emscripten_runtime_dialog_source}")
        endif()

        target_link_options("${target_name}" PRIVATE
            "SHELL:--preload-file ${PROJECT_SOURCE_DIR}/${_renegade_emscripten_runtime_dialog_source}@/${_renegade_emscripten_runtime_dialog_source}"
        )
    endforeach()
endfunction()
