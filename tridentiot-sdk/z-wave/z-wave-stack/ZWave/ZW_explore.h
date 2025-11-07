// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file ZW_explore.h
 * Explore module.
 * Module assists transport module with handling explorer frames. 
 * 
 * Module contains 4 queues:
 * 1: An event queue, feeding the state machine with events to react on.
 * 2: An explore frame queue, containing active explorer frames and their state.
 * 3: An explore ignore queue, containing information about explore packets to
 * ignore to avoid repeating the same explorer packet multiple times as it may
 * be received from different nodes.
 * 
 * Module contains two timer systems for keeping track of timeouts in the explore
 * frame queue and the explore ignore queue (a timer system for each)
 *
 * Module contains a state machine handling the frames in the explore frame queue.
 * The state machine has an instatiation for every entry in the explore frame queue.
 *
 * The vent queue is shared for the entires in the expliore frame queue. The state
 * state handlers are designed to be able to check if an event is relevant for the
 * individual frame, or only one frame is kept in specific states at a time (via
 * flag bExploreSlotTaken).
 *
 * The state machine has the following states:
 * New: The first state of a new explore frame. Discards frames with unsupported
 * state.cmd and starts a timeout timer (based on number of Hops etc.) for
 * approved frames. Short transitional state.
 * Delayed: Waits for timeout and resources to be available.
 * Transmit: Is a short transitional state. Queues frame for transmit and configures
 * it according to frame 'repeat' option. Starts a timeout timer.
 * Repeat: Awaits results of frame transmission and timeout timer
 * Search: Awaits results of frame transmission and timeout timer
 * Execute: Short transitional state. Cleans up - frees explore frame queue entry,
 * stops timeout timer if running etc.
 * Empty: Awaiting new frame.
 *
 * @startuml
 * title Explore state machine
 * [*] --> New : ExploreQueueFrame() - new explore frame
 * New -left-> Empty : Frame command is unsupported - Discard
 * New --> Delayed : Frame command is valid
 * New : Discard frames with unsupported commands
 * New : Starts timer on transition to delayed
 * Delayed --> Transmit : Timeout && !bExploreSlotTaken
 * Delayed : Wait for Timeout and available Explore slot
 * Delayed : Timer inactive on state exit
 * Transmit --> Repeat : Frame option 'repeat' && Frame queue for Tx OK
 * Transmit --> Search : NOT frame option 'repeat' && Frame queue for Tx OK
 * Transmit --> Execute : Frame queue for Tx FAIL
 * Transmit : Queues frame for transmit
 * Transmit : Starts timer on state exit
 * Repeat --> Execute : Timeout
 * Repeat --> Execute : Frame Transmitted
 * Repeat --> Execute : Search result received
 * Repeat : Awaits result of frame transmission
 * Search --> Execute : Timeout
 * Search --> Execute : Frame Transmitted
 * Search --> Execute : Search result received
 * Search : Awaits result of frame transmission
 * Execute --> Empty : Unconditional
 * Execute : Stops timer (if active) on state exit * 
 * Empty -->[*]
 * @enduml
 *
 * @copyright 2022 Silicon Laboratories Inc.
 */
#ifndef _ZW_EXPLORE_H_
#define _ZW_EXPLORE_H_

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <ZW_typedefs.h>
#include <ZW_protocol.h>
#include <SyncEvent.h>
#include <ZW_Frame.h>
#include <ZW_transport_transmit_cb.h>
#include <ZW_basis_api.h>

/****************************************************************************/
/*                     EXPORTED TYPES and DEFINITIONS                       */
/****************************************************************************/

/* Valid state values for exploreState */
#define EXPLORE_STATE_EMPTY             0
#define EXPLORE_STATE_NEW               1
#define EXPLORE_STATE_DELAYED           2
#define EXPLORE_STATE_TRANSMIT          4
#define EXPLORE_STATE_SEARCH            5
#define EXPLORE_STATE_REPEAT            6
#define EXPLORE_STATE_EXECUTE           7


