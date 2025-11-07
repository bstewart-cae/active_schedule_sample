// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/****************************************************************************
 *
 * @copyright 2018 Silicon Laboratories Inc.
 *
 *---------------------------------------------------------------------------
 *
 * Description: This module provide an interface to handle writing , reading,
 *              deleting, creating, converting and testing Slave related
 *              network data to non-volatile media.
 *
 *
 ****************************************************************************/

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include "ZW_slave_network_info_storage.h"

#include <stdbool.h>
#include <string.h>
#include <stddef.h>
#include "SizeOf.h"

//#define DEBUGPRINT  // NOSONAR
#include <DebugPrint.h>

#include <ZW_node.h>
#include "ZW_protocol.h"
#include <Assert.h>
#include <SyncEvent.h>
#include <ZW_NVMCaretaker.h>
#include <ZW_nvm.h>
#include <NodeMask.h>
#include <ZW_Security_Scheme2.h>
#include <zpal_nvm.h>

typedef struct SReturnRouteDestinationsCached
{
  uint8_t  ReturnRouteDestinationsCached[ZW_MAX_CACHED_RETURN_ROUTE_DESTINATIONS];
}SReturnRouteDestinationsCached;

typedef struct SReturnRouteInfo
{
  NVM_RETURN_ROUTE_STRUCT  ReturnRoute;
  NVM_RETURN_ROUTE_SPEED   ReturnRouteSpeed;
} SReturnRouteInfo;


typedef struct SSlaveInfo
{
  uint8_t   homeID[HOMEID_LENGTH];
  node_id_t nodeID;
  uint8_t   smartStartState;
  uint8_t   primaryLongRangeChannelId;
} SSlaveInfo;

typedef struct SRouteInfoFileMap
{
  NODE_MASK_TYPE nodeInfoExist;
  uint8_t sucNodeInfoExist;
}SRouteInfoFileMap;

static bool SetFile(zpal_nvm_object_key_t fileID, uint32_t copyLength);
static void WriteDefaultSetofFiles(void);

static SSlaveInfo end_device_info_cache __attribute__((section(".ret_sram"))) __attribute__((used));
static bool end_device_info_cached __attribute__((section(".ret_sram"))) __attribute__((used));

//Do not change the value of FILE_ID_ZW_VERSION
#define FILE_ID_ZW_VERSION                 (0x00000)
#define FILE_SIZE_ZW_VERSION               (sizeof(uint32_t))

#define FILE_ID_RETURNROUTESDESTINATIONS   (0x00002)
#define FILE_SIZE_RETURNROUTESDESTINATIONS (sizeof(SReturnRouteDestinationsCached))

#define FILE_ID_SLAVEINFO                  (0x00003)
#define FILE_SIZE_SLAVEINFO                (sizeof(SSlaveInfo))

#define FILE_ID_SLAVE_FILE_MAP             (0x00004)
#define FILE_SIZE_SLAVE_FILE_MAP           (sizeof(SRouteInfoFileMap))

//S2 keys files
#define FILE_ID_S2_KEYS                   (0x00010)
#define FILE_SIZE_S2_KEYS                 (sizeof(Ss2_keys))

#define FILE_ID_S2_KEYCLASSES_ASSIGNED    (0x00011)
#define FILE_SIZE_S2_KEYCLASSES_ASSIGNED  (sizeof(Ss2_keyclassesAssigned))

#define FILE_ID_S2_MPAN                   (0x00012)  //Deprecated, used for file migration.
#define FILE_ID_S2_SPAN                   (0x00013)  //Deprecated, used for file migration.

#define FILE_ID_ZW_PRIVATE_KEY            (0x00014)
#define FILE_SIZE_ZW_PRIVATE_KEY          (32)

#define FILE_ID_ZW_PUBLIC_KEY            (0x00015)
#define FILE_SIZE_ZW_PUBLIC_KEY          (32)

// File_ID ranges ////////////////////////////////////////////////

#define RETURNROUTEINFOS_PER_FILE         4
#define FILE_ID_RETURNROUTEINFO_BASE      (0x00200)
#define FILE_SIZE_RETURNROUTEINFO         (sizeof(SReturnRouteInfo) * RETURNROUTEINFOS_PER_FILE)
#define FILE_ID_RETURNROUTEINFO_LAST      (FILE_ID_S2_SPAN_BASE - 1)

#define FILE_ID_S2_SPAN_BASE              (0x00400)
#define FILE_SIZE_S2_SPAN                 (sizeof(struct SPAN))
#define FILE_ID_S2_SPAN_LAST              (FILE_ID_S2_MPAN_BASE - 1)

#define FILE_ID_S2_MPAN_BASE              (0x00500)
#define FILE_SIZE_S2_MPAN                 (sizeof(struct MPAN))
#define FILE_ID_S2_MPAN_LAST              (FILE_ID__NEXT - 1)

#define FILE_ID__NEXT                     (0x00600)  //Next free FILE_ID_BASE



static SRouteInfoFileMap SlaveFileMap = { 0 };

static zpal_nvm_handle_t pFileSystem;
static const SSyncEvent FileSystemFormattedCb = {
                                                .uFunctor.pFunction = WriteDefaultSetofFiles,
                                                .pObject = 0
                                               };


static const SObjectDescriptor g_aFileDescriptors[] = {
  { .ObjectKey = FILE_ID_RETURNROUTESDESTINATIONS,  .iDataSize = FILE_SIZE_RETURNROUTESDESTINATIONS },
  { .ObjectKey = FILE_ID_SLAVEINFO,                 .iDataSize = FILE_SIZE_SLAVEINFO                },
  { .ObjectKey = FILE_ID_SLAVE_FILE_MAP,            .iDataSize = FILE_SIZE_SLAVE_FILE_MAP           },
  { .ObjectKey = FILE_ID_S2_KEYS,                   .iDataSize = FILE_SIZE_S2_KEYS                  },
  { .ObjectKey = FILE_ID_S2_KEYCLASSES_ASSIGNED,    .iDataSize = FILE_SIZE_S2_KEYCLASSES_ASSIGNED   },
  { .ObjectKey = FILE_ID_ZW_PRIVATE_KEY,            .iDataSize = FILE_SIZE_ZW_PRIVATE_KEY           },
  { .ObjectKey = FILE_ID_ZW_PUBLIC_KEY,            .iDataSize = FILE_SIZE_ZW_PUBLIC_KEY           },
};

