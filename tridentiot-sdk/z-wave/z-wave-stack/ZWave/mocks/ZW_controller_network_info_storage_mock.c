// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file ZW_controller_network_info_storage_mock.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include <ZW_controller_network_info_storage.h>
#include <stdint.h>
#include <mock_control.h>
#include <ZW_typedefs.h>
#include <ZW_transport.h>
#include <string.h>

#define MOCK_FILE "ZW_controller_network_info_storage_mock.c"

#define MAX_NUMBER_OF_NODES 4000

bool
CtrlStorageGetBridgeNodeFlag(
  uint16_t nodeID)
{
  mock_t * pMock;
  MOCK_CALL_RETURN_IF_USED_AS_STUB(false);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(pMock, 0xFF);
  MOCK_CALL_ACTUAL(pMock, nodeID);
  MOCK_CALL_COMPARE_INPUT_UINT8(pMock, ARG0, nodeID);
  MOCK_CALL_RETURN_VALUE(pMock, bool);
}

void
CtrlStorageSetTxPower(node_id_t node_id, int8_t tx_power)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(pMock);

  MOCK_CALL_ACTUAL(pMock, node_id, tx_power);

  MOCK_CALL_COMPARE_INPUT_UINT16(pMock, ARG0, node_id);
  MOCK_CALL_COMPARE_INPUT_INT8(pMock, ARG1, tx_power);
}


int8_t
CtrlStorageGetTxPower(node_id_t node_id)
{
  mock_t * pMock;
  MOCK_CALL_RETURN_IF_USED_AS_STUB(0x0F);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(pMock, 14);  // DYNAMIC_TX_POWR_MAX
  MOCK_CALL_ACTUAL(pMock, node_id);

  MOCK_CALL_COMPARE_INPUT_UINT16(pMock, ARG0, node_id);

  MOCK_CALL_RETURN_VALUE(pMock, uint8_t);
}

void CtrlStorageSetCtrlSucUpdateIndex(node_id_t nodeID , uint8_t CtrlSucUpdateIndex)
{

}

void CtrlStorageSetSucLastIndex(uint8_t  SucLastIndex)
{

}

void CtrlStorageSetSucUpdateEntry(uint8_t bSucUpdateIndex, uint8_t bChangeType, node_id_t bNodeID)
{

}

void CtrlStorageSetCmdClassInSucUpdateEntry(uint8_t bSucUpdateIndex, uint8_t * cmdClasses)
{

}

uint8_t CtrlStorageGetCtrlSucUpdateIndex(node_id_t nodeID)
{
  return 0;
}

/* *************************************************************************************************
 * Pending update table
 * *************************************************************************************************
 */

static bool pending_updates[ZW_MAX_NODES_LR];

void reset_pending_update_table(void)
{
  for (uint32_t i = 0; i < ZW_MAX_NODES_LR; i++)
  {
    pending_updates[i] = false;
  }
}

bool CtrlStorageIsNodeInPendingUpdateTable_FAKE(node_id_t bNodeID)
{
  return pending_updates[bNodeID];
}

bool CtrlStorageIsNodeInPendingUpdateTable(node_id_t bNodeID)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(0);
  MOCK_CALL_RETURN_IF_USED_AS_FAKE(CtrlStorageIsNodeInPendingUpdateTable_FAKE, bNodeID);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, 0);
  MOCK_CALL_ACTUAL(p_mock, bNodeID);

  MOCK_CALL_COMPARE_INPUT_UINT16(p_mock, ARG0, bNodeID);

  MOCK_CALL_RETURN_VALUE(p_mock, bool);
}

void CtrlStorageChangeNodeInPendingUpdateTable_FAKE(node_id_t bNodeID, bool isPending)
{
  pending_updates[bNodeID] = isPending;
}

