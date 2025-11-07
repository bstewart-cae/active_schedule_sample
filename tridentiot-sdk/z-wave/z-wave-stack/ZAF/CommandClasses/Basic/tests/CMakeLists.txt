################################################################################
# Add test for Basic command class.
################################################################################

set(test_CC_Basic_src
  test_CC_Basic.c
  ../src/CC_Basic.c
  ${ZAF_CCDIR}/Common/mocks/CC_Common_mock.c
  ${ZAF_UTILDIR}/mocks/mock_ZW_TransportEndpoint.c
  ${ZAF_UTILDIR}/mocks/ZW_TransportSecProtocol_mock.c
  ${ZAF_UTILDIR}/ZAF_CC_Invoker.c
  )
add_unity_test(NAME test_CC_Basic
               FILES ${test_CC_Basic_src}
               LIBRARIES mock
                         test_common
                         ZAF_CommonInterfaceMock
                         ZAF_TSEMock
                         NodeMask
                         DebugPrintMock
                         zaf_config_mock
                         )
target_compile_definitions(test_CC_Basic PRIVATE
  test_CC_Basic_defines
)
target_include_directories(test_CC_Basic
  PRIVATE
    ../inc
  PUBLIC
    ${ZAF_CCDIR}/Association
    ${ZAF_CCDIR}/Common
    ${ZAF_UTILDIR}/TrueStatusEngine
    ${ZAF_UNITTESTEXTERNALS}
)

set(test_CC_Basic_cmock_src
  test_CC_Basic_cmock.c
  ../src/CC_Basic.c
  ${ZAF_CCDIR}/_TestUtils/test_common.c
  ${ZAF_UTILDIR}/ZAF_CC_Invoker.c
)
add_unity_test(NAME test_CC_Basic_cmock
               FILES ${test_CC_Basic_cmock_src}
               LIBRARIES CC_Common_cmock
                         DebugPrint_cmock 
                         ZAF_Common_interface_cmock
                         zaf_config_api_cmock
                         ZW_TransportSecProtocol_cmock
                         ZW_TransportEndpoint_cmock 
               USE_UNITY_WITH_CMOCK
)
target_include_directories(test_CC_Basic_cmock
  PRIVATE
    ../inc
  PUBLIC
    ${ZAF_CCDIR}/_TestUtils
)
