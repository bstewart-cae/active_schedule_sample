include_directories( .. )

################################################################################
# Add test for Battery command class.
################################################################################

set(test_CC_Battery_src
  test_CC_Battery.c
  ../src/CC_Battery.c
  ../mocks/CC_Battery_mock_weak.c
  ${ZAF_CCDIR}/Common/mocks/CC_Common_mock.c
  ${ZAF_UTILDIR}/mocks/mock_ZW_TransportEndpoint.c
  ${ZAF_UTILDIR}/mocks/mock_ZW_TransportMulticast.c
  ${ZAF_UTILDIR}/ZAF_CC_Invoker.c
  )
add_unity_test(NAME test_CC_Battery
               FILES ${test_CC_Battery_src}
               LIBRARIES mock
                         QueueNotifyingMock
                         NodeMask
                         AssertTest
                         ZAF_CommonInterfaceMock
                         cc_battery_io_cmock
                         cc_battery_config_api_cmock
                         test_common
)
target_compile_definitions(test_CC_Battery PRIVATE
  test_CC_Battery_defines
)
target_include_directories(test_CC_Battery PUBLIC
  ${ZAF_UTILDIR}
  ${ZAF_UTILDIR}/_commonIF
  ${ZAF_CCDIR}/Association
  ${ZAF_CCDIR}/Supervision/inc
  ${ZAF_CCDIR}/Common
  ${ZAF_UNITTESTEXTERNALS}
)