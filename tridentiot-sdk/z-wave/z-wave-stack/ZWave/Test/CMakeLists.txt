# SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
#
# SPDX-License-Identifier: BSD-3-Clause

include_directories(${ZW_ROOT}/ZWave)
add_test_subdirectory(Controller)

set(PAL_PLATFORM_DIR ${ZW_ROOT}/platform/SiliconLabs/PAL)
################################################################################
## slave_network_info_storage unit test
################################################################################

add_unity_test(NAME TestZW_slave_network_info_storage
  FILES
    "${ZW_ROOT}/ZWave/EndDevice/ZW_slave_network_info_storage.c"
    TestZW_slave_network_info_storage.c
    "${ZW_ROOT}/ZWave/mocks/ZW_nvm_mock.c"
    "${ZW_ROOT}/ZWave/ZW_node.c"
    "${ZW_ROOT}/ZWave/mocks/ZW_NVMCaretaker_mock.c"
  LIBRARIES
    AssertTest
    mock
    SyncEvent
    Utils
    NodeMask
    zpal_mock
)
target_include_directories(TestZW_slave_network_info_storage
  PRIVATE
    "${SUBTREE_LIBS2}/include"
    "${ZW_ROOT}/ZWave/Protocol"
    "${ZW_ROOT}/ZWave"
    ${ZW_ROOT}/ZWave/EndDevice
    "${ZWAVE_CONFIG_DIR}"
)
set_target_properties(TestZW_slave_network_info_storage PROPERTIES COMPILE_DEFINITIONS "ZW_slave_lib;UNITY_TEST")
target_compile_definitions(TestZW_slave_network_info_storage PRIVATE ZWAVE_MIGRATE_FILESYSTEM)


################################################################################
## controller_network_info_storage unit test
################################################################################

add_unity_test(NAME  TestZW_controller_network_info_storage
  FILES
    "${ZW_ROOT}/ZWave/Controller/ZW_controller_network_info_storage.c"
    TestZW_controller_network_info_storage.c
    "${ZW_ROOT}/ZWave/mocks/ZW_nvm_mock.c"
    "${ZW_ROOT}/ZWave/mocks/ZW_NVMCaretaker_mock.c"
    "${ZW_ROOT}/ZWave/ZW_node.c"
  LIBRARIES
    AssertTest
    mock
    SyncEvent
    Utils
    NodeMask
    QueueNotifyingMock
    zpal_mock
)
target_include_directories(TestZW_controller_network_info_storage
  PRIVATE
    "${SUBTREE_LIBS2}/include"
    "${ZW_ROOT}/ZWave/Protocol"
    ${ZW_ROOT}/ZWave/Controller
    "${ZWAVE_CONFIG_DIR}"
)
set_target_properties(TestZW_controller_network_info_storage PROPERTIES COMPILE_DEFINITIONS "ZW_controller_lib;UNITY_TEST")
target_compile_definitions(TestZW_controller_network_info_storage PRIVATE ZWAVE_MIGRATE_FILESYSTEM ZW_SECURITY_PROTOCOL)


################################################################################
## ZW_nvm unit test
################################################################################


add_unity_test(NAME TestZW_nvm
  FILES
    "${ZW_ROOT}/ZWave/ZW_nvm.c"
    TestZW_nvm.c
  LIBRARIES
    AssertTest
    mock
    SyncEvent
    Utils
    zpal_mock
)
target_compile_definitions(TestZW_nvm
  PUBLIC
    ZW_slave_lib
    UNITY_TEST
)
target_include_directories(TestZW_nvm
  PRIVATE
    ${ZW_ROOT}/ZWave/EndDevice
    "${ZWAVE_CONFIG_DIR}"
)

################################################################################
## ZW_NVMCaretaker unit test
################################################################################

add_unity_test(NAME TestZW_NVMCaretaker
  FILES
    "${ZW_ROOT}/ZWave/ZW_NVMCaretaker.c"
    TestZW_NVMCaretaker.c
  LIBRARIES
    DebugPrintMock
    mock
    AssertTest
    zpal_mock
)

################################################
##   ZW_ctimer unit test
################################################

add_unity_test(NAME TestZW_ctimer
  FILES
    "${ZW_ROOT}/ZWave/ZW_ctimer.c"
    "${ZW_ROOT}/ZWave/linked_list.c"
    "${ZWAVE_MOCKS_DIR}/ZW_timer_mock.c"
    TestZW_ctimer.c
  LIBRARIES
    SwTimerMock
    QueueNotifyingMock
    NodeMask
)
target_include_directories(TestZW_ctimer
  PRIVATE
    "${ZWAVE_MOCKS_DIR}/sleep"
)

################################################
##   ZW_home_id_generator unit test
################################################
add_unity_test(NAME  TestZW_home_id_generator
  FILES
    "${ZW_ROOT}/ZWave/ZW_home_id_generator.c"
    TestZW_home_id_generator.c
  LIBRARIES
    mock
    zpal_mock
)
target_include_directories(TestZW_home_id_generator
  PUBLIC
    ${ZWAVE_API_DIR}
)

################################################
##   ZW_home_id_hash unit test
################################################
add_unity_test(NAME  TestZW_home_id_hash
  FILES
    "${ZW_ROOT}/ZWave/ZW_home_id_hash.c"
    TestZW_home_id_hash.c
  LIBRARIES
    mock
    zpal_mock
)
target_include_directories(TestZW_home_id_hash
  PUBLIC
    ${ZWAVE_API_DIR}
)

