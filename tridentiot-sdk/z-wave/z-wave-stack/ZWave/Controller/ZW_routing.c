// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file ZW_routing.c
 * @copyright 2022 Silicon Laboratories Inc.
 *
 * @brief Routing functions used in a controller.
 */
#include "ZW_lib_defines.h"

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <ZW_controller_api.h>
#include <ZW_controller.h>
#include <ZW_protocol.h>
#include <ZW_transport.h>
#include <ZW_routing.h>
#include <ZW_tx_queue.h>
#include <SwTimer.h>
#include <TickTime.h>
#include <string.h>
#include <ZW_DataLinkLayer.h>
#include <zpal_watchdog.h>

#ifdef ZW_CONTROLLER
#include <ZW_controller_network_info_storage.h>
#endif

//#define DEBUGPRINT
#include "DebugPrint.h"

/****************************************************************************/
/*                                EXTERNAL DATA                             */
/****************************************************************************/
#ifdef DEBUGPRINT
/* From ZW_txq_protocol.c */
extern TxQueueElement *pFrameWaitingForACK;
#endif

/****************************************************************************/
/*                      PRIVATE TYPES and DEFINITIONS                       */
/****************************************************************************/
/* Function prototypes */
void SavePreferredRepeaters(void);

/* Defines used to keep track of which speeds were previously attempted during routing. */
#define ROUTING_INFO_TRIED_SPEED_9600     0x10
#define ROUTING_INFO_TRIED_SPEED_40K      0x20
#define ROUTING_INFO_TRIED_SPEED_100K     0x40
#define ROUTING_INFO_TRIED_SPEED_200K     0x80
#define ROUTING_INFO_TRIED_SPEED_RESERVED 0x0F

/* If a route calculation is taking longer than ROUTING_MAX_CALC_TIME 10ms ticks */
/* then we deem the routecalculation failed - For now 2.5 seconds */
#define ROUTING_MAX_CALC_TIME                 2500//0x00FA
#define ROUTING_MAX_LAST_REPEATER_CALC_TIME   320//0x0020

#ifndef NO_PREFERRED_CALC
/*Indicates AnalyseRoutingtable's state*/
uint8_t RoutingInfoState = ROUTING_STATE_IDLE;
#endif

#ifdef NO_PREFERRED_CALC
/* Nodemask buffers to be used internally in a function */
static uint8_t NextLevel[3][MAX_NODEMASK_LENGTH];
#else
/* Nodemask buffers to be used internally in a function */
static uint8_t NextLevel[4][MAX_NODEMASK_LENGTH];
#endif
/* Lock for NextLevel in use in AnalyseRoutingTable  */
#ifndef NO_PREFERRED_CALC
static bool RoutingBufferLocked = false;
#endif

/* Nodemask to be used internally in a function */
uint8_t abNeighbors[MAX_NODEMASK_LENGTH];

/* Nodemask to be used when merging several multi-speed range infos */
/* during an inclusion. */
uint8_t abMultispeedMergeNeighbors[MAX_NODEMASK_LENGTH];
/* TO#2683 fix update. */
uint8_t bMultispeedArrayByteCount;

/* Preferred repeaters */
uint8_t PreferredRepeaters[MAX_NODEMASK_LENGTH];

/* Routing analyse variables */
#define ANALYSE_STATE_IDLE            0
#define ANALYSE_STATE_START           1
#define ANALYSE_STATE_TABLE_READ      2
#define ANALYSE_STATE_REPEATER_READ   3
#define ANALYSE_STATE_SAVE_REPEATERS  4

#ifndef NO_PREFERRED_CALC
static uint8_t analyseState = ANALYSE_STATE_IDLE;
#endif

BAD_ROUTE badRoute = { 0 };

uint8_t NonRepeaters[MAX_NODEMASK_LENGTH];

/* TO#1984 fix - now tries 5 routes before falling back to explore scheme */
/* Default max route attempts before, if possible/wanted, fallback to explore frame */
#define MAX_ROUTING_ATTEMPTS   5

uint8_t bRouteAttemptsMAX = MAX_ROUTING_ATTEMPTS;
uint8_t bRouteAttempts;



/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/


#define MOST_USED_MAX_COUNT     0x1f
#define ENTRY_POINT_TIME_MASK   0x1f
#define ENTRY_POINT_USED_BIT    0x80

#if 0
/* TODO - something with this */
#define ENTRY_POINT_USED_BIT_100K 0x20
#define ENTRY_POINT_USED_BIT_40K  0x40
#define ENTRY_POINT_USED_BIT_9600 0x80
#endif

#define ENTRY_POINT_40K_NODES		0x05	/* Number of entry points reserved for 40k nodes */
#define ENTRY_POINT_40K_NODE		0x40

t_MostUsedTable mostUsedTable = { 0 };

/* Here we hide the last nodeID for every hop level - so that we can make the */
/* routes spread more out -> adding more diversity to the routing */
uint8_t abLastNode[MAX_REPEATERS];
uint8_t abLastStartNode[MAX_REPEATERS];

/* Current routing speed.
 * Routes supporting this speed or lower are returned by GetNextRouteToNode()
 * Allowed values: ZW_GET_ROUTING_INFO_9600
 *                 ZW_GET_ROUTING_INFO_40K
 *                 ZW_GET_ROUTING_INFO_100K
 *                 ZW_GET_ROUTING_INFO_100KLR
 *
 * Set by calling SET_CURRENT_ROUTING_SPEED(). */
uint8_t bCurrentRoutingSpeed;

uint8_t
OldGetNextRouteToNode(     /* RET  true - New route found,
                            * false - No more routes */
  uint8_t  bNodeID,        /* IN   Node ID that route should be found to */
  uint8_t *pRepeaterList,  /* OUT  Pointer to new repeater list */
  uint8_t *pHopCount       /* OUT  Pointer to hop count */
);

/****************************************************************************/
/*                            PRIVATE FUNCTIONS                             */
/****************************************************************************/
#ifdef DEBUGPRINT
void
SendMostUsedEntryTable(void)
{
  uint8_t i;
  for (i = 0; i < ENTRY_POINT_TABLE_SIZE; i++)
  {
    DPRINTF(" %02X-%02X",
            mostUsedTable.MostUsedEntry[i].bEntryPoint,
            mostUsedTable.MostUsedEntry[i].bTimesUsed
            );
  }
}


void
SendRoutingTable(
  uint8_t iMax,
  uint8_t bSpeed)
{
  uint8_t i,t, tmpspd;

  DPRINT("\r\n");

  tmpspd = bCurrentRoutingSpeed;
  SET_CURRENT_ROUTING_SPEED(bSpeed);
  for (i = 1; i <= iMax; i++)
  {
    for (t = 1; t <= iMax; t++)
    {
      if (ZW_AreNodesNeighbours(i, t))
        DPRINT("1");
      else
        DPRINT("0");
    }
    DPRINT("\r\n");
  }
  SET_CURRENT_ROUTING_SPEED(tmpspd);
}

void SendBitmask(uint8_t *nodelist)
{
  int i;

  for (i=1; i<9; i++)
  {
    if (ZW_NodeMaskNodeIn(nodelist, i))
    {
      DPRINT("1");
    }
    else
    {
      DPRINT("0");
    }
  }
  DPRINT("b");
}

#endif  // #ifdef DEBUGPRINT


#ifndef NO_PREFERRED_CALC
/*=======================   RoutingWriteComplete   ==========================
**
**  Release routing buffer
**
**  Side effects:
**
**--------------------------------------------------------------------------*/
static void
RoutingWriteComplete(void)
{
  if (RoutingInfoState == ROUTING_STATE_DELETING)
  {
    RoutingInfoState = ROUTING_STATE_ANALYSING;
  }
  if (RoutingInfoState == ROUTING_STATE_ANALYSING)
  {
    /* Routing table has been updated, re-calculate the preferred
       repeaters list */
    analyseState = ANALYSE_STATE_START;
    RoutingBufferLocked = true; /* Now we are locked - meaning the NextLevel buffers are in use */
  }
  else if (RoutingInfoState == ROUTING_STATE_SAVING)
  {
    /* save the preferred repeaters list */
    SavePreferredRepeaters();
    RoutingBufferLocked = false;  /* Unlock NextLevel buffers, so its possible to route again */
    RoutingInfoState = ROUTING_STATE_IDLE;
  }
}
#endif


/*========================   ZW_SetRoutingMAX   ==============================
**
**  Set the maximum number of route tries which should be done before failing
**  or resorting to explore if this is specified. ZERO Means only
**
**  Side effects:
**
**--------------------------------------------------------------------------*/
void
ZW_SetRoutingMAX(
  uint8_t maxRouteTries)
{
  bRouteAttemptsMAX = maxRouteTries;
}

