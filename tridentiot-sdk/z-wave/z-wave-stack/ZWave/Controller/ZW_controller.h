// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file
 * Z-Wave Controller node interface
 * @copyright 2020 Silicon Laboratories Inc.
 */
#ifndef _ZW_CONTROLLER_H_
#define _ZW_CONTROLLER_H_

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <ZW_typedefs.h>

/* Z-Wave */
#include <ZW_node.h>
#include <ZW_transport.h>
#include <SwTimer.h>
#include <ZW_controller_api.h>
#include "ZW_protocol_interface.h"
#include <ZW_MAC.h>

/****************************************************************************/
/*                     EXPORTED TYPES and DEFINITIONS                       */
/****************************************************************************/

// Nothing here.

/****************************************************************************/
/*                               PRIVATE DATA                               */
/****************************************************************************/

/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/
extern bool g_learnMode;  /* true when the "Assign ID's" command is accepted */
extern bool g_learnModeClassic;  /* true when the CLASSIC "Assign ID's" command is accepted */
extern bool g_learnModeDsk;

extern uint8_t g_learnNodeState;   /* not false when the "Node Info" frame is accepted */

extern uint8_t g_Dsk[16];

extern bool primaryController; /* true if this node is the primary/master controller */
extern bool controllerOnOther; /* true - controller on "another" network than the "original" */

extern bool realPrimaryController;
extern bool nodeIdserverPresent;
extern bool addSlaveNodes;
extern bool addCtrlNodes;
extern bool resetCtrl;

/* Status parameters to  LearnCheckIfallowed*/
#define NEW_NODE                1
#define DELETE_NODE             2
#define STOP                    3
#define STOP_FAILED             4
#define RECEIVE                 5
#define UPDATE_NODE             6
#define NEW_PRIMARY             7
#define CTRL_CHANGE             8

extern uint8_t bMaxNodeID;       /* Contains the max value of node ID used*/

extern uint8_t staticControllerNodeID; /* contains the Node id of the static controller in the system*/

typedef struct _ASSIGN_ID_
{
  uint8_t     assignIdState;
  node_id_t   newNodeID;
  node_id_t   sourceID;
  uint8_t     nodeInfoLen;
  SSwTimer    TimeoutTimer;
  TxOptions_t txOptions;
  uint8_t     rxOptions;
} ASSIGN_ID;

extern ASSIGN_ID  assign_ID;

#define ASSIGN_IDLE                     0
#define ASSIGN_ID_SEND                  1
#define ASSIGN_NOP_SEND                 2
#define ASSIGN_FIND_NODES_SEND          3
#define ASSIGN_RANGE_REQUESTED          4
#define ASSIGN_SEND_NODE_INFO_DELETE    5
#define ASSIGN_SEND_NODE_INFO_ADD       6
#define ASSIGN_SEND_RANGE_INFO          7
#define ASSIGN_FIND_SENSOR_NODES_SEND   8
#define ASSIGN_RANGE_SENSOR_REQUESTED   9
#define ASSIGN_LOCK                    10   /* this is used to prevent from using the */
                                          /* assign_buff */
#define PENDING_UPDATE_NODE_INFO_SENT  11
#define PENDING_UPDATE_GET_NODE_INFO   12
#define PENDING_UPDATE_WAIT_NODE_INFO  13
#define REMOVED_FAILED_NODE_INFO_SENT  14
#define PENDING_UPDATE_ROUTING_SENT    15
#define ASSIGN_SUC_ROUTES              16
#define REQUEST_NODE_INFO_WAIT         17
#define ASSIGN_REQUESTING_ID           18
#define ASSIGN_FAILED_STARTED          19
/* TO#1249 - We need to make sure the "Reserve Node ID" frame is not still in transit */
#define ASSIGN_REQUESTED_ID            20
#define NON_SECURE_INCLUSION_COMPLETE  21

#define EXCLUDE_REQUEST_IDLE           ASSIGN_IDLE
#define EXCLUDE_REQUEST_CONFIRM_SEND   ASSIGN_ID_SEND
#define EXCLUDE_REQUEST_NOP_SEND       ASSIGN_NOP_SEND

/* SetLearnNodeState parameter */
#define LEARN_NODE_STATE_OFF          0
#define LEARN_NODE_STATE_NEW          1
#define LEARN_NODE_STATE_UPDATE       2
#define LEARN_NODE_STATE_DELETE       3
#define LEARN_NODE_STATE_SMART_START  4

/* SetLearnNodeState callback status */
#define LEARN_STATE_ROUTING_PENDING     0x21
#define LEARN_STATE_DONE                0x22
#define LEARN_STATE_FAIL                0x23
#define LEARN_STATE_NO_SERVER           0x24
#define LEARN_STATE_LEARN_READY         0x25
#define LEARN_STATE_NODE_FOUND          0x26
#define LEARN_STATE_FIND_NEIGBORS_DONE  0x27
/* WARNING The LEARN_STATE_* status values should not be changed
   without changing the REQUEST_NEIGHBOR_UPDATE_* status defines */


/* ZW_NewController parameter */
#define NEW_CTRL_STATE_SEND           0x01
#define NEW_CTRL_STATE_RECEIVE        0x02
#define NEW_CTRL_STATE_STOP_OK        0x03
#define NEW_CTRL_STATE_STOP_FAIL      0x04
#define NEW_CTRL_STATE_ABORT          0x05
#define NEW_CTRL_STATE_CHANGE         0x06
#define NEW_CTRL_STATE_DELETE         0x07
#define NEW_CTRL_STATE_MAKE_NEW       0x08

/* ZW_NewController callback status */
#define NEW_CONTROLLER_FAILED         0x10
#define NEW_CONTROLLER_DONE           0x11
#define NEW_CONTROLLER_DELETED        0x14
#define NEW_CONTROLLER_CHANGE_DONE    0x15