static ECaretakerStatus g_aFileStatus[sizeof_array(g_aFileDescriptors)];

//RAM buffer that can hold the data from one RETURNROUTEINFO NVM file.
//The buffer thus have place for several SReturnRouteInfo structs.
static uint8_t returnRouteInfoBuffer[FILE_SIZE_RETURNROUTEINFO];

static const uint32_t ZW_Version = (ZW_SLAVE_FILESYS_VERSION << 24) | (ZW_VERSION_MAJOR << 16) | (ZW_VERSION_MINOR << 8) | (ZW_VERSION_PATCH);

/****************************************************************************/
/*                            PRIVATE FUNCTIONS                             */
/****************************************************************************/

static
bool ReturnRouteExists(uint16_t index)
{
  if (MAX_RETURN_ROUTES_MAX_ENTRIES <= index)
  {
    return false;
  }
  else if (0 < index)
  {
    return ZW_NodeMaskNodeIn(SlaveFileMap.nodeInfoExist, index);
  }
  else
  {
    return SlaveFileMap.sucNodeInfoExist;
  }
}

static void DeleteStorageCaches(void)
{
  uint8_t *p_end_device_info_cache = (uint8_t *)&end_device_info_cache;
  memset(p_end_device_info_cache, 0, sizeof(end_device_info_cache));
  end_device_info_cached = false;
}

//Function that updates the global buffer returnRouteInfoBuffer[] by reading in data stored in non volatile memory.
//If data corresponding to destRouteIndex is not already in the current returnRouteInfoBuffer[] the buffer is overwritten
//by data from the corresponding NVM file.
bool updateReturnRouteInfoBuffer(uint8_t destRouteIndex)
{
  static uint8_t lastReturnRouteIndex = 250; //Initiate with dummy value

  //Check if there is a NVM file or not
  bool foundFile = false;
  uint8_t tNodeID = destRouteIndex - (destRouteIndex % RETURNROUTEINFOS_PER_FILE);
  for(uint32_t i=0; i<RETURNROUTEINFOS_PER_FILE; i++)
  {
    if( MAX_RETURN_ROUTES_MAX_ENTRIES <= (tNodeID + i) )
    {
      break;
    }
    else if(ReturnRouteExists(tNodeID + i))
    {
      foundFile = true;
      break;
    }
  }

  //Several SReturnRouteInfo structs are saved in each RETURNROUTEINFO file in NVM.
  //Update returnRouteInfoBuffer[] if its old entries do not comprise the current destRouteIndex.
  if((destRouteIndex / RETURNROUTEINFOS_PER_FILE) != (lastReturnRouteIndex / RETURNROUTEINFOS_PER_FILE))
  {
    if (foundFile)
    {
      //Update returnRouteInfoBuffer
      zpal_nvm_object_key_t tFileID = FILE_ID_RETURNROUTEINFO_BASE + (destRouteIndex / RETURNROUTEINFOS_PER_FILE);
      zpal_nvm_read(pFileSystem, tFileID, returnRouteInfoBuffer, FILE_SIZE_RETURNROUTEINFO);
    }
    else
    {
      //Set returnRouteInfoBuffer to default
      memset(returnRouteInfoBuffer, 0xFF, FILE_SIZE_RETURNROUTEINFO);
    }
    lastReturnRouteIndex = destRouteIndex;
  }

  return foundFile;
}

bool SlaveStorageGetReturnRouteInfo(node_id_t destRouteIndex , uint8_t * pReturnRouteData, SELECT_RETURNROUTEINFO outputSelector)
{
  if(!ReturnRouteExists(destRouteIndex))
  {
    return false;
  }

  updateReturnRouteInfoBuffer(destRouteIndex);

  SReturnRouteInfo * pSourceRouteInfo;
  pSourceRouteInfo = (SReturnRouteInfo *)&returnRouteInfoBuffer[(destRouteIndex % RETURNROUTEINFOS_PER_FILE) * sizeof(SReturnRouteInfo)];

  switch (outputSelector)
  {
    case GET_RETURNROUTE:
    {
      const NVM_RETURN_ROUTE_STRUCT * pSourceReturnRoute = &(pSourceRouteInfo->ReturnRoute);
      memcpy(
         pReturnRouteData,
         pSourceReturnRoute,
         sizeof(NVM_RETURN_ROUTE_STRUCT)
      );
      break;
    }
    case GET_RETURNROUTE_SPEED:
    {
      const NVM_RETURN_ROUTE_SPEED * pSrcReturnRouteSpeed = &(pSourceRouteInfo->ReturnRouteSpeed);
      memcpy(
         pReturnRouteData,
         pSrcReturnRouteSpeed,
         sizeof(NVM_RETURN_ROUTE_SPEED)
      );
      break;
    }
    case GET_RETURNROUTE_INFO:
    {
      memcpy(
         pReturnRouteData,
         pSourceRouteInfo,
         sizeof(SReturnRouteInfo)
      );
      break;
    }
    default:
      return false;
  }

  return true;
}

void SlaveStorageGetReturnRoute(node_id_t destRouteIndex , NVM_RETURN_ROUTE_STRUCT* pReturnRoute)
{
  if(!SlaveStorageGetReturnRouteInfo(destRouteIndex , (uint8_t *)pReturnRoute, GET_RETURNROUTE))
  {
    //SlaveStorageGetReturnRouteInfo() returned false so fill output with zeros
    memset(
      pReturnRoute,
      0,
      sizeof(NVM_RETURN_ROUTE_STRUCT)
    );
  }
}

void SlaveStorageGetReturnRouteSpeed(node_id_t destRouteIndex , NVM_RETURN_ROUTE_SPEED* pReturnRouteSpeed)
{
  if(!SlaveStorageGetReturnRouteInfo(destRouteIndex , (uint8_t *)pReturnRouteSpeed, GET_RETURNROUTE_SPEED))
  {
    //SlaveStorageGetReturnRouteInfo() returned false so fill output with zeros
    memset(
      pReturnRouteSpeed,
      0,
      sizeof(NVM_RETURN_ROUTE_SPEED)
    );
  }
}

