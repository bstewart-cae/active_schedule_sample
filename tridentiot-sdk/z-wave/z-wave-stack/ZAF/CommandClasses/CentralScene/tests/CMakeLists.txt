include_directories( .. )

add_compile_definitions(ZW_SLAVE_ENHANCED_232)


################################################################################
# Add test for Central Scene command class.
###############################################################################

set(test_CC_CentralScene_src 
  test_CC_CentralScene.c
  ../src/CC_CentralScene.c
  ${ZAF_UTILDIR}/mocks/ZAF_nvm_mock.c
  ${ZAF_UTILDIR}/mocks/ZAF_nvm_app_mock.c
  ${ZAF_UTILDIR}/mocks/mock_ZW_TransportMulticast.c
  ${ZAF_UTILDIR}/mocks/mock_ZW_TransportEndpoint.c
  ${ZAF_UTILDIR}/ZAF_CC_Invoker.c
  )
add_unity_test(NAME test_CC_CentralScene
               FILES ${test_CC_CentralScene_src}
               LIBRARIES mock
                         QueueNotifyingMock
                         SwTimerMock
                         AppTimerMock
                         ZAF_TSEMock
                         CC_CommonMock
                         NodeMask
                         AssertTest
                         cc_central_scene_config_api_cmock
                         cc_central_scene_io_cmock
                         cc_association_group_info_cmock
                         test_common
                        )
target_include_directories(test_CC_CentralScene PUBLIC 
  "../config"
  ${ZAF_CCDIR}/Association
  ${ZAF_CCDIR}/Common
  ${ZAF_UTILDIR}
  ${ZAF_UNITTESTEXTERNALS}
  ${ZAF_UTILDIR}/mocks/inc
  ${COMPONENTS_DIR}/Utils
)