// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file ZW_replication.c
 * @copyright 2022 Silicon Laboratories Inc.
 *
 * @brief Controller replication functions.
 */
#include "ZW_lib_defines.h"

/***************************************************************************
 *		INCLUDE FILES:  LIBRARY INTERFACES			    *
 ****************************************************************************/
#include <ZW_replication.h>
#include <string.h>
#include <ZW_controller.h>
#include <ZW_routing.h>
#include <ZW_basis.h>
#include <ZW_timer.h>
#include <SwTimer.h>
#include <ZW_explore.h>
#include <ZW_controller_network_info_storage.h>
#include <ZW_inclusion_controller.h>
//#define DEBUGPRINT
#include "DebugPrint.h"
#include <NodeMask.h>

/****************************************************************************
 *		EXTERNAL DATA
 ***************************************************************************/
extern bool pendingUpdateOn;
extern bool pendingTableEmpty;

extern bool inclusionHomeIDActive;

extern bool spoof;

/****************************************************************************
 *		LOCAL DATA
 ***************************************************************************/
#define TRANSFER_PRESENTATION_TIMEOUT         2000  /* 2 sec */
#define TRANSFER_INFO_SEND_TIMEOUT            2000  /* 2 sec time out on transferinfo */

/* Replication Receive timeout defines */
#define REPLICATION_RECEIVE_TIMEOUT           2000  /* 2 sec pr. replication receive tick */
#define REPLICATION_RECEIVE_TIMEOUT_RETRY     10    /* 20 sec - We wait max 20 sec for next replication event */
#define REPLICATION_RECEIVE_TIMEOUT_EXCLUSION 1     /* 2 sec after being excluded */

#define PRIMARY_REPLICATION_IDLE              0
#define PRIMARY_REPLICATION_COMPLETE_SENT     1
#define PRIMARY_REPLICATION_NOP_SENT          2

/*Transfer end buffer is needed because the protocol might  have started another transmission*/
TRANSFER_END_FRAME transferEndBuf;


/* TO#1499 fix - give transfer presentation its own buffer as its transmitted via a timer */
TRANSFER_PRESENTATION_FRAME transferPresFrame;

/* Current replication state */
uint8_t replicationStatus = NEW_CTRL_STOP;

#define MAX_REPL_RETRY 5

uint8_t replRetry = 0;

/* Node ID of the other controller */
node_id_t otherController;

uint8_t replTXOptions;

/* Is the Controller considered included? */
bool controllerIncluded;
bool bReplicationDontDeleteOnfailure;

/* flag to indicate that this is a primary controller replication*/

bool newPrimaryReplication = false;
#if defined(ZW_CONTROLLER_STATIC)
bool doChangeToSecondary = false;
#endif
/* Flag to indicate that we shall send ZWAVE_CMD_TRANSFER_PRESENTATION with high power. */

bool transferPresentationHighPower;

/* TO#2647 fix - Flag to tell if Application has not been notified regarding replication started */
bool appReplicationNotNotified;

/* Global nodeinfo buffer */
LEARN_INFO_T glearnNodeInfo;

/* Timer used for retransmission of transfer presentation and node info */
static SSwTimer PresentationTimer;

/*ReplicationSend callback function. Is stored here..*/
static STransmitCallback ReplCompleteFunction;

/* Number increased for each package sent/received */
uint8_t sequenceNumber;

/* New controller Complete call back */
static VOID_CALLBACKFUNC(zcbp_CtrlCompleteFunction)(LEARN_INFO_T*);


static void ZCB_ReplicationEndsNow(ZW_Void_Function_t Context, uint8_t bTxStatus, TX_STATUS_TYPE *txStatusReport);

uint8_t bNextNode;

/* Handle to transfer info and replication receive Timeout timer */
static SSwTimer replicationTimer;

/* Local  functions */

uint8_t TransferInfo(void);
void ZCB_SendNextNode(ZW_Void_Function_t Context, uint8_t bTxStatus, TX_STATUS_TYPE *txStatusReport);
void ZCB_TransferExpired( SSwTimer* pTimer);

void
ZCB_LearnNodeCallback( /* RET Nothing */
  LEARN_INFO_T *learnNodeInfo); /* IN Learn status and information */

void
ZCB_ReplicationSendTimeout(SSwTimer* pTimer);

/*==========================   StopPresentationTimer   =======================
**    Function description
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
static void
StopPresentationTimer( void )
{
  TimerStop(&PresentationTimer);
}


/*=======================   ReplicationTimeoutStop   =========================
**  Stop the Replication timeout timer
**    Side effects:
**
**--------------------------------------------------------------------------*/
void
ReplicationTimeoutStop( void )
{
  TimerStop(&replicationTimer);
}


/*========================== ReplicationReceiveTimeout  ======================
**    Function description
**      Replication receive timeout function. Ends the Receive if nothing
**      has happened for XXX
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
void
ZCB_ReplicationReceiveTimeout(SSwTimer* pTimer)
{
  if (!--replRetry)
{
    /* TO#1964 possible fix - make sure findInProgress is reset - so its ready for next inclusion try */
    g_findInProgress = false;
    TimerStop(pTimer); /* Make sure its stopped */
    /* Replication ends succesfull if controllerIncluded is set or controllerOnOther and g_nodeID == 0 */
    ReplicationTransferEndReceived(otherController, ZWAVE_TRANSFER_FAIL);
  }
}


/*=======================   StartReplicationReceiveTimer   ===================
**    Function description
**      Start/Restart Replication Receive timeout handling
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
void
StartReplicationReceiveTimer(void)
{
  replRetry = (0 == g_nodeID) ? REPLICATION_RECEIVE_TIMEOUT_EXCLUSION : REPLICATION_RECEIVE_TIMEOUT_RETRY;
  TimerSetCallback(&replicationTimer, ZCB_ReplicationReceiveTimeout);
  TimerStart(&replicationTimer, REPLICATION_RECEIVE_TIMEOUT);
}

/*========================= TransferInfoTimeoutStart  ========================
**    Function description
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
static void
TransferInfoTimeoutStart(void)
{
  TimerSetCallback(&replicationTimer, ZCB_ReplicationSendTimeout);
  TimerStart(&replicationTimer, TRANSFER_INFO_SEND_TIMEOUT);
}


/*========================= ReplicationSendTimeout  =========================
**    Function description
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
void
ZCB_ReplicationSendTimeout( SSwTimer*  pTimer )
{
  TimerStop(pTimer);
  if (IS_REPLICATION_SEND_ONGOING)
  {
    /* If we received a correct command complete frame while we are waiting for */
    /* node/range info transfer to be done, process the next node after transfer */
    /* is done and not when the command complete is received */
    CLEAR_REPLICATION_SEND_ONGOING; /* Force it to be not ongoing */
    TransferInfoTimeoutStart();
  }
  else
  {
    if (!IS_WAIT_FOR_COMPLETE_BIT)
    {
      ReplicationSendDoneCallback(TRANSMIT_COMPLETE_OK);
    }
    else
    {
      ReplicationSendDoneCallback(TRANSMIT_COMPLETE_FAIL);
    }
  }
}


