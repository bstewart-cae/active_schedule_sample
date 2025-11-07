// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file TestZW_controller_network_info_storage.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include "SyncEvent.h"
//#define DEBUGPRINT
#include "DebugPrint.h"
#include "unity.h"
#include "mock_control.h"
#include "ZW_controller_network_info_storage.h"
#include "ZW_NVMCaretaker.h"
#include <ZW_nvm.h>

#include "Assert.h"
#include "SizeOf.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "ZW_lib_defines.h"
#include "ZW_Security_Scheme2.h"

void setUpSuite(void) {

}

void tearDownSuite(void) {

}

void setUp(void) {

}

void tearDown(void) {

}

// @attention: This needs to be updated each time the max power is raised. Exists also in mocks.
#define DYNAMIC_TX_POWR_MAX           14  // Same as zpal_radio_get_maximum_lr_tx_power()!

#define CONTROLLERINFO_FILE_SIZE      22

#define NODEINFOS_PER_FILE            4
#define NODEINFOS_PER_FILE_LR         50

#define SIZEOF_SNODINFOSTORAGE        35
#define SIZEOF_SNODINFOSTORAGE_LR     3

#define NODEROUTECACHES_PER_FILE      8
#define NODEROUTECACHE_FILES_IN_RAM   4

#define SUC_UNKNOWN_CONTROLLER        0xFE

#define SUCNODES_PER_FILE             8

#define SIZE_OF_SNODEINFOSTORAGE      35
#define SIZE_OF_SNODEINFOLONGRANGE     3

// Defined used in nodeinfo files and route cache files tests
 #define TEST_VAL1        (t_nodeID + 5)
 #define TEST_VAL2        (t_nodeID + 6)
 #define TEST_VAL3        (t_nodeID + 7)
 #define TEST_VAL4        (t_nodeID + 8)
 #define TEST_VAL5        (t_nodeID + 9)
 #define TEST_VAL6        (uint8_t)(i + t_nodeID + 10)
 #define TEST_VAL7        (t_nodeID + 11)
 #define TEST_VAL8(ID)    (uint8_t)(ID + t_nodeID + 12)
 #define TEST_VAL9        (t_nodeID + 13)
 #define TEST_VAL10(ID)   (uint8_t)(ID + t_nodeID + 14)
 #define TEST_VAL11       (t_nodeID + 15)


#define LR_NODES_TX_POWER_PER_FILE        32
#define LR_NODES_TX_POWER_FILES_COUNT     (ZW_MAX_NODES_LR / LR_NODES_TX_POWER_PER_FILE)

#define FILE_ID_LR_TX_POWER_BASE          (0x02000)           // The base file Id for the TX Power files in nvm
#define FILE_SIZE_LR_TX_POWER             (LR_NODES_TX_POWER_PER_FILE)

#define FILE_ID_LR_TX_POWER_LAST          (FILE_ID_LR_TX_POWER_BASE + LR_NODES_TX_POWER_FILES_COUNT - 1)

#define SIZE_OF_NODEINFOFILE     (SIZEOF_SNODINFOSTORAGE * NODEINFOS_PER_FILE)
#define SIZE_OF_NODEINFOFILE_LR  (SIZEOF_SNODINFOSTORAGE_LR * NODEINFOS_PER_FILE_LR)

// Set the current file system version that is of this release
uint32_t const m_current_ZW_Version = (ZW_CONTROLLER_FILESYS_VERSION << 24) | (ZW_VERSION_MAJOR << 16) | (ZW_VERSION_MINOR << 8) | (ZW_VERSION_PATCH);

// Set the earliest file system version to trigger migration of old file-system.
uint32_t const m_old_ZW_Version_v0 = (0 << 24) | (ZW_VERSION_MAJOR << 16) | (ZW_VERSION_MINOR << 8) | (ZW_VERSION_PATCH);

// Set the earliest file system version to trigger migration of old file-system.
uint32_t const m_old_ZW_Version_v2 = (2 << 24) | (ZW_VERSION_MAJOR << 16) | (ZW_VERSION_MINOR << 8) | (ZW_VERSION_PATCH);


//Copy of struct in ZW_controller_network_info_storage.c
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
  uint8_t                       LongRangeChannelAutoMode;
}SControllerInfo;

#define DefaultControllerInfoSucLastIndex             (0xFF)
#define DefaultControllerInfoControllerConfiguration  (0x28)

//SControllerInfo with default values
SControllerInfo defaultControllerInfo = {
                                         .HomeID = {0,0,0,0},
                                         .NodeID = 0x0001,
                                         .StaticControllerNodeId = 0,
                                         .LastUsedNodeId_LR = LOWEST_LONG_RANGE_NODE_ID - 1,
                                         .LastUsedNodeId = 1,
                                         .SucLastIndex = DefaultControllerInfoSucLastIndex,
                                         .MaxNodeId_LR = 0x0000,
                                         .MaxNodeId = 1,
                                         .ControllerConfiguration = DefaultControllerInfoControllerConfiguration,
                                         .ReservedId_LR = 0,
                                         .ReservedId = 0,
                                         .SystemState = 0,
                                         .PrimaryLongRangeChannelId = 0,
                                         .LongRangeChannelAutoMode = 0
};


//Ss2_keyclassesAssigned  with default value
Ss2_keyclassesAssigned default_Ss2_keyclassesAssigned = {.keyclasses_assigned = 0 };


//Wrapper that adds 1 to nodeID
static void NodeMaskSetBit(uint8_t* pMask, uint8_t nodeID)
{
  ZW_NodeMaskSetBit(pMask, nodeID + 1);
}

//Wrapper that adds 1 to nodeID
static void NodeMaskClearBit(uint8_t* pMask, uint8_t nodeID)
{
  ZW_NodeMaskClearBit(pMask, nodeID + 1);
}

