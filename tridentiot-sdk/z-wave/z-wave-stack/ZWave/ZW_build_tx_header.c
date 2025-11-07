// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file ZW_build_tx_header.c
 * @copyright 2022 Silicon Laboratories Inc.
 *
 * @brief This module provide functions to build the Tx frame header fr all frames format.
 */
#include "ZW_lib_defines.h"
#include <NodeMask.h>
#include <string.h>
#include <Assert.h>
#ifdef ZW_SLAVE
#include <ZW_slave.h>
#endif
#ifdef ZW_CONTROLLER
#include <ZW_controller.h>
#endif
#include "ZW_Frame.h"
//#define DEBUGPRINT
#include "DebugPrint.h"

#define MASK_FRAMETYPE_FRAMETYPE 0x0f
#define MASK_FRAMETYPE_SEQNO 0xf0

#define MINIMUM_ROUTING_HEADER_LENGTH_2CH        2
#define MINIMUM_ROUTING_HEADER_LENGTH_3CH        3
#ifdef ZW_CONTROLLER
extern uint8_t multiIDList[MULTICAST_NODE_LIST_SIZE];
#else
extern uint8_t multiIDList[MAX_NODEMASK_LENGTH];
#endif
extern uint8_t currentSeqNo;
extern void SetTransmitHomeID(TxQueueElement *ptrFrame);

static void
BuildTxAckHeader(
    ZW_HeaderFormatType_t headerType, /*IN header type of the transmitted frame*/
    TxQueueElement *pFrame)           /* IN Frame pointer               */
{
  /* build frame acknowledge header */
  memcpy(pFrame->frame.frameOptions.homeId, ZW_HomeIDGet(), HOMEID_LENGTH);
#ifndef ZW_CONTROLLER_BRIDGE
  pFrame->frame.frameOptions.sourceNodeId = g_nodeID;
#endif /* !ZW_CONTROLLER_BRIDGE */
  pFrame->frame.frameOptions.frameType = HDRTYP_TRANSFERACK;
  /* We are transmitting an ACK so use the received sequence number */
  if (HDRFORMATTYP_3CH == headerType || HDRFORMATTYP_LR == headerType)
  {
    /* We are transmitting an ACK so use the received sequence number instead */
    pFrame->frame.frameOptions.sequenceNumber = currentSeqNo;
  }
  else
  {
    pFrame->frame.frameOptions.sequenceNumber = (currentSeqNo & MASK_FRAMETYPE_SEQNO);
  }
  switch (headerType)
  {
    case HDRFORMATTYP_2CH:
      pFrame->frame.headerLength = sizeof(pFrame->frame.header.singlecast);
      pFrame->frame.header.singlecast.header.length = pFrame->frame.headerLength;
      break;
    case HDRFORMATTYP_3CH:
      pFrame->frame.headerLength = (sizeof(pFrame->frame.header.singlecast3ch) - sizeof(pFrame->frame.header.singlecast3ch.extension));
      pFrame->frame.header.singlecast3ch.header.length = pFrame->frame.headerLength;
      break;
    case HDRFORMATTYP_LR:
      pFrame->frame.headerLength = sizeof(pFrame->frame.header.transferACKLR);
      pFrame->frame.header.singlecastLR.header.length = pFrame->frame.headerLength;
      break;
    case HDRFORMATTYP_UNDEFINED:
      break;
  }
  DPRINTF("BuildTxAckHeader: pFrame->frame.headerLength = %02X\n", pFrame->frame.headerLength);
}

