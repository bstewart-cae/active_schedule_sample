// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file ZW_transport_mock.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include <ZW_typedefs.h>
#include <ZW_transport_api.h>
#include <ZW_transport.h>
#include <mock_control.h>

#define MOCK_FILE "ZW_transport_mock.c"
#include <ZW_transport_api.h>

RECEIVE_OPTIONS_TYPE rxopt;
uint8_t mTransportRxCurrentCh = 0;

enum ZW_SENDDATA_EX_RETURN_CODES
  ZW_SendDataMultiEx(
    uint8_t *pData,
    uint8_t  dataLength,
    TRANSMIT_MULTI_OPTIONS_TYPE *pTxOptionsMultiEx,
    const STransmitCallback* pCompletedFunc)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB((enum ZW_SENDDATA_EX_RETURN_CODES)ZW_TX_IN_PROGRESS);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(pMock, ZW_TX_FAILED);

  MOCK_CALL_COMPARE_INPUT_POINTER(pMock, ARG0, pData);
  MOCK_CALL_COMPARE_INPUT_UINT8(pMock, ARG1, dataLength);
  MOCK_CALL_COMPARE_INPUT_POINTER(pMock, ARG2, pTxOptionsMultiEx);
  MOCK_CALL_COMPARE_INPUT_POINTER(pMock, ARG3, pCompletedFunc);

  MOCK_CALL_RETURN_VALUE(pMock, enum ZW_SENDDATA_EX_RETURN_CODES);
}

uint8_t
ZW_SendDataMulti(
  uint8_t     *pNodeIDList,
  const uint8_t *pData,
  uint8_t      dataLength,
  TxOptions_t  txOptions,
  const STransmitCallback* pCompletedFunc)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(true);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(pMock, false);

  MOCK_CALL_COMPARE_INPUT_POINTER(pMock, ARG0, pNodeIDList);
  MOCK_CALL_COMPARE_INPUT_POINTER(pMock, ARG1, pData);
  MOCK_CALL_COMPARE_INPUT_UINT8(pMock, ARG2, dataLength);
  MOCK_CALL_COMPARE_INPUT_UINT8(pMock, ARG3, txOptions);
  MOCK_CALL_COMPARE_INPUT_POINTER(pMock, ARG4, pCompletedFunc);

  MOCK_CALL_RETURN_VALUE(pMock, uint8_t);
}

enum ZW_SENDDATA_EX_RETURN_CODES                /*RET Return code      */
  ZW_SendDataEx(
    uint8_t const * const pData,             /* IN Data buffer pointer           */
    uint8_t  dataLength,                     /* IN Data buffer length            */
    TRANSMIT_OPTIONS_TYPE *pTxOptionsEx,
    const STransmitCallback* pCompletedFunc)
{
  mock_t * p_mock;
  TX_STATUS_TYPE extendedTxStatus;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(1);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, ZW_TX_FAILED);
  MOCK_CALL_ACTUAL(p_mock, pData, dataLength, pTxOptionsEx, pCompletedFunc);

  // Compares both array length and array.
  MOCK_CALL_COMPARE_INPUT_UINT8_ARRAY(
          p_mock,
          ARG0,
          p_mock->expect_arg[1].v,
          pData,
          dataLength);

  MOCK_CALL_COMPARE_INPUT_UINT8(
          p_mock,
          ARG1,
          dataLength);

  MOCK_CALL_COMPARE_INPUT_POINTER(
          p_mock,
          ARG2,
          pTxOptionsEx);

  MOCK_CALL_COMPARE_INPUT_POINTER(
          p_mock,
          ARG3,
          pCompletedFunc);

  // This call is async in real life but we can't make it async on the unit test
  if(pCompletedFunc->pCallback) {
    pCompletedFunc->pCallback(pCompletedFunc->Context, TRANSMIT_COMPLETE_OK, &extendedTxStatus);
  }

  MOCK_CALL_RETURN_VALUE(p_mock, enum ZW_SENDDATA_EX_RETURN_CODES);
}

bool
EnQueueSingleDataOnLRChannels(
  uint8_t    rfSpeed,
  node_id_t  srcNodeID,
  node_id_t  destNodeID,
  uint8_t   *pData,
  uint8_t    dataLength,
  uint32_t   delayedTxMs,
  uint8_t    txPower,
  const STransmitCallback* pCompletedFunc)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(1);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, 0);
  MOCK_CALL_ACTUAL(p_mock, rfSpeed, srcNodeID, destNodeID, pData, dataLength, delayedTxMs, txPower, pCompletedFunc);

  MOCK_CALL_COMPARE_INPUT_UINT8(p_mock, ARG0, rfSpeed);
  MOCK_CALL_COMPARE_INPUT_UINT8(p_mock, ARG1, srcNodeID);
  MOCK_CALL_COMPARE_INPUT_UINT8(p_mock, ARG2, destNodeID);
  MOCK_CALL_COMPARE_INPUT_UINT8_ARRAY(p_mock, ARG3, p_mock->expect_arg[4].v, pData, dataLength);
  MOCK_CALL_COMPARE_INPUT_UINT8(p_mock, ARG4, dataLength);
  MOCK_CALL_COMPARE_INPUT_UINT32(p_mock, ARG5, delayedTxMs);
  MOCK_CALL_COMPARE_INPUT_UINT8(p_mock, ARG6, txPower);
  MOCK_CALL_COMPARE_INPUT_POINTER(p_mock, ARG7, pCompletedFunc);

  MOCK_CALL_RETURN_VALUE(p_mock, uint8_t);
}

