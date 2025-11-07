# SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
# SPDX-License-Identifier: LicenseRef-TridentMSLA

#
# Generates signing and encryption keys
#
# This function will generate a set of (temporary) private and public keys for signing, and
# a key and an initialization vector used for encryption.
#
# The keys will be located in ${CMAKE_BINARY_DIR}/temp_keys/.
#
# The generation will be skipped if:
# - the following configuration parameters are set, or
# - the generated keys/files already exist.
#
# The function supports five configuration parameters:
# 1. ZWSDK_CONFIG_PRIVATE_ROOT_SIGNING_KEY_PATH
# 2. ZWSDK_CONFIG_PUBLIC_ROOT_SIGNING_KEY_PATH
# 3. ZWSDK_CONFIG_PRIVATE_SIGNING_KEY_PATH
# 4. ZWSDK_CONFIG_PUBLIC_SIGNING_KEY_PATH
# 5. ZWSDK_CONFIG_ENCRYPTION_KEY_PATH
#
function(zwsdk_generate_signing_and_encryption_keys)
  if(DEFINED ZWSDK_CONFIG_PRIVATE_ROOT_SIGNING_KEY_PATH)
    if(NOT EXISTS ${ZWSDK_CONFIG_PRIVATE_ROOT_SIGNING_KEY_PATH})
      message(FATAL_ERROR "The private key set by ZWSDK_CONFIG_PRIVATE_ROOT_SIGNING_KEY_PATH was not found!")
    endif()
    set(CUSTOM_ROOT_SIGNING_KEY_PRIVATE_IS_SET "")
  endif()

  if(DEFINED ZWSDK_CONFIG_PUBLIC_ROOT_SIGNING_KEY_PATH)
    if(NOT EXISTS ${ZWSDK_CONFIG_PUBLIC_ROOT_SIGNING_KEY_PATH})
      message(FATAL_ERROR "The public key set by ZWSDK_CONFIG_PUBLIC_ROOT_SIGNING_KEY_PATH was not found!")
    endif()
    set(CUSTOM_ROOT_SIGNING_KEY_PUBLIC_IS_SET "")
  endif()

  if(DEFINED ZWSDK_CONFIG_PRIVATE_SIGNING_KEY_PATH)
    if(NOT EXISTS ${ZWSDK_CONFIG_PRIVATE_SIGNING_KEY_PATH})
      message(FATAL_ERROR "The private key set by ZWSDK_CONFIG_PRIVATE_SIGNING_KEY_PATH was not found!")
    endif()
    set(CUSTOM_SIGNING_KEY_PRIVATE_IS_SET "")
  endif()

  if(DEFINED ZWSDK_CONFIG_PUBLIC_SIGNING_KEY_PATH)
    if(NOT EXISTS ${ZWSDK_CONFIG_PUBLIC_SIGNING_KEY_PATH})
      message(FATAL_ERROR "The public key set by ZWSDK_CONFIG_PUBLIC_SIGNING_KEY_PATH was not found!")
    endif()
    set(CUSTOM_SIGNING_KEY_PUBLIC_IS_SET "")
  endif()

  if(DEFINED ZWSDK_CONFIG_ENCRYPTION_KEY_PATH)
    if(NOT EXISTS ${ZWSDK_CONFIG_ENCRYPTION_KEY_PATH})
      message(FATAL_ERROR "The encryption key set by ZWSDK_CONFIG_ENCRYPTION_KEY_PATH was not found!")
    endif()
    set(CUSTOM_ENCRYPTION_KEY_IS_SET "")
  endif()

  # Define a temporary location for keys if not supplied by configuration.
  set(TEMP_KEYS_DIR             ${CMAKE_BINARY_DIR}/temp_keys)

  # Always generate the temp keys directory for the IV.
  file(MAKE_DIRECTORY ${TEMP_KEYS_DIR})

  # Always use the temp IV because it must be unique for each release.
  # A fresh build (build/ newly created) will give a unique iv.
  set(TEMP_AES_ENC_IV_PATH  ${TEMP_KEYS_DIR}/temp_aes_iv.hex)

  # Always generate a random initialization vector (IV).
  # The IV MUST be unique for every firmware update image.
  message(STATUS "Generating temporary AES initialization vector in txt/hex format...")
  execute_process(
    COMMAND openssl rand -hex -out ${TEMP_AES_ENC_IV_PATH} 16
  )
  message(STATUS "Generating temporary AES initialization vector in txt/hex format - done")

  if(DEFINED CUSTOM_ROOT_SIGNING_KEY_PRIVATE_IS_SET AND
     DEFINED CUSTOM_ROOT_SIGNING_KEY_PUBLIC_IS_SET AND
     DEFINED CUSTOM_SIGNING_KEY_PRIVATE_IS_SET AND
     DEFINED CUSTOM_SIGNING_KEY_PUBLIC_IS_SET AND
     DEFINED CUSTOM_ENCRYPTION_KEY_IS_SET)
    # Custom keys exist. Use them.
    set(ROOT_PRIVATE_KEY_PEM_FILE "${ZWSDK_CONFIG_PRIVATE_ROOT_SIGNING_KEY_PATH}")
    set(ROOT_PUBLIC_KEY_DER_FILE  "${ZWSDK_CONFIG_PUBLIC_ROOT_SIGNING_KEY_PATH}")
    set(PRIVATE_KEY_PEM_FILE "${ZWSDK_CONFIG_PRIVATE_SIGNING_KEY_PATH}")
    set(PUBLIC_KEY_DER_FILE  "${ZWSDK_CONFIG_PUBLIC_SIGNING_KEY_PATH}")
    set(ENCRYPTION_KEY_FILE "${ZWSDK_CONFIG_ENCRYPTION_KEY_PATH}")

    # Create the JSON file in the same location as the public encryption key.
    get_filename_component(DIR ${ZWSDK_CONFIG_PUBLIC_SIGNING_KEY_PATH} DIRECTORY)
    set(JSON_FILE "${DIR}/keys.json")
  else()
    # Root signing keys
    set(ROOT_PRIVATE_KEY_PEM_FILE  ${TEMP_KEYS_DIR}/temp_root_private_key.pem)
    set(ROOT_PUBLIC_KEY_PEM_FILE   ${TEMP_KEYS_DIR}/temp_root_public_key.pem)
    set(ROOT_PUBLIC_KEY_DER_FILE   ${TEMP_KEYS_DIR}/temp_root_public_key.der)

    # Bootloader + app signing keys
    set(PRIVATE_KEY_PEM_FILE  ${TEMP_KEYS_DIR}/temp_private_key.pem)
    set(PUBLIC_KEY_PEM_FILE   ${TEMP_KEYS_DIR}/temp_public_key.pem)
    set(PUBLIC_KEY_DER_FILE   ${TEMP_KEYS_DIR}/temp_public_key.der)

    set(ENCRYPTION_KEY_FILE   ${TEMP_KEYS_DIR}/temp_aes_key.hex)
    set(JSON_FILE             ${TEMP_KEYS_DIR}/temp_keys.json)

    set(FILE_SET
      ${ROOT_PRIVATE_KEY_PEM_FILE}
      ${ROOT_PUBLIC_KEY_PEM_FILE}
      ${ROOT_PUBLIC_KEY_DER_FILE}
      ${PRIVATE_KEY_PEM_FILE}
      ${PUBLIC_KEY_PEM_FILE}
      ${PUBLIC_KEY_DER_FILE}
      ${ENCRYPTION_KEY_FILE}
      ${JSON_FILE}
    )
    list(LENGTH FILE_SET FILE_SET_LENGTH)

    set(COUNTER 0)
    foreach(FILE IN LISTS FILE_SET)
      if(NOT EXISTS ${FILE})
        math(EXPR COUNTER "${COUNTER} + 1")
      endif()
    endforeach()

    if(COUNTER EQUAL 0)
      message(STATUS "Using existing temporary signing and encryption keys.")
    elseif(COUNTER EQUAL FILE_SET_LENGTH)
      # No temporary files were found, generate.

      #####################
      # Root signing keys #
      #####################

      #
      # Generate private signing key
      #
      message(STATUS "Generating temporary private signing key in PEM format...")
      execute_process(
        COMMAND openssl ecparam -genkey -name prime256v1 -noout -out ${ROOT_PRIVATE_KEY_PEM_FILE}
        RESULT_VARIABLE ROOT_PRIVATE_KEY_GEN_EXIT_CODE
        OUTPUT_VARIABLE ROOT_PRIVATE_KEY_GEN_STDOUT
        ERROR_VARIABLE ROOT_PRIVATE_KEY_GEN_STDERR
        OUTPUT_QUIET
        ERROR_QUIET
      )
      if(NOT ${ROOT_PRIVATE_KEY_GEN_STDOUT} STREQUAL "")
        message(VERBOSE "${ROOT_PRIVATE_KEY_GEN_STDOUT}")
      endif()
      if(${ROOT_PRIVATE_KEY_GEN_EXIT_CODE})
        message(NOTICE "${ROOT_PRIVATE_KEY_GEN_STDERR}")
        message(FATAL_ERROR "Failed to generate private signing key in PEM format")
      endif()
      message(STATUS "Generating temporary private signing key in PEM format - done")

      #
      # Generate public signing key (PEM)
      #
      message(STATUS "Generating temporary public signing key in PEM format...")
      execute_process(
        COMMAND openssl ec -inform PEM -in ${ROOT_PRIVATE_KEY_PEM_FILE} --outform PEM  -pubout  -out ${ROOT_PUBLIC_KEY_PEM_FILE}
        RESULT_VARIABLE ROOT_PUBLIC_KEY_PEM_GEN_EXIT_CODE
        OUTPUT_VARIABLE ROOT_PUBLIC_KEY_PEM_GEN_STDOUT
        ERROR_VARIABLE ROOT_PUBLIC_KEY_PEM_GEN_STDERR
        OUTPUT_QUIET
        ERROR_QUIET
      )
      if(NOT ${ROOT_PUBLIC_KEY_PEM_GEN_STDOUT} STREQUAL "")
        message(VERBOSE "${ROOT_PUBLIC_KEY_PEM_GEN_STDOUT}")
      endif()
      if(${ROOT_PUBLIC_KEY_PEM_GEN_EXIT_CODE})
        message(NOTICE "STDERR: ${ROOT_PUBLIC_KEY_PEM_GEN_STDERR}")
        message(FATAL_ERROR "Failed to generate public signing key in PEM format")
      endif()
      message(STATUS "Generating temporary public signing key in PEM format - done")

      #
      # Generate public signing key (DER)
      #
      message(STATUS "Generating temporary public signing key in DER format...")
      execute_process(
        COMMAND openssl ec -inform PEM -in ${ROOT_PRIVATE_KEY_PEM_FILE} --outform DER  -pubout  -out ${ROOT_PUBLIC_KEY_DER_FILE}
        RESULT_VARIABLE ROOT_PUBLIC_KEY_DER_GEN_EXIT_CODE
        OUTPUT_VARIABLE ROOT_PUBLIC_KEY_DER_GEN_STDOUT
        ERROR_VARIABLE ROOT_PUBLIC_KEY_DER_GEN_STDERR
        OUTPUT_QUIET
        ERROR_QUIET
      )
      if(NOT ${ROOT_PUBLIC_KEY_DER_GEN_STDOUT} STREQUAL "")
        message(VERBOSE "${ROOT_PUBLIC_KEY_DER_GEN_STDOUT}")
      endif()
      if(${ROOT_PUBLIC_KEY_DER_GEN_EXIT_CODE})
        message(NOTICE "STDERR: ${ROOT_PUBLIC_KEY_DER_GEN_STDERR}")
        message(FATAL_ERROR "Failed to generate public signing key in DER format")
      endif()
      message(STATUS "Generating temporary public signing key in DER format - done")

      #################################
      # Bootloader + app signing keys #
      #################################

      #
      # Generate private signing key
      #
      message(STATUS "Generating temporary private signing key in PEM format...")
      execute_process(
        COMMAND openssl ecparam -genkey -name prime256v1 -noout -out ${PRIVATE_KEY_PEM_FILE}
        RESULT_VARIABLE PRIVATE_KEY_GEN_EXIT_CODE
        OUTPUT_VARIABLE PRIVATE_KEY_GEN_STDOUT
        ERROR_VARIABLE PRIVATE_KEY_GEN_STDERR
        OUTPUT_QUIET
        ERROR_QUIET
      )
      if(NOT ${PRIVATE_KEY_GEN_STDOUT} STREQUAL "")
        message(VERBOSE "${PRIVATE_KEY_GEN_STDOUT}")
      endif()
      if(${PRIVATE_KEY_GEN_EXIT_CODE})
        message(NOTICE "${PRIVATE_KEY_GEN_STDERR}")
        message(FATAL_ERROR "Failed to generate private signing key in PEM format")
      endif()
      message(STATUS "Generating temporary private signing key in PEM format - done")

      #
      # Generate public signing key (PEM)
      #
      message(STATUS "Generating temporary public signing key in PEM format...")
      execute_process(
        COMMAND openssl ec -inform PEM -in ${PRIVATE_KEY_PEM_FILE} --outform PEM  -pubout  -out ${PUBLIC_KEY_PEM_FILE}
        RESULT_VARIABLE PUBLIC_KEY_PEM_GEN_EXIT_CODE
        OUTPUT_VARIABLE PUBLIC_KEY_PEM_GEN_STDOUT
        ERROR_VARIABLE PUBLIC_KEY_PEM_GEN_STDERR
        OUTPUT_QUIET
        ERROR_QUIET
      )
      if(NOT ${PUBLIC_KEY_PEM_GEN_STDOUT} STREQUAL "")
        message(VERBOSE "${PUBLIC_KEY_PEM_GEN_STDOUT}")
      endif()
      if(${PUBLIC_KEY_PEM_GEN_EXIT_CODE})
        message(NOTICE "STDERR: ${PUBLIC_KEY_PEM_GEN_STDERR}")
        message(FATAL_ERROR "Failed to generate public signing key in PEM format")
      endif()
      message(STATUS "Generating temporary public signing key in PEM format - done")

      #
      # Generate public signing key (DER)
      #
      message(STATUS "Generating temporary public signing key in DER format...")
      execute_process(
        COMMAND openssl ec -inform PEM -in ${PRIVATE_KEY_PEM_FILE} --outform DER  -pubout  -out ${PUBLIC_KEY_DER_FILE}
        RESULT_VARIABLE PUBLIC_KEY_DER_GEN_EXIT_CODE
        OUTPUT_VARIABLE PUBLIC_KEY_DER_GEN_STDOUT
        ERROR_VARIABLE PUBLIC_KEY_DER_GEN_STDERR
        OUTPUT_QUIET
        ERROR_QUIET
      )
      if(NOT ${PUBLIC_KEY_DER_GEN_STDOUT} STREQUAL "")
        message(VERBOSE "${PUBLIC_KEY_DER_GEN_STDOUT}")
      endif()
      if(${PUBLIC_KEY_DER_GEN_EXIT_CODE})
        message(NOTICE "STDERR: ${PUBLIC_KEY_DER_GEN_STDERR}")
        message(FATAL_ERROR "Failed to generate public signing key in DER format")
      endif()
      message(STATUS "Generating temporary public signing key in DER format - done")

      #
      # Generate encryption key
      #
      message(STATUS "Generating temporary AES encryption key in txt/hex format...")
      execute_process(
        COMMAND openssl rand -hex -out ${ENCRYPTION_KEY_FILE} 16
      )
      message(STATUS "Generating temporary AES encryption key in txt/hex format - done")
    else()
      message(FATAL_ERROR "One or more auto-generated temporary key files are missing. Delete all files in ${TEMP_KEYS_DIR} and run CMake again.")
    endif()
  endif()

  # Create a custom target that can hold the key files.
  add_custom_target(signing_keys)
  set_target_properties(signing_keys
    PROPERTIES
      private_root_key_path  ${ROOT_PRIVATE_KEY_PEM_FILE}
      public_root_key_path   ${ROOT_PUBLIC_KEY_DER_FILE}
      private_key_path       ${PRIVATE_KEY_PEM_FILE}
      public_key_path        ${PUBLIC_KEY_DER_FILE}
      aes_symetrical_key     ${ENCRYPTION_KEY_FILE}
      aes_initial_vector     ${TEMP_AES_ENC_IV_PATH}
  )

  message(STATUS "Using private root signing key from ${ROOT_PRIVATE_KEY_PEM_FILE}")
  message(STATUS "Using public root signing key from ${ROOT_PUBLIC_KEY_DER_FILE}")
  message(STATUS "Using private signing key from ${PRIVATE_KEY_PEM_FILE}")
  message(STATUS "Using public signing key from ${PUBLIC_KEY_DER_FILE}")
  message(STATUS "Using encryption key from ${ENCRYPTION_KEY_FILE}")
  message(STATUS "Using initialization vector from ${TEMP_AES_ENC_IV_PATH}")

  # Generate a JSON file if it doesn't exist.
  if(NOT EXISTS ${JSON_FILE})
    get_target_property(_public_key signing_keys public_key_path)
    get_target_property(_encryption_key signing_keys aes_symetrical_key)
    message(STATUS "Generating JSON file ${JSON_FILE} with keys for elcap...")
    set(PYTHON_COMMAND ${Python3_EXECUTABLE} ${TRIDENT_SDK_ROOT}/tools/cmake/public_key_json.py -i ${_public_key} --encryption-key ${_encryption_key} -o ${JSON_FILE})
    execute_process(
      COMMAND ${PYTHON_COMMAND}
      RESULT_VARIABLE PYTHON_COMMAND_RESULT
      OUTPUT_VARIABLE JSON_GEN_STDOUT
      ERROR_VARIABLE JSON_GEN_STDERR
      OUTPUT_QUIET
      ERROR_QUIET
    )
    if(NOT ${JSON_GEN_STDOUT} STREQUAL "")
      message(VERBOSE "${JSON_GEN_STDOUT}")
    endif()
    if (${PYTHON_COMMAND_RESULT})
      message(NOTICE "STDERR: ${JSON_GEN_STDERR}")
      message(FATAL_ERROR "Failed to generate ${JSON_FILE}")
    endif()
    message(STATUS "Generating JSON file ${JSON_FILE} with keys for elcap - done")
  endif()
  set_target_properties(signing_keys
    PROPERTIES
      keys_json_file ${JSON_FILE}
  )
  message(DEBUG "####################################")
endfunction()
