add_executable(renegade_bootstrap
    sdl_bootstrap_main.cpp
)

renegade_configure_target(renegade_bootstrap)
target_link_libraries(renegade_bootstrap PRIVATE SDL3::SDL3)

if(TARGET SDL3::SDL3-shared)
    add_custom_command(TARGET renegade_bootstrap POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<TARGET_FILE:SDL3::SDL3-shared>"
            "$<TARGET_FILE_DIR:renegade_bootstrap>"
        VERBATIM
    )
endif()
