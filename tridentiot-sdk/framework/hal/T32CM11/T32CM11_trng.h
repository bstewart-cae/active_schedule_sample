/// ****************************************************************************
/// @file T32CM11_trng.h
///
/// @brief This is the chip specific include file for T32CM11 True Random Number
///        Generator (TRNG). Note that there is a common include file for this
///        HAL module that contains the APIs that should be used by the application.
///
///
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************

#ifndef T32CM11_TRNG_H_
#define T32CM11_TRNG_H_

#include "tr_hal_platform.h"

/// ****************************************************************************
/// @defgroup tr_hal_trng_cm11 TRNG CM11
/// @ingroup tr_hal_T32CM11
/// @{
/// ****************************************************************************


// *****************************************************************
// the TRNG registers are part of the SYSTEM CONTROL registers
// these are defined in the GPIO module. This shows the relevant
// registers
//
// typedef struct
// {
//     ...
//     // random number generator registers (see section 20)
//     __IO uint32_t random_number_trigger; // 0x40
//     __IO uint32_t random_number_select;  // 0x44
//     __IO uint32_t random_number_status;  // 0x48
//     __IO uint32_t random_number_value;   // 0x4C

// } SYS_CTRL_REGISTERS_T;


// *****************************************************************
// access the TRNG registers using this define:
// #define SYS_CTRL_CHIP_REGISTERS  ((SYS_CTRL_REGISTERS_T *) CHIP_MEMORY_MAP_SYS_CTRL_BASE)
//
// example:
// SYS_CTRL_CHIP_REGISTERS->random_number_trigger = TRNG_ENABLE;


// for random_number_trigger (0x40) register
#define TRNG_ENABLE          0x01
#define TRNG_CLEAR_INTERRUPT 0x02

// for random_number_select (0x44) register
#define TRNG_CORRECTION_VON_NEUMANN 0x00
#define TRNG_CORRECTION_XOR         0x01
#define TRNG_INTERRUPT_ENABLE       0x02
#define TRNG_INTERRUPT_DISABLE      0x00

// for random_number_status (0x48) register
#define TRNG_STATUS_BUSY              0x01
#define TRNG_STATUS_INTERRUPT_PENDING 0x02

// this is for the timeout when waiting for TRNG to finish creating
// a random number. This usually works in 4000 - 7000 iterations on
// a system with no load
#define TRNG_TIMEOUT_COUNT 20000

// used for testing. This returns a 4 byte random number but also
// returns the number of cycles it took to generate the number
tr_hal_status_t tr_hal_trng_read_internal(uint32_t* result,
                                          uint32_t* busy_cycles);


/// ****************************************************************************
/// @} // end of tr_hal_T32CM11
/// ****************************************************************************


#endif // T32CM11_GPIO_H_
