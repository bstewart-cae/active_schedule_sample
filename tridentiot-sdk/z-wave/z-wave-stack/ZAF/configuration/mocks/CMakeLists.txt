add_library(zaf_config_mock zaf_config_mock.c)

target_include_directories(zaf_config_mock 
  PUBLIC
    "../inc"
    "../config"
    ${ZWAVE_API_DIR}
)

target_link_libraries(zaf_config_mock mock)

# Create CMock
add_mock(zaf_config_api_cmock ${ZAF_CONFIGDIR}/inc/zaf_config_api.h)
target_include_directories(zaf_config_api_cmock
  PUBLIC 
    ${ZW_ROOT}/ZWave/API
)
