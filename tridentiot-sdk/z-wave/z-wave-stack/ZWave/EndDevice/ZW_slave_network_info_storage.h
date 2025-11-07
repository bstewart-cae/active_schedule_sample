// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file ZW_slave_network_info_storage.h
 * Functions to store slave related information to the file system.
 *
 * The Slave information is organized into 3 files categories.
 *   1. The return route information files.
 *      There can be up to 232 files depending on the number of nodes included in the network
 *      The file name maps to the node ID (1 to 232). Except for SUC route were the file ID is hardcoded to 0
 *
 *      Each file is organized as follow:
 *
 *      typedef struct SReturnRouteInfo
 *      {
 *         NVM_RETURN_ROUTE_STRUCT  ReturnRoute;
 *         NVM_RETURN_ROUTE_SPEED   ReturnRouteSpeed;
 *      } SReturnRouteInfo;
 *
 *      The struct size is 7 bytes.
 *      The structure  NVM_RETURN_ROUTE_STRUCT contains the repeaters ID s and the destination id. The structure length is 5 bytes
 *      The structure  NVM_RETURN_ROUTE_SPEED contains the ReturnRoute's speed information and beam capability and its 2 bytes long.
 *
 *   2. Cached returns route destination
 *      This file is created at power up if it doesn't exist yet. IT cannot be deleted
 *      The file contains cached destination node ID's for some return route. The file size is 5 bytes
 *
 *   3. network ID.
 *      Each file is organized as follow
 *      typedef struct SSlaveInfo
 *      {
 *        uint8_t homeID[HOMEID_LENGTH];
 *        uint8_t nodeID;
 *        uint8_t smartStartState;
 *      }SSlaveInfo;
 *
 *      This file is created at power up if it doesn't exist yet. It cannot be deleted.
 *      It contains the node home ID, node ID and smartStartState. The file length is 6 bytes.
 *
 * @copyright 2022 Silicon Laboratories Inc.
 */

#ifndef ZWAVE_ZW_SLAVE_NETWORK_INFO_STORAGE_H_
#define ZWAVE_ZW_SLAVE_NETWORK_INFO_STORAGE_H_

#include <stdint.h>
#include "ZW_transport_api.h"
#include <ZW_keystore.h>

#define RETURN_ROUTE_MAX  (4)   /* NOTE: The same define exists in ZW_controller_network_info_storage.h */

/* Libraries with this define can potentially have routes for every node in the network. */
#define ZW_MAX_RETURN_ROUTE_DESTINATIONS   ZW_MAX_NODES

#define RETURN_ROUTE_SIZE           (sizeof(NVM_REPEATER_LIST))
#define RETURN_ROUTE_STRUCT_SIZE    (sizeof(NVM_RETURN_ROUTE_STRUCT))

#define MAX_RETURN_ROUTES_MAX_ENTRIES (ZW_MAX_RETURN_ROUTE_DESTINATIONS + 1)

//The version number of the slave protocol file system. Valid range: 0-255
//Used to trigger migration of files between file system versions.
#define ZW_SLAVE_FILESYS_VERSION  5


typedef struct _nvm_repeater_list_
{
  uint8_t repeaterList[MAX_REPEATERS]; /* List of repeaters */
} NVM_REPEATER_LIST;

typedef struct _nvm_return_route_struct_
{
  uint8_t nodeID;
  NVM_REPEATER_LIST routeList[RETURN_ROUTE_MAX];
} NVM_RETURN_ROUTE_STRUCT;


typedef struct _nvm_speed_list_
{
  uint8_t speed_route_0 : 2;
  uint8_t speed_route_1 : 2;
  uint8_t speed_route_2 : 2;
  uint8_t speed_route_3 : 2;
  uint8_t beam_dst_0 : 2;
  uint8_t priority_route : 3;
  uint8_t unused : 3;
} NVM_SPEED_LIST;

typedef union _nvm_speed_list_union_
{
  NVM_SPEED_LIST bits;
  uint8_t bytes[sizeof(NVM_SPEED_LIST)];
} NVM_SPEED_LIST_UNION;


