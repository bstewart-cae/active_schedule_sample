// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file ZW_slave.c
 * @copyright 2022 Silicon Laboratories Inc.
 *
 * @brief Z-Wave Slave Application Interface.
 */
#include "ZW_lib_defines.h"

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <ZW_main.h>
#include "ZW_home_id_generator.h"
#include <ZW_slave.h>

#include <ZW_basis.h>
#include <ZW_nvm.h>
#include <ZW_timer.h>
#include <SwTimer.h>
#include <ZW_MAC.h>
#include <ZW_routing_all.h>
#include <zpal_radio_utils.h>

#include "ZW_protocol.h"
#include "ZW_protocol_cmd_handler.h"
#include "ZW_protocol_interface.h"
#include "ZW_CCList.h"

#include <ZW_explore.h>
#include <NodeMask.h>

#ifdef ZW_SECURITY_PROTOCOL
#include <ZW_Security_Scheme2.h>
#include <s2_inclusion.h>
#include <ZW_secure_learn_support.h>
#include <ZW_security_api.h>
#include <ZW_s2_inclusion_glue.h>
#endif

#include <ZW_noise_detect.h>
#ifdef USE_TRANSPORT_SERVICE
#include <transport_service2_external.h>
#endif
#ifdef ZW_SECURITY_PROTOCOL
#include <ZW_keystore.h>
#endif

//#define DEBUGPRINT
#include "DebugPrint.h"
#include "Assert.h"
#include <string.h>

#include <ZW_slave_network_info_storage.h>
#include <ZW_receivefilter_learnmode.h>
#include <zpal_power_manager.h>

#include <ZW_network_management.h>
#include <zpal_radio.h>

/****************************************************************************/
/*                      PRIVATE TYPES and DEFINITIONS                       */
/****************************************************************************/

typedef struct _SUC_UPDATE_
{
  uint8_t updateState;
  SSwTimer TimeoutTimer;
} SUC_UPDATE;

typedef union _CMD_BUFF_
{
  COMMAND_COMPLETE_FRAME  cmdCompleteBuf;
  NOP_POWER_FRAME         nopPowerFrame;
} CMD_BUFF;

#define ASSIGN_TIMEOUT_2_SEC    (2000 / TRANSFER_MIN_FRAME_WAIT_TIME_MS)

#define SUC_REQUEST_UPDATE_TIMEOUT 50800 /* 50.8 sec */

#define SUC_IDLE                            1
#define SUC_UPDATE_STATE_WAITING            2
#define SUC_LOST_STATE_WAITING              3
#define SUC_WAITING_FOR_NEIGHBOR_REQUEST    4
#define SUC_LOST_COMPLETED                  5

/* Define for making easy and consistent static callback definitions */
#define STATIC_VOID_CALLBACKFUNC(completedFunc)  static VOID_CALLBACKFUNC(completedFunc)

/****************************************************************************/
/*                              PRIVATE DATA                                */
/****************************************************************************/
static const SAppNodeInfo_t * m_pAppNodeInfo;

static zpal_pm_handle_t learn_power_lock;

static NODE_RANGE  nodeRange = { 0 };

static RANGEINFO_FRAME  nodeNormalRangeFrame = { 0 };

/* TO#3511 fix - allocate a noderange buffer each for FLiRS nodeneighbor and for listening nodeneighbor search */
static RANGEINFO_FRAME  nodeFLiRSRangeFrame = { 0 };
static RANGEINFO_FRAME  *pNodeRangeFrame;

/*
 * The wakeuptime to be used if nodes are zensor nodes.
 *
 * If this byte is missing or is ZERO then the rangeinfo is for a standard findneighbors sequence
 *  - if zensorWakeupTime is ZWAVE_ZENSOR_WAKEUPTIME_1000MS then the zensors has a wakeuptime at 1000ms,
 *  - if zensorWakeupTime is ZWAVE_ZENSOR_WAKEUPTIME_250MS then the wakeup time for the zensors is 250ms
 */
static uint8_t  zensorWakeupTime;

static CMD_BUFF cmdBuffer = { 0 };

static NODEINFO_FRAME nodeInfoFrame = { 0 };

static uint8_t bNeighborDiscoverySpeed;

// FIXME Only used once, and only to delay call to ZCB_NeighborCompleteFailCall
static SSwTimer NeighborCompleteDelayTimer = { 0 };

static ELearnStatus assignState = ELEARNSTATUS_ASSIGN_COMPLETE;
static SSwTimer assignTimer = { 0 };

/* NodeID of neighbor node currently being NOP'ed */
static uint8_t bCurrentRangeCheckNodeID = 0;

static sec_learn_complete_t learnNodeFunc;  /* Node learn call back function. */

/* Application level callback for learn mode. Called when
 * secure inclusion state machine is done. */
static sec_learn_complete_t secureLearnAppCbFunc;

#ifdef USE_TRANSPORT_SERVICE
/* These structs are used for parameter passing into the Transport
 * service module. */
static ts_param_t sTsParam = { 0 };

/* Transport Service rxOpt for passing srcNode to client layer */
static RECEIVE_OPTIONS_TYPE ts_rxOpt;
#endif

static uint8_t ZW_RouteDestinations[ZW_MAX_CACHED_RETURN_ROUTE_DESTINATIONS];

static uint8_t curNodeID = 0;
static STransmitCallback routeUpdateDone = { 0 };
static SUC_UPDATE  SUC_Update = { 0 };

#ifdef ZW_SELF_HEAL
static uint8_t helpNodeID;   /* Needed to remember which node asked for help */
#endif /*ZW_SELF_HEAL*/
static uint8_t oldSpeed = RF_SPEED_9_6K;

/* TO#1964 partial possible fix. */
static bool forceUse40K = false;

static struct sTxQueueEmptyEvent sTxQueueEmptyEvent_phyChange = { 0 };

static zpal_radio_region_t m_initRegion = REGION_UNDEFINED;


/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/

bool g_findInProgress = false; // Global variable

bool g_learnModeClassic;
bool g_learnMode = false;  /* true when the "Assign ID's" command is accepted */
bool g_learnModeDsk;

uint8_t g_Dsk[16]; // Global variable

uint8_t g_sucNodeID = 0; // Global variable
SSyncEventArg1 g_SlaveSucNodeIdChanged = { .uFunctor.pFunction = 0 };  /* Callback activated on change to SucNodeId */

SSyncEventArg2 g_SlaveHomeIdChanged = { .uFunctor.pFunction = 0 };  /* Callback activated on change to Home ID / Node ID */
                                                                    /* Arg1 is Home ID, Arg2 is Node ID */

/****************************************************************************/
/*                            PRIVATE FUNCTIONS                             */
/****************************************************************************/

static void FindNeighbors(uint8_t nextNode);
static void ZCB_AssignTimeout(SSwTimer* pTimer);
static void HomeIdUpdate(uint8_t HomeIdArg[HOMEID_LENGTH], node_id_t NodeId);
static void LearnModeDone(uint8_t bStatus, node_id_t nodeID);
static void ZCB_AssignComplete(ZW_Void_Function_t Context, uint8_t txStatus, TX_STATUS_TYPE *txStatusReport);

/*--------------------------------------------------------------------
Automatic update functions start
--------------------------------------------------------------------*/

/*============================   NetWorkTimerStop   ======================
**    Function description
**      Stops the specified timer handle and updates the handle pointer
**    Side effects:
**
**--------------------------------------------------------------------------*/
static void
NetWorkTimerStop(SSwTimer *pTimer) /*IN/OUT handle of timer*/
{
  TimerStop(pTimer);
}


/*-----------------------------    ResetSUC           --------------------------------
 rest the variables used by the SUC scheme
--------------------------------------------------------------------------------------*/
void
ResetSUC(void)
{
  NetWorkTimerStop(&SUC_Update.TimeoutTimer);
  SUC_Update.updateState = SUC_IDLE;
}


/*============================   NetworkTimerStart ===========================
**    Function description
**      Starts the Network update timeout function
**    Side effects:
**
**--------------------------------------------------------------------------*/
static void
NetWorkTimerStart(                      /*RET  nothing       */
  SSwTimer *pTimer,                       /*OUT  Timer handle*/
  VOID_CALLBACKFUNC(TimeOutCallback)(SSwTimer* pTimer),/*IN   Timeout function*/
  uint32_t iTimeout                     /*IN   timeout time in 1 msec ticks */
  )
{
  TimerSetCallback(pTimer, TimeOutCallback);
  TimerStart(pTimer, iTimeout);
}

/*=========================== RequestUpdateTimeout ===================================
------------------------------------------------------------------------------------*/
static void
ZCB_RequestUpdateTimeout(__attribute__((unused)) SSwTimer* pTimer)
{
  ZW_TransmitCallbackInvoke(&routeUpdateDone, ZW_ROUTE_UPDATE_ABORT, 0);
  ResetSUC();
}


/*=========================== ReqUPdateCallback =====================================
------------------------------------------------------------------------------------*/
static void
ZCB_ReqUpdateCallback(
  __attribute__((unused)) ZW_Void_Function_t Context,
  uint8_t txStatus,
  __attribute__((unused)) TX_STATUS_TYPE *txStatusReport)
{
  //  ZW_SetRFReceiveMode(true);
  if (txStatus == TRANSMIT_COMPLETE_OK)
  {
    NetWorkTimerStart(&SUC_Update.TimeoutTimer, ZCB_RequestUpdateTimeout, SUC_REQUEST_UPDATE_TIMEOUT);
  }
  else
  {
    ResetSUC(); /*Fix TO#:1398 - Should be called before Callback
                                in order to enable application to call new SUC operations*/

    ZW_TransmitCallbackInvoke(&routeUpdateDone, ZW_ROUTE_UPDATE_ABORT, txStatusReport);
  }
}

#ifdef ZW_SELF_HEAL
/*========================   CtrlAskedForHelpCallback   ======================
**    Function description
**      Tell the node that asked for help wheter the controller have been notified
**      or not.
**    Side effects:
**
**--------------------------------------------------------------------------*/
static void
ZCB_CtrlAskedForHelpCallback(
  __attribute__((unused)) ZW_Void_Function_t Context,
  uint8_t txStatus,
  __attribute__((unused)) TX_STATUS_TYPE *txStatusReport)
{
  ACCEPT_LOST_FRAME acceptLostFrame = {
    .cmdClass = ZWAVE_CMD_CLASS_PROTOCOL,
    .cmd = ZWAVE_CMD_ACCEPT_LOST
  };

  if(txStatus==TRANSMIT_COMPLETE_OK)  /*Got hold of controller*/
  {
    acceptLostFrame.accepted = ZW_ROUTE_LOST_ACCEPT;
  }
  else
  {
    acceptLostFrame.accepted = ZW_ROUTE_LOST_FAILED;
  }
  const STransmitCallback TxCallback = { .pCallback = NULL, .Context = 0 };
  EnQueueSingleData(RF_SPEED_9_6K, g_nodeID, helpNodeID, (uint8_t *)&acceptLostFrame,
                    sizeof(ACCEPT_LOST_FRAME),
                    (TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE),
                    0, // 0ms for tx-delay (any value)
                    ZPAL_RADIO_TX_POWER_DEFAULT,
                    &TxCallback);
}
#endif /*ZW_SELF_HEAL*/


/****************************************************************************/
/*                           EXPORTED FUNCTIONS                             */
/****************************************************************************/

/*===========================   NeighborComplete   ===========================
**    Transmission of NOP frame to destID is done, check if failed and if so
**    remove destID in range mask. Initiate next NOP transmission to next node.
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
static void        /*RET Nothing */
ZCB_NeighborComplete(
  __attribute__((unused)) ZW_Void_Function_t Context,
  uint8_t txStatus,   /* IN Transmit status, TRANSMIT_COMPLETE_OK or */
                   /*                     TRANSMIT_COMPLETE_NO_ACK */
  __attribute__((unused)) TX_STATUS_TYPE *txStatusReport)
{
  register uint8_t destID = bCurrentRangeCheckNodeID;

  if (txStatus != TRANSMIT_COMPLETE_OK)
  {
    /* The node couldn't be reached, Clear the bit in the range mask */
    (*pNodeRangeFrame).maskBytes[destID >> 3] &= ~(0x01 << (0x07 & destID));
  }
  /* Send next NOP to next node in range mask */
  FindNeighbors(destID + 1);
}


/*===========================   SendTestFrame   ==========================
**    Function description
**
**    Send a NOP_POWER frame to specified nodeID and powerlevel
**
**    Side effects:
**
**    cmdBuffer.nopPowerFrame (3 first bytes) are changed
**
**--------------------------------------------------------------------------*/
uint8_t                               /*RET false if transmitter busy else true */
SendTestFrame(
  node_id_t nodeID,                   /* IN nodeID to transmit to */
  uint8_t powerLevel,                 /* IN powerlevel index */
  const STransmitCallback* pCallback) /* Call back function called when done */
{
  DPRINTF("SendTestFrame, %u", powerLevel);
  cmdBuffer.nopPowerFrame.cmdClass = ZWAVE_CMD_CLASS_PROTOCOL;
  cmdBuffer.nopPowerFrame.cmd = ZWAVE_CMD_NOP_POWER;
  cmdBuffer.nopPowerFrame.parm = 0;
  if (ZPAL_RADIO_TX_POWER_REDUCED == powerLevel)
  {
    cmdBuffer.nopPowerFrame.parm2 = zpal_radio_get_reduce_tx_power();
  }
  else
  {
    cmdBuffer.nopPowerFrame.parm2 = powerLevel;
  }

  uint8_t RfSpeed = RF_SPEED_9_6K;

  if (bNeighborDiscoverySpeed == ZWAVE_FIND_NODES_SPEED_40K)
    RfSpeed = RF_SPEED_40K;
  if (bNeighborDiscoverySpeed == ZWAVE_FIND_NODES_SPEED_100K)
    RfSpeed = RF_SPEED_100K;

  if (zensorWakeupTime)
    RfSpeed = RF_SPEED_40K | ((zensorWakeupTime == ZWAVE_SENSOR_WAKEUPTIME_250MS) ? RF_OPTION_SEND_BEAM_250MS : RF_OPTION_SEND_BEAM_1000MS);

  return(EnQueueSingleData(RfSpeed, g_nodeID, nodeID, (uint8_t*)&cmdBuffer,
                           sizeof(NOP_POWER_FRAME),
                           (TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_NO_ROUTE),
                           0, // 0ms for tx-delay (any value)
                           powerLevel, pCallback));
}


