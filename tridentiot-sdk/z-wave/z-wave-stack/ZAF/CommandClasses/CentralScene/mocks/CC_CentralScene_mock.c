/**
 * @file CC_CentralScene_mock.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include <ZW_typedefs.h>
#include <ZW_classcmd.h>
#include <mock_control.h>
#include <CC_CentralScene.h>

#define MOCK_FILE "CC_CentralScene_mock.c"

received_frame_status_t
handleCommandClassCentralScene(
  RECEIVE_OPTIONS_TYPE_EX *rxOpt,
  ZW_APPLICATION_TX_BUFFER *pCmd,
  uint8_t cmdLength)
{
  return RECEIVED_FRAME_STATUS_SUCCESS;
}

JOB_STATUS
cc_central_scene_notification_tx(
    AGI_PROFILE* pProfile,
    uint8_t keyAttribute,
    uint8_t sceneNumber,
    ZAF_TX_Callback_t pCbFunc)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(JOB_STATUS_SUCCESS);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, JOB_STATUS_BUSY);

  if(pProfile) {
    MOCK_CALL_COMPARE_STRUCT_MEMBER_UINT8(p_mock, ARG0, pProfile, agi_profile_t, profile_MS);
    MOCK_CALL_COMPARE_STRUCT_MEMBER_UINT8(p_mock, ARG0, pProfile, agi_profile_t, profile_LS);
  }
  MOCK_CALL_COMPARE_INPUT_UINT8(p_mock, ARG1, keyAttribute);
  MOCK_CALL_COMPARE_INPUT_UINT8(p_mock, ARG2, sceneNumber);
  MOCK_CALL_COMPARE_INPUT_POINTER(p_mock, ARG3, pCbFunc);

  MOCK_CALL_RETURN_VALUE(p_mock, JOB_STATUS);
}

void
cc_central_scene_handle_notification_timer(
    bool start_timer,
    uint8_t scene_number)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(p_mock);

  MOCK_CALL_ACTUAL(p_mock, start_timer, scene_number);

  MOCK_CALL_COMPARE_INPUT_BOOL(p_mock, ARG0, start_timer);
  MOCK_CALL_COMPARE_INPUT_UINT8(p_mock, ARG1, scene_number);

}
