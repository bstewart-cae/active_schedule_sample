################################################################################
# Add test for Binary Switch command class using CMock
################################################################################

# ##################################################################################################
# Create a test target.
# ##################################################################################################
set(test_CC_BinarySwitch_src_cmock
  test_CC_BinarySwitch_cmock.c
  ../src/CC_BinarySwitch.c
  ${ZAF_UTILDIR}/ZAF_CC_Invoker.c
)
# Add the necessary libraries and the generated mock libraries
add_unity_test(NAME test_CC_BinarySwitch_cmock 
               FILES ${test_CC_BinarySwitch_src_cmock}
               LIBRARIES
                         test_common
                         ZW_Assert_unity2
                         ZW_TransportEndpoint_cmock
                         ZAF_TSE_cmock
                         ZAF_Actuator_cmock
                         CC_Supervision_cmock
                         CC_BinarySwitch_config_cmock
                         Utils
                         DebugPrint_cmock
                         zaf_config_api_cmock
               USE_UNITY_WITH_CMOCK
)
target_include_directories(test_CC_BinarySwitch_cmock
  INTERFACE
  PUBLIC
    ${CONFIG_DIR}
    ${ZAF_CONFIGDIR}/inc
    ${ZAF_CONFIGDIR}/config
)

# ##################################################################################################
# Create a test target that includes the real ZAF Actuator instead of a mock.
#
# Required to test scenarios that depend on when the Actuator invokes the actuator callback.
# ##################################################################################################
add_unity_test(NAME test_CC_BinarySwitch_with_actuator
               FILES
                 test_CC_BinarySwitch_with_actuator.c
                 ../src/CC_BinarySwitch.c
                 ${ZAF_UTILDIR}/ZAF_CC_Invoker.c
                 ${ZAF_UTILDIR}/Actuator/ZAF_Actuator.c
               LIBRARIES
                         test_common
                         ZW_Assert_unity2
                         ZW_TransportEndpoint_cmock
                         ZAF_TSE_cmock
                         ZAF_Actuator_cmock
                         CC_Supervision_cmock
                         CC_BinarySwitch_config_cmock
                         Utils
                         DebugPrint_cmock
                         AppTimer_cmock
                         SwTimerCMock
                         zaf_config_api_cmock
                         m
               USE_UNITY_WITH_CMOCK
)
