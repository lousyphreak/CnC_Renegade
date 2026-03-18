include_guard(GLOBAL)

add_library(renegade_project_options INTERFACE)
add_library(renegade::project_options ALIAS renegade_project_options)

target_compile_features(renegade_project_options INTERFACE cxx_std_17)
target_include_directories(renegade_project_options
    INTERFACE
        "${PROJECT_SOURCE_DIR}/Code"
        "${PROJECT_BINARY_DIR}/generated"
)

if(MSVC)
    target_compile_options(renegade_project_options INTERFACE /W4 /permissive- /EHsc /bigobj)
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
