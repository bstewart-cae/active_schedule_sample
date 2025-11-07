// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file ZW_routing_cache.c
 * @copyright 2022 Silicon Laboratories Inc.
 *
 * @brief Last Working Route cache module.
 */
#include "ZW_lib_defines.h"

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <ZW_basis_api.h>
#include <ZW_transport.h>
#include <ZW_routing_cache.h>
#include "ZW_Frame.h"
#ifdef ZW_CONTROLLER
#include <ZW_controller.h>
#endif
#include <Assert.h>
#include <string.h> // For memset/memcpy
//#define DEBUGPRINT
#include <DebugPrint.h>
#include "ZW_controller_network_info_storage.h"
#include <ZW_DataLinkLayer.h>

/****************************************************************************/
/*                      PRIVATE TYPES and DEFINITIONS                       */
/****************************************************************************/


/****************************************************************************/
/*                              INTERNAL DATA                               */
/****************************************************************************/

uint8_t bRouteCacheLineIn[ROUTECACHE_LINE_SIZE];
uint8_t bRouteCacheLineOut[ROUTECACHE_LINE_SIZE];

/* LWR RouteCache selector nodemask - if bit is ZERO for a specific nodeID then the current LWR resides in */
/*                                    the EX_NVM_ROUTECACHE_START_far table  */
/*                                    if bit is ONE for a specific nodeID then the current LWR resides in */
/*                                    the EX_NVM_ROUTECACHE_NLWR_SR_START_far table */
/* As the RouteCache selector is XRAM based then we need the Application Static Route to be placed */
/* in the EX_NVM_ROUTECACHE_NLWR_SR_START_far table - This means that if a LWR is placed in the */
/* EX_NVM_ROUTECACHE_NLWR_SR_START_far table and the Application sets a Static Route then the */
/* LWR needs to be moved to the EX_NVM_ROUTECACHE_START_far table so that we can determine which */
/* table the LWR is placed for a specific nodeID and which table is the Application Static Route for */
/* specific route */
uint8_t abRouteCacheLWRSelector[MAX_NODEMASK_LENGTH] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                                      0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                                      0, 0, 0, 0, 0, 0, 0, 0, 0};
/* IF true then any attempt to purge a nodeIDs LastWorkingRoute entry is denied */
bool lastWorkRouteCacheLock = false;

/* bRouteCached: */
/* if false, makes it possible to cache a received Route via the RouteCacheUpdate functionality. */
/* If true then all calls to the RouteCacheUpdate functionality results in no Caching. */
bool bRouteCached;

#ifdef MULTIPLE_LWR
/**********************************************************************/
/* bCurrentLWRNo = Current Static Route in use                        */
/*   XXXX....  Top 4 bit = bLWR_APP != ZERO                           */
/*             indicates Application controlled Route active          */
/*   ....YYYY  Bottom 4 bit = bLWR_ZW != ZERO                         */
/*             indicates number on last protocol LWR used             */
/*                                                                    */
/*  Current LWRs exists :                                             */
/*  bLWR_App = 0x00 App Static Route not active                       */
/*             0x01 App Static Route active                           */
/*  bLWR_ZW  = 0x01 ZW Last Working Route active                      */
/*             0x02 ZW Previous Last Working Route active             */
uint8_t bCurrentLWRNo;
#endif

/****************************************************************************/
/*                              EXTERNAL DATA                               */
/****************************************************************************/

/****************************************************************************/
/*                           EXPORTED FUNCTIONS                             */
/****************************************************************************/


#ifdef MULTIPLE_LWR
uint8_t
LastWorkingRouteCacheCurrentLWRSet(
  uint8_t bNodeID,
  uint8_t bValue)
{
  uint8_t blVal;
  bNodeID--;
  blVal = abRouteCacheLWRSelector[bNodeID >> 3];
  if (bValue)
  {
    blVal |= (0x1 << (bNodeID & 7));
  }
  else
  {
    blVal &= ~(0x1 << (bNodeID & 7));
  }
  abRouteCacheLWRSelector[bNodeID >> 3] = blVal;
  return bValue;
}


uint8_t
LastWorkingRouteCacheCurrentLWRGet(
  uint8_t bNodeID)
{
  bNodeID--;
  return ((abRouteCacheLWRSelector[bNodeID >> 3] >> (bNodeID & 7)) & 0x01);
}


uint8_t
LastWorkingRouteCacheCurrentLWRToggle(
  uint8_t bNodeID)
{
  return LastWorkingRouteCacheCurrentLWRSet(bNodeID, !LastWorkingRouteCacheCurrentLWRGet(bNodeID));
}


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
  uint8_t bNodeID)
{  
  return CtrlStorageGetAppRouteLockFlag(bNodeID);
}


