function(vanta_copy_directory target source_dir dest_subdir)
    add_custom_command(TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/${source_dir}
            $<TARGET_FILE_DIR:${target}>/${dest_subdir}
            COMMENT "Copying ${source_dir} to ${target} at ${dest_subdir}"
            VERBATIM
    )
endfunction()

function(vanta_copy_directory_absolute target source_dir dest_subdir)
    add_custom_command(TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${source_dir}"
            "$<TARGET_FILE_DIR:${target}>/${dest_subdir}"
            COMMENT "Copying ${source_dir} to ${target} at ${dest_subdir}"
            VERBATIM
    )
endfunction()