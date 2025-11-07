/// ****************************************************************************
/// @file T32CZ20_adc.h
///
/// @brief This is the chip specific include file for T32CZ20 ADC Driver
///        note that there is a common include file for this HAL module that
///        contains the APIs (such as the init function) that should be used
///        by the application.
///
///
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************

#ifndef T32CZ20_ADC_H_
#define T32CZ20_ADC_H_

#include "tr_hal_platform.h"

/// ****************************************************************************
/// @defgroup tr_hal_adc_cz20 ADC CZ20
/// @ingroup tr_hal_T32CZ20
/// @{
/// ****************************************************************************

#define TR_HAL_NUM_ADC 7

/// ******************************************************************
/// ADC IDs
/// there are 7 ADC channels since there are only 7 valid
/// GPIOs to use
/// ******************************************************************
typedef enum
{
    ADC_CHANNEL_0_ID = 0,
    ADC_CHANNEL_1_ID = 1,
    ADC_CHANNEL_2_ID = 2,
    ADC_CHANNEL_3_ID = 3,
    ADC_CHANNEL_4_ID = 4,
    ADC_CHANNEL_5_ID = 5,
    ADC_CHANNEL_6_ID = 6,

} tr_hal_adc_channel_id_t;

#define MAX_ADC_CHANNEL_ID ADC_CHANNEL_6_ID
#define TR_HAL_ADC_CHANNEL_NONE 0xFF

/// ******************************************************************
/// valid pins for ADC
/// we use pin 21 as the default
/// this is the first of 7 pins that are available (21, 22, 23, 28, 29, 30, 31)
/// ******************************************************************
#define TR_HAL_ADC_AIO0 21
#define TR_HAL_ADC_AIO1 22
#define TR_HAL_ADC_AIO2 23
// pin 24 = AIO3 is not available
#define TR_HAL_ADC_AIO4 28
#define TR_HAL_ADC_AIO5 29
#define TR_HAL_ADC_AIO6 30
#define TR_HAL_ADC_AIO7 31

#define ADC_VALID_PIN_CHOICE1 TR_HAL_ADC_AIO0
#define ADC_VALID_PIN_CHOICE2 TR_HAL_ADC_AIO1
#define ADC_VALID_PIN_CHOICE3 TR_HAL_ADC_AIO2
#define ADC_VALID_PIN_CHOICE4 TR_HAL_ADC_AIO4
#define ADC_VALID_PIN_CHOICE5 TR_HAL_ADC_AIO5
#define ADC_VALID_PIN_CHOICE6 TR_HAL_ADC_AIO6
#define ADC_VALID_PIN_CHOICE7 TR_HAL_ADC_AIO7

#define DEFAULT_ADC_PIN ADC_VALID_PIN_CHOICE1

// these are used to enable the AIO in the analog settings register
#define TR_ADC_ENABLE_AIO0 0x01UL
#define TR_ADC_ENABLE_AIO1 0x02UL
#define TR_ADC_ENABLE_AIO2 0x04UL
// AIO3 is not available
#define TR_ADC_ENABLE_AIO4 0x10UL
#define TR_ADC_ENABLE_AIO5 0x20UL
#define TR_ADC_ENABLE_AIO6 0x40UL
#define TR_ADC_ENABLE_AIO7 0x80UL


/// ***************************************************************************
/// section 2.2 of the data sheet explains the Memory map
/// this gives the base address for how to write the I2C registers
/// the chip registers are how the software interacts with the I2C
/// chip peripheral. We create a struct below that addresses the
/// individual registers. This makes it so we can use this base
/// address and a struct field to read or write a chip register
/// ***************************************************************************
#ifdef SADC_SECURE_EN
    #define CHIP_MEMORY_MAP_ADC_BASE            (0x5002F000UL)
    #define CHIP_MEMORY_MAP_AUX_COMPARATOR_BASE (0x5001E000UL)
#else
    #define CHIP_MEMORY_MAP_ADC_BASE            (0x4002F000UL)
    #define CHIP_MEMORY_MAP_AUX_COMPARATOR_BASE (0x4001E000UL)
#endif // SADC_SECURE_EN


/// ***************************************************************************
/// helper struct. there are 10 channels, each with the same 4 fields
/// this allows us to use an array and array index in the code which makes
/// the code simpler
/// ***************************************************************************
typedef struct
{
    __IO uint32_t ch_x_config;              // 0x00
    __IO uint32_t ch_x_burst;               // 0x04
    __IO uint32_t ch_x_threshholds;         // 0x08
    __IO uint32_t ch_x_reserved;            // 0x0C
} CHAN_SETTINGS_T;