/*============================   SendTransferEnd  ===========================
**    Function description
**      Use this function terminate replication.
**    Side effects:
**      Stops replication.
**--------------------------------------------------------------------------*/
void
ZCB_SendTransferEnd(
  __attribute__((unused)) ZW_Void_Function_t Context,
  uint8_t state,
  __attribute__((unused)) TX_STATUS_TYPE *txStatusReport)
{
  CLEAR_SEND_END_BIT;
  transferEndBuf.cmdClass = ZWAVE_CMD_CLASS_PROTOCOL;
  transferEndBuf.cmd      = ZWAVE_CMD_TRANSFER_END;
  transferEndBuf.status   = (state && !controllerIncluded) ? ZWAVE_TRANSFER_FAIL : ZWAVE_TRANSFER_OK;
  /* If we are deleting a node make sure to transmit to node ID 0x00 */
  if (GET_NEW_CTRL_STATE == NEW_CTRL_DELETE)
  {
    if (!state)
    {
      otherController = 0x00;
      ZW_HomeIDClear();
    }
    CHANGE_NEW_CTRL_STATE(NEW_CTRL_CREATED);
  }
  else if (controllerIncluded || (state == TRANSMIT_COMPLETE_OK))
  {
    /* We have successfully created a new controller */
    CHANGE_NEW_CTRL_STATE(NEW_CTRL_CREATED);
  }
  /* Send transfer END frame to slave controller */
  static const STransmitCallback TxCallback = { .pCallback = ZCB_ReplicationEndsNow, .Context = 0 };
  if (!EnQueueSingleData(false, g_nodeID, otherController, (uint8_t *)&transferEndBuf,
                         sizeof(TRANSFER_END_FRAME),
                         replTXOptions,
                         0, // 0ms for tx-delay (any value)
                         ZPAL_RADIO_TX_POWER_DEFAULT,
                         &TxCallback))
  {
    ZCB_ReplicationEndsNow(0, TRANSMIT_COMPLETE_FAIL, NULL);
  }
}


/*============================   NewControllerEnd  ===========================
**    Function description
**      Last function to call before ending replication
**    Side effects:
**      Stops replication.
**--------------------------------------------------------------------------*/
static void
ZCB_NewControllerEnd(
  __attribute__((unused)) ZW_Void_Function_t Context,
  __attribute__((unused)) uint8_t txStatus,
  __attribute__((unused)) TX_STATUS_TYPE *txStatusReport)
{
  /* We don't care about if SUC node ID end OK we just send the transfer ok frame */
  if (newPrimaryReplication)
  {
    assignIdBuf.TransferNewPrimary.cmdClass = ZWAVE_CMD_CLASS_PROTOCOL;
    assignIdBuf.TransferNewPrimary.cmd      = ZWAVE_CMD_TRANSFER_NEW_PRIMARY_COMPLETE;
    assignIdBuf.TransferNewPrimary.sourceControllerType = getAppNodeInfo()->NodeType.generic;

    static const STransmitCallback TxCallback = { .pCallback = ZCB_SendTransferEnd, .Context = 0 };
    if (!EnQueueSingleData(false, g_nodeID, otherController, (uint8_t *)&assignIdBuf,
                           sizeof(TRANSFER_NEW_PRIMARY_FRAME),
                           replTXOptions,
                           0, // 0ms for tx-delay (any value)
                           ZPAL_RADIO_TX_POWER_DEFAULT,
                           &TxCallback))
    {
      ZCB_SendTransferEnd(0, TRANSMIT_COMPLETE_FAIL, NULL);
    }
  }
  else
  {
    ZCB_SendTransferEnd(0, TRANSMIT_COMPLETE_OK, NULL);
  }
}


/****************************************************************************
 *		FUNCTION DECLARATIONS
 ***************************************************************************/

/*=========================   ReplicationSendCallback   ======================
**    Function description
**      Internal callback for ReplicationSend
**      Starts timeout to catch missing CMD_COMPLETE
**    Side effects:
**
**--------------------------------------------------------------------------*/
static void
ZCB_ReplicationSendCallback(
  __attribute__((unused)) ZW_Void_Function_t Context,
  __attribute__((unused)) uint8_t bTxStatus,                 /**/
  __attribute__((unused)) TX_STATUS_TYPE *txStatusReport)
{
  /* Restart time out */
  TransferInfoTimeoutStart();
  CLEAR_REPLICATION_SEND_ONGOING;
  if (!IS_WAIT_FOR_COMPLETE_BIT)
  {
    /* If we received a correct command complete frame while we are waiting for */
    /* node/range info transfer to be done, process the next node after transfer */
    /* is done and not when the command complete is received */
    ReplicationSendDoneCallback(TRANSMIT_COMPLETE_OK);
  }
}


/*============================   ReplicationEndsNow  =========================
**    Function description
**        Called when TRANSFER_END has been transmitted.
**    Side effects:
**
**--------------------------------------------------------------------------*/
static
void
ZCB_ReplicationEndsNow(
  __attribute__((unused)) ZW_Void_Function_t Context,
  __attribute__((unused)) uint8_t bTxStatus,             /* status TRANSMIT_COMPLETE_OK for last transmission ok*/
  __attribute__((unused)) TX_STATUS_TYPE *txStatusReport)
{
  uint8_t newctrlstate = GET_NEW_CTRL_STATE; /* Get it, then reset it... */
  /* LearnState is disabled in TransferDoneCallback */
  /* If we are in table then there is nothing to tell */
  /* In order to send to a deleting controller we might have zeroed the homeID. */
  /* So reload */
  ControllerStorageGetNetworkIds(ZW_HomeIDGet(), &g_nodeID);

  /* We dont care if the transmission of ZWAVE_CMD_TRANSFER_END was ACK'ed or not */
  /* Only if the copy process was ok */
  if (newctrlstate == NEW_CTRL_CREATED)
  {
    if (newPrimaryReplication)
    {
      /* TO#1513 fix - nosuc static controller cannot do new primary and */
      /* must always change to secondary if no SIS in network */
#if defined(ZW_CONTROLLER_STATIC)
      if (doChangeToSecondary)
#endif
      {
        realPrimaryController = false;
        primaryController = isNodeIDServerPresent();
        /* TO#3836 fix - this could still be our original homeID - so do not change controllerOnOther setting */
        //controllerOnOther = true;
        SaveControllerConfig();
      }
    }
    TransferDoneCallback(NEW_CONTROLLER_DONE);
  }
  else
  {
    /* No new ctrl was created. Abort and tell application */
#if defined(ZW_CONTROLLER_STATIC)
    if (doChangeToSecondary)
#else
    if (newPrimaryReplication)
#endif  /* ZW_CONTROLLER_STATIC */
    {
      if (otherController)
      {
        AddNodeInfo(otherController, NULL, false); /* Remove other controller */
      }
      /* If we was a primary on own network, we go back to the "original" homeID */
      if (primaryController && !controllerOnOther)
      {
        /* TODO - should we erase the nodetable and...? */
        /* NO - we have now cleared up the changes we made in vain, so go on */
        ControllerStorageSetNetworkIds(ZW_HomeIDGet(), NODE_CONTROLLER); /* Clear "internal" HomeID */
      }
    }
    else
    {
      /* Do we consider the controller as included? */
      if (otherController && !controllerIncluded)
      {
        /* Does the node exist? */
        if (ZCB_GetNodeType(otherController))
        {
          DPRINTF("=%d", g_learnNodeState);
          if (bReplicationDontDeleteOnfailure == false)
          {
            AddNodeInfo(otherController, NULL, false); /* Remove other controller */
#if defined(ZW_CONTROLLER_STATIC) && !defined(ZW_CONTROLLER_NOSUC)
            if (!ZW_IS_SUC)
            {
#endif
              SetPendingUpdate(otherController);
#if defined(ZW_CONTROLLER_STATIC) && !defined(ZW_CONTROLLER_NOSUC)
            }
#endif
          }
        }
      }
    }
    if (newctrlstate != NEW_CTRL_STOP)
    {
      /* HEH check added to avoid Calling NEW_CONTROLLER_FAILED when an ACK to TRANSFER_END */
      /* is received with great delay TO# 1153 */
      TransferDoneCallback(NEW_CONTROLLER_FAILED);
    }
  }
  otherController = 0;
}


