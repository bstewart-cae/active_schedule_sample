// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file ZW_routing.h
 * @copyright 2022 Silicon Laboratories Inc.
 *
 * @brief Description: Z-Wave routing functions.
 */
#ifndef _ZW_ROUTING_H_
#define _ZW_ROUTING_H_

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <ZW_typedefs.h>
#include <NodeMask.h>

/****************************************************************************/
/*                     EXPORTED TYPES and DEFINITIONS                       */
/****************************************************************************/
typedef struct _BAD_ROUTE_
{
  uint8_t from;
  uint8_t to;
} BAD_ROUTE;

/* Macro wrapper to change value of bCurrentRoutingSpeed.
 * May change to a function call in the future.
 * Use ZW_NODE_SUPPORT_SPEED_* values for sp. */
#define SET_CURRENT_ROUTING_SPEED(sp) (bCurrentRoutingSpeed = sp)

/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/
extern BAD_ROUTE badRoute;

#ifdef ZW_CONTROLLER
/* RangeInfoNeeded return values */
/* - NEIGHBORS_ID_INVALID and NEIGHBORS_COUNT_FAILED defined in ZW_controller_api.h */
#define RANGEINFO_NONEIGHBORS         0x00
#define RANGEINFO_ID_INVALID          NEIGHBORS_ID_INVALID

#define ROUTING_STATE_IDLE      0
#define ROUTING_STATE_ANALYSING 1
#define ROUTING_STATE_SAVING    2
#define ROUTING_STATE_DELETING  3

#ifndef NO_PREFERRED_CALC
extern uint8_t RoutingInfoState;
#endif
#endif

/* Nodemask to be used internally in a function */
extern uint8_t abNeighbors[MAX_NODEMASK_LENGTH];


/* Nodemask to be used when merging several multi-speed range infos */
/* during an inclusion. */
extern uint8_t abMultispeedMergeNeighbors[MAX_NODEMASK_LENGTH];
/* TO#2683 fix update. */
extern uint8_t bMultispeedArrayByteCount;


/* Current routing speed.
 * Routes supporting this speed or lower are returned by GetNextRouteToNode()
 * Allowed values: ZW_GET_ROUTING_INFO_9600
 *                 ZW_GET_ROUTING_INFO_40K
 *                 ZW_GET_ROUTING_INFO_100K
 *
 * Set by calling SET_CURRENT_ROUTING_SPEED(). */
extern uint8_t bCurrentRoutingSpeed;

#ifdef ZW_ROUTE_DIVERSITY
extern uint8_t abLastStartNode[MAX_REPEATERS];
#endif

/****************************************************************************/
/*                              TYPEDEFINES                                 */
/****************************************************************************/

/* Most used entry point table  */
typedef struct _EntryPoint_
{
  uint8_t  bEntryPoint;
  uint8_t  bTimesUsed;
} t_EntryPoint;

typedef struct _wEntryPoint_
{
  uint16_t  wEntryPoint;
} t_wEntryPoint;


#define ENTRY_POINT_TABLE_SIZE  10

typedef union _MostUsedTable
{
  t_EntryPoint    MostUsedEntry[ENTRY_POINT_TABLE_SIZE];
  t_wEntryPoint   MostUsedEntryWord[ENTRY_POINT_TABLE_SIZE];
} t_MostUsedTable;

/****************************************************************************/
/*                              PRIVATE DATA                                */
/****************************************************************************/

/* Defined in ZW_mem_upper.c */
#ifdef ZW_CONTROLLER
/* Nodemask buffers to be used internally in a function */

/* Nodemask to be used internally in a function */
extern uint8_t abNeighbors[MAX_NODEMASK_LENGTH];

/* Nodemask to be used when merging several multi-speed range infos */
/* during an inclusion. */
extern uint8_t abMultispeedMergeNeighbors[MAX_NODEMASK_LENGTH];
extern uint8_t bMultispeedArrayByteCount;

/* Preferred repeaters */
extern uint8_t PreferredRepeaters[MAX_NODEMASK_LENGTH];

extern uint8_t NonRepeaters[MAX_NODEMASK_LENGTH];
#endif

