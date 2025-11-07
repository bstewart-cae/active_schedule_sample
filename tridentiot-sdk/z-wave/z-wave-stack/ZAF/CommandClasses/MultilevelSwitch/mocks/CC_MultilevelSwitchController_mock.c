/**
 * @file CC_MultilevelSwitchController_mock.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include <CC_MultilevelSwitch_Control.h>
#include <mock_control.h>

#define MOCK_FILE "CC_MultilevelSwitchController_mock.c"

JOB_STATUS
CmdClassMultilevelSwitchStartLevelChange(
  AGI_PROFILE* pProfile,
  uint8_t sourceEndpoint,
  VOID_CALLBACKFUNC(pCbFunc)(TRANSMISSION_RESULT * pTransmissionResult),
  CCMLS_PRIMARY_SWITCH_T primarySwitch,
  CCMLS_IGNORE_START_LEVEL_T fIgnoreStartLevel,
  CCMLS_SECONDARY_SWITCH_T secondarySwitch,
  uint8_t primarySwitchStartLevel,
  uint8_t duration,
  uint8_t secondarySwitchStepSize)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(JOB_STATUS_SUCCESS);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, JOB_STATUS_BUSY);

  MOCK_CALL_COMPARE_STRUCT_MEMBER_UINT8(
		  p_mock,
		  ARG0,
		  pProfile,
		  agi_profile_t,
		  profile_MS);

  MOCK_CALL_COMPARE_STRUCT_MEMBER_UINT8(
		  p_mock,
		  ARG0,
		  pProfile,
		  agi_profile_t,
		  profile_LS);

  MOCK_CALL_COMPARE_INPUT_UINT8(
		  p_mock,
		  ARG1,
		  sourceEndpoint);

  MOCK_CALL_COMPARE_INPUT_POINTER(
		  p_mock,
		  ARG2,
		  pCbFunc);

  MOCK_CALL_COMPARE_INPUT_UINT32(
		  p_mock,
		  ARG3,
		  primarySwitch);

  MOCK_CALL_COMPARE_INPUT_UINT32(
		  p_mock,
		  ARG4,
		  fIgnoreStartLevel);

  MOCK_CALL_COMPARE_INPUT_UINT32(
		  p_mock,
		  ARG5,
		  secondarySwitch);

  MOCK_CALL_COMPARE_INPUT_UINT8(
		  p_mock,
		  ARG6,
		  primarySwitchStartLevel);

  MOCK_CALL_COMPARE_INPUT_UINT8(
		  p_mock,
		  ARG7,
		  duration);

  MOCK_CALL_RETURN_VALUE(p_mock, JOB_STATUS);
}

JOB_STATUS
CmdClassMultilevelSwitchStopLevelChange(
  AGI_PROFILE* pProfile,
  uint8_t sourceEndpoint,
  VOID_CALLBACKFUNC(pCbFunc)(TRANSMISSION_RESULT * pTransmissionResult))
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(JOB_STATUS_SUCCESS);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, JOB_STATUS_BUSY);

  MOCK_CALL_COMPARE_STRUCT_MEMBER_UINT8(
		  p_mock,
		  ARG0,
		  pProfile,
		  agi_profile_t,
		  profile_MS);

  MOCK_CALL_COMPARE_STRUCT_MEMBER_UINT8(
		  p_mock,
		  ARG0,
		  pProfile,
		  agi_profile_t,
		  profile_LS);

  MOCK_CALL_COMPARE_INPUT_UINT8(
		  p_mock,
		  ARG1,
		  sourceEndpoint);

  MOCK_CALL_COMPARE_INPUT_POINTER(
		  p_mock,
		  ARG2,
		  pCbFunc);

  MOCK_CALL_RETURN_VALUE(p_mock, JOB_STATUS);
}

JOB_STATUS
CmdClassMultilevelSwitchSetTransmit(
  AGI_PROFILE* pProfile,
  uint8_t sourceEndpoint,
  VOID_CALLBACKFUNC(pCbFunc)(TRANSMISSION_RESULT * pTransmissionResult),
  uint8_t value,
  uint8_t duration)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(JOB_STATUS_SUCCESS);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, JOB_STATUS_BUSY);

  MOCK_CALL_COMPARE_STRUCT_MEMBER_UINT8(
		  p_mock,
		  ARG0,
		  pProfile,
		  agi_profile_t,
		  profile_MS);

  MOCK_CALL_COMPARE_STRUCT_MEMBER_UINT8(
		  p_mock,
		  ARG0,
		  pProfile,
		  agi_profile_t,
		  profile_LS);

  MOCK_CALL_COMPARE_INPUT_UINT8(
		  p_mock,
		  ARG1,
		  sourceEndpoint);

  MOCK_CALL_COMPARE_INPUT_POINTER(
		  p_mock,
		  ARG2,
		  pCbFunc);

  MOCK_CALL_COMPARE_INPUT_UINT8(
		  p_mock,
		  ARG3,
		  value);

  MOCK_CALL_COMPARE_INPUT_UINT8(
		  p_mock,
		  ARG4,
		  duration);

  MOCK_CALL_RETURN_VALUE(p_mock, JOB_STATUS);
}
