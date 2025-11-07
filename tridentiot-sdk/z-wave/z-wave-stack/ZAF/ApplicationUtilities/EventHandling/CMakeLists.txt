add_library(ZAF_EventDistributor_ncp OBJECT EXCLUDE_FROM_ALL
  ${CMAKE_CURRENT_SOURCE_DIR}/zaf_event_distributor.c
  ${CMAKE_CURRENT_SOURCE_DIR}/zaf_event_distributor_ncp.c
)

target_include_directories(ZAF_EventDistributor_ncp
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/config
)

target_link_libraries(ZAF_EventDistributor_ncp
  PUBLIC
    EventDistributor
  PRIVATE
    DebugPrint
    SwTimer
    SyncEvent
    Utils
    ZWaveAPI
    ZAF_AppTimer
    ZAF_CommonInterface
)

add_library(ZAF_EventDistributor_soc OBJECT EXCLUDE_FROM_ALL
  ${CMAKE_CURRENT_SOURCE_DIR}/zaf_event_distributor.c
  ${CMAKE_CURRENT_SOURCE_DIR}/zaf_event_distributor_soc.c
)

target_include_directories(ZAF_EventDistributor_soc
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/config
    ${ZAF_UTILDIR}/nvm
    ${ZAF_CCDIR}/Indicator/inc
)

target_link_libraries(ZAF_EventDistributor_soc
  PUBLIC
    EventDistributor
  PRIVATE
    AppsHw
    Assert
    SwTimer
    CC_DeviceResetLocally
    ZWaveAPI
    ZAF_AppTimer
    ZAF_CommonInterface
)

add_library(zaf_job_helper OBJECT EXCLUDE_FROM_ALL
  ${CMAKE_CURRENT_SOURCE_DIR}/zaf_job_helper.c
)

target_include_directories(zaf_job_helper
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
  PRIVATE
    $<TARGET_PROPERTY:zpal_${PLATFORM_VARIANT},INTERFACE_INCLUDE_DIRECTORIES>
)

target_compile_definitions(zaf_job_helper
  PUBLIC
    ZAF_USE_LEGACY_JOB_HELPER
)

add_test_subdirectory(mocks)
add_test_subdirectory(tests)
