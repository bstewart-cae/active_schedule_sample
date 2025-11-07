// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file ZW_tx_queue.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include "ZW_lib_defines.h"

//#define DEBUGPRINT
#include <DebugPrint.h>

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <string.h>
#include <stdlib.h>
#include <ZW_typedefs.h>
#include <ZW_protocol.h>
#include <ZW_tx_queue.h>
#include <ZW_transport.h>
#include <ZW_timer.h>
#include <SwTimer.h>
#include <zpal_power_manager.h>
#include <TickTime.h>
#include <linked_list.h>
#include "ZW_DataLinkLayer_utils.h"
#include <zpal_radio.h>

_Static_assert(0 == TX_QUEUE_STATUS_FREE,
	       "error: TX_QUEUE_STATUS_FREE should remain 0 for memset");
_Static_assert(0 == TX_QUEUE_PRIORITY_UNDEF,
	       "error: TX_QUEUE_PRIORITY_UNDEF should remain 0 for memset");

/****************************************************************************/
/*                      PRIVATE TYPES and DEFINITIONS                       */
/****************************************************************************/

zpal_pm_handle_t tx_queue_power_lock;

#define FRAGMENTED_BEAM_PERIOD      200   // 200 ms steps (200ms is the time duration of one fragment and the time-space between fragments).
#define FRAGMENTS_PER_BEAM          (BEAM_TRAIN_DURATION_MS / FRAGMENTED_BEAM_PERIOD)
#define FRAGMENTS_PER_BEAM_USLR     (BEAM_TRAIN_DURATION_MS_USLR / FRAGMENTED_BEAM_PERIOD)

#define TX_LBT_TIME_DURATION        1200
static int beam_fragment_count = 0;

/* TO#5887 fix - For LOW_PRIORITY frame we need at least 2 free if no HIGH_PRIORITY in */
#define TXQUEUE_MIN_FREE_FOR_LOW_PRIORITY             2

// The maximum number of delayed transmissions allowed in the TxQueue.
#define TX_QUEUE_DELAYED_TX_COUNT_MAX                 (TRANSMIT_MAX - TXQUEUE_MIN_FREE_FOR_LOW_PRIORITY)

/****************************************************************************/
/*                              PRIVATE DATA                                */
/****************************************************************************/

typedef struct _LBT_STRUCT_
{
  uint32_t startTime;
  uint8_t isLBTRunning;
} LBT_STRUCT;

static LBT_STRUCT mLbt = { 0 };

/* The transmit queue */
static TxQueueElement m_TxQueue[TRANSMIT_MAX] = {{ 0 }};

/* Timer for transmit pause */
static SSwTimer m_TransmitPauseTimer = { 0 };

/* Timer for delayed transmissions. */
static SSwTimer m_TimerDelayedTX = { 0 };                        // The timer used to do the delaying of the TX.
static uint8_t  m_delayedTxCount = 0;                    // Used to count the number of delayed transmissions currently in the TxQueue.

/*
 * TODO If would be nicer to have an Abort function, like TxQeueueAbort(*element)  or TxQeueueAbortCurrent()
 */
extern bool bApplicationTxAbort;

/* The element in the Tx queue that is currently being transmitted */
static TxQueueElement* bCurrentTransmit = NULL;


/* TxQueue Empty event callback list */
LIST(txQueueEmptyEvent_list);


/****************************************************************************/
/*                             FORWARD DECLARATIONS                         */
/****************************************************************************/

static void ZCB_TransmitPauseTimerTimeout(SSwTimer* pTimer);
static void ZCB_DelayedTxTimerTimeout(SSwTimer* pTimer);

void TxQueueRegisterPowerLocks(void)
{
  tx_queue_power_lock = zpal_pm_register(ZPAL_PM_TYPE_USE_RADIO);
}

/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/

/* Holding place for a frame waiting for an ack */
TxQueueElement  *pFrameWaitingForACK = NULL;

/****************************************************************************/
/*                              EXPORTED FUNCTIONS                          */
/****************************************************************************/

/*=============================   TxQueueInit   =========================
**    Initialize the transmit queue
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
void
TxQueueInit(void)
{
  memset(m_TxQueue,0,sizeof(m_TxQueue));
  zpal_pm_cancel(tx_queue_power_lock);

  bCurrentTransmit = NULL;
  ZwTimerRegister(&m_TransmitPauseTimer, false, ZCB_TransmitPauseTimerTimeout);

  m_TimerDelayedTX.ptr = NULL;  // No TxQueueElement if currently set to be delayed.
  ZwTimerRegister(&m_TimerDelayedTX, false, ZCB_DelayedTxTimerTimeout);

  /* Initialize TxQueue Empty Event callback list */
  list_init(txQueueEmptyEvent_list);
}


