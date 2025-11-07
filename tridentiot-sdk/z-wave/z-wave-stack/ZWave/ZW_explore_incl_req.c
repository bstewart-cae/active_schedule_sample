// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file
 * Z-Wave explore frame inclusion request function
 *
 * @copyright 2021 Silicon Laboratories Inc.
 */

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/

#include "ZW_lib_defines.h"
#ifdef ZW_SLAVE
#include <ZW_slave.h>
#include "ZW_slave_network_info_storage.h"
#endif  /*ZW_SLAVE*/

#ifdef ZW_CONTROLLER
#include <ZW_controller.h>
#endif /*ZW_CONTROLLER*/

/* Commmon for all targets */
#include <ZW_explore.h>

#include <ZW_network_management.h>
#include <zpal_radio.h>

#include <string.h>

/****************************************************************************/
/*                      PRIVATE TYPES and DEFINITIONS                       */
/****************************************************************************/
#ifdef ZW_CONTROLLER
#define SIZEOF_NODEINFO_FRAME  sizeof(NODEINFO_FRAME)
#else
#define SIZEOF_NODEINFO_FRAME  sizeof(NODEINFO_SLAVE_FRAME)
#endif

/****************************************************************************/
/*                              EXTERN DATA                                 */
/****************************************************************************/

extern uint8_t g_Dsk[16];


#ifdef ZW_CONTROLLER
extern bool spoof;
#endif

/****************************************************************************/
/*                          FUNCTION PROTOTYPES                             */
/****************************************************************************/

static uint8_t
GenerateNIF(EXPLORE_REMOTE_INCLUSION_REQUEST_FRAME * inclusionRequest, uint8_t cmdClass)
{
  uint8_t lNodeInfoLen;
  
#ifdef ZW_CONTROLLER
  lNodeInfoLen = GenerateNodeInformation((NODEINFO_FRAME*)&(inclusionRequest->header.cmdClass), cmdClass);
#else
  lNodeInfoLen = GenerateNodeInformation((NODEINFO_SLAVE_FRAME *)&(inclusionRequest->header.cmdClass), cmdClass);
#endif
  inclusionRequest->networkHomeID[0] = 0;
  inclusionRequest->networkHomeID[1] = 0;
  inclusionRequest->networkHomeID[2] = 0;
  inclusionRequest->networkHomeID[3] = 0;

  return lNodeInfoLen;
}

uint8_t
ExploreINIF(void)
{
  STransmitCallback TxCallback;
  ZW_TransmitCallbackBind(&TxCallback, NULL, 0);
  /* Clear exploreRequestBuffer */
  EXPLORE_REMOTE_INCLUDED_NODE_INFORMATION_FRAME includedNodeInformation;
  memset((uint8_t*)&includedNodeInformation, 0, sizeof(EXPLORE_REMOTE_INCLUDED_NODE_INFORMATION_FRAME));
  NetworkManagementGenerateDskpart();
  memcpy(includedNodeInformation.smartStartNWIHomeID, &g_Dsk[8], HOMEID_LENGTH);
  if (zpal_radio_is_long_range_locked())
  {
    includedNodeInformation.header.cmdClass = ZWAVE_CMD_CLASS_PROTOCOL_LR;
    includedNodeInformation.header.cmd = ZWAVE_LR_CMD_INCLUDED_NODE_INFO;
    // Since this is not an explore frame, we skip the first 4 bytes (home ID).
    uint8_t *pData = (uint8_t*)&includedNodeInformation + 4;
    uint8_t len = sizeof(EXPLORE_REMOTE_INCLUDED_NODE_INFORMATION_FRAME) - 4;

    return EnQueueSingleData(RF_SPEED_LR_100K,
                             g_nodeID,
                             NODE_BROADCAST_LR,
                             pData,
                             len,
                             TRANSMIT_OPTION_LR_FORCE,
                             0, // 0ms for tx-delay (any value)
                             ZPAL_RADIO_TX_POWER_REDUCED,
                             &TxCallback);
  }
  else
  {
    includedNodeInformation.header.cmdClass = ZWAVE_CMD_CLASS_PROTOCOL;
    includedNodeInformation.header.cmd = ZWAVE_CMD_INCLUDED_NODE_INFO;
    return ExploreQueueFrame(g_nodeID,
                             NODE_BROADCAST,
                             (uint8_t*)&includedNodeInformation,
                             sizeof(EXPLORE_REMOTE_INCLUDED_NODE_INFORMATION_FRAME),
                             QUEUE_EXPLORE_CMD_AUTOINCLUSION,
                             &TxCallback);
  }
}


/****************************************************************************/
/*                              EXPORTED FUNCTIONS                          */
/****************************************************************************/

/*===========================   ZW_ExploreRequestInclusion   =================
**    Transmit a request for inclusion via an explore frame.
**
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
bool
ZW_ExploreRequestInclusion(STransmitCallback* pTxCallback)
{
  uint8_t bExploreTransmitOptions = QUEUE_EXPLORE_CMD_AUTOINCLUSION;
  uint8_t lNodeInfoLen;

  // Request inclusion only if learn mode is active.
  if (true != g_learnMode) {
    return false;
  }

  // Request inclusion only if we're not currently trying to find neighbors.
  if (false != g_findInProgress) {
    return false;
  }

  /* get Node info and current status from appl. layer */
