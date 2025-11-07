set(test_CC_ColorSwitch_src
  test_CC_ColorSwitch.c
  ../src/CC_ColorSwitch.c
  ${ZAF_UTILDIR}/mocks/mock_ZW_TransportEndpoint.c
  ${ZAF_UTILDIR}/Actuator/mocks/ZAF_Actuator_mock.c
  ${ZAF_UTILDIR}/ZAF_CC_Invoker.c
)

add_unity_test(NAME test_CC_ColorSwitch
               FILES ${test_CC_ColorSwitch_src}
               LIBRARIES mock
                         test_common
                         QueueNotifyingMock
                         CC_CommonMock
                         NodeMask
                         Utils
                         ZAF_TSEMock
                         DebugPrintMock
                         AssertTest
                         SwTimerMock
                         cc_color_switch_config_api_cmock
                         cc_color_switch_io_cmock
                         CC_SupervisionMock
               USE_UNITY_WITH_CMOCK
)

target_include_directories(test_CC_ColorSwitch
  PUBLIC
    #nothing
  PRIVATE
    ${ZAF_UTILDIR}/Actuator/
    ${ZAF_CCDIR}/Supervision/
)

add_unity_test(NAME test_CC_ColorSwitch_with_actuator
               FILES
                 test_CC_ColorSwitch_with_actuator.c
                 ../src/CC_ColorSwitch.c
                 ${ZAF_UTILDIR}/mocks/mock_ZW_TransportEndpoint.c
                 ${ZAF_UTILDIR}/ZAF_CC_Invoker.c
                 ${ZAF_UTILDIR}/Actuator/ZAF_Actuator.c
               LIBRARIES mock
                         test_common
                         QueueNotifyingMock
                         CC_CommonMock
                         NodeMask
                         Utils
                         ZAF_TSEMock
                         DebugPrintMock
                         AssertTest
                         SwTimerMock
                         cc_color_switch_config_api_cmock
                         cc_color_switch_io_cmock
                         CC_SupervisionMock
                         AppTimer_cmock
                         SwTimerCMock
                         m
               USE_UNITY_WITH_CMOCK
)
