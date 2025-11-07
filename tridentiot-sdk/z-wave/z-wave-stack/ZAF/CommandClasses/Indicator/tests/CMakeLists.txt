set(test_CC_Indicator_src
  test_CC_Indicator.c
  ../src/CC_Indicator.c
  ${ZAF_UTILDIR}/mocks/mock_ZW_TransportEndpoint.c
  ${ZAF_UTILDIR}/ZAF_CC_Invoker.c
  )

add_unity_test(NAME test_CC_Indicator
               FILES ${test_CC_Indicator_src}
               LIBRARIES mock
                         ZAF_TSEMock
                         NodeMask
                         FreeRtosMock
                         AssertTest
                         QueueNotifyingMock
                         test_common
                       )

target_include_directories(test_CC_Indicator PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/../inc/
  ${CMAKE_CURRENT_SOURCE_DIR}/../src/
  ${ZAF_UTILDIR}
  ${ZAF_UTILDIR}/mocks/inc
  ${ZAF_UTILDIR}/TrueStatusEngine
  ${ZAF_CCDIR}/Common
)
