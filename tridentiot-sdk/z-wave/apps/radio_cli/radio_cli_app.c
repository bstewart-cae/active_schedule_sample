/// ***************************************************************************
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************

/**
 * @file radio_cli_app.c
 * @brief Z-Wave radio CLI application
 */

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <FreeRTOS.h>
#include <task.h>
#include <stdio.h>
#include <string.h>
#include <Assert.h>
#include <inttypes.h>

#include <FreeRTOS.h>
#include "task.h"
#include <timers.h>

// #define DEBUGPRINT // NOSONAR
#include "DebugPrint.h"
#include "DebugPrintConfig.h"
#include "SizeOf.h"
#include "EventDistributor.h"

#include <zpal_watchdog.h>
#include <zpal_power_manager.h>
#include <zpal_radio_utils.h>
#include <zpal_radio_private.h>
#include <zpal_entropy.h>
#include <zpal_init.h>
#include <zpal_misc.h>
#include <zpal_uart.h>
#include <zpal_nvm.h>

#include "rf_mcu.h"
#include "sysctrl.h"
#include "flashctl.h"

#include "zpal_misc_private.h"
#include "zpal_radio_private.h"
#include "tr_mfg_tokens.h"
#include "tr_tlv_parser.h"

#include "cli_uart_interface.h"
#include "radio_cli_app.h"

/****************************************************************************/
/*                         FUNCTION PROTOTYPES                              */
/****************************************************************************/
static void ECLIEventHandlerRfRxBeam(void);
static void ECLIEventHandlerRfRxTimeout(void);
static void ECLIEventHandlerRfTxComplete(void);
static void cli_uart_do_transmit(void);
static void ECLIEventHandlerScriptTransition(void);
static void ECLIEventHandlerWaitTimeout(void);

/****************************************************************************/
/*                              PRIVATE DATA                                */
/****************************************************************************/
TaskHandle_t g_radioCliTaskHandle;

typedef struct {
  zpal_radio_region_t region;
  uint8_t             supported_channels;
  char*               region_string;
} region_type_t;

// Make internal regions numbers for LR backup regions and end device regions as they do not
// exist as actual regions in zpal radio API
#define INTERNAL_REGION_US_LR_920        100
#define INTERNAL_REGION_EU_LR_866_4      101
#define INTERNAL_REGION_US_LR_END_DEVICE 102
#define INTERNAL_REGION_EU_LR_END_DEVICE 103

#define XTAL_TRIM_TAG     0x01
#define XTAL_TRIM_SIZE    0x02

const region_type_t region_table[] = {
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
  {REGION_UNDEFINED,                  0, "UNDEFINED"},
};

typedef struct {
  char reset_reason_string[20];
} reset_reason_string_t;

const reset_reason_string_t reset_reason_table[] = {
  {"PIN"},                    ///< ZPAL_RESET_REASON_PIN                   0 - Reset triggered by reset pin.
  {"DEEP_SLEEP_WUT"},         ///< ZPAL_RESET_REASON_DEEP_SLEEP_WUT        1 - Reset triggered by wake up by timer from deep sleep state.
  {"UNDEFINED"},              ///< UNDEFINED                               2 - Reset triggered by undefined reason.
  {"WATCHDOG"},               ///< ZPAL_RESET_REASON_WATCHDOG              3 - Reset triggered by watchdog.
  {"DEEP_SLEEP_EXT_INT"},     ///< ZPAL_RESET_REASON_DEEP_SLEEP_EXT_INT    4 - Reset triggered by external interrupt event in deep sleep state.
  {"POWER_ON"},               ///< ZPAL_RESET_REASON_POWER_ON              5 - Reset triggered by power on.
  {"UNDEFINED"},              ///< UNDEFINED                               6 - Reset triggered by undefined reason.
  {"SOFTWARE"},               ///< ZPAL_RESET_REASON_SOFTWARE              7 - Reset triggered by software.
  {"UNDEFINED"},              ///< UNDEFINED                               8 - Reset triggered by undefined reason.
  {"BROWNOUT"},               ///< ZPAL_RESET_REASON_BROWNOUT              9 - Reset triggered by brownout circuit.
  {"TAMPER"},                 ///< ZPAL_RESET_REASON_TAMPER               10 - Reset triggered by a tamper attempt.
  {"OTHER"},                  ///< ZPAL_RESET_REASON_OTHER              0xFF - Reset triggered by none of the above.
};

#define RESET_REASON_MAX sizeof_array(reset_reason_table)

// Radio status and info
typedef struct  {
  uint32_t  homeID_uint32;
  uint8_t   network_homeid[4];
  node_id_t node_id;
} internal_network_ids_t;

typedef struct {
  uint32_t rx_frames;
  uint32_t rx_beams;
  uint32_t rx_foreign_homeid;
  uint32_t tx_frames;
  uint32_t tx_fail;
  uint32_t tx_fail_lbt;
} radio_channel_stats_t;

static internal_network_ids_t internal_network = {0x00000000, {0x0, 0x0, 0x0, 0x0}, 0};

static zpal_radio_network_stats_t sNetworkStatistic = {0};
static bool radio_initialized = false;

static radio_channel_stats_t radio_cli_radio_extended_channel_stats[5] = {{ 0 }};

// Transmit buffer for repeated frames
typedef struct {
  uint8_t frame_length;
  uint8_t* frame_buffer;
  uint32_t frame_repeat;
  bool wait_ack;
  zpal_radio_zwave_channel_t channel;
  uint32_t frame_delay_in_between;
  int8_t power;
  bool lbt;
  tx_callback_t tx_callback;
  uint32_t success;
  uint32_t failed;
  uint32_t failed_lbt;
  TimerHandle_t delay_timer;
  StaticTimer_t delay_timer_buffer;
} internal_frame_buffer_t;

static internal_frame_buffer_t internal_buffer;

// Receiver status
typedef struct {
  bool receiver_running;
  bool receiver_running_single_channel_scan;
  uint8_t channel;
  zpal_radio_receive_handler_t rx_callback;
  uint32_t receive_count;
  uint32_t beam_count;
} internal_rx_t;

static internal_rx_t rx_status = {.receiver_running = false,
                                  .receiver_running_single_channel_scan = false,
                                  .channel = 0,
                                  .rx_callback = NULL,
                                  .receive_count = 0,
                                  .beam_count = 0};

// NVM file ID defines
#define CLI_FILE_ID                     800
#define CLI_FILE_CAL_SETTING_ID         (CLI_FILE_ID + 1) // Reserved
#define CLI_FILE_SCRIPT_AUTORUN_ID      (CLI_FILE_CAL_SETTING_ID + 1)
#define CLI_FILE_SCRIPT_ID              (CLI_FILE_SCRIPT_AUTORUN_ID + 1)
#define CLI_FILE_SCRIPT_DESCRIPTOR_ID   (CLI_FILE_SCRIPT_ID + 11)

char const git_hash[40] = GIT_HASH_ID;

static zpal_nvm_handle_t cli_nvm_handle;
static zpal_radio_region_t current_region = REGION_US_LR;
static bool current_radio_rf_state_enable = false;
static bool current_radio_tx_max_power_20dbm = false;
static uint8_t current_script_autorun = 1;
static bool current_script_running = false;
static bool current_script_run_stop = false;
static zpal_reset_reason_t last_reset_reason = 0;
static bool timestamp_enabled = false;

static bool m_zw_classic_tx_power_enable = false;

typedef struct
{
  uint8_t region;
  uint8_t radio_rf_state_enable;
  bool tx_max_power_20dbm;
} cli_file_t;

typedef struct
{
  uint8_t autorun;
} cli_script_autorun_file_t;

typedef struct
{
  uint16_t script_version;
  uint8_t scriptlines;
  uint8_t script_active;
  uint32_t script_length;
} cli_script_file_descriptor_t;

typedef struct
{
  uint8_t script_data[32*200];
} cli_script_file_t;

static cli_script_file_t cli_script;

static cli_script_autorun_file_t cli_script_autorun = {1};

static cli_file_t cli_storage = {.region = REGION_US_LR,
                                 .radio_rf_state_enable = false,
                                 .tx_max_power_20dbm = false};

static bool transmit_option_force_checksum_errors = false;

static const uint8_t payload_default[5][17] = {{0xCD, 0x68, 0x98, 0x5F, 0x01, 0x41, 0x01, 0x0E, 0x04, 0x9F, 0x01, 0x57}, // 100Kbaud
                                               {0xCD, 0x68, 0x98, 0x5F, 0x01, 0x41, 0x01, 0x0D, 0x04, 0x20, 0x01, 0x40}, // 40kbaud
                                               {0xCD, 0x68, 0x98, 0x5F, 0x01, 0x41, 0x01, 0x0D, 0x04, 0x20, 0x01, 0x96}, // 9.6Kbaud
                                               {0xCD, 0x68, 0x98, 0x5F, 0x00, 0x11, 0x02, 0x11, 0x81, 0x1F, 0x9B, 0xFD, 0x9F, 0x01, 0x55}, // LR 100Kbaud
                                               {0xCD, 0x68, 0x98, 0x5F, 0x01, 0x81, 0x00, 0x0F, 0x0E, 0x04, 0x9F, 0x01, 0x56}}; // 3channel 100Kbaud

#ifdef DEBUGPRINT
  static uint8_t tx_buffer[64] __attribute__ ((aligned (4)));
  static uint8_t rx_buffer[32] __attribute__ ((aligned (4)));
  #define TX_BUFFER_SIZE sizeof(tx_buffer)
  #define RX_BUFFER_SIZE sizeof(rx_buffer)

  static zpal_uart_config_t uart_cfg = {.id = ZPAL_UART0,
                                        .tx_buffer = tx_buffer,
                                        .tx_buffer_len = TX_BUFFER_SIZE,
                                        .rx_buffer = rx_buffer,
                                        .rx_buffer_len = RX_BUFFER_SIZE,
                                        .baud_rate = 230400,
                                        .data_bits = 8,
                                        .parity_bit = ZPAL_UART_NO_PARITY,
                                        .stop_bits = ZPAL_UART_STOP_BITS_1,
                                        .ptr = NULL,
                                       };
#endif

// Prioritized events that can wakeup protocol thread.
typedef enum ECLIEvent
{
  ECLIEVENT_RFRXBEAM,
  ECLIEVENT_RFRXTIMEOUT,
  ECLIEVENT_RFRX,
  ECLIEVENT_RFTX,
  ECLIEVENT_UARTRX,
  ECLIEVENT_APP_RFTX,
  ECLIEVENT_SCRIPT_TRANSITION,
  ECLIEVENT_WAIT_TIMEOUT,
  ECLIEVENT_NUM
} ECLIEvent;

// Event distributor object
static SEventDistributor g_EventDistributor = { 0 };

// Event distributor event handler table
static const EventDistributorEventHandler g_aEventHandlerTable[ECLIEVENT_NUM] =
{
  ECLIEventHandlerRfRxBeam,           // Event 0
  ECLIEventHandlerRfRxTimeout,        // Event 1
  zpal_radio_get_last_received_frame, // Event 2
  ECLIEventHandlerRfTxComplete,       // Event 3
  cli_uart_receive_handler,           // Event 4
  cli_uart_do_transmit,               // Event 5
  ECLIEventHandlerScriptTransition,   // Event 6
  ECLIEventHandlerWaitTimeout,        // Event 7
};

