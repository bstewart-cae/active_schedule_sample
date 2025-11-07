/// ***************************************************************************
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2024 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************

/**
 * @file SerialAPI_hw.c
 */
#include <stdbool.h>
#include "SerialAPI_hw.h"
#include "DebugPrint.h"
#include "zpal_uart.h"

void app_hw_init(void)
{
  // Nothing here.
}

bool apps_hw_needs_uart(void)
{
  return true;
}

void SerialAPI_get_uart_config(serialapi_uart_config_t *uart_config)
{
  uart_config->uart_id = ZPAL_UART0;
  uart_config->baud_rate = 115200;
  uart_config->data_bits = 8;
  uart_config->parity_bit = ZPAL_UART_NO_PARITY;
  uart_config->stop_bits = ZPAL_UART_STOP_BITS_1;
}

const void* SerialAPI_get_uart_config_ext(void)
{
  return NULL;
}