/// ***************************************************************************
/// this is taken from the chip sample code and is needed to setup the analog
/// registers of the ADC
/// ***************************************************************************
typedef union tr_sadc_ana_set0_s
{
    struct tr_sadc_ana_set0_b
    {
        uint32_t    AUX_ADC_DEBUG       : 1;
        uint32_t    AUX_ADC_MODE        : 1;
        uint32_t    AUX_ADC_OUTPUTSTB   : 1;
        uint32_t    AUX_ADC_OSPN        : 1;
        uint32_t    AUX_ADC_CLK_SEL     : 2;
        uint32_t    AUX_ADC_MCAP        : 2;
        uint32_t    AUX_ADC_MDLY        : 2;
        uint32_t    AUX_ADC_SEL_DUTY    : 2;
        uint32_t    AUX_ADC_OS          : 2;
        uint32_t    RESERVED1           : 2;
        uint32_t    AUX_ADC_BR          : 4;
        uint32_t    AUX_ADC_PW          : 3;
        uint32_t    RESERVED2           : 1;
        uint32_t    AUX_ADC_STB_BIT     : 3;
        uint32_t    RESERVED3           : 1;
        uint32_t    AUX_PW              : 3;
        uint32_t    RESERVED4           : 1;

    } bit;
    uint32_t reg;
} tr_sadc_ana_set0_t;

typedef union tr_sadc_ana_set1_s
{
    struct tr_sadc_ana_set1_b
    {
        uint32_t    AUX_VGA_CMSEL       : 4;
        uint32_t    AUX_VGA_COMP        : 2;
        uint32_t    AUX_VGA_SIN         : 2;

        uint32_t    AUX_VGA_LOUT        : 1;
        uint32_t    AUX_VGA_SW_VDD      : 1;
        uint32_t    AUX_VGA_VLDO        : 2;
        uint32_t    AUX_VGA_ACM         : 4;

        uint32_t    AUX_VGA_PW          : 6;
        uint32_t    AUX_DC_ADJ          : 2;

        uint32_t    AUX_TEST_MODE       : 1;
        uint32_t    CFG_EN_CLKAUX       : 1;
        uint32_t    AUX_VGA_TEST_AIO_EN : 1;
        uint32_t    RESERVED1           : 5;

    } bit;
    uint32_t reg;
} tr_sadc_ana_set1_t;



/// ***************************************************************************
/// the struct we use so we can address the ADC registers using field names
/// ***************************************************************************
typedef struct
{
    // enable, reset, and start control register
    __IO uint32_t control_enable;           // 0x00
    __IO uint32_t control_reset;            // 0x04
    __IO uint32_t control_start;            // 0x08

    // settings
    __IO uint32_t clock_settings;           // 0x0C
    __IO uint32_t oversample_settings;      // 0x10

    // reserved
    __IO uint32_t reserved1[3];             // 0x14, 0x18, 0x1C

    // there are 10 channels: 0 to 9
    // each channel has 3 register fields, plus one reserved
    // rather than have 40 fields for these, these are setup as an array
    // for simpler code
    //
    // here is an example of CH 0 and CH 1 if we were doing individual fields:
    // CH 0: configurations, burst mode, and monitor threshhold
    // __IO uint32_t ch_0_config;              // 0x20
    // __IO uint32_t ch_0_burst;               // 0x24
    // __IO uint32_t ch_0_threshholds;         // 0x28
    // __IO uint32_t ch_0_reserved;            // 0x2C

    // CH 1: configurations, burst mode, and monitor threshhold
    // __IO uint32_t ch_1_config;              // 0x30
    // __IO uint32_t ch_1_burst;               // 0x34
    // __IO uint32_t ch_1_threshholds;         // 0x38
    // __IO uint32_t ch_1_reserved;            // 0x3C

    // each of the 10 channels has 4 registers = 40 total
    // CH 0: configurations (0x20), burst mode (0x24), and threshholds (0x28), rsvd (0x2C)
    // CH 1: configurations (0x30), burst mode (0x34), and threshholds (0x38), rsvd (0x3C)
    // CH 2: configurations (0x40), burst mode (0x44), and threshholds (0x48), rsvd (0x4C)
    // CH 3: configurations (0x50), burst mode (0x54), and threshholds (0x58), rsvd (0x5C)
    // CH 4: configurations (0x60), burst mode (0x64), and threshholds (0x68), rsvd (0x6C)
    // CH 5: configurations (0x70), burst mode (0x74), and threshholds (0x78), rsvd (0x7C)
    // CH 6: configurations (0x80), burst mode (0x84), and threshholds (0x88), rsvd (0x8C)
    // CH 7: configurations (0x90), burst mode (0x94), and threshholds (0x98), rsvd (0x9C)
    // CH 8: configurations (0xA0), burst mode (0xA4), and threshholds (0xA8), rsvd (0xAC)
    // CH 9: configurations (0xB0), burst mode (0xB4), and threshholds (0xB8), rsvd (0xBC)

    // here are the 10 channels, CHAN_SETTINGS_T is 4 x uint32_t
    __IO CHAN_SETTINGS_T ch_x_setting[9];     // 0x20 - 0xAC

    __IO uint32_t reserved2[3];               // 0xB0, 0xB4, 0xB8

    // analog AUX ADC settings
    // unfortunately the first one overlaps with the reserved field of the prior channel
    // we could pull that channel from the array but that makes everything harder.
    // so we just #define analog_settings0 to the right address below. Right after these
    // 2 fields we have a MASSIVE gap. so we weren't short on space.
    __IO tr_sadc_ana_set0_t analog_settings0;  // 0xBC
    __IO tr_sadc_ana_set1_t analog_settings1;  // 0xC0

    // reserved
    __IO uint32_t reserved3[15];               // 0xC4 - 0xFC

    // DMA: start, reset, buffer size, buffer addr
    __IO uint32_t enable_dma;                  // 0x100
    __IO uint32_t reset_dma;                   // 0x104
    __IO uint32_t dma_buffer_size;             // 0x108
    __IO uint32_t dma_buffer_addr;             // 0x10C
    __IO uint32_t dma_settings;                // 0x110
    __IO uint32_t dma_next_ptr_addr;           // 0x114
    __IO uint32_t dma_status;                  // 0x118
    __IO uint32_t dma_reserved2;               // 0x11C

    // interrupts
    __IO uint32_t interrupt_clear;              // 0x120
    // note that this register is opposite of most interrupt_enable registers:
    // 0 means enable and 1 means disable
    __IO uint32_t interrupt_enable;             // 0x124
    __IO uint32_t interrupt_status;             // 0x128

    // result
    __IO uint32_t result_oversample;            // 0x12C
    __IO uint32_t result_digital;               // 0x130
    __IO uint32_t result_analog;                // 0x134

} ADC_REGISTERS_T;


