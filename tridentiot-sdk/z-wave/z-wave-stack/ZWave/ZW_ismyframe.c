// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file
 * Check if the frame is addressed for this node.
 * @copyright 2019 Silicon Laboratories Inc.
 */

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include "ZW_lib_defines.h"
#include <string.h>

#ifdef ZW_SLAVE
#include <ZW_slave.h>
#endif /*ZW_SLAVE*/

#ifdef ZW_CONTROLLER
#include <ZW_controller_network_info_storage.h>
#include <ZW_controller.h>
#endif /*ZW_CONTROLLER*/

/*Commmon for all targets*/
#include <ZW_explore.h>
#include "ZW_Frame.h"
#include <zpal_radio.h>

#include <ZW_ismyframe.h>
#include <ZW_lr_virtual_node_id.h>
//#define DEBUGPRINT
#include "DebugPrint.h"
#define LR_PROTOCOL_CMDS_NUM   5
extern bool bUseExploreAsRouteResolution;
extern uint8_t crH[HOMEID_LENGTH];  /* HomeID on frame just received */

extern uint8_t inclusionHomeID[HOMEID_LENGTH];
extern bool inclusionHomeIDActive;

#ifdef ZW_CONTROLLER_BRIDGE
extern node_id_t virtualNodeID;                 /* Current virtual nodeID */
extern uint8_t learnSlaveMode;
extern bool virtualNodeDest;
#endif  /* ZW_CONTROLLER_BRIDGE */
extern uint8_t sACK;
extern TxQueueElement *pFrameWaitingForACK;
extern uint8_t currentSeqNo;
extern uint8_t mTransportRxCurrentSpeed;
static uint8_t myFrame;

static uint8_t * mpReceiveStatus;

/* currentSeqNoToMatch bit layout: xy00zzzz, where x=40k speed, y = routed frame, zzzz = seq. no */
uint8_t currentSeqNoToMatch;

#ifdef ZW_CONTROLLER

typedef struct _LONG_RANGE_SINGLECAST_REMOTE_INCLUSION_REQUEST_FRAME_
{
  CMD_CLASS_HEADER      header;
  uint8_t               capability;             /* Network capabilities */
  uint8_t               security;               /* Network security */
  uint8_t               reserved;
  NODE_TYPE             nodeType;               /* Basic, Generic and Specific Device Type */
  uint8_t               nodeInfo[NODEPARM_MAX]; /* Device capabilities */
} LONG_RANGE_SINGLECAST_REMOTE_INCLUSION_REQUEST_FRAME;

/* INIF - The Payload parsed by IsMyExploreFrame - An EXPLORE_CMD_AUTOINCLUSION frame */
typedef struct _LONG_RANGE_SINGLECAST_REMOTE_INCLUDED_NODE_INFORMATION_FRAME_
{
  CMD_CLASS_HEADER      header;
  uint8_t               smartStartNWIHomeID[HOMEID_LENGTH]; /* homeID used when using Smart Start */
} LONG_RANGE_SINGLECAST_REMOTE_INCLUDED_NODE_INFORMATION_FRAME;

//static bool isSmartNode(ZW_HeaderFormatType_t curHeader, uint8_t *pHomeID, frame *pFrame)
static bool isSmartNode(ZW_HeaderFormatType_t curHeader, RX_FRAME * pRxFrame)
{
  uint8_t headerType = GetHeaderType(curHeader, pRxFrame->pFrame);

  // Is this a Explorer auto-inclusion packet
  if ((HDRTYP_EXPLORE == headerType)
      && (EXPLORE_CMD_AUTOINCLUSION == (pRxFrame->pFrame->explore.ver_Cmd & EXPLORE_CMD_MASK)))
  {
    void * pTempFrame = GetExplorePayload(curHeader, pRxFrame->pFrame);
    EXPLORE_REMOTE_INCLUSION_REQUEST_FRAME * pRequestFrame = (EXPLORE_REMOTE_INCLUSION_REQUEST_FRAME*)pTempFrame;
    EXPLORE_REMOTE_INCLUDED_NODE_INFORMATION_FRAME * pInfoFrame = (EXPLORE_REMOTE_INCLUDED_NODE_INFORMATION_FRAME*)pTempFrame;

    if (ZWAVE_CMD_SMARTSTART_PRIME_NODE_INFO == pRequestFrame->header.cmd)
    {
      memcpy(inclusionHomeID, &pRxFrame->pFrame->header.homeID, HOMEID_LENGTH);
      *mpReceiveStatus |= RECEIVE_STATUS_SMART_NODE;
      return true;
    }
    else if (ZWAVE_CMD_INCLUDED_NODE_INFO == pInfoFrame->header.cmd)
    {
      /* INIF */
      if (memcmp(&pRxFrame->pFrame->header.homeID, ZW_HomeIDGet(), HOMEID_LENGTH))
      {
        /* Must be foreign HomeID */
        return true;
      }
    }
  }

  if ((HDRTYP_SINGLECAST == headerType) &&
      (HDRFORMATTYP_LR == curHeader) &&
      (NODE_BROADCAST_LR == GET_SINGLECAST_DESTINATION_NODEID_LR(*pRxFrame->pFrame)))
  {
    uint8_t * pTempFrame = GetSinglecastPayload(curHeader, pRxFrame->pFrame);
    LONG_RANGE_SINGLECAST_REMOTE_INCLUSION_REQUEST_FRAME * pRequestFrame = (LONG_RANGE_SINGLECAST_REMOTE_INCLUSION_REQUEST_FRAME *)pTempFrame;
    LONG_RANGE_SINGLECAST_REMOTE_INCLUDED_NODE_INFORMATION_FRAME * pInfoFrame = (LONG_RANGE_SINGLECAST_REMOTE_INCLUDED_NODE_INFORMATION_FRAME *)pTempFrame;

    if ((ZWAVE_CMD_SMARTSTART_PRIME_NODE_INFO == pRequestFrame->header.cmd) ||
        (ZWAVE_LR_CMD_SMARTSTART_PRIME_NODE_INFO == pRequestFrame->header.cmd))
    {
      memcpy(inclusionHomeID, &pRxFrame->pFrame->header.homeID, HOMEID_LENGTH);
      *mpReceiveStatus |= RECEIVE_STATUS_SMART_NODE;
      return true;
    }
    else if ((ZWAVE_CMD_INCLUDED_NODE_INFO == pInfoFrame->header.cmd) ||
             (ZWAVE_LR_CMD_INCLUDED_NODE_INFO == pInfoFrame->header.cmd))
    {
      /* INIF */
      if (memcmp(&pRxFrame->pFrame->header.homeID, ZW_HomeIDGet(), HOMEID_LENGTH))
      {
        /* Must be foreign HomeID */
        return true;
      }
    }
  }
  return false;
}
#endif // ZW_CONTROLLER


/*
 * Returns true if the frame needs to be dropped.
 */
static bool DropLrFrame(RX_FRAME *pRxFrame)
{
  frame *fr = (frame *)pRxFrame->pFrame;
  /* Check if we should process the frame at all */
  if ((GET_HEADERTYPE_LR(*fr) != HDRTYP_SINGLECAST) && (GET_HEADERTYPE_LR(*fr) != HDRTYP_TRANSFERACK))
  {
    /* Illegal header type. Long Range supports Singlecast and TransferAck only */
    DPRINT("Illegal header type. Long Range supports Singlecast and TransferAck only. \n");
    return true;  // Drop frame
  }

#ifdef ZW_SLAVE
  node_id_t srcID = GET_SINGLECAST_SOURCE_NODEID_LR(*fr);
  if ((NODE_RESERVED_BEGIN_LR <= srcID) && (LR_VIRTUAL_ID_INVALID == ZW_Get_lr_virtual_id_type(srcID)))
  {
    // drop frame if source id is not virtual LR and not within the accepted id range
    /* Illegal source nodeID */
    DPRINT("Illegal source nodeID and not virtual LR. \n");
    return true;  // Drop frame
  }

  if (((pRxFrame->status & STATUS_NOT_MY_HOMEID) == 0)
      /* TO#1715 - do not handle any frame which are not for us to handle */
      || (g_learnMode &&
          /* TO#1913, TO#1905 fix - Assign ID frames with homeID = newHomeID must be dumped when not in classic */
          g_learnModeClassic))
   {
     DPRINT("homeID = newHomeID must be dumped when not in classic. \n");
     return false; // Keep frame
   }
#endif /* ZW_SLAVE */

#ifdef ZW_CONTROLLER
  if (GET_SINGLECAST_SOURCE_NODEID_LR(*fr) >= NODE_RESERVED_BEGIN_LR)
  {
    /* Illegal source nodeID */
    DPRINT("Illegal source nodeID. \n");
    return true;  // Drop frame
  }
  bool rule1 = (g_learnMode && g_learnModeClassic);
  bool rule2 = (((NETWORK_WIDE_SMART_START == bNetworkWideInclusion) ||
                 (NETWORK_WIDE_SMART_START_NWI == bNetworkWideInclusion)) &&
                  isSmartNode(HDRFORMATTYP_LR, pRxFrame));
  bool rule3 = (g_learnNodeState == LEARN_NODE_STATE_DELETE);
  bool rule4 =  ((g_learnNodeState == LEARN_NODE_STATE_NEW) && (assign_ID.assignIdState < ASSIGN_NOP_SEND));
  if (((pRxFrame->status & STATUS_NOT_MY_HOMEID) == 0) ||
      (
#ifdef ZW_CONTROLLER_BRIDGE
          !learnSlaveMode &&
#endif
          (rule1 || rule2 || rule3 || rule4))
     )
  {
    DPRINT("Status is 'Not my homeID'. \n");
    return false; // Keep frame
  }
#endif /* ZW_CONTROLLER */

  DPRINT("Drop frame! \n");
  return true;  // Drop frame
}

