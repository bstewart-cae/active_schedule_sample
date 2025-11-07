/// ****************************************************************************
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "tr_platform.h"
#include "zpal_uart.h"
#include "zpal_uart_private.h"
#include "zpal_uart_gpio.h"
#include "zpal_bootloader.h"
#include "zpal_otw_hw_config.h"
#include "zpal_watchdog.h"
#include "sysctrl.h"
#include "tr_hal_gpio.h"
#include "tr_hal_uart.h"
#include "FreeRTOS.h"
#include "task.h"
#include "Assert.h"

#define ZWAVE_ALLIANCE_SUPPORT

#define COMM_INT_TX_BUFFER_SIZE 2
#define COMM_INT_RX_BUFFER_SIZE (128 + 5)

#define SOH            0x01  // Start of Header
#define EOT            0x04  // End of Transmission
#define ACK            0x06  // Acknowledge
#define NAK            0x15  // Negative Acknowledge
#define CRC_MODE       0x43  // Request CRC mode
#define PACKET_SIZE    128
#define PACKET_HEADER  5
#define CHAR_WAITTIME  1

static uint8_t tx_data[COMM_INT_TX_BUFFER_SIZE];
static uint8_t rx_data[COMM_INT_RX_BUFFER_SIZE];
static uint8_t rx_packet[PACKET_SIZE + PACKET_HEADER];
static uint8_t rx_mode = 0;  // 0: SILABS_DATA, 1: XMODEM_DATA
static uint8_t total_count = 0;
static zpal_uart_handle_t uart_handle = NULL;
static int write_offset = 0;
static bool watchdog_state;

static zpal_otw_uart_config_t m_uart_config;
static zpal_uart_config_ext_t m_uart_gpio_config;

// UART receive callback
static void uart_receive_callback(zpal_uart_handle_t handle, __attribute__((unused)) size_t length) {
#ifdef ZWAVE_ALLIANCE_SUPPORT
    if (rx_mode == 0) {  // SILABS_DATA mode
        uint8_t dummy;
        zpal_uart_receive(handle, &dummy, 1);
    }
    else
#endif
    {  // XMODEM_DATA mode
      uint8_t rx_count = zpal_uart_get_available(handle);
      if (rx_count > 0) {
          zpal_uart_receive(handle, &rx_packet[total_count], rx_count);
          total_count += rx_count;
      }
    }
}

// CRC-16-CCITT calculation (polynomial 0x1021)
static uint16_t calculate_crc(const uint8_t *data, int size) {
    uint16_t crc = 0x0000;
    for (int i = 0; i < size; i++) {
        crc ^= (uint16_t)data[i] << 8;
        for (int j = 0; j < 8; j++) {
            crc = (crc & 0x8000) ? (crc << 1) ^ 0x1021 : (crc << 1);
        }
    }
    return crc;
}

// Utility function: Transmit a single byte with a delay
static void transmit_byte(uint8_t byte, int delay_ms) {
    zpal_uart_transmit(uart_handle, &byte, 1, NULL);
    if (delay_ms > 0) {
        Delay_ms(delay_ms);
    }
}

// Utility function: Handle error and exit
static void handle_error(void) {
#ifdef ZWAVE_ALLIANCE_SUPPORT
    rx_mode = 0; // SILABS_DATA mode
    transmit_byte(0x3E, 10); // Signal end of communication
#endif
}

// Read a full packet into the buffer
static bool read_packet(void) {
    uint8_t wait_time = PACKET_SIZE + PACKET_HEADER;
    total_count = 0;

    while (wait_time && (sizeof(rx_packet) != total_count)) {
        vTaskDelay(CHAR_WAITTIME);
        wait_time--;
        if (rx_packet[0] == EOT) {
            return true;
        }
    }
    return (sizeof(rx_packet) == total_count);
}

// Define events and states for state-event table
typedef enum {
    EVENT_INIT,
    EVENT_PACKET_READY,
    EVENT_EOT,
    EVENT_TIMEOUT,
    EVENT_ERROR
} xmodem_event_t;

