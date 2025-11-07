# SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
#
# SPDX-License-Identifier: BSD-3-Clause

add_library(AppTimerMock AppTimer_mock.c)
target_include_directories(AppTimerMock PUBLIC
  ../
  ${ZWAVE_API_DIR}
)
target_link_libraries(AppTimerMock
  SwTimerMock
  zpal
)

add_library(ZAF_ADS_Mock ZAF_AssociationDataStore_mock.c)
target_link_libraries(ZAF_ADS_Mock
  mock
)
target_include_directories(ZAF_ADS_Mock PUBLIC
  ..
  ${ZWAVE_API_DIR}
)

add_library(ZAF_CmdPublisher_Mock ZAF_CmdPublisher_mock.c)
target_link_libraries(ZAF_CmdPublisher_Mock
  PUBLIC
    QueueNotifyingMock # Required because ZW_application_transport_interface.h includes QueueNotifying.h
    NodeMask # Required because ZW_application_transport_interface.h includes NodeMask.h
  PRIVATE
    mock
)
target_include_directories(ZAF_CmdPublisher_Mock PUBLIC
  ..
)

add_mock(ZAF_nvm_app_cmock
  ${ZW_ROOT}/ZAF/ApplicationUtilities/ZAF_nvm.h
  ${ZW_ROOT}/ZAF/ApplicationUtilities/ZAF_nvm_app.h
)
target_link_libraries(ZAF_nvm_app_cmock
  PRIVATE
    zpal
)

add_mock(AppTimer_cmock ${ZAF_UTILDIR}/AppTimer.h)
target_include_directories(AppTimer_cmock PUBLIC
  ${ZWAVE_API_DIR}
  ${COMPONENTS_DIR}/SwTimer
)
target_link_libraries(AppTimer_cmock
  PRIVATE
  zpal
)

add_mock(ZAF_network_learn_cmock ${ZAF_UTILDIR}/ZAF_network_learn.h)
target_include_directories(ZAF_network_learn_cmock PUBLIC
  ${ZWAVE_API_DIR}
)
target_link_libraries(ZAF_network_learn_cmock
  PRIVATE
  zpal
)

add_mock(ZW_TransportSecProtocol_cmock ${ZAF_UTILDIR}/ZW_TransportSecProtocol.h)
target_include_directories(ZW_TransportSecProtocol_cmock PUBLIC
  ${ZWAVE_API_DIR}
)
target_link_libraries(ZW_TransportSecProtocol_cmock
  PUBLIC
  Assert_cmock
  FreeRTOS_cmock
  NodeMask_cmock
  QueueNotifying_cmock
  PRIVATE
  zpal
)

add_mock(ZAF_CmdPublisher_cmock ${ZAF_UTILDIR}/ZAF_CmdPublisher.h)


add_mock(ZAF_PrintAppInfo_cmock ${ZAF_UTILDIR}/ZAF_PrintAppInfo.h)
target_link_libraries(ZAF_PrintAppInfo_cmock
  PUBLIC
  zpal
)
target_include_directories(ZAF_PrintAppInfo_cmock PUBLIC
  ..
  ${ZWAVE_API_DIR}
)

add_mock(ZAF_CC_Invoker_cmock ${ZAF_UTILDIR}/ZAF_CC_Invoker.h)
target_include_directories(ZAF_CC_Invoker_cmock PUBLIC
  ${ZWAVE_API_DIR}
)
target_link_libraries(ZAF_CC_Invoker_cmock
  PRIVATE
    zpal
)

add_mock(ZW_TransportEndpoint_cmock ${ZW_ROOT}/ZAF/ApplicationUtilities/ZW_TransportEndpoint.h)
target_include_directories(ZW_TransportEndpoint_cmock PUBLIC 
                           ${ZW_ROOT}/PAL/inc
                           ${ZW_ROOT}/ZWave/API
                           ${ZW_ROOT}/ZAF/ApplicationUtilities)

add_mock(ZW_TransportMulticast_cmock ${ZAF_UTILDIR}/ZW_TransportMulticast.h)
target_include_directories(ZW_TransportMulticast_cmock PUBLIC
  ${ZWAVE_API_DIR}
)
target_link_libraries(ZW_TransportMulticast_cmock
  PUBLIC
  Assert_cmock
  FreeRTOS_cmock
  NodeMask_cmock
  QueueNotifying_cmock
  PRIVATE
  zpal
)                           