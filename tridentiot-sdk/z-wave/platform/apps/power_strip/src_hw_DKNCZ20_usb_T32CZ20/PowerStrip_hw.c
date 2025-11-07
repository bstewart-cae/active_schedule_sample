/// ****************************************************************************
/// @file PowerStrip_hw.c
///
/// @brief PowerStrip Hardware setup for the DKNCZ20 board with UART on USB
///
///
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include "SizeOf.h"
#include <events.h>
#include <inttypes.h>
#include <app_hw.h>
#include <apps_hw.h>

#include <sysctrl.h>
#ifdef TR_CLI_ENABLED
#include "tr_cli.h"
#endif
#include <DebugPrint.h>
#include <zaf_event_distributor_soc.h>
#include "CC_MultilevelSwitch_Support.h"
#include "CC_BinarySwitch.h"
#include "zpal_uart.h"
#include "zpal_misc.h"
#include "zpal_misc_private.h"
#include "setup_common_cli.h"
#include "tr_board_DKNCZ20.h"
#include "rgb_led_drv.h"
#include "tr_hal_gpio.h"

/*defines what gpios ids are used*/
const uint8_t PB_LEARN_MODE = TR_BOARD_BTN1;
const uint8_t PB_OUTLET_SWITCH  = TR_BOARD_BTN2;


/*Defines if the gpio support low power mode or nots*/
const  low_power_wakeup_cfg_t PB_LEARN_MODE_LP             = LOW_POWER_WAKEUP_GPIO4;
const  low_power_wakeup_cfg_t PB_OUTLET_SWITCH_LP        = LOW_POWER_WAKEUP_GPIO5;



/*Defines the on state for the gpios*/
const uint8_t PB_LEARN_MODE_ON          = 0;
const uint8_t PB_OUTLET_SWITCH_ON      = 0;


/* Application specific LED*/
const uint8_t OUTLET1_LED     = TR_BOARD_LED_BLUE;
/*The LED id used for learn mode indicator*/
const uint8_t LED_LEARN_MODE_GPIO = TR_BOARD_LED_GREEN;

static gpio_info_t m_gpio_info[] = {
  GPIO_INFO(),
  GPIO_INFO()
};

static const gpio_config_t m_gpio_config[] = {
  //          GPIO number,       gpio powerdown mode   gpio on state         short press event                     hold event                        long press (5 sec) event           release event
  GPIO_CONFIG(PB_LEARN_MODE,     PB_LEARN_MODE_LP,     PB_LEARN_MODE_ON,     EVENT_SYSTEM_LEARNMODE_TOGGLE,        EVENT_SYSTEM_EMPTY,               EVENT_SYSTEM_RESET,                EVENT_APP_NOTIFICATION_TOGGLE),
  GPIO_CONFIG(PB_OUTLET_SWITCH,  PB_OUTLET_SWITCH_LP,   PB_OUTLET_SWITCH_ON, EVENT_APP_OUTLET1_TOGGLE,             EVENT_APP_OUTLET2_DIMMER_HOLD,    EVENT_APP_OUTLET2_DIMMER_RELEASE,  EVENT_APP_OUTLET2_DIMMER_RELEASE)
};

#define BUTTONS_SIZE    sizeof_array(m_gpio_config)

/*Setup for debug and remote CLI uart*/

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

static int cli_cmd_app_onoff(int  argc, char *argv[]);

// Application specific CLI commands
TR_CLI_COMMAND_TABLE(app_specific_commands) =
{
  { "toggle_notif", cli_cmd_app_onoff,   "Toggle notification on/off"                                },
  TR_CLI_COMMAND_TABLE_END
};

static int cli_cmd_app_onoff(__attribute__((unused)) int  argc,__attribute__((unused))  char *argv[])
{
  zaf_event_distributor_enqueue_app_event_from_isr(EVENT_APP_NOTIFICATION_TOGGLE);
  return 0;
}

#endif // #ifdef TR_CLI_ENABLED

static uint8_t outlet2_value = 0xFF;
static uint32_t pwm_duty_array;
static const uint32_t pwm_gpio = TR_BOARD_LED_RED;
static const uint8_t  pwm_gpio_mode = TR_HAL_GPIO_MODE_PWM0;
static const tr_hal_gpio_pin_t outled1 = {.pin = OUTLET1_LED};
static rgb_led_config_t rgb_led_config = {.pPwm_duty_arry = &pwm_duty_array,
                                          .pPwm_gpio_arry = &pwm_gpio,
                                          .pPwm_gpio_mode_arry = &pwm_gpio_mode,
                                          .pwm_ch_count = 1};
void zwsdk_app_hw_init(void)
{
  rgb_led_init(&rgb_led_config);
}

void app_hw_init(void)
{
  tr_hal_gpio_set_output(outled1, TR_HAL_GPIO_LEVEL_HIGH);
  tr_hal_gpio_set_direction(outled1, TR_HAL_GPIO_DIRECTION_OUTPUT);

  apps_hw_init(m_gpio_config, m_gpio_info, BUTTONS_SIZE);
#ifdef TR_CLI_ENABLED
  setup_cli(&UART_CONFIG);
#endif
}

void cc_binary_switch_handler(cc_binary_switch_t * p_switch)
{
  uint8_t switch_value = ZAF_Actuator_GetCurrentValue(&p_switch->actuator);
  if (p_switch->endpoint == 1)
  {
    switch_value?tr_hal_gpio_set_output(outled1, TR_HAL_GPIO_LEVEL_LOW) : tr_hal_gpio_set_output(outled1, TR_HAL_GPIO_LEVEL_HIGH);
  }
  else if (p_switch->endpoint == 2)
  {
    outlet2_value = switch_value;
    update_rgb_led(&rgb_led_config, 0, switch_value);
  }
}

void cc_multilevel_switch_support_cb(cc_multilevel_switch_t * p_switch)
{
  if (p_switch->endpoint == 2)
  {
    uint16_t multilevel_switch_value = cc_multilevel_switch_get_current_value(p_switch);
    uint8_t switch_value = (uint8_t)((multilevel_switch_value * outlet2_value) / cc_multilevel_switch_get_max_value());
    update_rgb_led(&rgb_led_config, 0, switch_value);
  }
}

uint8_t board_indicator_gpio_get( void)
{
  return LED_LEARN_MODE_GPIO;
}

// Return the state of the GPIO pin for turning off the LED
uint8_t board_indicator_led_off_gpio_state(void)
{
  return TR_HAL_GPIO_LEVEL_HIGH;
}
