// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file ZW_controller_network_info_storage.h
 * Functions to store controller related information to the file system.
 *
 * The controller information is organized into 4 files categories.
 *   1. The node information files.
 *      There can be up to 232 files depending on the number of nodes included in the network
 *      Each file is organized as follow:
 *      typedef struct SNodeInfo
 *      {
 *        EX_NVM_NODEINFO NodeInfo;
 *        NODE_MASK_TYPE neighboursInfo;
 *        uint8_t ControllerSucUpdateIndex;
 *        ROUTECACHE_LINE  routeCache;
 *        ROUTECACHE_LINE  routeCacheNlwrSr;
 *        uint8_t nodeFlags;
 *       } SNodeInfo
 *
 *       NodeInfo contains information obtained from the nodes info frame. The field size is 5 bytes.
 *       neighboursInfo contains the node's neighboring nodes. The field size is 29 bytes
 *       ControllerSucUpdateIndex holds the last SUC update index The field size is 1 byte
 *       routeCache holds the last working route from the controller to the node. The field size is 5 bytes
 *       routeCacheNlwrSr holds next last working route. The field size is 5 bytes
 *       nodeFlags a bit mask have the following definition. The field size is 1 byte
 *         Bit 0: App route Lock
 *         Bit 1: Route slave  SUC flag, If 1 then the nodes need a new returns route
 *         Bit 2: SUC pending Update. If 1 then the node info information need to be sent tot eh SUC
 *         Bit 3: Bride Node, If 1 then the node is virtual
 *         Bit 4: Pending discovery, If 1 then the nodes range info need update
 *   2. Preferred repeaters file.
 *      There are only one file and it is created at power up if it doesn't exist yet
 *      The file size is 29 bytes
 *   3. SUC updates.
 *      There are only one file and it is created at power up if it doesn't exist yet.
 *      The contains 64 of network update entries. Each entry size is 22 bytes, Thus file size is 1408 bytes.
 *   4. Controller information
 *      There are only one file and it is created at power up if it doesn't exist yet.
 *      The file size is 7 bytes
 *   This module provides function to read write this information to the fule system
 *
 * @copyright 2022 Silicon Laboratories Inc.
 */

#ifndef ZWAVE_ZW_CONTROLLER_NETWORK_INFO_STORAGE_H_
#define ZWAVE_ZW_CONTROLLER_NETWORK_INFO_STORAGE_H_

#include <stdint.h>
#include "ZW_lib_defines.h"
#ifdef UNITY_TEST
#define ZW_CONTROLLER_BRIDGE
#undef  NO_PREFERRED_CALC
#endif

#include "ZW_transport_api.h"
#include "NodeMask.h"

#include "ZW_keystore.h"

#define RETURN_ROUTE_MAX  (4)   /* NOTE: The same define exists in ZW_slave_network_info_storage.h */

#define EX_NVM_NODE_INFO_SIZE        (sizeof(EX_NVM_NODEINFO))  /* Node table */

#define SUC_UPDATE_NODEPARM_MAX  20   /* max. number of command classes in update list */

/* Node table */
#define EX_NVM_NODE_INFO_SIZE        (sizeof(EX_NVM_NODEINFO))

/* Max number of updates stored */
#define SUC_MAX_UPDATES          64

//Dummy node ID used to write flags for all nodes to NVM
#define WRITE_ALL_NODES_TO_NVM  251

//The version number of the controller protocol file system. Valid range: 0-255
//Used to trigger migration of files between file system versions.
#define ZW_CONTROLLER_FILESYS_VERSION  5

//The version number of the unify controller file system. Valid range: 0-255
//Used to trigger migration of files between file system versions.
#define ZW_HOST_SECURITY_FILESYS_VERSION        0  // Initial version was 0. (ControllerPortable beta)


typedef struct _ex_nvm_nodeinfo_
{
  uint8_t capability;       /* Network capabilities */
  uint8_t security;         /* Network security */
  uint8_t reserved;
  uint8_t generic;          /* Generic Device Type */
  uint8_t specific;         /* Specific Device Type */
} EX_NVM_NODEINFO;

typedef struct _suc_update_entry_struct_
{
  uint8_t      NodeID;
  uint8_t      changeType;
  uint8_t      nodeInfo[SUC_UPDATE_NODEPARM_MAX]; /* Device status */
} SUC_UPDATE_ENTRY_STRUCT;

