add_mock(cc_door_lock_config_api_cmock ${ZAF_CCDIR}/DoorLock/src/cc_door_lock_config_api.h)
target_include_directories(cc_door_lock_config_api_cmock PUBLIC 
                           ${ZPAL_API_DIR}
                           ${ZWAVE_API_DIR}
                           ${ZAF_UTILDIR}
                           ${ZAF_CCDIR}/Common
                           ${ZAF_CCDIR}/Association
                           ${ZAF_CCDIR}/DoorLock/inc
                         )

add_mock(cc_door_lock_config_io_cmock ${ZAF_CCDIR}/DoorLock/src/cc_door_lock_io.h)
target_include_directories(cc_door_lock_config_io_cmock PUBLIC 
                           ${ZPAL_API_DIR}
                           ${ZWAVE_API_DIR}
                           ${ZAF_UTILDIR}
                           ${ZAF_CCDIR}/Common
                           ${ZAF_CCDIR}/Association
                           ${ZAF_CCDIR}/DoorLock/inc
                           ${ZAF_CCDIR}/DoorLock/src)