/*======================   ReplicationSendDoneCallback =======================
**    Function description
**      Calls the application callback function with status
**      when a replication send have been completed including CMD_COMPLETE
**    Side effects:
**
**--------------------------------------------------------------------------*/
void
ReplicationSendDoneCallback(
  uint8_t bStatus)                     /*IN TRANSMIT_COMPLETE_OK
                                    or TRANSMIT_COMPLETE_FAIL*/
{
  /* We are done stop the time out */
  ReplicationTimeoutStop();

  ZW_TransmitCallbackInvoke(&ReplCompleteFunction, bStatus, NULL);
}


/*======================   WaitforRoutingAnalysis ============================
**    Function description
**      wait until the routing analysis process is done, before calling the
**      call back function
**    Side effects:
**--------------------------------------------------------------------------*/
void
ZCB_WaitforRoutingAnalysis(SSwTimer* pTimer)
{
  DPRINT("W");
#ifndef NO_PREFERRED_CALC
  if (RoutingInfoState == ROUTING_STATE_IDLE)
  {
#endif
    if (NULL != pTimer)
    {
      /*  ZCB_WaitforRoutingAnalysis is called directly and not by timer timeout*/
      TimerStop(pTimer);
    }
    /*Attempt to run PendingNodeUpdate after each inclusion/exclusion*/
    /* The PendingNodeUpdate should be called only once to start the flushing process */
    /* If the proecess is already started or the pending table is empty then PendingNodeUpdate() should not be called */
    if (!pendingUpdateOn && !pendingTableEmpty)
    {
      PendingNodeUpdate();
     /* We should not call ZCB_LearnNodeCallback before the network update process is done*/
      return;
    }
    /* TODO - Why only if CtrlCompleteFunction defined ??? */
    if (zcbp_CtrlCompleteFunction)
    {
      ZCB_LearnNodeCallback(&glearnNodeInfo);

      newPrimaryReplication = false;
#if defined(ZW_CONTROLLER_STATIC)
      doChangeToSecondary = false;
#endif
    }

    if (resetCtrl)
    {
      resetCtrl = false;
      ZW_SetDefault();
    }
#ifndef NO_PREFERRED_CALC
  }
#endif
}


/*========================   TransferDoneCallback   ==========================
**
**    Disable transfer state machine  and call application callback
**    function. This function is valid for both sending and receiving controller
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
void
TransferDoneCallback( /* RET Nothing */
  uint8_t bStatus)       /* IN  NEW_CONTROLLER_DONE on succes and
                             NEW_CONTROLLER_FAILED on failure */
{
  DPRINTF("TransferDoneCallback: %x:%x\n", bStatus,g_learnModeClassic);
  glearnNodeInfo.bStatus = bStatus;
  glearnNodeInfo.bSource = otherController;
  glearnNodeInfo.pCmd = NULL;
  glearnNodeInfo.bLen = 0;
  g_learnMode = false;

  #ifdef ZW_REPEATER
  if ((!g_learnModeClassic) && (NEW_CTRL_RECEIVER_BIT) && (bStatus == NEW_CONTROLLER_DONE))
  {
    /* If the node got included and are in NWI, then let the repeater help in the NWI senario - */
    /* by starting repeating autoinclusion requests. */
    ExploreSetNWI(true);
  }
#endif  /* ZW_REPEATER */
  g_learnModeClassic = false;

  /* TO#2478 Use own node id if we have been included */
  /* If were included then use own nodeID */
  if ((NEW_CTRL_RECEIVER_BIT) && (bStatus == NEW_CONTROLLER_DONE))
  {
    glearnNodeInfo.bSource = g_nodeID;
  }
  INIT_NEW_CTRL_STATE(NEW_CTRL_STOP);


  inclusionHomeIDActive = false;

  StopPresentationTimer();
  ControllerStorageGetNetworkIds(ZW_HomeIDGet(), &g_nodeID);
  /* Were done replicating. If we did not fail analyse the routing table */
  if (bStatus != NEW_CONTROLLER_FAILED)
  {
    RestartAnalyseRoutingTable();
  }
  ReplicationTimeoutStop();
#ifdef NO_PREFERRED_CALC
  ZCB_WaitforRoutingAnalysis(NULL);
#else
  /* Delay the sending of the Transfer until we are ready */
  TimerSetCallback(&replicationTimer, ZCB_WaitforRoutingAnalysis);
  TimerStart(&replicationTimer, 1);
#endif
}


/*============================   SendNextNode   ==============================
**
**    Transmit complete function. Called after a replication send
**    If it fails The nodeID should be deleted and replication should be stopped.
**
**    Side effects:
**--------------------------------------------------------------------------*/
void
ZCB_SendNextNode(
  __attribute__((unused)) ZW_Void_Function_t Context,
  uint8_t bTxStatus,
  __attribute__((unused)) TX_STATUS_TYPE *txStatusReport)
{
  if ((bTxStatus != TRANSMIT_COMPLETE_OK) && (IS_WAIT_FOR_COMPLETE_BIT))
  {
    if (replRetry <= MAX_REPL_RETRY)
    {
      while (TransferInfo())
        ;
    }
    else
    {
      /* We have tried MAX_REPL_RETRY + 1 times and failed everytime */
      replRetry++;
    }
    /* TO#1206 - If we fail to transmit then we have to stop replication */
    if (replRetry > MAX_REPL_RETRY + 1)
    {
      replRetry = 0;  /* Now we are failed! */
      /* If we fail here signal to slave controller that replication failed. */
      /* Do not delete the nodeID assigned though! */
      if (GET_NEW_CTRL_STATE != NEW_CTRL_STOP)
      {
        /* Transmit complete failed.. Terminate */
        ZCB_SendTransferEnd(0, TRANSMIT_COMPLETE_NO_ACK, NULL);
      }
      /* We failed. STOP Replication - Just do it! */
    }
  }
  else
  {
    if (GET_NEW_CTRL_STATE == NEW_CTRL_SEND_RANGE)
    {
      /* We have received a command complete for our Transfer RangeInfo */
      /* Now we consider the controller as included */
      controllerIncluded = true;
    }
    TransferCmdCompleteReceived();
  }
}


