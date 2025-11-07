zw_add_object_cc(NAME CC_Version
  INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CONFIG_DIR}
  SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/CC_Version.c
)

add_test_subdirectory(tests)
add_test_subdirectory(mocks)