/*========================   ZW_GetRoutingInfo   ==========================
**
**  Get a list of routing information for a node from the routing table.
**  Only include neighbor nodes supporting a certain speed.
**  Assumes that nodes support all lower speeds in addition to the advertised
**  highest speed.
**
**  Side effects:
**
**--------------------------------------------------------------------------*/
void
ZW_GetRoutingInfo(        /*RET  Nothing */
  node_id_t nodeID,       /* IN  Node ID on node whom routing info is needed on */
  uint8_t*  pMask,        /* OUT Pointer where routing info should be put */
  uint8_t   bOptions)     /* IN  Upper nibble is bit flag options, lower nibble is speed */
                          /*     Combine exactly one speed with any number of options */
                          /*     Bit flags options for upper nibble: */
                          /*       GET_ROUTING_INFO_REMOVE_BAD      - Remove bad link from routing info */
                          /*       GET_ROUTING_INFO_REMOVE_NON_REPS  - Remove non-repeaters from the routing info */
                          /*     Speed values for lower nibble:     */
                          /*       ZW_GET_ROUTING_INFO_ANY    - Return all nodes regardless of speed */
                          /*       ZW_GET_ROUTING_INFO_9600   - Return nodes supporting 9.6k    */
                          /*       ZW_GET_ROUTING_INFO_40K    - Return nodes supporting 40k     */
                          /*       ZW_GET_ROUTING_INFO_100K   - Return nodes supporting 100k    */
						  /*       ZW_GET_ROUTING_INFO_100KLR - Return nodes supporting 100k LR   */
{
  if (nodeID > ZW_MAX_NODES)
  {
    return;
  }

  register uint8_t t;
  uint8_t bSpeed = bOptions & ZW_GET_ROUTING_INFO_SPEED_MASK;

  /* Read line in routing table */
  CtrlStorageGetRoutingInfo(nodeID, (NODE_MASK_TYPE *)pMask);

  if ((0 == llIsHeaderFormat3ch()) && (bSpeed != ZW_GET_ROUTING_INFO_ANY))
  {
    if (!DoesNodeSupportSpeed(nodeID, bSpeed))
    {
      ZW_NodeMaskClear(pMask, MAX_NODEMASK_LENGTH);
    }
    else
    {
      /* Remove all neighbours that do not support bSpeed */
      for (t = 1; t <= ZW_MAX_NODES; t++)
      {
        if ((!DoesNodeSupportSpeed(t, bSpeed)) && (ZW_NodeMaskNodeIn(pMask, t)))
        {
          ZW_NodeMaskClearBit(pMask, t);
        }
      }
    }
  }

  /* Remove last bad link from routing */
  if (bOptions & GET_ROUTING_INFO_REMOVE_BAD)
  {
    if (badRoute.from == nodeID)
    {
      ZW_NodeMaskClearBit(pMask, badRoute.to);
    }
    if (badRoute.to == nodeID)
    {
      ZW_NodeMaskClearBit(pMask, badRoute.from);
    }
  }

  if (bOptions & GET_ROUTING_INFO_REMOVE_NON_REPS)
  {
    /* Remove non repeater nodes */
    for (t = 0; t < MAX_NODEMASK_LENGTH; t++)
    {
      pMask[t] &= ~(NonRepeaters[t]);
    }
  }
}


/*=========================   MergeNoneListeningNodes   ======================
**  Merge the allready neighboring none listening nodes into the new received
**  rangeInfo frame, which then can be saved as the new rangeinfo
**
**  Side effects:
**
**--------------------------------------------------------------------------*/
void
MergeNoneListeningNodes(
  register uint8_t sourceNode,
  uint8_t* pRangeFrame)
{
  uint8_t bTemp;
  // Get old routing line including none listening nodes
  // TODO : This should probably be ZW_GET_ROUTING_INFO_ANY
  ZW_GetRoutingInfo(sourceNode, abNeighbors, ZW_GET_ROUTING_INFO_ANY);
  /* TO#3066 fix - Only nonelistening nodes is to be merged in again. */
  /* First ZERO all none defined maskbytes in pRangeFrame */
  for (bTemp = ((RANGEINFO_FRAME*)pRangeFrame)->numMaskBytes; bTemp < MAX_NODEMASK_LENGTH; bTemp++)
  {
    if (((RANGEINFO_FRAME*)pRangeFrame)->numMaskBytes < bTemp + 1)
    {
      ((RANGEINFO_FRAME*)pRangeFrame)->numMaskBytes++;
      ((RANGEINFO_FRAME*)pRangeFrame)->maskBytes[bTemp] = 0;  /* ZERO byte as this is an undefined byte in frame */
    }
  }
  /* Check all previously neighbouring nodes if nonelistening and if so, merge them into pRangeFrame */
  bTemp = 0;
  do
  {
    /* Find next available node in network */
    bTemp = ZW_NodeMaskGetNextNode(bTemp, abNeighbors);
    if (bTemp && !(CtrlStorageListeningNodeGet(bTemp)))
    {
      /* Node found and node is none listening - Add to received rangeinfo */
      ZW_NodeMaskSetBit(((RANGEINFO_FRAME*)pRangeFrame)->maskBytes, bTemp);
    }
  } while (bTemp);
}


/*======================   MergeNoneListeningSensorNodes   ===================
**  Merge the already neighboring nodes into the new received
**  rangeInfo frame for sensor node, which then can be saved as the new rangeinfo
**
**  Side effects:
**
**--------------------------------------------------------------------------*/
void
MergeNoneListeningSensorNodes(
  register uint8_t sourceNode,
  uint8_t* pRangeFrame)
{
  uint8_t bTemp;
  // Get old routing line including none listening nodes
  ZW_GetRoutingInfo(sourceNode, abNeighbors, ZW_GET_ROUTING_INFO_ANY);
  for (bTemp = 0; bTemp < MAX_NODEMASK_LENGTH; bTemp++)
  {
    if (((RANGEINFO_FRAME*)pRangeFrame)->numMaskBytes < bTemp + 1)
    {
      ((RANGEINFO_FRAME*)pRangeFrame)->numMaskBytes++;
      ((RANGEINFO_FRAME*)pRangeFrame)->maskBytes[bTemp] = 0;  /* ZERO byte as this is an undefined byte in frame */
    }
  }
  for (bTemp = 1; bTemp <= bMaxNodeID; bTemp++)
  {
    /* TO#4649 fix - Only merge the NoneListening Sensor nodes */
    if (CtrlStorageCacheNodeExist(bTemp))
    {
      if ((IsNodeSensor(bTemp, false, false)) && (ZW_NodeMaskNodeIn(((RANGEINFO_FRAME*)pRangeFrame)->maskBytes, bTemp)))
      {
        ZW_NodeMaskSetBit(abNeighbors, bTemp);
      }
      else
      {
        ZW_NodeMaskClearBit(abNeighbors, bTemp);
      }
    }
  }
  memcpy(((RANGEINFO_FRAME*)pRangeFrame)->maskBytes, abNeighbors, MAX_NODEMASK_LENGTH);
}