typedef struct _routecache_line_
{
  uint8_t repeaterList[MAX_REPEATERS]; /* List of repeaters */
  uint8_t routecacheLineConfSize;
} ROUTECACHE_LINE;


/**
 * Route types  for \ref GetRouteCache
 */
typedef enum ROUTE_CACHE_TYPE
{
	/**
	 * Normal last working route
	*/
  ROUTE_CACHE_NORMAL = 0,
  /**
   * Next Last working route
  */
  ROUTE_CACHE_NLWR_SR = 1,
  NUM_ROUTE_CACHE_TYPE
} ROUTE_CACHE_TYPE;

 /**
    * Read a saved LWR to the node data file
    * If the node does not exist the route will be empty
    *
    * @param[in]    cacheType   	Which LWR to read
    * @param[in]    nodeID   		The node ID for the node we want to read its LWR
    * @param[out] 	pRouteCache     A buffer to To read the LWR into it.
    *
    * @return None
    */
void CtrlStorageGetRouteCache(ROUTE_CACHE_TYPE cacheType, node_id_t nodeID , ROUTECACHE_LINE  *pRouteCache);

 /**
    * write a LWR to node data file
    *
    * @param[in]    cacheType   	Which LWR to store
    * @param[in]    nodeID   		The node ID for the node we want to store its LWR
    * @param[in] 	pRouteCache     A buffer to To read the LWR from it.
    *
    * @return None
    */
void CtrlStorageSetRouteCache(ROUTE_CACHE_TYPE cacheType, node_id_t nodeID , ROUTECACHE_LINE*  pRouteCache);

/**
    * Writes all entries of the RouteCache RAM buffer to non volatile memory
    * Should be called at controlled reset of the system.
    *
    */
void StoreNodeRouteCacheBuffer(void);

/**
    * Writes one file of the RouteCache RAM buffer to non volatile memory
    *
    * @param[in]    nodeID      The file containing data for the node with nodeID is saved
    *
    */
void StoreNodeRouteCacheFile(node_id_t nodeID);

/**
    * Read a node information from a node data file
    * If the node does not exist the node info will be zero
    *
    * @param[in]     nodeID   The node ID for the node we want to read the node info
    * @param[out]    pNodeInfo     A buffer to To read the node info into it.
    *
    * @return None
    */
void CtrlStorageGetNodeInfo(node_id_t nodeID , EX_NVM_NODEINFO* pNodeInfo);

/**
    * Writea a node information to a node data file
    *
    * @param[in]     nodeID   The node ID for the node we want to read the node info
    * @param[int] pNodeInfo     A buffer to To read the node info from it.
    *
    * @return None
    */

void CtrlStorageSetNodeInfo(node_id_t nodeID , EX_NVM_NODEINFO* pNodeInfo);

/**
    * delete a node data file
    *
    * @param[in]     nodeID   The node ID for the node we want to delete its node data file
    * @param[in]     keepRouteCache   false if existing RouteCache for nodeID must be removed
    *
    * @return None
    */

void CtrlStorageRemoveNodeInfo(node_id_t nodeID, bool keepRouteCache);

/**
    * Read a controller SUC update index from a node data file
    * If it doesn't exist the value SUC_UNKNOWN_CONTROLLER will be returned
    * this flag is valid when the node is a controller node
    *
    * @param[in]     nodeID   The node ID for the node we want to read controller SUC update index
    *
    * @return the node controller SUC update index
    */
uint8_t CtrlStorageGetCtrlSucUpdateIndex(node_id_t nodeID);

 /**
    * write the controller SUC update index to the node data file
    * this flag is valid when the node is a controller node
    *
    * @param[in]     nodeID   The node ID for the node we want to store its controller SUC update index
    * @param[in] CtrlSucUpdateIndex     The controller SUC update index value we want to write to the node data file
    *
    * @return None
    */
void CtrlStorageSetCtrlSucUpdateIndex(node_id_t nodeID , uint8_t CtrlSucUpdateIndex);


 /**
    * Read the routing slave SUC update flag from the node data file
    * this flag is valid when the node is a slave node
    *
    * @param[in]     nodeID  The node ID for the node we want to read the routing SUC update flag
    *
    * @return the value of the flag
    */
bool CtrlStorageGetRoutingSlaveSucUpdateFlag(node_id_t nodeID);

