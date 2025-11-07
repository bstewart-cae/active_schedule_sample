// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file ZW_routing_all.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include "ZW_lib_defines.h"

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
//#define DEBUGPRINT
#include "DebugPrint.h"
#include <string.h>
#include <ZW_basis_api.h>
#include <ZW_transport.h>
#include <ZW_routing_all.h>
#ifdef ZW_CONTROLLER
#include <ZW_controller.h>
#endif
#include <ZW_explore.h>
#include <ZW_tx_queue.h>
#include <ZW_Frame.h>
#include <ZW_MAC.h>
#include <ZW_routing_all.h>
#include "ZW_Frame.h"
#include <zpal_retention_register.h>
#include <ZW_DataLinkLayer.h>

#ifdef USE_RESPONSEROUTE
/****************************************************************************/
/*                      PRIVATE TYPES and DEFINITIONS                       */
/****************************************************************************/
/* This must not be changed as is used be the transport layer to determine if direct or not */
#define RESPONSEROUTE_LINE_DIRECT  CACHED_ROUTE_LINE_DIRECT

bool  routesLocked = false;
bool  responseRouteLI;
/* bRouteCached: */
/* if false, makes it possible to cache a received Route via the RouteCacheUpdate functionality. */
/* If true then all calls to the RouteCacheUpdate functionality results in no Caching. */
bool bRouteCached;

uint8_t  responseRouteCount;         /* Keeps track of the number of routes in action*/

#ifndef ZW_3CH_SYSTEM
uint8_t responseRouteSpeedModified;  /* lsb=route 0, msb= route8 (2 is max), true bit means speed modified */
#endif

#define RESPONSE_ROUTE_SPEED_MODIFIED 0x01 /* Route is speed modified, mask after shiftright*/

/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/
uint8_t  responseRouteNodeID[MAX_ROUTED_RESPONSES];        /* nodeID on the originator */
uint8_t  responseRouteSpeedNumHops[MAX_ROUTED_RESPONSES];  /* Number of hops and RF speed */
uint8_t  responseRouteRepeaterList[MAX_ROUTED_RESPONSES][MAX_REPEATERS]; /* List of repeaters */
uint8_t  responseRouteRFOption[MAX_ROUTED_RESPONSES];

/****************************************************************************/
/*                              EXTERNAL DATA                               */
/****************************************************************************/

void
SetTransmitSpeedOptions(
  uint16_t speedOptions,
  TxQueueElement *pFrame);

uint16_t
GetTransmitSpeedOptions(TxQueueElement *pFrame);


/****************************************************************************/
/*                            PRIVATE FUNCTIONS                             */
/****************************************************************************/
/*============================   GetResponseRouteIndex   ======================
**    Function description
**      Returns Index to response route for node id. NO_ROUTE_TO_NODE If no match
**    Side effects:
**
**--------------------------------------------------------------------------*/
uint8_t                    /*RET NO_ROUTE_TO_NODE if no match*/
GetResponseRouteIndex(
  uint8_t bNodeID)         /* IN node ID to look for*/
{
  register uint8_t rtmp = MAX_ROUTED_RESPONSES;

  DPRINT("- GetResponseRouteIndex\r\n");

  if (bNodeID && responseRouteCount)
  {
    do
    {
      if (bNodeID == responseRouteNodeID[rtmp - 1])
      {
        return (rtmp - 1);
      }
    } while (--rtmp);
  }
  return NO_ROUTE_TO_NODE;
}


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
ResponseRouteFull(void)
{
  DPRINT("- ResponseRouteFull\r\n");
  return (((responseRouteCount == MAX_ROUTED_RESPONSES) && routesLocked));
}


