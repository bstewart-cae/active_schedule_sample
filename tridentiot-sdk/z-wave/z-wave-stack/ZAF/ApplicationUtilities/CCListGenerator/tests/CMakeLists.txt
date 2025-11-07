set(test_zaf_cc_list_generator_src
  test_zaf_cc_list_generator.c
  ../zaf_cc_list_generator.c
)
add_unity_test(NAME test_zaf_cc_list_generator
  FILES ${test_zaf_cc_list_generator_src}
  LIBRARIES ZAF_CC_Invoker_cmock
            FreeRTOS_cmock
            zaf_config_api_cmock
            DebugPrint_cmock
            zpal
            Utils
  USE_UNITY_WITH_CMOCK
)
target_include_directories(test_zaf_cc_list_generator PRIVATE
  ../
  ${ZWAVE_API_DIR}
)