TxQueueElement*
TxQueueGetFreeElement(
    TxQueue_ElementPriority_t bPriority,
    bool delayedTx)
{
  uint8_t bElement = 0xFF;
  uint8_t bFree = 0;
  uint8_t fHighPriorityQueued = false;

  // Check to see if maximum number of allowed delayed transmissions are consumed already.
  if ((m_delayedTxCount >= TX_QUEUE_DELAYED_TX_COUNT_MAX) && (delayedTx == true))
  {
    // The TxQueue has reached its limit in the number of delayed transmissions that it can hold at once.
    return NULL;
  }

  /* Find a free element and count the number of free elements */
  for (register uint8_t bCount = 0; bCount < TRANSMIT_MAX; bCount++)
  {
    if (m_TxQueue[bCount].bTxStatus == TX_QUEUE_STATUS_FREE)
    {
      if (bElement == 0xFF)
      {
        bElement = bCount;
      }
      bFree++;
    }
    else if (m_TxQueue[bCount].bTxPriority == TX_QUEUE_PRIORITY_HIGH)
    {
      fHighPriorityQueued = true;
    }
  }

  /* Check if there is at least one free element left if the frame isn't a high priority frame */
  /* unless there is already another high priority frame in the queue */
  /* TO#5887 fix - For LOW_PRIORITY frame we need at least 2 free if no HIGH_PRIORITY in */
  if ((bPriority != TX_QUEUE_PRIORITY_HIGH)
      && ((bFree < TXQUEUE_MIN_FREE_FOR_LOW_PRIORITY) && !fHighPriorityQueued))
  {
    bElement = 0xFF;
  }

  if (bElement < TRANSMIT_MAX)
  {
    /* Clear fields before use */
    memset(&m_TxQueue[bElement], 0, sizeof(m_TxQueue[bElement]));

    m_TxQueue[bElement].frame.txPower = ZPAL_RADIO_TX_POWER_UNINITIALIZED;

    m_TxQueue[bElement].bTxStatus = TX_QUEUE_STATUS_ALLOCATED;
    m_TxQueue[bElement].bTxPriority = bPriority;
    return (&m_TxQueue[bElement]);
  }

  return (NULL);
}


