add_library(zaf_transport_layer_interface INTERFACE)
target_include_directories(zaf_transport_layer_interface
  INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/inc
    ${CMAKE_CURRENT_SOURCE_DIR}/config
    ${ZAF_UTILDIR} 
)

add_library(zaf_transport_layer INTERFACE)
target_sources(zaf_transport_layer
  INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/zaf_transport_tx.c
)
target_include_directories(zaf_transport_layer
  INTERFACE
    ${ZAF_UTILDIR}/_commonIF
    ${ZAF_CCDIR}/Association/inc
    ${ZAF_CCDIR}/Association/config/
)
target_link_libraries(zaf_transport_layer
  INTERFACE
    ZWaveAPI
    DebugPrint
    zaf_transport_layer_interface
)

add_test_subdirectory (tests)
add_test_subdirectory (mocks)