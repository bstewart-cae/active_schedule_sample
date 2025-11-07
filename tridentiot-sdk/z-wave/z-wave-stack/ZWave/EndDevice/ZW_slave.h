// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file ZW_slave.h
 * @copyright 2022 Silicon Laboratories Inc.
 *
 * @brief Z-Wave Slave node interface.
 */
#ifndef _ZW_SLAVE_H_
#define _ZW_SLAVE_H_

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <ZW_typedefs.h>
#include <ZW_basis_api.h>
#include <ZW_node.h>
#include <ZW_transport_api.h>
#include <ZW_transport.h>
#include <ZW_slave_api.h>
#include <SyncEvent.h>

/****************************************************************************/
/*                     EXPORTED TYPES and DEFINITIONS                       */
/****************************************************************************/

/* Beware: ROUTE_SPEED_BAUD_RATE_MASK has one more bit than NVM_RETURN_ROUTE_SPEED_MASK */
/*         But it is currently unused.                                                  */
#define NVM_RETURN_ROUTE_SPEED_MASK     0x03
#define NVM_RETURN_ROUTE_SPEED_9600     ((ROUTE_SPEED_BAUD_9600 >> 3) & NVM_RETURN_ROUTE_SPEED_MASK)    /* 0x01 */
#define NVM_RETURN_ROUTE_SPEED_40K      ((ROUTE_SPEED_BAUD_40000 >> 3) & NVM_RETURN_ROUTE_SPEED_MASK)   /* 0x02 */
#define NVM_RETURN_ROUTE_SPEED_100K     ((ROUTE_SPEED_BAUD_100000 >> 3) & NVM_RETURN_ROUTE_SPEED_MASK)  /* 0x00 */
#define NVM_RETURN_ROUTE_SPEED_200K     ((ROUTE_SPEED_BAUD_200000 >> 3) & NVM_RETURN_ROUTE_SPEED_MASK)  /* 0x03 */

#define NVM_RETURN_ROUTE_BEAM_DEST_MASK 0x03
#define NVM_RETURN_ROUTE_PRIORITY_MASK  0x1C

typedef struct _node_range_
{
  uint8_t             lastController;
  TxOptions_t         txOptions;
} NODE_RANGE;

typedef void (*learn_mode_callback_t)(ELearnStatus bStatus, node_id_t nodeID);
/****************************************************************************/
/*                               PRIVATE DATA                               */
/****************************************************************************/

// Nothing here.

/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/

extern bool g_learnMode;  /* true when the "Assign ID's" command is accepted */
extern bool g_learnModeClassic;  /* true when the CLASSIC "Assign ID's" command is accepted */
extern bool g_learnModeDsk;

extern uint8_t g_Dsk[16];

extern SSyncEventArg1 g_SlaveSucNodeIdChanged;  // Callback activated on change to SucNodeId

extern SSyncEventArg2 g_SlaveHomeIdChanged;     /* Callback activated on change to Home ID / Node ID */
                                                /* Arg1 is Home ID, Arg2 is Node ID */

extern bool g_findInProgress;

extern uint8_t g_sucNodeID;

/****************************************************************************/
/*                           EXPORTED FUNCTIONS                             */
/****************************************************************************/
extern void
InitNodeInformation();

void
InternSetLearnMode(
  uint8_t mode,                                  /* IN learnMode bitmask */
  learn_mode_callback_t learnFunc);  /* IN Node learn call back function. */


/*===========================   ZW_SetLearnMode   ===========================
**    Enable/Disable home/node ID learn mode.
**    When learn mode is enabled, received "Assign ID's Command" are handled:
**    If the current stored ID's are zero, the received ID's will be stored.
**    If the received ID's are zero the stored ID's will be set to zero.
**
**    The learnFunc is called when the received assign command has been handled.
**    The returned parameter is the learned Node ID.
**--------------------------------------------------------------------------*/
extern void         /*RET  Nothing        */
ZW_SetLearnMode(
  uint8_t mode,                                       /* IN  learnMode bitmask */
  learn_mode_callback_t learnFunc);  /*IN  Node learn call back function. */

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
  NODEINFO_SLAVE_FRAME *pnodeInfo,
  uint8_t cmdClass);


/****************************************************************************/
/*                           EXPORTED FUNCTIONS                             */
/*                 Implemented within the Z-Wave moduls                     */
/****************************************************************************/

/*======================   SlaveRegisterPowerLocks   =======================
**    Initializes slave power locks
**--------------------------------------------------------------------------*/
void SlaveRegisterPowerLocks(void);

/*=============================   SlaveInit   ==============================
**    Initialize variables and structures.
**    Application node information is loaded from the application to
**    initialize the nodeinformation registered in the protocol.
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
void                          /*RET Nothing */
SlaveInit(const SAppNodeInfo_t * pAppNodeInfo); /* IN Request Nodeinformation from Applciation */



#ifdef ZW_SLAVE_ROUTING
/**
 * Function called for checking if saved configuration is valid and initialize configuration if not.
 * Called with the initial Region for which the End Device initially has been programmed to use.
 * Used by End Device when being excluded/reset
 */
