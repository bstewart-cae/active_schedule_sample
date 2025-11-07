// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file test_AppTimer.c
 * @copyright 2022 Silicon Laboratories Inc.
 */

//#define DEBUGPRINT

// Make __asm__() go away - (called by ZW_AppPowerDownCallBack() if debug print is enabled)
#define __asm__(arg)

#include <AppTimer.h>
#include <unity.h>
#include <mock_control.h>
#include <string.h>
#include <zpal_retention_register.h>

void setUpSuite(void) {

}

void tearDownSuite(void) {

}

SSwTimer swTimerA;
SSwTimer swTimerB_DeepSleep;
void SwTimerCallbackA(SSwTimer* pTimer) {};
void SwTimerCallbackB_DeepSleep(SSwTimer* pTimer) {};

static SSwTimer swTimer0;
static SSwTimer swTimer1_DeepSleep;
static SSwTimer swTimer2;
static SSwTimer swTimer3_DeepSleep;
static SSwTimer swTimer4_DeepSleep;
static SSwTimer swTimer5;

void SwTimerCallback0(SSwTimer* pTimer) {};
void SwTimerCallback1_DeepSleep(SSwTimer* pTimer) {};
void SwTimerCallback2(SSwTimer* pTimer) {};
void SwTimerCallback3_DeepSleep(SSwTimer* pTimer) {};
void SwTimerCallback4_DeepSleep(SSwTimer* pTimer) {};
void SwTimerCallback5(SSwTimer* pTimer) {};

extern void AppTimerDeepSleepCallbackWrapper(SSwTimer* pTimer);
extern void ZW_AppPowerDownCallBack(void);

/** 
 *  @{
 * Redefined from AppTimerDeepSleep.c
 * These defines must match else the test fails
 */

/**
 * First (zero based) retention register to use for persisting application
 * timers during Deep Sleep. Other retention registers used for Deep Sleep persistent
 * app timers are defined as offsets from this value.
 */
#define FIRST_APP_TIMER_RETENTION_REGISTER        16

/** Retention register to use for persisting the task tick value at power down */
#define TASKTICK_AT_POWERDOWN_RETENTION_REGISTER  (FIRST_APP_TIMER_RETENTION_REGISTER + 0)

/**
 * Retention register to use for persisting the task tick value when the timer
 * values are saved to retention registers
 */
#define TASKTICK_AT_SAVETIMERS_RETENTION_REGISTER (FIRST_APP_TIMER_RETENTION_REGISTER + 1)

/**
 * First retention register to use for persisting the Deep Sleep persistent application
 * timers during Deep Sleep. (actual number of registers used is determined by
 * how many times AppTimerDeepSleepPersistentRegister() is called).
 */
#define TIMER_VALUES_BEGIN_RETENTION_REGISTER     (FIRST_APP_TIMER_RETENTION_REGISTER + 2)

/** @}*/

/*****************************************************************************/
void test_AppTimerRegister(void)
{
  mock_t* pMock = NULL;
  mock_calls_clear();

  mock_call_use_as_fake(TO_STR(TimerLiaisonInit));

  AppTimerInit(0, NULL); // To initialize structures in AppTimer.c

  mock_call_expect(TO_STR(TimerLiaisonRegister), &pMock);
  pMock->compare_rule_arg[0] = COMPARE_NOT_NULL;
  pMock->expect_arg[1].p = &swTimerA;
  pMock->expect_arg[2].v = false;
  pMock->expect_arg[3].p = SwTimerCallbackA;
  pMock->return_code.v = ESWTIMERLIAISON_STATUS_SUCCESS;

  bool retVal = AppTimerRegister(&swTimerA, false, SwTimerCallbackA);
  TEST_ASSERT_EQUAL(retVal, true);

  mock_calls_verify();
}