// Testing Test writing and reading node info field in the noed info files
void test_ControllerStorage_nodeinfo(void)
{
  mock_calls_clear();
//--------------------------------------------------------------------------------------------------------
// File system init start
//--------------------------------------------------------------------------------------------------------
  uint8_t dummy;
  zpal_nvm_handle_t pFileSystem = &dummy;

//--------------------------------------------------------------------------------------------------------
// -------------------------------------------------------filesyseminit end
//--------------------------------------------------------------------------------------------------------

  // initialize controller network
  mock_t * pMockNvmFileSystemRegister;
  mock_call_use_as_stub(TO_STR(NvmFileSystemInit));
  mock_call_expect(TO_STR(NvmFileSystemRegister), &pMockNvmFileSystemRegister);
  pMockNvmFileSystemRegister->return_code.pointer = pFileSystem;
  pMockNvmFileSystemRegister->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;

  mock_t * pMock;
  mock_call_expect(TO_STR(zpal_nvm_read), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00000; //FILE_ID_ZW_VERSION
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG3].value = 4; //sizeof(uint32_t)
  //Set current file system version as output
  uint32_t version = m_current_ZW_Version;
  pMock->output_arg[ARG2].p = (uint8_t *)&version;

  mock_t * pMockCaretakerVerify;
  mock_call_expect(TO_STR(NVMCaretakerVerifySet), &pMockCaretakerVerify);
  pMockCaretakerVerify->return_code.value = ECTKR_STATUS_SUCCESS;  //return that the files were found
  pMockCaretakerVerify->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMockCaretakerVerify->compare_rule_arg[ARG1] = COMPARE_NOT_NULL;

  uint32_t file_id[8] = {0x00005,  //FILE_ID_NODE_STORAGE_EXIST
                         0x0000B,  //FILE_ID_NODE_ROUTECACHE_EXIST
                         0x0000C,  //FILE_ID_LRANGE_NODE_EXIST
                         0x00006,  //FILE_ID_APP_ROUTE_LOCK_FLAG
                         0x00007,  //FILE_ID_ROUTE_SLAVE_SUC_FLAG
                         0x00008,  //FILE_ID_SUC_PENDING_UPDATE_FLAG
                         0x00009,  //FILE_ID_BRIDGE_NODE_FLAG
                         0x0000A   //FILE_ID_PENDING_DISCOVERY_FLAG
                        };

  for(uint32_t i=0; i<(sizeof(file_id)/sizeof(uint32_t)); i++)
  {
    mock_call_expect(TO_STR(zpal_nvm_read), &pMock);
    pMock->return_code.value = ZPAL_STATUS_OK;
    pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
    pMock->expect_arg[ARG1].value = file_id[i]; //FILE_ID_
    pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
    if(0x0000C == file_id[i])  //Special nodemask for FILE_ID_LRANGE_NODE_EXIST file
    {
      pMock->expect_arg[ARG3].value = sizeof(LR_NODE_MASK_TYPE);
    }
    else
    {
      pMock->expect_arg[ARG3].value = sizeof(NODE_MASK_TYPE);
    }
  }

  CtrlStorageInit();

  SSyncEvent * pRegisterEvent = (SSyncEvent*)pMockNvmFileSystemRegister->actual_arg[ARG0].pointer;
  TEST_ASSERT_NOT_NULL(pRegisterEvent);
  TEST_ASSERT_NOT_NULL(pRegisterEvent->uFunctor.pFunction);
  TEST_ASSERT_NULL(pRegisterEvent->pObject);

  // test that when reading non existing node, all fields are zero
  for (uint8_t t_nodeID = 1; t_nodeID <= ZW_MAX_NODES; t_nodeID++)
  {
    EX_NVM_NODEINFO t_nodeInfo;
    NODE_MASK_TYPE rangeInfo;

     //TDU 1.1  Test non existing node info field is zero
    CtrlStorageGetNodeInfo(t_nodeID, &t_nodeInfo);
    CtrlStorageGetRoutingInfo(t_nodeID, &rangeInfo);
    TEST_ASSERT_MESSAGE(0 == t_nodeInfo.capability, "TDU 1.1");
    TEST_ASSERT_MESSAGE(0 == t_nodeInfo.security,   "TDU 1.1");
    TEST_ASSERT_MESSAGE(0 == t_nodeInfo.reserved,   "TDU 1.1");
    TEST_ASSERT_MESSAGE(0 == t_nodeInfo.generic,    "TDU 1.1");
    TEST_ASSERT_MESSAGE(0 == t_nodeInfo.specific,   "TDU 1.1");

     // TDU 1.2 Test non existing range info is zero
    for (uint8_t i =0; i < sizeof(NODE_MASK_TYPE); i++)
    {
      TEST_ASSERT_MESSAGE(0 == rangeInfo[i], "TDU 1.2");
    }

     // TDU 1.3 Test non existing SUC update index is SUC_UNKNOWN_CONTROLLER
    TEST_ASSERT_MESSAGE(SUC_UNKNOWN_CONTROLLER == CtrlStorageGetCtrlSucUpdateIndex(t_nodeID), "TDU 1.3");

     // TDU 1.6 Test non existing flags in nodeinfo file are all zero
    TEST_ASSERT_MESSAGE(0 == CtrlStorageGetAppRouteLockFlag(t_nodeID),          "TDU 1.6");
    TEST_ASSERT_MESSAGE(0 == CtrlStorageGetBridgeNodeFlag(t_nodeID),            "TDU 1.6");
    TEST_ASSERT_MESSAGE(0 == CtrlStorageGetPendingDiscoveryStatus(t_nodeID),    "TDU 1.6");
    TEST_ASSERT_MESSAGE(0 == CtrlStorageGetRoutingSlaveSucUpdateFlag(t_nodeID), "TDU 1.6");
    TEST_ASSERT_MESSAGE(0 == CtrlStorageIsNodeInPendingUpdateTable(t_nodeID),   "TDU 1.6");

    //TDU 1.1  Test non existing Long Range node info field is zero
    CtrlStorageGetNodeInfo(0x0100 + t_nodeID, &t_nodeInfo);
    TEST_ASSERT_MESSAGE(0 == t_nodeInfo.capability, "TDU 1.1 LR");
    TEST_ASSERT_MESSAGE(0 == t_nodeInfo.security,   "TDU 1.1 LR");
    TEST_ASSERT_MESSAGE(0 == t_nodeInfo.reserved,   "TDU 1.1 LR");
    TEST_ASSERT_MESSAGE(0 == t_nodeInfo.generic,    "TDU 1.1 LR");
    TEST_ASSERT_MESSAGE(0 == t_nodeInfo.specific,   "TDU 1.1 LR");
  }

  mock_calls_verify();

  uint8_t nodeInfoEntryWrite[SIZE_OF_NODEINFOFILE];
  uint8_t nodeInfoEntryRead[SIZE_OF_NODEINFOFILE];

  for (uint8_t t_nodeID = 1; t_nodeID <= ZW_MAX_NODES; t_nodeID++)
  {
    mock_calls_clear();

    //nodeID 1-232
    //nodeNr 0-231  for local indexing in arrays
    uint8_t t_nodeNr = t_nodeID - 1;

    EX_NVM_NODEINFO t_nodeInfo;
    NODE_MASK_TYPE rangeInfo;

    t_nodeInfo.capability = TEST_VAL1;
    t_nodeInfo.security = TEST_VAL2;
    t_nodeInfo.reserved = TEST_VAL3;
    t_nodeInfo.generic = TEST_VAL4;
    t_nodeInfo.specific = TEST_VAL5;

    if(0 == (t_nodeNr % NODEINFOS_PER_FILE))
    {
      memset(&nodeInfoEntryWrite, 0xFF, sizeof(nodeInfoEntryWrite));
    }

    uint32_t offset_in_file = (t_nodeNr % NODEINFOS_PER_FILE) * SIZEOF_SNODINFOSTORAGE;
    memcpy(&nodeInfoEntryWrite[offset_in_file], &t_nodeInfo, sizeof(EX_NVM_NODEINFO));
    memset(&nodeInfoEntryWrite[offset_in_file + sizeof(EX_NVM_NODEINFO)], 0, sizeof(NODE_MASK_TYPE));
    nodeInfoEntryWrite[offset_in_file + SIZEOF_SNODINFOSTORAGE -1] = 0xFE; //SUC_UNKNOWN_CONTROLLER

    mock_call_expect(TO_STR(zpal_nvm_write), &pMock);
    pMock->return_code.value = ZPAL_STATUS_OK;
    pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
    pMock->expect_arg[ARG1].value = 0x00200 + t_nodeNr/NODEINFOS_PER_FILE;
    pMock->expect_arg[ARG2].p = &nodeInfoEntryWrite;
    pMock->expect_arg[ARG3].value = SIZE_OF_NODEINFOFILE;

    uint8_t mockFileMap[sizeof(NODE_MASK_TYPE)];
    memset(mockFileMap, 0, sizeof(mockFileMap));
    for(uint32_t i=0; i<sizeof(mockFileMap); i++)
    {
      if(i < t_nodeNr/8)
      {
        mockFileMap[i] = 0xFF;
      }
      else if(i == t_nodeNr/8)
      {
        mockFileMap[i] = ~(0xFF00 >> (7 - t_nodeNr%8));
      }
      else
      {
        mockFileMap[i] = 0x00;
      }
    }
    mock_call_expect(TO_STR(zpal_nvm_write), &pMock);
    pMock->return_code.value = ZPAL_STATUS_OK;
    pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
    pMock->expect_arg[ARG1].value = 0x00005; //FILE_ID_NODE_STORAGE_EXIST
    pMock->expect_arg[ARG2].p = mockFileMap;
    pMock->expect_arg[ARG3].value = sizeof(NODE_MASK_TYPE);

    mock_call_expect(TO_STR(zpal_nvm_write), &pMock);
    pMock->return_code.value = ZPAL_STATUS_OK;
    pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
    pMock->expect_arg[ARG1].value = 0x00007; //FILE_ID_ROUTE_SLAVE_SUC_FLAG
    pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
    pMock->expect_arg[ARG3].value = sizeof(NODE_MASK_TYPE);

    CtrlStorageSetNodeInfo(t_nodeID, &t_nodeInfo);

    memcpy(nodeInfoEntryRead, nodeInfoEntryWrite, SIZE_OF_NODEINFOFILE);

    for (uint8_t i =0; i < sizeof(NODE_MASK_TYPE); i++)
    {
      rangeInfo[i] = TEST_VAL6;
    }

    memcpy(&nodeInfoEntryWrite[offset_in_file + 5], &rangeInfo, 29); //5 == offsetof(SNodeInfoStorage, neighboursInfo), 29 == sizeof(NODE_MASK_TYPE)

    mock_call_expect(TO_STR(zpal_nvm_write), &pMock);
    pMock->return_code.value = ZPAL_STATUS_OK;
    pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
    pMock->expect_arg[ARG1].value = 0x00200 + t_nodeNr/NODEINFOS_PER_FILE;
    pMock->expect_arg[ARG2].p = &nodeInfoEntryWrite;
    pMock->expect_arg[ARG3].value = SIZE_OF_NODEINFOFILE;

    CtrlStorageSetRoutingInfo(t_nodeID, &rangeInfo, true);

    memcpy(nodeInfoEntryRead, nodeInfoEntryWrite, SIZE_OF_NODEINFOFILE);

    nodeInfoEntryWrite[offset_in_file + SIZEOF_SNODINFOSTORAGE -1] = TEST_VAL11;

    mock_call_expect(TO_STR(zpal_nvm_write), &pMock);
    pMock->return_code.value = ZPAL_STATUS_OK;
    pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
    pMock->expect_arg[ARG1].value = 0x00200 + t_nodeNr/NODEINFOS_PER_FILE;
    pMock->expect_arg[ARG2].p = &nodeInfoEntryWrite;
    pMock->expect_arg[ARG3].value = SIZE_OF_NODEINFOFILE;

    CtrlStorageSetCtrlSucUpdateIndex(t_nodeID, TEST_VAL11);

    memcpy(nodeInfoEntryRead, nodeInfoEntryWrite, SIZE_OF_NODEINFOFILE);

    mock_call_expect(TO_STR(zpal_nvm_write), &pMock);
    pMock->return_code.value = ZPAL_STATUS_OK;
    pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
    pMock->expect_arg[ARG1].value = 0x00008;  //FILE_ID_SUC_PENDING_UPDATE_FLAG
    pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
    pMock->expect_arg[ARG3].value = sizeof(NODE_MASK_TYPE);

    CtrlStorageChangeNodeInPendingUpdateTable(t_nodeID, true);

    mock_call_expect(TO_STR(zpal_nvm_write), &pMock);
    pMock->return_code.value = ZPAL_STATUS_OK;
    pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
    pMock->expect_arg[ARG1].value = 0x00007;  //FILE_ID_ROUTE_SLAVE_SUC_FLAG
    pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
    pMock->expect_arg[ARG3].value = sizeof(NODE_MASK_TYPE);

    CtrlStorageSetRoutingSlaveSucUpdateFlag(t_nodeID, false, true);

    mock_call_expect(TO_STR(zpal_nvm_write), &pMock);
    pMock->return_code.value = ZPAL_STATUS_OK;
    pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
    pMock->expect_arg[ARG1].value = 0x00006;  //FILE_ID_APP_ROUTE_LOCK_FLAG
    pMock->expect_arg[ARG2].p = mockFileMap;
    pMock->expect_arg[ARG3].value = sizeof(NODE_MASK_TYPE);

    CtrlStorageSetAppRouteLockFlag(t_nodeID, true);

    mock_call_expect(TO_STR(zpal_nvm_write), &pMock);
    pMock->return_code.value = ZPAL_STATUS_OK;
    pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
    pMock->expect_arg[ARG1].value = 0x00009;  //FILE_ID_BRIDGE_NODE_FLAG
    pMock->expect_arg[ARG2].p = mockFileMap;
    pMock->expect_arg[ARG3].value = sizeof(NODE_MASK_TYPE);

    CtrlStorageSetBridgeNodeFlag(t_nodeID, true, true);

    mock_call_expect(TO_STR(zpal_nvm_write), &pMock);
    pMock->return_code.value = ZPAL_STATUS_OK;
    pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
    pMock->expect_arg[ARG1].value = 0x0000A;  //FILE_ID_PENDING_DISCOVERY_FLAG
    pMock->expect_arg[ARG2].p = mockFileMap;
    pMock->expect_arg[ARG3].value = sizeof(NODE_MASK_TYPE);

    CtrlStorageSetPendingDiscoveryStatus(t_nodeID, true);


    mock_call_expect(TO_STR(zpal_nvm_read_object_part), &pMock);
    pMock->return_code.value = ZPAL_STATUS_OK;
    pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
    pMock->expect_arg[ARG1].value = 0x00200 + t_nodeNr/NODEINFOS_PER_FILE; //FILE_ID_NODEINFO + nodeID
    pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
    pMock->output_arg[ARG2].p = &nodeInfoEntryRead[SIZE_OF_SNODEINFOSTORAGE * (t_nodeNr % NODEINFOS_PER_FILE)];
    pMock->expect_arg[ARG3].value = SIZE_OF_SNODEINFOSTORAGE * (t_nodeNr % NODEINFOS_PER_FILE);
    pMock->expect_arg[ARG4].value = sizeof(EX_NVM_NODEINFO);


    CtrlStorageGetNodeInfo(t_nodeID, &t_nodeInfo);

    // TDU 1.7 test that data read from the node info field are that same as the data prevouisly written to it
    TEST_ASSERT_MESSAGE(TEST_VAL1 == t_nodeInfo.capability, "TDU 1.7");
    TEST_ASSERT_MESSAGE(TEST_VAL2 == t_nodeInfo.security,   "TDU 1.7");
    TEST_ASSERT_MESSAGE(TEST_VAL3 == t_nodeInfo.reserved,   "TDU 1.7");
    TEST_ASSERT_MESSAGE(TEST_VAL4 == t_nodeInfo.generic,    "TDU 1.7");
    TEST_ASSERT_MESSAGE(TEST_VAL5 == t_nodeInfo.specific,   "TDU 1.7");

    mock_call_expect(TO_STR(zpal_nvm_read), &pMock);
    pMock->return_code.value = ZPAL_STATUS_OK;
    pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
    pMock->expect_arg[ARG1].value = 0x00200 + t_nodeNr/NODEINFOS_PER_FILE;
    pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
    pMock->output_arg[ARG2].p = &nodeInfoEntryRead;
    pMock->expect_arg[ARG3].value = SIZE_OF_NODEINFOFILE;

    // TDU 1.8 test that data read from the routing info field are that same as the data prevouisly written to it
    CtrlStorageGetRoutingInfo(t_nodeID, &rangeInfo);
    for (uint8_t i =0; i < sizeof(NODE_MASK_TYPE); i++)
    {
      TEST_ASSERT_MESSAGE(TEST_VAL6 == rangeInfo[i], "TDU 1.8");
    }

    mock_call_expect(TO_STR(zpal_nvm_read), &pMock);
    pMock->return_code.value = ZPAL_STATUS_OK;
    pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
    pMock->expect_arg[ARG1].value = 0x00200 + t_nodeNr/NODEINFOS_PER_FILE;
    pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
    pMock->output_arg[ARG2].p = &nodeInfoEntryRead;
    pMock->expect_arg[ARG3].value = SIZE_OF_NODEINFOFILE;

    // TDU 1.9 test that data read from the suc update index field is that same as the data prevouisly written to it
    TEST_ASSERT_MESSAGE(TEST_VAL11 == CtrlStorageGetCtrlSucUpdateIndex(t_nodeID), "TDU 1.9");

    // TDU 1.12 test that data read from the node flags filed are that same as the data previously written to it
    TEST_ASSERT_MESSAGE(CtrlStorageGetAppRouteLockFlag(t_nodeID),           "TDU 1.12");
    TEST_ASSERT_MESSAGE(CtrlStorageGetBridgeNodeFlag(t_nodeID),             "TDU 1.12");
    TEST_ASSERT_MESSAGE(CtrlStorageGetPendingDiscoveryStatus(t_nodeID),     "TDU 1.12");
    TEST_ASSERT_MESSAGE(!CtrlStorageGetRoutingSlaveSucUpdateFlag(t_nodeID), "TDU 1.12");
    TEST_ASSERT_MESSAGE(CtrlStorageIsNodeInPendingUpdateTable(t_nodeID),    "TDU 1.12");

    mock_calls_verify();
  }

  const uint8_t CAPABILITY_VAL = 0xC3;  // ZWAVE_NODEINFO_VERSION_4 | ZWAVE_NODEINFO_ROUTING_SUPPORT | ZWAVE_NODEINFO_LISTENING_SUPPORT

  const uint8_t SECURITY_VAL   = 0xF5;  // ZWAVE_NODEINFO_SECURITY_SUPPORT | ZWAVE_NODEINFO_SPECIFIC_DEVICE_TYPE | ZWAVE_NODEINFO_BEAM_CAPABILITY
                                        // | ZWAVE_NODEINFO_OPTIONAL_FUNC | NODEINFO_MASK_SENSOR

  const uint8_t RESERVED_VAL   = 0x02;  // ZWAVE_NODEINFO_BAUD_100KLR

  uint8_t nodeInfoEntryWrite_LR[SIZE_OF_NODEINFOFILE_LR];
  //Tests for Z-Wave Long Range nodes
  for (uint16_t t_nodeID = 1; t_nodeID <= ZW_MAX_NODES_LR; t_nodeID++)
  {
    mock_calls_clear();

    //nodeID 1-232
    //nodeNr 0-231  for local indexing in arrays
    uint16_t t_nodeNr = t_nodeID - 1;

    EX_NVM_NODEINFO t_nodeInfo;


    t_nodeInfo.capability = CAPABILITY_VAL;
    t_nodeInfo.security   = SECURITY_VAL;
    t_nodeInfo.reserved   = RESERVED_VAL;
    t_nodeInfo.generic    = (TEST_VAL4 & 0xFF); // "generic" is 8 bit. Wrap around after reaching 255 Long Range nodes
    t_nodeInfo.specific   = (TEST_VAL5 & 0xFF); // "specific" is 8 bit . Wrap around after reaching 255 Long Range nodes

    uint8_t packedNodeInfo[SIZEOF_SNODINFOSTORAGE_LR];
    packedNodeInfo[0] = 0x7F;  //All flags in byte 0 are set
    packedNodeInfo[1] = t_nodeInfo.generic;
    packedNodeInfo[2] = t_nodeInfo.specific;

    zpal_status_t statusGetObject = ZPAL_STATUS_OK;

    if(0 == (t_nodeNr % NODEINFOS_PER_FILE_LR))
    {
      memset(&nodeInfoEntryWrite_LR, 0xFF, sizeof(nodeInfoEntryWrite_LR));
      statusGetObject = ZPAL_STATUS_FAIL;
    }

    mock_call_expect(TO_STR(zpal_nvm_get_object_size), &pMock);
    pMock->return_code.value = statusGetObject;
    pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
    pMock->expect_arg[ARG1].value = 0x00800 + t_nodeNr/NODEINFOS_PER_FILE_LR;
    pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;

    if(ZPAL_STATUS_OK == statusGetObject)
    {
      mock_call_expect(TO_STR(zpal_nvm_read), &pMock);
      pMock->return_code.value = ZPAL_STATUS_OK;
      pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
      pMock->expect_arg[ARG1].value = 0x00800 + t_nodeNr/NODEINFOS_PER_FILE_LR;
      pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
      pMock->output_arg[ARG2].pointer = &nodeInfoEntryWrite_LR;
      pMock->expect_arg[ARG3].value = SIZE_OF_NODEINFOFILE_LR;
    }

    uint32_t offset_in_file = (t_nodeNr % NODEINFOS_PER_FILE_LR) * SIZEOF_SNODINFOSTORAGE_LR;
    memcpy(&nodeInfoEntryWrite_LR[offset_in_file], packedNodeInfo, SIZEOF_SNODINFOSTORAGE_LR);

    mock_call_expect(TO_STR(zpal_nvm_write), &pMock);
    pMock->return_code.value = ZPAL_STATUS_OK;
    pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
    pMock->expect_arg[ARG1].value = 0x00800 + t_nodeNr/NODEINFOS_PER_FILE_LR;
    pMock->expect_arg[ARG2].p = &nodeInfoEntryWrite_LR;
    pMock->expect_arg[ARG3].value = SIZE_OF_NODEINFOFILE_LR;

    uint8_t mockFileMap[sizeof(LR_NODE_MASK_TYPE)];
    memset(mockFileMap, 0, sizeof(mockFileMap));
    for(uint32_t i=0; i<sizeof(mockFileMap); i++)
    {
      if(i < t_nodeNr/8)
      {
        mockFileMap[i] = 0xFF;
      }
      else if(i == t_nodeNr/8)
      {
        mockFileMap[i] = ~(0xFF00 >> (7 - t_nodeNr%8));
      }
      else
      {
        mockFileMap[i] = 0x00;
      }
    }
    mock_call_expect(TO_STR(zpal_nvm_write), &pMock);
    pMock->return_code.value = ZPAL_STATUS_OK;
    pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
    pMock->expect_arg[ARG1].value = 0x0000C; //FILE_ID_LRANGE_NODE_EXIST
    pMock->expect_arg[ARG2].p = mockFileMap;
    pMock->expect_arg[ARG3].value = sizeof(LR_NODE_MASK_TYPE);

    CtrlStorageSetNodeInfo(LOWEST_LONG_RANGE_NODE_ID + t_nodeNr, &t_nodeInfo);

    mock_call_expect(TO_STR(zpal_nvm_read_object_part), &pMock);
    pMock->return_code.value = ZPAL_STATUS_OK;
    pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
    pMock->expect_arg[ARG1].value = 0x00800 + t_nodeNr/NODEINFOS_PER_FILE_LR;
    pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
    pMock->output_arg[ARG2].pointer = &nodeInfoEntryWrite_LR[SIZE_OF_SNODEINFOLONGRANGE * (t_nodeNr % NODEINFOS_PER_FILE_LR)];
    pMock->expect_arg[ARG3].value = SIZE_OF_SNODEINFOLONGRANGE * (t_nodeNr % NODEINFOS_PER_FILE_LR);
    pMock->expect_arg[ARG4].value = SIZE_OF_SNODEINFOLONGRANGE;

    EX_NVM_NODEINFO t_nodeInfoRead;
    CtrlStorageGetNodeInfo(LOWEST_LONG_RANGE_NODE_ID + t_nodeNr, &t_nodeInfoRead);

    TEST_ASSERT_MESSAGE(CAPABILITY_VAL == t_nodeInfoRead.capability, "TDU 1.7 LR");
    TEST_ASSERT_MESSAGE(SECURITY_VAL   == t_nodeInfoRead.security,   "TDU 1.7 LR");
    TEST_ASSERT_MESSAGE(RESERVED_VAL   == t_nodeInfoRead.reserved,   "TDU 1.7 LR");
    TEST_ASSERT_MESSAGE((TEST_VAL4 & 0xFF)  == t_nodeInfoRead.generic,    "TDU 1.7 LR");
    TEST_ASSERT_MESSAGE((TEST_VAL5 & 0xFF)  == t_nodeInfoRead.specific,   "TDU 1.7 LR");

    mock_calls_verify();
  }


  for (uint8_t t_nodeID = 1; t_nodeID <= ZW_MAX_NODES; t_nodeID++)
  {
    mock_calls_clear();

    //nodeID 1-232
    //nodeNr 0-231  for local indexing in arrays
    uint16_t t_nodeNr = t_nodeID - 1;

    uint8_t nodeInfoEntry[SIZE_OF_NODEINFOFILE];
    memset(nodeInfoEntry, TEST_VAL1, sizeof(nodeInfoEntry));

    mock_call_expect(TO_STR(zpal_nvm_read_object_part), &pMock);
    pMock->return_code.value = ZPAL_STATUS_OK;
    pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
    pMock->expect_arg[ARG1].value = 0x00200 + t_nodeNr/NODEINFOS_PER_FILE;
    pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
    pMock->output_arg[ARG2].p = &nodeInfoEntry[SIZE_OF_SNODEINFOSTORAGE * (t_nodeNr % NODEINFOS_PER_FILE)];
    pMock->expect_arg[ARG3].value = SIZE_OF_SNODEINFOSTORAGE * (t_nodeNr % NODEINFOS_PER_FILE);
    pMock->expect_arg[ARG4].value = sizeof(EX_NVM_NODEINFO);

     // TDU 1.13 test we can delete existing node info file
    EX_NVM_NODEINFO t_nodeInfo;
    CtrlStorageGetNodeInfo(t_nodeID, &t_nodeInfo);
    TEST_ASSERT_EQUAL_HEX8(TEST_VAL1, t_nodeInfo.capability);

    mock_call_expect(TO_STR(zpal_nvm_write), &pMock);
    pMock->return_code.value = ZPAL_STATUS_OK;
    pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
    pMock->expect_arg[ARG1].value = 0x00006;  //FILE_ID_APP_ROUTE_LOCK_FLAG
    pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
    pMock->expect_arg[ARG3].value = 29;  //sizeof(NODE_MASK_TYPE)

    mock_call_expect(TO_STR(zpal_nvm_write), &pMock);
    pMock->return_code.value = ZPAL_STATUS_OK;
    pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
    pMock->expect_arg[ARG1].value = 0x00007;  //FILE_ID_ROUTE_SLAVE_SUC_FLAG
    pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
    pMock->expect_arg[ARG3].value = 29;  //sizeof(NODE_MASK_TYPE)

    mock_call_expect(TO_STR(zpal_nvm_write), &pMock);
    pMock->return_code.value = ZPAL_STATUS_OK;
    pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
    pMock->expect_arg[ARG1].value = 0x00008;  //FILE_ID_SUC_PENDING_UPDATE_FLAG
    pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
    pMock->expect_arg[ARG3].value = 29;  //sizeof(NODE_MASK_TYPE)

    mock_call_expect(TO_STR(zpal_nvm_write), &pMock);
    pMock->return_code.value = ZPAL_STATUS_OK;
    pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
    pMock->expect_arg[ARG1].value = 0x00009;  //FILE_ID_BRIDGE_NODE_FLAG
    pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
    pMock->expect_arg[ARG3].value = 29;  //sizeof(NODE_MASK_TYPE)

    mock_call_expect(TO_STR(zpal_nvm_write), &pMock);
    pMock->return_code.value = ZPAL_STATUS_OK;
    pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
    pMock->expect_arg[ARG1].value = 0x0000A;  //FILE_ID_PENDING_DISCOVERY_FLAG
    pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
    pMock->expect_arg[ARG3].value = 29;  //sizeof(NODE_MASK_TYPE)

    mock_call_expect(TO_STR(zpal_nvm_write), &pMock);
    pMock->return_code.value = ZPAL_STATUS_OK;
    pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
    pMock->expect_arg[ARG1].value = 0x00005;  //FILE_ID_NODE_STORAGE_EXIST
    pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
    pMock->expect_arg[ARG3].value = 29;  //sizeof(NODE_MASK_TYPE)

    if((NODEINFOS_PER_FILE - 1) == (t_nodeNr % NODEINFOS_PER_FILE))
    {
      mock_call_expect(TO_STR(zpal_nvm_erase_object), &pMock);
      pMock->return_code.value = ZPAL_STATUS_OK;
      pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
      pMock->expect_arg[ARG1].value = 0x00200 + t_nodeNr/NODEINFOS_PER_FILE;
    }

    CtrlStorageRemoveNodeInfo(t_nodeID, false);
    CtrlStorageGetNodeInfo(t_nodeID, &t_nodeInfo);
    TEST_ASSERT_EQUAL_HEX8(0, t_nodeInfo.capability);

    mock_calls_verify();
  }

  mock_calls_clear();

  //Tests for Z-Wave Long Range
  for (uint16_t t_nodeID = 1; t_nodeID <= ZW_MAX_NODES_LR ; t_nodeID++)
  {
    //nodeID 1-232
    //nodeNr 0-231  for local indexing in arrays
    uint16_t t_nodeNr = t_nodeID - 1;

    uint8_t mockFileMap[sizeof(LR_NODE_MASK_TYPE)];
    memset(mockFileMap, 0xFF, sizeof(mockFileMap));
    for(uint32_t i=0; i<sizeof(mockFileMap); i++)
    {
      if(i < t_nodeNr/8)
      {
        mockFileMap[i] = 0x00;
      }
      else if(i == t_nodeNr/8)
      {
        mockFileMap[i] = ~(0x00FF >> (7 - t_nodeNr%8));
      }
      else
      {
        mockFileMap[i] = 0xFF;
      }
    }
    mock_call_expect(TO_STR(zpal_nvm_write), &pMock);
    pMock->return_code.value = ZPAL_STATUS_OK;
    pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
    pMock->expect_arg[ARG1].value = 0x0000C; //FILE_ID_NODE_STORAGE_EXIST
    pMock->expect_arg[ARG2].p = mockFileMap;
    pMock->expect_arg[ARG3].value = sizeof(LR_NODE_MASK_TYPE);

    if((NODEINFOS_PER_FILE_LR - 1) == (t_nodeNr % NODEINFOS_PER_FILE_LR) ||
       (ZW_MAX_NODES_LR - 1) == t_nodeNr)
    {
      mock_call_expect(TO_STR(zpal_nvm_erase_object), &pMock);
      pMock->return_code.value = ZPAL_STATUS_OK;
      pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
      pMock->expect_arg[ARG1].value = 0x00800 + t_nodeNr/NODEINFOS_PER_FILE_LR;
    }

    CtrlStorageRemoveNodeInfo(LOWEST_LONG_RANGE_NODE_ID + t_nodeNr, false);
  }

  mock_calls_verify();

  // TDU 1.14 test changing a bit in the bitmask field in nodeinfo don't effect other bits
  {
    mock_calls_clear();
    EX_NVM_NODEINFO t_nodeInfo;
    uint8_t t_nodeID = 6;

    mock_call_expect(TO_STR(zpal_nvm_write), &pMock);
    pMock->return_code.value = ZPAL_STATUS_OK;
    pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
    pMock->expect_arg[ARG1].value = 0x00200 + (t_nodeID - 1)/NODEINFOS_PER_FILE;
    pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
    pMock->expect_arg[ARG3].value = SIZE_OF_NODEINFOFILE;

    mock_call_expect(TO_STR(zpal_nvm_write), &pMock);
    pMock->return_code.value = ZPAL_STATUS_OK;
    pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
    pMock->expect_arg[ARG1].value = 0x00005;  //FILE_ID_NODE_STORAGE_EXIST
    pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
    pMock->expect_arg[ARG3].value = 29;  //sizeof(NODE_MASK_TYPE)

    CtrlStorageSetNodeInfo(t_nodeID, &t_nodeInfo);


    mock_call_expect(TO_STR(zpal_nvm_write), &pMock);
    pMock->return_code.value = ZPAL_STATUS_OK;
    pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
    pMock->expect_arg[ARG1].value = 0x00008;  //FILE_ID_SUC_PENDING_UPDATE_FLAG
    pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
    pMock->expect_arg[ARG3].value = 29;  //sizeof(NODE_MASK_TYPE)

    CtrlStorageChangeNodeInPendingUpdateTable(t_nodeID, true);
    TEST_ASSERT_TRUE(CtrlStorageIsNodeInPendingUpdateTable(t_nodeID));
    TEST_ASSERT_TRUE(CtrlStorageGetRoutingSlaveSucUpdateFlag(t_nodeID));
    TEST_ASSERT_TRUE(!CtrlStorageGetAppRouteLockFlag(t_nodeID));
    TEST_ASSERT_TRUE(!CtrlStorageGetBridgeNodeFlag(t_nodeID));
    TEST_ASSERT_TRUE(!CtrlStorageGetPendingDiscoveryStatus(t_nodeID));

    mock_call_expect(TO_STR(zpal_nvm_write), &pMock);
    pMock->return_code.value = ZPAL_STATUS_OK;
    pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
    pMock->expect_arg[ARG1].value = 0x00008;  //FILE_ID_SUC_PENDING_UPDATE_FLAG
    pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
    pMock->expect_arg[ARG3].value = 29;  //sizeof(NODE_MASK_TYPE)

    CtrlStorageChangeNodeInPendingUpdateTable(t_nodeID, false);


    mock_call_expect(TO_STR(zpal_nvm_write), &pMock);
    pMock->return_code.value = ZPAL_STATUS_OK;
    pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
    pMock->expect_arg[ARG1].value = 0x00007;  //FILE_ID_ROUTE_SLAVE_SUC_FLAG
    pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
    pMock->expect_arg[ARG3].value = 29;  //sizeof(NODE_MASK_TYPE)

    CtrlStorageSetRoutingSlaveSucUpdateFlag(t_nodeID, false, true);
    TEST_ASSERT_TRUE(!CtrlStorageIsNodeInPendingUpdateTable(t_nodeID));
    TEST_ASSERT_TRUE(!CtrlStorageGetRoutingSlaveSucUpdateFlag(t_nodeID));
    TEST_ASSERT_TRUE(!CtrlStorageGetAppRouteLockFlag(t_nodeID));
    TEST_ASSERT_TRUE(!CtrlStorageGetBridgeNodeFlag(t_nodeID));
    TEST_ASSERT_TRUE(!CtrlStorageGetPendingDiscoveryStatus(t_nodeID));

    mock_call_expect(TO_STR(zpal_nvm_write), &pMock);
    pMock->return_code.value = ZPAL_STATUS_OK;
    pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
    pMock->expect_arg[ARG1].value = 0x00007;  //FILE_ID_ROUTE_SLAVE_SUC_FLAG
    pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
    pMock->expect_arg[ARG3].value = 29;  //sizeof(NODE_MASK_TYPE)

    CtrlStorageSetRoutingSlaveSucUpdateFlag(t_nodeID, true, true);


    mock_call_expect(TO_STR(zpal_nvm_write), &pMock);
    pMock->return_code.value = ZPAL_STATUS_OK;
    pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
    pMock->expect_arg[ARG1].value = 0x00006;  //FILE_ID_APP_ROUTE_LOCK_FLAG
    pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
    pMock->expect_arg[ARG3].value = 29;  //sizeof(NODE_MASK_TYPE)

    CtrlStorageSetAppRouteLockFlag(t_nodeID, true);
    TEST_ASSERT_TRUE(!CtrlStorageIsNodeInPendingUpdateTable(t_nodeID));
    TEST_ASSERT_TRUE(CtrlStorageGetRoutingSlaveSucUpdateFlag(t_nodeID));
    TEST_ASSERT_TRUE(CtrlStorageGetAppRouteLockFlag(t_nodeID));
    TEST_ASSERT_TRUE(!CtrlStorageGetBridgeNodeFlag(t_nodeID));
    TEST_ASSERT_TRUE(!CtrlStorageGetPendingDiscoveryStatus(t_nodeID));

    mock_call_expect(TO_STR(zpal_nvm_write), &pMock);
    pMock->return_code.value = ZPAL_STATUS_OK;
    pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
    pMock->expect_arg[ARG1].value = 0x00006;  //FILE_ID_APP_ROUTE_LOCK_FLAG
    pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
    pMock->expect_arg[ARG3].value = 29;  //sizeof(NODE_MASK_TYPE)

    CtrlStorageSetAppRouteLockFlag(t_nodeID, false);


    mock_call_expect(TO_STR(zpal_nvm_write), &pMock);
    pMock->return_code.value = ZPAL_STATUS_OK;
    pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
    pMock->expect_arg[ARG1].value = 0x00009;  //FILE_ID_BRIDGE_NODE_FLAG
    pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
    pMock->expect_arg[ARG3].value = 29;  //sizeof(NODE_MASK_TYPE)

    CtrlStorageSetBridgeNodeFlag(t_nodeID, true, true);
    TEST_ASSERT_TRUE(!CtrlStorageIsNodeInPendingUpdateTable(t_nodeID));
    TEST_ASSERT_TRUE(CtrlStorageGetRoutingSlaveSucUpdateFlag(t_nodeID));
    TEST_ASSERT_TRUE(!CtrlStorageGetAppRouteLockFlag(t_nodeID));
    TEST_ASSERT_TRUE(CtrlStorageGetBridgeNodeFlag(t_nodeID));
    TEST_ASSERT_TRUE(!CtrlStorageGetPendingDiscoveryStatus(t_nodeID));

    mock_call_expect(TO_STR(zpal_nvm_write), &pMock);
    pMock->return_code.value = ZPAL_STATUS_OK;
    pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
    pMock->expect_arg[ARG1].value = 0x00009;  //FILE_ID_BRIDGE_NODE_FLAG
    pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
    pMock->expect_arg[ARG3].value = 29;  //sizeof(NODE_MASK_TYPE)

    CtrlStorageSetBridgeNodeFlag(t_nodeID, false, true);


    mock_call_expect(TO_STR(zpal_nvm_write), &pMock);
    pMock->return_code.value = ZPAL_STATUS_OK;
    pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
    pMock->expect_arg[ARG1].value = 0x0000A;  //FILE_ID_PENDING_DISCOVERY_FLAG
    pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
    pMock->expect_arg[ARG3].value = 29;  //sizeof(NODE_MASK_TYPE)

    CtrlStorageSetPendingDiscoveryStatus(t_nodeID, true);
    TEST_ASSERT_TRUE(!CtrlStorageIsNodeInPendingUpdateTable(t_nodeID));
    TEST_ASSERT_TRUE(CtrlStorageGetRoutingSlaveSucUpdateFlag(t_nodeID));
    TEST_ASSERT_TRUE(!CtrlStorageGetAppRouteLockFlag(t_nodeID));
    TEST_ASSERT_TRUE(!CtrlStorageGetBridgeNodeFlag(t_nodeID));
    TEST_ASSERT_TRUE(CtrlStorageGetPendingDiscoveryStatus(t_nodeID));

    mock_call_expect(TO_STR(zpal_nvm_write), &pMock);
    pMock->return_code.value = ZPAL_STATUS_OK;
    pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
    pMock->expect_arg[ARG1].value = 0x0000A;  //FILE_ID_PENDING_DISCOVERY_FLAG
    pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
    pMock->expect_arg[ARG3].value = 29;  //sizeof(NODE_MASK_TYPE)

    CtrlStorageSetPendingDiscoveryStatus(t_nodeID, false);

    mock_calls_verify();
  }
}

 // Testing Test writing and reading To the preffered repeaters file
