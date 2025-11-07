// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file ZW_main_region.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include "ZW_lib_defines.h"

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <ZW_main.h>
#include <ZW_main_region.h>
#include <ZW_DataLinkLayer.h>
#include <ZW_Channels.h>
#include <zpal_radio_utils.h>
#ifdef ZW_CONTROLLER
#include <ZW_controller_network_info_storage.h>
#include "ZW_controller.h"
#endif
#ifdef ZW_SLAVE
#include <ZW_slave_network_info_storage.h>
#include "ZW_slave.h"
#endif
#include "ZW_basis.h"
#include <zpal_radio.h>

bool ProtocolChangeLrChannelConfig(zpal_radio_lr_channel_config_t eLrChCfg)
{
  if ( false == llChangeLrChannelConfig(eLrChCfg) )
  {
    // Failed - Set changeRfPHY notification
    // maybe set max number of retries and then do a SW reset which then sets the correct RF PHY
    ProtocolChangeRfPHYNotify();
    return false;
  }
  return true;
}

uint8_t
ProtocolLongRangeChannelGet(void)
{
 /*The first  4-bits are the current active channel number.
   The last 4-bit are status bits:
    bit4: true if auto channel supported else false
    bit5 true if auot cahnnel active, else false
    bit 6 and 7: reserved
 */
  uint8_t result = (uint8_t)zpal_radio_get_primary_long_range_channel();
  result |= LR_AUTO_CHANNEL_SUPPORTED;
  if (zpal_radio_get_long_range_channel_auto_mode()) {
    result |= LR_AUTO_CHANNEL_ACTIVE;
  }
  return result;
}

bool
ProtocolLongRangeChannelSet(zpal_radio_lr_channel_t channelId)
{
#ifdef ZW_CONTROLLER
  bool status = false;
  if (zpal_radio_protocol_mode_supports_long_range(zpal_radio_get_protocol_mode())) {
    bool AutoModeEnabled = false;
    if (((ZPAL_RADIO_LR_CHANNEL_A == channelId) ||
         (ZPAL_RADIO_LR_CHANNEL_B == channelId)) &&
        (zpal_radio_get_primary_long_range_channel() != channelId)) {
      StorageSetPrimaryLongRangeChannelId(channelId);
      zpal_radio_set_primary_long_range_channel(channelId); //update primary channel in current_profile
      status = ProtocolChangeLrChannelConfig( ZW_LrChannelConfigToUse( zpal_radio_get_rf_profile() ) );
    } else if (ZPAL_RADIO_LR_CHANNEL_AUTO == channelId) {
      AutoModeEnabled = true;
      status = true;
    }
    StorageSetLongRangeChannelAutoMode(AutoModeEnabled);
    zpal_radio_set_long_range_channel_auto_mode(AutoModeEnabled);

  }
  return status;
#endif
#ifdef ZW_SLAVE
 if (zpal_radio_protocol_mode_supports_long_range(zpal_radio_get_protocol_mode()) &&
      ((ZPAL_RADIO_LR_CHANNEL_A == channelId) || (ZPAL_RADIO_LR_CHANNEL_B == channelId)) &&
      (zpal_radio_get_primary_long_range_channel() != channelId))
  {
    StorageSetPrimaryLongRangeChannelId(channelId);
    zpal_radio_set_primary_long_range_channel(channelId);
    return ProtocolChangeLrChannelConfig( ZW_LrChannelConfigToUse( zpal_radio_get_rf_profile() ) );
  }
  return false;
#endif
}

bool
ProtocolZWaveLongRangeChannelSet(uint8_t channelId)
{
  if (zpal_radio_protocol_mode_supports_long_range(zpal_radio_get_protocol_mode()))
  {
    zpal_radio_lr_channel_t channelIdToSet = ZW_RadioConvertChannelIdToLongRangeChannel(channelId);

    return ProtocolLongRangeChannelSet(channelIdToSet);
  }
  return true;
}