/*===========================   TransferInfo   ===============================
**
**    Transfer node and range information for the next node that is known
**    by this controller
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
uint8_t
TransferInfo( void ) /* RET Nothing */
{
  static uint8_t bSize;

  if (!replRetry)
  {
    SET_WAIT_FOR_COMPLETE_BIT;   /* always wait for command complete when sending transfer node/range info frame */
    bNextNode++;
    for (; bNextNode <= bMaxNodeID; bNextNode++)
    {
      if (ZCB_GetNodeType(bNextNode))
      {
        if (GET_NEW_CTRL_STATE == NEW_CTRL_SEND_NODES)
        {
          /* Node exist, build frame */
          assignIdBuf.TransNodeInfoFrame.cmdClass = ZWAVE_CMD_CLASS_PROTOCOL;
          assignIdBuf.TransNodeInfoFrame.cmd      = ZWAVE_CMD_TRANSFER_NODE_INFO;
          assignIdBuf.TransNodeInfoFrame.nodeID   = bNextNode;
          ZW_GetNodeProtocolInfo(bNextNode,
                                 &assignIdBuf.TransNodeInfoFrame.nodeInfo);
          bSize = sizeof(TRANSFER_NODE_INFO_FRAME);
          if (!(assignIdBuf.TransNodeInfoFrame.nodeInfo.security & ZWAVE_NODEINFO_CONTROLLER_NODE))  /* Slave node ? */
          {
            /* When we transmit slave nodeinfo the Basic Device Type is not transmitted... */
            assignIdBuf.TransNodeInfoFrame.nodeInfo.nodeType.basic = assignIdBuf.TransNodeInfoFrame.nodeInfo.nodeType.generic;
            assignIdBuf.TransNodeInfoFrame.nodeInfo.nodeType.generic = assignIdBuf.TransNodeInfoFrame.nodeInfo.nodeType.specific;
            bSize--;
          }
        }
        else
        {
          /* Node exist, check for routing info */
          /* TO#01787 fix - Assign Return Route incorrect for FLiRS Devices */
          ZW_GetRoutingInfo( bNextNode,
                             assignIdBuf.TransferRangeFrame.maskBytes,
                             0);

          if (ZW_NodeMaskBitsIn(assignIdBuf.TransferRangeFrame.maskBytes,
                                MAX_NODEMASK_LENGTH))
          {
            /* Build frame to slave controller */
            assignIdBuf.TransferRangeFrame.cmdClass = ZWAVE_CMD_CLASS_PROTOCOL;
            assignIdBuf.TransferRangeFrame.cmd = ZWAVE_CMD_TRANSFER_RANGE_INFO;
            assignIdBuf.TransferRangeFrame.nodeID       = bNextNode;
            assignIdBuf.TransferRangeFrame.numMaskBytes = MAX_NODEMASK_LENGTH;
            bSize = sizeof(TRANSFER_RANGE_INFO_FRAME);
          }
          else
          {
            continue;
          }
        }
        /* Send frame to slave controller */
        static const STransmitCallback TxCallback = { .pCallback = ZCB_SendNextNode, .Context = 0 };
        if (!ReplicationSend(otherController, (uint8_t *)&assignIdBuf, bSize,
                                replTXOptions,
                                &TxCallback))
        {
          /* Start replication timer here*/
          TransferInfoTimeoutStart();

          /* If we cannot put frame in queue then we stop replication - let SendNextNode do it */
          /* Delay it some - Try again in a bit.. */
          CLEAR_REPLICATION_SEND_ONGOING;
        }
        replRetry++;
        return false;
      }
    }
  }
  else
  {
    /* Send frame to slave controller */
    /* TO#1206 - A missing ! infront of ReplicationSend resulted in replication always failing */
    /* if first transfer rangeinfo/nodeinfo try failed (with 3 tries) not after 6 transfer */
    /* rangeinfo/nodeinfo replication tries failed as it should */
    static const STransmitCallback TxCallback = { .pCallback = ZCB_SendNextNode, .Context = 0 };
    if (!ReplicationSend(otherController, (uint8_t *)&assignIdBuf, bSize,
                            replTXOptions,
                            &TxCallback))
    {
      /* Start replication timer here*/
      TransferInfoTimeoutStart();
      /*If we cannot put frame in queue then we stop replication - let SendNextNode do it */
      /* Try again in a bit.. */
      CLEAR_REPLICATION_SEND_ONGOING;
    }
    replRetry++;
    return false;
  }
  /* Change status to send routing info */
  if (GET_NEW_CTRL_STATE == NEW_CTRL_SEND_NODES)
  {
    CHANGE_NEW_CTRL_STATE(NEW_CTRL_SEND_RANGE);
    bNextNode = 0;
    return true;
  }
  /* Indicate that the protocol is all done now and hand control to application */
  CHANGE_NEW_CTRL_STATE(NEW_CTRL_SEND);
  DPRINT("=");
  ReplicationTimeoutStop();
  glearnNodeInfo.bStatus = ADD_NODE_STATUS_PROTOCOL_DONE;
  glearnNodeInfo.bSource = otherController;
  glearnNodeInfo.pCmd = NULL;
  glearnNodeInfo.bLen = 0;
  zcbp_CtrlCompleteFunction(&glearnNodeInfo);
  return false;
}


/*===========================   TransferExpired   ============================
**
**    No node info frame was received after having send a transfer
**    presentation, try sending it again.
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
void
ZCB_TransferExpired( __attribute__((unused)) SSwTimer* pTimer )
{
  if (NEW_CTRL_RECEIVER_BIT)
  {
    TimerStop(pTimer);
  }
  else if (assign_ID.assignIdState == ASSIGN_IDLE)
  {
    /* Send out transfer presentation frame */
    /* TO#1499 fix - give transfer presentation its own buffer as its transmitted via a timer */
    transferPresFrame.cmdClass = ZWAVE_CMD_CLASS_PROTOCOL;
    transferPresFrame.cmd = ZWAVE_CMD_TRANSFER_PRESENTATION;
    /* TODO - Should we enable this only if NETWORK_WIDE_MODE_INCLUDE is set */
    transferPresFrame.option = TRANSFER_PRESENTATION_OPTION_UNIQ_HOMEID;
    /* TO#7022 partial fix */
    switch (g_learnNodeState)
    {
      case LEARN_NODE_STATE_DELETE:
        {
          /* We are doing exclusion therefor NOT inclusion */
          transferPresFrame.option |= TRANSFER_PRESENTATION_OPTION_NOT_INCLUSION;
        }
        break;

      case LEARN_NODE_STATE_UPDATE:
      case LEARN_NODE_STATE_NEW:
        {
          /* We are doing inclusion therefor NOT exclusion */
          transferPresFrame.option |= TRANSFER_PRESENTATION_OPTION_NOT_EXCLUSION;
        }

      default:
        break;
    }

#ifndef TEST_NO_DIRECT_INCLUDE
    /* Send frame as broadcast */
    static const STransmitCallback TxCallback = { .pCallback = 0, .Context = 0 };
    EnQueueSingleData(false, g_nodeID, NODE_BROADCAST, (uint8_t *)&transferPresFrame,
                      sizeof(TRANSFER_PRESENTATION_FRAME),
                      !transferPresentationHighPower ? TRANSMIT_OPTION_LOW_POWER : 0,
                      0, // 0ms for tx-delay (any value)
                      ZPAL_RADIO_TX_POWER_DEFAULT,
                      &TxCallback);
#endif
  }
}

#ifdef ZW_CONTROLLER_TEST_LIB
void
ExclusionDoneLR(
  node_id_t bSource)
{
  DPRINTF("S%d ", rxStatus);
  otherController = bSource;

  /* Signal to application that we got the new home ID */
  glearnNodeInfo.bStatus = ADD_NODE_STATUS_LEARN_READY;
  glearnNodeInfo.bSource = g_nodeID;
  glearnNodeInfo.pCmd = NULL;
  glearnNodeInfo.bLen = 0;
  zcbp_CtrlCompleteFunction(&glearnNodeInfo);
}

