// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file ZW_frames_filters.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include "ZW_lib_defines.h"

#include <ZW_typedefs.h>
#include <NodeMask.h>

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <string.h>
#ifdef ZW_SLAVE
#include <ZW_basis_api.h>
#include <ZW_slave.h>
#endif  /*ZW_SLAVE*/
#ifdef ZW_CONTROLLER
#include <ZW_controller.h>
#include <ZW_replication.h>
#endif /*ZW_CONTROLLER*/



#include <ZW_protocol.h>
#include <ZW_transport.h>
#include <ZW_MAC.h>

//#define DEBUGPRINT
#include "DebugPrint.h"
#ifdef ZW_CONTROLLER
#include <ZW_routing_cache.h>
#endif
#include <ZW_explore.h>

#if defined(ZW_SLAVE_ROUTING) || defined(ZW_CONTROLLER)
#include <ZW_transport_api.h>
#endif

/*
  This function test the received frame for some rules
  And if the rules are not passd we drop it.
*/

#define MY_NODEID    0x01
#define DROP_FRAME    0x02
uint8_t IsFrameIlLegal (ZW_HeaderFormatType_t curHeader, frame* pRxFrame, uint8_t* pCmdClass, __attribute__((unused)) uint8_t *pCmd)
{
     uint8_t frameState = false;
#ifdef ZW_CONTROLLER
     DPRINTF("L:%02x%02x%02x%02x\r\n", *pCmdClass, *pCmd, g_learnMode, g_learnNodeState);
#else
     DPRINTF("L:%02x%02x%02x\r\n", *pCmdClass, *pCmd, g_learnMode);
#endif
      /*
        Detect if the node's nodeID is in the sourceID field when the frame is not routed or the nodeID in both sourceID and destinationID field when the frame is routed
      */
     uint8_t isRouted = IsRouted(curHeader, pRxFrame);
     uint8_t frameType = GetHeaderType(curHeader, pRxFrame);
    if (HDRFORMATTYP_3CH == curHeader)
    {
      if ((GetSourceNodeID(curHeader, pRxFrame) == g_nodeID) && (!isRouted || (GetDestinationNodeIDSinglecast(curHeader, pRxFrame) == g_nodeID)) )
      {
        frameState |= MY_NODEID;
      }
    }
    else
    {
      if (
           (GetSourceNodeID(curHeader, pRxFrame) == g_nodeID) &&
           ((HDRTYP_SINGLECAST != frameType) || !isRouted ||
            (isRouted && (GetDestinationNodeIDSinglecast(curHeader, pRxFrame) == g_nodeID)))
         )
      {
        frameState |= MY_NODEID;
      }
    }
    /*
      MIT-12 Reject own source address in incoming S0 and S2 frames via deep packet inspection in protocol (to postpone releasing GW with MIT-1)
      MIT-13 Reject dest=0xFF in S0 in incoming frames  via deep packet inspection in protocol
    */
    if ((NULL != pCmdClass) && ((COMMAND_CLASS_SECURITY_2 == *pCmdClass) || (COMMAND_CLASS_SECURITY == *pCmdClass)))
    {
      if (frameState & MY_NODEID)
      {
        return true;
      }
      else if (COMMAND_CLASS_SECURITY == *pCmdClass )
      {
        node_id_t destID;
        destID = GetDestinationNodeIDSinglecast(curHeader, pRxFrame);
        if (0xFF == destID)  // Need to check classic 8-bit size node ID only (S0 security is not supported on Z-WaveLR)
        {
          return true;
        }
      }
    }
#ifdef ZW_SLAVE
      /*MIT-14: Slave: Reject own source address in all frames. Except when in learn mode*/
    if ( (!g_learnMode) && (frameState & MY_NODEID) )
    {
      return true;
    }
#endif
#ifdef ZW_CONTROLLER
#ifndef ZW_CONTROLLER_TEST_LIB
    /*In LR test controller we ignore the is test since no node ingormtion is available in the node info tabel*/
    if (
        /*
          We should drop incomming frame if it not virtual node (in bridge libary only) and it is unknown node or sourceID filed is the node's nodeID
          A node considered unknown if not in the node table and it is not and ACK frame
        */
#ifdef ZW_CONTROLLER_BRIDGE
         !ZW_IsVirtualNode(GetSourceNodeID(curHeader, pRxFrame)) &&
#endif
         (
           (frameState & MY_NODEID)  ||
           (!ZCB_GetNodeType(GetSourceNodeID(curHeader, pRxFrame)) && !IsTransferAck(curHeader, pRxFrame) && !IsRouted(curHeader, pRxFrame))
         )
       )
    {
      frameState |= DROP_FRAME;
    }
#endif /*#ifndef ZW_CONTROLLER_TEST_LIB*/
    /*
      MIT-10: SIS: Allow only frames from known nodes in the network.
      Frames from the SIS itself must also be ignored.
      Except during Add Node Mode and Remove Node Mode.
      When in Add Node Smart Start mode only inclusion requests MUST be allowed from unknown nodes.
      MIT 11: Inclusion controller does the same. Except for Smart Start Add Node Mode, which inclusion controllers MUST not use.
    */
       /*
         drop all frames from unknown nodes expect when the frame is an ack or a nop/nop power frame
       */
    if (g_learnMode || NEW_CTRL_RECEIVER_BIT)
    {
      frameState &= ~DROP_FRAME;
    }
    else if (g_learnNodeState)
    {
#ifdef ZW_CONTROLLER
      if (LEARN_NODE_STATE_SMART_START == g_learnNodeState)
      {
        if ( HDRTYP_EXPLORE == frameType)
        {
          uint8_t exploreCmdType;
          uint8_t exploreCmd;
          if (HDRFORMATTYP_3CH == curHeader)
          {
            exploreCmdType = pRxFrame->explore3ch.ver_Cmd & EXPLORE_CMD_MASK;
          }
          else
          {
            exploreCmdType = pRxFrame->explore.ver_Cmd & EXPLORE_CMD_MASK;
          }
          exploreCmd = ((EXPLORE_REMOTE_INCLUSION_REQUEST_FRAME*)GetExplorePayload(curHeader, pRxFrame))->header.cmd;
          if (EXPLORE_CMD_AUTOINCLUSION == exploreCmdType)
          {
           if (ZW_IS_SIS_CONTROLLER)
           {
             if  (ZWAVE_CMD_EXCLUDE_REQUEST == exploreCmd)
             {
               frameState |= DROP_FRAME;
             }
             else
             {
               frameState &=~DROP_FRAME;
             }
           }
           else
           {
             /*If we are in smartstart mode and it is autoinclusion frame then overside filter*/
             frameState &=~DROP_FRAME;
           }
         }
        }
        /*
         * Do not drop the frame if:
         * 1. It's a singlecast frame to 0xFFF (broadcast)
         * 2. It's a LR frame
         * 3. The command is either:
         *    - Smart Start Prime, or
         *    - Smart Start Include
         */
        else if ((HDRTYP_SINGLECAST == frameType) &&
                 (HDRFORMATTYP_LR == curHeader) &&
                 (NODE_BROADCAST_LR == GET_SINGLECAST_DESTINATION_NODEID_LR(*pRxFrame)))
        {
          uint8_t * pTempFrame = GetSinglecastPayload(curHeader, pRxFrame);

          typedef struct
          {
            uint8_t cc;
            uint8_t cmd;
          }
          ccc_t;

          ccc_t * ccc = (ccc_t *)pTempFrame;

          if ((ZWAVE_CMD_SMARTSTART_PRIME_NODE_INFO   == ccc->cmd) ||
              (ZWAVE_CMD_SMARTSTART_INCLUDE_NODE_INFO == ccc->cmd) ||
              (ZWAVE_LR_CMD_SMARTSTART_PRIME_NODE_INFO   == ccc->cmd) ||
              (ZWAVE_LR_CMD_SMARTSTART_INCLUDE_NODE_INFO == ccc->cmd))
          {
            frameState &= ~DROP_FRAME;
          }
        }
      }
      else
#endif
      {
        /*learnNodeState is EARN_NODE_STATE_NEW, LEARN_NODE_STATE_UPDATE or LEARN_NODE_STATE_DELETE then qccept all frames*/
        frameState &= ~DROP_FRAME;
      }
    }
    if (frameState & DROP_FRAME)
    {
      /*Override drop frame rule if it a nop power or transmit presentation frame*/
      if ((NULL !=pCmdClass)  && ( ZWAVE_CMD_CLASS_PROTOCOL == *pCmdClass))
      {
         if (
              (ZWAVE_CMD_NOP_POWER == *pCmd) ||
              (ZWAVE_CMD_TRANSFER_PRESENTATION == *pCmd)
            )
         {
           frameState &= ~DROP_FRAME;
         }
      }

      /*Override drop frame rule if it is a LR INIF frame*/
       if ((NULL != pCmdClass)  && (ZWAVE_CMD_CLASS_PROTOCOL_LR == *pCmdClass))
       {
          if (ZWAVE_LR_CMD_INCLUDED_NODE_INFO == *pCmd)
          {
            frameState &= ~DROP_FRAME;
          }
       }
    }
#endif
    DPRINTF("R %02x\r\n",frameState);
    return (frameState & DROP_FRAME);
}

