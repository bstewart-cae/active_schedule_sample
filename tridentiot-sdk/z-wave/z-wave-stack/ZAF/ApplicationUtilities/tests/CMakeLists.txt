# SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
#
# SPDX-License-Identifier: BSD-3-Clause

add_compile_definitions(ZW_SLAVE)

################################################################################
# Add test for Transport Multicast
################################################################################
add_compile_definitions(REQUESTED_SECURITY_KEYS=0x00)

set(test_ZW_TransportMulticast_src
  test_ZW_TransportMulticast.c
  "${CMAKE_CURRENT_SOURCE_DIR}/../mocks/ZW_TransportSecProtocol_mock.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/../mocks/mock_ZW_TransportEndpoint.c"
  "${ZWAVE_MOCKS_DIR}/ZW_keystore_mock.c"
  "${ZWAVE_MOCKS_DIR}/ZW_mem_mock.c"
  "${ZAF_CCDIR}/Association/mocks/mock_association_plus.c"
  "${ZAF_UTILDIR}/_commonIF/ZAF_Common_interface.c"
  "${ZAF_CCDIR}/Supervision/src/CC_Supervision.c"
  ${ZAF_CCDIR}/Supervision/src/cc_supervision_config_api.c
  ${ZAF_UTILDIR}/ZAF_CC_Invoker.c
  "${ZAF_CCDIR}/MultiChan/mocks/multichannel_mock.c"
  )
add_unity_test(NAME test_ZW_TransportMulticast
  FILES
    "${test_ZW_TransportMulticast_src}"
  LIBRARIES
    mock
    QueueNotifyingMock
    SwTimerMock
    NodeMask
    Utils
    zpal
    AssertTest
    ZAF_TSEMock
    ZAF_CmdPublisher_Mock
    zpal_mock
    AppTimerMock
    AppsHwMock
    zaf_protocol_config_cmock
    zaf_event_distributor_soc_cmock
)
target_compile_definitions(test_ZW_TransportMulticast PRIVATE
  test_ZW_TransportMulticast_defines
  ZAF_CONFIG_NUMBER_OF_END_POINTS=4
  CC_ASSOCIATION_MAX_GROUPS_PER_ENDPOINT=3
  CC_ASSOCIATION_MAX_NODES_IN_GROUP=5
)
target_include_directories(test_ZW_TransportMulticast
  PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/.."
    "${ZAF_CCDIR}/Association/inc"
    "${ZAF_CCDIR}/Common"
    "${ZAF_UNITTESTEXTERNALS}"
    "${ZAF_UTILDIR}/_commonIF"
    "${ZAF_UTILDIR}/TargetSpecificIncludes"
    ${ZAF_CONFIGDIR}/config
    ${ZAF_CCDIR}/Association/config
    ${ZAF_CCDIR}/Supervision/inc
    ${ZAF_CCDIR}/Supervision/config
    "${ZAF_CCDIR}/MultiChan/inc"
)

#[[
################################################################################
# Add test for Transport Sec Protocol
################################################################################
set(test_ZW_TransportSecProtocol_src
  test_ZW_TransportSecProtocol.c
  "${CMAKE_CURRENT_SOURCE_DIR}/../mocks/ZW_TransportSecProtocol_mock_extern.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/../mocks/mock_ZW_TransportEndpoint.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/../mocks/mock_ZW_TransportMulticast.c"
  "${ZAF_CCDIR}/MultiChan/mocks/multichannel_mock.c"
  "${ZWAVE_MOCKS_DIR}/ZW_keystore_mock.c"
  "${ZWAVE_MOCKS_DIR}/ZW_mem_mock.c"
  )
add_unity_test(NAME test_ZW_TransportSecProtocol
  FILES
    "${test_ZW_TransportSecProtocol_src}"
  LIBRARIES
    mock
    QueueNotifyingMock
)
target_include_directories(test_ZW_TransportSecProtocol
  PUBLIC
    "${ZAF_CCDIR}"
    "${ZAF_UNITTESTEXTERNALS}"
    "${ZWAVE_API_DIR}"
)
]]

