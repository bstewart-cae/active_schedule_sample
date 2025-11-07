add_library(CC_SupervisionMock CC_Supervision_mock.c)
target_include_directories(CC_SupervisionMock PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/../inc
  ${ZAF_UTILDIR}
  ${ZAF_UTILDIR}/TargetSpecificIncludes
  ${ZWAVE_API_DIR}
)
target_link_libraries(CC_SupervisionMock CC_CommonMock mock zaf_transport_layer_interface)

add_mock(cc_supervision_config_api_cmock ${ZAF_CCDIR}/Supervision/src/cc_supervision_config_api.h)
target_include_directories(cc_supervision_config_api_cmock PUBLIC
                           ../inc
                           ${ZPAL_API_DIR}
                           ${ZWAVE_API_DIR}
                           ${ZAF_UTILDIR})
target_link_libraries(cc_supervision_config_api_cmock 
  PUBLIC 
    zaf_transport_layer_interface
)

add_mock(cc_supervision_handlers_cmock ${ZAF_CCDIR}/Supervision/inc/cc_supervision_handlers.h)
target_include_directories(cc_supervision_handlers_cmock PUBLIC
                          ../inc 
                          ${ZPAL_API_DIR}
                          ${ZWAVE_API_DIR}
                          ${ZAF_UTILDIR})
target_link_libraries(cc_supervision_handlers_cmock 
  PUBLIC 
    zaf_transport_layer_interface
)
                        
add_mock(CC_Supervision_cmock ${ZAF_CCDIR}/Supervision/inc/CC_Supervision.h)
target_include_directories(CC_Supervision_cmock
  PUBLIC
    ../inc
    ${ZW_ROOT}/PAL/inc
    ${ZAF_CCDIR}/Common/
    ${ZWAVE_API_DIR}
    ${ZAF_UTILDIR}
)
target_link_libraries(CC_Supervision_cmock 
  PUBLIC 
    zaf_transport_layer_interface
)
