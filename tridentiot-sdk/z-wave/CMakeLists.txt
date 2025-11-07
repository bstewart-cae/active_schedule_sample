# SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
# SPDX-License-Identifier: LicenseRef-TridentMSLA

cmake_minimum_required(VERSION 3.23.5)
cmake_policy(VERSION 3.23.5)

find_package(Python3 REQUIRED)
if(NOT DEFINED Python3_FOUND)
  message(FATAL_ERROR "Python not found")
endif()

set(Python3_EXECUTABLE ${Python3_EXECUTABLE} CACHE INTERNAL Python3_EXECUTABLE)

message(STATUS "Python3_EXECUTABLE: ${Python3_EXECUTABLE}")

option(ZWSDK_BUILD_SAMPLE_APPLICATIONS "If set to ON, sample applications shipped with the SDK will be built." OFF)
option(ZWSDK_CONFIG_USE_SOURCES "If set to ON, the build system will compile all sources instead of linking pre-built libraries." OFF)

set(ZW_SDK_ROOT "${CMAKE_CURRENT_SOURCE_DIR}" CACHE INTERNAL ZW_SDK_ROOT)
set(ZW_SDK_MODULES "${ZW_SDK_ROOT}/modules")
set(TR_TEST_TOOLS_DIR "${ZW_SDK_ROOT}/tools" CACHE INTERNAL TR_TEST_TOOLS_DIR)
set(TRIDENT_SDK_ROOT "${ZW_SDK_ROOT}/.." CACHE INTERNAL TRIDENT_SDK_ROOT)
set(ZWSDK_APP_INSTALL_DIR "${TRIDENT_SDK_ROOT}/apps/bin/z-wave")

set(ZW_VERSION_MAJOR  25)
set(ZW_VERSION_MINOR  10)
set(ZW_VERSION_PATCH  0)

set(ZAF_VERSION_MAJOR 25)
set(ZAF_VERSION_MINOR 10)
set(ZAF_VERSION_PATCH 0)

set(SDK_VERSION_MAJOR 25)
set(SDK_VERSION_MINOR 10)
set(SDK_VERSION_PATCH 0)

set(ZW_PROTOCOL_MANUFACTURER_ID "0x0462")

if (${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_CURRENT_SOURCE_DIR})
  # If the source dir and current source dir match, it means this is the parent
  # CMake project.
  set(ZWSDK_IS_ROOT_PROJECT "true" CACHE INTERNAL "")
else()
  # Otherwise, it's not the parent project.
  set(ZWSDK_IS_ROOT_PROJECT "false" CACHE INTERNAL "")
endif()

message(DEBUG "ZWSDK_IS_ROOT_PROJECT: ${ZWSDK_IS_ROOT_PROJECT}")

