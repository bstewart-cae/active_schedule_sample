// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file ZW_Security_Scheme2_mock.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include <ZW_security_api.h>
#include <mock_control.h>
#include <string.h>
#include <ZW_transport.h>

#define MOCK_FILE "ZW_Security_Scheme2_mock.c"

uint8_t
sec2_send_data(void* vdata,
               uint16_t len,
               TRANSMIT_OPTIONS_TYPE *txOptionsEx,
               const STransmitCallback* pCallback)
{
  return 0;
}

uint8_t
sec2_send_data_multi(void* vdata,
                     uint16_t len,
                     TRANSMIT_MULTI_OPTIONS_TYPE *txOptionsMultiEx,
                     const STransmitCallback* pCallback)
{
  return 0;
}

void sec2_inclusion_abort(void)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(pMock);
}

void Security2CommandHandler(uint8_t *pCmd,
                             uint8_t cmdLength,
                             RECEIVE_OPTIONS_TYPE *rxopt)
{
  mock_t * pMock;
  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(pMock);

  MOCK_CALL_ACTUAL(pMock, pCmd, cmdLength, rxopt);
  MOCK_CALL_COMPARE_INPUT_POINTER(pMock, ARG0, pCmd);
  MOCK_CALL_COMPARE_INPUT_UINT8(pMock, ARG1, cmdLength);
  MOCK_CALL_COMPARE_INPUT_POINTER(pMock, ARG2, rxopt);

}

bool sec0_unpersist_netkey(void)
{
  mock_t * pMock;
  MOCK_CALL_RETURN_IF_USED_AS_STUB(true);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(pMock, false);

  MOCK_CALL_RETURN_VALUE(pMock, bool);
}
