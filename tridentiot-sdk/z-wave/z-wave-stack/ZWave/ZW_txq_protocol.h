// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file ZW_txq_protocol.h
 * @copyright 2022 Silicon Laboratories Inc.
 *
 * @brief Z-Wave TxQueue Protocol header.
 */
#ifndef _ZW_TXQ_PROTOCOL_H_
#define _ZW_TXQ_PROTOCOL_H_

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <ZW_tx_queue.h>

/****************************************************************************/
/*                     EXPORTED TYPES and DEFINITIONS                       */
/****************************************************************************/
void
ZCB_ProtocolTransmitComplete(TxQueueElement *pTransmittedFrame);

void
ProtocolWaitForACK(TxQueueElement *pTransmittedFrame);

/*============================   TxPermanentlyDone   =========================
**    Release TxQueue element and signal status to application.
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
void                   /* RET  nothing */
TxPermanentlyDone(uint8_t fStatus,     /* IN fStatus - true if TX was succesfully received */
                  uint8_t *pAckRssi,  /* IN Pointer to RSSI levels of the (Routed)ACK.
                                       Not used if fStatus is false. */
                  int8_t bDestinationAckUsedTxPower,  /* IN TxPower from the received ACK */
                  int8_t bDestinationAckMeasuredRSSI,  /* IN RSSI from the received ACK */
                  int8_t bDestinationAckMeasuredNoiseFloor); /* IN NoiseFloor from the received ACK */

#endif /* _ZW_TXQ_PROTOCOL_H_ */
