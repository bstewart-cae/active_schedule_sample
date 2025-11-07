zw_add_object_cc(NAME CC_Battery
  INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/config
  SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/CC_Battery.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cc_battery_io_weak.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cc_battery_config_api.c
)

zw_add_object_cc(NAME CC_Battery_nvm
  INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
  SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cc_battery_nvm.c
)

add_test_subdirectory(mocks)
add_test_subdirectory(tests)