/*==========================   ResetRoutedResponseRoutes =====================
**    Function description
**      Resets all response route informations
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
void
ResetRoutedResponseRoutes( void )
{
  DPRINT("- ResetRoutedResponseRoutes\r\n");
  memset(responseRouteNodeID, 0, MAX_ROUTED_RESPONSES);
}


/*============================   GetResponseRoute ============================
**    Function description
**      This function checks for a responseRoute to bNodeID and copies the
**      route to *frame if found.
**    Side effects:
**
**--------------------------------------------------------------------------*/
uint8_t                    /*RET true if activeTransmit changed*/
GetResponseRoute(
  uint8_t bNodeID,                 /*IN node ID to look for*/
  TxQueueElement *pFrame)        /*IN Frame to modify */
{
  register uint8_t rtmp = MAX_ROUTED_RESPONSES;
  register uint8_t repeaterCount;

  DPRINT("- GetResponseRoute\r\n");

  if (bNodeID && responseRouteCount)
  {
    do
    {
      if (bNodeID == responseRouteNodeID[rtmp-1])
      {
        repeaterCount = responseRouteSpeedNumHops[rtmp-1] & 0x0f;
        // We now the size of repeater list, so let's populate the routing header with that knowledge.
        // On a routed single cast, the repeater starts at index size of routing header, repeater list is variable size, so we must adjust according to number of repeaters.
        // (We assume extended header has already been added and pFrameHeaderStart is thus pointing correctly)
        // TODO: What to do if extended data is added later but after payload ? If extended data should go into payload, then everything should be fine.
        SET_RF_OPTION_SPEED(pFrame->wRFoptions, (responseRouteSpeedNumHops[rtmp-1] >> 4));
        /* Maybe a consequence for TO#01787 or TO#01841. Corrected unintentional zero-masks to TRANSMIT_SPEED_OPTION_BEAM_MASK */
        SetTransmitSpeedOptions((GetTransmitSpeedOptions(pFrame) &
                                ~(RF_OPTION_SEND_BEAM_250MS | RF_OPTION_SEND_BEAM_1000MS)) |
                                (responseRouteRFOption[rtmp-1] &
                                 (RF_OPTION_SEND_BEAM_250MS | RF_OPTION_SEND_BEAM_1000MS)),
                                 pFrame);

        // On a routed single cast, the repeater starts at index -repeaterCount (We assume extended header has already been added)
        if (1 == llIsHeaderFormat3ch())
        {
          memcpy(pFrame->frame.header.singlecastRouted3ch.repeaterList, responseRouteRepeaterList[rtmp-1], repeaterCount);
          pFrame->frame.header.singlecastRouted3ch.numRepsNumHops = repeaterCount << 4;
          pFrame->frame.headerLength = (sizeof(frameHeaderSinglecastRouted3ch)-sizeof(frameHeaderExtension3ch))-(MAX_REPEATERS-repeaterCount);
        }
        else
        {
          memcpy(&pFrame->frame.header.singlecastRouted.repeaterList, responseRouteRepeaterList[rtmp-1], repeaterCount);
          pFrame->frame.header.singlecastRouted.numRepsNumHops = repeaterCount << 4;
          pFrame->frame.headerLength = sizeof(frameHeaderSinglecastRouted)-(MAX_REPEATERS-repeaterCount);
        }
        if ((0 == llIsHeaderFormat3ch()) && (responseRouteSpeedModified & (RESPONSE_ROUTE_SPEED_MODIFIED << (rtmp-1))))
        {
          pFrame->bFrameOptions1 |= TRANSMIT_FRAME_OPTION_SPEED_MODIFIED;
          /* Clear speed modification in cache */
          /* We must first negate after doing the shift as we shift ZEROs in */
          responseRouteSpeedNumHops[rtmp-1] &= ~(RF_OPTION_SPEED_MASK << 4);
          /* Speed modified routes are always originally 100K */
          responseRouteSpeedNumHops[rtmp-1] |= (RF_SPEED_100K << 4);
          responseRouteSpeedModified &= ~(RESPONSE_ROUTE_SPEED_MODIFIED << (rtmp-1));
        }
        return true;
      }
    } while (--rtmp);
  }
  return false;
}


