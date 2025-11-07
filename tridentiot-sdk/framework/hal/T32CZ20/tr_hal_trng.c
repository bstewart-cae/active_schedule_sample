/// ****************************************************************************
/// @file tr_hal_trng.c
///
/// @brief This contains the code for the Trident HAL True Random Number
///        Generator (TRNG) for T32CZ20
///
///
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************

#include "tr_hal_trng.h"


static bool g_trng_enabled = false;

/// ****************************************************************************
/// enable_trng
///
/// after a reboot the TRNG clock is enabled by default but the TRNG is NOT
/// enabled. To enable it, the WRITE_KEY needs to be set in the sec_otp_write_key
/// register. This function sets the write key, enables TRNG, and then unsets
/// the write key. This is only done the first time that the TRNG runs.
/// ****************************************************************************
static void enable_trng(void)
{
    if (g_trng_enabled == false)
    {
        g_trng_enabled = true;

        // enable writes
        SECURITY_CTRL_CHIP_REGISTERS->sec_otp_write_key = OTP_WRITE_ENABLE_KEY;

        // enable TRNG
        TRNG_CHIP_REGISTERS->control |= (REG_CONTROL_ENABLE_TRNG_CLOCK | REG_CONTROL_ENABLE_TRNG_FUNCTION);

        // disable writes
        SECURITY_CTRL_CHIP_REGISTERS->sec_otp_write_key = OTP_WRITE_DISABLE_KEY;
    }
}


/// ****************************************************************************
/// TEST API
/// tr_hal_trng_debug - for reading the register values when debugging
/// ****************************************************************************
tr_hal_status_t tr_hal_trng_debug(uint32_t* version,
                                  uint32_t* status,
                                  uint32_t* data,
                                  uint32_t* control)
{
    if ( (version == NULL) || (status == NULL) || (data == NULL) )
    {
        return TR_HAL_ERROR_NULL_PARAMS;
    }

    (*version) = TRNG_CHIP_REGISTERS->version;
    (*status)  = TRNG_CHIP_REGISTERS->status;
    (*data)    = TRNG_CHIP_REGISTERS->data;
    (*control) = TRNG_CHIP_REGISTERS->control;

    return TR_HAL_SUCCESS;
}


/// ****************************************************************************
/// tr_hal_trng_read_internal - this is the function that is used by all the
/// other read functions to get a random number. This one ALSO counts the number
/// of cycles that it takes to get the random number
/// ****************************************************************************
tr_hal_status_t tr_hal_trng_read_internal(uint32_t* result, uint32_t* busy_cycles)
{
    // params can't be NULL
    if ( (result == NULL) || (busy_cycles == NULL) )
    {
        return TR_HAL_ERROR_NULL_PARAMS;
    }

    // enable the TRNG
    enable_trng();

    // wait for data to be ready
    uint32_t timeout_counter = 0;
    while (((TRNG_CHIP_REGISTERS->status) & REG_TRNG_STATUS_DATA_READY) == 0)
    {
        timeout_counter++;
        if (timeout_counter > TRNG_TIMEOUT_COUNT)
        {
            (*busy_cycles) = timeout_counter;
            (*result) = TRNG_CHIP_REGISTERS->data;
            return TR_HAL_TRNG_BUSY;
        }
    }

    (*busy_cycles) = timeout_counter;

    // return the value
    (*result) = TRNG_CHIP_REGISTERS->data;

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

