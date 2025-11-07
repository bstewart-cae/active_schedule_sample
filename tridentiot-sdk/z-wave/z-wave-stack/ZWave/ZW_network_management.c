// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file ZW_network_management.c
 * @brief Handling of network management functionality.
 * @author Johann Sigfredsson
 * @copyright 2022 Silicon Laboratories Inc.
 * @details Implements network management functionality handling for
 * the different inclusion and exclusion modes.
 */
#include "ZW_lib_defines.h"

/**
 *                              INCLUDE FILES
 */
#ifdef ZW_CONTROLLER
#include <ZW_controller.h>
#include <ZW_controller_api.h>
#include <ZW_controller_network_info_storage.h>
#else
#include <ZW_slave_network_info_storage.h>
#include <ZW_slave.h>
#endif

#include <ZW_basis.h>
#include <ZW_explore.h>
#include <ZW_keystore.h>
#include <ZW_network_management.h>
#include <ZW_timer.h>
#include <SwTimer.h>
#include <ZW_secure_learn_support.h>
#include <ZW_protocol_interface.h>
#include <ZW.h>
#include <ZW_main.h>
#include <zpal_power_manager.h>
#include <zpal_retention_register.h>
#include <ZW_DataLinkLayer.h>
#include <ZW_system_startup_api.h>

//#define DEBUGPRINT
#include "DebugPrint.h"

#include <zpal_entropy.h>
#include <zpal_radio.h>
#include <string.h>


/**
 *                      PRIVATE TYPES and DEFINITIONS
 */

/**
 * Timeout definitions in 1 msec ticks
 */
#define NETWORK_MANAGEMENT_ONE_HUNDRED_MSEC_TIMEOUT 100
#define NETWORK_MANAGEMENT_ONE_SECOND_TIMEOUT       (10 * NETWORK_MANAGEMENT_ONE_HUNDRED_MSEC_TIMEOUT)
#define NETWORK_MANAGEMENT_ONE_MINUTE_TIMEOUT       (60 * NETWORK_MANAGEMENT_ONE_SECOND_TIMEOUT)
#define NETWORK_MANAGEMENT_4SEC_ROUND_TIMEOUT       (4 * NETWORK_MANAGEMENT_ONE_SECOND_TIMEOUT)
#define NETWORK_MANAGEMENT_2SEC_ROUND_TIMEOUT       (2 * NETWORK_MANAGEMENT_ONE_SECOND_TIMEOUT)

/**
 * Timeout count for classic inclusion after initial
 * Broadcast Node Information transmission and inclusion state
 * shifts to network wide inclusion
 */
#define LEARN_MODE_CLASSIC_TIMEOUT              (2 * NETWORK_MANAGEMENT_ONE_SECOND_TIMEOUT)

/**
 * Timeout count for SMART START (pre-kitting) inclusion
 * - First Explore Autoinclusion transmit after initial
 * Broadcast Node Information transmission
 */
#define LEARN_MODE_SMARTSTART_TIMEOUT           (2 * NETWORK_MANAGEMENT_ONE_SECOND_TIMEOUT)

/**
 * Timeout count for network wide inclusion
 */
#define LEARN_MODE_NWI_TIMEOUT                  (4 * NETWORK_MANAGEMENT_ONE_SECOND_TIMEOUT)

/**
 * Max random timeout count for network wide inclusion
 * Unit: 100ms
 */
#define LEARN_MODE_NWI_MAX_RANDOM_TIMEOUT       70

/**
 * Default MAX 512sec timeout between Smart Start Autoinclusion requests
 */
#define LEARN_MODE_SMARTSTART_NWI_MAX_TIMEOUT   (512 * NETWORK_MANAGEMENT_ONE_SECOND_TIMEOUT)

/**
 * Max Inclusion Request Interval - 128second
 */
#define LEARN_MODE_SMARTSTART_NWI_REQUEST_INTERVAL  (128 * NETWORK_MANAGEMENT_ONE_SECOND_TIMEOUT)

#define LEARN_MODE_SMARTSTART_EXPLORE_TIMEOUT       (5 * NETWORK_MANAGEMENT_ONE_SECOND_TIMEOUT)

/**
 * Smart Start timeout.
 * Should be long enough to allow TB2_TIMEOUT (240 seconds) to expire
 */
#define LEARN_MODE_SMARTSTART_LEARN_TIMEOUT         (250 * NETWORK_MANAGEMENT_ONE_SECOND_TIMEOUT)

/**
 * Random granularity during smart start
 */
#define LEARN_MODE_SMARTSTART_RANDOM_GRANULARITY      120

/**
 * Random granularity during smart start long range
 */
#define LEARN_MODE_SMARTSTART_RANDOM_GRANULARITY_LR   40


#ifdef ZW_CONTROLLER
/**
 * 4min and 55 second timeout
 */
#define NETWORK_MANAGEMENT_SET_NWI_TIMEOUT  ((4 * NETWORK_MANAGEMENT_ONE_MINUTE_TIMEOUT) + (55 * NETWORK_MANAGEMENT_ONE_SECOND_TIMEOUT))
#endif

/**
 * Number of Network Wide Inclusion/Exclusion request
 * before giving up and return to Application with failed
 */
#define NETWORK_MANAGEMENT_NWI_NWE_REQUEST          4

typedef enum _E_NETWORK_MANAGEMENT_STATE_LEARN_
{
  E_NETWORK_MANAGEMENT_STATE_LEARN_IDLE,
  E_NETWORK_MANAGEMENT_STATE_LEARN_STOP,
  E_NETWORK_MANAGEMENT_STATE_LEARN_CLASSIC,
  E_NETWORK_MANAGEMENT_STATE_LEARN_NWI,
  E_NETWORK_MANAGEMENT_STATE_LEARN_SMARTSTART,
  E_NETWORK_MANAGEMENT_STATE_LEARN_IN_PROGRESS,
  E_NETWORK_MANAGEMENT_STATE_LEARN_COUNT
} E_NETWORK_MANAGEMENT_STATE_LEARN;


typedef enum { NW_NONE, NW_INCLUSION, NW_EXCLUSION} NW_ACTION;

#define SYSTEM_TYPE_SMART_START_COUNT_INC     0x01
#define SYSTEM_TYPE_SMART_START_COUNT_MAX     0x0E

