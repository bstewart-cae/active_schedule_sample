################################################################################
# Add test for Manufacturer Specific command class.
################################################################################
set(test_CC_ManufacturerSpecific_src 
  test_CC_ManufacturerSpecific.cpp
  ../CC_ManufacturerSpecific.c
  ${ZAF_UTILDIR}/mocks/mock_ZW_TransportEndpoint.c
  ${ZAF_UTILDIR}/mocks/mock_ZW_TransportMulticast.c
  ${ZAF_UTILDIR}/ZAF_CC_Invoker.c
  )
add_unity_test(NAME test_CC_ManufacturerSpecific 
               FILES ${test_CC_ManufacturerSpecific_src} 
               LIBRARIES mock 
                         CC_ManufacturerSpecific_test_utils
                         NodeMask
                         AssertTest
                         zaf_config_mock
                         zpal_mock
               USE_CPP)
target_compile_definitions(test_CC_ManufacturerSpecific PRIVATE
  test_CC_ManufacturerSpecific_defines
)
target_include_directories(test_CC_ManufacturerSpecific
  PRIVATE
   ..
    ${ZAF_CCDIR}/Association
  PUBLIC
    ${ZAF_UTILDIR}
)
