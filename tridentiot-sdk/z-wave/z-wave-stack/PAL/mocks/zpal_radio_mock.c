// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file zpal_radio_mock.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include <mock_control.h>
#include <zpal_radio.h>

#define MOCK_FILE "zpal_radio_mock.c"

//Default min/max tx power in dBm  // These needs to be updated each time the max power is changed.
#define DEFAULT_TX_POWER_MIN        -6
#define DEFAULT_TX_POWER_DEFAULT    0
#define DEFAULT_TX_POWER_MAX        14

#define MOCK_CALL_COMPARE_INPUT_STRUCT_TX_PARAMETERS(P_MOCK, ARGUMENT, P_ACTUAL) do {                 \
  MOCK_CALL_COMPARE_STRUCT_MEMBER_UINT8(P_MOCK, ARGUMENT, P_ACTUAL, zpal_radio_transmit_parameter_t, speed);         \
  MOCK_CALL_COMPARE_STRUCT_MEMBER_UINT8(P_MOCK, ARGUMENT, P_ACTUAL, zpal_radio_transmit_parameter_t, channel_id);     \
  MOCK_CALL_COMPARE_STRUCT_MEMBER_UINT8(P_MOCK, ARGUMENT, P_ACTUAL, zpal_radio_transmit_parameter_t, crc);           \
  MOCK_CALL_COMPARE_STRUCT_MEMBER_UINT8(P_MOCK, ARGUMENT, P_ACTUAL, zpal_radio_transmit_parameter_t, preamble_length);\
  MOCK_CALL_COMPARE_STRUCT_MEMBER_UINT8(P_MOCK, ARGUMENT, P_ACTUAL, zpal_radio_transmit_parameter_t, repeats);       \
  } while (0)


zpal_radio_profile_t rfProfile =
{
  .region = REGION_EU,
  .wakeup = ZPAL_RADIO_WAKEUP_ALWAYS_LISTEN,
  .primary_lr_channel = ZPAL_RADIO_LR_CHANNEL_UNINITIALIZED,
  .lr_channel_auto_mode = false,
  .active_lr_channel_config = ZPAL_RADIO_LR_CH_CFG_NO_LR,
  .listen_before_talk_threshold = 127, //ELISTENBEFORETALKTRESHOLD_DEFAULT,
  .tx_power_max = 0, //APP_MAX_TX_POWER,
  .tx_power_adjust = 00, //APP_MEASURED_0DBM_TX_POWER,
  .tx_power_max_lr = 140, //APP_MAX_TX_POWER_LR,
  .rx_cb = 0,
  .tx_cb = 0,
  .region_change_cb = 0,
  .assert_cb = 0,
  .network_stats = 0,
  .radio_debug_enable = 0,
  .receive_handler_cb = 0,
};


void zpal_radio_set_network_ids(uint32_t home_id, node_id_t node_id, zpal_radio_mode_t mode, uint8_t home_id_hash)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(p_mock);

  MOCK_CALL_ACTUAL(p_mock, home_id, node_id, mode);

  MOCK_CALL_COMPARE_INPUT_UINT32(p_mock, ARG0, home_id);
  MOCK_CALL_COMPARE_INPUT_UINT32(p_mock, ARG1, node_id);
  MOCK_CALL_COMPARE_INPUT_UINT32(p_mock, ARG2, mode);
  MOCK_CALL_COMPARE_INPUT_UINT32(p_mock, ARG3, home_id_hash);
}

void zpal_radio_init(zpal_radio_profile_t * const p_profile)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(p_mock);
  MOCK_CALL_ACTUAL(p_mock, p_profile);

  MOCK_CALL_COMPARE_INPUT_POINTER(p_mock, ARG0, p_profile);
}