/*=========================   FindLastRepeater   ============================
**
**  Find the best route to a node based on first repeater and destination
**  node, it is only the last repeater that is found because it isn't
**  possible to record the path througt the network when using a width
**  first search algorithm.
**
**  Side effects:
**
**--------------------------------------------------------------------------*/
uint8_t                 /*RET  Number of hops to destination, or 0xFF if failed. */
FindLastRepeater(
  uint8_t *pRouting,    /* IN  Pointer to routing field in message. */
  uint8_t bRepeater,    /* IN  Repeater that source node can see. */
  uint8_t bDestination, /* IN  Destination node ID. */
  bool bFirst)       /* IN  First call for this route */
{
  uint8_t * const RoutingInfo = NextLevel[2];

  uint8_t bNodeMsk = 0;
  uint8_t bHops;
  uint8_t r;
  register uint8_t t, t1;
  uint32_t wStartEntryTickTimeSample;

  wStartEntryTickTimeSample = getTickTime();

  ROUTING_ANALYSIS_STOP();
#ifndef NO_PREFERRED_CALC
  if (RoutingBufferLocked)
  {
    return(0xFF); /* buffers are busy */
  }
#endif
  DPRINT("L");

  /* TO#2632, TO#1831 fix */
  ZW_GetRoutingInfo(bDestination, RoutingInfo,
                    bCurrentRoutingSpeed | GET_ROUTING_INFO_REMOVE_BAD | GET_ROUTING_INFO_REMOVE_NON_REPS);

  if (!ZW_NodeMaskBitsIn(RoutingInfo, MAX_NODEMASK_LENGTH))
  {
    return(0xFF); /* no neighbors  */
  }

  /* Clear temp nodemasks */
  ZW_NodeMaskClear(NextLevel[0], MAX_NODEMASK_LENGTH);
  ZW_NodeMaskClear(NextLevel[1], MAX_NODEMASK_LENGTH);
  /* Set starting point in first temp nodemask */
  ZW_NodeMaskSetBit(NextLevel[0], bRepeater);
  /* Add first hop to message */
  bHops = 0;
  /* Loop until route is found or max hops is exceeded */
  while (bHops < MAX_REPEATERS)
  {
    /* Check to see if any repeaters in this level can see the */
    /* destination node */
    /* TODO - Here we should use the bHopNodeNext[] so we can get some diversity into the routes */
    if (++abLastNode[bHops] > bMaxNodeID)
    {
      abLastNode[bHops] = 1;
    }
    r = abLastNode[bHops];
    do
    {
      zpal_feed_watchdog();
      if (CtrlStorageCacheNodeExist(r) && ZW_NodeMaskNodeIn(NextLevel[bNodeMsk], r))
      {
        /* Get routing info for node r */
        /* If first call then include non-repeaters because the destination could */
        /* be a non-repeater node */
        ZW_GetRoutingInfo(r, RoutingInfo, bCurrentRoutingSpeed | GET_ROUTING_INFO_REMOVE_BAD
           | (bFirst ? 0 : GET_ROUTING_INFO_REMOVE_NON_REPS));
        DPRINTF("%02X,", r);
#ifdef DEBUGPRINT
        SendBitmask(RoutingInfo);
#endif

        /* Check if destination node is in repeater r's range */
        if (ZW_NodeMaskNodeIn(RoutingInfo, bDestination) &&
            (!IsNodeSensor(bDestination, false, false) || (GetNodesSecurity(r) & ZWAVE_NODEINFO_BEAM_CAPABILITY)))
        {
          /* It was, return the number of hops */
          *(pRouting + bHops) = r;
          abLastNode[bHops] = r;
          return (bHops + 1);
        }
        else
        {
          /* The destination node wasn't found through this repeater, add all */
          /* repeaters that this repeater (r) can see to the next search level */
          /* Remove non-repeaters for first call */
          if (bFirst)
          {
            ZW_GetRoutingInfo(r, RoutingInfo, bCurrentRoutingSpeed |
               GET_ROUTING_INFO_REMOVE_NON_REPS | GET_ROUTING_INFO_REMOVE_BAD);
          }
          for (t = 0; t < MAX_NODEMASK_LENGTH; t++)
          {
            NextLevel[bNodeMsk^1][t] |= RoutingInfo[t];
          }
        }
      }
      if (++r > bMaxNodeID)
      {
        r = 1;
      }
    } while ((r != abLastNode[bHops]) && (ROUTING_MAX_LAST_REPEATER_CALC_TIME >= getTickTimePassed(wStartEntryTickTimeSample)));
    /* TO#5854 fix */
    if (ROUTING_MAX_LAST_REPEATER_CALC_TIME < getTickTimePassed(wStartEntryTickTimeSample))
    {
      /* Bail... Timeout - max time has been used for calculating new route - No route */
      break;
    }
    /* Destination wasn't found in this repeater level, go to next level */
    /* of repeaters and search here */
    bHops++;
    /* Remove all repeaters that we have already searched from nodemask */
    for (t1 = 0; t1 < MAX_NODEMASK_LENGTH; t1++)
    {
      NextLevel[bNodeMsk^1][t1] ^= NextLevel[bNodeMsk][t1];
    }
    /* Clear current temp nodemaks and switch to the one we build based on the */
    /* previous loop */
    ZW_NodeMaskClear(NextLevel[bNodeMsk], MAX_NODEMASK_LENGTH);
    bNodeMsk ^= 1;
  }
  return 0xFF;
}


/*===========================   FindBestRouteToNode   ========================
**
**    Find the best route to a node based on first repeater and destination
**    node. The function uses a width first tree search algorithm to ensure
**    that the shortest route is found .
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
uint8_t
FindBestRouteToNode(  /* RET  Number of hops to destination, or 0xFF if no route. */
  uint8_t bRepeater,     /* IN   Repeater that source node can see.  */
  uint8_t bDestination,  /* IN   Destination node ID. */
  uint8_t *pHops,        /* OUT  Pointer to repeater count */
  uint8_t *pRouting)     /* OUT  Pointer to repeater list. */
{
  uint8_t bHopsLeft, bNewDestination, bMaxHops = 0;
  bool bFirst = true;

  DPRINTF("N%02X", bCurrentRoutingSpeed);
  bNewDestination = bDestination;
  bHopsLeft = MAX_REPEATERS;
  while (bHopsLeft != 0xFF)
  {
    bHopsLeft = FindLastRepeater(pRouting, bRepeater, bNewDestination, bFirst);

    bFirst = false;
    if (bHopsLeft > bMaxHops)
    {
      bMaxHops = bHopsLeft;
    }

    if (bHopsLeft == 1)
    {
      *pHops = bMaxHops;
      return bMaxHops;
    }
    bNewDestination = *(pRouting + bHopsLeft - 1);
  }
  return 0xFF;
}

#ifndef NO_PREFERRED_CALC
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
AnalyseRoutingTable(void) /* RET  Number of found repeaters */
{
  static uint8_t t, i, l;
  static bool bFirst;
  static uint8_t bBestRepeater, bBestNewCount;
  static uint8_t TempNodeMask[MAX_NODEMASK_LENGTH];

  uint8_t * const NewNodes = NextLevel[0];
  uint8_t * const OldNodes = NextLevel[1];
  uint8_t * const NodesReached = NextLevel[2];
  uint8_t * const tempPreferredRepeaters = NextLevel[3];

  if (analyseState == ANALYSE_STATE_IDLE)
  {
    return;
  }
  if (l)
  { /* let other functions execute */
    l--;
    return;
  }
  find40KRoute = false;
  switch (analyseState)
  {
    case ANALYSE_STATE_START:
      DPRINT("A>");
      /* Clear Preferred repeaters */
      ZW_NodeMaskClear(tempPreferredRepeaters, MAX_NODEMASK_LENGTH);
      ZW_NodeMaskClear(NodesReached, MAX_NODEMASK_LENGTH);
      bFirst = 1;
      t = 0;
      analyseState = ANALYSE_STATE_TABLE_READ;
      break;

    case ANALYSE_STATE_TABLE_READ:
#ifdef DEBUGPRINT
      uint16_t tickTimeDebug = getTickTime();
      DPRINTF("t%02X%02X", t, tickTimeDebug & 0xff);
#endif
      /* Analyse routing table to find repeaters */
      /* Get line from routing table */
      ZW_GetRoutingInfo(t + 1, TempNodeMask, false, true);
      l = 2;
      /* If empty mask then skip to next */
#ifdef ZW_REPEATER
      if ((t != (nodeID - 1)) && ZW_NodeMaskBitsIn(TempNodeMask, MAX_NODEMASK_LENGTH))
#else
      if (ZW_NodeMaskBitsIn(TempNodeMask, MAX_NODEMASK_LENGTH))
#endif
      {
        bBestRepeater = 0;
        bBestNewCount = 0;
        /* Clear temp node masks */
        ZW_NodeMaskClear(NewNodes, MAX_NODEMASK_LENGTH);
        ZW_NodeMaskClear(OldNodes, MAX_NODEMASK_LENGTH);
        i = 1;
        analyseState = ANALYSE_STATE_REPEATER_READ;
      }
      else
      {
        if (++t >= bMaxNodeID)
        {
          analyseState = ANALYSE_STATE_SAVE_REPEATERS;
        }
      }
      break;

    case ANALYSE_STATE_REPEATER_READ:
      DPRINTF("i%02X", i);
      /* Run through the entire routing table and find the best repeaters */
      /* Node found start to check if this should be a repeater */
#ifdef ZW_REPEATER
      if ((i != nodeID) && ZW_NodeMaskNodeIn(TempNodeMask, i))
#else
      if ( ZW_NodeMaskNodeIn(TempNodeMask, i) )
#endif
      {
        /* Check if found bit is already a repeater, break search for
           this node if it is  */
        if (ZW_NodeMaskNodeIn(tempPreferredRepeaters, i))
        {
          bBestRepeater = 0xFF;
          i = ZW_MAX_NODES; /* break */
        }
        else
        {
          /* Get line from routing table */
          ZW_GetRoutingInfo(i, TempNodeMask, false, true);
          l = 7;
          /* Bit is set, build mask of new nodes this node can see */
          for (uint8_t r = 0; r < MAX_NODEMASK_LENGTH; r++)
          {
            NewNodes[r] = ((NodesReached[r] | TempNodeMask[r]) ^ NodesReached[r]);
            OldNodes[r] = (NodesReached[r] & TempNodeMask[r]);
          }
          /* Number of new nodes */
          uint8_t bNewCount = ZW_NodeMaskBitsIn(NewNodes, MAX_NODEMASK_LENGTH);
          uint8_t bOldCount = ZW_NodeMaskBitsIn(OldNodes, MAX_NODEMASK_LENGTH);
          /* If this repeater can see more new nodes then use this one */
          if (bNewCount > bBestNewCount)
          {
            if (bFirst || (bOldCount))
            {
              bBestRepeater = i;
              bBestNewCount = bNewCount;
            }
          }
        }
      }
      if (++i <= bMaxNodeID)
      {
        break;
      }
      analyseState = ANALYSE_STATE_TABLE_READ;
      /* Add found repeater (if any) to preferred repeaters */
      if (bBestRepeater != 0xff)
      {
        bFirst = 0;
        ZW_NodeMaskSetBit(tempPreferredRepeaters, bBestRepeater);
        /* Get line from routing table */
        ZW_GetRoutingInfo(bBestRepeater, TempNodeMask, false, true);
        DPRINTF("R%02X", bBestRepeater);

        /* Add Nodes this repeater can see to NodesReached */
        for (r = 0; r < MAX_NODEMASK_LENGTH; r++)
        {
          NodesReached[r] |= TempNodeMask[r];
        }
        /* Add found repeater to NodesReached */
        ZW_NodeMaskSetBit(NodesReached, bBestRepeater);
        /* TODO - maybe make test if we can reach all nodes here and bail if done */
        /* - But then we need to establish how many nodes are to be reached */
        /*   (but only once in the begining of the analysis) */
        /* If a node then have no neighbours then we do the check in vain, because */
        /* this node will never be there - But then again all nodes must have neighbours */
        /* to create a real network */
      }
      if (++t >= bMaxNodeID)
      {
        analyseState = ANALYSE_STATE_SAVE_REPEATERS;
      }
      else
      {
        analyseState = ANALYSE_STATE_TABLE_READ;
      }
      break;

    case ANALYSE_STATE_SAVE_REPEATERS:
      DPRINT("A!");
      /* route table analyse completed, save the result */
      memcpy(PreferredRepeaters, tempPreferredRepeaters, MAX_NODEMASK_LENGTH);
      analyseState = ANALYSE_STATE_IDLE;
      RoutingInfoState = ROUTING_STATE_SAVING;
      RoutingWriteComplete();
      break;

    default:
      analyseState = ANALYSE_STATE_IDLE;
      break;
  }

}
#endif /* NO_PREFERRED_CALC */