#####################################################
##   ZW_network_management  unit test for ZW_SLAVE
#####################################################
add_unity_test(NAME  TestZW_network_management_SLAVE
  FILES
    "${ZW_ROOT}/ZWave/ZW_explore_incl_req.c"
    "${ZW_ROOT}/ZWave/ZW_network_management.c"
    "${ZWAVE_MOCKS_DIR}/ZW_explore_mock.c"
    "${ZWAVE_MOCKS_DIR}/ZW_explore_excl_req_mock.c"
    "${ZWAVE_MOCKS_DIR}/ZW_keystore_mock.c"
    "${ZWAVE_MOCKS_DIR}/ZW_main_mock.c"
    "${ZWAVE_MOCKS_DIR}/ZW_main_region_mock.c"
    "${ZWAVE_MOCKS_DIR}/ZW_protocol_interface_mock.c"
    "${ZWAVE_MOCKS_DIR}/ZW_secure_learn_support_mock.c"
    "${ZWAVE_MOCKS_DIR}/ZW_slave_mock.c"
    "${ZWAVE_MOCKS_DIR}/ZW_slave_network_info_storage_mock.c"
    "${ZWAVE_MOCKS_DIR}/ZW_transport_transmit_cb_mock.c"
    "${ZWAVE_MOCKS_DIR}/ZW_transport_mock.c"
    "${ZWAVE_MOCKS_DIR}/ZW_timer_mock.c"
    "${ZWAVE_MOCKS_DIR}/mock_system_startup.c"
    "${ZW_ROOT}/ZWave/Protocol/mocks/ZW_DataLinkLayer_mock.c"
    TestZW_network_management_SLAVE.c
  LIBRARIES
    mock
    QueueNotifyingMock
    SwTimerMock
    NodeMask
    zpal_mock
    AssertTest
    DebugPrintMock
    Utils
    SyncEvent
)
target_compile_definitions(TestZW_network_management_SLAVE
  PRIVATE
    ZW_slave_lib
)
target_include_directories(TestZW_network_management_SLAVE
  PRIVATE
    "${ZW_ROOT}/ZWave/Protocol"
    ${ZW_ROOT}/ZWave/EndDevice
    "${ZWAVE_CONFIG_DIR}"
    "${SUBTREE_LIBS2}/include"
)


#####################################################
##   ZW_frames_filter  unit test for ZW_CONTROLLER
#####################################################
add_unity_test(NAME  TestZW_frames_filters_CONTROLLER
  FILES
    "${ZW_ROOT}/ZWave/Protocol/ZW_Frame.c"
    "${ZW_ROOT}/ZWave/ZW_frames_filters.c"
    "${ZW_ROOT}/ZWave/Protocol/ZW_Frame.c"
    "${ZWAVE_MOCKS_DIR}/ZW_controller_mock.c"
    "${ZW_ROOT}/ZWave/Protocol/mocks/ZW_DataLinkLayer_mock.c"
    TestZW_frames_filters_CONTROLLER.c
    LIBRARIES
    QueueNotifyingMock
    SwTimerMock
    mock
    NodeMask
    zpal
    AssertTest
    SyncEvent
)
target_compile_definitions(TestZW_frames_filters_CONTROLLER
    PRIVATE
    ZW_CONTROLLER
    ZW_CONTROLLER_STATIC
    ZW_CONTROLLER_BRIDGE
)
target_include_directories(TestZW_frames_filters_CONTROLLER
  PRIVATE
    "${ZW_ROOT}/ZWave/Protocol"
    "${ZWAVE_CONFIG_DIR}"
    "${SUBTREE_LIBS2}/include"
    ${ZW_ROOT}/ZWave/Controller
)


#####################################################
##   ZW_frames_filter  unit test for ZW_SLAVE
#####################################################
add_unity_test(NAME  TestZW_frames_filters_SLAVE
  FILES
    "${ZW_ROOT}/ZWave/Protocol/ZW_Frame.c"
    "${ZW_ROOT}/ZWave/ZW_frames_filters.c"
    "${ZW_ROOT}/ZWave/ZW_lr_virtual_node_id.c"
    "${ZW_ROOT}/ZWave/Protocol/ZW_Frame.c"
    "${ZWAVE_MOCKS_DIR}/ZW_slave_mock.c"
    "${ZW_ROOT}/ZWave/Protocol/mocks/ZW_DataLinkLayer_mock.c"
    TestZW_frames_filters_SLAVE.c
  LIBRARIES
    QueueNotifyingMock
    SwTimerMock
    mock
    NodeMask
    zpal
    AssertTest
    SyncEvent
)
target_compile_definitions(TestZW_frames_filters_SLAVE
    PRIVATE
    ZW_SECURITY_PROTOCOL
    ZW_SLAVE
)
target_include_directories(TestZW_frames_filters_SLAVE
  PRIVATE
  "${ZW_ROOT}/ZWave/Protocol"
  "${ZWAVE_CONFIG_DIR}"
  "${SUBTREE_LIBS2}/include"
  ${ZW_ROOT}/ZWave/EndDevice
)
#####################################################
##   ZW_ismyframe  unit test for ZW_CONTROLLER
#####################################################
add_unity_test(NAME  TestZW_ismyframe_CONTROLLER
  FILES
    "${ZW_ROOT}/ZWave/Protocol/ZW_Frame.c"
    "${ZW_ROOT}/ZWave/ZW_ismyframe.c"
    "${ZW_ROOT}/ZWave/ZW_node.c"
    "${ZW_ROOT}/ZWave/Protocol/ZW_Frame.c"
    "${ZWAVE_MOCKS_DIR}/ZW_txq_protocol_mock.c"
    "${ZWAVE_MOCKS_DIR}/ZW_transport_mock.c"
    "${ZWAVE_MOCKS_DIR}/ZW_controller_mock.c"
    "${ZWAVE_MOCKS_DIR}/ZW_controller_network_info_storage_mock.c"
    "${ZWAVE_MOCKS_DIR}/ZW_tx_queue_mock.c"
    TestZW_ismyframe_CONTROLLER.c
    LIBRARIES
    QueueNotifyingMock
    NodeMask
    SwTimerMock
    zpal_mock
    AssertTest
    SyncEvent
)
target_compile_definitions(TestZW_ismyframe_CONTROLLER
  PRIVATE
    ZW_controller_lib
)