#define CLI_EVENT_RF_RX_FRAME_RECEIVED   (1UL << ECLIEVENT_RFRX)
#define CLI_EVENT_RF_RX_BEAM_RECEIVED    (1UL << ECLIEVENT_RFRXBEAM)
#define CLI_EVENT_RF_RX_TIMEOUT          (1UL << ECLIEVENT_RFRXTIMEOUT)
#define CLI_EVENT_RF_TX_FRAME_COMPLETE   (1UL << ECLIEVENT_RFTX)
#define CLI_EVENT_UART_RX_RECEIVE        (1UL << ECLIEVENT_UARTRX)
#define CLI_EVENT_APP_RF_TRANSMIT        (1UL << ECLIEVENT_APP_RFTX)
#define CLI_EVENT_SCRIPT_TRANSITION      (1UL << ECLIEVENT_SCRIPT_TRANSITION)
#define CLI_EVENT_WAIT_TIMEOUT           (1UL << ECLIEVENT_WAIT_TIMEOUT)

extern uint32_t __mfg_tokens_production_region_start;

/**************************************************
 * FRAME BYTE INDENTIFIERS
 *************************************************/
#define BEAM_FRAME_BEAM_TAG         0x55  /**< This is the BEAM TAG allowing awake nodes to determine that this is a BEAM frame being transmitted. It is the same as the preamble! */

#define START_OF_FRAME_CH2CH3       0xF0  /**< The start of frame byte for 2CH and 3CH */
#define START_OF_FRAME_LR           0x5E  /**< The start of frame byte for Long-Range */

#define PREAMBLE_BYTE_CH2CH3        0x55  /**< The preamble byte used to create the preamble sequence for 2CH and 3CH */
#define PREAMBLE_BYTE_LR            0x00  /**< The preamble byte for LR (Due to spread-spectrum, 0x00 is not what is being transmitted!) */

/*******************************
 * Static const values used when transmitting frame on 9.6k, 40, 100k, and wakeup beams.
 * 2CH transmission parameters!
 ******************************/
static const zpal_radio_transmit_parameter_t regions24ch[4] = {
  {.speed = ZPAL_RADIO_SPEED_100K, .channel_id = ZPAL_RADIO_ZWAVE_CHANNEL_0, .crc = ZPAL_RADIO_CRC_16_BIT_CCITT,.preamble = PREAMBLE_BYTE_CH2CH3, .preamble_length = 40, .start_of_frame = START_OF_FRAME_CH2CH3, .repeats = 0},
  {.speed = ZPAL_RADIO_SPEED_40K,  .channel_id = ZPAL_RADIO_ZWAVE_CHANNEL_1, .crc = ZPAL_RADIO_CRC_8_BIT_XOR,   .preamble = PREAMBLE_BYTE_CH2CH3, .preamble_length = 20, .start_of_frame = START_OF_FRAME_CH2CH3, .repeats = 0},
  {.speed = ZPAL_RADIO_SPEED_9600, .channel_id = ZPAL_RADIO_ZWAVE_CHANNEL_2, .crc = ZPAL_RADIO_CRC_8_BIT_XOR,   .preamble = PREAMBLE_BYTE_CH2CH3, .preamble_length = 10, .start_of_frame = START_OF_FRAME_CH2CH3, .repeats = 0},
  {.speed = ZPAL_RADIO_SPEED_100KLR, .channel_id = ZPAL_RADIO_ZWAVE_CHANNEL_3, .crc = ZPAL_RADIO_CRC_16_BIT_CCITT, .preamble = PREAMBLE_BYTE_LR, .preamble_length = 40, .start_of_frame = START_OF_FRAME_LR, .repeats = 0},
};

/*******************************
 * 3CH transmission parameters!
 ******************************/
static const zpal_radio_transmit_parameter_t regions3ch[3] = {
  {.speed = ZPAL_RADIO_SPEED_100K, .channel_id = ZPAL_RADIO_ZWAVE_CHANNEL_0, .crc = ZPAL_RADIO_CRC_16_BIT_CCITT, .preamble = PREAMBLE_BYTE_CH2CH3, .preamble_length = 24, .start_of_frame = START_OF_FRAME_CH2CH3, .repeats = 0},
  {.speed = ZPAL_RADIO_SPEED_100K, .channel_id = ZPAL_RADIO_ZWAVE_CHANNEL_1, .crc = ZPAL_RADIO_CRC_16_BIT_CCITT, .preamble = PREAMBLE_BYTE_CH2CH3, .preamble_length = 24, .start_of_frame = START_OF_FRAME_CH2CH3, .repeats = 0},
  {.speed = ZPAL_RADIO_SPEED_100K, .channel_id = ZPAL_RADIO_ZWAVE_CHANNEL_2, .crc = ZPAL_RADIO_CRC_16_BIT_CCITT, .preamble = PREAMBLE_BYTE_CH2CH3, .preamble_length = 24, .start_of_frame = START_OF_FRAME_CH2CH3, .repeats = 0},
};

static const zpal_radio_transmit_parameter_t region2lrch[2] = {
  {.speed = ZPAL_RADIO_SPEED_100KLR, .channel_id = ZPAL_RADIO_ZWAVE_CHANNEL_3, .crc = ZPAL_RADIO_CRC_16_BIT_CCITT, .preamble = PREAMBLE_BYTE_LR, .preamble_length = 40, .start_of_frame = START_OF_FRAME_LR, .repeats = 0},
  {.speed = ZPAL_RADIO_SPEED_100KLR, .channel_id = ZPAL_RADIO_ZWAVE_CHANNEL_4, .crc = ZPAL_RADIO_CRC_16_BIT_CCITT, .preamble = PREAMBLE_BYTE_LR, .preamble_length = 40, .start_of_frame = START_OF_FRAME_LR, .repeats = 0},
};

/**
 * Script definitions
 */
// Example of a hardcode script which will after a radioCLI erase-program be run on device start-up
#ifdef USE_DEFAULT_SCRIPT_IF_NO_DYNAMIC_EXISTS
scriptline_t default_script[] = {
                                 {"zw-region-set 9",},
                                 {"zw-init"},
                                 {"zw-rx-channel-set on 0"},
                                 {"zw-tx-delay-set 10"},
                                 {"zw-tx-channel-set 0"},
                                 {"rx off"},
                                 {"zw-radio-tx-continues-set on"},
                                 {"wait 5000"},
                                 {"zw-radio-tx-continues-set off"},
                                 {"zw-tx-channel-set 1"},
                                 {"zw-radio-tx-continues-set on"},
                                 {"wait 5000"},
                                 {"zw-radio-tx-continues-set off"},
                                 {"zw-tx-channel-set 2"},
                                 {"zw-radio-tx-continues-set on"},
                                 {"wait 5000"},
                                 {"zw-radio-tx-continues-set off"},
                                 {"zw-tx-channel-set 3"},
                                 {"zw-radio-tx-continues-set on"},
                                 {"wait 5000"},
                                 {"zw-radio-tx-continues-set off"},
                                 {"tx 1000"},
                                 {"zw-stats-get"},
                                };

#define DEFAULT_SCRIPT_COUNT (sizeof_array(default_script))
#endif

typedef scriptline_t (script_t)[200];

script_t dynamic_script = {0};
uint8_t script_file_number = 1;

#define DYNAMIC_SCRIPT_MAXCOUNT (sizeof_array(dynamic_script))

uint8_t dynamic_script_active = 0;
uint8_t dynamic_scriptlines = 0;
#ifdef USE_DEFAULT_SCRIPT_IF_NO_DYNAMIC_EXISTS
uint8_t active_scriptlines = DEFAULT_SCRIPT_COUNT;
scriptline_t *active_script = &default_script[0];
#else
uint8_t active_scriptlines = 0;
scriptline_t *active_script = &dynamic_script[0];
#endif
event_handler_script_callback_t scriptTransitionEvent = NULL;
TimerHandle_t wait_timer;
StaticTimer_t wait_timer_buffer;

/****************************************************************************/
/*                         FUNCTION PROTOTYPES                              */
/****************************************************************************/
bool helper_radio_transmit(internal_frame_buffer_t frame);

/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/
zpal_pm_handle_t application_radio_power_lock;

extern zpal_uart_config_t CLI_UART_CONFIG;

/****************************************************************************/
/*                              STUB FUNCTIONS                              */
/****************************************************************************/

/****************************************************************************/
/*                            PRIVATE FUNCTIONS                             */
/****************************************************************************/
/*
 * homeID converter function
 */
void home_id_convert(uint8_t *dest, uint32_t src)
{
  dest[0] = (uint8_t)((src >> 24) & 0xFF);
  dest[1] = (uint8_t)((src >> 16) & 0xFF);
  dest[2] = (uint8_t)((src >> 8) & 0xFF);
  dest[3] = (uint8_t)(src & 0xFF);
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
      return ZPAL_RADIO_LR_CH_CFG_COUNT;
  }
}

/*
 * Rx complete interrupt handler
 */
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
      status = xTaskNotifyFromISR(g_radioCliTaskHandle,
                                  CLI_EVENT_RF_RX_BEAM_RECEIVED,
                                  eSetBits,
                                  &xHigherPriorityTaskWoken);
    }
    break;

    case ZPAL_RADIO_EVENT_RX_TIMEOUT:
    {
      status = xTaskNotifyFromISR(g_radioCliTaskHandle,
                                  CLI_EVENT_RF_RX_TIMEOUT,
                                  eSetBits,
                                  &xHigherPriorityTaskWoken);
    }
    break;

    case ZPAL_RADIO_EVENT_RX_ABORT:
    {
      status = xTaskNotifyFromISR(g_radioCliTaskHandle,
                                  CLI_EVENT_RF_RX_FRAME_RECEIVED,
                                  eSetBits,
                                  &xHigherPriorityTaskWoken);
    }
    break;

    case ZPAL_RADIO_EVENT_RX_COMPLETE:
    {
      status = xTaskNotifyFromISR(g_radioCliTaskHandle,
                                  CLI_EVENT_RF_RX_FRAME_RECEIVED,
                                  eSetBits,
                                  &xHigherPriorityTaskWoken);
    }
    break;

    case ZPAL_RADIO_EVENT_RXTX_CALIBRATE:
    {
      status = xTaskNotifyFromISR(g_radioCliTaskHandle,
                                  CLI_EVENT_RF_RX_FRAME_RECEIVED,
                                  eSetBits,
                                  &xHigherPriorityTaskWoken);
    }
    break;

    default:
      break;
  }

  ASSERT(status == pdPASS);
  portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

/*
 * Tx complete interrupt handler
 */
