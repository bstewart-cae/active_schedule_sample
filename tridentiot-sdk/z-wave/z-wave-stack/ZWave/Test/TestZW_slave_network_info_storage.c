// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file TestZW_slave_network_info_storage.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include "SyncEvent.h"
#include <unity.h>
#include "mock_control.h"
#include "ZW_slave_network_info_storage.h"
#include "ZW_NVMCaretaker.h"
#include "NodeMask.h"
#include <ZW_nvm.h>
#include "Assert.h"
#include "SizeOf.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "ZW_lib_defines.h"
#include <ZW_Security_Scheme2.h>

void setUpSuite(void) {

}

void tearDownSuite(void) {

}

void setUp(void) {

}

void tearDown(void) {

}

// Testing  writing and reading destination return routes and speed information from nodes files

void test_SlaveStorage_ReturnRouteInfo(void)
{
  mock_calls_clear();
//--------------------------------------------------------------------------------------------------------
// File system init start
//--------------------------------------------------------------------------------------------------------

  static uint8_t dummy;
  zpal_nvm_handle_t pFileSystem = &dummy;

//--------------------------------------------------------------------------------------------------------
// -------------------------------------------------------filesyseminit end
//--------------------------------------------------------------------------------------------------------
  #define TEST_VAL1(INDEX)    (INDEX + 5)
  #define TEST_VAL2(INDEX)    (INDEX + 6)
  #define TEST_VAL3(INDEX)    (INDEX + 7)

  #define REPATER0_TEST_VAL(DESTN,ROUTEN)    (DESTN + ROUTEN + 10)
  #define REPATER1_TEST_VAL(DESTN,ROUTEN)    (DESTN + ROUTEN + 11)
  #define REPATER2_TEST_VAL(DESTN,ROUTEN)    (DESTN + ROUTEN + 12)
  #define REPATER3_TEST_VAL(DESTN,ROUTEN)    (DESTN + ROUTEN + 13)

 // initialize slave network
  mock_t * pMockNvmFileSystemRegister;
  mock_call_use_as_stub(TO_STR(NvmFileSystemInit));
  mock_call_expect(TO_STR(NvmFileSystemRegister), &pMockNvmFileSystemRegister);
  pMockNvmFileSystemRegister->return_code.pointer = pFileSystem;
  pMockNvmFileSystemRegister->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;

  mock_t * pMock;

  //Verify that FILE_ID_FILESYS_VERSION is read
  mock_call_expect(TO_STR(zpal_nvm_read), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00000; //FILE_ID_FILESYS_VERSION
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG3].value = 4; //sizeof(uint32_t)

  //Set current file system version as output
  uint32_t version = (ZW_SLAVE_FILESYS_VERSION << 24) | (ZW_VERSION_MAJOR << 16) | (ZW_VERSION_MINOR << 8) | (ZW_VERSION_PATCH);
  pMock->output_arg[ARG2].p = (uint8_t *)&version;


  mock_call_expect(TO_STR(NVMCaretakerVerifySet), &pMock);
  pMock->return_code.value = ECTKR_STATUS_SUCCESS;  //return that the files were found
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->compare_rule_arg[ARG1] = COMPARE_NOT_NULL;

  //Verify that FILE_ID_SLAVE_STORAGE_EXIST is read
  mock_call_expect(TO_STR(zpal_nvm_read), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00004; //FILE_ID_SLAVE_STORAGE_EXIST
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG3].value = 30; //sizeof(SRouteInfoFileMap)
  //Fill SlaveFileMap[] with zeros
  uint8_t SlaveFileMap_mock[30];
  memset(SlaveFileMap_mock, 0, 30);
  pMock->output_arg[ARG2].p = SlaveFileMap_mock;

  SlaveStorageInit();

  SSyncEvent * pRegisterEvent = (SSyncEvent*)pMockNvmFileSystemRegister->actual_arg[ARG0].pointer;
  TEST_ASSERT_NOT_NULL(pRegisterEvent);
  TEST_ASSERT_NOT_NULL(pRegisterEvent->uFunctor.pFunction);
  TEST_ASSERT_NULL(pRegisterEvent->pObject);


  uint8_t ReturnRouteInfo[sizeof(NVM_RETURN_ROUTE_STRUCT) + sizeof(NVM_RETURN_ROUTE_SPEED)];

  // test that when reading non existing files, all fields are zero
  for (uint8_t destIndex = 0; destIndex < MAX_RETURN_ROUTES_MAX_ENTRIES; destIndex++)
  {
    NVM_RETURN_ROUTE_STRUCT  t_ReturnRoute;
    NVM_RETURN_ROUTE_SPEED   t_ReturnRouteSpeed;

    SlaveStorageGetReturnRoute(destIndex, &t_ReturnRoute);
    SlaveStorageGetReturnRouteSpeed(destIndex, &t_ReturnRouteSpeed);
    //TDU 1.1  nodeID field in Test non existing return route stuct in return route info file is zero
    TEST_ASSERT_MESSAGE(0 == t_ReturnRoute.nodeID, "TDU 1.1");

    //TDU 1.2  repeaterList array in non existing return route stuct in return route info file is zero
    for (uint32_t i = 0; i< RETURN_ROUTE_MAX; i++)
    {
      for (uint32_t n= 0; n < MAX_REPEATERS; n++)
      {
        TEST_ASSERT_MESSAGE(0 == t_ReturnRoute.routeList[i].repeaterList[n], "TDU 1.2");
      }
    }

    //TDU 1.3  non existing return route speed stuct in return route info file is zero
    TEST_ASSERT_MESSAGE(0 == t_ReturnRouteSpeed.speed.bytes[0], "TDU 1.3");
    TEST_ASSERT_MESSAGE(0 == t_ReturnRouteSpeed.speed.bytes[1], "TDU 1.3");
  }

  mock_calls_verify();
  mock_calls_clear();

  // Test writing data to a return info file
  uint8_t ReturnRouteInfoBuffer[76];  //sizeof(SReturnRouteInfo) * RETURNROUTEINFOS_PER_FILE
  for (uint8_t destIndex = 0; destIndex < MAX_RETURN_ROUTES_MAX_ENTRIES; destIndex++)
  {
    NVM_RETURN_ROUTE_STRUCT  t_ReturnRoute;
    NVM_RETURN_ROUTE_SPEED   t_ReturnRouteSpeed;


    t_ReturnRoute.nodeID = TEST_VAL1(destIndex);
    for (uint8_t i = 0; i< RETURN_ROUTE_MAX; i++)
    {
      t_ReturnRoute.routeList[i].repeaterList[0] = REPATER0_TEST_VAL(destIndex, i);
      t_ReturnRoute.routeList[i].repeaterList[1] = REPATER1_TEST_VAL(destIndex, i);
      t_ReturnRoute.routeList[i].repeaterList[2] = REPATER2_TEST_VAL(destIndex, i);
      t_ReturnRoute.routeList[i].repeaterList[3] = REPATER3_TEST_VAL(destIndex, i);
    }

    memset(&t_ReturnRouteSpeed, 0, sizeof(t_ReturnRouteSpeed));

    if(0 == (destIndex % 4))
    {
      memset(ReturnRouteInfoBuffer, 0xFF, sizeof(ReturnRouteInfoBuffer));
    }
    memcpy(&ReturnRouteInfoBuffer[(destIndex % 4)*19], &t_ReturnRoute, sizeof(t_ReturnRoute));
    memcpy(&ReturnRouteInfoBuffer[(destIndex % 4)*19 + sizeof(t_ReturnRoute)], &t_ReturnRouteSpeed, sizeof(t_ReturnRouteSpeed));

    //Write a new RETURNROUTEINFO file
    mock_call_expect(TO_STR(zpal_nvm_write), &pMock);
    pMock->return_code.v = ZPAL_STATUS_OK;
    pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
    pMock->expect_arg[ARG1].v = 0x00200 + destIndex/4; //FILE_ID_RETURNROUTEINFO + destIndex/RETURNROUTEINFOS_PER_FILE
    pMock->expect_arg[ARG2].p = &ReturnRouteInfoBuffer;
    pMock->expect_arg[ARG3].v = 76; //sizeof(SReturnRouteInfo) * RETURNROUTEINFOS_PER_FILE

    mock_t * pMockSlaveInfoExists;
    //Update the SlaveInfoExist file
    mock_call_expect(TO_STR(zpal_nvm_write), &pMockSlaveInfoExists);
    pMockSlaveInfoExists->return_code.value = ZPAL_STATUS_OK;
    pMockSlaveInfoExists->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
    pMockSlaveInfoExists->expect_arg[ARG1].value = 0x00004; //FILE_ID_
    pMockSlaveInfoExists->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
    pMockSlaveInfoExists->expect_arg[ARG3].value = 30; //sizeof(SRouteInfoFileMap)

    SlaveStorageSetReturnRoute(destIndex, &t_ReturnRoute, &t_ReturnRouteSpeed);

    //Verify that correct bits are set in nodeInfoExist bitmap
    uint8_t * pSlaveInfoExists = (uint8_t *)(pMockSlaveInfoExists->actual_arg[ARG2].p);
    for(uint8_t slaveInfoByte = 0; slaveInfoByte < 29; slaveInfoByte++)
    {
      if(0 == destIndex)
      {
        //Verify that sucNodeInfoExist is set
        TEST_ASSERT_EQUAL_UINT8(0x01, *(pSlaveInfoExists + 29));
      }
      else if(slaveInfoByte < (destIndex - 1)/8)
      {
        TEST_ASSERT_EQUAL_UINT8(0xFF, *(pSlaveInfoExists + slaveInfoByte));
      }
      else if(slaveInfoByte == (destIndex - 1)/8)
      {
        //Check most recent bit
        TEST_ASSERT_EQUAL_UINT8(0x00FF >> (7 - (destIndex - 1)%8), *(pSlaveInfoExists + slaveInfoByte));
      }
      else
      {
        TEST_ASSERT_EQUAL_UINT8(0x00, *(pSlaveInfoExists + slaveInfoByte));
      }
    }
    //Verify that sucNodeInfoExist is still set
    TEST_ASSERT_EQUAL_UINT8(0x01, *(pSlaveInfoExists + 29));

    //test SlaveStorageSetReturnRouteSpeed()
    t_ReturnRouteSpeed.speed.bytes[0] = TEST_VAL2(destIndex);
    t_ReturnRouteSpeed.speed.bytes[1] = TEST_VAL3(destIndex);

    //Verify that RETURNROUTEINFO file is written
    mock_call_expect(TO_STR(zpal_nvm_write), &pMock);
    pMock->return_code.v = ZPAL_STATUS_OK;
    pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
    pMock->expect_arg[ARG1].v = 0x00200 + destIndex/4; //FILE_ID_RETURNROUTEINFO + destIndex/RETURNROUTEINFOS_PER_FILE
    pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
    pMock->expect_arg[ARG3].v = 76; //sizeof(SReturnRouteInfo) * RETURNROUTEINFOS_PER_FILE

    SlaveStorageSetReturnRouteSpeed(destIndex, &t_ReturnRouteSpeed);
    memcpy(&ReturnRouteInfoBuffer[(destIndex % 4)*19 + sizeof(t_ReturnRoute)], &t_ReturnRouteSpeed, sizeof(t_ReturnRouteSpeed));

    TEST_ASSERT_EQUAL_MEMORY(&ReturnRouteInfoBuffer, pMock->actual_arg[ARG2].p, sizeof(ReturnRouteInfoBuffer));
  }

  mock_calls_verify();
  mock_calls_clear();

  for (uint8_t destIndex = 0; destIndex < MAX_RETURN_ROUTES_MAX_ENTRIES; destIndex++)
  {
    NVM_RETURN_ROUTE_STRUCT  t_ReturnRoute;
    NVM_RETURN_ROUTE_SPEED   t_ReturnRouteSpeed;

    if (0 == (destIndex % 4))
    {
      for(uint8_t j=0; j<4; j++) //loop over number of ReturnRouteInfo structs that are stored per file
      {
        *(ReturnRouteInfo) = TEST_VAL1(destIndex + j);
        for (uint8_t i = 0; i< RETURN_ROUTE_MAX; i++)
        {
          *(ReturnRouteInfo + i*4 + 1) = REPATER0_TEST_VAL(destIndex + j, i);
          *(ReturnRouteInfo + i*4 + 2) = REPATER1_TEST_VAL(destIndex + j, i);
          *(ReturnRouteInfo + i*4 + 3) = REPATER2_TEST_VAL(destIndex + j, i);
          *(ReturnRouteInfo + i*4 + 4) = REPATER3_TEST_VAL(destIndex + j, i);
        }
        *(ReturnRouteInfo + sizeof(t_ReturnRoute)) = TEST_VAL2(destIndex + j);
        *(ReturnRouteInfo + sizeof(t_ReturnRoute) + 1) = TEST_VAL3(destIndex + j);
        memcpy(&ReturnRouteInfoBuffer[j * sizeof(ReturnRouteInfo)], ReturnRouteInfo, sizeof(ReturnRouteInfo));
      }

      //Verify that RETURNROUTEINFO file is read
      mock_call_expect(TO_STR(zpal_nvm_read), &pMock);
      pMock->return_code.v = ZPAL_STATUS_OK;
      pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
      pMock->expect_arg[ARG1].v = 0x00200 + destIndex/4; //FILE_ID_RETURNROUTEINFO + destIndex/RETURNROUTEINFOS_PER_FILE
      pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
      pMock->output_arg[ARG2].p = ReturnRouteInfoBuffer;
      pMock->expect_arg[ARG3].v = 76; //sizeof(SReturnRouteInfo) * RETURNROUTEINFOS_PER_FILE
    }

    SlaveStorageGetReturnRoute(destIndex, &t_ReturnRoute);

    SlaveStorageGetReturnRouteSpeed(destIndex, &t_ReturnRouteSpeed);

    //TDU 1.4  nodeID field has the correct test value for all files
    TEST_ASSERT_MESSAGE(TEST_VAL1(destIndex) == t_ReturnRoute.nodeID, "TDU 1.4");

    //TDU 1.5  repeaterList array have the correct values for all files
    for (uint8_t i = 0; i< RETURN_ROUTE_MAX; i++)
    {
      TEST_ASSERT_MESSAGE(REPATER0_TEST_VAL(destIndex, i) == t_ReturnRoute.routeList[i].repeaterList[0], "TDU 1.5");
      TEST_ASSERT_MESSAGE(REPATER1_TEST_VAL(destIndex, i) == t_ReturnRoute.routeList[i].repeaterList[1], "TDU 1.5");
      TEST_ASSERT_MESSAGE(REPATER2_TEST_VAL(destIndex, i) == t_ReturnRoute.routeList[i].repeaterList[2], "TDU 1.5");
      TEST_ASSERT_MESSAGE(REPATER3_TEST_VAL(destIndex, i) == t_ReturnRoute.routeList[i].repeaterList[3], "TDU 1.5");
    }

    //TDU 1.6  speed information have the correct test values for all files
    TEST_ASSERT_MESSAGE(TEST_VAL2(destIndex) == t_ReturnRouteSpeed.speed.bytes[0], "TDU 1.6");
    TEST_ASSERT_MESSAGE(TEST_VAL3(destIndex) == t_ReturnRouteSpeed.speed.bytes[1], "TDU 1.6");
  }

  // delete all return route info files
  {
    mock_t * pMockSlaveInfoExists;
    //Update the SlaveInfoExist file
    mock_call_expect(TO_STR(zpal_nvm_write), &pMockSlaveInfoExists);
    pMockSlaveInfoExists->return_code.value = ZPAL_STATUS_OK;
    pMockSlaveInfoExists->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
    pMockSlaveInfoExists->expect_arg[ARG1].value = 0x00004; //FILE_ID_
    pMockSlaveInfoExists->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
    pMockSlaveInfoExists->expect_arg[ARG3].value = 30; //sizeof(SRouteInfoFileMap)

    //Delete SUC info
    SlaveStorageDeleteSucReturnRouteInfo();

    //Verify that sucNodeInfoExist is not set
    uint8_t * pSlaveInfoExists = (uint8_t *)(pMockSlaveInfoExists->actual_arg[ARG2].p);
    TEST_ASSERT_EQUAL_UINT8(0x00, *(pSlaveInfoExists + 29));

    //Verify that all 58 files are erased. 58 = 232/4 (max number of routes / routes per file)
    for(uint8_t i = 0; i < ZW_MAX_NODES/4; i++)
    {
      mock_call_expect(TO_STR(zpal_nvm_erase_object), &pMock);
      pMock->return_code.value = ZPAL_STATUS_OK;
      pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
      pMock->expect_arg[ARG1].value = 0x00200 + i; //FILE_ID_RETURNROUTEINFO
    }

    //Update the SlaveInfoExist file
    mock_call_expect(TO_STR(zpal_nvm_write), &pMockSlaveInfoExists);
    pMockSlaveInfoExists->return_code.value = ZPAL_STATUS_OK;
    pMockSlaveInfoExists->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
    pMockSlaveInfoExists->expect_arg[ARG1].value = 0x00004; //FILE_ID_
    pMockSlaveInfoExists->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
    pMockSlaveInfoExists->expect_arg[ARG3].value = 30; //sizeof(SRouteInfoFileMap)

    SlaveStorageDeleteAllReturnRouteInfo();

    //Verify that slaveInfoExists has been cleared
    for(uint8_t i = 0; i<30; i++)
    {
      TEST_ASSERT_EQUAL_UINT8(0x00, *(pSlaveInfoExists + i));
    }
  }

  mock_calls_verify();
}





