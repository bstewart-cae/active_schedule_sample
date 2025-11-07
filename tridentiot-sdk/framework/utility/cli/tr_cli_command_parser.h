/// ****************************************************************************
/// @file tr_cli_command_parser.h
///
/// @brief common CLI utility for parsing incoming commands
///
///
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************
#ifndef TR_CLI_COMMAND_PARSER_H
#define TR_CLI_COMMAND_PARSER_H

#include <stdint.h>
#include "tr_cli_command_table.h"

/**
 * @addtogroup tr-utility-cli
 * @{
 * @addtogroup tr-utility-cli-command-parser command-parser
 * @brief
 * CLI command parsing utilities for processing command arguments.
 *
 * @{
 */

/**
 * @brief Maximum number of arguments that can be parsed from a command line.
 */
#define TR_ARGUMENT_PARSER_MAX_ARGS (15)

/**
 * @brief Parses a command and executes the appropriate handler.
 *
 * @param [in] cmd_table Pointer to the command table containing available commands.
 * @param [in] argc      Number of arguments in the command line.
 * @param [in] argv      Array of argument strings.
 *
 */
void tr_cli_parse_command(const tr_command_s *cmd_table,
                          int                argc,
                          char               **argv);

/**
 * @brief Gets an option from the command line arguments.
 *
 * @param [in]  argc      Number of arguments in the command line.
 * @param [in]  argv      Array of argument strings.
 * @param [in]  opt_string String containing the option characters to look for.
 * @param [out] ret_arg   Pointer to store the option argument if found.
 *
 * @return 1 if the option was found, 0 otherwise.
 *
 */
uint8_t tr_cli_get_option(int  argc,
                          char *argv[],
                          char *opt_string,
                          char **ret_arg);

/**
 * @brief Converts a decimal or hexadecimal string to an integer.
 *
 * @param [in] number_string String containing the number to convert.
 *
 * @return The converted integer value.
 *
 */
uint64_t tr_dec_or_hex_string_to_int(const char *number_string);

/**
 * @} tr-utility-cli-command-parser
 * @} tr-utility-cli
 */

#endif // TR_CLI_COMMAND_PARSER_H
