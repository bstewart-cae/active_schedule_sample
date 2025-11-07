/**
 * @file CC_ColorSwitch_mock.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include <mock_control.h>
#include <stdint.h>
#include <CC_ColorSwitch.h>

#define MOCK_FILE "CC_ColorSwitch_mock.c"

void CC_ColorSwitch_Init(s_colorComponent *colors,
                         uint8_t length,
                         uint8_t durationDefault,
                         void (*CC_ColorSwitch_cb)(void))
{
  mock_t *pMock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(pMock);
  MOCK_CALL_ACTUAL(pMock, colors, length, durationDefault, CC_ColorSwitch_cb);

  MOCK_CALL_COMPARE_INPUT_POINTER(pMock, ARG0, colors);
  MOCK_CALL_COMPARE_INPUT_UINT8(pMock, ARG1, length);
  MOCK_CALL_COMPARE_INPUT_UINT8(pMock, ARG2, durationDefault);
  MOCK_CALL_COMPARE_INPUT_POINTER(pMock, ARG3, CC_ColorSwitch_cb);
}


received_frame_status_t
CC_ColorSwitch_handler(RECEIVE_OPTIONS_TYPE_EX *rxOpt,
                       ZW_APPLICATION_TX_BUFFER *pCmd,
                       uint8_t cmdLength)
{
  mock_t *pMock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(RECEIVED_FRAME_STATUS_SUCCESS);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(pMock, RECEIVED_FRAME_STATUS_FAIL);
  MOCK_CALL_ACTUAL(pMock, rxOpt, pCmd, cmdLength);

  MOCK_CALL_COMPARE_INPUT_POINTER(pMock, ARG0, rxOpt);
  MOCK_CALL_COMPARE_INPUT_POINTER(pMock, ARG1, pCmd);
  MOCK_CALL_COMPARE_INPUT_UINT8(pMock, ARG2, cmdLength);

  MOCK_CALL_RETURN_VALUE(pMock, received_frame_status_t);
}
