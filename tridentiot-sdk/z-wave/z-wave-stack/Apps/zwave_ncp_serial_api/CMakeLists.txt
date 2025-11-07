# SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
# SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
#
# SPDX-License-Identifier: BSD-3-Clause
add_test_subdirectory(tests)

IF(CMAKE_BUILD_TYPE MATCHES Test)
  # Stop processing this file if building Tests
  return()
endif()

if(NOT DEFINED APP_NAME)
  set(APP_NAME "zwave_ncp_serial_api")
endif()

if(NOT TARGET SerialAPI_hw)
  # This library is required by the x86 build only.
  add_library(SerialAPI_hw INTERFACE)
  target_link_libraries(SerialAPI_hw
    INTERFACE
      AppsHw_platform
  )
endif()

set(APP_SOURCES_REGION
  app.c
)

set(APP_SOURCES
  app_node_info.c
  comm_interface.c
  serialapi_file.c
  nvm_backup_restore.c
  utils.c
  cmds_management.c
  cmds_rf.c
  cmds_security.c
  cmd_handlers.c
  cmd_handlers_invoker.c
  virtual_slave_node_info.c
  cmd_get_capabilities.c
  cmd_firmware_update.c
)

set(APP_INCLUDES
  ${ZAF_CONFIGDIR}/config/
  ${ZAF_CONFIGDIR}/inc/
)


#
# Create Serial API Controller
#
set(APP_LIBS
  SerialAPI_hw
  ZWaveController
  ZAF_AppTimer
  ZAF_CommonInterface
  ZAF_EventDistributor_ncp
  ZAF_nvm
  SwTimer
  SyncEvent
  Utils
)

# IF(CMAKE_BUILD_TYPE MATCHES Debug)
  list(APPEND APP_LIBS ZAF_DEBUGPRINT)
# ENDIF()

set(ZW_DEFINITIONS
  ZAF_CONFIG_DEVICE_OPTION_MASK=APPLICATION_NODEINFO_LISTENING
  ZAF_CONFIG_GENERIC_TYPE=GENERIC_TYPE_STATIC_CONTROLLER
  ZAF_CONFIG_INSTALLER_ICON_TYPE=0
  ZAF_CONFIG_PRODUCT_ID=4
  ZAF_CONFIG_SPECIFIC_TYPE=SPECIFIC_TYPE_PC_CONTROLLER
  ZAF_CONFIG_MANUFACTURER_ID=${ZW_CONFIG_MANUFACTURER_ID}
)

zw_create_app(
  NAME "${APP_NAME}_controller"
  SOURCES "${APP_SOURCES}"
  SOURCES_REGION "${APP_SOURCES_REGION}"
  INCLUDES "${APP_INCLUDES}"
  LIBS "${APP_LIBS}"
  DEFINITIONS "${ZW_DEFINITIONS}"
  REGIONS "${FREQ_LIST}"
)

#
# Create Serial API End Device
#
set(APP_LIBS
  SerialAPI_hw
  ZWaveSlave
  ZAF_AppTimer
  ZAF_CommonInterface
  ZAF_EventDistributor_ncp
  ZAF_nvm
  SwTimer
  SyncEvent
  Utils
)

set(ZW_DEFINITIONS
  ZAF_CONFIG_DEVICE_OPTION_MASK=APPLICATION_NODEINFO_LISTENING
  ZAF_CONFIG_GENERIC_TYPE=GENERIC_TYPE_SWITCH_MULTILEVEL
  ZAF_CONFIG_INSTALLER_ICON_TYPE=0
  ZAF_CONFIG_PRODUCT_ID=4
  ZAF_CONFIG_SPECIFIC_TYPE=SPECIFIC_TYPE_NOT_USED
  ZAF_CONFIG_MANUFACTURER_ID=${ZW_CONFIG_MANUFACTURER_ID}
  APP_MAX_TX_POWER_LR=200
  APP_MAX_TX_POWER=0
  APP_MEASURED_0DBM_TX_POWER=0
)

zw_create_app(
  NAME "${APP_NAME}_end_device"
  SOURCES "${APP_SOURCES}"
  SOURCES_REGION "${APP_SOURCES_REGION}"
  INCLUDES "${APP_INCLUDES}"
  LIBS "${APP_LIBS}"
  DEFINITIONS "${ZW_DEFINITIONS}"
  REGIONS "${FREQ_LIST}"
)
