/// ****************************************************************************
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************

/**************************************************************************************************
 *    INCLUDES
 *************************************************************************************************/
#include "cm33.h"
#include "Assert.h"  // NOSONAR
#include "rfb.h"
#include "rfb_port.h"
#include "rf_mcu.h"
#include "sysctrl.h"
#include "zwave_radio.h"
#include "rfb_comm_common.h"
#include "mp_sector.h"
//#define DEBUGPRINT // NOSONAR
#include <DebugPrint.h>
#ifdef DEBUGPRINT
#include <stdio.h>
#endif
#include <FreeRTOS.h>
#include <task.h>
#include "tr_hal_gpio.h"
#include "zpal_misc.h"
/**************************************************************************************************
 *    CONSTANTS AND DEFINES
 *************************************************************************************************/
#define RUCI_HEADER_LENGTH      (1)
#define RUCI_SUB_HEADER_LENGTH  (1)
#define RUCI_LENGTH             (2)
#define RUCI_PHY_STATUS_LENGTH  (3)
#define RX_CONTROL_FIELD_LENGTH (RUCI_HEADER_LENGTH+RUCI_SUB_HEADER_LENGTH+RUCI_LENGTH+RUCI_PHY_STATUS_LENGTH)
#define FSK_PHR_LENGTH          (2)
#define FSK_RX_HEADER_LENGTH    (RX_CONTROL_FIELD_LENGTH + FSK_PHR_LENGTH)

#define FREQ_900MHZ             900000

#define ZWAVE_SCAN_STAGE_MAX 7

#define GPIO_RF_DEBUG_ENABLED           (0)

#define GPIO_RF_DEBUG_RX                (21)
#define GPIO_RF_DEBUG_TX                (28)
#define GPIO_RF_DEBUG_CPU_WAKE          (0)

#define BROADCAST_NODE_ID               0xFF    /* broadcast destination nodeid */
#define BROADCAST_BEAM_NODE_ID          0xFF    /* broadcast destination nodeid for wakeup beam */
#define BROADCAST_NODE_ID_LR            0xFFF   /* broadcast destination nodeid for LR */
#define BROADCAST_BEAM_NODE_ID_LR       0xFFF   /* broadcast destination nodeid for LR wakeup beam */

#define ZWAVE_RADIO_DEFAULT_SAMPLE_FREQUENCY 498 // Every 498th channel scan cycle a RSSI sample is taken - a scan cycle is max 2ms
#define ZWAVE_RADIO_DEFAULT_SAMPLE_COUNT     2   // 2 RSSI sample for generating resulting RSSI average - two latest RSSI samples are averaged

#define PROTOCOL_MODE_COUNT 4

/**
 * FLiRS wake up time in ms for different protocol modes and settings
 */
#define ZWAVE_RADIO_FLIRS_WAKE_UP_TIME_250MS_LEAGACY_2CHANNEL   248
#define ZWAVE_RADIO_FLIRS_WAKE_UP_TIME_1000MS_LEAGACY_2CHANNEL  998
#define ZWAVE_RADIO_FLIRS_WAKE_UP_TIME_250MS_LEAGACY_3CHANNEL   222
#define ZWAVE_RADIO_FLIRS_WAKE_UP_TIME_1000MS_LEAGACY_3CHANNEL  897
#define ZWAVE_RADIO_FLIRS_WAKE_UP_TIME_250MS_LR                 223
#define ZWAVE_RADIO_FLIRS_WAKE_UP_TIME_1000MS_LR                898

/**************************************************************************************************
 *    TYPEDEFS
 *************************************************************************************************/

/**************************************************************************************************
 *    LOCAL VARIABLES
 *************************************************************************************************/

/* Flirs node wake up time according to protocol mode - 250ms and 1000ms setting */
static const uint16_t protocol_mode_flirs_wake_up_time[4][2] = {
                                                                 {ZWAVE_RADIO_FLIRS_WAKE_UP_TIME_250MS_LEAGACY_2CHANNEL,
                                                                  ZWAVE_RADIO_FLIRS_WAKE_UP_TIME_1000MS_LEAGACY_2CHANNEL},
                                                                 {ZWAVE_RADIO_FLIRS_WAKE_UP_TIME_250MS_LEAGACY_3CHANNEL,
                                                                  ZWAVE_RADIO_FLIRS_WAKE_UP_TIME_1000MS_LEAGACY_3CHANNEL},
                                                                 {ZWAVE_RADIO_FLIRS_WAKE_UP_TIME_250MS_LEAGACY_2CHANNEL,
                                                                  ZWAVE_RADIO_FLIRS_WAKE_UP_TIME_1000MS_LEAGACY_2CHANNEL},
                                                                 {ZWAVE_RADIO_FLIRS_WAKE_UP_TIME_250MS_LR,
                                                                  ZWAVE_RADIO_FLIRS_WAKE_UP_TIME_1000MS_LR}
                                                               };

static zpal_radio_zwave_channel_t m_rx_last_beam_channel;
static int8_t m_rx_last_beam_tx_power;
static uint8_t m_rx_last_beam_rssi;
static uint8_t m_rx_last_beam_data[4];
static uint16_t m_rx_last_beam_node_id = 0;
static uint8_t m_radio_last_tx_channel;
static bool g_rx_broadcast_beam_enabled = false;

static uint32_t rx_beam_count;
static uint32_t rx_current_beam_count;
static uint32_t rx_stop_beam_count_last;
static uint32_t rx_timeout_count;
static int8_t current_tx_power = -127;
static bool is_rx_flirs = false;

static uint16_t m_rssi_sample_frequency = ZWAVE_RADIO_DEFAULT_SAMPLE_FREQUENCY;
static uint8_t  m_rssi_sample_count_average = ZWAVE_RADIO_DEFAULT_SAMPLE_COUNT;
static uint8_t m_rssi_average[ZWAVE_SCAN_STAGE_MAX] = {ZPAL_RADIO_INVALID_RSSI_DBM};

static bool m_radio_network_filter_enabled = false;

/**************************************************************************************************
 *    GLOBAL VARIABLES
 *************************************************************************************************/
rfb_interrupt_event_t struct_rfb_interrupt_event;

static uint32_t m_scan_rx_count[7];

static uint32_t m_tx_beam_count;

static zw_protocol_mode_rf_setup_t protocol_mode[PROTOCOL_MODE_COUNT] =
{
  {{0,0,0,0}, {ZWAVE_100K, ZWAVE_40K, ZWAVE_9P6K}, {FSK, FSK, GFSK}, {CRC_16, CRC_16, CRC_16}, {10, 20, 40},
   .band_type = 0, .use_fixed_rx_channel = 0, .fixed_rx_channel = 0, .channel_scan_mode = ZWAVE_SCANNING_LEGACY_R2_ENHANCE},
  {{0,0,0,0}, {ZWAVE_100K, ZWAVE_100K, ZWAVE_100K}, {GFSK, GFSK, GFSK}, {CRC_16, CRC_16, CRC_16}, {24, 24, 24},
   .band_type = 0, .use_fixed_rx_channel = 0, .fixed_rx_channel = 0, .channel_scan_mode = ZWAVE_SCANNING_R3_SWITCH},
  {{0,0,0,0}, {ZWAVE_100K, ZWAVE_40K, ZWAVE_9P6K, ZWAVE_LR}, {FSK, FSK, GFSK, OQPSK}, {CRC_16, CRC_16, CRC_16, CRC_16}, {10, 20, 40, 40},
   .band_type = 0, .use_fixed_rx_channel = 0, .fixed_rx_channel = 0, .channel_scan_mode = ZWAVE_SCANNING_NORMAL_R2_ENHANCE},
  {{0,0,0,0}, {ZWAVE_LR, ZWAVE_LR, 0, 0}, {OQPSK, OQPSK}, {CRC_16, CRC_16}, {40, 40},
   .band_type = 0, .use_fixed_rx_channel = 0, .fixed_rx_channel = 0, .channel_scan_mode = ZWAVE_SCANNING_LR_SWITCH},
};

/* tx-related flag */
static bool g_tx_done = true;
static bool g_tx_beam = false;

/* radio parameters */
static uint8_t g_phy_cca_threshold;
static RFB_EVENT_STATUS m_radio_last_status = RFB_CNF_EVENT_INVALID_CMD;

#define OFFSET_TX_POWER_CONVERSION_0dBm_US_14dBm 21
#define MAX_TX_POWER_CONVERSION_US_14dBm         (sizeof(radio_rf_tx_power_conversion_US_14dBm) - (OFFSET_TX_POWER_CONVERSION_0dBm_US_14dBm + 1))
// 14dBm US Legacy Tx Power conversion Table based on RT584_Tx_Power_Table_20241218_v2.xlsx - US and LR channels "14 dBm Low Power Table Shrink"
// - dBm -> power_index (rfb_port_tx_power_set_oqpsk)
// index 0 ~= -21dBm, 1 ~= -20dBm, ..., 35 ~= 14dBm
static uint8_t const radio_rf_tx_power_conversion_US_14dBm[] = {148, 149, 150, 151, 152, 153, 154, 155, 156, 157,
                                                                159, 161, 163, 164, 165, 166, 167, 168, 169, 170,
                                                                171, 172, 173, 175, 176, 177, 178, 179, 181, 183,
                                                                185, 186, 188, 189, 190, 191};

#define OFFSET_TX_POWER_CONVERSION_0dBm_US_20dBm 21
#define MAX_TX_POWER_CONVERSION_US_20dBm         (sizeof(radio_rf_tx_power_conversion_US_20dBm) - (OFFSET_TX_POWER_CONVERSION_0dBm_US_20dBm + 1))
// 20dBm US Legacy channels Tx Power conversion Table based on RT584_Tx_Power_Table_20241218_v2.xlsx - US and LR channels "20 dBm Low Power Table Shrink"
// - dBm -> power_index (rfb_port_tx_power_set_oqpsk)
// Table index 0 ~= -21dBm, 1 ~= -20dBm, ..., 41 ~= 20dBm
// Added new/extra entries making table go to 20dBm (16dBm was 253 now 117)
static uint8_t const radio_rf_tx_power_conversion_US_20dBm[] = {207, 209, 210, 211, 212, 213, 214, 215, 217, 219,
                                                                221, 222, 223, 224, 225, 226, 227, 228, 229, 230,
                                                                231, 233, 234, 235, 236, 237, 238, 239, 240, 242,
                                                                246, 247, 248, 249, 250, 251, 252, 117, 119, 120,
                                                                122, 124 };

#define OFFSET_TX_POWER_CONVERSION_0dBm_14dBm 12
#define MAX_TX_POWER_CONVERSION_14dBm         (sizeof(radio_rf_tx_power_conversion_14dBm) - (OFFSET_TX_POWER_CONVERSION_0dBm_14dBm + 1))
// 14dBm Tx Power conversion Table based on RT584_Tx_Power_Table_20241218_v2.xlsx - All region channels except US and LR channels "14 dBm High Power Table Shrink"
// - dBm -> power_index (rfb_port_tx_power_set_oqpsk)
// Table index 0 ~= -12dBm, 1 ~= -20dBm, ..., 26 ~= 14dBm
static uint8_t const radio_rf_tx_power_conversion_14dBm[] = {28, 29, 31, 32, 33, 34, 35, 36, 37, 38,
                                                             39, 40, 41, 42, 43, 44, 45, 47, 48, 49,
                                                             50, 52, 54, 56, 58, 60, 62};

