################################################################################
# Add test for Powerlevel command class.
################################################################################
set(test_CC_Powerlevel_src
  test_CC_Powerlevel.c
  ../CC_PowerLevel.c
  ${ZAF_UTILDIR}/mocks/mock_ZW_TransportEndpoint.c
  ${ZAF_UTILDIR}/mocks/ZW_TransportSecProtocol_mock.c
  ${ZWAVE_MOCKS_DIR}/ZW_keystore_mock.c
  ${ZAF_UTILDIR}/ZAF_CC_Invoker.c
)

add_unity_test(NAME test_CC_Powerlevel 
               FILES ${test_CC_Powerlevel_src} 
               LIBRARIES mock 
                         QueueNotifying_cmock 
                         SwTimerMock 
                         AppTimerMock 
                         ZAF_CommonInterfaceMock
                         test_common
                         CC_CommonMock
                         CC_SupervisionMock
                         zaf_transport_layer_cmock
)

target_compile_definitions(test_CC_Powerlevel PRIVATE
  ZW_SLAVE
  test_CC_Powerlevel_defines
  ZAF_CONFIG_NUMBER_OF_END_POINTS=0
  MAX_ASSOCIATION_GROUPS=1
  MAX_ASSOCIATION_IN_GROUP=1
)
target_include_directories(test_CC_Powerlevel PRIVATE 
  ..
  ${ZAF_CCDIR}/Supervision/inc
)
