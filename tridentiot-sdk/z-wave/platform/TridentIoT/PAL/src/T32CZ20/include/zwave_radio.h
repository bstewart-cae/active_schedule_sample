/// ***************************************************************************
///
/// @file zwave_radio.h
///
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************

#ifndef _ZWAVE_RADIO_H_
#define _ZWAVE_RADIO_H_
/**************************************************************************************************
 *    INCLUDES
 *************************************************************************************************/
#include <stdlib.h>
#include <string.h>
#include "ZW_region_rf_definitions.h"
#include "zpal_radio.h"
#include "zpal_radio_private.h"

/**************************************************************************************************
 *    CONSTANTS AND DEFINES
 *************************************************************************************************/
#define MAX_TOTAL_CHANNEL_NUM (5)
#define MIN_LR_TX_POWER_DBM (-10)

#define TX_RX_TURNAROUND_TIME 0

typedef enum
{
  ENERGY_DETECTION                       = 0,
  CHARRIER_SENSING                       = 1,
  ENERGY_DETECTION_AND_CHARRIER_SENSING  = 2,
  ENERGY_DETECTION_OR_CHARRIER_SENSING   = 3
} cca_mode_t;

typedef enum _zwave_lbt_mode_e
{
  AVERAGE_RSSI                          = 0x00,
  PEAK_RSSI                             = 0x01
} zwave_lbt_mode_e;

typedef enum _transmission_algorithm_e
{
  DIRECT_TRANSMISSION                   = 0x00,
  NONBEACON_MODE_CSMACA                 = 0x01
} transmission_algorithm_e;

typedef enum _zwave_t2r_mode_e
{
  AUTO_T2R_DISABLE                      = 0x00,
  AUTO_T2R_ENABLE                       = 0x01
} zwave_T2R_mode_e;

typedef struct _zw_protocol_mode_rf_setup_
{
  uint32_t channel_frequency[4];
  uint8_t channel_datarate[4];
  uint8_t filter_type[4];
  uint8_t frc_type[4];
  uint8_t preamble_len[4];
  uint8_t band_type;
  uint8_t use_fixed_rx_channel;
  uint8_t fixed_rx_channel;
  uint8_t channel_scan_mode;
} zw_protocol_mode_rf_setup_t;

/**************************************************************************************************
 *    Global Prototypes
 *************************************************************************************************/
void zwave_radio_get_network_ids(uint32_t *home_id, node_id_t *node_id, zpal_radio_mode_t *mode, uint8_t *home_id_hash);
void zwave_radio_set_network_ids(uint32_t home_id, node_id_t node_id, zpal_radio_mode_t mode, uint8_t home_id_hash);
void zwave_radio_network_id_filter_set(bool enable);
bool zwave_radio_network_id_filter_enabled_get(void);
bool zwave_radio_network_filter_enable(bool enable);
void zwave_radio_init(zpal_radio_profile_t * const profile, const zw_region_rf_setup_t *p_current_region_setup);
zpal_status_t zwave_radio_transmit(zpal_radio_transmit_parameter_t const *const tx_parameters,
                                uint8_t frame_header_length,
                                uint8_t const *const frame_header_buffer,
                                uint8_t frame_payload_length,
                                uint8_t const *const frame_payload_buffer,
                                uint8_t use_lbt,
                                int8_t tx_power);
zpal_status_t zwave_radio_transmit_beam(zpal_radio_transmit_parameter_t const *const tx_parameters,
                                     uint8_t beam_data_len,
                                     uint8_t const *const beam_data,
                                     int8_t tx_power);
