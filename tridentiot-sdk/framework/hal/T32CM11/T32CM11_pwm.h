/// ****************************************************************************
/// @file T32CM11_pwm.h
///
/// @brief This is the chip specific include file for T32CM11 PWM Driver
///        note that there is a common include file for this HAL module that
///        contains the APIs (such as the init function) that should be used
///        by the application.
///
/// this chip supports 5 PWMs at one time. Each PWM can be assigned to a GPIO
///
///
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************

#ifndef T32CM11_PWM_H_
#define T32CM11_PWM_H_

#include "tr_hal_platform.h"

/// ****************************************************************************
/// @defgroup tr_hal_pwm_cm11 PWM CM11
/// @ingroup tr_hal_T32CM11
/// @{
/// ****************************************************************************

#define TR_HAL_NUM_PWM 5

// PWM IDs
typedef enum
{
    PWM_0_ID = 0,
    PWM_1_ID = 1,
    PWM_2_ID = 2,
    PWM_3_ID = 3,
    PWM_4_ID = 4,

} tr_hal_pwm_id_t;

/// ******************************************************************
/// valid pins for PWM
/// PWM0: 8,  20, 21, 22, 23
/// PWM1: 9,  20, 21, 22, 23
/// PWM2: 14, 20, 21, 22, 23
/// PWM3: 15, 20, 21, 22, 23
/// PWM4:     20, 21, 22, 23
/// ******************************************************************
#define PWM_DEFAULT_PIN 20

/// ******************************************************************
/// \brief chip register addresses
/// section 3.1 of the data sheet explains the Memory map.
/// this gives the base address for how to write the chip registers
/// the chip registers are how the software interacts and configures
/// the PWM peripherals. We create a struct below that addresses the
/// individual registers. This makes it so we can use this base address
/// and a struct field to read or write a chip register
/// ******************************************************************
#define CHIP_MEMORY_MAP_PWM0_BASE     (0xA0C00000UL)
#define CHIP_MEMORY_MAP_PWM1_BASE     (0xA0C00100UL)
#define CHIP_MEMORY_MAP_PWM2_BASE     (0xA0C00200UL)
#define CHIP_MEMORY_MAP_PWM3_BASE     (0xA0C00300UL)
#define CHIP_MEMORY_MAP_PWM4_BASE     (0xA0C00400UL)


/// ***************************************************************************
/// \brief the struct we use so we can address registers using field names
/// ***************************************************************************
typedef struct
{
    // enable PWM and enable CLK
    __IO  uint32_t   enable;             // 0x00

    // reset PWM
    __IO  uint32_t   reset;              // 0x04

    // PWM setup
    __IO  uint32_t   settings;           // 0x08

    // PWM counter end value
    __IO  uint32_t   counter_end;        // 0x0C

    // number of times to play the sequence
    // amount of playback
    __IO  uint32_t   sequence_repeat;    // 0x10

    // RSEQ
    __IO  uint32_t   rseq_num_elements;  // 0x14
    __IO  uint32_t   rseq_num_repeats;   // 0x18
    __IO  uint32_t   rseq_delay;         // 0x1C

    // TSEQ
    __IO  uint32_t   tseq_num_elements;  // 0x20
    __IO  uint32_t   tseq_num_repeats;   // 0x24
    __IO  uint32_t   tseq_delay;         // 0x28

    __IO  uint32_t   reserved1[5];       // 0x2C,
                                         // 0x30, 0x34, 0x38, 0x3C

    // RDMA 0 - RSEQ
    __IO  uint32_t   dma0_enable;        // 0x40
    __IO  uint32_t   dma0_reset;         // 0x44
    __IO  uint32_t   dma0_segment_size;  // 0x48
    __IO  uint32_t   dma0_start_addr;    // 0x4C
    __IO  uint32_t   reserved2[2];       // 0x50, 0x54
    __IO  uint32_t   dma0_next_ptr_addr; // 0x58
    __IO  uint32_t   dma0_debug;         // 0x5C

    // RDMA 1 - TSEQ
    __IO  uint32_t   dma1_enable;        // 0x60
    __IO  uint32_t   dma1_reset;         // 0x64
    __IO  uint32_t   dma1_segment_size;  // 0x68
    __IO  uint32_t   dma1_start_addr;    // 0x6C
    __IO  uint32_t   reserved3[2];       // 0x70, 0x74
    __IO  uint32_t   dma1_next_ptr_addr; // 0x78
    __IO  uint32_t   dma1_debug;         // 0x7C

    __IO  uint32_t   reserved4[8];       // 0x80, 0x84, 0x88, 0x8C
                                         // 0x90, 0x94, 0x98, 0x9C
    // interrupts
    __IO  uint32_t   interrupt_clear;    // 0xA0
    __IO  uint32_t   interrupt_mask;     // 0xA4
    __IO  uint32_t   interrupt_status;   // 0xA8

} PWM_REGISTERS_T;

