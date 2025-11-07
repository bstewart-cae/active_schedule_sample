// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file ZW_main.c
 * @copyright 2022 Silicon Laboratories Inc.
 *
 * @brief Z-Wave main module. System startup, software and hardware
 * initialization. Execution of the main poll loop.
 */
#include "ZW_classcmd.h"
#include "ZW_lib_defines.h"

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <ZW_main.h>
#include <string.h>
#include <Assert.h>

#include <TickTime.h>

#include "ZW_home_id_generator.h"

#ifdef ZW_SLAVE
#include <ZW_slave.h>
#include <ZW_slave_network_info_storage.h>
#include "MfgTokens.h"
#endif
#ifdef ZW_CONTROLLER
#include <ZW_controller.h>
#include <ZW_routing.h>
#include <ZW_controller_network_info_storage.h>
#endif

#ifdef USE_RESPONSEROUTE
#include <ZW_routing_all.h>
#endif

#include <ZW_timer.h>
#include <ZW_transport.h>
#include <ZW_MAC.h>

#include <SizeOf.h>

#ifdef ZW_SECURITY_PROTOCOL
#include <ZW_keystore.h>
#include <ZW_Security_Scheme2.h>
#include <ZW_secure_learn_support.h>
#endif /* ZW_SECURITY_PROTOCOL */

#include <FreeRTOS.h>
#include "task.h"
#include "SyncEvent.h"
#include "EventDistributor.h"

#include "ZW_protocol_interface.h"
#include "ZW_CCList.h"

#include "ZW_nvm.h"

#include <ZW_explore.h>

//#define DEBUGPRINT
#include "DebugPrint.h"

#include <ZW_tx_queue.h>

#ifdef ZW_SECURITY_PROTOCOL
#include <ZW_Security_Scheme0.h>
#endif /* ZW_SECURITY_PROTOCOL */

#include <ZW_noise_detect.h>

#include <ZW_ctimer.h>
#include <ZW_basis.h>
#include "key_generation.h"
#include <ZW_DataLinkLayer.h>
#include <MfgTokens.h>

#include <zpal_radio.h>
#include <zpal_watchdog.h>
#include <zpal_power_manager.h>
#include <zpal_radio_utils.h>
#include <zpal_entropy.h>
#include <zpal_init.h>
#include <zpal_misc.h>

#include "zpal_watchdog.h"

#include <ZW_UserTask.h>
#if defined(ZW_SLAVE) && defined(ZWAVE_PSA_SECURE_VAULT)
#include "psa/ZW_psa.h"
#endif

/****************************************************************************/
/*                      PRIVATE TYPES and DEFINITIONS                       */
/****************************************************************************/
/*
 * The amount of time that the protocol should stay awake, by default when
 * receiving a wakeup beam. This number has great consequences for the battery
 * lifetime of FLiRS devices. As system uses upto 500 times more power in awake
 * listening state than in FLiRS mode.
 */
#define FLIRS_WAKEUP_TIMEOUT 500

static void Init_ZW_SW(zpal_reset_reason_t bMainWakeUpReason, const SProtocolConfig_t * pProtocolConfig);

TaskHandle_t g_ZwaveMainTaskHandle;

static zpal_reset_reason_t mResetReason = ZPAL_RESET_REASON_OTHER;

// Prioritized events that can wakeup protocol thread.
typedef enum EProtocolEvent
{
  EPROTOCOLEVENT_RFRXBEAM = 0,
  EPROTOCOLEVENT_RFTXCOMPLETE,
  EPROTOCOLEVENT_RFRX,
  EPROTOCOLEVENT_RFTXBEAM,
  EPROTOCOLEVENT_RFTXFAILLBT,
  EPROTOCOLEVENT_RFTXFAIL,
  EPROTOCOLEVENT_RFRXABORT,
  EPROTOCOLEVENT_TIMER,
  EPROTOCOLEVENT_EXPLORE,
  EPROTOCOLEVENT_APP_TX,
  EPROTOCOLEVENT_APP_COMMAND,
  EPROTOCOLEVENT_NETWORKID_UPDATE,
  EPROTOCOLEVENT_CHANGE_RADIO_PHY,
  EPROTOCOLEVENT_RADIO_ASSERT,
  EPROTOCOLEVENT_RADIO_TX_TIMEOUT,
  EPROTOCOLEVENT_RADIO_RX_TIMEOUT,
  EPROTOCOLEVENT_RADIO_CALIBRATE,
#ifdef ZW_SECURITY_PROTOCOL
  EPROTOCOLEVENT_SECURITY_RUN,
#endif
  NUM_EPROTOCOLEVENT
} EProtocolEvent;


// Event distributor object
static SEventDistributor g_EventDistributor = { 0 };

// Event Handlers
static void EventHandlerRfRxBeam(void);
static void EventHandlerRfTxComplete(void);
static void EventHandlerRfTxBeamComplete(void);
static void EventHandlerRfTxFailedLBT(void);
static void EventHandlerRfTxFailed(void);
static void EventHandlerRfRxAbort(void);
static void EventHandlerNetworkIdUpdate(void);
static void EventHandlerChangeRfPHY(void);
static void EventHandlerRadioAssert(void);
static void EventHandlerRadioCalibrate(void);
static void EventHandlerRadioTxTimeout(void);
static void EventHandlerRadioRxTimeout(void);
#ifdef ZW_SECURITY_PROTOCOL
extern void runCycle(void);
#endif

// Event distributor event handler table
static const EventDistributorEventHandler g_aEventHandlerTable[NUM_EPROTOCOLEVENT] =
{
  EventHandlerRfRxBeam,               // Event 0
  EventHandlerRfTxComplete,           // Event 1
  zpal_radio_get_last_received_frame, // Event 2
  EventHandlerRfTxBeamComplete,       // Event 3
  EventHandlerRfTxFailedLBT,          // Event 4
  EventHandlerRfTxFailed,             // Event 5
  EventHandlerRfRxAbort,              // Event 6
  ZwTimerNotificationHandler,         // Event 7
  ExploreMachine,                     // Event 8
  ProtocolInterfaceAppTxHandler,      // Event 9
  ProtocolInterfaceAppCommandHandler, // Event 10
  EventHandlerNetworkIdUpdate,        // Event 11
  EventHandlerChangeRfPHY,            // Event 12
  EventHandlerRadioAssert,            // Event 13
  EventHandlerRadioTxTimeout,         // Event 14
  EventHandlerRadioRxTimeout,         // Event 15
  EventHandlerRadioCalibrate,         // Event 16
#ifdef ZW_SECURITY_PROTOCOL
  runCycle                            // Event 17
#endif
};