static void
BuildTxSingleHeader(
    ZW_HeaderFormatType_t headerType, /*IN header type of the transmitted frame*/
    TxQueueElement *pFrame)           /* IN Frame pointer               */
{
  uint8_t transmitRFSpeedOption;
  uint8_t reqAck;
  pFrame->frame.frameOptions.frameType = HDRTYP_SINGLECAST;

  if (pFrame->bFrameOptions & TRANSMIT_OPTION_ACK)
  {
    pFrame->bCallersStatus = TRANSFER_RETRIES; /* retransmissions */
    reqAck = 1;
  }
  else
  {
    pFrame->bCallersStatus = 1; /* no retransmission */
    reqAck = 0;
  }
#ifdef ZW_CONTROLLER
  transmitRFSpeedOption = IsNodeSensor(g_nodeID, true, false);
#else  /* ZW_CONTROLLER */
  transmitRFSpeedOption = ((ZW_nodeIsFLiRS() & APPLICATION_FREQ_LISTENING_MODE_1000ms) ? ZWAVE_NODEINFO_SENSOR_MODE_WAKEUP_1000 : 0) | ((ZW_nodeIsFLiRS() & APPLICATION_FREQ_LISTENING_MODE_250ms) ? ZWAVE_NODEINFO_SENSOR_MODE_WAKEUP_250 : 0);
#endif /* ZW_CONTROLLER */
  if (transmitRFSpeedOption & APPLICATION_FREQ_LISTENING_MODE_1000ms)
    pFrame->frame.frameOptions.wakeup1000ms = 1;
  if (transmitRFSpeedOption & APPLICATION_FREQ_LISTENING_MODE_250ms)
    pFrame->frame.frameOptions.wakeup250ms = 1;
  switch (headerType)
  {
    case HDRFORMATTYP_2CH:
      if (pFrame->bFrameOptions1 & TRANSMIT_FRAME_OPTION_SPEED_MODIFIED)
      {
        pFrame->frame.frameOptions.speedModified = 1;
      }
// fallthrough
    case HDRFORMATTYP_LR:
      if (reqAck)
      {
        /* Set followup bit */
        pFrame->frame.frameOptions.multicastfollowup = (0 != (pFrame->bFrameOptions1 & TRANSMIT_FRAME_OPTION_FOLLOWUP));
        if (pFrame->bFrameOptions & TRANSMIT_OPTION_ROUTED)
        {
          reqAck = 0;
        }
      }
      if (pFrame->bFrameOptions & TRANSMIT_OPTION_ROUTED)
      {
        pFrame->frame.frameOptions.routed = 1; /* routed frame */
      }
      break;
    case HDRFORMATTYP_3CH:
      SetWakeupBeamSpeedSrc(headerType, &pFrame->frame.header, transmitRFSpeedOption);
      break;
    default:
      break;
  }
  switch (headerType)
  {
    case HDRFORMATTYP_2CH:
      pFrame->frame.headerLength = sizeof(pFrame->frame.header.singlecast);
      pFrame->frame.header.singlecast.header.length = pFrame->frame.headerLength + pFrame->frame.payloadLength;
      break;
    case HDRFORMATTYP_3CH:
      pFrame->frame.headerLength = (sizeof(pFrame->frame.header.singlecast3ch) - sizeof(pFrame->frame.header.singlecast3ch.extension));
      pFrame->frame.header.singlecast3ch.header.length = pFrame->frame.headerLength + pFrame->frame.payloadLength;
      break;
    case HDRFORMATTYP_LR:
      pFrame->frame.headerLength = (sizeof(pFrame->frame.header.singlecastLR) - sizeof(pFrame->frame.header.singlecastLR.extension));
      pFrame->frame.header.singlecastLR.header.length = pFrame->frame.headerLength + pFrame->frame.payloadLength;
      break;
    case HDRFORMATTYP_UNDEFINED:
      break;
  }
  pFrame->frame.frameOptions.acknowledge = reqAck;
  DPRINTF("BuildTxSingleHeader: pFrame->frame.headerLength = %02X\n", pFrame->frame.headerLength);
}

