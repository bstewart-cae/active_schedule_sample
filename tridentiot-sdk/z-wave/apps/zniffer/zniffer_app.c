/// ***************************************************************************
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2023 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************

/**
 * @file zniffer_app.c
 * @brief Z-Wave zniffer application
 */

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <string.h>
#include <Assert.h>

#include <FreeRTOS.h>
#include "task.h"
#include <timers.h>

// #define DEBUGPRINT // NOSONAR
#include "DebugPrint.h"
#include "DebugPrintConfig.h"
#include "SizeOf.h"
#include "EventDistributor.h"
#include "zpal_radio_private.h"

#include <zniffer_app.h>
#include <comm_interface.h>

#include <zpal_radio.h>
#include <zpal_watchdog.h>
#include <zpal_power_manager.h>
#include <zpal_radio_utils.h>
#include <zpal_entropy.h>
#include <zpal_init.h>
#include <zpal_misc.h>
#include <zpal_uart.h>
#include <zpal_nvm.h>

// #define TEST_DATA

/****************************************************************************/
/*                      PRIVATE TYPES and DEFINITIONS                       */
/****************************************************************************/
#define ZNIFFER_CMD_VERSION           1
#define ZNIFFER_CMD_SET_REGION        2
#define ZNIFFER_CMD_GET_REGIONS       3
#define ZNIFFER_CMD_GET_REGION_STR    19
#define ZNIFFER_CMD_START             4
#define ZNIFFER_CMD_STOP              5
#define ZNIFFER_CMD_BAUD_RATE         14

#define ZNIFFER_FILE_ID               800

typedef struct {
  zpal_radio_region_t region;
  uint8_t             supported_channels;
  char*               region_string;
} zniffer_region_type_t;

// Make internal regions numbers for LR backup regions and end device regions as they do not
// exist as actual regions in zpal radio API
#define INTERNAL_REGION_US_LR_920        100
#define INTERNAL_REGION_EU_LR_866_4      101
#define INTERNAL_REGION_US_LR_END_DEVICE 102
#define INTERNAL_REGION_EU_LR_END_DEVICE 103

const zniffer_region_type_t region_table[] = {
  {REGION_EU,                         2, "EU"},
  {REGION_US,                         2, "US"},
  {REGION_ANZ,                        2, "ANZ"},
  {REGION_HK,                         2, "HK"},
  {REGION_IN,                         2, "IN"},
  {REGION_IL,                         2, "IL"},
  {REGION_RU,                         2, "RU"},
  {REGION_CN,                         2, "CN"},
  {REGION_US_LR,                      4, "US_LR_912MHz"},
  {INTERNAL_REGION_US_LR_920,         4, "US_LR_920MHz"},
  {REGION_EU_LR,                      4, "EU_LR_864MHz"},
  {INTERNAL_REGION_EU_LR_866_4,       4, "EU_LR_866MHz"},
  {REGION_JP,                         3, "JP"},
  {REGION_KR,                         3, "KR"},
  {INTERNAL_REGION_US_LR_END_DEVICE,  2, "US_LR_END_DEVICE"},
  {INTERNAL_REGION_EU_LR_END_DEVICE,  2, "EU_LR_END_DEVICE"},
};

// Structure of the file in NVM
typedef struct
{
  zpal_radio_region_t stored_region;
} zniffer_file_t;

/****************************************************************************/
/*                              PRIVATE DATA                                */
/****************************************************************************/
TaskHandle_t g_ZnifferTaskHandle;

static uint8_t network_homeid[4] = {0xDE, 0xAD, 0xBE, 0xEF};
static zpal_radio_network_stats_t sNetworkStatistic = {0};

static bool zniffer_started = false;
static zpal_radio_region_t current_internal_region = REGION_US_LR;

static uint32_t rxBeamReceived = 0;
static uint32_t rxTimeout = 0;

static bool in_beam_handling = false;
static uint16_t current_beam_count = 0;

typedef struct zniffer_timer
{
  TimerHandle_t handler;
  StaticTimer_t timer_buffer;
} zniffer_timer_t;

typedef enum
{
  ZNIFFER_BEAM_START = 1,
  ZNIFFER_BEAM_STOP,
  ZNIFFER_DATA
} zniffer_data_type;

typedef struct  __attribute__((packed)) zniffer_beam_stop
{
  int8_t rssi;
  uint16_t counter;
} zniffer_beam_stop_t;

typedef struct  __attribute__((packed)) zniffer_data_frame
{
  uint16_t timestamp;
  uint8_t ch_and_speed;
  uint8_t region;
  int8_t rssi;
  uint8_t frame_data[180];
  uint8_t frame_length;
} zniffer_data_frame_t;

typedef struct  __attribute__((packed)) zniffer_queue_elm
{
  union
  {
    zniffer_beam_stop_t beam_stop;
    zniffer_data_frame_t zniffer_data;
  } elm;
  zniffer_data_type frame_type;
} zniffer_queue_elm_t;

#define ZNIFFER_QUEUE_SIZE      32
static zniffer_queue_elm_t queue_elements_list[ZNIFFER_QUEUE_SIZE];
static uint8_t queue_element_read;
static uint8_t queue_element_write;

