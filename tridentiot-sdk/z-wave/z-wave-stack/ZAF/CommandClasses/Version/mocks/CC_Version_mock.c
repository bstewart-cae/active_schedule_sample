/**
 * @file CC_Version_mock.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include <ZW_typedefs.h>
#include <CC_Common.h>
#include <ZW_TransportEndpoint.h>
#include <mock_control.h>

#define MOCK_FILE "CC_Version_mock.c"


received_frame_status_t CC_Version_handler(RECEIVE_OPTIONS_TYPE_EX *rxOpt, ZW_APPLICATION_TX_BUFFER *pCmd, uint8_t cmdLength)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(RECEIVED_FRAME_STATUS_SUCCESS);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(pMock, RECEIVED_FRAME_STATUS_FAIL);
  MOCK_CALL_ACTUAL(pMock, rxOpt, pCmd, cmdLength);

  MOCK_CALL_COMPARE_INPUT_POINTER(pMock, ARG0, rxOpt);
  MOCK_CALL_COMPARE_INPUT_POINTER(pMock, ARG1, pCmd);
  MOCK_CALL_COMPARE_INPUT_UINT8(pMock, ARG2, cmdLength);

  MOCK_CALL_RETURN_VALUE(pMock, received_frame_status_t);
}
