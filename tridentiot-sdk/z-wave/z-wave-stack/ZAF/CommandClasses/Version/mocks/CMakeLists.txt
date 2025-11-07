add_library(CC_VersionMock CC_Version_mock.c)
target_include_directories(CC_VersionMock PUBLIC 
  ${CMAKE_CURRENT_SOURCE_DIR}/..
  ${ZAF_UTILDIR}
  ${ZAF_UTILDIR}/TargetSpecificIncludes
  ${ZWAVE_API_DIR}
)
target_link_libraries(CC_VersionMock CC_CommonMock QueueNotifyingMock mock)