/**
    * writes the routing slave SUC update flag to the node data file
    * this flag is valid when the node is a slave node
    *
    * @param[in]     nodeID  The node ID for the node we want to write the routing SUC update flag in its node data file
    * @param[in]     SlaveSucUpdate  The flag value we want to write in the node data file
    * @param[in]     writeToNVM  Save flags to Non Volatile Memory
    * @return true if flag actually updated
    */
bool CtrlStorageSetRoutingSlaveSucUpdateFlag(node_id_t nodeID , bool SlaveSucUpdate, bool writeToNVM);

/**
    * Read the routing range information from the node data file
    *
    * @param[in]     nodeID  The node ID for the node we want to read the routing information
    * @param[out]     pRoutingInfo  Buffer to put the reouting information we read from the node data file
    */
void CtrlStorageGetRoutingInfo(node_id_t nodeID , NODE_MASK_TYPE* pRoutingInfo);

/**
    * Check if the routing range information file is ready to be written to NVM
    *
    * @param[in]     nodeID  The ID for the node we want to write the routing information to its node data file
    * @param[in]     maxNodeID  The max ID for the nodes we want to write to file
    *
    * @return true if Routing info file is full and ready to be written to NVM or nodeID == maxNodeID
    */
bool CtrlStorageRoutingInfoFileIsReady(node_id_t nodeID, node_id_t maxNodeID);

/**
    * Write the routing range information to the node data file
    *
    * @param[in]     nodeID  The ID for the node we want to write the routing information to its node data file
    * @param[in]     pRoutingInfo  Buffer for the routing information we want to write to the node data file
    * @param[in]     writeToNVM  Save flags to Non Volatile Memory
    */
void CtrlStorageSetRoutingInfo(node_id_t nodeID , NODE_MASK_TYPE* pRoutingInfo, bool writeToNVM);

/**
    * Read the application route lock flag from the node data file
    *
    * @param[in]     nodeID  The node ID for the node we want to read the application route lock flag
    *
    * @return the value of the flag
    */
bool CtrlStorageGetAppRouteLockFlag(node_id_t nodeID);

/**
    * Write the application route lock flag to the node data file
    *
    * @param[in]     nodeID  The ID for the node we want to write the application route lock flag to its node data file
    * @param[in]     LockRoute  The value of the flag
    *
    * @return the value of the flag
    */
void CtrlStorageSetAppRouteLockFlag(node_id_t nodeID , bool LockRoute);

/**
    * Read the controller configuartion field from the controller info file
    *
    * @return the value of the field
    */
uint8_t CtrlStorageGetControllerConfig(void);

/**
    * write the controller configuartion field to the controller info file
    *
    * @param[in]     controllerConfig  The conttroller configuartion value to write to the controller info file
    *
    * @return None.
    */
void CtrlStorageSetControllerConfig(uint8_t  controllerConfig);


/**
    * Read the SUC last index field from the controller info file
    *
    * @return the value of the field
    */
uint8_t CtrlStorageGetSucLastIndex(void);

/**
    * write the SUC last index field to the controller info file
    *
    * @param[in]     SucLastIndex  The SUC last index value to write to the controller info file
    *
    * @return None.
    */
void CtrlStorageSetSucLastIndex(uint8_t  SucLastIndex);


/**
    * Read the last used Z-Wave Long Range node ID field from the controller info file
    *
    * @return the value of the field
    */
node_id_t CtrlStorageGetLastUsedLongRangeNodeId(void);

/**
    * write the Last Used Z-Wave Long Range nodeID field to the controller info file
    *
    * @param[in]     LastUSedNodeID  The last used node ID value to write to the controller info file
    *
    * @return None.
    */
void CtrlStorageSetLastUsedLongRangeNodeId(node_id_t  LastUSedNodeID_LR);

/**
    * Read the last used node ID field from the controller info file
    *
    * @return the value of the field
    */
node_id_t CtrlStorageGetLastUsedNodeId(void);

/**
    * write the Last Used nodeID field to the controller info file
    *
    * @param[in]     LastUSedNodeID  The last used node ID value to write to the controller info file
    *
    * @return None.
    */
void CtrlStorageSetLastUsedNodeId(node_id_t  LastUSedNodeID);


/**
    * Read the SUC node ID field from the controller info file
    *
    * @return the value of the field
    */

node_id_t CtrlStorageGetStaticControllerNodeId(void);

/**
    * write the SUC nodeID field to the controller info file
    *
    * @param[in]     StaticControllerNodeId  The SUC nodeID value to write to the controller info file
    *
    * @return None.
    */
