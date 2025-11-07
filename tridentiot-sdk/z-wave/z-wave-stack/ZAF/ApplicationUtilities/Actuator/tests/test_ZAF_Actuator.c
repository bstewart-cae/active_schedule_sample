// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file test_ZAF_Actuator.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include "ZAF_Actuator.h"
#include <mock_control.h>
#include <test_common.h>

#include <string.h>
#include <SizeOf.h>
#include <stdlib.h>

void setUpSuite(void) {

}

void tearDownSuite(void) {

}

/*
 * Tests description
 *
 * Formulas
 * stepValue = (target-current)/steps
 *
 * ** CMD SET ***
 * 1. SETUP: min = 0,max= 255, currentValue = 0, refreshRate = 10 ms
 *    NOTE: Test very short time interval with a big number of steps
 *    INPUT: command SET, duration = 1s, target = 255.
 *    EXPECTED: 100 steps, refreshRate = 10, stepValue = 255/100 = 2
 *
 * 2. SETUP: min = 0,max= 255, currentValue = 0, refreshRate = 10 ms
 *    INPUT: command SET, duration = 10s, target = 255.
 *    EXPECTED: 255 steps, refreshRate = 39, stepValue = 1
 *
 * 3. SETUP: min = 0,max= 255, currentValue = 0, refreshRate = 10 ms
 *    NOTE: Test duration in minutes conversion
 *    INPUT: command SET, duration = 1min, target = 255.
 *    EXPECTED: 255 steps, refreshRate = 235, stepValue = 1
 *
 * 4. SETUP: min = 0, max= 255, currentValue = 30, refreshRate = 10 ms
 *    NOTE: Test long time interval with a small number of steps, and max time interval
 *    INPUT: command SET, duration = 126min, target = 50.
 *    EXPECTED: 20 steps, refreshRate =378000ms , stepValue = 1
 *
 * 5. SETUP: min = 0, max= 100, currentValue = 0, refreshRate = 10 ms
 *    NOTE: Test default for Switch Multilevel
 *    INPUT: command SET, duration = 1s, target = 100.
 *    EXPECTED:
 *
 * 6. SETUP: min = 0, max= 255, currentValue = 200, refreshRate = 10 ms
 *    NOTE: To test decreasing change
 *    INPUT: command SET, duration = 1s, target = 50.
 *    EXPECTED:
 *
 * ** CMD START_LEVEL_CHANGE ***
 * 1. SETUP: min = 0,max= 255, currentValue = 0, refreshRate = 10 ms
 *    INPUT: command START_LEVEL_CHANGE, duration =1s, direction UP, ignoreLevelChange TRUE
 *    EXPECTED: 100 steps, refreshRate = 10, stepValue = 2
 *
 * 2. SETUP: min = 0,max= 255, currentValue = 0, refreshRate = 10 ms
 *    INPUT: command START_LEVEL_CHANGE, duration =30s, direction UP, ignoreLevelChange TRUE
 *    EXPECTED:
 *
 * 3. SETUP: min = 0,max= 255, currentValue = 127, refreshRate = 10 ms
 *    INPUT: command START_LEVEL_CHANGE, duration = 1s, direction UP, ignoreLevelChange TRUE
 *    EXPECTED:
 *
 * 4. SETUP: min = 0,max= 255, currentValue = 0, refreshRate = 10 ms
 *    NOTE: Behavior should be the same as  testSTART_LEVEL_CHANGE.2
 *    INPUT: command START_LEVEL_CHANGE, duration = 1s, direction UP, ignoreLevelChange FALSE, startLevel 127
 *    EXPECTED:
 *
 * 5. SETUP: min = 0,max= 255, currentValue = 127, refreshRate = 10 ms
 *    INPUT: command START_LEVEL_CHANGE, duration = 1s, direction DOWN, ignoreLevelChange TRUE
 *    EXPECTED:
 *
 * 6. SETUP: min = 0,max= 100 currentValue = 0, refreshRate = 10 ms
 *    INPUT: command START_LEVEL_CHANGE, duration = 10s, direction UP, ignoreLevelChange TRUE
 *    EXPECTED:
 *
 * 7. SETUP: min = 0,max= 100 currentValue = 0, refreshRate = 10 ms
 *    INPUT: command START_LEVEL_CHANGE, duration = 10s, direction DOWN, ignoreLevelChange FALSE, startLevel max
 *    EXPECTED: No change, already at target value
 *
 * 8. SETUP1: min = 0,max= 100 currentValue = 0, refreshRate = 100 ms
 *    INPUT: command START_LEVEL_CHANGE, duration = 10s, direction UP, ignoreLevelChange TRUE
 *    RESET.
 *    SETUP2: min = 0,max= 100 currentValue = 0, refreshRate = 1000 ms
 *    INPUT: command START_LEVEL_CHANGE, duration = 10s, direction UP, ignoreLevelChange TRUE
 *    EXPECTED: stepsCount2 < stepsCount1
 *
 * ** CMD STOP_LEVEL_CHANGE ***
 * 1. SETUP: min = 0,max= 255, refreshRate = 10 ms
 *    INPUT: START_LEVEL_CHANGE and stop in half. Call STOP_LEVEL_CHANGE
 *    EXPECTED: Return status: changing.
 *
 * 2. INPUT: command STOP_LEVEL_CHANGE while there is no ongoing change.
 *    EXPECTED: Return status: not changing.
 */
