/**
 * @file CC_Supervision_mock.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include <CC_Supervision.h>
#include <mock_control.h>

#define MOCK_FILE "CC_Supervision_mock.c"

void CommandClassSupervisionGetAdd(ZW_SUPERVISION_GET_FRAME* pbuf)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(p_mock);
  MOCK_CALL_ACTUAL(p_mock, pbuf);

  MOCK_CALL_COMPARE_INPUT_POINTER(p_mock, ARG0, pbuf);
}

bool CmdClassSupervisionReportSend(zaf_tx_options_t* tx_options,
                                    uint8_t properties,
                                    cc_supervision_status_t status,
                                    uint8_t duration)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(JOB_STATUS_SUCCESS);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, JOB_STATUS_BUSY);
  MOCK_CALL_ACTUAL(p_mock, tx_options, properties, status, duration);

  MOCK_CALL_COMPARE_INPUT_POINTER(p_mock, ARG0, tx_options);
  MOCK_CALL_COMPARE_INPUT_UINT8(p_mock, ARG1, properties);
  MOCK_CALL_COMPARE_INPUT_UINT8(p_mock, ARG2, status);
  MOCK_CALL_COMPARE_INPUT_UINT8(p_mock, ARG3, duration);

  MOCK_CALL_RETURN_VALUE(p_mock, JOB_STATUS);
}

void CommandClassSupervisionGetWrite(ZW_SUPERVISION_GET_FRAME* pbuf)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(p_mock);
  MOCK_CALL_ACTUAL(p_mock, pbuf);

  MOCK_CALL_COMPARE_INPUT_POINTER(p_mock, ARG0, pbuf);
}

void CommandClassSupervisionGetSetPayloadLength(ZW_SUPERVISION_GET_FRAME* pbuf, uint8_t payLoadlen)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(p_mock);
  MOCK_CALL_ACTUAL(p_mock, pbuf, payLoadlen);

  MOCK_CALL_COMPARE_INPUT_POINTER(p_mock, ARG0, pbuf);
  MOCK_CALL_COMPARE_INPUT_UINT8(p_mock, ARG1, payLoadlen);
}

uint8_t CommandClassSupervisionGetGetPayloadLength(ZW_SUPERVISION_GET_FRAME* pbuf)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(1);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, 0);
  MOCK_CALL_ACTUAL(p_mock, pbuf);

  MOCK_CALL_COMPARE_INPUT_POINTER(p_mock, ARG0, pbuf);

  MOCK_CALL_RETURN_VALUE(p_mock, uint8_t);
}
