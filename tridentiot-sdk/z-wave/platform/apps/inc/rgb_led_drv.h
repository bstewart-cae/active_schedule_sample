/// ***************************************************************************
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2024 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************

/**
 * @file
 * Platform abstraction for Z-Wave Applications
 * Driver for a RGB LED
 */
#ifndef RGB_LED_DRV_H_
#define RGB_LED_DRV_H_
#include <stddef.h>
#include <stdint.h>
#include <pwm.h>

/**
 * @brief Defines the configuration of the RGB LED controller.
 *
 */
typedef struct _rgb_led_config_t
{
  uint32_t const *pPwm_gpio_arry;       ///< Pointer to the gpios used as PWM output*/ 
  uint8_t  const *pPwm_gpio_mode_arry;  ///< Pointer to the gpios modes used as PWM output*/ 
  uint32_t *pPwm_duty_arry;             ///< Pointer to PWM controller channels duty cycle configuration*/
  uint8_t   pwm_ch_count;               ///< The number of the PWM channels used*/ 
} rgb_led_config_t;

/**
 * Initialize the RGB LED driver
 *
 * @param[in] pRgb_led_config The RGB LED configuration.* 
 */
void rgb_led_init(rgb_led_config_t * pRgb_led_config);

/**
 * Update the duty cycle of the RGB Led
 *
 * @param[in] pRgb_led_config The RGB LED configuration.
 * @param[in] color_id The id of the color to change it's duty cycle.
 * @param[in] color_value The duty cycle of the color
 *              the value from 0 to 255 unit (pwm frequncy is 1kHz)
 */
void update_rgb_led(rgb_led_config_t * pRgb_led_config, uint8_t color_id, uint8_t color_value);
#endif /* RGB_LED_DRV_H_ */