// *****************************************************************
// this orients the PWMx_REGISTERS struct with the correct addresses
// so referencing a field will now read/write the correct PWM
// register chip address
#define PWM0_REGISTERS  ((PWM_REGISTERS_T *) CHIP_MEMORY_MAP_PWM0_BASE)
#define PWM1_REGISTERS  ((PWM_REGISTERS_T *) CHIP_MEMORY_MAP_PWM1_BASE)
#define PWM2_REGISTERS  ((PWM_REGISTERS_T *) CHIP_MEMORY_MAP_PWM2_BASE)
#define PWM3_REGISTERS  ((PWM_REGISTERS_T *) CHIP_MEMORY_MAP_PWM3_BASE)
#define PWM4_REGISTERS  ((PWM_REGISTERS_T *) CHIP_MEMORY_MAP_PWM4_BASE)


// *****************************************************************
// helper defines for ENABLE REGISTER (0x00)
#define PWM_CTRL_REG_ENABLE_PWM  0x01
#define PWM_CTRL_REG_DISABLE_PWM 0x00
#define PWM_CTRL_REG_ENABLE_CLK  0x02
#define PWM_CTRL_REG_DISABLE_CLK 0x00

// *****************************************************************
// helper defines for RESET REGISTER (0x04)
#define PWM_CTRL_REG_RESET 0x01


// *****************************************************************
// helper defines for SETTINGS REGISTER (0x08)

// bit 0 = sequence order
#define PWM_CTRL_REG_RSEQ_FIRST 0x00
#define PWM_CTRL_REG_TSEQ_FIRST 0x01

// bit 1 = sequence selection
#define PWM_CTRL_REG_ONE_SEQUENCE 0x00
#define PWM_CTRL_REG_TWO_SEQUENCE 0x02

// bit 2 = play mode
#define PWM_CTRL_REG_NON_CONTINUOUS 0x00
#define PWM_CTRL_REG_CONTINUOUS     0x04

// bit 3 = DMA format
#define PWM_CTRL_REG_DMA_FORMAT_0 0x00
#define PWM_CTRL_REG_DMA_FORMAT_1 0x08

// bit 4 = counter mode
#define PWM_CTRL_REG_UP_COUNTER          0x00
#define PWM_CTRL_REG_DOWN_AND_UP_COUNTER 0x10

// bit 5 = counter trigger
#define PWM_CTRL_REG_TRIGGER_ON_ENABLE 0x00
#define PWM_CTRL_REG_TRIGGER_ON_FIFO   0x20

// bit 6 = auto trigger
#define PWM_CTRL_REG_NO_AUTO_TRIGGER 0x00
#define PWM_CTRL_REG_AUTO_TRIGGER    0x40

