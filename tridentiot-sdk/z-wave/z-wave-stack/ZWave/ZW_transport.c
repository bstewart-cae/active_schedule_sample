// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file ZW_transport.c
 * @copyright 2022 Silicon Laboratories Inc.
 *
 * @brief Z-Wave Transport layer module.
 */
#include "ZW_lib_defines.h"
#include <ZW_typedefs.h>
#include <Assert.h>
#include <NodeMask.h>

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <string.h>
#ifdef ZW_SLAVE
#include <ZW_basis_api.h>
#include <ZW_slave.h>
#endif  /*ZW_SLAVE*/
#include <ZW_basis.h>

#ifdef ZW_CONTROLLER
#include <ZW_controller.h>
#include <ZW_routing.h>
#endif /*ZW_CONTROLLER*/

/*Commmon for all targets*/
#include <ZW_timer.h>
#include <SwTimer.h>
#include <TickTime.h>
#include <zpal_entropy.h>
#include <zpal_radio.h>

#include <ZW_protocol.h>
#include <ZW_transport.h>
#include <ZW_network_management.h>
#include <zpal_radio_utils.h>

#include <ZW_tx_queue.h>
#include <ZW_txq_protocol.h>

//#define DEBUGPRINT
#include "DebugPrint.h"

#include <ZW_DataLinkLayer.h>
#include "ZW_DataLinkLayer_utils.h"
#include <ZW_receivefilter_transport.h>

#include <ZW_main_region.h>

#ifdef ZW_SLAVE
#ifdef USE_RESPONSEROUTE
#include <ZW_routing_all.h>
#endif
#endif
#ifdef ZW_CONTROLLER
#include <ZW_routing_cache.h>
#endif
#include <ZW_explore.h>

#if defined(ZW_SLAVE_ROUTING) || defined(ZW_CONTROLLER)
#include <ZW_transport_api.h>
#endif

#ifdef ZW_SECURITY_PROTOCOL
#include <ZW_libsec.h>
#include <ZW_Security_Scheme0.h>
#include <ZW_Security_Scheme2.h>
#ifdef USE_TRANSPORT_SERVICE
#include <transport_service2_external.h>
#endif
#endif

#ifdef ZW_CONTROLLER
#include <ZW_controller_network_info_storage.h>
#endif

#ifdef ZW_SLAVE
#include <ZW_slave_network_info_storage.h>
#endif

#include <ZW_protocol_interface.h>
#include <ZW_frames_filters.h>
#include <ZW_ismyframe.h>
#include <ZW_build_tx_header.h>

// Includes needed for Dynamic Transmission Power Algorithm
#include "ZW_dynamic_tx_power.h"
#include "ZW_dynamic_tx_power_algorithm.h"
#include "ZW_basis.h"
#include "ZW_dynamic_tx_power.h"
#include "ZW_lr_virtual_node_id.h"
#include "ZW_Frame.h"

/****************************************************************************/
/*                      PRIVATE TYPES and DEFINITIONS                       */
/****************************************************************************/

// Set default TXPower to 14 dBm
#define APP_DEFAULT_TX_POWER_LR      14
// Minimum RSSI used by the dynamic TX power algorithm
#define DYN_TXPOWER_ALG_MIN_RSSI     -102

#ifdef ZW_SECURITY_PROTOCOL
typedef enum _E_EX_ERRCODES
{
    E_EX_ERRCODE_TX_FAILED = 0,
    E_EX_ERRCODE_TX_IN_PROGRESS = 1,
    E_EX_ERRCODE_NO_SECURITY = 2,
    E_EX_ERRCODE_NO_SECURITY_2 = 3
} E_EX_ERRCODES;
#endif

/* A memcpy with builtin buffer overrun protection
 * If the copy length is longer than maxcount, only maxcount bytes
 * will be copied. */
static inline void safe_memcpy(void *dst, const void *src, size_t count, size_t maxcount)
{
  ASSERT(count <= maxcount);
  memcpy(dst, src, (count > maxcount) ? maxcount : count);
}

/* Rx frame status/action */

/* Give up on trying to transmit a frame when this count is reached. Happens when receiver is busy for this count */
#define TX_MAX_RETRY_COUNT    100

/* Number of timeouts we wait for a fragmented beam to be broadcast by last repeater */
/* (currently each timeout is 1.1 sec) */
#define FRAG_BEAM_WAIT_COUNT 3

static uint8_t mTransportRxCurrentCh;
uint8_t mTransportRxCurrentSpeed;     /* Saves speed of latest received frame in case the ASIC receives  */
                                      /* another frame while we are processing the current one.          */
static uint8_t mTransportTxCurrentCh; /* The current Tx channel used */


/* Check if we have currently received a 40K frame */
#define IS_CURRENT_SPEED_40K (mTransportRxCurrentSpeed == RF_SPEED_40K)

/* TO#2523 Fix */
uint8_t bRestartAckTimerAllowed;  /* 1 - Restart of timer is allowed */

bool bLastTxFailed;               /* Status of the last transmission */

/* local function prototypes */
void RetransmitFail( void );

/* TO#2535 fix update */
bool isRoutedAckErr;
bool bApplicationTxAbort;

node_id_t gNodeLastReceivedFrom;

bool bUseExploreAsRouteResolution;
uint8_t crH[HOMEID_LENGTH];  /* HomeID on frame just received */

uint8_t inclusionHomeID[HOMEID_LENGTH];
bool inclusionHomeIDActive;

#ifdef USE_TRANSPORT_SERVICE
static uint8_t m_transport_service_buffer[RX_MAX];  /* Local buffer for Transport Service data, we pass read-only pointer to transport_service2 */
#endif

#ifdef ZW_BEAM_RX_WAKEUP
static bool m_FragmentedBeamACKAllowed = true;
static void ResumeFragmentedBeamAckTx(void);
/*this timer used to delay the sending of beam ack (3ch and LR) for the duration of BEAM_TRAIN_DURATION_MS*/
SSwTimer m_flirs_beam_ack_delay_timer = {.pCallback = NULL};
#endif

/****************************************************************************/
/*                             EXTERNAL DATA                                */
/****************************************************************************/
/* From ZW_txq_protocol.c */
extern TxQueueElement *pFrameWaitingForACK;

/* From ZW_txq_protocol.c */
extern uint8_t waitingForRoutedACK;

/****************************************************************************/
/*                              PRIVATE DATA                                */
/****************************************************************************/


/* Mask definition to mask out framtype and seqno in the frameType byte in transmitElement  */
#define MASK_FRAMETYPE_FRAMETYPE  0x0f
#define MASK_FRAMETYPE_SEQNO      0xf0

static SSwTimer TransportTimer = { 0 };


#if defined(ZW_CONTROLLER) || defined(ZW_SLAVE_ROUTING)

#ifdef ZW_RETURN_ROUTE_PRIORITY
/* Priority list for changing the order of the stored return routes */
/* The first ZW_MAX_RETURN_ROUTE_DESTINATIONS are for normal return routes and */
/* the last entry is for the SUC route */
/* In Slave Enhanced 232 targets we use 2 bit for every route */
uint8_t abReturnRoutePriority[ZW_MAX_RETURN_ROUTE_DESTINATIONS + 1]; /* _at_ RETURN_ROUTE_PRIORITY_ADDR; */
uint8_t bReturnRoutePriorityIndex;
#endif

#ifdef ZW_SLAVE_ROUTING
uint8_t destRouteIndex;
RETURN_ROUTE returnRoute;
#endif
/* SLAVE Routing multiple destinations feature takedown... */
//uint8_t singleDestinationRoute;
#endif  /* ZW_CONTROLLER || ZW_SLAVE_ROUTING */

#ifdef ZW_CONTROLLER_BRIDGE
extern node_id_t virtualNodeID;                 /* Current virtual nodeID */
extern uint8_t learnSlaveMode;
bool virtualNodeDest;
#endif  /* ZW_CONTROLLER_BRIDGE */


uint8_t sACK;

/* Holds the received frames pointer to payload and soforth */

RX_FRAME rxFrame = { 0 };

static bool myFrame;

#ifdef ZW_SLAVE
/* Here we store node IDs of the last MAX_RESPONSE_SPEEDS nodes
 * who sent to us with a given speed. Same speed is used when
 * responding with a direct range tx. */
typedef struct _SPEED_TABLE_
{
  node_id_t wNodeID;
  uint8_t  bResponseSpeed;
} SPEED_TABLE;
SPEED_TABLE  abDirectNodesResponseSpeed[MAX_RESPONSE_SPEED_NODES];
/* TODO: Could add caching of FLiRS wakeup speed to response nodes */
uint8_t iResponseNodeIndex; /* Index for abDirectNodesResponseSpeed */
#endif /* ZW_SLAVE */

/* Next sequence number to be used when transmitting a frame */
uint8_t currentSeqNoTx;

/* current sequence number on last received frame */
/* also used as sequence number currently used in active RX transaction */
/* sequence number extracted from last received frame in a transaction not initiated by this node */
/* frames transmitted from this node needs its answering ACK and Routed_ACK/Routed_ERR frames */
/* to contain this sequencenumber, for receving to function */
uint8_t currentSeqNo;

#define MASK_SEQNO_DIRECT 0x0f
#define MASK_SEQNO_ROUTED 0xf0
/* Sequence numbers holder for 9600 frames. */
/* Upper nibble contains the sequence number for routed frames originating from this node. */
/* Lower nibble contains the sequence number for direct frames originating from this node */
/* The sequence numbers are independently and accordingly incremented everytime a new frame */
/* transaction is initiated from this node. Frames received as answer to this frame transaction */
/* needs to have the correct sequence number to be present for the received ACK or */
/* Routed_ACK/Routed_ERR frame to be considered valid - answers containing the sequence */
/* number 0 are for compatibility reasons always connsidered valid */
uint8_t current9600SeqNosTX;
/* Sequence numbers holder for 40k frames. */
/* Upper nibble contains the sequence number for routed frames originating from this node. */
/* Lower nibble contains the sequence number for direct frames originating from this node */
/* The sequence numbers are independently and accordingly incremented everytime a new frame */
/* transaction is initiated from this node. Frames received as answer to this frame transaction */
/* needs to have the correct sequence number to be present for the received ACK or */
/* Routed_ACK/Routed_ERR frame to be considered valid - answers containing the sequence */
/* number 0 are for compatibility reasons always connsidered valid */
uint8_t current40kSeqNosTX;

/* Flag telling if currentSeqNoTX is to be used in the current transaction - else we use currentSeqNo */
bool currentSeqNoUseTX;

/* Array holding rssi feedback values for application
 * Lowest index is last tx to this node. Higher indices are
 * rssi heard by repeaters at increasing distance from
 * this node.
 * So abRssiFeedback[1+MAX_REPEATERS] contains rssi from
 * repeater furthest away from this node. */
static uint8_t abRssiFeedback[1 + MAX_REPEATERS];

#ifdef USE_TRANSPORT_SERVICE
/* Application-level callback func for*/
static STransmitCallback ts_stored_callback = { .pCallback = NULL, .Context = NULL };
#endif /* #ifdef USE_TRANSPORT_SERVICE */

#ifdef ZW_SECURITY_PROTOCOL
E_EX_ERRCODES eErrcode;
#endif

/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/
/* Current active transmission */

ACTIVE_TRANSMIT activeTransmit = { 0 };

/* Receive options for the last received frame */
/* Passed by reference to ApplicationCommandHandlerEx */
RECEIVE_OPTIONS_TYPE rxopt = { 0 };

static uint8_t bRoutedRssiFeedbackEnabled; /* Set to True to append RSSI collection field to
                                    Routed ACKs */

/****************************************************************************/
/*                            PRIVATE FUNCTIONS                             */
/****************************************************************************/
#ifdef ZW_CONTROLLER_STATIC
static void getNextRoute(
  uint8_t bRouteSpeed,
  uint8_t bRepeaterCountInRoute,
  uint8_t *speed,
  uint8_t  *frameType,
  ZW_HeaderFormatType_t curHeaderFormat,
  TxQueueElement *pFreeTxElement)
{
  uint8_t repeaterNodes[MAX_REPEATERS] = { 0 };
  /* Search Return Route Table from beginning */
  abLastStartNode[0] = 0;
  if (GetNextRouteToNode(pFreeTxElement->frame.frameOptions.destinationNodeId, true,
                          repeaterNodes, &bRepeaterCountInRoute, &bRouteSpeed))
  {
    pFreeTxElement->bRouteSchemeState = ZW_ROUTINGSCHEME_ROUTE;
    if (HDRFORMATTYP_3CH != curHeaderFormat)
    {
      pFreeTxElement->frame.header.singlecastRouted.numRepsNumHops = bRepeaterCountInRoute << 4;
      memcpy(pFreeTxElement->frame.header.singlecastRouted.repeaterList, repeaterNodes, bRepeaterCountInRoute);
    }
    else
    {
      pFreeTxElement->frame.header.singlecastRouted3ch.numRepsNumHops = bRepeaterCountInRoute << 4;
      memcpy(pFreeTxElement->frame.header.singlecastRouted3ch.repeaterList, repeaterNodes, bRepeaterCountInRoute);
    }
    pFreeTxElement->wRFoptions = SetSpeedOptions(pFreeTxElement->wRFoptions, bRouteSpeed);
    *frameType = FRAME_TYPE_ROUTED;
    /* TO#2783 fix */
    /* Disable automatic speed selection in EnqueueCommon() */
    *speed = bRouteSpeed;
  }
}
#endif

#ifdef USE_TRANSPORT_SERVICE
/**
 * Callback function from Transport Service senddata.
 * Converts the transmit status parameters from libs2 void types to the proper TX_STATUS_TYPE used here.
 *
 * @param txStatus             The numeric Transmit Status (succes/failure)
 * @param voidExtendedTxStatus Structure containing extra transmit status information.
 */
void ZCB_TsCallbackFunc(uint8_t txStatus, void *voidExtendedTxStatus)
{
  TX_STATUS_TYPE *extendedTxStatus = (TX_STATUS_TYPE*)voidExtendedTxStatus;
  if (0 != ts_stored_callback.pCallback)
  {
    ZW_TransmitCallbackInvoke(&ts_stored_callback, txStatus, extendedTxStatus);
    ts_stored_callback.pCallback = 0;
  }
}
#endif /* #ifdef USE_TRANSPORT_SERVICE */

#ifdef ZW_SLAVE
uint8_t
ChooseSpeedForDestination_slave( /* RET Chosen speed RF_SPEED_* */
  node_id_t pNodeID);                 /* IN  Destination nodeID      */
#endif

#ifdef ZW_CONTROLLER
uint8_t
ChooseSpeedBeamForDestination_Common( /* RET Chosen speed RF_OPTION_SPEED_* */
  node_id_t       bNodeID,            /* IN  Destination nodeID      */
  TxQueueElement *pFrame,             /* IN Frame to choose speed for */
  uint8_t         bFrameType          /* IN Frametype. Singlecast, routed... */
);
#endif

#if defined(ZW_CONTROLLER) || defined(ZW_SLAVE_ROUTING)
static void ClearFrameOptions (ZW_TransmissionFrame_t * frame);
#endif

/*===============================   EnqueueSingleDataPtr   =====================
**    Transmit data buffer to a single Z-Wave node or all Z-Wave nodes (broadcast).
**    Takes a ptr to TxQueue element, allowing more control of txOptions
**    from calling functions.
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
static uint8_t                 /*RET false if transmitter busy      */
EnqueueSingleDataPtr(
  uint8_t      rfSpeed,        /* IN RF baud rate*/
  node_id_t    srcNodeID,      /* IN Source node ID */
  node_id_t    destNodeID,     /* IN Destination node ID, if equal to 0xFF then all nodes */
  uint8_t const * const pData, /* IN Data buffer pointer           */
  uint8_t      dataLength,     /* IN Data buffer length            */
  const STransmitCallback* pCompletedFunc,  /* TODO: This argument can be optimized out */
  TxQueueElement *pFreeTxElement    /*IN  Transmit completed call back function  */
);

#define RESTART_ACK_TIMER()        TimerRestart(&TransportTimer) /* air busy, extent retransmissions timer */

/*=========================   TimeOutConvert   ============================================
**  Convert time out values of 9.6K system to 40K, 100k , and LR 100k system time out values.
**
**  9.6k timoute is in 1 msec resolution .
**  The time out value is in 1 ms resolution
**----------------------------------------------------------------------------------------*/
uint32_t /*RET ticks matching the the baudrate*/
TimeOutConvert(
               uint32_t timeOut) /* IN timeout matching 9600 BaudRates 1 msec steps*/
{
  uint32_t retTimeOut = timeOut;

  /* NOTE: For now 100k uses same timeout as 40k */
  if ((1 == llIsHeaderFormat3ch()) ||
      ((pFrameWaitingForACK->wRFoptions & RF_OPTION_SPEED_MASK) == RF_OPTION_SPEED_100K)   ||
      ((pFrameWaitingForACK->wRFoptions & RF_OPTION_SPEED_MASK) == RF_OPTION_SPEED_40K)    ||
      ((pFrameWaitingForACK->wRFoptions & RF_OPTION_SPEED_MASK) == RF_OPTION_SPEED_LR_100K)
      )
  {
    /*from 9.6 timeout (in 1 msec steps) to 40k  timeout (in 1ms steps) = timeout / 4
     40kb and 100 kb is 4 times faster than 9.6 */
    retTimeOut /= 4;
  }
  // Ensure that the function never returns zero
  if (0 == retTimeOut)
  {
    retTimeOut = 1;
  }
  return retTimeOut;
}


/*=========================   TransmitTimerStop   ===========================
**
**  Stop retransmit timeout timer.
**
**
**--------------------------------------------------------------------------*/
void               /*RET Nothing */
TransmitTimerStop( void ) /*IN  Nothing */
{
  TimerStop(&TransportTimer);
}


/*=========================   TransmitTimerStart   ==========================
**
**  Activate retransmit timeout timer.
**
**
**--------------------------------------------------------------------------*/
void             /*RET Nothing */
TransmitTimerStart(
  VOID_CALLBACKFUNC(func)(),  /* IN Timeout function address         */
  uint32_t timerTicks)      /* IN Timeout value (value * 1 msec.)  */
{
  TimerStop(&TransportTimer);  // Ensure old timeout isnt running when we change callback
  TimerSetCallback(&TransportTimer, func);

  TimerStart(&TransportTimer, TimeOutConvert(timerTicks));
}


#ifdef ZW_SLAVE_ROUTING
#ifdef ZW_RETURN_ROUTE_PRIORITY
/*=========================  ReturnRouteResetPriority  =======================
**    Reset priorities for all return routes
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
void
ReturnRouteResetPriority(void)
{
  uint8_t bDestIndex = 0;

  for (bDestIndex = 0; bDestIndex < (ZW_MAX_RETURN_ROUTE_DESTINATIONS + 1); bDestIndex++)
  {
    ReturnRouteClearPriority(bDestIndex);
  }
}


/*=========================  ReturnRouteClearPriority  =======================
**    Set the return route priority to default values
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
void
ReturnRouteClearPriority(
  uint8_t bDestIndex)
{
  /* Default priority route 0 = p0, route 1 = p1,... */
  abReturnRoutePriority[bDestIndex] = 0xE4;

}


/*=========================  ReturnRouteFindPriority  ========================
**    Find the index of a route with a given priority
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
uint8_t
ZCB_ReturnRouteFindPriority(
  uint8_t bPriority,
  uint8_t bRouteIndex)
{
  uint8_t bIndex = 0;
  uint8_t bPriorityRoute = 0;
  uint8_t bMask = 3;

  /* Use bIndex as temp var for priority byte*/
  /* Unified ReturnRoute and SUCReturnRoute access */
  NVM_RETURN_ROUTE_SPEED   t_ReturnRouteSpeed;
  SlaveStorageGetReturnRouteSpeed(bRouteIndex, &t_ReturnRouteSpeed);
  bPriorityRoute = (t_ReturnRouteSpeed.speed.bytes[1] & NVM_RETURN_ROUTE_PRIORITY_MASK) >> 2;
  if (bPriorityRoute)
  {
    /* There is a priority route in this return route index */
    if (0 == bPriority)
    {
      /* Update priority table so priority route is first priority */
      ReturnRouteChangePriority(bPriorityRoute-1);
      /* Always return priority route as first priority*/
      return bPriorityRoute - 1;
    }
  }
  for (; bIndex < RETURN_ROUTE_MAX; bIndex++, bMask <<= 2)
  {
    if (((abReturnRoutePriority[bRouteIndex] & bMask) >> (bIndex << 1)) == bPriority)
    {
      bReturnRoutePriorityIndex = bIndex;
      return bIndex;
    }
  }
  return 0;
}


/*========================  ReturnRouteChangePriority  =======================
**    Change the priority of the return routes so the given index is the
**    new first priority.
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
void
ReturnRouteChangePriority(
  uint8_t bRouteIndex)
{
  uint8_t bOldPriority, bTemp2;
  uint8_t bMask;
  uint8_t bInc;
  uint8_t bPriority;

  bMask = 3 << bRouteIndex;
  bOldPriority = (abReturnRoutePriority[destRouteIndex] & bMask) >> bRouteIndex;

  for (bTemp2 = 0, bInc = 1, bMask = 3; bTemp2 < RETURN_ROUTE_MAX; bTemp2++, bMask <<= 2, bInc <<= 2)
  {
    bPriority = (abReturnRoutePriority[destRouteIndex] & bMask) >> bTemp2;

    if (bPriority < bOldPriority)
    {
      abReturnRoutePriority[destRouteIndex] += bInc;
    }

    /* Set priority for the given index to 0 - Highest priority */
    abReturnRoutePriority[destRouteIndex] &= ~(3 << bRouteIndex);
  }
}
#endif  /* ZW_RETURN_ROUTE_PRIORITY */


/*=========================   ReturnRouteFindNext   ========================
**  Finds the first available return route to destination node given by
**  ZW_RouteDestinations[destRouteIndex]. The return routes to the destination
**  are searched starting from bStartRouteNo.
**
**  Prerequisites:
**    Global var destRouteIndex must contain the index for the destination.
**
**  Side effects:
**    Copies return route from NVM to returnRoute struct.
**--------------------------------------------------------------------------*/
static uint8_t
ReturnRouteFindNext(  /*RET bRoutoNo or 0xFF if not found */
  uint8_t bStartRouteNo)        /* IN Return route number to return */
{
  uint8_t index;
  uint8_t bSpeed;
  uint8_t bBeam;

  NVM_RETURN_ROUTE_STRUCT t_ReturnRoute;
  NVM_RETURN_ROUTE_SPEED   t_ReturnRouteSpeed;

  SlaveStorageGetReturnRoute(destRouteIndex, &t_ReturnRoute);
  SlaveStorageGetReturnRouteSpeed(destRouteIndex, &t_ReturnRouteSpeed);


  while (bStartRouteNo < RETURN_ROUTE_MAX)
  {
    /* Check if any route available from the EEPROM return route list */
    /* destRouteIndex = 0 is SUC */
    returnRoute.nodeID = t_ReturnRoute.nodeID;

    if (returnRoute.nodeID)
    {
#ifdef ZW_RETURN_ROUTE_PRIORITY
      memcpy(&returnRoute.repeaterList[0],
             &t_ReturnRoute.routeList[ZCB_ReturnRouteFindPriority(bStartRouteNo, destRouteIndex)],
             RETURN_ROUTE_SIZE
            );
#else
      /* TO#2584 fix - destRouteIndex is 1 based (zero is SUC routes) */
      memcpy(&returnRoute.repeaterList[0],
             &t_ReturnRoute.routeList[bStartRouteNo],
             RETURN_ROUTE_SIZE
            );
#endif  /* ZW_RETURN_ROUTE_PRIORITY */

      returnRoute.routeNoNumHops = bStartRouteNo << 4;
      /* New Routing Scheme - Direct can be anywhere */
      if (returnRoute.repeaterList[0] != CACHED_ROUTE_LINE_DIRECT)
      {
        for (index = 0; index < RETURN_ROUTE_MAX; index++)
        {
          /* TODO - deprecated; first repeater should only be ZERO for empty route entries */
          if (!returnRoute.repeaterList[index])
          {
            if (bStartRouteNo && !index)  /* Only first route can be direct! */
            {
              bStartRouteNo = 0xff;
            }
            break;
          }
          returnRoute.routeNoNumHops++;
        }
      }
      break;
    }
    else
    {
      bStartRouteNo = RETURN_ROUTE_MAX;
    }
  }

  if (bStartRouteNo >= RETURN_ROUTE_MAX)
  {
    bStartRouteNo = 0xFF;
  }
  else
  {
    /* TODO - Taken priority into account when determining route */
    bSpeed =  t_ReturnRouteSpeed.speed.bytes[0];
    /* Get speed of route */
#ifdef ZW_RETURN_ROUTE_PRIORITY
    switch ((bSpeed >> (ZCB_ReturnRouteFindPriority(bStartRouteNo, destRouteIndex) << 1)) & NVM_RETURN_ROUTE_SPEED_MASK)
#else
    switch ((bSpeed >> (bStartRouteNo << 1)) & NVM_RETURN_ROUTE_SPEED_MASK)
#endif
    {
      case NVM_RETURN_ROUTE_SPEED_200K:
        returnRoute.bReturnSpeed = ROUTE_SPEED_BAUD_200000;
        break;

      case NVM_RETURN_ROUTE_SPEED_100K:
        returnRoute.bReturnSpeed = ROUTE_SPEED_BAUD_100000;
        break;

      case NVM_RETURN_ROUTE_SPEED_40K:
        returnRoute.bReturnSpeed = ROUTE_SPEED_BAUD_40000;
        break;

      default:
      case NVM_RETURN_ROUTE_SPEED_9600:
        returnRoute.bReturnSpeed = ROUTE_SPEED_BAUD_9600;
        break;
    }
    /* TO#2670 fix. Force 40k if any BEAM info */
    /* TO#2671 fix. Force 40k if any BEAM info */
    /* TO#6762 fix. Make sure only needed bits are extracted */
    bBeam = t_ReturnRouteSpeed.speed.bytes[1] & NVM_RETURN_ROUTE_BEAM_DEST_MASK;


    if (bBeam)
    {
      /* NVM_RETURN_ROUTE_BEAM_DEST_MASK << 1 == ROUTE_SPEED_DST_WAKEUP_BEAM_MASK */
      returnRoute.bReturnSpeed = ROUTE_SPEED_BAUD_40000 | (bBeam << 1);
    }
  }
  return(bStartRouteNo);
}


/*=========================   ReturnRouteFindFirst   ========================
**
**  Search for first return route for specified bDestNode in either
**  Return Route or SUC Return Route structure
**
**
**--------------------------------------------------------------------------*/
bool
ReturnRouteFindFirst(
  uint8_t bDestNode)
{
  NVM_RETURN_ROUTE_STRUCT t_ReturnRoute;
  SlaveStorageGetReturnRoute(0, &t_ReturnRoute);
  SlaveSetSucNodeId(t_ReturnRoute.nodeID); /* [0] is SUC */
  /* We assume not found */
  destRouteIndex = 0xFF;
  activeTransmit.routeIndex = 0xFF;

  /* TO#2995 fix - First check if any SUC Return Routes exists before */
  /* testing if any Normal Return Routes exists. */
  if (g_sucNodeID == bDestNode)
  {
    /* we are transmitting to the SUC */
    destRouteIndex = 0;
    /* Unified ReturnRoute and SUCReturnRoute access */
    activeTransmit.routeIndex = ReturnRouteFindNext(0); /* find first SUC return route */
  }
  else
  {
    SlaveStorageGetReturnRoute(bDestNode, &t_ReturnRoute);
    if (t_ReturnRoute.routeList[ZCB_ReturnRouteFindPriority(0, bDestNode)].repeaterList[0]) /* [0] is SUC */
    {
    /* Do there exist a route/direct return route for this node... */
      destRouteIndex = bDestNode;
      activeTransmit.routeIndex = ReturnRouteFindNext(0);
    }
  }
  return (destRouteIndex < 0xFF);
}
#endif /* ZW_SLAVE_ROUTING */


/*===============================   TxRandom   ==============================
**  Transmit two bits pseudo-random number function
**
**
**--------------------------------------------------------------------------*/
uint8_t              /* RET Random number (0|1|2|3) */
TxRandom( void )  /* IN Nothing */
{
  uint8_t delay = zpal_get_pseudo_random();

  return (((delay >> 7)^((delay >> 5)^((delay >> 4)^delay))) & 0x03);
}


/*======================   SetTransmitSpeedOptions   =========================
**  Set the speedoptions on a TxQueue element
**
**  Side effects:
**    Also overwrites RF_OPTION1_LONG_PREAMBLE and _SEND_BEAM_* bits. Care must
**    be taken to preserve these.
**--------------------------------------------------------------------------*/
void
SetTransmitSpeedOptions(
  uint16_t speedOptions,
  TxQueueElement *pFrame)
{
  pFrame->wRFoptions = speedOptions;
}