/*===========================   ZW_SendTestFrame   ==========================
**    Function description
**
**    Send a NOP_POWER frame to specified nodeID and power level
**
**    Side effects:
**
**    cmdBuffer.nopPowerFrame (3 first bytes) are changed
**
**--------------------------------------------------------------------------*/
uint8_t               /*RET false if transmitter busy else true */
ZW_SendTestFrame(
  uint8_t bNodeID,     /* IN nodeID to transmit to */
  uint8_t powerLevel, /* IN powerlevel index */
  const STransmitCallback* pCallback) /* Call back function called when done */
{
  /* TO#3930 fix */
  zensorWakeupTime = ZWAVE_SENSOR_WAKEUPTIME_NONE;

  /* Make sure power leevl test is always done at 9.6 */
  bNeighborDiscoverySpeed = ZWAVE_FIND_NODES_SPEED_9600;
  return (SendTestFrame(bNodeID, powerLevel, pCallback));
}

/*=====================   NeighborCompleteFailCall   =========================
**    Call NeighborComplete with TRANSMIT_COMPLETE_OK
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
void
ZCB_NeighborCompleteFailCall(SSwTimer* pTimer)
{
  TimerStop(pTimer);
  ZCB_NeighborComplete(0, TRANSMIT_COMPLETE_OK, NULL);
}


/*=====================   DelayNeighborCompleteFailCall   ====================
**    Start timer to delay the calling of NeighborComplete with
**    TRANSMIT_COMPLETE_OK
**
**    Side effects:
**      Timer taken
**
**--------------------------------------------------------------------------*/
void
DelayNeighborCompleteFailCall(void)
{
  /* Make timer which times out as quick as possible - 10ms oneshot */
  TimerStart(&NeighborCompleteDelayTimer, 10);
}


/*===========================   FindNeighbors   ============================
**    Send a NOP frame to the next node in nodeRange
**
**    Side effects:
**
**    nodeRange.frame.cmd is changed
**--------------------------------------------------------------------------*/
static void       /* RET Nothing */
FindNeighbors(
  uint8_t nextNode)  /* IN  Next node to transmit to */
{
  register uint8_t max;

  max = (*pNodeRangeFrame).numMaskBytes << 3;

  /* Only progress if findInProgress are true */
  if (g_findInProgress)
  {
    /* Find next bit in nodeRange */
    for (; nextNode < max; nextNode++)
    {
      if ((*pNodeRangeFrame).maskBytes[nextNode >> 3] & (0x01 << (0x07 & nextNode)))
      {
        bCurrentRangeCheckNodeID = nextNode;
        /* TO#3462 fix */
        const STransmitCallback TxCallback = { .pCallback = ZCB_NeighborComplete, .Context = 0 };
        if (!SendTestFrame(bCurrentRangeCheckNodeID + 1, ZPAL_RADIO_TX_POWER_REDUCED, &TxCallback))
        {
          /* Delay calling NeighborComplete max 10ms */
          DelayNeighborCompleteFailCall();
        }
        return;
      }
    }
    /* No more bits are set in the mask, send complete frame to controller */
    g_findInProgress = false;
    /* Reset the bNeighborDiscoverySpeed to Default speed - Is used by ZW_SendTestFrame */
    bNeighborDiscoverySpeed = (1 == llIsHeaderFormat3ch()) ? ZWAVE_FIND_NODES_SPEED_100K : ZWAVE_FIND_NODES_SPEED_9600;

    cmdBuffer.cmdCompleteBuf.cmdClass = ZWAVE_CMD_CLASS_PROTOCOL;
    cmdBuffer.cmdCompleteBuf.cmd = ZWAVE_CMD_CMD_COMPLETE;
    cmdBuffer.cmdCompleteBuf.seqNo = 0;
    assignState = ELEARNSTATUS_ASSIGN_INFO_PENDING;

    const STransmitCallback TxCallback = { .pCallback = ZCB_AssignComplete, .Context = 0 };
    if (!EnQueueSingleData(oldSpeed,
                      g_nodeID, nodeRange.lastController, (uint8_t*)&cmdBuffer,
                      sizeof(COMMAND_COMPLETE_FRAME),
                      nodeRange.txOptions,
                      0, // 0ms for tx-delay (any value)
                      ZPAL_RADIO_TX_POWER_DEFAULT,
                      &TxCallback))
    {
      ZCB_AssignComplete(0, TRANSMIT_COMPLETE_FAIL, NULL);
    }
  }
}

uint8_t
GenerateNodeInformation(
  NODEINFO_SLAVE_FRAME *pnodeInfo,
  uint8_t cmdClass)
{
  // Make sure the device options mask is updated (if changed by Serial API during runtime)
  ZW_nodeDeviceOptionsSet(m_pAppNodeInfo->DeviceOptionsMask);

  // Clear the NIF memory before inserting values.
  memset((uint8_t*)pnodeInfo, 0, sizeof(NODEINFO_SLAVE_FRAME));

  /* fill in cmdclass and cmd */
  pnodeInfo->cmdClass = cmdClass;
  pnodeInfo->cmd = (cmdClass == ZWAVE_CMD_CLASS_PROTOCOL) ? ZWAVE_CMD_NODE_INFO : ZWAVE_LR_CMD_NODE_INFO;

  pnodeInfo->capability = 0;
  if (ZW_nodeIsFLiRS())
  {
    pnodeInfo->security = ((ZW_nodeIsFLiRS() & APPLICATION_FREQ_LISTENING_MODE_1000ms) ? ZWAVE_NODEINFO_SENSOR_MODE_WAKEUP_1000 : ZWAVE_NODEINFO_SENSOR_MODE_WAKEUP_250);
  }
  else
  {
    pnodeInfo->security = 0;
  }

  /* Are we in a Region with a long range channel */
  pnodeInfo->reserved = (zpal_radio_protocol_mode_supports_long_range(zpal_radio_get_protocol_mode()) ? ZWAVE_NODEINFO_BAUD_100KLR : 0);

  if (LOWEST_LONG_RANGE_NODE_ID > g_nodeID)
  {
    /*
     * Not Long Range included or in Long Range only Region
     *
     * ZWAVE_NODEINFO_VERSION_4 not for Long Range channel
     * ZWAVE_NODEINFO_BAUD_40000 always set also in a 3 channel Region
     * ZWAVE_NODEINFO_ROUTING_SUPPORT always set in classic
     */
    pnodeInfo->capability |= ZWAVE_NODEINFO_VERSION_4 | ZWAVE_NODEINFO_BAUD_40000 | ZWAVE_NODEINFO_ROUTING_SUPPORT;
    /*
     * DT:00:11:0002.1 (SDS14224 Z-Wave Plus v2 Device Type Specification):
     *   "A Z-Wave Plus v2 node MUST set the Optional Functionality
     *    bit to 1 in its NIF"
     * Hence we simply hard-code the ZWAVE_NODEINFO_OPTIONAL_FUNC bit below.
     *
     * ZWAVE_NODEINFO_SLAVE_ROUTING only set for none LR node information
     * ZWAVE_NODEINFO_SPECIFIC_DEVICE_TYPE only set for none LR node information
     */
    pnodeInfo->security |= ZWAVE_NODEINFO_BEAM_CAPABILITY | ZWAVE_NODEINFO_OPTIONAL_FUNC |
                           ZWAVE_NODEINFO_SLAVE_ROUTING | ZWAVE_NODEINFO_SPECIFIC_DEVICE_TYPE;
    pnodeInfo->reserved |= ZWAVE_NODEINFO_BAUD_100K;
  }

  pnodeInfo->nodeType = m_pAppNodeInfo->NodeType;

  if (0 != ZW_nodeIsListening())
  {
    pnodeInfo->capability |= ZWAVE_NODEINFO_LISTENING_MASK;  /* always listening node */
  }

  uint8_t maxNumberOfCCs = NODEPARM_MAX;
  uint8_t CCCount;

  const SCommandClassList_t* pCCList = CCListGet(SECURITY_KEY_NONE);
  CCCount = pCCList->iListLength;

  uint8_t * pCC = &pnodeInfo->nodeInfo[0];
  uint8_t * pCount = NULL;
  if (ZWAVE_CMD_CLASS_PROTOCOL_LR == cmdClass)
  {
    /*
     * Transmitting as LR
     *
     * Make pCount point to the first byte so that we can update it after processing the list of
     * command classes and make room for one byte being number of command classes by increasing pCC.
     */
    pCount = pCC++;

    // Since we add a byte for the number of CCs, we subtract one from the max number of CCs.
    maxNumberOfCCs -= 1;
  }

  if (CCCount > maxNumberOfCCs)
  {
    CCCount = maxNumberOfCCs;  /* number of parameter limit exceeded */
  }

  /*
   * Copy the list of command classes without CC S0 if we're included as LR.
   * CC S0 is not supported by LR.
   */
  uint8_t sourceArrayIterator;
  uint8_t destinationArrayIterator = 0;
  for (sourceArrayIterator = 0; sourceArrayIterator < CCCount; sourceArrayIterator++)
  {
    uint8_t cc = *(pCCList->pCommandClasses + sourceArrayIterator);
    if ((COMMAND_CLASS_SECURITY == cc) && (g_nodeID >= LOWEST_LONG_RANGE_NODE_ID))
    {
      continue;
    }
    *(pCC + destinationArrayIterator++) = cc;
  }

  if (NULL != pCount)
  {
    // Update the command class count and increase destinationArrayIterator to include that byte.
    *pCount = destinationArrayIterator++;
  }

  // Return the final length of the NIF.
  return (sizeof(NODEINFO_SLAVE_FRAME) - NODEPARM_MAX + destinationArrayIterator);
}


/*===========================   ZW_SendNodeInformation   ====================
**    Create and transmit a node informations broadcast frame
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
uint8_t                                   /*RET true if put in transmit queue     */
ZW_SendNodeInformation(
  node_id_t    destNode,                  /* IN Destination Node ID  */
  TxOptions_t  txOptions,                 /* IN Transmit option flags */
  const STransmitCallback* pTxCallback)   /* IN Transmit completed call back function */
{
  NODEINFO_SLAVE_FRAME nif = { 0 };
  uint8_t nifLength;
  uint8_t rfSpeed = ChooseSpeedForDestination_slave(destNode);

  /* TO#6436 fix - only allow Application allowed Transmit options */
  txOptions &= TRANSMIT_APPLICATION_OPTION_MASK;
  /* TO#5964 fix - Remember to enable Explore frame resort if TRANSMIT_OPTION_EXPLORE present */
  /* Do we want to use the Explore Frame as a route resolution if all else fails */
  if (txOptions & TRANSMIT_OPTION_EXPLORE)
  {
    /* Note that we want Explore tried as route resolution if all else fails  */
    bUseExploreAsRouteResolution = true;
  }
  /* Not an application frame */
  txOptions &= ~TRANSMIT_OPTION_APPLICATION;

  /* transmit Node Info frame*/
  /* TO#1964 partial possible fix. */
  if (forceUse40K)
  {
    forceUse40K = false;
    nifLength = GenerateNodeInformation(&nif, ZWAVE_CMD_CLASS_PROTOCOL);

    return(EnQueueSingleData(RF_SPEED_40K, g_nodeID, destNode, (uint8_t *)&nif,
                             nifLength, txOptions, 0, // 0ms for tx-delay (any value)
                             (destNode == NODE_BROADCAST) ? ZPAL_RADIO_TX_POWER_REDUCED : ZPAL_RADIO_TX_POWER_DEFAULT,
                             pTxCallback));
  }
  else
  {
    nifLength = GenerateNodeInformation(&nif,
                                        (rfSpeed == RF_SPEED_LR_100K) ? ZWAVE_CMD_CLASS_PROTOCOL_LR : ZWAVE_CMD_CLASS_PROTOCOL);
    return(EnQueueSingleData(rfSpeed, g_nodeID, destNode, (uint8_t *)&nif,
                             nifLength, txOptions, 0, // 0ms for tx-delay (any value)
                             (destNode == NODE_BROADCAST) ? ZPAL_RADIO_TX_POWER_REDUCED : ZPAL_RADIO_TX_POWER_DEFAULT,
                             pTxCallback));
  }
}

/*===========================   ZW_SendExcludeRequestLR   ====================
**    Create and transmit a Z-Wave Long Range Exclude Request broadcast frame
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
bool                                      /*RET true if put in transmit queue     */
ZW_SendExcludeRequestLR(
  const STransmitCallback* pTxCallback)   /* IN Transmit completed call back function */
{
  EXCLUDE_REQUEST_LR_FRAME exclusionRequest = {
    .cmdClass = ZWAVE_CMD_CLASS_PROTOCOL_LR,
    .cmd = ZWAVE_LR_CMD_EXCLUDE_REQUEST
  };

  return EnQueueSingleDataOnLRChannels(RF_SPEED_LR_100K,
                                       g_nodeID,
                                       NODE_BROADCAST_LR,
                                       (uint8_t*)&exclusionRequest,
                                       sizeof(EXCLUDE_REQUEST_LR_FRAME),
                                       0,  // 0ms for tx-delay (any value)
                                       ZPAL_RADIO_TX_POWER_DEFAULT,
                                       pTxCallback);
}

