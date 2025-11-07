// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file ZW_transport_multi.c
 * @copyright 2022 Silicon Laboratories Inc.
 *
 * @brief Z-Wave Transport layer multicast module.
 */
#include "ZW_lib_defines.h"
#include "string.h"
/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <ZW_typedefs.h>
#ifdef ZW_SLAVE
#include <ZW_slave.h>
#endif  /*ZW_SLAVE*/

#ifdef ZW_CONTROLLER
#include <ZW_controller.h>
#endif /*ZW_CONTROLLER*/

#include <NodeMask.h>

//#define DEBUGPRINT
#include "DebugPrint.h"
#include <ZW_DataLinkLayer.h>
#include <ZW_application_transport_interface.h>
#include <zpal_radio.h>
/****************************************************************************/
/*                      PRIVATE TYPES and DEFINITIONS                       */
/****************************************************************************/

/* Multicast global variables */
static uint8_t bMultiSrcNodeID;
static const uint8_t * pMultiData;
static uint8_t bMultiDataLength;
static uint8_t  bMultiTxOptions;
static STransmitCallback zcbp_multiApplCompletedFunc = { 0 };
static uint8_t bMultiFrameOptions;
static uint8_t bMultiStatus; /*Used to keep track of Multicast transmit status*/

/* Multicast global variables */
#ifdef ZW_CONTROLLER
static bool m_lr_nodeid_list = false;
uint8_t multiIDList[MULTICAST_NODE_LIST_SIZE];
static uint8_t m_AckReceivedIDList[MULTICAST_NODE_LIST_SIZE];
#else
uint8_t multiIDList[MAX_NODEMASK_LENGTH];
#endif


extern bool bUseExploreAsRouteResolution;
extern bool bApplicationTxAbort;
extern void EnQueueCommon(uint8_t rfSpeed, uint8_t frameType, uint8_t const * const pData, uint8_t  dataLength, const STransmitCallback*, TxQueueElement *pFreeTxElement);


#ifdef ZW_CONTROLLER
uint8_t
IsFLiRSinList(
  uint8_t *pMask)
{
  node_id_t curNode;
  uint8_t bFLiRSCount = 0;
  uint8_t nodeListLength;

  if (m_lr_nodeid_list) {
    nodeListLength = MULTICAST_NODE_LIST_SIZE>>1;
  } else  {
    nodeListLength = ZW_MAX_NODES;
  }

  for (uint8_t i = 0; i < nodeListLength; i++)
  {
    if (m_lr_nodeid_list) {
      curNode = pMask[i<<1] << 8; // index = i *2
      curNode |= pMask[(i<<1) +1]; // index = (i *2) +1
    } else {
      curNode = ZW_NodeMaskNodeIn(pMask,i+1)?(i+1):0;
    }
    if ((0 != curNode) && (0 != IsNodeSensor(curNode, false, false))) {
      bFLiRSCount++;
    }
  }
  return (bFLiRSCount);
}
#endif /* #ifdef ZW_CONTROLLER */