/// Each Smart Start inclusion round has 2 states: Prime and Inclusion
#define NUMBER_OF_STATES_IN_SMART_START_CYCLE     2

/// LR nodes emit 4 following frames:
/// LR Prime, LR Inclusion, Explorer Prime, Explorer inclusion
#define NUMBER_OF_STATES_IN_SMART_START_CYCLE_LR  4

/// Flags to determine which frame should be sent next during Smart Start
enum E_SMART_START_STATES
{
  SMART_START_STATE_PRIME        = 0x01, /// Z-Wave Explorer Prime
  SMART_START_STATE_INCLUSION    = 0x02, /// Z-Wave Explorer Inclusion
  SMART_START_STATE_PRIME_LR     = 0x03, /// Z-Wave LR Prime
  SMART_START_STATE_INCLUSION_LR = 0x04, /// Z-Wave LR Inclusion
};


/// Struct for saving Smart Start data to retention RAM.
/// Must not be more than 4 bytes long to fit one word in retention RAM.
typedef struct SRetentionBytes_t
{
  uint8_t smartStartCounter; /// Number of Smart Start rounds
  uint8_t smartStartState;   /// Current E_SMART_START_STATES state or 0,
                             /// if Smart Start inclusion is not in progress
  uint8_t unused_2;
  uint8_t unused_3;
} SRetentionBytes_t;

/// Union for saving data Smart Start to retention RAM.
/// Must be 32bit long to fit one word in retention RAM.
typedef union URetentionWord_t
{
  uint32_t ram_word;
  SRetentionBytes_t b;
} URetentionWord_t;


/**
 *                              PRIVATE DATA
 */

/* true if Learnmode was a Smart Start mode */
static bool bSmartStartLearn = false;

static uint8_t wakeupReasonHandled = 0;

static zpal_pm_handle_t smart_start_power_lock;
static zpal_pm_handle_t long_range_power_lock;

static bool smartStartInclusionOngoing = false;  // Indicates that a SmartStart Related transmission is in progress

/**
 *                              EXPORTED DATA
 */

extern uint8_t g_Dsk[];

static SSwTimer   LearnStateTimer = { 0 };
static SSwTimer * pLearnStateTimer = NULL;

#ifdef ZW_CONTROLLER
static SSwTimer   NetworkManagementSetNWITimer = { 0 };
static SSwTimer * pNetworkManagementSetNWITimer = NULL;
#endif

static uint32_t bRequestNo;
static uint32_t bIncReqCount;
/* Default Inclusion Request Interval */
static uint32_t bInclusionRequestIntervals = 0;

static E_NETWORK_MANAGEMENT_STATE_LEARN learnState;
static uint32_t dwSmartStartInclusionRequestTimeout;
static NW_ACTION NW_Action = NW_NONE;

static void NetworkManagementLearnStateProcess(void);
static void ZCB_NetworkManagementLearnNodeStateTimeout(SSwTimer* pTimer);
static void ZCB_NetworkManagementExploreRequestComplete(ZW_Void_Function_t Context, uint8_t bTxStatus, TX_STATUS_TYPE* pTxStatusReport);
#ifdef ZW_CONTROLLER
static LEARN_INFO_T sNetworkManagementLearnInfo;
static void ZCB_NetworkManagementLearnModeCompleted(LEARN_INFO_T* psLearnInfo);
#else
static void ZCB_NetworkManagementLearnModeCompleted(ELearnStatus bStatus, node_id_t nodeID);
#endif

#ifdef ZW_SLAVE
static zpal_radio_lr_channel_config_t initialLrChCfg;
#endif

#if defined(ZW_SLAVE) || defined(ZW_CONTROLLER_TEST_LIB)
static bool stateSwitchRegion = false;
#endif
/******************************************************************************
 *                            PRIVATE FUNCTIONS
 *****************************************************************************/

__attribute__((always_inline)) static inline uint8_t
NetworkManagementSmartStartRandomGranularity(bool supportLR)
{
  return supportLR ?
    LEARN_MODE_SMARTSTART_RANDOM_GRANULARITY_LR :
    LEARN_MODE_SMARTSTART_RANDOM_GRANULARITY;
}

/**
 *  Clear Smart Start functionality specifics in eSystemTypeState
 */
static void
NetworkManagementSmartStartClear(void)
{
  URetentionWord_t retRAM = {
    .ram_word = 0
  };

  zpal_retention_register_write(ZPAL_RETENTION_REGISTER_SMARTSTART, retRAM.ram_word);

  ZW_mainDeepSleepPowerLockEnable(false);
}

/**
 *  Set Smart Start functionality to enabled/active
 */
static void
NetworkManagementSmartStartEnable(void)
{
  URetentionWord_t retRAM = {
  // If LR is supported, give it priority over Z-Wave protocol
    .b.smartStartState = NodeSupportsBeingIncludedAsLR () ?
          SMART_START_STATE_PRIME_LR :
          SMART_START_STATE_PRIME
  };
  zpal_retention_register_write(ZPAL_RETENTION_REGISTER_SMARTSTART, retRAM.ram_word);
}


/**
 *  Return true if Smart Start functionality has been enabled
 */
static bool
NetworkManagementSmartStartIsEnabled(void)
{
  URetentionWord_t retRAM = { 0 };
  zpal_retention_register_read(ZPAL_RETENTION_REGISTER_SMARTSTART, &(retRAM.ram_word));
  return (0 != retRAM.b.smartStartState);
}

/******************************************************************************
 *                                API FUNCTIONS
 *****************************************************************************/

bool NetworkManagement_IsSmartStartInclusionOngoing(void)
{
  return smartStartInclusionOngoing;
}

void NetworkManagement_IsSmartStartInclusionSet(void)
{
  smartStartInclusionOngoing = true;
#if defined(ZW_SLAVE)
  keystore_smartstart_initiated(true);
#endif
}

void NetworkManagement_IsSmartStartInclusionClear(void)
{
  smartStartInclusionOngoing = false;
#if defined(ZW_SLAVE)
  keystore_smartstart_initiated(false);
#endif
}

/**
 *  Return true if Smart Start functionality enabled and Smart Start INCLUDE bit is set
 *  this is used in Smart Start mode to determine if Smart Start PRIME or INCLUDE
 *  ExploreRequestinclusion Explore frame should be transmitted - if INCLUDE then the node will
 *  wait for the matching AssignID protocol command for 2 seconds before starting next wait period
 */
