# SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
#
# SPDX-License-Identifier: BSD-3-Clause

if(NOT CMAKE_BUILD_TYPE STREQUAL Test)

  set(PLATFORM_VARIANTS
    x86_REALTIME
    x86_EMULATED
  )

  string(REPLACE ";" "\n\t" PLATFORM_VARIANTS_STR "${PLATFORM_VARIANTS}")
  if(NOT DEFINED PLATFORM_VARIANT)
    message(FATAL_ERROR "PLATFORM_VARIANT not defined. Available platforms:\n\t${PLATFORM_VARIANTS_STR}")
  endif()
  if (NOT ${PLATFORM_VARIANT} IN_LIST PLATFORM_VARIANTS)
    message(FATAL_ERROR "Unknown PLATFORM_VARIANT: ${PLATFORM_VARIANT}. Available platforms:\n\t${PLATFORM_VARIANTS_STR}")
  endif()
  message("PLATFORM_VARIANT: ${PLATFORM_VARIANT}")

  set(PLATFORM_COMPILE_OPTIONS 
    "$<$<CONFIG:Debug>:-Og>"
    "$<$<CONFIG:Release>:-O2>"
  )
  set(PLATFORM_COMPILE_OPTIONS ${PLATFORM_COMPILE_OPTIONS} PARENT_SCOPE)
  add_compile_options(${PLATFORM_COMPILE_OPTIONS})

  set(PLATFORM_DEFINITIONS 
    ZWAVE_ON_LINUX # For libs2
    ${PLATFORM_VARIANT}
  )
  add_compile_definitions(${PLATFORM_DEFINITIONS})
  set(PLATFORM_DEFINITIONS ${PLATFORM_DEFINITIONS} PARENT_SCOPE)

  set(PLATFORM_SUPPORTED_APPS
    zwave_ncp_serial_api_controller
    zwave_ncp_serial_api_end_device
    zwave_soc_door_lock_keypad
    zwave_soc_led_bulb
    zwave_soc_power_strip
    zwave_soc_sensor_pir
    zwave_soc_switch_on_off
    zwave_soc_wall_controller
    zwave_soc_multilevel_sensor
  PARENT_SCOPE)

  add_subdirectory(zne)
  add_subdirectory(PAL)
  add_subdirectory(AppsHw)
endif()
