// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file ZW_explore.c
 * @copyright 2022 Silicon Laboratories Inc.
 *
 * @brief Z-Wave explore frame specific functions.
 */
#include "ZW_lib_defines.h"

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/

#include <ZW_basis_api.h>
#include <ZW_basis.h>
#include "ZW_timer.h"
#include "SwTimer.h"
#include "TickTime.h"
#include "Min2Max2.h"
#include "zpal_entropy.h"
//#define DEBUGPRINT
#include "DebugPrint.h"

#ifdef ZW_SLAVE
#include <ZW_slave.h>
#include <ZW_routing_all.h>
#ifdef ZW_BEAM_RX_WAKEUP
#include <ZW_basis_api.h>
#endif
#endif  /*ZW_SLAVE*/

#ifdef ZW_CONTROLLER
#include <ZW_controller.h>
#include <ZW_routing_cache.h>
#include <ZW_replication.h>
#endif /*ZW_CONTROLLER*/

/* Commmon for all targets */
#include <ZW_explore.h>
#include <ZW_event.h>
#include <ZW_transport.h>
#include <ZW_MAC.h>
#include <ZW_protocol_interface.h>
#include <string.h>
#include <ZW_DataLinkLayer.h>

/****************************************************************************/
/*                      PRIVATE TYPES and DEFINITIONS                       */
/****************************************************************************/
enum
{
/* Nothing special */
  EXPLORE_EVENT_IDLE = EVENT_IDLE,
/* exploreQueue element timed out  */
  EXPLORE_EVENT_TIMEOUT,
/* Search Result Stop explore frame transmitted */
  EXPLORE_EVENT_SEARCH_RESULT_TRANSMITTED,
/* Search Result Stop explore frame received */
  EXPLORE_EVENT_SEARCH_RESULT_RECEIVED,
/* Explore frame physically transmitted */
  EXPLORE_EVENT_FRAME_TRANSMITTED,
/* Timeout on receiving search result */
  EXPLORE_EVENT_TRANSMIT_TIMEOUT,
/*Explore frame physically failed to be sent*/
  EXPLORE_EVENT_TRANSMIT_FAILED,

};

#define EXPLORE_FRAME_IGNORE_SIZE       2

/* How long must specific nodeID-sequence number ident be ignored */
/* before removed. In 2msec ticks */
/* Default setting 4000 msec */
#define EXPLORE_FRAME_IGNORE_TIMEOUT    3600
/* How long must a STOP be active before removed. In 2msec ticks */
/* Default setting 6000 msec */
#define EXPLORE_FRAME_STOP_TIMEOUT      6000

typedef struct _exploreFrameIgnore_
{
  uint8_t id;
  uint8_t handle;
  uint32_t ignoreTickTime;  // Tick time when packet has timed out
  bool bIsActive;           // True if entry is active
} exploreFrameIgnore;

typedef union _EXPLORE_REQUEST_FRAME_
{
  EXPLORE_REMOTE_INCLUSION_REQUEST_FRAME inclusionRequest;
  EXPLORE_REMOTE_EXCLUSION_REQUEST_FRAME exclusionRequest;
  EXPLORE_REMOTE_INCLUDED_NODE_INFORMATION_FRAME includedNodeInformation;
  SET_NWI_MODE_FRAME setNWIMode;
} EXPLORE_REQUEST_FRAME;

void IsMyExploreFrame(ZW_ReceiveFrame_t *pFrame);

extern uint8_t inclusionHomeID[HOMEID_LENGTH];

/****************************************************************************/
/*                              PRIVATE DATA                                */
/****************************************************************************/

static exploreFrameIgnore exploreFrameIgnoreQueue[EXPLORE_FRAME_IGNORE_SIZE] = {{ 0 }};

static SSwTimer IgnoreQueueTimer = { 0 };
static SSwTimer ExploreQueueTimer = { 0 };

exploreQueueElementStruct *pCurrentExploreQueueElement;

TX_STATUS_TYPE currentTxStatusReport = { 0 };

/* Current Explore frame parameter set */
static uint8_t bExploreEvent;                   /* Current Explore event */
static uint8_t bExploreStatus;                  /* Current Explore event status */
static uint8_t bExploreQueueIndex;

static bool bExploreSlotTaken = false;

#ifdef ZW_CONTROLLER
 static uint8_t assignIDBuffer[1 + 1 + MAX_REPEATERS + sizeof(ASSIGN_IDS_FRAME)]; /* Buffer used for transmitting AssignID */
#endif

static frameExploreStruct exploreSearchResultBuffer = { 0 };

bool searchResultNeeded;

static const ZW_ReceiveFilter_t mExplorerReceiveFilter = {.headerType = HDRTYP_EXPLORE, .flag = 0, .frameHandler = IsMyExploreFrame};

/****************************************************************************/
/*                              Public DATA                                 */
/****************************************************************************/

#ifdef ZW_CONTROLLER
bool exploreRemoteInclusion = false;
#endif

#ifdef ZW_REPEATER
bool exploreInclusionModeRepeat = false;
#endif

bool bNetworkWideInclusionReady = false;

exploreQueueElementStruct exploreQueue[EXPLORE_QUEUE_MAX] = { 0 };
exploreQueueElementStruct *pExploreQueueElement;

/* State byte telling if Node are in any of the network wide inclusion modes */
uint8_t bNetworkWideInclusion = NETWORK_WIDE_MODE_IDLE;

// SyncEvent which must be bound. Is Explore module requesting
// ExploreMachine to be called.
SSyncEvent ExploreMachineUpdateRequest;


/****************************************************************************/
/*                              EXTERN DATA                                 */
/****************************************************************************/

/* Flag telling if currentSeqNoTX is to be used in the current transaction - else we use currentSeqNo */
extern bool currentSeqNoUseTX;

/* rxopt being passed by reference to the CommandHandler */
/* Reuse the one allocated in ZW_transport.c */
extern RECEIVE_OPTIONS_TYPE rxopt;

#ifdef ZW_CONTROLLER
extern uint8_t crH[HOMEID_LENGTH];
extern uint8_t addSmartNodeHomeId[2 * HOMEID_LENGTH];
#endif

/****************************************************************************/
/*                          FUNCTION PROTOTYPES                             */
/****************************************************************************/
uint8_t ExploreQueueFrame(uint8_t bSrcNodeID, uint8_t bDstNodeID, uint8_t *pData,
                       uint8_t bDataLength, uint8_t bExploreFlag,
                       const STransmitCallback* pCompletedFunc);
static uint8_t ExploreQueueIndex();

static bool ExploreFrameIgnoreCheck(uint8_t sourceID, uint8_t sequenceNo);

#ifdef ZW_CONTROLLER
// Method from Controller.c
void        /* RET  Nothing */
ZCB_AssignTxComplete(
  ZW_Void_Function_t Context, uint8_t txStatus, TX_STATUS_TYPE *txStatusReport);
#endif

static bool ExploreCheckTimeout(void);
static void SetExploreMachineUpdate(void);
static bool ExploreEventPush(uint8_t bEvent, uint8_t bStatus);
static void ActivateExploreFrameIgnoreQueueEntry(uint32_t iIndex, uint32_t iTimeout);
static void ExploreFrameIgnoreTiming(void);

static void ZCB_IgnoreQueueTimer(SSwTimer* ptimer);
static void ZCB_ExploreQueueTimer(SSwTimer* ptimer);

/****************************************************************************/
/*                              EXPORTED FUNCTIONS                          */
/****************************************************************************/


#if 0
/*============================= ZW_SendDataExplore =========================
**    Send out an explore frame
**
**  Side effects:
**
**--------------------------------------------------------------------------*/
uint8_t                          /*RET false if ExploreQueue full */
ZW_SendDataExplore(
  uint8_t bNodeID,               /* IN Destination node ID (0xFF == broadcast) */
  uint8_t *pData,                /* IN Data buffer pointer           */
  uint8_t bDataLength,           /* IN Data buffer length            */
  uint8_t bExploreOptions,       /* IN Explore frame options         */
  VOID_CALLBACKFUNC(completedFunc)(uint8_t)) /*IN  Transmit completed call back function  */
{
  DPRINT("SendDataExplore\r\n");
  /* Application originated explore frames are always EXPLORE_CMD_NORMAL */
  return ExploreQueueFrame(nodeID, bNodeID, pData, bDataLength,
                           ((bExploreOptions & TRANSMIT_EXPLORE_OPTION_MASK) | QUEUE_EXPLORE_CMD_NORMAL),
                           completedFunc);
}
#endif


#ifdef ZW_CONTROLLER
/*========================   ExploreTransmitSetNWIMode   =====================
**    Transmit a Set NWI to bMode command via a NODE_BROADCAST explore frame.
**    Current NWI modes are:
**      NWI_IDLE
**      NWI_REPEAT
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
uint8_t
ExploreTransmitSetNWIMode(
  uint8_t bMode,
  const STransmitCallback* pCompletedFunc)
{
  SET_NWI_MODE_FRAME setNWIMode = {
    .cmdClass = ZWAVE_CMD_CLASS_PROTOCOL,
    .cmd = ZWAVE_CMD_SET_NWI_MODE,
    .mode = bMode,
    .timeout = NWI_TIMEOUT_DEFAULT
  };
  return ExploreQueueFrame(g_nodeID, NODE_BROADCAST, (uint8_t*)&setNWIMode, sizeof(SET_NWI_MODE_FRAME),
                           QUEUE_EXPLORE_CMD_NORMAL,
                           pCompletedFunc);
}
#endif


/****************************************************************************/
/*                            Functions used in library                     */
/****************************************************************************/


void ExploreInit(void)
{
  ZwTimerRegister(
                  &IgnoreQueueTimer,
                  false,
                  ZCB_IgnoreQueueTimer
                );

  ZwTimerRegister(
                  &ExploreQueueTimer,
                  false,
                  ZCB_ExploreQueueTimer
                );

  SyncEventUnbind(&ExploreMachineUpdateRequest);
  // We want to receive all explorer frames directly.
  llReceiveFilterAdd(&mExplorerReceiveFilter);
}