#endif

/*===========================   ReplicationStart   ===========================
**
**    Set replication state so its ready to receive TRANSFER_INFO frames
**    and note the source ID of the controller including this controller
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
void
ReplicationStart(
  uint8_t rxStatus,      /* IN  Frame header info */
  uint8_t bSource)
{
  DPRINTF("S%d ", rxStatus);
  otherController = bSource;
  /* Extract inclusion power from rxStatus */
  transferPresentationHighPower = ((rxStatus & RECEIVE_STATUS_LOW_POWER) == 0);
  replTXOptions = TRANSMIT_OPTION_ACK | (!transferPresentationHighPower ? TRANSMIT_OPTION_LOW_POWER : 0);
  /* Are we started allready */
  if (GET_NEW_CTRL_STATE < NEW_CTRL_RECEIVE_INFO)
  {
    CHANGE_NEW_CTRL_STATE(NEW_CTRL_RECEIVE_INFO);
  }
  StartReplicationReceiveTimer();
  /* TO#2647 fix - Application has now been notified regarding replication initiated */
  appReplicationNotNotified = false;
  /* Signal to application that we got the new home ID */
  glearnNodeInfo.bStatus = ADD_NODE_STATUS_LEARN_READY;
  glearnNodeInfo.bSource = g_nodeID;
  glearnNodeInfo.pCmd = NULL;
  glearnNodeInfo.bLen = 0;
  zcbp_CtrlCompleteFunction(&glearnNodeInfo);
}


/*========================   PresentationReceived   ==========================
**
**    Transfer presentation frame was received, set controller in learn
**    mode and send a node info frame.
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
void
PresentationReceived( /* RET Nothing */
  uint8_t rxStatus,      /* IN  Frame header info */
  uint8_t bSource)       /* IN  Source id on controller that sent */
{
  /* Only answer if were in replication mode */
  if (GET_NEW_CTRL_STATE == NEW_CTRL_STOP)
  {
    return;
  }
  /* Extract inclusion power from rxStatus */
	transferPresentationHighPower = ((rxStatus & RECEIVE_STATUS_LOW_POWER) == 0);
	replTXOptions = TRANSMIT_OPTION_ACK | (!transferPresentationHighPower ? TRANSMIT_OPTION_LOW_POWER : 0);
  /* Save node ID of master controller */
  otherController = bSource;
  /* Are we started allready? */
  if (GET_NEW_CTRL_STATE < NEW_CTRL_RECEIVE_INFO)
  {
    /* Now we are ready to receive transfer info/range */
    CHANGE_NEW_CTRL_STATE(NEW_CTRL_RECEIVE_INFO);
    /* Reset the nodetable before continuing */
    ZW_ClearTables();
  }
  /* TO#2374 Fix */
  /* Do we need to spoof */
  if (rxStatus & RECEIVE_STATUS_FOREIGN_HOMEID)
  {
    /* Let us spoof if needed, because */
    DPRINT("$");
    spoof = true;
  }
  /* TO#2019 partial fix */
  else
  {
    DPRINT("{");
    spoof = false;
    ControllerStorageGetNetworkIds(ZW_HomeIDGet(), &g_nodeID);
  }
  /* TO#2875 fix - Do not use LWR/Response Route or Routing */
  static const STransmitCallback TxCallback = { .pCallback = NULL, .Context = 0 };
  ZW_SendNodeInformation(otherController,
                         (!transferPresentationHighPower ?
                          TRANSMIT_OPTION_LOW_POWER | TRANSMIT_OPTION_NO_ROUTE :
                          TRANSMIT_OPTION_NO_ROUTE),
                         &TxCallback);
}


/*=====================   TransferCmdCompleteReceived   ======================
**
**    Command complete frame was received, Send next information
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
void
TransferCmdCompleteReceived( void ) /* RET Nothing */
{
  replRetry = 0;
  while (((GET_NEW_CTRL_STATE == NEW_CTRL_SEND_NODES) ||
          (GET_NEW_CTRL_STATE == NEW_CTRL_SEND_RANGE)) && TransferInfo()) ;
}


/*======================   TransferNodeInfoReceived   ========================
**
**    Transfer Node Info frame was received, save the information and send
**    a complete frame
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
void
TransferNodeInfoReceived(
  uint8_t bCmdLength,        /* IN Command payload length */
  uint8_t *pCmd)
{
  DPRINTF("W%d ", replicationStatus);
  /* Replication process is still alive */
  StartReplicationReceiveTimer();
  if (((TRANSFER_NODE_INFO_FRAME*)pCmd)->nodeID > ZW_MAX_NODES)
  {
    ZW_ReplicationReceiveComplete();
  }
  else if (GET_NEW_CTRL_STATE == NEW_CTRL_RECEIVE_INFO)
  {
    /* copy sequence number that should be returned with command complete */
    sequenceNumber = ((TRANSFER_NODE_INFO_FRAME*)pCmd)->seqNo;
    /* Store node info*/
    /* First store until nodeType */
    EX_NVM_NODEINFO tNodeInfo = { 0 };
    memcpy((uint8_t *)&tNodeInfo.capability, &((TRANSFER_NODE_INFO_FRAME*)pCmd)->nodeInfo.capability, EX_NVM_NODE_INFO_SIZE - 2);

    uint32_t iNodeIndex;
    /* Does it contain Specific Device Type */
    if ((((TRANSFER_NODE_INFO_FRAME*)pCmd)->nodeInfo.security & ZWAVE_NODEINFO_SPECIFIC_DEVICE_TYPE) &&
        ((bCmdLength - offsetof(TRANSFER_NODE_INFO_FRAME, nodeInfo)) >= EX_NVM_NODE_INFO_SIZE))
    {
      /* Specific Device Type are included */
      if (((TRANSFER_NODE_INFO_FRAME*)pCmd)->nodeInfo.security & ZWAVE_NODEINFO_CONTROLLER_NODE)
      {
        /* It is a Controller node */
        memcpy((uint8_t *)&tNodeInfo.generic, &((TRANSFER_NODE_INFO_FRAME*)pCmd)->nodeInfo.nodeType.generic, 2);
        iNodeIndex = (((TRANSFER_NODE_INFO_FRAME*)pCmd)->nodeID);
      }
      else
      {
        /* It is a Slave node */
        memcpy((uint8_t *)&tNodeInfo.generic, &((TRANSFER_NODE_INFO_SLAVE_FRAME*)pCmd)->nodeInfo.nodeType.generic, 2);
        iNodeIndex = ((TRANSFER_NODE_INFO_SLAVE_FRAME*)pCmd)->nodeID;
      }
    }
    else
    {
      /* Old device - only Generic Device Type is known, set Specific Device Type to ZERO */
      tNodeInfo.generic = ((TRANSFER_NODE_INFO_OLD_FRAME*)pCmd)->nodeInfo.nodeType;
      /* Set Specific to ZERO */
      tNodeInfo.specific = 0;
      iNodeIndex = ((TRANSFER_NODE_INFO_OLD_FRAME*)pCmd)->nodeID;
    }
    // Save node info
    CtrlStorageSetNodeInfo(iNodeIndex,&tNodeInfo);

    ZW_ReplicationReceiveComplete();
    g_learnMode = false;

    g_learnModeClassic = false;

    /* TO#2647 fix - Has Application been notified regarding replication initiated */
    if (appReplicationNotNotified)
    {
      /* TO#2647 fix - Application has now been notified regarding replication initiated */
      appReplicationNotNotified = false;
      /* Signal to application that replication started. */
      glearnNodeInfo.bStatus = ADD_NODE_STATUS_LEARN_READY;
      glearnNodeInfo.bSource = g_nodeID;
      glearnNodeInfo.pCmd = NULL;
      glearnNodeInfo.bLen = 0;
      zcbp_CtrlCompleteFunction(&glearnNodeInfo);
    }
  }
}


