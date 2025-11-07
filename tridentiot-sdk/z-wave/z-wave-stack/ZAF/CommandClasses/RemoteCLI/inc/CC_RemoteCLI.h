/// ***************************************************************************
///
/// @file CC_RemoteCLI.h
///
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2023 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************

#ifndef _CC_REMOTE_CLI_H_
#define _CC_REMOTE_CLI_H_

#include <ZAF_types.h>
#include <ZW_TransportEndpoint.h>
#include "ZAF_Actuator.h"

/**
 * @addtogroup CC Command Classes
 * @{
 * @addtogroup RemoteCLI Remote CLI
 * @{
 */

/**
 * @brief CLI command handler function pointer type
 */
typedef void (*cc_cli_command_handler_t)(uint16_t cmd_length, char* cmd);

 /**
 * Register a callback function to pass recevied CLI commands to
 *
 * @param cli_command_handler Function pointer to the CLI command handler
  */
void CC_remote_cli_set_command_handler(cc_cli_command_handler_t cli_command_handler);

/**
 * Send a Remote CLI report command
 *
 * @param data_length Length of the provided data
 * @param data Pointer to data
 *
  */
void CC_remote_cli_buffer_data(uint16_t data_length, uint8_t * data);

/**
 * @}
 * @}
 */

#endif /* _CC_REMOTE_CLI_H_ */
