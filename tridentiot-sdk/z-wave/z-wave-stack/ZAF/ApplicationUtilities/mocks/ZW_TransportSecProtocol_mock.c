// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file ZW_TransportSecProtocol_mock.c
 * @copyright 2022 Silicon Laboratories Inc.
 */

#include <ZW_typedefs.h>
#include <ZW_security_api.h>
#include <mock_control.h>
#include <ZW_transport_api.h>
#include <ZW_TransportEndpoint.h>
#include <ZW_TransportSecProtocol.h>

#define MOCK_FILE "ZW_TransportSecProtocol_mock.c"


enum SECURITY_KEY
GetHighestSecureLevel(uint8_t protocolSecBits)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB((security_key_t)SECURITY_KEY_NONE);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(pMock, SECURITY_KEY_NONE);

  MOCK_CALL_COMPARE_INPUT_UINT8(pMock, ARG0, protocolSecBits);

  MOCK_CALL_RETURN_VALUE(pMock, security_key_t);
}


bool
TransportCmdClassSupported(uint8_t commandClass,
                           uint8_t command,
                           enum SECURITY_KEY eKey)
{
  return true;
}

zaf_cc_list_t*
GetCommandClassList(
    bool secureList,
    security_key_t eKey,
    uint8_t endpoint)
{
  mock_t * pMock;
  MOCK_CALL_RETURN_IF_USED_AS_STUB(NULL);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(pMock, NULL);
  MOCK_CALL_RETURN_POINTER(pMock, zaf_cc_list_t*);
}

void
ApplicationCommandHandler(void *pSubscriberContext, SZwaveReceivePackage* pRxPackage)
{
  mock_t * pMock;
  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(pMock);

  MOCK_CALL_ACTUAL(pMock, pSubscriberContext, pRxPackage);

  MOCK_CALL_COMPARE_INPUT_POINTER(pMock, ARG0, pSubscriberContext);
  MOCK_CALL_COMPARE_INPUT_POINTER(pMock, ARG1, pRxPackage);
}

uint8_t Transport_OnApplicationInitSW(void)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(false);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, false);

  MOCK_CALL_RETURN_VALUE(p_mock, bool);
}

void ZAF_Transport_OnLearnCompleted(void)
{
  mock_t * pMock;
  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(pMock);
}
