/// ****************************************************************************
/// @file tr_hal_gpio.c
///
/// @brief This contains the code for the Trident HAL GPIO for T32CZ20
///
///
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************

// std library
#include <string.h>
#include <stdio.h>

#include "tr_hal_gpio.h"


// array for keeping track of callback functions for each GPIO
// IF a GPIO is enabled as input then it can be assigned a callback
static tr_hal_gpio_event_callback_t gpio_callback_info[TR_HAL_MAX_PIN_NUMBER];


/// ****************************************************************************
/// tr_hal_gpio_is_available
///
/// platform specific function for T32CZ20 that lets the caller know if the
/// pin is valid to use
///
/// GPIOs 2,3 and 12,13 and 18,19 and 24,25,26,27 are not available for use
/// GPIOs 10,11 are used for programming, so these are not recommended to use
/// ****************************************************************************
bool tr_hal_gpio_is_available(tr_hal_gpio_pin_t pin)
{
    uint32_t pin_in_question = pin.pin;

    // must be less than max pins
    if (pin_in_question >= TR_HAL_MAX_PIN_NUMBER)
    {
        return false;
    }

    // skip pins 2-3
    if ((pin_in_question >= 2) && (pin_in_question <= 3))
    {
        return false;
    }

    // skip pins 10-13
    // GPIOs 10,11 are used for programming, so these are not recommended to use
    if ((pin_in_question >= 10) && (pin_in_question <= 13))
    {
        return false;
    }

    // skip pins 18-19
    if ((pin_in_question >= 18) && (pin_in_question <= 19))
    {
        return false;
    }

    // skip pins 24-27
    if ((pin_in_question >= 24) && (pin_in_question <= 27))
    {
        return false;
    }

    // everything else is ok
    return true;
}



/// ****************************************************************************
/// is_pin_set_for_output
///
/// checks if this pin has been set as an output
/// ****************************************************************************
static bool is_pin_set_for_output(tr_hal_gpio_pin_t pin)
{
    // read the output enable register, a value of 1 for the pin bit means this is an output
    uint32_t output_mask = GPIO_CHIP_REGISTERS->output_enable;

    // find the bit for this pin
    uint32_t pin_bit = (1 << pin.pin);

    // if the pin bit is SET then this pin is an output
    if ((output_mask & pin_bit) > 0)
    {
        return true;
    }
    // not set means not an output (is an input)
    return false;
}

/// ****************************************************************************
/// is_pin_set_for_input
///
/// checks if this pin has been set as an input
/// ****************************************************************************
static bool is_pin_set_for_input(tr_hal_gpio_pin_t pin)
{
    if (is_pin_set_for_output(pin))
    {
        return false;
    }
    return true;
}

/// ****************************************************************************
/// tr_hal_gpio_are_pins_equal
/// ****************************************************************************
bool tr_hal_gpio_are_pins_equal(tr_hal_gpio_pin_t pin1,
                                tr_hal_gpio_pin_t pin2)
{
    if (pin1.pin == pin2.pin)
    {
        return true;
    }
    return false;
}


/// ****************************************************************************
/// tr_hal_gpio_init
///
/// this is the simplest way to setup a GPIO. Create a tr_hal_gpio_settings_t and
/// fill in the fields with the behavior that is desired, then call this gpio_init
/// function. This is done INSTEAD of all the gpio_set functions below
/// ****************************************************************************
tr_hal_status_t tr_hal_gpio_init(tr_hal_gpio_pin_t       pin,
                                 tr_hal_gpio_settings_t* gpio_settings)
{
    tr_hal_status_t status;

    // settings can't be NULL
    if (gpio_settings == NULL)
    {
        return TR_HAL_ERROR_NULL_PARAMS;
    }

    // check if this GPIO is already configured to another peripheral
    status = tr_hal_gpio_mgr_check_gpio(pin);
    if (status != TR_HAL_SUCCESS)
    {
        return status;
    }

    // set the mode to GPIO
    status = tr_hal_gpio_set_mode(pin, TR_HAL_GPIO_MODE_GPIO);
    if (status != TR_HAL_SUCCESS)
    {
        return status;
    }

    // set the direction
    status = tr_hal_gpio_set_direction(pin, gpio_settings->direction);
    if (status != TR_HAL_SUCCESS)
    {
        return status;
    }

    // set output level
    status = tr_hal_gpio_set_output(pin, gpio_settings->output_level);
    if (status != TR_HAL_SUCCESS)
    {
        return status;
    }

    // set open drain
    status = tr_hal_gpio_set_open_drain(pin, gpio_settings->enable_open_drain);
    if (status != TR_HAL_SUCCESS)
    {
        return status;
    }

    // set drive strength
    status = tr_hal_gpio_set_drive_strength(pin, gpio_settings->drive_strength);
    if (status != TR_HAL_SUCCESS)
    {
        return status;
    }

    // set interrupt trigger
    status = tr_hal_gpio_set_interrupt_trigger(pin, gpio_settings->interrupt_trigger);
    if (status != TR_HAL_SUCCESS)
    {
        return status;
    }

    // set interrupt callback
    status = tr_hal_gpio_set_interrupt_callback(pin, gpio_settings->event_handler_fx);
    if (status != TR_HAL_SUCCESS)
    {
        return status;
    }

    // set pull mode
    status = tr_hal_gpio_set_pull_mode(pin, gpio_settings->pull_mode);
    if (status != TR_HAL_SUCCESS)
    {
        return status;
    }

    // debounce enable
    status = tr_hal_gpio_set_debounce(pin, gpio_settings->enable_debounce);
    if (status != TR_HAL_SUCCESS)
    {
        return status;
    }

    // set the wake mode
    status = tr_hal_gpio_set_wake_mode(pin, gpio_settings->wake_mode);
    if (status != TR_HAL_SUCCESS)
    {
        return status;
    }

    // tell the GPIO manager that this GPIO is in use
    tr_hal_gpio_mgr_reserve_gpio(pin, TR_HAL_GPIO_SET_FOR_GPIO);

    // it all worked
    return TR_HAL_SUCCESS;
}


/// ****************************************************************************
/// tr_hal_gpio_uninit
///
/// this turns off interrupts for this GPIO, sets pull to NONE, sets wake to 
/// NONE, disables debounce, and tells the GPIO Manager that this GPIO can be 
/// used by another HAL module. The only status value checked is for
/// tr_hal_gpio_is_available, because if there is an error after that we want 
/// to keep going and put the GPIO in as default of a state as possible
/// ****************************************************************************
tr_hal_status_t tr_hal_gpio_uninit(tr_hal_gpio_pin_t pin)
{
    // make sure pin is valid
    if (!(tr_hal_gpio_is_available(pin)))
    {
        return TR_HAL_ERROR_PIN_NOT_AVAILABLE;
    }

    // release the GPIO used
    tr_hal_gpio_mgr_release_gpio(pin);

    // set pull mode NONE
    tr_hal_gpio_set_pull_mode(pin, TR_HAL_PULLOPT_PULL_NONE);

    // set interrupt trigger OFF
    tr_hal_gpio_set_interrupt_trigger(pin, TR_HAL_GPIO_TRIGGER_NONE);

    // remove any interrupt callback
    tr_hal_gpio_set_interrupt_callback(pin, NULL);

    // set the wake mode to NONE
    tr_hal_gpio_set_wake_mode(pin, TR_HAL_WAKE_MODE_NONE);

    // set debounce to disable
    tr_hal_gpio_set_debounce(pin, false);

    return TR_HAL_SUCCESS;
}


/// ****************************************************************************
/// tr_hal_gpio_read_settings
///
/// this can be used to read the current settings of the GPIO pin, including
/// the output and input state. This returns a tr_hal_gpio_settings_t. The
/// fields that are valid for this are based on the direction field.
/// ****************************************************************************
tr_hal_status_t tr_hal_gpio_read_settings(tr_hal_gpio_pin_t       pin,
                                          tr_hal_gpio_settings_t* gpio_settings)
{
    tr_hal_status_t status;

    // read direction
    status = tr_hal_gpio_get_direction(pin, &(gpio_settings->direction));
    if (status != TR_HAL_SUCCESS)
    {
        return status;
    }

    // get the output value
    status = tr_hal_gpio_get_output(pin, &(gpio_settings->output_level));
    if (status != TR_HAL_SUCCESS)
    {
        return status;
    }

    // get the open drain setting
    status = tr_hal_gpio_get_open_drain(pin, &(gpio_settings->enable_open_drain));
    if (status != TR_HAL_SUCCESS)
    {
        return status;
    }

    // get the drive strength
    status = tr_hal_gpio_get_drive_strength(pin, &(gpio_settings->drive_strength));
    if (status != TR_HAL_SUCCESS)
    {
        return status;
    }

    // get interrupt trigger
    status = tr_hal_gpio_get_interrupt_trigger(pin, &(gpio_settings->interrupt_trigger));
    if (status != TR_HAL_SUCCESS)
    {
        return status;
    }

    // get event handler callback
    status = tr_hal_gpio_get_interrupt_callback(pin, &(gpio_settings->event_handler_fx));
    if (status != TR_HAL_SUCCESS)
    {
        return status;
    }

    // get the pull mode
    status = tr_hal_gpio_get_pull_mode(pin, &(gpio_settings->pull_mode));
    if (status != TR_HAL_SUCCESS)
    {
        return status;
    }

    // get the debounce value
    status = tr_hal_gpio_get_debounce(pin, &(gpio_settings->enable_debounce));
    if (status != TR_HAL_SUCCESS)
    {
        return status;
    }

    // get the wake mode for this GPIO
    status = tr_hal_gpio_get_wake_mode(pin, &(gpio_settings->wake_mode));
    if (status != TR_HAL_SUCCESS)
    {
        return status;
    }

    return TR_HAL_SUCCESS;
}


