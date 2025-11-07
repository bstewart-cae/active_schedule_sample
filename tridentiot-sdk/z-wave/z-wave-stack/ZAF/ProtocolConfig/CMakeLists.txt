add_test_subdirectory (mocks)

add_library(zaf_protocol_config_interface INTERFACE)
target_include_directories(zaf_protocol_config_interface
  INTERFACE
    ${ZAF_PROTOCOL_CONFIG}/inc
)