/*===============   LastWorkingRouteCacheNodeSRLockedSet   ==================
**    Function description
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
void
LastWorkingRouteCacheNodeSRLockedSet(
  node_id_t bNodeID,
  uint8_t bValue)
{
  CtrlStorageSetAppRouteLockFlag(bNodeID, bValue? true:false);
}


/*===================   LastWorkingRouteCacheLineStore   =====================
**    Function description
**      Store Route in LWR structure if needed
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
S_LWR_STORE_RES
LastWorkingRouteCacheLineStore(
  uint8_t bNodeID,
  uint8_t bLWRno)
{
  S_LWR_STORE_RES sResult = { 0 };

  sResult.bAppSRExists = LastWorkingRouteCacheNodeSRLocked(bNodeID);
  sResult.blCurrentLWR = LastWorkingRouteCacheCurrentLWRGet(bNodeID);

  if (CACHED_ROUTE_APP_SR == bLWRno)
  {
    if (0 == sResult.bAppSRExists)
    {
      /* Only check if NLWR placed route needs to be moved if we are not allready using App Priority Route */
      if (1 == sResult.blCurrentLWR)
      {

        CtrlStorageGetRouteCache(ROUTE_CACHE_NLWR_SR, bNodeID, (ROUTECACHE_LINE *)&bRouteCacheLineOut);
        /* TODO: Check if New Route is equal to old LWR and if so - do not exile old ZW_LWR, just "overwrite" it */
        if (bRouteCacheLineOut[ROUTECACHE_LINE_REPEATER_0_INDEX] != 0)
        {
          /* Only Exile if Route not a App Priority Route */
          if (0 != ((bRouteCacheLineOut[0] ^ bRouteCacheLineIn[0]) | (bRouteCacheLineOut[1] ^ bRouteCacheLineIn[1]) |
                    (bRouteCacheLineOut[2] ^ bRouteCacheLineIn[2]) | (bRouteCacheLineOut[3] ^ bRouteCacheLineIn[3]) |
                    (bRouteCacheLineOut[4] ^ bRouteCacheLineIn[4])))
          {
            CtrlStorageSetRouteCache(ROUTE_CACHE_NORMAL, bNodeID, (ROUTECACHE_LINE *)&bRouteCacheLineOut);
          }
        }
        else
        {
          sResult.bNLWRSRZERO = 1;
        }
      }
      /* Now indicate we are going Priority Route */
      LastWorkingRouteCacheNodeSRLockedSet(bNodeID, 1);
    }
    if (0 == sResult.blCurrentLWR)
    {
      /* Incase LWRSelector is out of sync (after RESET) - make sure LWR Selector is correct */
      LastWorkingRouteCacheCurrentLWRSet(bNodeID, 1);
      sResult.blCurrentLWR = 1;
    }
    sResult.bAppSRExists = 1;
    sResult.bLWRStored = 1;
    CtrlStorageSetRouteCache(ROUTE_CACHE_NLWR_SR, bNodeID,(ROUTECACHE_LINE *)& bRouteCacheLineIn);
    /* Priority Route now exists for bNodeID */
  }
  else
  {
    if (CACHED_ROUTE_ZW_LWR == bLWRno)
    {
      CtrlStorageGetRouteCache(ROUTE_CACHE_NORMAL, bNodeID,(ROUTECACHE_LINE *)&bRouteCacheLineOut);
      if (0 == bRouteCacheLineOut[0])
      {
        sResult.bLWRZERO = 1;
      }
      else
      {
        if (0 == ((bRouteCacheLineOut[0] ^ bRouteCacheLineIn[0]) | (bRouteCacheLineOut[1] ^ bRouteCacheLineIn[1]) |
                  (bRouteCacheLineOut[2] ^ bRouteCacheLineIn[2]) | (bRouteCacheLineOut[3] ^ bRouteCacheLineIn[3]) |
                  (bRouteCacheLineOut[4] ^ bRouteCacheLineIn[4])))
        {
          sResult.bLWREqual = 1;
        }
      }
      CtrlStorageGetRouteCache(ROUTE_CACHE_NLWR_SR, bNodeID,(ROUTECACHE_LINE *)&bRouteCacheLineOut);
      if (0 == bRouteCacheLineOut[0])
      {
        sResult.bNLWRSRZERO = 1;
      }
      else
      {
        if (0 == ((bRouteCacheLineOut[0] ^ bRouteCacheLineIn[0]) | (bRouteCacheLineOut[1] ^ bRouteCacheLineIn[1]) |
                  (bRouteCacheLineOut[2] ^ bRouteCacheLineIn[2]) | (bRouteCacheLineOut[3] ^ bRouteCacheLineIn[3]) |
                  (bRouteCacheLineOut[4] ^ bRouteCacheLineIn[4])))
        {
          sResult.bNLWRSREqual = 1;
        }
      }
      if (((1 == sResult.blCurrentLWR) && (0 == sResult.bNLWRSRZERO)) || (0 != sResult.bLWRZERO))
      {
        if (0 == sResult.bNLWRSREqual)
        {
          if (0 == sResult.bLWREqual)
          {
            CtrlStorageSetRouteCache(ROUTE_CACHE_NORMAL, bNodeID,(ROUTECACHE_LINE *)&bRouteCacheLineIn);
            sResult.bLWRStored = 1;
          }
          if ((1 == sResult.blCurrentLWR) && (0 == sResult.bAppSRExists))
          {
            sResult.bToggleNeeded = 1;
          }
        }
        else
        {
          if (0 == sResult.blCurrentLWR)
          {
            sResult.bToggleNeeded = 1;
          }
        }
      }
      else
      {
        if ((0 == sResult.bLWREqual) && (0 == sResult.bAppSRExists))
        {
          if (0 == sResult.bNLWRSREqual)
          {

            CtrlStorageSetRouteCache(ROUTE_CACHE_NLWR_SR, bNodeID,(ROUTECACHE_LINE *)&bRouteCacheLineIn);
            sResult.bLWRStored = 1;
          }
          if (0 == sResult.blCurrentLWR)
          {
            sResult.bToggleNeeded = 1;
          }
        }
        else
        {
          if ((1 == sResult.blCurrentLWR) && (0 == sResult.bAppSRExists))
          {
            sResult.bToggleNeeded = 1;
          }
        }
      }
      if (0 != sResult.bToggleNeeded)
      {
        LastWorkingRouteCacheCurrentLWRToggle(bNodeID);
      }
    }
  }
  return sResult;
}
#endif


