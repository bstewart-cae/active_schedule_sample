// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file ZW_Frame.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include "ZW_Frame.h"
#include "Assert.h"

__attribute__((used)) bool IsSingleCast(ZW_HeaderFormatType_t headerType, const frame *pFrame)
{
  switch (headerType)
  {
    /* fallthrough */
    case HDRFORMATTYP_2CH:
    case HDRFORMATTYP_3CH:
      return IS_SINGLECAST(*pFrame);

    case HDRFORMATTYP_LR:
      return IS_SINGLECAST_LR(*pFrame);

    default:
      return false;
  }
}

__attribute__((used)) bool IsMultiCast(ZW_HeaderFormatType_t headerType, const frame *pFrame) 
{
  switch (headerType)
  {
    /* fallthrough */
    case HDRFORMATTYP_2CH:
    case HDRFORMATTYP_3CH:
      return IS_MULTICAST(*pFrame);

    default:
      return false;
  }
}

__attribute__((used)) bool IsExplore(ZW_HeaderFormatType_t headerType, const frame *pFrame)
{
  switch (headerType)
  {
    /* fallthrough */
    case HDRFORMATTYP_2CH:
    case HDRFORMATTYP_3CH:
      return ((pFrame->header.headerInfo & MASK_HDRTYP) == HDRTYP_EXPLORE);

    default:
      return false;
  }
}

__attribute__((used)) bool IsTransferAck(ZW_HeaderFormatType_t headerType, const frame *pFrame)
{
  switch (headerType)
  {
    /* fallthrough */
    case HDRFORMATTYP_2CH:
    case HDRFORMATTYP_3CH:
      return IS_TRANSFERACK(*pFrame);

    case HDRFORMATTYP_LR:
      return IS_TRANSFERACK_LR(*pFrame);

    default:
      return false;
  }
}


__attribute__((used)) bool DoAck(ZW_HeaderFormatType_t headerType, const frame *pFrame)
{
  if (HDRFORMATTYP_3CH == headerType)
  {
    return DO_ACK_3CH(*pFrame);
  }
  else if (HDRFORMATTYP_2CH == headerType)
  {
    return DO_ACK(*pFrame);
  }
  else if (HDRFORMATTYP_LR == headerType)
  {
    return DO_ACK_LR(*pFrame);
  }
  return false;
}

__attribute__((used)) bool IsAck(ZW_HeaderFormatType_t headerType, const frame *pFrame)
{
  if (HDRFORMATTYP_3CH == headerType)
  {
    return IS_ACK_3CH(*pFrame);
  }
  else if (HDRFORMATTYP_2CH == headerType)
  {
    return IS_ACK(*pFrame);
  }
  else if (HDRFORMATTYP_LR == headerType)
  {
    return IS_ACK_LR(*pFrame);
  }
  return false;
}


__attribute__((used)) bool IsLowPower(ZW_HeaderFormatType_t headerType, const frame *pFrame)
{
  if (HDRFORMATTYP_3CH == headerType)
  {
    return IS_LOWPOWER_3CH(*pFrame);
  }
  else if (HDRFORMATTYP_2CH == headerType)
  {
    return IS_LOWPOWER(*pFrame);
  }
  else if (HDRFORMATTYP_LR == headerType)
  {
    return IS_LOWPOWER_LR(*pFrame);
  }
  else {return 0;}
}

__attribute__((used)) bool IsRouted(ZW_HeaderFormatType_t headerType, const frame *pFrame)
{
  if (HDRFORMATTYP_LR == headerType)
  {
    return IS_ROUTED_LR(*pFrame);
  }
  else if (HDRFORMATTYP_3CH == headerType)
  {
    return IS_ROUTED_3CH(*pFrame);
  }
  else if (HDRFORMATTYP_2CH == headerType)
  {
    return IS_ROUTED(*pFrame);
  }
  else
  {
    return 0;
  }

}

__attribute__((used)) bool IsRouteErr(ZW_HeaderFormatType_t headerType, const frame *pFrame)
{
  if (HDRFORMATTYP_3CH == headerType)
  {
    return IS_ROUTEERR_3CH(*pFrame);
  }
  else if (HDRFORMATTYP_2CH == headerType)
  {
    return IS_ROUTEERR(*pFrame);
  }
  else
  {
    return 0;
  }

}

__attribute__((used)) bool IsRouteAckErr(ZW_HeaderFormatType_t headerType, const frame *pFrame)
{
  if (HDRFORMATTYP_3CH == headerType)
  {
    return IS_ROUTEACKERR_3CH(*pFrame);
  }
  else if (HDRFORMATTYP_2CH == headerType)
  {
    return IS_ROUTEACKERR(*pFrame);
  }
  else
  {
    return 0;
  }
}

