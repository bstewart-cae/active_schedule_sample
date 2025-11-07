/// ****************************************************************************
/// @file T32CZ20_gpio.h
///
/// @brief This is the chip specific include file for T32CZ20 GPIO Driver
///        note that there is a common include file for this HAL module that
///        contains the APIs (such as the init function) that should be used
///        by the application.
///
/// this chip has 32 numbered GPIOs, from 0-31
/// GPIOs 2,3 and 12,13 and 18,19 and 24,25,26,27 are not available for use
/// GPIOs 10,11 are used for programming, so these are not recommended to use
///
/// GPIOs are set to a mode that determines their function (GPIO, UART, SPI, etc)
/// GPIOs can be set as input or output
/// the drive strength, open drain, pull up, pull down, debounce can all be set
///
/// the user is encouraged to use the GPIO settings struct (tr_hal_gpio_settings_t)
/// and tr_hal_gpio_init to set GPIO pins, since there is error checking
/// in the init function
///
///
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************

#ifndef T32CZ20_GPIO_H_
#define T32CZ20_GPIO_H_

#include "tr_hal_platform.h"


/// ****************************************************************************
/// @defgroup tr_hal_gpio_cz20 GPIO CZ20
/// @ingroup tr_hal_T32CZ20
/// @{
/// ****************************************************************************


/// ******************************************************************
/// \brief max pin number
/// ******************************************************************
#define  TR_HAL_MAX_PIN_NUMBER (32)


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
#ifdef GPIO_SECURE_EN
    #define CHIP_MEMORY_MAP_GPIO_BASE     (0x50001000UL)
#else
    #define CHIP_MEMORY_MAP_GPIO_BASE     (0x40001000UL)
#endif // GPIO_SECURE_EN

#ifdef SYSCTRL_SECURE_EN
    #define CHIP_MEMORY_MAP_SYS_CTRL_BASE (0x50000000UL)
#else
    #define CHIP_MEMORY_MAP_SYS_CTRL_BASE (0x40000000UL)
#endif // SYSCTRL_SECURE_EN

#ifdef XDMA_SECURE_EN
    #define CHIP_MEMORY_MAP_XDMA_BASE (0x50028000UL)
#else
    #define CHIP_MEMORY_MAP_XDMA_BASE (0x40028000UL)
#endif // XDMA_SECURE_EN


/// ***************************************************************************
/// the struct we use so we can address registers using field names
/// ***************************************************************************
typedef struct
{
    // a WRITE sets the output state to HIGH
    // a READ gets the input pin state
    __IO uint32_t state;                                // 0x00

    // a WRITE sets the output state to LOW
    // a READ gets the GPIO interrupt status
    __IO uint32_t interrupt_status;                     // 0x04

    // these two registers set a pin for output or input. Note that
    // all pins start as inputs (0) on chip bootup.
    // to set a pin for output: set a 1 in that bit in output_enable.
    // to set a pin for input: set a 1 in that bit in input_enable.
    //
    // read output_enable (0x08) to see if the pin is set for input or output
    // A value of 0 means that pin is set for input
    // A value of 1 means that pin is set for output
    //
    // read input_enable (0x0C) to see if the output pin is set for HIGH or LOW output
    // 0 = output low
    // 1 = output high
    __IO uint32_t output_enable;                         // 0x08
    __IO uint32_t input_enable;                          // 0x0C

    // settings for configuring interrupts on GPIO pins
    // enable / disable registers are a pair
    // read the enable register to get the current setting
    __IO uint32_t enable_interrupt;                      // 0x10
    __IO uint32_t disable_interrupt;                     // 0x14
    // edge/level are a pair, default is level
    __IO uint32_t enable_edge_trigger_interrupt;         // 0x18
    __IO uint32_t enable_level_trigger_interrupt;        // 0x1C
    // high/low are a pair, default is low
    __IO uint32_t enable_active_high_trigger_interrupt;  // 0x20
    __IO uint32_t enable_active_low_trigger_interrupt;   // 0x24
    // enable_edge/disable_edge are a pait, default is disable
    __IO uint32_t enable_any_edge_trigger_interrupt;     // 0x28
    __IO uint32_t disable_any_edge_trigger_interrupt;    // 0x2C

    // this register is how to clear edge triggered interrupts
    // level sensitive interrupts stay until the pin state is cleared
    __IO uint32_t clear_interrupt;                       // 0x30

    // reserved
    __IO uint32_t reserved1;                             // 0x34

    // in addition to setting a pin to input or output with output_enable/input_enable
    // registers, these must also be set, enable when INPUT and disable when OUTPUT
    __IO uint32_t enable_input_mode;                     // 0x38
    __IO uint32_t disable_input_mode;                    // 0x3C

    // debounce settings
    // set 1 in enable_debounce for that pin bit to enable debounce
    // set 1 in disable_debounce for that pin bit to disable debounce
    // note that reading enable_debounce and disable_debounce give the same value
    // on read a 1 means enabled and 0 means disabled
    __IO uint32_t enable_debounce;                       // 0x40
    __IO uint32_t disable_debounce;                      // 0x44
    __IO uint32_t debounce_time;                         // 0x48

    __IO uint32_t reserved2;                             // 0x4C

    // these fields are for setting a pin change to wake the chip from deep sleep
    __IO uint32_t enable_wake_from_sleep;                // 0x50
    __IO uint32_t disable_wake_from_sleep;               // 0x54
    __IO uint32_t wake_on_high_state;                    // 0x58
    __IO uint32_t wake_on_low_state;                     // 0x5C

} GPIO_REGISTERS_T;