#define VALUE_X_10(x) (10 * (x))
#define DEFAULT_DURATION  (1)

static int counter = 0;

static void actuator_cb(s_Actuator *pActuator);
static void resetActuatorValues(s_Actuator *pAct);
static void print_actuator(s_Actuator *a);
static uint8_t getStepNumber(s_Actuator *a);
static void helper_test_ZAF_Actuator_TimerExpired(s_Actuator *a, uint8_t duration);
static void helper_testActuatorSet_duration(s_Actuator *a, uint8_t value, uint8_t duration);

/*
 * Pointer to callback function ZAF_Actuator_TimerExpired. It's the only function used in ZAF_Actuator_Init.
 * So it can be reused in all tests.
 */
static void (*timer_expired_cb)(SSwTimer*);

/* ************************************************************************************************** */
void test_ZAF_Actuator_Init(void)
{
  mock_t *pMock = NULL;
  mock_calls_clear();

  s_Actuator actuator;
  uint8_t min = 0;
  uint8_t max = 100;
  uint8_t refreshRate = 20;
  memset(&actuator, 0x00, sizeof(s_Actuator));
  mock_call_expect(TO_STR(AppTimerRegister), &pMock);
  pMock->expect_arg[0].p = &actuator.timer;
  pMock->expect_arg[1].v = true;
  pMock->compare_rule_arg[2] = COMPARE_NOT_NULL;

  ZAF_Actuator_Init(&actuator, min, max, refreshRate, DEFAULT_DURATION, &actuator_cb);
  timer_expired_cb = pMock->actual_arg[2].p;

  TEST_ASSERT_EQUAL_UINT16_MESSAGE(0, ZAF_Actuator_GetDurationRemaining(&actuator), "Wrong duration!");
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(0, ZAF_Actuator_GetCurrentValue(&actuator), "Wrong current value!");
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(0, ZAF_Actuator_GetTargetValue(&actuator), "Wrong target value!");
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(max, ZAF_Actuator_GetLastOnValue(&actuator), "Wrong last on value!");
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(max, ZAF_Actuator_GetMax(&actuator), "Wrong maximum!");
  mock_calls_verify();
}

void test_ZAF_Actuator_Set_pass_durationSec(void)
{
  mock_t *pMock = NULL;
  mock_calls_clear();

  s_Actuator a;
  memset(&a, 0x00, sizeof(s_Actuator));
  mock_call_use_as_stub(TO_STR(AppTimerRegister));
  uint8_t min = 0;
  uint8_t max = 255;
  uint8_t refreshRate = 20;
  ZAF_Actuator_Init(&a, min, max, refreshRate, DEFAULT_DURATION, &actuator_cb);

  /* CASE0: duration = 0 (instant change) */
  uint8_t value = 255;
  uint8_t duration = 0; //in seconds
  uint8_t status = ZAF_Actuator_Set(&a, value, duration);
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(value, ZAF_Actuator_GetTargetValue(&a), "Wrong target value!");
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(refreshRate, a.defaultRefreshRate, "Wrong refresh rate value!");
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(EACTUATOR_NOT_CHANGING, status, "Wrong return status!");
  mock_calls_verify();

  /* CASE1.1: duration = 1sec, target value = 255 (just call, no timers testing)*/
  mock_calls_clear();
  resetActuatorValues(&a);
  duration = 1; //in seconds

  mock_call_expect(TO_STR(TimerIsActive), &pMock);  //Only because TimerIsActive mock returns true.
  pMock->expect_arg[0].p = &a.timer;
  pMock->return_code.v = false;

  mock_call_expect(TO_STR(TimerStart), &pMock);
  pMock->expect_arg[0].p = &a.timer;
  pMock->expect_arg[1].v = a.refreshRate;

  status = ZAF_Actuator_Set(&a, value, duration);
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(ZAF_Actuator_GetTargetValue(&a), value, "Wrong target value status!");
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(EACTUATOR_CHANGING, status, "Wrong return status!");
  mock_calls_verify();

  /* CASE1.2: Set value to 70.
   *          Then call SET again with duration 1 sec and value 70 and make sure it returns immediately */
  mock_calls_clear();
  duration = 0;
  value = 70;
  resetActuatorValues(&a);
  status = ZAF_Actuator_Set(&a, value, duration);
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(value, ZAF_Actuator_GetTargetValue(&a), "Wrong target value!");
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(0, ZAF_Actuator_GetDurationRemaining(&a), "Wrong remaining duration!");
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(EACTUATOR_NOT_CHANGING, status, "Wrong return status!");

  duration = 1; //in seconds
  status = ZAF_Actuator_Set(&a, value, duration);
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(0, ZAF_Actuator_GetDurationRemaining(&a), "Wrong remaining duration!");
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(EACTUATOR_NOT_CHANGING, status, "Wrong return status!");
  mock_calls_verify();

  /* CASE1.3: duration = 1sec, target value = 255, test timers */
  duration = 1; //in seconds
  resetActuatorValues(&a);
  helper_testActuatorSet_duration(&a, value, duration);

  /* CASE2: duration = 10sec, target value = 255*/
  duration = 10; //in seconds
  resetActuatorValues(&a);
  helper_testActuatorSet_duration(&a, value, duration);
}

