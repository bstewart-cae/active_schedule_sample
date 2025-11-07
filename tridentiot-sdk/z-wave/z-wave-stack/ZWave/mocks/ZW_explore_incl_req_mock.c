// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file ZW_explore_incl_req_mock.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include <ZW_typedefs.h>
#include "mock_control.h"
#include "ZW_transport_transmit_cb.h"

#define MOCK_FILE "ZW_explore_incl_req_mock.c"

uint8_t
ZW_ExploreRequestInclusion(STransmitCallback* pTxCallback)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(0x00);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(pMock, 0xFF);
  MOCK_CALL_ACTUAL(pMock, pTxCallback);

  MOCK_CALL_COMPARE_INPUT_POINTER(pMock, ARG0, pTxCallback);
  MOCK_CALL_RETURN_VALUE(pMock, uint8_t);
}

uint8_t
ExploreINIF(void)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(0x00);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(pMock, 0xFF);

  MOCK_CALL_RETURN_VALUE(pMock, uint8_t);
}
