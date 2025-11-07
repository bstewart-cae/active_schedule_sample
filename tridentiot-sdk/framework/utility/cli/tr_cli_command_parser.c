/// ****************************************************************************
/// @file tr_cli_command_parser.c
///
/// @brief common CLI utility for parsing incoming commands
///
///
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include "tr_cli.h"
#include "tr_cli_command_parser.h"

extern const tr_command_s general_commands[];

// Forward declaration
int tr_cli_help_handler(const tr_command_s *cmd_table);

int tr_str_cmp(const char *s1,
               const char *s2,
               int        len)
{
    // compare characters, fail as soon as we see a mismatch
    for (uint8_t i = 0 ; i < len ; i++)
    {
        if (tolower(s1[i]) != tolower(s2[i]))
        {
            return -1;
        }
    }
    return 0;
}

// See if there is a command match in the table
static const tr_command_s *tr_find_command(const tr_command_s *cmd_table,
                                           const char         *name)
{
    int                i              = 0;
    int                num_cmds_found = 0;
    const tr_command_s *ret_cmd       = NULL;

    // walk the command table until a null command string is encountered
    while (strlen(cmd_table[i].command))
    {
        // look for first character(s) match
        if (tr_str_cmp(cmd_table[i].command, name, (int)strlen(name)) == 0)
        {
            // we have a match, check to see if it is an exact match
            if (strlen(name) == strlen(cmd_table[i].command))
            {
                // this is an exact match, just return it
                return &cmd_table[i];
            }

            // a least character match was found, save the entry
            ret_cmd = &cmd_table[i];
            num_cmds_found++;
        }
        i++;
    }

    if (num_cmds_found == 1)
    {
        // exactly one match was found, return it
        return ret_cmd;
    }
    // either no matches or multiple matches were found
    return NULL;
}

// Parse the command, calling the handler if there is a command match. If the handler
// is null, use the pointer in the help string as the pointer to the sub-command table.
void tr_cli_parse_command(const tr_command_s *cmd_table,
                          int                argc,
                          char               **argv)
{
    if (argc != 0)
    {
        // see if we can find the command in the table
        const tr_command_s *command_ptr = tr_find_command(cmd_table, argv[0]);

        if (!command_ptr)
        {
            // no matching command was found, but was the command "help"?
            if ((tr_str_cmp(argv[0], "help", 4) == 0) ||
                (strcmp(argv[0], "h") == 0) ||
                (strcmp(argv[0], "H") == 0) ||
                (strcmp(argv[0], "?") == 0))
            {
                // it is help, print the help info
                tr_cli_help_handler(cmd_table);
            }
            else if (cmd_table != general_commands)
            {
                // if there is no match AND this isn't the base general_commands table print the help
                tr_cli_help_handler(cmd_table);
            }
            else
            {
                // there was no command match
                tr_cli_common_printf("  Unknown command: ");
                tr_cli_common_printf("  %s\n", argv[0]);
                tr_cli_common_printf("  Type 'help, ?' to list all commands\n");
            }
        }
        else
        {
            // we have a match, check it for a null handler
            if (command_ptr->handler == NULL)
            {
                // the handler is null, so the help string pointer should be
                // a pointer to the sub-command table
                // re-invoke the parser with the new table
                tr_cli_parse_command((tr_command_s*)command_ptr->help, argc - 1, &argv[1]);
            }
            else
            {
                // call the handler, passing in the arguments
                command_ptr->handler(argc, argv);
            }
        }
    }
    else
    {
        if (cmd_table != general_commands)
        {
            // this isn't the base general_commands table print the help
            tr_cli_help_handler(cmd_table);
        }
    }
}

// Print the help info for the given command table
int tr_cli_help_handler(const tr_command_s *cmd_table)
{
    int i = 0;
    int j;

    // walk the command table until a null command string is encountered
    while (strlen(cmd_table[i].command))
    {
        // print the command name from the table
        tr_cli_common_printf("  %s", cmd_table[i].command);

        // print out spaces to make the help strings line up
        for (j = 0 ; j < 20 - (int)strlen(cmd_table[i].command) ; j++)
        {
            tr_cli_common_printf(" ");
        }

        if (cmd_table[i].handler != NULL)
        {
            // print the help string from the table
            tr_cli_common_printf("%s\n", cmd_table[i].help);
        }
        else
        {
            tr_cli_common_printf("%s commands\n", cmd_table[i].command);
        }
        i++;
    }
    return 0;
}

// This function parses a single option at a time. It will return true if the
// option is found and if an argument is expected, it will return a pointer
// to that argument in ret_arg.
uint8_t tr_cli_get_option(int  argc,
                          char *argv[],
                          char *opt_string,
                          char **ret_arg)
{
    int     opt;
    char    *cp_argv[TR_ARGUMENT_PARSER_MAX_ARGS]; // this is for a copy of argv
    uint8_t ret_val = 0;                           // think about returning different error codes for missing option vs missing arg

    optind = 1;                                    // set the option indicator to the beginning
    opterr = 0;                                    // disable error printing for unkown options

    // we need to make a copy of argv because getopt may reorder the elements causing successive
    // calls to pull out arguments to return erroneous values
    if (argc <= TR_ARGUMENT_PARSER_MAX_ARGS)
    {
        memcpy(cp_argv, argv, sizeof(char*) * (unsigned int)argc);
    }
    else
    {
        memcpy(cp_argv, argv, sizeof(char*) * TR_ARGUMENT_PARSER_MAX_ARGS);
    }

    while ((opt = getopt(argc, cp_argv, opt_string)) != -1)
    {
        // is this the option we are looking for?
        if (opt == opt_string[0])
        {
            // option found, return the argument that goes with it
            if (opt_string[1] == ':')
            {
                // make sure optarg is not a null string
                if ((optarg != NULL) && (optarg[0] != '\0'))
                {
                    // there is an argument supplied, we are good to go
                    *ret_arg = optarg;
                    ret_val  = 1;
                }
            }
            else
            {
                // no argument required, we are good to go
                ret_val = 1;
            }
        }
    }

    optind = 1;
    return ret_val;
}

// convert a decimal or hex string to a number
uint64_t tr_dec_or_hex_string_to_int(const char *number_string)
{
    char *endptr;
    int  base = 10;

    // Check if the input string is valid
    if (number_string == NULL || number_string[0] == '\0')
    {
        // fprintf(stderr, "Invalid input: hexString is NULL or empty.\n");
        return 0;
    }

    // Check if the input string starts with "0x" or "0X" and skip it
    if (number_string[0] == '0' && (number_string[1] == 'x' || number_string[1] == 'X'))
    {
        number_string += 2;
        base           = 16;
    }
    uint64_t result = strtoull(number_string, &endptr, base);

    // Check for conversion errors
    if (*endptr != '\0' && *endptr != '\n')
    {
        // fprintf(stderr, "Conversion error: invalid characters in input.\n");
        return 0;
    }
    return result;
}
