add_mock(zaf_cc_list_generator_cmock ${CMAKE_CURRENT_SOURCE_DIR}/../zaf_cc_list_generator.h)
include_directories(zaf_cc_list_generator_cmock
  PUBLIC
    ${ZAF_UTILDIR}
    ${ZWAVE_API_DIR}
)
target_link_libraries(zaf_cc_list_generator_cmock
  PRIVATE
    zpal
)