/* DoesNodeSupportSpeed() options */
#define ZW_NODE_SUPPORT_SPEED_9600    RF_OPTION_SPEED_9600
#define ZW_NODE_SUPPORT_SPEED_40K     RF_OPTION_SPEED_40K
#define ZW_NODE_SUPPORT_SPEED_100K    RF_OPTION_SPEED_100K
#define ZW_NODE_SUPPORT_SPEED_LR_100K RF_OPTION_SPEED_LR_100K

typedef union _FRAME_BUFFER_
{
  NOP_FRAME                              nopFrame;
  NOP_FRAME_LR                           nopFrameLR;
  NOP_POWER_FRAME                        nopPowerFrame;
  ASSIGN_IDS_FRAME                       assignIDFrame;
  ASSIGN_IDS_FRAME_LR                    assignIDFrameLR;
  NON_SECURE_INCLUSION_COMPLETE_FRAME_LR nonSecureInclusionCompleteFrameLR;
  FIND_NODES_FRAME                       findNodesFrame;
  GET_NODE_RANGE_FRAME                   getNodesFrame;
  RANGEINFO_FRAME                        rangeInfoFrame;
/* REPLICATION frames */
  TRANSFER_PRESENTATION_FRAME PresentationFrame;
  TRANSFER_NODE_INFO_FRAME    TransNodeInfoFrame;
  TRANSFER_RANGE_INFO_FRAME   TransferRangeFrame;
  TRANSFER_END_FRAME          TransferCompleteFrame;
  COMMAND_COMPLETE_FRAME      TransferCmdCompleteFrame;
  TRANSFER_NEW_PRIMARY_FRAME  TransferNewPrimary;
  NODEINFO_FRAME              nodeInfo;
  SUC_NODE_ID_FRAME           SUCNodeID;
/*End Replication Frames */
#ifdef ZW_SELF_HEAL
  LOST_FRAME            lostFrame;
  ACCEPT_LOST_FRAME     acceptLostFrame;
#endif /*ZW_SELF_HEAL*/
  ASSIGN_RETURN_ROUTE_FRAME assignReturnRouteFrame;
  REQ_NODE_INFO_FRAME         ReqNodeInfo;
  COMMAND_COMPLETE_FRAME                     CmdCompleteFrame;
  AUTOMATIC_CONTROLLER_UPDATE_START_FRAME    updateStartFrame;
  SET_SUC_FRAME                              setSUCFrame;
  SET_SUC_ACK_FRAME                          setSUCAckFrame;
  NEW_NODE_REGISTERED_FRAME   newNodeInfo;
  NEW_RANGE_REGISTERED_FRAME  newRangeInfo;
  RESERVED_IDS_FRAME          reservedIDsFrame;
  RESERVE_NODE_IDS_FRAME      reserveNodeIDFrame;
  NODES_EXIST_FRAME           nodesExistFrame;
  NODES_EXIST_REPLY_FRAME     nodesExistReplyFrame;
  GET_NODES_EXIST_FRAME       getNodesExistFrame;
#ifdef ZW_DEBUG_TX
  uint8_t                        filler[sizeof(NEW_RANGE_REGISTERED_FRAME) + DEBUG_TX_SIZE];
#endif
} FRAME_BUFFER;


/* Define for making easy and consistent static callback definitions */
#define STATIC_VOID_CALLBACKFUNC(completedFunc)  static VOID_CALLBACKFUNC(completedFunc)

/* Define for making easy and consistent callback definitions */
#define VOID_CALLBACKFUNC_IPTR(completedFunc)   void ( *completedFunc)
#define STATIC_VOID_CALLBACKFUNC_IPTR(completedFunc)  static VOID_CALLBACKFUNC_IPTR(completedFunc)


/* Buffer used to assign id to other nodes */
extern FRAME_BUFFER assignIdBuf;

/* Speeds tried during current return route assignment */
extern uint8_t bReturnRouteTriedSpeeds;

extern VOID_CALLBACKFUNC(learnCompleteFunction)(LEARN_INFO_T *);

extern SSyncEventArg1 g_ControllerStaticControllerNodeIdChanged;  // Callback activated on change to StaticControllerNodeId

extern SSyncEventArg2 g_ControllerHomeIdChanged;                  /* Callback activated on change to Home ID / Node ID */
                                                                  /* Arg1 is Home ID, Arg2 is Node ID*/

extern bool g_findInProgress;

/****************************************************************************/
/*                           EXPORTED FUNCTIONS                             */
/****************************************************************************/
#ifdef ZW_CONTROLLER_STATIC
#define ZW_IS_NOT_SUC ((staticControllerNodeID != g_nodeID))
#define ZW_IS_SUC ((staticControllerNodeID == g_nodeID))
#else
#define ZW_IS_NOT_SUC (true)
#define ZW_IS_SUC (false)
#endif

#define ZW_IS_INCLUSION_CONTROLLER  (nodeIdserverPresent && ZW_IS_NOT_SUC )
#define ZW_IS_SIS_CONTROLLER  (nodeIdserverPresent && ZW_IS_SUC )


/**
*    Copy the Node's current protocol information from the non-volatile
*    memory.
*
* @param[in]     bNodeID     the nodeID
* @param[out]    nodeInfo   pointer to node info buffer to copy information to.
*/
void
ZW_GetNodeProtocolInfo(
  node_id_t  nodeID,
  NODEINFO *nodeInfo);

/**
 * Generates a Node Information Frame (NIF).
 *
 * @param[in]  pnodeInfo Memory to where the NIF will be written.
 * @param[out] cmdClass  Protocol command class being either ZWAVE_CMD_CLASS_PROTOCOL or
 *                       ZWAVE_CMD_CLASS_PROTOCOL_LR. Must be passed so that the correct frame is
 *                       generated according to spec.
 * @return               Returns the length of the generated NIF.
 */
uint8_t
GenerateNodeInformation(
  NODEINFO_FRAME *pNodeInfo,
  uint8_t cmdClass);


/****************************************************************************/
/*                           EXPORTED FUNCTIONS                             */
/*                 Implemented within the Z-Wave modules                     */
/****************************************************************************/