__attribute__((used)) uint8_t GetHeaderType(ZW_HeaderFormatType_t headerType, const frame *pFrame)
{
  switch (headerType)
  {
    /* fallthrough */
    case HDRFORMATTYP_2CH:
    case HDRFORMATTYP_3CH:
      return GET_HEADERTYPE(*pFrame);

    case HDRFORMATTYP_LR:
      return GET_HEADERTYPE_LR(*pFrame);

    default:
      return 0;
  }
}

__attribute__((used)) void SetHeaderType(ZW_HeaderFormatType_t headerType, frameTx *pFrame, uint8_t type)
{
  switch (headerType)
  {
    /* fallthrough */
    case HDRFORMATTYP_2CH:
    case HDRFORMATTYP_3CH:
      SET_HEADERTYPE(*pFrame, type);
      break;

    case HDRFORMATTYP_LR:
      SET_HEADERTYPE_LR(*pFrame, type);
      break;
      
    default:
      break;
  }
}

__attribute__((used)) void SetAck(ZW_HeaderFormatType_t headerType, frameTx *pFrame)
{
  if (HDRFORMATTYP_3CH == headerType)
  {
    SET_ACK_3CH(*pFrame);
  }
  else if (HDRFORMATTYP_2CH == headerType)
  {
    SET_ACK(*pFrame);
  }
  else if (HDRFORMATTYP_LR == headerType)
  {
    SET_ACK_LR(*pFrame);
  }
}

__attribute__((used)) void SetInComming(ZW_HeaderFormatType_t headerType, frameTx *pFrame)
{
  if (HDRFORMATTYP_3CH == headerType)
  {
    SET_INCOMING_3CH(*pFrame);
  }
  else if (HDRFORMATTYP_2CH == headerType)
  {
    SET_INCOMING(*pFrame);
  }
}

__attribute__((used)) void SetOutGoing(ZW_HeaderFormatType_t headerType, frameTx *pFrame)
{
  if (HDRFORMATTYP_3CH == headerType)
  {
    SET_OUTGOING_3CH(*pFrame);
  }
  else if (HDRFORMATTYP_2CH == headerType)
  {
    SET_OUTGOING(*pFrame);
  }
}

__attribute__((used)) void SetRouteAck(ZW_HeaderFormatType_t headerType, frameTx *pFrame)
{
  if (HDRFORMATTYP_3CH == headerType)
  {
    SET_ROUTE_ACK_3CH(*pFrame);
  }
  else if (HDRFORMATTYP_2CH == headerType)
  {
    SET_ROUTE_ACK(*pFrame);
  }
}

__attribute__((used)) void SetRouteErr(ZW_HeaderFormatType_t headerType, frameTx *pFrame)
{
  if (HDRFORMATTYP_3CH == headerType)
  {
    SET_ROUTE_ERR_3CH(*pFrame);
  }
  else if (HDRFORMATTYP_2CH == headerType)
  {
    SET_ROUTE_ERR(*pFrame);
  }
}

__attribute__((used)) uint8_t GetRouteLen(ZW_HeaderFormatType_t headerFormatType, const frame *pFrame)
{
  if (HDRFORMATTYP_3CH == headerFormatType)
  {
    return GET_ROUTE_LEN_3CH(*pFrame);
  }
  else if (HDRFORMATTYP_2CH == headerFormatType)
  {
    return GET_ROUTE_LEN(*pFrame);
  }
  else
  {
    return 0;
  }
}

__attribute__((used)) uint8_t GetHops(ZW_HeaderFormatType_t headerType, const frame *pFrame)
{
  if (HDRFORMATTYP_3CH == headerType)
  {
    return GET_HOPS_3CH(*pFrame);
  }
  else if (HDRFORMATTYP_2CH == headerType)
  {
    return GET_HOPS(*pFrame);
  }
  else
  {
    return 0;
  }
}

__attribute__((used)) bool IsOutgoing(ZW_HeaderFormatType_t headerType, const frame *pFrame)
{
  if (HDRFORMATTYP_3CH == headerType)
  {
    return !NOT_OUTGOING_3CH(*pFrame);
  }
  else if (HDRFORMATTYP_2CH == headerType)
  {
    return !NOT_OUTGOING(*pFrame);
  }
  else
  {
    return false;
  }
}

__attribute__((used)) bool NotOutgoing(ZW_HeaderFormatType_t headerType, const frame *pFrame)
{
  if (HDRFORMATTYP_3CH == headerType)
  {
    return NOT_OUTGOING_3CH(*pFrame);
  }
  else if (HDRFORMATTYP_2CH == headerType)
  {
    return NOT_OUTGOING(*pFrame);
  }
  else
  {
    return false;
  }
}

