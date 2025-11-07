zw_add_interface_cc(NAME CC_FirmwareUpdate
  INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/config
  SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/CC_FirmwareUpdate.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ota_util.c
  DEPENDS
    CRC
    SwTimer
)

add_test_subdirectory (tests)