#ifdef ZW_PROMISCUOUS_MODE
extern bool promisMode;
#endif

const SAppNodeInfo_t * getAppNodeInfo();

void
SetPendingUpdate( /* RET  Nothing */
  node_id_t bNodeID);  /* IN   Node ID that should be set */

void
PendingNodeUpdate( void );

void
ReplicationTransferEndReceived(
  uint8_t bSourceNode,
  uint8_t bStatus);

void                                              /* RET Nothing        */
InternSetLearnMode(
  uint8_t mode,                                      /* IN learnMode bitmask */
  VOID_CALLBACKFUNC(completedFunc)(LEARN_INFO_T*)); /* IN Callback function */

#ifdef ZW_CONTROLLER_STATIC
/*============================   DoSelfDiscovery   ======================
**    Function description
**      Makes the node check for its neighbors.
**    Side effects:
**
**--------------------------------------------------------------------------*/
bool DoSelfDiscovery(                    /*RET true if started, false if not*/
    VOID_CALLBACKFUNC(completedFunc)());  /*IN  Transmit completed call back function  */
#endif

/**
*    Get the type of the specified node, if the node isn't registered in
*    the controller 0 is returned.
* @param[in]     bNodeID     the nodeID
* @return         nonzero value as a Type of the node, else zero if the node is not registered
*/
uint8_t
ZCB_GetNodeType(         /* RET  Type of node, or 0 if not registered */
  node_id_t bNodeID);    /* IN   Node id */

/*========================   GetNodeCapabilities  ==========================
**
**    Get the type of the specified node, if the node isn't registered in
**    the controller 0 is returned.
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
uint8_t                   /* RET  Node capabilities */
GetNodeCapabilities(
  node_id_t nodeID);         /* IN   Node id */

/*=========================   GetMaxNodeID   =============================
**
**    Get the max node ID used in this controller
**
**    Side effects:
**      bMaxNodeID is updated accordingly
**
**--------------------------------------------------------------------------*/
node_id_t                 /* RET  Next free node id, or 0 if no free */
GetMaxNodeID( void );  /* IN   Nothing  */

/*=========================   GetMaxNodeID   =============================
**
**    Get the max node ID used in this controller
**
**    Side effects:
**      bMaxNodeID is updated accordingly
**
**--------------------------------------------------------------------------*/
node_id_t                 /* RET  Next free node id for LR, or 0 if no free */
GetMaxNodeID_LR( void );  /* IN   Nothing  */


/*==========================   AddNodeInfo   ===============================
**
**    Add node info to node info list in EEPROM
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
void
AddNodeInfo(   /* RET  Nothing */
  node_id_t wNodeID,      /* IN   Node ID */
  uint8_t *pNodeInfo,     /* IN   Pointer to node info frame */
  bool keepCacheRoute);   /* IN   true if cacheRoute must be preserved */


/*=============================   ControllerInit   ==========================
**    Initialize variables and structures.
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
void               /* RET Nothing */
ControllerInit(
                const SVirtualSlaveNodeInfoTable_t * pVirtualSlaveNodeInfoTable,
                const SAppNodeInfo_t * pAppNodeInfo,
                bool bFullInit
              );

/**
*    Remove all Nodes and timers from the EEPROM memory. Generates new
*    active home ID and resets node ID to 1.
*/
void
ZW_SetDefault(void);

/**
*    Remove all Nodes and timers from the EEPROM memory. Keeps its own current
*    active home and node ID.
*
*/
void
ZW_ClearTables(void);


/*==========================   UpdateFailedNodesList   ===============================
**
**    Update the failed nodes list, add a node to the list if it didn't return an acknowledgment
**    remove a node from the list if already included and it returned an acknowledgment
**    Side effects:
**
**--------------------------------------------------------------------------*/
void
UpdateFailedNodesList(
  node_id_t nodeID,
  uint8_t TxStatus);

/**
*    Enable/Disable home/node ID learn mode.
*    When learn mode is enabled, received "Assign ID's Command" are handled:
*    If the current stored ID's are zero, the received ID's will be stored.
*    If the received ID's are zero the stored ID's will be set to zero.
*
* @param[in]     mode     learnMode bitmask
* @param[in]     completedFunc  pointer to Callback function
*/
void
ZW_SetLearnMode(
  uint8_t mode,
  VOID_CALLBACKFUNC(completedFunc)(LEARN_INFO_T*));

/**
*    Set controller in "learn node mode".
*    When learn node mode is enabled, received "Node Info" frames are
*    handled and passed to the application through the call back function.
*
*    The states are:
*
*    LEARN_NODE_STATE_OFF     Stop accepting Node Info frames.
*    LEARN_NODE_STATE_NEW     Add new nodes to node list
*    LEARN_NODE_STATE_DELETE  Delete nodes from node list
*    LEARN_NODE_STATE_UPDATE  Update node-info in node list
*
* @param[in]     mode       1:New; 2:Update 3:Delete; 0: Disable learn mode
* @param[in]     completedFunc  pointer to Callback function
*/
void
ZW_SetLearnNodeState( /*RET  Nothing        */
  uint8_t mode,          /*IN  1:New; 2:Update 3:Delete; 0: Disable learn mode */
  VOID_CALLBACKFUNC(completedFunc)(LEARN_INFO_T*)); /* IN Callback function */


/*============================   SaveControllerConfig   ========================
**    Function description.
**     Save the configuration of the controller
**
**    Side effects:
**     EEPROM write
**--------------------------------------------------------------------------*/
void SaveControllerConfig();

/*===============================   IsNode40K   ============================
**    Function description.
**    	Returns true if a node is a 9.6k baud only node.
**
**    Side effects:
**     None
**--------------------------------------------------------------------------*/
bool
IsNode9600(
  node_id_t bNodeID);

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
  uint8_t bSpeed);    /* IN Speed: One of ZW_NODE_SUPPORT_SPEED_*   */