// *****************************************************************
// *** some registers are multi-purpose:

// at register address 0x00, a read means get pin state, a write is set_output_high
#define set_output_high   state

// at register address 0x04, a read is get interrupt status, a write is set_output_low
#define set_output_low  interrupt_status


// *****************************************************************
// this orients the GPIO_REGISTERS struct with the correct addresses
// so referencing a field will now read/write the correct GPIO register
// chip address
#define GPIO_CHIP_REGISTERS  ((GPIO_REGISTERS_T *) CHIP_MEMORY_MAP_GPIO_BASE)


/// ****************************************************************************
/// @brief defines for dealing with the SYS_CTRL pull registers and
///        drive registers
/// ****************************************************************************
#define TR_HAL_NUM_PULL_REGISTERS 4
#define TR_HAL_PINS_PER_PULL_REG 8

#define TR_HAL_NUM_DRIVE_REGISTERS 2
#define TR_HAL_PINS_PER_DRIVE_REG 16


/// ****************************************************************************
/// \brief offsets for where to find chip registers needed for System Control
/// register which is used to configure GPIO pins (what mode are they in and
/// pull up/down and open drain enable, etc
/// see section 17.4.2 in the chip datasheet
/// ****************************************************************************
typedef struct
{
    // bit 4 to bit 7 (0x000000F0) is the chip revision
    // bit 8 to bit 15 (0x0000FF00) is the chip ID
    __IO uint32_t chip_info;              // 0x00

    // select which real clock to use for virtual clocks: hclk, per_clk, slow_clk
    // and configure baseband frequency
    __IO uint32_t system_clock_control_0; // 0x04

    // select clk for UARTs, external slow clk, PWM, timers 0,1,2
    __IO uint32_t system_clock_control_1; // 0x08

    __IO uint32_t system_power_state;     // 0x0C

    // default is 0 = GPIO
    __IO uint32_t reserved_old_map[4];    // 0x10, 0x14, 0x18, 0x1C
    //__IO uint32_t gpio_pin_map[4];      // 0x10, 0x14, 0x18, 0x1C

    // default is 0b110 = 6 = 100K pull up
    __IO uint32_t gpio_pull_ctrl[TR_HAL_NUM_PULL_REGISTERS]; // 0x20, 0x24, 0x28, 0x2C

    // default is 0b11 = 3 = 20 mA (max)
    __IO uint32_t gpio_drv_ctrl[TR_HAL_NUM_DRIVE_REGISTERS]; // 0x30, 0x34

    // default is 0 = disabled
    __IO uint32_t open_drain_enable;      // 0x38

    __IO uint32_t enable_schmitt;         // 0x3C
    __IO uint32_t enable_filter;          // 0x40
    __IO uint32_t aio_control;            // 0x44
    __IO uint32_t cache_control;          // 0x48
    __IO uint32_t pwm_select;             // 0x4C

    // these are for telling the system how to power the RAM when in a power down
    // mode. The RAM can be retained or not retained
    __IO uint32_t sram_lowpower_0;        // 0x50
    __IO uint32_t sram_lowpower_1;        // 0x54
    __IO uint32_t sram_lowpower_2;        // 0x58
    __IO uint32_t sram_lowpower_3;        // 0x5C

    __IO uint32_t system_clock_control_2; // 0x60
    __IO uint32_t system_test;            // 0x64
    __IO uint32_t reserved[6];            // 0x68 - 0x7C

    // for setting GPIO pins as output functions
    __IO uint32_t gpio_output_mux[8];     // 0x80 - 0x9C

    // for setting GPIO pins as input functions
    // note that the reg at 0xAC == IMUX3 == 4th register == gpio_input_mux[3] is NOT CURRENTLY USED
    __IO uint32_t gpio_input_mux[8];      // 0xA0 - 0xBC

} SYS_CTRL_REGISTERS_T;

