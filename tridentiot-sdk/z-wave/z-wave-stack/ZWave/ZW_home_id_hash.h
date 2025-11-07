// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file
 * Z-Wave Home ID Hash calculation.
 *
 *
 * @copyright 2020 Silicon Laboratories Inc.
 */

#ifndef _ZW_HOME_ID_HASH_H_
#define _ZW_HOME_ID_HASH_H_

#include <stdint.h>
#include <zpal_radio.h>


/** Function for calculating 8 bit (XOR) hash based on home id.
 *
 *@param[in] homeId  uint32_t containing current homeId
 *@param[in] nodeId  node_id_t containing nodeId for which HomeIdHash is destined
 *
 *@return The calculated home id hash
 */
uint8_t HomeIdHashCalculate(uint32_t homeId, node_id_t nodeId, zpal_radio_protocol_mode_t zwProtMode);

#endif /* _ZW_HOME_ID_HASH_H_ */