void SlaveStorageSetReturnRoute(node_id_t destRouteIndex , const NVM_RETURN_ROUTE_STRUCT* pReturnRoute, const NVM_RETURN_ROUTE_SPEED* pReturnRouteSpeed)
{
  if(MAX_RETURN_ROUTES_MAX_ENTRIES <= destRouteIndex)
  {
    return;
  }

  updateReturnRouteInfoBuffer(destRouteIndex);

  SReturnRouteInfo * pReturnRouteInfo = (SReturnRouteInfo *)&returnRouteInfoBuffer[(destRouteIndex % RETURNROUTEINFOS_PER_FILE) * sizeof(SReturnRouteInfo)];

  memcpy(&(pReturnRouteInfo->ReturnRoute), pReturnRoute, sizeof(NVM_RETURN_ROUTE_STRUCT));
  memcpy(&(pReturnRouteInfo->ReturnRouteSpeed), pReturnRouteSpeed, sizeof(NVM_RETURN_ROUTE_SPEED));

  zpal_nvm_object_key_t tFileID = FILE_ID_RETURNROUTEINFO_BASE + (destRouteIndex / RETURNROUTEINFOS_PER_FILE);
  zpal_nvm_write(pFileSystem, tFileID, returnRouteInfoBuffer, FILE_SIZE_RETURNROUTEINFO);

  //Update SlaveFileMap
  if (!ReturnRouteExists(destRouteIndex))
  {
    if (0 < destRouteIndex)
    {
      ZW_NodeMaskSetBit(SlaveFileMap.nodeInfoExist, destRouteIndex);
    }
    else
    {
      SlaveFileMap.sucNodeInfoExist = 0x01;
    }
    zpal_nvm_write(pFileSystem, FILE_ID_SLAVE_FILE_MAP, &SlaveFileMap, sizeof(SlaveFileMap));
  }
}


void SlaveStorageSetReturnRouteSpeed(node_id_t destRouteIndex , const NVM_RETURN_ROUTE_SPEED* pReturnRouteSpeed)
{
  if(MAX_RETURN_ROUTES_MAX_ENTRIES <= destRouteIndex)
  {
    return;
  }

  if (ReturnRouteExists(destRouteIndex))
  {
    updateReturnRouteInfoBuffer(destRouteIndex);

    SReturnRouteInfo * pReturnRouteInfo = (SReturnRouteInfo *)&returnRouteInfoBuffer[(destRouteIndex % RETURNROUTEINFOS_PER_FILE) * sizeof(SReturnRouteInfo)];
    memcpy(&(pReturnRouteInfo->ReturnRouteSpeed), pReturnRouteSpeed, sizeof(NVM_RETURN_ROUTE_SPEED));

    zpal_nvm_object_key_t tFileID = FILE_ID_RETURNROUTEINFO_BASE + (destRouteIndex / RETURNROUTEINFOS_PER_FILE);
    zpal_nvm_write(pFileSystem, tFileID, returnRouteInfoBuffer, FILE_SIZE_RETURNROUTEINFO);
  }
  else
  {
    DPRINTF("return route  %d don't exist \r\n",destRouteIndex);
  }
}


void SlaveStorageDeleteAllReturnRouteInfo(void)
{
  int j;
  uint32_t fileNr;
  uint32_t highestFileNr = MAX_RETURN_ROUTES_MAX_ENTRIES / RETURNROUTEINFOS_PER_FILE;

  //Check if there is any ReturnRouteInfo to delete
  for (j = sizeof(NODE_MASK_TYPE) - 1; j >= 0; j--)
  {
    if (0 != SlaveFileMap.nodeInfoExist[j])
    {
      highestFileNr = (j + 1) * (NODEMASK_NODES_PER_BYTE / RETURNROUTEINFOS_PER_FILE);
      break;
    }
  }
  if (0 > j)
  {
    //No ReturnRouteInfo was found
    return;
  }

  //Special case for the lowest file since it contains SUC info
  if (!ReturnRouteExists(0)) //Don't remove lowest file if it contains SUC info
  {
    for(j = 1; j < RETURNROUTEINFOS_PER_FILE; j++)
    {
      if (ReturnRouteExists(j))
      {
        //Delete lowest file
        zpal_nvm_erase_object(pFileSystem, FILE_ID_RETURNROUTEINFO_BASE);
        break;
      }
    }
  }

  //Delete all higher files. fileNr 1..58
  for (fileNr = 1; fileNr < highestFileNr; fileNr++)
  {
    for (j = 0; j < RETURNROUTEINFOS_PER_FILE; j++)
    {
      if (ReturnRouteExists((fileNr * RETURNROUTEINFOS_PER_FILE) + j))
      {
        //Delete file
        zpal_nvm_erase_object(pFileSystem, FILE_ID_RETURNROUTEINFO_BASE + fileNr);
        break;
      }
    }
  }

  memset(SlaveFileMap.nodeInfoExist, 0, sizeof(NODE_MASK_TYPE));
  zpal_nvm_write(pFileSystem, FILE_ID_SLAVE_FILE_MAP ,&SlaveFileMap, sizeof(SlaveFileMap));
}


void SlaveStorageDeleteSucReturnRouteInfo(void)
{
  uint32_t j;

  //Remove lowest file if it only contains SUC info
  if(ReturnRouteExists(0))
  {
    //Mark SucReturnRouteInfo as erased in SlaveFileMap
    SlaveFileMap.sucNodeInfoExist = 0x00;
    zpal_nvm_write(pFileSystem, FILE_ID_SLAVE_FILE_MAP ,&SlaveFileMap, sizeof(SlaveFileMap));

    //Don't delete RETURNROUTEINFO file if it contains other info
    for(j = 1; j < RETURNROUTEINFOS_PER_FILE; j++)
    {
      if(ReturnRouteExists(j))
      {
        return;
      }
    }

    //Delete lowest RETURNROUTEINFO file
    zpal_nvm_erase_object(pFileSystem, FILE_ID_RETURNROUTEINFO_BASE);
  }
}


void SlaveStorageGetRouteDestinations(uint8_t * pRouteDestinations)
{
  zpal_nvm_read(pFileSystem, FILE_ID_RETURNROUTESDESTINATIONS, pRouteDestinations, FILE_SIZE_RETURNROUTESDESTINATIONS);
}


void SlaveStorageSetRouteDestinations(const uint8_t * pRouteDestinations)
{
  zpal_nvm_write(pFileSystem, FILE_ID_RETURNROUTESDESTINATIONS, pRouteDestinations, FILE_SIZE_RETURNROUTESDESTINATIONS);
}

