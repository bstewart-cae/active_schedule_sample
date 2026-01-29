/*
 * SPDX-License-Identifier: BSD-3-Clause
 * SPDX-FileCopyrightText: 2025 Z-Wave Alliance <https://z-wavealliance.org>
 * SPDX-FileCopyrightText: 2025 Card Access Engineering, LLC. <https://www.caengineering.com>
 */
/**
 * @file cc_active_schedule_config_api.h
 */

#ifndef _CC_ACTIVE_SCHEDULE_CONFIG_API_H_
#define _CC_ACTIVE_SCHEDULE_CONFIG_API_H_

#include "CC_ActiveSchedule_types.h"
#include "cc_active_schedule_config.h"

/**
 * @returns The number of CCs supported by the node that use Active Scheduling
 */
uint8_t cc_active_schedule_get_num_supported_ccs();

/**
 * @brief Determines if scheduling a particular Command Class is supported by the node
 *
 * @param cc_id  Command Class ID in question
 * @return true  Scheduling is supported
 * @return false Scheduling is not supported
 */
bool cc_active_schedule_is_cc_supported(const uint8_t cc_id);

#endif