static void TxQueueDoTransmit(TxQueueElement* e, bool useLBT)
{
  CommunicationProfile_t profile;

  if (e->bFrameOptions1 & TRANSMIT_FRAME_OPTION_NOBEAM)
  {
    /* If nobeam option is set then remove beam speed from transmit */
    e->wRFoptions &= ~RF_OPTION_BEAM_MASK;
  }

  ZW_HeaderFormatType_t curHeader = llGetCurrentHeaderFormat(e->frame.frameOptions.destinationNodeId, e->forceLR);

  if (HDRFORMATTYP_3CH == curHeader)
  {
    uint8_t bChannel;

    /* Select transmit channel */
    if ((TX_QUEUE_PRIORITY_HIGH == e->bTxPriority)
        && (0 == e->bPhyLBTAttempts))
    {
      /* Ack frames should use same channel as received frame in the first attempt */
      bChannel = TransportGetCurrentRxChannel();
    }
    else
    {
      /* Not first ack attempt let transport layer determine channel */
      bChannel = TransportGetChannel(e);
    }

    if (bChannel >= 3)
    {
      DPRINTF("Invalid channel reported %i\n", bChannel);
      bChannel = 0;
    }
    /*
     * Choose the right transmit function for transmitting a frame with or without a beam requirement!
     */

    if (e->wRFoptions & RF_OPTION_BEAM_MASK)
    {
      profile = RF_PROFILE_3CH_100K_WAKEUP_FRAGMENTED_A + bChannel;
      /*
       * This must be a fragmented beam, set the fragment count.
       */
      if ((e->wRFoptions & RF_OPTION_FRAGMENTED_BEAM) == 0)
      {
        beam_fragment_count = FRAGMENTS_PER_BEAM - 1;
        e->wRFoptions |= RF_OPTION_FRAGMENTED_BEAM;
      }
    }
    else
    {
      profile = RF_PROFILE_3CH_100K_CH_A + bChannel;
    }
  }
  else if (HDRFORMATTYP_2CH == curHeader)
  {
    beam_fragment_count = 0;
    if (e->wRFoptions & RF_OPTION_SEND_BEAM_1000MS)
    {
      profile = RF_PROFILE_40K_WAKEUP_1000;
    }
    else if (e->wRFoptions & RF_OPTION_SEND_BEAM_250MS)
    {
      profile = RF_PROFILE_40K_WAKEUP_250;
    }
    else
    {
      switch((e->wRFoptions & RF_OPTION_SPEED_MASK)) {
        case RF_SPEED_100K:
          profile = RF_PROFILE_100K;
          break;
        case RF_SPEED_40K:
          profile = RF_PROFILE_40K;
          break;
        default:
          profile = RF_PROFILE_9_6K;
          break;
      }
    }
  }
  else if (HDRFORMATTYP_LR == curHeader)
  {
    /*
     * Choose the right transmit function for transmitting a frame with or without a beam requirement!
     */
    if (e->wRFoptions & RF_OPTION_BEAM_MASK)
    {
      if (ZPAL_RADIO_LR_CHANNEL_B == zpal_radio_get_primary_long_range_channel())
      {
        profile = RF_PROFILE_100K_LR_WAKEUP_FRAGMENTED_B;
      }
      else
      {
        profile = RF_PROFILE_100K_LR_WAKEUP_FRAGMENTED_A;
      }
      if ((zpal_radio_get_lr_channel_config()==ZPAL_RADIO_LR_CH_CFG3) && (0 != (e->bFrameOptions & TRANSMIT_OPTION_LR_SECONDARY_CH)))
      {
        profile = (RF_PROFILE_100K_LR_WAKEUP_FRAGMENTED_A == profile) ?
                  RF_PROFILE_100K_LR_WAKEUP_FRAGMENTED_B :
                  RF_PROFILE_100K_LR_WAKEUP_FRAGMENTED_A;
      }
      /*
       * This must be a fragmented beam, set the fragment count.
       */
      if ((e->wRFoptions & RF_OPTION_FRAGMENTED_BEAM) == 0)
      {
        beam_fragment_count = FRAGMENTS_PER_BEAM_USLR - 1;
        e->wRFoptions |= RF_OPTION_FRAGMENTED_BEAM;
      }
    }
    else  // Transmit without using beam
    {
      if (ZPAL_RADIO_LR_CHANNEL_B == zpal_radio_get_primary_long_range_channel())
      {
        profile = RF_PROFILE_100K_LR_B;
      }
      else
      {
        profile = RF_PROFILE_100K_LR_A;
      }
      if ((zpal_radio_get_lr_channel_config() == ZPAL_RADIO_LR_CH_CFG3) && (0 != (e->bFrameOptions & TRANSMIT_OPTION_LR_SECONDARY_CH)))
      {
        profile = (RF_PROFILE_100K_LR_A == profile) ?
                  RF_PROFILE_100K_LR_B :
                  RF_PROFILE_100K_LR_A;
      }
    }
  }
  else  // Must be HDRFORMATTYP_UNDEFINED
  {
    profile = PROFILE_UNSUPPORTED;  // Unknown header-type
    DPRINT("Frame header could not be defined: HDRFORMATTYP_UNDEFINED");
  }

  if (e->wRFoptions & RF_OPTION_BEAM_MASK)
  {
    e->bBeamAddress = e->frame.frameOptions.destinationNodeId;
  }

  if (((HDRFORMATTYP_3CH == curHeader) || (HDRFORMATTYP_LR == curHeader) ) &&
      (e->wRFoptions & RF_OPTION_BEAM_ACK))
  {
    useLBT = 1;
  }
  else if( e->frame.frameOptions.frameType == HDRTYP_TRANSFERACK)
  {
    useLBT = 0;
  }
  e->frame.useLBT = useLBT;

  if ((profile == PROFILE_UNSUPPORTED) && (HDRFORMATTYP_2CH == curHeader))
  {
    profile =  RF_PROFILE_40K;
  }

  e->frame.txPower = transportGetTxPower(e, curHeader);
  e->bTxChannel = llConvertTransmitProfileToPHYChannel(profile);
  bCurrentTransmit = e;
  if (SUCCESS == llTransmitFrame(profile, &(e->frame)))
  {
    DPRINTF("TxQueueDoTransmit OK (TxQueueElem p: %p)\n", e);
    //TODO the timer here should not be necessary.... Why not?
    zpal_pm_stay_awake(tx_queue_power_lock, 60000);
    /* Frame is now being transmitted, change status of the frame */
    e->bTxStatus = TX_QUEUE_STATUS_TRANSMITTING;
    /* Allow restart of ack wait timer */
    /* TO#2523 Fix */
    bRestartAckTimerAllowed = 1;
    /* Fake that we have transmitted the frame. */
    mLbt.isLBTRunning = useLBT;
  }
  else
  {
    DPRINTF("TxQueueDoTransmit %d FAIL\n", e);
    mLbt.isLBTRunning = false;
    TxQueueTxComplete(RADIO_STATUS_TX_FAIL);
  }
}