/// ***************************************************************************
/// for AUX COMP register at addr 0x00
/// ***************************************************************************
typedef union tr_aux_comp_ana_ctl_s
{
    struct tr_aux_comp_ana_ctl_b
    {
        uint32_t COMP_SELREF         : 1;
        uint32_t COMP_SELINPUT       : 1;
        uint32_t COMP_PW             : 2;
        uint32_t COMP_SELHYS         : 2;
        uint32_t COMP_SWDIV          : 1;
        uint32_t COMP_PSRR           : 1;
        uint32_t COMP_VSEL           : 4;
        uint32_t COMP_REFSEL         : 4;
        uint32_t COMP_CHSEL          : 4;
        uint32_t COMP_TC             : 1;
        uint32_t RESERVED1           : 3;
        uint32_t COMP_EN_START       : 2;
        uint32_t RESERVED2           : 6;
    } bit;
    uint32_t reg;
} tr_aux_comp_ana_ctl_t;


/// ***************************************************************************
/// the struct we use so we can address the AUX comparator registers using 
/// field names. This is needed to make the ADC work
/// ***************************************************************************
typedef struct
{
    __IO tr_aux_comp_ana_ctl_t    COMP_ANA_CTRL;     // 0x00
    __IO uint32_t                 AUXCOMP_DIG_CTRL0; // 0x04
    __IO uint32_t                 AUXCOMP_DIG_CTRL1; // 0x04
    __IO uint32_t                 AUXCOMP_DIG_CTRL2; // 0x04

} AUX_COMPARATOR_REGISTERS_T;


// *****************************************************************
// these defines help when dealing with the CONTROL ENABLE register (0x00)

#define ADC_REG_ENABLE_ADC_DISABLE     0x000
#define ADC_REG_ENABLE_ADC_ENABLE      0x001

// undocumented
#define ADC_REG_ENABLE_VGA_ENABLE 0x02
#define ADC_REG_ENABLE_LDO_ENABLE 0x04

// bits 8-9 (undocumented -- same as CM11)
#define ADC_REG_ENABLE_CLK_FREE 0x100

// *****************************************************************
// these defines help when dealing with the CONTROL RESET register (0x04)

#define ADC_REG_RESET_ADC  0x001
#define ADC_REG_RESET_FIFO 0x100

// *****************************************************************
// these defines help when dealing with the CONTROL START register (0x08)

#define ADC_REG_START_ADC  0x01

// *****************************************************************
// these defines help when dealing with the CLOCK SETTINGS register (0x0C)

