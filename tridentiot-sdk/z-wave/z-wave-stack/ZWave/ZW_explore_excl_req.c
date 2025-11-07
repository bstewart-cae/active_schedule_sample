// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file
 * Z-Wave explore frame exclusion request function
 *
 * @copyright 2021 Silicon Laboratories Inc.
 */

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/

#include "ZW_lib_defines.h"
//#define DEBUGPRINT
#include "DebugPrint.h"

#ifdef ZW_SLAVE
#include <ZW_slave.h>
#endif  /*ZW_SLAVE*/

#ifdef ZW_CONTROLLER
#include <ZW_controller.h>
#endif /*ZW_CONTROLLER*/

/* Commmon for all targets */
#include <ZW_explore.h>
#include "ZW_DataLinkLayer_utils.h"

/****************************************************************************/
/*                      PRIVATE TYPES and DEFINITIONS                       */
/****************************************************************************/

#ifdef ZW_SLAVE
#define NODEINFO_FRAME_DEF      NODEINFO_SLAVE_FRAME
#endif

#ifdef ZW_CONTROLLER
#define NODEINFO_FRAME_DEF      NODEINFO_FRAME
#endif

/****************************************************************************/
/*                              EXTERN DATA                                 */
/****************************************************************************/

// Nothing here.

/****************************************************************************/
/*                          FUNCTION PROTOTYPES                             */
/****************************************************************************/

// Nothing here.

/****************************************************************************/
/*                              EXPORTED FUNCTIONS                          */
/****************************************************************************/

/*=======================   ZW_ExploreRequestExclusion   =====================
**    Transmit a request for exclusion via a explore frame.
**
**
**    Side effects:
**
**--------------------------------------------------------------------------*/
uint8_t
ZW_ExploreRequestExclusion(STransmitCallback* pTxCallback)
{
  uint8_t lNodeInfoLen;

  DPRINT("ExploreRequestExclusion\r\n");
  /* Only send new ExclusionRequest if not in neighbor search and in NWE */
  if (g_learnMode && !g_findInProgress && (bNetworkWideInclusion == NETWORK_WIDE_MODE_LEAVE))
  {
    EXPLORE_REMOTE_EXCLUSION_REQUEST_FRAME exclusionRequest = { 0 };
    lNodeInfoLen = GenerateNodeInformation((NODEINFO_FRAME_DEF*)&exclusionRequest.header.cmdClass,
                                           (LOWEST_LONG_RANGE_NODE_ID <= g_nodeID) ? ZWAVE_CMD_CLASS_PROTOCOL_LR : ZWAVE_CMD_CLASS_PROTOCOL);
    exclusionRequest.header.cmd = ZWAVE_CMD_EXCLUDE_REQUEST;

    // Set input here to make it obvious that it's the same for both calls.
    uint8_t * pData = (uint8_t*)&exclusionRequest;
    uint8_t len = lNodeInfoLen + (sizeof(EXPLORE_REMOTE_EXCLUSION_REQUEST_FRAME) - sizeof(NODEINFO_FRAME_DEF));

    if (HDRFORMATTYP_LR != llGetCurrentHeaderFormat(g_nodeID, false))
    {
      return ExploreQueueFrame(g_nodeID,
                               NODE_BROADCAST,
                               pData,
                               len,
                               (QUEUE_EXPLORE_CMD_NORMAL),
                               pTxCallback);
    }
    else
    {
      return EnQueueSingleData(RF_SPEED_LR_100K,
                               g_nodeID,
                               NODE_BROADCAST_LR,
                               pData,
                               len,
                               TRANSMIT_OPTION_LR_FORCE,
                               0, // 0ms for tx-delay (any value)
                               ZPAL_RADIO_TX_POWER_REDUCED,
                               pTxCallback);
    }
  }
  else
  {
    return(false);
  }
}