/*======================   TransferRangeInfoReceived   =======================
**
**    Transfer Range Info frame was received, save the information and send
**    a complete frame
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
void
TransferRangeInfoReceived(
  uint8_t bCmdLength,
  uint8_t *pCmd)
{
  (void)bCmdLength;

  if (GET_NEW_CTRL_STATE == NEW_CTRL_RECEIVE_INFO)
  {
    /* Now we consider this controller as included */
    controllerIncluded = true;
    /* Kick inclusion timer */
    StartReplicationReceiveTimer();
    /* copy sequence number that should be returned with command complete */
    sequenceNumber = ((TRANSFER_RANGE_INFO_FRAME*)pCmd)->seqNo;
    /* Add range info to routing table in EEPROM */
    NODE_MASK_TYPE nodeRangeInfo = { 0 };
    uint32_t iIndex = ((TRANSFER_RANGE_INFO_FRAME*)pCmd)->nodeID;
    uint32_t iLength = ((TRANSFER_RANGE_INFO_FRAME*)pCmd)->numMaskBytes;
    memcpy((uint8_t * )&nodeRangeInfo, ((TRANSFER_RANGE_INFO_FRAME*)pCmd)->maskBytes, iLength);
    CtrlStorageSetRoutingInfo(iIndex, &nodeRangeInfo, true);
  }
  ZW_ReplicationReceiveComplete();
}


/*=============================== SendSUCID  ===================================
** Send SUC ID
**----------------------------------------------------------------------------*/
uint8_t
SendSUCID(
  uint8_t node,
  uint8_t txOption,
  const STransmitCallback* pTxCallback)
{
  assignIdBuf.SUCNodeID.cmdClass = ZWAVE_CMD_CLASS_PROTOCOL;
  assignIdBuf.SUCNodeID.cmd      = ZWAVE_CMD_SUC_NODE_ID;
  assignIdBuf.SUCNodeID.nodeID   = staticControllerNodeID;
  assignIdBuf.SUCNodeID.SUCcapabilities = isNodeIDServerPresent() ? ID_SERVER_RUNNING_BIT : 0;

  if (!EnQueueSingleData(false, g_nodeID, node, (uint8_t *)&assignIdBuf, sizeof(SUC_NODE_ID_FRAME), txOption, 0, // 0ms for tx-delay (any value)
                         ZPAL_RADIO_TX_POWER_DEFAULT, pTxCallback))
  {
    ZW_TransmitCallbackInvoke(pTxCallback, TRANSMIT_COMPLETE_FAIL, NULL);
    return false;
  }
  return true;
}


/*========================   ReplicationDummyFunc   =========================
**    Dummy function used to ensure that there is always an application
**    callback function
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
void
ZCB_ReplicationDummyFunc(
  LEARN_INFO_T *learnNodeInfo)
{
  (void)learnNodeInfo;
  DPRINT("!");
}


/*=======================   StartPresentationTimer   =========================
**    Function description
**      Start timer to keep the sending of "Transfer Presentation" frame
**      alive so that one is send every every 2. sec, until either the
**      Application cancels the learn process or a node gets included
**      Called as a callback for the transmission of the Explore Set NWI mode.
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
void
StartPresentationTimer(void)
{
  /* TO3367 fix - Serial API Controller doesn't send Transfer Presentation */

/* TO#02691 - fix. Removed explore functionality in controller_static_single because of conflicting use of TRANSMIT_FRAME_OPTION_EXPLORE */
  bNetworkWideInclusionReady = true;

  if (addCtrlNodes)
  {
    /* Start timeout for retransmission of Transfer Presentation */
    TimerSetCallback(&PresentationTimer, ZCB_TransferExpired);
    TimerStart(&PresentationTimer, TRANSFER_PRESENTATION_TIMEOUT);
  }
}

/*==========================   StartReplication   ============================
**    Replication start
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
void
StartReplication(
  node_id_t bNodeID)
{
  if (zcbp_CtrlCompleteFunction == NULL)
  {
    zcbp_CtrlCompleteFunction = ZCB_ReplicationDummyFunc;
  }
  replTXOptions = TRANSMIT_OPTION_ACK | (transferPresentationHighPower ? TRANSMIT_OPTION_AUTO_ROUTE : TRANSMIT_OPTION_LOW_POWER);
  addCtrlNodes = true;
  /* Save node ID controller to replicate to */
  otherController = bNodeID;
  /* Change state */
  CHANGE_NEW_CTRL_STATE(NEW_CTRL_SEND_NODES);

  /* Start sending node information to other controller */
  bNextNode = 0;
  replRetry = 0;
  while (TransferInfo());
}


