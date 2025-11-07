/// ****************************************************************************
/// @file tr_printf.h
///
/// @brief A simple printf utility for debug prints and cli support.
///        Required for remote CLI to function properly.
///
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************
#include <ctype.h>
#include "tr_printf.h"

#define CONFIG_PRINTF_FMTB_SIZE 32

typedef struct
{
    bool    sign_always;   // Insert '+' or '-' for positive numbers
    bool    sign_space;    // Insert space if no sign is written
    bool    format_prefix; // Add prefix (0x, 0X) for hex values
    bool    left_justify;  // Left-justify within field width
    bool    zero_pad;      // Left-pad with zeros instead of spaces
    int     field_width;   // Minimum field width
    int     precision;     // Precision for floating point/strings
    uint8_t length;        // Length modifier (h, l, L)
} FormatFlags;

typedef struct
{
    putchar_cb putc;
    putstr_cb  puts;
}tr_printf_func_t;

// globals
static tr_printf_func_t g_printf_funcs;


// weakly defined callback that is optionally consumed by the remote CLI plugin
__attribute__((weak)) void remote_cli_server_add_output(const uint8_t *buf,
                                                        uint8_t       len)
{
    (void)buf;
    (void)len;
}

static void tr_printf_stdout_char(char ch)
{
    remote_cli_server_add_output((uint8_t*)&ch, 1);

    if (g_printf_funcs.putc)
    {
        g_printf_funcs.putc(ch);
    }
}

static void tr_printf_stdout_string(const char *str,
                                    int        len)
{
    remote_cli_server_add_output((const uint8_t*)str, len);

    if (g_printf_funcs.puts)
    {
        g_printf_funcs.puts(str, len);
    }
}

static void _fputc(int  ch,
                   char *pBuf,
                   int  *pCurlen,
                   int  maxlen)
{
    do
    {
        if (pBuf)
        {
            if (*pCurlen < maxlen)
            {
                pBuf[*pCurlen] = (char)(ch & 0xFF);
                (*pCurlen)++;
            }
        }
        else
        {
            if (ch == '\n')
            {
                tr_printf_stdout_char('\r');
            }
            tr_printf_stdout_char(ch);
        }
    }
    while (0);
}

static void _fputs(const char *str,
                   uint32_t   length,
                   char       *pBuf,
                   int        *pCurlen,
                   int        maxlen)
{
    do
    {
        if (pBuf)
        {
            while (length-- && *pCurlen < maxlen)
            {
                pBuf[*pCurlen] = (char)((*str++) & 0xFF);
                (*pCurlen)++;
            }
        }
        else
        {
            tr_printf_stdout_string(str, length);
        }
    }
    while (0);
}

static const char *_utility_trim_spaces(const char *str)
{
    while (*str == ' ')
    {
        ++str;
    }

    return str;
}

// hex to ascii
static void tr_xtoa(char          *buf,
                    unsigned long *buflen,
                    unsigned long value,
                    bool          capitalized)
{
    // Validate input parameters
    if (!buf || !buflen || *buflen == 0)
    {
        if (buflen)
        {
            *buflen = 0;
        }
        return;
    }

    // Constants for hex conversion
    const unsigned HEX_BITS    = 4;
    const unsigned BITS_TOTAL  = 8 * sizeof(unsigned long);
    const char     HEX_LOWER[] = "0123456789abcdef";
    const char     HEX_UPPER[] = "0123456789ABCDEF";
    const char     *hex_chars  = capitalized ? HEX_UPPER : HEX_LOWER;

    // Handle zero case
    if (value == 0)
    {
        if (*buflen < 2)
        {
            *buflen = 0;
            return;
        }
        buf[0]  = '0';
        buf[1]  = '\0';
        *buflen = 1;
        return;
    }

    char          *pos         = buf;
    const char    *end         = buf + *buflen - 1; // Reserve space for null terminator
    bool          skip_leading = true;
    unsigned long written      = 0;

    // Process each hex digit
    for (long bit_pos = BITS_TOTAL - HEX_BITS ; bit_pos >= 0 && pos < end ; bit_pos -= HEX_BITS)
    {
        uint8_t digit = (uint8_t)(value >> bit_pos) & 0x0F;

        if (skip_leading && digit == 0)
        {
            continue;
        }

        if (skip_leading)
        {
            skip_leading = false;
        }

        *pos++ = hex_chars[digit];
        written++;
    }

    // Always null terminate if there's space
    if (pos <= end)
    {
        *pos = '\0';
    }
    else
    {
        buf[*buflen - 1] = '\0';
        written          = *buflen - 1;
    }

    *buflen = written;
}

