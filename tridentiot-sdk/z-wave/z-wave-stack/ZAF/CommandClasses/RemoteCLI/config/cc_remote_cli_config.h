/// ***************************************************************************
///
/// @file cc_remote_cli_config.h
///
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2023 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************

#ifndef _CC_REMOTE_CLI_CONFIG_H_
#define _CC_REMOTE_CLI_CONFIG_H_

/**
 * \defgroup configuration Configuration
 * Configuration
 *
 * \addtogroup configuration
 * @{
 */
/**
 * \defgroup command_class_remote_cli_configuration Command Class Remote CLI Configuration
 * Command Class Remote CLI Configuration
 *
 * \addtogroup command_class_remote_cli_configuration
 * @{
 */

/**
 * Tx buffer timeout in milliseconds <1..255>
 *
 * Factory default duration in miliseconds
 */
#if !defined(CC_REMOTE_CLI_CONFIG_TRANSMIT_BUFFER_TIMEOUT)
#define CC_REMOTE_CLI_CONFIG_TRANSMIT_BUFFER_TIMEOUT  10
#endif /* !defined(CC_REMOTE_CLI_CONFIG_TRANSMIT_BUFFER_TIMEOUT) */

/**
 * Tx buffer size in bytes <1..1024>
 *
 * Factory default data buffer size
 */
#if !defined(CC_REMOTE_CLI_CONFIG_TRANSMIT_BUFFER_LENGTH)
#define CC_REMOTE_CLI_CONFIG_TRANSMIT_BUFFER_LENGTH  500
#endif /* !defined(CC_REMOTE_CLI_CONFIG_TRANSMIT_BUFFER_TIMEOUT) */

/**@}*/ /* \addtogroup command_class_remote_cli_configuration */

/**@}*/ /* \addtogroup configuration */
#endif /* _CC_REMOTE_CLI_CONFIG_H_ */
