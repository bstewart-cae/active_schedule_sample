# ###############################################################################
# Add test for event distributor soc
# ###############################################################################
set(test_zaf_event_distributor_soc_src
  test_zaf_event_distributor_soc.c
  ../zaf_event_distributor.c
  ../zaf_event_distributor_soc.c
)
add_unity_test(NAME test_zaf_event_distributor_soc
  FILES ${test_zaf_event_distributor_soc_src}
  LIBRARIES AppsHw_cmock
  EventDistributor_cmock
  DebugPrint_cmock
  AppTimer_cmock
  zaf_common_helper_cmock
  ZAF_Common_interface_cmock
  ZAF_network_learn_cmock
  zaf_job_helper_cmock
  ZAF_CmdPublisher_cmock
  ZW_TransportSecProtocol_cmock
  FreeRTOS_cmock
  zpal_cmock
  ZAF_nvm_app_cmock
  zaf_nvm_soc_cmock
  cc_device_reset_locally_cmock
  cc_indicator_cmock
  Utils
  USE_UNITY_WITH_CMOCK
)
target_include_directories(test_zaf_event_distributor_soc PRIVATE
  ../
  ../config
  ${ZWAVE_API_DIR}
  ${ZAF_CCDIR}/Indicator/inc/
)
