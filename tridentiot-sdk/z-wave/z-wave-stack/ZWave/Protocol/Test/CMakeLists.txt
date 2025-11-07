# SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
#
# SPDX-License-Identifier: BSD-3-Clause

include_directories(
  "${ZW_ROOT}/ZWave/Protocol"
  "${ZW_ROOT}/ZWave"
  ${ZW_ROOT}/ZWave/EndDevice
  "${ZWAVE_API_DIR}"
  "${ZWAVE_CONFIG_DIR}"
)

add_library(TestTransmitBase OBJECT)
target_sources(TestTransmitBase
  PRIVATE
    ${ZW_ROOT}/ZWave/ZW_DataLinkLayer_utils.c
    ${ZW_ROOT}/ZWave/Protocol/ZW_DataLinkLayer.c
    ${ZW_ROOT}/ZWave/Protocol/ZW_Frame.c
    ${ZW_ROOT}/ZWave/Protocol/ZW_Channels.c
    ${ZW_ROOT}/ZWave/ZW_dynamic_tx_power.c
    ${ZW_ROOT}/ZWave/ZW_dynamic_tx_power_algorithm.c
    ${ZW_ROOT}/ZWave/mocks/ZW_noise_detect_mock.c
    ${ZW_ROOT}/ZWave/mocks/ZW_home_id_hash_mock.c
)
target_link_libraries(TestTransmitBase
  PUBLIC
    AssertTest
    Utils
    zpal
)
set(TEST_TRANSMIT_COMMON_LIBS
  mock
  TestTransmitBase
  zpal_mock
)

add_unity_test(NAME TestTransmitSingleCastLinkLayer     FILES TestTransmitSingleCastLinkLayer.c                             LIBRARIES ${TEST_TRANSMIT_COMMON_LIBS})
add_unity_test(NAME TestTransmitExplorerFramesLinkLayer FILES TestTransmitExplorerFramesLinkLayer.c                         LIBRARIES ${TEST_TRANSMIT_COMMON_LIBS})
add_unity_test(NAME TestTransmitAckLinkLayer            FILES TestTransmitAckLinkLayer.c                                    LIBRARIES ${TEST_TRANSMIT_COMMON_LIBS})
add_unity_test(NAME TestTransmitRoutedFramesLinkLayer   FILES TestTransmitRoutedFramesLinkLayer.c                           LIBRARIES ${TEST_TRANSMIT_COMMON_LIBS})
add_unity_test(NAME TestTransmitMulticastLinkLayer      FILES TestTransmitMulticastLinkLayer.c                              LIBRARIES ${TEST_TRANSMIT_COMMON_LIBS})
add_unity_test(NAME TestReceiveFilterLinkLayer          FILES TestReceiveFilterLinkLayer.c                                  LIBRARIES ${TEST_TRANSMIT_COMMON_LIBS})
add_unity_test(NAME Testreceivefilter_learnmode         FILES ${ZW_ROOT}/ZWave/Protocol/ZW_receivefilter_learnmode.c Testreceivefilter_learnmode.c LIBRARIES ${TEST_TRANSMIT_COMMON_LIBS})
add_unity_test(NAME Testreceivefilter_transport         FILES ${ZW_ROOT}/ZWave/Protocol/ZW_receivefilter_transport.c Testreceivefilter_transport.c LIBRARIES ${TEST_TRANSMIT_COMMON_LIBS})
#add_unity_test(NAME TestLinkLayer FILES TestLinkLayer.c)

add_unity_test(NAME TestTransmitSingleCastLinkLayer_3ch     FILES TestTransmitSingleCastLinkLayer_3ch.c     LIBRARIES ${TEST_TRANSMIT_COMMON_LIBS})
add_unity_test(NAME TestReceiveFilterLinkLayer_3ch          FILES TestReceiveFilterLinkLayer_3ch.c          LIBRARIES ${TEST_TRANSMIT_COMMON_LIBS})
add_unity_test(NAME TestTransmitExplorerFramesLinkLayer_3ch FILES TestTransmitExplorerFramesLinkLayer_3ch.c LIBRARIES ${TEST_TRANSMIT_COMMON_LIBS})
add_unity_test(NAME TestTransmitAckLinkLayer_3ch            FILES TestTransmitAckLinkLayer_3ch.c            LIBRARIES ${TEST_TRANSMIT_COMMON_LIBS})
add_unity_test(NAME TestTransmitRoutedFramesLinkLayer_3ch   FILES TestTransmitRoutedFramesLinkLayer_3ch.c   LIBRARIES ${TEST_TRANSMIT_COMMON_LIBS})
add_unity_test(NAME TestTransmitMulticastLinkLayer_3ch      FILES TestTransmitMulticastLinkLayer_3ch.c      LIBRARIES ${TEST_TRANSMIT_COMMON_LIBS})
add_unity_test(NAME TestReceiveFramesLinkLayer_3ch          FILES TestReceiveFramesLinkLayer_3ch.c          LIBRARIES ${TEST_TRANSMIT_COMMON_LIBS})


add_unity_test(NAME TestTransmitAckLinkLayer_LR  
  FILES 
    ${ZW_ROOT}/ZWave/Protocol/ZW_Frame.c 
    ${ZW_ROOT}/ZWave/ZW_DataLinkLayer_utils.c
    ${ZW_ROOT}/ZWave/Protocol/ZW_DataLinkLayer.c 
    ${ZW_ROOT}/ZWave/Protocol/ZW_Frame.c
    ${ZW_ROOT}/ZWave/ZW_home_id_hash.c
    ${ZW_ROOT}/ZWave/ZW_dynamic_tx_power.c 
    ${ZW_ROOT}/ZWave/ZW_dynamic_tx_power_algorithm.c
    ${ZW_ROOT}/ZWave/mocks/ZW_noise_detect_mock.c 
    ${ZW_ROOT}/ZWave/Protocol/mocks/ZW_Channels_mock.c
    TestTransmitAckLinkLayer_LR.c  
  LIBRARIES 
    mock
    AssertTest
    zpal_mock
    Utils
    SyncEvent
)
set_target_properties(TestTransmitAckLinkLayer_LR PROPERTIES COMPILE_DEFINITIONS "ZW_SLAVE")

add_unity_test(NAME TestRetransmitSingleCastLinkLayer_LR
  FILES
    ${ZW_ROOT}/ZWave/Protocol/ZW_Frame.c
    ${ZW_ROOT}/ZWave/ZW_DataLinkLayer_utils.c
    ${ZW_ROOT}/ZWave/Protocol/ZW_DataLinkLayer.c
    ${ZW_ROOT}/ZWave/Protocol/ZW_Frame.c
    ${ZW_ROOT}/ZWave/ZW_home_id_hash.c
    ${ZW_ROOT}/ZWave/ZW_dynamic_tx_power.c
    ${ZW_ROOT}/ZWave/ZW_dynamic_tx_power_algorithm.c
    ${ZW_ROOT}/ZWave/mocks/ZW_noise_detect_mock.c
    ${ZW_ROOT}/ZWave/Protocol/mocks/ZW_Channels_mock.c
    TestRetransmitSingleCastLinkLayer_LR.c
  LIBRARIES
    mock
    AssertTest
    zpal_mock
    Utils
    SyncEvent
)
set_target_properties(TestRetransmitSingleCastLinkLayer_LR PROPERTIES COMPILE_DEFINITIONS "ZW_SLAVE")
