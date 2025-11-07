// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file ZW_routing_cache.h
 * @copyright 2022 Silicon Laboratories Inc.
 *
 * @brief Header file for the Last Working Route cache module.
 */
#ifndef _ZW_ROUTING_CACHE_H_
#define _ZW_ROUTING_CACHE_H_

#include <ZW_tx_queue.h>
#include <ZW_lib_defines.h>

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/

/****************************************************************************/
/*                     EXPORTED TYPES and DEFINITIONS                       */
/****************************************************************************/

#define ROUTECACHE_LINE_DIRECT                  CACHED_ROUTE_LINE_DIRECT

#define ROUTECACHE_LINE_CONF_SIZE               1
#define ROUTECACHE_LINE_SIZE                    (MAX_REPEATERS + ROUTECACHE_LINE_CONF_SIZE)

#define ROUTECACHE_LINE_COUNT                   2


/* For LWR cache line configuration speed type definitions */
/* we use the TRANSMIT_SPEED_OPTION_BAUD_xxxx */

/* LWR cache line index definitions */
#define ROUTECACHE_LINE_REPEATER_0_INDEX        0
#define ROUTECACHE_LINE_REPEATER_1_INDEX        1
#define ROUTECACHE_LINE_REPEATER_2_INDEX        2
#define ROUTECACHE_LINE_REPEATER_3_INDEX        3
#define ROUTECACHE_LINE_CONF_INDEX              4


/*****************************************************************************/
/* Definitions for selecting and overall handling of Multiple CACHED ROUTEs  */
/* (APP_SR(ZW_LWR/ZW_NLWR). Used as parameter (selecting) and return (found) */
/* value in LastWorkingRouteCacheLineGet and LastWorkingRouteCacheLineExile  */

/* Select any Cached Route present - APP_SR, ZW_LWR or ZW_NLWR */
#define CACHED_ROUTE_ANY          0xFF
/* Select any ZW LWR - ZW_LWR or ZW_NLWR */
#define CACHED_ROUTE_ZW_ANY       0x0F
/* Select any APP SR - APP_SR */
#define CACHED_ROUTE_APP_ANY      0xF0

// Returned if no CACHED ROUTE found
#define CACHED_ROUTE_NONE         0x00
/* Select ZW LWR - Last Working Route - ZW_LWR */
#define CACHED_ROUTE_ZW_LWR       0x01
/* Select ZW LWR1 - Next to Last Working Route - ZW_NLWR */
#define CACHED_ROUTE_ZW_NLWR      0x02
/* Select APP SR - Application defined Static Route - APP_SR */
#define CACHED_ROUTE_APP_SR       0x10


extern uint8_t bCurrentLWRNo;

extern uint8_t bRouteCacheLineIn[];


/*===============   LastWorkingRouteCacheNodeSRLockedSet   ==================
**    Function description
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
void
LastWorkingRouteCacheNodeSRLockedSet(
  node_id_t bNodeID,
  uint8_t bValue);


/* LastWorkingRouteCacheLineStore return value definition */
typedef struct _S_LWR_STORE_RES_
{
  uint8_t bAppSRExists : 1;
  uint8_t bToggleNeeded : 1;
  uint8_t blCurrentLWR : 1;
  uint8_t bLWREqual : 1;
  uint8_t bNLWRSREqual : 1;
  uint8_t bLWRZERO : 1;
  uint8_t bNLWRSRZERO : 1;
  uint8_t bLWRStored : 1;
} S_LWR_STORE_RES;


/* LastWorkingRouteCacheLineStore  */
S_LWR_STORE_RES
LastWorkingRouteCacheLineStore(
  uint8_t bNodeID,
  uint8_t bLWRno);

/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/

/****************************************************************************/
/*                           EXPORTED FUNCTIONS                             */
/****************************************************************************/

/*====================   LastWorkingRouteCacheUpdate   =======================
**    Function description
**      Update EEPROM route cache for destID with route obtained from received
**      routed frame - handles both incoming and outgoing routed frames
**    Side effects:
**
**--------------------------------------------------------------------------*/
void
LastWorkingRouteCacheLineUpdate(
  uint8_t destID,
#ifdef MULTIPLE_LWR
  uint8_t bLWRno,
#endif
  uint8_t *pData);


/*================   LastWorkingRouteCacheLineExploreUpdate   ================
**    Function description
**      Update EEPROM route cache for destID with route obtained from received
**      explore frame
**    Side effects:
**
**--------------------------------------------------------------------------*/
void
LastWorkingRouteCacheLineExploreUpdate(
  uint8_t  bNodeID,
  bool bRouteNoneInverted,
  uint8_t *pRoute);


#ifdef MULTIPLE_LWR