/*===============================   ExploreQueueFrame   ======================
**    Queue an explore frame destined for dstNodeID in Explore frame queue
**    if possible. If successful pExploreQueueElement points to the element
**    used.
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
uint8_t                    /*RET ZERO if ExploreFrame queue full */
ExploreQueueFrame(
  uint8_t bSrcNodeID,      /* IN Source node ID */
  uint8_t bDstNodeID,      /* IN Destination node ID, if equal to 0xFF then all nodes */
  uint8_t *pData,          /* IN Data buffer pointer           */
  uint8_t bDataLength,     /* IN Data buffer length            */
  uint8_t bExploreFlag,    /* IN Explore frame flag containing frame type and explore options */
  const STransmitCallback* pCompletedFunc)  /*IN  Transmit completed call back function  */
{
  /* Make sure the explore frame can contain the payload, if not - FAIL. */
  if ((bDataLength <= EXPLORE_MAX_PAYLOAD) &&
      ((bExploreQueueIndex = ExploreQueueIndex()) < EXPLORE_QUEUE_MAX))
  {
#ifdef ZW_CONTROLLER_BRIDGE
    if (bSrcNodeID == 0xff)
    {
      bSrcNodeID = g_nodeID;
    }
#endif
    /* Used by ReTransmitFail when resorting to explore frame route resolution */
    pExploreQueueElement = &exploreQueue[bExploreQueueIndex];

    pExploreQueueElement->state.exploreState = EXPLORE_STATE_NEW;
    pExploreQueueElement->state.exploreStatus = TRANSMIT_COMPLETE_OK;
    pExploreQueueElement->state.srcNode = pExploreQueueElement->exploreFrame.baseFrame.frameOptions.sourceNodeId = bSrcNodeID;
    pExploreQueueElement->state.destNode = pExploreQueueElement->exploreFrame.baseFrame.frameOptions.destinationNodeId = bDstNodeID;
    /* Get the cmd type and place where it belongs */
    pExploreQueueElement->state.cmd = ((bExploreFlag & QUEUE_EXPLORE_CMD_MASK) >> 4);
    /* Here we save the explore options */
    pExploreQueueElement->state.exploreOptions = (bExploreFlag & ~QUEUE_EXPLORE_CMD_MASK);
    pExploreQueueElement->exploreFrame.baseFrame.frameOptions.acknowledge = (bExploreFlag & TRANSMIT_OPTION_ACK) ? 1 : 0;
    pExploreQueueElement->exploreFrame.baseFrame.frameOptions.lowPower    = (bExploreFlag & TRANSMIT_OPTION_LOW_POWER) ? 1 : 0;
    /* The explore header starts at offset 0 in the payload buffer */
    pExploreQueueElement->exploreFrame.pExploreHeader = (frameHeaderExplore *) pExploreQueueElement->exploreFrame.baseFrame.payload;

    pExploreQueueElement->exploreFrame.baseFrame.payloadLength = sizeof(frameHeaderExplore);
    if (bDataLength && pData)
    {
      /* Copy the payload contents at offset after the explore header */
      memcpy(pExploreQueueElement->exploreFrame.baseFrame.payload + sizeof(frameHeaderExplore), pData, bDataLength);
      pExploreQueueElement->exploreFrame.baseFrame.payloadLength += bDataLength;
    }
    pExploreQueueElement->exploreFrame.pExploreHeader->sessionTxRandomInterval = EXPLORE_RANDOM_INTERVAL_DEFAULT;
    /* Set payload length to the full Explore Frame plus the possible piggybacked payload */
    pExploreQueueElement->state.payloadLength = bDataLength + sizeof(frameHeaderExplore);
    pExploreQueueElement->state.func = *pCompletedFunc;
    pExploreQueueElement->state.StartTicks = getTickTime();

    // Ensure Explore machine is updated - As new explore frame has been added
    SetExploreMachineUpdate();

    /* Return the exploreQueueIndex + 1, so that we can make explore resolution work */
    return(bExploreQueueIndex + 1);
  }
  return (0);
}


/*============================= ExploreComplete =============================
**  Callback function called when an explore frame has been transmitted
**
**  Side effects:
**
**--------------------------------------------------------------------------*/
void
ZCB_ExploreComplete(
  __attribute__((unused)) ZW_Void_Function_t Context,
  uint8_t bTxStatus,
  TX_STATUS_TYPE* pTxStatusReport)
{
  DPRINTF("ExploreComplete %d\r\n", bTxStatus);
  memcpy((uint8_t*)&currentTxStatusReport, (uint8_t*)pTxStatusReport, sizeof(currentTxStatusReport));
  uint8_t event = EXPLORE_EVENT_FRAME_TRANSMITTED;
  if (TX_QUEUE_PHY_TX_SUCCESS != bTxStatus)
  {
    event = EXPLORE_EVENT_TRANSMIT_FAILED;

  }
  ExploreEventPush(event, bTxStatus);
}


/*============================= ExploreStateNew =============================
**  Handles the Explore State New function
**
**  Side effects:
**
**--------------------------------------------------------------------------*/
static void
ExploreStateNew(void)
{
  uint8_t bHops;

  pCurrentExploreQueueElement->state.exploreState = EXPLORE_STATE_EMPTY;
  switch (pCurrentExploreQueueElement->state.cmd)
  {
    case EXPLORE_CMD_NORMAL:
    case EXPLORE_CMD_AUTOINCLUSION:
      {
        bHops = (pCurrentExploreQueueElement->exploreFrame.pExploreHeader->repeaterCountSessionTTL & // -5 equals SESSION_TTL_REPEATER_COUNT_INDEX
                 EXPLORE_REPEATERCOUNTSESSIONTTL_COUNT_MASK);

        /* Set Timeout - how much should we wait before transmitting the explore frame */
        uint32_t iTimeout = 0;

        if (bHops)
        {
          const uint8_t rand_max = pCurrentExploreQueueElement->exploreFrame.pExploreHeader->sessionTxRandomInterval;
          iTimeout = (
                      EXPLORE_REPEATER_DELAY_OFFSET +
                      (--bHops * EXPLORE_HOPFACTOR_DEFAULT) +
                      (2 * (zpal_get_pseudo_random() % (rand_max + 1))) // -6 equals SESSION_TX_RANDOM_INTERVAL_INDEX
                    );
        }

        ExploreSetTimeout(pCurrentExploreQueueElement, iTimeout);

        DPRINTF("ExploreStateNew %d\r\n", iTimeout);
        pCurrentExploreQueueElement->state.exploreState = EXPLORE_STATE_DELAYED;
      }
      break;

    default:
      /* Drop what we do not know how to handle */
      break;
  }
}


/*======================= ExploreCheckCountTimeout ======================
**  Checks TimeoutTickTime in the exploreQueue indexed
**  by bExploreQueueIndex and returns true if timeout has occured
**
**  NOTE: Dont call when timer timeout isnt active
**
**  Side effects:
**    pCurrentExploreQueueElement->state.bTimeoutActive updated
**
**--------------------------------------------------------------------------*/
static bool                            /* RET false if no timeout, true if timeout */
ExploreCheckTimeout(void)
{
  if (pCurrentExploreQueueElement->state.bTimeoutActive)
  {
    int32_t iTimeToTimeout = (int32_t)(pCurrentExploreQueueElement->state.TimeoutTickTime - getTickTime());

    if (iTimeToTimeout <= 0)
    {
      pCurrentExploreQueueElement->state.bTimeoutActive = false;
      return true;
    }
  }
  return false;
}


void ExploreSetTimeout(exploreQueueElementStruct* pQueueElement, uint32_t iTimeout)
{
  uint32_t iCurrentTime = getTickTime();

  pQueueElement->state.TimeoutTickTime = iCurrentTime + iTimeout;
  pQueueElement->state.bTimeoutActive = true;

  if (0 == iTimeout)  // Handle special case of timeout 0
  {
    ExploreEventPush(EXPLORE_EVENT_TIMEOUT, 0);
    return;
  }

  // Run through active timers to see if any existing timer runs out
  // sooner than the one we are setting up now
  bool bNewTimeoutIsNext = true;
  for (uint32_t i = 0; i < EXPLORE_QUEUE_MAX; i++)
  {
    // Skip timeout inactive elements and argument element
    if (exploreQueue[i].state.bTimeoutActive && (&exploreQueue[i] != pQueueElement))
    {
      int32_t iTimeToTimeout = (int32_t)(exploreQueue[i].state.TimeoutTickTime - iCurrentTime);

      if ((iTimeToTimeout > 0) && (iTimeToTimeout <= ((int32_t)iTimeout)))
      {
        bNewTimeoutIsNext = false;
        break;
      }
    }
  }

  // If new timer runs out first
  if (bNewTimeoutIsNext)
  {
    // New timer is first to run out - update SwTimer
    if (TimerIsActive(&ExploreQueueTimer))
    {
      // In case we are stopping a pending timer, push a timeout
      // event (Explore machine can handle false timeout events)
      ExploreEventPush(EXPLORE_EVENT_TIMEOUT, 0);
    }

    TimerStart(&ExploreQueueTimer, iTimeout);
  }
}


/*============================= ExploreStopTimeout ============================
**  Stop a timeout on an exploreQueueElement
**
** Handles practicalities around stopping a timeout on a queue element.
** It stops the ExploreQueueTimer if there are no other explore queue elements
** with active timeout.
**
**  Side effects:
**    pCurrentExploreQueueElement->state.bTimeoutActive updated
**    ExploreQueueTimer may be updated
**
**--------------------------------------------------------------------------*/
void ExploreStopTimeout(exploreQueueElementStruct* pQueueElement)
{
  pQueueElement->state.bTimeoutActive = false;

  // Check if any explore queue timeouts are still active
  for (uint32_t i = 0; i < EXPLORE_QUEUE_MAX; i++)
  {
    if (exploreQueue[i].state.bTimeoutActive)
    {
      return;
    }
  }

  // Stop timer if there are no active timeouts
  if (TimerIsActive(&ExploreQueueTimer))
  { // If avoids unnecessary context switches etc.
    TimerStop(&ExploreQueueTimer);
  }
}


static void ZCB_ExploreQueueTimer(SSwTimer* pTimer)
{
  ExploreEventPush(EXPLORE_EVENT_TIMEOUT, 0);

  uint32_t iCurrentTime = getTickTime();
  int32_t iNextTimeout = 0x7FFFFFFF;

  // Run through active timers to see when next timeout is
  for (uint32_t i = 0; i < EXPLORE_QUEUE_MAX; i++)
  {
    if (exploreQueue[i].state.bTimeoutActive)
    {
      int32_t iTimeToTimeout = (int32_t)(exploreQueue[i].state.TimeoutTickTime - iCurrentTime);

      if (iTimeToTimeout > 0)
      {
        iNextTimeout = Minimum2Signed(iTimeToTimeout, iNextTimeout);
      }
    }
  }

  if (iNextTimeout != 0x7FFFFFFF)
  {
    TimerStart(pTimer, iNextTimeout);
  }
}


