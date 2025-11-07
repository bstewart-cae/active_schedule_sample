# SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
# SPDX-License-Identifier: LicenseRef-TridentMSLA

cmake_minimum_required(VERSION 3.23.5)
cmake_policy(VERSION 3.23.5)

# Full documentation of the build time configuration can be found at https://tridentiot.github.io/tridentiot-sdk-docs/latest/z-wave/build_system_customization.html

if (NOT ZWSDK_CONFIG_USE_SOURCES)
  # Use Z-Wave stack and ZPAL libraries for build.
  set(ZWSDK_CONFIG_USE_SOURCES "OFF")
  # Build Z-Wave stack and ZPAL as source.
  # set(ZWSDK_CONFIG_USE_SOURCES "ON")
endif()

if (NOT DEFINED ZWSDK_CONFIG_REGION)
  # Set the region to build the application for.
  set(ZWSDK_CONFIG_REGION REGION_US_LR)
  # set(ZWSDK_CONFIG_REGION REGION_EU)
  # set(ZWSDK_CONFIG_REGION REGION_US)
  # set(ZWSDK_CONFIG_REGION REGION_ANZ)
  # set(ZWSDK_CONFIG_REGION REGION_HK)
  # set(ZWSDK_CONFIG_REGION REGION_IN)
  # set(ZWSDK_CONFIG_REGION REGION_IL)
  # set(ZWSDK_CONFIG_REGION REGION_RU)
  # set(ZWSDK_CONFIG_REGION REGION_CN)
  # set(ZWSDK_CONFIG_REGION REGION_EU_LR)
  # set(ZWSDK_CONFIG_REGION REGION_JP)
  # set(ZWSDK_CONFIG_REGION REGION_KR)
endif()

if (NOT DEFINED ZWSDK_CONFIG_USE_TR_CLI)
  # Enable local CLI.
  set(ZWSDK_CONFIG_USE_TR_CLI "LOCAL")
  # Enable remote CLI.
  # set(ZWSDK_CONFIG_USE_TR_CLI "REMOTE")
  # Disable CLI.
  # set(ZWSDK_CONFIG_USE_TR_CLI "")
endif()

if (NOT DEFINED ZWSDK_CONFIG_PRIVATE_ROOT_SIGNING_KEY_PATH)
  # Define the location of the root private signing key. If not set the build system will generate
  # a keypair on the first build.
  # NOTE: All keys MUST be configured to enable use of custom keys.
  #set(ZWSDK_CONFIG_PRIVATE_ROOT_SIGNING_KEY_PATH "${CMAKE_SOURCE_DIR}/keys/my_private_root_signing_key.pem")
endif()

if (NOT DEFINED ZWSDK_CONFIG_PUBLIC_ROOT_SIGNING_KEY_PATH)
  # Define the location of the public root signing key.
  # NOTE: All keys MUST be configured to enable use of custom keys.
  #set(ZWSDK_CONFIG_PUBLIC_ROOT_SIGNING_KEY_PATH "${CMAKE_SOURCE_DIR}/keys/my_public_root_signing_key.der")
endif()

if (NOT DEFINED ZWSDK_CONFIG_PRIVATE_SIGNING_KEY_PATH)
  # Define the location of the private signing key. If not set the build system will generate
  # a keypair on the first build.
  # NOTE: All keys MUST be configured to enable use of custom keys.
  #set(ZWSDK_CONFIG_PRIVATE_SIGNING_KEY_PATH "${CMAKE_SOURCE_DIR}/keys/my_private_signing_key.pem")
endif()

if (NOT DEFINED ZWSDK_CONFIG_PUBLIC_SIGNING_KEY_PATH)
  # Define the location of the public signing key.
  # NOTE: All keys MUST be configured to enable use of custom keys.
  #set(ZWSDK_CONFIG_PUBLIC_SIGNING_KEY_PATH "${CMAKE_SOURCE_DIR}/keys/my_public_signing_key.der")
endif()

if (NOT DEFINED ZWSDK_CONFIG_ENCRYPTION_KEY_PATH)
  # Define the location of the encryption key.
  # NOTE: All keys MUST be configured to enable use of custom keys.
  #set(ZWSDK_CONFIG_ENCRYPTION_KEY_PATH "${CMAKE_SOURCE_DIR}/keys/my_encryption_key.hex")
endif()
