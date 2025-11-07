/// ***************************************************************************
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2024 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************

/**
 * @file cli_uart_interface.h
 *
 * @brief Z-Wave radio test CLI tool uart interface
 */

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#ifndef __UART_INTERFACE__
#define __UART_INTERFACE__

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <zpal_uart.h>

#include "embedded_cli.h"
#include "radio_cli_commands.h"

/**
 * @brief Receive buffer size
 *
 */
 #define RECEIVE_BUFFER_SIZE     32

 /**
  * @brief Transmit buffer size
  *
  */
#define TRANSMIT_BUFFER_SIZE    100

/**
 * function callback definition for use when redirecting uart received data
 *
 * @param[out] value
 */
typedef void (*receiveCharHook_t)(char value);

/**
 * Function for redirecting received uart data from embeddedCLI
 *
 * @param[in] receiveCharFunc
 */
void cli_uart_receive_hook_set(receiveCharHook_t receiveCharFunc);

/**
 * @brief Initializes the given UART and the CLI.
 *
 * @param[in] uart_config   UART configuration
 * @param[in] callback  Callback function invoked on UART reception.
 */
void cli_uart_init(zpal_uart_config_t *uart_config, void (*callback)());

/**
 * Blocking function for printing a string to UART
 *
 * @param[in] message Message to print on the UART.
 */
void cli_uart_print(const char* message);

/**
 * Blocking printf for output on the CLI
 *
 * @param[in] pFormat Format to use for printing.
 */
void cli_uart_printf(const char* pFormat, ...);

/**
 * CLI UART Receive Handler
 */
void cli_uart_receive_handler(void);

#endif /* __UART_INTERFACE__ */
