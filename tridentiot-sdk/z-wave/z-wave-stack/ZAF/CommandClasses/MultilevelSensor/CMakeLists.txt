# SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
# SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
# SPDX-License-Identifier: BSD-3-Clause
zw_add_object_cc(NAME CC_MultilevelSensor
  INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/config
    ${CMAKE_CURRENT_SOURCE_DIR}/src
  SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/CC_MultilevelSensor_SensorHandler.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/CC_MultilevelSensor_SensorHandlerTypes.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/CC_MultilevelSensor_Support.c
  DEPENDS
    SwTimer
    CC_Supervision
    ZAF_TSE_weak
  CONFIG_KEY
    zw_cc_multilevel_sensor
  CONFIG_TEMPLATES
    cc_multilevel_sensor_config.c.jinja
    cc_multilevel_sensor_config.h.jinja
)

add_test_subdirectory(tests)