static bool Drop3chFrame(RX_FRAME *pRxFrame)
{
  frame *fr = (frame *)pRxFrame->pFrame;
  DPRINTF("%02X%02X%02X%02X", g_learnModeClassic, IsRouted(HDRFORMATTYP_3CH, fr), GetSinglecastPayload(HDRFORMATTYP_3CH, fr)[1], GetSinglecastPayload(HDRFORMATTYP_3CH, fr)[3]);
  /*protocol commands on multicast frames are not allowed*/
  if ((HDRTYP_MULTICAST == GetHeaderType(HDRFORMATTYP_3CH, fr)) &&
      IS_PROTOCOL_CLASS(*GetMulticastPayload(HDRFORMATTYP_3CH, fr))) {
    return true;
  }
  /* Check if we should process the frame at all */
#ifdef ZW_SLAVE
  if ((pRxFrame->status & STATUS_NOT_MY_HOMEID) == 0)
  {
    return false;
  }
  if ((g_nodeID && 
       ((exploreInclusionModeRepeat && 
        ((GetHeaderType(HDRFORMATTYP_3CH, fr) == HDRTYP_EXPLORE) ||
         (IsRouted(HDRFORMATTYP_3CH, fr) &&
         !memcmp(ZW_HomeIDGet(), &(GetRoutedPayload(HDRFORMATTYP_3CH, fr)[3]), HOMEID_LENGTH))))))
     )
  {
     return false;
  }
  if (g_learnMode &&
      (g_learnModeClassic || IsRouted(HDRFORMATTYP_3CH, fr) ||
       ((GetHeaderType(HDRFORMATTYP_3CH, fr) == HDRTYP_SINGLECAST) &&
        ((GetRoutedPayload(HDRFORMATTYP_3CH, fr)[1] != ZWAVE_CMD_ASSIGN_IDS) ||
          memcmp(crH, &(GetRoutedPayload(HDRFORMATTYP_3CH, fr)[3]), HOMEID_LENGTH))))
     )
  {
    return false;
  }
 
#endif /* ZW_SLAVE */
#ifdef ZW_CONTROLLER
  if (((pRxFrame->status & STATUS_NOT_MY_HOMEID) == 0) )
  {
    return false;
  }
#ifdef ZW_CONTROLLER_BRIDGE
  if (!learnSlaveMode)
  {
#endif
    bool rule1 = (g_learnModeClassic || IsRouted(HDRFORMATTYP_3CH, fr) ||
                  ((GetHeaderType(HDRFORMATTYP_3CH, fr) == HDRTYP_SINGLECAST) &&
                   ((GetRoutedPayload(HDRFORMATTYP_3CH, fr)[1] != ZWAVE_CMD_ASSIGN_IDS) ||
                    memcmp(crH, &(GetRoutedPayload(HDRFORMATTYP_3CH, fr)[3]), HOMEID_LENGTH))));
    bool rule2 = (((NETWORK_WIDE_SMART_START == bNetworkWideInclusion) ||
                   (NETWORK_WIDE_SMART_START_NWI == bNetworkWideInclusion)) &&
                  (isSmartNode(HDRFORMATTYP_3CH, pRxFrame)));
    bool rule3 = (g_learnNodeState == LEARN_NODE_STATE_DELETE);
    bool rule4 = ((g_learnNodeState == LEARN_NODE_STATE_NEW) && (assign_ID.assignIdState < ASSIGN_NOP_SEND));
    if ((g_learnMode && rule1) ||
         rule2 ||
         rule3 ||
         rule4
       )
    {
      return false;
    }
#ifdef ZW_CONTROLLER_BRIDGE
  }
#endif
  if (
#ifdef ZW_REPEATER
      exploreInclusionModeRepeat &&
#endif
      (GetHeaderType(HDRFORMATTYP_3CH, fr) == HDRTYP_EXPLORE))
  {
    return false;
  }

  if (IsRouted(HDRFORMATTYP_3CH, fr) &&
      !memcmp(ZW_HomeIDGet(), &GetRoutedPayload(HDRFORMATTYP_3CH, fr)[3], HOMEID_LENGTH))
  {
    return false;
  }
#endif /* ZW_CONTROLLER */
  return true;
}

static bool Drop2chFrame(RX_FRAME *pRxFrame)
{
  frame *fr = (frame *)pRxFrame->pFrame;
  /*protocol commands on multicast frames are not allowed*/  
  if ((HDRTYP_MULTICAST == GetHeaderType(HDRFORMATTYP_2CH, fr)) &&
      IS_PROTOCOL_CLASS(*GetMulticastPayload(HDRFORMATTYP_2CH, fr))) {
    return true;
  }  
#ifdef ZW_SLAVE
  if ((pRxFrame->status & STATUS_NOT_MY_HOMEID) == 0)
  {
    return false;
  }
  bool rule1 = ((GetHeaderType(HDRFORMATTYP_2CH, fr) == HDRTYP_EXPLORE) ||
                ((GetHeaderType(HDRFORMATTYP_2CH, fr) == HDRTYP_SINGLECAST) &&
                 IsRouted(HDRFORMATTYP_2CH, fr) &&
                 !memcmp(ZW_HomeIDGet(), &GetRoutedPayload(HDRFORMATTYP_2CH, fr)[3], HOMEID_LENGTH)));
  if (g_nodeID && (exploreInclusionModeRepeat && rule1))
  {
    return false;
  }
  rule1 = (g_learnModeClassic ||
           IsRouted(HDRFORMATTYP_2CH, fr) ||
           (GetSinglecastPayload(HDRFORMATTYP_2CH, fr)[1] != ZWAVE_CMD_ASSIGN_IDS) ||
           memcmp(crH, &GetSinglecastPayload(HDRFORMATTYP_2CH, fr)[3], HOMEID_LENGTH));
  if (g_learnMode && rule1)
  {
    return false;
  }
#endif /* ZW_SLAVE */
#ifdef ZW_CONTROLLER
  if ((pRxFrame->status & STATUS_NOT_MY_HOMEID) == 0)
  {
    return false;
  }
#ifdef ZW_CONTROLLER_BRIDGE
  if ((learnSlaveMode == VIRTUAL_SLAVE_LEARN_MODE_ENABLE) &&
      !g_learnMode && ((pRxFrame->status & STATUS_NOT_MY_HOMEID) == 0))
  {
    return false;
  }
#endif
  bool rule1 = (g_learnModeClassic ||
                IsRouted(HDRFORMATTYP_2CH, fr) ||
                (GetSinglecastPayload(HDRFORMATTYP_2CH, fr)[1] != ZWAVE_CMD_ASSIGN_IDS) ||
                memcmp(fr->header.homeID, &GetSinglecastPayload(HDRFORMATTYP_2CH, fr)[3], HOMEID_LENGTH));
  bool rule2 = ((NETWORK_WIDE_SMART_START == bNetworkWideInclusion) && (isSmartNode(HDRFORMATTYP_2CH, pRxFrame)));
  bool rule3 = (g_learnNodeState == LEARN_NODE_STATE_DELETE);
  bool rule4 = ((g_learnNodeState == LEARN_NODE_STATE_NEW) && (assign_ID.assignIdState < ASSIGN_NOP_SEND));
  if (
#ifdef ZW_CONTROLLER_BRIDGE
      !learnSlaveMode &&
#endif
      ((g_learnMode && rule1) || rule2 || rule3 || rule4) 
     )
  {
    return false;
  }
  if (
#ifdef ZW_REPEATER
      exploreInclusionModeRepeat &&
#endif
      (GetHeaderType(HDRFORMATTYP_2CH, fr) == HDRTYP_EXPLORE))
  {
    return false;
  }
  if (IsRouted(HDRFORMATTYP_2CH, fr) &&
      !memcmp(ZW_HomeIDGet(), &GetRoutedPayload(HDRFORMATTYP_2CH, fr)[3], HOMEID_LENGTH))
  {
    return false;
  }

#endif /* ZW_CONTROLLER */
  return true;
}


static bool isMyFrameDropFrame(ZW_HeaderFormatType_t curHeader, RX_FRAME *pRxFrame)
{
  if (HDRFORMATTYP_LR == curHeader)
  {
    return DropLrFrame(pRxFrame);
  }
  else if (HDRFORMATTYP_3CH == curHeader)
  {
    return Drop3chFrame(pRxFrame);
  }
  else if (HDRFORMATTYP_2CH == curHeader)
  {
    return Drop2chFrame(pRxFrame);
  }
  return true;
}

/*====================   RouteMatchesActive   ===========================
**    Check if the returning routed ACK/ERR frame matches the current
**    activetransmit frame
**
**--------------------------------------------------------------------------*/
bool              /*RET true if received routed frame matches current activetransmit frame */
RouteMatchesActive(
  ZW_HeaderFormatType_t curHeader,
  uint8_t repLen,    /* IN Number of hops in received routed frame */
  frame *fr)      /* IN ptr to received frame to check against current activetransmit frame */
{
  uint8_t i;
  if (HDRFORMATTYP_LR == curHeader)
  {
    /* Long Range doesn't do routing (yet) */
    return false;
  }
  else if (HDRFORMATTYP_3CH == curHeader)
  {
    /* Has the route the same length as expected and is the source of the received frame */
    /* the destination of the activetransmit frame */
    if ((repLen != GetRouteLen(curHeader, (frame *)&pFrameWaitingForACK->frame.header)) ||
        (fr->header.sourceID != pFrameWaitingForACK->frame.header.singlecast3ch.destinationID))
    {
      return false;
    }
    /* Is the repeaters the same */
    for (i = 0; i < repLen; i++)
    {
      if (*(pFrameWaitingForACK->frame.header.singlecastRouted3ch.repeaterList + i) != ReadRepeater(curHeader, fr, i))
      {
        return false;
      }
    }
  }
  else
  {
    /* Has the route the same length as expected and is the source of the received frame */
    /* the destination of the activetransmit frame */
    if ((repLen != GetRouteLen(curHeader,(frame *)&pFrameWaitingForACK->frame.header)) ||
        (fr->header.sourceID != pFrameWaitingForACK->frame.header.singlecast.destinationID))
    {
      return false;
    }
    /* Is the repeaters the same */
    for (i = 0; i < repLen; i++)
    {
      if (*(pFrameWaitingForACK->frame.header.singlecastRouted.repeaterList + i) != ReadRepeater(curHeader, fr, i))
      {
        return false;
      }
    }
  }
  return true;
}
/*============================   CheckSeqNo   ===============================
**    Check if the received frame matches the expected sequence number
**
**--------------------------------------------------------------------------*/
bool              /*RET True if sequnece number in received frame matches */
CheckReceivedSeqNo(ZW_HeaderFormatType_t curHeader, ZW_ReceiveFrame_t * pReceivedFrame, ZW_TransmissionFrame_t * pTransmittedFrame)
{
  if (pReceivedFrame == NULL || pTransmittedFrame == NULL)
  {
    return false;
  }

  if (HDRFORMATTYP_2CH == curHeader)
  {
    if (pReceivedFrame->profile != pTransmittedFrame->profile)
    {
      return false;
    }

    if (0 == pReceivedFrame->frameOptions.sequenceNumber)
    {
      return true;
    }

    if ((HDRTYP_TRANSFERACK == pReceivedFrame->frameOptions.frameType) &&
        (pReceivedFrame->frameOptions.sequenceNumber == pTransmittedFrame->frameOptions.sequenceNumber))
    {
      return true;
    }

    if (pReceivedFrame->frameOptions.routed != pTransmittedFrame->frameOptions.routed)
    {
      return false;
    }
  }

  // Valid for LR and 2CH
  if (pReceivedFrame->frameOptions.sequenceNumber != pTransmittedFrame->frameOptions.sequenceNumber)
  {
    return false;
  }

  return true;  // We like the sequence number
}