__attribute__((used)) void SetRouteLen(ZW_HeaderFormatType_t headerType, frameTx *pFrame, uint8_t len)
{
  if (HDRFORMATTYP_3CH == headerType)
  {
    SET_ROUTE_LEN_3CH(*pFrame, len);
  }
  else if (HDRFORMATTYP_2CH == headerType)
  {
    SET_ROUTE_LEN(*pFrame, len);
  }
}

__attribute__((used)) void SetHops(ZW_HeaderFormatType_t headerType, frameTx *pFrame, uint8_t hops)
{
  if (HDRFORMATTYP_3CH == headerType)
  {
    SET_HOPS_3CH(*pFrame, hops);
  }
  else if (HDRFORMATTYP_2CH == headerType)
  {
    SET_HOPS(*pFrame, hops);
  }
}

__attribute__((used)) bool IsRoutedAck(ZW_HeaderFormatType_t headerType, const frame *pFrame)
{
  if (HDRFORMATTYP_3CH == headerType)
  {
    return IS_ROUTEACK_3CH(*pFrame);
  }
  else if (HDRFORMATTYP_2CH == headerType)
  {
    return IS_ROUTEACK(*pFrame);
  }
  else
  {
    return false;
  }
}

__attribute__((used)) bool IsFrameSingleCast(ZW_HeaderFormatType_t headerType, const frame *pFrame)
{
  if (HDRFORMATTYP_3CH == headerType)
  {
    return IS_FRAME_SINGLECAST_3CH(*pFrame);
  }
  else if (HDRFORMATTYP_2CH == headerType)
  {
    return IS_FRAME_SINGLECAST(*pFrame);
  }
  else if (HDRFORMATTYP_LR == headerType)
  {
   return IS_FRAME_SINGLECAST_LR(*pFrame);
  }
  else
  {
    return false;
  }
}

__attribute__((used)) bool IsFrameRouted(ZW_HeaderFormatType_t headerType, const frame *pFrame)
{
  if (HDRFORMATTYP_LR == headerType)
  {
    return IS_FRAME_ROUTED_LR(*pFrame);
  }
  else if (HDRFORMATTYP_3CH == headerType)
  {
    return IS_FRAME_ROUTED_3CH(*pFrame);
  }
  else if (HDRFORMATTYP_2CH == headerType)
  {
    return IS_FRAME_ROUTED(*pFrame);
  }
  else
  {
    return false;
  }
}

__attribute__((used)) bool IsFrameAck(ZW_HeaderFormatType_t headerType, const frame *pFrame)
{
  if (HDRFORMATTYP_3CH == headerType)
  {
    return IS_FRAME_ACK_3CH(*pFrame);
  }
  else if (HDRFORMATTYP_2CH == headerType)
  {
    return IS_FRAME_ACK(*pFrame);
  }
  else if (HDRFORMATTYP_LR == headerType)
  {
    return IS_FRAME_ACK_LR(*pFrame);
  }
  else
  {
    return false;
  }
}

__attribute__((used)) uint32_t GetSeqNumber(ZW_HeaderFormatType_t headerType, const frame *pFrame)
{
  if (HDRFORMATTYP_LR == headerType)
  {
    return GET_SEQNUMBER_LR(*pFrame);
  }
  else if (HDRFORMATTYP_3CH == headerType)
  {
    return GET_SEQNUMBER_3CH(*pFrame);
  }
  else if (HDRFORMATTYP_2CH == headerType)
  {
    return GET_SEQNUMBER(*pFrame);
  }
  else
  {
    return 0;
  }
}

__attribute__((used)) void SetRouted(ZW_HeaderFormatType_t headerType, frameTx *pFrame)
{
  if (HDRFORMATTYP_3CH == headerType)
  {
    SET_ROUTED_3CH(*pFrame);
  }
  else if (HDRFORMATTYP_2CH == headerType)
  {
    SET_ROUTED(*pFrame);
  }
}

__attribute__((used)) void ClrRouted(ZW_HeaderFormatType_t headerType, frameTx *pFrame)
{
  switch (headerType)
  {
    /* fallthrough */
    case HDRFORMATTYP_2CH:
    case HDRFORMATTYP_3CH:
      SET_NOT_ROUTED(*pFrame);
      break;

    default:
      break;
  }
}

