// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file
 * See header file for description!
 *
 * @copyright 2020 Silicon Laboratories Inc.
 */

#include <ZW_UserTask.h>
#include <ZW_system_startup_api.h>
#include <stdbool.h>

//#define DEBUGPRINT
#include <DebugPrint.h>

/****************************************************************
 * CONFIGURATIONS OF THIS MODULE *
 ***************************************************************/

/**
 * These configurations are hidden from the application developers.
 */

#define USERTASK_TASK_COUNT_MAX               4  ///< The total number of tasks that can be created by this module.

/****************************************************************
 * DEFINITIONS
 ***************************************************************/

/****************************************************************
 * TYPEDEF and CONSTANTS
 ***************************************************************/

/****************************************************************
 * MACROS *
 ***************************************************************/

/**
 * This will check if the RTOS scheduler has ever been running.
 * This is used primarily to restrict activity of this module to
 * before the vTaskStartScheduler() has been called.
 * Tasks cannot be dynamically created at run-time!
 */
#define HAS_SCHEDULER_BEEN_RUNNING()    do {                                                           \
                                          if (ZW_system_startup_IsSchedulerStarted() == true)          \
                                          {                                                            \
                                            return Code_Fail_NotAllowed;                               \
                                          }                                                            \
                                        } while(0)


/****************************************************************
 * EXTERNAL VARIABLES (none preferred)
 ***************************************************************/

/****************************************************************
 * FORWARD DECLARATIONS (none preferred)
 ***************************************************************/

/****************************************************************
 * STATIC VARIABLES
 ***************************************************************/

/*
 * The internal states and variables of this singleton module!
 */
static struct {
  bool    isInitialized;      ///< Used to read if this module is initialized!
  uint8_t nbrOfCreatedTasks;  ///< Used to cuont the number of tasks created by this module!
  bool    mainAppCreated;     ///< Used to restrict the main app from being created more than once!
} m_userTaskInstance = {0};

/************************************
 * STACK and TASK memory allocations
 * of the old mainAPP-task!
 ***********************************/
/*
 * TODO: Should we allocate StackBuffer dynamically to make sure that it has the same number of
 * indexes as the size of iStackSize? That's what FreeRTOS recommends here:
 * https://www.freertos.org/xTaskCreateStatic.html
 */
// Task and stack buffer allocation for the default/main application task!
static StaticTask_t AppTaskBuffer;
static uint8_t      AppStackBuffer[TASK_STACK_SIZE_MAIN_USER_APP];

/****************************************************************
 * STATIC FUNCTIONS
 ***************************************************************/

/****************************************************************
 * API FUNCTIONS
 ***************************************************************/

ReturnCode_t ZW_UserTask_Init(void)
{
  HAS_SCHEDULER_BEEN_RUNNING();

  if (m_userTaskInstance.isInitialized == true)
  {
    return Code_Fail_InvalidState;  // Already initialized
  }

  m_userTaskInstance.isInitialized = true;
  m_userTaskInstance.nbrOfCreatedTasks = 0;
  m_userTaskInstance.mainAppCreated = false;

  return Code_Success;
}

