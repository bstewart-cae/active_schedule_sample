# SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
#
# SPDX-License-Identifier: BSD-3-Clause

add_library(ZAF_ActuatorMock ZAF_Actuator_mock.c)
target_include_directories(ZAF_ActuatorMock PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(ZAF_ActuatorMock
  AppTimerMock
  DebugPrintMock
  mock)

add_mock(ZAF_Actuator_cmock ${ZW_ROOT}/ZAF/ApplicationUtilities/Actuator/ZAF_Actuator.h)
target_include_directories(ZAF_Actuator_cmock
  PUBLIC
    ${COMPONENTS_DIR}/SwTimer
)