#define OFFSET_TX_POWER_CONVERSION_0dBm_20dBm 11
#define MAX_TX_POWER_CONVERSION_20dBm         (sizeof(radio_rf_tx_power_conversion_20dBm) - (OFFSET_TX_POWER_CONVERSION_0dBm_20dBm + 1))
// 20dBm Tx Power conversion Table based on RT584_Tx_Power_Table_20241218_v2.xlsx - All region channels except US and LR channels "20 dBm High Power Table Shrink"
// - dBm -> power_index (rfb_port_tx_power_set_oqpsk)
// Table index 0 ~= -11dBm, 1 ~= -10dBm, ..., 31 ~= 20dBm
static uint8_t const radio_rf_tx_power_conversion_20dBm[] = {76, 78, 80, 82, 83, 84, 85, 86, 87, 88,
                                                             89, 90, 92, 94, 96, 97, 98, 99, 100, 101,
                                                             102, 104, 105, 106, 109, 113, 116, 118, 119, 121,
                                                             123, 125};

// Default we use 14dBm Tx power conversion table
static int8_t radio_min_tx_power_US_dbm = (-OFFSET_TX_POWER_CONVERSION_0dBm_US_14dBm);
static int8_t radio_max_tx_power_US_dbm = MAX_TX_POWER_CONVERSION_US_14dBm;
static uint8_t const *radio_rf_tx_power_conversion_US_p = radio_rf_tx_power_conversion_US_14dBm;

static int8_t radio_min_tx_power_dbm = (-OFFSET_TX_POWER_CONVERSION_0dBm_14dBm);
static int8_t radio_max_tx_power_dbm = MAX_TX_POWER_CONVERSION_14dBm;
static uint8_t const *radio_rf_tx_power_conversion_p = radio_rf_tx_power_conversion_14dBm;
bool g_radio_tx_max_power_20dbm = false;

typedef enum
{
  POWER_INDEX_TABLE_ALL_REGION_CHANNELS_EXCEPT_US_CHANNELS_AND_LR,
  POWER_INDEX_TABLE_US_CHANNELS_AND_LR,
  POWER_INDEX_TABLE_COUNT,
} power_index_table_index_t;

#define MAX_TX_POWER_CONVERSION_TABLE_SIZE                  sizeof(radio_rf_tx_power_conversion_US_20dBm)

static uint8_t power_to_power_index_table[POWER_INDEX_TABLE_COUNT][MAX_TX_POWER_CONVERSION_TABLE_SIZE] = {{0}, {0}};
static bool power_to_power_index_table_active = false;
static bool power_to_power_index_table_US_and_LR_active = false;

static zpal_radio_rx_parameters_t rx_parameters;

/* rf control */
static rfb_subg_ctrl_t *g_rfb_ctrl;
void (*g_radio_init_cb)(void) = NULL;
void (*g_radio_dio_rf_debug_cb)(void) = NULL;

/* Tx buffer */
static uint8_t g_tx_buf[200];

/* radio parameters */
static uint8_t g_channel_id;
static uint8_t g_radio_speed;

/* radio profile */
static zpal_radio_profile_t g_radio;
static zw_region_rf_setup_t g_radio_region_setup;
static zpal_radio_protocol_mode_t g_radio_protocol_mode;

typedef struct radio_next_tx
{
  zpal_radio_zwave_channel_t channel;
  uint32_t time; // frame time in ms
  TickType_t transmit_time_start;
} radio_next_tx_t;

static radio_next_tx_t next_tx = {0};

typedef struct radio_tx_channel
{
  TickType_t start_tx_time;
  TickType_t start_channel_lock_time;
  TickType_t tx_time;
  bool channel_lock;
} radio_tx_channel_t;

static radio_tx_channel_t transmit_time_channel[5] = {0};

/* lbt-related */
bool is_lbt_enabled = true;
uint8_t g_cca_threshold[MAX_TOTAL_CHANNEL_NUM];

/* other parameters & flags */
static bool is_rx_start = false;
static node_id_t g_node_id = 0;
static uint32_t g_home_id = 0;
static uint8_t g_home_id_hash = 0x55;
static zpal_radio_mode_t g_radio_mode = ZPAL_RADIO_MODE_ALWAYS_LISTENING;

#ifdef DEBUGPRINT
static char str_buf[300];
#endif
/* Receive fifo */
#define ZWAVE_FRAME_RECEIVE_FIFO_MAX_ELEMENTS 6
static uint8_t in = 0;
static uint8_t out = 0;
static uint8_t cnt = 0;
static uint8_t maxcnt = 0;

#define ZWAVE_FRAME_RECEIVE_BUFFER_MAX_LENGTH 180

typedef struct ZwaveFrameReceived
{
  zpal_radio_rx_parameters_t parameters;
  uint8_t frame_buf_length;
  uint8_t frame_buf[ZWAVE_FRAME_RECEIVE_BUFFER_MAX_LENGTH];
} ZwaveFrameReceived_t;

static ZwaveFrameReceived_t frameReceiveFifo[ZWAVE_FRAME_RECEIVE_FIFO_MAX_ELEMENTS];

static bool zwave_radio_rx_frame_fifo_push(uint8_t *p_rx_frame, uint8_t frame_len, zpal_radio_rx_parameters_t *p_rx_parameters)
{
  bool res = false;

  if (ZWAVE_FRAME_RECEIVE_FIFO_MAX_ELEMENTS > cnt)
  {
    frameReceiveFifo[in].frame_buf_length = (ZWAVE_FRAME_RECEIVE_BUFFER_MAX_LENGTH > frame_len) ? frame_len : ZWAVE_FRAME_RECEIVE_BUFFER_MAX_LENGTH;
    memcpy(&frameReceiveFifo[in].frame_buf, p_rx_frame, frameReceiveFifo[in].frame_buf_length);
    memcpy(&frameReceiveFifo[in].parameters, p_rx_parameters, sizeof(zpal_radio_rx_parameters_t));
    in++;
    in %= ZWAVE_FRAME_RECEIVE_FIFO_MAX_ELEMENTS;
    cnt++;
    res = true;
    if (cnt > maxcnt)
    {
      maxcnt = cnt;
    }
  }
  return res;
}

bool zwave_radio_rx_frame_fifo_pop(zpal_radio_receive_frame_t *p_rx_frame, zpal_radio_rx_parameters_t *p_rx_parameters)
{
  bool res = false;

  if (0 != cnt)
  {
    p_rx_frame->frame_content_length = frameReceiveFifo[out].frame_buf_length;
    memcpy(p_rx_frame->frame_content, &frameReceiveFifo[out].frame_buf, p_rx_frame->frame_content_length);
    memcpy(p_rx_parameters, &frameReceiveFifo[out].parameters, sizeof(zpal_radio_rx_parameters_t));
    enter_critical_section();
    cnt--;
    leave_critical_section();
    out++;
    out %= ZWAVE_FRAME_RECEIVE_FIFO_MAX_ELEMENTS;
    res = true;
  }
  return res;
}

uint32_t zwave_radio_rx_frame_fifo_count(void)
{
  uint32_t res;
  enter_critical_section();
  res = cnt;
  leave_critical_section();
  return res;
}

/**************************************************************************************************
 *    LOCAL FUNCTIONS
 *************************************************************************************************/
static void radio_gpio_rf_debug(bool rf_state_enable);

static zpal_radio_header_type_t
frame_header_type_get(zpal_radio_protocol_mode_t mode, zpal_radio_zwave_channel_t ch)
{
  switch(mode)
  {
    case ZPAL_RADIO_PROTOCOL_MODE_1:
      return ZPAL_RADIO_HEADER_TYPE_2CH;

    case ZPAL_RADIO_PROTOCOL_MODE_2:
      return ZPAL_RADIO_HEADER_TYPE_3CH;

    case ZPAL_RADIO_PROTOCOL_MODE_3:
      if (ch < ZPAL_RADIO_ZWAVE_CHANNEL_3)
      {
        return ZPAL_RADIO_HEADER_TYPE_2CH;
      }
      else
      {
        return ZPAL_RADIO_HEADER_TYPE_LR;
      }

    case ZPAL_RADIO_PROTOCOL_MODE_4:
      return ZPAL_RADIO_HEADER_TYPE_LR;

    default:
      return ZPAL_RADIO_HEADER_TYPE_UNDEFINED;
  }
}

static void radio_rx_done(__attribute__((unused)) uint16_t ruci_packet_length, uint8_t *rx_data_address, uint8_t crc_status, uint8_t rssi, __attribute__((unused)) uint8_t snr)
{
  uint16_t rx_data_len;
  uint8_t data_offset = FSK_RX_HEADER_LENGTH - 2;

  g_radio.network_stats->rx_frames++;
  rx_data_len = rx_data_address[data_offset + 7];
  if (0 == protocol_mode[g_radio_protocol_mode].use_fixed_rx_channel)
  {
    uint8_t scan_stage = g_rfb_ctrl->zwave_scan_stage_get();
    m_scan_rx_count[scan_stage]++;
    g_channel_id = convert_phy_scan_stage_to_phy_channel(g_radio_protocol_mode, scan_stage);
  }
  if (crc_status == 0)
  {
    rx_parameters.channel_id = convert_phy_channel_to_zpal_channel(g_radio_protocol_mode, g_channel_id);
    rx_parameters.speed = g_radio_region_setup.channel_baudrate[g_channel_id];
    rx_parameters.channel_header_format = frame_header_type_get(g_radio_protocol_mode, rx_parameters.channel_id);
    if (rssi < 128)
    {
      rx_parameters.rssi = (-1) * rssi;
    }
    else
    {
      rx_parameters.rssi = -127;
    }
    zwave_radio_rx_frame_fifo_push(&rx_data_address[data_offset], rx_data_len, &rx_parameters);

    g_radio.rx_cb(ZPAL_RADIO_EVENT_RX_COMPLETE);
  }
  else
  {
    g_radio.network_stats->rx_crc_errors++;
    g_radio.rx_cb(ZPAL_RADIO_EVENT_RX_ABORT);
  }
  if (0 != rx_current_beam_count)
  {
    rx_stop_beam_count_last = rx_current_beam_count;
    rx_current_beam_count = 0;
    if ((false == g_rx_broadcast_beam_enabled) && (m_rx_last_beam_node_id == BROADCAST_BEAM_NODE_ID || m_rx_last_beam_node_id == BROADCAST_BEAM_NODE_ID_LR))
    {
      // Ignore broadcast beam
      return;
    }
    // BEAM has been Received - time out to indicate Beam ended
    g_radio.rx_cb(ZPAL_RADIO_EVENT_RX_TIMEOUT);
  }
}

static void radio_tx_done(uint8_t tx_status)
{
  zpal_radio_event_t radio_event = ZPAL_RADIO_EVENT_NONE;
  g_radio.network_stats->tx_frames++;

  /* tx_status =
  0x00: TX success
  0x10: CSMA-CA fail
  */
  if ((tx_status != 0))
  {
    if (tx_status == 0x10)
    {
      g_radio.network_stats->tx_lbt_back_offs++;
      radio_event = ZPAL_RADIO_EVENT_TX_FAIL_LBT;
    }
    else
    {
      radio_event = ZPAL_RADIO_EVENT_TX_FAIL;
    }
  }
  else
  {
    if (true == g_tx_beam)
    {
      m_tx_beam_count++;
      radio_event = ZPAL_RADIO_EVENT_TX_BEAM_COMPLETE;
    }
    else
    {
      radio_event = ZPAL_RADIO_EVENT_TX_COMPLETE;
    }
  }
  // update time on channel housekeeping - only used in three channel regions
  radio_update_transmit_time_phy_channel(g_channel_id);
  m_radio_last_tx_channel = convert_phy_channel_to_zpal_channel(g_radio_protocol_mode, g_channel_id);
  radio_network_statistics_update_tx_time();
  g_radio.tx_cb(radio_event);
  if (0 != protocol_mode[g_radio_protocol_mode].use_fixed_rx_channel)
  {
    // Fake to always able to receive on fixed channel
    g_channel_id = protocol_mode[g_radio_protocol_mode].fixed_rx_channel;
    m_radio_last_status = g_rfb_ctrl->frequency_set(protocol_mode[g_radio_protocol_mode].channel_frequency[g_channel_id]);
    g_radio_speed = protocol_mode[g_radio_protocol_mode].channel_datarate[g_channel_id];
    m_radio_last_status = g_rfb_ctrl->rx_config_set(g_radio_speed, 8, MOD_1, CRC_16, WHITEN_DISABLE, 0, true, protocol_mode[g_radio_protocol_mode].filter_type[g_channel_id]);
  }
  g_tx_beam = false;
  g_tx_done = true;
}

