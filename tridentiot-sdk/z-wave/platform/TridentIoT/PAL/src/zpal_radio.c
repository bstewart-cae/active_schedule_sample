/// ****************************************************************************
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************

#include "zwave_radio.h"
#include <zpal_radio_utils.h>
#include <Assert.h>
#include <CRC.h>
#include <zpal_power_manager.h>
#include "zpal_radio_private.h"
// #define DEBUGPRINT // NOSONAR
#include <DebugPrint.h>
#ifdef DEBUGPRINT
#include <stdio.h>
#endif

typedef struct
{
  node_id_t         node_id;
  uint32_t          home_id;
  zpal_radio_mode_t node_mode;
} SNetworkId_t;

#define FLIRS_WAKE_TIMEOUT   6  // in ms
#define DEFAULT_TX_POWER (0)
#define RSSI_TOTAL_COUNT (10)

extern const zw_region_rf_setup_t ZW_REGION_SPECIFIC_EU;
extern const zw_region_rf_setup_t ZW_REGION_SPECIFIC_EU_LR;
extern const zw_region_rf_setup_t ZW_REGION_SPECIFIC_EU_LR_BACKUP;
extern const zw_region_rf_setup_t ZW_REGION_SPECIFIC_EU_LR_END_DEVICE;
extern const zw_region_rf_setup_t ZW_REGION_SPECIFIC_US;
extern const zw_region_rf_setup_t ZW_REGION_SPECIFIC_US_LR;
extern const zw_region_rf_setup_t ZW_REGION_SPECIFIC_US_LR_BACKUP;
extern const zw_region_rf_setup_t ZW_REGION_SPECIFIC_US_LR_END_DEVICE;
extern const zw_region_rf_setup_t ZW_REGION_SPECIFIC_ANZ;
extern const zw_region_rf_setup_t ZW_REGION_SPECIFIC_HK;
extern const zw_region_rf_setup_t ZW_REGION_SPECIFIC_MY;
extern const zw_region_rf_setup_t ZW_REGION_SPECIFIC_IN;
extern const zw_region_rf_setup_t ZW_REGION_SPECIFIC_IL;
extern const zw_region_rf_setup_t ZW_REGION_SPECIFIC_RU;
extern const zw_region_rf_setup_t ZW_REGION_SPECIFIC_CN;
extern const zw_region_rf_setup_t ZW_REGION_SPECIFIC_JP;
extern const zw_region_rf_setup_t ZW_REGION_SPECIFIC_KR;

static zw_region_rf_setup_t const *mp_current_region_rf_setup = &ZW_REGION_SPECIFIC_EU;
static SNetworkId_t m_current_network_id = {.node_id = 0, .home_id = 0, .node_mode = ZPAL_RADIO_MODE_NON_LISTENING};
static uint8_t m_home_id_hash = 0x55;
static zpal_radio_profile_t m_current_profile;
static uint8_t m_tx_power_attenuation = 0; // dBm
static bool m_NodeIsLongRangeLocked = false;

#ifdef DEBUGPRINT
static char str_buf[400];
#endif

typedef enum
{
  RADIO_MODE_OFF,
  RADIO_MODE_ON,
  RADIO_MODE_FLIRS,
} radio_mode_t;

/**
 * @ brief list of long range region. Used only as index of the table lrSetupsIds
 */
typedef enum
{
  ZPAL_RADIO_LR_REGION_US,    ///< associated to REGION_US_LR
  ZPAL_RADIO_LR_REGION_EU,    ///< associated to REGION_EU_LR
  ZPAL_RADIO_LR_REGION_COUNT  ///< list item count
} zpal_radio_lr_region_t;

/**
 *  @brief Table with radio setup index for each Long Range region and each Long Range mode.
 */
