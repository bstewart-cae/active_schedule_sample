/**
 * SPDX-License-Identifier: LicenseRef-TridentMSLA
 * SPDX-FileCopyrightText: 2024 Trident IoT, LLC <https://www.tridentiot.com>
 */
#include <apps_hw.h>
#include <sysfun.h>
#include <sysctrl.h>
//#define DEBUGPRINT // NOSONAR
#include <DebugPrint.h>
#include "SizeOf.h"
#include "events.h"
#ifdef TR_CLI_ENABLED
#include "tr_cli.h"
#endif
#include "setup_common_cli.h"
#include "adc_drv.h"
#include "zpal_misc.h"
#include <zaf_event_distributor_soc.h>
#include <CC_Battery.h>
#include "zpal_uart_gpio.h"
#include <tr_board_DKNCZ20.h>
#include "tr_hal_gpio.h"
#include "tr_hal_power.h"
#include <ZW_system_startup_api.h>
#include "zwave_api_interface.h"

#define MY_BATTERY_SPEC_LEVEL_FULL         3300  // My battery's 100% level (millivolts)
#define MY_BATTERY_SPEC_LEVEL_EMPTY        2400  // My battery's 0% level (millivolts)

static void gpio_window_handler (tr_hal_gpio_pin_t pin, __attribute__((unused)) tr_hal_gpio_event_t event);

/*defines what gpios ids are used*/
const uint8_t PB_LEARN_MODE          = TR_BOARD_BTN_LEARN_MODE;
const uint8_t PB_BATTERY_REPORT      = TR_BOARD_BTN2;


const uint8_t GPIO_WINDOW_EVENT      = 0; // if the gpio is high then window is closed else it is opend

/*Defines if the gpio support low power mode or nots*/
const  low_power_wakeup_cfg_t PB_LEARN_MODE_LP        = LOW_POWER_WAKEUP_GPIO4;
const  low_power_wakeup_cfg_t PB_BATTERY_REPORT_LP    = LOW_POWER_WAKEUP_GPIO5;
const  low_power_wakeup_cfg_t GPIO_WINDOW_EVENT_LP    = LOW_POWER_WAKEUP_GPIO0;


/*Defines the on state for the gpios*/
const uint8_t PB_LEARN_MODE_ON        = 0;
const uint8_t PB_BATTERY_REPORT_ON    = 0;


/*The LED id used for learn mode indicator*/
const uint8_t LED_LEARN_MODE_GPIO = TR_BOARD_LED_LEARN_MODE;
static gpio_info_t m_gpio_info[] = {
  GPIO_INFO(),
  GPIO_INFO()
};

static const gpio_config_t m_gpio_config[] = {
  GPIO_CONFIG(PB_LEARN_MODE,     PB_LEARN_MODE_LP,     PB_LEARN_MODE_ON,     EVENT_SYSTEM_LEARNMODE_TOGGLE, EVENT_SYSTEM_EMPTY, EVENT_SYSTEM_RESET, EVENT_SYSTEM_EMPTY),
  GPIO_CONFIG(PB_BATTERY_REPORT, PB_BATTERY_REPORT_LP, PB_BATTERY_REPORT_ON, EVENT_APP_BATTERY_REPORT, EVENT_SYSTEM_EMPTY, EVENT_SYSTEM_EMPTY, EVENT_SYSTEM_EMPTY)
};

#define BUTTONS_SIZE    sizeof_array(m_gpio_config)

const tr_hal_gpio_pin_t sensor_event_pin = {.pin = GPIO_WINDOW_EVENT};

/*Setup for debug and remote CLI uart*/

static const zpal_uart_config_ext_t ZPAL_UART_CONFIG_GPIO = {
  .txd_pin = TR_BOARD_UART0_TX,
  .rxd_pin = TR_BOARD_UART0_RX,
  .cts_pin = 0, // Not used.
  .rts_pin = 0, // Not used.
  .uart_wakeup = true
};

static zpal_uart_config_t UART_CONFIG =
{
  .id = ZPAL_UART0,
  .baud_rate = 115200,
  .data_bits = 8,
  .parity_bit = ZPAL_UART_NO_PARITY,
  .stop_bits = ZPAL_UART_STOP_BITS_1,
  .receive_callback = NULL,
  .ptr = &ZPAL_UART_CONFIG_GPIO,
  .flags = 0
};

zpal_debug_config_t debug_port_cfg = &UART_CONFIG;

#ifdef TR_CLI_ENABLED
static int cli_cmd_app_battery(int  argc, char *argv[]);

// Application specific CLI commands
TR_CLI_COMMAND_TABLE(app_specific_commands) =
{
  { "battery", cli_cmd_app_battery,   "Send battery report"                                },
  TR_CLI_COMMAND_TABLE_END
};

static int cli_cmd_app_battery(__attribute__((unused)) int  argc,__attribute__((unused))  char *argv[])
{
  zaf_event_distributor_enqueue_app_event_from_isr(EVENT_APP_BATTERY_REPORT);
  return 0;
}

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

#endif // #ifdef TR_CLI_ENABLED