/*============================   StoreRoute  ================================
**    Function description
**      Call this function to store a route in the temporary routes buffer.
**      If a route already exist it is overwritten.
**    Side effects:
**
**--------------------------------------------------------------------------*/
uint8_t
StoreRoute(
  uint8_t headerTypeFormat, /* frameHeaderFormat type of received frame */
  uint8_t *pData,  /*pointer to a routed frame*/
  ZW_BasicFrameOptions_t* pFrameOption)   /*pointer to structure holding frame settings*/
{
  uint8_t* bPtr;
  uint8_t routeLen;
  uint8_t bNodeID;
  uint8_t tDistID;
  uint8_t isRouted;
  uint8_t notOutGoing;
  register uint8_t i;

  register uint8_t routetmp;

  DPRINT("- StoreRoute\r\n");
  if (HDRFORMATTYP_3CH == headerTypeFormat)
  {
    tDistID = ((frame *)pData)->singlecast3ch.destinationID;
  }
  else if (HDRFORMATTYP_2CH == headerTypeFormat)
  { 
    tDistID = ((frame *)pData)->singlecast.destinationID;
  }
  else
  {
    // since long range don't do routing then inform that no route has been saved
    return MAX_ROUTED_RESPONSES;
  }
  
  isRouted = IsRouted(headerTypeFormat, ((frame *)pData));
  notOutGoing = NotOutgoing(headerTypeFormat, ((frame *)pData));

  /* TO#2833 partial fix - Response Route/Last Working Route now never caches 9.6kb direct frames. */
  /* If Direct and 9.6kb - do not cache... */
  /* TO#2905 fix - Do not Cache Route if allready Cached. */
  if (bRouteCached || ((TransportGetCurrentRxSpeed() == RF_OPTION_SPEED_9600) && !isRouted))
  {
    return NO_ROUTE_TO_NODE;
  }
  bNodeID = (((frame *)pData)->header.sourceID != g_nodeID) ? ((frame *)pData)->header.sourceID : tDistID;
  routetmp = GetResponseRouteIndex(bNodeID);
  if (routetmp == NO_ROUTE_TO_NODE)
  {
    if (responseRouteCount < MAX_ROUTED_RESPONSES)
    {
      responseRouteCount++; /* One more route taken */
      for (routetmp = 0; routetmp < MAX_ROUTED_RESPONSES; routetmp++)
      {
        if (!responseRouteNodeID[routetmp])
        {
          break;  /* Found an empty route */
        }
      }
      if (routetmp == MAX_ROUTED_RESPONSES)
      {
        routetmp = MAX_ROUTED_RESPONSES - 1; /* Weird but we use this the last entry */
      }
      /* We will use this routeEntry */
      responseRouteLI = routetmp;
    }
    else
    { /* Buffer is full */
      if (!routesLocked)
      {
        /* Round robin - when 2 entries in responseRoute structure */
        responseRouteLI = !responseRouteLI;
        routetmp = responseRouteLI;
      }
      else
      {
        routetmp = MAX_ROUTED_RESPONSES - 1; /* Index is one less than max no of routes.. */
      }
    }
  }
  /* Even if buffer is locked we overwrite the last route */
  /* because we need it for routed busy signals */
  responseRouteNodeID[routetmp] = bNodeID;
  /* Direct can also be a response route */
  if (isRouted)
  {
    if (HDRFORMATTYP_2CH == headerTypeFormat)
    {
      if (GET_EXTEND_PRESENT(*((frame *)pData)))
      {
        responseRouteRFOption[routetmp] = (GET_EXTEND_BODY(*((frame *)pData), EXTEND_TYPE_WAKEUP_TYPE_SRC_DEST_OFFSET) & EXTEND_TYPE_WAKEUP_TYPE_SRC);
      }
      else
      {
        /* TO#2452 fix - reset the BEAM info if no BEAM info present in frame */
        responseRouteRFOption[routetmp] = 0;
      }
    }
     responseRouteSpeedNumHops[routetmp] = routeLen = GetRouteLen(headerTypeFormat, ((frame *)pData));
    if (HDRFORMATTYP_3CH == headerTypeFormat)
    {
      bPtr = ((frame *)pData)->singlecastRouted3ch.repeaterList;
    }
    else
    {
      bPtr = &((frame *)pData)->singlecastRouted.startOfRepeaterList;
    }
    if (!notOutGoing)
    {
      while (routeLen--)  /* Let us play it safe... */
      {
        responseRouteRepeaterList[routetmp][routeLen] = *bPtr;
        bPtr++;
      }
    }
    else
    {
    	i = 0;
      while (routeLen--)  /* Let us play it safe... */
      {
        responseRouteRepeaterList[routetmp][i++] = *bPtr;
        bPtr++;
      }
    }
  }
  else
  {
    responseRouteSpeedNumHops[routetmp] = 0;
    /* TO#2452 fix - reset the BEAM info if no BEAM info present in frame */
    responseRouteRFOption[routetmp] = 0;
    responseRouteRepeaterList[routetmp][0] = RESPONSEROUTE_LINE_DIRECT;
  }
  responseRouteSpeedNumHops[routetmp] |= (TransportGetCurrentRxSpeed() << 4);
  if (HDRFORMATTYP_2CH == headerTypeFormat)
  {
    if (IS_ROUTED_SPEED_MODIFIED(*((frame *)pData)) || pFrameOption->speedModified)
    {
      responseRouteSpeedModified |= (1 << routetmp);
    }
    else
    {
      responseRouteSpeedModified &= ~(1 << routetmp);
    }
  }
  /* Now Route is cached. */
  bRouteCached = true;
  /* Return index where route was saved */
  return routetmp;
}


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
  uint8_t *pRoute)
{
  uint8_t routeLen;
  register uint8_t i;

  register uint8_t routetmp;

  DPRINT("- StoreRouteExplore\r\n");

  /* TO#2905 fix - Do not Cache Route if allready Cached. */
  routetmp = NO_ROUTE_TO_NODE;
  if (!bRouteCached)
  {
    /* Find Response Route entry to use */
    routetmp = GetResponseRouteIndex(bNodeID);
    if (routetmp == NO_ROUTE_TO_NODE)
    {
      if (responseRouteCount < MAX_ROUTED_RESPONSES)
      {
        responseRouteCount++; /* One more route taken */
        for (routetmp = 0; routetmp < MAX_ROUTED_RESPONSES; routetmp++)
        {
          if (!responseRouteNodeID[routetmp])
          {
            break;  /* Found an empty route */
          }
        }
        if (routetmp == MAX_ROUTED_RESPONSES)
        {
          routetmp = MAX_ROUTED_RESPONSES - 1; /* Weird but we use this the last entry */
        }
      }
      else
      { /* Buffer is full */
        if (!routesLocked)
        {
          routetmp = 0; /* Loop back if routes not locked */
        }
        else
        {
          routetmp = MAX_ROUTED_RESPONSES - 1; /* Index is one less than max no of routes.. */
        }
      }
    }
    /* Even if buffer is locked we overwrite the last route */
    /* because we need it for routed busy signals */
    responseRouteNodeID[routetmp] = bNodeID;
    /* TO#2452 fix - reset the BEAM info if no BEAM info present in frame */
    responseRouteRFOption[routetmp] = 0;
    responseRouteSpeedNumHops[routetmp] = routeLen = (pRoute[0] & EXPLORE_REPEATERCOUNTSESSIONTTL_COUNT_MASK);
    responseRouteSpeedNumHops[routetmp] |= (TransportGetCurrentRxSpeed()  << 4);
    /* Any repeaters? */
    if (routeLen)
    {
      /* Repeaters exist */
      /* TO#2249 fix - We now only invert route if need to be... */
      i = 0;
      while (routeLen--)  /* Let us play it safe... */
      {
        if (bRouteNoneInverted)
        {
          pRoute++;
          responseRouteRepeaterList[routetmp][i++] = *pRoute;
        }
        else
        {
          pRoute++;
          responseRouteRepeaterList[routetmp][routeLen] = *pRoute;
        }
      }
    }
    else
    {
      /* Its a direct explore frame */
      responseRouteRepeaterList[routetmp][0] = RESPONSEROUTE_LINE_DIRECT;
    }
    if (0 == llIsHeaderFormat3ch())
    {
      /* Explore frame route not speed modified... */
      responseRouteSpeedModified &= (~(1 << routetmp));
    }
    /* Now Route is cached. */
    bRouteCached = true;
  }
  /* Return index where route was saved */
  return routetmp;
}


