add_mock(zaf_protocol_config_cmock ${ZAF_PROTOCOL_CONFIG}/inc/zaf_protocol_config.h)

target_include_directories(zaf_protocol_config_cmock
  PUBLIC
    ${ZAF_UTILDIR}
    ${ZWAVE_API_DIR}
)

target_link_libraries(zaf_protocol_config_cmock
  PUBLIC
    Assert_cmock
    FreeRTOS_cmock
    NodeMask_cmock
    QueueNotifying_cmock
  PRIVATE
    zpal  
)