// unsigned long to ascii
static void tr_ultoa(char          *a,
                     unsigned long *len,
                     unsigned long u)
{
    char *iter_b = a;
    char *iter_e = a + *len;
    char *iter   = iter_e - 1;

    while (iter >= iter_b)
    {
        *iter-- = (char)('0' + (uint8_t)(u % 10));

        u = u / 10;

        if (u == 0)
        {
            break;    // no more digits to extract
        }
    }

    while (++iter < iter_e)
    {
        *iter_b++ = *iter;
    }

    if (iter_b < iter_e)
    {
        *iter_b = 0x00;
    }
    *len = (unsigned long)(iter_b - a);  // not including null-terminating char
}

// signed long to ascii
static void tr_ltoa(char          *a,
                    unsigned long *len,
                    long          l,
                    bool          w_sign)
{
    char *iter_b = a;
    char *iter_e = a + *len;
    char *iter   = iter_e - 1;

    if (iter_b < iter_e)
    {
        if (l < 0)
        {
            l         = -l;
            *iter_b++ = '-';
        }
        else if (w_sign)
        {
            *iter_b++ = '+';
        }
    }

    while (iter >= iter_b)
    {
        *iter-- = (char)('0' + (uint8_t)(l % 10));

        l = l / 10;

        if (l == 0)
        {
            break;    // no more digits to extract
        }
    }

    while (++iter < iter_e)
    {
        *iter_b++ = *iter;
    }

    if (iter_b < iter_e)
    {
        *iter_b = 0x00;
    }
    *len = (unsigned long)(iter_b - a);  // not including null-terminating char
}

#if defined(PRINTF_FLOAT_ENABLE) && PRINTF_FLOAT_ENABLE == 1

// float to ascii
static void tr_ftoa(char          *a,
                    unsigned long *len,
                    double        f,
                    unsigned long frc_precision)
{
    char       *iter     = a;
    const char *iter_e   = iter + *len;
    double     round_val = 0.5; // round-up addition val
    double     frc;
    long       fint;

    // rounding up at the least significant digit
    if (frc_precision > 0)
    {
        unsigned long pp  = frc_precision;
        double        div = 1.0;

        while (pp-- > 0)
        {
            div *= 10.0;
        }
        round_val /= div;
    }

    if (f >= 0)
    {
        f = f + round_val;
    }
    else
    {
        f = f - round_val;
    }

    // make the integer part
    fint = (long)f;
    frc  = f - (double)fint;

    if ((f < 0) && (fint == 0))
    {
        // a positive frc value is needed for the following conversion loop
        frc *= -1;

        // if "fint" is "0" the sign of "f" will be lost if "f" is negtive
        if (iter < iter_e)
        {
            *iter++ = '-';
            --(*len);
        }
        tr_ltoa(iter, len, fint, false);
    }
    else
    {
        tr_ltoa(iter, len, fint, false);
    }

    iter += *len;

    if ((iter >= iter_e) || (frc_precision <= 0))
    {
        return;
    }

    *iter++ = '.';

    // handle the fraction part
    while (iter < iter_e && frc_precision > 0)
    {
        frc_precision--;
        frc  = frc * 10.0f;
        fint = (long)frc;
        frc  = frc - (double)fint;

        *iter++ = (char)('0' + fint);
    }

    if (iter < iter_e)
    {
        *iter = 0x00;
    }
    *len = (unsigned long)(iter - a);  // not including null-terminating char
}

