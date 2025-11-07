################################################################################
# Add test for Version command class.
################################################################################

set(test_CC_Version_src 
  test_CC_Version.c
  ../CC_Version.c
  ../mocks/CC_Version_mock_extern.c
  ${ZWAVE_MOCKS_DIR}/mock_system_startup.c
  ${ZAF_UTILDIR}/mocks/mock_ZW_TransportEndpoint.c
  ${ZAF_UTILDIR}/mocks/ZW_TransportSecProtocol_mock.c
  ${ZAF_UTILDIR}/ZAF_CC_Invoker.c
  ${ZWAVE_MOCKS_DIR}/ZW_build_number_mock.c
  )
add_unity_test(NAME test_CC_Version 
               FILES ${test_CC_Version_src} 
               LIBRARIES mock 
                         test_common
                         CC_CommonMock
                         ZAF_CommonInterfaceMock
                         zaf_config_mock
                         zpal_mock
                         Utils)
target_compile_definitions(test_CC_Version PRIVATE
  test_CC_Version_defines
)
target_include_directories(test_CC_Version PUBLIC
  ..
  ${CONFIG_DIR}
  ${ZAF_CONFIGDIR}/config
  ${ZWAVE_API_DIR}
)