/*======================   GetTransmitSpeedOptions   =========================
**  Get the speedoptions from the a TxQueue element
**
**  Side effects:
**    Also returns RF_OPTION1_LONG_PREAMBLE and _SEND_BEAM_* bits.
**--------------------------------------------------------------------------*/
uint16_t
GetTransmitSpeedOptions(TxQueueElement *pFrame)
{
  return(pFrame->wRFoptions);
}


/*========================   SetTransmitHomeID   ============================
**  Set HomeID in frame set transmit next
**
**--------------------------------------------------------------------------*/
void
SetTransmitHomeID(
  TxQueueElement *ptrFrame)    /* IN Frame pointer               */
{
  /* TODO - Refactoring needed */
  if (inclusionHomeIDActive)
  {
    memcpy(ptrFrame->frame.frameOptions.homeId, inclusionHomeID, HOMEID_LENGTH);
    inclusionHomeIDActive = false;
  }
  else
  {
    // If our current home is identical to second part of the DSK, then node ID (and home ID) has not been assigned.
    // Thus all frames are transmitted using first part of the DSK.
    if (g_learnModeDsk && NetworkManagement_IsSmartStartInclusionOngoing()) {
      memcpy(ptrFrame->frame.frameOptions.homeId, &g_Dsk[8], HOMEID_LENGTH);
      ZW_HomeIDSet(&g_Dsk[12]);
    } else {
#ifdef ZW_SLAVE
      /* Set homeID needed for Classic inclusion request. (This is the random homeID generated!) */
      /* Restore the stored random homeID into the read cache of homeID.*/
      /* HomeID are randomly generated and used in the inclusion process */
      /* So its possible to include one node at a time - even if several */
      /* nodes are in the state of accepting a AssignID frame */
      SlaveStorageGetNetworkIds(ZW_HomeIDGet(), NULL);
#endif
      memcpy(ptrFrame->frame.frameOptions.homeId, ZW_HomeIDGet(), HOMEID_LENGTH);
    }
  }
}


/*===============   UpdateRssiInRepeatedFrame   =============================
**
**  Fill in received RSSI in a repeated frame
**
**  Preconditions: pRxFrame must contain a RSSI_INCOMING routing header
**  extension.
**-------------------------------------------------------------------------*/
void
UpdateRssiInRepeatedFrame(
    ZW_HeaderFormatType_t headerType, /*IN incomming frame header formate*/
    frameTx *pFrame,       /* IN Pointer to the frame to be repeated */
    uint8_t rssiValue)     /* IN RSSI value to fill in */
{
  uint8_t repeaterIndex;
  repeaterIndex = GetHops(headerType, (frame *)pFrame);
  /* Find current repeater number from curHop
   * 0x0F = repeater 0, 0x00 = repeater 1 and so on...
   *
   * This relation hold for incoming frames (= R-Acks). As long as we
   * only update those this will work. Beware of updating outgoing routed
   * frames, though. */
  if (repeaterIndex == 0x0F)
  {
    repeaterIndex = 0;
  }
  else
  {
    repeaterIndex = repeaterIndex + 1;
  }
  /* repeater no.  is also the index we write to */
  SetExtendBody(headerType, pFrame, repeaterIndex, rssiValue);
}


/*================================  IncSeqNo  ===============================
**    Increment currentSeqNoTx for 3CH and Long Range frames
**
**    Side effects: currentSeqNoTx changed
**
**--------------------------------------------------------------------------*/
static uint8_t              /*RET currentSeqNoTx */
IncSeqNo(void)
{
  if (currentSeqNoUseTX)
    return currentSeqNoTx++;
  else
    return currentSeqNo;
}


/*================================  IncSeqNo_2CH  ===============================
**    Increment current9600SeqNosTX/current40kSeqNosTX and keep it
**    inside 1-15: 1 -> 2, 15 -> 1
**
**    Side effects: current9600SeqNosTX/current40kSeqNosTX changed
**
**--------------------------------------------------------------------------*/
static uint8_t              /*RET Sequence number to use in next self originated frame transmission in upper nibble */
IncSeqNo_2CH(
  bool baud40k,   /* IN true if speed is 40kbit, false if speed 9.6kbit */
  bool routed)    /* IN true if frame is routed, false if frame is direct */
{
  if (!baud40k)
  {
    if (routed)
    {
      /* Increment the routed sequence number in */
      /* current9600SeqNosTX and keep it inside 1-15: 1 -> 2, 15 -> 1 */
      if ((current9600SeqNosTX & MASK_SEQNO_ROUTED) == MASK_SEQNO_ROUTED)
      {
        current9600SeqNosTX &= MASK_SEQNO_DIRECT;
      }
      current9600SeqNosTX += 0x10;
      return (current9600SeqNosTX & MASK_SEQNO_ROUTED);
    }
    else
    {
      /* Increment the direct sequence number in */
      /* current9600SeqNosTX and keep it inside 1-15: 1 -> 2, 15 -> 1 */
      if ((current9600SeqNosTX & MASK_SEQNO_DIRECT) == MASK_SEQNO_DIRECT)
      {
        current9600SeqNosTX &= MASK_SEQNO_ROUTED;
      }
      current9600SeqNosTX++;
      return ((current9600SeqNosTX & MASK_SEQNO_DIRECT) << 4);
    }
  }
  else
  {
    if (routed)
    {
      /* Increment the routed sequence number in */
      /* current40kSeqNosTX and keep it inside 1-15: 1 -> 2, 15 -> 1 */
      if ((current40kSeqNosTX & MASK_SEQNO_ROUTED) == MASK_SEQNO_ROUTED)
      {
        current40kSeqNosTX &= MASK_SEQNO_DIRECT;
      }
      current40kSeqNosTX += 0x10;
      return (current40kSeqNosTX & MASK_SEQNO_ROUTED);
    }
    else
    {
      /* Increment the direct sequence number in */
      /* current40kSeqNosTX and keep it inside 1-15: 1 -> 2, 15 -> 1 */
      if ((current40kSeqNosTX & MASK_SEQNO_DIRECT) == MASK_SEQNO_DIRECT)
      {
        current40kSeqNosTX &= MASK_SEQNO_ROUTED;
      }
      current40kSeqNosTX++;
      return ((current40kSeqNosTX & MASK_SEQNO_DIRECT) << 4);
    }
  }
}


/*===============================   EnQueueCommon   =========================
**    Common function for data buffer transmit.
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
/* TO-DO check if this is called with anything but singlecast. IF so change txqueue refs. */
void                   /*RET Nothing      */
EnQueueCommon(
  uint8_t rfSpeed,        /* IN RF baud rate*/
  uint8_t  frameType,     /* IN Transmit header type          */
  uint8_t const * const pData,         /* IN Data buffer pointer           */
  uint8_t  dataLength,    /* IN Data buffer length            */
  const STransmitCallback* pCompletedFunc,  /* IN Transmit completed call back function  */
  TxQueueElement *pFreeTxElement)
{
  node_id_t tmpDestID;
  uint8_t  tmpSpeed = 0;
  ZW_HeaderFormatType_t curHeaderFormat =  llGetCurrentHeaderFormat(pFreeTxElement->frame.frameOptions.destinationNodeId, pFreeTxElement->forceLR);
  if (HDRFORMATTYP_LR == curHeaderFormat)
  {
    /* Don't do routing in LR demo */
    TxQueueClearOptionFlags(pFreeTxElement, TRANSMIT_OPTION_AUTO_ROUTE);
    TxQueueSetOptionFlags(pFreeTxElement, TRANSMIT_OPTION_NO_ROUTE);
  }

  /* save common transmit parameters */
  pFreeTxElement->frame.frameOptions.routed      = IsFrameRouted(curHeaderFormat, (frame *)&pFreeTxElement->frame.header);
  pFreeTxElement->frame.frameOptions.acknowledge = DoAck(curHeaderFormat, (frame *)&pFreeTxElement->frame.header);
  pFreeTxElement->frame.frameOptions.lowPower    = IsLowPower(curHeaderFormat, (frame *)&pFreeTxElement->frame.header);

  pFreeTxElement->wRFoptions = (TxQueueGetOptions(pFreeTxElement) & TRANSMIT_OPTION_LOW_POWER) ? RF_OPTION_LOW_POWER : 0;

  if (!(pFreeTxElement->bFrameOptions1 & TRANSMIT_FRAME_OPTION_NO_BUILD_TX_HDR))
  {
    if ((1 == dataLength) && (ZWAVE_CMD_NOP == *pData) && (HDRFORMATTYP_LR == curHeaderFormat))
    {
      /* Special case where an application sends a legacy NOP frame to a Long Range node */
      /* Be nice and replace it with a Long Range NOP */
      NOP_FRAME_LR nopFrameLR;
      nopFrameLR.cmdClass = ZWAVE_CMD_CLASS_PROTOCOL_LR;
      nopFrameLR.cmd      = ZWAVE_LR_CMD_NOP;
      dataLength          = sizeof(NOP_FRAME_LR);
      memcpy(pFreeTxElement->frame.payload, &nopFrameLR, dataLength);
    }
    else
    {
      /* Normal case. Copy the data to the TX payload buffer */
      memcpy(pFreeTxElement->frame.payload, pData, dataLength);
    }
  }
  pFreeTxElement->frame.payloadLength = dataLength;
  pFreeTxElement->AppCallback = *pCompletedFunc;
  pFreeTxElement->zcbp_InternalCallback = ZCB_ProtocolTransmitComplete;

  if ((HDRFORMATTYP_3CH == curHeaderFormat) || (HDRFORMATTYP_LR == curHeaderFormat))
  {
    tmpDestID = pFreeTxElement->frame.frameOptions.destinationNodeId;
#ifdef ZW_CONTROLLER
    if (0 == rfSpeed)
    {
      pFreeTxElement->wRFoptions = ChooseSpeedBeamForDestination_Common(tmpDestID, pFreeTxElement, frameType);
    }
    else
    {
      pFreeTxElement->wRFoptions = rfSpeed;
    }
#endif

#ifdef ZW_SLAVE
    pFreeTxElement->wRFoptions = rfSpeed;
#endif
  }
  else
  {
    tmpSpeed = rfSpeed;
    /* TODO - Are there any other frametype which we should watch out for */
    /* TO#1529 fix and the partial derived TO#2402 fix - Multicast is never send with 100kb even if all destination nodes supports 100kb */
    if ((frameType & MASK_FRAMETYPE_FRAMETYPE) != FRAME_TYPE_MULTI)
    {
      tmpDestID = pFreeTxElement->frame.frameOptions.destinationNodeId;

      /* If the given speed is not LR and the destination is broadcast, then we hardcode the speed
       * to 9600. */
      if ((RF_SPEED_LR_100K != rfSpeed) && (0xFF == (uint8_t)tmpDestID))
      {
        SET_RF_OPTION_SPEED(tmpSpeed, RF_OPTION_SPEED_9600);
      }
#ifdef ZW_SLAVE
      else if ((tmpSpeed & RF_OPTION_SPEED_MASK) == RF_OPTION_SPEED_NONE) /* if the value of the rfSpeed is false use currentSpeed for this node */
      {
        if (tmpDestID == gNodeLastReceivedFrom)
        {
          SET_RF_OPTION_SPEED(tmpSpeed, mTransportRxCurrentSpeed);
        }
        else
        {
          SET_RF_OPTION_SPEED(tmpSpeed, RF_OPTION_SPEED_9600);
        }

      }
#endif /* ZW_SLAVE */
#ifdef ZW_CONTROLLER
      else if (((frameType & MASK_FRAMETYPE_FRAMETYPE) == FRAME_TYPE_ROUTED) ||
               ((frameType & MASK_FRAMETYPE_FRAMETYPE) == FRAME_TYPE_SINGLE))
      {
        /* use the speed supported by the node if it is unknown or we did not request specific speed */
        /* also if the source and dest node ID are the same or the dest is 0xEF */
        if ((rfSpeed & RF_OPTION_SPEED_MASK) == RF_OPTION_SPEED_NONE)
        {
          tmpSpeed = ChooseSpeedBeamForDestination_Common(tmpDestID, pFreeTxElement, frameType);
        }
      }
#endif  /* ZW_CONTROLLER */
    }
  }

  /* Preamble length is always RF_PREAMP_SHORT(10) bytes at the first try, */
  /* except when transmitting MultiCast at 40k where we use RF_PREAMP_LONG(20) bytes */

  pFreeTxElement->wRFoptions &= ~RF_OPTION_LONG_PREAMBLE;

#ifdef ZW_CONTROLLER
  if ((frameType & MASK_FRAMETYPE_FRAMETYPE) == FRAME_TYPE_MULTI)
  {
    pFreeTxElement->wRFoptions |= RF_OPTION_LONG_PREAMBLE;
  }
#endif  /* ZW_CONTROLLER */
   uint8_t tSeqNo = 0;
  if (HDRFORMATTYP_2CH != curHeaderFormat)
  {
#ifdef ZW_CONTROLLER
    /* Restart routing diversity */
    if (TxQueueIsEmpty())
      abLastStartNode[0] = 0;
#endif
    tSeqNo = IncSeqNo();
    /* Default value is true as we only need it to be false when repeating */
    /* a frame or sending a routed ACK/ERR frame */
    currentSeqNoUseTX = true;
  }
  else
  {
    if (!(pFreeTxElement->bFrameOptions1 & TRANSMIT_FRAME_OPTION_NO_BUILD_TX_HDR))
    {
      bool bIsRouted = ((frameType & MASK_FRAMETYPE_FRAMETYPE) == FRAME_TYPE_ROUTED) || ((frameType & MASK_FRAMETYPE_FRAMETYPE) == FRAME_TYPE_EXPLORE);

      if (currentSeqNoUseTX)
      {
        /* Increment currentSeqNoTX and keep it inside 1-15: 1 -> 2, 15 -> 1 */
        frameType |= IncSeqNo_2CH( (tmpSpeed & RF_OPTION_SPEED_MASK) == RF_OPTION_SPEED_40K, bIsRouted);
      }
      else
      {
        frameType |= ((currentSeqNo & MASK_CURRENTSEQNO_SEQNO) << 4);
      }

      /* Default value is true as we only need it to be false when repeating */
      /* a frame or sending a routed ACK/ERR frame */
      currentSeqNoUseTX = true;
      /* set frame type */
      /* TO-DO what about upper nibble? Some SeqNo info seems to be stored there. SET_HEADERTYPE sets all 8 bits */
      pFreeTxElement->frame.frameOptions.frameType = frameType;
      /* Clearing upper nibble just in case */
      //pFreeTxElement->sFrameHeader.header.headerInfo &= ~MASK_FRAMETYPE_SEQNO;
    } /* if (!TRANSMIT_FRAME_OPTION_NO_BUILD_TX_HDR) */


    /* TO-DO What if a queue element is allocated, but not ready to send? IsEmpty() will return false */
    if (TxQueueIsEmpty())
    {
#ifdef ZW_ROUTE_DIVERSITY
      abLastStartNode[0] = 0;
#endif  /* ZW_ROUTE_DIVERSITY */
    }
    pFreeTxElement->wRFoptions = tmpSpeed;
  }
  if (!(pFreeTxElement->bFrameOptions1 & TRANSMIT_FRAME_OPTION_NO_BUILD_TX_HDR))
  {
    BuildTxHeader(curHeaderFormat, frameType, tSeqNo, pFreeTxElement);
#ifdef ZW_SLAVE
    if ((HDRFORMATTYP_LR == curHeaderFormat) &&
        (0 != (pFreeTxElement->bFrameOptions & TRANSMIT_OPTION_ACK)) &&
        (zpal_radio_get_lr_channel_config() == ZPAL_RADIO_LR_CH_CFG3) )
    {
      /* 1 transmission on primary channel + 2 retransmissions on primary channel + 1 on secondary channel */
      pFreeTxElement->bCallersStatus++;
    }
#endif
  }

  /* activate transmission */
  /* Save last RFoption in case they are modified by TqQueue (NOBEAM) */
  pFreeTxElement->wRFoptionsBeforeModify = pFreeTxElement->wRFoptions;
  TxQueueQueueElement(pFreeTxElement);
}

/*========================   TransportEnQueueExploreFrame   ==================
**    Transmit data buffer via a Z-Wave Explore frame to destination.
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
uint8_t                                     /* RET false if transmitter busy     */
TransportEnQueueExploreFrame(
  frameExploreStruct      *pFrame,           /* IN Frame pointer                 */
  TxOptions_t             exploreTxOptions, /* IN Transmit option flags        */
  ZW_SendData_Callback_t  completedFunc)    /* IN Transmit completed call back function  */
{
  uint8_t bSequenceNumber = 0;
  uint8_t frameType = FRAME_TYPE_EXPLORE;
  TxQueueElement *pFreeTxElement;
  // Explore only used on HDRFORMATTYP_3CH or HDRFORMATTYP_2CH type channels - No Explore frame on LR channel
  ZW_HeaderFormatType_t curHeaderFormat = llIsHeaderFormat3ch()?HDRFORMATTYP_3CH:HDRFORMATTYP_2CH;
  DPRINTF("EnqueueExplore %02X\n", pFrame->baseFrame.payloadLength);
  if (((HDRFORMATTYP_3CH == curHeaderFormat) && (MAX_FRAME_LEN > pFrame->baseFrame.payloadLength))
      ||
    /* 2 channel - Explore frame is always transmitted at 40K and therefore a frame can only be 64 bytes */
      ((HDRFORMATTYP_2CH == curHeaderFormat) && (RX_MAX_LEGACY > pFrame->baseFrame.payloadLength)))
  {
    pFreeTxElement = TxQueueGetFreeElement(TX_QUEUE_PRIORITY_LOW, false);
    if (pFreeTxElement)
    {
      if (currentSeqNoUseTX)
      {
        if (HDRFORMATTYP_3CH == curHeaderFormat)
        {
          bSequenceNumber = IncSeqNo();
        }
        else
        {
          /* We use the routed 40kb seqno for exploreframes */
          frameType |= IncSeqNo_2CH(RF_SPEED_40K, true);
        }
      }
      else
      {
        if (HDRFORMATTYP_3CH == curHeaderFormat)
        {
          bSequenceNumber = pFrame->baseFrame.frameOptions.sequenceNumber;//GET_SEQNUMBER(*(frameHeaderSinglecast*)pData);
        }
        else
        {
          /* Get sequence number from the frame to be sent */
          frameType |= pFrame->baseFrame.frameOptions.sequenceNumber << 4;
        }
      }
      /* pData points at the full Explore Frame plus piggybacked payload
          skip header in memcpy */
      memcpy(&pFreeTxElement->frame, &(pFrame->baseFrame), sizeof(pFrame->baseFrame) + sizeof(pFrame->explorerPayload));

      // Note: the pFreeTxElement->frame.frame_options field should be set before the funcion is called
      TxQueueSetOptionFlags(pFreeTxElement, exploreTxOptions);
      pFreeTxElement->wRFoptions = RF_SPEED_40K;
      pFreeTxElement->frame.headerLength = pFrame->baseFrame.headerLength;
      /* Set internal callback func so txqueue element gets released */
      pFreeTxElement->zcbp_InternalCallback = ZCB_ProtocolTransmitComplete;
      /* The app callback func is invoked by the internal callback func */
      const STransmitCallback TxCallback = { .pCallback = completedFunc, .Context = 0 };
      pFreeTxElement->AppCallback = TxCallback;
      /* Reset to default. */
      currentSeqNoUseTX = true;
      BuildTxHeader(curHeaderFormat, frameType, bSequenceNumber, pFreeTxElement);
      /* if frame is an explore and not a search result then reduce Tx power */
      frameHeaderExplore *exploreHeader = (frameHeaderExplore *)pFreeTxElement->frame.payload; // Explore header starts at payload buffer offset 0
      if (((HDRTYP_EXPLORE == (pFreeTxElement->frame.header.header.headerInfo & MASK_HDRTYP)) &&
          (0 == (exploreHeader->ver_Cmd & EXPLORE_CMD_SEARCH_RESULT))))
      {
        pFreeTxElement->frame.txPower = ZPAL_RADIO_TX_POWER_REDUCED;
      }
      else
      {
        pFreeTxElement->frame.txPower = ZPAL_RADIO_TX_POWER_DEFAULT;
      }

      /* Queue it */
      TxQueueQueueElement(pFreeTxElement);
      return (true);
    }
  }
  /* Reset to default */
  currentSeqNoUseTX = true;
  return (false);
}


#ifdef ZW_SLAVE
/*=========================   ReturnRouteGetSpeed   ==========================
**    Take a Return route bReturnSpeed field and convert to RF_OPTION_*
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
uint8_t            /*RET Return route speed and beaming encoded as RF_OPTION_* */
ReturnRouteGetSpeed(
  uint8_t bReturnSpeed)
{
  uint8_t beamInfo = 0;

  /* Swap beam bits in bReturnSpeed to match RF_OPTION_* */
  if (bReturnSpeed & RETURN_SPEED_DST_WAKEUP_BEAM_1000)
  {
    beamInfo |= RF_OPTION_SEND_BEAM_1000MS;
  }
  else if (bReturnSpeed & RETURN_SPEED_DST_WAKEUP_BEAM_250)
  {
    beamInfo |= RF_OPTION_SEND_BEAM_250MS;
  }
  /* TO#3324 fix - 3 Channel only do 100k, so always just OR BEAM bits, */
  /* they should be none existing if no BEAM is to be done... */
  if (1 == llIsHeaderFormat3ch())
  {
    return RF_OPTION_SPEED_100K | beamInfo;
  }
  else
  {
    switch (bReturnSpeed & ROUTE_SPEED_BAUD_RATE_MASK)
    {
      case ROUTE_SPEED_BAUD_100000:
        return RF_OPTION_SPEED_100K;

      case ROUTE_SPEED_BAUD_40000:
        return RF_OPTION_SPEED_40K | beamInfo;

      default:
      case ROUTE_SPEED_BAUD_9600:
        return RF_OPTION_SPEED_9600;
    }
  }
}
#endif /* ZW_SLAVE */


#ifdef MULTIPLE_LWR
uint8_t
DetermineCachedRouteNextID(
  uint8_t bRoutingScheme)
{
  /* Return ZERO if no more Cached Route types exists */
  uint8_t bNextLWRno = 0;
  if (ZW_ROUTINGSCHEME_CACHED_ROUTE_SR == bRoutingScheme)
  {
    /* Application Static Route Failed - try any ZW LWR */
    bNextLWRno = CACHED_ROUTE_ZW_ANY;
  }
  else
  {
    if (ZW_ROUTINGSCHEME_CACHED_ROUTE == bRoutingScheme)
    {
      /* ZW LWR Failed - try Next to Last Working Route (if any) */
      bNextLWRno = CACHED_ROUTE_ZW_NLWR;
    }
  }
  return bNextLWRno;
}


uint8_t
DetermineCachedRouteRoutingScheme(void)
{
  /* Return ZW_ROUTINGSCHEME_CACHED_ROUTE if no match */
  uint8_t bCurrentRoutingScheme = ZW_ROUTINGSCHEME_CACHED_ROUTE;

  if (CACHED_ROUTE_APP_SR == bCurrentLWRNo)
  {
    bCurrentRoutingScheme = ZW_ROUTINGSCHEME_CACHED_ROUTE_SR;
  }
  else
  {
    if (CACHED_ROUTE_ZW_NLWR == bCurrentLWRNo)
    {
      bCurrentRoutingScheme = ZW_ROUTINGSCHEME_CACHED_ROUTE_NLWR;
    }
  }
  return bCurrentRoutingScheme;
}
#endif


/*==========================   CleanUpAfterFailedTxElemAlloc   =====================
**    Cleanup of global state to be run after allocation of a TxQueue element
**    fails. This looks ugly, but I dont have a better solution, nor am I
**    sure they are not neede. So I am keeping them.
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
void CleanUpAfterFailedTxElemAlloc(void)
{
   /* Make sure we do not do explore frame unintended */
   bUseExploreAsRouteResolution = false;
 /* Could not enqueue frame so reset currentSeqNoUseTX to default setting */
 /* Default value is true as we only need it to be false when repeating */
 /* a frame or sending a routed ACK/ERR frame */
 currentSeqNoUseTX = true;
}

bool
EnQueueSingleDataOnLRChannels(
  uint8_t    rfSpeed,
  node_id_t  srcNodeID,
  node_id_t  destNodeID,
  uint8_t   *pData,
  uint8_t    dataLength,
  uint32_t   delayedTxMs,
  uint8_t    txPower,
  const STransmitCallback* pCompletedFunc)
{
    bool status;
#ifdef ZW_CONTROLLER_TEST_LIB
/*The test lib will only send on the LR primary channel*/
    status  = EnQueueSingleData(rfSpeed,
                                srcNodeID,
                                destNodeID,
                                pData,
                                dataLength,
                                TRANSMIT_OPTION_LR_FORCE,  // Transmit only on the PRIMARY LR Channel.
                                delayedTxMs,
                                txPower,
                                pCompletedFunc);

#else
    STransmitCallback pCompletedFunc_tmp = { .pCallback = NULL, .Context = 0 };
    status  = EnQueueSingleData(rfSpeed,
                                srcNodeID,
                                destNodeID,
                                pData,
                                dataLength,
                                TRANSMIT_OPTION_LR_FORCE,  // Transmit only on the PRIMARY LR Channel.
                                delayedTxMs,
                                txPower,
                                &pCompletedFunc_tmp);

    status &= EnQueueSingleData(rfSpeed,
                                srcNodeID,
                                destNodeID,
                                pData,
                                dataLength,
                                TRANSMIT_OPTION_LR_FORCE | TRANSMIT_OPTION_LR_SECONDARY_CH,  // Transmit only on SECONDARY LR Channel. LR_FORCE only forces LR TX.
                                delayedTxMs,
                                txPower,
                                pCompletedFunc);
#endif
    return status;
}

bool
EnQueueSingleData(
  uint8_t                   rfSpeed,
  node_id_t                 srcNodeID,
  node_id_t                 destNodeID,
  const uint8_t            *pData,
  uint8_t                   dataLength,
  TxOptions_t               txOptions,
  uint32_t                  delayedTxMs,
  uint8_t                   txPower,
  const STransmitCallback*  pCompletedFunc)
{
  TxQueueElement *pFreeTxElement;

  pFreeTxElement = TxQueueGetFreeElement(TX_QUEUE_PRIORITY_LOW, txOptions & TRANSMIT_OPTION_DELAYED_TX);
  if (pFreeTxElement)
  {
    pFreeTxElement->delayedTx.timeType = TIME_TYPE_RELATIVE;  // The time being inserted is relative to now!
    pFreeTxElement->delayedTx.delayedTxMs = delayedTxMs;

    pFreeTxElement->frame.txPower = txPower;
    pFreeTxElement->StartTicks = getTickTime();
    pFreeTxElement->bTransmitRouteCount = 0;
    TxQueueInitOptions(pFreeTxElement, txOptions);

    if (0 != EnqueueSingleDataPtr(rfSpeed, srcNodeID, destNodeID, pData, dataLength,
                                  pCompletedFunc, pFreeTxElement))
    {
      return true;
    }
#ifndef USE_TRANSPORT_SERVICE
    TxQueueReleaseElement(pFreeTxElement); /* Something in EnQueueSingleDataPtr failed, free
                                              * TxQueueElement and abort */
#endif
  }
  CleanUpAfterFailedTxElemAlloc();

  return false;
}


#ifdef USE_TRANSPORT_SERVICE
static uint8_t engage_transport_service = 0;

bool
engageTransportServicePossible(
  uint8_t frameOptions)
{
  if (frameOptions & TRANSMIT_FRAME_OPTION_TRANSPORT_SERVICE)
  {
    engage_transport_service = 1;
    return true;
  }
  return false;
}
#endif


#define RF_SPEED_AUTO     0x0000