static uint8_t
SendMulticastFrame(
  uint8_t      speed,
  node_id_t      bSrcNodeID,
  TxOptions_t  txOptions,
  const uint8_t *pData,                     /* IN Data buffer pointer           */
  uint8_t      dataLength,                /* IN Data buffer length            */
  const STransmitCallback* completedFunc) /* IN Transmit completed call back function  */
{
  TxQueueElement *pFreeTxElement;

  DPRINTF("speed %d\n", speed);
  pFreeTxElement = TxQueueGetFreeElement(TX_QUEUE_PRIORITY_LOW, false);
  if (pFreeTxElement)
  {

    pFreeTxElement->frame.frameOptions.sourceNodeId = bSrcNodeID;
    activeTransmit.nodeIndex = 0;
#ifdef ZW_SLAVE_ROUTING
    /* The ZW_SLAVE_ROUTING library do not have any idea what speeds the different */
    /* nodes support, therefore multicast always 40K */
    speed = RF_SPEED_40K;
    pFreeTxElement->bFLiRSCount = 0;
#else
    /* We need to know the number of FLiRS nodes in the group */
    pFreeTxElement->bFLiRSCount = IsFLiRSinList(multiIDList);
    if (pFreeTxElement->bFLiRSCount && m_lr_nodeid_list) {
      /* There is at least one FLiRS node in the list, send broadcast wakeup beam */
      speed = (RF_SPEED_LR_100K | RF_OPTION_SEND_BEAM_1000MS);
      pFreeTxElement->frame.frameOptions.destinationNodeId = BEAM_BROADCAST_LR;
      pFreeTxElement->bBeamAddress = BEAM_BROADCAST_LR;
    } else if (pFreeTxElement->bFLiRSCount) {
      speed = (RF_SPEED_40K | RF_OPTION_SEND_BEAM_1000MS);
      pFreeTxElement->frame.frameOptions.destinationNodeId = BEAM_BROADCAST;
      pFreeTxElement->bBeamAddress = BEAM_BROADCAST;
    }
#endif
    TxQueueInitOptions(pFreeTxElement, txOptions);
    if (TxQueueGetOptions(pFreeTxElement) & TRANSMIT_OPTION_MULTICAST_AS_BROADCAST)
    {
#ifdef ZW_CONTROLLER
      /* Send as broadcast */
      if (m_lr_nodeid_list) {
        pFreeTxElement->frame.frameOptions.destinationNodeId = NODE_BROADCAST_LR;
      } else {
        pFreeTxElement->frame.frameOptions.destinationNodeId = NODE_BROADCAST;
      }
#else
      pFreeTxElement->frame.frameOptions.destinationNodeId = NODE_BROADCAST;
#endif
      /* Remove Tx option again to avoid low power transmission */
      TxQueueClearOptionFlags(pFreeTxElement, (TRANSMIT_OPTION_MULTICAST_AS_BROADCAST | TRANSMIT_OPTION_ACK));
      bMultiTxOptions &= ~TRANSMIT_OPTION_MULTICAST_AS_BROADCAST;
      EnQueueCommon(speed, FRAME_TYPE_SINGLE, pData, dataLength,
                    completedFunc, pFreeTxElement);
    }
    else
    {
      DPRINT("Multi\n");
      TxQueueClearOptionFlags(pFreeTxElement, TRANSMIT_OPTION_ACK);
      /* Send as multicast */
      EnQueueCommon(speed, FRAME_TYPE_MULTI, pData, dataLength,
                    completedFunc, pFreeTxElement);
    }
    return (true);
  }
  return(false);
}

/*============================   MulticastCompletedFunc   ===================
**    Transmit complete callback function used for sending multicast
**    with ack
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
void
ZCB_MulticastCompletedFunc(
  __attribute__((unused)) ZW_Void_Function_t Context,
  uint8_t bStatus,                        /* IN   Transmit completion status  */
  __attribute__((unused)) TX_STATUS_TYPE* TxStatusReport)
{
  if (bStatus != TRANSMIT_COMPLETE_OK)
  {
    bMultiStatus = bStatus;
  }
#ifdef ZW_CONTROLLER
  if (m_lr_nodeid_list)
  {
     ZW_TransmitCallbackInvoke(&zcbp_multiApplCompletedFunc, bMultiStatus, 0);
     return;
  }
#endif
  activeTransmit.nodeIndex = ZW_NodeMaskGetNextNode(activeTransmit.nodeIndex,  multiIDList);  
  /* Send follow up frame to next node */
  if (activeTransmit.nodeIndex && !bApplicationTxAbort)
  {
    /* Should we resort to explore frame as route resolution if all else fails? */
    bUseExploreAsRouteResolution = ((bMultiFrameOptions & TRANSMIT_OPTION_EXPLORE) != 0);

    const STransmitCallback TxCallback = { .pCallback = ZCB_MulticastCompletedFunc, .Context = 0 };
    if (EnQueueSingleData(false,
                          bMultiSrcNodeID,
                          activeTransmit.nodeIndex,
                          pMultiData,
                          bMultiDataLength,
                          bMultiTxOptions | TRANSMIT_OPTION_FOLLOWUP,
                          0, // 0ms for tx-delay (any value)
                          ZPAL_RADIO_TX_POWER_DEFAULT,
                          &TxCallback) == false)
    {
      bMultiStatus = TRANSMIT_COMPLETE_FAIL;
      ZW_TransmitCallbackInvoke(&TxCallback, bMultiStatus, 0);
    }
  }
  else
  {
    ZW_TransmitCallbackInvoke(&zcbp_multiApplCompletedFunc, bMultiStatus, 0);
  }
}