/*==========================   GetNodeSecurity  =============================
**
**    Get the node security byte
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
uint8_t                /*RET Node security */
GetNodesSecurity(
  node_id_t bNodeID);   /* IN Node id */

/*===========================   IsNodeSensor  ==============================
**
**    Check if a node is a sensor node
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
uint8_t
IsNodeSensor(
  node_id_t bNodeID,
  bool bRetSensorMode,
  bool bCheckAssignState);


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
  uint8_t bDestNodeID,     /* IN Destination node ID */
  uint8_t bSourceNodeID    /* IN Source node ID */
);

/*============================   AreSUCAndAssignIdle   ======================
**    Function description
**      Test if SUC is ready to handle requests from nodes.
**      returns true if so, false if not.
**    Side effects:
**
**--------------------------------------------------------------------------*/
uint8_t
AreSUCAndAssignIdle( void );

/**
*   Used when the controller is replication mode.
*   It sends the payload and expects the receiver to respond with a
*   command complete message.
*
* @param[in]     destNodeID       Destination node ID. Only single cast allowed
* @param[in]    pData            Data buffer pointer.
* @param[in]    dataLength       Data buffer length.
* @param[in]    txOptions        Transmit option flags.
* @param[in]    pCompletedFunc   Transmit completed call back function.
*
* @return        returns false if transmitter is busy else true
*/
extern uint8_t
ZW_ReplicationSend(
  uint8_t      destNodeID,
  uint8_t     *pData,
  uint8_t      dataLength,
  TxOptions_t  txOptions,
  const STransmitCallback* pCompletedFunc);

/**
*  Assign static return routes within a Routing Slave node.
*
*  If a priority route is provided (not recommended), only the priority
*  route will be assigned.
*
*  If no Priority Route is provided (recommended):
*  The shortest transport routes from the Routing Slave node
*  to the route destination node will be calculated and
*  transmitted to the Routing Slave node.
*
*  If Destination Node ID is recognized as SUC, Route will be assigned as
*  SUC return route.*
*
* @param[in]    destNodeID        Routing Slave Node ID
* @param[in]    bDstNodeID        Report destination Node ID - if 0, destination will be self
* @param[in]    pPriorityRoute    Route to be assigned as priority route - Set to NULL to NOT supply a priority route (recommended)
* @param[in]    isSucRoute        True if this is a SUC route, else false
* @param[in]    pTxCallback       Status of process
*
* @return        returns true if assign was initiated. false if not
*/
extern bool
ZW_AssignReturnRoute(
  node_id_t  bSrcNodeID,
  node_id_t  bDstNodeID,
  uint8_t *pPriorityRoute,
  bool isSucRoute,
  const STransmitCallback* pTxCallback);

/**
*   Delete static return routes or SUC static return routes within a
*   Routing Slave node.
*
*   Transmit "NULL" routes to the Routing Slave node.
*
* @param[in]    nodeID        Slave Node ID
* @param[in]    bDeleteSUC    Delete SUC return routes only, or Delete standard return routes only
* @param[in]    pTxCallback   Callback function
*
* @return        returns true if delete return routes was initiated. false if not
*/
extern bool
ZW_DeleteReturnRoute(
  node_id_t  nodeID,
  bool bDeleteSUC,
  const STransmitCallback* pTxCallback);

/**
*   This function enable /disable a specified static controller
*   of functioning as the Static Update Controller
*
* @param[in]    nodeID          the node ID of the static controller to be a SUC
* @param[in]    SUCState        true enable SUC, false disable
* @param[in]    bTxOption       true if to use low power transmission, false for normal Tx power
* @param[in]    bCapabilities   The capabilities of the new SUC
* @param[in]    pTxCallback     Callback function
*
* @return        returns true if target is a static controller
*                        false if the target is not a static controller,
*                        the source is not primary or the SUC functinality is not enabled.
*/
uint8_t
  ZW_SetSUCNodeID(
    uint8_t nodeID,
    uint8_t SUCState,
    uint8_t bTxOption,
    uint8_t bCapabilities,
    const STransmitCallback* pTxCallback);

/**
*   Transmits SUC node id to the node specified. Only allowed from Primary or SUC
*
* @param[in]    nodeID          the node ID.
* @param[in]    bTxOption       Tx transmit option
* @param[in]    pTxCallback     Callback function
*
* @return        returns  TBD
*/
uint8_t ZW_SendSUCID(
  uint8_t node,
  uint8_t txOption,
  const STransmitCallback* pTxCallback);


/*========================   ZW_RequestNetWorkUpdate   ======================*/
/**
* \ingroup BASIS
* \macro{ZW_REQUEST_NETWORK_UPDATE (func)}
*
* Used to request network topology updates from the SUC/SIS node. The update
* is done on protocol level and any changes are notified to the application by
* passing a SReceiveNodeUpdate to the Application.
*
* Secondary controllers can only use this call when a SUC is present in the network.
* All controllers can use this call in case a SUC ID Server (SIS) is available.
*
* Routing Slaves can only use this call, when a SUC is present in the network.
* In case the Routing Slave has called ZW_RequestNewRouteDestinations prior to
* ZW_RequestNetWorkUpdate, then Return Routes for the destinations specified by
* the application in ZW_RequestNewRouteDestinations will be updated along with
* the SUC Return Route.
*
* \note The SUC can only handle one network update at a time, so care should be taken
* not to have all the controllers in the network ask for updates at the same time.
*
* \warning This API call will generate a lot of network activity that will use
* bandwidth and stress the SUC in the network. Therefore, network updates should
* be requested as seldom as possible and never more often that once every hour
* from a controller.
*
* \return true If the updating process is started.
* \return false If the requesting controller is the SUC node or the SUC node is unknown.
*
* \param[in] completedFunc IN  Transmit complete call back.
*
* Callback function Parameters:
* \param[in] txStatus IN Status of command:
* -ZW_SUC_UPDATE_DONE  The update process succeeded.
* -ZW_SUC_UPDATE_ABORT The update process aborted because of an error.
* -\ref ZW_SUC_UPDATE_WAIT  The SUC node is busy.
* -\ref ZW_SUC_UPDATE_DISABLED  The SUC functionality is disabled.
* -\ref ZW_SUC_UPDATE_OVERFLOW  The controller requested an update after more than 64 changes have occurred in the network. The update information is then out of date in respect to that controller. In this situation the controller have to make a replication before trying to request any new network updates.

Timeout: 65s
Exption recovery: Resume normal operation, no recovery needed

Serial API:
HOST->ZW: REQ | 0x53 | funcID
ZW->HOST: RES | 0x53 | retVal
ZW->HOST: REQ | 0x53 | funcID | txStatus
*/
uint8_t                      /* RET:  true SUC exists, false; SUC not configured or node is primary */
ZW_RequestNetWorkUpdate(
  const STransmitCallback* pTxCallback); /* call back function indicates the update result */