/**
 * @brief This function will calculate the time difference from current time to a future time.
 *
 * The function can handle the overflow of the timer value and fold the timer variables if needed.
 */
static uint32_t getCircularTimeDifference(uint32_t currentTickTime, uint32_t futureTimeMs)
{
  if (currentTickTime <= futureTimeMs)
  {
    return futureTimeMs - currentTickTime;  // This much time is remaining till me reach the future time.
  }
  else
  {
    /* First calculate the time untill overflow, and then add that time to the futureTimeMs which start from zero,
     * so the result is the combined waiting time until the future-time. */
    return (UINT32_MAX - currentTickTime) + futureTimeMs;
  }
}


/*
 * This function will manage the transmissions that are enqueued but set to be delayed.
 * This function will neglect all associations to transmission priority.
 * It will ASSERT in case it finds a high priority transmission that is delayed. (Seen as implementation issue)
 */
static void handleEnqueuedDelayedTransmissions(void)
{
  uint32_t pMsUntilTimeout;
  uint32_t currentTickTime;

  uint32_t shortestDelay = UINT32_MAX;  // Initialize to invalid delay to detect change.
  TxQueueElement *nextDelayedElementToTransmit = NULL;

  /*
   * The priority of the transmissions are being neglected!
   * A delayed high priority transmission makes no sense.
   */

  // Search for enqueued delayed transmissions in the queue and set or modify the timer.
  for (register uint8_t bCount = 0; bCount < TRANSMIT_MAX; bCount++)
  {
    /* First check for high priority frames */
    if (m_TxQueue[bCount].bTxStatus == TX_QUEUE_STATUS_DELAYED_TX)
    {
      ASSERT(m_TxQueue[bCount].bTxPriority == TX_QUEUE_PRIORITY_LOW);  // Only low priority TX are permitted!
      ASSERT(m_TxQueue[bCount].delayedTx.timeType == TIME_TYPE_ABSOLUTE);      // Run-time implementation check

      currentTickTime = getTickTime();

      // Search for the first element that needs transmission based on its set delay.
      if (getCircularTimeDifference(currentTickTime, m_TxQueue[bCount].delayedTx.delayedTxMs) < shortestDelay)
      {
        shortestDelay = getCircularTimeDifference(currentTickTime, m_TxQueue[bCount].delayedTx.delayedTxMs);
        nextDelayedElementToTransmit = &m_TxQueue[bCount];  // Keep the element for further processing.
      }
    }
  }

  /**
   * NOTE: Due to the fact that we are only queuing the elements for transmission and are not performing the TX,
   * and that the queuing is done by the RTOS timer task, we cannot miss a deadline and overshoot the delay set for
   * the transmission.
   * Therefore, it is not handled!
   */

  if (shortestDelay != UINT32_MAX)  // The maximum allowed delay is limited with TRANSMIT_OPTION_DELAYED_MAX_MS.
  {
    // Is the timer busy with another element in the queue?
    if (TimerIsActive(&m_TimerDelayedTX) && m_TimerDelayedTX.ptr != nextDelayedElementToTransmit)
    {
      ASSERT(0);  // Run-time test: The element currently holding the timer busy is not in the queue anymore, or calculation issue!
    }
    else if (TimerIsActive(&m_TimerDelayedTX) && m_TimerDelayedTX.ptr == nextDelayedElementToTransmit)
    {
      // Integrity check: (TODO convert to debug code, currently not supported)

      // Check to see if the delays still match.
      TimerGetMsUntilTimeout(&m_TimerDelayedTX, currentTickTime, &pMsUntilTimeout);
      // The delay in shortestDelay must belong to the currently active element that keeps the timer busy.
      // No need to do anything.
    }
    else
    {
      nextDelayedElementToTransmit->bTxStatus = TX_QUEUE_STATUS_DELAYED_TX_WAIT;
      m_TimerDelayedTX.ptr = nextDelayedElementToTransmit;
      TimerStart(&m_TimerDelayedTX, shortestDelay);
      m_delayedTxCount++;
    }
  }
  else
  {
    // None of the queued elements are set to be transmitted with a delay! Do nothing.
  }
}


