add_library(CC_Common OBJECT EXCLUDE_FROM_ALL
  ${CMAKE_CURRENT_SOURCE_DIR}/CC_Common.c
)
target_include_directories(CC_Common
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${ZAF_UTILDIR}
  PRIVATE
    ${ZAF_CCDIR}/Association/
)
target_link_libraries(CC_Common
  PUBLIC
    ZWaveAPI
  PRIVATE
    zaf_transport_layer_interface
)

add_test_subdirectory(mocks)
