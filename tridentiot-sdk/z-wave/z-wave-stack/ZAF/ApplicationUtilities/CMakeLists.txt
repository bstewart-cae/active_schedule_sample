add_library(ZAF_AppTimer OBJECT EXCLUDE_FROM_ALL
  ${ZAF_UTILDIR}/AppTimer.c
  # Only needed for apps that go into Deep Sleep
  ${ZAF_UTILDIR}/AppTimerDeepSleep.c
)

target_include_directories(ZAF_AppTimer
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(ZAF_AppTimer
  PUBLIC
    ZWaveAPI
    SwTimer
  PRIVATE
    DebugPrint
    Utils
)

add_library(ZAF_nvm OBJECT EXCLUDE_FROM_ALL
  ${ZAF_UTILDIR}/ZAF_nvm.c
  ${ZAF_UTILDIR}/ZAF_nvm_app.c
)

target_include_directories(ZAF_nvm
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(ZAF_nvm
  PUBLIC
    zpal
)

add_library(ZAF_retention_register OBJECT EXCLUDE_FROM_ALL
  ${ZAF_UTILDIR}/ZAF_retention_register.c
)

target_include_directories(ZAF_retention_register
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(ZAF_retention_register
  PUBLIC
    zpal
  PRIVATE
    ZAF_AppTimer
)

add_subdirectory(AppName)
add_subdirectory(CCListGenerator)
add_subdirectory(EventHandling)
add_subdirectory(_commonIF)
add_subdirectory(TrueStatusEngine)
add_subdirectory(Actuator)
add_subdirectory(nvm)

add_test_subdirectory(mocks)
add_test_subdirectory(tests)
