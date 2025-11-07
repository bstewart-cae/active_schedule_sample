add_library(ZAF_CommonInterface OBJECT EXCLUDE_FROM_ALL
  ${CMAKE_CURRENT_SOURCE_DIR}/ZAF_Common_interface.c
)

target_include_directories(ZAF_CommonInterface
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(ZAF_CommonInterface
  PUBLIC
    DebugPrint
    ZWaveAPI
    ZAF_AppName
    zaf_protocol_config_interface
)

add_test_subdirectory(tests)
add_test_subdirectory(mocks)
