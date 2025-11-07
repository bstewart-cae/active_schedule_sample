# SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
# SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
# SPDX-License-Identifier: BSD-3-Clause
zw_add_interface_cc(NAME CC_DoorLock
  INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/config
    ${CMAKE_CURRENT_SOURCE_DIR}/src
  SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/CC_DoorLock.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cc_door_lock_config_api.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cc_door_lock_io_weak.c
  CONFIG_KEY
    zw_cc_door_lock
  CONFIG_TEMPLATES
    cc_door_lock_config.h.jinja
)

zw_add_object_cc(NAME CC_DoorLock_nvm
  INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/config
    ${CMAKE_CURRENT_SOURCE_DIR}/src
  SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cc_door_lock_nvm.c
  DEPENDS
    CC_Association
)

add_test_subdirectory(mocks)
add_test_subdirectory(tests)
