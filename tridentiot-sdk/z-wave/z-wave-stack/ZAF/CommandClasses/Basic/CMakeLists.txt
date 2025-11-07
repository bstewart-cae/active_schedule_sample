zw_add_object_cc(NAME CC_Basic
  INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
  SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/CC_Basic.c
)

zw_add_object_cc(NAME CC_BasicController
  INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}/inc/
  SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/CC_BasicController.c
)

add_test_subdirectory (tests)
add_test_subdirectory (mocks)