#define PROTOCOL_EVENT_RF_RX_BEAM             (1UL << EPROTOCOLEVENT_RFRXBEAM)
#define PROTOCOL_EVENT_RF_TX_COMPLETE         (1UL << EPROTOCOLEVENT_RFTXCOMPLETE)
#define PROTOCOL_EVENT_RF_RX_FRAME_RECEIVED   (1UL << EPROTOCOLEVENT_RFRX)
#define PROTOCOL_EVENT_RF_TX_BEAM_COMPLETE    (1UL << EPROTOCOLEVENT_RFTXBEAM)
#define PROTOCOL_EVENT_RF_TX_FAIL_LBT         (1UL << EPROTOCOLEVENT_RFTXFAILLBT)
#define PROTOCOL_EVENT_RF_TX_FAIL             (1UL << EPROTOCOLEVENT_RFTXFAIL)
#define PROTOCOL_EVENT_RF_RX_ABORT            (1UL << EPROTOCOLEVENT_RFRXABORT)
#define PROTOCOL_EVENT_TIMER                  (1UL << EPROTOCOLEVENT_TIMER)
#define PROTOCOL_EVENT_EXPLORE                (1UL << EPROTOCOLEVENT_EXPLORE)
#define PROTOCOL_EVENT_APP_TX                 (1UL << EPROTOCOLEVENT_APP_TX)
#define PROTOCOL_EVENT_APP_COMMAND            (1UL << EPROTOCOLEVENT_APP_COMMAND)
#define PROTOCOL_EVENT_NETWORKID_UPDATE       (1UL << EPROTOCOLEVENT_NETWORKID_UPDATE)
#define PROTOCOL_EVENT_CHANGE_RF_PHY          (1UL << EPROTOCOLEVENT_CHANGE_RADIO_PHY)
#define PROTOCOL_EVENT_RADIO_ASSERT           (1UL << EPROTOCOLEVENT_RADIO_ASSERT)
#define PROTOCOL_EVENT_RADIO_TX_TIMEOUT       (1UL << EPROTOCOLEVENT_RADIO_TX_TIMEOUT)
#define PROTOCOL_EVENT_RADIO_RX_TIMEOUT       (1UL << EPROTOCOLEVENT_RADIO_RX_TIMEOUT)
#define PROTOCOL_EVENT_RADIO_CALIBRATE        (1UL << EPROTOCOLEVENT_RADIO_CALIBRATE)
#ifdef ZW_SECURITY_PROTOCOL
#define PROTOCOL_EVENT_SECURITY_RUN           (1UL << EPROTOCOLEVENT_SECURITY_RUN)
#endif

/****************************************************************************/
/*                              PRIVATE DATA                                */
/****************************************************************************/
static const uint8_t git_hash_id[40] = GIT_HASH_ID;

static zpal_radio_network_stats_t sNetworkStatistic = { 0 };

#ifdef ZW_BEAM_RX_WAKEUP
/* Minimum delay in ms for the beam ack frames */
#define FIX_DELAY_BEAM_ACK_MS                   6
/* Maximum random delay in ms for the beam ack frames (from 0 to 10 ms) */
#define MAX_RANDOM_DELAY_BROADCAST_BEAM_ACK_MS 11

static uint32_t mainStatisticRxBeamNoFLiRS = 0;
static uint32_t mainStatisticRxBeamDoACK = 0;
static uint32_t mainStatisticRxBeamWaitFrame = 0;
#endif

#define ZW_NORMAL_MODE  0
uint8_t sleepMode = ZW_NORMAL_MODE; /* true when sleep {CPU power save} mode requested */

static zpal_pm_handle_t protocol_radio_power_lock;
static zpal_pm_handle_t protocol_deep_sleep_power_lock;


#ifdef ZW_CONTROLLER

extern bool pendingUpdateOn;
extern bool pendingTableEmpty;
/* TO#1547 fix - flag added for making sure that the pending list only gets flushed once */
/* before the controller goes to sleep, incase the flushing do not empty the list */
/* Previously the controller would continue to flush the pending list until empty */
extern bool pendingUpdateNotCompleted;
#endif

#ifdef ZW_PROMISCUOUS_MODE
bool promisMode = false;
#endif

#define MAX_POWERDOWN_CALLBACKS 3

/** Array of application functions to call on power down */
static void (*a_AppPowerDownCallbacks[MAX_POWERDOWN_CALLBACKS])(void);

static bool do_after_sleep_handling = false;
static bool wdog_enabled = false;

/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/

#ifdef ZW_BEAM_RX_WAKEUP
SSwTimer flirsTimer;
SSwTimer beamACKTimer;
#endif

//Timer to sample noise every second
#define SAMPLE_NOISE_INTERVAL  1000
SSwTimer sampleNoiseTimer = { 0 };

/****************************************************************************/
/*                              STUB FUNCTIONS                              */
/****************************************************************************/

/****************************************************************************/
/*                            PRIVATE FUNCTIONS                             */
/****************************************************************************/
static void
PrepareProtocolAfterPowerUp();

static void mainPowerDownNotify();

#ifdef ZW_BEAM_RX_WAKEUP
static void flirsTimeout(SSwTimer * pTimer);
static void BeamAckTimeout( SSwTimer * pTimer);
#endif

static void
sampleNoiseTimeout(__attribute__((unused)) SSwTimer * timer)
{
  // Sample noiselevel only if TxQueue is empty
  if (TxQueueIsEmpty())
  {
    SampleNoiseLevel();
  }
}


static uint32_t
NetworkIdGenerateHomeId(uint8_t *pHomeId, node_id_t *pNodeId)
{
  uint32_t NewHomeId;

  NewHomeId = HomeIdGeneratorGetNewId(pHomeId);
#ifdef ZW_CONTROLLER
  *pNodeId = NODE_CONTROLLER;
  ControllerStorageSetNetworkIds(pHomeId, *pNodeId);
#else
  *pNodeId = 0;
  SlaveStorageSetNetworkIds(pHomeId, *pNodeId);
#endif

  return NewHomeId;
}


static uint32_t
ConvertHomeIDToUInt32(uint8_t *pHomeId)
{
  uint32_t ui32HomeId;

  ui32HomeId = (pHomeId[0] << 24) +
               (pHomeId[1] << 16) +
               (pHomeId[2] << 8) +
                pHomeId[3];
  return ui32HomeId;
}


/**@brief Function checking if Network HomeID is valid and generates a new HomeID if not valid
 *
 *    Check if current homeID is 0000 or FFFF and nodeID equals 0 (Slave)
 *    if so then a new random homeID is generated and nodeId set 1 for Controllers
 *
 * @return uint32_t current homeID
 */
static uint32_t
NetworkIdValidateAndUpdate(uint8_t *pHomeId, node_id_t *pNodeId)
{
  uint32_t ui32HomeID;

  ui32HomeID = ConvertHomeIDToUInt32(pHomeId);
  if ((
#ifdef ZW_SLAVE
       (0 == *pNodeId) &&
#endif
       (0 == ui32HomeID)) ||
      (0 == ~ui32HomeID))
  {
    ui32HomeID = NetworkIdGenerateHomeId(pHomeId, pNodeId);
  }
  return ui32HomeID;
}


/**@brief Function validates current Network HomeID and updates the nodeId and homeId if needed.
 *
 *
 */
