/*
 * SPDX-FileCopyrightText: 2025 Z-Wave Alliance <https://z-wavealliance.org/>
 * SPDX-FileCopyrightText: 2025 Card Access Engineering, LLC <https://www.caengineering.com/>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

/**
 * @file cc_version_migration_config_weak.c
 *
 * @brief This file controls the various migration process(es) supported by
 *        the supporting node.
 *
 */

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include "cc_version_migration_config.h"
#include "cc_version_migration_config_api.h"
#include "ZW_typedefs.h"

/****************************************************************************/
/*                             WEAK FUNCTIONS                               */
/****************************************************************************/

ZW_WEAK bool cc_version_is_migration_supported(void)
{
    return (CC_VERSION_MIGRATION_SUPPORTED == 0x01);
}

ZW_WEAK bool cc_version_is_migration_operation_supported(
    const cc_version_migration_operation_t operation)
{
    switch (operation) {
        case CC_VERSION_MIGRATION_OPERATION_USER_CODE_TO_U3C: {
            return (CC_VERSION_ENABLE_MIGRATION_USER_CODE_TO_U3C == 1);
        }
        case CC_VERSION_MIGRATION_OPERATION_U3C_TO_USER_CODE: {
            return (CC_VERSION_ENABLE_MIGRATION_U3C_TO_USER_CODE == 1);
        }
        default:
            return false;
    }
}