void test_ZAF_Actuator_Set_pass_durationMinutes(void)
{
  mock_calls_clear();
  s_Actuator a;
  memset(&a, 0x00, sizeof(s_Actuator));
  mock_call_use_as_stub(TO_STR(AppTimerRegister));

  uint8_t min = 0;
  uint8_t max = 255;
  uint8_t refreshRate = 20;

  ZAF_Actuator_Init(&a, min, max, refreshRate, DEFAULT_DURATION, &actuator_cb);

  /* CASE3: duration = 1min, target value = 255  */
  uint8_t value = 255;
  uint8_t duration = 0x80; //in minutes
  resetActuatorValues(&a);
  helper_testActuatorSet_duration(&a, value, duration);

  /* CASE4: duration = 127 minutes(max), current value = 30, target value=50 */
  value = 50;
  duration = 0xFE;
  resetActuatorValues(&a);
  a.valueCurrent = VALUE_X_10(30); // Set current value to 30.
  helper_testActuatorSet_duration(&a, value, duration);

  /* CASE5: duration = 127 minutes(max), current value = 0(min), target value=255(max) */
    value = max;
    duration = 0xFE;
    resetActuatorValues(&a);
    a.valueCurrent = VALUE_X_10(0); // Set current value to 0.
    helper_testActuatorSet_duration(&a, value, duration);
}

void test_ZAF_Actuator_Set_pass_MultilevelSwitchCase(void)
{
  mock_calls_clear();
  s_Actuator a;
  memset(&a, 0x00, sizeof(s_Actuator));
  mock_call_use_as_stub(TO_STR(AppTimerRegister));

  uint8_t min = 0;
  uint8_t max = 100;
  uint8_t refreshRate = 20;

  ZAF_Actuator_Init(&a, min, max, refreshRate, DEFAULT_DURATION, &actuator_cb);

  /* CASE5: duration = 1s, target value = 100  */
  uint8_t value = 100;
  uint8_t duration = 1;
  resetActuatorValues(&a);
  helper_testActuatorSet_duration(&a, value, duration);
}


void test_ZAF_Actuator_Set_pass_Decreasing(void)
{
  mock_calls_clear();
  s_Actuator a;
  memset(&a, 0x00, sizeof(s_Actuator));

  mock_call_use_as_stub(TO_STR(AppTimerRegister));

  uint8_t min = 0;
  uint8_t max = 255;
  uint8_t refreshRate = 20;
  ZAF_Actuator_Init(&a, min, max, refreshRate, DEFAULT_DURATION, &actuator_cb);

  /* CASE6: duration = 1s, target value = 100, current value = 200  */
  uint8_t value = 100;
  uint8_t duration = 1;
  resetActuatorValues(&a);
  a.valueCurrent = VALUE_X_10(200); // Set current value
  helper_testActuatorSet_duration(&a, value, duration);
}

void test_ZAF_Actuator_Set_fail(void)
{
  mock_calls_clear();

  /*
   *case1.1: value higher than max
   *case1.2: value less than min
   * Verify that actuator data was not changed. */
  s_Actuator a;
  memset(&a, 0x00, sizeof(s_Actuator));
  mock_call_use_as_stub(TO_STR(AppTimerRegister));
  uint8_t min = 50;
  uint8_t max = 100;
  uint8_t refreshRate = 20;
  ZAF_Actuator_Init(&a, min, max, refreshRate, DEFAULT_DURATION, &actuator_cb);

  uint8_t value = min-1;
  uint8_t duration = 10;
  uint8_t currentValue = ZAF_Actuator_GetCurrentValue(&a);
  uint8_t targetValue = ZAF_Actuator_GetTargetValue(&a);
  uint8_t status = ZAF_Actuator_Set(&a, value, duration);

  TEST_ASSERT_EQUAL_UINT16_MESSAGE(EACTUATOR_FAILED, status, "Wrong return status!");
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(currentValue, ZAF_Actuator_GetCurrentValue(&a) , "Wrong current value!");
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(targetValue, ZAF_Actuator_GetTargetValue(&a), "Wrong target value!");
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(0, ZAF_Actuator_GetDurationRemaining(&a), "Wrong duration remaining!");

  value = max+1;
  status = ZAF_Actuator_Set(&a, value, duration);
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(EACTUATOR_FAILED, status, "Wrong return status!");
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(currentValue, ZAF_Actuator_GetCurrentValue(&a) , "Wrong current value!");
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(targetValue, ZAF_Actuator_GetTargetValue(&a), "Wrong target value!");
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(0, ZAF_Actuator_GetDurationRemaining(&a), "Wrong duration remaining!");
  mock_calls_verify();
}