void CtrlStorageChangeNodeInPendingUpdateTable(node_id_t bNodeID, bool isPending)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_RETURN_IF_USED_AS_FAKE(CtrlStorageChangeNodeInPendingUpdateTable_FAKE, bNodeID, isPending);
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(p_mock);
  MOCK_CALL_ACTUAL(p_mock, bNodeID, isPending);

  MOCK_CALL_COMPARE_INPUT_UINT16(p_mock, ARG0, bNodeID);
  MOCK_CALL_COMPARE_INPUT_BOOL(p_mock, ARG1, isPending);
}

void CtrlStorageWriteBridgeNodePool(uint8_t *bridgeNodePool)
{

}

uint8_t CtrlStorageGetSucLastIndex(void)
{
  return 0;
}

node_id_t CtrlStorageGetMaxNodeId(void)
{
  return 0;
}

node_id_t CtrlStorageGetMaxLongRangeNodeId(void)
{
  return 0;
}

uint8_t CtrlStorageGetControllerConfig(void)
{
  return 0;
}

/* *************************************************************************************************
 * Set and get static controller node ID
 * *************************************************************************************************
 */
static node_id_t static_controller_node_id;

void reset_static_controller_node_id(void)
{
  static_controller_node_id = 0;
}

node_id_t CtrlStorageGetStaticControllerNodeId_FAKE(void)
{
  return static_controller_node_id;
}

node_id_t CtrlStorageGetStaticControllerNodeId(void)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(0);
  MOCK_CALL_RETURN_IF_USED_AS_FAKE(CtrlStorageGetStaticControllerNodeId_FAKE);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, 0);
  MOCK_CALL_RETURN_VALUE(p_mock, node_id_t);
}

void CtrlStorageSetStaticControllerNodeId_FAKE(node_id_t  StaticControllerNodeId)
{
  static_controller_node_id = StaticControllerNodeId;
}

void CtrlStorageSetStaticControllerNodeId(node_id_t  StaticControllerNodeId)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_RETURN_IF_USED_AS_FAKE(CtrlStorageSetStaticControllerNodeId_FAKE, StaticControllerNodeId);
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(p_mock);
  MOCK_CALL_ACTUAL(p_mock, StaticControllerNodeId);
}

void CtrlStorageSetMaxLongRangeNodeId(node_id_t  MaxNodeId_LR)
{
}

void CtrlStorageReadBridgeNodePool(uint8_t *bridgeNodePool)
{

}

bool CtrlStorageSetBridgeNodeFlag(node_id_t nodeID , bool IsBridgeNode, bool writeToNVM)
{
  return true;
}

void ControllerStorageGetNetworkIds(uint8_t *pHomeID, node_id_t *pNodeID)
{

}

void CtrlStorageSetReservedId(node_id_t  ReservedId)
{

}

void CtrlStorageSetMaxNodeId(node_id_t  MaxNodeId)
{

}

node_id_t CtrlStorageGetNodeID(void)
{
  return 0;
}

bool CtrlStorageGetRoutingSlaveSucUpdateFlag(node_id_t nodeID)
{
  return true;
}

node_id_t CtrlStorageGetLastUsedLongRangeNodeId(void)
{
  return 0;
}

void CtrlStorageSetLastUsedLongRangeNodeId(node_id_t  LastUSedNodeID_LR)
{

}

/* *************************************************************************************************
 * Set and get last used node ID
 * *************************************************************************************************
 */
static node_id_t last_used_node_id;

void reset_last_used_node_id(void)
{
  last_used_node_id = 1; // Default value MUST be 1 according to production source.
}

node_id_t CtrlStorageGetLastUsedNodeId_FAKE(void)
{
  return last_used_node_id;
}

node_id_t CtrlStorageGetLastUsedNodeId(void)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(0);
  MOCK_CALL_RETURN_IF_USED_AS_FAKE(CtrlStorageGetLastUsedNodeId_FAKE);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, 0);
  MOCK_CALL_RETURN_VALUE(p_mock, node_id_t);
}

void CtrlStorageSetLastUsedNodeId_FAKE(node_id_t  LastUSedNodeID)
{
  last_used_node_id = LastUSedNodeID;
}