/*====================   LastWorkingRouteCacheLineUpdate   ===================
**    Function description
**      Update NVM route cache for bNodeID with route in pData
**    Side effects:
**
**--------------------------------------------------------------------------*/
void
LastWorkingRouteCacheLineUpdate(
  uint8_t bNodeID,
#ifdef MULTIPLE_LWR
  uint8_t bLWRno,
#endif
  uint8_t *pData)     /* Pointer to a routed frame */
{
  uint8_t *bPtr;
  uint8_t tRouteLen;
  uint8_t outgoing;
  uint8_t isrouted;
  uint8_t issensor;
  uint8_t i;

  DPRINTF("Cu%02X", bNodeID);
  /* Do not Cache Route if allready Cached. */
  /* Do not change any Cache Route while including/excluding or being include/excludedß. */
  if (!bRouteCached && !lastWorkRouteCacheLock)
  {
    uint8_t nodeIs2ch = (0 == llIsHeaderFormat3ch());
    ZW_HeaderFormatType_t curHeader = (nodeIs2ch)? HDRFORMATTYP_2CH : HDRFORMATTYP_3CH;

    issensor = IsNodeSensor(bNodeID, true, true);
    isrouted = IsRouted(curHeader, ((frame *)pData));
    /* If Direct and 9.6kb - do not cache... */
    if (nodeIs2ch && (TransportGetCurrentRxSpeed() == RF_OPTION_SPEED_9600) && !isrouted)
    {
      DPRINT("U");
      return;
    }
    if (bNodeID && (bNodeID <= ZW_MAX_NODES))
    {
      /* First we assume that there always  are 1 repeater when we get here, so zero the rest */
      bRouteCacheLineIn[ROUTECACHE_LINE_REPEATER_1_INDEX] = 0;
      bRouteCacheLineIn[ROUTECACHE_LINE_REPEATER_2_INDEX] = 0;
      bRouteCacheLineIn[ROUTECACHE_LINE_REPEATER_3_INDEX] = 0;
      if (isrouted)
      {
        DPRINT("R");
        if (nodeIs2ch)
        {
          bPtr = ((frameTx *)pData)->singlecastRouted.repeaterList;
        }
        else
        {
          bPtr = ((frameTx *)pData)->singlecastRouted3ch.repeaterList;
        }
        tRouteLen = GetRouteLen(curHeader, (frame *)pData);
        outgoing = NotOutgoing(curHeader, (frame *)pData);

        if (tRouteLen > MAX_REPEATERS)
        {
          return;
        }
        if (!outgoing)
        {
          while (tRouteLen--)  /* Let us play it safe... */
          {
            bRouteCacheLineIn[tRouteLen] = *bPtr;
            bPtr++;
          }
        }
        else
        {
        	i = 0;
          while (tRouteLen--)  /* Let us play it safe... */
          {
            bRouteCacheLineIn[i++] = *bPtr;
            bPtr++;
          }
        }
        if (!nodeIs2ch)
        {
          if (issensor)
          {
            bRouteCacheLineIn[ROUTECACHE_LINE_CONF_INDEX] = RF_OPTION_SPEED_100K |
              (((issensor & ZWAVE_NODEINFO_SENSOR_MODE_MASK)
                                  == ZWAVE_NODEINFO_SENSOR_MODE_WAKEUP_250) ? RF_OPTION_SEND_BEAM_250MS : RF_OPTION_SEND_BEAM_1000MS);
          }
          else
          {
            /* Always 100K with GHZ protocol */
            bRouteCacheLineIn[ROUTECACHE_LINE_CONF_INDEX] = RF_OPTION_SPEED_100K;
          }
        }
        else
        {
          DPRINTF("b%02X%02X", issensor, TransportGetCurrentRxSpeed());
          if (issensor && (TransportGetCurrentRxSpeed() != RF_OPTION_SPEED_9600))
          {
            /* Destination is FLiRS node and the route is not 9.6k */

            /* Check if last repeater in route supports beaming */
            if (GetNodesSecurity(bRouteCacheLineIn[GET_ROUTE_LEN(*((frame *)pData))-1]) & ZWAVE_NODEINFO_BEAM_CAPABILITY)
            {
              DPRINT("r");
              /* Set beam speed and force route to 40k */
              bRouteCacheLineIn[ROUTECACHE_LINE_CONF_INDEX] = RF_OPTION_SPEED_40K |
                          (((issensor & ZWAVE_NODEINFO_SENSOR_MODE_MASK)
                          == ZWAVE_NODEINFO_SENSOR_MODE_WAKEUP_250) ? RF_OPTION_SEND_BEAM_250MS : RF_OPTION_SEND_BEAM_1000MS);
            }
          }
          else
          {
            uint8_t bSpeed = TransportGetCurrentRxSpeed();
            switch(bSpeed) {
              case RF_OPTION_SPEED_40K: __attribute__ ((fallthrough));
              case RF_OPTION_SPEED_100K: 
                bRouteCacheLineIn[ROUTECACHE_LINE_CONF_INDEX] = bSpeed;
                break;
              default:
                bRouteCacheLineIn[ROUTECACHE_LINE_CONF_INDEX] = RF_OPTION_SPEED_9600;
                break;
            }
          }
        }
      }
      else
      {        
        DPRINT("D");
        /* Its a direct frame */
        bRouteCacheLineIn[ROUTECACHE_LINE_REPEATER_0_INDEX] = ROUTECACHE_LINE_DIRECT;
        bRouteCacheLineIn[ROUTECACHE_LINE_CONF_INDEX] = 0;  /* No repeaters */
        DPRINTF("b%02X%02X", issensor, TransportGetCurrentRxSpeed());
          /* Check if destination is a FLiRS node */
        if (issensor)
        {
          if (nodeIs2ch)
          {
            /* FLiRS node, set beam speed and force 40K */
            bRouteCacheLineIn[ROUTECACHE_LINE_CONF_INDEX] = RF_OPTION_SPEED_40K;
          }
          else
          {
            bRouteCacheLineIn[ROUTECACHE_LINE_CONF_INDEX] = RF_OPTION_SPEED_100K;
          }
          bRouteCacheLineIn[ROUTECACHE_LINE_CONF_INDEX] |=  (((issensor & ZWAVE_NODEINFO_SENSOR_MODE_MASK)
                                                               == ZWAVE_NODEINFO_SENSOR_MODE_WAKEUP_250) ? RF_OPTION_SEND_BEAM_250MS : RF_OPTION_SEND_BEAM_1000MS);
        }
        else
        {
          if (nodeIs2ch)
          {
            /* Not a FLiRS, use receive speed */
            bRouteCacheLineIn[ROUTECACHE_LINE_CONF_INDEX] = TransportGetCurrentRxSpeed();
          }
          else
          {
            /* Always 100K with GHZ protocol */
            bRouteCacheLineIn[ROUTECACHE_LINE_CONF_INDEX] = RF_OPTION_SPEED_100K;
          }
        }
      }
#ifdef MULTIPLE_LWR
      /* Store new cached route (bRouteCacheLineIn) at the by bLWRno specified place in NVM */
      /* If ZW_LWR then exile ZW_old LWR if not same or NULL - else just Store */
      LastWorkingRouteCacheLineStore(bNodeID, bLWRno);
#else
      CtrlStorageSetRouteCache(ROUTE_CACHE_NORMAL, bNodeID, (ROUTECACHE_LINE *)&bRouteCacheLineIn);
#endif
      /* Now Route is cached. */
      bRouteCached = true;
    }
  }
  DPRINTF("U%02X", bRouteCacheLineIn[ROUTECACHE_LINE_CONF_INDEX]);
}


