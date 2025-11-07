/// ****************************************************************************
/// @file T32CZ20_power.h
///
/// @brief This is the chip specific include file for T32CZ20 Power Management
///        note that there is a common include file for this HAL module that
///        contains the APIs (such as the init function) that should be used
///        by the application.
///
///
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************

#ifndef T32CZ20_POWER_H_
#define T32CZ20_POWER_H_

#include "tr_hal_platform.h"

/// ****************************************************************************
/// @defgroup tr_hal_power_cz20 Power Mgmt CZ20
/// @ingroup tr_hal_T32CZ20
/// @{
/// ****************************************************************************


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
#ifdef DPD_SECURE_EN
    #define CHIP_MEMORY_MAP_DEEP_PWR_DOWN_BASE (0x50005000UL)
#else
    #define CHIP_MEMORY_MAP_DEEP_PWR_DOWN_BASE (0x40005000UL)
#endif // DPD_SECURE_EN

#ifdef SOC_PMU_SECURE_EN
    #define CHIP_MEMORY_MAP_POWER_MGMT_BASE (0x50006000UL)
#else
    #define CHIP_MEMORY_MAP_POWER_MGMT_BASE (0x40006000UL)
#endif // SOC_PMU_SECURE_EN

/// ****************************************************************************
/// \brief offsets for where to find chip registers needed for Deep Power Down
/// register which is used to
/// see section 4.8 and 5.5 in the chip datasheet
/// ****************************************************************************
typedef struct
{
    // read the reset reason from the chip
    __IO uint32_t reset_reason;          // 0x00

    // write 1 to clear the reset reason
    __IO uint32_t clear_reset_reason;    // 0x04

    // bitmask of what GPIOs can wake the chip
    __IO uint32_t gpio_wake_enable;      // 0x08

    // bitmask of wake polarity for GPIOs
    // value of 1 = wake on high
    // value of 0 = wake on low
    __IO uint32_t gpio_wake_polarity;    // 0x0C

    // retention registers
    // these can be used to store information and will not be cleared
    // when the chip goes to deep sleep
    __IO uint32_t retention_0;          // 0x10
    __IO uint32_t retention_1;          // 0x14
    __IO uint32_t retention_2;          // 0x18
    __IO uint32_t retention_3;          // 0x1C

} DEEP_POWER_DOWN_REGISTERS_T;

// *****************************************************************
// this is for the RESET REASON register (0x00)
#define TR_HAL_RESET_REASON_POWER           0x01
#define TR_HAL_RESET_REASON_EXTERNAL_RESET  0x02
#define TR_HAL_RESET_REASON_DEEP_POWER_DOWN 0x04
#define TR_HAL_RESET_REASON_DEEP_SLEEP      0x08
#define TR_HAL_RESET_REASON_WATCHDOG        0x10
#define TR_HAL_RESET_REASON_SOFTWARE        0x20
#define TR_HAL_RESET_REASON_MCU_LOCKUP      0x40


// *****************************************************************
// this orients the DEEP POWER DOWN REGISTERS struct with the correct addresses
// so referencing a field will now read/write the correct SYSCTRL register
// chip address
#define DEEP_POWER_DOWN_CHIP_REGISTERS  ((DEEP_POWER_DOWN_REGISTERS_T *) CHIP_MEMORY_MAP_DEEP_PWR_DOWN_BASE)



/// ****************************************************************************
/// \brief offsets for where to find chip registers needed for Power Mgmt
/// register. This is not documented in the Reference Manual
/// ****************************************************************************
typedef struct
{
    __IO uint32_t reserved0[2];         // 0x00, 0x04

    __IO uint32_t soc_bbpll_read;       // 0x08

    __IO uint32_t reserved1[5];         // 0x0C, 0x10, 0x14, 0x18, 0x1C

    __IO uint32_t pmu_soc_pmu_xtal_0;   // 0x20
    __IO uint32_t pmu_soc_pmu_xtal_1;   // 0x24

    __IO uint32_t reserved2[3];         // 0x28, 0x2C, 0x30

    // setup the 32KHz RC Oscillator
    __IO uint32_t pmu_osc_32k;          // 0x34

    __IO uint32_t reserved3[2];         // 0x38, 0x3C

    __IO uint32_t pmu_rvd_0;            // 0x40

    __IO uint32_t reserved4[18];        //       0x44, 0x48, 0x4C
                                        // 0x50, 0x54, 0x58, 0x5C
                                        // 0x60, 0x64, 0x68, 0x6C
                                        // 0x70, 0x74, 0x78, 0x7C
                                        // 0x80, 0x84, 0x88,
    // setup the 1MHz RC Oscillator
    __IO uint32_t soc_pmu_rco1m;        // 0x40

} POWER_MGMT_REGISTERS_T;