static void radio_rx_timeout(void)
{
  rx_timeout_count++;
  if (is_rx_flirs)
  {
    g_rfb_ctrl->zwave_beam_get(m_rx_last_beam_data, &m_rx_last_beam_rssi);
    if (0 == protocol_mode[g_radio_protocol_mode].use_fixed_rx_channel)
    {
      uint8_t scan_stage = g_rfb_ctrl->zwave_scan_stage_get();
      g_channel_id = convert_phy_scan_stage_to_phy_channel(g_radio_protocol_mode, scan_stage);
    }
    m_rx_last_beam_node_id = (ZWAVE_LR == protocol_mode[g_radio_protocol_mode].channel_datarate[g_channel_id]) ? ((m_rx_last_beam_data[1] & 0xF) << 8) | m_rx_last_beam_data[2] : m_rx_last_beam_data[1];
    if ((false == g_rx_broadcast_beam_enabled) && (m_rx_last_beam_node_id == BROADCAST_BEAM_NODE_ID || m_rx_last_beam_node_id == BROADCAST_BEAM_NODE_ID_LR))
    {
      // Ignore broadcast beam
      return;
    }
    m_rx_last_beam_channel = convert_phy_channel_to_zpal_channel(g_radio_protocol_mode, g_channel_id);
    g_radio.rx_cb(ZPAL_RADIO_EVENT_RX_TIMEOUT);
  }
  else if (0 != rx_current_beam_count)
  {
    rx_stop_beam_count_last = rx_current_beam_count;
    rx_current_beam_count = 0;
    // BEAM has been Received - time out to indicate Beam ended
    g_radio.rx_cb(ZPAL_RADIO_EVENT_RX_TIMEOUT);
  }
}

int8_t lr_tx_power_index_table[] = {-6, -2, 2, 6, 10, 13, 16, 19, 21, 23, 25, 26, 27, 28, 29, 30};

int8_t convert_tx_power_index_to_tx_power(uint8_t tx_power_index)
{
  return lr_tx_power_index_table[tx_power_index];
}

static void radio_rx_beam(void)
{
  rx_beam_count++;
  if (0 == rx_current_beam_count)
  {
    g_rfb_ctrl->zwave_beam_get(m_rx_last_beam_data, &m_rx_last_beam_rssi);
    if (0 == protocol_mode[g_radio_protocol_mode].use_fixed_rx_channel)
    {
      uint8_t scan_stage = g_rfb_ctrl->zwave_scan_stage_get();
      g_channel_id = convert_phy_scan_stage_to_phy_channel(g_radio_protocol_mode, scan_stage);
    }
    m_rx_last_beam_node_id = (ZWAVE_LR == protocol_mode[g_radio_protocol_mode].channel_datarate[g_channel_id]) ? ((m_rx_last_beam_data[1] & 0xF) << 8) | m_rx_last_beam_data[2] : m_rx_last_beam_data[1];
    if ((false == g_rx_broadcast_beam_enabled) && (m_rx_last_beam_node_id == BROADCAST_BEAM_NODE_ID || m_rx_last_beam_node_id == BROADCAST_BEAM_NODE_ID_LR))
    {
      // Ignore broadcast beam
      return;
    }
    m_rx_last_beam_channel = convert_phy_channel_to_zpal_channel(g_radio_protocol_mode, g_channel_id);
  }
  rx_current_beam_count++;
  g_radio.rx_cb(ZPAL_RADIO_EVENT_RX_BEAM_COMPLETE);
}

/**
 * @details   Return IC version information
 *             Bit0 ~ Bit15 is chip_revision
 *             Bit16 ~ Bit31 is chip_id
 */
void zwave_radio_chip_get_version(uint16_t *chip_id, uint16_t *chip_rev)
{
  uint32_t chip_id_rev = get_chip_version();
  *chip_id = (uint16_t)((chip_id_rev & 0xFFFF0000) >> 16);
  *chip_rev = (uint16_t)(chip_id_rev & 0xFFFF);
}

void radio_tx_init(void)
{
  /* Set channel frequency */
  m_radio_last_status = g_rfb_ctrl->frequency_set(protocol_mode[g_radio_protocol_mode].channel_frequency[g_channel_id]);

  /* Set TX config */
  m_radio_last_status = g_rfb_ctrl->tx_config_set(0, protocol_mode[g_radio_protocol_mode].channel_datarate[g_channel_id], 0, 0, 0, 0, 0);
}

void radio_rx_init(void)
{
  /* Set RX config */
  m_radio_last_status = g_rfb_ctrl->rx_config_set(protocol_mode[g_radio_protocol_mode].channel_datarate[g_channel_id], 8, MOD_1, CRC_16, WHITEN_DISABLE, 0, true, protocol_mode[g_radio_protocol_mode].filter_type[g_channel_id]);
}

bool radio_use_power_index_table_us_and_lr(uint8_t channel_id)
{
  return ((g_radio.region == REGION_US) || (g_radio.region == REGION_US_LR)) || ((g_radio.region == REGION_EU_LR) && ((2 < channel_id) || (g_radio.active_lr_channel_config == ZPAL_RADIO_LR_CH_CFG3)));
}

int8_t zwave_radio_min_tx_power_get(uint8_t channel_id)
{
  int8_t min_tx_power = -127;
  if (radio_use_power_index_table_us_and_lr(channel_id))
  {
    min_tx_power = radio_min_tx_power_US_dbm;
  }
  else
  {
    min_tx_power = radio_min_tx_power_dbm;
  }
  return min_tx_power;
}

int8_t zwave_radio_max_tx_power_get(uint8_t channel_id)
{
  int8_t max_tx_power = -127;
  if (radio_use_power_index_table_us_and_lr(channel_id))
  {
    max_tx_power = radio_max_tx_power_US_dbm;
  }
  else
  {
    max_tx_power = radio_max_tx_power_dbm;
  }
  return max_tx_power;
}

bool zpal_radio_tx_power_power_index_set(uint8_t channel_id, int8_t power, uint8_t power_index)
{
  bool value_updated = false;
  int8_t min_tx_power = zwave_radio_min_tx_power_get(channel_id);
  int8_t max_tx_power = zwave_radio_max_tx_power_get(channel_id);
  bool use_us_and_lr = radio_use_power_index_table_us_and_lr(channel_id);

  if ((min_tx_power <= power) && (max_tx_power >= power))
  {
    if (0 != power_index)
    {
      if (use_us_and_lr)
      {
        power_to_power_index_table[POWER_INDEX_TABLE_US_CHANNELS_AND_LR][power - min_tx_power] = power_index;
        power_to_power_index_table_US_and_LR_active = true;
      }
      else
      {
        power_to_power_index_table[POWER_INDEX_TABLE_ALL_REGION_CHANNELS_EXCEPT_US_CHANNELS_AND_LR][power - min_tx_power] = power_index;
        power_to_power_index_table_active = true;
      }
    }
    else
    {
      // Back to default
      if (use_us_and_lr)
      {
        power_to_power_index_table[POWER_INDEX_TABLE_US_CHANNELS_AND_LR][power - min_tx_power] = radio_rf_tx_power_conversion_US_p[power - min_tx_power];
      }
      else
      {
        power_to_power_index_table[POWER_INDEX_TABLE_ALL_REGION_CHANNELS_EXCEPT_US_CHANNELS_AND_LR][power - min_tx_power] = radio_rf_tx_power_conversion_p[power - min_tx_power];
      }
    }
    // Next time Tx Power needs to be set
    current_tx_power = -127;
    value_updated = true;
  }
  return value_updated;
}

bool zpal_radio_dBm_to_power_index_table_get(uint8_t channel_id, uint8_t const * *power_index_table)
{
  bool active = false;
  if (radio_use_power_index_table_us_and_lr(channel_id))
  {
    *power_index_table = (uint8_t const *)&power_to_power_index_table[POWER_INDEX_TABLE_US_CHANNELS_AND_LR][0];
    active = power_to_power_index_table_US_and_LR_active;
  }
  else
  {
    *power_index_table = (uint8_t const *)&power_to_power_index_table[POWER_INDEX_TABLE_ALL_REGION_CHANNELS_EXCEPT_US_CHANNELS_AND_LR][0];
    active = power_to_power_index_table_active;
  }
  return active;
}

int8_t radio_phy_tx_power_set(uint8_t channel_id, uint8_t band_type, int8_t tx_power)
{
  if (current_tx_power != tx_power)
  {
    uint8_t power_index;
    uint8_t tx_power_index;
    int8_t min_tx_power = zwave_radio_min_tx_power_get(channel_id);
    int8_t max_tx_power = zwave_radio_max_tx_power_get(channel_id);
    bool use_US = radio_use_power_index_table_us_and_lr(channel_id);
    if (min_tx_power > tx_power)
    {
      tx_power = min_tx_power;
    }
    else if (max_tx_power < tx_power)
    {
      tx_power = max_tx_power;
    }
    current_tx_power = tx_power;
    tx_power_index = tx_power - min_tx_power;
    if (use_US)
    {
      if (power_to_power_index_table_US_and_LR_active && (0 != power_to_power_index_table[POWER_INDEX_TABLE_US_CHANNELS_AND_LR][tx_power_index]))
      {
        power_index = power_to_power_index_table[POWER_INDEX_TABLE_US_CHANNELS_AND_LR][tx_power_index];
      }
      else
      {
        power_index = radio_rf_tx_power_conversion_US_p[tx_power_index];
      }
    }
    else
    {
      if (power_to_power_index_table_active && (0 != power_to_power_index_table[POWER_INDEX_TABLE_ALL_REGION_CHANNELS_EXCEPT_US_CHANNELS_AND_LR][tx_power_index]))
      {
        power_index = power_to_power_index_table[POWER_INDEX_TABLE_ALL_REGION_CHANNELS_EXCEPT_US_CHANNELS_AND_LR][tx_power_index];
      }
      else
      {
        power_index = radio_rf_tx_power_conversion_p[tx_power_index];
      }
    }
    rfb_port_tx_power_set_oqpsk(band_type, power_index);
  }
  return tx_power;
}

zpal_radio_zwave_channel_t zwave_radio_get_last_tx_channel(void)
{
  return m_radio_last_tx_channel;
}

void zwave_radio_rx_broadcast_beam_enable(bool enable)
{
  g_rx_broadcast_beam_enabled = enable;
}

bool zwave_radio_rx_broadcast_beam_is_enabled(void)
{
  return g_rx_broadcast_beam_enabled;
}

zpal_radio_zwave_channel_t zwave_radio_get_last_beam_channel(void)
{
  return m_rx_last_beam_channel;
}

