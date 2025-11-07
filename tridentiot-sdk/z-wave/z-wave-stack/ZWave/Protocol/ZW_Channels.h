// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file
 * @copyright 2022 Silicon Laboratories Inc.
 */
#ifndef ZWAVE_PROTOCOL_ZW_CHANNELS_H_
#define ZWAVE_PROTOCOL_ZW_CHANNELS_H_

#include <ZW_Frame.h>
#include <zpal_radio.h>

/**@brief Function to convert Z-Wave channelId to Long Range ChannelId
 *
 * Converts specified Z-Wave ChannelId to Long Range ChannelId if applicable
 *
 * @return ZPAL_RADIO_LR_CHANNEL_A, ZPAL_RADIO_LR_CHANNEL_B or ZPAL_RADIO_LR_CHANNEL_UNKNOWN
 */
zpal_radio_lr_channel_t
ZW_RadioConvertChannelIdToLongRangeChannel(uint8_t channelId);

/**@brief Function to get ChannelID for LR nodes based on the communicationProfile
 *
 * @return  channelID for LR node
 */
uint8_t
ZW_GetChannelIndexLR(CommunicationProfile_t communicationProfile, zpal_radio_lr_channel_config_t eLrChCfg);

#endif /* ZWAVE_PROTOCOL_ZW_CHANNELS_H_ */
