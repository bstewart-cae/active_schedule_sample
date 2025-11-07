################################################################################
# Add test for Wake Up command class.
################################################################################

set(test_CC_WakeUp_src 
  test_CC_WakeUp.cpp
  ../src/CC_WakeUp.c
  ../mocks/CC_WakeUp_config_mock.c
  ${ZAF_UTILDIR}/mocks/ZW_TransportSecProtocol_mock.c
  ${ZAF_UTILDIR}/mocks/mock_ZW_TransportEndpoint.c
  ${ZAF_UTILDIR}/mocks/ZAF_nvm_mock.c
  ${ZAF_UTILDIR}/mocks/ZAF_nvm_app_mock.c
  ${ZAF_UTILDIR}/ZAF_CC_Invoker.c
  ${ZWAVE_MOCKS_DIR}/mock_system_startup.c
)

add_unity_test(NAME test_CC_WakeUp
               FILES ${test_CC_WakeUp_src}
               LIBRARIES mock
                         QueueNotifyingMock
                         Utils
                         AppTimerMock
                         ZAF_CommonInterfaceMock
                         zpal_mock
                         test_common
               USE_CPP)

target_compile_definitions(test_CC_WakeUp PRIVATE
  test_CC_WakeUp_defines
  ZW_SLAVE
)

target_include_directories(test_CC_WakeUp PUBLIC 
  ../inc
  ../config
  ${ZAF_CCDIR}/Common
  ${ZAF_UTILDIR}
  ${ZAF_UNITTESTEXTERNALS}
  ${CONFIG_DIR}
)
