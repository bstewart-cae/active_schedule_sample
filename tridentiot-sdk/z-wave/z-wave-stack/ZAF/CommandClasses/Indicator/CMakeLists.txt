zw_add_object_cc(NAME CC_Indicator
  INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
  SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/CC_Indicator.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/CC_Indicator_weak.c
  DEPENDS
    ZAF_TSE_weak
)

add_test_subdirectory(tests)
add_test_subdirectory(mocks)