#ifndef NO_PREFERRED_CALC
/*=======================   SavePreferredRepeaters   =========================
**
**  Save a list of routing information for a node in the routing table
**
**  Side effects:
**
**--------------------------------------------------------------------------*/
static void
SavePreferredRepeaters(void) /* RET  Nothing */
{
  SetPreferredRepeaters(PreferredRepeaters);
}
#endif


/*===========================   ResetmostusedNodes   ========================
**    Clears all used bit in the entry point table.
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
static void
ResetMostUsedNodes(void)
{
  register uint8_t t;

  for (t = 0; t < ENTRY_POINT_TABLE_SIZE; t++)
  {
    mostUsedTable.MostUsedEntry[t].bTimesUsed &= ~ENTRY_POINT_USED_BIT;
#ifdef ZW_MOST_USED_NEEDS_REPLACE
    /* TO#1273 fix - if a node is in the most used table the only way it can get out */
    /* of the table is by it being replaced - a zero in bTimesUSed do not remove it */
#else
    if ((mostUsedTable.MostUsedEntry[t].bTimesUsed & ENTRY_POINT_TIME_MASK) == 0)
    {
      mostUsedTable.MostUsedEntry[t].bEntryPoint = 0;
    }
    else
#endif
    if (mostUsedTable.MostUsedEntry[t].bEntryPoint == 0)    /*Added to fix cases where entrypoint is zero, but timesUsed are not*/
    {
      mostUsedTable.MostUsedEntry[t].bTimesUsed = 0;
    }
  }
}


/****************************************************************************/
/*                           EXPORTED FUNCTIONS                             */
/****************************************************************************/


/*============================   IsNodeRepeater   ======================
**    Function description
**      Tests whether a node is a repeater or not
**    Side effects:
**
**--------------------------------------------------------------------------*/
bool             /*RET true if node is repeater*/
IsNodeRepeater(
  uint8_t bNodeID)
{
  return (CtrlStorageRoutingNodeGet(bNodeID) && CtrlStorageListeningNodeGet(bNodeID));
}


/*=======================   RestartAnalyseRoutingTable  ====================
**
**  Restarts AnalyseRoutingTable
**
**  Side effects:
**
**--------------------------------------------------------------------------*/
void
RestartAnalyseRoutingTable(void)
{
#ifdef NO_PREFERRED_CALC
  SET_CURRENT_ROUTING_SPEED(RF_SPEED_9_6K);
#ifdef ZW_CONTROLLER_STATIC
	/* Use neighbors as preferred repeaters */
	ZW_GetRoutingInfo(g_nodeID, PreferredRepeaters, ZW_GET_ROUTING_INFO_ANY | GET_ROUTING_INFO_REMOVE_NON_REPS);
#endif

#else
  RoutingInfoState = ROUTING_STATE_ANALYSING;
  RoutingWriteComplete();
#endif
}


/*========================   ZW_SetRoutingInfo   =============================
**
**  Save a list of routing information for a node in the routing table
**
**  Side effects:
**
**--------------------------------------------------------------------------*/
bool              /*RET true The info contained changes and was physically written to EEPROM */
ZW_SetRoutingInfo(
  uint8_t bNode,     /* IN Node number */
  uint8_t bLength,   /* IN Length of pMask in bytes */
  uint8_t* pMask)   /* IN pointer to routing info */
{

  uint8_t t;
  NODE_MASK_TYPE routingInfo = { 0 };
  NODE_MASK_TYPE routingInfoRemovedNode = { 0 };

  if (pMask)
  {
    /* Zero fill the rest of the buffer */
    for (; bLength < MAX_NODEMASK_LENGTH; bLength++)
    {
      *(pMask + bLength) = 0;
    }
  }
#ifndef NO_PREFERRED_CALC
  else
  {
    RoutingInfoState = ROUTING_STATE_DELETING;
  }
#else
  else
  {
    CtrlStorageGetRoutingInfo(bNode, &routingInfoRemovedNode);
  }
#endif

  /* Write bits in all rows */
  for (t = 1; t <= bMaxNodeID; t++)
  {
    bool nodeInMask;

    if (pMask)
    {
      if (CtrlStorageCacheNodeExist(t))
      {
        nodeInMask = ZW_NodeMaskNodeIn(pMask, t);
        if (t != bNode)
        {
          CtrlStorageGetRoutingInfo(t, &routingInfo);
          /* Determine if bit should be set or reset */
          /* Do we need to change t's routingInfo */
          if (nodeInMask != ZW_NodeMaskNodeIn((uint8_t*)routingInfo, bNode))
          {
            if (nodeInMask)
            {
              ZW_NodeMaskSetBit((uint8_t*)routingInfo, bNode);
            }
            else
            {
              ZW_NodeMaskClearBit((uint8_t*)routingInfo, bNode);
            }
            if (CtrlStorageRoutingInfoFileIsReady(t, bMaxNodeID))
            {
              CtrlStorageSetRoutingInfo(t, &routingInfo, true);
            }
            else
            {
              CtrlStorageSetRoutingInfo(t, &routingInfo, false);
            }
          }
        }
        else
        {
#ifdef NO_PREFERRED_CALC
#ifdef ZW_CONTROLLER_STATIC
          if (IsNodeRepeater(bNode))
          {
            if (nodeInMask)
            {
              PreferredSet(bNode);
            }
            else
            {
              PreferredRemove(bNode);
            }
          }
#endif
#endif
        }
      }
    }
    else
    {
      /* We are removing bNode so update the routing info of all bNodes neighbors */
      if ((t != bNode) &&
          ZW_NodeMaskNodeIn((uint8_t*)routingInfoRemovedNode, t) &&
          CtrlStorageCacheNodeExist(t))
      {
        CtrlStorageGetRoutingInfo(t, &routingInfo);
        if (ZW_NodeMaskNodeIn((uint8_t*)routingInfo, bNode))
        {
          ZW_NodeMaskClearBit((uint8_t*)routingInfo, bNode);
          if (CtrlStorageRoutingInfoFileIsReady(t, bMaxNodeID))
          {
            CtrlStorageSetRoutingInfo(t, &routingInfo, true);
          }
          else
          {
            CtrlStorageSetRoutingInfo(t, &routingInfo, false);
          }
        }
      }
    }
  }
  /* Write rangeinfo */
  /*if pointer to range info is NULL then we reset the range info for the node*/
  CtrlStorageSetRoutingInfo(bNode, (NODE_MASK_TYPE *)pMask, true);

#ifdef NO_PREFERRED_CALC
//  RoutingInfoState = ROUTING_STATE_IDLE;
#else
  RoutingWriteComplete();
#endif

  return(true);
}




