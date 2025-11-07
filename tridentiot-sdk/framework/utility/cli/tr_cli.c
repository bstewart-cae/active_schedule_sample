/// ****************************************************************************
/// @file tr_cli.c
///
/// @brief common CLI utility
///
///
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************
#include <stdio.h>
#include <string.h>

#include "tr_cli.h"

#define CTRL_R    0x12

#define CLEAR_EOL "\x1b[0K"
#define MOVE_BOL  "\x1b[1G"

extern const tr_command_s general_commands[];

static struct tr_cli cli;

static void cli_putchar(char ch)
{
#if TR_CLI_SERIAL_XLATE

    if (ch == '\n')
    {
        cli.put_char('\r');
    }
#endif
    cli.put_char(ch);
}

static void cli_puts(const char *s)
{
    for ( ; *s ; s++)
    {
        cli_putchar(*s);
    }
}

static void tr_cli_reset_line(void)
{
    cli.len      = 0;
    cli.cursor   = 0;
    cli.counter  = 0;
    cli.have_csi = cli.have_escape = false;
#if TR_CLI_HISTORY_LEN
    cli.history_pos = -1;
    cli.searching   = false;
#endif
}

/***********************/
void tr_cli_init(const char *prompt,
                 void (     *put_char )(char ch))
{
    // uint8_t data;

    memset((void*)&cli, 0, sizeof(cli));

    if (prompt)
    {
        strncpy(cli.prompt, prompt, sizeof(cli.prompt));
        cli.prompt[sizeof(cli.prompt) - 1] = '\0';
    }

    tr_cli_reset_line();
    cli.put_char = put_char;

    tr_cli_prompt();
}

static void cli_ansi(int  n,
                     char code)
{
    char buffer[5] = { '\x1b', '[', (char)('0' + (n % 10)), code, '\0' };
    cli_puts(buffer);
}

static void term_cursor_back(int n)
{
    while (n > 0)
    {
        int count = n > 9 ? 9 : n;
        cli_ansi(count, 'D');
        n -= count;
    }
}

static void term_cursor_fwd(int n)
{
    while (n > 0)
    {
        int count = n > 9 ? 9 : n;
        cli_ansi(count, 'C');
        n -= count;
    }
}

#if TR_CLI_HISTORY_LEN

static void term_backspace(int n)
{
    // printf("backspace %d ('%s': %d)\n", n, cli.buffer, cli.done);
    while (n--)
    {
        cli_putchar('\b');
    }
}

static const char *tr_cli_get_history_search(void)
{
    for (int i = 0 ; ; i++)
    {
        const char *h = tr_cli_get_history(i);

        if (!h)
        {
            return NULL;
        }

        if (strstr(h, cli.buffer))
        {
            return h;
        }
    }
    return NULL;
}

#endif /* if TR_CLI_HISTORY_LEN */

static void tr_cli_insert_default_char(char ch)
{
    // If the buffer is full, there's nothing we can do
    if (cli.len >= (int)sizeof(cli.buffer) - 1)
    {
        return;
    }
    // Insert a gap in the buffer for the new character
    memmove(&cli.buffer[cli.cursor + 1],
            &cli.buffer[cli.cursor],
            (size_t)(cli.len - cli.cursor));
    cli.buffer[cli.cursor] = ch;
    cli.len++;
    cli.buffer[cli.len] = '\0';
    cli.cursor++;

#if TR_CLI_HISTORY_LEN

    if (cli.searching)
    {
        cli_puts(MOVE_BOL CLEAR_EOL "search:");
        const char *h = tr_cli_get_history_search();

        if (h)
        {
            cli_puts(h);
        }
    }
    else
#endif /* if TR_CLI_HISTORY_LEN */
    {
#if TR_CLI_LOCAL_ECHO
        cli_puts(&cli.buffer[cli.cursor - 1]);
#endif
        term_cursor_back(cli.len - cli.cursor);
    }
}

