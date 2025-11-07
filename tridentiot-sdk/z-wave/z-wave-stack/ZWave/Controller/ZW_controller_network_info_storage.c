// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file ZW_controller_network_info_storage.c
 * @copyright 2022 Silicon Laboratories Inc.
 *
 * @brief This module provide an interface to handle writing , reading, deleting, creating, and testing
 * Controller related network data to non-volatile media.
 */
/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include "ZW_lib_defines.h"
#include <string.h>
#include "SizeOf.h"
#include "ZW_controller_network_info_storage.h"
#include "ZW_dynamic_tx_power.h"
#include "ZW_node.h"
#include "ZW_protocol.h"
#include "ZW_basis_api.h"
#include "ZW_controller_api.h"
//#define DEBUGPRINT
#include <DebugPrint.h>
#include <Assert.h>
#include <SyncEvent.h>
#include <ZW_NVMCaretaker.h>
#include <ZW_nvm.h>
#include <ZW_Security_Scheme2.h>
#include "ZW_transport.h"
#include <zpal_radio.h>

typedef struct SNodeInfoStorage
{
  EX_NVM_NODEINFO NodeInfo;
  NODE_MASK_TYPE neighboursInfo;
  uint8_t ControllerSucUpdateIndex;
} SNodeInfoStorage;

typedef struct SNodeRouteCache
{
  ROUTECACHE_LINE  routeCache;
  ROUTECACHE_LINE  routeCacheNlwrSr;
} SNodeRouteCache;

typedef struct SNodeInfoLongRange
{
  uint8_t  packedInfo;
  uint8_t  generic;
  uint8_t  specific;
}SNodeInfoLongRange;


#define DefaultControllerInfoSucLastIndex             (0xFF)
#define DefaultControllerInfoControllerConfiguration  (NO_NODES_INCLUDED | CONTROLLER_IS_REAL_PRIMARY)

#define NODEINFOS_PER_FILE        4

#define NODEINFO_LR_PER_FILE         50
#define NUMBER_OF_NODEINFO_LR_FILES  20

#define NODEROUTECACHES_PER_FILE        8
#define NUMBER_OF_NODEROUTECACHE_FILES  29  // supporting 232 nodes
#define NODEROUTECACHE_FILES_IN_RAM     4

#define SUCNODES_PER_FILE         8


#define NODEINFO_FLAG_ROUTING     0x01
#define NODEINFO_FLAG_LISTENING   0x02
#define NODEINFO_FLAG_SPECIFIC    0x04
#define NODEINFO_FLAG_BEAM        0x08
#define NODEINFO_FLAG_OPTIONAL    0x10
#define NODEINFO_MASK_SENSOR      0x60


//Do not change the value of FILE_ID_ZW_VERSION
#define FILE_ID_ZW_VERSION              (0x00000)
#define FILE_SIZE_ZW_VERSION            (sizeof(uint32_t))

//The file is only in use when NO_PREFERRED_CALC is NOT defined.
//The file ID macro is kept for file migration.
#define FILE_ID_PREFERREDREPEATERS      (0x00002)
#ifndef NO_PREFERRED_CALC
#define FILE_SIZE_PREFERREDREPEATERS    (sizeof(SPreferredRepeaters))
#endif  /*NO_PREFERRED_CALC*/

//Legacy file ID for SUCNODELIST. Used for file migration.
#define FILE_ID_SUCNODELIST_LEGACY_v4     (0x00003)
#define FILE_SIZE_SUCNODELIST_LEGACY_v4   (1408)

#define FILE_ID_CONTROLLERINFO            (0x00004)
#define FILE_SIZE_CONTROLLERINFO          (sizeof(SControllerInfo))

#define FILE_ID_NODE_STORAGE_EXIST        (0x00005)
#define FILE_SIZE_NODE_STORAGE_EXIST      (sizeof(NODE_MASK_TYPE))

#define FILE_ID_APP_ROUTE_LOCK_FLAG       (0x00006)
#define FILE_SIZE_APP_ROUTE_LOCK_FLAG     (sizeof(NODE_MASK_TYPE))

#define FILE_ID_ROUTE_SLAVE_SUC_FLAG      (0x00007)
#define FILE_SIZE_ROUTE_SLAVE_SUC_FLAG    (sizeof(NODE_MASK_TYPE))

#define FILE_ID_SUC_PENDING_UPDATE_FLAG   (0x00008)
#define FILE_SIZE_SUC_PENDING_UPDATE_FLAG (sizeof(NODE_MASK_TYPE))

#ifdef ZW_CONTROLLER_BRIDGE
#define FILE_ID_BRIDGE_NODE_FLAG          (0x00009)
#define FILE_SIZE_BRIDGE_NODE_FLAG        (sizeof(NODE_MASK_TYPE))
#endif

#define FILE_ID_PENDING_DISCOVERY_FLAG    (0x0000A)
#define FILE_SIZE_PENDING_DISCOVERY_FLAG  (sizeof(NODE_MASK_TYPE))

#define FILE_ID_NODE_ROUTECACHE_EXIST     (0x0000B)
#define FILE_SIZE_NODE_ROUTECACHE_EXIST   (sizeof(NODE_MASK_TYPE))

#define FILE_ID_LRANGE_NODE_EXIST         (0x0000C)
#define FILE_SIZE_LRANGE_NODE_EXIST       (sizeof(LR_NODE_MASK_TYPE))

// S2 keys files and SPANS (Deprecated)
#define FILE_ID_S2_KEYS                   (0x00010)  ///< Deprecated, was used for storing S2 keys
#define FILE_ID_S2_KEYCLASSES_ASSIGNED    (0x00011)  ///< Deprecated, was used for storing supported keyclasses
#define FILE_ID_S2_MPAN                   (0x00012)  ///< Deprecated, was used for storing SPAN
#define FILE_ID_S2_SPAN                   (0x00013)  ///< Deprecated, was used for storing MPAN
#define FILE_ID_S2_SPAN_BASE              (0x02000)  ///< Deprecated, was used for storing SPAN table
#define FILE_ID_S2_MPAN_BASE              (0x03000)  ///< Deprecated, was used for storing MPAN table
// end of deprecated file IDs

#define FILE_ID_NODEINFO_BASE             (0x00200)
#define FILE_SIZE_NODEINFO                (sizeof(SNodeInfoStorage) * NODEINFOS_PER_FILE)
#define FILE_ID_NODEINFO_LAST             (FILE_ID_NODEINFO_LR_BASE - 1)

#define FILE_ID_NODEINFO_LR_BASE          (0x00800)
#define FILE_SIZE_NODEINFO_LR             (sizeof(SNodeInfoLongRange) * NODEINFO_LR_PER_FILE)
#define FILE_ID_NODEINFO_LR_LAST          (FILE_ID_NODEROUTE_CACHE_BASE - 1)

#define FILE_ID_NODEROUTE_CACHE_BASE      (0x01400)
#define FILE_SIZE_NODEROUTE_CACHE         (sizeof(SNodeRouteCache) * NODEROUTECACHES_PER_FILE)
#define FILE_ID_NODEROUTE_CACHE_LAST      (FILE_ID_S2_SPAN_BASE - 1)

#define FILE_ID_SUCNODELIST_BASE          (0x04000)
#define FILE_SIZE_SUCNODELIST             (sizeof(SSucNodeList))
#define FILE_ID_SUCNODELIST_LAST          (FILE_ID__NEXT - 1)

#define FILE_ID__NEXT                     (0x04080)  // Next free file ID

#define FILE_ID__MAX                      (0x09FFF)  // Max file ID

#define ZW_CONTROLLER_FILESYS_VERSION_GET(version)        (version >> 24)

#define ZW_CONTROLLER_FILESYS_VERSION_SET(presentFilesysVersion, newFileSystemVersion)    \
    presentFilesysVersion = ((presentFilesysVersion & 0x00FFFFFFU) | (newFileSystemVersion << 24))


#ifndef NO_PREFERRED_CALC
  typedef struct SPreferredRepeaters
  {
    uint8_t                          PreferredRepeater[MAX_NODEMASK_LENGTH];
  } SPreferredRepeaters;
#endif /*NO_PREFERRED_CALC*/

typedef struct SSucNodeList
{
  SUC_UPDATE_ENTRY_STRUCT       aSucNodeList[SUCNODES_PER_FILE];
} SSucNodeList;


typedef struct SControllerInfo
{
  uint8_t                       HomeID[HOMEID_LENGTH];
  uint16_t                      NodeID;
  uint16_t                      StaticControllerNodeId;
  uint16_t                      LastUsedNodeId_LR;
  uint8_t                       LastUsedNodeId;
  uint8_t                       SucLastIndex; // Updated when SucNodeList is updated
  uint16_t                      MaxNodeId_LR;
  uint8_t                       MaxNodeId;
  uint8_t                       ControllerConfiguration;
  uint16_t                      ReservedId_LR;
  uint8_t                       ReservedId;
  uint8_t                       SystemState;
  uint8_t                       PrimaryLongRangeChannelId;
  uint8_t                       LonRangeChannelAutoMode;

}SControllerInfo;

//Original version of SControllerInfo.
//Used only for migration of file system.
typedef struct SControllerInfo_OLD
{
  uint8_t                       HomeID[HOMEID_LENGTH];
  uint8_t                       NodeID;
  uint8_t                       LastUsedNodeId;
  uint8_t                       StaticControllerNodeId;
  uint8_t                       SucLastIndex; // Updated when SucNodeList is updated
  uint8_t                       ControllerConfiguration;
  uint8_t                       SucAwarenessPushNeeded;
  uint8_t                       MaxNodeId;
  uint8_t                       ReservedId;
  uint8_t                       SystemState;

}SControllerInfo_OLD;

static bool SetFile(zpal_nvm_object_key_t fileID, uint32_t copyLength);
static void WriteDefaultSetofFiles(void);

#define APP_ROUTE_LOCK_FLAG       0x01
#define ROUTE_SLAVE_SUC_FLAG      0x02
#define SUC_PENDING_UPDATE_FLAG   0x04
#define BRIDGE_NODE_FLAG          0x08
#define PENDING_DISCOVERY_FLAG    0x10

static NODE_MASK_TYPE app_route_lock_flag;
static NODE_MASK_TYPE route_slave_suc_flag;
static NODE_MASK_TYPE suc_pending_update_flag;
#ifdef ZW_CONTROLLER_BRIDGE
static NODE_MASK_TYPE bridge_node_flag;
#endif
static NODE_MASK_TYPE pending_discovery_flag;

static NODE_MASK_TYPE node_info_exists;
static NODE_MASK_TYPE node_routecache_exists;
static NODE_MASK_TYPE capabilities_speed_100k_nodes;

static LR_NODE_MASK_TYPE node_info_Lrange_exists;

//Big RAM buffer for NodeRouteCaches. Currently 320 bytes long.
//It contains several RouteCache files that each comprise several RouteCache entries.
static uint8_t nodeRouteCacheBuffer[NODEROUTECACHE_FILES_IN_RAM * FILE_SIZE_NODEROUTE_CACHE];
//Store the file number of the nodeRouteCache files in the buffer above.
static uint8_t nodeRouteCacheFileID[NODEROUTECACHE_FILES_IN_RAM];
//Ordered list of highest to lowest priority position in the file buffer
static uint8_t nodeRouteCachePrio[NODEROUTECACHE_FILES_IN_RAM];

static zpal_nvm_handle_t pFileSystem;
static const SSyncEvent FileSystemFormattedCb = {
                                                .uFunctor.pFunction = WriteDefaultSetofFiles,
                                                .pObject = 0
                                               };

// FileSet - allows FileSystemCaretaker to validate and init required files
static const SObjectDescriptor m_aFileDescriptors[] = {
#ifndef NO_PREFERRED_CALC
  { .ObjectKey = FILE_ID_PREFERREDREPEATERS,            .iDataSize = FILE_SIZE_PREFERREDREPEATERS },
#endif  /*NO_PREFERRED_CALC*/
  { .ObjectKey = FILE_ID_CONTROLLERINFO,                .iDataSize = FILE_SIZE_CONTROLLERINFO          },
  { .ObjectKey = FILE_ID_NODE_STORAGE_EXIST,            .iDataSize = FILE_SIZE_NODE_STORAGE_EXIST      },
  { .ObjectKey = FILE_ID_NODE_ROUTECACHE_EXIST,         .iDataSize = FILE_SIZE_NODE_ROUTECACHE_EXIST   },
  { .ObjectKey = FILE_ID_LRANGE_NODE_EXIST,             .iDataSize = FILE_SIZE_LRANGE_NODE_EXIST       },
  { .ObjectKey = FILE_ID_APP_ROUTE_LOCK_FLAG,           .iDataSize = FILE_SIZE_APP_ROUTE_LOCK_FLAG     },
  { .ObjectKey = FILE_ID_ROUTE_SLAVE_SUC_FLAG,          .iDataSize = FILE_SIZE_ROUTE_SLAVE_SUC_FLAG    },
  { .ObjectKey = FILE_ID_SUC_PENDING_UPDATE_FLAG,       .iDataSize = FILE_SIZE_SUC_PENDING_UPDATE_FLAG },
#ifdef ZW_CONTROLLER_BRIDGE
  { .ObjectKey = FILE_ID_BRIDGE_NODE_FLAG,              .iDataSize = FILE_SIZE_BRIDGE_NODE_FLAG        },
#endif
  { .ObjectKey = FILE_ID_PENDING_DISCOVERY_FLAG,        .iDataSize = FILE_SIZE_PENDING_DISCOVERY_FLAG  }
};