uint8_t
NetworkManagementSmartStartIsInclude(void)
{
  URetentionWord_t retRAM = { 0 };
  zpal_retention_register_read(ZPAL_RETENTION_REGISTER_SMARTSTART, &(retRAM.ram_word));

  return (SMART_START_STATE_INCLUSION == retRAM.b.smartStartState)
      || (SMART_START_STATE_INCLUSION_LR == retRAM.b.smartStartState);
}

/**
 * Returns whether the current state is a LR state and Smart Start is enabled.
 *
 * @return true if current state is LR, false otherwise.
 */
bool NetworkManagementSmartStartIsLR(void)
{
  if (true != NodeSupportsBeingIncludedAsLR())
  {
    return false;
  }
  URetentionWord_t retRAM = { 0 };
  zpal_retention_register_read(ZPAL_RETENTION_REGISTER_SMARTSTART, &(retRAM.ram_word));
  return (SMART_START_STATE_PRIME_LR == retRAM.b.smartStartState)
      || (SMART_START_STATE_INCLUSION_LR == retRAM.b.smartStartState);
}

#ifdef DEBUGPRINT
void print_retention_register(uint32_t reg)
{
  DPRINT("\n0b");
  for (uint32_t i = 0; i < 32; i++)
  {
    if (reg & 0x80000000)
    {
      DPRINT("1");
    }
    else
    {
      DPRINT("0");
    }
    reg <<= 1;
  }
}
#endif

/**
 *  Return current Smart Start Request number stored eSystemState
 */
static uint32_t
NetworkManagementSmartStartCurrentRequestNo(void)
{
  URetentionWord_t retRAM = { 0 };
  zpal_retention_register_read(ZPAL_RETENTION_REGISTER_SMARTSTART, &(retRAM.ram_word));

  return (uint32_t) retRAM.b.smartStartCounter;
}

/*=================   ZCB_NetworkManagementLearnModeCompleted   ==============
**    Function description
**      Callback which is called on learnmode completes
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
#ifdef ZW_CONTROLLER
static void ZCB_NetworkManagementLearnModeCompleted(LEARN_INFO_T* psLearnInfo)
#else
static void ZCB_NetworkManagementLearnModeCompleted(ELearnStatus bStatus, __attribute__((__unused__)) node_id_t nodeID)
#endif
{
#ifdef ZW_CONTROLLER
  uint8_t bStatus;
#endif
  /* Learning in progress. Protocol will do timeout for us */
  if (ESWTIMER_STATUS_SUCCESS != TimerStop(pLearnStateTimer))
  {
    DPRINTF("TimerStop failed!\n");
    ASSERT(0);
  }

#ifdef ZW_CONTROLLER
  bStatus = psLearnInfo->bStatus;
  memcpy(&sNetworkManagementLearnInfo, psLearnInfo, sizeof(sNetworkManagementLearnInfo));

  if (bStatus == LEARN_MODE_STARTED)
  {
    if(learnState == E_NETWORK_MANAGEMENT_STATE_LEARN_SMARTSTART)
    {
      zpal_pm_stay_awake(smart_start_power_lock, LEARN_MODE_SMARTSTART_LEARN_TIMEOUT);
    }

    learnState = E_NETWORK_MANAGEMENT_STATE_LEARN_IN_PROGRESS;
    ProtocolInterfacePassToAppLearnStatus(psLearnInfo);
  }
  else if ((bStatus == LEARN_MODE_DONE) || (bStatus == LEARN_MODE_FAILED))
  {
    /* Assignment was complete. Tell application */
    if (learnState)
    {
      learnState = E_NETWORK_MANAGEMENT_STATE_LEARN_STOP;
      NetworkManagementLearnStateProcess();
    }
#ifdef ZW_SECURITY_PROTOCOL
    /* Set NVM_SYSTEM_STATE = NVM_SYSTEM_STATE_IDLE - Now idle used for determining if Local Reset needed on startup */
    // TODO : change to controller
      CtrlStorageSetSmartStartState(NVM_SYSTEM_STATE_IDLE);
#endif
    ProtocolInterfacePassToAppLearnStatus(psLearnInfo);
  }
#else
  if (ELEARNSTATUS_ASSIGN_NODEID_DONE == bStatus)
  {
    /* We are being included stop Smart Start */
    NetworkManagementSmartStartClear();

    if(learnState == E_NETWORK_MANAGEMENT_STATE_LEARN_SMARTSTART)
    {
      ProtocolInterfacePassToAppLearnStatus(ELEARNSTATUS_SMART_START_IN_PROGRESS);
      zpal_pm_stay_awake(smart_start_power_lock, LEARN_MODE_SMARTSTART_LEARN_TIMEOUT);
    }
    else
    {
      ProtocolInterfacePassToAppLearnStatus(ELEARNSTATUS_LEARN_IN_PROGRESS);
    }

    learnState = E_NETWORK_MANAGEMENT_STATE_LEARN_IN_PROGRESS;
  }
  else if (bStatus == ELEARNSTATUS_ASSIGN_RANGE_INFO_UPDATE)
  {
    if (NW_Action == NW_EXCLUSION)
    {
      /* Weird, but happens... */
      bRequestNo = 0;
      learnState = E_NETWORK_MANAGEMENT_STATE_LEARN_NWI;

      if (ESWTIMER_STATUS_SUCCESS != TimerStart(pLearnStateTimer, LEARN_MODE_CLASSIC_TIMEOUT))
      {
        DPRINTF("TimerStart failed!\n");
        ASSERT(0);
      }
    }
  }
  else if (bStatus == ELEARNSTATUS_ASSIGN_COMPLETE)
  {
    ELearnStatus t_Status = bStatus;
    if (E_SECURE_LEARN_ERRNO_S0_TIMEOUT != secure_learn_get_errno())
    {
      /* Assignment was complete. Tell application */
      if (bSmartStartLearn)
      {
        if ((0 != getSecureKeysRequested()) &&
            (E_SECURE_LEARN_ERRNO_COMPLETE != secure_learn_get_errno()))
        {
          /* Smart Start slave not included according to specification */
          nodeID = 0;
          t_Status = ELEARNSTATUS_LEARN_MODE_COMPLETED_FAILED;
        }
        bSmartStartLearn = false;
      }
#ifdef ZW_SECURITY_PROTOCOL
      /* Set NVM_SYSTEM_STATE = NVM_SYSTEM_STATE_IDLE - Now idle used for determining if Local Reset needed on startup */
      SlaveStorageSetSmartStartState(NVM_SYSTEM_STATE_IDLE);
#endif
      if (learnState)
      {
        learnState = E_NETWORK_MANAGEMENT_STATE_LEARN_STOP;
        NetworkManagementLearnStateProcess();
      }

      ProtocolInterfacePassToAppLearnStatus(t_Status);
      zpal_pm_cancel(smart_start_power_lock);
    }
    else
    {
      /* S0 timed out - wait for S2 inclusion */
      secure_learn_set_errno_reset();
    }
  }
