################################################################################
# SPDX-License-Identifier: BSD-3-Clause
# SPDX-FileCopyrightText: 2025 Card Access Engineering, LLC.
################################################################################

################################################################################
# Add mock for Active Schedule Command Class APIs.
################################################################################
add_mock(cc_active_schedule_config_api_cmock ${ZAF_CCDIR}/ActiveSchedule/inc/cc_active_schedule_config_api.h)
target_include_directories(cc_active_schedule_config_api_cmock PUBLIC
                           ../inc
                           ../config
                           ${ZPAL_API_DIR}
                           ${ZWAVE_API_DIR}
                           ${ZAF_UTILDIR}
                           ${ZAF_CCDIR}/Common
)

add_mock(cc_active_schedule_io_cmock ${ZAF_CCDIR}/ActiveSchedule/inc/cc_active_schedule_io.h)
target_include_directories(cc_active_schedule_io_cmock PUBLIC
                           ../inc
                           ../config
                           ${ZPAL_API_DIR}
                           ${ZWAVE_API_DIR}
                           ${ZAF_UTILDIR}
                           ${ZAF_CCDIR}/Common
)