static ECaretakerStatus m_aFileStatus[sizeof_array(m_aFileDescriptors)];

static const uint32_t ZW_Version = (ZW_CONTROLLER_FILESYS_VERSION << 24) | (ZW_VERSION_MAJOR << 16) | (ZW_VERSION_MINOR << 8) | (ZW_VERSION_PATCH);

#define SUC_UNKNOWN_CONTROLLER   0xFE

/****************************************************************************/
/*                            PRIVATE FUNCTIONS                             */
/****************************************************************************/

void  setToHighestBufferPrio(uint32_t index);
void  removeFromBufferPrio(uint32_t index);
uint8_t  getLowestBufferPrio(void);
static bool NodeInfoLongRangeExists(node_id_t nodeID);

static
bool NodeIdIsClassic(node_id_t nodeID)
{
  if(nodeID && (ZW_MAX_NODES >= nodeID))
  {
    return true;
  }
  return false;
}

static
bool NodeInfoExists(node_id_t nodeID)
{
  if(NodeIdIsClassic(nodeID))
  {
    return ZW_NodeMaskNodeIn(node_info_exists, nodeID);
  }
  return false; //Not a Z-Wave Classic node
}

static
bool NodeInfoLongRangeExists(node_id_t nodeID)
{
  if(ZW_nodeIsLRNodeID(nodeID))
  {
    uint16_t index = nodeID - LOWEST_LONG_RANGE_NODE_ID;
    return ZW_LR_NodeMaskNodeIn(node_info_Lrange_exists, index + 1);
  }
  return false; //Not a Z-Wave Long Range node
}

static
void SetLongRangeExists(node_id_t nodeID)
{
  if(ZW_nodeIsLRNodeID(nodeID))
  {
    uint16_t index = nodeID - LOWEST_LONG_RANGE_NODE_ID;
    ZW_LR_NodeMaskSetBit(node_info_Lrange_exists, index + 1);
    zpal_nvm_write(pFileSystem, FILE_ID_LRANGE_NODE_EXIST, node_info_Lrange_exists, sizeof(node_info_Lrange_exists));
  }
}

static
void ClearLongRangeExists(node_id_t nodeID)
{
  if(ZW_nodeIsLRNodeID(nodeID))
  {
    uint16_t index = nodeID - LOWEST_LONG_RANGE_NODE_ID;
    ZW_LR_NodeMaskClearBit(node_info_Lrange_exists, index + 1);
    zpal_nvm_write(pFileSystem, FILE_ID_LRANGE_NODE_EXIST, node_info_Lrange_exists, sizeof(node_info_Lrange_exists));
  }
}

static bool NodeRouteCacheExist(uint8_t nodeID)
{
  return ZW_NodeMaskNodeIn(node_routecache_exists, nodeID);
}

static uint8_t NodeRouteCacheNext(uint8_t nodeID)
{
  uint8_t nextCache = ZW_NodeMaskGetNextNode(nodeID, node_routecache_exists);
  if (0 == nextCache)
  {
    return 0xFF;
  }
  else
  {
    return nextCache;
  }
}

//Store all entries of the RouteCache RAM buffer to NVM.
//Should be called at controlled reset of the system.
void StoreNodeRouteCacheBuffer(void)
{
  for(uint32_t i = 0; i < NODEROUTECACHE_FILES_IN_RAM; i++)
  {
    //Check if there is a file in the buffer
    if(0xFF != nodeRouteCacheFileID[i])
    {
      uint8_t * pFilePosInBuffer = &nodeRouteCacheBuffer[i * FILE_SIZE_NODEROUTE_CACHE];
      zpal_nvm_object_key_t fileID = FILE_ID_NODEROUTE_CACHE_BASE + nodeRouteCacheFileID[i];
      if (fileID <= FILE_ID_NODEROUTE_CACHE_LAST)
      {
        zpal_nvm_write(pFileSystem, fileID, pFilePosInBuffer, FILE_SIZE_NODEROUTE_CACHE);
      }
    }
  }
}

void StoreNodeRouteCacheFile(node_id_t nodeID)
{
  //nodeID 1-232
  //nodeNr 0-231  for local indexing in arrays
  uint16_t nodeNr = nodeID - 1;

  //Z-Wave Long Range nodes must be rejected
  if(!NodeIdIsClassic(nodeID))
  {
    //Reject Long Range node IDs
    return;
  }

  if (NodeRouteCacheExist(nodeID))
  {
    //Find RouteCache in RAM buffer if it is there
    for(uint32_t i=0; i<NODEROUTECACHE_FILES_IN_RAM; i++)
    {
      if( nodeRouteCacheFileID[i] == (nodeNr / NODEROUTECACHES_PER_FILE) )
      {
        uint8_t * pFilePosInBuffer = &nodeRouteCacheBuffer[i * FILE_SIZE_NODEROUTE_CACHE];
        zpal_nvm_write(pFileSystem, FILE_ID_NODEROUTE_CACHE_BASE + nodeRouteCacheFileID[i], pFilePosInBuffer, FILE_SIZE_NODEROUTE_CACHE);
      }
    }
  }
}

static void RemoveNodeRouteCacheFile(node_id_t nodeID)
{
  //nodeID 1-232
  //nodeNr 0-231  for local indexing in arrays
  uint16_t nodeNr = nodeID - 1;

  if(!NodeIdIsClassic(nodeID))
  {
    //Reject Long Range node IDs
    return;
  }

  if (NodeRouteCacheExist(nodeID))
  {
    ZW_NodeMaskClearBit(node_routecache_exists, nodeID);
    zpal_nvm_write(pFileSystem, FILE_ID_NODE_ROUTECACHE_EXIST, node_routecache_exists, sizeof(node_routecache_exists));

    uint8_t tNodeId = nodeNr - (nodeNr % NODEROUTECACHES_PER_FILE) + 1;
    for(uint32_t i=0; i<NODEROUTECACHES_PER_FILE; i++)
    {
      if(NodeRouteCacheExist(tNodeId + i))
      {
        //don't delete file since it is in use for other nodeNrs
        return;
      }
    }

    //Remove file from RAM buffer if it is there
    for(uint32_t j=0; j<NODEROUTECACHE_FILES_IN_RAM; j++)
    {
      if(nodeRouteCacheFileID[j] == (nodeNr / NODEROUTECACHES_PER_FILE))
      {
        memset(&nodeRouteCacheBuffer[j * FILE_SIZE_NODEROUTE_CACHE], 0xFF, FILE_SIZE_NODEROUTE_CACHE);
        nodeRouteCacheFileID[j] = 0xFF;
        removeFromBufferPrio(j);
      }
    }

    //Delete file. The file may not exist so an error code can be returned by zpal_nvm_erase_object()
    zpal_nvm_object_key_t fileID = FILE_ID_NODEROUTE_CACHE_BASE + (nodeNr / NODEROUTECACHES_PER_FILE);
    if (fileID <= FILE_ID_NODEROUTE_CACHE_LAST)
    {
      zpal_nvm_erase_object(pFileSystem, fileID);
    }
  }
}

void CtrlNodeInfoStoragetWrite(uint8_t nodeID, uint32_t objectOffset, uint32_t objectSize, const uint8_t * objectSrc, bool writeToNVM)
{
  //nodeID 1-232
  //nodeNr 0-231  for local indexing in arrays
  uint16_t nodeNr = nodeID - 1;

  static uint8_t lastGroupNodeID = 250; //Initiate with dummy value
  static uint8_t tFileBuffer[FILE_SIZE_NODEINFO] = { 0 };
  zpal_nvm_object_key_t fileID = FILE_ID_NODEINFO_BASE + (nodeNr / NODEINFOS_PER_FILE);
  if (fileID > FILE_ID_NODEINFO_LAST)
  {
    return;
  }

  //Read file or set tFileBuffer to default if groupNodeID corresponds to a different file than the lastGroupNodeID
  const uint8_t groupNodeID = nodeNr - (nodeNr % NODEINFOS_PER_FILE) + 1;
  if(lastGroupNodeID != groupNodeID)
  {
    bool fileExist = false;
    for(uint8_t i=0; i<NODEINFOS_PER_FILE; i++)
    {
      if (NodeInfoExists(groupNodeID + i))
      {
        fileExist = true;
        break;
      }
    }

    if(fileExist)
    {
      zpal_nvm_read(pFileSystem, fileID, tFileBuffer, FILE_SIZE_NODEINFO);
    }
    else
    {
      memset(tFileBuffer, 0xFF, FILE_SIZE_NODEINFO);
    }

    lastGroupNodeID = groupNodeID;
  }

  uint8_t * pFileOffset = &tFileBuffer[sizeof(SNodeInfoStorage) * (nodeNr % NODEINFOS_PER_FILE) + objectOffset];

  if (NodeInfoExists(nodeID))
  {
    memcpy(pFileOffset, objectSrc, objectSize);
    if(writeToNVM)
    {
      zpal_nvm_write(pFileSystem, fileID, tFileBuffer, FILE_SIZE_NODEINFO);
    }
  }
  else
  {
    SNodeInfoStorage * pNodeInfoEntry = (SNodeInfoStorage *)&tFileBuffer[sizeof(SNodeInfoStorage) * (nodeNr % NODEINFOS_PER_FILE)];

    memset(pNodeInfoEntry, 0, sizeof(SNodeInfoStorage));
    pNodeInfoEntry->ControllerSucUpdateIndex = SUC_UNKNOWN_CONTROLLER;
    memcpy(pFileOffset, objectSrc, objectSize);

    if(writeToNVM)
    {
      zpal_nvm_write(pFileSystem, fileID, tFileBuffer, FILE_SIZE_NODEINFO);
    }

    ZW_NodeMaskSetBit(node_info_exists, nodeID);
    if(writeToNVM)
    {
      zpal_nvm_write(pFileSystem, FILE_ID_NODE_STORAGE_EXIST , node_info_exists, sizeof(node_info_exists));
    }
  }
}

bool
CtrlStorageCacheNodeExist(node_id_t nodeID)
{
  if ((nodeID > 0) && (nodeID <= ZW_MAX_NODES))
  {
    return NodeInfoExists(nodeID);
  }
  else if(ZW_nodeIsLRNodeID(nodeID))
  {
    return NodeInfoLongRangeExists(nodeID);
  }
  else
  {
    return false;
  }
}


bool
CtrlStorageListeningNodeGet(node_id_t nodeID)
{
  EX_NVM_NODEINFO sNodeInfo = { 0 };
  CtrlStorageGetNodeInfo(nodeID , &sNodeInfo);
  return (sNodeInfo.capability & ZWAVE_NODEINFO_LISTENING_SUPPORT);
}


bool
CtrlStorageRoutingNodeGet(node_id_t nodeID)
{
  EX_NVM_NODEINFO sNodeInfo = { 0 };
  CtrlStorageGetNodeInfo(nodeID , &sNodeInfo);
  return (sNodeInfo.capability & ZWAVE_NODEINFO_ROUTING_SUPPORT);
}


bool
CtrlStorageSlaveNodeGet(node_id_t nodeID)
{
  if (NodeIdIsClassic(nodeID))
  {
    EX_NVM_NODEINFO sNodeInfo = { 0 };
    CtrlStorageGetNodeInfo(nodeID , &sNodeInfo);
    return !(sNodeInfo.security & ZWAVE_NODEINFO_CONTROLLER_NODE);
  }
  else
  {
    return true; //Long Range nodes are always slave nodes
  }
}

bool
CtrlStorageSensorNodeGet(node_id_t nodeID)
{
  EX_NVM_NODEINFO sNodeInfo = { 0 };
  CtrlStorageGetNodeInfo(nodeID , &sNodeInfo);
  return (sNodeInfo.security & ZWAVE_NODEINFO_SENSOR_MODE_MASK);
}


bool
CtrlStorageCapabilitiesSpeed40kNodeGet(node_id_t nodeID)
{
  if (NodeIdIsClassic(nodeID))
  {
    EX_NVM_NODEINFO sNodeInfo = { 0 };
    CtrlStorageGetNodeInfo(nodeID , &sNodeInfo);
    return (sNodeInfo.capability & ZWAVE_NODEINFO_BAUD_40000);
  }
  else
  {
    return false;
  }
}


bool
CtrlStorageCacheCapabilitiesSpeed100kNodeGet(node_id_t nodeID)
{
  if (NodeIdIsClassic(nodeID))
  {
    return ZW_NodeMaskNodeIn(capabilities_speed_100k_nodes, nodeID);
  }
  else
  {
    return false;  // or should it return true?
  }
}


void
CtrlStorageCacheCapabilitiesSpeed100kNodeSet(node_id_t nodeID, bool value)
{
  if (!NodeIdIsClassic(nodeID))
  {
    return;
  }

  if (0 != value)
  {
    ZW_NodeMaskSetBit(capabilities_speed_100k_nodes, nodeID);
  }
  else
  {
    ZW_NodeMaskClearBit(capabilities_speed_100k_nodes, nodeID);
  }
}


