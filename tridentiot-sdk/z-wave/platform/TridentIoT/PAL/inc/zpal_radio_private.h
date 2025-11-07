/// ****************************************************************************
/// @file zpal_radio_private.h
///
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************

#ifndef ZPAL_RADIO_PRIVATE_H_
#define ZPAL_RADIO_PRIVATE_H_

#include <stdint.h>
#include "zpal_radio.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup zpal
 * @brief
 * Zwave Platform Abstraction Layer.
 * @{
 * @addtogroup zpal-radio
 * @brief
 * Platform specific extension of ZPAL Radio API
 * @{
 */

typedef enum
{
  ZPAL_RADIO_WAVE_TYPE_UNMODULATED = 0,         ///< Unmodulated wave type
  ZPAL_RADIO_WAVE_TYPE_MODULATED = 1,           ///< Modulated fixed byte pattern wave type
  ZPAL_RADIO_WAVE_TYPE_MODULATED_WHITENING = 2, ///< Modulated fixed byte pattern with whitening wave type
  ZPAL_RADIO_WAVE_TYPE_COUNT                    ///< Count valid wave types
} zpal_radio_wave_type_t;

/**
 * @brief Get last receive beam data
 * Retrieve the last received beam data
 *
 * @param[out] last_beam_data Pointer to array where last beam data should be copied.
 * @param[in]  max_beam_data  Maximum number of bytes last_beam_data can hold.
 *
 * @return Number of bytes copied to last_beam_data array
 */
uint8_t zpal_radio_get_last_beam_data(uint8_t *last_beam_data, uint8_t max_beam_data);

void zpal_radio_set_rx_fixed_channel(uint8_t enable, uint8_t channel);

uint8_t zpal_radio_region_channel_count_get(void);

uint32_t zpal_radio_get_beam_count(void);

uint32_t zpal_radio_get_current_beam_count(void);

uint32_t zpal_radio_stop_current_beam_count(void);

uint32_t zpal_radio_get_last_stop_beam_count(void);

zpal_radio_zwave_channel_t zpal_radio_get_last_tx_channel(void);

uint8_t zpal_radio_zpal_channel_to_internal(zpal_radio_zwave_channel_t zwave_channel_id);

int8_t zpal_radio_get_lbt_level(uint8_t channel);

zpal_status_t zpal_radio_tx_continues_wave_set(bool enable, zpal_radio_zwave_channel_t channel, int8_t power, zpal_radio_wave_type_t wave_type);

zpal_status_t zpal_radio_rf_debug_set(bool rf_state_enable);

void zpal_radio_rf_debug_reg_setting_list(bool listallreg);

int8_t zpal_radio_rssi_read(uint8_t channel, bool *is_rssi_valid);

bool zpal_radio_rssi_read_all_channels(int8_t *average_rssi, uint8_t average_rssi_size);

void zpal_radio_rssi_config_set(uint16_t rssi_sample_frequency, uint8_t rssi_sample_count_average);

void zpal_radio_rssi_config_get(uint16_t *rssi_sample_frequency, uint8_t *rssi_sample_count_average);

void zpal_radio_chip_get_version(uint16_t *chip_id, uint16_t *chip_rev);

uint32_t zpal_radio_version_get(void);

int8_t zpal_radio_min_tx_power_get(uint8_t channel_id);

int8_t zpal_radio_max_tx_power_get(uint8_t channel_id);

bool zpal_radio_classic_tx_power_is_enabled(void);

zpal_tx_power_t zpal_radio_set_classic_tx_power(zpal_tx_power_t tx_power);

zpal_tx_power_t zpal_radio_get_classic_tx_power(void);

zpal_tx_power_t zpal_radio_set_classic_tx_power_adjust(zpal_tx_power_t tx_power_adjust);

zpal_tx_power_t zpal_radio_get_classic_tx_power_adjust(void);

uint8_t zpal_radio_set_classic_tx_power_attenuation(uint8_t attenuation);

uint8_t zpal_radio_get_classic_tx_power_attenuation(void);

bool zpal_radio_tx_power_power_index_set(uint8_t channel, int8_t power, uint8_t power_index);

bool zpal_radio_dBm_to_power_index_table_get(uint8_t channel, uint8_t const * *power_index_table);

void zpal_radio_get_network_ids(uint32_t *home_id, node_id_t *node_id, zpal_radio_mode_t *mode, uint8_t *home_id_hash);

void zpal_radio_fw_preload(void);

bool zpal_radio_tx_power_config(zpal_tx_power_t tx_power_max);

bool zpal_radio_network_filter_enable(bool enable);

bool zpal_radio_network_id_filter_enabled_get(void);

/**
 * @brief shutdown the radio transceiver.
 */
void zpal_radio_power_shutdown(void);

/**
 * @} //zpal-radio
 * @} //zpal
 */

#ifdef __cplusplus
}
#endif

#endif /* ZPAL_RADIO_PRIVATE_H_ */