/*============================   AssignTimerInit   ======================
**    Function description
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
static void AssignTimerInit(void)
{
  ZwTimerRegister(&assignTimer, true, ZCB_AssignTimeout);
}

/*============================   AssignTimerStop   ======================
**    Function description
 **
**    Side effects:
**
**--------------------------------------------------------------------------*/
static void
AssignTimerStop(void)
{
  TimerStop(&assignTimer);
}

/*============================   AssignTimeout   ======================
**    Function description
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
static void
ZCB_AssignTimeout( __attribute__((unused)) SSwTimer* pTimer )
{
  AssignTimerStop();
  if (assignState == ELEARNSTATUS_ASSIGN_INFO_PENDING ||
      assignState == ELEARNSTATUS_ASSIGN_WAITING_FOR_FIND)
  {
    assignState = ELEARNSTATUS_ASSIGN_COMPLETE;
    ZCB_AssignComplete(0, TRANSMIT_COMPLETE_OK, NULL);
  }
}

void ForceAssignComplete(void)
{
  AssignTimerStop();
  assignState = ELEARNSTATUS_ASSIGN_COMPLETE;
}

static void
AssignTimerStart(
  uint32_t count)
{
  /* Start a timeout to ensure that we exit if controller fails to respond in due time */
  /* CTRL Wait time is (number of nodes to NOP + 1 + 5)* TRANSFER_MIN_FRAME_WAIT_TIME */
  TimerStart(&assignTimer, TRANSFER_MIN_FRAME_WAIT_TIME_MS * (count + 1));
}

/*============================   ZCB_AssignComplete   ======================
**    Function description
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
static void
ZCB_AssignComplete(
  __attribute__((unused)) ZW_Void_Function_t Context,
  __attribute__((unused)) uint8_t bTxStatus,
  __attribute__((unused)) TX_STATUS_TYPE *txStatusReport)
{
  AssignTimerStop();  /* Stop any timer that might be running. We will restart here if needed */

  /* TO#1757 */
  if ((assignState == ELEARNSTATUS_ASSIGN_WAITING_FOR_FIND) || (assignState == ELEARNSTATUS_ASSIGN_INFO_PENDING))
  {
    /* Delay done because we might receive another find nodes in range frame */
    /* TO#03829 Add 2 Sec more before inform Appl ELEARNSTATUS_ASSIGN_COMPLETE.*/
    AssignTimerStart(ASSIGN_TIMEOUT_2_SEC);
  }
  else
  { /* We are not waiting for timeout anymore.. Decide what to do */
    if (assignState == ELEARNSTATUS_ASSIGN_NODEID_DONE)
    {
      /* nodeID has been assigned or removed. */
      g_learnMode = false;
      g_learnModeDsk = false;
      rfLearnModeFilter_Set(ZW_SET_LEARN_MODE_DISABLE, g_nodeID, ZW_HomeIDGet());
      if (!g_nodeID)
      {
        assignState = ELEARNSTATUS_ASSIGN_COMPLETE;
      }
    }
    /* Tell Application when assignState != ELEARNSTATUS_ASSIGN_INFO_PENDING */
    if (assignState == ELEARNSTATUS_ASSIGN_COMPLETE)
    {
      /* Unlock route */
      ZW_LockRoute(0);

      if (true == g_learnMode)
      {
        rfLearnModeFilter_Set(ZW_SET_LEARN_MODE_DISABLE, g_nodeID, ZW_HomeIDGet());
      }
      g_learnMode = false;
      g_learnModeDsk = false;
      zensorWakeupTime = 0; /*TO#03853 clear flag because learnMode finish.*/
      /* If NWI then start repeater functionality */
      if (!g_learnModeClassic)
      {
        /* TO#2194 partial fix - A FLiRS must never be used as an explore frame repeater */
        /* Is the routing/enhanced slave registered as an FLiRS */
        if (0 == ZW_nodeIsFLiRS())
        {
          ExploreSetNWI(true);
        }
        else
        {
          ExploreSetNWI(false);
        }
      }
      g_learnModeClassic = false;
    }
    if (NULL != learnNodeFunc)
    {
      learnNodeFunc(assignState, g_nodeID);

      if (assignState == ELEARNSTATUS_ASSIGN_COMPLETE)
      {
        learnNodeFunc = NULL;
      }
    }

    /* A new nodeID was assigned. Give controller time to start further action. */
    if (assignState == ELEARNSTATUS_ASSIGN_NODEID_DONE && (ZW_MAX_NODES >= g_nodeID))
    {
      assignState = ELEARNSTATUS_ASSIGN_INFO_PENDING;
      /* TO#03829 Give Controller 2 Sec for further actions. Old controller use frame with BEAM info*/
      AssignTimerStart(ASSIGN_TIMEOUT_2_SEC);
    }
  }
}

/*============================   startSecureInclusion   ======================
**    Starts secure part of inclusion when S2 or SmartStart is used for Z-Wave nodes,
**    or in case of LR SmartStart inclusion
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
static void startSecureInclusion(void)
{
  uint32_t* pHomeIdUint32 = (uint32_t*)ZW_HomeIDGet();  // First cast to uint32 pointer to avoid
  // GCC type-punned pointer error
  /* convert big endian homeid to native format needed by libs2 */
  s2_ctx = S2_init_ctx(UIP_HTONL(*pHomeIdUint32));

  s2_inclusion_glue_set_securelearnCbFunc(LearnModeDone);
  s2_inclusion_glue_join_start(s2_ctx);
}

#ifdef ZW_SECURITY_PROTOCOL
void ZCB_SecureLearnProtocolCallback(uint8_t txStatus, __attribute__((unused)) node_id_t nodeId)
{
  if (g_nodeID == 0)
  {
    /* Remove any knowledge of any previous security settings */
    security_reset();

    /* We are being excluded, pass callback to application
     * and don't start security inclusion. */
    LearnModeDone(txStatus, g_nodeID);
    learnNodeFunc = NULL;
    return;
  }

  // If it's LR node, just exit. Secure inclusion will start upon receiving ZWAVE_LR_CMD_NON_SECURE_INCLUSION_COMPLETE
  if (g_nodeID >= LOWEST_LONG_RANGE_NODE_ID)
  {
    LearnModeDone(txStatus, g_nodeID);
    return;
  }
  uint8_t keys = getSecureKeysRequested();
  if ((ELEARNSTATUS_ASSIGN_COMPLETE == txStatus) && ((keys & SECURITY_KEY_S2_MASK) || (keys & SECURITY_KEY_S0_BIT)))
  {
    /* Drop state - Security 0/2 inclusion state machines - will handle the callback */

    /* Inform S0 inclusion machine that Protocol is all done - start S0 inclusion timeout */
    if (keys & SECURITY_KEY_S0_BIT)
    {
      security_learn_begin(LearnModeDone);
    }

    /* Inform S2 inclusion machine that Protocol is all done - start S2 inclusion timeout */
    if (keys & SECURITY_KEY_S2_MASK)
    {
      s2_inclusion_glue_set_securelearnCbFunc(LearnModeDone);
      s2_inclusion_neighbor_discovery_complete(s2_ctx);
    }
  }
  else
  {
    if (ELEARNSTATUS_ASSIGN_NODEID_DONE == txStatus)
    {
      /*Assume inclusion succeeds*/
      secure_learn_set_errno(E_SECURE_LEARN_ERRNO_COMPLETE);

      if (keys & SECURITY_KEY_S0_BIT)
      {
        security_learn_begin_early(LearnModeDone);
      }

      if (keys & SECURITY_KEY_S2_MASK)
      {
        startSecureInclusion();
      }
    }
    LearnModeDone(txStatus, g_nodeID);
  }
}

static void ProtocolChangeRfPHY();

/**
 * This function is the final step in learnmode, It calls the application callback.
 */
static void LearnModeDone(uint8_t bStatus,
                          node_id_t nodeID)
{
  if (bStatus == ELEARNSTATUS_ASSIGN_COMPLETE)
  {
    DPRINT("Learnmode is done\n");
    zpal_pm_cancel(learn_power_lock);
  }
  if (bStatus == ELEARNSTATUS_ASSIGN_COMPLETE || bStatus == ELEARNSTATUS_ASSIGN_NODEID_DONE)
    ProtocolChangeRfPHY();
  if (secureLearnAppCbFunc)
  {
    secureLearnAppCbFunc(bStatus, nodeID);
  }
}
#endif


/*===========================   ZW_SetLearnMode   ===========================
**    Enable/Disable home/node ID learn mode.
**    When learn mode is enabled, received "Assign ID's Command" are handled:
**    If the current stored ID's are zero, the received ID's will be stored.
**    If the received ID's are zero the stored ID's will be set to zero.
**
**    The learnFunc is called when the received assign command has been handled.
**    The returned parameter is the learned Node ID.
**
**--------------------------------------------------------------------------*/
void                                          /*RET Nothing        */
ZW_SetLearnMode(
  uint8_t mode,                                  /* IN learnMode bitmask */
  learn_mode_callback_t learnFunc)  /* IN Node learn call back function. */
{
  DPRINTF("ZW_SetLearnMode = %02x\r\n",mode);
  if (ZW_SET_LEARN_MODE_NWE < mode)
  {
    mode = ZW_SET_LEARN_MODE_DISABLE;
  }
  InternSetLearnMode(mode,learnFunc);

}

void
InternSetLearnMode(
  uint8_t mode,                                  /* IN learnMode bitmask */
  learn_mode_callback_t learnFunc)  /* IN Node learn call back function. */
{
  /* Are we IDLE */
  if (assignState == ELEARNSTATUS_ASSIGN_COMPLETE)
  {
    g_learnMode = (bool)mode;
    rfLearnModeFilter_Set(mode, g_nodeID, ZW_HomeIDGet());
    g_learnModeClassic = (ZW_SET_LEARN_MODE_CLASSIC == mode);
    g_learnModeDsk     = (ZW_SET_LEARN_MODE_SMARTSTART == mode);

    if (g_learnMode)
    {
      if(ZW_SET_LEARN_MODE_SMARTSTART != mode)
      {
        zpal_pm_stay_awake(learn_power_lock, 0);
      }
#ifdef ZW_SECURITY_PROTOCOL
      {
        secureLearnAppCbFunc = (sec_learn_complete_t)learnFunc;
        learnNodeFunc = ZCB_SecureLearnProtocolCallback;
        ZW_KeystoreInit();
      }
#else /* ZW_SECURITY_PROTOCOL */
      learnNodeFunc = learnFunc;
#endif /* ZW_SECURITY_PROTOCOL */
      /*  if not classic, then set NWI parameters */
      if (!g_learnModeClassic)
      {
        switch (mode)
        {
          case ZW_SET_LEARN_MODE_SMARTSTART:
            {
              NetworkManagementGenerateDskpart();
              NetworkManagement_IsSmartStartInclusionSet();
              bNetworkWideInclusion = NETWORK_WIDE_MODE_JOIN;
              exploreInclusionModeRepeat = false;
            }
            break;
          case ZW_SET_LEARN_MODE_NWI:
            {
              bNetworkWideInclusion = NETWORK_WIDE_MODE_JOIN;
              exploreInclusionModeRepeat = false;
              NetworkManagement_IsSmartStartInclusionClear();  // Not SmartStart
            }
            break;

          case ZW_SET_LEARN_MODE_NWE:
            {
              bNetworkWideInclusion = NETWORK_WIDE_MODE_LEAVE;
              exploreInclusionModeRepeat = false;
              NetworkManagement_IsSmartStartInclusionClear();  // Not SmartStart
            }
            break;

          default:
            g_learnMode = false;
            g_learnModeDsk = false;
            bNetworkWideInclusion = NETWORK_WIDE_MODE_IDLE;
            exploreInclusionModeRepeat = false;
            NetworkManagement_IsSmartStartInclusionClear();  // Not SmartStart
            break;
        }
      }
      else  // Classic
      {
        bNetworkWideInclusion = NETWORK_WIDE_MODE_IDLE;
        exploreInclusionModeRepeat = false;
        NetworkManagement_IsSmartStartInclusionClear();  // Not SmartStart
      }
    }
    else
    {
      NetworkManagement_IsSmartStartInclusionClear();  // Not SmartStart
      zpal_pm_cancel(learn_power_lock);
      DPRINT("LearnMode stopped by application\n\r");
      bNetworkWideInclusion = NETWORK_WIDE_MODE_IDLE;
#ifdef ZW_SECURITY_PROTOCOL
      secureLearnAppCbFunc = (sec_learn_complete_t)learnFunc;
      learnNodeFunc = ZCB_SecureLearnProtocolCallback;
#else
      learnNodeFunc = learnFunc;
#endif
#ifdef ZW_SECURITY_PROTOCOL
      /* Stop S0 inclusion machine if allready started */
      security_learn_exit();
      /* Stop S2 inclusion machine if allready started */
      sec2_inclusion_abort();
#endif
    }
  }
  else
  {
    /*assignState not ELEARNSTATUS_ASSIGN_COMPLETE. Let ZCB_AssignComplete decide what to do*/
    ZCB_AssignComplete(0, TRANSMIT_COMPLETE_OK, NULL);
  }
}


/*============================   ZW_GetSUCNodeID  ===========================
**    Function description
**    This function gets the nodeID of the current Static Update Controller
**    if ZERO then no SUC is available
**
**--------------------------------------------------------------------------*/
uint8_t                     /*RET nodeID on SUC, if ZERO -> no SUC */
ZW_GetSUCNodeID( void )  /* IN Nothing */
{
  return g_sucNodeID;
}

/*======================   SlaveRegisterPowerLocks   =======================
**    Initializes slave power locks
**--------------------------------------------------------------------------*/
void SlaveRegisterPowerLocks(void)
{
  learn_power_lock = zpal_pm_register(ZPAL_PM_TYPE_USE_RADIO);
}

