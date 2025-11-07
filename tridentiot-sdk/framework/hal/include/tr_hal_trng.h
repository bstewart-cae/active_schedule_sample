/// ****************************************************************************
/// @file tr_hal_trng.h
///
/// @brief This is the common include file for the Trident HAL True Random
///        Number Generator (TRNG)
///
///
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************

#ifndef TR_HAL_TRNG_H_
#define TR_HAL_TRNG_H_

#include "tr_hal_platform.h"


/// ****************************************************************************
/// @defgroup tr_hal_trng TRNG
/// @ingroup tr_hal_api
/// @{
/// ****************************************************************************


// ****************************************************************************
// True Random Number Generator (TRNG) API functions
// ****************************************************************************

// if return is TR_HAL_SUCCESS then result was set to a 4 byte random number
tr_hal_status_t tr_hal_trng_get_uint32(uint32_t* result);

// if return is TR_HAL_SUCCESS then result was set to a 2 byte random number
tr_hal_status_t tr_hal_trng_get_uint16(uint16_t* result);

// if return is TR_HAL_SUCCESS then result was set to a 1 byte random number
tr_hal_status_t tr_hal_trng_get_uint8(uint8_t* result);


/// ****************************************************************************
/// @} // end of tr_hal_api
/// ****************************************************************************


#endif //TR_HAL_TRNG_H_
