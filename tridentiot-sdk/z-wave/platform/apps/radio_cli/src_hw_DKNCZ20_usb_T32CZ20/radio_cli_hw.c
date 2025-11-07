/// ****************************************************************************
/// @file radio_cli_hw.c
///
/// @brief Radio CLIHardware setup for the DKNCZ20 board with UART on USB
///
///
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************

#include "zpal_uart.h"
#include "zpal_uart_gpio.h"
#include "zpal_misc.h"
#include <tr_board_DKNCZ20.h>
#include "tr_hal_gpio.h"
#include "FreeRTOS.h"
#include "timers.h"

/*Setup for debug and remote CLI uart*/

// Uart buffers
#define COMM_INT_TX_BUFFER_SIZE 192
#define COMM_INT_RX_BUFFER_SIZE 256

static uint8_t tx_data[COMM_INT_TX_BUFFER_SIZE] __attribute__ ((aligned(4)));
static uint8_t rx_data[COMM_INT_RX_BUFFER_SIZE] __attribute__ ((aligned(4)));

static const zpal_uart_config_ext_t ZPAL_UART_CONFIG_GPIO = {
  .txd_pin = TR_BOARD_UART0_TX,
  .rxd_pin = TR_BOARD_UART0_RX,
  .cts_pin = 0, // Not used.
  .rts_pin = 0, // Not used.
  .uart_wakeup = false
};

zpal_uart_config_t CLI_UART_CONFIG =
{
  .id = ZPAL_UART0,
  .tx_buffer = tx_data,
  .tx_buffer_len = COMM_INT_TX_BUFFER_SIZE,
  .rx_buffer = rx_data,
  .rx_buffer_len = COMM_INT_RX_BUFFER_SIZE,
  .baud_rate = 230400,
  .data_bits = 8,
  .parity_bit = ZPAL_UART_NO_PARITY,
  .stop_bits = ZPAL_UART_STOP_BITS_1,
  .receive_callback = NULL,
  .ptr = &ZPAL_UART_CONFIG_GPIO,
};

static tr_hal_gpio_pin_t pin = {0};
static TimerHandle_t pulse_timer = NULL;
static StaticTimer_t pulse_timer_buffer;

static void gpio_timer_cb(__attribute__((unused)) TimerHandle_t xTimer)
{
  tr_hal_gpio_set_output(pin, TR_HAL_GPIO_LEVEL_HIGH);
}

void radio_cli_hw_gpio_pulse(
  uint32_t pin_number,
  uint32_t width)
{
  pin.pin = pin_number;
  tr_hal_gpio_settings_t gpio_setting = DEFAULT_GPIO_OUTPUT_CONFIG;
  tr_hal_gpio_init(pin, &gpio_setting);

  tr_hal_gpio_set_output(pin, TR_HAL_GPIO_LEVEL_LOW);

  if (NULL == pulse_timer)
  {
    // Create the timer once on the first pulse.
    pulse_timer = xTimerCreateStatic("GPIO pulse width",
                                      pdMS_TO_TICKS(width),
                                      pdFALSE,
                                      NULL,
                                      gpio_timer_cb,
                                      &pulse_timer_buffer);
  }

  xTimerChangePeriod(pulse_timer, pdMS_TO_TICKS(width), 0);
  xTimerStart(pulse_timer, 0);
}

void radio_cli_hw_gpio_output(
  uint32_t pin_number,
  bool level)
{
  pin.pin = pin_number;
  tr_hal_gpio_settings_t gpio_setting = DEFAULT_GPIO_OUTPUT_CONFIG;
  tr_hal_gpio_init(pin, &gpio_setting);
  tr_hal_gpio_set_output(pin, (tr_hal_level_t)level);
}

void radio_cli_hw_gpio_set(
  uint32_t pin_number,
  bool level)
{
  pin.pin = pin_number;
  tr_hal_gpio_set_output(pin, (tr_hal_level_t)level);
}

bool radio_cli_hw_gpio_get(
  uint32_t pin_number,
  bool *level,
  bool *isOutput)
{
  pin.pin = pin_number;
  tr_hal_gpio_settings_t gpio_setting = DEFAULT_GPIO_OUTPUT_CONFIG;
  tr_hal_status_t status = tr_hal_gpio_read_settings(pin, &gpio_setting);

  if(TR_HAL_SUCCESS != status)
  {
    return false;
  }

  if (TR_HAL_GPIO_DIRECTION_INPUT == gpio_setting.direction)
  {
    // Input GPIOs must be read with tr_hal_gpio_read_input.
    tr_hal_level_t read_value;
    status = tr_hal_gpio_read_input(pin, &read_value);
    if (TR_HAL_SUCCESS != status)
    {
      return false;
    }
    *isOutput = false;
    *level = (TR_HAL_GPIO_LEVEL_HIGH == read_value);
  } else {
    // Output GPIO level is already stored in settings.
    *isOutput = true;
    *level = (TR_HAL_GPIO_LEVEL_HIGH == gpio_setting.output_level);
  }

  return true;
}

bool radio_cli_hw_gpio_input(
  uint32_t pin_number,
  bool pullup)
{
  pin.pin = pin_number;
  tr_hal_gpio_settings_t gpio_setting = DEFAULT_GPIO_INPUT_CONFIG;
  gpio_setting.pull_mode = pullup ? TR_HAL_PULLOPT_PULL_UP_10K : TR_HAL_PULLOPT_PULL_NONE;
  tr_hal_status_t status = tr_hal_gpio_init(pin, &gpio_setting);

  if (TR_HAL_SUCCESS != status)
  {
    return false; // Failed to initialize GPIO as input
  }

  return true; // Successfully set GPIO as input
}