/// ****************************************************************************
/// tr_hal_gpio_get_string
///
/// given a pin, create a string that represents that pin and return it in buffer
/// ****************************************************************************
char* tr_hal_gpio_get_string(tr_hal_gpio_pin_t pin)
{
    #define RETURN_BUFFER_LENGTH 4
    static char return_buffer[4][RETURN_BUFFER_LENGTH];

    // we can't write to and return the same piece of memory every time
    // if a user makes this call TWICE in one printf call then the 2 calls
    // happen first and then BOTH point to the same memory, which means the
    // first value gets OVERWRITTEN by the second. So we need to return
    // different pieces. We allow up to 4 calls with this before we re-use
    static uint8_t which_buffer = 0;
    which_buffer++;
    if (which_buffer == 4) { which_buffer = 0; }

    // clear the current buffer
    memset(return_buffer[which_buffer], 0, RETURN_BUFFER_LENGTH);

    // write to the buffer
    snprintf(return_buffer[which_buffer], 2, "%li", pin.pin);

    return return_buffer[which_buffer];
}


/// ****************************************************************************
/// tr_hal_gpio_set_direction
///
/// set specified pin as input or output
/// ****************************************************************************
tr_hal_status_t tr_hal_gpio_set_direction(tr_hal_gpio_pin_t pin,
                                          tr_hal_direction_t direction)
{
    tr_hal_status_t result = TR_HAL_SUCCESS;
    uint32_t mask = (1 << pin.pin);

    // make sure pin is valid
    if (!(tr_hal_gpio_is_available(pin)))
    {
        return TR_HAL_ERROR_PIN_NOT_AVAILABLE;
    }

    switch (direction) {

        // input
        case TR_HAL_GPIO_DIRECTION_INPUT:
            // set pin to input
            GPIO_CHIP_REGISTERS->input_enable = mask;
            GPIO_CHIP_REGISTERS->enable_input_mode = mask;

            // debounce the pin
            GPIO_CHIP_REGISTERS->enable_debounce = mask;

            result = TR_HAL_SUCCESS;
            break;

        // output
        case TR_HAL_GPIO_DIRECTION_OUTPUT:
            // set pin to output
            GPIO_CHIP_REGISTERS->output_enable = mask;
            GPIO_CHIP_REGISTERS->disable_input_mode = mask;

            // this is an output so we set it up with NO interrupt
            GPIO_CHIP_REGISTERS->disable_interrupt = mask;

            // when setting a pin to output, we may want to set the pin to TR_HAL_PULLOPT_PULL_NONE
            // if the system is going to SLEEP and if there is a PULL_UP and the pin output is LOW
            // then this pin could cause power leakage.
            // example of the API to use:
            //     tr_hal_gpio_set_pull_mode(pin, TR_HAL_PULLOPT_PULL_NONE);

            result = TR_HAL_SUCCESS;
            break;

        default:
            result = TR_HAL_ERROR_INVALID_DIRECTION;
            break;
    }

  return result;
}


/// ****************************************************************************
/// tr_hal_gpio_set_output
///
/// set specified pin to specified level
/// pin must be valid and set as an output
/// ****************************************************************************
tr_hal_status_t tr_hal_gpio_set_output(tr_hal_gpio_pin_t pin,
                                       tr_hal_level_t level)
{
    // make sure pin is valid
    if (!(tr_hal_gpio_is_available(pin)))
    {
        return TR_HAL_ERROR_PIN_NOT_AVAILABLE;
    }

    // set the level: LOW
    if (level == TR_HAL_GPIO_LEVEL_LOW)
    {
        // clear
        GPIO_CHIP_REGISTERS->set_output_low = (1 << pin.pin);
    }

    // set the level: HIGH
    else if (level == TR_HAL_GPIO_LEVEL_HIGH)
    {
        // set
        GPIO_CHIP_REGISTERS->set_output_high = (1 << pin.pin);
    }

    // error condition
    else
    {
        return TR_HAL_ERROR_INVALID_LEVEL;
    }

    return TR_HAL_SUCCESS;
}

/// ****************************************************************************
/// tr_hal_gpio_toggle_output
///
/// set specified pin to opposite of current level
/// pin must be valid and set as an output
/// ****************************************************************************
tr_hal_status_t tr_hal_gpio_toggle_output(tr_hal_gpio_pin_t pin)
{
    tr_hal_status_t status;
    tr_hal_level_t level;

    // get the output, this checks pin valid, pin dir is output, etc
    status = tr_hal_gpio_get_output(pin, &level);
    if (status != TR_HAL_SUCCESS)
    {
        return status;
    }

    if ( level == TR_HAL_GPIO_LEVEL_LOW )
    {
        status = tr_hal_gpio_set_output(pin, TR_HAL_GPIO_LEVEL_HIGH);
        return status;
    }
    else if ( level == TR_HAL_GPIO_LEVEL_HIGH )
    {
        status = tr_hal_gpio_set_output(pin, TR_HAL_GPIO_LEVEL_LOW);
        return status;
    }

    // error
    return TR_HAL_ERROR_UNKNOWN;
}


/// ****************************************************************************
/// tr_hal_gpio_read_input
///
/// read pin input level
/// pin must be valid and set as an input
/// ****************************************************************************
tr_hal_status_t tr_hal_gpio_read_input(tr_hal_gpio_pin_t pin,
                                       tr_hal_level_t* read_value)
{
    // make sure pin is valid
    if (!(tr_hal_gpio_is_available(pin)))
    {
        return TR_HAL_ERROR_PIN_NOT_AVAILABLE;
    }

    // need to restrict to ONLY INPUT pins
    if (!(is_pin_set_for_input(pin)))
    {
        return TR_HAL_ERROR_PIN_MUST_BE_INPUT;
    }

    // read the input level
    uint32_t register_value = GPIO_CHIP_REGISTERS->state;

    // get the pin bit
    uint32_t pin_bit = 1 << pin.pin;

    // if the bit is set then this is a level high
    if ((register_value & pin_bit) > 0)
    {
        (*read_value) = TR_HAL_GPIO_LEVEL_HIGH;
        return TR_HAL_SUCCESS;
    }

    // bit not set means this is a level low
    (*read_value) = TR_HAL_GPIO_LEVEL_LOW;
    return TR_HAL_SUCCESS;
}


/// ****************************************************************************
/// tr_hal_gpio_set_pull_mode
///
/// setting pull options (pull down/up and how much) are functions of SYSTEM
/// CONTROL chip register and not part of the GPIO chip register
///
/// in the reference manual this is section 17.4.2
/// ****************************************************************************
tr_hal_status_t tr_hal_gpio_set_pull_mode(tr_hal_gpio_pin_t pin,
                                          tr_hal_pullopt_t mode)
{
    uint32_t pull_register_value = 0;
    uint32_t pin_mask = 0;
    uint8_t register_index = 0;
    uint32_t pin_offset = 0;
    uint32_t pin_specific_mode = 0;

    // make sure pin is valid
    if (!(tr_hal_gpio_is_available(pin)))
    {
        return TR_HAL_ERROR_PIN_NOT_AVAILABLE;
    }

    // error check mode
    if (mode > TR_HAL_PULLOPT_MAX_VALUE)
    {
        return TR_HAL_ERROR_INVALID_PULL_MODE;
    }

    // there are 8 pins per register. figure out which register to read from
    register_index = (uint8_t)((pin.pin) >> 3);

    // figure out which bits to read in the register (which bits pertain to this pin)
    pin_offset = (pin.pin) & 0x07;
    pin_mask = 0x0F << (pin_offset * 4);

    // read the register
    pull_register_value = SYS_CTRL_CHIP_REGISTERS->gpio_pull_ctrl[register_index];

    // clear the bits in the register that are relevant for this pin
    pull_register_value = pull_register_value & ~pin_mask;

    // shift the new value up to the right location
    pin_specific_mode = mode << (pin_offset * 4);

    // add the new pin-specifc value to the full register value
    pull_register_value = pull_register_value | pin_specific_mode;

    // set the register value back
    SYS_CTRL_CHIP_REGISTERS->gpio_pull_ctrl[register_index] = pull_register_value;

    return TR_HAL_SUCCESS;
}

/// ****************************************************************************
/// tr_hal_gpio_enable_open_drain
///
/// enable open drain and disable open drain are functions of SYSTEM CONTROL
/// chip register and not part of the GPIO chip register
///
/// in the reference manual this is section 17.4.2
/// ****************************************************************************
tr_hal_status_t tr_hal_gpio_set_open_drain(tr_hal_gpio_pin_t pin,
                                           bool              enable)
{
    // make sure pin is valid
    if (!(tr_hal_gpio_is_available(pin)))
    {
        return TR_HAL_ERROR_PIN_NOT_AVAILABLE;
    }

    // read the open-drain-enable register
    uint32_t open_drain_enable_register_value = SYS_CTRL_CHIP_REGISTERS->open_drain_enable;

    // get the pin bit
    uint32_t pin_bit = 1 << pin.pin;

    if (enable)
    {
        // set the pin bit, leaving the other bits unchanged
        open_drain_enable_register_value = open_drain_enable_register_value | pin_bit;

        // write to the register
        SYS_CTRL_CHIP_REGISTERS->open_drain_enable = open_drain_enable_register_value;
    }
    else
    {
        // we need the inverse of the pin bit
        uint32_t clear_pin_mask = ~pin_bit;

        // clear the pin bit, leaving the other bits unchanged
        open_drain_enable_register_value = open_drain_enable_register_value & clear_pin_mask;
    }

    // write to the register
    SYS_CTRL_CHIP_REGISTERS->open_drain_enable = open_drain_enable_register_value;

    return TR_HAL_SUCCESS;
}


