add_cc_config_mock(cc_multilevel_switch_support_config_api_cmock 
    ${CMAKE_CURRENT_SOURCE_DIR}/../inc/CC_MultilevelSwitch_Support.h
)
target_include_directories(cc_multilevel_switch_support_config_api_cmock PUBLIC 
                           ${ZPAL_API_DIR}
                           ${ZWAVE_API_DIR}
                           ${ZAF_UTILDIR}
                           $<TARGET_PROPERTY:SwTimerMock,INTERFACE_INCLUDE_DIRECTORIES>
                           ${ZAF_CCDIR}/Common/
                           ${ZAF_UTILDIR}/Actuator)

add_mock(cc_multilevel_switch_support_io_cmock ${ZAF_CCDIR}/MultilevelSwitch/src/cc_multilevel_switch_support_io.h)
target_include_directories(cc_multilevel_switch_support_io_cmock PUBLIC 
                           ${ZPAL_API_DIR}
                           ${ZWAVE_API_DIR}
                           ${ZAF_UTILDIR}
                           $<TARGET_PROPERTY:SwTimerMock,INTERFACE_INCLUDE_DIRECTORIES>
                           ${ZAF_CCDIR}/Common/
                           ${ZAF_UTILDIR}/Actuator
                           ../inc)
