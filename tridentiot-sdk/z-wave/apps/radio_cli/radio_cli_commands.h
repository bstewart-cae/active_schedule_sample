/// ***************************************************************************
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2024 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************

/**
 * @file radio_cli_commands.h
 * @brief Radio CLI commands
 */
#ifndef _CLI_COMMANDS_H_
#define _CLI_COMMANDS_H_

#include <embedded_cli.h>

/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/

/****************************************************************************/
/*                            EXPORTED FUNCTIONS                            */
/****************************************************************************/

/**
 * Initialize the CLI and add commands
 *
 * @param[in] cli
 */
void cli_commands_init(EmbeddedCli *cli);

/**
 * Execute cmdStr
 *
 * @param[in] cmdStr
 * @param[in] length
 */
void cli_command_execute(char * cmdStr, size_t length);

#endif  /* _CLI_COMMANDS_H_ */
