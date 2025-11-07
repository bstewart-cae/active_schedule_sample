// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file ZW_node.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include <ZW_node.h>
#include <ZW_basis_api.h>
#include <string.h>
//#define DEBUGPRINT
#include "DebugPrint.h"

/************************************************************************************
 * The current node's "Node attributes parameters"
 ***********************************************************************************/

static uint8_t m_homeID[HOMEID_LENGTH];  // This works as read cache for the homeID stored in NMV3.

node_id_t g_nodeID;                      // The current node's Node-ID.

static uint8_t bldeviceOptionsMask;

/************************************************************************************
 * API functions
 ***********************************************************************************/

void
ZW_nodeDeviceOptionsSet(uint8_t newDeviceOptions)
{
  bldeviceOptionsMask = newDeviceOptions;
}


uint8_t
ZW_nodeDeviceOptionsGet(void)
{
  return bldeviceOptionsMask;
}


uint8_t
ZW_nodeIsFLiRS(void)
{
  uint8_t retVal = bldeviceOptionsMask & (APPLICATION_FREQ_LISTENING_MODE_1000ms | APPLICATION_FREQ_LISTENING_MODE_250ms);
  return retVal;
}


uint8_t
ZW_nodeIsListening(void)
{
  uint8_t retVal = bldeviceOptionsMask & APPLICATION_NODEINFO_LISTENING;
  return retVal;
}


bool ZW_nodeIsLRNodeID(node_id_t nodeID)
{
  if((LOWEST_LONG_RANGE_NODE_ID <= nodeID) && (HIGHEST_LONG_RANGE_NODE_ID >= nodeID))
  {
    return true;
  }
  return false;
}


void ZW_HomeIDClear(void)
{
  DPRINT("ZW_HomeIDClear()\n");
  memset(m_homeID, 0, HOMEID_LENGTH);
}

void ZW_HomeIDSet(uint8_t* homeID)
{
  DPRINTF("ZW_HomeIDSet: 0x%X%X%X%X\n", homeID[3], homeID[2], homeID[1], homeID[0]);
  memcpy(m_homeID, homeID, HOMEID_LENGTH);
}

uint8_t* ZW_HomeIDGet(void)
{
  DPRINTF("ZW_HomeIDGet: 0x%X%X%X%X\n", m_homeID[3], m_homeID[2], m_homeID[1], m_homeID[0]);
  return m_homeID;
}

void ZW_NodeIDSet(node_id_t assignedNodeID)
{
  DPRINTF("ZW_NodeIDSet: 0x%X\n", assignedNodeID);
  g_nodeID = assignedNodeID;
}

node_id_t ZW_NodeIDGet(void)
{
  DPRINTF("ZW_NodeIDGet: 0x%X\n", g_nodeID);
  return g_nodeID;
}