// Testing  writing and reading destinations id file

void test_SlaveStorage_DestinationId(void)
{
  mock_calls_clear();
//--------------------------------------------------------------------------------------------------------
// File system init start
//--------------------------------------------------------------------------------------------------------

  static uint8_t dummy;
  zpal_nvm_handle_t pFileSystem = &dummy;

//--------------------------------------------------------------------------------------------------------
// -------------------------------------------------------filesyseminit end
//--------------------------------------------------------------------------------------------------------
  #define DEST_TEST_VAL(INDEX)    (INDEX + 100)

 // initialize controller network
  mock_t * pMockNvmFileSystemRegister;
  mock_call_expect(TO_STR(NvmFileSystemRegister), &pMockNvmFileSystemRegister);
  pMockNvmFileSystemRegister->return_code.pointer = pFileSystem;
  pMockNvmFileSystemRegister->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;

  mock_t * pMock;

  //Verify that FILE_ID_FILESYS_VERSION is read
  mock_call_expect(TO_STR(zpal_nvm_read), &pMock);
  pMock->return_code.value = ZPAL_STATUS_FAIL;  //Make zpal_nvm_read() return error code to test that file system formats if file not found
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00000; //FILE_ID_FILESYS_VERSION
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG3].value = 4; //sizeof(uint32_t)

  //Verify that the file system is formated because the file was not found
  mock_call_expect(TO_STR(NvmFileSystemFormat), &pMock);
  pMock->return_code.value = true;

  mock_call_expect(TO_STR(NVMCaretakerVerifySet), &pMock);
  pMock->return_code.value = ECTKR_STATUS_SUCCESS;  //return that no file was found
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->compare_rule_arg[ARG1] = COMPARE_NOT_NULL;

  //Verify that FILE_ID_SLAVE_STORAGE_EXIST is read
  mock_call_expect(TO_STR(zpal_nvm_read), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00004; //FILE_ID_SLAVE_STORAGE_EXIST
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG3].value = 30; //sizeof(SRouteInfoFileMap)

  SlaveStorageInit();


  uint8_t t_DestinationID[ZW_MAX_CACHED_RETURN_ROUTE_DESTINATIONS];

  //Verify that FILE_ID_RETURNROUTESDESTINATIONS is read
  mock_call_expect(TO_STR(zpal_nvm_read), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00002; //FILE_ID_RETURNROUTESDESTINATIONS
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG3].value = 5; //ZW_MAX_CACHED_RETURN_ROUTE_DESTINATIONS

  SlaveStorageGetRouteDestinations(t_DestinationID);


  //Verify that FILE_ID_RETURNROUTESDESTINATIONS is written
  mock_call_expect(TO_STR(zpal_nvm_write), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00002; //FILE_ID_RETURNROUTESDESTINATIONS
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG3].value = 5; //ZW_MAX_CACHED_RETURN_ROUTE_DESTINATIONS

  SlaveStorageSetRouteDestinations(t_DestinationID);

  mock_calls_verify();
}