target_include_directories(TestZW_ismyframe_CONTROLLER
  PRIVATE
    "${ZW_ROOT}/ZWave"
    ${ZW_ROOT}/ZWave/Controller
    "${ZW_ROOT}/ZWave/Protocol"
    "${ZWAVE_CONFIG_DIR}"
    "${SUBTREE_LIBS2}/include"
)

#####################################################
##   ZW_ismyframe  unit test for ZW_SLAVE
#####################################################
add_unity_test(NAME  TestZW_ismyframe_SLAVE
  FILES
    "${ZW_ROOT}/ZWave/Protocol/ZW_Frame.c"
    "${ZW_ROOT}/ZWave/ZW_ismyframe.c"
    "${ZW_ROOT}/ZWave/ZW_node.c"
    "${ZW_ROOT}/ZWave/ZW_lr_virtual_node_id.c"
    "${ZW_ROOT}/ZWave/Protocol/ZW_Frame.c"
    "${ZWAVE_MOCKS_DIR}/ZW_txq_protocol_mock.c"
    "${ZWAVE_MOCKS_DIR}/ZW_transport_mock.c"
    "${ZWAVE_MOCKS_DIR}/ZW_tx_queue_mock.c"
    TestZW_ismyframe_SLAVE.c
    LIBRARIES
    QueueNotifyingMock
    NodeMask
    zpal_mock
    AssertTest
    SyncEvent
)
target_compile_definitions(TestZW_ismyframe_SLAVE
  PRIVATE
    ZW_slave_lib
)
target_include_directories(TestZW_ismyframe_SLAVE
  PRIVATE
    "${ZW_ROOT}/ZWave"
    ${ZW_ROOT}/ZWave/EndDevice
    "${ZW_ROOT}/ZWave/Protocol"
    "${ZWAVE_CONFIG_DIR}"
    "${SUBTREE_LIBS2}/include"
)

#####################################################
## ZW_controller
#####################################################
add_unity_test(NAME TestZW_controller
  FILES
    TestZW_controller.c
    "${ZW_ROOT}/ZWave/Protocol/ZW_Frame.c"
    ${ZW_ROOT}/ZWave/Controller/ZW_controller.c
    ${ZW_ROOT}/ZWave/Controller/ZW_inclusion_controller.c
    ${ZW_ROOT}/ZWave/Controller/ZW_replication.c
    ${ZW_ROOT}/ZWave/ZW_dynamic_tx_power.c
    ${ZW_ROOT}/ZWave/ZW_dynamic_tx_power_algorithm.c
    ${ZW_ROOT}/ZWave/ZW_lr_virtual_node_id.c
    "${ZW_ROOT}/ZWave/mocks/ZW_nvm_mock.c"
    "${ZW_ROOT}/ZWave/mocks/ZW_NVMCaretaker_mock.c"
    "${ZW_ROOT}/ZWave/Controller/ZW_controller_network_info_storage.c"
    "${ZW_ROOT}/ZWave/ZW_transport_transmit_cb.c"
    "${ZW_ROOT}/ZWave/ZW_CCList.c"
    "${ZW_ROOT}/ZWave/ZW_home_id_generator.c"
    "${ZW_ROOT}/ZWave/ZW_frames_filters.c"
    "${ZW_ROOT}/ZWave/Protocol/ZW_Frame.c"
    "${ZW_ROOT}/ZWave/ZW_node.c"
    "${ZW_ROOT}/ZWave/ZW_protocol_cmd_handler.c"
    "${ZW_ROOT}/ZWave/mocks/ZW_transport_mock.c"
    "${ZWAVE_MOCKS_DIR}/ZW_timer_mock.c"
    ${ZWAVE_MOCKS_DIR}/mock_system_startup.c
    "${ZW_ROOT}/ZWave/Protocol/mocks/ZW_DataLinkLayer_mock.c"
    "${ZW_ROOT}/ZWave/mocks/ZW_routing_cache_mock.c"
    "${ZW_ROOT}/ZWave/mocks/ZW_routing_mock.c"
    "${ZW_ROOT}/ZWave/mocks/ZW_protocol_interface_mock.c"
    "${ZW_ROOT}/ZWave/mocks/ZW_tx_queue_mock.c"
    "${ZW_ROOT}/ZWave/mocks/ZW_noise_detect_mock.c"
    "${ZW_ROOT}/ZWave/mocks/ZW_explore_mock.c"
    "${ZW_ROOT}/ZWave/mocks/ZW_main_mock.c"
    "${ZW_ROOT}/ZWave/mocks/ZW_main_region_mock.c"
    "${ZW_ROOT}/ZWave/mocks/ZW_network_management_mock.c"
  LIBRARIES
    QueueNotifyingMock
    SwTimerMock
    mock
    NodeMaskMock
    Utils
    SyncEvent
    zpal_mock
    DebugPrintMock
    AssertTest
)
target_compile_definitions(TestZW_controller
    PRIVATE
    ZW_controller_lib
)
target_include_directories(TestZW_controller
  PRIVATE
  "${ZWAVE_API_DIR}"
  ${ZW_ROOT}/ZWave/Controller
  "${ZW_ROOT}/ZWave/Protocol"
  "${ZWAVE_CONFIG_DIR}"
  "${SUBTREE_LIBS2}/include"
)

################################################################################
## ZW_transport
################################################################################

