// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file mock_system_startup.c
 * @copyright 2022 Silicon Laboratories Inc.
 *
 * @brief Startup function for Z-Wave when runnming FreeRTOS.
 */
#include <mock_control.h>

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"

#include "ZW_application_transport_interface.h"

#define MOCK_FILE "mock_system_startup.c"

/*======================= vApplicationStackOverflowHook =======================
**  Stack overflow handler
**----------------------------------------------------------------------------*/
void
vApplicationStackOverflowHook(TaskHandle_t pxTask, char *pcTaskName)
{
  ;
}

SApplicationHandles* ZW_system_startup_getAppHandles(void)
{
  return NULL;
}

void ZW_system_startup_SetMainApplicationTaskHandle(__attribute__((unused)) TaskHandle_t xHandle)
{
}

bool ZW_system_startup_IsSchedulerStarted(void)
{
  return false;
}

void ZW_system_startup_SetEventNotificationBitNumbers(__attribute__((unused)) uint8_t iZwRxQueueTaskNotificationBitNumber,
                                                      __attribute__((unused)) uint8_t iZwCommandStatusQueueTaskNotificationBitNumber,
                                                      __attribute__((unused)) const SProtocolConfig_t * pProtocolConfig)
{
}

bool
ZW_ApplicationRegisterTask(
                            VOID_CALLBACKFUNC(appTaskFunc)(SApplicationHandles*),
                            uint8_t iZwRxQueueTaskNotificationBitNumber,
                            uint8_t iZwCommandStatusQueueTaskNotificationBitNumber,
                            SProtocolConfig_t * pProtocolConfig
                          )
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(true);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, false);
  MOCK_CALL_ACTUAL(
		  p_mock,
		  appTaskFunc,
		  iZwRxQueueTaskNotificationBitNumber,
		  iZwCommandStatusQueueTaskNotificationBitNumber,
		  pProtocolConfig);

  MOCK_CALL_COMPARE_INPUT_POINTER(
          p_mock,
          ARG0,
          appTaskFunc);

  MOCK_CALL_COMPARE_INPUT_UINT8(
          p_mock,
          ARG1,
          iZwRxQueueTaskNotificationBitNumber);

  MOCK_CALL_COMPARE_INPUT_UINT8(
         p_mock,
         ARG2,
         iZwCommandStatusQueueTaskNotificationBitNumber);

  MOCK_CALL_COMPARE_INPUT_POINTER(
          p_mock,
          ARG3,
          pProtocolConfig);

  MOCK_CALL_RETURN_VALUE(p_mock, bool);
}


void
vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer,
  StackType_t **ppxIdleTaskStackBuffer,
  uint32_t *pulIdleTaskStackSize)
{
  ;
}


void
vApplicationGetTimerTaskMemory(StaticTask_t **ppxTimerTaskTCBBuffer,
  StackType_t **ppxTimerTaskStackBuffer,
  uint32_t *pulTimerTaskStackSize)
{
  ;
}

void vApplicationIdleHook( void )
{
  ;
}

bool IsWakeupCausedByRtccTimeout(void)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(false);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, false);
  MOCK_CALL_RETURN_VALUE(p_mock, bool);
}

uint32_t GetCompletedSleepDurationMs(void)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(0);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, 0);
  MOCK_CALL_RETURN_VALUE(p_mock, uint32_t);
}

zpal_reset_reason_t GetResetReason(void)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(ZPAL_RESET_REASON_POWER_ON);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, ZPAL_RESET_REASON_OTHER);

  MOCK_CALL_RETURN_VALUE(p_mock, zpal_reset_reason_t);
}

void ZW_system_startup_SetCCSet(SCommandClassSet_t *xHandle)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(pMock);

  MOCK_CALL_ACTUAL(pMock, xHandle);
  MOCK_CALL_COMPARE_INPUT_POINTER(pMock, ARG0, xHandle);  
}

SCommandClassSet_t *ZW_system_startup_GetCCSet(void)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(NULL);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, NULL);

  MOCK_CALL_RETURN_VALUE(p_mock, SCommandClassSet_t*);
}