#endif
}

/*If we have switched the smartstart mode off, then we need to set the channel config back to CH_CFG1 if we have already changed it to CH_CFG3*/
#ifdef ZW_SLAVE
static void ResetLrChannelConfig(void)
{
  /* Why channel config is set only if it was changed to ZPAL_RADIO_LR_CH_CFG3?
  Is there any case where another channel configuration must be kept? If so, which one?*/
  if ( (0 == g_nodeID) && (ZPAL_RADIO_LR_CH_CFG3 == zpal_radio_get_lr_channel_config()) )
  {
    /*Some times the radio would be busy, then we try up to 3 time for maximum of 3 ms*/
    for (uint8_t i  = 0; i < 3; i++ )
    {
      if ( false == llChangeLrChannelConfig(ZPAL_RADIO_LR_CH_CFG1) )
      {
        //change config fail, retry later.
        vTaskDelay(pdMS_TO_TICKS(1));
      }
      else
      {
        //change config success, exit function
        return;
      }
    }
    /*We tried for 3 times and we couldn't switch region, somthing is wrong so we assert in debug
      build to detect this situation*/
    ASSERT(0);
  }
  else {} //product is included in a network, keep the active long range mode
}
#endif
/*=============== ZCB_NetworkManagementExploreRequestComplete ================
**  Callback function called when explore request has been completed
**
**  Side effects:
**
**--------------------------------------------------------------------------*/
static void
ZCB_NetworkManagementExploreRequestComplete(
  __attribute__((unused)) ZW_Void_Function_t   Context,
  __attribute__((unused)) uint8_t bTxStatus,
  __attribute__((unused)) TX_STATUS_TYPE* pTxStatusReport)
{
  DPRINTF("\n%s", __func__);

  URetentionWord_t retRAM = { 0 };
  zpal_retention_register_read(ZPAL_RETENTION_REGISTER_SMARTSTART, &(retRAM.ram_word));
  DPRINTF("\nReg: %02x %02x", retRAM.b.smartStartState, retRAM.b.smartStartCounter);
#ifdef DEBUGPRINT
  //print_retention_register(retRAM.ram_word);
#endif

  bool supportLR      = NodeSupportsBeingIncludedAsLR();
  bool isLRstate      = NetworkManagementSmartStartIsLR();
  bool isIncludeState = NetworkManagementSmartStartIsInclude();

  //DPRINTF("\nNodeSupportsLR(): %s", supportLR ? "true" : "false");
  //DPRINTF("\nNetworkManagementSmartStartIsLR(): %s", isLRstate ? "true" : "false");
  //DPRINTF("\nNetworkManagementSmartStartIsInclude(): %s", NetworkManagementSmartStartIsInclude() ? "true" : "false");

  /* Setup next timeout period */
  if (false == isIncludeState)
  {
    dwSmartStartInclusionRequestTimeout = NETWORK_MANAGEMENT_4SEC_ROUND_TIMEOUT;
  }
  else if ((true == supportLR) && (true == isLRstate) && (true == isIncludeState))
  {
    dwSmartStartInclusionRequestTimeout = NETWORK_MANAGEMENT_2SEC_ROUND_TIMEOUT;
    /*
    * When we have sent a LR Smart Start Include, we must make sure that a battery device stays
    * awake to listen for assign ID.
    *
    * Hence, set the power lock if
    * 1. we support LR,
    * 2. the current state is a LR state, and
    * 3. the current state is an include state.
    */
    if (long_range_power_lock == NULL)
    {
      long_range_power_lock = zpal_pm_register(ZPAL_PM_TYPE_USE_RADIO);
    }

    DPRINTF("\nLR PL addr: %u", (uint32_t)long_range_power_lock);
    zpal_pm_stay_awake(long_range_power_lock, 2000);
  }
  else
  {
    /* SDS13826/SDS13944 - Smart Start inclusion request scheduling */
    /* Start timer sending out an explore inclusion request after */
    /* if (requestNo == 0) */
    /*   (random % 120 or 40 (LR)) * 100msec */
    /* else if (requestNo < 6) */
    /*   (16sec << (RequestNo - 1)) + (((random % 120 or 40 (LR)) * 100 msec) << (RequestNo - 1)) */
    /* else if (0 == maxInterval || requestNo == 6) */
    /*   512sec */
    /* else */
    /*   maxInterval * 128sec */
    bRequestNo = NetworkManagementSmartStartCurrentRequestNo();
    DPRINTF("\nLast frame in %d cycle, calculating timeout duration.", bRequestNo);

    if(bRequestNo == 0) {
      dwSmartStartInclusionRequestTimeout = ((uint32_t)(zpal_get_pseudo_random() % NetworkManagementSmartStartRandomGranularity(supportLR)) * NETWORK_MANAGEMENT_ONE_HUNDRED_MSEC_TIMEOUT);
      if(dwSmartStartInclusionRequestTimeout == 0) {
        dwSmartStartInclusionRequestTimeout = NETWORK_MANAGEMENT_ONE_HUNDRED_MSEC_TIMEOUT;
      }
    } else if (bRequestNo < 6) {
      dwSmartStartInclusionRequestTimeout = ((uint32_t)((zpal_get_pseudo_random() % NetworkManagementSmartStartRandomGranularity(supportLR)) << (bRequestNo - 1)) * NETWORK_MANAGEMENT_ONE_HUNDRED_MSEC_TIMEOUT);
      dwSmartStartInclusionRequestTimeout += ((uint32_t)(16 << (bRequestNo - 1)) * NETWORK_MANAGEMENT_ONE_SECOND_TIMEOUT);
    } else if (bRequestNo == 6 || bInclusionRequestIntervals == 0) {
      dwSmartStartInclusionRequestTimeout = LEARN_MODE_SMARTSTART_NWI_MAX_TIMEOUT;
    } else {
      dwSmartStartInclusionRequestTimeout = LEARN_MODE_SMARTSTART_NWI_REQUEST_INTERVAL * bInclusionRequestIntervals;
    }

    //Only enable deep sleep if period between wake ups is longer than the default max timeout.
    if(LEARN_MODE_SMARTSTART_NWI_MAX_TIMEOUT < dwSmartStartInclusionRequestTimeout)
    {
      ZW_mainDeepSleepPowerLockEnable(false);
    }
  }

  /*
   * When the maximum interval is reached, the node must continue a cycle with
   * 1. LR Prime        (0x03)
   * 2. LR Include      (0x04)
   * 3. Explore Prime   (0x01)
   * 4. Explore Include (0x02)
   */
  uint8_t number_of_states = supportLR ?
          NUMBER_OF_STATES_IN_SMART_START_CYCLE_LR :
          NUMBER_OF_STATES_IN_SMART_START_CYCLE;

  /* We have number_of_states steps in every ExploreRequestInclusion transmit Round
   * Explore inclusion SMART_START_STATE_INCLUSION is always the last, both for z-wave and LR
   * if we reached it, increase inclusion counter
   */
  if (SMART_START_STATE_INCLUSION == retRAM.b.smartStartState) {
    // entire inclusion cycle is done -> increase counter
    if (SYSTEM_TYPE_SMART_START_COUNT_MAX > retRAM.b.smartStartCounter) {
      retRAM.b.smartStartCounter += SYSTEM_TYPE_SMART_START_COUNT_INC;
    } else {
      /* MAX retry ExploreRequestInclusion timeout reached - just stop counting and leave the counter as it is.
       * Note that counter never gets reset while inclusion is ongoing */
      DPRINT("\nCounter Max reached!");
    }
  }

  /* Iterate through E_SMART_START_STATES states and go to SMART_START_STATE_PRIME if
   * SMART_START_STATE_INCLUSION (non LR) or SMART_START_STATE_INCLUSION_LR (for LR) is reached
   */
  if (number_of_states == retRAM.b.smartStartState) {
    retRAM.b.smartStartState = SMART_START_STATE_PRIME;
  } else {
    // Increase smartStartMode to go to next value in E_SMART_START_STATES
    retRAM.b.smartStartState += 1;
  }

  /* ZW_ExploreRequestInclusion (with Z-Wave command ZWAVE_CMD_SMARTSTART_PRIME_NODE_INFO) */
  /* transmit (no ACKbit) followed by a 4sec pause and then an ZW_ExploreRequestInclusion */
  /* (with Z-Wave command ZWAVE_CMD_SMARTSTART_INCLUDE_NODE_INFO) transmit (no ACKbit) */
  /* We use the Explore callback in both Explore transmits to kick the Smart Start */
  /* state machine onwards... */

  zpal_retention_register_write(ZPAL_RETENTION_REGISTER_SMARTSTART, retRAM.ram_word);




  DPRINTF("\nTime: %u", dwSmartStartInclusionRequestTimeout);
  /* Start Timer which will at timeout start the Network Management state transition */

  if (NetworkManagementSmartStartIsEnabled() && (learnState <= E_NETWORK_MANAGEMENT_STATE_LEARN_SMARTSTART))
  {
    if (ESWTIMER_STATUS_SUCCESS != TimerStart(pLearnStateTimer, dwSmartStartInclusionRequestTimeout))
    {
      DPRINTF("TimerStart failed!\n");
      ASSERT(0);
    }
    /* Non listening nodes - (sensor nodes) can go to Deep Sleep during smart start to save battery power. */
    /* In that case the callback of pLearnStateTimer will not be called since waking up from deep sleep reboots the program.*/

    if(NetworkManagementSmartStartIsEnabled())
    {
      zpal_pm_cancel(smart_start_power_lock);
    }
  }
}