#ifdef ZW_CONTROLLER
/* When sending multi frame check we use same speed, if not check if one of the of the destination nodes is 9.6 */
/* if one of the nodes is 9.6 send with this speed */
static 
uint8_t CheckLegacyMulticastNodes(uint8_t *pNodeList, uint8_t dataLength)
{
  uint8_t speed = RF_SPEED_100K;
  uint8_t NodeId = ZW_NodeMaskGetNextNode(0, pNodeList);
  while (NodeId != 0)
  {
    if (!DoesNodeSupportSpeed(NodeId, ZW_NODE_SUPPORT_SPEED_100K))
    {
      if (!llIsHeaderFormat3ch() && (dataLength > MAX_MULTICAST_PAYLOAD_LEGACY))
      {
        return 255;
      }
      if (IsNode9600(NodeId))
      {
        speed = RF_SPEED_9_6K;
        break;
      }
      else
      {
        speed = RF_SPEED_40K;
      }
    }
    NodeId = ZW_NodeMaskGetNextNode(NodeId, pNodeList);
  }
  return speed;
}
#endif
/*===============================   ZW_SendDataMultiWorker  =================
**    Transmit data buffer to a list of Z-Wave nodes.
**
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
uint8_t
ZW_SendDataMultiWorker(
  node_id_t    bSrcNodeID,
  uint8_t     *pNodeIDList,  /* IN List of destination node IDs  */
  const uint8_t *pData,        /* IN Data buffer pointer           */
  uint8_t      dataLength,   /* IN Data buffer length            */
  TxOptions_t  txOptions,    /* IN Transmit option flags         */
  const STransmitCallback* pCompletedFunc) /* IN Transmit completed call back function  */
{
#ifdef ZW_CONTROLLER
  uint8_t speed = RF_SPEED_100K;
#else
  uint8_t speed = RF_SPEED_40K;
#endif
  /* make sure only TRANSMIT OPTIONS valid for Application are used */
  txOptions &= TRANSMIT_APPLICATION_OPTION_MASK;
  /* Do we want to use the Explore Frame as a route resolution if all else fails */
  bMultiFrameOptions = (txOptions & TRANSMIT_OPTION_EXPLORE);

  /* Save multicast data for later use */
  bMultiSrcNodeID = bSrcNodeID;
  zcbp_multiApplCompletedFunc = *pCompletedFunc;
  pMultiData = pData;
  bMultiDataLength = dataLength;
  bMultiTxOptions = txOptions | TRANSMIT_OPTION_APPLICATION;  /* SendDataAbort */
  bMultiStatus = TRANSMIT_COMPLETE_OK; /*Reset Transmit Status*/
  const STransmitCallback TxCallback = { .pCallback = ZCB_MulticastCompletedFunc, .Context = 0 };

  /* SendDataAbort */
  bApplicationTxAbort = false;
#ifdef ZW_CONTROLLER
  if (m_lr_nodeid_list) {
    speed = RF_SPEED_LR_100K;
    memcpy(multiIDList, pNodeIDList, MULTICAST_NODE_LIST_SIZE);
  } else {
    memcpy(multiIDList, pNodeIDList, MAX_NODEMASK_LENGTH);
    /* When sending multi frame check we use same speed, if not check if one of the of the destination nodes is 9.6 */
    /* if one of the nodes is 9.6 send with this speed */
    speed = CheckLegacyMulticastNodes(pNodeIDList, dataLength);
    if (255 == speed)
    {
      return false;
    }
  }
#else
  memcpy(multiIDList, pNodeIDList, MAX_NODEMASK_LENGTH);
#endif
  if (dataLength > MAX_MULTICAST_PAYLOAD)
  {
      return false;
  }
  if (txOptions & TRANSMIT_OPTION_ACK)
  {
    /* Send multicast with follow-up singlecast */
    return (SendMulticastFrame(speed, bSrcNodeID, txOptions, pData, dataLength,
                               &TxCallback));
  }
  else
  {
    /* Only send multicast */
    return (SendMulticastFrame(speed, bSrcNodeID, txOptions, pData, dataLength,
                               &zcbp_multiApplCompletedFunc));
  }
}


