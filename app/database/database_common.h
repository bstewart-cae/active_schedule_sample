/*
 * SPDX-FileCopyrightText: 2026 Card Access Engineering, LLC <http://www.caengineering.com>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file database_common.h
 * @author bstewart-cae
 * @brief This file contains the functions for managing the app-level functionality
 *        with regards to Schedules and Migration. Because these are implemented
 *        within the application layer, these are stored separately from the Users,
 *        Credentials and Key Locker information.
 *
 * @copyright 2026 Card Access Engineering, LLC on behalf of the Z-Wave Alliance
 */

#ifndef _DATABASE_COMMON_H_
#define _DATABASE_COMMON_H_

/* CPP type safety */
#ifdef __cplusplus
extern "C" {
#endif

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <stdbool.h>
#include "cc_user_credential_nvm.h"

/****************************************************************************/
/*                      EXPORTED TYPES AND DEFINITIONS                      */
/****************************************************************************/

/* Note that Key Locker data is stored in ZAF memory with the Users and Credentials. */
#define APP_NVM_FILE_MIGRATION_TABLE (1)
#define APP_NVM_FILE_MIGRATION_DATA_BASE (2)
#define APP_NVM_FILE_MIGRATION_DATA_END (7) // Up to 5 migration operations (Reserved for future use)
#define APP_NVM_FILE_SCHEDULE_DATA_BASE (8)
#define APP_NVM_FILE_SCHEDULE_DATA_END \
  (APP_NVM_FILE_SCHEDULE_DATA_BASE +   \
   CC_USER_CREDENTIAL_MAX_USER_UNIQUE_IDENTIFIERS) // One file for each user

typedef enum _app_nvm_area_ {
  APP_NVM_AREA_MIGRATION_TABLE, ///< Flash area for migration metadata
  APP_NVM_AREA_MIGRATION_DATA,  ///< Flash area for storing data for each migration operation
  APP_NVM_AREA_SCHEDULE_DATA,   ///< Flash area to store schedule data
} app_nvm_area_t;

// Simple typedef for readability's sake, and ensuring all NVM operations share the same
// definitions
typedef u3c_nvm_operation app_nvm_operation_t;

/****************************************************************************/
/*                      EXPORTED FUNCTION DECLARATIONS                      */
/****************************************************************************/

/**
 * @brief This function initializes the application database and all subsequent
 *        app-level subsystems.
 */
bool app_nvm_init(void);


/**
 * @brief  Execute an NVM read or write operation for application device.
 *
 * @return true if the operation has been executed successfully and more than 0
 *         bytes were transferred
 */
bool app_nvm(
  const app_nvm_operation_t operation, const app_nvm_area_t area, uint16_t offset, void* pData,
  uint16_t size);


#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* _DATABASE_COMMON_H_ */