// Testing the reaction to Nvm File System formatted callback
// Modules should write all default files with default values, as when initing to empty FS.
// Test assumes that other test cases verify that init create the default files with default content.
void test_SlaveStorage_filesystem_format_callback(void)
{
  mock_calls_clear();
  //--------------------------------------------------------------------------------------------------------
  // File system init start
  //--------------------------------------------------------------------------------------------------------

  static uint8_t dummy;
  zpal_nvm_handle_t pFileSystem = &dummy;

  //--------------------------------------------------------------------------------------------------------
  // -------------------------------------------------------filesyseminit end
  //--------------------------------------------------------------------------------------------------------


  // register file system
  mock_t * pMockNvmFileSystemRegister;
  mock_call_expect(TO_STR(NvmFileSystemRegister), &pMockNvmFileSystemRegister); // TDU 3.0 Test DUT registers with NVM on init
  pMockNvmFileSystemRegister->return_code.pointer = pFileSystem;
  pMockNvmFileSystemRegister->compare_rule_arg[ARG0] = COMPARE_NOT_NULL; // TDU 3.1 Test DUT registers with non null pointer and no object

  mock_t * pMock;

  //Verify that FILE_ID_FILESYS_VERSION is read
  mock_call_expect(TO_STR(zpal_nvm_read), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00000; //FILE_ID_FILESYS_VERSION
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG3].value = 4; //sizeof(uint32_t)
  //Set current file system version as output
  uint32_t version = (ZW_SLAVE_FILESYS_VERSION << 24) | (ZW_VERSION_MAJOR << 16) | (ZW_VERSION_MINOR << 8) | (ZW_VERSION_PATCH);
  pMock->output_arg[ARG2].p = (uint8_t *)&version;

  // FileSet - allows FileSystemCaretaker to validate required files
  static const SObjectDescriptor t_aFileDescriptors[] = {
    { .ObjectKey = 0x00002,   .iDataSize = 5  },  //FILE_ID_RETURNROUTESDESTINATIONS
    { .ObjectKey = 0x00003,   .iDataSize = 8  },  //FILE_ID_SLAVEINFO
    { .ObjectKey = 0x00004,   .iDataSize = 30 },  //FILE_ID_SLAVE_FILE_MAP
    { .ObjectKey = 0x00010,   .iDataSize = 64 },  //FILE_ID_KEYS
    { .ObjectKey = 0x00011,   .iDataSize = 1  },  //FILE_ID_S2_KEYCLASSES_ASSIGNED
    { .ObjectKey = 0x00014,   .iDataSize = 32 },  //FILE_ID_ZW_PRIVATE_KEY
    { .ObjectKey = 0x00015,   .iDataSize = 32 },  //FILE_ID_ZW_PUBLIC_KEY
   };

  SObjectSet tFileSet = {
    .pFileSystem = pFileSystem,
    .iObjectCount = sizeof_array(t_aFileDescriptors),
    .pObjectDescriptors = t_aFileDescriptors
  };

  ECaretakerStatus returnFileStatus[7] = {ECTKR_STATUS_SUCCESS,
                                          ECTKR_STATUS_UNABLE_TO_AQUIRE_HANDLE,
                                          ECTKR_STATUS_SIZE_MISMATCH,
                                          ECTKR_STATUS_SUCCESS,
                                          ECTKR_STATUS_SUCCESS
  };

  mock_call_expect(TO_STR(NVMCaretakerVerifySet), &pMock);
  pMock->return_code.value = ECTKR_STATUS_FILESYS_MISMATCH;  //return that no file was found
  pMock->expect_arg[ARG0].p = &tFileSet;
  pMock->compare_rule_arg[ARG1] = COMPARE_NOT_NULL;
  pMock->output_arg[ARG1].p = &returnFileStatus;

  //Verify that FILE_ID_SLAVEINFO is written with default values
  uint8_t defaultSlaveInfo[8] = {0,0,0,0,0,0,0,0};
  mock_call_expect(TO_STR(zpal_nvm_write), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00003; //FILE_ID_SLAVEINFO
  pMock->expect_arg[ARG2].pointer = defaultSlaveInfo;
  pMock->expect_arg[ARG3].value = 8; //sizeof(SSlaveInfo)

  //Verify that FILE_ID_SLAVE_FILE_MAP is read and appended
  size_t oldFileSize = 24;
  mock_call_expect(TO_STR(zpal_nvm_get_object_size), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00004; //FILE_ID_SLAVE_STORAGE_EXIST
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
  pMock->output_arg[ARG2].pointer = &oldFileSize; //sizeof(SRouteInfoFileMap)

  //Verify that FILE_ID_SLAVE_FILE_MAP is read
  mock_call_expect(TO_STR(zpal_nvm_read), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00004; //FILE_ID_SLAVE_FILE_MAP
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG3].value = 30; //sizeof(SRouteInfoFileMap)

  //Verify that FILE_ID_SLAVE_FILE_MAP is written with default values
  mock_call_expect(TO_STR(zpal_nvm_write), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00004; //FILE_ID_SLAVE_FILE_MAP
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG3].value = 30; //sizeof(SRouteInfoFileMap)

  //Verify that FILE_ID_SLAVE_FILE_MAP is read
  mock_call_expect(TO_STR(zpal_nvm_read), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00004; //FILE_ID_SLAVE_FILE_MAP
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG3].value = 30; //sizeof(SRouteInfoFileMap)

  SlaveStorageInit();

  SSyncEvent * pRegisterEvent = (SSyncEvent*)(pMockNvmFileSystemRegister->actual_arg[ARG0].pointer);
  TEST_ASSERT_NOT_NULL(pRegisterEvent);
  TEST_ASSERT_NOT_NULL(pRegisterEvent->uFunctor.pFunction); // TDU 3.1
  TEST_ASSERT_NULL(pRegisterEvent->pObject);                // TDU 3.1

  //Invoking pRegisterEvent must lead to a call to WriteDefault()
  //Verify that default data is written to files.

  uint8_t routeArray[] = {0, 0, 0, 0, 0};
  mock_call_expect(TO_STR(zpal_nvm_write), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00002; //FILE_ID_RETURNROUTESDESTINATIONS
  pMock->expect_arg[ARG2].p = routeArray;
  pMock->expect_arg[ARG3].value = 5; //sizeof(SReturnRouteDestinationsCached)

  uint8_t slaveInfoArray[] = {0, 0, 0, 0, 0, 0, 0, 0};
  mock_call_expect(TO_STR(zpal_nvm_write), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00003; //FILE_ID_SLAVEINFO
  pMock->expect_arg[ARG2].p = slaveInfoArray;
  pMock->expect_arg[ARG3].value = 8; //sizeof(SSlaveInfo)

  uint8_t nodeMaskArray[30];
  memset(nodeMaskArray, 0, 30);
  mock_call_expect(TO_STR(zpal_nvm_write), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00004; //FILE_ID_SLAVE_STORAGE_EXIST
  pMock->expect_arg[ARG2].p = nodeMaskArray;
  pMock->expect_arg[ARG3].value = 30; //sizeof(SRouteInfoFileMap)

  mock_call_expect(TO_STR(zpal_nvm_write), &pMock);
  uint8_t keys[64];
  memset(&keys, 0xFF, 64);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00010; //FILE_ID_S2_KEYS
  pMock->expect_arg[ARG2].p = &keys;
  pMock->expect_arg[ARG3].value = 64; //sizeof(uint32_t)

  uint8_t keyclassAssigned = 0;
  mock_call_expect(TO_STR(zpal_nvm_write), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00011; //FILE_ID_S2_KEYCLASSES_ASSIGNED
  pMock->expect_arg[ARG2].p = &keyclassAssigned;
  pMock->expect_arg[ARG3].value = 1; //sizeof(uint8_t)

  version = (0x05 << 24) | (ZW_VERSION_MAJOR << 16) | (ZW_VERSION_MINOR << 8) | (ZW_VERSION_PATCH);
  mock_call_expect(TO_STR(zpal_nvm_write), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00000; //FILE_ID_ZW_VERSION  (Should always be 0x00000, must NEVER change)
  pMock->expect_arg[ARG2].p = &version;
  pMock->expect_arg[ARG3].value = 4; //sizeof(uint32_t)

  SyncEventInvoke(pRegisterEvent);

  mock_calls_verify();
}


// Test migration of file system from version 7.11.0 to 7.17.1
void test_SlaveStorage_FileMigration(void)
{
  mock_calls_clear();

  typedef struct SRouteInfoFileMap
  {
    NODE_MASK_TYPE nodeInfoExist;
    uint8_t sucNodeInfoExist;
  }SRouteInfoFileMap;

  static SRouteInfoFileMap SlaveFileMap;

  //Set up nodemask for nodeinfo entries
  memset(&SlaveFileMap, 0, sizeof(SlaveFileMap));

  SlaveFileMap.sucNodeInfoExist = 1;                  //nodeID 0
  ZW_NodeMaskSetBit(SlaveFileMap.nodeInfoExist,   2); //nodeID 2
  ZW_NodeMaskSetBit(SlaveFileMap.nodeInfoExist,   3); //nodeID 3
  ZW_NodeMaskSetBit(SlaveFileMap.nodeInfoExist,   9); //nodeID 9
  ZW_NodeMaskSetBit(SlaveFileMap.nodeInfoExist,  13); //nodeID 13
  ZW_NodeMaskSetBit(SlaveFileMap.nodeInfoExist,  14); //nodeID 14
  ZW_NodeMaskSetBit(SlaveFileMap.nodeInfoExist,  15); //nodeID 15
  ZW_NodeMaskSetBit(SlaveFileMap.nodeInfoExist, 100); //nodeID 100

  uint8_t route_info_file_0[19];
  uint8_t route_info_file_2[19];
  uint8_t route_info_file_3[19];
  uint8_t route_info_file_9[19];
  uint8_t route_info_file_13[19];
  uint8_t route_info_file_14[19];
  uint8_t route_info_file_15[19];
  uint8_t route_info_file_100[19];

  memset(route_info_file_0,   0x0A, 19);
  memset(route_info_file_2,   0x0B, 19);
  memset(route_info_file_3,   0x0C, 19);
  memset(route_info_file_9,   0x0D, 19);
  memset(route_info_file_13,  0x0E, 19);
  memset(route_info_file_14,  0x0F, 19);
  memset(route_info_file_15,  0x1A, 19);
  memset(route_info_file_100, 0x1B, 19);

  //expected merged nodeinfo file nr 0
  uint8_t merged_route_info_file_0[76];
  memset(merged_route_info_file_0, 0xFF, 76);
  memcpy(merged_route_info_file_0 + (0 * 19), route_info_file_0, 19);
  memcpy(merged_route_info_file_0 + (2 * 19), route_info_file_2, 19);
  memcpy(merged_route_info_file_0 + (3 * 19), route_info_file_3, 19);

  //expected merged nodeinfo file nr 2
  uint8_t merged_route_info_file_2[76];
  memset(merged_route_info_file_2, 0xFF, 76);
  memcpy(merged_route_info_file_2 + (1 * 19), route_info_file_9, 19);

  //expected merged nodeinfo file nr 3
  uint8_t merged_route_info_file_3[76];
  memset(merged_route_info_file_3, 0xFF, 76);
  memcpy(merged_route_info_file_3 + (1 * 19), route_info_file_13, 19);
  memcpy(merged_route_info_file_3 + (2 * 19), route_info_file_14, 19);
  memcpy(merged_route_info_file_3 + (3 * 19), route_info_file_15, 19);

  //expected merged nodeinfo file nr 50
  uint8_t merged_route_info_file_25[76];
  memset(merged_route_info_file_25, 0xFF, 76);
  memcpy(merged_route_info_file_25 + (0 * 19), route_info_file_100, 19);


  static uint8_t dummy;
  zpal_nvm_handle_t pFileSystem = &dummy;

  // initialize controller network
  mock_t * pMockNvmFileSystemRegister;
  mock_call_use_as_stub(TO_STR(NvmFileSystemInit));
  mock_call_expect(TO_STR(NvmFileSystemRegister), &pMockNvmFileSystemRegister);
  pMockNvmFileSystemRegister->return_code.pointer = pFileSystem;
  pMockNvmFileSystemRegister->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;

  // Array contains: Patch, Minor, Major, Filesystem
  uint8_t version_old[4] = {0x00, 0x0B, 0x07, 0x00};
  uint8_t version_new[4] = {ZW_VERSION_PATCH, ZW_VERSION_MINOR, ZW_VERSION_MAJOR, ZW_SLAVE_FILESYS_VERSION};

  mock_t * pMock;

  //Read old version number
  mock_call_expect(TO_STR(zpal_nvm_read), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00000; //FILE_ID_ZW_VERSION
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
  pMock->output_arg[ARG2].pointer = version_old;
  pMock->expect_arg[ARG3].value = 4; //sizeof(uint32_t)

  //Verify migration of NodeInfo files
  mock_call_expect(TO_STR(zpal_nvm_read), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00004,  //FILE_ID_SLAVE_FILE_MAP
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
  pMock->output_arg[ARG2].pointer = &SlaveFileMap;
  pMock->expect_arg[ARG3].value = 30; //sizeof(SRouteInfoFileMap)


  //Migrate files 0, 2 and 3 to new nr 0.
  mock_call_expect(TO_STR(zpal_nvm_read), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00100 + 0;  //Old FILE_ID_RETURNROUTEINFO
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
  pMock->output_arg[ARG2].pointer = route_info_file_0;
  pMock->expect_arg[ARG3].value = 19; //originalFileSize

  mock_call_expect(TO_STR(zpal_nvm_read), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00100 + 2;  //Old FILE_ID_RETURNROUTEINFO
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
  pMock->output_arg[ARG2].pointer = route_info_file_2;
  pMock->expect_arg[ARG3].value = 19; //originalFileSize

  mock_call_expect(TO_STR(zpal_nvm_read), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00100 + 3;  //Old FILE_ID_RETURNROUTEINFO
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
  pMock->output_arg[ARG2].pointer = route_info_file_3;
  pMock->expect_arg[ARG3].value = 19; //originalFileSize

  mock_call_expect(TO_STR(zpal_nvm_write), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00200 + 0;  //New FILE_ID_RETURNROUTEINFO
  pMock->expect_arg[ARG2].pointer = merged_route_info_file_0;
  pMock->expect_arg[ARG3].value = 19 * 4; //originalFileSize * 4

  mock_call_expect(TO_STR(zpal_nvm_erase_object), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00100 + 0;  //Old FILE_ID_RETURNROUTEINFO

  mock_call_expect(TO_STR(zpal_nvm_erase_object), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00100 + 2;  //Old FILE_ID_RETURNROUTEINFO

  mock_call_expect(TO_STR(zpal_nvm_erase_object), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00100 + 3;  //Old FILE_ID_RETURNROUTEINFO


  //Migrate file 9 to new nr 2
  mock_call_expect(TO_STR(zpal_nvm_read), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00100 + 9;  //Old FILE_ID_RETURNROUTEINFO
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
  pMock->output_arg[ARG2].pointer = route_info_file_9;
  pMock->expect_arg[ARG3].value = 19; //originalFileSize

  mock_call_expect(TO_STR(zpal_nvm_write), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00200 + 2;  //New FILE_ID_RETURNROUTEINFO
  pMock->expect_arg[ARG2].pointer = merged_route_info_file_2;
  pMock->expect_arg[ARG3].value = 19 * 4; //originalFileSize * 4

  mock_call_expect(TO_STR(zpal_nvm_erase_object), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00100 + 9;  //Old FILE_ID_RETURNROUTEINFO


  //Migrate files 13, 14 and 15 to new nr 3
  mock_call_expect(TO_STR(zpal_nvm_read), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00100 + 13;  //Old FILE_ID_RETURNROUTEINFO
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
  pMock->output_arg[ARG2].pointer = route_info_file_13;
  pMock->expect_arg[ARG3].value = 19; //originalFileSize

  mock_call_expect(TO_STR(zpal_nvm_read), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00100 + 14;  //Old FILE_ID_RETURNROUTEINFO
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
  pMock->output_arg[ARG2].pointer = route_info_file_14;
  pMock->expect_arg[ARG3].value = 19; //originalFileSize

  mock_call_expect(TO_STR(zpal_nvm_read), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00100 + 15;  //Old FILE_ID_RETURNROUTEINFO
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
  pMock->output_arg[ARG2].pointer = route_info_file_15;
  pMock->expect_arg[ARG3].value = 19; //originalFileSize

  mock_call_expect(TO_STR(zpal_nvm_write), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00200 + 3;  //New FILE_ID_RETURNROUTEINFO
  pMock->expect_arg[ARG2].pointer = merged_route_info_file_3;
  pMock->expect_arg[ARG3].value = 19 * 4; //originalFileSize * 4

  mock_call_expect(TO_STR(zpal_nvm_erase_object), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00100 + 13;  //Old FILE_ID_RETURNROUTEINFO

  mock_call_expect(TO_STR(zpal_nvm_erase_object), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00100 + 14;  //Old FILE_ID_RETURNROUTEINFO

  mock_call_expect(TO_STR(zpal_nvm_erase_object), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00100 + 15;  //Old FILE_ID_RETURNROUTEINFO


  //Migrate file 100 to new nr 25
  mock_call_expect(TO_STR(zpal_nvm_read), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00100 + 100;  //Old FILE_ID_RETURNROUTEINFO
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
  pMock->output_arg[ARG2].pointer = route_info_file_100;
  pMock->expect_arg[ARG3].value = 19; //originalFileSize

  mock_call_expect(TO_STR(zpal_nvm_write), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00200 + 25;  //New FILE_ID_RETURNROUTEINFO
  pMock->expect_arg[ARG2].pointer = merged_route_info_file_25;
  pMock->expect_arg[ARG3].value = 19 * 4; //originalFileSize * 4

  mock_call_expect(TO_STR(zpal_nvm_erase_object), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00100 + 100;  //Old FILE_ID_RETURNROUTEINFO

  //Migrate FILE_ID_SLAVEINFO
  typedef struct SSlaveInfo_old
  {
    uint8_t homeID[HOMEID_LENGTH];
    uint8_t nodeID;
    uint8_t smartStartState;
  }SSlaveInfo_old;

  typedef struct SSlaveInfo_new
  {
    uint8_t homeID[HOMEID_LENGTH];
    uint16_t nodeID;
    uint8_t smartStartState;
    uint8_t primaryLongRangeChannelId;
  }SSlaveInfo_new;


  SSlaveInfo_old  oldSlaveInfo;
  SSlaveInfo_new  newSlaveInfo;

  memset((uint8_t *)&newSlaveInfo, 0xFF, sizeof(newSlaveInfo));

  oldSlaveInfo.homeID[0] = 0x1A;
  oldSlaveInfo.homeID[1] = 0x2B;
  oldSlaveInfo.homeID[2] = 0x3C;
  oldSlaveInfo.homeID[3] = 0x4D;
  oldSlaveInfo.nodeID = 0x5E;
  oldSlaveInfo.smartStartState = 0x6F;

  newSlaveInfo.homeID[0] = oldSlaveInfo.homeID[0];
  newSlaveInfo.homeID[1] = oldSlaveInfo.homeID[1];
  newSlaveInfo.homeID[2] = oldSlaveInfo.homeID[2];
  newSlaveInfo.homeID[3] = oldSlaveInfo.homeID[3];
  newSlaveInfo.nodeID = oldSlaveInfo.nodeID;
  newSlaveInfo.smartStartState = oldSlaveInfo.smartStartState;
  newSlaveInfo.primaryLongRangeChannelId = 0;

  mock_call_expect(TO_STR(zpal_nvm_read), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00003,  //FILE_ID_SLAVEINFO
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
  pMock->output_arg[ARG2].pointer = (uint8_t *)&oldSlaveInfo;
  pMock->expect_arg[ARG3].value = 6; //Old size of SSlaveInfo

  mock_call_expect(TO_STR(zpal_nvm_write), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00003,  //FILE_ID_SLAVEINFO
  pMock->expect_arg[ARG2].pointer = (uint8_t *)&newSlaveInfo;
  pMock->expect_arg[ARG3].value = 8; //New size of SSlaveInfo

  uint8_t oldKeys[128];
  memset(oldKeys, 0, 64);
  memset(&oldKeys[64], 0x55, 64);

  //Migrate FILE_ID_S2_KEYS
  mock_call_expect(TO_STR(zpal_nvm_read), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00010,  //FILE_ID_S2_KEYS
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
  pMock->output_arg[ARG2].pointer = oldKeys;
  pMock->expect_arg[ARG3].value = 128; //Old size of Ss2_keys

  mock_call_expect(TO_STR(zpal_nvm_write), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00010,  //FILE_ID_S2_KEYS
  pMock->expect_arg[ARG2].pointer = &oldKeys[64];
  pMock->expect_arg[ARG3].value = 64; //New size of Ss2_keys


  //Migrate FILE_ID_S2_SPAN
  //lnode and rnode in SPAN was changed from uint8_t to uint16_t
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

  uint8_t old_span[440];
  memset(&old_span, 0, sizeof(old_span));
  SPAN_Old * pSPAN_Old;

  for(uint32_t i=0; i < 10; i++)
  {
    pSPAN_Old = (SPAN_Old *)(old_span + (i * 44));

    memset((uint8_t *)&(pSPAN_Old->d), 0x49 + i, sizeof(pSPAN_Old->d));

    pSPAN_Old->lnode = 0x5A + i;
    pSPAN_Old->rnode = 0x6B + i;

    pSPAN_Old->rx_seq = 0x7C + i;
    pSPAN_Old->tx_seq = 0x8D + i;
    pSPAN_Old->class_id = 0x9E + i;
    pSPAN_Old->state = SPAN_INSTANTIATE + i;
  }

  mock_call_expect(TO_STR(zpal_nvm_read), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00013;  //FILE_ID_S2_SPAN
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
  pMock->output_arg[ARG2].pointer = &old_span;
  pMock->expect_arg[ARG3].value = sizeof(old_span);


  struct SPAN new_span[10];
  memset(&new_span, 0, sizeof(new_span));

  for(uint32_t i=0; i < 10; i++)
  {
    memset((uint8_t *)&(new_span[i].d), 0x49 + i, sizeof(new_span[i].d));

    new_span[i].lnode = 0x005A + i;  //now 16 bit
    new_span[i].rnode = 0x006B + i;

    new_span[i].rx_seq = 0x7C + i;
    new_span[i].tx_seq = 0x8D + i;
    new_span[i].class_id = 0x9E + i;
    new_span[i].state = SPAN_INSTANTIATE + i;

    mock_call_expect(TO_STR(zpal_nvm_write), &pMock);
    pMock->return_code.value = ZPAL_STATUS_OK;
    pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
    pMock->expect_arg[ARG1].value = 0x00400 + i;  //FILE_ID_S2_SPAN_BASE + i
    pMock->expect_arg[ARG2].pointer = &new_span[i];
    pMock->expect_arg[ARG3].value = sizeof(new_span[i]);
  }

  mock_call_expect(TO_STR(zpal_nvm_erase_object), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00013;  //FILE_ID_S2_SPAN

  //Migrate FILE_ID_S2_MPAN
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

  uint8_t mockpan[240];    //size of old mpan_file
  memset(&mockpan, 0, 240);
  MPAN_Old * pMPAN_Old;

  for(uint32_t i=0; i < 10; i++)
  {
    pMPAN_Old = (MPAN_Old * )(mockpan + (i * 24));

    pMPAN_Old->owner_id = 0x5A + i;
    pMPAN_Old->group_id = 0x6B + i;
    memset(pMPAN_Old->inner_state, 0x7C + i, 16);
    pMPAN_Old->class_id = 0x8D + i;
    pMPAN_Old->state = MPAN_SET + i;
  }

  mock_call_expect(TO_STR(zpal_nvm_read), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value =  0x00012; //FILE_ID_S2_MPAN
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
  pMock->output_arg[ARG2].pointer = &mockpan;
  pMock->expect_arg[ARG3].value = sizeof(mockpan);

  struct MPAN mpan_new_file[10];
  memset(&mpan_new_file, 0, sizeof(mpan_new_file));

  for(uint32_t i=0; i < 10; i++)
  {
    mpan_new_file[i].owner_id = 0x005A + i;  //now 16 bit
    mpan_new_file[i].group_id = 0x6B + i;
    memset(mpan_new_file[i].inner_state, 0x7C + i, 16);
    mpan_new_file[i].class_id = 0x8D + i;
    mpan_new_file[i].state = MPAN_SET + i;

    mock_call_expect(TO_STR(zpal_nvm_write), &pMock);
    pMock->return_code.value = ZPAL_STATUS_OK;
    pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
    pMock->expect_arg[ARG1].value = 0x00500 + i;  //FILE_ID_S2_MPAN_BASE + i
    pMock->expect_arg[ARG2].p = &mpan_new_file[i];
    pMock->expect_arg[ARG3].value = sizeof(mpan_new_file[i]);
  }

  mock_call_expect(TO_STR(zpal_nvm_erase_object), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00012;  //FILE_ID_S2_MPAN


  //Update version number after migration
  mock_call_expect(TO_STR(zpal_nvm_write), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00000; //FILE_ID_ZW_VERSION
  pMock->expect_arg[ARG2].pointer = version_new;
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
  pMock->expect_arg[ARG1].value = 0x00004,  //FILE_ID_SLAVE_FILE_MAP
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG3].value = 30; //sizeof(SRouteInfoFileMap)

  SlaveStorageInit();

  mock_calls_verify();
}


// Test migration of file system from version 7.12.1 to 7.17.1
void test_SlaveStorage_FileMigration_2(void)
{
  mock_calls_clear();

  typedef struct SRouteInfoFileMap
  {
    NODE_MASK_TYPE nodeInfoExist;
    uint8_t sucNodeInfoExist;
  }SRouteInfoFileMap;


  static uint8_t dummy;
  zpal_nvm_handle_t pFileSystem = &dummy;

  // initialize controller network
  mock_t * pMockNvmFileSystemRegister;
  mock_call_use_as_stub(TO_STR(NvmFileSystemInit));
  mock_call_expect(TO_STR(NvmFileSystemRegister), &pMockNvmFileSystemRegister);
  pMockNvmFileSystemRegister->return_code.pointer = pFileSystem;
  pMockNvmFileSystemRegister->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;

  uint8_t version_old[4] = {0x01, 0x0C, 0x07, 0x01};  //0x01070C01
  uint8_t version_new[4] = {ZW_VERSION_PATCH, ZW_VERSION_MINOR, ZW_VERSION_MAJOR, ZW_SLAVE_FILESYS_VERSION};

  mock_t * pMock;

  //Read old version number
  mock_call_expect(TO_STR(zpal_nvm_read), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00000; //FILE_ID_ZW_VERSION
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
  pMock->output_arg[ARG2].pointer = version_old;
  pMock->expect_arg[ARG3].value = 4; //sizeof(uint32_t)


  //Remove unused RETURNROUTEINFO files.
  static SRouteInfoFileMap SlaveFileMap;

  //Set up nodemask for nodeinfo entries
  //The routeinfo for the nodes below is located in files 0x00200 + 0, 2, 3 and 50
  memset(&SlaveFileMap, 0, sizeof(SlaveFileMap));
  SlaveFileMap.sucNodeInfoExist = 1;                  //nodeID 0   file 0x00200
  ZW_NodeMaskSetBit(SlaveFileMap.nodeInfoExist,   2); //nodeID 2   file 0x00200
  ZW_NodeMaskSetBit(SlaveFileMap.nodeInfoExist,   3); //nodeID 3   file 0x00200
  ZW_NodeMaskSetBit(SlaveFileMap.nodeInfoExist,   9); //nodeID 9   file 0x00202
  ZW_NodeMaskSetBit(SlaveFileMap.nodeInfoExist,  13); //nodeID 13  file 0x00203
  ZW_NodeMaskSetBit(SlaveFileMap.nodeInfoExist,  14); //nodeID 14  file 0x00203
  ZW_NodeMaskSetBit(SlaveFileMap.nodeInfoExist,  15); //nodeID 15  file 0x00203
  ZW_NodeMaskSetBit(SlaveFileMap.nodeInfoExist, 100); //nodeID 100 file 0x00232

  //List of found return route info file object keys.
  //Files 0x00200 + 1, 4, 24 and 26 should be deleted
  //since they have no entries in the SlaveFileMap
  zpal_nvm_object_key_t rri_objectKeys[59];
  rri_objectKeys[0] = 0x00200 + 0;
  rri_objectKeys[1] = 0x00200 + 1;
  rri_objectKeys[2] = 0x00200 + 2;
  rri_objectKeys[3] = 0x00200 + 3;
  rri_objectKeys[4] = 0x00200 + 4;
  rri_objectKeys[5] = 0x00200 + 24;
  rri_objectKeys[6] = 0x00200 + 25;
  rri_objectKeys[7] = 0x00200 + 26;

  mock_call_expect(TO_STR(zpal_nvm_read), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00004;  //FILE_ID_SLAVE_FILE_MAP
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
  pMock->output_arg[ARG2].pointer = &SlaveFileMap;
  pMock->expect_arg[ARG3].value = 30; //sizeof(SRouteInfoFileMap)

  mock_call_expect(TO_STR(zpal_nvm_enum_objects), &pMock);
  pMock->return_code.value = 8;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->compare_rule_arg[ARG1] = COMPARE_NOT_NULL;
  pMock->output_arg[ARG1].pointer = rri_objectKeys;
  pMock->expect_arg[ARG2].value = 59;
  pMock->expect_arg[ARG3].value = 0x00200;  //FILE_ID_RETURNROUTEINFO
  pMock->expect_arg[ARG4].value = 0x0023A;  //FILE_ID_RETURNROUTEINFO + 58

  mock_call_expect(TO_STR(zpal_nvm_erase_object), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00200 + 1;  //FILE_ID_RETURNROUTEINFO + 1

  mock_call_expect(TO_STR(zpal_nvm_erase_object), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00200 + 4;  //FILE_ID_RETURNROUTEINFO + 4

  mock_call_expect(TO_STR(zpal_nvm_erase_object), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00200 + 24;  //FILE_ID_RETURNROUTEINFO + 24

  mock_call_expect(TO_STR(zpal_nvm_erase_object), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00200 + 26;  //FILE_ID_RETURNROUTEINFO + 26

  //Migrate FILE_ID_SLAVEINFO
  typedef struct SSlaveInfo_old
  {
    uint8_t homeID[HOMEID_LENGTH];
    uint8_t nodeID;
    uint8_t smartStartState;
  }SSlaveInfo_old;

  typedef struct SSlaveInfo_new
  {
    uint8_t homeID[HOMEID_LENGTH];
    uint16_t nodeID;
    uint8_t smartStartState;
    uint8_t primaryLongRangeChannelId;
  }SSlaveInfo_new;


  SSlaveInfo_old  oldSlaveInfo;
  SSlaveInfo_new  newSlaveInfo;

  memset((uint8_t *)&newSlaveInfo, 0xFF, sizeof(newSlaveInfo));

  oldSlaveInfo.homeID[0] = 0x1A;
  oldSlaveInfo.homeID[1] = 0x2B;
  oldSlaveInfo.homeID[2] = 0x3C;
  oldSlaveInfo.homeID[3] = 0x4D;
  oldSlaveInfo.nodeID = 0x5E;
  oldSlaveInfo.smartStartState = 0x6F;

  newSlaveInfo.homeID[0] = oldSlaveInfo.homeID[0];
  newSlaveInfo.homeID[1] = oldSlaveInfo.homeID[1];
  newSlaveInfo.homeID[2] = oldSlaveInfo.homeID[2];
  newSlaveInfo.homeID[3] = oldSlaveInfo.homeID[3];
  newSlaveInfo.nodeID = oldSlaveInfo.nodeID;
  newSlaveInfo.smartStartState = oldSlaveInfo.smartStartState;
  newSlaveInfo.primaryLongRangeChannelId = 0;

  mock_call_expect(TO_STR(zpal_nvm_read), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00003,  //FILE_ID_SLAVEINFO
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
  pMock->output_arg[ARG2].pointer = (uint8_t *)&oldSlaveInfo;
  pMock->expect_arg[ARG3].value = 6; //Old size of SSlaveInfo

  mock_call_expect(TO_STR(zpal_nvm_write), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00003,  //FILE_ID_SLAVEINFO
  pMock->expect_arg[ARG2].pointer = (uint8_t *)&newSlaveInfo;
  pMock->expect_arg[ARG3].value = 8; //New size of SSlaveInfo

  uint8_t oldKeys[128];
  memset(oldKeys, 0, 64);
  memset(&oldKeys[64], 0x55, 64);

  //Migrate FILE_ID_S2_KEYS
  mock_call_expect(TO_STR(zpal_nvm_read), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00010,  //FILE_ID_S2_KEYS
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
  pMock->output_arg[ARG2].pointer = oldKeys;
  pMock->expect_arg[ARG3].value = 128; //Old size of Ss2_keys

  mock_call_expect(TO_STR(zpal_nvm_write), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00010,  //FILE_ID_S2_KEYS
  pMock->expect_arg[ARG2].pointer = &oldKeys[64];
  pMock->expect_arg[ARG3].value = 64; //New size of Ss2_keys


  //Migrate FILE_ID_S2_SPAN
  //Files empty of data should be deleted and no new files should be written.
  uint8_t old_span[440];
  memset(&old_span, 0, sizeof(old_span));

  mock_call_expect(TO_STR(zpal_nvm_read), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00013;  //FILE_ID_S2_SPAN
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
  pMock->output_arg[ARG2].pointer = &old_span;
  pMock->expect_arg[ARG3].value = sizeof(old_span);

  mock_call_expect(TO_STR(zpal_nvm_erase_object), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00013;  //FILE_ID_S2_SPAN


  //Migrate FILE_ID_S2_MPAN
  //Files empty of data should be deleted and no new files should be written.
  uint8_t mockpan[240];    //sizeof(mpan_file)
  memset(&mockpan, 0, 240);

  mock_call_expect(TO_STR(zpal_nvm_read), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value =  0x00012; //FILE_ID_S2_MPAN
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
  pMock->output_arg[ARG2].pointer = &mockpan;
  pMock->expect_arg[ARG3].value = sizeof(mockpan); //sizeof(mpan_file)

  mock_call_expect(TO_STR(zpal_nvm_erase_object), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00012;  //FILE_ID_S2_MPAN


  //Update version number after migration
  mock_call_expect(TO_STR(zpal_nvm_write), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 0x00000; //FILE_ID_ZW_VERSION
  pMock->expect_arg[ARG2].pointer = version_new;
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
  pMock->expect_arg[ARG1].value = 0x00004,  //FILE_ID_SLAVE_FILE_MAP
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG3].value = 30; //sizeof(SRouteInfoFileMap)

  SlaveStorageInit();

  mock_calls_verify();
}


#define NR_OF_MPAN_FILES 4
#define FILE_ID_S2_MPAN_BASE 0x00500
#define DUMMY_VALUE 5

void test_SlaveStorage_GetS2MpanTable(void)
{
  mock_calls_clear();

  mock_t * pMock;

  struct S2 s2_ctx;

  //Calls zpal_nvm_enum_objects() and finds the files below.
  zpal_nvm_object_key_t mpan_objectKeys[NR_OF_MPAN_FILES] ={FILE_ID_S2_MPAN_BASE,
                                                            FILE_ID_S2_MPAN_BASE + 1,
                                                            FILE_ID_S2_MPAN_BASE + 5,
                                                            FILE_ID_S2_MPAN_BASE + 9};


  mock_call_expect(TO_STR(zpal_nvm_enum_objects), &pMock);
  pMock->return_code.value        = NR_OF_MPAN_FILES;
  pMock->compare_rule_arg[ARG0]   = COMPARE_NOT_NULL;
  pMock->compare_rule_arg[ARG1]   = COMPARE_NOT_NULL;
  pMock->output_arg[ARG1].pointer = mpan_objectKeys;
  pMock->expect_arg[ARG2].value   = MPAN_TABLE_SIZE;
  pMock->expect_arg[ARG3].value   = FILE_ID_S2_MPAN_BASE;
  pMock->expect_arg[ARG4].value   = FILE_ID_S2_MPAN_BASE + MPAN_TABLE_SIZE -1;

  struct MPAN mpan[NR_OF_MPAN_FILES];

  //Reads the files found.
  for (int i=0; i<NR_OF_MPAN_FILES; i++)
  {
    memset(&mpan[i], DUMMY_VALUE + i, sizeof(struct MPAN));

    mock_call_expect(TO_STR(zpal_nvm_read), &pMock);
    pMock->return_code.value        = ZPAL_STATUS_OK;
    pMock->compare_rule_arg[ARG0]   = COMPARE_NOT_NULL;
    pMock->expect_arg[ARG1].value   = mpan_objectKeys[i];
    pMock->compare_rule_arg[ARG2]   = COMPARE_NOT_NULL;
    pMock->output_arg[ARG2].pointer = &mpan[i];
    pMock->expect_arg[ARG3].value   = sizeof(struct MPAN);
  }

  StorageGetS2MpanTable((uint8_t*)&s2_ctx.mpan_table);


  //Verify that the dummy values in mpan[] were written to s2_ctx.mpan_table
  for (int i=0; i<NR_OF_MPAN_FILES; i++)
  {
    int table_index = mpan_objectKeys[i] - FILE_ID_S2_MPAN_BASE;
    uint32_t compare = memcmp(&s2_ctx.mpan_table[table_index], &mpan[i], sizeof(struct MPAN));
    TEST_ASSERT_EQUAL_UINT32(0 ,compare);
  }

  //Verify that all other positions in s2_ctx.mpan_table have status MPAN_NOT_USED
  for (int i=0; i<MPAN_TABLE_SIZE; i++)
  {
    bool fileExists = false;
    for (int j =0; j<NR_OF_MPAN_FILES; j++)
    {
      if (mpan_objectKeys[j] == (FILE_ID_S2_MPAN_BASE + i))
      {
        fileExists = true;
        break;
      }
    }
    if (!fileExists)
    {
      TEST_ASSERT(MPAN_NOT_USED == s2_ctx.mpan_table[i].state);
    }
  }

  mock_calls_verify();
}


#define NR_OF_SPAN_FILES 4
#define FILE_ID_S2_SPAN_BASE 0x00400

void test_SlaveStorage_GetS2SpanTable(void)
{
  mock_calls_clear();

  mock_t * pMock;

  struct S2 s2_ctx;

  //Calls zpal_nvm_enum_objects() and finds the files below.
  zpal_nvm_object_key_t span_objectKeys[NR_OF_SPAN_FILES] ={FILE_ID_S2_SPAN_BASE,
                                                            FILE_ID_S2_SPAN_BASE + 1,
                                                            FILE_ID_S2_SPAN_BASE + 5,
                                                            FILE_ID_S2_SPAN_BASE + 9};


  mock_call_expect(TO_STR(zpal_nvm_enum_objects), &pMock);
  pMock->return_code.value        = NR_OF_SPAN_FILES;
  pMock->compare_rule_arg[ARG0]   = COMPARE_NOT_NULL;
  pMock->compare_rule_arg[ARG1]   = COMPARE_NOT_NULL;
  pMock->output_arg[ARG1].pointer = span_objectKeys;
  pMock->expect_arg[ARG2].value   = SPAN_TABLE_SIZE;
  pMock->expect_arg[ARG3].value   = FILE_ID_S2_SPAN_BASE;
  pMock->expect_arg[ARG4].value   = FILE_ID_S2_SPAN_BASE + SPAN_TABLE_SIZE -1;

  struct SPAN span[NR_OF_SPAN_FILES];

  //Reads the files found.
  for (int i=0; i<NR_OF_SPAN_FILES; i++)
  {
    memset(&span[i], DUMMY_VALUE + i, sizeof(struct SPAN));

    mock_call_expect(TO_STR(zpal_nvm_read), &pMock);
    pMock->return_code.value        = ZPAL_STATUS_OK;
    pMock->compare_rule_arg[ARG0]   = COMPARE_NOT_NULL;
    pMock->expect_arg[ARG1].value   = span_objectKeys[i];
    pMock->compare_rule_arg[ARG2]   = COMPARE_NOT_NULL;
    pMock->output_arg[ARG2].pointer = &span[i];
    pMock->expect_arg[ARG3].value   = sizeof(struct SPAN);
  }

  StorageGetS2SpanTable((uint8_t*)&s2_ctx.span_table);


  //Verify that the dummy values in span[] were written to s2_ctx.span_table
  for (int i=0; i<NR_OF_SPAN_FILES; i++)
  {
    int table_index = span_objectKeys[i] - FILE_ID_S2_SPAN_BASE;
    uint32_t compare = memcmp(&s2_ctx.span_table[table_index], &span[i], sizeof(struct SPAN));
    TEST_ASSERT_EQUAL_UINT32(0 ,compare);
  }

  //Verify that all other positions in s2_ctx.span_table have status SPAN_NOT_USED
  for (int i=0; i<SPAN_TABLE_SIZE; i++)
  {
    bool fileExists = false;
    for (int j =0; j<NR_OF_SPAN_FILES; j++)
    {
      if (span_objectKeys[j] == (FILE_ID_S2_SPAN_BASE + i))
      {
        fileExists = true;
        break;
      }
    }
    if (!fileExists)
    {
      TEST_ASSERT(SPAN_NOT_USED == s2_ctx.span_table[i].state);
    }
  }

  mock_calls_verify();
}


void test_SlaveStorage_SetS2MpanTable(void)
{
  mock_calls_clear();

  mock_t * pMock;

  struct S2 s2_ctx;
  memset(&s2_ctx, 0, sizeof(struct S2));

  //Set valid data on index 1 and 5. Files for other indexes must be deleted if they exist.
  memset(&s2_ctx.mpan_table[0], 1, sizeof(s2_ctx.mpan_table[0]));
  memset(&s2_ctx.mpan_table[5], 1, sizeof(s2_ctx.mpan_table[0]));

  //Calls zpal_nvm_enum_objects() and finds the files below.
  zpal_nvm_object_key_t mpan_objectKeys[NR_OF_MPAN_FILES] ={FILE_ID_S2_MPAN_BASE,
                                                            FILE_ID_S2_MPAN_BASE + 1,
                                                            FILE_ID_S2_MPAN_BASE + 5,
                                                            FILE_ID_S2_MPAN_BASE + 9};


  mock_call_expect(TO_STR(zpal_nvm_enum_objects), &pMock);
  pMock->return_code.value        = NR_OF_MPAN_FILES;
  pMock->compare_rule_arg[ARG0]   = COMPARE_NOT_NULL;
  pMock->compare_rule_arg[ARG1]   = COMPARE_NOT_NULL;
  pMock->output_arg[ARG1].pointer = mpan_objectKeys;
  pMock->expect_arg[ARG2].value   = MPAN_TABLE_SIZE;
  pMock->expect_arg[ARG3].value   = FILE_ID_S2_MPAN_BASE;
  pMock->expect_arg[ARG4].value   = FILE_ID_S2_MPAN_BASE + MPAN_TABLE_SIZE -1;

  //Write to FILE_ID_S2_MPAN_BASE + 0
  mock_call_expect(TO_STR(zpal_nvm_write), &pMock);
  pMock->return_code.value      = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = FILE_ID_S2_MPAN_BASE + 0;
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG3].value = sizeof(struct MPAN);

  //Delete FILE_ID_S2_MPAN_BASE + 1
  mock_call_expect(TO_STR(zpal_nvm_erase_object), &pMock);
  pMock->return_code.value      = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = FILE_ID_S2_MPAN_BASE + 1;

  //Write to FILE_ID_S2_MPAN_BASE + 5
  mock_call_expect(TO_STR(zpal_nvm_write), &pMock);
  pMock->return_code.value      = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = FILE_ID_S2_MPAN_BASE + 5;
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG3].value = sizeof(struct MPAN);

  //Delete FILE_ID_S2_MPAN_BASE + 9
  mock_call_expect(TO_STR(zpal_nvm_erase_object), &pMock);
  pMock->return_code.value      = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = FILE_ID_S2_MPAN_BASE + 9;

  StorageSetS2MpanTable((uint8_t*)&s2_ctx.mpan_table);

  mock_calls_verify();
}

void test_SlaveStorage_SetS2SpanTable(void)
{
  mock_calls_clear();

  mock_t * pMock;

  struct S2 s2_ctx;
  memset(&s2_ctx, 0, sizeof(struct S2));

  //Set valid data on index 1 and 5. Files for other indexes must be deleted if they exist.
  memset(&s2_ctx.span_table[0], 1, sizeof(s2_ctx.span_table[0]));
  memset(&s2_ctx.span_table[5], 1, sizeof(s2_ctx.span_table[0]));

  //Calls zpal_nvm_enum_objects() and finds the files below.
  zpal_nvm_object_key_t span_objectKeys[NR_OF_SPAN_FILES] ={FILE_ID_S2_SPAN_BASE,
                                                            FILE_ID_S2_SPAN_BASE + 1,
                                                            FILE_ID_S2_SPAN_BASE + 5,
                                                            FILE_ID_S2_SPAN_BASE + 9};


  mock_call_expect(TO_STR(zpal_nvm_enum_objects), &pMock);
  pMock->return_code.value        = NR_OF_SPAN_FILES;
  pMock->compare_rule_arg[ARG0]   = COMPARE_NOT_NULL;
  pMock->compare_rule_arg[ARG1]   = COMPARE_NOT_NULL;
  pMock->output_arg[ARG1].pointer = span_objectKeys;
  pMock->expect_arg[ARG2].value   = SPAN_TABLE_SIZE;
  pMock->expect_arg[ARG3].value   = FILE_ID_S2_SPAN_BASE;
  pMock->expect_arg[ARG4].value   = FILE_ID_S2_SPAN_BASE + SPAN_TABLE_SIZE -1;

  //Write to FILE_ID_S2_SPAN_BASE + 0
  mock_call_expect(TO_STR(zpal_nvm_write), &pMock);
  pMock->return_code.value      = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = FILE_ID_S2_SPAN_BASE + 0;
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG3].value = sizeof(struct SPAN);

  //Delete FILE_ID_S2_SPAN_BASE + 1
  mock_call_expect(TO_STR(zpal_nvm_erase_object), &pMock);
  pMock->return_code.value      = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = FILE_ID_S2_SPAN_BASE + 1;

  //Write to FILE_ID_S2_SPAN_BASE + 5
  mock_call_expect(TO_STR(zpal_nvm_write), &pMock);
  pMock->return_code.value      = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = FILE_ID_S2_SPAN_BASE + 5;
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG3].value = sizeof(struct SPAN);

  //Delete FILE_ID_S2_SPAN_BASE + 9
  mock_call_expect(TO_STR(zpal_nvm_erase_object), &pMock);
  pMock->return_code.value      = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = FILE_ID_S2_SPAN_BASE + 9;

  StorageSetS2SpanTable((uint8_t*)&s2_ctx.span_table);

  mock_calls_verify();
}