static uint8_t checkForQueuedHighPriorityPackets(void)
{
  register uint8_t bElement = 0xFF;         // Invalid TxQueueElement Index.

  if ((NULL == bCurrentTransmit) || (bCurrentTransmit->bTxStatus != TX_QUEUE_STATUS_TRANSMITTING))
  {
    /* Find the next element in the queue to transmit */
    for (register uint8_t bCount = 0; bCount < TRANSMIT_MAX; bCount++)
    {
      /* First check for high priority frames */
      if (m_TxQueue[bCount].bTxPriority == TX_QUEUE_PRIORITY_HIGH && m_TxQueue[bCount].bTxStatus ==  TX_QUEUE_STATUS_READY_TO_SEND)
      {
        bElement = bCount;
        /* High priority element found, no need to continue */
        break;
      }

      // Run-time error check (In case of wrong flag setting, the above check wont catch the condition below)
      ASSERT( !((m_TxQueue[bCount].bTxPriority == TX_QUEUE_PRIORITY_HIGH)
        && (m_TxQueue[bCount].bTxStatus == TX_QUEUE_STATUS_READY_TO_SEND)) );
    }
  }
  return bElement;
}

static uint8_t checkForQueuedWaitingAndLowPriorityPackets(void)
{
  register uint8_t bElement = 0xFF;         // Invalid TxQueueElement Index.

  if ((NULL == bCurrentTransmit) || (bCurrentTransmit->bTxStatus != TX_QUEUE_STATUS_TRANSMITTING))
  {
    /* Find the next element in the queue to transmit */
    for (register uint8_t bCount = 0; bCount < TRANSMIT_MAX; bCount++)
    {
      if (m_TxQueue[bCount].bTxStatus == TX_QUEUE_STATUS_WAITING)
      {
        DPRINT("TxQueueServiceTransmit waiting for ack\n");

        /* If there is a waiting element then DON'T send any low priority frames */

        /* TODO - Here we could implement a Global timeout - NO TxElement must be in Waiting for longer than... */
        /* Just make use of the Global 10ms/1ms Ticker. */

        // We have a waiting packet for further processing. No low priority packet should be transmitted at this time.
        bElement = 0xFF;  // Do not make a new transmission.
        break;
      }
      else if (bElement == 0xFF && (m_TxQueue[bCount].bTxPriority == TX_QUEUE_PRIORITY_LOW && m_TxQueue[bCount].bTxStatus == TX_QUEUE_STATUS_READY_TO_SEND))
      {
        /* No high priority and no waiting, send low priority */
        bElement = bCount;  // Do transmission
      }
    }
  }
  return bElement;
}

/*==========================   TxQueueServiceTransmit  ======================
**    Transmit the next frame in the queue
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
void
TxQueueServiceTransmit(void)
{
  register uint8_t bElement = 0xFF;         // Invalid TxQueueElement Index.

  DPRINTF("TxQueueServiceTransmit - bTxPriority: %d bTxStatus: %d\n", (NULL == bCurrentTransmit) ? 0 : bCurrentTransmit->bTxPriority, (NULL == bCurrentTransmit) ? 0 : bCurrentTransmit->bTxStatus);
  if (TimerIsActive(&m_TransmitPauseTimer))
  {
    DPRINT("TxQueueServiceTransmit pause active\n");
    return;
  }

  // Regardless of whether we are busy transmitting, handle the newly enqueued DELAYED transmissions in parallel, if any.
  handleEnqueuedDelayedTransmissions();

  /*
   * TODO: We should make sure that frames do NOT change place in queue - as we now always
   * start search for next transmit from index ZERO - we have no in/out pointers...
   *
   * IDEA: A simple fix would be to implement an internal sequence number which is generated
   * when a new TxQueueElement is requested (so that when TxQueueElement are reused the Sequence number
   * do not change). This Sequence number is then used to select next TxQueueElement to transmit.
   */

  // HANDLE "HIGH" PRIORITY TRANSMISSIONS !
  /* Check if we are already transmitting and if not, pick the next HIGH PRIORITY frame in the queue for TX. */
  bElement = checkForQueuedHighPriorityPackets();
  if (bElement == 0xFF)
  {
    // HANDLE "LOW" PRIORITY TRANSMISSIONS !
    /* Check if we are already transmitting and if not, pick the next LOW PRIORITY frame in the queue for TX. */
    bElement = checkForQueuedWaitingAndLowPriorityPackets();
  }

  /* Transmit a packet, if any packet were found that is ready for transmission. */
  if (bElement != 0xFF)
  {
    TxQueueDoTransmit( &m_TxQueue[bElement] , true);
    mLbt.startTime = getTickTime();
  }
}


static bool
txQueueHandleTxFailLBT(void)
{
  ASSERT_PTR(bCurrentTransmit);
  bCurrentTransmit->bPhyLBTAttempts++;
  volatile uint32_t timePassed = getTickTimePassed(mLbt.startTime);
  if ((mLbt.isLBTRunning) && (TX_LBT_TIME_DURATION > timePassed))
  {
    if (0 == (bCurrentTransmit->wRFoptions & RF_OPTION_BEAM_MASK))
    {
      llReTransmitStart(&bCurrentTransmit->frame);
    }
    TxQueueDoTransmit(bCurrentTransmit, true);
    return true;
  }
  return false;
}