void helper_update_end_device_info_cache(void)
{
  if (!end_device_info_cached)
  {
    zpal_nvm_read(pFileSystem, FILE_ID_SLAVEINFO, &end_device_info_cache, FILE_SIZE_SLAVEINFO);
    end_device_info_cached = true;
  }
}

void SlaveStorageSetNetworkIds(const uint8_t *pHomeID, node_id_t NodeID)
{
  helper_update_end_device_info_cache();

  if (NULL != pHomeID)
  {
    memcpy(end_device_info_cache.homeID, pHomeID, HOMEID_LENGTH);
  }
  end_device_info_cache.nodeID = NodeID;

  const zpal_status_t status = zpal_nvm_write(pFileSystem, FILE_ID_SLAVEINFO, &end_device_info_cache, FILE_SIZE_SLAVEINFO);
  if ( ZPAL_STATUS_OK == status ) {
    ZW_HomeIDSet(end_device_info_cache.homeID);  // Set the homeID read-cache.
  }
}

void SlaveStorageSetSmartStartState(uint8_t state)
{
  helper_update_end_device_info_cache();

  end_device_info_cache.smartStartState = state;
  zpal_nvm_write(pFileSystem, FILE_ID_SLAVEINFO, &end_device_info_cache, FILE_SIZE_SLAVEINFO);
}

void StorageSetPrimaryLongRangeChannelId(zpal_radio_lr_channel_t channelId)
{
  helper_update_end_device_info_cache();

  end_device_info_cache.primaryLongRangeChannelId = (uint8_t) channelId;
  zpal_nvm_write(pFileSystem, FILE_ID_SLAVEINFO, &end_device_info_cache, FILE_SIZE_SLAVEINFO);
}

void SlaveStorageGetNetworkIds(uint8_t *pHomeID, node_id_t *pNodeID)
{
  zpal_status_t status = ZPAL_STATUS_OK;
  helper_update_end_device_info_cache();

  // Check the pHomeID pointer to avoid setting the homeID read cache twice.
  if (status == ZPAL_STATUS_OK && pHomeID != ZW_HomeIDGet()) {
    ZW_HomeIDSet(end_device_info_cache.homeID);  // Set the homeID read-cache.
  }

  if (NULL != pHomeID)
  {
    memcpy(pHomeID, &end_device_info_cache.homeID, HOMEID_LENGTH);  // Returning the stored homeID
  }

  if (NULL != pNodeID)
  {
    *pNodeID = end_device_info_cache.nodeID;
  }
}

void StorageSetNodeId(node_id_t NodeID)
{
  helper_update_end_device_info_cache();

  end_device_info_cache.nodeID = NodeID;
  zpal_nvm_write(pFileSystem, FILE_ID_SLAVEINFO, &end_device_info_cache, FILE_SIZE_SLAVEINFO);
}

void StorageGetNodeId(node_id_t *pNodeID)
{
  helper_update_end_device_info_cache();

  if (NULL != pNodeID)
  {
    *pNodeID = end_device_info_cache.nodeID;
  }
}

uint8_t SlaveStorageGetSmartStartState(void)
{
  helper_update_end_device_info_cache();

  return end_device_info_cache.smartStartState;
}

zpal_radio_lr_channel_t StorageGetPrimaryLongRangeChannelId(void)
{
  helper_update_end_device_info_cache();

  return (zpal_radio_lr_channel_t) end_device_info_cache.primaryLongRangeChannelId;
}

bool SlaveStorageGetZWVersion(uint32_t * version)
{
  if( ZPAL_STATUS_OK == zpal_nvm_read(pFileSystem, FILE_ID_ZW_VERSION, version, sizeof(uint32_t)) )
  {
    return true;
  }

  return false;
}

bool SlaveStorageSetZWVersion(const uint32_t * version)
{
  if( ZPAL_STATUS_OK == zpal_nvm_write(pFileSystem, FILE_ID_ZW_VERSION, version, sizeof(uint32_t)) )
  {
    return true;
  }

  return false;
}

bool StorageGetS2Keys(Ss2_keys * keys)
{
  if( ZPAL_STATUS_OK != zpal_nvm_read(pFileSystem, FILE_ID_S2_KEYS, keys, FILE_SIZE_S2_KEYS) )
  {
    return false;
  }

  return true;
}

bool StorageSetS2Keys(Ss2_keys * keys)
{
  if( ZPAL_STATUS_OK != zpal_nvm_write(pFileSystem, FILE_ID_S2_KEYS, keys, FILE_SIZE_S2_KEYS) )
  {
    return false;
  }
  return true;
}

bool StorageGetS2KeyClassesAssigned(Ss2_keyclassesAssigned * assigned)
{
  if( ZPAL_STATUS_OK != zpal_nvm_read(pFileSystem, FILE_ID_S2_KEYCLASSES_ASSIGNED, assigned, FILE_SIZE_S2_KEYCLASSES_ASSIGNED) )
  {
    return false;
  }

  return true;
}

bool StorageSetS2KeyClassesAssigned(Ss2_keyclassesAssigned * assigned)
{
  if( ZPAL_STATUS_OK != zpal_nvm_write(pFileSystem, FILE_ID_S2_KEYCLASSES_ASSIGNED, assigned, FILE_SIZE_S2_KEYCLASSES_ASSIGNED) )
  {
    return false;
  }
  return true;
}

ZW_WEAK bool StorageGetS2MpanTable(void * mpan_table)
{
  //Set all bytes in mpan to zero.
  memset(mpan_table, 0, MPAN_TABLE_SIZE * FILE_SIZE_S2_MPAN);

  //Get list of existing mpan files.
  zpal_nvm_object_key_t mpan_objectKeys[MPAN_TABLE_SIZE] = {0};
  size_t nrOfFiles = zpal_nvm_enum_objects(pFileSystem, mpan_objectKeys, MPAN_TABLE_SIZE, FILE_ID_S2_MPAN_BASE, FILE_ID_S2_MPAN_BASE + MPAN_TABLE_SIZE - 1);

  bool retVal = true;
  //Read existing files to mpan.
  for (size_t i=0; i < nrOfFiles; i++)
  {
	uint32_t tableIndex = mpan_objectKeys[i] - FILE_ID_S2_MPAN_BASE;
    if ( ZPAL_STATUS_OK != zpal_nvm_read(pFileSystem, mpan_objectKeys[i], (struct MPAN *)mpan_table + tableIndex, FILE_SIZE_S2_MPAN) )
    {
      retVal = false;
    }
  }

  return retVal;
}