/* Get the nodeID of a repeater in the route */
__attribute__((used)) uint8_t ReadRepeater(ZW_HeaderFormatType_t headerType, const frame *pFrame, uint8_t number)
{
  if (HDRFORMATTYP_3CH == headerType)
  {
    return READ_REPEATER_3CH(*pFrame,number);
  }
  else if (HDRFORMATTYP_2CH == headerType)
  {
    return READ_REPEATER(*pFrame, number);
  }
  else
  {
    return 0;
  }
}

/* Set the nodeID of a repeater in the route */
__attribute__((used)) void SetRepeater(ZW_HeaderFormatType_t headerType, frameTx *pFrame, uint8_t number, uint8_t id)
{
  if (HDRFORMATTYP_3CH == headerType)
  {
    SET_REPEATER_3CH(*pFrame,number,id);
  }
  else if (HDRFORMATTYP_2CH == headerType)
  {
    (pFrame->singlecastRouted.repeaterList[number] = id);
  }
}

/* Get the Payload as an array */
__attribute__((used)) uint8_t * GetSinglecastPayload(ZW_HeaderFormatType_t headerType, frame *pFrame)
{
  if (HDRFORMATTYP_LR == headerType)
  {
    return SINGLECAST_PAYLOAD_LR(pFrame);
  }
  else if (HDRFORMATTYP_3CH == headerType)
  {
    return SINGLECAST_PAYLOAD_3CH(pFrame);
  }
  else if (HDRFORMATTYP_2CH == headerType)
  {
    return (&pFrame->singlecast.destinationID + 1);
  }
  else
  {
    return 0;
  }
}

/* Get the offset of a multicast frame */
__attribute__((used)) uint8_t GetMulticastOffset(ZW_HeaderFormatType_t headerType, const frame *pFrame)
{
  if (HDRFORMATTYP_3CH == headerType)
  {
    return MULTICAST_OFFSET_3CH(*pFrame);
  }
  else if (HDRFORMATTYP_2CH == headerType)
  {
    return MULTICAST_OFFSET(*pFrame);
  }
  else
  {
    return 0;
  }
}

/* Number of addres bytes in destination mask */
__attribute__((used)) uint8_t GetMulticastAddrLen(ZW_HeaderFormatType_t headerType, const frame *pFrame)
{
  if (HDRFORMATTYP_3CH == headerType)
  {
    return MULTICAST_ADDRESS_LEN_3CH(*pFrame);
  }
  else if (HDRFORMATTYP_2CH == headerType)
  {
    return MULTICAST_ADDRESS_LEN(*pFrame);
  }
  else
  {
    return 0;
  }
}

/* Get the ReceiverMask as an array */
__attribute__((used)) uint8_t * GetReceiveMask(ZW_HeaderFormatType_t headerType, frame *pFrame)
{
  if (HDRFORMATTYP_3CH == headerType)
  {
    return RECEIVER_MASK_3CH(*pFrame);
  }
  else if (HDRFORMATTYP_2CH == headerType)
  {
    return RECEIVER_MASK(*pFrame);
  }
  else
  {
    return 0;
  }
}

/* Defines for Wakeup Beam settings for the source, set in the reserved byte in frame */
__attribute__((used)) void SetWakeupBeamSpeedSrc(ZW_HeaderFormatType_t headerType, frameTx *pFrame, uint8_t wakeupspeed)
{
  switch (headerType)
  {
    case HDRFORMATTYP_3CH:
      SET_WAKEUP_BEAM_SPEED_SRC_3CH(pFrame->singlecast3ch, wakeupspeed);
      break;

    /* fallthrough */
    case HDRFORMATTYP_2CH:
    case HDRFORMATTYP_LR:
      SET_WAKEUP_BEAM_SPEED_SRC(*pFrame, wakeupspeed);
      break;

    default:
      break;
  }
}

__attribute__((used)) void ClrWakeupBeamSpeedSrc(ZW_HeaderFormatType_t headerType, frameTx *pFrame)
{
  switch (headerType)
  {
    case HDRFORMATTYP_3CH:
      CLR_WAKEUP_BEAM_SPEED_SRC_3CH(pFrame->singlecast3ch);
      break;

    /* fallthrough */
    case HDRFORMATTYP_2CH:
    case HDRFORMATTYP_LR:
      CLR_WAKEUP_BEAM_SPEED_SRC(*pFrame);
      break;

    default:
      break;
  }
}

__attribute__((used)) uint8_t GetWakeupBeamSpeedSrc(ZW_HeaderFormatType_t headerType, const frame *pFrame)
{
  switch (headerType)
  {
    case HDRFORMATTYP_3CH:
      return GET_WAKEUP_BEAM_SPEED_SRC_3CH(pFrame->singlecast3ch);

    /* fallthrough */
    case HDRFORMATTYP_2CH:
    case HDRFORMATTYP_LR:
      return GET_WAKEUP_BEAM_SPEED_SRC(*pFrame);

    default:
      return 0;
  }
}