void CtrlStorageGetNodeInfo(node_id_t nodeID , EX_NVM_NODEINFO* pNodeInfo)
{
  zpal_nvm_object_key_t tFileID;

  if (NodeIdIsClassic(nodeID))
  {
    if (NodeInfoExists(nodeID))
    {
      //nodeID 1-232
      //nodeNr 0-231  for local indexing in arrays
      uint16_t nodeNr = nodeID - 1;
      tFileID = FILE_ID_NODEINFO_BASE + (nodeNr / NODEINFOS_PER_FILE);

      size_t filePosition = sizeof(SNodeInfoStorage) * (nodeNr % NODEINFOS_PER_FILE);

      zpal_nvm_read_object_part(pFileSystem, tFileID, (uint8_t *)pNodeInfo, filePosition, sizeof(EX_NVM_NODEINFO));

      return;
    }
  }
  else if (ZW_nodeIsLRNodeID(nodeID))
  {
    if (NodeInfoLongRangeExists(nodeID))
    {
      uint16_t nodeNr = nodeID - LOWEST_LONG_RANGE_NODE_ID;
      tFileID = FILE_ID_NODEINFO_LR_BASE + (nodeNr / NODEINFO_LR_PER_FILE);
      size_t filePosition = sizeof(SNodeInfoLongRange) * (nodeNr % NODEINFO_LR_PER_FILE);

      SNodeInfoLongRange   NodeInfoLongRange = { 0 };
      zpal_nvm_read_object_part(pFileSystem, tFileID, (uint8_t *)&NodeInfoLongRange, filePosition, sizeof(NodeInfoLongRange));

      EX_NVM_NODEINFO tNodeInfo = {0,0,0,0,0};

      //Hard code version for now
      tNodeInfo.capability |= ZWAVE_NODEINFO_VERSION_4;

      if(NODEINFO_FLAG_ROUTING & NodeInfoLongRange.packedInfo)
      {
        tNodeInfo.capability |= ZWAVE_NODEINFO_ROUTING_SUPPORT;
      }

      if(NODEINFO_FLAG_LISTENING & NodeInfoLongRange.packedInfo)
      {
        tNodeInfo.capability |= ZWAVE_NODEINFO_LISTENING_SUPPORT;
      }

      //Security support is mandatory for Long Range
      tNodeInfo.security |= ZWAVE_NODEINFO_SECURITY_SUPPORT;

      if(NODEINFO_FLAG_SPECIFIC & NodeInfoLongRange.packedInfo)
      {
        tNodeInfo.security |= ZWAVE_NODEINFO_SPECIFIC_DEVICE_TYPE;
      }

      if(NODEINFO_FLAG_BEAM & NodeInfoLongRange.packedInfo)
      {
        tNodeInfo.security |= ZWAVE_NODEINFO_BEAM_CAPABILITY;
      }

      if(NODEINFO_FLAG_OPTIONAL & NodeInfoLongRange.packedInfo)
      {
        tNodeInfo.security |= ZWAVE_NODEINFO_OPTIONAL_FUNC;
      }

      if(NODEINFO_MASK_SENSOR & NodeInfoLongRange.packedInfo)
      {
        tNodeInfo.security |= (NodeInfoLongRange.packedInfo & NODEINFO_MASK_SENSOR);
      }

      tNodeInfo.reserved |= ZWAVE_NODEINFO_BAUD_100KLR;
      tNodeInfo.generic  = NodeInfoLongRange.generic;
      tNodeInfo.specific = NodeInfoLongRange.specific;

      memcpy((uint8_t*)pNodeInfo, (uint8_t*)&tNodeInfo, sizeof(EX_NVM_NODEINFO));

      return;
    }
  }

  memset(
          (uint8_t*)pNodeInfo,                  /* Read in as EX_NVM_NODEINFO */
          0,
          EX_NVM_NODE_INFO_SIZE
        );
}

void CtrlStorageSetNodeInfo(node_id_t nodeID , EX_NVM_NODEINFO* pNodeInfo)
{
  if (NodeIdIsClassic(nodeID))
  {
    CtrlNodeInfoStoragetWrite(nodeID, offsetof(SNodeInfoStorage, NodeInfo), sizeof(EX_NVM_NODEINFO), (uint8_t *)pNodeInfo, true);

    //clear node specific flags
    CtrlStorageSetAppRouteLockFlag(nodeID, false);
    CtrlStorageSetRoutingSlaveSucUpdateFlag(nodeID, true, true);  //First true means that the node don't need static route update.
                                                                 //Second true meansthat it is stored in NVM.
    CtrlStorageChangeNodeInPendingUpdateTable(nodeID, false);
#ifdef ZW_CONTROLLER_BRIDGE
    CtrlStorageSetBridgeNodeFlag(nodeID, false, true);
#endif
    CtrlStorageSetPendingDiscoveryStatus(nodeID, false);
  }
  else if (ZW_nodeIsLRNodeID(nodeID))
  {
    uint16_t nodeNr = nodeID - LOWEST_LONG_RANGE_NODE_ID;
    uint8_t tFileBuffer[FILE_SIZE_NODEINFO_LR];
    zpal_nvm_object_key_t fileID = FILE_ID_NODEINFO_LR_BASE + (nodeNr / NODEINFO_LR_PER_FILE);
    uint8_t * pFilePos = tFileBuffer + (nodeNr % NODEINFO_LR_PER_FILE) * sizeof(SNodeInfoLongRange);
    size_t len;

    if(ZPAL_STATUS_OK == zpal_nvm_get_object_size(pFileSystem, fileID, &len))
    {
      zpal_nvm_read(pFileSystem, fileID, tFileBuffer, FILE_SIZE_NODEINFO_LR);
    }
    else
    {
      memset(tFileBuffer, 0xFF, FILE_SIZE_NODEINFO_LR);
    }

    SNodeInfoLongRange tNodeInfoLR = {
      .generic  = pNodeInfo->generic,
      .specific = pNodeInfo->specific
    };

    if(ZWAVE_NODEINFO_ROUTING_SUPPORT & pNodeInfo->capability)
    {
      tNodeInfoLR.packedInfo |= NODEINFO_FLAG_ROUTING;
    }

    if(ZWAVE_NODEINFO_LISTENING_SUPPORT & pNodeInfo->capability)
    {
      tNodeInfoLR.packedInfo |= NODEINFO_FLAG_LISTENING;
    }

    if(ZWAVE_NODEINFO_SPECIFIC_DEVICE_TYPE & pNodeInfo->security)
    {
      tNodeInfoLR.packedInfo |= NODEINFO_FLAG_SPECIFIC;
    }

    if(ZWAVE_NODEINFO_BEAM_CAPABILITY & pNodeInfo->security)
    {
      tNodeInfoLR.packedInfo |= NODEINFO_FLAG_BEAM;
    }

    if(ZWAVE_NODEINFO_OPTIONAL_FUNC & pNodeInfo->security)
    {
      tNodeInfoLR.packedInfo |= NODEINFO_FLAG_OPTIONAL;
    }

    tNodeInfoLR.packedInfo |= (pNodeInfo->security & NODEINFO_MASK_SENSOR);

    memcpy(pFilePos, (uint8_t *)&tNodeInfoLR, sizeof(SNodeInfoLongRange));
    zpal_nvm_write(pFileSystem, fileID, tFileBuffer, FILE_SIZE_NODEINFO_LR);

    SetLongRangeExists(nodeID);
  }
}

void CtrlStorageRemoveNodeInfo(node_id_t nodeID, bool keepCacheRoute)
{
  if (NodeIdIsClassic(nodeID) && NodeInfoExists(nodeID))
  {
    //clear node specific flags
    CtrlStorageSetAppRouteLockFlag(nodeID, false);
    CtrlStorageSetRoutingSlaveSucUpdateFlag(nodeID, true, true); //First true means that the node don't need static route update.
                                                                  //Second true means that it is stored in NVM.
    CtrlStorageChangeNodeInPendingUpdateTable(nodeID, false);
#ifdef ZW_CONTROLLER_BRIDGE
    CtrlStorageSetBridgeNodeFlag(nodeID, false, true);
#endif
    CtrlStorageSetPendingDiscoveryStatus(nodeID, false);

    //Clear nodeID from the node_info_exist[] array in RAM and clear it in NVM
    ZW_NodeMaskClearBit(node_info_exists, nodeID);
    zpal_nvm_write(pFileSystem, FILE_ID_NODE_STORAGE_EXIST, node_info_exists, sizeof(node_info_exists));

    if (!keepCacheRoute)
    {
      //Remove NodeRouteCache
      RemoveNodeRouteCacheFile(nodeID);
    }
    CtrlStorageCacheCapabilitiesSpeed100kNodeSet(nodeID, false);

    //nodeID 1-232
    //nodeNr 0-231  for local indexing in arrays
    uint16_t nodeNr = nodeID - 1;

    //Remove NodeInfo file corresponding to nodeID if it is empty.
    uint16_t tNodeID = nodeNr - (nodeNr % NODEINFOS_PER_FILE) + 1;
    for (uint32_t i = 0; i < NODEINFOS_PER_FILE; i++)
    {
      if (NodeInfoExists(tNodeID + i))
      {
        //don't delete file since it is in use for other nodeIDs
        return;
      }
    }

    zpal_nvm_erase_object(pFileSystem, FILE_ID_NODEINFO_BASE + (nodeNr / NODEINFOS_PER_FILE));
  }
  else if (ZW_nodeIsLRNodeID(nodeID) && NodeInfoLongRangeExists(nodeID))
  {
    //Clear nodeID from the node_info_Lrange_exists[] array in RAM and clear it in NVM
    ClearLongRangeExists(nodeID);

    //Remove NodeInfo file corresponding to nodeID if it is empty.
    uint16_t nodeNr = nodeID - LOWEST_LONG_RANGE_NODE_ID;
    uint16_t tNodeID = nodeID - (nodeNr % NODEINFO_LR_PER_FILE);
    for (uint32_t i = 0; i < NODEINFO_LR_PER_FILE; i++)
    {
      if (NodeInfoLongRangeExists(tNodeID + i))
      {
        //don't delete file since it is in use for other nodeIDs
        return;
      }
    }

    if (FILE_ID_NODEINFO_LR_BASE + (nodeNr / NODEINFO_LR_PER_FILE) <= FILE_ID_NODEINFO_LR_LAST)
    {
      zpal_nvm_erase_object(pFileSystem, FILE_ID_NODEINFO_LR_BASE + (nodeNr / NODEINFO_LR_PER_FILE));
    }
  }
}


uint8_t
CtrlStorageGetCtrlSucUpdateIndex(node_id_t nodeID)
{
  //nodeID 1-232
  //nodeNr 0-231  for local indexing in arrays
  uint16_t nodeNr = nodeID - 1;

  //Z-Wave Long Range nodes must be rejected
  if(!NodeIdIsClassic(nodeID))
  {
    return 0;
  }

  uint8_t CtrlSucUpdateIndex = SUC_UNKNOWN_CONTROLLER;

  if (NodeInfoExists(nodeID))
  {
    SNodeInfoStorage * pNodeInfo;
    uint8_t tFileBuffer[FILE_SIZE_NODEINFO] = { 0};

    zpal_nvm_read(pFileSystem, FILE_ID_NODEINFO_BASE + (nodeNr / NODEINFOS_PER_FILE), &tFileBuffer, FILE_SIZE_NODEINFO);
    pNodeInfo = (SNodeInfoStorage *)&tFileBuffer[sizeof(SNodeInfoStorage) * (nodeNr % NODEINFOS_PER_FILE)];
    CtrlSucUpdateIndex = pNodeInfo->ControllerSucUpdateIndex;
  }
  return CtrlSucUpdateIndex;
}

void
CtrlStorageSetCtrlSucUpdateIndex(node_id_t nodeID , uint8_t CtrlSucUpdateIndex)
{
  //Z-Wave Long Range nodes must be rejected
  if(!NodeIdIsClassic(nodeID))
  {
    return;
  }

  if (NodeInfoExists(nodeID))
  {
     CtrlNodeInfoStoragetWrite(nodeID, offsetof(SNodeInfoStorage, ControllerSucUpdateIndex), 1, (uint8_t*)&CtrlSucUpdateIndex, true);
  }
}

static bool GetNodeFlag(node_id_t nodeID, uint8_t FlagMask)
{
  //Attempts to read flags for Z-Wave Long Range nodes must be rejected
  if(!NodeIdIsClassic(nodeID))
  {
    return false;
  }

  if (NodeInfoExists(nodeID))
  {

    switch (FlagMask)
    {
      case APP_ROUTE_LOCK_FLAG:
      {
        return ZW_NodeMaskNodeIn(app_route_lock_flag, nodeID);
      }
      case ROUTE_SLAVE_SUC_FLAG:
      {
        return ZW_NodeMaskNodeIn(route_slave_suc_flag, nodeID);
      }
      case SUC_PENDING_UPDATE_FLAG:
      {
        return ZW_NodeMaskNodeIn(suc_pending_update_flag, nodeID);
      }
#ifdef ZW_CONTROLLER_BRIDGE
      case BRIDGE_NODE_FLAG:
      {
        return ZW_NodeMaskNodeIn(bridge_node_flag, nodeID);
      }
#endif
      case PENDING_DISCOVERY_FLAG:
      {
        return ZW_NodeMaskNodeIn(pending_discovery_flag, nodeID);
      }
      default:
      {
        ;
      }
    }
  }
  return false;
}

