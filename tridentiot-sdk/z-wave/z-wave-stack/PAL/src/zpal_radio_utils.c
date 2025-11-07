// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file
 *
 * Description: PAL radio utilities
 *
 * @copyright 2021 Silicon Laboratories Inc.
 */

#include <zpal_radio_utils.h>
#include "Assert.h"

zpal_radio_protocol_mode_t
zpal_radio_region_get_protocol_mode(uint32_t region, zpal_radio_lr_channel_config_t eLrChCfg)
{
  if ((REGION_DEPRECATED_4 == region)
  ||  (REGION_DEPRECATED_10 == region)
  ||  (REGION_DEPRECATED_48 == region))
  {
    //deprecated region should no longer be used
    ASSERT(false);
    return ZPAL_RADIO_PROTOCOL_MODE_UNDEFINED;
  }
  else if ( (false == zpal_radio_region_is_long_range(region)) && (ZPAL_RADIO_LR_CH_CFG_NO_LR != eLrChCfg) )
  {
    //non LR region does not support channel configuration other than NO_LR
    ASSERT(false);
    return ZPAL_RADIO_PROTOCOL_MODE_UNDEFINED;
  }
  /* (REGION_2CH_FIRST <= region) is commented because REGION_2CH_FIRST==0. So the test is always
  true, which create a warning.*/
  else if ( /*(REGION_2CH_FIRST <= region) &&*/ (region < REGION_2CH_END) )
  {
    switch (eLrChCfg)
    {
      case ZPAL_RADIO_LR_CH_CFG_NO_LR:  return ZPAL_RADIO_PROTOCOL_MODE_1;
      case ZPAL_RADIO_LR_CH_CFG1:       return ZPAL_RADIO_PROTOCOL_MODE_3;
      case ZPAL_RADIO_LR_CH_CFG2:       return ZPAL_RADIO_PROTOCOL_MODE_3;
      case ZPAL_RADIO_LR_CH_CFG3:       return ZPAL_RADIO_PROTOCOL_MODE_4;
      default:                          return ZPAL_RADIO_PROTOCOL_MODE_UNDEFINED;
    }
  }
  else if ( (REGION_3CH_FIRST <= region) && (region < REGION_3CH_END) )
  {
    return ZPAL_RADIO_PROTOCOL_MODE_2;
  }
  ASSERT(false);
  return ZPAL_RADIO_PROTOCOL_MODE_UNDEFINED;
}

bool
zpal_radio_protocol_mode_supports_long_range(zpal_radio_protocol_mode_t mode)
{
  return (((ZPAL_RADIO_PROTOCOL_MODE_3 == mode) || (ZPAL_RADIO_PROTOCOL_MODE_4 == mode)) ? true : false);
}

zpal_radio_region_t
zpal_radio_get_valid_region(zpal_radio_region_t region)
{
  if (REGION_DEFAULT == region) {
    return REGION_EU;
  }
  return region;
}

bool
zpal_radio_region_is_long_range(zpal_radio_region_t region)
{
  if ( (REGION_US_LR == region) || (REGION_EU_LR == region) )
  {
    return true;
  }
  return false;
}
