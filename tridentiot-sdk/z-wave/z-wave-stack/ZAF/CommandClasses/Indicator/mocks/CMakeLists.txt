
add_mock(cc_indicator_cmock ${ZAF_CCDIR}/Indicator/inc/CC_Indicator.h)
target_include_directories(cc_indicator_cmock PUBLIC 
                          "${CMAKE_CURRENT_SOURCE_DIR}/.."
                           ${ZAF_UTILDIR}
                           ${ZWAVE_API_DIR}
)

target_link_libraries(cc_indicator_cmock
  PUBLIC
  CC_CommonMock
)