#endif /* if defined(PRINTF_FLOAT_ENABLE) && PRINTF_FLOAT_ENABLE == 1 */

// string to long
static long tr_strtol(const char *str,
                      const char **ep)
{
    long       v    = 0;
    long       sign = 1;
    const char *cur = _utility_trim_spaces(str);

    // Handle sign
    if (*cur == '-')
    {
        sign = -1;
        ++cur;
    }
    else if (*cur == '+')
    {
        ++cur;
    }

    // Convert digits
    for (long nr_digits = 0 ; nr_digits < 10 ; ++nr_digits)
    {
        if ((*cur < '0') || (*cur > '9'))
        {
            break;
        }
        v = v * 10 + (long)(*cur++ - '0');
    }

    // Set end pointer if requested
    if (ep)
    {
        *ep = cur;
    }

    return sign * v;
}

static void init_format_flags(FormatFlags *flags)
{
    flags->sign_always   = false;
    flags->sign_space    = false;
    flags->format_prefix = false;
    flags->left_justify  = false;
    flags->zero_pad      = false;
    flags->field_width   = -1;
    flags->precision     = -1;
    flags->length        = 0;
}

// Helper function to parse format flags
static const char *parse_format_flags(const char  *fmt,
                                      FormatFlags *flags)
{
    while (*fmt)
    {
        switch (*fmt)
        {
            case '+':
                flags->sign_always = true;
                break;

            case '-':
                flags->left_justify = true;
                break;

            case ' ':
                flags->sign_space = true;
                break;

            case '0':
                flags->zero_pad = true;
                break;

            case '#':
                flags->format_prefix = true;
                break;

            default:
                return fmt;
        }
        fmt++;
    }
    return fmt;
}

// Helper function to parse field width and precision
static const char *parse_width_precision(const char    *fmt,
                                         FormatFlags   *flags,
                                         const va_list *args)
{
    // Parse field width
    if (*fmt == '*')
    {
        flags->field_width = va_arg(*args, int);
        fmt++;
    }
    else if (isdigit((unsigned char)*fmt))
    {
        const char *end;
        flags->field_width = tr_strtol(fmt, &end);
        fmt                = end;
    }

    // Parse precision
    if (*fmt == '.')
    {
        fmt++;

        if (*fmt == '*')
        {
            flags->precision = va_arg(*args, int);
            fmt++;
        }
        else if (isdigit((unsigned char)*fmt))
        {
            const char *end;
            flags->precision = tr_strtol(fmt, &end);
            fmt              = end;
        }
        else
        {
            flags->precision = 0;
        }
    }

    return fmt;
}

// Helper function to parse length modifiers
static const char *parse_length_modifier(const char  *fmt,
                                         FormatFlags *flags)
{
    switch (*fmt)
    {
        case 'h':
            flags->length |= 1;
            fmt++;
            break;

        case 'l':
            flags->length |= 2;
            fmt++;
            break;

        case 'L':
            flags->length |= 4;
            fmt++;
            break;

        default:
    }
    return fmt;
}

// Helper function to handle padding
static void handle_padding(char *buf,
                           int  *curlen,
                           int  maxlen,
                           int  count,
                           char pad_char)
{
    for (int i = 0 ; i < count ; i++)
    {
        _fputc(pad_char, buf, curlen, maxlen);
    }
}

// Helper function to handle string output with padding
static void format_string(char              *buf,
                          int               *curlen,
                          int               maxlen,
                          const char        *str,
                          const FormatFlags *flags)
{
    long str_len = str ? strlen(str) : 0;
    int  padding = 0;

    if (flags->precision > (int)str_len)
    {
        padding = flags->precision - str_len;
    }
    else if (flags->zero_pad && flags->field_width > (int)str_len)
    {
        padding = flags->field_width - str_len;
    }

    if (!flags->zero_pad && !flags->left_justify)
    {
        handle_padding(buf, curlen, maxlen, flags->field_width - str_len - padding, ' ');
    }

    handle_padding(buf, curlen, maxlen, padding, '0');
    _fputs(str, str_len, buf, curlen, maxlen);

    if (flags->left_justify)
    {
        handle_padding(buf, curlen, maxlen, flags->field_width - str_len - padding, ' ');
    }
}