typedef struct _nvm_return_route_speed_
{
  NVM_SPEED_LIST_UNION speed;
} NVM_RETURN_ROUTE_SPEED;

typedef enum SELECT_RETURNROUTEINFO
{
  GET_RETURNROUTE,
  GET_RETURNROUTE_SPEED,
  GET_RETURNROUTE_INFO
} SELECT_RETURNROUTEINFO;

 /**
    * Read the destination node return route file
    * If the file does not exist the route will be empty
    *
    * @param[in]   destRouteIndex   The destination node ID, valid value are 0 ..232 where 0 is reserved for SUC return route.
    *                               Values 1..232 mapped to destination node IDs
    * @param[out] pReturnRoute     A buffer to To copy the return route info to it.
    *
    * @return None
    */
void SlaveStorageGetReturnRoute(node_id_t destRouteIndex , NVM_RETURN_ROUTE_STRUCT* pReturnRoute);

/**
    * Read the destinatioin node return route speed info file
    * If the file does not exist the route will be empty
    *
    * @param[in]   destRouteIndex   The destination node ID, valid value are 0 ..232 where 0 is reserved for SUC return route speed info.
    *                               Values 1..232 mapped to destination node IDs
    * @param[out] pReturnRouteSpeed     A buffer to To copy the return route speed info to it.
    *
    * @return None
    */
void SlaveStorageGetReturnRouteSpeed(node_id_t destRouteIndex , NVM_RETURN_ROUTE_SPEED* pReturnRouteSpeed);

 /**
    * Write the destination node return route info the a file
    * If the file does not exist, it will be created
    *
    * @param[in]   destRouteIndex   The destination node ID, valid value are 0 ..232 where 0 is reserved for SUC return route.
    *                               Values 1..232 mapped to destination node IDs
    * @param[in] pReturnRouteData   A buffer to copy the return route info into it.
    * @param[in] outputSelector     type of the return route info to read from the nvm return route info file.
    *
    * @return true if return route info read correctly, else false
    */
bool SlaveStorageGetReturnRouteInfo(node_id_t destRouteIndex , uint8_t * pReturnRouteData, SELECT_RETURNROUTEINFO outputSelector);

 /**
    * Write the destination node return route info the a file
    * If the file does not exist, it will be created
    *
    * @param[in]   destRouteIndex   The destination node ID, valid value are 0 ..232 where 0 is reserved for SUC return route.
    *                               Values 1..232 mapped to destination node IDs
    * @param[in] pReturnRoute      A buffer to copy from it the return route info into a file.
    * @param[in] pReturnRouteSpeed A buffer to copy from it the return route speed into a file.
    *
    * @return None
    */
void SlaveStorageSetReturnRoute(node_id_t destRouteIndex , const NVM_RETURN_ROUTE_STRUCT* pReturnRoute, const NVM_RETURN_ROUTE_SPEED* pReturnRouteSpeed);

 /**
    * Write the destination node return route speed info the a file
    * If the file does not exist, it will be created
    *
    * @param[in]   destRouteIndex   The destination node ID, valid value are 0 ..232 where 0 is reserved for SUC return route.
    *                               Values 1..232 mapped to destination node IDs
    * @param[in] pReturnRoute     A buffer to copy from it the return route speed info into a file.
    *
    * @return None
    */
void SlaveStorageSetReturnRouteSpeed(node_id_t destRouteIndex , const NVM_RETURN_ROUTE_SPEED* pReturnRouteSpeed);

/**
    * Delete the destination node return route info files mapped to node IDs 1..232.
    * Since retunr route speed info is related to return route, then it will also be deleted.
    *
    * Does not delete SUC return route.
    *
    * @return None
    */
void SlaveStorageDeleteAllReturnRouteInfo(void);

/**
    * Delete the destination node return route info file mapped to node ID 0, the SUC return route.
    * Since retunr route speed info is related to return route, then it will also be deleted.
    *
    * @return None
    */
void SlaveStorageDeleteSucReturnRouteInfo(void);