void
NetworkIdValidate(void)
{
  uint32_t ui32HomeId;

  /* Validate the stored IDs from non-volatile memory, not just the locally stored ones,
   * since the stored values are updated elsewhere as well. */

#ifdef ZW_SLAVE
  SlaveStorageGetNetworkIds(ZW_HomeIDGet(), &g_nodeID);  // Restore the stored random homeID into the read cache of homeID.
#endif

  ui32HomeId = NetworkIdValidateAndUpdate(ZW_HomeIDGet(), &g_nodeID);
  ProtocolInterfaceHomeIdSet(ui32HomeId, g_nodeID);

  zpal_radio_mode_t radioMode = ZPAL_RADIO_MODE_NON_LISTENING;
  if (ZW_nodeIsFLiRS()) {
    radioMode = ZPAL_RADIO_MODE_FLIRS;
  } else if (ZW_nodeIsListening()) {
    radioMode = ZPAL_RADIO_MODE_ALWAYS_LISTENING;
  }
  llSetNetworkId(ui32HomeId, g_nodeID, radioMode);
}

// RF Tx completion signaling
static void
TXHandlerFromISR(zpal_radio_event_t txStatus)
{
  BaseType_t xHigherPriorityTaskWoken;
  BaseType_t status = pdPASS;

  xHigherPriorityTaskWoken = pdFALSE;
  DPRINTF("TXHandler, txStatus: 0x%x\n", txStatus);
  uint32_t protocolEvent = PROTOCOL_EVENT_RF_TX_FAIL;
  switch(txStatus)
  {
    case ZPAL_RADIO_EVENT_TX_COMPLETE:
    {
      protocolEvent = PROTOCOL_EVENT_RF_TX_COMPLETE;
    break;
    }
    case ZPAL_RADIO_EVENT_TX_BEAM_COMPLETE:
    {
      protocolEvent = PROTOCOL_EVENT_RF_TX_BEAM_COMPLETE;
    break;
    }
    case ZPAL_RADIO_EVENT_TX_FAIL:
    {
      protocolEvent = PROTOCOL_EVENT_RF_TX_FAIL;
    break;
    }
    case ZPAL_RADIO_EVENT_TX_FAIL_LBT:
    {
      protocolEvent = PROTOCOL_EVENT_RF_TX_FAIL_LBT;
    break;
    }
    default:
      DPRINTF("Unknow zpal_radio_event, reboot\n");
      ASSERT(0);
      zpal_reboot_with_info(MFG_ID_ZWAVE_ALLIANCE, ZPAL_RESET_UNHANDLED_RADIO_EVENT);
    break;
  }
  status = xTaskNotifyFromISR(g_ZwaveMainTaskHandle,
                              protocolEvent,
                              eSetBits,
                              &xHigherPriorityTaskWoken);
  ASSERT(status == pdPASS);
  portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}


/*
 * Handler for asserts comming from rail,
 */
static void
RadioAssertHandler(zpal_radio_event_t assertVal)
{
  BaseType_t xHigherPriorityTaskWoken;
  BaseType_t status = pdPASS;
  uint32_t event = PROTOCOL_EVENT_RADIO_ASSERT;
  if (ZPAL_RADIO_EVENT_TX_TIMEOUT == assertVal)
  {
    event = PROTOCOL_EVENT_RADIO_TX_TIMEOUT;
  }
  else if (ZPAL_RADIO_EVENT_RX_TIMEOUT == assertVal)
  {
    event = PROTOCOL_EVENT_RADIO_RX_TIMEOUT;
  }
  xHigherPriorityTaskWoken = pdFALSE;
  if (!zpal_in_isr())  // If not in IRQ
  {
    status = xTaskNotify(g_ZwaveMainTaskHandle,
             event,
             eSetBits);
  }
  else
  {
    status = xTaskNotifyFromISR(g_ZwaveMainTaskHandle,
                  event,
                  eSetBits,
                  &xHigherPriorityTaskWoken);
  }

  ASSERT(status == pdPASS);
  portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}


// RF Rx completion signaling
static void
RXHandlerFromISR(zpal_radio_event_t rxStatus)
{
  BaseType_t xHigherPriorityTaskWoken;
  BaseType_t status = pdPASS;

  xHigherPriorityTaskWoken = pdFALSE;
  switch (rxStatus)
  {
    case ZPAL_RADIO_EVENT_RX_BEAM_COMPLETE:
    {
      status = xTaskNotifyFromISR(g_ZwaveMainTaskHandle,
                                  PROTOCOL_EVENT_RF_RX_BEAM,
                                  eSetBits,
                                  &xHigherPriorityTaskWoken);
    }
    break;

    case ZPAL_RADIO_EVENT_RX_ABORT:
    {
      status = xTaskNotifyFromISR(g_ZwaveMainTaskHandle,
                                  PROTOCOL_EVENT_RF_RX_ABORT,
                                  eSetBits,
                                  &xHigherPriorityTaskWoken);
    }
    break;

    case ZPAL_RADIO_EVENT_RX_COMPLETE:
    {
      status = xTaskNotifyFromISR(g_ZwaveMainTaskHandle,
                                  PROTOCOL_EVENT_RF_RX_FRAME_RECEIVED,
                                  eSetBits,
                                  &xHigherPriorityTaskWoken);
    }
    break;

    case ZPAL_RADIO_EVENT_RX_TIMEOUT:
    {
      status = xTaskNotifyFromISR(g_ZwaveMainTaskHandle,
                                  PROTOCOL_EVENT_RADIO_RX_TIMEOUT,
                                  eSetBits,
                                  &xHigherPriorityTaskWoken);
    }
    break;

    case ZPAL_RADIO_EVENT_RXTX_CALIBRATE:
    {
      status = xTaskNotifyFromISR(g_ZwaveMainTaskHandle,
                                  PROTOCOL_EVENT_RADIO_CALIBRATE,
                                  eSetBits,
                                  &xHigherPriorityTaskWoken);
    }
    break;

    default:
      status = ~pdPASS;
      break;
  }

  ASSERT(status == pdPASS);
  portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}


static void
RegionChangeHandler(zpal_radio_event_t regionChangeStatus);


