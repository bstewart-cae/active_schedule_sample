/// ****************************************************************************
/// @file remote_cli.c
///
/// @brief Setting up CLI for remote use
///
///
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************

#include <SizeOf.h>
#include "tr_cli.h"
#include "Assert.h" // NOSONAR
#include "DebugPrint.h"
#include "zpal_uart.h"
#include "setup_common_cli.h"
#include "CC_RemoteCLI.h"

//#define DEBUGPRINT // NOSONAR
#include "DebugPrint.h"

// buffer used for vprintf
#define PRINTF_BUFFER_SIZE 256
static uint8_t m_pBuffer[PRINTF_BUFFER_SIZE]; // must be 0 before config

// Setup CLI for Remote CLI CC

void cli_cmd_wait_for_buffer_write(void)
{
  // Not implemented yet
}

void cli_put_char(__attribute__((unused)) char ch)
{
  CC_remote_cli_buffer_data(1, (uint8_t*)&ch);
}

void cli_pass_command_to_cli(uint16_t cmd_length, char* cmd)
{
  DPRINT("Remote CLI - cmd_to_cli\n");
  while (cmd_length)
  {
    tr_cli_char_received(*cmd);
    cmd_length--;
    cmd++;
  }
}

void setup_cli(__attribute__((unused)) zpal_uart_config_t *p_uart_config)
{
  DPRINT("Remote CLI - Init\n");
  CC_remote_cli_set_command_handler(cli_pass_command_to_cli);
  tr_cli_init(TR_CLI_PROMPT, cli_put_char);
}

// Print functions for remote CLI
void tr_cli_common_print(const char* message)
{
  size_t length = strlen(message); //NOSONAR
  // We understand the risk of using strlen here but ignores the security warning
  // because this is a function only used for zero terminated strings
  if (length > 0)
  {
    CC_remote_cli_buffer_data((uint16_t)length, (uint8_t*)message);
  }
}

__attribute__((__format__ (__printf__, 1, 0)))
void tr_cli_common_printf(const char *pFormat, ...) // NOSONAR
{
  va_list pArgs;
  va_start(pArgs, pFormat);

  vsnprintf((char*)m_pBuffer, PRINTF_BUFFER_SIZE, pFormat, pArgs);
  va_end(pArgs);

  tr_cli_common_print((char*)m_pBuffer);
}