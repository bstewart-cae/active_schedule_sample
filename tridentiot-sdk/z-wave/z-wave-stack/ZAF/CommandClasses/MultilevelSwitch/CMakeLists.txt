# SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
# SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
# SPDX-License-Identifier: BSD-3-Clause
zw_add_object_cc(NAME CC_MultilevelSwitchControl
  INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
  SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/CC_MultilevelSwitch_Control.c
  DEPENDS
    CC_Association
)

zw_add_object_cc(NAME CC_MultilevelSwitchSupport
  INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/config
  SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/CC_MultilevelSwitch_Support.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cc_multilevel_switch_support_io_weak.c
  DEPENDS
    ZAF_Actuator
    ZAF_TSE_weak
    CC_Supervision
  CONFIG_KEY
    zw_cc_multilevel_switch_support
  CONFIG_TEMPLATES
    cc_multilevel_switch_support_config.c.jinja
)

zw_add_object_cc(NAME CC_MultilevelSwitchSupport_nvm
  INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
  SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cc_multilevel_switch_support_nvm.c
  DEPENDS
    ZAF_Actuator
)

add_test_subdirectory (mocks)
add_test_subdirectory (tests)