/*==============================   Init_ZW_SW   ==============================
**    Z-Wave Software initial setup
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
static void           /*RET  Nothing                  */
Init_ZW_SW(zpal_reset_reason_t bMainWakeUpReason, const SProtocolConfig_t * pProtocolConfig)    /*IN   Nothing                  */
{
  ASSERT(g_ZwaveMainTaskHandle != NULL);
  ZwTimerInit(EPROTOCOLEVENT_TIMER, g_ZwaveMainTaskHandle);

  /* setup software system based on values
     saved in the non-volatile memory */

  TxQueueRegisterPowerLocks();
  TxQueueInit();

  // Configure radio
  zpal_radio_wakeup_t eWakeup = ZPAL_RADIO_WAKEUP_NEVER_LISTEN;
  uint8_t DeviceOptionsMask = pProtocolConfig->pNodeInfo->DeviceOptionsMask;

  if (0 != (DeviceOptionsMask & APPLICATION_NODEINFO_LISTENING))
  {
    /*Listening devices should be always awake.*/
    eWakeup = ZPAL_RADIO_WAKEUP_ALWAYS_LISTEN;
  }
  else if(0 != (DeviceOptionsMask & APPLICATION_FREQ_LISTENING_MODE_250ms))
  {
    eWakeup = ZPAL_RADIO_WAKEUP_EVERY_250ms;
  }
  else if (0 != (DeviceOptionsMask & APPLICATION_FREQ_LISTENING_MODE_1000ms))
  {
    eWakeup = ZPAL_RADIO_WAKEUP_EVERY_1000ms;
  }

  zpal_radio_region_t eRegion = zpal_radio_get_valid_region(pProtocolConfig->pRadioConfig->eRegion);
  if (ZPAL_RADIO_PROTOCOL_MODE_UNDEFINED == zpal_radio_region_get_protocol_mode(eRegion, ZPAL_RADIO_LR_CH_CFG_NO_LR))
  {
    DPRINTF("FATAL ERROR: Invalid region code: %d\r\n", eRegion);
    ASSERT(false);
  }

  /* Initialize all NVM memory in all node types if not already done.
   * This sets homeID and nodeID from NVM3, if they exist, otherwise set to 0. */
#ifdef ZW_CONTROLLER
  ControllerStorageGetNetworkIds(ZW_HomeIDGet(), &g_nodeID);
#else   // #ifdef ZW_CONTROLLER
  SlaveStorageGetNetworkIds(ZW_HomeIDGet(), &g_nodeID);
#endif  // #ifdef ZW_CONTROLLER

  // Init Application <-> Protocol interface
#ifdef ZW_CONTROLLER
  SyncEventArg1Bind(&g_ControllerStaticControllerNodeIdChanged, ProtocolInterfaceSucNodeIdSet);
  SyncEventArg2Bind(&g_ControllerHomeIdChanged, ProtocolInterfaceHomeIdSet);
#endif //   #ifdef ZW_CONTROLLER

#ifdef ZW_SLAVE_ROUTING
  SyncEventArg1Bind(&g_SlaveSucNodeIdChanged, ProtocolInterfaceSucNodeIdSet);
#endif // #ifdef ZW_SLAVE_ROUTING

#ifdef ZW_SLAVE
  SyncEventArg2Bind(&g_SlaveHomeIdChanged, ProtocolInterfaceHomeIdSet);
#endif // #ifdef ZW_SLAVE
#if defined(ZW_SECURITY_PROTOCOL)
  SyncEventArg1Bind(&g_KeystoreSecurityKeysChanged, ProtocolInterfaceSecurityKeysSet);
#endif // #if defined(ZW_SECURITY_PROTOCOL)

  zpal_radio_profile_t RfProfile = {.region = eRegion,
                              .wakeup = eWakeup,
                              .listen_before_talk_threshold = pProtocolConfig->pRadioConfig->iListenBeforeTalkThreshold,
                              .tx_power_max = pProtocolConfig->pRadioConfig->iTxPowerLevelMax,
                              .tx_power_adjust = pProtocolConfig->pRadioConfig->iTxPowerLevelAdjust,
                              .tx_power_max_lr = pProtocolConfig->pRadioConfig->iTxPowerLevelMaxLR,
                              .home_id = ZW_HomeIDGet(),
                              .rx_cb = RXHandlerFromISR,
                              .tx_cb = TXHandlerFromISR,
                              .region_change_cb = RegionChangeHandler,
                              .assert_cb = RadioAssertHandler,
                              .network_stats = &sNetworkStatistic,
                              .radio_debug_enable = pProtocolConfig->pRadioConfig->radio_debug_enable,
#ifdef ZW_CONTROLLER
                              .lr_channel_auto_mode = StorageGetLongRangeChannelAutoMode(),
#endif
                              .primary_lr_channel = ZPAL_RADIO_LR_CHANNEL_UNINITIALIZED,
                              .active_lr_channel_config = ZPAL_RADIO_LR_CH_CFG_COUNT,
  };

  // Set primaryLongRangeChannel to stored value
  //Force to ZPAL_RADIO_LR_CHANNEL_A in case of wrong value (e.g. empty flash memory)
  RfProfile.primary_lr_channel = StorageGetPrimaryLongRangeChannelId();
  if (zpal_radio_region_is_long_range(eRegion)
  && (   (ZPAL_RADIO_LR_CHANNEL_UNINITIALIZED == RfProfile.primary_lr_channel)
      || (ZPAL_RADIO_LR_CHANNEL_UNKNOWN <= RfProfile.primary_lr_channel) ) )
  {
    RfProfile.primary_lr_channel = ZPAL_RADIO_LR_CHANNEL_A;
    StorageSetPrimaryLongRangeChannelId(RfProfile.primary_lr_channel);
  }

  /* During the init, no need to check for channel configuration change. So active_lr_channel_config is set immediatly
  and should be used to set the first PHY configuration.*/
  RfProfile.active_lr_channel_config = ZW_LrChannelConfigToUse(&RfProfile);

  /*
   * Initialize the transport layer.
   * This also sets up the radio and decides if it is Long Range capable.
   * It must therefore be called before ControllerInit()/SlaveInit()
   */
  TransportInit(&RfProfile);

  /*
   * Initialize the random number generator after the radio is initialized in TransportInit().
   */
  zpal_entropy_init();

  /*
   * Initialize Controller/Slave.
   */
#ifdef ZW_CONTROLLER
  GetMaxNodeID();  /* This call should not be moved, because some of the later functions depends on it */
  GetMaxNodeID_LR();
  ControllerInit(pProtocolConfig->pVirtualSlaveNodeInfoTable, pProtocolConfig->pNodeInfo, true);
  InitRoutingValues();

  //Update the nodes own NodeInfo in case Long Range support has been enabled/disabled
  EX_NVM_NODEINFO sNodeInfo = { 0 };
  CtrlStorageGetNodeInfo(g_nodeID, &sNodeInfo);

  if(!(sNodeInfo.reserved & ZWAVE_NODEINFO_BAUD_100KLR) && zpal_radio_protocol_mode_supports_long_range(zpal_radio_get_protocol_mode()))
  {
    sNodeInfo.reserved |= ZWAVE_NODEINFO_BAUD_100KLR;
    CtrlStorageSetNodeInfo(g_nodeID, &sNodeInfo);
    ZCB_GetNodeType(HIGHEST_LONG_RANGE_NODE_ID); //Dummy call to change currentCachedNodeInfoNodeID in ZW_controller.c
  }
  else if((sNodeInfo.reserved & ZWAVE_NODEINFO_BAUD_100KLR) && !zpal_radio_protocol_mode_supports_long_range(zpal_radio_get_protocol_mode()))
  {
    sNodeInfo.reserved &= ~ZWAVE_NODEINFO_BAUD_100KLR;
    CtrlStorageSetNodeInfo(g_nodeID, &sNodeInfo);
    ZCB_GetNodeType(HIGHEST_LONG_RANGE_NODE_ID); //Dummy call to change currentCachedNodeInfoNodeID in ZW_controller.c
  }
#endif /* ZW_CONTROLLER */
#ifdef ZW_SLAVE
  SlaveRegisterPowerLocks();
  SlaveInit(pProtocolConfig->pNodeInfo);
#ifdef ZW_SLAVE_ROUTING
  SlaveInitDone(eRegion); /* This function reads value from NVM and saves the initial Region the End Device was programmed to use. */
#ifdef ZW_RETURN_ROUTE_PRIORITY
  ReturnRouteResetPriority();
#endif /* ZW_RETURN_ROUTE_PRIORITY */
#endif /* ZW_SLAVE_ROUTING*/
#endif  /* ZW_SLAVE */


#ifdef ZW_BEAM_RX_WAKEUP
  ZwTimerRegister(&flirsTimer, true, flirsTimeout);
  ZwTimerRegister(&beamACKTimer, false, BeamAckTimeout);
#endif

  /*
   * HomeID is validated asynchronously because the validator need the Random number
   * generator which again need the radio to initialize.
   */
  EventHandlerNetworkIdUpdate();


  if((ZPAL_RESET_REASON_DEEP_SLEEP_EXT_INT == bMainWakeUpReason) || (ZPAL_RESET_REASON_DEEP_SLEEP_WUT == bMainWakeUpReason))
  {
    PrepareProtocolAfterPowerUp();
  }
  NoiseDetectInit();
}