/*============================= ExploreStateDelayed ==========================
**  Handles the Explore State Delayed function
**
**  Side effects:
**
**--------------------------------------------------------------------------*/
void
ExploreStateDelayed(void)
{
  switch (bExploreEvent)
  {
    case EXPLORE_EVENT_TIMEOUT:

      {
        if (ExploreCheckTimeout())
        {
          bExploreSlotTaken = true;
          pCurrentExploreQueueElement->state.exploreState = EXPLORE_STATE_TRANSMIT;
          // Run machine as eplorer frame has transitioned to state transmit
          SetExploreMachineUpdate();
        }
      }
      break;

    //case EXPLORE_EVENT_IDLE:              /* Nothing happening */
    //case EXPLORE_EVENT_SEARCH_RESULT_RECEIVED:   /* Search Result Explore frame received */
    //case EXPLORE_EVENT_FRAME_TRANSMITTED: /* Explore frame physically transmitted */
     case EXPLORE_EVENT_TRANSMIT_FAILED:
        pCurrentExploreQueueElement->state.exploreStatus = bExploreStatus;
        pCurrentExploreQueueElement->state.exploreState = EXPLORE_STATE_EXECUTE;
          // Run machine as eplorer frame has transitioned to state transmit
        SetExploreMachineUpdate();

       break;
    default:
      break;
  }
}


/*============================= ExploreStateTransmit =========================
**  Handles the Explore State Transmit function
**
**  Side effects:
**
**--------------------------------------------------------------------------*/
void
ExploreStateTransmit(void)
{
  switch (bExploreEvent)
  {
    case EXPLORE_EVENT_IDLE:
      DPRINT("ExploreStateTransmit\r\n");

      uint32_t iTimeout;
      if (pCurrentExploreQueueElement->state.exploreOptions & TRANSMIT_OPTION_EXPLORE_REPEAT)
      {
        iTimeout = EXPLORE_REPEAT_FRAME_TIMEOUT;
        pCurrentExploreQueueElement->state.exploreState = EXPLORE_STATE_REPEAT;
        currentSeqNoUseTX = false;
      }
      else
      {
        /* The explore header starts at offset 0 in the payload buffer */
        pCurrentExploreQueueElement->exploreFrame.pExploreHeader = (frameHeaderExplore *)pCurrentExploreQueueElement->exploreFrame.baseFrame.payload;

        pCurrentExploreQueueElement->exploreFrame.pExploreHeader->ver_Cmd = EXPLORE_VER_DEFAULT | pCurrentExploreQueueElement->state.cmd;
        /* Max 4 repeaters  */
        pCurrentExploreQueueElement->exploreFrame.pExploreHeader->repeaterCountSessionTTL = 0x40;
        /* Initialy all repeaters are zero */
        memset(&pCurrentExploreQueueElement->exploreFrame.pExploreHeader->repeaterList, 0, MAX_REPEATERS); // -4 equals REPEATER_LIST_START_INDEX
        pCurrentExploreQueueElement->state.exploreState = EXPLORE_STATE_SEARCH;
        /* */
        if(0 != (pCurrentExploreQueueElement->state.exploreOptions & TRANSMIT_OPTION_EXPLORE_DO_ACK_TIMEOUT))
        {
          iTimeout = EXPLORE_FRAME_SMART_START_TIMEOUT; /* 2 sec */
        }
        else
        {
          iTimeout = EXPLORE_FRAME_TIMEOUT; /* 4 sec */
        }
        currentSeqNoUseTX = true;
      }

      ExploreSetTimeout(pCurrentExploreQueueElement, iTimeout);

      pCurrentExploreQueueElement->exploreFrame.baseFrame.frameOptions.sourceNodeId      = pCurrentExploreQueueElement->state.srcNode;
      pCurrentExploreQueueElement->exploreFrame.baseFrame.frameOptions.destinationNodeId = pCurrentExploreQueueElement->state.destNode;

      pExploreQueueElement->exploreFrame.baseFrame.status = TRANSMIT_OPTION_EXPLORE_REPEAT;
      if (!TransportEnQueueExploreFrame(&pCurrentExploreQueueElement->exploreFrame,
                                        pCurrentExploreQueueElement->state.exploreOptions & ~TRANSMIT_OPTION_EXPLORE_DO_ACK_TIMEOUT,
                                        ZCB_ExploreComplete))
      {
        pCurrentExploreQueueElement->state.exploreState = EXPLORE_STATE_EXECUTE;
        // Run machine as explore frame has transitioned to state execute
        SetExploreMachineUpdate();
      }
      break;

    //case EXPLORE_EVENT_FRAME_TRANSMITTED: /* Explore frame physically transmitted */
    //case EXPLORE_EVENT_2MS_TICK:          /* 2ms has passed */
    //case EXPLORE_EVENT_SEARCH_RESULT_RECEIVED:   /* Search Result Explore frame received */
    default:
      break;
  }
}


/*============================= ExploreStateSearch ===========================
**  Handles the Explore State Search functionality
**
**  Side effects:
**
**--------------------------------------------------------------------------*/
void
ExploreStateSearch(void)
{
  switch (bExploreEvent)
  {
    case EXPLORE_EVENT_TIMEOUT:
      if (ExploreCheckTimeout())
      {
        /* Timeout */
        searchResultNeeded = false;
        pCurrentExploreQueueElement->state.exploreStatus = TRANSMIT_COMPLETE_NO_ACK;
        pCurrentExploreQueueElement->state.exploreState = EXPLORE_STATE_EXECUTE;
        // Run machine as eplorer frame has transitioned to state execute
        SetExploreMachineUpdate();
      }
      break;

    case EXPLORE_EVENT_FRAME_TRANSMITTED: /* Explore frame physically transmitted */
      /* Copy currentTxStatusReport struct into pCurrentExploreElement */
      memcpy((uint8_t*)&pCurrentExploreQueueElement->state.sTxStatusReport, (uint8_t*)&currentTxStatusReport, sizeof(TX_STATUS_TYPE));
      /* Are we waiting for a Search Result Stop Explore Frame */
      if (!(pCurrentExploreQueueElement->state.exploreOptions & (TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_EXPLORE_DO_ACK_TIMEOUT)))
      {
        DPRINT("Search TXed\r\n");
        /* No - then we are done */
        pCurrentExploreQueueElement->state.exploreState = EXPLORE_STATE_EXECUTE;
        // Run machine as eplorer frame has transitioned to state execute
        SetExploreMachineUpdate();
      }
      else
      {
        /* Received ACK on TransportGetCurrentRxCh() */
        pCurrentExploreQueueElement->state.sTxStatusReport.bACKChannelNo = TransportGetCurrentRxChannel();

        /* We are waiting for a SEARCH_RESULT frame */
        searchResultNeeded = true;
      }
      break;

    case EXPLORE_EVENT_SEARCH_RESULT_RECEIVED:   /* Search Result Explore frame received */
      DPRINTF("Search RXed - Status %d, Dest %d", bExploreStatus, pCurrentExploreQueueElement->state.destNode);
      if (bExploreStatus == pCurrentExploreQueueElement->state.destNode)
      {
        DPRINT(" - Tx OK");
        /* We got a explore search result frame from the destination of our explore frame */
        pCurrentExploreQueueElement->state.exploreState = EXPLORE_STATE_EXECUTE;
        // Run machine as eplorer frame has transitioned to state execute
        SetExploreMachineUpdate();
      }
//      DPRINT("\r\n");
      break;
    case EXPLORE_EVENT_TRANSMIT_FAILED: /* Explore frame physically not transmitted */
      pCurrentExploreQueueElement->state.exploreStatus = bExploreStatus;
      pCurrentExploreQueueElement->state.exploreState = EXPLORE_STATE_EXECUTE;
        // Run machine as eplorer frame has transitioned to state execute
      SetExploreMachineUpdate();

      break;
    default:
      break;
  }
}


/*============================= ExploreStateRepeat ===========================
**  Handles the Explore State Repeat function
**
**  Side effects:
**
**--------------------------------------------------------------------------*/
void
ExploreStateRepeat(void)
{
  switch (bExploreEvent)
  {
    case EXPLORE_EVENT_TIMEOUT:
      if (!ExploreCheckTimeout())
      {
        break;                          // Break if timer havent timed out
      }                                   // Intentional fallthrough
    case EXPLORE_EVENT_TRANSMIT_FAILED: /* Explore frame physically not transmitted */
      pCurrentExploreQueueElement->state.exploreStatus = bExploreStatus;
      __attribute__((__fallthrough__));
    case EXPLORE_EVENT_FRAME_TRANSMITTED: /* Explore frame physically transmitted */
    case EXPLORE_EVENT_SEARCH_RESULT_RECEIVED:   /* Search Result Explore frame received */
      pCurrentExploreQueueElement->state.exploreState = EXPLORE_STATE_EXECUTE;
      // Run machine as eplorer frame has transitioned to state execute
      SetExploreMachineUpdate();
      break;

    //case EXPLORE_EVENT_IDLE:
    default:
      break;
  }
}


/*========================== SetExploreMachineUpdate =========================
**  Set a run of the explore machine pending
**
**  Side effects:
**
**--------------------------------------------------------------------------*/
static void
SetExploreMachineUpdate(void)
{
  // callback to request run of explore machine
  SyncEventInvoke(&ExploreMachineUpdateRequest);
}


/*========================== SetExploreMachineUpdate =========================
**  Push an event on the explore event queue.
** Also ensures explore machine will run to handle the event.
**
**  Side effects:
**
**--------------------------------------------------------------------------*/
static bool
ExploreEventPush(uint8_t bEvent, uint8_t bStatus)
{
  if (EventPush(bEvent, bStatus))
  {
    SetExploreMachineUpdate();
    return true;
  }

  DPRINTF("Failed to push event %d\r\n", bEvent);
  return false;
}


/*=================== ActivateExploreFrameIgnoreQueueEntry ===================
**  Activates a new entry in the IgnoreQueue
**  Must be called when a new entry is put in the exploreFrameIgnoreQueue.
**
**  Side effects:
**
**--------------------------------------------------------------------------*/
static void
ActivateExploreFrameIgnoreQueueEntry(uint32_t iIndex, uint32_t iTimeout)
{
  exploreFrameIgnoreQueue[iIndex].ignoreTickTime = iTimeout + getTickTime();
  exploreFrameIgnoreQueue[iIndex].bIsActive = true;
  ExploreFrameIgnoreTiming();
}


