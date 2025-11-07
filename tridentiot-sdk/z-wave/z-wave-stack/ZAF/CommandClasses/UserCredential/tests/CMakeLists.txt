################################################################################
# Common variables for User Credential command class tests.
################################################################################

set(test_u3c_common_sources
  ../src/CC_UserCredential.c
  ../src/cc_user_credential_tx.c
  ../src/cc_user_credential_manufacturer_validation_weak.c
  ../src/cc_user_credential_validation.c
  ../src/cc_user_credential_operations.c
  ../src/cc_user_credential_learn.c
  ../src/cc_user_credential_handlers_admin_code.c
  ../src/cc_user_credential_handlers_capabilities.c
  ../src/cc_user_credential_handlers_checksum.c
  ../src/cc_user_credential_handlers_database.c
  ${ZAF_UTILDIR}/ZAF_CC_Invoker.c
)

set(test_u3c_common_libraries
  test_common
  CC_Common_cmock
  cc_association_cmock
  CC_Supervision_cmock
  cc_user_credential_config_api_cmock
  Utils
  cc_notification_cmock
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
# Add test for User Credential command class.
################################################################################

set(test_CC_UserCredential_src
  test_CC_UserCredential.c
  ${test_u3c_common_sources}
)
add_unity_test(NAME test_CC_UserCredential
               FILES ${test_CC_UserCredential_src}
               LIBRARIES ${test_u3c_common_libraries}
                         cc_user_credential_io_cmock
                         ZAF_nvm_app_cmock
               USE_UNITY_WITH_CMOCK
)
target_include_directories(test_CC_UserCredential
  PRIVATE
    ../config
)

set(test_CC_UserCredential_admin_code_src
  test_CC_UserCredential_admin_code.c
  ${test_u3c_common_sources}
)
add_unity_test(NAME test_CC_UserCredential_admin_code
               FILES ${test_CC_UserCredential_admin_code_src}
               LIBRARIES ${test_u3c_common_libraries}
                         cc_user_credential_io_cmock
                         ZAF_nvm_app_cmock
               USE_UNITY_WITH_CMOCK
)
target_include_directories(test_CC_UserCredential_admin_code
  PRIVATE
    ../config
)

set(test_CC_UserCredential_capabilities_src
  test_CC_UserCredential_capabilities.c
  ${test_u3c_common_sources}
)
add_unity_test(NAME test_CC_UserCredential_capabilities
               FILES ${test_CC_UserCredential_capabilities_src}
               LIBRARIES ${test_u3c_common_libraries}
                         cc_user_credential_io_cmock
                         ZAF_nvm_app_cmock
               USE_UNITY_WITH_CMOCK
)
target_include_directories(test_CC_UserCredential_capabilities
  PRIVATE
    ../config
)

set(test_CC_UserCredential_credential_learn_src
  test_CC_UserCredential_credential_learn.c
  ${test_u3c_common_sources}
)
add_unity_test(NAME test_CC_UserCredential_credential_learn
               FILES ${test_CC_UserCredential_credential_learn_src}
               LIBRARIES ${test_u3c_common_libraries}
                         cc_user_credential_io_cmock
               USE_UNITY_WITH_CMOCK
)
target_include_directories(test_CC_UserCredential_credential_learn
  PRIVATE
    ../config
)

set(test_CC_UserCredential_credential_move_src
  test_CC_UserCredential_credential_move.c
  ${test_u3c_common_sources}
)
add_unity_test(NAME test_CC_UserCredential_credential_move
               FILES ${test_CC_UserCredential_credential_move_src}
               LIBRARIES ${test_u3c_common_libraries}
                         cc_user_credential_io_cmock
               USE_UNITY_WITH_CMOCK
)
target_include_directories(test_CC_UserCredential_credential_move
  PRIVATE
    ../config
)

set(test_CC_UserCredential_io_src
  test_CC_UserCredential_io.c
  ../src/cc_user_credential_nvm.c
  ${test_u3c_common_sources}
)
add_unity_test(NAME test_CC_UserCredential_io
               FILES ${test_CC_UserCredential_io_src}
               LIBRARIES ${test_u3c_common_libraries}
                         ZAF_nvm_app_cmock
               USE_UNITY_WITH_CMOCK
)
target_include_directories(test_CC_UserCredential_io
  PRIVATE
    ../inc
    ../config
  PUBLIC
    ${ZAF_CCDIR}/_TestUtils
)

################################################################################
################################################################################
# C++ tests
################################################################################
################################################################################
#[[
set(test_CC_UserCredential_set_get_src
  test_CC_UserCredential.hpp
  test_CC_UserCredential_set_get.cpp
  ${test_u3c_common_sources}
)
add_unity_test(NAME test_CC_UserCredential_set_get
               FILES ${test_CC_UserCredential_set_get_src}
               LIBRARIES ${test_u3c_common_libraries}
                         cc_user_credential_io_cmock
                         zw_frame_parsers
               USE_UNITY_WITH_CMOCK
               USE_CPP
)
target_include_directories(test_CC_UserCredential_set_get
  PRIVATE
    ../config
)
]]
################################################################################
# Test for User Credential command class. checksum related functions
################################################################################

set(test_CC_UserCredential_checksum_src
  test_CC_UserCredential.hpp
  test_CC_UserCredential_checksum.cpp
  ${ZAF_CCDIR}/_TestUtils/test_common.c
  ${test_u3c_common_sources}
)
add_unity_test(NAME test_CC_UserCredential_checksum
               FILES ${test_CC_UserCredential_checksum_src}
               LIBRARIES ${test_u3c_common_libraries}
                         cc_user_credential_io_cmock
                         ZAF_nvm_app_cmock
               USE_UNITY_WITH_CMOCK
               USE_CPP
)
target_include_directories(test_CC_UserCredential_checksum
  PRIVATE
    ../inc
    ../config
  PUBLIC
    ${ZAF_CCDIR}/_TestUtils
)