zniffer_timer_t beam_stop_timer;

#ifdef TEST_DATA
zniffer_timer_t test_timer;

static uint8_t transmit_rf_frame_to_host = 0;
#endif

zpal_nvm_handle_t zniffer_nvm_handle;

zniffer_file_t zniffer_storage;

#ifdef DEBUGPRINT
  static uint8_t tx_buffer[128];
  static uint8_t rx_buffer[64];
  #define TX_BUFFER_SIZE sizeof(tx_buffer)
  #define RX_BUFFER_SIZE sizeof(rx_buffer)

  static zpal_uart_config_t uart_cfg = {.id = ZPAL_UART1,
                                        .tx_buffer = tx_buffer,
                                        .tx_buffer_len = TX_BUFFER_SIZE,
                                        .rx_buffer = rx_buffer,
                                        .rx_buffer_len = RX_BUFFER_SIZE,
                                        .baud_rate = 115200,
                                        .data_bits = 8,
                                        .parity_bit = ZPAL_UART_NO_PARITY,
                                        .stop_bits = ZPAL_UART_STOP_BITS_1,
                                        .ptr = NULL,
                                        .flags = ZPAL_UART_CONFIG_FLAG_BLOCKING
                                       };
#endif

// Prioritized events that can wakeup protocol thread.
typedef enum EProtocolEvent
{
  EZNIFFER_EVENT_RFRXBEAM,
  EZNIFFER_EVENT_RFRXTIMEOUT,
  EZNIFFER_EVENT_RFRX,
  EZNIFFER_EVENT_UARTRX,
  EZNIFFER_EVENT_NUM
} EProtocolEvent;

// Event distributor object
static SEventDistributor g_EventDistributor = { 0 };

// Event Handlers
static void zniffer_event_handler_beam_receive(void);
static void zniffer_event_handler_receive_timeout(void);
static void zniffer_event_handler_uart_receive(void);

// Event distributor event handler table
static const EventDistributorEventHandler g_aEventHandlerTable[EZNIFFER_EVENT_NUM] =
{
  zniffer_event_handler_beam_receive,    // Event 0
  zniffer_event_handler_receive_timeout, // Event 1
  zpal_radio_get_last_received_frame,    // Event 2
  zniffer_event_handler_uart_receive,    // Event 3
};

#define ZNIFFER_EVENT_RF_RX_BEAM             (1UL << EZNIFFER_EVENT_RFRXBEAM)
#define ZNIFFER_EVENT_RF_RX_TIMEOUT          (1UL << EZNIFFER_EVENT_RFRXTIMEOUT)
#define ZNIFFER_EVENT_RF_RX_FRAME_RECEIVED   (1UL << EZNIFFER_EVENT_RFRX)
#define ZNIFFER_EVENT_UART_RX                (1UL << EZNIFFER_EVENT_UARTRX)

/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/
zpal_pm_handle_t application_radio_power_lock;

extern zpal_uart_config_t ZNIFFER_UART_CONFIG;

/****************************************************************************/
/*                              STUB FUNCTIONS                              */
/****************************************************************************/

/****************************************************************************/
/*                            PRIVATE FUNCTIONS                             */
/****************************************************************************/
static void zniffer_frame_transmit_done(transport_handle_t transport);
zpal_radio_speed_t GetBeamSpeed(zpal_radio_zwave_channel_t channel);
uint8_t GetRadioChannel(zpal_radio_zwave_channel_t channel_id, zpal_radio_speed_t speed);
uint8_t GetRadioSpeed(zpal_radio_speed_t speed);
zpal_radio_region_t internal_region_to_zpal_region(zpal_radio_region_t internal_region);

static void zniffer_frame_transmit_done(__attribute__((unused)) transport_handle_t transport)
{
  queue_element_read = (queue_element_read + 1) % ZNIFFER_QUEUE_SIZE;
  if (queue_element_write > queue_element_read)
  {
    if (queue_elements_list[queue_element_read].frame_type == ZNIFFER_BEAM_START)
    {
      comm_interface_transmit_beam_start(0,
      queue_elements_list[queue_element_read].elm.zniffer_data.ch_and_speed,
      queue_elements_list[queue_element_read].elm.zniffer_data.region,
      queue_elements_list[queue_element_read].elm.zniffer_data.rssi,
      queue_elements_list[queue_element_read].elm.zniffer_data.frame_data,
      queue_elements_list[queue_element_read].elm.zniffer_data.frame_length,
      zniffer_frame_transmit_done
     );

     return;
    }
    if (queue_elements_list[queue_element_read].frame_type == ZNIFFER_BEAM_STOP)
    {
      comm_interface_transmit_beam_stop(0, queue_elements_list[queue_element_read].elm.beam_stop.rssi,
                                        queue_elements_list[queue_element_read].elm.beam_stop.counter,
                                        zniffer_frame_transmit_done);
      return;

    }
    if (queue_elements_list[queue_element_read].frame_type == ZNIFFER_DATA)
    {
      comm_interface_transmit_frame(queue_elements_list[queue_element_read].elm.zniffer_data.timestamp,
                                    queue_elements_list[queue_element_read].elm.zniffer_data.ch_and_speed,
                                    queue_elements_list[queue_element_read].elm.zniffer_data.region,
                                    queue_elements_list[queue_element_read].elm.zniffer_data.rssi,
                                    queue_elements_list[queue_element_read].elm.zniffer_data.frame_data,
                                    queue_elements_list[queue_element_read].elm.zniffer_data.frame_length,
                                    zniffer_frame_transmit_done);
      return;
    }

  }
}

