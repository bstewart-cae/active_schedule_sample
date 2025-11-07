zw_add_object_cc(NAME CC_ManufacturerSpecific
  INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}
  SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/CC_ManufacturerSpecific.c
)

add_test_subdirectory(tests)
add_test_subdirectory(test_utils)