if( CMAKE_BUILD_TYPE STREQUAL Test )

  project(z-wave-sdk-unit-test
          VERSION ${SDK_VERSION_MAJOR}.${SDK_VERSION_MINOR}.${SDK_VERSION_PATCH}
          LANGUAGES C)

  include(CTest)
  enable_testing()

  add_compile_options(
    -m32
    -funwind-tables
    -g3
    -O0
    $<$<COMPILE_LANGUAGE:CXX>:-fexceptions>
    -Werror
    -Wall
    -fprofile-arcs
    -ftest-coverage
    -fPIC
  )

  add_link_options(
    -m32
    -funwind-tables
    -Wl,-Map=output.map
    -lgcov
    --coverage
  )

  #
  # Unity and CMock
  #

  # compile the unity version bundled along with cmock sources.
  add_library(unity
    ${ZW_SDK_ROOT}/z-wave-stack/ThirdParty/cmock/vendor/unity/src/unity.c
    ${ZW_SDK_ROOT}/z-wave-stack/SubTree/TestFramework/unity/generic/unity_print.c)
    target_include_directories(unity
      PUBLIC
        ${ZW_SDK_ROOT}/z-wave-stack/ThirdParty/cmock/vendor/unity/src # Unity in CMock folder MUST be the first include folder so that unity.h is taken from here.
        ${ZW_SDK_ROOT}/z-wave-stack/SubTree/TestFramework/unity
    )
  target_compile_options(unity PRIVATE "-fPIC")

  # Build the cmock library and link the above compiled unity with the cmock library
  add_library(cmock STATIC ${ZW_SDK_ROOT}/z-wave-stack/ThirdParty/cmock/src/cmock.c)
  target_include_directories(cmock PUBLIC ${ZW_SDK_ROOT}/z-wave-stack/ThirdParty/cmock/src)
  target_link_libraries(cmock PUBLIC unity)
  target_compile_options(cmock PRIVATE "-fPIC")
  target_compile_definitions(cmock PRIVATE "-DCMOCK_MEM_DYNAMIC") # Support large number of mocks

  include(tools/tr_add_mock.cmake)
  include(tools/tr_add_unity_test.cmake)

  # Use the RT584 SDK
  set(TRISDK_PATH "${TRIDENT_SDK_ROOT}/platform/T32CZ20")

  add_mock(hal_gpio_cmock ${TRIDENT_SDK_ROOT}/framework/hal/include/tr_hal_gpio.h)
  target_compile_definitions(hal_gpio_cmock PUBLIC __ARM_ARCH_PROFILE='M')
  target_include_directories(hal_gpio_cmock PUBLIC
    ${TRIDENT_SDK_ROOT}/framework/hal/T32CZ20/
    ${ZW_SDK_ROOT}/platform/TridentIoT/PAL/test/mock_includes/
    ${ZW_SDK_ROOT}/platform/TridentIoT/config/
    ${TRISDK_PATH}/Library/RT584/PHY/include/
    ${TRISDK_PATH}/Library/CMSIS/Include/
  )

  add_mock(hal_uart_cmock ${TRIDENT_SDK_ROOT}/framework/hal/include/tr_hal_uart.h)
  target_compile_definitions(hal_uart_cmock PUBLIC __ARM_ARCH_PROFILE='M')
  target_include_directories(hal_uart_cmock PUBLIC
    ${TRIDENT_SDK_ROOT}/framework/hal/T32CZ20/
    ${ZW_SDK_ROOT}/platform/TridentIoT/PAL/test/mock_includes/
    ${ZW_SDK_ROOT}/platform/TridentIoT/config/
    ${TRISDK_PATH}/Library/RT584/PHY/include/
    ${TRISDK_PATH}/Library/CMSIS/Include/
  )

  add_mock(sysfun_cmock ${TRISDK_PATH}/Library/RT584/Peripheral/Include/sysfun.h)
  target_include_directories(sysfun_cmock PUBLIC
    ${ZW_SDK_ROOT}/platform/TridentIoT/PAL/test/mock_includes/
    ${ZW_SDK_ROOT}/platform/TridentIoT/config/
    ${TRISDK_PATH}/Library/RT584/PHY/include/
    ${TRISDK_PATH}/Library/CMSIS/Include/
  )
  target_compile_definitions(sysfun_cmock PUBLIC __ARM_ARCH_PROFILE='M')

  add_mock(flashctl_cmock ${TRISDK_PATH}/Library/RT584/Peripheral/Include/flashctl.h)
  target_include_directories(flashctl_cmock PUBLIC
    ${ZW_SDK_ROOT}/platform/TridentIoT/PAL/test/mock_includes/
    ${ZW_SDK_ROOT}/platform/TridentIoT/config/
    ${TRISDK_PATH}/Library/RT584/PHY/include/
    ${TRISDK_PATH}/Library/CMSIS/Include/
  )
  target_compile_definitions(flashctl_cmock PUBLIC __ARM_ARCH_PROFILE='M')

  add_mock(radio_t32cz20_cmock ${ZW_SDK_ROOT}/platform/TridentIoT/PAL/src/T32CZ20/include/zwave_radio.h)
  target_include_directories(radio_t32cz20_cmock PUBLIC
    ${ZW_SDK_ROOT}/platform/TridentIoT/PAL/src/T32CZ20/include/
    ${TRISDK_PATH}/Library/CMSIS/Include/
    ${TRISDK_PATH}/Library/RT584/Include/
    ${TRISDK_PATH}/Library/RT584/PHY/include/
    ${TRISDK_PATH}/Library/RT584/Peripheral/Include/
    ${TRISDK_PATH}/Middleware/RFB/include/
    ${ZW_SDK_ROOT}/z-wave-stack/PAL/inc/
    ${ZW_SDK_ROOT}/platform/TridentIoT/PAL/inc/
    ${ZW_SDK_ROOT}/platform/TridentIoT/PAL/src/
  )

  # Generate mocks from fake header files as it is not possible to generate mocks from the real header
  add_mock(sysctrl_cmock ${ZW_SDK_ROOT}/platform/TridentIoT/PAL/test/mock_includes/sysctrl.h)
  add_mock(cm33_cmock ${ZW_SDK_ROOT}/platform/TridentIoT/PAL/test/mock_includes/cm33.h)
  target_include_directories(cm33_cmock
    PRIVATE
    ${ZW_SDK_ROOT}/platform/TridentIoT/PAL/test/mock_includes
  )
  target_compile_definitions(cm33_cmock PUBLIC __ARM_ARCH_PROFILE='M')

  add_mock(radio_cmock ${ZW_SDK_ROOT}/platform/TridentIoT/PAL/test/mock_includes/rfb.h)
  target_include_directories(radio_cmock
    PRIVATE
    ${ZW_SDK_ROOT}/platform/TridentIoT/PAL/test/mock_includes
  )

  target_compile_definitions(radio_cmock PRIVATE RFB_SUBG_ENABLED=1 RFB_ZIGBEE_ENABLED=1 RFB_BLE_ENABLED=1)
  target_compile_definitions(radio_t32cz20_cmock PRIVATE RFB_SUBG_ENABLED=1 RFB_ZIGBEE_ENABLED=1 RFB_BLE_ENABLED=1)

  add_mock(zpal_init_cmock ${ZW_SDK_ROOT}/z-wave-stack/PAL/inc/zpal_init.h)

  add_mock(zpal_power_manager_cmock ${ZW_SDK_ROOT}/z-wave-stack/PAL/inc/zpal_power_manager.h)