add_unity_test(NAME TestZW_transport
  FILES
    TestZW_transport.c
    "${ZW_ROOT}/ZWave/Protocol/ZW_Frame.c"
    "${ZW_ROOT}/ZWave/ZW_transport.c"
    "${ZW_ROOT}/ZWave/ZW_node.c"
    "${ZW_ROOT}/ZWave/ZW_transport_transmit_cb.c"
    "${ZW_ROOT}/ZWave/ZW_lr_virtual_node_id.c"
    "${ZW_ROOT}/ZWave/Protocol/ZW_Frame.c"
    "${ZW_ROOT}/ZWave/Protocol/mocks/ZW_DataLinkLayer_mock.c"
    "${ZWAVE_MOCKS_DIR}/ZW_timer_mock.c"
    "${SUBTREE_LIBS2}/include/mock/transport_service2_mock.c"
    "${ZWAVE_MOCKS_DIR}/ZW_txq_protocol_mock.c"
    "${ZWAVE_MOCKS_DIR}/ZW_tx_queue_mock.c"
    "${ZWAVE_MOCKS_DIR}/ZW_explore_mock.c"
    "${ZWAVE_MOCKS_DIR}/ZW_build_tx_header_mock.c"
    "${ZWAVE_MOCKS_DIR}/ZW_protocol_interface_mock.c"
    "${ZWAVE_MOCKS_DIR}/ZW_network_management_mock.c"
    "${ZWAVE_MOCKS_DIR}/ZW_tx_queue_mock.c"
    "${ZWAVE_MOCKS_DIR}/ZW_slave_mock.c"
    "${ZWAVE_MOCKS_DIR}/ZW_main_mock.c"
    "${ZWAVE_MOCKS_DIR}/ZW_main_region_mock.c"
    "${ZWAVE_MOCKS_DIR}/ZW_keystore_mock.c"
    "${ZWAVE_MOCKS_DIR}/ZW_routing_all_mock.c"
    "${ZWAVE_MOCKS_DIR}/ZW_slave_network_info_storage_mock.c"
    "${ZWAVE_MOCKS_DIR}/ZW_dynamic_tx_power_mock.c"
    "${ZWAVE_MOCKS_DIR}/ZW_frames_filters_mock.c"
    "${ZWAVE_MOCKS_DIR}/ZW_ismyframe_mock.c"
    "${ZWAVE_MOCKS_DIR}/ZW_Security_Scheme0_mock.c"
    "${ZWAVE_MOCKS_DIR}/ZW_Security_Scheme2_mock.c"
    "${ZWAVE_MOCKS_DIR}/ZW_receivefilter_transport_mock.c"
    "${ZWAVE_MOCKS_DIR}/ZW_transport_multi_mock.c"
    "${ZWAVE_MOCKS_DIR}/ZW_transport_mock_extern.c"
  LIBRARIES
    Utils
    NodeMask
    SwTimerMock
    QueueNotifyingMock
    SyncEvent
    zpal_mock
    AssertTest
)
target_include_directories(TestZW_transport
  PRIVATE
    "${SUBTREE_LIBS2}/include"
    "${SUBTREE_LIBS2}/transport_service"
    "${ZW_ROOT}/ZWave/Protocol"
    "${ZWAVE_CONFIG_DIR}"
    ${ZW_ROOT}/ZWave/EndDevice
)
set_target_properties(TestZW_transport PROPERTIES COMPILE_DEFINITIONS "ZW_slave_lib;ZWAVE_ON_LINUX")


#####################################################
## TestZW_controller_tx_power
## Test of API for storing and reading the LR nodes tx power values
#####################################################
add_unity_test(NAME TestZW_controller_tx_power
  FILES
      TestZW_controller_tx_power.c
    ${ZW_ROOT}/ZWave/ZW_dynamic_tx_power.c
    ${ZW_ROOT}/ZWave/ZW_dynamic_tx_power_algorithm.c
    "${ZW_ROOT}/ZWave/mocks/ZW_controller_network_info_storage_mock.c"
    "${ZW_ROOT}/ZWave/ZW_node.c"
  LIBRARIES
    QueueNotifyingMock
    SwTimerMock
    mock
    NodeMask
    Utils
    SyncEvent
    zpal_mock
    AssertTest
)
target_compile_definitions(TestZW_controller_tx_power
    PRIVATE
    ZW_controller_lib
)
target_include_directories(TestZW_controller_tx_power
  PRIVATE
  ${ZW_ROOT}/ZWave/Controller
  "${ZW_ROOT}/ZWave/Protocol"
  "${ZWAVE_CONFIG_DIR}"
)

#####################################################
## TestZW_slave_tx_power
## Test of API for storing and reading the LR nodes tx power values
#####################################################
add_unity_test(NAME TestZW_slave_tx_power
  FILES
    TestZW_slave_tx_power.c
    ${ZW_ROOT}/ZWave/ZW_dynamic_tx_power.c
    ${ZW_ROOT}/ZWave/ZW_dynamic_tx_power_algorithm.c
  LIBRARIES
    mock
    AssertTest
    DebugPrintMock
    zpal_mock
)
target_compile_definitions(TestZW_slave_tx_power
  PRIVATE
    ZW_SLAVE
)
target_include_directories(TestZW_slave_tx_power
  PRIVATE
    "${ZWAVE_CONFIG_DIR}"
    ${ZWAVE_API_DIR}
)

