# SPDX-License-Identifier: LicenseRef-TridentMSLA
# SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>

function(tr_combine_files input_file1 input_file2 output_file depends)
    # Create sanitized names for files using just the basename
    get_filename_component(output_basename ${output_file} NAME)
    string(REPLACE "." "_" sanitized_name "${output_basename}")

    set(script_path "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/tr_combine_files.sh")

    add_custom_target(
        ${sanitized_name}
        ALL
        COMMAND bash ${script_path} "${input_file1}" "${input_file2}" "${output_file}"
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        DEPENDS ${depends}
        COMMENT "Attempting to create ${output_basename}"
    )

endfunction()