/*========================== ExploreFrameIgnoreTick ==========================
**  Handles explore frame ignore timeouts - retires explore frame ignore
**  entries when timeout expires.
**  Must be called when a new entry is put in the exploreFrameIgnoreQueue (is
**  called by ActivateExploreFrameIgnoreQueueEntry).
**
**  Side effects:
**
**--------------------------------------------------------------------------*/
static void
ExploreFrameIgnoreTiming(void)
{
  uint32_t iCurrentTime = getTickTime();
  int32_t iNextTimeout = 0x7FFFFFFF;

  for (uint32_t i = 0; i < EXPLORE_FRAME_IGNORE_SIZE; i++)
  {
    /* Check if entry is active */
    if (exploreFrameIgnoreQueue[i].bIsActive)
    {
      int32_t iTimeToTimeout = (int32_t)(exploreFrameIgnoreQueue[i].ignoreTickTime - iCurrentTime);

      // If timeout has passed - retire the entry
      if(iTimeToTimeout <= 0)
      {
        DPRINTF(
                "Ignore entry retired - id %d, handle %d\r\n",
                exploreFrameIgnoreQueue[i].id,
                exploreFrameIgnoreQueue[i].handle
              );

        // Clear entry
        exploreFrameIgnoreQueue[i].handle = 255;
        exploreFrameIgnoreQueue[i].id = 0;
        exploreFrameIgnoreQueue[i].bIsActive = false;
      }
      else
      {
        // From if part - we ensured that iTimeToTimeout is positive
        iNextTimeout = Minimum2Signed(iNextTimeout, iTimeToTimeout);
      }
    }
  }

  // If there is an active entry in ignore queue
  if (iNextTimeout != 0x7FFFFFFF)
  {
    // setup timer to wake us up in NextTimeout
    TimerStart(&IgnoreQueueTimer, iNextTimeout);
  }
}


void ZCB_IgnoreQueueTimer(__attribute__((unused)) SSwTimer* pTimer)
{
  ExploreFrameIgnoreTiming();
}


/*============================= ExploreMachine ===============================
**  Main explore frame handling - must be called frequently from main loop
**
**  Side effects:
**
**--------------------------------------------------------------------------*/
void
ExploreMachine(void)
{

    EventPop(&bExploreEvent, &bExploreStatus);

//    DPRINTF("Machine Event %d\r\n", bExploreEvent);

    for (bExploreQueueIndex = 0; bExploreQueueIndex < EXPLORE_QUEUE_MAX; bExploreQueueIndex++)
    {
      pCurrentExploreQueueElement = &exploreQueue[bExploreQueueIndex];

//      DPRINTF("ExploreState %d\r\n", pCurrentExploreQueueElement->state.exploreState);

      switch (pCurrentExploreQueueElement->state.exploreState)
      {
      case EXPLORE_STATE_NEW:
        ExploreStateNew();
        break;

      case EXPLORE_STATE_DELAYED:
        ExploreStateDelayed();
        break;

      case EXPLORE_STATE_TRANSMIT:
        ExploreStateTransmit();
        break;

      case EXPLORE_STATE_SEARCH:
        ExploreStateSearch();
        break;

      case EXPLORE_STATE_REPEAT:
        ExploreStateRepeat();
        break;

      case EXPLORE_STATE_EXECUTE:
        if(ZW_TransmitCallbackIsBound(&pCurrentExploreQueueElement->state.func))
        {
          if (bExploreEvent != EXPLORE_EVENT_FRAME_TRANSMITTED)
          {
            bExploreStatus = pCurrentExploreQueueElement->state.exploreStatus;
          }

//          DPRINTF("Has func - Explore status %d\r\n", bExploreStatus);

          pCurrentExploreQueueElement->state.sTxStatusReport.TransmitTicks = getTickTimePassed(pCurrentExploreQueueElement->state.StartTicks);
          ZW_TransmitCallbackInvoke(
                                    &pCurrentExploreQueueElement->state.func,
                                    bExploreStatus,
                                    &pCurrentExploreQueueElement->state.sTxStatusReport
                                    );
          ZW_TransmitCallbackUnBind(&pCurrentExploreQueueElement->state.func);
        }
        /* Remove explore entry */
        ExploreStopTimeout(pCurrentExploreQueueElement);
        pCurrentExploreQueueElement->state.exploreState = EXPLORE_STATE_EMPTY;
        bExploreSlotTaken = false;
        break;

        //case EXPLORE_STATE_EMPTY:
      default:
        break;
      }
    }

}


/****************************************************************************/
/*                            PRIVATE FUNCTIONS                             */
/****************************************************************************/


/*============================== ExploreQueueIndex ===========================
**  Function which returns the index to the first empty slot in the exploreQueue
**
**  Side effects:
**
**--------------------------------------------------------------------------*/
uint8_t                    /*RET EXPLORE_QUEUE_MAX if exploreQueue is full */
ExploreQueueIndex(void)
{
  uint32_t i;

  for (i = 0; i < EXPLORE_QUEUE_MAX; i++)
  {
    if (exploreQueue[i].state.exploreState == EXPLORE_STATE_EMPTY)
    {
      /* Empty exploreQueue slot found */
      break;
    }
  }

//  DPRINTF("ExploreQueueIndex %d\r\n", i);

  return i;
}


/*======================== ExploreFrameIgnoreQueueIndex ======================
**  Returns the index into exploreFrameIgnoreQueue where the next explore
**  frame ignore entry should be placed. If an empty slot exist, then that
**  will be used else the entry with the ignoreTimeout closest to
**  a timeout will be used. The found slot id are set to bSourceNode
**
**  Side effects:
**
**--------------------------------------------------------------------------*/
static uint8_t
ExploreFrameIgnoreQueueIndex(
  uint8_t bSourceNode)
{
  uint8_t foundSlotIndex, i;
  uint8_t j = 0;
  int32_t n;

  if (!bSourceNode)
  {
    /* if nodeID is ZERO or NODE_CONTROLLER_OLD then its an autoinclusion request */
    /* and these we want to throttle - give a graceperiod */
    bSourceNode = NODE_CONTROLLER_OLD;
  }
  i = 0xff;
  n = 0x7fffffff;
  uint32_t iCurrentTime = getTickTime();
  for (foundSlotIndex = 0; foundSlotIndex < EXPLORE_FRAME_IGNORE_SIZE; foundSlotIndex++)
  {
    int32_t iTimeToTimeout = (int32_t)(exploreFrameIgnoreQueue[foundSlotIndex].ignoreTickTime - iCurrentTime);
    if (iTimeToTimeout <= n)
    {
      /* If no empty slots are found we use the ignore slot that */
      /* are nearest to a timeout */
      n = iTimeToTimeout;
      j = foundSlotIndex;
    }
    /* Do any slot exist which are allready assigned to the bSourceNode */
    if (exploreFrameIgnoreQueue[foundSlotIndex].id == bSourceNode)
    {
      /* There is one allready assigned, lets use it */
      j = foundSlotIndex;
      break;
    }
    else if (!exploreFrameIgnoreQueue[foundSlotIndex].bIsActive)
    {
      /* Empty Ignore Explore Frame slot found */
      i = foundSlotIndex;
    }
  }
  /* Have we found any slots allready assigned to the node */
  if (foundSlotIndex >= EXPLORE_FRAME_IGNORE_SIZE)
  {
    /* No slot assigned to the bSourceNode allready */
    if (i < 0xff)
    {
      /* We found an empty Ignore Explore Frame slot */
      j = i;
    }
    /* No slot allready assigned for this node, therefor its a new entry */
    exploreFrameIgnoreQueue[j].id = bSourceNode;
    /* If no empty slot found, then we use the slot which is Most Close to a Timeout */
  }
//  DPRINTF("ExploreFrameIgnoreQueueIndex %d", i);

  return j;
}


/*============================== exploreFrameIgnore ==========================
**  Check if the explore frame is a frame which is in the ignore list or the
**  explore frame is originated from itself
**  RETURNS true if the above applies, which then means that the frame should
**  be ignore/discarded.
**
**  Side effects:
**
**--------------------------------------------------------------------------*/
static bool
ExploreFrameIgnoreCheck(
  uint8_t sourceID,
  uint8_t sequenceNo)
{
  uint8_t i;

//  DPRINTF("ExploreFrameIgnoreCheck - sID %d, SeqNo %d\r\n", sourceID, sequenceNo);
  /* Is sourceID the node itself */
  if (sourceID == g_nodeID
#ifdef ZW_CONTROLLER_BRIDGE
      || ZW_IsVirtualNode(sourceID)
#endif
     )
  {
    /* Explore frames received, which was originated by the node itself are just discarded */
    return true;
  }
  /* Treat frames originated from nodeID ZERO and NODE_CONTROLLER_OLD as ONE */
  if (!sourceID)
  {
    sourceID = NODE_CONTROLLER_OLD;
  }
  /* Check if the received explore frame should just be ignored */
  for (i = 0; i < EXPLORE_FRAME_IGNORE_SIZE; i++)
  {
    if (exploreFrameIgnoreQueue[i].bIsActive)
    {
      /* Is it a REQUEST_INCLUSION and have we allready received any */
      /* 'REQUEST_INCLUSION graceperiod enable' Z-Wave protocol frames */
      /* (NOP, ASSIGN ID, CMD_COMP or a NOP_POWER) */
      if ((sourceID == NODE_CONTROLLER_OLD) && exploreFrameIgnoreQueue[i].id == (NODE_BROADCAST - 1))
      {
        /* An NOP, ASSIGN ID, CMD_COMP or a NOP_POWER has been received */
        /* - ignore ALL Explore Frames until ignoreTimeout */
        return true;
      }
      /* Any entries for sourceID active */
      if (exploreFrameIgnoreQueue[i].id == sourceID)
      {
        /* An explore ignore entry has been found for sourceID */
        /* If sourceID is ZERO or NODE_CONTROLLER_OLD then we do not care what the sequenceNo is */
        /* as the frame must be a REQUEST_INCLUSION explore frame and we want to ignore all REQUEST_INCLUSION */
        /* frames for the full ignoreTime */
        if ((exploreFrameIgnoreQueue[i].handle == sequenceNo) || (sourceID == NODE_CONTROLLER_OLD))
        {
//          DPRINT("+\r\n");
          return true;
        }
//        DPRINT("-\r\n");
        /* A new frame has been received from sourceID but with another */
        /* handle - remove the found ignore entry */
      }
    }
  }
  return false;
}