/*****************************************************************************/
void test_AppTimerDeepSleepPersistentRegister(void)
{
  mock_t* pMock = NULL;
  mock_calls_clear();

  mock_call_use_as_fake(TO_STR(TimerLiaisonInit));

  AppTimerInit(0, NULL); // To initialize structures in AppTimer.c

  mock_call_expect(TO_STR(TimerLiaisonRegister), &pMock);
  pMock->compare_rule_arg[0] = COMPARE_NOT_NULL;
  pMock->expect_arg[1].p = &swTimerB_DeepSleep;
  pMock->expect_arg[2].v = false;
  pMock->expect_arg[3].p = AppTimerDeepSleepCallbackWrapper;
  pMock->return_code.v = ESWTIMERLIAISON_STATUS_SUCCESS;

  bool retVal = AppTimerDeepSleepPersistentRegister(&swTimerB_DeepSleep, false, SwTimerCallbackB_DeepSleep);
  TEST_ASSERT_EQUAL(retVal, true);

  mock_calls_verify();
}


/*****************************************************************************/
void test_AppTimerDeepSleepPersistentSaveAll(void)
{
  mock_t* pMock = NULL;
  mock_calls_clear();

  uint32_t timerValSet0 = 1100;
  uint32_t timerValSet1 = 1200;
  uint32_t timerValSet2 = 1300;
  uint32_t timerValSet4 = 1500;
  uint32_t taskTickVal  = 10000;
  uint32_t taskTickValSleep = 13000;

  uint32_t retVal;
  uint32_t regValue;

  mock_call_use_as_fake(TO_STR(zpal_retention_register_write));
  mock_call_use_as_fake(TO_STR(zpal_retention_register_read));
  mock_call_use_as_fake(TO_STR(TimerLiaisonInit));
  mock_call_use_as_fake(TO_STR(TimerLiaisonRegister));
  mock_call_use_as_fake(TO_STR(TimerStart));
  mock_call_use_as_fake(TO_STR(TimerGetMsUntilTimeout));

  AppTimerInit(0, NULL); // To initialize structures in AppTimer.c

  /***********************************************************************
   * Register timers
   ***********************************************************************/

  retVal = AppTimerRegister(&swTimer0, false, SwTimerCallback0);
  TEST_ASSERT_EQUAL(true, retVal);
  retVal = AppTimerDeepSleepPersistentRegister(&swTimer1_DeepSleep, false, SwTimerCallback1_DeepSleep);
  TEST_ASSERT_EQUAL(true, retVal);
  retVal = AppTimerRegister(&swTimer2, false, SwTimerCallback2);
  TEST_ASSERT_EQUAL(true, retVal);
  retVal = AppTimerDeepSleepPersistentRegister(&swTimer3_DeepSleep, false, SwTimerCallback3_DeepSleep);
  TEST_ASSERT_EQUAL(true, retVal);
  retVal = AppTimerDeepSleepPersistentRegister(&swTimer4_DeepSleep, false, SwTimerCallback4_DeepSleep);
  TEST_ASSERT_EQUAL(true, retVal);
  retVal = AppTimerRegister(&swTimer5, false, SwTimerCallback5);
  TEST_ASSERT_EQUAL(true, retVal);

  /***********************************************************************
   * Load all timers before starting
   ***********************************************************************/
  mock_call_expect(TO_STR(xTaskGetTickCount), &pMock);
  pMock->return_code.v = taskTickVal;
  AppTimerDeepSleepPersistentLoadAll(ZPAL_RESET_REASON_POWER_ON);

  /***********************************************************************
   * Start timers
   ***********************************************************************/

  /* AppTimerDeepSleepPersistentSaveAll() is implicitly called when starting
   * an Deep Sleep persistent timer */

  mock_call_expect(TO_STR(xTaskGetTickCount), &pMock);
  pMock->return_code.v = taskTickVal;
  retVal = TimerStart(&swTimer0, timerValSet0);
  TEST_ASSERT_EQUAL(ESWTIMER_STATUS_SUCCESS, retVal);

  /* AppTimerDeepSleepPersistentStart() calls StartTimer() and AppTimerDeepSleepPersistentSaveAll().
   * Both calls xTaskGetTickCount() so we expect two calls to xTaskGetTickCount()
   */
  mock_call_expect(TO_STR(xTaskGetTickCount), &pMock);
  pMock->return_code.v = taskTickVal;
  mock_call_expect(TO_STR(xTaskGetTickCount), &pMock);
  pMock->return_code.v = taskTickVal;
  retVal = AppTimerDeepSleepPersistentStart(&swTimer1_DeepSleep, timerValSet1);
  TEST_ASSERT_EQUAL(ESWTIMER_STATUS_SUCCESS, retVal);

  mock_call_expect(TO_STR(xTaskGetTickCount), &pMock);
  pMock->return_code.v = taskTickVal;
  retVal = TimerStart(&swTimer2, timerValSet2);
  TEST_ASSERT_EQUAL(ESWTIMER_STATUS_SUCCESS, retVal);

  // NB: swTimer3_DeepSleep is NOT started

  mock_call_expect(TO_STR(xTaskGetTickCount), &pMock);
  pMock->return_code.v = taskTickVal;
  mock_call_expect(TO_STR(xTaskGetTickCount), &pMock);
  pMock->return_code.v = taskTickVal;
  retVal = AppTimerDeepSleepPersistentStart(&swTimer4_DeepSleep, timerValSet4);
  TEST_ASSERT_EQUAL(ESWTIMER_STATUS_SUCCESS, retVal);


  /***********************************************************************
   * Simulate going to sleep
   ***********************************************************************/
  mock_call_expect(TO_STR(xTaskGetTickCount), &pMock);
  pMock->return_code.v = taskTickValSleep;

  ZW_AppPowerDownCallBack();

  /***********************************************************************
   * Check that all the expected values have been saved to the retention
   * registers
   ***********************************************************************/

  // Task tick value when going to sleep
  retVal = zpal_retention_register_read(TASKTICK_AT_POWERDOWN_RETENTION_REGISTER, &regValue);
  TEST_ASSERT_EQUAL(0, retVal);
  TEST_ASSERT_EQUAL(taskTickValSleep, regValue);

  // Task tick value when saving timers
  retVal = zpal_retention_register_read(TASKTICK_AT_SAVETIMERS_RETENTION_REGISTER, &regValue);
  TEST_ASSERT_EQUAL(0, retVal);
  TEST_ASSERT_EQUAL(taskTickVal, regValue);

  // swTimer1_DeepSleep
  retVal = zpal_retention_register_read(TIMER_VALUES_BEGIN_RETENTION_REGISTER + 0, &regValue);
  TEST_ASSERT_EQUAL(0, retVal);
  TEST_ASSERT_EQUAL(timerValSet1, regValue);

  // swTimer3_DeepSleep (not started)
  retVal = zpal_retention_register_read(TIMER_VALUES_BEGIN_RETENTION_REGISTER + 1, &regValue);
  TEST_ASSERT_EQUAL(0, retVal);
  TEST_ASSERT_EQUAL(UINT32_MAX, regValue);

  // swTimer4_DeepSleep
  retVal = zpal_retention_register_read(TIMER_VALUES_BEGIN_RETENTION_REGISTER + 2, &regValue);
  TEST_ASSERT_EQUAL(0, retVal);
  TEST_ASSERT_EQUAL(timerValSet4, regValue);

  mock_calls_verify();
}


