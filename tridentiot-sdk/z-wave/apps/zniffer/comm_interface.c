/// ***************************************************************************
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************


#include "comm_interface.h"
#include "zpal_uart.h"
#include <FreeRTOS.h>
#include <timers.h>
#include <string.h>
#include "Assert.h"

// #define DEBUGPRINT // NOSONAR
#include "DebugPrint.h"
#ifdef DEBUGPRINT
#include <stdio.h>
#endif

#define COMMAND_SOF   '#'
#define FRAME_SOF     '!'

#define COMMAND_LENGTH_MAX        RECEIVE_BUFFER_SIZE

#define START_OF_DATA_DELIMITER  0x0321

#define DEFAULT_BYTE_TIMEOUT_MS 150
#define HEADER_LEN              2

#define TYPE_FRAME        1
#define TYPE_BEAM_FRAME   2
#define TYPE_BEAM_START   4
#define TYPE_BEAM_STOP    5

typedef enum
{
  COMM_INTERFACE_STATE_SOF      = 0,
  COMM_INTERFACE_STATE_CMD      = 3,
  COMM_INTERFACE_STATE_LEN      = 1,
  COMM_INTERFACE_STATE_DATA     = 4,
} comm_interface_state_t;

typedef struct
{
  transport_t transport;
  TimerHandle_t byte_timer;
  StaticTimer_t byte_timer_buffer;
  bool byte_timeout;
  uint32_t byte_timeout_ms;
  comm_interface_state_t state;
  uint8_t buffer_len;
  uint8_t buffer[RECEIVE_BUFFER_SIZE];
  bool rx_active;
  uint8_t rx_wait_count;
} comm_interface_t;

static comm_interface_t comm_interface = { 0 };
comm_interface_command_t* const serial_frame = (comm_interface_command_t*)comm_interface.buffer;

#ifdef DEBUGPRINT
static char str_buf[64];
#endif

comm_interface_command_t* comm_interface_get_command(void)
{
  return serial_frame;
}

void (*uart_receive_handler)(void);

static void receive_callback(__attribute__((unused)) const zpal_uart_handle_t handle, __attribute__((unused)) size_t available)
{
  if (NULL != uart_receive_handler)
  {
    uart_receive_handler();
  }
}

static void byte_timer_cb(__attribute__((unused)) TimerHandle_t xTimer )
{
  comm_interface.byte_timeout = true;
}

zpal_status_t comm_interface_transmit(transport_t *transport, const uint8_t *data, size_t len, transmit_done_cb_t cb)
{
  if (transport)
  {
    return zpal_uart_transmit(transport->handle, data, len, cb);
  }

  return ZPAL_STATUS_FAIL;
}


void comm_interface_transmit_command(uint8_t cmd, const uint8_t *payload, uint8_t len, transmit_done_cb_t cb)
{
  uint8_t transmit_length;
  comm_interface_command_t cmd_frame;
  xTimerStop(comm_interface.byte_timer, 100);

  comm_interface.byte_timeout = false;

  cmd_frame.sof = COMMAND_SOF;
  cmd_frame.len = len;
  cmd_frame.cmd = cmd;
  memcpy(cmd_frame.payload, payload, len);
  transmit_length = len+3;

#ifdef DEBUGPRINT
  char *p_str = str_buf;
  uint16_t str_len = 0;
  sprintf(p_str, "TX(%d):", len);
  str_len = strlen(str_buf);
  for (uint8_t i = 0; i < transmit_length; i++)
  {
    sprintf(p_str + str_len, "%02X", ((uint8_t*)&frame)[i]);
    str_len = strlen(str_buf);
  }
  DPRINTF("%s\n", str_buf);
#endif

  if (ZPAL_STATUS_FAIL != comm_interface_transmit(&comm_interface.transport, (uint8_t *)&cmd_frame, transmit_length, cb) )
  {
    comm_interface_wait_transmit_done();
  }
}

