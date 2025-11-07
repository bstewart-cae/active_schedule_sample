include_directories( .. )

add_compile_definitions(ZW_SLAVE_ENHANCED_232)

################################################################################
# Add test for Device Reset Locally command class.
################################################################################add_
set(test_CC_DeviceResetLocally_src
  test_CC_DeviceResetLocally.c
  ../CC_DeviceResetLocally.c
  ${ZAF_CCDIR}/Common/mocks/CC_Common_mock.c
  ${ZAF_UTILDIR}/mocks/mock_ZW_TransportEndpoint.c
  )
add_unity_test(NAME test_CC_DeviceResetLocally
               FILES ${test_CC_DeviceResetLocally_src}
               LIBRARIES mock
                         ZAF_CommonInterfaceMock
)
target_include_directories(test_CC_DeviceResetLocally PUBLIC 
  ${ZAF_CCDIR}/Common
  ${ZAF_UTILDIR}
)