// *****************************************************************
// this is for the soc_pmu_rco1m register (0x40)

// bits 0-6
#define PM_RCO_1M_TUNE_FINE_MASK           0x8F

// bits 8-11
#define PM_RCO_1M_TUNE_COARSE_MASK           0xF00

// bits 12-13: PW_RCO_1M
// bits 14-15: TEST_RCO_1M

// bit 16
#define PM_RCO_1M_ENABLE_BIT           0x10000


// *****************************************************************
// this is for the pmu_osc_32k register (0x34)

// bits 0-7
#define PM_RCO_32K_TUNE_FINE_MASK          0xFF

// bits 8-9
#define PM_RCO_32K_TUNE_COARSE_MASK        0x300

// bits 10-11: PW_BUF_RCO_32K
// bits 12-15: PW_RCO_32K
// bit  16   : EN_XO_32K
// bit  17   : EN_XO_32K_FAST
// bits 18-19: PW_BUF_XO_32K
// bits 20-22: PW_XO_32K
// bit  23   : FORCE_RCO_32K_OFF

// bit 24 = select / enable
#define PM_RCO_32K_ENABLE_BIT           0x01000000


// *****************************************************************
// this orients the POWER MGMT REGISTERS struct with the correct addresses
// so referencing a field will now read/write the correct SYSCTRL register
// chip address
#define POWER_MGMT_CHIP_REGISTERS  ((POWER_MGMT_REGISTERS_T *) CHIP_MEMORY_MAP_POWER_MGMT_BASE)



/// ****************************************************************************
/// \brief enum for the different power modes that the chip can be in
/// see section 5, table 5-2
/// ****************************************************************************
typedef enum
{
    TR_HAL_POWER_MODE_0 = 10, // wake = CPU on, all clocks on, wake on INT
    TR_HAL_POWER_MODE_1 = 11, // sleep = CPU on, slow clocks on, wake on INT
    TR_HAL_POWER_MODE_2 = 12, // deep sleep = CPU off, slow clocks on, wake on GPIO/RTC
    TR_HAL_POWER_MODE_3 = 13, // deep power down = CPU off, clocks off, wake on GPIO

} tr_hal_power_mode_t;


/// ***************************************************************************
/// \brief enum for the different clocks
/// some of these can be disabled and some cannot, the crystal oscillator
/// cannot be disabled but will be off when the device is sleeping
/// 32MHz - per clk using the crystal oscillator at 32 MHz
/// 16MHz - per clk using the crystal oscillator at 32 MHz
/// rco1m - RC oscillator at 921.6 KHz ~= 1MHz
/// rco32k - RC oscillator for slow clock timers, runs at 38.4 KHz
/// ***************************************************************************
typedef enum
{
    // this is the system clock, for use when the device is not sleeping
    // this is called per_clk in the SYS_CTRL_CHIP_REGISTERS->system_clock_control_1
    TR_HAL_CLOCK_32M     = 0,

    // this is still using the system clock but set for 16MHz
    // the field per_clk_sel is set to 0b10 = xtal_clk/2 in the
    // SYS_CTRL_CHIP_REGISTERS->system_clock_control_0
    TR_HAL_CLOCK_16M     = 1,

    // this is the rco1m option for clock. This is used for UARTs that are required
    // to operate normally in low-power modes. This is limited to 115200 maximum
    // baud rate. The clock rate is actually 921.6 KHz (not 1000 MHz)
    TR_HAL_CLOCK_1M      = 2,

    // this is the rco32k option for clock. This is used for UARTs powered on in
    // sleep mode, but off in deep sleep mode. This is set for 38.4 KHz with a
    // maximum baud rate of 9600
    TR_HAL_CLOCK_32K     = 3,

} tr_hal_clock_t;


/// ****************************************************************************
/// @} // end of tr_hal_T32CZ20
/// ****************************************************************************


#endif // T32CZ20_POWER_H_


