/// ****************************************************************************
/// @file tr_hal_gpio.h
///
/// @brief This is the common include file for the Trident HAL GPIO Driver
///
///
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************

#ifndef TR_HAL_GPIO_H_
#define TR_HAL_GPIO_H_

#include "tr_hal_platform.h"


// ****************************************************************************
// GPIO Driver API functions
// ****************************************************************************

/// ****************************************************************************
/// @defgroup tr_hal_gpio GPIO
/// @ingroup tr_hal_api
/// @{
/// ****************************************************************************


// ***************************************************************************
// ---- init, uninit, read back settings, read input ----
// ***************************************************************************


/// this is the simplest way to setup a GPIO. Create a tr_hal_gpio_settings_t
/// and fill in the fields with the behavior that is desired, then call this
/// gpio_init function. This is done INSTEAD of all the gpio_set functions below
tr_hal_status_t tr_hal_gpio_init(tr_hal_gpio_pin_t       pin,
                                 tr_hal_gpio_settings_t* gpio_settings);

/// this turns off interrupts for this GPIO, sets pull to NONE, sets wake to 
/// NONE, disables debounce, and tells the GPIO Manager that this GPIO can be 
/// used by another HAL module
tr_hal_status_t tr_hal_gpio_uninit(tr_hal_gpio_pin_t pin);

/// this can be used to read the current settings of the GPIO pin,
/// including the output and input state. This returns a tr_hal_gpio_settings_t.
/// The fields that are valid for this are based on the direction field.
tr_hal_status_t tr_hal_gpio_read_settings(tr_hal_gpio_pin_t       pin,
                                          tr_hal_gpio_settings_t* gpio_settings);

/// this function read the state of an input pin. The status return lets the
/// caller know if it worked. The read_value is set if the status is SUCCESS
tr_hal_status_t tr_hal_gpio_read_input(tr_hal_gpio_pin_t pin,
                                       tr_hal_level_t* read_value);


// ***************************************************************************
//  ---- is pin available, are pins equal ----
// ***************************************************************************

/// this functions checks if a pin can be used or not.
/// true = can be used. false = cannot be used.
bool tr_hal_gpio_is_available(tr_hal_gpio_pin_t pin);

/// since a pin is a struct, we cannot compare it directly.
/// need a comparator function.
bool tr_hal_gpio_are_pins_equal(tr_hal_gpio_pin_t pin1,
                                tr_hal_gpio_pin_t pin2);


// ***************************************************************************
// ---- APIs used by HAL modules to ensure GPIOs are only used by 1 module ----
// ***************************************************************************

/// the HAL modules call this when using GPIOs to reserve one
tr_hal_status_t tr_hal_gpio_mgr_reserve_gpio(tr_hal_gpio_pin_t pin,
                                             tr_hal_status_t   new_setting);

/// the HAL modules call this when seeing if a GPIO is available for use
tr_hal_status_t tr_hal_gpio_mgr_check_gpio(tr_hal_gpio_pin_t pin);

/// the HAL modules call this when done with a GPIO, normally in uninit
tr_hal_status_t tr_hal_gpio_mgr_release_gpio(tr_hal_gpio_pin_t pin);


// ***************************************************************************
// ---- setting/getting direction, output state ----
// ***************************************************************************

// do not need these if use the settings struct and the init function

tr_hal_status_t tr_hal_gpio_set_direction(tr_hal_gpio_pin_t pin,
                                          tr_hal_direction_t direction);

tr_hal_status_t tr_hal_gpio_get_direction(tr_hal_gpio_pin_t pin,
                                          tr_hal_direction_t* direction);

tr_hal_status_t tr_hal_gpio_set_output(tr_hal_gpio_pin_t pin,
                                       tr_hal_level_t level);

tr_hal_status_t tr_hal_gpio_get_output(tr_hal_gpio_pin_t pin,
                                       tr_hal_level_t* level);

tr_hal_status_t tr_hal_gpio_toggle_output(tr_hal_gpio_pin_t pin);


// ***************************************************************************
// ---- setting interrupt trigger/priority/callback ----
// ***************************************************************************

// do not need these if use the settings struct and the init function