/**
*   Request a node to send it's node information.
*   Function return true if the request is send, else it return false.
*   FUNC is a callback function, which is called with the status of the
*   Request node information frame transmission.
*   If a node sends its node info, a SReceiveNodeUpdate will be passed to app
*   with UPDATE_STATE_NODE_INFO_RECEIVED as status together with the received
*   node information.
*
* @param[in]    nodeID       node id of the node to request node info from it.
* @param[in]    pTxCallback  Callback function.
*
*@return returns false if transmitter busy
*/
bool
ZW_RequestNodeInfo(
  node_id_t nodeID,
  const STransmitCallback* pTxCallback);

/**
*    Start neighbor discovery for bNodeID, if any other nodes present.
*
* @param[in]    bNodeID       node id.
* @param[in]    pTxCallback  Callback function.
*
*@return returns true if neighbor discovery started, else false
*/
uint8_t
ZW_RequestNodeNeighborUpdate(
  node_id_t bNodeID,
  STransmitCallback * pTxCallback);


/**
*    Start neighbor discovery for bNodeID, if any other nodes present.
*
* @param[in]    bNodeID       node id.
* @param[in]    E_SYSTEM_TYPE nodeType,
* @param[in]    pTxCallback  Callback function.
*
*@return returns true if neighbor discovery started, else false
*/
uint8_t
ZW_RequestNodeTypeNeighborUpdate(
  node_id_t bNodeID,
  E_SYSTEM_TYPE nodeType,
  const STransmitCallback * pTxCallback);

void
ZW_UpdateCtrlNodeInformation(uint8_t forced);


/**
*  Get a list of routing information for a node from the routing table.
*  Only include neighbor nodes supporting a certain speed.
*  Assumes that nodes support all lower speeds in addition to the advertised
*  highest speed.
*
* @param[in]    nodeID        Node ID on node whom routing info is needed on
* @param[out]   pMask         Pointer where routing info should be saved.
* @param[in]    bOptions      Upper nibble is bit flag options, lower nibble is speed
*                             Combine exactly one speed with any number of options
*                             Bit flags options for upper nibble:
*                               GET_ROUTING_INFO_REMOVE_BAD       - Remove bad link from routing info
*                               GET_ROUTING_INFO_REMOVE_NON_REPS  - Remove non-repeaters from the routing info
*                             Speed values for lower nibble:
*                               ZW_GET_ROUTING_INFO_ANY  - Return all nodes regardless of speed
*                               ZW_GET_ROUTING_INFO_9600 - Return nodes supporting 9.6k
*                               ZW_GET_ROUTING_INFO_40K  - Return nodes supporting 40k
*                               ZW_GET_ROUTING_INFO_100K - Return nodes supporting 100k
*/
void
ZW_GetRoutingInfo(
  node_id_t nodeID,
  uint8_t*  pMask,
  uint8_t   bOptions);

/**
*  Sends command completed to primary controller. Called in replication mode
*  when a command from the sender has been processed.
*/
extern void
ZW_ReplicationReceiveComplete(void);

/**
*  Get a list of routing information for a node from the routing table.
*  Only include neighbor nodes supporting a certain speed.
*  Assumes that nodes support all lower speeds in addition to the advertised
*  highest speed.
*
* @param[in]    nodeID     the failed node ID
*
* @return returns true if node in failed node table, else false
*/
uint8_t
ZW_isFailedNode(
  node_id_t nodeID);   /* IN the failed node ID */

/**
*    remove a node from the failed node list, if it already exist.
*    A call back function should be provided otherwise the function will return
*    without removing the node.
*
*    The call back function parameter value is:
*    ZW_NODE_OK                     The node is working proppely (removed from the failed nodes list )
*    ZW_FAILED_NODE_REMOVED         The failed node was removed from the failed nodes list
*    ZW_FAILED_NODE_NOT_REMOVED     The failed node was not
*
* @param[in]    nodeID     the failed node ID
* @param[in]    completedFunc  callback function to be called when the remove process end.
*
* @return the function returns the following  codes:
*           If the removing process started successfully then the function will return
*             ZW_FAILED_NODE_REMOVE_STARTED        The removing process started
*           If the removing process can not be started then the API function will return
*           on or more of the following flags
*             ZW_NOT_PRIMARY_CONTROLLER            The removing process was aborted because the controller is not the primaray one
*             ZW_NO_CALLBACK_FUNCTION              The removing process was aborted because no call back function is used
*             ZW_FAILED_NODE_NOT_FOUND             The removing process aborted because the node was node found
*             ZW_FAILED_NODE_REMOVE_PROCESS_BUSY   The removing process is busy
*/
uint8_t
ZW_RemoveFailedNode(
  node_id_t nodeID,
  VOID_CALLBACKFUNC(completedFunc)(
    uint8_t));