/* Explore frame command are shifted into the high nibble in the bExploreOptions when */
/* calling ExploreQueueFrame */
/* Defines for handling Explore Cmd in ExploreQueueFrame when shifted into bExploreOptions */
#define QUEUE_EXPLORE_CMD_MASK          0xF0
#define QUEUE_EXPLORE_CMD_NORMAL        (EXPLORE_CMD_NORMAL << 4)
#define QUEUE_EXPLORE_CMD_AUTOINCLUSION (EXPLORE_CMD_AUTOINCLUSION << 4)
#define QUEUE_EXPLORE_CMD_STOPEXPLORE   (EXPLORE_CMD_STOPEXPLORE << 4)

/* Smart Start ExploreInclusionRequest do not set ACK in frameheader but we still need to wait */
/* for frame timeout if TRANSMIT_OPTION_EXPLORE_DO_ACK_TIMEOUT bit is set */
#define TRANSMIT_OPTION_EXPLORE_DO_ACK_TIMEOUT  0x08

enum NETWORK_WIDE_MODE
{
  /* Network Wide Inclusion mode idle */
  NETWORK_WIDE_MODE_IDLE = 0,
  /* Node is set in Network Wide Inclusion mode */
  NETWORK_WIDE_MODE_JOIN,
  /* Node is set in Network Wide Exclusion mode */
  NETWORK_WIDE_MODE_LEAVE,
  /* Node repeats Network Wide Inclusion requests */
  NETWORK_WIDE_MODE_REPEAT,
#ifdef ZW_CONTROLLER
  /* Controller is set in mode which allows it to do a Network Wide Exclusion */
  NETWORK_WIDE_MODE_EXCLUDE,
  /* Controller is set in mode which allows it to do a Network Wide Inclusion */
  NETWORK_WIDE_MODE_INCLUDE,
  /* Controller is set in mode which will push Node Information Frames to upper layer for provisioning list processing */
  NETWORK_WIDE_SMART_START,
  /* Controller is set in mode which allows it to do a Smart Start Network Wide Inclusion */
  NETWORK_WIDE_SMART_START_NWI,
#endif
  /* Number of defined Network Wide mode states */
  NETWORK_WIDE_MODE_COUNT
};


/* Z-Wave protocol cmd - transmitted in a explore frame */
/* cmdClass is ZWAVE_CMD_CLASS_PROTOCOL (0x01) - Thereby in the protocol specific area */
/* Is used when the explore frame destination answers the explore frame via */
/* the explored route */
typedef struct _CMD_CLASS_HEADER_
{
  uint8_t  cmdClass;
  uint8_t  cmd;
} CMD_CLASS_HEADER;


typedef struct _EXPLORE_REMOTE_INCLUSION_REQUEST_FRAME_
{
  uint8_t                  networkHomeID[HOMEID_LENGTH];
  CMD_CLASS_HEADER      header;
  uint8_t                  capability;             /* Network capabilities */
  uint8_t                  security;               /* Network security */
  uint8_t                  reserved;
#ifdef ZW_CONTROLLER
  NODE_TYPE             nodeType;               /* Basic, Generic and Specific Device Type */
#else
  /* Slaves do not transmit Basic Device Type in the nodeinfo frame only Generic and Specific */
  APPL_NODE_TYPE        nodeType;          /* Generic and Specific Device Type */
#endif
  uint8_t                  nodeInfo[NODEPARM_MAX]; /* Device capabilities */
} EXPLORE_REMOTE_INCLUSION_REQUEST_FRAME;


typedef struct _EXPLORE_REMOTE_EXCLUSION_REQUEST_FRAME_
{
  CMD_CLASS_HEADER      header;
  uint8_t                  capability;             /* Network capabilities */
  uint8_t                  security;               /* Network security */
  uint8_t                  reserved;
#ifdef ZW_CONTROLLER
  NODE_TYPE             nodeType;               /* Basic, Generic and Specific Device Type */
#else
  /* Slaves do not transmit Basic Device Type in the nodeinfo frame only Generic and Specific */
  APPL_NODE_TYPE        nodeType;          /* Generic and Specific Device Type */
#endif
  uint8_t                  nodeInfo[NODEPARM_MAX]; /* Device capabilities */
} EXPLORE_REMOTE_EXCLUSION_REQUEST_FRAME;