bool zwave_radio_is_receive_started(void);
void zwave_radio_start_receive(bool force_rx);
void zwave_radio_idle(void);
void zwave_radio_forced_idle(void);
void zwave_radio_set_lbt_level(uint8_t channel, int8_t level);
int8_t zwave_radio_get_lbt_level(uint8_t channel);
void zwave_radio_abort(void);
bool zwave_radio_is_lbt_enabled(void);
zpal_radio_zwave_channel_t zwave_radio_get_last_beam_channel(void);
int8_t zwave_radio_get_last_beam_tx_power(void);
int8_t zwave_radio_get_last_beam_rssi(void);
uint8_t zwave_radio_get_last_beam_data(uint8_t* p_last_beam_data, uint8_t max_beam_data);
node_id_t zwave_radio_get_last_beam_node_id(void);
void zwave_radio_set_rx_fixed_channel(uint8_t enable_fixed_channel, uint8_t channel);
uint32_t zwave_radio_get_beam_count(void);
uint32_t zwave_radio_get_current_beam_count(void);
uint32_t zwave_radio_stop_current_beam_count(void);
uint32_t zwave_radio_get_last_stop_beam_count(void);
void zwave_radio_rx_broadcast_beam_enable(bool enable);
bool zwave_radio_rx_broadcast_beam_is_enabled(void);
zpal_radio_zwave_channel_t zwave_radio_get_last_tx_channel(void);

bool zwave_radio_rx_frame_fifo_pop(zpal_radio_receive_frame_t* p_rx_frame, zpal_radio_rx_parameters_t* p_rx_parameters);
uint32_t zwave_radio_rx_frame_fifo_count(void);

uint8_t convert_zpal_channel_to_phy_channel(zpal_radio_protocol_mode_t protocol_mode, zpal_radio_zwave_channel_t zpal_channel);
zpal_radio_zwave_channel_t convert_phy_channel_to_zpal_channel(zpal_radio_protocol_mode_t protocol_mode, uint8_t channel);
uint8_t convert_phy_scan_stage_to_phy_channel(zpal_radio_protocol_mode_t protocol_mode, uint8_t scan_stage);

void radio_rf_channel_statistic_tx_channel_set(zpal_radio_zwave_channel_t zwavechannel);
zpal_radio_zwave_channel_t radio_rf_channel_statistic_tx_channel_get(void);

int8_t radio_phy_tx_power_set(uint8_t channel, uint8_t band_type, int8_t tx_power);
void radio_network_statistics_update_tx_time(void);
void radio_update_transmit_time_phy_channel(uint8_t phy_channel);

bool zwave_radio_is_transmit_allowed(uint8_t channel, uint8_t frame_length, uint8_t frame_priority);

int8_t zwave_radio_rssi_read(uint8_t channel, bool *is_rssi_valid);
bool zwave_radio_rssi_read_all_channels(int8_t *average_rssi, uint8_t average_rssi_size);
void zwave_radio_rssi_config_set(uint16_t rssi_sample_frequency, uint8_t rssi_sample_count_average);
void zwave_radio_rssi_config_get(uint16_t *rssi_sample_frequency, uint8_t *rssi_sample_count_average);
zpal_status_t zwave_radio_tx_continues_wave_set(bool enable, zpal_radio_zwave_channel_t channel_id, int8_t power, zpal_radio_wave_type_t wave_type);
zpal_status_t zwave_radio_rf_debug_set(bool rf_state_enable);

void zwave_radio_chip_get_version(uint16_t *chip_id, uint16_t *chip_rev);
uint32_t zwave_radio_version_get(void);

int8_t zwave_radio_min_tx_power_get(uint8_t channel_id);
int8_t zwave_radio_max_tx_power_get(uint8_t channel_id);
bool zwave_radio_tx_power_config(zpal_tx_power_t tx_power_max);

bool zwave_radio_is_hw_setup_for_20dbm(void);
zpal_tx_power_t zwave_radio_classic_tx_power_set(zpal_tx_power_t tx_power);
zpal_tx_power_t zwave_radio_classic_tx_power_get(void);

void zwave_radio_power_conversion_20dbm(bool set_20dbm_conversion);
void zwave_radio_initialize(void);
void zwave_radio_channel_scan_setup(bool enable);
void zwave_radio_fw_preload(void);
void zwave_radio_enable_flirs_mode(void);
void zwave_radio_restart_after_flirs_mode(void);
void zwave_radio_clear_isr_events(void);
#endif
