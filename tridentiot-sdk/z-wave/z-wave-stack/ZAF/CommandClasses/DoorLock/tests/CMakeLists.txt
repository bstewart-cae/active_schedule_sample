################################################################################
# Add test for Door Lock command class.
################################################################################
set(test_CC_DoorLock_src
  test_CC_DoorLock.c
  ../src/CC_DoorLock.c
  ${ZAF_UTILDIR}/mocks/mock_ZW_TransportEndpoint.c
  ${ZAF_UTILDIR}/ZAF_CC_Invoker.c
  )
add_unity_test(NAME test_CC_DoorLock
               FILES ${test_CC_DoorLock_src}
               LIBRARIES mock
                         CC_CommonMock
                         test_common
                         ZAF_TSEMock
                         NodeMask
                         DebugPrintMock
                         AssertTest
                         cc_door_lock_config_api_cmock
                         cc_door_lock_config_io_cmock
                         CC_Supervision_cmock
                         cc_supervision_config_api_cmock
                         zaf_event_distributor_soc_cmock
                      )
target_include_directories(test_CC_DoorLock PUBLIC
  ".."
  ${ZAF_CCDIR}/Association
  ${ZAF_UTILDIR}/TrueStatusEngine
  ${ZAF_UTILDIR}

)

# ###############################################################################
# Add test for Door Lock NVM Module
# ###############################################################################
set(test_cc_door_lock_nvm_src
  test_cc_door_lock_nvm.c
  ../src/cc_door_lock_nvm.c
)
add_unity_test(NAME test_cc_door_lock_nvm
  FILES ${test_cc_door_lock_nvm_src}
  LIBRARIES 
            cc_door_lock_config_api_cmock
            DebugPrint_cmock
            ZAF_nvm_app_cmock
            ZW_Assert_unity2
  USE_UNITY_WITH_CMOCK
)
target_include_directories(test_cc_door_lock_nvm PUBLIC
  ".."
  ../inc
  ../src
  ${ZAF_UTILDIR}
)
