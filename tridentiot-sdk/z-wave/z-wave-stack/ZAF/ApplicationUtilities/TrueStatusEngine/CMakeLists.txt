add_library(ZAF_TSE OBJECT EXCLUDE_FROM_ALL
  ${CMAKE_CURRENT_SOURCE_DIR}/ZAF_TSE.c
)

target_include_directories(ZAF_TSE
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/config
    ${ZAF_UTILDIR}
  PRIVATE
    $<TARGET_PROPERTY:CC_Association,INTERFACE_INCLUDE_DIRECTORIES>
)

target_link_libraries(ZAF_TSE
  PUBLIC
    ZWaveAPI
    CC_Common
    zaf_transport_layer_interface
  PRIVATE
    DebugPrint
)

add_library(ZAF_TSE_weak OBJECT EXCLUDE_FROM_ALL
  ${CMAKE_CURRENT_SOURCE_DIR}/ZAF_TSE_weak.c
)

target_include_directories(ZAF_TSE_weak
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${ZAF_UTILDIR}
)

target_link_libraries(ZAF_TSE_weak
  PUBLIC
    ZWaveAPI
    CC_Common
    zaf_transport_layer_interface
)

#Unit test
add_test_subdirectory(mocks)
add_test_subdirectory(tests)