/*****************************************************************************/
void test_AppTimerDeepSleepPersistentResetStorage(void)
{
  mock_calls_clear();

  uint32_t retVal;
  uint32_t regValue;

  mock_call_use_as_fake(TO_STR(zpal_retention_register_write));
  mock_call_use_as_fake(TO_STR(zpal_retention_register_read));

  /* The timers registered and started with test_SaveDeepSleepPersistentAppTimers()
   * should still exist */
  uint32_t firstReg = AppTimerDeepSleepGetFirstRetentionRegister();
  uint32_t lastReg  = AppTimerDeepSleepGetLastRetentionRegister();

  TEST_ASSERT_EQUAL(firstReg, TASKTICK_AT_POWERDOWN_RETENTION_REGISTER);
  TEST_ASSERT_EQUAL(lastReg, TIMER_VALUES_BEGIN_RETENTION_REGISTER + 2);

  /***********************************************************************
   * Ensure the registers are not already cleaned
   * (the previous test test_SaveDeepSleepPersistentAppTimers() should have
   * left them non-zero)
   ***********************************************************************/

  retVal = zpal_retention_register_read(TASKTICK_AT_POWERDOWN_RETENTION_REGISTER, &regValue);
  TEST_ASSERT_EQUAL(0, retVal);
  TEST_ASSERT_NOT_EQUAL(0, regValue);

  retVal = zpal_retention_register_read(TASKTICK_AT_SAVETIMERS_RETENTION_REGISTER, &regValue);
  TEST_ASSERT_EQUAL(0, retVal);
  TEST_ASSERT_NOT_EQUAL(0, regValue);

  // swTimer1_DeepSleep
  retVal = zpal_retention_register_read(TIMER_VALUES_BEGIN_RETENTION_REGISTER + 0, &regValue);
  TEST_ASSERT_EQUAL(0, retVal);
  TEST_ASSERT_NOT_EQUAL(0, regValue);

  // swTimer3_DeepSleep (not started)
  retVal = zpal_retention_register_read(TIMER_VALUES_BEGIN_RETENTION_REGISTER + 1, &regValue);
  TEST_ASSERT_EQUAL(0, retVal);
  TEST_ASSERT_NOT_EQUAL(0, regValue);

  // swTimer4_DeepSleep
  retVal = zpal_retention_register_read(TIMER_VALUES_BEGIN_RETENTION_REGISTER + 2, &regValue);
  TEST_ASSERT_EQUAL(0, retVal);
  TEST_ASSERT_NOT_EQUAL(0, regValue);

  /***********************************************************************
   * Now reset all registers
   ***********************************************************************/

  AppTimerDeepSleepPersistentResetStorage();

  /***********************************************************************
   * Check that all the expected values have been saved to the retention
   * registers
   ***********************************************************************/

  retVal = zpal_retention_register_read(TASKTICK_AT_POWERDOWN_RETENTION_REGISTER, &regValue);
  TEST_ASSERT_EQUAL(0, retVal);
  TEST_ASSERT_EQUAL(0, regValue);

  retVal = zpal_retention_register_read(TASKTICK_AT_SAVETIMERS_RETENTION_REGISTER, &regValue);
  TEST_ASSERT_EQUAL(0, retVal);
  TEST_ASSERT_EQUAL(0, regValue);

  // swTimer1_DeepSleep
  retVal = zpal_retention_register_read(TIMER_VALUES_BEGIN_RETENTION_REGISTER + 0, &regValue);
  TEST_ASSERT_EQUAL(0, retVal);
  TEST_ASSERT_EQUAL(0, regValue);

  // swTimer3_DeepSleep (not started)
  retVal = zpal_retention_register_read(TIMER_VALUES_BEGIN_RETENTION_REGISTER + 1, &regValue);
  TEST_ASSERT_EQUAL(0, retVal);
  TEST_ASSERT_EQUAL(0, regValue);

  // swTimer4_DeepSleep
  retVal = zpal_retention_register_read(TIMER_VALUES_BEGIN_RETENTION_REGISTER + 2, &regValue);
  TEST_ASSERT_EQUAL(0, retVal);
  TEST_ASSERT_EQUAL(0, regValue);

  mock_calls_verify();
}


