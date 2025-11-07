add_mock(cc_multi_channel_config_api_cmock ../src/cc_multi_channel_config_api.h)
target_include_directories(cc_multi_channel_config_api_cmock
  PUBLIC
    ../src
    ../config
)

add_mock(multichannel_cmock ${ZAF_CCDIR}/MultiChan/inc/multichannel.h)
target_include_directories(multichannel_cmock
  PUBLIC
    ../inc
    ${ZAF_UTILDIR}
    ${ZWAVE_API_DIR}
    ${ZPAL_API_DIR}
)