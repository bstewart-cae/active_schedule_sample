/// ***************************************************************************
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2024 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************

/**
 * @file cli_uart_interface.c
 *
 * @brief Z-Wave radio test CLI tool uart interface
 */

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <Min2Max2.h>

#include <zpal_uart.h>
#include <FreeRTOS.h>
#include <timers.h>
#include <string.h>
#include <Assert.h>
#include "sysctrl.h"

//#define DEBUGPRINT // NOSONAR
#include <DebugPrint.h>
#include <DebugPrintConfig.h>

#include "cli_uart_interface.h"

static zpal_uart_handle_t uart_handler;

// CLI configuration
static EmbeddedCliConfig *config;

// CLI handler
static EmbeddedCli *cli;

// CLI buffer for running with static allocartion
#define CLI_BUFFER_SIZE 1736
CLI_UINT cliBuffer[BYTES_TO_CLI_UINTS(CLI_BUFFER_SIZE)];

// buffer used for vprintf
#define PRINTF_BUFFER_SIZE 256
static uint8_t           m_pBuffer[PRINTF_BUFFER_SIZE]; // must be 0 before config

// Receive char hook for redirecting uart received data.
receiveCharHook_t receiveCharHook = NULL;

/*
 * Uart byte transmit for Embedded Cli
 */
void cli_uart_putc(__attribute__((unused)) EmbeddedCli *embeddedCli, char c)
{
  while (zpal_uart_transmit_in_progress(uart_handler)) ;
  zpal_uart_transmit(uart_handler, &c, 1, NULL);
}

void (*uart_receive_handler)(void);

void receive_callback(__attribute__((unused)) const zpal_uart_handle_t handle, __attribute__((unused)) size_t available)
{
  ASSERT(NULL != uart_receive_handler);
  if (NULL != uart_receive_handler)
  {
    uart_receive_handler();
  }
}

void cli_uart_receive_hook_set(receiveCharHook_t receiveCharFunc)
{
  receiveCharHook = receiveCharFunc;
}

/*
 * Uart receive function for Embedded CLI
 */
void cli_uart_receive_handler(void)
{
  uint8_t byte;
  size_t available = zpal_uart_get_available(uart_handler);
  for (; available; available--)
  {
    zpal_uart_receive(uart_handler, &byte, 1);
    if (NULL != receiveCharHook)
    {
      receiveCharHook(byte);
    }
    else
    {
      embeddedCliReceiveChar(cli, byte);
    }
  }
  /*
  * Service function for the Embedded CLI
  */
  embeddedCliProcess(cli);
}

/*
 * Initialize UART and Embedded CLI
 */
void cli_uart_init(zpal_uart_config_t *uart_config, void (*uart_rx_event_handler)())
{
  // Set up receive callback for uart
  uart_config->receive_callback = receive_callback;

  zpal_status_t status = zpal_uart_init(uart_config, &uart_handler);
  ASSERT(status == ZPAL_STATUS_OK);
  uart_receive_handler = uart_rx_event_handler;
  status = zpal_uart_enable(uart_handler);
  ASSERT(status == ZPAL_STATUS_OK);

  // Initialize the CLI
  config = embeddedCliDefaultConfig();
  ASSERT(config);
  config->rxBufferSize = 256;
  config->cmdBufferSize = 200;
  config->maxBindingCount = 50;
  config->enableAutoComplete = false;
  // The 2 lines below is used to find the required size of CLI_BUFFER_SIZE
  // uint16_t size = embeddedCliRequiredSize(config);
  // cli_uart_printf("Embedded Cli memory requirement (CLI_BUFFER_SIZE) %i", size);
  config->cliBuffer = cliBuffer;
  config->cliBufferSize = CLI_BUFFER_SIZE;
  cli = embeddedCliNew(config);
  ASSERT(cli);
  cli->writeChar = cli_uart_putc;
  DPRINTF("CLI started\n");

  // Add commands to cli
  cli_commands_init(cli);
}

/*
 * Blocking function for printing a string to UART
 */
void cli_uart_print(const char* message)
{
  int32_t length = strlen(message); //NOSONAR
  // We understand the risk of using strlen here but ignores the security warning
  // because this is a function only used for zero terminated strings
  if (length > 0)
  {
    while (zpal_uart_transmit_in_progress(uart_handler)) ;
    zpal_uart_transmit(uart_handler, message, length, NULL);
  }
}

/*
 * Blocking function for printf to UART
 */
__attribute__((__format__ (__printf__, 1, 0)))
void cli_uart_printf(const char* pFormat, ...)
{
  va_list pArgs;
  va_start(pArgs, pFormat);

  // Wait for last print to avoid buffer overwrite
  while (zpal_uart_transmit_in_progress(uart_handler)) ;

  vsnprintf((char*)m_pBuffer, PRINTF_BUFFER_SIZE, pFormat, pArgs);
  va_end(pArgs);

  cli_uart_print(m_pBuffer);
}
