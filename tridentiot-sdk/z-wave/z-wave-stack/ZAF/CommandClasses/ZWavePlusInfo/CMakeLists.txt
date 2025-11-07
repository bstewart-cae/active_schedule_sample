# SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
# SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
# SPDX-License-Identifier: BSD-3-Clause
zw_add_interface_cc(NAME CC_ZWavePlusInfo
  INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
  SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/CC_ZWavePlusInfo.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cc_zwave_plus_info_config_api.c
  CONFIG_KEY
    zw_cc_zwaveplusinfo
  CONFIG_TEMPLATES
    cc_zwaveplusinfo_config.c.jinja
)

add_test_subdirectory (tests)
