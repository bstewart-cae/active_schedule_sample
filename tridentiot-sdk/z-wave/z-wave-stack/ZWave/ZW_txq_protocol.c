// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file ZW_txq_protocol.c
 * @copyright 2022 Silicon Laboratories Inc.
 *
 * @bierf Z-Wave protocol functions.
 * Note that some protocol functions are still located in ZW_transport.c.
 */
#include "ZW_lib_defines.h"

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <string.h>
#include <ZW_txq_protocol.h>
#include <ZW_transport.h>
#include <TickTime.h>
#include <ZW_timer.h>
#include <SwTimer.h>
#include <ZW_MAC.h>
#include <ZW_protocol.h>
#include "ZW_Frame.h"
#include <ZW_tx_queue.h>
#include <Assert.h>
#ifdef ZW_CONTROLLER
#include <ZW_controller.h>
#endif
#ifdef ZW_SLAVE
#include <ZW_slave.h>
#endif

//#define DEBUGPRINT
#include <DebugPrint.h>
#include <ZW_DataLinkLayer_utils.h>

/****************************************************************************/
/*                            INCLUDED FUNCTIONS                            */
/****************************************************************************/

/****************************************************************************/
/*                       PRIVATE TYPES and DEFINITIONS                      */
/****************************************************************************/

/****************************************************************************/
/*                                IMPORTED DATA                             */
/****************************************************************************/

/****************************************************************************/
/*                                EXPORTED DATA                             */
/****************************************************************************/


/* For use in ReceiveHandler(). Is current transmission waiting for routed ACK? */
uint8_t waitingForRoutedACK = false; /* No bool memory space left, using uint8_t instead*/

/* Struct containing application transmit status for completed tx jobs */
/* Passed as a pointer to application in senddata callback */
TX_STATUS_TYPE tx_status = { 0 };

extern bool bApplicationTxAbort;

extern TxQueueElement  *pFrameWaitingForACK;

/****************************************************************************/
/*                              PRIVATE FUNCTIONS                           */
/****************************************************************************/

/*========================   ProtocolWaitACKTimeout   =====================
**    Save the transmitelement for later use for when an ack is received or a
**    timeout occurs
**
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
void
ProtocolWaitACKTimeout(void)
{
  if (!IS_TXQ_POINTER(pFrameWaitingForACK))
  {
    return;
  }
  DPRINTF("ProtocolWaitACKTimeout, seq nr: %d\n", pFrameWaitingForACK->frame.frameOptions.sequenceNumber);
  bLastTxFailed = true;
  ZW_HeaderFormatType_t curHeader = llGetCurrentHeaderFormat(pFrameWaitingForACK->frame.frameOptions.destinationNodeId, pFrameWaitingForACK->forceLR);
  pFrameWaitingForACK->bCallersStatus--;
  if (!bApplicationTxAbort && (pFrameWaitingForACK->bCallersStatus >= 1) && (pFrameWaitingForACK->bCallersStatus < 4))
  {
    DPRINTF("RE-TRANSMIT! (try left: %d)\n", pFrameWaitingForACK->bCallersStatus);
    pFrameWaitingForACK->frame.frameOptions.speedModified = 0;
    llReTransmitStart(&pFrameWaitingForACK->frame);
     /* Check if more retransmit attempts should be made */
    if (pFrameWaitingForACK->bCallersStatus > 1)
    {
      TxQueueQueueElement(pFrameWaitingForACK);
    }
    else
    {
      DPRINT("Last re-transmit! (on secondary LR channel)\n");
      if (HDRFORMATTYP_2CH == curHeader)
      {
        /* Last retransmit. Use frequency agility for 100K frames. */
        /* But never for repeated frames. */
        /* Avoid frequency agility during neighbor discovery? */
        if ((((pFrameWaitingForACK->wRFoptions) & RF_OPTION_SPEED_MASK) == RF_OPTION_SPEED_100K) &&
            ((pFrameWaitingForACK->frame.frameOptions.sourceNodeId) == g_nodeID) &&
            (!IS_ROUTED(pFrameWaitingForACK->frame.header)) &&
            (pFrameWaitingForACK->frame.header.header.length <= RX_MAX_LEGACY) &&
            (!g_findInProgress))
          /* Since there are retransmissions, we can safely assume this is a singlecast frame */
        {
          SET_RF_OPTION_SPEED(pFrameWaitingForACK->wRFoptions, RF_OPTION_SPEED_40K);
          pFrameWaitingForACK->frame.frameOptions.speedModified = 1;
        }
      }
      if (HDRFORMATTYP_LR == curHeader)
      {
        /* Before trying on the secondary LR channel, evaluate whether the packet was transmitted by the PHY-layer,
         * or it was just the ACK that was not received. */
        pFrameWaitingForACK->bTxStatus_PrimaryChannel = pFrameWaitingForACK->bPhyTxStatus != TX_QUEUE_PHY_TX_SUCCESS ?
            TRANSMIT_COMPLETE_FAIL : TRANSMIT_COMPLETE_NO_ACK;
        // Last retransmit. Use Secondary Long Channel
        pFrameWaitingForACK->bFrameOptions |= (TRANSMIT_OPTION_LR_FORCE + TRANSMIT_OPTION_LR_SECONDARY_CH);
      }
      TxQueueQueueElement(pFrameWaitingForACK);
    }
  }
  else /* Notify the application about the failure and free the Tx element*/
  {
    DPRINT("Re-transmission limit is hit\n");
    llReTransmitStop(&pFrameWaitingForACK->frame);
    waitingForRoutedACK = false;
    RetransmitFail();

    /* Moved to TxPermanentlyDone() called by RetransmitFail() */
  }
}

