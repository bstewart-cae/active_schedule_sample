/// ****************************************************************************
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2023 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************

#include <Assert.h>
#include <stdlib.h>

#include <FreeRTOS.h>
#include <timers.h>
#include <zpal_radio.h>
#include <zpal_radio_private.h>
#include <zpal_power_manager.h>
#include <zpal_power_manager_private.h>
#include <lpm.h>
#include <comm_subsystem_drv.h>
//#define DEBUGPRINT // NOSONAR
#include <DebugPrint.h>
#include <cmsis_gcc.h>
#include "chip_define.h"
#include "sysctrl.h"

#define ASSERT_ZPAL_PM_TYPE_T(t) assert(t == ZPAL_PM_TYPE_USE_RADIO || t == ZPAL_PM_TYPE_DEEP_SLEEP)

#define TIMER_BLOCK_TICKS     100  // the time in ticks that a task should block if the timer command queue is full
static uint8_t active_locks[2];
static zpal_pm_mode_t current_mode = ZPAL_PM_MODE_RUNNING;
static zpal_pm_mode_t allowed_mode = ZPAL_PM_MODE_SHUTOFF;

const uint32_t zpal_magic_number = 0x5B9E684D;

typedef struct
{
  uint32_t magic_number;  /*< The magic_number field is used to check the validity of the handle pointer passed to the power_manager APIs,
                               Since the handle type used for the power_manager API is a pointer for void.
                               handle pointer is typecasted to pointer for zpal_pm_power_lock_t inside the power_manager APIs
                               When the zpal_pm_register is called, a unique value is assigned to the magic_number field.*/
  zpal_pm_type_t type;
  bool active;
  bool forever;
  TimerHandle_t timer;
  StaticTimer_t timer_buffer;
} zpal_pm_power_lock_t;

static inline UBaseType_t START_CRITICAL_SECTION( void )
{
    if( 0 != __get_IPSR() ) {
        // In an ISR: mask up to configMAX_SYSCALL_INTERRUPT_PRIORITY
        return portSET_INTERRUPT_MASK_FROM_ISR();
    } else {
        // In thread mode: enter FreeRTOS critical section
        taskENTER_CRITICAL();
        return 0;  // dummy value
    }
}

static inline void END_CRITICAL_SECTION( UBaseType_t oldState )
{
    if( 0 != __get_IPSR() ) {
        // Back in ISR: restore BASEPRI
        portCLEAR_INTERRUPT_MASK_FROM_ISR( oldState );
    } else {
        // Back in thread mode: exit critical section
        taskEXIT_CRITICAL();
    }
}

static void timer_callback( TimerHandle_t xTimer )
{
  zpal_pm_handle_t handle = (zpal_pm_handle_t) pvTimerGetTimerID(xTimer);
  zpal_pm_cancel(handle);
}

// The RF ISR
extern void CommSubsys_Handler(void);

void zpal_pm_enter_sleep(TickType_t sleep_ticks)
{
  if ((allowed_mode > current_mode) && (0 < sleep_ticks))
  {
    zpal_zw_pm_event_handler(current_mode, allowed_mode);
    current_mode = allowed_mode;
    if (allowed_mode == ZPAL_PM_MODE_SLEEP)
    {
      __DSB();
      __WFI();
      __ISB();
      return;
    }
    else if (allowed_mode == ZPAL_PM_MODE_DEEP_SLEEP)
    {
      Lpm_Set_Low_Power_Level(LOW_POWER_LEVEL_SLEEP0);    //Sleep
    }
    else if (allowed_mode == ZPAL_PM_MODE_SHUTOFF)
    {
      zpal_radio_power_shutdown();
      Lpm_Set_Low_Power_Level(LOW_POWER_LEVEL_SLEEP2);    //Deep Sleep
      Lpm_Set_Sram_Sleep_Deepsleep_Shutdown(0x3F);        // keep the top 16kb SRAM powered on
      Lpm_Sub_System_Low_Power_Mode(COMMUMICATION_SUBSYSTEM_PWR_STATE_DEEP_SLEEP);     //if no load fw can call the function (Let subsystem enter sleep mode),  if load fw don't call the function
    }
    Lpm_Enter_Low_Power_Mode();
  }
}

void zpal_pm_exit_sleep(void)
{
  current_mode = ZPAL_PM_MODE_RUNNING;
  zpal_zw_pm_event_handler(allowed_mode, ZPAL_PM_MODE_RUNNING);
 }

zpal_pm_handle_t zpal_pm_register(zpal_pm_type_t type)
{
  ASSERT_ZPAL_PM_TYPE_T(type);
  zpal_pm_power_lock_t *lock = pvPortMalloc(sizeof(zpal_pm_power_lock_t));

  if (NULL == lock) {
    ASSERT(false);
    return NULL;
  }

  lock->magic_number = zpal_magic_number;
  lock->type = type;
  lock->active = false;
  lock->forever = false;

  lock->timer = xTimerCreateStatic( "",
                                    1,
                                    pdFALSE,
                                    lock,
                                    timer_callback,
                                    &lock->timer_buffer);

  ASSERT(lock->timer != NULL);

  DPRINTF("zpal_pm_register, handle: %p, type: %d\n", lock, type);
  return (zpal_pm_handle_t)lock;
}

