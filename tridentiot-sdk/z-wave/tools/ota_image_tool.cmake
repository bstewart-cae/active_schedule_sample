# SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
# SPDX-License-Identifier: LicenseRef-TridentMSLA

#
# Creates a target that signs the bootloader image.
#
# The signed bootloader will be used for combining bootloader and application
# into one binary.
#
function(zwsdk_create_signed_bootloader_target)
  get_target_property(ROOT_PRIVATE_KEY_PATH signing_keys private_root_key_path)

  get_target_property(PRIVATE_KEY_PATH signing_keys private_key_path)
  get_target_property(PUBLIC_KEY_PATH signing_keys public_key_path)

  set(BTL_SIGN_CMD
    ${Python3_EXECUTABLE} ${ZW_SDK_ROOT}/tools/bootloader_image_signer.py
     --bootloader ${BOOTLOADER_FILE}
     --root-priv ${ROOT_PRIVATE_KEY_PATH}
     --btl-priv ${PRIVATE_KEY_PATH}
     --btl-pub ${PUBLIC_KEY_PATH}
     --out ${CMAKE_BINARY_DIR}/bootloader_signed.bin
     --verbose
  )

  add_custom_target(bootloader_signed
    DEPENDS ${ZW_SDK_ROOT}/tools/bootloader_image_signer.py
    COMMAND ${BTL_SIGN_CMD}
    COMMENT "Signing bootloader."
  )
endfunction()

function(zw_create_mp_hex_target)

  add_custom_target(mp_page_hex
    COMMAND COMMAND ${CMAKE_OBJCOPY} -I binary -O ihex --change-addresses 0x100FF000 ${TRISDK_PATH}/Tools/Mass_Production/RT584_MP_SECTOR_Calibration_Data.bin ${CMAKE_BINARY_DIR}/default_mp_data.hex
    COMMENT "Generating MP data hex file"
  )
endfunction()

# This section contains common functions for CMakeLists.txt files.

##
# \skipline function\(add_executable\)
# \skipline function\(_add_executable\)
# \skipline function\(safeguard_add_executable\)

##
# @defgroup custom_func Custom CMake Functions

##
# @addtogroup custom_func
#
#
# @{
#
# @details Custom CMake Function description for build system
#

##
# @b Syntax
#
# &emsp; @c  elf_compress(\b ELF_FILENAME )
#
# Function for compressing the elf file using lzma algo
#
# @param[in] ELF_FILENAME  name of elf file to be compressed
#
function(zwsdk_generate_fw_update_image ELF_FILENAME)
  get_target_property(PRIVATE_KEY_PATH signing_keys private_key_path)
  get_target_property(AES_SYMETRICAL_KEY signing_keys aes_symetrical_key)
  get_target_property(AES_INITIAL_VECTOR signing_keys aes_initial_vector)
  get_target_property(KEYS_JSON_FILE signing_keys keys_json_file)

  # Add a dependency to the signed bootloader to make sure it's generated prior
  # to running the custom command below.
  add_dependencies(${ELF_FILENAME}.elf bootloader_signed mp_page_hex)
