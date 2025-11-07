################################################################################
# Add test for Configuration command class.
################################################################################
set(test_CC_Configuration_src
  test_CC_Configuration.c
  ../mocks/CC_Configuration_interface_mock.c
  ${ZAF_UTILDIR}/mocks/ZAF_nvm_mock.c
  ${ZAF_UTILDIR}/mocks/ZAF_nvm_app_mock.c
  ../src/CC_Configuration.c
  ${ZAF_UTILDIR}/mocks/mock_ZW_TransportEndpoint.c
  ${ZAF_UTILDIR}/ZAF_CC_Invoker.c
  )
add_unity_test(NAME test_CC_Configuration
               FILES ${test_CC_Configuration_src}
               LIBRARIES mock
                         CC_CommonMock
                         QueueNotifyingMock
                         DebugPrintMock
                         test_common
                         ZAF_TSEMock
                         ZAF_CommonInterfaceMock
                         Utils
                         zpal_mock
                         cc_configuration_config_api_cmock
                         CC_SupervisionMock
                      )
target_include_directories(test_CC_Configuration PUBLIC
  ../inc
  ../src
  ${ZAF_CCDIR}/Association
  ${ZAF_UTILDIR}/TrueStatusEngine
  ${ZAF_UTILDIR}
  ${ZAF_UNITTESTEXTERNALS}
  ${ZAF_CCDIR}/Common
  ${ZAF_CCDIR}/Supervision/inc
)

################################################################################
# Generate and add mocks using the cmock framework
################################################################################
################################################################################
# Add test for Configuration command class using cmock
################################################################################
set(test_CC_Configuration_src_cmock
  test_CC_Configuration_cmock.c
  ../src/CC_Configuration.c
  ../src/cc_configuration_io.c
  ${ZAF_UTILDIR}/ZAF_CC_Invoker.c
)
add_unity_test(NAME test_CC_Configuration_cmock
               FILES ${test_CC_Configuration_src_cmock}
               LIBRARIES mock
                         CC_CommonMock
                         QueueNotifyingMock
                         test_common
                         ZAF_TSEMock
                         DebugPrintMock
                         ZAF_Common_interface_cmock
                         Utils
                         zpal_mock
                         cc_configuration_config_api_cmock
                         ZAF_nvm_app_cmock
                         ZW_TransportEndpoint_cmock
                         CC_SupervisionMock
               USE_UNITY_WITH_CMOCK
)
target_include_directories(test_CC_Configuration_cmock PUBLIC
  ../inc
  ../src
  ${ZAF_CCDIR}/Association
  ${ZAF_UTILDIR}/TrueStatusEngine
  ${ZAF_UTILDIR}
  ${ZAF_UNITTESTEXTERNALS}
  ${ZAF_CCDIR}/Common
  ${ZAF_CCDIR}/Supervision/inc
)
