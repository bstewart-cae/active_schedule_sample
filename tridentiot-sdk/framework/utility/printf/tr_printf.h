/// ****************************************************************************
/// @file tr_printf.h
///
/// @brief A simple printf utility for debug prints and cli support.
///        Required for remote CLI to function properly.
///
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************
#ifndef TR_PRINTF_H
#define TR_PRINTF_H

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdarg.h>

/**
 * @addtogroup tr-utility
 * @{
 * @addtogroup tr-utility-printf Printf
 * @brief
 * A simple printf utility for debug prints and cli support.
 *
 * @{
 */

/**
 * @brief Definition to enable float data type formatting
 * Pass in to the build system with a value of 0 to disable
 */
#ifndef PRINTF_FLOAT_ENABLE
#define PRINTF_FLOAT_ENABLE 1
#endif

/**
 * @brief Function pointer definition for put char callback.
 *
 */
typedef void (*putchar_cb)(char ch);

/**
 * @brief Function pointer definition for put string callback.
 *
 */
typedef void (*putstr_cb)(const char *str,
                          int        len);

/**
 * @brief Initializes the printf utility.
 *
 * @param [in] putc Function pointer to a putc function used for writing a single byte.
 * @param [in] puts Function pointer to a puts function used for writing a string of bytes.
 *
 */
void tr_printf_init(putchar_cb putc,
                    putstr_cb  puts);

/**
 * @brief Write formated data from variable argument list to string.
 *
 * @param [in] buf    Pointer to a buffer where the resulting C-string is stored.
 *                    The buffer should be large enough to contain the resulting string.
 * @param [in] maxlen The max len of the buffer.
 * @param [in] fmt    A printf formatted C string.
 * @param [in] args   A value identifying a variable argument list initialized with va_start().
 *
 */
void tr_vsprintf(char          *buf,
                 int           maxlen,
                 const char    *fmt,
                 const va_list args);

/**
 *  @brief  Writes the passed in C string to the standard output (stdout).
 *  @param [in] fmt C string that contains the text to be written to stdout.
 *  @param [in] ... Depends on the format string
 *
 */
void tr_printf(const char *fmt,
               ...);

/**
 * @} tr-utility-printf
 * @} tr-utility
 */

#endif // TR_PRINTF_H