/*=======================   LastWorkingRouteCacheLineGet   ===================
**    Function description
**      Returns true if a cached route is found. The found route is injected
**      directly into the specificed TxQueueElement structure with speed and
**      frame configuration bits updated accordingly.
**
**      bLWRno = CACHED_ROUTE_ANY     - true if any LWR present ->
**                                        APP_SR, ZW_LWR or ZW_NLWR placed in frame
**                                      false if NO APP_SR, ZW_LWR or ZW_NLWR
**                                        present
**               CACHED_ROUTE_ZW_ANY  - true if any ZW LWR present ->
**                                        ZW_LWR or ZW_NLWR placed in frame
**                                      false if NO ZW_LWR or ZW_NLWR present
**               CACHED_ROUTE_APP_ANY - true if any APP SR present ->
**                                        APP_SR placed in frame
**                                      false if NO APP SR present
**               CACHED_ROUTE_APP_SR  - true if APP_SR present ->
**                                        APP_SR placed in frame
**                                      false if APP_SR not present
**               CACHED_ROUTE_ZW_LWR  - true if ZW_LWR present
**                                        ZW_LWR placed in frame
**                                      false if ZW_LWR not present
**               CACHED_ROUTE_ZW_NLWR - true if ZW_NLWR present ->
**                                        ZW_NLWR placed in frame
**                                      false if ZW_NLWR not present
**
**    Side effects:
**      bCurrentLWRNo if updated with current LWR read
**--------------------------------------------------------------------------*/
uint8_t
LastWorkingRouteCacheLineGet(
  uint8_t bNodeID,
  uint8_t bLWRno,
  TxQueueElement *frame);


/*=======================   LastWorkingRouteCacheLineExists   ===================
**    Function description
**      Returns NONE ZERO indicating the LWR found. If any found then either
**        CACHED_ROUTE_APP_SR, CACHED_ROUTE_ZW_LWR or CACHED_ROUTE_ZW_NLWR
**        is returned.
**      Returns ZERO if no LWR found for specified bNodeID
**
**      bLWRno = CACHED_ROUTE_ANY     - true if any LWR present ->
**                                        APP_SR, ZW_LWR or ZW_NLWR placed in frame
**                                      false if NO APP_SR, ZW_LWR or ZW_NLWR
**                                        present
**               CACHED_ROUTE_ZW_ANY  - true if any ZW LWR present ->
**                                        ZW_LWR or ZW_NLWR placed in frame
**                                      false if NO ZW_LWR or ZW_NLWR present
**               CACHED_ROUTE_APP_ANY - true if any APP SR present ->
**                                        APP_SR placed in frame
**                                      false if NO APP SR present
**               CACHED_ROUTE_APP_SR  - true if APP_SR present ->
**                                        APP_SR placed in frame
**                                      false if APP_SR not present
**               CACHED_ROUTE_ZW_LWR  - true if ZW_LWR present
**                                        ZW_LWR placed in frame
**                                      false if ZW_LWR not present
**               CACHED_ROUTE_ZW_NLWR - true if ZW_NLWR present ->
**                                        ZW_NLWR placed in frame
**                                      false if ZW_NLWR not present
**
**    Side effects:
**      bRouteCacheLineOut contents changed
**--------------------------------------------------------------------------*/
uint8_t
LastWorkingRouteCacheLineExists(
  uint8_t bNodeID,
  uint8_t bLWRno);


/*=====================   LastWorkingRouteCacheLineExile   ===================
**    Function description
**      Exile specified, in bLWRno, CACHED ROUTEs.
**      if CACHED_ROUTE_ZW_LWR is specified in bLWRno and App SR exist
**        then clear ZW_LWR else do nothing.
**      if CACHED_ROUTE_ZW_NLWR is specified in bLWRno then clear ZW_NLWR
**      and toggle currentLWR marker.
**      Returns ZERO if no Application Static Route Exists for bNodeID
**      Returns ONE if Application Static Route Exists for bNodeID
**
**    Side effects:
**      If LastWorkRouteCache not locked then requested existing cache lines
**      for bNodeId is exiled
**
**--------------------------------------------------------------------------*/
uint8_t                            /* RET - ZERO if no App SR exists for bNodeID */
LastWorkingRouteCacheLineExile(
  uint8_t bNodeID,                 /*  IN - NodeID for which LWR is to be Exiled */
  uint8_t bLWRno);                 /*  IN - bitmask indicating which LWR is to be */
                                /*       Exiled */

#else

/*=======================   LastWorkingRouteCacheLineGet   ===================
**    Function description
**      Returns true if a cached routed is found. The found route is injected
**      into transmit element specified and the speed and frame
**      configuration bits updated
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
bool
LastWorkingRouteCacheLineGet(
  uint8_t destID,
  TxQueueElement *frame);


/*=====================   LastWorkingRouteCacheLinePurge   ===================
**    Function description
**      Remove any LastWorkRouteCache Line with bNodeID as a repeater
**    Side effects:
**
**--------------------------------------------------------------------------*/
void
LastWorkingRouteCacheLinePurge(
  uint8_t bNodeID);
#endif /* #ifdef MULTIPLE_LWR */


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


/*===================   LastWorkingRouteCacheNodeSRLocked   ==================
**    Function description
**      Returns NONE ZERO if an Application controlled
**              Static Route DO exist for bNodeID.
**      Returns ZERO if Application controlled
**              Static Route DO NOT exist for bNodeID
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
uint8_t
LastWorkingRouteCacheNodeSRLocked(
  uint8_t bNodeID);


#endif