// *****************************************************************
// this is for the SYSTEM CLOCK CONTROL 0 register (0x04)

// bits 0,1 = HCLK select
// hclk: host clock (host, AHB, memory, DMA, flash, I2S, crypto)
#define SYS_CTRL_HCLK_SELECT_XTAL_CLK        0x00
#define SYS_CTRL_HCLK_SELECT_PLL_CLK         0x01
#define SYS_CTRL_HCLK_SELECT_XTAL_CLK_DIV2   0x02
#define SYS_CTRL_HCLK_SELECT_RCO_1M          0x03
#define SYS_CTRL_HCLK_SELECT_MASK            0x03

// bits 2,3 = PER CLK select
// per clk: wdog, I2C, SPI
#define SYS_CTRL_PER_CLK_SELECT_XTAL_CLK      0x00
#define SYS_CTRL_PER_CLK_SELECT_XTAL_CLK_DIV2 0x04
#define SYS_CTRL_PER_CLK_SELECT_RCO_1M        0x08
#define SYS_CTRL_PER_CLK_SELECT_MASK          0x0C

// bits 4,5 reserved

// bits 6,7 = SLOW CLK select
// slow_clk: RTC, timers 3,4
#define SYS_CTRL_SLOW_CLK_SELECT_RCO_32K  0x00
#define SYS_CTRL_SLOW_CLK_SELECT_XO_32K   0x40
#define SYS_CTRL_SLOW_CLK_SELECT_EXTERNAL 0xC0
#define SYS_CTRL_SLOW_CLK_SELECT_MASK     0xC0

// bits 8,9,10 = baseband frequency
#define SYS_CTRL_BASEBAND_FREQ_48_MHZ 0x00
#define SYS_CTRL_BASEBAND_FREQ_64_MHZ 0x100
#define SYS_CTRL_BASEBAND_FREQ_36_MHZ 0x600
#define SYS_CTRL_BASEBAND_FREQ_40_MHZ 0x700

// bits 11-14 reserved

// bit 15 - baseband PLL enable
#define SYS_CTRL_BASEBAND_PLL_ENABLE  0x8000
#define SYS_CTRL_BASEBAND_PLL_DISABLE 0x0000


// *****************************************************************
// this is for the SYSTEM CLOCK CONTROL 1 register (0x08)

// bits 0,1 UART0 clock select
// bits 2,3 UART1 clock select
// bits 4,5 UART2 clock select

// these are the values, and will need to be shifted based on the UART
// this is for bits 0 thru 5
#define SYS_CTRL_UART_CLOCK_SELECT_PER_CLOCK 0x00
#define SYS_CTRL_UART_CLOCK_SELECT_RCO_1M    0x02
#define SYS_CTRL_UART_CLOCK_SELECT_RCO_32K   0x03

#define SYS_CTRL_UART0_CLOCK_SELECT_BIT_SHIFT 0
#define SYS_CTRL_UART1_CLOCK_SELECT_BIT_SHIFT 2
#define SYS_CTRL_UART2_CLOCK_SELECT_BIT_SHIFT 4

// bits 6,7 reserved

// bits 8 to 13 are for setting the slow clock to an external clock

// bit 13 set = enable slow clock to use external source
#define SYS_CTRL_SLOW_CLK_ENABLE_EXTERNAL 0x2000

// the external clock source is a GPIO number, in bits 8 to 12
// so shift the GPIO number by this amount of bits
#define SYS_CTRL_SLOW_CLK_EXTERNAL_SRC_SHIFT 8