/*==========================   ProtocolWaitForACK   ======================
**    Wait for an ACK on a transmitted frame and set up timers for
**    retransmission in case ACK does not arrive.
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
void
ProtocolWaitForACK(TxQueueElement *pTransmittedFrame)
{
  pFrameWaitingForACK = pTransmittedFrame;
  if ((pFrameWaitingForACK->frame.frameOptions.frameType == HDRTYP_SINGLECAST) ||
      (pFrameWaitingForACK->frame.frameOptions.frameType == HDRTYP_ROUTED))
  {
    DPRINT("ProtocolWaitForACK - Tx singlecast/routed frame\n");
  }

  else if (pFrameWaitingForACK->frame.frameOptions.frameType == HDRTYP_MULTICAST)
  {
    DPRINT("ProtocolWaitForACK - Tx multicast frame\n");
     // TODO not implemented yet
  }

  DPRINTF("pFrameWaitingForACK->bPhyTxStatus: %d\n", pFrameWaitingForACK->bPhyTxStatus);

  if (pFrameWaitingForACK->bPhyTxStatus != TX_QUEUE_PHY_TX_SUCCESS)
  {
    DPRINT("ProtocolWaitForACK - Tx phy dropped frame\n");
    /* Frame was dropped by PHY layer, try to send it again and use the current value of pFrameWaitingForACK->bCallersStatus */
    TransmitTimerStart(ProtocolWaitACKTimeout, 1);
  }
  else
  {
    uint32_t Timeout = TRANSFER_ACK_WAIT_TIME_MS + 2 + pFrameWaitingForACK->frame.payloadLength;
    uint32_t LBTTimeout = 0;
    ZW_HeaderFormatType_t curHeader = llGetCurrentHeaderFormat(pFrameWaitingForACK->frame.frameOptions.destinationNodeId, pFrameWaitingForACK->forceLR);
    /* Payload length is pure payload without header, so add default header*/
    if (HDRFORMATTYP_3CH == curHeader)
    {
      Timeout += sizeof(frameHeader3ch);
    }
    else if (HDRFORMATTYP_2CH == curHeader)
    {
      Timeout += sizeof(frameHeader);
    }
    else
    {
      Timeout += sizeof(frameHeaderLR);
    }
    if (IsRouted(curHeader, (frame*)&pFrameWaitingForACK->frame.header))
    {
      waitingForRoutedACK = true;
      // Always add LBT delay when frame is routed
      LBTTimeout = TRANSFER_100K_LBT_DELAY_MS;
      if (HDRFORMATTYP_2CH == curHeader)
      {
        if ( ((pFrameWaitingForACK->wRFoptions & RF_OPTION_SPEED_MASK) == RF_OPTION_SPEED_9600))
        {
           LBTTimeout = TRANSFER_9_6K_LBT_DELAY_MS;
        }
      }
      if (IsOutgoing(curHeader, (frame*)&pFrameWaitingForACK->frame.header) &&     /* Outgoing ?      */
          (GetHops(curHeader, (frame*)&pFrameWaitingForACK->frame.header) == GetRouteLen(curHeader, (frame*)&pFrameWaitingForACK->frame.header)) /* Last repeater ? */
         )
      {
         Timeout = TRANSFER_ROUTED_ACK_WAIT_TIME_MS;
      }
    }
    DPRINTF("ProtocolWaitForACK - timeout %u\n", Timeout);
    if (pFrameWaitingForACK->bCallersStatus > 1)
    {
      /* More retransmissions left */
      /* Activate retransmit timer */
      uint32_t delayCountProtocol = TxRandom(); /* Delay of (1|2|3|4) * (Transfer ack time) */
      delayCountProtocol++; // We want 1-4. Never zero.
      Timeout = (Timeout * delayCountProtocol) + LBTTimeout;
    }
    DPRINTF("ProtocolWaitForACK - timeout * random %u\n", Timeout);

    TransmitTimerStart(ProtocolWaitACKTimeout, Timeout);
  }
}


