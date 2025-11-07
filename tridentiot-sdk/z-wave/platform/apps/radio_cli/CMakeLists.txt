# SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
# SPDX-License-Identifier: LicenseRef-TridentMSLA

if(NOT DEFINED APP_NAME)
  set(APP_NAME "radio_cli") # Must match the name passed to zwsdk_create_native_app().
endif()

# Set include directories required by optional functionality.
set(INCLUDES
  "${ZW_SDK_ROOT}/platform/apps/common/"
  $<TARGET_PROPERTY:ZAF,INTERFACE_INCLUDE_DIRECTORIES>
)

tr_zw_generate_hardware_libraries(
  NAME ${APP_NAME}
  SOURCES "${HW_LIB_SRC}"
  INCLUDES "${INCLUDES}"
  LIBS "${HARDWARE_LIBS}"
  DEFINITIONS "${CLI_CONFIG}"
)