int8_t zwave_radio_get_last_beam_tx_power(void)
{
  if (ZPAL_RADIO_ZWAVE_CHANNEL_4 >= zwave_radio_get_last_beam_channel())
  {
    uint8_t beamdata;
    enter_critical_section();
    beamdata = m_rx_last_beam_data[1];
    leave_critical_section();
    m_rx_last_beam_tx_power = convert_tx_power_index_to_tx_power((beamdata & 0xf0) >> 4);
  }
  return m_rx_last_beam_tx_power;
}

int8_t zwave_radio_get_last_beam_rssi(void)
{
  int8_t last_beam_rssi;
  uint8_t tmprssi;
  enter_critical_section();
  tmprssi = m_rx_last_beam_rssi;
  leave_critical_section();
  if (tmprssi < 128)
  {
    last_beam_rssi = (-1) * tmprssi;
  }
  else
  {
    last_beam_rssi = -127; // Invalid RSSI value
  }
  return last_beam_rssi;
}

node_id_t zwave_radio_get_last_beam_node_id(void)
{
  return m_rx_last_beam_node_id;
}

uint8_t zwave_radio_get_last_beam_data(uint8_t *last_beam_data,  __attribute__((unused)) uint8_t max_beam_data)
{
  uint8_t count = (ZPAL_RADIO_ZWAVE_CHANNEL_3 <= zwave_radio_get_last_beam_channel()) ? 4 : 3;
  enter_critical_section();
  memcpy(last_beam_data, m_rx_last_beam_data, count);
  leave_critical_section();
  return count;
}

uint32_t zwave_radio_get_beam_count(void)
{
  uint32_t return_val;
  enter_critical_section();
  return_val = rx_beam_count;
  leave_critical_section();
  return return_val;
}

uint32_t zwave_radio_get_current_beam_count(void)
{
  uint32_t return_val;
  enter_critical_section();
  return_val = rx_current_beam_count;
  leave_critical_section();
  return return_val;
}

uint32_t zwave_radio_stop_current_beam_count(void)
{
  uint32_t return_val;
  enter_critical_section();
  if (0 != rx_current_beam_count)
  {
    return_val = rx_current_beam_count;
    rx_stop_beam_count_last = return_val;
    rx_current_beam_count = 0;
  }
  else
  {
    return_val = rx_stop_beam_count_last;
  }
  leave_critical_section();
  return return_val;
}

uint32_t zwave_radio_get_last_stop_beam_count(void)
{
  return rx_stop_beam_count_last;
}

zwave_scan_region_t convert_zpal_region_to_t32cz20_scan_region(zpal_radio_region_t region)
{
  zwave_scan_region_t zwavescanregion;
  switch (region)
  {
    case REGION_KR:
      zwavescanregion = ZWAVE_SCANNING_KR;
      break;

    case REGION_JP:
      zwavescanregion = ZWAVE_SCANNING_JP;
      break;

    case REGION_CN:
      zwavescanregion = ZWAVE_SCANNING_CN;
      break;

    case REGION_RU:
      zwavescanregion = ZWAVE_SCANNING_RU;
      break;

    case REGION_IN:
      zwavescanregion = ZWAVE_SCANNING_IN;
      break;

    case REGION_IL:
      zwavescanregion = ZWAVE_SCANNING_IL;
      break;

    case REGION_HK:
      zwavescanregion = ZWAVE_SCANNING_HK;
      break;

    case REGION_ANZ:
      zwavescanregion = ZWAVE_SCANNING_ANZ;
      break;

    case REGION_EU:
    case REGION_EU_LR:
      zwavescanregion = ZWAVE_SCANNING_EU;
      break;

    case REGION_US:
    case REGION_US_LR:
      zwavescanregion = ZWAVE_SCANNING_US;
      break;

    default:
      zwavescanregion = ZWAVE_SCANNING_US;
      break;
  }
  return zwavescanregion;
}

static void radio_channel_scan_rssi_enable(bool enable)
{
  if (enable)
  {
    if (g_rfb_ctrl != NULL)
    {
      g_rfb_ctrl->zwave_scan_rssi_set(m_rssi_sample_frequency, m_rssi_sample_count_average);
    }
  }
  else
  {
    if (g_rfb_ctrl != NULL)
    {
      g_rfb_ctrl->zwave_scan_rssi_set(0, 0);
    }
  }
}

void zwave_radio_rssi_config_get(uint16_t *rssi_sample_frequency, uint8_t *rssi_sample_count)
{
  if ((NULL != rssi_sample_frequency) && (NULL != rssi_sample_count))
  {
    *rssi_sample_frequency = m_rssi_sample_frequency;
    *rssi_sample_count = m_rssi_sample_count_average;
  }
}

void zwave_radio_rssi_config_set(uint16_t rssi_sample_frequency, uint8_t rssi_sample_count)
{
  bool enable = false;
  if ((rssi_sample_frequency > 0) && (rssi_sample_count > 0))
  {
    m_rssi_sample_frequency = rssi_sample_frequency;
    m_rssi_sample_count_average = rssi_sample_count;
    enable = true;
  }
  else
  {
    m_rssi_sample_frequency = 0;
    m_rssi_sample_count_average = 0;
  }
  radio_channel_scan_rssi_enable(enable);
}

void zwave_radio_channel_scan_setup(bool enable)
{
  /* Setup setup rx channel scan */
  if (enable)
  {
    if (ZPAL_RADIO_PROTOCOL_MODE_3 == g_radio_protocol_mode)
    {
      if (0 != g_radio_region_setup.use_lr_backup_channel)
      {
        protocol_mode[g_radio_protocol_mode].channel_scan_mode = ZWAVE_SCANNING_NORMAL_LR2_R2_ENHANCE;
      }
      else
      {
        protocol_mode[g_radio_protocol_mode].channel_scan_mode = ZWAVE_SCANNING_NORMAL_R2_ENHANCE;
      }
    }
    m_radio_last_status = g_rfb_ctrl->zwave_scan_set(protocol_mode[g_radio_protocol_mode].channel_scan_mode, convert_zpal_region_to_t32cz20_scan_region(g_radio.region));
  }
  else
  {
    m_radio_last_status = g_rfb_ctrl->zwave_scan_set(ZWAVE_SCANNING_DISABLE, convert_zpal_region_to_t32cz20_scan_region(g_radio.region));
  }
  if (ZPAL_RADIO_MODE_FLIRS != g_radio_mode)
  {
    // If channel scan is enabled then set RSSI sampling rate to
    // every m_rssi_sample_frequency Rx scan cycle and create running RSSI average of m_rssi_sample_count RSSI samples
    // If channel scan is disabled then disable RSSI sampling
    radio_channel_scan_rssi_enable(enable);
  }
}

void zwave_radio_power_conversion_20dbm(bool set_20dbm_conversion)
{
  g_radio_tx_max_power_20dbm = set_20dbm_conversion;
  uint32_t table_len;
  uint32_t table_len_US;
  if (g_radio_tx_max_power_20dbm)
  {
    radio_min_tx_power_dbm = (-OFFSET_TX_POWER_CONVERSION_0dBm_20dBm);
    radio_max_tx_power_dbm = MAX_TX_POWER_CONVERSION_20dBm;
    radio_rf_tx_power_conversion_p = radio_rf_tx_power_conversion_20dBm;
    table_len = sizeof(radio_rf_tx_power_conversion_20dBm);

    radio_min_tx_power_US_dbm = (-OFFSET_TX_POWER_CONVERSION_0dBm_US_20dBm);
    radio_max_tx_power_US_dbm = MAX_TX_POWER_CONVERSION_US_20dBm;
    radio_rf_tx_power_conversion_US_p = radio_rf_tx_power_conversion_US_20dBm;
    table_len_US = sizeof(radio_rf_tx_power_conversion_US_20dBm);
  }
  else
  {
    radio_min_tx_power_dbm = (-OFFSET_TX_POWER_CONVERSION_0dBm_14dBm);
    radio_max_tx_power_dbm = MAX_TX_POWER_CONVERSION_14dBm;
    radio_rf_tx_power_conversion_p = radio_rf_tx_power_conversion_14dBm;
    table_len = sizeof(radio_rf_tx_power_conversion_14dBm);

    radio_min_tx_power_US_dbm = (-OFFSET_TX_POWER_CONVERSION_0dBm_US_14dBm);
    radio_max_tx_power_US_dbm = MAX_TX_POWER_CONVERSION_US_14dBm;
    radio_rf_tx_power_conversion_US_p = radio_rf_tx_power_conversion_US_14dBm;
    table_len_US = sizeof(radio_rf_tx_power_conversion_US_14dBm);
  }
  memcpy(power_to_power_index_table[POWER_INDEX_TABLE_ALL_REGION_CHANNELS_EXCEPT_US_CHANNELS_AND_LR], radio_rf_tx_power_conversion_p, table_len);
  power_to_power_index_table_active = false;
  memcpy(power_to_power_index_table[POWER_INDEX_TABLE_US_CHANNELS_AND_LR], radio_rf_tx_power_conversion_US_p, table_len_US);
  power_to_power_index_table_US_and_LR_active = false;
  // Next time Tx Power needs to be set
  current_tx_power = -127;
}

static bool first_radio_init = true;
static uint32_t m_filter_home_id = 0;
static uint16_t m_filter_node_id = 0;
static uint8_t m_filter_home_id_hash = 0x55;


void zwave_radio_initialize(void)
{
  uint8_t idx;

  /* PHY PIB Parameters */
  g_phy_cca_threshold = abs(g_radio.listen_before_talk_threshold);
  for (idx = 0; idx < MAX_TOTAL_CHANNEL_NUM; idx++)
  {
    zwave_radio_set_lbt_level(idx, g_radio.listen_before_talk_threshold);
  }

  /* Register rfb interrupt event */
  struct_rfb_interrupt_event.tx_done = radio_tx_done;
  struct_rfb_interrupt_event.rx_done = radio_rx_done;
  struct_rfb_interrupt_event.rx_timeout = radio_rx_timeout;
  struct_rfb_interrupt_event.rx_beam = radio_rx_beam;

  /* Init rfb */
  g_rfb_ctrl = rfb_subg_init();
#if (GPIO_RF_DEBUG_ENABLED == 1)
  radio_gpio_rf_debug(true);
#endif
  if (g_radio_init_cb != NULL)
  {
    g_rfb_ctrl->init_cb_register(g_radio_init_cb);
  }

  protocol_mode[g_radio_protocol_mode].band_type = BAND_SUBG_915M;

  if (FREQ_900MHZ > g_radio_region_setup.channel_frequency_khz[0])
  {
    protocol_mode[g_radio_protocol_mode].band_type = BAND_SUBG_868M;
  }
  m_radio_last_status = g_rfb_ctrl->init(&struct_rfb_interrupt_event, RFB_KEYING_ZWAVE, protocol_mode[g_radio_protocol_mode].band_type);

  m_radio_last_status = g_rfb_ctrl->phy_pib_set(TX_RX_TURNAROUND_TIME, ENERGY_DETECTION_OR_CHARRIER_SENSING, g_phy_cca_threshold, g_radio_region_setup.LBTRssiTime);
  if (first_radio_init)
  {
    first_radio_init = false;
    m_radio_last_status = g_rfb_ctrl->zwave_id_filter_set(m_filter_home_id, m_filter_node_id, m_filter_home_id_hash);
    m_radio_network_filter_enabled = false;
  }

  zwave_radio_channel_scan_setup(0 == protocol_mode[g_radio_protocol_mode].use_fixed_rx_channel);

  radio_tx_init();
  radio_rx_init();
}

/**************************************************************************************************
 *    GLOBAL FUNCTIONS
 *************************************************************************************************/

