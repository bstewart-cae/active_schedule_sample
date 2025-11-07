include_directories( .. )

add_compile_definitions(ZW_SLAVE_ENHANCED_232)

################################################################################
# Add test for Supervision command class.
################################################################################
set(test_CC_Supervision_src
  test_CC_Supervision.c
  ${ZAF_UTILDIR}/mocks/mock_ZW_TransportEndpoint.c
  ${ZAF_UTILDIR}/mocks/mock_ZW_TransportMulticast.c  
  ${ZAF_UTILDIR}/ZAF_CC_Invoker.c
  ${ZAF_CCDIR}/MultiChan/mocks/multichannel_mock.c
)
add_unity_test(NAME test_CC_Supervision
               FILES ${test_CC_Supervision_src}
               LIBRARIES mock
                         test_common
                         NodeMask
                         ZAF_CommonInterfaceMock
                         zpal_mock
                         cc_supervision_config_api_cmock
                         cc_supervision_handlers_cmock
                         zaf_transport_layer_cmock
                         zaf_event_distributor_soc_cmock
                USE_UNITY_WITH_CMOCK
                         )
target_include_directories(test_CC_Supervision PUBLIC
  ${ZAF_UTILDIR}
  ${ZAF_CCDIR}/Association
  ${ZAF_CCDIR}/Common
  ${ZAF_CCDIR}/MultiChan/inc
  ${ZAF_CCDIR}/Supervision/inc
  ${ZAF_CCDIR}/Supervision/src
  ${ZAF_CCDIR}/Supervision/config
  ${ZAF_UNITTESTEXTERNALS}
)