bool
TransportIsPayloadLengthToBig(
  uint8_t                speed,
  node_id_t              destNodeID,
  TxOptions_t            txOptions,
  __attribute__((unused)) uint8_t                frameOptions,
  uint8_t                dataLength,
  ZW_HeaderFormatType_t  curHeaderFormat)
{
#ifdef USE_TRANSPORT_SERVICE
  engage_transport_service = 0;
#endif


  if (RF_SPEED_LR_100K == speed)
  {
    if (dataLength > MAX_SINGLECAST_PAYLOAD_LR)
    {
      return true;
    }
    else
    {
      return false;
    }
  }

  /* Make sure the payload length matches the current specification. */
  // 3 Channel REGIONs
  /* Frame transmitted with TRANSMIT_OPTION_NO_ROUTE, max payload is MAX_SINGLECAST_PAYLOAD_3CH (158). */
  /* Frame transmitted with TRANSMIT_OPTION_EXPLORE, max payload is MAX_EXPLORE_PAYLOAD_3CH (150) */
  /* Frame transmitted without TRANSMIT_OPTION_NO_ROUTE, max payload is MAX_ROUTED_PAYLOAD_3CH (151). */
  /* as the frame could be transmitted using Response Route/Last Working Route. */
  // 2 Channel REGIONs
  /* Frame transmitted BROADCAST, max payload is MAX_SINGLECAST_PAYLOAD_LEGACY (54), */
  /* as BROADCAST is allways transmitted at 9600. */
  /* Frame transmitted with TRANSMIT_OPTION_NO_ROUTE, max payload at 100kb is MAX_SINGLECAST_PAYLOAD (159). */
  /* Frame transmitted with TRANSMIT_OPTION_NO_ROUTE, max payload at 9.6/40kb is MAX_SINGLECAST_PAYLOAD_LEGACY (54). */
  /* Frame transmitted with TRANSMIT_OPTION_EXPLORE, max payload at 40kb is MAX_EXPLORE_PAYLOAD_LEGACY (46), 2 channel explore is always 40kb. */
  /* Frame transmitted without TRANSMIT_OPTION_NO_ROUTE, max payload at 100kb is MAX_ROUTED_PAYLOAD (153 none FLiRS or 151 for FLiRS destination). */
  /* Frame transmitted without TRANSMIT_OPTION_NO_ROUTE, max payload at 9.6/40kb is MAX_ROUTED_PAYLOAD_LEGACY (48 none FLiRS or 46 for FLiRS destination). */
  /* as the frame could be transmitted using Response Route/Last Working Route. */
  /* The none LEGACY max payloads are of course only effective when transmitting with 100kb. */
  if ((RF_SPEED_AUTO == speed) || (RF_SPEED_100K == speed) || (HDRFORMATTYP_3CH == curHeaderFormat))
  {
    if (dataLength > MAX_SINGLECAST_PAYLOAD)
    {
      return true;
    }
    else if ((HDRFORMATTYP_3CH != curHeaderFormat)
            && ((destNodeID == NODE_BROADCAST) || ((RF_SPEED_LR_100K == speed) && (destNodeID == NODE_BROADCAST_LR)))
            && (dataLength > MAX_SINGLECAST_PAYLOAD_LEGACY))
    {
      // TODO - Long Range max frame sizes must be determined
      /* 2Ch - Broadcast is always transmitted with 9600 and therefore max total framelength is 64 byte (legacy). */
      return true;
    }
    // For 2 Channel Explore frame is always transmitted on 40kb and therefore MAX 64 byte frame (legacy) are in effect
    else if (bUseExploreAsRouteResolution && (dataLength > MAX_EXPLORE_PAYLOAD_LEGACY))
    {
      // For 3 Channel MAX total frame size is 170 byte
      if  ((HDRFORMATTYP_3CH != curHeaderFormat) || (dataLength > MAX_EXPLORE_PAYLOAD_3CH))
      {
#ifdef USE_TRANSPORT_SERVICE
        return engageTransportServicePossible(frameOptions) ? false : true;
#else /* #ifdef USE_TRANSPORT_SERVICE */
        return true;
#endif /* #ifdef USE_TRANSPORT_SERVICE */
      }
    }
#ifndef USE_TRANSPORT_SERVICE
    if (((txOptions & TRANSMIT_OPTION_NO_ROUTE) == 0) && (((HDRFORMATTYP_3CH == curHeaderFormat) && (dataLength > MAX_ROUTED_PAYLOAD_3CH)) ||
                                                          ((HDRFORMATTYP_3CH != curHeaderFormat) &&
                                                           ((dataLength > MAX_ROUTED_PAYLOAD)
#ifdef ZW_CONTROLLER
               || (IsNodeSensor(destNodeID, false, false) && (dataLength > MAX_ROUTED_PAYLOAD_FLIRS))
#endif
                                                           ))))
    {
      /* If TRANSMIT_OPTION_NO_ROUTE is NOT set then max payload is max MAX_ROUTED_PAYLOAD as a routed */
      /* Response Route/Last Working Route could exist. */
      return true;
    }
#endif /*#ifndef USE_TRANSPORT_SERVICE */
  }
  else if ((HDRFORMATTYP_3CH != curHeaderFormat) && (RF_SPEED_40K >= speed))
  {
    if (dataLength > MAX_SINGLECAST_PAYLOAD)
    {
      return true;
    }
    if (((destNodeID == NODE_BROADCAST) || ((RF_SPEED_LR_100K == speed) && (destNodeID == NODE_BROADCAST_LR)))
       && (dataLength > MAX_SINGLECAST_PAYLOAD_LEGACY))
    {
      return true;
    }
    else if (((0 != (txOptions & TRANSMIT_OPTION_NO_ROUTE)) && (dataLength > MAX_SINGLECAST_PAYLOAD_LEGACY)) ||
             (!bUseExploreAsRouteResolution && (0 != (txOptions & TRANSMIT_OPTION_AUTO_ROUTE)) &&
              ((dataLength > MAX_ROUTED_PAYLOAD_LEGACY)
#ifdef ZW_CONTROLLER
               || (IsNodeSensor(destNodeID, false, false) && (dataLength > MAX_ROUTED_PAYLOAD_FLIRS_LEGACY))
#endif
              )) ||
             ((bUseExploreAsRouteResolution && (dataLength > MAX_EXPLORE_PAYLOAD_LEGACY))))
    {
      /*
       * Transmission is at 9.6/40Kb so the 64 byte MAX frame size is active
       */
#ifdef USE_TRANSPORT_SERVICE
        return engageTransportServicePossible(frameOptions) ? false : true;
#else /* #ifdef USE_TRANSPORT_SERVICE */
        return true;
#endif /* #ifdef USE_TRANSPORT_SERVICE */
    }
  }
  else
  {
    return true;
  }
  return false;
}


/*===============================   EnqueueSingleDataPtr   =====================
**    Transmit data buffer to a single Z-Wave node or all Z-Wave nodes (broadcast).
**    Takes a ptr to TxQueue element, allowing more control of txOptions
**    from calling functions.
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
static uint8_t                              /* RET false if transmitter busy */
EnqueueSingleDataPtr(
  uint8_t                  rfSpeed,         /* IN RF baud rate*/
  node_id_t                srcNodeID,       /* IN Source node ID */
  node_id_t                destNodeID,      /* IN Destination node ID, if equal to 0xFF then all nodes */
  uint8_t const          * const pData,     /* IN Data buffer pointer */
  uint8_t                  dataLength,      /* IN Data buffer length */
  const STransmitCallback *pCompletedFunc,  /* TODO: This argument can be optimized out */
  TxQueueElement          *pFreeTxElement)  /* IN  Transmit completed call back function */
{
#ifdef ZW_CONTROLLER
  uint8_t bRouteSpeed;
#endif
  uint8_t frameType = FRAME_TYPE_SINGLE;
#if defined(ZW_SLAVE_ROUTING) || defined(ZW_CONTROLLER_STATIC)
  uint8_t bRepeaterCountInRoute = 0;
#endif

  uint8_t speed;
  uint8_t* freeTxElement_numRepsNumHops;
#ifdef ZW_CONTROLLER_TEST_LIB
  if ((TxQueueGetOptions(pFreeTxElement) & TRANSMIT_OPTION_LR_FORCE) ||
      ((srcNodeID == g_nodeID) && (LOWEST_LONG_RANGE_NODE_ID <= srcNodeID) && (NODE_CONTROLLER == destNodeID))
     )
#else
  if (TxQueueGetOptions(pFreeTxElement) & TRANSMIT_OPTION_LR_FORCE)
#endif
  {
    pFreeTxElement->forceLR = true;
  }

  uint8_t curHeaderFormat = llGetCurrentHeaderFormat(destNodeID, pFreeTxElement->forceLR);
  if (HDRFORMATTYP_LR == curHeaderFormat)
  {
    /* No routing on LR channel */
    TxQueueClearOptionFlags(pFreeTxElement, TRANSMIT_OPTION_AUTO_ROUTE);
    TxQueueSetOptionFlags(pFreeTxElement, TRANSMIT_OPTION_NO_ROUTE);
    bUseExploreAsRouteResolution = false;
#if defined(ZW_CONTROLLER)
    // If destination node is a sensor(FLiRS) the setting of speed is handled at a later stage
    if (0 == IsNodeSensor(destNodeID, true, true))
#endif	// #if defined(ZW_CONTROLLER)
    {
      // Speed always LR_100K on LR
      rfSpeed = RF_SPEED_LR_100K;
    }
  }
  speed = rfSpeed;
  if (HDRFORMATTYP_3CH != curHeaderFormat)
  {
    freeTxElement_numRepsNumHops = &pFreeTxElement->frame.header.singlecastRouted.numRepsNumHops;
  }
  else
  {
    freeTxElement_numRepsNumHops = &pFreeTxElement->frame.header.singlecastRouted3ch.numRepsNumHops;
  }

  if (TxQueueGetOptions(pFreeTxElement) & TRANSMIT_OPTION_FOLLOWUP)
  {
    pFreeTxElement->bFrameOptions1 |= TRANSMIT_FRAME_OPTION_FOLLOWUP;
  }

  /* Check payload size */
  /* Only check size if TRANSMIT_OPTION_ROUTED is not set
     If TRANSMIT_OPTION_ROUTED is set then the frame is repeated or
     created by the protocol */
  if (!(TxQueueGetOptions(pFreeTxElement) & (TRANSMIT_OPTION_ROUTED | TRANSMIT_OPTION_EXPLORE_REPEAT)))
  {
    if (0 != TransportIsPayloadLengthToBig(speed & RF_OPTION_SPEED_MASK, destNodeID, TxQueueGetOptions(pFreeTxElement), pFreeTxElement->bFrameOptions1, dataLength, curHeaderFormat)) // to be checked FIXME-LR
    {
#ifdef USE_TRANSPORT_SERVICE
      goto clean_up_and_exit;
#else
      return false;
#endif
    }
#ifdef USE_TRANSPORT_SERVICE
    if (engage_transport_service)
    {
      /* only engage transport service if requested by user */
      if (0 == (pFreeTxElement->bFrameOptions1 & TRANSMIT_FRAME_OPTION_TRANSPORT_SERVICE))
      {
        goto clean_up_and_exit;
      }
      ts_param_t proto_ts_param = {
        .snode = srcNodeID,
        .dnode = destNodeID

      };

      /* TRANSMIT_OPTION_APPLICATION and TRANSMIT_OPTION_EXPLORE use the same bit in txOptions.
       * Transport Service should not know about the former, so we clear that. And set it again
       * as TRANSMIT_OPTION_EXPLORE if bUseExploreAsResolution is set. */
      TxQueueClearOptionFlags(pFreeTxElement,TRANSMIT_OPTION_APPLICATION);
      proto_ts_param.tx_flags =
          (bUseExploreAsRouteResolution ? (TxQueueGetOptions(pFreeTxElement) | TRANSMIT_OPTION_EXPLORE) : TxQueueGetOptions(pFreeTxElement));
      ts_stored_callback = *pCompletedFunc;
      /* Free already allocated TxQueue element.
       * Transport Service will allocate anew if needed. */
      TxQueueReleaseElement(pFreeTxElement);
      bUseExploreAsRouteResolution = false;

      /* We need to copy the data for transport service. pData is a pointer to the m_TxQueue in ZW_protocol_interface,
       * which has been cleared when we are ready to send the subsequent Transport Service fragments. */
      /* Only update buffer if Transport Service is ready to accept a new send operation */
      if (!ZW_TransportService_Is_Sending())
      {
        /* Cap data size to buffer size */
        if (dataLength > sizeof (m_transport_service_buffer)) {
          dataLength = sizeof (m_transport_service_buffer);
        }
        memcpy(m_transport_service_buffer, pData, dataLength);
      }

      if (!ZW_TransportService_SendData(&proto_ts_param, m_transport_service_buffer, (uint16_t)dataLength, ZCB_TsCallbackFunc))
      {
        ts_stored_callback.pCallback = NULL;
        ts_stored_callback.Context = NULL;
        return false;
      }

      return true;
    }
#endif /*#ifdef USE_TRANSPORT_SERVICE */
  }
  DPRINT("=");

  /* TO#2550 fix - To allow for support of old ZW_SendData parameter - macro ZW_SendData uses 0xff as src. */
#ifdef ZW_CONTROLLER_BRIDGE
  /* TO#5557 fix - Only change srcNodeID if asked for it... */
  if ((srcNodeID == 0xFF)
      && !(pFreeTxElement->bFrameOptions1 & TRANSMIT_FRAME_OPTION_NO_BUILD_TX_HDR))
  {
    /* If srcNodeID is not a VirtualNode then it can only be nodeID - no spoofing allowed. */
    srcNodeID = g_nodeID;
  }
#endif

  /* save frame information into the transmit queue */
  pFreeTxElement->frame.frameOptions.sourceNodeId = srcNodeID;
  pFreeTxElement->frame.frameOptions.destinationNodeId = destNodeID;
  /* Set routeType initially to IDLE */
  pFreeTxElement->bRouteSchemeState = ZW_ROUTINGSCHEME_IDLE;
  // LR node will not enter here, correct? Maybe yes, FIXME-LR
  if (bUseExploreAsRouteResolution)
  {
    bUseExploreAsRouteResolution = false;
#ifdef ZW_CONTROLLER
    /* TO#1998 fix */
    /* If destNodeID either is BROADCAST or does not exist in the controller then use explore as requested. */
    if (((destNodeID == NODE_BROADCAST) || !ZCB_GetNodeType(destNodeID))
        /* If destNodeID is based on a library prior to 4.5x then do not use explore frame even if requested. */
        || (((GetNodeCapabilities(destNodeID) & ZWAVE_NODEINFO_PROTOCOL_VERSION_MASK) >= ZWAVE_NODEINFO_VERSION_4)

            /* TODO - Routing Slaves can (if Return Routes do exist for destination node) */
            /* use Return Route information to disallow Explore resolution if destination is a FLiRS node */

            /* TO#1999 fix */
            /* If destNodeID is a FLiRS node then do not use explore frame even if requested. */
            && (!IsNodeSensor(destNodeID, false, false))
       ))
#endif  /* ZW_CONTROLLER */
    {
      /* Mark frame to Use explore frame as fallback if everything else fails */
      pFreeTxElement->bFrameOptions1 |= TRANSMIT_FRAME_OPTION_EXPLORE;
    }
  }
  else
  {
    if (TxQueueGetOptions(pFreeTxElement) & TRANSMIT_OPTION_EXPLORE_REPEAT)
    {
      frameType = FRAME_TYPE_ROUTED;
      /* We are sending explore - set RouteSchemeState to RESORT_EXPLORE */
      pFreeTxElement->bRouteSchemeState = ZW_ROUTINGSCHEME_RESORT_EXPLORE;
    }
  }
  /* New ROUTING SCHEME */

  /* First Test if ResponseRoute/LastWorkingRoute is applicable */
  /* If not already routed. Check if we have a route to use */
  if (((TxQueueGetOptions(pFreeTxElement) & (TRANSMIT_OPTION_ROUTED | TRANSMIT_OPTION_NO_ROUTE)) == 0) &&
      (frameType == FRAME_TYPE_SINGLE))
  {
    /* Check if we have a response route and use it */
    /* TO#1228 - Only Portable Controllers should not use response routes here, */
    /* all other should use response routes */
#ifdef USE_RESPONSEROUTE

#if defined(ZW_SLAVE) || defined(ZW_CONTROLLER_STATIC)
    if (GetResponseRoute(destNodeID, pFreeTxElement))
    {
      /* Set NOBEAM flag if first transmission to a FLiRS node */
      if (pFreeTxElement->wRFoptions & RF_OPTION_BEAM_MASK)
      {
        pFreeTxElement->bFrameOptions1 |= TRANSMIT_FRAME_OPTION_NOBEAM;
      }

      /* Routing scheme update */
      if (*freeTxElement_numRepsNumHops & MASK_ROUT_REPTS)
      {
        /* Found cached route has repeaters - therefor a FRAME_ROUTED type */
        frameType = FRAME_TYPE_ROUTED;
      }
      /* TO#1627 fix - ZW_RequestNodeNeighbor fails if questioned node is routed and first routing attempt fails. */
      /* Indicate CACHED ROUTE */
      /* Routing scheme update */
      pFreeTxElement->bRouteSchemeState = ZW_ROUTINGSCHEME_CACHED_ROUTE;
      /* TO#2758, TO#2820 fix - Set speed and BEAM bits according to found Response Route */
      speed &= ~(RF_OPTION_SPEED_MASK | RF_OPTION_BEAM_MASK);
      speed |= pFreeTxElement->wRFoptions & (RF_OPTION_SPEED_MASK | RF_OPTION_BEAM_MASK);
    }
#else
    /* none static controllers always tries direct before routing */
    pFreeTxElement->bRouteSchemeState = ZW_ROUTINGSCHEME_DIRECT;
#endif  /* defined(ZW_SLAVE) || defined(ZW_CONTROLLER_STATIC) */

#else /* USE_RESPONSEROUTE */

#ifdef ZW_CONTROLLER
#ifndef ZW_CONTROLLER_STATIC
    /* none static controllers will when LWR not locked try direct before routing */
    pFreeTxElement->bRouteSchemeState = ZW_ROUTINGSCHEME_DIRECT;
    if (ZW_LockRouteGet())
    {
#endif
      if (LastWorkingRouteCacheLineGet(destNodeID,
#ifdef MULTIPLE_LWR
                                       CACHED_ROUTE_ANY,
#endif
                                      pFreeTxElement))
      {
        if (*freeTxElement_numRepsNumHops & MASK_ROUT_REPTS)
        {
          /* Found cached route has repeaters - therefor a FRAME_ROUTED type */
          frameType = FRAME_TYPE_ROUTED;
        }

        /* Check if we are beaming, Set nobeam flag if we are because this is first transmit to
            this destination*/
        if (pFreeTxElement->wRFoptions & RF_OPTION_BEAM_MASK)
        {
          pFreeTxElement->bFrameOptions1 |= TRANSMIT_FRAME_OPTION_NOBEAM;
        }

        /* Routing scheme update */
#ifdef MULTIPLE_LWR
        pFreeTxElement->bRouteSchemeState = DetermineCachedRouteRoutingScheme();
#else
        pFreeTxElement->bRouteSchemeState = ZW_ROUTINGSCHEME_CACHED_ROUTE;
#endif
        /* speed has been set in pFreeTxElement by LastWorkingRouteCacheLineGet() */
        speed &= ~(RF_OPTION_SPEED_MASK | RF_OPTION_BEAM_MASK);
        speed |= pFreeTxElement->wRFoptions & (RF_OPTION_SPEED_MASK | RF_OPTION_BEAM_MASK);
      }
      else
      {
        /* If nodes are neighbours lets try DIRECT */
        if (ZW_AreNodesNeighbours(srcNodeID, destNodeID))
        {
          pFreeTxElement->bRouteSchemeState = ZW_ROUTINGSCHEME_DIRECT;
        }
      }
#ifndef ZW_CONTROLLER_STATIC
    }
#endif  /* ZW_CONTROLLER_STATIC */

#endif /* ZW_CONTROLLER */

#endif /* USE_RESPONSEROUTE */
  }

  if ( ((destNodeID == NODE_BROADCAST_LR) && (HDRFORMATTYP_LR == curHeaderFormat))
       || ((destNodeID == NODE_BROADCAST) && (HDRFORMATTYP_LR != curHeaderFormat)) )
  {
    /* no acknowledge or auto_route on broadcast */
    TxQueueClearOptionFlags(pFreeTxElement, (TRANSMIT_OPTION_AUTO_ROUTE | TRANSMIT_OPTION_ACK));
  }

/* New ROUTING SCHEME - Assigned Return Route */

#ifdef ZW_SLAVE_ROUTING
  if ((pFreeTxElement->bRouteSchemeState == ZW_ROUTINGSCHEME_IDLE) &&
      ((TxQueueGetOptions(pFreeTxElement) & (TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE)) == (TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE)))
  {
    if (ReturnRouteFindFirst(destNodeID))
    {
      /* and return route found */
      /* Indicate ROUTE */
      /* Routing scheme update */
      speed = ReturnRouteGetSpeed(returnRoute.bReturnSpeed);
      /* TO#4144 memory overwrite... */
      bRepeaterCountInRoute = (returnRoute.routeNoNumHops & 0xF);
      if (bRepeaterCountInRoute && (MAX_REPEATERS >= bRepeaterCountInRoute))
      {
        /* one or more hops */
        if ((HDRFORMATTYP_3CH != curHeaderFormat))
        {
          memcpy(pFreeTxElement->frame.header.singlecastRouted.repeaterList, returnRoute.repeaterList, bRepeaterCountInRoute);
          pFreeTxElement->frame.header.singlecastRouted.numRepsNumHops = bRepeaterCountInRoute << 4;
        }
        else
        {
          memcpy(pFreeTxElement->frame.header.singlecastRouted3ch.repeaterList, returnRoute.repeaterList, bRepeaterCountInRoute);
          pFreeTxElement->frame.header.singlecastRouted3ch.numRepsNumHops = bRepeaterCountInRoute << 4;
        }
        frameType = FRAME_TYPE_ROUTED;
      }
      pFreeTxElement->bRouteSchemeState = ZW_ROUTINGSCHEME_ROUTE;
      pFreeTxElement->frame.frameOptions.destinationNodeId = returnRoute.nodeID;
    }
    else
    {
      /* We got no routes - go direct only */
      TxQueueClearOptionFlags(pFreeTxElement, TRANSMIT_OPTION_AUTO_ROUTE);
    }
  }
#endif /* ZW_SLAVE_ROUTING */

#ifdef ZW_CONTROLLER
  /* TO#1712 - 9.6k nodes would not have any neighbors if the last routing was to a 40k node */
  bRouteSpeed = bCurrentRoutingSpeed;
  SET_CURRENT_ROUTING_SPEED(RF_SPEED_9_6K);
  /* If destination node do not have any neighbours then it is not possible to route to the node */
  /* this should result in the removale of the TRANSMIT_OPTION_AUTO_ROUTE transmit option if present */
  if ((ZW_MAX_NODES < destNodeID) || !ZCB_HasNodeANeighbour(destNodeID))
  {
    TxQueueClearOptionFlags(pFreeTxElement, TRANSMIT_OPTION_AUTO_ROUTE);
  }
  SET_CURRENT_ROUTING_SPEED(bRouteSpeed);
#endif  /* ZW_CONTROLLER */

#ifdef ZW_CONTROLLER_STATIC
  if ( (pFreeTxElement->bRouteSchemeState == ZW_ROUTINGSCHEME_IDLE) &&
       (frameType == FRAME_TYPE_SINGLE))
  {
    if (!ZW_AreNodesNeighbours(srcNodeID, destNodeID))
    {
      if ((TxQueueGetOptions(pFreeTxElement) & (TRANSMIT_OPTION_ROUTED | TRANSMIT_OPTION_AUTO_ROUTE)) == TRANSMIT_OPTION_AUTO_ROUTE)
      {
          getNextRoute(bRouteSpeed, bRepeaterCountInRoute, &speed, &frameType, curHeaderFormat, pFreeTxElement);
      }
      else
      {
        TxQueueClearOptionFlags(pFreeTxElement, TRANSMIT_OPTION_AUTO_ROUTE);
      }
    }
    else
    {
      /* Src and Dest are neighbors - Transmit Direct */
      pFreeTxElement->bRouteSchemeState = ZW_ROUTINGSCHEME_DIRECT;
    }
  }
#endif  /* ZW_CONTROLLER_STATIC */
  /* New routing scheme - if nothing else we resort to direct transmission */
  if (pFreeTxElement->bRouteSchemeState == ZW_ROUTINGSCHEME_IDLE)
  {
    pFreeTxElement->bRouteSchemeState = ZW_ROUTINGSCHEME_RESORT_DIRECT;
  }

  if ((TxQueueGetOptions(pFreeTxElement) & (TRANSMIT_OPTION_ROUTED | TRANSMIT_OPTION_EXPLORE_REPEAT)) || (false == TransportIsPayloadLengthToBig(speed & RF_OPTION_SPEED_MASK, destNodeID, TxQueueGetOptions(pFreeTxElement), pFreeTxElement->bFrameOptions1, dataLength,curHeaderFormat)))
  {
    EnQueueCommon(speed, frameType, pData, dataLength, pCompletedFunc, pFreeTxElement);
    DPRINT("^");

    return(true);
  }
  else
  {
#ifdef USE_TRANSPORT_SERVICE
clean_up_and_exit:
#endif
    /* Something failed, free TxQueueElement and abort */
    TxQueueReleaseElement(pFreeTxElement);
    return false;
  }
}

/*===================   ConvertHeaderInfoToFrameType   =====================
**    Returns a frameType formatted status from a TxQueue
**    headerInfo in the lower nibble. Frametype is formatted as
**
**    +--------+----------+
**    |Seq No  |Frame Type|
**    +--------+----------+
**     8 7 6 5   4  3  2 1  bits
**
**    Only FrameType is converted. Seq No is always zero.
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
void
ConvertHeaderInfoToFrameType(
 uint8_t hdrInfo,       /*IN   HeaderInfo           */
 uint8_t *frameType)    /*OUT  FrameType            */
{
  switch (hdrInfo & MASK_HDRTYP)
  {
    case HDRTYP_SINGLECAST:
    {
      if (hdrInfo & MASK_ROUTED)
      {
        *frameType = FRAME_TYPE_ROUTED;
      }
      else
      {
        *frameType = FRAME_TYPE_SINGLE;
      }
      break;
    }
    case HDRTYP_MULTICAST:
    {
      *frameType = FRAME_TYPE_MULTI;
      break;
    }
    case HDRTYP_TRANSFERACK:
    {
      *frameType = FRAME_TYPE_ACK;
      break;
    }
    case HDRTYP_FLOODED:
    {
      *frameType = FRAME_TYPE_FLOODED;
      break;
    }
  }
}


#ifdef ZW_SLAVE_ROUTING
/*========================  TransmitReturnRouteSetup  ==================
**    Copies the selected return route info to frame waiting for ACK.
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
bool
TransmitReturnRouteSetup( void ) /* RET true if new return route found, false if not */
{
  if (activeTransmit.routeIndex != 0xFF)
  {
    uint8_t tmpRFOptions;
    uint8_t tmpFrameType;
    uint8_t bRepeaterCountInReturnRoute;

    pFrameWaitingForACK->frame.frameOptions.destinationNodeId = returnRoute.nodeID;
    tmpRFOptions = pFrameWaitingForACK->wRFoptions & (RF_OPTION_LOW_POWER | RF_OPTION_LONG_PREAMBLE);
    pFrameWaitingForACK->wRFoptions = ReturnRouteGetSpeed(returnRoute.bReturnSpeed);
     ZW_HeaderFormatType_t curHeaderFormat = llIsHeaderFormat3ch()?HDRFORMATTYP_3CH:HDRFORMATTYP_2CH;
    /* Write back saved LOW_POWER and LONG_PREAMBLE bits */
    pFrameWaitingForACK->wRFoptions |= tmpRFOptions;
    /* TO#4144 memory overwrite ... */
    bRepeaterCountInReturnRoute = (returnRoute.routeNoNumHops & 0xF);

    if (bRepeaterCountInReturnRoute && (MAX_REPEATERS >= bRepeaterCountInReturnRoute))
    {
      /* Can't use SET_HEADERTYPE macros as they overwrite route and direction bits */
      tmpFrameType = FRAME_TYPE_ROUTED;
      //activeTransmit.repeaterList = returnRoute.repeaterList;
    }
    else
    {
      tmpFrameType = FRAME_TYPE_SINGLE;
    }
    ClearFrameOptions(&pFrameWaitingForACK->frame);
    uint8_t tmpSeq;
    if (HDRFORMATTYP_2CH != curHeaderFormat)
    {
      if (tmpFrameType == FRAME_TYPE_ROUTED)
      {
        memcpy(pFrameWaitingForACK->frame.header.singlecastRouted3ch.repeaterList, returnRoute.repeaterList, bRepeaterCountInReturnRoute);
        pFrameWaitingForACK->frame.header.singlecastRouted3ch.numRepsNumHops = (uint8_t) (bRepeaterCountInReturnRoute << 4);
      }
      tmpSeq = IncSeqNo();
    }
    else
    {
      /* If we are here then it is always only when WE want to initiate something! */
      /* Increment currentSeqNoTX and keep it inside 1-15: 1 -> 2, 15 -> 1 */
      if (tmpFrameType == FRAME_TYPE_ROUTED)
      {
        memcpy(pFrameWaitingForACK->frame.header.singlecastRouted.repeaterList, returnRoute.repeaterList, bRepeaterCountInReturnRoute);
        pFrameWaitingForACK->frame.header.singlecastRouted.numRepsNumHops = (uint8_t) (bRepeaterCountInReturnRoute << 4);
        tmpFrameType |= IncSeqNo_2CH(
                              ((pFrameWaitingForACK->wRFoptions & RF_OPTION_SPEED_MASK) == RF_OPTION_SPEED_40K),
                              true);
      }
      else
      {
        tmpFrameType |= IncSeqNo_2CH(
                              ((pFrameWaitingForACK->wRFoptions & RF_OPTION_SPEED_MASK) == RF_OPTION_SPEED_40K),
                              false);
      }
      /* Default value is true as we only need it to be false when repeating */
      /* a frame or sending a routed ACK/ERR frame */
      currentSeqNoUseTX = true;
      tmpSeq = 0;
    }
    BuildTxHeader(curHeaderFormat, tmpFrameType, tmpSeq ,pFrameWaitingForACK);
    return true;
  }
  return false;
}
#endif /* ZW_SLAVE_ROUTING */