void test_ZAF_Actuator_defaultDuration(void)
{
  /*
   * Test that defaultDuration is set properly.
   * If defaultDuration is 0 -> instant change
   * If defaultDuration > 0 -> timed change change
   */
  mock_t *pMock = NULL;
  mock_calls_clear();

  s_Actuator a;
  memset(&a, 0x00, sizeof(s_Actuator));
  mock_call_use_as_stub(TO_STR(AppTimerRegister));
  uint8_t min = 0;
  uint8_t max = 255;
  uint8_t refreshRate = 100;
  uint8_t defaultDuration = 0; // default is instant change
  ZAF_Actuator_Init(&a, min, max, refreshRate, defaultDuration, &actuator_cb);

  uint8_t value = 255;
  uint8_t duration = 0xFF; // use factory default
  uint8_t status = ZAF_Actuator_Set(&a, value, duration);
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(value, ZAF_Actuator_GetTargetValue(&a), "Wrong target value!");
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(ZAF_Actuator_GetTargetValue(&a), ZAF_Actuator_GetCurrentValue(&a), "Wrong current value!");
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(EACTUATOR_NOT_CHANGING, status, "Wrong return status!");
  mock_calls_verify();

  /* All the same, but default duration is non-zero*/
  memset(&a, 0x00, sizeof(s_Actuator));
  mock_call_use_as_stub(TO_STR(AppTimerRegister));
  defaultDuration = 10; // don't care, non zero value
  ZAF_Actuator_Init(&a, min, max, refreshRate, defaultDuration, &actuator_cb);

  mock_call_expect(TO_STR(TimerIsActive), &pMock);
  pMock->expect_arg[0].p = &a.timer;
  pMock->return_code.v = false;

  mock_call_expect(TO_STR(TimerStart), &pMock);
  pMock->expect_arg[0].p = &a.timer;
  pMock->expect_arg[1].v = a.refreshRate;

  status = ZAF_Actuator_Set(&a, value, duration);
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(ZAF_Actuator_GetTargetValue(&a), value, "Wrong target value status!");
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(EACTUATOR_CHANGING, status, "Wrong return status!");
  mock_calls_verify();
}

