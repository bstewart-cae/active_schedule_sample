/// ***************************************************************************
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2024 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************

#ifndef APPS_HW_H_
#define APPS_HW_H_

#include <stdint.h>
#include "tr_platform.h"
#include <lpm.h>
#include <ev_man.h>

/**
 * @brief Macro for setting the configuration of a GPIO.
 *
 */
#define GPIO_CONFIG(btn, lpwd, func, short_press, hold, long_press, release_pin) \
  { \
    .gpio_no           = btn, \
    .low_power         = lpwd, \
    .on_value          = btn ## _ON, \
    .short_event       = short_press, \
    .hold_event        = hold, \
    .long_event        = long_press, \
    .release_event     = release_pin, \
  }

/**
 * @brief Macro for setting the info of a GPIO.
 *
 */
#define GPIO_INFO() \
  { \
    .gpio_state        = GPIO_UP, \
    .gpio_down_count   = 0, \
  }

/**
 * @brief Defines the states of a GPIO.
 *
 */
typedef enum _gpio_state_t {
  GPIO_DOWN = 0,  ///< GPIO is down.
  GPIO_UP,        ///< GPIO is up.
  GPIO_HOLD       ///< GPIO is held.
} gpio_state_t;

/**
 * @brief Defines an event type.
 *
 */
typedef uint32_t tr_zw_event_id_t;

/**
 * @brief Defines the configuration of a GPIO.
 *
 */
typedef struct _gpio_config_t
{
  uint8_t gpio_no;                  ///< GPIO number.
  low_power_wakeup_cfg_t low_power; ///< GPIO low power configuration.
  uint8_t on_value;                 ///< Defines "on" as either 1 or 0.
  tr_zw_event_id_t short_event;     ///< The event to trigger on a "short" press.
  tr_zw_event_id_t long_event;      ///< The event to trigger on a "long" press.
  tr_zw_event_id_t hold_event;      ///< The event to trigger when a button is held.
  tr_zw_event_id_t release_event;   ///< The event to trigger when a button is released.
} gpio_config_t;

/**
 * @brief Defines the info of a GPIO.
 *
 */
typedef struct _gpio_info_t
{
  gpio_state_t gpio_state;  ///< GPIO state.
  uint32_t gpio_down_count; ///< GPIO down count.
} gpio_info_t;

/**
 * @brief Initializes the application hardware.
 *
 * @param[in] p_gpio_config Address of GPIO configuration.
 * @param[in] p_gpio_info   Address of GPIO info.
 * @param[in] gpio_size     Number of configured GPIOs.
 */
void apps_hw_init(const gpio_config_t *p_gpio_config,
                  gpio_info_t *p_gpio_info,
                  uint8_t gpio_size);

/**
 * @brief Returns the GPIO used for CC Indicator.
 *
 * @return uint8_t GPIO number.
 */
uint8_t board_indicator_gpio_get(void);

/**
 * @brief Returns the GPIO state that defines "off" for the indicator LED.
 *
 * @return uint8_t Indicator LED "off" state.
 */
uint8_t board_indicator_led_off_gpio_state(void);
#endif /* APPS_HW_H_ */
