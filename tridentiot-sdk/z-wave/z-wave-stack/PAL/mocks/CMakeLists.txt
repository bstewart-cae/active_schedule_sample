# SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
#
# SPDX-License-Identifier: BSD-3-Clause

add_library(zpal_mock OBJECT
  ${CMAKE_CURRENT_SOURCE_DIR}/zpal_bootloader_mock.c
  ${CMAKE_CURRENT_SOURCE_DIR}/zpal_entropy_mock.c
  ${CMAKE_CURRENT_SOURCE_DIR}/zpal_init_mock.c
  ${CMAKE_CURRENT_SOURCE_DIR}/zpal_misc_mock.c
  ${CMAKE_CURRENT_SOURCE_DIR}/zpal_nvm_mock.c
  ${CMAKE_CURRENT_SOURCE_DIR}/zpal_power_manager_mock.c
  ${CMAKE_CURRENT_SOURCE_DIR}/zpal_radio_mock.c
  ${CMAKE_CURRENT_SOURCE_DIR}/zpal_retention_register_mock.c
  ${CMAKE_CURRENT_SOURCE_DIR}/zpal_watchdog_mock.c
)
target_link_libraries(zpal_mock
  PUBLIC
    Assert
    mock
    zpal
)
target_sources(zpal_mock
  INTERFACE
    $<TARGET_OBJECTS:zpal_common>
)

add_mock(zpal_cmock
  ${ZPAL_API_DIR}/zpal_bootloader.h
  ${ZPAL_API_DIR}/zpal_entropy.h
  ${ZPAL_API_DIR}/zpal_init.h
  ${ZPAL_API_DIR}/zpal_misc.h
  ${ZPAL_API_DIR}/zpal_nvm.h
  ${ZPAL_API_DIR}/zpal_power_manager.h
  ${ZPAL_API_DIR}/zpal_radio.h
  ${ZPAL_API_DIR}/zpal_radio_utils.h
  ${ZPAL_API_DIR}/zpal_retention_register.h
  ${ZPAL_API_DIR}/zpal_status.h
  ${ZPAL_API_DIR}/zpal_uart.h
  ${ZPAL_API_DIR}/zpal_watchdog.h
)
