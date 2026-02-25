/*
 * SPDX-FileCopyrightText: 2025 Z-Wave Alliance <https://z-wavealliance.org>
 * SPDX-FileCopyrightText: 2026 Card Access Engineering, LLC <http://www.caengineering.com>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file app_schedules.h
 * @author bstewart-cae
 * @brief This file contains the application level logic for the scheduling
 *        implementation of User Credential v2.
 *
 * @copyright 2026 Card Access Engineering, LLC on behalf of the Z-Wave Alliance
 */

#ifndef _APP_SCHEDULES_H_
#define _APP_SCHEDULES_H_

/* CPP type safety */
#ifdef __cplusplus
extern "C" {
#endif

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include "database_types_ascc.h"
/****************************************************************************/
/*                      EXPORTED TYPES AND DEFINITIONS                      */
/****************************************************************************/

/****************************************************************************/
/*                      EXPORTED FUNCTION DECLARATIONS                      */
/****************************************************************************/

/**
 * @brief Active Schedule CC requires that separate function stubs be provided
 *        to the stack so for each CC that is scheduled.
 */
void app_sch_initialize_handlers(void);

/**
 * @brief Helper function called to prepopulate a user's schedule slots on bootup
 */
void app_sch_init_schedule_db(void);

/**
 * @brief Clear all stored schedule information
 */
void app_sch_reset_schedules(void);

/** 
 * @brief Helper function that Gets a given user from the database and deletes all schedules of a 
 *        given type for that user. If the user does not exist, nothing will happen.
 * 
 * @note  Providing a UUID of 0 in this case will find the first available user and delete
 *        the schedules for the first user, rather than deleting schedules for all of them.
 *
 * @param uuid          User Unique identifier in question
 * @param schedule_type Which type of schedule (Year Day/Daily Repeating) to clear from the user.
 * 
 * @returns true if successful, false otherwise.
 */
bool app_sch_local_delete_for_user(const uint16_t p_uuid, const ascc_type_t p_schedule_type);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* _APP_SCHEDULES_H_ */
