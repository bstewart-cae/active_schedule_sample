/**
 * @file CC_MultilevelSwitch_Support_mock.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include <CC_MultilevelSwitch_Support.h>
#include <mock_control.h>

#define MOCK_FILE "CC_MultilevelSwitch_Support_mock.c"

void cc_multilevel_switch_set(cc_multilevel_switch_t * p_switch, uint8_t value, uint8_t duration)
{
  mock_t *pMock;
  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(pMock);
  MOCK_CALL_ACTUAL(pMock, p_switch, value, duration);

  MOCK_CALL_COMPARE_INPUT_POINTER(pMock, ARG0, p_switch);
  MOCK_CALL_COMPARE_INPUT_UINT8(pMock, ARG0, value);
  MOCK_CALL_COMPARE_INPUT_UINT8(pMock, ARG0, duration);
}

void cc_multilevel_switch_start_level_change(cc_multilevel_switch_t * p_switch,
                                             bool up,
                                             bool ignore_start_level,
                                             uint8_t start_level,
                                             uint8_t duration)
{
  mock_t *pMock;
  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(pMock);
  MOCK_CALL_ACTUAL(pMock, p_switch, up, ignore_start_level, start_level, duration);

  MOCK_CALL_COMPARE_INPUT_POINTER(pMock, ARG0, p_switch);
  MOCK_CALL_COMPARE_INPUT_BOOL(pMock, ARG1, up);
  MOCK_CALL_COMPARE_INPUT_BOOL(pMock, ARG2, ignore_start_level);
  MOCK_CALL_COMPARE_INPUT_UINT8(pMock, ARG3, start_level);
  MOCK_CALL_COMPARE_INPUT_UINT8(pMock, ARG0, duration);
}

void cc_multilevel_switch_stop_level_change(cc_multilevel_switch_t * p_switch)
{
  mock_t *pMock;
  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(pMock);
  MOCK_CALL_ACTUAL(pMock, p_switch);
  MOCK_CALL_COMPARE_INPUT_POINTER(pMock, ARG0, p_switch);
}

uint8_t cc_multilevel_switch_get_current_value(cc_multilevel_switch_t * p_switch)
{
  mock_t *pMock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(0);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(pMock, 0);
  MOCK_CALL_ACTUAL(pMock, p_switch);
  MOCK_CALL_COMPARE_INPUT_POINTER(pMock, ARG0, p_switch);
  MOCK_CALL_RETURN_VALUE(pMock, uint8_t);
}

uint8_t cc_multilevel_switch_get_last_on_value(cc_multilevel_switch_t * p_switch)
{
  mock_t *pMock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(0);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(pMock, 0);
  MOCK_CALL_ACTUAL(pMock, p_switch);
  MOCK_CALL_COMPARE_INPUT_POINTER(pMock, ARG0, p_switch);
  MOCK_CALL_RETURN_VALUE(pMock, uint8_t);
}