// bits 16,17 = PWM0 clock select
// bits 18,19 = PWM1 clock select
// bits 20,21 = PWM2 clock select
// bits 22,23 = PWM3 clock select
// bits 24,25 = PWM4 clock select

#define SYS_CTRL_PWM_CLOCK_SELECT_HCLK     0x00
#define SYS_CTRL_PWM_CLOCK_SELECT_PER_CLK  0x01
#define SYS_CTRL_PWM_CLOCK_SELECT_RCO_1M   0x02
#define SYS_CTRL_PWM_CLOCK_SELECT_SLOW_CLK 0x03

#define SYS_CTRL_PWM0_CLOCK_SELECT_BIT_SHIFT 16
#define SYS_CTRL_PWM1_CLOCK_SELECT_BIT_SHIFT 18
#define SYS_CTRL_PWM2_CLOCK_SELECT_BIT_SHIFT 20
#define SYS_CTRL_PWM3_CLOCK_SELECT_BIT_SHIFT 22
#define SYS_CTRL_PWM4_CLOCK_SELECT_BIT_SHIFT 24

// bits 26,27 = Timer0 clock select
// bits 28,29 = Timer1 clock select
// bits 30,31 = Timer2 clock select

#define SYS_CTRL_TIMER_CLOCK_SELECT_PER_CLK  0x00
#define SYS_CTRL_TIMER_CLOCK_SELECT_RCO_1M   0x02
#define SYS_CTRL_TIMER_CLOCK_SELECT_SLOW_CLK 0x03

#define SYS_CTRL_TIMER0_CLOCK_SELECT_BIT_SHIFT 26
#define SYS_CTRL_TIMER1_CLOCK_SELECT_BIT_SHIFT 28
#define SYS_CTRL_TIMER2_CLOCK_SELECT_BIT_SHIFT 30


// *****************************************************************
// this is for the SYSTEM POWER STATE register (0x0C)
#define TR_HAL_POWER_NORMAL     0x00
#define TR_HAL_POWER_LITE_SLEEP 0x01
#define TR_HAL_POWER_DEEP_SLEEP 0x02
#define TR_HAL_POWER_POWERDOWN  0x04


// *****************************************************************
// this orients the SYSCTRL_REGISTERS struct with the correct addresses
// so referencing a field will now read/write the correct SYSCTRL register
// chip address
#define SYS_CTRL_CHIP_REGISTERS  ((SYS_CTRL_REGISTERS_T *) CHIP_MEMORY_MAP_SYS_CTRL_BASE)

// these are for setting the system_clock_control register
#define     SCC_UART0_CLOCK_BIT                 16
#define     SCC_UART1_CLOCK_BIT                 17
#define     SCC_UART2_CLOCK_BIT                 18



/// ****************************************************************************
/// \brief offsets for where to find chip registers needed for xDMA
/// register .
/// see section 10.3in the chip datasheet
/// ****************************************************************************
typedef struct
{
    __IO uint32_t enable;              // 0x00

} XDMA_REGISTERS_T;

// *****************************************************************
// this orients the XDMA_REGISTERS struct with the correct addresses
// so referencing a field will now read/write the correct XDMA register
// chip address
#define XDMA_CHIP_REGISTERS  ((XDMA_REGISTERS_T *) CHIP_MEMORY_MAP_XDMA_BASE)


