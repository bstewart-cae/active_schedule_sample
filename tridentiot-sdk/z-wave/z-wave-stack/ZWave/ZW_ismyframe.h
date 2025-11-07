// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file
 * Check if the frame is addressed for this node.
 * @copyright 2019 Silicon Laboratories Inc.
 */

#ifndef _ZW_ISMYFRAME_H_
#define _ZW_ISMYFRAME_H_

#define MASK_CURRENTSEQNO_ROUTE 0x40
#define MASK_CURRENTSEQNO_SEQNO 0x0f
#define MASK_CURRENTSEQNO_SPEED 0x80

/* sACK status bits telling if the recently received frame should be treated as an ACK */
/* and if it should result in the next transmission being delayed */
#define RECEIVE_SILENT_ACK  1
#define RECEIVE_DO_DELAY    2
#define RECEIVE_DO_REPEAT   4
/* Check if we have currently received a 40K frame */
#define IS_CURRENT_SPEED_40K (mTransportRxCurrentSpeed == RF_SPEED_40K)

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/

#include <ZW_transport.h>

/**@brief Function to test if the received frame is sent to the current node
 *
 * If the received frame is a Smart Start frame, RECEIVE_STATUS_SMART_NODE will be set in the
 * variable pointed to by pReceiveStatus.
 *
 * @param[in] curHeader     The frame format of the current frame
 * @param[in] pRxFrame      Pointer to the received frame structure
 * @return                  true if the received frame addressed to the node itself, else false
 */
bool IsMyFrame(
    ZW_HeaderFormatType_t curHeader,
    RX_FRAME *pRxFrame,
    uint8_t * pReceiveStatus);

#endif /* _ZW_ISMYFRAME_H_ */
