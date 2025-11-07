zw_add_object_cc(NAME CC_WakeUp
  INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/config
  SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/CC_WakeUp.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cc_wake_up_config_api.c
  DEPENDS
    SwTimer
)

add_test_subdirectory(tests)
