add_mock(cc_association_group_info_cmock ${ZAF_CCDIR}/Association/src/CC_AssociationGroupInfo.h)
target_include_directories(cc_association_group_info_cmock
  PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/../inc"
    ${ZAF_CCDIR}/Common
    ${ZAF_UTILDIR}
    ${ZPAL_API_DIR}
    ${ZWAVE_API_DIR}
    ${ZAF_UNITTESTEXTERNALS}
    ${ZAF_CONFIGDIR}/config
    ../config
)

add_mock(cc_association_config_api_cmock ${ZAF_CCDIR}/Association/inc/cc_agi_config_api.h)
target_include_directories(cc_association_config_api_cmock
  PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/.."
    ${ZAF_CCDIR}/Common
    ${ZAF_UTILDIR}
    ${ZPAL_API_DIR}
    ${ZWAVE_API_DIR}
    ${ZAF_UNITTESTEXTERNALS}
    ${ZAF_CONFIGDIR}/config
    ../config
)

add_mock(cc_association_cmock ${ZAF_CCDIR}/Association/inc/association_plus_base.h)
target_include_directories(cc_association_cmock
  PUBLIC
    ${ZAF_UTILDIR}
    ${ZPAL_API_DIR}
    ${ZWAVE_API_DIR}
    ../config
)