/*=================   LastWorkingRouteCacheLineExploreUpdate   ===============
**    Function description
**      Update NVM route cache for bNodeID with route obtained from received
**      explore frame
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
void
LastWorkingRouteCacheLineExploreUpdate(
  uint8_t bNodeID,
  bool bRouteNoneInverted,
  uint8_t *pRoute)
{
  uint8_t tRouteLen;
  register uint8_t n;

  DPRINTF("Ce%02X", bNodeID);
  /* TO#2905 fix - Do not Cache Route if allready Cached. */
//  if (!bRouteCached)
  {
    /* First check if bNodeID is valid and if there is any route to store */
    if (bNodeID && (bNodeID <= ZW_MAX_NODES))
    {
      /* First we assume that there always are 1 repeater when we get here, so zero the rest */
      bRouteCacheLineIn[ROUTECACHE_LINE_REPEATER_1_INDEX] = 0;
      bRouteCacheLineIn[ROUTECACHE_LINE_REPEATER_2_INDEX] = 0;
      bRouteCacheLineIn[ROUTECACHE_LINE_REPEATER_3_INDEX] = 0;
      /* All explore frames contain repeater information - also the initial direct explore frame */
      if (*pRoute & EXPLORE_REPEATERCOUNTSESSIONTTL_COUNT_MASK)
      {
        DPRINT("R");
        tRouteLen = (*pRoute & EXPLORE_REPEATERCOUNTSESSIONTTL_COUNT_MASK);
        if (tRouteLen > MAX_REPEATERS)
        {
          return;
        }
        n = 0;
        while (tRouteLen--)  /* Let us play it safe... */
        {
          if (bRouteNoneInverted)
          {
            pRoute++;
            bRouteCacheLineIn[n++] = *pRoute;
          }
          else
          {
            pRoute++;
            bRouteCacheLineIn[tRouteLen] = *pRoute;
          }
        }
      }
      else
      {
        DPRINT("D");
        /* Received a direct Explore frame */
        bRouteCacheLineIn[ROUTECACHE_LINE_REPEATER_0_INDEX] = ROUTECACHE_LINE_DIRECT;
      }
      /* Explore frames are always transmitted at 40k+ */
      bRouteCacheLineIn[ROUTECACHE_LINE_CONF_INDEX] = TransportGetCurrentRxSpeed();
#ifdef MULTIPLE_LWR
      /* Store new cached route (bRouteCacheLineIn) at the by bLWRno specified place in NVM */
      /* If ZW_LWR then exile ZW_old LWR if not same or NULL - else just Store */
      LastWorkingRouteCacheLineStore(bNodeID, CACHED_ROUTE_ZW_LWR);
#else
      CtrlStorageSetRouteCache(ROUTE_CACHE_NORMAL, bNodeID, (ROUTECACHE_LINE *)&bRouteCacheLineIn);
#endif
      /* Now Route is cached. */
      bRouteCached = true;
    }
  }
  DPRINT("E");
}