typedef enum {
    STATE_INIT,
    STATE_WAIT_FOR_PACKET,
    STATE_RECEIVE_PACKET,
    STATE_HANDLE_EOT,
    STATE_ERROR,
    STATE_DONE
} xmodem_state_t;

typedef struct {
    xmodem_state_t next_state;
    void (*action)(xmodem_event_t *event);
} state_transition_t;

// Function prototypes for state actions
static void action_init(xmodem_event_t *event);
static void action_wait_for_packet(xmodem_event_t *event);
static void action_receive_packet(xmodem_event_t *event);
static void action_handle_eot(xmodem_event_t *event);
static void action_error(xmodem_event_t *event);
static void action_done(xmodem_event_t *event);

// State-event transition table
state_transition_t state_table[STATE_DONE + 1][EVENT_ERROR + 1] = {
    [STATE_INIT] = {
        [EVENT_INIT]         = {STATE_WAIT_FOR_PACKET, action_init},
        [EVENT_ERROR]        = {STATE_ERROR, action_error},
        [EVENT_PACKET_READY] = {STATE_INIT, NULL},
        [EVENT_EOT]          = {STATE_INIT, NULL},
        [EVENT_TIMEOUT]      = {STATE_INIT, NULL},
    },
    [STATE_WAIT_FOR_PACKET] = {
        [EVENT_PACKET_READY] = {STATE_RECEIVE_PACKET, action_wait_for_packet},
        [EVENT_TIMEOUT]      = {STATE_ERROR, action_error},
        [EVENT_ERROR]        = {STATE_ERROR, action_error},
        [EVENT_EOT]          = {STATE_RECEIVE_PACKET, NULL},
        [EVENT_INIT]         = {STATE_WAIT_FOR_PACKET, NULL},
    },
    [STATE_RECEIVE_PACKET] = {
        [EVENT_PACKET_READY] = {STATE_WAIT_FOR_PACKET, action_receive_packet},
        [EVENT_ERROR]        = {STATE_ERROR, action_error},
        [EVENT_EOT]          = {STATE_HANDLE_EOT, action_handle_eot},
        [EVENT_TIMEOUT]      = {STATE_RECEIVE_PACKET, NULL},
        [EVENT_INIT]         = {STATE_RECEIVE_PACKET, NULL},

    },
    [STATE_HANDLE_EOT] = {
        [EVENT_ERROR]        = {STATE_DONE, action_done},
        [EVENT_PACKET_READY] = {STATE_HANDLE_EOT, NULL},
        [EVENT_EOT]          = {STATE_HANDLE_EOT, NULL},
        [EVENT_TIMEOUT]      = {STATE_HANDLE_EOT, NULL},
        [EVENT_INIT]         = {STATE_HANDLE_EOT, NULL},
    },
    [STATE_ERROR] = {
        [EVENT_ERROR]        = {STATE_DONE, action_error},
        [EVENT_PACKET_READY] = {STATE_ERROR, NULL},
        [EVENT_EOT]          = {STATE_ERROR, NULL},
        [EVENT_TIMEOUT]      = {STATE_ERROR, NULL},
        [EVENT_INIT]         = {STATE_ERROR, NULL},


    },
    [STATE_DONE] = {
        [EVENT_ERROR]        = {STATE_DONE, action_error},
        [EVENT_PACKET_READY] = {STATE_DONE, NULL},
        [EVENT_EOT]          = {STATE_DONE, NULL},
        [EVENT_TIMEOUT]      = {STATE_DONE, NULL},
        [EVENT_INIT]         = {STATE_DONE, NULL},

    }

};

static xmodem_state_t current_state = STATE_INIT;

void execute_state(xmodem_event_t *event) {
    state_transition_t transition = state_table[current_state][*event];
    if (transition.action) {
        transition.action(event);
    }
    current_state = transition.next_state;
}

