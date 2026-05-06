include_guard(GLOBAL)

function(_renegade_bgfx_get_profile_path_ext profile out_var)
    set(_profile "${profile}")
    string(REPLACE "100_es" "essl" _profile "${_profile}")
    string(REPLACE "300_es" "essl" _profile "${_profile}")
    string(REPLACE "120" "glsl" _profile "${_profile}")
    string(REPLACE "430" "glsl" _profile "${_profile}")
    string(REPLACE "s_5_0" "dxbc" _profile "${_profile}")
    string(REPLACE "s_6_0" "dxil" _profile "${_profile}")
    set(${out_var} "${_profile}" PARENT_SCOPE)
endfunction()

function(_renegade_bgfx_get_profile_ext profile out_var)
    set(_profile "${profile}")
    string(REPLACE "100_es" "essl" _profile "${_profile}")
    string(REPLACE "300_es" "essl" _profile "${_profile}")
    string(REPLACE "120" "glsl" _profile "${_profile}")
    string(REPLACE "430" "glsl" _profile "${_profile}")
    string(REPLACE "spirv" "spv" _profile "${_profile}")
    string(REPLACE "metal" "mtl" _profile "${_profile}")
    string(REPLACE "s_5_0" "dxbc" _profile "${_profile}")
    string(REPLACE "s_6_0" "dxil" _profile "${_profile}")
    set(${out_var} "${_profile}" PARENT_SCOPE)
endfunction()

function(_renegade_bgfx_get_platform_and_profiles shader_type out_platform out_profiles)
    if(IOS)
        set(_platform ios)
        if(shader_type STREQUAL "COMPUTE")
            set(_profiles 430)
        else()
            set(_profiles 120)
        endif()
        list(APPEND _profiles metal)
    elseif(ANDROID)
        set(_platform android)
        set(_profiles spirv)
        if(shader_type STREQUAL "COMPUTE")
            list(APPEND _profiles 300_es)
        else()
            list(APPEND _profiles 100_es)
        endif()
    elseif(EMSCRIPTEN)
        set(_platform asm.js)
        if(shader_type STREQUAL "COMPUTE")
            if(BGFX_CONFIG_RENDERER_WEBGPU)
                set(_profiles wgsl)
            else()
                message(FATAL_ERROR "shaderc: Emscripten compute shaders require BGFX_CONFIG_RENDERER_WEBGPU.")
            endif()
        else()
            set(_profiles 100_es)
            if(BGFX_CONFIG_RENDERER_WEBGPU)
                list(APPEND _profiles wgsl)
            endif()
        endif()
    elseif(UNIX AND NOT APPLE)
        set(_platform linux)
        set(_profiles spirv)
        if(shader_type STREQUAL "COMPUTE")
            list(APPEND _profiles 430)
        else()
            list(APPEND _profiles 120)
        endif()
    elseif(APPLE)
        set(_platform osx)
        set(_profiles spirv)
        if(shader_type STREQUAL "COMPUTE")
            list(APPEND _profiles 430)
        else()
            list(APPEND _profiles 120)
        endif()
        list(APPEND _profiles metal)
    elseif(WIN32 OR MINGW OR MSYS OR CYGWIN)
        set(_platform windows)
        set(_profiles spirv)
        if(shader_type STREQUAL "COMPUTE")
            list(APPEND _profiles 430)
        else()
            list(APPEND _profiles 120)
        endif()
        list(APPEND _profiles s_5_0 s_6_0)
    elseif(ORBIS)
        set(_platform orbis)
        set(_profiles spirv)
        if(shader_type STREQUAL "COMPUTE")
            list(APPEND _profiles 430)
        else()
            list(APPEND _profiles 120)
        endif()
        list(APPEND _profiles pssl)
    else()
        message(FATAL_ERROR "shaderc: Unsupported platform")
    endif()

    set(${out_platform} "${_platform}" PARENT_SCOPE)
    set(${out_profiles} "${_profiles}" PARENT_SCOPE)
endfunction()