void
TxQueueTxComplete(ZW_Radio_Status_t radioStatus)
{
  DPRINT("TxQueueTxComplete\n");
  /* Check for transmit complete */
  if (RADIO_STATUS_TX_COMPLETE == radioStatus)
  {
    /* Tx completed */
    ASSERT_PTR(bCurrentTransmit);
    bCurrentTransmit->bPhyTxStatus = TX_QUEUE_PHY_TX_SUCCESS;
    zpal_radio_rf_channel_statistic_tx_frames();
    /* Change the state of the frame if any */
    if (bCurrentTransmit->bTxStatus == TX_QUEUE_STATUS_TRANSMITTING)
    {
      DPRINT("Set to TX_QUEUE_STATUS_WAITING\n");
      bCurrentTransmit->bTxStatus = TX_QUEUE_STATUS_WAITING;
      /* TODO: Global TxQueueElement timeout - sample Global timer tick here. */
      /* Notify the transport layer */
      if (bCurrentTransmit->zcbp_InternalCallback)
      {
        bCurrentTransmit->zcbp_InternalCallback(bCurrentTransmit);
      }
    }
  }

  if ((RADIO_STATUS_TX_FAIL == radioStatus) || (RADIO_STATUS_TX_FAIL_LBT == radioStatus))
  {
    // We flag this for upper layers so they will choose another channel on next transmit.
    bLastTxFailed = true;
    // Failed transmit - try again if applicable
    ASSERT_PTR(bCurrentTransmit);
    bCurrentTransmit->bPhyTxStatus = TX_QUEUE_PHY_TX_FAILED;
    if (RADIO_STATUS_TX_FAIL_LBT == radioStatus)
    {
      // CCA has failed
      bCurrentTransmit->bPhyTxStatus = TX_QUEUE_PHY_TX_LBT_FAILED;
    }
    bool txFailLBTHandlingDone = txQueueHandleTxFailLBT();
    if (true == txFailLBTHandlingDone)
    {
      return;  // Continue with CSMA and do not set the transmission as being done yet.
    }
    ASSERT_PTR(bCurrentTransmit);
    bCurrentTransmit->bTxStatus = TX_QUEUE_STATUS_FREE;
    /* Notify the transport layer */
    if (bCurrentTransmit->zcbp_InternalCallback)
    {
      bCurrentTransmit->zcbp_InternalCallback(bCurrentTransmit);
    }
  }
}

/*===========================   TxQueueEmptyEvent_Add  =======================
**    Add a Callback to TxQueue Empty Event list (oneshot - only called once)
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
void
TxQueueEmptyEvent_Add(
  struct sTxQueueEmptyEvent *elm,
  VOID_CALLBACKFUNC(EmptyEventCallback)(void))
{
  elm->EmptyEventCallback = EmptyEventCallback;
  list_add(txQueueEmptyEvent_list, elm);
}


/*=============================   TxQueueIsEmpty  =========================
**    Returns true if the transmit queue is empty
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
uint8_t
TxQueueIsEmpty(void)
{
  for (register uint8_t bCount = 0; bCount < TRANSMIT_MAX; bCount++)
  {
    if (m_TxQueue[bCount].bTxStatus != TX_QUEUE_STATUS_FREE)
    {
      return false;
    }
  }
  return true;
}


/*==============================   TxQueueIsIdle  ============================
**    Returns true if the transmitter is not transmitting
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
uint8_t
TxQueueIsIdle(void)
{
  /* TxQueueIsIdle is called from a non task when starting up. */
  for (register uint8_t bCount = 0; bCount < TRANSMIT_MAX; bCount++)
  {
    if (m_TxQueue[bCount].bTxStatus == TX_QUEUE_STATUS_TRANSMITTING)
    {
      return false;
    }
  }
  return true;
}


/*=============================   TxQueueReleaseElement  =========================
**    Release a Tx Queue element for use
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
void
TxQueueReleaseElement(
  TxQueueElement *pFreeTxElement)
{
  /* Free the queue element */
  DPRINTF("TxQueue Release %p\n", pFreeTxElement);
  pFreeTxElement->bTxStatus = TX_QUEUE_STATUS_FREE;
  pFreeTxElement->bTxPriority = TX_QUEUE_PRIORITY_UNDEF;
  llReTransmitStop(&pFreeTxElement->frame);

  if (true == TxQueueIsEmpty())
  {
    zpal_pm_cancel(tx_queue_power_lock);
    // Check if TxQueeue still empty as callbacks are being called
    for (struct sTxQueueEmptyEvent *elm = list_head(txQueueEmptyEvent_list); ((elm != NULL) && (true == TxQueueIsEmpty())); elm = elm->next)
    {
      list_remove(txQueueEmptyEvent_list, elm);
      /* call the TxQueue Empty Event oneshot callback function */
      if (elm->EmptyEventCallback != NULL)
      {
        elm->EmptyEventCallback();
      }
    }
  }
  // Check if anything needs transmitting - A waiting packet may just have been cleared
  TxQueueServiceTransmit();
}


