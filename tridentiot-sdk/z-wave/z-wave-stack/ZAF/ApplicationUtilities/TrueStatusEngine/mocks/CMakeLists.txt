# SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
#
# SPDX-License-Identifier: BSD-3-Clause

add_library(ZAF_TSEMock ZAF_TSE_mock.c)
target_include_directories(ZAF_TSEMock PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/..
  ${ZWAVE_API_DIR}
  ${ZAF_UTILDIR}
  ${ZAF_CCDIR}/Common
)

target_link_libraries(ZAF_TSEMock
  mock
  zpal
  zaf_transport_layer_cmock
)

add_mock(ZAF_TSE_cmock ${ZW_ROOT}/ZAF/ApplicationUtilities/TrueStatusEngine/ZAF_TSE.h)
target_include_directories(ZAF_TSE_cmock PUBLIC ${ZW_ROOT}/ZAF/ApplicationUtilities
                           ${ZW_ROOT}/ZWave/API
                           ${ZW_ROOT}/PAL/inc
                           ${ZW_ROOT}/ZAF/CommandClasses/MultiChan
                          ${ZW_ROOT}/ZAF/ApplicationUtilities/_commonIF
                          ${ZW_ROOT}/ZAF/CommandClasses/Common)
target_link_libraries(ZAF_TSE_cmock
  PUBLIC
    zaf_transport_layer_cmock
)