/// ****************************************************************************
/// \brief these are the pin MODEs to be passed to tr_hal_gpio_set_mode
/// note that these are defined by the chip and cannot be changed
/// see section 17.3 of datasheet
/// ****************************************************************************
typedef enum
{
    // ******************************************************
    // values for pin output options
    // these values are set by the chip - DO NOT CHANGE
    // ******************************************************
    // simple GPIO
    TR_HAL_GPIO_MODE_GPIO             = 0x00,
    // -------- UART 0 ---------------------
    TR_HAL_GPIO_MODE_UART_0_TX        = 0x01,
    // -------- UART 1 ---------------------
    TR_HAL_GPIO_MODE_UART_1_TX        = 0x02,
    TR_HAL_GPIO_MODE_UART_1_RTSN      = 0x03,
    // -------- UART 2 ---------------------
    TR_HAL_GPIO_MODE_UART_2_TX        = 0x04,
    TR_HAL_GPIO_MODE_UART_2_RTSN      = 0x05,
    // -------- PWM ------------------------
    TR_HAL_GPIO_MODE_PWM0             = 0x06,
    TR_HAL_GPIO_MODE_PWM1             = 0x07,
    TR_HAL_GPIO_MODE_PWM2             = 0x08,
    TR_HAL_GPIO_MODE_PWM3             = 0x09,
    TR_HAL_GPIO_MODE_PWM4             = 0x0A,
    // -------- infrared modulator ---------
    TR_HAL_GPIO_MODE_IRM              = 0x0B,
    // -------- I2C ------------------------
    TR_HAL_GPIO_MODE_I2C_0_MASTER_SCL = 0x0C,
    TR_HAL_GPIO_MODE_I2C_0_MASTER_SDA = 0x0D,
    TR_HAL_GPIO_MODE_I2C_1_MASTER_SCL = 0x0E,
    TR_HAL_GPIO_MODE_I2C_1_MASTER_SDA = 0x0F,
    TR_HAL_GPIO_MODE_I2C_SLAVE_SCL    = 0x10,
    TR_HAL_GPIO_MODE_I2C_SLAVE_SDA    = 0x11,
    // -------- SPI 0 ----------------------
    TR_HAL_GPIO_MODE_SPI_0_CLK        = 0x12,
    TR_HAL_GPIO_MODE_SPI_0_SDATA_0    = 0x13,
    TR_HAL_GPIO_MODE_SPI_0_SDATA_1    = 0x14,
    TR_HAL_GPIO_MODE_SPI_0_SDATA_2    = 0x15,
    TR_HAL_GPIO_MODE_SPI_0_SDATA_3    = 0x16,
    TR_HAL_GPIO_MODE_SPI_0_CS_0       = 0x17,
    TR_HAL_GPIO_MODE_SPI_0_CS_1       = 0x18,
    TR_HAL_GPIO_MODE_SPI_0_CS_2       = 0x19,
    TR_HAL_GPIO_MODE_SPI_0_CS_3       = 0x1A,
    // -------- SPI 1 ----------------------
    TR_HAL_GPIO_MODE_SPI_1_CLK        = 0x1B,
    TR_HAL_GPIO_MODE_SPI_1_SDATA_0    = 0x1C,
    TR_HAL_GPIO_MODE_SPI_1_SDATA_1    = 0x1D,
    TR_HAL_GPIO_MODE_SPI_1_SDATA_2    = 0x1E,
    TR_HAL_GPIO_MODE_SPI_1_SDATA_3    = 0x1F,
    TR_HAL_GPIO_MODE_SPI_1_CS_0       = 0x20,
    TR_HAL_GPIO_MODE_SPI_1_CS_1       = 0x21,
    TR_HAL_GPIO_MODE_SPI_1_CS_2       = 0x22,
    TR_HAL_GPIO_MODE_SPI_1_CS_3       = 0x23,
    // -------- I2S ------------------------
    TR_HAL_GPIO_MODE_I2S_BCK          = 0x24,
    TR_HAL_GPIO_MODE_I2S_WCK          = 0x25,
    TR_HAL_GPIO_MODE_I2S_SDO          = 0x26,
    TR_HAL_GPIO_MODE_I2S_MCLK         = 0x27,

    // ---- special case -------------------
    // ---- serial wire debug (pin 11) -----
    TR_HAL_GPIO_MODE_SWDIO            = 0x2F,

    // ----- radio debug -------------------
    TR_HAL_GPIO_MODE_DBG0             = 0x30,
    TR_HAL_GPIO_MODE_DBG1             = 0x31,
    TR_HAL_GPIO_MODE_DBG2             = 0x32,
    TR_HAL_GPIO_MODE_DBG3             = 0x33,
    TR_HAL_GPIO_MODE_DBG4             = 0x34,
    TR_HAL_GPIO_MODE_DBG5             = 0x35,
    TR_HAL_GPIO_MODE_DBG6             = 0x36,
    TR_HAL_GPIO_MODE_DBG7             = 0x37,
    TR_HAL_GPIO_MODE_DBG8             = 0x38,
    TR_HAL_GPIO_MODE_DBG9             = 0x39,
    TR_HAL_GPIO_MODE_DBGA             = 0x3A,
    TR_HAL_GPIO_MODE_DBGB             = 0x3B,
    TR_HAL_GPIO_MODE_DBGC             = 0x3C,
    TR_HAL_GPIO_MODE_DBGD             = 0x3D,
    TR_HAL_GPIO_MODE_DBGE             = 0x3E,
    TR_HAL_GPIO_MODE_DBGF             = 0x3F,

    // max
    TR_HAL_GPIO_OUTPUT_MODE_MAX       = 0x3F,

    // *****************************************************************
    // values for pin input options
    // these values are set by the chip - DO NOT CHANGE
    // input values - these are NOT set by the chip
    // *****************************************************************
    TR_HAL_GPIO_INPUT_MODE_MIN  = 0xE0,

    // imux register 0xA0
    TR_HAL_GPIO_MODE_UART_2_CTS = 0xE0,
    TR_HAL_GPIO_MODE_UART_2_RX  = 0xE1,
    TR_HAL_GPIO_MODE_UART_1_CTS = 0xE2,
    TR_HAL_GPIO_MODE_UART_1_RX  = 0xE3,

    // imux register 0xA4
    //TR_HAL_GPIO_MODE_I2C_SLAVE_SDA // both output and input
    //TR_HAL_GPIO_MODE_I2C_SLAVE_SCL // both output and input
    TR_HAL_GPIO_MODE_I2S_SDI   = 0xE4,
    TR_HAL_GPIO_MODE_UART_0_RX = 0xE5,

    // imux register 0xA8
    //TR_HAL_GPIO_MODE_I2C_1_MASTER_SDA // both output and input
    //TR_HAL_GPIO_MODE_I2C_1_MASTER_SCL // both output and input
    //TR_HAL_GPIO_MODE_I2C_0_MASTER_SDA // both output and input
    //TR_HAL_GPIO_MODE_I2C_0_MASTER_SCL // both output and input

    // imux register 0xB0
    TR_HAL_GPIO_MODE_SPI_0_PERIPH_SDATA_1 = 0xE6,
    TR_HAL_GPIO_MODE_SPI_0_PERIPH_SDATA_0 = 0xE7,
    TR_HAL_GPIO_MODE_SPI_0_PERIPH_CLK     = 0xE8,
    TR_HAL_GPIO_MODE_SPI_0_PERIPH_CS      = 0xE9,

    // imux register 0xB4
    TR_HAL_GPIO_MODE_SPI_0_PERIPH_SDATA_3 = 0xEA,
    TR_HAL_GPIO_MODE_SPI_0_PERIPH_SDATA_2 = 0xEB,

    // imux register 0xB8
    TR_HAL_GPIO_MODE_SPI_1_PERIPH_SDATA_1 = 0xEC,
    TR_HAL_GPIO_MODE_SPI_1_PERIPH_SDATA_0 = 0xED,
    TR_HAL_GPIO_MODE_SPI_1_PERIPH_CLK     = 0xEE,
    TR_HAL_GPIO_MODE_SPI_1_PERIPH_CS      = 0xEF,

    // imux register 0xBC
    TR_HAL_GPIO_MODE_SPI_1_PERIPH_SDATA_3 = 0xF0,
    TR_HAL_GPIO_MODE_SPI_1_PERIPH_SDATA_2 = 0xF1,
    TR_HAL_GPIO_INPUT_MODE_MAX            = 0xF1,

} tr_hal_pin_mode_t;