ReturnCode_t ZW_UserTask_CreateTask(ZW_UserTask_t* task, TaskHandle_t* xHandle)
{
  // Used for converting the stackBuffer-array and its size!
  StackType_t* stackBuf = NULL;
  uint16_t stackSizeConverted = 0;

  TaskHandle_t taskHandle = NULL;

  HAS_SCHEDULER_BEEN_RUNNING();  // Block calls after scheduler has started!

  if (m_userTaskInstance.isInitialized != true)
  {
    return Code_Fail_InvalidState;  // Module not yet initialized!
  }

  if (m_userTaskInstance.nbrOfCreatedTasks >= USERTASK_TASK_COUNT_MAX)
  {
    return Code_Fail_InvalidOperation;  // Task number limit is reached!
  }

  // Lift the USERTASK priority value with TASK_PRIORITY_BACKGROUND to align with FreeRTOS priority pool.
  UBaseType_t priority = task->priority + TASK_PRIORITY_BACKGROUND;

  // Is the priority out of bounds for User-Tasks?
  if (priority < TASK_PRIORITY_BACKGROUND || priority > TASK_PRIORITY_HIGHEST)
  {
    DPRINT("\r\nInvalid priority setting!");
    return Code_Fail_InvalidParameter;
  }

  // Check other parameters
  if (task->pTaskFunc == NULL
      || task->taskBuffer->stackBuffer == NULL
      || task->taskBuffer->taskBuffer == NULL)
  {
    return Code_Fail_InvalidParameter;
  }

  // END OF CHECKS ////////////////////////////////////////////////////

  // Apply app-name if needed!
  if (task->pTaskName == NULL)
  {
    task->pTaskName = TASK_NAME_MAIN_USER_APP;
  }

  /*
   * Since we now deal with a StackType_t-array, we need to convert the stack-depth accordingly.
   * If the conversion does not align on StackType_t boundary, the division will round down the value,
   * which will not cause any issue!
   */
  stackBuf = (StackType_t*) task->taskBuffer->stackBuffer;  // Converts from a byte-array into a StackType_t-array.
  stackSizeConverted = (task->taskBuffer->stackBufferLength / sizeof(StackType_t));

  // Create the task with given settings!
  taskHandle = xTaskCreateStatic(
                                  task->pTaskFunc,               // pvTaskCode
                                  task->pTaskName,               // pcName
                                  stackSizeConverted,            // usStackDepth
                                  task->pUserTaskParam,          // pvParameters
                                  priority,                      // uxPriority
                                  stackBuf,                      // pxStackBuffer
                                  task->taskBuffer->taskBuffer   // pxTaskBuffer
                                );

  if (xHandle != NULL)
  {
    *xHandle = taskHandle;  // Return handle to caller
  }

  // Check for creation success
  if (taskHandle == NULL)
  {
    DPRINT("\r\nFailed to create a UserTask!");
    return Code_Fail_Unknown;  // This should never happen, since we have covered all cases above.
  }

  m_userTaskInstance.nbrOfCreatedTasks++;  // Register the task creation!

  DPRINTF("\r\nA UserTask was created! (Name: %s)", task->pTaskName);
  return Code_Success;  // Task created!
}


bool ZW_UserTask_ApplicationRegisterTask(  VOID_CALLBACKFUNC(appTaskFunc)(SApplicationHandles*),
                                           uint8_t iZwRxQueueTaskNotificationBitNumber,
                                           uint8_t iZwCommandStatusQueueTaskNotificationBitNumber,
                                           const SProtocolConfig_t * pProtocolConfig)
{
  if(m_userTaskInstance.mainAppCreated == true)
  {
    DPRINT("\r\nZW_UserTask_ApplicationRegisterTask() can only be called once with success!");
    return false;  // The main app is already created!
  }

  // Register the association between Event Notification Bit Number and the event handler of the APP!
  ZW_system_startup_SetEventNotificationBitNumbers(iZwRxQueueTaskNotificationBitNumber,
                                                   iZwCommandStatusQueueTaskNotificationBitNumber,
                                                   pProtocolConfig);

  // Create the buffer bundle!
  ZW_UserTask_Buffer_t mainAppTaskBuffer = {
    .taskBuffer = &AppTaskBuffer,
    .stackBuffer = AppStackBuffer,
    .stackBufferLength = TASK_STACK_SIZE_MAIN_USER_APP
  };

  // Create the task setting-structure!
  ZW_UserTask_t task = {
    .pTaskFunc = (TaskFunction_t)appTaskFunc,
    .pTaskName = TASK_NAME_MAIN_USER_APP,
    .pUserTaskParam = ZW_system_startup_getAppHandles(),
    .priority = USERTASK_PRIORITY_HIGHEST,
    .taskBuffer = &mainAppTaskBuffer
  };

  // Create the task!
  TaskHandle_t xHandle = NULL;
  ZW_UserTask_CreateTask(&task, &xHandle);

  ASSERT(xHandle != NULL);

  // Created successfully?
  if (xHandle == NULL)
  {
    return false;  // Failed
  }
  m_userTaskInstance.mainAppCreated = true;  // Block further main app creation!

  ZW_system_startup_SetMainApplicationTaskHandle(xHandle);  // Pass the task handle to the system_startup's AppInterface structure.

  return true;
}

bool ZW_ApplicationRegisterTask(  VOID_CALLBACKFUNC(appTaskFunc)(SApplicationHandles*),
                                  uint8_t iZwRxQueueTaskNotificationBitNumber,
                                  uint8_t iZwCommandStatusQueueTaskNotificationBitNumber,
                                  const SProtocolConfig_t * pProtocolConfig)
{
  return ZW_UserTask_ApplicationRegisterTask(
                                              appTaskFunc,
                                              iZwRxQueueTaskNotificationBitNumber,
                                              iZwCommandStatusQueueTaskNotificationBitNumber,
                                              pProtocolConfig
                                            );
}

/****************************************************************
 * THREAD FUNCTION *
 ***************************************************************/