################################################################################
# Add test for Transport endpoint
################################################################################
set(test_ZW_TransportEndpoint_src
  test_ZW_TransportEndpoint.c
  "${CMAKE_CURRENT_SOURCE_DIR}/../mocks/ZW_TransportSecProtocol_mock.c"
  "${ZAF_CCDIR}/MultiChan/mocks/multichannel_mock.c"
  "${ZWAVE_MOCKS_DIR}/ZW_keystore_mock.c"
  ${ZAF_UTILDIR}/ZAF_CC_Invoker.c
)
add_unity_test(NAME test_ZW_TransportEndpoint
  FILES
    "${test_ZW_TransportEndpoint_src}"
  LIBRARIES
    mock
    QueueNotifyingMock
    SwTimerMock
    zpal_mock
    zaf_common_helper_cmock
    zaf_cc_list_generator_cmock
    zaf_config_api_cmock
    zaf_transport_layer_cmock
  USE_UNITY_WITH_CMOCK
)
target_include_directories(test_ZW_TransportEndpoint
  PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/.."
    "${ZAF_CCDIR}/Common"
    "${ZAF_CCDIR}/MultiChan/inc"
    "${ZAF_UNITTESTEXTERNALS}"
)
set_target_properties(test_ZW_TransportEndpoint PROPERTIES COMPILE_DEFINITIONS "ZAF_CONFIG_NUMBER_OF_END_POINTS=2")
target_link_libraries(test_ZW_TransportEndpoint mock ZAF_CommonInterfaceMock)

################################################################################
# Add test for Command Class utils functionality
################################################################################

set(test_ZAF_command_class_utils_src
  test_ZAF_command_class_utils.cpp
  "${CMAKE_CURRENT_SOURCE_DIR}/../ZAF_command_class_utils.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/../mocks/ZW_TransportSecProtocol_mock.c"
)
add_unity_test(NAME test_ZAF_command_class_utils
  FILES
    "${test_ZAF_command_class_utils_src}"
  LIBRARIES
    mock
    QueueNotifyingMock
    ZAF_CommonInterfaceMock
    zaf_protocol_config_cmock
    USE_CPP
)

target_include_directories(test_ZAF_command_class_utils PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}/.."
  "${ZAF_CCDIR}/Common"
  "${ZAF_UTILDIR}/mocks/inc"
)

################################################################################
# Add test for Association Data Store
################################################################################

set(test_ZAF_AssociationDataStore_src
  test_ZAF_AssociationDataStore.c
  "${CMAKE_CURRENT_SOURCE_DIR}/../ZAF_AssociationDataStore.c"
)
add_unity_test(NAME test_ZAF_AssociationDataStore
  FILES
    "${test_ZAF_AssociationDataStore_src}"
  LIBRARIES
    mock
    AssertTest
)

target_include_directories(test_ZAF_AssociationDataStore
  PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/.."
    ${ZWAVE_API_DIR}
)

################################################################################
# Add test for AppTimer.c
################################################################################

set(test_AppTimer_src
  ${ZAF_UTILDIR}/AppTimer.c
  ${ZAF_UTILDIR}/AppTimerDeepSleep.c
  test_AppTimer.c
  ${ZWAVE_MOCKS_DIR}/mock_system_startup.c
)
add_unity_test(NAME test_AppTimer
  FILES
    ${test_AppTimer_src}
  LIBRARIES
    mock
    QueueNotifyingMock
    SwTimerMock
    zpal_mock
    NodeMask
    DebugPrintMock
    AssertTest
    Utils
)
target_include_directories(test_AppTimer
  PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

################################################################################
# Add test for Security Discovery Cmd Publisher
################################################################################

set(test_ZAF_CmdPublisher_src
  test_ZAF_CmdPublisher.c
  "${CMAKE_CURRENT_SOURCE_DIR}/../ZAF_CmdPublisher.c"
)

add_unity_test(NAME test_ZAF_CmdPublisher
  FILES
    "${test_ZAF_CmdPublisher_src}"
  LIBRARIES
    QueueNotifyingMock
    NodeMask
    AssertTest
)

target_include_directories(test_ZAF_CmdPublisher
  PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

################################################################################
# Add test for ZAF_file_ids.h
################################################################################

set(test_ZAF_file_ids_src
  test_ZAF_file_ids.c
)

add_unity_test(NAME test_ZAF_file_ids
  FILES
    ${test_ZAF_file_ids_src}
)

target_include_directories(test_ZAF_file_ids
  PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/.."
)

################################################################################
# Add test for network management commands
################################################################################

set(test_ZAF_network_management_src
  test_ZAF_network_management.cpp
  ../ZAF_network_management.c
)
add_unity_test(NAME test_ZAF_network_management FILES ${test_ZAF_network_management_src}
               LIBRARIES mock
                         QueueNotifyingMock
                         ZAF_CommonInterfaceMock
                         zpal
               USE_CPP
)
target_include_directories(test_ZAF_network_management PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/.."
)
