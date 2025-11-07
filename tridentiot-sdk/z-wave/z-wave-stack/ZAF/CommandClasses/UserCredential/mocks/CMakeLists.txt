################################################################################
# Add mock for User Credential Command Class Configuration API.
################################################################################
add_mock(cc_user_credential_config_api_cmock ${ZAF_CCDIR}/UserCredential/inc/cc_user_credential_config_api.h)
target_include_directories(cc_user_credential_config_api_cmock PUBLIC
                           ../inc
                           ../config
                           ${ZPAL_API_DIR}
                           ${ZWAVE_API_DIR}
                           ${ZAF_UTILDIR}
                           ${ZAF_CCDIR}/Common
)

add_mock(cc_user_credential_io_cmock ${ZAF_CCDIR}/UserCredential/inc/cc_user_credential_io.h)
target_include_directories(cc_user_credential_io_cmock PUBLIC
                           ../inc
                           ../config
                           ${ZPAL_API_DIR}
                           ${ZWAVE_API_DIR}
                           ${ZAF_UTILDIR}
                           ${ZAF_CCDIR}/Common
)
