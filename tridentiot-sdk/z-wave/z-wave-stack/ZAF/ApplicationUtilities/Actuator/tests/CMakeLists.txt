# SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
#
# SPDX-License-Identifier: BSD-3-Clause

set(test_ZAF_Actuator_src
  test_ZAF_Actuator.c
  ../ZAF_Actuator.c
)

add_unity_test(NAME test_ZAF_Actuator
               FILES ${test_ZAF_Actuator_src}
               LIBRARIES mock
                         test_common
                         NodeMask
                         Utils
                         SwTimerMock
                         AppTimerMock
                         DebugPrintMock
                         AssertTest
                         m
)

target_include_directories(test_ZAF_Actuator
  PUBLIC
  #nothing
  PRIVATE
  ..
)