/// ****************************************************************************
/// tr_hal_gpio_set_debounce
///
/// enabled a pin to use debounce
/// ****************************************************************************
tr_hal_status_t tr_hal_gpio_set_debounce(tr_hal_gpio_pin_t pin,
                                         bool              enable)
{
    // make sure pin is valid
    if (!(tr_hal_gpio_is_available(pin)))
    {
        return TR_HAL_ERROR_PIN_NOT_AVAILABLE;
    }

    if (enable)
    {
        // read the enable register
        uint32_t enable_debounce_register = GPIO_CHIP_REGISTERS->enable_debounce;

        // get the pin bit
        uint32_t pin_bit = 1 << pin.pin;

        // set the pin bit, leaving the other bits unchanged
        enable_debounce_register = enable_debounce_register | pin_bit;

        // write to the register
        GPIO_CHIP_REGISTERS->enable_debounce = enable_debounce_register;
    }
    else
    {
        // because of how the disable debounce register works (always read as 
        // zero), we do NOT need to keep the existing state, just set the 
        // 1 bit that represents the GPIO that we want to disable debounce for

        // get the pin bit
        uint32_t pin_bit = 1 << pin.pin;

        // write to the register
        GPIO_CHIP_REGISTERS->disable_debounce = pin_bit;
    }

    return TR_HAL_SUCCESS;
}


/// ****************************************************************************
/// tr_hal_gpio_set_debounce_time
///
/// this sets the debounce time for ALL GPIOs
/// ****************************************************************************
tr_hal_status_t tr_hal_gpio_set_debounce_time(tr_hal_debounce_time_t new_debounce_time)
{
    if (new_debounce_time > TR_HAL_DEBOUNCE_TIME_MAX_VALUE)
    {
        return TR_HAL_ERROR_INVALID_PARAM;
    }

    // write to the register
    GPIO_CHIP_REGISTERS->debounce_time = new_debounce_time;

    return TR_HAL_SUCCESS;
}


/// ****************************************************************************
/// tr_hal_gpio_get_debounce
///
/// ****************************************************************************
tr_hal_status_t tr_hal_gpio_get_debounce(tr_hal_gpio_pin_t pin,
                                         bool*             debounce_enabled)
{
    // make sure pin is valid
    if (!(tr_hal_gpio_is_available(pin)))
    {
        return TR_HAL_ERROR_PIN_NOT_AVAILABLE;
    }

    // read the register
    uint32_t enable_debounce_register = GPIO_CHIP_REGISTERS->enable_debounce;

    // get the pin bit
    uint32_t pin_bit = 1 << pin.pin;

    if ((pin_bit & enable_debounce_register) > 0)
    {
        (*debounce_enabled) = true;
        return TR_HAL_SUCCESS;
    }

    (*debounce_enabled) = false;
    return TR_HAL_SUCCESS;
}


/// ****************************************************************************
/// tr_hal_gpio_get_debounce_time
///
/// ****************************************************************************
tr_hal_status_t tr_hal_gpio_get_debounce_time(tr_hal_debounce_time_t* debounce_time)
{
    (*debounce_time) = GPIO_CHIP_REGISTERS->debounce_time;
    return TR_HAL_SUCCESS;
}


/// ****************************************************************************
/// tr_hal_gpio_set_drive_strength
/// ****************************************************************************
tr_hal_status_t tr_hal_gpio_set_drive_strength(tr_hal_gpio_pin_t       pin,
                                               tr_hal_drive_strength_t new_drive_strength)
{
    uint32_t drive_register_value = 0;
    uint32_t pin_mask = 0;
    uint8_t register_index = 0;
    uint32_t pin_offset = 0;
    uint32_t pin_specific_drive = 0;

    // make sure pin is valid
    if (!(tr_hal_gpio_is_available(pin)))
    {
        return TR_HAL_ERROR_PIN_NOT_AVAILABLE;
    }

    // error check mode
    if (new_drive_strength > TR_HAL_DRIVE_STRENGTH_MAX)
    {
        return TR_HAL_ERROR_INVALID_DRV_STR;
    }

    // there are 16 pins per drive strength register. figure out which register to read from
    register_index = (uint8_t)((pin.pin) >> 4);

    // figure out which bits to read in the register (which bits pertain to this pin)
    pin_offset = (pin.pin) & 0x0F;
    pin_mask = 0x03 << (pin_offset * 2);

    // read the register
    drive_register_value = SYS_CTRL_CHIP_REGISTERS->gpio_drv_ctrl[register_index];

    // clear the bits in the register that are relevant for this pin
    drive_register_value = drive_register_value & ~pin_mask;

    // shift the new value up to the right location
    pin_specific_drive = new_drive_strength << (pin_offset * 2);

    // add the new pin-specifc value to the full register value
    drive_register_value = drive_register_value | pin_specific_drive;

    // set the register value back
    SYS_CTRL_CHIP_REGISTERS->gpio_drv_ctrl[register_index] = drive_register_value;

    return TR_HAL_SUCCESS;
}


/// ****************************************************************************
/// tr_hal_gpio_get_drive_strength
/// ****************************************************************************
tr_hal_status_t tr_hal_gpio_get_drive_strength(tr_hal_gpio_pin_t        pin,
                                               tr_hal_drive_strength_t* drive_strength)
{
    uint32_t drive_register_value = 0;
    uint32_t pin_mask = 0;
    uint8_t register_index = 0;
    uint32_t pin_offset = 0;
    uint32_t pin_specific_value = 0;

    // make sure pin is valid
    if (!(tr_hal_gpio_is_available(pin)))
    {
        return TR_HAL_ERROR_PIN_NOT_AVAILABLE;
    }

    // there are 16 pins per drive strength register. figure out which register to read from
    register_index = (uint8_t)((pin.pin) >> 4);

    // figure out which bits to read in the register
    pin_offset = (pin.pin) & 0x0F;
    pin_mask = 0x03 << (pin_offset * 2);

    // read the register
    drive_register_value = SYS_CTRL_CHIP_REGISTERS->gpio_drv_ctrl[register_index];

    // clear all the bits in the register value except those that are relevant for this pin
    drive_register_value = drive_register_value & pin_mask;

    // shift the bits down to the least significant 4 bits
    pin_specific_value = drive_register_value >> (pin_offset * 2);

    // set the return value in the argument
    (*drive_strength) = (tr_hal_drive_strength_t) pin_specific_value;

    return TR_HAL_SUCCESS;
}


/// ****************************************************************************
/// tr_hal_gpio_set_wake_mode
/// ****************************************************************************
tr_hal_status_t tr_hal_gpio_set_wake_mode(tr_hal_gpio_pin_t       pin,
                                          tr_hal_wake_mode_t wake_mode)
{
    // make sure pin is valid
    if (!(tr_hal_gpio_is_available(pin)))
    {
        return TR_HAL_ERROR_PIN_NOT_AVAILABLE;
    }

    // get the mask with the pin bit set to use for adding
    // and get an inverted mask for removing
    uint32_t pin_bit = (1 << pin.pin);
    uint32_t pin_bit_inv = ~pin_bit;

    if (wake_mode == TR_HAL_WAKE_MODE_NONE)
    {
        // disable this pin to wake from sleep
        GPIO_CHIP_REGISTERS->disable_wake_from_sleep |= pin_bit;

        // disable BOTH high and low
        GPIO_CHIP_REGISTERS->wake_on_low_state &= pin_bit_inv;
        GPIO_CHIP_REGISTERS->wake_on_high_state &= pin_bit_inv;
    }
    else if (wake_mode == TR_HAL_WAKE_MODE_INPUT_LOW)
    {
        // enable this pin to wake from sleep
        GPIO_CHIP_REGISTERS->enable_wake_from_sleep |= pin_bit;

        // add this pin to LOW reg and remove from HIGH reg
        GPIO_CHIP_REGISTERS->wake_on_low_state |= pin_bit;
        GPIO_CHIP_REGISTERS->wake_on_high_state &= pin_bit_inv;
    }
    else if (wake_mode == TR_HAL_WAKE_MODE_INPUT_HIGH)
    {
        // enable this pin to wake from sleep
        GPIO_CHIP_REGISTERS->enable_wake_from_sleep |= pin_bit;

        // add this pin to HIGH reg and remove from LOW reg
        GPIO_CHIP_REGISTERS->wake_on_low_state &= pin_bit_inv;
        GPIO_CHIP_REGISTERS->wake_on_high_state |= pin_bit;
    }

    return TR_HAL_SUCCESS;
}