// bit 0 = sample rate mode
#define ADC_REG_TIMER_RATE_DEPENDS_ON_SOFTWARE  0x00
#define ADC_REG_TIMER_RATE_DEPENDS_ON_TIMER     0x01
// bit 1 = timer clock source
#define ADC_REG_TIMER_USE_SYSTEM_CLOCK   0x00
#define ADC_REG_TIMER_USE_SLOW_CLOCK     0x02
// bit 2 = clock phase
#define ADC_REG_TIMER_RISING_EDGE  0x00
#define ADC_REG_TIMER_FALLING_EDGE 0x04
// bit 3 to 6 = debug selection
#define ADC_REG_TIMER_DEBUG_MASK 0x78

// bit 16 to 31 (high 2 bytes) = timer clock divider
#define ADC_REG_TIMER_CLOCK_DIV_MASK 0xFFFF0000
#define ADC_REG_TIMER_CLOCK_DIV_SHIFT 16
// one is added to the value given before being used to divide the clock
// smallest value is 2 which would be a clk div of 3
// largest value is 65535 which would be a clk div of 65536
#define TR_HAL_ADC_MAX_CLOCK_DIVISOR 65536
#define TR_HAL_ADC_MIN_CLOCK_DIVISOR 3

// *****************************************************************
// these defines help when dealing with the OVERSAMPLE SETTINGS register (0x10)

// bit 0 to 3 = output resolution (chip default 2)
#define ADC_REG_SAMPLE_OUTPUT_RESOLUTION_8_BIT  0x00
#define ADC_REG_SAMPLE_OUTPUT_RESOLUTION_10_BIT 0x01
#define ADC_REG_SAMPLE_OUTPUT_RESOLUTION_12_BIT 0x02
#define ADC_REG_SAMPLE_OUTPUT_RESOLUTION_14_BIT 0x03

// bits 4 to 7 = channel select
#define ADC_REG_SAMPLE_SELECT_CHANNEL_0 0x00
#define ADC_REG_SAMPLE_SELECT_CHANNEL_1 0x10
#define ADC_REG_SAMPLE_SELECT_CHANNEL_2 0x20
#define ADC_REG_SAMPLE_SELECT_CHANNEL_3 0x30
#define ADC_REG_SAMPLE_SELECT_CHANNEL_4 0x40
#define ADC_REG_SAMPLE_SELECT_CHANNEL_5 0x50
#define ADC_REG_SAMPLE_SELECT_CHANNEL_6 0x60
#define ADC_REG_SAMPLE_SELECT_CHANNEL_7 0x70
#define ADC_REG_SAMPLE_SELECT_CHANNEL_8 0x80
#define ADC_REG_SAMPLE_SELECT_CHANNEL_9 0x90

// bits 8 to 11 = oversample rate
typedef enum
{
    TR_HAL_ADC_REG_SAMPLE_NO_OVERSAMPLE  = 0x000,
    TR_HAL_ADC_REG_SAMPLE_OVERSAMPLE_2   = 0x100,
    TR_HAL_ADC_REG_SAMPLE_OVERSAMPLE_4   = 0x200,
    TR_HAL_ADC_REG_SAMPLE_OVERSAMPLE_8   = 0x300,
    TR_HAL_ADC_REG_SAMPLE_OVERSAMPLE_16  = 0x400,
    TR_HAL_ADC_REG_SAMPLE_OVERSAMPLE_32  = 0x500,
    TR_HAL_ADC_REG_SAMPLE_OVERSAMPLE_64  = 0x600,
    TR_HAL_ADC_REG_SAMPLE_OVERSAMPLE_128 = 0x700,
    TR_HAL_ADC_REG_SAMPLE_OVERSAMPLE_256 = 0x800,

} tr_hal_adc_oversample_t;

// bits 12-15 are for testing
#define ADC_REG_SAMPLE_VALUE_BYPASS        0x1000
#define ADC_REG_SAMPLE_MSB_BIT_INVERSION   0x2000
#define ADC_REG_SAMPLE_ENABLE_MANUAL_MODE1 0x4000
#define ADC_REG_SAMPLE_ENABLE_MANUAL_MODE2 0x8000

// undocumented = in order to make ADC single shot work
// need to enable ADC_REG_SAMPLE_ENABLE_MANUAL_MODE1
// and ADC_REG_SAMPLE_ENABLE_MANUAL_MODE2, which
// gives a value of 0xC000

// bits 16 to 27 = ADC adjuts value, used for calibration
#define ADC_REG_SAMPLE_CALIBRATION_MASK 0x0FFF0000

// *****************************************************************
// these defines help when dealing with the CH_X_CONFIG register (0x20, 0x30, 0x40...)