bool zwave_radio_network_filter_enable(bool enable)
{
  if ((g_home_id == 0)
      && (g_node_id == 0))
  {
    enable = false;
    g_home_id_hash = 0x55;
  }
  if (enable)
  {
    if ((true == m_radio_network_filter_enabled)
        && (m_filter_home_id == g_home_id)
        && (m_filter_node_id == g_node_id)
        && (m_filter_home_id_hash == g_home_id_hash)) {
      return true;
    }
    m_filter_home_id = g_home_id;
    m_filter_node_id = g_node_id;
    m_filter_home_id_hash = g_home_id_hash;
  } else {
    if (false == m_radio_network_filter_enabled) {
      return false;
    }
    // Disable network id filter - assuming it do not matter that preamble are different in classic and LR
    m_filter_home_id = 0;
    m_filter_node_id = 0;
    m_filter_home_id_hash = 0x55;
  }
  m_radio_last_status = g_rfb_ctrl->zwave_id_filter_set(m_filter_home_id, m_filter_node_id, m_filter_home_id_hash);
  m_radio_network_filter_enabled = enable;
  return enable;
}

bool zwave_radio_network_id_filter_enabled_get(void)
{
  return m_radio_network_filter_enabled;
}

void zwave_radio_network_id_filter_set(bool enable)
{
  // Only enable network id filter if in FLiRS mode and node id is set
  // Currently in all other cases network id filter is disabled
  if ((ZPAL_RADIO_MODE_FLIRS != g_radio_mode) || (0 == g_node_id)) {
    enable = false;
  }
  zwave_radio_network_filter_enable(enable);
}

void zwave_radio_get_network_ids(uint32_t *home_id, node_id_t *node_id, zpal_radio_mode_t *mode, uint8_t *home_id_hash)
{
  *node_id = g_node_id;
  *home_id = g_home_id;
  *mode = g_radio_mode;
  *home_id_hash = g_home_id_hash;
}

void zwave_radio_set_network_ids(uint32_t home_id, node_id_t node_id, zpal_radio_mode_t mode, uint8_t home_id_hash)
{
  bool network_id_changed = (g_node_id != node_id) || (g_home_id != home_id) || (g_radio_mode != mode) || (g_home_id_hash != home_id_hash);
  if ((zwave_radio_network_id_filter_enabled_get() == false) && (network_id_changed == false))
  {
    return;
  }
  g_node_id = node_id;
  g_home_id = home_id;
  g_radio_mode = mode;
  g_home_id_hash = home_id_hash;
  zwave_radio_network_id_filter_set(true);
}

static void radio_dio_rf_dbg_set(void)
{
  uint32_t reg_val;
  uint16_t reg_addr;

#if (defined(RT584_SHUTTLE_IC) && (RT584_SHUTTLE_IC == 1))
  reg_addr = 0x043c;
#else
  reg_addr = 0x04c4;
#endif
  reg_val = RfMcu_RegGet(reg_addr);
  reg_val &= 0xFF00FFFF;
  reg_val |= 0x002C0000;
  RfMcu_RegSet(reg_addr, reg_val);
}

static void radio_config(void)
{
  // Read radio calibration from MP page
  MPK_RF_TRIM_T rf_mp_cal_val;
  uint32_t reg_val;
  uint16_t reg_addr;

  if (MpCalRftrimRead(MP_ID_RF_TRIM_584_SUBG0, MP_CNT_RFTRIM, (uint8_t *) &rf_mp_cal_val)== STATUS_SUCCESS)
  {
    reg_addr = 0x308;
    reg_val = RfMcu_RegGet(reg_addr);
    reg_val &= 0xFFFF0000;
    reg_val |= (((uint16_t)rf_mp_cal_val.rx_iq_gain & 0x7f)      << 0);
    reg_val |= (((uint16_t)rf_mp_cal_val.rx_iq_gain_sel & 0x01)  << 7);
    reg_val |= (((uint16_t)rf_mp_cal_val.rx_iq_phase & 0x7f)     << 8);
    reg_val |= (((uint16_t)rf_mp_cal_val.rx_iq_phase_sel & 0x01) << 15);
    RfMcu_RegSet(reg_addr, reg_val);
  }
}

static void radio_init_cb(void)
{
  radio_config();
  if (NULL != g_radio_dio_rf_debug_cb)
  {
    g_radio_dio_rf_debug_cb();
  }
}

static void set_gpio_debug_mode(uint32_t pin, uint32_t mode)
{
  tr_hal_gpio_pin_t _pin = {.pin = pin};
  tr_hal_gpio_set_mode(_pin, mode);
  tr_hal_gpio_set_pull_mode(_pin,  TR_HAL_PULLOPT_PULL_NONE);
  tr_hal_gpio_set_direction(_pin,  TR_HAL_GPIO_DIRECTION_OUTPUT);
}

static void radio_gpio_rf_debug(bool rf_state_enable)
{
  if (rf_state_enable)
  {
    /* MCU P0 DEBUG */
#if (CHIP_MODEL == CHIP_ID(RT584, RT58X_MPB)) || (CHIP_MODEL == CHIP_ID(RT584, RT58X_MPA))
    set_gpio_debug_mode(GPIO_RF_DEBUG_RX, TR_HAL_GPIO_MODE_DBGB);
    set_gpio_debug_mode(GPIO_RF_DEBUG_TX, TR_HAL_GPIO_MODE_DBGC);
    set_gpio_debug_mode(GPIO_RF_DEBUG_CPU_WAKE, TR_HAL_GPIO_MODE_DBG0);
#else
    SYSCTRL->GPIO_MAP0 = 0x77777777;
    /**
     * GPIO_RF_DBG_CPU_WAKE as rf cpu state (  1: cpu awake,  0: sleep),
     */
    Pin_Set_Mode(GPIO_RF_DEBUG_CPU_WAKE, 7);

    /**
     * GPIO_RF_DBG_RX       as rf rx        (  1: rx on,      0: rx off),
     * GPIO_RF_DBG_TX       as rf tx        (  1: tx on,      0: tx off)
     */
    Pin_Set_Mode(GPIO_RF_DEBUG_RX, 7);
    Pin_Set_Mode(GPIO_RF_DEBUG_TX, 7);
#endif
    SYSCTRL->SYS_TEST.bit.DBG_OUT_SEL = 27;
    g_radio_dio_rf_debug_cb = radio_dio_rf_dbg_set;
  }
  else
  {
    g_radio_dio_rf_debug_cb = NULL;
  }
}

bool zwave_radio_tx_power_config(zpal_tx_power_t tx_power_max)
{
  bool configs_setting_change_status = false;
  // Get current Default setting
  txpower_default_cfg_t txpowerdefaultsetting = Sys_TXPower_GetDefault();
  txpower_default_cfg_t txpowersetting = TX_POWER_14DBM_DEF;
  if (ZW_TX_POWER_14DBM < tx_power_max)
  {
    txpowersetting = TX_POWER_20DBM_DEF;
  }
  if (txpowersetting != txpowerdefaultsetting)
  {
    // New setting
    configs_setting_change_status = (STATUS_SUCCESS == MpSectorWriteTxPwrCfg(txpowersetting));
  }
  return configs_setting_change_status;
}

bool zwave_radio_is_hw_setup_for_20dbm(void)
{
  return (TX_POWER_20DBM_DEF == Sys_TXPower_GetDefault());
}

zpal_tx_power_t zwave_radio_classic_tx_power_set(zpal_tx_power_t tx_power)
{
  zpal_tx_power_t tx_power_set = tx_power;
  if (zwave_radio_is_hw_setup_for_20dbm())
  {
    if (ZW_TX_POWER_20DBM < tx_power_set)
    {
      tx_power_set = ZW_TX_POWER_20DBM;
    }
  }
  else if (ZW_TX_POWER_14DBM < tx_power_set)
  {
    tx_power_set = ZW_TX_POWER_14DBM;
  }
  g_radio.tx_power_max = tx_power_set;
  return tx_power_set;
}

zpal_tx_power_t zwave_radio_classic_tx_power_get(void)
{
  return g_radio.tx_power_max;
}

void zwave_radio_init(zpal_radio_profile_t *const profile, const zw_region_rf_setup_t *p_current_region_setup)
{
  // Set CommSubsystem Interrupt priority
  NVIC_SetPriority(CommSubsystem_IRQn, 0x01);

  g_radio = *profile;
  g_radio_region_setup = *p_current_region_setup;
  g_radio_protocol_mode = (g_radio_region_setup.protocolMode < PROTOCOL_MODE_COUNT) ? g_radio_region_setup.protocolMode : 0;
  zwave_radio_power_conversion_20dbm((g_radio.tx_power_max_lr > ZW_TX_POWER_14DBM));
  if (zwave_radio_tx_power_config(g_radio.tx_power_max_lr))
  {
    DPRINT("\n** There is a discrepancy between Application Max Tx Power setting and current Max Tx Power configuration, Please Reset module to rectify issue **\n");
  }
  if (ZPAL_RADIO_RSSI_NOT_AVAILABLE == g_radio.listen_before_talk_threshold)
  {
    g_radio.listen_before_talk_threshold = g_radio_region_setup.LBTRssiLevel;
  }
  protocol_mode[g_radio_protocol_mode].channel_frequency[0] = g_radio_region_setup.channel_frequency_khz[0];
  protocol_mode[g_radio_protocol_mode].channel_frequency[1] = g_radio_region_setup.channel_frequency_khz[1];
  protocol_mode[g_radio_protocol_mode].channel_frequency[2] = g_radio_region_setup.channel_frequency_khz[2];
  protocol_mode[g_radio_protocol_mode].channel_frequency[3] = g_radio_region_setup.channel_frequency_khz[3];
  protocol_mode[g_radio_protocol_mode].use_fixed_rx_channel = g_radio_region_setup.use_fixed_rx_channel;
  protocol_mode[g_radio_protocol_mode].fixed_rx_channel = g_radio_region_setup.fixed_rx_channel;
  g_channel_id = 0;
  if (0 != protocol_mode[g_radio_protocol_mode].use_fixed_rx_channel)
  {
    g_channel_id = protocol_mode[g_radio_protocol_mode].fixed_rx_channel;
  }
  g_radio_speed = protocol_mode[g_radio_protocol_mode].channel_datarate[g_channel_id];
  g_radio_init_cb = radio_init_cb;
  memset(transmit_time_channel, 0, sizeof(transmit_time_channel));
  /* Init radio */
  zwave_radio_initialize();
  zwave_radio_idle();
}

void zwave_radio_set_rx_fixed_channel(uint8_t enable_fixed_channel, uint8_t channel)
{
  protocol_mode[g_radio_protocol_mode].use_fixed_rx_channel = enable_fixed_channel;
  protocol_mode[g_radio_protocol_mode].fixed_rx_channel = channel;
  g_channel_id = 0;
  if (0 != protocol_mode[g_radio_protocol_mode].use_fixed_rx_channel)
  {
    g_channel_id = protocol_mode[g_radio_protocol_mode].fixed_rx_channel;
  }
  g_radio_speed = protocol_mode[g_radio_protocol_mode].channel_datarate[g_channel_id];
  zwave_radio_idle();
  zwave_radio_initialize();
  zwave_radio_start_receive(false);
}

