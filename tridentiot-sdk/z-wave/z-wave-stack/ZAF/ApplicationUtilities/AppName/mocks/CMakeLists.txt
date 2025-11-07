add_mock(ZAF_AppName_cmock ${ZAF_UTILDIR}/AppName/ZAF_AppName.h)
target_include_directories(ZAF_AppName_cmock
  PUBLIC
    ${ZAF_UTILDIR}/AppName
    ${ZAF_UTILDIR}/AppName/config
    ${ZWAVE_API_DIR}
)