/*============================== ExplorePurgeQueue ===========================
**    Purges the Explore frame queue if needed
**
**  Side effects:
**
**--------------------------------------------------------------------------*/
void
ExplorePurgeQueue(
  uint8_t sourceID)
{
  uint8_t j;

  DPRINTF("Explore purge queue - sID %d", sourceID);
  /* Is sourceID the node itself */
  if ((sourceID != g_nodeID)
#ifdef ZW_CONTROLLER_BRIDGE
      && !ZW_IsVirtualNode(sourceID)
#endif
     )
  {
//    DPRINT(" - Not self ");
    for (j = 0; j < EXPLORE_QUEUE_MAX; j++)
    {
      /* For now we purge everything */
      exploreQueue[j].state.sTxStatusReport.TransmitTicks = getTickTimePassed(exploreQueue[j].state.StartTicks);

      ZW_TransmitCallbackInvoke(
                                &exploreQueue[j].state.func,
                                TRANSMIT_COMPLETE_NO_ACK,
                                &exploreQueue[j].state.sTxStatusReport
                                );
      ZW_TransmitCallbackUnBind(&exploreQueue[j].state.func);

      exploreQueue[j].state.exploreState = EXPLORE_STATE_EMPTY;
      ExploreStopTimeout(&exploreQueue[j]);
    }

    bExploreSlotTaken = false;
    /* (NODE_BROADCAST - 1) is used when receiving an ASSIGN_ID frame - */
    /* this should result in a ignoreEntry, which makes the node ignore all Explore Frames for ignoreTimeout */
    /* This is to give the inclusion in progress time to "do its thing", with as "little" interference as possible */
    if (sourceID == (NODE_BROADCAST - 1))
    {
      /* Insert (NODE_BROADCAST - 1) so that we can ignore ALL explore frames for ignoreTimeout */
      uint32_t iAvailableIgnoreQueueEntryIndex = ExploreFrameIgnoreQueueIndex(NODE_BROADCAST - 1);
      ActivateExploreFrameIgnoreQueueEntry(
                                            iAvailableIgnoreQueueEntryIndex,
											EXPLORE_FRAME_STOP_TIMEOUT
                                          );
    }
  }
//  DPRINT("\r\n");
  return;
}

/*============================ IsNetworkHomeIDValid ==========================
**    Returns true if received EXPLORE_CMD_AUTOINCLUSION networkHomeID is
**  either ZERO or equal to homeID
**
**  Side effects:
**
**--------------------------------------------------------------------------*/
static bool
IsNetworkHomeIDValid(
  uint8_t* pNetworkHomeID)
{
  /* networkHomeID is valid if networkHomeID and homeID are equal */
  if (memcmp(pNetworkHomeID, ZW_HomeIDGet(), HOMEID_LENGTH) == 0)
  {
    return true;
  }
  /* networkHomeID is valid if networkHomeID is ZERO */
  return (0 == (pNetworkHomeID[0] | pNetworkHomeID[1] | pNetworkHomeID[2] | pNetworkHomeID[3]));
}

bool
IsExploreRouteRepeaterNotDuplicated(const uint8_t *route, uint8_t index)
{
  if (0 != route[index])
  {
    // Indexed repeater is non zero then prior repeater also need to be non zero.
    if (0 == route[index - 1])
    {
      return false;
    }
    for (uint8_t i = 1; i <= index; i++)
    {
      // Every repeater nodeId must only be present in the route once.
      if (route[index] == route[index - i])
      {
        return false;
      }
    }
  }
  return true;
}

bool
IsExploreRouteRepeaterValid(const uint8_t *route, uint8_t index)
{
  if ((0 < index) && (MAX_REPEATERS > index))
  {
    return IsExploreRouteRepeaterNotDuplicated(route, index);
  }
  if (0 == index)
  {
    return true;
  }
  return false;
}

bool
IsExploreRouteValid(const frameHeaderExplore *pExplorerHeader, __attribute__((unused)) uint8_t bExploreFrameLength)
{
  uint8_t sessionTTL = (pExplorerHeader->repeaterCountSessionTTL & EXPLORE_REPEATERCOUNTSESSIONTTL_TTL_MASK) >> 4;
  uint8_t sessionCount = pExplorerHeader->repeaterCountSessionTTL & EXPLORE_REPEATERCOUNTSESSIONTTL_COUNT_MASK;
  const uint8_t *route = pExplorerHeader->repeaterList;
  bool valid = false;

  // For Explore frames the sessionTTL + sessionCount always equals MAX_REPEATERS (4)
  if (MAX_REPEATERS == sessionTTL + sessionCount)
  {
    for (uint8_t i = MAX_REPEATERS - 1; i > 0; i--)
    {
      valid = IsExploreRouteRepeaterValid(route, i);
      if (false == valid)
      {
        break;
      }
    }
  }
  return valid;
}

#ifdef ZW_REPEATER
frameExploreStruct*
ExploreSearchResultGenerateRepeatFrame(ZW_ReceiveFrame_t *pRxFrame, frameHeaderExplore *pExplorerHeader, uint8_t singleCastLen, uint8_t RepeaterCountSessionTTL)
{
  RepeaterCountSessionTTL--;
  /* We are the next hop */
  /* The frame should be repeated */
  /* Insert new repeaterCount and TTL in frame to repeat - inc TTL and dec repeaterCount */
  pExplorerHeader->repeaterCountSessionTTL = (pExplorerHeader->repeaterCountSessionTTL & EXPLORE_REPEATERCOUNTSESSIONTTL_TTL_MASK) + EXPLORE_REPEATERCOUNTSESSIONTTL_TTL_UNIT + RepeaterCountSessionTTL;
  /* No random interval - this is a source routed explore frame */
  pExplorerHeader->sessionTxRandomInterval = 0;
  exploreSearchResultBuffer.pExploreHeader = (frameHeaderExplore*)&pRxFrame->frameContent[singleCastLen];
  /* copy explorer header and payload contents to offset 0 in the TX payload buffer */
  exploreSearchResultBuffer.baseFrame.payloadLength = sizeof(EXPLORE_SEARCH_RESULT_FRAME);
  memcpy(exploreSearchResultBuffer.baseFrame.payload,
         (uint8_t*)exploreSearchResultBuffer.pExploreHeader,
         exploreSearchResultBuffer.baseFrame.payloadLength);
  currentSeqNoUseTX = false;
  // Make sure we use the correct homeID
  memcpy(exploreSearchResultBuffer.baseFrame.frameOptions.homeId, pRxFrame->frameOptions.homeId, HOMEID_LENGTH);
  exploreSearchResultBuffer.baseFrame.frameOptions.sourceNodeId = pRxFrame->frameOptions.sourceNodeId;
  exploreSearchResultBuffer.baseFrame.frameOptions.destinationNodeId = pRxFrame->frameOptions.destinationNodeId;
  exploreSearchResultBuffer.baseFrame.frameOptions.sequenceNumber = pRxFrame->frameOptions.sequenceNumber;

  return &exploreSearchResultBuffer;
}
#endif

void
ExploreSearchResultHandle(ZW_ReceiveFrame_t *pRxFrame, frameHeaderExplore *pExplorerHeader,
                          __attribute__((unused)) uint8_t singleCastLen, __attribute__((unused)) uint8_t checksumBytes)
{
  /* We have received an EXPLORE_CMD_SEARCH_RESULT frame */
  if ((0 == (pRxFrame->status & STATUS_NOT_MY_HOMEID)) && ((EXPLORE_OPTION_DIRECTION | EXPLORE_OPTION_SOURCEROUTED) == (pExplorerHeader->option & (EXPLORE_OPTION_DIRECTION | EXPLORE_OPTION_SOURCEROUTED))))
  {
    /* Its a frame from my network */
    if (pRxFrame->frameOptions.destinationNodeId == g_nodeID
#ifdef ZW_CONTROLLER_BRIDGE
        || ZW_IsVirtualNode(pRxFrame->frameOptions.destinationNodeId)
#endif
        )
    {
      /* Its for me, but do we need it */
      if (searchResultNeeded)
      {
        /* Yes, we were waiting for SearchResult frame... */
        searchResultNeeded = false;
        /* We do not care if the frame has been through all the hops */
        ExploreEventPush(EXPLORE_EVENT_SEARCH_RESULT_RECEIVED, (uint8_t)pRxFrame->frameOptions.sourceNodeId);
        /* Lets keep the store route functions nice and clean... */
#ifdef ZW_CONTROLLER
        /* Save the received route as Last Working Route for the explore frame destination */
        LastWorkingRouteCacheLineExploreUpdate((uint8_t)pRxFrame->frameOptions.sourceNodeId, true, &((EXPLORE_SEARCH_RESULT_FRAME*)(pExplorerHeader))->repeaterCountSessionTTL);
#else
        /* Save the received route as Response Route for the explore frame destination */
        /* Update Return routes if a response route is received. */
        UpdateResponseRouteLastReturnRoute(StoreRouteExplore((uint8_t)pRxFrame->frameOptions.sourceNodeId, true, &((EXPLORE_SEARCH_RESULT_FRAME*)(pExplorerHeader))->repeaterCountSessionTTL));
#endif
      }
    }
#ifdef ZW_REPEATER
    else
    {
      /* Are we the next hop in the source routed explore frame? */
      uint8_t RepeaterCountSessionTTL = (pExplorerHeader->repeaterCountSessionTTL & EXPLORE_REPEATERCOUNTSESSIONTTL_COUNT_MASK);
      if ((0 != RepeaterCountSessionTTL) && (pExplorerHeader->repeaterList[RepeaterCountSessionTTL - 1] == g_nodeID)
          /* If received searchResultFrame has wrong length then drop it. */
          && (pRxFrame->frameContentLength == sizeof(EXPLORE_SEARCH_RESULT_FRAME) + singleCastLen + checksumBytes))
      {
        ExploreSearchResultGenerateRepeatFrame(pRxFrame, pExplorerHeader, singleCastLen, RepeaterCountSessionTTL);
        TransportEnQueueExploreFrame(&exploreSearchResultBuffer,
                                     TRANSMIT_OPTION_EXPLORE_REPEAT,
                                     NULL);
      }
    }
#endif  /* ZW_REPEATER */
  }
}

