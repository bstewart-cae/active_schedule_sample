# SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
# SPDX-License-Identifier: LicenseRef-TridentMSLA

##
# @brief Generates a key pair (private and public) required for signing binaries.
#
# Required Inputs:
# @param[in] TSDK_CONFIG_PRIVATE_SIGNING_KEY_PATH   path/to/private_key.pem
# @param[in] TSDK_CONFIG_PUBLIC_SIGNING_KEY_PATH    path/to/public_key.der
#
function(tr_generate_signing_keys)
    message(DEBUG "####################################")
    message(DEBUG "Invoke generate_signing_keys()")

    if(NOT DEFINED TSDK_CONFIG_PRIVATE_SIGNING_KEY_PATH)
        message(FATAL_ERROR "TSDK_CONFIG_PRIVATE_SIGNING_KEY_PATH not defined")
    endif()

    if(NOT DEFINED TSDK_CONFIG_PUBLIC_SIGNING_KEY_PATH)
        message(FATAL_ERROR "TSDK_CONFIG_PUBLIC_SIGNING_KEY_PATH not defined")
    endif()

    get_filename_component(KEY_DIR ${TSDK_CONFIG_PUBLIC_SIGNING_KEY_PATH} DIRECTORY)
    get_filename_component(NAME_WE ${TSDK_CONFIG_PUBLIC_SIGNING_KEY_PATH} NAME_WE)  # Get filename without extension

    set(JSON_FILE "${KEY_DIR}/${NAME_WE}.json")

    if(NOT EXISTS ${TSDK_CONFIG_PRIVATE_SIGNING_KEY_PATH} OR NOT EXISTS ${TSDK_CONFIG_PUBLIC_SIGNING_KEY_PATH})
        # If one of the keys exist make sure to clean up.
        file(REMOVE ${TSDK_CONFIG_PRIVATE_SIGNING_KEY_PATH} ${TSDK_CONFIG_PUBLIC_SIGNING_KEY_PATH} ${JSON_FILE})

        # Make sure key formats are expected
        set(SUPPORTED_PRIV_EXTENSIONS ".pem")
        set(SUPPORTED_PUB_EXTENSIONS ".der")

        get_filename_component(PRIV_EXT ${TSDK_CONFIG_PRIVATE_SIGNING_KEY_PATH} EXT)
        get_filename_component(PUB_EXT ${TSDK_CONFIG_PUBLIC_SIGNING_KEY_PATH} EXT)

        if(NOT PRIV_EXT IN_LIST SUPPORTED_PRIV_EXTENSIONS)
            message(FATAL_ERROR "\nInvalid TSDK_CONFIG_PRIVATE_SIGNING_KEY_PATH file extension, use one of: ${SUPPORTED_PRIV_EXTENSIONS}\n")
        endif()

        if(NOT PUB_EXT IN_LIST SUPPORTED_PUB_EXTENSIONS)
            message(FATAL_ERROR "\nInvalid TSDK_CONFIG_PUBLIC_SIGNING_KEY_PATH file extension, use one of: ${SUPPORTED_PUB_EXTENSIONS}\n")
        endif()

        # Generate keys in default location.
        file(MAKE_DIRECTORY ${KEY_DIR})
        message(DEBUG "Generate private key...")
        execute_process(
            COMMAND openssl ecparam -genkey -name prime256v1 -noout -out ${TSDK_CONFIG_PRIVATE_SIGNING_KEY_PATH}
        )
        message(DEBUG "Generate public key...")
        execute_process(
            COMMAND openssl ec -in ${TSDK_CONFIG_PRIVATE_SIGNING_KEY_PATH} --outform DER  -pubout  -out ${TSDK_CONFIG_PUBLIC_SIGNING_KEY_PATH}
        )
    endif()

    add_custom_target(signing_keys)
    set_target_properties(signing_keys
        PROPERTIES
        private_key_path ${TSDK_CONFIG_PRIVATE_SIGNING_KEY_PATH}
        public_key_path ${TSDK_CONFIG_PUBLIC_SIGNING_KEY_PATH}
    )
    message(STATUS "Using private key from ${TSDK_CONFIG_PRIVATE_SIGNING_KEY_PATH}")
    message(STATUS "Using public key from ${TSDK_CONFIG_PUBLIC_SIGNING_KEY_PATH}")

    # NOTE: assumes python script and cmake function files live at the same place
    if(NOT EXISTS ${JSON_FILE})
        get_target_property(_public_key signing_keys public_key_path)
        message(DEBUG "Generates public key tokens JSON file ${JSON_FILE}...")
        set(PYTHON_COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_FUNCTION_LIST_DIR}/public_key_json.py -i ${_public_key} -o ${JSON_FILE})
        message(DEBUG "PYTHON_COMMAND: ${PYTHON_COMMAND}")
        execute_process(
            COMMAND ${PYTHON_COMMAND}
        )
    endif()
    message(DEBUG "####################################")
endfunction()