void CtrlStorageSetStaticControllerNodeId(node_id_t  StaticControllerNodeId);

/**
    * Read the max Z-Wave Long Range node ID field from the controller info file
    *
    * @return the value of the field
    */

node_id_t CtrlStorageGetMaxLongRangeNodeId(void);

/**
    * write the Max Z-Wave Long Range node ID field to the controller info file
    *
    * @param[in]    MaxNodeId  The Max node ID value to write to the controller info file
    *
    * @return None.
    */
void CtrlStorageSetMaxLongRangeNodeId(node_id_t  MaxNodeId_LR);

/**
    * Read the max node ID field from the controller info file
    *
    * @return the value of the field
    */

node_id_t CtrlStorageGetMaxNodeId(void);

/**
    * write the Max node ID field to the controller info file
    *
    * @param[in]    MaxNodeId  The Max node ID value to write to the controller info file
    *
    * @return None.
    */
void CtrlStorageSetMaxNodeId(node_id_t  MaxNodeId);

/**
    * Read the Z-Wave Long Range reserved ID field from the controller info file
    *
    * @return the value of the field
    */

node_id_t CtrlStorageGetReservedLongRangeId(void);

/**
    * write the Z-Wave Long Range reservedID field to the controller info file
    *
    * @param[in]    ReservedId_LR  The reserved ID value to write to the controller info file
    *
    * @return None.
    */
void CtrlStorageSetReservedLongRangeId(node_id_t  ReservedId_LR);

/**
    * Read the reserved ID field from the controller info file
    *
    * @return the value of the field
    */

node_id_t CtrlStorageGetReservedId(void);

/**
    * write the reservedID field to the controller info file
    *
    * @param[in]    ReservedId  The reserved ID value to write to the controller info file
    *
    * @return None.
    */
void CtrlStorageSetReservedId(node_id_t  ReservedId);


/**
    * Read the SUC pending update flag from the node data file
    * this flag is valid when the node is a controller node
    *
    * @param[in]     nodeID  The node ID for the node we want to read the SUC pending update flag
    *
    * @return the value of the flag
    */
bool CtrlStorageIsNodeInPendingUpdateTable(node_id_t bNodeID);

/**
    * Write the SUC pending update flag to the node data file
    * this flag is valid when the node is a controller node
    *
    * @param[in]     nodeID  The node ID for the node we want to write the SUC pending update flag to it
    * @param[in]     isPending  The value of the flag
    *
    * @return the value of the flag
    */
void CtrlStorageChangeNodeInPendingUpdateTable(node_id_t bNodeID, bool isPending);


/**
    * Read a SUC update entery from the SUC updates file
    *
    * @param[in]     bSucUpdateIndex  The index of the SUC update reored entry we want to read from the SUC updates file
    * @param[out]     SucUpdateEntry  The buffer to store the read SUC update entry recored
    *
    * @return onde.
    */
void CtrlStorageGetSucUpdateEntry(uint8_t bSucUpdateIndex, SUC_UPDATE_ENTRY_STRUCT *SucUpdateEntry);

/**
    * Update supported commands classes field in the corresponding SUC update entery from the SUC updates file
    *
    * @param[in]     bSucUpdateIndex  The index of the SUC update entry in the SUC updates file to write the command classes to it.
    * @param[in]     cmdClasses  The buffer to read the SUC update entry from it.
    *
    * @return nonde.
    */
void CtrlStorageSetCmdClassInSucUpdateEntry(uint8_t bSucUpdateIndex, uint8_t * cmdClasses);

/**
    * Update SUC update entery from the SUC updates file
    *
    * @param[in]     bSucUpdateIndex  The index of the SUC update entry in the SUC updates file to update.
    * @param[in]     bChangeType  The value of the changeType field in the SUC update entry to be updated.
    * @param[in]     bNodeID  The value of the nodeID filed in the SUC update entry to be udpated.
    *
    */
void CtrlStorageSetSucUpdateEntry(uint8_t bSucUpdateIndex, uint8_t bChangeType, node_id_t bNodeID);

/**
    * Read the pending discovery flag from the node data file
    *
    * @param[in]     nodeID  The node ID for the node we want to read the pending discovery flag
    *
    * @return the value of the flag
    */
bool CtrlStorageGetPendingDiscoveryStatus(node_id_t bNodeID);

