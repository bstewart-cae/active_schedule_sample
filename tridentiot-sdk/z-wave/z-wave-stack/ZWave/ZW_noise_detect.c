// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file
 *
 * RF noise and jamming detection.
 *
 * This module estimates the background noise while the radio is in
 * receive mode. It provides support for implementing jamming detect in
 * the application.
 *
 * @copyright 2018 Silicon Laboratories Inc.
 */

#include "ZW_lib_defines.h"

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <ZW_typedefs.h>
#include <zpal_radio.h>
#include <ZW_MAC.h>
#include <ZW_basis_api.h>
#include <ZW_DataLinkLayer.h>
#include <string.h>
#include <zpal_radio_utils.h>

//#define DEBUGPRINT
#include "DebugPrint.h"
/****************************************************************************/
/*                      PRIVATE TYPES and DEFINITIONS                       */
/****************************************************************************/

/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/

/****************************************************************************/
/*                              PRIVATE DATA                                */
/****************************************************************************/

/**
 * Since the RSSI value is being used by some customers,
 * we cannot change how it is being calculated.
 */
#define NOISE_LEVEL_SAMPLES   8

/* the latest */
static int8_t sInternal_noise_levels[ZPAL_RADIO_NUM_CHANNELS_LR_CH_CFG_1_2][NOISE_LEVEL_SAMPLES];
static int8_t sInternal_noise_levels_last_sample[ZPAL_RADIO_NUM_CHANNELS_LR_CH_CFG_1_2];

static uint8_t bNextSample = 0;      // The next sample

/****************************************************************************/
/*                              PRIVATE FUNCTIONS                           */
/****************************************************************************/
/*======================  GetNumberOfChannels    ==========================
**
**  Return the number of channels based on type of node (ZWave LR or ZWave)
**  and status (node included/not included)
**
**  Side effects: None
**
**-----------------------------------------------------------------------*/
static uint8_t GetNumberOfChannels(void)
{
  uint8_t NumberOfChannels = ZPAL_RADIO_NUM_CHANNELS;

  // Check if it is a long range node - Two different checks depending on if the device is a controller or a slave device
#ifdef ZW_CONTROLLER
  if ( zpal_radio_protocol_mode_supports_long_range(zpal_radio_get_protocol_mode()) )
  {
    // If the device is a long range node, set channel number to four
    NumberOfChannels = ZPAL_RADIO_NUM_CHANNELS_LR_CH_CFG_1_2;
  }
#endif
#ifdef ZW_SLAVE
  if(zpal_radio_get_lr_channel_config() == ZPAL_RADIO_LR_CH_CFG3)
  {
    // If the device is an already included long range node, set channel number to ZPAL_RADIO_NUM_CHANNELS_LR_CH_CFG3
    NumberOfChannels = ZPAL_RADIO_NUM_CHANNELS_LR_CH_CFG3;
  }
  else
  {
    if ( (0 == g_nodeID) && zpal_radio_protocol_mode_supports_long_range(zpal_radio_get_protocol_mode()) )
    {
      // If the device is a node supporting LR, but not already included, set channel number to ZPAL_RADIO_NUM_CHANNELS_LR_CH_CFG_1_2
      NumberOfChannels = ZPAL_RADIO_NUM_CHANNELS_LR_CH_CFG_1_2;
    }
  }
#endif

  return NumberOfChannels;
}

