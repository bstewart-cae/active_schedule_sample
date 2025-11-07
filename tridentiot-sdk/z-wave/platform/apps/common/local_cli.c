/// ****************************************************************************
/// @file local_cli.c
///
/// @brief Setting up cli for local use
///
///
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************

#include <SizeOf.h>
#include "zpal_uart.h"
#include "zpal_misc.h"
#include "zpal_misc_private.h"
#include "ZAF_ApplicationEvents.h"
#include "zaf_event_distributor_soc.h"
#include "events.h"
#include "ZAF_Common_helper.h"
#include "tr_cli.h"
#include "Assert.h" // NOSONAR
#include "setup_common_cli.h"

//#define DEBUGPRINT // NOSONAR
#include "DebugPrint.h"

// buffer used for vprintf
#define PRINTF_BUFFER_SIZE 256
static uint8_t m_pBuffer[PRINTF_BUFFER_SIZE]; // must be 0 before config

// UART vars
static zpal_uart_handle_t uart_handle = NULL;

#define COMM_INT_TX_BUFFER_SIZE 128 // ok\r\n + Git commit hash is 40 characters long + \r\n.
#define COMM_INT_RX_BUFFER_SIZE 64

static uint8_t tx_data[COMM_INT_TX_BUFFER_SIZE];
static uint8_t rx_data[COMM_INT_RX_BUFFER_SIZE];

// HW Event queue vars
#define HW_QUEUE_ELEMENT_SIZE 5
static SQueueNotifying m_HWEventNotifyingQueue = { 0 };
static StaticQueue_t m_HWEventQueueObject = { 0 };
static uint8_t m_HWEventQueueStorage[HW_QUEUE_ELEMENT_SIZE];
static QueueHandle_t m_HWEventQueue = { 0 };

// HW event numbers
typedef enum _HW_EVENTS_
{
  HW_NO_EVENT = 0,
  HW_EVENT_UART_RECEIVE,
} HW_EVENTS;

// Setup CLI for uart

void cli_cmd_wait_for_buffer_write(void)
{
  while (zpal_uart_transmit_in_progress(uart_handle))
      ;
}

void cli_process_bytes(void)
{
  uint8_t byte;
  size_t available = zpal_uart_get_available(uart_handle);
  for (; available; available--)
  {
    zpal_uart_receive(uart_handle, &byte, 1);
    tr_cli_char_received(byte);
  }
}

void EventHandlerHW(void)
{
  uint8_t event = HW_NO_EVENT;
  while (xQueueReceive(m_HWEventQueue, &event, 0) == pdTRUE) {
    DPRINTF("Event: %d\r\n", event);
    switch (event) { // NOSONAR
      case HW_EVENT_UART_RECEIVE:
        cli_process_bytes();
        break;

      default:
        break;
    }
  }
}

bool event_distributor_enqueue_hw_event_from_isr(const uint8_t event)
{
  EQueueNotifyingStatus Status;

  Status = QueueNotifyingSendToBackFromISR(&m_HWEventNotifyingQueue, &event);
  if (Status != EQUEUENOTIFYING_STATUS_SUCCESS) {
    DPRINT("Failed to queue event\n");
    return false;
  }

  return true;
}

bool event_distributor_enqueue_hw_event(const uint8_t event)
{
  EQueueNotifyingStatus Status;

  Status = QueueNotifyingSendToBack(&m_HWEventNotifyingQueue, &event, 0);
  if (Status != EQUEUENOTIFYING_STATUS_SUCCESS) {
    DPRINT("Failed to queue event\n");
    return false;
  }

  return true;
}

void cli_uart_rx_callback(__attribute__((unused)) zpal_uart_handle_t handle, __attribute__((unused)) size_t length)
{
  event_distributor_enqueue_hw_event_from_isr(HW_EVENT_UART_RECEIVE);
}

void cli_put_char(char ch)
{
  uint8_t buf = (uint8_t)ch;
  while (zpal_uart_transmit_in_progress(uart_handle))
      ;
  zpal_uart_transmit(uart_handle, &buf, 1, NULL);
}

void setup_cli(zpal_uart_config_t *p_uart_config)
{
  // Set up notify queue for HW events
  m_HWEventQueue = xQueueCreateStatic(
    sizeof_array(m_HWEventQueueStorage),
    sizeof(m_HWEventQueueStorage[0]),
    (uint8_t*)m_HWEventQueueStorage,
    &m_HWEventQueueObject
    );

  QueueNotifyingInit(
    &m_HWEventNotifyingQueue,
    m_HWEventQueue,
    ZAF_getAppTaskHandle(),
    EAPPLICATIONEVENT_HW);

  // Set common UART configuration
  p_uart_config->tx_buffer = tx_data,
  p_uart_config->tx_buffer_len = COMM_INT_TX_BUFFER_SIZE,
  p_uart_config->rx_buffer = rx_data,
  p_uart_config->rx_buffer_len = COMM_INT_RX_BUFFER_SIZE,
  p_uart_config->receive_callback = cli_uart_rx_callback;

  zpal_status_t status = zpal_uart_init(p_uart_config, &uart_handle);
  tr_cli_init(TR_CLI_PROMPT, cli_put_char);
  ASSERT(ZPAL_STATUS_OK == status);
}

// Print functions for UART CLI
void tr_cli_common_print(const char* message)
{
  size_t length = strlen(message); //NOSONAR
  // We understand the risk of using strlen here but ignores the security warning
  // because this is a function only used for zero terminated strings
  if (length > 0)
  {
    while (zpal_uart_transmit_in_progress(uart_handle))
      ;
    zpal_uart_transmit(uart_handle, (uint8_t*)message, length, NULL);
  }
}

/*
 * Blocking function for printf to UART
 */
__attribute__((__format__ (__printf__, 1, 0)))
void tr_cli_common_printf(const char *pFormat, ...) // NOSONAR
{
  va_list pArgs;
  va_start(pArgs, pFormat);

  // Wait for last print to avoid buffer overwrite
  while (zpal_uart_transmit_in_progress(uart_handle))
      ;

  vsnprintf((char*)m_pBuffer, PRINTF_BUFFER_SIZE, pFormat, pArgs);
  va_end(pArgs);

  tr_cli_common_print((char*)m_pBuffer);
}