/*===========================   GetRepeaters   =============================
**
**  Extract the number of repeaters from a TxQueue element.
**
**
**-------------------------------------------------------------------------*/
uint8_t
GetRepeaters(TxQueueElement *elem)
{
  uint8_t repeaters = 0;
  ZW_HeaderFormatType_t curHeader = llGetCurrentHeaderFormat(elem->frame.frameOptions.destinationNodeId, elem->forceLR);

  if (HDRFORMATTYP_LR == curHeader)
  {
    return 0;
  }
  if(IsFrameRouted(curHeader, (frame*)&elem->frame.header))
  {
    repeaters = GetRouteLen(curHeader, (frame*)&elem->frame.header);
  }
  return repeaters;
}

/*======================   ClearSendDataTxStatus   ========================
**
**  Clear the tx_status buffer. Use before delivering a new senddata
**  callback to application. Then populate the struct with relevant
**  information for the callback.
**
**
**-------------------------------------------------------------------------*/
void ClearSendDataTxStatus(void)
{
  memset((uint8_t*)&tx_status, 0, sizeof(tx_status));
  memset((uint8_t*)&tx_status.rssi_values, ZPAL_RADIO_RSSI_NOT_AVAILABLE, sizeof(tx_status.rssi_values));
}


/*============================   TxPermanentlyDone   =========================
**    Release TxQueue element and signal status to application.
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
void                /* RET  nothing */
TxPermanentlyDone(
  uint8_t fStatus,     /* IN fStatus - true if TX was successfully received */
  uint8_t *pAckRssi,   /* IN Pointer to RSSI levels of the (Routed)ACK. */
                    /*    Not used if fStatus is false. */
  int8_t bDestinationAckUsedTxPower,  /* IN TxPower from the received ACK */
  int8_t bDestinationAckMeasuredRSSI,  /* IN RSSI from the received ACK */
  int8_t bDestinationAckMeasuredNoiseFloor  /* IN NoiseFloor from the received ACK*/
  )
{
  DPRINTF("TxPermanentlyDone - status %u\n", fStatus);
  ClearSendDataTxStatus();
  /* TO#5772 fix */
  if (!IS_TXQ_POINTER(pFrameWaitingForACK))
  {
    return;
  }
  /* Set the callback status for ZW_SendData() */
  tx_status.bRepeaters = GetRepeaters(pFrameWaitingForACK);
  if (0 < tx_status.bRepeaters)
  {
    memcpy(tx_status.pLastUsedRoute,
           ((1 == llIsHeaderFormat3ch()) ? pFrameWaitingForACK->frame.header.singlecastRouted3ch.repeaterList :
                                         pFrameWaitingForACK->frame.header.singlecastRouted.repeaterList),
           (tx_status.bRepeaters <= MAX_REPEATERS) ?
            tx_status.bRepeaters :
            MAX_REPEATERS);
  }
  tx_status.pLastUsedRoute[LAST_USED_ROUTE_CONF_INDEX] = pFrameWaitingForACK->wRFoptions & (RF_OPTION_SPEED_MASK | RF_OPTION_BEAM_MASK);
  tx_status.bRouteSchemeState = pFrameWaitingForACK->bRouteSchemeState;
  tx_status.bRouteTries = pFrameWaitingForACK->bTransmitRouteCount;
  tx_status.bLastFailedLink.from = pFrameWaitingForACK->bBadRouteFrom;
  tx_status.bLastFailedLink.to = pFrameWaitingForACK->bBadRouteTo;
  tx_status.bACKChannelNo = pFrameWaitingForACK->bACKChannel;
  tx_status.bLastTxChannelNo = pFrameWaitingForACK->bTxChannel;
  tx_status.TransmitTicks = getTickTimePassed(pFrameWaitingForACK->StartTicks);
  if ((RF_PROFILE_100K_LR_A == pFrameWaitingForACK->frame.profile)
      || (RF_PROFILE_100K_LR_B == pFrameWaitingForACK->frame.profile))
  {
    tx_status.bUsedTxpower = pFrameWaitingForACK->frame.txPower;
    tx_status.bMeasuredNoiseFloor = pFrameWaitingForACK->frame.frameOptions.noiseFloor;
  }
  else
  {
    // If not LR, those values are not applicable
    tx_status.bUsedTxpower = ZPAL_RADIO_RSSI_NOT_AVAILABLE;
    tx_status.bMeasuredNoiseFloor = ZPAL_RADIO_RSSI_NOT_AVAILABLE;
  }
  tx_status.bDestinationAckUsedTxPower = bDestinationAckUsedTxPower;
  tx_status.bDestinationAckMeasuredRSSI = bDestinationAckMeasuredRSSI;
  tx_status.bDestinationAckMeasuredNoiseFloor = bDestinationAckMeasuredNoiseFloor;

  DPRINTF("Primary CH TxStatus: %d  |  Latest/secondary CH TxStatus: %d\n",
      pFrameWaitingForACK->bTxStatus_PrimaryChannel,
      pFrameWaitingForACK->bPhyTxStatus);

  uint8_t returnTxStatus = TRANSMIT_COMPLETE_NO_ACK;  // Set the ACK status.

  /**
   * Only return tx status for the primary channel if the TX has failed!
   */

  ZW_HeaderFormatType_t curHeaderFormat = llGetCurrentHeaderFormat(
      pFrameWaitingForACK->frame.frameOptions.destinationNodeId, pFrameWaitingForACK->forceLR);

  if (fStatus)  // Is all good. Did we get an ACK?
  {
    /* RSSI is only available if ACK was received
     * only change the default value of ZPAL_RADIO_RSSI_NOT_AVAILABLE if ACK was received. */
    memcpy(tx_status.rssi_values.incoming, pAckRssi, MAX_REPEATERS + 1);
    returnTxStatus = TRANSMIT_COMPLETE_OK;  // Always return success if ACK was received.
  }
  else if(HDRFORMATTYP_LR == curHeaderFormat)
  {
    // What we send to app from here may not be a TRANSMIT_COMPLETE_OK, because fStatus was false.

    /* For Z-WaveLR:
     * If bTxStatus_PrimaryChannel != TRANSMIT_COMPLETE_OK, then this latest TX was on secondary LR channel,
     * because bTxStatus_PrimaryChannel is initialized to 0 by memset() to begin with.
     * bTxStatus_PrimaryChannel can be either FAIL or NO_ACK for the primary LR channel for the line below. */
    returnTxStatus = pFrameWaitingForACK->bTxStatus_PrimaryChannel;
  }
  else if (TX_QUEUE_PHY_TX_SUCCESS != pFrameWaitingForACK->bPhyTxStatus)
  {
    // For Z-Wave, return FAIL, since the PHY layer never transmitted the frame!
    returnTxStatus = TRANSMIT_COMPLETE_FAIL;  // PHY layer never transmitted the frame!
  }

  // Free the TxQueue Element before calling the app callback as a quick buffer release.
  STransmitCallback appCallback = pFrameWaitingForACK->AppCallback;
  TxQueueReleaseElement(pFrameWaitingForACK);
  waitingForRoutedACK = false;

  ZW_TransmitCallbackInvoke(&appCallback, returnTxStatus, &tx_status);
  // Do NOT ZERO pFrameWaitingForACK before calling callback as ex. repeaters need
  // the repeated frame for determining where to transmit RoutedErr, if transmit failed
  pFrameWaitingForACK = NULL;
}


