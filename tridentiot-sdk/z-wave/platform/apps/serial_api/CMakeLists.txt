# SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
# SPDX-License-Identifier: LicenseRef-TridentMSLA

if(NOT DEFINED APP_NAME)
  set(APP_NAME "serial_api") # Must match the name passed to zw_create_app().
endif()

# Set include directories required by optional functionality.
set(INCLUDES
  # No include directories yet.
)

tr_zw_generate_hardware_libraries(
  NAME ${APP_NAME}
  SOURCES "${TRISDK_PERIPHERAL_PATH}/pwm.c"
  INCLUDES "${INCLUDES}"
  LIBS ""
  DEFINITIONS ""
)
