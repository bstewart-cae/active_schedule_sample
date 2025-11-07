// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file ZW_routing_init.c
 * @copyright 2022 Silicon Laboratories Inc.
 *
 * @brief Routing init functions used in a controller.
 */
/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/

#include <ZW_controller.h>
#include <ZW_routing.h>


/****************************************************************************/
/*                                EXTERNAL DATA                             */
/****************************************************************************/

/****************************************************************************/
/*                      PRIVATE TYPES and DEFINITIONS                       */
/****************************************************************************/

/****************************************************************************/
/*                              PRIVATE DATA                                */
/****************************************************************************/

/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/
extern uint8_t abLastNode[MAX_REPEATERS];
extern uint8_t PreferredRepeaters[];

/****************************************************************************/
/*                           EXPORTED FUNCTIONS                             */
/****************************************************************************/

/*===========================   ResetRouting   ==============================
**
**  Reset Routing diversity
**
**
**  Side effects:
**
**--------------------------------------------------------------------------*/
void
ResetRouting(void)
{
  abLastNode[0] = 1;
  abLastNode[1] = 1;
  abLastNode[2] = 1;
  abLastNode[3] = 1;
  SET_CURRENT_ROUTING_SPEED(RF_SPEED_9_6K);
}


/*=========================   InitRoutingValues   ============================
**
**  Initialize the values in memory that are used in the routing
**
**
**  Side effects:
**
**--------------------------------------------------------------------------*/
void
InitRoutingValues(void) /*RET Nothing */
{
  /* Clear Non-Repeater list */
  ZW_NodeMaskClear(NonRepeaters, MAX_NODEMASK_LENGTH);
  /* TO#01763 Fix - Controllers can loose their ability to route using ZW_SendData() */
  ResetRouting();

  /* TO#1514 fix - When NO_PREFERRED_CALC is defined then the neighbouring repeaters */
  /* should be the preferred */
  /* We use PreferredRepeaters buffer to save the static controllers neighbors, */
  /* which we need to prepopulate MostUsed */
	ZW_GetRoutingInfo(g_nodeID, PreferredRepeaters, ZW_GET_ROUTING_INFO_ANY | GET_ROUTING_INFO_REMOVE_NON_REPS);

  /* Build non-repeater list */
  /* Remember to test all nodes not just some of them */
  for (node_id_t nodeId = GetMaxNodeID(); nodeId > 0; --nodeId) {
    if (!ZCB_GetNodeType(nodeId)) {
      // Node is not registered in the controller
      continue;
    }
    if (!IsNodeRepeater(nodeId)) {
      RoutingAddNonRepeater(nodeId);
      /* Only neighbouring repeaters should be allowed to be preferred */
      PreferredRemove(nodeId);
    }
    /* fix, a static controller needs its MostUsed entries to be neighbours */
    else {
      /* All neighbouring repeaters are now preferred */
      if (nodeId == ZW_NodeIDGet()) {
        PreferredRemove(nodeId);
      }

    }
  }

#ifdef ZW_DEBUG_ROUTING
  SendRoutingTable(5
                   , false
                  );
  DPRINT("\r\n");
  SendRoutingTable(5
                   , true
                  );
  SendMostUsedEntryTable();
#endif
}