/// ****************************************************************************
/// \brief values for setting the direction in the Trident HAL GPIO APIs
/// ****************************************************************************
typedef enum
{
    TR_HAL_GPIO_DIRECTION_OUTPUT = 0,
    TR_HAL_GPIO_DIRECTION_INPUT  = 1,
} tr_hal_direction_t;


/// ****************************************************************************
/// \brief values for setting the level in the Trident HAL GPIO APIs
/// ****************************************************************************
typedef enum
{
    TR_HAL_GPIO_LEVEL_LOW  = 0,
    TR_HAL_GPIO_LEVEL_HIGH = 1,
} tr_hal_level_t;


/// ****************************************************************************
/// \brief values for setting the interrupt trigger in the Trident HAL GPIO APIs
/// ****************************************************************************
typedef enum
{
    TR_HAL_GPIO_TRIGGER_NONE         = 0,
    TR_HAL_GPIO_TRIGGER_RISING_EDGE  = 1,
    TR_HAL_GPIO_TRIGGER_FALLING_EDGE = 2,
    TR_HAL_GPIO_TRIGGER_EITHER_EDGE  = 3,
    TR_HAL_GPIO_TRIGGER_LEVEL_LOW    = 4,
    TR_HAL_GPIO_TRIGGER_LEVEL_HIGH   = 5,
} tr_hal_trigger_t;


