set(test_ZAF_Common_interface_src
    test_ZAF_Common_interface.c
    ../ZAF_Common_interface.c
    ${ZAF_UTILDIR}/ZAF_CC_Invoker.c
    ${ZAF_UTILDIR}/mocks/mock_ZW_TransportEndpoint.c
    ${ZAF_UTILDIR}/mocks/mock_ZW_TransportMulticast.c
    ${ZAF_UTILDIR}/mocks/ZW_TransportSecProtocol_mock.c
    ${ZAF_UTILDIR}/mocks/ZAF_nvm_mock.c
    ${ZAF_UTILDIR}/mocks/ZAF_nvm_app_mock.c
)

add_unity_test(NAME test_ZAF_Common_interface
               FILES ${test_ZAF_Common_interface_src}
               LIBRARIES mock
                         SwTimerMock
                         CC_CommonMock
                         ZAF_TSEMock
                         ZAF_CmdPublisher_Mock
                         AssertTest
                         zpal_mock
                         AppTimerMock
                         AppsHwMock
                         zaf_protocol_config_cmock
                         )

target_include_directories(test_ZAF_Common_interface PUBLIC
  ../
)

set(test_ZAF_Common_helper_src
    test_ZAF_Common_helper.c
    ../ZAF_Common_helper.c
    ../ZAF_Common_interface.c
    ${ZAF_UTILDIR}/ZAF_CC_Invoker.c
    ${ZAF_UTILDIR}/mocks/mock_ZW_TransportEndpoint.c
    ${ZAF_UTILDIR}/mocks/mock_ZW_TransportMulticast.c
    ${ZAF_UTILDIR}/mocks/ZW_TransportSecProtocol_mock.c
    ${ZAF_UTILDIR}/mocks/ZAF_nvm_mock.c
    ${ZAF_UTILDIR}/mocks/ZAF_nvm_app_mock.c
    ${ZWAVE_MOCKS_DIR}/mock_system_startup.c
)

add_unity_test(NAME test_ZAF_Common_helper
               FILES ${test_ZAF_Common_helper_src} 
               LIBRARIES mock 
                         SwTimerMock
                         CC_CommonMock
                         ZAF_TSEMock
                         ZAF_CmdPublisher_Mock
                         AssertTest
                         zpal_mock
                         AppTimerMock
                         AppsHwMock
                         zaf_config_api_cmock
                         zaf_cc_list_generator_cmock
                         zaf_protocol_config_cmock
                         EventDistributorMock
                         ZAF_EventHandlingMock
                         zaf_transport_layer_cmock
                         ZAF_AppName_cmock
               USE_UNITY_WITH_CMOCK
)

target_include_directories(test_ZAF_Common_helper PUBLIC
   ../
  ${ZAF_UTILDIR}
  ${ZAF_UTILDIR}/mocks
  ${ZAF_UTILDIR}/TrueStatusEngine
  ${ZAF_UTILDIR}/CmdPublisher
)