void SlaveInitDone(zpal_radio_region_t initRegion);

#endif


/*====================   UpdateResponseRouteLastReturnRoute   ================
**    Function called when a new Response Route has been stored.
**    Checks if a Return Route exists for the destination :
**      If the destination is registered as needing a BEAM the
**    Response Route is updated accordingly.
**#ifdef ZW_RETURN_ROUTE_UPDATE
**      For Nodes supporting it, the Response Route is inserted into
**    the Return Route with the lowest priority if the route do not exist
**    allready.
**      Updates Last Return Route/the Return Route with the lowest priority
**    with route in Response Route structure at bResponseRouteIndex if
**    route do not exist allready.
**#endif ZW_RETURN_ROUTE_UPDATE
**
**
**    Side effects:
**      Response Route updated with BEAM info according to Return Route present.
**#ifdef ZW_RETURN_ROUTE_UPDATE
**      Last Return Route/the Return Route with the lowest priority updated
**      with new route if conditions are met.
**#endif ZW_RETURN_ROUTE_UPDATE
**
**--------------------------------------------------------------------------*/
void
UpdateResponseRouteLastReturnRoute(
  uint8_t bResponseRouteIndex);        /* IN Index pointing to the Response Route */
                                    /*    in question */


/*============================   NonsecureAssignStop   ======================
**   This function stops AssignTimer and sets assignState = ELEARNSTATUS_ASSIGN_COMPLETE;
----------------------------------------------------------------------------*/
void ForceAssignComplete(void);

/*----------------------------------------------------------------------------
This function resquest network update from the Static Update Controller
----------------------------------------------------------------------------*/
uint8_t                      /* RET: True; SUC is known to the controller,  */
                          /*      false; SUC not known to the controller */
  ZW_RequestNetWorkUpdate(
    const STransmitCallback* pTxCallback);  /* IN call back function indicates of the update
                                            suceeded or failed*/


/*============================   ZW_RequestNodeInfo   ======================
**    Function description.
**     Request a node to send it's node information.
**     Function return true if the request is send, else it return false.
**     FUNC is a callback function, which is called with the status of the
**     Request nodeinformation frame transmission.
**     If a node sends its node info, an SReceiveNodeUpdate will be passed to app
**     with UPDATE_STATE_NODE_INFO_RECEIVED as status together with the received
**     nodeinformation.
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
bool                      /*RET false if transmitter busy */
ZW_RequestNodeInfo(
  node_id_t nodeID,                     /*IN: node id of the node to request node info from it.*/
  const STransmitCallback* pTxCallback);   /* IN Callback function */


/*=======================   ZW_RequestNewRouteDestinations   =================
**    Request new destinations for return routes.
**      list = pointer to array of node ids
**      len = length of nodeID array (Max len =
**            ZW_MAX_RETURN_ROUTE_DESTINATIONS
**      func = callback, called with status when operation is done.
**             status can be one of the following:
**
**    ZW_ROUTE_UPDATE_DONE      - The update process is ended successfully.
**    ZW_ROUTE_UPDATE_ABORT     - The update process aborted because of error.
**    ZW_ROUTE_UPDATE_WAIT      - The SUC node is busy.
**    ZW_ROUTE_UPDATE_DISABLED  - The SUC functionality is disabled.
**
**--------------------------------------------------------------------------*/
bool                                                /*RET true if SUC/SIS exist false if not*/
ZW_RequestNewRouteDestinations(
  uint8_t* destList,                                  /*IN Pointer to new destinations*/
  uint8_t destListLen,                                 /*IN len of buffer */
  const STransmitCallback* pTxCallback);            /* IN callback function called when completed*/


/*======================  ZW_IsNodeWithinDirectRange   ======================
**    Test if ReturnRouted indicate that bNodeID is within direct range.
**
**--------------------------------------------------------------------------*/
bool                          /*RET TRUE if neighbours, FALSE if not*/
ZW_IsNodeWithinDirectRange(
  node_id_t nodeID);          /*IN nodeID to check*/


/*===========================   ZW_SetDefault   ================================
**    Reset the slave to its default state.
**    Delete all routes in routing slave
**    Reset the homeID and nodeID
**    Side effects:
**
**--------------------------------------------------------------------------*/
void           /*RET  Nothing        */
ZW_SetDefault(void);


#ifdef ZW_SLAVE_ROUTING
/*=======================   ZW_RequestNewRouteDestinations   =================
**    Set new Suc Node ID
**    
**    Only public since Transport changes slave suc node id
**
**--------------------------------------------------------------------------*/
void SlaveSetSucNodeId(uint8_t SucNodeId);
#endif // ifdef ZW_SLAVE_ROUTING

bool NodeSupportsBeingIncludedAsLR(void);


/**
* Function called when PROTOCOL_EVENT_CHANGE_RF_PHY event has been triggered
*
*/
void
ProtocolEventChangeRfPHYdetected(void);

#endif /* _ZW_SLAVE_H_ */

