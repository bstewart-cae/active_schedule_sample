# SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
#
# SPDX-License-Identifier: BSD-3-Clause

#####################################################
## ZW_controller slave inclusion 
#####################################################
add_unity_test(NAME TestZW_controller_inclusion_slave
  FILES
    # Test
    TestZW_controller_inclusion_slave.c
    
    # Production code
    "${ZW_ROOT}/ZWave/Controller/ZW_controller.c"
    "${ZW_ROOT}/ZWave/Controller/ZW_inclusion_controller.c"
    "${ZW_ROOT}/ZWave/Controller/ZW_replication.c"
    "${ZW_ROOT}/ZWave/ZW_dynamic_tx_power.c"
    "${ZW_ROOT}/ZWave/ZW_dynamic_tx_power_algorithm.c"
    "${ZW_ROOT}/ZWave/ZW_lr_virtual_node_id.c"
    "${ZW_ROOT}/ZWave/ZW_transport_transmit_cb.c"
    "${ZW_ROOT}/ZWave/ZW_CCList.c"
    "${ZW_ROOT}/ZWave/ZW_frames_filters.c"
    "${ZW_ROOT}/ZWave/Protocol/ZW_Frame.c"
    "${ZW_ROOT}/ZWave/ZW_node.c"
    "${ZW_ROOT}/ZWave/ZW_protocol_cmd_handler.c"
    
    # Mocks
    ${ZWAVE_MOCKS_DIR}/mock_system_startup.c
    "${ZW_ROOT}/ZWave/mocks/ZW_nvm_mock.c"
    "${ZW_ROOT}/ZWave/mocks/ZW_NVMCaretaker_mock.c"
    "${ZW_ROOT}/ZWave/mocks/ZW_controller_network_info_storage_mock.c"
    "${ZW_ROOT}/ZWave/mocks/ZW_home_id_generator_mock.c"
    "${ZW_ROOT}/ZWave/mocks/ZW_transport_mock.c"
    "${ZWAVE_MOCKS_DIR}/ZW_timer_mock.c"
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
    NodeMask
    Utils
    SyncEvent
    zpal_mock
    AssertTest
)
target_compile_definitions(TestZW_controller_inclusion_slave
    PRIVATE
    ZW_controller_lib
)
target_include_directories(TestZW_controller_inclusion_slave
  PRIVATE
  "${ZW_ROOT}/ZWave/Protocol"
  ${ZW_ROOT}/ZWave/Controller
  "${ZWAVE_CONFIG_DIR}"
)

# Test disabled for portable controller because it doesn't support SIS functionality
#[[
#####################################################
## ZW_controller portable slave inclusion
#####################################################
add_unity_test(NAME TestZW_controller_portable_inclusion_slave
  TEST_BASE
    # Test
    TestZW_controller_inclusion_slave.c
  FILES
    # Production code
    "${ZW_ROOT}/ZWave/ZW_controller.c"
    "${ZW_ROOT}/ZWave/ZW_inclusion_controller.c"
    "${ZW_ROOT}/ZWave/ZW_replication.c"
    "${ZW_ROOT}/ZWave/ZW_dynamic_tx_power.c"
    "${ZW_ROOT}/ZWave/ZW_lr_virtual_node_id.c"
    "${ZW_ROOT}/ZWave/ZW_transport_transmit_cb.c"
    "${ZW_ROOT}/ZWave/ZW_CCList.c"
    "${ZW_ROOT}/ZWave/ZW_frames_filters.c"
    "${ZW_ROOT}/ZWave/Protocol/ZW_Frame.c"
    "${ZW_ROOT}/ZWave/ZW_node.c"
    
    # Mocks
    "${ZW_ROOT}/ZWave/mocks/ZW_nvm_mock.c"
    "${ZW_ROOT}/ZWave/mocks/ZW_NVMCaretaker_mock.c"
    "${ZW_ROOT}/ZWave/mocks/ZW_controller_network_info_storage_mock.c"
    "${ZW_ROOT}/ZWave/mocks/ZW_home_id_generator_mock.c"
    "${ZW_ROOT}/ZWave/mocks/ZW_transport_mock.c"
    "${ZWAVE_MOCKS_DIR}/ZW_timer_mock.c"
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
    NodeMask
    Utils
    SyncEvent
    zpal_mock
    AssertTest
)
target_compile_definitions(TestZW_controller_portable_inclusion_slave
    PRIVATE
    ZW_controller_portable_lib
)
target_include_directories(TestZW_controller_portable_inclusion_slave
  PRIVATE
  "${ZW_ROOT}/ZWave/Protocol"
  "${ZWAVE_CONFIG_DIR}"
)
]]
