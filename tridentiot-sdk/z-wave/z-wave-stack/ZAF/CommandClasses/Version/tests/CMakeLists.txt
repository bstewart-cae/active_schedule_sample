# SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
#
# SPDX-License-Identifier: BSD-3-Clause

################################################################################
# Add test for Version command class.
################################################################################

set(test_CC_Version_src
  test_CC_Version.c
  ../src/CC_Version.c
  ../mocks/CC_Version_mock_extern.c
  ${ZWAVE_MOCKS_DIR}/mock_system_startup.c
  ${ZAF_UTILDIR}/mocks/mock_ZW_TransportEndpoint.c
  ${ZAF_UTILDIR}/mocks/ZW_TransportSecProtocol_mock.c
  ${ZAF_UTILDIR}/ZAF_CC_Invoker.c
  ${ZWAVE_MOCKS_DIR}/ZW_build_number_mock.c
  )
add_unity_test(NAME test_CC_Version
               FILES ${test_CC_Version_src}
               LIBRARIES mock
                         cc_association_cmock
                         cc_version_migration_config_api_cmock
                         cc_version_migration_api_cmock
                         test_common
                         CC_CommonMock
                         ZAF_CommonInterfaceMock
                         ZAF_TSE_cmock
                         zaf_event_distributor_soc_cmock
                         zaf_config_mock
                         zpal_mock
                         Utils)
target_compile_definitions(test_CC_Version PRIVATE
  test_CC_Version_defines
)
target_include_directories(test_CC_Version PUBLIC
  ..
  ../inc
  ../config
  ${CONFIG_DIR}
  ${ZAF_CONFIGDIR}/config
  ${ZWAVE_API_DIR}
)