#ifdef MULTIPLE_LWR
/*=======================   LastWorkingRouteCacheLineExists   ===================
**    Function description
**      Returns NONE ZERO indicating the LWR found. If any found then either
**        CACHED_ROUTE_APP_SR, CACHED_ROUTE_ZW_LWR or CACHED_ROUTE_ZW_NLWR
**        is returned (bCurrentLWRNo) and the found LWR is placed in
**        bRouteCacheLineOut structure.
**      Returns ZERO if no LWR found for specified bNodeID.
**
**     bLWRno              Function description       Function return value
** CACHED_ROUTE_ANY      Retrive either APP_SR(p1)   APP_SR, ZW_LWR or ZW_NLWR
**                       ZW_LWR (p2) or ZW_NLWR(p3)  Route descriptor returned
**                                                   if any route found
**                                                   route is placed in bRouteCacheLineOut
**                                                   false if no route found
** CACHED_ROUTE_ZW_ANY   Retrieve ZW_LWR or ZW_NLWR  ZW_LWR or ZW_NLWR
**                                                   Route descriptor returned
**                                                   if any route found
**                                                   route is placed in bRouteCacheLineOut
**                                                   false if no route found
** CACHED_ROUTE_APP_ANY  Retrieve any APP_SR         APP_SR
**                                                   Route descriptor returned
**                                                   if any route found
**                                                   route is placed in bRouteCacheLineOut
**                                                   false if no route found
** CACHED_ROUTE_APP_SR   Retrieve APP_SR             APP_SR
**                                                   Route descriptor returned
**                                                   if any route found
**                                                   route is placed in bRouteCacheLineOut
**                                                   false if no route found
** CACHED_ROUTE_ZW_LWR   Retrieve ZW_LWR             ZW_LWR
**                                                   Route descriptor returned
**                                                   if any route found
**                                                   route is placed in bRouteCacheLineOut
**                                                   false if no route found
** CACHED_ROUTE_ZW_NLWR  Retrieve ZW_NLWR            ZW_NLWR
**                                                   Route descriptor returned
**                                                   if any route found
**                                                   route is placed in bRouteCacheLineOut
**                                                   false if no route found
**
**    Side effects:
**      bRouteCacheLineOut contents changed
**--------------------------------------------------------------------------*/
uint8_t
LastWorkingRouteCacheLineExists(
  uint8_t bNodeID,
  uint8_t bLWRno)
{
  bCurrentLWRNo = CACHED_ROUTE_NONE;
  ROUTE_CACHE_TYPE routeCacheType;
  if (bNodeID && (bNodeID <= ZW_MAX_NODES))
  {
    uint8_t bAppSRExists;
    /* Initially nothing found */
    bAppSRExists = LastWorkingRouteCacheNodeSRLocked(bNodeID);
    bRouteCacheLineOut[ROUTECACHE_LINE_REPEATER_0_INDEX] = 0;
    if (0 != (CACHED_ROUTE_APP_SR & bLWRno) && (0 != bAppSRExists))
    {
      /* Application controller Static Route (APP_SR) requested */
      CtrlStorageGetRouteCache(ROUTE_CACHE_NLWR_SR, bNodeID, (ROUTECACHE_LINE *)&bRouteCacheLineOut);
      if (((0 < bRouteCacheLineOut[ROUTECACHE_LINE_REPEATER_0_INDEX]) &&
           (ZW_MAX_NODES >= bRouteCacheLineOut[ROUTECACHE_LINE_REPEATER_0_INDEX])) ||
          (CACHED_ROUTE_LINE_DIRECT == bRouteCacheLineOut[ROUTECACHE_LINE_REPEATER_0_INDEX]))
      {
        bCurrentLWRNo = CACHED_ROUTE_APP_SR;
        LastWorkingRouteCacheCurrentLWRSet(bNodeID, 1);
      }
    }
    if (0 == bCurrentLWRNo)
    {
      /* Either APP_SR not requested or APP_SR not found */
      if (0 != (bLWRno & CACHED_ROUTE_ZW_LWR))
      {
        /* Current Last Working Route (ZW_LWR) requested */
        // Determine which file to load from
        if ((0 == LastWorkingRouteCacheCurrentLWRGet(bNodeID)) || (0 != bAppSRExists))
        {
          routeCacheType = ROUTE_CACHE_NORMAL;
        }
        else
        {
          routeCacheType = ROUTE_CACHE_NLWR_SR;
        }
        CtrlStorageGetRouteCache(routeCacheType, bNodeID, (ROUTECACHE_LINE *)&bRouteCacheLineOut);
        if (((0 < bRouteCacheLineOut[ROUTECACHE_LINE_REPEATER_0_INDEX]) &&
             (ZW_MAX_NODES >= bRouteCacheLineOut[ROUTECACHE_LINE_REPEATER_0_INDEX])) ||
            (CACHED_ROUTE_LINE_DIRECT == bRouteCacheLineOut[ROUTECACHE_LINE_REPEATER_0_INDEX]))
        {
          bCurrentLWRNo = CACHED_ROUTE_ZW_LWR;
        }
      }
      /* If no Route found yet and no Application Static Route exists -> then only one protocol LWR exists */
      if ((0 == bCurrentLWRNo) && (0 == bAppSRExists))
      {
        /* Either ZW_LWR not requested or ZW_LWR not found and no App SR exists */
        if (0 != (bLWRno & CACHED_ROUTE_ZW_NLWR))
        {
          /* Next to Last Working Route (ZW_NLWR) requested */
          // Determine which file to load from
          if (0 == LastWorkingRouteCacheCurrentLWRGet(bNodeID))
          {
            routeCacheType = ROUTE_CACHE_NLWR_SR;
          }
          else
          {
            routeCacheType = ROUTE_CACHE_NORMAL;
          }
          CtrlStorageGetRouteCache(routeCacheType, bNodeID, (ROUTECACHE_LINE *)&bRouteCacheLineOut);
          if (((0 < bRouteCacheLineOut[ROUTECACHE_LINE_REPEATER_0_INDEX]) &&
               (ZW_MAX_NODES >= bRouteCacheLineOut[ROUTECACHE_LINE_REPEATER_0_INDEX])) ||
              (CACHED_ROUTE_LINE_DIRECT == bRouteCacheLineOut[ROUTECACHE_LINE_REPEATER_0_INDEX]))
          {
            bCurrentLWRNo = CACHED_ROUTE_ZW_NLWR;
          }
        }
      }
    }
  }
  return bCurrentLWRNo;
}
#endif