const char *tr_cli_get_history(int history_pos)
{
#if TR_CLI_HISTORY_LEN
    int pos = 0;

    if (history_pos < 0)
    {
        return NULL;
    }

    // Search back through the history buffer for `history_pos` entry
    for (int i = 0 ; i < history_pos ; i++)
    {
        int len = (int)strlen(&cli.history[pos]);

        if (len == 0)
        {
            return NULL;
        }
        pos += len + 1;

        if (pos == sizeof(cli.history))
        {
            return NULL;
        }
    }

    return &cli.history[pos];
#else  /* if TR_CLI_HISTORY_LEN */
    (void)cli;
    (void)history_pos;
    return NULL;
#endif /* if TR_CLI_HISTORY_LEN */
}

#if TR_CLI_HISTORY_LEN

static void tr_cli_extend_history(void)
{
    int len = (int)strlen(cli.buffer);

    if (len > 0)
    {
        // If the new entry is the same as the most recent history entry,
        // then don't insert it
        if (strcmp(cli.buffer, cli.history) == 0)
        {
            return;
        }
        memmove(&cli.history[len + 1],
                &cli.history[0],
                sizeof(cli.history) - (unsigned int)len + 1);
        memcpy(cli.history, cli.buffer, (size_t)(len + 1));
        // Make sure it's always nul terminated
        cli.history[sizeof(cli.history) - 1] = '\0';
    }
}

static void tr_cli_stop_search(bool print)
{
    const char *h = tr_cli_get_history_search();

    if (h)
    {
        strncpy(cli.buffer, h, sizeof(cli.buffer));
        cli.buffer[sizeof(cli.buffer) - 1] = '\0';
    }
    else
    {
        cli.buffer[0] = '\0';
    }
    cli.len       = cli.cursor = (int)strlen(cli.buffer);
    cli.searching = false;

    if (print)
    {
        cli_puts(MOVE_BOL CLEAR_EOL);
        cli_puts(cli.prompt);
        cli_puts(cli.buffer);
    }
}

#endif /* if TR_CLI_HISTORY_LEN */