/****************************************************************************/
/*                           EXPORTED FUNCTIONS                             */
/****************************************************************************/

/*============================   IsNodeRepeater   ======================
**    Function description
**      Tests wheter a node is a repeater or not
**    Side effects:
**
**--------------------------------------------------------------------------*/
bool             /*RET true if node is repeater*/
IsNodeRepeater(
  uint8_t bNodeID);


/*=======================   RestartAnalyseRoutingTable  ====================
**
**  Restarts AnalyseRoutingTable
**
**  Side effects:
**
**--------------------------------------------------------------------------*/
void
RestartAnalyseRoutingTable(void);


/*==========================   UpdateMostUsedNodes   =======================
**    Increment or decrement the usage counter for a node in the most
**    used entry table.
**
**    Side effects:
**    Updates MostUsedEntry with new counter value
**
**--------------------------------------------------------------------------*/
void
UpdateMostUsedNodes( /* RET  Nothing */
  bool bLastTxStatus,     /* IN   false Failed to transmit to the node
                                  true  Succesfull transmit to the node */
  uint8_t bEntryPoint);      /* IN   Node id of the entry point used   */

/*========================   RoutingInfoReceived   ==========================
**
**  Start processing the routing info received from a node
**
**  Side effects:
**
**--------------------------------------------------------------------------*/
uint8_t
RoutingInfoReceived(       /* RET  false if unable to save the routing info */
  uint8_t bLength,            /* IN   Length of range info buffer */
  uint8_t bNode,              /* IN   Node number */
  uint8_t *pRangeInfo);       /* IN   Range info pointer */


/*===========================   ClearMostUsed   ============================
**
**    Clear most used table
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
void
ClearMostUsed(  /* RET Nothing */
  void);


/*===========================   DeleteMostUsed   ============================
**
**    Delete a node from most used table
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
void
DeleteMostUsed(  /* RET Nothing */
  uint8_t bNodeID);      /* IN   Node number */


/*=========================   RangeInfoNeeded   ===========================
**
**  Check if it is necessary to get new range information from a node
**
**  Side effects:
**
**--------------------------------------------------------------------------*/
uint8_t
RangeInfoNeeded(  /* RET  true if range info should be updated */
  uint8_t bNodeID);       /* IN   Node ID */

/*========================   SetPendingDiscovery   ==========================
**
**  Sets the specified node bit in the pending discovery list
**
**  Side effects:
**
**--------------------------------------------------------------------------*/
void
SetPendingDiscovery( /* RET  Nothing */
  uint8_t bNodeID);          /* IN   Node ID that should be set */

/*=======================   ClearPendingDiscovery   =========================
**
**  Clear the specified node bit in the pending discovery list
**
**
**  Side effects:
**
**--------------------------------------------------------------------------*/
void
ClearPendingDiscovery( /* RET  Nothing */
  uint8_t bNodeID);            /* IN   Node ID that should be set */


/*=============================   HasNodeANeighbour   ========================
**
**  Do a node have any neighbors
**
**
**  Side effects:
**
**--------------------------------------------------------------------------*/
bool
ZCB_HasNodeANeighbour(  /*RET true if any neighbours */
  uint8_t bNode);           /* IN Node ID on node to test for neighbours */

/*===========================   FindBestRouteToNode   ========================
**
**    Find the best route to a node based on first repeater and destination
**    node. The function uses a width first tree search algorithm to ensure
**    that the shortest route is found .
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
uint8_t                     /* RET  Number of hops to destination, or 0xFF if no route. */
FindBestRouteToNode(
  uint8_t bRepeater,        /* IN   Repeater that source node can see.  */
  uint8_t bDestination,     /* IN   Destination node ID. */
  uint8_t *pHops,           /* OUT  Pointer to repeater count */
  uint8_t *pRouting);       /* OUT  Pointer to repeater list. */