//Returns true if Flag is actually changed
static bool SetNodeFlag(node_id_t nodeID, bool IsFlagSet, uint8_t FlagMask, bool writeToNVM)
{
  uint8_t * pFlagNodeMask;
  zpal_nvm_object_key_t fileId;
  size_t fileSize;

  //Attempts to set flags for Z-Wave Long Range nodes must be rejected
  if(!NodeIdIsClassic(nodeID))
  {
    if(nodeID != WRITE_ALL_NODES_TO_NVM)
    {
      return false;
    }
  }

  switch (FlagMask)
  {
    case APP_ROUTE_LOCK_FLAG:
    {
      pFlagNodeMask = app_route_lock_flag;
      fileId = FILE_ID_APP_ROUTE_LOCK_FLAG;
      fileSize = FILE_SIZE_APP_ROUTE_LOCK_FLAG;
      break;
    }
    case ROUTE_SLAVE_SUC_FLAG:
    {
      pFlagNodeMask = route_slave_suc_flag;
      fileId = FILE_ID_ROUTE_SLAVE_SUC_FLAG;
      fileSize = FILE_SIZE_ROUTE_SLAVE_SUC_FLAG;
      break;
    }
    case SUC_PENDING_UPDATE_FLAG:
    {
      pFlagNodeMask = suc_pending_update_flag;
      fileId = FILE_ID_SUC_PENDING_UPDATE_FLAG;
      fileSize = FILE_SIZE_SUC_PENDING_UPDATE_FLAG;
      break;
    }
#ifdef ZW_CONTROLLER_BRIDGE
    case BRIDGE_NODE_FLAG:
    {
      pFlagNodeMask = bridge_node_flag;
      fileId = FILE_ID_BRIDGE_NODE_FLAG;
      fileSize = FILE_SIZE_BRIDGE_NODE_FLAG;
      break;
    }
#endif
    case PENDING_DISCOVERY_FLAG:
    {
      pFlagNodeMask = pending_discovery_flag;
      fileId = FILE_ID_PENDING_DISCOVERY_FLAG;
      fileSize = FILE_SIZE_PENDING_DISCOVERY_FLAG;
      break;
    }
    default:
    {
      return false;
    }
  }

  if (NodeInfoExists(nodeID))
  {
    //Dont write to files if content will not change
    if(IsFlagSet != ZW_NodeMaskNodeIn(pFlagNodeMask, nodeID))
    {
      if(IsFlagSet)
      {
        ZW_NodeMaskSetBit(pFlagNodeMask, nodeID);
      }
      else
      {
        ZW_NodeMaskClearBit(pFlagNodeMask, nodeID);
      }
      if (writeToNVM)
      {
        zpal_nvm_write(pFileSystem, fileId, pFlagNodeMask, fileSize);
      }
      return true;
    }
  }
  else if((nodeID == WRITE_ALL_NODES_TO_NVM) && writeToNVM)
  {
    zpal_nvm_write(pFileSystem, fileId, pFlagNodeMask, fileSize);
  }

  return false;
}

void
CtrlStorageGetRouteCache(ROUTE_CACHE_TYPE cacheType, node_id_t nodeID , ROUTECACHE_LINE  *pRouteCache)
{
  //nodeID 1-232
  //nodeNr 0-231  for local indexing in arrays
  uint16_t nodeNr = nodeID - 1;

  //Z-Wave Long Range nodes must be rejected
  if(!NodeIdIsClassic(nodeID))
  {
    return;
  }

  if (NodeRouteCacheExist(nodeID))
  {
     SNodeRouteCache * pNodeRouteCache;
     const ROUTECACHE_LINE * pStoredRouteCache;
     bool fileInRAMBuffer = false;

     uint8_t tFileBuf[FILE_SIZE_NODEROUTE_CACHE] = { 0 };

     //Read RouteCache from RAM buffer if it is there
     for(uint8_t i=0; i<NODEROUTECACHE_FILES_IN_RAM; i++)
     {
       if( nodeRouteCacheFileID[i] == (nodeNr / NODEROUTECACHES_PER_FILE) )
       {
         setToHighestBufferPrio(i);
         uint32_t positionInBuffer = i * FILE_SIZE_NODEROUTE_CACHE + (nodeNr % NODEROUTECACHES_PER_FILE) * sizeof(SNodeRouteCache);
         pNodeRouteCache = (SNodeRouteCache *)&nodeRouteCacheBuffer[positionInBuffer];
         fileInRAMBuffer = true;
       }
     }

     //Read from file if not in RAM buffer
     if(!fileInRAMBuffer)
     {
       if(ZPAL_STATUS_OK != zpal_nvm_read(pFileSystem, FILE_ID_NODEROUTE_CACHE_BASE + (nodeNr / NODEROUTECACHES_PER_FILE) , tFileBuf, FILE_SIZE_NODEROUTE_CACHE))
       {
         //File was not found. It was probably never saved. Remove corresponding flag bits in node_routecache_exists.
         uint32_t nodeIndex = nodeNr - (nodeNr % NODEROUTECACHES_PER_FILE);
         for (uint32_t j = 0; j < NODEROUTECACHES_PER_FILE; j++)
         {
           ZW_NodeMaskClearBit(node_routecache_exists, nodeIndex + j + 1);
         }
         zpal_nvm_write(pFileSystem, FILE_ID_NODE_ROUTECACHE_EXIST, &node_routecache_exists, sizeof(node_routecache_exists));

         //Set output to zeros and return
         memset(pRouteCache, 0, sizeof(ROUTECACHE_LINE));
         return;
       }
       pNodeRouteCache = (SNodeRouteCache *)&tFileBuf[(nodeNr % NODEROUTECACHES_PER_FILE) * sizeof(SNodeRouteCache)];
     }

     if (cacheType == ROUTE_CACHE_NORMAL)
     {
       pStoredRouteCache = &(pNodeRouteCache->routeCache);
     }
     else
     {
       pStoredRouteCache = &(pNodeRouteCache->routeCacheNlwrSr);
     }
     memcpy(
       pRouteCache,
       pStoredRouteCache,
       sizeof(ROUTECACHE_LINE)
      );
  }
  else
  {
    memset(
      pRouteCache,
      0,
      sizeof(ROUTECACHE_LINE)
    );
  }
}

//The function sets the input index as the highest priority part of the RAM buffer that should not be replaced
void setToHighestBufferPrio(uint32_t index)
{
  uint32_t i;

  //Find position of index in priority list
  for(i=0; i<(NODEROUTECACHE_FILES_IN_RAM - 1); i++)
  {
    if(index == nodeRouteCachePrio[i])
    {
      break;
    }
  }

  //Move back buffer positions that had higher priority in the ordered list
  while(i > 0)
  {
    nodeRouteCachePrio[i] = nodeRouteCachePrio[i-1];
    i--;
  }

  //Set highest priority to the input value
  nodeRouteCachePrio[0] = index;
}

//Get the lowest priority, longest since used, part of the RAM buffer that can be replaced.
//Returns 0xFF if there are free slots.
uint8_t getLowestBufferPrio(void)
{
  //return last element of ordered priority list
  return nodeRouteCachePrio[NODEROUTECACHE_FILES_IN_RAM - 1];
}

//In case a ROUTECACHE file is erased while it is in RAM buffer it must be removed from priority list.
void removeFromBufferPrio(uint32_t index)
{
  uint32_t i;

  //Find position of index in priority list
  for(i=0; i<NODEROUTECACHE_FILES_IN_RAM; i++)
  {
    if(index == nodeRouteCachePrio[i])
    {
      break;
    }
  }

  //Move up buffer positions that had lower priority in the ordered list
  while(i < (NODEROUTECACHE_FILES_IN_RAM - 1))
  {
    nodeRouteCachePrio[i] = nodeRouteCachePrio[i+1];
    i++;
  }

  //Set lowest priority index to 0xFF indicating that a slot is empty
  nodeRouteCachePrio[NODEROUTECACHE_FILES_IN_RAM - 1] = 0xFF;
}

void
CtrlStorageSetRouteCache(ROUTE_CACHE_TYPE cacheType, node_id_t nodeID , ROUTECACHE_LINE*  pRouteCache)
{
  //nodeID 1-232
  //nodeNr 0-231  for local indexing in arrays
  uint16_t nodeNr = nodeID - 1;

  //Z-Wave Long Range nodes must be rejected
  if(!NodeIdIsClassic(nodeID))
  {
    return;
  }

  const uint8_t* const pObjSrc = (uint8_t*)pRouteCache;
  SNodeRouteCache * pNodeRouteCache;
  bool fileInRAMBuffer = false;

  //If node is already in RAM buffer get pointer to nodes position in RAM
  for(uint8_t i=0; i<NODEROUTECACHE_FILES_IN_RAM; i++)
  {
    if(nodeRouteCacheFileID[i] == (nodeNr / NODEROUTECACHES_PER_FILE))
    {
      setToHighestBufferPrio(i);
      uint32_t positionInBuffer = i * FILE_SIZE_NODEROUTE_CACHE + (nodeNr % NODEROUTECACHES_PER_FILE) * sizeof(SNodeRouteCache);
      pNodeRouteCache = (SNodeRouteCache *)&nodeRouteCacheBuffer[positionInBuffer];
      fileInRAMBuffer = true;
    }
  }

  //If node is not in RAM buffer. Find empty space or replace one file in the buffer with file corresponding to node
  if(!fileInRAMBuffer)
  {
    //Select the buffer position with lowest priority for the new file
    uint8_t bufFilePosition = getLowestBufferPrio();


    if(0xFF == bufFilePosition)
    {
      //There is a unused slot in the buffer. Find it.
      for(uint8_t i=0; i<NODEROUTECACHE_FILES_IN_RAM; i++)
      {
        if(0xFF == nodeRouteCacheFileID[i])
        {
          bufFilePosition = i;
          break;
        }
      }
    }
    else
    {
      //Write previous RouteCaches in RAM buffer to NVM
      zpal_nvm_write(pFileSystem, FILE_ID_NODEROUTE_CACHE_BASE + nodeRouteCacheFileID[bufFilePosition], &nodeRouteCacheBuffer[bufFilePosition * FILE_SIZE_NODEROUTE_CACHE], FILE_SIZE_NODEROUTE_CACHE);
    }

    //Update RAM buffer with data from NVM if a file containing nodeNr is expected to exist.
    uint8_t nodeIndex = nodeNr - (nodeNr % NODEROUTECACHES_PER_FILE) + 1;
    if(NodeRouteCacheExist(nodeIndex) || (NodeRouteCacheNext(nodeIndex) < (nodeIndex + NODEROUTECACHES_PER_FILE)))
    {
      zpal_nvm_read(pFileSystem, FILE_ID_NODEROUTE_CACHE_BASE + (nodeNr / NODEROUTECACHES_PER_FILE), &nodeRouteCacheBuffer[bufFilePosition * FILE_SIZE_NODEROUTE_CACHE], FILE_SIZE_NODEROUTE_CACHE);
    }

    //Store the file number used to update the buffer
    nodeRouteCacheFileID[bufFilePosition] = nodeNr / NODEROUTECACHES_PER_FILE;

    //The old bufFilePosition position is updated with new data and is therefore set to highest priority.
    setToHighestBufferPrio(bufFilePosition);

    //Locate pointer to RouteCache in RAM.
    pNodeRouteCache = (SNodeRouteCache *)&nodeRouteCacheBuffer[bufFilePosition * FILE_SIZE_NODEROUTE_CACHE + (nodeNr % NODEROUTECACHES_PER_FILE) * sizeof(SNodeRouteCache)];
  }

  //Write correct data to RAM buffer
  if (NodeRouteCacheExist(nodeID))
  {
    if (cacheType == ROUTE_CACHE_NORMAL)
    {
      memcpy((uint8_t*)&(pNodeRouteCache->routeCache), pObjSrc, sizeof(ROUTECACHE_LINE));
    }
    else
    {
      memcpy((uint8_t*)&(pNodeRouteCache->routeCacheNlwrSr), pObjSrc, sizeof(ROUTECACHE_LINE));
    }
  }
  else
  {
    if (cacheType == ROUTE_CACHE_NORMAL)
    {
      memcpy((uint8_t*)&(pNodeRouteCache->routeCache), pObjSrc, sizeof(ROUTECACHE_LINE));
      memset((uint8_t*)&(pNodeRouteCache->routeCacheNlwrSr), 0, sizeof(ROUTECACHE_LINE));
    }
    else
    {
      memcpy((uint8_t*)&(pNodeRouteCache->routeCacheNlwrSr), pObjSrc, sizeof(ROUTECACHE_LINE));
      memset((uint8_t*)&(pNodeRouteCache->routeCache), 0, sizeof(ROUTECACHE_LINE));
    }
    //Save to NVM that ROUTECACHE for this node now exists
    ZW_NodeMaskSetBit(node_routecache_exists, nodeID);
    zpal_nvm_write(pFileSystem, FILE_ID_NODE_ROUTECACHE_EXIST , node_routecache_exists, sizeof(node_routecache_exists));
  }
}

#ifdef ZW_CONTROLLER_BRIDGE
bool
CtrlStorageGetBridgeNodeFlag(node_id_t nodeID)
{
  return GetNodeFlag(nodeID, BRIDGE_NODE_FLAG);
}

bool
CtrlStorageSetBridgeNodeFlag(node_id_t nodeID , bool IsBridgeNode, bool writeToNVM)
{
  return SetNodeFlag(nodeID, IsBridgeNode, BRIDGE_NODE_FLAG, writeToNVM);
}
#endif

bool
CtrlStorageGetRoutingSlaveSucUpdateFlag(node_id_t nodeID)
{
  return GetNodeFlag(nodeID, ROUTE_SLAVE_SUC_FLAG);
}

bool
CtrlStorageSetRoutingSlaveSucUpdateFlag(node_id_t nodeID , bool SlaveSucUpdate, bool writeToNVM)
{
  return SetNodeFlag(nodeID, SlaveSucUpdate, ROUTE_SLAVE_SUC_FLAG, writeToNVM);
}


bool
CtrlStorageGetAppRouteLockFlag(node_id_t nodeID)
{
  return GetNodeFlag(nodeID, APP_ROUTE_LOCK_FLAG);
}

