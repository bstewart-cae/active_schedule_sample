/// ****************************************************************************
/// @file T32CZ20_wdog.h
///
/// @brief This is the chip specific include file for T32CZ20 Watchdog Driver
///        note that there is a common include file for this HAL module that
///        contains the APIs (such as the init function) that should be used
///        by the application.
///
///
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************

#ifndef T32CZ20_WDOG_H_
#define T32CZ20_WDOG_H_

#include "tr_hal_platform.h"


/// ****************************************************************************
/// @defgroup tr_hal_wdog_cz20 Watchdog CZ20
/// @ingroup tr_hal_T32CZ20
/// @{
/// ****************************************************************************


/// ******************************************************************
/// \brief chip register addresses
/// section 2.2 of the data sheet explains the Memory map.
/// this gives the base address for how to write the chip registers
/// the chip registers are how the software interacts and configures
/// the HAL peripherals. We create a struct below that addresses the
/// individual registers. This makes it so we can use this base address
/// and a struct field to read or write a chip register
/// ******************************************************************
#ifdef WDT_SECURE_EN
    #define CHIP_MEMORY_MAP_WDOG_BASE     (0x50010000UL)
#else
    #define CHIP_MEMORY_MAP_WDOG_BASE     (0x40010000UL)
#endif //WDT_SECURE_EN


/// ***************************************************************************
/// \brief the struct we use so we can address registers using field names
/// ***************************************************************************
typedef struct
{
    // this is the value the watchdog timer starts at, and counts down from
    __IO  uint32_t   initial_value;         // 0x00

    // this is the CURRENT value of the timer
    __IO  uint32_t   current_value;         // 0x04

    // this enables the wdog, interrupt, reset, prescalar and can lock settings
    __IO  uint32_t   control;               // 0x08

    // write 0x0000A5A5 to pet the watchdog and reset the counter to initial value
    __IO  uint32_t   reset_watchdog;        // 0x0C

    // this counts wdog resets up to 255
    __IO  uint32_t   reset_counter;         // 0x10

    // clears active interrupt
    __IO  uint32_t   interrupt_clear;       // 0x14

    // when current value gets to this value the interrupt will fire
    __IO  uint32_t   interrupt_on_value;    // 0x18

    // there needs to be at least this much time passed before the wdog timer can be reset
    __IO  uint32_t   min_time_before_reset; // 0x1C

    // timer clock is divided by this +1 unless this is 0 then it is not used
    __IO  uint32_t   clock_prescale;        // 0x20

} WDOG_REGISTERS_T;


// *****************************************************************
// these defines help when dealing with the INITIAL VALUE (0x00)

// we set the minimum value to be at least 1 second when using a 32 MHz
// clock. note: when checking this the prescalar needs to be taken into
// account
#define TR_HAL_WDOG_MINIMUM_INITIAL_VALUE 32000000


// *****************************************************************
// these defines help when dealing with the CONTROL REGISTER (0x08)

// bit 1 = lockout bit (disables changes when set)
#define TR_HAL_WDOG_CTRL_LOCKOUT            0x01
// bits 2,3,4,5 = reserved

// need to test this and see if it is really the same result as 0x18
#define TR_HAL_WDOG_CTRL_CLK_PRESCALAR_ALSO_4096  0x1C
// bit 6 = interrupt enabled
#define TR_HAL_WDOG_CTRL_INTERRUPT_ENABLED  0x40
#define TR_HAL_WDOG_CTRL_INTERRUPT_DISABLED 0x00
// bit 7 = watchdog enabled
#define TR_HAL_WDOG_CTRL_TIMER_ENABLED      0x80
#define TR_HAL_WDOG_CTRL_TIMER_DISABLED     0x00


// *****************************************************************
// these defines help when dealing with the RESET WATCHDOG REGISTER (0x0C)
#define TR_HAL_WDOG_RESET_WATCHDOG_VALUE 0xA5A5

// *****************************************************************
// these defines help when dealing with the RESET_COUNTER REGISTER (0x10)
#define TR_HAL_WDOG_CLEAR_RESET_COUNTER 0x01

// *****************************************************************
// these defines help when dealing with the INTERRUPT CLEAR REGISTER (0x14)
#define TR_HAL_WDOG_CLEAR_INTERRUPT 0x01

// *****************************************************************
// these defines help when dealing with the MIN TIME BEFORE RESET REGISTER (0x1C)
#define TR_HAL_WDOG_DEFAULT_MIN_TIME_BEFORE_RESET 0

// *****************************************************************
// these defines help when dealing with the CLOCK PRESCALE (0x20)
#define TR_HAL_WDOG_CTRL_CLK_PRESCALAR_1    0
#define TR_HAL_WDOG_CTRL_CLK_PRESCALAR_16   15
#define TR_HAL_WDOG_CTRL_CLK_PRESCALAR_32   31
#define TR_HAL_WDOG_CTRL_CLK_PRESCALAR_128  127
#define TR_HAL_WDOG_CTRL_CLK_PRESCALAR_256  255
#define TR_HAL_WDOG_CTRL_CLK_PRESCALAR_1024 1023
#define TR_HAL_WDOG_CTRL_CLK_PRESCALAR_4096 4095

#define TR_HAL_WDOG_PRESCALAR_MAX_VALUE TR_HAL_WDOG_CTRL_CLK_PRESCALAR_4096