/// ****************************************************************************
/// \brief values for setting the pull option in the Trident HAL GPIO APIs
/// NOTE: these CANNOT be changed. These are in the chip data sheet
/// THESE ARE NOT ARBITRARY
/// ****************************************************************************
typedef enum
{
    TR_HAL_PULLOPT_PULL_NONE      = 0,
    TR_HAL_PULLOPT_PULL_DOWN_10K  = 1,
    TR_HAL_PULLOPT_PULL_DOWN_100K = 2,
    TR_HAL_PULLOPT_PULL_DOWN_1M   = 3,
    TR_HAL_PULLOPT_PULL_ALSO_NONE = 4,
    TR_HAL_PULLOPT_PULL_UP_10K    = 5,
    TR_HAL_PULLOPT_PULL_UP_100K   = 6,
    TR_HAL_PULLOPT_PULL_UP_1M     = 7,
    TR_HAL_PULLOPT_MAX_VALUE      = 7,
} tr_hal_pullopt_t;


/// ****************************************************************************
/// \brief values for setting the debounce time register
/// each individual GPIO can be set to enable or disable debounce
/// but the debounce time is set globally for ALL GPIOs.
/// NOTE: these CANNOT be changed. These come from the chip data sheet
/// ****************************************************************************
typedef enum
{
    TR_HAL_DEBOUNCE_TIME_32_CLOCKS   = 0,
    TR_HAL_DEBOUNCE_TIME_64_CLOCKS   = 1,
    TR_HAL_DEBOUNCE_TIME_128_CLOCKS  = 2,
    TR_HAL_DEBOUNCE_TIME_256_CLOCKS  = 3,
    TR_HAL_DEBOUNCE_TIME_512_CLOCKS  = 4,
    TR_HAL_DEBOUNCE_TIME_1024_CLOCKS = 5,
    TR_HAL_DEBOUNCE_TIME_2048_CLOCKS = 6,
    TR_HAL_DEBOUNCE_TIME_4096_CLOCKS = 7,
    TR_HAL_DEBOUNCE_TIME_MAX_VALUE   = 7,
} tr_hal_debounce_time_t;



/// ****************************************************************************
/// \brief values for setting the GPIO drive strength in the Trident HAL APIs
/// NOTE: these CANNOT be changed. These come from the chip data sheet
/// ****************************************************************************
typedef enum
{
    TR_HAL_DRIVE_STRENGTH_4_MA   = 0,
    TR_HAL_DRIVE_STRENGTH_10_MA   = 1,
    TR_HAL_DRIVE_STRENGTH_14_MA   = 2,
    TR_HAL_DRIVE_STRENGTH_20_MA   = 3,
    TR_HAL_DRIVE_STRENGTH_MAX     = 3,
    TR_HAL_DRIVE_STRENGTH_DEFAULT = 3,
} tr_hal_drive_strength_t;


/// ****************************************************************************
/// \brief values for setting the GPIO wake mode
/// ****************************************************************************
typedef enum
{
    TR_HAL_WAKE_MODE_NONE       = 0,
    TR_HAL_WAKE_MODE_INPUT_LOW  = 1,
    TR_HAL_WAKE_MODE_INPUT_HIGH = 2,
} tr_hal_wake_mode_t;


/// ****************************************************************************
/// \brief GPIO interrupt callback functions
/// ****************************************************************************

// there is only one event that can come back from a GPIO callback currently
// reserve 0 for none in case we need it later
typedef enum
{
    TR_HAL_GPIO_EVENT_NONE            = 0,
    TR_HAL_GPIO_EVENT_INPUT_TRIGGERED = 1,
} tr_hal_gpio_event_t;

