// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file
 *
 *
 * @copyright 2022 Silicon Laboratories Inc.
 */

#ifndef ZW_DYNAMIC_TX_POWER_ALGORITHM_H_
#define ZW_DYNAMIC_TX_POWER_ALGORITHM_H_
#include "stdint.h"

/**
 * Route types  for \ref GetRouteCache
 */
typedef enum TX_POWER_RETRANSMISSION_TYPE
{
  NOT_RETRANSMISSION = 0,
  RETRANSMISSION,
  RETRANSMISSION_FLIRS
} TX_POWER_RETRANSMISSION_TYPE;

/**
 * Calculates the Optimal TX Power using a Dynamic Transmission Power Algorithm
 *
 * @param txPower - TX Power of the last received frame - in the interval: -10 .. 14 dBm
 * @param RSSIValue - RSSI value of the last received frame - in the interval:  -102 .. -40 dBm
 * @param Noisefloor - Last Measured Noisefloor - in the interval: -102 .. -65 dBm
 * @param TX_POWER_RETRANSMISSION_TYPE retransmissionType - select type of retransmission, else zero
 *
 * @return signed 8-bit integer
 */
int8_t ZW_DynamicTxPowerAlgorithm(int8_t txPower, int8_t RSSIValue, int8_t noisefloor, TX_POWER_RETRANSMISSION_TYPE retransmissionType);

#endif /* ZW_DYNAMIC_TX_POWER_ALGORITHM_H_ */