/// ****************************************************************************
/// tr_hal_gpio_get_wake_mode
/// ****************************************************************************
tr_hal_status_t tr_hal_gpio_get_wake_mode(tr_hal_gpio_pin_t   pin,
                                          tr_hal_wake_mode_t* wake_mode)
{

    // read the relevant registers
    uint32_t register_value_wake_enabled = GPIO_CHIP_REGISTERS->enable_wake_from_sleep;
    uint32_t register_value_wake_low = GPIO_CHIP_REGISTERS->wake_on_low_state;
    uint32_t register_value_wake_high = GPIO_CHIP_REGISTERS->wake_on_high_state;

    // make a bitmask for this pin
    uint32_t pin_bit = (1 << pin.pin);

    // check the registers to determine current state
    if (   ((pin_bit & register_value_wake_enabled) > 0)
        && ((pin_bit & register_value_wake_high) > 0) )
    {
        (*wake_mode) = TR_HAL_WAKE_MODE_INPUT_HIGH;
    }
    else if ( ((pin_bit & register_value_wake_enabled) > 0)
           && ((pin_bit & register_value_wake_low) > 0) )
    {
        (*wake_mode) = TR_HAL_WAKE_MODE_INPUT_LOW;
    }
    else
    {
        (*wake_mode) = TR_HAL_WAKE_MODE_NONE;
    }

    return TR_HAL_SUCCESS;
}


/// ****************************************************************************
/// tr_hal_gpio_set_interrupt_callback
///
/// this enables the interrupt for the passed in pin and it remembers the
/// callback so that it can be called from tr_hal_gpio_handler
///
/// the pin must be valid and must be set as an input
/// ****************************************************************************
tr_hal_status_t tr_hal_gpio_set_interrupt_callback(tr_hal_gpio_pin_t            pin,
                                                   tr_hal_gpio_event_callback_t callback_function)
{
    // make sure pin is valid
    if (!(tr_hal_gpio_is_available(pin)))
    {
        return TR_HAL_ERROR_PIN_NOT_AVAILABLE;
    }

    uint32_t pin_value = pin.pin;

    /// handle the case where the user does NOT want an interrupt on the pin
    if (callback_function == NULL)
    {
        // no callback function
        gpio_callback_info[pin_value] = NULL;

        // the user may still want interrupts on even if there is no callback
        // // disable the interrupt on this pin
        // GPIO_CHIP_REGISTERS->disable_interrupt = (1 << pin_value);

        return TR_HAL_SUCCESS;
    }

    gpio_callback_info[pin_value] = callback_function;

    // enable GPIO combined interrupt
    NVIC_EnableIRQ(Gpio_IRQn);

    GPIO_CHIP_REGISTERS->enable_interrupt = (1 << pin_value);

    return TR_HAL_SUCCESS;
}


/// ****************************************************************************
/// tr_hal_gpio_get_interrupt_callback
/// ****************************************************************************
tr_hal_status_t tr_hal_gpio_get_interrupt_callback(tr_hal_gpio_pin_t             pin,
                                                   tr_hal_gpio_event_callback_t* callback_function)
{
    // copy in the saved callback function
    uint32_t pin_value = pin.pin;
    (*callback_function) = gpio_callback_info[pin_value];

    return TR_HAL_SUCCESS;
}


/// ****************************************************************************
/// tr_hal_gpio_set_interrupt_trigger
///
/// sets when does interrupt trigger: rising, falling, either
///
/// the pin must be valid and must be set as an input
/// ****************************************************************************
tr_hal_status_t tr_hal_gpio_set_interrupt_trigger(tr_hal_gpio_pin_t pin,
                                                  tr_hal_trigger_t trigger)
{
    tr_hal_status_t result = TR_HAL_SUCCESS;

    // make sure pin is valid
    if (!(tr_hal_gpio_is_available(pin)))
    {
        return TR_HAL_ERROR_PIN_NOT_AVAILABLE;
    }

    // get bit representing this pin
    uint32_t mask = (1 << pin.pin);


    // set trigger based on argument passed in
    switch (trigger) {

        case TR_HAL_GPIO_TRIGGER_NONE:
            // disable interrupts
            GPIO_CHIP_REGISTERS->disable_interrupt = mask;
            // disable ANY-EDGE (ANY-EDGE off is the default)
            GPIO_CHIP_REGISTERS->disable_any_edge_trigger_interrupt = mask;
            // disable HIGH by setting to LOW (LOW is the default)
            GPIO_CHIP_REGISTERS->enable_active_low_trigger_interrupt   = mask;
            // disable EDGE by setting to LEVEL (LEVEL is the default)
            GPIO_CHIP_REGISTERS->enable_level_trigger_interrupt = mask;
            break;

        case TR_HAL_GPIO_TRIGGER_RISING_EDGE:
            // enable HIGH, EDGE
            GPIO_CHIP_REGISTERS->enable_active_high_trigger_interrupt  = mask;
            GPIO_CHIP_REGISTERS->enable_edge_trigger_interrupt  = mask;
            // disable ANY-EDGE
            GPIO_CHIP_REGISTERS->disable_any_edge_trigger_interrupt = mask;
            // enable interrupts
            GPIO_CHIP_REGISTERS->enable_interrupt = mask;
            break;

        case TR_HAL_GPIO_TRIGGER_FALLING_EDGE:
            // enable LOW, EDGE
            GPIO_CHIP_REGISTERS->enable_active_low_trigger_interrupt   = mask;
            GPIO_CHIP_REGISTERS->enable_edge_trigger_interrupt  = mask;
            // disable ANY-EDGE
            GPIO_CHIP_REGISTERS->disable_any_edge_trigger_interrupt = mask;
            // enable interrupts
            GPIO_CHIP_REGISTERS->enable_interrupt = mask;
            break;

        case TR_HAL_GPIO_TRIGGER_EITHER_EDGE:
            // enable BOTH (high, low), EDGE
            GPIO_CHIP_REGISTERS->enable_any_edge_trigger_interrupt = mask;
            GPIO_CHIP_REGISTERS->enable_edge_trigger_interrupt  = mask;
            // disable HIGH by setting to LOW
            GPIO_CHIP_REGISTERS->enable_active_low_trigger_interrupt   = mask;
            // enable interrupts
            GPIO_CHIP_REGISTERS->enable_interrupt = mask;
            break;

        case TR_HAL_GPIO_TRIGGER_LEVEL_HIGH:
            // enable HIGH, LEVEL
            GPIO_CHIP_REGISTERS->enable_active_high_trigger_interrupt  = mask;
            GPIO_CHIP_REGISTERS->enable_level_trigger_interrupt = mask;
            // disable ANY-EDGE
            GPIO_CHIP_REGISTERS->disable_any_edge_trigger_interrupt = mask;
            // enable interrupts
            GPIO_CHIP_REGISTERS->enable_interrupt = mask;
            break;

        case TR_HAL_GPIO_TRIGGER_LEVEL_LOW:
            // enable LOW, LEVEL
            GPIO_CHIP_REGISTERS->enable_active_low_trigger_interrupt   = mask;
            GPIO_CHIP_REGISTERS->enable_level_trigger_interrupt = mask;
            // disable ANY-EDGE
            GPIO_CHIP_REGISTERS->disable_any_edge_trigger_interrupt = mask;
            // enable interrupts
            GPIO_CHIP_REGISTERS->enable_interrupt = mask;
            break;

        default:
            result = TR_HAL_ERROR_INVALID_INT_TRIGGER;
            break;
    }

  return result;
}


/// ****************************************************************************
/// tr_hal_gpio_set_interrupt_priority
///
/// to set the interrupt priority use GPIO_SetInterruptPriority API
///
/// sets interrupt priority for all GPIOs
/// ****************************************************************************
tr_hal_status_t tr_hal_gpio_set_interrupt_priority(tr_hal_int_pri_t new_interrupt_priority)
{
    // check the new priority is within bounds
    tr_hal_status_t status = tr_hal_check_interrupt_priority(new_interrupt_priority);
    if (status != TR_HAL_SUCCESS)
    {
        return status;
    }

    NVIC_SetPriority(Gpio_IRQn, new_interrupt_priority);

    return TR_HAL_SUCCESS;
}


/// ****************************************************************************
/// GPIO_Handler
///
/// this is called when the GPIO interrupt is triggered
/// ****************************************************************************
void GPIO_Handler(void)
{
    uint32_t irq_state;
    uint32_t pin = 0;
    uint32_t mask = 1;

    tr_hal_gpio_event_callback_t gpio_callback;

    irq_state = GPIO_CHIP_REGISTERS->interrupt_status;

    for (pin = 0; pin < TR_HAL_MAX_PIN_NUMBER; pin++, mask <<= 1)
    {
       if (irq_state & mask)
        {
            gpio_callback = gpio_callback_info[pin];

            // clear Edgeinterrupt status. if the interrupt source is level
            // triggered, this clear does NOT change it
            GPIO_CHIP_REGISTERS->clear_interrupt = mask;

            // this runs the callback configured for the interrupt
            if (gpio_callback != NULL)
            {
                gpio_callback( (tr_hal_gpio_pin_t){pin}, TR_HAL_GPIO_EVENT_INPUT_TRIGGERED);
            }
        }
    }
}


