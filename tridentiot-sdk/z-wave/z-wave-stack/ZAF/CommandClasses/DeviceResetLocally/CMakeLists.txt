zw_add_object_cc(NAME CC_DeviceResetLocally
  INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}
  SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/CC_DeviceResetLocally.c
    ${CMAKE_CURRENT_SOURCE_DIR}/CC_DeviceResetLocally_weak.c
)

add_test_subdirectory (mocks)
add_test_subdirectory (tests)