uint8_t convert_phy_channel_to_phy_scan_stage(zpal_radio_protocol_mode_t protocolmode, uint8_t phy_channel)
{
  uint8_t scan_stage = 0;
  switch(protocolmode)
  {
    case ZPAL_RADIO_PROTOCOL_MODE_4:
      scan_stage = ZWAVE_SCANNING_LR;
      if (phy_channel != 0)
      {
        scan_stage = ZWAVE_SCANNING_SWITCH_LR;
      }
      break;

    case ZPAL_RADIO_PROTOCOL_MODE_3:
      scan_stage = phy_channel;
      if ((3 == phy_channel) && (ZWAVE_SCANNING_NORMAL_LR2_R2_ENHANCE == protocol_mode[g_radio_protocol_mode].channel_scan_mode))
      {
        scan_stage = ZWAVE_SCANNING_SWITCH_LR;
      }
      break;

    case ZPAL_RADIO_PROTOCOL_MODE_2:
      if (phy_channel != 0)
      {
        if (phy_channel == 2)
        {
          scan_stage = ZWAVE_SCANNING_R3;
        }
        else
        {
          scan_stage = ZWAVE_SCANNING_SWITCH_R3_2;
        }
      }
      else
      {
        scan_stage = ZWAVE_SCANNING_SWITCH_R3_3;
      }
      break;

    case ZPAL_RADIO_PROTOCOL_MODE_1:
    default:
      scan_stage = phy_channel;
      break;
  }
  return scan_stage;
}

uint8_t convert_zpal_channel_to_phy_channel(zpal_radio_protocol_mode_t protocol_mode, zpal_radio_zwave_channel_t zpal_channel)
{
  uint8_t channel = 0;
  switch(protocol_mode)
  {
    case ZPAL_RADIO_PROTOCOL_MODE_4:
      if ((zpal_channel >= 3) && (zpal_channel <= 4))
      {
        channel = zpal_channel - 3;
      }
      else
      {
        return zpal_channel;
      }
      break;

    case ZPAL_RADIO_PROTOCOL_MODE_1:
    case ZPAL_RADIO_PROTOCOL_MODE_2:
    case ZPAL_RADIO_PROTOCOL_MODE_3:
    default:
      channel = zpal_channel;
      if (channel == 4)
      {
        channel = 3;
      }
      break;
  }
  return channel;
}

uint8_t convert_phy_scan_stage_to_phy_channel(zpal_radio_protocol_mode_t protocol_mode, uint8_t scan_stage)
{
  uint8_t phy_channel = 0;
  switch (protocol_mode)
  {
    case ZPAL_RADIO_PROTOCOL_MODE_4:
      phy_channel = 0;
      if ((ZWAVE_SCANNING_LR == scan_stage) || (ZWAVE_SCANNING_SWITCH_LR == scan_stage))
      {
        phy_channel = scan_stage - ZWAVE_SCANNING_LR;
      }
      break;

    case ZPAL_RADIO_PROTOCOL_MODE_3:
      phy_channel = scan_stage;
      if (ZWAVE_SCANNING_SWITCH_LR == scan_stage)
      {
        phy_channel = ZWAVE_SCANNING_LR;
      }
      break;

    case ZPAL_RADIO_PROTOCOL_MODE_2:
      phy_channel = 2;  // ZWAVE_SCANNING_R3 is channel 2
      if (scan_stage == ZWAVE_SCANNING_SWITCH_R3_2)
      {
        phy_channel = 1;
      }
      else if (scan_stage == ZWAVE_SCANNING_SWITCH_R3_3)
      {
        phy_channel = 0;
      }
      break;

    case ZPAL_RADIO_PROTOCOL_MODE_1:
    default:
      phy_channel = scan_stage;
      break;
  }
  return phy_channel;
}

zpal_radio_zwave_channel_t convert_phy_channel_to_zpal_channel(zpal_radio_protocol_mode_t protocol_mode, uint8_t channel)
{
  zpal_radio_zwave_channel_t zpal_channel = 0;
  switch (protocol_mode)
  {
    case ZPAL_RADIO_PROTOCOL_MODE_4:
      if (channel < 2)
      {
        zpal_channel = channel + 3;
      }
      else
      {
        zpal_channel = 3;
      }
      break;

    case ZPAL_RADIO_PROTOCOL_MODE_1:
    case ZPAL_RADIO_PROTOCOL_MODE_2:
    case ZPAL_RADIO_PROTOCOL_MODE_3:
    default:
      zpal_channel = channel;
      break;
  }
  return zpal_channel;
}

uint32_t radio_tx_time_channel(uint8_t phy_channel, uint16_t frame_length, uint8_t preamble_length)
{
  uint32_t frame_time_ms;
  // Add preambles and SOF to frame length
  uint32_t total_frame_length = preamble_length + 1 + frame_length;
  // Calculate frame transmission time - add 1 for DIV operation
  switch (protocol_mode[g_radio_protocol_mode].channel_datarate[phy_channel])
  {
    case ZWAVE_9P6K:
      frame_time_ms = (((total_frame_length * 8) * 100) / 96) + 1;
      break;

    case ZWAVE_40K:
      frame_time_ms = (((total_frame_length * 8) * 100) / 400) + 1;
      break;

    default:
      // 100kbit
      frame_time_ms = (((total_frame_length * 8) * 100) / 1000) + 1;
      break;
  }
  return frame_time_ms;
}

void radio_update_transmit_time_phy_channel(uint8_t phy_channel)
{
  // If tx_time has NOT been set then nothing to update
  if (transmit_time_channel[phy_channel].tx_time)
  {
    TickType_t currentTickTime = xTaskGetTickCount();
    if (!transmit_time_channel[phy_channel].start_tx_time)
    {
      // New transmit window set start_tx_time
      transmit_time_channel[phy_channel].start_tx_time = currentTickTime;
    }
    transmit_time_channel[phy_channel].start_channel_lock_time = currentTickTime + transmit_time_channel[phy_channel].tx_time;
    transmit_time_channel[phy_channel].tx_time = 0;
  }
}

/**
 * Check if LBTChannelLockTime has expired and reset transmit window if so
 *
 * return true if LBTChannelLockTime has expired and transmit window has been reset. Or if channel_lock is NOT active
 * return false if LBTChannelLockTime has NOT expired and channel_lock is active
 */
bool update_channel_lock_time(TickType_t currentTickTime, uint8_t phy_channel)
{
  bool return_value;
  // If channel_lock is active LBTChannelLockTime must expire for transmit to be allowed
  return_value = transmit_time_channel[phy_channel].channel_lock ? false : true;
  if (g_radio_region_setup.LBTChannelLockTime < currentTickTime - transmit_time_channel[phy_channel].start_channel_lock_time)
  {
    // If LBTChannelLockTime has passed since last transmit then reset current transmit window - allowing pending transmit
    transmit_time_channel[phy_channel].start_tx_time = 0;
    transmit_time_channel[phy_channel].start_channel_lock_time = 0;
    transmit_time_channel[phy_channel].channel_lock = false;
    return_value = true;
  }
  return return_value;
}

bool zwave_radio_is_transmit_allowed(uint8_t channel, uint8_t frame_length, __attribute__((unused)) uint8_t frame_priority)
{
  bool return_value = true;
  if (!g_radio_region_setup.LBTChannelLockTime || !g_radio_region_setup.LBTChannelTxTime)
  {
    // If no rule exist for maximum tx time or lock time on a channel just return true
    return return_value;
  }
  uint8_t phy_channel = convert_zpal_channel_to_phy_channel(g_radio_protocol_mode, channel);
  uint32_t transmit_frame_time_ms = radio_tx_time_channel(phy_channel, frame_length, protocol_mode[g_radio_protocol_mode].preamble_len[phy_channel]);
  TickType_t currentTickTime = xTaskGetTickCount();
  //
  return_value = update_channel_lock_time(currentTickTime, phy_channel);
  if (return_value)
  {
    if (g_radio_region_setup.LBTChannelTxTime > ((currentTickTime + transmit_frame_time_ms) - transmit_time_channel[phy_channel].start_tx_time))
    {
      // Current transmit fits into current transmit window so update start_lock_time to reflect that
      transmit_time_channel[phy_channel].start_channel_lock_time = 0;
      transmit_time_channel[phy_channel].tx_time = transmit_frame_time_ms;
    }
    else
    {
      if (!transmit_time_channel[phy_channel].start_tx_time)
      {
        // If start_tx_time is zero then this is 'first' transmit in new transmit window
        transmit_time_channel[phy_channel].start_channel_lock_time = 0;
        transmit_time_channel[phy_channel].tx_time = transmit_frame_time_ms;
      }
      else
      {
        // LBTChannelTxTime has passed since 'first' transmit, so now channel needs to be locked for LBTChannelLockTime
        transmit_time_channel[phy_channel].channel_lock = true;
        return_value = false;
      }
    }
  }
  return return_value;
}

void radio_network_statistics_calculate_next_tx_time(zpal_radio_transmit_parameter_t const *const tx_parameters, uint8_t frame_length)
{
  next_tx.channel = (*tx_parameters).channel_id;
  if ((*tx_parameters).crc != ZPAL_RADIO_CRC_NONE)
  {
    // Add checksum byte/s
    frame_length += ((*tx_parameters).crc == ZPAL_RADIO_CRC_16_BIT_CCITT) ? 2 : 1;
  }
  // calculate frame transmission time - including preamble, SOF, frame and crc if any
  next_tx.time = radio_tx_time_channel(convert_zpal_channel_to_phy_channel(g_radio_protocol_mode, next_tx.channel), frame_length, (*tx_parameters).preamble_length);
  if ((*tx_parameters).repeats)
  {
    next_tx.time *= (*tx_parameters).repeats;
  }
  next_tx.transmit_time_start = xTaskGetTickCount();
}

void radio_network_statistics_update_tx_time(void)
{
  switch (next_tx.channel)
  {
    case ZPAL_RADIO_ZWAVE_CHANNEL_0:
      (*g_radio.network_stats).tx_time_channel_0 += next_tx.time;
      break;

    case ZPAL_RADIO_ZWAVE_CHANNEL_1:
      (*g_radio.network_stats).tx_time_channel_1 += next_tx.time;
      break;

    case ZPAL_RADIO_ZWAVE_CHANNEL_2:
      (*g_radio.network_stats).tx_time_channel_2 += next_tx.time;
      break;

    case ZPAL_RADIO_ZWAVE_CHANNEL_3:
      (*g_radio.network_stats).tx_time_channel_3 += next_tx.time;
      break;

    case ZPAL_RADIO_ZWAVE_CHANNEL_4:
      (*g_radio.network_stats).tx_time_channel_4 += next_tx.time;
      break;

    default:
      break;
  }
}

#ifdef DEBUGPRINT
#define DEBUG_DUMP_FRAME

#ifdef DEBUG_DUMP_FRAME
static void radio_generate_string_data(char * const p_string, uint16_t max_len, uint8_t const * data, uint16_t data_length)
{
  uint16_t str_len = 0;
  if (NULL != p_string)
  {
    for (int i = 0; (i < max_len); i++)
    {
      if  ('\0' == p_string[i])
      {
        str_len = i;
        break;
      }
    }
    // Make sure we only write as much as there is room for
    if (data_length > ((max_len - str_len) / 2))
    {
      data_length = (max_len - str_len) / 2;
    }
    for (int j = 0; j < data_length; j++)
    {
      snprintf(p_string + str_len, max_len - str_len, "%02X", data[j]);
      str_len += 2;
    }
  }
}
#endif
#endif

void radio_setup_to_transmit(uint8_t channel_id, __attribute__((unused)) uint8_t preamble_length, int8_t tx_power)
{
  /* check previous tx done */
  while (!g_tx_done)
    ;
  g_tx_done = false;
  if (0 == protocol_mode[g_radio_protocol_mode].use_fixed_rx_channel)
  {
    if (true == is_rx_start)
    {
      g_rfb_ctrl->auto_state_set(false);
    }
  }
  /* check channel */
  g_channel_id = channel_id;
  g_rfb_ctrl->frequency_set(protocol_mode[g_radio_protocol_mode].channel_frequency[g_channel_id]);
  g_radio_speed = protocol_mode[g_radio_protocol_mode].channel_datarate[g_channel_id];
  g_rfb_ctrl->tx_config_set(0, g_radio_speed, 0, 0, 0, 0, 0);
  radio_phy_tx_power_set(g_channel_id, protocol_mode[g_radio_protocol_mode].band_type, tx_power);
}