void test_ControllerStorage_repeaters(void)
{

  mock_calls_clear();
//--------------------------------------------------------------------------------------------------------
// File system init start
//--------------------------------------------------------------------------------------------------------
  uint8_t dummy;
  zpal_nvm_handle_t pFileSystem = &dummy;

//--------------------------------------------------------------------------------------------------------
// -------------------------------------------------------filesysteminit end
//--------------------------------------------------------------------------------------------------------

  // initialize controller network
  mock_t * pMockNvmFileSystemRegister;
  mock_call_expect(TO_STR(NvmFileSystemRegister), &pMockNvmFileSystemRegister);
  pMockNvmFileSystemRegister->return_code.pointer = pFileSystem;
  pMockNvmFileSystemRegister->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;

  mock_t * pMock;
  mock_call_expect(TO_STR(zpal_nvm_read), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00000; //FILE_ID_ZW_VERSION
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG3].value = 4; //sizeof(uint32_t)
  //Set file system version to the latest to avoid triggering migration.
  uint32_t version = m_current_ZW_Version;
  pMock->output_arg[ARG2].p = (uint8_t *)&version;

  // FileSet - allows FileSystemCaretaker to validate required files
  static const SObjectDescriptor t_aFileDescriptors[] = {
    { .ObjectKey = 0x00002,   .iDataSize = sizeof(NODE_MASK_TYPE)   },  //FILE_ID_PREFERREDREPEATERS
    { .ObjectKey = 0x00004,   .iDataSize = CONTROLLERINFO_FILE_SIZE },  //FILE_ID_CONTROLLERINFO
    { .ObjectKey = 0x00005,   .iDataSize = sizeof(NODE_MASK_TYPE)   },  //FILE_ID_NODE_STORAGE_EXIST
    { .ObjectKey = 0x0000B,   .iDataSize = sizeof(NODE_MASK_TYPE)   },  //FILE_ID_NODE_ROUTECACHE_EXIST
    { .ObjectKey = 0x0000C,   .iDataSize = sizeof(LR_NODE_MASK_TYPE)},  //FILE_ID_LRANGE_NODE_EXIST
    { .ObjectKey = 0x00006,   .iDataSize = sizeof(NODE_MASK_TYPE)   },  //FILE_ID_APP_ROUTE_LOCK_FLAG
    { .ObjectKey = 0x00007,   .iDataSize = sizeof(NODE_MASK_TYPE)   },  //FILE_ID_ROUTE_SLAVE_SUC_FLAG
    { .ObjectKey = 0x00008,   .iDataSize = sizeof(NODE_MASK_TYPE)   },  //FILE_ID_SUC_PENDING_UPDATE_FLAG
    { .ObjectKey = 0x00009,   .iDataSize = sizeof(NODE_MASK_TYPE)   },  //FILE_ID_BRIDGE_NODE_FLAG
    { .ObjectKey = 0x0000A,   .iDataSize = sizeof(NODE_MASK_TYPE)   },  //FILE_ID_PENDING_DISCOVERY_FLAG
   };

  SObjectSet tFileSet = {
    .pFileSystem = pFileSystem,
    .iObjectCount = 10, //sizeof_array(t_aFileDescriptors)
    .pObjectDescriptors = t_aFileDescriptors
  };

  ECaretakerStatus returnFileStatus[] = {ECTKR_STATUS_SUCCESS,
                                          ECTKR_STATUS_UNABLE_TO_AQUIRE_HANDLE,
                                          ECTKR_STATUS_SIZE_MISMATCH,
                                          ECTKR_STATUS_SIZE_MISMATCH,
                                          ECTKR_STATUS_SUCCESS,
                                          ECTKR_STATUS_SUCCESS,
                                          ECTKR_STATUS_SUCCESS,
                                          ECTKR_STATUS_SUCCESS,
                                          ECTKR_STATUS_SUCCESS,
                                          ECTKR_STATUS_SUCCESS,
                                          ECTKR_STATUS_SUCCESS,
                                          ECTKR_STATUS_SUCCESS,
                                          ECTKR_STATUS_SUCCESS
  };

  mock_call_expect(TO_STR(NVMCaretakerVerifySet), &pMock);
  pMock->return_code.value = ECTKR_STATUS_FILESYS_MISMATCH;  //return that no file was found
  pMock->expect_arg[ARG0].p = &tFileSet;
  pMock->compare_rule_arg[ARG1] = COMPARE_NOT_NULL;
  pMock->output_arg[ARG1].p = &returnFileStatus;

  //Verify that FILE_ID_CONTROLLERINFO is written with default values
  mock_call_expect(TO_STR(zpal_nvm_write), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00004; //FILE_ID_CONTROLLERINFO
  pMock->expect_arg[ARG2].pointer = (uint8_t *)&defaultControllerInfo;
  pMock->expect_arg[ARG3].value = CONTROLLERINFO_FILE_SIZE; //sizeof(SControllerInfo)

  //Verify that FILE_ID_SLAVE_FILE_MAP is read and appended
  size_t oldFileSize = 24;
  mock_call_expect(TO_STR(zpal_nvm_get_object_size), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00005; //FILE_ID_NODE_STORAGE_EXIST
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
  pMock->output_arg[ARG2].pointer = &oldFileSize;

  uint8_t presentNodeMask[24];
  memset(presentNodeMask, 0xFA, 24);

  //Verify that FILE_ID_SLAVE_STORAGE_EXIST is read
  mock_call_expect(TO_STR(zpal_nvm_read), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00005;  //FILE_ID_NODE_STORAGE_EXIST
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
  pMock->output_arg[ARG2].p = presentNodeMask;
  pMock->expect_arg[ARG3].value = 29;  //sizeof(NODE_MASK_TYPE)

  //Verify that FILE_ID_SLAVE_STORAGE_EXIST is read
  mock_call_expect(TO_STR(zpal_nvm_write), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00005;  //FILE_ID_NODE_STORAGE_EXIST
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
  pMock->output_arg[ARG2].p = presentNodeMask;
  pMock->expect_arg[ARG3].value = 29;  //sizeof(NODE_MASK_TYPE)

  //Verify that FILE_ID_NODE_ROUTECACHE_EXIST is read and appended
  oldFileSize = 24;
  mock_call_expect(TO_STR(zpal_nvm_get_object_size), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x0000B; //FILE_ID_NODE_ROUTECACHE_EXIST
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
  pMock->output_arg[ARG2].pointer = &oldFileSize;

  memset(presentNodeMask, 0xFA, 24);

  //Verify that FILE_ID_NODE_ROUTECACHE_EXIST is read
  mock_call_expect(TO_STR(zpal_nvm_read), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x0000B;  //FILE_ID_NODE_ROUTECACHE_EXIST
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
  pMock->output_arg[ARG2].p = presentNodeMask;
  pMock->expect_arg[ARG3].value = 29;  //sizeof(NODE_MASK_TYPE)

  //Verify that FILE_ID_NODE_ROUTECACHE_EXIST is read
  mock_call_expect(TO_STR(zpal_nvm_write), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x0000B;  //FILE_ID_NODE_ROUTECACHE_EXIST
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
  pMock->output_arg[ARG2].p = presentNodeMask;
  pMock->expect_arg[ARG3].value = 29;  //sizeof(NODE_MASK_TYPE)

  uint8_t zeroedNodeMask[29];
  memset(zeroedNodeMask, 0, 29);

  //Verify that FILE_ID_SLAVE_STORAGE_EXIST is read
  mock_call_expect(TO_STR(zpal_nvm_read), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00005;  //FILE_ID_NODE_STORAGE_EXIST
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
  pMock->output_arg[ARG2].p = zeroedNodeMask;
  pMock->expect_arg[ARG3].value = 29;  //sizeof(NODE_MASK_TYPE)

  memset(zeroedNodeMask, 0, 29);

  //Verify that FILE_ID_NODE_ROUTECACHE_EXIST is read
  mock_call_expect(TO_STR(zpal_nvm_read), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x0000B;  //FILE_ID_NODE_ROUTECACHE_EXIST
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
  pMock->output_arg[ARG2].p = zeroedNodeMask;
  pMock->expect_arg[ARG3].value = 29;  //sizeof(NODE_MASK_TYPE)

  uint8_t zeroedLRNodeMask[sizeof(LR_NODE_MASK_TYPE)] = {0};

  //Verify that FILE_ID_LRANGE_NODE_EXIST is read
  mock_call_expect(TO_STR(zpal_nvm_read), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x0000C;  //FILE_ID_LRANGE_NODE_EXIST
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
  pMock->output_arg[ARG2].p = zeroedLRNodeMask;
  pMock->expect_arg[ARG3].value = sizeof(LR_NODE_MASK_TYPE);

  //Verify that FILE_ID_APP_ROUTE_LOCK_FLAGT is read
  mock_call_expect(TO_STR(zpal_nvm_read), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00006;  //FILE_ID_APP_ROUTE_LOCK_FLAG
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
  pMock->output_arg[ARG2].p = zeroedNodeMask;
  pMock->expect_arg[ARG3].value = 29;  //sizeof(NODE_MASK_TYPE)

  //Verify that FILE_ID_ROUTE_SLAVE_SUC_FLAG is read
  mock_call_expect(TO_STR(zpal_nvm_read), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00007;  //FILE_ID_ROUTE_SLAVE_SUC_FLAG
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
  pMock->output_arg[ARG2].p = zeroedNodeMask;
  pMock->expect_arg[ARG3].value = 29;  //sizeof(NODE_MASK_TYPE)

  //Verify that FILE_ID_SUC_PENDING_UPDATE_FLAG is read
  mock_call_expect(TO_STR(zpal_nvm_read), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00008;  //FILE_ID_SUC_PENDING_UPDATE_FLAG
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
  pMock->output_arg[ARG2].p = zeroedNodeMask;
  pMock->expect_arg[ARG3].value = 29;  //sizeof(NODE_MASK_TYPE)

  //Verify that FILE_ID_BRIDGE_NODE_FLAG is read
  mock_call_expect(TO_STR(zpal_nvm_read), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00009;  //FILE_ID_BRIDGE_NODE_FLAG
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
  pMock->output_arg[ARG2].p = zeroedNodeMask;
  pMock->expect_arg[ARG3].value = 29;  //sizeof(NODE_MASK_TYPE)

  //Verify that FILE_ID_PENDING_DISCOVERY_FLAG is read
  mock_call_expect(TO_STR(zpal_nvm_read), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x0000A;  //FILE_ID_PENDING_DISCOVERY_FLAG
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
  pMock->output_arg[ARG2].p = zeroedNodeMask;
  pMock->expect_arg[ARG3].value = 29;  //sizeof(NODE_MASK_TYPE)

  CtrlStorageInit();

  SSyncEvent * pRegisterEvent = (SSyncEvent*)pMockNvmFileSystemRegister->actual_arg[ARG0].pointer;
  TEST_ASSERT_NOT_NULL(pRegisterEvent);
  TEST_ASSERT_NOT_NULL(pRegisterEvent->uFunctor.pFunction);
  TEST_ASSERT_NULL(pRegisterEvent->pObject);

  NODE_MASK_TYPE preRepeater;

#ifdef CANT_BE_TESTED_USING_MOCKS
  // TDU 2.1 test that The prefered repeters file exist on power up
  DPRINT("TDU 2.1 start \r\n");
  CtrlStorageGetPreferredRepeaters(&preRepeater);
  for (uint8_t index = 0; index < sizeof(NODE_MASK_TYPE); index++)
  {
    TEST_ASSERT_EQUAL(0, preRepeater[index]);
  }
#endif

  // TDU 2.2 test that data written to preferred repeater file.
  DPRINT("TDU 2.2 start \r\n");

  for (uint8_t index = 0; index < sizeof(NODE_MASK_TYPE); index++)
  {
    preRepeater[index] = (index + 0x42);  //Dummy values used to test read/write
  }

  mock_call_expect(TO_STR(zpal_nvm_write), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00002;  //FILE_ID_PREFERREDREPEATERS
  pMock->expect_arg[ARG2].p = preRepeater;
  pMock->expect_arg[ARG3].value = 29;  //sizeof(NODE_MASK_TYPE)

  CtrlStorageSetPreferredRepeaters(&preRepeater);

  mock_call_expect(TO_STR(zpal_nvm_read), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00002;  //FILE_ID_PREFERREDREPEATERS
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
  pMock->output_arg[ARG2].p = preRepeater;
  pMock->expect_arg[ARG3].value = 29;  //sizeof(NODE_MASK_TYPE)

  NODE_MASK_TYPE preRepeater2;
  CtrlStorageGetPreferredRepeaters(&preRepeater2);
  for (uint8_t index = 0; index < sizeof(NODE_MASK_TYPE); index++)
  {
    TEST_ASSERT_EQUAL((index + 0x42), preRepeater2[index]);
  }

  mock_calls_verify();
}

#define CTRL_INFO_TEST_VAL   0x42

#define CTRL_INFO_TEST_VAL1   (CTRL_INFO_TEST_VAL + 10)
#define CTRL_INFO_TEST_VAL2   (CTRL_INFO_TEST_VAL + 20)
#define CTRL_INFO_TEST_VAL3   (CTRL_INFO_TEST_VAL + 30)
#define CTRL_INFO_TEST_VAL4   (CTRL_INFO_TEST_VAL + 40)
#define CTRL_INFO_TEST_VAL5   (CTRL_INFO_TEST_VAL + 50)
#define CTRL_INFO_TEST_VAL6   (CTRL_INFO_TEST_VAL + 60)
#define CTRL_INFO_TEST_VAL7   (CTRL_INFO_TEST_VAL + 70)

 // Testing Test writing and reading To the controller information file
void test_ControllerStorage_controller_info(void)
{
  mock_calls_clear();

//--------------------------------------------------------------------------------------------------------
// File system init start
//--------------------------------------------------------------------------------------------------------
  uint8_t dummy;
  zpal_nvm_handle_t pFileSystem = &dummy;
//--------------------------------------------------------------------------------------------------------
// -------------------------------------------------------filesyseminit end
//--------------------------------------------------------------------------------------------------------

  // initialize controller network
  mock_t * pMockNvmFileSystemRegister;
  mock_call_use_as_stub(TO_STR(NvmFileSystemInit));
  mock_call_expect(TO_STR(NvmFileSystemRegister), &pMockNvmFileSystemRegister);
  pMockNvmFileSystemRegister->return_code.pointer = pFileSystem;
  pMockNvmFileSystemRegister->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;

  mock_t * pMock;
  mock_call_expect(TO_STR(zpal_nvm_read), &pMock);
  pMock->return_code.value = ZPAL_STATUS_FAIL;  //Make zpal_nvm_read() return error code to test that file system formats if file not found
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00000; //FILE_ID_FILESYS_VERSION
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG3].value = 4; //sizeof(uint32_t)

  //Verify that the file system is formated because the file was not found
  mock_call_expect(TO_STR(NvmFileSystemFormat), &pMock);
  pMock->return_code.value = true;

  mock_t * pMockCaretakerVerify;
  mock_call_expect(TO_STR(NVMCaretakerVerifySet), &pMockCaretakerVerify);
  pMockCaretakerVerify->return_code.value = ECTKR_STATUS_SUCCESS;  //return that the files were found
  pMockCaretakerVerify->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMockCaretakerVerify->compare_rule_arg[ARG1] = COMPARE_NOT_NULL;

  uint32_t file_id[8] = {0x00005,  //FILE_ID_NODE_STORAGE_EXIST
                         0x0000B,  //FILE_ID_NODE_ROUTECACHE_EXIST
                         0x0000C,  //FILE_ID_LRANGE_NODE_EXIST
                         0x00006,  //FILE_ID_APP_ROUTE_LOCK_FLAG
                         0x00007,  //FILE_ID_ROUTE_SLAVE_SUC_FLAG
                         0x00008,  //FILE_ID_SUC_PENDING_UPDATE_FLAG
                         0x00009,  //FILE_ID_BRIDGE_NODE_FLAG
                         0x0000A   //FILE_ID_PENDING_DISCOVERY_FLAG
                        };

  for(uint32_t i=0; i<(sizeof(file_id)/sizeof(uint32_t)); i++)
  {
    mock_call_expect(TO_STR(zpal_nvm_read), &pMock);
    pMock->return_code.value = ZPAL_STATUS_OK;
    pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
    pMock->expect_arg[ARG1].value = file_id[i]; //FILE_ID_
    pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
    if(0x0000C == file_id[i])  //Special nodemask for FILE_ID_LRANGE_NODE_EXIST file
    {
      pMock->expect_arg[ARG3].value = sizeof(LR_NODE_MASK_TYPE);
    }
    else
    {
      pMock->expect_arg[ARG3].value = sizeof(NODE_MASK_TYPE);
    }
  }

  CtrlStorageInit();

  SSyncEvent * pRegisterEvent = (SSyncEvent*)pMockNvmFileSystemRegister->actual_arg[ARG0].pointer;
  TEST_ASSERT_NOT_NULL(pRegisterEvent);
  TEST_ASSERT_NOT_NULL(pRegisterEvent->uFunctor.pFunction);
  TEST_ASSERT_NULL(pRegisterEvent->pObject);


  SControllerInfo readInfo = {
                              .HomeID = {0,0,0,0},
                              .NodeID = 1,
                              .StaticControllerNodeId = 0,
                              .LastUsedNodeId_LR = 0x0101,
                              .LastUsedNodeId = 1,
                              .SucLastIndex = DefaultControllerInfoSucLastIndex,
                              .MaxNodeId_LR = 0x0101,
                              .MaxNodeId = 1,
                              .ControllerConfiguration = DefaultControllerInfoControllerConfiguration,
                              .ReservedId_LR = 0,
                              .ReservedId = 0,
                              .SystemState = 0,
                              .LongRangeChannelAutoMode = 0
  };

  SControllerInfo writeInfo;

  uint8_t * pReadInfo  = (uint8_t *)&readInfo;
  uint8_t * pWriteInfo = (uint8_t *)&writeInfo;

  mock_call_expect(TO_STR(zpal_nvm_read), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00004; //FILE_ID_CONTROLLERINFO
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
  pMock->output_arg[ARG2].p = pReadInfo;
  pMock->expect_arg[ARG3].value = CONTROLLERINFO_FILE_SIZE; //sizeof(SControllerInfo)

  uint8_t  fieldVal;
  uint16_t fieldVal_16b;
 // TDU 3.1 test controller configuration filed is DefaultControllerInfoControllerConfiguration on startup
  DPRINT("TDU 3.1 start \r\n");
  fieldVal = CtrlStorageGetControllerConfig();
  TEST_ASSERT_EQUAL(DefaultControllerInfoControllerConfiguration, fieldVal);


  mock_call_expect(TO_STR(zpal_nvm_read), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00004; //FILE_ID_CONTROLLERINFO
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
  pMock->output_arg[ARG2].p = pReadInfo;
  pMock->expect_arg[ARG3].value = CONTROLLERINFO_FILE_SIZE; //sizeof(SControllerInfo)

 // TDU 3.2 test StaticControllerNodeId filed in controller information file is zero on startup
  DPRINT("TDU 3.2 start \r\n");
  fieldVal_16b = CtrlStorageGetStaticControllerNodeId();
  TEST_ASSERT_EQUAL(0, fieldVal_16b);


  mock_call_expect(TO_STR(zpal_nvm_read), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00004; //FILE_ID_CONTROLLERINFO
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
  pMock->output_arg[ARG2].p = pReadInfo;
  pMock->expect_arg[ARG3].value = CONTROLLERINFO_FILE_SIZE; //sizeof(SControllerInfo)

// TDU 3.3 test LastUsedNodeId filed in controller information file is 0x01 on startup
  DPRINT("TDU 3.3 start \r\n");
  fieldVal = CtrlStorageGetLastUsedNodeId();
  TEST_ASSERT_EQUAL(1, fieldVal);


  mock_call_expect(TO_STR(zpal_nvm_read), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00004; //FILE_ID_CONTROLLERINFO
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
  pMock->output_arg[ARG2].pointer = pReadInfo;
  pMock->expect_arg[ARG3].value = CONTROLLERINFO_FILE_SIZE; //sizeof(SControllerInfo)

// TDU 3.4 test SucLastIndex filed in controller information file is DefaultControllerInfoSucLastIndex on startup
  DPRINT("TDU 3.4 start \r\n");
  fieldVal = CtrlStorageGetSucLastIndex();
  TEST_ASSERT_EQUAL(DefaultControllerInfoSucLastIndex, fieldVal);


  mock_call_expect(TO_STR(zpal_nvm_read), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00004; //FILE_ID_CONTROLLERINFO
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
  pMock->output_arg[ARG2].pointer = pReadInfo;
  pMock->expect_arg[ARG3].value = CONTROLLERINFO_FILE_SIZE; //sizeof(SControllerInfo)

// TDU 3.6 test MaxNodeId filed in controller information file is 0x01 on startup
  DPRINT("TDU 3.6 start \r\n");
  fieldVal = CtrlStorageGetMaxNodeId();
  TEST_ASSERT_EQUAL(1, fieldVal);


  mock_call_expect(TO_STR(zpal_nvm_read), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00004; //FILE_ID_CONTROLLERINFO
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
  pMock->output_arg[ARG2].pointer = pReadInfo;
  pMock->expect_arg[ARG3].value = CONTROLLERINFO_FILE_SIZE; //sizeof(SControllerInfo)

// TDU 3.7 test ReservedId filed in controller information file is 0 on startup
  DPRINT("TDU 3.7 start \r\n");
  fieldVal = CtrlStorageGetReservedId();
  TEST_ASSERT_EQUAL(0, fieldVal);

  // now set all fileds to a new defualt value
  fieldVal = CTRL_INFO_TEST_VAL;
  memcpy(pWriteInfo, pReadInfo, sizeof(writeInfo));
  writeInfo.ControllerConfiguration = fieldVal;

  mock_call_expect(TO_STR(zpal_nvm_read), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00004; //FILE_ID_CONTROLLERINFO
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
  pMock->output_arg[ARG2].pointer = pReadInfo;
  pMock->expect_arg[ARG3].value = CONTROLLERINFO_FILE_SIZE; //sizeof(SControllerInfo)

  mock_call_expect(TO_STR(zpal_nvm_write), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00004; //FILE_ID_CONTROLLERINFO
  pMock->expect_arg[ARG2].pointer = pWriteInfo;
  pMock->expect_arg[ARG3].value = CONTROLLERINFO_FILE_SIZE; //sizeof(SControllerInfo)

  CtrlStorageSetControllerConfig(fieldVal);

  fieldVal_16b = CTRL_INFO_TEST_VAL1 + (CTRL_INFO_TEST_VAL2 << 8);
  memcpy(pReadInfo, pWriteInfo, sizeof(writeInfo));
  writeInfo.StaticControllerNodeId = fieldVal_16b;

  mock_call_expect(TO_STR(zpal_nvm_read), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00004; //FILE_ID_CONTROLLERINFO
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
  pMock->output_arg[ARG2].pointer = pReadInfo;
  pMock->expect_arg[ARG3].value = CONTROLLERINFO_FILE_SIZE; //sizeof(SControllerInfo)

  mock_call_expect(TO_STR(zpal_nvm_write), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00004; //FILE_ID_CONTROLLERINFO
  pMock->expect_arg[ARG2].pointer = pWriteInfo;
  pMock->expect_arg[ARG3].value = CONTROLLERINFO_FILE_SIZE; //sizeof(SControllerInfo)

  CtrlStorageSetStaticControllerNodeId(fieldVal_16b);


  memcpy(pReadInfo, pWriteInfo, sizeof(writeInfo));
  writeInfo.LastUsedNodeId = fieldVal;

  mock_call_expect(TO_STR(zpal_nvm_read), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00004; //FILE_ID_CONTROLLERINFO
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
  pMock->output_arg[ARG2].pointer = pReadInfo;
  pMock->expect_arg[ARG3].value = CONTROLLERINFO_FILE_SIZE; //sizeof(SControllerInfo)

  mock_call_expect(TO_STR(zpal_nvm_write), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00004; //FILE_ID_CONTROLLERINFO
  pMock->expect_arg[ARG2].pointer = pWriteInfo;
  pMock->expect_arg[ARG3].value = CONTROLLERINFO_FILE_SIZE; //sizeof(SControllerInfo)

  CtrlStorageSetLastUsedNodeId(fieldVal);


  memcpy(pReadInfo, pWriteInfo, sizeof(writeInfo));
  writeInfo.SucLastIndex = fieldVal;

  mock_call_expect(TO_STR(zpal_nvm_read), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00004; //FILE_ID_CONTROLLERINFO
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
  pMock->output_arg[ARG2].pointer = pReadInfo;
  pMock->expect_arg[ARG3].value = CONTROLLERINFO_FILE_SIZE; //sizeof(SControllerInfo)

  mock_call_expect(TO_STR(zpal_nvm_write), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00004; //FILE_ID_CONTROLLERINFO
  pMock->expect_arg[ARG2].pointer = pWriteInfo;
  pMock->expect_arg[ARG3].value = CONTROLLERINFO_FILE_SIZE; //sizeof(SControllerInfo)

  CtrlStorageSetSucLastIndex(fieldVal);


  memcpy(pReadInfo, pWriteInfo, sizeof(writeInfo));
  writeInfo.MaxNodeId = fieldVal;

  mock_call_expect(TO_STR(zpal_nvm_read), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00004; //FILE_ID_CONTROLLERINFO
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
  pMock->output_arg[ARG2].pointer = pReadInfo;
  pMock->expect_arg[ARG3].value = CONTROLLERINFO_FILE_SIZE; //sizeof(SControllerInfo)

  mock_call_expect(TO_STR(zpal_nvm_write), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00004; //FILE_ID_CONTROLLERINFO
  pMock->expect_arg[ARG2].pointer = pWriteInfo;
  pMock->expect_arg[ARG3].value = CONTROLLERINFO_FILE_SIZE; //sizeof(SControllerInfo)

  CtrlStorageSetMaxNodeId(fieldVal);


  memcpy(pReadInfo, pWriteInfo, sizeof(writeInfo));
  writeInfo.ReservedId = fieldVal;

  mock_call_expect(TO_STR(zpal_nvm_read), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00004; //FILE_ID_CONTROLLERINFO
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
  pMock->output_arg[ARG2].pointer = pReadInfo;
  pMock->expect_arg[ARG3].value = CONTROLLERINFO_FILE_SIZE; //sizeof(SControllerInfo)

  mock_call_expect(TO_STR(zpal_nvm_write), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00004; //FILE_ID_CONTROLLERINFO
  pMock->expect_arg[ARG2].pointer = pWriteInfo;
  pMock->expect_arg[ARG3].value = CONTROLLERINFO_FILE_SIZE; //sizeof(SControllerInfo)

  CtrlStorageSetReservedId(fieldVal);

// Test reading long range channel selection mode
  mock_call_expect(TO_STR(zpal_nvm_read), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00004; //FILE_ID_CONTROLLERINFO
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
  pMock->output_arg[ARG2].pointer = pReadInfo;
  pMock->expect_arg[ARG3].value = CONTROLLERINFO_FILE_SIZE; //sizeof(SControllerInfo)

  fieldVal = (uint8_t)StorageGetLongRangeChannelAutoMode();
  TEST_ASSERT_EQUAL(false, fieldVal);

  // test writing long range channel selection mode

  memcpy(pReadInfo, pWriteInfo, sizeof(writeInfo));
  writeInfo.LongRangeChannelAutoMode = (uint8_t)true;

  mock_call_expect(TO_STR(zpal_nvm_read), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00004; //FILE_ID_CONTROLLERINFO
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
  pMock->output_arg[ARG2].pointer = pReadInfo;
  pMock->expect_arg[ARG3].value = CONTROLLERINFO_FILE_SIZE; //sizeof(SControllerInfo)

  mock_call_expect(TO_STR(zpal_nvm_write), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00004; //FILE_ID_CONTROLLERINFO
  pMock->expect_arg[ARG2].pointer = pWriteInfo;
  pMock->expect_arg[ARG3].value = CONTROLLERINFO_FILE_SIZE; //sizeof(SControllerInfo)

  StorageSetLongRangeChannelAutoMode(true);

  mock_calls_verify();

#ifdef NOT_USEFUL_WHEN_USING_MOCKS
 // TDU 3.8 Set ControllerConfiguration filed to a new value, then verify it was written and other fileds are still the same

  fieldVal = CTRL_INFO_TEST_VAL1;
  CtrlStorageSetControllerConfig(fieldVal);


  fieldVal = CtrlStorageGetControllerConfig();
  TEST_ASSERT_EQUAL(CTRL_INFO_TEST_VAL1, fieldVal);
  fieldVal = CtrlStorageGetStaticControllerNodeId();
  TEST_ASSERT_EQUAL(CTRL_INFO_TEST_VAL, fieldVal);
  fieldVal = CtrlStorageGetLastUsedNodeId();
  TEST_ASSERT_EQUAL(CTRL_INFO_TEST_VAL, fieldVal);
  fieldVal = CtrlStorageGetSucLastIndex();
  TEST_ASSERT_EQUAL(CTRL_INFO_TEST_VAL, fieldVal);
  fieldVal = CtrlStorageGetMaxNodeId();
  TEST_ASSERT_EQUAL(CTRL_INFO_TEST_VAL, fieldVal);
  fieldVal = CtrlStorageGetReservedId();
  TEST_ASSERT_EQUAL(CTRL_INFO_TEST_VAL, fieldVal);


 // TDU 3.9 Set GetStaticControllerNodeId filed to a new value, then verify it was written and other fileds are still the same

  fieldVal = CTRL_INFO_TEST_VAL2;
  CtrlStorageSetStaticControllerNodeId(fieldVal);


  fieldVal = CtrlStorageGetControllerConfig();
  TEST_ASSERT_EQUAL(CTRL_INFO_TEST_VAL1, fieldVal);
  fieldVal = CtrlStorageGetStaticControllerNodeId();
  TEST_ASSERT_EQUAL(CTRL_INFO_TEST_VAL2, fieldVal);
  fieldVal = CtrlStorageGetLastUsedNodeId();
  TEST_ASSERT_EQUAL(CTRL_INFO_TEST_VAL, fieldVal);
  fieldVal = CtrlStorageGetSucLastIndex();
  TEST_ASSERT_EQUAL(CTRL_INFO_TEST_VAL, fieldVal);
  fieldVal = CtrlStorageGetMaxNodeId();
  TEST_ASSERT_EQUAL(CTRL_INFO_TEST_VAL, fieldVal);
  fieldVal = CtrlStorageGetReservedId();
  TEST_ASSERT_EQUAL(CTRL_INFO_TEST_VAL, fieldVal);


 // TDU 3.10 Set LastUsedNodeId filed to a new value, then verify it was written and other fileds are still the same

  fieldVal = CTRL_INFO_TEST_VAL3;
  CtrlStorageSetLastUsedNodeId(fieldVal);


  fieldVal = CtrlStorageGetControllerConfig();
  TEST_ASSERT_EQUAL(CTRL_INFO_TEST_VAL1, fieldVal);
  fieldVal = CtrlStorageGetStaticControllerNodeId();
  TEST_ASSERT_EQUAL(CTRL_INFO_TEST_VAL2, fieldVal);
  fieldVal = CtrlStorageGetLastUsedNodeId();
  TEST_ASSERT_EQUAL(CTRL_INFO_TEST_VAL3, fieldVal);
  fieldVal = CtrlStorageGetSucLastIndex();
  TEST_ASSERT_EQUAL(CTRL_INFO_TEST_VAL, fieldVal);
  fieldVal = CtrlStorageGetMaxNodeId();
  TEST_ASSERT_EQUAL(CTRL_INFO_TEST_VAL, fieldVal);
  fieldVal = CtrlStorageGetReservedId();
  TEST_ASSERT_EQUAL(CTRL_INFO_TEST_VAL, fieldVal);


 // TDU 3.11 Set SucLastIndex filed to a new value, then verify it was written and other fileds are still the same

  fieldVal = CTRL_INFO_TEST_VAL4;
  CtrlStorageSetSucLastIndex(fieldVal);


  fieldVal = CtrlStorageGetControllerConfig();
  TEST_ASSERT_EQUAL(CTRL_INFO_TEST_VAL1, fieldVal);
  fieldVal = CtrlStorageGetStaticControllerNodeId();
  TEST_ASSERT_EQUAL(CTRL_INFO_TEST_VAL2, fieldVal);
  fieldVal = CtrlStorageGetLastUsedNodeId();
  TEST_ASSERT_EQUAL(CTRL_INFO_TEST_VAL3, fieldVal);
  fieldVal = CtrlStorageGetSucLastIndex();
  TEST_ASSERT_EQUAL(CTRL_INFO_TEST_VAL4, fieldVal);
  fieldVal = CtrlStorageGetMaxNodeId();
  TEST_ASSERT_EQUAL(CTRL_INFO_TEST_VAL, fieldVal);
  fieldVal = CtrlStorageGetReservedId();
  TEST_ASSERT_EQUAL(CTRL_INFO_TEST_VAL, fieldVal);



 // TDU 3.12 Set SucAwarenessSetting filed to a new value, then verify it was written and other fileds are still the same

  fieldVal = CtrlStorageGetControllerConfig();
  TEST_ASSERT_EQUAL(CTRL_INFO_TEST_VAL1, fieldVal);
  fieldVal = CtrlStorageGetStaticControllerNodeId();
  TEST_ASSERT_EQUAL(CTRL_INFO_TEST_VAL2, fieldVal);
  fieldVal = CtrlStorageGetLastUsedNodeId();
  TEST_ASSERT_EQUAL(CTRL_INFO_TEST_VAL3, fieldVal);
  fieldVal = CtrlStorageGetSucLastIndex();
  TEST_ASSERT_EQUAL(CTRL_INFO_TEST_VAL4, fieldVal);
  fieldVal = CtrlStorageGetMaxNodeId();
  TEST_ASSERT_EQUAL(CTRL_INFO_TEST_VAL, fieldVal);
  fieldVal = CtrlStorageGetReservedId();
  TEST_ASSERT_EQUAL(CTRL_INFO_TEST_VAL, fieldVal);



 // TDU 3.13 Set MaxNodeId filed to a new value, then verify it was written and other fileds are still the same

  fieldVal = CTRL_INFO_TEST_VAL6;
  CtrlStorageSetMaxNodeId(fieldVal);


  fieldVal = CtrlStorageGetControllerConfig();
  TEST_ASSERT_EQUAL(CTRL_INFO_TEST_VAL1, fieldVal);
  fieldVal = CtrlStorageGetStaticControllerNodeId();
  TEST_ASSERT_EQUAL(CTRL_INFO_TEST_VAL2, fieldVal);
  fieldVal = CtrlStorageGetLastUsedNodeId();
  TEST_ASSERT_EQUAL(CTRL_INFO_TEST_VAL3, fieldVal);
  fieldVal = CtrlStorageGetSucLastIndex();
  TEST_ASSERT_EQUAL(CTRL_INFO_TEST_VAL4, fieldVal);
  fieldVal = CtrlStorageGetMaxNodeId();
  TEST_ASSERT_EQUAL(CTRL_INFO_TEST_VAL6, fieldVal);
  fieldVal = CtrlStorageGetReservedId();
  TEST_ASSERT_EQUAL(CTRL_INFO_TEST_VAL, fieldVal);



 // TDU 3.14 Set ReservedId filed to a new value, then verify it was written and other fileds are still the same

  fieldVal = CTRL_INFO_TEST_VAL7;
  CtrlStorageSetReservedId(fieldVal);


  fieldVal = CtrlStorageGetControllerConfig();
  TEST_ASSERT_EQUAL(CTRL_INFO_TEST_VAL1, fieldVal);
  fieldVal = CtrlStorageGetStaticControllerNodeId();
  TEST_ASSERT_EQUAL(CTRL_INFO_TEST_VAL2, fieldVal);
  fieldVal = CtrlStorageGetLastUsedNodeId();
  TEST_ASSERT_EQUAL(CTRL_INFO_TEST_VAL3, fieldVal);
  fieldVal = CtrlStorageGetSucLastIndex();
  TEST_ASSERT_EQUAL(CTRL_INFO_TEST_VAL4, fieldVal);
  fieldVal = CtrlStorageGetMaxNodeId();
  TEST_ASSERT_EQUAL(CTRL_INFO_TEST_VAL6, fieldVal);
  fieldVal = CtrlStorageGetReservedId();
  TEST_ASSERT_EQUAL(CTRL_INFO_TEST_VAL7, fieldVal);

#endif //NOT_USEFUL_WHEN_USING_MOCKS

  // Test the SUC update table
#define SUC_UPDATE_NODEID_TEST       0x10
#define SUC_UPDATE_CHANGETYPE_TEST   0x20
#define SUC_UPDATE_NODEINFO_TEST     0x30
  // TDU 4.1 test that all SUC updates entries in the table are zero at startup
  SUC_UPDATE_ENTRY_STRUCT SucUpdateEntry;
  uint8_t writeSucList[1408];
  uint8_t readSucList[1408];
  memset(readSucList, 0xff, sizeof(readSucList));

  for (uint8_t i =0 ; i < SUC_MAX_UPDATES; i++)
  {
    mock_calls_clear();

    mock_call_expect(TO_STR(zpal_nvm_read_object_part), &pMock);
    pMock->return_code.value = ZPAL_STATUS_OK;
    pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
    pMock->expect_arg[ARG1].value = 0x04000 + (i / SUCNODES_PER_FILE); //FILE_ID_SUCNODELIST_BASE + file
    pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
    pMock->output_arg[ARG2].pointer = readSucList + i*sizeof(SUC_UPDATE_ENTRY_STRUCT);
    pMock->expect_arg[ARG3].value = (i % SUCNODES_PER_FILE) * sizeof(SUC_UPDATE_ENTRY_STRUCT);
    pMock->expect_arg[ARG4].value = sizeof(SUC_UPDATE_ENTRY_STRUCT);

    CtrlStorageGetSucUpdateEntry(i, &SucUpdateEntry);
    TEST_ASSERT_EQUAL(0, SucUpdateEntry.NodeID);
    TEST_ASSERT_EQUAL(0xff, SucUpdateEntry.changeType);

    for (uint8_t x = 0 ; x <SUC_UPDATE_NODEPARM_MAX; x++)
    {
      TEST_ASSERT_EQUAL(0xff, SucUpdateEntry.nodeInfo[x]);
    }
    mock_calls_verify();
  }
  mock_calls_clear();


  // write test values to the SUC updates entries

  for (uint8_t i =0 ; i < SUC_MAX_UPDATES; i++)
  {


    SucUpdateEntry.NodeID = SUC_UPDATE_NODEID_TEST + i ;
    SucUpdateEntry.changeType = SUC_UPDATE_CHANGETYPE_TEST + i;
    for (uint8_t x = 0 ; x <SUC_UPDATE_NODEPARM_MAX; x++)
    {

      SucUpdateEntry.nodeInfo[x] = SUC_UPDATE_NODEINFO_TEST + x + i;
    }

    mock_call_expect(TO_STR(zpal_nvm_read), &pMock);
    pMock->return_code.value = ZPAL_STATUS_OK;
    pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
    pMock->expect_arg[ARG1].value = 0x04000 + (i / SUCNODES_PER_FILE); //FILE_ID_SUCNODELIST_BASE + file
    pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
    pMock->output_arg[ARG2].pointer = readSucList + (i / SUCNODES_PER_FILE)*176;
    pMock->expect_arg[ARG3].value = 176; //sizeof(SSucNodeList)

    memcpy(writeSucList, readSucList, sizeof(writeSucList));
    writeSucList[22*i] = SucUpdateEntry.NodeID;  //22 == sizeof(SUC_UPDATE_ENTRY_STRUCT)
    writeSucList[22*i + 1] = SucUpdateEntry.changeType;

    mock_call_expect(TO_STR(zpal_nvm_write), &pMock);
    pMock->return_code.value = ZPAL_STATUS_OK;
    pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
    pMock->expect_arg[ARG1].value = 0x04000 + (i / SUCNODES_PER_FILE); //FILE_ID_SUCNODELIST_BASE + file
    pMock->expect_arg[ARG2].pointer = writeSucList + (i / SUCNODES_PER_FILE)*176;
    pMock->expect_arg[ARG3].value = 176; //sizeof(SSucNodeList)

    CtrlStorageSetSucUpdateEntry(i, SucUpdateEntry.changeType, SucUpdateEntry.NodeID);

    memcpy(readSucList, writeSucList, sizeof(writeSucList));

    mock_call_expect(TO_STR(zpal_nvm_read), &pMock);
    pMock->return_code.value = ZPAL_STATUS_OK;
    pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
    pMock->expect_arg[ARG1].value = 0x04000 + (i / SUCNODES_PER_FILE); //FILE_ID_SUCNODELIST_BASE + file
    pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
    pMock->output_arg[ARG2].pointer = readSucList + (i / SUCNODES_PER_FILE)*176;
    pMock->expect_arg[ARG3].value = 176; //sizeof(SSucNodeList)

    memcpy(writeSucList, readSucList, sizeof(writeSucList));
    memcpy(&writeSucList[22*i + 2], SucUpdateEntry.nodeInfo, SUC_UPDATE_NODEPARM_MAX);

    mock_call_expect(TO_STR(zpal_nvm_write), &pMock);
    pMock->return_code.value = ZPAL_STATUS_OK;
    pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
    pMock->expect_arg[ARG1].value = 0x04000 + (i / SUCNODES_PER_FILE); //FILE_ID_SUCNODELIST_BASE + file
    pMock->expect_arg[ARG2].pointer = writeSucList + (i / SUCNODES_PER_FILE)*176;
    pMock->expect_arg[ARG3].value = 176; //sizeof(SSucNodeList)

    CtrlStorageSetCmdClassInSucUpdateEntry( i, SucUpdateEntry.nodeInfo);

    memcpy(readSucList, writeSucList, sizeof(writeSucList));

    mock_calls_verify();
  }

  // TDU 4.2 verify written data are correct

  for (uint8_t i =0 ; i < SUC_MAX_UPDATES; i++)
  {
    mock_call_expect(TO_STR(zpal_nvm_read_object_part), &pMock);
    pMock->return_code.value = ZPAL_STATUS_OK;
    pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
    pMock->expect_arg[ARG1].value = 0x04000 + (i / SUCNODES_PER_FILE); //FILE_ID_SUCNODELIST_BASE + file
    pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
    pMock->output_arg[ARG2].pointer = readSucList + i*sizeof(SUC_UPDATE_ENTRY_STRUCT);
    pMock->expect_arg[ARG3].value = (i % SUCNODES_PER_FILE) * sizeof(SUC_UPDATE_ENTRY_STRUCT);
    pMock->expect_arg[ARG4].value = sizeof(SUC_UPDATE_ENTRY_STRUCT);

    CtrlStorageGetSucUpdateEntry(i, &SucUpdateEntry);
    TEST_ASSERT_EQUAL((SUC_UPDATE_NODEID_TEST + i) , SucUpdateEntry.NodeID);
    TEST_ASSERT_EQUAL((SUC_UPDATE_CHANGETYPE_TEST + i), SucUpdateEntry.changeType);
    for (uint8_t x = 0 ; x <SUC_UPDATE_NODEPARM_MAX; x++)
    {
      TEST_ASSERT_EQUAL((SUC_UPDATE_NODEINFO_TEST + x + i), SucUpdateEntry.nodeInfo[x]);
    }

  }
  mock_calls_verify();
}


// Testing the reaction to Nvm File System formatted callback
// Modules should write all default files with default values, as when initializing to empty FS.
// Test assumes that other test cases verify that init create the default files with default content.
void test_ControllerStorage_filesystem_format_callback(void)
{
  mock_calls_clear();

  //--------------------------------------------------------------------------------------------------------
  // File system init start
  //--------------------------------------------------------------------------------------------------------
  uint8_t dummy;
  zpal_nvm_handle_t pFileSystem = &dummy;
  //--------------------------------------------------------------------------------------------------------
  // -------------------------------------------------------filesyseminit end
  //--------------------------------------------------------------------------------------------------------

  // initialize controller network
  mock_t * pMockNvmFileSystemRegister;
  mock_call_use_as_stub(TO_STR(NvmFileSystemInit));
  mock_call_expect(TO_STR(NvmFileSystemRegister), &pMockNvmFileSystemRegister); // TDU 4.0 Test DUT registers with NVM on init
  pMockNvmFileSystemRegister->return_code.pointer = pFileSystem;
  pMockNvmFileSystemRegister->compare_rule_arg[ARG0] = COMPARE_NOT_NULL; // TDU 4.1 Test DUT registers with non null pointer and no object

  mock_t * pMock;
  mock_call_expect(TO_STR(zpal_nvm_read), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00000; //FILE_ID_ZW_VERSION
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG3].value = 4; //sizeof(uint32_t)
  //Set current file system version as output
  uint32_t version = (ZW_CONTROLLER_FILESYS_VERSION << 24) | (ZW_VERSION_MAJOR << 16) | (ZW_VERSION_MINOR << 8) | (ZW_VERSION_PATCH);
  pMock->output_arg[ARG2].p = (uint8_t *)&version;


  // FileSet - allows FileSystemCaretaker to validate required files
  static const SObjectDescriptor t_aFileDescriptors[] = {
    { .ObjectKey = 0x00002,   .iDataSize = 29   },  //FILE_ID_PREFERREDREPEATERS
    { .ObjectKey = 0x00004,   .iDataSize = CONTROLLERINFO_FILE_SIZE   },  //FILE_ID_CONTROLLERINFO
    { .ObjectKey = 0x00005,   .iDataSize = 29   },  //FILE_ID_NODE_STORAGE_EXIST
    { .ObjectKey = 0x0000B,   .iDataSize = 29   },  //FILE_ID_NODE_ROUTECACHE_EXIST
    { .ObjectKey = 0x0000C,   .iDataSize = sizeof(LR_NODE_MASK_TYPE)   },  //FILE_ID_LRANGE_NODE_EXIST
    { .ObjectKey = 0x00006,   .iDataSize = 29   },  //FILE_ID_APP_ROUTE_LOCK_FLAG
    { .ObjectKey = 0x00007,   .iDataSize = 29   },  //FILE_ID_ROUTE_SLAVE_SUC_FLAG
    { .ObjectKey = 0x00008,   .iDataSize = 29   },  //FILE_ID_SUC_PENDING_UPDATE_FLAG
    { .ObjectKey = 0x00009,   .iDataSize = 29   },  //FILE_ID_BRIDGE_NODE_FLAG
    { .ObjectKey = 0x0000A,   .iDataSize = 29   },  //FILE_ID_PENDING_DISCOVERY_FLAG
  };

  SObjectSet tFileSet = {
    .pFileSystem = pFileSystem,
    .iObjectCount = 10, //sizeof_array(t_aFileDescriptors)
    .pObjectDescriptors = t_aFileDescriptors
  };

  mock_call_expect(TO_STR(NVMCaretakerVerifySet), &pMock);
  pMock->return_code.value = ECTKR_STATUS_SUCCESS;  //return that no file was found
  pMock->expect_arg[ARG0].p = &tFileSet;
  pMock->compare_rule_arg[ARG1] = COMPARE_NOT_NULL;

  uint32_t file_id[] = { 0x00005,  //FILE_ID_NODE_STORAGE_EXIST
                         0x0000B,  //FILE_ID_NODE_ROUTECACHE_EXIST
                         0x0000C,  //FILE_ID_LRANGE_NODE_EXIST
                         0x00006,  //FILE_ID_APP_ROUTE_LOCK_FLAG
                         0x00007,  //FILE_ID_ROUTE_SLAVE_SUC_FLAG
                         0x00008,  //FILE_ID_SUC_PENDING_UPDATE_FLAG
                         0x00009,  //FILE_ID_BRIDGE_NODE_FLAG
                         0x0000A   //FILE_ID_PENDING_DISCOVERY_FLAG
                        };

  for(uint32_t i=0; i<(sizeof(file_id)/sizeof(uint32_t)); i++)
  {
    mock_call_expect(TO_STR(zpal_nvm_read), &pMock);
    pMock->return_code.value = ZPAL_STATUS_OK;
    pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
    pMock->expect_arg[ARG1].value = file_id[i]; //FILE_ID_
    pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
    if(0x0000C == file_id[i])  //Special nodemask for FILE_ID_LRANGE_NODE_EXIST file
    {
      pMock->expect_arg[ARG3].value = sizeof(LR_NODE_MASK_TYPE);
    }
    else
    {
      pMock->expect_arg[ARG3].value = sizeof(NODE_MASK_TYPE);
    }
  }

  CtrlStorageInit();

  SSyncEvent * pRegisterEvent = (SSyncEvent*)(pMockNvmFileSystemRegister->actual_arg[ARG0].pointer);
  TEST_ASSERT_NOT_NULL(pRegisterEvent);
  TEST_ASSERT_NOT_NULL(pRegisterEvent->uFunctor.pFunction); // TDU 4.1
  TEST_ASSERT_NULL(pRegisterEvent->pObject);                // TDU 4.1

  //Verify that default data is written to files.
  LR_NODE_MASK_TYPE tNodeMask;  //Full Long Range nodemask to not run out of space
  memset(tNodeMask, 0, sizeof(LR_NODE_MASK_TYPE));

  uint8_t sucNodeList[1408];
  memset(sucNodeList, 0, 1408);


  mock_call_expect(TO_STR(zpal_nvm_write), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00002; //FILE_ID_PREFERREDREPEATERS
  pMock->expect_arg[ARG2].p = tNodeMask;
  pMock->expect_arg[ARG3].value = 29; //sizeof(SPreferredRepeaters)

  mock_call_expect(TO_STR(zpal_nvm_write), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00004; //FILE_ID_CONTROLLERINFO
  pMock->expect_arg[ARG2].p = (uint8_t *)&defaultControllerInfo;
  pMock->expect_arg[ARG3].value = CONTROLLERINFO_FILE_SIZE; //sizeof(SControllerInfo)

  for(uint32_t i=0; i<(sizeof(file_id)/sizeof(uint32_t)); i++)
  {
    mock_call_expect(TO_STR(zpal_nvm_write), &pMock);
    pMock->return_code.value = ZPAL_STATUS_OK;
    pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
    pMock->expect_arg[ARG1].value = file_id[i]; //FILE_ID_
    pMock->expect_arg[ARG2].p = tNodeMask;
    if(0x0000C == file_id[i])  //Special nodemask for FILE_ID_LRANGE_NODE_EXIST file
    {
      pMock->expect_arg[ARG3].value = sizeof(LR_NODE_MASK_TYPE);
    }
    else
    {
      pMock->expect_arg[ARG3].value = sizeof(NODE_MASK_TYPE);
    }
  }

  // Testing current file system version
  version = m_current_ZW_Version;

  mock_call_expect(TO_STR(zpal_nvm_write), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00000; //FILE_ID_ZW_VERSION  (Should always be 0x00000, must NEVER change)
  pMock->expect_arg[ARG2].p = &version;
  pMock->expect_arg[ARG3].value = 4; //sizeof(uint32_t)

  SyncEventInvoke(pRegisterEvent);

  mock_calls_verify();
}

// Test writing and reading route cache files
void test_ControllerStorage_RouteCache(void)
{
  mock_calls_clear();

  //Set up pointer to file system
  uint8_t dummy;
  zpal_nvm_handle_t pFileSystem = &dummy;

  // initialize controller network
  mock_t * pMockNvmFileSystemRegister;
  mock_call_use_as_stub(TO_STR(NvmFileSystemInit));
  mock_call_expect(TO_STR(NvmFileSystemRegister), &pMockNvmFileSystemRegister);
  pMockNvmFileSystemRegister->return_code.pointer = pFileSystem;
  pMockNvmFileSystemRegister->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;

  mock_t * pMock;
  mock_call_expect(TO_STR(zpal_nvm_read), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00000; //FILE_ID_ZW_VERSION
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG3].value = 4; //sizeof(uint32_t)
  // Use the current file system version
  uint32_t versionNr = m_current_ZW_Version;
  pMock->output_arg[ARG2].pointer = &versionNr;

  mock_t * pMockCaretakerVerify;
  mock_call_expect(TO_STR(NVMCaretakerVerifySet), &pMockCaretakerVerify);
  pMockCaretakerVerify->return_code.value = ECTKR_STATUS_SUCCESS;  //return that the files were found
  pMockCaretakerVerify->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMockCaretakerVerify->compare_rule_arg[ARG1] = COMPARE_NOT_NULL;

  uint32_t file_id[8] = {0x00005,  //FILE_ID_NODE_STORAGE_EXIST
                         0x0000B,  //FILE_ID_NODE_ROUTECACHE_EXIST
                         0x0000C,  //FILE_ID_LRANGE_NODE_EXIST
                         0x00006,  //FILE_ID_APP_ROUTE_LOCK_FLAG
                         0x00007,  //FILE_ID_ROUTE_SLAVE_SUC_FLAG
                         0x00008,  //FILE_ID_SUC_PENDING_UPDATE_FLAG
                         0x00009,  //FILE_ID_BRIDGE_NODE_FLAG
                         0x0000A   //FILE_ID_PENDING_DISCOVERY_FLAG
                        };

  for(uint32_t i=0; i<(sizeof(file_id)/sizeof(uint32_t)); i++)
  {
    printf("i: %d, file_id[i]: %x \n", i, file_id[i]);

    mock_call_expect(TO_STR(zpal_nvm_read), &pMock);
    pMock->return_code.value = ZPAL_STATUS_OK;
    pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
    pMock->expect_arg[ARG1].value = file_id[i]; //FILE_ID_
    pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
    if(0x0000C == file_id[i])  //Special nodemask for FILE_ID_LRANGE_NODE_EXIST file
    {
      pMock->expect_arg[ARG3].value = sizeof(LR_NODE_MASK_TYPE);
    }
    else
    {
      pMock->expect_arg[ARG3].value = sizeof(NODE_MASK_TYPE);
    }
  }

  CtrlStorageInit();

  SSyncEvent * pRegisterEvent = (SSyncEvent*)pMockNvmFileSystemRegister->actual_arg[ARG0].pointer;
  TEST_ASSERT_NOT_NULL(pRegisterEvent);
  TEST_ASSERT_NOT_NULL(pRegisterEvent->uFunctor.pFunction);
  TEST_ASSERT_NULL(pRegisterEvent->pObject);

  // test that when reading non existing node, all fields are zero
  for (uint8_t t_nodeID = 1; t_nodeID <= ZW_MAX_NODES; t_nodeID++)
  {
    ROUTECACHE_LINE routeLine;

     // TDU 1.4 Test non existing route cache line are zero
    CtrlStorageGetRouteCache(ROUTE_CACHE_NORMAL, t_nodeID, &routeLine);
    TEST_ASSERT_MESSAGE(0 == routeLine.routecacheLineConfSize, "TDU 1.4");
    TEST_ASSERT_MESSAGE(0 == routeLine.repeaterList[0], "TDU 1.4");
    TEST_ASSERT_MESSAGE(0 == routeLine.repeaterList[1], "TDU 1.4");
    TEST_ASSERT_MESSAGE(0 == routeLine.repeaterList[2], "TDU 1.4");
    TEST_ASSERT_MESSAGE(0 == routeLine.repeaterList[3], "TDU 1.4");

     // TDU 1.5 Test non existing route cache lines are zero
    CtrlStorageGetRouteCache(ROUTE_CACHE_NLWR_SR, t_nodeID, &routeLine);
    TEST_ASSERT_MESSAGE(0 == routeLine.routecacheLineConfSize, "TDU 1.5");
    TEST_ASSERT_MESSAGE(0 == routeLine.repeaterList[0], "TDU 1.5");
    TEST_ASSERT_MESSAGE(0 == routeLine.repeaterList[1], "TDU 1.5");
    TEST_ASSERT_MESSAGE(0 == routeLine.repeaterList[2], "TDU 1.5");
    TEST_ASSERT_MESSAGE(0 == routeLine.repeaterList[3], "TDU 1.5");
  }

  mock_calls_verify();

  for (uint8_t t_nodeID = 1; t_nodeID <= ZW_MAX_NODES; t_nodeID++)
  {
    mock_calls_clear();

    //t_nodeID 1-232
    //t_nodeNr 0-231  for local indexing in arrays
    uint16_t t_nodeNr = t_nodeID - 1;

    ROUTECACHE_LINE routeLine;
    ROUTECACHE_LINE routeLineRead;

    routeLine.routecacheLineConfSize = TEST_VAL7;
    routeLine.repeaterList[0] = TEST_VAL8(0);
    routeLine.repeaterList[1] = TEST_VAL8(1);
    routeLine.repeaterList[2] = TEST_VAL8(2);
    routeLine.repeaterList[3] = TEST_VAL8(3);

    //Verify that old data is saved to file when the RAM buffer is full and the lowest prio slot must be used for other data
    if((0 == (t_nodeNr % NODEROUTECACHES_PER_FILE)) && ((NODEROUTECACHES_PER_FILE * NODEROUTECACHE_FILES_IN_RAM) <= t_nodeNr))
    {
      mock_call_expect(TO_STR(zpal_nvm_write), &pMock);
      pMock->return_code.value = ZPAL_STATUS_OK;
      pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
      pMock->expect_arg[ARG1].value = 0x01400 + (t_nodeNr / NODEROUTECACHES_PER_FILE) - NODEROUTECACHE_FILES_IN_RAM;
      pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
      pMock->expect_arg[ARG3].value = 80; //sizeof(SNodeRouteCache) * NODEROUTECACHES_PER_FILE
    }

    //Verify that ROUTECACHE_EXIST nodemask is written
    mock_call_expect(TO_STR(zpal_nvm_write), &pMock);
    pMock->return_code.value = ZPAL_STATUS_OK;
    pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
    pMock->expect_arg[ARG1].value = 0x0000B; //FILE_ID_NODE_ROUTECAHE_EXIST
    pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
    pMock->expect_arg[ARG3].value = 29; //sizeof(NODE_MASK_TYPE)

    CtrlStorageSetRouteCache(ROUTE_CACHE_NORMAL, t_nodeID, &routeLine);

    //Verify that the data written can be read back
    CtrlStorageGetRouteCache(ROUTE_CACHE_NORMAL, t_nodeID, &routeLineRead);

    TEST_ASSERT_EQUAL_UINT8_ARRAY((uint8_t*)&routeLineRead, (uint8_t*)&routeLine , sizeof(routeLine));

    //Verify that further writes and reads to the same nodeID will not trigger calls to NVM3
    routeLine.routecacheLineConfSize = TEST_VAL9;
    routeLine.repeaterList[0] = TEST_VAL10(0);
    routeLine.repeaterList[1] = TEST_VAL10(1);
    routeLine.repeaterList[2] = TEST_VAL10(2);
    routeLine.repeaterList[3] = TEST_VAL10(3);

    CtrlStorageSetRouteCache(ROUTE_CACHE_NLWR_SR, t_nodeID, &routeLine);

    CtrlStorageGetRouteCache(ROUTE_CACHE_NLWR_SR, t_nodeID, &routeLineRead);

    TEST_ASSERT_EQUAL_UINT8_ARRAY((uint8_t*)&routeLineRead, (uint8_t*)&routeLine , sizeof(routeLine));

    mock_calls_verify();
  }

  //Verify that reading data from nodes not in the RAM buffer will trigger zpal_nvm_read()
  uint8_t nodeIdInRAM = (ZW_MAX_NODES + 1) - (NODEROUTECACHES_PER_FILE * NODEROUTECACHE_FILES_IN_RAM);
  for (uint8_t t_nodeID = 1; t_nodeID < nodeIdInRAM; t_nodeID++)
  {
    mock_calls_clear();

    //t_nodeID 1-232
    //t_nodeNr 0-231  for local indexing in arrays
    uint16_t t_nodeNr = t_nodeID - 1;

    uint8_t routeCacheEntry[80];

    ROUTECACHE_LINE routeLine;
    ROUTECACHE_LINE routeLineRead;

    routeLine.routecacheLineConfSize = TEST_VAL7;
    routeLine.repeaterList[0] = TEST_VAL8(0);
    routeLine.repeaterList[1] = TEST_VAL8(1);
    routeLine.repeaterList[2] = TEST_VAL8(2);
    routeLine.repeaterList[3] = TEST_VAL8(3);

    mock_call_expect(TO_STR(zpal_nvm_read), &pMock);
    pMock->return_code.value = ZPAL_STATUS_OK;
    pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
    pMock->expect_arg[ARG1].value = 0x01400 + (t_nodeNr / NODEROUTECACHES_PER_FILE);
    pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
    pMock->output_arg[ARG2].p = &routeCacheEntry;
    pMock->expect_arg[ARG3].value = 80; //sizeof(SNodeRouteCache) * NODEROUTECACHES_PER_FILE

    uint32_t positionInFile = (t_nodeNr % NODEROUTECACHES_PER_FILE) * 10; //10 == sizeof(SNodeRouteCache)
    memcpy(&routeCacheEntry[positionInFile], &routeLine, 5);

    // TDU 1.10 test that data read from the route cache line field is that same as the data previously written to it
    CtrlStorageGetRouteCache(ROUTE_CACHE_NORMAL, t_nodeID, &routeLineRead);
    TEST_ASSERT_MESSAGE(TEST_VAL7 == routeLineRead.routecacheLineConfSize, "TDU 1.10");
    TEST_ASSERT_MESSAGE(TEST_VAL8(0) == routeLineRead.repeaterList[0],     "TDU 1.10");
    TEST_ASSERT_MESSAGE(TEST_VAL8(1) == routeLineRead.repeaterList[1],     "TDU 1.10");
    TEST_ASSERT_MESSAGE(TEST_VAL8(2) == routeLineRead.repeaterList[2],     "TDU 1.10");
    TEST_ASSERT_MESSAGE(TEST_VAL8(3) == routeLineRead.repeaterList[3],     "TDU 1.10");


    routeLine.routecacheLineConfSize = TEST_VAL9;
    routeLine.repeaterList[0] = TEST_VAL10(0);
    routeLine.repeaterList[1] = TEST_VAL10(1);
    routeLine.repeaterList[2] = TEST_VAL10(2);
    routeLine.repeaterList[3] = TEST_VAL10(3);

    mock_call_expect(TO_STR(zpal_nvm_read), &pMock);
    pMock->return_code.value = ZPAL_STATUS_OK;
    pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
    pMock->expect_arg[ARG1].value = 0x01400 + (t_nodeNr / NODEROUTECACHES_PER_FILE);
    pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
    pMock->output_arg[ARG2].p = &routeCacheEntry;
    pMock->expect_arg[ARG3].value = 80; //sizeof(SNodeRouteCache) * NODEROUTECACHES_PER_FILE

    positionInFile = sizeof(routeLine) + (t_nodeNr % NODEROUTECACHES_PER_FILE) * 10; //10 == sizeof(SNodeRouteCache)
    memcpy(&routeCacheEntry[positionInFile], &routeLine, 5); //5 == sizeof(ROUTECACHE_LINE)

    // TDU 1.11 test that data read from the route cache line field is that same as the data previously written to it
    CtrlStorageGetRouteCache(ROUTE_CACHE_NLWR_SR, t_nodeID, &routeLineRead);
    TEST_ASSERT_MESSAGE(TEST_VAL9 == routeLineRead.routecacheLineConfSize, "TDU 1.11");
    TEST_ASSERT_MESSAGE(TEST_VAL10(0) == routeLineRead.repeaterList[0],    "TDU 1.11");
    TEST_ASSERT_MESSAGE(TEST_VAL10(1) == routeLineRead.repeaterList[1],    "TDU 1.11");
    TEST_ASSERT_MESSAGE(TEST_VAL10(2) == routeLineRead.repeaterList[2],    "TDU 1.11");
    TEST_ASSERT_MESSAGE(TEST_VAL10(3) == routeLineRead.repeaterList[3],    "TDU 1.11");
    mock_calls_verify();
  }

  //Verify that reading data from nodes in the RAM buffer will not trigger zpal_nvm_read()
  //Verify that data in RAM is still correct
  for (uint8_t t_nodeID = nodeIdInRAM; t_nodeID <= ZW_MAX_NODES; t_nodeID++)
  {
    mock_calls_clear();

    ROUTECACHE_LINE routeLineRead;

    // TDU 1.10 test that data read from the route cache line field is that same as the data previously written to it
    CtrlStorageGetRouteCache(ROUTE_CACHE_NORMAL, t_nodeID, &routeLineRead);
    TEST_ASSERT_MESSAGE(TEST_VAL7 == routeLineRead.routecacheLineConfSize, "TDU 1.10");
    TEST_ASSERT_MESSAGE(TEST_VAL8(0) == routeLineRead.repeaterList[0],     "TDU 1.10");
    TEST_ASSERT_MESSAGE(TEST_VAL8(1) == routeLineRead.repeaterList[1],     "TDU 1.10");
    TEST_ASSERT_MESSAGE(TEST_VAL8(2) == routeLineRead.repeaterList[2],     "TDU 1.10");
    TEST_ASSERT_MESSAGE(TEST_VAL8(3) == routeLineRead.repeaterList[3],     "TDU 1.10");


    // TDU 1.11 test that data read from the route cache line field is that same as the data previously written to it
    CtrlStorageGetRouteCache(ROUTE_CACHE_NLWR_SR, t_nodeID, &routeLineRead);
    TEST_ASSERT_MESSAGE(TEST_VAL9 == routeLineRead.routecacheLineConfSize, "TDU 1.11");
    TEST_ASSERT_MESSAGE(TEST_VAL10(0) == routeLineRead.repeaterList[0],    "TDU 1.11");
    TEST_ASSERT_MESSAGE(TEST_VAL10(1) == routeLineRead.repeaterList[1],    "TDU 1.11");
    TEST_ASSERT_MESSAGE(TEST_VAL10(2) == routeLineRead.repeaterList[2],    "TDU 1.11");
    TEST_ASSERT_MESSAGE(TEST_VAL10(3) == routeLineRead.repeaterList[3],    "TDU 1.11");
    mock_calls_verify();
  }

  //Verify that trying to read file that does not exist in file system will lead to corresponding
  //bits in FILE_ID_NODE_ROUTECAHE_EXIST being cleared
  {
    mock_calls_clear();

    static uint8_t nodeID = 20;
    uint8_t routeLine[sizeof(ROUTECACHE_LINE)];
    //Fill routeLine with dummy values that must be overWritten by zeros
    memset(&routeLine, 0x3F, sizeof(ROUTECACHE_LINE));


    static NODE_MASK_TYPE node_routecache_exists = {0};
    memset(&node_routecache_exists, 0xFF, ZW_MAX_NODES/8);
    NodeMaskClearBit(node_routecache_exists, 16);
    NodeMaskClearBit(node_routecache_exists, 17);
    NodeMaskClearBit(node_routecache_exists, 18);
    NodeMaskClearBit(node_routecache_exists, 19);
    NodeMaskClearBit(node_routecache_exists, 20);
    NodeMaskClearBit(node_routecache_exists, 21);
    NodeMaskClearBit(node_routecache_exists, 22);
    NodeMaskClearBit(node_routecache_exists, 23);

    mock_call_expect(TO_STR(zpal_nvm_read), &pMock);
    pMock->return_code.value = ZPAL_STATUS_FAIL;
    pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
    pMock->expect_arg[ARG1].value = 0x01400 + (nodeID / NODEROUTECACHES_PER_FILE);
    pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
    pMock->expect_arg[ARG3].value = 80; //sizeof(SNodeRouteCache) * NODEROUTECACHES_PER_FILE

    mock_call_expect(TO_STR(zpal_nvm_write), &pMock);
    pMock->return_code.value = ZPAL_STATUS_OK;
    pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
    pMock->expect_arg[ARG1].value = 0x0000B; //FILE_ID_NODE_ROUTECAHE_EXIST
    pMock->expect_arg[ARG2].pointer = node_routecache_exists;
    pMock->expect_arg[ARG3].value = 29; //sizeof(NODE_MASK_TYPE)

    CtrlStorageGetRouteCache(ROUTE_CACHE_NORMAL, nodeID, (ROUTECACHE_LINE *)&routeLine);

    //Create an array of zeros to be used for comparison
    uint8_t expected[sizeof(ROUTECACHE_LINE)];
    memset(expected, 0, sizeof(ROUTECACHE_LINE));

    //Verify that roteLine contains only zeros
    TEST_ASSERT_EQUAL_UINT8_ARRAY_MESSAGE(&expected, &routeLine, sizeof(ROUTECACHE_LINE), "Nonzero output");

    mock_calls_verify();
  }
}

// Test that RAM buffer for RouteCaches is filled with data from files at initialization of file system
void test_ControllerStorage_RouteCache_Init(void)
{
  mock_calls_clear();

  static NODE_MASK_TYPE node_routecache_exists;
  static NODE_MASK_TYPE node_routecache_exists_2;

  uint8_t dummy;
  zpal_nvm_handle_t pFileSystem = &dummy;

  // initialize controller network
  mock_t * pMockNvmFileSystemRegister;
  mock_call_use_as_stub(TO_STR(NvmFileSystemInit));
  mock_call_expect(TO_STR(NvmFileSystemRegister), &pMockNvmFileSystemRegister);
  pMockNvmFileSystemRegister->return_code.pointer = pFileSystem;
  pMockNvmFileSystemRegister->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;

  mock_t * pMock;
  mock_call_expect(TO_STR(zpal_nvm_read), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00000; //FILE_ID_ZW_VERSION
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG3].value = 4; //sizeof(uint32_t)
  uint32_t versionNr = m_current_ZW_Version;
  pMock->output_arg[ARG2].pointer = &versionNr;

  mock_t * pMockCaretakerVerify;
  mock_call_expect(TO_STR(NVMCaretakerVerifySet), &pMockCaretakerVerify);
  pMockCaretakerVerify->return_code.value = ECTKR_STATUS_SUCCESS;  //return that the files were found
  pMockCaretakerVerify->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMockCaretakerVerify->compare_rule_arg[ARG1] = COMPARE_NOT_NULL;

  mock_call_expect(TO_STR(zpal_nvm_read), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00005,  //FILE_ID_NODE_STORAGE_EXIST
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG3].value = 29; //sizeof(NODE_MASK_TYPE)

  //Set up nodemask for routecache entries to trigger reads from NODEROUTECACHE files
  memset(node_routecache_exists, 0, sizeof(node_routecache_exists));
  //File 0
  ZW_NodeMaskSetBit(node_routecache_exists, 1);
  ZW_NodeMaskSetBit(node_routecache_exists, 2);
  ZW_NodeMaskSetBit(node_routecache_exists, 3);
  ZW_NodeMaskSetBit(node_routecache_exists, 4);
  //File 2
  ZW_NodeMaskSetBit(node_routecache_exists, 18);
  ZW_NodeMaskSetBit(node_routecache_exists, 20);
  //File 3
  ZW_NodeMaskSetBit(node_routecache_exists, 32);
  //File 21
  ZW_NodeMaskSetBit(node_routecache_exists, 172);
  //File 24
  ZW_NodeMaskSetBit(node_routecache_exists, 196);
  ZW_NodeMaskSetBit(node_routecache_exists, 198);
  //File 25
  ZW_NodeMaskSetBit(node_routecache_exists, 201);
  ZW_NodeMaskSetBit(node_routecache_exists, 203);

  memcpy(node_routecache_exists_2, node_routecache_exists, sizeof(node_routecache_exists));
  //Clear File 21 entry
  NodeMaskClearBit(node_routecache_exists_2, 171);

  mock_call_expect(TO_STR(zpal_nvm_read), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x0000B,  //FILE_ID_NODE_ROUTECACHE_EXIST
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
  pMock->output_arg[ARG2].pointer = node_routecache_exists;
  pMock->expect_arg[ARG3].value = 29; //sizeof(NODE_MASK_TYPE)

  static LR_NODE_MASK_TYPE node_lrange_exists;
  memset(&node_lrange_exists, 0x00, sizeof(LR_NODE_MASK_TYPE));

  mock_call_expect(TO_STR(zpal_nvm_read), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x0000C,  //FILE_ID_LRANGE_NODE_EXIST
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
  pMock->output_arg[ARG2].pointer = node_lrange_exists;
  pMock->expect_arg[ARG3].value = sizeof(LR_NODE_MASK_TYPE);

  //Verify that the first four expected files according to node_routecache_exists[]
  //are read into RAM buffer
  mock_call_expect(TO_STR(zpal_nvm_read), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = (0x01400 + 0),  //FILE_ID_NODEROUTECAHE
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG3].value = 80; //FILE_SIZE_NODEROUTECAHE

  mock_call_expect(TO_STR(zpal_nvm_read), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = (0x01400 + 2),  //FILE_ID_NODEROUTECAHE
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG3].value = 80; //FILE_SIZE_NODEROUTECAHE

  mock_call_expect(TO_STR(zpal_nvm_read), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = (0x01400 + 3),  //FILE_ID_NODEROUTECAHE
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG3].value = 80; //FILE_SIZE_NODEROUTECAHE

  //If file is not found in file system verify that the corresponding entries are
  //removed from FILE_ID_NODE_ROUTECACHE_EXIST
  {
    mock_call_expect(TO_STR(zpal_nvm_read), &pMock);
    pMock->return_code.value = ZPAL_STATUS_FAIL;
    pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
    pMock->expect_arg[ARG1].value = (0x01400 + 21),  //FILE_ID_NODEROUTECAHE
    pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
    pMock->expect_arg[ARG3].value = 80; //FILE_SIZE_NODEROUTECAHE

    mock_call_expect(TO_STR(zpal_nvm_write), &pMock);
    pMock->return_code.value = ZPAL_STATUS_OK;
    pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
    pMock->expect_arg[ARG1].value = 0x0000B,  //FILE_ID_NODE_ROUTECACHE_EXIST
    pMock->expect_arg[ARG2].pointer = node_routecache_exists_2;
    pMock->expect_arg[ARG3].value = 29; //sizeof(NODE_MASK_TYPE)
  }

  //Verify that the next expected RouteCache file is read
  mock_call_expect(TO_STR(zpal_nvm_read), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = (0x01400 + 24),  //FILE_ID_NODEROUTECAHE
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG3].value = 80; //FILE_SIZE_NODEROUTECAHE

  //Even though file (0x01400 + 25) is expected to exist it must not be read
  //since the RAM buffer is already full

  //Some other files are read subsequently
  uint32_t file_id[5] = {0x00006,  //FILE_ID_APP_ROUTE_LOCK_FLAG
                         0x00007,  //FILE_ID_ROUTE_SLAVE_SUC_FLAG
                         0x00008,  //FILE_ID_SUC_PENDING_UPDATE_FLAG
                         0x00009,  //FILE_ID_BRIDGE_NODE_FLAG
                         0x0000A   //FILE_ID_PENDING_DISCOVERY_FLAG
                        };

  for(uint32_t i=0; i<(sizeof(file_id)/sizeof(uint32_t)); i++)
  {
    mock_call_expect(TO_STR(zpal_nvm_read), &pMock);
    pMock->return_code.value = ZPAL_STATUS_OK;
    pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
    pMock->expect_arg[ARG1].value = file_id[i];
    pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
    pMock->expect_arg[ARG3].value = 29; //sizeof(NODE_MASK_TYPE)
  }

  CtrlStorageInit();

  //Verify that reading RouteCache from nodes in RAM buffer does not trigger read from NVM3

  ROUTECACHE_LINE  routeCacheLine;
  //file 0
  CtrlStorageGetRouteCache(ROUTE_CACHE_NORMAL,   1, &routeCacheLine);
  CtrlStorageGetRouteCache(ROUTE_CACHE_NORMAL,   2, &routeCacheLine);
  CtrlStorageGetRouteCache(ROUTE_CACHE_NORMAL,   3, &routeCacheLine);
  CtrlStorageGetRouteCache(ROUTE_CACHE_NORMAL,   4, &routeCacheLine);
  //file 2
  CtrlStorageGetRouteCache(ROUTE_CACHE_NORMAL,  18, &routeCacheLine);
  CtrlStorageGetRouteCache(ROUTE_CACHE_NORMAL,  20, &routeCacheLine);
  //file 3
  CtrlStorageGetRouteCache(ROUTE_CACHE_NORMAL,  32, &routeCacheLine);
  //file 24
  CtrlStorageGetRouteCache(ROUTE_CACHE_NORMAL, 196, &routeCacheLine);
  CtrlStorageGetRouteCache(ROUTE_CACHE_NORMAL, 198, &routeCacheLine);

  //Verify that reading RouteCache from nodes outside RAM buffer triggers rerad from NVM3

  //file 25
  mock_call_expect(TO_STR(zpal_nvm_read), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = (0x01400 + 25),  //FILE_ID_NODEROUTECAHE
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG3].value = 80; //FILE_SIZE_NODEROUTECAHE

  CtrlStorageGetRouteCache(ROUTE_CACHE_NORMAL, 201, &routeCacheLine);

  mock_call_expect(TO_STR(zpal_nvm_read), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = (0x01400 + 25),  //FILE_ID_NODEROUTECAHE
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG3].value = 80; //FILE_SIZE_NODEROUTECAHE

  CtrlStorageGetRouteCache(ROUTE_CACHE_NORMAL, 203, &routeCacheLine);

  mock_calls_verify();

  //Verify that calling StoreNodeRouteCacheFile(nodeID) where nodeID is in the RAM buffer will
  //trigger nvmr_writeData()
  mock_calls_clear();

  uint8_t t_nodeID = 18; //Node 18 is in the RAM cache

  mock_call_expect(TO_STR(zpal_nvm_write), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x01400 + ((t_nodeID - 1) / NODEROUTECACHES_PER_FILE);
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG3].value = 80; //sizeof(SNodeRouteCache) * NODEROUTECACHES_PER_FILE

  StoreNodeRouteCacheFile(t_nodeID);

  mock_calls_verify();


  //Verify that calling StoreNodeRouteCacheFile(nodeID) where nodeID outside the RAM buffer will
  //not trigger nvmr_writeData()
  mock_calls_clear();
  StoreNodeRouteCacheFile(200);
  mock_calls_verify();

  //Verify that calling StoreNodeRouteCacheBuffer() will trigger nvmr_writeData() on all files
  //in the RAM buffer.
  mock_calls_clear();

  mock_call_expect(TO_STR(zpal_nvm_write), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x01400 + (0 / NODEROUTECACHES_PER_FILE);
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG3].value = 80; //sizeof(SNodeRouteCache) * NODEROUTECACHES_PER_FILE

  mock_call_expect(TO_STR(zpal_nvm_write), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x01400 + (17 / NODEROUTECACHES_PER_FILE);
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG3].value = 80; //sizeof(SNodeRouteCache) * NODEROUTECACHES_PER_FILE

  mock_call_expect(TO_STR(zpal_nvm_write), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x01400 + (31 / NODEROUTECACHES_PER_FILE);
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG3].value = 80; //sizeof(SNodeRouteCache) * NODEROUTECACHES_PER_FILE

  mock_call_expect(TO_STR(zpal_nvm_write), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x01400 + (195 / NODEROUTECACHES_PER_FILE);
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG3].value = 80; //sizeof(SNodeRouteCache) * NODEROUTECACHES_PER_FILE

  StoreNodeRouteCacheBuffer();

  mock_calls_verify();
}


#define INCREMENT_NEW_FILE_PARAMETERS()                                                       \
    if (newByte == NEW_VALUES_PER_FILE - 1) {                                                 \
      printf("                             Switching new file. newFile: %d \n", newFile + 1); \
      newByte = 0;                                                                            \
      newFile++;                                                                              \
    } else {                                                                                  \
      newByte++;                                                                              \
    }


/*
 * To avoid variable length arrays in test below.
 */
#define OLD_FILE_COUNT          (1024 / 64)   // Max file count for version 2 is 16.
#define OLD_BYTES_PER_FILE      32            // File size in bytes
#define OLD_VALUES_PER_FILE     64            // Number of tx power values stored in one file.

#define NEW_VALUES_PER_FILE     32            // Number of tx power values stored in one file.


// Test migration of file system from version 7.11.0 to 7.12.1
void test_ControllerStorage_FileMigration(void)
{
  mock_calls_clear();

  static NODE_MASK_TYPE node_info_exists;
  //Set up nodemask for nodeinfo entries
  memset(node_info_exists, 0, sizeof(NODE_MASK_TYPE));
  NodeMaskSetBit(node_info_exists,   0);
  NodeMaskSetBit(node_info_exists,   2);
  NodeMaskSetBit(node_info_exists,   3);
  NodeMaskSetBit(node_info_exists,   9);
  NodeMaskSetBit(node_info_exists,  13);
  NodeMaskSetBit(node_info_exists,  14);
  NodeMaskSetBit(node_info_exists,  15);
  NodeMaskSetBit(node_info_exists, 100);

  uint8_t node_info_file_0[35];
  uint8_t node_info_file_2[35];
  uint8_t node_info_file_3[35];
  uint8_t node_info_file_9[35];
  uint8_t node_info_file_13[35];
  uint8_t node_info_file_14[35];
  uint8_t node_info_file_15[35];
  uint8_t node_info_file_100[35];

  memset(node_info_file_0,   0x0A, 35);
  memset(node_info_file_2,   0x0B, 35);
  memset(node_info_file_3,   0x0C, 35);
  memset(node_info_file_9,   0x0D, 35);
  memset(node_info_file_13,  0x0E, 35);
  memset(node_info_file_14,  0x0F, 35);
  memset(node_info_file_15,  0x1A, 35);
  memset(node_info_file_100, 0x1B, 35);

  //expected merged nodeinfo file nr 0
  uint8_t merged_node_info_file_0[140];
  memset(merged_node_info_file_0, 0xFF, 140);
  memcpy(merged_node_info_file_0 + (0 * 35), node_info_file_0, 35);
  memcpy(merged_node_info_file_0 + (2 * 35), node_info_file_2, 35);
  memcpy(merged_node_info_file_0 + (3 * 35), node_info_file_3, 35);

  //expected merged nodeinfo file nr 2
  uint8_t merged_node_info_file_2[140];
  memset(merged_node_info_file_2, 0xFF, 140);
  memcpy(merged_node_info_file_2 + (1 * 35), node_info_file_9, 35);

  //expected merged nodeinfo file nr 3
  uint8_t merged_node_info_file_3[140];
  memset(merged_node_info_file_3, 0xFF, 140);
  memcpy(merged_node_info_file_3 + (1 * 35), node_info_file_13, 35);
  memcpy(merged_node_info_file_3 + (2 * 35), node_info_file_14, 35);
  memcpy(merged_node_info_file_3 + (3 * 35), node_info_file_15, 35);

  //expected merged nodeinfo file nr 50
  uint8_t merged_node_info_file_25[140];
  memset(merged_node_info_file_25, 0xFF, 140);
  memcpy(merged_node_info_file_25 + (0 * 35), node_info_file_100, 35);


  static NODE_MASK_TYPE node_routecache_exists;
  //Set up nodemask for routecache entries
  memset(node_routecache_exists, 0, sizeof(NODE_MASK_TYPE));
  NodeMaskSetBit(node_routecache_exists,   0);
  NodeMaskSetBit(node_routecache_exists,   2);
  NodeMaskSetBit(node_routecache_exists,   3);
  NodeMaskSetBit(node_routecache_exists,   9);
  NodeMaskSetBit(node_routecache_exists,  13);
  NodeMaskSetBit(node_routecache_exists,  14);
  NodeMaskSetBit(node_routecache_exists,  15);
  NodeMaskSetBit(node_routecache_exists, 100);

  uint8_t node_routecache_file_0[10];
  uint8_t node_routecache_file_2[10];
  uint8_t node_routecache_file_3[10];
  uint8_t node_routecache_file_9[10];
  uint8_t node_routecache_file_13[10];
  uint8_t node_routecache_file_14[10];
  uint8_t node_routecache_file_15[10];
  uint8_t node_routecache_file_100[10];

  memset(node_routecache_file_0,   0x0A, 10);
  memset(node_routecache_file_2,   0x0B, 10);
  memset(node_routecache_file_3,   0x0C, 10);
  memset(node_routecache_file_9,   0x0D, 10);
  memset(node_routecache_file_13,  0x0E, 10);
  memset(node_routecache_file_14,  0x0F, 10);
  memset(node_routecache_file_15,  0x1A, 10);
  memset(node_routecache_file_100, 0x1B, 10);

  //expected merged routecache file nr 0
  uint8_t merged_node_routecache_file_0[80];
  memset(merged_node_routecache_file_0, 0xFF, 80);
  memcpy(merged_node_routecache_file_0 + (0 * 10), node_routecache_file_0, 10);
  memcpy(merged_node_routecache_file_0 + (2 * 10), node_routecache_file_2, 10);
  memcpy(merged_node_routecache_file_0 + (3 * 10), node_routecache_file_3, 10);

  //expected merged routecache file nr 1
  uint8_t merged_node_routecache_file_1[80];
  memset(merged_node_routecache_file_1, 0xFF, 80);
  memcpy(merged_node_routecache_file_1 + (1 * 10), node_routecache_file_9, 10);
  memcpy(merged_node_routecache_file_1 + (5 * 10), node_routecache_file_13, 10);
  memcpy(merged_node_routecache_file_1 + (6 * 10), node_routecache_file_14, 10);
  memcpy(merged_node_routecache_file_1 + (7 * 10), node_routecache_file_15, 10);

  //expected merged routecache file nr 12
  uint8_t merged_node_routecache_file_12[80];
  memset(merged_node_routecache_file_12, 0xFF, 80);
  memcpy(merged_node_routecache_file_12 + (4 * 10), node_routecache_file_100, 10);


  static NODE_MASK_TYPE node_routecache_empty;
  memset(node_routecache_empty, 0, sizeof(NODE_MASK_TYPE));

  uint8_t dummy;
  zpal_nvm_handle_t pFileSystem = &dummy;

  // initialize controller network
  mock_t * pMockNvmFileSystemRegister;
  mock_call_use_as_stub(TO_STR(NvmFileSystemInit));
  mock_call_expect(TO_STR(NvmFileSystemRegister), &pMockNvmFileSystemRegister);
  pMockNvmFileSystemRegister->return_code.pointer = pFileSystem;
  pMockNvmFileSystemRegister->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;

  // Array contains: Patch, Minor, Major, Filesystem
  uint8_t version_old[4] = {0x00, 0x0B, 0x07, 0x00};
  uint8_t version_new[4] = {ZW_VERSION_PATCH, ZW_VERSION_MINOR, ZW_VERSION_MAJOR, ZW_CONTROLLER_FILESYS_VERSION};

  mock_t * pMock;

  //Read old version number
  mock_call_expect(TO_STR(zpal_nvm_read), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00000; //FILE_ID_ZW_VERSION
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
  pMock->output_arg[ARG2].pointer = (uint8_t *)&version_old;
  pMock->expect_arg[ARG3].value = 4; //sizeof(uint32_t)

  //Verify migration of NodeInfo files
  mock_call_expect(TO_STR(zpal_nvm_read), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00005,  //FILE_ID_NODE_STORAGE_EXIST
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
  pMock->output_arg[ARG2].pointer = node_info_exists;
  pMock->expect_arg[ARG3].value = 29; //sizeof(NODE_MASK_TYPE)


  //Migrate files 0, 2 and 3 to new nr 0.
  mock_call_expect(TO_STR(zpal_nvm_read), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00100 + 0;  //Old FILE_ID_NODEINFO
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
  pMock->output_arg[ARG2].pointer = node_info_file_0;
  pMock->expect_arg[ARG3].value = 35; //originalFileSize

  mock_call_expect(TO_STR(zpal_nvm_read), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00100 + 2;  //Old FILE_ID_NODEINFO
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
  pMock->output_arg[ARG2].pointer = node_info_file_2;
  pMock->expect_arg[ARG3].value = 35; //originalFileSize

  mock_call_expect(TO_STR(zpal_nvm_read), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00100 + 3;  //Old FILE_ID_NODEINFO
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
  pMock->output_arg[ARG2].pointer = node_info_file_3;
  pMock->expect_arg[ARG3].value = 35; //originalFileSize

  mock_call_expect(TO_STR(zpal_nvm_write), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00200 + 0;  //New FILE_ID_NODEINFO
  pMock->expect_arg[ARG2].pointer = merged_node_info_file_0;
  pMock->expect_arg[ARG3].value = 35 * 4; //originalFileSize * 4

  mock_call_expect(TO_STR(zpal_nvm_erase_object), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00100 + 0;  //Old FILE_ID_NODEINFO

  mock_call_expect(TO_STR(zpal_nvm_erase_object), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00100 + 2;  //Old FILE_ID_NODEINFO

  mock_call_expect(TO_STR(zpal_nvm_erase_object), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00100 + 3;  //Old FILE_ID_NODEINFO


  //Migrate file 9 to new nr 2
  mock_call_expect(TO_STR(zpal_nvm_read), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00100 + 9;  //Old FILE_ID_NODEINFO
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
  pMock->output_arg[ARG2].pointer = node_info_file_9;
  pMock->expect_arg[ARG3].value = 35; //originalFileSize

  mock_call_expect(TO_STR(zpal_nvm_write), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00200 + 2;  //New FILE_ID_NODEINFO
  pMock->expect_arg[ARG2].pointer = merged_node_info_file_2;
  pMock->expect_arg[ARG3].value = 35 * 4; //originalFileSize * 4

  mock_call_expect(TO_STR(zpal_nvm_erase_object), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00100 + 9;  //Old FILE_ID_NODEINFO


  //Migrate files 13, 14 and 15 to new nr 3
  mock_call_expect(TO_STR(zpal_nvm_read), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00100 + 13;  //Old FILE_ID_NODEINFO
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
  pMock->output_arg[ARG2].pointer = node_info_file_13;
  pMock->expect_arg[ARG3].value = 35; //originalFileSize

  mock_call_expect(TO_STR(zpal_nvm_read), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00100 + 14;  //Old FILE_ID_NODEINFO
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
  pMock->output_arg[ARG2].pointer = node_info_file_14;
  pMock->expect_arg[ARG3].value = 35; //originalFileSize

  mock_call_expect(TO_STR(zpal_nvm_read), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00100 + 15;  //Old FILE_ID_NODEINFO
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
  pMock->output_arg[ARG2].pointer = node_info_file_15;
  pMock->expect_arg[ARG3].value = 35; //originalFileSize

  mock_call_expect(TO_STR(zpal_nvm_write), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00200 + 3;  //New FILE_ID_NODEINFO
  pMock->expect_arg[ARG2].pointer = merged_node_info_file_3;
  pMock->expect_arg[ARG3].value = 35 * 4; //originalFileSize * 4

  mock_call_expect(TO_STR(zpal_nvm_erase_object), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00100 + 13;  //Old FILE_ID_NODEINFO

  mock_call_expect(TO_STR(zpal_nvm_erase_object), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00100 + 14;  //Old FILE_ID_NODEINFO

  mock_call_expect(TO_STR(zpal_nvm_erase_object), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00100 + 15;  //Old FILE_ID_NODEINFO


  //Migrate file 100 to new nr 50
  mock_call_expect(TO_STR(zpal_nvm_read), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00100 + 100;  //Old FILE_ID_NODEINFO
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
  pMock->output_arg[ARG2].pointer = node_info_file_100;
  pMock->expect_arg[ARG3].value = 35; //originalFileSize

  mock_call_expect(TO_STR(zpal_nvm_write), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00200 + 25;  //New FILE_ID_NODEINFO
  pMock->expect_arg[ARG2].pointer = merged_node_info_file_25;
  pMock->expect_arg[ARG3].value = 35 * 4; //originalFileSize * 4

  mock_call_expect(TO_STR(zpal_nvm_erase_object), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00100 + 100;  //Old FILE_ID_NODEINFO


  //Verify migration of RouteCache files
  mock_call_expect(TO_STR(zpal_nvm_read), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x0000B,  //FILE_ID_NODE_ROUTECACHE_EXIST
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
  pMock->output_arg[ARG2].pointer = node_routecache_exists;
  pMock->expect_arg[ARG3].value = 29; //sizeof(NODE_MASK_TYPE)

  //Migrate files 0, 2 and 3 to new nr 0.
  mock_call_expect(TO_STR(zpal_nvm_read), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00400 + 0;  //Old FILE_ID_NODEROUTECAHE
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
  pMock->output_arg[ARG2].pointer = node_routecache_file_0;
  pMock->expect_arg[ARG3].value = 10; //originalFileSize

  mock_call_expect(TO_STR(zpal_nvm_read), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00400 + 2;  //Old FILE_ID_NODEROUTECAHE
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
  pMock->output_arg[ARG2].pointer = node_routecache_file_2;
  pMock->expect_arg[ARG3].value = 10; //originalFileSize

  mock_call_expect(TO_STR(zpal_nvm_read), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00400 + 3;  //Old FILE_ID_NODEROUTECAHE
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
  pMock->output_arg[ARG2].pointer = node_routecache_file_3;
  pMock->expect_arg[ARG3].value = 10; //originalFileSize

  mock_call_expect(TO_STR(zpal_nvm_write), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x01400 + 0;  //New FILE_ID_NODEROUTECAHE
  pMock->expect_arg[ARG2].pointer = merged_node_routecache_file_0;
  pMock->expect_arg[ARG3].value = 10 * 8 ; //originalFileSize * 8

  mock_call_expect(TO_STR(zpal_nvm_erase_object), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00400 + 0;  //Old FILE_ID_NODEROUTECAHE

  mock_call_expect(TO_STR(zpal_nvm_erase_object), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00400 + 2;  //Old FILE_ID_NODEROUTECAHE

  mock_call_expect(TO_STR(zpal_nvm_erase_object), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00400 + 3;  //Old FILE_ID_NODEROUTECAHE


  //Migrate files 9, 13, 14 and 15 to new nr 1.
  mock_call_expect(TO_STR(zpal_nvm_read), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00400 + 9;  //Old FILE_ID_NODEROUTECAHE
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
  pMock->output_arg[ARG2].pointer = node_routecache_file_9;
  pMock->expect_arg[ARG3].value = 10; //originalFileSize

  mock_call_expect(TO_STR(zpal_nvm_read), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00400 + 13;  //Old FILE_ID_NODEROUTECAHE
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
  pMock->output_arg[ARG2].pointer = node_routecache_file_13;
  pMock->expect_arg[ARG3].value = 10; //originalFileSize

  mock_call_expect(TO_STR(zpal_nvm_read), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00400 + 14;  //Old FILE_ID_NODEROUTECAHE
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
  pMock->output_arg[ARG2].pointer = node_routecache_file_14;
  pMock->expect_arg[ARG3].value = 10; //originalFileSize

  mock_call_expect(TO_STR(zpal_nvm_read), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00400 + 15;  //Old FILE_ID_NODEROUTECAHE
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
  pMock->output_arg[ARG2].pointer = node_routecache_file_15;
  pMock->expect_arg[ARG3].value = 10; //originalFileSize

  mock_call_expect(TO_STR(zpal_nvm_write), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x01400 + 1;  //New FILE_ID_NODEROUTECAHE
  pMock->expect_arg[ARG2].pointer = merged_node_routecache_file_1;
  pMock->expect_arg[ARG3].value = 10 * 8 ; //originalFileSize * 8

  mock_call_expect(TO_STR(zpal_nvm_erase_object), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00400 + 9;  //Old FILE_ID_NODEROUTECAHE

  mock_call_expect(TO_STR(zpal_nvm_erase_object), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00400 + 13;  //Old FILE_ID_NODEROUTECAHE

  mock_call_expect(TO_STR(zpal_nvm_erase_object), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00400 + 14;  //Old FILE_ID_NODEROUTECAHE

  mock_call_expect(TO_STR(zpal_nvm_erase_object), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00400 + 15;  //Old FILE_ID_NODEROUTECAHE


  //Migrate file 100 to new nr 25
  mock_call_expect(TO_STR(zpal_nvm_read), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00400 + 100;  //Old FILE_ID_NODEROUTECAHE
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
  pMock->output_arg[ARG2].pointer = node_routecache_file_100;
  pMock->expect_arg[ARG3].value = 10; //originalFileSize

  mock_call_expect(TO_STR(zpal_nvm_write), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x01400 + 12;  //New FILE_ID_NODEROUTECAHE
  pMock->expect_arg[ARG2].pointer = merged_node_routecache_file_12;
  pMock->expect_arg[ARG3].value = 10 * 8 ; //originalFileSize * 8

  mock_call_expect(TO_STR(zpal_nvm_erase_object), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00400 + 100;  //Old FILE_ID_NODEROUTECAHE



  /*
   * Migrate from File system V1 to V2.
   */

  //Migrate FILE_ID_CONTROLLERINFO
  mock_call_expect(TO_STR(zpal_nvm_read), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00004,  //FILE_ID_CONTROLLERINFO
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG3].value = 13; //sizeof(SControllerInfo_OLD)

  mock_call_expect(TO_STR(zpal_nvm_write), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00004,  //FILE_ID_CONTROLLERINFO
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG3].value = CONTROLLERINFO_FILE_SIZE; //sizeof(SControllerInfo)


  //Remove files related to S2 security since they were never in use for controllers.
  mock_call_expect(TO_STR(zpal_nvm_erase_object), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00010,  //FILE_ID_S2_KEYS

  mock_call_expect(TO_STR(zpal_nvm_erase_object), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00011,  //FILE_ID_S2_KEYCLASSES_ASSIGNED

  mock_call_expect(TO_STR(zpal_nvm_erase_object), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00012,  //FILE_ID_S2_MPAN

  mock_call_expect(TO_STR(zpal_nvm_erase_object), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00013,  //FILE_ID_S2_SPAN


  //Create FILE_ID_LRANGE_NODE_EXIST
  mock_call_expect(TO_STR(zpal_nvm_write), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x0000C,  //FILE_ID_LRANGE_NODE_EXIST
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG3].value = sizeof(LR_NODE_MASK_TYPE);



  /*
   * Migrate from File system V2 to V3.
   *
   * (This migration has been deprecated and removed)
   */



  /*
   * Migrate from File system V3 to V4.
   */

  {
    #define FILE_ID_LR_TX_POWER_BASE_V2       (0x00014)  // The base file Id for the TX Power files in nvm during file system version 2.
    #define FILE_ID_LR_TX_POWER_FILE_COUNT_V2 (1024 / 64)

    #define FILE_ID_LR_TX_POWER_BASE_V3       (0x02000)  // The base file Id for the TX Power files in nvm during file system version 3.
    #define FILE_ID_LR_TX_POWER_FILE_COUNT_V3 (1024 / 32)

    uint32_t fileIDs[]    = { FILE_ID_LR_TX_POWER_BASE_V2,        FILE_ID_LR_TX_POWER_BASE_V3       };
    uint32_t fileCount[]  = { FILE_ID_LR_TX_POWER_FILE_COUNT_V2,  FILE_ID_LR_TX_POWER_FILE_COUNT_V3 };

    for (int i = 0; i < 2; i++)  // We need to cover to version migrations.
    {
      // Read file by file for all valid fileID allocated for tx power storage.
      for (zpal_nvm_object_key_t fileID = fileIDs[i]; fileID < fileIDs[i] + fileCount[i]; fileID++)  // (1024 / 64) - 1 = 15, gives 16 file count for the old file system.
      {
        // Only delete as many as the number of files generated, not the entire fileID range as previously allocated.
        mock_call_expect(TO_STR(zpal_nvm_erase_object), &pMock);
        pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
        pMock->expect_arg[ARG1].value = fileID;
      }
    }
  }

  /*
   * Migrate from File system V4 to V5.
   */

  //Split the 1408 bytes long SUCNODELIST file into 176 bytes long files.
  //Only write files where valid info exists in the SUCNODELIST file.

  //Create legacy file area.
  uint8_t readSucList[1408]; //FILE_SIZE_SUCNODELIST_LEGACY_v4
  memset(readSucList, 0, sizeof(readSucList));

  //Add valid SUC values at position 13 to test migration
  SUC_UPDATE_ENTRY_STRUCT * pSUC = (SUC_UPDATE_ENTRY_STRUCT *)readSucList;
  pSUC = pSUC + 13;
  //dummy values for test
  pSUC->NodeID     = 33;
  pSUC->changeType = 1;
  memset(pSUC->nodeInfo, 0x55, SUC_UPDATE_NODEPARM_MAX);

  //Migration function reads all possible SUCs in old file
  for (uint8_t i =0 ; i < 64; i++) //v4 SUC_MAX_UPDATES == 64
  {
    mock_call_expect(TO_STR(zpal_nvm_read_object_part), &pMock);
    pMock->return_code.value = ZPAL_STATUS_OK;
    pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
    pMock->expect_arg[ARG1].value = 0x00003; //FILE_ID_SUCNODELIST_LEGACY_v4
    pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
    pMock->output_arg[ARG2].pointer = readSucList + i*sizeof(SUC_UPDATE_ENTRY_STRUCT);
    pMock->expect_arg[ARG3].value = i * sizeof(SUC_UPDATE_ENTRY_STRUCT);
    pMock->expect_arg[ARG4].value = sizeof(SUC_UPDATE_ENTRY_STRUCT);
  }

  //Data expected to be written as new file containing data on position 13.
  uint8_t writeSucList[176];
  memset(writeSucList, 0xff, 176);
  SUC_UPDATE_ENTRY_STRUCT * pWriteSUC = (SUC_UPDATE_ENTRY_STRUCT *)writeSucList;
  pWriteSUC = pWriteSUC + (13 % SUCNODES_PER_FILE);
  pWriteSUC->NodeID     = 33;
  pWriteSUC->changeType = 1;
  memset(pWriteSUC->nodeInfo, 0x55, SUC_UPDATE_NODEPARM_MAX);

  //Migration function writes 1 new file since it only found valid data on position 13.
  mock_call_expect(TO_STR(zpal_nvm_write), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x04000 + (13 / SUCNODES_PER_FILE); //FILE_ID_SUCNODELIST_BASE + file for SUC position 13
  pMock->expect_arg[ARG2].pointer = writeSucList;
  pMock->expect_arg[ARG3].value = 176; //sizeof(SSucNodeList)

  //Migration function  erases the old file
  mock_call_expect(TO_STR(zpal_nvm_erase_object), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00003; //FILE_ID_SUCNODELIST_LEGACY_v4

  /**
   * Write the new file system version number to NMV.
   */

  //Update version number after migration
  mock_call_expect(TO_STR(zpal_nvm_write), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00000; //FILE_ID_ZW_VERSION
  pMock->expect_arg[ARG2].pointer = (uint8_t *)&version_new;
  pMock->expect_arg[ARG3].value = 4; //sizeof(uint32_t)

  //Continue with verification of the rest of the files
  mock_call_expect(TO_STR(NVMCaretakerVerifySet), &pMock);
  pMock->return_code.value = ECTKR_STATUS_SUCCESS;  //return that the files were found
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->compare_rule_arg[ARG1] = COMPARE_NOT_NULL;

  //Read node info exist
  mock_call_expect(TO_STR(zpal_nvm_read), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00005,  //FILE_ID_NODE_STORAGE_EXIST
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG3].value = 29; //sizeof(NODE_MASK_TYPE)

  //Read back an empty routecache_exist nodemap to avoid testing initialization to RAM buffer again.
  //It has its own test function above.
  mock_call_expect(TO_STR(zpal_nvm_read), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x0000B,  //FILE_ID_NODE_ROUTECACHE_EXIST
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
  pMock->output_arg[ARG2].pointer = node_routecache_empty;
  pMock->expect_arg[ARG3].value = 29; //sizeof(NODE_MASK_TYPE)

  //Read Long Range node info exist
  mock_call_expect(TO_STR(zpal_nvm_read), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x0000C,  //FILE_ID_LRANGE_NODE_EXIST
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG3].value = sizeof(LR_NODE_MASK_TYPE);

  //Some other files are read subsequently
  uint32_t file_id[5] = {0x00006,  //FILE_ID_APP_ROUTE_LOCK_FLAG
                         0x00007,  //FILE_ID_ROUTE_SLAVE_SUC_FLAG
                         0x00008,  //FILE_ID_SUC_PENDING_UPDATE_FLAG
                         0x00009,  //FILE_ID_BRIDGE_NODE_FLAG
                         0x0000A   //FILE_ID_PENDING_DISCOVERY_FLAG
                        };

  for (uint32_t i = 0; i < (sizeof(file_id) / sizeof(uint32_t)); i++)
  {
    mock_call_expect(TO_STR(zpal_nvm_read), &pMock);
    pMock->return_code.value = ZPAL_STATUS_OK;
    pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
    pMock->expect_arg[ARG1].value = file_id[i];
    pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
    pMock->expect_arg[ARG3].value = 29;  //sizeof(NODE_MASK_TYPE)
  }

  //Read nodeInfo files to fill RAM caches
  mock_call_expect(TO_STR(zpal_nvm_read), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00200;  //FILE_ID_NODEINFO
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG3].value = 140; //file size

  //Read nodeInfo files to fill RAM caches
  mock_call_expect(TO_STR(zpal_nvm_read), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00200 + 2;  //FILE_ID_NODEINFO
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG3].value = 140; //file size

  //Read nodeInfo files to fill RAM caches
  mock_call_expect(TO_STR(zpal_nvm_read), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00200 + 3;  //FILE_ID_NODEINFO
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG3].value = 140; //file size

  //Read nodeInfo files to fill RAM caches
  mock_call_expect(TO_STR(zpal_nvm_read), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00200 + 25;  //FILE_ID_NODEINFO
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG3].value = 140; //file size

  CtrlStorageInit();

  mock_calls_verify();
}
