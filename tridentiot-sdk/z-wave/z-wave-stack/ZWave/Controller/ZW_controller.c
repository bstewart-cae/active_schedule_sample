// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file ZW_controller.c
 * @copyright 2022 Silicon Laboratories Inc.
 *
 * @brief Z-Wave Controller Application Interface.
 */
#include "ZW_lib_defines.h"

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <string.h>

#include "SizeOf.h"
#include <ZW.h>
#include <ZW_main.h>
#include <ZW_main_region.h>
#include <ZW_controller.h>
#include <ZW_classcmd.h>
#include <ZW_routing.h>
#include <ZW_routing_cache.h>
#include <ZW_basis_api.h>
#include <ZW_basis.h>
#include <ZW_protocol.h>
#include <zpal_radio_utils.h>

#include <ZW_timer.h>
#include <SwTimer.h>
#include <ZW_replication.h>
#include "ZW_home_id_generator.h"
#include "ZW_CCList.h"
//#define DEBUGPRINT
#include <DebugPrint.h>
#include <ZW_controller.h>
#include <ZW_MAC.h>
#include <ZW_explore.h>

#include <ZW_noise_detect.h>

#include <Assert.h>
#include "ZW_controller_network_info_storage.h"
#include <ZW_nvm.h>

#include <ZW_keystore.h>
#include <ZW_network_management.h>
#include <ZW_frames_filters.h>
#include "ZW_DataLinkLayer_utils.h"
#include <ZW_dynamic_tx_power.h>
#include "ZW_lr_virtual_node_id.h"
#include <zpal_radio.h>
#include <ZW_inclusion_controller.h>
#include <ZW_protocol_cmd_handler.h>
// Leave this now for testing then we need to remove this and the related #ifdef

/* Minimum supported Controller's Node ID. It should never be less than 1 */
#define MIN_CONTROLLER_NODE_ID 1

// The delay used for sending a delayed assign new ID on LR channel.
#define TRANSMIT_OPTION_DELAY_ASSIGN_ID_LR_MS           100

enum {
  ADD_ANY = 1,
  ADD_CONTROLLER ,
  ADD_SLAVE,
  ADD_EXISTING,
  ADD_STOP,
  ADD_STOP_FAILED,
  ADD_RESERVED,
  ADD_HOME_ID,
  ADD_SMART_START,
  ADD_NODE_MAX
} eAddNode;

enum {
  REMOVE_ID_ANY = 0,
  REMOVE_ANY = REMOVE_NODE_ANY,
  REMOVE_CONTROLLER,
  REMOVE_SLAVE,
  REMOVE_STOP = REMOVE_NODE_STOP,
  REMOVE_NODE_MAX
} eRemoveNode;

SSyncEventArg1 g_ControllerStaticControllerNodeIdChanged = { .uFunctor.pFunction = 0 };  /* Callback activated on change to StaticControllerNodeId */
SSyncEventArg2 g_ControllerHomeIdChanged = { .uFunctor.pFunction = 0 };                  /* Callback activated on change to Home ID / Node ID */
                                                                                         /* Arg1 is Home ID, Arg2 is Node ID*/
/****************************************************************************/
/*                      PRIVATE TYPES and DEFINITIONS                       */
/****************************************************************************/

#ifdef ZW_CONTROLLER_STATIC
#define SUC_ENABLED_MASK    0x80      /* SUC enabled bit in sucEnable byte */
#define SUC_ENABLED_DEFAULT (SUC_ENABLED_MASK | ZW_SUC_FUNC_NODEID_SERVER)
#endif

/* Defines used to determine rediscovery speed */
#define ZW_NEIGHBOR_DISCOVERY_SPEED_START      ZWAVE_FIND_NODES_SPEED_9600
#define ZW_NEIGHBOR_DISCOVERY_SPEED_BEAM       (ZWAVE_FIND_NODES_SPEED_100K + 1)
#define ZW_NEIGHBOR_DISCOVERY_SPEED_END        (ZW_NEIGHBOR_DISCOVERY_SPEED_BEAM + 1)

#if defined(ZW_CONTROLLER_STATIC)
/* Enum defines for SendRouteUpdate return values */
typedef enum ESendRouteUpdateStatus
{
  ESENDROUTE_ASSIGN_RETURN_ROUTE_NOT_STARTED,
  ESENDROUTE_ASSIGN_RETURN_ROUTE_INITIATED,
  ESENDROUTE_ASSIGN_RETURN_ROUTE_ENDED
} ESendRouteUpdateStatus;
#endif

/****************************************************************************/
/*                              PRIVATE DATA                                */
/****************************************************************************/

/*
 * Will be set when the controller enters "add node" mode using ZW_AddNodeToNetwork().
 */
static bool addUsingLR;

/* Buffer used for sending and receiving node information */
typedef struct tNewNodeInfo {
  NODEINFO_FRAME nodeInfoFrame;
  uint8_t mNodeID;
  uint8_t cmdLength;
} tNewNodeInfo;

static tNewNodeInfo  newNodeInfoBuffer = { 0 };

/* NodeInfo cache and variable for keeping track which nodeID the cached nodeInfo is for */
static node_id_t currentCachedNodeInfoNodeID = 0;
static EX_NVM_NODEINFO tNodeInfo = {0};

static struct _node_range_
{
  RANGEINFO_FRAME  frame;
  uint8_t          lastController;
  TxOptions_t      txOptions;
} nodeRange;

uint8_t  zensorWakeupTime;    /* The wakeuptime to be used if nodes are zensor nodes */
                              /* If this byte is missing or is ZERO then the rangeinfo */
                              /* is for a standard findneighbors sequence - If zensor- */
                              /* WakupTime is ZWAVE_SENSOR_WAKEUPTIME_1000MS then the */
                              /* zensors has a wakeuptime at 1000ms, if zensorWakeupTime */
                              /* is ZWAVE_SENSOR_WAKEUPTIME_250MS then the wakeup time */
                              /* for the zensors is 250ms */

typedef union _CMD_BUFF_
{
  COMMAND_COMPLETE_FRAME  cmdCompleteBuf;
#ifdef ZW_CONTROLLER_STATIC
  TRANSFER_END_FRAME      transferEndFrame;
#endif
  NOP_POWER_FRAME         nopPowerFrame;
} CMD_BUFF;

static CMD_BUFF cmdBuffer;

#ifdef ZW_CONTROLLER_STATIC
static bool cmdBufferLock;
#endif

bool g_findInProgress;

static void          /* RET Nothing */
FindNeighbors(
  uint8_t nextNode);      /* IN  Next node to transmit to */

static void ListeningNodeDiscoveryDone (void);

#if defined(ZW_CONTROLLER_STATIC)
static void
ZCB_SendRouteUpdate(
  ZW_Void_Function_t Context,
  uint8_t txStatus,
  TX_STATUS_TYPE *txStatusReport);

static ESendRouteUpdateStatus
SendRouteUpdate(
  uint8_t txStatus);
#endif

#ifdef ZW_CONTROLLER_STATIC
#ifdef ZW_SELF_HEAL
/* Used to store lost nodes Neighbors so that in order to evaluate if network changed from the lost */
uint8_t tempNodeLostMask[MAX_NODEMASK_LENGTH];
bool lostOngoing;
#endif  /* ZW_SELF_HEAL */

#endif  /* ZW_CONTROLLER_STATIC */

#ifdef ZW_NETWORK_UPDATE_REPLICATE
uint8_t updateReplicationsNodeID;
uint8_t bSourceOfNewNodeRegistred;
#endif

/* Buffer used to assign id to other nodes */
FRAME_BUFFER assignIdBuf = { 0 };

NEW_NODE_REGISTERED_FRAME sNewRegisteredNodeFrame = { 0 };
uint8_t bNewRegisteredNodeFrameSize = 0;

#ifdef ZW_ROUTED_DISCOVERY
/* Do we ask for a neighborhood discovery for a specific Node even */
/* if it has been done allready? - Update neighborhood */
bool updateNodeNeighbors;
#endif


#define PENDING_TIMER_RELOAD         450  /* 15 minutes in 2 seconds unit*/
#define PENDING_TIMER_MAX            96   /* 24 hours in 15 minutes  unit*/
#define PENDING_TIMER_TIMEOUT        2000 /* 2000 ms*/
#define SUC_REQUEST_UPDATE_TIMEOUT   20

#define REQUEST_ID_TIMEOUT           2000  /* 2 sec timeout on requesting an ID */

typedef struct _SUC_UPDATE_
{
  uint8_t updateState;
  SSwTimer TimeoutTimer;
} SUC_UPDATE;

bool pendingUpdateOn;
bool pendingTableEmpty;
/* TO#1547 fix - Allow only one pending list flush before going to sleep */
bool pendingUpdateNotCompleted;
#ifdef ZW_CONTROLLER_STATIC
uint8_t sucEnabled;                       /* Default we want to be SIS */
#endif
bool sucAssignRoute;
bool sendSUCRouteNow;       /* For now we need to request that SUC routes are sent */
bool spoof;                 /* We do not spoof when sending */
bool resetCtrl = false;

#ifdef REPLACE_FAILED
bool failedNodeReplace;
#endif

static uint16_t pendingTimerReload;
static uint8_t pendingTimerCount;

static node_id_t pendingNodeID;
static SSwTimer TimerHandler = { 0 };

uint8_t staticControllerNodeID; // Only set through ControllerSetStaticControllerId()
static SSwTimer UpdateTimeoutTimer = { 0 };


// FIXME Only used once
// assign_ID timer be used instead? its stopped just before learn info timer is started
static SSwTimer LearnInfoTimer = { 0 };

// FIXME Only used once, and only to delay call to ZCB_FindNeighborCompleteFailCall
static SSwTimer FindNeighborCompleteDelayTimer = { 0 };

// FIXME Only used once, and only to delay call to ZCB_CallAssignTxCompleteFailed
static SSwTimer AssignTxCompleteDelayTimer = { 0 };

// FIXME only used twice
static SSwTimer LostRediscoveryTimer = { 0 };

/*Use to delay the application notification of new info for 500 ms*/
static SSwTimer newNodeAssignedUpdateDelay = { 0 };

static STransmitCallback SUCCompletedFunc = { 0 };

static  SSwTimer assignSlaveTimer = { 0 };

static struct sTxQueueEmptyEvent sTxQueueEmptyEvent_phyChange = { 0 };

static uint8_t GetSUCNodeID(void);

static bool ZW_StoreNodeInfoFrame(node_id_t wNodeID,  uint8_t* pNodeInfoFrame);

static void SetStaticControllerNodeId(uint8_t NewStaticControllerNodeId);

#ifdef ZW_CONTROLLER_TEST_LIB
static void HomeIdUpdate(uint8_t aHomeId[HOMEID_LENGTH], node_id_t NodeId);
#else
static void HomeIdUpdate(uint8_t aHomeId[HOMEID_LENGTH], uint8_t NodeId);
#endif

static void UpdateNodeInformation(NODEINFO_FRAME *pNodeInfo);


static const SVirtualSlaveNodeInfoTable_t * m_pVirtualSlaveInfoTable;

static const SAppNodeInfo_t * m_pAppNodeInfo;
const SAppNodeInfo_t * getAppNodeInfo(void) { return m_pAppNodeInfo; }

#ifdef ZW_CONTROLLER_STATIC
static uint8_t SUCLastIndex;
static bool cmdCompleteSeen;
#endif
static bool didWrite;   /* Maybe this could be moved into ZW_CONTROLLER_STATIC only section */
SUC_UPDATE SUC_Update = { .TimeoutTimer.pLiaison = NULL };  // Must be zero intialized
static node_id_t scratchID;
#ifdef ZW_CONTROLLER_STATIC
static uint8_t destNodeIDList[ZW_MAX_CACHED_RETURN_ROUTE_DESTINATIONS];
#endif
static bool findNeighborsWithFlirs = true;
static bool defaultFLNotLookingForFL = false;
static void resetDefaultFLNotLookingForFL(void) { defaultFLNotLookingForFL = false; }
static void resetFindNeighborsWithFlirs(void) { findNeighborsWithFlirs = true; }

#define FIND_NEIGHBOR_NODETYPE_IDLE      0
#define FIND_NEIGHBOR_NODETYPE_STARTED   1
#define FIND_NEIGHBOR_NODETYPE_DONE      2
static uint8_t findNeighborsWithNodetypeState = FIND_NEIGHBOR_NODETYPE_IDLE;
static node_id_t    m_listning_nodeid = 0;       // the node id of the listening node we want the flirs nodes to ping
static uint8_t listning_node_neighbors[MAX_NODEMASK_LENGTH];
static STransmitCallback NodeTypeNeighborCallback = { 0 };
static node_id_t current_flirs_nodeid = 2;

static uint8_t count;  /* common counter */

uint8_t bUpdateIndex;   /* global SUC controller update index used when updateing controllers as SUC/SIS */

/* Used by ZW_RemoveNodeIDFromNetwork */
node_id_t bNetworkRemoveNodeID;

/* Used by LearnInfoCallBack, ZW_AddNodeToNetwork and ZW_RemoveNodeFromNetwork/ZW_RemoveNodeIDFromNetwork */
LEARN_INFO_T learnNodeInfo = { 0 };

uint8_t addSmartNodeHomeId[2 * HOMEID_LENGTH];

static NODE_MASK_TYPE classicFailedNodesMask;
static LR_NODE_MASK_TYPE lrFailedNodesMask;

ASSIGN_ID assign_ID = { 0 };

 STATIC_VOID_CALLBACKFUNC(removeFailedNodeCallBack) (uint8_t);

void
UpdateFailedNodesList(
  node_id_t bNodeID,
  uint8_t TxStatus);

#define REQUEST_NODE_INFO_TIMEOUT          60

#define SUC_ADD           1
#define SUC_DELETE        2
#define SUC_UPDATE_RANGE  3
#define SUC_UNKNOWN_CONTROLLER   0xFE
#define SUC_OUT_OF_DATE          0xFF

#define SUC_IDLE                                0               /* idle nothing happening */
#define REQUEST_SUC_UPDATE_WAIT_START_ACK       2               /* wait for ack of receiving SUC update start frame */
#define SUC_UPDATE_STATE_WAIT_NODE_INFO         3               /* wait for receving node info frame */
#define SUC_SET_NODE_WAIT_ACK                   5               /* wait for ack of Set suc frame. */
#define SUC_SET_NODE_WAIT_ACK_FRAME             6               /* wait for set SUC ack frame. */

#ifdef ZW_CONTROLLER_STATIC
#define SEND_SUC_UPDATE_NODE_INFO_SEND          7               /* node info should be send from the SUC */
#define SEND_SUC_UPDATE_NODE_RANGE_SEND         8               /* range info should be send from the SUC */
#define SEND_SUC_UPDATE_NODE_INFO_WAIT          9               /* node info was sent from the  SUC */
#define SEND_SUC_UPDATE_NODE_RANGE_WAIT         10              /* range info was sent from the SUC */
#endif
#define SUC_UPDATE_ROUTING_SLAVE                11


#define SUC_DELETE_NODE                         14
#define SUC_SEND_ID                             15              /* sending new SUC ID to controller wanting update */

#define SUC_ENABLE    0x01

#define NODE_INFO     0x01
#define RANGE_INFO    0x02

#define CLEAR_ALL     false
#define CLEAR_TABLES  true

uint8_t oldSpeed;

uint8_t bNeighborDiscoverySpeed;

/* NodeID of neighbor node currently being NOP'ed */
uint8_t bCurrentRangeCheckNodeID;

/* When adding we try sending upto 6*3 NOP frames to the new nodeID, to establish if it got included */
#define ASSIGN_NOP_ADD_MAX_TRIES      6
/* When removing we try sending upto 1*3 NOP frames to the old nodeID, to establish that it got removed */
#define ASSIGN_NOP_REMOVE_MAX_TRIES   1
#define EXCLUDE_REQUEST_CONFIRM_NOP_MAX_TRIES   ASSIGN_NOP_REMOVE_MAX_TRIES

uint8_t nOPTries;

bool forceUse40K;

bool bNeighborUpdateFailed;

#ifdef ZW_CONTROLLER_BRIDGE
uint8_t nodePool[MAX_NODEMASK_LENGTH]; /* Cached nodePool */
node_id_t virtualNodeID;     /* Current virtual nodeID */

uint8_t learnSlaveMode;          /* true when the "Assign ID's" command is accepted */

/* Internal state used by AssignID sequence. */
#define ASSIGN_INFO_PENDING       0x03  /*Node is waiting for request of range information*/

uint8_t assignSlaveState;
uint8_t newID;

  STATIC_VOID_CALLBACKFUNC(learnSlaveNodeFunc)(uint8_t bStatus, node_id_t orgID, node_id_t newID);  /* Node learn call back function. */

static void
ZCB_AssignSlaveComplete(
  ZW_Void_Function_t Context, uint8_t txStatus, TX_STATUS_TYPE *txStatusReport);

#endif  /* ZW_CONTROLLER_BRIDGE */

/* Learn Node Complete call back */
static VOID_CALLBACKFUNC(zcbp_learnCompleteFunction)(LEARN_INFO_T *);

static void
ClearPendingUpdate( /* RET  Nothing */
  node_id_t bNodeID);    /* IN   Node ID that should be set */

node_id_t                  /*RET Next free node id, or 0 if no free */
GetMaxNodeID( void ); /* IN Nothing  */

node_id_t                  /*RET Next free node id for LR, or 0 if no free */
GetMaxNodeID_LR( void ); /* IN Nothing  */

static void       /*RET Nothing */
UpdateMaxNodeID(
    node_id_t wNodeID,   /* IN Node ID on node causing the update */
  uint8_t  deleted);  /* IN true if node has been removed from network, false if added */

static node_id_t
GetNextFreeNode(  /*RET Next free node id, or 0 if no free */
  bool isLongRangeChannel);          /* IN Nothing  */

void
SetupNodeInformation(
  uint8_t cmdClass);

/* Variables used in GenerateNodeInformation */
static const uint8_t *plnodeParm;
static uint8_t  blparmLength;
static uint8_t blnodeInfoLength;

#ifdef ZW_CONTROLLER_BRIDGE
/* Variables used in GenerateVirtualSlaveNodeInformation */
static uint8_t blslavedeviceOptionsMask, blslaveparmLength, blslavenodeInfoLength;
#endif

uint8_t
ZCB_GetNodeType(      /*RET Generic Type of node, or 0 if not registered */
  node_id_t bNodeID);  /* IN Node id */

uint8_t
GetNodeSpecificType( /*RET Specific Type of node, or 0 if not registered */
  node_id_t nodeID);     /* IN Node id */

uint8_t              /*RET Node security */
GetNodesSecurity(
  node_id_t bNodeID);   /* IN Node id */

bool                 /*RET Is valid controller */
isValidController(
  node_id_t controllerID); /* IN Controller node ID */

/* TO#01763 Fix - Controllers can loose their ability to route using ZW_SendData() */
void
ResetRouting(void);

uint8_t
MaxCommonSpeedSupported(
  uint8_t bDestNodeID,   /* Destination node ID */
  uint8_t bSourceNodeID  /* Source node ID */
);


#ifdef ZW_ROUTED_DISCOVERY
static  STransmitCallback mTxCallback = { 0 };

static void ApiCallCompleted(LEARN_INFO_T * pLearnInfo)
{
  ZW_TransmitCallbackInvoke(&mTxCallback, pLearnInfo->bStatus, NULL);
  if (REQUEST_NEIGHBOR_UPDATE_STARTED != pLearnInfo->bStatus)
  {
    ZW_TransmitCallbackUnBind(&mTxCallback);
  }
}
#endif

/*============================   GetNodeBasicType   ======================
**    Function description
**      Gets the BASIC_TYPE of a node
**    Side effects:
**
**--------------------------------------------------------------------------*/
static uint8_t
GetNodeBasicType(
  node_id_t nodeID);

uint8_t                  /*RET Basic Device Type */
ZW_GetNodeTypeBasic(
  NODEINFO *nodeInfo);/* IN pointer NODEINFO struct to use in Basic Type decision */

void
AssignNewID(bool isLongRangeChannel );  /* RET  Nothing */

void
RequestRangeInfo( void ); /* RET  Nothing */

void
ZCB_ExcludeRequestConfirmTxComplete(
  ZW_Void_Function_t Context, uint8_t txStatus, TX_STATUS_TYPE *txStatusReport);

// Note that this is called directly fom Explore.c(thus not static)
void
ZCB_AssignTxComplete(
  ZW_Void_Function_t Context, uint8_t txStatus, TX_STATUS_TYPE *txStatusReport);

uint8_t
RequestNodeRange(   /*RET - false  No range info needed
                          - true   Neighbor discovery started */
  uint8_t bFirstTime       /* IN Is this first request */
);


static void         /*RET Nothing */
LearnInfoCallBack(
  uint8_t       bStatus,    /* IN Status of learn mode */
  node_id_t  bSource,    /* IN Node id of the node that send node info */
  uint8_t       bLen);      /* IN Node info length                        */


/* Assign static return route */

/* Defines for returnRouteState if more states is to be defined then */
/* retrunRouteState MUST be define as a uint8_t instead of a bool */
#define RETURN_ROUTE_STATE_ASSIGN   false
#define RETURN_ROUTE_STATE_DELETE   true

static STransmitCallback cmdCompleteReturnRouteFunc;

/* Buffer used to assign static return route */
static ASSIGN_RETURN_ROUTE_FRAME assignReturnRouteBuf = { 0 };
uint8_t returnRouteNumber;
uint8_t returnRouteNodeID; /* NodeID of routing slave receiving return routes */
uint8_t returnDestNodeID;  /* Return route destination node id */
bool returnRouteAssignActive; /* If false then no Return Route Assign/Delete is active */
bool returnRouteState;  /* For now we have two states therefor a bool is enough ;-) */
bool returnRouteNoDirect;
uint8_t returnRoutePriorityPresent;
#define PRIORITY_ROUTE_NOT_PRESENT    0
#define PRIORITY_ROUTE_WAITING        1
#define PRIORITY_ROUTE_SEND           2
#define PRIORITY_ROUTE_RESET_SEARCH   4
uint8_t returnRoutePriorityBuffer[MAX_REPEATERS+1];
#define returnRoutePriorityNumber returnRoutePriorityBuffer[0] /* Reuse buffer as number */
uint8_t bReturnRouteTriedSpeeds; /* Speeds tried during current return route assignment */

static void
SUCSendCmdComp(
  uint8_t seq);

static void
SendReturnRoute( void );

static void
ResetSUC(void);

void
ZW_SetLearnNodeStateNoServer(
  uint8_t mode);

void
RequestReservedNodeID(void);

static void
ZCB_RequestUpdateCallback(ZW_Void_Function_t Context, uint8_t txStatus, TX_STATUS_TYPE *txStatusReport);

static void
controllerVarInit(void);

static void
PendingNodeUpdateEnd(void);

typedef struct
{
  bool is_listening;
  bool supports_flirs_1000ms;
  bool supports_flirs_250ms;
  bool beam_capability;
  bool is_controller;
  bool supports_speed_100klr;
  uint8_t basic_type;
  uint8_t generic_device_class;
  uint8_t specific_device_class;
  uint8_t cc_list_length;
  uint8_t cc_list[NODEPARM_MAX];
  bool is_long_range;
}
zw_node_t;

/*
 * Setters
 */
void node_listening_set(zw_node_t * const p_node, const bool is_listening)
{
  p_node->is_listening = is_listening;
}

void node_supports_flirs_1000ms_set(zw_node_t * const p_node, const bool supports_flirs_1000ms)
{
  p_node->supports_flirs_1000ms = supports_flirs_1000ms;
}

void node_supports_flirs_250ms_set(zw_node_t * const p_node, const bool supports_flirs_250ms)
{
  p_node->supports_flirs_250ms = supports_flirs_250ms;
}

void node_beam_capability_set(zw_node_t * const p_node, const bool beam_capability)
{
  p_node->beam_capability = beam_capability;
}

void node_controller_set(zw_node_t * const p_node, const bool is_controller)
{
  p_node->is_controller = is_controller;
}

void node_speed_100klr_set(zw_node_t * const p_node, const bool supports_speed_100klr)
{
  p_node->supports_speed_100klr = supports_speed_100klr;
}

void node_basic_type_set(zw_node_t * const p_node, const uint8_t basic_type)
{
  p_node->basic_type = basic_type;
}

void node_generic_device_class_set(zw_node_t * const p_node, const uint8_t generic_device_class)
{
  p_node->generic_device_class = generic_device_class;
}

void node_specific_device_class_set(zw_node_t * const p_node, const uint8_t specific_device_class)
{
  p_node->specific_device_class = specific_device_class;
}

void node_cc_list_set(zw_node_t * const p_node, const uint8_t * const p_cc_list, const uint8_t cc_list_length)
{
  p_node->cc_list_length = cc_list_length;
  if (NODEPARM_MAX < p_node->cc_list_length)
  {
    p_node->cc_list_length = NODEPARM_MAX;
  }
  memcpy((uint8_t *)&p_node->cc_list[0], p_cc_list, p_node->cc_list_length);
}

void node_long_range_set(zw_node_t * const p_node, const bool is_long_range)
{
  p_node->is_long_range = is_long_range;
}

/*
 * Getters
 */
bool node_is_listening(const zw_node_t * const p_node)
{
  return p_node->is_listening;
}

bool node_supports_flirs_1000ms(const zw_node_t * const p_node)
{
  return p_node->supports_flirs_1000ms;
}

bool node_supports_flirs_250ms(const zw_node_t * const p_node)
{
  return p_node->supports_flirs_250ms;
}

bool node_supports_beam_capability(const zw_node_t * const p_node)
{
  return p_node->beam_capability;
}

bool node_is_controller(const zw_node_t * const p_node)
{
  return p_node->is_controller;
}

bool node_supports_speed_100klr(const zw_node_t * const p_node)
{
  return p_node->supports_speed_100klr;
}

uint8_t node_basic_type_get(const zw_node_t * const p_node)
{
  return p_node->basic_type;
}

uint8_t node_generic_device_class_get(const zw_node_t * const p_node)
{
  return p_node->generic_device_class;
}

uint8_t node_specific_device_class_get(const zw_node_t * const p_node)
{
  return p_node->specific_device_class;
}

uint8_t node_cc_list_get(const zw_node_t * const p_node, uint8_t * p_destination)
{
  memcpy(p_destination, &p_node->cc_list[0], p_node->cc_list_length);
  return p_node->cc_list_length;
}

bool node_is_long_range(const zw_node_t * const p_node)
{
  return p_node->is_long_range;
}

/*============================   AreSUCAndAssignIdle   ======================
**    Function description
**      Test if SUC is ready to handle requests from nodes.
**      returns true if so, false if not.
**    Side effects:
**
**--------------------------------------------------------------------------*/
uint8_t
AreSUCAndAssignIdle( void )
{
  DPRINT("AreSUCAndAssignIdle\r\n");
  DPRINTF("SUC_Update.updateState = %02x\r\n",SUC_Update.updateState);
  DPRINTF("SUC_Update = %02x\r\n",SUC_Update.updateState);
  DPRINTF("GET_NEW_CTRL_STATE = %02x\r\n",GET_NEW_CTRL_STATE);

  return ((SUC_Update.updateState == SUC_IDLE) &&
          (assign_ID.assignIdState == ASSIGN_IDLE) &&
          (
           ((LEARN_NODE_STATE_SMART_START == g_learnNodeState) &&
            (GET_NEW_CTRL_STATE == NEW_CTRL_SEND)) ||
           (GET_NEW_CTRL_STATE == NEW_CTRL_STOP)));
}


/*============================   CreateRequestNodeList   ======================
**    Function description
**      Runs through the nodeinfo table and creates a list of the listning nodes
**      in the network
**    Side effects:
**
**--------------------------------------------------------------------------*/
uint8_t
CreateRequestNodeList(  /*RET Returns false if no nodes found else it returns the number of valid bytes in the nodeMask */
  uint8_t *nodeMask,       /*OUT Nodemask which indicates which nodes to ask */
  uint8_t bSkipNodeID      /*IN  NodeID which indicates a node id that should be omitted */
);                      /*    from the nodeMask. Typically it is the nodeID the nodeMask is transmitted to */


/*=========================   FindNodesTimeout   ============================
**
**    Timeout function for find nodes within range
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
static void
ZCB_FindNodesTimeout(SSwTimer* pTimer); /* RET  Nothing */


/* Application level callback for learn mode. Called when
 * secure inclusion state machine is done. */
VOID_CALLBACKFUNC(secureLearnAppCbFunc)(LEARN_INFO_T* psLearnInfo);
VOID_CALLBACKFUNC(secureAddAppCbFunc)(LEARN_INFO_T* psLearnInfo);


void
CtrlStorageCacheNodeInfoUpdate(node_id_t nodeID, bool speed100kb);


/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/

bool g_learnMode;/* true when the "Assign ID" command is to be accepted */
bool g_learnModeClassic;  /* true when the classic "Assign ID" is to be accepted */
bool g_learnModeDsk;

uint8_t g_learnNodeState;   /* not false when the "Node Info" frame is accepted */

uint8_t g_Dsk[16];

bool primaryController; /* true - Master controller, false - Secondary controller */
bool controllerOnOther; /* true - controller on "another" network than the "original" */

bool realPrimaryController;
bool addSlaveNodes;
bool addCtrlNodes;
bool addSmartStartNode;

uint8_t  bMaxNodeID;       /* Contains the max value of node ID used*/
static node_id_t g_wMaxNodeID_LR = 0;   /* Contains the max value of node ID used for LR*/

bool nodeIDAssigned;
extern bool controllerIncluded;
/* The HomeID on the controller whom we are about to include in the network */
extern uint8_t inclusionHomeID[HOMEID_LENGTH];
extern bool inclusionHomeIDActive;
extern uint8_t crH[HOMEID_LENGTH];

extern bool bReplicationDontDeleteOnfailure;

#ifdef ZW_CONTROLLER_STATIC
static void
DeleteNodeTimeout(void);
#endif


/*--------------------------------------------------------------------
Autmatic update functions start
--------------------------------------------------------------------*/

/*=========================   UpdateSUCstate  ================================
**    Function description
**      Reports to Application about SUC nodeID.
**    Side effects:
**
**--------------------------------------------------------------------------*/
static void
ZCB_UpdateSUCstate(
  __attribute__((unused)) ZW_Void_Function_t Context,
  __attribute__((unused)) uint8_t bStatus,
  __attribute__((unused)) TX_STATUS_TYPE *txStatusReport)
{
  ProtocolInterfacePassToAppNodeUpdate(UPDATE_STATE_SUC_ID, staticControllerNodeID, NULL, 0);
}


/*============================   ResetSUC  ===================================
**    Function description
**      reset the variables used by the SUC scheme
**    Side effects:
**
**--------------------------------------------------------------------------*/
static void
ResetSUC(void)
{
  TimerStop(&SUC_Update.TimeoutTimer);
  SUC_Update.updateState = SUC_IDLE;
  assign_ID.assignIdState = ASSIGN_IDLE;
  scratchID = 0;
#ifdef ZW_NETWORK_UPDATE_REPLICATE
  updateReplicationsNodeID = 0;
#endif
  /* TODO - Why is it only when doing Lost we reset the below variables??? */
#ifdef ZW_SELF_HEAL
  /* TO#2772 partial fix - Make sure we reset all variables used in the lost functionality */
#if defined(ZW_CONTROLLER_STATIC)
  if (lostOngoing)
  {
    updateNodeNeighbors = false;
    lostOngoing = false;
    assign_ID.newNodeID = 0;
    zcbp_learnCompleteFunction = NULL;
    g_learnNodeState = LEARN_NODE_STATE_OFF;
  }
#endif
#endif
  /* TO#4118 fix - Make sure we can use AssignReturnRoute after RESET */
  returnRouteAssignActive = false;
  sucAssignRoute = false;
}


/*============================   NetworkTimerStart ===========================
**    Function description
**      Starts the Network update timeout function
**    Side effects:
**
**--------------------------------------------------------------------------*/
static void
NetWorkTimerStart(                        /* RET  nothing       */
  SSwTimer* pTimer,                         /* IN   Pointer to timer */
  VOID_CALLBACKFUNC(TimeoutCallback)(SSwTimer* pTimer),  /* IN   Timeout callback */
  uint32_t Timeout                        /* IN   timeout time in ms */
  )
{
  TimerSetCallback(pTimer, TimeoutCallback);
  TimerStart(pTimer, Timeout);
}

#if defined(ZW_CONTROLLER_STATIC)
/*============================   TransferEndDone   ===========================
**    Function description
**      Callback used when TransferEnd has been send.
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
static void
ZCB_TransferEndDone(
  __attribute__((unused)) ZW_Void_Function_t Context,
  __attribute__((unused)) uint8_t txStatus,
  __attribute__((unused)) TX_STATUS_TYPE *txStatusReport)
{
  cmdBufferLock = false;
  /* TODO - Can we do this better ??? */
  if (cmdBuffer.transferEndFrame.status != ZWAVE_TRANSFER_UPDATE_WAIT)
  {
    ResetSUC();
  }
}


/*============================   DeleteNodeCallback   ======================
**    Function description
**      Callback used when deleting node.
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
static void
ZCB_DeleteNodeCallback(
  __attribute__((unused)) ZW_Void_Function_t Context,
  __attribute__((unused)) uint8_t txStatus,
  __attribute__((unused)) TX_STATUS_TYPE *txStatusReport)
{
  ResetSUC();
  ROUTING_ANALYSIS_STOP();
  DeleteNodeTimeout();
  cmdBufferLock = false;
}


/*============================   SendSUCTransferEnd   ========================
**    Function description
**      Send transfer end to the controller when there more updates to send.
**      state indicates the reason for ending
**      Following values for state are valid:
**            ZWAVE_TRANSFER_UPDATE_DONE      - Transfer completed succesfully
**            ZWAVE_TRANSFER_UPDATE_ABORT     - Abort rest of transfer.
**            ZWAVE_TRANSFER_UPDATE_WAIT      - Tell the other to wait.
**            ZWAVE_TRANSFER_UPDATE_DISABLED  - Tell other SUC is not enabled
**            ZWAVE_TRANSFER_UPDATE_OVERFLOW  - Some Kind of overflow. Manual
**                                              replication needed.
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
static void                       /*RET nothing*/
SendSUCTransferEnd(node_id_t bNodeID,  /* IN nodeID to send to*/
                   uint8_t bState)  /* IN Status of termination*/
{
  if (cmdBufferLock)
  {
    if (SUC_Update.updateState == SUC_DELETE_NODE)
    {
      ZCB_DeleteNodeCallback(0, 0, NULL);
      cmdBufferLock = true;
    }
    else
    {
      if (bState != ZWAVE_TRANSFER_UPDATE_WAIT)
      {
        ResetSUC();
      }
    }
    return;
  }
  cmdBufferLock = true;

  cmdBuffer.transferEndFrame.cmdClass = ZWAVE_CMD_CLASS_PROTOCOL;
  cmdBuffer.transferEndFrame.cmd = ZWAVE_CMD_TRANSFER_END;
  cmdBuffer.transferEndFrame.status = bState;
  if (SUC_Update.updateState == SUC_DELETE_NODE)
  {
    static const STransmitCallback TxCallback = { .pCallback = ZCB_DeleteNodeCallback, .Context = 0 };
    if (!EnQueueSingleData(false, g_nodeID, bNodeID, (uint8_t *)&cmdBuffer, sizeof(TRANSFER_END_FRAME),
                              (TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE),
                              0, // 0ms for tx-delay (any value)
                              ZPAL_RADIO_TX_POWER_DEFAULT,
                              &TxCallback))
    {
      ZCB_DeleteNodeCallback(0, 0, NULL);
    }
  }
  else
  {
    static const STransmitCallback TxCallback = { .pCallback = ZCB_TransferEndDone, .Context = 0 };
    if (!EnQueueSingleData(false, g_nodeID, bNodeID, (uint8_t *)&cmdBuffer, sizeof(TRANSFER_END_FRAME),
                              (TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE),
                              0, // 0ms for tx-delay (any value)
                              ZPAL_RADIO_TX_POWER_DEFAULT,
                              &TxCallback))
    {
      ZCB_TransferEndDone(0, 0, NULL);
    }
  }
}


/*==============================   SUCIDSend   ==============================
**    Function description
**        SUC ID transmission done
**    Side effects:
**      Enqueues transmission of Transfer End frame
**
**--------------------------------------------------------------------------*/
static void
ZCB_SUCIDSend(
  __attribute__((unused)) ZW_Void_Function_t Context,
  __attribute__((unused)) uint8_t txStatus,
  __attribute__((unused)) TX_STATUS_TYPE *txStatusReport)
{
  /* We do not care if the SUC ID was send successfully or not - just get on with it! */
  SendSUCTransferEnd(scratchID, ZWAVE_TRANSFER_UPDATE_DONE);
}


/*----------------------------- SUCUpdateNodeInfo -------------------------------------
**    Function description
**        write the SUC update data to the eeprom when they are received from the
**        primary controller
**    Side effects
--------------------------------------------------------------------------------------*/
static void
SUCUpdateNodeInfo(
  node_id_t bNodeID,
  uint8_t changeType,
  uint8_t infoLen)
{
  if (ZW_IS_NOT_SUC)
  {
    if ((newNodeInfoBuffer.nodeInfoFrame.security & ZWAVE_NODEINFO_CONTROLLER_NODE))
    {
      uint8_t SucStatus = (changeType == SUC_ADD) ? SUC_OUT_OF_DATE : 0;
      CtrlStorageSetCtrlSucUpdateIndex(bNodeID, SucStatus);
    }
    return;
  }
  /* Circular buffer. wraparound */
  if (didWrite)
  {
    /* AND with 0x3f (63) and add one -> 64 -> 1, 1 -> 2 and 63 -> 64 and soforth */
    SUCLastIndex &= (SUC_MAX_UPDATES - 1);
    SUCLastIndex++;
    /* Write change */
    CtrlStorageSetSucLastIndex(SUCLastIndex);
    CtrlStorageSetSucUpdateEntry(SUCLastIndex - 1,changeType,bNodeID );
  }
  if (changeType == SUC_UPDATE_RANGE)
  {
    /* When the network change is distributed we get the nodeInfo from the nodeinfo table. */
    /* As a rangeinfo update MUST NOT move the SUC update pointer for the possible involved */
    /* controller - we are therefor done now */
  }
  else
  {
    /* if a controller node was added then changes up to here are replicated automaticaly */
    if ((newNodeInfoBuffer.nodeInfoFrame.security & ZWAVE_NODEINFO_CONTROLLER_NODE))
    {
      CtrlStorageSetCtrlSucUpdateIndex(bNodeID, SUCLastIndex);
    }

    if (didWrite)
    {
      if (infoLen > SUC_UPDATE_NODEPARM_MAX)
      {
        infoLen = SUC_UPDATE_NODEPARM_MAX;
      }
      // Clear unused bytes - If its a SUC_DELETE then infoLen = 0
      memset(&newNodeInfoBuffer.nodeInfoFrame.nodeInfo[infoLen], 0, (SUC_UPDATE_NODEPARM_MAX - infoLen));
      // Save the nodeinfo...

      CtrlStorageSetCmdClassInSucUpdateEntry(SUCLastIndex - 1, (uint8_t*)&newNodeInfoBuffer.nodeInfoFrame.nodeInfo[0]);
    }
  }
  if (didWrite)
  {
    // open file for write
    for (uint32_t i = 1; i <= bMaxNodeID; i++)
    {
      /* Check if any controller changes are outdated */
      /* SUCLastIndex catched up with this controller. It is now outdated. Unless it was this controller */
      if ((CtrlStorageGetCtrlSucUpdateIndex(i) == SUCLastIndex) && (bNodeID != i))
      {
        CtrlStorageSetCtrlSucUpdateIndex(i, SUC_OUT_OF_DATE);
      }
    }
  }
  SUC_Update.updateState = SUC_IDLE;
}


/*===================   InvalidateSUCRoutingSlaveList   ======================
**    Function description
**        Mark all Routing Slave nodes as needing network updates
**    Side effects:
**
**--------------------------------------------------------------------------*/
static void
InvalidateSUCRoutingSlaveList(void)
{
  bool writeToNVM = false;
  for (uint8_t i = 1; i <= bMaxNodeID; i++)
  {
    if (CtrlStorageSlaveNodeGet(i))
    {
      if (CtrlStorageSetRoutingSlaveSucUpdateFlag( i, false, false))
      {
        writeToNVM = true;
      }
    }
  }
  if(writeToNVM)
  {
    CtrlStorageSetRoutingSlaveSucUpdateFlag(WRITE_ALL_NODES_TO_NVM, false, true);
  }
}


/*==========================   SUCUpdateRangeInfo   ==========================
**    Function description
**        Register Range Info change if needed
**    Side effects:
**
**--------------------------------------------------------------------------*/
static bool
SUCUpdateRangeInfo(
  uint8_t bNodeID)
{
  if (bNodeID && ZW_IS_SUC)
  {
    /* New range info added mark all routing slaves as not updated */
    InvalidateSUCRoutingSlaveList();
    /* If we are SUC and the previous network change was for another node or the change was not a SUC_ADD */
    SUC_UPDATE_ENTRY_STRUCT SucUpdateEntry = { 0 };
    CtrlStorageGetSucUpdateEntry(SUCLastIndex - 1, &SucUpdateEntry);

    if ((SucUpdateEntry.NodeID != bNodeID) ||
        (SucUpdateEntry.changeType != SUC_ADD))
    {
      /* Here we register the new change. */
      SUCUpdateNodeInfo(bNodeID, SUC_UPDATE_RANGE, 0);
      return(didWrite);
    }

  }
  return(false);
}


/*========================   SUCUpdateControllerList   =======================
**    Function description
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
static void
SUCUpdateControllerList(
  node_id_t bNodeID)
{
  if ((ZW_IS_SUC) &&
      (CtrlStorageGetCtrlSucUpdateIndex(bNodeID) == ((SUCLastIndex != 1) ? SUCLastIndex - 1 : SUC_MAX_UPDATES)))
  {
    /* if only change is this new node then set Controller as Updated */
    CtrlStorageSetCtrlSucUpdateIndex(bNodeID, SUCLastIndex);
  }
}


#endif /* defined(ZW_CONTROLLER_STATIC) */


/*----------------------------- SUCTimeOut ------------------------------------------
a time out function called when the controller waited too long to receive a frame.
--------------------------------------------------------------------------------------*/
void
ZCB_SUCTimeOut( __attribute__((unused)) SSwTimer* pTimer)
{
  if ((SUC_Update.updateState == SUC_SET_NODE_WAIT_ACK)||
      (SUC_Update.updateState == SUC_SET_NODE_WAIT_ACK_FRAME))
  {
    ZW_TransmitCallbackInvoke(&SUCCompletedFunc, ZW_SUC_SET_FAILED, NULL);
    ResetSUC();
  }
#if defined(ZW_CONTROLLER_STATIC)
  else if ((SUC_Update.updateState >= SEND_SUC_UPDATE_NODE_INFO_SEND) &&
           (SUC_Update.updateState <= SEND_SUC_UPDATE_NODE_RANGE_WAIT))
  {
    SendSUCTransferEnd(scratchID, ZWAVE_TRANSFER_UPDATE_ABORT);
  }
#endif
  else /*All other states*/
  {
    /* Should be SUC_Update.updateState == SUC_UPDATE_STATE_WAIT_NODE_INFO
       but in anycase we had an timeout. So tell application*/
    RestartAnalyseRoutingTable();
    ZW_TransmitCallbackInvoke(&SUCCompletedFunc, ZW_SUC_UPDATE_ABORT, NULL);
    ResetSUC();
  }
}


/*============================   StartSUCTimer   =============================
**    Function description
**        Start Network Update timeout
**    Side effects:
**
**--------------------------------------------------------------------------*/
static void
StartSUCTimer(void)
{
  NetWorkTimerStart(&SUC_Update.TimeoutTimer, ZCB_SUCTimeOut, 0xFE * 10 * (SUC_REQUEST_UPDATE_TIMEOUT + 1));
}


#if defined(ZW_CONTROLLER_STATIC)

static void GetNextSUCUpdate(bool firstTest);

/*=========================   SendSUCUpdateCallback   ========================
**    Function description
**      Callback used by SUC after an update frame have been sent
**    Side effects:
**
**--------------------------------------------------------------------------*/
static void
ZCB_SUCSendNodeUpdateCallback(
  __attribute__((unused)) ZW_Void_Function_t Context,
  uint8_t txStatus,
  __attribute__((unused)) TX_STATUS_TYPE *txStatusReport)
{
  if (SUC_Update.updateState == SEND_SUC_UPDATE_NODE_INFO_WAIT)
  {
    if (txStatus == TRANSMIT_COMPLETE_OK)
    {
      SUC_Update.updateState = SEND_SUC_UPDATE_NODE_RANGE_SEND;
      //return;
    }
    StartSUCTimer();
  }
  else if (SUC_Update.updateState == SEND_SUC_UPDATE_NODE_RANGE_WAIT)
  {
    if (txStatus == TRANSMIT_COMPLETE_OK)
    {
      SUC_Update.updateState = SEND_SUC_UPDATE_NODE_INFO_SEND;
      //return;
    }
    StartSUCTimer();
  }
  /* TO#3196 fix - If Command Complete allready received then transmit next new registered frame. */
  if (cmdCompleteSeen)
  {
    GetNextSUCUpdate(false);
  }
}
#endif  /* CONTROLLER_STATIC */


/*===========================   SUCUpdateCallback   ==========================
**    Function description
**      a call back function used when the controller send frames part of
**      the automatic update process. Acts as the states machine of the
**      update process. Callback used by SUC after an update frame have
**      been sent
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
static void
ZCB_SUCUpdateCallback(
  __attribute__((unused)) ZW_Void_Function_t Context,
  uint8_t txStatus,
  __attribute__((unused)) TX_STATUS_TYPE *txStatusReport)
{
  if (SUC_Update.updateState == REQUEST_SUC_UPDATE_WAIT_START_ACK )
  {
    /* We are trying to receive updates from SUC */
    if (txStatus == TRANSMIT_COMPLETE_OK)
    {
      SUC_Update.updateState = SUC_UPDATE_STATE_WAIT_NODE_INFO;
      StartSUCTimer();
    }
    else
    {
      ResetSUC();
      PendingNodeUpdateEnd();
      ZW_TransmitCallbackInvoke(&SUCCompletedFunc, ZW_SUC_UPDATE_ABORT, NULL);
    }
  }
}


/*----------------------------- SUCSendCmdComp ----------------------------------------
send a command complete frame when the controller received an
update frame - NEW_NODE_REGISTERED/NEW_RANGE_REGISTERED
-------------------------------------------------------------------------------------*/
static void
SUCSendCmdComp(
  uint8_t seq)
{
  assign_ID.assignIdState = ASSIGN_LOCK;
  assignIdBuf.CmdCompleteFrame.cmdClass = ZWAVE_CMD_CLASS_PROTOCOL;
  assignIdBuf.CmdCompleteFrame.cmd = ZWAVE_CMD_CMD_COMPLETE;
  assignIdBuf.CmdCompleteFrame.seqNo = seq;
  StartSUCTimer();
  /* Let the SUCTimer handle the case if transmitBuffer full.. */
  static const STransmitCallback TxCallback = { .pCallback = NULL, .Context = 0 };
  EnQueueSingleData(false, g_nodeID, staticControllerNodeID, (uint8_t *)&assignIdBuf, sizeof(COMMAND_COMPLETE_FRAME),
                         (TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE), 0, // 0ms for tx-delay (any value)
                         ZPAL_RADIO_TX_POWER_DEFAULT, &TxCallback);
}


#if defined(ZW_CONTROLLER_STATIC)
/*----------------------------- SUCSendNodeUpdate --------------------------------
This function send the the updates information to the calling controller
-------------------------------------------------------------------------------------*/
static void
SUCSendNodeUpdate(void)
{
  SUC_UPDATE_ENTRY_STRUCT SucUpdateEntry;
 uint8_t NodeIdForUpdating;
#ifdef ZW_NETWORK_UPDATE_REPLICATE
  if (updateReplicationsNodeID)
  {
    NodeIdForUpdating = updateReplicationsNodeID;
  }
  else
#endif
  {
    if (bUpdateIndex >= SUC_MAX_UPDATES)
    {
      bUpdateIndex = 0;
    }
    /* Get the node ID of the changed node */
    CtrlStorageGetSucUpdateEntry(bUpdateIndex, &SucUpdateEntry);
    NodeIdForUpdating = SucUpdateEntry.NodeID;
    if (!NodeIdForUpdating)
    {
      /* TODO - What to do??? This must never occure! */
      SendSUCTransferEnd(scratchID, ZWAVE_TRANSFER_UPDATE_OVERFLOW);
      return;
    }
  }
  assign_ID.assignIdState = ASSIGN_LOCK;
  uint32_t iLength = 0;
  /* TO#3196 fix - Do not start next SUC Update transmit before previous transmit is done. */
  if (SUC_Update.updateState == SEND_SUC_UPDATE_NODE_INFO_SEND)
  {
    SUC_Update.updateState = SEND_SUC_UPDATE_NODE_INFO_WAIT;
    memset((uint8_t *)&assignIdBuf.newNodeInfo, 0, sizeof(assignIdBuf.newNodeInfo));
    assignIdBuf.newNodeInfo.cmdClass = ZWAVE_CMD_CLASS_PROTOCOL;
    assignIdBuf.newNodeInfo.cmd = ZWAVE_CMD_NEW_NODE_REGISTERED;
    assignIdBuf.newNodeInfo.nodeID = NodeIdForUpdating;
#ifdef ZW_NETWORK_UPDATE_REPLICATE
    if (updateReplicationsNodeID)
    {
      ZW_GetNodeProtocolInfo(NodeIdForUpdating, (NODEINFO *)&assignIdBuf.newNodeInfo.capability);
      assignIdBuf.newNodeInfo.nodeInfo[0] = 0;  /* We do not have any command classes to send */
    }
    else
#endif
    {
      if (SucUpdateEntry.changeType == SUC_ADD)
      {
        ZW_GetNodeProtocolInfo(NodeIdForUpdating, (NODEINFO *)&assignIdBuf.newNodeInfo.capability);
        memcpy(assignIdBuf.newNodeInfo.nodeInfo, SucUpdateEntry.nodeInfo, SUC_UPDATE_NODEPARM_MAX);
      }
      else if (SucUpdateEntry.changeType == SUC_UPDATE_RANGE)
      {
        ZW_GetNodeProtocolInfo(NodeIdForUpdating, (NODEINFO *)&assignIdBuf.newNodeInfo.capability);
        assignIdBuf.newNodeInfo.nodeInfo[0] = 0;  /* We do not have any command classes to send */
      }
      else
      {
        assignIdBuf.newNodeInfo.nodeType.generic = 0;
      }
    }

    if (assignIdBuf.newNodeInfo.nodeType.generic)
    {
      /* TO#3281 partial fix - If node has NODEPARM_MAX command classes a buffer overflow can occure */
      while ((iLength < NODEPARM_MAX) && assignIdBuf.newNodeInfo.nodeInfo[iLength])
      {
        iLength++;
      }
    }

    if (!(assignIdBuf.newNodeInfo.security & ZWAVE_NODEINFO_CONTROLLER_NODE))  /* Slave node ? */
    {
      iLength += (sizeof(NEW_NODE_REGISTERED_SLAVE_FRAME) - NODEPARM_MAX);
      /* When we transmit slave nodeinfo the Basic Device Type is not transmitted... */
      uint8_t tmpBuffer[NODEPARM_MAX + sizeof(APPL_NODE_TYPE)];
      memcpy(tmpBuffer,
             &assignIdBuf.newNodeInfo.nodeType.generic,
             iLength - offsetof(NEW_NODE_REGISTERED_SLAVE_FRAME, nodeType));

      memcpy(&assignIdBuf.newNodeInfo.nodeType.basic,
             tmpBuffer,
             iLength - offsetof(NEW_NODE_REGISTERED_SLAVE_FRAME, nodeType));
    }
    else
    {
      iLength += (sizeof(NEW_NODE_REGISTERED_FRAME) - NODEPARM_MAX);
    }
  }
  /* TO#3196 fix - Do not start next SUC Update transmit before previous transmit is done. */
  else if (SUC_Update.updateState == SEND_SUC_UPDATE_NODE_RANGE_SEND)
  {
    SUC_Update.updateState = SEND_SUC_UPDATE_NODE_RANGE_WAIT;
    assignIdBuf.newRangeInfo.cmdClass = ZWAVE_CMD_CLASS_PROTOCOL;
    assignIdBuf.newRangeInfo.cmd = ZWAVE_CMD_NEW_RANGE_REGISTERED;
    assignIdBuf.newRangeInfo.nodeID = NodeIdForUpdating;
    SET_CURRENT_ROUTING_SPEED(RF_SPEED_9_6K);
    ZW_GetRoutingInfo(NodeIdForUpdating, assignIdBuf.newRangeInfo.maskBytes, ZW_GET_ROUTING_INFO_ANY);
    for (uint8_t i = 0; i < MAX_NODEMASK_LENGTH; i++)
    {
      if (assignIdBuf.newRangeInfo.maskBytes[i])
      {
        iLength = i + 1;
      }
    }
    assignIdBuf.newRangeInfo.numMaskBytes = iLength;
    iLength += (sizeof(NEW_RANGE_REGISTERED_FRAME) - MAX_NODEMASK_LENGTH);
  }
  StartSUCTimer();
  static const STransmitCallback TxCallback = { .pCallback = ZCB_SUCSendNodeUpdateCallback, .Context = 0 };
  EnQueueSingleData(false, g_nodeID, scratchID, (uint8_t *)&assignIdBuf, iLength,
                         (TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE),
                         0, // 0ms for tx-delay (any value)
                         ZPAL_RADIO_TX_POWER_DEFAULT,
                         &TxCallback);
  cmdCompleteSeen = false;
}


#ifdef ZW_NETWORK_UPDATE_REPLICATE
/*=======================   SendNodesExistReplyTimeout   =====================
**    Function description
**      Timeout function for SendNodesExist reply
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
static void
ZCB_SendNodesExistReplyTimeout(SSwTimer* pTimer)
{
  TimerStop(pTimer);

  /* Send DONE status if we are SIS to preserve compatibility with 4.2x
     inclusion controllers */
  if (ZW_IS_SUC && isNodeIDServerPresent())
    SendSUCTransferEnd(scratchID, ZWAVE_TRANSFER_UPDATE_DONE);
  else
    SendSUCTransferEnd(scratchID, ZWAVE_TRANSFER_UPDATE_OVERFLOW);
}

/*=======================   SendNodesExistCompleted   ========================
**    Function description
**      Callback function for SendNodesExist
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
static void
ZCB_SendNodesExistCompleted(
  __attribute__((unused)) void* Context,
  __attribute__((unused)) uint8_t bStatus,
  __attribute__((unused)) TX_STATUS_TYPE *txStatusReport)
{
  /* TODO: should do this when the NODES_EXIST_REPLY arrives successfully */
  NetWorkTimerStart(&SUC_Update.TimeoutTimer, ZCB_SendNodesExistReplyTimeout, 2000);
}


/*==========================   ZW_SendNodesExist   ===========================
**    Function description
**      Sends a nodemask containing the nodes which exists in the network
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
uint8_t                                        /*RET false if transmitter busy else true */
ZW_SendNodesExist(
  uint8_t bNode,                               /* IN NodeID on node to whom nodeExist is to be send */
  const STransmitCallback* pCompletedFunc)  /* IN callback function */
{
  assignIdBuf.nodesExistFrame.cmdClass = ZWAVE_CMD_CLASS_PROTOCOL;
  assignIdBuf.nodesExistFrame.cmd = ZWAVE_CMD_NODES_EXIST;
  assignIdBuf.nodesExistFrame.nodeMaskType = NODES_EXIST_TYPE_ALL;
  ZW_NodeMaskClear(assignIdBuf.nodesExistFrame.nodeMask, MAX_NODEMASK_LENGTH);
  uint32_t i = bMaxNodeID;
  do
  {
    if (CtrlStorageCacheNodeExist(i))
    {
      ZW_NodeMaskSetBit(assignIdBuf.nodesExistFrame.nodeMask, i);
    }
  } while (--i);

  uint32_t iLength = ((bMaxNodeID - 1) >> 3) + 1;
  assignIdBuf.nodesExistFrame.numNodeMask = iLength;
  if (!EnQueueSingleData(false, nodeID, bNode, (uint8_t *)&assignIdBuf, iLength,
                              (TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE),
                              0, // 0ms for tx-delay (any value)
                              ZPAL_RADIO_TX_POWER_DEFAULT,
                              pCompletedFunc))
  {
    ZW_TransmitCallbackInvoke(pCompletedFunc, TRANSMIT_COMPLETE_FAIL, NULL);
    return(false);
  }
  return(true);
}
#endif


/*----------------------------- GetNextSUCUpdate --------------------------------------
when a command compelete is received, this function is called to check for more update
information, if any updates information is available, this will be sent else
transfer end is sent.
--------------------------------------------------------------------------------------*/
static void
GetNextSUCUpdate(
  __attribute__((unused)) bool firstTest)
{
  bUpdateIndex = CtrlStorageGetCtrlSucUpdateIndex(scratchID);


  /* Is index a valid value - if not we treat it as SUC_OUT_OF_DATE */
  if (bUpdateIndex && (bUpdateIndex <= SUC_MAX_UPDATES))
  {
    if (bUpdateIndex == SUCLastIndex)
    {
      SendSUCTransferEnd(scratchID, ZWAVE_TRANSFER_UPDATE_DONE);
    }
    else
    {
      SUCSendNodeUpdate();
    }
  }
  else
  {
#ifdef ZW_NETWORK_UPDATE_REPLICATE
    if (firstTest)
    {
      /* Start with NodeID 1 */
      updateReplicationsNodeID = 1;
    }
    if (updateReplicationsNodeID > bMaxNodeID)
    {
      static const STransmitCallback TxCallback = { .pCallback = ZCB_SendNodesExistCompleted, .Context = 0 };
      ZW_SendNodesExist(scratchID, &TxCallback);
      updateReplicationsNodeID = 0;
    }
    else
    {
      SUCSendNodeUpdate();
    }
#else
    SendSUCTransferEnd(scratchID, ZWAVE_TRANSFER_UPDATE_OVERFLOW);
#endif
  }
}

static void
ZCB_SendRouteUpdate(
  __attribute__((unused)) ZW_Void_Function_t Context,
  uint8_t txStatus,
  __attribute__((unused)) TX_STATUS_TYPE *txStatusReport)
{
  SendRouteUpdate(txStatus);
}

static uint8_t DoAssignReturnRoute(uint8_t *nodeIdList, node_id_t srcNodeId)
{
  uint8_t retVal = ESENDROUTE_ASSIGN_RETURN_ROUTE_NOT_STARTED;
  for (count = 0; count < ZW_MAX_CACHED_RETURN_ROUTE_DESTINATIONS; count++)
  {
    if (nodeIdList[count] != 0)
    {
      retVal = ESENDROUTE_ASSIGN_RETURN_ROUTE_ENDED;
      static const STransmitCallback TxCallback = {.pCallback = ZCB_SendRouteUpdate, .Context = 0};
      if (ZW_AssignReturnRoute(srcNodeId, nodeIdList[count], NULL, false, &TxCallback))
      {
        nodeIdList[count] = 0;
        retVal = ESENDROUTE_ASSIGN_RETURN_ROUTE_INITIATED;
      }
      return retVal;
    }
  }
  return retVal;
}

/*========================= SendRouteUpdate   ===================================
**
**
**
**---------------------------------------------------------------------*/
static ESendRouteUpdateStatus
SendRouteUpdate(
  uint8_t txStatus)
{
  uint8_t retVal = ESENDROUTE_ASSIGN_RETURN_ROUTE_NOT_STARTED;
  if ((TRANSMIT_COMPLETE_OK == txStatus) && (false == sendSUCRouteNow))
  {
    retVal = DoAssignReturnRoute(destNodeIDList, scratchID);
    if (ESENDROUTE_ASSIGN_RETURN_ROUTE_NOT_STARTED != retVal)  return retVal;
    if ( 0 != scratchID )
    {
      CtrlStorageSetRoutingSlaveSucUpdateFlag(scratchID, true, true);
    }
    SendSUCTransferEnd(scratchID, ZWAVE_TRANSFER_UPDATE_DONE);
  }
  else if (staticControllerNodeID && (TRANSMIT_COMPLETE_OK == txStatus) && (true == sendSUCRouteNow))
  {
    sendSUCRouteNow = false;
    retVal = ESENDROUTE_ASSIGN_RETURN_ROUTE_ENDED;
    static const STransmitCallback TxCallback = { .pCallback = ZCB_SendRouteUpdate, .Context = 0 };
    /* We can only be here if SUC exists - so calling AssignReturnRoute without SUCNodeId check is OK */
    if (ZW_AssignReturnRoute(scratchID, GetSUCNodeID(), NULL, true, &TxCallback))
    {
      retVal = ESENDROUTE_ASSIGN_RETURN_ROUTE_INITIATED;
    }
  }
  else
  {
    SendSUCTransferEnd(scratchID, ZWAVE_TRANSFER_UPDATE_ABORT);
    retVal = ESENDROUTE_ASSIGN_RETURN_ROUTE_ENDED;
  }
  return retVal;
}


/*========================= deleteRoutesDone ===================================
**  After the controller finsished deleting the return routes with in the routing slave.
**  The routes update are send to the routing slave
**---------------------------------------------------------------------*/
static void
ZCB_deleteRoutesDone(
  __attribute__((unused)) ZW_Void_Function_t Context, __attribute__((unused)) uint8_t txStatus,
  __attribute__((unused)) TX_STATUS_TYPE *txStatusReport)
{
  sendSUCRouteNow = true;
  SendRouteUpdate(TRANSMIT_COMPLETE_OK);
}
#endif /* defined(ZW_CONTROLLER_STATIC) */


/*============================   SetSUCAckFunc   ============================
**    Function description
**      this call back function is used when trying to set a static controller
**      to work as the SUC. work as the state machine of the process
**    Side effects:
**
**--------------------------------------------------------------------------*/
static void
ZCB_SetSUCAckFunc(
  __attribute__((unused)) ZW_Void_Function_t Context,
  uint8_t txStatus,
  __attribute__((unused)) TX_STATUS_TYPE *txStatusReport)    /*IN TX status*/
{
  if (SUC_Update.updateState == SUC_SET_NODE_WAIT_ACK)
  {
    if (txStatus == TRANSMIT_COMPLETE_OK)
    {
      StartSUCTimer();
      SUC_Update.updateState = SUC_SET_NODE_WAIT_ACK_FRAME;
    }
    else
    {
      ZW_TransmitCallbackInvoke(&SUCCompletedFunc, ZW_SUC_SET_FAILED, NULL);
      ResetSUC();
    }
  }
}


/*------------------------------------------------------------------------
Automatic update code end
--------------------------------------------------------------------------*/

#ifdef ZW_SELF_HEAL
/*============================   SaveOnlyNeighbor   ======================
**    Function description
**      Removes all routing information for bNodeID except bNeighborID which is
**      added as a neighbor
**    Side effects:
**
**--------------------------------------------------------------------------*/
uint8_t
SaveOnlyNeighbor(    /*RET Returns true if success */
  uint8_t bNodeID,      /* IN NodeID to modify neighbors for */
  uint8_t bNeighborID)  /* IN Neighbor to set for bNodeID */
{
  /* Nodes should get a new update when done here */
#if defined(ZW_CONTROLLER_STATIC)
  InvalidateSUCRoutingSlaveList();
#endif
  if (bNodeID != bNeighborID)
  {
    ZW_NodeMaskClear(abNeighbors, MAX_NODEMASK_LENGTH);
    ZW_NodeMaskSetBit(abNeighbors, bNeighborID);
    ROUTING_ANALYSIS_STOP();
    ZW_SetRoutingInfo(bNodeID, MAX_NODEMASK_LENGTH, abNeighbors);
    /*Remove any information that might be outdated*/
    DeleteMostUsed(bNodeID);
    /* TO#1583 fix - RoutingRemoveNonRepeater(bNodeID); */
    return true;
  }
  return false;
}
#endif /*ZW_SELF_HEAL*/


/*-------------------PendingUpdateTimeout  --------------------------------*/
/* This timer time out function used to check the pending update table     */
/* on a regular basis                                                      */
/*-------------------------------------------------------------------------*/
void
ZCB_PendingUpdateTimeout(__attribute__((unused)) SSwTimer * pTimer)
{
  if (--pendingTimerReload == 0)
  {
    if (++pendingTimerCount > PENDING_TIMER_MAX)
    {
      pendingTimerCount = 1;
    }
    pendingTimerReload = (uint16_t)(PENDING_TIMER_RELOAD * pendingTimerCount);
    if (AreSUCAndAssignIdle())
    {
      PendingNodeUpdate();
    }
  }
}


/*=======================   CheckPendingNodeUpdate   =========================
**
**    This function test the pending node table to see if any information is
**    incomplete.
**    If that is the case it will start the pending update timer and set
**    the empty table to false.
**  Side effects:
**--------------------------------------------------------------------------*/
static void
CheckPendingUpdateTable(void)
{
#if !defined(ZW_CONTROLLER_STATIC)
  if (staticControllerNodeID)
#endif
  {
    pendingTableEmpty = true;
    /* was bMaxNodeID - but if the bMaxNodeID node has been removed then current bMaxNodeID will be lower */
    for (pendingNodeID = 1; pendingNodeID <= ZW_MAX_NODES; pendingNodeID++)
    {
      if (CtrlStorageIsNodeInPendingUpdateTable(pendingNodeID))
      {
        pendingTableEmpty = false;
        NetWorkTimerStart(&TimerHandler, ZCB_PendingUpdateTimeout, PENDING_TIMER_TIMEOUT);
        break;
      }
    }
    pendingNodeID = 0x00;
  }
}


/*============================   PendingNodeUpdateEnd   ======================
**    Function description
**        This function is called when something goes wrong while transmitting
**        nodeinformation or rangeinformation to the SUC
**    Side effects:
**
**--------------------------------------------------------------------------*/
static void
PendingNodeUpdateEnd(void)
{
  if (pendingUpdateOn)
  {
    /*We only call the function when we are during inclusion process to notify the application that the protocol inclusion phase is done*/
    ZCB_WaitforRoutingAnalysis(NULL);
  }
  pendingUpdateOn = false;
  assign_ID.assignIdState = ASSIGN_IDLE;
  CheckPendingUpdateTable();
  /* Allow only one pending list flush before going to sleep */
  pendingUpdateNotCompleted = false;
}


/*=======================   PendingNodeUpdate   =========================
**
**  This function used by both the remote and the static controller to update
**  network topology update.
**  In the primary controller the function try to replicate all the new nodes
**  information to the static controller , that failed previously to be replicated
**  to the static controller. The functin is callled after new node
**  is added or removed or when a timer timeout.
**  in the static controller the function try to get the nodes information that
**  failed prevouisly to be updated
**  The function is called when the static controller receive new node
**  registered frame or when a timer timeout.
**
**  Side effects:
**
**--------------------------------------------------------------------------*/
void
PendingNodeUpdate( void )
{
  if (GET_NEW_CTRL_STATE != NEW_CTRL_STOP)
  {
    pendingUpdateOn = false;
    return;
  }
  TimerStop(&TimerHandler);
  if (!pendingTableEmpty)
  {
    if (!pendingUpdateOn)
    {
      /* PendingUpdate was just started. Check list from start */
      pendingNodeID = 0;
    }
    pendingUpdateOn = true;

    /* was bMaxNodeID - but if the bMaxNodeID node has been removed then current bMaxNodeID will be lower */
    while (++pendingNodeID <= ZW_MAX_NODES)
    {
      if(CtrlStorageIsNodeInPendingUpdateTable(pendingNodeID))
      {
        if (!staticControllerNodeID)
        {
          /* No SUC assigned - forget it */
          ClearPendingUpdate(pendingNodeID);
          PendingNodeUpdateEnd();
          return;
        }
#if defined(ZW_CONTROLLER_STATIC)
        if (ZW_IS_SUC)
        {
          /* TO#2424 fix - Do not request for nodeinfo frame when cmdClasses are missing. */
          ClearPendingUpdate(pendingNodeID);
        }
        else
#endif
        {
          assign_ID.assignIdState = PENDING_UPDATE_NODE_INFO_SENT;
          uint8_t len;
          if ((bNewRegisteredNodeFrameSize != 0) && (sNewRegisteredNodeFrame.nodeID == pendingNodeID))
          {
            len = bNewRegisteredNodeFrameSize;
            memcpy((uint8_t*)&assignIdBuf, (uint8_t*)&sNewRegisteredNodeFrame.cmdClass, len);
          }
          else
          {
            len = offsetof(NEW_NODE_REGISTERED_FRAME, nodeInfo);
            assignIdBuf.newNodeInfo.cmdClass = ZWAVE_CMD_CLASS_PROTOCOL;
            assignIdBuf.newNodeInfo.cmd = ZWAVE_CMD_NEW_NODE_REGISTERED;
            assignIdBuf.newNodeInfo.nodeID = pendingNodeID;
            if (CtrlStorageCacheNodeExist(pendingNodeID))
            {
              ZW_GetNodeProtocolInfo(pendingNodeID, (NODEINFO *)&assignIdBuf.newNodeInfo.capability);
              if (!(assignIdBuf.newNodeInfo.security & ZWAVE_NODEINFO_CONTROLLER_NODE))  /* Slave node ? */
              {
               /* When we transmit slave nodeinfo the Basic Device Type is not transmitted... */
               /*We also handle newNodeInfo (type NEW_NODE_REGISTERED_FRAME) as NEW_NODE_REGISTERED_SLAVE_FRAME  ( 1 byte smaller)*/
                assignIdBuf.newNodeInfo.nodeType.basic = assignIdBuf.newNodeInfo.nodeType.generic;
                assignIdBuf.newNodeInfo.nodeType.generic = assignIdBuf.newNodeInfo.nodeType.specific;
                assignIdBuf.newNodeInfo.nodeType.specific = 0; /*this is the first nodeinfo byte in NEW_NODE_REGISTERED_SLAVE_FRAME*/
                /*Since NEW_NODE_REGISTERED_SLAVE_FRAMEis one byte shorter, we decrement the lenght of sent bytes since the length is incremented later*/
                len--;
              }

            }
            else
            {
              memset(&assignIdBuf.newNodeInfo.capability, 0, offsetof(NEW_NODE_REGISTERED_FRAME, nodeInfo) - offsetof(NEW_NODE_REGISTERED_FRAME, security));
            }
            assignIdBuf.newNodeInfo.nodeInfo[0] = 0;
            len++;
          }
          static const STransmitCallback TxCallback = { .pCallback = ZCB_AssignTxComplete, .Context = 0 };
          if (!EnQueueSingleData(false, g_nodeID, staticControllerNodeID, (uint8_t *)&assignIdBuf, len,
                                    (TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE),
                                    0, // 0ms for tx-delay (any value)
                                    ZPAL_RADIO_TX_POWER_DEFAULT, &TxCallback))
          {
            PendingNodeUpdateEnd();
          }
        }
        return;
      }
    }
    PendingNodeUpdateEnd();
  }
}


/*========================   SetPendingUpdate   ==========================
**
**  Sets the specified node bit in the new node pending update table
**
**  Side effects:
**
**--------------------------------------------------------------------------*/
void
SetPendingUpdate( /* RET  Nothing */
  node_id_t bNodeID)   /* IN   Node ID that should be set */
{
  if (!ZW_IS_SUC || CtrlStorageListeningNodeGet(bNodeID))
  {
    /* TODO: Handle battery operated nodes.. Currently their NodeInfo will not be requested */
    if (0 != bNodeID)
    {
      CtrlStorageChangeNodeInPendingUpdateTable(bNodeID, true);
    }
    pendingTableEmpty = false;
  }
  if (!TimerIsActive(&TimerHandler))
  {
    pendingTimerReload = PENDING_TIMER_RELOAD;
    pendingTimerCount = 0;
    NetWorkTimerStart(&TimerHandler, ZCB_PendingUpdateTimeout, PENDING_TIMER_TIMEOUT);
  }
}


/*========================   ClearPendingUpdate   ============================
**
**  Clear the specified node bit in the new node pending update table
**
**
**  Side effects:
**
**--------------------------------------------------------------------------*/
static void
ClearPendingUpdate( /* RET  Nothing */
  node_id_t bNodeID)             /* IN   Node ID to be informed about */
{
#if !defined(ZW_CONTROLLER_STATIC)
  if (staticControllerNodeID && primaryController)
  {
#endif
    if ( 0 != bNodeID )
    {
      CtrlStorageChangeNodeInPendingUpdateTable(bNodeID, false);
    }

#if !defined(ZW_CONTROLLER_STATIC)
  }
#endif
  if ((bNewRegisteredNodeFrameSize != 0) && (sNewRegisteredNodeFrame.nodeID == bNodeID))
  {
    bNewRegisteredNodeFrameSize = 0;
  }
}


/*============================   DeleteNodeTimeout   =========================
**    Function description
**      Timeout function for delaying the notification to controller
**      application when a node has been removed.
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
static void
DeleteNodeTimeout(void)
{
  TimerStop(&UpdateTimeoutTimer);

  AddNodeInfo(scratchID, NULL, false);
  ClearPendingUpdate(scratchID);
  ProtocolInterfacePassToAppNodeUpdate(UPDATE_STATE_DELETE_DONE, scratchID, NULL, 0);
  if (SUC_Update.updateState == SUC_UPDATE_STATE_WAIT_NODE_INFO)
  {
    SUCSendCmdComp(3);
  }
}


/*============================   RestartRoutingAnalysis   ======================
**    Function description
**      Timeout used to restart Routing analysis
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
static void
ZCB_RestartRoutingAnalysis(__attribute__((unused)) SSwTimer* pTimer)
{
  // NOTE: Method is sometimes called directly with null pointer!
  if (--count)
  {
    /* Timeout function is called from normal code, start timer if not running */
    NetWorkTimerStart(&UpdateTimeoutTimer, ZCB_RestartRoutingAnalysis, 1000);
  }
  else
  {
    TimerStop(&UpdateTimeoutTimer);
    if (!pendingTableEmpty)
    {
      NetWorkTimerStart(&TimerHandler, ZCB_PendingUpdateTimeout, PENDING_TIMER_TIMEOUT);
    }
    RestartAnalyseRoutingTable();
  }
}


/*===============================   LearnInfoCallBack   =====================
**    Learn state change, report via call back function.
**
**--------------------------------------------------------------------------*/
static void   /*RET Nothing */
LearnInfoCallBack(
  uint8_t       bStatus,      /*IN Status of learn mode */
  node_id_t  bSource,      /*IN Node id of the node that send node info */
  uint8_t       bLen)         /*IN Node info length                        */
{
  DPRINTF("LearnInfoCallBack: %x \n", bStatus);
  if (zcbp_learnCompleteFunction)
#ifdef ZW_ROUTED_DISCOVERY
  {
    if (LEARN_STATE_FIND_NEIGBORS_DONE == bStatus)
    {
      if (findNeighborsWithFlirs) // don't call the callback
        return;
      else
          resetFindNeighborsWithFlirs();
    }
    if (updateNodeNeighbors)
    {
       learnNodeInfo.bStatus = bStatus;
    }
    else
    {
      /* Make application callback */
      learnNodeInfo.bStatus = bStatus;
      learnNodeInfo.bSource = bSource;
      learnNodeInfo.pCmd = (uint8_t*)&newNodeInfoBuffer.nodeInfoFrame.nodeType;
      learnNodeInfo.bLen = bLen;
    }
    zcbp_learnCompleteFunction(&learnNodeInfo);
  }

  if ((bStatus == LEARN_STATE_DONE) ||
      (bStatus == LEARN_STATE_FAIL) ||
      (bStatus == LEARN_STATE_NO_SERVER))
  {
    /* TO#1278 fix - LearnNodeState is not reset after ZW_RequestNeighborUpdate*/
    assign_ID.assignIdState = ASSIGN_IDLE;
    updateNodeNeighbors = false;
    bNeighborUpdateFailed = false;
    g_learnNodeState = LEARN_NODE_STATE_OFF;

    /* Remove last working route to avoid incorrect speed information */
    if ((bSource) && (bSource <= ZW_MAX_NODES) && (bStatus == LEARN_STATE_DONE))
    {
      ZW_LockRoute(false);
#ifdef MULTIPLE_LWR
      LastWorkingRouteCacheLineExile(bSource, CACHED_ROUTE_ZW_ANY);
#else
      LastWorkingRouteCacheLinePurge(bSource);
#endif  /* #ifdef MULTIPLE_LWR */
    }
  }
#else /*ZW_ROUTED_DISCOVERY*/
  {
    learnNodeInfo.bStatus = bStatus;
    learnNodeInfo.bSource = bSource;
    learnNodeInfo.pCmd = (uint8_t*)&newNodeInfoBuffer.nodeInfoFrame.nodeType;
    learnNodeInfo.bLen = bLen;
    zcbp_learnCompleteFunction(&learnNodeInfo);
  }
#endif /* ZW_ROUTED_DISCOVERY */
}


/*=========================   AssignTimerStop   ===========================
**
**  Stop retransmit timeout timer.
**
**
**--------------------------------------------------------------------------*/
static void               /*RET Nothing */
AssignTimerStop( void ) /*IN  Nothing */
{
  TimerStop(&assign_ID.TimeoutTimer); /* stop retransmissions timer */
}


/*=========================   AssignTimerStart   ==========================
**
**  Activate retransmit timeout timer.
**
**
**--------------------------------------------------------------------------*/
static void                   /*RET Nothing */
AssignTimerStart(
  uint32_t Timeout)            /* IN Timeout value (value * 1 msec.)  */
{
  TimerSetCallback(&assign_ID.TimeoutTimer, ZCB_FindNodesTimeout);
  TimerStart(&assign_ID.TimeoutTimer, Timeout);
}


/*=========================   ReturnRouteTxComplete   =============================
**
**  Assign Return Route transmit completed handling function.
**
**--------------------------------------------------------------------------*/
static void                /*RET Nothing */
ZCB_ReturnRouteTxComplete(
  __attribute__((unused)) ZW_Void_Function_t Context,
  uint8_t txStatus,    /*IN  Transmit status */
  TX_STATUS_TYPE *pTxStatusReport)
{
  if (txStatus == TRANSMIT_COMPLETE_OK)
  {
    SendReturnRoute(); /* transmit next return route */
  }
  else
  {
    sucAssignRoute = false;
    returnRouteAssignActive = false;

    /* report transmit error */
    ZW_TransmitCallbackInvoke(&cmdCompleteReturnRouteFunc, txStatus, pTxStatusReport);
  }
}


/*-----------------------------------------------------------------------------------------------*/
/* This founction try to find a route that consist only of 40k nodes, if this is not possible    */
/* We try to find a mixed route                                                                  */
/*===============================================================================================*/
static bool
FindReturnRoutes(
  uint8_t *hopList,
  uint8_t *hopCount,
  bool fResetNeighbors) /*IN Reset bNextNeighbor in GetNextRouteToNode() if true */
{
  /* we are not the destination call GetNextRouteFromNodeToNode function */
  return GetNextRouteFromNodeToNode(returnDestNodeID, returnRouteNodeID,
                                      hopList,
                                      hopCount, returnRouteNumber + (returnRouteNoDirect ? 1 : 0),
                                      fResetNeighbors);
}

/*===========   ConvertRoutingSpeedToReturnRouteSpeed   ======================
**
**  Converts from bRoutingSpeed to Return Route speeds.
**
**--------------------------------------------------------------------------*/
uint8_t ConvertRoutingSpeedToReturnRouteSpeed(uint8_t bRoutinSpeed)
{
  if (bRoutinSpeed == RF_SPEED_100K)
  {
    return ROUTE_SPEED_BAUD_100000;
  }
  else if(bRoutinSpeed == RF_SPEED_40K)
  {
    return  ROUTE_SPEED_BAUD_40000;
  }
  else
  {
    return ROUTE_SPEED_BAUD_9600;
  }
}

static uint8_t findPriorityRoute(void)
{
  uint8_t repeaters;
  for (repeaters = 0; repeaters < MAX_REPEATERS; repeaters++)
  {
    if (returnRoutePriorityBuffer[repeaters] == 0)
      break;
  }
  memcpy(assignReturnRouteBuf.repeaterList, returnRoutePriorityBuffer, MAX_REPEATERS);
  assignReturnRouteBuf.nodeID = returnDestNodeID;
  assignReturnRouteBuf.routeNoNumHops = (returnRouteNumber << 4);
  assignReturnRouteBuf.routeNoNumHops += repeaters;
  if (returnRoutePriorityBuffer[0] == 0)
  {
    returnRoutePriorityNumber = 0;
  }
  else
  {
    returnRoutePriorityNumber = returnRouteNumber;
  }
  returnRouteNumber++;
  /* Here we  determine and indicate wakeup sensor type */
  /* Wakeup type for src and dst are placed in the routeSpeed byte : */
  /* dst wakeup type mask = 00000110 (bit1 and bit2) */
  /* src wakeup type mask = 11000000 (bit6 and bit7) */
  /* The Route speed mask = 00111000 (bit3, bit4 and bit5) */
  /* returnRouteNodeID is the src and returnDestNodeID is the dst */
  assignReturnRouteBuf.routeSpeed = ConvertRoutingSpeedToReturnRouteSpeed(returnRoutePriorityBuffer[MAX_REPEATERS]);
  assignReturnRouteBuf.routeSpeed |= ((GetNodesSecurity(returnRouteNodeID) & ZWAVE_NODEINFO_SENSOR_MODE_MASK) << 1) |
                                     ((GetNodesSecurity(returnDestNodeID) & ZWAVE_NODEINFO_SENSOR_MODE_MASK) >> 4);
  return repeaters;
}

/*Try to find a return route*/
static uint8_t FindReturnRoute(bool resetRouteSearch)
{
  uint8_t repeaters = 0;
  bool fContinueRouteSearch = resetRouteSearch;
  do /* Loop over all speeds until a route is found */
  {
    if (FindReturnRoutes(assignReturnRouteBuf.repeaterList, &repeaters, fContinueRouteSearch))
    {
      /* Check if this route was already tried at a different speed */
      if (!(DoesRouteSupportTriedSpeed(assignReturnRouteBuf.repeaterList, repeaters, bReturnRouteTriedSpeeds)))
      {
        /* transmit next route */
        assignReturnRouteBuf.nodeID = returnDestNodeID;
        assignReturnRouteBuf.routeNoNumHops = (returnRouteNumber << 4);
        assignReturnRouteBuf.routeNoNumHops += repeaters;
        returnRouteNumber++;
        /* Here we  determine and indicate wakeup sensor type */
        /* Wakeup type for src and dst are placed in the routeSpeed byte : */
        /* dst wakeup type mask = 00000110 (bit1 and bit2) */
        /* src wakeup type mask = 11000000 (bit6 and bit7) */
        /* The Route speed mask = 00111000 (bit3, bit4 and bit5) */
        /* returnRouteNodeID is the src and returnDestNodeID is the dst */
        assignReturnRouteBuf.routeSpeed = ConvertRoutingSpeedToReturnRouteSpeed(bCurrentRoutingSpeed);
        assignReturnRouteBuf.routeSpeed |= ((GetNodesSecurity(returnRouteNodeID) & ZWAVE_NODEINFO_SENSOR_MODE_MASK) << 1) |
                                           ((GetNodesSecurity(returnDestNodeID) & ZWAVE_NODEINFO_SENSOR_MODE_MASK) >> 4);
        /* Route found, exit while loop */
        break;
      }
      else
      {
        /* Route was tried at a higher speed */
        /* Loop and try next neighbor node */
        fContinueRouteSearch = true;
      }
    }
    else
    {
      /* No more routes found at this speed */
      MarkSpeedAsTried(bCurrentRoutingSpeed, &bReturnRouteTriedSpeeds);
      fContinueRouteSearch = NextLowerSpeed(returnRouteNodeID, returnDestNodeID);
      if (!fContinueRouteSearch) /* no more routes found, delete the remaining routes */
      {
        returnRouteState = RETURN_ROUTE_STATE_DELETE;
      }
    }
  } while (fContinueRouteSearch);

  return repeaters;
}

static uint8_t BuildAssignReturnRoute(bool forceDirect)
{
  uint8_t repeaters = 0;
  /* First route can be direct if, one of the parts in the route do not have any neighbours or they are neighbours */
  if (!returnRouteNumber &&
      (!ZCB_HasNodeANeighbour(returnDestNodeID) ||
       !ZCB_HasNodeANeighbour(returnRouteNodeID) ||
       ZW_AreNodesNeighbours(returnDestNodeID, returnRouteNodeID) ||
       true == forceDirect))
  {
    /* Only first return route can be direct... */
    /* Start with highest speed supported by src and dest */
    SET_CURRENT_ROUTING_SPEED(MaxCommonSpeedSupported(returnRouteNodeID, returnDestNodeID));

    // if we are assigning priority without repeaters then the direct route speed should be overwritten
    if ((returnRoutePriorityPresent == PRIORITY_ROUTE_WAITING) &&
        ( 0 == returnRoutePriorityBuffer[0])) {
      bCurrentRoutingSpeed = returnRoutePriorityBuffer[MAX_REPEATERS];
    }

    returnRouteNoDirect = false;
    repeaters = 0; /* first return route is direct */
    assignReturnRouteBuf.nodeID = returnDestNodeID;
    assignReturnRouteBuf.routeNoNumHops = 0;
    returnRouteNumber++;
    /* Here we  determine and indicate wakeup sensor type */
    /* Wakeup type for src and dst are placed in the routeSpeed byte : */
    /* dst wakeup type mask = 00000110 (bit1 and bit2) */
    /* src wakeup type mask = 11000000 (bit6 and bit7) */
    /* The Route speed mask = 00111000 (bit3, bit4 and bit5) */
    /* returnRouteNodeID is the src and returnDestNodeID is the dst */

    assignReturnRouteBuf.routeSpeed = ConvertRoutingSpeedToReturnRouteSpeed(bCurrentRoutingSpeed);
    assignReturnRouteBuf.routeSpeed |= ((GetNodesSecurity(returnRouteNodeID) & ZWAVE_NODEINFO_SENSOR_MODE_MASK) << 1) |
                                       ((GetNodesSecurity(returnDestNodeID) & ZWAVE_NODEINFO_SENSOR_MODE_MASK) >> 4);
  }
  else
  {
    if (!returnRouteNumber)
    {
      returnRouteNoDirect = true;

      /* Start with highest speed supported by bNextNode */
      SET_CURRENT_ROUTING_SPEED(MaxCommonSpeedSupported(returnRouteNodeID, returnDestNodeID));
    }
    /* Check if we should send priority route */
    /* and if priority route is direct and this is not route 0 then drop it */
    if ((returnRoutePriorityPresent == PRIORITY_ROUTE_WAITING) && (0 != returnRoutePriorityBuffer[0]))
    {
      repeaters = findPriorityRoute();
      returnRoutePriorityPresent = PRIORITY_ROUTE_RESET_SEARCH;
    }
    else
    {
      // if we trying to create return route after we set the priority route then reset the return route search
      bool resetSearch = false;
      if (returnRoutePriorityPresent == PRIORITY_ROUTE_RESET_SEARCH) {
        resetSearch = true;
      }
      if ((returnRoutePriorityPresent == PRIORITY_ROUTE_WAITING) ||
          (returnRoutePriorityPresent == PRIORITY_ROUTE_RESET_SEARCH) ) {
        returnRoutePriorityPresent = PRIORITY_ROUTE_SEND;
      }
      repeaters = FindReturnRoute(resetSearch);
    }
  }
  return repeaters;
}

static bool DeleteReturnRoute(void)
{
#ifdef ZW_CONTROLLER_STATIC
  if ((!returnRouteNumber) && (returnDestNodeID))
  {
    /* If current is first route and there is a destination node (not delete routes) */
    /* then we do not send any routes and report to app. that no routes was found */
    sucAssignRoute = false;
    returnRouteAssignActive = false;
    /* report completed: NO ROUTE */

    ZW_TransmitCallbackInvoke(&cmdCompleteReturnRouteFunc, TRANSMIT_COMPLETE_NOROUTE, NULL);
    return true; /* Just get out - we do not have anything to send */
  }
  else
#endif
  {
    assignReturnRouteBuf.nodeID = 0; /* delete return route */
    assignReturnRouteBuf.routeNoNumHops = (returnRouteNumber << 4);
    returnRouteNumber++;
  }
  /* TODO - Here we need to determine and indicate wakeup sensor type */
  /* Wakeup type for src and dst is in the routeSpeed byte : */
  /* dst wakeup type mask = 00000110 (bit1 and bit2) */
  /* src wakeup type mask = 11000000 (bit6 and bit7) */
  /* The Route speed mask = 00111000 (bit3, bit4 and bit5) */
  assignReturnRouteBuf.routeSpeed = (uint8_t)ROUTE_SPEED_BAUD_9600;
  return false;
}
  // TODO: Move this function to ZW_routing.c and make bCurrentRoutingSpeed static again
  //       and make NextLowerSpeed(), DoesNodeSupportTriedSpeed() private again
  //       Also move ConvertRoutingSpeedToReturnRouteSpeed() to ZW_routing.c
  /*=========================   SendReturnRoute   =============================
  **
  **  Build an "Assign Return Route" frame and transmit it.
  **
  **--------------------------------------------------------------------------*/
  static void           /*RET Nothing */
  SendReturnRoute(void) /* IN Nothing */
  {
    uint8_t repeaters = 0;
    /* Initialization: No speeds have been tried yet */
    bReturnRouteTriedSpeeds = 0;
    /* Only send AssingSucReturnRoute one time if LearnMode*/
    if (((returnRouteNumber < RETURN_ROUTE_MAX) && (g_learnNodeState != LEARN_NODE_STATE_NEW)) || (returnRouteNumber < 1))
    {
      bool wasReturnRouteState = returnRouteState;
      /* Build assign return route frame */
      assignReturnRouteBuf.cmdClass = ZWAVE_CMD_CLASS_PROTOCOL;
      assignReturnRouteBuf.cmd = sucAssignRoute ? ZWAVE_CMD_ASSIGN_SUC_RETURN_ROUTE : ZWAVE_CMD_ASSIGN_RETURN_ROUTE;
      if (returnRouteState == RETURN_ROUTE_STATE_ASSIGN)
      {
        repeaters = BuildAssignReturnRoute(false);
      }
      if (returnRouteState == RETURN_ROUTE_STATE_DELETE)
      {
        if ((0 == returnRouteNumber) && (RETURN_ROUTE_STATE_ASSIGN == wasReturnRouteState) && (0 != returnDestNodeID) && (true == sucAssignRoute))
        {
          /* Assign SUC Return Route always needs one SUC Return Route to deliver the SUC nodeID */
          returnRouteState = RETURN_ROUTE_STATE_ASSIGN;
          repeaters = BuildAssignReturnRoute(true);
        }
        else
        { /* delete return route (route destination = ZERO) */
          repeaters = 0;                   /* no repeaters */
          if (DeleteReturnRoute()) {
            return;
          }
        }
      }
      assignReturnRouteBuf.repeaterList[repeaters] = assignReturnRouteBuf.routeSpeed;
      /* Send assign route/suc route frame to node */
      static const STransmitCallback TxCallback = {.pCallback = ZCB_ReturnRouteTxComplete, .Context = 0};
      uint8_t len = sizeof(ASSIGN_RETURN_ROUTE_FRAME) - MAX_REPEATERS + repeaters;
      if (!EnQueueSingleData(false, g_nodeID, returnRouteNodeID, (uint8_t *)&assignReturnRouteBuf,
                             len,
                             (TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE),
                             0, // 0ms for tx-delay (any value)
                             ZPAL_RADIO_TX_POWER_DEFAULT, &TxCallback))
      {
        ZCB_ReturnRouteTxComplete(0, TRANSMIT_COMPLETE_FAIL, NULL);
      }
    }
    else
    { /* end of route list */
      if (returnRoutePriorityPresent == PRIORITY_ROUTE_SEND)
      {
        /* Build the assign return route priority set frame*/
        assignReturnRouteBuf.cmdClass = ZWAVE_CMD_CLASS_PROTOCOL;
        assignReturnRouteBuf.cmd = sucAssignRoute ? ZWAVE_CMD_ASSIGN_SUC_RETURN_ROUTE_PRIORITY : ZWAVE_CMD_ASSIGN_RETURN_ROUTE_PRIORITY;
        assignReturnRouteBuf.nodeID = returnDestNodeID;
        assignReturnRouteBuf.routeNoNumHops = returnRoutePriorityNumber;
        returnRoutePriorityPresent = PRIORITY_ROUTE_RESET_SEARCH;

        static const STransmitCallback TxCallback = {.pCallback = ZCB_ReturnRouteTxComplete, .Context = 0};
        if (!EnQueueSingleData(false, g_nodeID, returnRouteNodeID, (uint8_t *)&assignReturnRouteBuf,
                               sizeof(ASSIGN_RETURN_ROUTE_PRIORITY_FRAME),
                               (TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE),
                               0, // 0ms for tx-delay (any value)
                               ZPAL_RADIO_TX_POWER_DEFAULT, &TxCallback))
        {
          ZCB_ReturnRouteTxComplete(0, TRANSMIT_COMPLETE_FAIL, NULL);
        }
      }
      else
      {
        sucAssignRoute = false;
        returnRouteAssignActive = false; /* We are done with return route assign/delete */

        /* report completed: OK */
        ZW_TransmitCallbackInvoke(&cmdCompleteReturnRouteFunc, TRANSMIT_COMPLETE_OK, NULL);
      }
    }
  }


#ifdef ZW_CONTROLLER_BRIDGE
/*==========================   RemoveAllVirtualNodes   =======================
**    Remove All Virtual Nodes.
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
static void
RemoveAllVirtualNodes(void)
{
  memset(nodePool, 0, MAX_NODEMASK_LENGTH);
  CtrlStorageWriteBridgeNodePool(nodePool);
}
#endif



/*===============================   SetDefaultCmd   =========================
**    Set Default command handler function.
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
static void           /*RET  Nothing                  */
SetDefaultCmd( bool bClearAll ) /* IN  Nothing                   */
{
  /* Clear settings in internal eeprom */
  /* The following is cleared here:    */
  /* - Preferred repeaters             */
  /* - Pending discovery               */
  /* - Real Time Timers                */
  /* Clear the most used table */
  ClearMostUsed();
  /* Clear settings in external eeprom */
  /* The following is cleared here:    */
  /* - Routing table                   */
  /* - Node information                */
  /* Last used node ID                 */
  /* static controller node id if available */
  if (!NvmFileSystemFormat())
  {
    ASSERT(false);
  }
  ProtocolInterfaceReset();

/* TO#1873 Fix - SIS controller after power reset doesn't send Reserved ID to Inclusion Controller */
#if defined(ZW_CONTROLLER_STATIC)
  uint8_t iValue = 1;
  CtrlStorageSetSucLastIndex(iValue);
#endif  /* ZW_CONTROLLER_STATIC */
  /* Clear node id in internal eeprom */
  /* Clear node id in internal eeprom */
  if (bClearAll)
  {
#ifdef ZW_CONTROLLER_BRIDGE
    /* We are doing the full reset so everything is going, also our Virtual Slave Nodes */
    RemoveAllVirtualNodes();
#endif
    controllerOnOther = false;
    primaryController = true;
    realPrimaryController = true;
    SetNodeIDServerPresent(false);

    HomeIdGeneratorGetNewId(ZW_HomeIDGet());

    g_nodeID = NODE_CONTROLLER;
  }
  else
  {
#ifdef ZW_CONTROLLER_BRIDGE
    /* Rewrite the nodePool */
    CtrlStorageWriteBridgeNodePool(nodePool);
#endif
  }
  // Create new HomeId
  HomeIdUpdate(ZW_HomeIDGet(), g_nodeID);

  SaveControllerConfig();
  TimerStop(&TimerHandler);
  TimerStop(&UpdateTimeoutTimer);

  /* TO#2280 fix - After Initializing EEPROM, the various variables need to be initialized */
  /* Reinitialize Nodeinformation according to Application information */
   ControllerInit(m_pVirtualSlaveInfoTable, m_pAppNodeInfo, bClearAll);
  /* TO#2390 - node could hang if TxQueue wasnt empty */
  TxQueueInit();
  ZW_EnableRoutedRssiFeedback(true);
  NoiseDetectInit();
}


/****************************************************************************/
/*                           EXPORTED FUNCTIONS                             */
/****************************************************************************/

#if defined(ZW_CONTROLLER_STATIC)
/*=============================   SetupsucEnabled   ==========================
**    Set SetupsucEnable
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
static void
SetupsucEnabled(void)
{
  sucEnabled = SUC_ENABLED_MASK;
  if (isNodeIDServerPresent())
  {
    sucEnabled |= ZW_SUC_FUNC_NODEID_SERVER;
  }
}
#endif


/*=============================   ControllerInit   ==========================
**    Initialize variables and structures.
**
**    NOTE: ControllerInit is called internally in Controller during
**    SetDefault.
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
void                  /* RET Nothing */
ControllerInit(
                const SVirtualSlaveNodeInfoTable_t * pVirtualSlaveNodeInfoTable,
                const SAppNodeInfo_t * pAppNodeInfo,
                bool bFullInit    // Init all, or only partial variable reset
              )
{
  m_pVirtualSlaveInfoTable = pVirtualSlaveNodeInfoTable;
  m_pAppNodeInfo = pAppNodeInfo;

  uint8_t  bControllerConfiguration;
  uint8_t  bMaxNode;
  node_id_t wMaxNode_LR;

#ifdef ZW_PROMISCUOUS_MODE
  /* Everytime we call ControllerInit we should clear the Promiscuous Mode bit */
  promisMode = false;
#endif

  /* Make sure LWR is not locked */
  ZW_LockRoute(false);

  /* TO#2583 fix - We should only do a full variable reset after RESET or when doing a full SetDefault Reset of module. */
  if (bFullInit)
  {
    controllerVarInit();
  }
  /* We have been reset/ZW_SetDefault one way or the other so release possible allocated memory. */
  bNewRegisteredNodeFrameSize = 0;
  ZW_NodeMaskClear(classicFailedNodesMask, sizeof(classicFailedNodesMask));
  ZW_NodeMaskClear(lrFailedNodesMask, sizeof(lrFailedNodesMask));
  removeFailedNodeCallBack = NULL;
#if defined(ZW_CONTROLLER_STATIC)
  SUCLastIndex = CtrlStorageGetSucLastIndex();
#endif
  /* Retrieve current MaxNodeID */
  bMaxNode = CtrlStorageGetMaxNodeId();
  if (!bMaxNode || (bMaxNode > ZW_MAX_NODES))
  {
    bMaxNode = 0x01;
  }
  bMaxNodeID = bMaxNode;
   /* Retrieve current wMaxNodeID_LR, i.e., maximum LR nodeID */
  wMaxNode_LR = CtrlStorageGetMaxLongRangeNodeId();
  if ( LOWEST_LONG_RANGE_NODE_ID > wMaxNode_LR || HIGHEST_LONG_RANGE_NODE_ID < wMaxNode_LR)
  {
    wMaxNode_LR = 0; // Set to 0 to indicate no LR node IDs are assigned
    CtrlStorageSetMaxLongRangeNodeId(wMaxNode_LR);
  }
  g_wMaxNodeID_LR = wMaxNode_LR;
  /* Setup Primary controller flag. */
  bControllerConfiguration = CtrlStorageGetControllerConfig();
  primaryController = !(bControllerConfiguration & CONTROLLER_IS_SECONDARY);
  controllerOnOther = (bControllerConfiguration & CONTROLLER_ON_OTHER_NETWORK);

  realPrimaryController = (bControllerConfiguration & CONTROLLER_IS_REAL_PRIMARY);
  SetNodeIDServerPresent((bControllerConfiguration & CONTROLLER_NODEID_SERVER_PRESENT));
  if (isNodeIDServerPresent())
  {
    primaryController=true;
  }

  ZW_UpdateCtrlNodeInformation(false);
  staticControllerNodeID = CtrlStorageGetStaticControllerNodeId();
  SyncEventArg1Invoke(&g_ControllerStaticControllerNodeIdChanged, staticControllerNodeID);
#if defined(ZW_CONTROLLER_STATIC)
  if (ZW_IS_SUC)
  {
    if (bFullInit)
    {
      /* We are SUC - update SIS info */
      SetupsucEnabled();
    }

  }
  else
  {
    /* TO#2374 Fix - do not reset learn/replication state variables if we are trying to be included */
    if (bFullInit)
    {
      sucEnabled = SUC_ENABLED_DEFAULT; /* Default we want to be SIS */
    }
  }

#endif /* defined(ZW_CONTROLLER_STATIC) */
  /* TO#2374 Fix - do not reset learn/replication state variables if we are trying to be included */
  if (bFullInit)
  {
    /* Is it a full reset */
    ZW_AddNodeToNetwork(ADD_NODE_STOP, NULL);
    /* TO#4118 fix - Reset of LOST used variables */
#ifdef ZW_SELF_HEAL
#if defined(ZW_CONTROLLER_STATIC)
    lostOngoing = true;
#endif
#endif
    ResetSUC();
    /* TO#3317 fix - We need to reset the RFBusy flag as if set - RF cannot be put in powerdown */
    /* Reset the PendingUpdate functionality */
    PendingNodeUpdateEnd();
  }
#ifdef ZW_CONTROLLER_BRIDGE
  CtrlStorageReadBridgeNodePool(nodePool);
#endif
  /* TO#01763 Fix - Controllers can loose their ability to route using ZW_SendData() */
  /* Reset Routing */
  InitRoutingValues();
  /* Initialize range info to a valid value */
  nodeRange.frame.numMaskBytes = 1;
  nodeRange.frame.maskBytes[0] = 0;
  pendingNodeID = 0x00;
#ifdef ZW_CONTROLLER_BRIDGE
  virtualNodeID = 0;
#endif
  UpdateNodeInformation(&assignIdBuf.nodeInfo);
}


/*===========================   NeighborComplete   ===========================
**    Send a NOP frame to the next node in nodeRange
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
static void        /* RET  Nothiong */
ZCB_FindNeighborComplete(
  __attribute__((unused)) ZW_Void_Function_t Context,
  uint8_t txStatus,   /* In   Transmit status, TRANSMIT_COMPLETE_OK or
                   *                       TRANSMIT_COMPLETE_NO_ACK */
  __attribute__((unused)) TX_STATUS_TYPE *txStatusReport)
{
  uint8_t destID;

  destID = bCurrentRangeCheckNodeID;
  if (txStatus != TRANSMIT_COMPLETE_OK)
  {
    /* The node couldn't be reached, Clear the bit in the range mask */
    nodeRange.frame.maskBytes[destID >> 3] &= ~(0x01 << (0x07 & destID));
  }
  if (g_findInProgress)
  {
    /* Send next NOP to next node in range mask */
    FindNeighbors(destID + 1);
  }

}


/*===========================   SendTestFrame   ==========================
**    Send a NOP_POWER frame to specified nodeID and powerlevel
**
**    Side effects:
**
**    cmdCompleteBuf (3 first bytes) are changed
**
**--------------------------------------------------------------------------*/
uint8_t               /*RET false if transmitter busy else true */
SendTestFrame(
  uint8_t bNodeID,    /* IN nodeID to transmit to */
  uint8_t powerLevel, /* IN powerlevel index */
  const STransmitCallback* pTxCallback) /* Call back function called when done */
{
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
  {
    RfSpeed = RF_SPEED_40K;
  }
  if (bNeighborDiscoverySpeed == ZWAVE_FIND_NODES_SPEED_100K)
  {
    RfSpeed = RF_SPEED_100K;
  }

  if (zensorWakeupTime)
  {
    RfSpeed = RF_SPEED_40K | ((zensorWakeupTime == ZWAVE_SENSOR_WAKEUPTIME_250MS) ? RF_OPTION_SEND_BEAM_250MS : RF_OPTION_SEND_BEAM_1000MS);
  }

#ifdef ZW_CONTROLLER_BRIDGE
  if (virtualNodeID != 0)
  {
    return (ZW_IsVirtualNode(virtualNodeID) &&
           (EnQueueSingleData(RfSpeed, virtualNodeID, bNodeID, (uint8_t *)&cmdBuffer,
                                   sizeof(NOP_POWER_FRAME),
                                   (TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_NO_ROUTE),
                                   0, // 0ms for tx-delay (any value)
                                   powerLevel, pTxCallback)));
  }
  else
#endif
  {
    return (EnQueueSingleData(RfSpeed, g_nodeID, bNodeID, (uint8_t *)&cmdBuffer,
                                   sizeof(NOP_POWER_FRAME),
                                   (TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_NO_ROUTE),
                                   0, // 0ms for tx-delay (any value)
                                   powerLevel, pTxCallback));
  }
}


/*===========================   ZW_SendTestFrame   ==========================
**    Send a NOP_POWER frame to specified nodeID and powerlevel
**
**    Side effects:
**
**    cmdCompleteBuf (3 first bytes) are changed
**
**--------------------------------------------------------------------------*/
uint8_t               /*RET false if transmitter busy else true */
ZW_SendTestFrame(
  uint8_t bNodeID,    /* IN nodeID to transmit to */
  uint8_t powerLevel, /* IN powerlevel index */
  const STransmitCallback* pTxCallback) /* Call back function called when done */
{
  zensorWakeupTime = ZWAVE_SENSOR_WAKEUPTIME_NONE;
  /* Make sure power level test is always done at 9.6 */
  bNeighborDiscoverySpeed = ZWAVE_FIND_NODES_SPEED_9600;
  return (SendTestFrame(bNodeID, powerLevel, pTxCallback));
}

/*===================   FindNeighborCompleteFailCall   =======================
**    Call FindNeighborComplete with TRANSMIT_COMPLETE_OK
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
void
ZCB_FindNeighborCompleteFailCall(SSwTimer* pTimer)
{
  if (NULL != pTimer) // Method is sometimes called directly, with a null pointer
  {
    TimerStop(pTimer);
  }

  /* We handle our Tx queue stuffed problem positively - assume neighbor situation when Tx is denied */
  ZCB_FindNeighborComplete(0, TRANSMIT_COMPLETE_OK, NULL);
}


/*===================   DelayFindNeighborCompleteFailCall   ==================
**    Start timer to delay the calling of FindNeighborComplete with
**    TRANSMIT_COMPLETE_OK
**
**    Side effects:
**      Timer taken
**
**--------------------------------------------------------------------------*/
void
DelayFindNeighborCompleteFailCall(void)
{
  /* Make timer which times out as quick as possible - 10ms oneshot */
  NetWorkTimerStart(&FindNeighborCompleteDelayTimer, ZCB_FindNeighborCompleteFailCall, 10);
}



/*===========================   FindNeighbors   ============================
**    Send a NOP frame to the next node in nodeRange
**
**    Side effects:
**
**    nodeRange.frame.cmd is changed
**--------------------------------------------------------------------------*/
static void     /* RET Nothing */
FindNeighbors(
  uint8_t nextNode)  /* IN  Next node to transmit to */
{
  uint8_t max = nodeRange.frame.numMaskBytes << 3;

  if (g_findInProgress)
  {
    /* Find next bit in nodeRange */
#ifdef ZW_CONTROLLER_BRIDGE
    for (uint32_t i = nextNode + 1; i <= max; i++)  /* We are one based therefor we do want 'max' */
#else
    for (uint32_t i = nextNode; i < max; i++)  /* We are zero based therefor we do not want 'max' */
#endif
    {
      /* If we are being included then we need to kick the ReplicationReceive timeout */
      if (NEW_CTRL_RECEIVER_BIT)
      {
        StartReplicationReceiveTimer();
      }
      /*
        MIT-9: Never NOP known non-listening nodes when receiving Find Nodes In Range, controllers only.
      */
      if (DropPingTest(i))
      {
        continue;
      }
#ifdef ZW_CONTROLLER_BRIDGE
      if (ZW_NodeMaskNodeIn(nodeRange.frame.maskBytes, i))
      {
        if  (!ZW_IsVirtualNode(i) && (i != g_nodeID))
        {
          bCurrentRangeCheckNodeID = i - 1;
          /* TO#3462 fix - make sure we 'get on with it' even if frame could not be transmitted */
          static const STransmitCallback TxCallback = { .pCallback = ZCB_FindNeighborComplete, .Context = 0 };
          if (!SendTestFrame(i, ZPAL_RADIO_TX_POWER_REDUCED, &TxCallback))
          {
            DelayFindNeighborCompleteFailCall();
          }
          return;
        }
        else
        {
          ZW_NodeMaskClearBit(nodeRange.frame.maskBytes, i); /* Node now removed from neighbor list */
        }
      }
#else
      if ((nodeRange.frame.maskBytes[i >> 3] & ((uint8_t)0x01 << (0x07 & i))))
      {
        bCurrentRangeCheckNodeID = i;
        /* TO#3462 fix - make sure we 'get on with it' even if frame could not be transmitted */
        static const STransmitCallback TxCallback = { .pCallback = ZCB_FindNeighborComplete, .Context = 0 };
        if (!SendTestFrame(i + 1, ZPAL_RADIO_TX_POWER_REDUCED, &TxCallback))
        {
          DelayFindNeighborCompleteFailCall();
        }
        return;
      }
#endif
    }
    /* No more bits are set in the mask, send complete frame to controller */
    g_findInProgress = false;
#ifdef ZW_CONTROLLER_STATIC
#ifdef ZW_CONTROLLER_BRIDGE
    if ((nodeRange.lastController == g_nodeID) || (nodeRange.lastController == virtualNodeID))
    {
      /* Neighbor update was requested by ourself */
      ZW_SetRoutingInfo(nodeRange.lastController, nodeRange.frame.numMaskBytes, nodeRange.frame.maskBytes);
    }
#else
    if (nodeRange.lastController == nodeID)
    {
      /* Neighbor update was requested by ourself */
      ZW_SetRoutingInfo(nodeID, nodeRange.frame.numMaskBytes, nodeRange.frame.maskBytes);
    }
#endif
    else
#endif
    {
      /* Signal to node requesting information. */
      cmdBuffer.cmdCompleteBuf.cmdClass = ZWAVE_CMD_CLASS_PROTOCOL;
      cmdBuffer.cmdCompleteBuf.cmd = ZWAVE_CMD_CMD_COMPLETE;
      cmdBuffer.cmdCompleteBuf.seqNo = 0;
#ifdef ZW_CONTROLLER_BRIDGE
      if (virtualNodeID != 0)
      {
        assignSlaveState = ASSIGN_INFO_PENDING;
        static const STransmitCallback TxCallback = { .pCallback = ZCB_AssignSlaveComplete, .Context = 0 };
        if (!EnQueueSingleData(oldSpeed,
                               virtualNodeID, nodeRange.lastController,
                               (uint8_t *)&cmdBuffer, sizeof(COMMAND_COMPLETE_FRAME),
                               nodeRange.txOptions, 0, // 0ms for tx-delay (any value)
                               ZPAL_RADIO_TX_POWER_DEFAULT,
                               &TxCallback))
        {
          /* Could not send to other node.. Let Timeout on requesting node handle it */
          ZCB_AssignSlaveComplete(0, TRANSMIT_COMPLETE_NO_ACK, NULL);
        }
        learnSlaveMode = false; /* Now slaveLearnMode done */
      }
      else
#endif
      {
        static const STransmitCallback TxCallback = { .pCallback = ZCB_FindNeighborComplete, .Context = 0 };
        if(!EnQueueSingleData(oldSpeed,
                              g_nodeID, nodeRange.lastController,
                              (uint8_t *)&cmdBuffer, sizeof(COMMAND_COMPLETE_FRAME),
                              nodeRange.txOptions, 0, // 0ms for tx-delay (any value)
                              ZPAL_RADIO_TX_POWER_DEFAULT,
                              &TxCallback))
        {
          /* Could not send to other node.. Let Timeout on requesting node handle it */
          ZCB_FindNeighborCompleteFailCall(NULL);
        }
      }
    }
  }
}


/*=======================   UpdateNodeInformation   ==========================
**    Update protocol NodeInformation
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
static void
UpdateNodeInformation(
  NODEINFO_FRAME *pNodeInfo)
{
  ZW_nodeDeviceOptionsSet(m_pAppNodeInfo->DeviceOptionsMask);
  pNodeInfo->nodeType.generic = m_pAppNodeInfo->NodeType.generic;
  pNodeInfo->nodeType.specific = m_pAppNodeInfo->NodeType.specific;
  const SCommandClassList_t* pCCList = CCListGet(SECURITY_KEY_NONE);
  blparmLength = pCCList->iListLength;
  plnodeParm = pCCList->pCommandClasses;
}

uint8_t
GenerateNodeInformation(
  NODEINFO_FRAME *pNodeInfo,
  uint8_t cmdClass)
{
#ifdef ZW_CONTROLLER_TEST_LIB
  /* Clear buffer */
  memset((uint8_t*)pNodeInfo, 0, sizeof(NODEINFO_FRAME));
  UpdateNodeInformation(pNodeInfo);
  pNodeInfo->cmdClass = cmdClass;
  pNodeInfo->cmd =(cmdClass == ZWAVE_CMD_CLASS_PROTOCOL_LR)?ZWAVE_LR_CMD_NODE_INFO :  ZWAVE_CMD_NODE_INFO; //NOSONAR
  pNodeInfo->capability = 0;
  /* Are we in a Region with a long range channel, if so add 100KLR in reserved */
  pNodeInfo->reserved = (zpal_radio_protocol_mode_supports_long_range(zpal_radio_get_protocol_mode()) ? ZWAVE_NODEINFO_BAUD_100KLR : 0);
  if (0 != ZW_nodeIsListening()) {
    pNodeInfo->capability |= ZWAVE_NODEINFO_LISTENING_MASK;  /* always listening node */
  }
  // When sending LR, since we add a byte for the number of CCs, we subtract one from the max number of CCs.
  uint8_t maxNumberOfCCs = (cmdClass == ZWAVE_CMD_CLASS_PROTOCOL_LR)? NODEPARM_MAX-1:NODEPARM_MAX;
  if (blparmLength > maxNumberOfCCs) {
    blparmLength = maxNumberOfCCs;  /* number of parameter limit exceeded */
  }

  if (cmdClass == ZWAVE_CMD_CLASS_PROTOCOL_LR) {
    NODEINFO_SLAVE_FRAME * pnodeInfo = (NODEINFO_SLAVE_FRAME*)pNodeInfo;
    /* Are we in a Region with a long range channel */
    pnodeInfo->nodeType = m_pAppNodeInfo->NodeType;
    uint8_t * pCC = &pnodeInfo->nodeInfo[0];
    /*
     * Transmitting as LR
     *
     * Make pCount point to the first byte so that we can update it after processing the list of
     * command classes and make room for one byte being number of command classes by increasing pCC.
     */
    uint8_t *pCount = pCC++;
    /*
     * Copy the list of command classes without CC S0 and CC ZWAVE_PLUS_INFO if we're included as LR.
     * CC S0 is not supported by LR.
     * CC ZWAVE_PLUS_INFO will result in inclusion failure in old controller (the inclusion controller will send malformated frames)
     */
    uint8_t sourceArrayIterator;
    uint8_t destinationArrayIterator = 0;
    for (sourceArrayIterator = 0; sourceArrayIterator < blparmLength; sourceArrayIterator++)
    {
      uint8_t cc = *(plnodeParm + sourceArrayIterator);
      if ((COMMAND_CLASS_SECURITY == cc) || (COMMAND_CLASS_ZWAVEPLUS_INFO == cc)) {
        continue;
      }
      *(pCC + destinationArrayIterator++) = cc;
    }
    // Update the command class count and increase destinationArrayIterator to include that byte.
    *pCount = destinationArrayIterator++;
    // Return the final length of the NIF.
    blnodeInfoLength = (sizeof(NODEINFO_SLAVE_FRAME) - NODEPARM_MAX + destinationArrayIterator);
  }
  else {
    /* get Node info and current status from appl. layer */
//    UpdateNodeInformation(pNodeInfo);
    pNodeInfo->nodeType.basic = BASIC_TYPE_STATIC_CONTROLLER;
    /* fill Node Information structure */
    /* Set Controller bit in security */
    pNodeInfo->security = ZWAVE_NODEINFO_CONTROLLER_NODE;
    /*
     * Node information to be transmitted on node LR channel
     *
     * ZWAVE_NODEINFO_VERSION_4 always set for none LR node information
     * ZWAVE_NODEINFO_BAUD_40000 always set for none LR node information
     * ZWAVE_NODEINFO_ROUTING_SUPPORT always set for none LR node information
     */
    pNodeInfo->capability |= ZWAVE_NODEINFO_VERSION_4 | ZWAVE_NODEINFO_BAUD_40000 | ZWAVE_NODEINFO_ROUTING_SUPPORT;

    /*
     * DT:00:11:0002.1 (SDS14224 Z-Wave Plus v2 Device Type Specification):
     *   "A Z-Wave Plus v2 node MUST set the Optional Functionality
     *    bit to 1 in its NIF"
     * Hence we simply hardcode the ZWAVE_NODEINFO_OPTIONAL_FUNC bit below.
     *
     * ZWAVE_NODEINFO_BEAM_CAPABILITY always set for none LR node information
     * ZWAVE_NODEINFO_SPECIFIC_DEVICE_TYPE always set for none LR node information
     */
    pNodeInfo->security |= ZWAVE_NODEINFO_OPTIONAL_FUNC | ZWAVE_NODEINFO_SPECIFIC_DEVICE_TYPE | ZWAVE_NODEINFO_BEAM_CAPABILITY;
    pNodeInfo->reserved |= ZWAVE_NODEINFO_BAUD_100K;
    /* calculate actual info length */
    blnodeInfoLength = sizeof(NODEINFO_FRAME) - NODEPARM_MAX + blparmLength;
    /* copy node application parameters */
    memcpy(pNodeInfo->nodeInfo, plnodeParm, blparmLength);
  }
#else
  /* Clear buffer */
  /* TO#2381 Fix - Clear full nodeinfo buffer */
  memset((uint8_t*)pNodeInfo, 0, sizeof(NODEINFO_FRAME));
  /* get Node info and current status from appl. layer */
  UpdateNodeInformation(pNodeInfo);

  pNodeInfo->nodeType.basic = BASIC_TYPE_STATIC_CONTROLLER;

  /* fill Node Information structure */
  pNodeInfo->cmdClass = cmdClass;
  pNodeInfo->cmd = (cmdClass == ZWAVE_CMD_CLASS_PROTOCOL) ? ZWAVE_CMD_NODE_INFO : ZWAVE_LR_CMD_NODE_INFO; //NOSONAR

  pNodeInfo->capability = 0;
  /* Set Controller bit in security */

  pNodeInfo->security = ZWAVE_NODEINFO_CONTROLLER_NODE;

  /* Are we in a Region with a long range channel, if so add 100KLR in reserved */
  pNodeInfo->reserved = (zpal_radio_protocol_mode_supports_long_range(zpal_radio_get_protocol_mode()) ? ZWAVE_NODEINFO_BAUD_100KLR : 0);

  if (cmdClass == ZWAVE_CMD_CLASS_PROTOCOL)
  {
    /*
     * Node information to be transmitted on node LR channel
     *
     * ZWAVE_NODEINFO_VERSION_4 always set for none LR node information
     * ZWAVE_NODEINFO_BAUD_40000 always set for none LR node information
     * ZWAVE_NODEINFO_ROUTING_SUPPORT always set for none LR node information
     */
    pNodeInfo->capability |= ZWAVE_NODEINFO_VERSION_4 | ZWAVE_NODEINFO_BAUD_40000 | ZWAVE_NODEINFO_ROUTING_SUPPORT;

    /*
     * DT:00:11:0002.1 (SDS14224 Z-Wave Plus v2 Device Type Specification):
     *   "A Z-Wave Plus v2 node MUST set the Optional Functionality
     *    bit to 1 in its NIF"
     * Hence we simply hardcode the ZWAVE_NODEINFO_OPTIONAL_FUNC bit below.
     *
     * ZWAVE_NODEINFO_BEAM_CAPABILITY always set for none LR node information
     * ZWAVE_NODEINFO_SPECIFIC_DEVICE_TYPE always set for none LR node information
     */
    pNodeInfo->security |= ZWAVE_NODEINFO_OPTIONAL_FUNC | ZWAVE_NODEINFO_SPECIFIC_DEVICE_TYPE | ZWAVE_NODEINFO_BEAM_CAPABILITY;

    pNodeInfo->reserved |= ZWAVE_NODEINFO_BAUD_100K;
  }

  if (0 != ZW_nodeIsListening())
  {
    pNodeInfo->capability |= ZWAVE_NODEINFO_LISTENING_MASK;  /* always listening node */
  }

  if (blparmLength > NODEPARM_MAX)
  {
    blparmLength = NODEPARM_MAX;  /* number of parameter limit exceeded */
  }
  /* calculate actual info length */
  blnodeInfoLength = sizeof(NODEINFO_FRAME) - NODEPARM_MAX + blparmLength;
  /* copy node application parameters */
  memcpy(pNodeInfo->nodeInfo, plnodeParm, blparmLength);
#endif
  return blnodeInfoLength;
}


/*======================   SetupNodeInformation   =========================
**    Setup nodeinformation for this node in the newNodeInfoBuffer.nodeInfoFrame structure
**
**    Side effects:
**      newNodeInfoBuffer.nodeInfoFrame structure changed
**
**--------------------------------------------------------------------------*/
void
SetupNodeInformation(uint8_t cmdClass)
{
  GenerateNodeInformation(&newNodeInfoBuffer.nodeInfoFrame, cmdClass);
}

/*===========================   SendNodeInformation   =======================
**    Create and transmit a node informations broadcast frame
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
uint8_t                    /*RET  false if transmitter queue overflow*/
ZW_SendNodeInformation(
  node_id_t    destNode,   /*IN  Destination Node ID  */
  TxOptions_t  txOptions,  /*IN  Transmit option flags         */
  const STransmitCallback* pTxCallback) /*IN  Transmit completed call back function  */
{
#ifdef ZW_CONTROLLER_TEST_LIB
  if ((LOWEST_LONG_RANGE_NODE_ID <= destNode) ||
      ((LOWEST_LONG_RANGE_NODE_ID <= g_nodeID) && (NODE_CONTROLLER == destNode))) {
    SetupNodeInformation(LOWEST_LONG_RANGE_NODE_ID <= g_nodeID ? ZWAVE_CMD_CLASS_PROTOCOL_LR : ZWAVE_CMD_CLASS_PROTOCOL);
  }
#else
  SetupNodeInformation(LOWEST_LONG_RANGE_NODE_ID <= destNode ? ZWAVE_CMD_CLASS_PROTOCOL_LR : ZWAVE_CMD_CLASS_PROTOCOL);
#endif
  if (spoof && (NEW_CTRL_RECEIVER_BIT)) /* If we are in replication receive and in spoof mode then SPOOF */
  {
    g_nodeID = NODE_CONTROLLER_OLD;
  }
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

  /* transmit Node Info frame */
  /* TO#1964 partial possible fix. */
  if (forceUse40K)
  {
    forceUse40K = false;
    return(EnQueueSingleData(RF_SPEED_40K, g_nodeID, destNode, (uint8_t *)&newNodeInfoBuffer.nodeInfoFrame, blnodeInfoLength,
                                  txOptions, 0, // 0ms for tx-delay (any value)
                                  destNode == NODE_BROADCAST ? ZPAL_RADIO_TX_POWER_REDUCED : ZPAL_RADIO_TX_POWER_DEFAULT,
                                  pTxCallback));
  }
  else

  {
    return(EnQueueSingleData(false, g_nodeID, destNode, (uint8_t *)&newNodeInfoBuffer.nodeInfoFrame, blnodeInfoLength,
                                txOptions, 0, // 0ms for tx-delay (any value)
                                destNode == NODE_BROADCAST ? ZPAL_RADIO_TX_POWER_REDUCED : ZPAL_RADIO_TX_POWER_DEFAULT,
                                pTxCallback));
  }

}


/*==============================   CheckRemovedNode   ========================
**    Check if a removed node was SUC and update if so
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
static void
CheckRemovedNode(
  uint8_t bNodeID)
{
  if (bNodeID == staticControllerNodeID)
  {
    SetStaticControllerNodeId(0x00);
    SetNodeIDServerPresent(false);
    SaveControllerConfig();
  }
}

/*=====================   SetupNewNodeRegisteredDeleted   ====================
**    Setup assignIdBuf to contain a newregistered frame for a removed node
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
static void
SetupNewNodeRegisteredDeleted(void)
{
  assignIdBuf.newNodeInfo.cmdClass = ZWAVE_CMD_CLASS_PROTOCOL;
  assignIdBuf.newNodeInfo.cmd = ZWAVE_CMD_NEW_NODE_REGISTERED;
  memset(&assignIdBuf.newNodeInfo.capability, 0, offsetof(NEW_NODE_REGISTERED_FRAME, nodeInfo) - offsetof(NEW_NODE_REGISTERED_FRAME, capability) + 1);
}

#ifdef ZW_CONTROLLER_BRIDGE
/*=================   GenerateVirtualSlaveNodeInformation   ==================
**    Generate virtual slave nodeinformation in the nodeInfo structure
**
**    Side effects:
**      nodeInfo structure updated
**
**--------------------------------------------------------------------------*/
static void
GenerateVirtualSlaveNodeInformation(
  uint8_t sourceNode)
{
  const uint8_t* plslavenodeParm;
  /* Clear buffer */
  /* TO#2381 Fix - Clear full nodeinfo buffer */
  memset((uint8_t*)&newNodeInfoBuffer.nodeInfoFrame, 0, sizeof(NODEINFO_FRAME));

  /* get Node info and current status from appl. layer for virtual slave node */
  // Find requested node in virtual node info table
  const SVirtualSlaveNodeInfo_t * pVirtualNodeInfo = NULL;
  if (NULL != m_pVirtualSlaveInfoTable) // Ensure a table was provided
  {
    for (uint32_t i = 0; i < m_pVirtualSlaveInfoTable->iTableLength; i++)
    {
      if (NULL != m_pVirtualSlaveInfoTable->ppNodeInfo[i])  // Skip inactive table entries
      {
        if (m_pVirtualSlaveInfoTable->ppNodeInfo[i]->NodeId == sourceNode)
        {
          pVirtualNodeInfo = m_pVirtualSlaveInfoTable->ppNodeInfo[i];
          break;
        }
      }
    }
    if ((NULL == pVirtualNodeInfo) && (NULL != m_pVirtualSlaveInfoTable->ppNodeInfo[0]))
    {
      pVirtualNodeInfo = m_pVirtualSlaveInfoTable->ppNodeInfo[0];
    }
  }

  if (NULL != pVirtualNodeInfo)
  {
    // Node was found in table - copy info

    // Ensure blslavedeviceOptionsMask is either 0 or APPLICATION_NODEINFO_LISTENING
    blslavedeviceOptionsMask = pVirtualNodeInfo->bListening;
    if (blslavedeviceOptionsMask)
    {
      blslavedeviceOptionsMask = APPLICATION_NODEINFO_LISTENING;
    }

    newNodeInfoBuffer.nodeInfoFrame.nodeType.generic = pVirtualNodeInfo->NodeType.generic;
    newNodeInfoBuffer.nodeInfoFrame.nodeType.specific = pVirtualNodeInfo->NodeType.specific;
    plslavenodeParm = pVirtualNodeInfo->CommandClasses.pCommandClasses;
    blslaveparmLength = pVirtualNodeInfo->CommandClasses.iListLength;
  }
  else
  {
    // Virtual node info not available

    // Node info was memset to zero - ensure command class list is harmless
    plslavenodeParm = NULL;
    blslaveparmLength = 0;

    DPRINTF(
      "Warning: Virtual slave node info requested for unavailable node ID %d\r\n",
      sourceNode
    );
  }
  newNodeInfoBuffer.nodeInfoFrame.nodeType.basic = BASIC_TYPE_END_NODE;
  /* No routing (ZWAVE_ROUTING_SUPPORT) capabilities -> virtual slave node cannot be used as a repeater */
/* TO#2445 fix - Controller should be able to transmit Explore frames to the virtual nodes */

  newNodeInfoBuffer.nodeInfoFrame.capability = (ZWAVE_NODEINFO_BAUD_40000 | ZWAVE_NODEINFO_VERSION_4);

  if ((blslavedeviceOptionsMask & APPLICATION_NODEINFO_LISTENING))
  {
    newNodeInfoBuffer.nodeInfoFrame.capability |= ZWAVE_NODEINFO_LISTENING_MASK;  /* always listening node */
  }
  newNodeInfoBuffer.nodeInfoFrame.security = ZWAVE_NODEINFO_OPTIONAL_FUNC | ZWAVE_NODEINFO_SPECIFIC_DEVICE_TYPE;

  /* Are we in a Region with a long range channel */
  newNodeInfoBuffer.nodeInfoFrame.reserved = ZWAVE_NODEINFO_BAUD_100K | (zpal_radio_protocol_mode_supports_long_range(zpal_radio_get_protocol_mode()) ? ZWAVE_NODEINFO_BAUD_100KLR : 0);

  if (blslaveparmLength > NODEPARM_MAX)
  {
    blslaveparmLength = NODEPARM_MAX;  /* number of parameter limit exceeded */
  }
  /* calculate actual info length */
  blslavenodeInfoLength = sizeof(NODEINFO_FRAME) - NODEPARM_MAX + blslaveparmLength;
  /* copy virtual slave node application parameters */
  if (plslavenodeParm) {
    memcpy(&(*((NODEINFO_FRAME*)&newNodeInfoBuffer.nodeInfoFrame)).nodeInfo, (uint8_t *)plslavenodeParm, blslaveparmLength);
  }
}


/*=======================   ZW_SendSlaveNodeInformation   ====================
**    Create and transmit a virtual slave node information frame
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
uint8_t                                    /*RET true if put in transmit queue     */
ZW_SendSlaveNodeInformation(
  node_id_t   sourceNode,                  /* IN Which virtual node is to transmit the nodeinfo */
  node_id_t   destNode,                    /* IN Destination Node ID  */
  TxOptions_t txOptions,                   /* IN Transmit option flags */
  const STransmitCallback* pCompletedFunc) /* IN Transmit completed call back function  */
{
  if ((!sourceNode || ZW_IsVirtualNode(sourceNode)))
  {
    /* TO#5964 fix - Remember to enable Explore frame resort if TRANSMIT_OPTION_EXPLORE present */
    /* Do we want to use the Explore Frame as a route resolution if all else fails */
    if (txOptions & TRANSMIT_OPTION_EXPLORE)
    {
      /* Note that we want Explore tried as route resolution if all else fails  */
      bUseExploreAsRouteResolution = true;
    }

    /* fill Node Information structure */
    GenerateVirtualSlaveNodeInformation(sourceNode);
    uint8_t len = blslavenodeInfoLength;
    memmove(&newNodeInfoBuffer.nodeInfoFrame.nodeType.basic,
            &newNodeInfoBuffer.nodeInfoFrame.nodeType.generic,
            len-- - offsetof(NEW_NODE_REGISTERED_FRAME, nodeType));
    newNodeInfoBuffer.nodeInfoFrame.cmdClass = ZWAVE_CMD_CLASS_PROTOCOL;
    newNodeInfoBuffer.nodeInfoFrame.cmd = ZWAVE_CMD_NODE_INFO;
    /* transmit Node Info frame*/
    /* After obsoleeting ZW_SendSlaveData we need to let bSrcNodeID == 0 through as this is needed when transmitting */
    /* the Nodeinformation for a reset Virtual Node - This we do by NOT using ZW_SendData_Bridge but use the internal */
    /* EnQueueSingleData */
    txOptions &= TRANSMIT_APPLICATION_OPTION_MASK;
    return (EnQueueSingleData(false, sourceNode, destNode, (uint8_t *)&newNodeInfoBuffer.nodeInfoFrame, len,
                                   txOptions, 0, // 0ms for tx-delay (any value)
                                   destNode == NODE_BROADCAST ? ZPAL_RADIO_TX_POWER_REDUCED : ZPAL_RADIO_TX_POWER_DEFAULT,
                                   pCompletedFunc));
  }
  return false;
}


/*============================   AssignSlaveTimeout   =======================
**    Function description
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
void
ZCB_AssignSlaveTimeout(SSwTimer* pTimer)
{
  TimerStop(pTimer);
  if (assignSlaveState == ASSIGN_INFO_PENDING)
  {
    assignSlaveState = ASSIGN_COMPLETE;
    NetWorkTimerStart(pTimer, ZCB_AssignSlaveTimeout, 10);
    ZCB_AssignSlaveComplete(0, TRANSMIT_COMPLETE_OK, NULL);
  }

}


/*============================   AssignSlaveTimerStop   ======================
**    Function description
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
static void
AssignSlaveTimerStop(void)
{
  TimerStop(&assignSlaveTimer);
}


/*============================   AssignSlaveTimerStart   ======================
**    Function description
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
static void
AssignSlaveTimerStart(void)
{
  NetWorkTimerStart(&assignSlaveTimer, ZCB_AssignSlaveTimeout, 200);
}


/*============================   AssignSlaveComplete   =========================
**    Function description
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
static void
ZCB_AssignSlaveComplete(
  __attribute__((unused)) ZW_Void_Function_t Context,
  __attribute__((unused)) uint8_t bStatus,
  __attribute__((unused)) TX_STATUS_TYPE *txStatusReport)
{
  AssignSlaveTimerStop(); /* Stop any timer that might be running. We will restart here if needed */
  if (assignSlaveState == ASSIGN_INFO_PENDING)
  {
    /*Start a timeout to ensure that we exit if controller fails to respond in due time*/
    AssignSlaveTimerStart();
  }
  else
  {
    if ((assignSlaveState == ASSIGN_NODEID_DONE) &&
        (learnSlaveMode != VIRTUAL_SLAVE_LEARN_MODE_ADD))
    {
      /* nodeID have been assigned. No need to accept further ID assignment */
      learnSlaveMode = false; /* Now slaveLearnMode done */
      /* If we got a nodeID then give controller time to initiate further action */
    }
    /* Tell Application when assignState != ASSIGN_INFO_PENDING */
    if (assignSlaveState == ASSIGN_COMPLETE)
    {
      learnSlaveMode = false;
    }
    /* ASSIGN_NODEID_DONE, ASSIGN_RANGE_INFO_UPDATE and ASSIGN_COMPLETE are reported to appl. */
    if (learnSlaveNodeFunc)
    {
      learnSlaveNodeFunc(assignSlaveState, virtualNodeID, newID);
    }
    /* a new NodeID was assigned */
    if (assignSlaveState == ASSIGN_NODEID_DONE)
    {
      if (!newID)
      {
        assignSlaveState = ASSIGN_COMPLETE; /* Node reset, then we are done */
        learnSlaveMode = false;
      }
      else
      {
        if (learnSlaveMode == VIRTUAL_SLAVE_LEARN_MODE_ADD)
        {
          assignSlaveState = ASSIGN_COMPLETE; /* Node created, then we are done */
          learnSlaveMode = false;
        }
        else
        {
          assignSlaveState = ASSIGN_INFO_PENDING; /* We wait for findNeighbors request */
          AssignSlaveTimerStart();  /* start timer to make sure we get out */
        }
      }
    }
  }
}


/*============================   RemoveVirtualNode   =========================
**    Function description
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
static void
RemoveVirtualNode(
  uint8_t node)
{
  ZW_NodeMaskClearBit(nodePool, node); /* Node now removed */
  CtrlStorageSetBridgeNodeFlag(node, false, true);
}

/*==========================   ReportNewVirtualNode   ========================
**    Function description
**      Report to SUC/SIS that a new node has been added, if needed.
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
static uint8_t
ReportNewVirtualNode(void)
{
  newID = assign_ID.newNodeID;
  if (primaryController && staticControllerNodeID &&
      (staticControllerNodeID != virtualNodeID) &&
      ZW_IS_NOT_SUC) /* Only if SUC are not ME... */
  {
    assignIdBuf.newNodeInfo.cmdClass = ZWAVE_CMD_CLASS_PROTOCOL;
    assignIdBuf.newNodeInfo.cmd = ZWAVE_CMD_NEW_NODE_REGISTERED;
    assignIdBuf.newNodeInfo.nodeID = (!virtualNodeID) ? newID : virtualNodeID;
    uint8_t len = assign_ID.nodeInfoLen;
    if (learnSlaveMode != VIRTUAL_SLAVE_LEARN_MODE_REMOVE)
    {
      memcpy((uint8_t *)&assignIdBuf.newNodeInfo.capability,
             (uint8_t *)&newNodeInfoBuffer.nodeInfoFrame.capability,
             (offsetof(NODEINFO_FRAME, nodeType) - offsetof(NODEINFO_FRAME, capability)));
      /* When we transmit slave nodeinfo the Basic Device Type is not transmitted... */
      memcpy(&assignIdBuf.newNodeInfo.nodeType.basic,
             &newNodeInfoBuffer.nodeInfoFrame.nodeType.generic,
             len-- - offsetof(NODEINFO_FRAME, nodeType));
    }
    else
    {
      /* Node was deleted */
      SetupNewNodeRegisteredDeleted();
    }
    /* Do we allready have a nodeInfo cached */
    if (pendingTableEmpty)
    {
      /* Pending is empty. */
      bNewRegisteredNodeFrameSize = offsetof(NEW_NODE_REGISTERED_FRAME, nodeInfo) +
                                      (len - offsetof(NODEINFO_FRAME, nodeInfo));
      /* Copy the generated frame */
      memcpy((uint8_t*)&sNewRegisteredNodeFrame.cmdClass, (uint8_t*)&assignIdBuf, bNewRegisteredNodeFrameSize);
    }
    SetPendingUpdate(assignIdBuf.newNodeInfo.nodeID);
  }
  return false;
}


/*===========================   LearnSlaveCallBack   ============================
**
**  Callback function called when Adding virtual slave node internally as a
**  primary/inclusion controller
**
**
**-----------------------------------------------------------------------------*/

void                   /*RET Nothing */
ZCB_LearnSlaveCallBack(
  LEARN_INFO_T *learnNodeInfo)  /* IN Learn status and information */
{
  if ((learnNodeInfo->bStatus == LEARN_STATE_FAIL) ||
      (learnNodeInfo->bStatus == LEARN_STATE_NO_SERVER))
  {
    assignSlaveState = ASSIGN_NODEID_DONE;
    ZW_AddNodeToNetwork(ADD_NODE_STOP, NULL);
    newID = 0;
  }
  ZCB_AssignSlaveComplete(0, TRANSMIT_COMPLETE_OK, NULL);
}


/*==========================   ZW_SetSlaveLearnMode   ========================
**    Enable/Disable home/node ID learn mode for virtual nodes.
**    When learn mode is enabled, received "Assign ID's Command" for the current virtual node 'node' are handled:
**    If 'node' is zero, the received nodeID will be stored as a new virtual node.
**    If the received nodeID is zero the virtual node 'node' will be removed.
**
**    The learnFunc is called when the received assign command has been handled.
**    The returned parameters are the virtual node 'node' and the learned Node ID.
**
**-----------------------------------------------------------------------------*/
uint8_t                  /*RET Returns true if successful or false if node invalid or controller is primary */
ZW_SetSlaveLearnMode(
  node_id_t nodeID,      /* IN nodeID on virtual node to set in Learn Mode - for adding a virtual node then this must be ZERO */
  uint8_t mode,          /* IN true  Enable, false  Disable or SLAVE_LEARN_REMOVE virtual node is to be removed */
  VOID_CALLBACKFUNC(learnFunc)(uint8_t bStatus, node_id_t orgID, node_id_t newID)) /* IN Slave node learn call back function. */
{
  if (!nodeID || ZW_IsVirtualNode(nodeID))
  {
    /* Primary and inclusion controllers can add/remove Virtual Slave Nodes internally */
    if (primaryController)
    {
      if (mode == VIRTUAL_SLAVE_LEARN_MODE_ADD)
      {
        /* node must be ZERO! else a Virtual Slave Node allready exist with this nodeID */
        if (!nodeID && AreSUCAndAssignIdle())
        {
          /* Here we start determining if we have a nodeID to spare */
          learnSlaveMode = VIRTUAL_SLAVE_LEARN_MODE_ADD;
          learnSlaveNodeFunc = learnFunc;
          ZW_AddNodeToNetwork(ADD_NODE_SLAVE, ZCB_LearnSlaveCallBack);
          return true;
        }
        /* If user used a none ZERO nodeID then we fail */
      }
      else if (mode == VIRTUAL_SLAVE_LEARN_MODE_REMOVE)
      {
        if (nodeID && AreSUCAndAssignIdle())
        {
          learnSlaveMode = VIRTUAL_SLAVE_LEARN_MODE_REMOVE;
          learnSlaveNodeFunc = learnFunc;
          assignSlaveState = ASSIGN_NODEID_DONE;
          virtualNodeID = nodeID;
          /* Remove the Virtual Slave Node locally */
          RemoveVirtualNode(virtualNodeID);
          AddNodeInfo(virtualNodeID, NULL, false);
          assign_ID.newNodeID = 0;
          assign_ID.nodeInfoLen = sizeof(NODEINFO_FRAME) - NODEPARM_MAX;
          assign_ID.assignIdState = ASSIGN_SEND_NODE_INFO_DELETE;
          /* Report action to SUC/SIS if applicable */
          if (!ReportNewVirtualNode())
          {
            ZCB_AssignSlaveComplete(0, TRANSMIT_COMPLETE_OK, NULL);
            assign_ID.assignIdState = ASSIGN_IDLE;
            virtualNodeID = 0;
            /* Force pending update */
            pendingTimerReload = 1;
          }
          return true;
        }
        /* Not applicable to remove a node that is not there (ZERO) */
      }
    }
    /* Only if inclusion controller or secondary controller is it applicable */
    /* to enter SlaveLearnMode and thereby emulate a slave fully - because */
    /* an external controller must do the inclusion or exclusion */
    if (((!primaryController) || (mode == VIRTUAL_SLAVE_LEARN_MODE_DISABLE)) || (isNodeIDServerPresent()))
    {
      /* If mode is either VIRTUAL_SLAVE_LEARN_MODE_DISABLE or VIRTUAL_SLAVE_LEARN_MODE_ENABLE */
      if (mode < VIRTUAL_SLAVE_LEARN_MODE_ADD)
      {
        if (assignSlaveState == ASSIGN_COMPLETE)
        {
          virtualNodeID = nodeID;   /* We are now working on this node */
          learnSlaveMode = mode;
          if (learnSlaveMode)
          {
            learnSlaveNodeFunc = learnFunc;
          }
          return true;
        }
        else
        {
          /* assignState not ASSIGN_COMPLETE. Let AssignSlaveComplete decide what to do */
          ZCB_AssignSlaveComplete(0, TRANSMIT_COMPLETE_OK, NULL);
        }
      }
    }
  }
  return false;
}
#endif /* ZW_CONTROLLER_BRIDGE */

/*===========================   ZW_SetLearnMode   ============================
**    Enable/Disable home/node ID learn mode.
**    When learn mode is enabled, received "Assign ID's Command" are handled:
**    If the current stored ID's are zero, the received ID's will be stored.
**    If the received ID's are zero the stored ID's will be set to zero.
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
void
ZW_SetLearnMode( /*RET  Nothing        */
  uint8_t mode,                                      /* IN Learnmode bitmask */
  VOID_CALLBACKFUNC(completedFunc)(LEARN_INFO_T*))  /* IN Callback function */
{
  DPRINT("ZW_SetLearnMode\r\n");
  DPRINTF("mode = %02x\r\n",mode);

  if (ZW_SET_LEARN_MODE_NWE < mode)
  {
    mode = ZW_SET_LEARN_MODE_DISABLE;
  }
  InternSetLearnMode(mode, completedFunc);
}

void
InternSetLearnMode(
  uint8_t mode,                                  /* IN learnMode bitmask */
  learn_mode_callback_t completedFunc)  /* IN Callback function */

{
#ifdef ZW_ROUTED_DISCOVERY
  updateNodeNeighbors = false;  /* Just make sure */
#endif
  g_learnMode = mode;
  g_learnModeClassic = (mode == ZW_SET_LEARN_MODE_CLASSIC);
  g_learnModeDsk = (mode == ZW_SET_LEARN_MODE_SMARTSTART);

  if (spoof) /* If spoof then deSPOOF */
  {
    spoof = false;
    ControllerStorageGetNetworkIds(ZW_HomeIDGet(), &g_nodeID);
  }

  /* Start learn/replication */
  if (mode)
  {
    ReplicationInit(RECEIVE, completedFunc);

    /*  if not classic, the set NWI parameters */
    if (!g_learnModeClassic)
    {
      switch (mode)
      {
        case ZW_SET_LEARN_MODE_SMARTSTART:
        {
          NetworkManagementGenerateDskpart();
          NetworkManagement_IsSmartStartInclusionSet();
          bNetworkWideInclusion = NETWORK_WIDE_MODE_JOIN;
#ifdef ZW_REPEATER
          exploreInclusionModeRepeat = false;
#endif
        }
        break;

        case ZW_SET_LEARN_MODE_NWI:
          {
            bNetworkWideInclusion = NETWORK_WIDE_MODE_JOIN;
#ifdef ZW_REPEATER
            exploreInclusionModeRepeat = false;
#endif
          }
          break;

        case ZW_SET_LEARN_MODE_NWE:
          {
            bNetworkWideInclusion = NETWORK_WIDE_MODE_LEAVE;
#ifdef ZW_REPEATER
            exploreInclusionModeRepeat = false;
#endif
          }
          break;

        default:
        {
          /* Unknown mode -> learnMode = disabled */
          g_learnMode = 0;
          g_learnModeDsk = 0;
          goto zw_setlearnmode_abort;
        }
      }
    }
    else
    {
      // Classic
      NetworkManagement_IsSmartStartInclusionClear();  // Not SmartStart
      bNetworkWideInclusion = NETWORK_WIDE_MODE_IDLE;
#ifdef ZW_REPEATER
      exploreInclusionModeRepeat = false;
#endif
    }
  }
  else
  {
zw_setlearnmode_abort:
    if ((bNetworkWideInclusion == NETWORK_WIDE_MODE_JOIN) ||
        (bNetworkWideInclusion == NETWORK_WIDE_MODE_LEAVE))
    {
      bNetworkWideInclusion = NETWORK_WIDE_MODE_IDLE;
    }

    /* TO#1964 possible fix - If we are stopping, then lets release findInProgress JIC */
    g_findInProgress = false;
    ReplicationInit(STOP, NULL);
  }
}


/*==========================   LearnCheckIfAllowed   ========================
**    Description:
**
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
static bool
LearnCheckIfAllowed(
  uint8_t bMode)
{
  if (primaryController && ((bMode == NEW_NODE) || (bMode == DELETE_NODE)))
  {
    return true;
  }

#ifdef ZW_CONTROLLER_STATIC
  if ((bMode == NEW_PRIMARY) && (ZW_IS_SUC) &&
      (!primaryController) && (!isNodeIDServerPresent()))
  {
    return true;
  }
  if ((bMode == CTRL_CHANGE) && primaryController)
  {
    return true;
  }
#else
  if ((bMode == CTRL_CHANGE) && primaryController && !isNodeIDServerPresent())
  {
    return true;
  }
#endif

  /* Learn failed */
  LearnInfoCallBack(((bMode == NEW_PRIMARY) || (bMode == CTRL_CHANGE)) ? ADD_NODE_STATUS_FAILED : LEARN_STATE_FAIL, 0, 0);

  return false;
}


bool
AddNodeToNetworkCheckIfAllowed(
  uint8_t bMode)
{
  switch (bMode & ADD_NODE_MODE_MASK)
  {
    // Drop through to "case ADD_NODE_RESERVED"
    case ADD_NODE_ANY:
    case ADD_NODE_CONTROLLER:
    case ADD_NODE_SLAVE:
    case ADD_NODE_EXISTING:
    case ADD_NODE_RESERVED:
      if (!LearnCheckIfAllowed(NEW_NODE))
      {
        return false;
      }
      break;

    // Drop through to "case ADD_NODE_SMART_START"
    case ADD_NODE_HOME_ID:
    case ADD_NODE_SMART_START:
      if ((isNodeIDServerPresent() && !ZW_IS_SUC) || !LearnCheckIfAllowed(NEW_NODE))
      {
        return false;
      }
      break;

    case ADD_NODE_STOP:
      if (PENDING_UPDATE_NODE_INFO_SENT == assign_ID.assignIdState)
      {
        return false;
      }
	  break;

    default:
      break;
  }
  // All other modes always return true;
  return true;
}


/*==========================   LearnSetNodesAllowed   ========================
**    Set addSlaveNodes and addCtrlNodes according to bMode contents
**
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
static void
LearnSetNodesAllowed(
  uint8_t bMode)
{
  addSlaveNodes = true;
  addCtrlNodes = true;
  addSmartStartNode = false;
  if (bMode == ADD_NODE_CONTROLLER)
  {
    addSlaveNodes = false;
  }
  else if (bMode == ADD_NODE_SLAVE)
  {
    addCtrlNodes = false;
  }
  else if (bMode == ADD_NODE_HOME_ID)
  {
    addSlaveNodes = false;
    addCtrlNodes = false;
    addSmartStartNode = true;
  }
}

void
ZW_AddNodeDskToNetwork(
  uint8_t bMode,
  const uint8_t *pDSK,
  VOID_CALLBACKFUNC(completedFunc)(LEARN_INFO_T*));


/*==========================   ZW_AddNodeToNetwork   ========================
**
**    Add any type of node to the network
**
**    The modes are:
**
**    ADD_NODE_ANY            Add any node to the network
**    ADD_NODE_CONTROLLER     Add a controller to the network
**    ADD_NODE_SLAVE          Add a slave node to the network
**
**    ADD_NODE_SMART_START    Listen for SMARTSTART node wanting to be included
**
**    ADD_NODE_STOP           Stop learn mode without reporting an error.
**    ADD_NODE_STOP_FAILED    Stop learn mode and report an error to the
**                            new controller.
**
**    ADD_NODE_OPTION_NORMAL_POWER    Set this flag in bMode for High Power inclusion.
**
**    ADD_NODE_OPTION_NETWORK_WIDE  Set this flag in bMode for enabling
**                                  Networkwide inclusion via explore frames
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
void
ZW_AddNodeToNetwork(
  uint8_t bMode,
  VOID_CALLBACKFUNC(completedFunc)(LEARN_INFO_T*))
{
  ZW_AddNodeDskToNetwork(bMode, NULL, completedFunc);
}

void
ZW_AddNodeDskToNetwork(
  uint8_t bMode,
  const uint8_t *pDSK,
  VOID_CALLBACKFUNC(completedFunc)(LEARN_INFO_T*))
{
  // Get the bit to see if finding neighbors will include FL nodes
  findNeighborsWithFlirs = !(bMode & ADD_NODE_OPTION_NO_FL_SEARCH);
  // Clear that bit
  bMode &= ~ADD_NODE_OPTION_NO_FL_SEARCH;

  DPRINTF("AddNodeDskToNetwork: %x\n", bMode);
  if (ADD_NODE_MAX <= (bMode & ADD_NODE_MODE_MASK))
  {
    return;
  }
  if (!AddNodeToNetworkCheckIfAllowed(bMode))
  {
    /* Stop SetNWI rearm Timer if enabled */
    NetworkManagementStopSetNWI();
    return;
  }
  DPRINT("AddNodeDskToNetwork: 1\n");
  bNetworkRemoveNodeID = 0;
  /* Extract inclusion power from bMode */
  transferPresentationHighPower = ((bMode & ADD_NODE_OPTION_NORMAL_POWER) != 0);

  if (bMode & ADD_NODE_OPTION_NETWORK_WIDE)
  {
    bNetworkWideInclusion = NETWORK_WIDE_MODE_INCLUDE;
    bNetworkWideInclusionReady = false;
  }
  else
  {
    bNetworkWideInclusion = NETWORK_WIDE_MODE_IDLE;
  }

  // Check whether the node MUST be included using LR
  addUsingLR = false;
  if ((ADD_NODE_OPTION_LR | ADD_NODE_HOME_ID) == (bMode & (ADD_NODE_OPTION_LR | ADD_NODE_HOME_ID)))
  {
    // ADD_NODE_OPTION_LR only valid together with ADD_NODE_HOME_ID
    addUsingLR = true;
  }

  bMode &= ADD_NODE_MODE_MASK;
  if (bMode == ADD_NODE_SMART_START)
  {
    /* Smart Start prime mode - wait for ZWAVE_CMD_SMARTSTART_PRIME_NODE_INFO */
    addSmartNodeHomeId[0] = 0;
    addSmartNodeHomeId[4] = 0;
    bNetworkWideInclusion = NETWORK_WIDE_SMART_START;
    g_learnNodeState = LEARN_NODE_STATE_SMART_START;
    /* Transmit Explore SetNWI and Restart timer if allready started */
    ZCB_NetworkManagementSetNWI(NULL);
    return;
  }
  if (bMode == ADD_NODE_HOME_ID)
  {
    if (NULL != pDSK)
    {
      /* Smart Start primed mode - We know who to include; wait for ZWAVE_CMD_SMARTSTART_INCLUDE_NODE_INFO */
      memcpy(addSmartNodeHomeId, pDSK, 2*HOMEID_LENGTH);
      addSmartNodeHomeId[0] |= 0xC0;
      addSmartNodeHomeId[3] &= 0xFE;
      addSmartNodeHomeId[4] |= 0xC0;
      addSmartNodeHomeId[7] &= 0xFE;
      bNetworkWideInclusion = NETWORK_WIDE_SMART_START_NWI;
      g_learnNodeState = LEARN_NODE_STATE_SMART_START;
      // No reason for starting Explore SetNWI as we are trying to include on Long Range channel
      if (false == addUsingLR)
      {
        /* Only starts Explore SetNWI timer if not allready running */
        NetworkManagementStartSetNWI();
      }
    }
    else
    {
      bMode = ADD_NODE_STOP_FAILED;
      /* Stop SetNWI rearm Timer if enabled */
      NetworkManagementStopSetNWI();
    }
  }
  else
  {
    /* Stop SetNWI rearm Timer if enabled */
    NetworkManagementStopSetNWI();
  }
  /* Handle stop modes */
  if (bMode == ADD_NODE_STOP)
  {
    /* Always send done callback if already stopped and callback function is specified */
    if (GET_NEW_CTRL_STATE == NEW_CTRL_STOP)
    {
      if (completedFunc) {
       TransferDoneCallback(NEW_CONTROLLER_DONE);
      } else {
       /* reset the learn state machine when learn process stop and
        and stops the neighbour discovery timeout timer */
       ReplicationInit(STOP, NULL);
       LearnInfoCallBack(LEARN_STATE_DONE, 0, 0);
       AssignTimerStop();
     }
    }
    else
    {
      ReplicationInit(STOP, completedFunc);
    }

    bNetworkWideInclusion = NETWORK_WIDE_MODE_IDLE;
    return;
  }
  if (bMode == ADD_NODE_STOP_FAILED)
  {
    ReplicationInit(STOP_FAILED, completedFunc);
    bNetworkWideInclusion = NETWORK_WIDE_MODE_IDLE;
    return;
  }

  if (!AreSUCAndAssignIdle())
  { /* Protocol not ready for ZW_AddNodeToNetwork */
    learnNodeInfo.bStatus = ADD_NODE_STATUS_FAILED;
    learnNodeInfo.bSource = 0;
    learnNodeInfo.pCmd = 0;
    learnNodeInfo.bLen = 0;
    if (completedFunc)
    {
      completedFunc(&learnNodeInfo);
    }
    return;
  }
  /* Set node type parameter */
  LearnSetNodesAllowed(bMode);

  if (bMode == ADD_NODE_EXISTING)
  {
    ReplicationInit(UPDATE_NODE, completedFunc);
    return;
  }
  zcbp_learnCompleteFunction = completedFunc;
  /* Check if learn is allowed in this ctrl. */
  if (!LearnCheckIfAllowed(NEW_NODE))
  {
    inclusionHomeIDActive = false;
    return;
  }
  /* Start learn/replication */
  ReplicationInit(NEW_NODE, completedFunc);
}


/*==========================   _RemoveNodeFromNetwork   ========================
**
**    Remove any type of node from the network - internal function
**
**    The modes are:
**
**    REMOVE_NODE_ANY            Remove any node from the network
**    REMOVE_NODE_CONTROLLER     Remove a controller from the network
**    REMOVE_NODE_SLAVE          Remove a slave node from the network
**
**    REMOVE_NODE_STOP           Stop learn mode without reporting an error.
**
**    REMOVE_NODE_OPTION_NORMAL_POWER   Set this flag in bMode for Normal Power
**                                      exclusion.
**    REMOVE_NODE_OPTION_NETWORK_WIDE   Set this flag in bMode for enabling
**                                      Networkwide explore via explore frames
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
void
_RemoveNodeFromNetwork(
  uint8_t bMode,
  VOID_CALLBACKFUNC(completedFunc)(LEARN_INFO_T*))
{
  /* Extract exclusion power from bMode */
  transferPresentationHighPower = ((bMode & REMOVE_NODE_OPTION_NORMAL_POWER) != 0);

  bNetworkWideInclusion = (bMode & REMOVE_NODE_OPTION_NETWORK_WIDE) ? NETWORK_WIDE_MODE_EXCLUDE : NETWORK_WIDE_MODE_IDLE;

  bMode &= REMOVE_NODE_MODE_MASK;
  /* Handle stop modes */
  if (bMode == REMOVE_NODE_STOP)
  {
    bNetworkRemoveNodeID = 0;
    ReplicationInit(STOP, completedFunc);
    return;
  }
  if (!AreSUCAndAssignIdle())
  { /* Protocol not ready */
    bNetworkRemoveNodeID = 0;
    learnNodeInfo.bStatus = REMOVE_NODE_STATUS_FAILED;
    learnNodeInfo.bSource = SUC_Update.updateState;
    learnNodeInfo.pCmd = &assign_ID.assignIdState;
    learnNodeInfo.bLen = (GET_NEW_CTRL_STATE == NEW_CTRL_STOP) ? 1 : 0;
    if (completedFunc)
    {
      completedFunc(&learnNodeInfo);
    }
    return;
  }
  zcbp_learnCompleteFunction = completedFunc;
  /* Check if learn is allowed in this ctrl. */
  if (!LearnCheckIfAllowed(DELETE_NODE))
  {
    bNetworkRemoveNodeID = 0;
    return;
  }
  /* set node type parameter */
  LearnSetNodesAllowed(bMode);
  /* Clean-up nodeInfoFrame, which has incorrect values after resetting the Controller.
     This incorrectly generates an extra frame (transfer end) when a LR node (with a
     foreign home Id) tries to exclude itself */
  memset((uint8_t*)&newNodeInfoBuffer.nodeInfoFrame, 0, sizeof(NODEINFO_FRAME));
  // No node has been removed yet
  otherController = 0;
  /* Start learn/replication */
  ReplicationInit(DELETE_NODE, completedFunc);
}


/*======================   ZW_RemoveNodeIDFromNetwork   ======================
**
**    Remove specific node ID from the network
**
**    - If valid nodeID (1-232) is specified then only the specified nodeID
**     matching the mode settings can be removed.
**    - If REMOVE_NODE_ID_ANY or none valid nodeID (0, 233-255) is specified
**     then any node which matches the mode settings can be removed.
**
**    The modes are:
**
**    REMOVE_NODE_ANY            Remove Specified nodeID (any type) from the network
**    REMOVE_NODE_CONTROLLER     Remove Specified nodeID (controller) from the network
**    REMOVE_NODE_SLAVE          Remove Specified nodeID (slave) from the network
**
**    REMOVE_NODE_STOP           Stop learn mode without reporting an error.
**
**    REMOVE_NODE_OPTION_NORMAL_POWER   Set this flag in bMode for Normal Power
**                                      exclusion.
**    REMOVE_NODE_OPTION_NETWORK_WIDE   Set this flag in bMode for enabling
**                                      Networkwide explore via explore frames
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
void
ZW_RemoveNodeIDFromNetwork(
  uint8_t bMode,
  node_id_t nodeID,
  VOID_CALLBACKFUNC(completedFunc)(LEARN_INFO_T*))
{
  if (REMOVE_NODE_MAX <= (bMode & REMOVE_NODE_MODE_MASK))
  {
    return;
  }
  if ((0 == nodeID) || (ZW_MAX_NODES < nodeID))
  {
    /* No valid specified NodeID - Remove any NodeID which matches bMode */
    bNetworkRemoveNodeID = 0;
  }
  else
  {
    /* Specific NodeID specified - Only Remove Node which matches */
    /* specified bNodeID and specified bMode */
    bNetworkRemoveNodeID = nodeID;
  }
  _RemoveNodeFromNetwork(bMode, completedFunc);
}


/*========================   ZW_ControllerChange   ======================
**
**    Transfer the role as primary controller to another controller
**
**    The modes are:
**
**    CONTROLLER_CHANGE_START          Start the creation of a new primary
**    CONTROLLER_CHANGE_STOP           Stop the creation of a new primary
**    CONTROLLER_CHANGE_STOP_FAILED    Report that the replication failed
**
**    ADD_NODE_OPTION_NORMAL_POWER       Set this flag in bMode for High Power exchange.
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
void
ZW_ControllerChange(
  uint8_t bMode,
  VOID_CALLBACKFUNC(completedFunc)(LEARN_INFO_T*))
{
  /* Only normal power is supported */
  transferPresentationHighPower = true;
  bMode &= ADD_NODE_MODE_MASK;

  /* Handle stop modes */
  if (bMode == CONTROLLER_CHANGE_STOP)
  {
    ReplicationInit(STOP, completedFunc);
    return;
  }
  if (bMode == CONTROLLER_CHANGE_STOP_FAILED)
  {
    ReplicationInit(STOP_FAILED, completedFunc);
    return;
  }
  /* Set node type parameter */
  LearnSetNodesAllowed(ADD_NODE_CONTROLLER);
  zcbp_learnCompleteFunction = completedFunc;
  /* Check if learn is allowed in this ctrl. */
  if (!LearnCheckIfAllowed(CTRL_CHANGE))
  {
    return;
  }
  /* Get a new node ID if needed */
  if (g_nodeID > ZW_MAX_NODES)  /* Have we been made primary by a pre ZWAVE_VERSION_3 controller? */
  {
    g_nodeID = GetNextFreeNode(false); /* Then we need a new nodeID */
  }

  /* Start learn/replication */
  ReplicationInit(NEW_NODE, completedFunc);
  newPrimaryReplication = true;

#ifdef ZW_CONTROLLER_STATIC
  doChangeToSecondary = true;
#endif
}

/*=========================   ZW_SetLearnNodeState   =======================
**    Set controller in "learn node mode".
**    When learn node mode is enabled, received "Node Info" frames are
**    handled and passed to the application through the call back function.
**
**    The states are:
**
**    LEARN_NODE_STATE_OFF     Stop accepting Node Info frames.
**    LEARN_NODE_STATE_NEW     Add new nodes to node list
**    LEARN_NODE_STATE_DELETE  Delete nodes from node list
**    LEARN_NODE_STATE_UPDATE  Update node-info in node list
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
void
ZW_SetLearnNodeState( /*RET Nothing        */
  uint8_t mode,          /*IN  1-New 2-Update 3-Delete 0-Disable learn mode */
  VOID_CALLBACKFUNC(completedFunc)(LEARN_INFO_T*)) /* IN Callback function */
{
/*  if(assign_ID.assignIdState != ASSIGN_IDLE)
  {
      LEARN_INFO_T learnNodeInfo;

      learnNodeInfo.bStatus = LEARN_MODE_FAILED;
      learnNodeInfo.bSource = 0;
      learnNodeInfo.pCmd = 0;
      learnNodeInfo.bLen = 0;
      completedFunc(&learnNodeInfo);
  }
  */
  DPRINTF("ZW_SetLearnNodeState R%d\n", mode);
#ifdef REPLACE_FAILED
  if (!failedNodeReplace)
  {
    /* Clear the new nodeID in the assign_ID struct */
    assign_ID.newNodeID = 0;
  }
#endif

  zcbp_learnCompleteFunction = completedFunc;

#if defined(ZW_CONTROLLER_STATIC)
  /* Use old learn mode if no server, we are SUC or we are not adding a new node */
  if ((!isNodeIDServerPresent()) || (ZW_IS_SUC) || (mode != LEARN_NODE_STATE_NEW))
  {
#ifdef ZW_CONTROLLER_BRIDGE
    if ((mode == LEARN_NODE_STATE_NEW) && (learnSlaveMode == VIRTUAL_SLAVE_LEARN_MODE_ADD))
    {
      assign_ID.newNodeID = GetNextFreeNode(false);
    }
#endif
    ZW_SetLearnNodeStateNoServer(mode);
    return;
  }
#else
  /* Use old learn mode if no server, we are SUC or we are not adding a new node */
  if ((!isNodeIDServerPresent()) || (mode != LEARN_NODE_STATE_NEW))
  {
    ZW_SetLearnNodeStateNoServer(mode);
    return;
  }
#endif

  /* We are adding a node using a nodeID server */
#ifdef REPLACE_FAILED
  if (failedNodeReplace && (assign_ID.newNodeID == staticControllerNodeID))
  {
    ZCB_RequestUpdateCallback(0, ZW_SUC_UPDATE_DONE, NULL);
  }
  else
  {
#endif
    /* Request and update from the SUC */
    static const STransmitCallback TxCallback = { .pCallback = ZCB_RequestUpdateCallback, .Context = 0 };
    if (!ZW_RequestNetWorkUpdate(&TxCallback))
    {
      ZCB_RequestUpdateCallback(0, ZW_SUC_UPDATE_ABORT, NULL);
    }
#ifdef REPLACE_FAILED
  }
#endif
}


#ifdef ZW_CONTROLLER_BRIDGE
/*=====================   SetNewVirtualNodePresent   =========================
**
**    Generate nodeinformation for Virtual node and set it present using newID
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
static void
SetNewVirtualNodePresent(void)
{
  GenerateVirtualSlaveNodeInformation(0); /* Get the Virtual Slave node Information */
  assign_ID.nodeInfoLen = blslavenodeInfoLength;
  AddNodeInfo(newID, (uint8_t*)&newNodeInfoBuffer.nodeInfoFrame, false);
  ZW_NodeMaskSetBit(nodePool, newID); /* Now node is present */
  CtrlStorageSetBridgeNodeFlag(newID, true, true);
}
#endif


/*========================   SetLearnNodeStateNoServer   ======================
**    Set controller in "learn node mode".
**    When learn node mode is enabled, received "Node Info" frames are
**    handled and passed to the application through the call back function.
**
**    The states are:
**
**    LEARN_NODE_STATE_OFF     Stop accepting Node Info frames.
**    LEARN_NODE_STATE_NEW     Add new nodes to node list
**    LEARN_NODE_STATE_DELETE  Delete nodes from node list
**    LEARN_NODE_STATE_UPDATE  Update node-info in node list
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
void
ZW_SetLearnNodeStateNoServer( /*RET Nothing        */
  uint8_t mode)                  /*IN  1-New 2-Update 3-Delete 0-Disable learn mode */
{
  DPRINTF("ZW_SetLearnNodeStateNoServer: %x \n", mode);
#ifdef REPLACE_FAILED
  if (failedNodeReplace && (mode != LEARN_NODE_STATE_NEW))
  {
    failedNodeReplace = false;
  }
#endif
  g_learnNodeState = mode;

/* TODO: Initialy we should transmit a NOP frame to the nodeID which we want to use */
/* and if ACKed then get a new nodeID */
  if (g_learnNodeState != LEARN_NODE_STATE_OFF)
  {
#ifdef ZW_CONTROLLER_BRIDGE
    if (learnSlaveMode == VIRTUAL_SLAVE_LEARN_MODE_ADD)
    {
      if (assign_ID.newNodeID && (CtrlStorageCacheNodeExist(assign_ID.newNodeID) == 0))
      {
        virtualNodeID = 0;
        newID = assign_ID.newNodeID;
        SetNewVirtualNodePresent(); /* uses newID */
        assignSlaveState = ASSIGN_NODEID_DONE;
        assign_ID.assignIdState = ASSIGN_SEND_NODE_INFO_ADD;
        //find40KRoute = false;
        ZW_GetRoutingInfo(g_nodeID, assignIdBuf.newRangeInfo.maskBytes, ZW_GET_ROUTING_INFO_ANY);
        ZW_SetRoutingInfo(newID, MAX_NODEMASK_LENGTH, assignIdBuf.newRangeInfo.maskBytes);
        if (isNodeIDServerPresent())
        {
          /* We have now used the reserved node ID */
          uint8_t iZero = 0;
          CtrlStorageSetReservedId(iZero);
        }
        /* TO#3078 fix - Add Virtual node is done */
        if (!ReportNewVirtualNode())
        {
          assign_ID.assignIdState = ASSIGN_IDLE;
          ZW_AddNodeToNetwork(ADD_NODE_STOP, NULL);
          ZW_SetLearnNodeState(LEARN_NODE_STATE_OFF, NULL);
        }
        ZCB_AssignSlaveComplete(0, TRANSMIT_COMPLETE_OK, NULL);
        virtualNodeID = 0;
        return;
      }
      else
      {
        assignSlaveState = ASSIGN_COMPLETE;
        ZCB_AssignSlaveComplete(0, TRANSMIT_COMPLETE_FAIL, NULL);
        ZW_AddNodeToNetwork(ADD_NODE_STOP, NULL);
        assign_ID.assignIdState = ASSIGN_IDLE;
        virtualNodeID = 0;
        return;
      }
    }
#endif  /* ZW_CONTROLLER_BRIDGE */
    LearnInfoCallBack(LEARN_STATE_LEARN_READY, 0, 0);
  }
  else
  {
    /* STOP LEARN */
    g_findInProgress = false;
  }
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


/*========================   GetNodeProtocolInfo   ===========================
**
**    Copy the Node's current protocol information from the non-volatile
**    memory.
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
void                    /*RET Nothing        */
ZW_GetNodeProtocolInfo(
  node_id_t  nodeID,        /* IN Node ID */
  NODEINFO *nodeInfo)   /*OUT Node info buffer */
{
  if ( !nodeID ||
      ((nodeID > ZW_MAX_NODES) && (nodeID < LOWEST_LONG_RANGE_NODE_ID)) ||
      (nodeID > HIGHEST_LONG_RANGE_NODE_ID))
  {
    memset((uint8_t *)nodeInfo, 0, sizeof(NODEINFO));
  }
  else
  {
    if ((currentCachedNodeInfoNodeID != nodeID) || (0 == tNodeInfo.generic))
    {
      currentCachedNodeInfoNodeID = nodeID;
      CtrlStorageGetNodeInfo(nodeID, &tNodeInfo);
    }
    /* Convert nodeInfo from EX_NVM_NODEINFO -> NODEINFO */
    nodeInfo->capability = tNodeInfo.capability;
    nodeInfo->security = tNodeInfo.security;
    nodeInfo->reserved = tNodeInfo.reserved;
    nodeInfo->nodeType.specific = tNodeInfo.specific;
    nodeInfo->nodeType.generic = tNodeInfo.generic;
    ZW_GetNodeTypeBasic(nodeInfo);

    DPRINTF("Node %d node info :\n", nodeID);
    DPRINTF(".capability :%x\n", tNodeInfo.capability);
    DPRINTF(".security :%x\n", tNodeInfo.security);
    DPRINTF(".reserved :%x\n", tNodeInfo.reserved);
    DPRINTF(".basic :%x\n", nodeInfo->nodeType.basic);
    DPRINTF(".generic : %x\n", tNodeInfo.generic);
    DPRINTF(".specific : %x\n", tNodeInfo.specific);
  }
}


/*========================   ZW_AssignReturnRoute   =========================
**
**    Assign static return routes within a Routing Slave node.
**
**    If a priority route is provided (not recommended), only the priority
**    route will be assigned.
**
**    If no Priority Route is provided (recommended):
**    The shortest transport routes from the Routing Slave node
**    to the route destination node will be calculated and
**    transmitted to the Routing Slave node.
**
**    If Destination Node ID is recognized as SUC, Route will be assigned as
**    SUC return route.
**--------------------------------------------------------------------------*/
bool ZW_AssignReturnRoute(    /*RET true if assign was initiated. false if not */
  node_id_t  bSrcNodeID,           /* IN Routing Slave Node ID */
  node_id_t  bDstNodeID,           /* IN Report destination Node ID */
  uint8_t *pPriorityRoute,         /* IN Route to be assigned */
  bool isSucRoute,                 /*In  true if this a SUC route else false*/
  const STransmitCallback* pTxCallback) /*IN  Status of process */
{
  if (returnRouteAssignActive   ||
      ZW_MAX_NODES < bSrcNodeID || //reject routing for long range nodes
      ZW_MAX_NODES < bDstNodeID )
  {
    return false;
  }

  if (0 == bDstNodeID)
  {
    bDstNodeID = g_nodeID;    /*  Report destination is myself */
  }

  bool bIsSucNodeFound = ( (0 != GetSUCNodeID()) &&  (ZW_MAX_NODES >= GetSUCNodeID()));

  if (sucAssignRoute)
  {
    return false;
  } else {
    sucAssignRoute = isSucRoute;
    if(sucAssignRoute && !bIsSucNodeFound) {
     return false;
    }
  }

  /* Are we going to assign a priority route */
  if (pPriorityRoute == NULL)
  {
    returnRoutePriorityPresent = PRIORITY_ROUTE_NOT_PRESENT;
  }
  else
  {
    returnRoutePriorityPresent = PRIORITY_ROUTE_WAITING;
    memcpy(returnRoutePriorityBuffer, pPriorityRoute, sizeof(returnRoutePriorityBuffer));
  }
  returnDestNodeID = bDstNodeID;
  returnRouteNodeID = bSrcNodeID;
  returnRouteState = RETURN_ROUTE_STATE_ASSIGN;
  returnRouteNumber = 0;
  cmdCompleteReturnRouteFunc = *pTxCallback;
  returnRouteAssignActive = true;
  SendReturnRoute(); /* transmit first return route */

  return true;
}


/*========================   ZW_DeleteReturnRoute   =========================
**
**    Delete static return routes within a Routing Slave node.
**    Transmit "NULL" routes to the Routing Slave node.
**
**--------------------------------------------------------------------------*/
bool                  /*RET true if delete return routes was initiated. */
                      /*    false if a return route assign/delete is allready active */
ZW_DeleteReturnRoute(
  node_id_t  bNodeID,      /*IN Routing Slave Node ID */
  bool bDeleteSUC,    /*IN Delete SUC return routes only, or Delete standard return routes only */
  const STransmitCallback* pTxCallback) /*IN  Status callback of process */
{
  if (returnRouteAssignActive ||
      ZW_MAX_NODES < bNodeID ) //reject routing for long range nodes
  {
    return false;
  }

  if (bDeleteSUC)
  {
    sucAssignRoute = true;
  }

  returnDestNodeID = 0;         /*  It is a delete */
  returnRouteNodeID = bNodeID;
  returnRouteState = RETURN_ROUTE_STATE_DELETE;
  returnRouteNumber = 0;
  cmdCompleteReturnRouteFunc = *pTxCallback;
  returnRouteAssignActive = true;
  SendReturnRoute(); /* transmit first return route */
  return true;
}


/*===========================   SetDefault   ================================
**    Remove all Nodes and timers from the EEPROM memory.
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
void           /*RET  Nothing        */
ZW_SetDefault(void)
{
  /* Full reset */
  SetDefaultCmd(true);
}


/*===========================   ZW_ClearTables  =============================
**    Remove all Nodes and timers from the EEPROM memory. Keeps its own current
**    active home and node ID.
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
void           /*RET  Nothing        */
ZW_ClearTables(void)
{
  /* Set setDefault only to reset EEPROM and none learnMode/replication state variables */
  SetDefaultCmd(false);
}


/*=========================   FindNodesTimeout   ============================
**
**    Timeout function for find nodes within range
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
void
ZCB_FindNodesTimeout(SSwTimer* pTimer) /* RET  Nothing */
{
  if (assign_ID.assignIdState == ASSIGN_FIND_NODES_SEND)
  {
    /* Cancel wait forever timer */
    RequestRangeInfo();
  }
  else if (assign_ID.assignIdState == ASSIGN_RANGE_REQUESTED)
  {
    /* We requested Rangeinfo, but did not receive it. */
    TimerStop(pTimer); /* We always run FOREVER. So stop it here */
    zensorWakeupTime = ZWAVE_SENSOR_WAKEUPTIME_NONE;
    if (FIND_NEIGHBOR_NODETYPE_IDLE !=findNeighborsWithNodetypeState)
    {
      ListeningNodeDiscoveryDone();
      return;
    }
    /* Range info wasn't received in time, send complete to application, and */
    /* mark node as pending discovery */
#ifdef ZW_ROUTED_DISCOVERY
    if (!updateNodeNeighbors)
#endif
    {
      SetPendingDiscovery(assign_ID.newNodeID);
      if (staticControllerNodeID && primaryController)
      {
        SetPendingUpdate(assign_ID.newNodeID);
      }
    }
#ifdef ZW_ROUTED_DISCOVERY
    /* We are done but did not get any range info */
    LearnInfoCallBack(updateNodeNeighbors ? LEARN_STATE_FAIL : LEARN_STATE_DONE,
                      assign_ID.newNodeID, 0);
#else
    LearnInfoCallBack(LEARN_STATE_DONE, assign_ID.newNodeID, 0);
#endif
  }
  else if (assign_ID.assignIdState == ASSIGN_FIND_SENSOR_NODES_SEND)
  {
    /* Cancel wait forever timer */
    RequestRangeInfo();
  }
  else if (assign_ID.assignIdState == ASSIGN_RANGE_SENSOR_REQUESTED)
  {
    /* We requested Rangeinfo, but did not receive it. */
    AssignTimerStop(); /* We always run FOREVER. So stop it here */
    zensorWakeupTime = ZWAVE_SENSOR_WAKEUPTIME_NONE;

    /* Range info wasn't received in time, send complete to application, and */
    /* mark node as pending discovery */
#ifdef ZW_ROUTED_DISCOVERY
    if (!updateNodeNeighbors)
#endif
    {
      SetPendingDiscovery(assign_ID.newNodeID);
      if (staticControllerNodeID && primaryController)
      {
        SetPendingUpdate(assign_ID.newNodeID);
      }
    }
#ifdef ZW_ROUTED_DISCOVERY
    /* We are done but did not get any range info */
    LearnInfoCallBack(updateNodeNeighbors ? LEARN_STATE_FAIL : LEARN_STATE_DONE,
                      assign_ID.newNodeID, 0);
#else
    LearnInfoCallBack(LEARN_STATE_DONE, assign_ID.newNodeID, 0);
#endif
  }
  else
  {
    zensorWakeupTime = ZWAVE_SENSOR_WAKEUPTIME_NONE;
    /* Controller have moved on. Make sure to stop timer */
    AssignTimerStop();
  }
}


/*=====================   CallAssignTxCompleteFailed   =======================
**
**    Call AssignTxComplete with TRANSMIT_COMPLETE_FAIL
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
void
ZCB_CallAssignTxCompleteFailed(SSwTimer* pTimer)
{
  if (NULL != pTimer)     // Method is sometimes called directly with null pointer
  {
    TimerStop(pTimer);
  }

  ZCB_AssignTxComplete(0, TRANSMIT_COMPLETE_FAIL, NULL);
}


/*====================   DelayAssignTxCompleteFailCall   =====================
**
**    Delay calling AssignTxComplete with TRANSMIT_COMPLETE_FAIL - max 10ms
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
static void
DelayAssignTxCompleteFailCall(void)
{
  NetWorkTimerStart(&AssignTxCompleteDelayTimer, ZCB_CallAssignTxCompleteFailed, 10);
}

/*===========================   LearnInfoTimeout   ===========================
**
**    Timer used to delay the notification to the application for 500 ms so we make
**    sure range info is updated
**    Side effects:
**
**--------------------------------------------------------------------------*/
void
ZCB_NewNodeAssignedUpdateTimeout( __attribute__((unused)) SSwTimer* pTimer )
{
  ProtocolInterfacePassToAppNodeUpdate(
      UPDATE_STATE_NEW_ID_ASSIGNED,
      newNodeInfoBuffer.mNodeID,
      (uint8_t *)&newNodeInfoBuffer.nodeInfoFrame.nodeType,
      newNodeInfoBuffer.cmdLength);
}
/*===========================   LearnInfoTimeout   ===========================
**
**    Timer used to delay the callback until the EEPROM write of the range
**    info is complete. This should be handled by a callback on
**    RoutingInfoReceived(), but that callback doesnt exist (yet)
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
void
ZCB_LearnInfoTimeout( SSwTimer* pTimer )
{
  TimerStop(pTimer);
  /*If we arent replicating Start analyse routing table now*/
  if (GET_NEW_CTRL_STATE == NEW_CTRL_STOP)
  {
    RestartAnalyseRoutingTable();
  }

  /* Learn Complete */
  if (primaryController &&
      staticControllerNodeID && (staticControllerNodeID != assign_ID.newNodeID)
#if defined(ZW_CONTROLLER_STATIC)
      && (ZW_IS_NOT_SUC) /* Only if SUC are not ME... */
#endif
     )
  {
    assign_ID.assignIdState = ASSIGN_SEND_RANGE_INFO;
    if (pendingTableEmpty)
    {
      ROUTING_ANALYSIS_STOP();
      assignIdBuf.newRangeInfo.cmdClass = ZWAVE_CMD_CLASS_PROTOCOL;
      assignIdBuf.newRangeInfo.cmd = ZWAVE_CMD_NEW_RANGE_REGISTERED;
      static const STransmitCallback TxCallback = { .pCallback = ZCB_AssignTxComplete, .Context = 0 };
      uint8_t len = offsetof(NEW_RANGE_REGISTERED_FRAME, maskBytes) + assignIdBuf.newRangeInfo.numMaskBytes;
      if(!EnQueueSingleData(false, g_nodeID, staticControllerNodeID, (uint8_t *)&assignIdBuf,
                           len,
                           (TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE),
                           0, // 0ms for tx-delay (any value)
                           ZPAL_RADIO_TX_POWER_DEFAULT,
                           &TxCallback))
      {
        ZCB_CallAssignTxCompleteFailed(NULL);
      }
      return;
    }
    else
    {
      ZCB_CallAssignTxCompleteFailed(NULL);
    }

  }
  else
  {
    LearnInfoCallBack(LEARN_STATE_DONE, assign_ID.newNodeID, 0);
  }
}


#if defined(ZW_CONTROLLER_STATIC)
/*=========================== UpdateTimeOutHandler ===========================
** Time out handler function used to delay the static controller topology
** update process  Until after all the new node information are received
**
**----------------------------------------------------------------------------*/
void
ZCB_UpdateTimeOutHandler(SSwTimer* pTimer)
{
  if (--count == 0)
  {
    TimerStop(pTimer);
    PendingNodeUpdate();
  }
}


#ifdef ZW_SELF_HEAL
/*============================   NodeLostCompleted   ======================
**    Function description
**      Callbackfunction for lost node rediscovery
**    Side effects:
**
**--------------------------------------------------------------------------*/
void
ZCB_NodeLostCompleted(
  LEARN_INFO_T *plearnNodeInfo)
{
  if ((((*plearnNodeInfo).bStatus == LEARN_STATE_DONE))||
      ((*plearnNodeInfo).bStatus == LEARN_STATE_FAIL))
  {
    if (lostOngoing)
    {
      /* Compare new nodeInfo with old one and only update if neighbors actually changed*/
      ZW_GetRoutingInfo((*plearnNodeInfo).bSource, abNeighbors, ZW_GET_ROUTING_INFO_ANY);
      if (memcmp(tempNodeLostMask, abNeighbors, MAX_NODEMASK_LENGTH) != 0)
      {
        /* TO#1244 fix - If a node requesting ZW_Rediscovery only have the node */
        /* that helps as a neighbour the new range info will NOT be distributed to */
        /* secondary controllers/Inclusion controllers from the SUC/SIS */
        /* Invalidate Rangeinfo for this node */
        didWrite = true;
        SUCUpdateNodeInfo((*plearnNodeInfo).bSource, SUC_UPDATE_RANGE, 0);
      }
    }
    SendSUCTransferEnd((*plearnNodeInfo).bSource, ZWAVE_TRANSFER_UPDATE_DONE);
    /* TO#4118 fix - Reset all variables used specifically in the lost functionality */
    ResetSUC();
    g_learnNodeState = LEARN_NODE_STATE_OFF;
  }
}


/*============================   StartLostRediscovery   ======================
**    Function description
**      Timeout function which starts the Lost rediscovery process.
**    Side effects:
**
**--------------------------------------------------------------------------*/
void
ZCB_StartLostRediscovery(SSwTimer* pTimer)
{
  TimerStop(pTimer);
  if (!RequestNodeRange(true))
  {
    /* Cant get rangeinfo */
    LearnInfoCallBack(LEARN_STATE_FAIL,
                      assign_ID.newNodeID,
                      assign_ID.nodeInfoLen - offsetof(NODEINFO_FRAME, nodeType));
    /* TO#2772 partial fix - Make sure we reset all variables used in the lost functionality */
    ResetSUC();
  }
  else
  {
    NetWorkTimerStart(&SUC_Update.TimeoutTimer, ZCB_SUCTimeOut, 0xFE * 10 * (SUC_REQUEST_UPDATE_TIMEOUT + 1));
  }
}
#endif /*ZW_SELF_HEAL */
#endif  /* ZW_CONTROLLER_STATIC) */

/*============================   ToNewNodeInfo  =============================
**    Function description
**      makes sure that received nodeInfo is in the "new" format.
**    Side effects:
**
**--------------------------------------------------------------------------*/
static void
ToNewNodeInfo(
  uint8_t *pCmd)
{
  uint8_t cmdLength = assign_ID.nodeInfoLen;
  /* TODO - Why only when primary??? */
  if (primaryController)
  {
    memset(newNodeInfoBuffer.nodeInfoFrame.nodeInfo, 0, NODEPARM_MAX);
  }
  if (!(((NODEINFO_FRAME*)pCmd)->security & ZWAVE_NODEINFO_SPECIFIC_DEVICE_TYPE))
  {
    /* No Specific Device Type in nodeinfo frame... */
    /* Now set the Specific Device Type support bit */
    ((NODEINFO_FRAME*)pCmd)->security |= ZWAVE_NODEINFO_SPECIFIC_DEVICE_TYPE;
    /* First copy until and including nodeType (for old devices its generic device type) */
    memcpy((uint8_t*)&newNodeInfoBuffer.nodeInfoFrame, pCmd, offsetof(NODEINFO_OLD_FRAME, nodeInfo));
    newNodeInfoBuffer.nodeInfoFrame.nodeType.generic = newNodeInfoBuffer.nodeInfoFrame.nodeType.basic;  /* Put generic the right place */
    ZW_GetNodeTypeBasic((NODEINFO *)&newNodeInfoBuffer.nodeInfoFrame.capability); /* Set the Basic Device Type */
    newNodeInfoBuffer.nodeInfoFrame.nodeType.specific = 0; /* Set the Specific Device Type to something useful */
    assign_ID.nodeInfoLen += 2; /* Add the Basic and Specific Device classes */
    /* Copy the command classes - if any... */
    if (cmdLength - offsetof(NODEINFO_OLD_FRAME, nodeInfo) > 0)
    {
      /* TO#3281 partial fix : Received newNodeInfoBuffer.nodeInfoFrame must MAX have NODEPARM_MAX commandclasses */
      if (cmdLength - offsetof(NODEINFO_OLD_FRAME, nodeInfo) > NODEPARM_MAX)
      {
        cmdLength = offsetof(NODEINFO_OLD_FRAME, nodeInfo) + NODEPARM_MAX;
      }
      memcpy(newNodeInfoBuffer.nodeInfoFrame.nodeInfo,
             pCmd + offsetof(NODEINFO_OLD_FRAME, nodeInfo),
             cmdLength - offsetof(NODEINFO_OLD_FRAME, nodeInfo));
    }
  }
  else
  {
    /* First copy everything, for Controllers the first Device Type in the nodeinfo */
    /* is the Basic Device Type, for Slaves the First Device Type it is the Generic Device Type */
    if (cmdLength > sizeof(newNodeInfoBuffer.nodeInfoFrame))
    {
      cmdLength = sizeof(newNodeInfoBuffer.nodeInfoFrame);
    }
    memcpy((uint8_t*)&newNodeInfoBuffer.nodeInfoFrame, pCmd, cmdLength);
    if (!(newNodeInfoBuffer.nodeInfoFrame.security & ZWAVE_NODEINFO_CONTROLLER_NODE))
    {
      /* It is a Slave */
      /* Set the Basic Device Type */
      ZW_GetNodeTypeBasic((NODEINFO *)&newNodeInfoBuffer.nodeInfoFrame.capability);
      /* One more for the Basic Device Type, which is generated */
      assign_ID.nodeInfoLen++;
      /* TO#3281 partial fix : Received newNodeInfoBuffer.nodeInfoFrame must MAX have NODEPARM_MAX commandclasses */
      if (cmdLength == sizeof(newNodeInfoBuffer.nodeInfoFrame))
      {
        /* Drop one more Command Class as no room for it. */
        cmdLength--;
      }
      /* Copy Generic and Specfic Device Types and the Command Classes */
      memcpy((uint8_t*)&newNodeInfoBuffer.nodeInfoFrame.nodeType.generic,
             pCmd + offsetof(NODEINFO_SLAVE_FRAME, nodeType),
             cmdLength - offsetof(NODEINFO_SLAVE_FRAME, nodeType));
    }
  }
  /* TO#3281 partial fix : Received nodeinfoframe must MAX have NODEPARM_MAX commandclasses */
  if (assign_ID.nodeInfoLen - offsetof(NODEINFO_FRAME, nodeInfo) > NODEPARM_MAX)
  {
    assign_ID.nodeInfoLen = offsetof(NODEINFO_FRAME, nodeInfo) + NODEPARM_MAX;
  }
}


/* TO#1871 Fix - Static/Bridge norep controller with SUC/SIS enabled does not respond to Lost frame */
#if defined(ZW_SELF_HEAL) && defined(ZW_CONTROLLER_STATIC) && defined(ZW_REPEATER)
/*============================   AcceptLostSent   ======================
**    Function description
**      Sent Accept lost now is ok to start Rediscovery
**    Side effects:
**
**--------------------------------------------------------------------------*/
static void
ZCB_AcceptLostSent(
  __attribute__((unused)) ZW_Void_Function_t Context,
  __attribute__((unused)) uint8_t txStatus,
  __attribute__((unused)) TX_STATUS_TYPE *txStatusReport)
{
  if (ZW_IS_SUC)
  {
    /* TO#1571 fix - Only start timer if SUC */
    NetWorkTimerStart(&LostRediscoveryTimer, ZCB_StartLostRediscovery, 500);
  }
}


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
  assignIdBuf.acceptLostFrame.cmdClass = ZWAVE_CMD_CLASS_PROTOCOL;
  assignIdBuf.acceptLostFrame.cmd = ZWAVE_CMD_ACCEPT_LOST;
  if (txStatus == TRANSMIT_COMPLETE_OK)
  {
    /* Got hold of controller */
    assignIdBuf.acceptLostFrame.accepted = ZW_ROUTE_LOST_ACCEPT;
  }
  else
  {
    assignIdBuf.acceptLostFrame.accepted = ZW_ROUTE_LOST_FAILED;
  }
  static const STransmitCallback TxCallback = { .pCallback = ZCB_AcceptLostSent, .Context = 0 };
  if (!EnQueueSingleData(false, g_nodeID, assign_ID.newNodeID, (uint8_t *)&assignIdBuf.acceptLostFrame,
                              sizeof(ACCEPT_LOST_FRAME),
                              (TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE),
                              0, // 0ms for tx-delay (any value)
                              ZPAL_RADIO_TX_POWER_DEFAULT,
                              &TxCallback))
  {
    ResetSUC();
  }
}
#endif /* ZW_SELF_HEAL && ZW_CONTROLLER_STATIC && ZW_REPEATER */


/*=====================   ReplicationTransferEndReceived   ===================
**    Function description
**      Transfer End Received.
**      Also called by Replication module when Replication Receive times out.
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
void
ReplicationTransferEndReceived(
  uint8_t bSourceNodeID,
  uint8_t bStatus)
{
  (void)bSourceNodeID;// = bSourceNodeID; /* HACK: to avoid *** WARNING C280 unreferenced local variable */
  /* Are we included? */
  if ((bStatus == ZWAVE_TRANSFER_OK) || ((bStatus == ZWAVE_TRANSFER_FAIL) && controllerIncluded) || (controllerOnOther && !g_nodeID))
  {
    /* make sure we are in Receive mode before we change configuration */
    if (NEW_CTRL_RECEIVER_BIT)
    {
      controllerIncluded = false;
      //Code optimization HEH
      controllerOnOther = true;
      realPrimaryController = true;
      primaryController = true;

      if (newPrimaryReplication)
      {
        SaveControllerConfig();
        ControllerStorageGetNetworkIds(ZW_HomeIDGet(), &g_nodeID);
        CtrlStorageSetMaxNodeId(bMaxNodeID);
        TransferDoneCallback(NEW_CONTROLLER_DONE);
        return;
      }

      if (g_nodeID) /* Added to the network as secondary */
      {
        realPrimaryController = false;
        primaryController = isNodeIDServerPresent();
      }
      else /* Deleted */
      {
        controllerOnOther = false;  /* We have been reset our original HomeID */
        /* We must remove the node ID server preset status when we execlude the node from the network.*/
        SetNodeIDServerPresent(false);
        HomeIdGeneratorGetNewId(ZW_HomeIDGet());
        g_nodeID = NODE_CONTROLLER;
        HomeIdUpdate(ZW_HomeIDGet(), g_nodeID);
      }
      SaveControllerConfig();
      TransferDoneCallback(NEW_CONTROLLER_DONE);
      InitRoutingValues();
    }
  }
  else if (bStatus == ZWAVE_TRANSFER_FAIL)
  {
    /* Do we ever get here without being in Receive? */
    if (NEW_CTRL_RECEIVER_BIT)
    {
      /* We are going to be Reset */
      resetCtrl = true;
      TransferDoneCallback(NEW_CONTROLLER_FAILED);
    }
  }
  else
  {
    if (SUC_Update.updateState != SUC_IDLE)
    {
      ResetSUC();
      if ((bStatus == ZWAVE_TRANSFER_UPDATE_DONE) || (bStatus == ZWAVE_TRANSFER_UPDATE_ABORT))
      {
        RestartAnalyseRoutingTable();
      }
      /* Convert to Application values */
      ZW_TransmitCallbackInvoke(&SUCCompletedFunc, bStatus - ZWAVE_APPLICATION_VAL_OFFSET, NULL);
    }
  }
  /* Unlock Last Working Route for purging and updating */
  ZW_LockRoute(false);
}

// Struct describing Multicast nodemask header
typedef struct SMultiCastNodeMaskHeader
{
  uint8_t iNodemaskLength : 5;  // Bits 0-4 is length. Length of Nodemask in bytes - Valid values [0-29]
  uint8_t iNodeMaskOffset : 3;  // Bits 5-7 is offset. Denotes which node the first bit in the nodemask describes
                                // First node in nodemask is (Value * 32) + 1 - e.g. 2 -> first node is 65
} SMultiCastNodeMaskHeader;

/*=======================   ZW_HandleCmdNodeInfo   ==========================
**
**    Common command handler for:
**           - ZWAVE_CMD_NODE_INFO
**           - ZWAVE_LR_CMD_NODE_INFO
**
**--------------------------------------------------------------------------*/
static void ZW_HandleCmdNodeInfo(uint8_t *pCmd, uint8_t cmdLength, RECEIVE_OPTIONS_TYPE *rxopt)
{
  uint8_t rxStatus = rxopt->rxStatus;
  node_id_t sourceNode = rxopt->sourceNode;
  node_id_t bNodeID;

  /* Check if frame is to short then drop it... */
  if (cmdLength < offsetof(NODEINFO_OLD_FRAME, nodeInfo))
  {
    return;
  }

  if (0 != bNetworkRemoveNodeID)
  {
    if (g_learnNodeState == LEARN_NODE_STATE_DELETE)
    {
      if ((0 != (rxStatus & RECEIVE_STATUS_FOREIGN_HOMEID)) || (bNetworkRemoveNodeID != sourceNode))
      {
        /* We have received a ZWAVE_CMD_NODE_INFO/ZWAVE_CMD_EXCLUDE_REQUEST command but */
        /* the source do not match specified requirements */
        return;
      }
      /* sourceNode equals HomeID - bNetworkRemoveNodeID */
    }
    else
    {
      /* We are not in LEARN_NODE_STATE_DELETE */
      bNetworkRemoveNodeID = 0;
    }
  }

  /* TO#2381 fix 5.0x and 4.2x nodes don't zero reserved field in Node Info frame before tx */
  if (((((NODEINFO_FRAME*)pCmd)->capability & ZWAVE_NODEINFO_PROTOCOL_VERSION_MASK) < ZWAVE_NODEINFO_VERSION_4)
      && (ZWAVE_CMD_CLASS_PROTOCOL_LR != *pCmd))
  {
    // Must be done for non-LR frames only because otherwise we lose the 100K LR bit.
    ((NODEINFO_FRAME*)pCmd)->reserved = 0;
  }
  /* TO#491 fix - If we are trying to make an old remote (prior to 2.3) a new primary then abort the operation */
  if ((GET_NEW_CTRL_STATE == NEW_CTRL_SEND) &&
      newPrimaryReplication &&
      ((((NODEINFO_FRAME*)pCmd)->security & (ZWAVE_NODEINFO_SPECIFIC_DEVICE_TYPE | ZWAVE_NODEINFO_CONTROLLER_NODE)) == ZWAVE_NODEINFO_CONTROLLER_NODE ) )
  {
    /* An old remote version do not support new primary controller, then abort the replication */
    otherController = sourceNode;
#if defined(ZW_CONTROLLER_STATIC)
    doChangeToSecondary = false;
#endif  /* ZW_CONTROLLER_STATIC */
    newPrimaryReplication = false;
    ZCB_SendTransferEnd(0, TRANSMIT_COMPLETE_NO_ACK, NULL);
    return;
  }
  zw_node_t new_node = { 0 };
#ifdef ZW_ROUTED_DISCOVERY
  /* We do not want any new nodes while in routed discovery ;=) */
  if (!updateNodeNeighbors && (assign_ID.assignIdState == ASSIGN_IDLE))
#else
  if (assign_ID.assignIdState == ASSIGN_IDLE)
#endif
  {
    assign_ID.nodeInfoLen = cmdLength;
    /* Received a Nodeinformation frame from other Node Convert it to new node info if it is not allready */
    if (ZWAVE_CMD_CLASS_PROTOCOL_LR == *pCmd)
    {
      // Parse the frame as a LR frame.

      const NODEINFO_LR_SLAVE_FRAME * const pFrame = (NODEINFO_LR_SLAVE_FRAME *)pCmd;

      if (pFrame->capability & ZWAVE_NODEINFO_LISTENING_MASK)
      {
        node_listening_set(&new_node, true);
      }

      if (pFrame->security & 0x40)
      {
        node_supports_flirs_1000ms_set(&new_node, true);
      }
      else if (pFrame->security & 0x20)
      {
        node_supports_flirs_250ms_set(&new_node, true);
      }

      if (pFrame->security & ZWAVE_NODEINFO_CONTROLLER_NODE)
      {
        node_controller_set(&new_node, true);
      }

      if (pFrame->reserved & ZWAVE_NODEINFO_BAUD_100KLR)
      {
        node_speed_100klr_set(&new_node, true);
      }

      // Long Range node info frames doesn't contain a basic type value. Hardcode it.
      node_basic_type_set(&new_node, BASIC_TYPE_END_NODE);

      node_generic_device_class_set(&new_node, pFrame->nodeType.generic);
      node_specific_device_class_set(&new_node, pFrame->nodeType.specific);
      node_cc_list_set(&new_node, &pFrame->nodeInfo[0], pFrame->cc_list_length);
      node_long_range_set(&new_node, true);

      /*
       * Now update the global variable like ToNewNodeInfo() does.
       *
       * This is required because the global variable most likely is used somewhere.
       *
       * Long term: use new_node_info.
       */
      memset((uint8_t *)&newNodeInfoBuffer.nodeInfoFrame, 0, sizeof(NODEINFO_FRAME));

      if (node_is_listening(&new_node))
      {
        newNodeInfoBuffer.nodeInfoFrame.capability |= ZWAVE_NODEINFO_LISTENING_MASK;
      }

      if (node_supports_flirs_1000ms(&new_node))
      {
        newNodeInfoBuffer.nodeInfoFrame.security |= 0x40;
      }
      else if(node_supports_flirs_250ms(&new_node))
      {
        newNodeInfoBuffer.nodeInfoFrame.security |= 0x20;
      }

      if (node_is_controller(&new_node))
      {
        newNodeInfoBuffer.nodeInfoFrame.security |= ZWAVE_NODEINFO_CONTROLLER_NODE;
      }

      if (node_supports_speed_100klr(&new_node))
      {
        newNodeInfoBuffer.nodeInfoFrame.reserved |= ZWAVE_NODEINFO_BAUD_100KLR;
      }

      newNodeInfoBuffer.nodeInfoFrame.nodeType.basic    = node_basic_type_get(&new_node);
      newNodeInfoBuffer.nodeInfoFrame.nodeType.generic  = node_generic_device_class_get(&new_node);
      newNodeInfoBuffer.nodeInfoFrame.nodeType.specific = node_specific_device_class_get(&new_node);

      node_cc_list_get(&new_node, &newNodeInfoBuffer.nodeInfoFrame.nodeInfo[0]);
    }
    else
    {
      // Parse the frame in the oldschool way.
      ToNewNodeInfo(pCmd);
    }

    assign_ID.sourceID = sourceNode;
    assign_ID.rxOptions = rxStatus;
    /* TODO: Network wide inclusion - the TRANSMIT_OPTION_NO_ROUTE should only be present when not doing network */
    /* wide inclusion - Some problems could arrive when doing remove node - when to know when a node is removed? */
    assign_ID.txOptions = TRANSMIT_OPTION_ACK |
                          ((rxStatus & RECEIVE_STATUS_TYPE_EXPLORE) ? 0 : TRANSMIT_OPTION_NO_ROUTE) |
                          (rxStatus & RECEIVE_STATUS_LOW_POWER); // RECEIVE_STATUS_LOW_POWER == TRANSMIT_OPTION_LOW_POWER

    if (g_learnNodeState == LEARN_NODE_STATE_DELETE)
    {
      /* Does it include a Specific Device Type ??? */
      /* If already cleared then call complete function */
      /* TO#2778 - check for already cleared removed */
      /*
      if (sourceNode == 0)
      {
        LearnInfoCallBack(LEARN_STATE_DONE, 0x00,
                          assign_ID.nodeInfoLen - offsetof(NODEINFO_FRAME, nodeType));
        return;
      }
      */
      if ((pCmd[1] == ZWAVE_CMD_EXCLUDE_REQUEST) && (0 == (rxStatus & RECEIVE_STATUS_TYPE_EXPLORE)) && (!rxopt->isLongRangeChannel))
      {
        /* ZWAVE_CMD_EXCLUDE_REQUEST are only allowed via an Explore frame if it comes from classic Z-Wave channel and from singlecast if it from LR channel*/
        return;
      }

      /* TODO: Should we add some more security - by checking if controller or slave and if allowed */
      if (((addCtrlNodes == false) && ( 0 != (newNodeInfoBuffer.nodeInfoFrame.security & ZWAVE_NODEINFO_CONTROLLER_NODE))) ||
          ((addSlaveNodes == false) && (0 == (newNodeInfoBuffer.nodeInfoFrame.security & ZWAVE_NODEINFO_CONTROLLER_NODE))))
      {
        return;
      }
      assign_ID.newNodeID = 0;
      otherController = (newNodeInfoBuffer.nodeInfoFrame.security & ZWAVE_NODEINFO_CONTROLLER_NODE) ? sourceNode : 0;
      AssignNewID(rxopt->isLongRangeChannel);
    }
    else if (pCmd[1] == ZWAVE_CMD_EXCLUDE_REQUEST)
    {
      /* Drop frame we are not in exclude mode */
      return;
    }
    else if (
              (NETWORK_WIDE_SMART_START != bNetworkWideInclusion) &&
              (ZWAVE_CMD_SMARTSTART_PRIME_NODE_INFO != pCmd[1]) &&
              ((g_learnNodeState == LEARN_NODE_STATE_NEW) ||
              (g_learnNodeState == LEARN_NODE_STATE_UPDATE))
            )
    {
      /* We are adding a new node */
      DPRINT("!");
      if (addSmartStartNode == true)
      {
        // Break if Inclusion must be done on Long Range and NIF either has been received on a legacy channel or NIF do not state Long Range support
        if (addUsingLR && (!rxopt->isLongRangeChannel || (0 == (ZWAVE_NODEINFO_BAUD_100KLR & newNodeInfoBuffer.nodeInfoFrame.reserved))))
        {
          return;
        }
        // Break if Inclusion must be done on Legacy channel and NIF has been received on a Long Range channel
        if (!addUsingLR && rxopt->isLongRangeChannel)
        {
          return;
        }

        /* We must be in Smart Start Include mode - Check if DSK matches */
        if (0 != memcmp(addSmartNodeHomeId, rxopt->homeId.array, HOMEID_LENGTH))
        {
          DPRINT(" x");
          /* No match */
          return;
        }
        else
        {
          DPRINT(" v");
          /* Match! - copy second part of DSK to crH as we need it for HomeID when sending assignID */
          memcpy(crH, &addSmartNodeHomeId[4], HOMEID_LENGTH);
        }
      }
      else
      {
        if (rxopt->isLongRangeChannel)
        {
          // Only Smart Start can include on Long Range channel
          return;
        }
      }
      /* break if wrong node type */
      if (
          !(addSmartStartNode == true) &&
          (((addCtrlNodes == false) && (newNodeInfoBuffer.nodeInfoFrame.security & ZWAVE_NODEINFO_CONTROLLER_NODE)) ||
            ((addSlaveNodes == false) && !(newNodeInfoBuffer.nodeInfoFrame.security & ZWAVE_NODEINFO_CONTROLLER_NODE)))
          )
      {
        return;
      }
      /* Use inclusion homeid except when including virtual nodes (they send out node info with
        * zero homeid, but will not accept it in AssignID frames) */
      if (0 != rxopt->homeId.word)
      {
        /* Now inclusionHomeID is active */
        inclusionHomeIDActive = true;
        memcpy(inclusionHomeID, crH, HOMEID_LENGTH);
      }
      if (sourceNode)
      {
#ifdef REPLACE_FAILED
        /* If we are trying to replace a node with a new slave node then it have to be reset */
        if (failedNodeReplace && !(newNodeInfoBuffer.nodeInfoFrame.security & ZWAVE_NODEINFO_CONTROLLER_NODE))
        {
          return;
        }
#endif
        if ((rxStatus & RECEIVE_STATUS_FOREIGN_HOMEID) == 0)
        {
          /* Node already has a valid Node id.*/
          if(g_learnNodeState == LEARN_NODE_STATE_UPDATE)
          {
            /* If we are updating then this line makes sure that we do not try to replicate to a controller */
            addCtrlNodes = false;
          }
          else
          {
            if ((newNodeInfoBuffer.nodeInfoFrame.capability & ZWAVE_NODEINFO_PROTOCOL_VERSION_MASK) < ZWAVE_NODEINFO_VERSION_3)
            {
              /* Only controllers can replicate */
              if ((GET_NEW_CTRL_STATE == NEW_CTRL_SEND) && ((newNodeInfoBuffer.nodeInfoFrame.security & ZWAVE_NODEINFO_CONTROLLER_NODE) != 0))
              {
                if (newPrimaryReplication || (sourceNode == NODE_CONTROLLER_OLD))
                {
                  assign_ID.newNodeID = newPrimaryReplication ? NODE_CONTROLLER_OLD : GetNextFreeNode(rxopt->isLongRangeChannel);
                  AssignNewID(rxopt->isLongRangeChannel);
                  return;
                }
              }
            }
            else
            {
              if (sourceNode == NODE_CONTROLLER_OLD)
              {
                assign_ID.newNodeID = GetNextFreeNode(rxopt->isLongRangeChannel);
                AssignNewID(rxopt->isLongRangeChannel);
                return;
              }
            }
          }
          /* Valid homeID and node ID.. Signal to application */
          assign_ID.newNodeID = sourceNode;

          /* Signal to application that a node is discovered */
          LearnInfoCallBack(LEARN_STATE_NODE_FOUND, assign_ID.newNodeID,
                            assign_ID.nodeInfoLen - offsetof(NODEINFO_FRAME, nodeType));
          /* Check that Application do not want to switch LearnMode OFF */
          /* TO#1250 - The application kan here abort the inclusion process if the received */
          /* nodeinformation do not fulfill the requirements for being included - this is */
          /* possible if ZW_AddNodeToNetwork has been called, with ADD_NODE_STOP, in the */
          /* callback that should have been called in LearnInfoCallBack. */
          if (g_learnNodeState != LEARN_NODE_STATE_OFF)
          {
            /* Let the inclusion proceed. */
            /* As we already got a nodeId, but we store information and update routing if needed, */
            /* skip to NOP sent */
            assign_ID.assignIdState = ASSIGN_NOP_SEND;
            /* Fake that NOP transmit was ok */
            ZCB_AssignTxComplete(0, TRANSMIT_COMPLETE_OK, NULL);
            /* Make sure the node is not removed from node table if replication fails  */
            bReplicationDontDeleteOnfailure = true;
          }
          else
          {
            assign_ID.assignIdState = ASSIGN_IDLE;
          }
        }
        /* Only Controllers can replicate */
        else if ((GET_NEW_CTRL_STATE == NEW_CTRL_SEND) && ((newNodeInfoBuffer.nodeInfoFrame.security & ZWAVE_NODEINFO_CONTROLLER_NODE) != 0))
        {
          /* Home id was invalid but we are replicating */
          /*   so try and assign new homeID */
          if (newPrimaryReplication && ((newNodeInfoBuffer.nodeInfoFrame.capability & ZWAVE_NODEINFO_PROTOCOL_VERSION_MASK) < ZWAVE_NODEINFO_VERSION_3))
          {
            bNodeID = NODE_CONTROLLER_OLD;
          }
          else
          {
            if (!assign_ID.newNodeID)
            {
              bNodeID = GetNextFreeNode(rxopt->isLongRangeChannel);
            }
            else
            {
              bNodeID = assign_ID.newNodeID;
            }
          }
          if (bNodeID)
          {
            assign_ID.newNodeID = bNodeID;
            AssignNewID(rxopt->isLongRangeChannel);
          }
          else
          {
            /* No nodeID could be allocated therefor we FAIL */
            otherController = sourceNode;
            ZCB_SendTransferEnd(0, TRANSMIT_COMPLETE_NO_ACK, NULL);
          }
        }
      }
      /* Only Slaves can have nodeID ZERO when transmitting nodeinformation frame */
      else if (((newNodeInfoBuffer.nodeInfoFrame.security & ZWAVE_NODEINFO_CONTROLLER_NODE) == 0))  /* New node */
      {
        /* Make sure that the nodeID is unused - before assigning it to a new node */
        /* Fixed TO#2010 - When replacing old node id should be used*/
        if (!assign_ID.newNodeID || (CtrlStorageCacheNodeExist(assign_ID.newNodeID) && !failedNodeReplace))
        {
          assign_ID.newNodeID = GetNextFreeNode(rxopt->isLongRangeChannel);
        }
        if (assign_ID.newNodeID)
        {
          AssignNewID(rxopt->isLongRangeChannel);
        }
        else
        {
          LearnInfoCallBack(LEARN_STATE_FAIL, 0, 0);
        }
      }
    }
    else if ((ZWAVE_CMD_SMARTSTART_PRIME_NODE_INFO == pCmd[1]) && (rxStatus & RECEIVE_STATUS_SMART_NODE))
    {
      LEARN_INFO_SMARTSTART smartstart_nodeinfo = { 0 };
      uint8_t bStatus = UPDATE_STATE_NODE_INFO_SMARTSTART_HOMEID_RECEIVED;

      // Copy the home ID in all cases.
      memcpy(&smartstart_nodeinfo.homeID[0], &rxopt->homeId.array[0], HOMEID_LENGTH);

      if (node_is_long_range(&new_node))
      {
        // LR Slave
        bStatus = UPDATE_STATE_NODE_INFO_SMARTSTART_HOMEID_RECEIVED_LR;

        smartstart_nodeinfo.nodeType.basic = node_basic_type_get(&new_node);
        smartstart_nodeinfo.nodeType.generic = node_generic_device_class_get(&new_node);
        smartstart_nodeinfo.nodeType.specific = node_specific_device_class_get(&new_node);

        smartstart_nodeinfo.nodeInfoLength = node_cc_list_get(&new_node, &smartstart_nodeinfo.nodeInfo[0]);
      }
      else
      {
        // Slave or Controller
        uint8_t cc_count = assign_ID.nodeInfoLen - offsetof(NODEINFO_FRAME, nodeInfo);
        if (NODEPARM_MAX < cc_count)
        {
          cc_count = NODEPARM_MAX;
        }

        smartstart_nodeinfo.nodeInfoLength = cc_count;
        smartstart_nodeinfo.nodeType.basic = newNodeInfoBuffer.nodeInfoFrame.nodeType.basic;
        smartstart_nodeinfo.nodeType.generic = newNodeInfoBuffer.nodeInfoFrame.nodeType.generic;
        smartstart_nodeinfo.nodeType.specific = newNodeInfoBuffer.nodeInfoFrame.nodeType.specific;

        memcpy((uint8_t*)&smartstart_nodeinfo.nodeInfo,
                (uint8_t*)&newNodeInfoBuffer.nodeInfoFrame.nodeInfo,
                cc_count);
      }

      ProtocolInterfacePassToAppNodeUpdate(bStatus,
                                            sourceNode,
                                            (uint8_t*)&smartstart_nodeinfo.homeID[0],
                                            (sizeof(LEARN_INFO_SMARTSTART) - sizeof(smartstart_nodeinfo.nodeInfo))
                                            + smartstart_nodeinfo.nodeInfoLength);
    }
    else
    {
      if (0 == (rxStatus & RECEIVE_STATUS_FOREIGN_HOMEID))
      {

      /* We got a Nodeinformation frame, but the protocol did not need it, so we just send it upstairs */
        ProtocolInterfacePassToAppNodeUpdate(
                                              UPDATE_STATE_NODE_INFO_RECEIVED,
                                              sourceNode,
                                              (uint8_t*)&newNodeInfoBuffer.nodeInfoFrame.nodeType,
                                              assign_ID.nodeInfoLen - offsetof(NODEINFO_FRAME, nodeType)
                                            );
      }
    }
  }
#if defined(ZW_CONTROLLER_STATIC)
  /* the static controller trying to get a pending node information */
  /* when the node information is received check to see if it need */
  /* routing information */
  else if ((assign_ID.assignIdState == PENDING_UPDATE_WAIT_NODE_INFO) &&
            (pendingNodeID == sourceNode))
  {
    assign_ID.nodeInfoLen = cmdLength;
    /* Received a Nodeinformation frame from other Node Convert it to new node info if it is not allready */
    ToNewNodeInfo(pCmd);
    /* TODO - Do we save the received NodeInfo??? - Only if we detect a change in it */
    /* or if cmdclasses should be saved by the protocol */
    /* We need to set assign_ID.txOptions */
    /* Should we use TRANSMIT_OPTION_AUTO_ROUTE??? */
    assign_ID.txOptions = TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_EXPLORE;
    assign_ID.newNodeID = sourceNode;
    TimerStop(&UpdateTimeoutTimer);

    ClearPendingUpdate(pendingNodeID);
    /* Here we force the change */
    /* We start with normal listening nodes */
    zensorWakeupTime = ZWAVE_SENSOR_WAKEUPTIME_NONE;
    didWrite = true;
    ZW_StoreNodeInfoFrame(pendingNodeID, (uint8_t*)&newNodeInfoBuffer.nodeInfoFrame.nodeType);

    SUCUpdateNodeInfo(pendingNodeID, SUC_ADD,
                      assign_ID.nodeInfoLen - offsetof(NODEINFO_FRAME, nodeInfo));
    if (RequestNodeRange(true) == false)
    {
      /* We did not need to get routing info from this node, return complete */
      /* to the application */
      ProtocolInterfacePassToAppNodeUpdate(
                                            UPDATE_STATE_NEW_ID_ASSIGNED,
                                            assign_ID.newNodeID,
                                            (uint8_t*)&newNodeInfoBuffer.nodeInfoFrame.nodeType,
                                            assign_ID.nodeInfoLen - offsetof(NODEINFO_FRAME, nodeType)
                                          );
      assign_ID.assignIdState = ASSIGN_IDLE;
      PendingNodeUpdate();
    }
  }
#endif
}

/*=======================   ZW_HandleCmdSmartStart   ========================
**
**    Common command handler for:
**           - ZWAVE_CMD_SMARTSTART_PRIME_NODE_INFO
**           - ZWAVE_CMD_SMARTSTART_INCLUDE_NODE_INFO
**           - ZWAVE_LR_CMD_SMARTSTART_PRIME_NODE_INFO
**           - ZWAVE_LR_CMD_SMARTSTART_INCLUDE_NODE_INFO
**
**--------------------------------------------------------------------------*/
static void ZW_HandleCmdSmartStart(uint8_t *pCmd, uint8_t cmdLength, RECEIVE_OPTIONS_TYPE *rxopt)
{
  if ( (NETWORK_WIDE_SMART_START != bNetworkWideInclusion) && (NETWORK_WIDE_SMART_START_NWI != bNetworkWideInclusion) )
  {
    /* If we receive a SMART START ARM command and controller is NOT in SMART START ARM mode */
    /* We must ignore the command as it could come from a Battery node which will not be ready */
    /* for an Inclusion attempt */
    /* Same goes for a Smart Start Include Node Info command (ZWAVE_CMD_SMARTSTART_INCLUDE_NODE_INFO) */
    return;
  }
  ZW_HandleCmdNodeInfo(pCmd, cmdLength, rxopt);
}

/*=======================   ZW_HandleCmdIncludedNodeInfo  ====================
**
**    Common command handler for:
**           - ZWAVE_CMD_INCLUDED_NODE_INFO
**           - ZWAVE_LR_CMD_INCLUDED_NODE_INFO
**
**--------------------------------------------------------------------------*/
static void ZW_HandleCmdIncludedNodeInfo(uint8_t *pCmd, uint8_t cmdLength, RECEIVE_OPTIONS_TYPE *rxopt)
{
  uint8_t rxStatus = rxopt->rxStatus;
  node_id_t sourceNode = rxopt->sourceNode;

  DPRINT("=\r\n");
  /* Only inform Application about received INIF if received on foreign HomeID.    */
  /* The information is used so a controller can now if a node it wants to connect */
  /* to is already included in a different network and therefore must be manually  */
  /* excluded from it.                                                             */
  if ( memcmp(ZW_HomeIDGet(), rxopt->homeId.array, HOMEID_LENGTH) &&
      (bNetworkWideInclusion == NETWORK_WIDE_SMART_START || bNetworkWideInclusion == NETWORK_WIDE_SMART_START_NWI) &&
       sizeof(REMOTE_INCLUDED_NODE_INFORMATION_FRAME) <= cmdLength )
  {
    /* NOTE: Protocol INIF definition is 1 byte bigger than Application INIF definiton */
    /*       and contains the two bytes CommandClass and Cmd instead of the */
    /*       one byte bINIFrxStatus in the Application INIF - else they are equal */
    /* Set CONTROLLER_UPDATE_INCLUDED_NODE_INFORMATION_FRAME.bINIFrxStatus */
    /* First remove RECEIVE_STATUS_SMART_NODE bit as it meaningless for Application */
    pCmd[1] = rxStatus & ~RECEIVE_STATUS_SMART_NODE;
    pCmd[1] |= RECEIVE_STATUS_FOREIGN_HOMEID;
    /* We got an INIF */
    ProtocolInterfacePassToAppNodeUpdate(
                                        UPDATE_STATE_INCLUDED_NODE_INFO_RECEIVED,
                                        sourceNode,
                                        (uint8_t *)&pCmd[1],
                                        sizeof(CONTROLLER_UPDATE_INCLUDED_NODE_INFORMATION_FRAME)
                                      );

  }
}

/*=======================   ZW_HandleCmdRequestNodeInfo  ====================
**
**    Common command handler for:
**           - ZWAVE_CMD_REQUEST_NODE_INFO
**           - ZWAVE_LR_CMD_REQUEST_NODE_INFO
**
**--------------------------------------------------------------------------*/
static void ZW_HandleCmdRequestNodeInfo(__attribute__((unused)) uint8_t *pCmd, __attribute__((unused)) uint8_t cmdLength, RECEIVE_OPTIONS_TYPE *rxopt)
{
  uint8_t rxStatus = rxopt->rxStatus;
  node_id_t sourceNode = rxopt->sourceNode;
#ifdef ZW_CONTROLLER_BRIDGE
  node_id_t destNode = rxopt->destNode;     /* To whom it might concern - which node is to receive the frame */
#endif

  static const STransmitCallback TxCallback = { .pCallback = NULL, .Context = 0 };
#ifdef ZW_CONTROLLER_BRIDGE
  /* If receiving destNode is NOT nodeID (controller ident) then it must be a Virtual slave ident */
  /* BROADCASTs are directed towards the controller ident as the controller ident is always present */
  if ((destNode != g_nodeID) && (destNode != NODE_BROADCAST) && (destNode != NODE_BROADCAST_LR))
  {
    ZW_SendSlaveNodeInformation(destNode, sourceNode, (TRANSMIT_OPTION_ACK |
                                                        TRANSMIT_OPTION_AUTO_ROUTE |
                                                        (rxStatus & RECEIVE_STATUS_LOW_POWER)), &TxCallback);
  }
  else
#endif
  {
    ZW_SendNodeInformation(sourceNode, (TRANSMIT_OPTION_ACK |
                                        TRANSMIT_OPTION_AUTO_ROUTE |
                                        (rxStatus & RECEIVE_STATUS_LOW_POWER)), &TxCallback);
  }
}

/* TO#1871 Fix - Static/Bridge norep controller with SUC/SIS enabled does not respond to Lost frame */
#if defined(ZW_SELF_HEAL) && defined(ZW_CONTROLLER_STATIC) && defined(ZW_REPEATER)
ZW_PROTOCOL_ADD_CMD(ZWAVE_CMD_LOST)
{
  /* TO#1296 fix - now pass ZW_RediscoveryNeeded to SIS/SUC if youre not the one */
  if (ZW_IS_SUC)
  {
    if (AreSUCAndAssignIdle())
    {
      /* Only act on it if we are idle or if request is from same ID */
      SUC_Update.updateState = SUC_UPDATE_ROUTING_SLAVE;
      /* Save Neighbors that existed before Lost */
      ZW_GetRoutingInfo(((LOST_FRAME*)pCmd)->nodeID, tempNodeLostMask, ZW_GET_ROUTING_INFO_ANY);
      lostOngoing = true;
      SetPendingDiscovery(((LOST_FRAME*)pCmd)->nodeID);
      assign_ID.newNodeID = ((LOST_FRAME*)pCmd)->nodeID;
      ZW_GetNodeProtocolInfo(assign_ID.newNodeID, (NODEINFO *)&newNodeInfoBuffer.nodeInfoFrame.capability);   /* Node info buffer */
      assign_ID.txOptions = (TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE); /* Do route! */
      zcbp_learnCompleteFunction = ZCB_NodeLostCompleted;
      g_learnNodeState = LEARN_NODE_STATE_UPDATE;
      if (rxopt->sourceNode == ((LOST_FRAME*)pCmd)->nodeID)
      {
        SaveOnlyNeighbor(((LOST_FRAME*)pCmd)->nodeID, g_nodeID);
        ZCB_CtrlAskedForHelpCallback(0, TRANSMIT_COMPLETE_OK, NULL);
      }
      else
      {
        /* Clear routing info for lost node and add source node as the only neighbor */
        SaveOnlyNeighbor(((LOST_FRAME*)pCmd)->nodeID, rxopt->sourceNode);
        /* Give the node helping out time to tell that it reached a controller */
        /* TO#1571 fix - Only start timer if SUC should not send ACCEPT_LOST */
        NetWorkTimerStart(&LostRediscoveryTimer, ZCB_StartLostRediscovery, 500);
      }
    }
  }
#ifdef ZW_REPEATER
  else
#endif
  /* TO#1871 Fix - Static/Bridge norep controller with SUC/SIS enabled does not respond to Lost frame */
#ifdef ZW_REPEATER
  {
    /* This node is not SUC or SIS - Just Pass request on */
    if ((0 != ZW_nodeIsListening()) && staticControllerNodeID)
    {
      /* Store nodeID so result can be returned */
      assign_ID.newNodeID = ((LOST_FRAME*)pCmd)->nodeID;
      /* Try to pass message to SUC or Primary controller Node */
      assignIdBuf.lostFrame.cmdClass = ZWAVE_CMD_CLASS_PROTOCOL;
      assignIdBuf.lostFrame.cmd = ZWAVE_CMD_LOST;
      assignIdBuf.lostFrame.nodeID = assign_ID.newNodeID;
      /* TODO - what to do if no room in transmitBuffer... */
      static const STransmitCallback TxCallback = { .pCallback = ZCB_CtrlAskedForHelpCallback, .Context = 0 };
      if (!EnQueueSingleData(false, g_nodeID, staticControllerNodeID,
                              (uint8_t *)&assignIdBuf.lostFrame, sizeof(LOST_FRAME),
                              (TRANSMIT_OPTION_ACK|TRANSMIT_OPTION_AUTO_ROUTE),
                              0, // 0ms for tx-delay (any value)
                              ZPAL_RADIO_TX_POWER_DEFAULT,
                              &TxCallback))
      {
        /* TransmitBuffer full - fail */
        ZCB_CtrlAskedForHelpCallback(0, TRANSMIT_COMPLETE_FAIL, NULL);
      }
    }
  }
#endif
}
#endif /* ZW_SELF_HEAL && ZW_CONTROLLER_STATIC && ZW_REPEATER */

ZW_PROTOCOL_ADD_CMD(ZWAVE_CMD_NODES_EXIST)
{
  /* TODO: seriously missing sanity checks and this command can be effectuated ANYTIME */
  assignIdBuf.nodesExistReplyFrame.cmdClass = ZWAVE_CMD_CLASS_PROTOCOL;
  assignIdBuf.nodesExistReplyFrame.cmd = ZWAVE_CMD_NODES_EXIST_REPLY;
  if (((NODES_EXIST_FRAME*)pCmd)->nodeMaskType == NODES_EXIST_TYPE_ALL)
  {
    assignIdBuf.nodesExistReplyFrame.nodeMaskType = NODES_EXIST_TYPE_ALL;
    assignIdBuf.nodesExistReplyFrame.status = NODES_EXIST_REPLY_DONE;
    node_id_t bNodeID = ((NODES_EXIST_FRAME*)pCmd)->numNodeMask << 3;
    do
    {
      if (!ZW_NodeMaskNodeIn(((NODES_EXIST_FRAME*)pCmd)->nodeMask, bNodeID))
      {
        /* If a node exists with this bNodeID then it must be removed */
        if (CtrlStorageCacheNodeExist(bNodeID))
        {
          AddNodeInfo(bNodeID, NULL, false);
          /* TO#1714 - Node update notification was not passed to app for every node removed */
          ProtocolInterfacePassToAppNodeUpdate(UPDATE_STATE_DELETE_DONE, bNodeID, NULL, 0);
        }
      }
    } while (--bNodeID);
  }
  else
  {
    assignIdBuf.nodesExistReplyFrame.status = NODES_EXIST_REPLY_UNKNOWN_TYPE;
  }
  /* TODO - what to do if no room in transmitBuffer... Nothing - no state involved */
  static const STransmitCallback TxCallback = { .pCallback = 0, .Context = 0 };
  EnQueueSingleData(false, g_nodeID, rxopt->sourceNode, (uint8_t *)&assignIdBuf.nodesExistReplyFrame,
                    sizeof(NODES_EXIST_REPLY_FRAME),
                    (TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE),
                    0, // 0ms for tx-delay (any value)
                    ZPAL_RADIO_TX_POWER_DEFAULT,
                    &TxCallback);
}

#if defined(ZW_CONTROLLER_STATIC)
ZW_PROTOCOL_ADD_CMD(ZWAVE_CMD_NODES_EXIST_REPLY)
{
  if (SUC_Update.updateState != SUC_IDLE)
  {
    TimerStop(&SUC_Update.TimeoutTimer);
    /* Did the receiver understand it? */
    if (((NODES_EXIST_REPLY_FRAME*)pCmd)->status == NODES_EXIST_REPLY_DONE)
    {
      /* Done updating - note it in the controller list */
      CtrlStorageSetCtrlSucUpdateIndex(scratchID, SUCLastIndex);
    }
    SendSUCTransferEnd(scratchID, ZWAVE_TRANSFER_UPDATE_DONE);
  }
}
#endif /* ZW_CONTROLLER_STATIC */

#ifdef ZW_GET_NODES_EXIST_ENABLED
/* Not used */
ZW_PROTOCOL_ADD_CMD(ZWAVE_CMD_GET_NODES_EXIST)
{
  static const STransmitCallback TxCallback = { .pCallback = 0, .Context = 0 };
  if (((GET_NODES_EXIST_FRAME*)pCmd)->nodeMaskType == NODES_EXIST_TYPE_ALL)
  {
    ZW_SendNodesExist(rxopt->sourceNode, &TxCallback);
  }
  else
  {
    assignIdBuf.nodesExistReplyFrame.cmdClass = ZWAVE_CMD_CLASS_PROTOCOL;
    assignIdBuf.nodesExistReplyFrame.cmd = ZWAVE_CMD_NODES_EXIST_REPLY;
    assignIdBuf.nodesExistReplyFrame.status = NODES_EXIST_REPLY_UNKNOWN_TYPE;
    EnQueueSingleData(false, g_nodeID, rxopt->sourceNode, (uint8_t *)&assignIdBuf.nodesExistReplyFrame,
                      sizeof(NODES_EXIST_REPLY_FRAME),
                      (TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE),
                      0, // 0ms for tx-delay (any value)
                      ZPAL_RADIO_TX_POWER_DEFAULT,
                      &TxCallback);
  }
}
#endif  /* ZW_GET_NODES_EXIST_ENABLED */

ZW_PROTOCOL_ADD_CMD(ZWAVE_CMD_INCLUDED_NODE_INFO)
{
  ZW_HandleCmdIncludedNodeInfo(pCmd, cmdLength, rxopt);
}

ZW_PROTOCOL_ADD_CMD(ZWAVE_CMD_SMARTSTART_PRIME_NODE_INFO)
{
  ZW_HandleCmdSmartStart(pCmd, cmdLength, rxopt);
}

ZW_PROTOCOL_ADD_CMD(ZWAVE_CMD_SMARTSTART_INCLUDE_NODE_INFO)
{
  ZW_HandleCmdSmartStart(pCmd, cmdLength, rxopt);
}

ZW_PROTOCOL_ADD_CMD(ZWAVE_CMD_EXCLUDE_REQUEST)
{
  /* Are we in Network Wide Exclusion mode */
  if (NETWORK_WIDE_MODE_EXCLUDE != bNetworkWideInclusion)
  {
    /* We have received a ZWAVE_CMD_EXCLUDE_REQUEST command but are not in Network Wide Exclusion mode */
    return;
  }
  ZW_HandleCmdNodeInfo(pCmd, cmdLength, rxopt);
}

ZW_PROTOCOL_ADD_CMD(ZWAVE_CMD_NODE_INFO)
{
  ZW_HandleCmdNodeInfo(pCmd, cmdLength, rxopt);
}

ZW_PROTOCOL_ADD_CMD(ZWAVE_CMD_CMD_COMPLETE)
{
  if ((assign_ID.assignIdState == ASSIGN_FIND_NODES_SEND) ||
      (assign_ID.assignIdState == ASSIGN_FIND_SENSOR_NODES_SEND))
  {
    /* Request range information from node */
    RequestRangeInfo();
  }
  else if (IS_WAIT_FOR_COMPLETE_BIT) /* Check that we are waiting for command complete */
  {
    /* Is received Command Complete sequence number equal to number we are waiting for? */
    if (sequenceNumber == pCmd[2])
    {
      /* Check that command complete is from the correct node */
      if ((updateNodeNeighbors) && (assign_ID.newNodeID != rxopt->sourceNode))
      {
        return; /* Ignore frame is it is from another node ID */
      }
      /* If local seq nr is equal the received seq nr transmission was OK */
      CLEAR_WAIT_FOR_COMPLETE_BIT;  /* Only reset if equal - ignore if not equal*/
      if (!IS_REPLICATION_SEND_ONGOING)
      {
        /* If we received a correct command complete frame after node/range info transfer */
        /* is done, process the next node here. Else wait until the node/range transfer callback is done. */
        ReplicationSendDoneCallback(TRANSMIT_COMPLETE_OK);
      }
    }
  }
#if defined(ZW_CONTROLLER_STATIC)
  /* TO#1066 - We only handle it as SUC if we are SUC */
  else if (ZW_IS_SUC)
  {
    if (scratchID == rxopt->sourceNode)
    {
      if  (!cmdCompleteSeen)
      {
        if (((SUC_Update.updateState == SEND_SUC_UPDATE_NODE_INFO_SEND) ||
              (SUC_Update.updateState == SEND_SUC_UPDATE_NODE_RANGE_WAIT)) &&
            (pCmd[2] == 2))
        {
          cmdCompleteSeen = true;
#ifdef ZW_NETWORK_UPDATE_REPLICATE
          /* Is it a normal update or a full? */
          if (!updateReplicationsNodeID)
#endif
          {
            /* Update sent. Circular buffer wrap around when end is reached */
            /* As SUC_MAX_UPDATES equals 64 then this can be applied - For safety we reread index from EEPROM */
            uint8_t SucControllerListEntry = (CtrlStorageGetCtrlSucUpdateIndex(scratchID) & (SUC_MAX_UPDATES - 1)) + 1;
            CtrlStorageSetCtrlSucUpdateIndex(scratchID, SucControllerListEntry);

          }
#ifdef ZW_NETWORK_UPDATE_REPLICATE
          else
          {
            /* Full update */
            do
            {
              ++updateReplicationsNodeID;
            } while ((CtrlStorageCacheNodeExist(updateReplicationsNodeID) == 0) && (updateReplicationsNodeID <= bMaxNodeID));
          }
#endif
        }
        else if (!(((SUC_Update.updateState == SEND_SUC_UPDATE_NODE_RANGE_SEND) ||
                    (SUC_Update.updateState == SEND_SUC_UPDATE_NODE_INFO_WAIT)) &&
                    ((pCmd[2] == 1) || (pCmd[2] == 3) || (pCmd[2] == 5))))
        {
          return;
        }
        /* TO#3196 fix - Do not start next SUC Update transmit before previous transmit is done. */
        if ((SUC_Update.updateState == SEND_SUC_UPDATE_NODE_RANGE_SEND) ||
            (SUC_Update.updateState == SEND_SUC_UPDATE_NODE_INFO_SEND))
        {
          GetNextSUCUpdate(false);
        }
      }
    }
  }
#endif
}

static void UpdateMultiSpeedList(const uint8_t *pList, uint8_t listLength)
{
  /* Merge pRangeFrame with previous multispeed range infos received */
  for (uint8_t i = 0; i < listLength; i++)
  {
    abMultispeedMergeNeighbors[i] |= pList[i];
  }
  if (listLength > bMultispeedArrayByteCount)
  {
    bMultispeedArrayByteCount = listLength;
  }

}
/* TODO - if the received rangeinfo is an update (not received in the assignID sequence) then it should still be acknowledged */
/* as a network update, so that if we are the SUC/SIS then other controller nodes should receive updates about this when */
/* they ask for it by transmiting the automatic controller update frame... */
ZW_PROTOCOL_ADD_CMD(ZWAVE_CMD_RANGE_INFO)
{
  if ((assign_ID.assignIdState != ASSIGN_RANGE_REQUESTED) &&
      (assign_ID.assignIdState != ASSIGN_RANGE_SENSOR_REQUESTED))
  {
    return;
  }

/* Cancel wait timer */
  AssignTimerStop();
  /* Check that the frame is long enough for our needs. */
  /* For forward compatibility, don't discard longer frames. */
  RANGEINFO_FRAME *pStruct; // Just to get the size of the maskBytes member of FIND_NODES_FRAME struct.
  /* Minimum length of a correct Range Info frame */
  uint32_t iMinLength = sizeof(RANGEINFO_FRAME) - sizeof(pStruct->maskBytes) + ((RANGEINFO_FRAME*)pCmd)->numMaskBytes
      - 1; /* zensorWakeupTime field is optional */
  if (cmdLength < iMinLength)
  {
    /* RangeInfo length mismatch! */
    if (FIND_NEIGHBOR_NODETYPE_IDLE  != findNeighborsWithNodetypeState)
    {
      ListeningNodeDiscoveryDone();
    } else
    {
      LearnInfoCallBack(LEARN_STATE_FAIL, rxopt->sourceNode, 0);
    }
    return;
  }

  /* Moved here so we also handle the noneempty Zensor Node RangeInfo situation */
  /* if we are doing ZW_RequestNodeNeighborUpdate and returned nodemask is empty -> fail */
  /* For 6.0x we do 9.6/40 findneighbors and 100 findneighbors and to determine */
  /* if no rangeinfo we need to combine the two received rangeinfos. If one rangeinfo contains nodes */
  /* then combined rangeinfo is not EMPTY. And therefor we have reinverted the test for empty rangeinfo. */
  if (updateNodeNeighbors && ZW_NodeMaskBitsIn(((RANGEINFO_FRAME*)pCmd)->maskBytes, ((RANGEINFO_FRAME*)pCmd)->numMaskBytes))
  {
    /* TO#2712 fix - For 6.0x we do 9.6/40 findneighbors and 100 findneighbors and to determine */
    /* if no rangeinfo we need to combine the two received rangeinfos. */
    bNeighborUpdateFailed = false;
    /* As rangeinfo was not empty we will save the rangeinfo and will report not failed */
  }
  if (assign_ID.assignIdState == ASSIGN_RANGE_SENSOR_REQUESTED)
  {
    /* We are waiting for ZENSOR rangeinfo and we got it */
    MergeNoneListeningSensorNodes(rxopt->sourceNode, pCmd);
  }
  else
  {
    /* Only none listening nodes knows whom THEY can SEE */
    if (CtrlStorageListeningNodeGet(rxopt->sourceNode))
    {
      /* Node is a listening node therefore set the previously neighboring nonelistening nodes in the new rangeinfo */
      MergeNoneListeningNodes(rxopt->sourceNode, pCmd);
    }
  }
  if (FIND_NEIGHBOR_NODETYPE_IDLE == findNeighborsWithNodetypeState)
  {
    UpdateMultiSpeedList(((RANGEINFO_FRAME*)pCmd)->maskBytes, ((RANGEINFO_FRAME*)pCmd)->numMaskBytes);
  }

  /* If needed, send another Find Nodes in Range frame */
  if (RequestNodeRange(false))
  {
    /* Find  Nodes frame sent, wait for reply */
    return;
  }

  if (LEARN_NODE_STATE_NEW == g_learnNodeState)
  {
    LearnInfoCallBack(LEARN_STATE_FIND_NEIGBORS_DONE, rxopt->sourceNode, 0);
  }
  uint32_t i = 0;
  /* No more range infos to request, write merged result to EEPROM */
  /* Add routing information to routing table */
  if ((FIND_NEIGHBOR_NODETYPE_IDLE == findNeighborsWithNodetypeState) && // we don't support pending discovery when doing listening node discovery by FLIRS
       (!updateNodeNeighbors ||
        !bNeighborUpdateFailed) &&
      !(i = RoutingInfoReceived(MAX_NODEMASK_LENGTH,
                                rxopt->sourceNode,
                                abMultispeedMergeNeighbors)))
  {

    /* Couldn't write to routing table, set routing to pending */
    SetPendingDiscovery(rxopt->sourceNode);
    /* Learn Complete */
    if (staticControllerNodeID && primaryController)
    {
      SetPendingUpdate(rxopt->sourceNode);
    }
#ifdef ZW_ROUTED_DISCOVERY
    LearnInfoCallBack(updateNodeNeighbors ? LEARN_STATE_FAIL : LEARN_STATE_DONE,
                      rxopt->sourceNode, 0);
    updateNodeNeighbors = false;
#else
    LearnInfoCallBack(LEARN_STATE_DONE, sourceNode, 0);
#endif

  }

  if (FIND_NEIGHBOR_NODETYPE_DONE == findNeighborsWithNodetypeState )
  {
    ListeningNodeDiscoveryDone();
    return;
  }

  assign_ID.assignIdState = ASSIGN_IDLE;

  /* Routing info received */
#if defined(ZW_CONTROLLER_STATIC)
  /* If RoutingInfo was actually written to EEPROM then i will be greater than 1 */
  didWrite = (--i);
#endif
  if (primaryController &&
#if defined(ZW_CONTROLLER_STATIC)
        /* Only if SUC are not ME... */
        ZW_IS_NOT_SUC &&
#endif
      staticControllerNodeID)
  {
    /* Prepare newRangeInfo For SUC/SIS. */
    assignIdBuf.newRangeInfo.nodeID = rxopt->sourceNode;
    /* TO#2683 fix - The New Rangeinfo registered need to contain the combined rangeinfo. */
    /* TO#2683 fix update - Remember to copy the combined Range Info to the correct place in frame. */
    assignIdBuf.newRangeInfo.numMaskBytes = bMultispeedArrayByteCount;
    memcpy((uint8_t*)&assignIdBuf.newRangeInfo.maskBytes[0],
            (uint8_t*)&abMultispeedMergeNeighbors[0],
            bMultispeedArrayByteCount);
  }
#ifdef ZW_SELF_HEAL
#if defined(ZW_CONTROLLER_STATIC)
  else
  {
    if (lostOngoing)
    {
      lostOngoing = false;
      /* Compare new nodeInfo with old one and only update if neighbors actually changed*/
      ZW_GetRoutingInfo(rxopt->sourceNode, abNeighbors, ZW_GET_ROUTING_INFO_ANY);
      if (memcmp(tempNodeLostMask, abNeighbors, MAX_NODEMASK_LENGTH) != 0)
      {
        /* TO#1244 fix - If a node requesting ZW_Rediscovery only have the node */
        /* that helps as a neighbour the new range info will NOT be distributed to */
        /* secondary controllers/Inclusion controllers from the SUC/SIS */
        /* Invalidate Rangeinfo for this node */
        didWrite = true;
        SUCUpdateNodeInfo(rxopt->sourceNode, SUC_UPDATE_RANGE, 0);
      }
    }
    else
    {
      SUCUpdateRangeInfo(rxopt->sourceNode);
    }
  }
#endif  /* defined(ZW_CONTROLLER_STATIC) */
#endif  /* ZW_SELF_HEAL */
  if (updateNodeNeighbors && bNeighborUpdateFailed)
  {
    LearnInfoCallBack(LEARN_STATE_FAIL, rxopt->sourceNode, 0);
  }
  else
  {
    /* Now we got a range info - clear pendingDiscovery */
    ClearPendingDiscovery(rxopt->sourceNode);

    /* TO#1215 fix - we got a new rangeinfo therefor success, */
    /* We check if no SUC before calling ZW_AssignReturnRoute */
    assign_ID.assignIdState = ASSIGN_SUC_ROUTES;
    if (staticControllerNodeID && (GetNodeBasicType(rxopt->sourceNode) == BASIC_TYPE_ROUTING_END_NODE))
    {
      uint8_t SUCNodeID = GetSUCNodeID();
      bool bAssignSUCWasStarted = false;
      if (SUCNodeID != 0)
      {
        static const STransmitCallback TxCallback = { .pCallback = ZCB_AssignTxComplete,.Context = 0 };
        bAssignSUCWasStarted = ZW_AssignReturnRoute(rxopt->sourceNode, SUCNodeID, NULL, true, &TxCallback);
      }

      if (!bAssignSUCWasStarted)
      {
        /* Call AssignTxComplete directly, AssignSUC was not started */
        ZCB_CallAssignTxCompleteFailed(NULL);
      }
    }
    else
    {
      ZCB_AssignTxComplete(0, TRANSMIT_COMPLETE_OK, NULL);
    }
  }
  /* if a SUC is present in the network then prepare to send */
  /* the range information to it */
}

ZW_PROTOCOL_ADD_CMD(ZWAVE_CMD_REQUEST_NODE_INFO)
{
  ZW_HandleCmdRequestNodeInfo(pCmd, cmdLength, rxopt);
}

ZW_PROTOCOL_ADD_CMD(ZWAVE_CMD_ASSIGN_IDS)
{
  /* Check if length of frame is correct */
  if (!(cmdLength >= sizeof(ASSIGN_IDS_FRAME)))
  {
    return;
  }
  /* Only listen to command if in receive replication */
  /* Lets remember to also check if were in learnMode */
  /* TO#1422 fix - test for learnMode so we do not assign a new ID while allready */
  /*               in the middle of being added to the network */
  if (NEW_CTRL_RECEIVER_BIT && g_learnMode)
  {
    /* Make sure we remove our real nodeID */
    if (spoof)
    {
      spoof = false;
      g_nodeID = CtrlStorageGetNodeID();
    }
    /* Reset any nodeInformation about the current ID */
    if (((ASSIGN_IDS_FRAME*)pCmd)->newNodeID == 0)
    {
      AddNodeInfo(g_nodeID, NULL, false);
    }
    else
    {
      /* The includie must call ZW_LockRoute to keep route all way through the inclusion */
      ZW_LockRoute(true);
      /* If we are receiving replication and we assigned a new home ID we delete */
      /* the node information stored using the old node ID, but keep CachedRoute if sourceNode equals old nodeID */
      AddNodeInfo(g_nodeID, NULL, (rxopt->sourceNode == g_nodeID));
    }
    /* Now  we are not in learnMode */
    g_learnMode = false;
#ifdef ZW_CONTROLLER_BRIDGE
    /* TO#2268 fix - Check if new homeID is foreign */
    if ((rxopt->rxStatus & RECEIVE_STATUS_FOREIGN_HOMEID) ||
        memcmp(ZW_HomeIDGet(), ((ASSIGN_IDS_FRAME*)pCmd)->newHomeID, HOMEID_LENGTH))
    {
      /* We are doing the full reset so everything is going, also our Virtual Slave Nodes */
      RemoveAllVirtualNodes();
    }
#endif
    /* set new home and node ID's */
    g_nodeID = ((ASSIGN_IDS_FRAME*)pCmd)->newNodeID;
    ZW_HomeIDSet(((ASSIGN_IDS_FRAME*)pCmd)->newHomeID);
    NetworkManagement_IsSmartStartInclusionClear();
    /* Need this for replication */
    ReplicationStart(rxopt->rxStatus, rxopt->sourceNode);

    /* If we were removed we need to add ourselves to the table */
    if (g_nodeID == 0)
    {
      SetupNodeInformation(ZWAVE_CMD_CLASS_PROTOCOL);  // Setup node info default to Z-Wave cmdClass
      AddNodeInfo(NODE_CONTROLLER, (uint8_t*)&newNodeInfoBuffer.nodeInfoFrame, false);
    }
    else
    {
      /* store the ID's */
      HomeIdUpdate(ZW_HomeIDGet(), g_nodeID);
    }
  }
#ifdef ZW_CONTROLLER_BRIDGE
  else
  {
    if (learnSlaveMode)
    {
      assignSlaveState = ASSIGN_NODEID_DONE;
      newID = ((ASSIGN_IDS_FRAME*)pCmd)->newNodeID;
      if (!newID)
      {
        if (rxopt->destNode)
        {
          /* Virtual Node now removed */
          RemoveVirtualNode(rxopt->destNode);
        }
        else
        {
          /* Cannot be... */
        }
        /* delete response routes */
      }
      else
      {
        /* set newID as Virtual Node */
        SetNewVirtualNodePresent();
      }
      ZCB_AssignSlaveComplete(0, TRANSMIT_COMPLETE_OK, NULL);
      /* report the new node ID to the application */
      virtualNodeID = newID;
    }
  }
#endif /* ZW_CONTROLLER_BRIDGE */
}

ZW_PROTOCOL_ADD_CMD(ZWAVE_CMD_TRANSFER_PRESENTATION)
{
  /* Only useful if we are in Replication Receive */
  if (NEW_CTRL_RECEIVER_BIT && g_learnMode && !g_learnModeDsk)
  {
    /* If the size of the received Transfer Presentation payload is bigger or equal to current definition (4.5x) */
    /* of the Transfer Presentation Frame or we are in learnModeClassic then answer the TransferPresentation.  */
    if ((cmdLength >= sizeof(TRANSFER_PRESENTATION_FRAME)) || g_learnModeClassic)
    {
      if ((cmdLength >= sizeof(TRANSFER_PRESENTATION_FRAME)) &&
          (((TRANSFER_PRESENTATION_FRAME*)pCmd)->option & TRANSFER_PRESENTATION_OPTION_UNIQ_HOMEID))
      {
        if (((bNetworkWideInclusion == NETWORK_WIDE_MODE_JOIN) &&
              (((TRANSFER_PRESENTATION_FRAME*)pCmd)->option & TRANSFER_PRESENTATION_OPTION_NOT_INCLUSION))
            ||
            ((bNetworkWideInclusion == NETWORK_WIDE_MODE_LEAVE) &&
              (((TRANSFER_PRESENTATION_FRAME*)pCmd)->option & TRANSFER_PRESENTATION_OPTION_NOT_EXCLUSION)))
        {
          return;
        }
        forceUse40K = true;
      }

      /* TODO - maybe do we need to check if we have received a transfer presentation already */
      /* and started acting upon it */
      /* TO#2374 Fix - moved if spoof is needed, to Replication module after ZW_ClearTables is called */
      /* TO#1446 fix - learnMode should not be set to false here. */
      /* If so an AssignID frame with 0 (remove node) will not be accepted */
      /* Fix TO1293 - By removing check of wheter this controller is primary. */
      /* Now we always accept an TRANSFER_PRESENTATION */
      PresentationReceived(rxopt->rxStatus, rxopt->sourceNode);
    }
  }
}

ZW_PROTOCOL_ADD_CMD(ZWAVE_CMD_TRANSFER_NODE_INFO)
{
  g_learnMode = false;
  UpdateMaxNodeID((node_id_t)((TRANSFER_NODE_INFO_FRAME*)pCmd)->nodeID, false);
#if defined(ZW_CONTROLLER_STATIC)
  /* If a controller then register node in the SUC controller list */
  if ((((TRANSFER_NODE_INFO_FRAME*)pCmd)->nodeInfo.security & ZWAVE_NODEINFO_CONTROLLER_NODE))
  {
    /* TO#1066 fix - Controller which is replicating should not be outdated afterwards */
    uint32_t SucControllerListIndex = rxopt->sourceNode;
    uint8_t SucControllerListEntry = SUCLastIndex;
    if (!(((TRANSFER_NODE_INFO_FRAME*)pCmd)->nodeID == rxopt->sourceNode))
    {
      SucControllerListIndex = ((TRANSFER_NODE_INFO_FRAME*)pCmd)->nodeID;
      SucControllerListEntry = ZW_IS_SUC ? SUCLastIndex : SUC_OUT_OF_DATE;
    }
    CtrlStorageSetCtrlSucUpdateIndex(SucControllerListIndex, SucControllerListEntry);
  }
#endif
  /* TO#3587 Check state before execute Transfer...*/
  if (GET_NEW_CTRL_STATE == NEW_CTRL_RECEIVE_INFO)
  {
    TransferNodeInfoReceived(cmdLength, pCmd);
    if ((0 < ((TRANSFER_NODE_INFO_FRAME*)pCmd)->nodeID) && (ZW_MAX_NODES >= ((TRANSFER_NODE_INFO_FRAME*)pCmd)->nodeID))
    {
      CtrlStorageCacheNodeInfoUpdate(((TRANSFER_NODE_INFO_FRAME*)pCmd)->nodeID,
                (0 != (((TRANSFER_NODE_INFO_FRAME*)pCmd)->nodeInfo.reserved & ZWAVE_NODEINFO_BAUD_100K)));
      if (!IsNodeRepeater(((TRANSFER_NODE_INFO_FRAME*)pCmd)->nodeID))
      {
        RoutingAddNonRepeater(((TRANSFER_NODE_INFO_FRAME*)pCmd)->nodeID);
      }
    }
  }
}

ZW_PROTOCOL_ADD_CMD(ZWAVE_CMD_TRANSFER_RANGE_INFO)
{
  /* TO#3587 Check state before execute Transfer...*/
  if (GET_NEW_CTRL_STATE == NEW_CTRL_RECEIVE_INFO)
  {
    TransferRangeInfoReceived(cmdLength, pCmd);
    if (g_nodeID == ((TRANSFER_NODE_INFO_FRAME*)pCmd)->nodeID)
    {
      RestartAnalyseRoutingTable();
    }
  }
}

ZW_PROTOCOL_ADD_CMD(ZWAVE_CMD_TRANSFER_END)
{
  /* Received a transfer end in replication. - TODO are we sure about that allways? */
  ReplicationTransferEndReceived(rxopt->sourceNode, pCmd[2]);
}

ZW_PROTOCOL_ADD_CMD(ZWAVE_CMD_RESERVED_IDS)
{
  /* TO#1249 - We need to make sure the "Reserve Node ID" frame is not still in transit */
  if ((assign_ID.assignIdState == ASSIGN_REQUESTING_ID) ||
      (assign_ID.assignIdState == ASSIGN_REQUESTED_ID))
  {
    if ((((RESERVED_IDS_FRAME*)pCmd)->numberOfIDs)&NUMBER_OF_IDS_MASK)
    {
      /* NodeID was received */
      assign_ID.newNodeID = ((RESERVED_IDS_FRAME*)pCmd)->reservedID1;
      /* Save the node ID in eeprom */
      CtrlStorageSetReservedId(assign_ID.newNodeID);
      /* TO#1249 - We need to make sure the "Reserve Node ID" frame is not still in transit */
      if (assign_ID.assignIdState == ASSIGN_REQUESTED_ID)
      {
        /* We now have a node ID, start the "old" learn process */
        assign_ID.assignIdState = ASSIGN_IDLE;
        ZW_SetLearnNodeStateNoServer(LEARN_NODE_STATE_NEW);
      }
    }
    else
    {
      /* No ID received, learn failed */
      LearnInfoCallBack(LEARN_STATE_NO_SERVER, 0, 0);
    }
  }
}

ZW_PROTOCOL_ADD_CMD(ZWAVE_CMD_TRANSFER_NEW_PRIMARY_COMPLETE)
{
  if (NEW_CTRL_RECEIVER_BIT)
  {
    newPrimaryReplication = true;
  }
}

ZW_PROTOCOL_ADD_CMD(ZWAVE_CMD_FIND_NODES_IN_RANGE)
{
  if (!IsFindNodeInRangeAllowed(rxopt->sourceNode))
  {
    return;
  }
  /* TO#3499 fix - Controller should ignore other nodes asking for it to execute a FindNodesInRange if in Replication Send */
  if (!updateNodeNeighbors && (GET_NEW_CTRL_STATE != NEW_CTRL_SEND))
  {
    g_learnMode = false;
    if ((g_findInProgress == false)
        && (cmdLength<=sizeof(FIND_NODES_FRAME)))
    {
#ifdef ZW_CONTROLLER_BRIDGE
      if (rxopt->destNode != g_nodeID)
      {
        /* Application should know that it must not abort receive mode */
        assignSlaveState = ASSIGN_RANGE_INFO_UPDATE;
        ZCB_AssignSlaveComplete(0, TRANSMIT_COMPLETE_OK, NULL);
      }
      learnSlaveMode = false; /* Now we are done with learning (if we was in learnSlaveMode!) */
#endif
      g_findInProgress = true;
      /* Copy range request to buffer */
      memcpy((uint8_t *)&nodeRange, (uint8_t *)pCmd, cmdLength);
      /* Set neighbor control vars to default values */
      zensorWakeupTime = 0;
      bNeighborDiscoverySpeed = (1 == llIsHeaderFormat3ch()) ? ZWAVE_FIND_NODES_SPEED_100K : ZWAVE_FIND_NODES_SPEED_9600;

      /* TO#1318 - fix - pCmd must be copied into nodeRange before nodeRange can be used */
      /* Do the findNeighbor frame contain zensorWakeupTime - then we are checking for sensors */
      if (cmdLength > (sizeof(FIND_NODES_FRAME) - 2*sizeof(uint8_t) - (MAX_NODEMASK_LENGTH)) + (nodeRange.frame.numMaskBytes & ZWAVE_FIND_NODES_NUMMASKBYTES_MASK))
      {
        /* The frame contains zensor wakeup speed */
        zensorWakeupTime = nodeRange.frame.maskBytes[(nodeRange.frame.numMaskBytes & ZWAVE_FIND_NODES_NUMMASKBYTES_MASK)];
      }
      if (cmdLength > (sizeof(FIND_NODES_FRAME) - sizeof(uint8_t) - (MAX_NODEMASK_LENGTH)) + (nodeRange.frame.numMaskBytes & ZWAVE_FIND_NODES_NUMMASKBYTES_MASK))
      {
        /* The frame contains neighbor discovery speed */
        bNeighborDiscoverySpeed = nodeRange.frame.maskBytes[(nodeRange.frame.numMaskBytes & ZWAVE_FIND_NODES_NUMMASKBYTES_MASK) + 1];
      }

      nodeRange.lastController = rxopt->sourceNode;
      nodeRange.txOptions = TRANSMIT_OPTION_ACK | (rxopt->rxStatus & RECEIVE_STATUS_LOW_POWER);  // RECEIVE_STATUS_LOW_POWER == TRANSMIT_OPTION_LOW_POWER
      oldSpeed = TransportGetCurrentRxSpeed();
      FindNeighbors(0);
    }
  }
}

ZW_PROTOCOL_ADD_CMD(ZWAVE_CMD_GET_NODES_IN_RANGE)
{
  /* TO#3499 fix - Controller should ignore other nodes asking for it to execute a FindNodesInRange if in Replication Send */
  if (!updateNodeNeighbors && (GET_NEW_CTRL_STATE != NEW_CTRL_SEND))
  {
    /* Set command to range info */
    nodeRange.frame.cmdClass = ZWAVE_CMD_CLASS_PROTOCOL;
    nodeRange.frame.cmd = ZWAVE_CMD_RANGE_INFO;
    /* transmit range Info frame */
    TxOptions_t txOptions = TRANSMIT_OPTION_ACK | (rxopt->rxStatus & RECEIVE_STATUS_LOW_POWER); // RECEIVE_STATUS_LOW_POWER == TRANSMIT_OPTION_LOW_POWER
    /* 0 bytes in nodemask is an invalid size */
    if (nodeRange.frame.numMaskBytes == 0)
    {
      nodeRange.frame.numMaskBytes = 1;
    }

    static const STransmitCallback TxCallback = { .pCallback = ZCB_FindNeighborComplete, .Context = 0 };
    nodeRange.frame.maskBytes[nodeRange.frame.numMaskBytes] = zensorWakeupTime;
    uint8_t len = nodeRange.frame.numMaskBytes + sizeof(RANGEINFO_FRAME) - MAX_NODEMASK_LENGTH;
    if (!EnQueueSingleData(false,g_nodeID, rxopt->sourceNode, (uint8_t *)&nodeRange,
                           len,
                           txOptions, 0, // 0ms for tx-delay (any value)
                           ZPAL_RADIO_TX_POWER_DEFAULT, &TxCallback))
    {
      ZCB_FindNeighborComplete(0, TRANSMIT_COMPLETE_NO_ACK, NULL);
    }
  }
}

ZW_PROTOCOL_ADD_CMD(ZWAVE_CMD_NEW_NODE_REGISTERED)
{
  /* TODO - what to do if we are in the middle of a "remove failing node" Do we need to do anything? */
  /* TODO - What to do with receiving of the same frame several times because com. problems - now we */
  /* register every frame as a new network change... */
  uint8_t tempNodeID = ((NEW_NODE_REGISTERED_FRAME*)pCmd)->nodeID;
  node_id_t srcNodeID = ((RECEIVE_OPTIONS_TYPE*)rxopt)->sourceNode;

  /* If nodeID is ZERO or length is too short then drop it... */
  if (tempNodeID == 0
      || cmdLength < offsetof(NEW_NODE_REGISTERED_OLD_FRAME, nodeInfo))
  {
    return;
  }
  /* Do no process NEW_NODE_REGISTERED_FRAME if not sent from a controller*/
  if (!isValidController(srcNodeID))
  {
    return;
  }
  /* Check if frame is too long (Too many command classes) */
  if (cmdLength > (sizeof(NODEINFO_FRAME) + 1))
  {
    /* Set length to max length */
    cmdLength = (sizeof(NODEINFO_FRAME) + 1);
  }

  /* if a static controller accept the frame when in assign idle statE or when asking for automatic update*/
#if defined(ZW_CONTROLLER_STATIC)
  uint8_t SucState = SUC_IDLE;
  if ((SUC_Update.updateState != SUC_IDLE))
  {
    /* if a new node frame is received while in suc update stop suc and process the frame*/
    if (ZW_IS_SUC)
    {
      SucState = SUC_Update.updateState;
      SUC_Update.updateState = SUC_IDLE;
    }
  }
  if ((SUC_Update.updateState == SUC_IDLE )||
      (SUC_Update.updateState == SUC_UPDATE_STATE_WAIT_NODE_INFO))
#else
  if (SUC_Update.updateState == SUC_UPDATE_STATE_WAIT_NODE_INFO )
#endif
  {
    /* Is the new node deleted or added - if deleted then first nodetype byte will be ZERO */
    if (((NEW_NODE_REGISTERED_OLD_FRAME*)pCmd)->nodeType)
    {
      /* add the node*/
#if defined(ZW_CONTROLLER_STATIC)
      /* We received a new node from primary. */
      /* More important so send abort to the node that was in the process of being updated */
      if (SucState != SUC_IDLE)
      {
        SendSUCTransferEnd(scratchID, ZWAVE_TRANSFER_UPDATE_ABORT);
      }
#endif
      newNodeInfoBuffer.nodeInfoFrame.nodeInfo[0] = 0xFF;
      if (!(((NEW_NODE_REGISTERED_FRAME*)pCmd)->security & ZWAVE_NODEINFO_SPECIFIC_DEVICE_TYPE))
      {
        /* If command class bytes are present then it is not possible to determine */
        /* No Specific Device Type in nodeinfo frame... */
        /* Now set the Specific Device Type support bit */
        ((NEW_NODE_REGISTERED_FRAME*)pCmd)->security |= ZWAVE_NODEINFO_SPECIFIC_DEVICE_TYPE;
        /* First copy until and including nodeType */
        memcpy((uint8_t*)&newNodeInfoBuffer.nodeInfoFrame.capability,
                (uint8_t*)&((NEW_NODE_REGISTERED_OLD_FRAME*)pCmd)->capability,
                offsetof(NEW_NODE_REGISTERED_OLD_FRAME, nodeInfo) - offsetof(NEW_NODE_REGISTERED_OLD_FRAME, capability));
        newNodeInfoBuffer.nodeInfoFrame.nodeType.generic = newNodeInfoBuffer.nodeInfoFrame.nodeType.basic;  /* Put Generic the right place */
        ZW_GetNodeTypeBasic((NODEINFO *)&newNodeInfoBuffer.nodeInfoFrame.capability); /* Set the Basic Device Type */
        newNodeInfoBuffer.nodeInfoFrame.nodeType.specific = 0; /* Set the Specific Device Type to something useful */
        /* Anything left ? */
        if (cmdLength - offsetof(NEW_NODE_REGISTERED_OLD_FRAME, nodeInfo) > 0)
        {
          /* Copy the remaining Command Class bytes */
          memcpy((uint8_t*)&newNodeInfoBuffer.nodeInfoFrame.nodeInfo[0],
                  pCmd + offsetof(NEW_NODE_REGISTERED_OLD_FRAME, nodeInfo),
                  cmdLength - offsetof(NEW_NODE_REGISTERED_OLD_FRAME, nodeInfo));
        }
        cmdLength += 2; /* Add the Basic and Specific Device classes */
      }
      else
      {
        /* First copy everything, for Controllers the first Device Type in the nodeinfo */
        /* is the Basic Device Type, for Slaves the First Device Type it is the Generic Device Type */
        memcpy((uint8_t*)&newNodeInfoBuffer.nodeInfoFrame.capability,
                (uint8_t*)&((NEW_NODE_REGISTERED_FRAME*)pCmd)->capability,
                cmdLength - offsetof(NEW_NODE_REGISTERED_FRAME,capability));
        if (!(newNodeInfoBuffer.nodeInfoFrame.security & ZWAVE_NODEINFO_CONTROLLER_NODE))
        {
          /* It is a Slave */
          ZW_GetNodeTypeBasic((NODEINFO *)&newNodeInfoBuffer.nodeInfoFrame.capability); /* Set the Basic Device Type */
          /* Copy Generic and Specfic Device Types and the Command Classes */
          memcpy((uint8_t*)&newNodeInfoBuffer.nodeInfoFrame.nodeType.generic,
                  pCmd + offsetof(NEW_NODE_REGISTERED_SLAVE_FRAME, nodeType),
                  cmdLength - offsetof(NEW_NODE_REGISTERED_SLAVE_FRAME, nodeType));
          cmdLength++;  /* One more for the Basic Device Type, which is generated */
        }
      }
      /* Clear rest */
      uint8_t i = cmdLength - offsetof(NEW_NODE_REGISTERED_FRAME, nodeInfo);
      memset(newNodeInfoBuffer.nodeInfoFrame.nodeInfo + i, 0, NODEPARM_MAX - i);
      AddNodeInfo(tempNodeID, (uint8_t*)&newNodeInfoBuffer.nodeInfoFrame, false);
#if defined(ZW_CONTROLLER_STATIC)
      SUCUpdateControllerList(rxopt->sourceNode);
#endif
      if (TimerIsActive(&newNodeAssignedUpdateDelay))
      {
        /* if we received a new newnoderegistered frame while the delay timer is active
            call the timeout functioin to notify the application about the previous node then delay
        */
        ZCB_NewNodeAssignedUpdateTimeout(NULL);
        TimerStop(&newNodeAssignedUpdateDelay);
      }
      if (SUC_Update.updateState == SUC_UPDATE_STATE_WAIT_NODE_INFO)
      {
        SUCSendCmdComp(1);
      }
      newNodeInfoBuffer.mNodeID = tempNodeID;
      newNodeInfoBuffer.cmdLength = cmdLength - offsetof(NEW_NODE_REGISTERED_FRAME, nodeType);
      NetWorkTimerStart(&newNodeAssignedUpdateDelay, ZCB_NewNodeAssignedUpdateTimeout, 500);
    }
    else
    {     /* delete a node*/
      if (CtrlStorageCacheNodeExist(tempNodeID) == 0)
      {
        if (SUC_Update.updateState == SUC_UPDATE_STATE_WAIT_NODE_INFO)
        {
          SUCSendCmdComp(5);
        }
#if defined(ZW_CONTROLLER_STATIC)
        else if (SucState != SUC_IDLE)
        {
          SUC_Update.updateState = SucState;
        }
#endif
        return;
      }
      if (SUC_Update.updateState == SUC_UPDATE_STATE_WAIT_NODE_INFO)
      {
        TimerStop(&SUC_Update.TimeoutTimer);
      }
#if defined(ZW_CONTROLLER_STATIC)
      else
      {
        SUC_Update.updateState = SUC_IDLE;  // ??? we must be SUC_IDLE allready!
      }
      if (SucState != SUC_IDLE)
      {
        SUC_Update.updateState = SUC_DELETE_NODE;
        SucState = scratchID;
        scratchID = tempNodeID;
        SendSUCTransferEnd(SucState, ZWAVE_TRANSFER_UPDATE_ABORT);
        return;
      }
#endif
      scratchID = tempNodeID;
      DeleteNodeTimeout();
#if defined(ZW_CONTROLLER_STATIC)
      SUCUpdateControllerList(rxopt->sourceNode);
#endif
    }
    ROUTING_ANALYSIS_STOP();
    if (SUC_Update.updateState == SUC_IDLE)
    {
      TimerStop(&TimerHandler);
      count = 30 + 1;
      ZCB_RestartRoutingAnalysis(NULL);
    }
  }
}


ZW_PROTOCOL_ADD_CMD(ZWAVE_CMD_NEW_RANGE_REGISTERED)
{
  /* Is nodeID not ZERO */
  if (!(((NEW_RANGE_REGISTERED_FRAME*)pCmd)->nodeID))
  {
    return;
  }
  if (!isValidController(((RECEIVE_OPTIONS_TYPE*)rxopt)->sourceNode))
  {
    return;
  }
  /* if a static controller accept the frame when in assign idle statE or when asking for automatic update*/
#if defined(ZW_CONTROLLER_STATIC)
  if ((SUC_Update.updateState == SUC_UPDATE_STATE_WAIT_NODE_INFO) ||
      (SUC_Update.updateState == SUC_IDLE) )
#else
  if (SUC_Update.updateState == SUC_UPDATE_STATE_WAIT_NODE_INFO )
#endif
  {
    /* Check for correct length of frame */
    if ((cmdLength - (sizeof(NEW_RANGE_REGISTERED_FRAME) - MAX_NODEMASK_LENGTH))
        == ((NEW_RANGE_REGISTERED_FRAME*)pCmd)->numMaskBytes)
    {
      /* Add routing information to routing table */
      memcpy((uint8_t* )&assignIdBuf.newRangeInfo.nodeID,
              (uint8_t* )&pCmd[2],sizeof(NEW_RANGE_REGISTERED_FRAME));
      RoutingInfoReceived(assignIdBuf.newRangeInfo.numMaskBytes,
                          assignIdBuf.newRangeInfo.nodeID,
                          assignIdBuf.newRangeInfo.maskBytes);
      {
#if defined(ZW_CONTROLLER_STATIC)
        didWrite = true;
        /* If RoutingInfo was actually written to EEPROM then i will be greater than 1 */
        /* Was the previous registered network change for the same nodeID and if so was it a SUC_ADD? Then */
        /* do not register a new network change -> this is not a network "update" but the rangeinfo was */
        /* received as a part of an add node sequence */
        /* TODO - what if the rangeinfo received/not received in the assign process was empty and we receive */
        /* a new rangeinfo (could be as a pendingupdate) later and before any other change has been registered */
        /* - do we have a state we could check instead to distinguish between an assign releated rangeinfo and */
        /* a neighbor update change rangeinfo, to also take care of the pending update situation */
        if (SUCUpdateRangeInfo(assignIdBuf.newRangeInfo.nodeID))
        {
          /* Only update SUC Controller List if we actually did make a new update... */
            SUCUpdateControllerList(rxopt->sourceNode);
          }
#endif
        ClearPendingDiscovery(assignIdBuf.newRangeInfo.nodeID);
      }
      ROUTING_ANALYSIS_STOP();
      if (SUC_Update.updateState == SUC_UPDATE_STATE_WAIT_NODE_INFO)
      {
        TimerStop(&SUC_Update.TimeoutTimer);
        SUCSendCmdComp(2);
      }
#if defined(ZW_CONTROLLER_STATIC)
      else if (SUC_Update.updateState == SUC_IDLE)
      {
        TimerStop(&TimerHandler);
        count = 30 + 1;
        ZCB_RestartRoutingAnalysis(NULL);
      }
#endif
    }
    // if assign_new_id timer is running stop it and  notify the application
    if (TimerIsActive(&newNodeAssignedUpdateDelay))
    {
      ZCB_NewNodeAssignedUpdateTimeout(NULL);
      TimerStop(&newNodeAssignedUpdateDelay);
    }
  }
}


ZW_PROTOCOL_ADD_CMD(ZWAVE_CMD_SUC_NODE_ID)
{
  /* TO#3564 fix - SIS controller changed its role to SUC Primary controller during a stress test */
  /* Implemented validity test for SUC Node Id frame */
  /* Only handle Command if frame size is inside valid size */
  /* - For compatibility reasons a frame 1 byte smaller must also be allowed */
  if ((cmdLength > sizeof(SUC_NODE_ID_FRAME)) || (cmdLength < sizeof(SUC_NODE_ID_FRAME) - 1))
  {
    return;
  }
  else
  {
    /* If sourceNode exist and NOT controller -> ignore frame */
    /* If ((SUC_NODE_ID_FRAME *)pCmd)->nodeID exist and NOT controller -> ignore frame */
    if ((CtrlStorageCacheNodeExist(rxopt->sourceNode)
          && CtrlStorageSlaveNodeGet(rxopt->sourceNode))
        ||
        (CtrlStorageCacheNodeExist(((SUC_NODE_ID_FRAME *)pCmd)->nodeID)
          && CtrlStorageSlaveNodeGet(((SUC_NODE_ID_FRAME *)pCmd)->nodeID)))
    {
      /* None controllers involved - ignore frame */
      return;
    }
  }
  SetStaticControllerNodeId(((SUC_NODE_ID_FRAME *)pCmd)->nodeID);
  SetNodeIDServerPresent(false);
  /* Lets see if the capability field is present in the frame */
  if (cmdLength > offsetof(SUC_NODE_ID_FRAME, SUCcapabilities))
  {
    if ((((SUC_NODE_ID_FRAME *)pCmd)->SUCcapabilities & ID_SERVER_RUNNING_BIT))
    {
      /* There exist a SIS */
      SetNodeIDServerPresent(true);
      primaryController = true;
    }
    else
    {
      primaryController = realPrimaryController;
    }
  }
#if defined(ZW_CONTROLLER_STATIC)
  /* TO#1066 - if we just received a SUCID telling we are the SUC/SIS then update sucEnabled accordingly */
  if (ZW_IS_SUC)
  {
    SetupsucEnabled();
  }
#endif  /* ZW_CONTROLLER_STATIC) */
  SaveControllerConfig();
  /* Tell Application about SUC state */
  ZCB_UpdateSUCstate(0, 0, NULL);
}

ZW_PROTOCOL_ADD_CMD(ZWAVE_CMD_SET_SUC_ACK)
{
  if (SUC_Update.updateState == SUC_SET_NODE_WAIT_ACK_FRAME)
  {
    SetNodeIDServerPresent(false);
    if (((SET_SUC_ACK_FRAME*)pCmd)->result)
    {
      uint8_t i = 0;
      if (assignIdBuf.setSUCFrame.state)
      {
        /* The static controller accepted to be SUC */
        i = staticControllerNodeID;

        SetStaticControllerNodeId(rxopt->sourceNode);
        /* First check if the frame contain the SUCcapabilities byte (pre 3.40 based nodes do not send it) */
        /* and if so do the Static Controller accept to be SIS */
        if ((cmdLength > offsetof(SET_SUC_ACK_FRAME, SUCcapabilities)) &&
            (((SET_SUC_ACK_FRAME*)pCmd)->SUCcapabilities & ID_SERVER_RUNNING_BIT))
        {
          /* The static controller accepted to be SIS */
          SetNodeIDServerPresent(true);
        }
      }
      else
      {
        /* The Static Controller accepted to stop being SUC/SIS */
        if (staticControllerNodeID == rxopt->sourceNode)
        {
          SetStaticControllerNodeId(0x00);
          /* TO#2197 fix - Reset to none SUC/SIS role in network */
          primaryController = realPrimaryController;
          SetNodeIDServerPresent(false);
        }
      }

      ZCB_UpdateSUCstate(0, 0, NULL);  /* Tell Application about SUC state */
      ZW_TransmitCallbackInvoke(&SUCCompletedFunc, ZW_SUC_SET_SUCCEEDED, NULL);
      /* TO#6522 fix - Do not transmit SUC ID to yourself... */
      if (i && (i != g_nodeID) && (i != staticControllerNodeID))
      {
        static const STransmitCallback TxCallback = { .pCallback = NULL, .Context = 0 };
        SendSUCID(i, (TRANSMIT_OPTION_AUTO_ROUTE | TRANSMIT_OPTION_ACK), &TxCallback);
      }
    }
    else
    {
      if (staticControllerNodeID == rxopt->sourceNode)
      {
        SetStaticControllerNodeId(0x00);
        /* TO#2197 fix - Reset to none SUC/SIS role in network */
        primaryController = realPrimaryController;
        SetNodeIDServerPresent(false);
        ZCB_UpdateSUCstate(0, 0, NULL);  /* Tell Application about SUC state */
      }

      ZW_TransmitCallbackInvoke(&SUCCompletedFunc, ZW_SUC_SET_FAILED, NULL);
    }
    SaveControllerConfig();
    ResetSUC();
  }
}

#ifdef ZW_CONTROLLER_STATIC
ZW_PROTOCOL_ADD_CMD(ZWAVE_CMD_SET_SUC)
{
  if (sucEnabled)
  {
    SetNodeIDServerPresent(false);
    if (((SET_SUC_FRAME*)pCmd)->state)
    {
      SetStaticControllerNodeId(g_nodeID);
      if (cmdLength > offsetof(SET_SUC_FRAME, SUCcapabilities))  /* If the capability field is in the frame */
      {
        if ((sucEnabled & ZW_SUC_FUNC_NODEID_SERVER) &&
            (((SET_SUC_FRAME*)pCmd)->SUCcapabilities & ID_SERVER_RUNNING_BIT))
        {
          /* Set primary ctrl to be updated */
          CtrlStorageSetCtrlSucUpdateIndex(rxopt->sourceNode, SUCLastIndex);
          SetNodeIDServerPresent(true);;
          primaryController = true;
        }
      }
      /* Make sure the last used node ID is correct */
      node_id_t NewLastUsedNodeId = GetMaxNodeID();
      CtrlStorageSetLastUsedNodeId(NewLastUsedNodeId);
    }
    else
    {
      SetStaticControllerNodeId(0x00);
    }
    SaveControllerConfig();
  }
  assignIdBuf.setSUCAckFrame.cmdClass = ZWAVE_CMD_CLASS_PROTOCOL;
  assignIdBuf.setSUCAckFrame.cmd = ZWAVE_CMD_SET_SUC_ACK;
  assignIdBuf.setSUCAckFrame.result = (sucEnabled) ? SUC_ENABLED_MASK : false;
  assignIdBuf.setSUCAckFrame.SUCcapabilities = isNodeIDServerPresent() ? ID_SERVER_RUNNING_BIT : 0;
  static const STransmitCallback TxCallback = { .pCallback = ZCB_UpdateSUCstate, .Context = 0 };
  /* TODO - what to do if no room in transmitBuffer... If SUC ACK isnt transmitted then */
  /* the controller asking for the static controller to be SUC/SIS, assumes that the */
  /* static controller declined... So some sort of cleanup should be done here. */
  TxOptions_t txOptions = (TxOptions_t)(TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE | (rxopt->rxStatus & RECEIVE_STATUS_LOW_POWER));
  EnQueueSingleData(false, g_nodeID, rxopt->sourceNode,(uint8_t *)&assignIdBuf,sizeof(SET_SUC_ACK_FRAME),
                    txOptions, 0, // 0ms for tx-delay (any value)
                    ZPAL_RADIO_TX_POWER_DEFAULT,
                    &TxCallback);
}

/* TO#2659 fix - If nodeId server has been disabled during Automatic controller update */
/* SUC/SIS functionality is never DISABLED - we also need to tell controllers that no SUC/SIS exists */
ZW_PROTOCOL_ADD_CMD(ZWAVE_CMD_AUTOMATIC_CONTROLLER_UPDATE_START)
{
  uint8_t resumeUpdate = false;

  /* We know that a SUC/SIS exist... */
  if (ZW_IS_SUC && (AreSUCAndAssignIdle() ||
                    (resumeUpdate = ((scratchID == rxopt->sourceNode) &&
                                      (SEND_SUC_UPDATE_NODE_INFO_SEND <= SUC_Update.updateState) &&
                                      (SEND_SUC_UPDATE_NODE_RANGE_WAIT >= SUC_Update.updateState)))))
  {
    SUC_Update.updateState = SEND_SUC_UPDATE_NODE_INFO_SEND;
    scratchID = rxopt->sourceNode;
    GetNextSUCUpdate(true);
  }
  else
  {
    /* If SUC, AssignID and Replication are idle then we can */
    /* not be SUC -> tell originator who now is SUC */
    if (AreSUCAndAssignIdle())
    {
      SUC_Update.updateState = SUC_SEND_ID;
      scratchID = rxopt->sourceNode;
      static const STransmitCallback TxCallback = { .pCallback = ZCB_SUCIDSend, .Context = 0 };
      SendSUCID(scratchID, TRANSMIT_OPTION_ACK, &TxCallback);
    }
    /* We can receive several requests from the same controller even if we */
    /* allready have started servicing so just drop them - but requests */
    /* from an other controller we answer it with a TransferEnd frame */
    /* containing ZWAVE_TRANSFER_UPDATE_WAIT */
    else if (scratchID != rxopt->sourceNode)
    {
      SendSUCTransferEnd(rxopt->sourceNode, ZWAVE_TRANSFER_UPDATE_WAIT);
    }
    else
    {
      /* Here we could maybe restart current pending update if any???*/
/*
      SUC_Update.updateState = SEND_SUC_UPDATE_NODE_INFO_SEND;
      GetNextSUCUpdate(true);
*/
    }
  }
}
/* TO#2659 fix - If nodeId server has been disabled during Automatic controller update */
/* SUC/SIS functionality is never DISABLED - we also need to tell controllers that no SUC/SIS exists */

ZW_PROTOCOL_ADD_CMD(ZWAVE_CMD_STATIC_ROUTE_REQUEST)
{
  if (cmdBufferLock)
  {
    return;
  }
  if (ZW_IS_NOT_SUC)
  {
    /* TO#5249 fix - Do not start an Assign Return Route if allready executing the functionality */
    if (staticControllerNodeID && !returnRouteAssignActive)
    {
      /* TODO - Do we really want to send the new SUC routes now and then */
      /* routes for the requested destinations without testing if any of them */
      /* have disappeared - Maybe what we want is to only transmit the */
      /* SUC routes so that the new SUC can take over with keeping track */
      /* on nodes being added/removed to/from the network and thereby */
      /* skipping the transmission of the destination routes */
      scratchID = rxopt->sourceNode;
      sendSUCRouteNow = true;
        /* TO#5249 fix - If start Assign Return Route functionality failed */
      if (ESENDROUTE_ASSIGN_RETURN_ROUTE_NOT_STARTED != SendRouteUpdate(TRANSMIT_COMPLETE_OK))
      {
        /* Assign SUC Return Routes started/handled */
        return;
      }
      sendSUCRouteNow = false;
      SendSUCTransferEnd(rxopt->sourceNode, ZWAVE_TRANSFER_UPDATE_WAIT);
    }
    else
    {
      /* No known SUC/SIS - therefor */
      /* TO#5249 fix - Do not start an Assign Return Route if allready executing the functionality */
      SendSUCTransferEnd(rxopt->sourceNode, (staticControllerNodeID && returnRouteAssignActive) ?
                                             ZWAVE_TRANSFER_UPDATE_WAIT : ZWAVE_TRANSFER_UPDATE_DISABLED);
    }
  }
  else
  {
    /* We are SUC/SIS therefor we service the request accordingly */
    /* Do not start an Assign Return Route if allready executing the functionality */
    if (AreSUCAndAssignIdle() && !returnRouteAssignActive)
    {
      scratchID = rxopt->sourceNode;
      zcbp_learnCompleteFunction = NULL;
      {
        uint8_t deleteRoute = false;
        uint8_t i = ZW_MAX_CACHED_RETURN_ROUTE_DESTINATIONS;
        do
        {
          destNodeIDList[i - 1] = 0;
        } while (--i);
        if (cmdLength > 2)
        {
          pCmd += 2;
          int32_t u = 0;
          while (u < (cmdLength - 2))
          {
            i = pCmd[u];
            if (i != 0)
            {
              /* Only do something if an actual destination exist */
              if ((CtrlStorageCacheNodeExist(i) != 0) || (i == NODE_CONTROLLER_OLD))
              {
                /* TODO - make sure that u never is ZW_MAX_CACHED_RETURN_ROUTE_DESTINATIONS or more */
                destNodeIDList[u] = i;   /* copy only nodes that exist in the network*/
              }
              else
              {
                deleteRoute = true;
              }
            }
            u++;
          }
        }

        bool bNodeIsUpdated = CtrlStorageGetRoutingSlaveSucUpdateFlag(scratchID);
        if(!bNodeIsUpdated || deleteRoute)
        {
          if (deleteRoute)  /* if one of the nodes is non existing send deletes the routes first*/
          {
            /* */
            static const STransmitCallback TxCallback = { .pCallback = ZCB_deleteRoutesDone, .Context = 0 };
            if (false == ZW_DeleteReturnRoute(scratchID, false, &TxCallback))
            {
              SendSUCTransferEnd(rxopt->sourceNode, ZWAVE_TRANSFER_UPDATE_WAIT);
            }
            else
            {
              SUC_Update.updateState = SUC_UPDATE_ROUTING_SLAVE;

              /* Start timer so we can get out of the situation if we fail to get any transmit in queue */
              StartSUCTimer();
            }
          }
          else
          {
            sendSUCRouteNow = true;
            if (ESENDROUTE_ASSIGN_RETURN_ROUTE_NOT_STARTED == SendRouteUpdate(TRANSMIT_COMPLETE_OK))
            {
              sendSUCRouteNow = false;
              SendSUCTransferEnd(rxopt->sourceNode, ZWAVE_TRANSFER_UPDATE_WAIT);
            }
            else
            {
              SUC_Update.updateState = SUC_UPDATE_ROUTING_SLAVE;

              /* Start timer so we can get out of the situation if we fail to get any transmit in queue */
              StartSUCTimer();
            }
          }
          return;
        }
      }
      SendSUCTransferEnd(rxopt->sourceNode, ZWAVE_TRANSFER_UPDATE_DONE);
    }
    else
    {
      /* We are not idle - please WAIT */
      if ((SUC_Update.updateState == SUC_UPDATE_ROUTING_SLAVE) && (scratchID == rxopt->sourceNode))
      {
        return;
      }
      SendSUCTransferEnd(rxopt->sourceNode, ZWAVE_TRANSFER_UPDATE_WAIT);
    }
  }
}

ZW_PROTOCOL_ADD_CMD(ZWAVE_CMD_RESERVE_NODE_IDS)
{
  /* Only SIS must answer to this command */
  if (!(ZW_IS_SUC && isNodeIDServerPresent()))
  {
    return;
  }
  if (!isValidController(((RECEIVE_OPTIONS_TYPE*)rxopt)->sourceNode))
  {
    return;
  }
  /* Build respones */
  assignIdBuf.reservedIDsFrame.cmdClass     = ZWAVE_CMD_CLASS_PROTOCOL;
  assignIdBuf.reservedIDsFrame.cmd          = ZWAVE_CMD_RESERVED_IDS;

  /* Get the free node IDs */
  uint32_t i;
  for (i = 0; i < (((RESERVE_NODE_IDS_FRAME*)pCmd)->numberOfIDs & NUMBER_OF_IDS_MASK); i++)
  {
    node_id_t bNodeID = GetNextFreeNode(false);
    if (bNodeID)
    {
      *(((uint8_t*)&assignIdBuf.reservedIDsFrame.reservedID1) + i) = bNodeID;
    }
    else
    {
      break;
    }
  }
  assignIdBuf.reservedIDsFrame.numberOfIDs = i;

  /* Send Frame */
  TxOptions_t txOptions = TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE;
  static const STransmitCallback TxCallback = { .pCallback = NULL, .Context = 0 };
  EnQueueSingleData(false, g_nodeID, rxopt->sourceNode, (uint8_t *)&assignIdBuf,
                        i + sizeof(RESERVE_NODE_IDS_FRAME),
                        txOptions, 0, // 0ms for tx-delay (any value)
                        ZPAL_RADIO_TX_POWER_DEFAULT, &TxCallback);
}
#endif /* ZW_CONTROLLER_STATIC*/

/* TO#1928 - must be enabled for ZW_REPEATER targets */
#ifdef ZW_REPEATER
ZW_PROTOCOL_ADD_CMD(ZWAVE_CMD_SET_NWI_MODE)
{
  /* TODO - maybe something should be done with the FLiRS nodes here */
  if (NETWORK_WIDE_MODE_IDLE == bNetworkWideInclusion) {
    ExploreSetNWI(((SET_NWI_MODE_FRAME*)pCmd)->mode);
  }
}
#endif

ZW_PROTOCOL_ADD_CMD_LR(ZWAVE_LR_CMD_NOP)
{
  /* NOP command. Nothing to do */
}

ZW_PROTOCOL_ADD_CMD_LR(ZWAVE_LR_CMD_ASSIGN_IDS)
{
#ifdef ZW_CONTROLLER_TEST_LIB
  /*Only handle LR assign ID on the controller test lib*/
  /* Check if length of frame is correct */
  if (!(cmdLength >= sizeof(ASSIGN_IDS_FRAME_LR)))
  {
    return;
  }
  /* Only listen to command if in receive replication */
  /* Lets remember to also check if were in learnMode */
  /* test for learnMode so we do not assign a new ID while allready */
  /* in the middle of being added to the network */
  if (NEW_CTRL_RECEIVER_BIT && g_learnMode)
  {
    /* Make sure we remove our real nodeID */
    if (spoof)
    {
      spoof = false;
      g_nodeID = CtrlStorageGetNodeID();
    }
    /* The includie must call ZW_LockRoute to keep route all way through the inclusion */
    ZW_LockRoute(true);
    /* If we are receiving replication and we assigned a new home ID we delete */
    /* the node information stored using the old node ID, but keep CachedRoute if sourceNode equals old nodeID */
    AddNodeInfo(g_nodeID, NULL, (rxopt->sourceNode == g_nodeID));
    /* Now  we are not in learnMode */
    g_learnMode = false;
    /* set new home and node ID's */
    g_nodeID = ((ASSIGN_IDS_FRAME_LR*)pCmd)->newNodeID[0] << 8 | ((ASSIGN_IDS_FRAME_LR*)pCmd)->newNodeID[1];
    ZW_HomeIDSet(((ASSIGN_IDS_FRAME_LR*)pCmd)->newHomeID);
    NetworkManagement_IsSmartStartInclusionClear();
    /* If we were removed we need to add ourselves to the table */
    /* store the ID's */
    HomeIdUpdate(ZW_HomeIDGet(), g_nodeID);
  }
#endif // A controller cannot be added to a LR network. Hence, ignore LR Assign IDs.
}

ZW_PROTOCOL_ADD_CMD_LR(ZWAVE_LR_CMD_NODE_INFO)
{
  ZW_HandleCmdNodeInfo(pCmd, cmdLength, rxopt);
}

ZW_PROTOCOL_ADD_CMD_LR(ZWAVE_LR_CMD_EXCLUDE_REQUEST)
{
  node_id_t sourceNode = rxopt->sourceNode;

  /* Check if frame is to short then drop it... */
  if (cmdLength < sizeof(EXCLUDE_REQUEST_LR_FRAME))
  {
    return;
  }

  if (g_learnNodeState != LEARN_NODE_STATE_DELETE)
  {
    return; /* We are not in node exclusion state */
  }

  /* We use the assignIdBuf for the Exclude Request Confirmation command. It has the same structure */
  assignIdBuf.assignIDFrameLR.cmdClass     = ZWAVE_CMD_CLASS_PROTOCOL_LR;
  assignIdBuf.assignIDFrameLR.cmd          = ZWAVE_LR_CMD_EXCLUDE_REQUEST_CONFIRM;
  assignIdBuf.assignIDFrameLR.newNodeID[0] = sourceNode >> 8;   // MSB
  assignIdBuf.assignIDFrameLR.newNodeID[1] = sourceNode & 0xFF; // LSB

  /* Copy our Home ID to the Exclude Request Confirmation frame */
  memcpy(assignIdBuf.assignIDFrameLR.newHomeID, ZW_HomeIDGet(), HOMEID_LENGTH);

  static const STransmitCallback TxCallback = { .pCallback = ZCB_ExcludeRequestConfirmTxComplete, .Context = 0 };

  /* Send the Exclude Request Confirm frame to the leaving node */
  if (!EnQueueSingleData(RF_SPEED_LR_100K,
                         g_nodeID,
                         sourceNode,
                         (uint8_t*)&assignIdBuf.assignIDFrameLR,
                         sizeof(ASSIGN_IDS_FRAME_LR),
                         (TRANSMIT_OPTION_LR_FORCE | TRANSMIT_OPTION_ACK),
                         0, // 0ms for tx-delay (any value)
                         ZPAL_RADIO_TX_POWER_REDUCED,
                         &TxCallback))

  {
    /* TX of the Exclude Request Confirmation command failed. Unusual situation. */
    /* Just return and let the leaving node timeout and retry... */
    DPRINT("\r\nExclude Request Confirm Tx failure!");
    return;
  }

   /* Inform the application that the exclusion process has started */
  LearnInfoCallBack(LEARN_STATE_NODE_FOUND, 0, 0);

  /* Utilize the existing assign_ID variable for the housekeeping data */
  assign_ID.assignIdState = EXCLUDE_REQUEST_CONFIRM_SEND;
  assign_ID.sourceID = sourceNode;
  assign_ID.rxOptions = rxopt->rxStatus;
}
#ifdef ZW_CONTROLLER_TEST_LIB
extern void
ExclusionDoneLR(node_id_t bSource);
#endif
ZW_PROTOCOL_ADD_CMD_LR(ZWAVE_LR_CMD_EXCLUDE_REQUEST_CONFIRM)
{
#ifdef ZW_CONTROLLER_TEST_LIB
  /*Only handle LR assign ID on the controller test lib*/
  /* Check if length of frame is correct */
  if (!(cmdLength >= sizeof(ASSIGN_IDS_FRAME_LR)))
  {
    return;
  }
  /* Only listen to command if in receive replication */
  /* Lets remember to also check if were in learnMode */
  if (NEW_CTRL_RECEIVER_BIT && g_learnMode)
  {
    /* Reset any nodeInformation about the current ID */
    AddNodeInfo(g_nodeID, NULL, false);
  /* Clear our home and node ID's */
    ZW_HomeIDClear();
    /* If we were removed we need to add ourselves to the table */
    SetupNodeInformation(ZWAVE_CMD_CLASS_PROTOCOL);  // Setup node info default to Z-Wave cmdClass
    g_nodeID = NODE_CONTROLLER;
    AddNodeInfo(NODE_CONTROLLER, (uint8_t*)&newNodeInfoBuffer.nodeInfoFrame, false);
    ExclusionDoneLR(rxopt->sourceNode);
    g_nodeID = 0;
    ReplicationTransferEndReceived(otherController, ZWAVE_TRANSFER_OK);

  }
#endif // A controller cannot be added to a LR network. Hence, ignore LR Assign IDs
}

ZW_PROTOCOL_ADD_CMD_LR(ZWAVE_LR_CMD_REQUEST_NODE_INFO)
{
  ZW_HandleCmdRequestNodeInfo(pCmd, cmdLength, rxopt);
}

ZW_PROTOCOL_ADD_CMD_LR(ZWAVE_LR_CMD_NON_SECURE_INCLUSION_COMPLETE)
{
  /* Non-secure inclusion completed. Nothing to do */
}

ZW_PROTOCOL_ADD_CMD_LR(ZWAVE_LR_CMD_INCLUDED_NODE_INFO)
{
  ZW_HandleCmdIncludedNodeInfo(pCmd, cmdLength, rxopt);
}

static void ZW_HandleCmdSmartStartLR(uint8_t *pCmd, uint8_t cmdLength, RECEIVE_OPTIONS_TYPE *rxopt)
{
   /*Only handle SmartStart*/
  /*
    * We can use NODEINFO_FRAME because Smart Start Prime and Include match the Node
    * Information Frame in structure.
    */
  NODEINFO_FRAME * const pNodeInfo = (NODEINFO_FRAME *)pCmd;
  if (pNodeInfo->security & ZWAVE_NODEINFO_CONTROLLER_NODE)
  {
    /*
      * Always ignore Smart Start Prime and Include if transmitter is a controller since we do
      * not support Smart Start inclusion of controllers on LR.
      */
    return;
  }
  ZW_HandleCmdSmartStart(pCmd, cmdLength, rxopt);
}

ZW_PROTOCOL_ADD_CMD_LR(ZWAVE_LR_CMD_SMARTSTART_PRIME_NODE_INFO)
{
  ZW_HandleCmdSmartStartLR(pCmd, cmdLength, rxopt);
}

ZW_PROTOCOL_ADD_CMD_LR(ZWAVE_LR_CMD_SMARTSTART_INCLUDE_NODE_INFO)
{
  ZW_HandleCmdSmartStartLR(pCmd, cmdLength, rxopt);
}

void CommandHandler(CommandHandler_arg_t * pArgs)
{
  uint8_t * pCmd = pArgs->cmd;
  uint8_t cmdLength = pArgs->cmdLength;
  RECEIVE_OPTIONS_TYPE * rxopt = pArgs->rxOpt;

  uint8_t rxStatus;     /* Frame header info */
  rxStatus = rxopt->rxStatus;

#ifdef DEBUGPRINT
  DPRINTF("CHandler(%02X) %02X", cmdLength, pCmd[0]);
#endif

  /* If TxComplete then goto Rx mode */
#ifdef ZW_CONTROLLER_BRIDGE
  /* If we are in findNeighbors then we do not want to handle any frames */
  if (g_findInProgress)
  {
    return;
  }
#endif

  switch (*pCmd)
  {
    case ZWAVE_CMD_CLASS_PROTOCOL:
      if ( !(rxStatus & RECEIVE_STATUS_FOREIGN_FRAME) &&
          (false == rxopt->isLongRangeChannel))
      {
        /* Z-Wave protocol command */
        zw_protocol_invoke_cmd_handler(pCmd, cmdLength, rxopt);
      }
      break;

    case ZWAVE_CMD_CLASS_PROTOCOL_LR:
      /* Z-Wave Long Range protocol command */
      if (true == rxopt->isLongRangeChannel)
      {
        zw_protocol_invoke_cmd_handler_lr(pCmd, cmdLength, rxopt);
      }
      break;

    default:
      if (IS_APPL_CLASS(*pCmd) &&
          cmdLength <= MAX_SINGLECAST_PAYLOAD &&
          0 == (rxStatus & RECEIVE_STATUS_FOREIGN_HOMEID))
      {
          /* TO#3627 fix - Controller in the process of including/excluding receives Application */
          /* frames from foreign network, which are directed to the Controller nodeID */
          /* Application layer command */
          /* Copy sequence number if we received a application command class */
          sequenceNumber = pCmd[2];
#ifdef ZW_CONTROLLER_BRIDGE
          uint8_t * multi = pArgs->multi;
          if( NULL != multi)
          {
            ProtocolInterfacePassToAppMultiFrame(
                                                ((SMultiCastNodeMaskHeader*)multi)->iNodeMaskOffset,
                                                ((SMultiCastNodeMaskHeader*)multi)->iNodemaskLength,
                                                &multi[1],
                                                (ZW_APPLICATION_TX_BUFFER *)pCmd,
                                                cmdLength,
                                                rxopt
            );
          }
          else
          {
            uint8_t multi_dummy = 0;
            ProtocolInterfacePassToAppMultiFrame(
                                                0,
                                                0,
                                                &multi_dummy,
                                                (ZW_APPLICATION_TX_BUFFER *)pCmd,
                                                cmdLength,
                                                rxopt
            );
          }
#else
          ProtocolInterfacePassToAppSingleFrame(
                                                cmdLength,
                                                (ZW_APPLICATION_TX_BUFFER *)pCmd,
                                                rxopt
                                               );
#endif /*ZW_CONTROLLER_BRIDGE*/
          /* TO#3627 fix - Controller in the process of including/excluding receives Application */
          /* frames from foreign network, which are directed to the Controller nodeID */
      }
      break;
  }
}

/*===========================   GetNodeType   ===============================
**
**    Get the generic type of the specified node, if the node is not
**    registered in the controller 0 is returned.
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
uint8_t              /* RET  Type of node, or 0 if not registred */
ZCB_GetNodeType(
  node_id_t nodeID)   /* IN   Node id */
{
  if ( !nodeID ||
      ((nodeID > ZW_MAX_NODES) && (nodeID < LOWEST_LONG_RANGE_NODE_ID)) ||
      (nodeID > HIGHEST_LONG_RANGE_NODE_ID))
  {
    return 0;
  }
  if ((currentCachedNodeInfoNodeID != nodeID) || (0 == tNodeInfo.generic))
  {
    currentCachedNodeInfoNodeID = nodeID;
    CtrlStorageGetNodeInfo(nodeID, &tNodeInfo);
  }
  uint8_t GenericNodeType = tNodeInfo.generic;
  return GenericNodeType;
}


/*=======================   GetNodeSpecificType   ============================
**
**    Get the specific device type of the specified node
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
uint8_t                  /* RET  Specific Device Type of node */
GetNodeSpecificType(
  node_id_t nodeID)       /* IN   Node id */
{
  if ( !nodeID ||
      ((nodeID > ZW_MAX_NODES) && (nodeID < LOWEST_LONG_RANGE_NODE_ID)) ||
      (nodeID > HIGHEST_LONG_RANGE_NODE_ID))
  {
    return 0;
  }
  if ((currentCachedNodeInfoNodeID != nodeID) || (0 == tNodeInfo.generic))
  {
    currentCachedNodeInfoNodeID = nodeID;
    CtrlStorageGetNodeInfo(nodeID, &tNodeInfo);
  }
  uint8_t SpecificNodeType = tNodeInfo.specific;
  return SpecificNodeType;
}


/*============================   GetNodeBasicType   ======================
**    Function description
**      Gets the BASIC_TYPE of a node
**    Side effects:
**
**--------------------------------------------------------------------------*/
static uint8_t
GetNodeBasicType(
  node_id_t nodeID)
{
  uint8_t retVal = 0;
  if ( !nodeID ||
      ((nodeID > ZW_MAX_NODES) && (nodeID < LOWEST_LONG_RANGE_NODE_ID)) ||
      (nodeID > HIGHEST_LONG_RANGE_NODE_ID))
  {
    return retVal;
  }

  if (!CtrlStorageSlaveNodeGet(nodeID))
  {
    /* It is a Controller! */
    /* TODO - Can we not be more precis in determining the controller library/basicdevice type */
    if (CtrlStorageListeningNodeGet(nodeID))
    {
      retVal = BASIC_TYPE_STATIC_CONTROLLER;
    }
    else
    {
      retVal = BASIC_TYPE_CONTROLLER;
    }
  }
  else
  {
    /* It is a Slave! */
    if ((GetNodesSecurity(nodeID) & ZWAVE_NODEINFO_SLAVE_ROUTING))
    {
      retVal = BASIC_TYPE_ROUTING_END_NODE;
    }
    else
    {
      retVal = BASIC_TYPE_END_NODE;
    }
  }

  return (retVal);
}


/*========================   GetNodeCapabilities  ==========================
**
**    Get the node capability byte
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
uint8_t                  /*RET Node capability */
GetNodeCapabilities(
  node_id_t nodeID)       /* IN Node id */
{
  /* Primary controllers capabilities are not listed in */
  /* nodeinfo table. We assume not listening and not repeater!*/
  if ( !nodeID ||
      ((nodeID > ZW_MAX_NODES) && (nodeID < LOWEST_LONG_RANGE_NODE_ID)) ||
      (nodeID > HIGHEST_LONG_RANGE_NODE_ID))
  {
    return 0;
  }
  if ((currentCachedNodeInfoNodeID != nodeID) || (0 == tNodeInfo.generic))
  {
    currentCachedNodeInfoNodeID = nodeID;
    CtrlStorageGetNodeInfo(nodeID, &tNodeInfo);
  }
  /* All others we can look up in nodeinfo table */
  uint8_t NodeCapability = tNodeInfo.capability;
  return NodeCapability;
}

/*========================   GetExtendedNodeCapabilities  ==========================
**
**    Get the extended node capability byte (was previously reserved)
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
uint8_t                  /*RET Node capability */
GetExtendedNodeCapabilities(
  node_id_t nodeID)       /* IN Node id */
{
  /* Primary controllers capabilities are not listed in */
  /* nodeinfo table. We assume not listening and not repeater!*/
  if ( !nodeID ||
      ((nodeID > ZW_MAX_NODES) && (nodeID < LOWEST_LONG_RANGE_NODE_ID)) ||
      (nodeID > HIGHEST_LONG_RANGE_NODE_ID))
  {
    return 0;
  }
  if ((currentCachedNodeInfoNodeID != nodeID) || (0 == tNodeInfo.generic))
  {
    currentCachedNodeInfoNodeID = nodeID;
    CtrlStorageGetNodeInfo(nodeID, &tNodeInfo);
  }
  /* All others we can look up in nodeinfo table */
  uint8_t ExtendedNodeCapability = tNodeInfo.reserved;
  return ExtendedNodeCapability;
}


/*==========================   GetNodeSecurity  =============================
**
**    Get the node security byte
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
uint8_t              /*RET Node security */
GetNodesSecurity(
  node_id_t nodeID)   /* IN Node id */
{
  if ( !nodeID ||
      ((nodeID > ZW_MAX_NODES) && (nodeID < LOWEST_LONG_RANGE_NODE_ID)) ||
      (nodeID > HIGHEST_LONG_RANGE_NODE_ID))
  {
    return 0;
  }
  if ((currentCachedNodeInfoNodeID != nodeID) || (0 == tNodeInfo.generic))
  {
    currentCachedNodeInfoNodeID = nodeID;
    CtrlStorageGetNodeInfo(nodeID, &tNodeInfo);
  }
  uint8_t NodeSecurity = tNodeInfo.security;
  return NodeSecurity;
}


/*===========================   IsValidController  ==============================
**
**    Check if a node is a controller
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
bool
isValidController(
  node_id_t controllerID)
{
  return (GetNodesSecurity(controllerID) & ZWAVE_NODEINFO_CONTROLLER_NODE);
}

/*===========================   IsNodeSensor  ==============================
**
**    Check if a node is a sensor node
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
uint8_t
IsNodeSensor(
  node_id_t bSensorNodeID,
  bool bRetSensorMode,
  bool bCheckAssignState)
{
  /* Don't beam during assign ID */
  if ((bCheckAssignState) &&
      (assign_ID.assignIdState > ASSIGN_IDLE) &&
      (assign_ID.assignIdState < ASSIGN_LOCK) &&
      (g_learnNodeState != LEARN_NODE_STATE_UPDATE))
  {
    return 0;
  }
  else
  {
    if (CtrlStorageCacheNodeExist(bSensorNodeID))
    {
      // Do we need the nodeinfo sensor mode bits or just if it is a sensor or not
      if (false == bRetSensorMode)
      {
        return CtrlStorageSensorNodeGet(bSensorNodeID);
      }
      return (GetNodesSecurity(bSensorNodeID) & ZWAVE_NODEINFO_SENSOR_MODE_MASK);
    }
  }
  return 0;
}


/*=========================   GetNextFreeNode   =============================
**
**    Get the type of the specified node, if the node isn't registred in
**    the controller 0 is returned.
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
static node_id_t             /*RET  Next free node id, or 0 if no free */
GetNextFreeNode(bool isLongRangeChannel) /* IN   Nothing  */
{

  if (true == isLongRangeChannel)
  {
    uint32_t bNode = CtrlStorageGetLastUsedLongRangeNodeId();
    if (bNode == HIGHEST_LONG_RANGE_NODE_ID)
    {
      bNode = LOWEST_LONG_RANGE_NODE_ID;
    }
    else
    {
      bNode++;
    }
    uint32_t cntie = HIGHEST_LONG_RANGE_NODE_ID;
    do
    {
      if (CtrlStorageCacheNodeExist(bNode) == 0)
      {
        CtrlStorageSetLastUsedLongRangeNodeId(bNode);
        return (bNode);
      }
      if (++bNode > HIGHEST_LONG_RANGE_NODE_ID)
      {
        bNode = LOWEST_LONG_RANGE_NODE_ID;
      }
    } while (--cntie >= LOWEST_LONG_RANGE_NODE_ID);
  }
  else
  {
    uint32_t bNode = CtrlStorageGetLastUsedNodeId();
    if (bNode == ZW_MAX_NODES)
    {
      bNode = 0;
    }

    bNode++;
    uint32_t cntie = ZW_MAX_NODES;
    do
    {
      if (CtrlStorageCacheNodeExist(bNode) == 0)
      {
        CtrlStorageSetLastUsedNodeId(bNode);
        return (bNode);
      }
      if (++bNode > ZW_MAX_NODES)
      {
        bNode = 1;
      }
    } while (--cntie);
  }
  return (0);
}


/*===========================   GetMaxNode   ================================
**
**    Get the max node ID currently in this network
**
**    Side effects:
**      None
**
**--------------------------------------------------------------------------*/
node_id_t
GetMaxNode(node_id_t maxNodeID)
{
  do
  {
    if (CtrlStorageCacheNodeExist(maxNodeID))  /* Any node here ? */
    {
      break;
    }
  } while (--maxNodeID);
  return maxNodeID;
}


/*=========================   UpdateMaxNodeID   ==============================
**
**    Update the max node ID currently in this network
**
**    Side effects:
**      bMaxNodeID is updated accordingly
**
**--------------------------------------------------------------------------*/
static void       /*RET Nothing */
UpdateMaxNodeID(
  node_id_t wNodeID,   /* IN Node ID on node causing the update */
  uint8_t      deleted)   /* IN true if node has been removed from network, false if added */
{

  if ((ZW_MAX_NODES >= wNodeID) &&
      (wNodeID >= bMaxNodeID))
  { // ZWave node
    bMaxNodeID = wNodeID;
    if (deleted)
    {
      // Max Node ID in the network must not be less than controller's minimum.
      if (bMaxNodeID > MIN_CONTROLLER_NODE_ID)
      {
        bMaxNodeID--;
        bMaxNodeID = GetMaxNode(bMaxNodeID);
      }
    }
    CtrlStorageSetMaxNodeId(bMaxNodeID);
  }
  else if ((wNodeID >= g_wMaxNodeID_LR) &&
           (LOWEST_LONG_RANGE_NODE_ID <= wNodeID) &&
           (HIGHEST_LONG_RANGE_NODE_ID >= wNodeID))
  { // ZWave LR node
    g_wMaxNodeID_LR = wNodeID;
    if (deleted)
    {
      g_wMaxNodeID_LR--;
      g_wMaxNodeID_LR = GetMaxNode(g_wMaxNodeID_LR);
      /* Make sure the returned node ID is in the LR range. If not, return 0 to indicate there are no LR nodes IDs assigned */
      if (LOWEST_LONG_RANGE_NODE_ID > g_wMaxNodeID_LR )
      {
        g_wMaxNodeID_LR = 0;
      }
    }
    CtrlStorageSetMaxLongRangeNodeId(g_wMaxNodeID_LR);
  }
}


/*=========================   GetMaxNodeID   =============================
**
**    Get the max node ID used in this controller
**
**    Side effects:
**      bMaxNodeID is updated accordingly
**
**--------------------------------------------------------------------------*/
node_id_t      /* RET  Next free node id, or 0 if no free */
GetMaxNodeID( void )               /* IN   Nothing  */
{
  bMaxNodeID = GetMaxNode(ZW_MAX_NODES);
  return bMaxNodeID;
}

/*=========================   GetMaxNodeID   =============================
**
**    Get the max node ID used in this controller
**
**    Side effects:
**      bMaxNodeID is updated accordingly
**
**--------------------------------------------------------------------------*/
node_id_t      /* RET  Th max LR node ID assigned, or 0 if no LR node ID is assigned */
GetMaxNodeID_LR( void )               /* IN   Nothing  */
{
  g_wMaxNodeID_LR = GetMaxNode(HIGHEST_LONG_RANGE_NODE_ID);
  if (LOWEST_LONG_RANGE_NODE_ID > g_wMaxNodeID_LR )
  {
    g_wMaxNodeID_LR = 0; // Means there are no LR node ID's assigned
  }
  return g_wMaxNodeID_LR;

}

/*=========================   RequestRangeInfo   ============================
**
**    Sends out request range info frame
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
void
RequestRangeInfo(void) /* RET  Nothing */
{
  /*Stop any running assign timer.*/
  AssignTimerStop();
  /* Build get range info frame */
  assignIdBuf.getNodesFrame.cmdClass = ZWAVE_CMD_CLASS_PROTOCOL;
  assignIdBuf.getNodesFrame.cmd = ZWAVE_CMD_GET_NODES_IN_RANGE;
  if (FIND_NEIGHBOR_NODETYPE_STARTED == findNeighborsWithNodetypeState)
  {
    assignIdBuf.getNodesFrame.zensorWakeupTime = 0;
    assign_ID.assignIdState = ASSIGN_RANGE_REQUESTED;
  }
  else
  {
    /* Set state to request send */
    if (bNeighborDiscoverySpeed == ZW_NEIGHBOR_DISCOVERY_SPEED_BEAM)
    {
      zensorWakeupTime = ZWAVE_SENSOR_WAKEUPTIME_1000MS;
    }
    assignIdBuf.getNodesFrame.zensorWakeupTime = zensorWakeupTime;
    if (zensorWakeupTime)
    {
      assign_ID.assignIdState = ASSIGN_RANGE_SENSOR_REQUESTED;
    }
    else
    {
      assign_ID.assignIdState = ASSIGN_RANGE_REQUESTED;
    }
  }

  /* TO#4161 - Make sure EXPLORE frame is used if needed */
  if (assign_ID.txOptions & TRANSMIT_OPTION_EXPLORE)
  {
    /* Note that we want Explore tried as route resolution if all else fails  */
    bUseExploreAsRouteResolution = true;
  }

  /* Send frame to node */
  /* TODO - what to do if no room in transmitBuffer... */
  static const STransmitCallback TxCallback = { .pCallback = ZCB_AssignTxComplete, .Context = 0 };
  bNeighborDiscoverySpeed++;
  if ((ZW_NEIGHBOR_DISCOVERY_SPEED_BEAM == bNeighborDiscoverySpeed ) && (FIND_NEIGHBOR_NODETYPE_STARTED == findNeighborsWithNodetypeState))
  {
    bNeighborDiscoverySpeed = ZW_NEIGHBOR_DISCOVERY_SPEED_END;
  }

  /* TO#4161 - Make sure EXPLORE frame is used if needed */
  if (!EnQueueSingleData(false, g_nodeID, assign_ID.newNodeID, (uint8_t *)&assignIdBuf,
                              sizeof(GET_NODE_RANGE_FRAME) - (!zensorWakeupTime ? 1 : 0),
                              ~TRANSMIT_OPTION_EXPLORE & assign_ID.txOptions, 0, // 0ms for tx-delay (any value)
                              ZPAL_RADIO_TX_POWER_DEFAULT, &TxCallback))

  {
    ZCB_CallAssignTxCompleteFailed(NULL);
  }
}


#ifdef ZW_ROUTED_DISCOVERY

static
bool ListeningNodeDiscoverByFlirs (void)
{
  while(current_flirs_nodeid <= ZW_MAX_NODES)
  {
    if (IsNodeSensor(current_flirs_nodeid, false, false) && (current_flirs_nodeid != g_nodeID))
    {
      findNeighborsWithNodetypeState = FIND_NEIGHBOR_NODETYPE_STARTED;
      assign_ID.newNodeID = current_flirs_nodeid;
      ZW_GetNodeProtocolInfo(current_flirs_nodeid, (NODEINFO *)&newNodeInfoBuffer.nodeInfoFrame.capability); /* Node info buffer */
      /* For 6.0x we do 9.6/40 findneighbors and 100 findneighbors and to determine */
      /* if no rangeinfo we need to combine the two received rangeinfos. */
      /* We assume NeighborUpdate fails. */
      bNeighborUpdateFailed = true;
      updateNodeNeighbors = true;
      /* We start with normal listening nodes */
      zensorWakeupTime = ZWAVE_SENSOR_WAKEUPTIME_NONE;
      updateNodeNeighbors = RequestNodeRange(true);
      current_flirs_nodeid++;
      break;
    }
    current_flirs_nodeid++;
  }

  if (current_flirs_nodeid > ZW_MAX_NODES)
  {
    if (0 !=ZW_NodeMaskBitsIn(listning_node_neighbors,MAX_NODEMASK_LENGTH))
    {
      uint8_t tmp_node_list[MAX_NODEMASK_LENGTH] = { 0 };
      ZW_GetRoutingInfo(m_listning_nodeid,
                        tmp_node_list,
                        (ZW_GET_ROUTING_INFO_ANY|GET_ROUTING_INFO_REMOVE_NON_REPS));
      for (uint8_t i = 0; i < MAX_NODEMASK_LENGTH; i++)
      {
        listning_node_neighbors[i] |= tmp_node_list[i];
      }
      if (0 == RoutingInfoReceived(MAX_NODEMASK_LENGTH,
                                   (uint8_t)m_listning_nodeid,
                                   listning_node_neighbors))
      {
        bNeighborUpdateFailed = true;
      }
    }

    current_flirs_nodeid = 2;
    g_learnNodeState = LEARN_NODE_STATE_OFF;
    findNeighborsWithNodetypeState = FIND_NEIGHBOR_NODETYPE_IDLE;
    m_listning_nodeid = 0;
    ZW_TransmitCallbackInvoke(&NodeTypeNeighborCallback,
                              (bNeighborUpdateFailed? REQUEST_NEIGHBOR_UPDATE_FAILED:REQUEST_NEIGHBOR_UPDATE_DONE),
                              NULL);
    ZW_TransmitCallbackUnBind(&NodeTypeNeighborCallback);
    zcbp_learnCompleteFunction = NULL;
  }
  return updateNodeNeighbors;
}

static void ListeningNodeDiscoveryDone (void)
{
      /* Remove last working route to avoid incorrect speed information */
    ZW_LockRoute(false);
#ifdef MULTIPLE_LWR
    LastWorkingRouteCacheLineExile((uint8_t)assign_ID.newNodeID, CACHED_ROUTE_ZW_ANY);
#else
    LastWorkingRouteCacheLinePurge(assign_ID.newNodeID);
#endif /* #ifdef MULTIPLE_LWR */
    RoutingInfoReceived(MAX_NODEMASK_LENGTH,
                        (uint8_t)current_flirs_nodeid,
                        abMultispeedMergeNeighbors);
    ZW_NodeMaskSetBit(listning_node_neighbors, (uint8_t)(current_flirs_nodeid - 1));
    updateNodeNeighbors = false;
    assign_ID.assignIdState = ASSIGN_IDLE;
    if (zcbp_learnCompleteFunction)
    {
      learnNodeInfo.bStatus = REQUEST_NEIGHBOR_UPDATE_STARTED;
      zcbp_learnCompleteFunction(&learnNodeInfo);
    }

}
static
void ListeningNodeDiscoverCallBack (__attribute__((unused)) ZW_Void_Function_t Context,
                                    __attribute__((unused)) uint8_t txStatus,
                                    __attribute__((unused)) TX_STATUS_TYPE* extendedTxStatus)
{
  updateNodeNeighbors = ListeningNodeDiscoverByFlirs();
}

static uint8_t
RequestNodeNeighborDiscovery (  node_id_t bNodeID,
  E_SYSTEM_TYPE nodeType,                           /* IN Node id */
  const STransmitCallback * pTxCallback) /* IN Function to be called when the done */

{
  if (!primaryController || !AreSUCAndAssignIdle())
  {
    updateNodeNeighbors = false;
    return updateNodeNeighbors;
  }

  ROUTING_ANALYSIS_STOP();
  zcbp_learnCompleteFunction = NULL;
  g_learnNodeState = LEARN_NODE_STATE_UPDATE;
  assign_ID.txOptions = (TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE); /* Do route! */
  if (nodeType == E_SYSTEM_TYPE_FLIRS)
  {
    resetFindNeighborsWithFlirs(); // reset it here before the search, in case it was not reset before.

    findNeighborsWithNodetypeState = FIND_NEIGHBOR_NODETYPE_STARTED;
    m_listning_nodeid = bNodeID;
    NodeTypeNeighborCallback = *pTxCallback;
    bNeighborUpdateFailed = true;
    current_flirs_nodeid = 2;
    updateNodeNeighbors = ListeningNodeDiscoverByFlirs();
    if (!updateNodeNeighbors)
    { /* No update started, therefore get out of learnNodeState */
      /* - completedFunc do NOT get called */
      g_learnNodeState = LEARN_NODE_STATE_OFF;
      findNeighborsWithNodetypeState = FIND_NEIGHBOR_NODETYPE_IDLE;
      m_listning_nodeid = 0;
      ZW_TransmitCallbackUnBind(&NodeTypeNeighborCallback);
      bNeighborUpdateFailed = false;
      updateNodeNeighbors = false;

    }
    else
    {
      memset(listning_node_neighbors, 0, MAX_NODEMASK_LENGTH);
      zcbp_learnCompleteFunction = ApiCallCompleted;
      mTxCallback.pCallback = ListeningNodeDiscoverCallBack;
      ZW_TransmitCallbackInvoke(&NodeTypeNeighborCallback, REQUEST_NEIGHBOR_UPDATE_STARTED, NULL);
    }
  }
  else
  {
    ZW_GetNodeProtocolInfo(bNodeID, (NODEINFO *)&newNodeInfoBuffer.nodeInfoFrame.capability);   /* Node info buffer */
    /* For 6.0x we do 9.6/40 findneighbors and 100 findneighbors and to determine */
    /* if no rangeinfo we need to combine the two received rangeinfos. */
    /* We assume NeighborUpdate fails. */
    bNeighborUpdateFailed = true;
    assign_ID.newNodeID = bNodeID;
    resetFindNeighborsWithFlirs(); // reset it here before the search, in case it was not reset before.
    /* We start with normal listening nodes */
    zensorWakeupTime = ZWAVE_SENSOR_WAKEUPTIME_NONE;
    updateNodeNeighbors = true;
    updateNodeNeighbors = RequestNodeRange(true);  /* Let us see if we got it started */
    if (!updateNodeNeighbors)
    { /* No update started, therefore get out of learnNodeState */
      /* - completedFunc do NOT get called */
      g_learnNodeState = LEARN_NODE_STATE_OFF;
    }
    else
    {
      zcbp_learnCompleteFunction = ApiCallCompleted;
      mTxCallback = *pTxCallback;
    }

  }
  return updateNodeNeighbors;
}

/*=====================   ZW_RequestNodeNeighborUpdate  ======================
**
**    Start neighbor discovery for bNodeID, if primary and other nodes present.
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
uint8_t                                      /*RET true neighbor discovery started */
ZW_RequestNodeNeighborUpdate(
  node_id_t bNodeID,                           /* IN Node id */
  STransmitCallback * pTxCallback) /* IN Function to be called when the done */
{
  return RequestNodeNeighborDiscovery(bNodeID,
                                      E_SYSTEM_TYPE_LISTENING,
                                      pTxCallback);
}

/*=====================   ZW_RequestNodeTypeNeighborUpdate  ======================
**
**    Start neighbor discovery for bNodeID and node type specified,
**    if primary and other nodes present.
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
uint8_t                                      /*RET true neighbor discovery started */
ZW_RequestNodeTypeNeighborUpdate(
  node_id_t bNodeID,
  E_SYSTEM_TYPE nodeType,                           /* IN Node id */
  const STransmitCallback * pTxCallback) /* IN Function to be called when the done */
{
   return RequestNodeNeighborDiscovery(bNodeID, nodeType,pTxCallback);
}


#endif


/* TODO - instead of running through all nodes (when finding listening nodes for */
/* findNeighbors) and thereby potentially do 232*2 EEPROM reads (4 SPI operations */
/* - about 55us + 10us for the next to begin) a nodemask should be managed in the */
/* EEPROM containing all listening nodes */
/* - a nodemask in EEPROM containing all nodes could also minimize the EEPROM operations */
/*============================   CreateRequestNodeList   ======================
**    Function description
**      Runs through the nodeinfo table and creates a list of the listning nodes
**      in the network
**    Side effects:
**
**--------------------------------------------------------------------------*/
uint8_t                     /*RET Returns 0 if no nodes found else it returns the number of valid bytes in the nodeMask */
CreateRequestNodeList(
  uint8_t *nodeMask,        /*OUT Nodemask which indicates which nodes to ask */
  uint8_t bSkipNodeID)      /* IN NodeID which indicates a node id that should be omitted */
                         /*    from the nodeMask. Typically it is the nodeID the nodeMask is transmitted to */
{
  uint8_t bLastBit = 0;
  uint8_t bMaxSpeed;

  ZW_NodeMaskClear(nodeMask, MAX_NODEMASK_LENGTH);
  for (uint8_t bCount = 1; bCount <= bMaxNodeID; bCount++)
  {
    if (bCount == bSkipNodeID)
    {
      continue;
    }
    if (CtrlStorageCacheNodeExist(bCount)) { /* Any Node registered ? */
      if (bNeighborDiscoverySpeed == ZW_NEIGHBOR_DISCOVERY_SPEED_BEAM
          && IsNodeSensor(bCount, false, false)) {
        ZW_NodeMaskSetBit(nodeMask, bCount);
        bLastBit = bCount;
      } else
      if (CtrlStorageListeningNodeGet(bCount)) {
        bMaxSpeed = MaxCommonSpeedSupported(bCount, bSkipNodeID);
        if (((bNeighborDiscoverySpeed == ZWAVE_FIND_NODES_SPEED_100K) && (bMaxSpeed >= ZW_NODE_SUPPORT_SPEED_100K)) ||
            ((bNeighborDiscoverySpeed == ZWAVE_FIND_NODES_SPEED_40K)  && (bMaxSpeed == ZW_NODE_SUPPORT_SPEED_40K)) ||
            ((bNeighborDiscoverySpeed == ZWAVE_FIND_NODES_SPEED_9600) && (bMaxSpeed == ZW_NODE_SUPPORT_SPEED_9600)))
        {
          ZW_NodeMaskSetBit(nodeMask, bCount);
          bLastBit = bCount;
        }
      }
    }
  }
  /* No other nodes found */
  if (bLastBit == 0)
  {
    return bLastBit;
  }
  /* Divide lastbit with 8 to find the number of mask bytes */
  return ((bLastBit - 1) >> 3) + 1;
}


/*=========================   RequestNodeRangeSubFunc   ============================
**
**    Check if new routing information is needed from a node, and starts
**    the neighbor discovery process.
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
uint8_t                    /* RET - false  No range info needed */
                        /*     - true   Neighbor discovery started */
RequestNodeRangeSubFunc(uint8_t bFirstTime)  /* IN  nothing. Uses assign_ID.newNodeID*/
{
  uint8_t infoNeeded;

  if (!(newNodeInfoBuffer.nodeInfoFrame.security & ZWAVE_NODEINFO_CONTROLLER_NODE) ||   /* All slaves should be asked */
      (newNodeInfoBuffer.nodeInfoFrame.capability & ZWAVE_NODEINFO_LISTENING_SUPPORT))   /* A nonelistening controller should not */
  {
    if (bFirstTime)
    {
      infoNeeded = RangeInfoNeeded(assign_ID.newNodeID);
    }
    else
    {
      infoNeeded = RANGEINFO_NONEIGHBORS;
    }
#ifdef ZW_ROUTED_DISCOVERY
    if (updateNodeNeighbors || (infoNeeded == RANGEINFO_NONEIGHBORS))
#else
    if (infoNeeded == RANGEINFO_NONEIGHBORS)
#endif
    {
      /* We need to get routing information from this node */
      assignIdBuf.findNodesFrame.numMaskBytes = CreateRequestNodeList(assignIdBuf.findNodesFrame.maskBytes,
                                                                      assign_ID.newNodeID);
      /* Build find nodes in range frame */
      assignIdBuf.findNodesFrame.cmdClass = ZWAVE_CMD_CLASS_PROTOCOL;
      assignIdBuf.findNodesFrame.cmd = ZWAVE_CMD_FIND_NODES_IN_RANGE;

      if (assignIdBuf.findNodesFrame.numMaskBytes == 0)
      {
        return false;
      }

      if (bNeighborDiscoverySpeed == ZW_NEIGHBOR_DISCOVERY_SPEED_BEAM)
      {
        assignIdBuf.findNodesFrame.maskBytes[assignIdBuf.findNodesFrame.numMaskBytes & ZWAVE_FIND_NODES_NUMMASKBYTES_MASK] = ZWAVE_SENSOR_WAKEUPTIME_1000MS;
        assignIdBuf.findNodesFrame.maskBytes[(assignIdBuf.findNodesFrame.numMaskBytes  & ZWAVE_FIND_NODES_NUMMASKBYTES_MASK)+ 1] = ZWAVE_FIND_NODES_SPEED_40K;
        assign_ID.assignIdState = ASSIGN_FIND_SENSOR_NODES_SEND;
      }
      else
      {
        assignIdBuf.findNodesFrame.maskBytes[assignIdBuf.findNodesFrame.numMaskBytes & ZWAVE_FIND_NODES_NUMMASKBYTES_MASK] = ZWAVE_SENSOR_WAKEUPTIME_NONE;
        assignIdBuf.findNodesFrame.maskBytes[(assignIdBuf.findNodesFrame.numMaskBytes & ZWAVE_FIND_NODES_NUMMASKBYTES_MASK)+ 1] = bNeighborDiscoverySpeed;
        assign_ID.assignIdState = ASSIGN_FIND_NODES_SEND;
      }

      /* TO#4161 - Make sure EXPLORE frame is used if needed */
      if (assign_ID.txOptions & TRANSMIT_OPTION_EXPLORE)
      {
        /* Note that we want Explore tried as route resolution if all else fails  */
        bUseExploreAsRouteResolution = true;
      }

      /* Send frame to node */
      FIND_NODES_FRAME *pStruct; // Just to get the size of the maskBytes member of FIND_NODES_FRAME struct.
      uint8_t len = sizeof(FIND_NODES_FRAME) - sizeof(pStruct->maskBytes) + assignIdBuf.findNodesFrame.numMaskBytes;
      static const STransmitCallback TxCallback = { .pCallback = ZCB_AssignTxComplete, .Context = 0 };
      if (!EnQueueSingleData(false, g_nodeID, assign_ID.newNodeID, (uint8_t *)&assignIdBuf,
                                  len,
                                  ~TRANSMIT_OPTION_EXPLORE & assign_ID.txOptions, 0, // 0ms for tx-delay (any value)
                                  ZPAL_RADIO_TX_POWER_DEFAULT,
                                  &TxCallback)
         )
      {
        /* Findneightbors transmit failed */
        ZCB_CallAssignTxCompleteFailed(NULL);
      }

#if defined(ZW_CONTROLLER_STATIC)
      if (pendingUpdateOn)  /* if we are during updating of node information */
      {                     /* send  route updating pending to application */
        ProtocolInterfacePassToAppNodeUpdate(
                                              UPDATE_STATE_NEW_ID_ASSIGNED,
                                              assign_ID.newNodeID,
                                              (uint8_t*)&newNodeInfoBuffer.nodeInfoFrame.nodeType,
                                              assign_ID.nodeInfoLen - offsetof(NODEINFO_FRAME, nodeType)
                                            );
      }
      else
#endif /* defined(ZW_CONTROLLER_STATIC) */
      {
        /* Send routing pending to application */
#ifdef ZW_ROUTED_DISCOVERY
        if (updateNodeNeighbors)
        {
          LearnInfoCallBack(LEARN_STATE_ROUTING_PENDING, assign_ID.newNodeID, 0);
        }
#else
        LearnInfoCallBack(LEARN_STATE_ROUTING_PENDING,
                          assign_ID.newNodeID,
                          assign_ID.nodeInfoLen - offsetof(NODEINFO_FRAME, nodeType));
#endif /*ZW_ROUTED_DISCOVERY*/
      }
      return true;
    }
  }
  return RANGEINFO_ID_INVALID;
}



/*=========================   PingListeningNodeFunc   ============================
**
**    Ask a flirs node to ping a listening node
**--------------------------------------------------------------------------*/
uint8_t                    /* RET - false  listening node pinging failed */
                           /*     - true   listening node pinging started */
PingListeningNodeFunc(void)  /* IN  nothing.*/
{
  if (newNodeInfoBuffer.nodeInfoFrame.security & ZWAVE_NODEINFO_SENSOR_MODE_MASK) /*Only flirs node */
  {
    if ((bNeighborDiscoverySpeed == ZW_NEIGHBOR_DISCOVERY_SPEED_BEAM) && (FIND_NEIGHBOR_NODETYPE_STARTED == findNeighborsWithNodetypeState))
    {
      findNeighborsWithNodetypeState = FIND_NEIGHBOR_NODETYPE_DONE;
      return false;
    }
    /* We need to get routing information from this node */
    assignIdBuf.findNodesFrame.numMaskBytes = 0;
    assignIdBuf.findNodesFrame.maskBytes[0] = 0;
    uint8_t bMaxSpeed = MaxCommonSpeedSupported((uint8_t)m_listning_nodeid, (uint8_t)assign_ID.newNodeID);
    if (((bNeighborDiscoverySpeed == ZWAVE_FIND_NODES_SPEED_100K) && (bMaxSpeed >= ZW_NODE_SUPPORT_SPEED_100K)) ||
        ((bNeighborDiscoverySpeed == ZWAVE_FIND_NODES_SPEED_40K) && (bMaxSpeed == ZW_NODE_SUPPORT_SPEED_40K)) ||
        ((bNeighborDiscoverySpeed == ZWAVE_FIND_NODES_SPEED_9600) && (bMaxSpeed == ZW_NODE_SUPPORT_SPEED_9600)))
    {
      ZW_NodeMaskSetBit(assignIdBuf.findNodesFrame.maskBytes, (uint8_t)m_listning_nodeid);
      assignIdBuf.findNodesFrame.numMaskBytes = 1;
    }
    if (!assignIdBuf.findNodesFrame.numMaskBytes)
    { // no common speed then now need to find neighbors
      return false;
    }
    /* Build find nodes in range frame */
    assignIdBuf.findNodesFrame.cmdClass = ZWAVE_CMD_CLASS_PROTOCOL;
    assignIdBuf.findNodesFrame.cmd = ZWAVE_CMD_FIND_NODES_IN_RANGE;
    assignIdBuf.findNodesFrame.maskBytes[assignIdBuf.findNodesFrame.numMaskBytes & ZWAVE_FIND_NODES_NUMMASKBYTES_MASK] = ZWAVE_SENSOR_WAKEUPTIME_NONE;
    assignIdBuf.findNodesFrame.maskBytes[(assignIdBuf.findNodesFrame.numMaskBytes & ZWAVE_FIND_NODES_NUMMASKBYTES_MASK) + 1] = bNeighborDiscoverySpeed;
    assign_ID.assignIdState = ASSIGN_FIND_NODES_SEND;
    /* TO#4161 - Make sure EXPLORE frame is used if needed */
    if (assign_ID.txOptions & TRANSMIT_OPTION_EXPLORE)
    {
      /* Note that we want Explore tried as route resolution if all else fails  */
      bUseExploreAsRouteResolution = true;
    }

    /* Send frame to node */
    uint8_t len = sizeof(FIND_NODES_FRAME) - sizeof_structmember(FIND_NODES_FRAME,maskBytes) + assignIdBuf.findNodesFrame.numMaskBytes;
    static const STransmitCallback TxCallback = {.pCallback = ZCB_AssignTxComplete, .Context = 0};
    if (!EnQueueSingleData(false, g_nodeID, assign_ID.newNodeID, (uint8_t *)&assignIdBuf,
                           len,
                           ~TRANSMIT_OPTION_EXPLORE & assign_ID.txOptions, 0, // 0ms for tx-delay (any value)
                           ZPAL_RADIO_TX_POWER_DEFAULT,
                           &TxCallback))
    {
      /* Findneightbors transmit failed */
      ZCB_CallAssignTxCompleteFailed(NULL);
    }
    return true;
  }
  return RANGEINFO_ID_INVALID;
}


/*=========================   RequestNodeRange   ============================
**
**    Check if new routing information is needed from a node, and starts
**    the neighbor discovery process.
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
uint8_t RequestNodeRange(uint8_t bFirstTime)
{
  uint8_t bTempVar;
  ZW_HeaderFormatType_t curHeader = llGetCurrentHeaderFormat(assign_ID.newNodeID, false);
  bool result = false;

  if (HDRFORMATTYP_LR == curHeader)
  {
    return false;
  }

  // A FL node should not find FL nodes during inclusion
  if (LEARN_NODE_STATE_NEW == g_learnNodeState &&
      0 == (newNodeInfoBuffer.nodeInfoFrame.capability & ZWAVE_NODEINFO_LISTENING_SUPPORT))
  {
    defaultFLNotLookingForFL = true;
  }

  if (bFirstTime == true)
  {
    bNeighborDiscoverySpeed = (HDRFORMATTYP_2CH == curHeader) ? ZW_NEIGHBOR_DISCOVERY_SPEED_START : ZWAVE_FIND_NODES_SPEED_100K;
    ZW_NodeMaskClear(abMultispeedMergeNeighbors, MAX_NODEMASK_LENGTH);
    bMultispeedArrayByteCount = 0;
  }

  while ((findNeighborsWithFlirs && !defaultFLNotLookingForFL && bNeighborDiscoverySpeed < ZW_NEIGHBOR_DISCOVERY_SPEED_END) ||
          bNeighborDiscoverySpeed < ZW_NEIGHBOR_DISCOVERY_SPEED_BEAM)
  {
    if (FIND_NEIGHBOR_NODETYPE_STARTED == findNeighborsWithNodetypeState)
    {
      bTempVar = PingListeningNodeFunc();
    }
    else
    {
      bTempVar = RequestNodeRangeSubFunc(bFirstTime);
    }

    /* RANGEINFO_ID_INVALID : No need to ask for neighbors */
    /* true == bTempVar : It was started, break out and wait for answer */
    if (bTempVar == RANGEINFO_ID_INVALID || true == bTempVar)
    {
      result = true == bTempVar;
      break;
    }

    bNeighborDiscoverySpeed++;
  }

  if (!result)
  {
    if (FIND_NEIGHBOR_NODETYPE_IDLE != findNeighborsWithNodetypeState)
    {
      findNeighborsWithNodetypeState = FIND_NEIGHBOR_NODETYPE_DONE;
    }
    resetDefaultFLNotLookingForFL();
  }
  return result;
}


/*=========================   DelayPendingUpdate  ============================
**
** Delay sending the next new node registered frame from the pending table.
** to give the other node some time to process the previous frame.
**    Side effects:
**
**--------------------------------------------------------------------------*/
void
ZCB_DelayPendingUpdate(SSwTimer* pTimer)
{
  TimerStop(pTimer);
//  updateTimeOut = 0;
  PendingNodeUpdate();
  if (pendingUpdateOn)
  {
    return;
  }
  PendingNodeUpdateEnd();
}

/*=========================   DelayPendingUpdate  ============================
**
** Delay sending the next new node registered frame from the pending table.
** to give the other node some time to process the previous frame.
**    Side effects:
**
**--------------------------------------------------------------------------*/
void
ZCB_DelayNewRangeRegistred(SSwTimer* pTimer)
{
  TimerStop(pTimer);
  assign_ID.assignIdState = PENDING_UPDATE_ROUTING_SENT;
  ROUTING_ANALYSIS_STOP();
  /* TO#01787 fix - Assign Return Route incorrect for FLiRS Devices */
  ZW_GetRoutingInfo(pendingNodeID, assignIdBuf.newRangeInfo.maskBytes, 0);
  assignIdBuf.newRangeInfo.nodeID = pendingNodeID;
  assignIdBuf.newRangeInfo.cmdClass = ZWAVE_CMD_CLASS_PROTOCOL;
  assignIdBuf.newRangeInfo.cmd = ZWAVE_CMD_NEW_RANGE_REGISTERED;
  assignIdBuf.newRangeInfo.numMaskBytes = MAX_NODEMASK_LENGTH;
  uint8_t len = (offsetof(NEW_RANGE_REGISTERED_FRAME, maskBytes) + assignIdBuf.newRangeInfo.numMaskBytes);
  static const STransmitCallback TxCallback = { .pCallback = ZCB_AssignTxComplete, .Context = 0 };
  if (!EnQueueSingleData(false, g_nodeID, staticControllerNodeID, (uint8_t *)&assignIdBuf,
                              len,
                              (TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE),
                              0, // 0ms for tx-delay (any value)
                              ZPAL_RADIO_TX_POWER_DEFAULT,
                              &TxCallback))
  {
    PendingNodeUpdateEnd();
  }
}

/*=========================   ZCB_ExcludeRequestNOPTxComplete  ==============
**
**    Called when the transmit of the Exclude Request follow-up NOP command
**    is complete.
**    Side effects:
**
**--------------------------------------------------------------------------*/
static void            /* RET  Nothing */
ZCB_ExcludeRequestNOPTxComplete(
  __attribute__((unused)) ZW_Void_Function_t Context,
  uint8_t txStatus,
  __attribute__((unused)) TX_STATUS_TYPE *txStatusReport)
{
  static const STransmitCallback TxCallback = { .pCallback = ZCB_ExcludeRequestNOPTxComplete, .Context = 0 };

  if (txStatus != TRANSMIT_COMPLETE_OK)
  {
    /* The Exclude Request follow-up NOP wasn't ACK'ed. Retry if required, or consider the exclusion done */
    nOPTries--;
    if (nOPTries)
    {
      if (!EnQueueSingleData(RF_SPEED_LR_100K, g_nodeID, assign_ID.newNodeID, (uint8_t *)&assignIdBuf, sizeof(NOP_FRAME_LR),
                              (TRANSMIT_OPTION_LR_FORCE | TRANSMIT_OPTION_ACK),
                              0, // 0ms for tx-delay (any value)
                              ZPAL_RADIO_TX_POWER_DEFAULT, &TxCallback))
      {
        /* Transmit of NOP frame failed. Unusual situation */
        /* Just set nOPTries to 1 and call this function again to finish the exclusion process */
        nOPTries = 1;
        DPRINT("\r\nExclude Request NOP retry Tx failure!");
        ZCB_ExcludeRequestNOPTxComplete(0, TRANSMIT_COMPLETE_FAIL, NULL);
      }
    }
    else
    {
      /* We are now done excluding the node */
      assign_ID.assignIdState = EXCLUDE_REQUEST_IDLE;

      /* Check if node belongs to this controller */
      if (((assign_ID.rxOptions & RECEIVE_STATUS_FOREIGN_HOMEID) == 0) && (CtrlStorageCacheNodeExist(assign_ID.sourceID)))
      {
        /* Remove the node id from the failing node */
        /* when deleting a node with a valid node ID */
        UpdateFailedNodesList(assign_ID.sourceID, true);

        /* Remove the node from Controller storage */
        AddNodeInfo(assign_ID.sourceID, NULL, false);
      }
      else
      {
        /* Hmm, the leaving node is unknown to us. Return sourceID 0x00 in the callback to application */
        assign_ID.sourceID = 0x00;
      }
      /*We are excluding a node that is not in our network, so we don't have knowledge about node info */
      if (offsetof(NODEINFO_FRAME, nodeType) > assign_ID.nodeInfoLen)
      {
        assign_ID.nodeInfoLen = offsetof(NODEINFO_FRAME, nodeType);
      }
      /* Inform the application the node is being removed (which is what LEARN_STATE_ROUTING_PENDING translates to) */
      LearnInfoCallBack(LEARN_STATE_ROUTING_PENDING, assign_ID.sourceID,
                        assign_ID.nodeInfoLen - offsetof(NODEINFO_FRAME, nodeType));
    }
  }
  else
  {
    /* Transmit of NOP was ACK'ed */
    /* The exclusion did not succeed. Return to idle and inform the application that it failed */
    LearnInfoCallBack(LEARN_STATE_FAIL, 0, 0);
  }
}

/*=========================   ZCB_ExcludeRequestConfirmTxComplete ===========
**
**    Called when the transmit of the Exclude Request Confirmation command is
**    complete.
**    Side effects:
**
**--------------------------------------------------------------------------*/
void            /* RET  Nothing */
ZCB_ExcludeRequestConfirmTxComplete(
  __attribute__((unused)) ZW_Void_Function_t Context,
  __attribute__((unused)) uint8_t txStatus,
  __attribute__((unused)) TX_STATUS_TYPE *txStatusReport)
{
  static const STransmitCallback TxCallback = { .pCallback = ZCB_ExcludeRequestNOPTxComplete, .Context = 0 };

  /* The Exclude Request Confirm command was transmitted.. Go to next state which is to send the follow-up NOP */
  g_learnMode = false;
  assign_ID.assignIdState = EXCLUDE_REQUEST_NOP_SEND;

  assign_ID.newNodeID = assign_ID.sourceID;

  assignIdBuf.nopFrameLR.cmdClass = ZWAVE_CMD_CLASS_PROTOCOL_LR;
  assignIdBuf.nopFrameLR.cmd      = ZWAVE_LR_CMD_NOP;

  /* We try max EXCLUDE_REQUEST_CONFIRM_NOP_MAX_TRIES*3 for Remove node */
  nOPTries = EXCLUDE_REQUEST_CONFIRM_NOP_MAX_TRIES;

  /* Send NOP frame to node */
  if (!EnQueueSingleData(RF_SPEED_LR_100K, g_nodeID, assign_ID.newNodeID, (uint8_t *)&assignIdBuf, sizeof(NOP_FRAME_LR),
                         (TRANSMIT_OPTION_LR_FORCE | TRANSMIT_OPTION_ACK),
                         0, // 0ms for tx-delay (any value)
                         ZPAL_RADIO_TX_POWER_DEFAULT, &TxCallback))
  {
    /* Transmit of NOP frame failed. Unusual situation */
    /* Just call the NOP Tx complete function to finish the exclusion process */
    DPRINT("\r\nExclude Request NOP Tx failure!");
    ZCB_ExcludeRequestNOPTxComplete(0, TRANSMIT_COMPLETE_FAIL, NULL);
  }
}

/*
 * Function returns next nodeID to be assigned.
 */
static node_id_t getNewNodeId(__attribute__((unused)) uint8_t bHadInfo)
{
  /* If a nodeId server is present, then we are not SIS and we may reload nodeID */
  if (isNodeIDServerPresent() && (bHadInfo != 0))
  {
    /* Reload nodeID received */
    return CtrlStorageGetReservedId();
  }
  return  0;
}

/*=========================   handleNOPTransmitted   =============================
**
**    Function called when NOP frame was successfully transmitted and inclusion process can continue.
**    Called by ZCB_AssignTxComplete().
**    Side effects:
**
**--------------------------------------------------------------------------*/
static void handleNOPTransmitted(const STransmitCallback* pCompletedFunc)
{
  nodeIDAssigned = false;
  /* Transmit of NOP was ok or NodeID was assigned */
  if (g_learnNodeState == LEARN_NODE_STATE_DELETE)
  {
    /* If we tried to delete then we did not succeed! return */
    /* to idle and inform the application that it failed */
    LearnInfoCallBack(LEARN_STATE_FAIL, 0, 0);
  }
  else if ((g_learnNodeState == LEARN_NODE_STATE_NEW) ||
           (g_learnNodeState == LEARN_NODE_STATE_UPDATE))
  {
    // Check whether we already have some info on the node ID.
    uint8_t bHadInfo = CtrlStorageCacheNodeExist(assign_ID.newNodeID);
    /* Assign ID completed successfully, save node information, and notify */
    /* the application, that the network now contain a new node. */
    AddNodeInfo(assign_ID.newNodeID, (uint8_t*)&newNodeInfoBuffer.nodeInfoFrame, false);

    // Insecure part of inclusion is done,
    // send ZWAVE_LR_CMD_NON_SECURE_INCLUSION_COMPLETE to including node
    if (addUsingLR)
    {
      assign_ID.assignIdState = NON_SECURE_INCLUSION_COMPLETE;

      NON_SECURE_INCLUSION_COMPLETE_FRAME_LR nonSecInclusionCompleteFrame = {
        .cmdClass = ZWAVE_CMD_CLASS_PROTOCOL_LR,
        .cmd      = ZWAVE_LR_CMD_NON_SECURE_INCLUSION_COMPLETE
      };
      if (!EnQueueSingleData(RF_SPEED_LR_100K,
                             g_nodeID,
                             assign_ID.newNodeID,
                             (uint8_t *)&nonSecInclusionCompleteFrame,
                             sizeof(NON_SECURE_INCLUSION_COMPLETE_FRAME_LR),
                             assign_ID.txOptions, 0, // 0ms for tx-delay (any value)
                             ZPAL_RADIO_TX_POWER_DEFAULT,
                             pCompletedFunc))
      {
        DPRINT("\nSending ZWAVE_LR_CMD_NON_SECURE_INCLUSION_COMPLETE failed.\n");
        DelayAssignTxCompleteFailCall();
      }
      // All done, we can exit
      return;
    }

    // If not Long Range, continue Z-Wave node inclusion
    LearnInfoCallBack(LEARN_STATE_ROUTING_PENDING, assign_ID.newNodeID,
                      assign_ID.nodeInfoLen - offsetof(NODEINFO_FRAME, nodeType));

    if (pendingTableEmpty &&
        primaryController &&
        staticControllerNodeID &&
        (staticControllerNodeID != assign_ID.newNodeID) &&
        ZW_IS_NOT_SUC) /* Only if SUC are not ME... */
    {
      assign_ID.assignIdState = ASSIGN_SEND_NODE_INFO_ADD;

      bNewRegisteredNodeFrameSize = offsetof(NEW_NODE_REGISTERED_FRAME, nodeInfo);
      // Create new frame type depending on if the node is slave or not.
      if (!(newNodeInfoBuffer.nodeInfoFrame.security & ZWAVE_NODEINFO_CONTROLLER_NODE))  /* Slave node ? */
      {
        NEW_NODE_REGISTERED_SLAVE_FRAME newNodeRegistered = {
          .cmdClass = ZWAVE_CMD_CLASS_PROTOCOL,
          .cmd = ZWAVE_CMD_NEW_NODE_REGISTERED,
          .nodeID = assign_ID.newNodeID,
          .capability = newNodeInfoBuffer.nodeInfoFrame.capability,
          .security = newNodeInfoBuffer.nodeInfoFrame.security,
          .reserved = newNodeInfoBuffer.nodeInfoFrame.reserved,

        /* newNodeRegistered.nodeType is type of APPL_NODE_TYPE and contains members 'generic' and 'specific'
         * nodeInfoFrame.nodeType is type of NODE_TYPE and contains members 'basic', 'generic' and 'specific'
         * Field 'basic' exists for Controllers only and should be skipped in case of Slave device.
         */
          .nodeType.generic = newNodeInfoBuffer.nodeInfoFrame.nodeType.generic,
          .nodeType.specific = newNodeInfoBuffer.nodeInfoFrame.nodeType.specific
        };
        memcpy(&newNodeRegistered.nodeInfo, &newNodeInfoBuffer.nodeInfoFrame.nodeInfo, NODEPARM_MAX);
        bNewRegisteredNodeFrameSize += (assign_ID.nodeInfoLen > offsetof(NODEINFO_FRAME, nodeInfo) + 1) ? (assign_ID.nodeInfoLen - offsetof(NODEINFO_FRAME, nodeInfo) - 1) : 0;
        memcpy((uint8_t*)&sNewRegisteredNodeFrame.cmdClass,
               (uint8_t*)&newNodeRegistered,
               bNewRegisteredNodeFrameSize);
      }
      else
      {
        NEW_NODE_REGISTERED_FRAME newNodeRegistered = {
          .cmdClass = ZWAVE_CMD_CLASS_PROTOCOL,
          .cmd = ZWAVE_CMD_NEW_NODE_REGISTERED,
          .nodeID = assign_ID.newNodeID,
          .capability = newNodeInfoBuffer.nodeInfoFrame.capability,
          .security = newNodeInfoBuffer.nodeInfoFrame.security,
          .reserved = newNodeInfoBuffer.nodeInfoFrame.reserved,
          .nodeType = newNodeInfoBuffer.nodeInfoFrame.nodeType
        };
        memcpy(&newNodeRegistered.nodeInfo, &newNodeInfoBuffer.nodeInfoFrame.nodeInfo, NODEPARM_MAX);
        bNewRegisteredNodeFrameSize += (assign_ID.nodeInfoLen > offsetof(NODEINFO_FRAME, nodeInfo)) ? (assign_ID.nodeInfoLen - offsetof(NODEINFO_FRAME, nodeInfo)) : 0;
        memcpy((uint8_t*)&sNewRegisteredNodeFrame.cmdClass,
               (uint8_t*)&newNodeRegistered,
               bNewRegisteredNodeFrameSize);
      }
      /* Most likely redundant... */
      SetPendingUpdate(assign_ID.newNodeID);
      DelayAssignTxCompleteFailCall();
    }
    else
    {
      if (!pendingTableEmpty)
      {
        SetPendingUpdate(assign_ID.newNodeID);
      }
      /* We start with normal listening nodes */
      zensorWakeupTime = ZWAVE_SENSOR_WAKEUPTIME_NONE;
      if (RequestNodeRange(true) == false)
      {
        /* We did not need to get routing info from this node, */
        /* return complete to the application */
        LearnInfoCallBack(LEARN_STATE_FIND_NEIGBORS_DONE, assign_ID.newNodeID,
                          assign_ID.nodeInfoLen - offsetof(NODEINFO_FRAME, nodeType));
        LearnInfoCallBack(LEARN_STATE_DONE, assign_ID.newNodeID,
                          assign_ID.nodeInfoLen - offsetof(NODEINFO_FRAME, nodeType));
        assign_ID.newNodeID = getNewNodeId(bHadInfo);
      }
    }
  }
}


/*=========================   AssignTxComplete   =============================
**
**    Transmit complete for Assign a new node/home ID to a node
**    Note that this is called directly fom Explore.c (thus not static)
**    Side effects:
**
**--------------------------------------------------------------------------*/
void            /* RET  Nothing */
ZCB_AssignTxComplete(
  __attribute__((unused)) ZW_Void_Function_t Context,
  uint8_t txStatus,
  __attribute__((unused)) TX_STATUS_TYPE *txStatusReport)
{
  uint8_t   dataLength;

  static const STransmitCallback TxCallback = { .pCallback = ZCB_AssignTxComplete, .Context = 0 };

  switch (assign_ID.assignIdState)
  {
    case ASSIGN_ID_SEND:
      /* Assign ID was transmitted.. Go to next state */
      if (newPrimaryReplication && (GET_NEW_CTRL_STATE == NEW_CTRL_SEND))
      {
        /* If new primary is a member of this network and of pre v3.40 controller type */
        /* remove its secondary former self. TODO - we need to emit a new registered frame to the SUC */
        if (assign_ID.newNodeID == NODE_CONTROLLER_OLD &&
            !(assign_ID.rxOptions & RECEIVE_STATUS_FOREIGN_HOMEID) &&
            CtrlStorageCacheNodeExist(assign_ID.sourceID))
        {
          /* Only if pre 3.40 controller in the receiving end */
          /* When switching between controllers, delete the node ID of the receiving controller */
          /* from the info table if it already exist. */
          AddNodeInfo(assign_ID.sourceID, NULL, false);
        }
        /* TODO - Primary can be SUC - But only post v3.40 controllers... So what to do*/
        /* Was it the SUC? If so clear it.. - The correct network!!! */
        if (assign_ID.newNodeID == NODE_CONTROLLER_OLD)
        {
          CheckRemovedNode(assign_ID.sourceID);
        }
      }

      /* Now the cached foreign HomeID are not valid anymore */
      inclusionHomeIDActive = false;
      /* If the AssignID frame was ACKed then the new nodeID was received by node */
      nodeIDAssigned = (txStatus == TRANSMIT_COMPLETE_OK);

      /* Regardless of wheter assign ID was transmitted correctly or not, we will try to */
      /* transmit NOP to the node.. */
      /* No longer accept frames from other HomeIds - We have now tried to assign an ID on this network */
      /* TO#1385 fix - When adding/removing nodes Controller accepts frames from foreign HomeId after assignID */
      g_learnMode = false;
      assign_ID.assignIdState = ASSIGN_NOP_SEND;

      /* If delete then use source ID */
      uint8_t RfSpeed = false;
      if (g_learnNodeState == LEARN_NODE_STATE_DELETE)
      {
        assign_ID.newNodeID = assign_ID.sourceID;
      }
      else if (g_learnNodeState == LEARN_NODE_STATE_NEW ||
          g_learnNodeState == LEARN_NODE_STATE_UPDATE )
      {
        if (addUsingLR)
        {
          RfSpeed = RF_SPEED_LR_100K;
        }
        else
        {
          RfSpeed = RF_SPEED_9_6K;
        }
      }
      if (LOWEST_LONG_RANGE_NODE_ID <= assign_ID.newNodeID)
      {
        /* Destination node is a Long Range node */
        assignIdBuf.nopFrameLR.cmdClass = ZWAVE_CMD_CLASS_PROTOCOL_LR;
        assignIdBuf.nopFrameLR.cmd      = ZWAVE_LR_CMD_NOP;
        assign_ID.txOptions |= TRANSMIT_OPTION_LR_FORCE;
        dataLength = sizeof(NOP_FRAME_LR);
      }
      else
      {
        assignIdBuf.nopFrame.cmd = ZWAVE_CMD_NOP;
        dataLength = sizeof(NOP_FRAME);
      }
      /* We try max ASSIGN_NOP_REMOVE_MAX_TRIES*3 for Remove node */
      /* We try max ASSIGN_NOP_ADD_MAX_TRIES*3 for Add node */
      nOPTries = (g_learnNodeState == LEARN_NODE_STATE_DELETE) ? ASSIGN_NOP_REMOVE_MAX_TRIES : ASSIGN_NOP_ADD_MAX_TRIES;

      /* TO#4161 - Make sure EXPLORE frame is used if needed */
      if (assign_ID.txOptions & TRANSMIT_OPTION_EXPLORE)
      {
        /* Note that we want Explore tried as route resolution if all else fails  */
        bUseExploreAsRouteResolution = true;
      }

      /* Send NOP frame to node */
      /* TODO - what to do if no room in transmitBuffer... */

  /* TO#4161 - Make sure EXPLORE frame is used if needed */
      if (!EnQueueSingleData(RfSpeed, g_nodeID, assign_ID.newNodeID, (uint8_t *)&assignIdBuf, dataLength,
                                  ~TRANSMIT_OPTION_EXPLORE & assign_ID.txOptions, 0, // 0ms for tx-delay (any value)
                                  ZPAL_RADIO_TX_POWER_DEFAULT, &TxCallback))
      {
        DelayAssignTxCompleteFailCall();
      }
      break;

    case ASSIGN_NOP_SEND:
      /* Now we know if it should be Beamed */
      zensorWakeupTime = ZWAVE_SENSOR_WAKEUPTIME_NONE;
      /* If AssignID or NOP was ACKed then consider nodeID assigned */
      if ((txStatus != TRANSMIT_COMPLETE_OK) &&
          /* TO#1861 fix for Remove node its good that no ACK is received */
          ((--nOPTries) || (!nOPTries && (!nodeIDAssigned || (g_learnNodeState == LEARN_NODE_STATE_DELETE)))
           || (g_learnNodeState == LEARN_NODE_STATE_NEW)))
      {
        uint8_t RfSpeed = false;
        if (addUsingLR)
        {
          RfSpeed = RF_SPEED_LR_100K;
        }
        if (nOPTries)
        {
          /* TO#4161 - Make sure EXPLORE frame is used if needed */
          if (assign_ID.txOptions & TRANSMIT_OPTION_EXPLORE)
          {
            /* Note that we want Explore tried as route resolution if all else fails  */
            bUseExploreAsRouteResolution = true;
          }
          /* Retry with 3 more NOP frames to node */
          /* TODO - what to do if no room in transmitBuffer... */
  /* TO#4161 - Make sure EXPLORE frame is used if needed */
          if (LOWEST_LONG_RANGE_NODE_ID <= assign_ID.newNodeID)
          {
            /* Destination node is a Long Range node */
            dataLength = sizeof(NOP_FRAME_LR);
          }
          else
          {
            dataLength = sizeof(NOP_FRAME);
          }
          if (!EnQueueSingleData(RfSpeed, g_nodeID, assign_ID.newNodeID, (uint8_t *)&assignIdBuf, dataLength,
                                      ~TRANSMIT_OPTION_EXPLORE & assign_ID.txOptions, 0, // 0ms for tx-delay (any value)
                                      ZPAL_RADIO_TX_POWER_DEFAULT, &TxCallback))
          {
            DelayAssignTxCompleteFailCall();
          }
          break;
        }
        /* We have now tried 6*3(ADD) or 1*3(REMOVE) NOP frames and no answer received - assume failed. */
        /* When deleting. TX failure of NOPs means that the node was deleted */
        if (g_learnNodeState == LEARN_NODE_STATE_DELETE)
        {
          /* We are now done excluding node */
          assign_ID.assignIdState = ASSIGN_IDLE;
          /* Check if node belongs to this controller */
          if ((assign_ID.rxOptions & RECEIVE_STATUS_FOREIGN_HOMEID) == 0)
          {
            /* Was it the SUC? If so clear it.. */
            CheckRemovedNode(assign_ID.sourceID);
            /* Remove the node id from the failing node */
            /* when deleting a node with a valid node ID */
            UpdateFailedNodesList(assign_ID.sourceID, true);
            /* If no SIS present then just delete node if node is locally present. */
            /* If SIS present then also send the new reg. frame */
            if (isNodeIDServerPresent() || CtrlStorageCacheNodeExist(assign_ID.sourceID))
            {
              AddNodeInfo(assign_ID.sourceID, NULL, false);
              /* TO#1230 fix - pendingTableEmpty added */
              if (pendingTableEmpty && primaryController &&
                  staticControllerNodeID
#if defined(ZW_CONTROLLER_STATIC)
                  && ZW_IS_NOT_SUC  /* Only if SUC are not ME... */
#endif
                 )
              {
                SetupNewNodeRegisteredDeleted();
                assignIdBuf.newNodeInfo.nodeID = assign_ID.sourceID;
                assign_ID.assignIdState = ASSIGN_SEND_NODE_INFO_DELETE;
                /* TODO - what to do if no room in transmitBuffer... */
                if (!EnQueueSingleData(RfSpeed, g_nodeID, staticControllerNodeID,
                                            (uint8_t *)&assignIdBuf,
                                            (sizeof(NEW_NODE_REGISTERED_FRAME) - NODEPARM_MAX),
                                            (TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE),
                                            0, // 0ms for tx-delay (any value)
                                            ZPAL_RADIO_TX_POWER_DEFAULT,
                                            &TxCallback))
                {
                  DelayAssignTxCompleteFailCall();
                }
                return;
              }
              else
              {
                if (!pendingTableEmpty)
                {
                  SetPendingUpdate(assign_ID.sourceID);
                }
              }
            }
          }
          else
          {
            assign_ID.sourceID = 0x00;
          }

          LearnInfoCallBack(LEARN_STATE_ROUTING_PENDING, assign_ID.sourceID,
                            assign_ID.nodeInfoLen - offsetof(NODEINFO_FRAME, nodeType));
        }
        else
        {
          /* We are not deleting and transmission of NOPs failed. */
          /* We must assume that the node still exists. */

          /*Try to see if a residual information exists and if so remove it*/
          AddNodeInfo(assign_ID.newNodeID, NULL, false);

          LearnInfoCallBack(LEARN_STATE_FAIL, 0, 0);
        }
      }
      else
      {
        // NOP was successfully transmitted. Continue inclusion process.
        handleNOPTransmitted(&TxCallback);
      }
      break;

    case NON_SECURE_INCLUSION_COMPLETE:
      // Now it's OK to start Secure part of inclusion
        /* We start with normal listening nodes */
      zensorWakeupTime = ZWAVE_SENSOR_WAKEUPTIME_NONE;
      if (txStatus != TRANSMIT_COMPLETE_FAIL)
      {
        LearnInfoCallBack(LEARN_STATE_ROUTING_PENDING, assign_ID.newNodeID,
                          assign_ID.nodeInfoLen - offsetof(NODEINFO_FRAME, nodeType));
        LearnInfoCallBack(LEARN_STATE_DONE, assign_ID.newNodeID,
                          assign_ID.nodeInfoLen - offsetof(NODEINFO_FRAME, nodeType));
      }
      else
      {
        // if can't send the frame then we fail inclusion
        DPRINTF("\nZWAVE_LR_CMD_NON_SECURE_INCLUSION_COMPLETE transmission failed, status %X\n", txStatus);
        LearnInfoCallBack(LEARN_STATE_FAIL, assign_ID.newNodeID,0);
      }
      // Node ID has been assigned, so clear newNodeID
      assign_ID.newNodeID = 0;
      break;
    case ASSIGN_SEND_NODE_INFO_DELETE:
      /* We are now done excluding node */
      assign_ID.assignIdState = ASSIGN_IDLE;
      if (txStatus == TRANSMIT_COMPLETE_OK)
      {
        ClearPendingUpdate(assign_ID.newNodeID);
        if (pendingUpdateOn)
        {
#ifdef ZW_CONTROLLER_BRIDGE
          if (assignSlaveState != ASSIGN_COMPLETE)
          {
            ZCB_AssignSlaveComplete(0, TRANSMIT_COMPLETE_OK, NULL);
            virtualNodeID = 0;
          }
#endif
          return;
        }
      }
      else /* if (txStatus == TRANSMIT_COMPLETE_NO_ACK) - TO#1620 - what if we get TRANSMIT_COMPLETE_FAIL*/
      {
        SetPendingUpdate(assign_ID.newNodeID);
      }

#ifdef ZW_CONTROLLER_BRIDGE
      if (assignSlaveState != ASSIGN_COMPLETE)
      {
        ZCB_AssignSlaveComplete(0, TRANSMIT_COMPLETE_OK, NULL);
        virtualNodeID = 0;
      }
      else
#endif
      {
        /* Was LEARN_STATE_DONE, but pending is used if no SUC present in another place */
        LearnInfoCallBack(LEARN_STATE_ROUTING_PENDING, assign_ID.newNodeID,
                          assign_ID.nodeInfoLen - offsetof(NODEINFO_FRAME, nodeType));
      }
      break;

    case ASSIGN_SEND_NODE_INFO_ADD:
      assign_ID.assignIdState = ASSIGN_IDLE;
      if (txStatus == TRANSMIT_COMPLETE_OK)
      {
        ClearPendingUpdate(assign_ID.newNodeID);
      }
      else
      {
        SetPendingUpdate(assign_ID.newNodeID);
      }
#ifdef ZW_CONTROLLER_BRIDGE
      if (ZW_IsVirtualNode(assign_ID.newNodeID))
      {
        /* We have just created a Virtual Node and have reported the news to the SUC/SIS */
        assign_ID.assignIdState = ASSIGN_SEND_RANGE_INFO;
        ROUTING_ANALYSIS_STOP();
        //find40KRoute = false;
        ZW_GetRoutingInfo(newID, assignIdBuf.newRangeInfo.maskBytes, ZW_GET_ROUTING_INFO_ANY);
        assignIdBuf.newRangeInfo.nodeID = newID;
        assignIdBuf.newRangeInfo.cmdClass = ZWAVE_CMD_CLASS_PROTOCOL;
        assignIdBuf.newRangeInfo.cmd = ZWAVE_CMD_NEW_RANGE_REGISTERED;
        assignIdBuf.newRangeInfo.numMaskBytes = MAX_NODEMASK_LENGTH;
        uint8_t len = (offsetof(NEW_RANGE_REGISTERED_FRAME, maskBytes) + assignIdBuf.newRangeInfo.numMaskBytes);
        static const STransmitCallback TxCallback = { .pCallback = ZCB_AssignTxComplete, .Context = 0 };
        if (!EnQueueSingleData(false, g_nodeID, staticControllerNodeID, (uint8_t *)&assignIdBuf,
                                    len,
                                    (TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE),
                                    0, // 0ms for tx-delay (any value)
                                    ZPAL_RADIO_TX_POWER_DEFAULT,
                                    &TxCallback))
        {
          /* TODO - Is this the correct thing to do if no room in transmitBuffer??? */
          /*      - Delayed AssignTXComplete TRANSMIT_COMPLETE_FAIL call instead??? */
          SetPendingUpdate(newID);
          assign_ID.assignIdState = ASSIGN_IDLE;
          ZW_AddNodeToNetwork(ADD_NODE_STOP, NULL);
          ZW_SetLearnNodeState(LEARN_NODE_STATE_OFF, NULL);
        }
        /* We are done here either way... */
        break;
      }
#endif  /* ZW_CONTROLLER_BRIDGE */
      /* We start with normal listening nodes */
      zensorWakeupTime = ZWAVE_SENSOR_WAKEUPTIME_NONE;
      if (RequestNodeRange(true) == false)
      {
        /* We did'nt need to get routing info from this node, return complete */
        /* to the application */
        LearnInfoCallBack(LEARN_STATE_FIND_NEIGBORS_DONE, assign_ID.newNodeID,
                          assign_ID.nodeInfoLen - offsetof(NODEINFO_FRAME, nodeType));
        LearnInfoCallBack(LEARN_STATE_DONE, assign_ID.newNodeID,
                          assign_ID.nodeInfoLen - offsetof(NODEINFO_FRAME, nodeType));
      }
      break;

    case ASSIGN_SEND_RANGE_INFO:
      if (txStatus == TRANSMIT_COMPLETE_OK)
      {
        /* Is the added node a slave based type? */
        if (GetNodeBasicType(assign_ID.newNodeID) > BASIC_TYPE_STATIC_CONTROLLER)
        {
          INIT_NEW_CTRL_STATE(NEW_CTRL_STOP);
        }
      }
      else
      {
        SetPendingUpdate(assign_ID.newNodeID);
      }
#ifdef ZW_CONTROLLER_BRIDGE
      if (ZW_IsVirtualNode(assign_ID.newNodeID))
      {
        assign_ID.assignIdState = ASSIGN_IDLE;
        ZW_AddNodeToNetwork(ADD_NODE_STOP, NULL);
        ZW_SetLearnNodeState(LEARN_NODE_STATE_OFF, NULL);
        return;
      }
#endif
      if ((GET_NEW_CTRL_STATE == NEW_CTRL_STOP))
      {
        RestartAnalyseRoutingTable();
      }
      LearnInfoCallBack(LEARN_STATE_DONE, assign_ID.newNodeID, 0);
      break;

    case ASSIGN_FIND_NODES_SEND:
    {
      /* We do not care if transmit was ok. Wait for the node to send command */
      /* complete before we continue */
      uint32_t iTimeout = ZW_NodeMaskBitsIn(assignIdBuf.findNodesFrame.maskBytes,
                                            assignIdBuf.findNodesFrame.numMaskBytes)
                                            + 1 + 5; // Give it some extra for possible routing
      /* Start timeout for find neighbors complete */
      AssignTimerStart(iTimeout * TRANSFER_MIN_FRAME_WAIT_TIME_MS);
      break;
    }

    case ASSIGN_RANGE_REQUESTED:
      /* Start timeout for request range info */
      AssignTimerStart(TRANSFER_ROUTED_FRAME_WAIT_TIME_MS * 2 * 2);
      break;

    /* TODO ZENSOR - use zensorWakeupTime - which indicates how long a timeout is needed per node */
    case ASSIGN_FIND_SENSOR_NODES_SEND:
    {
      uint32_t iTimeout;
      /* We do not care if transmit was ok. Wait for the node to send command complete before we continue */
      /* Start timeout for find sensor neighbors complete */
      if (ZPAL_RADIO_PROTOCOL_MODE_2 == zpal_radio_get_protocol_mode())
      {
        /* 3 Channel BEAM Transmission transmits up to 3 seconds of BEAM every try
         * -> 3*3 seconds + 3*LBT ms for every not answering node
         * ~ (9900 * numberOfNodes) + 3000 ms */
        iTimeout = (ZW_NodeMaskBitsIn(assignIdBuf.findNodesFrame.maskBytes, assignIdBuf.findNodesFrame.numMaskBytes) * 9900) + 3000;
      }
      else
      {
        /*
         * 2 Channel 1000MS BEAM Transmission transmits 1.1 second of BEAM every try + 1 frame transmit
         * -> 3*1.1s + 3*x ms + 3*LBT ms for every not answering node
         * ~ (4200 * numberOfNodes) + 3000 ms
         */
        /*
         * 2 Channel 250MS BEAM Transmission transmits 0.275 second of BEAM every try + 1 frame transmit
         * -> 3*0.275s + 3*x ms + 3*LBT ms for every not answering node
         * ~ (1100 * numberOfNodes) + 3000 ms
         */
        /* For Simplicity both for 1000MS and 250MS FLiRS nodes the neighbor search is done with 1000ms BEAMs -> 4200ms pr. node */
        iTimeout = (ZW_NodeMaskBitsIn(assignIdBuf.findNodesFrame.maskBytes, assignIdBuf.findNodesFrame.numMaskBytes) * 4200) + 3000;
      }
      AssignTimerStart(iTimeout);
      break;
    }

    case ASSIGN_RANGE_SENSOR_REQUESTED:
      /* Start timeout for request range info */
      AssignTimerStart(TRANSFER_ROUTED_FRAME_WAIT_TIME_MS * 2 * 2);
      break;


    case ASSIGN_SUC_ROUTES:
      AssignTimerStop();
      NetWorkTimerStart(&LearnInfoTimer, ZCB_LearnInfoTimeout, 200);
      break;

    case PENDING_UPDATE_NODE_INFO_SENT:
      /* Add new state to send Range info. */
      if (txStatus == TRANSMIT_COMPLETE_OK)
      {
        /* Start timer that sends new range registred delayed so SUC has time to save the
           node ofnfo frame and update all its tables */
        NetWorkTimerStart(&UpdateTimeoutTimer, ZCB_DelayNewRangeRegistred, 200);
      }
      else
      {
        /*Did not get info sent to SUC Just finish and return control to Application*/
        PendingNodeUpdateEnd();
      }
      break;

    case PENDING_UPDATE_ROUTING_SENT:
      assign_ID.assignIdState = ASSIGN_IDLE;
      if (txStatus == TRANSMIT_COMPLETE_OK)
      {
        ClearPendingUpdate(pendingNodeID);
        NetWorkTimerStart(&UpdateTimeoutTimer, ZCB_DelayPendingUpdate, 1000);
      }
      else
      {
        PendingNodeUpdateEnd();
      }
      break;

#if defined(ZW_CONTROLLER_STATIC)
    case PENDING_UPDATE_GET_NODE_INFO:
      if (txStatus == TRANSMIT_COMPLETE_OK)
      {
        assign_ID.assignIdState = PENDING_UPDATE_WAIT_NODE_INFO;
        count = MAX_REPEATERS << 5;
        NetWorkTimerStart(&UpdateTimeoutTimer,
                          ZCB_UpdateTimeOutHandler,
                          TRANSFER_ROUTED_FRAME_WAIT_TIME_MS
                          );
      }
      else
      {
        assign_ID.assignIdState = ASSIGN_IDLE;
        PendingNodeUpdate();
      }
      break;
#endif

    default:
      break;
  }
}


/*==========================   AssignNewID   ===============================
**
**    Assign a new node/home ID to a node and check that it is responding
**    on the new address.
**    Uses the global assign_ID.newNodeID to determine action
**    Side effects:
**
**--------------------------------------------------------------------------*/
void
AssignNewID(bool isLongRangeChannel) /* RET Nothing */
{
  uint8_t dataLength = isLongRangeChannel ? sizeof(ASSIGN_IDS_FRAME_LR) : sizeof(ASSIGN_IDS_FRAME);
  uint32_t delayedTxMs = 0;  // Used to delay transmission in ms.

  /* Build assign id frame */
  LearnInfoCallBack(LEARN_STATE_NODE_FOUND, 0, 0);
  if (g_learnNodeState != LEARN_NODE_STATE_OFF)
  {
    if (isLongRangeChannel)
    {
      assignIdBuf.assignIDFrameLR.cmdClass     = ZWAVE_CMD_CLASS_PROTOCOL_LR;
      assignIdBuf.assignIDFrameLR.cmd          = ZWAVE_LR_CMD_ASSIGN_IDS;
      assignIdBuf.assignIDFrameLR.newNodeID[0] = assign_ID.newNodeID >> 8;   // MSB
      assignIdBuf.assignIDFrameLR.newNodeID[1] = assign_ID.newNodeID & 0xFF; // LSB
    }
    else
    {
      assignIdBuf.assignIDFrame.cmdClass  = ZWAVE_CMD_CLASS_PROTOCOL;
      assignIdBuf.assignIDFrame.cmd       = ZWAVE_CMD_ASSIGN_IDS;
      assignIdBuf.assignIDFrame.newNodeID = assign_ID.newNodeID;
    }

    /* ID not assigned - Yet */
    nodeIDAssigned = false;

    /* Start the assignment. Do not allow application to interrupt*/
    assign_ID.assignIdState = ASSIGN_ID_SEND;
    /* If node is 0 then clear the home ID, because then it is a delete */
    if (assign_ID.newNodeID == 0)
    {
      if (isLongRangeChannel)
      {
        memset(assignIdBuf.assignIDFrameLR.newHomeID, 0, HOMEID_LENGTH);
      }
      else
      {
        memset(assignIdBuf.assignIDFrame.newHomeID, 0, HOMEID_LENGTH);
      }
    }
    else  /* Clear stored ID if a node ID server is present */
    {
      if (isLongRangeChannel)
      {
        memcpy(assignIdBuf.assignIDFrameLR.newHomeID, ZW_HomeIDGet(), HOMEID_LENGTH);
      }
      else
      {
        memcpy(assignIdBuf.assignIDFrame.newHomeID, ZW_HomeIDGet(), HOMEID_LENGTH);
      }
      if (isNodeIDServerPresent())
      {
        CtrlStorageSetReservedId(0);
      }
    }
    /* Send assign ID frame to node */
    static const STransmitCallback TxCallback = { .pCallback = ZCB_AssignTxComplete, .Context = 0 };

    if (!exploreRemoteInclusion)
    {
      TxOptions_t txOptions = assign_ID.txOptions;

      uint8_t learnSpeed = RF_SPEED_9_6K; //if speed is zero then speed is based on channel's speed  in use

      if (isLongRangeChannel)
      {
        learnSpeed = RF_SPEED_LR_100K;
        /*
         * Delay the AssignId packet transmission with 100ms so that the end-node manages to
         * transmit the SmartStart Include packet on both LR channels.
         */
        txOptions |= TRANSMIT_OPTION_DELAYED_TX;  // This flag is temporary. Don't store it in assign_ID.txOptions.
        delayedTxMs = TRANSMIT_OPTION_DELAY_ASSIGN_ID_LR_MS;
      }
      else
      {
        // Long Range do not support Explore frames
        if (assign_ID.txOptions & TRANSMIT_OPTION_EXPLORE)
        {
          /* Note that we want Explore tried as route resolution if all else fails  */
          bUseExploreAsRouteResolution = true;
        }
      }

      txOptions &= ~TRANSMIT_OPTION_EXPLORE;

      if (addUsingLR)
      {
        txOptions |= TRANSMIT_OPTION_LR_FORCE;
      }

      if (!EnQueueSingleData(learnSpeed,
                             g_nodeID,
                             assign_ID.sourceID,
                             (uint8_t *)&assignIdBuf,
                             dataLength,
                             txOptions,
                             delayedTxMs,
                             ZPAL_RADIO_TX_POWER_DEFAULT,
                             &TxCallback))
      {
        DelayAssignTxCompleteFailCall();
      }
    }
  }
  else
  {
    /* Abort the learning process */
    assign_ID.assignIdState = ASSIGN_IDLE;
  }
}


void
CtrlStorageCacheNodeInfoUpdate(
    node_id_t nodeID,                   /* IN Node ID */
    bool speed100kb)                /* IN Pointer to node info frame */
{
  CtrlStorageCacheCapabilitiesSpeed100kNodeSet(nodeID, speed100kb);
}


/*==========================   ZW_StoreNodeInfoFrame   ============================
**
**    Add node info to node info list in EEPROM
**
**    Side effects:
**      didWrite updated
**
**-------------------------------------------------------------------------------*/
static bool                              /*RET true then nodeID is valid and nodeInfo was stored */
ZW_StoreNodeInfoFrame(
  node_id_t nodeID,                   /* IN Node ID */
  uint8_t *pNodeInfoFrame)               /* IN Pointer to node info frame */
{
  if ( !nodeID ||
      ((nodeID > ZW_MAX_NODES) && (nodeID < LOWEST_LONG_RANGE_NODE_ID)) ||
      (nodeID > HIGHEST_LONG_RANGE_NODE_ID))
  {
    return false;
  }
  else
  {
    /* Convert nodeInfo from NODEINFO -> EX_NVM_NODEINFO */
    EX_NVM_NODEINFO tNodeInfo = { 0 };
    memcpy((uint8_t *)&tNodeInfo.capability, &((NODEINFO_FRAME *)pNodeInfoFrame)->capability, EX_NVM_NODE_INFO_SIZE - 2);
#ifdef ZW_CONTROLLER_TEST_LIB
    if ((g_nodeID == nodeID) && (LOWEST_LONG_RANGE_NODE_ID <= g_nodeID)) {
      memcpy(&tNodeInfo.generic, &((NODEINFO_FRAME *)pNodeInfoFrame)->nodeType.basic, 2);
    } else {
      memcpy(&tNodeInfo.generic, &((NODEINFO_FRAME *)pNodeInfoFrame)->nodeType.generic, 2);
    }
#else
    memcpy(&tNodeInfo.generic, &((NODEINFO_FRAME *)pNodeInfoFrame)->nodeType.generic, 2);
#endif
    /* Save node info */
    CtrlStorageSetNodeInfo(nodeID, &tNodeInfo);
    didWrite = true;
    CtrlStorageCacheNodeInfoUpdate(nodeID,
                        (0 != (((NODEINFO_FRAME*)pNodeInfoFrame)->reserved & ZWAVE_NODEINFO_BAUD_100K)));
    if ((ZW_MAX_NODES >= nodeID) && !IsNodeRepeater(nodeID))
    {
      RoutingAddNonRepeater((uint8_t)nodeID);
    }
  }
  UpdateMaxNodeID(nodeID, false);

  return didWrite;
}

/*==========================   AddNodeInfo   =================================
**
**    Add node info to node info list in EEPROM
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
void
AddNodeInfo(  /* RET  Nothing */
  node_id_t nodeID, /* IN   Node ID */
  uint8_t  *pNodeInfo, /* IN   Pointer to node info frame */
  bool keepRouteCache)
{
  if ( !nodeID ||
      ((nodeID > ZW_MAX_NODES) && (nodeID < LOWEST_LONG_RANGE_NODE_ID)) ||
      (nodeID > HIGHEST_LONG_RANGE_NODE_ID))
  {
    return;
  }
#ifdef ZW_CONTROLLER_STATIC
  uint8_t bInvalidateList = false;
#endif
  if (pNodeInfo == NULL)
  {
    if (ZW_MAX_NODES >= nodeID)  //if classic node
    {
#ifdef ZW_CONTROLLER_STATIC
      if (IsNodeRepeater(nodeID))
      {
        /*Test if the node is a repeater before deleting the node info entry*/
        /* Repeater removed or added. Routing slaves should get new routes */
        bInvalidateList = true;
      }
#endif
      /* Clear routing info for this node */
      ZW_SetRoutingInfo(nodeID, MAX_NODEMASK_LENGTH, NULL);

      /* delete node info */
      CtrlStorageRemoveNodeInfo(nodeID, keepRouteCache);
      didWrite = true;
      DeleteMostUsed(nodeID);
      RoutingRemoveNonRepeater(nodeID);
      /* A node has been removed - could be a repeater, so reset routing */
      ResetRouting();

#if defined(ZW_CONTROLLER_STATIC)
      if (ZW_IS_SUC)
      {
        ClearPendingUpdate(nodeID);
        ClearPendingDiscovery(nodeID);
      }
      SUCUpdateNodeInfo(nodeID, SUC_DELETE, 0);
#endif  /* defined(ZW_CONTROLLER_STATIC) */
      UpdateMaxNodeID(nodeID, true);
    }
    else
    {
      /* delete node info */
      CtrlStorageRemoveNodeInfo(nodeID, keepRouteCache);
      UpdateMaxNodeID(nodeID, true);
      // invalidate the tx power stored in nvm
      SetTXPowerforLRNode(nodeID, DYNAMIC_TX_POWR_INVALID);
    }
  }
  else
  {
    /* Save node info */
    ZW_StoreNodeInfoFrame(nodeID, pNodeInfo);
#ifdef ZW_CONTROLLER_STATIC
    /* didWrite is set in ZW_StoreNodeInfo */

    if (ZW_MAX_NODES >= nodeID) //if classic node
    {
      SUCUpdateNodeInfo(nodeID, SUC_ADD, NODEPARM_MAX);  /* nodeInfo.nodeInfo should allready be ready */
      if (IsNodeRepeater(nodeID))
      {
        /*Test if the node is a repeater after adding the node info entry*/
        /* Repeater removed or added. Routing slaves should get new routes */
        bInvalidateList = true;
      }
    }
#endif
  }
#ifndef ZW_CONTROLLER_STATIC
  /* TO#1441 fix - Portable controller must have its preferred */
  /* repeaters updated when adding/removing nodes */
  if (ZW_MAX_NODES >= nodeID)  //if classic node
  {
    if (IsNodeRepeater(nodeID))
    {
      PreferredSet(nodeID);
    }
    else
    {
      /* If node removed then it can not be a repeater */
      PreferredRemove(nodeID);
    }
  }
#endif

#ifdef ZW_CONTROLLER_STATIC
  if (bInvalidateList)
  {
    /* Repeater removed or added. Routing slaves should get new routes */
    InvalidateSUCRoutingSlaveList();
  }
#endif
}


/*========================   UpdateFailedNodesList   =========================
**
**    Update the failed nodes list, add a node to the list if it didn't return an acknowledgment
**    remove a node from the list if already included and it returned an acknowledgment
**    Side effects:
**
**--------------------------------------------------------------------------*/
void
UpdateFailedNodesList(
  node_id_t bNodeID,
  uint8_t TxStatus)
{
  if (!primaryController)
  {
    return;
  }
  if (TxStatus)
  {
    if ((bNodeID > 0) && (bNodeID <= ZW_MAX_NODES))
    {
      ZW_NodeMaskClearBit(classicFailedNodesMask, bNodeID);
    }
    else if (ZW_nodeIsLRNodeID(bNodeID))
    {
      uint16_t index = bNodeID - LOWEST_LONG_RANGE_NODE_ID + 1;
      ZW_LR_NodeMaskClearBit(lrFailedNodesMask, index);
    }
  }
  else
  {
    if ((bNodeID > 0) && (bNodeID <= ZW_MAX_NODES))
    {
      ZW_NodeMaskSetBit(classicFailedNodesMask, bNodeID);
    }
    else if (ZW_nodeIsLRNodeID(bNodeID))
    {
      uint16_t index = bNodeID - LOWEST_LONG_RANGE_NODE_ID + 1;
      ZW_LR_NodeMaskSetBit(lrFailedNodesMask, index);
    }
  }
}


#ifdef REPLACE_FAILED
/*======================   ReplaceFailedCallBack   ===========================
**
**    Callback function used in the ReplaceFailedNode sequence to keep
**    application updated with the progress
**    Side effects:
**
**--------------------------------------------------------------------------*/
void
ZCB_ReplaceFailedCallBack(       /* RET Nothing */
  LEARN_INFO_T *plearnNodeInfo) /* IN Learn status and information*/
{
  if ((plearnNodeInfo->bStatus == LEARN_MODE_DONE) ||
      (plearnNodeInfo->bStatus == ADD_NODE_STATUS_PROTOCOL_DONE))
  {
    ZW_AddNodeToNetwork(ADD_NODE_STOP, NULL);
    failedNodeReplace = false;
    removeFailedNodeCallBack(ZW_FAILED_NODE_REPLACE_DONE);
  }
  else if (plearnNodeInfo->bStatus == ADD_NODE_STATUS_FAILED)
  {
    ZW_AddNodeToNetwork(ADD_NODE_STOP_FAILED,NULL);
    failedNodeReplace = false;
    /* Now tell Application that we could not replace the failed node */
    removeFailedNodeCallBack(ZW_FAILED_NODE_REPLACE_FAILED);
  }
  else if ((plearnNodeInfo->bStatus == ADD_NODE_STATUS_ADDING_SLAVE) ||
           (plearnNodeInfo->bStatus == ADD_NODE_STATUS_ADDING_CONTROLLER))
  {
    failedNodeReplace = true;
  }
}
#endif


/*==========================   FailedNodeTest   ===============================
**    This fuction is a call back function used when sending NOP
**    command to a node to test if it can communicate.
**    This function call an application call back function to inform the
**    applicaton of the result of the failed node removing proccess.
**---------------------------------------------------------------------------*/
static void
ZCB_FailedNodeTest(
  __attribute__((unused)) ZW_Void_Function_t Context,
  uint8_t TxStatus,
  __attribute__((unused)) TX_STATUS_TYPE *txStatusReport)
{
  uint32_t Status = ZW_FAILED_NODE_NOT_REMOVED;

  if (assign_ID.assignIdState == ASSIGN_FAILED_STARTED)
  {
    if (TxStatus == TRANSMIT_COMPLETE_OK)
    {
      UpdateFailedNodesList(scratchID, true); /* Node seems to be OK */
      Status = ZW_NODE_OK;
    }
    else if (TxStatus == TRANSMIT_COMPLETE_NO_ACK)
    {
      /* If we are replacing, we do not want to remove the node from failed */
      /* before we are sure the node have been replaced correctly */
#ifdef REPLACE_FAILED
      if (!failedNodeReplace)
#endif
      {
        UpdateFailedNodesList(scratchID, true);
      }
      Status = ZW_FAILED_NODE_REMOVED;
    }
    else  // Else - NODE NOT REMOVED - REGARDLESS OF WHAT HAPPENED!
    {
      Status = ZW_FAILED_NODE_NOT_REMOVED;
    }



    if (Status == ZW_FAILED_NODE_REMOVED)
    {
      /* It is now OK to remove node */
#ifdef REPLACE_FAILED
      if (!failedNodeReplace)
#endif
      {
        assign_ID.assignIdState = REMOVED_FAILED_NODE_INFO_SENT;
        if (staticControllerNodeID && (staticControllerNodeID != scratchID)
#if defined(ZW_CONTROLLER_STATIC)
            && ZW_IS_NOT_SUC  /* Only if SUC are not ME... */
#endif
           )
        {
          SetupNewNodeRegisteredDeleted();
          assignIdBuf.newNodeInfo.nodeID = scratchID;
          /* TODO - what to do if no room in transmitBuffer... */
          uint8_t len = offsetof(NEW_NODE_REGISTERED_FRAME, nodeInfo);
          static const STransmitCallback TxCallback = { .pCallback = ZCB_FailedNodeTest, .Context = 0 };
          if (EnQueueSingleData(false,g_nodeID, staticControllerNodeID, (uint8_t *)&assignIdBuf,
                                     len,
                                     (TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE),
                                     0, // 0ms for tx-delay (any value)
                                     ZPAL_RADIO_TX_POWER_DEFAULT, &TxCallback))
          {
            return;
          }
          /* Handle it as a failed transmission... */
          TxStatus = TRANSMIT_COMPLETE_FAIL;
          /* Drop through */
        }
        else
        {
          /* We arrive here if no SUC is present */
          TxStatus = TRANSMIT_COMPLETE_OK;
        }
      }
#ifdef REPLACE_FAILED
      else
      {
        /* Now tell Application that we are ready to replace */
        removeFailedNodeCallBack(ZW_FAILED_NODE_REPLACE);
        /* NodeID was received */
        /*Clear the RouteInformation for this node to force Rediscovery next time*/
        ZW_SetRoutingInfo(scratchID, MAX_NODEMASK_LENGTH, NULL);
        assign_ID.newNodeID = scratchID;
        scratchID = 0;
        /* We now have a node ID, start the "old" learn process */
        assign_ID.assignIdState = ASSIGN_IDLE;
        LearnSetNodesAllowed(ADD_NODE_ANY);
        ZW_AddNodeDskToNetwork((ADD_NODE_ANY|ADD_NODE_OPTION_NETWORK_WIDE), NULL,  ZCB_ReplaceFailedCallBack);
        return;
      }
#endif
    }
  }
  if (assign_ID.assignIdState == REMOVED_FAILED_NODE_INFO_SENT)
  {
    if (TxStatus == TRANSMIT_COMPLETE_OK)
    {
      ClearPendingUpdate(scratchID);
    }
    else
    {
      SetPendingUpdate(scratchID);
    }
    AddNodeInfo(scratchID, NULL, false);
    Status = ZW_FAILED_NODE_REMOVED;
    /* TO#2414 fix - Check and update if the removed node was the SUC */
    CheckRemovedNode(scratchID);
  }
  assign_ID.assignIdState = ASSIGN_IDLE;
  scratchID = 0;
  removeFailedNodeCallBack(Status);
}


/*==========================   ZW_isFailedNode   =============================
**
**    Check if a node is in the failed nodes table
**    Side effects:
**
**--------------------------------------------------------------------------*/
uint8_t                          /*RET: true is node is found in the failed node table, else false*/
ZW_isFailedNode(
  node_id_t bNodeID)
{
  if ((bNodeID > 0) && (bNodeID <= ZW_MAX_NODES))
  {
    return ZW_NodeMaskNodeIn(classicFailedNodesMask, bNodeID);
  }
  else if (ZW_nodeIsLRNodeID(bNodeID))
  {
    uint16_t index = bNodeID - LOWEST_LONG_RANGE_NODE_ID + 1;
    return ZW_LR_NodeMaskNodeIn(lrFailedNodesMask, index);
  }
  else
  {
    return false;
  }
}


/*==========================   ZW_RemoveFailedNode   ===============================
**
**    remove a node from the failed node list, if it already exist.
**    A call back function should be provided otherwise the function will return
**    without trying to remove the node.
**    Side effects:
**
**--------------------------------------------------------------------------*/
uint8_t                                      /*RET return the result of the function call */
ZW_RemoveFailedNode(
  node_id_t nodeID,                           /* IN the failed nodeID */
  VOID_CALLBACKFUNC(completedFunc)(uint8_t)) /* IN call back function to be called when the */
                                          /*    the remove process end. */
{
  uint32_t Status = 0;
  uint8_t  dataLength;

  if (!primaryController)
  {
    Status |= ZW_NOT_PRIMARY_CONTROLLER;
  }
  if (completedFunc == NULL)
  {
    Status |= ZW_NO_CALLBACK_FUNCTION;
  }
  if (!AreSUCAndAssignIdle())
  {
    Status |= ZW_FAILED_NODE_REMOVE_PROCESS_BUSY;
  }
  if (!ZW_isFailedNode(nodeID))
  {
    Status |= ZW_FAILED_NODE_NOT_FOUND;
  }
  if (!Status)
  {
    assign_ID.assignIdState = ASSIGN_FAILED_STARTED;
    if (LOWEST_LONG_RANGE_NODE_ID <= nodeID)
    {
      /* Destination node is a Long Range node */
      assignIdBuf.nopFrameLR.cmdClass = ZWAVE_CMD_CLASS_PROTOCOL_LR;
      assignIdBuf.nopFrameLR.cmd      = ZWAVE_LR_CMD_NOP;
      dataLength = sizeof(NOP_FRAME_LR);
    }
    else
    {
      assignIdBuf.nopFrame.cmd = ZWAVE_CMD_NOP;
      dataLength = sizeof(NOP_FRAME);
    }
    /* double check here if the node is still failing ???*/

    removeFailedNodeCallBack = completedFunc;
    scratchID = nodeID;
    /* We assume it all goes well ;-) */
#if (ZW_FAILED_NODE_REMOVE_STARTED != 0)
    Status = ZW_FAILED_NODE_REMOVE_STARTED;
#endif
    static const STransmitCallback TxCallback = { .pCallback = ZCB_FailedNodeTest, .Context = 0 };
    if (!EnQueueSingleData(false, g_nodeID, nodeID, (uint8_t *)&assignIdBuf, dataLength,
                                (TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE),
                                0, // 0ms for tx-delay (any value)
                                ZPAL_RADIO_TX_POWER_DEFAULT,
                                &TxCallback))
    {
      ZCB_FailedNodeTest(0, TRANSMIT_COMPLETE_FAIL, NULL);
      /* Crash and burn... */
      Status = ZW_FAILED_NODE_REMOVE_FAIL;
    }
  }
  return Status;
}


#ifdef REPLACE_FAILED
/*======================   ZW_ReplaceFailedNode   ============================
**
**    Replace a node from the failed node list, if it already exist.
**    A call back function should be provided otherwise the function will return
**    ithout trying to remove the node.
**    Side effects:
**
**--------------------------------------------------------------------------*/
uint8_t                                      /*RET return the result of the function call */
ZW_ReplaceFailedNode(
  node_id_t bNodeID,                           /* IN the nodeID on the failed node to replace */
  /* TO#2177 fix - ZW_ReplaceFailedNodeID can now be instructed to */
  /* use NormalPower when doing the potential Replace */
  bool bNormalPower,                      /* IN true the replacement is included with normal power */
  VOID_CALLBACKFUNC(completedFunc)(uint8_t)) /* IN call back function to be called when the */
                                          /*    the replace process end. */
{
  uint8_t retval;
  /* TO#2177 fix - ZW_ReplaceFailedNodeID can now be instructed to */
  /* use NormalPower when doing the potential Replace */
  /* Should we use Normal Power when replaceing */
  transferPresentationHighPower = bNormalPower;
  failedNodeReplace = true; /* Tell we are trying to replace a failed node */
  retval = ZW_RemoveFailedNode(bNodeID, completedFunc);
  if (retval)
  {
    failedNodeReplace = false; /* Did not start */
  }
  return retval;
}
#endif

/*--------------------------------------------------------------------
Automatic update functions start
--------------------------------------------------------------------*/

/*============================   ZW_SetSUCNodeID  ===========================
**    Function description
**    This function enable /disable a specified static controller
**    of functioning as the Static Update Controller
**    Side effects:
**
**--------------------------------------------------------------------------*/
                                  /* RET:true If able to connect to the static controller*/
                                  /*      false if the target is not static controller,  */
uint8_t                              /*      the source is not primary or the SUC functinality is not enabled.*/
ZW_SetSUCNodeID(
  uint8_t bNodeID,                   /* the node ID of the static controller to be a SUC*/
  uint8_t SUCState,                  /* true enable SUC, false disable */
  uint8_t bTxOption,                 /* True use low power transmition, False use normal Tx power*/
  uint8_t bCapabilities,             /* The capabilities of the new SUC */
  const STransmitCallback* pTxCallback)   /* a call back */
{
  if (!primaryController)
  {
    return false;
  }
#if defined(ZW_CONTROLLER_STATIC)
  if (bNodeID == g_nodeID)
  {
    SetNodeIDServerPresent(((bCapabilities & ZW_SUC_FUNC_NODEID_SERVER) != 0));
    SetStaticControllerNodeId((SUCState) ? g_nodeID : 0);
    SaveControllerConfig();
    /* TO#01992 fix - Do not set sucEnabled if called with SUCState = false */
    if (SUCState)
    {
      /* TO#1878 fix - We need need to set sucEnabled */
      SetupsucEnabled();
    }
    else
    {
      sucEnabled = 0;
    }

    return true;
  }
#endif
  if (CtrlStorageSlaveNodeGet(bNodeID) ||     /* It must be a controller and listening */
      !CtrlStorageListeningNodeGet(bNodeID)) /* - else it can not be a static! */
  {
    return false;
  }

  SUCCompletedFunc = *pTxCallback;

  assignIdBuf.setSUCFrame.cmdClass = ZWAVE_CMD_CLASS_PROTOCOL;
  assignIdBuf.setSUCFrame.cmd = ZWAVE_CMD_SET_SUC;
  assignIdBuf.setSUCFrame.state = SUCState;
  assignIdBuf.setSUCFrame.SUCcapabilities = (bCapabilities & ZW_SUC_FUNC_NODEID_SERVER) ? ID_SERVER_RUNNING_BIT : 0;
  assign_ID.assignIdState = ASSIGN_LOCK;
  SUC_Update.updateState = SUC_SET_NODE_WAIT_ACK;
  if (bTxOption)
  {
    bTxOption = (TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_LOW_POWER);
  }
  else
  {
    bTxOption = (TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE);
  }
  static const STransmitCallback NextTxCallback = { .pCallback = ZCB_SetSUCAckFunc, .Context = 0};
  if (!EnQueueSingleData(false, g_nodeID, bNodeID, (uint8_t *)&assignIdBuf, sizeof(SET_SUC_FRAME), bTxOption,
                         0, // 0ms for tx-delay (any value)
                         ZPAL_RADIO_TX_POWER_DEFAULT, &NextTxCallback))
  {
    ZW_TransmitCallbackUnBind(&SUCCompletedFunc);
    ZCB_SetSUCAckFunc(0, TRANSMIT_COMPLETE_FAIL, NULL);
    return false;
  }
  return true;
}


/*============================   GetSUCNodeID  ===========================
**    Function description
**    This function gets the nodeID of the current Static Update Controller
**    if ZERO then no SUC is available
**
**--------------------------------------------------------------------------*/
static uint8_t              /*RET nodeID on SUC, if ZERO -> no SUC */
GetSUCNodeID( void )  /* IN Nothing */
{
  return staticControllerNodeID;
}

/*============================   ZW_SendSUCID   ======================
**    Function description
**      Transmits SUC node id to the node specified. Only allowed from Primary or SUC
**    Side effects:
**
**--------------------------------------------------------------------------*/
uint8_t
ZW_SendSUCID(
  uint8_t node,
  uint8_t txOption,
  const STransmitCallback* pTxCallback)
{
  if (primaryController || (staticControllerNodeID == g_nodeID))
  {
    // If this node is primary or static controller. Allow application to send SUCID
    return SendSUCID(node, txOption, pTxCallback);
  }
  else
  {
    return false;
  }
}

/*============================   ZW_RequestNetWorkUpdate   ======================
**    Function description
**      This function request network update from the SUC
**    Side effects:
**
**------------------------------------------------------------------------------*/
uint8_t                      /* RET: Always true */
ZW_RequestNetWorkUpdate(
  const STransmitCallback* pTxCallback) /* call back function indicates the update result */
{
#if defined(ZW_CONTROLLER_STATIC)
  if (ZW_IS_SUC)
  {
    /* I'm the SUC, nothing to do */
    ZW_TransmitCallbackInvoke(pTxCallback,ZW_SUC_UPDATE_DONE,0);
    return true;
  }
#endif

  /* If no Static Update Controller or we are a primary controller and there are no SIS */
  /* then we should not ask for updates! */
  if (!staticControllerNodeID || (primaryController && !isNodeIDServerPresent())) {
      ZW_TransmitCallbackInvoke(pTxCallback,ZW_SUC_UPDATE_DISABLED,0);
      return true;
  }

  /* TO#1810 fix - When SIS exists and inclusion controller wants to include/exclude a new node */
  /* this is called, but after NEW_CTRL_SEND is set and therefor AreSUCAndAssignIdle would always fail */
  if((SUC_Update.updateState != SUC_IDLE) || (assign_ID.assignIdState != ASSIGN_IDLE))
  {
    ZW_TransmitCallbackInvoke(pTxCallback,ZW_SUC_UPDATE_WAIT,0);
    return true;
  }

  SUCCompletedFunc = *pTxCallback;
  assignIdBuf.updateStartFrame.cmdClass = ZWAVE_CMD_CLASS_PROTOCOL;
  assignIdBuf.updateStartFrame.cmd = ZWAVE_CMD_AUTOMATIC_CONTROLLER_UPDATE_START;
  assign_ID.assignIdState = ASSIGN_LOCK;
  SUC_Update.updateState = REQUEST_SUC_UPDATE_WAIT_START_ACK;

  /* Note that we want Explore tried as route resolution if all else fails  */
  bUseExploreAsRouteResolution = true;

  static const STransmitCallback NextTxCallback = { .pCallback = ZCB_SUCUpdateCallback, .Context = 0 };
  if (!EnQueueSingleData(false, g_nodeID, staticControllerNodeID,
                              (uint8_t *)&assignIdBuf.updateStartFrame,
                              sizeof(AUTOMATIC_CONTROLLER_UPDATE_START_FRAME),
                              (TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE),
                              0, // 0ms for tx-delay (any value)
                              ZPAL_RADIO_TX_POWER_DEFAULT,
                              &NextTxCallback))
  {
    ZW_TransmitCallbackUnBind(&SUCCompletedFunc);
    ResetSUC();
    PendingNodeUpdateEnd();

    ZW_TransmitCallbackInvoke(pTxCallback,ZW_SUC_UPDATE_WAIT,0);
    return true;
  }
  return true;
}


/*======================   ZW_GetControllerCapabilities  =====================
**    Function description
**      Returns the Controller capabilities
**      The returned capability is a bitmask where folowing bits are defined :
**       CONTROLLER_IS_SECONDARY
**       CONTROLLER_ON_OTHER_NETWORK
**       CONTROLLER_NODEID_SERVER_PRESENT
**       CONTROLLER_IS_REAL_PRIMARY
**       CONTROLLER_IS_SUC
**       NO_NODES_INCLUDED
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
uint8_t
ZW_GetControllerCapabilities(void)
{
#if defined(ZW_CONTROLLER_STATIC)
  return ((uint8_t)(!primaryController)) + (((uint8_t)controllerOnOther) << 1) +
          (((uint8_t)isNodeIDServerPresent()) << 2) + (((uint8_t)realPrimaryController) << 3)
          + ((ZW_IS_SUC << 4) & CONTROLLER_IS_SUC) + (((uint8_t)bMaxNodeID <= 1) << 5);
#else
  return ((uint8_t)(!primaryController)) + (((uint8_t)controllerOnOther) << 1) +
          (((uint8_t)isNodeIDServerPresent()) << 2) + (((uint8_t)realPrimaryController) << 3) +
          (((uint8_t)bMaxNodeID <= 1) << 5);
#endif
}


/*============================   ZW_IsPrimaryCtrl   ==========================
**    Function description.
**      Returns true When the controller is a primary.
**              false if it is a slave
**    Side effects:
**      Updates the primaryController flag
**--------------------------------------------------------------------------*/
bool                      /*RET true if primary controller, false if slave ctrl */
ZW_IsPrimaryCtrl( void )
{
  return primaryController;
}

#ifdef ZW_CONTROLLER_BRIDGE
/*============================   ZW_IsVirtualNode   ==========================
**    Function description.
**      Returns true if nodeID is a virtual slave node
**              false if it is not a virtual slave node
**    Side effects:
**--------------------------------------------------------------------------*/
bool                 /*RET true if virtual slave node, false if not */
ZW_IsVirtualNode(
  node_id_t nodeID)      /* IN Node ID on node to test for if Virtual */
{
  /* TO#2675 fix - Only valid nodeId can be tested if Virtual */
  if (nodeID && (nodeID <= ZW_MAX_NODES))
  {
    return ZW_NodeMaskNodeIn(nodePool, nodeID);
  }
  else if (LR_VIRTUAL_ID_ENABLED == ZW_Get_lr_virtual_id_type(nodeID))
  {
    return true;
  }
  return false;
}
#endif


/* TODO - move to a new module shared with the slave libraries? */
/*============================   ZW_RequestNodeInfo   ======================
**
**  Function description.
**   Request a node to send it's node information.
**   Function return true if the request is send, else it return false.
**   completedFunc is a callback function, which is called with the status
**   of the Request nodeinformation frame transmission.
**   If a node sends its node info, a SReceiveNodeUpdate will be passed to app
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
  if (LOWEST_LONG_RANGE_NODE_ID <= nodeID)
  {
    /* Destination node is a Long Range node */
    assignIdBuf.ReqNodeInfo.cmdClass = ZWAVE_CMD_CLASS_PROTOCOL_LR;
    assignIdBuf.ReqNodeInfo.cmd = ZWAVE_LR_CMD_REQUEST_NODE_INFO;

    bUseExploreAsRouteResolution = false; // Long Range doesn't do explore
  }
  else
  {
    assignIdBuf.ReqNodeInfo.cmdClass = ZWAVE_CMD_CLASS_PROTOCOL;
    assignIdBuf.ReqNodeInfo.cmd = ZWAVE_CMD_REQUEST_NODE_INFO;

    /* Note that we want Explore tried as route resolution if all else fails  */
    bUseExploreAsRouteResolution = true;
  }
  return(EnQueueSingleData(false, g_nodeID, nodeID,
                                (uint8_t *)&assignIdBuf.ReqNodeInfo, sizeof(REQ_NODE_INFO_FRAME),
                                (TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE),
                                0, // 0ms for tx-delay (any value)
                                ZPAL_RADIO_TX_POWER_DEFAULT,
                                pTxCallback));
}


/*============================   SaveControllerConfig   ========================
**    Function description.
**     Save the configuration of the controller
**
**    Side effects:
**     EEPROM write
**--------------------------------------------------------------------------*/
void
SaveControllerConfig(void)
{
  uint8_t ControllerConfig = ZW_GetControllerCapabilities();
  CtrlStorageSetControllerConfig(ControllerConfig);
}


/*==========================   RequestUpdateCallback   ======================
**    Function description.
**     Callback function for requesting update before learn mode is started
**
**    Side effects:
**     None
**--------------------------------------------------------------------------*/
static void
ZCB_RequestUpdateCallback(
  __attribute__((unused)) ZW_Void_Function_t Context,
  uint8_t txStatus,
  __attribute__((unused)) TX_STATUS_TYPE *txStatusReport)
{
  if (txStatus == ZW_SUC_UPDATE_DONE)
  {
    /* If add was stopped during the update then don't continue */
    if (GET_NEW_CTRL_STATE == NEW_CTRL_SEND)
    {
      if (!failedNodeReplace)
      {
        /* Check if we already have a reserved node ID */
        assign_ID.newNodeID = CtrlStorageGetReservedId();
      }

      /* Check if stored node id is already used, if it is then discard it */
      if ((!failedNodeReplace) && (CtrlStorageCacheNodeExist(assign_ID.newNodeID)))
      {
        assign_ID.newNodeID = 0;
      }
      /* If nodeId server has been disabled during Automatic controller update */
      /* then get next free nodeID */
      if ((!isNodeIDServerPresent()) && (!assign_ID.newNodeID))
      {
        assign_ID.newNodeID = GetNextFreeNode(false);
      }
      /* If we already have an ID then use old assign */
      /* If nodeId server has been disabled during Automatic controller update - use old */
      if ((assign_ID.newNodeID != 0) || !isNodeIDServerPresent())
      {
        ZW_SetLearnNodeStateNoServer(LEARN_NODE_STATE_NEW);
      }
      else
      {
        /* Update is done, request an ID */
        assign_ID.assignIdState = ASSIGN_REQUESTING_ID;
        RequestReservedNodeID();
      }
    }
  }
  else
  {
    LearnInfoCallBack(LEARN_STATE_NO_SERVER, 0, 0);
  }
}

/*============================   RequestIdTimeout   ========================
**    Function description.
**     Timeout function for requesting an node ID from the nodeID server
**
**    Side effects:
**     None
**--------------------------------------------------------------------------*/
void
ZCB_RequestIdTimeout(SSwTimer* pTimer)
{
  TimerStop(pTimer);

  if ((assign_ID.assignIdState == ASSIGN_REQUESTING_ID) ||
      (assign_ID.assignIdState == ASSIGN_REQUESTED_ID))
  {
    LearnInfoCallBack(LEARN_STATE_NO_SERVER, 0, 0);
  }
}

/*============================   RequestIDcallback   ========================
**    Function description.
**     Callback function for sending the request ID to the nodeID server
**
**    Side effects:
**     None
**--------------------------------------------------------------------------*/
static void
ZCB_RequestIDcallback(
  __attribute__((unused)) ZW_Void_Function_t Context,
  __attribute__((unused)) uint8_t txStatus,
  __attribute__((unused)) TX_STATUS_TYPE *txStatusReport)
{
  if (assign_ID.assignIdState == ASSIGN_REQUESTING_ID)
  {
    /* Have we received a node ID? */
    if (assign_ID.newNodeID)
    {
      /* We now have a node ID, start the "old" learn process */
      assign_ID.assignIdState = ASSIGN_IDLE;
      ZW_SetLearnNodeStateNoServer(LEARN_NODE_STATE_NEW);
    }
    else
    {
      /* Even if tx failed we could be "lucky" and receive a node ID as the */
      /* other side could have received the "Reserve Node ID" frame anyway */
      assign_ID.assignIdState = ASSIGN_REQUESTED_ID;
      /* Start timeout for the reply */
      NetWorkTimerStart(&assign_ID.TimeoutTimer,
                        ZCB_RequestIdTimeout,
                        REQUEST_ID_TIMEOUT
                        );
    }
  }
}

/*============================   GetReservedNodeID   ========================
**    Function description.
**     Request a reserved node ID from a node id server (SUC)
**
**    Side effects:
**     None
**--------------------------------------------------------------------------*/
void
RequestReservedNodeID(void)
{
  /* Build the request */
  assignIdBuf.reserveNodeIDFrame.cmdClass     = ZWAVE_CMD_CLASS_PROTOCOL;
  assignIdBuf.reserveNodeIDFrame.cmd          = ZWAVE_CMD_RESERVE_NODE_IDS;
  assignIdBuf.reserveNodeIDFrame.numberOfIDs  = 1;

  /* Send Frame */
  /* TODO - what to do if no room in transmitBuffer... */
  static const STransmitCallback TxCallback = { .pCallback = ZCB_RequestIDcallback, .Context = 0 };
  if (!EnQueueSingleData(false,g_nodeID, staticControllerNodeID, (uint8_t *)&assignIdBuf,
                              sizeof(RESERVE_NODE_IDS_FRAME),
                              (TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE),
                              0, // 0ms for tx-delay (any value)
                              ZPAL_RADIO_TX_POWER_DEFAULT, &TxCallback))
  {
    ZCB_RequestIDcallback(0, TRANSMIT_COMPLETE_FAIL, NULL);
  }
}

/*===============================   IsNode9600   ============================
**    Function description.
**      Returns true if a node is a 9.6k baud only node.
**
**    Side effects:
**     None
**--------------------------------------------------------------------------*/
bool
IsNode9600(
  node_id_t bNodeID)
{
  return ((GetNodeCapabilities(bNodeID) & ZWAVE_NODEINFO_BAUD_RATE_MASK) == ZWAVE_NODEINFO_BAUD_9600);
}

/*===========================   DoesNodeSupportSpeed   =========================
**    Function description.
**      Returns true if bNodeID supports bSpeed. bSpeed can be either
**      ZW_NODE_SUPPORT_SPEED_9600, ZW_NODE_SUPPORT_SPEED_40K, ZW_NODE_SUPPORT_SPEED_100K
**      or ZW_NODE_SUPPORT_SPEED_LR_100K.
**
**    Side effects:
**     None
**--------------------------------------------------------------------------*/
bool
DoesNodeSupportSpeed(
  node_id_t bNodeID,
  uint8_t bSpeed)    /* IN Speed: One of ZW_NODE_SUPPORT_SPEED_*   */
{
  uint8_t bSpeedExtensionBitmask;

  switch(bSpeed)
  {
    case ZW_NODE_SUPPORT_SPEED_9600:
      return (((GetNodeCapabilities(bNodeID) & ZWAVE_NODEINFO_BAUD_RATE_MASK) == ZWAVE_NODEINFO_BAUD_40000)
              || IsNode9600(bNodeID));

    case ZW_NODE_SUPPORT_SPEED_40K:
      return ((GetNodeCapabilities(bNodeID) & ZWAVE_NODEINFO_BAUD_RATE_MASK) == ZWAVE_NODEINFO_BAUD_40000);

    case ZW_NODE_SUPPORT_SPEED_100K:
      bSpeedExtensionBitmask = GetExtendedNodeCapabilities(bNodeID);
      return (bSpeedExtensionBitmask & ZWAVE_NODEINFO_BAUD_100K);

    case ZW_NODE_SUPPORT_SPEED_LR_100K:
      bSpeedExtensionBitmask = GetExtendedNodeCapabilities(bNodeID);
      return (bSpeedExtensionBitmask & ZWAVE_NODEINFO_BAUD_100KLR);

    default:
      return false;
  }
}


/*===========================   MaxCommonSpeedSupported   =========================
**    Function description.
**      Returns the maximum speed two nodes can use to communicate with
**      ZW_NODE_SUPPORT_SPEED_9600, ZW_NODE_SUPPORT_SPEED_40K, ZW_NODE_SUPPORT_SPEED_100K
**      or ZW_NODE_SUPPORT_SPEED_LR_100K.
**
**    Side effects:
**     None
**--------------------------------------------------------------------------*/
uint8_t                    /* RET ZW_NODE_SUPPORT_SPEED_* */
MaxCommonSpeedSupported(
  uint8_t bDestNodeID,   /* IN Destination node ID */
  uint8_t bSourceNodeID) /* IN Source node ID */
{
  uint8_t bOldSpeed;
  bool use100K;
  bool useLRSpeed;

  /* If any of the nodes are 9.6k only then set bOldSpeed to 9.6K */
  bOldSpeed = (CtrlStorageCapabilitiesSpeed40kNodeGet(bDestNodeID) && CtrlStorageCapabilitiesSpeed40kNodeGet(bSourceNodeID)) ? ZWAVE_NODEINFO_BAUD_40000 : ZWAVE_NODEINFO_BAUD_9600;
  /* Speed extension is a bitmask, just and them together to get common speed */
  if ((0 == llIsHeaderFormat3ch()) && (IsNodeSensor(bDestNodeID, false, false) || IsNodeSensor(bSourceNodeID, false, false)))
  {
    use100K = false;
  }
  else
  {
    use100K = (CtrlStorageCacheCapabilitiesSpeed100kNodeGet(bDestNodeID) && CtrlStorageCacheCapabilitiesSpeed100kNodeGet(bSourceNodeID));
  }
  /*Test if both nodes can send using LR speed
    If the soruceNode is me (controller) then we only depend on the destination node since we only have one controller in the network and it always
    can use LR.
  */
  useLRSpeed = (HDRFORMATTYP_LR == llGetCurrentHeaderFormat(bDestNodeID, false))? true : false;
  if (bSourceNodeID != g_nodeID)
  {
    useLRSpeed &= (HDRFORMATTYP_LR == llGetCurrentHeaderFormat(bSourceNodeID, false))? true : false;
  }

  if (useLRSpeed)
  {
    return ZW_NODE_SUPPORT_SPEED_LR_100K;
  }

  if (use100K)
  {
    return ZW_NODE_SUPPORT_SPEED_100K;
  }

  if (bOldSpeed == ZWAVE_NODEINFO_BAUD_40000)
  {
    return ZW_NODE_SUPPORT_SPEED_40K;
  }

  if (bOldSpeed == ZWAVE_NODEINFO_BAUD_9600)
  {
    return ZW_NODE_SUPPORT_SPEED_9600;
  }

  return false;
}

/*===========================   controllerVarInit   =========================
**    Function description.
**      Initializes all global variables in controller module.
**
**    Side effects:
**     None
**--------------------------------------------------------------------------*/
static void
controllerVarInit(void)
{
#ifdef ZW_CONTROLLER_STATIC
  cmdBufferLock = false;
#endif

  g_findInProgress = false;
  oldSpeed = RF_SPEED_9_6K;

#ifdef ZW_CONTROLLER_STATIC
#ifdef ZW_SELF_HEAL
  /* TO#4118 fix - Reset all variables used specifically in the lost functionality */
  lostOngoing = true;
  ResetSUC();
#endif  /* ZW_SELF_HEAL */

#endif  /* ZW_CONTROLLER_STATIC */

#ifdef ZW_ROUTED_DISCOVERY
  updateNodeNeighbors = false;
#endif

  pendingUpdateOn = false;
  pendingTableEmpty = true;
/* TO#1547 fix - Allow only one pending list flush before going to sleep */
  pendingUpdateNotCompleted = false;
#ifdef ZW_CONTROLLER_STATIC
  sucEnabled = SUC_ENABLED_DEFAULT; /* Default we want to be SIS */
#endif
  sucAssignRoute = false;
  sendSUCRouteNow = false;       /* For now we need to request that SUC routes are sent */
  spoof = false;                 /* We do not spoof when sending */

#ifdef REPLACE_FAILED
  failedNodeReplace = false;
#endif

  pendingTimerReload = PENDING_TIMER_RELOAD;
  pendingTimerCount = 0;

#ifdef ZW_CONTROLLER_BRIDGE
  pendingNodeID = 0x00;
#else
  pendingNodeID = 0x00;
#endif

  // Initialize timers
  SSwTimer* const aTimerInitList[] =
  {
    &TimerHandler,
    &UpdateTimeoutTimer,
    &LearnInfoTimer,
    &assign_ID.TimeoutTimer,
    &FindNeighborCompleteDelayTimer,
    &AssignTxCompleteDelayTimer,
    &LostRediscoveryTimer,
    &assignSlaveTimer,
    &newNodeAssignedUpdateDelay,
    &SUC_Update.TimeoutTimer
  };

  for (uint32_t i = 0; i < sizeof_array(aTimerInitList); i++)
  {
    ZwTimerRegister(aTimerInitList[i], true, NULL);
    TimerStop(aTimerInitList[i]);
  }


#ifdef ZW_CONTROLLER_STATIC
  SUCLastIndex = 0x01;
  cmdCompleteSeen = false;
#endif

  scratchID = 0;
  assign_ID.assignIdState = ASSIGN_IDLE;
  assign_ID.newNodeID = 0;
  assign_ID.sourceID = 0;
  oldSpeed = RF_SPEED_9_6K;
  bCurrentRangeCheckNodeID = 0;

  forceUse40K = false;

#ifdef ZW_CONTROLLER_BRIDGE
  virtualNodeID = 0; /* Current virtual nodeID */
  learnSlaveMode = false;  /* true when the "Assign ID's" command is accepted */
  assignSlaveState = ASSIGN_COMPLETE;
  newID = 0;
#endif  /* ZW_CONTROLLER_BRIDGE */

}


/**
* Function used when RF PHY needs to be changed runtime - Is used by the protocol PROTOCOL_EVENT_CHANGE_RF_PHY event
*
*/
static void
ProtocolChangeRfPHY(void)
{
  // Only applicable for Long Range Regions
  if (zpal_radio_protocol_mode_supports_long_range(zpal_radio_get_protocol_mode()))
  {
    ProtocolChangeLrChannelConfig( ZW_LrChannelConfigToUse( zpal_radio_get_rf_profile() ) );
  }
}


/**
* Function called when PROTOCOL_EVENT_CHANGE_RF_PHY event has been triggered
*
*/
void
ProtocolEventChangeRfPHYdetected(void)
{
  // Setup tx queue to inform when Tx queue is empty and try to change RF PHY
  TxQueueEmptyEvent_Add(&sTxQueueEmptyEvent_phyChange, ProtocolChangeRfPHY);
}

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
    switch (pRfProfile->primary_lr_channel)
    {
      case ZPAL_RADIO_LR_CHANNEL_A :    return ZPAL_RADIO_LR_CH_CFG1;
      case ZPAL_RADIO_LR_CHANNEL_B :    return ZPAL_RADIO_LR_CH_CFG2;
      default :
        assert(false);
        // should never happen. zpal_radio_set_primary_long_range_channel only accept CHANNEL_A or B
        return ZPAL_RADIO_LR_CH_CFG_NO_LR;
    }
  }
  else
  {
    return ZPAL_RADIO_LR_CH_CFG_NO_LR;
  }
}



void ZW_UpdateCtrlNodeInformation(uint8_t forced)
{
  if (CtrlStorageCacheNodeExist(g_nodeID) == 0 || forced)
  {
    SetupNodeInformation(ZWAVE_CMD_CLASS_PROTOCOL);  // Setup node info default to Z-Wave cmdClass
    DPRINT("\r\n**L");
    AddNodeInfo(g_nodeID, (uint8_t*)&newNodeInfoBuffer.nodeInfoFrame, false);
  }
}


static void SetStaticControllerNodeId(uint8_t NewStaticControllerNodeId)
{
  staticControllerNodeID = NewStaticControllerNodeId;
  SyncEventArg1Invoke(&g_ControllerStaticControllerNodeIdChanged, NewStaticControllerNodeId);
  CtrlStorageSetStaticControllerNodeId(staticControllerNodeID);
}

#ifdef ZW_CONTROLLER_TEST_LIB
void HomeIdUpdate(uint8_t aHomeId[HOMEID_LENGTH], node_id_t NodeId)
#else
void HomeIdUpdate(uint8_t aHomeId[HOMEID_LENGTH], uint8_t NodeId)
#endif
{
  ControllerStorageSetNetworkIds(aHomeId, NodeId);

  SyncEventArg2Invoke(&g_ControllerHomeIdChanged, *((uint32_t*)aHomeId), NodeId);
  NetworkIdUpdateValidateNotify();
}

bool NodeSupportsBeingIncludedAsLR(void)
{
#ifdef ZW_CONTROLLER_TEST_LIB
  return true;
#else
  return false;
#endif
}