/// ****************************************************************************
/// check_pin_and_mode_are_valid
///
/// make sure the pin/mode combo passed in is valid
/// 1. make sure pin is available
/// 2. check the special case of pin 11 set for SWDIO (only pin 11 can do this)
/// 3. check if mode is valid OUTPUT mode
/// 4. check if mode is valid INPUT mode
///
/// ****************************************************************************
static tr_hal_status_t check_pin_and_mode_are_valid(tr_hal_gpio_pin_t pin,
                                                    tr_hal_pin_mode_t mode)
{
    // see if it is out of range or one of the unavailable pins
    if (!(tr_hal_gpio_is_available(pin)))
    {
        return TR_HAL_ERROR_PIN_NOT_AVAILABLE;
    }

    uint32_t pin_num = pin.pin;

    // check the special case for pin 11 which is the only pin for SWDIO mode
    if ( (pin_num == 11) && (mode == TR_HAL_GPIO_MODE_SWDIO) )
    {
        return TR_HAL_SUCCESS;
    }

    // ****************
    // make sure the mode is valid. All other modes can be set on all available pins
    // ****************

    // check for valid OUTPUT modes
    if (mode <= TR_HAL_GPIO_OUTPUT_MODE_MAX)
    {
        return TR_HAL_SUCCESS;
    }

    // check for valid INPUT modes
    if ((mode >= TR_HAL_GPIO_INPUT_MODE_MIN) && (mode <= TR_HAL_GPIO_INPUT_MODE_MAX))
    {
        return TR_HAL_SUCCESS;
    }

    // otherwise we have a bad mode
    // QUESTION: should we have a more specific error for this?
    // (or ok since we have a specific error for bad pin and this is the only other param, so it is ok)
    return TR_HAL_ERROR_INVALID_PARAM;
}


/// ****************************************************************************
/// look_for_pin_in_imux
///
/// there are 7 input mux registers. each register holds 4 slots
/// each slot is 8 bits, with high bit being enable and lowest 5 bits being pin number
/// so for each slot: if enabled bit is set then this function is enabled for that pin
///
/// if return status == FALSE then the pin was not found in an enabled slot
/// if return status == TRUE then the return values are set
/// ****************************************************************************
static bool look_for_pin_in_imux(tr_hal_gpio_pin_t pin,
                                 uint32_t*         return_value_reg_index,
                                 uint32_t*         return_value_slot_index)
{
    // note that the 4th register, index 3 is not used
    // for simplicity we check it anyway, because the enable bits will not be set

    #define NUM_IMUX_REG 8
    for (uint8_t imux_index = 0; imux_index < NUM_IMUX_REG; imux_index++)
    {
        uint32_t reg_val = SYS_CTRL_CHIP_REGISTERS->gpio_input_mux[imux_index];

        // we look at all 4 bytes
        for (uint8_t i = 0; i<4; i++)
        {
            // for each byte in the register, check the high bit. if enabled, and
            // the pin# matches then we found the setting
            if ( (reg_val & 0x80) && (pin.pin == (reg_val & 0x1F)) )
            {
                (*return_value_reg_index) = imux_index;
                (*return_value_slot_index) = i;
                return true;
            }
            // slide to the next byte
            reg_val = reg_val >> 8;
        }
    }

    // did not find it
    return false;
}


/// ****************************************************************************
/// get_imux_mode_from_reg_index_and_slot_index
///
/// given the register index and slot index (0-3) this returns which mode
/// from the imux registers
/// ****************************************************************************
static tr_hal_pin_mode_t get_imux_mode_from_reg_index_and_slot_index(uint32_t register_index,
                                                                     uint32_t slot_index)
{
    switch (register_index)
    {
        case 0:
            if (slot_index == 0) { return TR_HAL_GPIO_MODE_UART_1_RX;}
            if (slot_index == 1) { return TR_HAL_GPIO_MODE_UART_1_CTS;}
            if (slot_index == 2) { return TR_HAL_GPIO_MODE_UART_2_RX;}
            if (slot_index == 3) { return TR_HAL_GPIO_MODE_UART_2_CTS;}
            break;
        case 1:
            if (slot_index == 0) { return TR_HAL_GPIO_MODE_UART_0_RX;}
            if (slot_index == 1) { return TR_HAL_GPIO_MODE_I2S_SDI;}
            if (slot_index == 2) { return TR_HAL_GPIO_MODE_I2C_SLAVE_SCL;}
            if (slot_index == 3) { return TR_HAL_GPIO_MODE_I2C_SLAVE_SDA;}
            break;
        case 2:
            if (slot_index == 0) { return TR_HAL_GPIO_MODE_I2C_0_MASTER_SCL;}
            if (slot_index == 1) { return TR_HAL_GPIO_MODE_I2C_0_MASTER_SDA;}
            if (slot_index == 2) { return TR_HAL_GPIO_MODE_I2C_1_MASTER_SCL;}
            if (slot_index == 3) { return TR_HAL_GPIO_MODE_I2C_1_MASTER_SDA;}
            break;
        case 4:
            if (slot_index == 0) { return TR_HAL_GPIO_MODE_SPI_0_PERIPH_CS;}
            if (slot_index == 1) { return TR_HAL_GPIO_MODE_SPI_0_PERIPH_CLK;}
            if (slot_index == 2) { return TR_HAL_GPIO_MODE_SPI_0_PERIPH_SDATA_0;}
            if (slot_index == 3) { return TR_HAL_GPIO_MODE_SPI_0_PERIPH_SDATA_1;}
            break;
        case 5:
            if (slot_index == 0) { return TR_HAL_GPIO_MODE_SPI_0_PERIPH_SDATA_2;}
            if (slot_index == 1) { return TR_HAL_GPIO_MODE_SPI_0_PERIPH_SDATA_3;}
            break;
        case 6:
            if (slot_index == 0) { return TR_HAL_GPIO_MODE_SPI_1_PERIPH_CS;}
            if (slot_index == 1) { return TR_HAL_GPIO_MODE_SPI_1_PERIPH_CLK;}
            if (slot_index == 2) { return TR_HAL_GPIO_MODE_SPI_1_PERIPH_SDATA_0;}
            if (slot_index == 3) { return TR_HAL_GPIO_MODE_SPI_1_PERIPH_SDATA_1;}
            break;
        case 7:
            if (slot_index == 0) { return TR_HAL_GPIO_MODE_SPI_1_PERIPH_SDATA_2;}
            if (slot_index == 1) { return TR_HAL_GPIO_MODE_SPI_1_PERIPH_SDATA_3;}
            break;
    }

    // we should never get here since the function calling this has already
    // found one of these and we just need to map it to a mode. but just in
    // case we haven't thought of everything, the default is GPIO
    return TR_HAL_GPIO_MODE_GPIO;
}


/// ****************************************************************************
/// get_imux_reg_index_from_mode
///
/// for pins that are INPUT, need to set the correct input mux register
/// the register location that is set is based on the MODE and then the pin
/// value is set in that register spot.
///
/// there are 7 input mux registers. each register holds 4 slots
/// each slot is 8 bits, with high bit being enable and lowest 5 bits being pin number
/// so for each slot: if enabled bit is set then this function is enabled for that pin
///
/// see section 17.4.2 and part with GPIO_IMUX (starts at offset 0xA0)
/// ****************************************************************************
static uint32_t get_imux_reg_index_from_mode(tr_hal_pin_mode_t mode)
{
    switch(mode)
    {
        case TR_HAL_GPIO_MODE_UART_2_CTS:
        case TR_HAL_GPIO_MODE_UART_2_RX:
        case TR_HAL_GPIO_MODE_UART_1_CTS:
        case TR_HAL_GPIO_MODE_UART_1_RX:
            return 0;

        case TR_HAL_GPIO_MODE_I2C_SLAVE_SCL:
        case TR_HAL_GPIO_MODE_I2C_SLAVE_SDA:
        case TR_HAL_GPIO_MODE_I2S_SDI:
        case TR_HAL_GPIO_MODE_UART_0_RX:
            return 1;

        case TR_HAL_GPIO_MODE_I2C_1_MASTER_SCL:
        case TR_HAL_GPIO_MODE_I2C_1_MASTER_SDA:
        case TR_HAL_GPIO_MODE_I2C_0_MASTER_SCL:
        case TR_HAL_GPIO_MODE_I2C_0_MASTER_SDA:
            return 2;

        // note register 3 is skipped. this one is currently not used

        case TR_HAL_GPIO_MODE_SPI_0_PERIPH_SDATA_1:
        case TR_HAL_GPIO_MODE_SPI_0_PERIPH_SDATA_0:
        case TR_HAL_GPIO_MODE_SPI_0_PERIPH_CLK:
        case TR_HAL_GPIO_MODE_SPI_0_PERIPH_CS:
        // TODO: check this
        case TR_HAL_GPIO_MODE_SPI_0_SDATA_1:
            return 4;

        case TR_HAL_GPIO_MODE_SPI_0_PERIPH_SDATA_3:
        case TR_HAL_GPIO_MODE_SPI_0_PERIPH_SDATA_2:
            return 5;

        case TR_HAL_GPIO_MODE_SPI_1_PERIPH_SDATA_1:
        case TR_HAL_GPIO_MODE_SPI_1_PERIPH_SDATA_0:
        case TR_HAL_GPIO_MODE_SPI_1_PERIPH_CLK:
        case TR_HAL_GPIO_MODE_SPI_1_PERIPH_CS:
        // TODO: check this
        case TR_HAL_GPIO_MODE_SPI_1_SDATA_1:
            return 6;

        case TR_HAL_GPIO_MODE_SPI_1_PERIPH_SDATA_3:
        case TR_HAL_GPIO_MODE_SPI_1_PERIPH_SDATA_2:
            return 7;

        default:
            return 0;
    }

    return 0;
}