void
CtrlStorageSetAppRouteLockFlag(node_id_t nodeID , bool LockRoute)
{
  SetNodeFlag(nodeID, LockRoute, APP_ROUTE_LOCK_FLAG, true);
}


bool
CtrlStorageIsNodeInPendingUpdateTable(node_id_t bNodeID)
{
  return GetNodeFlag(bNodeID, SUC_PENDING_UPDATE_FLAG);
}

void
CtrlStorageChangeNodeInPendingUpdateTable(node_id_t bNodeID, bool isPending)
{
  SetNodeFlag(bNodeID, isPending, SUC_PENDING_UPDATE_FLAG, true);
}


bool
CtrlStorageGetPendingDiscoveryStatus(node_id_t bNodeID)
{
  return GetNodeFlag(bNodeID, PENDING_DISCOVERY_FLAG);
}

void
CtrlStorageSetPendingDiscoveryStatus(node_id_t bNodeID, bool isPendingDiscovery)
{
  SetNodeFlag(bNodeID, isPendingDiscovery, PENDING_DISCOVERY_FLAG, true);
}


void
CtrlStorageGetRoutingInfo(node_id_t nodeID , NODE_MASK_TYPE* pRoutingInfo)
{
  //nodeID 1-232
  //nodeNr 0-231  for local indexing in arrays
  uint16_t nodeNr = nodeID - 1;

  if( !NodeIdIsClassic(nodeID) )
  {
    return;
  }

  if (NodeInfoExists(nodeID))
  {
    SNodeInfoStorage * pNodeInfo;
    uint8_t tFileBuffer[FILE_SIZE_NODEINFO] = { 0 };

    zpal_nvm_read(pFileSystem, FILE_ID_NODEINFO_BASE + (nodeNr / NODEINFOS_PER_FILE), &tFileBuffer, FILE_SIZE_NODEINFO);
    pNodeInfo = (SNodeInfoStorage *)&tFileBuffer[sizeof(SNodeInfoStorage) * (nodeNr % NODEINFOS_PER_FILE)];
    const uint8_t *pSrcNeighboursInfo =  (uint8_t *)&(pNodeInfo->neighboursInfo);

    memcpy(
       pRoutingInfo,
       pSrcNeighboursInfo,
       sizeof(NODE_MASK_TYPE)
     );
  }
  else
  {
    memset(
      pRoutingInfo,
      0,
      sizeof(NODE_MASK_TYPE)
    );
  }
}


bool
CtrlStorageRoutingInfoFileIsReady(node_id_t nodeID, node_id_t maxNodeID)
{
  //nodeID 1-232
  //nodeNr 0-231  for local indexing in arrays
  uint16_t nodeNr = nodeID - 1;

  if( !NodeIdIsClassic(nodeID) || !NodeIdIsClassic(maxNodeID))
  {
    //Reject Long Range node IDs
    return false;
  }

  if (nodeID == maxNodeID)
  {
    //Return true if the nodeID reaches maximum ID
    return true;
  }

  if ((nodeNr % NODEINFOS_PER_FILE) == (NODEINFOS_PER_FILE -1))
  {
    //Return true if the input value is the highest possible nodeNr in file
    return true;
  }

  //Loop over the nodes in the file that have higher nodeNr than the input value
  for (uint8_t i = ((nodeNr % NODEINFOS_PER_FILE) + 1); i < NODEINFOS_PER_FILE ; i++)
  {
    if (NodeInfoExists(nodeID + i))
    {
      //Return false if a node with higher nodeID exist in the file
      return false;
    }
  }
  //Return true if no node with higher nodeID exist in the file
  return true;
}

void
CtrlStorageSetRoutingInfo(node_id_t nodeID , NODE_MASK_TYPE* pRoutingInfo,  bool writeToNVM)
{
  if( !NodeIdIsClassic(nodeID) )
  {
    //Reject Long Range node IDs
    return;
  }

  if (NodeInfoExists(nodeID))
  {
    if (pRoutingInfo == NULL)
    {
      NODE_MASK_TYPE tempRoutingInfo = { 0 };
      CtrlNodeInfoStoragetWrite(nodeID, offsetof(SNodeInfoStorage, neighboursInfo), sizeof(NODE_MASK_TYPE), (uint8_t*)&tempRoutingInfo, writeToNVM);
    }
    else
    {
      CtrlNodeInfoStoragetWrite(nodeID, offsetof(SNodeInfoStorage, neighboursInfo), sizeof(NODE_MASK_TYPE), (uint8_t*)pRoutingInfo, writeToNVM);
    }
  }
  else
  {
    // Ignore the routing info write as node is probably not in existence
  }
}

#ifndef NO_PREFERRED_CALC
void
CtrlStorageGetPreferredRepeaters(NODE_MASK_TYPE* pPreferredRepeaters)
{
  zpal_nvm_read(pFileSystem, FILE_ID_PREFERREDREPEATERS, pPreferredRepeaters, sizeof(NODE_MASK_TYPE));
}

void
CtrlStorageSetPreferredRepeaters(NODE_MASK_TYPE* pPreferredRepeaters)
{
  zpal_nvm_write(pFileSystem, FILE_ID_PREFERREDREPEATERS, pPreferredRepeaters, sizeof(NODE_MASK_TYPE));
}
#endif  /*NO_PREFERRED_CALC*/


void
CtrlStorageSetControllerConfig(uint8_t  controllerConfig)
{
  SControllerInfo tControllerInfo = { 0 };
  zpal_nvm_read(pFileSystem, FILE_ID_CONTROLLERINFO, &tControllerInfo, sizeof(SControllerInfo));
  tControllerInfo.ControllerConfiguration = controllerConfig;
  zpal_nvm_write(pFileSystem, FILE_ID_CONTROLLERINFO, &tControllerInfo, sizeof(SControllerInfo));
}


uint8_t
CtrlStorageGetControllerConfig(void)
{
  SControllerInfo tControllerInfo = { 0 };
  zpal_nvm_read(pFileSystem, FILE_ID_CONTROLLERINFO, &tControllerInfo, sizeof(SControllerInfo));

  return tControllerInfo.ControllerConfiguration;
}


void
CtrlStorageSetSucLastIndex(uint8_t  SucLastIndex)
{
  SControllerInfo tControllerInfo = { 0 };
  zpal_nvm_read(pFileSystem, FILE_ID_CONTROLLERINFO, &tControllerInfo, sizeof(SControllerInfo));
  tControllerInfo.SucLastIndex = SucLastIndex;
  zpal_nvm_write(pFileSystem, FILE_ID_CONTROLLERINFO, &tControllerInfo, sizeof(SControllerInfo));
}


uint8_t
CtrlStorageGetSucLastIndex(void)
{
  SControllerInfo tControllerInfo = { 0 };
  zpal_nvm_read(pFileSystem, FILE_ID_CONTROLLERINFO, &tControllerInfo, sizeof(SControllerInfo));

  return tControllerInfo.SucLastIndex;
}

void
CtrlStorageSetLastUsedLongRangeNodeId(node_id_t  LastUsedNodeID_LR)
{
  SControllerInfo tControllerInfo = { 0 };
  zpal_nvm_read(pFileSystem, FILE_ID_CONTROLLERINFO, &tControllerInfo, sizeof(SControllerInfo));
  tControllerInfo.LastUsedNodeId_LR = LastUsedNodeID_LR;
  zpal_nvm_write(pFileSystem, FILE_ID_CONTROLLERINFO, &tControllerInfo, sizeof(SControllerInfo));
}


node_id_t
CtrlStorageGetLastUsedLongRangeNodeId(void)
{
  SControllerInfo tControllerInfo = { 0 };
  zpal_nvm_read(pFileSystem, FILE_ID_CONTROLLERINFO, &tControllerInfo, sizeof(SControllerInfo));

  return tControllerInfo.LastUsedNodeId_LR;
}

void
CtrlStorageSetLastUsedNodeId(node_id_t  LastUsedNodeID)
{
  if(!NodeIdIsClassic(LastUsedNodeID))
  {
    //Reject Long Range node IDs
    return;
  }
  SControllerInfo tControllerInfo = { 0 };
  zpal_nvm_read(pFileSystem, FILE_ID_CONTROLLERINFO, &tControllerInfo, sizeof(SControllerInfo));
  tControllerInfo.LastUsedNodeId = LastUsedNodeID;
  zpal_nvm_write(pFileSystem, FILE_ID_CONTROLLERINFO, &tControllerInfo, sizeof(SControllerInfo));
}


node_id_t
CtrlStorageGetLastUsedNodeId(void)
{
  SControllerInfo tControllerInfo = { 0 };
  zpal_nvm_read(pFileSystem, FILE_ID_CONTROLLERINFO, &tControllerInfo, sizeof(SControllerInfo));
  //Please note that an 8-bit value is returned.
  return tControllerInfo.LastUsedNodeId;
}

void
CtrlStorageSetStaticControllerNodeId(node_id_t  StaticControllerNodeId)
{
  SControllerInfo tControllerInfo = { 0 };
  zpal_nvm_read(pFileSystem, FILE_ID_CONTROLLERINFO, &tControllerInfo, sizeof(SControllerInfo));
  tControllerInfo.StaticControllerNodeId = StaticControllerNodeId;
  zpal_nvm_write(pFileSystem, FILE_ID_CONTROLLERINFO, &tControllerInfo, sizeof(SControllerInfo));
}

node_id_t
CtrlStorageGetStaticControllerNodeId(void)
{
  SControllerInfo tControllerInfo = { 0 };
  zpal_nvm_read(pFileSystem, FILE_ID_CONTROLLERINFO, &tControllerInfo, sizeof(SControllerInfo));

  return tControllerInfo.StaticControllerNodeId;
}

void
CtrlStorageSetMaxLongRangeNodeId(node_id_t  MaxNodeId_LR)
{
  SControllerInfo tControllerInfo = { 0 };
  zpal_nvm_read(pFileSystem, FILE_ID_CONTROLLERINFO, &tControllerInfo, sizeof(SControllerInfo));
  tControllerInfo.MaxNodeId_LR = MaxNodeId_LR;
  zpal_nvm_write(pFileSystem, FILE_ID_CONTROLLERINFO, &tControllerInfo, sizeof(SControllerInfo));
}

node_id_t
CtrlStorageGetMaxLongRangeNodeId(void)
{
  SControllerInfo tControllerInfo = { 0 };
  zpal_nvm_read(pFileSystem, FILE_ID_CONTROLLERINFO, &tControllerInfo, sizeof(SControllerInfo));

  return tControllerInfo.MaxNodeId_LR;
}

void
CtrlStorageSetMaxNodeId(node_id_t  MaxNodeId)
{
  if(ZW_nodeIsLRNodeID(MaxNodeId))
  {
    //Reject Long Range node IDs
    return;
  }
  SControllerInfo tControllerInfo = { 0 };
  zpal_nvm_read(pFileSystem, FILE_ID_CONTROLLERINFO, &tControllerInfo, sizeof(SControllerInfo));
  tControllerInfo.MaxNodeId = MaxNodeId;
  zpal_nvm_write(pFileSystem, FILE_ID_CONTROLLERINFO, &tControllerInfo, sizeof(SControllerInfo));
}

node_id_t
CtrlStorageGetMaxNodeId(void)
{
  SControllerInfo tControllerInfo = { 0 };
  zpal_nvm_read(pFileSystem, FILE_ID_CONTROLLERINFO, &tControllerInfo, sizeof(SControllerInfo));
  //Please note that an 8-bit value is returned.
  return tControllerInfo.MaxNodeId;
}

void
CtrlStorageSetReservedLongRangeId(node_id_t  ReservedId_LR)
{
  SControllerInfo tControllerInfo = { 0 };
  zpal_nvm_read(pFileSystem, FILE_ID_CONTROLLERINFO, &tControllerInfo, sizeof(SControllerInfo));
  tControllerInfo.ReservedId_LR = ReservedId_LR;
  zpal_nvm_write(pFileSystem, FILE_ID_CONTROLLERINFO, &tControllerInfo, sizeof(SControllerInfo));
}

node_id_t
CtrlStorageGetReservedLongRangeId(void)
{
  SControllerInfo tControllerInfo = { 0 };
  zpal_nvm_read(pFileSystem, FILE_ID_CONTROLLERINFO, &tControllerInfo, sizeof(SControllerInfo));

  return tControllerInfo.ReservedId_LR;
}

void
CtrlStorageSetReservedId(node_id_t  ReservedId)
{
  //Reject Long Range node IDs but allow 0. Value 0 means ReservedID is unassigned.
  if(ZW_MAX_NODES < ReservedId)
  {
    return;
  }
  SControllerInfo tControllerInfo;
  zpal_nvm_read(pFileSystem, FILE_ID_CONTROLLERINFO, &tControllerInfo, sizeof(SControllerInfo));
  tControllerInfo.ReservedId = ReservedId;
  zpal_nvm_write(pFileSystem, FILE_ID_CONTROLLERINFO, &tControllerInfo, sizeof(SControllerInfo));
}

node_id_t
CtrlStorageGetReservedId(void)
{
  SControllerInfo tControllerInfo = { 0 };
  zpal_nvm_read(pFileSystem, FILE_ID_CONTROLLERINFO, &tControllerInfo, sizeof(SControllerInfo));
  //Please note that an 8-bit value is returned.
  return tControllerInfo.ReservedId;
}

