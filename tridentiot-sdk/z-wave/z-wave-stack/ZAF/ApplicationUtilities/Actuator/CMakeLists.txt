add_library(ZAF_Actuator OBJECT EXCLUDE_FROM_ALL
  ${CMAKE_CURRENT_SOURCE_DIR}/ZAF_Actuator.c
)
target_include_directories(ZAF_Actuator
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
  PRIVATE
    ${ZAF_UTILDIR}
)
target_link_libraries(ZAF_Actuator
  PUBLIC
    SwTimer
  PRIVATE
    DebugPrint
    ZWaveAPI
    m
)

#Unit test
add_test_subdirectory(mocks)
add_test_subdirectory(tests)
