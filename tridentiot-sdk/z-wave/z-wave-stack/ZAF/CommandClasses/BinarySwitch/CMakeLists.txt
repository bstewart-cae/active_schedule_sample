# SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
# SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
# SPDX-License-Identifier: BSD-3-Clause
zw_add_object_cc(NAME CC_BinarySwitch
  INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/config
    ${ZAF_CONFIGDIR}/inc
  SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/CC_BinarySwitch.c
  DEPENDS
    ZAF_Actuator
    ZAF_TSE_weak
    CC_Supervision
  CONFIG_KEY
    zw_cc_binaryswitch
  CONFIG_TEMPLATES
    cc_binary_switch_config.c.jinja
)

add_test_subdirectory(tests)
add_test_subdirectory(mocks)
