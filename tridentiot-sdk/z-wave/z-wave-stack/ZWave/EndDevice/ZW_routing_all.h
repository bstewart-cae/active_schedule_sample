// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file ZW_routing_all.h
 * @copyright 2022 Silicon Laboratories Inc.
 *
 * @brief Routing common for all node types.
 */
#ifndef _ZW_ROUTING_ALL_H_
#define _ZW_ROUTING_ALL_H_

#ifdef USE_RESPONSEROUTE
/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <ZW_tx_queue.h>

/****************************************************************************/
/*                     EXPORTED TYPES and DEFINITIONS                       */
/****************************************************************************/
#define NO_ROUTE_TO_NODE 0xFF                       /* When returned no response route exist*/

#define MAX_ROUTED_RESPONSES 2
/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/
extern uint8_t  responseRouteNodeID[MAX_ROUTED_RESPONSES];        /* nodeID on the originator */
extern uint8_t  responseRouteSpeedNumHops[MAX_ROUTED_RESPONSES];  /* Number of hops and RF speed */
extern uint8_t  responseRouteRepeaterList[MAX_ROUTED_RESPONSES][MAX_REPEATERS]; /* List of repeaters */
extern uint8_t  responseRouteRFOption[MAX_ROUTED_RESPONSES];


/****************************************************************************/
/*                           EXPORTED FUNCTIONS                             */
/****************************************************************************/
/*============================   ResponseRouteFull   ======================
**    Function description
**      Checks if the response route buffer is full and locked
**    Side effects:
**
**--------------------------------------------------------------------------*/
uint8_t /*RET  True if buffer full and locked*/
ResponseRouteFull(void);


/*==========================   ResetRoutedResponseRoutes =====================
**    Function description
**      Resets all routed response route informations
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
void
ResetRoutedResponseRoutes( void );


/*============================   GetResponseRouteIndex   ======================
**    Function description
**      Returns Index to response route for node id. NO_ROUTE_TO_NODE If no match
**    Side effects:
**
**--------------------------------------------------------------------------*/
uint8_t                    /*RET NO_ROUTE_TO_NODE if no match*/
GetResponseRouteIndex(
  uint8_t bNodeID);        /* IN node ID to look for*/


/*============================   GetResponseRoute ======================
**    Function description
**      This function checks the responseRoutes and modify the activeTransmit frame
**      if with route
**      if a route exist Returns true if a route was added. false if not.
**    Side effects:
**
**--------------------------------------------------------------------------*/
uint8_t                                  /*RET true if match, false if not*/
GetResponseRoute(
  uint8_t bNodeID,                       /* IN node ID to look for*/
  TxQueueElement *frame               /*IN Frame to modify */
);

/*============================   RemoveResponseRoute   ======================
**    Function description
**        Removes response route to a specific node...
**    Side effects:
**
**--------------------------------------------------------------------------*/
void
RemoveResponseRoute(
  uint8_t bNodeID);            /*IN id of node to remove*/

/*============================   StoreRouteExplore  ==========================
**    Function description
**      Call this function to store a route retrieved from a received explore
**      frame into the temporary routes buffer.
**      If a route already exist it is overwritten.
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
uint8_t
StoreRouteExplore(
  uint8_t bNodeID,
  bool bRouteNoneInverted,
  uint8_t *pRoute);


/*============================   StoreRoute  ================================
**    Function description
**      Call this function to store a route in the temporary routes buffer.
**      If a route already exist it is overwritten.
**    Side effects:
**
**--------------------------------------------------------------------------*/
uint8_t
StoreRoute(
  uint8_t headerFormatType,   /* frameHeaderFormat type of received frame */
  uint8_t *pData,             /* pointer to a routed frame*/
  ZW_BasicFrameOptions_t* pFrameOption);  /*pointer to structure holding frame settings*/


#ifdef ZW_CONTROLLER
/*======================   RemoveResponseRouteWithRepeater   ================
**    Function description
**        Remove a response route that contains a specific repeater
**    Side effects:
**
**--------------------------------------------------------------------------*/
void
RemoveResponseRouteWithRepeater(
  uint8_t bNodeID);            /*IN id of node to remove*/
#endif /* ZW_CONTROLLER */


/*===========================   RouteCachedReset   ===========================
**    Function description
**      Reset bRouteCached false. If bRouteCached is false, makes it
**      possible to cache a received Route via the RouteCacheUpdate
**      functionality. If bRouteCached is true then all calls to the
**      RouteCacheUpdate functionality results in no Caching.
**
**    Side effects:
**      Resets bRouteCached to false
**
**--------------------------------------------------------------------------*/
void
RouteCachedReset(void);

/*=====================   ReturnRouteStoreForPowerDown   ======================
**    Function description
**      Store response routes in retention registers
**
**    Side effects:
**      Uses retention registers 0-3
**
**--------------------------------------------------------------------------*/
void
ReturnRouteStoreForPowerDown();

/*=====================   ReturnRouteRestoreAfterWakeup   ======================
**    Function description
**      Restore response routes from retention registers
**
**    Side effects:
**      None
**
**--------------------------------------------------------------------------*/
void
ReturnRouteRestoreAfterWakeup();

#endif  /* USE_RESPONSEROUTE */

#endif /*_ZW_ROUTING_ALL_H_*/