zpal_status_t zpal_radio_change_region(zpal_radio_region_t region, zpal_radio_lr_channel_config_t eLrChCfg)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(ZPAL_STATUS_FAIL);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, ZPAL_STATUS_FAIL);
  MOCK_CALL_RETURN_IF_ERROR_SET(p_mock, zpal_status_t);

  MOCK_CALL_COMPARE_INPUT_UINT32(p_mock, ARG0, region);
  MOCK_CALL_COMPARE_INPUT_UINT32(p_mock, ARG1, eLrChCfg);

  MOCK_CALL_RETURN_VALUE(p_mock, zpal_status_t);
}

zpal_radio_region_t zpal_radio_get_region(void)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(REGION_EU);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(pMock, REGION_UNDEFINED);

  MOCK_CALL_RETURN_VALUE(pMock, zpal_radio_region_t);
}

zpal_radio_lr_channel_config_t zpal_radio_get_lr_channel_config(void)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(ZPAL_RADIO_LR_CH_CFG_NO_LR);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(pMock, ZPAL_RADIO_LR_CH_CFG_COUNT);

  MOCK_CALL_RETURN_VALUE(pMock, zpal_radio_lr_channel_config_t);
}

const zpal_radio_profile_t * zpal_radio_get_rf_profile(void)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(&rfProfile);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(pMock, 0);

  MOCK_CALL_RETURN_VALUE(pMock, zpal_radio_profile_t*);
}

zpal_status_t
zpal_radio_transmit(zpal_radio_transmit_parameter_t const * const pTxParameters,
                    uint8_t frameHeaderLength,
                    uint8_t const * const frameHeaderBuffer,
                    uint8_t framePayloadLength,
                    uint8_t const * const framePayloadBuffer,
                    uint8_t useLBT,
                    int8_t txPower)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(ZPAL_STATUS_OK);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, ZPAL_STATUS_OK);

  MOCK_CALL_ACTUAL(p_mock, pTxParameters, frameHeaderLength, frameHeaderBuffer, framePayloadLength, framePayloadBuffer);

  MOCK_CALL_COMPARE_INPUT_STRUCT_TX_PARAMETERS(p_mock, ARG0, pTxParameters);
  MOCK_CALL_COMPARE_INPUT_UINT8(p_mock, ARG1, frameHeaderLength);
  MOCK_CALL_COMPARE_INPUT_UINT8(p_mock, ARG3, framePayloadLength);
  MOCK_CALL_COMPARE_INPUT_UINT8(p_mock, ARG5, useLBT);
  MOCK_CALL_COMPARE_INPUT_UINT8(p_mock, ARG6, txPower);

  MOCK_CALL_RETURN_VALUE(p_mock, zpal_status_t);
}

zpal_status_t
zpal_radio_transmit_beam(zpal_radio_transmit_parameter_t const * const pTxParameters,
                         uint8_t beamDataLen,
                         uint8_t const * const pBeamData,
                         int8_t txPower)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(ZPAL_STATUS_OK);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, ZPAL_STATUS_OK);

  MOCK_CALL_ACTUAL(p_mock, pTxParameters, beamDataLen, pBeamData);

  MOCK_CALL_COMPARE_INPUT_STRUCT_TX_PARAMETERS(p_mock, ARG0, pTxParameters);
  MOCK_CALL_COMPARE_INPUT_UINT8(p_mock, ARG1, beamDataLen);
  MOCK_CALL_COMPARE_INPUT_UINT8(p_mock, ARG3, txPower);

  MOCK_CALL_RETURN_VALUE(p_mock, zpal_status_t);
}

zpal_radio_protocol_mode_t
zpal_radio_get_protocol_mode(void)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(ZPAL_RADIO_PROTOCOL_MODE_2);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, ZPAL_RADIO_PROTOCOL_MODE_UNDEFINED);
  MOCK_CALL_RETURN_VALUE(p_mock, zpal_radio_protocol_mode_t);
}

int8_t
zpal_radio_get_last_beam_rssi(void)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(0x00);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(pMock, INT8_MAX);

  MOCK_CALL_RETURN_VALUE(pMock, int8_t);
}