// bits 8 to 11 = clock dividier
#define PWM_CLK_DIV_1   0x0000
#define PWM_CLK_DIV_2   0x0100
#define PWM_CLK_DIV_4   0x0200
#define PWM_CLK_DIV_8   0x0300
#define PWM_CLK_DIV_16  0x0400
#define PWM_CLK_DIV_32  0x0500
#define PWM_CLK_DIV_64  0x0600
#define PWM_CLK_DIV_128 0x0700
#define PWM_CLK_DIV_256 0x0800

#define PWM_CLK_DIV_MASK 0x0F00

// bits 12-14: enable PWM to trigger on other PWM
#define PWM_CTRL_REG_TRIGGER_ON_PWM0 0x0000
#define PWM_CTRL_REG_TRIGGER_ON_PWM1 0x1000
#define PWM_CTRL_REG_TRIGGER_ON_PWM2 0x2000
#define PWM_CTRL_REG_TRIGGER_ON_PWM3 0x3000
#define PWM_CTRL_REG_TRIGGER_ON_PWM4 0x4000
#define PWM_CTRL_REG_SELF_TRIGGER    0x7000

// bits 15-31 reserved

// *****************************************************************
// helper defines for DMAx ENABLE REGISTER (0x40, 0x60)
#define PWM_DMA_ENABLE  0x00000001
#define PWM_DMA_DISABLE 0x00000000

// *****************************************************************
// helper defines for DMAx RESET REGISTER (0x44, 0x64)
#define PWM_DMA_RESET 0x00000001


/// ***************************************************************************
/// enum for valid clock divider values
/// ***************************************************************************
typedef enum
{
    TR_HAL_PWM_CLOCK_DIVIDER_1 = 1,
    TR_HAL_PWM_CLOCK_DIVIDER_2 = 2,
    TR_HAL_PWM_CLOCK_DIVIDER_4 = 3,
    TR_HAL_PWM_CLOCK_DIVIDER_8 = 4,
    TR_HAL_PWM_CLOCK_DIVIDER_16 = 5,
    TR_HAL_PWM_CLOCK_DIVIDER_32 = 6,
    TR_HAL_PWM_CLOCK_DIVIDER_64 = 7,
    TR_HAL_PWM_CLOCK_DIVIDER_128 = 8,
    TR_HAL_PWM_CLOCK_DIVIDER_256 = 9,

} tr_hal_pwm_clk_div_t;


/// ***************************************************************************
/// the PWM on the CM11 only has 1 choice of source clock
/// ***************************************************************************
typedef enum
{
    TR_HAL_PWM_CLK_SELECT_PER_CLK = 0,
    TR_HAL_PWM_CLK_SELECT_DEFAULT = 0,

} tr_hal_pwm_clk_select_t;


/// ***************************************************************************
/// below are some examples that show how to setup the clock divider, threshold,
/// and end count fields of the PWM settings struct to get the desired PWM
/// waveform
/// ***************************************************************************

/// ***************************************************************************
/// defines for 1 MHz signals, duty cycle of 25%, 50%, 75%
/// these work when the clock divisor is 1
/// ***************************************************************************
// end_count = 0x001F + 1 = 0x0020 = 32 --> 32Mhz/32 = 1 MHz
#define PWM_END_COUNT_CLKDIV_1_1MHZ                  0x0020
// threshhold = 0x0018 = 24 --> 24/32 = 75%
#define PWM_THRESHHOLD_CLKDIV_1_1MHZ_DUTY_CYCLE_75   0x0018
// threshhold = 0x0010 = 16 --> 16/32 = 50%
#define PWM_THRESHHOLD_CLKDIV_1_1MHZ_DUTY_CYCLE_50   0x0010
// threshhold = 0x0008 =  8 -->  8/32 = 25%
#define PWM_THRESHHOLD_CLKDIV_1_1MHZ_DUTY_CYCLE_25   0x0008