/**
    * Write the pending discovery flag to the node data file
    *
    * @param[in]     nodeID  The node ID for the node to write the pending discovery flag
    * @param[in]     isPendingDiscovery  The value of the pending discovery flag to write to the node data file
    *
    * @return the value of the flag
    */
void CtrlStorageSetPendingDiscoveryStatus(node_id_t bNodeID, bool isPendingDiscovery);


#ifndef NO_PREFERRED_CALC
/**
    * Read the preferred repeaters bitmask file
    *
    * @param[out]     pPreferredRepeaters  buffer contains the preferred repeaters bitmask read from the file
    *
    * @return the value of the flag
    */
void CtrlStorageGetPreferredRepeaters(NODE_MASK_TYPE* pPreferredRepeaters);

/**
    * Write the preferred repeaters bitmask file
    *
    * @param[in]     pPreferredRepeaters  buffer contains the preferred repeaters bitmask to write the file
    *
    */
void CtrlStorageSetPreferredRepeaters(NODE_MASK_TYPE* pPreferredRepeaters);

#endif

#ifdef ZW_CONTROLLER_BRIDGE
/**
    * Read the virtual node flags for all nodes in the network
    * Will read the flag from all available node data files
    * @param[out]     bridgeNodePool Buffer to store the  virtual node flags read from the nodes data files
    *                the flags are stored as a bitmask array and the array size is 29 bytes
    */
void CtrlStorageReadBridgeNodePool(uint8_t *bridgeNodePool);
/**
    * Write the virtual node flags for all nodes in the network
    * Will write the flags of all available node data files
    * @param[in]     bridgeNodePool Buffer to read the  virtual node flags we want to write to the nodes data files
    *                the flags are stored as a bitmask array and the array size is 29 bytes
    */
void CtrlStorageWriteBridgeNodePool(uint8_t *bridgeNodePool);

/**
    * Read the virtual node flag from the node data file
    *
    * @param[in]     nodeID  The node ID for the node we want to read the virual node flag
    *
    * @return the value of the flag
    */
bool CtrlStorageGetBridgeNodeFlag(node_id_t nodeID);

/**
    * Write the virtual node flag to the node data file
    *
    * @param[in]     nodeID  The node ID for the node we want to write the virual node flag to its node data file
    * @param[in]     IsBridgeNode  Set if node is bridge node
    * @param[in]     writeToNVM  Save flags to Non Volatile Memory
    *
    * @return true if flag actually updated
    */
bool CtrlStorageSetBridgeNodeFlag(node_id_t nodeID , bool IsBridgeNode, bool writeToNVM);
#endif

/**
* write the node home ID and node ID
*
* @param[in] pHomeID   A buffer holding the network home ID to write to the network info file
* @param[in] NodeID    node ID to write to the network info file
*
* @return None
*/
void ControllerStorageSetNetworkIds(const uint8_t *pHomeID, node_id_t nodeID);

/**
* read the node home ID and node ID
*
* @param[out] pHomeID   A buffer to copy the network home ID to it from the network info file
* @param[out] NodeID    A buffer to copy the network node ID to it from the network info file
*
* @return None
*/
void ControllerStorageGetNetworkIds(uint8_t *pHomeID, node_id_t *pNodeID);

/**
    * Set the controller node ID in the controller info file
    *
    *
    * @return None
    */

void CtrlStorageSetNodeID(node_id_t NodeID);

/**
    * Read the controller node ID from the controller info file
    *
    * @param[in] NodeID    node ID to write
    *
    * @return the controller node ID
    */

node_id_t CtrlStorageGetNodeID(void);

/**
    * write the state of SmartStart
    *
    * @param[in] SystemState    state of SmartStart
    *
    * @return None
    */
void CtrlStorageSetSmartStartState(uint8_t SystemState);

/**
    * read the state of SmartStart
    *
    * @return state of SmartStart
    */
uint8_t CtrlStorageGetSmartStartState(void);

/**
    * write Primary Long Range ChannelId
    *
    * @param[in] channelId    Primary Long Range ChannelId
    *
    * @return None
    */
void StorageSetPrimaryLongRangeChannelId(zpal_radio_lr_channel_t channelId);

/**
    * write Long Range Channel selection mode
    *
    * @param[in] enable   Long Range Channel selection mode
    *
    * @return None
    */
void StorageSetLongRangeChannelAutoMode(bool enable );

/**
    * read Primary Long Range ChannelId
    *
    * @return Primary Long Range ChannelId
    */
