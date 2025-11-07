set(test_zaf_nvm_soc_src
  test_zaf_nvm_soc.c
  ../zaf_nvm_soc.c
)
add_unity_test(NAME test_zaf_nvm_soc
  FILES ${test_zaf_nvm_soc_src}
  LIBRARIES Assert_cmock
  DebugPrint_cmock
  zaf_common_helper_cmock
  zpal_cmock
  ZAF_nvm_app_cmock
  USE_UNITY_WITH_CMOCK
)
target_include_directories(test_zaf_nvm_soc PRIVATE
  ../
  ${ZAF_UTILDIR}
)
