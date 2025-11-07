// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file
 *
 * Description: Weights and Lookup tables needed for the Dynamic Transmission Power Algorithm for Long Range
 * This holds the implementation of a Dynamic Transmission Power Algorithm for Long Range
 *
 * @copyright 2020 Silicon Laboratories Inc.
 * Author: Lucas Balling
 */

#ifndef ZW_dynamic_tx_power_H_
#define ZW_dynamic_tx_power_H_
#include "stdint.h"
#include <zpal_radio.h>

#define DYNAMIC_TX_POWR_INVALID    -128

/**************************************************************************************/
/*                                EXPORTED FUNCTIONS                                  */
/**************************************************************************************/

/**
 * Fetches the tx power for a long range node from the tx power buffer.
 *
 * Note:
 * If no such value exists for the node, a dfault value will be returned.
 * If the is node has been marked as far away and a high power is needed to reach it,
 * a higher default value will be used.
 *
 * @param[in] nodeID    The node ID of the long range node to read its tx power.
 *
 * @return The tx power of the node
 */
int8_t
GetTXPowerforLRNode(node_id_t nodeID);

/**
 * Puts the tx power for a long range node into the tx power buffer for later use.
 *
 * @param[in] nodeID    The node id of the long range node to save its tx power.
 * @param[in] txPower   The tx power of the node to put into the tx power buffer.
 *                      Tx power value range -10 to 14 (but accepts -128 to 127)
 * @return nothing
 */
void
SetTXPowerforLRNode(node_id_t nodeID, int8_t txPower);

/**
 * Initialize the cache array
 * This function used for unit testing.
 * @return nothing
 */
void
TxPowerBufferInit(void);

/**
 * Erase all buffered TX Power values.
 * This will not remove the LONG RANGE flags.
 * This function used for unit testing.
 * @return nothing
 */
void
TxPowerBufferErase(void);

/*
 * Save Tx Power and RSSI to retention RAM.
 * This function can only be used by slave nodes
 *
 * @param[in] txPower   The Tx Power to save to retention RAM.
 * @param[in] rssi      The rssi to save to retention RAM.
 *
 * @return nothing
 */
void
SaveTxPowerAndRSSI(int8_t txPower, int8_t rssi);

/*
 * Read Tx Power and RSSI from retention RAM.
 * This function can only be used by slave nodes
 *
 * @param[out] txPower   The Tx Power read from retention RAM.
 * @param[out] rssi      The rssi read from  retention RAM.
 *
 * @return nothing
 */
void
ReadTxPowerAndRSSI(int8_t * txPower, int8_t * rssi);

#endif /* ZW_dynamic_tx_power_H_ */
