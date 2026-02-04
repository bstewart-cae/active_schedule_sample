# SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
# SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
# SPDX-License-Identifier: BSD-3-Clause
add_test_subdirectory(tests)

if(NOT DEFINED APP_NAME)
  set(APP_NAME "zwave_soc_door_lock_keypad")
endif()

if(NOT TARGET DoorLockKeyPad_hw)
  add_library(DoorLockKeyPad_hw INTERFACE)
  target_link_libraries(DoorLockKeyPad_hw
    INTERFACE
      AppsHw_platform
  )
endif()

set(ZW_INCLUDES
  database
  database/schedules/inc
)

set(APP_SOURCES_REGION
  app.c
)

set(APP_SOURCES
  app_credentials.c
  app_cc_event_handlers.c
  database/database_common.c
  database/schedules/src/app_schedules.c
)

set(ZW_DEFINITIONS
  ZAF_CONFIG_PRODUCT_ID=1
  ZAF_CONFIG_GENERIC_TYPE=GENERIC_TYPE_ENTRY_CONTROL
  ZAF_CONFIG_SPECIFIC_TYPE=SPECIFIC_TYPE_DOOR_LOCK
  ZAF_CONFIG_REQUEST_KEY_S0=1
  ZAF_CONFIG_REQUEST_KEY_S2_ACCESS=1
  ZAF_CONFIG_DEVICE_OPTION_MASK=APPLICATION_FREQ_LISTENING_MODE_1000ms
  ZAF_CONFIG_APP_ROLE_TYPE=ZWAVEPLUS_INFO_REPORT_ROLE_TYPE_END_NODE_SLEEPING_LISTENING
  ZAF_CONFIG_INSTALLER_ICON_TYPE=ICON_TYPE_GENERIC_DOOR_LOCK_KEYPAD
  ZAF_CONFIG_USER_ICON_TYPE=ICON_TYPE_GENERIC_DOOR_LOCK_KEYPAD
  ZAF_CONFIG_SECURE_ONLY=1
  ZAF_CONFIG_MANUFACTURER_ID=${ZW_CONFIG_MANUFACTURER_ID}
  APP_MAX_TX_POWER_LR=200
  APP_MAX_TX_POWER=0
  APP_MEASURED_0DBM_TX_POWER=0
  # Example of User Credential CC configuration that overrides the default
  # values in cc_user_credential_config.h.
  CC_USER_CREDENTIAL_MAX_USER_UNIQUE_IDENTIFIERS=5
  CC_USER_CREDENTIAL_MAX_LENGTH_USER_NAME=10
  CC_USER_CREDENTIAL_USER_SCHEDULING_SUPPORTED=1
  CC_USER_CREDENTIAL_YEAR_DAY_SCHEDULES_PER_USER=1
  CC_USER_CREDENTIAL_DAILY_REPEATING_SCHEDULES_PER_USER=7
)

IF( CMAKE_BUILD_TYPE MATCHES Release OR CMAKE_BUILD_TYPE MATCHES Debug )

  set(ZW_LIBS
    DoorLockKeyPad_hw
    ZWaveSlave
    ZAF
    ZAF_EventDistributor_soc
    ZAF_TSE
    CC_ActiveSchedule
    CC_Basic
    CC_Battery
    CC_Battery_nvm
    CC_DoorLock
    CC_DoorLock_nvm
    CC_Notification
    CC_UserCode
    CC_UserCode_UserCredential
    CC_UserCredential
    CC_UserCredential_config
    CC_UserCredential_nvm
  )

IF(CMAKE_BUILD_TYPE MATCHES Debug)
  list(APPEND ZW_LIBS ZAF_DEBUGPRINT)
ENDIF()

  zw_create_app(
    NAME "${APP_NAME}"
    SOURCES "${APP_SOURCES}"
    SOURCES_REGION "${APP_SOURCES_REGION}"
    INCLUDES "${ZW_INCLUDES}"
    LIBS "${ZW_LIBS}"
    DEFINITIONS "${ZW_DEFINITIONS}"
    REGIONS "${FREQ_LIST}"
  )
ENDIF()
