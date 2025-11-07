/// ****************************************************************************
/// @file tr_hal_gpio.c
///
/// @brief This contains the code for the Trident HAL GPIO for T32CM11
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
/// platform specific function for T32CM11 that lets the caller know if the
/// pin is valid to use
/// ****************************************************************************
bool tr_hal_gpio_is_available(tr_hal_gpio_pin_t pin)
{
    uint32_t pin_in_question = pin.pin;

    // must be less than max pins
    if (pin_in_question >= TR_HAL_MAX_PIN_NUMBER)
    {
        return false;
    }

    // skip pins 10-13
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
/// is_pin_set_for_input
///
/// checks if this pin has been set as an input
/// ****************************************************************************
static bool is_pin_set_for_input(tr_hal_gpio_pin_t pin)
{
    // read the input enable register, a value of 1 for the pin bit means this is an input
    uint32_t input_mask = GPIO_CHIP_REGISTERS->input_enable;

    // find the bit for this pin
    uint32_t pin_bit = (1 << pin.pin);

    // if the pin bit is set then this pin is an input
    if ((input_mask & pin_bit) > 0)
    {
        return true;
    }
    // not set means not an input (is an output)
    return false;

}

/// ****************************************************************************
/// is_pin_set_for_output
///
/// checks if this pin has been set as an output
/// ****************************************************************************
//static bool is_pin_set_for_output(tr_hal_gpio_pin_t pin)
//{
//    if (is_pin_set_for_input(pin))
//    {
//        return false;
//    }
//    return true;
//}


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

    // get the wake from deep sleep value
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

            // debounce the pin
            GPIO_CHIP_REGISTERS->enable_debounce = mask;

            result = TR_HAL_SUCCESS;
            break;

        // output
        case TR_HAL_GPIO_DIRECTION_OUTPUT:
            // set pin to output
            GPIO_CHIP_REGISTERS->output_enable = mask;

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
/// in the reference manual this is section 19
/// (I note this since there are OVERLAPS in the address which can be confusing)
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
/// in the reference manual this is section 19
/// (I note this since there are OVERLAPS in the address which can be confusing)

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
        // because of how the disable debounce register works (a read actually
        // gives us what GPIOs are ENABLED), we do NOT need to read/keep the
        // existing state (in fact reading the current state causes a bug, 
        // see GH-553). To disable debounce on one GPIO, we just set the 
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
tr_hal_status_t tr_hal_gpio_set_wake_mode(tr_hal_gpio_pin_t  pin,
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
        // remove this pin from either wake register
        SYS_CTRL_CHIP_REGISTERS->enable_wake_on_high &= pin_bit_inv;
        SYS_CTRL_CHIP_REGISTERS->enable_wake_on_low &= pin_bit_inv;
    }
    else if (wake_mode == TR_HAL_WAKE_MODE_INPUT_LOW)
    {
        // add this pin to LOW reg and remove from HIGH reg
        SYS_CTRL_CHIP_REGISTERS->enable_wake_on_high &= pin_bit_inv;
        SYS_CTRL_CHIP_REGISTERS->enable_wake_on_low |= pin_bit;
    }
    else if (wake_mode == TR_HAL_WAKE_MODE_INPUT_HIGH)
    {
        // add this pin to HIGH reg and remove from LOW reg
        SYS_CTRL_CHIP_REGISTERS->enable_wake_on_high |= pin_bit;
        SYS_CTRL_CHIP_REGISTERS->enable_wake_on_low &= pin_bit_inv;
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
    uint32_t register_value_wake_high = SYS_CTRL_CHIP_REGISTERS->enable_wake_on_high;
    uint32_t register_value_wake_low = SYS_CTRL_CHIP_REGISTERS->enable_wake_on_low;

    // make a bitmask for this pin
    uint32_t pin_bit = (1 << pin.pin);

    // check the registers to determine current state
    if ((pin_bit & register_value_wake_high) > 0)
    {
        (*wake_mode) = TR_HAL_WAKE_MODE_INPUT_HIGH;
    }
    else if ((pin_bit & register_value_wake_low) > 0)
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
/// tr_hal_gpio_handler
///
/// gpio_handler is called from the assembly code in RT58x/Device/ARM/startup_cm3_mcu.s
/// when the GPIO combined interrupt is triggered (any interrupt on a configured GPIO)
/// then gpio_handler is called. There can be only ONE gpio_handler. In order to
/// allow the Rafael hal code to run at the same time as the CMSIS code, I cheated
/// a little and just called this function from gpio_handler
/// ****************************************************************************
void gpio_handler(void)
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
/// tr_hal_gpio_set_mode
///
/// this sets the GPIOx_SEL register. not all pins support all values.
/// see section 19.3.3 - 19.3.6 GPIO_MAP
///
/// value table:
/// 0 GPIO
/// 1 PWM  (data sheet also lists SPI and QSPI0)
/// 2 PWM  (data sheet also lists SPI)
/// 3 PWM  (data sheet also lists SPI / X51_xxx)
/// 4 PWM / I2C / I2S
/// 5 PWM  (data sheet also lists SPI / EXT32K)
/// 6 UART (data sheet also lists SPI)
/// 7 PWM
/// ****************************************************************************
tr_hal_status_t tr_hal_gpio_set_mode(tr_hal_gpio_pin_t pin,
                                     tr_hal_pin_mode_t mode)
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

    // make sure mode is valid
    if (mode > TR_HAL_GPIO_MODE_MAX)
    {
        return TR_HAL_ERROR_INVALID_PARAM;
    }

    // there are 8 pins per register. figure out which register to read from
    register_index = (uint8_t)((pin.pin) >> 3);

    // figure out which bits to read in the register (which bits pertain to this pin)
    pin_offset = (pin.pin) & 0x07;
    pin_mask = 0x0F << (pin_offset * 4);

    // read the register
    pull_register_value = SYS_CTRL_CHIP_REGISTERS->gpio_pin_map[register_index];

    // clear the bits in the register that are relevant for this pin
    pull_register_value = pull_register_value & ~pin_mask;

    // shift the new value up to the right location
    pin_specific_mode = mode << (pin_offset * 4);

    // add the new pin-specifc value to the full register value
    pull_register_value = pull_register_value | pin_specific_mode;

    // set the register value back
    SYS_CTRL_CHIP_REGISTERS->gpio_pin_map[register_index] = pull_register_value;

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
    uint32_t state_on_bitmask = GPIO_CHIP_REGISTERS->state;
    uint32_t pin_bit = (1 << pin.pin);

    if ((state_on_bitmask & pin_bit) == 0)
    {
        (*level) = TR_HAL_GPIO_LEVEL_LOW;
    }
    else
    {
        (*level) = TR_HAL_GPIO_LEVEL_HIGH;
    }

    return TR_HAL_SUCCESS;
}


/// ****************************************************************************
/// tr_hal_gpio_get_pull_mode
///
/// the registers that control the PULL value for each pin are in the SYCTRL
/// registers.
///
/// starting at SYCTRL_BASE+0x20 there are 4-bit sections of data that represent
/// the pull mode for a pin. So for the 32-bit register that lives at 0x20
/// there are 4 pins represented, pins 0-7. the registers at 0x24 represents
/// pins 8-15.
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
    uint32_t mode_register_value = 0;
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
    mode_register_value = SYS_CTRL_CHIP_REGISTERS->gpio_pin_map[register_index];

    // clear all the bits in the register value except those that are relevant for this pin
    mode_register_value = mode_register_value & pin_mask;

    // shift the bits down to the least significant 4 bits
    pin_specific_value = mode_register_value >> (pin_offset * 4);

    // set the return value in the argument
    (*mode) = (tr_hal_pin_mode_t) pin_specific_value;

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
        // this is: pins 10-13, 18-19, 24-27
        if (
            ((pin_num >= 10) && (pin_num <= 13))
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

