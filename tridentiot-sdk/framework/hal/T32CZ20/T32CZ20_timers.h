/// ****************************************************************************
/// @file T32CZ20_timers.h
///
/// @brief This is the chip specific include file for T32CZ20 Timers Driver
///        note that there is a common include file for this HAL module that
///        contains the APIs (such as the init function) that should be used
///        by the application.
///
/// This chip supports 5 timers
///     timers 0,1,2 run at  32 MHz
///     timers 3,4   run at  32 KHz
///
///
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************

#ifndef T32CZ20_TIMERS_H_
#define T32CZ20_TIMERS_H_

#include "tr_hal_platform.h"


/// ****************************************************************************
/// @defgroup tr_hal_timers_cz20 Timers CZ20
/// @ingroup tr_hal_T32CZ20
/// @{
/// ****************************************************************************


#define TR_HAL_NUM_TIMERS 5

// timer IDs
typedef enum
{
    TIMER_0_ID = 0,
    TIMER_1_ID = 1,
    TIMER_2_ID = 2,
    SLOW_CLOCK_TIMER_0_ID = 3,
    SLOW_CLOCK_TIMER_1_ID = 4,

} tr_hal_timer_id_t;

// these are in a non-sequential order because that is how the chip register works
// do not change these - these are set based on the chip
typedef enum
{
    TR_HAL_TIMER_PRESCALER_1       = (0 << 2),
    TR_HAL_TIMER_PRESCALER_16      = (1 << 2),
    TR_HAL_TIMER_PRESCALER_256     = (2 << 2),
    TR_HAL_TIMER_PRESCALER_2       = (3 << 2),
    TR_HAL_TIMER_PRESCALER_8       = (4 << 2),
    TR_HAL_TIMER_PRESCALER_32      = (5 << 2),
    TR_HAL_TIMER_PRESCALER_128     = (6 << 2),
    TR_HAL_TIMER_PRESCALER_1024    = (7 << 2),
    TR_HAL_TIMER_PRESCALER_MAX     = (7 << 2),
} tr_hal_timer_prescalar_t;


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
#ifdef TIMER0_SECURE_EN
    #define CHIP_MEMORY_MAP_TIMER0_BASE     (0x5000A000UL)
#else
    #define CHIP_MEMORY_MAP_TIMER0_BASE     (0x4000A000UL)
#endif // TIMER0_SECURE_EN

#ifdef TIMER1_SECURE_EN
    #define CHIP_MEMORY_MAP_TIMER1_BASE     (0x5000B000UL)
#else
    #define CHIP_MEMORY_MAP_TIMER1_BASE     (0x4000B000UL)
#endif // TIMER1_SECURE_EN

#ifdef TIMER2_SECURE_EN
    #define CHIP_MEMORY_MAP_TIMER2_BASE     (0x5000C000UL)
#else
    #define CHIP_MEMORY_MAP_TIMER2_BASE     (0x4000C000UL)
#endif // TIMER2_SECURE_EN

#ifdef TIMER32K0_SECURE_EN
    #define CHIP_MEMORY_MAP_TIMER3_BASE     (0x5000D000UL)
#else
    #define CHIP_MEMORY_MAP_TIMER3_BASE     (0x4000D000UL)
#endif // TIMER32K0_SECURE_EN

#ifdef TIMER32K1_SECURE_EN
    #define CHIP_MEMORY_MAP_TIMER4_BASE     (0x5000E000UL)
#else
    #define CHIP_MEMORY_MAP_TIMER4_BASE     (0x4000E000UL)
#endif // TIMER32K1_SECURE_EN



/// ***************************************************************************
/// the struct we use so we can address registers using field names
/// this is valid for timer0, timer1, timer2
/// ***************************************************************************
typedef struct
{
    // timer start value, it counts down from this
    __IO uint32_t initial_value;                        // 0x00

    // current countdown value
    __IO uint32_t current_countdown_value;              // 0x04

    // timer control - enable, disable, mode, and set prescale value
    __IO uint32_t control;                              // 0x08

    // to clear the timer interrupt
    __IO uint32_t clear_interrupt;                      // 0x0C

    // for capture (reading input) wave forms
    __IO uint32_t capture_clear;                        // 0x10
    __IO uint32_t capture_ch_0;                         // 0x14
    __IO uint32_t capture_ch_1;                         // 0x18

    // timer prescale value. if this is 0 then the prescale field in
    // the control register is used.
    __IO uint32_t prescalar;                            // 0x1C

    // capture and PWM
    __IO uint32_t expire_value;                         // 0x20
    __IO uint32_t capture_enable;                       // 0x24
    __IO uint32_t capture_IO_select;                    // 0x28
    __IO uint32_t PWM_threshhold;                       // 0x2C
    __IO uint32_t PWM_phase;                            // 0x30

} FAST_TIMER_REGISTERS_T;