/// ****************************************************************************
/// get_imux_value_from_mode_and_pin
///
/// for pins that are INPUT, need to set the correct input mux register
/// the register location that is set is based on the MODE and then the pin
/// value is set in that register spot.
///
/// see section 17.4.2 and part with GPIO_IMUX (starts at offset 0xA0)
/// ****************************************************************************
static uint32_t get_imux_value_from_mode_and_pin(tr_hal_gpio_pin_t pin,
                                                 tr_hal_pin_mode_t mode,
                                                 uint32_t* imux_clear_mask)
{
    // the register value is the pin in the lower 5 bits and the top bit (most significant bit) set
    // this also gets shifted up for some values, determined in the case stmt below
    uint32_t reg_value = pin.pin | 0x80;
    uint32_t shift_by_bits = 0;

    // determine where in the register this value goes, by determining the shift value
    switch(mode)
    {
        // ***************************************************
        // this is for 1 byte shift
        case TR_HAL_GPIO_MODE_UART_1_CTS:
        case TR_HAL_GPIO_MODE_I2S_SDI:
        case TR_HAL_GPIO_MODE_I2C_0_MASTER_SDA:
        case TR_HAL_GPIO_MODE_SPI_0_PERIPH_CLK:
        case TR_HAL_GPIO_MODE_SPI_0_PERIPH_SDATA_3:
        case TR_HAL_GPIO_MODE_SPI_1_PERIPH_CLK:
        case TR_HAL_GPIO_MODE_SPI_1_PERIPH_SDATA_3:
            shift_by_bits = 8;
            break;

        // ***************************************************
        // this is for 2 byte shift
        case TR_HAL_GPIO_MODE_UART_2_RX:
        case TR_HAL_GPIO_MODE_I2C_SLAVE_SCL:
        case TR_HAL_GPIO_MODE_I2C_1_MASTER_SCL:
        case TR_HAL_GPIO_MODE_SPI_0_PERIPH_SDATA_0:
        case TR_HAL_GPIO_MODE_SPI_1_PERIPH_SDATA_0:
            shift_by_bits = 16;
            break;

        // ***************************************************
        // this is for 3 byte shift
        case TR_HAL_GPIO_MODE_UART_2_CTS:
        case TR_HAL_GPIO_MODE_I2C_SLAVE_SDA:
        case TR_HAL_GPIO_MODE_I2C_1_MASTER_SDA:
        case TR_HAL_GPIO_MODE_SPI_0_PERIPH_SDATA_1:
        case TR_HAL_GPIO_MODE_SPI_1_PERIPH_SDATA_1:
        // TODO: check this
        case TR_HAL_GPIO_MODE_SPI_0_SDATA_1:
        case TR_HAL_GPIO_MODE_SPI_1_SDATA_1:
            shift_by_bits = 24;
            break;

        // ***************************************************
        // this is for no shift (0 shift_by_bits already set)
        // keeping this here to prevent compiler warning
        default:
            shift_by_bits = 0;
            break;
    }

    reg_value = reg_value << shift_by_bits;

    // we also need to return a mask to clear the 1-byte of the 4 byte register
    // before it gets written. We return a mask of all 1s except for the byte
    // we want to erase. The current register value can be &= with this mask
    uint32_t imux_new_mask = 0xFF << shift_by_bits;
    (*imux_clear_mask) = ~(imux_new_mask);

    return reg_value;
}

/// ****************************************************************************
/// tr_hal_gpio_set_mode
///
/// this sets the GPIO mode. Most pins can be mapped to most functions. There
/// are very ferw exceptions. The pins are setup using INPUT MUX registers and
/// OUTPUT MUX registers.
///
/// see section 17.4.2 and the part with GPIO_OMUX (output mux) and GPIO_IMUX
/// (input mux)
///
/// based on what mode, the OMUX or IMUX or BOTH need to be set
///
/// for pins that are OUTPUT, need to set the correct output mux register
/// this is setting the mode value in the right spot in the register based
/// on the pin:
///     OMUX0 = gpio_output_mux[0] = pins 0,1,2,3
///     OMUX1 = gpio_output_mux[1] = pins 4,5,6,7
///     OMUX2 = gpio_output_mux[2] = pins 8,9,10,11
///     OMUX3 = gpio_output_mux[3] = pins 12,13,14,15
///     OMUX4 = gpio_output_mux[4] = pins 16,17,18,19
///     OMUX5 = gpio_output_mux[5] = pins 20,21,22,23
///     OMUX6 = gpio_output_mux[6] = pins 24,25,26,27
///     OMUX7 = gpio_output_mux[7] = pins 28,29,30,31
///     each omux value is an 8-bit value with LSbyte being LSpin
///     for instance: bits   0-7 in OMUX1 is pin 0
///                         8-15 in OMUX1 is pin 1
///                        16-23 in OMUX1 is pin 2
///                        24-31 in OMUX1 is pin 3
///
///
/// for pins that are INPUT, need to set the correct input mux register
/// the register location that is set is based on the MODE and then the pin
/// value is set in that register spot.
/// ****************************************************************************
tr_hal_status_t tr_hal_gpio_set_mode(tr_hal_gpio_pin_t pin,
                                     tr_hal_pin_mode_t mode)
{
    // check that pin and mode choice is valid
    tr_hal_status_t status = check_pin_and_mode_are_valid(pin, mode);
    if (status != TR_HAL_SUCCESS)
    {
        return status;
    }

    // figure out the output mux register index to use and the bit offset
    uint32_t omux_reg = pin.pin >> 2;
    uint32_t omux_bit_offset = (pin.pin % 4) * 8;
    uint32_t omux_value = mode << omux_bit_offset;

    // we need a mask for clearing the part of the omux register that will be written
    uint32_t omux_new_mask = 0xFF << omux_bit_offset;
    uint32_t omux_clear_mask = ~(omux_new_mask);

    // figure out the input mux register index to use and bit offset
    uint32_t imux_clear_mask = 0;
    uint32_t imux_reg = get_imux_reg_index_from_mode(mode);
    uint32_t imux_value = get_imux_value_from_mode_and_pin(pin, mode, &imux_clear_mask);

    // read the current register values
    uint32_t output_mux_register = SYS_CTRL_CHIP_REGISTERS->gpio_output_mux[omux_reg];
    uint32_t input_mux_register = SYS_CTRL_CHIP_REGISTERS->gpio_input_mux[imux_reg];

    // clear out the portion of the register that we will be writing
    output_mux_register &= omux_clear_mask;
    input_mux_register &= imux_clear_mask;


    switch (mode)
    {
        // **************************************
        // modes that only set the output mux
        case TR_HAL_GPIO_MODE_GPIO:
        case TR_HAL_GPIO_MODE_UART_0_TX:
        case TR_HAL_GPIO_MODE_UART_1_TX:
        case TR_HAL_GPIO_MODE_UART_1_RTSN:
        case TR_HAL_GPIO_MODE_UART_2_TX:
        case TR_HAL_GPIO_MODE_UART_2_RTSN:
        case TR_HAL_GPIO_MODE_PWM0:
        case TR_HAL_GPIO_MODE_PWM1:
        case TR_HAL_GPIO_MODE_PWM2:
        case TR_HAL_GPIO_MODE_PWM3:
        case TR_HAL_GPIO_MODE_PWM4:
        case TR_HAL_GPIO_MODE_IRM:
        case TR_HAL_GPIO_MODE_SPI_0_CLK:
        case TR_HAL_GPIO_MODE_SPI_0_SDATA_0:
        case TR_HAL_GPIO_MODE_SPI_0_SDATA_2:
        case TR_HAL_GPIO_MODE_SPI_0_SDATA_3:
        case TR_HAL_GPIO_MODE_SPI_0_CS_0:
        case TR_HAL_GPIO_MODE_SPI_0_CS_1:
        case TR_HAL_GPIO_MODE_SPI_0_CS_2:
        case TR_HAL_GPIO_MODE_SPI_0_CS_3:
        case TR_HAL_GPIO_MODE_SPI_1_CLK:
        case TR_HAL_GPIO_MODE_SPI_1_SDATA_0:
        case TR_HAL_GPIO_MODE_SPI_1_SDATA_2:
        case TR_HAL_GPIO_MODE_SPI_1_SDATA_3:
        case TR_HAL_GPIO_MODE_SPI_1_CS_0:
        case TR_HAL_GPIO_MODE_SPI_1_CS_1:
        case TR_HAL_GPIO_MODE_SPI_1_CS_2:
        case TR_HAL_GPIO_MODE_SPI_1_CS_3:
        case TR_HAL_GPIO_MODE_I2S_BCK:
        case TR_HAL_GPIO_MODE_I2S_WCK:
        case TR_HAL_GPIO_MODE_I2S_SDO:
        case TR_HAL_GPIO_MODE_I2S_MCLK:
        case TR_HAL_GPIO_MODE_DBG0:
        case TR_HAL_GPIO_MODE_DBG1:
        case TR_HAL_GPIO_MODE_DBG2:
        case TR_HAL_GPIO_MODE_DBG3:
        case TR_HAL_GPIO_MODE_DBG4:
        case TR_HAL_GPIO_MODE_DBG5:
        case TR_HAL_GPIO_MODE_DBG6:
        case TR_HAL_GPIO_MODE_DBG7:
        case TR_HAL_GPIO_MODE_DBG8:
        case TR_HAL_GPIO_MODE_DBG9:
        case TR_HAL_GPIO_MODE_DBGA:
        case TR_HAL_GPIO_MODE_DBGB:
        case TR_HAL_GPIO_MODE_DBGC:
        case TR_HAL_GPIO_MODE_DBGD:
        case TR_HAL_GPIO_MODE_DBGE:
        case TR_HAL_GPIO_MODE_DBGF:
            // set the output mux
            output_mux_register |= omux_value;
            SYS_CTRL_CHIP_REGISTERS->gpio_output_mux[omux_reg] = output_mux_register;
            break;

        // **************************************
        // modes that only set BOTH the output mux AND the input mux
        case TR_HAL_GPIO_MODE_I2C_SLAVE_SCL:
        case TR_HAL_GPIO_MODE_I2C_SLAVE_SDA:
        case TR_HAL_GPIO_MODE_I2C_0_MASTER_SCL:
        case TR_HAL_GPIO_MODE_I2C_0_MASTER_SDA:
        case TR_HAL_GPIO_MODE_I2C_1_MASTER_SCL:
        case TR_HAL_GPIO_MODE_I2C_1_MASTER_SDA:
        case TR_HAL_GPIO_MODE_SWDIO:
        // TODO: not sure about these but trying to get SPI to work
        case TR_HAL_GPIO_MODE_SPI_0_SDATA_1:
        case TR_HAL_GPIO_MODE_SPI_1_SDATA_1:
            // set the output mux
            output_mux_register |= omux_value;
            SYS_CTRL_CHIP_REGISTERS->gpio_output_mux[omux_reg] = output_mux_register;

            // set the input mux
            input_mux_register |= imux_value;
            SYS_CTRL_CHIP_REGISTERS->gpio_input_mux[imux_reg] = input_mux_register;
            break;

        // **************************************
        // modes that only set the input mux
        case TR_HAL_GPIO_MODE_UART_2_CTS:
        case TR_HAL_GPIO_MODE_UART_2_RX:
        case TR_HAL_GPIO_MODE_UART_1_CTS:
        case TR_HAL_GPIO_MODE_UART_1_RX:
        case TR_HAL_GPIO_MODE_I2S_SDI:
        case TR_HAL_GPIO_MODE_UART_0_RX:
        case TR_HAL_GPIO_MODE_SPI_0_PERIPH_SDATA_1:
        case TR_HAL_GPIO_MODE_SPI_0_PERIPH_SDATA_0:
        case TR_HAL_GPIO_MODE_SPI_0_PERIPH_CLK:
        case TR_HAL_GPIO_MODE_SPI_0_PERIPH_CS:
        case TR_HAL_GPIO_MODE_SPI_0_PERIPH_SDATA_3:
        case TR_HAL_GPIO_MODE_SPI_0_PERIPH_SDATA_2:
        case TR_HAL_GPIO_MODE_SPI_1_PERIPH_SDATA_1:
        case TR_HAL_GPIO_MODE_SPI_1_PERIPH_SDATA_0:
        case TR_HAL_GPIO_MODE_SPI_1_PERIPH_CLK:
        case TR_HAL_GPIO_MODE_SPI_1_PERIPH_CS:
        case TR_HAL_GPIO_MODE_SPI_1_PERIPH_SDATA_3:
        case TR_HAL_GPIO_MODE_SPI_1_PERIPH_SDATA_2:
            // set the input mux
            input_mux_register |= imux_value;
            SYS_CTRL_CHIP_REGISTERS->gpio_input_mux[imux_reg] = input_mux_register;
            break;
    }

    // NOTE: if a pin gets set as a OUTPUT, it goes into the OMUX
    // if that pin is set to a DIFFERENT function, it does not get UNSET
    // from the OMUX. Wheneber a pin is set, will need to search for this
    // pin in OMUX and, if found, clear that byte of the register

    return TR_HAL_SUCCESS;
}