void zpal_radio_enable_rx_broadcast_beam(bool enable)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(pMock);

  MOCK_CALL_ACTUAL(pMock, enable);
  MOCK_CALL_COMPARE_INPUT_BOOL(pMock, ARG0, enable);
}

zpal_status_t zpal_radio_get_background_rssi(uint8_t channel, int8_t *rssi)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(ZPAL_STATUS_FAIL);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(pMock, ZPAL_STATUS_FAIL);

  MOCK_CALL_ACTUAL(pMock, channel, rssi);
  MOCK_CALL_COMPARE_INPUT_UINT8(pMock, ARG0, channel);
  MOCK_CALL_COMPARE_INPUT_POINTER(pMock, ARG1, rssi);
  MOCK_CALL_SET_OUTPUT_ARRAY(pMock->output_arg[1].p, rssi, 1, int8_t);
  MOCK_CALL_RETURN_VALUE(pMock, zpal_status_t);
}

uint8_t zpal_radio_get_reduce_tx_power(void)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(0x00);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(pMock, 0x00);

  MOCK_CALL_RETURN_VALUE(pMock, uint8_t);
}

bool zpal_radio_is_lbt_enabled(void)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(true);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(pMock, false);

  MOCK_CALL_RETURN_VALUE(pMock, bool);
}

uint16_t zpal_radio_get_beam_startup_time(void)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(0);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(pMock, 0);

  MOCK_CALL_RETURN_VALUE(pMock, uint16_t);
}

zpal_tx_power_t zpal_radio_get_minimum_lr_tx_power(void)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(DEFAULT_TX_POWER_MIN);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(pMock, DEFAULT_TX_POWER_MIN);

  MOCK_CALL_RETURN_VALUE(pMock, zpal_tx_power_t);
}

zpal_tx_power_t zpal_radio_get_maximum_lr_tx_power(void)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(DEFAULT_TX_POWER_MAX);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(pMock, DEFAULT_TX_POWER_MAX);

  MOCK_CALL_RETURN_VALUE(pMock, zpal_tx_power_t);
}

zpal_tx_power_t
zpal_radio_get_default_tx_power(void)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(DEFAULT_TX_POWER_DEFAULT);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(pMock, DEFAULT_TX_POWER_DEFAULT);

  MOCK_CALL_RETURN_VALUE(pMock, zpal_tx_power_t);
}

zpal_radio_lr_channel_t zpal_radio_get_primary_long_range_channel(void)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(ZPAL_RADIO_LR_CHANNEL_A);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, ZPAL_RADIO_LR_CHANNEL_UNKNOWN);
  MOCK_CALL_RETURN_VALUE(p_mock, zpal_radio_lr_channel_t);
}

void zpal_radio_set_primary_long_range_channel(zpal_radio_lr_channel_t channel)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(p_mock);

  MOCK_CALL_ACTUAL(p_mock, channel);
  MOCK_CALL_COMPARE_INPUT_UINT8(p_mock, ARG0, channel);
}

bool zpal_radio_is_long_range_locked(void)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(false);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(pMock, false);

  MOCK_CALL_RETURN_VALUE(pMock, bool);
}

void zpal_radio_set_long_range_lock(bool lock)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(pMock);

  MOCK_CALL_ACTUAL(pMock, lock);
  MOCK_CALL_COMPARE_INPUT_BOOL(pMock, ARG0, lock);
}

int8_t zpal_radio_get_flirs_beam_tx_power(void)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(0x00);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, 0x00);

  MOCK_CALL_RETURN_VALUE(p_mock, int8_t);
}

bool zpal_radio_is_transmit_allowed(uint8_t channel, uint8_t frame_length, uint8_t frame_priority)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(false);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(pMock, false);
  MOCK_CALL_ACTUAL(pMock, channel, frame_length, frame_priority);
  MOCK_CALL_COMPARE_INPUT_UINT8(pMock, ARG0, channel);
  MOCK_CALL_COMPARE_INPUT_UINT8(pMock, ARG1, frame_length);
  MOCK_CALL_COMPARE_INPUT_UINT8(pMock, ARG2, frame_priority);
  MOCK_CALL_RETURN_VALUE(pMock, bool);
}