/*=============================   SlaveInit   ==============================
**    Initialize variables and structures. If bLoadNodeInformation != 0 then
**    Application node information is loaded from the application to
**    initialize the nodeinformation registered in the protocol.
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
void                          /*RET Nothing */
SlaveInit(const SAppNodeInfo_t * pAppNodeInfo) /* IN Request Nodeinformation from Application */
{
  m_pAppNodeInfo = pAppNodeInfo;

  /* Initialize range info to a valid value */
  nodeNormalRangeFrame.numMaskBytes = 1;
  nodeNormalRangeFrame.maskBytes[0] = 0;
  nodeFLiRSRangeFrame.numMaskBytes = 1;
  nodeFLiRSRangeFrame.maskBytes[0] = 0;
  pNodeRangeFrame = &nodeNormalRangeFrame;
  /* TO#3152 fix - Reset the bNeighborDiscoverySpeed to Default speed - Is used by ZW_SendTestFrame */
  bNeighborDiscoverySpeed = (1 == llIsHeaderFormat3ch()) ? ZWAVE_FIND_NODES_SPEED_100K : ZWAVE_FIND_NODES_SPEED_9600;

  // Initialize timers
  ZwTimerRegister(&NeighborCompleteDelayTimer, true, ZCB_NeighborCompleteFailCall);

  ZwTimerRegister(&SUC_Update.TimeoutTimer, true, NULL);

	ZW_nodeDeviceOptionsSet(m_pAppNodeInfo->DeviceOptionsMask);
  zpal_radio_enable_rx_broadcast_beam(false);

  /* Stop any possible ongoing inclusion/exclusion functionality */
  AssignTimerInit();  // Ensure timer is inited
  AssignTimerStop();
  assignState = ELEARNSTATUS_ASSIGN_COMPLETE;
  g_learnMode = false;
  g_learnModeDsk = false;
  g_findInProgress = false;
  g_learnModeClassic = false;
  forceUse40K = false;
  /* Unlock route if any locked. */
  ZW_LockRoute(0);
}


/*===================   RemoveAllReturnRouteDestination   ====================
**    Remove Cahced Return Route Destinations.
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
static void
RemoveAllReturnRouteDestination(void)
{
  memset(ZW_RouteDestinations, 0, sizeof(ZW_RouteDestinations));
  SlaveStorageSetRouteDestinations(ZW_RouteDestinations);
}


/*======================   ReloadRouteDestinations   =========================
**    Reload route destinations from none volatile RAM
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
void
ReloadRouteDestinations(void)
{
  bool bWriteBack = false;
  uint32_t i= sizeof(ZW_RouteDestinations);
  SlaveStorageGetRouteDestinations(ZW_RouteDestinations) ;
  do
  {
    if (ZW_RouteDestinations[--i] > ZW_MAX_NODES)
    {
      /* Give it a valid value - ZERO no node... */
      ZW_RouteDestinations[i] = 0;
      bWriteBack = true;
    }
  } while (i);

  if (bWriteBack)
  {
    SlaveStorageSetRouteDestinations(ZW_RouteDestinations)  ;
  }
}


/*====================   UpdateResponseRouteLastReturnRoute   ================
**    Function called when a new Response Route has been stored.
**    Checks if a Return Route exists for the destination :
**      If the destination is registered as needing a BEAM the
**    Response Route is updated accordingly.
**
**      For Nodes supporting it, the Response Route is inserted into
**    the Return Route with the lowest priority if the route do not exist
**    allready.
**      Updates Last Return Route/the Return Route with the lowest priority
**    with route in Response Route structure at bResponseRouteIndex if
**    route do not exist allready.
**
**
**
**    Side effects:
**      Response Route updated with BEAM info according to Return Route present.
**
**      Last Return Route/the Return Route with the lowest priority updated
**      with new route if conditions are met.
**
**
**--------------------------------------------------------------------------*/
void
UpdateResponseRouteLastReturnRoute(
  uint8_t bResponseRouteIndex)         /* IN Index pointing to the Response Route */
                                    /*    in question */
{
  uint8_t t, q, bRepeat;

  /* TO#2833 partial fix - Response Route/Last Working Route now never caches 9.6kb direct frames. */
  uint8_t bDestNodeID;
  if (g_findInProgress)
  {
    return;
  }
  if (bResponseRouteIndex >= MAX_ROUTED_RESPONSES)
  {
    /* Route not stored - drop update */
    return;
  }
  bDestNodeID = responseRouteNodeID[bResponseRouteIndex];

  uint32_t i = 0;
  if (bDestNodeID != g_sucNodeID)
  {
    i = bDestNodeID;
  }
  NVM_RETURN_ROUTE_STRUCT t_ReturnRoute = { 0 };
  NVM_RETURN_ROUTE_SPEED   t_ReturnRouteSpeed = { 0 };
  SlaveStorageGetReturnRoute(i, &t_ReturnRoute);
  SlaveStorageGetReturnRouteSpeed(i, &t_ReturnRouteSpeed);

  /* Mark entry as used */
  t_ReturnRoute.nodeID = bDestNodeID;
  {
    /* A matching return route destination exist */
    /* i is index of the destination where ZERO is the SUC routes */
    /* TO#2758 fix - Return Routes exists; check if node should be BEAMed and update the Response Route accordingly */
    uint8_t BeamInfoDestinationReturnRoutes;

    /* Get route BEAM information. */
    /* BeamInfoDestinationReturnRoutes contains BEAM info for destination return routes. */
    /* Speed/beam info uses 2 byte for every Destination - Beam is second byte. */
    /*  Dest_1_beam[1].unused[b6].beam_dst_0[b2] */
    /* NVM_RETURN_ROUTE_BEAM_DEST_MASK = 0x03 */
    /* TODO - SHL of i will it result in a uint16_t?? */
    BeamInfoDestinationReturnRoutes = t_ReturnRouteSpeed.speed.bytes[1];
    /* If destination is registered as a FLiRS then force the response route to reflect this */
    if (BeamInfoDestinationReturnRoutes & NVM_RETURN_ROUTE_BEAM_DEST_MASK)
    {
      /* Force 40k and BEAM as destination is a FLiRS */
      responseRouteSpeedNumHops[bResponseRouteIndex] &= ~(RF_OPTION_SPEED_MASK << 4);
      responseRouteSpeedNumHops[bResponseRouteIndex] |= (RF_SPEED_40K << 4);
      /* RF_OPTION_SEND_BEAM_250MS        = 0x20 */
      /* RF_OPTION_SEND_BEAM_1000MS       = 0x40 */
      responseRouteRFOption[bResponseRouteIndex] &= ~(RF_OPTION_SEND_BEAM_250MS | RF_OPTION_SEND_BEAM_1000MS);
      responseRouteRFOption[bResponseRouteIndex] |= ((BeamInfoDestinationReturnRoutes & NVM_RETURN_ROUTE_BEAM_DEST_MASK) << 5);
    }

    /* Destination return route can be placed in SUC Return Routes if i == ZERO */
    /* else it should be in the Return Route structure */
    /* TO#2814 fix - As i == 0 indicates SUC return routes then we need to offset from NVM_RETURN_ROUTE_SUC */
    for (q = 0; q < RETURN_ROUTE_MAX; q++)
    {
      /* Check if the route is a new route */

      /* Check repeater list in frame with repeaters in eeprom */
      for (t = 0; t < RETURN_ROUTE_SIZE; t++)
      {
        uint8_t Repeater = t_ReturnRoute.routeList[q].repeaterList[t];
        bRepeat = (t < (responseRouteSpeedNumHops[bResponseRouteIndex] & 0x0f)) ? responseRouteRepeaterList[bResponseRouteIndex][t] : (!t ? CACHED_ROUTE_LINE_DIRECT : 0);
        if (bRepeat != Repeater)
        {
          break; /* Not equal check next route */
        }
        else
        {
          /* TO#2817 fix */
          if ((bRepeat == CACHED_ROUTE_LINE_DIRECT) || (t && !bRepeat && (t == (responseRouteSpeedNumHops[bResponseRouteIndex] & 0x0f))))
          {
            /* Both routes are Direct or all valid repeaters and number of repeaters are equal. */
            /* - no need for testing anymore. */
            return;
          }
        }
      }
      if (t == RETURN_ROUTE_SIZE)
      {
        return; /* route match, dont add it */
      }
    }

    /* No match */

    /* Check if first route is direct */
    /* New Routing Scheme - Direct can be in assigned routes */
    for (t = 0; t < RETURN_ROUTE_MAX; t++)
    {
      /* Read first repeater to see if the route is free */
      if (!t_ReturnRoute.routeList[ZCB_ReturnRouteFindPriority(t, i)].repeaterList[0])
      {
        break;  /* Route is free */
      }
    }

    if (t >= RETURN_ROUTE_MAX)
    {
      t = RETURN_ROUTE_MAX - 1;
    }

    /* t points to route index now */
    t = ZCB_ReturnRouteFindPriority(t, i);

    /* Store new route */
    /* HopCount contains the number of hops in the repeater list */
    uint8_t HopCount = (responseRouteSpeedNumHops[bResponseRouteIndex] & 0x0f);
    q = 0;
    if (!HopCount)
    {
      /* New Routing Scheme - First route and direct */
      t_ReturnRoute.routeList[t].repeaterList[0] = CACHED_ROUTE_LINE_DIRECT;
      q++;
    }
    for (; q < HopCount; q++)
    {
      /* Save the repeater in list */
      t_ReturnRoute.routeList[t].repeaterList[q] = responseRouteRepeaterList[bResponseRouteIndex][q];
    }
    /* Set speed bit for new route */
    if ((responseRouteSpeedNumHops[bResponseRouteIndex] & 0xF0) < RF_SPEED_40K)
    {
      t_ReturnRouteSpeed.speed.bytes[0] &= ~(1 << t);
    }
    else
    {
      t_ReturnRouteSpeed.speed.bytes[0] |= (1 << t);
    }

    /* Store speed info and return routes for destination node referenced by i*/
    SlaveStorageSetReturnRoute(i, &t_ReturnRoute, &t_ReturnRouteSpeed);
  }
}


/*==========================   SlaveInitDone   ==============================
**    Callback function called when configuration is valid
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
void
SlaveInitDone(zpal_radio_region_t eRegion)
{
  m_initRegion = eRegion;
  SlaveStorageGetNetworkIds(ZW_HomeIDGet(), &g_nodeID);
  ResetSUC();
  NVM_RETURN_ROUTE_STRUCT t_ReturnRoute = { 0 };
  SlaveStorageGetReturnRoute(0, &t_ReturnRoute);
  SlaveSetSucNodeId(t_ReturnRoute.nodeID);  /* [0] is SUC */

  ReloadRouteDestinations();
  node_id_t tmpNodeID;
  StorageGetNodeId(&tmpNodeID);
  zpal_radio_set_long_range_lock(tmpNodeID >= LOWEST_LONG_RANGE_NODE_ID);
}


/*========================  ConvertRouteSpeedToSlaveNVM ============================
** Convert from 3-bit speed field in AssignReturnRoute frame to the 2-bit
** representation in NVM - EEPROM, of enhanced/routing slaves.
**
** This conversion is possible without losing information because only four
** of the 2^3 values are actually used.
** ------------------------------------------------------------------------- */
#define ConvertRouteSpeedToSlaveNVM(rs) (((rs) >> 3) & NVM_RETURN_ROUTE_SPEED_MASK)

/*========================  CalculateReturnRouteSpeed ============================
** Save the destination route speed
**
#ifdef ONE_ROUTE_TO_DESTINATION
** For the single route to a destination three bits are saved to describe the
** speed of the route. Each destination's speed/beam info takes up one full byte.
#else
** For each route to a destination two bits are saved to describe the speed of
** the route. There are max 4 routes to a destination, and beam info uses 4
** bits per destination. In total, each destination's speed/beam info takes
** up two bytes.
#endif
** ------------------------------------------------------------------------- */
bool
CalculateReturnRouteSpeed(
  ASSIGN_RETURN_ROUTE_FRAME* returnRoute,    /* Return route frame */
  uint8_t destIndex,                         /* Destination index, 0 is SUC route */
  uint8_t bPayloadSize,                      /* Size of the payload in the assign frame */
  NVM_RETURN_ROUTE_SPEED * returnRouteSpeed) /* Output route speed*/
{
  register uint8_t n;

  SlaveStorageGetReturnRouteSpeed(destIndex, returnRouteSpeed);

  if (returnRoute->nodeID == 0)
  {
    /* if node id is zero don't manipulate returnRouteSpeed */
    return false;
  }

  /* TO#1258 fix - DestinationSpeed, bRouteNumber in the function "CalculateReturnRouteSpeed" was */
  /* only initialized correctly if the route was a 40k route. If route */
  /* was a 9600 some variables was not initialized correctly but used anyway */
  /* and thereby the speed could end up being 40k even if the destination was */
  /* a 100 series. */

  uint8_t bRouteNumber;
  /* route number */
  bRouteNumber = (returnRoute->routeNoNumHops >> 4);
  /* Check if the frame contains speed information */
  if (bPayloadSize - (returnRoute->routeNoNumHops & 0xF) >= (int)(sizeof(ASSIGN_RETURN_ROUTE_FRAME) - MAX_REPEATERS))
  {
    /* Wakeup type for src and dst are placed in the routeSpeed byte : */
    /* dst wakeup type mask = 00000110 (bit2 and bit1) */
    /* src wakeup type mask = 11000000 (bit7 and bit6) */
    /* The Route speed mask = 00111000 (bit5, bit4 and bit3) */
    /* returnRouteNodeID is the src and returnDestNodeID is the dst */
    /* routeSpeed is at variable offset in frame depending on repeater list length */
    n = returnRoute->routeSpeed = returnRoute->repeaterList[(returnRoute->routeNoNumHops & 0x0F)];
    /* prepare DestCapabilities for wakeup beam info so it can be saved in EEPROM */
    /* TO#1527 fix - only set wakeup beam info if first route */
    if (!bRouteNumber)
    {
      /* TO#01782 fix - BinarySensorListening sends Wakeup Beam to the Leddimmer after Assign Return Router to it */
      returnRouteSpeed->speed.bytes[1] &= ~(NVM_RETURN_ROUTE_BEAM_DEST_MASK);
      /* In ASSIGN_RETURN_ROUTE frame the BEAM dst bits are place at bit1-2 */
      /* In EEPROM we place the BEAM info at bit0-1 in the byte following the speed byte. */
      returnRouteSpeed->speed.bytes[1] |= ((n & ROUTE_SPEED_DST_WAKEUP_BEAM_MASK) >> 1);
    }
  }
  else /* No speed info, use 9.6 */
  {
    /* Set speed to 9.6*/
    returnRouteSpeed->speed.bytes[0] &= ~NVM_RETURN_ROUTE_SPEED_MASK;
    returnRouteSpeed->speed.bytes[0] |= NVM_RETURN_ROUTE_SPEED_9600;
    returnRoute->routeSpeed = RETURN_ROUTE_BAUD_9600;
  }
  /* Shift speed bits to proper position: aabbccdd, aa=route 3, bb=route 2, ..., dd = route 0 */
  returnRouteSpeed->speed.bytes[0] &= ~(NVM_RETURN_ROUTE_SPEED_MASK << (bRouteNumber << 1));
  returnRouteSpeed->speed.bytes[0] |= (ConvertRouteSpeedToSlaveNVM(returnRoute->routeSpeed) << (bRouteNumber << 1));

  return true;
}