/*=======================   LastWorkingRouteCacheLineGet   ===================
**    Function description
#ifdef MULTIPLE_LWR
**      Returns NONE ZERO indicating the LWR ID found. The found route is injected
#else
**      Returns true if a cached route is found. The found route is injected
#endif
**      directly into the specificed TxQueueElement structure with speed and
**      frame configuration bits updated accordingly.
**
#ifdef MULTIPLE_LWR
**     bLWRno              Function description       Function return value
** CACHED_ROUTE_ANY      Retrive either APP_SR(p1)   APP_SR, ZW_LWR or ZW_NLWR
**                       ZW_LWR (p2) or ZW_NLWR(p3)  Route descriptor returned
**                                                   if any route found
**                                                   and route placed in frame
**                                                   false if no route found
** CACHED_ROUTE_ZW_ANY   Retrieve ZW_LWR or ZW_NLWR  ZW_LWR or ZW_NLWR
**                                                   Route descriptor returned
**                                                   if any route found
**                                                   and route placed in frame
**                                                   false if no route found
** CACHED_ROUTE_APP_ANY  Retrieve any APP_SR         APP_SR
**                                                   Route descriptor returned
**                                                   if any route found
**                                                   and route placed in frame
**                                                   false if no route found
** CACHED_ROUTE_APP_SR   Retrieve APP_SR             APP_SR
**                                                   Route descriptor returned
**                                                   if any route found
**                                                   and route placed in frame
**                                                   false if no route found
** CACHED_ROUTE_ZW_LWR   Retrieve ZW_LWR             ZW_LWR
**                                                   Route descriptor returned
**                                                   if any route found
**                                                   and route placed in frame
**                                                   false if no route found
** CACHED_ROUTE_ZW_NLWR  Retrieve ZW_NLWR            ZW_NLWR
**                                                   Route descriptor returned
**                                                   if any route found
**                                                   and route placed in frame
**                                                   false if no route found
#endif
**
**    Side effects:
**      bCurrentLWRNo is updated with LWR descriptor found
**--------------------------------------------------------------------------*/
#ifdef MULTIPLE_LWR
uint8_t
#else
bool
#endif
LastWorkingRouteCacheLineGet(
  uint8_t bNodeID,
#ifdef MULTIPLE_LWR
  uint8_t bLWRno,
#endif
  TxQueueElement *frame)        /*IN Frame to modify */
{
  uint8_t repeaterCount;
  uint8_t* tmp_numRepsNumHops;
  uint8_t* tmp_repeaterList;
  
  if (1 == llIsHeaderFormat3ch())  
  {
    tmp_numRepsNumHops = &frame->frame.header.singlecastRouted3ch.numRepsNumHops;
    tmp_repeaterList = frame->frame.header.singlecastRouted3ch.repeaterList;
  }
  else
  {
    tmp_numRepsNumHops = &frame->frame.header.singlecastRouted.numRepsNumHops;
    tmp_repeaterList = frame->frame.header.singlecastRouted.repeaterList;
  }
  
  DPRINTF("Cg%02X", bNodeID);
  if (bNodeID && (bNodeID <= ZW_MAX_NODES))
  {
#ifdef MULTIPLE_LWR
    if (CACHED_ROUTE_NONE != LastWorkingRouteCacheLineExists(bNodeID, bLWRno))
#else
    CtrlStorageGetRouteCache(ROUTE_CACHE_NORMAL, bNodeID, (ROUTECACHE_LINE *)&bRouteCacheLineOut);

    if (bRouteCacheLineOut[ROUTECACHE_LINE_REPEATER_0_INDEX] != 0)
#endif  /* #ifdef MULTIPLE_LWR */
    {
      DPRINT("+");
      frame->wRFoptions &= ~(RF_OPTION_SPEED_MASK | RF_OPTION_BEAM_MASK);
      frame->wRFoptions |= ((bRouteCacheLineOut[ROUTECACHE_LINE_CONF_INDEX]) & (RF_OPTION_SPEED_MASK | RF_OPTION_BEAM_MASK));
      repeaterCount = ROUTECACHE_LINE_REPEATER_0_INDEX;
      while ((bRouteCacheLineOut[repeaterCount]) && (repeaterCount < MAX_REPEATERS))
      {
        tmp_repeaterList[repeaterCount] = bRouteCacheLineOut[repeaterCount];
        repeaterCount++;
      }
      /* Route resolution enhancement - Direct is also a valid LWR  */
      /* First repeater will contain ROUTECACHE_LINE_DIRECT if route is direct */
      /* Routing scheme update */
      if (tmp_repeaterList[0] == CACHED_ROUTE_LINE_DIRECT)
      {
        /* The Last Working Route is a Direct Route. */
        tmp_repeaterList[0] = 0;
        /* When direct there is no repeaters */
        repeaterCount = 0;
      }
      *tmp_numRepsNumHops = repeaterCount << 4;
      /* Return true if first repeater is nonezero - this means that a route is present for destID */
      DPRINTF("G%02X", frame->wRFoptions);
      return true;
    }
  }
  DPRINT("-G");
  return false;
}