void
ControllerStorageSetNetworkIds(const uint8_t *pHomeID, node_id_t bNodeID)
{
  SControllerInfo tControllerInfo = { 0 };
  zpal_nvm_read(pFileSystem, FILE_ID_CONTROLLERINFO, &tControllerInfo, sizeof(SControllerInfo));
  uint32_t* pTempHomeId = (uint32_t*)&tControllerInfo.HomeID[0]; // Use pTempHomeId to avoid GCC type punned pointer error
  *pTempHomeId = *((uint32_t*)pHomeID);
  tControllerInfo.NodeID = bNodeID;

  zpal_nvm_write(pFileSystem, FILE_ID_CONTROLLERINFO, &tControllerInfo, sizeof(SControllerInfo));
}

void
ControllerStorageGetNetworkIds(uint8_t *pHomeID, node_id_t *pNodeID)
{
  SControllerInfo tControllerInfo = { 0 };
  zpal_nvm_read(pFileSystem, FILE_ID_CONTROLLERINFO, &tControllerInfo, sizeof(SControllerInfo));

  memcpy(pHomeID, &tControllerInfo.HomeID[0], HOMEID_LENGTH);
  if (NULL != pNodeID) {
   *pNodeID = tControllerInfo.NodeID;
  }
}

void
CtrlStorageSetNodeID(node_id_t NodeID)
{
  SControllerInfo tControllerInfo = { 0 };
  zpal_nvm_read(pFileSystem, FILE_ID_CONTROLLERINFO, &tControllerInfo, sizeof(SControllerInfo));
  tControllerInfo.NodeID = NodeID;
  zpal_nvm_write(pFileSystem, FILE_ID_CONTROLLERINFO, &tControllerInfo, sizeof(SControllerInfo));
}

node_id_t
CtrlStorageGetNodeID(void)
{
  SControllerInfo tControllerInfo = { 0 };
  zpal_nvm_read(pFileSystem, FILE_ID_CONTROLLERINFO, &tControllerInfo, sizeof(SControllerInfo));

  return tControllerInfo.NodeID;
}


void
CtrlStorageSetCmdClassInSucUpdateEntry(uint8_t bSucUpdateIndex, uint8_t * cmdClasses)
{
  if (SUC_MAX_UPDATES <= bSucUpdateIndex)
  {
    return;
  }

  SSucNodeList tSucNodeList;
  memset(&tSucNodeList, 0xff, sizeof(tSucNodeList));
  zpal_nvm_object_key_t fileId = FILE_ID_SUCNODELIST_BASE + (bSucUpdateIndex / SUCNODES_PER_FILE);
  zpal_nvm_read(pFileSystem, fileId, &tSucNodeList, sizeof(SSucNodeList));

  uint8_t index = bSucUpdateIndex % SUCNODES_PER_FILE;
  memcpy(tSucNodeList.aSucNodeList[index].nodeInfo, cmdClasses, SUC_UPDATE_NODEPARM_MAX);
  zpal_nvm_write(pFileSystem, fileId, &tSucNodeList, sizeof(SSucNodeList));
}

void
CtrlStorageSetSucUpdateEntry(uint8_t bSucUpdateIndex, uint8_t bChangeType, node_id_t bNodeID)
{
  //Attempts to set flags for Z-Wave Long Range nodes must be rejected
  if(!NodeIdIsClassic(bNodeID))
  {
    //Reject Long Range node IDs
    return;
  }

  if (SUC_MAX_UPDATES <= bSucUpdateIndex)
  {
    return;
  }

  SSucNodeList tSucNodeList;
  memset(&tSucNodeList, 0xff, sizeof(tSucNodeList));
  zpal_nvm_object_key_t fileId = FILE_ID_SUCNODELIST_BASE + (bSucUpdateIndex / SUCNODES_PER_FILE);
  zpal_nvm_read(pFileSystem, fileId, &tSucNodeList, sizeof(SSucNodeList));

  uint8_t index = bSucUpdateIndex % SUCNODES_PER_FILE;
  tSucNodeList.aSucNodeList[index].changeType = bChangeType;
  //The SucNodeList is only used for classic nodes so the NodeID is stored in 8-bit format.
  tSucNodeList.aSucNodeList[index].NodeID = (uint8_t)bNodeID;
  zpal_nvm_write(pFileSystem, fileId, &tSucNodeList, sizeof(SSucNodeList));
}


void
CtrlStorageGetSucUpdateEntry(uint8_t bSucUpdateIndex, SUC_UPDATE_ENTRY_STRUCT *SucUpdateEntry)
{
  if (SUC_MAX_UPDATES <= bSucUpdateIndex)
  {
    return;
  }

  SUC_UPDATE_ENTRY_STRUCT tSucNode = { 0 };
  zpal_nvm_object_key_t fileId = FILE_ID_SUCNODELIST_BASE + (bSucUpdateIndex / SUCNODES_PER_FILE);
  size_t offset = (bSucUpdateIndex % SUCNODES_PER_FILE) * sizeof(SUC_UPDATE_ENTRY_STRUCT);
  zpal_nvm_read_object_part(pFileSystem, fileId, &tSucNode, offset, sizeof(tSucNode));

  SucUpdateEntry->changeType = tSucNode.changeType;
  memcpy(SucUpdateEntry->nodeInfo, tSucNode.nodeInfo, SUC_UPDATE_NODEPARM_MAX);
  if (0xff == tSucNode.NodeID)
  {
    SucUpdateEntry->NodeID = 0x00;
  }
  else
  {
    SucUpdateEntry->NodeID = tSucNode.NodeID;
  }
}

#ifdef ZW_CONTROLLER_BRIDGE
void
CtrlStorageReadBridgeNodePool(uint8_t *bridgeNodePool)
{
  memset(bridgeNodePool, 0, MAX_NODEMASK_LENGTH);
  for (uint8_t i = 0; i < ZW_MAX_NODES -1; i++)
  {
    if (CtrlStorageGetBridgeNodeFlag(i+1))
    {
      bridgeNodePool[(i >>3)] |= (1<< (i & 0x07));
    }
  }
}

void
CtrlStorageWriteBridgeNodePool(uint8_t *bridgeNodePool)
{
  bool isBridgeNode;
  bool writeToNVM = false;

  for (uint8_t i = 0; i < ZW_MAX_NODES -1; i++)
  {
    isBridgeNode = ( bridgeNodePool[(i >>3)] & (1<< (i & 0x07)) );
    if(CtrlStorageSetBridgeNodeFlag(i+1, isBridgeNode, false))
    {
      writeToNVM = true;
    }
  }
  if (writeToNVM)
  {
    zpal_nvm_write(pFileSystem, FILE_ID_BRIDGE_NODE_FLAG, bridge_node_flag, FILE_SIZE_BRIDGE_NODE_FLAG);
  }
}
#endif



void CtrlStorageSetSmartStartState(uint8_t SystemState)
{
  SControllerInfo tControllerInfo = { 0 };
  zpal_nvm_read(pFileSystem, FILE_ID_CONTROLLERINFO, &tControllerInfo, sizeof(SControllerInfo));
  tControllerInfo.SystemState = SystemState;
  zpal_nvm_write(pFileSystem, FILE_ID_CONTROLLERINFO, &tControllerInfo, sizeof(SControllerInfo));
}

uint8_t CtrlStorageGetSmartStartState(void)
{
  SControllerInfo tControllerInfo = { 0 };
  zpal_nvm_read(pFileSystem, FILE_ID_CONTROLLERINFO, &tControllerInfo, sizeof(SControllerInfo));
  return tControllerInfo.SystemState;
}

void StorageSetPrimaryLongRangeChannelId(zpal_radio_lr_channel_t channelId)
{
  SControllerInfo tControllerInfo = { 0 };
  zpal_nvm_read(pFileSystem, FILE_ID_CONTROLLERINFO, &tControllerInfo, sizeof(SControllerInfo));
  tControllerInfo.PrimaryLongRangeChannelId = channelId;
  zpal_nvm_write(pFileSystem, FILE_ID_CONTROLLERINFO, &tControllerInfo, sizeof(SControllerInfo));
}

void StorageSetLongRangeChannelAutoMode(bool enable )
{
  SControllerInfo tControllerInfo = { 0 };
  zpal_nvm_read(pFileSystem, FILE_ID_CONTROLLERINFO, &tControllerInfo, sizeof(SControllerInfo));
  tControllerInfo.LonRangeChannelAutoMode = (uint8_t)enable;
  zpal_nvm_write(pFileSystem, FILE_ID_CONTROLLERINFO, &tControllerInfo, sizeof(SControllerInfo));
}


zpal_radio_lr_channel_t StorageGetPrimaryLongRangeChannelId(void)
{
  SControllerInfo tControllerInfo = { 0 };
  zpal_nvm_read(pFileSystem, FILE_ID_CONTROLLERINFO, &tControllerInfo, sizeof(SControllerInfo));
  return tControllerInfo.PrimaryLongRangeChannelId;
}

bool StorageGetLongRangeChannelAutoMode(void)
{
  SControllerInfo tControllerInfo = { 0 };
  zpal_nvm_read(pFileSystem, FILE_ID_CONTROLLERINFO, &tControllerInfo, sizeof(SControllerInfo));
  return (bool)tControllerInfo.LonRangeChannelAutoMode;
}


bool CtrlStorageGetZWVersion(uint32_t * version)
{
  if( ZPAL_STATUS_OK == zpal_nvm_read(pFileSystem, FILE_ID_ZW_VERSION, version, sizeof(uint32_t)) )
  {
    return true;
  }

  return false;
}

bool CtrlStorageSetZWVersion(const uint32_t * version)
{
  if( ZPAL_STATUS_OK == zpal_nvm_write(pFileSystem, FILE_ID_ZW_VERSION, version, sizeof(uint32_t)) )
  {
    return true;
  }

  return false;
}

bool CtrlStorageLongRangeGet(node_id_t nodeID)
{
  return NodeInfoLongRangeExists(nodeID);
}

void CtrlStorageLongRangeSet(node_id_t nodeID)
{
  SetLongRangeExists(nodeID);
}

void MergeNodeFiles(zpal_nvm_object_key_t originalBaseID,
                    zpal_nvm_object_key_t mergedBaseID,
                    NODE_MASK_TYPE node_exist_map,
                    uint32_t  originalIdRange,
                    uint32_t  mergeN,
                    uint32_t  originalFileSize,
                    uint32_t  mergedFileSize,
                    uint8_t * pFileBuffer)
{
  const uint32_t mergedIdRange = (originalIdRange / mergeN) + 1;
  zpal_nvm_object_key_t originalFileID;
  uint32_t nodeID;
  bool writeMergedFile;

  for(uint32_t i=0; i<mergedIdRange; i++)
  {
    writeMergedFile = false;
    memset(pFileBuffer, 0xFF, mergedFileSize);

    //Read original files
    for(uint32_t j=0; j<mergeN; j++)
    {
      nodeID = (i * mergeN) + j;
      if (ZW_NodeMaskNodeIn(node_exist_map, nodeID + 1))
      {
        originalFileID = originalBaseID + nodeID;
        zpal_nvm_read(pFileSystem, originalFileID, pFileBuffer + (j * originalFileSize), originalFileSize);
        writeMergedFile = true;
      }
    }

    //Write merged file
    if(writeMergedFile)
    {
      zpal_nvm_write(pFileSystem, mergedBaseID + i, pFileBuffer, mergedFileSize);
    }

    //Delete original files
    for(uint32_t j=0; j<mergeN; j++)
    {
      nodeID = (i * mergeN) + j;
      if (ZW_NodeMaskNodeIn(node_exist_map, nodeID + 1))
      {
        originalFileID = originalBaseID + nodeID;
        zpal_nvm_erase_object(pFileSystem, originalFileID);
      }
    }
  }
}


/***************************************************************************************
 * Functions and definitions needed for File-System Migration
 **************************************************************************************/

/// This will allow us to save some program memory on redundant migration functions and tables.
#define FILE_ID_LR_TX_POWER_BASE_V2       (0x00014)  // The base file Id for the TX Power files in nvm during file system version 2.
#define FILE_ID_LR_TX_POWER_FILE_COUNT_V2 (1024 / 64)

#define FILE_ID_LR_TX_POWER_BASE_V3       (0x02000)  // The base file Id for the TX Power files in nvm during file system version 3.
#define FILE_ID_LR_TX_POWER_FILE_COUNT_V3 (1024 / 32)

#ifdef ZWAVE_MIGRATE_FILESYSTEM
/**
 * Since tx power is no longer stored on flash memory,
 * this migration function deletes all related files from flash/NVM3.
 *
 * @return the version number to which it has migrated.
 */
static
uint8_t MigrateTxPowerFileSystemToV4FromV3(void)
{
  /**
   * FILE_ID_LR_TX_POWER is being removed in this version!
   */

  /*
   * This operation deletes all fileIDs for the FILE_ID_LR_TX_POWER_BASE for the
   * entire range of both FS version 2 and 3.
   */

  uint32_t fileIDs[]    = { FILE_ID_LR_TX_POWER_BASE_V2,        FILE_ID_LR_TX_POWER_BASE_V3       };
  uint32_t fileCount[]  = { FILE_ID_LR_TX_POWER_FILE_COUNT_V2,  FILE_ID_LR_TX_POWER_FILE_COUNT_V3 };

  for (int i = 0; i < 2; i++)  // We need to cover to version migrations.
  {
    // Read file by file for all valid fileID allocated for tx power storage.
    for (zpal_nvm_object_key_t fileID = fileIDs[i]; fileID < fileIDs[i] + fileCount[i]; fileID++)  // (1024 / 64) - 1 = 15, gives 16 file count for the old file system.
    {
      // Only delete as many as the number of files generated, not the entire fileID range as previously allocated.
      zpal_nvm_erase_object(pFileSystem, fileID);
    }
  }

  return 4;  // The version the migration resulted into.
}

