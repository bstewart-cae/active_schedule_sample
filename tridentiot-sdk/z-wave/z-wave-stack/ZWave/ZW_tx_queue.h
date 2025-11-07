// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file ZW_tx_queue.h
 * @copyright 2022 Silicon Laboratories Inc.
 *
 * @brief Z-Wave Protocol tx queue module.
 */
#ifndef _ZW_TX_QUEUE_H_
#define _ZW_TX_QUEUE_H_

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <ZW_typedefs.h>
#include <ZW_protocol.h>
#include <ZW_MAC.h>
#include "ZW_transport_transmit_cb.h"
#include <ZW_Frame.h>

/****************************************************************************/
/*                              CONFIGURATION                               */
/****************************************************************************/

/* Z-Wave internal transmit options */
#define TRANSMIT_MAX                            4     /* max. number of frames in the transmit queue */
#define TRANSMIT_OPTION_DELAYED_MAX_MS          1000  /* [ms] The maximum acceptable delay. */

#define BEAM_TRAIN_DURATION_MS      3000
#define BEAM_TRAIN_DURATION_MS_USLR 2000
/****************************************************************************/
/*                     EXPORTED TYPES and DEFINITIONS                       */
/****************************************************************************/

/* Transport routing scheme state define definitions */
/* 1 = direct, 2 = ApplicationStaticRoute, 3 = responseRoute/lastworkingRoute, */
/* 4 = Next to LastWorkingRoute(controller), 5 = returnRoute/controllerAutoRoute, 6 = directResort and 7 = explore */
typedef enum _E_ROUTING_SCHEME_T_
{
  ZW_ROUTINGSCHEME_IDLE = 0x00,
  ZW_ROUTINGSCHEME_DIRECT = 1,
  ZW_ROUTINGSCHEME_CACHED_ROUTE_SR = 2,
  ZW_ROUTINGSCHEME_CACHED_ROUTE = 3,
  ZW_ROUTINGSCHEME_CACHED_ROUTE_NLWR = 4,
  ZW_ROUTINGSCHEME_ROUTE = 5,
  ZW_ROUTINGSCHEME_RESORT_DIRECT = 6,
  ZW_ROUTINGSCHEME_RESORT_EXPLORE = 7
} E_ROUTING_SCHEME_T;

typedef enum {
  TIME_TYPE_NOT_USED,  ///< The time is not set and not in use.
  TIME_TYPE_RELATIVE,  ///< This indicates that the time in question in a relative time from a given starting point.
  TIME_TYPE_ABSOLUTE,  ///< This indicates that the time in question is in absolute value relative to some starting point.
} TimeType_t;

typedef struct{
  TimeType_t  timeType;     ///< Please @see TimeType_t
  uint32_t    delayedTxMs;  ///< [ms] The duration of the TX delaying. Used only when TRANSMIT_OPTION_DELAYED_TX is set.
} DelayedTx_t;