/*=======================   ZW_GetNodeTypeBasic   ============================
**
**    Get the Basic Device Type according to the bits in nodeInfo
**
**    Side effects:
**      nodeInfo->nodeType.basic updated
**
**--------------------------------------------------------------------------*/
uint8_t                  /*RET Basic Device Type */
ZW_GetNodeTypeBasic(
  NODEINFO *nodeInfo) /* IN pointer NODEINFO struct to use in Basic Type decision */
{
  if ((nodeInfo->security & ZWAVE_NODEINFO_CONTROLLER_NODE))
  {
    /* It is a Controller! */
    if ((nodeInfo->capability & ZWAVE_NODEINFO_LISTENING_SUPPORT))
    {
      /* TODO: WE NEED TO FIND A BETTER WAY TO DISTINGUISH THE CONTROLLER LIBRARIES */
      nodeInfo->nodeType.basic = BASIC_TYPE_STATIC_CONTROLLER;
    }
    else
    {
      nodeInfo->nodeType.basic = BASIC_TYPE_CONTROLLER;
    }
  }
  else
  {
    /* It is a Slave! */
    if ((nodeInfo->security & ZWAVE_NODEINFO_SLAVE_ROUTING))
    {
      nodeInfo->nodeType.basic = BASIC_TYPE_ROUTING_END_NODE;
    }
    else
    {
      nodeInfo->nodeType.basic = BASIC_TYPE_END_NODE;
    }
  }
  return (nodeInfo->nodeType.basic);
}

/*=======================   ZW_HandleCmdNOP   ============================
**
**    Common command handler for:
**           - ZWAVE_CMD_NOP
**           - ZWAVE_LR_CMD_NOP
**
**--------------------------------------------------------------------------*/
void ZW_HandleCmdNOP(__attribute__((unused)) uint8_t *pCmd, __attribute__((unused)) uint8_t cmdLength, RECEIVE_OPTIONS_TYPE *rxopt)
{
  node_id_t sourceNode = rxopt->sourceNode;

  DPRINTF("ZWAVE_CMD_NOP - g_learnMode:%02X, assignState:%02X\r\n", g_learnMode, assignState);
  if (!g_learnMode && (ELEARNSTATUS_ASSIGN_COMPLETE == assignState))
  {
    /* We got a NOP frame so we just send it upstairs as NOP_POWER frame*/
    ProtocolInterfacePassToAppNodeUpdate(
        UPDATE_STATE_NOP_POWER_RECEIVED,
        sourceNode,
        NULL,
        0);
  }
  return;
}

/*=======================   ZW_HandleCmdNodeInfo  ===========================
**
**    Common command handler for:
**           - ZWAVE_CMD_NODE_INFO
**           - ZWAVE_LR_CMD_NODE_INFO
**
**--------------------------------------------------------------------------*/
static void ZW_HandleCmdNodeInfo(uint8_t *pCmd, uint8_t cmdLength, RECEIVE_OPTIONS_TYPE *rxopt)
{
  node_id_t sourceNode = rxopt->sourceNode;

  /* Check if frame is to short then drop it... */
  if (cmdLength < offsetof(NODEINFO_OLD_FRAME, nodeInfo))
  {
    return;
  }
  uint8_t deviceClassLen = cmdLength;
  memset(nodeInfoFrame.nodeInfo, 0, NODEPARM_MAX);
  if (!(((NODEINFO_FRAME*)pCmd)->security & ZWAVE_NODEINFO_SPECIFIC_DEVICE_TYPE))
  { /* No Specific Device Type in nodeInfoFrame frame... */
    /* Now set the Specific Device Type support bit */
    ((NODEINFO_FRAME*)pCmd)->security |= ZWAVE_NODEINFO_SPECIFIC_DEVICE_TYPE;
    /* First copy until and including nodeType (for old devices its generic device type) */
    memcpy((uint8_t*)&nodeInfoFrame, pCmd, offsetof(NODEINFO_OLD_FRAME, nodeInfo));
    nodeInfoFrame.nodeType.generic = nodeInfoFrame.nodeType.basic;  /* Put generic the right place */
    ZW_GetNodeTypeBasic((NODEINFO *)&nodeInfoFrame.capability); /* Set the Basic Device Type */
    nodeInfoFrame.nodeType.specific = 0; /* Set the Specific Device Type to something useful */
    deviceClassLen += 2; /* Add the Basic and Specific Device classes */
    /* Copy the command classes - if any... */
    if (cmdLength - offsetof(NODEINFO_OLD_FRAME, nodeInfo) > 0)
    {
      /* TO#3281 partial fix : Received nodeInfoFrame must MAX have NODEPARM_MAX commandclasses */
      if (cmdLength - offsetof(NODEINFO_OLD_FRAME, nodeInfo) > NODEPARM_MAX)
      {
        cmdLength = offsetof(NODEINFO_OLD_FRAME, nodeInfo) + NODEPARM_MAX;
      }
      memcpy(nodeInfoFrame.nodeInfo,
              pCmd + offsetof(NODEINFO_OLD_FRAME, nodeInfo),
              cmdLength - offsetof(NODEINFO_OLD_FRAME, nodeInfo));
      /* TO#3281 partial fix : Received nodeInfoFrame must MAX have NODEPARM_MAX commandclasses */
      deviceClassLen = cmdLength;
    }
  }
  else
  {
    /* First copy everything, for Controllers the first Device Type in the nodeinfo */
    /* is the Basic Device Type, for Slaves the First Device Type it is the Generic Device Type */
    /* TO#3281 partial fix : Received nodeInfoFrame must MAX have NODEPARM_MAX commandclasses */
    if (cmdLength > sizeof(nodeInfoFrame))
    {
      cmdLength = sizeof(nodeInfoFrame);
      deviceClassLen = cmdLength;
    }
    memcpy((uint8_t*)&nodeInfoFrame, pCmd, cmdLength);
    if (!(nodeInfoFrame.security & ZWAVE_NODEINFO_CONTROLLER_NODE))
    { /* It is a Slave */
      ZW_GetNodeTypeBasic((NODEINFO *)&nodeInfoFrame.capability); /* Set the Basic Device Type */
      /* TO#3281 partial fix : Received nodeInfoFrame must MAX have NODEPARM_MAX commandclasses */
      if (cmdLength == sizeof(nodeInfoFrame))
      {
        /* Drop one more Command Class as no room for it. */
        cmdLength--;
      }
      else
      {
        deviceClassLen++;  /* One more for the Basic Device Type, which is generated */
      }
      /* Copy Generic and Specfic Device Types and the Command Classes */
      memcpy((uint8_t*)&nodeInfoFrame.nodeType.generic,
              pCmd + offsetof(NODEINFO_SLAVE_FRAME, nodeType),
              cmdLength - offsetof(NODEINFO_SLAVE_FRAME, nodeType));
    }
  }
  /* We got a Nodeinformation frame so we just send it upstairs */
  ProtocolInterfacePassToAppNodeUpdate(
                                        UPDATE_STATE_NODE_INFO_RECEIVED,
                                        sourceNode,
                                        (uint8_t*)&nodeInfoFrame.nodeType,
                                        deviceClassLen - offsetof(NODEINFO_FRAME, nodeType)
                                      );
}

/*=======================   ZW_HandleCmdRequestNodeInfo  =====================
**
**    Common command handler for:
**           - ZWAVE_CMD_REQUEST_NODE_INFO
**           - ZWAVE_LR_CMD_REQUEST_NODE_INFO
**
**--------------------------------------------------------------------------*/
static void ZW_HandleCmdRequestNodeInfo(__attribute__((unused)) uint8_t *pCmd, __attribute__((unused)) uint8_t cmdLength, RECEIVE_OPTIONS_TYPE *rxopt)
{
  node_id_t sourceNode = rxopt->sourceNode;
  uint8_t rxStatus = rxopt->rxStatus;

  const STransmitCallback TxCallbackNull = { .pCallback = NULL, .Context = 0 };

  if (rxStatus & RECEIVE_STATUS_LOW_POWER)
  {
    ZW_SendNodeInformation(sourceNode,
                            TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE | TRANSMIT_OPTION_LOW_POWER, &TxCallbackNull);
  }
  else
  {
    ZW_SendNodeInformation(sourceNode,
                            TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE, &TxCallbackNull);
  }
  /* Notify the application about the received frame */
  /* Utilized by sensor applications to extend the "stay awake" period by a number of seconds */
  ProtocolInterfacePassToAppNodeUpdate(
                                        UPDATE_STATE_REQUEST_NODE_INFO_RECEIVED,
                                        sourceNode,
                                        NULL,
                                        0
                                      );
}

/*=======================   ZW_HandleCmdAssignIDs   ========================
**
**    Command handler for:
**           - ZWAVE_CMD_ASSIGN_IDS
**
**--------------------------------------------------------------------------*/
static void ZW_HandleCmdAssignIDs(uint8_t *pCmd, uint8_t cmdLength, __attribute__((unused)) RECEIVE_OPTIONS_TYPE *rxopt)
{
  node_id_t sourceNode = rxopt->sourceNode;

  if ((g_findInProgress == false) && g_learnMode)
  {
    /* Fix for TO#1362, TO#1367, TO#1386 */
    /* Check if length of frame is correct */
    if ( cmdLength < sizeof(ASSIGN_IDS_FRAME) )
    {
      return;
    }

    assignState = ELEARNSTATUS_ASSIGN_NODEID_DONE;

    /* set new home and node ID's */
    g_nodeID = ((ASSIGN_IDS_FRAME*)pCmd)->newNodeID;
    ZW_HomeIDSet(((ASSIGN_IDS_FRAME*)pCmd)->newHomeID);

    /* store the ID's */
    HomeIdUpdate(ZW_HomeIDGet(), g_nodeID);
    /* Remove specific learnMode Receive filters */
    rfLearnModeFilter_Set(ZW_SET_LEARN_MODE_DISABLE, g_nodeID, ZW_HomeIDGet());

    if (!g_nodeID)
    {
      /* delete response routes */
      ResetRoutedResponseRoutes();
      FlushResponseSpeeds();
      RemoveAllReturnRouteDestination();
      /*Deletes all return routes info files*/
      SlaveStorageDeleteSucReturnRouteInfo();
      SlaveStorageDeleteAllReturnRouteInfo();

      SlaveSetSucNodeId(0);
#ifdef ZW_SECURITY_PROTOCOL
      sec2_inclusion_abort(); /* Make sure to stop S2 inclusion state machine */
      security_reset();
#endif /* ZW_SECURITY_PROTOCOL */
    }

    /* report the new node ID to the application */
    ZCB_AssignComplete(0, TRANSMIT_COMPLETE_OK, NULL);

    /* Lock the Response Route, if any, to make sure we keep the response route route */
    /* until inclusion is done. */
    ZW_LockRoute(sourceNode);
  }
}


/**
* Function used when RF PHY needs to be changed runtime - Is used by the protocol PROTOCOL_EVENT_CHANGE_RF_PHY event
* Before inclusion, end device must use both classic Z-Wave channels and LR channel A (LR_CH_CFG1)
* After Classic inclusion, end device should use only classic Z-Wave channels (LR_CH_CFG_NO_LR).
* After long range inclusion, end device should use only LR channels (LR_CH_CFG3).
* Current nodeID is use to detect the inclusion type.
*/
static void
ProtocolChangeRfPHY(void)
{
  // Only applicable for Long Range Regions
  if (zpal_radio_region_is_long_range(m_initRegion))
  {
    zpal_radio_lr_channel_config_t eLrChCfg = ZPAL_RADIO_LR_CH_CFG1;

    if (0 != g_nodeID)
    {
      if (zpal_radio_is_long_range_locked())
      {
        eLrChCfg = ZPAL_RADIO_LR_CH_CFG3;
      }
      else
      {
        eLrChCfg = ZPAL_RADIO_LR_CH_CFG_NO_LR;
      }
    }

    ProtocolChangeLrChannelConfig(eLrChCfg);
  }
}


/**
* Function called when PROTOCOL_EVENT_CHANGE_RF_PHY event has been triggered
*
*/
void
ProtocolEventChangeRfPHYdetected(void)
{
  // Setup TxQueue to inform when TxQueue is empty and try to change RF PHY
  TxQueueEmptyEvent_Add(&sTxQueueEmptyEvent_phyChange, ProtocolChangeRfPHY);
}

ZW_PROTOCOL_ADD_CMD_LR(ZWAVE_LR_CMD_NOP)
{
  ZW_HandleCmdNOP(pCmd, cmdLength, rxopt);
}