#ifdef ZW_PROMISCUOUS_MODE
/*============================   ZW_SetPromiscuousMode   ======================
**
**  Function description.
**   Enable / disable the installer library promiscuous mode.
**   When promiscuous mode is enabled, all application layer frames will be passed
**   to the application layer regardless if the frames are addressed to another node.
**   When promiscuous mode is disabled, only application frames addressed to the node will be passed
**   to the application layer.
**
**  Side effects:
**
**--------------------------------------------------------------------------*/
void                /*Nothing */
ZW_SetPromiscuousMode(
  bool state)     /* IN Node id of the node to request node info from.*/
{
	promisMode = state;
}
#endif

/*===============================   ZW_SetRFReceiveMode   ===================
**    Initialize the Z-Wave RF chip.
**    Mode on:  Set the RF chip in receive mode and starts the data sampling.
**    Mode off: Set the RF chip in power down mode.
**
**--------------------------------------------------------------------------*/
uint8_t                  /*RET true if operation was executed successfully, false if not */
ZW_SetRFReceiveMode(
  uint8_t mode )         /* IN true: On; false: Off mode */
{
  /*If we have something in the TX queue, now is not a good time to
   * turn off the radio */
  if (!TxQueueIsIdle())
  {
    return false;
  }

  if (mode)
  {
    /* Turn on RX */
    zpal_radio_start_receive();
  }
  else
  {
    /* Turn off RX */
    zpal_radio_power_down();
    xTaskNotifyStateClear(g_ZwaveMainTaskHandle);
    ulTaskNotifyValueClear(g_ZwaveMainTaskHandle, PROTOCOL_EVENT_RF_RX_BEAM);
    ulTaskNotifyValueClear(g_ZwaveMainTaskHandle, PROTOCOL_EVENT_RF_RX_FRAME_RECEIVED);
    ulTaskNotifyValueClear(g_ZwaveMainTaskHandle, PROTOCOL_EVENT_RF_RX_ABORT);
    // What about PROTOCOL_EVENT_RADIO_CALIBRATE?
    ulTaskNotifyValueClear(g_ZwaveMainTaskHandle, PROTOCOL_EVENT_RF_TX_COMPLETE);
    ulTaskNotifyValueClear(g_ZwaveMainTaskHandle, PROTOCOL_EVENT_RF_TX_BEAM_COMPLETE);
    ulTaskNotifyValueClear(g_ZwaveMainTaskHandle, PROTOCOL_EVENT_RF_TX_FAIL_LBT);
    ulTaskNotifyValueClear(g_ZwaveMainTaskHandle, PROTOCOL_EVENT_RF_TX_FAIL);
    TxQueueInit();
    ExplorePurgeQueue(0);
  }
  return true;
}

#ifdef ZW_BEAM_RX_WAKEUP
static void
StartFLiRSTimer(void)
{
}

static void
flirsTimeout(__attribute__((unused)) SSwTimer * pTimer)
{
  // Nothing to do we are already in FLiRS listening mode
}


static void
BeamAckTimeout( __attribute__((unused)) SSwTimer * pTimer )
{
  TransportSetCurrentRxChannel(zpal_radio_get_last_beam_channel());

  DPRINTF("Sending beam ACK on channel %i\r\n", TransportGetCurrentRxChannel());
  /*
   * Beams should be seen as a received package. Using TransportSetCurrentRxChannel(),
   * will allow the beam ACK to be sent on the same channel which the beam was received on.
   */
  SendFragmentedBeamACK();

  if (!ZW_nodeIsListening()){
    zpal_pm_stay_awake(protocol_radio_power_lock, FLIRS_WAKEUP_TIMEOUT);
  }
}
#endif // #ifdef ZW_BEAM_RX_WAKEUP


static void ExploreUpdateRequest(void)
{
  BaseType_t status = pdPASS;

  status = xTaskNotify(g_ZwaveMainTaskHandle,
                       PROTOCOL_EVENT_EXPLORE,
                       eSetBits);

  if (status != pdPASS)
  {
    DPRINT("\nExplore FAIL!\n");
  }
}

void
ZW_mainDeepSleepPowerLockEnable(bool powerLockEnable)
{
  // FLiRS nodes must never remove this lock
  if ((true == powerLockEnable) || (0 != ZW_nodeIsFLiRS()))
  {
    zpal_pm_stay_awake(protocol_deep_sleep_power_lock, 0);
  }
  else
  {
    zpal_pm_cancel(protocol_deep_sleep_power_lock);
  }
}

