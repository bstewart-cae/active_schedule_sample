add_mock(cc_color_switch_config_api_cmock ${ZAF_CCDIR}/ColorSwitch/src/cc_color_switch_config_api.h)
target_include_directories(cc_color_switch_config_api_cmock PUBLIC
                           ../inc
                           ${ZPAL_API_DIR}
                           ${ZWAVE_API_DIR}
                           ${ZAF_UTILDIR}
                           $<TARGET_PROPERTY:SwTimerMock,INTERFACE_INCLUDE_DIRECTORIES>
                           ${ZAF_UTILDIR}/Actuator)

add_mock(cc_color_switch_io_cmock ${ZAF_CCDIR}/ColorSwitch/src/cc_color_switch_io.h)
target_include_directories(cc_color_switch_io_cmock PUBLIC
                           ../inc
                           ../src
                           ${ZPAL_API_DIR}
                           ${ZWAVE_API_DIR}
                           ${ZAF_UTILDIR}
                           $<TARGET_PROPERTY:SwTimerMock,INTERFACE_INCLUDE_DIRECTORIES>
                           ${ZAF_UTILDIR}/Actuator)