ZW_PROTOCOL_ADD_CMD_LR(ZWAVE_LR_CMD_ASSIGN_IDS)
{
  if ((g_findInProgress == false) && g_learnMode)
  {
    if ( cmdLength < sizeof(ASSIGN_IDS_FRAME_LR) )
    {
      return;
    }

    assignState = ELEARNSTATUS_ASSIGN_NODEID_DONE;

    /* set new home and node ID's */
    node_id_t newNodeID = ((ASSIGN_IDS_FRAME_LR*)pCmd)->newNodeID[0] << 8 | ((ASSIGN_IDS_FRAME_LR*)pCmd)->newNodeID[1];
    if ( (LOWEST_LONG_RANGE_NODE_ID > newNodeID) || (NODE_RESERVED_BEGIN_LR <= newNodeID) )
    {
      // Invalid node ID value (zero OR outside the range of valid Long Range Node IDs)
      return;
    }
    g_nodeID = newNodeID;
    DPRINTF("\r\nZWAVE_LR_CMD_ASSIGN_IDS: g_nodeID=0x%x, newNodeID=0x%x", g_nodeID, newNodeID);
    ZW_HomeIDSet(((ASSIGN_IDS_FRAME_LR*)pCmd)->newHomeID);

    /* store the ID's */
    HomeIdUpdate(ZW_HomeIDGet(), g_nodeID);
    /* Remove specific learnMode Receive filters */
    rfLearnModeFilter_Set(ZW_SET_LEARN_MODE_DISABLE, g_nodeID, ZW_HomeIDGet());

    zpal_radio_set_long_range_lock(true);

    /* report the new node ID to the application */
    ZCB_AssignComplete(0, TRANSMIT_COMPLETE_OK, NULL);
  }
}

ZW_PROTOCOL_ADD_CMD_LR(ZWAVE_LR_CMD_NODE_INFO)
{
  ZW_HandleCmdNodeInfo(pCmd, cmdLength, rxopt);
}

ZW_PROTOCOL_ADD_CMD_LR(ZWAVE_LR_CMD_REQUEST_NODE_INFO)
{
  ZW_HandleCmdRequestNodeInfo(pCmd, cmdLength, rxopt);
}

ZW_PROTOCOL_ADD_CMD_LR(ZWAVE_LR_CMD_EXCLUDE_REQUEST_CONFIRM)
{
  if (!g_learnMode)
  {
    return; /* We didn't request to leave the network */
  }

  if ( cmdLength < sizeof(ASSIGN_IDS_FRAME_LR) )
  {
    return; /* Command invalid (payload too small) */
  }

  /* Validate the Exclude Request Confirm command is for us */
  node_id_t nodeId = ((ASSIGN_IDS_FRAME_LR*)pCmd)->newNodeID[0] << 8 | ((ASSIGN_IDS_FRAME_LR*)pCmd)->newNodeID[1];
  if (g_nodeID != nodeId)
  {
    return; /* Not for us */
  }

  assignState = ELEARNSTATUS_ASSIGN_NODEID_DONE;

  /* Clear our home and node ID's */
  g_nodeID = 0;
  ZW_HomeIDClear();

  /* Store the ID's */
  HomeIdUpdate(ZW_HomeIDGet(), g_nodeID);

  /* Remove specific learnMode Receive filters */
  rfLearnModeFilter_Set(ZW_SET_LEARN_MODE_DISABLE, g_nodeID, ZW_HomeIDGet());

  zpal_radio_set_long_range_lock(false); /* Node has been excluded and therefore no longer Long Range locked */

#ifdef ZW_SECURITY_PROTOCOL
  sec2_inclusion_abort(); /* Make sure to stop S2 inclusion state machine */
  security_reset();
#endif /* ZW_SECURITY_PROTOCOL */

  /* Report the new node ID to the application */
  ZCB_AssignComplete(0, TRANSMIT_COMPLETE_OK, NULL);
}

ZW_PROTOCOL_ADD_CMD_LR(ZWAVE_LR_CMD_NON_SECURE_INCLUSION_COMPLETE)
{
  // Verify the frame length
  if ( cmdLength < sizeof(NON_SECURE_INCLUSION_COMPLETE_FRAME_LR) )
  {
    return;
  }
  DPRINT("\nNon secure inclusion part done. Start secure inclusion.\n");
  startSecureInclusion();
  // Set correct S2 inclusion state - 30sec timeout on KEX GET receive
  s2_inclusion_neighbor_discovery_complete(s2_ctx);
}

ZW_PROTOCOL_ADD_CMD(ZWAVE_CMD_TRANSFER_PRESENTATION)
{
  /* We only react if we are in learnMode */
  /* Do not react if we are in SMARTSTART */
  if (g_findInProgress || !g_learnMode || g_learnModeDsk)
  {
    return;
  }
  /* If the size of the received Transfer Presentation payload is bigger or equal to current definition */
  /* of the Transfer Presentation Frame then the payload contains the option bitmask */
  if ((cmdLength >= sizeof(TRANSFER_PRESENTATION_FRAME)) &&
      (((TRANSFER_PRESENTATION_FRAME*)pCmd)->option & TRANSFER_PRESENTATION_OPTION_UNIQ_HOMEID))
  {
    /* TO#7022 partial fix */
    if (((bNetworkWideInclusion == NETWORK_WIDE_MODE_JOIN) &&
          (((TRANSFER_PRESENTATION_FRAME*)pCmd)->option & TRANSFER_PRESENTATION_OPTION_NOT_INCLUSION))
        ||
        ((bNetworkWideInclusion == NETWORK_WIDE_MODE_LEAVE) &&
          (((TRANSFER_PRESENTATION_FRAME*)pCmd)->option & TRANSFER_PRESENTATION_OPTION_NOT_EXCLUSION)))
    {
      return;
    }
    const STransmitCallback TxCallbackNull = { .pCallback = NULL, .Context = 0 };
    /* TO#1964 partial possible fix. */
    forceUse40K = true;
    /* TO#2875 fix - Do not use LWR/Response Route or Routeing when answering TransferPresentation */
    ZW_SendNodeInformation(rxopt->sourceNode,
                           ((rxopt->rxStatus & RECEIVE_STATUS_LOW_POWER) ?
                           TRANSMIT_OPTION_LOW_POWER | TRANSMIT_OPTION_NO_ROUTE :
                           TRANSMIT_OPTION_NO_ROUTE),
                           &TxCallbackNull);
  }
}

ZW_PROTOCOL_ADD_CMD(ZWAVE_CMD_REQUEST_NODE_INFO)
{
  ZW_HandleCmdRequestNodeInfo(pCmd, cmdLength, rxopt);
}

ZW_PROTOCOL_ADD_CMD(ZWAVE_CMD_ASSIGN_IDS)
{
  ZW_HandleCmdAssignIDs(pCmd, cmdLength, rxopt);
}

#ifdef ZW_SELF_HEAL
ZW_PROTOCOL_ADD_CMD(ZWAVE_CMD_LOST)
{
  /* Here we must also be able to determine if the node itself is in distress so */
  /* that we can ignore the request if that is so */
  /* Only listening slaves can be of any help */
  if (0 != ZW_nodeIsListening())
  {
    /*If we have a SUC in network then this is used. If not we try the primary controller*/
    uint8_t DestinationNode = NODE_CONTROLLER; // TODO - What to do when 0xEF disappears???;

    if (g_sucNodeID)
    {
      DestinationNode = g_sucNodeID;
    }

    helpNodeID = ((LOST_FRAME*)pCmd)->nodeID; /*Store nodeID so result can be returned*/
    /*try to pass message to SUC or Primary controller Node*/
    LOST_FRAME  lostFrame;
    lostFrame.cmdClass = ZWAVE_CMD_CLASS_PROTOCOL;
    lostFrame.cmd = ZWAVE_CMD_LOST;
    lostFrame.nodeID = helpNodeID;

    const STransmitCallback TxCallback = { .pCallback = ZCB_CtrlAskedForHelpCallback, .Context = 0 };
    EnQueueSingleData(RF_SPEED_9_6K, g_nodeID, DestinationNode, (uint8_t *)&lostFrame, sizeof(LOST_FRAME),
                      (TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE),
                      0, // 0ms for tx-delay (any value)
                      ZPAL_RADIO_TX_POWER_DEFAULT,
                      &TxCallback);
  }
}
#endif /*ZW_SELF_HEAL*/

ZW_PROTOCOL_ADD_CMD(ZWAVE_CMD_ASSIGN_RETURN_ROUTE)
{
  /* TODO: Buffer boundary checks needs to be enforced. FACT - No check is done for number of repeaters. */
  /* TODO: Start using the second, short return route laid out in NVM (ZW040 routing slaves only) */
  uint32_t NodeId;
  uint8_t repeaterIndex;
  uint32_t destinationIndex;

  DPRINT("Assign ReturnRoute\n");

  bool isFirstRoute = false;
  destinationIndex = 0xff;
  const uint8_t retRouteNo = (((ASSIGN_RETURN_ROUTE_FRAME*)pCmd)->routeNoNumHops >> 4) & 0x7;
  if (!retRouteNo) /* If routeNo is ZERO then set nodeID */
  {
    curNodeID = ((ASSIGN_RETURN_ROUTE_FRAME*)pCmd)->nodeID;
    /* New Routing Scheme - Direct can be in any assigned route */
    /* Indicate first route */
    isFirstRoute = true;
  }
  if (curNodeID)  /* nodeID NOT ZERO ? */
  {
    /* Update ZW_RouteDestinations structure */
    uint32_t i = 0;
    while ((i < sizeof(ZW_RouteDestinations)) &&
            (ZW_RouteDestinations[i] != curNodeID))
    {
      if (!ZW_RouteDestinations[i] && (destinationIndex == 0xff))
      {
        destinationIndex = i;
      }
      i++;
    }
    if ((i >= sizeof(ZW_RouteDestinations)) && (destinationIndex < 0xff))
    {
      ZW_RouteDestinations[destinationIndex] = curNodeID;
//        DPRINTF("Route destination dst %02X\n", curNodeID);
      SlaveStorageSetRouteDestinations(ZW_RouteDestinations);
    }
    /* Return Route structure is Zero based */
    NodeId = curNodeID - 1;
    if (retRouteNo < RETURN_ROUTE_MAX)
    {
      if (isFirstRoute)
      {
        /* Set the routing priority to default */
        ReturnRouteClearPriority(curNodeID);
      }
//        DPRINT("Get ReturnRoute\n");
      NVM_RETURN_ROUTE_STRUCT t_ReturnRoute = { 0 };
      SlaveStorageGetReturnRoute(NodeId + 1, &t_ReturnRoute);

      /* NodeId should now contain the dest. route struct number */
      /* TO#2394 fix  SSE maybe this comment should be removed*/
      if (isFirstRoute)
      {
        /* Only save if first Route */
        /* Save the current nodeID to indicate that this route struct is taken */
        t_ReturnRoute.nodeID = curNodeID;
      }
      /* RepeaterHops contains the number of hops in the repeater list */
      uint8_t RepeaterHops = ((ASSIGN_RETURN_ROUTE_FRAME*)pCmd)->routeNoNumHops & 0x7;

      repeaterIndex = 0;
      if (isFirstRoute && !RepeaterHops)
      {
        /* New Routing Scheme - First route and direct */
        t_ReturnRoute.routeList[retRouteNo].repeaterList[repeaterIndex] = CACHED_ROUTE_LINE_DIRECT;
        repeaterIndex++;
      }
      for (; repeaterIndex < MAX_REPEATERS; repeaterIndex++)
      {
        /* Save the repeater in list */
        if (repeaterIndex < RepeaterHops)
          t_ReturnRoute.routeList[retRouteNo].repeaterList[repeaterIndex] = ((ASSIGN_RETURN_ROUTE_FRAME*)pCmd)->repeaterList[repeaterIndex];
        else
          t_ReturnRoute.routeList[retRouteNo].repeaterList[repeaterIndex] = 0;
      }
//        DPRINTF("Set ReturnRoute %02X\n", curNodeID);
      NVM_RETURN_ROUTE_SPEED t_ReturnRouteSpeed = { 0 };
      CalculateReturnRouteSpeed((ASSIGN_RETURN_ROUTE_FRAME*)pCmd, curNodeID, cmdLength, &t_ReturnRouteSpeed);
      SlaveStorageSetReturnRoute(NodeId +1, &t_ReturnRoute, &t_ReturnRouteSpeed);
      return;
    }
  }
  else /* if (curNodeID)  nodeID ZERO ? */
  {
    /* if ZERO nodeID then delete all routes */
    if (isFirstRoute)
    {
      SlaveStorageDeleteAllReturnRouteInfo();
      RemoveAllReturnRouteDestination();
    }
  }
  if (SUC_Update.updateState != SUC_IDLE)
  {
    NetWorkTimerStart(&SUC_Update.TimeoutTimer, ZCB_RequestUpdateTimeout, SUC_REQUEST_UPDATE_TIMEOUT);
  }
}