#####################################################
## ZW_slave
#####################################################
add_unity_test(NAME TestZW_slave
  FILES
    "${ZW_ROOT}/ZWave/Protocol/ZW_Frame.c"
    TestZW_slave.c
    ${ZW_ROOT}/ZWave/EndDevice/ZW_slave.c
    ${ZW_ROOT}/ZWave/ZW_dynamic_tx_power.c
    ${ZW_ROOT}/ZWave/ZW_dynamic_tx_power_algorithm.c
    "${ZW_ROOT}/ZWave/EndDevice/ZW_slave_network_info_storage.c"
    "${ZW_ROOT}/ZWave/ZW_transport_transmit_cb.c"
    "${ZW_ROOT}/ZWave/ZW_CCList.c"
    "${ZW_ROOT}/ZWave/ZW_home_id_generator.c"
    "${ZW_ROOT}/ZWave/ZW_frames_filters.c"
    "${ZW_ROOT}/ZWave/Protocol/ZW_Frame.c"
    "${ZW_ROOT}/ZWave/ZW_node.c"
    "${ZW_ROOT}/ZWave/ZW_protocol_cmd_handler.c"
    "${ZW_ROOT}/ZWave/ZW_s2_inclusion_glue.c" # mock it!
    "${ZWAVE_MOCKS_DIR}/ZW_timer_mock.c"
    "${ZW_ROOT}/ZWave/Protocol/mocks/ZW_DataLinkLayer_mock.c"
    "${ZW_ROOT}/ZWave/Protocol/mocks/ZW_receivefilter_learnmode_mock.c"
    "${ZWAVE_MOCKS_DIR}/ZW_nvm_mock.c"
    "${ZWAVE_MOCKS_DIR}/ZW_NVMCaretaker_mock.c"
    "${ZWAVE_MOCKS_DIR}/ZW_transport_mock.c"
    "${ZWAVE_MOCKS_DIR}/ZW_network_management_mock.c"
    "${ZWAVE_MOCKS_DIR}/ZW_protocol_interface_mock.c"
    "${ZWAVE_MOCKS_DIR}/ZW_tx_queue_mock.c"
    "${ZWAVE_MOCKS_DIR}/ZW_noise_detect_mock.c"
    "${ZWAVE_MOCKS_DIR}/ZW_explore_mock.c"
    "${ZWAVE_MOCKS_DIR}/ZW_main_mock.c"
    "${ZWAVE_MOCKS_DIR}/ZW_main_region_mock.c"
    "${ZWAVE_MOCKS_DIR}/ZW_keystore_mock.c"
    "${ZWAVE_MOCKS_DIR}/ZW_secure_learn_support_mock.c"
    "${ZWAVE_MOCKS_DIR}/ZW_keystore_mock.c"
    "${ZWAVE_MOCKS_DIR}/ZW_routing_all_mock.c"
    "${ZWAVE_MOCKS_DIR}/ZW_ctimer_mock.c"
    "${ZWAVE_MOCKS_DIR}/ZW_Security_Scheme2_mock.c"
    "${ZWAVE_MOCKS_DIR}/mock_system_startup.c"
    "${SUBTREE_LIBS2}/inclusion/mock/s2_inclusion_mock.c"
    "${SUBTREE_LIBS2}/include/mock/s2_mock.c"
    "${SUBTREE_LIBS2}/include/mock/transport_service2_mock.c"
  LIBRARIES
    QueueNotifyingMock
    SwTimerMock
    mock
    NodeMask
    Utils
    SyncEvent
    zpal_mock
    AssertTest
)
target_compile_definitions(TestZW_slave
  PUBLIC
  PRIVATE
    ZW_slave_lib
    UNITY_TEST
    ZWAVE_ON_LINUX # for libs2
)


target_include_directories(TestZW_slave
  PUBLIC
  PRIVATE
  "${ZW_ROOT}/ZWave"
  ${ZW_ROOT}/ZWave/EndDevice
  "${ZWAVE_CONFIG_DIR}"
  "${ZW_ROOT}/ZWave/Protocol"
  "${SUBTREE_LIBS2}/include"
  "${SUBTREE_LIBS2}/transport_service"
)

# Compile options are commented out because they introduce too many errors.
#target_compile_options(TestZW_slave PUBLIC -Wall -Wextra -Wpedantic -Werror)



#####################################################
##   ZW_tx_build_header  unit test for ZW_SLAVE
#####################################################
add_unity_test(NAME TestZW_build_tx_header_slave
  FILES
    "${ZW_ROOT}/ZWave/Protocol/ZW_Frame.c"
    "${ZW_ROOT}/ZWave/ZW_build_tx_header.c"
    "${ZW_ROOT}/ZWave/Protocol/ZW_Frame.c"
    "${ZW_ROOT}/ZWave/ZW_node.c"
    "${ZWAVE_MOCKS_DIR}/ZW_transport_mock.c"
    TestZW_build_tx_header_slave.c
    LIBRARIES
    QueueNotifyingMock
    NodeMask
    zpal
    AssertTest
    SyncEvent
)
target_compile_definitions(TestZW_build_tx_header_slave
  PRIVATE
    ZW_slave_lib
)
target_include_directories(TestZW_build_tx_header_slave
  PRIVATE
    "${ZW_ROOT}/ZWave"
    ${ZW_ROOT}/ZWave/EndDevice 
    "${ZW_ROOT}/ZWave/Protocol"
    "${ZWAVE_CONFIG_DIR}"
    "${SUBTREE_LIBS2}/include"
)



#####################################################
##   ZW_tx_build_header  unit test for ZW_CONTROLLER
#####################################################
add_unity_test(NAME TestZW_build_tx_header_controller
  FILES
     "${ZW_ROOT}/ZWave/Protocol/ZW_Frame.c"
     TestZW_build_tx_header_controller.c
    "${ZW_ROOT}/ZWave/ZW_build_tx_header.c"
    "${ZW_ROOT}/ZWave/ZW_node.c"
    "${ZW_ROOT}/ZWave/Protocol/ZW_Frame.c"
    "${ZWAVE_MOCKS_DIR}/ZW_txq_protocol_mock.c"
    "${ZWAVE_MOCKS_DIR}/ZW_transport_mock.c"
    "${ZWAVE_MOCKS_DIR}/ZW_controller_mock.c"
    "${ZWAVE_MOCKS_DIR}/ZW_controller_network_info_storage_mock.c"
    LIBRARIES
    QueueNotifyingMock
    NodeMask
    SwTimerMock
    zpal
    AssertTest
    SyncEvent
)
target_compile_definitions(TestZW_build_tx_header_controller
  PRIVATE
    ZW_controller_lib
)