/**
*    Replace a node from the failed node list.
*    A call back function should be provided otherwise the function will return
*    without replacing the node.
*
*    The call back function parameter value is:
*
*    ZW_NODE_OK                     The node is working properly (removed from the failed nodes list )
*    ZW_FAILED_NODE_REPLACE         The failed node are ready to be replaced and controller
*                                   is ready to add new node with nodeID of the failed node
*    ZW_FAILED_NODE_REPLACE_DONE    The failed node has been replaced
*    ZW_FAILED_NODE_REPLACE_FAILED  The failed node has not been replaced
*
* @param[in]    bNodeID        the nodeID of the failed node to replace
* @param[in]    bNormalPower   TRUE the replacement is included with normal power
* @param[in]    completedFunc  callback function to be called when the replace process end.
*
* @return the function returns the following  codes:
*           If the replacing process started successfully then the function will return:
*             ZW_FAILED_NODE_REMOVE_STARTED  The replacing process started and now the new
*                                            node must emit its node information frame to
*                                            start the assign process
*           If the replace process can not be started then the API function will return
*           on or more of the following flags:
*             ZW_NOT_PRIMARY_CONTROLLER           The replacing process was aborted because
*                                                 the controller is not the primary controller
*             ZW_NO_CALLBACK_FUNCTION             The replacing process was aborted because no
*                                                 call back function is used
*             ZW_FAILED_NODE_NOT_FOUND            The replacing process aborted because
*                                                 the node was node found
*             ZW_FAILED_NODE_REMOVE_PROCESS_BUSY  The replacing process is busy
*             ZW_FAILED_NODE_REMOVE_FAIL          The replacing process could not be started
**                                                because of
*/
uint8_t
ZW_ReplaceFailedNode(
  node_id_t bNodeID,
  bool bNormalPower,
  VOID_CALLBACKFUNC(completedFunc)(uint8_t));

/**
* Check if the current controller is primary
*
* @return    Returns TRUE When the controller is a primary.
*                    FALSE if it is a slave
*/
bool ZW_IsPrimaryCtrl(void);

/**
* Get the Controller capabilities
*
* @return          The returned capability is a bitmask where following bits are defined :
*                   CONTROLLER_IS_SECONDARY
*                   CONTROLLER_ON_OTHER_NETWORK
*                   CONTROLLER_NODEID_SERVER_PRESENT
*                   CONTROLLER_IS_REAL_PRIMARY
*                   CONTROLLER_IS_SUC
*                   NO_NODES_INCLUDED
*
*/
uint8_t
ZW_GetControllerCapabilities(void);

/**
* Add any type of node to the network
*
* @param[in]    bMode       The inclusion mode to be used during inclusion process
*                           The modes are:
*                           ADD_NODE_ANY            Add any node to the network
*                           ADD_NODE_CONTROLLER     Add a controller to the network
*                           ADD_NODE_SLAVE          Add a slave node to the network
*                           ADD_NODE_STOP           Stop learn mode without reporting an error.
*                           ADD_NODE_STOP_FAILED    Stop learn mode and report an error to the
*                             new controller.
*                           ADD_NODE_OPTION_NORMAL_POWER    Set this flag in bMode for High Power inclusion.
*                           ADD_NODE_OPTION_NETWORK_WIDE    Set this flag in bMode to use NWI for the inclusion.
*                           ADD_NODE_OPTION_LR              Set this flag in bMode to use LR for the inclusion.
*                             This flag is only valid when using SmartStart, as Long range  nodes support only smartStart inclusion
*                           ADD_NODE_OPTION_NO_FL_SEARCH    Set this flag in bMode to ignore FL nodes in the neighbor discovery of the inclusion.
* @param[in]                completedFunc    Pointer the callback function.
*
*/
void
ZW_AddNodeToNetwork(
  uint8_t bMode,
  VOID_CALLBACKFUNC(completedFunc)(LEARN_INFO_T*));

/**
* Add any type of node to the network
*
* @param[in]    bMode           The inclusion mode to be used during inclusion process
*                               The modes are:
*                               ADD_NODE_ANY            Add any node to the network
*                               ADD_NODE_CONTROLLER     Add a controller to the network
*                               ADD_NODE_SLAVE          Add a slave node to the network
*                               ADD_NODE_STOP           Stop learn mode without reporting an error.
*                               ADD_NODE_STOP_FAILED    Stop learn mode and report an error to the
*                                 new controller.
*                               ADD_NODE_OPTION_NORMAL_POWER    Set this flag in bMode for High Power inclusion.
*                               ADD_NODE_OPTION_NETWORK_WIDE    Set this flag in bMode to use NWI for the inclusion.
*                               ADD_NODE_OPTION_LR              Set this flag in bMode to use LR for the inclusion.
*                                 This flag is only valid when using SmartStart, as Long range  nodes support only smartStart inclusion
*                               ADD_NODE_OPTION_NO_FL_SEARCH    Set this flag in bMode to ignore FL nodes in the neighbor discovery of the inclusion.
* @param[in]    pDsk             Pointer to the DSK for the node to be included
* @param[in]    completedFunc    Pointer the callback function.
*
*/
void
ZW_AddNodeDskToNetwork(
  uint8_t bMode,
  const uint8_t *pDsk,
  VOID_CALLBACKFUNC(completedFunc)(LEARN_INFO_T*));

/**
*   Remove any type of node from the network
*
* @param[in]    bMode           valid modes are:
*                               REMOVE_NODE_ANY            Remove Specified nodeID (any type) from the network
*                               REMOVE_NODE_CONTROLLER     Remove Specified nodeID (controller) from the network
*                               REMOVE_NODE_SLAVE          Remove Specified nodeID (slave) from the network
*                               REMOVE_NODE_STOP           Stop learn mode without reporting an error.
*                               REMOVE_NODE_OPTION_NORMAL_POWER   Set this flag in bMode for Normal Power exclusion.
* @param[in]    completedFunc   pointer to callback function.
*/
#define ZW_RemoveNodeFromNetwork( mode, callback)  ZW_RemoveNodeIDFromNetwork(mode, 0, callback)

