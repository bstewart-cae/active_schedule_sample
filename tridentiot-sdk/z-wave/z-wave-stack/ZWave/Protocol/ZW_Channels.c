// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file
 *
 * Description: Functions related to Protocol Mode
 *
 * @copyright 2021 Silicon Laboratories Inc.
 */

#include <ZW_Channels.h>

//TODO: move to api?
#define CHANNEL_NO_0                      0  // Z-Wave channel for 2CH/3CH channel
#define CHANNEL_NO_1                      1  // Z-Wave channel for 2CH/3CH channel
#define CHANNEL_NO_2                      2  // Z-Wave channel for 3CH channel
#define CHANNEL_NO_3                      3  // Z-Wave channel for LR channel A (Any one of these LR channels can become primary channel)
#define CHANNEL_NO_4                      4  // Z-Wave channel for LR channel B (backup) (Secondary to begin with)
#define CHANNEL_NOT_VALID                 255

#define CHANNEL_NO_9600                   CHANNEL_NO_2
#define CHANNEL_NO_40K                    CHANNEL_NO_1
#define CHANNEL_NO_100K_2CH               CHANNEL_NO_0

#define CHANNEL_NO_100K_LR                CHANNEL_NO_3

#define CHANNEL_NO_100K_LR1               CHANNEL_NO_0
#define CHANNEL_NO_100K_LR2               CHANNEL_NO_1


zpal_radio_lr_channel_t
ZW_RadioConvertChannelIdToLongRangeChannel(uint8_t channelId)
{
  if (CHANNEL_NO_3 == channelId)
  {
    return ZPAL_RADIO_LR_CHANNEL_A;
  }
  if (CHANNEL_NO_4 == channelId)
  {
    return ZPAL_RADIO_LR_CHANNEL_B;
  }
  return ZPAL_RADIO_LR_CHANNEL_UNKNOWN;
}

uint8_t
ZW_GetChannelIndexLR(CommunicationProfile_t communicationProfile, zpal_radio_lr_channel_config_t eLrChCfg)
{
    uint8_t channelIndex = CHANNEL_NO_100K_LR;
    if ( ZPAL_RADIO_LR_CH_CFG3 == eLrChCfg )
    {
      channelIndex = ( RF_PROFILE_100K_LR_A == communicationProfile )? CHANNEL_NO_100K_LR1 : CHANNEL_NO_100K_LR2;
    }

    return channelIndex;
}
