// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file TestZW_NVMCaretaker.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include "ZW_NVMCaretaker.h"
#include "mock_control.h"

//#define DEBUGPRINT
#include <DebugPrint.h>
#include "string.h"

void setUpSuite(void) {

}

void tearDownSuite(void) {

}

void setUp(void) {

}

void tearDown(void) {

}

void test_NVMCaretakerVerifyObject_invalid_inputs(void)
{
  mock_calls_clear();
  mock_t * pMock;

  zpal_nvm_handle_t fileSystem;
  ECaretakerStatus return_val;

  zpal_nvm_object_key_t fileID = 1;
  size_t fileLength = 10;

  SObjectDescriptor objectDescriptor;
  objectDescriptor.ObjectKey = fileID;
  objectDescriptor.iDataSize = fileLength;

  //File not found
  mock_call_expect(TO_STR(zpal_nvm_get_object_size), &pMock);
  pMock->return_code.value = ZPAL_STATUS_FAIL;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = fileID;
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;

  return_val = NVMCaretakerVerifyObject(&fileSystem, &objectDescriptor);

  TEST_ASSERT(2 == return_val); //ECTKR_STATUS_UNABLE_TO_AQUIRE_HANDLE

  mock_calls_verify();
  mock_calls_clear();

  //File has unexpected size
  size_t wrongFileLength= 5;

  mock_call_expect(TO_STR(zpal_nvm_get_object_size), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = fileID;
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
  pMock->output_arg[ARG2].p = &wrongFileLength;

  return_val = NVMCaretakerVerifyObject(&fileSystem, &objectDescriptor);

  TEST_ASSERT(3 == return_val); //ECTKR_STATUS_SIZE_MISMATCH

  mock_calls_verify();
}


void test_NVMCaretakerVerifyObject_valid_inputs(void)
{
  mock_calls_clear();
  mock_t * pMock;

  zpal_nvm_handle_t fileSystem;
  ECaretakerStatus return_val;

  zpal_nvm_object_key_t fileID = 2;
  size_t fileLength = 10;

  SObjectDescriptor objectDescriptor;
  objectDescriptor.ObjectKey = fileID;
  objectDescriptor.iDataSize = fileLength;

  //File found with correct length
  mock_call_expect(TO_STR(zpal_nvm_get_object_size), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = fileID;
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
  pMock->output_arg[ARG2].p = &(fileLength);

  return_val = NVMCaretakerVerifyObject(&fileSystem, &objectDescriptor);

  TEST_ASSERT(0 == return_val); //ECTKR_STATUS_SUCCESS

  mock_calls_verify();
}

void test_NVMCaretakerVerifySet_invalid_inputs(void)
{
  mock_calls_clear();
  mock_t * pMock;

  zpal_nvm_handle_t fileSystem;
  ECaretakerStatus return_val;

  zpal_nvm_object_key_t fileIDs[4] = {7, 8, 90, 44};
  size_t fileLengths[4]      = {20, 30, 88, 19};
  size_t wrongFileLengths[4] = {20, 33, 88, 20};

  SObjectDescriptor g_aFileDescriptors[] = {
    { .ObjectKey = fileIDs[0],      .iDataSize = fileLengths[0]  },
    { .ObjectKey = fileIDs[1],      .iDataSize = fileLengths[1]  },
    { .ObjectKey = fileIDs[2],      .iDataSize = fileLengths[2]  },
    { .ObjectKey = fileIDs[3],      .iDataSize = fileLengths[3]  }
  };

  static ECaretakerStatus g_aFileStatus[4] = {1, 2, 3, 4}; //Dummy values that must be overwritten by NVMCaretakerVerifySet()

  SObjectSet FileSet = {
    .pFileSystem = &fileSystem,
    .iObjectCount = 4,
    .pObjectDescriptors = g_aFileDescriptors
  };


  //Test of missing file

  zpal_status_t findObjectReturnVals[4] = {ZPAL_STATUS_OK, ZPAL_STATUS_OK, ZPAL_STATUS_FAIL, ZPAL_STATUS_OK}; //Indicates fileIDs[2] was not found.

  for(uint32_t i=0; i<4; i++)
  {
    mock_call_expect(TO_STR(zpal_nvm_get_object_size), &pMock);
    pMock->return_code.value = findObjectReturnVals[i];
    pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
    pMock->expect_arg[ARG1].value = fileIDs[i];
    pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
    pMock->output_arg[ARG2].p = &fileLengths[i];
  }

  return_val = NVMCaretakerVerifySet(&FileSet, g_aFileStatus);

  //Verify that NVMCaretakerVerifySet() returns that the file system dosen't match the one described by FileSet
  TEST_ASSERT(1 == return_val); //ECTKR_STATUS_FILESYS_MISMATCH

  //Verify that g_aFileStatus indicates that fileIDs[2] is missing
  TEST_ASSERT(0 == g_aFileStatus[0]); //ECTKR_STATUS_SUCCESS
  TEST_ASSERT(0 == g_aFileStatus[1]); //ECTKR_STATUS_SUCCESS
  TEST_ASSERT(2 == g_aFileStatus[2]); //ECTKR_STATUS_UNABLE_TO_AQUIRE_HANDLE
  TEST_ASSERT(0 == g_aFileStatus[3]); //ECTKR_STATUS_SUCCESS

  mock_calls_verify();
  mock_calls_clear();


  //Test of file size mismatch

  for(uint32_t i=0; i<4; i++)
  {
    mock_call_expect(TO_STR(zpal_nvm_get_object_size), &pMock);
    pMock->return_code.value = ZPAL_STATUS_OK;
    pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
    pMock->expect_arg[ARG1].value = fileIDs[i];
    pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
    pMock->output_arg[ARG2].p = &wrongFileLengths[i];
  }

  return_val = NVMCaretakerVerifySet(&FileSet, g_aFileStatus);

  //Verify that NVMCaretakerVerifySet() returns that the file system dosen't match the one described by FileSet
  TEST_ASSERT(1 == return_val); //ECTKR_STATUS_FILESYS_MISMATCH

  //Verify that g_aFileStatus indicates that fileIDs[1] and fileIDs[3] do not have sizes that match
  TEST_ASSERT(0 == g_aFileStatus[0]); //ECTKR_STATUS_SUCCESS
  TEST_ASSERT(3 == g_aFileStatus[1]); //ECTKR_STATUS_SIZE_MISMATCH
  TEST_ASSERT(0 == g_aFileStatus[2]); //ECTKR_STATUS_SUCCESS
  TEST_ASSERT(3 == g_aFileStatus[3]); //ECTKR_STATUS_SIZE_MISMATCH

  mock_calls_verify();
}


void test_NVMCaretakerVerifySet_valid_inputs(void)
{
  mock_calls_clear();
  mock_t * pMock;

  zpal_nvm_handle_t fileSystem;
  ECaretakerStatus return_val;

  zpal_nvm_object_key_t fileIDs[4] = {7, 8, 90, 44};
  size_t fileLengths[4] = {20, 30, 88, 19};

  SObjectDescriptor g_aFileDescriptors[] = {
    { .ObjectKey = fileIDs[0],      .iDataSize = fileLengths[0]  },
    { .ObjectKey = fileIDs[1],      .iDataSize = fileLengths[1]  },
    { .ObjectKey = fileIDs[2],      .iDataSize = fileLengths[2]  },
    { .ObjectKey = fileIDs[3],      .iDataSize = fileLengths[3]  }
  };

  static ECaretakerStatus g_aFileStatus[4] = {1, 2, 3, 4}; //Dummy values that must be overwritten by NVMCaretakerVerifySet()

  SObjectSet FileSet = {
    .pFileSystem = &fileSystem,
    .iObjectCount = 4,
    .pObjectDescriptors = g_aFileDescriptors
  };

  for(uint32_t i=0; i<4; i++)
  {
    mock_call_expect(TO_STR(zpal_nvm_get_object_size), &pMock);
    pMock->return_code.value = ZPAL_STATUS_OK;
    pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
    pMock->expect_arg[ARG1].value = fileIDs[i];
    pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;
    pMock->output_arg[ARG2].p = &fileLengths[i];
  }

  return_val = NVMCaretakerVerifySet(&FileSet, g_aFileStatus);

  //Verify that NVMCaretakerVerifySet() returns that the file system dosen't match the one described by FileSet
  TEST_ASSERT(0 == return_val); //ECTKR_STATUS_FILESYS_MISMATCH

  //Verify that g_aFileStatus indicates that all files were found and have correct length
  TEST_ASSERT(0 == g_aFileStatus[0]); //ECTKR_STATUS_SUCCESS
  TEST_ASSERT(0 == g_aFileStatus[1]); //ECTKR_STATUS_SUCCESS
  TEST_ASSERT(0 == g_aFileStatus[2]); //ECTKR_STATUS_SUCCESS
  TEST_ASSERT(0 == g_aFileStatus[3]); //ECTKR_STATUS_SUCCESS

  mock_calls_verify();
}
