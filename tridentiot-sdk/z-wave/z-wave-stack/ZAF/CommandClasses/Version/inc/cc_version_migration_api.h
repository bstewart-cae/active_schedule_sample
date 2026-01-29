/*
 * SPDX-FileCopyrightText: 2025 Z-Wave Alliance <https://z-wavealliance.org/>
 * SPDX-FileCopyrightText: 2025 Card Access Engineering, LLC <https://www.caengineering.com/>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

/**
 * @file cc_version_migration_api.h
 * This file defines the API for managing migration operations for Version CC, V4.
 *
 * These functions are intended to be implemented in the application layer as
 * implementation details are highly dependent on the use case.
 *
 */

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include "cc_version_migration_config.h"
#include "cc_version_migration_types.h"
#include "ZAF_types.h"
/****************************************************************************/
/*                       PUBLIC TYPES and DEFINITIONS                       */
/****************************************************************************/

/****************************************************************************/
/*                      EXPORTED FUNCTION DEFINITIONS                       */
/****************************************************************************/
/**
 * @brief Retrieves the current migration status for a given operation.
 *
 * @param operation Which operation to query
 * @param[out] state The state of the operation in question
 *
 * @return True if the operation is valid, false if not supported or uninitialized
 */
bool CC_Version_Migration_GetStatus(
    const cc_version_migration_operation_t operation,
    cc_version_migration_operation_state_t *state
    );

/**
 * @brief Starts the provided migration operation, if supported.
 *
 * @param operation Which operation to query
 *
 * @return True if the operation is valid, false if not supported or uninitialized
 */
bool CC_Version_Migration_Start(
    const cc_version_migration_operation_t operation
    );