// 4 bits (0 to 3) of the P-channel
#define ADC_CONFIG_REG_P_CHANNEL_AIN_0       0x00000000
#define ADC_CONFIG_REG_P_CHANNEL_AIN_1       0x00000001
#define ADC_CONFIG_REG_P_CHANNEL_AIN_2       0x00000002
#define ADC_CONFIG_REG_P_CHANNEL_AIN_3       0x00000003
#define ADC_CONFIG_REG_P_CHANNEL_AIN_4       0x00000004
#define ADC_CONFIG_REG_P_CHANNEL_AIN_5       0x00000005
#define ADC_CONFIG_REG_P_CHANNEL_AIN_6       0x00000006
#define ADC_CONFIG_REG_P_CHANNEL_AIN_7       0x00000007
#define ADC_CONFIG_REG_P_CHANNEL_TEMP_SENSOR 0x00000008
#define ADC_CONFIG_REG_P_CHANNEL_BATT_VOLT   0x0000000A
#define ADC_CONFIG_REG_P_CHANNEL_NONE        0x0000000F

// 4 bits (4 to 7) of the N-channel
#define ADC_CONFIG_REG_N_CHANNEL_AIN_0       0x00000000
#define ADC_CONFIG_REG_N_CHANNEL_AIN_1       0x00000010
#define ADC_CONFIG_REG_N_CHANNEL_AIN_2       0x00000020
#define ADC_CONFIG_REG_N_CHANNEL_AIN_3       0x00000030
#define ADC_CONFIG_REG_N_CHANNEL_AIN_4       0x00000040
#define ADC_CONFIG_REG_N_CHANNEL_AIN_5       0x00000050
#define ADC_CONFIG_REG_N_CHANNEL_AIN_6       0x00000060
#define ADC_CONFIG_REG_N_CHANNEL_AIN_7       0x00000070
#define ADC_CONFIG_REG_N_CHANNEL_TEMP_SENSOR 0x00000080
#define ADC_CONFIG_REG_N_CHANNEL_BATT_VOLT   0x000000A0
#define ADC_CONFIG_REG_N_CHANNEL_NONE        0x000000F0

// 6 bits of VGA gain (8 to 13)
//
// upper 2 bits get multiplied by 6 dB
//     max value of 3 -> 3 x 6 = 18
//
// lower 4 bits get multiplied by 3 dB
//     max value of 15 -> 15 x 3 = 45
//
// so max value is 45 + 18 = 63
#define ADC_CONFIG_REG_MAX_GAIN_SETTING 63

// default is 0x14 = 0b010100
// upper 2 bits =   01 = 1 -> 1 x 6 dB =  6 dB
// lower 4 bits = 0100 = 4 -> 4 x 3 dB = 12 dB
// -> 6 dB + 12 dB = 18 dB
#define ADC_CONFIG_REG_DEFAULT_GAIN    0x00001400

// 2 bits of channel select (14 to 15)
#define ADC_CONFIG_REG_SELECT_REF_IN 0x00004000

// 8 bits of PULL (16 to 19)
#define ADC_CONFIG_REG_P_CHAN_PULL_NONE   0x00000000
#define ADC_CONFIG_REG_N_CHAN_PULL_NONE   0x00000000
#define ADC_CONFIG_REG_P_CHAN_PULL_HIGH   0x00010000
#define ADC_CONFIG_REG_P_CHAN_PULL_LOW    0x00020000
#define ADC_CONFIG_REG_N_CHAN_PULL_HIGH   0x00040000
#define ADC_CONFIG_REG_N_CHAN_PULL_LOW    0x00080000
#define ADC_CONFIG_REG_P_CHAN_TO_VDD      0x00100000
#define ADC_CONFIG_REG_P_CHAN_TO_GND      0x00200000
#define ADC_CONFIG_REG_N_CHAN_TO_VDD      0x00400000
#define ADC_CONFIG_REG_N_CHAN_TO_GND      0x00800000

#define ADC_CONFIG_REG_P_CHAN_VCM_VOLTAGE 0x00030000
#define ADC_CONFIG_REG_N_CHAN_VCM_VOLTAGE 0x000C0000

#define ADC_CONFIG_REG_PULL_NONE       0x00000000
#define ADC_CONFIG_REG_PULL_VCM_MODE   0x000F0000

// 20 to 23 reserved