/// ****************************************************************************
/// tr_hal_gpio_get_direction
/// ****************************************************************************
tr_hal_status_t tr_hal_gpio_get_direction(tr_hal_gpio_pin_t pin,
                                          tr_hal_direction_t* direction)
{
    // make sure pin is valid
    if (!(tr_hal_gpio_is_available(pin)))
    {
        return TR_HAL_ERROR_PIN_NOT_AVAILABLE;
    }

    if (is_pin_set_for_input(pin))
    {
        (*direction) = TR_HAL_GPIO_DIRECTION_INPUT;
    }
    else
    {
        (*direction) = TR_HAL_GPIO_DIRECTION_OUTPUT;
    }

    return TR_HAL_SUCCESS;
}


/// ****************************************************************************
/// tr_hal_gpio_get_output
/// ****************************************************************************
tr_hal_status_t tr_hal_gpio_get_output(tr_hal_gpio_pin_t pin,
                                       tr_hal_level_t* level)
{
    // make sure pin is valid
    if (!(tr_hal_gpio_is_available(pin)))
    {
        return TR_HAL_ERROR_PIN_NOT_AVAILABLE;
    }

    // in this bitmask 1 = pin is logic HIGH, 0 = pin is logic LOW
    uint32_t state_on_bitmask = GPIO_CHIP_REGISTERS->input_enable;
    uint32_t pin_bit = (1 << pin.pin);

    if ((state_on_bitmask & pin_bit) > 0)
    {
        (*level) = TR_HAL_GPIO_LEVEL_HIGH;
    }
    else
    {
        (*level) = TR_HAL_GPIO_LEVEL_LOW;
    }

    return TR_HAL_SUCCESS;
}


/// ****************************************************************************
/// tr_hal_gpio_get_pull_mode
///
/// the registers that control the PULL value for each pin are in the SYCTRL
/// registers.
///
/// starting at SYS_CTRL field gpio_pull_ctrl there are 4-bit sections of
/// data that represent the pull mode for a pin. So for the 32-bit register
/// that lives at gpio_pull_ctrl[0] = 0x20: there are 8 pins represented, pins 0-7.
/// the register at gpio_pull_ctrl[0] = 0x24 represents pins 8-15.
///
///           Byte3         Byte2         Byte1         Byte0
///        FEDC   BA98   7654   3210   FEDC   BA98   7654   3210
/// 0x20:  pin 7  pin 6  pin 5  pin 4  pin 3  pin 2  pin 1  pin 0
/// 0x24:  pin15  pin14  pin13  pin12  pin11  pin10  pin 9  pin 8
/// 0x28:  pin23  pin22  pin21  pin20  pin19  pin18  pin17  pin16
/// 0x2C   pin31  pin30  pin29  pin28  pin27  pin26  pin25  pin24
///
/// the PULL mode is just 3 bits. So each 4 bit section does not use the MSB
///
/// ****************************************************************************
tr_hal_status_t tr_hal_gpio_get_pull_mode(tr_hal_gpio_pin_t pin,
                                          tr_hal_pullopt_t* mode)
{
    uint32_t pull_register_value = 0;
    uint32_t pin_mask = 0;
    uint8_t register_index = 0;
    uint32_t pin_offset = 0;
    uint32_t pin_specific_value = 0;

    // make sure pin is valid
    if (!(tr_hal_gpio_is_available(pin)))
    {
        return TR_HAL_ERROR_PIN_NOT_AVAILABLE;
    }

    // there are 8 pins per register. figure out which register to read from
    register_index = (uint8_t)((pin.pin) >> 3);

    // figure out which bits to read in the register
    pin_offset = (pin.pin) & 0x07;
    pin_mask = 0x0F << (pin_offset * 4);

    // read the register
    pull_register_value = SYS_CTRL_CHIP_REGISTERS->gpio_pull_ctrl[register_index];

    // clear all the bits in the register value except those that are relevant for this pin
    pull_register_value = pull_register_value & pin_mask;

    // shift the bits down to the least significant 4 bits
    pin_specific_value = pull_register_value >> (pin_offset * 4);

    // set the return value in the argument
    (*mode) = (tr_hal_pullopt_t) pin_specific_value;

    return TR_HAL_SUCCESS;
}

/// ****************************************************************************
/// tr_hal_gpio_get_mode
///
/// ****************************************************************************
tr_hal_status_t tr_hal_gpio_get_mode(tr_hal_gpio_pin_t pin,
                                     tr_hal_pin_mode_t* mode)
{
    // make sure pin is valid
    if (!(tr_hal_gpio_is_available(pin)))
    {
        return TR_HAL_ERROR_PIN_NOT_AVAILABLE;
    }

    // see if there is a value set in the output mux for this pin
    uint32_t omux_reg = pin.pin << 2;
    uint32_t omux_mask = 0x0F;
    uint32_t omux_bit_offset = (pin.pin % 4) * 8;

    uint32_t reg_value = SYS_CTRL_CHIP_REGISTERS->gpio_output_mux[omux_reg];
    reg_value = reg_value << omux_bit_offset;
    uint32_t omux_mode = reg_value & omux_mask;

    // see if this pin is set for any of the enabled input mux slots
    uint32_t return_value_reg_index = 0;
    uint32_t return_value_slot_index = 0;
    bool found = look_for_pin_in_imux(pin,
                                      &return_value_reg_index,
                                      &return_value_slot_index);

    // if we found an input match use that
    if (found)
    {
        tr_hal_pin_mode_t imux_mode = get_imux_mode_from_reg_index_and_slot_index(return_value_reg_index,
                                                                                  return_value_slot_index);
        (*mode) = imux_mode;
        return TR_HAL_SUCCESS;
    }

    // otherwise use the output mode, since this will always be set (at least to GPIO)
    (*mode) = omux_mode;
    return TR_HAL_SUCCESS;
}