/*============================   RetransmitFail   ===========================
**    Retransmit frame failed. Try new routes or permanently give up.
**
**    Assumptions:
**      pFrameWaitingForACK points to the failed frame.
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
void             /*RET Nothing */
RetransmitFail( void )  /*IN  Nothing */
{
#if defined(ZW_CONTROLLER) || defined(ZW_SLAVE_ROUTING)
  uint8_t frameType = FRAME_TYPE_SINGLE;
#if defined(ZW_CONTROLLER)
  uint8_t bRepeaterCountInRoute;
  uint8_t bRouteSpeed;
#endif
#ifdef MULTIPLE_LWR
  uint8_t bAnyLWRExist;
#endif
  node_id_t WaitingForACK_destID;
#ifdef ZW_CONTROLLER
  uint8_t* WaitingForACK_numRepsNumHops;
#endif
  /* TO#3177 fix - Stop if no active frame. */
  if (!IS_TXQ_POINTER(pFrameWaitingForACK))
  {
    /* pFrameWaitingForACK not valid. */
    DPRINT("|");
    return;
  }

   uint8_t tmpFrameType;
   uint8_t tmpSeqNo;

   llReTransmitStop(&pFrameWaitingForACK->frame);
   ZW_HeaderFormatType_t curHeaderFormat = llGetCurrentHeaderFormat(pFrameWaitingForACK->frame.frameOptions.destinationNodeId, pFrameWaitingForACK->forceLR);
   WaitingForACK_destID  = GetDestinationNodeIDSinglecast(curHeaderFormat, (frame*)&pFrameWaitingForACK->frame.header);
   if (HDRFORMATTYP_LR == curHeaderFormat)
   {
#ifdef ZW_CONTROLLER
    WaitingForACK_numRepsNumHops = NULL; // Long Range doesn't do routing
#endif
    frameType = GET_HEADERTYPE_LR(pFrameWaitingForACK->frame.header);
   }
   else if (HDRFORMATTYP_3CH == curHeaderFormat)
  {
#ifdef ZW_CONTROLLER
    WaitingForACK_numRepsNumHops = &pFrameWaitingForACK->frame.header.singlecastRouted3ch.numRepsNumHops;
#endif
    frameType = GET_HEADERTYPE(pFrameWaitingForACK->frame.header);
  }
  else
  {
#ifdef ZW_CONTROLLER
    WaitingForACK_numRepsNumHops = &pFrameWaitingForACK->frame.header.singlecastRouted.numRepsNumHops;
#endif
    ConvertHeaderInfoToFrameType(pFrameWaitingForACK->frame.header.header.headerInfo, &frameType);
  }
#endif  /* ZW_CONTROLLER || ZW_SLAVE_ROUTING */

  /* SendDataAbort */
  /* Check if frame should be aborted */
  if ((bApplicationTxAbort) && (TxQueueGetOptions(pFrameWaitingForACK) & TRANSMIT_OPTION_APPLICATION))
  {
    /* Abort the frame (do nothing) */
#ifdef ZW_CONTROLLER
    /* Cleanup after GetNextRouteToNode() */
    badRoute.to = 0;
    badRoute.from = 0;
    UpdateFailedNodesList(WaitingForACK_destID, false);
#endif  //ZW_CONTROLLER

    /* Everything failed - Tell frame initiator */
    TxPermanentlyDone(false, 0, ZPAL_RADIO_RSSI_NOT_AVAILABLE, ZPAL_RADIO_RSSI_NOT_AVAILABLE, ZPAL_RADIO_RSSI_NOT_AVAILABLE);
    return;
  }

  if ((HDRFORMATTYP_2CH == curHeaderFormat) && IS_SPEED_MODIFIED(pFrameWaitingForACK))
  {
    pFrameWaitingForACK->bFrameOptions1 &= ~TRANSMIT_FRAME_OPTION_SPEED_MODIFIED;
    pFrameWaitingForACK->frame.frameOptions.speedModified = 0;
  }

#ifdef ZW_CONTROLLER
  if (HDRFORMATTYP_LR == curHeaderFormat)
  {
    if (pFrameWaitingForACK->bFrameOptions1 & TRANSMIT_FRAME_OPTION_NOBEAM)
    {
      /* No last working route stored */
      /* Restart routing scheme with beaming */
      pFrameWaitingForACK->bFrameOptions1 &= ~TRANSMIT_FRAME_OPTION_NOBEAM;
      pFrameWaitingForACK->wRFoptions = pFrameWaitingForACK->wRFoptionsBeforeModify;
      pFrameWaitingForACK->bRouteSchemeState = ZW_ROUTINGSCHEME_DIRECT;  // Use beam on LR
      ClearFrameOptions(&pFrameWaitingForACK->frame);

      //Reduce txPower because of preceding retransmissions.
      pFrameWaitingForACK->frame.frameOptions.txPower = ZW_DynamicTxPowerAlgorithm(pFrameWaitingForACK->frame.frameOptions.txPower, 0, 0,
                                                                                   RETRANSMISSION_FLIRS);
      pFrameWaitingForACK->frame.txPower = pFrameWaitingForACK->frame.frameOptions.txPower;
      SetTXPowerforLRNode(pFrameWaitingForACK->frame.frameOptions.destinationNodeId, pFrameWaitingForACK->frame.txPower);

      tmpFrameType = FRAME_TYPE_SINGLE;
      tmpSeqNo = IncSeqNo();

      BuildTxHeader(curHeaderFormat, tmpFrameType, tmpSeqNo, pFrameWaitingForACK);
      // Reset transmit count, the first attempts without beam doesn't count as retries
      pFrameWaitingForACK->bTransmitRouteCount = 0;
      TxQueueQueueElement(pFrameWaitingForACK);
      return;
    }
  }

#ifndef ZW_CONTROLLER_STATIC
  /* Use the Last Working route if found, when a direct has failed only if the */
  /* frame is not routed ack , repeated or routed frame */
  if (pFrameWaitingForACK->bRouteSchemeState == ZW_ROUTINGSCHEME_DIRECT)
  {
    if (!(TxQueueGetOptions(pFrameWaitingForACK) & TRANSMIT_OPTION_NO_ROUTE) &&
#ifdef USE_RESPONSEROUTE
        GetResponseRoute(WaitingForACK_destID, pFrameWaitingForACK))
#else
        LastWorkingRouteCacheLineGet(WaitingForACK_destID,
#ifdef MULTIPLE_LWR
                                     CACHED_ROUTE_ANY,
#endif
                                     pFrameWaitingForACK))
#endif  /* USE_REPONSEROUTE */
    {
      ClearFrameOptions(&pFrameWaitingForACK->frame);
      bRepeaterCountInRoute = *WaitingForACK_numRepsNumHops & MASK_ROUT_REPTS;
      //memcpy(pFrameWaitingForACK->sFrameHeader.singlecastRouted.repeaterList, activeTransmit.repeaterList, activeTransmit.repeaterCount);
      /* ResponseRoute/LastWorkingRoute can be DIRECT */
      if (HDRFORMATTYP_2CH != curHeaderFormat)
      {
        tmpFrameType = bRepeaterCountInRoute ? FRAME_TYPE_ROUTED : FRAME_TYPE_SINGLE;
        tmpSeqNo = IncSeqNo();
      }
      else
      {
        tmpFrameType = IncSeqNo_2CH(
                                /* GetResponseRoute sets pFrameWaitingForAck->wRFOptions accordingly so we can use it here to select speed */
                                 (pFrameWaitingForACK->wRFoptions & RF_OPTION_SPEED_MASK ) == RF_OPTION_SPEED_40K,
                                 bRepeaterCountInRoute) | (bRepeaterCountInRoute ? FRAME_TYPE_ROUTED : FRAME_TYPE_SINGLE);
        tmpSeqNo = 0;
      }
      BuildTxHeader(curHeaderFormat, tmpFrameType, tmpSeqNo, pFrameWaitingForACK);

      /* TO#1627 fix - ZW_RequestNodeNeighbor fails if questioned node is routed and first routing attempt fails. */
      /* Indicate Response Route/Last Working Route */
#ifdef MULTIPLE_LWR
      pFrameWaitingForACK->bRouteSchemeState = DetermineCachedRouteRoutingScheme();
#else
      pFrameWaitingForACK->bRouteSchemeState = ZW_ROUTINGSCHEME_CACHED_ROUTE;
#endif
      TxQueueQueueElement(pFrameWaitingForACK);
      return;
    }
    else if (pFrameWaitingForACK->bFrameOptions1 & TRANSMIT_FRAME_OPTION_NOBEAM)
    {
      /* No last working route stored */
      /* Restart routing scheme with beaming */
      pFrameWaitingForACK->bFrameOptions1 &= ~TRANSMIT_FRAME_OPTION_NOBEAM;
      pFrameWaitingForACK->wRFoptions = pFrameWaitingForACK->wRFoptionsBeforeModify;
      pFrameWaitingForACK->bRouteSchemeState = ZW_ROUTINGSCHEME_DIRECT;
      ClearFrameOptions(&pFrameWaitingForACK->frame);
      if (HDRFORMATTYP_2CH != curHeaderFormat)
      {
        tmpFrameType = FRAME_TYPE_SINGLE;
        tmpSeqNo = IncSeqNo();
      }
      else
      {
        tmpFrameType = IncSeqNo_2CH((pFrameWaitingForACK->wRFoptions & RF_OPTION_SPEED_MASK ) == RF_OPTION_SPEED_40K, false) | FRAME_TYPE_SINGLE;
        tmpSeqNo = 0;
      }
      BuildTxHeader(curHeaderFormat, tmpFrameType, tmpSeqNo, pFrameWaitingForACK);
      // Reset transmit count, the first attempts without beam doesn't count as retries
      pFrameWaitingForACK->bTransmitRouteCount = 0;
      TxQueueQueueElement(pFrameWaitingForACK);
      return;
    }
  }
#endif  /* !ZW_CONTROLLER_STATIC */
  /* TO#2947 fix - Purge LWR if LWR failed and if Static Controller and Neighbor try Direct. */
#ifdef MULTIPLE_LWR
  if ((pFrameWaitingForACK->bRouteSchemeState >= ZW_ROUTINGSCHEME_CACHED_ROUTE_SR) &&
      (pFrameWaitingForACK->bRouteSchemeState <= ZW_ROUTINGSCHEME_CACHED_ROUTE_NLWR))
#else
  if (pFrameWaitingForACK->bRouteSchemeState == ZW_ROUTINGSCHEME_CACHED_ROUTE)
#endif
  {
    /* Drop Last Working Route. */
    if (!(pFrameWaitingForACK->bFrameOptions1 & TRANSMIT_FRAME_OPTION_NOBEAM))
    {
#ifdef MULTIPLE_LWR
      /* Only try next Static Route if not nobeam attempt */
      /* Fetch next Static Route and if any found, copy the found Route to pFrameWaitingForACK */
      bAnyLWRExist = LastWorkingRouteCacheLineGet(pFrameWaitingForACK->frame.frameOptions.destinationNodeId,
                                                  DetermineCachedRouteNextID(pFrameWaitingForACK->bRouteSchemeState),
                                                  pFrameWaitingForACK);
      if (pFrameWaitingForACK->bRouteSchemeState >= ZW_ROUTINGSCHEME_CACHED_ROUTE)
      {
        /* if LWR failed */
        /* Move LWR onto ZW_NLWR and clear LWR */
        /* if NLWR failed */
        /* clear ZW_NLWR */
        LastWorkingRouteCacheLineExile(pFrameWaitingForACK->frame.frameOptions.destinationNodeId,
                                       ((pFrameWaitingForACK->bRouteSchemeState == ZW_ROUTINGSCHEME_CACHED_ROUTE) && (0 != bAnyLWRExist)) ?
                                       CACHED_ROUTE_ZW_LWR :
                                       CACHED_ROUTE_ZW_NLWR);
      }
      /* Any next Static Route found */
      if (0 != bAnyLWRExist)
      {
        /* Static Route found - set it up */
        ClearFrameOptions(&pFrameWaitingForACK->frame);
        if (HDRFORMATTYP_3CH == curHeaderFormat)
        {
          bRepeaterCountInRoute = *WaitingForACK_numRepsNumHops & MASK_ROUT_REPTS;
          tmpFrameType = bRepeaterCountInRoute ? FRAME_TYPE_ROUTED : FRAME_TYPE_SINGLE;
          tmpSeqNo = IncSeqNo();
        }
        else if (HDRFORMATTYP_2CH == curHeaderFormat)
        {
          bRepeaterCountInRoute = *WaitingForACK_numRepsNumHops & MASK_ROUT_REPTS;
          tmpFrameType = IncSeqNo_2CH((pFrameWaitingForACK->wRFoptions & RF_OPTION_SPEED_MASK ) == RF_OPTION_SPEED_40K,
                                  bRepeaterCountInRoute) | (bRepeaterCountInRoute ? FRAME_TYPE_ROUTED : FRAME_TYPE_SINGLE);
          tmpSeqNo = 0;
        }
        else
        {
          return;  //To make sure LR or undefined frames are not routed
        }

        BuildTxHeader(curHeaderFormat, tmpFrameType, tmpSeqNo, pFrameWaitingForACK);
        pFrameWaitingForACK->bRouteSchemeState = DetermineCachedRouteRoutingScheme();
        TxQueueQueueElement(pFrameWaitingForACK);
        return;
      }
#else   /* #ifdef MULTIPLE_LWR */
        /* Only drop if not nobeam attempt */
      LastWorkingRouteCacheLinePurge(pFrameWaitingForACK->frame.frameOptions.destinationNodeId);
#endif  /* #ifdef MULTIPLE_LWR */
    }
    else
    {
      /* NoBeam was set restore beam speed and restart routing */
      pFrameWaitingForACK->bFrameOptions1 &= ~TRANSMIT_FRAME_OPTION_NOBEAM;
      ClearFrameOptions(&pFrameWaitingForACK->frame);
#ifdef ZW_CONTROLLER_STATIC
      /* Restore beam speed options */
      pFrameWaitingForACK->wRFoptions = pFrameWaitingForACK->wRFoptionsBeforeModify;
      /* Rebuild routing header */
      if (HDRFORMATTYP_2CH != curHeaderFormat)
      {
        tmpFrameType = frameType;
        tmpSeqNo = IncSeqNo();
      }
      else
      {
        tmpFrameType = IncSeqNo_2CH((pFrameWaitingForACK->wRFoptions & RF_OPTION_SPEED_MASK ) == RF_OPTION_SPEED_40K, false) | frameType;
        tmpSeqNo = 0;
      }
      BuildTxHeader(curHeaderFormat, tmpFrameType, tmpSeqNo, pFrameWaitingForACK);
#else
      pFrameWaitingForACK->wRFoptions = pFrameWaitingForACK->wRFoptionsBeforeModify;
      pFrameWaitingForACK->bRouteSchemeState = ZW_ROUTINGSCHEME_DIRECT;
      if (HDRFORMATTYP_2CH != curHeaderFormat)
      {
        tmpFrameType = frameType;
        tmpSeqNo = IncSeqNo();
      }
      else
      {
        tmpFrameType = IncSeqNo_2CH((pFrameWaitingForACK->wRFoptions & RF_OPTION_SPEED_MASK ) == RF_OPTION_SPEED_40K,
                                 false) | frameType;
        tmpSeqNo = 0;
      }
      BuildTxHeader(curHeaderFormat, tmpFrameType, tmpSeqNo, pFrameWaitingForACK);

#endif
      // Reset transmit count, the first attempts without beam doesn't count as retries
      pFrameWaitingForACK->bTransmitRouteCount = 0;
      TxQueueQueueElement(pFrameWaitingForACK);
      return;
    }
#ifdef ZW_CONTROLLER_STATIC
    /* Are we a neighbor to the destination node... */
    if (ZW_AreNodesNeighbours(g_nodeID, WaitingForACK_destID))
    {
      /* We are neighbors then lets try DIRECT - we have removed the CACHED_ROUTE */
      /* so if the DIRECT also fails then we will try ROUTES if so stated. */
      pFrameWaitingForACK->bRouteSchemeState = ZW_ROUTINGSCHEME_DIRECT;
      ClearFrameOptions(&pFrameWaitingForACK->frame);
      tmpSeqNo = 0;
      if (HDRFORMATTYP_2CH != curHeaderFormat)
      {
        tmpFrameType = FRAME_TYPE_SINGLE;
        tmpSeqNo = IncSeqNo();
      }
      else
      {
        uint16_t curSpeed = (ZW_NODE_SUPPORT_SPEED_100K == MaxCommonSpeedSupported((uint8_t)g_nodeID, (uint8_t)pFrameWaitingForACK->frame.frameOptions.destinationNodeId)) ?
                             RF_SPEED_100K: RF_SPEED_40K;
        SET_SPEED(pFrameWaitingForACK, curSpeed);
        tmpFrameType = IncSeqNo_2CH(((pFrameWaitingForACK->wRFoptions & RF_OPTION_SPEED_MASK ) == RF_OPTION_SPEED_40K), false) | FRAME_TYPE_SINGLE;
      }
      BuildTxHeader(curHeaderFormat, tmpFrameType, tmpSeqNo, pFrameWaitingForACK);
      TxQueueQueueElement(pFrameWaitingForACK);
      return;
    }
#endif  /* ZW_CONTROLLER_STATIC */
  }
  if (TxQueueGetOptions(pFrameWaitingForACK) & TRANSMIT_OPTION_AUTO_ROUTE)
  {
    if (pFrameWaitingForACK->bRouteSchemeState < ZW_ROUTINGSCHEME_ROUTE)  /* Response/Last Working Route ? */
    {
      /* Drop Response Route, and get first route */
      abLastStartNode[0] = 0;
    }
    if (pFrameWaitingForACK->bRouteSchemeState <= ZW_ROUTINGSCHEME_ROUTE)
    {
      /* Route failed, try next route */
      if (frameType == FRAME_TYPE_SINGLE)
      {
        /* Direct Route failed, try routing via repeater nodes */
        UpdateMostUsedNodes(false, WaitingForACK_destID);
      }
      else
      {
        if (HDRFORMATTYP_3CH == curHeaderFormat)
        {
          /* Routed Route failed */
          UpdateMostUsedNodes(false, pFrameWaitingForACK->frame.header.singlecastRouted3ch.repeaterList[0]);
        }
        else
        {
          /* Routed Route failed */
          UpdateMostUsedNodes(false, pFrameWaitingForACK->frame.header.singlecastRouted.repeaterList[0]);
        }
      }

#ifndef ZW_CONTROLLER_STATIC
      /* Make sure that nobeam option is not set when calculating routes */
      pFrameWaitingForACK->bFrameOptions1 &= ~TRANSMIT_FRAME_OPTION_NOBEAM;
#endif

      if (HDRFORMATTYP_2CH == curHeaderFormat
          && IS_ROUTED(pFrameWaitingForACK->frame.header)
          && GET_SPEED(pFrameWaitingForACK) == RF_SPEED_100K
          && pFrameWaitingForACK->frame.header.header.length <= RX_MAX_LEGACY
          && ( pFrameWaitingForACK->frame.header.header.sourceID == g_nodeID
#ifdef ZW_CONTROLLER_BRIDGE
               || ZW_IsVirtualNode(pFrameWaitingForACK->frame.header.header.sourceID))
#else
             )
#endif
      )
      {
        SET_SPEED(pFrameWaitingForACK, RF_SPEED_40K);
        ClearFrameOptions(&pFrameWaitingForACK->frame);
        pFrameWaitingForACK->bFrameOptions1 |= (TRANSMIT_FRAME_OPTION_SPEED_MODIFIED);
        BuildTxHeader(curHeaderFormat, IncSeqNo_2CH(true,true) | FRAME_TYPE_ROUTED, 0, pFrameWaitingForACK);
        pFrameWaitingForACK->bFrameOptions1 &= ~(TRANSMIT_FRAME_OPTION_SPEED_MODIFIED);
        TxQueueQueueElement(pFrameWaitingForACK);
        return;
      }

      uint8_t repeaterNodes[MAX_REPEATERS] = { 0 };
      uint8_t nextRouteExist = GetNextRouteToNode(pFrameWaitingForACK->frame.frameOptions.destinationNodeId,
                                                  true,
                                                  repeaterNodes, &bRepeaterCountInRoute, &bRouteSpeed);
      if (nextRouteExist)
      {
        pFrameWaitingForACK->wRFoptions = SetSpeedOptions(pFrameWaitingForACK->wRFoptions, bRouteSpeed);
        pFrameWaitingForACK->bRouteSchemeState = ZW_ROUTINGSCHEME_ROUTE;
        ClearFrameOptions(&pFrameWaitingForACK->frame);
      }
      if (nextRouteExist && (HDRFORMATTYP_3CH == curHeaderFormat))
      {
        *WaitingForACK_numRepsNumHops = bRepeaterCountInRoute << 4;
        memcpy(pFrameWaitingForACK->frame.header.singlecastRouted3ch.repeaterList, repeaterNodes, bRepeaterCountInRoute);
        tmpFrameType = FRAME_TYPE_ROUTED;
        tmpSeqNo = IncSeqNo();

        BuildTxHeader(curHeaderFormat, tmpFrameType, tmpSeqNo, pFrameWaitingForACK);
        TxQueueQueueElement(pFrameWaitingForACK);
        return;
      }
      if (nextRouteExist && (HDRFORMATTYP_2CH == curHeaderFormat))
      {
        *WaitingForACK_numRepsNumHops = bRepeaterCountInRoute << 4;
        memcpy(pFrameWaitingForACK->frame.header.singlecastRouted.repeaterList, repeaterNodes, bRepeaterCountInRoute);
        tmpFrameType = IncSeqNo_2CH(
                                   /* GetNextRouteToNode sets bCurrentRoutingSpeed accordingly so we can use it here to select speed */
                                    bCurrentRoutingSpeed == RF_SPEED_40K,
                                    true) | FRAME_TYPE_ROUTED;
        tmpSeqNo = 0;

        BuildTxHeader(curHeaderFormat, tmpFrameType, tmpSeqNo, pFrameWaitingForACK);
        TxQueueQueueElement(pFrameWaitingForACK);
        return;
      }
    }
    /* RESORT_DIRECT is only executed (after a failed Tx) if AUTO_ROUTE has been specified */
    if (pFrameWaitingForACK->bRouteSchemeState < ZW_ROUTINGSCHEME_RESORT_DIRECT)
    {
      /* If we get here then resort to Direct attempt */
      ClearFrameOptions(&pFrameWaitingForACK->frame);
      if (HDRFORMATTYP_2CH != curHeaderFormat)
      {
        tmpFrameType = FRAME_TYPE_SINGLE;
        tmpSeqNo =  IncSeqNo();
      }
      else
      {
        /* TO#2729 fix - Direct should be send with highest possible speed */
        SET_SPEED(pFrameWaitingForACK, ChooseSpeedBeamForDestination_Common(WaitingForACK_destID, pFrameWaitingForACK, FRAME_TYPE_SINGLE));
        tmpFrameType = IncSeqNo_2CH((pFrameWaitingForACK->wRFoptions & RF_OPTION_SPEED_MASK ) == RF_OPTION_SPEED_40K,
                                 false) | FRAME_TYPE_SINGLE;
        tmpSeqNo =  0;
      }
      BuildTxHeader(curHeaderFormat, tmpFrameType, tmpSeqNo, pFrameWaitingForACK);
      if (HDRFORMATTYP_3CH == curHeaderFormat)
      {
        pFrameWaitingForACK->wRFoptions = ChooseSpeedBeamForDestination_Common(WaitingForACK_destID,
                                                                               pFrameWaitingForACK,
                                                                               FRAME_TYPE_SINGLE);
      }

      pFrameWaitingForACK->bRouteSchemeState = ZW_ROUTINGSCHEME_RESORT_DIRECT;
      TxQueueQueueElement(pFrameWaitingForACK);
      return;
    }
  }
  UpdateFailedNodesList(WaitingForACK_destID, false);
#endif  /* ZW_CONTROLLER */
#ifdef ZW_SLAVE_ROUTING
  if (HDRFORMATTYP_2CH == curHeaderFormat)
  {
    /* TO 2790 fix */
    /* If this is a 100K routed frame we originated, re-queue it at 40K for frequency agility */
    if ((pFrameWaitingForACK->frame.header.header.sourceID == g_nodeID)
      && IsRouted(HDRFORMATTYP_2CH, (frame*)&pFrameWaitingForACK->frame.header) && (GET_SPEED(pFrameWaitingForACK) == RF_SPEED_100K) &&
      (pFrameWaitingForACK->frame.header.header.length <= RX_MAX_LEGACY))
    {
      SET_SPEED(pFrameWaitingForACK, RF_SPEED_40K);
      ClearFrameOptions(&pFrameWaitingForACK->frame);
      pFrameWaitingForACK->bFrameOptions1 |= TRANSMIT_FRAME_OPTION_SPEED_MODIFIED;
      BuildTxHeader(HDRFORMATTYP_2CH, IncSeqNo_2CH(true,true) | FRAME_TYPE_ROUTED, 0, pFrameWaitingForACK);
      pFrameWaitingForACK->bFrameOptions1 &= ~TRANSMIT_FRAME_OPTION_SPEED_MODIFIED;
      TxQueueQueueElement(pFrameWaitingForACK);
      return;
    }
  }
  if (TxQueueGetOptions(pFrameWaitingForACK) & TRANSMIT_OPTION_AUTO_ROUTE &&
      pFrameWaitingForACK->bRouteSchemeState <= ZW_ROUTINGSCHEME_ROUTE)
  {
    /* tx failed, try next return route */
    if (pFrameWaitingForACK->bFrameOptions1 & TRANSMIT_FRAME_OPTION_NOBEAM)
    {
      /* Remove nobeam option */
      pFrameWaitingForACK->bFrameOptions1 &= ~TRANSMIT_FRAME_OPTION_NOBEAM;
      /* Restart routing with response route */
      pFrameWaitingForACK->wRFoptions = pFrameWaitingForACK->wRFoptionsBeforeModify;
      ClearFrameOptions(&pFrameWaitingForACK->frame);
      if (HDRFORMATTYP_2CH != curHeaderFormat)
      {
        tmpFrameType = FRAME_TYPE_SINGLE;
        tmpSeqNo = IncSeqNo();
      }
      else
      {
        tmpFrameType = IncSeqNo_2CH((pFrameWaitingForACK->wRFoptions & RF_OPTION_SPEED_MASK ) == RF_OPTION_SPEED_40K,
                                 false) | frameType;
        tmpSeqNo = 0;
      }
      BuildTxHeader(curHeaderFormat, tmpFrameType, tmpSeqNo, pFrameWaitingForACK);
      // Reset transmit count, the first attempts without beam doesn't count as retries
      pFrameWaitingForACK->bTransmitRouteCount = 0;

      TxQueueQueueElement(pFrameWaitingForACK);
      return;
    }
    if (pFrameWaitingForACK->bRouteSchemeState <= ZW_ROUTINGSCHEME_CACHED_ROUTE)
    {
      /* Determine value of destRouteIndex to make ReturnRouteFindNext look the right place */
      ReturnRouteFindFirst(WaitingForACK_destID);
      /* destRouteIndex = 0 -> SUC */
    }
    else
    {
      /* Return Routes */
      /* Unified ReturnRoute and SUCReturnRoute access */
      activeTransmit.routeIndex = ReturnRouteFindNext(activeTransmit.routeIndex + 1);
    }
    pFrameWaitingForACK->bRouteSchemeState = ZW_ROUTINGSCHEME_ROUTE;
    if (activeTransmit.routeIndex != 0xFF)
    {
      if (TransmitReturnRouteSetup())
      {
        TxQueueQueueElement(pFrameWaitingForACK);                 /* start retransmission */
        return;
      }
    }
    /* TO#2413 fix - RESORT_DIRECT is only executed (after a failed Tx) if AUTO_ROUTE has been specified */
    if (pFrameWaitingForACK->bRouteSchemeState < ZW_ROUTINGSCHEME_RESORT_DIRECT)
    {
      ClearFrameOptions(&pFrameWaitingForACK->frame);
      /* If we get here then resort to Direct attempt */
      if (HDRFORMATTYP_2CH != curHeaderFormat)
      {
        tmpFrameType = FRAME_TYPE_SINGLE;
        tmpSeqNo = IncSeqNo();
      }
      else
      {
#ifdef ZW_CONTROLLER
        if (ZW_NODE_SUPPORT_SPEED_100K == MaxCommonSpeedSupported((uint8_t)g_nodeID, (uint8_t)pFrameWaitingForACK->frame.frameOptions.destinationNodeId))
        {
          SET_SPEED(pFrameWaitingForACK, RF_SPEED_100K);
        }
        else
        {
          SET_SPEED(pFrameWaitingForACK, RF_SPEED_40K);
        }
        tmpFrameType = IncSeqNo_2CH((pFrameWaitingForACK->wRFoptions & RF_OPTION_SPEED_MASK ) == RF_OPTION_SPEED_40K,
                                 false) | FRAME_TYPE_SINGLE;
#else
        SET_SPEED(pFrameWaitingForACK, RF_SPEED_100K);
        tmpFrameType = IncSeqNo_2CH(false,  false) | FRAME_TYPE_SINGLE;
#endif

        tmpSeqNo = 0;
      }
      BuildTxHeader(curHeaderFormat, tmpFrameType, tmpSeqNo, pFrameWaitingForACK);
      pFrameWaitingForACK->bRouteSchemeState = ZW_ROUTINGSCHEME_RESORT_DIRECT;
      TxQueueQueueElement(pFrameWaitingForACK);
      return;
    }
  }