/*===============   ZCB_NetworkManagementLearnNodeStateTimeout   =============
**    Function description
**      Timeout function that shift learn mode state according to current learn mode
**      if we are in classic mode then switch to NWI else stop learn process
**      Should not be called directly.
**    Side effects:
**
**--------------------------------------------------------------------------*/
static void
ZCB_NetworkManagementLearnNodeStateTimeout(__attribute__((unused)) SSwTimer* pTimer)
{
  STransmitCallback TxCallback = { 0 };
  ESwTimerStatus timerStatus = ESWTIMER_STATUS_FAILED;

  timerStatus = TimerStop(pLearnStateTimer);
  if (ESWTIMER_STATUS_SUCCESS != timerStatus)
  {
    DPRINTF("TimerStop failed!\n");
    ASSERT(0);
  }

  if (learnState == E_NETWORK_MANAGEMENT_STATE_LEARN_CLASSIC)
  {
    InternSetLearnMode(ZW_SET_LEARN_MODE_DISABLE, NULL);
    learnState = E_NETWORK_MANAGEMENT_STATE_LEARN_NWI;
    NetworkManagementLearnStateProcess();
  }
  else if (learnState == E_NETWORK_MANAGEMENT_STATE_LEARN_NWI)
  {
    /* We are doing Network Wide Inclusion/Exclusion */
    if (bIncReqCount)
    {
      //bind TxCallback to NULL
      ZW_TransmitCallbackBind(&TxCallback, NULL, 0);
      if (NW_Action == NW_INCLUSION)
      {
        ZW_ExploreRequestInclusion(&TxCallback);
      }
      else if (NW_Action == NW_EXCLUSION)
      {
        ZW_ExploreRequestExclusion(&TxCallback);
      }

      bIncReqCount--;
      /* Start timer sending out an explore inclusion request after 4 + random sec */
      /* if (bIncReqCount > 0) */
      /*   LEARN_MODE_NWI_TIMEOUT (4sec) + ((random % 70) * 100msec) */
      /* else */
      /*   LEARN_MODE_NWI_TIMEOUT (4sec) */
      timerStatus = TimerStart(pLearnStateTimer,
                 bIncReqCount ?
                 (LEARN_MODE_NWI_TIMEOUT + ((zpal_get_pseudo_random() % LEARN_MODE_NWI_MAX_RANDOM_TIMEOUT) * NETWORK_MANAGEMENT_ONE_HUNDRED_MSEC_TIMEOUT)) :
                 LEARN_MODE_NWI_TIMEOUT);

      if (ESWTIMER_STATUS_SUCCESS != timerStatus)
      {
        DPRINTF("TimerStart failed!\n");
        ASSERT(0);
      }
    }
    else
    {
      learnState = E_NETWORK_MANAGEMENT_STATE_LEARN_STOP;
      NetworkManagementLearnStateProcess();
#ifdef ZW_SLAVE
      /* Return ELEARNSTATUS_LEARN_MODE_COMPLETED_TIMEOUT if the learn process timeout */
      ProtocolInterfacePassToAppLearnStatus(ELEARNSTATUS_LEARN_MODE_COMPLETED_TIMEOUT);
#endif
#ifdef ZW_CONTROLLER
      /* Return nodeID = APPLICATION_NETWORK_LEARN_MODE_COMPLETED_TIMEOUT if the learn process timeout */
      sNetworkManagementLearnInfo.bStatus = LEARN_MODE_FAILED;
      ProtocolInterfacePassToAppLearnStatus(&sNetworkManagementLearnInfo);
#endif
    }
  }
  else if (learnState == E_NETWORK_MANAGEMENT_STATE_LEARN_SMARTSTART)
  {
#if defined(ZW_SLAVE) || defined(ZW_CONTROLLER_TEST_LIB)
    if ( !stateSwitchRegion &&
          NetworkManagementSmartStartIsLR() )
    {
#ifdef ZW_SLAVE
      initialLrChCfg = zpal_radio_get_lr_channel_config();
      llChangeRfPHYToLrChannelConfig(ZPAL_RADIO_LR_CH_CFG3);
#endif
      stateSwitchRegion = true;
    }
    else if ( stateSwitchRegion &&
         !NetworkManagementSmartStartIsLR() )
    {
#ifdef ZW_SLAVE
      llChangeRfPHYToLrChannelConfig(initialLrChCfg);
#endif
      stateSwitchRegion = false;
    }
#endif

    /* Transmit ExploreRequestInclusion Frame */
    ZW_TransmitCallbackBind(&TxCallback, ZCB_NetworkManagementExploreRequestComplete, 0);
    ZW_ExploreRequestInclusion(&TxCallback);
    zpal_pm_stay_awake(smart_start_power_lock, LEARN_MODE_SMARTSTART_EXPLORE_TIMEOUT);
  }
}