/*=============================   zwave_task   ===============================
**    Z-Wave protocol task
**    Calls ApplicationPoll
**    This function should be called in a forever loop
**--------------------------------------------------------------------------*/
void
ZwaveTask(SApplicationInterface* pAppInterface)
{
//  DPRINTF("Free FreeRTOS heap = %d\r\n", xPortGetFreeHeapSize());

  // Make ZwaveTaskhandle available outside Zwave task context
  g_ZwaveMainTaskHandle = xTaskGetCurrentTaskHandle();

  /* Check NVM contents */
  ENvmFsInitStatus eNvmStatus = NvmFileSystemInit();
  ASSERT(ENVMFSINITSTATUS_FAILED != eNvmStatus);
  if (ENVMFSINITSTATUS_FORMATTED == eNvmStatus)
  {
    DPRINT("Z-Wave FileSystem Reset\r\n");
  }
  else
  {
    DPRINT("Z-Wave FileSystem mounted\r\n");
  }

  for(uint32_t i=0; i < MAX_POWERDOWN_CALLBACKS; i++)
  {
    a_AppPowerDownCallbacks[i] = NULL;
  }

#ifdef ZW_CONTROLLER
  CtrlStorageInit();
#endif
#ifdef ZW_SLAVE
  SlaveStorageInit();
#endif  // ZW_SLAVE

  transportVarInit();

  /* use the new wake up reason if it was override by the beam detection process*/

  /* Init Z-Wave basis software */
  Init_ZW_SW(pAppInterface->wakeupReason, pAppInterface->pProtocolConfig);

  if (0 != ZW_nodeIsListening())
  {
    ZW_SetRFReceiveMode(true);
  }

  // Setup application read address for network statistics
  pAppInterface->AppHandles.pNetworkStatistics = &sNetworkStatistic;

  ProtocolInterfaceInit(
                        pAppInterface,
                        EPROTOCOLEVENT_APP_TX,
                        EPROTOCOLEVENT_APP_COMMAND
                       );

  CCListSetNetworkInfo(
              pAppInterface->AppHandles.pNetworkInfo
            );

  // Configure event distributor
  EventDistributorConfig(
                          &g_EventDistributor,
                          sizeof_array(g_aEventHandlerTable),
                          g_aEventHandlerTable,
                          NULL
                        );

#ifdef ZW_REPEATER
  // setup and Init explorer module
  NWIInitTimer();
#endif

  ExploreInit();

  /**
   * Register the protocol powerlocks, the radio will be off in M3 and above
   */
  protocol_radio_power_lock = zpal_pm_register(ZPAL_PM_TYPE_USE_RADIO);
  protocol_deep_sleep_power_lock = zpal_pm_register(ZPAL_PM_TYPE_DEEP_SLEEP);

  /*Listening nodes will by default stay awake for a very long time */
  if (ZW_nodeIsListening())
  {
    zpal_pm_stay_awake(protocol_radio_power_lock,  0);

    ZwTimerRegister(&sampleNoiseTimer, true, sampleNoiseTimeout);
    TimerStart(&sampleNoiseTimer, SAMPLE_NOISE_INTERVAL);
  }
  // FLiRS node will always have DeepSleepPowerLockEnabled
  ZW_mainDeepSleepPowerLockEnable(false);

  SyncEventBind(&ExploreMachineUpdateRequest, ExploreUpdateRequest);

  ctimer_init();

#if defined(ZW_SLAVE) && defined(ZWAVE_PSA_SECURE_VAULT)
  psa_key_id_t zw_ecc_key_id = ZWAVE_PSA_ECDH_KEY_ID;
  /* Generate ECC keypair */
  psa_status_t status = zw_psa_gen_ecc_keypair(&zw_ecc_key_id);
  if (status != PSA_SUCCESS)
  {
    zw_security_error(status);
  }
#endif

#ifdef ZW_SECURITY_PROTOCOL
  security_register_power_locks();
  security_init(*(pAppInterface->pProtocolConfig->pSecureKeysRequested));
  check_create_keys_qrcode(zpal_radio_protocol_mode_supports_long_range(zpal_radio_get_protocol_mode()));
#endif /* ZW_SECURITY_PROTOCOL */

#ifdef ZW_CONTROLLER
  check_create_keys_qrcode(zpal_radio_protocol_mode_supports_long_range(zpal_radio_get_protocol_mode()));
#endif

  if(true != zpal_init_is_valid(pAppInterface->pProtocolConfig->pNodeInfo->NodeType.generic,
                                pAppInterface->pProtocolConfig->pNodeInfo->NodeType.specific))
  {
    zpal_init_invalidate();
  }

#ifdef ZW_SECURITY_PROTOCOL
  if ((ZPAL_RESET_REASON_WATCHDOG == pAppInterface->wakeupReason) ||
      (ZPAL_RESET_REASON_PIN == pAppInterface->wakeupReason)) {
    sec2_reset_nonces_tables();
  }
#endif

  // Wait for and process events
  DPRINT("Protocol event processor started\r\n");

  uint32_t iEventWait = 0xFFFFFFFF;
  for (;;)
  {
#ifdef ZW_CONTROLLER
    /* TO#1282 - fix */
    /* TO#1547 fix */
    if (sleepMode && (!pendingTableEmpty && pendingUpdateNotCompleted))
    {
      if (!pendingUpdateOn)
      {
        PendingNodeUpdate();
      }
    }
#endif
    //TODO move to somewhere else --- end ---
    EventDistributorDistribute(&g_EventDistributor, iEventWait, 0);
  }

  // We never want to return from the task
}


// Rf Event Rx Beam received
static void EventHandlerRfRxBeam(void)
{
#ifdef ZW_BEAM_RX_WAKEUP
  if (0 != ZW_nodeIsFLiRS())
  {
    if (zpal_radio_is_fragmented_beam_enabled())
    {
  	  mainStatisticRxBeamDoACK++;
      /*
       * Restart receiver with Channel Scanning and all.
       */
      if (zpal_radio_is_flirs_enabled())
      {
        zpal_radio_reset_after_beam_receive(true);
      }

      /*
       * This timer is set each time we get a beam segment in. The duration of a beam segment is
       * (8+1+3)*8 = 96 bits, which corresponds to ~ 960us. If the transmitter has a turn around time of
       * ~1ms and our time has a jitter of 1ms, then a timeout of 3ms should be sufficient.
       *
       * However 6ms seems to work much better,
       * TODO: understand why?
       */
      uint32_t random_delay_tmp = 0;
      uint16_t nodeID_mask = (ZPAL_RADIO_PROTOCOL_MODE_2 == zpal_radio_get_protocol_mode())?NODE_BROADCAST:NODE_BROADCAST_LR;
      /*if we already sent a beam ack then we are not send a new ackbefore a delay of
        BEAM_TRAIN_DURATION_MS has past */
      if (SendBeamAckAllowed()) {
        if (nodeID_mask == zpal_radio_get_beam_node_id()) {
          random_delay_tmp = zpal_get_pseudo_random() % MAX_RANDOM_DELAY_BROADCAST_BEAM_ACK_MS;
        }
        TimerStart(&beamACKTimer, FIX_DELAY_BEAM_ACK_MS + random_delay_tmp);
      }
      /*
       * The protocol stays awake in
       */
      zpal_pm_stay_awake(protocol_radio_power_lock, FLIRS_WAKEUP_TIMEOUT);
    }
    else
    {
      mainStatisticRxBeamWaitFrame++;
      zpal_pm_stay_awake(protocol_radio_power_lock, FLIRS_WAKEUP_TIMEOUT);
    }
    StartFLiRSTimer();
  }
  else
  {
    mainStatisticRxBeamNoFLiRS++;
  }
#endif
}


// Rf Event Rx Abort
static void EventHandlerRfRxAbort(void)
{
  DPRINT("Rx Abort\n");
}


// Rf Event Assert
static void EventHandlerRadioAssert(void)
{
  DPRINT("*** PUF *** Airbags deployed, rebooting firmware.\r\n");
  ASSERT(0);
  zpal_reboot_with_info(MFG_ID_ZWAVE_ALLIANCE, ZPAL_RESET_RADIO_ASSERT);
}

// RF tx timeout event handle
static void EventHandlerRadioTxTimeout(void)
{
  DPRINTF("Timeout transmitting a packet, restart the radio\r\n");
  zpal_radio_start_receive();
  TxQueueTxComplete(RADIO_STATUS_TX_FAIL);
}

