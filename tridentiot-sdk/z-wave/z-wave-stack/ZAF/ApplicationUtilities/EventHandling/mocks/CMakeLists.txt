add_library(ZAF_EventHandlingMock zaf_job_helper_mock.c zaf_event_distributor_mock.c)
target_include_directories(ZAF_EventHandlingMock PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/..")
target_link_libraries(ZAF_EventHandlingMock mock QueueNotifyingMock)

add_mock(zaf_job_helper_cmock ${CMAKE_CURRENT_SOURCE_DIR}/../zaf_job_helper.h)
target_link_libraries(zaf_job_helper_cmock
  PUBLIC
  QueueNotifying_cmock
)

add_mock_custom_config(
  TARGET
    zaf_event_distributor_soc_cmock
  CONFIG_FILE
    ${CMAKE_CURRENT_SOURCE_DIR}/zaf_event_distributor_soc_cmock_config.yml
  HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/../zaf_event_distributor_soc.h
)
target_link_libraries(zaf_event_distributor_soc_cmock
  PUBLIC
    EventDistributor_cmock
    ZWaveAPI  
)
