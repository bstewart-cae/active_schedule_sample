add_library(ZAF_AppName INTERFACE)
target_include_directories(ZAF_AppName
  INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/config
    ${ZWAVE_API_DIR}
)
target_sources(ZAF_AppName
  INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/ZAF_AppName_weak.c
)

add_test_subdirectory(mocks)