/*=============================   TxQueueQueueElement  =========================
**    Insert an element in the transmit queue for transmission.
**
**    This function also used internally by the delayed transmission timer to
**    Re-Queue the element as READY TO SEND.
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
void
TxQueueQueueElement(
  TxQueueElement *pNewTxElement)
{
  // Handle the delayed transmission transmit-option.
  if (TxQueueGetOptions(pNewTxElement) & TRANSMIT_OPTION_DELAYED_TX)
  {
    /* Set the element ready for delayed transmit */

    // TODO: Convert to debug code
    // Check to see if maximum number of allowed delayed transmissions are consumed already.
    if (m_delayedTxCount >= TX_QUEUE_DELAYED_TX_COUNT_MAX)
    {
      /*
       * The problem might be that TxQueueGetFreeElement() was not called with the indication
       * that the element was needed for a delayed transmission to receive an error due to limit.
       */
      ASSERT(0);  // The maximum number of delayed TX elements are exceeded.
    }

    // Convert the relative delay into an absolute delay for internal use in the TXQueue module.
    pNewTxElement->delayedTx.timeType = TIME_TYPE_ABSOLUTE;
    // This will overflow and it is intentional!
    pNewTxElement->delayedTx.delayedTxMs += getTickTime();  // getTickTime() must always return uint32_t!

    // Explicitly set the TX_QUEUE_STATUS_DELAYED_TX flag in the TxQueueElement.
    pNewTxElement->bTxStatus = TX_QUEUE_STATUS_DELAYED_TX;
  }
  else
  {
    /* Set the element ready to send */
    pNewTxElement->bTxStatus = TX_QUEUE_STATUS_READY_TO_SEND;
  }

  pNewTxElement->bTransmitRouteCount++;
  // Check if anything needs transmitting
  TxQueueServiceTransmit();  // Run the transmission state machine.
}

/*=========================   TxQueueStartTransmissionPause  =====================
**    Pauses Transmissions for a period.
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
void TxQueueStartTransmissionPause(uint32_t Timeout)
{
  TimerStart(&m_TransmitPauseTimer, Timeout);
}


/****************************************************************************/
/*                              PRIVATE FUNCTIONS                           */
/****************************************************************************/

/*=========================   ZCB_TransmitPauseTimerTimeout  =====================
**    Callback for Transmission pause timer timeout.
**    Ensures any pending transmissions are sent
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
static void ZCB_TransmitPauseTimerTimeout(__attribute__((unused)) SSwTimer* pTimer)
{
  DPRINT("ZCB_TransmitPauseTimerTimeout() \n");
  TxQueueServiceTransmit();
}

/*=========================   ZCB_TransmitPauseTimerTimeout  =====================
**    Callback for delayed transmission timer timeout.
**    Makes the TxQueueElement ready for immediate transmission.
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
static void ZCB_DelayedTxTimerTimeout(SSwTimer* pTimer)
{
  DPRINT("ZCB_DelayedTxTimerTimeout() \n");

  TxQueueElement *txQueueElem = pTimer->ptr;

  // Clear the timer's pointer to txQueueElement so that it appears not in use.
  pTimer->ptr = NULL;

  /* Clear the Delayed transmission flag to disable all branches and ASSERTs related to delayed transmission in
   * TxQueueQueueElement(). */
  TxQueueClearOptionFlags(txQueueElem, TRANSMIT_OPTION_DELAYED_TX);

  // Indicate to the module that an additional delayed transmissions are accepted.
  m_delayedTxCount--;
  ASSERT(m_delayedTxCount != 255);     // Just checking to see if we ever decrement a 0.

  TxQueueQueueElement(txQueueElem);    // Re-enqueue the element, which will set the STATUS to ready to send.

  // The state machine (TxQueueServiceTransmit()) will be run by TxQueueQueueElement().
  // If there is any need, this timer will be re-started.
}

