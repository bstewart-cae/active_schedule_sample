include_directories( .. )

add_compile_definitions(ZW_SLAVE_ENHANCED_232)

################################################################################
# Add test for User Code command class.
################################################################################

set(test_CC_UserCode_src 
  test_CC_UserCode.c
  ../src/cc_user_code_database.c
  ../src/CC_UserCode.c
  ${ZAF_CCDIR}/Common/mocks/CC_Common_mock.c
  ${ZAF_UTILDIR}/mocks/mock_ZW_TransportMulticast.c
  ${ZAF_UTILDIR}/mocks/mock_ZW_TransportEndpoint.c
  ${ZAF_UTILDIR}/ZAF_CC_Invoker.c
  )
add_unity_test(NAME test_CC_UserCode
               FILES ${test_CC_UserCode_src}
               LIBRARIES mock
                         QueueNotifyingMock
                         ZAF_TSEMock
                         test_common
                         NodeMask
                         AssertTest
                         cc_user_code_io_cmock
                         zaf_event_distributor_soc_cmock
               USE_UNITY_WITH_CMOCK)
target_include_directories(test_CC_UserCode PUBLIC
  ../config
  ../inc
  ${ZAF_CCDIR}/Association
  ${ZAF_CCDIR}/Common
  ${ZAF_UTILDIR}
  ${ZAF_UNITTESTEXTERNALS}
)
target_compile_definitions(test_CC_UserCode PUBLIC
  CC_USER_CODE_MAX_IDS=5
)