ZW_PROTOCOL_ADD_CMD(ZWAVE_CMD_ASSIGN_SUC_RETURN_ROUTE)
{
  /* TODO: Buffer boundary checks needs to be enforced. FACT - No check is done for number of repeaters. */
  const uint8_t retRouteNo = (((ASSIGN_RETURN_ROUTE_FRAME*)pCmd)->routeNoNumHops >> 4) & 0x7;

  // get number of hops
  uint8_t HopCount = ((ASSIGN_RETURN_ROUTE_FRAME*)pCmd)->routeNoNumHops & 0x7;
  /* If routeNo is ZERO then set nodeID */
  NVM_RETURN_ROUTE_STRUCT t_ReturnRoute = { 0 };
  SlaveStorageGetReturnRoute(0, &t_ReturnRoute);

  if (!retRouteNo)
  {
    /* Set the routing priority to default */
    ReturnRouteClearPriority(RETURN_ROUTE_PRIORITY_SUC);

    SlaveSetSucNodeId(((ASSIGN_RETURN_ROUTE_FRAME*)pCmd)->nodeID);

    t_ReturnRoute.nodeID = g_sucNodeID;
  }

  if (retRouteNo < RETURN_ROUTE_MAX)
  {
    /* Save the static SUC return route */
    /* Only first Route can be direct. */
    /* HopCount contains repeaterCount. */

    if ((!HopCount) && (!retRouteNo))
    {
      /* First route is direct */
        t_ReturnRoute.routeList[0].repeaterList[0] = CACHED_ROUTE_LINE_DIRECT;
    }
    else
    {
      for (uint32_t i = 0; i < MAX_REPEATERS; i++)
      {
        if (i < HopCount)
          t_ReturnRoute.routeList[retRouteNo].repeaterList[i] = ((ASSIGN_RETURN_ROUTE_FRAME*)pCmd)->repeaterList[i];
        else
          t_ReturnRoute.routeList[retRouteNo].repeaterList[i] = 0;
      }
    }

    if (g_sucNodeID)
    {
      NVM_RETURN_ROUTE_SPEED t_ReturnRouteSpeed = { 0 };
      CalculateReturnRouteSpeed((ASSIGN_RETURN_ROUTE_FRAME*)pCmd, 0, cmdLength, &t_ReturnRouteSpeed);
      SlaveStorageSetReturnRoute(0, &t_ReturnRoute, &t_ReturnRouteSpeed);
    }
    else
    {
      SlaveStorageDeleteSucReturnRouteInfo();
    }
  }
  /* Suc route are being assigned. Give time to finish */
  if(assignState != ELEARNSTATUS_ASSIGN_COMPLETE)
  {
    AssignTimerStart(1);
  }
}

#ifndef ONE_ROUTE_TO_DESTINATION
ZW_PROTOCOL_ADD_CMD(ZWAVE_CMD_ASSIGN_RETURN_ROUTE_PRIORITY)
{
  const uint8_t retRouteNo = ((ASSIGN_RETURN_ROUTE_PRIORITY_FRAME*)pCmd)->routeNumber;
  curNodeID = ((ASSIGN_RETURN_ROUTE_PRIORITY_FRAME*)pCmd)->nodeID;

  if (curNodeID)  /* nodeID NOT ZERO ? */
  {
    /* Clear the response route */
    NVM_RETURN_ROUTE_STRUCT t_ReturnRoute = { 0 };
    SlaveStorageGetReturnRoute(curNodeID, &t_ReturnRoute);

    RemoveResponseRoute(curNodeID);
    uint8_t StoredNodeID = t_ReturnRoute.nodeID;

    if (curNodeID == StoredNodeID)
    {
      NVM_RETURN_ROUTE_SPEED   t_ReturnRouteSpeed = { 0 };
      SlaveStorageGetReturnRouteSpeed(curNodeID, &t_ReturnRouteSpeed);
      /* Destination found, set first priority route */
      /* Get current beam info */
      t_ReturnRouteSpeed.speed.bytes[1] &= NVM_RETURN_ROUTE_BEAM_DEST_MASK;
      /* Set priority route in the byte */
      t_ReturnRouteSpeed.speed.bytes[1] |= ((retRouteNo+1) <<2);
      //Save it again
      SlaveStorageSetReturnRouteSpeed(curNodeID, &t_ReturnRouteSpeed);
    }
  }
}

ZW_PROTOCOL_ADD_CMD(ZWAVE_CMD_ASSIGN_SUC_RETURN_ROUTE_PRIORITY)
{
  const uint8_t retRouteNo = ((ASSIGN_RETURN_ROUTE_PRIORITY_FRAME*)pCmd)->routeNumber;
  curNodeID = ((ASSIGN_RETURN_ROUTE_PRIORITY_FRAME*)pCmd)->nodeID;

  /* TO#6940 fix - retRouteNo must be less than RETURN_ROUTE_MAX */
  NVM_RETURN_ROUTE_STRUCT t_ReturnRoute = { 0 };
  SlaveStorageGetReturnRoute(0, &t_ReturnRoute);
  uint8_t TempSucNodeID = t_ReturnRoute.nodeID;

  if ((curNodeID == TempSucNodeID) &&
      (retRouteNo < RETURN_ROUTE_MAX))
  {
    /* Clear the response route */
    RemoveResponseRoute(curNodeID);
    /* Destination found, set as first priority route */
    /* Get current beam info */
    NVM_RETURN_ROUTE_SPEED  t_ReturnRouteSpeed = { 0 };
    SlaveStorageGetReturnRouteSpeed(0, &t_ReturnRouteSpeed);
    t_ReturnRouteSpeed.speed.bytes[1] &= NVM_RETURN_ROUTE_BEAM_DEST_MASK;
    /* Set priority route in the byte */
    t_ReturnRouteSpeed.speed.bytes[1] |= ((retRouteNo+1) <<2);
    //Save it again
    SlaveStorageSetReturnRouteSpeed(0, &t_ReturnRouteSpeed);
  }
}
#endif /* #ifndef ONE_ROUTE_TO_DESTINATION */

ZW_PROTOCOL_ADD_CMD(ZWAVE_CMD_TRANSFER_END)
{
  if (SUC_Update.updateState != SUC_IDLE)
  {
    ResetSUC();
    assignState = ELEARNSTATUS_ASSIGN_COMPLETE;
    AssignTimerStop();
    ReloadRouteDestinations();

    ZW_TransmitCallbackInvoke(&routeUpdateDone, pCmd[2] - ZWAVE_APPLICATION_VAL_OFFSET, 0);
  }
}

ZW_PROTOCOL_ADD_CMD(ZWAVE_CMD_FIND_NODES_IN_RANGE)
{
  if ((g_findInProgress == false)
      && (cmdLength <= sizeof(FIND_NODES_FRAME))
      && (cmdLength > (sizeof(FIND_NODES_FRAME) - (MAX_NODEMASK_LENGTH) - (2 * sizeof(uint8_t))))
      && ((*(FIND_NODES_FRAME*)pCmd).numMaskBytes <= (MAX_NODEMASK_LENGTH)))
  {
    /* save the speed of the node that sent the request */
    oldSpeed = TransportGetCurrentRxSpeed();
    /* Application should know that it must not abort receive mode */
    assignState = ELEARNSTATUS_ASSIGN_RANGE_INFO_UPDATE;
    ZCB_AssignComplete(0, TRANSMIT_COMPLETE_OK, NULL);

    g_findInProgress = true;
    if (SUC_Update.updateState == SUC_WAITING_FOR_NEIGHBOR_REQUEST)
    {
      /* We received a suc neighbor discovery. */
      NetWorkTimerStop(&SUC_Update.TimeoutTimer);
    }
    /* Lock route so we can transmit routed to the node that requested the update */
    ZW_LockRoute(rxopt->sourceNode);

    /* Set neighbor control vars to default values */
    zensorWakeupTime = 0;
    bNeighborDiscoverySpeed = (1 == llIsHeaderFormat3ch()) ? ZWAVE_FIND_NODES_SPEED_100K : ZWAVE_FIND_NODES_SPEED_9600;

    /* TO#1318 - fix - pCmd must be copied into nodeRange before nodeRange can be used */
    /* Do the findNeighbor frame contain zensorWakeupTime - then we are checking for sensors */
    if (cmdLength > (sizeof(FIND_NODES_FRAME) - 2 * sizeof(uint8_t) - (MAX_NODEMASK_LENGTH)) + ((*(FIND_NODES_FRAME*)pCmd).numMaskBytes & ZWAVE_FIND_NODES_NUMMASKBYTES_MASK))
    {
      /* The frame contains zensor wakeup speed */
      zensorWakeupTime = (*(FIND_NODES_FRAME*)pCmd).maskBytes[(*(FIND_NODES_FRAME*)pCmd).numMaskBytes];
    }
    pNodeRangeFrame = (0 == zensorWakeupTime) ? &nodeNormalRangeFrame : &nodeFLiRSRangeFrame;

    /* Copy range request to buffer */
    memcpy((uint8_t *)pNodeRangeFrame, (uint8_t *)pCmd, cmdLength);

    if (cmdLength > (sizeof(FIND_NODES_FRAME) - sizeof(uint8_t) - (MAX_NODEMASK_LENGTH)) + ((*(FIND_NODES_FRAME*)pCmd).numMaskBytes & ZWAVE_FIND_NODES_NUMMASKBYTES_MASK))
    {
      /* The frame contains neighbor discovery speed */
      bNeighborDiscoverySpeed = (*(FIND_NODES_FRAME*)pCmd).maskBytes[((*(FIND_NODES_FRAME*)pCmd).numMaskBytes & ZWAVE_FIND_NODES_NUMMASKBYTES_MASK) + 1];
    }

    nodeRange.lastController = rxopt->sourceNode;
    nodeRange.txOptions = TRANSMIT_OPTION_ACK;
    if (rxopt->rxStatus & RECEIVE_STATUS_LOW_POWER)
    {
      nodeRange.txOptions |= TRANSMIT_OPTION_LOW_POWER;
    }

    FindNeighbors(0);
  }
}

ZW_PROTOCOL_ADD_CMD(ZWAVE_CMD_GET_NODES_IN_RANGE)
{
  /* Delay done because we might receive another find nodes in range frame */
  assignState = ELEARNSTATUS_ASSIGN_WAITING_FOR_FIND;

  AssignTimerStop();

  /* Set command to range info */
  /* Which rangeinfo is requested */
  zensorWakeupTime = 0;
  if (cmdLength > (sizeof(GET_NODE_RANGE_FRAME) - sizeof(uint8_t)))
  {
    zensorWakeupTime = (*(GET_NODE_RANGE_FRAME*)pCmd).zensorWakeupTime;
  }
  pNodeRangeFrame = (0 == zensorWakeupTime) ? &nodeNormalRangeFrame : &nodeFLiRSRangeFrame;

  /* Set command to range info */
  (*pNodeRangeFrame).cmdClass = ZWAVE_CMD_CLASS_PROTOCOL;
  (*pNodeRangeFrame).cmd = ZWAVE_CMD_RANGE_INFO;

  /* transmit range Info frame */
  TxOptions_t txOptions = TRANSMIT_OPTION_ACK;
  if (rxopt->rxStatus & RECEIVE_STATUS_LOW_POWER)
  {
    txOptions |= TRANSMIT_OPTION_LOW_POWER;
  }

  const STransmitCallback TxCallback = { .pCallback = ZCB_AssignComplete, .Context = 0 };
  /* 0 bytes in nodemask is an invalid size */
  if ((*pNodeRangeFrame).numMaskBytes == 0)
  {
    (*pNodeRangeFrame).numMaskBytes = 1;
  }
  (*pNodeRangeFrame).maskBytes[(*pNodeRangeFrame).numMaskBytes] = zensorWakeupTime;
  /* TODO - What to do if transmitBuffer full - handled as failed transmission */
  uint8_t len = (*pNodeRangeFrame).numMaskBytes + sizeof(RANGEINFO_FRAME) - (MAX_NODEMASK_LENGTH);
  if (!EnQueueSingleData(false, g_nodeID, rxopt->sourceNode, (uint8_t *)pNodeRangeFrame,
                          len,
                          txOptions, 0, // 0ms for tx-delay (any value)
                          ZPAL_RADIO_TX_POWER_DEFAULT, &TxCallback))
  {
    ZCB_AssignComplete(0, TRANSMIT_COMPLETE_FAIL, NULL);
  }
}

ZW_PROTOCOL_ADD_CMD(ZWAVE_CMD_NODE_INFO)
{
  ZW_HandleCmdNodeInfo(pCmd, cmdLength, rxopt);
}

ZW_PROTOCOL_ADD_CMD(ZWAVE_CMD_SET_NWI_MODE)
{
  /* TODO - What if we are in LearnMode - do we get this then??? - it could change our learnMode state from NWI to not */
  if (!g_learnMode && (NETWORK_WIDE_MODE_IDLE == bNetworkWideInclusion))
  {
    /* A FLiRS must never be used as an explore frame repeater Is the routing/enhanced slave registered as an FLiRS */
    if (0 == ZW_nodeIsFLiRS())
    {
      ExploreSetNWI(((SET_NWI_MODE_FRAME*)pCmd)->mode);
    }
    else
    {
      ExploreSetNWI(false);
    }
  }
}

ZW_PROTOCOL_ADD_CMD(ZWAVE_CMD_NOP_POWER)
{
  DPRINTF("ZWAVE_CMD_NOP_POWER - g_learnMode:%02X, assignState:%02X\r\n", g_learnMode, assignState);
  if (!g_learnMode && (ELEARNSTATUS_ASSIGN_COMPLETE == assignState))
  {
    /* We got a NOP_POWER frame so we just send it upstairs */
    ProtocolInterfacePassToAppNodeUpdate(
      UPDATE_STATE_NOP_POWER_RECEIVED,
      rxopt->sourceNode,
      NULL,
      0
    );
  }
}