void zpal_pm_stay_awake(zpal_pm_handle_t handle, uint32_t timeout_ms)
{
  if (NULL == handle)
  {
    return;
  }
  zpal_pm_power_lock_t *lock = (zpal_pm_power_lock_t *)handle;
  ASSERT(lock->magic_number == zpal_magic_number);
  DPRINTF("zpal_pm_stay_awake, handle: %p, timeout: %u, type: %d\n", handle, timeout_ms, lock->type);

  UBaseType_t criticalSectionInterruptState = START_CRITICAL_SECTION();

  if (!lock->active)
  {
    active_locks[lock->type]++;
    DPRINTF("active_locks[%d] = %d\n", lock->type, active_locks[lock->type]);
    lock->active = true;
  }

  if (timeout_ms)
  {
    // Set timeout and start timer
    if( (0 != __get_IPSR()) || (taskSCHEDULER_SUSPENDED == xTaskGetSchedulerState() )) {
      BaseType_t xHigherPriorityTaskWoken = pdFALSE;
      BaseType_t res = xTimerChangePeriodFromISR(lock->timer, pdMS_TO_TICKS(timeout_ms), &xHigherPriorityTaskWoken);
      ASSERT(res == pdPASS);
      /* If xHigherPriorityTaskWoken is now set to pdTRUE then a context switch
      should be performed to ensure the interrupt returns directly to the highest
      priority task.  The macro used for this purpose is dependent on the port in
      use and may be called portEND_SWITCHING_ISR(). */

      portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
    }
    else
    {
      BaseType_t res = xTimerChangePeriod(lock->timer, pdMS_TO_TICKS(timeout_ms), TIMER_BLOCK_TICKS);
      ASSERT(res == pdPASS);
    }
    lock->forever = false;
  }
  else
  {
    if( (0 != __get_IPSR()) || (taskSCHEDULER_SUSPENDED == xTaskGetSchedulerState() )) {
      BaseType_t xHigherPriorityTaskWoken = pdFALSE;
      BaseType_t res = xTimerStopFromISR(lock->timer, &xHigherPriorityTaskWoken);
      ASSERT(res == pdPASS);
      portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
    }
    else
    {
      BaseType_t res = xTimerStop(lock->timer, TIMER_BLOCK_TICKS);
      ASSERT(res == pdPASS);
    }
    lock->forever = true;
  }

  if (active_locks[ZPAL_PM_TYPE_DEEP_SLEEP] > 0)
  {
    allowed_mode = ZPAL_PM_MODE_DEEP_SLEEP;
  }
  if (active_locks[ZPAL_PM_TYPE_USE_RADIO] > 0)
  {
    allowed_mode = ZPAL_PM_MODE_SLEEP;
  }
  END_CRITICAL_SECTION(criticalSectionInterruptState);
}

void zpal_pm_cancel(zpal_pm_handle_t handle)
{
  if (NULL == handle)
  {
    return;
  }
  zpal_pm_power_lock_t *lock = (zpal_pm_power_lock_t *)handle;
  ASSERT(lock->magic_number == zpal_magic_number);
  DPRINTF("zpal_pm_cancel, handle: %p, timer: %p, active: %d, type: %d\n", handle, lock->timer, lock->active, lock->type);

  if (!lock || !lock->active)
  {
    return;
  }

  UBaseType_t criticalSectionInterruptState = START_CRITICAL_SECTION();
  if( (0 != __get_IPSR()) || (taskSCHEDULER_SUSPENDED == xTaskGetSchedulerState() )) {

    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    BaseType_t res = xTimerStopFromISR(lock->timer, &xHigherPriorityTaskWoken);
    ASSERT(res == pdPASS);
    portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
  }
  else
  {
    BaseType_t res = xTimerStop(lock->timer, TIMER_BLOCK_TICKS);
    ASSERT(res == pdPASS);
  }
  lock->forever = false;
  lock->active = false;

  if (active_locks[lock->type] > 0)
  {
    active_locks[lock->type]--;
    DPRINTF("active_locks[%d] = %d\n", lock->type, active_locks[lock->type]);
  }

  if (active_locks[ZPAL_PM_TYPE_USE_RADIO] == 0)
  {
    allowed_mode = ZPAL_PM_MODE_DEEP_SLEEP;

    if (active_locks[ZPAL_PM_TYPE_DEEP_SLEEP] == 0)
    {
      allowed_mode = ZPAL_PM_MODE_SHUTOFF;
    }
  }
  END_CRITICAL_SECTION(criticalSectionInterruptState);
}

void zpal_pm_cancel_all(void)
{
}