#if defined(ZW_DYNAMIC_TOPOLOGY_ADD) || defined(ZW_DYNAMIC_TOPOLOGY_DELETE)
/*============================   SetRoutingLink   ============================
**    Set or Remove a link between bANodeID and bBNodeID in the RoutingTable
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
  bool _bset)
{
  /* Make sure the Network topology table keeps on being symmetric */
  NODE_MASK_TYPE nodeARoutingInfo;
  NODE_MASK_TYPE nodeBroutingInfo;

  GetRouteingInfo((bANodeID -1), &nodeARoutingInfo);
  GetRouteingInfo((bBNodeID -1), &nodeBRoutingInfo);
  if (_bset)
  {
    ZW_NodeMaskSetBit((uint8_t*)nodeARoutingInfo, bBNodeID);
    ZW_NodeMaskSetBit((uint8_t*)nodeBRoutingInfo, bANodeID);
  }
  else
  {
    ZW_NodeMaskClearBit((uint8_t*)nodeARoutingInfo, bBNodeID);
    ZW_NodeMaskClearBit((uint8_t*)nodeBRoutingInfo, bANodeID);
  }
  SetRouteingInfo((bANodeID -1), &nodeARoutingInfo);
  SetRouteingInfo((bBNodeID -1), &nodeBRoutingInfo);

}
#endif


/*==========================   UpdateMostUsedNodes   =======================
**    Increment or decrement the usage counter for a node in the most
**    used entry table.
**
**    Side effects:
**    Updates MostUsedEntry with new counter value
**
**--------------------------------------------------------------------------*/
void
UpdateMostUsedNodes(  /* RET  Nothing */
  bool bLastTxStatus, /* IN   false Failed to transmit to the node
                              true Succesfull transmit to the node */
  uint8_t bEntryPoint)   /* IN   Node id of the entry point used   */
{
uint8_t t;
uint8_t bTmpCount;

#ifdef ZW_MOST_USED_NEEDS_REPLACE
  uint8_t bEmpty;
#endif
  /* For now nodeIDs greater than ZW_MAX_NODES do not exist in routing table */
  if (!bEntryPoint || (bEntryPoint > ZW_MAX_NODES))
  {
    return;
  }

#ifdef ZW_CONTROLLER_STATIC
	/* TO#3066 fix update - If Node is not in the PreferredRepeaters list/a Neighbor */
	/* then it cannot be a MostUsed node either. */
  if (!(ZW_NodeMaskNodeIn(PreferredRepeaters, bEntryPoint)))
#else
  if (!IsNodeRepeater(bEntryPoint))
#endif
  {
    return;
  }

  /* Update counter in table */
  for (t = 0; t < ENTRY_POINT_TABLE_SIZE; t++)
  {
    /* TO#1427 fix - Only enter if a entrypoint exist */
    if (mostUsedTable.MostUsedEntry[t].bEntryPoint)
    {
      /* Use temp counter to preserve used bit */
      bTmpCount = mostUsedTable.MostUsedEntry[t].bTimesUsed & ENTRY_POINT_TIME_MASK;
      {
        if (mostUsedTable.MostUsedEntry[t].bEntryPoint == bEntryPoint)
        {
          /* Entry point found, update counter value */
          if (bLastTxStatus)
          {
            /* Make sure that counter doesn't overflow */
            if (bTmpCount < MOST_USED_MAX_COUNT)
            {
              bTmpCount++;
            }
          }
          else
          {
#ifdef ZW_MOST_USED_NEEDS_REPLACE
            if (bTmpCount)
            {
              bTmpCount--;
            }
#else
            bTmpCount--;
#endif
          }
          /* Save counter value and set used bit */
          mostUsedTable.MostUsedEntry[t].bTimesUsed = bTmpCount | ENTRY_POINT_USED_BIT;
          /* Found, exit function */
          return;
        }
      }
    }
  }
  bTmpCount = IsNode9600(bEntryPoint);

  /* If 9.6 node then only check the part of the table reserved for 9.6 nodes */
  if (bTmpCount)
  {
    t = ENTRY_POINT_40K_NODES;
  }
  else
  {
    t = 0;
  }

#ifdef ZW_MOST_USED_NEEDS_REPLACE
  bEmpty = ENTRY_POINT_TABLE_SIZE;
#endif /* ZW_MOST_USED_NEEDS_REPLACE */
	/* Add node to table if there is room */
  for (; t < ENTRY_POINT_TABLE_SIZE; t++)
  {
#ifdef ZW_MOST_USED_NEEDS_REPLACE
    if ((mostUsedTable.MostUsedEntry[t].bTimesUsed & ENTRY_POINT_TIME_MASK) == 0)
    {
      bEmpty = t;
    }
    /* If free entry */
    if (mostUsedTable.MostUsedEntry[t].bEntryPoint == 0)
   	{
   	  mostUsedTable.MostUsedEntry[t].bEntryPoint = bEntryPoint;
      mostUsedTable.MostUsedEntry[t].bTimesUsed = bLastTxStatus | ENTRY_POINT_USED_BIT; /* 0 (false) or 1 (true) */
      return;
    }
#else
    /* If free entry */
    if ((mostUsedTable.MostUsedEntry[t].bTimesUsed & ENTRY_POINT_TIME_MASK) == 0)
   	{
   	  mostUsedTable.MostUsedEntry[t].bEntryPoint = bEntryPoint;
      mostUsedTable.MostUsedEntry[t].bTimesUsed = bLastTxStatus; /* 0 (false) or 1 (true) */
      return;
    }
#endif
  }
#ifdef ZW_MOST_USED_NEEDS_REPLACE
  if (bEmpty < ENTRY_POINT_TABLE_SIZE)
  {
    mostUsedTable.MostUsedEntry[bEmpty].bEntryPoint = bEntryPoint;
    mostUsedTable.MostUsedEntry[bEmpty].bTimesUsed = bLastTxStatus | ENTRY_POINT_USED_BIT; /* 0 (false) or 1 (true) */
  }
#endif
}


/*===========================   GetMostUsedNode   ==========================
**
**    Returns the most used entry point and mark it as used. The function
**    can be called ENTRY_POINT_TABLE_SIZE times and it will return the
**    next most used entry point each time.
**    The used entrys in the table are released with a call to
**    ResetMostUsedNodes(). The function will never return a Node ID equal
**    to bNodeID.
**
**    Side effects:
**    Sets the used bit for the returned entry points in the MostUsedEntry
**    table
**
**--------------------------------------------------------------------------*/
uint8_t
GetMostUsedNode( /*RET 0xff    - No more entry points in list
                       0-0x3f  - Node ID of entry point    */
  uint8_t bNodeID)  /* IN Node ID of the switch we don't want the
                       function to return. */
{
  uint8_t bCount = 0;

  DPRINTF("M%02X", bCurrentRoutingSpeed);
  uint8_t bEntry = 0xFF;
  /* Find the next non-used entry point with the largest count value */
  for (uint8_t t = 0; t < ENTRY_POINT_TABLE_SIZE; t++)
  {
    /* If entrypoint is zero then timeused cant be a value!! */
    if (!mostUsedTable.MostUsedEntry[t].bEntryPoint)
    {
      mostUsedTable.MostUsedEntry[t].bTimesUsed = 0;
    }
    else
    {
      /* Only check nodes that hasn't been returned yet */
      if (!(mostUsedTable.MostUsedEntry[t].bTimesUsed & ENTRY_POINT_USED_BIT))
      {
        /* If times used is bigger for this node then use it instead */
        /* "Most used" repeaters can have bTimesUsed = 0 */
        if (mostUsedTable.MostUsedEntry[t].bTimesUsed >= bCount)
        {
          /* If node does not support speed we are looking for, continue */
          /* to the next node */
          if (!DoesNodeSupportSpeed(mostUsedTable.MostUsedEntry[t].bEntryPoint, bCurrentRoutingSpeed))
          {
            continue;
          }
          /* Check if the node has the right speed */
          /* Make sure that we don't return the node we are actually trying */
          /* to reach in the routed network */
          if (mostUsedTable.MostUsedEntry[t].bEntryPoint != bNodeID)
          {
            bCount = mostUsedTable.MostUsedEntry[t].bTimesUsed;
            bEntry = t;
          }
        }
      }
    }
  }
  if (bEntry != 0xff)
  {
    /* TODO - use bCurrentRoutingSpeed to */
    /* Set used bit in found entry */
    mostUsedTable.MostUsedEntry[bEntry].bTimesUsed |= ENTRY_POINT_USED_BIT;
    return(mostUsedTable.MostUsedEntry[bEntry].bEntryPoint);
  }
  else
  {
    return (bEntry);
  }
}


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
  uint8_t bNodeID)
{
  ZW_NodeMaskSetBit(PreferredRepeaters, bNodeID);
}
#endif