#ifdef ZW_CONTROLLER
  /* We only Request Inclusion if we're in LearnMode and an ignore Explore frame is not in effect */
  /* TO#2019 fix - only allow reset nodes to transmit ExploreRequestInclusion frames */
  /* TO#2044 fixed - Check for reset was not correct */
  /* TO#2429 fixed - Do not transmit if in findNeighbors */
  if (true == primaryController)
  {
    if (bNetworkWideInclusion != NETWORK_WIDE_MODE_JOIN)
    {
      /* If we are not in JOIN mode allready then clear EEPROM */
      ZW_ClearTables();
    }
    /* TODO - Maybe some more rules for when it should be possible to RequestInclusion - not a member of any network... */
    /* TODO - For controllers we need a unification of NetworkWideInclusion and old school replication */
    /* we need it to be more simple - also more safe from the possibility to destroy an allready ongoing */
    /* inclusion process by requesting a new ZW_ExploreRequestInclusion */
    spoof = true;
    /* Spoof! */
    g_nodeID = NODE_CONTROLLER_OLD;
#else /* #ifdef ZW_CONTROLLER */

  /* Only send new InclusionRequest if in learnMode */
  /* TO#2019 fix - only allow reset nodes to transmit ExploreRequestInclusion frames */
  /* TO#2429 fixed - Do not transmit if in findNeighbors */
  node_id_t t_NodeID;
  SlaveStorageGetNetworkIds(NULL, &t_NodeID);
  if (!t_NodeID && (bNetworkWideInclusion == NETWORK_WIDE_MODE_JOIN))
  {
#endif /* #ifdef ZW_CONTROLLER */

    EXPLORE_REMOTE_INCLUSION_REQUEST_FRAME inclusionRequest = {0};
    uint8_t primeCmd;
    uint8_t includeCmd;
    bool isSmartStartLR = NetworkManagementSmartStartIsLR();

    if (true == isSmartStartLR)
    {
#ifdef ZW_CONTROLLER_TEST_LIB
      g_nodeID = 0;
#endif
      lNodeInfoLen = GenerateNIF(&inclusionRequest, ZWAVE_CMD_CLASS_PROTOCOL_LR);
      primeCmd     = ZWAVE_LR_CMD_SMARTSTART_PRIME_NODE_INFO;
      includeCmd   = ZWAVE_LR_CMD_SMARTSTART_INCLUDE_NODE_INFO;
    }
    else
    {
      lNodeInfoLen = GenerateNIF(&inclusionRequest, ZWAVE_CMD_CLASS_PROTOCOL);
      primeCmd     = ZWAVE_CMD_SMARTSTART_PRIME_NODE_INFO;
      includeCmd   = ZWAVE_CMD_SMARTSTART_INCLUDE_NODE_INFO;
    }

    if (g_learnModeDsk)
    {
      /* We are doing Smart Start inclusion */
      if (0 != NetworkManagementSmartStartIsInclude())
      {
        /* This command is used by the Controller to start the network/security inclusion of specific node */
        inclusionRequest.header.cmd = includeCmd;
        /* ExploreMachine need to wait for ACK timeout, so Inclusion has time to start */
        bExploreTransmitOptions |= TRANSMIT_OPTION_EXPLORE_DO_ACK_TIMEOUT;
      }
      else
      {
        /* This command is used to notify the Controller HOST/Application that a node with specified */
        /* specifics has notified about its presense and the desire to be included into network */
        /* The Controller HOST/Application should then if HomeID matches a DSK in the provisioning list */
        /* initiate Smart Start include with the matching DSK (ZW_AddNodeDskToNetwork) */
        inclusionRequest.header.cmd = primeCmd;
      }
      /* Smart Start Inclusion Request do not set the ACK bit as "old" SDKs uses the ACK to indicate */
      /* if a AutoInclusion frame should potentially be answered with an ASSIGN_ID frame. */
      /* By doing this Smart Start nodes will only be tried included by Controllers in Smart Start mode. */
    }
    else
    {
      bExploreTransmitOptions |= TRANSMIT_OPTION_ACK;
    }

    if (true == isSmartStartLR)
    {
      // Since this is not an explore frame, we skip the first 4 bytes (home ID).
      uint8_t * pData = (uint8_t*)&inclusionRequest + 4;
      uint8_t len = lNodeInfoLen + (sizeof(EXPLORE_REMOTE_INCLUSION_REQUEST_FRAME) - SIZEOF_NODEINFO_FRAME) - 4;

      // Transmit pData frame on both LR channels
      return EnQueueSingleDataOnLRChannels(RF_SPEED_LR_100K,
                                          g_nodeID,
                                          NODE_BROADCAST_LR,
                                          pData,
                                          len,
                                          0, // 0ms for tx-delay (any value)
                                          ZPAL_RADIO_TX_POWER_REDUCED,
                                          pTxCallback);
    }
    else
    {
      return ExploreQueueFrame((uint8_t)g_nodeID,
                               NODE_BROADCAST,
                               (uint8_t*)&inclusionRequest,
                               lNodeInfoLen + (sizeof(EXPLORE_REMOTE_INCLUSION_REQUEST_FRAME) - SIZEOF_NODEINFO_FRAME),
                               bExploreTransmitOptions,
                               pTxCallback);
    }
  }
  else
  {
    return(false);
  }
}