static const zw_region_rf_setup_t * const lrSetupsIds[ZPAL_RADIO_LR_REGION_COUNT][ZPAL_RADIO_LR_CH_CFG_COUNT] =
{
  /*LR_CH_CFG_NO_LR,        LR_CH_CFG1,                 LR_CH_CFG2,                 LR_CH_CFG3 */
  {&ZW_REGION_SPECIFIC_US, &ZW_REGION_SPECIFIC_US_LR, &ZW_REGION_SPECIFIC_US_LR_BACKUP, &ZW_REGION_SPECIFIC_US_LR_END_DEVICE},
  {&ZW_REGION_SPECIFIC_EU, &ZW_REGION_SPECIFIC_EU_LR, &ZW_REGION_SPECIFIC_EU_LR_BACKUP, &ZW_REGION_SPECIFIC_EU_LR_END_DEVICE},
};

/* for channel statistic functions */
zpal_radio_rf_channel_statistic_t g_channel_radio[5];

/* RSSI storage */
int8_t bg_rssi[MAX_TOTAL_CHANNEL_NUM][RSSI_TOTAL_COUNT];
uint8_t bg_rssi_idx[MAX_TOTAL_CHANNEL_NUM] = {0};
bool bg_rssi_full[MAX_TOTAL_CHANNEL_NUM] = {0};
int8_t ed_rssi[MAX_TOTAL_CHANNEL_NUM][RSSI_TOTAL_COUNT];
uint8_t ed_rssi_idx[MAX_TOTAL_CHANNEL_NUM] = {0};
bool ed_rssi_full[MAX_TOTAL_CHANNEL_NUM] = {0};

static radio_mode_t radio_mode;
static bool flirs_allowed;

void zpal_restart_radio(void )
{
  radio_mode = RADIO_MODE_ON;
  zwave_radio_init(&m_current_profile, mp_current_region_rf_setup);
}

static void
SetRegionEU(void)
{
  mp_current_region_rf_setup = &ZW_REGION_SPECIFIC_EU;
}

static void
SetRegionUS(void)
{
  mp_current_region_rf_setup = &ZW_REGION_SPECIFIC_US;
}

static void
SetRegionANZ(void)
{
  mp_current_region_rf_setup = &ZW_REGION_SPECIFIC_ANZ;
}

static void
SetRegionHK(void)
{
  mp_current_region_rf_setup = &ZW_REGION_SPECIFIC_HK;
}

static void
SetRegionIN(void)
{
  mp_current_region_rf_setup = &ZW_REGION_SPECIFIC_IN;
}

static void
SetRegionIL(void)
{
  mp_current_region_rf_setup = &ZW_REGION_SPECIFIC_IL;
}

static void
SetRegionRU(void)
{
  mp_current_region_rf_setup = &ZW_REGION_SPECIFIC_RU;
}

static void
SetRegionCN(void)
{
  mp_current_region_rf_setup = &ZW_REGION_SPECIFIC_CN;
}

static void
SetRegionJP(void)
{
  mp_current_region_rf_setup = &ZW_REGION_SPECIFIC_JP;
}

static void
SetRegionKR(void)
{
  mp_current_region_rf_setup = &ZW_REGION_SPECIFIC_KR;
}

/**
 * @brief Convert a region from zpal_radio_region_t (full region list) to zpal_radio_lr_region_t (list of long range region)
 * The id returned by the function could be used as index in table of long range setups (lrSetupsIds)
 * @param[in] eRegion: the product region
 * @return the lr region id (zpal_radio_lr_region_t), or ZPAL_RADIO_LR_REGION_COUNT if the region does not support LR.
 */
static zpal_radio_lr_region_t ZW_radioGetLrRegionId(zpal_radio_region_t eRegion)
{
  switch (eRegion)
  {
    case REGION_US_LR:    return ZPAL_RADIO_LR_REGION_US;
    case REGION_EU_LR:    return ZPAL_RADIO_LR_REGION_EU;
    default: return ZPAL_RADIO_LR_REGION_COUNT;
  }
}