tr_hal_status_t tr_hal_gpio_set_interrupt_trigger(tr_hal_gpio_pin_t pin,
                                                  tr_hal_trigger_t trigger);

tr_hal_status_t tr_hal_gpio_get_interrupt_trigger(tr_hal_gpio_pin_t pin,
                                                  tr_hal_trigger_t* trigger);

tr_hal_status_t tr_hal_gpio_set_interrupt_priority(tr_hal_int_pri_t new_interrupt_priority);

tr_hal_status_t tr_hal_gpio_get_interrupt_priority(tr_hal_int_pri_t* interrupt_priority);

tr_hal_status_t tr_hal_gpio_set_interrupt_callback(tr_hal_gpio_pin_t            pin,
                                                   tr_hal_gpio_event_callback_t callback_function);

tr_hal_status_t tr_hal_gpio_get_interrupt_callback(tr_hal_gpio_pin_t             pin,
                                                   tr_hal_gpio_event_callback_t* callback_function);


// ***************************************************************************
// ---- setting/getting open drain and pull mode ----
// ***************************************************************************

// do not need these if use the settings struct and the init function

tr_hal_status_t tr_hal_gpio_set_open_drain(tr_hal_gpio_pin_t pin,
                                           bool              enable);

tr_hal_status_t tr_hal_gpio_get_open_drain(tr_hal_gpio_pin_t pin,
                                           bool*             drain_enabled);

tr_hal_status_t tr_hal_gpio_set_pull_mode(tr_hal_gpio_pin_t pin,
                                          tr_hal_pullopt_t mode);

tr_hal_status_t tr_hal_gpio_get_pull_mode(tr_hal_gpio_pin_t pin,
                                          tr_hal_pullopt_t* mode);


// ***************************************************************************
// ---- setting/getting debounce ----
// ***************************************************************************

// do not need these if use the settings struct and the init function

tr_hal_status_t tr_hal_gpio_set_debounce(tr_hal_gpio_pin_t pin,
                                         bool              enable);

tr_hal_status_t tr_hal_gpio_get_debounce(tr_hal_gpio_pin_t pin,
                                         bool*             debounce_enabled);

tr_hal_status_t tr_hal_gpio_set_debounce_time(tr_hal_debounce_time_t new_debounce_time);

tr_hal_status_t tr_hal_gpio_get_debounce_time(tr_hal_debounce_time_t* debounce_time);


// ***************************************************************************
// ---- setting/getting drive strength ----
// ***************************************************************************

// do not need these if use the settings struct and the init function

tr_hal_status_t tr_hal_gpio_set_drive_strength(tr_hal_gpio_pin_t       pin,
                                               tr_hal_drive_strength_t new_drive_strength);

tr_hal_status_t tr_hal_gpio_get_drive_strength(tr_hal_gpio_pin_t        pin,
                                               tr_hal_drive_strength_t* drive_strength);


// ***************************************************************************
// ---- enable/disable wake on deep sleep ----
// ***************************************************************************

// do not need these if use the settings struct and the init function

tr_hal_status_t tr_hal_gpio_set_wake_mode(tr_hal_gpio_pin_t  pin,
                                          tr_hal_wake_mode_t wake_mode);

tr_hal_status_t tr_hal_gpio_get_wake_mode(tr_hal_gpio_pin_t   pin,
                                          tr_hal_wake_mode_t* wake_mode);

// ***************************************************************************
// ---- set/get the pin mode (GPIO, UART, SPI, etc) for this pin ----
// ***************************************************************************
tr_hal_status_t tr_hal_gpio_set_mode(tr_hal_gpio_pin_t pin,
                                     tr_hal_pin_mode_t mode);

tr_hal_status_t tr_hal_gpio_get_mode(tr_hal_gpio_pin_t pin,
                                     tr_hal_pin_mode_t* mode);

// ***************************************************************************
// ---- print a pin in a debug print ----
// ***************************************************************************
char* tr_hal_gpio_get_string(tr_hal_gpio_pin_t pin);


/// ****************************************************************************
/// @} // end of tr_hal_api
/// ****************************************************************************


#endif //TR_HAL_GPIO_H_