ZW_WEAK bool StorageWriteS2Mpan(uint32_t id, void * mpan)
{
  if (MPAN_TABLE_SIZE <= id)
  {
    return false;
  }

  zpal_status_t retVal = ZPAL_STATUS_OK;

  const struct MPAN * pMpan = (struct MPAN *)mpan;
  if (MPAN_NOT_USED != pMpan->state)
  {
    retVal = zpal_nvm_write(pFileSystem, FILE_ID_S2_MPAN_BASE + id, mpan, FILE_SIZE_S2_MPAN);
  }
  return (ZPAL_STATUS_OK == retVal);
}

ZW_WEAK bool StorageSetS2MpanTable(void * mpan_table)
{
  //Get list of existing mpan files.
  zpal_nvm_object_key_t mpan_objectKeys[MPAN_TABLE_SIZE] = {0};
  size_t nrOfFiles = zpal_nvm_enum_objects(pFileSystem, mpan_objectKeys, MPAN_TABLE_SIZE, FILE_ID_S2_MPAN_BASE, FILE_ID_S2_MPAN_BASE + MPAN_TABLE_SIZE - 1);

  bool retVal = true;

  //Save files with valid content
  for (uint32_t i=0; i < MPAN_TABLE_SIZE; i++)
  {
    retVal &= StorageWriteS2Mpan(i, (struct MPAN *)mpan_table + i);
  }

  //Delete files without valid content
  for (size_t j=0; j < nrOfFiles; j++)
  {
	uint32_t i = mpan_objectKeys[j] - FILE_ID_S2_MPAN_BASE;
    if (MPAN_NOT_USED == ((struct MPAN *)mpan_table + i)->state)
    {
      zpal_nvm_erase_object(pFileSystem, mpan_objectKeys[j]);
    }
  }

  return retVal;
}

ZW_WEAK bool StorageGetS2SpanTable(void * span_table)
{
  memset(span_table, 0, SPAN_TABLE_SIZE * FILE_SIZE_S2_SPAN);

  //Get list of existing span files.
  zpal_nvm_object_key_t span_objectKeys[SPAN_TABLE_SIZE] = {0};
  size_t nrOfFiles = zpal_nvm_enum_objects(pFileSystem, span_objectKeys, SPAN_TABLE_SIZE, FILE_ID_S2_SPAN_BASE, FILE_ID_S2_SPAN_BASE + SPAN_TABLE_SIZE - 1);

  bool retVal = true;
  //Read existing files
  for (size_t i=0; i < nrOfFiles; i++)
  {
	uint32_t tableIndex = span_objectKeys[i] - FILE_ID_S2_SPAN_BASE;
    if ( ZPAL_STATUS_OK != zpal_nvm_read(pFileSystem, span_objectKeys[i], (struct SPAN *)span_table + tableIndex, FILE_SIZE_S2_SPAN) )
    {
      retVal = false;
    }
  }

  return retVal;
}

ZW_WEAK bool StorageWriteS2Span(uint32_t id, void * span)
{
  if (SPAN_TABLE_SIZE <= id)
  {
    return false;
  }

  zpal_status_t retVal = ZPAL_STATUS_OK;

  const struct SPAN * pSpan = (struct SPAN *)span;
  if (SPAN_NOT_USED != pSpan->state)
  {
    retVal = zpal_nvm_write(pFileSystem, FILE_ID_S2_SPAN_BASE + id, span, FILE_SIZE_S2_SPAN);
  }
  return (ZPAL_STATUS_OK == retVal);
}

ZW_WEAK bool StorageSetS2SpanTable(void * span_table)
{
  //Get list of existing span files.
  zpal_nvm_object_key_t span_objectKeys[SPAN_TABLE_SIZE] = {0};
  size_t nrOfFiles = zpal_nvm_enum_objects(pFileSystem, span_objectKeys, SPAN_TABLE_SIZE, FILE_ID_S2_SPAN_BASE, FILE_ID_S2_SPAN_BASE + SPAN_TABLE_SIZE - 1);

  bool retVal = true;

  //Save files with valid content
  for (uint32_t i=0; i < SPAN_TABLE_SIZE; i++)
  {
    retVal &= StorageWriteS2Span(i, (struct SPAN *)span_table + i);
  }

  //Delete files without valid content
  for (size_t j=0; j < nrOfFiles; j++)
  {
	uint32_t i = span_objectKeys[j] - FILE_ID_S2_SPAN_BASE;
    if (SPAN_NOT_USED == ((struct SPAN *)span_table + i)->state)
    {
      zpal_nvm_erase_object(pFileSystem, span_objectKeys[j]);
    }
  }

  return retVal;
}


bool StorageGetZWPrivateKey(uint8_t * key)
{
  return zpal_nvm_read(pFileSystem, FILE_ID_ZW_PRIVATE_KEY, key,
                        FILE_SIZE_ZW_PRIVATE_KEY) == ZPAL_STATUS_OK;
}

bool StorageSetZWPrivateKey(uint8_t * key)
{
  return zpal_nvm_write(pFileSystem, FILE_ID_ZW_PRIVATE_KEY, key,
                        FILE_SIZE_ZW_PRIVATE_KEY) == ZPAL_STATUS_OK;
}

bool StorageGetZWPublicKey(uint8_t * key)
{
  return zpal_nvm_read(pFileSystem, FILE_ID_ZW_PUBLIC_KEY, key,
                        FILE_SIZE_ZW_PUBLIC_KEY) == ZPAL_STATUS_OK;
}

bool StorageSetZWPublicKey(uint8_t * key)
{
  return zpal_nvm_write(pFileSystem, FILE_ID_ZW_PUBLIC_KEY, key,
                        FILE_SIZE_ZW_PUBLIC_KEY) == ZPAL_STATUS_OK;
}



void MergeNodeFiles(zpal_nvm_object_key_t originalBaseID,
                    zpal_nvm_object_key_t mergedBaseID,
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
      if(nodeID >= originalIdRange)
      {
        break;
      }
      else if (ReturnRouteExists(nodeID))
      {
        originalFileID = originalBaseID + nodeID;
        const zpal_status_t status = zpal_nvm_read(pFileSystem, originalFileID, pFileBuffer + (j * originalFileSize), originalFileSize);
        writeMergedFile = writeMergedFile || (ZPAL_STATUS_OK == status);
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
      if(nodeID >= originalIdRange)
      {
        break;
      }
      else if (ReturnRouteExists(nodeID))
      {
        originalFileID = originalBaseID + nodeID;
        zpal_nvm_erase_object(pFileSystem, originalFileID);
      }
    }
  }
}

