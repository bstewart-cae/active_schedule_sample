// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file
 * Platform abstraction for Serial API application
 *
 * @copyright 2022 Silicon Laboratories Inc.
 */
#ifndef SERIALAPI_HW_H_
#define SERIALAPI_HW_H_

#include "zpal_uart.h"

/**
 * typedef for serialapi uart configuration
 */
typedef struct
{
  zpal_uart_id_t uart_id;
  uint32_t baud_rate;
  uint8_t data_bits;
  zpal_uart_parity_bit_t parity_bit;              ///< Defines parity bit in the UART frame.
  zpal_uart_stop_bits_t stop_bits;
} serialapi_uart_config_t;

/**
 * Get zpal-uart config extension.
 */
const void* SerialAPI_get_uart_config_ext(void);

/**
 * Get zpal uart for use by SerialAPI
 */
void SerialAPI_get_uart_config(serialapi_uart_config_t *uart_config);

#ifdef USB_SUSPEND_SUPPORT

typedef void (*SerialAPI_hw_usb_suspend_callback_t)(void);

/**
 * Set USB suspend callback.
 *
 * @param[in] callback USB suspend callback.
 */
void SerialAPI_set_usb_supend_callback(SerialAPI_hw_usb_suspend_callback_t callback);

/**
 * USB suspend handler.
 */
void SerialAPI_hw_usb_suspend_handler(void);

#endif /* USB_SUSPEND_SUPPORT */

#endif /* SERIALAPI_HW_H_ */
