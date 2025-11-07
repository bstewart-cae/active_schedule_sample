/// ****************************************************************************
/// @file tr_cli_command_table.h
///
/// @brief common CLI command table structure
///
///
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************
#ifndef TR_CLI_COMMAND_TABLE_H
#define TR_CLI_COMMAND_TABLE_H

/**
 * @addtogroup tr-utility-cli
 * @{
 * @addtogroup tr-utility-cli-command-table command-table
 * @brief
 * CLI command table structures and macros for defining command handlers.
 *
 * @{
 */

/**
 * @brief Structure used for each CLI command table entry.
 *
 */
typedef struct
{
    /**
     * @brief Command string that triggers this handler.
     */
    const char *command;

    /**
     * @brief Function pointer to the command handler.
     *
     * @param [in] argc Number of arguments.
     * @param [in] argv Array of argument strings.
     *
     * @return Command execution result.
     */
    int (*handler)(int  argc,
                   char *argv[]);

    /**
     * @brief Help text describing the command.
     */
    const char *help;
} tr_command_s;

/**
 * @brief Macro for creating CLI command tables.
 *
 * @param table Name of the command table array.
 */
#define TR_CLI_COMMAND_TABLE(table) const tr_command_s table[]

/**
 * @brief Macro for marking the end of a command table.
 */
#define TR_CLI_COMMAND_TABLE_END { "", NULL, "" }

/**
 * @brief Macro for indicating no sub-commands.
 */
#define TR_CLI_SUB_COMMANDS NULL

/**
 * @brief Macro for creating sub-command table references.
 */
#define TR_CLI_SUB_COMMAND_TABLE (const char*)

/**
 * @} tr-utility-cli-command-table
 * @} tr-utility-cli
 */


#endif // TR_CLI_COMMAND_TABLE_H
