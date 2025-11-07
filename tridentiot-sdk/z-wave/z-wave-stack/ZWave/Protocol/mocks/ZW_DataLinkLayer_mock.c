// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file ZW_DataLinkLayer_mock.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include "ZW_DataLinkLayer.h"
#include "mock_control.h"
#include <stdint.h>
#include <string.h>
#include <Assert.h>

#define MOCK_FILE "ZW_DataLinkLayer_mock.c"


ZW_ReturnCode_t llInit(zpal_radio_profile_t* const pRfProfile)
{
  mock_t * pMock;
  MOCK_CALL_RETURN_IF_USED_AS_STUB(SUCCESS);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(pMock, INVALID_PARAMETERS);
  MOCK_CALL_ACTUAL(pMock, pRfProfile);

  MOCK_CALL_COMPARE_INPUT_POINTER(pMock, ARG0, pRfProfile);
  MOCK_CALL_RETURN_VALUE(pMock, ZW_ReturnCode_t);
}

void llReTransmitStop(ZW_TransmissionFrame_t *pFrame)
{
  mock_t * pMock;
  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(pMock);
  MOCK_CALL_ACTUAL(pMock, pFrame);

  MOCK_CALL_COMPARE_INPUT_POINTER(pMock, ARG0, pFrame);
}

uint8_t llIsReTransmitEnabled(ZW_TransmissionFrame_t *pFrame)
{
  mock_t * pMock;
  MOCK_CALL_RETURN_IF_USED_AS_STUB(1);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(pMock, 0);
  MOCK_CALL_ACTUAL(pMock, pFrame);

  MOCK_CALL_COMPARE_INPUT_POINTER(pMock, ARG0, pFrame);
  MOCK_CALL_RETURN_VALUE(pMock, uint8_t);
}

uint32_t llIsHeaderFormat3ch(void)
{
  mock_t * pMock;
  MOCK_CALL_RETURN_IF_USED_AS_STUB(1);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(pMock, 0xFF);
  MOCK_CALL_RETURN_VALUE(pMock, uint32_t);
}

ZW_HeaderFormatType_t llGetCurrentHeaderFormat(node_id_t bNodeID, uint8_t forceLR)
{
  mock_t * pMock;
  MOCK_CALL_RETURN_IF_USED_AS_STUB(HDRFORMATTYP_3CH);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(pMock, 0xFF);
  MOCK_CALL_ACTUAL(pMock, bNodeID, forceLR);

  MOCK_CALL_COMPARE_INPUT_UINT16(pMock, ARG0, bNodeID);
  MOCK_CALL_COMPARE_INPUT_UINT8(pMock, ARG1, forceLR);

  MOCK_CALL_RETURN_VALUE(pMock, ZW_HeaderFormatType_t);
}

ZW_ReturnCode_t llReceiveFilterAdd(const ZW_ReceiveFilter_t * pReceiveFilter)
{
  mock_t * pMock;
  MOCK_CALL_RETURN_IF_USED_AS_STUB(1);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(pMock, 0);
  MOCK_CALL_ACTUAL(pMock, pReceiveFilter);

  MOCK_CALL_COMPARE_INPUT_POINTER(pMock, ARG0, pReceiveFilter);
  MOCK_CALL_RETURN_VALUE(pMock, ZW_ReturnCode_t);
}

uint32_t llIsLBTEnabled(void)
{
  return 0;
}

bool llChangeLrChannelConfig(zpal_radio_lr_channel_config_t eLrChCfg)
{
  return true;
}


bool llChangeRfPHYToLrChannelConfig(zpal_radio_lr_channel_config_t eLrChCfg)
{
  mock_t * pMock;
  MOCK_CALL_RETURN_IF_USED_AS_STUB(true);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(pMock, false);
  MOCK_CALL_ACTUAL(pMock, eLrChCfg);

  MOCK_CALL_COMPARE_INPUT_UINT8(pMock, ARG0, eLrChCfg);
  MOCK_CALL_RETURN_VALUE(pMock, bool);
}
