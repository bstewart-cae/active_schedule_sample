/*
 * SPDX-FileCopyrightText: 2025 Z-Wave Alliance <https://z-wavealliance.org/>
 * SPDX-FileCopyrightText: 2025 Card Access Engineering, LLC <https://www.caengineering.com/>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

/**
 * @file cc_version_migration_types.h
 * @brief This file contains the various defines and data structures used by the migration
 *        processes.
 *        By design, this needs to be as flexible as possible to allow for future additions of
 *        other migration operations.
 *
 */


 #ifndef CC_VERSION_MIGRATION_TYPES
 #define CC_VERSION_MIGRATION_TYPES
/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include "ZAF_types.h"
#include <stdint.h>
#include <stdbool.h>

/****************************************************************************/
/*                       PUBLIC TYPES and DEFINITIONS                       */
/****************************************************************************/

/**
 * @brief Enumerated values for migration operation identifiers.
 */
typedef enum _cc_version_migration_operation_ {
    /* Version, V4*/
    CC_VERSION_MIGRATION_OPERATION_USER_CODE_TO_U3C = 0x01,    ///< Migrate User Codes to Users and PIN Codes
    CC_VERSION_MIGRATION_OPERATION_U3C_TO_USER_CODE = 0x02,    ///< Migrate Users and PIN Codes to User Codes
    /* Add additional migration operations above */
    CC_VERSION_MIGRATION_OPERATION_EOT,                        ///< Placeholder for iteration purposes
} cc_version_migration_operation_t;

/**
 * @brief Enumerated values for the migration status of a particular migration
 */
typedef enum _cc_version_migration_status_ {
    MIGRATION_STATUS_READY = 0x00,            ///< Migration is ready to start
    MIGRATION_STATUS_IN_PROGRESS,             ///< Migration is currently in progress
    MIGRATION_STATUS_COMPLETE_SUCCEEDED,      ///< Migration operation finished and successfully completed
    MIGRATION_STATUS_COMPLETE_FAILED,         ///< Migration operation finished and failed to complete
    MIGRATION_STATUS_NOT_SUPPORTED = 0xFE,    ///< Migration operation is not supported
} cc_version_migration_status_t;

/**
 * @brief This defines the state machine for a given migration operation
 */
typedef struct cc_version_migration_operation_state_ {
    cc_version_migration_status_t status; ///< Status of the current operation
    uint16_t remaining_time;              ///< Remaining time in seconds
} cc_version_migration_operation_state_t;

/**
 * @brief Holder struct for Version CC event data
 */
typedef struct cc_version_migration_event_data_ {
    RECEIVE_OPTIONS_TYPE_EX * rx_opts;              ///< Copy of the rx_opts
    cc_version_migration_operation_t operation;
    cc_version_migration_operation_state_t state;
} cc_version_migration_event_data_t;

/**
 * @brief List of Version v4 application events
 */
typedef enum cc_version_app_events_ {
    MIGRATION_EVENT_ON_STATUS_CHANGE ///< Migration operation has completed,
                                          ///  result is baked into the event payload
} cc_version_app_events_t;

#endif /* CC_VERSION_MIGRATION_TYPES */