#if defined(ZW_CONTROLLER_STATIC) || defined(NO_PREFERRED_CALC)
/*===========================   PreferredRemove   ============================
**
**    Remove bNodeID as preferred repeater
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
void
PreferredRemove(
  uint8_t bNodeID)
{
  ZW_NodeMaskClearBit(PreferredRepeaters, bNodeID);
  DeleteMostUsed(bNodeID);
}
#endif


/*===========================   ClearMostUsed   ============================
**
**    Clear most used table
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
void
ClearMostUsed(void) /* RET Nothing */
{
  memset((uint8_t*)mostUsedTable.MostUsedEntry, 0, sizeof(mostUsedTable));
}



/*===========================   DeleteMostUsed   ============================
**
**    Delete a node from most used table
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
void
DeleteMostUsed(  /* RET Nothing */
  uint8_t bNodeID)       /* IN   Node number */
{
  register uint8_t t;

  /* Find node ID in table */
  for (t = 0; t < ENTRY_POINT_TABLE_SIZE; t++)
  {
    if (mostUsedTable.MostUsedEntry[t].bEntryPoint == bNodeID)
    {
      mostUsedTable.MostUsedEntryWord[t].wEntryPoint = 0;
    }
  }
}


/*===========================   IsMostUsed   ============================
**
**    Check if the specified node is a most used node
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
uint8_t
IsMostUsed(  /* RET true if node is in most used table */
  uint8_t bNodeID)   /* IN  Node id of the node to check */
{
  register uint8_t t;

  for (t = 0; t < ENTRY_POINT_TABLE_SIZE; t++)
  {
    if (mostUsedTable.MostUsedEntry[t].bEntryPoint == bNodeID)
    {
      return true;
    }
  }
  return false;
}


/*========================   RoutingInfoReceived   ==========================
**
**  Start processing the routing info received from a node
**
**  Side effects:
**
**--------------------------------------------------------------------------*/
uint8_t
RoutingInfoReceived(  /*RET 0 if unable to save the routing info, 1 if saved, 2 if the written contained changes */
  uint8_t bLength,       /* IN Length of range info buffer */
  uint8_t bNode,         /* IN Node number */
  uint8_t *pRangeInfo)   /* IN Range info pointer */
{
  return ((uint8_t)ZW_SetRoutingInfo(bNode, bLength, pRangeInfo) + 1);
}


/*=========================   RangeInfoNeeded   ===========================
**
**  Check if it is necessary to get new range information from a node
**
**  Returns:
**    NEIGHBORS_NONEIGHBORS - No neighbors
**    0x01-0xE7             - Number of neighbors
**    NEIGHBORS_NEEDED      - Rangeinfo needed
**
**  Side effects:
**
**--------------------------------------------------------------------------*/
uint8_t
RangeInfoNeeded(  /* RET  true if range info should be updated */
  uint8_t bNodeID)        /* IN   Node ID */
{
  /* First check if node ID is valid */
  if ((bNodeID > 0) && (bNodeID <= ZW_MAX_NODES))
  {
    /* We are now trying to determine if any sensors are neighbors */
    if (assign_ID.assignIdState == ASSIGN_RANGE_REQUESTED)
    {
      return RANGEINFO_NONEIGHBORS;
    }
    /* If pending discovery then return RANGEINFO_NONEIGHBORS */
    if (CtrlStorageGetPendingDiscoveryStatus(bNodeID))
    {
      return RANGEINFO_NONEIGHBORS;
    }

    ZW_GetRoutingInfo(bNodeID, abNeighbors, ZW_GET_ROUTING_INFO_ANY | GET_ROUTING_INFO_REMOVE_NON_REPS);

    /* Return number of neighbors */
    return ZW_NodeMaskBitsIn(abNeighbors, MAX_NODEMASK_LENGTH);
  }
  return RANGEINFO_ID_INVALID;
}


/*========================   SetPendingDiscovery   ==========================
**
**  Sets the specified node bit in the pending discovery list
**
**  Side effects:
**
**--------------------------------------------------------------------------*/
void
SetPendingDiscovery(  /*RET Nothing */
  uint8_t bNodeID)       /* IN Node ID that should be set */
{
  CtrlStorageSetPendingDiscoveryStatus(bNodeID, true);
}


/*=======================   ClearPendingDiscovery   =========================
**
**  Clear the specified node bit in the pending discovery list
**
**
**  Side effects:
**
**--------------------------------------------------------------------------*/
void
ClearPendingDiscovery(  /*RET Nothing */
  uint8_t bNodeID)         /* IN Node ID that should be set */
{
  CtrlStorageSetPendingDiscoveryStatus(bNodeID, false);
}


/*=============================   HasNodeANeighbour   ========================
**
**  Do a node have any neighbors - Bad Routes and noneRepeaters are removed
**
**
**  Side effects:
**
**--------------------------------------------------------------------------*/
bool               /* RET true if any neighbours */
ZCB_HasNodeANeighbour(
  uint8_t bNode)      /* IN Node ID on node to test for neighbours */
{
  if((NODE_BROADCAST != bNode) && (NODE_CONTROLLER_OLD != bNode) && (0 != bNode))
  {
    uint8_t i;
    uint8_t  * p;
    ZW_GetRoutingInfo(bNode, abNeighbors, ZW_GET_ROUTING_INFO_ANY
        | GET_ROUTING_INFO_REMOVE_BAD | GET_ROUTING_INFO_REMOVE_NON_REPS);
    i = MAX_NODEMASK_LENGTH;
    p = abNeighbors;
    do
    {
      if (*p != 0)
      {
        return true;
      }
      p++;
    } while (--i);
  }
  return false;
}


/*==========================   ZW_AreNodesNeighbours   ============================
**
**  Are two specific nodes neighbours
**
**
**  Side effects:
**
**--------------------------------------------------------------------------*/
bool              /*RET true if nodes are neighbours else false */
ZW_AreNodesNeighbours(
  uint8_t bNodeA,    /* IN first node id */
  uint8_t bNodeB)    /* IN second node id */
{
  //return false on invalid nodes
  if( (NODE_BROADCAST == bNodeA)      ||
      (NODE_BROADCAST == bNodeB)      ||
      (NODE_CONTROLLER_OLD == bNodeA) ||
      (NODE_CONTROLLER_OLD == bNodeB) ||
      (0 == bNodeA)                   ||
      (0 == bNodeB)
    )
  {
    return false;
  }

#ifdef ZW_CONTROLLER_STATIC
  ZW_GetRoutingInfo(bNodeA, abNeighbors, ZW_GET_ROUTING_INFO_ANY);
  return (ZW_NodeMaskNodeIn(abNeighbors, bNodeB));
#else
  if ((bNodeA != g_nodeID) && (bNodeB != g_nodeID))
  {
    ZW_GetRoutingInfo(bNodeA, abNeighbors, ZW_GET_ROUTING_INFO_ANY);

    DPRINTF("A%02X%02X-", bNodeA, bNodeB);
#ifdef DEBUGPRINT
    SendBitmask(abNeighbors);
#endif

    return (ZW_NodeMaskNodeIn(abNeighbors, bNodeB));
  }
  else
  {
    /* We are a portable controller and therefor we never can tell if we can reach a node */
    /* directly or not, so therefor we assume we are neighbors */
    return true;
  }
#endif
}