#ifdef MULTIPLE_LWR
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
  uint8_t bLWRno)                  /*  IN - bitmask indicating which LWR is to be */
                                /*       Exiled */
{
  uint8_t bAppSRExists = 0;
  if (!lastWorkRouteCacheLock)
  {
    uint8_t aZeroes[ROUTECACHE_LINE_SIZE];
    memset(aZeroes, 0, sizeof(aZeroes));
    bAppSRExists = LastWorkingRouteCacheNodeSRLocked(bNodeID);
    if (CACHED_ROUTE_ZW_NLWR == (CACHED_ROUTE_ZW_NLWR & bLWRno))
    {
      uint8_t blNewLWR = (1 == bAppSRExists) ? 0 : LastWorkingRouteCacheCurrentLWRToggle(bNodeID);
      /* Exile ZW_NLWR -> Clear - Old ZW_LWR now the ZW_NLWR */
      ROUTE_CACHE_TYPE rCacheType = ROUTE_CACHE_NORMAL;
      if (1 == blNewLWR)
      {
          rCacheType = ROUTE_CACHE_NLWR_SR;
      }
      CtrlStorageSetRouteCache(rCacheType, bNodeID, (ROUTECACHE_LINE *)&aZeroes);
    }
    if (CACHED_ROUTE_ZW_LWR == (CACHED_ROUTE_ZW_LWR & bLWRno))
    {
      /* if AppSR Exists LWR Exiled (Clear) */
      /* if no AppSR Exist, then LWR is Exiled at ZW_NLWR Exile */
      if (1 == bAppSRExists)
      {
        CtrlStorageSetRouteCache(ROUTE_CACHE_NLWR_SR, bNodeID, (ROUTECACHE_LINE *)&aZeroes);
      }
    }
  }
  return bAppSRExists;
}


#else

/*=====================   LastWorkingRouteCacheLinePurge   ===================
**    Function description
**      Remove LastWorkRouteCache Line for bNodeID
**
**    Side effects:
**      If LastWorkRouteCache not locked then any existing cache line for
**      bNodeId is purged
**
**--------------------------------------------------------------------------*/
void
LastWorkingRouteCacheLinePurge(
  uint8_t bNodeID)
{
  if (!lastWorkRouteCacheLock)
  {
    uint8_t aZeroes[ROUTECACHE_LINE_SIZE];
    memset(aZeroes, 0, sizeof(aZeroes));
    CtrlStorageSetRouteCache(ROUTE_CACHE_NORMAL, bNodeID, (ROUTECACHE_LINE *)&aZeroes);
  }
}
#endif


/*=============================   ZW_LockRoute   ============================
**    Function description
**      IF bLockRoute true then any attempt to purge a LastWorkingRoute entry
**      is denied.
**
**    Side effects:
**
**
**--------------------------------------------------------------------------*/
void
ZW_LockRoute(
  bool bLockRoute)
{
  lastWorkRouteCacheLock = bLockRoute;
}

/**
   * Return if current LastWorkingRoute is locked 
   * 
   * @return true if current LastWorkingRoute is locked for update/change
*/
bool
ZW_LockRouteGet(void)
{
  return lastWorkRouteCacheLock;
}

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
RouteCachedReset(void)
{
  bRouteCached = false;
}


/*==========================   ZW_GetPriorityRoute   ======================
**    Function description
**      Returns NON ZERO if a Priority Route is found. Priority route is either
**        an Application injected Route or a LWR.
**        ZW_PRIORITY_ROUTE_APP_PR = Route is an App defined Priority Route
**        ZW_PRIORITY_ROUTE_ZW_LWR = Route is a Last Working Route
**        ZW_PRIORITY_ROUTE_ZW_NLWR = Route is a Next to Last Working Route
**      Returns false if no Priority Route is found.
**      If Route is found then the found route is copied into the specified
**      ROUTECACHE_LINE_SIZE (5) byte sized byte array, where the first
**      4 bytes (index 0-3) contains the repeaters active in the route and
**      the last (index 4) byte contains the speed information.
**      First ZERO in repeaters (index 0-3) indicates no more repeaters in route
**      A direct route is indicated by the first repeater (index 0) being ZERO.
**
**      Example: 0,0,0,0,ZW_PRIORITY_ROUTE_SPEED_100K ->
**                  Direct 100K
**               2,3,0,0,ZW_PRIORITY_ROUTE_SPEED_40K  ->
**                  40K route through repeaters 2 and 3
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
#ifdef MULTIPLE_LWR
uint8_t
#else
bool
#endif
ZW_GetPriorityRoute(
  node_id_t bNodeID,
  uint8_t *pPriorityRoute)
{
#ifdef MULTIPLE_LWR
  uint8_t bRetVal = false;
#endif
  if (bNodeID && (bNodeID <= ZW_MAX_NODES))
  {
#ifdef MULTIPLE_LWR
    /* Get Priority Route - If no App generated Priority exists then LWR is returned, if any */
    bRetVal = LastWorkingRouteCacheLineExists(bNodeID, CACHED_ROUTE_ANY);
    if (bRetVal)
    {
      /* A Route exist */
      pPriorityRoute[ROUTECACHE_LINE_REPEATER_0_INDEX] = bRouteCacheLineOut[ROUTECACHE_LINE_REPEATER_0_INDEX];
      pPriorityRoute[ROUTECACHE_LINE_REPEATER_1_INDEX] = bRouteCacheLineOut[ROUTECACHE_LINE_REPEATER_1_INDEX];
      pPriorityRoute[ROUTECACHE_LINE_REPEATER_2_INDEX] = bRouteCacheLineOut[ROUTECACHE_LINE_REPEATER_2_INDEX];
      pPriorityRoute[ROUTECACHE_LINE_REPEATER_3_INDEX] = bRouteCacheLineOut[ROUTECACHE_LINE_REPEATER_3_INDEX];
      pPriorityRoute[ROUTECACHE_LINE_CONF_INDEX] = bRouteCacheLineOut[ROUTECACHE_LINE_CONF_INDEX];
      pPriorityRoute[ROUTECACHE_LINE_CONF_INDEX] &= RF_OPTION_SPEED_MASK;
      if (pPriorityRoute[ROUTECACHE_LINE_REPEATER_0_INDEX] == CACHED_ROUTE_LINE_DIRECT)
      {
        /* Route is a direct route */
        pPriorityRoute[ROUTECACHE_LINE_REPEATER_0_INDEX] = 0;
      }
    }