/*============================== IsMyExploreFrame ===========================
**    Command handler for Explore frames
**
**  Side effects:
**
**--------------------------------------------------------------------------*/
void
IsMyExploreFrame(ZW_ReceiveFrame_t *pRxFrame)
{
  uint8_t bExploreFrameLength;
  uint8_t singleCastLen;
  uint8_t checksumBytes = 1;

  frameHeaderExplore *pExplorerHeader = (frameHeaderExplore *)pRxFrame->pPayloadStart;
  uint8_t bCurrentCmd = pExplorerHeader->ver_Cmd & EXPLORE_CMD_MASK;
  singleCastLen = (1 == llIsHeaderFormat3ch()) ? (sizeof(frameHeader3ch) + 1) : (sizeof(frameHeader) + 1);

  bExploreFrameLength = pRxFrame->frameContentLength - singleCastLen - 1; // remove the chksum byte length
  if ( (RF_PROFILE_100K == pRxFrame->profile) || (RF_PROFILE_3CH_100K == pRxFrame->profile) )
  {
    checksumBytes = 2;
    pRxFrame->frameContent[7]--; // remove the 2nd byte of the crc 16-bit word
    bExploreFrameLength--;
  }
  /* Check if it is my homeID */
  if (memcmp(pRxFrame->frameOptions.homeId, ZW_HomeIDGet(), HOMEID_LENGTH))
  {
    pRxFrame->status = STATUS_NOT_MY_HOMEID;
  }
  else
  {
    pRxFrame->status = 0;
  }

  DPRINTF("IsMyExp: %X:%02X:%02X\n", g_nodeID, g_learnMode, bNetworkWideInclusion);
  /* If in JOIN or LEAVE mode then drop all received explore frames */
  if ((bNetworkWideInclusion == NETWORK_WIDE_MODE_JOIN) ||
      (bNetworkWideInclusion == NETWORK_WIDE_MODE_LEAVE) ||
      ((sizeof(frameHeaderExplore) ) > bExploreFrameLength) ||
      /* Min Explore Search Result frame length */
      ((bCurrentCmd == EXPLORE_CMD_SEARCH_RESULT) && ((sizeof(EXPLORE_SEARCH_RESULT_FRAME)) > bExploreFrameLength)) ||
      /* Min Explore Auto Inclusion Request frame length */
      ((bCurrentCmd == EXPLORE_CMD_AUTOINCLUSION) && ((sizeof(frameHeaderExplore) + sizeof(EXPLORE_REMOTE_INCLUDED_NODE_INFORMATION_FRAME) ) > bExploreFrameLength)))
  {
    /* Explore frame length is lower than minimum context sensitive Explore frame length */
    return;
  }

  /* If EXPLORE_OPTION_STOP bit is set then : */
  /* Purge already present explore frames in explore Queue, if received frame originated from THIS node */
  if ((pExplorerHeader->option & EXPLORE_OPTION_STOP) &&
      (pRxFrame->frameOptions.destinationNodeId != g_nodeID)
#ifdef ZW_CONTROLLER_BRIDGE
      && !ZW_IsVirtualNode(pRxFrame->frameOptions.destinationNodeId)
#endif
     )
  {
    ExplorePurgeQueue(pRxFrame->frameOptions.sourceNodeId);
  }

  if (!IsExploreRouteValid(pExplorerHeader, bExploreFrameLength))
  {
    // Explore frame with non valid Explore route received - just drop it.
    return;
  }

  /*Drop an application frame if it is mine and the protocol to App task queue is full*/
  if ((pRxFrame->frameOptions.destinationNodeId == g_nodeID) &&  !(pRxFrame->status & STATUS_NOT_MY_HOMEID) )
  {
    uint8_t * pPayloadPtr = pRxFrame->pPayloadStart + sizeof(frameHeaderExplore);
    if ((ZWAVE_CMD_CLASS_APPL_MIN <= pPayloadPtr[0] )  &&  ProtocolInterfaceFramesToAppQueueFull() )
    {
      return;
    }
  }

#if defined(USE_RESPONSEROUTE) || defined(ZW_CONTROLLER)
    /* Only allow RouteCacheUpdate functionality to cache Route ONCE. */
  RouteCachedReset();
#endif

  TransportSetCurrentRxChannel(pRxFrame->channelId);
  TransportSetCurrentRxSpeedThroughProfile((uint32_t)pRxFrame->profile);

  /* For now we have only SearchResult frame as source routed explore */
  if (bCurrentCmd == EXPLORE_CMD_SEARCH_RESULT)
  {
    ExploreSearchResultHandle(pRxFrame, pExplorerHeader, singleCastLen, checksumBytes);
    /* SearchResult Explore frames should always by ignored by later stages */
    return;
  }
#ifdef ZW_REPEATER
  else
  {
    /* Loop prevention #1 - do not ever repeat a frame which have been repeated before */
    for (uint32_t i = 0; i < 4; i++)
    {
      if (pExplorerHeader->repeaterList[i] == g_nodeID
#ifdef ZW_CONTROLLER_BRIDGE
          || ZW_IsVirtualNode(pExplorerHeader->repeaterList[i])
#endif
         )
      {
        return;
      }
    }
  }
#endif
  /* Loop prevention #2 - Check if the frame should be ignored - only ignore if we are not the destination */
  if (ExploreFrameIgnoreCheck(pRxFrame->frameOptions.sourceNodeId,
                              pRxFrame->frameOptions.sequenceNumber))
  {
    /* Discard the received explore frame */
    DPRINT("Discard explore frame\r\n");
    return;
  }
#ifdef ZW_REPEATER
//  DPRINTF("ExploreInclusinoModeRepeat %d\r\n", exploreInclusionModeRepeat);
  /* Is the frame for us? */
  /* If the explore command is an AUTOINCLUSION cmd and networkHomeID is either ZERO or equal homeID then repeat */
  if ((pRxFrame->frameOptions.destinationNodeId != g_nodeID) &&
#ifdef ZW_CONTROLLER_BRIDGE
      !ZW_IsVirtualNode(pRxFrame->frameOptions.destinationNodeId) &&
#endif
#ifdef ZW_CONTROLLER
      ((bNetworkWideInclusion != NETWORK_WIDE_MODE_INCLUDE) &&
       (bNetworkWideInclusion != NETWORK_WIDE_MODE_EXCLUDE) &&
       (bNetworkWideInclusion != NETWORK_WIDE_SMART_START)  &&
       (bNetworkWideInclusion != NETWORK_WIDE_SMART_START_NWI)) &&

#endif
#ifdef ZW_BEAM_RX_WAKEUP
      /* TO#2194 fix - A FLiRS should never be used in Explore frame Flooding */
      (0 == ZW_nodeIsFLiRS()) &&
#endif
      /* TO#2214 fix - none listening repeater nodes must never repeat explore frames */
      (0 != ZW_nodeIsListening()) &&
      (((bCurrentCmd != EXPLORE_CMD_AUTOINCLUSION) && ((pRxFrame->status & STATUS_NOT_MY_HOMEID) == 0)) ||
       (exploreInclusionModeRepeat && (bCurrentCmd == EXPLORE_CMD_AUTOINCLUSION) &&
        IsNetworkHomeIDValid((uint8_t*)(pExplorerHeader->repeaterList + MAX_REPEATERS)))))
  {
    /* Not especially for us (could be broadcast) - then it should probably be repeated, if can do */
//    DPRINT("Frame not uni cast for us\r\n");

    /* Check if the Explore queue is in use */
    if ((bExploreQueueIndex = ExploreQueueIndex()) == EXPLORE_QUEUE_MAX)
    {
      return;
    }

    /* TO#4144 memory overwrite fix */
    if (sizeof(pExploreQueueElement->exploreFrame) < pRxFrame->frameContentLength)
    {
      /* Frame to big no room for frame */
      return;
    }

    /* If its a Network Wide Inclusion request we need to set the networkHomeID */
    if (bCurrentCmd == EXPLORE_CMD_AUTOINCLUSION)
    {
      /* We do not care if its allready set - set it anyway. */
      memcpy((uint8_t*)(pExplorerHeader->repeaterList + MAX_REPEATERS), ZW_HomeIDGet(), HOMEID_LENGTH);
    }
    pExploreQueueElement = &exploreQueue[bExploreQueueIndex];
    pExploreQueueElement->exploreFrame.pExploreHeader = (frameHeaderExplore *)pExploreQueueElement->exploreFrame.baseFrame.payload;

    pExploreQueueElement->exploreFrame.baseFrame.payloadLength = bExploreFrameLength;
    /* The frame should be repeated */
    /* Copy the explore header and payload contents to offset 0 in the TX payload buffer  */
    memcpy(pExploreQueueElement->exploreFrame.baseFrame.payload, (uint8_t*)pExplorerHeader, bExploreFrameLength);

    /* Update Ignore Explore Frame Queue with sourceID and sequence number */
    /* Make it less likely that we repeat the explore frame again when we receive it from our neighbors */
    uint32_t iAvailableIgnoreQueueEntryIndex = ExploreFrameIgnoreQueueIndex(pRxFrame->frameOptions.sourceNodeId);
    exploreFrameIgnoreQueue[iAvailableIgnoreQueueEntryIndex].handle = pRxFrame->frameOptions.sequenceNumber;
    ActivateExploreFrameIgnoreQueueEntry(iAvailableIgnoreQueueEntryIndex, EXPLORE_FRAME_IGNORE_TIMEOUT);
    /* Should the frame be repeated according to the TTL - is it ZERO? */
    uint32_t FrameTTL = (pExplorerHeader->repeaterCountSessionTTL & EXPLORE_REPEATERCOUNTSESSIONTTL_TTL_MASK);
    /* Only repeat if TTL is not zero and TTL is sane */
    if ((FrameTTL != 0) && ((MAX_REPEATERS << 4) >= FrameTTL))
    {
//      DPRINT("frame TTL valid and non zero\r\n");
      /* Decrement the explore frame TTL */
      FrameTTL -= EXPLORE_REPEATERCOUNTSESSIONTTL_TTL_UNIT;

      /* Get current repeaterCount */
      uint32_t CurrentRepeaterCount = (pExplorerHeader->repeaterCountSessionTTL & EXPLORE_REPEATERCOUNTSESSIONTTL_COUNT_MASK);
      /* TO#4144 */
      /* Only repeat if repeaterCount is sane */
      if (MAX_REPEATERS > CurrentRepeaterCount)
      {
        /* Insert nodeID as repeater in repeaterList */
        memcpy(&(pExploreQueueElement->exploreFrame.baseFrame.frameOptions), &(pRxFrame->frameOptions), sizeof(ZW_BasicFrameOptions_t));
        pExploreQueueElement->exploreFrame.pExploreHeader->repeaterList[CurrentRepeaterCount] = g_nodeID;
        /* Insert new repeaterCount and TTL in frame to repeat - remember to inc the repeaterCount */
        pExploreQueueElement->exploreFrame.pExploreHeader->repeaterCountSessionTTL = FrameTTL + CurrentRepeaterCount + 1;
        pExploreQueueElement->state.srcNode = pRxFrame->frameOptions.sourceNodeId;
        pExploreQueueElement->state.destNode = pRxFrame->frameOptions.destinationNodeId;

        /* A new explore frame is to be transmitted */
        pExploreQueueElement->state.exploreState = EXPLORE_STATE_NEW;
        /* Tell transmitter not to do anything with the frame as it is as it should */
        pExploreQueueElement->state.exploreOptions = TRANSMIT_OPTION_EXPLORE_REPEAT;
        pExploreQueueElement->state.payloadLength = bExploreFrameLength - sizeof(frameHeaderExplore);
        ZW_TransmitCallbackUnBind(&pExploreQueueElement->state.func);

        // Ensure Explore machine is updated - As new explore frame has been added
        SetExploreMachineUpdate();
      }
    }
  }
#endif  /* ZW_REPEATER */
#ifdef ZW_CONTROLLER
  DPRINTF("R2: %x:%x:%x:%x:%x\n", bCurrentCmd, pRxFrame->frameOptions.destinationNodeId, bNetworkWideInclusionReady, assign_ID.assignIdState, (pRxFrame->status & STATUS_NOT_MY_HOMEID));
  memcpy(crH, pRxFrame->frameOptions.homeId, HOMEID_LENGTH);
  memset((uint8_t*)&rxopt, 0, sizeof(rxopt));
  EXPLORE_REQUEST_FRAME *pExploreRequestFrame = (EXPLORE_REQUEST_FRAME*)(pRxFrame->pPayloadStart + sizeof(frameHeaderExplore));
  if ((((pRxFrame->frameOptions.destinationNodeId == g_nodeID) ||
#ifdef ZW_CONTROLLER_BRIDGE
       ZW_IsVirtualNode(pRxFrame->frameOptions.destinationNodeId) ||
#endif
       (pRxFrame->frameOptions.destinationNodeId == NODE_BROADCAST)) &&
      ((((pRxFrame->status & STATUS_NOT_MY_HOMEID) == 0) &&
        (bCurrentCmd == EXPLORE_CMD_NORMAL)) ||
       ((((bNetworkWideInclusion == NETWORK_WIDE_MODE_INCLUDE) || (bNetworkWideInclusion == NETWORK_WIDE_SMART_START_NWI))&&
         (bCurrentCmd == EXPLORE_CMD_AUTOINCLUSION) && bNetworkWideInclusionReady &&
         IsNetworkHomeIDValid((uint8_t*)(pExplorerHeader->repeaterList + MAX_REPEATERS)) &&
         (assign_ID.assignIdState == ASSIGN_IDLE)))))
      // Do not accept the frame if the controller is in NWE or Remove Node mode.
      && (((bNetworkWideInclusion != NETWORK_WIDE_MODE_EXCLUDE) && (g_learnNodeState != LEARN_NODE_STATE_DELETE)) ||
          // Do accept the frame if the controller is in NWE and it is a NETWORK_WIDE_MODE_EXCLUDE command.
          ((bNetworkWideInclusion == NETWORK_WIDE_MODE_EXCLUDE) &&
           (bCurrentCmd == EXPLORE_CMD_NORMAL) &&
           (ZWAVE_CMD_CLASS_PROTOCOL == pExploreRequestFrame->exclusionRequest.header.cmdClass) &&
           (ZWAVE_CMD_EXCLUDE_REQUEST == pExploreRequestFrame->exclusionRequest.header.cmd))
         )
      )
#else
  if (((pRxFrame->frameOptions.destinationNodeId == g_nodeID) ||
       (pRxFrame->frameOptions.destinationNodeId == NODE_BROADCAST)) &&
      (((pRxFrame->status & STATUS_NOT_MY_HOMEID) == 0) &&
       (bCurrentCmd == EXPLORE_CMD_NORMAL)))
#endif
  {
    /* The frame is for me */
    pRxFrame->status |= STATUS_FRAME_IS_MINE;
    /* Payload now points to the piggybacked payload - if any... */
    pRxFrame->pPayloadStart = pRxFrame->pPayloadStart + sizeof(frameHeaderExplore);
    pRxFrame->frameContentLength = pRxFrame->frameContentLength - (sizeof(frameHeaderExplore) + singleCastLen + checksumBytes);
#ifdef ZW_CONTROLLER
    /* For controllers also save LWR if received explore frame was Broadcasted */
    if (bCurrentCmd != EXPLORE_CMD_AUTOINCLUSION)
    {
      if (CACHED_ROUTE_NONE == LastWorkingRouteCacheLineExists(pRxFrame->frameOptions.sourceNodeId, CACHED_ROUTE_ZW_ANY))
      {
        // If no cached route exist for this node; save the route as LWR
        /* Save the received route as the LastWorkingRoute to the transmitting node */
        LastWorkingRouteCacheLineExploreUpdate(pRxFrame->frameOptions.sourceNodeId, false, &pExplorerHeader->repeaterCountSessionTTL);
      }
    }
#endif  /* ZW_CONTROLLER */
    if (pRxFrame->frameOptions.acknowledge
#ifdef ZW_CONTROLLER
        || ((NETWORK_WIDE_SMART_START_NWI == bNetworkWideInclusion) &&
            (ZWAVE_CMD_SMARTSTART_INCLUDE_NODE_INFO == ((EXPLORE_REMOTE_INCLUSION_REQUEST_FRAME*)pRxFrame->pPayloadStart)->header.cmd) &&
            (memcmp(addSmartNodeHomeId, crH, HOMEID_LENGTH) == 0))
#endif
       )
    {
#ifdef ZW_CONTROLLER
      if (bCurrentCmd == EXPLORE_CMD_AUTOINCLUSION)
      {
        /* Now we do not want to let any other node (NWI wise) - this is the one */
        bNetworkWideInclusionReady = false;
        /* The autoInclusion payload contains a networkHomeID, which is only used for filtering and */
        /* must be skipped for the CommandHandler to be able to handle the autoInclusion frame correctly */
        pRxFrame->pPayloadStart += HOMEID_LENGTH;
        pRxFrame->frameContentLength -= HOMEID_LENGTH;
        /* Get Repeater count */
        uint32_t RepeaterCount = (pExplorerHeader->repeaterCountSessionTTL & EXPLORE_REPEATERCOUNTSESSIONTTL_COUNT_MASK);
        DPRINTF("Repeater count %d\r\n", RepeaterCount);
        /* Have explore frame been repeated - any repeater... */
        if (RepeaterCount)
        {
          /* Its a repeated frame. */
          /* Outgoing routed frame - gets set by BuildTxHeader */
          assignIDBuffer[0] = 0;
          /* Set NumRepeaters */
          assignIDBuffer[1] = (RepeaterCount << 4);
          for (uint32_t i = 0; i < RepeaterCount; i++)
          {
            assignIDBuffer[2 + i] = pExplorerHeader->repeaterList[(RepeaterCount - 1) - i];
            DPRINTF("Repeater %02X\r\n", assignIDBuffer[2 + i]);
          }
          /* Get length of routing header */
          RepeaterCount += 2;
        }
#ifdef TEST_NO_DIRECT_INCLUDE
        else
        {
          bNetworkWideInclusionReady = true;
          pRxFrame->status = 0;
          return;
        }
#endif
        /* RoutingHeaderLength now contains length of routing header - if ZERO then no routing header present; meaning DIRECT */
        uint32_t RoutingHeaderLength = RepeaterCount;
        /* We need to convert payload into ZWAVE_CMD_CLASS_PROTOCOL */
        /* So that CommandHandler handles it like we want to... */
        /* The explore autoinclusion frame */
        /* Tell CommandHandler not to act on the nodeinfo frame by transmitting a assignID frame */
        /* as we need to inject the source route to be used in reaching the node wanting inclusion */
        exploreRemoteInclusion = true;
        /* Clear rxopt before passing to upper layer */
        rxopt.rxStatus = RECEIVE_STATUS_TYPE_BROAD | RECEIVE_STATUS_TYPE_EXPLORE;
        uint32_t* pHomeId = (uint32_t *)pRxFrame->frameOptions.homeId; // Use pHomeId to avoid GCC type punned pointer error
        rxopt.homeId.word = *pHomeId;
        rxopt.destNode = pRxFrame->frameOptions.destinationNodeId;
        rxopt.sourceNode = pRxFrame->frameOptions.sourceNodeId;
        rxopt.rxRSSIVal = pRxFrame->rssi;

        CommandHandler_arg_t args = {
          .cmd       = pRxFrame->pPayloadStart,
          .cmdLength = pRxFrame->frameContentLength,
#if defined(ZW_CONTROLLER_BRIDGE)
          .multi     = NULL,
#endif
          .rxOpt     = &rxopt
        };

        CommandHandler(&args);

        /* Now release the CommandHandler again so that it can take over the inclusion */
        /* when we are done injecting the source route to be used in the inclusion process */
        exploreRemoteInclusion = false;
        /* Now inject the source route if assignID is possible */
        if (assign_ID.assignIdState != ASSIGN_IDLE)
        {
          memcpy(&assignIDBuffer[RoutingHeaderLength], (uint8_t *)&assignIdBuf, sizeof(ASSIGN_IDS_FRAME));
          RoutingHeaderLength += sizeof(ASSIGN_IDS_FRAME);
        }
        else
        {
          return;
        }
        /* Now we have determined the node we want to include */
        bNetworkWideInclusion = NETWORK_WIDE_MODE_IDLE;
        /* We now have the nodeID on the node to include - if succesfull */
        /* Save the received route as the LastWorkingRoute for this new node */
        LastWorkingRouteCacheLineExploreUpdate(assignIdBuf.assignIDFrame.newNodeID, false, &pExplorerHeader->repeaterCountSessionTTL);
        /* Protocol has Locked LWR purging */
        ZW_LockRoute(true);
        currentSeqNoUseTX = true;
        /* transmit the frame */
        uint8_t len = RoutingHeaderLength + ((RoutingHeaderLength > sizeof(ASSIGN_IDS_FRAME)) ? 1 : 0);
        TxOptions_t txOptions = (RoutingHeaderLength > sizeof(ASSIGN_IDS_FRAME)) ? (TRANSMIT_OPTION_EXPLORE_REPEAT | TRANSMIT_OPTION_ROUTED | TRANSMIT_OPTION_ACK) : TRANSMIT_OPTION_ACK;
        const STransmitCallback TxCallback = { .pCallback = ZCB_AssignTxComplete, .Context = 0 };
        EnQueueSingleData(RF_SPEED_40K,
                          g_nodeID,
                          pRxFrame->frameOptions.sourceNodeId,
                          assignIDBuffer,
                          len,
                          txOptions, 0, // 0ms for tx-delay (any value)
                          ZPAL_RADIO_TX_POWER_REDUCED,
                          &TxCallback);
        return;
      }
      else
#endif  /* ZW_CONTROLLER */
      {
#ifdef ZW_SLAVE
        /* Save the received route as the LastWorkingRoute to the transmitting node */
        /* TO#2220 partial FEATURE - added the update of Return routes if a response route is received. */
        UpdateResponseRouteLastReturnRoute(StoreRouteExplore(pRxFrame->frameOptions.sourceNodeId, false, &pExplorerHeader->repeaterCountSessionTTL));
#endif
        /* Copy the exploreheader and payload contents from the received explore frame */
        exploreSearchResultBuffer.baseFrame.payloadLength = sizeof(EXPLORE_SEARCH_RESULT_FRAME);
        memcpy(exploreSearchResultBuffer.baseFrame.payload, (uint8_t*)pExplorerHeader, exploreSearchResultBuffer.baseFrame.payloadLength);

        /* Remove ACK request bit - As we do not want an ACK for our Search Result frame. */
        exploreSearchResultBuffer.baseFrame.frameOptions.acknowledge = 0;
        exploreSearchResultBuffer.baseFrame.frameOptions.frameType = HDRTYP_EXPLORE;
        // copy sequence number
        exploreSearchResultBuffer.baseFrame.frameOptions.sequenceNumber = pRxFrame->frameOptions.sequenceNumber;
        // Make sure we use correct homeID
        memcpy(exploreSearchResultBuffer.baseFrame.frameOptions.homeId, pRxFrame->frameOptions.homeId, HOMEID_LENGTH);
        /* Make sure we are the originator */
#ifdef ZW_CONTROLLER_BRIDGE
        exploreSearchResultBuffer.baseFrame.frameOptions.sourceNodeId = pRxFrame->frameOptions.destinationNodeId;
#else
        exploreSearchResultBuffer.baseFrame.frameOptions.sourceNodeId = g_nodeID;
#endif
        /* Make sure the destination is the originator of the received explore frame needing a result */
        exploreSearchResultBuffer.baseFrame.frameOptions.destinationNodeId = pRxFrame->frameOptions.sourceNodeId;

        EXPLORE_SEARCH_RESULT_FRAME* pExploreSearchResultPtr = (EXPLORE_SEARCH_RESULT_FRAME*) exploreSearchResultBuffer.baseFrame.payload;

        /* The explore frame command is SearchResult */
        pExploreSearchResultPtr->exploreHeader.ver_Cmd = EXPLORE_VER_DEFAULT | EXPLORE_CMD_SEARCH_RESULT;

        /* The explore frame is source routed, the direction is backwards and we want all pending explore frames purged  */
        pExploreSearchResultPtr->exploreHeader.option = EXPLORE_OPTION_SOURCEROUTED + EXPLORE_OPTION_DIRECTION + EXPLORE_OPTION_STOP;

        /* Tell the listning nodes which frameHandle the STOP is meant for */
        pExploreSearchResultPtr->frameHandle = exploreSearchResultBuffer.baseFrame.frameOptions.sequenceNumber;

        /* Tell the listning nodes which nodeID the STOP is meant for */
        pExploreSearchResultPtr->nodeID = pRxFrame->frameOptions.sourceNodeId;

        /* copy Explore frame route result */
        memcpy(&pExploreSearchResultPtr->repeaterCountSessionTTL,
               &pExplorerHeader->repeaterCountSessionTTL,
               sizeof(EXPLORE_SEARCH_RESULT_FRAME) - sizeof(frameHeaderExplore) - 2);

        currentSeqNoUseTX = false;
        TransportEnQueueExploreFrame(&exploreSearchResultBuffer,
                                     TRANSMIT_OPTION_EXPLORE_REPEAT/* | TRANSMIT_OPTION_ROUTED*/, NULL);
      }
    }
  }
#ifdef ZW_CONTROLLER
  else if ((bCurrentCmd == EXPLORE_CMD_AUTOINCLUSION) &&
           IsNetworkHomeIDValid((uint8_t*)(pExplorerHeader->repeaterList + MAX_REPEATERS)))

  {
    DPRINT("smart start frame \r\n");
    /* The frame is for me */
    pRxFrame->status &= ~STATUS_FRAME_IS_MINE;
    // Let pPayloadStart point to the EXPLORE_REMOTE_INCLUSION_REQUEST_FRAME structure start
    pRxFrame->pPayloadStart = pRxFrame->pPayloadStart + sizeof(frameHeaderExplore);
    if (bNetworkWideInclusion == NETWORK_WIDE_SMART_START)
    {
      if ((ZWAVE_CMD_SMARTSTART_PRIME_NODE_INFO ==
          ((EXPLORE_REMOTE_INCLUSION_REQUEST_FRAME*)pRxFrame->pPayloadStart)->header.cmd))
      {
        pRxFrame->status |= STATUS_FRAME_IS_MINE;
        memcpy(inclusionHomeID, pRxFrame->frameOptions.homeId, HOMEID_LENGTH);
        rxopt.rxStatus = RECEIVE_STATUS_SMART_NODE;
      }
    }
    else if  (bNetworkWideInclusion == NETWORK_WIDE_SMART_START_NWI)
    {
      if  ((ZWAVE_CMD_SMARTSTART_INCLUDE_NODE_INFO ==
            ((EXPLORE_REMOTE_INCLUSION_REQUEST_FRAME*)pRxFrame->pPayloadStart)->header.cmd) &&
           memcmp(pRxFrame->frameOptions.homeId, ZW_HomeIDGet(), HOMEID_LENGTH) &&
           !memcmp(pRxFrame->frameOptions.homeId, inclusionHomeID, HOMEID_LENGTH) )
      {
        pRxFrame->status |= STATUS_FRAME_IS_MINE;
      }
    }

    if (ZWAVE_CMD_INCLUDED_NODE_INFO == ((EXPLORE_REMOTE_INCLUDED_NODE_INFORMATION_FRAME*)pRxFrame->pPayloadStart)->header.cmd)
    {
      pRxFrame->status |= STATUS_FRAME_IS_MINE;
    }

    // the pPayloadStart should noew point to the header field of the  EXPLORE_REMOTE_INCLUSION_REQUEST_FRAME structure
    pRxFrame->pPayloadStart +=HOMEID_LENGTH;
    pRxFrame->frameContentLength = pRxFrame->frameContentLength - (sizeof(frameHeaderExplore) + HOMEID_LENGTH + singleCastLen + checksumBytes);
    if (0 == (pRxFrame->status & STATUS_FRAME_IS_MINE))
    {
      return;
    }

  }
#endif // ZW_CONTROLLER
  else
  {
    return;
  }
  DPRINT("m\r\n");
#ifdef ZW_CONTROLLER
  rxopt.rxStatus |= RECEIVE_STATUS_TYPE_BROAD | RECEIVE_STATUS_TYPE_EXPLORE;
  uint32_t* pHomeId = (uint32_t *)pRxFrame->frameOptions.homeId; // Use pHomeId to avoid GCC type punned pointer error
  rxopt.homeId.word = *pHomeId;
  rxopt.rxRSSIVal = pRxFrame->rssi;
#endif
  rxopt.sourceNode = pRxFrame->frameOptions.sourceNodeId;
  rxopt.destNode   = pRxFrame->frameOptions.destinationNodeId;

  CommandHandler_arg_t args = {
    .cmd       = pRxFrame->pPayloadStart,
    .cmdLength = pRxFrame->frameContentLength,
#if defined(ZW_CONTROLLER_BRIDGE)
    .multi     = NULL,
#endif
    .rxOpt     = &rxopt
  };

  CommandHandler(&args);
}

