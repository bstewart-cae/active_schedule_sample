# SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
# SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
# SPDX-License-Identifier: BSD-3-Clause
zw_add_object_cc(NAME CC_CentralScene
  INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/config
  SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/CC_CentralScene.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cc_central_scene_io_weak.c
  DEPENDS
    CC_Association
    ZAF_TSE_weak
  CONFIG_KEY
    zw_cc_central_scene
  CONFIG_TEMPLATES
    cc_central_scene_config.c.jinja
)

zw_add_object_cc(NAME CC_CentralScene_nvm
  INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
  SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cc_central_scene_nvm.c
)

add_test_subdirectory (tests)
add_test_subdirectory (mocks)