static void
BuildTxRoutedHeader(
    ZW_HeaderFormatType_t headerType, /*IN header type of the transmitted frame*/
    TxQueueElement *pFrame)           /* IN Frame pointer               */
{
  uint8_t bRepeaterCount;
  uint8_t* pRoutingHeader;
  if (HDRFORMATTYP_3CH == headerType)
  {
    pRoutingHeader = &pFrame->frame.header.singlecastRouted3ch.routeStatus;
  }
  else
  {
    pRoutingHeader = &pFrame->frame.header.singlecastRouted.routeStatus;
  }
  pRoutingHeader[0] = 0;
  /* build frame header from the first element into the Tx queue */
  if (pFrame->bFrameOptions & TRANSMIT_OPTION_EXPLORE_REPEAT)
  {
    bRepeaterCount = ((pFrame->frame.payload[1] & MASK_ROUT_REPTS) >> 4);
    if (MAX_REPEATERS < bRepeaterCount)
    {
      bRepeaterCount = MAX_REPEATERS;
      // set the numRepsNumHops byte
      pFrame->frame.payload[1] = (pFrame->frame.payload[1] & MASK_ROUT_HOPS) | (MAX_REPEATERS << 4);
    }
    /* Copy repeater */
    memcpy(pRoutingHeader, pFrame->frame.payload, 2 + bRepeaterCount);
    /* Adjust data pointer and length to point to payload instead of header */
    /* TO#4144 memory overwrite fix */
    if (pFrame->frame.payloadLength > (2 + bRepeaterCount + 1))
    {
      pFrame->frame.payloadLength -= (2 + bRepeaterCount + 1);
    }
    else
    {
      pFrame->frame.payloadLength = 0;
    }
    memmove(pFrame->frame.payload, &pFrame->frame.payload[(2 + bRepeaterCount)], pFrame->frame.payloadLength);
  }
  /* no low output power */
  pFrame->frame.frameOptions.routed = 1; /* routed frame */
  if (pFrame->bFrameOptions & TRANSMIT_OPTION_ACK)
  {
    pFrame->bCallersStatus = TRANSFER_RETRIES; /* retransmissions */
  }
  else
  {
    pFrame->bCallersStatus = 1; /* no retransmission */
  }
  // payload start is the singlecastrouted frame routeStatus field
  // Repeater list is already filled out.
  if (HDRFORMATTYP_3CH != headerType)
  {
    if (pFrame->bFrameOptions1 & TRANSMIT_FRAME_OPTION_SPEED_MODIFIED)
    {
      pFrame->frame.header.singlecastRouted.routeStatus |= MASK_ROUT_SPEED_MODIFIED;
    }
  }
  /* For now we never beam to the repeater - only destination */
#ifdef ZW_CONTROLLER
  if (IsNodeSensor(pFrame->frame.frameOptions.destinationNodeId, false, true))
#endif
#ifdef ZW_SLAVE_ROUTING
    if (pFrame->wRFoptions & RF_OPTION_BEAM_MASK)
#endif
    {
      /* Are we the originator of the frame */
#ifdef ZW_CONTROLLER_BRIDGE
      if ((pFrame->frame.frameOptions.sourceNodeId == g_nodeID || ZW_IsVirtualNode(pFrame->frame.frameOptions.sourceNodeId))
#else
    if ((pFrame->frame.frameOptions.sourceNodeId == g_nodeID)
#endif
          && IsOutgoing(headerType, (frame *)&pFrame->frame.header) &&
          !(pFrame->bFrameOptions1 & TRANSMIT_FRAME_OPTION_NOBEAM))
      {
        if (HDRFORMATTYP_3CH != headerType)
        {
          /* Set Extend body present in routing header */
          SetExtendPresent(headerType, &pFrame->frame.header);
          /* Set Extend body type */
          SetExtendTypeHeader(headerType, &pFrame->frame.header, EXTEND_TYPE_WAKEUP_TYPE);
          /* Set Extend body length */
          SetExtendLenHeader(headerType, &pFrame->frame.header, 1);
          /* bit 0x40 and bit 0x20 are Zensor Wakeup type bits */
#ifdef ZW_CONTROLLER
          SetExtendBodyHeader(headerType, &pFrame->frame.header, EXTEND_TYPE_WAKEUP_TYPE_SRC_DEST_OFFSET, (IsNodeSensor(pFrame->frame.frameOptions.destinationNodeId, true, false) | (IsNodeSensor(g_nodeID, true, false) >> 2)));
#endif
#ifdef ZW_SLAVE_ROUTING
          /* TO#2670 fix. Use the correct MASK. BEAM bits are already the right place. */
          /* TO#2671 fix. Use the correct MASK. BEAM bits are already the right place. */
          SetExtendBodyHeader(headerType, &pFrame->frame.header, EXTEND_TYPE_WAKEUP_TYPE_SRC_DEST_OFFSET, (pFrame->wRFoptions & RF_OPTION_BEAM_MASK));
#endif
        }
        else
        {
          /* Set destination wakeup type in route */
#ifdef ZW_CONTROLLER
          SET_ROUTING_DEST_WAKEUP_3CH(pFrame->frame.header, IsNodeSensor(pFrame->frame.frameOptions.destinationNodeId, true, false) >> 5);
#else
        SET_ROUTING_DEST_WAKEUP_3CH(pFrame->frame.header,
                                    ((pFrame->wRFoptions & RF_OPTION_BEAM_MASK) >> 5));
#endif
        }
        /* Routing slave would beam to repeater when routing to a FLiRS */
        pFrame->wRFoptions &= ~RF_OPTION_BEAM_MASK;
      }
      else
      {
        if (HDRFORMATTYP_3CH != headerType)
        {
          ClrExtendPresent(headerType, (frame *)&pFrame->frame.header);
        }
      }
    }
    else
    {
      /* Clear Extend body present in routing header */
      if (HDRFORMATTYP_3CH != headerType)
      {
        ClrExtendPresent(headerType, (frame *)&pFrame->frame.header);
      }
      else
      {
        /* Set destination wakeup type in route */
        CLR_ROUTING_DEST_WAKEUP_3CH(pFrame->frame.header);
      }
    }
  uint8_t routedHeaderLen = GetRouteLen(headerType, (frame *)&pFrame->frame.header);
  if (HDRFORMATTYP_3CH != headerType)
  {
    if (GetExtendPresent(headerType, (frame *)&pFrame->frame.header))
    {
      /*When extension bit is 1 then we add 1 byte for the extension header and the lenght of the repeater list for the total length of routing header*/
      /*the extension header offset is 1 byte after the repeater list (repeaterList[routedHeaderLen])*/
      routedHeaderLen += (1 + MINIMUM_ROUTING_HEADER_LENGTH_2CH + (pFrame->frame.header.singlecastRouted.repeaterList[routedHeaderLen] >> 4));
    }
    else
    {
      /*When the extension bit is 0 then the routing header total length is 2 bytes routing header + length of the repeater list*/
      routedHeaderLen += MINIMUM_ROUTING_HEADER_LENGTH_2CH;
    }
    pFrame->frame.frameOptions.frameType = HDRTYP_SINGLECAST; /* singlecast only */
  }
  else
  {
    /*The minimum 3ch routing header length is 3 bytes*/
    if (GetExtendPresent(headerType, (frame *)&pFrame->frame.header))
    {
      /*When extension bit is 1 then we add 1 byte for the extension header and the lenght of the repeater list for the total length of routing header*/
      /*the extension header offset is 2 bytes after the repeater list (repeaterList[routedHeaderLen + 1])*/
      routedHeaderLen +=  (1+ MINIMUM_ROUTING_HEADER_LENGTH_3CH + (pFrame->frame.header.singlecastRouted3ch.repeaterList[routedHeaderLen + 1] & MASK_EXTENSION_LENGTH_3CH));
    }
    else
    {
      /*When the extension bit is 0 then the routing header total length is 3 bytes routing header + length of the repeater list*/
      routedHeaderLen += MINIMUM_ROUTING_HEADER_LENGTH_3CH;
    }
    pFrame->frame.frameOptions.frameType = HDRTYP_ROUTED;
  }

  pFrame->frame.headerLength = routedHeaderLen > sizeof(pFrame->frame.header) ? sizeof(pFrame->frame.header) : routedHeaderLen;
  switch (headerType)
  {
    case HDRFORMATTYP_2CH:
      pFrame->frame.headerLength += sizeof(pFrame->frame.header.singlecast);
      pFrame->frame.header.singlecast.header.length = pFrame->frame.headerLength + pFrame->frame.payloadLength;
      break;
    case HDRFORMATTYP_3CH:
      pFrame->frame.headerLength += (sizeof(pFrame->frame.header.singlecast3ch) - sizeof(pFrame->frame.header.singlecast3ch.extension));
      pFrame->frame.header.singlecast3ch.header.length = pFrame->frame.headerLength + pFrame->frame.payloadLength;
      break;
    case HDRFORMATTYP_LR:
      pFrame->frame.headerLength += (sizeof(pFrame->frame.header.singlecastLR) - sizeof(pFrame->frame.header.singlecastLR.extension));
      pFrame->frame.header.singlecastLR.header.length = pFrame->frame.headerLength + pFrame->frame.payloadLength;
      break;
    case HDRFORMATTYP_UNDEFINED:
      break;
  }
}

static void
BuildTxMultiHeader(
    ZW_HeaderFormatType_t headerType, /*IN header type of the transmitted frame*/
    TxQueueElement *pFrame)           /* IN Frame pointer               */
{
  pFrame->frame.frameOptions.frameType = HDRTYP_MULTICAST;
  if (pFrame->bFrameOptions & TRANSMIT_OPTION_LOW_POWER)
  {
    pFrame->frame.frameOptions.lowPower = 1; /* Transmit frame at low output power level */
  }
  pFrame->bCallersStatus = 1; /* no retransmission */
  switch (headerType)
  {
    case HDRFORMATTYP_2CH:
     // Set length of the multicast specific header
      pFrame->frame.headerLength = sizeof(frameHeaderMulticast);
      pFrame->frame.header.multicast.addrOffsetNumMaskBytes = (uint8_t)MAX_NODEMASK_LENGTH;
      memcpy(&pFrame->frame.header.multicast.destinationMask, multiIDList, MAX_NODEMASK_LENGTH);
      /* frame length */
      pFrame->frame.header.multicast.header.length = pFrame->frame.headerLength + pFrame->frame.payloadLength;
      break;
    case HDRFORMATTYP_3CH:
      // Set length of the multicast specific header
      pFrame->frame.headerLength = sizeof(frameHeaderMulticast3ch) - sizeof(frameHeaderExtension3ch);
      pFrame->frame.header.multicast3ch.addrOffsetNumMaskBytes = (uint8_t)MAX_NODEMASK_LENGTH;
      memcpy(&pFrame->frame.header.multicast3ch.destinationMask, multiIDList, MAX_NODEMASK_LENGTH);
      /* frame length */
      pFrame->frame.header.multicast3ch.header.length = pFrame->frame.headerLength + pFrame->frame.payloadLength;
      break;
    default:
      break;
  }
}

static void
BuildTxExploreHeader(
  ZW_HeaderFormatType_t headerType, /*IN header type of the transmitted frame*/
   TxQueueElement *pFrame)           /* IN Frame pointer               */
{
  pFrame->frame.frameOptions.frameType = HDRTYP_EXPLORE;
  if ((pFrame->bFrameOptions & TRANSMIT_OPTION_EXPLORE_REPEAT) == 0)
  {
    SetTransmitHomeID(pFrame);
  }
  pFrame->bCallersStatus = 1; /* One shot */
  switch (headerType)
  {
    case HDRFORMATTYP_2CH:
      pFrame->frame.headerLength = sizeof(pFrame->frame.header.singlecast);
      pFrame->frame.header.singlecast.header.length = pFrame->frame.headerLength + pFrame->frame.payloadLength;
      break;
    case HDRFORMATTYP_3CH:
      pFrame->frame.headerLength = (sizeof(pFrame->frame.header.singlecast3ch) - sizeof(pFrame->frame.header.singlecast3ch.extension));
      pFrame->frame.header.singlecast3ch.header.length = pFrame->frame.headerLength + pFrame->frame.payloadLength;
      break;
    case HDRFORMATTYP_LR:
    case HDRFORMATTYP_UNDEFINED:
      break;
  }
  DPRINTF("BuildTxExploreHeader: pFrame->frame.headerLength = %02X\n", pFrame->frame.headerLength);
}

/*============================   BuildTxHeader   ============================
**    Build the specified frame header.
**
**    Prerequisites: Repeaterlist (if routed frame) must be copied to pFrame
**      in advance.
**
**    Note: The FRAME_TYPE_SINGLE is in 2ch (among other things) used to send
**    Routed ACKs. FRAME_TYPE_ROUTED is not used for this purpose.
**
**    Side effects:
**
**--------------------------------------------------------------------------*/

void BuildTxHeader(
    ZW_HeaderFormatType_t headerType, /*IN header type of the transmitted frame*/
    uint8_t frameType,                /* IN Frame type (FRAMETYPE_*, lower nibble), seq no. (upper nibble)    */
    uint8_t bSequenceNumber,          /* IN Sequence number of the frame that should be transmitted */
    TxQueueElement *pFrame)           /* IN Frame pointer               */
{
  /* Mask out the possible sequence number in the upper nibble */
  uint8_t bframeType = frameType & MASK_FRAMETYPE_FRAMETYPE;
  /* Default we set the sequence number in transmitQueue[0] but if frame is ACK */
  /* we change the sequence number to the sequence number received in latest frame */
  if (HDRFORMATTYP_2CH == headerType)
  {
    pFrame->frame.frameOptions.sequenceNumber = ((frameType & MASK_FRAMETYPE_SEQNO) >> 4);
  }
  else if (HDRFORMATTYP_LR == headerType)
  {
    pFrame->frame.frameOptions.sequenceNumber = bSequenceNumber;
  }
  else
  {
    /* Set followup bit */
    pFrame->frame.frameOptions.multicastfollowup = (0 != (pFrame->bFrameOptions1 & TRANSMIT_FRAME_OPTION_FOLLOWUP));
    /* Repeated frames are queued as singlecast frame fix it here */
    /* TODO: this should not be fixed here but rather where the frame is queued */
    if ((bframeType == FRAME_TYPE_SINGLE) && (pFrame->bFrameOptions & TRANSMIT_OPTION_ROUTED))
    {
      frameType = FRAME_TYPE_INTERNAL_REPEATED;
    }
    if (!((frameType == FRAME_TYPE_EXPLORE) &&
          (pFrame->bFrameOptions & TRANSMIT_OPTION_EXPLORE_REPEAT)))
    {
      /* Clear options field in header before beginning */
      pFrame->frame.header.header3ch.headerInfo = 0;
      pFrame->frame.header.header3ch.headerInfo2 = 0;
      /* Set sequence number */
      pFrame->frame.frameOptions.sequenceNumber = bSequenceNumber;
      /* Set header type */
      pFrame->frame.frameOptions.frameType = frameType;
    }
  }

  /* Set home ID in frame */
  if (bframeType != FRAME_TYPE_EXPLORE)
  {
    SetTransmitHomeID(pFrame);
  }
  /* Set low power if requested */
  if (pFrame->bFrameOptions & TRANSMIT_OPTION_LOW_POWER)
  {
    pFrame->frame.frameOptions.lowPower = 1;
  }

  switch (bframeType)
  {
    case FRAME_TYPE_ACK:
      BuildTxAckHeader(headerType, pFrame);
      break;

    case FRAME_TYPE_SINGLE:
      BuildTxSingleHeader(headerType, pFrame);
      break;
    case FRAME_TYPE_ROUTED:
      BuildTxRoutedHeader(headerType, pFrame);
      break;
    case FRAME_TYPE_MULTI:
      BuildTxMultiHeader(headerType, pFrame);
      break;
    case FRAME_TYPE_EXPLORE:
      ASSERT(HDRFORMATTYP_LR != headerType);
      BuildTxExploreHeader(headerType, pFrame);
      break;
    default:
      break;
  }
}
