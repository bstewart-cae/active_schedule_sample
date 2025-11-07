# SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
# SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
# SPDX-License-Identifier: BSD-3-Clause
zw_add_interface_cc(NAME CC_MultiChannelControl
  INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
  SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/multichannel.c
)

zw_add_interface_cc(NAME CC_MultiChannelSupport
  INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}/config
    ${CMAKE_CURRENT_SOURCE_DIR}/inc/
    ${CMAKE_CURRENT_SOURCE_DIR}/src/
  SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/CC_MultiChan.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cc_multi_channel_config_api.c
  CONFIG_KEY
    zw_cc_multi_channel
  CONFIG_TEMPLATES
    cc_multi_channel_config.c.jinja
)

add_test_subdirectory(mocks)
add_test_subdirectory(tests)
