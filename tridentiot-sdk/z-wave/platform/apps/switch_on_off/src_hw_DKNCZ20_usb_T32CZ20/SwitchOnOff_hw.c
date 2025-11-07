/// ****************************************************************************
/// @file SwitchOnOff_hw.c
///
/// @brief SwitchOnOff Hardware setup for the DKNCZ20 board with UART on USB
///
///
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************

#include <apps_hw.h>
#include <DebugPrint.h>
#include "CC_BinarySwitch.h"
#include "SizeOf.h"
#include "events.h"
#include <inttypes.h>
#ifdef TR_CLI_ENABLED
#include "tr_cli.h"
#endif
#include <zaf_event_distributor_soc.h>
#include "tr_board_DKNCZ20.h"
#include "zpal_uart.h"
#include "zpal_misc.h"
#include "zpal_misc_private.h"
#include "setup_common_cli.h"
#include "MfgTokens.h"
#include "ZAF_Common_interface.h"
#include "tr_hal_gpio.h"

/*defines what gpios ids are used*/
const uint8_t PB_LEARN_MODE = (uint8_t)TR_BOARD_BTN_LEARN_MODE;
const uint8_t PB_SWITCH     = (uint8_t)TR_BOARD_BTN2;

/*Defines if the gpio support low power mode or nots*/
const  low_power_wakeup_cfg_t PB_LEARN_MODE_LP    = LOW_POWER_WAKEUP_GPIO4;
const  low_power_wakeup_cfg_t PB_SWITCH_LP        = LOW_POWER_WAKEUP_GPIO5;

/*Defines the on state for the gpios*/
const uint8_t PB_LEARN_MODE_ON  = 0;
const uint8_t PB_SWITCH_ON      = 0;

/* Application specific LED */
const uint8_t LED_SWITCH     = TR_BOARD_LED_BLUE;

static gpio_info_t m_gpio_info[] = {
  GPIO_INFO(),
  GPIO_INFO()
};

static const gpio_config_t m_gpio_config[] = {
  GPIO_CONFIG(PB_LEARN_MODE, PB_LEARN_MODE_LP, PB_LEARN_MODE_ON, EVENT_SYSTEM_LEARNMODE_TOGGLE, EVENT_SYSTEM_EMPTY, EVENT_SYSTEM_RESET, EVENT_SYSTEM_EMPTY),
  GPIO_CONFIG(PB_SWITCH, PB_SWITCH_LP, PB_SWITCH_ON, EVENT_APP_TOGGLE_LED, EVENT_SYSTEM_EMPTY, EVENT_SYSTEM_EMPTY, EVENT_SYSTEM_EMPTY)
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
static int cli_cmd_app_nif(int  argc, char *argv[]);

// Application specific CLI commands
TR_CLI_COMMAND_TABLE(app_specific_commands) =
{
  { "toggle_state", cli_cmd_app_onoff,   "Toggle switch state"                                },
  { "nif",          cli_cmd_app_nif,     "Send a node information frame"                      },
  TR_CLI_COMMAND_TABLE_END
};

static int cli_cmd_app_onoff(__attribute__((unused)) int  argc,__attribute__((unused))  char *argv[])
{
  zaf_event_distributor_enqueue_app_event_from_isr(EVENT_APP_TOGGLE_LED);
  return 0;
}

static int cli_cmd_app_nif(__attribute__((unused)) int argc, __attribute__((unused)) char *argv[])
{
  zaf_event_distributor_enqueue_app_event_from_isr(EVENT_APP_SEND_NIF);
  return 0;
}

#endif // #ifdef TR_CLI_ENABLED

void app_hw_init(void)
{
  apps_hw_init(m_gpio_config, m_gpio_info, BUTTONS_SIZE);
  tr_hal_gpio_pin_t pin = {.pin = LED_SWITCH};
  tr_hal_gpio_settings_t gpio_setting = DEFAULT_GPIO_OUTPUT_CONFIG;
  tr_hal_gpio_init(pin, &gpio_setting);

#ifdef TR_CLI_ENABLED
  setup_cli(&UART_CONFIG);
#endif
}

void cc_binary_switch_handler(cc_binary_switch_t * p_switch)
{
  tr_hal_gpio_pin_t pin = {.pin = LED_SWITCH};
  uint8_t value = ZAF_Actuator_GetCurrentValue(&p_switch->actuator);
  if(value)
  {
    tr_hal_gpio_set_output(pin, TR_HAL_GPIO_LEVEL_LOW);
  }
  else
  {
    tr_hal_gpio_set_output(pin, TR_HAL_GPIO_LEVEL_HIGH);
  }
  DPRINTF("cc_binary_switch_handler: %d\n", value);
}

uint8_t board_indicator_gpio_get( void)
{
  return TR_BOARD_LED_LEARN_MODE;
}

uint8_t board_indicator_led_off_gpio_state(void)
{
  return TR_HAL_GPIO_LEVEL_HIGH;
}