/*============================   ZW_LockRoute   ==============================
**    Function description
**      This function locks and unlocks any temporary route to a specific nodeID
**    Side effects:
**
**--------------------------------------------------------------------------*/
void
ZW_LockRoute(
  uint8_t bNodeID)          /*IN If nonezero lock bNodeID entry, */
                         /*   if zero unlock entry */
{
  register uint8_t routetmp;

  DPRINT("- ZW_LockRoute\r\n");

  /* Update Status */
  if (bNodeID)
  {
    routesLocked = true;
    routetmp = GetResponseRouteIndex(bNodeID);
    if (routetmp == (MAX_ROUTED_RESPONSES - 1))
    {
      /* The route we need are at the scratch spot, so we move it */
      responseRouteNodeID[0] = responseRouteNodeID[MAX_ROUTED_RESPONSES - 1];
      responseRouteNodeID[MAX_ROUTED_RESPONSES - 1] = 0; /* Free the scratch spot */
      responseRouteSpeedNumHops[0] = responseRouteSpeedNumHops[MAX_ROUTED_RESPONSES - 1];
      /* TO#2452 fix - reset the BEAM info if no BEAM info present in frame */
      responseRouteRFOption[0] = responseRouteRFOption[MAX_ROUTED_RESPONSES - 1];
      responseRouteCount--;   /* One more response route is free ;-) */
      if (0 == llIsHeaderFormat3ch())
      {
        /* copy speed modified bit to new entry (idx 0) */
        if (responseRouteSpeedModified & (RESPONSE_ROUTE_SPEED_MODIFIED << routetmp))
        {
          responseRouteSpeedModified |= RESPONSE_ROUTE_SPEED_MODIFIED;
        }
        else
        {
          responseRouteSpeedModified &=  (~RESPONSE_ROUTE_SPEED_MODIFIED);
        }
        /* clear speed modified bit for old entry */
        responseRouteSpeedModified &= (~(RESPONSE_ROUTE_SPEED_MODIFIED << routetmp));
      }
      for (routetmp = 0; routetmp < MAX_REPEATERS; routetmp++)
      {
        responseRouteRepeaterList[0][routetmp] = responseRouteRepeaterList[MAX_ROUTED_RESPONSES - 1][routetmp];
      }
    }
  }
  else
  {
    routesLocked = false;
  }
}


