/// ****************************************************************************
/// @file LEDBulb_hw.c
///
/// @brief LEDBulb Hardware setup for the DKNCZ20 board with UART on USB
///
///
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************

#include <stddef.h>
#include <stdint.h>
#include <inttypes.h>
#include <apps_hw.h>
#include <sysctrl.h>
#include <CC_MultilevelSwitch_Support.h>
#include "CC_ColorSwitch.h"
#include "SizeOf.h"
#include "events.h"
#ifdef TR_CLI_ENABLED
#include "tr_cli.h"
#endif
#include <zaf_event_distributor_soc.h>
#include <ZAF_Actuator.h>
#include "zpal_uart.h"
#include "zpal_misc.h"
#include "zpal_misc_private.h"
#include "setup_common_cli.h"
#include "tr_board_DKNCZ20.h"
#include "rgb_led_drv.h"
#include "tr_hal_gpio.h"

static const uint8_t PB_LEARN_MODE             = (uint8_t)TR_BOARD_BTN_LEARN_MODE;

/*Defines if the gpio support low power mode or nots*/
static const low_power_wakeup_cfg_t PB_LEARN_MODE_LP             = LOW_POWER_WAKEUP_GPIO4;

/*Defines the on state for the gpios*/
static const uint8_t PB_LEARN_MODE_ON             = 0;

static gpio_info_t m_gpio_info[] = {
  GPIO_INFO()
};

static const gpio_config_t m_gpio_config[] = {
  GPIO_CONFIG(PB_LEARN_MODE, PB_LEARN_MODE_LP, PB_LEARN_MODE_ON, EVENT_SYSTEM_LEARNMODE_TOGGLE, EVENT_SYSTEM_EMPTY, EVENT_SYSTEM_RESET, EVENT_SYSTEM_EMPTY)
};

#define BUTTONS_SIZE    sizeof_array(m_gpio_config)

/*Setup uart for debug and remote CLI uart*/

static zpal_uart_config_t UART_CONFIG =
{
  .id = ZPAL_UART0,
  .baud_rate = 115200,
  .data_bits = 8,
  .parity_bit = ZPAL_UART_NO_PARITY,
  .stop_bits = ZPAL_UART_STOP_BITS_1,
  .receive_callback = NULL,
  .ptr = NULL,
  .flags = 0
};

zpal_debug_config_t debug_port_cfg = &UART_CONFIG;

#ifdef TR_CLI_ENABLED

// Application specific CLI commands
TR_CLI_COMMAND_TABLE(app_specific_commands) =
{
  TR_CLI_COMMAND_TABLE_END
};

#endif // #ifdef TR_CLI_ENABLED

static uint8_t color_switch_value[3];
static uint32_t pwm_duty_array[3];
static const uint32_t pwm_gpio[] = {TR_BOARD_LED_RED, TR_BOARD_LED_BLUE, TR_BOARD_LED_GREEN};
static const uint8_t  pwm_gpio_mode[] = {TR_HAL_GPIO_MODE_PWM0, TR_HAL_GPIO_MODE_PWM1, TR_HAL_GPIO_MODE_PWM2};
static rgb_led_config_t rgb_led_config = {.pPwm_duty_arry =pwm_duty_array,
                                          .pPwm_gpio_arry = pwm_gpio,
                                          .pPwm_gpio_mode_arry = pwm_gpio_mode,
                                          .pwm_ch_count = 3};
void zwsdk_app_hw_init(void)
{
  rgb_led_init(&rgb_led_config);
}

void app_hw_init(void)
{
  apps_hw_init(m_gpio_config, m_gpio_info, BUTTONS_SIZE);
#ifdef TR_CLI_ENABLED
  setup_cli(&UART_CONFIG);
#endif
}

uint8_t board_indicator_gpio_get( void)
{
  return (uint8_t)TR_BOARD_LED_LEARN_MODE;
}

// Return the state of the GPIO pin for turning off the LED
uint8_t board_indicator_led_off_gpio_state(void)
{
  return TR_HAL_GPIO_LEVEL_HIGH;
}

void cc_color_switch_cb(s_colorComponent * colorComponent)
{
  switch (colorComponent->colorId) {
    case ECOLORCOMPONENT_RED:
      color_switch_value[0] = ZAF_Actuator_GetCurrentValue(&colorComponent->obj);
      update_rgb_led(&rgb_led_config, 0, color_switch_value[0]);
      break;

    case ECOLORCOMPONENT_GREEN:
      color_switch_value[2] = ZAF_Actuator_GetCurrentValue(&colorComponent->obj);
      update_rgb_led(&rgb_led_config, 2, color_switch_value[2]);
      break;

    case ECOLORCOMPONENT_BLUE:
      color_switch_value[1] = ZAF_Actuator_GetCurrentValue(&colorComponent->obj);
      update_rgb_led(&rgb_led_config, 1, color_switch_value[1]);
      break;

    default:
      break;
  }
}

void cc_multilevel_switch_support_cb(cc_multilevel_switch_t * p_switch)
{
  uint16_t multilevel_switch_value = cc_multilevel_switch_get_current_value(p_switch);
  for (pwm_id_t pwm_id = PWM_ID_0; pwm_id <= PWM_ID_2; pwm_id++)
  {
    uint8_t color_value = (uint8_t)((multilevel_switch_value * color_switch_value[pwm_id]) / cc_multilevel_switch_get_max_value());
    update_rgb_led(&rgb_led_config, pwm_id, color_value);
  }
}