void SlaveStorageInit(void)
{
  pFileSystem = NvmFileSystemRegister(&FileSystemFormattedCb);

  //Read present file system version file
  bool     versionFileExists;
  uint32_t presentVersion = 0;
  versionFileExists = SlaveStorageGetZWVersion(&presentVersion);

  if(!versionFileExists)
  {
    DeleteStorageCaches();
    //File system area is brand new or corrupt. Formate it.
    bool bFormatStatus = NvmFileSystemFormat();
    // FileSystemFormattedCb callback of NvmFileSystemRegister() is called after formatting.
    ASSERT(bFormatStatus);
    presentVersion = ZW_Version;
  }

  if(ZW_Version < presentVersion)
  {
    //File system downgrade. Should not be allowed. The bootloader is configured to prevent this.
    return;
  }

  if(ZW_Version > presentVersion)
  {
    DeleteStorageCaches();
    //Find ZW_SLAVE_FILESYS_VERSION of the present file system
    uint32_t presentFilesysVersion = (presentVersion >> 24) & 0x000000FF;

#ifdef ZWAVE_MIGRATE_FILESYSTEM

    //Migrate files from file system version 0 to 1.
    if ( 0 == presentFilesysVersion )
    {
      //Read SLAVE_FILE_MAP to buffer
      zpal_nvm_read(pFileSystem, FILE_ID_SLAVE_FILE_MAP, &SlaveFileMap, sizeof(SlaveFileMap));

      //merge RETURNROUTEINFO files
      uint32_t originalBaseID   = 0x00100;
      uint32_t originalIdRange  = 233;  //MAX_RETURN_ROUTES_MAX_ENTRIES
      uint32_t originalFileSize = 19;
      MergeNodeFiles(originalBaseID, FILE_ID_RETURNROUTEINFO_BASE, originalIdRange, RETURNROUTEINFOS_PER_FILE, originalFileSize, FILE_SIZE_RETURNROUTEINFO, returnRouteInfoBuffer);
    }

    //Remove unused RETURNROUTEINFO files.
    if ( 1 == presentFilesysVersion )
    {
      //Read SLAVE_FILE_MAP to buffer
      zpal_nvm_read(pFileSystem, FILE_ID_SLAVE_FILE_MAP, &SlaveFileMap, sizeof(SlaveFileMap));

      //Create a list of existing RETURNROUTEINFO files.
      const size_t maxNrOfFiles = 59; //ZW_MAX_NODES/RETURNROUTEINFOS_PER_FILE + 1
      zpal_nvm_object_key_t rri_objectKeys[59] = {0}; //List of possible return route info file object keys.
      size_t nrOfFiles = zpal_nvm_enum_objects(pFileSystem, rri_objectKeys, maxNrOfFiles, FILE_ID_RETURNROUTEINFO_BASE, FILE_ID_RETURNROUTEINFO_BASE + maxNrOfFiles - 1);

      //Remove files that can't be found in the SlaveFileMap
      for(uint32_t i=0; i < nrOfFiles; i++)
      {
        uint32_t nodeNr = (rri_objectKeys[i] - FILE_ID_RETURNROUTEINFO_BASE) * RETURNROUTEINFOS_PER_FILE;
        if(!ReturnRouteExists(nodeNr) && !ReturnRouteExists(nodeNr+1) && !ReturnRouteExists(nodeNr+2) && !ReturnRouteExists(nodeNr+3))
        {
          zpal_nvm_erase_object(pFileSystem, rri_objectKeys[i]);
        }
      }
    }

//There were 10 SPAN or MPAN structs in the each of the old files FILE_ID_S2_SPAN and FILE_ID_S2_MPAN
#define SPAN_MPAN_TABLE_SIZE_V0  10

    //Migrate files from file system version 0 or 1 to 3.
    if ( 2 > presentFilesysVersion )
    {
      //SSlaveInfo nodeID changed from uint8_t to uint16_t
      SSlaveInfo slaveInfo;
      uint8_t * pSlaveInfo = (uint8_t *)&slaveInfo;
      memset(pSlaveInfo, 0xFF, sizeof(SSlaveInfo)); //make sure all bits in slaveInfo are set
      const size_t oldFileLength = 6;
      zpal_nvm_read(pFileSystem, FILE_ID_SLAVEINFO, pSlaveInfo, oldFileLength);

      //Move smartStartState and nodeID to new positions
      slaveInfo.smartStartState = *(pSlaveInfo + HOMEID_LENGTH + 1);
      slaveInfo.nodeID = *(pSlaveInfo + HOMEID_LENGTH);  //overwrites *(pSlaveInfo + HOMEID_LENGTH + 1)
      slaveInfo.primaryLongRangeChannelId = 0;
      zpal_nvm_write(pFileSystem, FILE_ID_SLAVEINFO, pSlaveInfo, sizeof(slaveInfo));

      //s2_private_key and s2_public_key were removed from Ss2_keys
      typedef struct Ss2_keys_Old
      {
        uint8_t s2_private_key[32]; //[ZW_S2_PRIVATE_KEY_SIZE]
        uint8_t s2_public_key[32];  //[ZW_S2_PUBLIC_KEY_SIZE]
        uint8_t s2_network_keys[4][16];  //[S2_NUM_KEY_CLASSES][ZW_S2_NETWORK_KEY_SIZE];
      } Ss2_keys_Old;
      Ss2_keys_Old oldKeys = {0};
      const uint32_t sizeof_network_keys = 4 * 16;
      memset(oldKeys.s2_network_keys, 0xFF, sizeof_network_keys); //memset in case reading fails
      zpal_nvm_read(pFileSystem, FILE_ID_S2_KEYS, &oldKeys, sizeof(oldKeys));
      Ss2_keys keys;
      memcpy(keys.s2_network_keys, oldKeys.s2_network_keys, sizeof_network_keys);
      zpal_nvm_write(pFileSystem, FILE_ID_S2_KEYS, &keys, sizeof(keys));

      //lnode and rnode in SPAN were changed from uint8_t to uint16_t
      typedef struct SPAN_Old
      {
        union
        {
          CTR_DRBG_CTX rng;
          uint8_t r_nonce[16];
        } d;
        uint8_t lnode;
        uint8_t rnode;

        uint8_t rx_seq; // sequence number of last received message
        uint8_t tx_seq; // sequence number of last sent message

        uint8_t class_id; //The id of the security group in which this span is negotiated.
        span_state_t state;
      } SPAN_Old;

      SPAN_Old span_old_file[SPAN_MPAN_TABLE_SIZE_V0] = {0};  //There were 10 SPANs in the old file
      zpal_nvm_read(pFileSystem, FILE_ID_S2_SPAN, &span_old_file, sizeof(span_old_file));

      for (uint32_t i=0; i < SPAN_MPAN_TABLE_SIZE_V0; i++)
      {
        struct SPAN span_new_file = {
          .lnode    = span_old_file[i].lnode,
          .rnode    = span_old_file[i].rnode,
          .rx_seq   = span_old_file[i].rx_seq,
          .tx_seq   = span_old_file[i].tx_seq,
          .class_id = span_old_file[i].class_id,
          .state    = span_old_file[i].state
        };
        memcpy(&(span_new_file.d), &(span_old_file[i].d), sizeof(span_old_file[i].d));
        StorageWriteS2Span(i, &span_new_file);
      }
      zpal_nvm_erase_object(pFileSystem, FILE_ID_S2_SPAN);

      //owner_id in MPAN was changed from uint8_t to uint16_t
      typedef struct MPAN_Old
      {
        uint8_t owner_id; //this is the node id of the node maintaining mcast group members
        uint8_t group_id; //a unique id generated by the group maintainer(sender)
        uint8_t inner_state[16]; //The Multicast  pre-agreed nonce inner state
        uint8_t class_id;

        enum
        {
          MPAN_NOT_USED, MPAN_SET, MPAN_MOS
        } state; //State of this entry
      } MPAN_Old;

      MPAN_Old mpan_old_file[SPAN_MPAN_TABLE_SIZE_V0] = {0};  //There were 10 MPANs in the old file
      zpal_nvm_read(pFileSystem, FILE_ID_S2_MPAN, &mpan_old_file, sizeof(mpan_old_file));
      for (uint32_t i=0; i < SPAN_MPAN_TABLE_SIZE_V0; i++)
      {
        struct MPAN mpan_new_file = {
          .owner_id = mpan_old_file[i].owner_id,
          .group_id = mpan_old_file[i].group_id,
          .class_id = mpan_old_file[i].class_id,
          .state    = (size_t)mpan_old_file[i].state
        };
        memcpy(&(mpan_new_file.inner_state), &(mpan_old_file[i].inner_state), sizeof(mpan_old_file[i].inner_state));

        StorageWriteS2Mpan(i, &mpan_new_file);
      }
      zpal_nvm_erase_object(pFileSystem, FILE_ID_S2_MPAN);
    }

    //Migrate files from file system version 2 to 3.
    if ( 2 == presentFilesysVersion )
    {
      //Changed from saving an array of 10 SPANs in one file to saving 10 different files with 1 SPAN each.
      struct SPAN span_old_file[SPAN_MPAN_TABLE_SIZE_V0] = {0};  //There were 10 SPAN structs in the old file
      zpal_nvm_read(pFileSystem, FILE_ID_S2_SPAN, &span_old_file, sizeof(span_old_file));

      for (uint32_t i=0; i < SPAN_MPAN_TABLE_SIZE_V0; i++)
      {
        StorageWriteS2Span(i, &span_old_file[i]);
      }
      //Delete old file
      zpal_nvm_erase_object(pFileSystem, FILE_ID_S2_SPAN);

      //Changed from saving an array of 10 MPANs in one file to saving 10 different files with 1 MPAN each.
      struct MPAN mpan_old_file[SPAN_MPAN_TABLE_SIZE_V0] = {0};  //There were 10 MPAN structs in the old file
      zpal_nvm_read(pFileSystem, FILE_ID_S2_MPAN, &mpan_old_file, sizeof(mpan_old_file));

      for (uint32_t i=0; i < SPAN_MPAN_TABLE_SIZE_V0; i++)
      {
        StorageWriteS2Mpan(i, &mpan_old_file[i]);
      }
      //Delete old file
      zpal_nvm_erase_object(pFileSystem, FILE_ID_S2_MPAN);
    }
#endif // ZWAVE_MIGRATE_FILESYSTEM

    // From version 3 to 5 nothing happened for 700s devices. On 800s devices the application file system was merged
    // to the protocol file system. Migration code is in zpal_nvm.c
    if ( 5 > presentFilesysVersion )
    {
      zpal_nvm_migrate_legacy_app_file_system();
    }

    //Update File System Version file
    SlaveStorageSetZWVersion(&ZW_Version);
  }

  //Verify that the expected files exist in the present file system and have correct file sizes
  SObjectSet FileSet = {
    .pFileSystem = pFileSystem,
    .iObjectCount = sizeof_array(g_aFileDescriptors),
    .pObjectDescriptors = g_aFileDescriptors
  };
  ECaretakerStatus eFileSetVerifyStatus = NVMCaretakerVerifySet(&FileSet, g_aFileStatus);

  bool set_file_ok = true;
  if (ECTKR_STATUS_SUCCESS != eFileSetVerifyStatus)
  {
    zpal_nvm_object_key_t objectKey = 0;

    DeleteStorageCaches();
    DPRINT("FileSystem Verify failed\r\n");
    for(uint8_t i = 0; i < FileSet.iObjectCount; i++)
    {
      objectKey = FileSet.pObjectDescriptors[i].ObjectKey;

      if(ECTKR_STATUS_UNABLE_TO_AQUIRE_HANDLE == g_aFileStatus[i])
      {
        DPRINTF("Object Key: 0x%x not found\r\n", objectKey);
        //Write new file with default data.
        set_file_ok = set_file_ok && SetFile(objectKey, 0);
      }
      else if(ECTKR_STATUS_SIZE_MISMATCH == g_aFileStatus[i])
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

  //Read SLAVE_FILE_MAP to buffer
  zpal_nvm_read(pFileSystem, FILE_ID_SLAVE_FILE_MAP, &SlaveFileMap, sizeof(SlaveFileMap));
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
void ZeroPadFile(zpal_nvm_object_key_t fileID, void * dataBuf, size_t bufLength, size_t copyLength)
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
  zpal_status_t wStatus = ZPAL_STATUS_FAIL;
  switch (fileID)
  {
    case FILE_ID_RETURNROUTESDESTINATIONS:
    {
      //Write default Return Route Destinations file
      SReturnRouteDestinationsCached tReturnRouteDestinationsCached = { 0 };
      ZeroPadFile(FILE_ID_RETURNROUTESDESTINATIONS, &tReturnRouteDestinationsCached, sizeof(tReturnRouteDestinationsCached), copyLength);
      wStatus = zpal_nvm_write(pFileSystem, FILE_ID_RETURNROUTESDESTINATIONS, &tReturnRouteDestinationsCached, sizeof(tReturnRouteDestinationsCached));
      break;
    }
    case FILE_ID_SLAVEINFO:
    {
      //Write default Network IDs file
      SSlaveInfo tSlaveInfo = { 0 };
      ZeroPadFile(FILE_ID_SLAVEINFO, &tSlaveInfo, sizeof(tSlaveInfo), copyLength);
      wStatus = zpal_nvm_write(pFileSystem, FILE_ID_SLAVEINFO, &tSlaveInfo, sizeof(tSlaveInfo));
      break;
    }
    case FILE_ID_SLAVE_FILE_MAP:
    {
      ZeroPadFile(FILE_ID_SLAVE_FILE_MAP, &SlaveFileMap, sizeof(SlaveFileMap), copyLength);
      wStatus = zpal_nvm_write(pFileSystem, FILE_ID_SLAVE_FILE_MAP, &SlaveFileMap, sizeof(SlaveFileMap));
      break;
    }
    case FILE_ID_S2_KEYS:
    {
      Ss2_keys tS2_keys;
      memset(&tS2_keys, 0xFF, sizeof(tS2_keys));
      wStatus = zpal_nvm_write(pFileSystem, FILE_ID_S2_KEYS, &tS2_keys, sizeof(tS2_keys));
      break;
    }
    case FILE_ID_S2_KEYCLASSES_ASSIGNED:
    {
      Ss2_keyclassesAssigned tS2_keyclassesAssigned;
      ZeroPadFile(FILE_ID_S2_KEYCLASSES_ASSIGNED, &tS2_keyclassesAssigned, sizeof(tS2_keyclassesAssigned), copyLength);
      wStatus = zpal_nvm_write(pFileSystem, FILE_ID_S2_KEYCLASSES_ASSIGNED, &tS2_keyclassesAssigned, sizeof(tS2_keyclassesAssigned));
      break;
    }
    case FILE_ID_ZW_PRIVATE_KEY:
    {
      uint8_t zw_prk[FILE_SIZE_ZW_PRIVATE_KEY];
      memset(&zw_prk, 0xFF, FILE_SIZE_ZW_PRIVATE_KEY);
      wStatus = zpal_nvm_write(pFileSystem, FILE_ID_ZW_PRIVATE_KEY, &zw_prk, FILE_SIZE_ZW_PRIVATE_KEY);
      break;
    }
    case FILE_ID_ZW_PUBLIC_KEY:
    {
      uint8_t zw_puk[FILE_SIZE_ZW_PUBLIC_KEY];
      memset(&zw_puk, 0xFF, FILE_SIZE_ZW_PUBLIC_KEY);
      wStatus = zpal_nvm_write(pFileSystem, FILE_ID_ZW_PUBLIC_KEY, &zw_puk, FILE_SIZE_ZW_PUBLIC_KEY);
      break;
    }

    default:
      DPRINTF("Wrong fileID\n");
  }
  return ZPAL_STATUS_OK == wStatus;
}

/**
* Write default slave storage files with default content.
*
* Used both as File system formatted callback from NVM module,
* and by SlaveStorageFileSystemInit if FS has been formatted.
*
* Clears FileHandles (needed when used as callback that FS is formatted).
* Writes required files to FS with default values.
*/
static void
WriteDefaultSetofFiles(void)
{
  bool set_file_ok = true;

  DeleteStorageCaches();

  //Write default Return Route Destinations file
  set_file_ok = set_file_ok && SetFile(FILE_ID_RETURNROUTESDESTINATIONS, 0);

  //Write default Network IDs file
  set_file_ok = set_file_ok && SetFile(FILE_ID_SLAVEINFO, 0);

  //Write default File Map file
  set_file_ok = set_file_ok && SetFile(FILE_ID_SLAVE_FILE_MAP, 0);

  //Init S2 keys
  set_file_ok = set_file_ok && SetFile(FILE_ID_S2_KEYS, 0);

  //Init S2 keyclasses assigned
  set_file_ok = set_file_ok && SetFile(FILE_ID_S2_KEYCLASSES_ASSIGNED, 0);

  //Assert files were written
  ASSERT(set_file_ok);

  //Write default File system Version file
  SlaveStorageSetZWVersion(&ZW_Version);

  DPRINT("Slave Storage Reset to default\r\n");
}

ZW_WEAK void SlaveStorageDeleteMPANs(void)
{
  for(uint8_t i = 0; i < MPAN_TABLE_SIZE; i++)
  {
    zpal_nvm_erase_object(pFileSystem, FILE_ID_S2_MPAN_BASE + i);
  }
}

ZW_WEAK void SlaveStorageDeleteSPANs(void)
{
  for(uint8_t i = 0; i < SPAN_TABLE_SIZE; i++)
  {
    zpal_nvm_erase_object(pFileSystem, FILE_ID_S2_SPAN_BASE + i);
  }
}

/* @brief Delete all slave network info storage by file-id
 */

void DeleteSlaveNetworkInfoStorage(void)
{
  DeleteStorageCaches();

  zpal_nvm_erase_object(pFileSystem, FILE_ID_ZW_VERSION);

  zpal_nvm_erase_object(pFileSystem, FILE_ID_S2_KEYS);

  zpal_nvm_erase_object(pFileSystem, FILE_ID_S2_KEYCLASSES_ASSIGNED);

  zpal_nvm_erase_object(pFileSystem, FILE_ID_SLAVEINFO);

  zpal_nvm_erase_object(pFileSystem, FILE_ID_SLAVE_FILE_MAP);

  zpal_nvm_erase_object(pFileSystem, FILE_ID_RETURNROUTESDESTINATIONS);

  SlaveStorageDeleteAllReturnRouteInfo();
  SlaveStorageDeleteSucReturnRouteInfo();
  SlaveStorageDeleteMPANs();
  SlaveStorageDeleteSPANs();

  NVM3_InvokeCbs();
}