/* Defines for setting/getting and clearing the Extended route header body present bit */
/* This bit, if true, indicates if an extended route header body is present and placed */
/* inbetween the route header and the payload in a routed frame*/
__attribute__((used)) void SetExtendPresent(ZW_HeaderFormatType_t headerType, frameTx *pFrame)
{
  switch (headerType)
  {
    case HDRFORMATTYP_3CH:
      SET_EXTEND_PRESENT_3CH(*pFrame);
      break;

    /* fallthrough */
    case HDRFORMATTYP_2CH:
    case HDRFORMATTYP_LR:
      SET_EXTEND_PRESENT(*pFrame);
      break;

    default:
      break;
  }
}

__attribute__((used)) void ClrExtendPresent(ZW_HeaderFormatType_t headerType, frame *pFrame)
{
  switch (headerType)
  {
    case HDRFORMATTYP_3CH:
      CLR_EXTEND_PRESENT_3CH(*pFrame);
      break;

    /* fallthrough */
    case HDRFORMATTYP_2CH:
    case HDRFORMATTYP_LR:
      CLR_EXTEND_PRESENT(*pFrame);
      break;

    default:
      break;
  }
}

__attribute__((used)) uint8_t GetExtendPresent(ZW_HeaderFormatType_t headerType, const frame *pFrame)
{
  if (HDRFORMATTYP_3CH == headerType)
  {
    return GET_EXTEND_PRESENT_3CH(*pFrame);
  }
  else if (HDRFORMATTYP_2CH == headerType)
  {
    return GET_EXTEND_PRESENT(*pFrame);
  }
  else if (HDRFORMATTYP_LR == headerType)
  {
    return GET_EXTEND_PRESENT_LR(*pFrame);
  }
  else
  {
    return 0;
  }
}

/* Macros for Extend body handling to be used when accessing via activeTransmit header */
__attribute__((used)) void SetExtendTypeHeader(ZW_HeaderFormatType_t headerType, frameTx *pFrame, uint8_t extendType)
{
  switch (headerType)
  {
    /* fallthrough */
    case HDRFORMATTYP_2CH:
    case HDRFORMATTYP_LR:
      SET_EXTEND_TYPE_HEADER(*pFrame, extendType);
      break;

    default:
      break;
  }
}

__attribute__((used)) uint8_t GetExtendTypeHeader(ZW_HeaderFormatType_t headerType, const frameTx *pFrame)
{
  switch (headerType)
  {
    /* fallthrough */
    case HDRFORMATTYP_2CH:
    case HDRFORMATTYP_LR:
      return GET_EXTEND_TYPE_HEADER(*pFrame);

    default:
      return 0;
  }
}

__attribute__((used)) void SetExtendLenHeader(ZW_HeaderFormatType_t headerType, frameTx *pFrame, uint8_t len)
{
  switch (headerType)
  {
    /* fallthrough */
    case HDRFORMATTYP_2CH:
    case HDRFORMATTYP_LR:
      SET_EXTEND_LENGTH_HEADER(*pFrame, len);
      break;

    default:
      break;
  }
}

__attribute__((used)) uint8_t GetExtendLenHeader(ZW_HeaderFormatType_t headerType, const frameTx *pFrame)
{
  switch (headerType)
  {
    /* fallthrough */
    case HDRFORMATTYP_2CH:
    case HDRFORMATTYP_LR:
      return GET_EXTEND_LENGTH_HEADER(*pFrame);

    default:
      return 0;
  }
}

__attribute__((used)) void SetExtendBodyHeader(ZW_HeaderFormatType_t headerType, frameTx *pFrame,uint8_t offset,  uint8_t val)
{
  if (HDRFORMATTYP_2CH == headerType)
  {
    SET_EXTEND_BODY_HEADER(*pFrame, offset, val);
  }
}

__attribute__((used)) uint8_t GetExtendBodyHeader(ZW_HeaderFormatType_t headerType, const frameTx *pFrame, uint8_t offset)
{
  if (HDRFORMATTYP_2CH == headerType)
  {
    return GET_EXTEND_BODY_HEADER(*pFrame, offset);
  }
  else
  {
    return 0;
  }
}

__attribute__((used)) static uint8_t * skip_header_3ch(frameHeaderSinglecastRouted3ch *singlecastRouted3ch)
{
  uint8_t *p = (uint8_t *)singlecastRouted3ch;
  /* skip over basis header and constant part of routing header */
  p += sizeof(frameHeader3ch) + ROUTING_HEADER_CONSTANT_LENGTH_3CH;
  p += (singlecastRouted3ch->numRepsNumHops >> 4); /* skip over variable part of routing header */

  return p;
}

