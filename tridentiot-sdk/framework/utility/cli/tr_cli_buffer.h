/// ****************************************************************************
/// @file tr_cli_buffer.h
///
/// @brief code for buffering bytes to be sent to the CLI utility
///
///
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************
#ifndef TR_CLI_BUFFER_H
#define TR_CLI_BUFFER_H

#include <stdint.h>

/**
 * @addtogroup tr-utility-cli
 * @{
 * @addtogroup tr-utility-cli-buffer buffer
 * @brief
 * CLI buffer management for handling incoming UART data.
 *
 * @{
 */

/**
 * @brief Initializes the CLI byte buffer.
 *
 * This function initializes the internal buffer used for storing incoming
 * UART characters before they are processed by the CLI system.
 *
 */
void tr_cli_buffer_init(void);

/**
 * @brief Stores a byte from the UART in the buffer.
 *
 * @param [in] new_input_char The character received from UART to be buffered.
 *
 */
void tr_cli_buffer_byte(uint8_t new_input_char);

/**
 * @brief Processes buffered bytes and hands them to the CLI.
 *
 * This function empties the buffer and processes all stored characters
 * through the CLI command parser.
 *
 */
void tr_cli_buffer_process_bytes(void);

/**
 * @brief Clears the buffer driver statistics.
 *
 * Resets all statistical counters to zero.
 *
 */
void tr_cli_buffer_clear_stats(void);

/**
 * @brief Structure containing CLI buffer driver statistics.
 *
 */
typedef struct
{
    /**
     * @brief Maximum number of buffered bytes at any time since stats were cleared.
     */
    uint8_t buffer_high_watermark;

    /**
     * @brief Number of character drops due to buffer being full.
     */
    uint16_t byte_drops;

    /**
     * @brief Size of the character buffer.
     */
    uint16_t buffer_size;

} tr_cli_buffer_stats_t;

/**
 * @brief Reads the current buffer driver statistics.
 *
 * @return Current statistics for the CLI buffer driver.
 *
 */
tr_cli_buffer_stats_t tr_cli_buffer_get_stats(void);

/**
 * @} tr-utility-cli-buffer
 * @} tr-utility-cli
 */


#endif // TR_CLI_BUFFER_H
