# SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
# SPDX-License-Identifier: LicenseRef-TridentMSLA

if(NOT DEFINED APP_NAME)
  set(APP_NAME "radio_cli")
endif()

set(ZW_INCLUDES
  ${ZW_SDK_ROOT}/platform/apps/inc
  ${CONFIG_DIR}/zw_rf
  ${ZW_SDK_ROOT}/modules/api/
  ${ZW_SDK_ROOT}/ThirdParty/embedded-cli/lib/include
  ${TRIDENT_SDK_ROOT}/framework/common/tokens/
  ${TRISDK_PATH}/tokens/
  ${TRIDENT_SDK_ROOT}/framework/utility/tlv_parser/
)

set(APP_SOURCES_REGION
  # Nothing here.
)

set(APP_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/radio_cli_startup.c
  ${CMAKE_CURRENT_SOURCE_DIR}/radio_cli_app.c
  ${CMAKE_CURRENT_SOURCE_DIR}/cli_uart_interface.c
  ${CMAKE_CURRENT_SOURCE_DIR}/radio_cli_commands.c
  ${ZW_SDK_ROOT}/z-wave-stack/Components/DebugPrint/DebugPrint.c
  ${ZW_SDK_ROOT}/z-wave-stack/Components/Assert/Assert_zw.c
  ${ZW_SDK_ROOT}/z-wave-stack/Components/EventDistributor/EventDistributor.c
  ${ZW_SDK_ROOT}/ThirdParty/embedded-cli/lib/src/embedded_cli.c
)

set(ZW_DEFINITIONS
  GIT_HASH_ID="${GIT_HASH}"
  ZAF_CONFIG_DEVICE_OPTION_MASK=APPLICATION_NODEINFO_LISTENING
)

set(ZW_LIBS "")

IF(CMAKE_BUILD_TYPE MATCHES Debug)
#  list(APPEND ZW_LIBS ZAF_DEBUGPRINT)
ENDIF()

if(CMAKE_BUILD_TYPE MATCHES Release OR CMAKE_BUILD_TYPE MATCHES Debug)
  zwsdk_create_native_app(
    NAME "${APP_NAME}"
    SOURCES "${APP_SOURCES}"
    INCLUDES "${ZW_INCLUDES}"
    LIBRARIES "${ZW_LIBS}"
    DEFINITIONS "${ZW_DEFINITIONS}"
  )
endif()