/****************************************************************************/
/*                              EXPORTED FUNCTIONS                          */
/****************************************************************************/
/*============================   CalculateRssi   ===============================
**
**  Given a variable length array of raw rssi samples return the average rssi
**  value in dBms. This is typically used to turn a number of RSSI samples taken
**  during frame reception into a single value for the entire frame.
**
**  It is also used to convert background rssi samples into dBms.
**
**  PLEASE note that the result is a SIGNED char. Typical values are in
**  the range -100 to -40 (dBm).
**
**-------------------------------------------------------------------------*/
int8_t             /*RET Average RSSI value as specified in comment */
CalculateRssi(
  int8_t *rssi_array,     /* IN RSSI sample array taken during frame reception */
  uint8_t len)             /* IN Length of rssi_array*/
{
  uint8_t i;
  uint8_t n = 0;
  int16_t sum = 0;

  for (i = 0; i < len; i++)
  {
    if(ZPAL_RADIO_INVALID_RSSI_DBM != rssi_array[i])
    {
      sum += rssi_array[i];
      n++;
    }
  }

  if(0 < n)
  {
    sum = sum / n;
  }
  else
  {
    sum = ZPAL_RADIO_INVALID_RSSI_DBM;
  }

  return sum;
}

/*=====================  ZW_GetBackgroundRSSI    ========================
**
**  Obtain the current noise level samples
**
**  Side effects: None
**
**-------------------------------------------------------------------------*/
void
ZW_GetBackgroundRSSI(RSSI_LEVELS *noise_levels)
{
  uint8_t i, NumberOfChannels;

  NumberOfChannels = GetNumberOfChannels();
  for (i = 0; i < NumberOfChannels; i++)
  {
    noise_levels->rssi_dBm[i] = CalculateRssi(&(sInternal_noise_levels[i][0]), NOISE_LEVEL_SAMPLES);
  }
}

ZW_WEAK bool zpal_radio_rssi_read_all_channels(__attribute__((unused)) int8_t *average_rssi, __attribute__((unused)) uint8_t average_rssi_size)
{
  return false;
}

/*========================   SampleNoiseLevel   ============================
**
**  Sample instantaneous noise level if the receiver is currently
**  on the channel scheduled for sampling and update noise estimate.
**
**  Poll this function frequently, e.g. from main loop.
**
**  Side effects: It is possible to force RAIL into RX mode.
**
**-------------------------------------------------------------------------*/
void SampleNoiseLevel(void)
{
  uint8_t NumberOfChannels = GetNumberOfChannels();

  bool is_rssi_sample_valid = zpal_radio_rssi_read_all_channels(sInternal_noise_levels_last_sample, NumberOfChannels);
  if (is_rssi_sample_valid)
  {
    for (uint8_t channel = 0; channel < NumberOfChannels; channel++)
    {
      sInternal_noise_levels[channel][bNextSample] = sInternal_noise_levels_last_sample[channel];
    }
  }
  else
  {
    int8_t rssi_value = ZPAL_RADIO_INVALID_RSSI_DBM;
    for (uint8_t channel = 0 ;channel < NumberOfChannels; channel++)
    {
      const zpal_status_t status = zpal_radio_get_background_rssi(channel, &rssi_value);
      if (ZPAL_STATUS_OK == status) {
        is_rssi_sample_valid = true;
        sInternal_noise_levels[channel][bNextSample] = rssi_value;
      }
    }
  }
  if (is_rssi_sample_valid)
  {
    bNextSample = (bNextSample + 1) % NOISE_LEVEL_SAMPLES;
  }
}

/*========================   NoiseDetectInit   ============================
**
**  Set default function for noise detect module. Protocol must call
**  on every SetDefault, change of Radio Region and on power up.
**
**  Side effects: previously measured noise levels are deleted.
**
**-------------------------------------------------------------------------*/
void
NoiseDetectInit(void)
{
  uint8_t channel;
  uint8_t sample;

  bNextSample = 0;

  for (channel = 0 ;channel < ZPAL_RADIO_NUM_CHANNELS_LR_CH_CFG_1_2; channel++)
  {
    sInternal_noise_levels_last_sample[channel] = ZPAL_RADIO_INVALID_RSSI_DBM;
    for (sample = 0; sample < NOISE_LEVEL_SAMPLES; sample++)
    {
      sInternal_noise_levels[channel][sample] = ZPAL_RADIO_INVALID_RSSI_DBM;
    }
  }
}
