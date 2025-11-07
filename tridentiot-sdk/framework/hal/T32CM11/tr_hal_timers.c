/// ****************************************************************************
/// @file tr_hal_timers.c
///
/// @brief This contains the code for the Trident HAL Timers for T32CM11
///
///
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************

#include "tr_hal_timers.h"


// to keep track of the callback functions for each timer
static tr_hal_timer_callback_t g_saved_callbacks[TR_HAL_NUM_TIMERS];


/// ***************************************************************************
/// tr_hal_uart_get_uart_register_address
/// ***************************************************************************
TIMER_REGISTERS_T* tr_hal_timer_get_register_address(tr_hal_timer_id_t timer_id)
{
    if      (timer_id == TIMER_0_ID) { return TIMER0_REGISTERS;}
    else if (timer_id == TIMER_1_ID) { return TIMER1_REGISTERS;}
    else if (timer_id == TIMER_2_ID) { return TIMER2_REGISTERS;}
    else if (timer_id == TIMER_3_ID) { return TIMER3_REGISTERS;}
    else if (timer_id == TIMER_4_ID) { return TIMER4_REGISTERS;}

    return 0;
}


/// ***************************************************************************
/// common interrupt handler for all timers
/// ***************************************************************************
static void common_timer_interrupt_handler(tr_hal_timer_id_t timer_id)
{
    tr_hal_timer_callback_t timer_callback;

    TIMER_REGISTERS_T* timer_reg = tr_hal_timer_get_register_address(timer_id);

    // clear the interrupt
    timer_reg->clear_interrupt = 1;

    timer_callback = g_saved_callbacks[timer_id];

    // make sure it isn't NULL
    if (timer_callback != NULL)
    {
        timer_callback(timer_id);
    }
}


/// ***************************************************************************
/// interrupt functions called when TIMERs expire
/// ***************************************************************************
void Timer0_Handler(void) { common_timer_interrupt_handler(TIMER_0_ID); }
void Timer1_Handler(void) { common_timer_interrupt_handler(TIMER_1_ID); }
void Timer2_Handler(void) { common_timer_interrupt_handler(TIMER_2_ID); }
void Timer3_Handler(void) { common_timer_interrupt_handler(TIMER_3_ID); }
void Timer4_Handler(void) { common_timer_interrupt_handler(TIMER_4_ID); }


/// ***************************************************************************
/// static utility function: set_timer_interrupt_priority
///
/// we don't error check the timer ID or interrupt priority since this is
/// called only from functions that have already error checked the timer ID
/// and the interrupt priority
/// ***************************************************************************
static void set_timer_interrupt_priority(tr_hal_timer_id_t timer_id,
                                         tr_hal_int_pri_t new_priority)
{
    if (timer_id == TIMER_0_ID)
    {
        NVIC_EnableIRQ(Timer0_IRQn);
        NVIC_SetPriority(Timer0_IRQn, new_priority);
    }
    else if (timer_id == TIMER_1_ID)
    {
        NVIC_EnableIRQ(Timer1_IRQn);
        NVIC_SetPriority(Timer1_IRQn, new_priority);
    }
    else if (timer_id == TIMER_2_ID)
    {
        NVIC_EnableIRQ(Timer2_IRQn);
        NVIC_SetPriority(Timer2_IRQn, new_priority);
    }
    else if (timer_id == TIMER_3_ID)
    {
        NVIC_EnableIRQ(Timer3_IRQn);
        NVIC_SetPriority(Timer3_IRQn, new_priority);
    }
    else if (timer_id == TIMER_4_ID)
    {
        NVIC_EnableIRQ(Timer4_IRQn);
        NVIC_SetPriority(Timer4_IRQn, new_priority);
    }
}


/// ***************************************************************************
/// static utility function: disable_timer_interrupt
///
/// we don't error check the timer ID since this is called only from functions
/// that have already error checked the timer ID
/// ***************************************************************************
static void disable_timer_interrupt(tr_hal_timer_id_t timer_id)
{
    if (timer_id == TIMER_0_ID)
    {
        NVIC_DisableIRQ(Timer0_IRQn);
    }
    else if (timer_id == TIMER_1_ID)
    {
        NVIC_DisableIRQ(Timer1_IRQn);
    }
    else if (timer_id == TIMER_2_ID)
    {
        NVIC_DisableIRQ(Timer2_IRQn);
    }
    else if (timer_id == TIMER_3_ID)
    {
        NVIC_DisableIRQ(Timer3_IRQn);
    }
    else if (timer_id == TIMER_4_ID)
    {
        NVIC_DisableIRQ(Timer4_IRQn);
    }
}


