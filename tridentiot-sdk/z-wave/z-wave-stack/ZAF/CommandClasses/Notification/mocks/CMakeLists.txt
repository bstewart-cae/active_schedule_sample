add_mock(cc_notification_cmock ${ZAF_CCDIR}/Notification/inc/CC_Notification.h)

target_include_directories(cc_notification_cmock PUBLIC
                           ${ZPAL_API_DIR}
                           ${ZWAVE_API_DIR}
                           ${ZAF_UTILDIR}
                           ../inc
                           ../src
                           )

add_mock(cc_notification_io_cmock ${ZAF_CCDIR}/Notification/src/cc_notification_io.h)

target_include_directories(cc_notification_io_cmock PUBLIC
                           ${ZPAL_API_DIR}
                           ${ZWAVE_API_DIR}
                           ${ZAF_UTILDIR}
                           ${ZAF_CCDIR}/Association
                           ${ZAF_CCDIR}/Common
                           ../src
                           )

add_mock(cc_notification_config_api_cmock ${ZAF_CCDIR}/Notification/src/cc_notification_config_api.h)

target_include_directories(cc_notification_config_api_cmock PUBLIC 
                           ${ZPAL_API_DIR}
                           ${ZWAVE_API_DIR}
                           ${ZAF_UTILDIR}
                           ${ZAF_CCDIR}/Association
                           ${ZAF_CCDIR}/Common
                           ../inc
                           ../config
                           ../src
                           )

