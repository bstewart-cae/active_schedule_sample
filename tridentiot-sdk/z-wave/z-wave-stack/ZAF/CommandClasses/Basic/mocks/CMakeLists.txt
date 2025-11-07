add_mock(CC_Basic_cmock ../inc/CC_Basic.h)
target_include_directories(CC_Basic_cmock
  PUBLIC
    ${ZAF_UTILDIR}
    ${ZWAVE_API_DIR}
    ${ZPAL_API_DIR}
)