// *****************************************************************
// this orients the WDOG_REGISTERS struct with the correct address
// so referencing a field will now read/write the correct WDOG
// register chip address
#define WDOG_REGISTERS ((WDOG_REGISTERS_T *) CHIP_MEMORY_MAP_WDOG_BASE)


/// *****************************************************************************
/// \brief this enum is used for setting the clock prescalar in the
/// settings struct
/// *****************************************************************************
typedef enum
{
    TR_HAL_WDOG_CLK_PRESCALAR_1    = TR_HAL_WDOG_CTRL_CLK_PRESCALAR_1,
    TR_HAL_WDOG_CLK_PRESCALAR_16   = TR_HAL_WDOG_CTRL_CLK_PRESCALAR_16,
    TR_HAL_WDOG_CLK_PRESCALAR_256  = TR_HAL_WDOG_CTRL_CLK_PRESCALAR_256,
    TR_HAL_WDOG_CLK_PRESCALAR_32   = TR_HAL_WDOG_CTRL_CLK_PRESCALAR_32,
    TR_HAL_WDOG_CLK_PRESCALAR_128  = TR_HAL_WDOG_CTRL_CLK_PRESCALAR_128,
    TR_HAL_WDOG_CLK_PRESCALAR_1024 = TR_HAL_WDOG_CTRL_CLK_PRESCALAR_1024,
    TR_HAL_WDOG_CLK_PRESCALAR_4096 = TR_HAL_WDOG_CTRL_CLK_PRESCALAR_4096,

} tr_hal_wdog_prescalar_t;


/// ***************************************************************************
/// these defines make it easier to setup the wdog for a specific amount of time
/// ***************************************************************************

// value to set for the wdog timer to get one second
// needs to be paired with correct prescalar
#define TR_HAL_WDOG_1_SECOND_TIMER_VALUE 32000

// value to set for the prescalar  to get one second
// needs to be paired with correct timer value
#define TR_HAL_WDOG_1_SECOND_PRESCALAR_VALUE TR_HAL_WDOG_CLK_PRESCALAR_1024


/// ***************************************************************************
/// if the app wants to directly interface with the chip registers, this is a
/// convenience function for getting the address/struct of the WDOG registers
/// ***************************************************************************
WDOG_REGISTERS_T* tr_hal_wdog_get_register_address(void);

/// ***************************************************************************
/// these are the EVENTS that can be received into the WDOG event handler
/// functions. These are BITMASKs since we can have more than 1 in an event
/// these are what the APP needs to handle in its event_handler_fx
/// ***************************************************************************
#define TR_HAL_WDOG_EVENT_INT_TRIGGERED          0x00000001

/// ***************************************************************************
/// prototype for callback from the Trident HAL to the app when an event happens
/// ***************************************************************************
typedef void (*tr_hal_wdog_event_callback_t) (uint32_t event_bitmask);

/// ***************************************************************************
/// watchdog settings struct - this is passed to tr_hal_wdog_init
///
/// ***************************************************************************
typedef struct
{
    // **** basic watchdog settings ****

    // if this is FALSE nothing else matters, watchdog is DISABLED
    bool watchdog_enabled;

    // initial time and clock prescalar - relates to how fast the timer runs down
    tr_hal_wdog_prescalar_t clock_prescalar;
    uint32_t                initial_value;


    // **** advanced watchdog settings ****

    // should we clear reset counter when initialized
    bool clear_reset_counter_on_init;

    // are settings locked
    bool lockout_enabled;

    // can configure a minimum time before a second timer reset will work
    uint32_t min_time_before_reset;


    // **** interrupt settings ****

    // if we want to enable an interrupt we also set the time when tyhe interrupt fires
    // when the watchdog timer gets to this time, the interrupt fires
    bool     interrupt_enabled;
    uint32_t interrupt_time_value;

    // set the INT priority
    tr_hal_int_pri_t interrupt_priority;

    // event callback from HAL to App when the watchdog interrupt
    // if the app doesn't want this, then set it to NULL
    tr_hal_wdog_event_callback_t  event_handler_fx;

    // for read function only: this is the CURRENT value of the countdown timer
    uint32_t current_value;

} tr_hal_wdog_settings_t;


/// ***************************************************************************
/// default watchdog settings
///
/// watchdog ENABLED
/// timer set for 6 seconds
/// keep reset count (don't clear it on init)
/// no lockout, default min time before reset,
/// no interrupts enabled, no event handler function
/// ***************************************************************************
#define DEFAULT_WDOG_CONFIG                                 \
    {                                                       \
        .watchdog_enabled = false,                          \
        .clock_prescalar = TR_HAL_WDOG_1_SECOND_PRESCALAR_VALUE,  \
        .initial_value = (6 * TR_HAL_WDOG_1_SECOND_TIMER_VALUE),  \
        .clear_reset_counter_on_init = false,               \
        .lockout_enabled = false,                           \
        .min_time_before_reset = TR_HAL_WDOG_DEFAULT_MIN_TIME_BEFORE_RESET,\
        .interrupt_enabled = false,                         \
        .interrupt_time_value = 0,                          \
        .interrupt_priority = TR_HAL_INTERRUPT_PRIORITY_5,  \
        .event_handler_fx = NULL,                           \
    }


/// ****************************************************************************
/// @} // end of tr_hal_T32CZ20
/// ****************************************************************************


#endif // T32CZ20_WDOG_H_
