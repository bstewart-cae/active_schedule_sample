# SPDX-License-Identifier: LicenseRef-TridentMSLA
# SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
include(CMakeParseArguments)

function(tr_generate_token_json)
    # Define the accepted arguments
    set(options "")
    set(oneValueArgs
        TYPE          # APP or MFG
        CHIP          # Target chip platform
        FLASH_SIZE    # Target platform flash size
        TOKEN_PATH    # Path to token directory
        PLATFORM_PATH # Path to platform directory
        HEADER_FILE   # Path to token header file (only for APP tokens)
        OUTPUT_DIR    # Output directory for JSON file
    )
    set(multiValueArgs "")

    # Parse the arguments
    cmake_parse_arguments(TOKEN_GEN
        "${options}"
        "${oneValueArgs}"
        "${multiValueArgs}"
        ${ARGN}
    )

    # Validate required arguments
    if(NOT TOKEN_GEN_TYPE)
        message(FATAL_ERROR "TOKEN_GEN: TYPE argument is required (APP or MFG)")
    endif()

    if(NOT TOKEN_GEN_CHIP)
        message(FATAL_ERROR "TOKEN_GEN: CHIP argument is required")
    endif()

    if(NOT TOKEN_GEN_FLASH_SIZE)
        message(FATAL_ERROR "TOKEN_GEN: FLASH_SIZE argument is required")
    endif()

    if(NOT TOKEN_GEN_TOKEN_PATH)
        message(FATAL_ERROR "TOKEN_GEN: TOKEN_PATH argument is required")
    endif()

    if(NOT TOKEN_GEN_PLATFORM_PATH)
        message(FATAL_ERROR "TOKEN_GEN: PLATFORM_PATH argument is required")
    endif()

    if(NOT TOKEN_GEN_OUTPUT_DIR)
        message(FATAL_ERROR "TOKEN_GEN: OUTPUT_DIR argument is required")
    endif()

    # Validate type argument
    if(NOT TOKEN_GEN_TYPE STREQUAL "APP" AND NOT TOKEN_GEN_TYPE STREQUAL "MFG")
        message(FATAL_ERROR "TOKEN_GEN: TYPE must be either 'APP' or 'MFG'")
    endif()

    # For APP tokens, validate header file is provided
    if(TOKEN_GEN_TYPE STREQUAL "APP" AND NOT TOKEN_GEN_HEADER_FILE)
        message(FATAL_ERROR "TOKEN_GEN: HEADER_FILE is required for APP token generation")
    endif()

    # Validate paths exist
    if(NOT EXISTS "${TOKEN_GEN_TOKEN_PATH}")
        message(FATAL_ERROR "TOKEN_GEN: TOKEN_PATH '${TOKEN_GEN_TOKEN_PATH}' does not exist")
    endif()

    if(NOT EXISTS "${TOKEN_GEN_PLATFORM_PATH}")
        message(FATAL_ERROR "TOKEN_GEN: PLATFORM_PATH '${TOKEN_GEN_PLATFORM_PATH}' does not exist")
    endif()

    if(NOT EXISTS "${TOKEN_GEN_OUTPUT_DIR}")
        message(FATAL_ERROR "TOKEN_GEN: OUTPUT_DIR '${TOKEN_GEN_OUTPUT_DIR}' does not exist")
    endif()

    # Construct the Python script path
    set(SCRIPT_PATH "${TOKEN_GEN_TOKEN_PATH}/generator/token_json_gen.py")
    if(NOT EXISTS "${SCRIPT_PATH}")
        message(FATAL_ERROR "TOKEN_GEN: token_json_gen.py not found at '${SCRIPT_PATH}'")
    endif()

    # Construct the command
    set(CMD_ARGS
        -t "${TOKEN_GEN_TYPE}"
        -c "${TOKEN_GEN_CHIP}"
        -s "${TOKEN_GEN_FLASH_SIZE}"
        -u "${TOKEN_GEN_TOKEN_PATH}"
        -p "${TOKEN_GEN_PLATFORM_PATH}"
        -o "${TOKEN_GEN_OUTPUT_DIR}"
    )

    # Add header file argument for APP tokens
    if(TOKEN_GEN_TYPE STREQUAL "APP")
        if(NOT EXISTS "${TOKEN_GEN_HEADER_FILE}")
            message(FATAL_ERROR "TOKEN_GEN: HEADER_FILE '${TOKEN_GEN_HEADER_FILE}' does not exist")
        endif()
        list(APPEND CMD_ARGS -f "${TOKEN_GEN_HEADER_FILE}")
    endif()

    # Calculate output JSON file path for dependency tracking
    set(JSON_OUTPUT "${TOKEN_GEN_OUTPUT_DIR}/${TOKEN_GEN_CHIP}${TOKEN_GEN_FLASH_SIZE}_${TOKEN_GEN_TYPE}_token_def.json")

    add_custom_target(generate_${TOKEN_GEN_TYPE}_tokens_${TOKEN_GEN_CHIP}${TOKEN_GEN_FLASH_SIZE}
        ALL
        COMMAND ${CMAKE_COMMAND} -E env MAKEFLAGS=-j ${Python3_EXECUTABLE} ${SCRIPT_PATH} ${CMD_ARGS}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Generating ${TOKEN_GEN_TYPE} token definitions for ${TOKEN_GEN_CHIP}${TOKEN_GEN_FLASH_SIZE}"
    )

endfunction()
