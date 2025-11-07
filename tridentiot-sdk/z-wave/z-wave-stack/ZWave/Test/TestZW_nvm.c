// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file TestZW_nvm.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include "SizeOf.h"
#include "ZW_nvm.h"
#include "unity.h"
#include "mock_control.h"
#include "ZW_lib_defines.h"

void setUpSuite(void) {

}

void tearDownSuite(void) {

}

void setUp(void) {

}

void tearDown(void) {

}

/*
Test Strategy
-------------

TDU: TestZw_Nvm 1.0
Name: TestInit
Purpose: Test that File system is opened on NVM init.
Precondition: FS can be opened.

TDU: TestZw_Nvm 1.1
Name: TestInitNoFs
Purpose: Test that if FS cannot be opened on NVM init, the function returns with error output.
Precondition: FS cannot be opened.

TDU: TestZw_Nvm 1.2
Name: TestRegisterBeforeInit
Purpose: Test that registrering prior to NVM open causes NVM open.
Precondition: NVM is not opened.

TDU: TestZw_Nvm 1.3
Name: TestRegisterAfterInit
Purpose: Test that registrering after NVM open does NOT cause NVM open.
Precondition: NVM is opened.

TDU: TestZw_Nvm 1.4
Name: TestNvmFormatPerformed
Purpose: Test that FS is erased on NVM format.
Precondition: NVM is successfully inited.

TDU: TestZw_Nvm 1.5
Name: TestNvmFormatRegistreeCallbacks
Purpose: Test that registree callbacks are performed on NVM format.
Precondition: NVM is successfully inited.

TDU: TestZw_Nvm 1.6
Name: TestInitClearRegistrations
Purpose: Test that registrations are cleared on NVM init.
Precondition: NVM has registrees.


TDU: TestZw_Nvm 1.7  NOT APPLICABLE ANYMORE
Name: TestRegistreesInformedOnNonFormat
Purpose: Test that registrees are are informed correctly when registrering to NVM where FS has NOT been formated.
Precondition: NVM is inited, and FS was NOT formatted during init.


TDU: TestZw_Nvm 1.8  NOT APPLICABLE ANYMORE
Name: TestRegistreesInformedOnFormatCauseCommand
Purpose: Test that registrees are are informed correctly when registrering to NVM where FS has been formated
on call to NvmFileSystemFormat.
Precondition: NVM is inited without FS format.

TDU: TestZw_Nvm 1.9  NOT APPLICABLE ANYMORE
Name: TestRegistreesInformedOnFormatCauseInit
Purpose: Test that registrees are are informed correctly when registrering to NVM where FS has been formated on NVM init.
Precondition: NVM is inited, and FS formatted during init.

TDU: TestZw_Nvm 1.10  NOT APPLICABLE ANYMORE
Name: TestRegistreesInformedOnFormatCauseRegistrerUninited
Purpose: Test that registrees are are informed correctly when registrering to NVM where FS has been formated.
on registering prior to init.
Precondition: NVM is not inited.
NOTES: This cannot be tested due to the inability to reset m_bIsFsInitialized in DUT module


*/


static uint32_t iFormattedCallbackCount = 0;
void FormattedCallbackReceiver(void)
{
  iFormattedCallbackCount++;
}

static uint32_t iFormattedCallbackCount2 = 0;
void FormattedCallbackReceiver2(void)
{
  iFormattedCallbackCount2++;
}