/* Macros for Extend body handling to be used when accessing via received frame */
__attribute__((used)) void SetExtendType(ZW_HeaderFormatType_t headerType, frameTx *pFrame, uint8_t extendType)
{
  switch (headerType)
  {
    case HDRFORMATTYP_3CH:
    {
      uint8_t *p = skip_header_3ch(&pFrame->singlecastRouted3ch);
      *p = (uint8_t)(extendType << 4);
    }
    break;

    /* fallthrough */
    case HDRFORMATTYP_2CH:
    case HDRFORMATTYP_LR:
      SET_EXTEND_TYPE(*pFrame, extendType);
      break;

    default:
      break;
  }
}

__attribute__((used)) uint8_t GetExtendType(ZW_HeaderFormatType_t headerType, frame *pFrame)
{
  switch (headerType)
  {
    case HDRFORMATTYP_3CH:
    {
      const uint8_t *p = skip_header_3ch(&pFrame->singlecastRouted3ch);
      return (*p >> 4);
    }

    /* fallthrough */
    case HDRFORMATTYP_2CH:
    case HDRFORMATTYP_LR:
      return GET_EXTEND_TYPE(*pFrame);

    default:
      return 0;
  }
}

/* GET_EXTEND_TYPE2 is for TxQueueElement->sFrameheader, GET_EXTEND_TYPE is for RX_FRAME.*/
__attribute__((used)) uint8_t GetExtendType2(ZW_HeaderFormatType_t headerType, const frameTx *pFrame)
{
  switch (headerType)
  {
    case HDRFORMATTYP_3CH:
      return GET_EXTEND_TYPE2_3CH(*pFrame);

    /* fallthrough */
    case HDRFORMATTYP_2CH:
    case HDRFORMATTYP_LR:
      return GET_EXTEND_TYPE2(*pFrame);

    default:
      return 0;
  }
}

__attribute__((used)) void SetExtendLength(ZW_HeaderFormatType_t headerType, frameTx *pFrame, uint8_t extendLength)
{
  switch (headerType)
  {
    case HDRFORMATTYP_3CH:
    {
      uint8_t *p = skip_header_3ch(&pFrame->singlecastRouted3ch);
      *p = (*p & ~MASK_EXTENSION_LENGTH_3CH) | (extendLength & MASK_EXTENSION_LENGTH_3CH);
    }
    break;

    /* fallthrough */
    case HDRFORMATTYP_2CH:
    case HDRFORMATTYP_LR:
      SET_EXTEND_LENGTH(*pFrame, extendLength);
      break;

    default:
      break;
  }
}

/* fix Routing in 1000/250ms populated FLiRS networks can result in 1000ms wakeup beams to 250ms FLiRS */
__attribute__((used)) void SetExtendBody(ZW_HeaderFormatType_t headerType, frameTx *pFrame, uint8_t offset, uint8_t value)
{
  switch (headerType)
  {
    case HDRFORMATTYP_3CH:
    {
      uint8_t *p = skip_header_3ch(&pFrame->singlecastRouted3ch);
      p += 1; /* skip over extension type/length byte */
      p += offset;
      *p = value;
    }
    break;

    /* fallthrough */
    case HDRFORMATTYP_2CH:
    case HDRFORMATTYP_LR:
      SET_EXTEND_BODY(*pFrame, offset, value);
      break;

    default:
      break;
  }
}

__attribute__((used)) uint8_t GetExtendBody(ZW_HeaderFormatType_t headerType, const frame *pFrame, uint8_t offset)
{
  switch (headerType)
  {
    /* fallthrough */
    case HDRFORMATTYP_2CH:
    case HDRFORMATTYP_LR:
      return GET_EXTEND_BODY(*pFrame, offset);

    default:
      return 0;
  }
}

__attribute__((used)) uint8_t GetRoutedHeaderExtensionLen(ZW_HeaderFormatType_t headerType,  const frame *pFrame)
{
  switch (headerType)
  {
    case HDRFORMATTYP_3CH:
      return GET_ROUTED_SINGLECAST_HEADER_EXTENSION_LENGTH_3CH(*pFrame);

    /* fallthrough */
    case HDRFORMATTYP_2CH:
    case HDRFORMATTYP_LR:
      return GET_ROUTED_SINGLECAST_HEADER_EXTENSION_LENGTH(*pFrame);

    default:
      return 0;
  }
}

