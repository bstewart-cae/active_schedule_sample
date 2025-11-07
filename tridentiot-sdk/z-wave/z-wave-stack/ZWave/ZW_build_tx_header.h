// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file ZW_build_tx_header.h
 * @copyright 2022 Silicon Laboratories Inc.
 *
 * @brief This module provide functions to build the Tx frame header fr all frames format.
 */
#ifndef _ZW_BUILD_TX_HEADER_H_
#define _ZW_BUILD_TX_HEADER_H_

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/

#include <stdint.h>
#include <ZW_Frame.h>
#include <ZW_tx_queue.h>

/**@brief Function to build the Tx frame header
 *
 * @param[in] curHeader         The frame format of the current frame
 * @param[in] frameType         Frame type (FRAMETYPE_*, lower nibble), seq no. (upper nibble)
 * @param[in] bSequenceNumber   Sequence number of the frame that should be transmitted (only used for 3CH)
 * @param[in] pFrame            Frame pointer
 */
void           /* RET Frame header length */
BuildTxHeader(
  ZW_HeaderFormatType_t headerType,
  uint8_t  frameType,
  uint8_t bSequenceNumber,
  TxQueueElement *pFrame);

#endif /* _ZW_BUILD_TX_HEADER_H_*/