/**
*   Remove specific node ID from the network
*
*    - If valid nodeID (1-232) is specified then only the specified nodeID
*     matching the mode settings can be removed.
*    - If REMOVE_NODE_ID_ANY or none valid nodeID (0, 233-255) is specified
*     then any node which matches the mode settings can be removed.
* @param[in]    bMode           valid modes are:
*                               REMOVE_NODE_ANY            Remove Specified nodeID (any type) from the network
*                               REMOVE_NODE_CONTROLLER     Remove Specified nodeID (controller) from the network
*                               REMOVE_NODE_SLAVE          Remove Specified nodeID (slave) from the network
*                               REMOVE_NODE_STOP           Stop learn mode without reporting an error.
*                               REMOVE_NODE_OPTION_NORMAL_POWER   Set this flag in bMode for Normal Power exclusion.
*                               REMOVE_NODE_OPTION_NETWORK_WIDE   Set this flag in bMode for enabling
*                                 Networkwide explore via explore frames
* @param[in]    nodeID          The node ID of the node to be removed from the network.
* @param[in]    completedFunc   pointer to callback function.
*/
void
ZW_RemoveNodeIDFromNetwork(
  uint8_t bMode,
  node_id_t nodeID,
  VOID_CALLBACKFUNC(completedFunc)(LEARN_INFO_T*));

/**
*   Transfer the role as primary controller to another controller
*
* @param[in]    bMode           valid values re:
                                CONTROLLER_CHANGE_START          Start the creation of a new primary
                                CONTROLLER_CHANGE_STOP           Stop the creation of a new primary
                                CONTROLLER_CHANGE_STOP_FAILED    Report that the replication failed
                                ADD_NODE_OPTION_NORMAL_POWER     Set this flag in bMode for High Power exchange.
* @param[in]    completedFunc   pointer to callback function.
*/
void
ZW_ControllerChange(
  uint8_t bMode,
  VOID_CALLBACKFUNC(completedFunc)(LEARN_INFO_T*));

/**
*   Are two specific nodes neighbours?
*
* @param[in]    bNodeA     First node ID
* @param[in]    bNodeB     Second node ID.
* @return return non-zeor if bNodeA and bNodeB are neighbors, else return zero
*
*/
bool
ZW_AreNodesNeighbours(
  uint8_t bNodeA,
  uint8_t bNodeB);

/**
*  Set the maximum number of route tries which should be done before failing
*  or resorting to explore frame if this is specified
*
* @param[in]    maxRouteTries     Maximum number of routing retires.
*
*/
void
ZW_SetRoutingMAX(
  uint8_t maxRouteTries);


/**
*   Read the priority route of an specific node from NVM
*   If Route is found then the found route is copied into the specified
*   ROUTECACHE_LINE_SIZE (5) byte sized byte array, where the first
*   4 bytes (index 0-3) contains the repeaters active in the route and
*   the last (index 4) byte contains the speed information.
*   First ZERO in repeaters (index 0-3) indicates no more repeaters in route
*   A direct route is indicated by the first repeater (index 0) being ZERO.
*
*   Example: 0,0,0,0,ZW_PRIORITY_ROUTE_SPEED_100K ->
*               Direct 100K
*            2,3,0,0,ZW_PRIORITY_ROUTE_SPEED_40K  ->
*               40K route through repeaters 2 and 3
*
* @param[in]     bNodeID          node ID for the node to read its priority route.
* @param[out]    pPriorityRoute   pointer to the node bNondeID priority route
*
* @return return  ZW_PRIORITY_ROUTE_APP_PR if Route is an App defined Priority Route
*                 ZW_PRIORITY_ROUTE_ZW_LWR = Route is a Last Working Route
*                 ZW_PRIORITY_ROUTE_ZW_NLWR = Route is a Next to Last Working Route
*                  Zero if no priority route is found.
*
*/
uint8_t
ZW_GetPriorityRoute(
  node_id_t bNodeID,
  uint8_t *pPriorityRoute);


/**
*   Write the priority route of an specific node to the NVM
*      pPriorityRoute MUST point to a ROUTECACHE_LINE_SIZE (5) byte sized
*      byte array containing the wanted route. The first 4 bytes (index 0-3)
*      contains the repeaters active in the route and last (index 4) byte
*      contains the speed information.
*      First ZERO in repeaters (index 0-3) indicates no more repeaters in route.
*      A direct route is indicated by the first repeater (index 0) being ZERO.
*
*      Example: {0,0,0,0,ZW_PRIORITY_ROUTE_SPEED_100K}->
*                  Direct 100K Priority route
*               {2,3,0,0,ZW_PRIORITY_ROUTE_SPEED_40K}  ->
*                  40K Priority route through repeaters 2 and 3
*               {2,3,4,0,ZW_PRIORITY_ROUTE_SPEED_9600} ->
*                  9600 Priority route through repeaters 2, 3 and 4
*
* @param[in]    nodeID           node ID for the node to write its priority route.
* @param[in]    pPriorityRoute   pointer to the node bNondeID priority route
*
* @return return  TRUE if specified pPriorityRoute has been saved for bNodeID.
*                 FALSE if specified bNodeID is NOT valid and no PriorityRoute
*/
bool
ZW_SetPriorityRoute( node_id_t nodeID, uint8_t *pPriorityRoute);

bool NodeSupportsBeingIncludedAsLR(void);

/**
* Function called when PROTOCOL_EVENT_CHANGE_RF_PHY event has been triggered
*
*/
void
ProtocolEventChangeRfPHYdetected(void);

/****************************************************************************
* Functionality specific for the Bridge Controller API.
****************************************************************************/


/* Defines used to handle inclusion and exclusion of virtual slave nodes */
/* Are returned as callback parameter when callback, setup with */
/* ZW_SetSlaveLearnMode, is called during inclusion/exclusion process */
#define ASSIGN_COMPLETE             0x00
#define ASSIGN_NODEID_DONE          0x01  /* Node ID have been assigned */
#define ASSIGN_RANGE_INFO_UPDATE    0x02  /* Node is doing Neighbor discovery */

/* Defines defining modes possible in ZW_SetSlaveLearnMode : */

