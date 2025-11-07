// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file ZW_protocol_interface_mock.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include <ZW_typedefs.h>
#include "mock_control.h"
#include <ZW_application_transport_interface.h>

#define MOCK_FILE "ZW_protocol_interface_mock.c"

void
ProtocolInterfacePassToAppLearnStatus(ELearnStatus Status)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(pMock);
  MOCK_CALL_ACTUAL(pMock, Status);

  MOCK_CALL_COMPARE_INPUT_UINT32(pMock, ARG0, Status);
}

bool ProtocolInterfacePassToAppNodeUpdate(
                                          uint8_t Status,
                                          node_id_t NodeId,
                                          const uint8_t *pCommand,
                                          uint8_t iLength
                                          )
{
  mock_t * pMock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(true);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(pMock, false);
  MOCK_CALL_ACTUAL(pMock, Status, NodeId, pCommand, iLength);

  MOCK_CALL_COMPARE_INPUT_UINT8(pMock, ARG0, Status);
  MOCK_CALL_COMPARE_INPUT_UINT8(pMock, ARG1, NodeId);
  MOCK_CALL_COMPARE_INPUT_UINT8_ARRAY(pMock, ARG2, pMock->expect_arg[ARG3].v, pCommand, iLength);

  MOCK_CALL_RETURN_VALUE(pMock, bool);
}

void ProtocolInterfaceReset(void)
{

}

bool ProtocolInterfacePassToAppMultiFrame(
                                          uint8_t NodeMaskOffset,
                                          uint8_t NodeMaskLength,
                                          const uint8_t* pNodeMask,
                                          const ZW_APPLICATION_TX_BUFFER *pCommand,
                                          uint8_t iCommandLength,
                                          const RECEIVE_OPTIONS_TYPE *rxOpt
                                        )
{
  mock_t * pMock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(true);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(pMock, false);
  MOCK_CALL_ACTUAL(pMock, NodeMaskOffset, NodeMaskLength, pNodeMask, pCommand, iCommandLength, rxOpt);

  MOCK_CALL_COMPARE_INPUT_UINT8(pMock, ARG0, NodeMaskOffset);
  MOCK_CALL_COMPARE_INPUT_UINT8(pMock, ARG1, NodeMaskLength);
  MOCK_CALL_COMPARE_INPUT_POINTER(pMock, ARG2, pNodeMask);
  MOCK_CALL_COMPARE_INPUT_POINTER(pMock, ARG3, pCommand);
  MOCK_CALL_COMPARE_INPUT_UINT8(pMock, ARG4, iCommandLength);
  MOCK_CALL_COMPARE_INPUT_POINTER(pMock, ARG5, rxOpt);

  MOCK_CALL_RETURN_VALUE(pMock, bool);
}

uint32_t ProtocolInterfaceFramesToAppQueueFull(void)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(0);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(pMock, 1);

  MOCK_CALL_RETURN_VALUE(pMock, uint32_t);
}


bool ProtocolInterfacePassToAppSingleFrame(
                                           uint8_t iCommandLength,
                                           const ZW_APPLICATION_TX_BUFFER *pCommand,
                                           const RECEIVE_OPTIONS_TYPE *pRxOptions
                                           )
{
  mock_t * pMock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(true);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(pMock, false);
  MOCK_CALL_ACTUAL(pMock, iCommandLength, pCommand, pRxOptions);

  MOCK_CALL_COMPARE_INPUT_UINT8(pMock, ARG0, iCommandLength);
  MOCK_CALL_COMPARE_INPUT_POINTER(pMock, ARG1, pCommand);
  MOCK_CALL_COMPARE_INPUT_POINTER(pMock, ARG2, pRxOptions);
  MOCK_CALL_RETURN_VALUE(pMock, bool);
}

bool ProtocolInterfacePassToAppSecurityEvent(const s_application_security_event_data_t *pSecurityEvent)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(true);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(pMock, false);
  MOCK_CALL_ACTUAL(pMock, pSecurityEvent);

  MOCK_CALL_COMPARE_INPUT_POINTER(pMock, ARG0, pSecurityEvent);
  MOCK_CALL_RETURN_VALUE(pMock, bool);
}

bool ProtocolInterfacePassToAppStayAwake(void)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(true);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(pMock, false);

  MOCK_CALL_RETURN_VALUE(pMock, bool);
}