/*****************************************************************************/
void test_AppTimerDeepSleepPersistentLoadAll(void)
{
  mock_t* pMock = NULL;
  mock_calls_clear();

  uint32_t timerValSet1 = 6100;       // swTimer1_DeepSleep
  uint32_t timerValSet3 = UINT32_MAX; // swTimer3_DeepSleep (not started)
  uint32_t timerValSet4 = 8200;       // swTimer4_DeepSleep
  uint32_t taskTickValAtSave      = 10000;  // Timers saved 2000 ms before sleep
  uint32_t taskTickValAtPowerDown = 12000;

  /* Sleeping for 4100 ms
   *   --> swTimer1 has caused the wakeup (4100 + 2000 = 6100)
   *   --> swTimer4 should keep running for another 2100 ms (8200 - 6100 = 2100)
   */
  uint32_t completedSleepDurationMs = 4100;

  uint32_t timer4Remaining = timerValSet4 - (taskTickValAtPowerDown - taskTickValAtSave) - completedSleepDurationMs;

  uint32_t retVal;

  mock_call_use_as_fake(TO_STR(zpal_retention_register_write));
  mock_call_use_as_fake(TO_STR(zpal_retention_register_read));
  mock_call_use_as_fake(TO_STR(TimerLiaisonInit));
  mock_call_use_as_fake(TO_STR(TimerLiaisonRegister));

  AppTimerInit(0, NULL); // To initialize structures in AppTimer.c

  memset(&swTimer0,     0, sizeof(swTimer0));
  memset(&swTimer1_DeepSleep, 0, sizeof(swTimer1_DeepSleep));
  memset(&swTimer2,     0, sizeof(swTimer2));
  memset(&swTimer3_DeepSleep, 0, sizeof(swTimer3_DeepSleep));
  memset(&swTimer4_DeepSleep, 0, sizeof(swTimer4_DeepSleep));
  memset(&swTimer5,     0, sizeof(swTimer5));

  /***********************************************************************
   * Write values to retention registers
   ***********************************************************************/

  retVal = zpal_retention_register_write(TASKTICK_AT_POWERDOWN_RETENTION_REGISTER, taskTickValAtPowerDown);
  TEST_ASSERT_EQUAL(0, retVal);

  retVal = zpal_retention_register_write(TASKTICK_AT_SAVETIMERS_RETENTION_REGISTER, taskTickValAtSave);
  TEST_ASSERT_EQUAL(0, retVal);

  // swTimer1_DeepSleep
  retVal = zpal_retention_register_write(TIMER_VALUES_BEGIN_RETENTION_REGISTER + 0, timerValSet1);
  TEST_ASSERT_EQUAL(0, retVal);

  // swTimer3_DeepSleep (not started)
  retVal = zpal_retention_register_write(TIMER_VALUES_BEGIN_RETENTION_REGISTER + 1, timerValSet3);
  TEST_ASSERT_EQUAL(0, retVal);

  // swTimer4_DeepSleep
  retVal = zpal_retention_register_write(TIMER_VALUES_BEGIN_RETENTION_REGISTER + 2, timerValSet4);
  TEST_ASSERT_EQUAL(0, retVal);

  /***********************************************************************
   * Register timers
   ***********************************************************************/

  retVal = AppTimerRegister(&swTimer0, false, SwTimerCallback0);
  TEST_ASSERT_EQUAL(true, retVal);
  retVal = AppTimerDeepSleepPersistentRegister(&swTimer1_DeepSleep, false, SwTimerCallback1_DeepSleep);
  TEST_ASSERT_EQUAL(true, retVal);
  retVal = AppTimerRegister(&swTimer2, false, SwTimerCallback2);
  TEST_ASSERT_EQUAL(true, retVal);
  retVal = AppTimerDeepSleepPersistentRegister(&swTimer3_DeepSleep, false, SwTimerCallback3_DeepSleep);
  TEST_ASSERT_EQUAL(true, retVal);
  retVal = AppTimerDeepSleepPersistentRegister(&swTimer4_DeepSleep, false, SwTimerCallback4_DeepSleep);
  TEST_ASSERT_EQUAL(true, retVal);
  retVal = AppTimerRegister(&swTimer5, false, SwTimerCallback5);
  TEST_ASSERT_EQUAL(true, retVal);


  /***********************************************************************
   * Load timers from retention registers
   ***********************************************************************/

  mock_call_expect(TO_STR(IsWakeupCausedByRtccTimeout), &pMock);
  pMock->return_code.v = true;

  mock_call_expect(TO_STR(GetCompletedSleepDurationMs), &pMock);
  pMock->return_code.v = completedSleepDurationMs;

  /* swTimer1_DeepSleep has caused the wakeup so it's callback should be activated */
  mock_call_expect(TO_STR(TimerLiaisonExpiredTimerCallback), &pMock);
  pMock->expect_arg[0].p = &swTimer1_DeepSleep;

  /* swTimer4_DeepSleep should be started to complete its period */
  mock_call_expect(TO_STR(TimerStart), &pMock);
  pMock->expect_arg[0].p = &swTimer4_DeepSleep;
  pMock->expect_arg[1].v = timer4Remaining;
  pMock->return_code.v = ESWTIMER_STATUS_SUCCESS;

  /* Will be called once when saving timers after swTimer4_DeepSleep are restarted */
  mock_call_expect(TO_STR(xTaskGetTickCount), &pMock);
  pMock->return_code.v = 333; // Used below when calling TimerGetMsUntilTimeout()

  mock_call_expect(TO_STR(TimerGetMsUntilTimeout), &pMock);
  pMock->expect_arg[0].p = &swTimer1_DeepSleep;
  pMock->expect_arg[1].v = 333;  // Return value from xTaskGetTickCount
  pMock->compare_rule_arg[2] = COMPARE_NOT_NULL;
  pMock->return_code.v = ESWTIMER_STATUS_SUCCESS;

  mock_call_expect(TO_STR(TimerGetMsUntilTimeout), &pMock);
  pMock->expect_arg[0].p = &swTimer3_DeepSleep;
  pMock->expect_arg[1].v = 333;  // Return value from xTaskGetTickCount
  pMock->compare_rule_arg[2] = COMPARE_NOT_NULL;
  pMock->return_code.v = ESWTIMER_STATUS_SUCCESS;

  mock_call_expect(TO_STR(TimerGetMsUntilTimeout), &pMock);
  pMock->expect_arg[0].p = &swTimer4_DeepSleep;
  pMock->expect_arg[1].v = 333;  // Return value from xTaskGetTickCount
  pMock->compare_rule_arg[2] = COMPARE_NOT_NULL;
  pMock->return_code.v = ESWTIMER_STATUS_SUCCESS;

  AppTimerDeepSleepPersistentLoadAll(ZPAL_RESET_REASON_DEEP_SLEEP_WUT);

  mock_calls_verify();
}