// 3 bits of result aquisition time (24 to 26)
#define ADC_CONFIG_REG_AQUISITION_TIME_0p3_uS 0x00000000
#define ADC_CONFIG_REG_AQUISITION_TIME_1_uS   0x01000000
#define ADC_CONFIG_REG_AQUISITION_TIME_2_uS   0x02000000
#define ADC_CONFIG_REG_AQUISITION_TIME_3_uS   0x03000000
#define ADC_CONFIG_REG_AQUISITION_TIME_4_uS   0x04000000
#define ADC_CONFIG_REG_AQUISITION_TIME_8_uS   0x05000000
#define ADC_CONFIG_REG_AQUISITION_TIME_12_uS  0x06000000
#define ADC_CONFIG_REG_AQUISITION_TIME_16_uS  0x07000000

// 3 bits of end delay time (28 to 30)
#define ADC_CONFIG_REG_END_DELAY_TIME_0p3_uS 0x00000000
#define ADC_CONFIG_REG_END_DELAY_TIME_1_uS   0x10000000
#define ADC_CONFIG_REG_END_DELAY_TIME_2_uS   0x20000000
#define ADC_CONFIG_REG_END_DELAY_TIME_3_uS   0x30000000
#define ADC_CONFIG_REG_END_DELAY_TIME_4_uS   0x40000000
#define ADC_CONFIG_REG_END_DELAY_TIME_8_uS   0x50000000
#define ADC_CONFIG_REG_END_DELAY_TIME_12_uS  0x60000000
#define ADC_CONFIG_REG_END_DELAY_TIME_16_uS  0x70000000

// use this when uninit a channel
#define ADC_CONFIG_REG_CLEAR_VALUE           0x240000FF


// *****************************************************************
// these defines help when dealing with the CH_X_BURST register (0x24, 0x34, 0x44...)
#define ADC_BURST_REG_DISABLE_BURST 0x00000000
#define ADC_BURST_REG_ENABLE_BURST  0x80000000

// *****************************************************************
// these defines help when dealing with the CH_X_THRESHHOLD register (0x28, 0x38, 0x48...)
// this register is 14 bits of low threshhold (bits 0-13)
// and then 14 buts of high threshhold (bits 16-29)
// basically the low 2 bytes is low TH and high 2 bytes is high TH but only uses 14 bits

#define ADC_THRESHHOLD_LOW_DEFAULT  0x00000000
#define ADC_THRESHHOLD_HIGH_DEFAULT 0x3FFF0000

// *****************************************************************
// these defines help when dealing with the ENABLE DMA register (0x100)
#define ADC_ENDMA_REG_ENABLE_DMA 0x01

// *****************************************************************
// these defines help when dealing with the RESET DMA register (0x104)
#define ADC_ENDMA_REG_RESET_DMA 0x01

// *****************************************************************
// these defines help when dealing with the DMA SETTINGS register (0x110)

// bit 0
#define ADC_DMASET_REG_LOAD_ADDR_ON_DMA_START 0x00
#define ADC_DMASET_REG_LOAD_ADDR_ON_DMA_RESET 0x01

// bit 4,5
#define ADC_DMASET_REG_4_BYTE_FORMAT 0x00
#define ADC_DMASET_REG_2_BYTE_FORMAT 0x10
#define ADC_DMASET_REG_1_BYTE_FORMAT 0x20

// *****************************************************************
// these defines help when dealing with the INTERRUPT registers (0x120, 0x124, 0x128)
#define TR_HAL_ADC_INTERRUPT_DMA       0x0000001
#define TR_HAL_ADC_INTERRUPT_DONE      0x0000004
#define TR_HAL_ADC_INTERRUPT_VALID     0x0000008
#define TR_HAL_ADC_INTERRUPT_MODE_DONE 0x0000010
#define TR_HAL_ADC_INTERRUPT_CHAN_0    0x0040000
#define TR_HAL_ADC_INTERRUPT_CHAN_1    0x0080000
#define TR_HAL_ADC_INTERRUPT_CHAN_2    0x0100000
#define TR_HAL_ADC_INTERRUPT_CHAN_3    0x0200000
#define TR_HAL_ADC_INTERRUPT_CHAN_4    0x0400000
#define TR_HAL_ADC_INTERRUPT_CHAN_5    0x0800000
#define TR_HAL_ADC_INTERRUPT_CHAN_6    0x1000000
#define TR_HAL_ADC_INTERRUPT_CHAN_7    0x2000000

#define TR_HAL_ADC_INTERRUPT_LOW_THRESH  0x0003FF00
#define TR_HAL_ADC_INTERRUPT_HIGH_THRESH 0x0FFC0000

#define TR_HAL_ADC_INTERRUPT_ALL  0x0FFFFF1D
#define TR_HAL_ADC_INTERRUPT_BASE 0x0000001D

