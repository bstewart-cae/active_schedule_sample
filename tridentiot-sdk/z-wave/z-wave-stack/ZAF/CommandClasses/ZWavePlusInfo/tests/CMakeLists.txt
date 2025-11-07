################################################################################
# Add test for Z-Wave Plus Info command class.
################################################################################
set(test_CC_ZWavePlusInfo_src
  test_CC_ZWavePlusInfo.c
  ../src/CC_ZWavePlusInfo.c
  ../src/cc_zwave_plus_info_config_api.c
  ../mocks/CC_ZWavePlusInfo_mock.c
  ${ZAF_UTILDIR}/mocks/mock_ZW_TransportEndpoint.c
  ${ZAF_UTILDIR}/ZAF_CC_Invoker.c
  )
add_unity_test(NAME test_CC_ZWavePlusInfo
               FILES ${test_CC_ZWavePlusInfo_src}
               LIBRARIES mock
                         CC_CommonMock
                         AssertTest
                         Utils
                         test_common
                       )

target_compile_definitions(test_CC_ZWavePlusInfo PRIVATE
  ZAF_CONFIG_DEVICE_OPTION_MASK=APPLICATION_NODEINFO_LISTENING
  ZAF_CONFIG_APP_ROLE_TYPE=ZWAVEPLUS_INFO_REPORT_ROLE_TYPE_END_NODE_ALWAYS_ON
)

target_include_directories(test_CC_ZWavePlusInfo PRIVATE
  ../inc
  .
  ${ZAF_UTILDIR}
  ${ZAF_CONFIGDIR}/inc
  ${ZAF_CONFIGDIR}/config
)