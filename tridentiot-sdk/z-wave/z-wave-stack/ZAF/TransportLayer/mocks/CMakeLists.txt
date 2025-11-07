add_mock(zaf_transport_layer_cmock ../inc/zaf_transport_tx.h)

target_include_directories(zaf_transport_layer_cmock PRIVATE
  ../inc
  ${ZAF_UTILDIR}
  ${ZWAVE_API_DIR}
)

target_link_libraries(zaf_transport_layer_cmock
  PUBLIC
    zpal
)