/*=====================   GetNextRouteFromNodeToNode   =======================
**
**  Finds the next route to the specified node
**
**
**  Side effects:
**
**--------------------------------------------------------------------------*/
uint8_t
GetNextRouteFromNodeToNode( /* RET  true - New route found, */
                            /*      false - No more routes */
  uint8_t  bDestNodeID,        /* IN   Node ID that route should be found to */
  uint8_t  bSourceNodeID,      /* IN   Node ID that route should be found from */
  uint8_t *pRepeaterList,      /* OUT  Pointer to new repeater list */
  uint8_t *pHopCount,          /* OUT  Pointer to hop count */
  uint8_t bRouteNumber,        /* IN   Route number to find, starting with 1 */
  bool fResetNextNode)      /* IN   reset bNextNode if true*/
{
  bool bNonRepeater;
  bool bRouteFound = false;
  static uint8_t bNextNode;

  uint32_t wStartTickTimeSample = getTickTime();

  DPRINTF("Gn%02X", bCurrentRoutingSpeed);
  if (fResetNextNode)
  {
    bNextNode = 1;
  }
  if (bRouteNumber == 1)
  {
    bNextNode = 1; /* Reset the next node counter if its the first route we want*/
  }
  /* Set source node as non-repeater to prevent it from beeing used as repeater */
  if (ZW_NodeMaskNodeIn(NonRepeaters, bSourceNodeID) != 0)
  {
    bNonRepeater = true;
  }
  else
  {
    bNonRepeater = false;
    RoutingAddNonRepeater(bSourceNodeID);
  }
  /* Get neighbors for source node */
  ZW_GetRoutingInfo(bSourceNodeID, abNeighbors, bCurrentRoutingSpeed | GET_ROUTING_INFO_REMOVE_NON_REPS);

  /* Remove source and dest from neighbors */
  ZW_NodeMaskClearBit(abNeighbors, bDestNodeID);

  /* Find Route for next neighbor */
  while (bNextNode <= bMaxNodeID)
  {
    if (CtrlStorageCacheNodeExist(bNextNode) && ZW_NodeMaskNodeIn(abNeighbors, bNextNode))
    {
      zpal_feed_watchdog();
      /* Find route */
      /* Check if this route was already tried at a different speed */
      if (FindBestRouteToNode(bNextNode, bDestNodeID, pHopCount, pRepeaterList) != 0xFF
          && !DoesRouteSupportTriedSpeed(pRepeaterList, *pHopCount, bReturnRouteTriedSpeeds)) {
        /* Route accepted. Terminate search. */
        bRouteFound = true;
      }
      if (ROUTING_MAX_CALC_TIME < getTickTimePassed(wStartTickTimeSample) || bRouteFound) {
        /* Bail... Timeout - max time has been used for calculating new route - No route */
        break;
      }
      /* Route has already been tried, skip to next */
    }
    bNextNode++;
  }
  /* Remove non-repeater bit again */
  if (!bNonRepeater)
  {
    RoutingRemoveNonRepeater(bSourceNodeID);
  }
  bNextNode++;
  return bRouteFound;
}

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
  uint8_t speed,                 /* IN The speed to be marked as tried, RF_SPEED_* */
  uint8_t* pRoutInfo)            /* OUT The routing info that should be changed
                             *     Format: ROUTING_INFO_TRIED_SPEED_* */
{
  uint8_t tmpRoutingInfo = 0;
  switch (speed)
  {
    case RF_SPEED_9_6K:
      tmpRoutingInfo = ROUTING_INFO_TRIED_SPEED_9600;
      break;

    case RF_SPEED_40K:
      tmpRoutingInfo = ROUTING_INFO_TRIED_SPEED_40K;
      break;

    case RF_SPEED_100K:
      tmpRoutingInfo = ROUTING_INFO_TRIED_SPEED_100K;
      break;

    default:  // its not intended to ever get here.
      tmpRoutingInfo = ROUTING_INFO_TRIED_SPEED_9600;
      break;
  }
  *pRoutInfo |= tmpRoutingInfo;
}


/*==========================   ClearTriedSpeeds   ============================
**
**  Clear tried routing speed for a frame in the TxQueue.
**
**  Side effects:
**
**--------------------------------------------------------------------------*/
void
ClearTriedSpeeds(
  uint8_t* pRoutInfo) /* OUT The frame that should be changed */
{
  *pRoutInfo &= 0x00;
}


/*=====================   DoesRouteSupportSpeed   =======================
**
**  Check if route supports a certain speed
**
**  Side effects:
**
**--------------------------------------------------------------------------*/
bool                      /*RET true if route supported by tried speeds,
                           *    false otherwise */
DoesRouteSupportSpeed(
  uint8_t *pRepeaterList,      /*IN  List of repeaters in route */
  uint8_t bHopCount,           /*IN  Hop count in route */
  uint8_t bSpeed)              /*IN  Speed to support RF_SPEED_* */
{
  uint8_t idx;

  for (idx = 0; idx < bHopCount; idx++)
  {
    if (!DoesNodeSupportSpeed(pRepeaterList[idx], bSpeed))
    {
      return false;
    }
  }
  return true;
}


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
  uint8_t bDestID)             /* IN Node ID of the destination node */
{
  uint8_t bHigherSpeeds;

  switch (bRouteSpeed)
  {
    case RF_SPEED_100K:
      bHigherSpeeds = 0;
      break;
    case RF_SPEED_40K:
      /* TO#2796 - Only do higher speed check if destination supports a higher speed
                   than the current routing speed */
      if (DoesNodeSupportSpeed(bDestID, ZW_NODE_SUPPORT_SPEED_100K))
        bHigherSpeeds = ROUTING_INFO_TRIED_SPEED_100K;
      else
        return false;
      break;
    case RF_SPEED_9_6K:
    default:
      /* TO#2796 - Only do higher speed check if destination supports a higher speed
                   than the current routing speed */
      if (DoesNodeSupportSpeed(bDestID, ZW_NODE_SUPPORT_SPEED_40K))
        bHigherSpeeds = ROUTING_INFO_TRIED_SPEED_100K | ROUTING_INFO_TRIED_SPEED_40K;
      else
        return false;
      break;
  }
  return DoesRouteSupportTriedSpeed(pRepeaterList, bRepeaterCount, bHigherSpeeds);
}


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
  uint8_t bTriedSpeeds)          /*IN  Tried speeds, ROUTING_INFO_TRIED_SPEED_* */
{
  if ((bTriedSpeeds & ROUTING_INFO_TRIED_SPEED_9600) &&
      DoesRouteSupportSpeed(pRepeaterList, bHopCount, RF_SPEED_9_6K))
  {
      return true;
  }
  if (bTriedSpeeds & ROUTING_INFO_TRIED_SPEED_40K)
  {
    if (DoesRouteSupportSpeed(pRepeaterList, bHopCount, RF_SPEED_40K))
    {
      return true;
    }
  }
  if (bTriedSpeeds & ROUTING_INFO_TRIED_SPEED_100K)
  {
    if (DoesRouteSupportSpeed(pRepeaterList, bHopCount, RF_SPEED_100K))
    {
      return true;
    }
  }
  return false;
}


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
uint8_t bDestNodeID)  /*IN  Destination node ID */
{
  while(--bCurrentRoutingSpeed > 0)
  {
    if (DoesNodeSupportSpeed(bSrcNodeID, bCurrentRoutingSpeed)
        && DoesNodeSupportSpeed(bDestNodeID, bCurrentRoutingSpeed))
    {
      DPRINTF("Nl%02X", bCurrentRoutingSpeed);
      return true;
    }
  }

  DPRINT("NlF");
  return false;
}


/*========================   GetNextRouteToNode   ==========================
**
**  Finds the next route to the specified node
**
**  Side effects:
**
**--------------------------------------------------------------------------*/
uint8_t
GetNextRouteToNode(     /* RET  true - New route found,
                         * false - No more routes */
  uint8_t  bNodeID,           /* IN   Node ID that route should be found to */
  uint8_t  bCheckMaxAttempts, /* IN   Should number of routing attempt have a MAX */
  uint8_t *pRepeaterList,     /* OUT  Pointer to new repeater list */
  uint8_t *pHopCount,         /* OUT  Pointer to hop count */
  uint8_t *speed)             /* OUT  Speed of the new route */
{
  uint8_t fContinueRouteSearch = 0;

  DPRINTF("Z%02X", bCurrentRoutingSpeed);

  /* Always default start with finding 100K routes */
  if (abLastStartNode[0] == 0)
  {
    SET_CURRENT_ROUTING_SPEED(ZW_NODE_SUPPORT_SPEED_100K);
    bRouteAttempts = 0;
  }
  /* TO#1984, TO#1998 and TO#1999 fix */
  /* Only enforce max route attempts if using explore frame as last resort */
  if (bCheckMaxAttempts)
  {
    bRouteAttempts++;
    /* Bail out if we have tried the max allowed number of routes */
    if (bRouteAttempts > bRouteAttemptsMAX)
    {
      return false;
    }
  }
  uint8_t nodeIs2Ch = (0 == llIsHeaderFormat3ch());
  /* Try routing at all possible speeds. Highest speed first */
  /* Make sure node supports this speed */
  if ( nodeIs2Ch &&(!DoesNodeSupportSpeed(bNodeID, bCurrentRoutingSpeed)))
  {
    if (NextLowerSpeed(g_nodeID, bNodeID) == false)
    {
      return false;
    }
  }
  do
  {
    /* Find route at speed bCurrentRoutingSpeed */
    uint8_t bTemp = OldGetNextRouteToNode(bNodeID, pRepeaterList, pHopCount);
    if (bTemp)
    {
      DPRINTF(
              "K%02X%02X%02X%02X%02X",
              *pHopCount,
              pRepeaterList[0],
              pRepeaterList[1],
              pRepeaterList[2],
              pRepeaterList[3]
            );
      /* Check if this route was already tried at a different speed */
      if (nodeIs2Ch && (DoesRouteSupportHigherSpeed(pRepeaterList, *pHopCount, bCurrentRoutingSpeed, bNodeID)))
      {
        DPRINT("#");
        /* route has already been tried, skip to next */
        /* set flag just to pass test in while statement below */
        fContinueRouteSearch = true;
        continue;
      }
      DPRINT("+");
      *speed = (nodeIs2Ch && IsNodeSensor(bNodeID, false, false)) ? RF_OPTION_SPEED_40K : bCurrentRoutingSpeed;
      return bTemp;
    }
    if (nodeIs2Ch)
    {
      /* No routes found at current speed */
      /* reset route diversity after speed change */
      abLastStartNode[0] = 0; /* HEH: Since we switched speed we need to start over on routing attempts */
      /* try switching to next lower speed supported by both nodes */
      fContinueRouteSearch = NextLowerSpeed(g_nodeID, bNodeID);
    }
  } while (nodeIs2Ch && (fContinueRouteSearch));
  DPRINT("%%");
  return false;
}