// these are the events that can come back to the callback
#define TR_HAL_ADC_EVENT_CH_0_RESULT  0x001
#define TR_HAL_ADC_EVENT_CH_1_RESULT  0x002
#define TR_HAL_ADC_EVENT_CH_2_RESULT  0x004
#define TR_HAL_ADC_EVENT_CH_3_RESULT  0x008
#define TR_HAL_ADC_EVENT_CH_4_RESULT  0x010
#define TR_HAL_ADC_EVENT_CH_5_RESULT  0x020
#define TR_HAL_ADC_EVENT_CH_6_RESULT  0x040
#define TR_HAL_ADC_EVENT_CH_7_RESULT  0x080
#define TR_HAL_ADC_EVENT_ALL_CH_DONE  0x100
#define TR_HAL_ADC_EVENT_DMA          0x200


// *****************************************************************
// these defines help when dealing with the RESULT OVERSAMPLE register (0x12C)
#define TR_HAL_ADC_R0_RESULT_MASK 0x00003FFF


// *****************************************************************
// these defines help when dealing with the RESULT DIGITAL and RESULT ANALOG register (0x130, 0x134)
#define TR_HAL_ADC_R1_RESULT_MASK 0x00000FFF
#define TR_HAL_ADC_R2_RESULT_MASK 0x00000FFF


// *****************************************************************
// this orients the ADC REGISTERS struct with the correct address
// so referencing a field will now read/write the correct ADC
// register chip address
#define ADC_REGISTERS ((ADC_REGISTERS_T *) CHIP_MEMORY_MAP_ADC_BASE)


// *****************************************************************
// this orients the AUX COMP  REGISTERS struct with the correct address
// so referencing a field will now read/write the correct register 
// chip address
#define AUX_COMP_REGISTERS ((AUX_COMPARATOR_REGISTERS_T *) CHIP_MEMORY_MAP_AUX_COMPARATOR_BASE)


/// ***************************************************************************
/// prototype for callback from the Trident HAL to the app when an event happens
/// ***************************************************************************
typedef void (*tr_hal_adc_event_callback_t) (uint32_t raw_result,
                                             uint32_t converted_result,
                                             uint32_t event_bitmask,
                                             uint32_t int_status);


/// ***************************************************************************
/// ADC mode
/// ***************************************************************************
typedef enum
{
    TR_HAL_ADC_MODE_ONE_SHOT = 0,
    TR_HAL_ADC_MODE_TIMER    = 1,
    TR_HAL_ADC_MODE_SCAN     = 2,

} tr_hal_adc_mode_t;


/// ***************************************************************************
/// ADC resolution (number of bits used)
/// ***************************************************************************
typedef enum
{
    TR_HAL_ADC_MODE_RESOLUTION_8_BIT  = ADC_REG_SAMPLE_OUTPUT_RESOLUTION_8_BIT,
    TR_HAL_ADC_MODE_RESOLUTION_10_BIT = ADC_REG_SAMPLE_OUTPUT_RESOLUTION_10_BIT,
    TR_HAL_ADC_MODE_RESOLUTION_12_BIT = ADC_REG_SAMPLE_OUTPUT_RESOLUTION_12_BIT,
    TR_HAL_ADC_MODE_RESOLUTION_14_BIT = ADC_REG_SAMPLE_OUTPUT_RESOLUTION_14_BIT,

} tr_hal_adc_resolution_t;


/// ***************************************************************************
/// ADC gain
/// ***************************************************************************
#define TR_HAL_ADC_DEFAULT_GAIN 6


/// ***************************************************************************
/// ADC pull mode
/// ***************************************************************************
typedef enum
{
    TR_HAL_ADC_PULL_LOW      = 0,
    TR_HAL_ADC_PULL_HIGH     = 1,
    TR_HAL_ADC_PULL_TO_VCC   = 2,
    TR_HAL_ADC_PULL_TO_GND   = 3,
    TR_HAL_ADC_PULL_NOT_USED = 4,

} tr_hal_adc_pull_mode_t;


/// ***************************************************************************
/// ADC threshholds
/// ***************************************************************************
#define TR_HAL_ADC_THRESH_LOW_DEFAULT  0x0000
#define TR_HAL_ADC_THRESH_HIGH_DEFAULT 0x0003


/// ***************************************************************************
/// ADC clock to use
/// ***************************************************************************
typedef enum
{
    TR_HAL_ADC_USE_SYSTEM_CLOCK = 0,
    TR_HAL_ADC_USE_SLOW_CLOCK   = 1,

} tr_hal_adc_clock_t;

