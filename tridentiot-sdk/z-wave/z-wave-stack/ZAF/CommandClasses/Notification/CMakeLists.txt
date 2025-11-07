# SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
# SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
# SPDX-License-Identifier: BSD-3-Clause
zw_add_object_cc(NAME CC_Notification
  INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/config
    ${CMAKE_CURRENT_SOURCE_DIR}/src
  SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/CC_Notification.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cc_notification_io_weak.c
  DEPENDS
    CC_Association
    ZAF_TSE_weak
  CONFIG_KEY
    zw_cc_notification
  CONFIG_TEMPLATES
    cc_notification_config.c.jinja
)

zw_add_object_cc(NAME CC_Notification_nvm
  INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/config
    ${CMAKE_CURRENT_SOURCE_DIR}/src
  SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cc_notification_nvm.c
)

add_test_subdirectory(tests)
add_test_subdirectory(mocks)