/*========================   ProtocolTransmitComplete   ====================
**    Transmit Complete function
**
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
void
ZCB_ProtocolTransmitComplete(TxQueueElement *pTransmittedFrame)
{
  register uint8_t bCallbackStatus;
  /* Check if we should wait for an ack frame */
  if (!IS_TXQ_POINTER(pTransmittedFrame))
  {
    return;
  }
  if (TX_QUEUE_PHY_TX_LBT_FAILED == pTransmittedFrame->bPhyTxStatus)
  {
    zpal_radio_rf_channel_statistic_tx_lbt_failures();
  }
  if ((pTransmittedFrame->bFrameOptions & TRANSMIT_OPTION_ACK) &&
      (!IS_EXPLORE(pTransmittedFrame->frame))
     )
  {
    ProtocolWaitForACK(pTransmittedFrame);
    return;
  }

  DPRINT("ZCB_ProtocolTransmitComplete - No ACK requested");
  ClearSendDataTxStatus();
  /* Set the callback status for ZW_SendData() */
  tx_status.bRepeaters = GetRepeaters(pTransmittedFrame);
  if (0 < tx_status.bRepeaters)
  {
    memcpy(tx_status.pLastUsedRoute,
           ((1 == llIsHeaderFormat3ch()) ? pTransmittedFrame->frame.header.singlecastRouted3ch.repeaterList :
                                        pTransmittedFrame->frame.header.singlecastRouted.repeaterList),
           (tx_status.bRepeaters <= MAX_REPEATERS) ?
            tx_status.bRepeaters :
            MAX_REPEATERS);
  }
  tx_status.pLastUsedRoute[LAST_USED_ROUTE_CONF_INDEX] = pTransmittedFrame->wRFoptions & (RF_OPTION_SPEED_MASK | RF_OPTION_BEAM_MASK);
  tx_status.bRouteSchemeState = pTransmittedFrame->bRouteSchemeState;
  tx_status.bRouteTries = pTransmittedFrame->bTransmitRouteCount;
  tx_status.bLastFailedLink.from = pTransmittedFrame->bBadRouteFrom;
  tx_status.bLastFailedLink.to = pTransmittedFrame->bBadRouteTo;
  tx_status.bACKChannelNo = pTransmittedFrame->bACKChannel;
  tx_status.bLastTxChannelNo = pTransmittedFrame->bTxChannel;
  tx_status.TransmitTicks = getTickTimePassed(pTransmittedFrame->StartTicks);
  if (TX_QUEUE_PHY_TX_SUCCESS == pTransmittedFrame->bPhyTxStatus)
  {
    /* RSSI unavailable when ACK not requested
     * No need to change the default value of ZPAL_RADIO_RSSI_NOT_AVAILABLE. */
    bCallbackStatus = TRANSMIT_COMPLETE_OK;
  }
  else if ((TX_QUEUE_PHY_TX_FAILED == pTransmittedFrame->bPhyTxStatus) ||
           (TX_QUEUE_PHY_TX_LBT_FAILED == pTransmittedFrame->bPhyTxStatus))
  {
    bCallbackStatus = TRANSMIT_COMPLETE_FAIL;
  }
  else
  {
    bCallbackStatus = TRANSMIT_COMPLETE_NO_ACK;
  }

  /* Call the application callback function ??? What about transmit status */
  /* TRANSMIT_COMPLETE_OK callback must wait until ACK is received. */
  ZW_TransmitCallbackInvoke(&pTransmittedFrame->AppCallback, bCallbackStatus, &tx_status);

  /* Release queue element */
  TxQueueReleaseElement((TxQueueElement *)pTransmittedFrame);
}


/****************************************************************************/
/*                                API Functions                             */
/****************************************************************************/