void test_ZAF_Actuator_StartChange_pass(void)
{
  mock_calls_clear();

  s_Actuator a;
  memset(&a, 0x00, sizeof(s_Actuator));
  mock_call_use_as_stub(TO_STR(AppTimerRegister));
  uint8_t min = 0;
  uint8_t max = 255;
  uint8_t refreshRate = 20;
  ZAF_Actuator_Init(&a, min, max, refreshRate, DEFAULT_DURATION, &actuator_cb);

  /* CASE0.1: duration = 0 (instant change) */
  bool ignoreStartLevel = true;
  bool upDown = false; // increasing
  uint8_t duration = 0;

  uint8_t status = ZAF_Actuator_StartChange(&a, ignoreStartLevel, upDown, 0, duration);
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(EACTUATOR_NOT_CHANGING, status, "Wrong return status!");
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(ZAF_Actuator_GetTargetValue(&a), ZAF_Actuator_GetCurrentValue(&a), "Wrong current value!");
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(max, ZAF_Actuator_GetTargetValue(&a), "Wrong target value!");
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(max, ZAF_Actuator_GetLastOnValue(&a), "Wrong last on value!");
  mock_calls_verify();

  /* CASE0.2:  duration =1s, direction UP, ignoreLevelChange FALSE, startLevel = max */
  mock_calls_clear();
  resetActuatorValues(&a);
  ignoreStartLevel = false;
  upDown = false; // increasing
  duration = 1;
  uint8_t startLevel = max;

  status = ZAF_Actuator_StartChange(&a, ignoreStartLevel, upDown, startLevel, duration);
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(1, counter, "CC callback should be triggered when startLevel was set!");

  TEST_ASSERT_EQUAL_UINT16_MESSAGE(EACTUATOR_NOT_CHANGING, status, "Wrong return status!");
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(startLevel, ZAF_Actuator_GetCurrentValue(&a), "Wrong current value!");
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(max, ZAF_Actuator_GetTargetValue(&a), "Wrong target value!");
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(startLevel, ZAF_Actuator_GetLastOnValue(&a), "Wrong last on value!");
  mock_calls_verify();

  /* CASE1:  duration =1s, direction UP, ignoreLevelChange TRUE */
  mock_calls_clear();
  resetActuatorValues(&a);
  duration = 1;

  mock_t *pMock = NULL;
  mock_call_expect(TO_STR(TimerIsActive), &pMock);
  pMock->expect_arg[0].p = &a.timer;
  pMock->return_code.v = false;
  mock_call_use_as_stub(TO_STR(TimerStart));

  status = ZAF_Actuator_StartChange(&a, ignoreStartLevel, upDown, 0, duration);
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(EACTUATOR_CHANGING, status, "Wrong return status!");
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(0, ZAF_Actuator_GetCurrentValue(&a), "Wrong current value!");
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(max, ZAF_Actuator_GetTargetValue(&a), "Wrong target value!");
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(0, ZAF_Actuator_GetLastOnValue(&a), "Wrong last on value!");
  helper_test_ZAF_Actuator_TimerExpired(&a, duration);
  mock_calls_verify();

  /* CASE2:  duration =30s, direction UP, ignoreLevelChange TRUE */
  mock_calls_clear();
  resetActuatorValues(&a);
  duration = 30;

  mock_call_expect(TO_STR(TimerIsActive), &pMock);
  pMock->expect_arg[0].p = &a.timer;
  pMock->return_code.v = false;
  mock_call_use_as_stub(TO_STR(TimerStart));

  status = ZAF_Actuator_StartChange(&a, ignoreStartLevel, upDown, 0, duration);
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(EACTUATOR_CHANGING, status, "Wrong return status!");

  helper_test_ZAF_Actuator_TimerExpired(&a, duration);
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(max, ZAF_Actuator_GetTargetValue(&a), "Wrong target value!");
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(ZAF_Actuator_GetTargetValue(&a), ZAF_Actuator_GetCurrentValue(&a), "Wrong current value!");
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(ZAF_Actuator_GetCurrentValue(&a), ZAF_Actuator_GetLastOnValue(&a), "Wrong last on value!");
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(0, ZAF_Actuator_GetDurationRemaining(&a), "Wrong duration!");
  mock_calls_verify();

  /* CASE3:  duration =30s, direction UP, ignoreLevelChange TRUE */
  mock_calls_clear();
  resetActuatorValues(&a);
  duration = 1;
  a.valueCurrent = VALUE_X_10(127);

  mock_call_expect(TO_STR(TimerIsActive), &pMock);
  pMock->expect_arg[0].p = &a.timer;
  pMock->return_code.v = false;

  mock_call_use_as_stub(TO_STR(TimerStart));

  status = ZAF_Actuator_StartChange(&a, ignoreStartLevel, upDown, 0, duration);
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(EACTUATOR_CHANGING, status, "Wrong return status!");

  helper_test_ZAF_Actuator_TimerExpired(&a, duration);
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(max, ZAF_Actuator_GetTargetValue(&a), "Wrong target value!");
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(ZAF_Actuator_GetTargetValue(&a), ZAF_Actuator_GetCurrentValue(&a), "Wrong current value!");
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(ZAF_Actuator_GetCurrentValue(&a), ZAF_Actuator_GetLastOnValue(&a), "Wrong last on value!");
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(0, ZAF_Actuator_GetDurationRemaining(&a), "Wrong duration!");
  mock_calls_verify();

  /* CASE4:  duration =1s, direction UP, ignoreLevelChange FALSE, startLevel 127 */
  mock_calls_clear();
  resetActuatorValues(&a);
  duration = 1;
  ignoreStartLevel = false;
  upDown = false;
  startLevel = 127;

  mock_call_expect(TO_STR(TimerIsActive), &pMock);
  pMock->expect_arg[0].p = &a.timer;
  pMock->return_code.v = false;

  mock_call_use_as_stub(TO_STR(TimerStart));

  status = ZAF_Actuator_StartChange(&a, ignoreStartLevel, upDown, startLevel, duration);
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(EACTUATOR_CHANGING, status, "Wrong return status!");
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(1, counter, "CC callback should be triggered when startLevel was set!");

  helper_test_ZAF_Actuator_TimerExpired(&a, duration);
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(max, ZAF_Actuator_GetTargetValue(&a), "Wrong target value!");
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(ZAF_Actuator_GetTargetValue(&a), ZAF_Actuator_GetCurrentValue(&a), "Wrong current value!");
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(ZAF_Actuator_GetCurrentValue(&a), ZAF_Actuator_GetLastOnValue(&a), "Wrong last on value!");
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(0, ZAF_Actuator_GetDurationRemaining(&a), "Duration must be 0 when target value was reached");
  mock_calls_verify();

  /* CASE5:  duration =2s, direction DOWN, ignoreLevelChange FALSE, startLevel 127 */
  /* Duration is 2 secs, because duration will cause an instant change with this start value. */
  mock_calls_clear();
  resetActuatorValues(&a);
  duration = 2;
  ignoreStartLevel = false;
  upDown = true;
  startLevel = 127;

  mock_call_expect(TO_STR(TimerIsActive), &pMock);
  pMock->expect_arg[0].p = &a.timer;
  pMock->return_code.v = false;

  mock_call_use_as_stub(TO_STR(TimerStart));

  status = ZAF_Actuator_StartChange(&a, ignoreStartLevel, upDown, startLevel, duration);
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(EACTUATOR_CHANGING, status, "Wrong return status!");
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(1, counter, "CC callback should be triggered when startLevel was set!");

  helper_test_ZAF_Actuator_TimerExpired(&a, duration);
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(min, ZAF_Actuator_GetTargetValue(&a), "Wrong target value!");
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(ZAF_Actuator_GetTargetValue(&a), ZAF_Actuator_GetCurrentValue(&a), "Wrong current value!");
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(startLevel, ZAF_Actuator_GetLastOnValue(&a), "Wrong last on value!");
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(0, ZAF_Actuator_GetDurationRemaining(&a), "Duration must be 0 when target value was reached!");
  mock_calls_verify();
}

