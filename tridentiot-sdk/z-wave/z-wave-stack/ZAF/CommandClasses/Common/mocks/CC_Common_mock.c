/**
 * @file CC_Common_mock.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include <ZW_typedefs.h>
#include <CC_Common.h>
#include <mock_control.h>

#define MOCK_FILE "CC_Common_mock.c"

JOB_STATUS cc_engine_multicast_request(const AGI_PROFILE* pProfile,
                                       uint8_t endpoint,
                                       CMD_CLASS_GRP *pcmdGrp,
                                       uint8_t* pPayload,
                                       uint8_t size,
                                       uint8_t fSupervisionEnable,
                                       VOID_CALLBACKFUNC(pCbFunc) (TRANSMISSION_RESULT * pTransmissionResult))
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(JOB_STATUS_SUCCESS);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, JOB_STATUS_BUSY);
  MOCK_CALL_ACTUAL(p_mock, pProfile, endpoint, pcmdGrp, pPayload, size, fSupervisionEnable, pCbFunc);

  if(pProfile) {
    MOCK_CALL_COMPARE_STRUCT_MEMBER_UINT8(p_mock, ARG0, pProfile, agi_profile_t, profile_MS);
    MOCK_CALL_COMPARE_STRUCT_MEMBER_UINT8(p_mock, ARG0, pProfile, agi_profile_t, profile_LS);
  }

  MOCK_CALL_COMPARE_INPUT_UINT8(p_mock, ARG1, endpoint);

  MOCK_CALL_COMPARE_STRUCT_MEMBER_UINT8(p_mock, ARG2, pcmdGrp, cc_group_t, cmdClass);
  MOCK_CALL_COMPARE_STRUCT_MEMBER_UINT8(p_mock, ARG2, pcmdGrp, cc_group_t, cmd);

  MOCK_CALL_COMPARE_INPUT_UINT8_ARRAY(p_mock, ARG3, p_mock->expect_arg[4].v, pPayload, size);

  MOCK_CALL_COMPARE_INPUT_UINT8(p_mock, ARG4, size);
  MOCK_CALL_COMPARE_INPUT_UINT8(p_mock, ARG5, fSupervisionEnable);
  MOCK_CALL_COMPARE_INPUT_POINTER(p_mock, ARG6, pCbFunc);

  MOCK_CALL_RETURN_VALUE(p_mock, JOB_STATUS);
}