/// ***************************************************************************
/// defines for 500 KHz signals, duty cycle of 25%, 50%, 75%
/// these work when the clock divisor is 1
/// ***************************************************************************
// end_count = 0x003F + 1 = 0x0040 = 64 --> 32Mhz/64 = 500 KHz
#define PWM_END_COUNT_CLKDIV_1_500KHZ                0x0040
// threshhold = 0x0030 = 48 --> 48/64 = 75%
#define PWM_THRESHHOLD_CLKDIV_1_500KHZ_DUTY_CYCLE_75 0x0030
// threshhold = 0x0020 = 32 --> 32/64 = 50%
#define PWM_THRESHHOLD_CLKDIV_1_500KHZ_DUTY_CYCLE_50 0x0020
// threshhold = 0x0010 = 16 --> 16/64 = 25%
#define PWM_THRESHHOLD_CLKDIV_1_500KHZ_DUTY_CYCLE_25 0x0010

/// ***************************************************************************
/// defines for 250 KHz signals, duty cycle of 25%, 50%, 75%
/// these work when the clock divisor is 1
/// ***************************************************************************
// end_count = 0x007F + 1 = 0x0080 = 128 --> 32Mhz/128 = 250 KHz
#define PWM_END_COUNT_CLKDIV_1_250KHZ                0x0080
// threshhold = 0x0060 = 96 --> 96/128 = 75%
#define PWM_THRESHHOLD_CLKDIV_1_250KHZ_DUTY_CYCLE_75 0x0060
// threshhold = 0x0040 = 64 --> 64/128 = 50%
#define PWM_THRESHHOLD_CLKDIV_1_250KHZ_DUTY_CYCLE_50 0x0040
// threshhold = 0x0020 = 32 --> 32/128 = 25%
#define PWM_THRESHHOLD_CLKDIV_1_250KHZ_DUTY_CYCLE_25 0x0020

// limits for end count
#define MINIMUM_END_COUNT_VALUE 4
#define MAXIMUM_END_COUNT_VALUE 0x7FFF

// limits for threshhold
#define MINIMUM_THRESHHOLD_VALUE 4
#define MAXIMUM_THRESHHOLD_VALUE 0x7FFF


/// ***************************************************************************
/// PWM settings struct - this is passed to tr_hal_pwm_init
///
/// ***************************************************************************
typedef struct
{
    // which pin to use for PWM
    tr_hal_gpio_pin_t pin_to_use;

    // which clock to use
    tr_hal_pwm_clk_select_t clock_to_use;

    // clock divider
    tr_hal_pwm_clk_div_t clock_divider;

    // this along with the clock creates the frequency
    // freq = clock / (clk_div * end_count)
    uint16_t end_count;

    // this creates the duty cycle
    // duty cycle = threshhold / end_count
    uint16_t threshhold;

} tr_hal_pwm_settings_t;

/// ***************************************************************************
/// initializer macros for default PWM settings
///
/// ***************************************************************************


/// PWM settings for regular sawtooth wave
#define DEFAULT_PWM_CONFIG                                            \
    {                                                                 \
        .pin_to_use = (tr_hal_gpio_pin_t) { PWM_DEFAULT_PIN },        \
        .clock_to_use  = TR_HAL_PWM_CLK_SELECT_DEFAULT,               \
        .clock_divider = TR_HAL_PWM_CLOCK_DIVIDER_1,                  \
        .end_count     = PWM_END_COUNT_CLKDIV_1_500KHZ,               \
        .threshhold    = PWM_THRESHHOLD_CLKDIV_1_500KHZ_DUTY_CYCLE_75,\
    }


/// ***************************************************************************
/// if the app wants to directly interface with the chip registers, this is a
/// convenience function for getting the address/struct of a particular PWM
/// so the chip registers can be accessed.
/// ***************************************************************************
PWM_REGISTERS_T* tr_hal_pwm_get_register_address(tr_hal_pwm_id_t pwm_id);


/// ****************************************************************************
/// @} // end of tr_hal_T32CM11
/// ****************************************************************************


#endif //T32CM11_PWM_H_