#ifdef ZW_REPEATER
/* Only repeaters need this functionality */

static const uint32_t iNwiTimeoutMs = (5 * 60 * 1000);
static SSwTimer networkWideInclusionModeTimer;

/*==============================   NWIStopTimer   ============================
**    Stop NWI timer
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
static void
NWIStopTimer(void)
{
  TimerStop(&networkWideInclusionModeTimer);
  zpal_radio_network_id_filter_set(true);
}

/*===============================   NWITimeout   =============================
**    NWI mode timeout handler
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
static void
ZCB_NWITimeout(__attribute__((unused)) SSwTimer* pTimer)
{
  exploreInclusionModeRepeat = false;
  NWIStopTimer();
}

/*==============================   NWIInitTimer   ============================
**    Init NWI timer
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
void
NWIInitTimer(void)
{
  ZwTimerRegister(&networkWideInclusionModeTimer,
                  false,
                  ZCB_NWITimeout
                  );
}


/*=============================   NWIStartTimeout   ==========================
**    Set NWI mode according to mode.
**    If mode is not NWI_IDLE a timer is started making the node return
**    to NWI_IDLE mode on timeout.
**
**    Current NWI modes are:
**      NWI_IDLE
**      NWI_REPEAT
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
static void
NWIStartTimer(void)
{
  TimerStart(&networkWideInclusionModeTimer, iNwiTimeoutMs);
  zpal_radio_network_id_filter_set(false);
}


/*=============================   ExploreSetNWI   ===========================
**    Set NWI mode according to mode and timeout.
**    Start timer if mode equals NWI_REPEAT which makes the node exit the
**    repeat mode after timeout.
**    If timeout is ZERO then timeout is set to 5min.
**
**    Current NWI modes are:
**      NWI_IDLE
**      NWI_REPEAT
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
void
ExploreSetNWI(
  uint8_t mode)
{
  bNetworkWideInclusion = NETWORK_WIDE_MODE_IDLE;
  exploreInclusionModeRepeat = mode;

  if (mode)
  {
    NWIStartTimer();
  }
  else
  {
    NWIStopTimer();
  }
}
#endif /* ZW_REPEATER */