target_include_directories(TestZW_build_tx_header_controller
  PRIVATE
    "${ZW_ROOT}/ZWave"
    ${ZW_ROOT}/ZWave/Controller
    "${ZW_ROOT}/ZWave/Protocol"
    "${ZWAVE_CONFIG_DIR}"
    "${SUBTREE_LIBS2}/include"
)


#####################################################
##   ZW_lr_virtual_node_id for controllers
#####################################################
add_unity_test(NAME TestZW_lr_virtual_node_id_controller
  FILES
    "TestZW_lr_virtual_node_id_controller.c"
    "${ZW_ROOT}/ZWave/ZW_lr_virtual_node_id.c"
  LIBRARIES
    AssertTest
    zpal
    FreeRtosMock
    QueueNotifyingMock
    NodeMask
    SyncEvent
)
target_include_directories(TestZW_lr_virtual_node_id_controller
  PRIVATE
    "${ZWAVE_API_DIR}"
    "${ZW_ROOT}/ZWave"
    "${ZW_ROOT}/ZWave/Protocol"
    "${ZWAVE_CONFIG_DIR}"
)

target_compile_definitions(TestZW_lr_virtual_node_id_controller
  PRIVATE
    ZW_controller_lib
)

#####################################################
##   ZW_lr_virtual_node_id for slaves
#####################################################
add_unity_test(NAME TestZW_lr_virtual_node_id_slave
  FILES
    "TestZW_lr_virtual_node_id_slave.c"
    "${ZW_ROOT}/ZWave/ZW_lr_virtual_node_id.c"
  LIBRARIES
    AssertTest
    zpal
    FreeRtosMock
    QueueNotifyingMock
    NodeMask
    SyncEvent
)
target_include_directories(TestZW_lr_virtual_node_id_slave
  PRIVATE
  "${ZWAVE_API_DIR}"
  "${ZW_ROOT}/ZWave"
  "${ZW_ROOT}/ZWave/Protocol"
  "${ZWAVE_CONFIG_DIR}"
)

target_compile_definitions(TestZW_lr_virtual_node_id_slave
  PRIVATE
    ZW_slave_lib
)

#####################################################
## TestZW_noise_detect
## Test of API for measuring and reading background noise
#####################################################
add_unity_test(NAME TestZW_noise_detect
  FILES
    TestZW_noise_detect.c
    ${ZW_ROOT}/ZWave/ZW_noise_detect.c
    ${ZW_ROOT}/ZWave/Protocol/mocks/ZW_DataLinkLayer_mock.c
  LIBRARIES
    mock
    SyncEvent
    AssertTest
    zpal_mock
)
target_compile_definitions(TestZW_noise_detect
  PRIVATE
    ZW_SLAVE
)
target_include_directories(TestZW_noise_detect
  PRIVATE
    "${ZWAVE_API_DIR}"
    "${ZW_ROOT}/ZWave/Protocol"
    "${ZWAVE_CONFIG_DIR}"
)

#####################################################
## TestZ W_transport_transmit_cb
#####################################################
add_unity_test(NAME TestZW_transport_transmit_cb
  FILES
    TestZW_transport_transmit_cb.c
    ${ZW_ROOT}/ZWave/ZW_transport_transmit_cb.c
  LIBRARIES
    mock
    Assert
    DebugPrintMock
    zpal_mock
)
target_include_directories(TestZW_transport_transmit_cb
  PRIVATE
    "${ZWAVE_API_DIR}"
    "${ZW_ROOT}/ZWave"
)

#####################################################
## TestZW_explore
#####################################################
add_unity_test(NAME TestZW_explore
  FILES
    TestZW_explore.c
    ${ZW_ROOT}/ZWave/ZW_explore.c
    ${ZW_ROOT}/ZWave/ZW_node.c
    "${ZWAVE_MOCKS_DIR}/ZW_ctimer_mock.c"
    "${ZWAVE_MOCKS_DIR}/ZW_keystore_mock.c"
    "${ZWAVE_MOCKS_DIR}/ZW_protocol_interface_mock.c"
    "${ZWAVE_MOCKS_DIR}/ZW_slave_network_info_storage_mock.c"
    "${ZWAVE_MOCKS_DIR}/ZW_slave_mock.c"
    "${ZWAVE_MOCKS_DIR}/ZW_routing_all_mock.c"
    "${ZWAVE_MOCKS_DIR}/ZW_event_mock.c"
    "${ZWAVE_MOCKS_DIR}/ZW_transport_mock.c"
    "${ZWAVE_MOCKS_DIR}/ZW_transport_mock_extern.c"
    "${ZWAVE_MOCKS_DIR}/ZW_transport_transmit_cb_mock.c"
    "${ZWAVE_MOCKS_DIR}/ZW_timer_mock.c"
    "${ZW_ROOT}/ZWave/Protocol/mocks/ZW_DataLinkLayer_mock.c"
    "${ZW_ROOT}/ZWave/Protocol/mocks/ZW_receivefilter_learnmode_mock.c"
  LIBRARIES
    mock
    Assert
    DebugPrintMock
    QueueNotifyingMock
    NodeMaskMock
    SwTimerMock
    SyncEvent
    zpal_mock
    FreeRtosMock
)
target_include_directories(TestZW_explore
  PRIVATE
    "${ZWAVE_API_DIR}"
    "${ZW_ROOT}/ZWave"
    ${ZW_ROOT}/ZWave/EndDevice
    "${ZW_ROOT}/Components/Utils"
    "${ZWAVE_CONFIG_DIR}"
    "${ZW_ROOT}/ZWave/Protocol"
    "${SUBTREE_LIBS2}/include"
)
target_compile_definitions(TestZW_explore
  PRIVATE
    ZW_slave_lib
)

