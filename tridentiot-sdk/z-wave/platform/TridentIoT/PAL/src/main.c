/// ****************************************************************************
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************

#include "FreeRTOS.h"
#include "task.h"
#include "SizeOf.h"
#include <stdbool.h>
#include "MfgTokens.h"
#include "queue.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zpal_init.h>
#include <zpal_nvm.h>
#include <zpal_watchdog.h>
#include <zpal_gpio_private.h>
#include <zpal_misc_private.h>
#include <zpal_defs.h>
#include <zpal_bootloader.h>
#include "chip_define.h"
#include "tr_platform.h"
#include "tr_mfg_tokens.h"
#include <lpm.h>
#include <rtc.h>
#include <dpd.h>
#include "mp_sector.h"
#include "sysctrl.h"
#include "sysfun.h"
#include <flashctl.h>
#include "comm_subsystem_drv.h"

#include <sys/stat.h>
#if (CHIP_MODEL == CHIP_ID(RT584, RT58X_MPB)) || (CHIP_MODEL == CHIP_ID(RT584, RT58X_MPA))
#include <rtc_util.h>
#endif
#include "zpal_radio_private.h"
extern uint8_t __ret_sram_start__;
extern uint8_t __ret_sram_end__;

extern void zpal_watchdog_init(void);
extern void zpal_retention_register_clear(void);
extern uint32_t GetLastTickBeforeDeepSleep(void);

#define CONFIG_IDLE_TASK_STACK_SIZE   1024



/* Following 2 module variables are used by the application (in
 * sleeping devices) to update application timers after wake-up
 * from Deep Sleep.
 * Will be WRITTEN to during setup of task tick (before scheduler
 * is started).
 * Will be READ by application task during application start (with
 * IsWakeupCausedByRtccTimeout() and GetCompletedSleepDurationMs()
 * I.e. no data access synchronization is needed.
 */

static bool     mIsWakeupCausedByRtccTimeout = false;
static uint32_t mCompletedSleepDurationMs    = UINT32_MAX;
static uint32_t mWakeUpTicks                 = UINT32_MAX;

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

#ifdef DEBUG
typedef struct soc_gpio_drv1_s
{
  uint32_t grpio_drv_sel16: 2;
  uint32_t grpio_drv_sel17: 2;
  uint32_t grpio_drv_sel18: 2;
  uint32_t grpio_drv_sel19: 2;
  uint32_t grpio_drv_sel20: 2;
  uint32_t grpio_drv_sel21: 2;
  uint32_t grpio_drv_sel22: 2;
  uint32_t grpio_drv_sel23: 2;
  uint32_t grpio_drv_sel24: 2;
  uint32_t grpio_drv_sel25: 2;
  uint32_t grpio_drv_sel26: 2;
  uint32_t grpio_drv_sel27: 2;
  uint32_t grpio_drv_sel28: 2;
  uint32_t grpio_drv_sel29: 2;
  uint32_t grpio_drv_sel30: 2;
  uint32_t grpio_drv_sel31: 2;
} soc_gpio_drv1_t, *soc_gpio_drv1_ptr_t;

/* Set UART driving strength
 * Valid setting 3, 2, 1, 0. Default is 3 which is the strongest driving strength
 */
void gpio_drv_change(uint8_t drv_sel)
{
  soc_gpio_drv1_ptr_t gpio_drv_ctrl = (soc_gpio_drv1_ptr_t)&SYSCTRL->GPIO_DRV_CTRL[1];

  gpio_drv_ctrl->grpio_drv_sel16 = drv_sel;
  gpio_drv_ctrl->grpio_drv_sel17 = drv_sel;
  gpio_drv_ctrl->grpio_drv_sel28 = drv_sel;
  gpio_drv_ctrl->grpio_drv_sel29 = drv_sel;
}

void init_default_pin_mux(void)
{
#if (PRINTF_ENABLE == 1)
  pin_set_mode(16, MODE_UART);     /*GPIO16 as UART0 RX*/
  pin_set_mode(17, MODE_UART);     /*GPIO17 as UART0 TX*/
#endif
  /* MCU P0 DEBUG */
  SYSCTRL->GPIO_MAP0 = 0x77777777;
  SYSCTRL->SYS_TEST.bit.DBG_OUT_SEL = 25;

  pin_set_mode(20, MODE_GPIO);
  gpio_cfg_output(20);
}
#endif

ZWSDK_WEAK void  zwsdk_app_hw_init(void)
{
  // This is a weak function
  // initializes any HW component that must be ready before the Z-Wave stack starts up.
}

static void clear_retentionram(void)
{
  uint8_t *start_addr =&__ret_sram_start__;
  uint8_t const *end_addr =&__ret_sram_end__;
  size_t size = end_addr - start_addr;
  memset(start_addr, 0, size);
}