#endif /* ZW_SLAVE_ROUTING */

  /* Routing exhausted - Should we try using Explore frame */
  if (!bApplicationTxAbort && (pFrameWaitingForACK->bRouteSchemeState < ZW_ROUTINGSCHEME_RESORT_EXPLORE) &&
      (pFrameWaitingForACK->bFrameOptions1 & TRANSMIT_FRAME_OPTION_EXPLORE))
  {
    /* Try Explore Frame */
    /* Queue the frame in the exploreQueue - if possible */
    if (ExploreQueueFrame(pFrameWaitingForACK->frame.frameOptions.sourceNodeId, pFrameWaitingForACK->frame.frameOptions.destinationNodeId, pFrameWaitingForACK->frame.payload, pFrameWaitingForACK->frame.payloadLength,
                          ((TxQueueGetOptions(pFrameWaitingForACK) & TRANSMIT_EXPLORE_OPTION_MASK)| TRANSMIT_OPTION_ACK | QUEUE_EXPLORE_CMD_NORMAL),
                          &pFrameWaitingForACK->AppCallback))
    {
      /* We got a slot - now the exploreQueue slot must */
      /* be "manhandled" to convince that Explore is the way to go */
      /* pExploreQueueElement point to the exploreQueue element where frame was entered */
      /* This abuse of the Explorer module should be re-designed ASAP. Interaction with the Explorer module
         should be through its interface and not behind its back.
         It creates really nasty dependencies when in regard to changing the internals of the explorer module */

      pFrameWaitingForACK->frame.headerLength = 0; /* Explore header bytes counts in the payloadLength. Hence the frame header 0 length at this point */

      pExploreQueueElement->exploreFrame.pExploreHeader->ver_Cmd = EXPLORE_VER_DEFAULT | pExploreQueueElement->state.cmd;

      /* Max 4 repeaters  */
      pExploreQueueElement->exploreFrame.pExploreHeader->repeaterCountSessionTTL = 0x40;

      /* Initialy all repeaters are zero */
      memset(&pExploreQueueElement->exploreFrame.pExploreHeader->repeaterList, 0, MAX_REPEATERS);
      /* Save Start Tick sample in Explore Queue Element */
      pExploreQueueElement->state.StartTicks = pFrameWaitingForACK->StartTicks;
      pExploreQueueElement->state.exploreState = EXPLORE_STATE_SEARCH;
      ExploreSetTimeout(pExploreQueueElement, EXPLORE_FRAME_TIMEOUT); ; /* 4 sec */
      /* Set internal callback func so txqueue element gets released */
      pFrameWaitingForACK->zcbp_InternalCallback = ZCB_ProtocolTransmitComplete;
      /* Make the explore statemachine handle the rest */
      ZW_TransmitCallbackBind(&pFrameWaitingForACK->AppCallback, ZCB_ExploreComplete, 0);
      /* Force 40k speed */
      /* TODO - should be handled for 100/200k */
      /* TO#2737 fix - Let us actually force the 40kb */
      pFrameWaitingForACK->wRFoptions &= ~RF_OPTION_SPEED_MASK;
      pFrameWaitingForACK->wRFoptions |= RF_OPTION_SPEED_40K;
      TxQueueSetOptionFlags(pFrameWaitingForACK, pExploreQueueElement->state.exploreOptions);
      if ((HDRFORMATTYP_3CH == curHeaderFormat))
      {
        if (RX_MAX < pExploreQueueElement->state.payloadLength)
        {
          pFrameWaitingForACK->frame.payloadLength = RX_MAX;
        }
        else
        {
          pFrameWaitingForACK->frame.payloadLength = pExploreQueueElement->state.payloadLength;
        }
      }
      else
      {
        if (RX_MAX_LEGACY < pExploreQueueElement->state.payloadLength)
        {
          pFrameWaitingForACK->frame.payloadLength = RX_MAX_LEGACY;
        }
        else
        {
          pFrameWaitingForACK->frame.payloadLength = pExploreQueueElement->state.payloadLength;
        }
      }
      /* Copy Explore Header and payload contents into TXQueue payload buffer */
      memcpy(pFrameWaitingForACK->frame.payload, pExploreQueueElement->exploreFrame.baseFrame.payload,
             pFrameWaitingForACK->frame.payloadLength);

      pFrameWaitingForACK->bRouteSchemeState = ZW_ROUTINGSCHEME_RESORT_EXPLORE;
      /* We use the routed 40kb seqno for exploreframes */
      if (HDRFORMATTYP_2CH != curHeaderFormat)
      {
        tmpFrameType = FRAME_TYPE_EXPLORE;
        tmpSeqNo = IncSeqNo();
      }
      else
      {
        tmpFrameType = FRAME_TYPE_EXPLORE | IncSeqNo_2CH(RF_SPEED_40K, true);
        tmpSeqNo = 0;
      }
      BuildTxHeader(curHeaderFormat, tmpFrameType, tmpSeqNo, pFrameWaitingForACK);

      /* Peeewww - here goes */
      TxQueueQueueElement(pFrameWaitingForACK);                 /* start retransmission */
      return;
    }
    /* Could not transmit a Explore Frame - fail */
  }
  /* TODO - should we cleanup after route failures here also?? */

  /* Everything failed - Tell frame initiator */
  TxPermanentlyDone(false, 0, ZPAL_RADIO_RSSI_NOT_AVAILABLE, ZPAL_RADIO_RSSI_NOT_AVAILABLE, ZPAL_RADIO_RSSI_NOT_AVAILABLE);
}

/*=========================   DelayedRetransmitFail   =========================
**    Calls RetransmitFail() when this function has been called delayCount
**    times.
**
**    Useful for delays longer than the 2.53 ms maximum of system timers.
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
static void             /*RET Nothing */
ZCB_DelayedRetransmitFail( __attribute__((unused)) SSwTimer* pTimer )  /* IN Nothing */
{
  DPRINT("W6D/");
  RetransmitFail();
}


/*============================   ZWTransmitComplete   =========================
**    Action on Z-Wave transport layer transmit frame completed
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
static void           /*RET Nothing */
ZCB_ZWTransmitComplete(
  __attribute__((unused)) ZW_Void_Function_t Context,
  __attribute__((unused)) uint8_t status,        /* IN Transmission status */
  __attribute__((unused)) TX_STATUS_TYPE* TxStatusReport)
{
}


#ifdef ZW_REPEATER
/*============================   RepeatComplete   ==========================
**    Transmission of a repeated frame completed
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
static void       /*RET Nothing */
ZCB_ZWRepeatComplete(
  __attribute__((unused)) ZW_Void_Function_t Context,
  uint8_t status,    /* IN Transmission status  TRANSMIT_COMPLETE_OK or  */
                  /*                         TRANSMIT_COMPLETE_NO_ACK */
  __attribute__((unused)) TX_STATUS_TYPE* TxStatusReport)
{
  TxQueueElement *pFrame;
  /* Buffer for transferring repeater list from routed singlecast to routed error frame */
  uint8_t routingBuffer[MAX_REPEATERS];
  uint8_t routeLen;
  uint8_t tNumRepsNumHops;
  uint8_t tSeqNum;
  uint8_t repeatSource;
  uint8_t distNodeID;
  uint8_t tHeaderLength;
  uint8_t* tmp_repeaterList;
  uint8_t* tmp_routeStatus;

  /* An alternative approach could be to recycle pFrameWaitingForACK for sending
   * the routed error.  But I like the approach of copying data to routingBuffer,
   * then allocating a new TxQueue element better. It is cleaner since we dont need
   * to manually clear the txQueueElement, and equally resistant to TxQueue filling
   * up. This is because we still deallocate pFrameWaitingForACK before allocating
   * the R-Err element. And it only costs us sizeof(routingBuffer), 8 bytes of memory. */

    uint8_t curHeaderFormat;
   if ((status != TRANSMIT_COMPLETE_OK) &&
        IS_TXQ_POINTER(pFrameWaitingForACK))
   {
     curHeaderFormat = llIsHeaderFormat3ch()?HDRFORMATTYP_3CH:HDRFORMATTYP_2CH;
   }
   else
   {
     return;
   }

  /* Only send error if the frame is outgoing */
  if ( IsOutgoing(curHeaderFormat, (frame*)&pFrameWaitingForACK->frame.header))
  {
    /* Repeat failed, send an error to the source of the */
    /* routed frame */

    /* Copy data we need out of (just deallocated by our caller) pFrameWaitingForACK
     * We are relying on the fact that TxQueueElements are not cleared until
     * they are allocated again. So we still have time to recover the needed information
     * before allocating a queue element of our own (and quite possible clearing
     * pFrameWaitingForACK. */

    tSeqNum = pFrameWaitingForACK->frame.frameOptions.sequenceNumber;
    distNodeID = pFrameWaitingForACK->frame.frameOptions.destinationNodeId;

    if (HDRFORMATTYP_3CH == curHeaderFormat)
    {
      tNumRepsNumHops = pFrameWaitingForACK->frame.header.routedERR3ch.numRepsNumHops;
      routeLen =  (tNumRepsNumHops & MASK_ROUT_REPTS) >> 4;
      tmp_repeaterList = pFrameWaitingForACK->frame.header.routedERR3ch.repeaterList;
      tHeaderLength = sizeof(frameHeaderRoutedERR3ch)
                      - sizeof(frameHeaderSinglecast3ch)
                      - sizeof(uint8_t)*(MAX_REPEATERS - routeLen);
    }
    else
    {
      tNumRepsNumHops = pFrameWaitingForACK->frame.header.routedERR.numRepsNumHops;
      routeLen =  (tNumRepsNumHops & MASK_ROUT_REPTS) >> 4;
      tmp_repeaterList = pFrameWaitingForACK->frame.header.routedERR.repeaterList;
      /* We store the seqno in routing buffer similar to the 3CH code */
      tHeaderLength = sizeof(frameHeaderRoutedERR)
                      - sizeof(frameHeaderSinglecast)
                      - sizeof(uint8_t)*(MAX_REPEATERS - routeLen)
                      - MAX_EXTEND_BODY;

    }
    memcpy(routingBuffer, tmp_repeaterList, routeLen);
    repeatSource = pFrameWaitingForACK->frame.frameOptions.sourceNodeId;
    pFrame = TxQueueGetFreeElement(TX_QUEUE_PRIORITY_LOW, false);
    if (!pFrame)
    {
      return; /* txqueue full - cannot send r-err */
    }
    pFrame->bCallersStatus = TRANSFER_RETRIES;
    /* Fill out R-Err in the transmitted byte order */
    pFrame->frame.frameOptions.sourceNodeId = distNodeID;
    if ((HDRFORMATTYP_3CH == curHeaderFormat))
    {
      tmp_routeStatus = &pFrame->frame.header.routedERR3ch.routeStatus;
      tmp_repeaterList = pFrame->frame.header.routedERR3ch.repeaterList;
      pFrame->frame.frameOptions.frameType = FRAME_TYPE_ROUTED;
      pFrame->frame.frameOptions.sequenceNumber = tSeqNum;
      /* Keep route length, decrement hop count by 2.
       * Decrement once for changing direction and once more for using previous hop */
      pFrame->frame.header.routedERR3ch.numRepsNumHops =
  	  (tNumRepsNumHops & MASK_ROUT_REPTS) | ((tNumRepsNumHops - 2) & MASK_ROUT_HOPS);
  	  if ((pFrame->frame.header.routedERR3ch.numRepsNumHops & MASK_ROUT_HOPS) == 0x0F)
  	  {
          pFrame->frame.frameOptions.acknowledge = 1;
      }
    }
    else
    {
      tmp_routeStatus = &pFrame->frame.header.routedERR.routeStatus;
      tmp_repeaterList = pFrame->frame.header.routedERR.repeaterList;
      pFrame->frame.frameOptions.frameType = FRAME_TYPE_SINGLE;
      pFrame->frame.frameOptions.routed = 1;
      pFrame->frame.frameOptions.sequenceNumber = tSeqNum & MASK_SEQNO;
      SET_ROUTED(pFrame->frame.header);
      /* Keep route length, decrement hop count by 2.
       * Decrement once for changing direction and once more for using previous hop */
      pFrame->frame.header.routedERR.numRepsNumHops =
  	  (tNumRepsNumHops & MASK_ROUT_REPTS) | ((tNumRepsNumHops - 2) & MASK_ROUT_HOPS);
  	  if ((pFrame->frame.header.routedERR.numRepsNumHops & MASK_ROUT_HOPS) == 0x0F)
  	  {
          pFrame->frame.frameOptions.acknowledge = 1;
      }
    }
    SetHeaderType(curHeaderFormat, &pFrame->frame.header, pFrame->frame.frameOptions.frameType);
    SetInComming(curHeaderFormat, &pFrame->frame.header);
    SetRouteErr(curHeaderFormat, &pFrame->frame.header);
    if (pFrame->frame.frameOptions.acknowledge)
    {
      SetAck(curHeaderFormat, &pFrame->frame.header);
    }
    pFrame->frame.frameOptions.destinationNodeId = repeatSource;

    /*  Set failed hop. Repeater count is in the upper nibble - shift by 4 */
    *tmp_routeStatus |= ((tNumRepsNumHops << 4) & 0xF0);
    /*copy repeater list*/
    safe_memcpy(tmp_repeaterList, routingBuffer, routeLen, MAX_REPEATERS);
    pFrame->frame.payloadLength = 0;
    pFrame->frame.headerLength = tHeaderLength > sizeof(pFrame->frame.header) ? sizeof(pFrame->frame.header) : tHeaderLength;
    switch (curHeaderFormat)
    {
      case HDRFORMATTYP_2CH:
        pFrame->frame.headerLength += sizeof(pFrame->frame.header.singlecast);
        pFrame->frame.header.singlecast.header.length = pFrame->frame.headerLength;
        break;
      case HDRFORMATTYP_3CH:
        pFrame->frame.headerLength += (sizeof(pFrame->frame.header.singlecast3ch) - sizeof(pFrame->frame.header.singlecast3ch.extension));
        pFrame->frame.header.singlecast3ch.header.length = pFrame->frame.headerLength;
        break;
      case HDRFORMATTYP_LR:
        pFrame->frame.headerLength += (sizeof(pFrame->frame.header.singlecastLR) - sizeof(pFrame->frame.header.singlecastLR.extension));
        pFrame->frame.header.singlecastLR.header.length = pFrame->frame.headerLength;
        break;
      case HDRFORMATTYP_UNDEFINED:
        break;
    }
    /* Tell EnqueueSingleDataPtr() and EnqueueCommon() not to mess with the
         * protocol header or payload pointer. We already updated them. */
    pFrame->bFrameOptions1 |= TRANSMIT_FRAME_OPTION_NO_BUILD_TX_HDR;
    SetTransmitHomeID(pFrame);

    const STransmitCallback TxCallback = { .pCallback = ZCB_ZWTransmitComplete, .Context = 0 };
    TxQueueSetOptionFlags(pFrame, (TRANSMIT_OPTION_ROUTED | TRANSMIT_OPTION_ACK));
    EnqueueSingleDataPtr(mTransportRxCurrentSpeed,
                                          pFrame->frame.frameOptions.sourceNodeId,
                                          pFrame->frame.frameOptions.destinationNodeId,
                                          pFrame->frame.payload,
                                          pFrame->frame.payloadLength,
                                          &TxCallback,
                                          pFrame);
  }
}
#endif  /* ZW_REPEATER */

/****************************************************************************/
/*                           EXPORTED FUNCTIONS                             */
/****************************************************************************/
#ifdef ZW_SLAVE
/*================   ChooseSpeedForDestination_slave   =======================
**    This function selects the speed to send a frame with, taking the
**    destination nodeid as input.
**
**    The following is used to select speed:
**     - use response speed saved from last 2 frames received at each speed.
**     - otherwise use 9.6k which everyone supports.
**
**    Speed for return routes and response routes are set up in ReturnRouteFindNext()
**    and GetResponseRoute().
**
**    Side effects:
**--------------------------------------------------------------------------*/
uint8_t
ChooseSpeedForDestination_slave( /* RET Chosen speed RF_SPEED_* */
  node_id_t pNodeID)                  /* IN  Destination nodeID      */
{
  uint8_t idx;

  for (idx = 0; idx < MAX_RESPONSE_SPEED_NODES; idx++)
  {
    if (abDirectNodesResponseSpeed[idx].wNodeID == pNodeID)
    {
      return abDirectNodesResponseSpeed[idx].bResponseSpeed;
    }
  }
  return RF_SPEED_9_6K;  //TODO fix when we have asip code that can run 9.6k
}
#endif /* ZW_SLAVE */

/*===============================   SetSpeedOptions   ===========================
**    Set the speed options in a given value
**
**    Side effects:
**--------------------------------------------------------------------------*/
uint8_t
SetSpeedOptions(
  uint8_t bOldOptions,
  uint8_t bNewOption)
{
  /* Clear old speed option bits */
  bOldOptions &= ~RF_OPTION_SPEED_MASK;

  /* Set new speed option */
  return (bOldOptions | bNewOption);
}

#ifdef ZW_RAW_SUPPORT

/*=============================   ZW_SendDataRaw   ===========================
**    Transmit data buffer directly, only thing that is added is preamble
**    start of frame
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
uint8_t                          /* RET false if transmitter busy      */
ZW_SendDataRaw(
  uint8_t     *pData,            /* IN Data buffer pointer           */
  uint8_t      dataLength,       /* IN Data buffer length            */
  TxOptions_t  txOptions,        /* IN Transmit option flags         */
  uint8_t      bTxSpeedChannel   /* IN Transmit speed                */
  VOID_CALLBACKFUNC(completedFunc)(uint8_t)) /* IN Transmit completed call back function */
{
  return false;
}
#endif

#ifdef ZW_SECURITY_PROTOCOL
/*========================   IsKeySupported   ============================
**
**  Takes a specific security key as input and returns true if the key is
**  supported and activated at inclusion, false otherwise.
**
**-------------------------------------------------------------------------*/
static bool
IsKeySupported(
  enum SECURITY_KEY sc)
{
  uint8_t bSecKeyMask;

  bSecKeyMask = keystore_get_cached_security_keys();
  switch (sc)
  {
    case SECURITY_KEY_NONE:
      /* SECURITY_SCHEME_NONE always supported */
      return true;

    case SECURITY_KEY_S2_UNAUTHENTICATED:
      return (bSecKeyMask & SECURITY_KEY_S2_UNAUTHENTICATED_BIT);

    case SECURITY_KEY_S2_AUTHENTICATED:
      return (bSecKeyMask & SECURITY_KEY_S2_AUTHENTICATED_BIT);

    case SECURITY_KEY_S2_ACCESS:
      return (bSecKeyMask & SECURITY_KEY_S2_ACCESS_BIT);

    case SECURITY_KEY_S0:
      return (bSecKeyMask & SECURITY_KEY_S0_BIT);

    default:
      break;
  }
  return false;
}


enum ZW_SENDDATA_EX_RETURN_CODES                /*RET Return code      */
ZW_SendDataEx(
  uint8_t const * const pData,      /* IN Data buffer pointer           */
  uint8_t  dataLength, /* IN Data buffer length            */
  TRANSMIT_OPTIONS_TYPE *pTxOptionsEx,
  const STransmitCallback* pCompletedFunc)  /* IN Transmit completed call back function */
{
  TxQueueElement *pFreeTxElement;
  uint8_t return_val = ZW_TX_FAILED;

  DPRINTF("%d", pTxOptionsEx->destNode);
  eErrcode = E_EX_ERRCODE_TX_FAILED;
  /* TO#6625 fix - Make sure only Application allowed transmit options can be set */
  pTxOptionsEx->txOptions &= TRANSMIT_APPLICATION_OPTION_MASK;
  if (false == IsKeySupported(pTxOptionsEx->securityKey))
  {
    eErrcode = E_EX_ERRCODE_NO_SECURITY;
    return ZW_TX_FAILED;
  }
  if (SECURITY_KEY_NONE == pTxOptionsEx->securityKey)
  {
    /* Mimic good old senddata */
    /* Do we want to use the Explore Frame as a route resolution if all else fails */
    bUseExploreAsRouteResolution = (0 != (pTxOptionsEx->txOptions & TRANSMIT_OPTION_EXPLORE));
#if defined(ZW_CONTROLLER) || defined(ZW_SLAVE_ROUTING)
    /* Handling of SendDataAbort */
    pTxOptionsEx->txOptions |= TRANSMIT_OPTION_APPLICATION;
    bApplicationTxAbort = false;
#endif /* ZW_CONTROLLER */

    pFreeTxElement = TxQueueGetFreeElement(TX_QUEUE_PRIORITY_LOW, false);
    if (pFreeTxElement)
    {
      pFreeTxElement->StartTicks = getTickTime();
      pFreeTxElement->bTransmitRouteCount = 0;
      if (pTxOptionsEx->txOptions2 & TRANSMIT_OPTION_2_TRANSPORT_SERVICE)
      {
        pFreeTxElement->bFrameOptions1 |= TRANSMIT_FRAME_OPTION_TRANSPORT_SERVICE;
      }

      TxQueueInitOptions(pFreeTxElement, pTxOptionsEx->txOptions);
#ifdef ZW_CONTROLLER
      return_val = EnqueueSingleDataPtr(0, g_nodeID, bDestNodeID, pData, dataLength, pCompletedFunc, pFreeTxElement);
#endif  /* ZW_CONTROLLER */
#ifdef ZW_SLAVE
      return_val = EnqueueSingleDataPtr(ChooseSpeedForDestination_slave(pTxOptionsEx->destNode),
                                        g_nodeID, pTxOptionsEx->destNode, pData, dataLength, pCompletedFunc, pFreeTxElement);
#endif /* ZW_SLAVE */
      if (0 != return_val )
      {
        eErrcode = E_EX_ERRCODE_TX_IN_PROGRESS;
        return ZW_TX_IN_PROGRESS;
      }
#ifndef USE_TRANSPORT_SERVICE
      TxQueueReleaseElement(pFreeTxElement); /* Something in EnQueueSingleDataPtr failed, free
                                                * TxQueueElement and abort */
#endif
    }
    CleanUpAfterFailedTxElemAlloc();

    return ZW_TX_FAILED;
  }
  else if (SECURITY_KEY_S0 == pTxOptionsEx->securityKey)
  {
    if (sec0_send_data((node_t)g_nodeID, (node_t)pTxOptionsEx->destNode, pData, dataLength,
                       pTxOptionsEx->txOptions, pCompletedFunc))
    {
      eErrcode = E_EX_ERRCODE_TX_IN_PROGRESS;
      return ZW_TX_IN_PROGRESS;
    }
    else
    {
      return ZW_TX_FAILED;
    }
  }
  else if (pTxOptionsEx->securityKey <= SECURITY_KEY_S2_ACCESS)
  {
#ifdef ZW_CONTROLLER_BRIDGE
    /* If bSrcNode is not a VirtualNode then it can only be nodeID - no spoofing allowed. */
    if (!ZW_IsVirtualNode(pTxOptionsEx->bSrcNode))
#endif
    {
      pTxOptionsEx->bSrcNode = g_nodeID;
    }
    if (sec2_send_data((void*) pData, (uint16_t) dataLength, pTxOptionsEx,
                       pCompletedFunc))
    {
      eErrcode = E_EX_ERRCODE_TX_IN_PROGRESS;
      return ZW_TX_IN_PROGRESS;
    }
    else
    {
      return ZW_TX_FAILED;
    }
  }
  /* Unknown failure - should never happen */
  return ZW_TX_FAILED;
}


/**
 * Send multicast security s2 encrypted frame.
 * Only a S2 encrypted Broadcast frame will be transmitted. There will be no single cast follow ups.
 *
 * \param pTxOptionsMultiEx    Transmit options structure containing the transmission source, options and
 *                             groupID identifying the multicast group to use,
 *                             this group id will be used when calling S2_send_frame_multi
 * \param pData                plaintext to which is going to be sent.
 * \param dataLength           length of data to be sent.
 *
 */
