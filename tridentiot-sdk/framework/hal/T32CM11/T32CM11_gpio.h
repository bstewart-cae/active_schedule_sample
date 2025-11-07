/// ****************************************************************************
/// @file T32CM11_gpio.h
///
/// @brief This is the chip specific include file for T32CM11 GPIO Driver
///        note that there is a common include file for this HAL module that
///        contains the APIs (such as the init function) that should be used
///        by the application.
///
/// this chip has 32 numbered GPIOs, from 0-31
/// GPIOs 10,11,12,13 and 18,19 and 24,25,26,27 are not available for use
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

#ifndef T32CM11_GPIO_H_
#define T32CM11_GPIO_H_

#include "tr_hal_platform.h"


/// ****************************************************************************
/// @defgroup tr_hal_gpio_cm11 GPIO CM11
/// @ingroup tr_hal_T32CM11
/// @{
/// ****************************************************************************


/// ******************************************************************
/// \brief max pin number
/// ******************************************************************
#define  TR_HAL_MAX_PIN_NUMBER (32)


/// ******************************************************************
/// \brief chip register addresses
/// section 3.1 of the data sheet explains the Memory map.
/// this gives the base address for how to write the chip registers
/// the chip registers are how the software interacts configures GPIOs,
/// reads GPIOs, and gets/sets information on the chip We create a
/// struct below that addresses the individual
/// registers. This makes it so we can use this base address and a
/// struct field to read or write a chip register
/// ******************************************************************
#define CHIP_MEMORY_MAP_GPIO_BASE     (0x40000000UL)
#define CHIP_MEMORY_MAP_SYS_CTRL_BASE (0x40800000UL)


