/*
 * SPDX-License-Identifier: BSD-3-Clause
 * SPDX-FileCopyrightText: 2025 Card Access Engineering, LLC <https://www.caengineering.com/>
 */
/**
 * @file database_properties.h
 *
 * @brief This file contains compile time definitions and properties that define the
 *        database layout and memory regions.
 * 
 * @date 2025-11-10
 * @author bstewart (Card Access Engineering, LLC)
 */

#ifndef DATABASE_PROPERTIES_H_
#define DATABASE_PROPERTIES_H_

/****************************************************************************
 *                              INCLUDE FILES                               *
 ****************************************************************************/
// Module includes should always go first and be listed alphabetically

// Stack/SDK includes should always go second to last and be listed alphabetically

// Language includes should always go last and be listed alphabetically

// Must go below includes above
#ifdef __cplusplus
extern "C" {
#endif

/****************************************************************************
 *                     EXPORTED TYPES and DEFINITIONS                       *
 ****************************************************************************/
#define MAX_SCHEDULE_PER_USER 32 //< DO NOT EDIT!

#ifndef MAX_YEAR_DAY_SCHEDULES_PER_USER
/**
 * @brief Number of Year Day schedule slots stored with each user
 */
#define MAX_YEAR_DAY_SCHEDULES_PER_USER 1
#endif /* MAX_YEAR_DAY_SCHEDULES_PER_USER */

#ifndef MAX_DAILY_REPEATING_SCHEDULES_PER_USER
/**
 * @brief Number of Daily Repeating schedule slots stored with each user
 */
#define MAX_DAILY_REPEATING_SCHEDULES_PER_USER 3
#endif

#if (MAX_YEAR_DAY_SCHEDULES_PER_USER > MAX_SCHEDULE_PER_USER) || \
    (MAX_DAILY_REPEATING_SCHEDULES_PER_USER > MAX_SCHEDULE_PER_USER)
#error "Due to limitations of the application sample, please limit schedules to 32 per user or fewer"
#endif 
/**
 * @brief Maximum number of User and User Name objects that can be stored in the NVM
 */
#define MAX_USER_OBJECTS                   \
  ZAF_FILE_ID_CC_USER_CREDENTIAL_USER_LAST \
  - ZAF_FILE_ID_CC_USER_CREDENTIAL_USER_BASE

/**
 * @brief Maximum number of Credential and Credential Data objects that can be stored
 *        in the NVM
 */
#define MAX_CREDENTIAL_OBJECTS                   \
  ZAF_FILE_ID_CC_USER_CREDENTIAL_CREDENTIAL_LAST \
  - ZAF_FILE_ID_CC_USER_CREDENTIAL_CREDENTIAL_BASE

/****************************************************************************
 *                              EXPORTED DATA                               *
 ****************************************************************************/

/****************************************************************************
 *                     EXPORTED FUNCTION DECLARATIONS                       *
 ****************************************************************************/

// Must go after exported function declarations
#ifdef __cplusplus
}
#endif

#endif /* DATABASE_PROPERTIES_H_ */