// RF rx timeout event handle
static void EventHandlerRadioRxTimeout(void)
{
#ifdef ZW_BEAM_RX_WAKEUP
    if ( (0 != ZW_nodeIsFLiRS()) && zpal_radio_is_flirs_enabled())
    {
      if (zpal_radio_is_fragmented_beam_enabled())
      {
        mainStatisticRxBeamDoACK++;
        /*
         * This timer is set each time we get a beam segment in. The duration of a beam segment is
         * (8+1+3)*8 = 96 bits, which corresponds to ~ 960us. If the transmitter has a turn around time of
         * ~1ms and our time has a jitter of 1ms, then a timeout of 3ms should be sufficient.
         *
         * However 6ms seems to work much better,
         * TODO: understand why?
         */
        uint32_t random_delay_tmp = 0;
        uint16_t nodeID_mask = (ZPAL_RADIO_PROTOCOL_MODE_2 == zpal_radio_get_protocol_mode()) ? NODE_BROADCAST : NODE_BROADCAST_LR;
        /*if we already sent a beam ack then we are not send a new ackbefore a delay of
          BEAM_TRAIN_DURATION_MS has past */
        if (SendBeamAckAllowed())
        {
          if (nodeID_mask == zpal_radio_get_beam_node_id())
          {
            random_delay_tmp = zpal_get_pseudo_random() % MAX_RANDOM_DELAY_BROADCAST_BEAM_ACK_MS;
          }
          TimerStart(&beamACKTimer, FIX_DELAY_BEAM_ACK_MS + random_delay_tmp);
        }
        /*
         * The protocol stays awake in
         */
        zpal_pm_stay_awake(protocol_radio_power_lock, FLIRS_WAKEUP_TIMEOUT);
      }
      else
      {
        zpal_pm_stay_awake(protocol_radio_power_lock, 14);
      }
    }
    else
    {
      zpal_radio_start_receive();
    }

#else
  zpal_radio_start_receive();
#endif
}

// Rf Event Calibration needed
static void EventHandlerRadioCalibrate(void)
{
  // Radio needs Calibration
  zpal_radio_calibrate();
}


// Rf Event Tx Beam Complete
static void EventHandlerRfTxBeamComplete(void)
{
  // Legacy protocol. Keep for now.
  DPRINT("TX Beam complete\n");
  TxQueueTxBeamComplete();
}


// Rf Event Tx Complete
static void EventHandlerRfTxComplete(void)
{
  DPRINT("TX complete\n");
#ifdef ZW_BEAM_RX_WAKEUP
  if ((0 != ZW_nodeIsFLiRS()) && zpal_radio_is_flirs_enabled())
  {
    if (zpal_radio_is_fragmented_beam_enabled())
    {
      zpal_radio_reset_after_beam_receive(true);
    }
    else
    {
      zpal_radio_start_receive_after_power_down(true);
    }
  }
  else
  {
    zpal_radio_start_receive();
  }
#endif
  TxQueueTxComplete(RADIO_STATUS_TX_COMPLETE);
  // Ensure App Tx queue is rechecked, now that we have transmitted a package.
  BaseType_t status = pdPASS;
  status = xTaskNotify(g_ZwaveMainTaskHandle, PROTOCOL_EVENT_APP_TX, eSetBits);

  if (status != pdPASS)
  {
    DPRINT("App FAIL!\r\n");
  }
}


// Rf Event Tx Complete but failed LBT
static void EventHandlerRfTxFailedLBT(void)
{
  TxQueueTxComplete(RADIO_STATUS_TX_FAIL_LBT);
}


// Rf Event Tx Complete but failed
static void EventHandlerRfTxFailed(void)
{
  TxQueueTxComplete(RADIO_STATUS_TX_FAIL);
}


// NetworkId updated - validate and update if needed
void
NetworkIdUpdateValidateNotify(void)
{
  BaseType_t status = pdPASS;
  status = xTaskNotify(g_ZwaveMainTaskHandle,
                      PROTOCOL_EVENT_NETWORKID_UPDATE,
                      eSetBits);
  if (status != pdPASS)
  {
    DPRINT("NetworkId Update notify FAIL!\n");
  }
}


static void EventHandlerNetworkIdUpdate(void)
{
  NetworkIdValidate();
#ifdef  ZW_BEAM_RX_WAKEUP
  StartFLiRSTimer();
#endif
  DPRINT("NetworkId validated/updated\n");
}


static void
RegionChangeHandler(__attribute__((unused)) zpal_radio_event_t regionChangeStatus)
{
  // Region has changed
  // we need to trigger forced IR calibration
  zpal_radio_request_calibration(true);
  // Make sure buffered RSSI measurements are reset when region changes.
  NoiseDetectInit();
}


// Change RF PHY when applicable
void
ProtocolChangeRfPHYNotify(void)
{
  BaseType_t status = pdPASS;
  status = xTaskNotify(g_ZwaveMainTaskHandle,
                       PROTOCOL_EVENT_CHANGE_RF_PHY,
                       eSetBits);
  if (status != pdPASS)
  {
    DPRINT("Change RF PHY notify FAIL!\n");
  }
}

// Change RF PHY event notification detected
static void EventHandlerChangeRfPHY(void)
{
  ProtocolEventChangeRfPHYdetected();
}


#ifdef ZW_SECURITY_PROTOCOL
void
SecurityRunCycleNotify(void)
{
  BaseType_t status = pdPASS;
  status = xTaskNotify(g_ZwaveMainTaskHandle,
                      PROTOCOL_EVENT_SECURITY_RUN,
                      eSetBits);
  if (status != pdPASS)
  {
    DPRINT("NetworkId Update notify FAIL!\n");
  }
}
#endif

static void
PrepareProtocolForPowerDown(void)
{
#ifdef USE_RESPONSEROUTE
  ReturnRouteStoreForPowerDown();
#endif
}

static void
PrepareProtocolAfterPowerUp(void)
{
#ifdef USE_RESPONSEROUTE
  ReturnRouteRestoreAfterWakeup();
#endif
}


static void
mainPowerDownNotify(void)
{
  PrepareProtocolForPowerDown();
#ifdef ZW_SECURITY_PROTOCOL
  if (0 == (ZW_nodeDeviceOptionsGet() &
            (APPLICATION_NODEINFO_LISTENING |
             APPLICATION_FREQ_LISTENING_MODE_250ms |
             APPLICATION_FREQ_LISTENING_MODE_1000ms)))
  {
    sec2_PowerDownHandler();
  }
#endif

  ZW_AppPowerDownCallBack();
  /* Activate callbacks set with ZW_SetAppPowerDownCallback() */
  for (uint32_t i=0; i < MAX_POWERDOWN_CALLBACKS; i++)
  {
    if (NULL != a_AppPowerDownCallbacks[i])
    {
      a_AppPowerDownCallbacks[i]();
    }
  }
}

ZW_WEAK void
ZW_AppPowerDownCallBack(void)
{

}

bool ZW_SetAppPowerDownCallback(void (*callback)(void))
{
  for (uint32_t i=0; i < MAX_POWERDOWN_CALLBACKS; i++)
  {
    if (NULL == a_AppPowerDownCallbacks[i])
    {
      //Found an empty slot
      a_AppPowerDownCallbacks[i] = callback;
      return true;
    }
  }
  //No place left in array
  return false;
}

/*----------------------------------------------------------------------------
**  Local defines
**----------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------
**  Global variables
**----------------------------------------------------------------------------*/

static SApplicationInterface g_AppInterface = {
  // Ensure CCSet is NULL at start
  .CCSet = NULL
};
static bool m_schedulerIsStarted = false;

/*
 * The bellow stack size for protocol necessary for initializing security ,
 * The mainloop(unsigned int work[64],const unsigned char e[32]) in smul.c consume about 3256 bytes of stack
 * when this implementation changes then we can reduce the protocol stack usage further
 */