void radio_do_transmit(bool use_lbt, uint16_t total_payload_length, uint8_t tx_control)
{
  // Is lbt requested
  if (use_lbt)
  {
    if (g_phy_cca_threshold != g_cca_threshold[g_channel_id])
    {
      g_phy_cca_threshold = g_cca_threshold[g_channel_id];
      g_rfb_ctrl->phy_pib_set(TX_RX_TURNAROUND_TIME, ENERGY_DETECTION_OR_CHARRIER_SENSING, g_phy_cca_threshold, g_radio_region_setup.LBTRssiTime);
    }
    // Use Average rssi when doing lbt
    tx_control |= ((NONBEACON_MODE_CSMACA << 1) & 0x02) | ((AVERAGE_RSSI << 4) & 0x10);
  }
  if (is_rx_flirs && (ZPAL_RADIO_PROTOCOL_MODE_1 == g_radio_protocol_mode))
  {
    is_rx_start = false;
  }
  else
  {
    is_rx_flirs = false;
    // Return to Rx after transmit
    tx_control |= ((AUTO_T2R_ENABLE << 5) & 0x20);
    is_rx_start = true;
  }
  g_rfb_ctrl->data_send(g_tx_buf, total_payload_length, tx_control, 11);
}

zpal_status_t zwave_radio_transmit(zpal_radio_transmit_parameter_t const *const tx_parameters,
                                uint8_t frame_header_length,
                                uint8_t const *const frame_header_buffer,
                                uint8_t frame_payload_length,
                                uint8_t const *const frame_payload_buffer,
                                uint8_t use_lbt,
                                int8_t tx_power)
{
  // Assume no lbt needed

  uint8_t tx_control = 0;
  uint16_t total_payload_length = frame_header_length + frame_payload_length;
  uint8_t phy_channel = convert_zpal_channel_to_phy_channel(g_radio_protocol_mode, (*tx_parameters).channel_id);
  radio_setup_to_transmit(phy_channel, (*tx_parameters).preamble_length, tx_power);

  g_tx_beam = false;
  memcpy(&g_tx_buf[0], &frame_header_buffer[0], frame_header_length);
  memcpy(&g_tx_buf[frame_header_length], &frame_payload_buffer[0], frame_payload_length);
#ifdef DEBUGPRINT
  snprintf(str_buf, sizeof(str_buf), "TX(%02X,%i,%02X) ", total_payload_length, tx_power, (*tx_parameters).channel_id);
#ifdef DEBUG_DUMP_FRAME
  radio_generate_string_data(str_buf, sizeof(str_buf), g_tx_buf, total_payload_length);
#endif
  DPRINTF("%s\n", str_buf);
#endif
  radio_rf_channel_statistic_tx_channel_set((*tx_parameters).channel_id);
  radio_do_transmit(use_lbt, total_payload_length, tx_control);
  radio_network_statistics_calculate_next_tx_time(tx_parameters, total_payload_length);
  return ZPAL_STATUS_OK;
}

zpal_status_t zwave_radio_transmit_beam(zpal_radio_transmit_parameter_t const *const tx_parameters,
                                     uint8_t beam_data_len,
                                     uint8_t const *const beam_data,
                                     int8_t tx_power)
{
  // Transmit BEAM
  uint8_t tx_control = 0x08;
  uint16_t total_payload_length = beam_data_len + 2;
  uint8_t phy_channel = convert_zpal_channel_to_phy_channel(g_radio_protocol_mode, (*tx_parameters).channel_id);
  radio_setup_to_transmit(phy_channel, (*tx_parameters).preamble_length, tx_power);
  g_tx_beam = true;

  uint8_t beamframeindex = 0;
  g_tx_buf[beamframeindex++] = beam_data_len - 1;
  g_tx_buf[beamframeindex++] = (*tx_parameters).repeats;

  memcpy(&g_tx_buf[beamframeindex], &beam_data[0], beam_data_len);

#ifdef DEBUGPRINT
  snprintf(str_buf, sizeof(str_buf), "TXB(%02X,%d,%02X,%04X) ", total_payload_length, tx_power, (*tx_parameters).channel_id, (*tx_parameters).repeats);
#ifdef DEBUG_DUMP_FRAME
  radio_generate_string_data(str_buf, sizeof(str_buf), g_tx_buf, total_payload_length);
#endif
  DPRINTF("%s\n", str_buf);
#endif
  // Do not use lbt if region is setup to not use it...
  radio_do_transmit((0 < g_radio_region_setup.LBTRssiTime), total_payload_length, tx_control);
  radio_network_statistics_calculate_next_tx_time(tx_parameters, beam_data_len);

  return ZPAL_STATUS_OK;
}

bool zwave_radio_is_receive_started(void)
{
  return is_rx_start;
}

void zwave_radio_start_receive(bool force_rx)
{
  if (!is_rx_start || force_rx)
  {
    g_rfb_ctrl->auto_state_set(true);
  }
  is_rx_start = true;
}

void zwave_radio_idle(void)
{
  if (is_rx_start)
  {
    g_rfb_ctrl->auto_state_set(false);
    is_rx_start = false;
  }
}


void zwave_radio_forced_idle(void)
{
  if (RFB_EVENT_SUCCESS != g_rfb_ctrl->auto_state_set(false))
  {
    ASSERT(0);
    zpal_reboot_with_info(0x462, ZPAL_RESET_UNHANDLED_RADIO_EVENT);
  }
  is_rx_start = false;
}

int8_t zwave_radio_rssi_read(uint8_t channel, bool *is_rssi_valid)
{
  int8_t rssi_value = ZPAL_RADIO_RSSI_NOT_AVAILABLE;
  do
  {
    // If we are in FLIRS mode and the region is not LR then skip the RSSI reading.
    if ((ZPAL_RADIO_MODE_FLIRS == g_radio_mode) && (ZPAL_RADIO_PROTOCOL_MODE_4 != g_radio_protocol_mode))
    {
      *is_rssi_valid = false;
      break;
    }
    // Perform the RSSI reading
    uint8_t rssi_read;
    uint8_t phy_channel = convert_zpal_channel_to_phy_channel(g_radio_protocol_mode, channel);
    if (0 == protocol_mode[g_radio_protocol_mode].use_fixed_rx_channel)
    {
      bool read_rssi_average = ((ZPAL_RADIO_MODE_NON_LISTENING != g_radio_mode) && ((m_rssi_sample_frequency != 0) && (m_rssi_sample_count_average != 0))) ? true : false;
      uint8_t scanstage = convert_phy_channel_to_phy_scan_stage(g_radio_protocol_mode, phy_channel);
      rssi_read = g_rfb_ctrl->zwave_scan_rssi_read(scanstage, is_rssi_valid, read_rssi_average);
    }
    else
    {
      bool change_channel = 0;
      if (g_channel_id != phy_channel)
      {
        g_rfb_ctrl->frequency_set(protocol_mode[g_radio_protocol_mode].channel_frequency[phy_channel]);
        change_channel = 1;
      }
      rssi_read = g_rfb_ctrl->rssi_read(is_rssi_valid);
      if (change_channel)
      {
        g_rfb_ctrl->frequency_set(protocol_mode[g_radio_protocol_mode].channel_frequency[g_channel_id]);
      }
    }
    if (rssi_read < 128)
    {
      rssi_value = (-1) * rssi_read;
    }
  } while (0);

  return rssi_value;
}

// Noise floor values for non-listening nodes that cannot get average rssi values.
// In ABS values, will be converted to negative in return value
#define NOISE_FLOOR_VAL   96    // Default rssi value that is considered noise floor when no valid reading is available
#define NOISE_FLOOR_MIN   65    // Min rssi value that is considered noise floor when no valid reading is available
#define NOISE_FLOOR_MAX   110   // Max rssi value that is considered noise floor when no valid reading is available

bool zwave_radio_rssi_read_all_channels(int8_t *average_rssi, uint8_t average_rssi_size)
{
  bool retval = false;
  if ((ZPAL_RADIO_MODE_NON_LISTENING == g_radio_mode) && (NULL != average_rssi) && (0 != average_rssi_size))
  {
    // None listening nodes have not been able to get average rssi values so just read the current rssi value for each channel
    bool is_rssi_valid = false;
    uint8_t rssi_read = 0;
    for (uint8_t i = 0; i < average_rssi_size; i++) {
      rssi_read = zwave_radio_rssi_read(i, &is_rssi_valid);
      if (!is_rssi_valid && ((rssi_read < NOISE_FLOOR_MIN) || (rssi_read > NOISE_FLOOR_MAX))) {
        rssi_read = NOISE_FLOOR_VAL;
      }
      average_rssi[i] = (rssi_read < 128) ? (-1) * (int8_t)rssi_read : (int8_t)rssi_read;
    }
    retval = true;
  } else if (!((NULL == average_rssi) || (0 == average_rssi_size) || (m_rssi_sample_frequency == 0) || (m_rssi_sample_count_average == 0))) {
    RFB_EVENT_STATUS status = RFB_CNF_EVENT_NOT_AVAILABLE;
    memset(average_rssi, ZPAL_RADIO_INVALID_RSSI_DBM, average_rssi_size);
    if (average_rssi_size > ZWAVE_SCAN_STAGE_MAX) {
      average_rssi_size = ZWAVE_SCAN_STAGE_MAX;
    }
    status = g_rfb_ctrl->zwave_all_channel_average_rssi_read(m_rssi_average);
    if (status == RFB_EVENT_SUCCESS) {
      uint8_t rssi_read = 0;
      for (uint8_t channel = 0; channel < average_rssi_size; channel++) {
        uint8_t phy_channel = convert_zpal_channel_to_phy_channel(g_radio_protocol_mode, channel);
        rssi_read = m_rssi_average[convert_phy_channel_to_phy_scan_stage(g_radio_protocol_mode, phy_channel)];
        average_rssi[channel] = (rssi_read < 128) ? (-1) * (int8_t)rssi_read : (int8_t)rssi_read;
      }
      retval = true;
    }
  }
  return retval;
}

static zpal_radio_wave_type_t m_last_wave_type = ZPAL_RADIO_WAVE_TYPE_UNMODULATED;

/**
 * @brief Set radio to continuous wave transmit on current tx channel
 *
 * @param enable single tone mode - unmodulated carrier
 *   true:  Enable single tone mode
 *   false: Disable single tone mode
 * @param channel channel number for carrier wave
 * @param tx_power
 * @param wave_type
 */
