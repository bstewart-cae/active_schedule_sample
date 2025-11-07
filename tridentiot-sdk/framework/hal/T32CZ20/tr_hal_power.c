/// ****************************************************************************
/// @file tr_hal_power.c
///
/// @brief This contains the code for the Trident HAL POwer Mgmt for T32CZ20
///
///
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************

#include "tr_hal_power.h"

static bool rco1m_enabled = false;
static bool rco32k_enabled = false;


/// ****************************************************************************
/// tr_hal_power_enable_clock
///
/// ****************************************************************************
tr_hal_status_t tr_hal_power_enable_clock(tr_hal_clock_t clock)
{
    if (clock == TR_HAL_CLOCK_1M)
    {
        // Question: should we check if it is already enabled and send back
        // an error? or toggle it?

        POWER_MGMT_CHIP_REGISTERS->soc_pmu_rco1m |= PM_RCO_1M_ENABLE_BIT;
        rco1m_enabled = true;

        return TR_HAL_SUCCESS;
    }
    else if (clock == TR_HAL_CLOCK_32K)
    {
        // Question: should we check if it is already enabled and send back
        // an error? or toggle it?

        POWER_MGMT_CHIP_REGISTERS->pmu_osc_32k |= PM_RCO_32K_ENABLE_BIT;
        rco32k_enabled = true;

        return TR_HAL_SUCCESS;
    }

    return TR_HAL_ERROR_INVALID_PARAM;
}


/// ****************************************************************************
/// tr_hal_power_disable_clock
///
/// ****************************************************************************
tr_hal_status_t tr_hal_power_disable_clock(tr_hal_clock_t clock)
{
    if (clock == TR_HAL_CLOCK_1M)
    {
        // Question: should we check if it is already enabled and send back
        // an error? or toggle it?

        POWER_MGMT_CHIP_REGISTERS->soc_pmu_rco1m &= (uint32_t)(~PM_RCO_1M_ENABLE_BIT);
        rco1m_enabled = false;

        return TR_HAL_SUCCESS;
    }
    else if (clock == TR_HAL_CLOCK_32K)
    {
        // Question: should we check if it is already enabled and send back
        // an error? or toggle it?

        POWER_MGMT_CHIP_REGISTERS->pmu_osc_32k &= (uint32_t)(~PM_RCO_32K_ENABLE_BIT);
        rco32k_enabled = false;

        return TR_HAL_SUCCESS;
    }

    return TR_HAL_ERROR_INVALID_PARAM;
}


/// ****************************************************************************
/// tr_hal_power_is_clock_enabled
///
/// ****************************************************************************
tr_hal_status_t tr_hal_power_is_clock_enabled(tr_hal_clock_t clock,
                                              bool* is_enabled)
{
    // ptr result can't be NULL
    if (is_enabled == NULL)
    {
        return TR_HAL_ERROR_NULL_PARAMS;
    }

    // return value based on 1M clock
    if (clock == TR_HAL_CLOCK_1M)
    {
        (*is_enabled) = rco1m_enabled;
        return TR_HAL_SUCCESS;
    }

    // return value based on 32K clock
    else if (clock == TR_HAL_CLOCK_32K)
    {
        (*is_enabled) = rco32k_enabled;
        return TR_HAL_SUCCESS;
    }

    return TR_HAL_ERROR_INVALID_PARAM;
}