static inline uint16_t swap_uint16(uint16_t value)
{
  return (value << 8) | ((value >> 8) & 0xFF);
}

uint16_t get_16bit_tick_reversed()
{
  // Get the tick counter from FreeRTOS and convert it to 16 bit and reverse byte order
  return  swap_uint16(0xffff & xTaskGetTickCount());
}

void enqueue_zniffer_element(zniffer_data_type elem_type,zpal_radio_rx_parameters_t * pRxParameters, zpal_radio_receive_frame_t * pZpalFrame)
{
   if (elem_type == ZNIFFER_BEAM_START)
   {
      queue_elements_list[queue_element_write].frame_type =  ZNIFFER_BEAM_START;
      queue_elements_list[queue_element_write].elm.zniffer_data.rssi = zpal_radio_get_last_beam_rssi();
      zpal_radio_zwave_channel_t channel = zpal_radio_get_last_beam_channel();
      zpal_radio_speed_t speed = GetBeamSpeed(channel);
      queue_elements_list[queue_element_write].elm.zniffer_data.region = (uint8_t)zpal_radio_get_region();
      queue_elements_list[queue_element_write].elm.zniffer_data.frame_length = zpal_radio_get_last_beam_data(queue_elements_list[queue_element_write].elm.zniffer_data.frame_data, 4);
      queue_elements_list[queue_element_write].elm.zniffer_data.ch_and_speed = (GetRadioChannel(channel, speed) << 5) | GetRadioSpeed(speed);
      queue_element_write = (queue_element_write +1) % ZNIFFER_QUEUE_SIZE;
   }
   else if (elem_type == ZNIFFER_BEAM_STOP)
   {
     queue_elements_list[queue_element_write].frame_type = ZNIFFER_BEAM_STOP;
     queue_elements_list[queue_element_write].elm.beam_stop.counter = swap_uint16(current_beam_count);
     queue_elements_list[queue_element_write].elm.beam_stop.rssi = zpal_radio_get_last_beam_rssi();
     queue_element_write = (queue_element_write +1) % ZNIFFER_QUEUE_SIZE;
   }
   else if (elem_type == ZNIFFER_DATA)
   {
     uint8_t channel_and_speed = (GetRadioChannel(pRxParameters->channel_id, pRxParameters->speed) << 5) + GetRadioSpeed(pRxParameters->speed);
     int8_t rssi = pRxParameters->rssi;
     uint16_t sample_time = get_16bit_tick_reversed();
     queue_elements_list[queue_element_write].frame_type = ZNIFFER_DATA;
     queue_elements_list[queue_element_write].elm.zniffer_data.timestamp = sample_time;
     queue_elements_list[queue_element_write].elm.zniffer_data.ch_and_speed = channel_and_speed;
     queue_elements_list[queue_element_write].elm.zniffer_data.region = internal_region_to_zpal_region(current_internal_region);
     queue_elements_list[queue_element_write].elm.zniffer_data.rssi = rssi;
     queue_elements_list[queue_element_write].elm.zniffer_data.frame_length = pZpalFrame->frame_content_length;
     memcpy(queue_elements_list[queue_element_write].elm.zniffer_data.frame_data, pZpalFrame->frame_content, pZpalFrame->frame_content_length);
     queue_element_write =  (queue_element_write + 1) % ZNIFFER_QUEUE_SIZE;
   }
}

zpal_radio_region_t internal_region_to_zpal_region(zpal_radio_region_t internal_region)
{
  if ((INTERNAL_REGION_US_LR_920 == internal_region) || (INTERNAL_REGION_US_LR_END_DEVICE == internal_region))
  {
    return REGION_US_LR;
  }
  else if ((INTERNAL_REGION_EU_LR_866_4 == internal_region) || (INTERNAL_REGION_EU_LR_END_DEVICE == internal_region))
  {
    return REGION_EU_LR;
  }
  return internal_region;
}

zpal_radio_lr_channel_config_t internal_region_to_channel_config(zpal_radio_region_t region)
{
  switch(region)
  {
    case REGION_EU:   __attribute__ ((fallthrough));
    case REGION_US:
    case REGION_ANZ:
    case REGION_HK:
    case REGION_IN:
    case REGION_IL:
    case REGION_RU:
    case REGION_CN:
    case REGION_JP:
    case REGION_KR:
      return ZPAL_RADIO_LR_CH_CFG_NO_LR;

    case REGION_US_LR:  __attribute__ ((fallthrough));
    case REGION_EU_LR:
      return ZPAL_RADIO_LR_CH_CFG1;

    case INTERNAL_REGION_US_LR_920:  __attribute__ ((fallthrough));
    case INTERNAL_REGION_EU_LR_866_4:
      return ZPAL_RADIO_LR_CH_CFG2;

    case INTERNAL_REGION_US_LR_END_DEVICE:  __attribute__ ((fallthrough));
    case INTERNAL_REGION_EU_LR_END_DEVICE:
      return ZPAL_RADIO_LR_CH_CFG3;

    default:
      return ZPAL_RADIO_LR_CH_CFG_NO_LR;
  }
}