// GPIO/button interrupt callback function type
// to create a function in the app that can be used as a callback:
//     void app_gpio_button_callback(tr_hal_gpio_pin_t pin, tr_hal_gpio_event_t event)
//     where pin = the pin triggered
//     and event = what happened (TR_HAL_GPIO_EVENT_xxx)
// this is set using the tr_hal_gpio_set_interrupt_callback API
typedef void (* tr_hal_gpio_event_callback_t)(tr_hal_gpio_pin_t pin, tr_hal_gpio_event_t event);


/// ****************************************************************************
/// GPIO Settings struct
///
/// instead of calling various functions to setup a GPIO, create an instance of
/// this struct, fill in the details, and pass it to tr_hal_gpio_init()
///
/// there is also a way to setup a GPIO with reasonable defaults, and just change
/// the options that you need to, example:
///
///     // default setting for an input (for use as a button)
///     tr_hal_gpio_settings_t button_cfg = DEFAULT_GPIO_INPUT_CONFIG;
///
///     // we want to wake from deep sleep, so adjust that
///     button_cfg.wake_from_deep_sleep = true;
///
///     // set pin 9 for the button config
///     tr_hal_gpio_pin_t button_pin = { 9 }
///     tr_hal_gpio_init(button_pin, button_cfg);
///
///     // default setting for an output (for use as an LED)
///     tr_hal_gpio_settings_t led_cfg = DEFAULT_GPIO_OUTPUT_CONFIG;
///
///     // we want to adjust pull mode
///     led_cfg.pull_mode = TR_HAL_PULLOPT_PULL_DOWN_1M;
///
///     // set pin 15 for an LED
///     tr_hal_gpio_pin_t led_pin = { 15 }
///     tr_hal_gpio_init(led_pin, led_cfg);
///
/// ****************************************************************************
typedef struct
{
    // direction - INPUT or OUTPUT
    tr_hal_direction_t direction;

    // output level
    tr_hal_level_t output_level;

    // open drain
    bool enable_open_drain;

    // output drive strength
    tr_hal_drive_strength_t drive_strength;

    // interrupt trigger (edge high, edge low, etc)
    // (note: int priority is not set here since it is set for ALL GPIOs)
    tr_hal_trigger_t interrupt_trigger;

    // event callback
    tr_hal_gpio_event_callback_t event_handler_fx;

    // pull up / pull down
    tr_hal_pullopt_t pull_mode;

    // debounce
    // (note: debounce time is not set here since it is set for ALL GPIOs)
    bool enable_debounce;

    // set wake mode for this GPIO
    tr_hal_wake_mode_t wake_mode;

} tr_hal_gpio_settings_t;


/// ****************************************************************************
/// default values so an app can quickly load a reasonable set of
/// values for an input or output GPIO
///
/// ****************************************************************************
#define DEFAULT_GPIO_OUTPUT_CONFIG                        \
    {                                                     \
        .direction = TR_HAL_GPIO_DIRECTION_OUTPUT,        \
        .output_level = TR_HAL_GPIO_LEVEL_HIGH,           \
        .enable_open_drain = false,                       \
        .drive_strength =  TR_HAL_DRIVE_STRENGTH_DEFAULT, \
        .interrupt_trigger = TR_HAL_GPIO_TRIGGER_NONE,    \
        .event_handler_fx = NULL,                         \
        .pull_mode = TR_HAL_PULLOPT_PULL_NONE,            \
        .enable_debounce = false,                         \
        .wake_mode = TR_HAL_WAKE_MODE_NONE,               \
    }

#define DEFAULT_GPIO_INPUT_CONFIG                             \
    {                                                         \
        .direction = TR_HAL_GPIO_DIRECTION_INPUT,             \
        .interrupt_trigger = TR_HAL_GPIO_TRIGGER_EITHER_EDGE, \
        .event_handler_fx = NULL,                             \
        .pull_mode = TR_HAL_PULLOPT_PULL_NONE,                \
        .enable_debounce = true,                              \
        .wake_mode = false,                                   \
        .output_level = TR_HAL_GPIO_LEVEL_HIGH,               \
        .enable_open_drain = false,                           \
        .drive_strength =  TR_HAL_DRIVE_STRENGTH_DEFAULT      \
    }


/// ****************************************************************************
/// @} // end of tr_hal_T32CZ20
/// ****************************************************************************


#endif // T32CZ20_GPIO_H_
