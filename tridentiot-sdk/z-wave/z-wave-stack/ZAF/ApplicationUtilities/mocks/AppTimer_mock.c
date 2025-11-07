// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
* @file AppTimer_mock.c
* @brief Application Timer module
*
* @copyright 2018 Silicon Laboratories Inc.
*/


/* Z-Wave includes */
#include <AppTimer.h>

#include "mock_control.h"
#include "unity.h"

#define MOCK_FILE "AppTimer_mock.c"


void
AppTimerInit(
             uint8_t iTaskNotificationBitNumber,
             void * ReceiverTask
            )
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(p_mock);
  MOCK_CALL_ACTUAL(p_mock, iTaskNotificationBitNumber, ReceiverTask);

  MOCK_CALL_COMPARE_INPUT_UINT8(p_mock, ARG0, iTaskNotificationBitNumber);
  MOCK_CALL_COMPARE_INPUT_POINTER(p_mock, ARG1, ReceiverTask);
}


bool
AppTimerRegister(
                 SSwTimer* pTimer,
                 bool bAutoReload,
                 void(*pCallback)(SSwTimer* pTimer)
                )
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(true);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, false);
  MOCK_CALL_ACTUAL(p_mock, pTimer, bAutoReload, pCallback);

  MOCK_CALL_COMPARE_INPUT_POINTER(p_mock, ARG0, pTimer);
  MOCK_CALL_COMPARE_INPUT_BOOL(p_mock, ARG1, bAutoReload);
  MOCK_CALL_COMPARE_INPUT_POINTER(p_mock, ARG2, pCallback);

  MOCK_CALL_RETURN_IF_ERROR_SET(p_mock, bool);
  MOCK_CALL_RETURN_VALUE(p_mock, bool);
}

void
AppTimerNotificationHandler(void)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(p_mock);
}


void AppTimerSetReceiverTask(void * ReceiverTask)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(p_mock);
  MOCK_CALL_ACTUAL(p_mock, ReceiverTask);
  MOCK_CALL_COMPARE_INPUT_UINT32(p_mock, ARG0, ReceiverTask);
}


bool AppTimerDeepSleepPersistentRegister(SSwTimer *pTimer,
                                   bool bAutoReload,
                                   void (*pCallback)(SSwTimer *pTimer))
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(true);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, false);
  MOCK_CALL_ACTUAL(p_mock, pTimer, bAutoReload, pCallback);

  MOCK_CALL_COMPARE_INPUT_POINTER(p_mock, ARG0, pTimer);
  MOCK_CALL_COMPARE_INPUT_BOOL(p_mock, ARG1, bAutoReload);
  MOCK_CALL_COMPARE_INPUT_POINTER(p_mock, ARG2, pCallback);

  MOCK_CALL_RETURN_IF_ERROR_SET(p_mock, bool);
  MOCK_CALL_RETURN_VALUE(p_mock, bool);
}


ESwTimerStatus AppTimerDeepSleepPersistentStart(SSwTimer *pTimer, uint32_t iTimeout)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(ESWTIMER_STATUS_SUCCESS);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, ESWTIMER_STATUS_FAILED);
  MOCK_CALL_ACTUAL(p_mock, pTimer, iTimeout);

  MOCK_CALL_COMPARE_INPUT_POINTER(p_mock, ARG0, pTimer);
  MOCK_CALL_COMPARE_INPUT_UINT32(p_mock, ARG1, iTimeout);

  MOCK_CALL_RETURN_IF_ERROR_SET(p_mock, ESwTimerStatus);
  MOCK_CALL_RETURN_VALUE(p_mock, ESwTimerStatus);
}


ESwTimerStatus AppTimerDeepSleepPersistentRestart(SSwTimer *pTimer)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(ESWTIMER_STATUS_SUCCESS);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, ESWTIMER_STATUS_FAILED);
  MOCK_CALL_ACTUAL(p_mock, pTimer);

  MOCK_CALL_COMPARE_INPUT_POINTER(p_mock, ARG0, pTimer);

  MOCK_CALL_RETURN_IF_ERROR_SET(p_mock, ESwTimerStatus);
  MOCK_CALL_RETURN_VALUE(p_mock, ESwTimerStatus);
}


ESwTimerStatus AppTimerDeepSleepPersistentStop(SSwTimer *pTimer)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(ESWTIMER_STATUS_SUCCESS);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, ESWTIMER_STATUS_FAILED);
  MOCK_CALL_ACTUAL(p_mock, pTimer);

  MOCK_CALL_COMPARE_INPUT_POINTER(p_mock, ARG0, pTimer);

  MOCK_CALL_RETURN_IF_ERROR_SET(p_mock, ESwTimerStatus);
  MOCK_CALL_RETURN_VALUE(p_mock, ESwTimerStatus);
}


void AppTimerDeepSleepPersistentResetStorage(void)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(p_mock);
}


void AppTimerDeepSleepPersistentSaveAll(void)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(p_mock);
}

void AppTimerDeepSleepPersistentLoadAll(zpal_reset_reason_t resetReason)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(p_mock);
  MOCK_CALL_ACTUAL(p_mock, resetReason);
  MOCK_CALL_COMPARE_INPUT_INT32(p_mock, ARG0, resetReason);
}


uint32_t AppTimerDeepSleepGetFirstRetentionRegister(void)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(0);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, 0);
  MOCK_CALL_RETURN_VALUE(p_mock, uint32_t);
}


uint32_t AppTimerDeepSleepGetLastRetentionRegister(void)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(0);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, 0);
  MOCK_CALL_RETURN_VALUE(p_mock, uint32_t);
}