/**
 * To improve lifetime of the NVM3 file system the SUCNODELIST file
 * is split into 8 smaller files that are only created if data exists.
 *
 * @return the version number to which it has migrated.
 */
static
uint8_t MigrateFileSystemToV5FromV4(void)
{
  /**
   * FILE_ID_SUCNODELIST_LEGACY_v4 is being removed in this version!
   */

  //There are max 64 entries in the original file. (SUC_MAX_UPDATES)
  //Read each entry. Check if data is valid. In that case write it to smaller
  //files with max SUCNODES_PER_FILES entries per file.
  for (uint32_t i=0; i<(64/SUCNODES_PER_FILE); i++)
  {
    //Struct used for v5 files
    SSucNodeList tSucNodeList;
    memset(&tSucNodeList, 0xff, sizeof(tSucNodeList));
    bool saveData = false;

    for (uint32_t j=0; j<SUCNODES_PER_FILE; j++)
    {
      SUC_UPDATE_ENTRY_STRUCT tSucNode = { 0 }; //Single SUC entry
      size_t offset = (i * sizeof(SSucNodeList)) + (j * sizeof(SUC_UPDATE_ENTRY_STRUCT));
      zpal_status_t status = zpal_nvm_read_object_part(pFileSystem, FILE_ID_SUCNODELIST_LEGACY_v4, &tSucNode, offset, sizeof(tSucNode));

      //NodeID != 0 indicates valid data in tSucNode
      if ((ZPAL_STATUS_OK == status) && tSucNode.NodeID)
      {
        memcpy(&(tSucNodeList.aSucNodeList[j]), &tSucNode, sizeof(tSucNode));
        saveData = true;
      }
    }

    //Only save file if there is valid data in it.
    if (saveData)
    {
      zpal_nvm_write(pFileSystem, FILE_ID_SUCNODELIST_BASE + i, &tSucNodeList, sizeof(SSucNodeList));
    }
  }
  //Migration finished. Erase legacy file.
  zpal_nvm_erase_object(pFileSystem, FILE_ID_SUCNODELIST_LEGACY_v4);

  return 5;  // The version the migration resulted into.
}
#endif // ZWAVE_MIGRATE_FILESYSTEM

static void
FileSystemMigrationManagement(void)
{
  //Read present file system version file
  bool     versionFileExists;
  uint32_t presentVersion = 0;
  uint32_t presentFilesysVersion;
  uint32_t expectedFilesysVersion;  // This will hold the file system version that current SW will support.

  versionFileExists = CtrlStorageGetZWVersion(&presentVersion);

  // Find ZW_CONTROLLER_FILESYS_VERSION of the present file system.
  presentFilesysVersion = ZW_CONTROLLER_FILESYS_VERSION_GET(presentVersion);
  DPRINTF("presentFilesysVersion: %d \n", presentFilesysVersion);

  // Find ZW_CONTROLLER_FILESYS_VERSION compatible with this current SW version.
  expectedFilesysVersion = ZW_CONTROLLER_FILESYS_VERSION_GET(ZW_Version);
  DPRINTF("expectedFilesysVersion: %d \n", expectedFilesysVersion);

  if(!versionFileExists)
  {
    //File system area is brand new or corrupt. Format it.
    bool bFormatStatus = NvmFileSystemFormat();
    // FileSystemFormattedCb callback of NvmFileSystemRegister() is called after formatting.
    ASSERT(bFormatStatus);
  }
  else if(expectedFilesysVersion < presentFilesysVersion)
  {
    //System downgrade. Should not be allowed. The bootloader is configured to prevent this.
    return;
  }
  else if(expectedFilesysVersion > presentFilesysVersion)  // File system upgrade needed. Initiating file system migration...
  {
#ifdef ZWAVE_MIGRATE_FILESYSTEM
    /**
     * Continuous migration until all needed migrations are performed,
     * to lift from any version to the latest file system version.
     */

    //Migrate files from file system version 0 to 1.
    if ( presentFilesysVersion == 0)
    {
      //read file that keep track of which nodeinfo files exist
      zpal_nvm_read(pFileSystem, FILE_ID_NODE_STORAGE_EXIST, &node_info_exists, sizeof(node_info_exists));
      //merge nodeinfo files
      uint32_t originalBaseID   = 0x00100;
      uint32_t originalIdRange  = 232;
      uint32_t originalFileSize = 35;
      uint8_t  nodeInfoFileBuffer[FILE_SIZE_NODEINFO];
      MergeNodeFiles(originalBaseID, FILE_ID_NODEINFO_BASE, node_info_exists, originalIdRange, NODEINFOS_PER_FILE, originalFileSize, FILE_SIZE_NODEINFO, nodeInfoFileBuffer);

      //read file that keep track of which node route caches exist
      zpal_nvm_read(pFileSystem, FILE_ID_NODE_ROUTECACHE_EXIST, &node_routecache_exists, sizeof(node_routecache_exists));
      //merge routecache files
      originalBaseID   = 0x00400;
      originalIdRange  = 232;  //ZW_MAX_NODES
      originalFileSize = 10;
      uint8_t nodeRouteCacheFileBuffer[FILE_SIZE_NODEROUTE_CACHE];
      MergeNodeFiles(originalBaseID, FILE_ID_NODEROUTE_CACHE_BASE, node_routecache_exists, originalIdRange, NODEROUTECACHES_PER_FILE, originalFileSize, FILE_SIZE_NODEROUTE_CACHE, nodeRouteCacheFileBuffer);

      // Lifted to version 1
    }

    //Migrate files from file system version 0 or 1 to 2.
    if ( presentFilesysVersion <= 1)
    {
      //SControllerInfo nodeID changed from uint8_t to uint16_t
      SControllerInfo_OLD oldControllerInfo;
      zpal_nvm_read(pFileSystem, FILE_ID_CONTROLLERINFO, &oldControllerInfo, sizeof(SControllerInfo_OLD));

      SControllerInfo controllerInfo;
      memset(&controllerInfo, 0xFF, sizeof(SControllerInfo));

      //Copy old data to new positions
      memcpy(&controllerInfo.HomeID, &oldControllerInfo.HomeID, HOMEID_LENGTH);
      controllerInfo.NodeID = oldControllerInfo.NodeID;
      controllerInfo.LastUsedNodeId = oldControllerInfo.LastUsedNodeId;
      controllerInfo.StaticControllerNodeId = oldControllerInfo.StaticControllerNodeId;
      controllerInfo.SucLastIndex = oldControllerInfo.SucLastIndex;
      controllerInfo.ControllerConfiguration = oldControllerInfo.ControllerConfiguration;
      controllerInfo.MaxNodeId = oldControllerInfo.MaxNodeId;
      controllerInfo.ReservedId = oldControllerInfo.ReservedId;
      controllerInfo.SystemState = oldControllerInfo.SystemState;
      //Set Long Range specific data to default values
      controllerInfo.LastUsedNodeId_LR = LOWEST_LONG_RANGE_NODE_ID - 1;
      controllerInfo.MaxNodeId_LR = 0; // 0 to indicate no LR node IDs have been assigned yet
      controllerInfo.ReservedId_LR = 0x0000;
      controllerInfo.PrimaryLongRangeChannelId = 0;

      zpal_nvm_write(pFileSystem, FILE_ID_CONTROLLERINFO, &controllerInfo, sizeof(SControllerInfo));

      //Remove files related to S2 security since they were never in use for controllers.
      zpal_nvm_erase_object(pFileSystem, FILE_ID_S2_KEYS);
      zpal_nvm_erase_object(pFileSystem, FILE_ID_S2_KEYCLASSES_ASSIGNED);
      zpal_nvm_erase_object(pFileSystem, FILE_ID_S2_MPAN);
      zpal_nvm_erase_object(pFileSystem, FILE_ID_S2_SPAN);

      //Create FILE_ID_LRANGE_NODE_EXIST file
      memset(&node_info_Lrange_exists, 0x00, sizeof(node_info_Lrange_exists));
      zpal_nvm_write(pFileSystem, FILE_ID_LRANGE_NODE_EXIST, &node_info_Lrange_exists, sizeof(node_info_Lrange_exists));

#ifdef NO_PREFERRED_CALC
      //Remove file if it is not in use
      zpal_nvm_erase_object(pFileSystem, FILE_ID_PREFERREDREPEATERS);
#endif /*NO_PREFERRED_CALC */

      // Lifted to version 2
    }

    /*
     * Migration from version 2 to 3 has been removed. We move directly to version 4 with below migration.
     */

    // Migrate files from file system version 3 to 4.
    if ( presentFilesysVersion <= 3 )
    {
      MigrateTxPowerFileSystemToV4FromV3();  // This migration deletes all files related tx power from file system.

      // Lifted to version 4
    }

    // Migrate files from file system version 4 to 5.
    if ( presentFilesysVersion <= 4 )
    {
      //On 800s devices the application file system was merged to the protocol file system in v5.
      //Migration code for this is in zpal_nvm.c
      zpal_nvm_migrate_legacy_app_file_system();

      //Migrate FILE_ID_SUCNODELIST into 8 smaller files.
      MigrateFileSystemToV5FromV4();
    }

#endif // ZWAVE_MIGRATE_FILESYSTEM

    /**
     * @attention Don't forget to update ZW_CONTROLLER_FILESYS_VERSION!
     */

    /**
     * Write the new file system version number to NMV.
     */
    // Update File System Version file to the current version supported by the firmware.
    CtrlStorageSetZWVersion(&ZW_Version);

    // Run-time unit-test: Make sure that the file system version changed successfully.
    DEBUG_CODE(
      CtrlStorageGetZWVersion(&presentVersion);
      // Find ZW_CONTROLLER_FILESYS_VERSION of the present file system.
      presentFilesysVersion = ZW_CONTROLLER_FILESYS_VERSION_GET(presentVersion);
      // Find ZW_CONTROLLER_FILESYS_VERSION compatible with this current SW version.
      expectedFilesysVersion = ZW_CONTROLLER_FILESYS_VERSION_GET(ZW_Version);
      ASSERT(presentFilesysVersion == expectedFilesysVersion);  // Are we ready to operate as expected?
    );
  }
}

static
void Ctrl_storage_init (SObjectSet* pFileSet, bool force)
{
  bool set_file_ok = true;
  if (force)
  {
    zpal_nvm_object_key_t objectKey;
    DPRINT("FileSystem Verify failed\r\n");
    for(uint8_t i = 0; i < pFileSet->iObjectCount; i++)
    {
      objectKey = pFileSet->pObjectDescriptors[i].ObjectKey;

      if(ECTKR_STATUS_UNABLE_TO_AQUIRE_HANDLE == m_aFileStatus[i])
      {
        DPRINTF("Object Key: 0x%x not found\r\n", objectKey);
        //Write new file with default data.
        set_file_ok = set_file_ok && SetFile(objectKey, 0);
      }
      else if(ECTKR_STATUS_SIZE_MISMATCH == m_aFileStatus[i])
      {
        DPRINTF("Object: 0x%x size mismatch\r\n", objectKey);
        size_t   dataLen = 0;
        zpal_nvm_get_object_size(pFileSystem, objectKey, &dataLen);

        //Expected file size different than present file. Copy present file to new. Zero pad end of new file if longer than old file.
        set_file_ok = set_file_ok && SetFile(objectKey, dataLen);
      }
      ASSERT(set_file_ok);
    }
  }
  //read file that keep track of which nodeinfo files exist
  zpal_nvm_read(pFileSystem, FILE_ID_NODE_STORAGE_EXIST, &node_info_exists, sizeof(node_info_exists));

  //read file that keep track of which node routechache exist
  zpal_nvm_read(pFileSystem, FILE_ID_NODE_ROUTECACHE_EXIST, &node_routecache_exists, sizeof(node_routecache_exists));

  //read file that keep track of which long range nodeinfo files exist
  zpal_nvm_read(pFileSystem, FILE_ID_LRANGE_NODE_EXIST, &node_info_Lrange_exists, sizeof(node_info_Lrange_exists));

  //read NodeRouteCache from files and store in RAM
  uint32_t buffIndex = 0;
  memset(nodeRouteCacheBuffer, 0xFF, sizeof(nodeRouteCacheBuffer));
  memset(nodeRouteCacheFileID, 0xFF, sizeof(nodeRouteCacheFileID));
  memset(nodeRouteCachePrio,   0xFF, sizeof(nodeRouteCachePrio));
  for(uint32_t i = 0; i < NUMBER_OF_NODEROUTECACHE_FILES; i++)
  {
    uint8_t nodeIndex = i * NODEROUTECACHES_PER_FILE + 1;

    //Check if the file is expected to exist
    if(NodeRouteCacheExist(nodeIndex) || (NodeRouteCacheNext(nodeIndex) < (nodeIndex + NODEROUTECACHES_PER_FILE)))
    {
      nodeRouteCacheFileID[buffIndex] = i;
      setToHighestBufferPrio(buffIndex);
      uint8_t * pFilePosInBuffer = &nodeRouteCacheBuffer[buffIndex * FILE_SIZE_NODEROUTE_CACHE];
      if(ZPAL_STATUS_OK != zpal_nvm_read(pFileSystem, FILE_ID_NODEROUTE_CACHE_BASE + i, pFilePosInBuffer, FILE_SIZE_NODEROUTE_CACHE))
      {
        //File was not found. It was probably never saved. Remove corresponding flag bits in node_routecache_exists.
        for (uint32_t j = 0; j < NODEROUTECACHES_PER_FILE; j++)
        {
          ZW_NodeMaskClearBit(node_routecache_exists, nodeIndex + j);
        }
        zpal_nvm_write(pFileSystem, FILE_ID_NODE_ROUTECACHE_EXIST, &node_routecache_exists, sizeof(node_routecache_exists));
      }
      else
      {
        buffIndex++;
      }
    }

    if(buffIndex >= NODEROUTECACHE_FILES_IN_RAM)
    {
      //NODEROUTECACHE RAM buffer is full. Read no more files even though they may exist.
      break;
    }
  }
  //read stored flags to RAM
  zpal_nvm_read(pFileSystem, FILE_ID_APP_ROUTE_LOCK_FLAG,     &app_route_lock_flag,     sizeof(app_route_lock_flag));
  zpal_nvm_read(pFileSystem, FILE_ID_ROUTE_SLAVE_SUC_FLAG,    &route_slave_suc_flag,    sizeof(route_slave_suc_flag));
  zpal_nvm_read(pFileSystem, FILE_ID_SUC_PENDING_UPDATE_FLAG, &suc_pending_update_flag, sizeof(suc_pending_update_flag));
#ifdef ZW_CONTROLLER_BRIDGE
  zpal_nvm_read(pFileSystem, FILE_ID_BRIDGE_NODE_FLAG,        &bridge_node_flag,        sizeof(bridge_node_flag));
#endif
  zpal_nvm_read(pFileSystem, FILE_ID_PENDING_DISCOVERY_FLAG,  &pending_discovery_flag,  sizeof(pending_discovery_flag));

  memset(capabilities_speed_100k_nodes, 0, sizeof(capabilities_speed_100k_nodes));
  uint8_t nodeInfoFileBuffer[FILE_SIZE_NODEINFO];
  memset(nodeInfoFileBuffer, 0, sizeof(nodeInfoFileBuffer));

  //Loop over all NODEINFO files to fill NodeInfo RAM caches
  for (uint32_t fileNr = 0; fileNr < (ZW_MAX_NODES / NODEINFOS_PER_FILE); fileNr++)
  {
    //nodeID of lowest node in file.
    //Please note that the range 1-232 is used. No 0 node.
    uint8_t nID = (fileNr * NODEINFOS_PER_FILE) + 1;

    bool fileIsRead = false;
    for (uint8_t i = 0; i < NODEINFOS_PER_FILE; i++)
    {
      if (CtrlStorageCacheNodeExist(nID + i))
      {
        if(!fileIsRead)
        {
          zpal_nvm_read(pFileSystem, FILE_ID_NODEINFO_BASE + fileNr, nodeInfoFileBuffer, FILE_SIZE_NODEINFO);
          fileIsRead = true;
        }

        EX_NVM_NODEINFO * NodeInfo = (EX_NVM_NODEINFO *)&nodeInfoFileBuffer[sizeof(SNodeInfoStorage) * i];

        //Set RAM caches
        CtrlStorageCacheCapabilitiesSpeed100kNodeSet(nID + i, NodeInfo->reserved & ZWAVE_NODEINFO_BAUD_100K);
      }
    }
  }
}


