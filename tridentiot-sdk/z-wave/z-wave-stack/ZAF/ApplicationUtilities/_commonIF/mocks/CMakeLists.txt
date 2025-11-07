# SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
#
# SPDX-License-Identifier: BSD-3-Clause

add_library(ZAF_CommonInterfaceMock ZAF_Common_interface_mock.c)

target_include_directories(ZAF_CommonInterfaceMock PUBLIC
 "${CMAKE_CURRENT_SOURCE_DIR}/.."
  ${ZAF_UTILDIR}
)

target_link_libraries(ZAF_CommonInterfaceMock
  PUBLIC
    mock
    QueueNotifyingMock
    ZAF_CmdPublisher_Mock
    AssertTest
)

add_mock(zaf_common_helper_cmock ${ZAF_UTILDIR}/_commonIF/ZAF_Common_helper.h)
target_include_directories(zaf_common_helper_cmock
  PUBLIC
  ${ZWAVE_API_DIR}
)
target_link_libraries(zaf_common_helper_cmock
  PUBLIC
  Assert_cmock
  FreeRTOS_cmock
  NodeMask_cmock
  QueueNotifying_cmock
  PRIVATE
  zpal
)

add_mock(ZAF_Common_interface_cmock ${ZAF_UTILDIR}/_commonIF/ZAF_Common_interface.h)
target_include_directories(ZAF_Common_interface_cmock
  PUBLIC
  ${ZAF_UTILDIR}
  ${ZWAVE_API_DIR}
)
target_link_libraries(ZAF_Common_interface_cmock
  PUBLIC
  Assert_cmock
  FreeRTOS_cmock
  NodeMask_cmock
  QueueNotifying_cmock
  PRIVATE
  zpal  
)