/*
  Check if the protocol commnds is allowed when destination node id is a broadcast
*/
static bool IsBroadCastIdAllowed (ZW_HeaderFormatType_t curHeader,frame *frame)
{
  uint8_t   cmdClass    = GetSinglecastPayload(curHeader, frame)[0];
  uint8_t   cmd         = GetSinglecastPayload(curHeader, frame)[1];
  node_id_t destID      = GetDestinationNodeIDSinglecast(curHeader, frame);
  uint8_t CmdsList [] = {ZWAVE_CMD_NODE_INFO,
                         ZWAVE_CMD_INCLUDED_NODE_INFO,
                         ZWAVE_CMD_SMARTSTART_PRIME_NODE_INFO,
                         ZWAVE_CMD_SMARTSTART_INCLUDE_NODE_INFO,
                         ZWAVE_CMD_EXCLUDE_REQUEST,
                         ZWAVE_CMD_TRANSFER_PRESENTATION,
                         ZWAVE_CMD_SET_NWI_MODE};
    
  if (HDRFORMATTYP_LR == curHeader) {
    /*if the destination id is not broadcast then its allowed*/
    if (NODE_BROADCAST_LR != destID) {
      return true;
    }
    /*if it is  not protocol command classthen its allowed*/    
    if ( !IS_PROTOCOL_CLASS(cmdClass) && !IS_PROTOCOL_LR_CLASS(cmdClass)){
      return true;
    }
    /*if the frame is a protocol command with broadcast destination id then check if the protocol command can be accepted with 
      broadcast destination id*/
    for (uint8_t i = 0; i < LR_PROTOCOL_CMDS_NUM; i++) {
      if (cmd == CmdsList[i]) {
        return true;
      }
    }
    return false;
  } else {
    if (NODE_BROADCAST != destID) {
      return true;
    } 
    if (!IS_PROTOCOL_CLASS(cmdClass)){
      return true;
    }
    for (uint8_t i = 0; i < sizeof (CmdsList); i++) {
      if (cmd == CmdsList[i]) {
        return true;
      }
    }
    return false;
  }
}

