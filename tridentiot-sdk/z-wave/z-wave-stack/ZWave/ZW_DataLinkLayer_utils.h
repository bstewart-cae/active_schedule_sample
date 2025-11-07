// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file ZW_DataLinkLayer_utils.h
 * @copyright 2022 Silicon Laboratories Inc.
 */
#ifndef _ZW_DATALINKLAYER_UTILS_H_
#define _ZW_DATALINKLAYER_UTILS_H_

#include <stdint.h>
#include <ZW_DataLinkLayer.h>
/**@brief Function for determining current Frame Header Format
 *
 * @param[in] wNodeID            The node ID for the node we want to get the used frame format.
 * @param[in] forceLR            Force the frame format to Long range
 *
 * @retval HDRFORMATTYP_3CH      The current Frame Header Format is 3 channel
 * @retval HDRFORMATTYP_2CH      The current Frame Header Format is 2 channel
 * @retval HDRFORMATTYP_LR       The current Frame Header Format is Long range
 */
ZW_HeaderFormatType_t llGetCurrentHeaderFormat(node_id_t wNodeID, uint8_t forceLR);

/**@brief Function for updating the txPower field for ACK and singlecast LR frames
 *
 * @param[in,out]    pFrame Pointer to the frame. The txPower field is updated
 *                   based on the dynamic Tx Power algorithm (ZW_DynamicTxPowerAlgorithm)
 *                   which calculates a new txPower based on the current noiseFloor, and
 *                   the RSSI and txPower of the last frame received.
 */
void updateLRFrameTXPower(ZW_TransmissionFrame_t * pFrame);

/**@brief Check if frame received frame has a header type thsat match the current node configuration
 *
 * If a slave node is already included as a LR then we drop frames received on classic channels
 * If a slave node is already included as ZWave node then we drop frames received on ZWave LR channel
 * Controller nodes donæt have a specific configuration
 *
 * @param[int]   headerType the received frame haeder type
 *
 * @return true if received frame match node configuration, else false
 */
bool IsReceivedFrameMatchNodeConfig (ZW_HeaderFormatType_t headerType);
#endif // _ZW_DATALINKLAYER_H_UTILS_