/*========================   GetNextRouteToNode   ==========================
**
**  Finds the next route to the specified node
**
**
**  Side effects:
**
**--------------------------------------------------------------------------*/
uint8_t
GetNextRouteToNode(     /* RET  true - New route found,
                         * false - No more routes */
#ifdef ZW_ROUTE_DIVERSITY
  uint8_t  bNodeID,           /* IN   Node ID that route should be found to */
  uint8_t  bCheckMaxAttempts, /* IN Should number of routing attempt have a MAX */
  uint8_t *pRepeaterList,     /* OUT  Pointer to new repeater list */
  uint8_t *pHopCount,         /* OUT  Pointer to hop count */
  uint8_t *speed              /* OUT  Speed of the new route */
);
#else
  uint8_t  bNodeID,        /* IN   Node ID that route should be found to */
  uint8_t  bLastNode,      /* IN   Node ID that this function was called
                         * with last time */
  uint8_t *pRepeaterList,  /* OUT  Pointer to new repeater list */
  uint8_t *pHopCount);     /* OUT  Pointer to hop count */
#endif

/*=========================   MergeNoneListeningNodes   ======================
**  Merge the allready neighboring none listening nodes into the new received
**  rangeInfo frame, which then can be saved as the new rangeinfo
**
**  Side effects:
**
**--------------------------------------------------------------------------*/
void
MergeNoneListeningNodes( /* RET  Nothing */
  register uint8_t sourceNode,          /* IN   Node ID on node which the rangeInfo are meant for */
  uint8_t* pRangeFrame);        /* OUT  Pointer to Rangeinfo frame in  to update */


/*======================   MergeNoneListeningSensorNodes   ===================
**  Merge the allready neighboring nodes into the new received
**  rangeInfo frame for sensor node, which then can be saved as the new rangeinfo
**
**  Side effects:
**
**--------------------------------------------------------------------------*/
void
MergeNoneListeningSensorNodes(
  register uint8_t sourceNode,
  uint8_t* pRangeFrame);


#ifdef NO_PREFERRED_CALC
/*===========================   PreferredSet   ============================
**
**    Set bNodeID as preferred repeater
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
void
PreferredSet(
  uint8_t bNodeID);


/*===========================   PreferredRemove   ============================
**
**    Remove bNodeID as preferred repeater
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
void
PreferredRemove(
  uint8_t bNodeID);
#endif


/*=========================   AnalyseRoutingTable  =========================
**
**  Analyse the routing table and find all preferred repeaters in the
**  network. The found repeaters should be able to reach the whole network.
**
**  The strategy for finding repeaters is to look for the repeaters that can
**  se as many new nodes as possible and still has a connection to another
**  repeater.
**
**  Side effects:
**
**--------------------------------------------------------------------------*/
void
AnalyseRoutingTable(void); /* RET  Nothing */


/*=========================   InitRoutingValues   ==========================
**
**  Initialize the values in memory that are used in the routing
**
**
**  Side effects:
**
**--------------------------------------------------------------------------*/
void
InitRoutingValues(void); /* RET  Nothing */


/*========================   ZW_SetRoutingInfo   ==========================
**
**  Save a list of routing information for a node in the routing table
**
**  Side effects:
**
**--------------------------------------------------------------------------*/
bool              /*RET true The info contained changes and was physically written to EEPROM */
ZW_SetRoutingInfo(       /* RET  Nothing */
  uint8_t bNode,               /* IN   Node number */
  uint8_t bLength,             /* IN   Length of pMask in bytes */
  uint8_t* pMask);            /* IN   pointer to routing info */


/*========================   RoutingAddNonRepeater   ==========================
**
**  Add a now node to the non-repeater list.
**
**  Side effects:
**
**--------------------------------------------------------------------------*/
void
RoutingAddNonRepeater( /* RET Nothinf */
  uint8_t bNodeID);            /* IN  Node ID */

/*========================   RoutingRemoveNonRepeater   ======================
**
**  Remove a node from the non-repeater list.
**
**  Side effects:
**
**--------------------------------------------------------------------------*/
void
RoutingRemoveNonRepeater( /* RET Nothinf */
  uint8_t bNodeID);            /* IN  Node ID */


