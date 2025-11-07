# SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
# SPDX-License-Identifier: LicenseRef-TridentMSLA

if(NOT DEFINED APP_NAME)
  set(APP_NAME "power_strip") # Must match the name passed to zw_create_app().
endif()

# Set include directories required by optional functionality.
set(INCLUDES
  "${ZW_SDK_ROOT}/platform/apps/common/"
  "${TRIDENT_SDK_ROOT}/framework/utility/cli/"
  "${TRIDENT_SDK_ROOT}/framework/utility/printf/"
  $<TARGET_PROPERTY:ZAF,INTERFACE_INCLUDE_DIRECTORIES>
)

set(HARDWARE_LIBS "")
if( "${ZWSDK_CONFIG_USE_TR_CLI}" STREQUAL "" )
  set(HW_LIB_SRC "${ZW_SDK_ROOT}/platform/apps/src/rgb_led_drv.c")
else()
  set(HW_LIB_SRC "${ZW_SDK_ROOT}/platform/apps/src/rgb_led_drv.c"
                 "${ZW_SDK_ROOT}/platform/apps/common/setup_common_cli.c"
                 "${TRIDENT_SDK_ROOT}/framework/utility/cli/tr_cli.c"
                 "${TRIDENT_SDK_ROOT}/framework/utility/cli/tr_cli_command_parser.c")
  if( ${ZWSDK_CONFIG_USE_TR_CLI} STREQUAL "REMOTE" )
    set(HW_LIB_SRC "${HW_LIB_SRC};${ZW_SDK_ROOT}/platform/apps/common/remote_cli.c")
    set(CLI_CONFIG "TR_CLI_HISTORY_LEN=0;TR_CLI_LOCAL_ECHO=0;TR_CLI_ENABLED")
    set(INCLUDES "${INCLUDES};${ZW_SDK_ROOT}/z-wave-stack/ZAF/CommandClasses/RemoteCLI/inc/")
  else()
    set(HW_LIB_SRC "${HW_LIB_SRC};${ZW_SDK_ROOT}/platform/apps/common/local_cli.c")
    set(CLI_CONFIG "TR_CLI_ENABLED")
  endif()
endif()

tr_zw_generate_hardware_libraries(
  NAME ${APP_NAME}
  SOURCES "${HW_LIB_SRC}"
  INCLUDES "${INCLUDES}"
  LIBS "${HARDWARE_LIBS}"
  DEFINITIONS "${CLI_CONFIG}"
)