/// ***************************************************************************
/// static utility function: get_timer_interrupt_priority
///
/// we don't error check the timer ID since this is called only from functions
/// that have already error checked the timer ID
/// ***************************************************************************
static tr_hal_int_pri_t get_timer_interrupt_priority(tr_hal_timer_id_t timer_id)
{
    if (timer_id == TIMER_0_ID)
    {
        return NVIC_GetPriority(Timer0_IRQn);
    }
    else if (timer_id == TIMER_1_ID)
    {
        return NVIC_GetPriority(Timer1_IRQn);
    }
    else if (timer_id == TIMER_2_ID)
    {
        return NVIC_GetPriority(Timer2_IRQn);
    }
    else if (timer_id == TIMER_3_ID)
    {
        return NVIC_GetPriority(Timer3_IRQn);
    }
    else if (timer_id == TIMER_4_ID)
    {
        return NVIC_GetPriority(Timer4_IRQn);
    }

    // should not happen
    return 0;
}


/// ***************************************************************************
/// init timer
///
/// this sets up a timer based on the fields in the timer settings struct
/// ***************************************************************************
tr_hal_status_t tr_hal_timer_init(tr_hal_timer_id_t        timer_id,
                                  tr_hal_timer_settings_t* timer_settings)
{
    // settings can't be NULL
    if (timer_settings == NULL)
    {
        return TR_HAL_ERROR_NULL_PARAMS;
    }

    // error check the timer ID
    if (timer_id >= TR_HAL_NUM_TIMERS)
    {
        return TR_HAL_INVALID_TIMER_ID;
    }

    // callback can't be NULL - we do not allow NULL, since
    // it would be difficult to know when a timer expired
    if (timer_settings->event_handler_fx == NULL)
    {
        return TR_HAL_TIMER_CALLBACK_NULL;
    }

    // error check that the interrupt priority is within range
    tr_hal_status_t status = tr_hal_check_interrupt_priority(timer_settings->interrupt_priority);
    if (status != TR_HAL_SUCCESS)
    {
        return status;
    }

    TIMER_REGISTERS_T* timer_register = tr_hal_timer_get_register_address(timer_id);

    // write the start value
    timer_register->initial_value = timer_settings->timer_start_value;

    // create the control register value by starting with the prescalar
    uint32_t control_reg_value = timer_settings->prescalar;

    // enable interrupts
    if (timer_settings->interrupt_enabled == true)
    {
        control_reg_value = control_reg_value | CR_INT_ENABLE_BIT;
        set_timer_interrupt_priority(timer_id, timer_settings->interrupt_priority);
    }
    // when the interrupt is NOT enabled, make sure we disable it. If the app has
    // a timer with interrupts enabled and then wants to DISABLE the interrupts
    // for that timer, this is the only way to do it (we don't have an uninit
    // timer function)
    else
    {
        disable_timer_interrupt(timer_id);
    }

    // enable mode periodic or free running
    if (timer_settings->timer_repeats)
    {
        control_reg_value = control_reg_value | CR_MODE_BIT;
    }

    // enable timer to start
    if (timer_settings->timer_enabled)
    {
        control_reg_value = control_reg_value | CR_TIMER_RUNNING_BIT;
    }

    // write the control register
    timer_register->control = control_reg_value;

    // save the callback - we check for NULL above
    g_saved_callbacks[timer_id] = timer_settings->event_handler_fx;


    return TR_HAL_SUCCESS;
}



