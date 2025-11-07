// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file main.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include <stdint.h>
#include <stdio.h>

#include "Assert.h"
#include "SizeOf.h"

#include <FreeRTOSConfigTasks.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#include "zpal_init.h"

/* Driver includes */
//#define DEBUGPRINT
#include "DebugPrint.h"

/* Following two module variables are used by the application (in
 * sleeping devices) to update application timers after wake-up
 * from Deep Sleep.
 * Will be WRITTEN to during setup of task tick (before scheduler
 * is started).
 * Will be READ by application task during application start (with
 * IsWakeupCausedByRtccTimeout() and GetCompletedSleepDurationMs())
 * I.e. no data access synchronization is needed.
 */
static bool     mIsWakeupCausedByRtccTimeout = false;
static uint32_t mCompletedSleepDurationMs    = UINT32_MAX;
static uint32_t mWakeUpTicks                 = UINT32_MAX;
static uint32_t mGoToSleepTicks              = UINT32_MAX;


bool IsWakeupCausedByRtccTimeout(void)
{
  /* mIsWakeupCausedByRtccTimeout is only written to before the scheduler
   * is started. I.e. it is safe to call this function from any thread
   * after the scheduler is started. */
  return mIsWakeupCausedByRtccTimeout;
}

uint32_t GetCompletedSleepDurationMs(void)
{
  /* mCompletedSleepDurationMs is only written to before the scheduler
   * is started. I.e. it is safe to call this function from any thread
   * after the scheduler is started. */
  return mCompletedSleepDurationMs;
}

uint32_t GetDeepSleepWakeupTick(void)
{
  /* mWakeUpTicks is only written to before the scheduler
   * is started. I.e. it is safe to call this function from any thread
   * after the scheduler is started. */
  return mWakeUpTicks;
}

uint32_t GetLastTickBeforeDeepSleep(void)
{
  /* mGoToSleepTicks is only written to before the scheduler
   * is started. I.e. it is safe to call this function from any thread
   * after the scheduler is started. */
  return mGoToSleepTicks;
}

int main(void)
{
  zpal_reset_reason_t eResetReason = ZPAL_RESET_REASON_POWER_ON;

  // TODO: HW initialization

  /***********************************************************************************
   * HW bring-up completed!
   **********************************************************************************/

  zpal_system_startup(eResetReason);
}

/* Idle hook */
void vApplicationIdleHook( void )
{
  // TODO: idle hook implementation
}

/* Stack overflow hook */
void vApplicationStackOverflowHook(__attribute__((unused)) TaskHandle_t pxTask, __attribute__((unused)) char *pcTaskName)
{
  /* This function will get called if a task overflows its stack.   If the
   parameters are corrupt then inspect pxCurrentTCB to find which was the
   offending task. */

  DPRINTF("Stack overflow for task. Name: %s, HandleAddr: %ld\r\n", pcTaskName, (uint32_t)pxTask);
  ASSERT(false);
}

/* Tick hook */
void vApplicationTickHook(void)
{
  // TODO: tick hook implementation
}

// Provide memory for FreeRtos idle task stack and task information
static StaticTask_t xIdleTaskTCB;
static StackType_t uxIdleTaskStack[configMINIMAL_STACK_SIZE];

// Note that application is not application in this case, application seen from FreeRtos.
void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer,
  StackType_t **ppxIdleTaskStackBuffer,
  uint32_t *pulIdleTaskStackSize)
{
  /* Pass out a pointer to the StaticTask_t structure in which the Idle task's
  state will be stored. */
  *ppxIdleTaskTCBBuffer = &xIdleTaskTCB;

  /* Pass out the array that will be used as the Idle task's stack. */
  *ppxIdleTaskStackBuffer = uxIdleTaskStack;

  /* Pass out the size of the array pointed to by *ppxIdleTaskStackBuffer.
  Note that, as the array is necessarily of type StackType_t,
  configMINIMAL_STACK_SIZE is specified in words, not bytes. */
  *pulIdleTaskStackSize = sizeof_array(uxIdleTaskStack);

}

// Provide memory for FreeRtos timer daemon task stack and task information
static StaticTask_t xTimerTaskTCB;
static StackType_t uxTimerTaskStack[configTIMER_TASK_STACK_DEPTH];

// Note that application is not application in this case, application seen from FreeRtos.
void vApplicationGetTimerTaskMemory(StaticTask_t **ppxTimerTaskTCBBuffer,
  StackType_t **ppxTimerTaskStackBuffer,
  uint32_t *pulTimerTaskStackSize)
{
  /* Pass out a pointer to the StaticTask_t structure in which the Timer
  task's state will be stored. */
  *ppxTimerTaskTCBBuffer = &xTimerTaskTCB;

  /* Pass out the array that will be used as the Timer task's stack. */
  *ppxTimerTaskStackBuffer = uxTimerTaskStack;

  /* Pass out the size of the array pointed to by *ppxTimerTaskStackBuffer.
  Note that, as the array is necessarily of type StackType_t,
  configTIMER_TASK_STACK_DEPTH is specified in words, not bytes. */
  *pulTimerTaskStackSize = sizeof_array(uxTimerTaskStack);
}