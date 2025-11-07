/// ****************************************************************************
/// @file T32CM11_timers.h
///
/// @brief This is the chip specific include file for T32CM11 Timers Driver
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

#ifndef T32CM11_TIMERS_H_
#define T32CM11_TIMERS_H_

#include "tr_hal_platform.h"


/// ****************************************************************************
/// @defgroup tr_hal_timers_cm11 Timers CM11
/// @ingroup tr_hal_T32CM11
/// @{
/// ****************************************************************************


#define TR_HAL_NUM_TIMERS 5

// timer IDs
typedef enum
{
    TIMER_0_ID = 0,
    TIMER_1_ID = 1,
    TIMER_2_ID = 2,
    TIMER_3_ID = 3,
    TIMER_4_ID = 4,

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
/// section 3.1 of the data sheet explains the Memory map.
/// this gives the base address for how to write the chip registers
/// the chip registers are how the software interacts configures GPIOs,
/// reads GPIOs, and gets/sets information on the chip We create a
/// struct below that addresses the individual
/// registers. This makes it so we can use this base address and a
/// struct field to read or write a chip register
/// ******************************************************************
#define CHIP_MEMORY_MAP_TIMER0_BASE     (0xA0700000UL)
#define CHIP_MEMORY_MAP_TIMER1_BASE     (0xA0800000UL)
#define CHIP_MEMORY_MAP_TIMER2_BASE     (0xA0F00000UL)
#define CHIP_MEMORY_MAP_TIMER3_BASE     (0xA0F40000UL)
#define CHIP_MEMORY_MAP_TIMER4_BASE     (0xA0F80000UL)


/// ***************************************************************************
/// the struct we use so we can address registers using field names
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

    // only valid for timer 3 and timer 4 when sleep is enabled
    __IO uint32_t repeat_delay;                         // 0x10

} TIMER_REGISTERS_T;


// *****************************************************************
// these defines help when dealing with the CONTROL REGISTER (0x08)

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


// *****************************************************************
// this orients the TIMERx_REGISTERS struct with the correct addresses
// so referencing a field will now read/write the correct timer
// register chip address
#define TIMER0_REGISTERS  ((TIMER_REGISTERS_T *) CHIP_MEMORY_MAP_TIMER0_BASE)
#define TIMER1_REGISTERS  ((TIMER_REGISTERS_T *) CHIP_MEMORY_MAP_TIMER1_BASE)
#define TIMER2_REGISTERS  ((TIMER_REGISTERS_T *) CHIP_MEMORY_MAP_TIMER2_BASE)
#define TIMER3_REGISTERS  ((TIMER_REGISTERS_T *) CHIP_MEMORY_MAP_TIMER3_BASE)
#define TIMER4_REGISTERS  ((TIMER_REGISTERS_T *) CHIP_MEMORY_MAP_TIMER4_BASE)


/// ***************************************************************************
/// if the app wants to directly interface with the chip registers, this is a
/// convenience function for getting the address/struct of a particular TIMER
/// so the chip registers can be accessed.
/// ***************************************************************************
TIMER_REGISTERS_T* tr_hal_timer_get_register_address(tr_hal_timer_id_t timer_id);


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
/// @} // end of tr_hal_T32CM11
/// ****************************************************************************


#endif // T32CM11_TIMERS_H_