/// ***************************************************************************
/// read timer info
///
/// loads the timer info of the specified timer into the timer_settings passed in
/// ***************************************************************************
tr_hal_status_t tr_hal_timer_read(tr_hal_timer_id_t        timer_id,
                                  tr_hal_timer_settings_t* timer_settings)
{
    // error check the timer ID
    if (timer_id >= TR_HAL_NUM_TIMERS)
    {
        return TR_HAL_INVALID_TIMER_ID;
    }

    // timer settings can't be NULL, this is what we fill in
    if (timer_settings == NULL)
    {
        return TR_HAL_TIMER_NULL_SETTINGS;
    }

    // get a ptr to the timer register struct
    // no need to error check since it was already checked above that the timer is within range
    TIMER_REGISTERS_T* timer_register = tr_hal_timer_get_register_address(timer_id);

    // read the start value
    timer_settings->timer_start_value = timer_register->initial_value;

    // read the control register
    uint32_t control_reg_value = timer_register->control;

    // read prescalar part of control register
    timer_settings->prescalar = (control_reg_value & CR_PRESCALER_MASK);

    // read interrupt enable bit part of control register
    if ((control_reg_value & CR_INT_ENABLE_BIT) > 1)
    {
        // if interrupts are enabled, read the interrupt priority
        timer_settings->interrupt_enabled = true;
        timer_settings->interrupt_priority = get_timer_interrupt_priority(timer_id);
    }
    else
    {
        timer_settings->interrupt_enabled = false;
    }

    // read mode bit part of control register
    if ((control_reg_value & CR_MODE_BIT) > 1) { timer_settings->timer_repeats = true; }
    else                                       { timer_settings->timer_repeats = false; }

    // read timer active/running bit part of control register
    if ((control_reg_value & CR_TIMER_RUNNING_BIT) > 1) { timer_settings->timer_enabled = true; }
    else                                                { timer_settings->timer_enabled = false; }

    // this returns if the timer interrupt bit (meaning it expired) is active
    if ((control_reg_value & CR_INT_STATUS_BIT) > 1) { timer_settings->interrupt_is_active = true; }
    else                                             { timer_settings->interrupt_is_active = false; }

    // read the current countdown value
    timer_settings->timer_current_countdown_value = timer_register->current_countdown_value;

    // get the callback
    timer_settings->event_handler_fx = g_saved_callbacks[timer_id];

    return TR_HAL_SUCCESS;
}


/// ***************************************************************************
/// start timer
///
/// calling this on a timer that is already started will have no affect
/// ***************************************************************************
tr_hal_status_t tr_hal_timer_start(tr_hal_timer_id_t timer_id)
{
    // error check the timer ID
    if (timer_id >= TR_HAL_NUM_TIMERS)
    {
        return TR_HAL_INVALID_TIMER_ID;
    }

    TIMER_REGISTERS_T* timer_register = tr_hal_timer_get_register_address(timer_id);

    // read the current control value so it can be set again
    uint32_t control_reg_value = timer_register->control;

    // add in the running bit of 1
    control_reg_value = control_reg_value | CR_TIMER_RUNNING_BIT;

    // write the control register back
    timer_register->control = control_reg_value;

    return TR_HAL_SUCCESS;
}


/// ***************************************************************************
/// stop timer
///
/// calling this on a timer that is already stopped will have no affect
/// ***************************************************************************
tr_hal_status_t tr_hal_timer_stop(tr_hal_timer_id_t timer_id)
{
    // error check the timer ID
    if (timer_id >= TR_HAL_NUM_TIMERS)
    {
        return TR_HAL_INVALID_TIMER_ID;
    }

    TIMER_REGISTERS_T* timer_register = tr_hal_timer_get_register_address(timer_id);

    // read the current control value so it can be set again
    uint32_t control_reg_value = timer_register->control;

    uint32_t mask = (uint32_t)(~CR_TIMER_RUNNING_BIT);

    // clear the running bit to 0
    control_reg_value = control_reg_value & mask;

    // write the control register back
    timer_register->control = control_reg_value;

    return TR_HAL_SUCCESS;
}


/// ***************************************************************************
/// restart
///
/// this does a tr_hal_timer_stop and then tr_hal_timer_start
/// returns an error if either fail but no additional intelligence
/// if this is called when a timer is ALREADY STOPPED, then the timer will be STARTED
/// ***************************************************************************
tr_hal_status_t tr_hal_timer_restart(tr_hal_timer_id_t timer_id)
{
    tr_hal_status_t status;

    // error check the timer ID
    if (timer_id >= TR_HAL_NUM_TIMERS)
    {
        return TR_HAL_INVALID_TIMER_ID;
    }

    status = tr_hal_timer_stop(timer_id);
    if (status != TR_HAL_SUCCESS)
    {
        return status;
    }

    status = tr_hal_timer_start(timer_id);

    return status;
}


/// ***************************************************************************
/// is timer running?
///
/// this sets the boolean to true if the register enable flag is set and false
// if it is not set. This is the timer running bit, AKA the enable bit
/// ***************************************************************************
tr_hal_status_t tr_hal_timer_is_running(tr_hal_timer_id_t timer_id,
                                        bool* is_running)
{
    // error check the timer ID
    if (timer_id >= TR_HAL_NUM_TIMERS)
    {
        return TR_HAL_INVALID_TIMER_ID;
    }

    TIMER_REGISTERS_T* timer_register = tr_hal_timer_get_register_address(timer_id);

    // read the current control value so it can be set again
    uint32_t control_reg_value = timer_register->control;

    // if the timer running bit (enable bit) is set then running=true
    if ( (control_reg_value & CR_TIMER_RUNNING_BIT) > 0 )
    {
        (*is_running) = true;
        return TR_HAL_SUCCESS;
    }

    // else running is false
    (*is_running) = false;
    return TR_HAL_SUCCESS;
}