/*=====================   NetworkManagementLearnStateProcess   ===============
**    Function description
**      This function do the following:
**        If learnState == E_NETWORK_MANAGEMENT_STATE_LEARN_CLASSIC
**        - Set the Slave/Controller in Learnmode
**        - Starts a one second timeout after which learn mode is disabled
**        - Broadcast the NODEINFORMATION frame once when called.
**      ProtocolInterfacePassToAppLearnStatus() will be called if a controller performs an assignID.
**    Side effects:
**
**--------------------------------------------------------------------------*/
static void
NetworkManagementLearnStateProcess(void)
{
  if (learnState == E_NETWORK_MANAGEMENT_STATE_LEARN_CLASSIC)
  {
    InternSetLearnMode(ZW_SET_LEARN_MODE_CLASSIC, ZCB_NetworkManagementLearnModeCompleted);
#ifdef ZW_SLAVE
    const STransmitCallback TxCallbackNull = { .pCallback = NULL, .Context = 0 };
    if (zpal_radio_is_long_range_locked())
    { //if node is included as LR
      if (NW_EXCLUSION == NW_Action)
      {
        /* Node wants to be excluded. Send Exclude Request command to the controller */
        ZW_SendExcludeRequestLR(&TxCallbackNull);
      }
      else
      {
        /* Else send Node Info command to the controller */
        ZW_SendNodeInformation(NODE_BROADCAST_LR, 0, &TxCallbackNull);
      }
    }
    else
    { //if node is included as classic
      ZW_SendNodeInformation(NODE_BROADCAST, 0, &TxCallbackNull);
    }
#endif
    /* Disable Learn mode after 2 sec and start NWI Learn mode. */
    if (ESWTIMER_STATUS_SUCCESS != TimerStart(pLearnStateTimer, LEARN_MODE_CLASSIC_TIMEOUT))
    {
      DPRINTF("TimerStart failed!\n");
      ASSERT(0);
    }
  }
  else if (learnState == E_NETWORK_MANAGEMENT_STATE_LEARN_NWI)
  {
    dwSmartStartInclusionRequestTimeout = 0;
    if (NW_EXCLUSION == NW_Action)
    {
      InternSetLearnMode(ZW_SET_LEARN_MODE_NWE, ZCB_NetworkManagementLearnModeCompleted);
      ZCB_NetworkManagementLearnNodeStateTimeout(NULL);
    }
    else
    {
      InternSetLearnMode(ZW_SET_LEARN_MODE_NWI, ZCB_NetworkManagementLearnModeCompleted);
      ZCB_NetworkManagementLearnNodeStateTimeout(NULL);
    }
  }
  else if (learnState == E_NETWORK_MANAGEMENT_STATE_LEARN_SMARTSTART)
  {
    /* We are in SMARTSTART but now we start calling ZW_ExploreRequestInclusion() */
    InternSetLearnMode(ZW_SET_LEARN_MODE_SMARTSTART, ZCB_NetworkManagementLearnModeCompleted);
    ZCB_NetworkManagementLearnNodeStateTimeout(NULL);
  }
  else if (learnState == E_NETWORK_MANAGEMENT_STATE_LEARN_STOP)
  {
    InternSetLearnMode(ZW_SET_LEARN_MODE_DISABLE, NULL);

    if (ESWTIMER_STATUS_SUCCESS != TimerStop(pLearnStateTimer))
    {
      DPRINTF("TimerStop failed!\n");
      ASSERT(0);
    }

    NetworkManagementSmartStartClear();
    learnState = E_NETWORK_MANAGEMENT_STATE_LEARN_IDLE;
  }
}


#ifdef ZW_CONTROLLER
void
NetworkManagementStopSetNWI(void)
{
  if (NULL != pNetworkManagementSetNWITimer)
  {
    if (ESWTIMER_STATUS_SUCCESS != TimerStop(pNetworkManagementSetNWITimer))
    {
      DPRINTF("TimerStop failed!\n");
      ASSERT(0);
    }
  }
}


