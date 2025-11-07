// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file ZW_build_tx_header_mock.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include <stdint.h>
#include <ZW_Frame.h>
#include <ZW_tx_queue.h>
#include "mock_control.h"

#define MOCK_FILE "ZW_build_tx_header_mock.c"

void BuildTxHeader(
    ZW_HeaderFormatType_t headerType, /*IN header type of the transmitted frame*/
    uint8_t frameType,                /* IN Frame type (FRAMETYPE_*, lower nibble), seq no. (upper nibble)    */
    uint8_t bSequenceNumber,          /* IN Sequence number of the frame that should be transmitted */
    TxQueueElement *pFrame)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(pMock);
  MOCK_CALL_ACTUAL(pMock, headerType, frameType, bSequenceNumber, pFrame);

  MOCK_CALL_COMPARE_INPUT_UINT32(pMock, ARG0, headerType);
  MOCK_CALL_COMPARE_INPUT_UINT8(pMock, ARG1, frameType);
  MOCK_CALL_COMPARE_INPUT_UINT8(pMock, ARG2, bSequenceNumber);
  MOCK_CALL_COMPARE_INPUT_POINTER(pMock, ARG3, pFrame);
}