__attribute__((used)) uint8_t GetRoutedAckHeaderExtensionLen(ZW_HeaderFormatType_t headerType,  const frame *pFrame)
{
  switch (headerType)
  {
    case HDRFORMATTYP_3CH:
      return GET_ROUTED_ACK_HEADER_EXTENSION_LENGTH_3CH(*pFrame);

    /* fallthrough */
    case HDRFORMATTYP_2CH:
    case HDRFORMATTYP_LR:
      return GET_ROUTED_SINGLECAST_HEADER_EXTENSION_LENGTH(*pFrame);

    default:
      return 0;
  }
}

/* Get pointer to extend body */
__attribute__((used)) uint8_t * GetExtendBodyAddr(ZW_HeaderFormatType_t headerType, frame *pFrame, uint8_t offset)
{
  switch (headerType)
  {
    case HDRFORMATTYP_3CH:
    {
      uint8_t *p = skip_header_3ch(&pFrame->singlecastRouted3ch);
      p += 1; /* skip over extension type/length byte */
      p += offset;
      return p;
    }

    /* fallthrough */
    case HDRFORMATTYP_2CH:
    case HDRFORMATTYP_LR:
      return GET_EXTEND_BODY_ADDR(*pFrame, offset);

    default:
      return 0;
  }
}

/* Get the Payload as an array */
__attribute__((used)) uint8_t * GetRoutedPayload(ZW_HeaderFormatType_t headerType, frame *pFrame)
{
  if (HDRFORMATTYP_3CH == headerType)
  {
    return SINGLECAST_ROUTED_PAYLOAD_3CH(*pFrame);
  }
  else if (HDRFORMATTYP_2CH == headerType)
  {
    return SINGLECAST_ROUTED_PAYLOAD(*pFrame);
  }
  else
  {
    return 0;
  }
}

/* Get the number of bytes in the Payload */
__attribute__((used)) uint8_t GetRoutedPayloadLen(ZW_HeaderFormatType_t headerType, const frame *pFrame)
{
  if (HDRFORMATTYP_3CH == headerType)
  {
    return SINGLECAST_ROUTED_PAYLOAD_LEN_3CH(*pFrame);
  }
  else if (HDRFORMATTYP_2CH == headerType)
  {
    return SINGLECAST_ROUTED_PAYLOAD_LEN(*pFrame);
  }
  else
  {
    return 0;
  }
}

/* Get the Payload as an array */
__attribute__((used)) uint8_t * GetExplorePayload(ZW_HeaderFormatType_t headerType, frame *pFrame)
{
  if (HDRFORMATTYP_3CH == headerType)
  {
    return EXPLORE_PAYLOAD_3CH(*pFrame);
  }
  else if (HDRFORMATTYP_2CH == headerType)
  {
    return EXPLORE_PAYLOAD(*pFrame);
  }
  else
  {
    return 0;
  }
}

/* Get the number of bytes in the payload*/
__attribute__((used)) uint8_t GetExplorePayloadLen(ZW_HeaderFormatType_t headerType, const frame *pFrame)
{
  if (HDRFORMATTYP_3CH == headerType)
  {
    return EXPLORE_PAYLOAD_LEN_3CH(*pFrame);
  }
  else if (HDRFORMATTYP_2CH == headerType)
  {
    return EXPLORE_PAYLOAD_LEN(*pFrame);
  }
  else
  {
    return 0;
  }

}

/* Get the Payload as an array */
__attribute__((used)) uint8_t * GetMulticastPayload(ZW_HeaderFormatType_t headerType, frame *pFrame)
{
  if (HDRFORMATTYP_3CH == headerType)
  {
    return MULTICAST_PAYLOAD_3CH(pFrame);
  }
  else if (HDRFORMATTYP_2CH == headerType)
  {
    return MULTICAST_PAYLOAD(*pFrame);
  }
  else
  {
    return 0;
  }

}

/* Get the number of bytes in the payload*/
__attribute__((used)) uint8_t GetMulticastPayloadLen(ZW_HeaderFormatType_t headerType, const frame *pFrame)
{
  if (HDRFORMATTYP_3CH == headerType)
  {
    return MULTICAST_PAYLOAD_LENGTH_3CH(pFrame);
  }
  else if (HDRFORMATTYP_2CH == headerType)
  {
    return MULTICAST_PAYLOAD_LEN(*pFrame);
  }
  else
  {
    return 0;
  }

}

__attribute__((used)) uint8_t * GetRoutedAckPayload(ZW_HeaderFormatType_t headerType, frame *pFrame)
{
  if (HDRFORMATTYP_3CH == headerType)
  {
    return ROUTED_ACK_PAYLOAD_3CH(*pFrame);
  }
  else if (HDRFORMATTYP_2CH == headerType)
  {
    return NULL;
  }
  else
  {
    return NULL;
  }
}