bool tr_cli_insert_char(char ch)
{
    // If we're inserting a character just after a finished line, clear things
    // up
    if (cli.done)
    {
        cli.buffer[0] = '\0';
        cli.done      = false;
    }

    // printf("Inserting char %d 0x%x '%c'\n", ch, ch, ch);
    if (cli.have_csi)
    {
        if (ch >= '0' && ch <= '9' && cli.counter < 100)
        {
            cli.counter = cli.counter * 10 + ch - '0';
            // printf("cli.counter -> %d\n", cli.counter);
        }
        else
        {
            if (cli.counter == 0)
            {
                cli.counter = 1;
            }

            switch (ch)
            {
                case 'A':
                {
#if TR_CLI_HISTORY_LEN
                    // Backspace over our current line
                    term_backspace(cli.done ? 0 : cli.cursor);
                    const char *line =
                        tr_cli_get_history(cli.history_pos + 1);

                    if (line)
                    {
                        int len = (int)strlen(line);
                        cli.history_pos++;
                        // printf("history up %d = '%s'\n", cli.history_pos,
                        // line);
                        strncpy(cli.buffer, line, sizeof(cli.buffer));
                        cli.buffer[sizeof(cli.buffer) - 1] = '\0';
                        cli.len                            = len;
                        cli.cursor                         = len;
                        cli_puts(cli.buffer);
                        cli_puts(CLEAR_EOL);
                    }
                    else
                    {
                        int tmp = cli.history_pos; // We don't want to wrap this
                                                   // history, so retain it
                        cli.buffer[0] = '\0';
                        tr_cli_reset_line();
                        cli.history_pos = tmp;
                        cli_puts(CLEAR_EOL);
                    }
#endif /* if TR_CLI_HISTORY_LEN */
                    break;
                }

                case 'B':
                {
#if TR_CLI_HISTORY_LEN
#ifndef NEW_CLI_FIXES
                    term_backspace(cli.done ? 0 : (int)strlen(cli.buffer));
#else
                    term_backspace(cli.done ? 0 : cli.cursor);
#endif
                    const char *line =
                        tr_cli_get_history(cli.history_pos - 1);

                    if (line)
                    {
                        int len = (int)strlen(line);
                        cli.history_pos--;
                        // printf("history down %d = '%s'\n",
                        // cli.history_pos, line);
                        strncpy(cli.buffer, line, sizeof(cli.buffer));
                        cli.buffer[sizeof(cli.buffer) - 1] = '\0';
                        cli.len                            = len;
                        cli.cursor                         = len;
                        cli_puts(cli.buffer);
                        cli_puts(CLEAR_EOL);
                    }
                    else
                    {
                        cli.buffer[0] = '\0';
                        tr_cli_reset_line();
                        cli_puts(CLEAR_EOL);
                    }
#endif /* if TR_CLI_HISTORY_LEN */
                    break;
                }

                case 'C':
                    if (cli.cursor <= cli.len - cli.counter)
                    {
                        cli.cursor += cli.counter;
                        term_cursor_fwd(cli.counter);
                    }
                    break;

                case 'D':
                    // printf("back %d vs %d\n", cli.cursor, cli.counter);
                    if (cli.cursor >= cli.counter)
                    {
                        cli.cursor -= cli.counter;
                        term_cursor_back(cli.counter);
                    }
                    break;

                case 'F':
                    term_cursor_fwd(cli.len - cli.cursor);
                    cli.cursor = cli.len;
                    break;

                case 'H':
                    term_cursor_back(cli.cursor);
                    cli.cursor = 0;
                    break;

                case '~':
                    if (cli.counter == 3) // delete key
                    {
                        if (cli.cursor < cli.len)
                        {
                            memmove(&cli.buffer[cli.cursor],
                                    &cli.buffer[cli.cursor + 1],
                                    (size_t)(cli.len - cli.cursor));
                            cli.len--;
                            cli_puts(&cli.buffer[cli.cursor]);
                            cli_puts(" ");
                            term_cursor_back(cli.len - cli.cursor + 1);
                        }
                    }
                    break;

                default:
                    // TODO: Handle more escape sequences
                    break;
            }
            cli.have_csi = cli.have_escape = false;
            cli.counter  = 0;
        }
    }
    else
    {
        switch (ch)
        {
            case '\0':
                break;

            case '\x01':
                // Go to the beginning of the line
                term_cursor_back(cli.cursor);
                cli.cursor = 0;
                break;

            case '\x03':
                cli_puts("^C\n");
                cli_puts(cli.prompt);
                tr_cli_reset_line();
                cli.buffer[0] = '\0';
                break;

            case '\x05': // Ctrl-E
                term_cursor_fwd(cli.len - cli.cursor);
                cli.cursor = cli.len;
                break;

            case '\x0b': // Ctrl-K
                cli_puts(CLEAR_EOL);
                cli.buffer[cli.cursor] = '\0';
                cli.len                = cli.cursor;
                break;

            case '\x0c': // Ctrl-L
                cli_puts(MOVE_BOL CLEAR_EOL);
                cli_puts(cli.prompt);
                cli_puts(cli.buffer);
                term_cursor_back(cli.len - cli.cursor);
                break;

            case '\b': // Backspace
            case 0x7f: // backspace?
                       // printf("backspace %d\n", cli.cursor);
#if TR_CLI_HISTORY_LEN

                if (cli.searching)
                {
                    tr_cli_stop_search(true);
                }
#endif

                if (cli.cursor > 0)
                {
                    memmove(&cli.buffer[cli.cursor - 1],
                            &cli.buffer[cli.cursor],
                            (size_t)(cli.len - cli.cursor + 1));
                    cli.cursor--;
                    cli.len--;
                    term_cursor_back(1);
                    cli_puts(&cli.buffer[cli.cursor]);
                    cli_puts(" ");
                    term_cursor_back(cli.len - cli.cursor + 1);
                }
                break;

            case CTRL_R:
#if TR_CLI_HISTORY_LEN

                if (!cli.searching)
                {
                    cli_puts("\nsearch:");
                    cli.searching = true;
                }
#endif
                break;

            case '\x1b':
#if TR_CLI_HISTORY_LEN

                if (cli.searching)
                {
                    tr_cli_stop_search(true);
                }
#endif
                cli.have_csi    = false;
                cli.have_escape = true;
                cli.counter     = 0;
                break;

            case '[':
                if (cli.have_escape)
                {
                    cli.have_csi = true;
                }
                else
                {
                    tr_cli_insert_default_char(ch);
                }
                break;

#if TR_CLI_SERIAL_XLATE
            case '\r':
                ch = '\n'; // So cli.done will exit

#endif
            // fallthrough
            case '\n':
                cli_putchar('\n');
                break;

            default:
                if (ch > 0)
                {
                    tr_cli_insert_default_char(ch);
                }
        }
    }
    cli.done = (ch == '\n');

    if (cli.done)
    {
#if TR_CLI_HISTORY_LEN

        if (cli.searching)
        {
            tr_cli_stop_search(false);
        }
        tr_cli_extend_history();
#endif
        tr_cli_reset_line();
    }
    // printf("Done with char 0x%x (done=%d)\n", ch, cli.done);
    return cli.done;
}

