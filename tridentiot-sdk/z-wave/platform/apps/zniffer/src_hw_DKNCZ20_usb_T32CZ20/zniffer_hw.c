/// ****************************************************************************
/// @file zniffer_hw.c
///
/// @brief Znuffer Hardware setup for the DKNCZ20 board with UART on USB
///
///
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************

#include "zpal_uart.h"
#include "zpal_uart_gpio.h"
#include "zpal_misc.h"
#include <tr_board_DKNCZ20.h>

/*Setup uart for host communication */

// Uart buffers
#define COMM_INT_RX_BUFFER_SIZE 64
static uint8_t rx_data[COMM_INT_RX_BUFFER_SIZE] __attribute__ ((aligned(4)));

static const zpal_uart_config_ext_t ZPAL_UART_CONFIG_GPIO = {
  .txd_pin = TR_BOARD_UART0_TX,
  .rxd_pin = TR_BOARD_UART0_RX,
  .cts_pin = 0, // Not used.
  .rts_pin = 0, // Not used.
  .uart_wakeup = false
};

zpal_uart_config_t ZNIFFER_UART_CONFIG =
{
  .id = ZPAL_UART0,
  .tx_buffer = NULL,
  .tx_buffer_len = 0,
  .rx_buffer = rx_data,
  .rx_buffer_len = COMM_INT_RX_BUFFER_SIZE,
  .baud_rate = 230400,
  .data_bits = 8,
  .parity_bit = ZPAL_UART_NO_PARITY,
  .stop_bits = ZPAL_UART_STOP_BITS_1,
  .receive_callback = NULL,
  .ptr = &ZPAL_UART_CONFIG_GPIO,
};
