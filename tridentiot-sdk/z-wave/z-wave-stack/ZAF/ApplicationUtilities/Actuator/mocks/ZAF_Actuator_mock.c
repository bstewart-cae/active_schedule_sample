// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file ZAF_Actuator_mock.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include "ZAF_Actuator.h"
#include "mock_control.h"

#define MOCK_FILE "ZAF_Actuator_mock.c"

//#define DEBUGPRINT
#include <DebugPrint.h>

static void timerExpired_cb(SSwTimer *timer);

void ZAF_Actuator_Init_FAKE(s_Actuator *pActuator,
                               uint8_t minValue,
                               uint8_t maxValue,
                               uint8_t refreshRate,
                               uint8_t durationDefault,
                               zaf_actuator_callback_t cc_callback)
{
  TEST_ASSERT_NOT_NULL(pActuator);
  pActuator->timer.pCallback = &timerExpired_cb; // faking registering the timer
  pActuator->timer.ptr = pActuator;
  pActuator->min = minValue;
  pActuator->max = maxValue;
  pActuator->refreshRate = refreshRate;
  pActuator->durationDefault = durationDefault;
  pActuator->cc_cb = cc_callback;
}

void ZAF_Actuator_Init(s_Actuator *pActuator,
                       uint8_t minValue,
                       uint8_t maxValue,
                       uint16_t refreshRate,
                       uint8_t durationDefault,
                       zaf_actuator_callback_t cc_callback)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_RETURN_VOID_IF_USED_AS_FAKE(ZAF_Actuator_Init_FAKE,
                                        pActuator,
                                        minValue,
                                        maxValue,
                                        refreshRate,
                                        durationDefault,
                                        cc_callback);
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(pMock);
  MOCK_CALL_ACTUAL(pMock, pActuator, minValue, maxValue, refreshRate, durationDefault, cc_callback);

  MOCK_CALL_COMPARE_INPUT_POINTER(pMock, ARG0, pActuator);
  MOCK_CALL_COMPARE_INPUT_UINT8(pMock, ARG1, minValue);
  MOCK_CALL_COMPARE_INPUT_UINT8(pMock, ARG2, maxValue);
  MOCK_CALL_COMPARE_INPUT_UINT16(pMock, ARG3, refreshRate);
  MOCK_CALL_COMPARE_INPUT_UINT8(pMock, ARG4, durationDefault);
  MOCK_CALL_COMPARE_INPUT_POINTER(pMock, ARG5, cc_callback);
}


eActuatorState ZAF_Actuator_StopChange(s_Actuator *pActuator)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(0);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(pMock, 0);
  MOCK_CALL_ACTUAL(pMock, pActuator);

  MOCK_CALL_COMPARE_INPUT_POINTER(pMock, ARG0, pActuator);
  MOCK_CALL_RETURN_VALUE(pMock, eActuatorState);
}

eActuatorState ZAF_Actuator_StartChange(s_Actuator *pActuator,
                                    bool ignoreStartLevel,
                                    bool upDown,
                                    uint8_t startLevel,
                                    uint8_t duration)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(0);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(pMock, 0);
  MOCK_CALL_ACTUAL(pMock, pActuator, ignoreStartLevel, upDown, startLevel, duration);

  MOCK_CALL_COMPARE_INPUT_POINTER(pMock, ARG0, pActuator);
  MOCK_CALL_COMPARE_INPUT_BOOL(pMock, ARG1, ignoreStartLevel);
  MOCK_CALL_COMPARE_INPUT_BOOL(pMock, ARG2, upDown);
  MOCK_CALL_COMPARE_INPUT_UINT8(pMock, ARG3, startLevel);
  MOCK_CALL_COMPARE_INPUT_UINT8(pMock, ARG4, duration);
  MOCK_CALL_RETURN_VALUE(pMock, eActuatorState);
}

