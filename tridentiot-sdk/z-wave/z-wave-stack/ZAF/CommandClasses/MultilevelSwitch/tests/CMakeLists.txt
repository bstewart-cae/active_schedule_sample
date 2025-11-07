################################################################################
# Add test for MultilevelSwitch command class.
################################################################################
set(test_CC_MultilevelSwitch_src
  test_CC_MultilevelSwitch.cpp
  ../src/CC_MultilevelSwitch_Support.c
  ${ZAF_UTILDIR}/mocks/mock_ZW_TransportEndpoint.c
  ${ZAF_UTILDIR}/ZAF_CC_Invoker.c
  )
add_unity_test(NAME test_CC_MultilevelSwitch
               FILES ${test_CC_MultilevelSwitch_src}
               LIBRARIES mock
                         ZAF_CommonInterfaceMock
                         AppTimerMock
                         test_common
                         ZAF_TSEMock
                         ZAF_ActuatorMock
                         Utils
                         CC_SupervisionMock
                         cc_multilevel_switch_support_config_api_cmock
                         cc_multilevel_switch_support_io_cmock
                         CC_SupervisionMock
               USE_CPP
               USE_UNITY_WITH_CMOCK
)

target_include_directories(test_CC_MultilevelSwitch
  PRIVATE
    ../inc
    ${ZAF_UTILDIR}
    ${ZAF_UNITTESTEXTERNALS}
)

################################################################################
# Add test for CC Multilevel Switch with Actuator.
################################################################################
add_unity_test(NAME test_CC_MultilevelSwitch_with_actuator
               FILES
                 test_CC_MultilevelSwitch_with_actuator.cpp
                 ../src/CC_MultilevelSwitch_Support.c
                 ${ZAF_UTILDIR}/mocks/mock_ZW_TransportEndpoint.c
                 ${ZAF_UTILDIR}/ZAF_CC_Invoker.c
                 ${ZAF_UTILDIR}/Actuator/ZAF_Actuator.c
               LIBRARIES mock
                         ZAF_CommonInterfaceMock
                         test_common
                         Utils
                         CC_SupervisionMock
                         cc_multilevel_switch_support_config_api_cmock
                         cc_multilevel_switch_support_io_cmock
                         AppTimer_cmock
                         SwTimerCMock
                         ZAF_TSE_cmock
                         m
               USE_CPP
               USE_UNITY_WITH_CMOCK
)
target_include_directories(test_CC_MultilevelSwitch_with_actuator
  PRIVATE
    ../inc
)