const char *tr_cli_get_line(void)
{
    if (!cli.done)
    {
        return NULL;
    }
    return cli.buffer;
}

static bool is_whitespace(char ch)
{
    return (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r');
}

int tr_cli_argc(char ***argv)
{
    int  pos       = 0;
    bool in_arg    = false;
    bool in_escape = false;
    char in_string = '\0';

    if (!cli.done)
    {
        return 0;
    }

    for (size_t i = 0 ; i < sizeof(cli.buffer) && cli.buffer[i] != '\0' ; i++)
    {

        // If we're escaping this character, just absorb it regardless
        if (in_escape)
        {
            in_escape = false;
            continue;
        }

        if (in_string)
        {
            // If we're finishing a string, blank it out
            if (cli.buffer[i] == in_string)
            {
                memmove(&cli.buffer[i],
                        &cli.buffer[i + 1],
                        sizeof(cli.buffer) - i - 1);
                in_string = '\0';
                i--;
            }
            continue;
        }

        // Skip over whitespace, and replace it with nul terminators so
        // each argv is nul terminated
        if (is_whitespace(cli.buffer[i]))
        {
            if (in_arg)
            {
                cli.buffer[i] = '\0';
            }
            in_arg = false;
            continue;
        }

        if (!in_arg)
        {
            if (pos >= TR_CLI_MAX_ARGC)
            {
                break;
            }
            cli.argv[pos] = &cli.buffer[i];
            pos++;
            in_arg = true;
        }

        if (cli.buffer[i] == '\\')
        {
            // Absorb the escape character
            memmove(&cli.buffer[i],
                    &cli.buffer[i + 1],
                    sizeof(cli.buffer) - i - 1);
            i--;
            in_escape = true;
        }

        // If we're starting a new string, absorb the character and shuffle
        // things back
        if (cli.buffer[i] == '\'' || cli.buffer[i] == '"')
        {
            in_string = cli.buffer[i];
            memmove(&cli.buffer[i],
                    &cli.buffer[i + 1],
                    sizeof(cli.buffer) - i - 1);
            i--;
        }
    }

    // Traditionally, there is a NULL entry at argv[argc].
    if (pos >= TR_CLI_MAX_ARGC)
    {
        pos--;
    }
    cli.argv[pos] = NULL;

    *argv = cli.argv;
    return pos;
}

void tr_cli_prompt(void)
{
    cli_puts(cli.prompt);
}

// we received a character! it will arrive here
void tr_cli_char_received(char data)
{
    if (tr_cli_insert_char(data))
    {
        int  cli_argc;
        char **cli_argv;
        cli_argc = tr_cli_argc(&cli_argv);
        tr_cli_parse_command(general_commands, cli_argc, cli_argv);
        tr_cli_prompt();
    }
}

__attribute__((weak)) void tr_cli_common_printf(const char *pFormat,
                                                ...)
{
    va_list va;
    va_start(va, pFormat);
    tr_vsprintf(0, 0, pFormat, va);
    va_end(va);
}
