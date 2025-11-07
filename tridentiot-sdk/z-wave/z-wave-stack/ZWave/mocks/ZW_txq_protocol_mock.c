// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file ZW_txq_protocol_mock.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include <ZW_typedefs.h>
#include "mock_control.h"
#include "unity.h"
#include <ZW_tx_queue.h>


#define MOCK_FILE "ZW_txq_protocol_mock.c"

TxQueueElement  *pFrameWaitingForACK = NULL;
uint8_t bRetransmitTimer                = 0;
uint8_t waitingForRoutedACK             = false;

void ZCB_ProtocolTransmitComplete(TxQueueElement *pTransmittedFrame)
{
  mock_t * p_mock;
  
  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(p_mock);
  MOCK_CALL_ACTUAL(p_mock, pTransmittedFrame);
  MOCK_CALL_COMPARE_INPUT_POINTER(p_mock, ARG0, pTransmittedFrame);
}

void ProtocolWaitForACK(TxQueueElement *pTransmittedFrame)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(p_mock);
  MOCK_CALL_ACTUAL(p_mock, pTransmittedFrame);
  MOCK_CALL_COMPARE_INPUT_POINTER(p_mock, ARG0, pTransmittedFrame);
}

void TxPermanentlyDone(uint8_t fStatus,
                       uint8_t *pAckRssi,
                       int8_t bDestinationAckUsedTxPower,
                       int8_t bDestinationAckMeasuredRSSI,
                       int8_t bDestinationAckMeasuredNoiseFloor)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(p_mock);
  MOCK_CALL_ACTUAL(p_mock,
                   fStatus,
                   pAckRssi,
                   bDestinationAckUsedTxPower,
                   bDestinationAckMeasuredRSSI,
                   bDestinationAckMeasuredNoiseFloor);

  MOCK_CALL_COMPARE_INPUT_UINT8(p_mock, ARG0, fStatus);
  MOCK_CALL_COMPARE_INPUT_UINT8(p_mock, ARG1, *pAckRssi);
  MOCK_CALL_COMPARE_INPUT_INT8(p_mock, ARG2, bDestinationAckUsedTxPower);
  MOCK_CALL_COMPARE_INPUT_INT8(p_mock, ARG3, bDestinationAckMeasuredRSSI);
  MOCK_CALL_COMPARE_INPUT_INT8(p_mock, ARG4, bDestinationAckMeasuredNoiseFloor);
}