/************************************************************************************/
/**                                Public Functions                                **/
/************************************************************************************/

void tr_printf_init(putchar_cb putc,
                    putstr_cb  puts)
{
    memset(&g_printf_funcs, 0, sizeof(g_printf_funcs));
    g_printf_funcs.putc = putc;
    g_printf_funcs.puts = puts;
}

void tr_vsprintf(char          *buf,
                 int           maxlen,
                 const char    *fmt,
                 const va_list args)
{
    int         curlen = 0;
    char        temp_buf[CONFIG_PRINTF_FMTB_SIZE];
    FormatFlags flags;

    while (*fmt)
    {
        if (*fmt != '%')
        {
            _fputc(*fmt++, buf, &curlen, maxlen);
            continue;
        }

        fmt++; // Skip '%'

        if (*fmt == '%')
        {
            _fputc(*fmt++, buf, &curlen, maxlen);
            continue;
        }

        // Initialize format flags for new format specifier
        init_format_flags(&flags);

        // Parse format specifier
        fmt = parse_format_flags(fmt, &flags);
        fmt = parse_width_precision(fmt, &flags, &args);
        fmt = parse_length_modifier(fmt, &flags);

        // Handle format specifier
        switch (*fmt)
        {
            case 'c':
            {
                char c = (char)va_arg(args, int);

                if (!flags.left_justify)
                {
                    handle_padding(buf, &curlen, maxlen, flags.field_width - 1, ' ');
                }
                _fputc(c, buf, &curlen, maxlen);

                if (flags.left_justify)
                {
                    handle_padding(buf, &curlen, maxlen, flags.field_width - 1, ' ');
                }
                break;
            }

            case 's':
            {
                const char *str = va_arg(args, char*);
                format_string(buf, &curlen, maxlen, str, &flags);
                break;
            }

            case 'd':
            case 'i':
            {
                long value = (flags.length & 2) ?
                             va_arg(args, long) : va_arg(args, int);
                unsigned long str_len = CONFIG_PRINTF_FMTB_SIZE;

                if (flags.precision == 0 && value == 0)
                {
                    break;
                }

                tr_ltoa(temp_buf, &str_len, value, false);
                format_string(buf, &curlen, maxlen, temp_buf, &flags);
                break;
            }

            case 'u':
            {
                unsigned long value = (flags.length & 2) ?
                                      va_arg(args, unsigned long) : va_arg(args, unsigned int);
                unsigned long str_len = CONFIG_PRINTF_FMTB_SIZE;

                if (flags.precision == 0 && value == 0)
                {
                    break;
                }

                tr_ultoa(temp_buf, &str_len, value);
                format_string(buf, &curlen, maxlen, temp_buf, &flags);
                break;
            }

            case 'x':
            case 'X':
            {
                unsigned long value = (flags.length & 2) ?
                                      va_arg(args, unsigned long) : va_arg(args, unsigned int);
                unsigned long str_len = CONFIG_PRINTF_FMTB_SIZE;

                if (flags.precision == 0 && value == 0)
                {
                    break;
                }

                // Convert to hex with appropriate case
                tr_xtoa(temp_buf, &str_len, value, *fmt == 'X');

                int total_width = str_len;

                if (flags.format_prefix && value != 0)
                {
                    total_width += 2;  // For "0x" or "0X" prefix
                }

                int padding = 0;

                if (flags.precision > (int)str_len)
                {
                    padding      = flags.precision - str_len;
                    total_width += padding;
                }

                // Left padding with spaces if needed
                if (!flags.zero_pad && !flags.left_justify)
                {
                    handle_padding(buf, &curlen, maxlen, flags.field_width - total_width, ' ');
                }

                // Add prefix if requested
                if (flags.format_prefix && value != 0)
                {
                    _fputc('0', buf, &curlen, maxlen);
                    _fputc(*fmt, buf, &curlen, maxlen);  // 'x' or 'X'
                }

                // Zero padding for field if needed
                if ((flags.zero_pad) && (flags.field_width > 0))
                {
                    padding = flags.field_width - total_width;
                    handle_padding(buf, &curlen, maxlen, padding, '0');
                }

                // Add zero padding for precision if needed
                if (flags.precision > (int)str_len)
                {
                    handle_padding(buf, &curlen, maxlen, flags.precision - str_len, '0');
                }

                // Value
                _fputs(temp_buf, str_len, buf, &curlen, maxlen);

                // Right padding if needed
                if (flags.left_justify)
                {
                    handle_padding(buf, &curlen, maxlen, flags.field_width - total_width, ' ');
                }
                break;
            }

            case 'p':
            {
                void          *ptr    = va_arg(args, void*);
                unsigned long str_len = CONFIG_PRINTF_FMTB_SIZE;

                tr_xtoa(temp_buf, &str_len, (unsigned long)ptr, true);

                // Always prefix with 0x for pointers
                _fputs("0x", 2, buf, &curlen, maxlen);
                _fputs(temp_buf, str_len, buf, &curlen, maxlen);
                break;
            }

#if defined(PRINTF_FLOAT_ENABLE) && PRINTF_FLOAT_ENABLE == 1
            case 'f':
            case 'g':
            case 'G':
            case 'e':
            case 'E':
            {
                double        value   = va_arg(args, double);
                unsigned long str_len = CONFIG_PRINTF_FMTB_SIZE;

                if (flags.precision == 0 && value == 0.0)
                {
                    break;
                }

                // Use default precision of 6 if not specified
                int precision = (flags.precision >= 0) ? flags.precision : 6;

                tr_ftoa(temp_buf, &str_len, value, precision);

                // Handle sign
                int  total_width = str_len;
                bool need_sign   = (value < 0.0 || flags.sign_always || flags.sign_space);

                if (need_sign)
                {
                    total_width++;
                }

                // Left padding with spaces
                if (!flags.zero_pad && !flags.left_justify)
                {
                    handle_padding(buf, &curlen, maxlen, flags.field_width - total_width, ' ');
                }

                // Output sign if needed
                if (value < 0.0)
                {
                    _fputc('-', buf, &curlen, maxlen);
                }
                else if (flags.sign_always)
                {
                    _fputc('+', buf, &curlen, maxlen);
                }
                else if (flags.sign_space)
                {
                    _fputc(' ', buf, &curlen, maxlen);
                }

                // Zero padding if needed
                if (flags.zero_pad && !flags.left_justify)
                {
                    handle_padding(buf, &curlen, maxlen, flags.field_width - total_width, '0');
                }

                // Value
                _fputs(temp_buf, str_len, buf, &curlen, maxlen);

                // Right padding if needed
                if (flags.left_justify)
                {
                    handle_padding(buf, &curlen, maxlen, flags.field_width - total_width, ' ');
                }
                break;
            }
#endif /* if defined(PRINTF_FLOAT_ENABLE) && PRINTF_FLOAT_ENABLE == 1 */
        }

        fmt++;
    }

    // Ensure null termination
    if (maxlen > 0)
    {
        if (curlen < (maxlen - 1))
        {
            buf[curlen] = '\0';
        }
        else
        {
            buf[maxlen - 1] = '\0';
        }
    }
}

void tr_printf(const char *fmt,
               ...)
{
    va_list va;
    va_start(va, fmt);
    tr_vsprintf(0, 0, fmt, va);
    va_end(va);
}

// must define this to make compiler happy
int _write(int        fd,
           const char *ptr,
           int        len)
{
    (void)fd;

    for (int i = 0 ; i < len ; i++)
    {
        if (ptr[i] == 0x0a)
        {
            tr_printf_stdout_char(0x0d);
        }
        tr_printf_stdout_char(ptr[i]);
    }

    return len;
}

// must define this to make compiler happy
int _read(int        fd,
          const char *ptr,
          int        len)
{
    (void)fd;
    (void)*ptr;
    (void)len;
    return 1;
}