static void otw_uart_config(void)
{

  zpal_otw_hw_config(&m_uart_config,&m_uart_gpio_config );
  // UART configuration
  const zpal_uart_config_t UART_CONFIG = {
    .tx_buffer = tx_data,
    .tx_buffer_len = COMM_INT_TX_BUFFER_SIZE,
    .rx_buffer = rx_data,
    .rx_buffer_len = COMM_INT_RX_BUFFER_SIZE,
    .id = m_uart_config.uart_id,
    .baud_rate = m_uart_config.baud_rate,
    .data_bits = m_uart_config.data_bits,
    .parity_bit = m_uart_config.parity_bit,
    .stop_bits = m_uart_config.stop_bits,
    .receive_callback = uart_receive_callback,
    .ptr = &m_uart_gpio_config,
    .flags = 0
  };
  zpal_uart_uninit(UART_CONFIG.id);
  zpal_status_t status = zpal_uart_init(&UART_CONFIG, &uart_handle);
  ASSERT(ZPAL_STATUS_OK == status);
}

// Action implementations
static void action_init(xmodem_event_t *event) {

    watchdog_state = zpal_is_watchdog_enabled();
    zpal_enable_watchdog(false);

    otw_uart_config();
#ifdef ZWAVE_ALLIANCE_SUPPORT
    rx_mode = 0; // SILABS_DATA mode
    transmit_byte('>', 10); // Bootloader emulation
    transmit_byte('d', 10);
    transmit_byte(CRC_MODE, 10);
#endif
    rx_mode = 1; // XMODEM_DATA mode
    transmit_byte(CRC_MODE, 0);

    *event = EVENT_PACKET_READY; // Trigger the next event
}


static void action_wait_for_packet(xmodem_event_t *event) {
    if (read_packet()) {
        if (rx_packet[0] == SOH) {
            *event = EVENT_PACKET_READY; // Packet ready event
        } else if (rx_packet[0] == EOT) {
            *event = EVENT_EOT; // End of transmission event
        } else {
            *event = EVENT_ERROR; // Invalid packet event
        }
    } else {
        *event = EVENT_TIMEOUT; // Timeout event
    }
}

static void action_receive_packet(xmodem_event_t *event) {
    uint16_t received_crc = (rx_packet[PACKET_SIZE + 3] << 8) | rx_packet[PACKET_SIZE + 4];
    uint16_t calculated_crc = calculate_crc(&rx_packet[3], PACKET_SIZE);
    if (received_crc != calculated_crc) {
        transmit_byte(NAK, 0);
        *event = EVENT_ERROR;
        return;
    }

    if (ZPAL_STATUS_FAIL == zpal_bootloader_write_data(write_offset, &rx_packet[3], PACKET_SIZE)) {
        *event = EVENT_ERROR;
        return;
    }

    write_offset += PACKET_SIZE;
    transmit_byte(ACK, 0); // Acknowledge packet
    *event = EVENT_PACKET_READY;
}

static void action_handle_eot(xmodem_event_t *event) {
    transmit_byte(ACK, 0); // End of transmission
    *event = EVENT_ERROR; // Transition to done state
}

static void action_error(xmodem_event_t *event) {
    handle_error();
    *event = EVENT_ERROR;
}

static void action_done(xmodem_event_t *event) {
    zpal_enable_watchdog(watchdog_state);
    *event = EVENT_ERROR;
}

// Main function to drive the state-event table state machine
bool xmodem_receive(void) {
    xmodem_event_t event = EVENT_INIT;
    execute_state(&event); // Start state machine

    while (current_state != STATE_DONE) {
        execute_state(&event); // Process the triggered event
    }

    return current_state != STATE_ERROR;
}

ZWSDK_WEAK void zpal_otw_hw_config(zpal_otw_uart_config_t *uart_config,
                                   zpal_uart_config_ext_t *uart_gpio_config)
{
  if (NULL != uart_config)
  {
    uart_config->uart_id    = ZPAL_UART0;
    uart_config->baud_rate  = 115200;
    uart_config->data_bits  = 8;
    uart_config->parity_bit = ZPAL_UART_NO_PARITY;
    uart_config->stop_bits  = ZPAL_UART_STOP_BITS_1;
  }

  if (NULL != uart_gpio_config)
  {
    uart_gpio_config->txd_pin = 17;
    uart_gpio_config->rxd_pin = 16;
    uart_gpio_config->cts_pin = 0; // Not used.
    uart_gpio_config->rts_pin = 0;  // Not used.
    uart_gpio_config->uart_wakeup = false;
  }
}
