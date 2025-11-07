# SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
# SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
# SPDX-License-Identifier: BSD-3-Clause
zw_add_interface_cc(NAME CC_Association
  INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/config
  SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/association_plus.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/CC_Association.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/CC_AssociationGroupInfo.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/CC_MultiChanAssociation.c
  DEPENDS
    SwTimer
  CONFIG_KEY
    zw_cc_agi
  CONFIG_TEMPLATES
    cc_agi_config.c.jinja
)

add_test_subdirectory(mocks)
add_test_subdirectory(tests)