void CtrlStorageSetLastUsedNodeId(node_id_t  LastUSedNodeID)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_RETURN_IF_USED_AS_FAKE(CtrlStorageSetLastUsedNodeId_FAKE, LastUSedNodeID);
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(p_mock);
  MOCK_CALL_ACTUAL(p_mock, LastUSedNodeID);

  MOCK_CALL_COMPARE_INPUT_UINT16(p_mock, ARG0, LastUSedNodeID);
}

node_id_t CtrlStorageGetReservedId(void)
{
  return 0;
}

bool CtrlStorageCapabilitiesSpeed40kNodeGet(node_id_t nodeID)
{
  return true;
}

bool CtrlStorageCacheCapabilitiesSpeed100kNodeGet(node_id_t nodeID)
{
  return true;
}

void CtrlStorageCacheCapabilitiesSpeed100kNodeSet(node_id_t nodeID, bool value)
{

}

/* *************************************************************************************************
 * Node info
 * *************************************************************************************************
 */

static bool nodeinfo_exists[MAX_NUMBER_OF_NODES] = {false};
static EX_NVM_NODEINFO nodeinfo[MAX_NUMBER_OF_NODES] = {0};

void reset_node_info(void)
{
  for (uint32_t i = 0; i < MAX_NUMBER_OF_NODES; i++)
  {
    nodeinfo_exists[i] = false;
  }
  memset(nodeinfo, 0, sizeof(EX_NVM_NODEINFO) * MAX_NUMBER_OF_NODES);
}

bool CtrlStorageCacheNodeExist_FAKE(node_id_t nodeID)
{
  return nodeinfo_exists[nodeID];
}

bool CtrlStorageCacheNodeExist(node_id_t nodeID)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(0);
  MOCK_CALL_RETURN_IF_USED_AS_FAKE(CtrlStorageCacheNodeExist_FAKE, nodeID);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, 0);
  MOCK_CALL_ACTUAL(p_mock, nodeID);

  MOCK_CALL_COMPARE_INPUT_UINT16(p_mock, ARG0, nodeID);

  MOCK_CALL_RETURN_VALUE(p_mock, bool);
}

void CtrlStorageGetNodeInfo_FAKE(node_id_t nodeID , EX_NVM_NODEINFO* pNodeInfo)
{
  memcpy(pNodeInfo, &nodeinfo[nodeID], sizeof(EX_NVM_NODEINFO));
}

void CtrlStorageGetNodeInfo(node_id_t nodeID , EX_NVM_NODEINFO* pNodeInfo)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_RETURN_IF_USED_AS_FAKE(CtrlStorageGetNodeInfo_FAKE, nodeID, pNodeInfo);
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(p_mock);
  MOCK_CALL_ACTUAL(p_mock, nodeID, pNodeInfo);

  MOCK_CALL_COMPARE_INPUT_UINT16(p_mock, ARG0, nodeID);
  MOCK_CALL_COMPARE_INPUT_POINTER(p_mock, ARG1, pNodeInfo);
}

void CtrlStorageSetNodeInfo_FAKE(node_id_t nodeID , EX_NVM_NODEINFO* pNodeInfo)
{
  nodeinfo_exists[nodeID] = true;
  memcpy((uint8_t *)&nodeinfo[nodeID], pNodeInfo, sizeof(EX_NVM_NODEINFO));
}

void CtrlStorageSetNodeInfo(node_id_t nodeID , EX_NVM_NODEINFO* pNodeInfo)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_RETURN_IF_USED_AS_FAKE(CtrlStorageSetNodeInfo_FAKE, nodeID, pNodeInfo);
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(p_mock);
  MOCK_CALL_ACTUAL(p_mock, nodeID, pNodeInfo);

  MOCK_CALL_COMPARE_INPUT_UINT16(p_mock, ARG0, nodeID);
  MOCK_CALL_COMPARE_INPUT_POINTER(p_mock, ARG1, pNodeInfo);
}

void CtrlStorageRemoveNodeInfo(node_id_t nodeID, bool keepRouteCache)
{

}

