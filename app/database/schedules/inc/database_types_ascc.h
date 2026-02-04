/*
 * SPDX-FileCopyrightText: 2026 Z-Wave Alliance <https://z-wavealliance.org>
 * SPDX-FileCopyrightText: 2026 Card Access Engineering, LLC <http://www.caengineering.com>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file database_types_ascc.h
 * @brief File containing the definitions for all the data types and structures
 *        used by the schedules and scheduling code.
 *
 */
#pragma once

/* CPP type safety */
#ifdef __cplusplus
extern "C" {
#endif

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include "CC_ActiveSchedule_types.h"
#include "cc_user_credential_config.h"

/****************************************************************************/
/*                      EXPORTED TYPES AND DEFINITIONS                      */
/****************************************************************************/
/**
 * @brief Packs relevant Year Day schedule information into single struct
 */
typedef struct year_day_nvm_ {
  bool occupied;
  ascc_year_day_schedule_t schedule;
} year_day_nvm_t;

/**
 * @brief Packs relevant Daily Repeating schedule information into single struct
 */
typedef struct daily_repeating_nvm_ {
  bool occupied;
  ascc_daily_repeating_schedule_t schedule;
} daily_repeating_nvm_t;

/**
 * Schedule metadata object for storage in NVM.
 *
 * This contains all of the schedule information for a given User.
 *
 * @note The schedules are stored in zero-indexed arrays - a schedule
 * 'slot' will correspond to its array index + 1 as slots are 1-indexed.
 *
 * If there are no supported schedules for a given type in the config, instead, an empty
 * dummy pointer is provided and MUST be handled appropriately by the application.
 */
typedef struct schedule_metadata_nvm_t_ {
  uint16_t uuid;
  bool scheduling_active;
#if (CC_USER_CREDENTIAL_YEAR_DAY_SCHEDULES_PER_USER > 0)
  year_day_nvm_t year_day_schedules[CC_USER_CREDENTIAL_YEAR_DAY_SCHEDULES_PER_USER];
#else
  year_day_nvm_t * year_day_schedules;
#endif
#if (CC_USER_CREDENTIAL_DAILY_REPEATING_SCHEDULES_PER_USER > 0)
  daily_repeating_nvm_t daily_repeating_schedules[CC_USER_CREDENTIAL_DAILY_REPEATING_SCHEDULES_PER_USER];
#else
  daily_repeating_nvm_t * daily_repeating_schedules;
#endif
} schedule_metadata_nvm_t;

/****************************************************************************/
/*                      EXPORTED FUNCTION DECLARATIONS                      */
/****************************************************************************/

#ifdef __cplusplus
}
#endif /* __cplusplus */