zpal_status_t zwave_radio_tx_continues_wave_set(bool enable, zpal_radio_zwave_channel_t channel_id, int8_t power, __attribute__((unused)) zpal_radio_wave_type_t wave_type)
{
  zpal_status_t status = ZPAL_STATUS_FAIL;

  if (ZPAL_RADIO_WAVE_TYPE_COUNT > wave_type)
  {
    if (enable)
    {
      m_last_wave_type = wave_type;
      // Disable channel scanning, Set Tx frequency and Tx power
      g_channel_id = convert_zpal_channel_to_phy_channel(g_radio_protocol_mode, channel_id);
      zwave_radio_channel_scan_setup(false);
      g_radio_speed = protocol_mode[g_radio_protocol_mode].channel_datarate[g_channel_id];
      g_rfb_ctrl->tx_config_set(0, g_radio_speed, 0, 0, 0, 0, 0);
      g_rfb_ctrl->frequency_set(protocol_mode[g_radio_protocol_mode].channel_frequency[g_channel_id]);
      radio_phy_tx_power_set(g_channel_id, protocol_mode[g_radio_protocol_mode].band_type, power);
    }
    else
    {
      wave_type = m_last_wave_type;
    }

    if (ZPAL_RADIO_WAVE_TYPE_UNMODULATED == wave_type)
    {
      if (RFB_EVENT_SUCCESS == g_rfb_ctrl->tx_continuous_wave_set(enable))
      {
        status = ZPAL_STATUS_OK;
      }
    }
    else if (ZPAL_RADIO_WAVE_TYPE_MODULATED == wave_type)
    {
      if (RFB_EVENT_SUCCESS == g_rfb_ctrl->tx_continuous_modulated_wave_set(enable, false))
      {
        status = ZPAL_STATUS_OK;
      }
    }
    else if (ZPAL_RADIO_WAVE_TYPE_MODULATED_WHITENING == wave_type)
    {
      if (RFB_EVENT_SUCCESS == g_rfb_ctrl->tx_continuous_modulated_wave_set(enable, true))
      {
        status = ZPAL_STATUS_OK;
      }
    }
  }
  return status;
}

uint8_t zpal_radio_zpal_channel_to_internal(zpal_radio_zwave_channel_t zwave_channel_id)
{
  return convert_zpal_channel_to_phy_channel(g_radio_protocol_mode, zwave_channel_id);
}

zpal_status_t zwave_radio_rf_debug_set(bool rf_state_enable)
{
  radio_gpio_rf_debug(rf_state_enable);

  return ZPAL_STATUS_OK;
}

#ifdef DEBUG_CALIBRATION
static void radio_rf_debug_calibration_dump(void)
{
  uint32_t reg_val;
  uint16_t reg_addr;

  reg_addr = 0x308;
  reg_val = RfMcu_RegGet(reg_addr);
  reg_val &= 0x0000FFFF;

  DPRINTF("reg rxiq_gain:%"PRIu32"\n", (reg_val >> 0)  & 0x7f);
  DPRINTF("reg rxiq_gain_sel:%"PRIu32"\n", (reg_val >> 7)  & 0x01);
  DPRINTF("reg rxiq_phase:%"PRIu32"\n", (reg_val >> 8)  & 0x7f);
  DPRINTF("reg rxiq_phase_sel:%"PRIu32"\n", (reg_val >> 15) & 0x01);

  reg_addr = 0x380;
  reg_val = RfMcu_RegGet(reg_addr);
  reg_val &= 0x0000003F;

  DPRINTF("reg if_filt_asic:%"PRIu32"\n", reg_val);

  reg_addr = 0x484;
  reg_val = RfMcu_RegGet(reg_addr);
  reg_val &= 0x0000003F;

  DPRINTF("reg xosc_cap_ini:%"PRIu32"\n", reg_val);

  DPRINTF("hreg xosc_cap_ini:%"PRIu16"\n", PMU_CTRL->PMU_SOC_PMU_XTAL1.bit.XOSC_CAP_INI);

  reg_addr = 0x403C;
  reg_val = RfMcu_RegGet(reg_addr);
  DPRINTF("modem select:%"PRIu32"\n", (reg_val >> 24) & 0xff);
}

static void radio_rf_debug_reg_dump(void)
{
  uint32_t reg_val;
  uint16_t reg_addr;

  /* MAC */
  DPRINT("MAC:\n");
  for (reg_addr = 0x100 ; reg_addr < 0x200 ; reg_addr += 4)
  {
    reg_val = RfMcu_RegGet(reg_addr);
    DPRINTF("Addr: 0x%04"PRIx16", Val: 0x%08"PRIx32"\n", reg_addr, reg_val);
  }

  /* BMU */
  DPRINT("BMU:\n");
  for (reg_addr = 0x200 ; reg_addr < 0x300 ; reg_addr += 4)
  {
    reg_val = RfMcu_RegGet(reg_addr);
    DPRINTF("Addr: 0x%04"PRIx16", Val: 0x%08"PRIx32"\n", reg_addr, reg_val);
  }

  /* RF */
  DPRINT("RF:\n");
  for (reg_addr = 0x300 ; reg_addr < 0x400 ; reg_addr += 4)
  {
    reg_val = RfMcu_RegGet(reg_addr);
    DPRINTF("Addr: 0x%04"PRIx16", Val: 0x%08"PRIx32"\n", reg_addr, reg_val);
  }

  /* PMU */
  DPRINT("PMU:\n");
  for (reg_addr = 0x400 ; reg_addr < 0x500 ; reg_addr += 4)
  {
    reg_val = RfMcu_RegGet(reg_addr);
    DPRINTF("Addr: 0x%04"PRIx16", Val: 0x%08"PRIx32"\n", reg_addr, reg_val);
  }

  /* FW */
  DPRINT("FW:\n");
  for (reg_addr = 0x4000 ; reg_addr < 0x4040 ; reg_addr += 4)
  {
    reg_val = RfMcu_RegGet(reg_addr);
    DPRINTF("Addr: 0x%04"PRIx16", Val: 0x%08"PRIx32"\n", reg_addr, reg_val);
  }
}

static void radio_app_host_debug_reg_dump(void)
{
  uint32_t *reg_addr;

  /* SYS_CTRL */
  for (reg_addr = (uint32_t *)0x50000000 ; reg_addr < (uint32_t *)0x500000C0 ; reg_addr += 1)
  {
    printf("Addr: 0x%08"PRIx32", Val: 0x%08"PRIx32" \r\n", (uint32_t)reg_addr, *reg_addr);
  }

  /* SEC_CTRL */
  for (reg_addr = (uint32_t *)0x50003000 ; reg_addr < (uint32_t *)0x5000304C ; reg_addr += 1)
  {
    printf("Addr: 0x%08"PRIx32", Val: 0x%08"PRIx32" \r\n", (uint32_t)reg_addr, *reg_addr);
  }

  /* SOC_PMU */
  for (reg_addr = (uint32_t *)0x50006000 ; reg_addr < (uint32_t *)0x500060F4 ; reg_addr += 1)
  {
    printf("Addr: 0x%08"PRIx32", Val: 0x%08"PRIx32" \r\n", (uint32_t)reg_addr, *reg_addr);
  }

  /* RT569_AHB */
  for (reg_addr = (uint32_t *)0x5001A000 ; reg_addr < (uint32_t *)0x5001A038 ; reg_addr += 1)
  {
    printf("Addr: 0x%08"PRIx32", Val: 0x%08"PRIx32" \r\n", (uint32_t)reg_addr, *reg_addr);
  }
}

void zpal_radio_rf_debug_reg_setting_list(__attribute__((unused)) bool listallreg)
{
  radio_rf_debug_calibration_dump();
  if (true == listallreg)
  {
    radio_rf_debug_reg_dump();
    radio_app_host_debug_reg_dump();
  }
}
#endif  // #ifdef DEBUG_CALIBRATION

/**
 * @brief Get radio firmware version
 */
uint32_t zwave_radio_version_get()
{
  return g_rfb_ctrl->fw_version_get();
}

void zwave_radio_set_lbt_level(uint8_t zwavechannel, int8_t level)
{
  if (MAX_TOTAL_CHANNEL_NUM > zwavechannel)
  {
    g_cca_threshold[zwavechannel] = abs(level);
  }
}

int8_t zwave_radio_get_lbt_level(uint8_t zwavechannel)
{
  if (MAX_TOTAL_CHANNEL_NUM > zwavechannel)
  {
    return (-1) * g_cca_threshold[zwavechannel];
  }
  else
  {
    return -127;
  }
}

static zpal_radio_zwave_channel_t g_channel_statistic = ZPAL_RADIO_ZWAVE_CHANNEL_0;

void radio_rf_channel_statistic_tx_channel_set(zpal_radio_zwave_channel_t zwavechannel)
{
  enter_critical_section();
  g_channel_statistic = zwavechannel;
  leave_critical_section();
}

zpal_radio_zwave_channel_t radio_rf_channel_statistic_tx_channel_get(void)
{
  zpal_radio_zwave_channel_t zwavechannel;
  enter_critical_section();
  zwavechannel = g_channel_statistic;
  leave_critical_section();
  return zwavechannel;
}

void zwave_radio_abort(void)
{
  if (is_rx_start)
  {
    g_rfb_ctrl->auto_state_set(false);
  }
  is_rx_start = false;
}

bool zwave_radio_is_lbt_enabled(void)
{
  return is_lbt_enabled;
}

void zwave_radio_enable_flirs_mode(void)
{
  zwave_radio_idle();
  is_rx_flirs = true;
  uint32_t wakeup_time = protocol_mode_flirs_wake_up_time[g_radio_protocol_mode][ZPAL_RADIO_WAKEUP_EVERY_250ms == g_radio.wakeup ? 0 : 1];
  if (ZPAL_RADIO_PROTOCOL_MODE_1 == g_radio_protocol_mode)
  {
    g_rfb_ctrl->zwave_scan_set(ZWAVE_SCANNING_R2_ONLY, convert_zpal_region_to_t32cz20_scan_region(g_radio.region));
    g_rfb_ctrl->zwave_wake_on_radio_set(0, wakeup_time, 4, 1, 0, 0, 0);
  }
  else if (ZPAL_RADIO_PROTOCOL_MODE_2 == g_radio_protocol_mode)
  {
    g_rfb_ctrl->zwave_scan_set(ZWAVE_SCANNING_R3_SWITCH, convert_zpal_region_to_t32cz20_scan_region(g_radio.region));
    g_rfb_ctrl->zwave_wake_on_radio_set(0, wakeup_time, 4, 1, 0, 0, 0);
  }
  else if (ZPAL_RADIO_PROTOCOL_MODE_4 == g_radio_protocol_mode)
  {
    g_rfb_ctrl->zwave_scan_set(ZWAVE_SCANNING_LR_SWITCH, convert_zpal_region_to_t32cz20_scan_region(g_radio.region));
    g_rfb_ctrl->zwave_wake_on_radio_set(0, wakeup_time, 4, 1,
      10,  //  uint8_t up rssi_read_period how often the rssi is measured 1 means every wake 2 every second wakeup and so on
      3,   //  uint8_t rssi_average_number how many rssi measurement used to calculate average
      5);  //  uint8_t scan_cycle_for_rssi_read the number of scanning cycles used for rssi
  }
}

void zwave_radio_restart_after_flirs_mode(void)
{
  zwave_radio_idle();
  is_rx_flirs = false;
  zwave_radio_channel_scan_setup(0 == protocol_mode[g_radio_protocol_mode].use_fixed_rx_channel);
}

void zwave_radio_fw_preload(void)
{
  struct_rfb_interrupt_event.tx_done = radio_tx_done;
  struct_rfb_interrupt_event.rx_done = radio_rx_done;
  struct_rfb_interrupt_event.rx_timeout = radio_rx_timeout;
  struct_rfb_interrupt_event.rx_beam = radio_rx_beam;

  rfb_comm_fw_preload(&struct_rfb_interrupt_event);
}

void zwave_radio_clear_isr_events(void)
{
  enter_critical_section();
  RfMcu_HostBusySet(true); // let radio MCU wake so radio ISR can be cleared by host MCU
  rfb_comm_rx_queue_read_all(false); // ignore previous RX
  rfb_comm_event_intr_clear();
  RfMcu_HostBusySet(false); // release the radio MCU wake HW flag
  leave_critical_section();
}