/// ***************************************************************************
/// ADC time - used for setting aquisition_time and end_delay_time
///
/// ***************************************************************************
typedef enum
{
    TR_HAL_ADC_TIME_HALF = 0,
    TR_HAL_ADC_TIME_1    = 1,
    TR_HAL_ADC_TIME_2    = 2,
    TR_HAL_ADC_TIME_3    = 3,
    TR_HAL_ADC_TIME_4    = 4,
    TR_HAL_ADC_TIME_8    = 5,
    TR_HAL_ADC_TIME_12   = 6,
    TR_HAL_ADC_TIME_16   = 7,

} tr_hal_time_t;


/// ***************************************************************************
/// ADC settings struct - this is passed to tr_hal_adc_init
///
/// ***************************************************************************
typedef struct
{
    // pin(s) to use for the ADC
    // use both pin P (positive) and pin N (negative) when using a differential signal
    // use just pin P when using a single ended signal
    tr_hal_gpio_pin_t adc_pin_p;
    tr_hal_gpio_pin_t adc_pin_n;

    // mode: one-shot, timer, or scan
    tr_hal_adc_mode_t mode;

    // start immediately or not?
    bool start_now;

    // resolution (8-bit, 10-bit, 12-bit, 14-bit)
    tr_hal_adc_resolution_t resolution;

    // VGA gain, this must be in increments of 3 dB
    uint16_t vga_gain_in_dB;

    // pull
    tr_hal_adc_pull_mode_t pin_p_pull_mode;
    tr_hal_adc_pull_mode_t pin_n_pull_mode;

    // aquisition time and end delay time
    tr_hal_time_t aquisition_time;
    tr_hal_time_t end_delay_time;

    // clock select and clock divider
    tr_hal_adc_clock_t clock_to_use;
    uint32_t           clock_divider;

    // enable burst
    bool enable_burst_mode;

    // oversample setting
    tr_hal_adc_oversample_t oversample;

    // threshhold: high and low (for scan mode)
    uint16_t threshhold_low;
    uint16_t threshhold_high;

    // interrupts
    bool             interrupt_enabled;
    tr_hal_int_pri_t interrupt_priority;

    // event callback from HAL to App when a reading is ready
    // if the app doesn't want this, then set it to NULL
    tr_hal_adc_event_callback_t  event_handler_fx;

    // these are for doing conversion of an ADC reading to uV
    // the values could be pulled from NV from cal values
    // or set constant if there isn't enough flux
    bool     enable_microvolt_conversion;
    uint32_t min_expected_adc_reading;
    uint32_t max_expected_adc_reading;
    uint32_t min_microvolt_value;
    uint32_t max_microvolt_value;

} tr_hal_adc_settings_t;


/// ****************************************************************************
/// default values for a single ended (one pin) ADC that fires once and then
/// when start API is called. Will call the event_handler_fx when the result
/// is ready
/// ****************************************************************************
#define DEFAULT_ADC_SINGLE_ENDED_CONFIG                          \
    {                                                            \
        .adc_pin_p = (tr_hal_gpio_pin_t) { DEFAULT_ADC_PIN },    \
        .adc_pin_n = (tr_hal_gpio_pin_t) { TR_HAL_PIN_NOT_SET }, \
        .mode = TR_HAL_ADC_MODE_ONE_SHOT,                        \
        .start_now = false,                                      \
        .resolution = TR_HAL_ADC_MODE_RESOLUTION_12_BIT,         \
        .vga_gain_in_dB = TR_HAL_ADC_DEFAULT_GAIN,               \
        .pin_p_pull_mode = TR_HAL_ADC_PULL_NOT_USED,             \
        .pin_n_pull_mode = TR_HAL_ADC_PULL_NOT_USED,             \
        .aquisition_time = TR_HAL_ADC_TIME_4,                    \
        .end_delay_time = TR_HAL_ADC_TIME_2,                     \
        .clock_to_use = ADC_REG_TIMER_USE_SYSTEM_CLOCK,          \
        .clock_divider = 4,                                      \
        .enable_burst_mode = true,                               \
        .oversample = TR_HAL_ADC_REG_SAMPLE_NO_OVERSAMPLE,       \
        .threshhold_low = TR_HAL_ADC_THRESH_LOW_DEFAULT,         \
        .threshhold_high = TR_HAL_ADC_THRESH_HIGH_DEFAULT,       \
        .interrupt_enabled = false,                              \
        .interrupt_priority = TR_HAL_INTERRUPT_PRIORITY_5,       \
        .event_handler_fx = NULL,                                \
        .enable_microvolt_conversion = true,                     \
        .min_expected_adc_reading = 3900,                        \
        .max_expected_adc_reading = 10000,                       \
        .min_microvolt_value = 0,                                \
        .max_microvolt_value = 3300000,                          \
    }


/// ****************************************************************************
/// @} // end of tr_hal_T32CZ20
/// ****************************************************************************


#endif // T32CZ20_ADC_H_