void
CtrlStorageInit(void)
{
  pFileSystem = NvmFileSystemRegister(&FileSystemFormattedCb);

  // In case the file-system on is older than supported by this version of the FW, then upgrade.
  FileSystemMigrationManagement();

  SObjectSet FileSet = {
    .pFileSystem = pFileSystem,
    .iObjectCount = sizeof_array(m_aFileDescriptors),
    .pObjectDescriptors = m_aFileDescriptors
  };
  ECaretakerStatus eFileSetVerifyStatus = NVMCaretakerVerifySet(&FileSet, m_aFileStatus);
  Ctrl_storage_init(&FileSet, (ECTKR_STATUS_SUCCESS != eFileSetVerifyStatus ? true: false));
}

/**
* Reads file and copies content to first part of dataBuf
* Zero pads the rest of the file.
*
* @param[in] fileID, zpal_nvm_object_key_t for file to be read
* @param[out] dataBuf, buffer for writing file content
* @param[in] bufLength, length of buffer
* @param[in] copyLength, read copyLength bytes from file
*
*/
void ZeroPadFile(zpal_nvm_object_key_t fileID, void * dataBuf, size_t bufLength, uint32_t copyLength)
{
  if(copyLength && (copyLength <= bufLength))
  {
    zpal_nvm_read(pFileSystem, fileID, dataBuf, bufLength);
    memset((uint8_t *)dataBuf + copyLength, 0x00, bufLength - copyLength);
  }
  else
  {
    memset(dataBuf, 0x00, bufLength);
  }
}

/**
* Get the nodes IDs for the nodes included in the network
*
* @param[out] node_id_list a bitmask list of the nodes included in the network
*
*/
void GetIncludedNodes(NODE_MASK_TYPE node_id_list)
{
  memcpy(node_id_list, node_info_exists, sizeof(NODE_MASK_TYPE));
}


/**
* Get the nodes IDs for the long range nodes included in the network
*
* @param[out] node_id_list a bitmask list of the long range nodes included in the network
*
*/
void GetIncludedLrNodes(LR_NODE_MASK_TYPE node_id_list)
{
  memcpy(node_id_list, node_info_Lrange_exists, sizeof(LR_NODE_MASK_TYPE));
}

/**
* Writes individual slave files with correct size.
*
* @param[in] fileID, zpal_nvm_object_key_t for file to be written
* @param[in] copyLength, 0-> write only zero padding, else append zeros after copyLength bytes from present file
*
* @return true or false
*/
static bool
SetFile(zpal_nvm_object_key_t fileID, uint32_t copyLength)
{
  switch (fileID)
  {
#ifndef NO_PREFERRED_CALC
    case FILE_ID_PREFERREDREPEATERS:
    {
      //Write default Preferred Repeaters file
      SPreferredRepeaters tPreferredRepeaters;
      size_t structSize = sizeof(SPreferredRepeaters);
      ZeroPadFile(FILE_ID_PREFERREDREPEATERS, &tPreferredRepeaters, structSize, copyLength);
      return ZPAL_STATUS_OK == zpal_nvm_write(pFileSystem, FILE_ID_PREFERREDREPEATERS, &tPreferredRepeaters, structSize);
    }
#endif  /*NO_PREFERRED_CALC*/
    case FILE_ID_CONTROLLERINFO:
    {
      //Write default Controller Info file
      SControllerInfo tControllerInfo = {
                                         .HomeID = {0,0,0,0},
                                         .NodeID = 0x0001,
                                         .StaticControllerNodeId=0,
                                         .LastUsedNodeId_LR = LOWEST_LONG_RANGE_NODE_ID - 1,
                                         .LastUsedNodeId = 0x01,
                                         .SucLastIndex = DefaultControllerInfoSucLastIndex,
                                         .MaxNodeId_LR = 0, // 0 to indicate no LR node IDs have been assigned yet
                                         .MaxNodeId = 0x01,
                                         .ControllerConfiguration = DefaultControllerInfoControllerConfiguration,
                                         .ReservedId_LR = 0,
                                         .ReservedId = 0,
                                         .SystemState = 0,
                                         .PrimaryLongRangeChannelId = 0,
                                         .LonRangeChannelAutoMode = 0

      };
      size_t structSize = sizeof(SControllerInfo);
      if(copyLength && (copyLength <= structSize))
      {
        zpal_nvm_read(pFileSystem, FILE_ID_CONTROLLERINFO, &tControllerInfo, structSize);
      }
      return ZPAL_STATUS_OK == zpal_nvm_write(pFileSystem, FILE_ID_CONTROLLERINFO, &tControllerInfo, structSize);
    }
    case FILE_ID_NODE_ROUTECACHE_EXIST:
    {
      ZeroPadFile(FILE_ID_NODE_ROUTECACHE_EXIST, &node_routecache_exists, sizeof(node_routecache_exists), copyLength);
      return ZPAL_STATUS_OK == zpal_nvm_write(pFileSystem, FILE_ID_NODE_ROUTECACHE_EXIST, &node_routecache_exists, sizeof(node_routecache_exists));
    }
    case FILE_ID_NODE_STORAGE_EXIST:
    {
      ZeroPadFile(FILE_ID_NODE_STORAGE_EXIST, &node_info_exists, sizeof(node_info_exists), copyLength);
      return ZPAL_STATUS_OK == zpal_nvm_write(pFileSystem, FILE_ID_NODE_STORAGE_EXIST, &node_info_exists, sizeof(node_info_exists));
    }
    case FILE_ID_LRANGE_NODE_EXIST:
    {
      ZeroPadFile(FILE_ID_LRANGE_NODE_EXIST, &node_info_Lrange_exists, sizeof(node_info_Lrange_exists), copyLength);
      return ZPAL_STATUS_OK == zpal_nvm_write(pFileSystem, FILE_ID_LRANGE_NODE_EXIST, &node_info_Lrange_exists, sizeof(node_info_Lrange_exists));
    }
    case FILE_ID_APP_ROUTE_LOCK_FLAG:
    {
      ZeroPadFile(FILE_ID_APP_ROUTE_LOCK_FLAG, &app_route_lock_flag, sizeof(app_route_lock_flag), copyLength);
      return ZPAL_STATUS_OK == zpal_nvm_write(pFileSystem, FILE_ID_APP_ROUTE_LOCK_FLAG, &app_route_lock_flag, sizeof(app_route_lock_flag));
    }
    case FILE_ID_ROUTE_SLAVE_SUC_FLAG:
    {
      ZeroPadFile(FILE_ID_ROUTE_SLAVE_SUC_FLAG, &route_slave_suc_flag, sizeof(route_slave_suc_flag), copyLength);
      return ZPAL_STATUS_OK == zpal_nvm_write(pFileSystem, FILE_ID_ROUTE_SLAVE_SUC_FLAG, &route_slave_suc_flag, sizeof(route_slave_suc_flag));
    }
    case FILE_ID_SUC_PENDING_UPDATE_FLAG:
    {
      ZeroPadFile(FILE_ID_SUC_PENDING_UPDATE_FLAG, &suc_pending_update_flag, sizeof(suc_pending_update_flag), copyLength);
      return ZPAL_STATUS_OK == zpal_nvm_write(pFileSystem, FILE_ID_SUC_PENDING_UPDATE_FLAG, &suc_pending_update_flag, sizeof(suc_pending_update_flag));
    }
#ifdef ZW_CONTROLLER_BRIDGE
    case FILE_ID_BRIDGE_NODE_FLAG:
    {
      ZeroPadFile(FILE_ID_BRIDGE_NODE_FLAG, &bridge_node_flag, sizeof(bridge_node_flag), copyLength);
      return ZPAL_STATUS_OK == zpal_nvm_write(pFileSystem, FILE_ID_BRIDGE_NODE_FLAG, &bridge_node_flag, sizeof(bridge_node_flag));
    }
#endif
    case FILE_ID_PENDING_DISCOVERY_FLAG:
    {
      ZeroPadFile(FILE_ID_PENDING_DISCOVERY_FLAG, &pending_discovery_flag, sizeof(pending_discovery_flag), copyLength);
      return ZPAL_STATUS_OK == zpal_nvm_write(pFileSystem, FILE_ID_PENDING_DISCOVERY_FLAG, &pending_discovery_flag, sizeof(pending_discovery_flag));
    }
    default:
      return false;
  }
}


/**
* Write default Controller storage files with default content.
*
* Used both as File system formatted callback from NVM module,
* and by CtrlStorageFileSystemInit if FS has been formatted.
*
* Clears FileHandles (needed when used as callback that FS is formatted).
* Writes required files to FS with default values.
*/
static void
WriteDefaultSetofFiles(void)
{
  bool set_file_ok = true;
#ifndef NO_PREFERRED_CALC
  //Write default Preferred Repeaters file
  set_file_ok = set_file_ok && SetFile(FILE_ID_PREFERREDREPEATERS, 0);
#endif  /*NO_PREFERRED_CALC*/

  //Write default Controller Info file
  set_file_ok = set_file_ok && SetFile(FILE_ID_CONTROLLERINFO, 0);

  //Init node_info_exists
  set_file_ok = set_file_ok && SetFile(FILE_ID_NODE_STORAGE_EXIST, 0);

  //Init node_routecache exist
  set_file_ok = set_file_ok && SetFile(FILE_ID_NODE_ROUTECACHE_EXIST, 0);

  //Init node_info_exists
  set_file_ok = set_file_ok && SetFile(FILE_ID_LRANGE_NODE_EXIST, 0);

  //Init app_route_lock_flag
  set_file_ok = set_file_ok && SetFile(FILE_ID_APP_ROUTE_LOCK_FLAG, 0);

  //Init route_slave_suc_flag
  set_file_ok = set_file_ok && SetFile(FILE_ID_ROUTE_SLAVE_SUC_FLAG, 0);

  //Init suc_pending_update_flag
  set_file_ok = set_file_ok && SetFile(FILE_ID_SUC_PENDING_UPDATE_FLAG, 0);

#ifdef ZW_CONTROLLER_BRIDGE
  //Init bridge_node_flag
  set_file_ok = set_file_ok && SetFile(FILE_ID_BRIDGE_NODE_FLAG, 0);
#endif

  //Init pending_discovery_flag
  set_file_ok = set_file_ok && SetFile(FILE_ID_PENDING_DISCOVERY_FLAG, 0);

  //Assert files were written
  ASSERT(set_file_ok);

  //Write filesystem version
  CtrlStorageSetZWVersion(&ZW_Version);

  DPRINT("Controller storage Reset to default\r\n");
}
