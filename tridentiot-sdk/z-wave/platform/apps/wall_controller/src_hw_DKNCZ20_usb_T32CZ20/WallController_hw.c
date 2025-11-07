/// ****************************************************************************
/// @file WallController_hw.c
///
/// @brief Wall Controller Hardware setup for the DKNCZ20 board with UART on USB
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
#include "zpal_uart.h"
#include "zpal_misc.h"
#include "zpal_misc_private.h"
#include "setup_common_cli.h"
#include "MfgTokens.h"
#include "ZAF_Common_interface.h"
#include "tr_board_DKNCZ20.h"
#include "tr_hal_gpio.h"

/*defines what gpios ids are used*/
static const uint8_t PB_LEARN_MODE  = (uint8_t)TR_BOARD_BTN_LEARN_MODE;
static const uint8_t PB_KEY1        = (uint8_t)TR_BOARD_BTN2;

/*Defines if the gpio support low power mode or nots*/
static const low_power_wakeup_cfg_t PB_LEARN_MODE_LP = LOW_POWER_WAKEUP_NULL;
static const low_power_wakeup_cfg_t PB_KEY1_LP       = LOW_POWER_WAKEUP_NULL;

/*Defines the on state for the gpios*/
const uint8_t PB_LEARN_MODE_ON  = 0;
const uint8_t PB_KEY1_ON        = 0;

/**struct used to hodl the state of the gpios*/
static gpio_info_t m_gpio_info[] = {
  GPIO_INFO(),
  GPIO_INFO()
};
/*
structure that descibed how are the gpios are configured
  {
    gpio_no        : the gpio id
    low_power      : the gpio power down state
    on_value       : the gpio ON state
    short_event    : the event triggered on the gpio short press
    hold_event     : the event triggered when the gpio is hold
    long_event     : the event triggered on the gpio long press
    release_event  : the event triggered on the gpio is released (UP) after 300 ms
  }
*/
static const gpio_config_t m_gpio_config[] = {
  GPIO_CONFIG(PB_LEARN_MODE, PB_LEARN_MODE_LP, PB_LEARN_MODE_ON, EVENT_SYSTEM_LEARNMODE_TOGGLE, EVENT_SYSTEM_EMPTY, EVENT_SYSTEM_RESET, EVENT_SYSTEM_EMPTY),
  GPIO_CONFIG(PB_KEY1, PB_KEY1_LP, PB_KEY1_ON, EVENT_APP_KEY01_SHORT_PRESS, EVENT_APP_KEY01_HOLD, EVENT_APP_KEY01_RELEASE, EVENT_APP_KEY01_RELEASE)
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

// Application specific CLI commands
TR_CLI_COMMAND_TABLE(app_specific_commands) =
{
  TR_CLI_COMMAND_TABLE_END
};

#endif // #ifdef TR_CLI_ENABLED

void app_hw_init(void)
{
  apps_hw_init(m_gpio_config, m_gpio_info, BUTTONS_SIZE);
#ifdef TR_CLI_ENABLED
  setup_cli(&UART_CONFIG);
#endif
}

uint8_t board_indicator_gpio_get( void)
{
  return TR_BOARD_LED_LEARN_MODE;
}

// Return the state of the GPIO pin for turning off the LED
uint8_t board_indicator_led_off_gpio_state(void)
{
  return TR_HAL_GPIO_LEVEL_HIGH;
}