static void IsMyFrameHandleSingleCast(ZW_HeaderFormatType_t curHeader, RX_FRAME *pRxFrame)
{
  frame *fr = (frame *)pRxFrame->pFrame;
#ifdef ZW_CONTROLLER_BRIDGE
  node_id_t tmp;
#endif
  sACK = 0;

  uint8_t hopFirst2, hopFirst3;
  uint8_t hopFirst, hopFirst1, hopFirst4;
  uint8_t hopNext;

  if (HDRFORMATTYP_LR == curHeader)
  {
    /* Check frame size for singlecast */
    if (sizeof(frameHeaderSinglecastLR) + CRC16_LENGTH - sizeof(frameHeaderExtensionLR) > fr->singlecastLR.header.length)
    {
      pRxFrame->status = 0;
      return;
    }
    if (!IsBroadCastIdAllowed(curHeader, fr)) {
      pRxFrame->status = 0;
      return;
    }
    /* We should always know the contents of a single cast frame even if the frame is not for us. */
    pRxFrame->pPayload      = GetSinglecastPayload(curHeader, fr);
    pRxFrame->payloadLength = fr->header.length - 1 - (sizeof(frameHeaderSinglecastLR) - sizeof(frameHeaderExtensionLR)) + (GetExtendPresent(curHeader, fr) ? 1 + (fr->singlecastLR.extension.extensionInfo & MASK_EXTENSION_LENGTH_LR) : 0);

#ifdef ZW_CONTROLLER_BRIDGE
    tmp             = GetDestinationNodeIDSinglecast(curHeader, fr);
    virtualNodeDest = ((learnSlaveMode == VIRTUAL_SLAVE_LEARN_MODE_ENABLE) && !tmp);
    if ((virtualNodeDest && ((pRxFrame->status & STATUS_NOT_MY_HOMEID) == 0)) ||
        (tmp == g_nodeID) || (tmp == NODE_BROADCAST_LR) ||
        /* TO#2675 fix - ZW_IsVirtualNode now tests if g_nodeID is valid */
        ZW_IsVirtualNode(tmp))
    {
      /* was it our Controller NodeID, a Broadcast or one of our virtualNode IDs */
      virtualNodeID = ((tmp == g_nodeID) || (tmp == NODE_BROADCAST_LR)) ? 0 : tmp;
#else  /*ZW_CONTROLLER*/
    if (GET_SINGLECAST_DESTINATION_NODEID_LR(*fr) == g_nodeID || GET_SINGLECAST_DESTINATION_NODEID_LR(*fr) == NODE_BROADCAST_LR)
    {
#endif /*ZW_CONTROLLER*/
      /* The frame is for me */
      pRxFrame->status |= STATUS_FRAME_IS_MINE;
      pRxFrame->ackNodeID = GET_SINGLECAST_SOURCE_NODEID_LR(*fr); /* Set ack address */

      /* Check if we should send and ack */
      if (DoAck(curHeader, fr))
      {
        pRxFrame->status |= STATUS_DO_ACK;
      }
    }

  }
  else if (HDRFORMATTYP_3CH == curHeader)
  {
    /* Check frame size for singlecast */
    if (sizeof(frameHeaderSinglecast3ch) + CHECKSUM_LENGTH - sizeof(frameHeaderExtension3ch) > fr->singlecast3ch.header.length)
    {
      pRxFrame->status = 0;
      return;
    }
    if (!IsBroadCastIdAllowed(curHeader, fr)) {
      pRxFrame->status = 0;
      return;
    }
    /* We should always know the contents of a single cast frame even if the frame is not for us. */
    pRxFrame->pPayload      = GetSinglecastPayload(curHeader, fr);
    pRxFrame->payloadLength = fr->header.length - 1 - (sizeof(frameHeaderSinglecast3ch) - sizeof(frameHeaderExtension3ch)) + (GetExtendPresent(curHeader, fr) ? 1 + (fr->singlecast3ch.extension.extensionLength & 0x07) : 0);

    /* Check if the frame is for me */
#ifdef ZW_CONTROLLER_BRIDGE
    /* TO#2865 fix - We need to handle the virtual nodes also */
    tmp             = fr->singlecast3ch.destinationID;
    virtualNodeDest = ((learnSlaveMode == VIRTUAL_SLAVE_LEARN_MODE_ENABLE) && !tmp);
    if ((virtualNodeDest && ((pRxFrame->status & STATUS_NOT_MY_HOMEID) == 0)) ||
        (tmp == g_nodeID) || (tmp == NODE_BROADCAST) ||
        /* TO#2675 fix - ZW_IsVirtualNode now tests if g_nodeID is valid */
        ZW_IsVirtualNode(tmp))
    {
      /* was it our Controller NodeID, a Broadcast or one of our virtualNode IDs */
      virtualNodeID = ((tmp == g_nodeID) || (tmp == NODE_BROADCAST)) ? 0 : tmp;
#else
    if ((fr)->singlecast3ch.destinationID == g_nodeID || (fr)->singlecast3ch.destinationID == NODE_BROADCAST)
    {
#endif
      /* The frame is for me */
      pRxFrame->status |= STATUS_FRAME_IS_MINE;
      pRxFrame->ackNodeID = fr->header.sourceID; /* Set ack address */

      /* Check if we should send and ack */
      if (DoAck(curHeader, fr))
      {
        pRxFrame->status |= STATUS_DO_ACK;
      }
    }
  }
  else /*if 3ch*/
  {
    /* Check frame size for singlecast */
    if (sizeof(frameHeaderSinglecast) + CHECKSUM_LENGTH > fr->singlecast.header.length)
    {
      pRxFrame->status = 0;
      return;
    }
    if (IsRouted(curHeader, fr)) /* The frame is routed */
    {
      currentSeqNo |= MASK_CURRENTSEQNO_ROUTE;
      /* Check for valid repeater field */
      /* And frame length */
      hopFirst1 = GetRouteLen(curHeader, fr);
      if ((!hopFirst1 || (hopFirst1 > MAX_REPEATERS)) ||
          (sizeof(frameHeaderSinglecastRouted) - (MAX_REPEATERS + MAX_EXTEND_BODY) > fr->singlecast.header.length))
      {
#ifdef ZW_PROMISCUOUS_MODE
        /* if the routed frame is inncorrect dump it*/
        pRxFrame->status = STATUS_NOT_MY_HOMEID;
#else
        pRxFrame->status = 0;
#endif
        return;
      }

      /* TO#6124 / TO#6815 fix - Hack for 6_5x.xx nodes */
      /* 6.5x nodes will send a routed ack frame where the extended routing header bit is
             set but there is no extended header in the frame. We will try to detect the frame
             here and remove the extended header bit */
      if (IsRoutedAck(curHeader, fr))
      {
        if (GetExtendPresent(curHeader, fr))
        {
          /* Franme is routed ack with extended header*/
          /* Check if there is room for a extended header in the frame */
          if (fr->singlecast.header.length < (sizeof(frameHeaderSinglecastRouted) - MAX_REPEATERS -
                                              MAX_EXTEND_BODY + EXTEND_HEADER_MIN_SIZE + hopFirst1 +
                                              CHECKSUM_LENGTH))
          {
            /* Remove the extended header bit */
            ClrExtendPresent(curHeader, fr);
          }
        }
      }
      /* TODO SILENT ACK - we need to be certain that if the received frame is a routed ACK/ERR */
      /* then we need to be sure that the frame is an answer for frame we have in activetransmit */

      /* We should always know the payload length of the frame regardless if the frame is for */
      /* us or not (because we use it for repeating r-ack) )*/
      /* Routed frame. Always setup payload pointer. This works for all routed frame types on 2CH,
           * R-ACK, R-ERR and routed singlecast. On 3CH we must be more careful. */
      pRxFrame->pPayload      = GetRoutedPayload(curHeader, fr);
      pRxFrame->payloadLength = GetRoutedPayloadLen(curHeader, fr);
#ifdef ZW_CONTROLLER_BRIDGE
      tmp             = fr->singlecastRouted.destinationID;
      virtualNodeDest = ((learnSlaveMode == VIRTUAL_SLAVE_LEARN_MODE_ENABLE) && !tmp);
      if (virtualNodeDest || (tmp == g_nodeID) ||
          /* TO#2675 fix - ZW_IsVirtualNode now tests if nodeID is valid */
          ZW_IsVirtualNode(tmp))
      {
        /* was it our Controller NodeID or one of our virtualNode IDs */
        virtualNodeID = (tmp == g_nodeID) ? 0 : tmp;
#else
      if (fr->singlecastRouted.destinationID == g_nodeID)
      {
#endif /* ZW_CONTROLLER */
        /* The frame is for me, but I have to check that the frame */
        /* has been through all hops */
        hopFirst4 = GetHops(curHeader, fr);
#ifdef ZW_CONTROLLER
        if (hopFirst4 == GetRouteLen(curHeader, fr))
#else
        if (hopFirst4 == hopFirst1)
#endif /* ZW_CONTROLLER */
        {
          if (DoAck(curHeader, fr))
          {
            pRxFrame->status |= (STATUS_FRAME_IS_MINE | STATUS_DO_ACK |
                                 STATUS_DO_ROUTING_ACK);
          }
          else
          {
            pRxFrame->status |= (STATUS_FRAME_IS_MINE | STATUS_DO_ROUTING_ACK);
          }
          pRxFrame->ackNodeID = ReadRepeater(curHeader, fr, hopFirst4 - 1);
          if (((pRxFrame->status & STATUS_NOT_MY_HOMEID) == 0) && IsRouted(curHeader, fr) &&
              (GetRoutedPayload(curHeader, fr)[0] == ZWAVE_CMD_CLASS_PROTOCOL) &&
              (GetRoutedPayload(curHeader, fr)[1] == ZWAVE_CMD_ASSIGN_IDS))
          {
            if (bNetworkWideInclusion != NETWORK_WIDE_MODE_LEAVE)
            {
              /* GetSinglecastPayload(curHeader, fr)[2] contains the new nodeID */
              /* It is now determined that we are going to be included and */
              /* the routed ACK must be send on the new homeID */
              memcpy(inclusionHomeID, &GetRoutedPayload(curHeader, fr)[3], HOMEID_LENGTH);
            }
            else
            {
              /* If we are being Excluded do not answer on HomeID 00000000, but use old HomeID */
              memcpy(inclusionHomeID, crH, HOMEID_LENGTH);
            }
            inclusionHomeIDActive = true;
          }
        }
        else
        {
          /* TO#2775 - Code reworked so we always send a ack in the frame is for us */
          /* Check if routed frame is either a routed ack or a routed err frame */
          /* TO#1233 fix - We need to determine if the received routed ACK/ERR is an answer */
          /* for the current activetransmit frame */
          if ((GetHops(curHeader, fr) == 0xf))
          {
            if (DoAck(curHeader, fr))
            {
              pRxFrame->status |= (STATUS_DO_ACK);
            }

            if (IS_TXQ_POINTER(pFrameWaitingForACK) &&
                CheckReceivedSeqNo(curHeader, pRxFrame->pReceiveFrame, &(pFrameWaitingForACK->frame)) &&
                RouteMatchesActive(curHeader, GetRouteLen(curHeader, fr), fr))
            {
              pRxFrame->status |= (STATUS_FRAME_IS_MINE);
            }
            /* TO#5814 fix - We need to handle Routed ACK/ERR even if we are capable of going promiscuous */
            if (IsRoutedAck(curHeader, fr))
            {
              pRxFrame->status |= STATUS_ROUTE_ACK;
            }
            else if (IsRouteErr(curHeader, fr))
            {
              pRxFrame->status |= STATUS_ROUTE_ERR;
            }
            pRxFrame->ackNodeID = ReadRepeater(curHeader, fr, 0);
          }
        }
      }
      else //if (GetHops(curHeader, fr) < GetRouteLen(curHeader, fr))
      {
        /* The frame is not for me, but we need to check if we should */
        /* repeat the frame */
        /* Check if we are the repeater */
        hopFirst2 = GetHops(curHeader, fr);
        hopFirst3 = GetRouteLen(curHeader, fr);
        if ((hopFirst2 < hopFirst3) && (ReadRepeater(curHeader, fr, hopFirst2) == g_nodeID))
        {
          /* We are the repeater */
          if (!IsOutgoing(curHeader, fr)) /* Incoming frame */
          {
            /* TO#1952 fix start (replace contens of if statement with this) */
            /* First make sure it matches the frame we need */
            if (IS_TXQ_POINTER(pFrameWaitingForACK) &&
                CheckReceivedSeqNo(curHeader, pRxFrame->pReceiveFrame, &pFrameWaitingForACK->frame))
            {
              /* TODO - SILENT ACK */
              /* We got a silent ACK */
              pRxFrame->status |= (STATUS_TRANSFERACK | STATUS_FRAME_IS_MINE);
              sACK = (RECEIVE_SILENT_ACK | RECEIVE_DO_REPEAT);
            }
            if (hopFirst2 == (hopFirst3 - 1))
            {
              pRxFrame->ackNodeID = fr->header.sourceID;
            }
            else
            {
              pRxFrame->ackNodeID = ReadRepeater(curHeader, fr, hopFirst2 + 1);
            }
            /* Even if we are running silent ACK we need to ACK the previous node if we are to beam */
            /* the source node up, so that we get no problems with legacy */
            if (!hopFirst2 && (GetExtendPresent(curHeader, fr) &&
                               /* Is the Extend body of EXTEND_TYPE_WAKEUP_TYPE */
                               (GetExtendType(curHeader, fr) == EXTEND_TYPE_WAKEUP_TYPE) &&
                               /* Do source need a beam to wakeup */
                               (GetExtendBody(curHeader, fr, EXTEND_TYPE_WAKEUP_TYPE_SRC_DEST_OFFSET) & EXTEND_TYPE_WAKEUP_TYPE_SRC)))
            {
              pRxFrame->status |= STATUS_DO_ACK;
            }
          }
          else /* Outgoing frame */
          {
            /* If it is the first hop then ACK to source */
            if (hopFirst2 == 0)
            {
              pRxFrame->ackNodeID = fr->header.sourceID;
            }
            else /* Ack to previous repeater */
            {
              pRxFrame->ackNodeID = ReadRepeater(curHeader, fr, hopFirst2 - 1);
            }
            /* Even if we are running silent ACK we need to ACK the previous node if we are to beam */
            /* the destination node up, so that we get no problems with legacy */
            if ((hopFirst2 == hopFirst3 - 1) && (GetExtendPresent(curHeader, fr) &&
                                                 /* Is the Extend body of EXTEND_TYPE_WAKEUP_TYPE */
                                                 (GetExtendType(curHeader, fr) == EXTEND_TYPE_WAKEUP_TYPE) &&
                                                 /* Do destination need a beam to wakeup */
                                                 (GetExtendBody(curHeader, fr, EXTEND_TYPE_WAKEUP_TYPE_SRC_DEST_OFFSET) & EXTEND_TYPE_WAKEUP_TYPE_DEST)))
            {
              pRxFrame->status |= STATUS_DO_ACK;
            }
          }
          if (DoAck(curHeader, fr))
          {
            pRxFrame->status |= (STATUS_DO_ACK | STATUS_ROUTE_FRAME);
          }
          else
          {
            pRxFrame->status |= STATUS_ROUTE_FRAME;
          }
        }
        else
        {
          hopFirst = GetHops(curHeader, fr);
          /* We are not the repeater but the frame could be our silent ACK */
          if ( IS_TXQ_POINTER(pFrameWaitingForACK) &&
               CheckReceivedSeqNo(curHeader, pRxFrame->pReceiveFrame, &(pFrameWaitingForACK->frame)))
          {
            if (!IsOutgoing(curHeader, fr)) /* Incoming frame */
            {
              hopFirst = (hopFirst + 2) & 0x0f;
              /* If we are the previous repeater and the routed frame is incoming */
              if ((hopFirst < GetRouteLen(curHeader, fr)) &&
                  (ReadRepeater(curHeader, fr, hopFirst) == g_nodeID))
              {
                /* REPEATER INCOMING */
                /* We got a silent ACK */
                pRxFrame->status |= (STATUS_TRANSFERACK | STATUS_FRAME_IS_MINE);
                sACK = RECEIVE_SILENT_ACK;
              }
            }
            else
            {
              if (hopFirst >= 2)
              {
                /* If we are the previous repeater and the routed frame is outgoing */
                if (ReadRepeater(curHeader, fr, (hopFirst - 2)) == g_nodeID)
                {
                  /* REPEATER OUTGOING */
                  /* We got a silent ACK */
                  pRxFrame->status |= (STATUS_TRANSFERACK | STATUS_FRAME_IS_MINE);
                  sACK = RECEIVE_SILENT_ACK;
                }
              }
            }
          }
        }
      }
#ifdef ZW_CONTROLLER_BRIDGE
      if (IS_TXQ_POINTER(pFrameWaitingForACK) &&
          ((fr->header.sourceID == g_nodeID) || (ZW_IsVirtualNode(fr->header.sourceID))) &&
          CheckReceivedSeqNo(curHeader, pRxFrame->pReceiveFrame, &pFrameWaitingForACK->frame)
         )
#else
      if (IS_TXQ_POINTER(pFrameWaitingForACK) &&
          ((fr->header.sourceID == g_nodeID) ||
           (!memcmp(fr->header.homeID, pFrameWaitingForACK->frame.frameOptions.homeId, HOMEID_LENGTH) &&
            (fr->header.sourceID == pFrameWaitingForACK->frame.frameOptions.sourceNodeId))) &&
          CheckReceivedSeqNo(curHeader, pRxFrame->pReceiveFrame, &pFrameWaitingForACK->frame)
         )
#endif
      {
        /* We need to check if the frame is in the correct incarnation */
        /* We are the source of the routed payload/ACK/ERR frame */
        hopNext = GetHops(curHeader, fr);
        /* TO#1229 - fix, to make sure that the frame thought as a silent ACK frame also fits the frame, */
        /* which needs an ACK - A routed err frame could mistakenly be accepted as a silent ACK */
        /* Make sure that destination fits */
        if (fr->singlecast.destinationID == pFrameWaitingForACK->frame.frameOptions.destinationNodeId)
        {
          /* Now determine direction */
          if (!IsOutgoing(curHeader, fr))
          {
            /* Incoming */
            /* If this frame is the first incoming repeated frame */
            if (hopNext == ((GetRouteLen(curHeader, fr) - 2) & 0x0f))
            {
              /* DESTINATION ROUTED ACK/ERR SILENT ACK */
              /* We got a silent ACK */
              pRxFrame->status |= (STATUS_TRANSFERACK | STATUS_FRAME_IS_MINE);
              sACK = RECEIVE_SILENT_ACK | RECEIVE_DO_DELAY;
            }
          }
          else if (hopNext == 1)
          {
            /* Outgoing */
            /* SOURCE SILENT ACK - one repeater */
            /* This is the first repeated frame from the the first repeater */
            /* We got a silent ACK */
            pRxFrame->status |= (STATUS_TRANSFERACK | STATUS_FRAME_IS_MINE);
            sACK = RECEIVE_SILENT_ACK;
          }
        }
      }
    }
    else /* The frame is not routed */
    {
      if (!IsBroadCastIdAllowed(curHeader, fr)) {
        pRxFrame->status = 0;
       return;
      }
#ifdef ZW_PROMISCUOUS_MODE
      /* We should always know the contents of a single cast frame even if the frame is not for us. */
      pRxFrame->pPayload      = &fr->singlecast.startOfPayload;
      pRxFrame->payloadLength = fr->header.length - sizeof(frameSinglecast);
#endif
#ifdef ZW_CONTROLLER_BRIDGE
      tmp             = fr->singlecast.destinationID;
      virtualNodeDest = ((learnSlaveMode == VIRTUAL_SLAVE_LEARN_MODE_ENABLE) && !tmp);
      if ((virtualNodeDest && ((pRxFrame->status & STATUS_NOT_MY_HOMEID) == 0)) ||
          (tmp == g_nodeID) || (tmp == NODE_BROADCAST) ||
          /* TO#2675 fix - ZW_IsVirtualNode now tests if nodeID is valid */
          ZW_IsVirtualNode(tmp))
      {
        /* was it our Controller NodeID, a Broadcast or one of our virtualNode IDs */
        virtualNodeID = ((tmp == g_nodeID) || (tmp == NODE_BROADCAST)) ? 0 : tmp;
#else
      if ((fr->singlecast.destinationID == g_nodeID) ||
          (fr->singlecast.destinationID == NODE_BROADCAST))
      {
#endif
        pRxFrame->status |= STATUS_FRAME_IS_MINE;
        pRxFrame->ackNodeID = fr->header.sourceID; //
#ifndef ZW_PROMISCUOUS_MODE
        pRxFrame->pPayload      = &fr->singlecast.startOfPayload;
        pRxFrame->payloadLength = fr->header.length - sizeof(frameSinglecast);
#endif
        if (DoAck(curHeader, fr))
        {
          pRxFrame->status |= STATUS_DO_ACK;
        }
      }
    }
  }
}

static void IsMyFrameHandleMulticast(ZW_HeaderFormatType_t curHeader, RX_FRAME *pRxFrame)
{
  frame *fr = (frame *)pRxFrame->pFrame;
  uint8_t bBit;
  /*We should always know the content of the multicast frame even if the frame is not for us*/
  pRxFrame->pPayload      = GetMulticastPayload(curHeader, fr);
  pRxFrame->payloadLength = GetMulticastPayloadLen(curHeader, fr);

  /* Check if this nodes nodeID is in the address range of the
        * multicast frame */
  /* TODO - some BRIDGEWORK - need to handle check which, if any, nodes are in the multicast */
  /* frame and then the application (if application oriented), must receive ONE frame telling which */
  /* nodes was included in the multicast */
#ifdef ZW_CONTROLLER_BRIDGE
  bBit = GetMulticastOffset(curHeader, fr) + 1;
  do
  {
    /* Check if bBit is either nodeID or one of the virtual nodes */
    if ((bBit != g_nodeID)  && !CtrlStorageGetBridgeNodeFlag(bBit))
    {
      /* Remove bit, if present, as is not either nodeID or a virtual node */
      ZW_NodeMaskClearBit(GetReceiveMask(curHeader, fr), bBit - GetMulticastOffset(curHeader, fr));
    }
  } while ((++bBit <= (GetMulticastOffset(curHeader, fr) + (GetMulticastAddrLen(curHeader, fr) << 3))) && (bBit <= bMaxNodeID));
  /* Any bits left in multicast nodemask */
  if (ZW_NodeMaskBitsIn(GetReceiveMask(curHeader, fr), GetMulticastAddrLen(curHeader, fr)))
  {
    /* There was at least one match in the multicast address range */
    pRxFrame->status |= STATUS_FRAME_IS_MINE;
  }
#else
  if ((g_nodeID > GetMulticastOffset(curHeader, fr)) && (g_nodeID <= GetMulticastOffset(curHeader, fr) +
                                                              (GetMulticastAddrLen(curHeader, fr) << 3)))
  {
    /* It is in the address range, check if the addres bit is set */
    bBit = g_nodeID - GetMulticastOffset(curHeader, fr) - 1; /* Bit number in address */
    if (*(GetReceiveMask(curHeader, fr) + (bBit >> 3)) & ((uint8_t)0x01 << (0x07 & bBit)))
    {
      /* Set frame is mine status and set payload pointer and length */
      pRxFrame->status |= STATUS_FRAME_IS_MINE;
    }
  }
#endif
}

static void IsMyFrameHandleAck(ZW_HeaderFormatType_t curHeader, RX_FRAME *pRxFrame)
{
  frame *fr = (frame *)pRxFrame->pFrame;

  // Handle Beam-ACK
    if (((HDRFORMATTYP_3CH == curHeader) || (HDRFORMATTYP_LR == curHeader)) 
       && ((pRxFrame->status & STATUS_NOT_MY_HOMEID) == 0))
    {
      // Check if we are waiting for BEAM ACK - if so no more handling needed
      if (TxQueueBeamACKReceived(pRxFrame->pReceiveFrame->frameOptions.sourceNodeId, pRxFrame->pReceiveFrame->frameOptions.destinationNodeId))
      {
        return;
      }
    }

  // Handle ACK reception for non-beam ACK. (Regular transmission)
  if (HDRFORMATTYP_3CH == curHeader)
  {
    /*We should always tell if the frame is an ack even if the frame is not for us*/
    pRxFrame->status |= STATUS_TRANSFERACK;

    /* Check sequence number */
    if (currentSeqNo == currentSeqNoToMatch)
    {
#ifdef ZW_CONTROLLER_BRIDGE
      uint8_t tmp = ((frameTransferACK3ch *)fr)->destinationID;
      if ((!tmp && learnSlaveMode) || (tmp == g_nodeID) ||
          /* TO#2675 fix - ZW_IsVirtualNode now tests if nodeID is valid */
          ZW_IsVirtualNode(tmp))
      {
#else
      /* Is the frame for me */
      if (((frameTransferACK3ch *)fr)->destinationID == g_nodeID)
      {
#endif
        pRxFrame->status |= STATUS_FRAME_IS_MINE;
      }
    }
  }
  else  // LR and 2CH
  {
#ifdef ZW_PROMISCUOUS_MODE
    /*We should always tell if the frame is an ack even if the frame is not for us*/
    pRxFrame->status |= STATUS_TRANSFERACK;
#endif
    /* TO#2166 fix - Nodes which do not have repeat functionality must also mask the route bit, */
    /* when they are the destination. This is needed when src receives an ACK from repeater which */
    /* are to BEAM to destination. */
    /* ACKs are never routed therefor we need to make this assumption that if the transmitted frame */
    /* was a repeated frame the sequence number in the received ACK must only match with speed */
    /* and sequence number. Thankfully when the ACK is for a direct frame then the active frame */
    /* of course is not routed so we can just always mask out the route bit in currentSeqNoToMatch */
    if (IS_TXQ_POINTER(pFrameWaitingForACK) &&
        (NULL != pRxFrame) &&
        CheckReceivedSeqNo(curHeader, pRxFrame->pReceiveFrame, &pFrameWaitingForACK->frame))
    {
      node_id_t tmp = GetDestinationNodeIDSinglecast(curHeader, fr);
#ifdef ZW_CONTROLLER_BRIDGE
      if ((!tmp && learnSlaveMode) || (tmp == g_nodeID) ||
          /* TO#2675 fix - ZW_IsVirtualNode now tests if g_nodeID is valid */
          ZW_IsVirtualNode(tmp))
      {
#else
      if (tmp == g_nodeID)
      {
#endif
#ifdef ZW_PROMISCUOUS_MODE
        pRxFrame->status |= STATUS_FRAME_IS_MINE;
#else
        pRxFrame->status |= (STATUS_TRANSFERACK | STATUS_FRAME_IS_MINE);
#endif
      }
    }
  }
}

static void IsMyFrameHandleRouted(ZW_HeaderFormatType_t curHeader, RX_FRAME *pRxFrame)
{
  frame *fr = (frame *)pRxFrame->pFrame;
  uint8_t hopFirst2, hopFirst3;
  uint8_t hopFirst, hopFirst1, hopFirst4;
  sACK = 0;
  if ( HDRFORMATTYP_3CH == curHeader)
  {
    /* Check for valid repeater field and length */
    hopFirst1 = GetRouteLen(curHeader, fr);
    if (!hopFirst1 || (hopFirst1 > MAX_REPEATERS) ||
        (sizeof(frameHeaderSinglecastRouted3ch) - (MAX_REPEATERS - hopFirst1) - sizeof(frameHeaderExtension3ch) > fr->singlecast3ch.header.length))
    {
      pRxFrame->status = 0;
      return;
    }

    /* Get payload pointer and length */

    /* We should always know if the frame is routed ack even if the frame is not for us. */
    if (IsRoutedAck(curHeader, fr))
    {
      pRxFrame->status |= STATUS_ROUTE_ACK;
      pRxFrame->pPayload      = GetRoutedAckPayload(curHeader, fr);
      pRxFrame->payloadLength = GetRoutedAckPayloadLen(curHeader, fr);
    }
    else if (IsRouteErr(curHeader, fr))
    {
      pRxFrame->status |= STATUS_ROUTE_ERR;
      pRxFrame->pPayload      = GetRoutedAckPayload(curHeader, fr);
      pRxFrame->payloadLength = GetRoutedAckPayloadLen(curHeader, fr);
    }
    else
    {
      /* Not routed ack or err => this is an outgoing routed singlecast frame */
      pRxFrame->pPayload      = GetRoutedPayload(curHeader, fr);
      pRxFrame->payloadLength = GetRoutedPayloadLen(curHeader, fr);
    }

    /* Check if the frame is for me */
#ifdef ZW_CONTROLLER_BRIDGE
    uint8_t tmp     = fr->singlecastRouted3ch.destinationID;
    virtualNodeDest = ((learnSlaveMode == VIRTUAL_SLAVE_LEARN_MODE_ENABLE) && !tmp);
    if (virtualNodeDest || (tmp == g_nodeID) ||
        /* TO#2675 fix - ZW_IsVirtualNode now tests if nodeID is valid */
        ZW_IsVirtualNode(tmp))
    {
      /* was it our Controller NodeID or one of our virtualNode IDs */
      virtualNodeID = (tmp == g_nodeID) ? 0 : tmp;
#else
    if (fr->singlecastRouted3ch.destinationID == g_nodeID)
    {
#endif /* #ifdef ZW_CONTROLLER  */

      /* The frame is for me, but I have to check that the frame */
      /* has been through all hops */

      hopFirst4 = GetHops(curHeader, fr); /* Code size optimization */

      /* We know that hopFirst1 = GetRouteLen(curHeader, fr) from earlier */
      if (hopFirst1 == hopFirst4)
      {
        /* Frame has been through all repeaters (outgoing only) */
        if (DoAck(curHeader, fr))
        {
          pRxFrame->status |= (STATUS_FRAME_IS_MINE | STATUS_DO_ACK |
                               STATUS_DO_ROUTING_ACK);
        }
        else
        {
          pRxFrame->status |= (STATUS_FRAME_IS_MINE | STATUS_DO_ROUTING_ACK);
        }
        pRxFrame->ackNodeID = ReadRepeater(curHeader, fr, hopFirst4 - 1);
        if (((pRxFrame->status & STATUS_NOT_MY_HOMEID) == 0) && IsRouted(curHeader,fr) &&
            (GetRoutedPayload(curHeader, fr)[0] == ZWAVE_CMD_CLASS_PROTOCOL) &&
            (GetRoutedPayload(curHeader, fr)[1] == ZWAVE_CMD_ASSIGN_IDS))
        {
          if (bNetworkWideInclusion != NETWORK_WIDE_MODE_LEAVE)
          {
            /* GetSinglecastPayload(curHeader, fr)[2] contains the new nodeID */
            /* It is now determined that we are going to be included and */
            /* the routed ACK must be send on the new homeID */
            memcpy(inclusionHomeID, &GetRoutedPayload(curHeader, fr)[3], HOMEID_LENGTH);
          }
          else
          {
            /* If we are being Excluded do not answer on HomeID 00000000, but use old HomeID */
            memcpy(inclusionHomeID, crH, HOMEID_LENGTH);
          }
          inclusionHomeIDActive = true;
        }
      }
      else
      {
        /* Check if the frame is an incomming frame for me */
        if (GetHops(curHeader, fr) == 0xf)
        {
          /* TO#2775 - Code reworked so we always send a ack in the frame is for us */
          /* Incomming frame for me */
          if (DoAck(curHeader, fr))
          {
            pRxFrame->status |= (STATUS_DO_ACK);
          }
          /* TO#2903 fix - We must always ACK/ERR frames even if Received frame do not match active */
          if (IS_TXQ_POINTER(pFrameWaitingForACK) &&
              CheckReceivedSeqNo(curHeader, pRxFrame->pReceiveFrame, &(pFrameWaitingForACK->frame)) &&
              RouteMatchesActive(curHeader, hopFirst1, fr))
          {
            pRxFrame->status |= (STATUS_FRAME_IS_MINE);
          }

          pRxFrame->ackNodeID = ReadRepeater(curHeader, fr, 0);
        }
      }
    }
    else
    {
      /* The frame is not for me, but we need to check if we should */
      /* repeat the frame */
      /* Check if we are the repeater */
      hopFirst2 = GetHops(curHeader, fr);
      hopFirst3 = GetRouteLen(curHeader, fr);
      if ((hopFirst2 < hopFirst3) && (ReadRepeater(curHeader, fr, hopFirst2) == g_nodeID))
      {
        /* We are the repeater */
        if (!IsOutgoing(curHeader, fr)) /* Incoming frame */
        {
          /* Incomming frame, it must be an routed ack or routed err */
          /* First make sure it matches the frame we need */
          if (IS_TXQ_POINTER(pFrameWaitingForACK) &&
              CheckReceivedSeqNo(curHeader, pRxFrame->pReceiveFrame, &(pFrameWaitingForACK->frame)))
          {
            /* We got a silent ACK */
            pRxFrame->status |= (STATUS_TRANSFERACK | STATUS_FRAME_IS_MINE);
            sACK = (RECEIVE_SILENT_ACK | RECEIVE_DO_REPEAT);
          }
          if (hopFirst2 == (hopFirst3 - 1))
          {
            pRxFrame->ackNodeID = fr->header.sourceID;
          }
          else
          {
            pRxFrame->ackNodeID = ReadRepeater(curHeader, fr, hopFirst2 + 1);
          }
        }
        else
        { /* Outgoing frame */
          /* If it is the first hop then ACK to source */
          if (hopFirst2 == 0)
          {
            pRxFrame->ackNodeID = fr->header.sourceID;
          }
          else /* Ack to previous repeater */
          {
            pRxFrame->ackNodeID = ReadRepeater(curHeader, fr, hopFirst2 - 1);
          }
          /* Even if we are running silent ACK we need to ACK the previous node if we are to beam */
          /* the destination node up, so that we get no problems with legacy */
          /* Do destination need a beam to wakeup */
          if ((hopFirst2 == hopFirst3 - 1) &&
              (GET_ROUTING_DEST_WAKEUP_3CH(*fr)))
          {
            pRxFrame->status |= STATUS_DO_ACK;
          }
        }
        if (DoAck(curHeader, fr))
        {
          pRxFrame->status |= (STATUS_DO_ACK | STATUS_ROUTE_FRAME);
        }
        else
        {
          pRxFrame->status |= STATUS_ROUTE_FRAME;
        }
      }
      else
      {
        /* It is not for us and we are not the repeater but the frame
               could be our silent ack */
        hopFirst = GetHops(curHeader, fr);
        if (IS_TXQ_POINTER(pFrameWaitingForACK) &&
            CheckReceivedSeqNo(curHeader, pRxFrame->pReceiveFrame, &(pFrameWaitingForACK->frame)))
        {
          /* Sequence number is ok */
          if (!IsOutgoing(curHeader, fr)) /* Incoming frame */
          {
            hopFirst = (hopFirst + 2) & 0x0f;
            /* If we are the previous repeater and the routed frame is incoming */
            if ((hopFirst < GetRouteLen(curHeader, fr)) &&
                (ReadRepeater(curHeader, fr, hopFirst) == g_nodeID))
            {
              /* REPEATER INCOMING */
              /* We got a silent ACK */
              pRxFrame->status |= (STATUS_TRANSFERACK | STATUS_FRAME_IS_MINE);
              sACK = RECEIVE_SILENT_ACK;
            }
          }
          else
          {
            if (hopFirst >= 2)
            {
              /* If we are the previous repeater and the routed frame is outgoing */
              if (ReadRepeater(curHeader, fr, (hopFirst - 2)) == g_nodeID)
              {
                /* REPEATER OUTGOING */
                /* We got a silent ACK */
                pRxFrame->status |= (STATUS_TRANSFERACK | STATUS_FRAME_IS_MINE);
                sACK = RECEIVE_SILENT_ACK;
              }
            }
          }
        }
      }
    }
    /* Check if the frame is our repeated routed ack */
#ifdef ZW_CONTROLLER_BRIDGE
    if (IS_TXQ_POINTER(pFrameWaitingForACK) &&
        ((fr->header.sourceID == g_nodeID) || (ZW_IsVirtualNode(fr->header.sourceID))) &&
        (fr->singlecast3ch.destinationID == pFrameWaitingForACK->frame.frameOptions.destinationNodeId)
       )
#else
    if (IS_TXQ_POINTER(pFrameWaitingForACK) &&
        ((fr->header.sourceID == g_nodeID) ||
         (!memcmp(fr->header.homeID, pFrameWaitingForACK->frame.frameOptions.homeId, HOMEID_LENGTH) &&
          (fr->header.sourceID == pFrameWaitingForACK->frame.frameOptions.sourceNodeId))) &&
        (fr->singlecast3ch.destinationID == pFrameWaitingForACK->frame.frameOptions.destinationNodeId)
       )
#endif
    {
      hopFirst = GetHops(curHeader, fr);
      /* Now determine direction */
      if (!IsOutgoing(curHeader, fr))
      {
        /* Incoming */
        /* If this frame is the first incoming repeated frame */
        if (hopFirst == ((GetRouteLen(curHeader, fr) - 2) & 0x0f))
        {
          /* DESTINATION ROUTED ACK/ERR SILENT ACK */
          /* We got a silent ACK */
          pRxFrame->status |= (STATUS_TRANSFERACK | STATUS_FRAME_IS_MINE);
          sACK = RECEIVE_SILENT_ACK | RECEIVE_DO_DELAY;
        }
      }
      else if (hopFirst == 1)
      {
        /* Outgoing */
        /* SOURCE SILENT ACK - one repeater */
        /* This is the first repeated frame from the the first repeater */
        /* We got a silent ACK */
        pRxFrame->status |= (STATUS_TRANSFERACK | STATUS_FRAME_IS_MINE);
        sACK = RECEIVE_SILENT_ACK;
      }
    }
  }
}


/**********************************************************************************************************
 *                                      PACKET DROP CONDITIONS
 *********************************************************************************************************/

/**********************************************************************************************************
 * Conditions for behavior when in learn mode near foreign networks.
 * Make sure that controller is not going to acknowledging the "Node Info" or "Node Info Request" packets
 * originating from a different networks, when it is in learn mode classic (inclusion and exclusion mode).
 * These transmissions will be single casts and with an already included source node (sourceID != 0).
 *********************************************************************************************************/

/**
 * This condition return true for packets that must be dropped!!
 * This condition is valid only for when the controller is in inclusion learn mode.
 *
 * With this condition, NODE INFO REQUESTs, NODE INFOs and TRANSFER NODE INFOs (etc) are only accepted,
 * if they are not originated from a foreign NWK or have their nodeID set to zero. (trying to get included!)
 */
#define CONTROLLER_PACKET_DROP_CONDITION_FOR_FOREIGN_NWK_PACKET_DROPPING_DURING_INCLUSION_MODE                                          \
  ( /* --> Conditions for behavior when in learn mode near foreign networks. */                                                         \
    (                                                                                                                                   \
      (btmplearnNodeState == LEARN_NODE_STATE_NEW)   /* This condition is valid while controller is in include mode. */                 \
    ) && (                                                                                                                              \
           (  /* The source node is included into foreign network or it is a legacy controller responding to REQUEST_NODE_INFO! */     \
              (btmpStatus & STATUS_NOT_MY_HOMEID)                                                                                      \
               && (                                                                                                                     \
                     (GetSourceNodeID(curHeader, fr) != 0) && /* Node is already included. */                                              \
                     (GetSourceNodeID(curHeader, fr) != NODE_CONTROLLER_OLD)  /* LEGACY ControllerID */                              \
                  )                                                                                                                     \
           ) && (    /* Commands in interest for this condition. (Protocol commands during learn mode) */                               \
                     (btmpCmd == ZWAVE_CMD_REQUEST_NODE_INFO)     /* Node Info Request in a in foreign NWK. */                          \
                  || (btmpCmd == ZWAVE_LR_CMD_REQUEST_NODE_INFO)  /* - same as above, just for LR. */                                   \
                  || (btmpCmd == ZWAVE_CMD_NODE_INFO)             /* Node Info response to controller request send in the foreign NWK.*/\
                  || (btmpCmd == ZWAVE_LR_CMD_NODE_INFO)          /* - same as above, just for LR. */                                   \
                  || (btmpCmd == ZWAVE_CMD_TRANSFER_NODE_INFO)    /* Controller to controller communication in foreign NWK. */          \
                  || (btmpCmd == ZWAVE_CMD_ASSIGN_IDS)            /* Avoid having the virtual node responding with ACK to this. */      \
                  || (btmpCmd == ZWAVE_LR_CMD_ASSIGN_IDS)         /* - same as above, just for LR. */                                   \
                )                                                                                                                       \
         )                                                                                                                              \
  )

/**
 * This condition return true for packets that must be dropped!!
 * This condition is valid only for when the controller is in exclusion learn mode.
 *
 * With this condition, NODE INFO REQUESTs, NODE INFOs and TRANSFER NODE INFOs are only accepted,
 * if they are originated from an excluded node that is broadcasting these frames.
 */
#define CONTROLLER_PACKET_DROP_CONDITION_FOR_FOREIGN_NWK_PACKET_DROPPING_DURING_EXCLUSION_MODE                                          \
  (                                                                                                                                     \
    (                                                                                                                                   \
      (btmplearnNodeState == LEARN_NODE_STATE_DELETE)   /* This condition is valid while controller is in exclude mode. */              \
    ) && (                                                                                                                              \
           (       /* Broadcast is always used for inclusion and exclusion requests. Those are accepted packets in this condition. */   \
             !( (destNodeID == NODE_BROADCAST) || (destNodeID == NODE_BROADCAST_LR) || (destNodeID == g_nodeID) )                       \
           ) && (  /* Commands in interest for this condition. (Protocol commands during learn mode) */                                 \
                     (btmpCmd == ZWAVE_CMD_REQUEST_NODE_INFO)     /* Node Info Request in a in foreign NWK. */                          \
                  || (btmpCmd == ZWAVE_LR_CMD_REQUEST_NODE_INFO)  /* same as above, just for LR. */                                     \
                  || (btmpCmd == ZWAVE_CMD_NODE_INFO)             /* Node Info response to controller request send in the foreign NWK.*/\
                  || (btmpCmd == ZWAVE_LR_CMD_NODE_INFO)          /* same as above, just for LR. */                                     \
                  || (btmpCmd == ZWAVE_CMD_TRANSFER_NODE_INFO)    /* Controller to controller communication in foreign NWK. */          \
                )                                                                                                                       \
         )                                                                                                                              \
  )

/**
 * This condition return true for packets that must be dropped!!
 * This condition is valid only for when the controller is in exclusion learn mode.
 *
 * With this condition, NODE INFO REQUESTs are only accepted if they originate from own NWK.
 */
#define CONTROLLER_PACKET_DROP_CONDITION_FOR_FOREIGN_REQ_NODE_INFO_PACKET_DROPPING_DURING_EXCLUSION_MODE                                \
  (                                                                                                                                     \
    ( /* TO#2926 possible fix */                                                                                                        \
      (btmplearnNodeState == LEARN_NODE_STATE_DELETE)   /* This condition is valid while controller is in exclude mode. */              \
    ) && (                                                                                                                              \
            (btmpStatus & STATUS_NOT_MY_HOMEID)         /* The source node is included into foreign network! */                         \
            && (  /* Block of commands in interest for this condition. (Protocol commands during learn mode) */                         \
                     (btmpCmd == ZWAVE_CMD_REQUEST_NODE_INFO)     /* Node Info Request in a in foreign NWK. */                          \
                  || (btmpCmd == ZWAVE_LR_CMD_REQUEST_NODE_INFO)  /* same as above, just for LR. */                                     \
               )                                                                                                                        \
         )                                                                                                                              \
  )


static bool FrameNotAllowed(ZW_HeaderFormatType_t curHeader, RX_FRAME *pRxFrame, uint8_t bCmdClass, uint8_t bCmd)
{
  frame *fr = (frame *)pRxFrame->pFrame;
#ifdef ZW_CONTROLLER  
  uint8_t bStatus = pRxFrame->status; 
#endif  
  if ((HDRFORMATTYP_3CH == curHeader) || (HDRFORMATTYP_LR == curHeader))
  {
#ifdef ZW_CONTROLLER
      if (((bCmdClass <= ZWAVE_CMD_CLASS_PROTOCOL) || (bCmdClass == ZWAVE_CMD_CLASS_PROTOCOL_LR)) &&
          (!g_learnMode ||
           ((bCmd == ZWAVE_CMD_TRANSFER_PRESENTATION) ||
            (((bCmd == ZWAVE_CMD_TRANSFER_NODE_INFO) ||
              (bCmd == ZWAVE_CMD_FIND_NODES_IN_RANGE)) &&
             !(bStatus & STATUS_NOT_MY_HOMEID)) ||
            ((bCmd == ZWAVE_CMD_ASSIGN_IDS) &&
             (!(bStatus & STATUS_NOT_MY_HOMEID) ||
              ((g_learnModeClassic && !memcmp(crH, &GetSinglecastPayload(curHeader, fr)[3], HOMEID_LENGTH)) ||
               ((GetSinglecastPayload(curHeader, fr)[3] | GetSinglecastPayload(curHeader, fr)[4] |
                 GetSinglecastPayload(curHeader, fr)[5] | GetSinglecastPayload(curHeader, fr)[6]) == 0))))
#ifdef ZW_CONTROLLER_TEST_LIB
            || ((bCmd == ZWAVE_LR_CMD_EXCLUDE_REQUEST_CONFIRM) && !memcmp(crH, &GetSinglecastPayload(curHeader, fr)[4], HOMEID_LENGTH))
#endif
                )))
#endif /*#ifdef ZW_CONTRIOLLER*/
#ifdef ZW_SLAVE
      if (/* Only the protocol command */
           (IS_PROTOCOL_CLASS(bCmdClass) || IS_PROTOCOL_LR_CLASS(bCmdClass)) &&
           (
            /* Transfer Presentation */
            (bCmd == ZWAVE_CMD_TRANSFER_PRESENTATION) ||
            (
              /* and the ASSIGN_ID command should be allowed... */
             (bCmd == ZWAVE_CMD_ASSIGN_IDS) &&
             ((g_learnModeClassic && !memcmp(crH, &GetSinglecastPayload(curHeader, fr)[3], HOMEID_LENGTH)) ||
              ((GetSinglecastPayload(curHeader, fr)[3] | GetSinglecastPayload(curHeader, fr)[4] |
                GetSinglecastPayload(curHeader, fr)[5] | GetSinglecastPayload(curHeader, fr)[6]) == 0))) ||
               ((bCmd == ZWAVE_LR_CMD_EXCLUDE_REQUEST_CONFIRM) && !memcmp(crH, &GetSinglecastPayload(curHeader, fr)[4], HOMEID_LENGTH))))
#endif /*#ifdef ZW_SLAVE*/
      {
        return false;
      }
      return true;
  }
  else if (HDRFORMATTYP_2CH == curHeader)
  {
#ifdef ZW_CONTROLLER
      if ((bCmdClass <= ZWAVE_CMD_CLASS_PROTOCOL) &&
          (!g_learnMode ||
           ((bCmd == ZWAVE_CMD_TRANSFER_PRESENTATION) ||
            /* TO#2054 fix */
            (((bCmd == ZWAVE_CMD_TRANSFER_NODE_INFO) || (bCmd == ZWAVE_CMD_FIND_NODES_IN_RANGE)) && !(bStatus & STATUS_NOT_MY_HOMEID)) ||
            ((bCmd == ZWAVE_CMD_ASSIGN_IDS) &&
             (!(bStatus & STATUS_NOT_MY_HOMEID) ||
              ((g_learnModeClassic && !memcmp(crH, &GetSinglecastPayload(curHeader, fr)[3], HOMEID_LENGTH)) ||
               ((GetSinglecastPayload(curHeader, fr)[3] | GetSinglecastPayload(curHeader, fr)[4] |
                 GetSinglecastPayload(curHeader, fr)[5] | GetSinglecastPayload(curHeader, fr)[6]) == 0)))))))
#endif /*#ifdef ZW_CONTRIOLLER*/
#ifdef ZW_SLAVE
        if (IS_PROTOCOL_CLASS(bCmdClass) &&               /* only the protocol command ASSIGN_ID */
            ((bCmd == ZWAVE_CMD_TRANSFER_PRESENTATION) || /* and the Transfer Presentation command */
             (((g_learnModeClassic && !memcmp(crH, &GetSinglecastPayload(curHeader, fr)[3], HOMEID_LENGTH)) ||
               ((GetSinglecastPayload(curHeader, fr)[3] | GetSinglecastPayload(curHeader, fr)[4] | GetSinglecastPayload(curHeader, fr)[5] | GetSinglecastPayload(curHeader, fr)[6]) == 0)) &&
              (bCmd == ZWAVE_CMD_ASSIGN_IDS)))) /* should be allowed... */
#endif                                             /*#ifdef ZW_SLAVE*/
      {
        return false;
      }
      return true;
  }
  return false;
}

/*In special cases even if singlecast is labeled as our frame we negate that*/
static void IsMyFrameHandleException(ZW_HeaderFormatType_t curHeader, RX_FRAME *pRxFrame)
{
  uint8_t btmpStatus;
  uint8_t btmpCmdClass;
  uint8_t btmpCmd;
#ifdef ZW_CONTROLLER
  uint8_t btmplearnNodeState;
  node_id_t destNodeID;
#endif

  frame *fr = pRxFrame->pFrame;
  btmpStatus = pRxFrame->status;
  if (GetHeaderType(curHeader, fr) == HDRTYP_SINGLECAST) 
  {
    if (IsRouted(curHeader, fr))
    {
      btmpCmdClass = GetRoutedPayload(curHeader, fr)[0];
      btmpCmd = GetRoutedPayload(curHeader, fr)[1];
    }
    else
    {
      btmpCmdClass = GetSinglecastPayload(curHeader, fr)[0];
      btmpCmd = GetSinglecastPayload(curHeader, fr)[1];
    }
#ifdef ZW_CONTROLLER
    btmplearnNodeState = g_learnNodeState;
    destNodeID = GetDestinationNodeIDSinglecast(curHeader, fr);
#endif
    if ((HDRFORMATTYP_3CH == curHeader) || (HDRFORMATTYP_LR == curHeader) || (HDRFORMATTYP_2CH == curHeader))
    {
#ifdef ZW_CONTROLLER
      if (
          /* only the TransferPresentation , the AssignId protocol and the NOP commands frames accepted in learn mode */
          ((g_learnMode ||
            /* TO#2926 possible fix */
            (btmplearnNodeState == LEARN_NODE_STATE_DELETE) ||
            (btmplearnNodeState == LEARN_NODE_STATE_NEW)) &&
           /* TO#1198 fix - ROUTED ACK and ROUTED ERR should always be allowed through */
           ((btmpStatus & (STATUS_FRAME_IS_MINE | STATUS_TRANSFERACK | STATUS_ROUTE_ACK | STATUS_ROUTE_ERR)) == STATUS_FRAME_IS_MINE) &&
           FrameNotAllowed(curHeader, pRxFrame, btmpCmdClass, btmpCmd)) ||
          ( // NEW ROOT CONDITION (the above conditions are not a limiting factor)
            CONTROLLER_PACKET_DROP_CONDITION_FOR_FOREIGN_NWK_PACKET_DROPPING_DURING_INCLUSION_MODE ||
            CONTROLLER_PACKET_DROP_CONDITION_FOR_FOREIGN_NWK_PACKET_DROPPING_DURING_EXCLUSION_MODE ||
            CONTROLLER_PACKET_DROP_CONDITION_FOR_FOREIGN_REQ_NODE_INFO_PACKET_DROPPING_DURING_EXCLUSION_MODE))
#endif /* ZW_CONTROLLER */

#ifdef ZW_SLAVE
        /* in LEARNMODE, */
        if (
            (g_learnMode &&
             /* and NOT_MY_HOMEID, */
             ((btmpStatus & (STATUS_NOT_MY_HOMEID | STATUS_TRANSFERACK | STATUS_ROUTE_ACK | STATUS_ROUTE_ERR)) == STATUS_NOT_MY_HOMEID) &&
             FrameNotAllowed(curHeader, pRxFrame, btmpCmdClass, btmpCmd)))
#endif /* ZW_SLAVE */
        /* The if-statement body of above condition */
        {
#ifdef ZW_REPEATER
          pRxFrame->status &= ~(STATUS_FRAME_IS_MINE | STATUS_ROUTE_FRAME | STATUS_DO_ACK); // Clear flags
#else
          pRxFrame->status &= ~(STATUS_FRAME_IS_MINE | STATUS_DO_ACK);
#endif
          btmpStatus =  pRxFrame->status;
        }
    }
    else
    {
      ASSERT(0); // Unknown curHeader!
    }
  }



  /*****************************************************
   * Publish packet evaluation result.
   ****************************************************/

  if (btmpStatus & STATUS_FRAME_IS_MINE)
  {
    myFrame = true;
  }
}


bool IsMyFrame(
    ZW_HeaderFormatType_t curHeader,
    RX_FRAME *pRxFrame,
    uint8_t * pReceiveStatus)
{
  mpReceiveStatus = pReceiveStatus;
  frame *fr = (frame *)pRxFrame->pFrame;
  pRxFrame->status = 0;
  myFrame          = false;

  /* Store the HomeID that was just received in the received frame. */
  memcpy(crH, fr->header.homeID, HOMEID_LENGTH);

  if (memcmp(crH, ZW_HomeIDGet(), HOMEID_LENGTH) != 0)
  {
    DPRINT("Received homeId does not match our homeID! \n");
    pRxFrame->status |= STATUS_NOT_MY_HOMEID;
  }

  // Check wheather the packet should be dropped
  if (!isMyFrameDropFrame(curHeader, pRxFrame))
  {
    if (HDRFORMATTYP_3CH == curHeader || HDRFORMATTYP_LR == curHeader)
    {
      /* Get sequence numbers */
      currentSeqNo = GetSeqNumber(curHeader, fr);
      /* TO#5772 fix */
      currentSeqNoToMatch = IS_TXQ_POINTER(pFrameWaitingForACK) ? pFrameWaitingForACK->frame.frameOptions.sequenceNumber : 0xFF;
    }
    else
    {
      currentSeqNo = GetSeqNumber(curHeader, fr) | (IS_CURRENT_SPEED_40K ? MASK_CURRENTSEQNO_SPEED : 0);
      /* We use the speed and route bits to differentiate the different sequence numbers */
      /* TO-DO: Verify that pFrameWaitingForAck points to "the right" element (not NULL, perhaps?) */
      /* "the right" is an element we are waiting for an ACK on */
      /* TO#5772 fix */
      if (!IS_TXQ_POINTER(pFrameWaitingForACK))
      {
        currentSeqNoToMatch = 0xFF;
      }
    }
    DPRINTF("seq nr: %d, currentSeqNoToMatch: %d \n", currentSeqNo, currentSeqNoToMatch);

    /* this network */
    /* TODO SILENT ACK - we probably need to do some more test to make sure that the frames */
    /* accepted as silent ACK frames also fits the frames they should be silent ACKs for */
    switch (GetHeaderType(curHeader, fr))
    {
      case HDRTYP_SINGLECAST:
      {
        IsMyFrameHandleSingleCast(curHeader, pRxFrame);
        break;
      }

      case HDRTYP_MULTICAST:
      {
        IsMyFrameHandleMulticast(curHeader, pRxFrame);
        break;
      }

      case HDRTYP_TRANSFERACK:
      {
        IsMyFrameHandleAck(curHeader, pRxFrame);
        break;
      }

      case HDRTYP_ROUTED:
      {
        IsMyFrameHandleRouted(curHeader, pRxFrame);
        break;
      }
    }
  }
  IsMyFrameHandleException(curHeader, pRxFrame);
  return myFrame;
}
