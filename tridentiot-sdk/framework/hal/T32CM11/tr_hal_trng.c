/// ****************************************************************************
/// @file tr_hal_trng.c
///
/// @brief This contains the code for the Trident HAL Trident HAL True Random
///        Number Generator (TRNG) for T32CM11
///
///
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************

#include "tr_hal_trng.h"


/// ****************************************************************************
/// tr_hal_trng_read_internal - this is the function that is used by all the
/// other read functions to get a random number. This one ALSO counts the number
/// of cycles that it takes to get the random number
/// ****************************************************************************
tr_hal_status_t tr_hal_trng_read_internal(uint32_t* result,
                                          uint32_t* busy_cycles)
{
    // params can't be NULL
    if ( (result == NULL) || (busy_cycles == NULL) )
    {
        return TR_HAL_ERROR_NULL_PARAMS;
    }

    SYS_CTRL_CHIP_REGISTERS->random_number_select = TRNG_CORRECTION_VON_NEUMANN | TRNG_INTERRUPT_DISABLE;
    SYS_CTRL_CHIP_REGISTERS->random_number_trigger = TRNG_ENABLE;

    // wait for it to not be busy
    uint32_t timeout_counter = 0;
    while (((SYS_CTRL_CHIP_REGISTERS->random_number_status) & TRNG_STATUS_BUSY) > 0)
    {
        timeout_counter++;
        if (timeout_counter > TRNG_TIMEOUT_COUNT)
        {
            (*busy_cycles) = timeout_counter;
            (*result) = SYS_CTRL_CHIP_REGISTERS->random_number_value;
            return TR_HAL_TRNG_BUSY;
        }
    }

    (*busy_cycles) = timeout_counter;

    // return the value
    (*result) = SYS_CTRL_CHIP_REGISTERS->random_number_value;

    return TR_HAL_SUCCESS;
}

/// ****************************************************************************
// if return is TR_HAL_SUCCESS then result was set to a 4 byte random number
/// ****************************************************************************
tr_hal_status_t tr_hal_trng_get_uint32(uint32_t* result)
{
    tr_hal_status_t status;
    uint32_t busy = 0;

    status = tr_hal_trng_read_internal(result,
                                       &busy);

    return status;
}


/// ****************************************************************************
// if return is TR_HAL_SUCCESS then result was set to a 2 byte random number
/// ****************************************************************************
tr_hal_status_t tr_hal_trng_get_uint16(uint16_t* result)
{
    tr_hal_status_t status;
    uint32_t temp = 0;
    uint32_t busy = 0;

    status = tr_hal_trng_read_internal(&temp,
                                       &busy);

    if (result != NULL)
    {
        (*result) = (uint16_t) (temp & 0xFFFF);
    }

    return status;
}


/// ****************************************************************************
// if return is TR_HAL_SUCCESS then result was set to a 1 byte random number
/// ****************************************************************************
tr_hal_status_t tr_hal_trng_get_uint8(uint8_t* result)
{
    tr_hal_status_t status;
    uint32_t temp = 0;
    uint32_t busy = 0;

    status = tr_hal_trng_read_internal(&temp,
                                       &busy);

    if (result != NULL)
    {
        (*result) = (uint8_t) (temp & 0xFF);
    }

    return status;
}

