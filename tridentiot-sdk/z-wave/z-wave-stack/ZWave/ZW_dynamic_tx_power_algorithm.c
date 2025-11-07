// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file
 *
 *
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include "ZW_dynamic_tx_power_algorithm.h"
#include "ZW_dynamic_tx_power.h"
#include <zpal_radio.h>

//Bounds for valid RSSIMargin (RSSI - noise)
#define PRECODED_RSSI_MARGIN_LOWEST     -5
#define PRECODED_RSSI_MARGIN_HIGHEST    26

//The algorithm should accept incoming RSSI in this range
#define DYNAMIC_TX_RSSI_LOWEST        -120
#define DYNAMIC_TX_RSSI_HIGHEST         10

//Optimal power margin
#define RSSI_MARGIN_MIN                  6
#define RSSI_MARGIN_MAX                 15

//Steps used when adjusting tx power up or down
#define TX_POWER_STEP                    3

//Noise floor limits
#define LOWEST_ACCEPTED_NOISE_LEVEL   -102
#define HIGHEST_ACCEPTED_NOISE_LEVEL   -65

/**
* ZW_DynamicTxPowerAlgorithm
*
* Algorithm used to find the Optimal TX Power for the Z-Wave LR node
* Input Arguments:
* int8_t txPower in the range -10 to 20dBm
* int8_t RSSIValue in the range -120 to 10 dBm
* int8_t noisefloor in the range -102 to -65dBm
* TX_POWER_RETRANSMISSION_TYPE retransmissionType - select type of retransmission, else zero
*/
int8_t ZW_DynamicTxPowerAlgorithm(int8_t txPower, int8_t RSSIValue, int8_t noisefloor, TX_POWER_RETRANSMISSION_TYPE retransmissionType){

  int8_t OptimalTxPowerdBm = 0;
  if (RETRANSMISSION == retransmissionType || ZPAL_RADIO_RSSI_NOT_AVAILABLE == RSSIValue)
  {
    // Retransmission - Increase the TX Power with 3dBm
    OptimalTxPowerdBm = txPower + 3;
  }
  else if (RETRANSMISSION_FLIRS == retransmissionType)
  {
  	// Decrease the TX Power with 3dBm since FLiRS is preceded by singlecast retransmissions
    OptimalTxPowerdBm = txPower - 3;
  }
  else if ( (noisefloor == DYNAMIC_TX_POWR_INVALID) || (RSSIValue == DYNAMIC_TX_POWR_INVALID) )
  {
    // If the noise floor or the RSSI Value is invalid (-128) don't modify TX Power.
    OptimalTxPowerdBm =  txPower;
  }
  else if ( ( RSSIValue < DYNAMIC_TX_RSSI_LOWEST ) || ( RSSIValue > DYNAMIC_TX_RSSI_HIGHEST ) )
  {
    // If the RSSIValue is out of the defined range for the Dynamic TX Power Algorithm don't modify TX Power.
    OptimalTxPowerdBm = txPower;
  }
  else
  {
    // Make sure noise floor doesn't go below -102dBm(Which is the sensitivity of the 700 series) or above -65 dBm when LBT kicks in.
    if (noisefloor < LOWEST_ACCEPTED_NOISE_LEVEL)
    {
      noisefloor = LOWEST_ACCEPTED_NOISE_LEVEL;
    }
    else if (noisefloor > HIGHEST_ACCEPTED_NOISE_LEVEL)
    {
      noisefloor = HIGHEST_ACCEPTED_NOISE_LEVEL;
    }

    // Calculate the RSSIMargin
    int8_t RSSIMargin = RSSIValue - noisefloor;

    // Make sure the RSSIMargin is within -5 to 26
    if (RSSIMargin < PRECODED_RSSI_MARGIN_LOWEST)
    {
      RSSIMargin = PRECODED_RSSI_MARGIN_LOWEST;
    }
    else if (RSSIMargin > PRECODED_RSSI_MARGIN_HIGHEST)
    {
      RSSIMargin = PRECODED_RSSI_MARGIN_HIGHEST;
    }

    if (RSSIMargin < RSSI_MARGIN_MIN)
    {
      OptimalTxPowerdBm = txPower + TX_POWER_STEP;
    }
    else if (RSSIMargin > RSSI_MARGIN_MAX)
    {
      OptimalTxPowerdBm = txPower - TX_POWER_STEP;
    }
    else {
      OptimalTxPowerdBm = txPower;
    }
  }


  // Make sure the Optimal TX Power is within the TX Power limit set in the Radio driver.
  zpal_tx_power_t maxTxPower = zpal_radio_get_maximum_lr_tx_power();
  zpal_tx_power_t minTxPower = zpal_radio_get_minimum_lr_tx_power();

  // Ensure the max and min TX power values within the int8_t range
  maxTxPower = (maxTxPower > INT8_MAX) ? INT8_MAX : maxTxPower;
  minTxPower = (minTxPower < INT8_MIN) ? INT8_MIN : minTxPower;

  if (OptimalTxPowerdBm > maxTxPower) {
    OptimalTxPowerdBm = (int8_t) maxTxPower;
  } else if (OptimalTxPowerdBm < minTxPower) {
    OptimalTxPowerdBm = (int8_t) minTxPower;
  }
  return OptimalTxPowerdBm;
}
