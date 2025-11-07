// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file TestZW_transport_transmit_cb.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include "ZW_transport_transmit_cb.h"
#include "mock_control.h"

void setUpSuite(void) {

}

void tearDownSuite(void) {

}

void test_ZW_TransmitCallback_Bind_Unbind_Invoke(void)
{
  uint8_t verifyStatus = 0;
  TX_STATUS_TYPE * pVerifyExtendedTxStatus = NULL;
  bool verifyContext = false;

  TX_STATUS_TYPE extendedTxStatus;

  void sendDataCB(ZW_Void_Function_t Context, uint8_t txStatus, TX_STATUS_TYPE* extendedTxStatus)
  {
    Context();

    verifyStatus = txStatus;
    pVerifyExtendedTxStatus = extendedTxStatus;
  }

  void contextFunction(void)
  {
    verifyContext = true;
  }


  STransmitCallback TxCallback = {NULL, NULL};
  bool isBound = false;

  //test binding
  ZW_TransmitCallbackBind(&TxCallback, sendDataCB, contextFunction);
  TEST_ASSERT_EQUAL_PTR_MESSAGE(&sendDataCB, TxCallback.pCallback,"Callback does not match.");
  TEST_ASSERT_EQUAL_PTR_MESSAGE(&contextFunction, TxCallback.Context,"Context does not match.");

  isBound = ZW_TransmitCallbackIsBound(&TxCallback);
  TEST_ASSERT_MESSAGE(true == isBound, "ZW_TransmitCallbackIsBound() incorrectly returns false.");

  //test invoking
  ZW_TransmitCallbackInvoke(&TxCallback, 5, &extendedTxStatus);
  TEST_ASSERT_MESSAGE(true == verifyContext,"contextFunction was not called.");
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(5, verifyStatus,"txStatus does not match.");
  TEST_ASSERT_EQUAL_PTR_MESSAGE(&extendedTxStatus, pVerifyExtendedTxStatus, "extendedTxStatus does not match.");

  //test unbinding
  ZW_TransmitCallbackUnBind(&TxCallback);
  TEST_ASSERT_EQUAL_PTR_MESSAGE(NULL, TxCallback.pCallback,"Callback did not unbind.");
  TEST_ASSERT_EQUAL_PTR_MESSAGE(NULL, TxCallback.Context,"Context did not unbind.");

  isBound = ZW_TransmitCallbackIsBound(&TxCallback);
  TEST_ASSERT_MESSAGE(false == isBound, "ZW_TransmitCallbackIsBound() incorrectly returns true.");
}