void test_ZAF_Actuator_StartChange_pass2(void)
{
  mock_t *pMock = NULL;
  mock_calls_clear();

  s_Actuator a;
  memset(&a, 0x00, sizeof(s_Actuator));
  mock_call_use_as_stub(TO_STR(AppTimerRegister));

  uint8_t min = 0;
  uint8_t max = 100;
  uint16_t refreshRate = 100;
  ZAF_Actuator_Init(&a, min, max, refreshRate, DEFAULT_DURATION, &actuator_cb);

  /* CASE6: duration = 10s, direction UP, ignoreLevelChange FALSE, */
  bool ignoreStartLevel = true;
  bool upDown = false; // increasing
  uint8_t duration = 10;

  mock_calls_clear();
  resetActuatorValues(&a);

  mock_call_expect(TO_STR(TimerIsActive), &pMock);
  pMock->expect_arg[0].p = &a.timer;
  pMock->return_code.v = false;

  mock_call_use_as_stub(TO_STR(TimerStart));

  uint8_t status = ZAF_Actuator_StartChange(&a, ignoreStartLevel, upDown, 0, duration);
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(EACTUATOR_CHANGING, status, "Wrong return status!");
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(0, ZAF_Actuator_GetCurrentValue(&a), "Wrong current value!");
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(max, ZAF_Actuator_GetTargetValue(&a), "Wrong target value!");

  helper_test_ZAF_Actuator_TimerExpired(&a, duration);
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(max, ZAF_Actuator_GetTargetValue(&a), "Wrong target value!");
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(ZAF_Actuator_GetTargetValue(&a), ZAF_Actuator_GetCurrentValue(&a), "Wrong current value!");
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(ZAF_Actuator_GetCurrentValue(&a), ZAF_Actuator_GetLastOnValue(&a), "Wrong last on value!");
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(0, ZAF_Actuator_GetDurationRemaining(&a), "Duration must be 0 when target value was reached");
  uint8_t CCTriggeredCount1 = counter; // save how many times CC callback was triggered.
  mock_calls_verify();

  /* CASE7: duration = 10s, direction DOWN,  ignoreLevelChange FALSE, startLevel max*/
  ignoreStartLevel = false;
  upDown = true; // decreasing
  uint8_t startLevel = max;

  mock_calls_clear();
  resetActuatorValues(&a);

  mock_call_expect(TO_STR(TimerIsActive), &pMock);
  pMock->expect_arg[0].p = &a.timer;
  pMock->return_code.v = false;
  mock_call_use_as_stub(TO_STR(TimerStart));

  status = ZAF_Actuator_StartChange(&a, ignoreStartLevel, upDown, startLevel, duration);
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(EACTUATOR_CHANGING, status, "Wrong return status!");
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(max, ZAF_Actuator_GetCurrentValue(&a), "Wrong current value!");
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(min, ZAF_Actuator_GetTargetValue(&a), "Wrong target value!");

  helper_test_ZAF_Actuator_TimerExpired(&a, duration);
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(min, ZAF_Actuator_GetTargetValue(&a), "Wrong target value!");
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(ZAF_Actuator_GetTargetValue(&a), ZAF_Actuator_GetCurrentValue(&a), "Wrong current value!");
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(startLevel, ZAF_Actuator_GetLastOnValue(&a), "Wrong last on value!");
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(0, ZAF_Actuator_GetDurationRemaining(&a), "Duration must be 0 when target value was reached");
  mock_calls_verify();

  /* CASE8: Compare count of app updates on higher and lower refresh rate.
   * The smaller refresh rate is, the app should be refreshed higher number of times
   * Repeat the same steps as for CASE6, but with refresh rate 1 second.*/
  mock_calls_clear();
  counter = 0;
  memset(&a, 0, sizeof(a));
  refreshRate = 1000;

  mock_call_expect(TO_STR(AppTimerRegister), &pMock);
  pMock->expect_arg[0].p = &a.timer;
  pMock->expect_arg[1].v = true;
  pMock->compare_rule_arg[2] = COMPARE_NOT_NULL;
  ZAF_Actuator_Init(&a, min, max, refreshRate, DEFAULT_DURATION, &actuator_cb);

  ignoreStartLevel = true;
  upDown = false; // increasing
  duration = 10;
  mock_call_expect(TO_STR(TimerIsActive), &pMock);
  pMock->expect_arg[0].p = &a.timer;
  pMock->return_code.v = false;
  mock_call_use_as_stub(TO_STR(TimerStart));

  status = ZAF_Actuator_StartChange(&a, ignoreStartLevel, upDown, 0, duration);
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(EACTUATOR_CHANGING, status, "Wrong return status!");
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(0, ZAF_Actuator_GetCurrentValue(&a), "Wrong current value!");
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(max, ZAF_Actuator_GetTargetValue(&a), "Wrong target value!");

  helper_test_ZAF_Actuator_TimerExpired(&a, duration);
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(max, ZAF_Actuator_GetTargetValue(&a), "Wrong target value!");
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(ZAF_Actuator_GetTargetValue(&a), ZAF_Actuator_GetCurrentValue(&a), "Wrong current value!");
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(ZAF_Actuator_GetCurrentValue(&a), ZAF_Actuator_GetLastOnValue(&a), "Wrong last on value!");
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(0, ZAF_Actuator_GetDurationRemaining(&a), "Duration must be 0 when target value was reached");

  uint8_t CCTriggeredCount2 = counter; // save how many times CC callback was triggered.
  char message[80];
  snprintf(message, sizeof(message), "Number of steps taken should be less when refresh rate is higher: %u < %u", CCTriggeredCount1, CCTriggeredCount2);
  TEST_ASSERT_TRUE_MESSAGE(CCTriggeredCount1 > CCTriggeredCount2, message);
  mock_calls_verify();
}