bool EnQueueSingleData(
  uint8_t  rfSpeed,
  node_id_t  srcNodeID,
  node_id_t  destNodeID,
  const uint8_t *pData,
  uint8_t  dataLength,
  uint32_t txOptions,
  uint32_t delayedTxMs,
  uint8_t  txPower,
  const STransmitCallback* pCompletedFunc)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(1);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, 0);
  MOCK_CALL_ACTUAL(p_mock, rfSpeed, srcNodeID, destNodeID, pData, dataLength, txOptions, delayedTxMs, txPower, pCompletedFunc);

  MOCK_CALL_COMPARE_INPUT_UINT8(p_mock, ARG0, rfSpeed);
  MOCK_CALL_COMPARE_INPUT_UINT16(p_mock, ARG1, srcNodeID);
  MOCK_CALL_COMPARE_INPUT_UINT16(p_mock, ARG2, destNodeID);
  MOCK_CALL_COMPARE_INPUT_UINT8_ARRAY(p_mock, ARG3, p_mock->expect_arg[4].v, pData, dataLength);
  MOCK_CALL_COMPARE_INPUT_UINT8(p_mock, ARG4, dataLength);
  MOCK_CALL_COMPARE_INPUT_UINT32(p_mock, ARG5, txOptions);
  MOCK_CALL_COMPARE_INPUT_UINT32(p_mock, ARG6, delayedTxMs);
  MOCK_CALL_COMPARE_INPUT_UINT8(p_mock, ARG7, txPower);
  MOCK_CALL_COMPARE_INPUT_POINTER(p_mock, ARG8, pCompletedFunc);

  MOCK_CALL_RETURN_VALUE(p_mock, bool);
}

uint8_t
TransportEnQueueExploreFrame(
  frameExploreStruct      *pFrame,
  TxOptions_t             exploreTxOptions,
  ZW_SendData_Callback_t  completedFunc)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(1);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, 0);
  MOCK_CALL_ACTUAL(p_mock, pFrame, exploreTxOptions, completedFunc);

  MOCK_CALL_COMPARE_INPUT_POINTER(p_mock, ARG0, pFrame);
  MOCK_CALL_COMPARE_INPUT_UINT32(p_mock, ARG1, exploreTxOptions);
  MOCK_CALL_COMPARE_INPUT_POINTER(p_mock, ARG2, completedFunc);

  MOCK_CALL_RETURN_VALUE(p_mock, uint8_t);
}

uint8_t
TransportGetCurrentRxChannel(void)
{
  return mTransportRxCurrentCh;
}

void
TransportSetCurrentRxChannel(
  uint8_t rfRxCurrentCh)
{
  mTransportRxCurrentCh = rfRxCurrentCh;
}

void
TransportSetCurrentRxSpeedThroughProfile(__attribute__((unused)) uint32_t profile)
{
}

void
ZW_EnableRoutedRssiFeedback(
  __attribute__((unused)) uint8_t bEnabled)
{
}

uint8_t
TransportGetCurrentRxSpeed(void)
{
  return 0;
}


uint8_t ChooseSpeedForDestination_slave(node_id_t pNodeID)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(RF_SPEED_LR_100K);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(pMock, RF_SPEED_9_6K);
  MOCK_CALL_ACTUAL(pMock, pNodeID);

  MOCK_CALL_COMPARE_INPUT_UINT16(pMock, ARG0, pNodeID);

  MOCK_CALL_RETURN_VALUE(pMock, uint8_t);
}

uint8_t ZCB_ReturnRouteFindPriority(uint8_t bPriority, uint8_t bRouteIndex)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(1);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(pMock, 0);
  MOCK_CALL_ACTUAL(pMock, bPriority, bRouteIndex);

  MOCK_CALL_COMPARE_INPUT_UINT8(pMock, ARG0, bPriority);
  MOCK_CALL_COMPARE_INPUT_UINT8(pMock, ARG1, bRouteIndex);

  MOCK_CALL_RETURN_VALUE(pMock, uint8_t);

}

void ReturnRouteClearPriority(uint8_t bDestIndex)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(pMock);
  MOCK_CALL_ACTUAL(pMock, bDestIndex);

  MOCK_CALL_COMPARE_INPUT_UINT8(pMock, ARG0, bDestIndex);
}

void FlushResponseSpeeds(void)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(pMock);
}


void
SetTransmitHomeID(TxQueueElement *ptrFrame)    /* IN Frame pointer               */
{
  mock_t * pMock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(pMock);
  MOCK_CALL_ACTUAL(pMock, ptrFrame);

  MOCK_CALL_COMPARE_INPUT_POINTER(pMock, ARG0, ptrFrame);
}