static uint8_t set_sensor_state(tr_hal_gpio_pin_t pin)
{
  tr_hal_level_t read_value;
  tr_hal_gpio_read_input(pin, &read_value);
  if (TR_HAL_GPIO_LEVEL_HIGH == read_value)
  {
    tr_hal_gpio_set_interrupt_trigger(pin, TR_HAL_GPIO_TRIGGER_LEVEL_LOW);
    tr_hal_gpio_set_wake_mode(pin, TR_HAL_WAKE_MODE_INPUT_LOW);
    return EVENT_APP_WINDOW_CLOSE;
  }
  else
  {
    tr_hal_gpio_set_interrupt_trigger(pin, TR_HAL_GPIO_TRIGGER_LEVEL_HIGH);
    tr_hal_gpio_set_wake_mode(pin, TR_HAL_WAKE_MODE_INPUT_HIGH);
    return EVENT_APP_WINDOW_OPEN;
  }
}


static void gpio_window_handler (tr_hal_gpio_pin_t pin, __attribute__((unused)) tr_hal_gpio_event_t event)
{
  // When a gpio event is triggered
  // Invert the polarity of the gpio event trigger (low->high and vice versa)
  // Report the current gpio event to the upper layer
  zaf_event_distributor_enqueue_app_event_from_isr(set_sensor_state(sensor_event_pin));
  GPIO_CHIP_REGISTERS->clear_interrupt = (1<<pin.pin);
}


void app_hw_init(void)
{
  zpal_reset_reason_t resetReason = GetResetReason();
  enter_critical_section();
  apps_hw_init(m_gpio_config, m_gpio_info, BUTTONS_SIZE);

  tr_hal_gpio_settings_t   gpio_setting = {
    .direction = TR_HAL_GPIO_DIRECTION_INPUT,
    .interrupt_trigger = TR_HAL_GPIO_TRIGGER_NONE,
    .event_handler_fx = gpio_window_handler,
    .pull_mode = TR_HAL_PULLOPT_PULL_UP_100K,
    .enable_debounce = true
  };
  tr_hal_gpio_init(sensor_event_pin ,&gpio_setting);
  // if the sensor is waken up from deep sleep by an external interrupt
  // We skip setting the sensor gpio trigger polarity until we service the sensor gpio interrupt
  if ( ZPAL_RESET_REASON_DEEP_SLEEP_EXT_INT != resetReason)
  {
    set_sensor_state(sensor_event_pin);
  }
  leave_critical_section();

#ifdef TR_CLI_ENABLED
  // Enable wake up from UART0 upon reception of some data.
  Lpm_Enable_Low_Power_Wakeup(LOW_POWER_WAKEUP_UART0_DATA);
  set_powerdown_callback(disable_cli_lowpower);
  setup_cli(&UART_CONFIG);
#endif

}

uint8_t board_indicator_gpio_get( void)
{
  return LED_LEARN_MODE_GPIO;
}

void app_hw_deep_sleep_wakeup_handler(void)
{
  /* Nothing here, but offers the option to perform something after wake up
     from deep sleep. */
}


uint8_t
CC_Battery_BatteryGet_handler(uint8_t endpoint)
{
  uint32_t VBattery;

  uint8_t  accurateLevel;
  uint8_t  roundedLevel;
  uint8_t reporting_decrements;

  (void)endpoint;
   adc_init();
  /*
   * Simple example how to use the ADC to measure the battery voltage
   * and convert to a percentage battery level on a linear scale.
   */

  adc_get_voltage(&VBattery);
  /* Turn off the ADC when the conversion is finished to save power. */
  adc_enable(false);
  DPRINTF("\r\nBattery voltage: %dmV", VBattery);
  if (MY_BATTERY_SPEC_LEVEL_FULL <= VBattery)
  {
    // Level is full
    return (uint8_t)CMD_CLASS_BATTERY_LEVEL_FULL;
  }
  else if (MY_BATTERY_SPEC_LEVEL_EMPTY > VBattery)
  {
    // Level is empty (<0%)
    return (uint8_t)CMD_CLASS_BATTERY_LEVEL_WARNING;
  }
  else
  {
    reporting_decrements = cc_battery_config_get_reporting_decrements();
    // Calculate the percentage level from 0 to 100
    accurateLevel = (uint8_t)((100 * (VBattery - MY_BATTERY_SPEC_LEVEL_EMPTY)) / (MY_BATTERY_SPEC_LEVEL_FULL - MY_BATTERY_SPEC_LEVEL_EMPTY));

    // And round off to the nearest "reporting_decrements" level
    roundedLevel = (accurateLevel / reporting_decrements) * reporting_decrements; // Rounded down
    if ((accurateLevel % reporting_decrements) >= (reporting_decrements / 2))
    {
      roundedLevel += reporting_decrements; // Round up
    }
  }
  return roundedLevel;
}

// Return the state of the GPIO pin for turning off the LED
uint8_t board_indicator_led_off_gpio_state(void)
{
  return TR_HAL_GPIO_LEVEL_HIGH;
}