/* Transmit queue element */
typedef struct _TxQueueElement_
{
  uint8_t     bTxStatus:5;          /* Transmit  status (@see TxQueue_ElementState_t */
  uint8_t     bTxPriority:3;        /* Transmit priority (@see TxQueue_ElementPriority */
  uint8_t     bCallersStatus;       /* Status field for use by the function that queued the frame */
  TxOptions_t bFrameOptions;        /* Transmit option and status flags       */
  uint8_t     bFrameOptions1;       /* Transmit option and status flags byte 2*/ // Appears to be used for internal flags.
  uint16_t    wRFoptions;           /* Options passed to the RF layer, options stats with RF_OPTION (Z-Wave)
                                       or RF_OPTION1 (Z-WaveAV protocol) */
  uint16_t wRFoptionsBeforeModify;  /* Copy of RFoptions before they are modified */
  node_id_t  bBeamAddress;
  VOID_CALLBACKFUNC(zcbp_InternalCallback)(struct _TxQueueElement_*);     /* Transmit complete callback function    */
  STransmitCallback AppCallback;
  /* bRouteSchemeState - Routing Scheme State holder */
  E_ROUTING_SCHEME_T bRouteSchemeState; /* 1 = direct, 2 = ApplicationStaticRoute(controller), 3 = responseRoute/lastworkingRoute, */
  /* 4 = Next to LastWorkingRoute(controller), 5 = returnRoute/controllerAutoRoute, 6 = directResort and 7 = explore */
  uint8_t bPhyTxStatus;          /* Status of the transmitted frame from the phy layer */
  uint8_t bTxStatus_PrimaryChannel; /* This will store the result of the transmission attempt on the primary channel. Currently used in LR only. */
  uint8_t bPhyLBTAttempts;
  uint8_t bFLiRSCount;
  uint8_t bACKChannel;           /* Channel ACK frame received on */
  uint8_t bTxChannel;            /* Channel used in Last transmit */
  uint8_t bTransmitRouteCount;   /* Number of frame transmits including retries */
  uint8_t bBadRouteFrom;         /* If router received this is the failed link "From" node */
  uint8_t bBadRouteTo;           /* If router received this is the failed link "To" node */
  uint32_t StartTicks;
  uint8_t forceLR;
  DelayedTx_t delayedTx;         /* Data related to delayed transmission. */
  // New section begins.
  ZW_TransmissionFrame_t frame;
  // Allocate portion of memory for the payload contents (ZW_TransmissionFrame_t includes payload of 8 bytes)
  uint8_t        alloc[MAX_FRAME_LEN - TRANSMISSION_FRAME_PAYLOAD_SIZE];
} TxQueueElement;

/**************************************************************************************
 * Packet priority and status flags for the packets to be transmitted in the Tx Queue.
 *************************************************************************************/

/**
 * TODO remove the use of flags in both TxQueue_ElementPriority_t and TxQueue_ElementState_t. They should be values/states.
 *
 * The priority setting of the packets to be transmitted when placed in the Tx Queue.
 * These bits are cleared using TX_QUEUE_STATUS_FREE. (@see TxQueue_ElementState_t)
 */
typedef enum {
  TX_QUEUE_PRIORITY_UNDEF        = 0,   /* This value means that the transmit element is not in use. */
  TX_QUEUE_PRIORITY_HIGH         = 1,   /* Frames with this priority will be send first */
  TX_QUEUE_PRIORITY_LOW          = 2,   /* Frames with this priority will be send when all high priority frames has been send */
} TxQueue_ElementPriority_t;

/**
 * TODO remove the use of flags in both TxQueue_ElementPriority_t and TxQueue_ElementState_t. They should be values/states.
 *
 * These are the states of the elements in the TX-QUEUE.
 * The module will threat these elements one-by-one and alters their state individually.
 */
typedef enum {
  TX_QUEUE_STATUS_FREE            = 0,   /* This value means that the transmit element is not in use. */
  TX_QUEUE_STATUS_ALLOCATED       = 1,   /* The transmit queue element has been allocated but is not ready to be send */
  TX_QUEUE_STATUS_DELAYED_TX      = 2,   /* The transmit queue element has been marked for delayed transmission. */
  TX_QUEUE_STATUS_DELAYED_TX_WAIT = 3,   /* The transmit queue element is waiting for the TX delay timer to timeout. */
  TX_QUEUE_STATUS_READY_TO_SEND   = 4,   /* The frame is ready to send */
  TX_QUEUE_STATUS_TRANSMITTING    = 5,   /* The frame is currently being transmitted by the RF */
  TX_QUEUE_STATUS_WAITING         = 6,   /* Frames with this priority have already been sent and are awaiting further processing */
} TxQueue_ElementState_t;

/* Defines for bPhyTxStatus */
#define TX_QUEUE_PHY_TX_SUCCESS         0x00    /* The frame was transmitted by the PHY layer */
#define TX_QUEUE_PHY_TX_FAILED          0x01    /* The frame was not transmitted, PHY layer returned Tx fail */
#define TX_QUEUE_PHY_TX_LBT_FAILED      0x02    /* The frame was not transmitted because of to many LBT failures */
#define TX_QUEUE_PHY_TX_TOO_LARGE       0x03    /* The frame was not transmitted because it was to large for the chosen baud rate */

#define SET_RF_OPTION_SPEED(opt, sp) (opt = (opt & ~RF_OPTION_SPEED_MASK) | sp)

/* TODO: Check for exact starting addresses of TxQueue, not just NULL pointer */
#define IS_TXQ_POINTER(p) (NULL != (p))