static void
TXHandlerFromISR(zpal_radio_event_t txStatus)
{
  BaseType_t xHigherPriorityTaskWoken;
  BaseType_t status = pdPASS;

  xHigherPriorityTaskWoken = pdFALSE;

  switch(txStatus)
  {
    case ZPAL_RADIO_EVENT_TX_BEAM_COMPLETE:
      internal_buffer.success++;
      break;

    case ZPAL_RADIO_EVENT_TX_COMPLETE:
      internal_buffer.success++;
      radio_cli_radio_extended_channel_stats[zpal_radio_get_last_tx_channel()].tx_frames++;
      break;

    case ZPAL_RADIO_EVENT_TX_FAIL:
      internal_buffer.failed++;
      radio_cli_radio_extended_channel_stats[zpal_radio_get_last_tx_channel()].tx_fail++;
      break;

    case ZPAL_RADIO_EVENT_TX_FAIL_LBT:
      internal_buffer.failed_lbt++;
      radio_cli_radio_extended_channel_stats[zpal_radio_get_last_tx_channel()].tx_fail_lbt++;
      break;

    default:
      break;
  }

  status = xTaskNotifyFromISR(g_radioCliTaskHandle,
                              CLI_EVENT_RF_TX_FRAME_COMPLETE,
                              eSetBits,
                              &xHigherPriorityTaskWoken);

  ASSERT(status == pdPASS);
  portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

void
UartReceiveEvent(void)
{
  BaseType_t xHigherPriorityTaskWoken;
  BaseType_t status = pdPASS;

  status = xTaskNotifyFromISR(g_radioCliTaskHandle,
                              CLI_EVENT_UART_RX_RECEIVE,
                              eSetBits,
                              &xHigherPriorityTaskWoken);

  ASSERT(status == pdPASS);
  portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

/**
 * Handler for signaling script state transition
 */
void
cli_radio_script_state_transition_event(void)
{
  if (scriptTransitionEvent != NULL)
  {
    BaseType_t status = pdPASS;
    status = xTaskNotify(g_radioCliTaskHandle,
                         CLI_EVENT_SCRIPT_TRANSITION,
                         eSetBits);

    ASSERT(status == pdPASS);
  }
}

/*
 * Handler for asserts comming from radio,
 */
static void
RadioAssertHandler(zpal_radio_event_t assertVal)
{
  // Empty on purpose
}

/*
 * Handler region change complete
 */
static void
RegionChangeHandler(zpal_radio_event_t regionChangeStatus)
{
  // Empty on purpose
}

/*
 * Handler for Beam received,
 */
static void ECLIEventHandlerRfRxBeam(void)
{
  radio_cli_radio_extended_channel_stats[zpal_radio_get_last_beam_channel()].rx_beams++;
  if (NULL != rx_status.rx_callback)
  {
    rx_status.beam_count++;
    cli_uart_print("B");
  }
}

/*
 * Handler for timeout on frame receive after Beam received
 */
static void ECLIEventHandlerRfRxTimeout(void)
{
  if (NULL != rx_status.rx_callback)
  {
    uint8_t last_beam_data[4];
    zpal_radio_get_last_beam_data(last_beam_data, 4);
    cli_uart_printf("T %d %d(%d) %i, %02X%02X%02X%02X\n", zpal_radio_stop_current_beam_count(), zpal_radio_zpal_channel_to_internal(zpal_radio_get_last_beam_channel()), zpal_radio_get_last_beam_channel(), zpal_radio_get_flirs_beam_tx_power(), last_beam_data[0], last_beam_data[1], last_beam_data[2], last_beam_data[3]);
  }
}

/*
 * Tx complete event handler
 */
static void ECLIEventHandlerRfTxComplete(void)
{
  if (internal_buffer.frame_repeat)
  {
    internal_buffer.frame_repeat--;
  }
  if (!internal_buffer.frame_repeat || current_script_run_stop)
  {
    current_script_run_stop = false;
    xTimerStop(internal_buffer.delay_timer, 0);
    if (NULL != internal_buffer.tx_callback)
    {
      internal_buffer.tx_callback(internal_buffer.success, internal_buffer.failed, internal_buffer.failed_lbt);
    }
  }
  else
  {
    xTimerReset(internal_buffer.delay_timer, 0);
  }
}

static void ECLIEventHandlerScriptTransition(void)
{
  if (scriptTransitionEvent != NULL)
  {
    event_handler_script_callback_t tmp_scriptTransitionEvent = scriptTransitionEvent;
    scriptTransitionEvent = NULL;
    tmp_scriptTransitionEvent();
  }
}

static void ECLIEventHandlerWaitTimeout(void)
{
  xTimerStop(wait_timer, 0);
  cli_radio_script_state_transition_event();
}

uint16_t helper_strnlen(const char *zero_ended_string, size_t max_length)
{
  size_t length = 0;
  if (NULL != zero_ended_string)
  {
    for (length = 0; (length < max_length) && ('\0' != zero_ended_string[length]); length++)
    {
      // We only want to find string end, which is marked with '\0' - no other computation needed except traversing
    }
  }
  return length;
}

/*
 * Function for storing CLI data in NVM
 */
void cli_store_data(void)
{
  cli_storage.region = current_region;
  cli_storage.radio_rf_state_enable = current_radio_rf_state_enable;
  cli_storage.tx_max_power_20dbm = current_radio_tx_max_power_20dbm;
  zpal_nvm_write(cli_nvm_handle, CLI_FILE_ID, &cli_storage, sizeof(cli_storage));
}

void cli_store_autorun(void)
{
  cli_script_autorun.autorun = current_script_autorun;
  zpal_nvm_write(cli_nvm_handle, CLI_FILE_SCRIPT_AUTORUN_ID, &cli_script_autorun, sizeof(cli_script_autorun));
}

void cli_store_script(uint8_t script_number)
{
  cli_script_file_descriptor_t cli_script_descriptor;
  uint16_t script_data_length = 0;
  memset(&cli_script, 0, sizeof(cli_script));

  cli_script_descriptor.script_version = RADIO_CLI_SCRIPT_VERSION;
  cli_script_descriptor.script_active = dynamic_script_active;
  cli_script_descriptor.scriptlines = dynamic_scriptlines;
  for (int i = 0; i < dynamic_scriptlines; i++)
  {
    uint8_t line_length = helper_strnlen(dynamic_script[i].entry, sizeof(scriptline_t) - 1);
    memcpy(&cli_script.script_data[script_data_length], dynamic_script[i].entry, line_length);
    script_data_length += line_length;
    cli_script.script_data[script_data_length++] = '\0';
  }
  cli_script.script_data[script_data_length++] = '\0';
  cli_script_descriptor.script_length = script_data_length;
  zpal_nvm_write(cli_nvm_handle, CLI_FILE_SCRIPT_ID + script_number - 1, &cli_script, cli_script_descriptor.script_length);
  zpal_nvm_write(cli_nvm_handle, CLI_FILE_SCRIPT_DESCRIPTOR_ID + script_number - 1, &cli_script_descriptor, sizeof(cli_script_descriptor));
}

void cli_restore_config(void)
{
  zpal_status_t status;
  status = zpal_nvm_read(cli_nvm_handle, CLI_FILE_ID, &cli_storage, sizeof(cli_storage));
  if (status == ZPAL_STATUS_OK)
  {
    current_region = cli_storage.region;
    current_radio_rf_state_enable = cli_storage.radio_rf_state_enable;
    current_radio_tx_max_power_20dbm = cli_storage.tx_max_power_20dbm;
  }
  else
  {
    cli_store_data();
  }
  DPRINTF("Flash File read %u\n", status);
}

void cli_restore_autorun(void)
{
  zpal_status_t status;
  status = zpal_nvm_read(cli_nvm_handle, CLI_FILE_SCRIPT_AUTORUN_ID, &cli_script_autorun, sizeof(cli_script_autorun));
  if (status == ZPAL_STATUS_OK)
  {
    current_script_autorun = cli_script_autorun.autorun;
  }
  else
  {
    cli_store_autorun();
  }
}

void cli_restore_script(uint8_t script_number)
{
#ifdef USE_DEFAULT_SCRIPT_IF_NO_DYNAMIC_EXISTS
  if (0 == script_number)
  {
    active_script = &default_script[0];
    dynamic_script_active = false;
    active_scriptlines = DEFAULT_SCRIPT_COUNT;
    return;
  }
#endif
  cli_script_file_descriptor_t cli_script_descriptor;
  zpal_status_t status;

  memset(&cli_script_descriptor, 0, sizeof(cli_script_descriptor));
  status = zpal_nvm_read(cli_nvm_handle, CLI_FILE_SCRIPT_DESCRIPTOR_ID + script_number - 1, &cli_script_descriptor, sizeof(cli_script_descriptor));
  if (status == ZPAL_STATUS_OK)
  {
    memset(&cli_script, 0, sizeof(cli_script));
    status = zpal_nvm_read(cli_nvm_handle, CLI_FILE_SCRIPT_ID + script_number - 1, &cli_script, cli_script_descriptor.script_length);
  }
  memset(dynamic_script, 0, sizeof(dynamic_script));
  dynamic_scriptlines = 0;
  dynamic_script_active = 0;
  if (status == ZPAL_STATUS_OK)
  {
    uint16_t data_index = 0;
    dynamic_scriptlines = cli_script_descriptor.scriptlines;
    dynamic_script_active = cli_script_descriptor.script_active;
    for (int i = 0; i < dynamic_scriptlines; i++)
    {
      size_t line_length = helper_strnlen(&cli_script.script_data[data_index], sizeof(scriptline_t) - 1);
      if (line_length)
      {
        memcpy(dynamic_script[i].entry, &cli_script.script_data[data_index], line_length);
        dynamic_script[i].entry[line_length++] = '\0';
        data_index += line_length;
      }
    }
    if (dynamic_script_active)
    {
      active_script = &dynamic_script[0];
    }
    active_scriptlines = dynamic_scriptlines;
  }
  if (status != ZPAL_STATUS_OK)
  {
    active_scriptlines = 0;
    cli_store_script(script_number);
  }
}

/*
 * Receive handler
 */
void cli_frame_receive_handler(zpal_radio_rx_parameters_t * pRxParameters, zpal_radio_receive_frame_t * pZpalFrame)
{
  radio_cli_radio_extended_channel_stats[pRxParameters->channel_id].rx_frames++;
  if (memcmp(pZpalFrame->frame_content, internal_network.network_homeid, 4))
  {
    radio_cli_radio_extended_channel_stats[pRxParameters->channel_id].rx_foreign_homeid++;
  }
  rx_status.receive_count++;
  if (NULL != rx_status.rx_callback)
  {
    rx_status.rx_callback(pRxParameters, pZpalFrame);
  }
}

/*
 * Helper function for getting channel configuration
 */
const zpal_radio_transmit_parameter_t* helper_get_radio_parameters(zpal_radio_region_t region,
                                                            zpal_radio_zwave_channel_t channel)
{
  switch (region)
  {
    case REGION_EU:
    case REGION_US:
    case REGION_ANZ:
    case REGION_HK:
    case REGION_IN:
    case REGION_IL:
    case REGION_RU:
    case REGION_CN:
    case REGION_US_LR:
    case REGION_EU_LR:
    case INTERNAL_REGION_US_LR_920:
    case INTERNAL_REGION_EU_LR_866_4:
      return &regions24ch[channel];

    case REGION_JP:
    case REGION_KR:
      return &regions3ch[channel];

    case INTERNAL_REGION_EU_LR_END_DEVICE:
    case INTERNAL_REGION_US_LR_END_DEVICE:
      if ((3 == channel) || (4 == channel))
      {
        channel -= 3;
      }
      if ((0 != channel) && (1 != channel))
      {
        channel = 0;
      }
      return &region2lrch[channel];

    default:
      return NULL;
  }
}

bool cli_radio_initialized()
{
  if (false == radio_initialized)
  {
    cli_uart_print("** Radio not initialized\n");
  }
  return radio_initialized;
}

void cli_radio_fw_version_get(void)
{
  if (true == radio_initialized)
  {
    cli_uart_printf("Radio firmware version=%u (0x%X)\n", zpal_radio_version_get(), zpal_radio_version_get());
  }
}

static uint16_t radio_cli_rssi_sample_frequency = 0;
static uint8_t radio_cli_rssi_sample_count_average = 0;

/*
 * Initialize the radio for the given region
 */
void cli_radio_setup(zpal_radio_region_t eRegion)
{
  current_region = eRegion;

  // Clear internal Tx structure
  memset(&internal_buffer, 0, sizeof(internal_frame_buffer_t));

  // Configure radio
  zpal_radio_wakeup_t eWakeup = ZPAL_RADIO_WAKEUP_ALWAYS_LISTEN;

  zpal_radio_profile_t RfProfile = {.region = internal_region_to_zpal_region(eRegion),
                                    .wakeup = eWakeup,
                                    .listen_before_talk_threshold = -65,
                                    .tx_power_max = 0,
                                    .tx_power_adjust = 0,
                                    .tx_power_max_lr = current_radio_tx_max_power_20dbm ? ZW_TX_POWER_20DBM : ZW_TX_POWER_14DBM,
                                    .home_id = internal_network.network_homeid,
                                    .rx_cb = RXHandlerFromISR,
                                    .tx_cb = TXHandlerFromISR,
                                    .region_change_cb = RegionChangeHandler,
                                    .assert_cb = RadioAssertHandler,
                                    .receive_handler_cb = cli_frame_receive_handler,
                                    .network_stats = &sNetworkStatistic,
                                    .radio_debug_enable = false,
                                    .lr_channel_auto_mode = false,
                                    .primary_lr_channel = ZPAL_RADIO_LR_CHANNEL_UNINITIALIZED,
                                    .active_lr_channel_config = internal_region_to_channel_config(eRegion)
                                   };

  zpal_radio_rssi_config_set(radio_cli_rssi_sample_frequency, radio_cli_rssi_sample_count_average);

  zpal_radio_init(&RfProfile);
  radio_initialized = true;
  cli_radio_fw_version_get();
}

// '__attribute__((weak))' function in zpal_radio.c, which always returns true.
// Overriding here to be able to enable/disable classic tx power feature
bool
zpal_radio_classic_tx_power_is_enabled(void)
{
  return m_zw_classic_tx_power_enable;
}

bool cli_radio_zw_classic_tx_power_enable(bool enable)
{
  m_zw_classic_tx_power_enable = enable;
  return m_zw_classic_tx_power_enable;
}

bool cli_radio_zw_classic_tx_power_is_enabled(void)
{
  return zpal_radio_classic_tx_power_is_enabled();
}

int16_t cli_radio_classic_tx_power_set(int16_t tx_power)
{
  return zpal_radio_set_classic_tx_power(tx_power);
}

int16_t cli_radio_classic_tx_power_get(void)
{
  return zpal_radio_get_classic_tx_power();
}

int16_t cli_radio_classic_tx_power_adjust_set(int16_t tx_power_adjust)
{
  return zpal_radio_set_classic_tx_power_adjust(tx_power_adjust);
}

int16_t cli_radio_classic_tx_power_adjust_get(void)
{
  return zpal_radio_get_classic_tx_power_adjust();
}

uint8_t cli_radio_classic_tx_power_attenuation_set(uint8_t tx_power_attenuation)
{
  return zpal_radio_set_classic_tx_power_attenuation(tx_power_attenuation);
}

uint8_t cli_radio_classic_tx_power_attenuation_get(void)
{
  return zpal_radio_get_classic_tx_power_attenuation();
}

/*
 * Enable or disable fixed Rx channel
*/
void cli_radio_set_fixed_channel(uint8_t enable_fixed_channel, uint8_t channel_id)
{
  rx_status.receiver_running_single_channel_scan = enable_fixed_channel;
  rx_status.channel = channel_id;
  zpal_radio_set_rx_fixed_channel(enable_fixed_channel, channel_id);
}

/*
 * Get channel count for current Region
 */
uint8_t cli_radio_region_channel_count_get(void)
{
  return zpal_radio_region_channel_count_get();
}

char * helper_get_region_string(zpal_radio_region_t region)
{
  for (uint32_t i = 0; i < sizeof_array(region_table); i++)
  {
    if (region_table[i].region == region)
    {
      return region_table[i].region_string;
    }
    if (region_table[i].region == REGION_UNDEFINED)
    {
      return "UNDEFINED";
    }
  }
  return "";
}

void helper_list_regions(void)
{
  for (uint32_t i = 0; i < sizeof_array(region_table); i++)
  {
    cli_uart_printf(" %3u - %s\n", region_table[i].region, region_table[i].region_string);
  }
  cli_uart_printf("\n");
}

zpal_radio_region_t cli_radio_region_current_get(void)
{
  return current_region;
}

char *cli_radio_region_current_description_get(void)
{
  return helper_get_region_string(current_region);
}

/**
 * Print out current region and list of supported regions
 */
void cli_radio_region_list(zpal_radio_region_t region)
{
  cli_uart_printf("\nCurrent Region:\n %3u - %s\n\n", region, helper_get_region_string(region));
  helper_list_regions();
}

/*
 * Change/Set the region
 */
bool cli_radio_change_region(zpal_radio_region_t new_internal_region)
{
  bool ret_status = false;
  zpal_radio_region_t new_region = internal_region_to_zpal_region(new_internal_region);
  zpal_radio_lr_channel_config_t lrchconfig = internal_region_to_channel_config(new_internal_region);

  if (ZPAL_RADIO_LR_CH_CFG_COUNT != lrchconfig)
  {
    current_region = new_internal_region;
    cli_store_data();

    if (true == radio_initialized)
    {
      ret_status = zpal_radio_change_region(new_region, lrchconfig) == ZPAL_STATUS_OK ? true : false;
    }
    else
    {
      // If Radio not initialized we just accept the valid region change
      ret_status = true;
    }
  }
  else
  {
    cli_uart_printf("\nInvalid requested Region: %i\n", new_internal_region);
  }

  return ret_status;
}

uint8_t homeid_hash(uint32_t homeid, node_id_t nodeid)
{
  zpal_radio_protocol_mode_t protocolmode = zpal_radio_get_protocol_mode();
  uint32_t homeidhash = homeid ^ 0xFF;

  homeidhash ^= (homeidhash >> 8);
  homeidhash ^= (homeidhash >> 16);
  homeidhash &= 0xFF;

  if (ZPAL_RADIO_PROTOCOL_MODE_UNDEFINED != protocolmode)
  {
    // Check and correct for valid homeid hash values
    if (ZPAL_RADIO_PROTOCOL_MODE_2 == protocolmode)
    {
      switch (homeidhash)
      {
        case 0x05:
        case 0x0a:
        case 0x25:
        case 0x45:
        case 0x4a:
        case 0x55:
        case 0x65:
          {
            homeidhash++;
          }
          break;

        default:
          break;
      }
    }
    else if ((ZPAL_RADIO_PROTOCOL_MODE_1 == protocolmode) || ((ZPAL_RADIO_PROTOCOL_MODE_3 == protocolmode) && (0x0100 > nodeid)))
    {
      switch (homeidhash)
      {
        case 0x0a:
        case 0x4a:
        case 0x55:
          {
            homeidhash++;
          }
          break;

        default:
          break;
      }
    }
    // For Long Range all values are valid for homeid hash
  }
  if ((0 == homeid) && (0 == nodeid))
  {
    homeidhash  = 0x55;
  }
  return (uint8_t)homeidhash;
}

/*
 * Update the homeID and nodeID for the Rx filter
 */
void cli_radio_update_ids(void)
{
  if (zpal_radio_network_id_filter_enabled_get() && ((0 != internal_network.homeID_uint32) || (0 != internal_network.node_id))) {
    zpal_radio_network_filter_enable(false);
  }
  zpal_radio_set_network_ids(internal_network.homeID_uint32, internal_network.node_id,
                             ZPAL_RADIO_MODE_ALWAYS_LISTENING, homeid_hash(internal_network.homeID_uint32, internal_network.node_id));
  zpal_radio_network_filter_enable(true);
}

/*
 * Change/Set the homeID for the Rx filter
 */
void cli_radio_set_homeid(uint32_t home_id)
{
  internal_network.homeID_uint32 = home_id;
  home_id_convert(internal_network.network_homeid, home_id);

  cli_radio_update_ids();
}

/*
 * Change/Set the NodeID for the Rx filter
 */
void cli_radio_set_nodeid(node_id_t node_id)
{
  internal_network.node_id = node_id;
  cli_radio_update_ids();
}

void cli_radio_set_payload_default(radio_cli_tx_frame_config_t *frame)
{
  if ((INTERNAL_REGION_EU_LR_END_DEVICE == current_region) || (INTERNAL_REGION_US_LR_END_DEVICE == current_region) || (3 == (*frame).channel))
  {
    (*frame).payload_length = payload_default[3][7] - 2;
    memcpy((*frame).payload_buffer, &payload_default[3][0], (*frame).payload_length);
  }
  else if ((REGION_JP == current_region) || (REGION_KR == current_region))
  {
    (*frame).payload_length = payload_default[4][7] - 2;
    memcpy((*frame).payload_buffer, &payload_default[4][0], (*frame).payload_length);
  }
  else
  {
    if (2 < (*frame).channel)
    {
      (*frame).channel = 0;
    }
    (*frame).payload_length = payload_default[(*frame).channel][7] - ((0 == (*frame).channel) ? 2 : 1);
    memcpy((*frame).payload_buffer, &payload_default[(*frame).channel][0], (*frame).payload_length);
  }
}

uint32_t radio_time_stamp_get(void)
{
  return xTaskGetTickCount();
}

// Just split buffer at byte 9, no need to calculate actual header size
#define FIXED_HEADER_LENGTH 9

/*
 * Send frame from internal buffer
 */
bool helper_radio_transmit(internal_frame_buffer_t frame)
{
  zpal_status_t status;
  zpal_radio_transmit_parameter_t const *tx_params = helper_get_radio_parameters(current_region, frame.channel);
  int8_t classic_tx_power = ((cli_radio_classic_tx_power_get() - cli_radio_classic_tx_power_adjust_get()) / 10) - cli_radio_classic_tx_power_attenuation_get();
  if (!zpal_radio_classic_tx_power_is_enabled())
  {
    classic_tx_power = 0;
  }
  if (timestamp_enabled)
  {
    cli_uart_printf("%9u - ", radio_time_stamp_get());
  }
  cli_uart_printf("TX int_region=%u(%u,%u),ch=%u(%u),pwr=%d(%d),cnt=%u,lbt=%u,delay=%ums\n",
                  current_region, zpal_radio_get_region(), internal_region_to_channel_config(current_region),
                  frame.channel, (*tx_params).channel_id, frame.power, classic_tx_power, frame.frame_repeat, frame.lbt ? 1 : 0, frame.frame_delay_in_between);
  status = zpal_radio_transmit(tx_params, FIXED_HEADER_LENGTH, frame.frame_buffer,
                               frame.frame_length - FIXED_HEADER_LENGTH, frame.frame_buffer + FIXED_HEADER_LENGTH,
                               frame.lbt, frame.power);
  return status == ZPAL_STATUS_OK ? true : false;
}

static void cli_uart_do_transmit(void)
{
  helper_radio_transmit(internal_buffer);
}

static void delay_timer_cb(__attribute__((unused)) TimerHandle_t xTimer )
{
  BaseType_t status = pdPASS;

  status = xTaskNotify(g_radioCliTaskHandle,
                       CLI_EVENT_APP_RF_TRANSMIT,
                       eSetBits);

  ASSERT(status == pdPASS);
}

static void wait_timer_cb(__attribute__((unused)) TimerHandle_t xTimer )
{
  BaseType_t status = pdPASS;

  status = xTaskNotify(g_radioCliTaskHandle,
                       CLI_EVENT_WAIT_TIMEOUT,
                       eSetBits);

  ASSERT(status == pdPASS);
}

void cli_radio_wait(uint32_t time_ms)
{
  if (current_script_running)
  {
    wait_timer = xTimerCreateStatic("Wait",
                                    pdMS_TO_TICKS(time_ms),
                                    pdFALSE,
                                    NULL,
                                    wait_timer_cb,
                                    &wait_timer_buffer);
    xTimerReset(wait_timer, 0);
  }
  else
  {
    cli_uart_print("Wait only has a use when used in a script - skipping execution\n");
  }
}

/*
 * Copy data to internal transmit buffer and start/stop transmit
 */
bool cli_radio_transmit_frame(const radio_cli_tx_frame_config_t *const tx_frame_config)
{
  bool ret_status = true;

  if (!cli_radio_initialized())
  {
    return false;
  }
  if (0 == tx_frame_config->frame_repeat)
  {
    // Do we have an active repeated transmit to stop
    if (pdFALSE != xTimerIsTimerActive(internal_buffer.delay_timer))
    {
      // Stop transmit
      internal_buffer.frame_repeat = 1;
    }
  }
  else if (internal_buffer.frame_repeat)
  {
    if (tx_frame_config->frame_repeat < internal_buffer.frame_repeat)
    {
      internal_buffer.frame_repeat = tx_frame_config->frame_repeat;
    }
  }
  else
  {
    // Save frame information if frame is repeated
    internal_buffer.frame_length = tx_frame_config->payload_length + (transmit_option_force_checksum_errors ? 1 : 0);
    internal_buffer.frame_buffer = tx_frame_config->payload_buffer;
    internal_buffer.frame_repeat = tx_frame_config->frame_repeat;
    internal_buffer.wait_ack = tx_frame_config->wait_ack;
    internal_buffer.channel = tx_frame_config->channel;
    internal_buffer.lbt = tx_frame_config->lbt;
    internal_buffer.power = tx_frame_config->power;
    internal_buffer.frame_delay_in_between = tx_frame_config->delay;
    internal_buffer.tx_callback = tx_frame_config->tx_callback;
    internal_buffer.success = 0;
    internal_buffer.failed = 0;
    internal_buffer.failed_lbt = 0;

    if (1 < tx_frame_config->frame_repeat)
    {
      internal_buffer.delay_timer = xTimerCreateStatic("Tx delay",
                                                       pdMS_TO_TICKS(internal_buffer.frame_delay_in_between),
                                                       pdFALSE,
                                                       NULL,
                                                       delay_timer_cb,
                                                       &internal_buffer.delay_timer_buffer);
    }
    ret_status = helper_radio_transmit(internal_buffer);
  }
  return ret_status;
}

static const char speed_description[][10] = {
                                             "UNDEFINED",  ///< Undefined baud rate.
                                             "9.6Kb   ",  ///< 9.6 kbps
                                             " 40Kb   ",  ///< 40 kbps
                                             "100Kb   ",  ///< 100 kbps
                                             "100Kb LR",  ///< 100 kbps for long range
                                            };

#define MIN_BYTES_FOR_FRAME_PRINT (2 + 1)

uint16_t generate_string_from_array(char *p_str, uint16_t max_string_length, const char *p_array, uint16_t array_length)
{
  uint16_t retval = 0;
  if ((array_length == 0) || (p_array == NULL))
  {
    p_str[0] = '\0';
  }
  else
  {
    if ((array_length << 1) > max_string_length)
    {
      array_length = (max_string_length >> 1) - 2;
    }
    for (int j = 0; j < array_length; j++)
    {
      snprintf(p_str + (j << 1), MIN_BYTES_FOR_FRAME_PRINT, "%02X", p_array[j]);
    }
    retval = helper_strnlen(p_str, max_string_length);
  }
  return retval;
}

/*
 * Callback function for Z-Wave receive command
 */
static void radio_rx_frame(zpal_radio_rx_parameters_t *rx_parameters, zpal_radio_receive_frame_t *frame)
{
  char str_buf[300];
  char *p_str = str_buf;
  if (timestamp_enabled)
  {
    cli_uart_printf("%9u - ", radio_time_stamp_get());
  }
  snprintf(p_str, sizeof(str_buf) - MIN_BYTES_FOR_FRAME_PRINT, "RX int_region=%3d(%2d,%d),ch=%u(%u),rssi=%4i,(%9s) ",
                  current_region,
                  internal_region_to_zpal_region(current_region),
                  internal_region_to_channel_config(current_region),
                  zpal_radio_zpal_channel_to_internal(rx_parameters->channel_id), rx_parameters->channel_id,
                  rx_parameters->rssi,
                  speed_description[(5 > rx_parameters->speed) ? rx_parameters->speed : 0]);
  uint16_t current_strbuf_length = helper_strnlen(str_buf, sizeof(str_buf) - MIN_BYTES_FOR_FRAME_PRINT);
  uint16_t frame_printable_length = (sizeof(str_buf) - current_strbuf_length > frame->frame_content_length << 1) ? frame->frame_content_length : (sizeof(str_buf) - current_strbuf_length) >> 1;
  p_str += current_strbuf_length;
  generate_string_from_array(p_str, sizeof(str_buf) - current_strbuf_length, frame->frame_content, frame_printable_length);
  cli_uart_printf("%s\n", str_buf);
}

/*
 * Start or stop the receiver
 */
bool
cli_radio_start_receive(bool start_receive)
{
  if (!cli_radio_initialized())
  {
    return false;
  }

  rx_status.receiver_running = start_receive;
  if (start_receive)
  {
    // Start receiving
    rx_status.receive_count = 0;
    rx_status.rx_callback = radio_rx_frame;
    zpal_radio_start_receive();
  }
  else
  {
    // Set Radio to Idle
    zpal_radio_power_down();
  }
}

/*
 * Get the number of received frames
*/
uint16_t
cli_radio_get_rx_count(void)
{
  return rx_status.receive_count;
}

/*
 * Set LBT level for a given channel
*/
bool
cli_radio_set_lbt_level(uint8_t channel_id, int8_t level)
{
  if (!cli_radio_initialized())
  {
    return false;
  }
  zpal_radio_set_lbt_level(channel_id, level);
  return true;
}

void cli_radio_set_tx_max_power_20dbm(bool max_tx_power_20dbm)
{
  current_radio_tx_max_power_20dbm = max_tx_power_20dbm;
  cli_store_data();
  zpal_radio_tx_power_config(current_radio_tx_max_power_20dbm ? ZW_TX_POWER_20DBM : ZW_TX_POWER_14DBM);
  cli_uart_printf("\n** Reset module to set new Tx max power setting %udBm into effect **\n", current_radio_tx_max_power_20dbm ? 20 : 14);
}

bool cli_radio_get_tx_max_power_20dbm(void)
{
  return current_radio_tx_max_power_20dbm;
}

int8_t cli_radio_get_tx_min_power(uint8_t channel_id)
{
  return zpal_radio_min_tx_power_get(channel_id);
}

int8_t cli_radio_get_tx_max_power(uint8_t channel_id)
{
  return zpal_radio_max_tx_power_get(channel_id);
}

/*
 * Get radio statistics
*/
zpal_radio_network_stats_t* cli_radio_get_stats(void)
{
  if (cli_radio_initialized())
  {
    return &sNetworkStatistic;
  }
  return NULL;
}

void cli_radio_clear_stats(bool clear_tx_timers_stat)
{
  if (clear_tx_timers_stat)
  {
    // Clear Tx timers only
    zpal_radio_clear_tx_timers();
  }
  else
  {
    // Clear all network statistics
    zpal_radio_clear_network_stats();
  }
}

bool cli_radio_tx_option_set(uint8_t option, uint8_t enable)
{
  bool status = false;
  if (1 == option)
  {
    // Tx option 'fail-crc'
    transmit_option_force_checksum_errors = enable;
    cli_uart_printf("Tx option fail-crc %s\n", enable ? "enabled" : "disabled");
    status = true;
  }
  return status;
}

void cli_radio_tx_continues_set(bool enable, radio_cli_tx_frame_config_t *frame)
{
  zpal_radio_transmit_parameter_t const *tx_params = helper_get_radio_parameters(current_region, frame->channel);
  zpal_status_t status = zpal_radio_tx_continues_wave_set(enable, tx_params->channel_id, frame->power, frame->wave_type);

  if (enable)
  {
    cli_uart_printf("Continuous %s signal transmit mode %s\n",
                    (0 == frame->wave_type) ? "unmodulated" : ((1 == frame->wave_type) ? "modulated" : "modulated with whitening"),
                    (ZPAL_STATUS_OK == status) ? "enabled" : "set failed");
  }
  else
  {
    cli_uart_printf("Continuous %s signal transmit mode disable%s\n",
                    (0 == frame->wave_type) ? "unmodulated" : ((1 == frame->wave_type) ? "modulated" : "modulated with whitening"),
                    (ZPAL_STATUS_OK == status) ? "d" : " failed");
  }
}

void cli_radio_rf_debug_set(bool rf_state_enable)
{
  current_radio_rf_state_enable = rf_state_enable;
  cli_store_data();

  cli_uart_print("\n** Reset module to put the new rf state gpio settings into effect **\n");
}

void radio_rf_debug_calibration_list(void)
{
    uint32_t reg_val;
    uint16_t reg_addr;

    reg_addr = 0x308;
    reg_val = RfMcu_RegGet(reg_addr);
    reg_val &= 0x0000FFFF;

    cli_uart_printf("reg rxiq_gain:      %"PRIu32"\n", (reg_val >> 0)  & 0x7f);
    cli_uart_printf("reg rxiq_gain_sel:  %"PRIu32"\n", (reg_val >> 7)  & 0x01);
    cli_uart_printf("reg rxiq_phase:     %"PRIu32"\n", (reg_val >> 8)  & 0x7f);
    cli_uart_printf("reg rxiq_phase_sel: %"PRIu32"\n", (reg_val >> 15) & 0x01);

    reg_addr = 0x380;
    reg_val = RfMcu_RegGet(reg_addr);
    reg_val &= 0x0000003F;

    cli_uart_printf("reg if_filt_asic:   %"PRIu32"\n", reg_val);

    reg_addr = 0x484;
    reg_val = RfMcu_RegGet(reg_addr);
    reg_val &= 0x0000003F;

    cli_uart_printf("reg xosc_cap_ini:   %"PRIu32"\n", reg_val);

    cli_uart_printf("hreg xosc_cap_ini:  %"PRIu16"\n", PMU_CTRL->PMU_SOC_PMU_XTAL1.bit.XOSC_CAP_INI);

    reg_addr = 0x403C;
    reg_val = RfMcu_RegGet(reg_addr);
    cli_uart_printf("modem select:       %"PRIu32"\n", (reg_val >> 24) & 0xff);
}

void radio_rf_debug_reg_list(void)
{
    uint32_t reg_val;
    uint16_t reg_addr;

    /* MAC */
    cli_uart_print("MAC:\n");
    for (reg_addr = 0x100 ; reg_addr < 0x200 ; reg_addr += 4)
    {
        reg_val = RfMcu_RegGet(reg_addr);
        cli_uart_printf("Addr: 0x%04"PRIx16", Val: 0x%08"PRIx32"\n", reg_addr, reg_val);
    }

    /* BMU */
    cli_uart_print("BMU:\n");
    for (reg_addr = 0x200 ; reg_addr < 0x300 ; reg_addr += 4)
    {
        reg_val = RfMcu_RegGet(reg_addr);
        cli_uart_printf("Addr: 0x%04"PRIx16", Val: 0x%08"PRIx32"\n", reg_addr, reg_val);
    }

    /* RF */
    cli_uart_print("RF:\n");
    for (reg_addr = 0x300 ; reg_addr < 0x400 ; reg_addr += 4)
    {
        reg_val = RfMcu_RegGet(reg_addr);
        cli_uart_printf("Addr: 0x%04"PRIx16", Val: 0x%08"PRIx32"\n", reg_addr, reg_val);
    }

    /* PMU */
    cli_uart_print("PMU:\n");
    for (reg_addr = 0x400 ; reg_addr < 0x500 ; reg_addr += 4)
    {
        reg_val = RfMcu_RegGet(reg_addr);
        cli_uart_printf("Addr: 0x%04"PRIx16", Val: 0x%08"PRIx32"\n", reg_addr, reg_val);
    }

    /* FW */
    cli_uart_print("FW:\n");
    for (reg_addr = 0x4000 ; reg_addr < 0x4040 ; reg_addr += 4)
    {
        reg_val = RfMcu_RegGet(reg_addr);
        cli_uart_printf("Addr: 0x%04"PRIx16", Val: 0x%08"PRIx32"\n", reg_addr, reg_val);
    }
}

static void radio_app_host_debug_reg_list(void)
{
  uint32_t *reg_addr;

  /* SYS_CTRL */
  for (reg_addr = (uint32_t *)0x50000000 ; reg_addr < (uint32_t *)0x500000C0 ; reg_addr += 1)
  {
    cli_uart_printf("Addr: 0x%08"PRIx32", Val: 0x%08"PRIx32" \r\n", reg_addr, *reg_addr);
  }

  /* SEC_CTRL */
  for (reg_addr = (uint32_t *)0x50003000 ; reg_addr < (uint32_t *)0x5000304C ; reg_addr += 1)
  {
    cli_uart_printf("Addr: 0x%08"PRIx32", Val: 0x%08"PRIx32" \r\n", reg_addr, *reg_addr);
  }

  /* SOC_PMU */
  for (reg_addr = (uint32_t *)0x50006000 ; reg_addr < (uint32_t *)0x500060F4 ; reg_addr += 1)
  {
    cli_uart_printf("Addr: 0x%08"PRIx32", Val: 0x%08"PRIx32" \r\n", reg_addr, *reg_addr);
  }

  /* RT569_AHB */
  for (reg_addr = (uint32_t *)0x5001A000 ; reg_addr < (uint32_t *)0x5001A038 ; reg_addr += 1)
  {
    cli_uart_printf("Addr: 0x%08"PRIx32", Val: 0x%08"PRIx32" \r\n", reg_addr, *reg_addr);
  }
}

void cli_radio_rf_debug_reg_setting_list(bool listallreg)
{
  radio_rf_debug_calibration_list();
  if (true == listallreg)
  {
    radio_rf_debug_reg_list();
    radio_app_host_debug_reg_list();
  }
}

enum rssi_get_mode_t
{
  RSSI_MODE_CHANNEL,
  RSSI_MODE_ALL,
};

static enum rssi_get_mode_t rssi_get_mode = RSSI_MODE_CHANNEL;
static uint8_t rssi_channel = 0;
static uint32_t rssi_get_repeats = 1;
static uint32_t rssi_get_delay = 1000;
TimerHandle_t rssi_get_timer;
StaticTimer_t rssi_get_timer_buffer;
static int8_t rssi_get_all_latest_sample[ZPAL_RADIO_NUM_CHANNELS_LR_CH_CFG_1_2];

void radio_cli_rssi_read_all_channels(void)
{
  uint8_t number_of_channels = cli_radio_region_channel_count_get();
  int8_t rssi_get_all_current_sample[ZPAL_RADIO_NUM_CHANNELS_LR_CH_CFG_1_2] = {0};
  bool status = zpal_radio_rssi_read_all_channels(rssi_get_all_current_sample, number_of_channels);
  for (uint8_t i = 0; i < number_of_channels; i++)
  {
    bool is_rssi_valid = false;
    if (!status)
    {
      rssi_get_all_latest_sample[i] = zpal_radio_rssi_read(i, &is_rssi_valid);
    }
    if (status || is_rssi_valid)
    {
      rssi_get_all_latest_sample[i] = rssi_get_all_current_sample[i];
    }
  }
  cli_uart_printf("rssi=%4d,%4d,%4d,%4d\n", rssi_get_all_latest_sample[0], rssi_get_all_latest_sample[1],
                                            rssi_get_all_latest_sample[2], rssi_get_all_latest_sample[3]);
}

void cli_radio_rssi_read(void)
{
  if (timestamp_enabled)
  {
    cli_uart_printf("%9u - ", radio_time_stamp_get());
  }
  if (rssi_get_mode == RSSI_MODE_CHANNEL)
  {
    bool is_rssi_valid = false;
    int8_t rssi_sample = zpal_radio_rssi_read(rssi_channel, &is_rssi_valid);
    cli_uart_printf("channel %u rssi=%4i - %s\n", rssi_channel, rssi_sample, is_rssi_valid ? "valid" : "not valid");
  }
  else if (rssi_get_mode == RSSI_MODE_ALL)
  {
    radio_cli_rssi_read_all_channels();
  }
}

static void rssi_get_timer_cb(TimerHandle_t xTimer)
{
  cli_radio_rssi_read();
  if (rssi_get_repeats > 0)
  {
    rssi_get_repeats--;
    xTimerReset(xTimer, 0);
  }
  else
  {
    xTimerStop(xTimer, 0);
  }
}

void cli_radio_rssi_get(uint8_t channel_id, uint32_t repeats, uint32_t delay)
{
  xTimerStop(rssi_get_timer, 0);
  rssi_get_mode = RSSI_MODE_CHANNEL;
  rssi_channel = channel_id;
  rssi_get_repeats = repeats;
  rssi_get_delay = delay;
  if (rssi_get_repeats < 2)
  {
    rssi_get_repeats = 0;
    cli_radio_rssi_read();
  }
  else
  {
    rssi_get_timer = xTimerCreateStatic("RSSIc",
                                  pdMS_TO_TICKS(rssi_get_delay),
                                  pdFALSE,
                                  NULL,
                                  rssi_get_timer_cb,
                                  &rssi_get_timer_buffer);
    xTimerStart(rssi_get_timer, 0);
  }
}

void cli_radio_rssi_get_all(uint32_t repeats, uint32_t delay)
{
  xTimerStop(rssi_get_timer, 0);
  rssi_get_mode = RSSI_MODE_ALL;
  rssi_get_repeats = repeats;
  rssi_get_delay = delay;
  bool receiver_running = rx_status.receiver_running;
  if (radio_cli_rssi_sample_frequency == 0)
  {
    cli_uart_print("** RSSI sample frequency not set, using default values 'zw-radio-rssi-config-set 498 2' **\n");
    if (receiver_running)
    {
      cli_radio_start_receive(false);
    }
    cli_radio_rssi_config_set(498, 2);
  }
  if (0 == receiver_running)
  {
    cli_uart_print("** Receiver not running, starting receiver **\n");
  }
  if (0 == rx_status.receiver_running)
  {
    cli_radio_start_receive(true);
  }
  if (rssi_get_repeats < 2)
  {
    rssi_get_repeats = 0;
    cli_radio_rssi_read();
  }
  else
  {
    rssi_get_timer = xTimerCreateStatic("RSSIa",
                                  pdMS_TO_TICKS(rssi_get_delay),
                                  pdFALSE,
                                  NULL,
                                  rssi_get_timer_cb,
                                  &rssi_get_timer_buffer);
    xTimerStart(rssi_get_timer, 0);
  }
}

void cli_radio_rssi_config_get(uint16_t *rssi_sample_frequency, uint8_t *rssi_sample_count_average)
{
  zpal_radio_rssi_config_get(&radio_cli_rssi_sample_frequency, &radio_cli_rssi_sample_count_average);
  *rssi_sample_frequency = radio_cli_rssi_sample_frequency;
  *rssi_sample_count_average = radio_cli_rssi_sample_count_average;
}

void cli_radio_rssi_config_set(uint16_t rssi_sample_frequency, uint8_t rssi_sample_count_average)
{
  radio_cli_rssi_sample_frequency = rssi_sample_frequency;
  radio_cli_rssi_sample_count_average = rssi_sample_count_average;
  zpal_radio_rssi_config_set(radio_cli_rssi_sample_frequency, radio_cli_rssi_sample_count_average);
}

void radio_cli_rf_debug(void)
{
  if (current_radio_rf_state_enable)
  {
    zpal_radio_power_down();
    cli_radio_setup(current_region);
    zpal_status_t status = zpal_radio_rf_debug_set(current_radio_rf_state_enable);

    if (current_radio_rf_state_enable)
    {
      cli_uart_printf("\n%s\n", (ZPAL_STATUS_OK == status) ? "RF Rx and Tx state on GPIO 21 and 28 enabled - 1 = active" : "RF Rx and TX state on GPIO enable failed");
      cli_uart_printf("%s\n\n", (ZPAL_STATUS_OK == status) ? "RF MCU state on GPIO 0 enabled - 1 = active, 0 = sleep" : "RF MCU state on GPIO enable failed");
    }
    else
    {
      cli_uart_printf("\n%s\n", (ZPAL_STATUS_OK == status) ? "RF Rx and TX state on GPIO disabled" : "RF Rx and TX state on GPIO disable failed");
      cli_uart_printf("%s\n\n", (ZPAL_STATUS_OK == status) ? "RF MCU state on GPIO disabled" : "RF MCU state on GPIO disable failed");
    }
  }
}

void radio_cli_print_rf_statistics_on_channel(uint8_t channel_id)
{
  zpal_radio_transmit_parameter_t const *tx_params = helper_get_radio_parameters(current_region, channel_id);
  cli_uart_printf("RF Statistics for channel %u(%u)\n", channel_id, (*tx_params).channel_id);
  cli_uart_printf("Rx Frames   = %lu\n", radio_cli_radio_extended_channel_stats[(*tx_params).channel_id].rx_frames);
  cli_uart_printf("Rx Beams    = %lu\n", radio_cli_radio_extended_channel_stats[(*tx_params).channel_id].rx_beams);
  cli_uart_printf("Tx Frames   = %lu\n", radio_cli_radio_extended_channel_stats[(*tx_params).channel_id].tx_frames);
  cli_uart_printf("Tx Fail     = %lu\n", radio_cli_radio_extended_channel_stats[(*tx_params).channel_id].tx_fail);
  cli_uart_printf("Tx LBT fail = %lu\n", radio_cli_radio_extended_channel_stats[(*tx_params).channel_id].tx_fail_lbt);
}

void cli_radio_print_statistics(bool print_extended)
{
  cli_uart_printf("Beam Count              = %d\n", zpal_radio_get_beam_count());
  cli_uart_printf("Current Beam Count      = %d\n", zpal_radio_get_current_beam_count());
  cli_uart_printf("Last Stop Beam Count    = %d\n", zpal_radio_get_last_stop_beam_count());
  for (uint32_t i = 0; i < zpal_radio_region_channel_count_get(); i++)
  {
    int8_t channel_lbt = zpal_radio_get_lbt_level(i);
    cli_uart_printf("LBT Threshold channel %01d = %4i - %s\n", i, channel_lbt, (0 == channel_lbt) ? "Off" : "On");
  }
  if (print_extended)
  {
    for (int i = 0; i < zpal_radio_region_channel_count_get(); i++)
    {
      radio_cli_print_rf_statistics_on_channel(i);
    }
  }
}

zpal_reset_reason_t cli_radio_reset_reason_get(void)
{
  return last_reset_reason;
}

void cli_radio_timestamp_print()
{
  cli_uart_printf("timestamp turned %s on Rx and Tx printout\n", timestamp_enabled ? "ON" : "OFF");
}

void cli_radio_timestamp_set(bool enable)
{
  timestamp_enabled = enable;
  cli_radio_timestamp_print();
}

bool cli_radio_timestamp_get(void)
{
  return timestamp_enabled;
}

void cli_radio_reset_reason_set(zpal_reset_reason_t reset_reason)
{
  last_reset_reason = reset_reason;
}

__attribute__((noreturn)) void cli_radio_reset(void)
{
  zpal_clear_restarts();
  zpal_reboot_with_info(0x462, ZPAL_RESET_INFO_DEFAULT);
  for (;;);
}

void cli_radio_version_print(void)
{
  uint16_t chip_id;
  uint16_t chip_rev;
  char *chip_id_string;

  zpal_radio_chip_get_version(&chip_id, &chip_rev);
  if (chip_id == 0x584)
  {
    chip_id_string = "T32CZ20B";
  }
  else
  {
    chip_id_string = "T32CM11C";
  }

  cli_uart_printf("Trident Radio Test CLI version %u.%u.%u, Trident chip %s rev %u, ", CLI_MAJOR_VERSION, CLI_MINOR_VERSION, CLI_PATCH_VERSION, chip_id_string, chip_rev);
  cli_uart_print("git hash ");
  for (int i=0; i<8; i++)
  {
    cli_uart_printf("%c", git_hash[i]);
  }
  cli_uart_print("\n");
  cli_radio_fw_version_get();
  cli_uart_printf("Reset reason %i - %s\n", last_reset_reason, ((RESET_REASON_MAX - 1) > last_reset_reason) ? reset_reason_table[last_reset_reason].reset_reason_string : reset_reason_table[RESET_REASON_MAX - 1].reset_reason_string);
  cli_uart_printf("Resets since Power ON - %u\n", zpal_get_restarts());
}

void cli_radio_current_setting_print(void)
{
  cli_uart_printf("Radio %s, %sdBm hardware configuration\n", radio_initialized ? "initialized" : "uninitialized",
                              current_radio_tx_max_power_20dbm ? "20" : "14");
  cli_uart_printf("Current Z-Wave Region:\n %3u - %s\n", current_region, helper_get_region_string(current_region));
  cli_uart_printf("Script autorun %s (%d)\n", cli_script_autorun.autorun ? "enabled" : "disabled", cli_script_autorun.autorun);
}

bool cli_radio_tx_power_index_set(uint8_t channel_id, int8_t txpower, uint8_t power_index)
{
  return zpal_radio_tx_power_power_index_set(channel_id, txpower, power_index);
}

void cli_radio_tx_power_index_list(uint8_t channel_id)
{
  uint8_t const * power_index_table;
  bool power_index_table_changed = zpal_radio_dBm_to_power_index_table_get(channel_id, &power_index_table);
  int8_t min_power = zpal_radio_min_tx_power_get(channel_id);
  int8_t max_power = zpal_radio_max_tx_power_get(channel_id);

  cli_uart_printf("Conversion table for dBm to radio power_index used on channel %d - %s\n", channel_id, power_index_table_changed ? "changed" : "default");
  for (int i = min_power; i <= max_power; i++)
  {
    cli_uart_printf("power_index used for %idBm = %d\n", i, power_index_table[i - min_power]);
  }
}

static int scriptNextEntry = 0;

static void radio_cli_script_next_command(void)
{
  if (current_script_running && (active_scriptlines > scriptNextEntry))
  {
    uint8_t len = helper_strnlen(active_script[scriptNextEntry].entry, sizeof(active_script[scriptNextEntry].entry));
    scriptTransitionEvent = radio_cli_script_next_command;
    cli_command_execute(active_script[scriptNextEntry++].entry, len);
  }
  else
  {
    current_script_running = false;
    scriptNextEntry = 0;
    scriptTransitionEvent = NULL;
  }
}

static void radio_cli_script_initiate_run_script(void)
{
  if ((NULL != active_script) && (0 < active_scriptlines))
  {
    current_script_running = true;
    scriptTransitionEvent = radio_cli_script_next_command;
    cli_radio_script_state_transition_event();
  }
  else
  {
    cli_uart_print("No script defined - cannot run script\n");
    current_script_running = false;
    scriptTransitionEvent = NULL;
  }
}

static uint8_t current_script_line = 0;
static uint8_t current_script_line_char_pos = 0;

static void radio_cli_script_char_entry(char ch)
{
  cli_uart_printf("%c", ch);
  if (('\n' == ch) || ('\r' == ch) || (sizeof(scriptline_t) - 1 == current_script_line_char_pos))
  {
    dynamic_script[current_script_line].entry[current_script_line_char_pos] = '\0';
    if ((0 == current_script_line_char_pos) || (current_script_line >= DYNAMIC_SCRIPT_MAXCOUNT - 1))
    {
#ifdef USE_DEFAULT_SCRIPT_IF_NO_DYNAMIC_EXISTS
      uint8_t current_script_file_number = script_file_number;
#endif
      dynamic_scriptlines = current_script_line;
      if (dynamic_scriptlines)
      {
        active_script = &dynamic_script[0];
        active_scriptlines = dynamic_scriptlines;
        dynamic_script_active = true;
        cli_uart_print("\nScript entry done\n");
      }
      else
      {
#ifdef USE_DEFAULT_SCRIPT_IF_NO_DYNAMIC_EXISTS
        current_script_file_number = 0;
        active_script = &default_script[0];
        active_scriptlines = DEFAULT_SCRIPT_COUNT;
        cli_uart_print("\nScript cleared - default script active\n");
#else
        cli_uart_printf("\nScript %d cleared\n", script_file_number);
        active_scriptlines = 0;
#endif
        dynamic_script_active = false;
      }
      cli_uart_receive_hook_set(NULL);
      cli_store_script(script_file_number);
#ifdef USE_DEFAULT_SCRIPT_IF_NO_DYNAMIC_EXISTS
      script_file_number = current_script_file_number;
#endif
      current_script_line = 0;
      current_script_line_char_pos = 0;
    }
    else
    {
      cli_uart_print("\n");
      current_script_line++;
      current_script_line_char_pos = 0;
    }
  }
  else
  {
    dynamic_script[current_script_line].entry[current_script_line_char_pos++] = ch;
  }
}

void radio_cli_script_list(int8_t script_number)
{
  if (-1 != script_number)
  {
    cli_restore_script(script_number);
  }
  if (active_scriptlines)
  {
    cli_uart_printf("\nScript %d ***\n", script_number);
    for (int i = 0; i < active_scriptlines; i++)
    {
      cli_uart_printf("%s\n", active_script[i].entry);
    }
  }
}

static void radio_script_start(int8_t script_number)
{
  if (!current_script_running)
  {
    if (-1 != script_number)
    {
      script_file_number = script_number;
    }
    current_script_run_stop = false;
    cli_uart_printf("Clearing Script %d\n", script_file_number);
    memset(dynamic_script, 0, sizeof(dynamic_script));
    cli_uart_print("End script with CR on empty line\n");
    cli_uart_receive_hook_set(radio_cli_script_char_entry);
  }
  else
  {
    cli_uart_printf("Script %d running - cannot execute script start inside script run\n", script_file_number);
  }
}

static void radio_script_stop(void)
{
  if (current_script_running && internal_buffer.frame_repeat)
  {
    current_script_run_stop = true;
  }
  current_script_running = false;
}

static void radio_script_run(int8_t script_number)
{
  scriptNextEntry = 0;
  if (!current_script_running)
  {
    if ((-1 != script_number) && (script_file_number != script_number))
    {
      script_file_number = script_number;
      cli_restore_script(script_file_number);
    }
    radio_cli_script_initiate_run_script();
  }
  else
  {
    cli_uart_printf("Script %d running - cannot execute script run inside script run\n", script_file_number);
  }
}

static void radio_script_list(int8_t script_number)
{
  if (!current_script_running)
  {
    if (-1 == script_number)
    {
#ifdef USE_DEFAULT_SCRIPT_IF_NO_DYNAMIC_EXISTS
      if (active_script == &default_script[0])
      {
        radio_cli_script_list(0);
      }
#endif
      // List all dynamic scripts
      for (int i = 1; i < 6; i++)
      {
        radio_cli_script_list(i);
      }
    }
    else
    {
      radio_cli_script_list(script_number);
    }
    if (script_number != script_file_number)
    {
      cli_restore_script(script_file_number);
      cli_uart_printf("\nActive script %d (%s)\n", script_file_number, active_scriptlines ? "present" : "empty");
    }
  }
  else
  {
    cli_uart_printf("Script %d running - cannot execute script list inside script run\n", script_file_number);
  }
}

static void radio_script_autorun_on(int8_t script_number)
{
  int8_t tmp_script_number = script_number;
  if ((-1 != script_number) && (script_file_number != script_number))
  {
    cli_restore_script(tmp_script_number);
  }
  else if (-1 == script_number)
  {
    tmp_script_number = script_file_number;
  }
  if (active_scriptlines)
  {
    current_script_autorun = tmp_script_number;
    cli_uart_printf("Script %d autorun on upstart enabled\n", tmp_script_number);
  }
  else
  {
    current_script_autorun = 0;
    cli_uart_printf("Script %d autorun on upstart disabled - no script defined\n", script_number);
  }
  cli_store_autorun();
  if ((-1 != script_number) && (script_file_number != script_number))
  {
    cli_restore_script(script_file_number);
  }
}

static void radio_script_autorun_off(void)
{
  current_script_autorun = 0;
  cli_store_autorun();
  cli_uart_print("Script autorun on upstart disabled\n");
}

static void radio_script_clear(int8_t script_number)
{
  if (!current_script_running)
  {
    uint8_t tmp_script_number = (script_number != -1) ? script_number : script_file_number;
    memset(dynamic_script, 0, sizeof(dynamic_script));
    dynamic_script_active = 0;
    dynamic_scriptlines = 0;
#ifdef USE_DEFAULT_SCRIPT_IF_NO_DYNAMIC_EXISTS
    active_script = &default_script[0];
    active_scriptlines = DEFAULT_SCRIPT_COUNT;
    cli_uart_printf("\nScript %d cleared\n", tmp_script_number);
#else
    active_scriptlines = 0;
    current_script_autorun = false;
    cli_store_autorun();
    cli_uart_printf("\nScript %d cleared\n", tmp_script_number);
#endif
    cli_store_script(tmp_script_number);
    if (script_file_number != tmp_script_number)
    {
      cli_restore_script(script_file_number);
    }
  }
  else
  {
    cli_uart_printf("Script %d running - cannot execute script clear inside script run\n", script_file_number, script_number);
  }
}

void cli_radio_script(radio_cli_script_cmd_t script_state_request, int8_t script_number)
{
  switch (script_state_request)
  {
    case SCRIPT_START:
      radio_script_start(script_number);
      break;

    case SCRIPT_STOP:
      radio_script_stop();
      break;

    case SCRIPT_RUN:
      radio_script_run(script_number);
      break;

    case SCRIPT_LIST:
      radio_script_list(script_number);
      break;

    case SCRIPT_AUTORUN_ON:
      radio_script_autorun_on(script_number);
      break;

    case SCRIPT_AUTORUN_OFF:
      radio_script_autorun_off();
      break;

    case SCRIPT_CLEAR:
      radio_script_clear(script_number);
      break;

    default:
      cli_uart_printf("\nUndefined script command - %d\n", script_state_request);
      break;
  }
}

void cli_radio_status_get(const radio_cli_tx_frame_config_t *const tx_frame_config)
{
  char print_buf[300];
  zpal_radio_mode_t mode;
  uint8_t home_id_hash;

  cli_radio_version_print();
  cli_radio_current_setting_print();
  zpal_radio_get_network_ids(&internal_network.homeID_uint32, &internal_network.node_id, &mode, &home_id_hash);
  cli_uart_printf("Homeid 0x%08X, Nodeid %d, HomeidHash 0x%02X, mode %s\n", internal_network.homeID_uint32, internal_network.node_id, home_id_hash,
                  (mode == ZPAL_RADIO_MODE_NON_LISTENING) ? "NONE LISTENING" : (mode == ZPAL_RADIO_MODE_ALWAYS_LISTENING) ? "ALWAYS LISTENING"
                                                                                                                          : "FLIRS");
  cli_uart_printf("Network filtering %s\n", zpal_radio_network_id_filter_enabled_get() ? "Enabled" : "Disabled");
  cli_uart_printf("Tx channel %d\n", tx_frame_config->channel);
  cli_uart_printf("Z-Wave Classic Tx Power setting     - %s\n", cli_radio_zw_classic_tx_power_is_enabled() ? "Enabled" : "Disabled");
  cli_uart_printf("Z-Wave Classic Tx Power             - %3d deci dBm\n", cli_radio_classic_tx_power_get());
  cli_uart_printf("Z-Wave Classic Tx Power adjust      - %3d deci dBm\n", cli_radio_classic_tx_power_adjust_get());
  cli_uart_printf("Z-Wave Classic Tx Power attenuation - %3d dBm\n", cli_radio_classic_tx_power_attenuation_get());
  if (cli_radio_zw_classic_tx_power_is_enabled()) {
    uint16_t classic_tx_power = ((cli_radio_classic_tx_power_get() - cli_radio_classic_tx_power_adjust_get()) / 10)
                                - cli_radio_classic_tx_power_attenuation_get() + tx_frame_config->power;
    int8_t tx_max_power = zpal_radio_max_tx_power_get(0);
    if (classic_tx_power > tx_max_power) {
      classic_tx_power = tx_max_power;
    }
    cli_uart_printf("Z-Wave Classic Tx Power currently used is %3d dBm\n", classic_tx_power);
  }
  cli_uart_printf("Tx power %idBm\n", tx_frame_config->power);
  cli_uart_printf("Tx delay %dms\n", tx_frame_config->delay);
  generate_string_from_array(print_buf, sizeof(print_buf), tx_frame_config->payload_buffer, tx_frame_config->payload_length);
  cli_uart_printf("Tx payload %s\n", print_buf);
  cli_uart_printf("Tx LBT %s - threshold %idBm\n", tx_frame_config->lbt ? "ON" : "OFF", zpal_radio_get_lbt_level(tx_frame_config->channel));
  cli_uart_printf("Tx force crc-fail %s\n", transmit_option_force_checksum_errors ? "enabled" : "disabled");
  cli_uart_printf("Rx %s, %s", rx_status.receiver_running ? "ON" : "OFF", rx_status.receiver_running_single_channel_scan ? "singlechannel" : "channel scan");
  if (rx_status.receiver_running_single_channel_scan) {
    cli_uart_printf(" on channel %d", rx_status.channel);
  }
  cli_uart_print("\n");
  cli_radio_timestamp_print();
}

void helper_print_memory(const uint8_t *data, uint32_t data_length, uint8_t bytes_pr_row)
{
  for (uint8_t row = 0; row < data_length / bytes_pr_row; row++)
  {
    uint32_t row_addr = row * bytes_pr_row;
    cli_uart_printf("0x%08x: ", row_addr);
    for (uint8_t hex_byte = 0; hex_byte < bytes_pr_row; hex_byte++)
    {
      cli_uart_printf("%02x ", data[row_addr + hex_byte]);
    }
    cli_uart_print("\n");
  }
}

#define FT_CALIBERATION_DATA_PAGE 0x00000000
#define FT_USER_DATA_PAGE         0x00001200

void cli_system_dumpft()
{
  uint8_t sec_page_buf[LENGTH_PAGE];

  flash_read_sec_register((uint32_t)sec_page_buf, FT_CALIBERATION_DATA_PAGE);
  helper_print_memory(sec_page_buf, LENGTH_PAGE, 16);
}

void cli_system_dumpmp()
{
  const void * mp_sector_addr = &__mfg_tokens_production_region_start;

  helper_print_memory(mp_sector_addr, 4096, 32);
}

void cli_system_dumpuft()
{
  uint8_t sec_page_buf[LENGTH_PAGE];

  flash_read_sec_register((uint32_t)sec_page_buf, FT_USER_DATA_PAGE);
  helper_print_memory(sec_page_buf, LENGTH_PAGE, 16);
}

void cli_calibration_change_xtal(uint16_t xtal_value)
{
  tr_platform_token_process_xtal_trim(xtal_value);
}

void cli_calibration_read_xtal(uint16_t *xtal_cal)
{
  tr_mfg_tok_type_xtal_trim xtal_trim;
  tr_get_mfg_token((uint8_t*)&xtal_trim, TR_MFG_TOKEN_XTAL_TRIM);
  *xtal_cal = (uint16_t)xtal_trim.value;
}

void cli_calibration_store_xtal(uint16_t xtal_cal)
{
  tr_mfg_tok_type_xtal_trim xtal_trim;
  xtal_trim.value = xtal_cal;
  tr_set_mfg_token((uint8_t*)&xtal_trim, sizeof(tr_mfg_tok_type_xtal_trim), TR_MFG_TOKEN_XTAL_TRIM);
}

void cli_calibration_store_xtal_sec_reg(uint16_t xtal_cal)
{
  uint8_t sec_page_buf[4][LENGTH_PAGE];
  uint8_t xtal_trim[2];
  flash_read_sec_register((uint32_t)&sec_page_buf[0], FLASH_SECREG_R1_P0);
  flash_read_sec_register((uint32_t)&sec_page_buf[1], FLASH_SECREG_R1_P1);
  flash_read_sec_register((uint32_t)&sec_page_buf[2], FLASH_SECREG_R1_P2);
  flash_read_sec_register((uint32_t)&sec_page_buf[3], FLASH_SECREG_R1_P3);
  flash_erase(FLASH_ERASE_SECURE, FLASH_SECREG_R1_P0);
  // convert to big-endian order
  xtal_trim[0] = (uint8_t)(xtal_cal >> 8);
  xtal_trim[1] = (uint8_t)(xtal_cal & 0xFF);
  update_tlv_tag(sec_page_buf[2], LENGTH_PAGE, XTAL_TRIM_TAG, xtal_trim, XTAL_TRIM_SIZE);
  flash_write_sec_register((uint32_t)&sec_page_buf[0], FLASH_SECREG_R1_P0);
  while (flash_check_busy()) {/*do nothing*/};
  flash_write_sec_register((uint32_t)&sec_page_buf[1], FLASH_SECREG_R1_P1);
  while (flash_check_busy()) {/*do nothing*/};
  flash_write_sec_register((uint32_t)&sec_page_buf[2], FLASH_SECREG_R1_P2);
  while (flash_check_busy()) {/*do nothing*/};
  flash_write_sec_register((uint32_t)&sec_page_buf[3], FLASH_SECREG_R1_P3);
  while (flash_check_busy()) {/*do nothing*/};
}

void cli_calibration_read_xtal_sec_reg(uint16_t *xtal_cal)
{
  uint8_t sec_page_buf[LENGTH_PAGE];
  uint8_t xtal_value[2];
  uint8_t xtal_value_len;
  flash_read_sec_register((uint32_t)sec_page_buf, FLASH_SECREG_R1_P2);
   if (!find_tlv_by_tag(sec_page_buf ,LENGTH_PAGE, 0x01, xtal_value, &xtal_value_len) ||
       (xtal_value_len != sizeof(xtal_value)))
   {
    xtal_value[0] = 0xFF;
    xtal_value[1] = 0xFF;
  }
  *xtal_cal = ((uint16_t)xtal_value[1]) | ((uint16_t)(xtal_value[0] << 8));
}

static bool wdog_enabled = false;

void zpal_zw_pm_event_handler(zpal_pm_mode_t from, zpal_pm_mode_t to)
{
  if (to > from)
  {
    // Nothing
  }
  else if (to < ZPAL_PM_MODE_DEEP_SLEEP && from >= ZPAL_PM_MODE_DEEP_SLEEP)
  {
    // Wake up from deep sleep
    zpal_radio_start_receive_after_power_down(false);
  }

  if (to > ZPAL_PM_MODE_DEEP_SLEEP)
  {
    // We are going into shut down
    zpal_radio_power_down();
  }
  else if (to == ZPAL_PM_MODE_DEEP_SLEEP)
  {
    /* zpal_radio_power_down is called to make sure that the RF FSM is up to date. */
    zpal_radio_power_down();
  }
}

void enterPowerDown(__attribute__((unused)) uint32_t millis)
{
  wdog_enabled = zpal_is_watchdog_enabled();

  if (wdog_enabled)
  {
    zpal_enable_watchdog(false);
  }
}

void exitPowerDown(__attribute__((unused)) uint32_t millis)
{
  if (wdog_enabled)
  {
    zpal_enable_watchdog(wdog_enabled);
  }
}

/*
 * Application Task
 * Initializes the system and service the cli and the event manager
 */
void
ZwaveCliTask(void* unused_prt)
{
  zpal_status_t status;

  // Make radioCliTaskhandle available outside radioCli task context
  g_radioCliTaskHandle = xTaskGetCurrentTaskHandle();

#ifdef DEBUGPRINT
  // Setup debug printing
  zpal_debug_init(&CLI_UART_CONFIG);
  static uint8_t m_aDebugPrintBuffer[96];
  DebugPrintConfig(m_aDebugPrintBuffer, sizeof(m_aDebugPrintBuffer), zpal_debug_output);
  DPRINTF("ApplicationInit\n");
#endif
  // Initialize NVM storage
  cli_nvm_handle = zpal_nvm_init(ZPAL_NVM_AREA_APPLICATION);
  DPRINTF("Flash file system initialized %s\n", cli_nvm_handle == NULL ? "Failed" : "Succesfully");

  cli_restore_config();
  cli_restore_autorun();
  if (current_script_autorun)
  {
    script_file_number = current_script_autorun;
  }
  cli_restore_script(script_file_number);
#ifdef USE_DEFAULT_SCRIPT_IF_NO_DYNAMIC_EXISTS
  if (!active_scriptlines)
  {
    active_script = &default_script[0];
    active_scriptlines = DEFAULT_SCRIPT_COUNT;
    cli_uart_print("\nNo active script - default script made active\n");
  }
#endif
  EventDistributorConfig(&g_EventDistributor,
                         sizeof(g_aEventHandlerTable)/sizeof(*g_aEventHandlerTable),
                         g_aEventHandlerTable,
                         NULL);

  // Enable watchdog
  zpal_enable_watchdog(true);

  // Initialize the uart and the CLI
  cli_uart_init(&CLI_UART_CONFIG, UartReceiveEvent);

  radio_cli_rf_debug();

  // Register the protocol powerlocks, the radio will be off in shut down and above
  application_radio_power_lock = zpal_pm_register(ZPAL_PM_TYPE_USE_RADIO);
  zpal_pm_stay_awake(application_radio_power_lock, 0);

  cli_radio_version_print();
  cli_radio_current_setting_print();

  DPRINT("Trident Radio Test CLI task started\n");
  if (current_script_autorun)
  {
    radio_cli_script_initiate_run_script();
  }
  uint32_t iEventWait = 0xFFFF;
  while (1)
  {
    EventDistributorDistribute(&g_EventDistributor, iEventWait, 0);
    // We never want to return from the task
  }
}
