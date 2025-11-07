add_mock(cc_configuration_config_api_cmock ${ZAF_CCDIR}/Configuration/src/cc_configuration_config_api.h)
target_include_directories(cc_configuration_config_api_cmock
  PRIVATE
    ../inc
  PUBLIC 
   ${ZPAL_API_DIR}
   ${ZWAVE_API_DIR}
   ${ZAF_UTILDIR}
)
