add_cc_config_mock(cc_central_scene_config_api_cmock ${ZAF_CCDIR}/CentralScene/inc/CC_CentralScene.h)

target_include_directories(cc_central_scene_config_api_cmock PUBLIC 
                           ${ZPAL_API_DIR}
                           ${ZWAVE_API_DIR}
                           ${ZAF_UTILDIR}
                           ${ZAF_CCDIR}/Common
                            ../config
                           )

add_mock(cc_central_scene_io_cmock ${ZAF_CCDIR}/CentralScene/src/cc_central_scene_io.h)

target_include_directories(cc_central_scene_io_cmock PUBLIC 
                           ${ZPAL_API_DIR}
                           ${ZWAVE_API_DIR}
                           ${ZAF_UTILDIR}
                           ${ZAF_CCDIR}/Common
                            ../config
                            ../inc
                           )