#ifdef ZW_CONTROLLER
/*
  MIT-9: Never NOP known non-listening nodes when receiving Find Nodes In Range, controllers only.
*/
uint8_t  DropPingTest (uint8_t bdestNode)
{
  if ( ZCB_GetNodeType(bdestNode) && ((GetNodeCapabilities(bdestNode) & NODEINFO_LISTENING_SUPPORT) == 0 ))
  {
    return true;
  }
  return false;
}

/*
  MIT-4 SIS MUST ignore all Find Nodes In Range frame
  MIT-5: Inclusion controllers MUST ignore all Find Nodes In Range frame from unknown nodes.
  During learn mode, Find Nodes In Range is always allowed from any unknownnode. Outside learn mode, Find Nodes frames are only allowed if we know the sender.
*/

uint8_t IsFindNodeInRangeAllowed(uint8_t bSourceID)
{
  DPRINTF("O:%02x%02x%02x%02x\r\n",bSourceID, g_learnMode, g_learnNodeState,NEW_CTRL_RECEIVER_BIT);

#ifdef ZW_CONTROLLER_STATIC
  if (ZW_IS_SIS_CONTROLLER)
  {
    return false;
  }
  else
#endif
  {
#ifdef ZW_CONTROLLER_STATIC
    if (ZW_IS_INCLUSION_CONTROLLER && !ZCB_GetNodeType(bSourceID) )
    {
      return false;
    }
    else
#endif
    if (!(g_learnMode || NEW_CTRL_RECEIVER_BIT) && !ZCB_GetNodeType(bSourceID) )
    {
      return false;
    }
  }
  return true;
}
#endif