void
NetworkManagementStartSetNWI(void)
{
  /* Only reArm timer and transmit SetNWI if we are in Smart Start Mode */
  if ((NETWORK_WIDE_SMART_START == bNetworkWideInclusion) ||
      (NETWORK_WIDE_SMART_START_NWI == bNetworkWideInclusion))
  {
    if(NULL == pNetworkManagementSetNWITimer)
    {
      ZwTimerRegister(&NetworkManagementSetNWITimer, false, ZCB_NetworkManagementSetNWI);
      pNetworkManagementSetNWITimer = &NetworkManagementSetNWITimer;
    }
    /* Only transmit Explore SetNWI and arm Explore SetNWI timer if no timer allready running */
    if ( !TimerIsActive(pNetworkManagementSetNWITimer))
    {
      const STransmitCallback TxCallback = { .pCallback = 0, .Context = 0 };
      ExploreTransmitSetNWIMode(NWI_REPEAT, &TxCallback);

      if (ESWTIMER_STATUS_SUCCESS != TimerStart(pNetworkManagementSetNWITimer, NETWORK_MANAGEMENT_SET_NWI_TIMEOUT))
      {
        DPRINTF("TimerStart failed!\n");
        ASSERT(0);
      }
    }
  }
}

/*=======================   ZCB_NetworkManagementSetNWI   ====================
 *    Function description
 *      Callback function which keeps the Repeaters in NWI repeat mode
 *
 *    Side effects:
 *
 */
void
ZCB_NetworkManagementSetNWI(__attribute__((unused)) SSwTimer* pTimer)
{
  /* Timer should allready be in stop/free state, but stop it anyway */
  NetworkManagementStopSetNWI();
  NetworkManagementStartSetNWI();
}
#endif


/**
 *
 *
 */
void
NetworkManagementGenerateDskpart(void)
{
  if (g_Dsk[8] == 0)
  {
    keystore_public_dsk_read(g_Dsk);
  }
  g_Dsk[8] |= 0xC0;
  g_Dsk[11] &= 0xFE;
  g_Dsk[12] |= 0xC0;
  g_Dsk[15] &= 0xFE;
}



/**
 *                           EXPORTED FUNCTIONS
 */


/**
 *
 * \ref ZW_NetworkManagementSetMaxInclusionRequestIntervals can be used to set the maximum interval inbetween SmartStart Inclusion requests
 *
 * Declared in: ZW_network_management.h
 *
 * \return 0    Requested maximum intervals either 0(default) or not valid
 * \return 5-99 The requested number of intervals set
 * \param[in] bInclRequestIntervals The maximum number of 128sec ticks inbetween SmartStart inclusion requests.
              Valid range 5-99, which corresponds to 640-12672sec.
 *
 */
uint32_t
ZW_NetworkManagementSetMaxInclusionRequestIntervals(
  uint32_t bNewInterval)
{
  if ((5 <= bNewInterval) && (99 >= bNewInterval))
  {
    bInclusionRequestIntervals = bNewInterval;
  }
  else if (0 == bNewInterval)
  {
    bInclusionRequestIntervals = 0;
  }
  return bInclusionRequestIntervals;
}


bool
NetworkManagementIsNodeInNetwork(void)
{
#ifdef ZW_SLAVE
  if (0 == g_nodeID)
#endif
#ifdef ZW_CONTROLLER
    /* Controller is deemed not in a network if not secondary, not on other network and no nodes included */
  if (NO_NODES_INCLUDED == ((CONTROLLER_IS_SECONDARY |
                            CONTROLLER_ON_OTHER_NETWORK |
                            NO_NODES_INCLUDED) & ZW_GetControllerCapabilities()))
#endif
  {
    return false;
  }
  return true;
}


bool
NetworkManagementIsNodeIncluded(void)
{
#ifdef ZW_SLAVE
  if (0 != g_nodeID)
#endif
#ifdef ZW_CONTROLLER
  /* Controller is deemed included and should transmit INIF if included on other network and NOT SUC */
  if (CONTROLLER_ON_OTHER_NETWORK ==
                          ((CONTROLLER_ON_OTHER_NETWORK |
                            CONTROLLER_IS_SUC |
                            NO_NODES_INCLUDED) & ZW_GetControllerCapabilities()))
#endif
  {
    return true;
  }
  return false;
}


void
NetworkManagementTransmitINIF(void)
{
  if (0 == wakeupReasonHandled)
  {
    wakeupReasonHandled = true;  /* Seen */
    if ((ZPAL_RESET_REASON_POWER_ON == GetResetReason()) && NetworkManagementIsNodeIncluded())
    {
      /* Only if "Power On Reset" and first time called do we emit an INIF when included */
      ExploreINIF();
    }
  }
}


static void
NetworkManagementEnterSmartStart(void)
{
  if((ZPAL_RESET_REASON_DEEP_SLEEP_WUT == GetResetReason()) || (ZPAL_RESET_REASON_DEEP_SLEEP_EXT_INT == GetResetReason()))
  {
    //must register smart_start_power_lock after every deep sleep wakeup
    if (smart_start_power_lock == NULL)
    {
      smart_start_power_lock = zpal_pm_register(ZPAL_PM_TYPE_USE_RADIO);
    }
  }
  else
  {
    URetentionWord_t retRAM = { 0 };
    zpal_retention_register_write(ZPAL_RETENTION_REGISTER_SMARTSTART, retRAM.ram_word);
  }

  //Disable deep sleep until smart start listening interval is over.
  ZW_mainDeepSleepPowerLockEnable(true);

  bSmartStartLearn = true;
  if (false == NetworkManagementSmartStartIsEnabled())
  {
    /* We are now in Smart Start learn mode */
    NetworkManagementSmartStartEnable();

    if(smart_start_power_lock != NULL)
    {
      zpal_pm_cancel(smart_start_power_lock);
    }
    else
    {
      smart_start_power_lock = zpal_pm_register(ZPAL_PM_TYPE_USE_RADIO);
    }
#ifdef ZW_SECURITY_PROTOCOL
      /* Set NVM_SYSTEM_STATE = NVM_SYSTEM_STATE_SMART_START - Used for determining if Local Reset needed on startup */
#ifdef ZW_CONTROLLER
      CtrlStorageSetSmartStartState(NVM_SYSTEM_STATE_SMART_START);
#endif
#ifdef ZW_SLAVE
      SlaveStorageSetSmartStartState(NVM_SYSTEM_STATE_SMART_START);
#endif
#endif
  }
}