#####################################################
## TestZW_Security_Scheme2
#####################################################
add_unity_test(NAME TestZW_Security_Scheme2
  FILES
    TestZW_Security_Scheme2.c
    ${ZW_ROOT}/ZWave/ZW_Security_Scheme2.c
    ${ZW_ROOT}/ZWave/ZW_node.c
    "${ZWAVE_MOCKS_DIR}/ZW_ctimer_mock.c"
    "${ZWAVE_MOCKS_DIR}/ZW_keystore_mock.c"
    "${ZWAVE_MOCKS_DIR}/ZW_protocol_interface_mock.c"
    "${ZWAVE_MOCKS_DIR}/ZW_s2_inclusion_glue_mock.c"
    "${ZWAVE_MOCKS_DIR}/ZW_slave_network_info_storage_mock.c"
    "${ZWAVE_MOCKS_DIR}/ZW_transport_mock.c"
    "${ZWAVE_MOCKS_DIR}/ZW_transport_multi_mock.c"
    "${ZWAVE_MOCKS_DIR}/ZW_transport_transmit_cb_mock.c"
    "${SUBTREE_LIBS2}/inclusion/mock/s2_inclusion_mock.c"
    "${SUBTREE_LIBS2}/include/mock/s2_mock.c"
  LIBRARIES
    mock
    Assert
    DebugPrintMock
    QueueNotifyingMock
    NodeMaskMock
    SyncEvent
    zpal_mock
    FreeRtosMock
)
target_include_directories(TestZW_Security_Scheme2
  PRIVATE
    "${ZWAVE_API_DIR}"
    "${ZW_ROOT}/ZWave"
    ${ZW_ROOT}/ZWave/EndDevice
    "${ZWAVE_CONFIG_DIR}"
    "${ZW_ROOT}/ZWave/Protocol"
    "${SUBTREE_LIBS2}/include"
)
target_compile_definitions(TestZW_Security_Scheme2
    PRIVATE
    ZW_SECURITY_PROTOCOL
    ZW_SLAVE
)

#####################################################
## TestZW_Security_Scheme0
#####################################################
add_unity_test(NAME TestZW_Security_Scheme0
  FILES
    TestZW_Security_Scheme0.c
    ${ZW_ROOT}/ZWave/ZW_Security_Scheme0.c
    ${ZW_ROOT}/ZWave/ZW_node.c
    ${SUBTREE_LIBS2}/crypto/aes/aes.c
    "${ZWAVE_MOCKS_DIR}/ZW_ctimer_mock.c"
    "${ZWAVE_MOCKS_DIR}/ZW_keystore_mock.c"
    "${ZWAVE_MOCKS_DIR}/ZW_protocol_interface_mock.c"
    "${ZWAVE_MOCKS_DIR}/ZW_s2_inclusion_glue_mock.c"
    "${ZWAVE_MOCKS_DIR}/ZW_slave_network_info_storage_mock.c"
    "${ZWAVE_MOCKS_DIR}/ZW_transport_mock.c"
    "${ZWAVE_MOCKS_DIR}/ZW_transport_transmit_cb_mock.c"
    "${SUBTREE_LIBS2}/inclusion/mock/s2_inclusion_mock.c"
    "${SUBTREE_LIBS2}/inclusion/mock/s2_keystore_mock.c"
    "${SUBTREE_LIBS2}/include/mock/s2_mock.c"
  LIBRARIES
    mock
    Assert
    QueueNotifyingMock
    NodeMaskMock
    SyncEvent
    DebugPrintMock
    zpal_mock
    FreeRtosMock
)

target_include_directories(TestZW_Security_Scheme0
  PRIVATE
    "${ZWAVE_API_DIR}"
    "${ZW_ROOT}/ZWave"
    ${ZW_ROOT}/ZWave/EndDevice
    "${ZWAVE_CONFIG_DIR}"
    "${ZW_ROOT}/ZWave/Protocol"
    "${SUBTREE_LIBS2}/include"
    "${FREERTOS_DIR}/include"
    "${ZW_ROOT}/ZWave/Freertos/include/Generic"
    "${ZW_ROOT}/ZWave/Freertos/target/Generic"
    "${ZW_ROOT}/ZWave/Freertos/include"
)

target_compile_definitions(TestZW_Security_Scheme0
    PRIVATE
    ZW_SECURITY_PROTOCOL
    ZW_SLAVE
    ZW_SLAVE_ROUTING
    DllExport=extern
)

#####################################################
##  Test NVM3 file sizes in the slave targets
#####################################################
add_unity_test(NAME TestAsserts_ZW_slave_network_info_files_sizes
  FILES
    "TestAsserts_ZW_slave_network_info_files_sizes.c"
    "${ZW_ROOT}/ZWave/mocks/ZW_nvm_mock.c"
    "${ZW_ROOT}/ZWave/ZW_node.c"
    "${ZW_ROOT}/ZWave/mocks/ZW_NVMCaretaker_mock.c"
  LIBRARIES
    Assert
    mock
    SyncEvent
    Utils
    NodeMask
    DebugPrintMock
    zpal_mock
)
target_include_directories(TestAsserts_ZW_slave_network_info_files_sizes
  PRIVATE
  "${SUBTREE_LIBS2}/include"
  "${ZW_ROOT}/ZWave/Protocol"
  "${ZW_ROOT}/ZWave"
  ${ZW_ROOT}/ZWave/EndDevice
  "${ZWAVE_CONFIG_DIR}"
  "${ZWAVE_API_DIR}"
  "${COMPONENTS_DIR}/DebugPrint"
)