#[[
   Steps needed to generate the private and public key before signing the image.
   We use the open source tool openssl to do this.
   private key generation
     openssl ecparam -genkey -name prime256v1 -noout -out <private_key_name>.pem
   Public key generation
     openssl ec -in <private_key_name>.pem  -pubout -out <public_key_name>.pem
   image signing
     openssl dgst -sha256 -sign <private_key_name>.pem <image_file>.bin  >  <signed_file>.der
]]
  get_target_property(MY_ELF_LOCATION ${ELF_FILENAME}.elf BINARY_DIR)
  if(${PLATFORM} STREQUAL "ARM")
    set (IMAGE_START_ADDRESS  0x00000000)
  else()
    set (IMAGE_START_ADDRESS  0x10000000)
  endif()

  string(RANDOM RANDOM_STRING)
  add_custom_command(
    TARGET ${ELF_FILENAME}.elf
    POST_BUILD
    # Convert ELF to binary
    COMMAND ${CMAKE_OBJCOPY} -O binary ${ELF_FILENAME}.elf ${ELF_FILENAME}.bin

    # Generate a signed application binary
    COMMAND ${Python3_EXECUTABLE} ${ZW_SDK_ROOT}/tools/ota_image_generate.py --bin_only --sign ${PRIVATE_KEY_PATH} --input ${ELF_FILENAME}.bin --output ${ELF_FILENAME}_signed.bin

    # Combine signed application and signed bootloader
    COMMAND ${Python3_EXECUTABLE} ${ZW_SDK_ROOT}/tools/bootloader_app_merge.py --bootloader ${CMAKE_BINARY_DIR}/bootloader_signed.bin --app ${ELF_FILENAME}_signed.bin --output ${ELF_FILENAME}_signed_combined.bin

    # Convert the signed and combined binary to HEX
    COMMAND ${CMAKE_OBJCOPY} -I binary -O ihex --change-addresses ${IMAGE_START_ADDRESS} ${ELF_FILENAME}_signed_combined.bin ${ELF_FILENAME}_signed_combined.hex

    # Patch the signed and combined HEX file with the keys from the JSON file.
    COMMAND ${Python3_EXECUTABLE} ${TRIDENT_SDK_ROOT}/tools/cmake/tokens_patch.py --token-file ${TRIDENT_SDK_ROOT}/tools/tokens/T32CZ20B_mfg_token_def.json -m ${KEYS_JSON_FILE} ${ELF_FILENAME}_signed_combined.hex ${ELF_FILENAME}_signed_combined.hex

    # Add default MP data to combined and signed image
    COMMAND ${Python3_EXECUTABLE} ${ZW_SDK_ROOT}/tools/merge_hexfiles.py --hex1 ${ELF_FILENAME}_signed_combined.hex --hex2 ${CMAKE_BINARY_DIR}/default_mp_data.hex --output ${ELF_FILENAME}_signed_combined.hex

    # Generate firmware upgrade image
    COMMAND ${Python3_EXECUTABLE} ${ZW_SDK_ROOT}/tools/ota_image_generate.py --sign ${PRIVATE_KEY_PATH} --compress --encrypt ${AES_SYMETRICAL_KEY} --nonce ${AES_INITIAL_VECTOR} --input ${ELF_FILENAME}.bin --output ${ELF_FILENAME}.ota

    # Clean up each file.
    # Don't use *.bin because several different application targets are generated
    # in parallel in the same folder. Using *.bin might delete files before they
    # have been used.
    COMMAND ${CMAKE_COMMAND} -E remove ${ELF_FILENAME}.bin
    COMMAND ${CMAKE_COMMAND} -E remove ${ELF_FILENAME}_signed.bin
    COMMAND ${CMAKE_COMMAND} -E remove ${ELF_FILENAME}_signed_combined.bin

    WORKING_DIRECTORY ${MY_ELF_LOCATION}
    COMMENT "Signing ${ELF_FILENAME} and generating firmware update image."
  )

  # Original string containing the version
  set(ORIGINAL_FILE ${ELF_FILENAME})

  # Extract the version string using a regular expression
  string(REGEX MATCH "([0-9]+\\.[0-9]+\\.[0-9]+)" VERSION_STRING "${ORIGINAL_FILE}")

  # Check if the version string was found
  if(VERSION_STRING)

    # Extract the minor and patch versions
    string(REGEX REPLACE "^[0-9]+\\.(.*)" "\\1" MINOR_PATCH_VERSION "${VERSION_STRING}")

    # Set the new major version
    set(NEW_MAJOR_VERSION "255")

    # Combine the new major version with the minor and patch versions
    set(NEW_VERSION_STRING "${NEW_MAJOR_VERSION}.${MINOR_PATCH_VERSION}")

    # Replace the old version in the original string with the new version
    string(REPLACE "${VERSION_STRING}" "${NEW_VERSION_STRING}" V255_FILE "${ORIGINAL_FILE}")

    add_custom_command(
      TARGET ${ELF_FILENAME}.elf
      POST_BUILD
      # Convert ELF to binary
      COMMAND ${CMAKE_OBJCOPY} -O binary ${ELF_FILENAME}.elf ${V255_FILE}.bin

      # convert the major version to 255
      COMMAND ${Python3_EXECUTABLE} ${ZW_SDK_ROOT}/tools/change_major_version_to_255.py ${V255_FILE}.bin

      # Generate firmware upgrade image
      COMMAND ${Python3_EXECUTABLE} ${ZW_SDK_ROOT}/tools/ota_image_generate.py --sign ${PRIVATE_KEY_PATH} --compress --input ${V255_FILE}.bin --output ${V255_FILE}.ota
      COMMAND ${Python3_EXECUTABLE} ${ZW_SDK_ROOT}/tools/ota_image_generate.py --sign ${PRIVATE_KEY_PATH} --compress --encrypt ${AES_SYMETRICAL_KEY} --nonce ${AES_INITIAL_VECTOR} --input ${V255_FILE}.bin --output ${V255_FILE}.ota

      # Clean up each file.
      # Don't use *.bin because several different application targets are generated
      # in parallel in the same folder. Using *.bin might delete files before they
      # have been used.
      COMMAND ${CMAKE_COMMAND} -E remove ${V255_FILE}.bin
      WORKING_DIRECTORY ${MY_ELF_LOCATION}
      COMMENT "Generating the v255 ota file..."
    )
  else()
     message(DEBUG "Version string not found in the original string.")
  endif()

  if (${ZWSDK_IS_ROOT_PROJECT} STREQUAL "true")
    # Install files only if the SDK is the root project, and let application
    # projects configure installation independently, if desired.
    install(
      FILES
        ${CMAKE_CURRENT_BINARY_DIR}/${ELF_FILENAME}_signed_combined.hex
        ${CMAKE_CURRENT_BINARY_DIR}/${ELF_FILENAME}.ota
        ${CMAKE_CURRENT_BINARY_DIR}/${V255_FILE}.ota
      CONFIGURATIONS Release
      DESTINATION ${ZWSDK_APP_INSTALL_DIR}
    )
  endif()
endfunction()
