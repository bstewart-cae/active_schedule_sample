# SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
#
# SPDX-License-Identifier: BSD-3-Clause

# Mock setup for Version V4
add_mock(cc_version_migration_api_cmock ${ZAF_CCDIR}/Version/inc/cc_version_migration_api.h)
target_include_directories(cc_version_migration_api_cmock PUBLIC
                           ../inc
                           ../config
                           ${ZPAL_API_DIR}
                           ${ZWAVE_API_DIR}
                           ${ZAF_UTILDIR}
                           ${ZAF_CCDIR}/Common
)

add_mock(cc_version_migration_config_api_cmock ${ZAF_CCDIR}/Version/inc/cc_version_migration_config_api.h)
target_include_directories(cc_version_migration_config_api_cmock PUBLIC
                           ../inc
                           ../config
                           ${ZPAL_API_DIR}
                           ${ZWAVE_API_DIR}
                           ${ZAF_UTILDIR}
                           ${ZAF_CCDIR}/Common
)

# Mock setup for legacy tests
add_library(CC_VersionMock CC_Version_mock.c)
target_include_directories(CC_VersionMock PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/..
  ${ZAF_UTILDIR}
  ${ZAF_UTILDIR}/TargetSpecificIncludes
  ${ZWAVE_API_DIR}
)
target_link_libraries(CC_VersionMock CC_CommonMock QueueNotifyingMock mock)