target_compile_definitions(TestAsserts_ZW_slave_network_info_files_sizes
  PRIVATE
    ZW_slave_lib
)


#####################################################
##  Test NVM3 file sizes in the controller targets
#####################################################
add_unity_test(NAME TestAsserts_ZW_controller_network_info_files_sizes
  FILES
    "TestAsserts_ZW_controller_network_info_files_sizes.c"
    "${ZW_ROOT}/ZWave/mocks/ZW_nvm_mock.c"
    "${ZW_ROOT}/ZWave/mocks/ZW_NVMCaretaker_mock.c"
    "${ZW_ROOT}/ZWave/ZW_node.c"
  LIBRARIES
    Assert
    mock
    SyncEvent
    Utils
    NodeMask
    QueueNotifyingMock
    DebugPrintMock
    zpal_mock
)

target_include_directories(TestAsserts_ZW_controller_network_info_files_sizes
  PRIVATE
  "${SUBTREE_LIBS2}/include"
  "${ZWAVE_API_DIR}"
  "${ZW_ROOT}/ZWave/Protocol"
  ${ZW_ROOT}/ZWave/Controller
  "${ZWAVE_CONFIG_DIR}"
  "${COMPONENTS_DIR}/Assert"
  "${COMPONENTS_DIR}/DebugPrint"
)

target_compile_definitions(TestAsserts_ZW_controller_network_info_files_sizes
  PRIVATE
    ZW_controller_lib
)

add_unity_test(NAME TestZW_main_region_controller
  FILES
    ../ZW_main_region.c
    ${ZW_ROOT}/ZWave/Protocol/mocks/ZW_DataLinkLayer_mock.c
    ${ZW_ROOT}/ZWave/Protocol/ZW_Channels.c
#    ${PAL_PLATFORM_DIR}/mocks/Rail_Mock.c
    ${ZW_ROOT}/PAL/mocks/zpal_radio_mock.c
    ${ZW_ROOT}/PAL/mocks/zpal_init_mock.c
    ${ZWAVE_MOCKS_DIR}/ZW_controller_network_info_storage_mock.c
    ${ZWAVE_MOCKS_DIR}/ZW_main_mock.c
    ${ZWAVE_MOCKS_DIR}/ZW_controller_mock.c
  LIBRARIES
    AssertTest
    mock
    SyncEvent
    Utils
    zpal_common
    FreeRtosMock

)
target_include_directories(TestZW_main_region_controller
  PRIVATE
    "${PAL_PLATFORM_DIR}/inc"
    "${PAL_PLATFORM_DIR}/src/Radio"
    "${PAL_PLATFORM_DIR}/src/Radio/zwconfig"
    "${RAILIMPORT_DIR}/platform/radio/rail_lib/common"
    "${RAILIMPORT_DIR}/platform/radio/rail_lib/chip/efr32/efr32xg1x"
    "${RAILIMPORT_DIR}/platform/radio/rail_lib/protocol/zwave"
    "${RAILIMPORT_DIR}/platform/radio/rail_lib/plugin/pa-conversions"
    "${ZW_ROOT}/ZWave/ZWaveConfig"
    "${ZW_ROOT}/ZWave/API"
    "${ZW_ROOT}/Components/QueueNotifying"
    "${ZW_ROOT}/Components/NodeMask"
    "${ZW_ROOT}/ZWave/Protocol"
    "${ZW_ROOT}/Components/SwTimer"
    ${ZW_ROOT}/ZWave/Controller
)
target_compile_definitions(TestZW_main_region_controller
  PRIVATE
    UNITY_TEST
    ZW_controller_lib
)

add_unity_test(NAME TestZW_main_region_slave
  FILES
    ../ZW_main_region.c
    ${ZW_ROOT}/ZWave/Protocol/mocks/ZW_DataLinkLayer_mock.c
    ${ZW_ROOT}/ZWave/Protocol/ZW_Channels.c
    ${ZW_ROOT}/PAL/mocks/zpal_radio_mock.c
    ${ZW_ROOT}/PAL/mocks/zpal_init_mock.c
    ${ZWAVE_MOCKS_DIR}/ZW_slave_network_info_storage_mock.c
    ${ZWAVE_MOCKS_DIR}/ZW_main_mock.c
    ${ZWAVE_MOCKS_DIR}/ZW_slave_mock.c
  LIBRARIES
    AssertTest
    mock
    SyncEvent
    Utils
    zpal_common
    FreeRtosMock

)
target_include_directories(TestZW_main_region_slave
  PRIVATE
    "${PAL_PLATFORM_DIR}/inc"
    "${PAL_PLATFORM_DIR}/src/Radio"
    "${PAL_PLATFORM_DIR}/src/Radio/zwconfig"
    "${RAILIMPORT_DIR}/platform/radio/rail_lib/common"
    "${RAILIMPORT_DIR}/platform/radio/rail_lib/chip/efr32/efr32xg1x"
    "${RAILIMPORT_DIR}/platform/radio/rail_lib/protocol/zwave"
    "${RAILIMPORT_DIR}/platform/radio/rail_lib/plugin/pa-conversions"
    "${ZW_ROOT}/ZWave/ZWaveConfig"
    "${ZW_ROOT}/ZWave/API"
    "${ZW_ROOT}/Components/QueueNotifying"
    "${ZW_ROOT}/Components/NodeMask"
    "${ZW_ROOT}/ZWave/Protocol"
    ${ZW_ROOT}/ZWave/EndDevice
)
target_compile_definitions(TestZW_main_region_slave
  PRIVATE
    UNITY_TEST
    ZW_slave_lib
)