void SwTimerCallback_DeepSleep1(SSwTimer* pTimer) { };
void SwTimerCallback_DeepSleep2(SSwTimer* pTimer) { };

/*****************************************************************************/
void test_AppTimerDeepSleepOverlapping_1(void)
{
  mock_t* pMock = NULL;
  SSwTimer swTimer_DeepSleep1;
  SSwTimer swTimer_DeepSleep2;
  bool retVal;
  const uint32_t taskTickVal  = 10000;
  const uint32_t taskTickValSleep = 20000;
  const uint32_t timerValSet1_1 = 60000;
  const uint32_t timerValSet1_2 = 120000;
  const uint32_t timerValSet2 = 300000;
  /* Sleeping for 40000 ms
   *   --> DeepSleep1 has caused the wakeup (50000 + 10000 = 60000)
   *   --> DeepSleep2 should keep running for another 2100 ms (300000 - 60000 = 240000)
   */
  const uint32_t completedSleepDurationMs = 50000;

  mock_calls_clear();

  mock_call_use_as_fake(TO_STR(zpal_retention_register_write));
  mock_call_use_as_fake(TO_STR(zpal_retention_register_read));
  mock_call_use_as_fake(TO_STR(TimerLiaisonInit));
  mock_call_use_as_fake(TO_STR(TimerLiaisonRegister));
  mock_call_use_as_fake(TO_STR(TimerStart));
  mock_call_use_as_fake(TO_STR(TimerGetMsUntilTimeout));

  AppTimerInit(0, NULL); // To initialize structures in AppTimer.c

  memset(&swTimer_DeepSleep1, 0, sizeof(swTimer_DeepSleep1));
  memset(&swTimer_DeepSleep2, 0, sizeof(swTimer_DeepSleep2));
  /***********************************************************************
  * Register timers and start timers
  ***********************************************************************/
  retVal = AppTimerDeepSleepPersistentRegister(&swTimer_DeepSleep1, false, SwTimerCallback_DeepSleep1);
  TEST_ASSERT_EQUAL(retVal, true);

  mock_call_expect(TO_STR(xTaskGetTickCount), &pMock);
  pMock->return_code.v = taskTickVal;
  retVal = AppTimerDeepSleepPersistentStart(&swTimer_DeepSleep1, timerValSet1_1);
  TEST_ASSERT_EQUAL(ESWTIMER_STATUS_SUCCESS, retVal);

  retVal = AppTimerDeepSleepPersistentRegister(&swTimer_DeepSleep2, false, SwTimerCallback_DeepSleep2);
  TEST_ASSERT_EQUAL(retVal, true);

  mock_call_expect(TO_STR(xTaskGetTickCount), &pMock);
  pMock->return_code.v = taskTickVal;
  retVal = AppTimerDeepSleepPersistentStart(&swTimer_DeepSleep2, timerValSet2);
  TEST_ASSERT_EQUAL(ESWTIMER_STATUS_SUCCESS, retVal);

  mock_call_expect(TO_STR(xTaskGetTickCount), &pMock);
  pMock->return_code.v = taskTickVal;
  AppTimerDeepSleepPersistentLoadAll(ZPAL_RESET_REASON_POWER_ON);

  /***********************************************************************
  * Simulate going to sleep
  ***********************************************************************/
  mock_call_expect(TO_STR(xTaskGetTickCount), &pMock);
  pMock->return_code.v = taskTickValSleep;

  ZW_AppPowerDownCallBack();

  /***********************************************************************
  * Simulate waking up
  ***********************************************************************/

  AppTimerInit(0, NULL); // To initialize structures in AppTimer.c

  memset(&swTimer_DeepSleep1, 0, sizeof(swTimer_DeepSleep1));
  memset(&swTimer_DeepSleep2, 0, sizeof(swTimer_DeepSleep2));
  /***********************************************************************
  * Register timers and start timer
  ***********************************************************************/
  retVal = AppTimerDeepSleepPersistentRegister(&swTimer_DeepSleep1, false, SwTimerCallback_DeepSleep1);
  TEST_ASSERT_EQUAL(retVal, true);

  mock_call_expect(TO_STR(xTaskGetTickCount), &pMock);
  pMock->return_code.v = timerValSet1_1;
  retVal = AppTimerDeepSleepPersistentStart(&swTimer_DeepSleep1, timerValSet1_2);
  TEST_ASSERT_EQUAL(ESWTIMER_STATUS_SUCCESS, retVal);

  retVal = AppTimerDeepSleepPersistentRegister(&swTimer_DeepSleep2, false, SwTimerCallback_DeepSleep2);
  TEST_ASSERT_EQUAL(retVal, true);

  /***********************************************************************
  * Load timers from retention registers
  ***********************************************************************/
  mock_call_expect(TO_STR(IsWakeupCausedByRtccTimeout), &pMock);
  pMock->return_code.v = true;

  mock_call_expect(TO_STR(GetCompletedSleepDurationMs), &pMock);
  pMock->return_code.v = completedSleepDurationMs;

  /* swTimer_DeepSleep1 has caused the wakeup so it's callback should be activated */
  mock_call_expect(TO_STR(TimerLiaisonExpiredTimerCallback), &pMock);
  pMock->expect_arg[0].p = &swTimer_DeepSleep1;

  mock_call_expect(TO_STR(xTaskGetTickCount), &pMock);
  pMock->return_code.v = timerValSet1_1;

  mock_call_expect(TO_STR(xTaskGetTickCount), &pMock);
  pMock->return_code.v = timerValSet1_1;

  AppTimerDeepSleepPersistentLoadAll(ZPAL_RESET_REASON_DEEP_SLEEP_WUT);

  mock_calls_verify();
}