function(renegade_bgfx_compile_shaders)
    set(options AS_HEADERS)
    set(oneValueArgs TYPE VARYING_DEF OUTPUT_DIR OUT_FILES_VAR)
    set(multiValueArgs SHADERS INCLUDE_DIRS DEFINES DEPENDS)
    cmake_parse_arguments(ARGS "${options}" "${oneValueArgs}" "${multiValueArgs}" "${ARGN}")

    if(NOT TARGET bgfx::shaderc)
        message(FATAL_ERROR "renegade_bgfx_compile_shaders requires the bgfx::shaderc target.")
    endif()

    if(NOT ARGS_TYPE)
        message(FATAL_ERROR "renegade_bgfx_compile_shaders requires TYPE.")
    endif()
    if(NOT ARGS_OUTPUT_DIR)
        message(FATAL_ERROR "renegade_bgfx_compile_shaders requires OUTPUT_DIR.")
    endif()
    if(NOT ARGS_VARYING_DEF)
        message(FATAL_ERROR "renegade_bgfx_compile_shaders requires VARYING_DEF.")
    endif()

    string(TOUPPER "${ARGS_TYPE}" _renegade_bgfx_shader_type_upper)
    if(NOT _renegade_bgfx_shader_type_upper MATCHES "^(VERTEX|FRAGMENT|COMPUTE)$")
        message(FATAL_ERROR "Unsupported shader type '${ARGS_TYPE}'.")
    endif()
    string(TOLOWER "${_renegade_bgfx_shader_type_upper}" _renegade_bgfx_shader_type_lower)

    _renegade_bgfx_get_platform_and_profiles(
        "${_renegade_bgfx_shader_type_upper}"
        _renegade_bgfx_platform
        _renegade_bgfx_profiles
    )

    set(_renegade_bgfx_define_args)
    if(ARGS_DEFINES)
        string(JOIN ";" _renegade_bgfx_defines_joined ${ARGS_DEFINES})
        string(REPLACE ";" "\;" _renegade_bgfx_defines_arg "${_renegade_bgfx_defines_joined}")
        list(APPEND _renegade_bgfx_define_args --define "${_renegade_bgfx_defines_arg}")
    endif()

    set(_renegade_bgfx_include_args)
    foreach(_renegade_bgfx_include_dir IN LISTS ARGS_INCLUDE_DIRS)
        list(APPEND _renegade_bgfx_include_args -i "${_renegade_bgfx_include_dir}")
    endforeach()

    set(_renegade_bgfx_all_outputs)
    foreach(_renegade_bgfx_shader_file IN LISTS ARGS_SHADERS)
        get_filename_component(_renegade_bgfx_shader_basename "${_renegade_bgfx_shader_file}" NAME)
        get_filename_component(_renegade_bgfx_shader_name_we "${_renegade_bgfx_shader_file}" NAME_WE)
        get_filename_component(_renegade_bgfx_shader_absolute "${_renegade_bgfx_shader_file}" ABSOLUTE)

        foreach(_renegade_bgfx_profile IN LISTS _renegade_bgfx_profiles)
            _renegade_bgfx_get_profile_path_ext("${_renegade_bgfx_profile}" _renegade_bgfx_profile_path_ext)
            set(_renegade_bgfx_header_suffix "")
            set(_renegade_bgfx_bin2c_args)
            if(ARGS_AS_HEADERS)
                _renegade_bgfx_get_profile_ext("${_renegade_bgfx_profile}" _renegade_bgfx_profile_ext)
                set(_renegade_bgfx_header_suffix ".h")
                list(APPEND _renegade_bgfx_bin2c_args --bin2c "${_renegade_bgfx_shader_name_we}_${_renegade_bgfx_profile_ext}")
            endif()

            set(_renegade_bgfx_output "${ARGS_OUTPUT_DIR}/${_renegade_bgfx_profile_path_ext}/${_renegade_bgfx_shader_basename}.bin${_renegade_bgfx_header_suffix}")
            set(_renegade_bgfx_depfile "${_renegade_bgfx_output}.d")
            list(APPEND _renegade_bgfx_all_outputs "${_renegade_bgfx_output}")

            add_custom_command(
                OUTPUT "${_renegade_bgfx_output}"
                BYPRODUCTS "${_renegade_bgfx_depfile}"
                COMMAND ${CMAKE_COMMAND} -E make_directory "${ARGS_OUTPUT_DIR}/${_renegade_bgfx_profile_path_ext}"
                COMMAND bgfx::shaderc
                    --depends
                    -f "${_renegade_bgfx_shader_absolute}"
                    -o "${_renegade_bgfx_output}"
                    --type "${_renegade_bgfx_shader_type_lower}"
                    --platform "${_renegade_bgfx_platform}"
                    -p "${_renegade_bgfx_profile}"
                    -O "$<IF:$<CONFIG:Debug>,0,3>"
                    --varyingdef "${ARGS_VARYING_DEF}"
                    --Werror
                    "$<$<CONFIG:Debug,RelWithDebInfo>:--debug>"
                    ${_renegade_bgfx_bin2c_args}
                    ${_renegade_bgfx_include_args}
                    ${_renegade_bgfx_define_args}
                MAIN_DEPENDENCY "${_renegade_bgfx_shader_absolute}"
                DEPENDS "${ARGS_VARYING_DEF}" ${ARGS_DEPENDS}
                DEPFILE "${_renegade_bgfx_depfile}"
                COMMAND_EXPAND_LISTS
                VERBATIM
            )
        endforeach()
    endforeach()

    if(DEFINED ARGS_OUT_FILES_VAR)
        set(${ARGS_OUT_FILES_VAR} ${_renegade_bgfx_all_outputs} PARENT_SCOPE)
    endif()
endfunction()