/*=========================   LearnNodeCallback   ==========================
**    Function description
**
**    Callback function used to handle status from ZW_SetLearnNodeState
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
void
ZCB_LearnNodeCallback( /* RET Nothing */
  LEARN_INFO_T *learnNodeInfo) /* IN Learn status and information*/
{
  /* Cancel timer that retransmits the transfer presentation frame */
  uint8_t bStatus = learnNodeInfo->bStatus;
  uint8_t bSource = learnNodeInfo->bSource;

  DPRINTF("ZCB_LearnNodeCallback H%d \n", bStatus);
  /* Stop presentation timer if learn is started */
  if (bStatus != LEARN_STATE_LEARN_READY)
  {
    StopPresentationTimer();
  }
  else
  {
    /* Send learn ready to application */
    learnNodeInfo->bStatus = ADD_NODE_STATUS_LEARN_READY;

/* TO#02691 - fix. Removed explore functionality in controller_static_single because of conflicting use of TRANSMIT_FRAME_OPTION_EXPLORE */
    inclusionHomeIDActive = false;
    static const STransmitCallback TxCallback = { .pCallback = 0, .Context = 0 };
    /* Only ask repeaters to repeat Explore Inclusion Requests if we are doing Network Wide Inclusion */
    /* If not doing Network Wide Inclusion then ask them to stop repeating Explore Inclusion Requests */
    /* TO#3341 fix */
    ExploreTransmitSetNWIMode((bNetworkWideInclusion == NETWORK_WIDE_MODE_INCLUDE) ||
                              (bNetworkWideInclusion == NETWORK_WIDE_SMART_START) ||
                              (bNetworkWideInclusion == NETWORK_WIDE_SMART_START_NWI)  ?
                              NWI_REPEAT : NWI_IDLE, &TxCallback);

    /* TO#3367 fix update */
    StartPresentationTimer();

    /* Tell Application we started */
    zcbp_CtrlCompleteFunction(learnNodeInfo);
  }

  if (bStatus == LEARN_STATE_FIND_NEIGBORS_DONE)
  {
    learnNodeInfo->bStatus = ADD_NODE_STATUS_FIND_NEIGHBORS_DONE;
    zcbp_CtrlCompleteFunction(learnNodeInfo);
  }
  else if ((bStatus == LEARN_STATE_FAIL) || (bStatus == LEARN_STATE_NO_SERVER))
  {
    /* Assign ID failed. LearnnodeState off and return to Application */
    TransferDoneCallback(NEW_CONTROLLER_FAILED);
  }
  else if (bStatus == LEARN_STATE_NODE_FOUND)
  {
    /* Send learn ready to application */
    learnNodeInfo->bStatus = ADD_NODE_STATUS_NODE_FOUND;
    zcbp_CtrlCompleteFunction(learnNodeInfo);
  }
  else if (bStatus == LEARN_STATE_ROUTING_PENDING)
  {
    /* Set status to the type of the node */
    /* Bad code ??? */
    if ((*(learnNodeInfo->pCmd - 2) & ZWAVE_NODEINFO_CONTROLLER_NODE))
    {
      learnNodeInfo->bStatus = ADD_NODE_STATUS_ADDING_CONTROLLER;
      addSlaveNodes = false;
      addCtrlNodes = true;
      /* Set the bit that indicates that we should send transfer end if
         learn is stopped */
      SET_SEND_END_BIT;
      /* If deleting controller then send transfer end */
      if (GET_NEW_CTRL_STATE == NEW_CTRL_DELETE)
      {
        /* TO#2803 fix - Turn off learnNodeState - to disallow new nodeinformation frames to be accepted */
        ZW_SetLearnNodeState(LEARN_NODE_STATE_OFF, NULL);
        zcbp_CtrlCompleteFunction(learnNodeInfo);
        learnNodeInfo->bStatus = LEARN_MODE_DONE;
        ZCB_SendTransferEnd(0, TRANSMIT_COMPLETE_OK, NULL);
        /* TO#3750 fix - If we are "Removing a Controller" then the REMOVE_NODE_STATUS_DONE status */
        /* is delivered to App after "Transfer End" has been transmitted */
        return;
      }
    }
    else
    {
      learnNodeInfo->bStatus = ADD_NODE_STATUS_ADDING_SLAVE;
      /* TO#1780 - Callback from ZW_AddNodeToNetwork() had wrong node id for slaves */
      otherController = learnNodeInfo->bSource;
      addCtrlNodes = false;
      DPRINTF("%d", GET_NEW_CTRL_STATE);

      /* if removing a slave node then delete is finished now */
      if (GET_NEW_CTRL_STATE == NEW_CTRL_DELETE)
      {
        INIT_NEW_CTRL_STATE(NEW_CTRL_STOP);
        /* We are now done */
        ZW_SetLearnNodeState(LEARN_NODE_STATE_OFF, NULL);
        /* TO#1425 fix - callback status REMOVE_NODE_STATUS_REMOVING_SLAVE */
        /*               must be send according to spec */
        zcbp_CtrlCompleteFunction(learnNodeInfo);
        learnNodeInfo->bStatus = LEARN_MODE_DONE;
        /* REMOVE_NODE_STATUS_DONE is delivered to the App by the following */
        /* CtrlCompleteFunction callback call */
      }
      else
      {
        UpdateMostUsedNodes(true, learnNodeInfo->bSource);
      }
    }
    /* Make callback to application */
    zcbp_CtrlCompleteFunction(learnNodeInfo);
  }
  else if (bStatus == NEW_CONTROLLER_DONE)
  {
    DPRINTF("c%d", learnNodeInfo->bSource);
    /* Replication was successfull. */

    INIT_NEW_CTRL_STATE(NEW_CTRL_STOP);
    ZW_SetLearnNodeState(LEARN_NODE_STATE_OFF, NULL);
    assign_ID.assignIdState = ASSIGN_IDLE;
    learnNodeInfo->bStatus = LEARN_MODE_DONE;
    zcbp_CtrlCompleteFunction(learnNodeInfo);
  }
  else if (bStatus == NEW_CONTROLLER_FAILED)
  {
    INIT_NEW_CTRL_STATE(NEW_CTRL_STOP);
    learnNodeInfo->bStatus = ADD_NODE_STATUS_FAILED;
    zcbp_CtrlCompleteFunction(learnNodeInfo);
  }
  else if (bStatus == LEARN_STATE_DONE)
  {
    DPRINTF("%d", GET_NEW_CTRL_STATE);
    if (GET_NEW_CTRL_STATE == NEW_CTRL_DELETE)
    {
      /* We have deleted a controller */
      ZW_SetLearnNodeState(LEARN_NODE_STATE_OFF, NULL);
      learnNodeInfo->bStatus = LEARN_MODE_DONE;
      zcbp_CtrlCompleteFunction(learnNodeInfo);
    }
    else  /* ID was assigned correctly */
    {
      DPRINTF(",%d %d ", GET_NEW_CTRL_STATE, addCtrlNodes);
      if (addCtrlNodes)
      {
        if (GET_NEW_CTRL_STATE == NEW_CTRL_SEND)
        {
          StartReplication(bSource);
          DPRINT("+");
        }
        else
        {
          learnNodeInfo->bStatus = ADD_NODE_STATUS_ADDING_CONTROLLER;

          if (GET_NEW_CTRL_STATE == NEW_CTRL_CREATED)
          {
            INIT_NEW_CTRL_STATE(NEW_CTRL_STOP);
            ZW_SetLearnNodeState(LEARN_NODE_STATE_OFF, NULL);
            learnNodeInfo->bStatus = LEARN_MODE_DONE;
            zcbp_CtrlCompleteFunction(learnNodeInfo);
            /* We are completly done with replication */
          }
          else
          {
            DPRINTF("-%d ", learnNodeInfo->bStatus);
            zcbp_CtrlCompleteFunction(learnNodeInfo);
          }
        }
      }
      else  /* Add node is complete */
      {
        INIT_NEW_CTRL_STATE(NEW_CTRL_STOP);
        ZW_SetLearnNodeState(LEARN_NODE_STATE_OFF, NULL);
        learnNodeInfo->bStatus = ADD_NODE_STATUS_PROTOCOL_DONE;
        zcbp_CtrlCompleteFunction(learnNodeInfo);
      }
    }
  }
}


static void ReplicationNewCtrlSend(void)
{
  if (DO_SEND_TRANSFER_END)
  {
    /* Delay the sending of the SUC ID or Transfer End */
    ReplicationTimeoutStop();
    /* Tell otherController about SUC - even if none present */
    if (otherController)
    {
      static const STransmitCallback TxCallback = { .pCallback = ZCB_NewControllerEnd, .Context = 0 };
      SendSUCID(otherController,
                replTXOptions,
                &TxCallback);
    }
    else
    {
      ZCB_NewControllerEnd(0, TRANSMIT_COMPLETE_OK, NULL);
    }
  }
  else
  {
    /* Stop learn mode unconditionally */
    TransferDoneCallback(NEW_CONTROLLER_DONE);
  }
}

static void ReplicationNewCtrlStop(void)
{
  ZW_SetLearnNodeState(LEARN_NODE_STATE_OFF, NULL);

  if (zcbp_CtrlCompleteFunction)
  {
    glearnNodeInfo.bStatus = ADD_NODE_STATUS_PROTOCOL_DONE;
    glearnNodeInfo.bSource = 0;
    glearnNodeInfo.pCmd = NULL;
    glearnNodeInfo.bLen = 0;
    zcbp_CtrlCompleteFunction(&glearnNodeInfo);
  }
}