#ifndef ZW_CONTROLLER_BRIDGE
/*===============================   ZW_SendDataMulti   ======================
**    Transmit data buffer to a list of Z-Wave nodes.
**
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
uint8_t
ZW_SendDataMulti(
  uint8_t     *pNodeIDList,  /* IN List of destination node IDs  */
  const uint8_t *pData,      /* IN Data buffer pointer           */
  uint8_t      dataLength,   /* IN Data buffer length            */
  TxOptions_t  txOptions,    /* IN Transmit option flags         */
  const STransmitCallback* pCompletedFunc)  /* IN Transmit completed call back function  */
{
  return ZW_SendDataMultiWorker(g_nodeID, pNodeIDList, pData, dataLength, txOptions, pCompletedFunc);
}


#else   /*ZW_CONTROLLER_BRIDGE*/
/*===========================   ZW_SendDataMulti_Bridge   ====================
**    Transmit data buffer to a list of Z-Wave nodes.
**
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
uint8_t
ZW_SendDataMulti_Bridge(
  node_id_t    bSrcNodeID,    /*IN Source nodeID - if 0xFF then controller is set as source */
  uint8_t     *pNodeIDList,   /*IN List of destination node ID's */
  uint8_t     *pData,         /*IN Data buffer pointer           */
  uint8_t      dataLength,    /*IN Data buffer length            */
  TxOptions_t  txOptions,     /*IN Transmit option flags         */
  bool lr_nodeid_list,           /*IN type of the ndoe ids in the node id list*/
  const STransmitCallback* pCompletedFunc) /*IN  Transmit completed call back function  */
{
  m_lr_nodeid_list = lr_nodeid_list;
  memset(m_AckReceivedIDList, 0, MULTICAST_NODE_LIST_SIZE);
  if (!ZW_IsVirtualNode(bSrcNodeID))
  {
    /* If srcNodeID is not a VirtualNode then it can only be nodeID - no spoofing allowed. */
    bSrcNodeID = g_nodeID;
  }
  if (m_lr_nodeid_list) {
    txOptions |= TRANSMIT_OPTION_MULTICAST_AS_BROADCAST;
    txOptions &= ~TRANSMIT_OPTION_ACK;
  }
  return ZW_SendDataMultiWorker(bSrcNodeID, pNodeIDList, pData, dataLength, txOptions, pCompletedFunc);
}

#endif /* ZW_CONTROLLER_BRIDGE */


#ifdef ZW_BEAM_RX_WAKEUP
/*===========================   ZW_FollowUpReceived   ======================
**    A follow up frame was received, enable FLiRS broadcast in RF driver
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
void ZW_FollowUpReceived(void)
{
  zpal_radio_enable_rx_broadcast_beam(true);
}
#endif

#ifdef ZW_CONTROLLER
static
bool UpdateReceivedAckIDList(uint8_t id_index, node_id_t node_id)
{
  node_id_t curNode = ((uint16_t)m_AckReceivedIDList[id_index<<1] << 8); // index = i *2
  curNode |= (uint16_t)m_AckReceivedIDList[(id_index<<1) +1]; // index = (i *2) +1

  if (0 == curNode) {
    m_AckReceivedIDList[id_index << 1] = (uint8_t)(node_id >> 8);
    m_AckReceivedIDList[(id_index << 1) + 1] = (uint8_t)(node_id & 0xFF);
    return true;
  } else {
    return false;
  }
}

bool
ZW_broadcast_beam_ack_id_update(node_id_t node_id)
{
  if (m_lr_nodeid_list) {
    node_id_t curNode;
    /*If the node id is in the mutlicast_id list and not in the received_ack_id list then its first time we receive the ack frame.
      update the receive_ack_id list and return true, else return false
    */
    for (uint8_t i = 0; i < (MULTICAST_NODE_LIST_SIZE >> 1); i++) {
      curNode = ((uint16_t)multiIDList[i << 1] << 8);   // index = i *2
      curNode |= (uint16_t)multiIDList[(i << 1) + 1]; // index = (i *2) +1
      if (curNode && (curNode == node_id)) {
         return UpdateReceivedAckIDList(i, node_id);
      }
    }
  } else  {
    if (ZW_NodeMaskNodeIn(m_AckReceivedIDList, (uint8_t)(node_id & 0xFF))) {
      return false;
    } else if (ZW_NodeMaskNodeIn(multiIDList, (uint8_t)(node_id & 0xFF))) {
      ZW_NodeMaskSetBit(m_AckReceivedIDList, (uint8_t)(node_id & 0xFF));
      return true;
    }
  }
  return false;
}
#endif
