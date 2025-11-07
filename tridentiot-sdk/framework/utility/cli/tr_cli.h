/// ****************************************************************************
/// @file tr_cli.h
///
/// @brief common CLI utility
///
///
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************
#ifndef TR_CLI_H
#define TR_CLI_H

#include <stdbool.h>
#include "tr_printf.h"
#include "tr_cli_command_parser.h"
#include "tr_cli_buffer.h"

/**
 * @addtogroup tr-utility
 * @{
 * @addtogroup tr-utility-cli CLI
 * @brief
 * Command Line Interface utilities for embedded systems.
 *
 * @{
 * @addtogroup tr-utility-cli-common common
 * @brief
 * Common CLI functionality including main structure and core functions.
 *
 * @{
 */

#ifndef TR_CLI_MAX_LINE
/**
 * @brief Maximum number of bytes to accept in a single line.
 */
#define TR_CLI_MAX_LINE 120
#endif

#ifndef TR_CLI_HISTORY_LEN
/**
 * @brief Maximum number of bytes to retain of history data.
 *
 * Define this to 0 to remove history support.
 */
#define TR_CLI_HISTORY_LEN 1000
#endif

#ifndef TR_CLI_MAX_ARGC
/**
 * @brief Maximum number of arguments we reserve space for.
 */
#define TR_CLI_MAX_ARGC 16
#endif

#ifndef TR_CLI_PROMPT
/**
 * @brief CLI prompt to display after pressing enter.
 */
#define TR_CLI_PROMPT "trident> "
#endif

#ifndef TR_CLI_MAX_PROMPT_LEN
/**
 * @brief Maximum number of bytes in the prompt.
 */
#define TR_CLI_MAX_PROMPT_LEN 10
#endif

#ifndef TR_CLI_SERIAL_XLATE
/**
 * @brief Translate CR -> NL on input and output CR NL on output.
 *
 * This allows "natural" processing when using a serial terminal.
 */
#define TR_CLI_SERIAL_XLATE 1
#endif

#ifndef TR_CLI_LOCAL_ECHO
/**
 * @brief Enable or disable local echo.
 */
#define TR_CLI_LOCAL_ECHO 1
#endif

/**
 * @brief Structure which defines the current state of the CLI.
 *
 * @note Although this structure is exposed here, it is not recommended
 * that it be interacted with directly. Use the accessor functions below to
 * interact with it. It is exposed here to make it easier to use as a static
 * structure, but all elements of the structure should be considered private.
 */
struct tr_cli
{
    /**
     * @brief Internal buffer. This should not be accessed directly, use the
     * access functions below.
     */
    char buffer[TR_CLI_MAX_LINE];

#if TR_CLI_HISTORY_LEN
    /**
     * @brief List of history entries.
     */
    char history[TR_CLI_HISTORY_LEN];

    /**
     * @brief Are we searching through the history?
     */
    bool searching;

    /**
     * @brief How far back in the history are we?
     */
    int history_pos;
#endif

    /**
     * @brief Number of characters in buffer at the moment.
     */
    int len;

    /**
     * @brief Position of the cursor.
     */
    int cursor;

    /**
     * @brief Have we just parsed a full line?
     */
    bool done;

    /**
     * @brief Callback function to output a single character to the user.
     */
    void (*put_char)(char ch);

    /**
     * @brief Data to provide to the put_char callback.
     */
    void *cb_data;

    /**
     * @brief Flag indicating if we have an escape sequence.
     */
    bool have_escape;

    /**
     * @brief Flag indicating if we have a CSI sequence.
     */
    bool have_csi;

    /**
     * @brief Counter of the value for the CSI code.
     */
    int counter;

    /**
     * @brief Array of argument strings.
     */
    char *argv[TR_CLI_MAX_ARGC];

    /**
     * @brief CLI prompt string.
     */
    char prompt[TR_CLI_MAX_PROMPT_LEN];
};

/**
 * @brief Starts up the Embedded CLI subsystem.
 *
 * This should only be called once during system initialization.
 *
 * @param [in] prompt   The prompt string to display.
 * @param [in] put_char Function pointer for character output.
 *
 */
void tr_cli_init(const char *prompt,
                 void       (*put_char)(char ch));

/**
 * @brief Adds a new character into the buffer.
 *
 * @param [in] ch The character to add to the buffer.
 *
 * @return true if the buffer should now be processed.
 *
 * @note This function should not be called from an interrupt handler.
 *
 */
bool tr_cli_insert_char(char ch);

/**
 * @brief Returns the null terminated internal buffer.
 *
 * @return Pointer to the internal buffer, or NULL if the buffer is not yet complete.
 *
 */
const char *tr_cli_get_line(void);

/**
 * @brief Parses the internal buffer and returns it as an argc/argv combo.
 *
 * @param [out] argv Pointer to store the argument vector.
 *
 * @return Number of values in argv (maximum of TR_CLI_MAX_ARGC).
 *
 */
int tr_cli_argc(char ***argv);

/**
 * @brief Outputs the CLI prompt.
 *
 * This should be called after @ref tr_cli_argc or @ref
 * tr_cli_get_line has been called and the command fully processed.
 *
 */
void tr_cli_prompt(void);

/**
 * @brief Retrieves a history command line.
 *
 * @param [in] history_pos 0 is the most recent command, 1 is the one before that, etc.
 *
 * @return Pointer to the history command line, or NULL if the history buffer is exceeded.
 *
 */
const char *tr_cli_get_history(int history_pos);

/**
 * @brief Passes a received character to the CLI.
 *
 * @param [in] data Character received on the terminal.
 *
 */
void tr_cli_char_received(char data);

/**
 * @brief Printf function used for printing to terminal.
 *
 * This function can be re-defined by the application using the CLI
 * to add additional formatting behavior if desired.
 *
 * @param [in] pFormat Printf format string.
 * @param [in] ...     Printf parameters.
 *
 */
void tr_cli_common_printf(const char *pFormat,
                          ...);

/**
 * @} tr-utility-cli-common
 * @} tr-utility-cli
 * @} tr-utility
 */

#endif // TR_CLI_H