eActuatorState ZAF_Actuator_Set_FAKE(s_Actuator *pActuator, uint8_t value, uint8_t duration)
{
  pActuator->valueTarget = value;

  if(pActuator->valueCurrent == pActuator->valueTarget)
  {
    return EACTUATOR_NOT_CHANGING;
  }

  if (duration)
  {
    timerExpired_cb(&pActuator->timer);
  }
  else
  {
    pActuator->valueCurrent = value;
    if (NULL != pActuator->cc_cb)
    {
      void(*callback)(s_Actuator *ds) = pActuator->cc_cb;
      callback(pActuator);
    }
  }
  return duration ? EACTUATOR_CHANGING : EACTUATOR_NOT_CHANGING;
}

eActuatorState ZAF_Actuator_Set(s_Actuator *pActuator, uint8_t value, uint8_t duration)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(0);
  MOCK_CALL_RETURN_IF_USED_AS_FAKE(ZAF_Actuator_Set_FAKE, pActuator, value, duration);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(pMock, 0);
  MOCK_CALL_ACTUAL(pMock, pActuator, value, duration);

  MOCK_CALL_COMPARE_INPUT_POINTER(pMock, ARG0, pActuator);
  MOCK_CALL_COMPARE_INPUT_UINT8(pMock, ARG1, value);
  MOCK_CALL_COMPARE_INPUT_UINT8(pMock, ARG2, duration);
  MOCK_CALL_RETURN_VALUE(pMock, eActuatorState);
}

uint8_t ZAF_Actuator_GetCurrentValue(s_Actuator *pActuator)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(0);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(pMock, 0);
  MOCK_CALL_ACTUAL(pMock, pActuator);

  MOCK_CALL_COMPARE_INPUT_POINTER(pMock, ARG0, pActuator);
  MOCK_CALL_RETURN_VALUE(pMock, uint8_t);
}

uint8_t ZAF_Actuator_GetTargetValue(s_Actuator *pActuator)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(0);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(pMock, 0);
  MOCK_CALL_ACTUAL(pMock, pActuator);

  MOCK_CALL_COMPARE_INPUT_POINTER(pMock, ARG0, pActuator);
  MOCK_CALL_RETURN_VALUE(pMock, uint8_t);
}

uint8_t ZAF_Actuator_GetDurationRemaining(s_Actuator *pActuator)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(0);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(pMock, 0);
  MOCK_CALL_ACTUAL(pMock, pActuator);

  MOCK_CALL_COMPARE_INPUT_POINTER(pMock, ARG0, pActuator);
  MOCK_CALL_RETURN_VALUE(pMock, uint8_t);
}

static void timerExpired_cb(SSwTimer *timer)
{
  /* FAKE:
   * First time just trigger callback function. Then call itself
   * Next time set valueCurrent to valueTarget, to simulate "change done"
   */
  s_Actuator *pActuator = timer->ptr;
  zaf_actuator_callback_t pCC_callback = pActuator->cc_cb;
  pCC_callback(pActuator);

  if (pActuator->valueCurrent != pActuator->valueTarget)
  {
    DPRINTF("%s current: %x, target: %x\n", __func__, pActuator->valueCurrent, pActuator->valueTarget);
    pActuator->valueCurrent = pActuator->valueTarget;
    timerExpired_cb(timer);
  }
}

uint8_t ZAF_Actuator_GetLastOnValue(s_Actuator *pActuator)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(0);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(pMock, 0);
  MOCK_CALL_ACTUAL(pMock, pActuator);

  MOCK_CALL_COMPARE_INPUT_POINTER(pMock, ARG0, pActuator);
  MOCK_CALL_RETURN_VALUE(pMock, uint8_t);
}


uint8_t ZAF_Actuator_GetMax(s_Actuator *pActuator)
{
  mock_t * pMock;
  MOCK_CALL_RETURN_IF_USED_AS_STUB(0);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(pMock, 0);
  MOCK_CALL_ACTUAL(pMock, pActuator);

  MOCK_CALL_COMPARE_INPUT_POINTER(pMock, ARG0, pActuator);
  MOCK_CALL_RETURN_VALUE(pMock, uint8_t);

}