/// ****************************************************************************
/// tr_hal_gpio_get_open_drain_setting
///
/// read the open drain setting for a particular pin
/// if return status is TR_HAL_SUCCESS then this sets the drain_setting
/// to 1 (enabled) or 0 (disabled)
/// ****************************************************************************
tr_hal_status_t tr_hal_gpio_get_open_drain(tr_hal_gpio_pin_t pin,
                                           bool*             drain_enabled)
{
    // make sure pin is valid
    if (!(tr_hal_gpio_is_available(pin)))
    {
        return TR_HAL_ERROR_PIN_NOT_AVAILABLE;
    }

    uint32_t open_drain_enable_register_value = SYS_CTRL_CHIP_REGISTERS->open_drain_enable;
    uint32_t pin_value = 1 << pin.pin;

    if ((pin_value & open_drain_enable_register_value) > 0)
    {
        (*drain_enabled) = true;
    }
    else
    {
        (*drain_enabled) = false;
    }

    return TR_HAL_SUCCESS;
}

/// ****************************************************************************
/// tr_hal_gpio_get_interrupt_priority
/// ****************************************************************************
tr_hal_status_t tr_hal_gpio_get_interrupt_priority(tr_hal_int_pri_t* interrupt_priority)
{
    (*interrupt_priority) = (tr_hal_int_pri_t) NVIC_GetPriority(Gpio_IRQn);

    return TR_HAL_SUCCESS;

}


/// ****************************************************************************
/// tr_hal_gpio_get_interrupt_trigger
///
/// return the TR_HAL_GPIO_TRIGGER_xxx value that the pin is set to
/// ****************************************************************************
tr_hal_status_t tr_hal_gpio_get_interrupt_trigger(tr_hal_gpio_pin_t pin,
                                                  tr_hal_trigger_t* trigger)
{
    // make sure pin is valid
    if (!(tr_hal_gpio_is_available(pin)))
    {
        return TR_HAL_ERROR_PIN_NOT_AVAILABLE;
    }

    uint32_t pin_value = 1 << pin.pin;

    // check for interrupts being off
    uint32_t enable_interrupts_mask = GPIO_CHIP_REGISTERS->enable_interrupt;
    if ((pin_value & enable_interrupts_mask) == 0)
    {
        (*trigger) = TR_HAL_GPIO_TRIGGER_NONE;
        return TR_HAL_SUCCESS;
    }

    uint32_t trigger_on_edge_mask = GPIO_CHIP_REGISTERS->enable_edge_trigger_interrupt;
    uint32_t trigger_on_level_mask = GPIO_CHIP_REGISTERS->enable_level_trigger_interrupt;
    uint32_t either_edge_trigger_mask  = GPIO_CHIP_REGISTERS->enable_any_edge_trigger_interrupt;

    // high edge and low edge read out the same values
    // value of 1 means high, value of 0 means low
    uint32_t high_edge_trigger_mask = GPIO_CHIP_REGISTERS->enable_active_high_trigger_interrupt;
    uint32_t low_edge_trigger_mask = ~high_edge_trigger_mask;

    // need to check this case FIRST since EITHER overrides LOW or HIGH
    // and if EITHER is set the LOW or HIGH register will ALSO be set
    //
    // EITHER(low or high) and EDGE
    if ( ((pin_value & either_edge_trigger_mask) > 0)
        && ((pin_value & trigger_on_edge_mask) > 0))
    {
        (*trigger) = TR_HAL_GPIO_TRIGGER_EITHER_EDGE;
        return TR_HAL_SUCCESS;
    }

    // HIGH and EDGE
    if ( ((pin_value & high_edge_trigger_mask) > 0)
        && ((pin_value & trigger_on_edge_mask) > 0))
    {
        (*trigger) = TR_HAL_GPIO_TRIGGER_RISING_EDGE;
        return TR_HAL_SUCCESS;
    }

    // LOW and EDGE
    if ( ((pin_value & low_edge_trigger_mask) > 0)
        && ((pin_value & trigger_on_edge_mask) > 0))
    {
        (*trigger) = TR_HAL_GPIO_TRIGGER_FALLING_EDGE;
        return TR_HAL_SUCCESS;
    }


    // LOW and LEVEL
    if ( ((pin_value & low_edge_trigger_mask) > 0)
        && ((pin_value & trigger_on_level_mask) > 0))
    {
        (*trigger) = TR_HAL_GPIO_TRIGGER_LEVEL_LOW;
        return TR_HAL_SUCCESS;
    }

    // HIGH and LEVEL
    if ( ((pin_value & high_edge_trigger_mask) > 0)
        && ((pin_value & trigger_on_level_mask) > 0))
    {
        (*trigger) = TR_HAL_GPIO_TRIGGER_LEVEL_HIGH;
        return TR_HAL_SUCCESS;
    }

    // don't know how this could happen
    return TR_HAL_ERROR_UNKNOWN;
}

/// ****************************************************************************
/// RAM storage for keeping track of what peripheral pins are assigned to
/// ****************************************************************************
uint8_t g_gpio_mgr_curr_pin_settings[TR_HAL_MAX_PIN_NUMBER];
bool    g_gpio_mgr_init_completed = false;

/// ****************************************************************************
/// tr_hal_gpio_mgr_init
///
/// ****************************************************************************
void tr_hal_gpio_mgr_init(void)
{
    uint8_t pin_num;

    // only do this once, but it can be called as many times as we want in order
    // to make sure it happens at least once
    if (g_gpio_mgr_init_completed)
    {
        return;
    }

    // go thru all pins...
    for (pin_num = 0; pin_num < TR_HAL_MAX_PIN_NUMBER; pin_num++)
    {
        // set the unavailable pins to unavailable
        // this is: pins 2-3, 10-13, 18-19, 24-27
        // GPIOs 10,11 are used for programming, so these are not recommended to use
        if (
            ((pin_num >= 2) && (pin_num <= 3))
            || ((pin_num >= 10) && (pin_num <= 13))
            || ((pin_num >= 18) && (pin_num <= 19))
            || ((pin_num >= 24) && (pin_num <= 27))
           )
        {
            g_gpio_mgr_curr_pin_settings[pin_num] = TR_HAL_GPIO_CFG_ERR_UNAVAIL_GPIO;
        }
        // set ALL other pins to available
        else
        {
            g_gpio_mgr_curr_pin_settings[pin_num] = TR_HAL_GPIO_CFG_GPIO_AVAILABLE;
        }
    }

    // we have done the init now, don't do it again
    g_gpio_mgr_init_completed = true;
}

/// ****************************************************************************
/// tr_hal_gpio_mgr_reserve_gpio
///
/// ****************************************************************************
tr_hal_status_t tr_hal_gpio_mgr_reserve_gpio(tr_hal_gpio_pin_t pin,
                                             tr_hal_status_t   new_setting)
{
    // make sure the RAM memory has been initialized
    tr_hal_gpio_mgr_init();

    uint32_t pin_num = pin.pin;

    // make sure the pin num is not out of bounds
    if (pin_num > TR_HAL_MAX_PIN_NUMBER)
    {
        return TR_HAL_ERROR_PIN_NOT_AVAILABLE;
    }

    // make sure the new setting is within range
    if ((new_setting < TR_HAL_GPIO_CFG_ERR_SET_FOR_GPIO)
        || (new_setting > TR_HAL_GPIO_CFG_ERR_UNAVAIL_GPIO))
    {
        return TR_HAL_ERROR_INVALID_PARAM;
    }

    // if that pin num is not free then return an error
    if (g_gpio_mgr_curr_pin_settings[pin_num] != TR_HAL_GPIO_CFG_GPIO_AVAILABLE)
    {
        return (g_gpio_mgr_curr_pin_settings[pin_num]);
    }

    // if that pin is free, then set it to the new setting and return success
    g_gpio_mgr_curr_pin_settings[pin_num] = new_setting;
    return TR_HAL_SUCCESS; 
}

/// ****************************************************************************
/// tr_hal_gpio_mgr_release_gpio
///
/// ****************************************************************************
tr_hal_status_t tr_hal_gpio_mgr_release_gpio(tr_hal_gpio_pin_t pin)
{
    // make sure the RAM memory has been initialized
    tr_hal_gpio_mgr_init();

    uint32_t pin_num = pin.pin;

    // make sure the pin num is not out of bounds
    if (pin_num > TR_HAL_MAX_PIN_NUMBER)
    {
        return TR_HAL_ERROR_PIN_NOT_AVAILABLE;
    }

    // release the pin (even if already released) UNLESS the pin is unavailable
    if (g_gpio_mgr_curr_pin_settings[pin_num] == TR_HAL_GPIO_CFG_ERR_UNAVAIL_GPIO)
    {
        return TR_HAL_ERROR_PIN_NOT_AVAILABLE;
    }
    
    // release the pin
    g_gpio_mgr_curr_pin_settings[pin_num] = TR_HAL_GPIO_CFG_GPIO_AVAILABLE;
    return TR_HAL_SUCCESS; 
}

/// ****************************************************************************
/// tr_hal_gpio_mgr_check_gpio
///
/// ****************************************************************************
tr_hal_status_t tr_hal_gpio_mgr_check_gpio(tr_hal_gpio_pin_t pin)
{
    // make sure the RAM memory has been initialized
    tr_hal_gpio_mgr_init();
    
    uint32_t pin_num = pin.pin;
    
    // make sure the pin num is not out of bounds
    if (pin_num > TR_HAL_MAX_PIN_NUMBER)
    {
        return TR_HAL_ERROR_PIN_NOT_AVAILABLE;
    }

    // the setting is either TR_HAL_SUCCESS (when not used) or the error code
    // for being used, so just return it since we have a valid index
    return (g_gpio_mgr_curr_pin_settings[pin_num]);
}