static comm_interface_frame_t frame_to_send;
void comm_interface_transmit_frame(uint16_t timestamp, uint8_t ch_and_speed, uint8_t region, int8_t rssi, const uint8_t *payload, uint8_t length, transmit_done_cb_t cb)
{
  uint8_t transmit_length;

  frame_to_send.sof_frame         = FRAME_SOF;
  frame_to_send.type              = TYPE_FRAME;
  frame_to_send.timestamp         = timestamp;
  frame_to_send.channel_and_speed = ch_and_speed;
  frame_to_send.region            = region;
  frame_to_send.rssi              = rssi;
  frame_to_send.start_of_data     = START_OF_DATA_DELIMITER;
  frame_to_send.len               = length;
  memcpy(frame_to_send.payload, payload, length);

#ifdef DEBUGPRINT
  char *p_str = str_buf;
  uint16_t str_len = 0;
  sprintf(p_str, "TX(%d):", length);
  str_len = strlen(str_buf);
  transmit_length = offsetof(comm_interface_frame_t, len) + 1;
  for (uint8_t i = 0; i < transmit_length; i++)
  {
    sprintf(p_str + str_len, "%02X", ((uint8_t*)&frame_to_send)[i]);
    str_len = strlen(str_buf);
  }
  sprintf(p_str + str_len, "%c", '-');
  str_len = strlen(str_buf);
  for (uint8_t j = transmit_length; j < length + transmit_length; j++)
  {
    sprintf(p_str + str_len, "%02X", ((uint8_t*)&frame_to_send)[j]);
    str_len = strlen(str_buf);
  }
  DPRINTF("%s\n", str_buf);
#endif
  transmit_length = length + offsetof(comm_interface_frame_t, len) + 1;
  comm_interface_transmit(&comm_interface.transport, (uint8_t *)&frame_to_send, transmit_length, cb);
}

static comm_interface_beam_start_frame_t beam_start_frame;
void comm_interface_transmit_beam_start(uint16_t timestamp, uint8_t ch_and_speed, uint8_t region, int8_t rssi, const uint8_t *payload, uint8_t length, transmit_done_cb_t cb)
{
  uint8_t transmit_length;

  beam_start_frame.sof               = FRAME_SOF;
  beam_start_frame.type              = TYPE_BEAM_START;
  beam_start_frame.timestamp         = timestamp;
  beam_start_frame.channel_and_speed = ch_and_speed;
  beam_start_frame.region            = region;
  beam_start_frame.rssi              = rssi;

  if (length < sizeof(beam_start_frame.payload))
  {
    beam_start_frame.payload[0] = payload[0];
    beam_start_frame.payload[1] = payload[1];
    beam_start_frame.payload[2] = 0x01;
    beam_start_frame.payload[3] = payload[2];
    length = sizeof(beam_start_frame.payload);
  }
  else
  {
    memcpy(beam_start_frame.payload, payload, sizeof(beam_start_frame.payload));
  }

  transmit_length = length +  offsetof(comm_interface_beam_start_frame_t, payload);
  comm_interface_transmit(&comm_interface.transport, (uint8_t *)&beam_start_frame, transmit_length, cb);
}

static comm_interface_beam_stop_frame_t beam_stop_frame;
void comm_interface_transmit_beam_stop(uint16_t timestamp, int8_t rssi, uint16_t counter, transmit_done_cb_t cb)
{

  beam_stop_frame.sof        = FRAME_SOF;
  beam_stop_frame.type       = TYPE_BEAM_STOP;
  beam_stop_frame.timestamp  = timestamp;
  beam_stop_frame.rssi       = rssi;
  beam_stop_frame.counter    = counter;
  comm_interface_transmit(&comm_interface.transport, (uint8_t *)&beam_stop_frame, sizeof(comm_interface_beam_stop_frame_t), cb);
}

void comm_interface_wait_transmit_done(void)
{
  while(zpal_uart_transmit_in_progress(comm_interface.transport.handle));
}

bool comm_interface_was_transmit_done(void)
{
  if (zpal_uart_transmit_in_progress(comm_interface.transport.handle))
  {
    return false;
  }
  return true;
}

size_t comm_interface_data_available(void)
{
  return zpal_uart_get_available(comm_interface.transport.handle);
}
void comm_interface_init(zpal_uart_config_t *uart_config, void (*uart_rx_event_handler)())
{
  uart_config->receive_callback = receive_callback;

  zpal_status_t status = zpal_uart_init(uart_config, &comm_interface.transport.handle);
  ASSERT(status == ZPAL_STATUS_OK);
  uart_receive_handler = uart_rx_event_handler;
  status = zpal_uart_enable(comm_interface.transport.handle);
  ASSERT(status == ZPAL_STATUS_OK);

  comm_interface.byte_timer = xTimerCreateStatic( "",
                              DEFAULT_BYTE_TIMEOUT_MS,
                              pdFALSE,
                              NULL,
                              byte_timer_cb,
                              &comm_interface.byte_timer_buffer);

  comm_interface.state = COMM_INTERFACE_STATE_SOF;
  comm_interface.buffer_len = 0;
}