__attribute__((used)) uint8_t GetRoutedAckPayloadLen(ZW_HeaderFormatType_t headerType, const frame *pFrame)
{
  if (HDRFORMATTYP_3CH == headerType)
  {
    return ROUTED_ACK_PAYLOAD_LEN_3CH(*pFrame);
  }
  else if (HDRFORMATTYP_2CH == headerType)
  {
    return 0;
  }
  else
  {
    return 0;
  }
}

/**
 * Returns the source node ID given a header type and a frame.
 *
 * It works for singlecast, multicast and broadcast.
 * @param headerType Type of the header of the frame.
 * @param pFrame Pointer to frame.
 * @return Source node ID
 */
__attribute__((used)) node_id_t GetSourceNodeID(ZW_HeaderFormatType_t headerType, const frame *pFrame)
{
  if (HDRFORMATTYP_LR == headerType)
  {
    return (node_id_t)GET_SINGLECAST_SOURCE_NODEID_LR(*pFrame);
  }
  else if (HDRFORMATTYP_3CH == headerType)
  {
    return GET_SINGLECAST_SOURCE_NODEID_3CH(*pFrame);
  }
  else if (HDRFORMATTYP_2CH == headerType)
  {
    return GET_SINGLECAST_SOURCE_NODEID_2CH(*pFrame);
  }
  else
  {
    return 0;
  }
}

__attribute__((used)) void SetSourceNodeIDSinglecast(ZW_HeaderFormatType_t headerType, frame *pFrame, node_id_t nodeID)
{
  if (HDRFORMATTYP_LR == headerType)
  {
    SET_SINGLECAST_SOURCE_NODEID_LR(*pFrame, nodeID);
  }
  else if (HDRFORMATTYP_3CH == headerType)
  {
    SET_SINGLECAST_SOURCE_NODEID_3CH(*pFrame, nodeID);
  }
  else if (HDRFORMATTYP_2CH == headerType)
  {
    SET_SINGLECAST_SOURCE_NODEID_2CH(*pFrame, nodeID);
  }
}

__attribute__((used)) node_id_t GetDestinationNodeIDSinglecast(ZW_HeaderFormatType_t headerType, const frame *pFrame)
{
  if (HDRFORMATTYP_LR == headerType)
  {
    return (node_id_t)GET_SINGLECAST_DESTINATION_NODEID_LR(*pFrame);
  }
  else if (HDRFORMATTYP_3CH == headerType)
  {
    return GET_SINGLECAST_DESTINATION_NODEID_3CH(*pFrame);
  }
  else if (HDRFORMATTYP_2CH == headerType)
  {
    return GET_SINGLECAST_DESTINATION_NODEID_2CH(*pFrame);
  }
  else
  {
    return 0;
  }
}

__attribute__((used)) void SetDestinationNodeIDSinglecast(ZW_HeaderFormatType_t headerType, frame *pFrame, node_id_t nodeID)
{
  if (HDRFORMATTYP_LR == headerType)
  {
    SET_SINGLECAST_DESTINATION_NODEID_LR(*pFrame, nodeID);
  }
  else if (HDRFORMATTYP_3CH == headerType)
  {
    SET_SINGLECAST_DESTINATION_NODEID_3CH(*pFrame, nodeID);
  }
  else if (HDRFORMATTYP_2CH == headerType)
  {
    SET_SINGLECAST_DESTINATION_NODEID_2CH(*pFrame, nodeID);
  }
}

#if 0
/* Detect if a routed frame uses beaming. */
uint8_t IsBeamRoute(ZW_HeaderFormatType_t headerType, const frame *pFrame)
{
  if (HDRFORMATTYP_2CH == headerType)
  {
    return IS_BEAM_ROUTE(*pFrame);
  }
}

uint8_t IsRoutedSpeedModified(ZW_HeaderFormatType_t headerType, const frame *pFrame)
{
  if (HDRFORMATTYP_2CH == headerType)
  {
    return IS_ROUTED_SPEED_MODIFIED(*pFrame);
  }
}

void SetRoutedSpeedModified(ZW_HeaderFormatType_t headerType, frameTx *pFrame)
{
 if (HDRFORMATTYP_2CH == headerType)
  {
    SET_ROUTED_SPEED_MODIFIED(*pFrame, len);
  }
}

void ClrRoutedSpeedModified(ZW_HeaderFormatType_t headerType, frameTx *pFrame)
{
  if (HDRFORMATTYP_2CH == headerType)
  {
    CLEAR_ROUTED_SPEED_MODIFIED(*pFrame, len);
  }
}
#endif
