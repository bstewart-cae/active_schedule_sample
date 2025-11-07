add_mock(cc_battery_io_cmock ${ZAF_CCDIR}/Battery/src/cc_battery_io.h)
target_include_directories(cc_battery_io_cmock PUBLIC 
                           ../inc
                           ${ZPAL_API_DIR}
                           ${ZWAVE_API_DIR}
                           ${ZAF_UTILDIR}
                           ${ZAF_CCDIR}/Common
                           ${ZAF_CCDIR}/Association
)

add_cc_config_mock(cc_battery_config_api_cmock 
  ${CMAKE_CURRENT_SOURCE_DIR}/../inc/CC_Battery.h
)
target_include_directories(cc_battery_config_api_cmock PUBLIC 
                           ${ZPAL_API_DIR}
                           ${ZWAVE_API_DIR}
                           ${ZAF_UTILDIR}
                           ${ZAF_CCDIR}/Common
                           ${ZAF_CCDIR}/Association
)

add_library(CC_Battery_mock_weak OBJECT)
target_sources(CC_Battery_mock_weak
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/CC_Battery_mock_weak.c
)
target_include_directories(CC_Battery_mock_weak PUBLIC 
                           ${ZPAL_API_DIR}
                           ${ZWAVE_API_DIR}
                           ${ZAF_UTILDIR}
                           ${ZAF_CCDIR}/Battery/inc
                           ${ZAF_CCDIR}/Common
                           ${ZAF_CCDIR}/Association)
target_link_libraries(CC_Battery_mock_weak
    PUBLIC
        mock
)
                         