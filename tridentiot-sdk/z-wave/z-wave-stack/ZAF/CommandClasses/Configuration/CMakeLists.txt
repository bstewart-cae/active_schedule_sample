# SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
# SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
# SPDX-License-Identifier: BSD-3-Clause
zw_add_object_cc(NAME CC_Configuration
  INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/src
  SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/CC_Configuration.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cc_configuration_io.c
  CONFIG_KEY
    zw_cc_configuration
  CONFIG_TEMPLATES
    cc_configuration_config.c.jinja
)

add_test_subdirectory(mocks)
add_test_subdirectory(tests)