struct sTxQueueEmptyEvent
{
  struct sTxQueueEmptyEvent *next;
  VOID_CALLBACKFUNC(EmptyEventCallback)(void);
};


/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/

/****************************************************************************/
/*                           EXPORTED FUNCTIONS                             */
/****************************************************************************/
/*===========================   TxQueueEmptyEvent_Add  =======================
**    Add a Callback to TxQueue Empty Event list (oneshot - only called once)
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
void
TxQueueEmptyEvent_Add(
  struct sTxQueueEmptyEvent *elm,
  VOID_CALLBACKFUNC(EmptyEventCallback)(void));

/*=============================   TxQueueReleaseElement  =========================
**    Release a Tx Queue element for use
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
void
TxQueueReleaseElement(TxQueueElement *pFreeTxElement);

/*=============================   TxQueueIsEmpty  =========================
**    Returns true if the transmit queue is empty
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
uint8_t
TxQueueIsEmpty(void);

/*==============================   TxQueueIsIdle  ============================
**    Returns true if the transmitter is not transmitting
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
uint8_t
TxQueueIsIdle(void);

/*=============================   TxQueueTransmit  =========================
**    Transmit the next frame in the queue
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
void
TxQueueServiceTransmit(void);

/*=============================   TxQueueRegisterPowerLocks   ===============================
**    Initializes Tx-Queue power locks.
**--------------------------------------------------------------------------*/
void TxQueueRegisterPowerLocks(void);

/*=============================   TxQueueInit   ===============================
**    Initializes the Tx-Queue module.
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
void
TxQueueInit(void);

/*=============================   TxQueueQueueElement  ========================
**    Insert an element in the transmit queue for transmission
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
void
TxQueueQueueElement(TxQueueElement *pNewTxElement);

/*=============================   TxQueueGetFreeElement   ====================
**    Get a pointer to the frame-header field in the next free tx buffer.
**
**    @param bPriority The priority of the transmission. @see TxQueue_ElementPriority_t.
**    @param delayedTx Whether the element will be a delayed transmission.
**
**    Returns: Pointer to free element, or NULL if none is available or the limit of delayed transmissions is met.
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
TxQueueElement*
TxQueueGetFreeElement(TxQueue_ElementPriority_t bPriority, bool delayedTx);

/*=========================   TxQueueStartTransmissionPause  =====================
**    Pauses Transmissions for a period.
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
void TxQueueStartTransmissionPause(uint32_t Timeout);

/*=========================   TxQueueTxComplete  =====================
**
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
void TxQueueTxComplete(ZW_Radio_Status_t radioStatus);

/*=========================   TxQueueTxBeamComplete  =====================
**   Send the Z-Wave frame after the beam fragments are sent
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
void TxQueueTxBeamComplete(void);

/**
 * Notify queue that we have received a Beam ACK,
 * @param source_node NodeId of the sending node
 *
 * @retval true if acknowledge determined to be a BEAM acknowledge
 */
bool TxQueueBeamACKReceived(node_id_t source_node, node_id_t destination_node);

/**
 * Get the bFrameOptions value of pElement
 * @param pElement Pointer to a TxQueueElement element
 *
 * @retval pElement->bFrameOptions
 */
TxOptions_t TxQueueGetOptions(const TxQueueElement *pElement);

/**
 * Set the bFrameOptions value of pElement to options
 * @param pElement Pointer to a TxQueueElement element
 * @param options  TX_OPTION flags to be set
 *
 */
void TxQueueInitOptions(TxQueueElement *pElement, TxOptions_t txOptions);

/**
 * Add options to bFrameOptions of pElement
 * @param pElement Pointer to a TxQueueElement element
 * @param options  TX_OPTION flags to be set
 *
 */
void TxQueueSetOptionFlags(TxQueueElement *pElement, TxOptions_t txOptions);

/**
 * Clear the bFrameOptions value of pElement
 * @param pElement Pointer to a TxQueueElement element
 * @param options  TX_OPTION flags to be cleared
 *
 */
void TxQueueClearOptionFlags(TxQueueElement *pElement, TxOptions_t txOptions);
#endif /* _ZW_TX_QUEUE_H_ */
