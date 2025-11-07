add_mock(cc_user_code_io_cmock ${ZAF_CCDIR}/UserCode/inc/cc_user_code_io.h)
target_include_directories(cc_user_code_io_cmock PUBLIC 
                           ${ZPAL_API_DIR}
                           ${ZWAVE_API_DIR}
                           ${ZAF_UTILDIR}
                           $<TARGET_PROPERTY:SwTimerMock,INTERFACE_INCLUDE_DIRECTORIES>
                           ${ZAF_CCDIR}/Common/
                           ${ZAF_CCDIR}/Association/
                           ${ZAF_CCDIR}/UserCode/inc/
                           ${ZAF_UTILDIR}/Actuator)