else()

  project(z-wave-sdk
          VERSION ${SDK_VERSION_MAJOR}.${SDK_VERSION_MINOR}.${SDK_VERSION_PATCH}
          LANGUAGES C ASM)

  # The z-wave-stack will use the regions defined in a variable named "CUSTOM_FREQ".
  if(DEFINED ZWSDK_CONFIG_REGION AND NOT ${ZWSDK_CONFIG_REGION} STREQUAL "")
    message(DEBUG "Set CUSTOM_FREQ to ${ZWSDK_CONFIG_REGION}")
    set(CUSTOM_FREQ "${ZWSDK_CONFIG_REGION}")
  else()
    set(CUSTOM_FREQ
      "REGION_EU"
      "REGION_EU_LR"
      "REGION_US"
      "REGION_US_LR"
      "REGION_ANZ"
      "REGION_HK"
      "REGION_IN"
      "REGION_IL"
      "REGION_RU"
      "REGION_CN"
      "REGION_JP"
      "REGION_KR"
    )
  endif()

  # Enable application CLI for debug build or if enabled command line
  if(NOT DEFINED ZWSDK_CONFIG_USE_TR_CLI)
    if(CMAKE_BUILD_TYPE MATCHES Debug)
      set(ZWSDK_CONFIG_USE_TR_CLI "LOCAL" CACHE INTERNAL "" )
    else()
      set(ZWSDK_CONFIG_USE_TR_CLI "" CACHE INTERNAL "" )
    endif()
  endif()

  find_program(GIT "git")
  IF((DEFINED ENV{GITHUB_SHA}) AND ($ENV{GITHUB_SHA} MATCHES "^[a-fA-F0-9]+$"))
    # GIT_COMMIT exists and it is valid commit ID number.
    SET(GIT_HASH $ENV{GITHUB_SHA} CACHE INTERNAL GIT_HASH)
  ELSEIF(GIT)
    execute_process(
      COMMAND ${GIT} log -1 --format=%H
      RESULT_VARIABLE GIT_LOG_RESULT
      OUTPUT_VARIABLE GIT_LOG_OUTPUT
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    IF((NOT GIT_LOG_RESULT) AND (GIT_LOG_OUTPUT MATCHES "^[a-fA-F0-9]+$"))
      SET(GIT_HASH ${GIT_LOG_OUTPUT} CACHE INTERNAL GIT_HASH)
    ENDIF()
  ENDIF()

  IF(NOT DEFINED GIT_HASH)
    SET(GIT_HASH 0000000000000000000000000000000000000000 CACHE INTERNAL GIT_HASH)
  ENDIF()

  message("GIT_HASH: ${GIT_HASH}")

  if(${PLATFORM} STREQUAL "T32CZ20")
    set(TRISDK_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../platform/T32CZ20" CACHE INTERNAL TRISDK_PATH)
    set(TRISDK_PERIPHERAL_PATH "${TRISDK_PATH}/Library/RT584/Peripheral" CACHE INTERNAL TRISDK_PERIPHERAL_PATH)
    set(TRISDK_PHY_PATH "${TRISDK_PATH}/Library/RT584/PHY" CACHE INTERNAL TRISDK_PHY_PATH)

    if(DEFINED ZWSDK_CONFIG_BOOTLOADER_PATH AND NOT ${ZWSDK_CONFIG_BOOTLOADER_PATH} STREQUAL "")
      if (NOT EXISTS "${ZWSDK_CONFIG_BOOTLOADER_PATH}")
        message(FATAL_ERROR "${ZWSDK_CONFIG_BOOTLOADER_PATH} does not exist")
      endif()
      set(BOOTLOADER_FILE "${ZWSDK_CONFIG_BOOTLOADER_PATH}" CACHE INTERNAL "Path to bootloader binary.")
    else()
      set(BOOTLOADER_FILE "${CMAKE_CURRENT_SOURCE_DIR}/bootloader/bootloader_T32CZ20_v25.10.0.bin" CACHE INTERNAL BOOTLOADER_FILE)
    endif()

    message(STATUS "Using bootloader from ${BOOTLOADER_FILE}")

    # To use the FlashDB file system set TARGET_FS to FLASH_DB
    set(TARGET_FS "FLASH_DB")

    include(tools/ota_image_tool.cmake)
    include(tools/image_size_tool.cmake)
    include(cmake/zw_create_app_external.cmake)
    include(cmake/zwsdk_generate_signing_keys.cmake)
    include(z-wave-stack/DevTools/CMakeFunctions/to_hex.cmake)
    include(z-wave-stack/DevTools/CMakeFunctions/app_executable.cmake)

    zwsdk_generate_signing_and_encryption_keys()

    zw_create_mp_hex_target()

    add_subdirectory("platform")

    zwsdk_create_signed_bootloader_target()
    set(ZW_CONFIG_MANUFACTURER_ID "0x0462" CACHE INTERNAL "") # Set Trident IoT manufacturer ID.

    # Defining zw_app_callback() makes zw_create_app() invoke it.
    function(zw_app_callback ELF_TARGET)
      # Generate OTA files
      string(REPLACE ".elf" ""  ELF_NAME ${ELF_TARGET})

      set(DEVICE_NAME T32CZ20)
      set(CORE_NAME M33)
      set(FREERTOS_PLUGIN ARM)

      if(NOT DEFINED ENV{GITHUB_ACTIONS})
        add_custom_command (
          TARGET ${ELF_TARGET}
          POST_BUILD
          COMMAND ${Python3_EXECUTABLE} ${TR_TEST_TOOLS_DIR}/generate_ozone_project_file.py --elf-file  ${CMAKE_CURRENT_BINARY_DIR}/${ELF_NAME}.elf --template ${TR_TEST_TOOLS_DIR}/templates/ozone_project_file.jinja --output-dir ${CMAKE_CURRENT_BINARY_DIR} --device ${DEVICE_NAME} --core ${CORE_NAME} --freertos ${FREERTOS_PLUGIN}
          COMMENT "Generating Ozone debugger project file for ${PLATFORM} using ${Python3_EXECUTABLE}"
        )
      else()
        message(STATUS "Running from github pipeline. Don't create the Ozone project")
      endif()

      zwsdk_generate_fw_update_image(${ELF_NAME})

      # Generate size file
      size_file_generate(${ELF_NAME})

    endfunction()
  endif()
endif()

if(${ZWSDK_IS_ROOT_PROJECT} STREQUAL "false")
  # This is not the root project => Don't build example applications.
  # Unless the root project desires so.
  if(ZWSDK_BUILD_SAMPLE_APPLICATIONS)
    # Do nothing.
  else()
    # Skip sample applications.
    set(ZW_SKIP_APPS 1)
  endif()

  if(ZWSDK_CONFIG_USE_SOURCES)
    # Don't import libraries
  else()
    # Do import libraries.
    set(ZW_CONFIG_IMPORT_LIBRARIES "true")
  endif()
endif()

set(ZW_CONFIG_LIBRARY_DESTINATION "${ZW_SDK_ROOT}/lib")

add_subdirectory("z-wave-stack")

add_subdirectory("modules")

if( NOT CMAKE_BUILD_TYPE STREQUAL Test)
  if(${PLATFORM} STREQUAL "T32CZ20")
    add_subdirectory("apps")
  endif()
endif(NOT CMAKE_BUILD_TYPE STREQUAL Test)

if( CMAKE_BUILD_TYPE STREQUAL Test )
  add_subdirectory("platform/TridentIoT/PAL/test")
endif(CMAKE_BUILD_TYPE STREQUAL Test)
