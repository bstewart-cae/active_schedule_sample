/// ****************************************************************************
/// @file T32CZ20_trng.h
///
/// @brief This is the chip specific include file for T32CZ20 True Random Number
///        Generator (TRNG). Note that there is a common include file for this
///        HAL module that contains the APIs that should be used by the application.
///
///
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************

#ifndef T32CZ20_TRNG_H_
#define T32CZ20_TRNG_H_

#include "tr_hal_platform.h"


/// ****************************************************************************
/// @defgroup tr_hal_trng_cz20 TRNG CZ20
/// @ingroup tr_hal_T32CZ20
/// @{
/// ****************************************************************************


/// ******************************************************************
/// security control register
///
/// we need this register since the first time the TRNG gets run it
/// needs to enable the TRNG and to do that it needs to have write
/// access which it can only get by setting the write access key_comp
/// in register sec_otp_write_key. This may eventually move to
/// another module
/// ******************************************************************
#define CHIP_MEMORY_MAP_SECURITY_CTRL_BASE 0x50003000

typedef struct
{
    __IO uint32_t reserved[18];                  // 0x00 - 0x44
    __IO uint32_t sec_otp_write_key;             // 0x48

} SECURITY_CTRL_REGISTERS_T;

#define SECURITY_CTRL_CHIP_REGISTERS ((SECURITY_CTRL_REGISTERS_T *) CHIP_MEMORY_MAP_SECURITY_CTRL_BASE)

#define OTP_WRITE_ENABLE_KEY  (0x28514260)
#define OTP_WRITE_DISABLE_KEY (0x00000000)


/// ******************************************************************
/// \brief chip register addresses
/// section 2.2 of the data sheet explains the Memory map.
/// this gives the base address for how to write the chip registers
/// the chip registers are how the software interacts configures GPIOs,
/// reads GPIOs, and gets/sets information on the chip We create a
/// struct below that addresses the individual
/// registers. This makes it so we can use this base address and a
/// struct field to read or write a chip register
/// ******************************************************************
// this is the address of the PUFrt register, PLUS the offset of 0xA00
// which is where the TRNG registers start, see section 11.4
#ifdef PUF_OTP_SECURE_EN
    #define CHIP_MEMORY_MAP_TRNG_BASE     (0x50044A00UL)
#else
    #define CHIP_MEMORY_MAP_TRNG_BASE     (0x40044A00UL)
#endif // PUF_OTP_SECURE_EN


/// ***************************************************************************
/// the struct we use so we can address registers using field names
/// note that these are offset from the PUFrt register PLUS the TRNG offset
/// see section 11.4. Register 0x00 here will be register 0xA00 in the docs.
/// ***************************************************************************
typedef struct
{
    // expect this to be 0x39304200
    __IO uint32_t version;                       // 0x00
    __IO uint32_t reserved1[3];                  // 0x04, 0x08, 0x0C

    // use this to determine if TRNG is working
    __IO uint32_t status;                        // 0x10
    __IO uint32_t reserved2[3];                  // 0x14, 0x18, 0x1C

    // use this to enable clock, enable TRNG
    __IO uint32_t control;                       // 0x20
    __IO uint32_t reserved3[19];                 // 0x24 - 0x6C

    // read this to get the random number
    __IO uint32_t data;                          // 0x70

} TRNG_REGISTERS_T;


// *****************************************************************
// helper define for VERSION register (0x00)

#define REG_TRNG_EXPECTED_VERSION 0x39304200


// *****************************************************************
// helper defines for STATUS register (0x04)

#define REG_TRNG_STATUS_BUSY               0x01
#define REG_TRNG_STATUS_FIFO_CLEARED       0x02
#define REG_TRNG_STATUS_ENTROPY_SRC_AVAIL  0x04
// bits 3 to 7 reserved (5 bits): 0x08, 0x10, 0x20, 0x40, 0x80
#define REG_TRNG_STATUS_NOT_ENABLED        0x100
#define REG_TRNG_STATUS_HEALTH_TEST_ACTIVE 0x200
#define REG_TRNG_STATUS_DATA_READY         0x400
#define REG_TRNG_STATUS_HALTED_ERROR       0x800


// *****************************************************************
// helper defines for CONTROL register (0x08)

#define REG_CONTROL_ENABLE_TRNG_FUNCTION 0x01
#define REG_CONTROL_ENABLE_TRNG_CLOCK    0x02


// *****************************************************************
// this orients the TRNG_REGISTER struct with the correct addresses
// so referencing a field will now read/write the correct GPIO register
// chip address
#define TRNG_CHIP_REGISTERS  ((TRNG_REGISTERS_T *) CHIP_MEMORY_MAP_TRNG_BASE)


// this is for the timeout when waiting for TRNG to finish creating
// a random number. This usually works in 4000 - 7000 iterations on
// a system with no load
#define TRNG_TIMEOUT_COUNT 20000


// used for testing. This returns a 4 byte random number but also
// returns the number of cycles it took to generate the number
tr_hal_status_t tr_hal_trng_read_internal(uint32_t* result,
                                          uint32_t* busy_cycles);

// used for testing
tr_hal_status_t tr_hal_trng_debug(uint32_t* version,
                                  uint32_t* status,
                                  uint32_t* data,
                                  uint32_t* control);


/// ****************************************************************************
/// @} // end of tr_hal_T32CZ20
/// ****************************************************************************


#endif // T32CZ20_GPIO_H_
