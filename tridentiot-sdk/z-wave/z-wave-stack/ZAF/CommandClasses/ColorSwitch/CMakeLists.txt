# SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
# SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
# SPDX-License-Identifier: BSD-3-Clause
zw_add_object_cc(NAME CC_ColorSwitch
  INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/config
  SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/CC_ColorSwitch.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cc_color_switch_io_weak.c
  DEPENDS
    ZAF_Actuator
    CC_Supervision
    ZAF_TSE_weak
  CONFIG_KEY
    zw_cc_color_switch
  CONFIG_TEMPLATES
    cc_color_switch_config.c.jinja
)

zw_add_object_cc(NAME CC_ColorSwitch_nvm
  INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/src
  SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cc_color_switch_nvm.c
  DEPENDS
    ZAF_Actuator
    CC_Supervision
)

add_test_subdirectory(mocks)
add_test_subdirectory(tests)
