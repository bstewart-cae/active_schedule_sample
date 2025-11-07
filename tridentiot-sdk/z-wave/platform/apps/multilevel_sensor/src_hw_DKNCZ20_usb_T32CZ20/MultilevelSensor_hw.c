/// ****************************************************************************
/// @file MultilevelSensor_hw.c
///
/// @brief MultiLevel Sensor Hardware setup for the DKNCZ20 board with UART on USB
///
///
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************

#include <apps_hw.h>
#include <DebugPrint.h>
#include "SizeOf.h"
#include "events.h"
#ifdef TR_CLI_ENABLED
#include "tr_cli.h"
#endif
#include "setup_common_cli.h"
#include "lpm.h"
#include "zpal_misc.h"
#include <zaf_event_distributor_soc.h>
#include <tr_board_DKNCZ20.h>
#include "tr_hal_gpio.h"
#include "tr_hal_power.h"
#include "zwave_api_interface.h"

/*defines what gpios ids are used*/
const uint8_t PB_LEARN_MODE                  = TR_BOARD_BTN_LEARN_MODE;
const uint8_t PB_BATT_AND_TEMP_REPORT        = TR_BOARD_BTN2;

/*Defines if the gpio support low power mode or nots*/
static const low_power_wakeup_cfg_t PB_LEARN_MODE_LP             = LOW_POWER_WAKEUP_GPIO4;
static const low_power_wakeup_cfg_t PB_BATT_AND_TEMP_REPORT_LP   = LOW_POWER_WAKEUP_GPIO5;


/*Defines the on state for the gpios*/
const uint8_t PB_LEARN_MODE_ON           = 0;
const uint8_t PB_BATT_AND_TEMP_REPORT_ON        = 0;

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
  GPIO_CONFIG(PB_BATT_AND_TEMP_REPORT, PB_BATT_AND_TEMP_REPORT_LP, PB_BATT_AND_TEMP_REPORT_ON, EVENT_APP_SEND_BATTERY_LEVEL_AND_SENSOR_REPORT, EVENT_SYSTEM_EMPTY, EVENT_SYSTEM_EMPTY, EVENT_SYSTEM_EMPTY)
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
/*
  This callback is called just before entring deep sleep mode
  Since the UART does't work in this power mode, we disable the UART's low power clock source to save power.
*/
static void disable_cli_lowpower(void)
{
  Lpm_Disable_Low_Power_Wakeup(LOW_POWER_WAKEUP_UART0_DATA);
  // this is a workaround to disable the uart's low power clock source.
  tr_hal_power_disable_clock(TR_HAL_CLOCK_1M);
}


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
  // Enable wake up from UART0 upon reception of some data.
  Lpm_Enable_Low_Power_Wakeup(LOW_POWER_WAKEUP_UART0_DATA);
  set_powerdown_callback(disable_cli_lowpower);
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