void CtrlStorageSetControllerConfig(uint8_t  controllerConfig)
{

}

void ControllerStorageSetNetworkIds(const uint8_t *pHomeID, node_id_t nodeID)
{

}

void CtrlStorageSetRoutingInfo(node_id_t nodeID , NODE_MASK_TYPE* pRoutingInfo, bool writeToNVM)
{

}

void StorageSetPrimaryLongRangeChannelId(zpal_radio_lr_channel_t channelId)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(p_mock);
  MOCK_CALL_ACTUAL(p_mock, channelId);

  MOCK_CALL_COMPARE_INPUT_UINT8(p_mock, ARG0, channelId);
}


zpal_radio_lr_channel_t StorageGetPrimaryLongRangeChannelId(void)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(0x01);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, 0x00);

  MOCK_CALL_RETURN_VALUE(p_mock, zpal_radio_lr_channel_t);
}

void StorageSetLongRangeChannelAutoMode(bool enable)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(p_mock);
  MOCK_CALL_ACTUAL(p_mock, enable);

  MOCK_CALL_COMPARE_INPUT_UINT8(p_mock, ARG0, enable);
}

bool StorageGetLongRangeChannelAutoMode(void)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(0x01);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, 0x00);

  MOCK_CALL_RETURN_VALUE(p_mock, bool);
}

/* *************************************************************************************************
 * Slave nodes
 * *************************************************************************************************
 */
bool CtrlStorageSlaveNodeGet_FAKE(node_id_t nodeID)
{
  return !(nodeinfo[nodeID].security & ZWAVE_NODEINFO_CONTROLLER_NODE);
}

bool CtrlStorageSlaveNodeGet(node_id_t nodeID)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(0);
  MOCK_CALL_RETURN_IF_USED_AS_FAKE(CtrlStorageSlaveNodeGet_FAKE, nodeID);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, 0);
  MOCK_CALL_ACTUAL(p_mock, nodeID);
  MOCK_CALL_RETURN_VALUE(p_mock, bool);
}

bool CtrlStorageSetRoutingSlaveSucUpdateFlag(node_id_t nodeID , bool SlaveSucUpdate, bool writeToNVM)
{
  return true;
}

void CtrlStorageGetSucUpdateEntry(uint8_t bSucUpdateIndex, SUC_UPDATE_ENTRY_STRUCT *SucUpdateEntry)
{

}

/* *************************************************************************************************
 * Listening nodes
 * *************************************************************************************************
 */
bool CtrlStorageListeningNodeGet_FAKE(node_id_t nodeID)
{
  return nodeinfo[nodeID].capability & ZWAVE_NODEINFO_LISTENING_SUPPORT;
}

bool CtrlStorageListeningNodeGet(node_id_t nodeID)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(0);
  MOCK_CALL_RETURN_IF_USED_AS_FAKE(CtrlStorageListeningNodeGet_FAKE, nodeID);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, 0);
  MOCK_CALL_ACTUAL(p_mock, nodeID);

  MOCK_CALL_COMPARE_INPUT_UINT16(p_mock, ARG0, nodeID);

  MOCK_CALL_RETURN_VALUE(p_mock, bool);
}

/* *************************************************************************************************
 * Sensor nodes
 * *************************************************************************************************
 */
bool CtrlStorageSensorNodeGet_FAKE(node_id_t nodeID)
{
  return nodeinfo[nodeID].security & ZWAVE_NODEINFO_SENSOR_MODE_MASK;
}

bool CtrlStorageSensorNodeGet(node_id_t nodeID)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(false);
  MOCK_CALL_RETURN_IF_USED_AS_FAKE(CtrlStorageSensorNodeGet_FAKE, nodeID);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, false);
  MOCK_CALL_ACTUAL(p_mock, nodeID);

  MOCK_CALL_COMPARE_INPUT_UINT16(p_mock, ARG0, nodeID);

  MOCK_CALL_RETURN_VALUE(p_mock, bool);
}