/*===================   GetNextRouteFromNodeToNode   ========================
**
**  Finds the next route to the specified node
**
**
**  Side effects:
**
**--------------------------------------------------------------------------*/
uint8_t                         /* RET  true - New route found, false - No more routes */
GetNextRouteFromNodeToNode(
  uint8_t  bDestNodeID,             /* IN   Node ID that route should be found to */
  uint8_t  bSourceNodeID,           /* IN   Node ID that route should be found from */
  uint8_t *pRepeaterList,           /* OUT  Pointer to new repeater list */
  uint8_t *pHopCount,               /* OUT  Pointer to hop count */
  uint8_t bRouteNumber,             /* IN   Route Number */
  bool fResetNextNode);          /* IN   reset bNextNode if true*/


#ifndef NO_PREFERRED_CALC
/*==========================   RoutingAnalysisStop   ========================
**
**  Stop the routing analysis process.
**
**  Side effects:
**
**--------------------------------------------------------------------------*/
void
RoutingAnalysisStop(void);

#define ROUTING_ANALYSIS_STOP() RoutingAnalysisStop()
#else

#define ROUTING_ANALYSIS_STOP()
#endif

/*==========================   MarkSpeedAsTried   ============================
**
**  Marks a speed as tried for routing on a frame in the TxQueue. New routes
**  that support an already tried speed will be dropped for that frame.
**
**  Side effects:
**
**--------------------------------------------------------------------------*/
void
MarkSpeedAsTried(
uint8_t speed,             /* IN The speed to be marked as tried */
uint8_t* pRoutInfo);       /* OUT The routing info that should be changed
                         *     Format: ROUTING_INFO_TRIED_SPEED_* */

/*==========================   ClearTriedSpeeds   ============================
**
**  Clear tried routing speed for a frame in the TxQueue.
**
**  Side effects:
**
**--------------------------------------------------------------------------*/
void
ClearTriedSpeeds(
uint8_t* pRoutInfo); /* OUT The routing info that should be cleared */

/*=====================   DoesRouteSupportTriedSpeeds   =======================
**
**  Check if route is supported by speeds previously tried while routing this
**  frame.
**  Allows discarding routes that were already tried at a different speed.
**
**  Side effects:
**
**--------------------------------------------------------------------------*/
bool                        /*RET true if route supported by tried speeds,
                             *    false otherwise */
DoesRouteSupportTriedSpeed(
uint8_t *pRepeaterList,        /*IN  List of repeaters in route */
uint8_t bHopCount,
uint8_t bTriedSpeeds);          /*IN  Tried speeds, ROUTING_INFO_TRIED_SPEED_* */

/*==========================   NextLowerSpeed   ============================
**
**  Finds the next speed lower than bCurrentRoutingSpeed that is supported
**  by both source and destination node.
**  Returns true if a route is found.
**  Returns false and sets bCurrentRoutingSpeed to 0 if no route is found.
**
**  Side effects: Decrements bCurrentRoutingSpeed.
**
**--------------------------------------------------------------------------*/
bool               /*RET true if speed found, false if not */
NextLowerSpeed(
uint8_t bSrcNodeID,   /*IN  Source node ID */
uint8_t bDestNodeID);  /*IN  Destination node ID */

#ifndef ZW_3CH_SYSTEM
/*=====================   DoesRouteSupportHigherSpeed   =======================
**
**  Check if a route supports a higher speed than bRouteSpeed.
**
**  Side effects:
**
**--------------------------------------------------------------------------*/
bool                          /* RET true/false */
DoesRouteSupportHigherSpeed(
    uint8_t *pRepeaterList,      /* IN repeater list */
    uint8_t bRepeaterCount,      /* IN repeater count */
    uint8_t bRouteSpeed,         /* IN Route speed to check against */
    uint8_t bDestID              /* IN Node ID of the destination node */
);
#endif /* !ZW_3CH_SYSTEM */

#if defined(ZW_DYNAMIC_TOPOLOGY_ADD) || defined(ZW_DYNAMIC_TOPOLOGY_DELETE)
/*============================   SetRoutingLink   ============================
**    Set or Remove a link between bANodeID and bBNodeID in the EEPROM
**    based RoutingTable
**
**    Side effects:
**      Updates EEPROM_ROUTING_TABLE with new link state
**      between bANodeID and bBNodeID
**
**--------------------------------------------------------------------------*/
void
SetRoutingLink(
  uint8_t bANodeID,
  uint8_t bBNodeID,
  bool _bset);
#endif
#endif /* _ZW_ROUTING_H_ */
