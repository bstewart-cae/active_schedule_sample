// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

#include "ZW_home_id_hash.h"
#include <ZW_MAC.h>
#include <ZW_transport_api.h>

uint8_t HomeIdHashCalculate(uint32_t homeId, node_id_t nodeId, zpal_radio_protocol_mode_t zwProtMode)
{
  uint32_t hash = homeId ^ 0xFF;

  hash ^= (hash >> 8);
  hash ^= (hash >> 16);
  hash &= 0xFF;

  if (ZPAL_RADIO_PROTOCOL_MODE_UNDEFINED != zwProtMode)
  {
    /* The value of preamble is not a valid hash value */
    if (ZPAL_RADIO_PROTOCOL_MODE_2 == zwProtMode)
    {
      switch (hash)
      {
        case HOME_ID_HASH_3CH_ILLEGAL1:
        case HOME_ID_HASH_3CH_ILLEGAL2:
        case HOME_ID_HASH_3CH_ILLEGAL3:
        case HOME_ID_HASH_3CH_ILLEGAL4:
        case HOME_ID_HASH_3CH_ILLEGAL5:
        case HOME_ID_HASH_3CH_INCREASED1:
        case HOME_ID_HASH_3CH_INCREASED2:
          {
            hash++;
          }
          break;

        default:
          break;
      }
    }
    else if ((ZPAL_RADIO_PROTOCOL_MODE_1 == zwProtMode) || ((ZPAL_RADIO_PROTOCOL_MODE_3 == zwProtMode) && (LOWEST_LONG_RANGE_NODE_ID > nodeId)))
    {
      // ZPAL_RADIO_PROTOCOL_MODE_1 & ZPAL_RADIO_PROTOCOL_MODE_3
      switch (hash)
      {
        case HOME_ID_HASH_2CH_ILLEGAL1:
        case HOME_ID_HASH_2CH_ILLEGAL2:
        case HOME_ID_HASH_2CH_ILLEGAL3:
          {
            hash++;
          }
          break;

        default:
          break;
      }
    }
    // For Long Range HomeIdHash has no Illegal values
  }
  return (uint8_t)hash;
}