/*============================   RemoveResponseRoute   ======================
**    Function description
**        Removes response route to a specific node...
**    Side effects:
**
**--------------------------------------------------------------------------*/
void
RemoveResponseRoute(
  uint8_t bNodeID)            /*IN id of node to remove*/
{
  register uint8_t routetmp;

  DPRINT("- RemoveResponseRoute\r\n");

  routetmp = GetResponseRouteIndex(bNodeID);
  /*If Routes not Locked then clear route*/
  if ((routetmp != NO_ROUTE_TO_NODE) && !routesLocked)
  {
    /* If we are not locked and a route exist. Remove it now */
    responseRouteNodeID[routetmp] = 0;
    responseRouteCount--;
  }
}

#ifdef ZW_CONTROLLER
/*======================   RemoveResponseRouteWithRepeater   ================
**    Function description
**        Remove a response route that contains a specific repeater
**    Side effects:
**
**--------------------------------------------------------------------------*/
void
RemoveResponseRouteWithRepeater(
  uint8_t bNodeID)            /*IN id of node to remove*/
{
  register uint8_t rtmp = MAX_ROUTED_RESPONSES;
  register uint8_t btmp;

  DPRINT("- RemoveResponseRouteWithRepeater\r\n");

  do
  {
    if (responseRouteNodeID[rtmp - 1])
    {
      btmp = responseRouteSpeedNumHops[rtmp - 1];
      do
      {
        if (responseRouteRepeaterList[rtmp - 1][btmp-1] == bNodeID)
        {
          /* Repeater found in this route, delete it */
          RemoveResponseRoute(responseRouteNodeID[rtmp - 1]);
          break;
        }
      } while (--btmp);
    }
  } while (--rtmp);
}
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
RouteCachedReset(void)
{
  DPRINT("- RouteCachedReset\r\n");
  bRouteCached = false;
}

