// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file ZW_main_region.h
 * @copyright 2022 Silicon Laboratories Inc.
 */
#ifndef ZWAVE_ZW_MAIN_REGION_H_
#define ZWAVE_ZW_MAIN_REGION_H_

#include <zpal_radio.h>
#define LR_AUTO_CHANNEL_SUPPORTED   0x10
#define LR_AUTO_CHANNEL_ACTIVE      0x20

/**
* Change Long Range Channel configuration
* @param[in]    eLrChCfg Long Range channel configuration to change to
*
* @return       TRUE Long Range channel configuration changed to eLrChCfg
*/
bool ProtocolChangeLrChannelConfig(zpal_radio_lr_channel_config_t eLrChCfg);

/**
* Set current Long Range Channel if applicable.
* @param[in]    channelId Z-Wave ChannelId to set as primary Long Range Channel
*
* @return       TRUE primary Long Range Channel set
*/
bool
ProtocolZWaveLongRangeChannelSet(uint8_t channelId);

/**
* Set current Long Range Channel if applicable.
* @param[in]    channelId Long Range Channel to set as primary Long Range Channel
*
* @return       TRUE primary Long Range Channel set
*/
bool
ProtocolLongRangeChannelSet(zpal_radio_lr_channel_t channelId);

/**
* Get current Long Range Channel if applicable.
*
* @return       ZPAL_RADIO_LR_CHANNEL_A, ZPAL_RADIO_LR_CHANNEL_B or LONG_RANGE_CHANNEL_UNKNOWN
*/
uint8_t
ProtocolLongRangeChannelGet(void);
#endif /* ZWAVE_ZW_MAIN_REGION_H_ */