/* Disable SlaveLearnMode (disable possibility to add/remove Virtual Slave nodes) */
/* Allowed when bridge is a primary controller, an inclusion controller or a secondary controller */
#define VIRTUAL_SLAVE_LEARN_MODE_DISABLE  0x00

/* Enable SlaveLearnMode - Enable possibility for including/excluding a */
/* Virtual Slave node by an external primary/inclusion controller */
/* Allowed when bridge is an inclusion controller or a secondary controller */
#define VIRTUAL_SLAVE_LEARN_MODE_ENABLE   0x01

/* Add new Virtual Slave node if possible */
/* Allowed when bridge is a primary or an inclusion controller */
#define VIRTUAL_SLAVE_LEARN_MODE_ADD      0x02

/* Remove existing Virtual Slave node */
/* Allowed when bridge is a primary or an inclusion controller */
#define VIRTUAL_SLAVE_LEARN_MODE_REMOVE   0x03


/*===============================   ZW_SendSlaveData   ===========================
**    Transmit data buffer to a single Z-Wave node or all Z-Wave nodes - broadcast
**    and transmit it from srcNodeID
**
**
**    txOptions:
**          TRANSMIT_OPTION_LOW_POWER   transmit at low output power level (1/3 of
**                                      normal RF range).
**          TRANSMIT_OPTION_ACK         the multicast frame will be followed by a
**                                      singlecast frame to each of the destination nodes
**                                      and request acknowledge from each destination node.
**
**    RET  TRUE  if data buffer was successfully put into the transmit queue
**         FALSE if transmitter queue overflow or if controller primary or srcNodeID invalid
**               then completedFunc will NOT be called
**
** uint8_t                                 RET  FALSE if transmitter queue overflow or srcNodeID invalid or controller primary
** ZW_SendSlaveData(
**    uint8_t      srcNodeID,              IN  Source node ID - Virtual nodeID
**    uint8_t      destNodeID,             IN  Destination node ID - 0xFF == Broadcast
**    uint8_t     *pData,                  IN  Data buffer pointer
**    uint8_t      dataLength,             IN  Data buffer length
**    TxOptions_t  txOptions,              IN  Transmit option flags
**    VOID_CALLBACKFUNC(completedFunc)(    IN  Transmit completed call back function
**    uint8_t txStatus));                  IN  Transmit status
**--------------------------------------------------------------------------------*/
uint8_t                /*RET FALSE if transmitter busy or srcNodeID invalid or controller primary */
ZW_SendSlaveData(
  uint8_t  srcNodeID,  /* IN Source node ID - Virtuel nodeID */
  uint8_t  destNodeID, /* IN Destination node ID - 0xFF == all nodes */
  uint8_t *pData,      /* IN Data buffer pointer           */
  uint8_t  dataLength, /* IN Data buffer length            */
  uint8_t  txOptions,  /* IN Transmit option flags         */
  const STransmitCallback* pCompletedFunc); /* IN Transmit completed call back function  */


/*============================   ZW_SendSlaveNodeInformation   ============================
**    Create and transmit a slave node information frame
**    RET  TRUE  if Slave NodeInformation frame was successfully put into the transmit queue
**         FALSE if transmitter queue overflow or if controller primary or destNode invalid
**               then completedFunc will NOT be called
**---------------------------------------------------------------------------------------*/
uint8_t                                     /*RET FALSE if SlaveNodeinformation not in transmit queue */
ZW_SendSlaveNodeInformation(
  node_id_t   sourceNode,                   /* IN Which node is to transmit the node info */
  node_id_t   destNode,                     /* IN Destination Node ID  */
  TxOptions_t txOptions,                    /* IN Transmit option flags */
  const STransmitCallback* pCompletedFunc); /* IN Transmit completed call back function  */


/*===========================   ZW_SetSlaveLearnMode   =======================
**    Enable/Disable home/node ID learn mode.
**    When learn mode is enabled, received "Assign ID's Command" are handled:
**    If the current stored ID's are zero, the received ID's will be stored.
**    If the received ID's are zero the stored ID's will be set to zero.
**
**    The learnFunc is called when the received assign command has been handled.
**    The returned parameter is the learned Node ID.
**
** void           RET  Nothing
** ZW_SetSlaveLearnMode(
**   node_id_t nodeID,           IN  nodeID on node to set Learn Node Mode -
**                               ZERO if new node is to be learned
**   uint8_t mode,               IN  VIRTUAL_SLAVE_LEARN_MODE_DISABLE: Disable
**                               VIRTUAL_SLAVE_LEARN_MODE_ENABLE:  Enable
**                               VIRTUAL_SLAVE_LEARN_MODE_ADD:     Create New Virtual Slave Node
**                               VIRTUAL_SLAVE_LEARN_MODE_REMOVE:  Remove Virtual Slave Node
**   VOID_CALLBACKFUNC(learnFunc)(uint8_t bStatus, uint8_t orgId, uint8_t newID)); IN  Node learn call back function.
**--------------------------------------------------------------------------*/
uint8_t                  /*RET Returns TRUE if successful or FALSE if node invalid or controller is primary */
ZW_SetSlaveLearnMode(
  node_id_t nodeID,      /* IN nodeID on Virtual node to set in Learn Node Mode - if new node wanted then it must be ZERO */
  uint8_t mode,          /* IN TRUE  Enable, FALSE  Disable */
  VOID_CALLBACKFUNC(learnFunc)(uint8_t bStatus, node_id_t orgID, node_id_t newID));  /* IN Slave node learn call back function. */


/*============================   ZW_IsVirtualNode   ======================
**    Function description.
**      Returns TRUE if bNodeID is a virtual slave node
**              FALSE if it is not a virtual slave node
**    Side effects:
**--------------------------------------------------------------------------*/
bool                      /*RET TRUE if virtual slave node, FALSE if not */
ZW_IsVirtualNode(node_id_t nodeID);

#endif /* _ZW_CONTROLLER_H_ */