void test_ZAF_Actuator_StartChange_fail(void)
{
  /*
   * Try to start the change with invalid start level and make sure it fails
   */
  mock_calls_clear();

  s_Actuator a;
  memset(&a, 0x00, sizeof(s_Actuator));
  mock_call_use_as_stub(TO_STR(AppTimerRegister));

  uint8_t min = 10;
  uint8_t max = 100;
  uint16_t refreshRate = 100;
  ZAF_Actuator_Init(&a, min, max, refreshRate, DEFAULT_DURATION, &actuator_cb);

  /* duration = 1s, direction UP, ignoreLevelChange FALSE, startLevel invalid*/
  bool ignoreStartLevel = false;
  bool upDown = false; // increasing
  uint8_t duration = 1;
  ignoreStartLevel = false;
  upDown = true; // decreasing
  uint8_t startLevel = max + 1; // invalid value
  uint8_t status = ZAF_Actuator_StartChange(&a, ignoreStartLevel, upDown, startLevel, duration);
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(EACTUATOR_FAILED, status, "Wrong return status!");

  startLevel = min - 1; // invalid value
  status = ZAF_Actuator_StartChange(&a, ignoreStartLevel, upDown, startLevel, duration);
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(EACTUATOR_FAILED, status, "Wrong return status!");

  mock_calls_verify();
}

void test_ZAF_Actuator_StopChange_pass(void)
{
  mock_t *pMock = NULL;
  mock_calls_clear();

  s_Actuator a;
  memset(&a, 0x00, sizeof(s_Actuator));
  mock_call_use_as_stub(TO_STR(AppTimerRegister));

  uint8_t min = 0;
  uint8_t max = 255;
  uint16_t refreshRate = 100;
  ZAF_Actuator_Init(&a, min, max, refreshRate, DEFAULT_DURATION, &actuator_cb);

  /* CASE1: currentValue != targetValue.*/

  /* Start the change. */
  bool ignoreStartLevel = true;
  bool upDown = false; // increasing
  uint8_t duration = 10;
  mock_call_expect(TO_STR(TimerIsActive), &pMock);
  pMock->expect_arg[0].p = &a.timer;
  pMock->return_code.v = false;
  mock_call_use_as_stub(TO_STR(TimerStart));

  uint8_t status = ZAF_Actuator_StartChange(&a, ignoreStartLevel, upDown, 0, duration);
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(EACTUATOR_CHANGING, status, "Wrong return status!");
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(max, ZAF_Actuator_GetTargetValue(&a), "Wrong target value!");

  /* Stop in the middle of transition*/
  uint8_t expectedStepsCount = getStepNumber(&a);
  for (int i= 0; i< expectedStepsCount/2; i++)
  {
    uint16_t currentValueTmp = i * a.singleStepValue;
    TEST_ASSERT_EQUAL_UINT16_MESSAGE(a.valueCurrent, currentValueTmp, "Wrong current value status!");
    timer_expired_cb(&a.timer);
  }
  TEST_ASSERT_TRUE_MESSAGE(max > ZAF_Actuator_GetCurrentValue(&a), "Change in progress, should be MIN < currentVale < MAX!");
  TEST_ASSERT_TRUE_MESSAGE(min < ZAF_Actuator_GetCurrentValue(&a), "Change in progress, should be MIN < currentVale < MAX!");
  TEST_ASSERT_TRUE_MESSAGE(ZAF_Actuator_GetDurationRemaining(&a), "Change in progress, should be duration > 0");

  mock_call_expect(TO_STR(TimerIsActive), &pMock);
  pMock->expect_arg[0].p = &a.timer;
  pMock->return_code.v = true;
  mock_call_expect(TO_STR(TimerStop), &pMock);
  pMock->expect_arg[0].p = &a.timer;

  status = ZAF_Actuator_StopChange(&a);
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(EACTUATOR_CHANGING, status, "Wrong return status!");
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(ZAF_Actuator_GetCurrentValue(&a), ZAF_Actuator_GetTargetValue(&a), "Wrong target value!");

  /* call ZAF_Actuator_StopChange and make sure there was no ongoing change this time*/
  mock_call_expect(TO_STR(TimerIsActive), &pMock);
  pMock->expect_arg[0].p = &a.timer;
  pMock->return_code.v = false;

  status = ZAF_Actuator_StopChange(&a);
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(EACTUATOR_NOT_CHANGING, status, "Wrong return status!");
  mock_calls_verify();
}