zpal_radio_lr_channel_t StorageGetPrimaryLongRangeChannelId(void);

/**
    * read Long Range Channel selection mode
    *
    * @return Long Range Channel selection mode
    */
bool StorageGetLongRangeChannelAutoMode(void);

/**
    * Initialize the controller storage
    *
    * Formats File system if required files are not present (unless
    * files are missing because FS was just formatted).
    * Writes default files with default content if FS was formatted.
    *
    * @return nothing
    */
void CtrlStorageInit(void);

/**
    * read the version of the protocol from NVM
    *
    * @param[out] version
    *
    * @return true if file found. false if file not found.
    */
bool CtrlStorageGetZWVersion(uint32_t * version);

#if defined(ZW_SECURITY_PROTOCOL)
/**
    * Read S2 keys from NVM. Used by ZW_keystore.c
    *
    * @param[out] keys
    *
    * @return true if file is found, false if files is not found.
    */
bool StorageGetS2Keys(Ss2_keys * keys);

/**
    * Write S2 keys to NVM. Used by ZW_keystore.c
    *
    * @param[in] keys
    *
    * @return true if file is successfully written, false if not
    */
bool StorageSetS2Keys(Ss2_keys * keys);

/**
    * Read S2 key classes assigned byte from NVM. Used by ZW_keystore.c
    *
    * @param[out] assigned
    *
    * @return true if file is found, false if files is not found.
    */
bool StorageGetS2KeyClassesAssigned(Ss2_keyclassesAssigned * assigned);

/**
    * Write S2 key classes assigned byte to NVM. Used by ZW_keystore.c
    *
    * @param[out] assigned
    *
    * @return true if file is successfully written, false if not
    */
bool StorageSetS2KeyClassesAssigned(Ss2_keyclassesAssigned * assigned);

bool StorageGetS2MpanTable(void * mpan_table);
bool StorageSetS2MpanTable(void * mpan_table);
bool StorageGetS2SpanTable(void * span_table);
bool StorageSetS2SpanTable(void * span_table);
#endif /*ZW_SECURITY_PROTOCOL*/

/**
    * Check if a node is currently running in Long Range mode
    *
    * @param[in] nodeID    NodeID of the node to check
    *
    * @return true if Long Range, false if classic
    */
bool CtrlStorageLongRangeGet(node_id_t nodeID);

/**
    * Set that a node is currently running in Long Range mode
    *
    * @param[in] nodeID    NodeID of the node to set to Long Range
    *
    * @return nothing
    */
void CtrlStorageLongRangeSet(node_id_t nodeID);

/**
    * Check if a node is a listening node by reading NodeInfo stored in NVM
    *
    * @param[in] nodeID    NodeID of the node to check
    *
    * @return true if listening node
    */
bool CtrlStorageListeningNodeGet(node_id_t nodeID);

/**
    * Check if a node is a sensor node by reading NodeInfo stored in NVM
    *
    * @param[in] nodeID    NodeID of the node to check
    *
    * @return true if sensor node
    */
bool CtrlStorageSensorNodeGet(node_id_t nodeID);

/**
    * Check if a node is a routing node by reading NodeInfo stored in NVM
    *
    * @param[in] nodeID    NodeID of the node to check
    *
    * @return true if routing node
    */
bool CtrlStorageRoutingNodeGet(node_id_t nodeID);

/**
    * Check if a node is a slave node by reading NodeInfo stored in NVM
    *
    * @param[in] nodeID    NodeID of the node to check
    *
    * @return true if slave node
    */
bool CtrlStorageSlaveNodeGet(node_id_t nodeID);

/**
    * Check if a node is capable of 40k baud rate by reading NodeInfo stored in NVM
    *
    * @param[in] nodeID    NodeID of the node to check
    *
    * @return true if capable of 40k baud rate
    */
bool CtrlStorageCapabilitiesSpeed40kNodeGet(node_id_t nodeID);

bool CtrlStorageCacheNodeExist(node_id_t nodeID);

bool CtrlStorageCacheCapabilitiesSpeed100kNodeGet(node_id_t nodeID);
void CtrlStorageCacheCapabilitiesSpeed100kNodeSet(node_id_t nodeID, bool value);
void GetIncludedNodes(NODE_MASK_TYPE node_id_list);
void GetIncludedLrNodes(LR_NODE_MASK_TYPE node_id_list);

#endif /* ZWAVE_ZW_CONTROLLER_NETWORK_INFO_STORAGE_H_ */