/*==========================   ReplicationInit   ===========================
**    Function description
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
void
ReplicationInit(
  uint8_t bMode,
  void ( *completedFunc)(LEARN_INFO_T *)) /* IN Callback function */
{
  /* Initialize replication variables */
  bNextNode = 0;
  sequenceNumber = 0;
  zcbp_CtrlCompleteFunction = completedFunc;
  DPRINT("ReplicationInit st\n");
  if (zcbp_CtrlCompleteFunction == NULL)
  {
    zcbp_CtrlCompleteFunction = ZCB_ReplicationDummyFunc;
  }

  // Init timers
  ZwTimerRegister(&PresentationTimer, true, NULL);
  ZwTimerRegister(&replicationTimer, true, NULL);

  /* Dont clear on stop because we need to know so we can */
  /* send the correct transfer end frame */
  if (bMode != STOP)
  {
#if defined(ZW_CONTROLLER_STATIC)
    doChangeToSecondary = false;
#endif
    newPrimaryReplication = false;
  }

  DPRINTF("R%d-%d \n", bMode, replicationStatus);

  /* TO#3258 fix update - Do not Cache any Route while locked. */
  /* The includie must call ZW_LockRoute before receiving AssignID */

  /* Stop replication */
  if (bMode == STOP)
  {
    /* We are receiving, just stop */
    if (NEW_CTRL_RECEIVER_BIT)
    {
      /* Just stop replication and learn */
      INIT_NEW_CTRL_STATE(NEW_CTRL_STOP);
      ZW_SetLearnMode(false, NULL);
    }
    else /* We are transmitting */
    {
      /* If send is complete then send SUC ID if we are adding a controller */
      if (GET_NEW_CTRL_STATE == NEW_CTRL_SEND)
      {
        ReplicationNewCtrlSend();
      }
      else if (GET_NEW_CTRL_STATE == NEW_CTRL_DELETE)
      {
        TransferDoneCallback(NEW_CONTROLLER_DONE);
      }
      else if (GET_NEW_CTRL_STATE == NEW_CTRL_STOP)
      {
        ReplicationNewCtrlStop();
      }
    }

    /* Now the cached foreign HomeID are not valid anymore */
    bNetworkWideInclusion = NETWORK_WIDE_MODE_IDLE;

    /* Unlock Last Working Route for purging */
    ZW_LockRoute(false);
  }
  else if (bMode == STOP_FAILED)
  {
    /* If were receiving clear any node information */
    if (NEW_CTRL_RECEIVER_BIT)
    {
      ZW_ClearTables();
    }
    if (GET_NEW_CTRL_STATE != NEW_CTRL_STOP)
    {
      ZCB_SendTransferEnd(0, TRANSMIT_COMPLETE_NO_ACK, NULL);
      TransferDoneCallback(NEW_CONTROLLER_FAILED);
    }

    /* Now the cached foreign HomeID are not valid anymore */
    bNetworkWideInclusion = NETWORK_WIDE_MODE_IDLE;

    /* Unlock Last Working Route for purging */
    ZW_LockRoute(false);
  }

  /* Initialize replication state */
  INIT_NEW_CTRL_STATE(NEW_CTRL_STOP);

  /* TO#2647 fix - Just in case. */
  appReplicationNotNotified = false;
  if (bMode == UPDATE_NODE)
  {
    bReplicationDontDeleteOnfailure = false;
    /* Start learn mode */
    ZW_SetLearnNodeState(LEARN_NODE_STATE_UPDATE, ZCB_LearnNodeCallback);
  }
  else if (bMode == NEW_NODE)
  {
    /* No new controller included yet... */
    controllerIncluded = false;
    bReplicationDontDeleteOnfailure = false;
    /* Initialize replication state machine */
    INIT_NEW_CTRL_STATE(NEW_CTRL_SEND);

    /* Start learn mode */
    ZW_SetLearnNodeState(LEARN_NODE_STATE_NEW, ZCB_LearnNodeCallback);
  }
  else if (bMode == DELETE_NODE)
  {
    /* Initialize replication mode */
    INIT_NEW_CTRL_STATE(NEW_CTRL_DELETE);

    /* Set controller in learn node state */
    ZW_SetLearnNodeState(LEARN_NODE_STATE_DELETE, ZCB_LearnNodeCallback);
  }
  else if (bMode == RECEIVE)
  {
    /* TO#2647 fix - Flag to tell if Application has not been notified regarding replication started */
    appReplicationNotNotified = true;
    /* We are not included yet... */
    controllerIncluded = false;
    bReplicationDontDeleteOnfailure = false;
    /*It is allowed to receive mode regardless of
    (SIS/SUC/PRIMARY or secondary flags.. Fix TO#905)*/
    INIT_NEW_CTRL_STATE(NEW_CTRL_RECEIVE);
    SET_RECEIVER_BIT;
//    ZW_SetRFReceiveMode(true);
  }
}


/****************************************************************************/
/*                           EXPORTED FUNCTIONS                             */
/****************************************************************************/

/*===================   ReplicationReceiveComplete   =========================
**
**    Send command complete frame to master controller. Indicates that
**    the command received was executed.
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
void
ZW_ReplicationReceiveComplete( void )
{
  assignIdBuf.TransferCmdCompleteFrame.cmdClass = ZWAVE_CMD_CLASS_PROTOCOL;
  assignIdBuf.TransferCmdCompleteFrame.cmd = ZWAVE_CMD_CMD_COMPLETE;
  assignIdBuf.TransferCmdCompleteFrame.seqNo = sequenceNumber;
  /* Send Command Complete frame to master controller */
  static const STransmitCallback TxCallback = { .pCallback = NULL, .Context = 0 };
  EnQueueSingleData(false, g_nodeID, otherController, (uint8_t *)&assignIdBuf,
                    sizeof(COMMAND_COMPLETE_FRAME),
                    replTXOptions,
                    0, // 0ms for tx-delay (any value)
                    ZPAL_RADIO_TX_POWER_DEFAULT,
                    &TxCallback);
}


/*============================   ReplicationSend   ===========================
**    Function description
**      Sends the payload to the receiver, which should respond with
**      command completed, with matching sequenceNumber.
**    Side effects:
**
**--------------------------------------------------------------------------*/
uint8_t                                    /*RET  false if transmitter busy      */
ReplicationSend(
  uint8_t      destNodeID,                 /*IN  Destination node ID. Only single cast allowed*/
  uint8_t     *pData,                      /*IN  Data buffer pointer           */
  uint8_t      dataLength,                 /*IN  Data buffer length            */
  TxOptions_t  txOptions,                  /*IN  Transmit option flags         */
  const STransmitCallback* pCompletedFunc) /*IN  Transmit completed call back function  */
{
  if (!replRetry)
  {
    sequenceNumber++;
  }
  ReplCompleteFunction = *pCompletedFunc;
  pData[2] = sequenceNumber;
  SET_REPLICATION_SEND_ONGOING;/* Indicate that we are waiting for the the node/range info transfer to be done */
  /* Send data to receiver */
  static const STransmitCallback TxCallback = { .pCallback = ZCB_ReplicationSendCallback, .Context = 0 };
  return EnQueueSingleData(false, g_nodeID, destNodeID, pData, dataLength, txOptions, 0, // 0ms for tx-delay (any value)
                           ZPAL_RADIO_TX_POWER_DEFAULT, &TxCallback);
}
