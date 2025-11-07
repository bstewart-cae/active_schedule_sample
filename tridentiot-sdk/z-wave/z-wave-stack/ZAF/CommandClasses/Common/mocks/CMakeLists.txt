add_library(CC_CommonMock CC_Common_mock.c)
target_include_directories(CC_CommonMock PUBLIC 
  ${CMAKE_CURRENT_SOURCE_DIR}/..
  ${ZWAVE_API_DIR}
  PRIVATE
    ${ZAF_UTILDIR}
)
target_link_libraries(CC_CommonMock
  mock
  zpal
)

add_mock(CC_Common_cmock ${ZAF_CCDIR}/Common/CC_Common.h)
target_include_directories(CC_Common_cmock
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${ZAF_UTILDIR}
    ${ZWAVE_API_DIR}
    ${ZPAL_API_DIR}
    ${ZAF_CONFIGDIR}/config
)