bool
TxQueueBeamACKReceived(
  node_id_t source_node,
  node_id_t destination_node)
{
  DPRINTF("TxBeamAck Rx %d\n", bCurrentTransmit);
  bool broadCastAckReceived = false;
  if (!beam_fragment_count) {
    return false;
  }
  node_id_t broadCast_id = (ZPAL_RADIO_PROTOCOL_MODE_2 == zpal_radio_get_protocol_mode())?NODE_BROADCAST:NODE_BROADCAST_LR;

#ifdef ZW_CONTROLLER
  if ((destination_node == bCurrentTransmit->bBeamAddress) &&
      (broadCast_id == destination_node) &&
      ((TxQueueElement*) ASSERT_PTR(bCurrentTransmit))->bFLiRSCount &&
      ZW_broadcast_beam_ack_id_update(source_node)) {
    if (0 == --bCurrentTransmit->bFLiRSCount) {
      broadCastAckReceived = true;
    }
  }
#endif
#ifdef ZW_SLAVE
  if ((destination_node == bCurrentTransmit->bBeamAddress) && (broadCast_id == destination_node) ) {
     broadCastAckReceived = true;
  }
#endif
  ASSERT_PTR(bCurrentTransmit);
  if ((source_node == bCurrentTransmit->bBeamAddress) || broadCastAckReceived)
  {
    beam_fragment_count = 0;
    if (TimerIsActive(&m_TransmitPauseTimer))
    {
      TimerStop(&m_TransmitPauseTimer);
    }
    uint16_t u16RFoptions = bCurrentTransmit->wRFoptions;
    bCurrentTransmit->wRFoptions &= ~(RF_OPTION_BEAM_MASK | RF_OPTION_FRAGMENTED_BEAM);
    TxQueueDoTransmit(bCurrentTransmit, false);
    bCurrentTransmit->wRFoptions = (u16RFoptions & ~RF_OPTION_FRAGMENTED_BEAM);
    return true;
  }
  return false;
}


void
TxQueueTxBeamComplete(void)
{
  DPRINTF("TxBeam compl %d\n", bCurrentTransmit);
  ASSERT_PTR(bCurrentTransmit);

  if (bApplicationTxAbort)
  {
    beam_fragment_count = 0;
    /* Notify the transport layer */
    if (bCurrentTransmit->zcbp_InternalCallback)
    {
      bCurrentTransmit->zcbp_InternalCallback(bCurrentTransmit);
    }
    return;
  }

  if (beam_fragment_count > 0)
  {
    if (bCurrentTransmit->bTxStatus == TX_QUEUE_STATUS_TRANSMITTING)
    {
      bCurrentTransmit->bTxStatus = TX_QUEUE_STATUS_READY_TO_SEND;
    }
    beam_fragment_count--;

    uint16_t fragmentedBeamOffTime = FRAGMENTED_BEAM_PERIOD - llGetWakeUpBeamFragmentTime();
    TxQueueStartTransmissionPause( fragmentedBeamOffTime );
  }
  else
  {
    if (RF_OPTION_FRAGMENTED_BEAM & bCurrentTransmit->wRFoptions)
    {
      bCurrentTransmit->wRFoptions &= ~RF_OPTION_FRAGMENTED_BEAM;
    }
    uint16_t u16RFoptions = bCurrentTransmit->wRFoptions;
    bCurrentTransmit->wRFoptions &= ~(RF_OPTION_BEAM_MASK);
    TxQueueDoTransmit(bCurrentTransmit, false);
    bCurrentTransmit->wRFoptions = u16RFoptions;
  }
}

/**
 * Get the bFrameOptions value of pElement
 * @param pElement Pointer to a TxQueueElement element
 *
 * @retval pElement->bFrameOptions
 */
TxOptions_t TxQueueGetOptions(const TxQueueElement *pElement)
{
  return pElement->bFrameOptions;
}

/**
 * Set the bFrameOptions value of pElement to options
 * @param pElement Pointer to a TxQueueElement element
 * @param options  TX_OPTION flags to be set
 *
 */
void TxQueueInitOptions(TxQueueElement *pElement, TxOptions_t txOptions)
{
  pElement->bFrameOptions = txOptions;
}

/**
 * Add options to bFrameOptions of pElement
 * @param pElement Pointer to a TxQueueElement element
 * @param options  TX_OPTION flags to be set
 *
 */
void TxQueueSetOptionFlags(TxQueueElement *pElement, TxOptions_t txOptions)
{
  pElement->bFrameOptions |= txOptions;
}

/**
 * Clear the bFrameOptions value of pElement
 * @param pElement Pointer to a TxQueueElement element
 * @param options  TX_OPTION flags to be cleared
 *
 */
void TxQueueClearOptionFlags(TxQueueElement *pElement, TxOptions_t txOptions)
{
  pElement->bFrameOptions &= ~txOptions;
}