/// ***************************************************************************
/// read the current countdown value of an active timer
///
/// if a timer is NOT running, the timer_enabled arg will be set to FALSE
/// if a timer is RUNNING, then timer_enabled arg will be TRUE and prescalar and
///       current_countdown_value args will be set to their current register values
/// ***************************************************************************
tr_hal_status_t tr_hal_read_active_timer_state(tr_hal_timer_id_t         timer_id,
                                               bool*                     timer_enabled,
                                               tr_hal_timer_prescalar_t* prescalar,
                                               uint32_t*                 current_countdown_value)
{
    // error check the timer ID
    if (timer_id >= TR_HAL_NUM_TIMERS)
    {
        return TR_HAL_INVALID_TIMER_ID;
    }

    // make sure none of the return arguments are NULL
    if ( (timer_enabled == NULL)
        || (prescalar == NULL)
        || (current_countdown_value == NULL) )
    {
        return TR_HAL_ERROR_NULL_PARAMS;
    }

    // get a ptr to the timer register struct
    // no need to error check since it was already checked above that the timer is within range
    TIMER_REGISTERS_T* timer_register = tr_hal_timer_get_register_address(timer_id);

    // read the control register
    uint32_t control_reg_value = timer_register->control;

    // is the timer active/running?
    if ((control_reg_value & CR_TIMER_RUNNING_BIT) == 0)
    {
        // not running - set enabled to false and return
        (*timer_enabled) = false;
        return TR_HAL_SUCCESS;
    }

    // timer IS running
    (*timer_enabled) = true;

    // read prescalar part of control register
    (*prescalar) = (control_reg_value & CR_PRESCALER_MASK);

    (*current_countdown_value) = timer_register->current_countdown_value;

    return TR_HAL_SUCCESS;
}


/// ***************************************************************************
/// change period
///
/// if a timer is RUNNING, this changes the period but does NOT restart it
/// if a timer if NOT RUNNING, this does NOT start the timer
/// ***************************************************************************
tr_hal_status_t tr_hal_timer_change_period(tr_hal_timer_id_t        timer_id,
                                           uint32_t                 timer_start_value,
                                           tr_hal_timer_prescalar_t prescalar)
{
    // error check the timer ID
    if (timer_id >= TR_HAL_NUM_TIMERS)
    {
        return TR_HAL_INVALID_TIMER_ID;
    }

    // error check the prescalar value
    if (prescalar > TR_HAL_TIMER_PRESCALER_MAX)
    {
        return TR_HAL_ERROR_BAD_PRESCALAR;
    }

    TIMER_REGISTERS_T* timer_register = tr_hal_timer_get_register_address(timer_id);

    // write the start value
    timer_register->initial_value = timer_start_value;

    // read the current control value so it can be set again
    uint32_t control_reg_value = timer_register->control;
    uint32_t inv_mask = CR_PRESCALER_MASK;
    uint32_t mask = ~inv_mask;

    // mask out the prescalar
    control_reg_value = control_reg_value & mask;

    // add the prescalar
    control_reg_value = control_reg_value | prescalar;

    // write the control register back
    timer_register->control = control_reg_value;

    return TR_HAL_SUCCESS;
}


/// ***************************************************************************
/// tr_hal_timer_set_callback_function
/// ***************************************************************************
tr_hal_status_t tr_hal_timer_set_callback_function(tr_hal_timer_id_t       timer_id,
                                                   tr_hal_timer_callback_t callback_function)
{
    // error check the timer ID
    if (timer_id >= TR_HAL_NUM_TIMERS)
    {
        return TR_HAL_INVALID_TIMER_ID;
    }

    // callback can't be NULL
    if (callback_function == NULL)
    {
        return TR_HAL_TIMER_CALLBACK_NULL;
    }

    // we need interrupts to be enabled to use the callback and we need a
    // valid interrupt priority to be set. Read the int pri for this timer
    // if it is valid then leave it alone, if not valid then set it to
    // a valid value
    tr_hal_int_pri_t priority = get_timer_interrupt_priority(timer_id);
    if (tr_hal_check_interrupt_priority(priority) != TR_HAL_SUCCESS)
    {
        priority = TR_HAL_INTERRUPT_PRIORITY_5;
    }

    // this makes sure interrupts are enabled for this timer and sets the int priority
    set_timer_interrupt_priority(timer_id,
                                 priority);

    // save the callback
    g_saved_callbacks[timer_id] = callback_function;

    return TR_HAL_SUCCESS;
}