/// ***************************************************************************
/// the struct we use so we can address registers using field names
/// ***************************************************************************
typedef struct
{
    // read current pin input state and interrupt status
    // (note these 2 registers have different meanings for read and
    // write. See below for their alternate use)
    __IO uint32_t state;                                // 0x00
    __IO uint32_t interrupt_status;                     // 0x04

    // these two registers set a pin for output or input. Note that
    // all pins start as inputs on chip bootup.
    // to set a pin for output: set a 1 in that bit in output_enable.
    // to set a pin for input: set a 1 in that bit in input_enable.
    // note that reading output_enable and input_enable will give the same value.
    // A value of 1 means that pin is set for input
    // A value of 0 means that pin is set for output
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

    // the next 3 fields are test fields - we don't expect them to be used
    __IO uint32_t enable_loopback_mode;                  // 0x34
    __IO uint32_t enable_inhibit_input_mode;             // 0x38
    __IO uint32_t disable_inhibit_input_mode;            // 0x3C

    // debounce settings
    // set 1 in enable_debounce for that pin bit to enable debounce
    // set 1 in disable_debounce for that pin bit to disable debounce
    // note that reading enable_debounce and disable_debounce give the same value
    // on read a 1 means enabled and 0 means disabled
    __IO uint32_t enable_debounce;                       // 0x40
    __IO uint32_t disable_debounce;                      // 0x44
    __IO uint32_t debounce_time;                         // 0x48

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
/// @brief defines for dealing with the SYS_CTRL pull registers, mode registers,
///        and drive registers
/// ****************************************************************************
#define TR_HAL_NUM_PULL_REGISTERS 4
#define TR_HAL_PINS_PER_PULL_REG 8

#define TR_HAL_NUM_MODE_REGISTERS 4
#define TR_HAL_PINS_PER_MODE_REG 8

#define TR_HAL_NUM_DRIVE_REGISTERS 2
#define TR_HAL_PINS_PER_DRIVE_REG 16


/// ****************************************************************************
/// \brief offsets for where to find chip registers needed for System Control
/// register which is used to configure GPIO pins (what mode are they in and
/// pull up/down and open drain enable, etc
/// see section 19.3 in the chip datasheet
/// ****************************************************************************
typedef struct
{
    __IO uint32_t sleep_enable;          // 0x00
    __IO uint32_t system_clock_control;  // 0x04
    __IO uint32_t reserved[2];           // 0x08, 0x0C

    // default is 0 = GPIO
    __IO uint32_t gpio_pin_map[TR_HAL_NUM_MODE_REGISTERS];   // 0x10, 0x14, 0x18, 0x1C

    // default is 0b110 = 6 = 100K pull up
    __IO uint32_t gpio_pull_ctrl[TR_HAL_NUM_PULL_REGISTERS]; // 0x20, 0x24, 0x28, 0x2C

    // default is 0b11 = 3 = 20 mA (max)
    __IO uint32_t gpio_drv_ctrl[TR_HAL_NUM_DRIVE_REGISTERS]; // 0x30, 0x34

    // default is 0 = disabled
    __IO uint32_t open_drain_enable;     // 0x38

    // note this only works for pins 24-31, of which 24-27 are not available
    // lowest 8 bits are used to enable these 8 pins
    // bit 0 = pin 24... bit 7 = pin 31
    __IO uint32_t analog_IO_enable;     // 0x3C

    // random number generator registers (see section 20)
    __IO uint32_t random_number_trigger; // 0x40
    __IO uint32_t random_number_select;  // 0x44
    __IO uint32_t random_number_status;  // 0x48
    __IO uint32_t random_number_value;   // 0x4C

    // reserved
    __IO uint32_t reserved2[4];          // 0x50, 0x54, 0x58, 0x5C
    __IO uint32_t scratchpad[8];         // 0x60, 0x64, 0x68, 0x6C, 0x70, 0x74, 0x78, 0x7C

    // wake from sleep on low-to-high transition of GPIO
    __IO uint32_t enable_wake_on_high;   // 0x80

    // wake from sleep on high-to-low transition of GPIO
    __IO uint32_t enable_wake_on_low;    // 0x84

    // reserved
    __IO uint32_t reserved3[2];          // 0x88, 0x8C

    // chip info
    // bit 4 to bit 7 (0x000000F0) is the chip revision
    // bit 8 to bit 15 (0x0000FF00) is the chip ID
    __IO uint32_t chip_info;             // 0x90


} SYS_CTRL_REGISTERS_T;

// *****************************************************************
// this orients the SYSCTRL_REGISTERS struct with the correct addresses
// so referencing a field will now read/write the correct SYSCTRL register
// chip address
#define SYS_CTRL_CHIP_REGISTERS  ((SYS_CTRL_REGISTERS_T *) CHIP_MEMORY_MAP_SYS_CTRL_BASE)

// these are for setting the system_clock_control register
#define     SCC_UART0_CLOCK_BIT                 16
#define     SCC_UART1_CLOCK_BIT                 17
#define     SCC_UART2_CLOCK_BIT                 18

#define TR_HAL_ENABLE_LITE_SLEEP 1
#define TR_HAL_ENABLE_DEEP_SLEEP 2

/// ****************************************************************************
/// \brief these are the pin MODEs to be passed to tr_hal_gpio_set_mode
/// note that these are defined by the chip and cannot be changed
/// see section 19.3.3 of datasheet
/// ****************************************************************************
typedef enum
{
    TR_HAL_GPIO_MODE_GPIO  = 0,
    TR_HAL_GPIO_MODE_QSPI0 = 1,
    TR_HAL_GPIO_MODE_I2C   = 4,
    TR_HAL_GPIO_MODE_UART  = 6,

    TR_HAL_GPIO_MODE_I2S   = 4,
    TR_HAL_GPIO_MODE_PWM   = 4,
    TR_HAL_GPIO_MODE_PWM0  = 1,
    TR_HAL_GPIO_MODE_PWM1  = 2,
    TR_HAL_GPIO_MODE_PWM2  = 3,
    TR_HAL_GPIO_MODE_PWM3  = 5,
    TR_HAL_GPIO_MODE_PWM4  = 7,

    // there are 4 pins that don't adhere to the normal
    // PWM modes
    TR_HAL_GPIO_MODE_PWM_SPECIAL = 4,

    // for setting to SPI mode
    TR_HAL_GPIO_MODE_SPI0  = 1,
    TR_HAL_GPIO_MODE_SPI1  = 5,

    // SPI1 has multiple CS options
    TR_HAL_GPIO_MODE_SPI0_CS1  = 2,
    TR_HAL_GPIO_MODE_SPI0_CS2  = 3,
    TR_HAL_GPIO_MODE_SPI0_CS3  = 6,

    TR_HAL_GPIO_MODE_MAX   = 7,
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
        .wake_mode = TR_HAL_WAKE_MODE_NONE,                   \
        .output_level = TR_HAL_GPIO_LEVEL_HIGH,               \
        .enable_open_drain = false,                           \
        .drive_strength =  TR_HAL_DRIVE_STRENGTH_DEFAULT      \
    }


/// ****************************************************************************
/// @} // end of tr_hal_T32CM11
/// ****************************************************************************


#endif // T32CM11_GPIO_H_