/*=====================   ReturnRouteStoreForPowerDown   ======================
**    Function description
**      Store response routes in retention registers
**
**    Side effects:
**      Uses retention registers 0-3
**
**--------------------------------------------------------------------------*/
void ReturnRouteStoreForPowerDown(void)
{
#if (MAX_ROUTED_RESPONSES != 2)
#error "This function requires MAX_ROUTED_RESPONSES to be equal to 2"
#endif
  uint32_t   Registervalue = 0;

  DPRINT("- ReturnRouteStoreForPowerDown\r\n");
  DPRINTF("   Route 1 - %d : %d %d,%d,%d,%d\r\n",   responseRouteNodeID[0],  responseRouteSpeedNumHops[0],
                                                    responseRouteRepeaterList[0][0],
                                                    responseRouteRepeaterList[0][1],
                                                    responseRouteRepeaterList[0][2],
                                                    responseRouteRepeaterList[0][3]);
  DPRINTF("   Route 2 - %d : %d %d,%d,%d,%d\r\n",   responseRouteNodeID[1],  responseRouteSpeedNumHops[1],
                                                    responseRouteRepeaterList[1][0],
                                                    responseRouteRepeaterList[1][1],
                                                    responseRouteRepeaterList[1][2],
                                                    responseRouteRepeaterList[1][3]);

  Registervalue |= responseRouteNodeID[0];
  Registervalue |= responseRouteNodeID[1] << 8;
  Registervalue |= responseRouteSpeedNumHops[0] << 16;
  Registervalue |= responseRouteSpeedNumHops[1] << 24;
  zpal_retention_register_write(ZPAL_RETENTION_REGISTER_RESPONSEROUTE_1, Registervalue);

  memcpy(&Registervalue, (uint8_t*)responseRouteRepeaterList, 4);
  zpal_retention_register_write(ZPAL_RETENTION_REGISTER_RESPONSEROUTE_2, Registervalue);
  memcpy(&Registervalue, ((uint8_t*)responseRouteRepeaterList)+4, 4);
  zpal_retention_register_write(ZPAL_RETENTION_REGISTER_RESPONSEROUTE_3, Registervalue);

  Registervalue = 0;
  Registervalue |= responseRouteCount;

  Registervalue |= responseRouteRFOption[0] << 8;
  Registervalue |= responseRouteRFOption[1] << 16;

  zpal_retention_register_write(ZPAL_RETENTION_REGISTER_RESPONSEROUTE_4, Registervalue);
}

/*=====================   ReturnRouteRestoreAfterWakeup   ======================
**    Function description
**      Restore response routes from retention registers
**
**    Side effects:
**      None
**
**--------------------------------------------------------------------------*/
void ReturnRouteRestoreAfterWakeup(void)
{
#if (MAX_ROUTED_RESPONSES != 2)
#error "This function requires MAX_ROUTED_RESPONSES to be equal to 2"
#endif
  uint32_t   Registervalue = 0;

  DPRINT("- ReturnRouteRestoreAfterWakeup\r\n");

  zpal_retention_register_read(ZPAL_RETENTION_REGISTER_RESPONSEROUTE_1, &Registervalue);
  responseRouteNodeID[0]        = (uint8_t)Registervalue;
  responseRouteNodeID[1]        = (uint8_t)(Registervalue >> 8);
  responseRouteSpeedNumHops[0]  = (uint8_t)(Registervalue >> 16);
  responseRouteSpeedNumHops[1]  = (uint8_t)(Registervalue >> 24);

  zpal_retention_register_read(ZPAL_RETENTION_REGISTER_RESPONSEROUTE_2, &Registervalue);
  memcpy((uint8_t*)responseRouteRepeaterList, &Registervalue, 4);
  zpal_retention_register_read(ZPAL_RETENTION_REGISTER_RESPONSEROUTE_3, &Registervalue);
  memcpy(((uint8_t*)responseRouteRepeaterList)+4, &Registervalue, 4);

  zpal_retention_register_read(ZPAL_RETENTION_REGISTER_RESPONSEROUTE_4, &Registervalue);
  responseRouteCount = (uint8_t)Registervalue;

  responseRouteRFOption[0] = (uint8_t)Registervalue >> 8;
  responseRouteRFOption[1] = (uint8_t)(Registervalue >> 16);

  DPRINTF("   Route 1 - %d : %d %d,%d,%d,%d\r\n",   responseRouteNodeID[0],  responseRouteSpeedNumHops[0],
                                                      responseRouteRepeaterList[0][0],
                                                      responseRouteRepeaterList[0][1],
                                                      responseRouteRepeaterList[0][2],
                                                      responseRouteRepeaterList[0][3]);
  DPRINTF("   Route 2 - %d : %d %d,%d,%d,%d\r\n",   responseRouteNodeID[1],  responseRouteSpeedNumHops[1],
                                                      responseRouteRepeaterList[1][0],
                                                      responseRouteRepeaterList[1][1],
                                                      responseRouteRepeaterList[1][2],
                                                      responseRouteRepeaterList[1][3]);
}

#endif  /* USE_RESPONSEROUTE */