void test_ZwNvm_Basics(void)
{
  // Test Registering prior to nvm3_open()
  mock_calls_clear();

  static uint8_t dummy;

  mock_t * pMock;
  mock_call_expect(TO_STR(zpal_nvm_init), &pMock);                     // TDU 1.2
  pMock->return_code.p = &dummy;
  pMock->expect_arg[0].v = ZPAL_NVM_AREA_STACK;

  SSyncEvent RegisterEvent0;
  zpal_nvm_handle_t pFileSystem = NvmFileSystemRegister(&RegisterEvent0);
  TEST_ASSERT_NOT_NULL(pFileSystem);
  TEST_ASSERT_EQUAL(pFileSystem, &dummy);

  mock_calls_verify();                                                  // TDU 1.2

  // Test registrering after zpal_nvm_init()
  mock_calls_clear();

  SSyncEvent RegisterEvent1;
  zpal_nvm_handle_t pFileSystem1 = NvmFileSystemRegister(&RegisterEvent1);
  TEST_ASSERT_EQUAL(pFileSystem, pFileSystem1);

                                                                        // Verification is really that no calls are made to FileSystemInit and FileSystemMount
  mock_calls_verify();                                                  // TDU 1.3


  // Test NVM format call
  mock_calls_clear();
  SyncEventBind(&RegisterEvent0, FormattedCallbackReceiver);
  SyncEventBind(&RegisterEvent1, FormattedCallbackReceiver2);

  mock_call_expect(TO_STR(zpal_nvm_erase_all), &pMock);     // TDU 1.4
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->expect_arg[ARG0].p = pFileSystem;

  iFormattedCallbackCount = 0;
  iFormattedCallbackCount2 = 0;

  bool bStatus = NvmFileSystemFormat();

  TEST_ASSERT_TRUE(bStatus);
  TEST_ASSERT_EQUAL(1, iFormattedCallbackCount);        // TDU 1.5
  TEST_ASSERT_EQUAL(1, iFormattedCallbackCount2);       // TDU 1.5

  pFileSystem1 = NvmFileSystemRegister(&RegisterEvent1);
  TEST_ASSERT_EQUAL(pFileSystem, pFileSystem1);

  mock_calls_verify();                                  // TDU 1.4

  // Test NvmfileSystemInit failure
  mock_calls_clear();
  mock_call_expect(TO_STR(zpal_nvm_init), &pMock);     // TDU 1.1
  pMock->return_code.p = NULL;
  pMock->expect_arg[0].v = ZPAL_NVM_AREA_STACK;

  ENvmFsInitStatus InitStatus = NvmFileSystemInit();
  TEST_ASSERT_EQUAL(ENVMFSINITSTATUS_FAILED, InitStatus);

  mock_calls_verify();                                  // TDU 1.1


  // Test NvmfileSystemInit success
  mock_calls_clear();

  mock_call_expect(TO_STR(zpal_nvm_init), &pMock);     // TDU 1.0
  pMock->return_code.p = &dummy;
  pMock->expect_arg[0].v = ZPAL_NVM_AREA_STACK;

  InitStatus = NvmFileSystemInit();
  TEST_ASSERT_EQUAL(ENVMFSINITSTATUS_SUCCESS, InitStatus);

  mock_calls_verify();                                  // TDU 1.0


  // Test registrations are cleared on init
  mock_calls_clear();
  pFileSystem = NvmFileSystemRegister(&RegisterEvent0);
  pFileSystem = NvmFileSystemRegister(&RegisterEvent1);

  mock_call_expect(TO_STR(zpal_nvm_init), &pMock);     // TDU 1.0
  pMock->return_code.p = &dummy;
  pMock->expect_arg[0].v = ZPAL_NVM_AREA_STACK;

  InitStatus = NvmFileSystemInit();
  TEST_ASSERT_EQUAL(ENVMFSINITSTATUS_SUCCESS, InitStatus);

  mock_calls_verify();
  mock_calls_clear();

  // Perform nvm format to see if registrations are cleared
  mock_call_expect(TO_STR(zpal_nvm_erase_all), &pMock);
  pMock->return_code.value = ZPAL_STATUS_OK;
  pMock->expect_arg[ARG0].p = pFileSystem;

  iFormattedCallbackCount = 0;
  iFormattedCallbackCount2 = 0;

  bool bFormatStatus = NvmFileSystemFormat();
  TEST_ASSERT_TRUE(bFormatStatus);

  //verify that the callback functions were NOT invoked
  TEST_ASSERT_EQUAL(0, iFormattedCallbackCount);                        // TDU 1.6
  TEST_ASSERT_EQUAL(0, iFormattedCallbackCount2);                       // TDU 1.6

  mock_calls_verify();

}