void
RadioSetRegionSettings(zpal_radio_region_t region, zpal_radio_lr_channel_config_t eLrChCfg)
{
  zpal_radio_lr_region_t lrRegionId = ZW_radioGetLrRegionId(region);
  if (ZPAL_RADIO_LR_REGION_COUNT > lrRegionId)
  {
    mp_current_region_rf_setup = lrSetupsIds[lrRegionId][eLrChCfg];
  }
  else
  {
    switch ((uint8_t)region)
    {
      case REGION_EU:
        SetRegionEU();
        break;

      case REGION_US:
        SetRegionUS();
        break;

      case REGION_ANZ:
        SetRegionANZ();
        break;

      case REGION_HK:
        SetRegionHK();
        break;

      case REGION_IN:
        SetRegionIN();
        break;

      case REGION_IL:
        SetRegionIL();
        break;

      case REGION_RU:
        SetRegionRU();
        break;

      case REGION_CN:
        SetRegionCN();
        break;

      case REGION_JP:
        SetRegionJP();
        break;

      case REGION_KR:
        SetRegionKR();
        break;

      default:
      {
        break;
      }
    }
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

void radio_rf_channel_statistic_rx_foreign_homeid(__attribute__((unused)) zpal_radio_zwave_channel_t zwavechannel)
{
  g_channel_radio[radio_rf_channel_statistic_tx_channel_get()].rf_channel_rx_foreign_homeid++;
}

static uint8_t framebuf[190];
static zpal_radio_rx_parameters_t rx_parameters;

void zpal_radio_get_last_received_frame(void)
{
  bool anyframesreceived = false;
  zpal_radio_receive_frame_t *frame = (zpal_radio_receive_frame_t *)&framebuf[0];
  zpal_radio_rx_parameters_t *params = &rx_parameters;
  anyframesreceived = zwave_radio_rx_frame_fifo_pop(frame, params);
  if (true == anyframesreceived)
  {
#ifdef DEBUGPRINT
    snprintf(str_buf, sizeof(str_buf), "RX(%02X,%02X,%i) ", frame->frame_content_length, params->channel_id, params->rssi);
#ifdef DEBUG_DUMP_FRAME
    radio_generate_string_data(str_buf, sizeof(str_buf), frame->frame_content, frame->frame_content_length);
#endif
    DPRINTF("%s\n", str_buf);
#endif
    if (memcmp(frame->frame_content, m_current_profile.home_id, 4))
    {
      m_current_profile.network_stats->rx_foreign_home_id++;
      radio_rf_channel_statistic_rx_foreign_homeid(params->channel_id);
    }

    m_current_profile.receive_handler_cb(params, frame);
    anyframesreceived = (0 != zwave_radio_rx_frame_fifo_count());
  }
  if (true == anyframesreceived)
  {
    m_current_profile.rx_cb(ZPAL_RADIO_EVENT_RX_COMPLETE);
  }
}

bool zpal_radio_network_filter_enable(bool enable)
{
  return zwave_radio_network_filter_enable(enable);
}

bool zpal_radio_network_id_filter_enabled_get(void)
{
  return zwave_radio_network_id_filter_enabled_get();
}

void zpal_radio_network_id_filter_set(bool enable)
{
  zwave_radio_network_id_filter_set(enable);
}

void zpal_radio_get_network_ids(uint32_t *home_id, node_id_t *node_id, zpal_radio_mode_t *mode, uint8_t *home_id_hash)
{
  zwave_radio_get_network_ids(&m_current_network_id.home_id, &m_current_network_id.node_id, &m_current_network_id.node_mode, &m_home_id_hash);
  *home_id = m_current_network_id.home_id;
  *node_id = m_current_network_id.node_id;
  *mode = m_current_network_id.node_mode;
  *home_id_hash = m_home_id_hash;
}

void zpal_radio_set_network_ids(uint32_t home_id, node_id_t node_id, zpal_radio_mode_t mode, uint8_t home_id_hash)
{
  m_current_network_id.node_id = node_id;
  m_current_network_id.home_id = home_id;
  m_current_network_id.node_mode = mode;
  m_home_id_hash = home_id_hash;
  zwave_radio_set_network_ids(home_id, node_id, mode, home_id_hash);
}

void zpal_radio_init(zpal_radio_profile_t * const profile)
{
  m_current_profile = *profile;
  RadioSetRegionSettings(m_current_profile.region, m_current_profile.active_lr_channel_config);
  radio_mode = RADIO_MODE_ON;
  zwave_radio_init(&m_current_profile, mp_current_region_rf_setup);
}

zpal_status_t zpal_radio_change_region(zpal_radio_region_t region, zpal_radio_lr_channel_config_t eLrChCfg)
{

  if (ZPAL_RADIO_LR_CH_CFG_COUNT <= eLrChCfg)
  {
    return ZPAL_STATUS_INVALID_ARGUMENT;
  }
  else if ( (m_current_profile.region == region) && (eLrChCfg == m_current_profile.active_lr_channel_config) )
  {
    //nothing to do : the requested configuration is already the active configuration.
    return ZPAL_STATUS_OK;
  }
  else
  {
    m_current_profile.region = region;
    RadioSetRegionSettings(m_current_profile.region, eLrChCfg);
    m_current_profile.active_lr_channel_config = eLrChCfg;
    radio_mode = RADIO_MODE_ON;
    zwave_radio_init(&m_current_profile, mp_current_region_rf_setup);
    zpal_radio_start_receive();
    m_current_profile.region_change_cb(ZPAL_RADIO_EVENT_FLAG_SUCCESS);
    return ZPAL_STATUS_OK;
  }
}

uint32_t zpal_radio_get_beam_count(void)
{
  return zwave_radio_get_beam_count();
}

uint32_t zpal_radio_get_current_beam_count(void)
{
  return zwave_radio_get_current_beam_count();
}

uint32_t zpal_radio_stop_current_beam_count(void)
{
  return zwave_radio_stop_current_beam_count();
}

uint32_t zpal_radio_get_last_stop_beam_count(void)
{
  return zwave_radio_get_last_stop_beam_count();
}

void zpal_radio_set_rx_fixed_channel(__attribute__((unused)) uint8_t enable, __attribute__((unused)) uint8_t channel)
{
  zwave_radio_set_rx_fixed_channel(enable, channel);
}

uint8_t zpal_radio_region_channel_count_get(void)
{
  return mp_current_region_rf_setup->channel_count;
}

zpal_radio_region_t zpal_radio_get_region(void)
{
  return m_current_profile.region;
}

int8_t classic_zwave_tx_power_helper(zpal_radio_transmit_parameter_t const * const tx_parameters, int8_t tx_power)
{
  int16_t tx_power_radio = tx_power;
  // Adjust the tx power for classic Z-Wave channels if enabled
  if ((ZPAL_RADIO_ZWAVE_CHANNEL_3 > (*tx_parameters).channel_id) && zpal_radio_classic_tx_power_is_enabled()) {
    if (ZPAL_RADIO_TX_POWER_REDUCED == tx_power) {
      tx_power = -zpal_radio_get_reduce_tx_power();
    } else if (tx_power > 0) {
      tx_power = 0;
    } else if (tx_power < -9) {
      tx_power = -9;
    }
    tx_power_radio = (m_current_profile.tx_power_max - m_current_profile.tx_power_adjust) / 10;
    tx_power_radio = tx_power_radio + tx_power - m_tx_power_attenuation;
    int8_t tx_max_power = zwave_radio_max_tx_power_get((*tx_parameters).channel_id);
    if (tx_power_radio > tx_max_power) {
      tx_power_radio = tx_max_power;
    }
  }
  return (int8_t)tx_power_radio;
}

zpal_status_t zpal_radio_transmit(zpal_radio_transmit_parameter_t const * const tx_parameters,
                                  uint8_t                                       frame_header_length,
                                  uint8_t                         const * const frame_header_buffer,
                                  uint8_t                                       frame_payload_length,
                                  uint8_t                         const * const frame_payload_buffer,
                                  uint8_t                                       use_lbt,
                                  int8_t                                        tx_power)
{
  int8_t zwave_tx_power = classic_zwave_tx_power_helper(tx_parameters, tx_power);
  return zwave_radio_transmit(tx_parameters, frame_header_length, frame_header_buffer, frame_payload_length, frame_payload_buffer, use_lbt, zwave_tx_power);
}

zpal_status_t zpal_radio_transmit_beam(zpal_radio_transmit_parameter_t const * const tx_parameters,
                                       uint8_t                                       beam_data_len,
                                       uint8_t                         const * const beam_data,
                                       int8_t                                        tx_power)
{
  int8_t zwave_tx_power = classic_zwave_tx_power_helper(tx_parameters, tx_power);
  return zwave_radio_transmit_beam(tx_parameters, beam_data_len, beam_data, zwave_tx_power);
}

void zpal_radio_start_receive(void)
{
  DPRINT("Rf+\n");
  radio_mode = RADIO_MODE_ON;
  zwave_radio_start_receive(true);
}

void zpal_radio_power_down(void)
{
  DPRINT("Rf-\n")  ;
  radio_mode = RADIO_MODE_OFF;
  zwave_radio_idle();
}

void zpal_radio_power_shutdown(void)
{
  DPRINT("Rf--\n");
  radio_mode = RADIO_MODE_OFF;
  zwave_radio_forced_idle();
}
/**
 * @param enable continues carrier signale - true = on, false = off
 * @param channel Channel to use for carrier wave
 * @param power the continues signal should be transmitted at
 * @param wave_type the type of the continues wave to be transmitted
 */
zpal_status_t zpal_radio_tx_continues_wave_set(__attribute__((unused)) bool enable,
                                               __attribute__((unused)) zpal_radio_zwave_channel_t channel,
                                               __attribute__((unused)) int8_t power,
                                               __attribute__((unused)) zpal_radio_wave_type_t wave_type)
{
  return zwave_radio_tx_continues_wave_set(enable, channel, power, wave_type);
}

zpal_status_t zpal_radio_rf_debug_set(__attribute__((unused)) bool rf_state_enable)
{
  return zwave_radio_rf_debug_set(rf_state_enable);
}

void zpal_radio_rssi_config_get(__attribute__((unused)) uint16_t *rssi_sample_frequency, __attribute__((unused)) uint8_t *rssi_sample_count_average)
{
  zwave_radio_rssi_config_get(rssi_sample_frequency, rssi_sample_count_average);
}

void zpal_radio_rssi_config_set(__attribute__((unused)) uint16_t rssi_sample_frequency, __attribute__((unused)) uint8_t rssi_sample_count_average)
{
  zwave_radio_rssi_config_set(rssi_sample_frequency, rssi_sample_count_average);
}

int8_t zpal_radio_rssi_read(__attribute__((unused)) uint8_t channel, bool *is_rssi_valid)
{
  return zwave_radio_rssi_read(channel, is_rssi_valid);
}

bool zpal_radio_rssi_read_all_channels(int8_t *average_rssi, uint8_t average_rssi_size)
{
  return zwave_radio_rssi_read_all_channels(average_rssi, average_rssi_size);
}

int8_t zpal_radio_min_tx_power_get(__attribute__((unused)) uint8_t channel_id)
{
  return zwave_radio_min_tx_power_get(channel_id);
}

int8_t zpal_radio_max_tx_power_get(__attribute__((unused)) uint8_t channel_id)
{
  return zwave_radio_max_tx_power_get(channel_id);
}

void zpal_radio_chip_get_version(uint16_t *chip_id, uint16_t *chip_rev)
{
  zwave_radio_chip_get_version(chip_id, chip_rev);
}

uint32_t zpal_radio_version_get(void)
{
  return zwave_radio_version_get();
}

zpal_radio_protocol_mode_t zpal_radio_get_protocol_mode(void)
{
  return mp_current_region_rf_setup->protocolMode;
}

zpal_radio_zwave_channel_t zpal_radio_get_last_tx_channel(void)
{
  return zwave_radio_get_last_tx_channel();
}

zpal_radio_zwave_channel_t zpal_radio_get_last_beam_channel(void)
{
  return zwave_radio_get_last_beam_channel();
}

int8_t zpal_radio_get_last_beam_rssi(void)
{
  return zwave_radio_get_last_beam_rssi();
}

uint8_t zpal_radio_get_last_beam_data(uint8_t* p_last_beam_data, uint8_t max_beam_data)
{
  return zwave_radio_get_last_beam_data(p_last_beam_data, max_beam_data);
}

void zpal_radio_set_lbt_level(uint8_t channel, int8_t level)
{
  zwave_radio_set_lbt_level(channel, level);
}

int8_t zpal_radio_get_lbt_level(__attribute__((unused)) uint8_t channel)
{
  return zwave_radio_get_lbt_level(channel);
}

void zpal_radio_enable_rx_broadcast_beam(bool enable)
{
  zwave_radio_rx_broadcast_beam_enable(enable);
}

void zpal_radio_clear_tx_timers(void)
{
  (*m_current_profile.network_stats).tx_time_channel_0 = 0;
  (*m_current_profile.network_stats).tx_time_channel_1 = 0;
  (*m_current_profile.network_stats).tx_time_channel_2 = 0;
  (*m_current_profile.network_stats).tx_time_channel_3 = 0;
  (*m_current_profile.network_stats).tx_time_channel_4 = 0;
}

void zpal_radio_clear_network_stats(void)
{
  static zpal_radio_network_stats_t NetworkStatistic = {0};
  *m_current_profile.network_stats = NetworkStatistic;
}

zpal_status_t zpal_radio_get_background_rssi(uint8_t channel, int8_t *rssi)
{
  bool start_rx = 0;
  bool is_rssi_valid = false;

  if (!zwave_radio_is_receive_started())
  {
    zwave_radio_start_receive(true);
    start_rx = 1;
  }

  *rssi = zwave_radio_rssi_read(channel, &is_rssi_valid);

  if (start_rx)
  {
    zwave_radio_idle();
  }

  return (is_rssi_valid) ? ZPAL_STATUS_OK : ZPAL_STATUS_FAIL;
}

zpal_tx_power_t
zpal_radio_get_default_tx_power()
{
  return DEFAULT_TX_POWER;
}

uint8_t zpal_radio_get_reduce_tx_power(void)
{
  // -3dbm
  return 3;
}

void zpal_radio_enable_flirs(void)
{
  zwave_radio_enable_flirs_mode();
  // Make sure that any pending radio ISR is cleared
  //Any pending radio interrupt will prevent the radio MCU from entering the Rx FLIRS mode
  zwave_radio_clear_isr_events();
  radio_mode = RADIO_MODE_FLIRS;
  flirs_allowed = true;
}

bool zpal_radio_is_flirs_enabled(void)
{
  return radio_mode == RADIO_MODE_FLIRS;
}

void zpal_radio_start_receive_after_power_down(__attribute__((unused))  bool wait_for_beam)
{
   zwave_radio_restart_after_flirs_mode();
   if (!zpal_radio_is_fragmented_beam_enabled())
   {
     zwave_radio_start_receive(true);
     radio_mode = RADIO_MODE_ON;
   }
}

void
zpal_radio_abort(void)
{
  zwave_radio_abort();
}

void zpal_radio_reset_after_beam_receive(bool start_receiver)
{
  radio_mode = RADIO_MODE_ON;
  zwave_radio_start_receive(start_receiver);
}

bool zpal_radio_is_fragmented_beam_enabled(void)
{
  return mp_current_region_rf_setup->useFragementedBeam == RADIO_FRAGEMENTED_BEAM_USE;
}

void zpal_radio_calibrate(void)
{
  ASSERT(false);
}

bool zpal_radio_is_lbt_enabled(void)
{
  return zwave_radio_is_lbt_enabled();
}

uint16_t zpal_radio_get_beam_startup_time(void)
{
  return 0;
}

node_id_t zpal_radio_get_beam_node_id(void)
{
  return zwave_radio_get_last_beam_node_id();
}

zpal_tx_power_t zpal_radio_get_minimum_lr_tx_power(void)
{
  return (-60 / 10);
}

zpal_tx_power_t zpal_radio_get_maximum_lr_tx_power(void)
{
  return (m_current_profile.tx_power_max_lr / 10);
}

zpal_tx_power_t
zpal_radio_get_maximum_tx_power(void)
{
  return ZW_TX_POWER_20DBM;
}

__attribute__((weak)) bool
zpal_radio_classic_tx_power_is_enabled(void)
{
  return true;
}

zpal_tx_power_t
zpal_radio_set_classic_tx_power(zpal_tx_power_t tx_power)
{
  m_current_profile.tx_power_max = zwave_radio_classic_tx_power_set(tx_power);
  return m_current_profile.tx_power_max;
}

zpal_tx_power_t
zpal_radio_get_classic_tx_power(void)
{
  return zwave_radio_classic_tx_power_get();
}

zpal_tx_power_t
zpal_radio_set_classic_tx_power_adjust(zpal_tx_power_t tx_power_adjust)
{
  if (tx_power_adjust < -50)
  {
    tx_power_adjust = -50;
  }
  else if (tx_power_adjust > 50)
  {
    tx_power_adjust = 50;
  }
  m_current_profile.tx_power_adjust = tx_power_adjust;
  return m_current_profile.tx_power_adjust;
}

zpal_tx_power_t zpal_radio_get_classic_tx_power_adjust(void)
{
  return m_current_profile.tx_power_adjust;
}

uint8_t zpal_radio_set_classic_tx_power_attenuation(uint8_t attenuation)
{
  if (attenuation > 20)
  {
    attenuation = 20;
  }
  m_tx_power_attenuation = attenuation;
  return attenuation;
}

uint8_t zpal_radio_get_classic_tx_power_attenuation(void)
{
  return m_tx_power_attenuation;
}

bool zpal_radio_is_debug_enabled(void)
{
  return false;
}

const zpal_radio_profile_t * zpal_radio_get_rf_profile(void)
{
  return &m_current_profile;
}

zpal_radio_lr_channel_config_t zpal_radio_get_lr_channel_config(void)
{
  return m_current_profile.active_lr_channel_config;
}

zpal_radio_lr_channel_t zpal_radio_get_primary_long_range_channel(void)
{
  return m_current_profile.primary_lr_channel;
}

void zpal_radio_set_primary_long_range_channel(zpal_radio_lr_channel_t channel)
{
  if (zpal_radio_region_is_long_range(m_current_profile.region) && (ZPAL_RADIO_LR_CHANNEL_UNINITIALIZED != channel) &&
      (ZPAL_RADIO_LR_CHANNEL_UNKNOWN > channel) && (m_current_profile.primary_lr_channel != channel))
  {
    m_current_profile.primary_lr_channel = channel;
  }
}

void zpal_radio_set_long_range_lock(bool lock)
{
  m_NodeIsLongRangeLocked = lock;
}

bool zpal_radio_is_long_range_locked(void)
{
  return m_NodeIsLongRangeLocked;
}

int8_t zpal_radio_get_flirs_beam_tx_power(void)
{
  return zwave_radio_get_last_beam_tx_power();
}

bool zpal_radio_is_transmit_allowed(__attribute__((unused)) uint8_t channel, __attribute__((unused)) uint8_t frame_length, __attribute__((unused)) uint8_t frame_priority)
{
  return zwave_radio_is_transmit_allowed(channel, frame_length, frame_priority);
}

uint8_t zpal_radio_attenuate_get(void)
{
  return zpal_radio_get_classic_tx_power_attenuation();
}

bool zpal_radio_attenuate(__attribute__((unused)) uint8_t adjust_tx_power)
{
  return zpal_radio_set_classic_tx_power_attenuation(adjust_tx_power);
}

void zpal_radio_request_calibration(__attribute__((unused)) bool forced)
{
}

bool
zpal_radio_get_long_range_channel_auto_mode(void)
{
  return m_current_profile.lr_channel_auto_mode;
}

void
zpal_radio_set_long_range_channel_auto_mode(bool enable)
{
  m_current_profile.lr_channel_auto_mode = enable;
}

void
zpal_radio_rf_channel_statistic_clear(zpal_radio_zwave_channel_t zwavechannel)
{
  static zpal_radio_rf_channel_statistic_t ChannelStatistic = {0};
  g_channel_radio[zwavechannel] = ChannelStatistic;
  bg_rssi_idx[zwavechannel] = 0;
  bg_rssi_full[zwavechannel] = 0;
  ed_rssi_idx[zwavechannel] = 0;
  ed_rssi_full[zwavechannel] = 0;
}

bool
zpal_radio_rf_channel_statistic_get(zpal_radio_zwave_channel_t zwavechannel, zpal_radio_rf_channel_statistic_t* p_radio_channel_statistic)
{
  *p_radio_channel_statistic = g_channel_radio[zwavechannel];
  return true;
}

void
zpal_radio_rf_channel_statistic_tx_channel_set(zpal_radio_zwave_channel_t zwavechannel)
{
  radio_rf_channel_statistic_tx_channel_set(zwavechannel);
}

void
zpal_radio_rf_channel_statistic_tx_frames(void)
{
  g_channel_radio[radio_rf_channel_statistic_tx_channel_get()].rf_channel_tx_frames++;
}

void
zpal_radio_rf_channel_statistic_tx_retries(void)
{
  g_channel_radio[radio_rf_channel_statistic_tx_channel_get()].rf_channel_tx_retries++;
}

void
zpal_radio_rf_channel_statistic_tx_lbt_failures(void)
{
  g_channel_radio[radio_rf_channel_statistic_tx_channel_get()].rf_channel_tx_lbt_failures++;
}

void zpal_radio_rf_channel_statistic_background_rssi_average_update(zpal_radio_zwave_channel_t zwavechannel, int8_t rssi)
{
  uint8_t i;
  uint8_t average_count;
  int16_t sum = 0;

  bg_rssi[zwavechannel][bg_rssi_idx[zwavechannel]] = rssi;
  bg_rssi_idx[zwavechannel] = (bg_rssi_idx[zwavechannel] + 1) % 10;
  if (bg_rssi_idx[zwavechannel] == 0)
  {
    bg_rssi_full[zwavechannel] = 1;
  }

  if (bg_rssi_full[zwavechannel])
  {
    average_count = RSSI_TOTAL_COUNT;
  }
  else
  {
    average_count = bg_rssi_idx[zwavechannel];
  }

  for (i = 0; i < average_count; i++)
  {
    sum += bg_rssi[zwavechannel][i];
  }

  g_channel_radio[zwavechannel].rf_channel_background_rssi_average = sum / average_count;
}

void zpal_radio_rf_channel_statistic_end_device_rssi_average_update(zpal_radio_zwave_channel_t zwavechannel, int8_t rssi)
{
  uint8_t i;
  uint8_t average_count;
  int16_t sum = 0;

  ed_rssi[zwavechannel][ed_rssi_idx[zwavechannel]] = rssi;
  ed_rssi_idx[zwavechannel] = (ed_rssi_idx[zwavechannel] + 1) % 10;
  if (ed_rssi_idx[zwavechannel] == 0)
  {
    ed_rssi_full[zwavechannel] = 1;
  }

  if (ed_rssi_full[zwavechannel])
  {
    average_count = RSSI_TOTAL_COUNT;
  }
  else
  {
    average_count = ed_rssi_idx[zwavechannel];
  }

  for (i = 0; i < average_count; i++)
  {
    sum += ed_rssi[zwavechannel][i];
  }

  g_channel_radio[zwavechannel].rf_channel_end_device_rssi_average = sum / average_count;
}

bool zpal_radio_is_region_supported(const zpal_radio_region_t region)
{
  bool supported = false;

  switch (region) {
    case REGION_EU:  //NOSONAR
    case REGION_US:
    case REGION_ANZ:
    case REGION_HK:
    case REGION_IN:
    case REGION_IL:
    case REGION_RU:
    case REGION_CN:
    case REGION_US_LR:
    case REGION_EU_LR:
    case REGION_JP:
    case REGION_KR:
      supported = true;
      break;

    default:
      supported = false;
      break;
  }
  return supported;
}

bool zpal_radio_tx_power_config(zpal_tx_power_t tx_power_max)
{
  return zwave_radio_tx_power_config(tx_power_max);
}

void zpal_radio_fw_preload(void)
{
  zwave_radio_fw_preload();
}

