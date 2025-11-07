/// ****************************************************************************
/// @file tr_hal_common.c
///
/// @brief This is the chip specific code for the T32CM11 Common HAL
///
///
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************

#include "tr_hal_common.h"
#include "tr_hal_platform.h"


/// ****************************************************************************
/// tr_hal_check_interrupt_priority
///
/// the interrupt priorities on CM11 are 0-7, with 0 being highest priority and
/// 7 being lowest priority. We should not allow use of 0, since this causes
/// problems with sleep and wake
/// ****************************************************************************
tr_hal_status_t tr_hal_check_interrupt_priority(tr_hal_int_pri_t interrupt_priority)
{
    // check for being too low
    if (interrupt_priority < TR_HAL_INTERRUPT_PRIORITY_1)
    {
        return TR_HAL_ERROR_INT_PRI_TOO_LOW;
    }

    // check for being too high
    if (interrupt_priority > TR_HAL_INTERRUPT_PRIORITY_7)
    {
        return TR_HAL_ERROR_INT_PRI_TOO_HIGH;
    }

    return TR_HAL_SUCCESS;
}