/* INIF - The Payload parsed by IsMyExploreFrame - An EXPLORE_CMD_AUTOINCLUSION frame */
typedef struct _EXPLORE_REMOTE_INCLUDED_NODE_INFORMATION_FRAME_
{
  uint8_t                  networkHomeID[HOMEID_LENGTH];
  CMD_CLASS_HEADER      header;
  uint8_t                  smartStartNWIHomeID[HOMEID_LENGTH]; /* homeID used when using Smart Start */
} EXPLORE_REMOTE_INCLUDED_NODE_INFORMATION_FRAME;

/* INIF - The Payload send to ReceiveHandler */
typedef struct _REMOTE_INCLUDED_NODE_INFORMATION_FRAME_
{
  CMD_CLASS_HEADER      header;
  uint8_t                  smartStartNWIHomeID[HOMEID_LENGTH]; /* homeID used when using Smart Start */
} REMOTE_INCLUDED_NODE_INFORMATION_FRAME;

/* Explore Search result and STOP frame definition */
typedef struct _EXPLORE_SEARCH_RESULT_FRAME_
{
  frameHeaderExplore    exploreHeader;
  uint8_t                  nodeID;
  uint8_t                  frameHandle;
  uint8_t                  repeaterCountSessionTTL;
  uint8_t                  repeaterList[MAX_REPEATERS];
} EXPLORE_SEARCH_RESULT_FRAME;


#define EXPLORE_QUEUE_MAX 2

#define EXPLORE_MAX_PAYLOAD (RX_MAX_LEGACY - sizeof(frameHeader) - 1 - sizeof(frameHeaderExplore))

typedef struct _frameExploreStruct_
{
  frameHeaderExplore     * pExploreHeader;
  ZW_TransmissionFrame_t   baseFrame;
  //ZW_TransmissionFrame_t includes payload of 8 bytes. More space is allocated so frame can fit.
  uint8_t                  explorerPayload[EXPLORE_MAX_PAYLOAD - TRANSMISSION_FRAME_PAYLOAD_SIZE];
} frameExploreStruct;


typedef struct _exploreStateStruct_
{
  uint8_t exploreState;
  uint8_t exploreStatus;
  uint8_t srcNode;
  uint8_t destNode;
  uint8_t exploreOptions;
  uint8_t cmd;
  uint8_t payloadLength;
  bool bTimeoutActive;      /* Marks if timeout timing is active */
  uint32_t TimeoutTickTime; /* Tick time marks timeout time*/
  uint32_t StartTicks;      /* Start tick sample */
  TX_STATUS_TYPE sTxStatusReport;
  STransmitCallback func;
} exploreStateStruct;


typedef struct _exploreQueueElementStruct_
{
  exploreStateStruct state;
  frameExploreStruct exploreFrame;
} exploreQueueElementStruct;


/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/

extern bool exploreRemoteInclusion;
#ifdef ZW_REPEATER
extern bool exploreInclusionModeRepeat;
#endif
extern bool bNetworkWideInclusionReady;
extern exploreQueueElementStruct exploreQueue[EXPLORE_QUEUE_MAX];
/* Set to the used exploreQueue element if successful by ExploreQueueFrame */
/* Used by ReTransmitFail when resorting to explore frame route resolution */
/* Todo This pointer should be private */
extern exploreQueueElementStruct *pExploreQueueElement;
extern uint8_t bNetworkWideInclusion;

extern SSyncEvent ExploreMachineUpdateRequest;

/****************************************************************************/
/*                           EXPORTED FUNCTIONS                             */
/****************************************************************************/


/*===============================   ExploreInit   ============================
**    Init Explore module. Must be called prior to any other Explore methods
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
void
ExploreInit(void);

/**
 * Enqueues an INIF frame on the Explorer Queue.
 * @return Returns the return value of ExploreQueueFrame().
 */
