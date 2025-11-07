################################################################################
# Add test for Multi Channel command class.
################################################################################
set(test_CC_MultiChan_src 
  test_CC_MultiChan.c
  ../src/CC_MultiChan.c
  ../src/multichannel.c
  ${ZAF_UTILDIR}/mocks/ZAF_command_class_utils_mock.c
  ${ZAF_UTILDIR}/mocks/ZW_TransportSecProtocol_mock.c
  ${ZAF_UTILDIR}/ZAF_CC_Invoker.c
)
add_unity_test(NAME test_CC_MultiChan
               FILES ${test_CC_MultiChan_src}
               LIBRARIES mock
                         NodeMask
                         ZAF_CommonInterfaceMock
                         cc_multi_channel_config_api_cmock
                         zaf_config_api_cmock
                         Utils
                         ZW_TransportEndpoint_cmock
                         test_common
                         zpal_cmock
               USE_UNITY_WITH_CMOCK
)
target_include_directories(test_CC_MultiChan
  PRIVATE
    ../inc
    ../src
    ${ZAF_CCDIR}/Common
    ${ZAF_CONFIGDIR}/config
    ${ZAF_UTILDIR}
    ${ZAF_UNITTESTEXTERNALS}
)

################################################################################
# Add test for Multi Channel Framework command class.
################################################################################
set(test_CC_MultiChan_Framework_src 
  test_CC_MultiChan_Framework.c
  ../src/CC_MultiChan.c
  ../src/multichannel.c
  ${ZAF_UTILDIR}/ZW_TransportEndpoint.c
  ${ZAF_UTILDIR}/mocks/ZAF_command_class_utils_mock.c
  ${ZAF_UTILDIR}/ZAF_CC_Invoker.c
  ${ZWAVE_MOCKS_DIR}/ZW_keystore_mock.c
  ${ZWAVE_MOCKS_DIR}/ZW_mem_mock.c
  ${ZAF_UTILDIR}/ZAF_CC_Invoker.c
)
add_unity_test(NAME test_CC_MultiChan_Framework
               FILES ${test_CC_MultiChan_Framework_src}
               LIBRARIES mock 
                         QueueNotifyingMock
                         Utils
                         ZAF_CommonInterfaceMock
                         zaf_common_helper_cmock
                         zaf_cc_list_generator_cmock
                         cc_multi_channel_config_api_cmock
                         zaf_config_api_cmock
                         zaf_transport_layer_cmock
                         Utils
                         test_common
                         zpal_cmock
               USE_UNITY_WITH_CMOCK
)
target_compile_definitions(test_CC_MultiChan_Framework PRIVATE
  REQUESTED_SECURITY_KEYS=0
  ZAF_CONFIG_NUMBER_OF_END_POINTS=3
  ZW_SLAVE
)
target_include_directories(test_CC_MultiChan_Framework PRIVATE
  ../inc
  ../src
  ${ZAF_CCDIR}/Association
  ${ZAF_CCDIR}/Common
  ${ZAF_CONFIGDIR}/config
  ${ZAF_UTILDIR}
  ${ZAF_UNITTESTEXTERNALS}
)

set(test_CC_MultiChan_cmock_src
  test_CC_MultiChan_cmock.c
  ${ZAF_UTILDIR}/ZAF_CC_Invoker.c
  ${ZAF_CCDIR}/MultiChan/src/CC_MultiChan.c
  ${ZAF_UTILDIR}/ZAF_command_class_utils.c
)
add_unity_test(NAME test_CC_MultiChan_cmock
               FILES ${test_CC_MultiChan_cmock_src}
               LIBRARIES test_common
                         CC_Common_cmock
                         DebugPrint_cmock 
                         multichannel_cmock
                         ZAF_Common_interface_cmock
                         ZW_TransportSecProtocol_cmock
                         ZW_TransportEndpoint_cmock
                         zaf_config_api_cmock
                         zaf_protocol_config_cmock
                         cc_multi_channel_config_api_cmock
                         Utils
                         zpal_cmock
               USE_UNITY_WITH_CMOCK
)
target_include_directories(test_CC_MultiChan_cmock
  PRIVATE
    ../src
  PUBLIC
    ${ZAF_CCDIR}/_TestUtils
    ${ZAF_UTILDIR}
)