#else
    NVM_GETBUFFER((uint16_t)&EX_NVM_ROUTECACHE_START_far[bNodeID - 1], pPriorityRoute, ROUTECACHE_LINE_SIZE);
    pPriorityRoute[ROUTECACHE_LINE_CONF_INDEX] &= RF_OPTION_SPEED_MASK;
    if (pPriorityRoute[ROUTECACHE_LINE_REPEATER_0_INDEX] != 0)
    {
      /* A Route exist */
      if (pPriorityRoute[ROUTECACHE_LINE_REPEATER_0_INDEX] == CACHED_ROUTE_LINE_DIRECT)
      {
        /* Route is a direct route */
        pPriorityRoute[ROUTECACHE_LINE_REPEATER_0_INDEX] = 0;
      }
      return true;
    }
#endif
  }
#ifdef MULTIPLE_LWR
  return bRetVal;
#else
  return false;
#endif
}


/*==========================   ZW_SetPriorityRoute   ======================
**    Function description
**      Returns true if specified pPriorityRoute has been saved for bNodeID.
**      Returns false if specified bNodeID is NOT valid and no PriorityRoute
**      has been saved for bNodeID.
**      pPriorityRoute MUST point to a ROUTECACHE_LINE_SIZE (5) byte sized
**      byte array containing the wanted route. The first 4 bytes (index 0-3)
**      contains the repeaters active in the route and last (index 4) byte
**      contains the speed information.
**      First ZERO in repeaters (index 0-3) indicates no more repeaters in route.
**      A direct route is indicated by the first repeater (index 0) being ZERO.
**
**      Example: 0,0,0,0,ZW_PRIORITY_ROUTE_SPEED_100K ->
**                  Direct 100K
**               2,3,0,0,ZW_PRIORITY_ROUTE_SPEED_40K  ->
**                  40K route through repeaters 2 and 3
**               2,3,4,0,ZW_PRIORITY_ROUTE_SPEED_9600 ->
**                  9600 route through repeaters 2, 3 and 4
**
**    Side effects:
**      If any LWR (different from specified pPriorityRoute) exists at
**      the Priority Route entry in NVM, it will be Exiled
**      to become NextToLastWorkingRoute (NLWR).
**
**--------------------------------------------------------------------------*/
bool
ZW_SetPriorityRoute(
  node_id_t nodeID,
  uint8_t *pPriorityRoute)
{
  if (nodeID && (nodeID <= ZW_MAX_NODES))
  {
    if (0 != pPriorityRoute)
    {
      uint8_t fIsSensor;
      if (1 == llIsHeaderFormat3ch())
      {
        pPriorityRoute[ROUTECACHE_LINE_CONF_INDEX] = RF_OPTION_SPEED_100K;
        fIsSensor = IsNodeSensor(nodeID, true, true);
        if (fIsSensor)
        {
          pPriorityRoute[ROUTECACHE_LINE_CONF_INDEX] |=
            (((fIsSensor & ZWAVE_NODEINFO_SENSOR_MODE_MASK) == ZWAVE_NODEINFO_SENSOR_MODE_WAKEUP_250) ?
              RF_OPTION_SEND_BEAM_250MS : RF_OPTION_SEND_BEAM_1000MS);
        }
      }
      else
      {
        pPriorityRoute[ROUTECACHE_LINE_CONF_INDEX] &= RF_OPTION_SPEED_MASK;
        fIsSensor = IsNodeSensor(nodeID, true, true);
        if (fIsSensor)
        {
          pPriorityRoute[ROUTECACHE_LINE_CONF_INDEX] = RF_OPTION_SPEED_40K |
            (((fIsSensor & ZWAVE_NODEINFO_SENSOR_MODE_MASK) == ZWAVE_NODEINFO_SENSOR_MODE_WAKEUP_250) ?
              RF_OPTION_SEND_BEAM_250MS : RF_OPTION_SEND_BEAM_1000MS);
        }
      }
      if (pPriorityRoute[ROUTECACHE_LINE_REPEATER_0_INDEX] == 0)
      {
        /* Route is a direct route */
        pPriorityRoute[ROUTECACHE_LINE_REPEATER_0_INDEX] = CACHED_ROUTE_LINE_DIRECT;
      }
      bRouteCacheLineIn[ROUTECACHE_LINE_REPEATER_0_INDEX] = pPriorityRoute[ROUTECACHE_LINE_REPEATER_0_INDEX];
      bRouteCacheLineIn[ROUTECACHE_LINE_REPEATER_1_INDEX] = pPriorityRoute[ROUTECACHE_LINE_REPEATER_1_INDEX];
      bRouteCacheLineIn[ROUTECACHE_LINE_REPEATER_2_INDEX] = pPriorityRoute[ROUTECACHE_LINE_REPEATER_2_INDEX];
      bRouteCacheLineIn[ROUTECACHE_LINE_REPEATER_3_INDEX] = pPriorityRoute[ROUTECACHE_LINE_REPEATER_3_INDEX];
      bRouteCacheLineIn[ROUTECACHE_LINE_CONF_INDEX] = pPriorityRoute[ROUTECACHE_LINE_CONF_INDEX];
      LastWorkingRouteCacheLineStore(nodeID, CACHED_ROUTE_APP_SR);
      /* If bNodeID was a FLiRS, the BEAM info are now present in pLastWorkingRoute[ROUTECACHE_LINE_CONF_INDEX] */
    }
    else
    {
      /* Golden Route now released and will be used by protocol as Next to Last Working Route (NLWR) */
      LastWorkingRouteCacheNodeSRLockedSet(nodeID, 0);
    }
    /* Make sure that the priority route is saved to non volatile memory*/
    StoreNodeRouteCacheFile(nodeID);
    return true;
  }
  return false;
}
