set(test_zaf_transport_src
  test_zaf_transport.cpp
  ../src/zaf_transport_tx.c
)
add_unity_test(NAME test_zaf_transport
  FILES ${test_zaf_transport_src}
  LIBRARIES 
             FreeRTOS_cmock
             QueueNotifying_cmock
             NodeMask_cmock
             Assert_cmock
             ZAF_Common_interface_cmock
             ZW_TransportSecProtocol_cmock
             ZW_TransportMulticast_cmock
             ZW_TransportEndpoint_cmock
             cc_association_cmock
             DebugPrint_cmock
             zpal
  USE_CPP
  USE_UNITY_WITH_CMOCK
)
target_include_directories(test_zaf_transport PRIVATE
  ../inc
  ../config
  ${ZAF_UTILDIR}
  ${ZWAVE_API_DIR}
)