int main(void)
{
  zpal_gpio_status_store(GPIO->INT_STATUS);
  zpal_radio_fw_preload();
  zpal_watchdog_init();

  zpal_reset_reason_t reset_reason = zpal_get_reset_reason();
  Lpm_Set_Low_Power_Level(LOW_POWER_LEVEL_NORMAL);
#if (CHIP_MODEL != CHIP_ID(RT584, RT58X_MPB)) && (CHIP_MODEL != CHIP_ID(RT584, RT58X_MPA))
  pin_set_mode(10, MODE_SWCLK_PIN10);
  pin_set_mode(11, MODE_SWDIO_PIN11);
#endif
#ifdef DEBUG
  init_default_pin_mux();
  gpio_drv_change(2);
#endif

zpal_increase_restarts();

  /*
  * Do not clear retention RAM when the reset reason is
  * ZPAL_RESET_REASON_DEEP_SLEEP_WUT,
  * ZPAL_RESET_REASON_DEEP_SLEEP_EXT_INT, or an internally triggered soft reset.
  */
  if (((ZPAL_RESET_REASON_DEEP_SLEEP_WUT != reset_reason) &&
       (ZPAL_RESET_REASON_DEEP_SLEEP_EXT_INT != reset_reason) &&
       (ZPAL_RESET_REASON_SOFTWARE != reset_reason)) ||
       ((ZPAL_RESET_REASON_SOFTWARE == reset_reason) && !zpal_was_internal_soft_reset()))
  {
    clear_retentionram();
  }

  Rtc_Enable();

#if RCO32K_ENABLE == 1
  // for 32K clock source
  Rtc_Set_Clk(0x00200000);
#elif RCO20K_ENABLE == 1
  // for 20K clock source
  Rtc_Set_Clk(0x000A0000);
#else
  // for 16K clock source
  Rtc_Set_Clk(0x00100000);
#endif

  mWakeUpTicks = get_rtc_time();
  mCompletedSleepDurationMs = get_rtc_duration(mWakeUpTicks);

  Rtc_Disable_Alarm();
  if (ZPAL_RESET_REASON_DEEP_SLEEP_WUT == reset_reason)
  {
    mIsWakeupCausedByRtccTimeout = true;
  }
  zwsdk_app_hw_init();
  zpal_bootloader_reset_page_counters();
  zpal_system_startup(reset_reason);

  // Initialize MP sector values
  MpSectorInit();

  // Load (common) calibration values from tokens to registers
  tr_mfg_tokens_process();

  vTaskStartScheduler();
  while(true);
  return 0;
}

void vApplicationIdleHook(void)
{
  zpal_feed_watchdog();
}

#if 0
void vApplicationMallocFailedHook(size_t xWantedSize)
{
  /* vApplicationMallocFailedHook() will only be called if
  configUSE_MALLOC_FAILED_HOOK is set to 1 in FreeRTOSConfig.h.  It is a hook
  function that will get called if a call to pvPortMalloc() fails.
  pvPortMalloc() is called internally by the kernel whenever a task, queue,
  timer or semaphore is created.  It is also called by various parts of the
  demo application.  If heap_1.c or heap_2.c are used, then the size of the
  heap available to pvPortMalloc() is defined by configTOTAL_HEAP_SIZE in
  FreeRTOSConfig.h, and the xPortGetFreeHeapSize() API function can be used
  to query the size of free heap space that remains (although it does not
  provide information on how the remaining heap might be fragmented). */
  taskDISABLE_INTERRUPTS();
  RM_ASSERT();
}
#endif

void vApplicationStackOverflowHook(TaskHandle_t pxTask, char *pcTaskName)
{
  (void)pxTask;
  (void)pcTaskName;
  /* This function will get called if a task overflows its stack.   If the
   parameters are corrupt then inspect pxCurrentTCB to find which was the
   offending task. */
  RM_ASSERT();
}

// Provide memory for FreeRtos idle task stack and task information
static StaticTask_t xIdleTaskTCB;
static StackType_t uxIdleTaskStack[CONFIG_IDLE_TASK_STACK_SIZE] __attribute__((aligned(4)));

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
static StackType_t uxTimerTaskStack[/*1024*/configTIMER_TASK_STACK_DEPTH] __attribute__((aligned(4)));

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

// Runtime stubs

int _close_r(__attribute__((unused)) int file)
{
  return -1;
}

int _fstat_r(__attribute__((unused)) int file, struct stat *st)
{
  st->st_mode = S_IFCHR;
  return 0;
}

int _getpid_r(void)
{
  return 1;
}

int _isatty_r(__attribute__((unused)) int file)
{
  return 1;
}

extern int errno;

#define EINVAL		22	/* Invalid argument */

int _kill_r(__attribute__((unused)) int pid, __attribute__((unused)) int sig)
{
  errno = EINVAL;
  return -1;
}

int _lseek_r(__attribute__((unused)) int file, __attribute__((unused)) int ptr, __attribute__((unused)) int dir)
{
  return 0;
}

int _read_r(__attribute__((unused)) int file, __attribute__((unused)) const char * const ptr, __attribute__((unused)) int len)
{
  return  0;                            /* EOF */
}

#define	EBADF 9		/* Bad file number */

int _write_r(__attribute__((unused)) int file, __attribute__((unused)) const char * const ptr, __attribute__((unused)) int len)
{
  errno = EBADF;
  return -1;
}