static void store_byte(uint8_t byte)
{
  // Restart timer if active
  xTimerReset(comm_interface.byte_timer, 100);

  comm_interface.byte_timeout = false;
  comm_interface.buffer[comm_interface.buffer_len] = byte;
  comm_interface.buffer_len++;
}

static comm_interface_parse_result_t handle_sof(uint8_t input)
{
  comm_interface_parse_result_t result = PARSE_IDLE;

  if (input == COMMAND_SOF)
  {
    comm_interface.state = COMM_INTERFACE_STATE_CMD;
    comm_interface.buffer_len = 0;
    comm_interface.rx_active = true; // now we're receiving - check for timeout
    store_byte(input);
  }
  return result;
}

static void handle_len(uint8_t input)
{
  // Check for length to be inside valid range
  if (input > COMMAND_LENGTH_MAX)
  {
    comm_interface.state = COMM_INTERFACE_STATE_SOF; // Restart looking for SOF
    comm_interface.rx_active = false;  // Not really active now...
    xTimerStop(comm_interface.byte_timer, 100);
    comm_interface.byte_timeout = false;
  }
  else
  {
    comm_interface.state = COMM_INTERFACE_STATE_DATA;
    store_byte(input);
    comm_interface.rx_wait_count = input;
  }
}

static void handle_cmd(uint8_t input)
{
  store_byte(input);
  comm_interface.state = COMM_INTERFACE_STATE_LEN;
}

static comm_interface_parse_result_t handle_data(uint8_t input)
{
  comm_interface_parse_result_t result = PARSE_IDLE;

  if (0 < comm_interface.rx_wait_count)
  {
    comm_interface.rx_wait_count--;
    store_byte(input);
  }

  if ((comm_interface.buffer_len >= RECEIVE_BUFFER_SIZE) ||
      ((comm_interface.buffer_len > 2) && (comm_interface.buffer_len > 2 + serial_frame->len)))      //buffer_len - sizeof(sof) >= serial_frame->len
  {
    xTimerStop(comm_interface.byte_timer, 100);
    comm_interface.byte_timeout = false;
    comm_interface.state = COMM_INTERFACE_STATE_SOF; // Restart looking for SOF
    comm_interface.rx_active = false;  // Not really active
    result = PARSE_FRAME_RECEIVED;

#ifdef DEBUGPRINT
    char *p_str = str_buf;
    uint16_t str_len = 0;
    sprintf(p_str, "RX(%d):", comm_interface.buffer_len);
    str_len = strlen(str_buf);
    for (uint8_t i = 0; i < comm_interface.buffer_len; i++)
    {
      sprintf(p_str + str_len, "%02X", comm_interface.buffer[i]);
      str_len = strlen(str_buf);
    }
    DPRINTF("%s\n", str_buf);
#endif
  }

  return result;
}

static void handle_default(void)
{
  comm_interface.state = COMM_INTERFACE_STATE_SOF; // Restart looking for SOF
  comm_interface.rx_active = false;  // Not really active now...
  comm_interface.byte_timeout = false;
  xTimerStop(comm_interface.byte_timer, 100);
}

comm_interface_parse_result_t comm_interface_parse_data()
{
  uint8_t rx_byte;
  comm_interface_parse_result_t result = PARSE_IDLE;

  while ((result == PARSE_IDLE) && zpal_uart_get_available(comm_interface.transport.handle))
  {
    zpal_uart_receive(comm_interface.transport.handle, &rx_byte, sizeof(rx_byte));

    switch (comm_interface.state)
    {
      case COMM_INTERFACE_STATE_SOF:
        result = handle_sof(rx_byte);
        break;

      case COMM_INTERFACE_STATE_LEN:
        handle_len(rx_byte);
        if (0 == comm_interface.rx_wait_count)
        {
          result = handle_data(rx_byte);
        }
        break;

      case COMM_INTERFACE_STATE_CMD:
        handle_cmd(rx_byte);
        break;

      case COMM_INTERFACE_STATE_DATA:
        result = handle_data(rx_byte);
        break;

      default :
        DPRINT("Unknown\n");
        handle_default();
        break;
    }
  }

  /* Check for timeouts - if no other events detected */
  if (result == PARSE_IDLE)
  {
    /* Are we in the middle of collecting a frame and have we timed out? */
    if (comm_interface.rx_active && comm_interface.byte_timeout)
    {
      comm_interface.byte_timeout = false;
      /* Reset to SOF hunting */
      comm_interface.state = COMM_INTERFACE_STATE_SOF;
      comm_interface.rx_active = false; /* Not inframe anymore */
      result = PARSE_RX_TIMEOUT;
    }
  }
  return result;
}