void test_ZAF_Actuator_GetLastOnValue(void)
{
  mock_calls_clear();

  /* 1. SET value to positive value.
   * 2. SET value to 0
   * 3. SET to positive value.
   * Verify it matches value from step 1 */
  s_Actuator a;
  memset(&a, 0x00, sizeof(s_Actuator));
  mock_call_use_as_stub(TO_STR(AppTimerRegister));

  uint8_t min = 0;
  uint8_t max = 255;
  uint8_t refreshRate = 20;
  ZAF_Actuator_Init(&a, min, max, refreshRate, DEFAULT_DURATION, &actuator_cb);

  uint8_t value = 70;
  uint8_t duration = 0; //in seconds
  uint8_t status = ZAF_Actuator_Set(&a, value, duration);
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(value, ZAF_Actuator_GetTargetValue(&a), "Wrong target value!");
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(value, ZAF_Actuator_GetCurrentValue(&a), "Wrong current value!");
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(value, ZAF_Actuator_GetLastOnValue(&a), "Wrong last on value!");
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(EACTUATOR_NOT_CHANGING, status, "Wrong return status!");

  status = ZAF_Actuator_Set(&a, 0, duration);
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(0, ZAF_Actuator_GetCurrentValue(&a), "Wrong current value!");
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(EACTUATOR_NOT_CHANGING, status, "Wrong return status!");

  status = ZAF_Actuator_Set(&a, ZAF_Actuator_GetLastOnValue(&a), duration);
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(value, ZAF_Actuator_GetCurrentValue(&a), "Wrong current value!");
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(EACTUATOR_NOT_CHANGING, status, "Wrong return status!");

  mock_calls_verify();
}

/* ***************************************** */
/*       Helper functions                    */
/* ***************************************** */

/* Calls and tests ZAF_ActuatorSet with different parameters, when duration>0*/
static void helper_testActuatorSet_duration(s_Actuator *a, uint8_t value, uint8_t duration)
{
  mock_t *pMock = NULL;

  mock_call_expect(TO_STR(TimerIsActive), &pMock);
  pMock->expect_arg[0].p = &a->timer;
  pMock->return_code.v = false;
  mock_call_use_as_stub(TO_STR(TimerStart));

  uint8_t status = ZAF_Actuator_Set(a, value, duration);
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(ZAF_Actuator_GetTargetValue(a), value, "Wrong target value status!");
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(EACTUATOR_CHANGING, status, "Wrong return status!");

  helper_test_ZAF_Actuator_TimerExpired(a, duration);
}

/*
 * Tests timer callback, as if it was triggered by timer timeout. Also it was calculated how many times
 * callback is called before final value is reached
 */
static void helper_test_ZAF_Actuator_TimerExpired(s_Actuator *a, uint8_t d)
{
  print_actuator(a);

  uint8_t expectedStepsCount = getStepNumber(a);
  uint16_t currentValue = a->valueCurrent;
  bool decreasing = !a->directionUp;
  counter = 0;

  printf("V:");
  for (int i= 0; i< expectedStepsCount; i++)
  {
    uint16_t currentValueTmp = decreasing ? (currentValue - i*a->singleStepValue) : (currentValue + i*a->singleStepValue);
    printf (" %u",ZAF_Actuator_GetCurrentValue(a));
    TEST_ASSERT_EQUAL_UINT16_MESSAGE(a->valueCurrent, currentValueTmp, "Wrong current value status!");
    timer_expired_cb(&a->timer);
  }
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(expectedStepsCount, counter, "CC callback triggered wrong number of times!");

  // Change is done, now expect that timer stops
  mock_call_use_as_stub(TO_STR(TimerStop));
  timer_expired_cb(&a->timer);
  printf (" %u\n",ZAF_Actuator_GetCurrentValue(a));
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(ZAF_Actuator_GetCurrentValue(a), ZAF_Actuator_GetTargetValue(a), "Wrong target value status!");
  TEST_ASSERT_EQUAL_UINT32_MESSAGE(ZAF_Actuator_GetDurationRemaining(a), 0, "Wrong remaining duration!");
  // Make sure that Remaining duration is always within the limits (0x00 - 0xFD)
  TEST_ASSERT_FALSE_MESSAGE(ZAF_Actuator_GetDurationRemaining(a) == 0xFE, "Invalid remaining duration!");
}

static void actuator_cb(s_Actuator *pActuator)
{
  counter++;
//  printf("%s: %d\n", __func__, counter);
}

static void resetActuatorValues(s_Actuator *pAct)
{
  counter = 0;
  pAct->lastOnValue = 0;
  pAct->valueCurrent = 0;
  pAct->valueTarget = 0;
  pAct->singleStepValue = 0;
  pAct->refreshRate = pAct->defaultRefreshRate;
}

/* debug only */
static void print_actuator(s_Actuator *a)
{
  printf("Actuator: vCurrent=%u\n"
      "\t vTarget=%u\n"
      "\t vStep=%u\n"
      "\t Up=%u\n"
      "\t dRefreshRate=%u\n"
      "\t refreshRate=%u\n"
      "\t max=%u\n",
      a->valueCurrent, a->valueTarget,
      a->singleStepValue, a->directionUp, a->defaultRefreshRate, a->refreshRate, a->max);
}

static uint8_t getStepNumber(s_Actuator *a)
{
  uint8_t expectedStepsCount = abs(a->valueTarget - a->valueCurrent) / a->singleStepValue;
  printf("expectedStepsCount: %u=|%u-%u|/%u\n", expectedStepsCount, a->valueTarget, a->valueCurrent, a->singleStepValue);
  return expectedStepsCount;
}