// RF Rx completion signaling
static void
RXHandlerFromISR(zpal_radio_event_t rxStatus)
{
  BaseType_t xHigherPriorityTaskWoken;
  BaseType_t status = pdPASS;

  xHigherPriorityTaskWoken = pdFALSE;
  switch (rxStatus)
  {
    case ZPAL_RADIO_EVENT_RX_BEAM_COMPLETE:
    {
      status = xTaskNotifyFromISR(g_ZnifferTaskHandle,
                                  ZNIFFER_EVENT_RF_RX_BEAM,
                                  eSetBits,
                                  &xHigherPriorityTaskWoken);
    }
    break;

    case ZPAL_RADIO_EVENT_RX_ABORT:
    {
      status = xTaskNotifyFromISR(g_ZnifferTaskHandle,
                                  ZNIFFER_EVENT_RF_RX_FRAME_RECEIVED,
                                  eSetBits,
                                  &xHigherPriorityTaskWoken);
    }
    break;

    case ZPAL_RADIO_EVENT_RX_COMPLETE:
    {
      status = xTaskNotifyFromISR(g_ZnifferTaskHandle,
                                  ZNIFFER_EVENT_RF_RX_FRAME_RECEIVED,
                                  eSetBits,
                                  &xHigherPriorityTaskWoken);
    }
    break;

    case ZPAL_RADIO_EVENT_RXTX_CALIBRATE:
    {
      status = xTaskNotifyFromISR(g_ZnifferTaskHandle,
                                  ZNIFFER_EVENT_RF_RX_FRAME_RECEIVED,
                                  eSetBits,
                                  &xHigherPriorityTaskWoken);
    }
    break;

    case ZPAL_RADIO_EVENT_RX_TIMEOUT:
    {
      status = xTaskNotifyFromISR(g_ZnifferTaskHandle,
                                  ZNIFFER_EVENT_RF_RX_TIMEOUT,
                                  eSetBits,
                                  &xHigherPriorityTaskWoken);
    }
    break;

    default:
      status = ~pdPASS;
      break;
  }

  ASSERT(status == pdPASS);
  portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

// RF Tx completion signaling
static void
TXHandlerFromISR(__attribute__((unused)) zpal_radio_event_t txStatus)
{
  // Empty on purpose
}

/*
 * Handler for asserts comming from radio,
 */
static void
RadioAssertHandler(__attribute__((unused)) zpal_radio_event_t assertVal)
{
  // portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

static void
RegionChangeHandler(__attribute__((unused)) zpal_radio_event_t regionChangeStatus)
{
}

static void
UartReceiveEvent(void)
{
  BaseType_t xHigherPriorityTaskWoken;
  BaseType_t status = pdPASS;

  status = xTaskNotifyFromISR(g_ZnifferTaskHandle,
                              ZNIFFER_EVENT_UART_RX,
                              eSetBits,
                              &xHigherPriorityTaskWoken);

  ASSERT(status == pdPASS);
  portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void zniffer_reply_no_data(uint8_t command)
{
  comm_interface_transmit_command(command, NULL, 0, NULL);
}

void zniffer_reply_data(uint8_t command, uint8_t *payload, uint8_t length)
{
  comm_interface_transmit_command(command, payload, length, NULL);
}

#ifdef TEST_DATA
void zniffer_send_frame(const uint8_t *payload, uint8_t len)
{
  // Test frame - channel 1 (9.6kb and 40kn channel), speed 0 (9.kb)
  comm_interface_transmit_frame(get_16bit_tick_reversed(), (1 << 5) + 0, internal_region_to_zpal_region(current_internal_region), -10, payload, len, NULL);
}
#endif

void zniffer_store_current_region()
{
  zniffer_storage.stored_region = current_internal_region;
  zpal_nvm_write(zniffer_nvm_handle, ZNIFFER_FILE_ID, &zniffer_storage, sizeof(zniffer_storage));
}

void zniffer_handle_command(comm_interface_command_t *frame)
{
  uint8_t payload[64];
  const uint8_t num_regions = sizeof(region_table)/sizeof(zniffer_region_type_t);

  switch (frame->cmd)
  {
    case ZNIFFER_CMD_VERSION:
      DPRINT("CmdVersionGet\n");
      payload[0] = zpal_get_chip_type();
      payload[1] = 0;//zpal_get_chip_revision();
      payload[2] = 11;//zpal_get_app_version_major();
      payload[3] = 0;//zpal_get_app_version_minor();
      zniffer_reply_data(ZNIFFER_CMD_VERSION, payload, 4);
      break;

    case ZNIFFER_CMD_SET_REGION:
      DPRINT("CmdRegionSet: ");
      current_internal_region = frame->payload[0];
      zniffer_store_current_region();

      DPRINTF("%u\n", current_internal_region);
      zpal_radio_change_region(internal_region_to_zpal_region(current_internal_region), internal_region_to_channel_config(current_internal_region));
      break;

    case ZNIFFER_CMD_GET_REGIONS:
      DPRINTF("CmdRegionsGet: %d\n", num_regions);
      payload[0] = zpal_radio_get_region();

      for (uint8_t i = 0; i < num_regions; i++)
      {
        payload[1+i] = region_table[i].region;
      }
      zniffer_reply_data(ZNIFFER_CMD_GET_REGIONS, payload, num_regions + 1);
      break;

    case ZNIFFER_CMD_GET_REGION_STR:
      DPRINT("CmdRegionStringGet: ");
      uint8_t region_number = frame->payload[0];
      uint8_t payload_length = 0;

      // Find the description for the given region
      bool region_found = false;
      for (uint8_t i = 0; i < num_regions; i++)
      {
        if (region_table[i].region == region_number)
        {
          payload[0] = region_number;
          payload[1] = region_table[i].supported_channels;
          payload_length = strlen(region_table[i].region_string);
          memcpy(&payload[2], region_table[i].region_string, payload_length + 1);
          region_found = true;
          break;
        }
      }
      if (true == region_found)
      {
        DPRINTF("Region %d, %s\n", region_number, (char *) &payload[2]);
        zniffer_reply_data(ZNIFFER_CMD_GET_REGION_STR, payload, payload_length + 2);
      }
      else
      {
        DPRINTF("Region %d not supported\n", region_number);
        zniffer_reply_no_data(ZNIFFER_CMD_GET_REGION_STR);
      }
      break;

    case ZNIFFER_CMD_START:
      zniffer_started = true;
      DPRINT("CmdStart\n");
      zniffer_reply_no_data(ZNIFFER_CMD_START);
      zpal_radio_start_receive();
      break;

    case ZNIFFER_CMD_STOP:
      zniffer_started = false;
      DPRINT("CmdStop\n");
      zniffer_reply_no_data(ZNIFFER_CMD_STOP);
      zpal_radio_power_down();
      break;

    case ZNIFFER_CMD_BAUD_RATE:
      DPRINT("CmdBaudRate ");
      // baud rate 0 -> 115200, 1 -> 230400
      uint8_t new_baud_rate;
      new_baud_rate = frame->payload[0];
      DPRINTF("%d\n", new_baud_rate);
      if (0 == new_baud_rate)
      {
        // 115200 baudrate
        zniffer_reply_no_data(ZNIFFER_CMD_BAUD_RATE);
      }
      else if (1 == new_baud_rate)
      {
        // 230400 baudrate
        zniffer_reply_no_data(ZNIFFER_CMD_BAUD_RATE);
      }
      break;

    default:
      DPRINTF("CmdUnknown - %d\n", frame->cmd);
      break;
  }
}

uint8_t GetRadioSpeed(zpal_radio_speed_t speed)
{
  switch(speed)
  {
    case ZPAL_RADIO_SPEED_9600:
      return 0;
    case ZPAL_RADIO_SPEED_40K:
      return 1;
    case ZPAL_RADIO_SPEED_100K:
      return 2;
    case ZPAL_RADIO_SPEED_100KLR:
      return 3;
    default:
      return 0;
  }
}

uint8_t GetRadioChannel(zpal_radio_zwave_channel_t channel_id, zpal_radio_speed_t speed)
{
  uint8_t region = zpal_radio_get_region();
  switch(region)
  {
    case REGION_EU:   __attribute__ ((fallthrough));
    case REGION_US:
    case REGION_ANZ:
    case REGION_HK:
    case REGION_IN:
    case REGION_IL:
    case REGION_RU:
    case REGION_CN:
      if (ZPAL_RADIO_SPEED_9600 == speed || ZPAL_RADIO_SPEED_40K == speed)
      {
        return 1;
      }
      return 0;

    case REGION_US_LR:  __attribute__ ((fallthrough));
    case INTERNAL_REGION_US_LR_920:
    case REGION_EU_LR:
    case INTERNAL_REGION_EU_LR_866_4:
      if (ZPAL_RADIO_SPEED_9600 == speed || ZPAL_RADIO_SPEED_40K == speed)
      {
        return 1;
      }
      return channel_id;

    case INTERNAL_REGION_US_LR_END_DEVICE:  __attribute__ ((fallthrough));
    case INTERNAL_REGION_EU_LR_END_DEVICE:
      return channel_id;

    case REGION_JP:   __attribute__ ((fallthrough));
    case REGION_KR:
      return channel_id;

    default:
      return 0;
  }
}

void zniffer_frame_receive_handler(zpal_radio_rx_parameters_t * pRxParameters, zpal_radio_receive_frame_t * pZpalFrame)
{
  if (0 != current_beam_count)
  {
    enqueue_zniffer_element(ZNIFFER_BEAM_STOP, NULL, NULL);
    if (comm_interface_was_transmit_done())
    {
      comm_interface_transmit_beam_stop(0,
                                        queue_elements_list[queue_element_read].elm.beam_stop.rssi,
                                        queue_elements_list[queue_element_read].elm.beam_stop.counter,
                                        zniffer_frame_transmit_done);
    }
    current_beam_count = 0;
    xTimerStop(beam_stop_timer.handler, 0);
  }
  // channel = 0, speed = 100kb
  enqueue_zniffer_element(ZNIFFER_DATA, pRxParameters, pZpalFrame);
  if (comm_interface_was_transmit_done())
  {
    comm_interface_transmit_frame(queue_elements_list[queue_element_read].elm.zniffer_data.timestamp,
                                  queue_elements_list[queue_element_read].elm.zniffer_data.ch_and_speed,
                                  queue_elements_list[queue_element_read].elm.zniffer_data.region,
                                  queue_elements_list[queue_element_read].elm.zniffer_data.rssi,
                                  queue_elements_list[queue_element_read].elm.zniffer_data.frame_data,
                                  queue_elements_list[queue_element_read].elm.zniffer_data.frame_length,
                                  zniffer_frame_transmit_done);
  }
}

zpal_radio_speed_t GetBeamSpeed(zpal_radio_zwave_channel_t channel)
{
  uint8_t region = zpal_radio_get_region();
  switch(region)
  {
    case REGION_EU:  __attribute__ ((fallthrough));
    case REGION_US:
    case REGION_ANZ:
    case REGION_HK:
    case REGION_IN:
    case REGION_IL:
    case REGION_RU:
    case REGION_CN:
      return ZPAL_RADIO_SPEED_40K;

    case REGION_US_LR:  __attribute__ ((fallthrough));
    case INTERNAL_REGION_US_LR_920:
    case REGION_EU_LR:
    case INTERNAL_REGION_EU_LR_866_4:
      // Any non-LR channels will use 40K
      if (channel <= ZPAL_RADIO_ZWAVE_CHANNEL_2)
      {
        return ZPAL_RADIO_SPEED_40K;
      }   __attribute__ ((fallthrough));

    case INTERNAL_REGION_EU_LR_END_DEVICE:  __attribute__ ((fallthrough));
    case INTERNAL_REGION_US_LR_END_DEVICE:
      return ZPAL_RADIO_SPEED_100KLR;

    case REGION_JP:  __attribute__ ((fallthrough));
    case REGION_KR:
      return ZPAL_RADIO_SPEED_100K;

    default:
      return 0;
  }
}

/*
 * Handler for Beam received,
 */
static void zniffer_event_handler_beam_receive(void)
{
  rxBeamReceived++;
  if (0 == current_beam_count)
  {
    enqueue_zniffer_element(ZNIFFER_BEAM_START, NULL, NULL);
    if (comm_interface_was_transmit_done())
    {
      comm_interface_transmit_beam_start(0,
                                         queue_elements_list[queue_element_read].elm.zniffer_data.ch_and_speed,
                                         queue_elements_list[queue_element_read].elm.zniffer_data.region,
                                         queue_elements_list[queue_element_read].elm.zniffer_data.rssi,
                                         (const uint8_t*)queue_elements_list[queue_element_read].elm.zniffer_data.frame_data,
                                         queue_elements_list[queue_element_read].elm.zniffer_data.frame_length,
                                         zniffer_frame_transmit_done);
    }

    xTimerStart(beam_stop_timer.handler, 0);
  }
  else
  {
    xTimerReset(beam_stop_timer.handler, 0);
  }
  current_beam_count++;
}

/*
 * Handler for receive timeout
 */
static void zniffer_event_handler_receive_timeout(void)
{
  rxTimeout++;

  if (0 != current_beam_count)
  {
    enqueue_zniffer_element(ZNIFFER_BEAM_STOP, NULL, NULL);
    if (comm_interface_was_transmit_done())
    {
      comm_interface_transmit_beam_stop(0,
                                        queue_elements_list[queue_element_read].elm.beam_stop.rssi,
                                        queue_elements_list[queue_element_read].elm.beam_stop.counter,
                                        zniffer_frame_transmit_done);
    }
    current_beam_count = 0;
    xTimerStop(beam_stop_timer.handler, 0);
  }
}

/*=========================   zniffer_radio_setup   ===========================
**    Z-Wave zniffer radio setup
**--------------------------------------------------------------------------*/
void zniffer_radio_setup(void)
{
  // Default region
  zpal_radio_region_t eRegion = internal_region_to_zpal_region(current_internal_region);

  // Configure radio
  zpal_radio_wakeup_t eWakeup = ZPAL_RADIO_WAKEUP_ALWAYS_LISTEN;

  if (ZPAL_RADIO_PROTOCOL_MODE_UNDEFINED == zpal_radio_region_get_protocol_mode(eRegion, internal_region_to_channel_config(eRegion)))
  {
    DPRINTF("FATAL ERROR: Invalid region code: %d\n", eRegion);
    ASSERT(false);
  }

  zpal_radio_profile_t RfProfile = {.region = eRegion,
                                    .wakeup = eWakeup,
                                    .listen_before_talk_threshold = -100,
                                    .tx_power_max = 140,
                                    .tx_power_adjust = 0,
                                    .tx_power_max_lr = 140,
                                    .home_id = network_homeid,
                                    .rx_cb = RXHandlerFromISR,
                                    .tx_cb = TXHandlerFromISR,
                                    .region_change_cb = RegionChangeHandler,
                                    .assert_cb = RadioAssertHandler,
                                    .receive_handler_cb = zniffer_frame_receive_handler,
                                    .network_stats = &sNetworkStatistic,
                                    .radio_debug_enable = false,
                                    .lr_channel_auto_mode = false,
                                    .primary_lr_channel = ZPAL_RADIO_LR_CHANNEL_UNINITIALIZED,
                                    .active_lr_channel_config = internal_region_to_channel_config(current_internal_region)
  };

  zpal_radio_rssi_config_set(0, 0);

  zpal_radio_init(&RfProfile);
}

#ifdef TEST_DATA // Test code

static zpal_radio_transmit_parameter_t TxParameter9_6kCh2   = {.speed = ZPAL_RADIO_SPEED_9600, .channel_id = 2, .crc = ZPAL_RADIO_CRC_8_BIT_XOR,   .preamble = 0x55, .preamble_length = 10, .start_of_frame = 0xF0, .repeats = 0};
static zpal_radio_transmit_parameter_t TxParameter40kCh1    = {.speed = ZPAL_RADIO_SPEED_40K,  .channel_id = 1, .crc = ZPAL_RADIO_CRC_8_BIT_XOR,   .preamble = 0x55, .preamble_length = 20, .start_of_frame = 0xF0, .repeats = 0};
static zpal_radio_transmit_parameter_t TxParameter100kCh0   = {.speed = ZPAL_RADIO_SPEED_100K, .channel_id = 0, .crc = ZPAL_RADIO_CRC_16_BIT_CCITT,.preamble = 0x55, .preamble_length = 40, .start_of_frame = 0xF0, .repeats = 0};
static zpal_radio_transmit_parameter_t TxParameter100k3Ch0  = {.speed = ZPAL_RADIO_SPEED_100K, .channel_id = 0, .crc = ZPAL_RADIO_CRC_16_BIT_CCITT, .preamble = 0x55, .preamble_length = 24, .start_of_frame = 0xF0, .repeats = 0};
static zpal_radio_transmit_parameter_t TxParameter100k3Ch1  = {.speed = ZPAL_RADIO_SPEED_100K, .channel_id = 1, .crc = ZPAL_RADIO_CRC_16_BIT_CCITT, .preamble = 0x55, .preamble_length = 24, .start_of_frame = 0xF0, .repeats = 0};
static zpal_radio_transmit_parameter_t TxParameter100k3Ch2  = {.speed = ZPAL_RADIO_SPEED_100K, .channel_id = 2, .crc = ZPAL_RADIO_CRC_16_BIT_CCITT, .preamble = 0x55, .preamble_length = 24, .start_of_frame = 0xF0, .repeats = 0};
static zpal_radio_transmit_parameter_t TxParameter100kLRCh3 = {.speed = ZPAL_RADIO_SPEED_100K, .channel_id = 3, .crc = ZPAL_RADIO_CRC_16_BIT_CCITT, .preamble = 0x00, .preamble_length = 40, .start_of_frame = 0x5E, .repeats = 0};
static zpal_radio_transmit_parameter_t TxParameter100kLRCh4 = {.speed = ZPAL_RADIO_SPEED_100K, .channel_id = 4, .crc = ZPAL_RADIO_CRC_16_BIT_CCITT, .preamble = 0x00, .preamble_length = 40, .start_of_frame = 0x5E, .repeats = 0};

zpal_radio_transmit_parameter_t* frame_types[4][4] = {{&TxParameter9_6kCh2, &TxParameter40kCh1, &TxParameter100kCh0},
                                                      {&TxParameter100k3Ch0, &TxParameter100k3Ch1, &TxParameter100k3Ch2},
                                                      {&TxParameter9_6kCh2, &TxParameter40kCh1, &TxParameter100kCh0, &TxParameter100kLRCh3},
                                                      {&TxParameter100kLRCh3, &TxParameter100kLRCh4}};
uint8_t protocolmode_channel_count[4] = {3, 3, 4, 2};

uint8_t frame_type_index = 0;
uint8_t msdu_length_index = 0;
uint32_t frameTransmitted = 0;
uint8_t protocolmode = ZPAL_RADIO_PROTOCOL_MODE_1;

static bool send_test_frame()
{
  if (true == zniffer_started)
  {
    // header
    uint8_t header_buffer[] = {0xC7, 0xB8, 0x16, 0x36, 0x01, 0x41, 0x09, 0x09, 0x02};
    uint8_t payload_buffer[] = { 0x20, 0x01, 0xAA, 0x55, 0xAA, 0x55, 0xAA};

    uint8_t header_bufferLR[] = {0xFB, 0xD3, 0xDB, 0x7C, 0x00, 0x11, 0x01, 0x11, 0x81, 0x19, 0x98, 0xFD};
    uint8_t payload_bufferLR[] =  {0x20, 0x01, 0xFF, 0x5A, 0xA5, 0x5A, 0xA5};
    uint8_t msdu_size[] = {2, 3, 4, 5, 6, 7};

    protocolmode = zpal_radio_get_protocol_mode();
    frame_type_index %= protocolmode_channel_count[protocolmode];
    zpal_radio_transmit(frame_types[protocolmode][frame_type_index],
                        ((protocolmode == 3) || (3 == frame_type_index)) ? sizeof_array(header_bufferLR) : sizeof_array(header_buffer),
                        ((protocolmode == 3) || (3 == frame_type_index)) ? (uint8_t *)&header_bufferLR : (uint8_t *)&header_buffer,
                        msdu_size[msdu_length_index],
                        (uint8_t *)&payload_buffer,
                        true,
                        0);
    frameTransmitted++;
    if (0 == frameTransmitted % 6)
    {
      frame_type_index = (frame_type_index + 1) % protocolmode_channel_count[protocolmode];
    }
    msdu_length_index = (msdu_length_index + 1) % sizeof(msdu_size);
  }
  return true;
}

static void test_timer_cb(__attribute__((unused)) TimerHandle_t xTimer )
{
  transmit_rf_frame_to_host++;
}

#endif

static void beam_stop_timer_cb(__attribute__((unused)) TimerHandle_t xTimer)
{
  if (0 != current_beam_count)
  {
    enqueue_zniffer_element(ZNIFFER_BEAM_STOP, NULL, NULL);
    if (comm_interface_was_transmit_done())
    {
      comm_interface_transmit_beam_stop(0,
                                        queue_elements_list[queue_element_read].elm.beam_stop.rssi,
                                        queue_elements_list[queue_element_read].elm.beam_stop.counter,
                                        zniffer_frame_transmit_done);
    }
    current_beam_count = 0;
  }
  xTimerStop(beam_stop_timer.handler, 0);
}
static void zniffer_event_handler_uart_receive(void)
{
  comm_interface_parse_result_t interface_status;
  comm_interface_command_t* command;

  interface_status = comm_interface_parse_data();

  /* Check if we received a command */
  if (PARSE_FRAME_RECEIVED == interface_status)
  {
    /* We got a command */
    command = comm_interface_get_command();
    zniffer_handle_command(command);
  }

  if(comm_interface_data_available())
  {
    UartReceiveEvent();
  }

}

/*=============================   ZwaveZnifferTask   ===============================
**    Z-Wave zniffer task
**    This function should be called in a forever loop
**--------------------------------------------------------------------------*/
void
ZwaveZnifferTask(void* unused_prt)
{
  zpal_status_t status;
  // Make ZwaveTaskhandle available outside Zwave task context
  g_ZnifferTaskHandle = xTaskGetCurrentTaskHandle();

#ifdef DEBUGPRINT
  // Setup debug printing
  zpal_debug_init(&uart_cfg);
  static uint8_t m_aDebugPrintBuffer[300];
  DebugPrintConfig(m_aDebugPrintBuffer, sizeof(m_aDebugPrintBuffer), zpal_debug_output);
  DPRINTF("ApplicationInit\n");
#endif
  // Initialize NVM storage
  zniffer_nvm_handle = zpal_nvm_init(ZPAL_NVM_AREA_APPLICATION);
  DPRINTF("Flash file system initialized %s\n", zniffer_nvm_handle == NULL ? "Failed" : "Succesfully");

  // Check if file exists
  status = zpal_nvm_read(zniffer_nvm_handle, ZNIFFER_FILE_ID, &zniffer_storage, sizeof(zniffer_storage));
  if (status == ZPAL_STATUS_OK)
  {
    current_internal_region = zniffer_storage.stored_region;
  }
  else
  {
    // File does not exist, create it
    zniffer_store_current_region();
  }
  DPRINTF("Current region set to %u\n", current_internal_region);

  // Configure event distributor
  EventDistributorConfig(&g_EventDistributor,
                         sizeof(g_aEventHandlerTable)/sizeof(*g_aEventHandlerTable),
                         g_aEventHandlerTable,
                         NULL);

  // Enable watchdog
  zpal_enable_watchdog(true);

  // Start the radio
  zniffer_radio_setup();

  // Register the protocol powerlocks, the radio will be off in M3 and above
  application_radio_power_lock = zpal_pm_register(ZPAL_PM_TYPE_USE_RADIO);
  zpal_pm_stay_awake(application_radio_power_lock,  0);

  // Initialize UART for host communication
  comm_interface_init(&ZNIFFER_UART_CONFIG, UartReceiveEvent);


  beam_stop_timer.handler = xTimerCreateStatic("",
                                               pdMS_TO_TICKS(10),
                                               pdFALSE,
                                               NULL,
                                               beam_stop_timer_cb,
                                               &beam_stop_timer.timer_buffer);

#ifdef TEST_DATA // Test code
  test_timer.handler = xTimerCreateStatic( "",
                                          pdMS_TO_TICKS(5000),
                                          pdTRUE,
                                          NULL,
                                          test_timer_cb,
                                          &test_timer.timer_buffer);
  xTimerStart(test_timer.handler, 100);
#endif

  DPRINT("Zniffer task started\n");
  uint32_t iEventWait = 0xFFFFF;
  for (;;)
  {
#ifdef TEST_DATA // Test code
    if (transmit_rf_frame_to_host)
    {
      if (send_test_frame())
      {
        transmit_rf_frame_to_host--;
      }
    }
#endif
    EventDistributorDistribute(&g_EventDistributor, iEventWait, 0);
    // We never want to return from the task
  }
}
