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
#include <tr_board_TZM8202-02.h>

/*Setup for debug and remote CLI uart*/

// Uart buffers
#define COMM_INT_TX_BUFFER_SIZE 192
#define COMM_INT_RX_BUFFER_SIZE 256

static uint8_t tx_data[COMM_INT_TX_BUFFER_SIZE] __attribute__ ((aligned(4)));
static uint8_t rx_data[COMM_INT_RX_BUFFER_SIZE] __attribute__ ((aligned(4)));

static const zpal_uart_config_ext_t ZPAL_UART_CONFIG_GPIO = {
  .txd_pin = TR_BOARD_UART2_TX,
  .rxd_pin = TR_BOARD_UART2_RX,
  .cts_pin = 0, // Not used.
  .rts_pin = 0, // Not used.
  .uart_wakeup = false
};

zpal_uart_config_t CLI_UART_CONFIG =
{
  .id = ZPAL_UART2,
  .tx_buffer = tx_data,
  .tx_buffer_len = COMM_INT_TX_BUFFER_SIZE,
  .rx_buffer = rx_data,
  .rx_buffer_len = COMM_INT_RX_BUFFER_SIZE,
  .baud_rate = 115200,
  .data_bits = 8,
  .parity_bit = ZPAL_UART_NO_PARITY,
  .stop_bits = ZPAL_UART_STOP_BITS_1,
  .receive_callback = NULL,
  .ptr = &ZPAL_UART_CONFIG_GPIO,
};
