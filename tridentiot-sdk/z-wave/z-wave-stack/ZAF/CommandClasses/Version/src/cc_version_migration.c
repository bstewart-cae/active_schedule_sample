/*
 * SPDX-FileCopyrightText: 2025 Z-Wave Alliance <https://z-wavealliance.org/>
 * SPDX-FileCopyrightText: 2025 Card Access Engineering, LLC <https://www.caengineering.com/>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

/**
 * @file cc_version_migration.c
 *
 * @brief Empty migration function implementations. Most devices
 *        do not support this particular feature, but are required to include the
 *        Version CC as a core component of Z-Wave.
 *        Supporting nodes can override these as needed.
 *
 */

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include "cc_version_migration_types.h"
#include "ZW_typedefs.h"

/****************************************************************************/
/*                      PRIVATE TYPES and DEFINITIONS                       */
/****************************************************************************/

/****************************************************************************/
/*                              PRIVATE DATA                                */
/****************************************************************************/

/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/

/****************************************************************************/
/*                             WEAK FUNCTIONS                               */
/****************************************************************************/

ZW_WEAK bool CC_Version_Migration_GetStatus(
  __attribute__((unused)) const cc_version_migration_operation_t operation,
  __attribute__((unused)) cc_version_migration_operation_state_t *state
)
{
  return false;
}

ZW_WEAK bool CC_Version_Migration_Start(
    __attribute__((unused)) const cc_version_migration_operation_t operation
)
{
  return false;
}

/****************************************************************************/
/*                            PRIVATE FUNCTIONS                             */
/****************************************************************************/