/**
    * Read the cached destination nodes return routes
    *
    * @param[out] pRouteDestinations     A buffer to To copy data to it
    *
    * @return None
    */
void SlaveStorageGetRouteDestinations(uint8_t * pRouteDestinations);

/**
    * write the cached destination nodes return routes to a file
    *
    * @param[in] pRouteDestinations     A buffer to To copy data from it into a file
    *
    * @return None
    */
void SlaveStorageSetRouteDestinations(const uint8_t * pRouteDestinations);

/**
    * write the node home ID and node ID
    *
    * @param[in] pHomeID   A buffer holding the network home ID to write to the network info file
    * @param[in] NodeID    Node ID to write to the network info file
    *
    * @return None
    */
void SlaveStorageSetNetworkIds(const uint8_t *pHomeID, node_id_t NodeID);

/**
    * read the node home ID and node ID
    *
    * @param[out] pHomeID   A buffer to copy the network home ID to it from the network info file
    * @param[out] NodeID    A buffer to copy the network node ID to it from the network info file
    *
    * @return None
    */
void SlaveStorageGetNetworkIds(uint8_t *pHomeID, node_id_t *pNodeID);


/**
    * read the full node ID
    *
    * @param[out] NodeID    A buffer to copy the network node ID to from the network info file
    *
    * @return None
    */
void StorageGetNodeId(node_id_t *pNodeID);

/**
    * write the full node ID
    *
    * @param[in] NodeID     Node ID to write to the network info file
    *
    * @return None
    */
void StorageSetNodeId(node_id_t NodeID);

/**
    * write the state of SmartStart
    *
    * @param[in] SystemState    state of SmartStart
    *
    * @return None
    */
void SlaveStorageSetSmartStartState(uint8_t SystemState);

/**
    * read the state of SmartStart
    *
    * @return state of SmartStart
    */
uint8_t SlaveStorageGetSmartStartState(void);

/**
    * read the Primary Long Range ChannelId
    *
    * @return Primary Long Range ChannelId
    */
zpal_radio_lr_channel_t StorageGetPrimaryLongRangeChannelId(void);

/**
    * write the Primary Long Range ChannelId
    *
    * @param[in] channelId    Long Range ChannelId
    *
    * @return None
    */
void StorageSetPrimaryLongRangeChannelId(zpal_radio_lr_channel_t channelId);

/**
    * Initialize the slave network storage
    *
    * Formats File system if required files are not present (unlesss
    * files are missing because FS was just formatted).
    * Writes default files with default content if FS was formatted.
    *
    * @return None
    */
void SlaveStorageInit(void);

/**
    * read the version of the protocol from NVM
    *
    * @param[out] version 0x[00, ZW_VERSION_MAJOR, ZW_VERSION_MINOR, ZW_VERSION_PATCH]
    *
    * @return true if file is found. false if files is not found.
    */
bool SlaveStorageGetZWVersion(uint32_t * version);


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

/**
 * @brief Reads the Private Key from NVM
 *
 * @param[out] key Private key
 * @return true if file is successfully read
 * @return false if file was not successfully read
 */
bool StorageGetZWPrivateKey(uint8_t * key);

/**
 * @brief Writes the Private Key to NVM
 *
 * @param[out] key
 * @return true if file is successfully written
 * @return false if file was not successfully written
 */
bool StorageSetZWPrivateKey(uint8_t * key);


/**
 * @brief Reads the Public Key from NVM
 *
 * @param[out] key Public key
 * @return true if file is successfully read
 * @return false if file was not successfully read
 */
bool StorageGetZWPublicKey(uint8_t * key);

/**
 * @brief Writes the Public Key to NVM
 *
 * @param[out] key
 * @return true if file is successfully written
 * @return false if file was not successfully written
 */
bool StorageSetZWPublicKey(uint8_t * key);

/* Delete all slave network info storage files by ID */
void DeleteSlaveNetworkInfoStorage(void);

#endif /* ZWAVE_ZW_SLAVE_NETWORK_INFO_STORAGE_H_ */