enum ZW_SENDDATA_EX_RETURN_CODES                /*RET Return code      */
ZW_SendDataMultiEx(
  uint8_t *pData,            /* IN Data buffer pointer           */
  uint8_t  dataLength,       /* IN Data buffer length            */
  TRANSMIT_MULTI_OPTIONS_TYPE *pTxOptionsMultiEx,
  const STransmitCallback* pCompletedFunc)  /* IN Transmit completed call back function */
{
  eErrcode = E_EX_ERRCODE_TX_FAILED;
  /* Multicast Secure only supported in Security 2 */
  if ((SECURITY_KEY_NONE == pTxOptionsMultiEx->securityKey) || (SECURITY_KEY_S0 == pTxOptionsMultiEx->securityKey))
  {
    eErrcode = E_EX_ERRCODE_NO_SECURITY_2;
    return ZW_TX_FAILED;
  }
  /* TODO: what are the payload min and max sizes */
  if ((1 > dataLength) || (dataLength > MAX_MULTICAST_PAYLOAD - 16))
  {
    return ZW_TX_FAILED;
  }
  /* ZW_SendDataMultiEx do not support any TRANSMIT_OPTION but TRANSMIT_OPTION_APPLICATION */
  pTxOptionsMultiEx->txOptions = TRANSMIT_OPTION_APPLICATION;  /* SendDataAbort */
#ifdef ZW_CONTROLLER_BRIDGE
  if (!ZW_isVirtualNode(pTxOptionsMultiEx->bSrcNode)
  {
    pTxOptionsMultiEx->bSrcNode = g_nodeID;
  }
#else
  pTxOptionsMultiEx->bSrcNode = g_nodeID;
#endif
  if (sec2_send_data_multi((void*) pData, (uint16_t) dataLength, pTxOptionsMultiEx, pCompletedFunc))
  {
    eErrcode = E_EX_ERRCODE_TX_IN_PROGRESS;
    return ZW_TX_IN_PROGRESS;
  }
  return ZW_TX_FAILED;
}

#endif  /* ZW_SECURITY_PROTOCOL */

/*===============================   ZW_SendData   ===========================
**    Transmit data buffer to a single Z-Wave node or all Z-Wave nodes - broadcast
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
uint8_t                      /* RET false if transmitter busy     */
ZW_SendData(
  node_id_t     bDestNodeID, /* IN Destination node ID */
  const uint8_t *pData,       /* IN Data buffer pointer           */
  uint8_t       dataLength,  /* IN Data buffer length            */
  TxOptions_t   txOptions,   /* IN Transmit option flags         */
  const STransmitCallback* pCompletedFunc)  /* IN Transmit completed call back function */
{
  /* TO#5154 fix - make sure only TRANSMIT OPTIONS valid for Application are used */
  txOptions &= TRANSMIT_APPLICATION_OPTION_MASK;
  /* Do we want to use the Explore Frame as a route resolution if all else fails */
  if (txOptions & TRANSMIT_OPTION_EXPLORE)
  {
    /* Note that we want Explore tried as route resolution if all else fails  */
    bUseExploreAsRouteResolution = true;
  }
  /* Handling of SendDataAbort */
  txOptions |= TRANSMIT_OPTION_APPLICATION;
  bApplicationTxAbort = false;

  return (EnQueueSingleData(0, g_nodeID, bDestNodeID, pData, dataLength, txOptions,
                            0, // 0ms for tx-delay (any value)
                            ZPAL_RADIO_TX_POWER_DEFAULT, pCompletedFunc));
}

#ifdef ZW_CONTROLLER_BRIDGE
/*===========================   ZW_SendData_Bridge   =========================
**    Transmit data buffer to a single Z-Wave node or all Z-Wave nodes - broadcast
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
uint8_t                      /*RET false if transmitter busy     */
ZW_SendData_Bridge(
  node_id_t    bSrcNodeID,   /* IN Source node ID */
  node_id_t    bDestNodeID,  /* IN Destination node ID */
  uint8_t     *pData,        /* IN Data buffer pointer           */
  uint8_t      dataLength,   /* IN Data buffer length            */
  TxOptions_t  txOptions,    /* IN Transmit option flags         */
  const STransmitCallback* pCompletedFunc)  /* IN Transmit completed call back function */
{
  /* TO#5558 fix */
  if (!ZW_IsVirtualNode(bSrcNodeID))
  {
    /* If srcNodeID is not a VirtualNode then it can only be nodeID - no spoofing allowed. */
    bSrcNodeID = g_nodeID;
  }
  /* TO#5154 fix - make sure only TRANSMIT OPTIONS valid for Application are used */
  txOptions &= TRANSMIT_APPLICATION_OPTION_MASK;

  /* Do we want to use the Explore Frame as a route resolution if all else fails */
  if (txOptions & TRANSMIT_OPTION_EXPLORE)
  {
    /* Note that we want Explore tried as route resolution if all else fails  */
    bUseExploreAsRouteResolution = true;
  }

  /* Handling of SendDataAbort */
  txOptions |= TRANSMIT_OPTION_APPLICATION;
  bApplicationTxAbort = false;
  return (EnQueueSingleData(0, bSrcNodeID, bDestNodeID, pData, dataLength, txOptions,
                            0,  // 0ms for tx-delay (any value)
                            ZPAL_RADIO_TX_POWER_DEFAULT, pCompletedFunc));
}

#endif  /* ZW_CONTROLLER_BRIDGE */


#if defined(ZW_CONTROLLER) || defined(ZW_SLAVE_ROUTING)
void
ZW_SendDataAbort(void)
{
  /* TO#5772 fix */
  if (IS_TXQ_POINTER(pFrameWaitingForACK))
  {
    /* Only allow abort if the application is transmitting a frame */
    if (TxQueueGetOptions(pFrameWaitingForACK) & TRANSMIT_OPTION_APPLICATION) {

      /*
       * TODO Abort Transprot service
       * TODO Abort S2
       * TODO Abort S0
       */
      bApplicationTxAbort = true;
    }
  /* TO#5772 fix */
  }
}

#endif  /* defined(ZW_CONTROLLER) || defined(ZW_SLAVE_ROUTING) */


/*===============================   SendRouteAck  ==========================
**    Send a route acknowledge frame back to the destination
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
void               /* RET  Nothing */
SendRouteAck(
  RX_FRAME *pRx)       /* IN   Pointer to received frame */
{
  uint8_t bTemp;
  TxQueueElement *pFrame;

  pFrame = TxQueueGetFreeElement(TX_QUEUE_PRIORITY_LOW, false);
  if (!pFrame)
  {
    return; /* txqueue full - cannot send r-ack */
  }
  pFrame->bCallersStatus = TRANSFER_RETRIES;
  uint8_t curHeaderFormat = pRx->pReceiveFrame->channelHeaderFormat;
  /* totalLength has already been adjusted for CRC16 extra byte, ok to use CHECKSUM_LENGTH=1
   * on both 3CH and 2CH systems */
  uint8_t tHeaderLength = pRx->bTotalLength - pRx->payloadLength - CHECKSUM_LENGTH;
  /* Strip header extension if present - we will add a different one */
    if (GetExtendPresent(curHeaderFormat, pRx->pFrame))
    {
      tHeaderLength -= 1 + GetRoutedHeaderExtensionLen(curHeaderFormat, pRx->pFrame);
    }
    /* Strip dest wakeup byte (it is never present in R-Ack) */
  if (HDRFORMATTYP_3CH == curHeaderFormat)
  {
    tHeaderLength--;
  }
  /* Copy frame header from pRx to pFrame */
  safe_memcpy((uint8_t*)&pFrame->frame.header,
              (uint8_t*)&pRx->pFrame->header,
              tHeaderLength,
              sizeof(pFrame->frame.header));
  pFrame->frame.payloadLength = 0;
  SetInComming(curHeaderFormat, &pFrame->frame.header);
  SetRouteAck(curHeaderFormat, &pFrame->frame.header);
    /* decrement hops counter */
  bTemp = GetHops(curHeaderFormat, (frame*)&pFrame->frame.header);
  if (HDRFORMATTYP_3CH == curHeaderFormat)
  {
    pFrame->frame.header.singlecastRouted3ch.numRepsNumHops &= MASK_ROUT_REPTS;
  }
  else
  {
    pFrame->frame.header.singlecastRouted.numRepsNumHops &= MASK_ROUT_REPTS;
  }

  SetHops(curHeaderFormat, &pFrame->frame.header, (bTemp-1));

  /* Add rssi feedback header */
  /* Routed Ack does not have destination byte, which is assumed when temp was calculated */
  SetExtendPresent(curHeaderFormat, &pFrame->frame.header);
  SetExtendType(curHeaderFormat, &pFrame->frame.header, EXTEND_TYPE_RSSI_INCOMING);
  SetExtendLength(curHeaderFormat, &pFrame->frame.header, EXTEND_TYPE_RSSI_INCOMING_LEN);
  SetExtendBody(curHeaderFormat, &pFrame->frame.header, 0, ZPAL_RADIO_RSSI_NOT_AVAILABLE);
  SetExtendBody(curHeaderFormat, &pFrame->frame.header, 1, ZPAL_RADIO_RSSI_NOT_AVAILABLE);
  SetExtendBody(curHeaderFormat, &pFrame->frame.header, 2, ZPAL_RADIO_RSSI_NOT_AVAILABLE);
  SetExtendBody(curHeaderFormat, &pFrame->frame.header, 3, ZPAL_RADIO_RSSI_NOT_AVAILABLE);
  pFrame->frame.frameOptions.extended = 1;

  tHeaderLength += 1 + EXTEND_TYPE_RSSI_INCOMING_LEN;
  if (HDRFORMATTYP_3CH == curHeaderFormat)
  {
    tHeaderLength -= (sizeof(frameHeaderSinglecast3ch) - sizeof(frameHeaderExtension3ch));
  }
  else
  {
    tHeaderLength -= sizeof(frameHeaderSinglecast);
  }
    // copy constructed frame header to the payload (Is this is the right way ?)
  pFrame->frame.headerLength = tHeaderLength > sizeof(pFrame->frame.header) ? sizeof(pFrame->frame.header) : tHeaderLength;
  /* swap source/destination */
  pFrame->frame.frameOptions.sourceNodeId = pRx->pReceiveFrame->frameOptions.destinationNodeId;
  pFrame->frame.frameOptions.destinationNodeId = pRx->pReceiveFrame->frameOptions.sourceNodeId;

  pFrame->bCallersStatus = TRANSFER_RETRIES; /* retransmissions */

  pFrame->frame.headerLength = tHeaderLength;
  pFrame->frame.payloadLength = 0;
  switch (curHeaderFormat)
  {
    case HDRFORMATTYP_2CH:
      pFrame->frame.headerLength += sizeof(pFrame->frame.header.singlecast);
      pFrame->frame.header.header.length = pFrame->frame.headerLength + pFrame->frame.payloadLength;
      break;
    case HDRFORMATTYP_3CH:
      pFrame->frame.headerLength += (sizeof(pFrame->frame.header.singlecast3ch) - sizeof(pFrame->frame.header.singlecast3ch.extension));
      pFrame->frame.header.header.length = pFrame->frame.headerLength + pFrame->frame.payloadLength;
      break;
    case HDRFORMATTYP_LR:
      pFrame->frame.headerLength += (sizeof(pFrame->frame.header.singlecastLR) - sizeof(pFrame->frame.header.singlecastLR.extension));
      pFrame->frame.header.header.length = pFrame->frame.headerLength + pFrame->frame.payloadLength;
      break;
    case HDRFORMATTYP_UNDEFINED:
      break;
  }

  /* Tell EnqueueSingleDataPtr() and EnqueueCommon() not to mess with the
     * protocol header or payload pointer. We already updated them. */
  pFrame->bFrameOptions1 |= TRANSMIT_FRAME_OPTION_NO_BUILD_TX_HDR;
  pFrame->frame.frameOptions.frameType = pRx->pReceiveFrame->frameOptions.frameType;
  pFrame->frame.frameOptions.sequenceNumber = pRx->pReceiveFrame->frameOptions.sequenceNumber;

  SetTransmitHomeID(pFrame);
  /* transmit the frame and lock the local transmit buffer if succ. */
  /* A repeater may NEVER change the speed when repeating a frame!! */

  const STransmitCallback TxCallback = { .pCallback = ZCB_ZWTransmitComplete,.Context = 0 };

  TxQueueInitOptions(pFrame, (TRANSMIT_OPTION_ROUTED | TRANSMIT_OPTION_ACK));
  EnqueueSingleDataPtr(mTransportRxCurrentSpeed,
                                        pFrame->frame.frameOptions.sourceNodeId,
                                        pFrame->frame.frameOptions.destinationNodeId,
                                        pFrame->frame.payload,
                                        pFrame->frame.payloadLength,
                                        &TxCallback,
                                        pFrame);
}


/*==========================   HandleTransferAck   ========================
**
** Handle transfer acknowledge
**
** Prerequisites: pFrameWaitingForACK != NULL
**
**--------------------------------------------------------------------------*/
void                    /* RET Nothing */
HandleTransferAck(
  uint8_t curHeaderFormat,  /* IN  Frameheader format */
  uint8_t bTransferAckRssi, /* IN  RSSI value of transfer ack */
  int8_t bDestinationAckUsedTxPower, /* IN TxPower from the received ACK */
  int8_t bDestinationAckMeasuredRSSI, /* IN TxPower from the received ACK */
  int8_t bDestinationAckMeasuredNoiseFloor /* IN TxPower from the received ACK */
  )
{
  DPRINT("+");
  /* If the frame is routed then start routed acknowledge timeout */
  if (IsFrameRouted(curHeaderFormat, (frame*)&pFrameWaitingForACK->frame.header) &&
      (
#ifdef ZW_CONTROLLER_BRIDGE
       (ZW_IsVirtualNode(pFrameWaitingForACK->frame.header.header.sourceID)) ||
#endif
       (pFrameWaitingForACK->frame.header.header.sourceID == g_nodeID)) &&
        /* Don't wait for r-ack on r-errs/r-acks */
       /* We check if the received ACK is an routed Ack/Err */
       !isRoutedAckErr)
  {
    /* We are the routed frame initiator - wait for Routed ACK */
    waitingForRoutedACK = true;
    /* Refactoring idea: just call internal callback func here? */
    /* Start routing failed timer */
    if (((HDRFORMATTYP_3CH == curHeaderFormat) && (GET_ROUTING_DEST_WAKEUP_3CH(pFrameWaitingForACK->frame.header))) ||
        ((HDRFORMATTYP_3CH != curHeaderFormat) &&
         (
          /* Do the Extend body exist */
          GET_EXTEND_PRESENT(pFrameWaitingForACK->frame.header) &&
          /* Is the Extend body of EXTEND_TYPE_WAKEUP_TYPE */
          (GET_EXTEND_TYPE_HEADER(pFrameWaitingForACK->frame.header) == EXTEND_TYPE_WAKEUP_TYPE) &&
          /* For now we only examine if dest is to receive a beam */
          (GET_EXTEND_BODY_HEADER(pFrameWaitingForACK->frame.header, EXTEND_TYPE_WAKEUP_TYPE_SRC_DEST_OFFSET) & EXTEND_TYPE_WAKEUP_TYPE_DEST))))
    {
      /*
       * Every hop in route adds 1.2 sec to timeout.
       * 3 Channel fragmented BEAM adds 3 * 3300ms to timeout.
       * 2 Channel 1000MS BEAM adds 3 * 1200ms to timeout. We use this for 250MS BEAMs also for simplicity.
       */
      uint32_t iDelayMultiplier = (HDRFORMATTYP_3CH == curHeaderFormat) ? (FRAG_BEAM_WAIT_COUNT * 3300) : (TRANSFER_RETRIES * 1200);
      iDelayMultiplier += (GetRouteLen(curHeaderFormat, (frame*)&pFrameWaitingForACK->frame.header) * 1200);

      /*
       * TransmitTimerStart divides the timeout value by 4. So we compensate
       * by multiplying the input timeout value before calling it
       */
      TransmitTimerStart(ZCB_DelayedRetransmitFail, iDelayMultiplier * 4);  // DelayMultiplier may never be 0
    }
    else
    {
      /* TODO BEAM - Timeout needs to be aligned with the hops and destination beam requirements */
      /*the number of retires we should wait for isd according to the following 6*n + 2
      The repeated frame from first repeater don't count in*/
      uint32_t delay_time = (TRANSFER_ROUTED_FRAME_WAIT_TIME_MS * GetRouteLen(curHeaderFormat, (frame*)&pFrameWaitingForACK->frame.header));
      if ( ((pFrameWaitingForACK->wRFoptions & RF_OPTION_SPEED_MASK) == RF_OPTION_SPEED_9600))
      {
        delay_time += (((GetRouteLen(curHeaderFormat, (frame*)&pFrameWaitingForACK->frame.header) * 6) + 2) * TRANSFER_9_6K_LBT_DELAY_MS);
      }
      else
      {
        delay_time += (((GetRouteLen(curHeaderFormat, (frame*)&pFrameWaitingForACK->frame.header) * 6) + 2) * TRANSFER_100K_LBT_DELAY_MS);
      }
      TransmitTimerStart(ZCB_DelayedRetransmitFail, delay_time);  /*Fix TO# 1574 - Was (GET_ROUTE_LEN(..) +1)*/
    }
  }  /* routed frame AND we are initiator */
  else
  {
    /* not routed frame OR not initiator */
    /* Signal tx complete to application */
     {
      /*Only REAL TransferACK frames should here result in ResponseRoute/LastWorkingRoute */
      if (!IsRouted(curHeaderFormat, (frame*)&pFrameWaitingForACK->frame.header))
      {
#ifdef ZW_CONTROLLER
        node_id_t tmpDestID;
        if (HDRFORMATTYP_3CH == curHeaderFormat)
        {
          tmpDestID = pFrameWaitingForACK->frame.header.singlecast3ch.destinationID;
        }
        else if (HDRFORMATTYP_LR == curHeaderFormat)
        {
          tmpDestID = GET_SINGLECAST_DESTINATION_NODEID_LR(pFrameWaitingForACK->frame.header);
        }
        else
        {
          tmpDestID = pFrameWaitingForACK->frame.header.singlecast.destinationID;
        }
        UpdateFailedNodesList(tmpDestID, true);

        /* New Routing Scheme */
        /* Direct can now also be in Response Routes/Last Working Routes */
#ifdef USE_RESPONSEROUTE
        StoreRoute(curHeaderFormat, &pFrameWaitingForACK->frame.header, &pFrameWaitingForACK->frame.frameOptions);
#else
        /* We just received an ACK for the frame in pFrameWaitingForACK */
        if ((0 != tmpDestID) &&
            (NODE_CONTROLLER_OLD != tmpDestID) &&
            (ZW_ROUTINGSCHEME_RESORT_DIRECT == pFrameWaitingForACK->bRouteSchemeState) &&
            (RF_PROFILE_9_6K == rxFrame.pReceiveFrame->profile))
        {
          /*If an ack is received to a direct route using 9.6K speed after last working routes failed, then we purge the last working routes*/
          uint8_t aZeroes[ROUTECACHE_LINE_SIZE];
          memset(aZeroes, 0, sizeof(aZeroes));
          LastWorkingRouteCacheNodeSRLockedSet(tmpDestID, false);
          CtrlStorageSetRouteCache(ROUTE_CACHE_NORMAL, tmpDestID, (ROUTECACHE_LINE *)&aZeroes);
          CtrlStorageSetRouteCache(ROUTE_CACHE_NLWR_SR, tmpDestID, (ROUTECACHE_LINE *)&aZeroes);
        }

        if ((HDRFORMATTYP_3CH == curHeaderFormat) ||
            (!pFrameWaitingForACK->frame.frameOptions.speedModified  && !IS_ROUTED_SPEED_MODIFIED(pFrameWaitingForACK->frame.header)))
        {
          /* The LastWorkingRoute is for the destination */
#ifdef MULTIPLE_LWR
          LastWorkingRouteCacheLineUpdate(tmpDestID, CACHED_ROUTE_ZW_LWR, (uint8_t*)&pFrameWaitingForACK->frame.header);
#else
          LastWorkingRouteCacheLineUpdate(tmpDestID, (uint8_t*)&pFrameWaitingForACK->frame.header);
#endif
        }
#endif  /* USE_RESPONSEROUTE */

#endif  /* ZW_CONTROLLER */

#ifdef ZW_SLAVE
#ifdef USE_RESPONSEROUTE
#ifdef ZW_RETURN_ROUTE_PRIORITY
        if (activeTransmit.routeIndex != 0xFF)
        {
          /* Ack received from this route. Change its priority */
          ReturnRouteChangePriority(bReturnRoutePriorityIndex);
        }
#endif  /* ZW_RETURN_ROUTE_PRIORITY */
        /* Update Response Route and Last Return Route - Transfer ACK received */
        UpdateResponseRouteLastReturnRoute(StoreRoute(curHeaderFormat, (uint8_t*)&pFrameWaitingForACK->frame.header, &pFrameWaitingForACK->frame.frameOptions));
#endif  /* USE_RESPONSEROUTE */
#endif  /* ZW_SLAVE */
      }
      DPRINT("((");
      TransmitTimerStop();
      /* TO-DO Which status to report here? */
      memset(&abRssiFeedback[1], ZPAL_RADIO_RSSI_NOT_AVAILABLE,sizeof(abRssiFeedback)-1);
      abRssiFeedback[0] = bTransferAckRssi;
      /* Received ACK on TransportGetCurrentRxChannel() */
      pFrameWaitingForACK->bACKChannel = TransportGetCurrentRxChannel();
      TxPermanentlyDone(true,
                        abRssiFeedback,
                        bDestinationAckUsedTxPower,
                        bDestinationAckMeasuredRSSI,
                        bDestinationAckMeasuredNoiseFloor); /* Transmit completed */
    }
  }
}

/*==========================   HandleRoutedAck   ============================
**
** Handle routed transfer acknowledge send to the controller
**
** Prerequisites: We have an outstanding routed frame and are waiting for an
**   ACK on it
**--------------------------------------------------------------------------*/
void                  /* RET Nothing */
HandleRoutedAck(
  RX_FRAME *pRxFrame,   /*IN The incoming routed ack frame  */
  uint8_t bRoutedAckRSSI) /*IN  RSSI value of routed ACK heard from this node*/
{
#ifdef ZW_CONTROLLER
  UpdateFailedNodesList(pFrameWaitingForACK->frame.frameOptions.destinationNodeId, true);
#endif
#ifdef ZW_SLAVE_ROUTING
#ifdef ZW_RETURN_ROUTE_PRIORITY
  if ((TxQueueGetOptions(pFrameWaitingForACK) & TRANSMIT_OPTION_AUTO_ROUTE) &&
      (activeTransmit.routeIndex != 0xFF))
  {
    /* Routed Ack from this route. Change its priority */
    ReturnRouteChangePriority(bReturnRoutePriorityIndex);
  }
#endif  /* ZW_RETURN_ROUTE_PRIORITY */
#endif  /* ZW_SLAVE_ROUTING */
  /* TO#2520 fix - transmit timer was not stopped on routed frames */
  DPRINT("H");
  TransmitTimerStop();

  /* If there are RSSI values in r-ack then copy them to
   * app callback */
  if ( GetExtendPresent(pRxFrame->pReceiveFrame->channelHeaderFormat, pRxFrame->pFrame) && (GetExtendType(pRxFrame->pReceiveFrame->channelHeaderFormat, pRxFrame->pFrame) == EXTEND_TYPE_RSSI_INCOMING) )
  {
    memcpy(&abRssiFeedback[1], GetExtendBodyAddr(pRxFrame->pReceiveFrame->channelHeaderFormat, pRxFrame->pFrame, 0), MAX_REPEATERS);
  }

  abRssiFeedback[0] = bRoutedAckRSSI;
  /* Received ACK on TransportGetCurrentRxChannel() */
  pFrameWaitingForACK->bACKChannel = TransportGetCurrentRxChannel();
  /* Notification application of delivery success */
  TxPermanentlyDone(true, abRssiFeedback, ZPAL_RADIO_RSSI_NOT_AVAILABLE, ZPAL_RADIO_RSSI_NOT_AVAILABLE, ZPAL_RADIO_RSSI_NOT_AVAILABLE);
}


/*================================   UpdateRxStatus   =========================================
 Update the Rx status flags
================================================================================================*/
uint8_t
UpdateRxStatus(void)
{
#ifdef USE_RESPONSEROUTE
  uint8_t tRxStatus = ResponseRouteFull() ? RECEIVE_STATUS_ROUTED_BUSY : 0;
#else
  uint8_t tRxStatus = 0;
#endif
  uint8_t curHeaderFormat = rxFrame.pReceiveFrame->channelHeaderFormat;
  if (IsLowPower(curHeaderFormat, rxFrame.pFrame))
  {
    tRxStatus |= RECEIVE_STATUS_LOW_POWER;
  }
  if (rxFrame.status & STATUS_NOT_MY_HOMEID)
  {
    tRxStatus |= RECEIVE_STATUS_FOREIGN_HOMEID;
  }
  uint8_t hdrType = GetHeaderType(curHeaderFormat, rxFrame.pFrame);
  if (HDRTYP_EXPLORE == hdrType)
  {
    tRxStatus |= RECEIVE_STATUS_TYPE_EXPLORE;
  }
  else
  if (HDRTYP_SINGLECAST == hdrType)
  {
    if ((GetDestinationNodeIDSinglecast(curHeaderFormat, rxFrame.pFrame) == NODE_BROADCAST) ||
        (GetDestinationNodeIDSinglecast(curHeaderFormat, rxFrame.pFrame) == NODE_BROADCAST_LR))
    {
      tRxStatus |= RECEIVE_STATUS_TYPE_BROAD;
    }
  }
  /* TO#2660 - Routed singlecast frames are noted as being Multicast frames */
  else if (HDRTYP_MULTICAST == hdrType)
  {
    tRxStatus |= RECEIVE_STATUS_TYPE_MULTI;
  }
  return tRxStatus;
}


void
SetTransmitPause(void)
{
  if (sACK & RECEIVE_DO_DELAY)
  {
    /* Freeze all new transmits for a while. We need to avoid collision with the Transfer ACK on a routed-ACK
     * we originated.
     *If frame speed is 100k or 40k reduce the pause time by 4
     */
    TxQueueStartTransmissionPause(TimeOutConvert(MAC_TRANSMIT_DELAY_MS));
  }
}

bool
IsRouteValid(ZW_HeaderFormatType_t headerFormatType, const frame *pFrame)
{
  /**
   * Received frame is routed - check if route seems valid
   *
   */
  uint8_t route[] = {0, 0, 0, 0};
  uint8_t routeLen = GetRouteLen(headerFormatType, pFrame);
  uint8_t routeHopsIndex = GetHops(headerFormatType, pFrame);

  if ((0 == routeLen) || (4 < routeLen))
  {
    // If frame is routed it needs at least one repeater and max supported repeaters are 4
    return false;
  }
  if (NotOutgoing(headerFormatType, pFrame))
  {
    // Route incomming
    // For incomming routes Hops can never be equal to RouteLen
    if (routeHopsIndex == routeLen)
    {
      return false;
    }
    // For incomming routes Hops can only be bigger than RouteLen if it equals 15
    if ((routeLen < routeHopsIndex) && (15 != routeHopsIndex))
    {
      return false;
    }
  }
  else
  {
    // Route outgoing
    // For outgoing routes Hops can never be greater than RouteLen
    if (routeLen < routeHopsIndex)
    {
      return false;
    }
  }
  for (uint8_t i = 0; i < routeLen; i++)
  {
    route[i] = ReadRepeater(headerFormatType, pFrame, i);
  }
  // 4 repeater route - here 4th repeater needs to be non zero and this repeater nodeID must only be present once in the route.
  if ((4 == routeLen) && ((0 == route[3]) || (route[3] == route[2]) || (route[3] == route[1]) || (route[3] == route[0])))
  {
    return false;
  }
  // 3+ repeater route - here 3rd repeater needs to be non zero and this repeater nodeID must only be present once in the route.
  if ((3 <= routeLen) && ((0 == route[2]) || (route[2] == route[1]) || (route[2] == route[0])))
  {
    return false;
  }
    // 2+ repeater route - here 2nd repeater needs to be non zero and this repeater nodeID must only be present once in the route.
  if ((2 <= routeLen) && ((0 == route[1]) || (route[1] == route[0])))
  {
    return false;
  }
  // If frame is a routed frame then first repeater must be non zero.
  if (0 == route[0])
  {
    return false;
  }
  // Route seems valid
  return true;
}

bool ProtocolZWaveLongRangeChannelSet(uint8_t channelId);


/*===============================   ReceiveHandler   ========================
**    Validation and forwarding of a received frame
**
****
**    save buffer pointer
**    validate frame: length, checksum, Home ID
**    switch (frameType) {
**    case: singlecast
**           check Destination Node ID
**           if (!(broadcast frame) && (frame destination is for this node))
**             if (ACK request)
**               Send Transfer Acknowledge frame
**    #if ZW_REPEATER
**           if (routed frame)
**               Forward frame to Repeat module
**    #endif
**    case: multicast
**           check Mask bit
**    }
**    if (frame destination is for this node)
**      if (application command)
**        Forward payload pointer to application_command handler
**      else
**        Forward payload pointer to ZW_command handler
****
**    Side effects:
**
**--------------------------------------------------------------------------*/
void                          /*RET  Nothing      */
ReceiveHandler(
  ZW_ReceiveFrame_t *pReceiveFrame)
{
  // Data buffer pointer
  uint8_t  *pData      = pReceiveFrame->frameContent;
  // Data buffer length
  uint8_t  dataLength  = pReceiveFrame->frameContentLength;
  // RSSI samples taken during frame reception
  uint8_t  rssi = pReceiveFrame->rssi;
  uint8_t numHops;
#ifdef ZW_REPEATER
  uint8_t numHops1;
#endif
  uint8_t activeNumRepeatersNumHops;
#ifdef ZW_CONTROLLER
  uint8_t routeStatus;
  uint8_t numRepsNumHops;
  uint8_t hopNumber;
#endif
  uint8_t *protPayloadCmd = NULL;
  uint8_t *protCmdClass = NULL;
  DPRINT("H");
  rxFrame.pReceiveFrame = pReceiveFrame;
  if ( (RF_PROFILE_100K == rxFrame.pReceiveFrame->profile) ||
       (RF_PROFILE_3CH_100K == rxFrame.pReceiveFrame->profile) ||
       (RF_PROFILE_100K_LR_A == rxFrame.pReceiveFrame->profile) ||
       (RF_PROFILE_100K_LR_B == rxFrame.pReceiveFrame->profile))
  {
    pData[7]--;
    dataLength--;
  }
  rxFrame.pFrame = (frame *)pData;
  rxFrame.bTotalLength = dataLength;
  TransportSetCurrentRxChannel(pReceiveFrame->channelId);
  uint8_t curHeaderFormat = rxFrame.pReceiveFrame->channelHeaderFormat;
  if ((dataLength > sizeof(frameHeader)) // ToDo: Length check is now parsed in link layer, consider remove redundant check
      && (dataLength >= ((frame *)pData)->header.length))
  {
    /* frame length and checksum OK */
    DPRINT("x");
    rxFrame.status = 0;
    if (HDRFORMATTYP_3CH == curHeaderFormat)
    {
      mTransportRxCurrentSpeed = RF_OPTION_SPEED_100K;
    }
    else
    {
      switch (pReceiveFrame->profile)
      {
        case RF_PROFILE_100K_LR_A:
        case RF_PROFILE_100K_LR_B:
          mTransportRxCurrentSpeed = RF_OPTION_SPEED_LR_100K;
        break;
        case RF_PROFILE_40K:
        case RF_PROFILE_40K_WAKEUP_250:
        case RF_PROFILE_40K_WAKEUP_1000:
          mTransportRxCurrentSpeed = RF_OPTION_SPEED_40K;
        break;
        case RF_PROFILE_100K:
          mTransportRxCurrentSpeed = RF_OPTION_SPEED_100K;
        break;
        case PROFILE_UNSUPPORTED:
        case RF_PROFILE_9_6K:
        default:
          mTransportRxCurrentSpeed = RF_OPTION_SPEED_9600;
        break;
      }
    }

#if defined(USE_RESPONSEROUTE) || defined(ZW_CONTROLLER)
    /* Only allow RouteCacheUpdate functionality to cache Route ONCE. */
    RouteCachedReset();
#endif

    /* Network Wide Inclusion/Exclusion - Drop all Explore frame if an NOP, ASSIGN_ID, COMMAND_COMPLETE or a NOP_POWER frame */
    /* from THIS homeID is received regardless if we are being used as repeater or... */
    if ( (GetHeaderType(curHeaderFormat, (frame *)pData) ==  HDRTYP_SINGLECAST) || IsRouted(curHeaderFormat, (frame*)pData)  )
    {
      if ((IsRouted(curHeaderFormat, (frame*)pData)) && (!IsRouteValid(curHeaderFormat, (frame*)pData)))
      {
        return;
      }
      /* Setup protPayloadCmd to point at Cmd byte in Payload */
      if (IsRouted(curHeaderFormat, (frame*)pData))
      {
        protPayloadCmd = &GetRoutedPayload(curHeaderFormat, (frame*)pData)[1];
        protCmdClass = &GetRoutedPayload(curHeaderFormat, (frame*)pData)[0];
      }
      else
      {
        /* 3 Channel SINGLECAST_PAYLOAD needs pointer to Payload as parameter */
        protPayloadCmd = &GetSinglecastPayload(curHeaderFormat, rxFrame.pFrame)[1];
        protCmdClass = &GetSinglecastPayload(curHeaderFormat, rxFrame.pFrame)[0];
      }
      if (((*protPayloadCmd == ZWAVE_CMD_NOP) ||
           (*protPayloadCmd == ZWAVE_CMD_NOP_POWER) ||
           (*protPayloadCmd == ZWAVE_CMD_CMD_COMPLETE)) ||
          ((*protPayloadCmd == ZWAVE_CMD_ASSIGN_IDS) &&
           !memcmp(ZW_HomeIDGet(), (protPayloadCmd + 2), HOMEID_LENGTH)))
      {
        ExplorePurgeQueue(NODE_BROADCAST - 1);
      }
    }
    else if ( HDRTYP_EXPLORE == GetHeaderType(curHeaderFormat, (frame *)pData))
    {
       uint8_t tmpCmd;
      if (HDRFORMATTYP_3CH == curHeaderFormat)
      {
         tmpCmd = (rxFrame.pFrame->explore3ch.ver_Cmd & EXPLORE_CMD_MASK);
      }
      else
      {
         tmpCmd = (rxFrame.pFrame->explore.ver_Cmd & EXPLORE_CMD_MASK);
      }
      if (EXPLORE_CMD_NORMAL == tmpCmd)
      {
        protPayloadCmd = &GetExplorePayload(curHeaderFormat, rxFrame.pFrame)[1];
        protCmdClass = &GetExplorePayload(curHeaderFormat, rxFrame.pFrame)[1];
      }
    }
    else if (HDRTYP_MULTICAST == GetHeaderType(curHeaderFormat, (frame *)pData))
    {
      protPayloadCmd = &GetMulticastPayload(curHeaderFormat, rxFrame.pFrame)[1];
      protCmdClass = &GetMulticastPayload(curHeaderFormat, rxFrame.pFrame)[0];
    }

    if (IsFrameIlLegal(curHeaderFormat, rxFrame.pFrame, protCmdClass, protPayloadCmd))
    {
      return;
    }

    // Reset rxopt right before invocation of IsMyFrame() because this function will edit it.
    memset((uint8_t*)&rxopt, 0, sizeof(rxopt));
    myFrame = IsMyFrame(curHeaderFormat, &rxFrame, &rxopt.rxStatus);

    if (myFrame && (HDRFORMATTYP_LR == curHeaderFormat))
    {
      // Calculate the Optimal TX Power For Z-Wave Long Range
#ifdef ZW_SLAVE
      ProtocolZWaveLongRangeChannelSet(pReceiveFrame->channelId);

      if (zpal_radio_is_long_range_locked())
      {
        // Update TX power to retention RAM for Slave devices
        SaveTxPowerAndRSSI(pReceiveFrame->frameOptions.txPower, pReceiveFrame->rssi);
      }
#else // ZW_CONTROLLER
      if (zpal_radio_protocol_mode_supports_long_range(zpal_radio_get_protocol_mode()) && CtrlStorageLongRangeGet(pReceiveFrame->frameOptions.sourceNodeId))
      {
        // Calculate and update TX power to nodelist for Controllers
        // Call the algorithm, which will update the TXPower depending on the environment
	      int8_t modifiedTxPower = ZW_DynamicTxPowerAlgorithm(pReceiveFrame->frameOptions.txPower,
                                                        pReceiveFrame->rssi,
                                                        pReceiveFrame->frameOptions.noiseFloor,
                                                        NOT_RETRANSMISSION);
        SetTXPowerforLRNode(pReceiveFrame->frameOptions.sourceNodeId, modifiedTxPower);
      }
#endif
    }

#ifdef ZW_PROMISCUOUS_MODE
#else
    /*Drop an application frame if it is mine and the protocol to App task queue is full*/
    if (((rxFrame.status & (STATUS_NOT_MY_HOMEID | STATUS_ROUTE_ERR | STATUS_ROUTE_ACK |
                            STATUS_TRANSFERACK | STATUS_FRAME_IS_MINE)) ==  STATUS_FRAME_IS_MINE) &&
        (protCmdClass != NULL) && (ZWAVE_CMD_CLASS_APPL_MIN <= protCmdClass[0]) && ProtocolInterfaceFramesToAppQueueFull())
    {
      // Drop frame but send ack to avoid retransmissions that will stress the application even more
      myFrame = false;
      rxFrame.status &= ~STATUS_FRAME_IS_MINE;
    }
#endif
    /* This also is correct for routed frames as routed frames will always be send */
    /* with the same speed through all hops */
    gNodeLastReceivedFrom = GetSourceNodeID(curHeaderFormat, rxFrame.pFrame);

#ifdef ZW_SLAVE
    if ((HDRFORMATTYP_LR == curHeaderFormat) &&
        (NULL != rxFrame.pPayload) &&
        ZW_Drop_frame_with_lr_virtual_id(rxFrame.pPayload[0], gNodeLastReceivedFrom))
#else
    if ((HDRFORMATTYP_LR == curHeaderFormat) &&
        (NULL != rxFrame.pPayload) &&
        ZW_Drop_frame_with_lr_virtual_id(rxFrame.pPayload[0], GetDestinationNodeIDSinglecast(curHeaderFormat, rxFrame.pFrame)))
#endif
    {
      return;
    }
#ifdef ZW_SLAVE
    if (rxFrame.status & STATUS_FRAME_IS_MINE) /* Only update if frame is for me */
    {
      uint8_t IDX;
      uint8_t nextIndexToUse = iResponseNodeIndex;
      /* Check if node is already in the direct response table */
      for (IDX = 0; IDX < MAX_RESPONSE_SPEED_NODES; IDX++)
      {
        if (abDirectNodesResponseSpeed[IDX].wNodeID == gNodeLastReceivedFrom)
        {
          iResponseNodeIndex = IDX;
          break;
        }
      }
      if (IDX == MAX_RESPONSE_SPEED_NODES)
      {
        iResponseNodeIndex = nextIndexToUse;
      }

      /* Add/Update table with the speed */
      abDirectNodesResponseSpeed[iResponseNodeIndex].wNodeID = gNodeLastReceivedFrom;
      abDirectNodesResponseSpeed[iResponseNodeIndex].bResponseSpeed = mTransportRxCurrentSpeed;
      iResponseNodeIndex++;
      iResponseNodeIndex &= MAX_RESPONSE_SPEED_NODES - 1;
    }
#endif /* ZW_SLAVE */
#ifdef ZW_BEAM_RX_WAKEUP
     if (myFrame) {
       ResumeFragmentedBeamAckTx();
     }
#endif
    /* Do ack before checking if it is my frame, because routed frames should */
    /* also be acknowledged, but they are not for me */
    if ((rxFrame.status & STATUS_DO_ACK))
    {
      TxQueueElement *pACKFrame;
      pACKFrame = TxQueueGetFreeElement(TX_QUEUE_PRIORITY_HIGH, false);
      if (pACKFrame)
      {
#ifdef ZW_CONTROLLER_BRIDGE
        pACKFrame->frame.frameOptions.sourceNodeId = (!virtualNodeID && !virtualNodeDest) ? g_nodeID : virtualNodeID;
#endif
        uint8_t tmpFrameType;
        uint8_t tmpSeqNo;
        if (HDRFORMATTYP_3CH == rxFrame.pReceiveFrame->channelHeaderFormat)
        {
          tmpFrameType = FRAME_TYPE_ACK;
          tmpSeqNo = currentSeqNo;
        }
        else
        {
          tmpFrameType = FRAME_TYPE_ACK;
          tmpSeqNo = 0;
        }
#ifdef ZW_CONTROLLER_TEST_LIB
        if (HDRFORMATTYP_LR == rxFrame.pReceiveFrame->channelHeaderFormat) {
          pACKFrame->forceLR = true;
        }
#endif
        pACKFrame->frame.payloadLength = 0;
        BuildTxHeader(curHeaderFormat, tmpFrameType, tmpSeqNo, pACKFrame);
        if (HDRFORMATTYP_2CH == curHeaderFormat)
        {
          if (rxFrame.pReceiveFrame->frameOptions.speedModified)
          {
            pACKFrame->frame.frameOptions.speedModified = 1;
          }
        }

        pACKFrame->zcbp_InternalCallback =ZCB_ProtocolTransmitComplete;
        ZW_TransmitCallbackUnBind(&pACKFrame->AppCallback);
        pACKFrame->frame.frameOptions.destinationNodeId = rxFrame.ackNodeID;
        pACKFrame->frame.frameOptions.sequenceNumber    = currentSeqNo;
        SET_RF_OPTION_SPEED(pACKFrame->wRFoptions, mTransportRxCurrentSpeed);

        pACKFrame->frame.txPower = pReceiveFrame->frameOptions.txPower;

        if (IsLowPower(curHeaderFormat, rxFrame.pFrame))
        {
          pACKFrame->frame.frameOptions.lowPower = 1;
          pACKFrame->frame.txPower = ZPAL_RADIO_TX_POWER_REDUCED;
        }
        else if ((IS_PROTOCOL_CLASS(*rxFrame.pPayload)) &&
                 (((NOP_POWER_FRAME*)rxFrame.pPayload)->cmd == ZWAVE_CMD_NOP_POWER))
        {
          if (rxFrame.payloadLength == sizeof(NOP_POWER_FRAME))
          {
            /* It is a new CMD_NOP_POWER command, then set NOP power as Tx power level*/
            pACKFrame->frame.txPower -= ((NOP_POWER_FRAME*)rxFrame.pPayload)->parm2; /* Get the powerlevel */
          }
        }
        /*
         * Pass the RSSI value of the received frame onto the TX queue element so that it can
         * be added to the ACK frame.
         */
        pACKFrame->frame.rssi = pReceiveFrame->rssi;
        TxQueueQueueElement(pACKFrame);
      } /* if(pACKFrame) */
      else
      {
        // drop the frame
         myFrame = false;
         rxFrame.status = 0;
         return;
      }
    }

    if (myFrame)
    {
      /* The frame is for me */
#ifdef ZW_CONTROLLER
      badRoute.from = 0;
      badRoute.to = 0;
      if ((rxFrame.status & (STATUS_ROUTE_ERR | STATUS_TRANSFERACK)) == STATUS_ROUTE_ERR)
      {
        /* There was a route error, set from and to, to the values of the connection */
        /* that doesn't work */
        if (HDRFORMATTYP_3CH == curHeaderFormat)
        {
          routeStatus = rxFrame.pFrame->singlecastRouted3ch.routeStatus;
          numRepsNumHops = rxFrame.pFrame->singlecastRouted3ch.numRepsNumHops;
        }
        else
        {
          routeStatus = rxFrame.pFrame->singlecastRouted.routeStatus;
          numRepsNumHops = rxFrame.pFrame->singlecastRouted.numRepsNumHops;
        }
        hopNumber = routeStatus >> 4;
        badRoute.to = ((routeStatus & MASK_ROUT_ERR_HOP) == (numRepsNumHops & MASK_ROUT_REPTS)) ?
                                                             rxFrame.pFrame->header.sourceID :
                                                             ReadRepeater(curHeaderFormat, rxFrame.pFrame, hopNumber);
        hopNumber--;
        badRoute.from = ReadRepeater(curHeaderFormat, rxFrame.pFrame, hopNumber);

        /* BAD Route - try new route, if any, as soon as possible */
        if (waitingForRoutedACK)
        {
          /* Fix for TO#1050 */
          /* BAD Route - try new route, if any, as soon as possible */
          /* TO#1838 - Timeout when receiving routed error changed from 10ms to something more
                     intelligent */
          /* We wait <number of repeaters> * 160ms before we send next frame, to let things settle */
          pFrameWaitingForACK->bBadRouteFrom = badRoute.from;
          pFrameWaitingForACK->bBadRouteTo = badRoute.to;
          pFrameWaitingForACK->frame.frameOptions.acknowledge = 0;
          pFrameWaitingForACK->frame.frameOptions.routed = 0;
          TransmitTimerStart(ZCB_DelayedRetransmitFail,
                             (numRepsNumHops & 0xF0) * 10);
        }
      }
      else
#endif /* ZW_CONTROLLER */
#ifdef ZW_SLAVE_ROUTING
      if ((rxFrame.status & (STATUS_ROUTE_ERR | STATUS_TRANSFERACK)) == STATUS_ROUTE_ERR)
      {
        /* BAD Route - try new route, if any, as soon as possible */
        /* TO#3177 fix - Test (slaves) if node really are waiting */
        /* for an RoutedACK before starting a TransmitTimer. */
        if (waitingForRoutedACK)
        {
          /* We wait <number of repeaters> * 160ms before we send next frame, to let things settle */
          uint8_t delay_time;
          if (HDRFORMATTYP_3CH == curHeaderFormat)
          {
            delay_time = (pFrameWaitingForACK->frame.header.singlecastRouted3ch.numRepsNumHops & 0xF0) * 10;
          }
          else
          {
            delay_time = (pFrameWaitingForACK->frame.header.singlecastRouted.numRepsNumHops & 0xF0) * 10;
          }
          TransmitTimerStart(ZCB_DelayedRetransmitFail, delay_time);
        }
      }
      else
#endif  /* ZW_SLAVE_ROUTING */
      {
        if (rxFrame.status & STATUS_TRANSFERACK)
        {
          node_id_t TXNODE = 0;

          if ((pFrameWaitingForACK) && (pFrameWaitingForACK->bTxStatus == TX_QUEUE_STATUS_TRANSMITTING))
          {
            /*
            * Pending ACK received while a transmit is in progress.
            * Can happen if we have timed out while waiting for the ACK and triggered a re-transmission.
            *
            * Trying to handle an ACK at an unexpected point in time can lead to trouble. So just ignore
            * it and wait for the next ACK (or timeout) that will come in response to the re-transmitted
            * frame.
            */
            DPRINTF("\r\nDropping ACK received while TX in progress (p=0x%x)", pFrameWaitingForACK);
            return;
          }

          if (IsRouted(curHeaderFormat, (frame*)&pFrameWaitingForACK->frame.header))
          {
            uint8_t frameLen;
            uint8_t tNumRepeatersNumHops;
            uint8_t * pRepeaterList;
            if (HDRFORMATTYP_3CH == curHeaderFormat)
            {
              frameLen = (sizeof(frameHeaderSinglecast3ch) - sizeof(frameHeaderExtension3ch));
              tNumRepeatersNumHops = pFrameWaitingForACK->frame.header.singlecastRouted3ch.numRepsNumHops;
              pRepeaterList = pFrameWaitingForACK->frame.header.singlecastRouted3ch.repeaterList;
            }
            else
            {
              frameLen = sizeof(frameSinglecast);
              tNumRepeatersNumHops = pFrameWaitingForACK->frame.header.singlecastRouted.numRepsNumHops;
              pRepeaterList = pFrameWaitingForACK->frame.header.singlecastRouted.repeaterList;
            }
            if (GET_FRAME_LEN(pFrameWaitingForACK->frame)  > frameLen)
            {
              activeNumRepeatersNumHops = tNumRepeatersNumHops;
            }
            else
            {
              /* Abort, no frame waiting for ACK */
              return;
            }
              /* Get ack from previous hop */
            numHops = activeNumRepeatersNumHops & MASK_ROUT_HOPS;
#ifdef ZW_REPEATER
            /* If we are repeating then if numHops equals 0x0f, routed ACK has reached dest */
            if (numHops == 0x0F)
            {
              TXNODE = pFrameWaitingForACK->frame.frameOptions.destinationNodeId;
            }
            else
            {
#endif  /* ZW_REPEATER */
              if (GET_FRAME_LEN(pFrameWaitingForACK->frame) > frameLen)
              {
                TXNODE = pRepeaterList[numHops];
              }
              else
              {
                /* Abort, no frame waiting for ACK */
                return;
              }
#ifdef ZW_REPEATER
                /* If we are repeating then if numHops equals repeaters dest is reached */
              if (numHops == ((activeNumRepeatersNumHops & MASK_ROUT_REPTS) >> 4))
              {
                TXNODE = pFrameWaitingForACK->frame.frameOptions.destinationNodeId;
              }
            }
#endif  /* ZW_REPEATER */
            SetTransmitPause();

          }
          else if (IsSingleCast(curHeaderFormat, (frame*)&pFrameWaitingForACK->frame.header))
          {
            TXNODE = pFrameWaitingForACK->frame.frameOptions.destinationNodeId;
          }
          if (pFrameWaitingForACK != NULL &&
              ((GetSourceNodeID(curHeaderFormat, rxFrame.pFrame) == TXNODE) || sACK))
          {
#ifdef ZW_CONTROLLER
            /* TODO - SILENT ACK: as we when receiving a SILENT ACK do not receive */
            /* a direct frame it is not source in that frame, which should go into "most used update" */
            UpdateMostUsedNodes(true, TXNODE);
#endif
            /* TO#2535 fix update */
            /* TODO: Ugly workaround to be fixed properly */
            /* SendRouteAck() does not populate the routeStatus in txqueue, but the payload pointer instead. */
            /* Therefore we introduce the global isRoutedAckErr. */
            if (IsRouted(curHeaderFormat, rxFrame.pFrame))
            {
              isRoutedAckErr = IsRoutedAck(curHeaderFormat, rxFrame.pFrame);
            }

            // Prepare data for HandleTransferAck()
            int8_t bDestinationAckUsedTxPower = ZPAL_RADIO_RSSI_NOT_AVAILABLE;
            int8_t bDestinationAckMeasuredRSSI = ZPAL_RADIO_RSSI_NOT_AVAILABLE;
            int8_t bDestinationAckMeasuredNoiseFloor = ZPAL_RADIO_RSSI_NOT_AVAILABLE;
            if (HDRFORMATTYP_LR == curHeaderFormat)
            {
              // Those values are used for LR frames only
              bDestinationAckUsedTxPower = pReceiveFrame->frameOptions.txPower;
              bDestinationAckMeasuredRSSI = rxFrame.pFrame->transferACKLR.receiveRSSI;
              bDestinationAckMeasuredNoiseFloor = pReceiveFrame->frameOptions.noiseFloor;
              zpal_radio_rf_channel_statistic_end_device_rssi_average_update((zpal_radio_zwave_channel_t)pReceiveFrame->channelId, bDestinationAckMeasuredNoiseFloor);
            }
            HandleTransferAck(curHeaderFormat,
                              rssi,
                              bDestinationAckUsedTxPower,
                              bDestinationAckMeasuredRSSI,
                              bDestinationAckMeasuredNoiseFloor);
            isRoutedAckErr = 0;
          }
        }
        else if (rxFrame.status & STATUS_ROUTE_ACK)
        {
          /* Are we waiting for a Routed ACK */
          if (waitingForRoutedACK)
          {
#ifdef ZW_CONTROLLER
            if ((!rxFrame.pFrame->header.sourceID || (rxFrame.pFrame->header.sourceID == NODE_CONTROLLER_OLD)) &&
                ((pFrameWaitingForACK != NULL) && (pFrameWaitingForACK->frame.payload[1] == ZWAVE_CMD_ASSIGN_IDS)))
            {
              rxFrame.pFrame->header.sourceID = assign_ID.newNodeID;
            }
            /* Regardless of wheter we have room and if locked or not. */
            /* Store the route when we receive routed ack */
#endif  /* ZW_CONTROLLER */
#ifdef USE_RESPONSEROUTE
            /* TO#2760 fix */
            /* Update Return Route - Routing ACK received */
            UpdateResponseRouteLastReturnRoute(StoreRoute(curHeaderFormat, pData, &rxFrame.pReceiveFrame->frameOptions));
#else
#ifdef ZW_CONTROLLER
            if (HDRFORMATTYP_3CH == curHeaderFormat)
            {
#ifdef MULTIPLE_LWR
              LastWorkingRouteCacheLineUpdate(rxFrame.pFrame->header.sourceID, CACHED_ROUTE_ZW_LWR, pData);
#else
              LastWorkingRouteCacheLineUpdate(rxFrame.pFrame->header.sourceID, pData);
#endif
            }
            else
            {
              if (!IS_ROUTED_SPEED_MODIFIED(*(rxFrame.pFrame)))
              {
#ifdef MULTIPLE_LWR
                LastWorkingRouteCacheLineUpdate(rxFrame.pFrame->header.sourceID, CACHED_ROUTE_ZW_LWR, pData);
#else
                LastWorkingRouteCacheLineUpdate(rxFrame.pFrame->header.sourceID, pData);
#endif
              }
            }
#endif
#endif  /* USE_RESPONSEROUTE */

            HandleRoutedAck(&rxFrame, rssi);
          }
        }
        else
        {
          /* Not ROUTE_ERR, ROUTE_ACK or TRANSFER_ACK - then it must be a new frame for us */
          /* New Scheme - Also cache direct frames. */
          /* TO#1529 fix - Do not cache Broadcast or Multicast frames */
          /* Do not cache BROADCASTs or MULTICASTs */
          if (!IS_MULTICAST(*((frame*)pData)) &&
              (((HDRFORMATTYP_3CH == curHeaderFormat) && (rxFrame.pFrame->singlecast3ch.destinationID != NODE_BROADCAST)) ||
              ((HDRFORMATTYP_2CH == curHeaderFormat) && (rxFrame.pFrame->singlecast.destinationID != NODE_BROADCAST)) ||
              ((HDRFORMATTYP_LR == curHeaderFormat)  && (GET_SINGLECAST_DESTINATION_NODEID_LR(*rxFrame.pFrame) != NODE_BROADCAST_LR))))
          {
#ifdef ZW_BEAM_RX_WAKEUP
            /* Check if the frame is a multicast followup frame */
            if (rxFrame.pReceiveFrame->frameOptions.multicastfollowup)
            {
              ZW_FollowUpReceived();
            }
#endif
            /* New Routing Scheme */
            /* TO#2411 fix - Here we update lastworking route/Response routes as long its not a BROADCAST/MULTICAST */
            /* Regardless of wheter we have room and if locked or not. Store the route */
            /* Direct also are valid */
#ifdef USE_RESPONSEROUTE
            /* TO#2220 partial FEATURE - added the update of Return routes if */
            /* a response route is received. */
            /* We have received a frame - Store responseroute and update last */
            /* return route if sourceID match */
            /* Update Return Route - Routed/Direct frame received */
            UpdateResponseRouteLastReturnRoute(StoreRoute(curHeaderFormat, pData, &rxFrame.pReceiveFrame->frameOptions));
#else /* USE_RESPONSEROUTE */
#ifdef ZW_CONTROLLER
            if ((HDRFORMATTYP_3CH == curHeaderFormat)  || (!rxFrame.pReceiveFrame->frameOptions.speedModified && !IS_ROUTED_SPEED_MODIFIED(*(rxFrame.pFrame))))
            {
#ifdef MULTIPLE_LWR
              LastWorkingRouteCacheLineUpdate(rxFrame.pFrame->header.sourceID, CACHED_ROUTE_ZW_LWR, pData);
#else
              LastWorkingRouteCacheLineUpdate(rxFrame.pFrame->header.sourceID, pData);
#endif
            }
#endif  /* ZW_CONTROLLER */
#endif  /* USE_RESPONSEROUTE */
          }
          /* handle the payload data */
          if (rxFrame.status & STATUS_DO_ROUTING_ACK)
          {
            /* We need to send a routing acknowledge to the source */
            SendRouteAck(&rxFrame);
          }
          rxopt.rxStatus |= UpdateRxStatus();
          /* Call the Application */
          rxopt.sourceNode = GetSourceNodeID(curHeaderFormat, rxFrame.pFrame);
          uint32_t* pHomeId = (uint32_t *)rxFrame.pFrame->header.homeID; // Use pHomeId to avoid GCC type punned pointer error
          rxopt.homeId.word = *pHomeId;
#ifdef ZW_CONTROLLER_BRIDGE
          rxopt.destNode = ((rxopt.rxStatus & RECEIVE_STATUS_TYPE_MULTI) ? 0 : ((!virtualNodeID && !virtualNodeDest) ? g_nodeID : virtualNodeID));
#else
          rxopt.destNode = g_nodeID;
#endif  /* ZW_CONTROLLER */
          rxopt.rxRSSIVal = rssi;
          rxopt.rxChannelNo = pReceiveFrame->channelId;
          rxopt.isLongRangeChannel = (HDRFORMATTYP_LR == rxFrame.pReceiveFrame->channelHeaderFormat);
          if (HDRFORMATTYP_LR == curHeaderFormat)
          {
            // Those values are used for LR frames only
            rxopt.bSourceTxPower = pReceiveFrame->frameOptions.txPower;
            rxopt.bSourceNoiseFloor = pReceiveFrame->frameOptions.noiseFloor;
          }
          else
          {
            rxopt.bSourceTxPower = ZPAL_RADIO_RSSI_NOT_AVAILABLE;
            rxopt.bSourceNoiseFloor = ZPAL_RADIO_RSSI_NOT_AVAILABLE;
          }
          CommandHandler_arg_t args = {
            .cmd = rxFrame.pPayload,
            .cmdLength = rxFrame.payloadLength,
#if defined(ZW_CONTROLLER_BRIDGE)
            .multi = ((rxopt.rxStatus & RECEIVE_STATUS_TYPE_MULTI) ? &rxFrame.pFrame->multicast.addrOffsetNumMaskBytes : NULL),
#endif
            .rxOpt = &rxopt
          };
          CommandHandler(&args);
        }
      }
    } /* if (myframe) */
    else
    {
      if (pFrameWaitingForACK && TimerIsActive(&TransportTimer))
      {
        /* TO#2523 Fix */
        if (bRestartAckTimerAllowed)
        {
          RESTART_ACK_TIMER();
          bRestartAckTimerAllowed = 0;
        }
      }
    }
#ifdef ZW_REPEATER
    if (rxFrame.status & STATUS_ROUTE_FRAME)
    {
      uint8_t tHeaderLength = dataLength - rxFrame.payloadLength - CHECKSUM_LENGTH;
      TxQueueElement *pFrameToRepeat = TxQueueGetFreeElement(TX_QUEUE_PRIORITY_LOW, false);
      if (!pFrameToRepeat)
      {
        inclusionHomeIDActive = false;
        return;  /* cannot repeat - tx queue full */
      }
      /* Now inclusionHomeID is active */
      inclusionHomeIDActive = true;
      memcpy(inclusionHomeID, crH, HOMEID_LENGTH);

      uint8_t tmpSpeed = mTransportRxCurrentSpeed;

      pFrameToRepeat->bCallersStatus = TRANSFER_RETRIES;

      /* dataLength has already been adjusted for CRC16 extra byte, ok to use CHECKSUM_LENGTH=1
       * on both 3CH and 2CH systems */
      /* Copy frame header from rxFrame to pFrameToRepeat */
      safe_memcpy((uint8_t*)&pFrameToRepeat->frame.header,
                  (uint8_t*)&rxFrame.pFrame->header,
                  tHeaderLength,
                  sizeof(pFrameToRepeat->frame.header));
      /* Copy payload from rxFrame to pFrameToRepeat */
      pFrameToRepeat->frame.payloadLength = rxFrame.payloadLength;
      safe_memcpy(pFrameToRepeat->frame.payload,
                  rxFrame.pPayload,
                  pFrameToRepeat->frame.payloadLength,
                  sizeof(pFrameToRepeat->alloc));

      /* set hop pointer to next node */
      numHops1 = GetHops(curHeaderFormat, (frame*)&pFrameToRepeat->frame.header);
      if (NotOutgoing(curHeaderFormat, (frame*)&pFrameToRepeat->frame.header))
      {
        /* Incoming */
        numHops1--;
        /* Request ACK only if we are the last repeater in incoming direction
         * In all other cases, we will use silent ack. */
        if (0xFF == numHops1)
        {
          SetAck(curHeaderFormat, &pFrameToRepeat->frame.header);
          pFrameToRepeat->frame.frameOptions.acknowledge = 1;
        }
      }
      else
      {
        /* Are we the last repeater ? */
        if (numHops1 == (GetRouteLen(curHeaderFormat, (frame*)&pFrameToRepeat->frame.header) - 1))
        {
          if (HDRFORMATTYP_3CH == curHeaderFormat)
          {
            /* Get the beam speed */
            if (GET_ROUTING_DEST_WAKEUP_3CH(pFrameToRepeat->frame.header))
            {
              tmpSpeed |= RF_OPTION_SEND_BEAM_1000MS;
            }
          }
          else
          {
            /* We are the last repeater - lets check if destination needs a beam */
            if (GET_EXTEND_PRESENT(*rxFrame.pFrame) &&
                /* Is the Extend body of EXTEND_TYPE_WAKEUP_TYPE */
                (GET_EXTEND_TYPE(*rxFrame.pFrame) == EXTEND_TYPE_WAKEUP_TYPE) &&
                /* TO#2670 fix. Lets make sure that is DEST that needs a BEAM. */
                /* TO#2671 fix. Lets make sure that is DEST that needs a BEAM. */
                (GET_EXTEND_BODY(*rxFrame.pFrame, EXTEND_TYPE_WAKEUP_TYPE_SRC_DEST_OFFSET) & EXTEND_TYPE_WAKEUP_TYPE_DEST))
            {
              /* Set wakeup speed. */
              /* Is it a 250ms BEAM the destination needs. */
              if ((GET_EXTEND_BODY(*rxFrame.pFrame, EXTEND_TYPE_WAKEUP_TYPE_SRC_DEST_OFFSET) & EXTEND_TYPE_WAKEUP_TYPE_DEST) ==
                  EXTEND_TYPE_WAKEUP_TYPE_DEST_WAKEUP_250)
              {
                tmpSpeed |= RF_OPTION_SEND_BEAM_250MS;
              }
              /* TODO - Should we test for, if its a 1000ms BEAM the destination needs. */
              else
              {
                /* Assume 1000ms BEAM. */
                tmpSpeed |= RF_OPTION_SEND_BEAM_1000MS;
              }
            }
          }
        }
        numHops1++;
      }

      SetHops(curHeaderFormat, &pFrameToRepeat->frame.header, numHops1);

      if (IsRoutedAck(curHeaderFormat, (frame*)&pFrameToRepeat->frame.header) &&
          /* Check if we should add RSSI feedback to this routed ack. */
          /* For now, we only recognize the EXTEND_TYPE_RSSI extension if it comes first.
           * TODO: traverse multiple header extensions, looking for those we understand */
          /* TODO: (suggestion) If hdr is not present, we could insert it. */
          GetExtendPresent(curHeaderFormat, (frame*)&pFrameToRepeat->frame.header) &&
          /* Is the Extend body of EXTEND_TYPE_RSSI_INCOMING ? */
          /* Typecast FrameHeaderTx_t to frame is safe because they are defined identically.
           * We should really remove the redundant definitions and just have one. */
          (GetExtendType2(curHeaderFormat, &pFrameToRepeat->frame.header) == EXTEND_TYPE_RSSI_INCOMING))
      {
          UpdateRssiInRepeatedFrame(curHeaderFormat, &pFrameToRepeat->frame.header, rssi);
      }

      /* Tell EnqueueSingleDataPtr() and EnqueueCommon() not to mess with the
       * protocol header or payload pointer. We already updated them. */
      pFrameToRepeat->bFrameOptions1 |= TRANSMIT_FRAME_OPTION_NO_BUILD_TX_HDR;
      pFrameToRepeat->frame.frameOptions.extended = rxFrame.pReceiveFrame->frameOptions.extended;
      pFrameToRepeat->frame.frameOptions.frameType = rxFrame.pReceiveFrame->frameOptions.frameType;

      // copy constructed frame header to the payload and set pFrameHeaderStart to point to routedHeader start in reserved
      tHeaderLength = GetRouteLen(curHeaderFormat, (frame*)&pFrameToRepeat->frame.header);
      if (GetExtendPresent(curHeaderFormat, (frame*)&pFrameToRepeat->frame.header))
      {
        if (IsRoutedAck(curHeaderFormat, (frame*)&pFrameToRepeat->frame.header) && (HDRFORMATTYP_3CH == curHeaderFormat))
        {
          tHeaderLength += GetRoutedAckHeaderExtensionLen(curHeaderFormat, (frame*)&pFrameToRepeat->frame.header) + 3;
        }
        else
        {
          tHeaderLength += GetRoutedHeaderExtensionLen(curHeaderFormat, (frame*)&pFrameToRepeat->frame.header);
          if (HDRFORMATTYP_3CH == curHeaderFormat)
          {
            tHeaderLength +=4;
          }
          else
          {
            tHeaderLength += 3;
          }
        }
      }
      else
      {
        if (HDRFORMATTYP_2CH == curHeaderFormat)
        {
          tHeaderLength += 2;
        }
        else
        {
          if (IsRoutedAck(curHeaderFormat, (frame*)&pFrameToRepeat->frame.header))
          {
            tHeaderLength += 2;
          }
          else
          {
            tHeaderLength += 3;
          }
        }
      }
      pFrameToRepeat->frame.headerLength = tHeaderLength > sizeof(pFrameToRepeat->frame.header) ? sizeof(pFrameToRepeat->frame.header) : tHeaderLength;
      switch (curHeaderFormat)
      {
        case HDRFORMATTYP_2CH:
          pFrameToRepeat->frame.headerLength += sizeof(pFrameToRepeat->frame.header.singlecast);
          pFrameToRepeat->frame.header.singlecast.header.length = pFrameToRepeat->frame.headerLength + pFrameToRepeat->frame.payloadLength;
          break;
        case HDRFORMATTYP_3CH:
          pFrameToRepeat->frame.headerLength += (sizeof(pFrameToRepeat->frame.header.singlecast3ch) - sizeof(pFrameToRepeat->frame.header.singlecast3ch.extension));
          pFrameToRepeat->frame.header.singlecast3ch.header.length = pFrameToRepeat->frame.headerLength + pFrameToRepeat->frame.payloadLength;
          break;
        case HDRFORMATTYP_LR:
          pFrameToRepeat->frame.headerLength += (sizeof(pFrameToRepeat->frame.header.singlecastLR) - sizeof(pFrameToRepeat->frame.header.singlecastLR.extension));
          pFrameToRepeat->frame.header.singlecastLR.header.length = pFrameToRepeat->frame.headerLength + pFrameToRepeat->frame.payloadLength;
          break;
        case HDRFORMATTYP_UNDEFINED:
          break;
      }
      pFrameToRepeat->frame.frameOptions.sequenceNumber = GetSeqNumber(curHeaderFormat, (frame*)&pFrameToRepeat->frame.header);
      /* Copy payload from rxFrame to pFrameToRepeat */
      safe_memcpy((uint8_t* )&pFrameToRepeat->frame.payload,
                  rxFrame.pPayload,
                  pFrameToRepeat->frame.payloadLength,
                  sizeof(pFrameToRepeat->alloc));
      pFrameToRepeat->frame.frameOptions.destinationNodeId = rxFrame.pReceiveFrame->frameOptions.destinationNodeId;
      pFrameToRepeat->frame.frameOptions.sourceNodeId = rxFrame.pReceiveFrame->frameOptions.sourceNodeId;

      SetTransmitHomeID(pFrameToRepeat);
      const STransmitCallback TxCallback = { .pCallback = ZCB_ZWRepeatComplete, .Context = 0 };
      TxQueueInitOptions(pFrameToRepeat, (TRANSMIT_OPTION_ROUTED | TRANSMIT_OPTION_ACK));
      EnqueueSingleDataPtr(tmpSpeed,
                           pFrameToRepeat->frame.frameOptions.sourceNodeId,
                           pFrameToRepeat->frame.frameOptions.destinationNodeId,
                           pFrameToRepeat->frame.payload,
                           pFrameToRepeat->frame.payloadLength,
                           &TxCallback,
                           pFrameToRepeat);
    }
#endif  /* ZW_REPEATER */
  }
}


/*============================   TransportInit   ============================
**    Initializes transport layer data.
**
**    Side effects: none
**
**--------------------------------------------------------------------------*/
void TransportInit(zpal_radio_profile_t* const pRfProfile)
{
#if defined(ZW_SLAVE)
  FlushResponseSpeeds();
#endif /* defined(ZW_SLAVE) */
  bRoutedRssiFeedbackEnabled = true;

  ZwTimerRegister(&TransportTimer, false, NULL);
#ifdef ZW_SLAVE
  if (g_nodeID == 0)
  {
     SaveTxPowerAndRSSI(APP_DEFAULT_TX_POWER_LR, DYN_TXPOWER_ALG_MIN_RSSI);
  }
#endif
  ZW_ReturnCode_t eLinkLayerStatus = llInit(pRfProfile);
  ASSERT(SUCCESS == eLinkLayerStatus);
  rfTransportFilter_Set();
}


#ifdef ZW_CONTROLLER
/*==================   ChooseSpeedBeamForDestination_Common   ====================
**    Speed and beam selection logic invoked by EnqueueCommon() in in
**    controllers just before enquing singlecast or routed frames.
**
**    Side effects:
**--------------------------------------------------------------------------*/
uint8_t
ChooseSpeedBeamForDestination_Common( /* RET Chosen speed RF_OPTION_SPEED_* */
  node_id_t       bNodeID,            /* IN  Destination nodeID      */
  TxQueueElement *pFrame,             /* IN Frame to choose speed for */
  uint8_t         bFrameType)         /* IN Frametype. Singlecast, routed... */
{
  uint8_t sensorWakeupSpeed;
  uint8_t tmpSpeed;
  uint8_t loopcount;

  ZW_HeaderFormatType_t curHeaderFormat =  llGetCurrentHeaderFormat(bNodeID, false);

  if (HDRFORMATTYP_LR == curHeaderFormat)
  {
    tmpSpeed = RF_OPTION_SPEED_LR_100K;
    /* sensorWakeupSpeed is one of
     * #define ZWAVE_NODEINFO_SENSOR_MODE_WAKEUP_1000   0x40
     * #define ZWAVE_NODEINFO_SENSOR_MODE_WAKEUP_250    0x20
     */
    sensorWakeupSpeed = IsNodeSensor(bNodeID, true, true); /* sensorWakeupSpeed is wakeup speed */

    if (((bFrameType & MASK_FRAMETYPE_FRAMETYPE) == FRAME_TYPE_SINGLE) &&
        sensorWakeupSpeed)
    {
      /* Set nobeam flag if this is the first transmit to this destination. */
      if (pFrame->bRouteSchemeState == ZW_ROUTINGSCHEME_RESORT_DIRECT)
      {
        pFrame->bFrameOptions1 |= TRANSMIT_FRAME_OPTION_NOBEAM;
      }

      tmpSpeed = RF_OPTION_SPEED_LR_100K | RF_OPTION_SEND_BEAM_FRAG;
    }
  }
  else if (1 == llIsHeaderFormat3ch())
  {
    tmpSpeed = RF_OPTION_SPEED_100K;
    /* sensorWakeupSpeed is one of
     * #define ZWAVE_NODEINFO_SENSOR_MODE_WAKEUP_1000   0x40
     * #define ZWAVE_NODEINFO_SENSOR_MODE_WAKEUP_250    0x20
     */
    sensorWakeupSpeed = IsNodeSensor(bNodeID, true, true); /* sensorWakeupSpeed is wakeup speed */
    if (((bFrameType & MASK_FRAMETYPE_FRAMETYPE) == FRAME_TYPE_SINGLE) &&
        sensorWakeupSpeed)
    {
      /* Check if we are beaming, Set nobeam flag if we are because this is first transmit to
         this destination*/
      if (pFrame->bRouteSchemeState == ZW_ROUTINGSCHEME_CACHED_ROUTE
#ifndef ZW_CONTROLLER_STATIC
          || (pFrame->bRouteSchemeState == ZW_ROUTINGSCHEME_DIRECT)
#endif
        )
      {
        pFrame->bFrameOptions1 |= TRANSMIT_FRAME_OPTION_NOBEAM;
      }
      tmpSpeed = RF_OPTION_SPEED_100K |
          (((sensorWakeupSpeed & ZWAVE_NODEINFO_SENSOR_MODE_MASK)
                              == ZWAVE_NODEINFO_SENSOR_MODE_WAKEUP_250) ? RF_OPTION_SEND_BEAM_250MS : RF_OPTION_SEND_BEAM_1000MS);
    }
  }
  else  // Must be 2CH
  {
    if ((bNodeID != g_nodeID) && (g_nodeID != NODE_CONTROLLER_OLD) &&
        (bNodeID != NODE_CONTROLLER_OLD) && (ZCB_GetNodeType(bNodeID)))
    {
      /* TO2555 fix */
      tmpSpeed = MaxCommonSpeedSupported(bNodeID, g_nodeID);
/* sensorWakeupSpeed is one of
 * #define ZWAVE_NODEINFO_SENSOR_MODE_WAKEUP_1000   0x40
 * #define ZWAVE_NODEINFO_SENSOR_MODE_WAKEUP_250    0x20
 */
      sensorWakeupSpeed = IsNodeSensor(bNodeID, true, true); /* sensorWakeupSpeed is wakeup speed */
      if (((bFrameType & MASK_FRAMETYPE_FRAMETYPE) == FRAME_TYPE_SINGLE) &&
          sensorWakeupSpeed)
      {
        if (pFrame->bRouteSchemeState == ZW_ROUTINGSCHEME_CACHED_ROUTE
#ifndef ZW_CONTROLLER_STATIC
            || (pFrame->bRouteSchemeState == ZW_ROUTINGSCHEME_DIRECT)
#endif
          )
        {
          pFrame->bFrameOptions1 |= TRANSMIT_FRAME_OPTION_NOBEAM;
        }
        tmpSpeed = RF_OPTION_SPEED_40K |
            (((sensorWakeupSpeed & ZWAVE_NODEINFO_SENSOR_MODE_MASK)
                      == ZWAVE_NODEINFO_SENSOR_MODE_WAKEUP_250) ? RF_OPTION_SEND_BEAM_250MS : RF_OPTION_SEND_BEAM_1000MS);
      }
    }
    /* TO#1350 and TO#1351 fix : frames for node with same nodeID as this node are send with 9600 */
    else if ((bNodeID != g_nodeID) &&
             (bNodeID == gNodeLastReceivedFrom))
    {
      /* we use the detected speed if we cannot lookup the speed capabiliy */
      tmpSpeed = mTransportRxCurrentSpeed;
    }
    else
    {
      tmpSpeed = RF_OPTION_SPEED_9600;
    }
    if (((tmpSpeed & RF_OPTION_SPEED_MASK) != RF_OPTION_SPEED_9600)
        && ((bFrameType & MASK_FRAMETYPE_FRAMETYPE) == FRAME_TYPE_ROUTED))
    {
      /* search the hop nodes and if at least one of them does not support selected speed,*/
      /* then send in 9.6k.  */
      /* TODO: Choose lowest common speed instead of 9.6K*/
      /* TODO: Make this check in routing engine, and remove it here. */
      for (loopcount = 0; loopcount < GET_ROUTE_LEN(pFrame->frame.header); loopcount++)
      {
        if (!DoesNodeSupportSpeed(pFrame->frame.header.singlecastRouted.repeaterList[loopcount],
             tmpSpeed & RF_OPTION_SPEED_MASK))
        {
          tmpSpeed = RF_OPTION_SPEED_9600; /* TODO: Make sure this will never overwrite any beaming options */
          break;
        }
      }
    }
  }
  return tmpSpeed;
}
#endif /* ZW_CONTROLLER */


#ifdef ZW_SLAVE
/*============================   FlushResponseSpeeds   ===========================
**    Flush the stores response speeds.
**
**    Side effects:
**--------------------------------------------------------------------------*/
void
FlushResponseSpeeds(void)
{
  memset((uint8_t*)abDirectNodesResponseSpeed, 0, MAX_RESPONSE_SPEED_NODES*sizeof(SPEED_TABLE));
}
#endif


uint8_t
TransportGetCurrentRxChannel(void)
{
  return mTransportRxCurrentCh;
}


void
TransportSetCurrentRxChannel(
  uint8_t rfRxCurrentCh)
{
  mTransportRxCurrentCh = rfRxCurrentCh;
}


uint8_t
TransportGetCurrentRxSpeed(void)
{
  return mTransportRxCurrentSpeed;
}

void
TransportSetCurrentRxSpeedThroughProfile(uint32_t profile)
{
  switch (profile)
  {
    // Intentional drop through
    case RF_PROFILE_100K:
    case RF_PROFILE_3CH_100K:
    case RF_PROFILE_3CH_100K_CH_A:
    case RF_PROFILE_3CH_100K_CH_B:
    case RF_PROFILE_3CH_100K_CH_C:
      mTransportRxCurrentSpeed = RF_OPTION_SPEED_100K;
      break;

    default:
      mTransportRxCurrentSpeed = RF_OPTION_SPEED_40K;
      break;
  }
}

void
TransportSetCurrentRxSpeed(
  uint8_t rfRxCurrentSpeed)
{
  mTransportRxCurrentSpeed = rfRxCurrentSpeed;
}


/*==========================   TransportSetChannel   =========================
**    Set the RF channel the next frame should be send on
**
**    Side effects:
**--------------------------------------------------------------------------*/
void TransportSetChannel(uint8_t bChannel)
{
  mTransportTxCurrentCh = bChannel;
}


/*==========================   transportGetTxPower   =========================
**    Get the Tx power to be used with a given tx queue element
**
**    Side effects:
**--------------------------------------------------------------------------*/
int8_t
transportGetTxPower(TxQueueElement* element, ZW_HeaderFormatType_t headerFormat)
{
  int8_t txPower;

  // just to safeguard that Tx power is set correctly
  txPower = (ZPAL_RADIO_TX_POWER_UNINITIALIZED == element->frame.txPower ? ZPAL_RADIO_TX_POWER_DEFAULT : element->frame.txPower);

  //Never reduce Tx power for LongRange frames.
  if (HDRFORMATTYP_LR == headerFormat)
  {
    return txPower;
  }

  /* If frame is broadcast OR an explore and not a search result, then
   ** reduce Tx power.
   **/
  if ((((element->frame.frameOptions.frameType & MASK_HDRTYP) == HDRTYP_EXPLORE) &&
       ((element->frame.header.explore.ver_Cmd & EXPLORE_CMD_SEARCH_RESULT) == 0)) ||
     ((((element->bFrameOptions & TRANSMIT_OPTION_APPLICATION) == 0) &&
        (element->frame.frameOptions.destinationNodeId == NODE_BROADCAST)) &&
       ((element->frame.frameOptions.frameType & MASK_HDRTYP) != HDRTYP_TRANSFERACK)))
  {
    txPower = ZPAL_RADIO_TX_POWER_REDUCED;
  }

  return txPower;
}

/*==========================   TransportGetChannel   =========================
**    Return the RF channel the next frame should be send on
**
**    Side effects:
**--------------------------------------------------------------------------*/
uint8_t                              /* RET  Channel frame should be send on */
TransportGetChannel(
  TxQueueElement *pFrameToSend)   /* IN Pointer to frame */
{
  uint8_t bTempChannel;

  /* Default is to use current channel */
  bTempChannel = mTransportTxCurrentCh;
  if (1 == llIsHeaderFormat3ch())
  {
    /*Calculate the frame length */
    uint8_t len = pFrameToSend->frame.header.header.length;

    /*If this is a retrasmitted frame don't add the checksum*/
    if (!llIsReTransmitEnabled(&pFrameToSend->frame))
    {
      // Add the 2 byte crc16 to the length
      len += 2;
    }
    if (!zpal_radio_is_transmit_allowed(bTempChannel, len, 0))
    {
      bLastTxFailed = true;
    }
  }
  /* Dont change channel on beam ack */
  if (!((TX_QUEUE_PRIORITY_HIGH == pFrameToSend->bTxPriority)
      && (pFrameToSend->frame.header.transferACK3ch.destinationID == NODE_BROADCAST)))
  {
    /* If last transmit failed then choose another channel */
    if (bLastTxFailed)
    {
      do
      {
        bTempChannel = zpal_get_pseudo_random() % 3;
      } while (bTempChannel == mTransportTxCurrentCh);
    }
  }

  /* Save channel and clear tx failed status */
  mTransportTxCurrentCh = bTempChannel;

  bLastTxFailed = false;
  return bTempChannel;
}


/*===========================   transportVarInit   ==========================
**    Function description.
**      Initializes all global variables in transport module.
**
**    Side effects:
**     None
**--------------------------------------------------------------------------*/
void
transportVarInit(void)
{
  bApplicationTxAbort = false;
  inclusionHomeIDActive = false;
  currentSeqNoUseTX = true;

#if defined(ZW_CONTROLLER) || defined(ZW_SLAVE_ROUTING)
#ifdef ZW_RETURN_ROUTE_PRIORITY
  bReturnRoutePriorityIndex = 0;
#endif
#endif  /* ZW_CONTROLLER || ZW_SLAVE_ROUTING */
#ifdef ZW_SLAVE
  iResponseNodeIndex = false;
#endif  /* ZW_SLAVE */

#ifdef USE_TRANSPORT_SERVICE
  ts_stored_callback.pCallback = NULL;
  ts_stored_callback.Context = NULL;
#endif
}


#define TX_OPT_DEFAULT 0   /* TODO: What are sensible defaults here? */

#ifdef ZW_BEAM_RX_WAKEUP
static void ResumeFragmentedBeamAckTx(void)
{
  m_FragmentedBeamACKAllowed = true;
  if ((NULL != m_flirs_beam_ack_delay_timer.pCallback) &&
      TimerIsActive(&m_flirs_beam_ack_delay_timer)) {
    TimerStop(&m_flirs_beam_ack_delay_timer);
  }
}

static void FragmentedBeamAckDelayTimeout(__attribute__((unused)) SSwTimer * pTimer)
{
  m_FragmentedBeamACKAllowed = true;
}

bool
SendBeamAckAllowed(void)
{
  return m_FragmentedBeamACKAllowed;
}

static void ZCB_FragBeamACKTransmitComplete(TxQueueElement *pTransmittedFrame)
{
  /*if the bema ack is not transmited then allow sending beam ack gain, else resume beam ack sending after BEAM_TRAIN_DURATION_MS*/
  if (TX_QUEUE_PHY_TX_SUCCESS != pTransmittedFrame->bPhyTxStatus) {
    m_FragmentedBeamACKAllowed = true;
  } else {
    ZW_HeaderFormatType_t curHeaderFormat = llGetCurrentHeaderFormat(pTransmittedFrame->frame.frameOptions.sourceNodeId, false);
    if (HDRFORMATTYP_LR == curHeaderFormat) {
      TimerStart(&m_flirs_beam_ack_delay_timer, BEAM_TRAIN_DURATION_MS_USLR);
    } else {
      TimerStart(&m_flirs_beam_ack_delay_timer, BEAM_TRAIN_DURATION_MS);
    }
  }
  ZCB_ProtocolTransmitComplete(pTransmittedFrame);
}

void
SendFragmentedBeamACK(void)
{
  TxQueueElement *pACKFrame;
  if (NULL == m_flirs_beam_ack_delay_timer.pCallback) {
    ZwTimerRegister(&m_flirs_beam_ack_delay_timer, false, FragmentedBeamAckDelayTimeout);
  }
  pACKFrame = TxQueueGetFreeElement(TX_QUEUE_PRIORITY_HIGH, false);
  if (pACKFrame)
  {
    m_FragmentedBeamACKAllowed = false;
    /* ACK'ed beam is currently only for 3ch systems and long range.*/
    if (LOWEST_LONG_RANGE_NODE_ID <= g_nodeID)
    {
      pACKFrame->frame.txPower = zpal_radio_get_flirs_beam_tx_power();
      pACKFrame->frame.rssi = zpal_radio_get_last_beam_rssi();
      BuildTxHeader(HDRFORMATTYP_LR, FRAME_TYPE_ACK, currentSeqNo, pACKFrame);
      pACKFrame->frame.frameOptions.destinationNodeId = NODE_BROADCAST_LR;
        // Save Updated TX Power to Retention RAM for Slave devices
      SaveTxPowerAndRSSI(zpal_radio_get_flirs_beam_tx_power(), zpal_radio_get_last_beam_rssi());
    }
    else
    {
      BuildTxHeader(HDRFORMATTYP_3CH, FRAME_TYPE_ACK, currentSeqNo, pACKFrame);
      pACKFrame->frame.frameOptions.destinationNodeId = NODE_BROADCAST;
    }
    pACKFrame->wRFoptions |= RF_OPTION_BEAM_ACK;
    pACKFrame->frame.payloadLength = 0;
    pACKFrame->zcbp_InternalCallback = ZCB_FragBeamACKTransmitComplete;
    ZW_TransmitCallbackUnBind(&pACKFrame->AppCallback);
    TxQueueQueueElement(pACKFrame);
  } /* if(pACKFrame) */
  else
  {
    /* No room in txqueue for ACK. Give up. */
  }
}
#endif


#ifdef ZW_SECURITY_PROTOCOL
void
ZW_RxToTxOptions(
    RECEIVE_OPTIONS_TYPE *rxopt,     /* IN  receive options to convert */
    TRANSMIT_OPTIONS_TYPE *txopt)   /* OUT converted transmit options */
{
  memset((uint8_t*)txopt, 0, sizeof(TRANSMIT_OPTIONS_TYPE));
  txopt->destNode = rxopt->sourceNode;
  txopt->txOptions = TX_OPT_DEFAULT;
  txopt->securityKey = rxopt->securityKey;
}
#endif /* ZW_SECURITY_PROTOCOL */


/*==================   ZW_EnableRoutedRssiFeedback   =======================
**
**  Enable or disable collection of routed rssi feedback on frames sent
**  to this node.
**
**
**-------------------------------------------------------------------------*/
void
ZW_EnableRoutedRssiFeedback(
  uint8_t bEnabled)
{
  bRoutedRssiFeedbackEnabled = bEnabled;
}

#if defined(ZW_CONTROLLER) || defined(ZW_SLAVE_ROUTING)
static void ClearFrameOptions (ZW_TransmissionFrame_t * frame)
{
  frame->frameOptions.acknowledge = 0;
  frame->frameOptions.routed = 0;
}
#endif

/*
 * Maximum fragment size definitions.
 */
#define S2_ENCAPSULATION_LENGTH 12
#define S0_ENCAPSULATION_LENGTH 20

// Defines how many S0 frames a payload can be split into.
#define S0_SEQUENCING_FACTOR 2

uint16_t ZW_GetMaxPayloadSize(uint8_t keys)
{
  uint16_t maxPayloadSize;

  // Get payload size for LR
  if(zpal_radio_is_long_range_locked())
  {
    if (SECURITY_KEY_S2_MASK & keys)
    {
      // Long range included slaves need max singlecast payload - S2_ENCAPSULATION_LENGTH
      return MAX_SINGLECAST_PAYLOAD_LR - S2_ENCAPSULATION_LENGTH;
    }
    else
    {
      return 0;  // LR always uses S2 always
    }
  }

  // Get maximum payload size that includes all headers except security
  if (1 == llIsHeaderFormat3ch())
  {
    maxPayloadSize = MAX_EXPLORE_PAYLOAD_3CH;
  }
  else
  {
    maxPayloadSize = MAX_EXPLORE_PAYLOAD_LEGACY;
  }

  // Subtract security payload overhead length
  if (SECURITY_KEY_S2_MASK & keys)
  {
    return (maxPayloadSize - S2_ENCAPSULATION_LENGTH);
  }
  else if (SECURITY_KEY_S0_BIT & keys)
  {
    maxPayloadSize = ((maxPayloadSize - S0_ENCAPSULATION_LENGTH) * S0_SEQUENCING_FACTOR);
#ifdef ZW_SLAVE
    // Apply this part for slaves only because security is not part of the embedded controller.
    // Hence, it's the controller host's responsibility to use the correct, adjusted payload size.
    maxPayloadSize = ((maxPayloadSize > S0_MAX_ENCRYPTED_MSG_SIZE) ? S0_MAX_ENCRYPTED_MSG_SIZE : maxPayloadSize);
#endif
  }
  return (maxPayloadSize);
}

#ifdef ZW_CONTROLLER
uint16_t ZW_GetMaxLRPayloadSize(void)
{
  //Controllers need to know the total max payload including S2_ENCAPSULATION_LENGTH
  return MAX_SINGLECAST_PAYLOAD_LR;
}
#endif
