################################################################################
# SPDX-License-Identifier: BSD-3-Clause
# SPDX-FileCopyrightText: 2025 Card Access Engineering, LLC.
################################################################################

################################################################################
# Common variables for Active Schedule command class tests.
################################################################################

set(test_ascc_common_sources
  ../src/CC_ActiveSchedule.c
  ${ZAF_UTILDIR}/ZAF_CC_Invoker.c
)

set(test_ascc_common_libraries
  test_common
  CC_Common_cmock
  CC_Supervision_cmock
  cc_active_schedule_config_api_cmock
  Utils
  zaf_transport_layer_cmock
  ZAF_Common_interface_cmock
  zaf_event_distributor_soc_cmock
  ZAF_TSE_cmock
  AppTimer_cmock
  SwTimerCMock
  DebugPrintMock
  CRC
  zpal_cmock
  ZW_TransportSecProtocol_cmock
)

################################################################################
# Add test for Active Schedule integration with the User Credential command class.
################################################################################

set(test_CC_ActiveSchedule_u3c_src
  test_CC_ActiveSchedule_u3c.c
  ${test_ascc_common_sources}
)

add_unity_test(NAME test_CC_ActiveSchedule_u3c
               FILES ${test_CC_ActiveSchedule_u3c_src}
               LIBRARIES ${test_ascc_common_libraries}
                         cc_active_schedule_io_cmock
                         cc_user_credential_config_api_cmock
                         cc_user_credential_io_cmock
                         ZAF_nvm_app_cmock
               USE_UNITY_WITH_CMOCK
)
target_include_directories(test_CC_ActiveSchedule_u3c
  PRIVATE
    ../config
    ../inc
)

## TODO: Add tests below for integration with other command classes as appropriate.