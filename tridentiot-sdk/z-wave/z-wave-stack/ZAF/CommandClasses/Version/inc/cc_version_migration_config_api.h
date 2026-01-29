/*
 * SPDX-FileCopyrightText: 2025 Z-Wave Alliance <https://z-wavealliance.org/>
 * SPDX-FileCopyrightText: 2025 Card Access Engineering, LLC <https://www.caengineering.com/>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

/**
 * @file cc_version_migration_config_api.h
 * This file defines the API for retrieving migration operations for Version, V4.
 *
 * These have a useable weak implementation out of the gate, but the application
 * developer is free to override these functions.
 *
 */

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include "cc_version_migration_types.h"

/****************************************************************************/
/*                       PUBLIC TYPES and DEFINITIONS                       */
/****************************************************************************/


/****************************************************************************/
/*                      EXPORTED FUNCTION DEFINITIONS                       */
/****************************************************************************/

/**
 * @brief Checks if migration is supported at all on the node
 *
 * @returns true if node has migration support, false otherwise
 */
bool cc_version_is_migration_supported(void);

/**
 * @brief Checks if the node supports a particular migration operation
 *
 * @param operation Migration operation to query
 * @returns true if node supports that particular operation
 */
bool cc_version_is_migration_operation_supported(
    const cc_version_migration_operation_t operation
    );