#if 0
/* Here we save the Most Recently Used Entrypoint for every speed - use (bCurrentRoutingSpeed-1) as index */
uint8_t aMostRecentlyUsedEntryPoint[3];

#define ENTRYPOINT_STATE_MOSTUSED   0
#define ENTRYPOINT_STATE_PREFERRED  1

uint8_t
GetNextEntryPoint(void)
{
  switch (bEntryPointState)
  {
    case ENTRYPOINT_STATE_MOSTUSED:
      break;

    case ENTRYPOINT_STATE_PREFERRED:
      break;

    default:
      break;
  }
}
#endif


/*========================   OldGetNextRouteToNode   ==========================
**
**  Finds the next route to the specified node
**
**
**  Side effects:
**
**--------------------------------------------------------------------------*/
uint8_t
OldGetNextRouteToNode(     /* RET  true - New route found,
                          * false - No more routes */
  uint8_t  bNodeID,        /* IN   Node ID that route should be found to */
  uint8_t *pRepeaterList,  /* OUT  Pointer to new repeater list */
  uint8_t *pHopCount)      /* OUT  Pointer to hop count */
{
  uint8_t useCurrentEntryPoint = false;
  static uint8_t bEntryPoint;
  static uint8_t bEntryCount;
  static bool bMostUsedFirst; /* Try most-used entry points first? */
  uint8_t bNonRepeater = false;

  uint32_t wStartTickTimeSample = getTickTime();

  DPRINTF("W%02X", bCurrentRoutingSpeed);

/* TODO - this function needs a serious REFACTORING!! */

  /* Set source node as non-repeater to prevent it from being used as repeater */
  if (ZW_NodeMaskNodeIn(NonRepeaters, g_nodeID))
  {
    bNonRepeater = true;
  }
  else
  {
    RoutingAddNonRepeater(g_nodeID);
  }

  /* TODO - Make function which handles the finding of EntryPoints according to ch/speed agility needs */

  /* First call for this transmission, clear the used bit in the most used table */
  if (!abLastStartNode[0])
  {
    DPRINT("*");
    abLastStartNode[0] = abLastNode[0];
    abLastStartNode[1] = abLastNode[1];
    abLastStartNode[2] = abLastNode[2];
    abLastStartNode[3] = abLastNode[3];
    ResetMostUsedNodes();
    bMostUsedFirst = true;
    bEntryPoint = 0;
    bEntryCount = 0;
  }
  DPRINTF("A%02X%02X%02X", badRoute.from, badRoute.to, bEntryCount);
  if ((bEntryCount == 1) && (badRoute.from != 0))
  {
    DPRINTF("B%02X", badRoute.from);
    /* There is a bad route in the system, try another route with the */
    /* same entry point */
    useCurrentEntryPoint = true;
  } else {
    if (!bMostUsedFirst) {
      /* now "preferred repeaters" should function also and */
      /* not end in an endless loop... */
      if ((bEntryCount >= 2) || (!badRoute.from)) {
        ++bEntryPoint;
        if (bEntryPoint > bMaxNodeID) {
          bEntryPoint = 1;
        }
      }
      if (bEntryPoint == abLastStartNode[0]) {
        goto ExitFalse;
      }
      bEntryCount = 0;
      badRoute.from = 0;
      badRoute.to = 0;
    }
  }
  if (bMostUsedFirst)
  {
    do
    {
      if (useCurrentEntryPoint)
      {
        useCurrentEntryPoint = false;
      }
      else
      {
        /* Get next most used (If any) */
        bEntryPoint = GetMostUsedNode(bNodeID);
        bEntryCount = 0;
        badRoute.from = 0;
        badRoute.to = 0;
      }
      if (bEntryPoint != 0xFF) /* Try next most used node */
      {
        DPRINTF("b%02X%02X", bEntryPoint, bNodeID);
        zpal_feed_watchdog();
#ifdef DEBUGPRINT
        SendMostUsedEntryTable();
#endif
        /* Get route to node */
        if (FindBestRouteToNode(bEntryPoint, bNodeID, pHopCount, pRepeaterList) != 0xFF)
        {
          bEntryCount++;
          goto ExitTrue;
        }
        DPRINT("c");
      }
      if (ROUTING_MAX_CALC_TIME < getTickTimePassed(wStartTickTimeSample))
      {
        /* Bail... Timeout - max time has been used for calculating new route - No route */
        goto ExitFalse;
      }
    } while (bEntryPoint != 0xFF);
    DPRINTF("d%02X%02X%02X", PreferredRepeaters[0], abLastStartNode[0], abLastNode[0]);
    bMostUsedFirst = false;
    abLastNode[0] = abLastStartNode[0];
    bEntryPoint = abLastNode[0];
  } /* if(bFirstPreferred) */
  do
  {
    if (bEntryPoint != bNodeID)
    {
      DPRINTF("E%02X%02X%02X", bEntryPoint, abLastNode[0], abLastStartNode[0]);
      /* Check if the node is a preferred repeater */
      if (CtrlStorageCacheNodeExist(bEntryPoint) && ZW_NodeMaskNodeIn(PreferredRepeaters, bEntryPoint))
      {
        DPRINT("-");
        if (useCurrentEntryPoint || (IsMostUsed(bEntryPoint) == false))
        {
          DPRINT("0");
          zpal_feed_watchdog();
          /* Get route to node */
          if (FindBestRouteToNode(bEntryPoint, bNodeID, pHopCount, pRepeaterList) != 0xFF)
          {
            abLastNode[0] = bEntryPoint;
            bEntryCount++;
            useCurrentEntryPoint = false;
            DPRINT("1");
            goto ExitTrue;
          }
          DPRINT("2");
        }
        DPRINT("3");
      }
    }
    if (++bEntryPoint > bMaxNodeID)
    {
      bEntryPoint = 1;
    }
    if (ROUTING_MAX_CALC_TIME < getTickTimePassed(wStartTickTimeSample))
    {
      /* Bail... Timeout - max time has been used for calculating new route - No route */
      break;
    }
  } while (bEntryPoint != abLastStartNode[0]);
ExitFalse:
  bMostUsedFirst = true;
  /* Remove non-repeater bit again */
  if (!bNonRepeater)
  {
    RoutingRemoveNonRepeater(g_nodeID);
  }
  return false;
ExitTrue:
  if (!bNonRepeater)
  {
    RoutingRemoveNonRepeater(g_nodeID);
  }
  return true;
}

/*========================   RoutingAddNonRepeater   ==========================
**
**  Add a now node to the non-repeater list.
**
**  Side effects:
**
**--------------------------------------------------------------------------*/
void
RoutingAddNonRepeater( /* RET Nothing */
  uint8_t bNodeID)        /* IN  Node ID */
{
  ZW_NodeMaskSetBit(NonRepeaters, bNodeID);
}


/*========================   RoutingRemoveNonRepeater   ======================
**
**  Remove a node from the non-repeater list.
**
**  Side effects:
**
**--------------------------------------------------------------------------*/
void
RoutingRemoveNonRepeater( /* RET Nothing */
  uint8_t bNodeID)           /* IN  Node ID */
{
  ZW_NodeMaskClearBit(NonRepeaters, bNodeID);
}


#ifndef NO_PREFERRED_CALC
/*========================   RoutingRemoveNonRepeater   ======================
**
**  Stop the routing analysis process.
**
**  Side effects:
**
**--------------------------------------------------------------------------*/
void
RoutingAnalysisStop(void)
{
  RoutingBufferLocked = false;
  RoutingInfoState = ROUTING_STATE_IDLE;
  analyseState = ANALYSE_STATE_IDLE;
}
#endif