/// ***************************************************************************
/// the struct we use so we can address registers using field names
/// this is valid for slow timer0 and slow timer1
/// ***************************************************************************
typedef struct
{
    // timer start value, it counts down from this
    __IO uint32_t initial_value;                        // 0x00

    // current countdown value
    __IO uint32_t current_countdown_value;              // 0x04

    // timer control - enable, disable, mode, and set prescale value
    __IO uint32_t control;                              // 0x08

    // to clear the timer interrupt
    __IO uint32_t clear_interrupt;                      // 0x0C

    __IO uint32_t interrupt_repeat;                     // 0x10

    // timer prescale value. if this is 0 then the prescale field in
    // the control register is used.
    __IO uint32_t prescalar;                            // 0x14

    __IO uint32_t expire_value;                         // 0x18

} SLOW_TIMER_REGISTERS_T;


// *****************************************************************
// these defines help when dealing with the CONTROL REGISTER (0x08)
// bit 0 = count down (default) or up
#define CR_COUNT_MODE_DOWN   0x00
#define CR_COUNT_MODE_UP     0x01
// bit 1 = one shot enable (1) or one shot disable(0)
#define CR_ONE_SHOT_DISABLE  0x00
#define CR_ONE_SHOT_ENABLE   0x10
// bits 2-4 = prescalar
#define CR_PRESCALER_MASK    0x1C
// bit 5 = interrupt enable
#define CR_INT_ENABLE_BIT    0x20
// bit 6 = free running (0) or periodic(1)
#define CR_MODE_BIT          0x40
// bit 7 = timer running (1 = enabled) or not (0 = disabled)
#define CR_TIMER_RUNNING_BIT 0x80
// interrupt status bit
#define CR_INT_STATUS_BIT    0x100
// when we support PWM and input capture then define bits 9 - 21


// *****************************************************************
// this orients the TIMERx_REGISTERS struct with the correct addresses
// so referencing a field will now read/write the correct timer
// register chip address
#define TIMER0_REGISTERS  ((FAST_TIMER_REGISTERS_T *) CHIP_MEMORY_MAP_TIMER0_BASE)
#define TIMER1_REGISTERS  ((FAST_TIMER_REGISTERS_T *) CHIP_MEMORY_MAP_TIMER1_BASE)
#define TIMER2_REGISTERS  ((FAST_TIMER_REGISTERS_T *) CHIP_MEMORY_MAP_TIMER2_BASE)
#define TIMER3_REGISTERS  ((SLOW_TIMER_REGISTERS_T *) CHIP_MEMORY_MAP_TIMER3_BASE)
#define TIMER4_REGISTERS  ((SLOW_TIMER_REGISTERS_T *) CHIP_MEMORY_MAP_TIMER4_BASE)


/// ***************************************************************************
/// if the app wants to directly interface with the chip registers, this is a
/// convenience function for getting the address/struct of a particular TIMER
/// so the chip registers can be accessed.
/// ***************************************************************************
FAST_TIMER_REGISTERS_T* tr_hal_fast_timer_get_register_address(tr_hal_timer_id_t timer_id);

SLOW_TIMER_REGISTERS_T* tr_hal_slow_timer_get_register_address(tr_hal_timer_id_t timer_id);


// prototype for callback from the Trident HAL to the app when a timer expires
typedef void (*tr_hal_timer_callback_t) (tr_hal_timer_id_t expired_timer_id);


/// ***************************************************************************
/// Timers settings struct - this is passed to tr_hal_timer_init
///
/// this contains timer settings:
///    timer starting value
///    timer settings
///    callback function
///    current state (passed back on call to tr_hal_timer_read)
///
/// ***************************************************************************
typedef struct
{
    // timer value (counts down from this to 0)
    uint32_t                 timer_start_value;
    tr_hal_timer_prescalar_t prescalar;

    // timer settings
    bool timer_repeats;
    bool timer_enabled;

    // interrupt settings
    bool             interrupt_enabled;
    tr_hal_int_pri_t interrupt_priority;

    // function to call when we expire
    tr_hal_timer_callback_t event_handler_fx;

    // *** info that is passed back on tr_hal_timer_read: ***
    bool     interrupt_is_active;
    uint32_t timer_current_countdown_value;

} tr_hal_timer_settings_t;


/// ***************************************************************************
/// default timer settings
///
/// default is to run every 10 seconds, using interrupts, on repeat, not started
/// timers 0,1,2 are 32 MHz and timers 3,4 are 32 KHz
/// ***************************************************************************
#define DEFAULT_32MHZ_TIMER_CONFIG                \
    {                                             \
        .timer_start_value =  (32000 * 10),       \
        .prescalar = TR_HAL_TIMER_PRESCALER_1024, \
        .timer_repeats = true,                    \
        .timer_enabled = false,                   \
        .interrupt_enabled = true,                \
        .interrupt_priority = TR_HAL_INTERRUPT_PRIORITY_5, \
        .event_handler_fx = NULL,                 \
    }

#define DEFAULT_32KHZ_TIMER_CONFIG                \
    {                                             \
        .timer_start_value =  (32000 * 10),       \
        .prescalar = TR_HAL_TIMER_PRESCALER_1,    \
        .timer_repeats = true,                    \
        .timer_enabled = false,                   \
        .interrupt_enabled = true,                \
        .interrupt_priority = TR_HAL_INTERRUPT_PRIORITY_5, \
        .event_handler_fx = NULL,                 \
    }


/// ****************************************************************************
/// @} // end of tr_hal_T32CZ20
/// ****************************************************************************


#endif // T32CZ20_TIMERS_H_