void
NetworkManagementRegisterLearnStateTimer(void)
{
  if(NULL == pLearnStateTimer)
  {
    ZwTimerRegister(&LearnStateTimer, false, ZCB_NetworkManagementLearnNodeStateTimeout);
    pLearnStateTimer = &LearnStateTimer;
  }
}

/*========================= ZW_NetworkLearnModeStart =========================
 *
 * \ingroup BASIS
 * \ref ZW_NetworkLearnModeStart is used to enable/disable the Z-Wave Network
 * Node inclusion/exclusion functionality.
 *
 * Declared in: ZW_basis_api.h
 *
 * \return true  Requested Network Learn process initiated
 * \return false
 * \param[out] bMode
      - \ref E_NETWORK_LEARN_MODE_DISABLE       Disable learn process
      - \ref E_NETWORK_LEARN_MODE_INCLUSION     Enable the learn process to do an inclusion
      - \ref E_NETWORK_LEARN_MODE_EXCLUSION     Enable the learn process to do an exclusion
      - \ref E_NETWORK_LEARN_MODE_EXCLUSION_NWE Enable the learn process to do an network wide exclusion
      - \ref E_NETWORK_LEARN_MODE_INCLUSION_SMARTSTART Enable the learn process to do an SMARTSTART inclusion
 *
 * \b Serial API:
 *
 * Not implemented.
 */
uint8_t
ZW_NetworkLearnModeStart(E_NETWORK_LEARN_MODE_ACTION bMode)
{
  bSmartStartLearn = false;

  NetworkManagementRegisterLearnStateTimer();

#ifdef ZW_SECURITY_PROTOCOL
  /* Test if node is in network and persistent marker set, if so SmartStart inclusion must have failed -> local reset. */
  if (NetworkManagementIsNodeInNetwork() &&
      (NVM_SYSTEM_STATE_SMART_START ==
#ifdef ZW_CONTROLLER
       CtrlStorageGetSmartStartState()
#endif
#ifdef ZW_SLAVE
       SlaveStorageGetSmartStartState()
#endif
     ))
  {
#ifdef ZW_CONTROLLER
    memset((uint8_t*)&sNetworkManagementLearnInfo, 0, sizeof(sNetworkManagementLearnInfo));
    sNetworkManagementLearnInfo.bStatus = LEARN_MODE_FAILED;
    sNetworkManagementLearnInfo.bSource = ELEARNSTATUS_LEARN_MODE_COMPLETED_FAILED;
    ProtocolInterfacePassToAppLearnStatus(&sNetworkManagementLearnInfo);
#endif
#ifdef ZW_SLAVE
    ProtocolInterfacePassToAppLearnStatus(ELEARNSTATUS_LEARN_MODE_COMPLETED_FAILED);
#endif
    return false;
  }
  /* Set NVM_SYSTEM_STATE = NVM_SYSTEM_STATE_IDLE - System state now Idle */
#ifdef ZW_CONTROLLER
  CtrlStorageSetSmartStartState(NVM_SYSTEM_STATE_IDLE);
#endif
#ifdef ZW_SLAVE
  SlaveStorageSetSmartStartState(NVM_SYSTEM_STATE_IDLE);
#endif
#endif /*ZW_SECURITY_PROTOCOL*/

#ifdef ZW_SLAVE
  /*Resets the LR mode back to both classic+LR if we have already change it but were not included.*/
  ResetLrChannelConfig();
#endif

  if (E_NETWORK_LEARN_MODE_DISABLE != bMode)
  {
    NW_Action = NW_NONE;
    if (E_NETWORK_MANAGEMENT_STATE_LEARN_IDLE != learnState) /* Learn mode is started, stop it */
    {
      learnState = E_NETWORK_MANAGEMENT_STATE_LEARN_STOP;
      NetworkManagementLearnStateProcess();
    }

    if (E_NETWORK_LEARN_MODE_INCLUSION == bMode)
    {
      NW_Action = NW_INCLUSION;
      NetworkManagementSmartStartClear();
      bIncReqCount = NETWORK_MANAGEMENT_NWI_NWE_REQUEST;
      learnState = E_NETWORK_MANAGEMENT_STATE_LEARN_CLASSIC;
    }
    else if (E_NETWORK_LEARN_MODE_EXCLUSION_NWE == bMode)
    {
      NW_Action = NW_EXCLUSION;
      NetworkManagementSmartStartClear();
      bIncReqCount = NETWORK_MANAGEMENT_NWI_NWE_REQUEST;
      learnState = E_NETWORK_MANAGEMENT_STATE_LEARN_CLASSIC;
    }
    else if (E_NETWORK_LEARN_MODE_INCLUSION_SMARTSTART == bMode)
    {
      /**
       * Test if included, only emit INIF if included :
       *
       * Slave: not included if nodeID == ZERO
       *
       * Controller: not included if: not secondary, not controller on other network
       *             and no other nodes in network.
       *             This means only "included" controllers emit INIF.
       *             The Primary controller do not emit INIF on POR
       *
       * If not included enter the wanted SMARTSTART learnMode
       */
      if (0 == NetworkManagementIsNodeInNetwork())
      {
        NetworkManagementEnterSmartStart();
        learnState = E_NETWORK_MANAGEMENT_STATE_LEARN_SMARTSTART;
      }
      else
      {
        NetworkManagementTransmitINIF();
        NetworkManagementSmartStartClear();
        /* We are included therefore we did not enter specified SMARTSTART learnMode */
        return false;
      }
    }
    else if (E_NETWORK_LEARN_MODE_EXCLUSION == bMode)
    {
      /* Must be E_NETWORK_LEARN_MODE_EXCLUSION - Do Classic Exclusion */
      bIncReqCount = 0;
      NetworkManagementSmartStartClear();
      learnState = E_NETWORK_MANAGEMENT_STATE_LEARN_CLASSIC;
    }
  }
  else
  {
    NW_Action = NW_NONE;
    learnState = E_NETWORK_MANAGEMENT_STATE_LEARN_STOP;
  }
  NetworkManagementLearnStateProcess();
  return (E_NETWORK_MANAGEMENT_STATE_LEARN_IDLE != learnState);
}
