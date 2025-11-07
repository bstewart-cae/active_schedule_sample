/// ****************************************************************************
/// @file setup_common_cli.h
///
/// @brief Setting up common application test CLI
///
///
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************

#ifndef SETUP_CLI_H_
#define SETUP_CLI_H_

#include "zpal_uart.h"

/**
 * @brief Initialize the application CLI
 *
 * Note: The CLI can be used both as a local or a remote CLI.
 * If used as a local CLI a valid UART configuration must be provided.alignas
 * If used as a remote CLI, a NULL pointer can be passed as UART configuration.
 *
 * @param[in]  p_uart_config Pointer UART configuration used by the CLI
 *
 */
void setup_cli(zpal_uart_config_t *p_uart_config);

/**
 * @brief Implementation of the print function for the CLI
 *
 * @param[in]  message String to print to the CLI
 *
 */
void tr_cli_common_print(const char* message);

#endif // SETUP_CLI_H_