bool zpal_radio_attenuate(uint8_t adjust_tx_power)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(false);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(pMock, false);
  MOCK_CALL_ACTUAL(pMock, adjust_tx_power);
  MOCK_CALL_COMPARE_INPUT_UINT8(pMock, ARG0, adjust_tx_power);

  MOCK_CALL_RETURN_VALUE(pMock, bool);
}

void zpal_radio_request_calibration(bool forced)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(pMock);

  MOCK_CALL_ACTUAL(pMock, forced);
  MOCK_CALL_COMPARE_INPUT_BOOL(pMock, ARG0, forced);
}

void
zpal_radio_set_long_range_channel_auto_mode(bool enable)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(pMock);

  MOCK_CALL_ACTUAL(pMock, enable);
  MOCK_CALL_COMPARE_INPUT_BOOL(pMock, ARG0, enable);
}

void
zpal_radio_rf_channel_statistic_clear(zpal_radio_zwave_channel_t zwavechannel)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(pMock);

  MOCK_CALL_ACTUAL(pMock, zwavechannel);
  MOCK_CALL_COMPARE_INPUT_UINT8(pMock, ARG0, zwavechannel);
}

bool
zpal_radio_rf_channel_statistic_get(zpal_radio_zwave_channel_t zwavechannel, zpal_radio_rf_channel_statistic_t* p_radio_channel_statistic)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(false);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(pMock, false);

  MOCK_CALL_ACTUAL(pMock, zwavechannel, p_radio_channel_statistic);
  MOCK_CALL_COMPARE_INPUT_UINT8(pMock, ARG0, zwavechannel);
  MOCK_CALL_COMPARE_INPUT_POINTER(pMock, ARG1, p_radio_channel_statistic);

  if(NULL != pMock->output_arg[ARG1].p)
  {
    MOCK_CALL_SET_OUTPUT_ARRAY(pMock->output_arg[1].p, p_radio_channel_statistic, 1, zpal_radio_rf_channel_statistic_t);
  }

  MOCK_CALL_RETURN_VALUE(pMock, bool);
}

void
zpal_radio_rf_channel_statistic_tx_frames(void)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(pMock);
}

void
zpal_radio_rf_channel_statistic_tx_retries(void)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(pMock);
}

void
zpal_radio_rf_channel_statistic_tx_lbt_failures(void)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(pMock);
}

void
zpal_radio_rf_channel_statistic_background_rssi_average_update(zpal_radio_zwave_channel_t zwavechannel, int8_t rssi)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(pMock);
  MOCK_CALL_ACTUAL(pMock, zwavechannel, rssi);
  MOCK_CALL_COMPARE_INPUT_UINT8(pMock, ARG0, zwavechannel);
  MOCK_CALL_COMPARE_INPUT_INT8(pMock, ARG1, rssi);
}

void
zpal_radio_rf_channel_statistic_end_device_rssi_average_update(zpal_radio_zwave_channel_t zwavechannel, int8_t rssi)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(pMock);
  MOCK_CALL_ACTUAL(pMock, zwavechannel, rssi);
  MOCK_CALL_COMPARE_INPUT_UINT8(pMock, ARG0, zwavechannel);
  MOCK_CALL_COMPARE_INPUT_INT8(pMock, ARG1, rssi);
}

bool zpal_radio_is_region_supported(zpal_radio_region_t region)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(true);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(pMock, false);
  MOCK_CALL_ACTUAL(pMock, region);
  MOCK_CALL_COMPARE_INPUT_UINT8(pMock, ARG0, region);

  MOCK_CALL_RETURN_VALUE(pMock, bool);
}

void zpal_radio_network_id_filter_set(bool enable)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(pMock);

  MOCK_CALL_ACTUAL(pMock, enable);
  MOCK_CALL_COMPARE_INPUT_BOOL(pMock, ARG0, enable);
}