void CommandHandler(CommandHandler_arg_t * pArgs)
{
  uint8_t * pCmd = pArgs->cmd;
  uint8_t cmdLength = pArgs->cmdLength;
  RECEIVE_OPTIONS_TYPE * rxopt = pArgs->rxOpt;

  uint8_t rxStatus;     /* IN  Frame header info */
  // node_id_t destNode;     /* To  whom it might concern - which node is to receive the frame */
  node_id_t sourceNode;   /* IN  Command sender Node ID */

  rxStatus = rxopt->rxStatus;
  sourceNode = rxopt->sourceNode;
  // destNode = rxopt->destNode;

#ifdef DEBUGPRINT
  DPRINTF(".%02X", cmdLength);
  {
	char pCharBuf[255];

	tfp_sprintf(pCharBuf, "RX(%02X) %02X", cmdLength, pCmd[0]);
	for (int j = 1; j < cmdLength; j++)
	{
	  tfp_sprintf(&pCharBuf[strlen(pCharBuf)], " %02X", pCmd[j]);
	}
	DPRINTF("%s\n", pCharBuf);
  }
#endif

  switch (*pCmd)
  {
#ifdef ZW_SECURITY_PROTOCOL
    case COMMAND_CLASS_SECURITY_2:
      Security2CommandHandler((uint8_t*) pCmd, (uint8_t) cmdLength, rxopt);
      break;
    case COMMAND_CLASS_SECURITY:
      if (false == zpal_radio_is_long_range_locked())
      {
        SecurityCommandHandler(pCmd, cmdLength, rxopt);
      }
      break;

#ifdef USE_TRANSPORT_SERVICE
    case COMMAND_CLASS_TRANSPORT_SERVICE_V2:
      memset((uint8_t*)&sTsParam, 0, sizeof(sTsParam));
      sTsParam.snode = sourceNode;
      sTsParam.dnode = g_nodeID;    /* Destination is always our own nodeid on non-bridge libraries */
      sTsParam.rx_flags = rxStatus;
      sTsParam.tx_flags = TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE | TRANSMIT_OPTION_EXPLORE;
      TransportService_ApplicationCommandHandler(&sTsParam, pCmd, cmdLength);
      break;
#endif /* USE_TRANSPORT_SERVICE */
#endif /* ZW_SECURITY_PROTOCOL */
    case ZWAVE_CMD_NOP:
      ZW_HandleCmdNOP(pCmd, cmdLength, rxopt);
      break;
    case ZWAVE_CMD_CLASS_PROTOCOL:
      /* Z-Wave protocol command */
      zw_protocol_invoke_cmd_handler(pCmd, cmdLength, rxopt);
      break;

    case ZWAVE_CMD_CLASS_PROTOCOL_LR:
      /* Z-Wave Long Range protocol command */
      zw_protocol_invoke_cmd_handler_lr(pCmd, cmdLength, rxopt);
      break;

    default:
      /* Unencrypted application frames are not allowed in Long Range */
      if (IS_APPL_CLASS(*pCmd) && (cmdLength <= MAX_SINGLECAST_PAYLOAD) && (false == zpal_radio_is_long_range_locked()))
      { /* Application layer command */
        ProtocolInterfacePassToAppSingleFrame(
                                              cmdLength,
                                              (ZW_APPLICATION_TX_BUFFER*)pCmd,
                                              rxopt
                                             );
      }
      break;
  }
}

/*=================   ZW_RequestNetworkUpdateInternal   ======================
**    Request Network Update from the Static Update Controller
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
uint8_t
ZW_RequestNetworkUpdateInternal(
  uint8_t force,
  const STransmitCallback* pTxCallback) /* call back function indicates of the update suceeded or failed*/
{
  uint8_t frameSize = sizeof(STATIC_ROUTE_REQUEST_FRAME);
  if (!g_sucNodeID || (SUC_Update.updateState != SUC_IDLE))
  {
    return false;
  }

  SUC_Update.updateState = SUC_UPDATE_STATE_WAITING;
  routeUpdateDone = *pTxCallback;
  typedef union _FRAME_BUFFER_
  {
    STATIC_ROUTE_REQUEST_FRAME reqRouteUpdate;
    FORCE_STATIC_ROUTE_REQUEST_FRAME reqRouteUpdateForce; /*To make sure that there is room for a Forced Update*/
  } FRAME_BUFFER;

  FRAME_BUFFER frameBuffer = {
    .reqRouteUpdate.cmdClass = ZWAVE_CMD_CLASS_PROTOCOL,
    .reqRouteUpdate.cmd = ZWAVE_CMD_STATIC_ROUTE_REQUEST
  };
  memcpy((uint8_t *)frameBuffer.reqRouteUpdate.desNodeIDList, (uint8_t *)ZW_RouteDestinations, sizeof(ZW_RouteDestinations));
  if (force)
  {
    /* If force then add one byte at the end of destList. */
    /* This will force controller to delete routes before assigning them */
    frameBuffer.reqRouteUpdateForce.force = 0xff;
    frameSize++;
  }
  const STransmitCallback TxCallback = { .pCallback = ZCB_ReqUpdateCallback, .Context = 0 };
  if (!EnQueueSingleData(ChooseSpeedForDestination_slave(g_sucNodeID), g_nodeID, g_sucNodeID, (uint8_t *)&frameBuffer.reqRouteUpdate,
                         frameSize,
                         (TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE),
                         0, // 0ms for tx-delay (any value)
                         ZPAL_RADIO_TX_POWER_DEFAULT,
                         &TxCallback))
  {
    SUC_Update.updateState = SUC_IDLE;
    ZW_TransmitCallbackUnBind(&routeUpdateDone);
    return false;
  }
  return true;
}

/*----------------------------------------------------------------------
 This function request network update from the Static Update Controller
------------------------------------------------------------------------*/
uint8_t                      /* RET:  True; SUC is known to the controller, false; SUC not known to the controller*/
ZW_RequestNetWorkUpdate(
  const STransmitCallback* pTxCallback) /* call back function indicates of the update sucessed or failed*/
{
  ReloadRouteDestinations();
  return ZW_RequestNetworkUpdateInternal(false, pTxCallback);
}


/*======================  ZW_IsNodeWithinDirectRange   ======================
**    Test if ReturnRouted indicate that bNodeID is within direct range.
**
**--------------------------------------------------------------------------*/
bool                          /*RET true if neighbours, false if not*/
ZW_IsNodeWithinDirectRange(
  node_id_t nodeID)           /*IN nodeID to check*/
{
  if (nodeID == 0)
  {
    return false;
  }

  /* ReturnRouteIndex now points to Return Route file for bNodeID */
  uint32_t ReturnRouteIndex = nodeID - 1;

  /* If dest == ZERO then no ReturnRoutes exist for this destination */
  NVM_RETURN_ROUTE_STRUCT t_ReturnRoute = { 0 };
  SlaveStorageGetReturnRoute(ReturnRouteIndex + 1, &t_ReturnRoute);

  if (t_ReturnRoute.routeList[0].repeaterList[0] != 0) /* [0] is SUC */
  {
    /* ReturnRouteIndex now points to first repeater in first physical route (no priority involved) */
    for (uint32_t i = 0; i < RETURN_ROUTE_MAX; i++)
    {
      if (t_ReturnRoute.routeList[i].repeaterList[0]  == CACHED_ROUTE_LINE_DIRECT) /* [0] is SUC */
      {
        /* If first byte in route is CACHED_ROUTE_LINE_DIRECT then it is direct */
        return true;
      }
    }
  }
  return false;
}


/*=======================   ZW_RequestNewRouteDestinations   =================
**    Request new destinations for return routes.
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
bool                                               /*RET true if SUC/SIS exist false if not*/
ZW_RequestNewRouteDestinations(
  uint8_t* destList,                                 /* IN Pointer to new destinations*/
  uint8_t destListLen,                                /* IN len of buffer */
  const STransmitCallback* pTxCallback)   /* IN callback function called when completed*/
{
  uint8_t i = 0;

  if (!g_sucNodeID || (SUC_Update.updateState != SUC_IDLE))
  {
    return false;
  }
  for (i = 0; i < sizeof(ZW_RouteDestinations); i++)
  {
    ZW_RouteDestinations[i] = (i < destListLen) ? destList[i] : 0;
  }
  SlaveStorageSetRouteDestinations(ZW_RouteDestinations);
  return ZW_RequestNetworkUpdateInternal(true, pTxCallback);
}


/*===========================   SetDefault   ================================
**    Remove all Nodes and timers from the EEPROM memory.
**    Reset the homeID and nodeID
**    Side effects:
**
**--------------------------------------------------------------------------*/
void           /*RET  Nothing        */
ZW_SetDefault(void)
{
  SlaveInit(m_pAppNodeInfo);

  /* clear the home id and the node id. */
  ZW_HomeIDClear();

  HomeIdGeneratorGetNewId(ZW_HomeIDGet());  // Generate a random HomeID
  HomeIdUpdate(ZW_HomeIDGet(), 0);

  zpal_radio_set_long_range_lock(false);

  /* clear the tables */
  ResetRoutedResponseRoutes();

  /* A complete NVM erase will remove PSA persistent keys.
   * Walk through slave files by ID and delete them individually.
   */
  DeleteSlaveNetworkInfoStorage();

  ProtocolInterfaceReset();

  SlaveInitDone(m_initRegion);

  ProtocolChangeRfPHY();

  /* TO#2390 - node could hang if TxQueue wasnt empty */
  TxQueueInit();
#ifdef ZW_SECURITY_PROTOCOL
  security_reset();
#endif /* ZW_SECURITY_PROTOCOL */

  ZW_EnableRoutedRssiFeedback(true);
  NoiseDetectInit();
}


/* TODO - move to a new module shared with the controller libraries */
/*============================   ZW_RequestNodeInfo   ======================
**
**  Function description.
**   Request a node to send it's node information.
**   Function return true if the request is send, else it return false.
**   TxCallback is a callback function, which is called with the status
**   of the Request nodeinformation frame transmission.
**   If a node sends its node info, an SReceiveNodeUpdate will be passed to app
**   with UPDATE_STATE_NODE_INFO_RECEIVED as status together with the received
**   nodeinformation.
**
**  Side effects:
**
**--------------------------------------------------------------------------*/
bool                /*RET true if primary controller, false if slave ctrl */
ZW_RequestNodeInfo(
  node_id_t nodeID,     /* IN Node id of the node to request node info from.*/
  const STransmitCallback* pTxCallback)
{
  REQ_NODE_INFO_FRAME request_nif = { 0 };
  if (LOWEST_LONG_RANGE_NODE_ID <= nodeID)
  {
    /* Destination node is a Long Range node */
    request_nif.cmdClass = ZWAVE_CMD_CLASS_PROTOCOL_LR;
    request_nif.cmd = ZWAVE_LR_CMD_REQUEST_NODE_INFO;
    bUseExploreAsRouteResolution = false; // Long Range doesn't do explore
  }
  else
  {
    request_nif.cmdClass = ZWAVE_CMD_CLASS_PROTOCOL;
    request_nif.cmd = ZWAVE_CMD_REQUEST_NODE_INFO;
    /* Note that we want Explore tried as route resolution if all else fails  */
    bUseExploreAsRouteResolution = true;
  }

  return(EnQueueSingleData(false, g_nodeID, nodeID,
                           (uint8_t *)&request_nif, sizeof(REQ_NODE_INFO_FRAME),
                           (TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE),
                           0, // 0ms for tx-delay (any value)
                           ZPAL_RADIO_TX_POWER_DEFAULT,
                           pTxCallback));
}


#ifdef USE_TRANSPORT_SERVICE
void TransportService_msg_received_event(unsigned char* pCmd, unsigned char cmdLen,  unsigned char srcNode)
{
    ts_rxOpt.rxStatus = RECEIVE_STATUS_TYPE_SINGLE; /* 0 */
    ts_rxOpt.sourceNode = srcNode;
    ts_rxOpt.destNode = g_nodeID;
    ts_rxOpt.rxRSSIVal = 0; /* TODO: RSSI not supported for TS yet */
    ts_rxOpt.securityKey = SECURITY_KEY_NONE; /* TS MUST always be outside security encap */

    CommandHandler_arg_t args = {
      .cmd = pCmd,
      .cmdLength = cmdLen,
      .rxOpt = &ts_rxOpt
    };
    CommandHandler(&args);
}
#endif /* USE_TRANSPORT_SERVICE */

/**
 * @brief This Function return the LR channel configuration that should be used by the PHY layer.
 * If the configuration given by this function is not the one available in RfProfile, that mean
 * that the PHY layer should be reconfigured.
 * @param[in] pRfProfile Pointer on the radio profile to use for PHY configuration.
 * @return               Returns the long range channel configuration to apply
 */
zpal_radio_lr_channel_config_t ZW_LrChannelConfigToUse(const zpal_radio_profile_t * pRfProfile)
{
  if (NULL == pRfProfile)
  {
    assert(false);
    return ZPAL_RADIO_LR_CH_CFG_COUNT;
  }
  else if ( true == zpal_radio_region_is_long_range(pRfProfile->region) )
  {
    if (ZPAL_RADIO_LR_CH_CFG3 == pRfProfile->active_lr_channel_config)
    {
      //if already in channel configuration 3, stay in channel configuration 3.
      //behavior of RadioLongRangeRegionNeeded (used before 24Q2 refactoring)
      //used at least in case of LR smart start inclusion
      return ZPAL_RADIO_LR_CH_CFG3;
    }
    else if (0 == g_nodeID)
    {
      //end device not included, so it must use both classic and LR channels
      return ZPAL_RADIO_LR_CH_CFG1;
    }
    else if (g_nodeID >= LOWEST_LONG_RANGE_NODE_ID)
    {
      //end device included in Long Range network, use only long range channels
      return ZPAL_RADIO_LR_CH_CFG3;
    }
    else
    {
      //end device included in classic Z-Wave network, do not use long range channels
      return ZPAL_RADIO_LR_CH_CFG_NO_LR;
    }
  }
  else
  {
    //Product's region does not support long range, disable long range channels
    return ZPAL_RADIO_LR_CH_CFG_NO_LR;
  }
}

void SlaveSetSucNodeId(uint8_t SucNodeId)
{
  g_sucNodeID = SucNodeId;
  SyncEventArg1Invoke(&g_SlaveSucNodeIdChanged, SucNodeId);
}


void HomeIdUpdate(uint8_t HomeIdArg[HOMEID_LENGTH], node_id_t NodeId)
{
  SlaveStorageSetNetworkIds(HomeIdArg, NodeId);

  SyncEventArg2Invoke(&g_SlaveHomeIdChanged, *((uint32_t*)HomeIdArg), NodeId);
  NetworkIdUpdateValidateNotify();
}

bool NodeSupportsBeingIncludedAsLR(void)
{
  return zpal_radio_protocol_mode_supports_long_range(zpal_radio_get_protocol_mode()) ? true : false;
}