uint8_t
ExploreINIF(void);


/*===============================   ExploreRequestInclusion   ================
**    Transmit a request for inclusion via an explore frame.
**
**--------------------------------------------------------------------------*/
uint8_t
ExploreRequestInclusion(
  uint8_t bNeedACK);


/*===============================   ExploreQueueFrame   =====================
**    Queue an explore frame destined for dstNodeID in Explore frame queue
**    if possible.
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
uint8_t          /*RE ZERO if ExploreFrame queue full */
ExploreQueueFrame(
  uint8_t bSrcNodeID,      /* IN Source node ID */
  uint8_t bDstNodeID,      /* IN Destination node ID, if equal to 0xFF then all nodes */
  uint8_t *pData,          /* IN Data buffer pointer           */
  uint8_t bDataLength,     /* IN Data buffer length            */
  uint8_t bExploreFlag,    /* IN Explore frame flag containing frame type and explore options */
  const STransmitCallback* pCompletedFunc); /*IN  Transmit completed call back function  */


/*===============================   ExploreComplete   =====================
**    Callback function - called when explore frame has been transmitted
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
void
ZCB_ExploreComplete(
  ZW_Void_Function_t Context, uint8_t bTxStatus, TX_STATUS_TYPE* TxStatusReport);


/*============================== IsMyExploreFrame ============================
**    Command handler for Explore frames
**
**  Side effects:
**
**--------------------------------------------------------------------------*/
//bool
//IsMyExploreFrame(
//  RX_FRAME *pRxFrame, /* IN Frame structure pointer */
//  uint8_t *pRssiArray);  /* IN RSSI samples taken during frame reception */
void IsMyExploreFrame(ZW_ReceiveFrame_t *pFrame);

/*============================== ExploreQueueIsFull =========================
**  Function to check if node allready has max explore frames in queue
**
**  Side effects:
**
**--------------------------------------------------------------------------*/
bool
ExploreQueueIsFull();


/*============================== ExplorePurgeQueue ===========================
**    Purges the Explore frame queue if needed
**
**  Side effects:
**
**--------------------------------------------------------------------------*/
void
ExplorePurgeQueue(
  uint8_t  sourceID);


/*============================= ExploreMachine ===============================
**  Main explore frame handling - must be called when requested via
**  ExploreMachineUpdateRequest
** 
**  Do NOT bind ExploreMachineUpdateRequest directly to this method. If you
**  do it will call it self recursively. Use it to set a flag or similar 
**  to ensure method is called at some point.
**
**  Side effects:
**
**--------------------------------------------------------------------------*/
void
ExploreMachine(void);


/*============================= ExploreSetTimeout ============================
**  Set a timeout on an exploreQueueElement
**
** TODO: This method should be private - but due to the explorer module being
** abused by the transport module, it has to be public for now.
**
** Handles practicalities around setting a timeout on a queue element.
** It configures the ExploreQueueTimer to run out when the first
** explore queue timeout occurs and sets the Timeout flags in the explore
** queue element state structs.
**
**  Side effects:
**    pCurrentExploreQueueElement->state.bTimeoutActive updated
**    pCurrentExploreQueueElement->state.TimeoutTickTime updated
**    ExploreQueueTimer may be updated
**
**--------------------------------------------------------------------------*/
void
ExploreSetTimeout(exploreQueueElementStruct* pQueueElement, uint32_t iTimeout);


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
  const STransmitCallback* pCompletedFunc);


/*==============================   NWIInitTimer   ============================
**    Init NWI timer
**    Must be called prior to other NWI timer methods.
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
void
NWIInitTimer(void);

/*=============================   ExploreSetNWI   ===========================
**    Set NWI mode according to mode and timeout.
**    Start timer if mode equals NWI_REPEAT which makes the exit the repeat mode after timeout.
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
  uint8_t mode);

#endif /* _ZW_EXPLORE_H_ */
