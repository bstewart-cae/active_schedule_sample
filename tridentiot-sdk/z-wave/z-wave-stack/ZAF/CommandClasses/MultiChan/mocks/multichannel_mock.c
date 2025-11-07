/**
 * @file multichannel_mock.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include "mock_control.h"
#include "multichannel.h"

#define MOCK_FILE "multichannel_mock.c"

void CmdClassMultiChannelEncapsulate(
  uint8_t **ppData,
  size_t  *dataLength,
  TRANSMIT_OPTIONS_TYPE_SINGLE_EX *pTxOptionsEx)
{

}

bool ZAF_CC_MultiChannel_IsCCSupported(
  RECEIVE_OPTIONS_TYPE_EX * pRxOpt,
  ZW_APPLICATION_TX_BUFFER * pCmd)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(true);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(pMock, false);
  MOCK_CALL_ACTUAL(pMock, pRxOpt, pCmd);
  MOCK_CALL_RETURN_IF_ERROR_SET(pMock, bool);

  MOCK_CALL_COMPARE_INPUT_POINTER(pMock, ARG0, pRxOpt);
  MOCK_CALL_COMPARE_INPUT_POINTER(pMock, ARG1, pCmd);

  MOCK_CALL_RETURN_VALUE(pMock, bool);
}