// Task and stack buffer allocation for the Z-Wave task!
static StackType_t StackBuffer[TASK_STACK_SIZE_Z_WAVE_STACK];
static StaticTask_t TaskBuffer;
/*============================== system_startup ===============================
**  Startup the system, This function is the first call from the startup code
**----------------------------------------------------------------------------*/
int ZW_system_startup(zpal_reset_reason_t eResetReason)
{
  m_schedulerIsStarted = false;  // The FreeRTOS Scheduler is set to be in NOT STARTED state!

  DPRINTF("Starting 0x%x...\r\n", eResetReason);

  // Set the reset-reason to pass into App and Z-Wave tasks!
  g_AppInterface.wakeupReason = eResetReason;

  // Save reset reason to a static variable.
  mResetReason = eResetReason;

  ZW_MfgTokenModuleInit();

  // Initialize the UserTask-module before calling any customer implementation code!
  ZW_UserTask_Init();

  /***********************************************************************
   * Call application init function!
   *
   * All application side initialization operations shall be done here!
   * In this function all app-side task creation operations is performed
   * before the call to vTaskStartScheduler()!
   **********************************************************************/
  ZW_APPLICATION_STATUS applicationStatus = ApplicationInit(eResetReason);

  /* Did application require special startup handling? */
  if (APPLICATION_POWER_DOWN == applicationStatus)
  {
    /* Create Zwave task, let the scheduler take us down */
    goto start_scheduler;

    /* Should never return from this call */
  }
  if (APPLICATION_TEST == applicationStatus)
  {
    // Do nothing
  }

  /* Create Z-Wave protocol task */
  TaskHandle_t xHandle = xTaskCreateStatic(
                                  (TaskFunction_t)&ZwaveTask,    // pvTaskCode
                                  TASK_NAME_Z_WAVE_STACK,        // pcName
                                  TASK_STACK_SIZE_Z_WAVE_STACK,  // usStackDepth
                                  &g_AppInterface,               // pvParameters
                                  TASK_PRIORITY_Z_WAVE_STACK,    // uxPriority
                                  StackBuffer,                   // pxStackBuffer
                                  &TaskBuffer                    // pxTaskBuffer
                                );
  ASSERT(xHandle != NULL);
  start_scheduler:
  /* Start the scheduler. */
  m_schedulerIsStarted = true;  // Indicate that the Scheduler has started!

  return 0;
}

void zpal_system_startup(zpal_reset_reason_t reset_reason)
{
#ifdef ZW_CONTROLLER
  zpal_init_set_library_type(ZPAL_LIBRARY_TYPE_CONTROLLER);
#endif
#ifdef ZW_SLAVE
  zpal_init_set_library_type(ZPAL_LIBRARY_TYPE_END_DEVICE);
#endif
  ZW_system_startup((zpal_reset_reason_t)reset_reason);
}

void ZW_system_startup_SetEventNotificationBitNumbers(uint8_t iZwRxQueueTaskNotificationBitNumber,
                                                      uint8_t iZwCommandStatusQueueTaskNotificationBitNumber,
                                                      const SProtocolConfig_t * pProtocolConfig)
{
  // Register the association between Event Notification Bit Number and the event handler of the APP!
  g_AppInterface.iZwRxQueueTaskNotificationBitNumber = iZwRxQueueTaskNotificationBitNumber;
  g_AppInterface.iZwCommandStatusQueueTaskNotificationBitNumber = iZwCommandStatusQueueTaskNotificationBitNumber;
  g_AppInterface.pProtocolConfig = pProtocolConfig;
}

void ZW_system_startup_SetMainApplicationTaskHandle(TaskHandle_t xHandle)
{
  g_AppInterface.ApplicationTaskHandle = xHandle;
}

void ZW_system_startup_SetCCSet(SCommandClassSet_t *CCSet)
{
  g_AppInterface.CCSet = CCSet;
}

SCommandClassSet_t *ZW_system_startup_GetCCSet(void)
{
  return g_AppInterface.CCSet;
}

SApplicationHandles* ZW_system_startup_getAppHandles(void)
{
  return &g_AppInterface.AppHandles;
}

const SAppNodeInfo_t* ZW_system_startup_getAppNodeInfo(void)
{
  return g_AppInterface.pProtocolConfig->pNodeInfo;
}


bool ZW_system_startup_IsSchedulerStarted(void)
{
  return m_schedulerIsStarted;
}

void enterPowerDown(__attribute__((unused)) uint32_t millis)
{
  wdog_enabled = zpal_is_watchdog_enabled();

  if (wdog_enabled)
  {
    zpal_enable_watchdog(false);
  }
}

void exitPowerDown(__attribute__((unused)) uint32_t millis)
{
  if (wdog_enabled)
  {
    zpal_enable_watchdog(wdog_enabled);
  }
}

void zpal_zw_pm_event_handler(zpal_pm_mode_t from, zpal_pm_mode_t to)
{
  if (to > from)
  {
    do_after_sleep_handling = false;
  }
  else if (to < ZPAL_PM_MODE_DEEP_SLEEP && from >= ZPAL_PM_MODE_DEEP_SLEEP)
  {
    //Wake up from deep sleep
    do_after_sleep_handling = true;
  }

  if (to > ZPAL_PM_MODE_DEEP_SLEEP)
  {
    // We are going into deep sleep
    mainPowerDownNotify();
  }
  else if (to == ZPAL_PM_MODE_DEEP_SLEEP)
  {
#ifdef ZW_BEAM_RX_WAKEUP
    DPRINTF("Sleep %i %i\r\n", to, ZW_nodeIsFLiRS());
    if (ZW_nodeIsFLiRS() && (0 != g_nodeID))
    {
       // Radio off after FLIRS
       zpal_radio_abort();

      // Allow FLiRS mode after power up
       zpal_radio_enable_flirs();

    }
    else
#endif
    {
      /* zpal_radio_power_down is called to make sure that the RF FSM is up to date. */
      zpal_radio_power_down();
    }
  }
}

zpal_reset_reason_t GetResetReason(void)
{
  /* mResetReason is only written to before the scheduler
   * is started. I.e. it is safe to call this function from any thread
   * after the scheduler is started. */
  return mResetReason;
}

/**
**    Makes System time in ms available to LibS2.
**
**    It is not allowed to call method from ISR.
**    Not in header file since its only referenced from inside LibS2.
*/
uint32_t clock_time(void)
{
  return getTickTime();
}

uint16_t ZW_GetProtocolBuildNumber(void)
{
#ifdef ZW_BUILD_NO
#define ZW_BUILD_PROTOCOL_NO                ZW_BUILD_NO
#else
#define ZW_BUILD_PROTOCOL_NO                0xABCD
#endif
  return (uint16_t)ZW_BUILD_PROTOCOL_NO;
}

uint8_t* ZW_GetProtocolGitHash(void)
{
  return (uint8_t *) git_hash_id;
}

uint16_t ZW_GetProtocolManufacturerId(void)
{
#ifdef ZW_PROTOCOL_MANUFACTURER_ID
  return ZW_PROTOCOL_MANUFACTURER_ID;
#else
  return MFG_ID_ZWAVE_ALLIANCE;
#endif
}
