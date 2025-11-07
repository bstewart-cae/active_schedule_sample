/**
 * @file ZW_event_mock.h
 * @copyright 2023 Silicon Laboratories Inc.
 */
#include "mock_control.h"

#define MOCK_FILE "ZW_event_mock.c"

bool
EventPush(
  uint8_t bEvent,
  uint8_t bStatus)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(true);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(pMock, false);

  MOCK_CALL_ACTUAL(pMock, bEvent, bStatus);
  MOCK_CALL_COMPARE_INPUT_UINT8(pMock, ARG0, bEvent);
  MOCK_CALL_COMPARE_INPUT_UINT8(pMock, ARG1, bStatus);

  MOCK_CALL_RETURN_VALUE(pMock, bool);
}

bool
EventPop(
  uint8_t *pbEvent,
  uint8_t *pbStatus)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(true);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(pMock, false);

  MOCK_CALL_ACTUAL(pMock, pbEvent, pbStatus);
  MOCK_CALL_COMPARE_INPUT_POINTER(pMock, ARG0, pbEvent);
  MOCK_CALL_COMPARE_INPUT_POINTER(pMock, ARG1, pbStatus);
  
  MOCK_CALL_RETURN_VALUE(pMock, bool);
}

bool
EventPeek(
  uint8_t *pbEvent,
  uint8_t *pbStatus)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(true);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(pMock, false);

  MOCK_CALL_ACTUAL(pMock, pbEvent, pbStatus);
  MOCK_CALL_COMPARE_INPUT_POINTER(pMock, ARG0, pbEvent);
  MOCK_CALL_COMPARE_INPUT_POINTER(pMock, ARG1, pbStatus);
  
  MOCK_CALL_RETURN_VALUE(pMock, bool);
}
