# SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
# SPDX-License-Identifier: LicenseRef-TridentMSLA

add_compile_definitions( GIT_HASH_ID="${GIT_HASH}" )

add_library(AppsHw_platform STATIC)
target_sources(AppsHw_platform
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/apps_hw.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/board_init.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/board_indicator.c
)

target_include_directories(AppsHw_platform
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
    ${ZW_SDK_ROOT}/platform/TridentIoT/inc
    ${ZW_SDK_ROOT}/platform/TridentIoT/PAL/inc
    ${TRISDK_PATH}/Library/CMSIS/Include/
    ${ZW_SDK_ROOT}/z-wave-stack/ZAF/ApplicationUtilities/
    ${ZW_SDK_ROOT}/z-wave-stack/ZAF/ApplicationUtilities/EventHandling/
    ${ZW_SDK_ROOT}/z-wave-stack/ZAF/ApplicationUtilities/Actuator/
    ${ZW_SDK_ROOT}/z-wave-stack/ZAF/CommandClasses/Common/
    ${ZW_SDK_ROOT}/z-wave-stack/ZAF/CommandClasses/BinarySwitch/inc/
    ${ZW_SDK_ROOT}/z-wave-stack/ZAF/CommandClasses/ColorSwitch/inc/
    ${ZW_SDK_ROOT}/z-wave-stack/ZAF/CommandClasses/MultilevelSwitch/inc/
    ${ZW_SDK_ROOT}/z-wave-stack/Components/EventDistributor/
    ${ZW_SDK_ROOT}/z-wave-stack/Components/QueueNotifying/
    ${ZW_SDK_ROOT}/z-wave-stack/Components/NodeMask/
    ${ZW_SDK_ROOT}/z-wave-stack/Components/Assert/
    ${ZW_SDK_ROOT}/z-wave-stack/Components/SwTimer/
    ${ZW_SDK_ROOT}/z-wave-stack/Components/Utils/
    ${ZW_SDK_ROOT}/z-wave-stack/Components/DebugPrint
    ${ZW_SDK_ROOT}/z-wave-stack/ZWave/API/
    ${ZW_SDK_ROOT}/z-wave-stack/PAL/inc
  PRIVATE
    ${ZW_SDK_ROOT}/z-wave-stack/AppsHw/inc
    $<TARGET_PROPERTY:zpal_${PLATFORM_VARIANT},INTERFACE_INCLUDE_DIRECTORIES>
 )

target_link_libraries(AppsHw_platform
  PUBLIC
    zpal_${PLATFORM_VARIANT}
  INTERFACE
    AppsHw
    MfgTokens
)
target_compile_definitions(AppsHw_platform
  PUBLIC
    -DTR_PLATFORM_${PLATFORM_VARIANT} # Must be public so that application hardware libraries inherit this define.
)

if(ZWSDK_IS_ROOT_PROJECT STREQUAL "false" AND NOT ZWSDK_BUILD_SAMPLE_APPLICATIONS)
  # Do not generate hardware libraries
else()
  add_subdirectory(door_lock_keypad)
  add_subdirectory(led_bulb)
  add_subdirectory(multilevel_sensor)
  add_subdirectory(power_strip)
  add_subdirectory(sensor_pir)
  add_subdirectory(serial_api)
  add_subdirectory(switch_on_off)
  add_subdirectory(wall_controller)
  add_subdirectory(window_sensor)
  add_subdirectory(radio_cli)
  add_subdirectory(zniffer)
endif()
