/**
 ******************************************************************************
 * @file     cm33.h
 * @author
 * @brief   cm33 header file
 ******************************************************************************
 * @attention
 * Copyright (c) 2024 Rafael Micro.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without restriction,
 * including without limitation the rights to use, copy, modify, merge,
 * publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE
 * AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * This file is part of library_name.
 */

#ifndef _RT584_CM33_H_
#define _RT584_CM33_H_
/**
  \mainpage Introduction
  *
    *
  * <b>  </b>
  *
  * This document describes the usage of RT584Z peripheral driver
  *
  * <b> </b>
  *
  *
  * <b>Copyright Notice</b>
  *
  * Copyright (C) 2019-2024 Rafael Mirco Technology Corp. All rights reserved.
  */

#ifdef __cplusplus
extern "C" {
#endif


#include <arm_cmse.h>
/******************************************************************************/
/*                Processor and Core Peripherals                              */
/******************************************************************************/
/**
  @addtogroup RT584Z_CM33 RT584Z Device CMSIS Definitions
  Configuration of the RT584_CM33 Processor and Core Peripherals
  @{
*/

/*
 * ==========================================================================
 * ---------- Interrupt Number Definition -----------------------------------
 * ==========================================================================
 */
typedef enum IRQn_Type
{
    /******  Cortex-CM33 Processor Exceptions Numbers *************************************************/
    NonMaskableInt_IRQn           = -14,    /*!<  2 Cortex-M33 Non Maskable Interrupt                */
    HardFault_IRQn                = -13,    /*!<  3 Cortex-M33 Hard Fault Interrupt                  */
    MemoryManagement_IRQn         = -12,    /*!<  4 Cortex-M33 Memory Management Interrupt           */
    BusFault_IRQn                 = -11,    /*!<  5 Cortex-M33 Bus Fault Interrupt                   */
    UsageFault_IRQn               = -10,    /*!<  6 Cortex-M33 Usage Fault Interrupt                 */
    SecureFault_IRQn              = -9,     /*!<  7 Cortex-M33 Secure Fault Interrupt                */
    SVCall_IRQn                   = -5,     /*!< 11 Cortex-M33 SV Call Interrupt                     */
    DebugMonitor_IRQn             = -4,     /*!< 12 Cortex-M33 Debug Monitor Interrupt               */
    PendSV_IRQn                   = -2,     /*!< 14 Cortex-M33 Pend SV Interrupt                     */
    SysTick_IRQn                  = -1,     /*!< 15 Cortex-M33 System Tick Interrupt                 */

    /******  RT584_CM33 Specific Interrupt Numbers *************************************************/
    Gpio_IRQn                     = 0,       /*!< GPIO combined Interrupt                          */

    Timer0_IRQn                   = 1,       /*!< TIMER 0 Interrupt                                */
    Timer1_IRQn                   = 2,       /*!< TIMER 1 Interrupt                                */
    Timer2_IRQn                   = 3,       /*!< TIMER 2 Interrupt                                */
    Timer32K0_IRQn                = 4,       /*!< TIMER 32K0 Interrupt                                */
    Timer32K1_IRQn                = 5,       /*!< TIMER 32K1 Interrupt                                */
    Wdt_IRQn                      = 6,       /*!< WatchDog Interrupt                               */
    Rtc_IRQn                      = 7,       /*!< RTC Interrupt                                    */

    Soft0_IRQn                    = 9,       /*!< SOFTWARE0  Interrupt                             */
    Soft1_IRQn                    = 10,      /*!< SOFTWARE1  Interrupt                             */

    Dma_Ch0_IRQn                  = 12,      /*!< DMA Channel 0 Interrupt                          */
    Dma_Ch1_IRQn                  = 13,      /*!< DMA Channel 1 Interrupt                          */

    Uart0_IRQn                    = 16,      /*!< UART 0 Interrupt                                 */
    Uart1_IRQn                    = 17,      /*!< UART 1 Interrupt                                 */
    Uart2_IRQn                    = 18,      /*!< UART 2 Interrupt                                 */

    Irm_IRQn                      = 20,      /*!< IRM Interrupt                                    */

    I2C_Master0_IRQn              = 21,      /*!< I2C Master0 Interrupt                            */
    I2C_Master1_IRQn              = 22,      /*!< I2C Master1 Interrupt                            */
    I2C_Slave_IRQn                = 23,      /*!< I2C Slave Interrupt                              */

    Qspi0_IRQn                    = 25,      /*!< QSPI0 Interrupt                                  */
    Qspi1_IRQn                    = 26,      /*!< QSPI1 Interrupt                                  */

    I2s0_IRQn                     = 29,      /*!< I2S0  Interrupt                                  */

    Pwm0_IRQn                     = 32,      /*!< PWM0 Interrupt                                   */
    Pwm1_IRQn                     = 33,      /*!< PWM1 Interrupt                                   */
    Pwm2_IRQn                     = 34,      /*!< PWM2 Interrupt                                   */
    Pwm3_IRQn                     = 35,      /*!< PWM3 Interrupt                                   */
    Pwm4_IRQn                     = 36,      /*!< PWM4 Interrupt                                   */

    FlashCtl_IRQn                 = 39,      /*!< FlashCtl Interrupt                               */
    OTP_IRQn                      = 40,      /*!< OTP Interrupt                                    */
    Crypto_IRQn                   = 41,      /*!< Crypto  Interrupt                                */
    Bod_Comp_IRQn                 = 42,      /*!< BOD COMPARATOR  Interrupt                        */

    CCM_AES_IQRn                  = 43,      /*!< AES CCM Interrupt                                */
    Sec_Ctrl_IQRn                 = 44,      /*!< SECURE CTRL Interrupt                            */

    CommSubsystem_IRQn            = 45,      /*!< COMM_SUBSYSTEM_COMM Interrupt                    */

    Sadc_IRQn                     = 46,      /*!< SADC Interrupt                                   */
    Aux_Comp_IRQn                 = 47,      /*!< AUX COMPARATOR Interrupt                         */

} IRQn_Type;

/*
 * ==========================================================================
 * ----------- Processor and Core Peripheral Section ------------------------
 * ==========================================================================
 */

/* -------  Start of section using anonymous unions and disabling warnings  ------- */
#if   defined (__CC_ARM)
#pragma push
#pragma anon_unions
#elif defined (__ICCARM__)
#pragma language=extended
#elif defined(__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wc11-extensions"
#pragma clang diagnostic ignored "-Wreserved-id-macro"
#elif defined (__GNUC__)
/* anonymous unions are enabled by default */
#elif defined (__TMS470__)
/* anonymous unions are enabled by default */
#elif defined (__TASKING__)
#pragma warning 586
#elif defined (__CSMC__)
/* anonymous unions are enabled by default */
#else
#warning Not supported compiler type
#endif


#define __CM33_REV                0x0001U        /**< Core revision r0p1 */
#define __MPU_PRESENT                  1U        /**< Defines if an MPU is present or not */
#define __NVIC_PRIO_BITS               3U        /**< Number of priority bits implemented in the NVIC */
#define __Vendor_SysTickConfig         0U        /**< Vendor specific implementation of SysTickConfig is defined */
#define __FPU_PRESENT                  1U        /**< Defines if an FPU is present or not */
#define __DSP_PRESENT                  1U        /**< Defines if Armv8-M Mainline core supports DSP instructions */
#define __SAUREGION_PRESENT            1U        /**< Defines if an SAU is present or not */
#define __VTOR_PRESENT                 1U        /**< VTOR present */

#include "core_cm33.h"                 /* Core Peripheral Access Layer */
#include "system_cm33.h"

/**@}*/ /* end of group RT584_CMSIS */



#include "status.h"
#include "sysfun.h"
#include "assert_help.h"
#include "debug_log.h"
/************************************************************************/
/*                Peripheral Register Header                            */
/************************************************************************/
#include "reg_base.h"
#include "security_enable_config.h"
#include "rco1m_reg.h"
#include "rco32k_reg.h"
#include "pufs_rt_regs.h"
#include "aux_comp_reg.h"
#include "bod_comp_reg.h"
#include "crypto_reg.h"
#include "dma_reg.h"
#include "dpd_reg.h"
#include "flashctl_reg.h"
#include "gpio_reg.h"
#include "i2c_master_reg.h"
#include "i2c_slave_reg.h"
#include "i2s_reg.h"
#include "irm_reg.h"
#include "pmu_reg.h"
#include "pwm_reg.h"
#include "qspi_reg.h"
#include "rtc_reg.h"
#include "sadc_reg.h"
#include "sec_ctrl_reg.h"
#include "swi_reg.h"
#include "sysctrl_reg.h"
#include "timer_reg.h"
#include "trng_reg.h"
#include "uart_reg.h"
#include "wdt_reg.h"
#include "xdma_reg.h"
#include "comm_subsystem_ahb_reg.h"
#include "pufs_rt_regs.h"


/* --------------------  End of section using anonymous unions  ------------------- */
#if   defined (__CC_ARM)
#pragma pop
#elif defined (__ICCARM__)
/* leave anonymous unions enabled */
#elif (__ARMCC_VERSION >= 6010050)
#pragma clang diagnostic pop
#elif defined (__GNUC__)
/* anonymous unions are enabled by default */
#elif defined (__TMS470__)
/* anonymous unions are enabled by default */
#elif defined (__TASKING__)
#pragma warning restore
#elif defined (__CSMC__)
/* anonymous unions are enabled by default */
#else
#warning Not supported compiler type
#endif

/******************************************************************************/
/*                         Peripheral declaration                             */
/******************************************************************************/
/** @addtogroup PERIPHERAL_BASE RT584Z Peripheral Base Address Declaration
  The RT584z Peripheral Point Declaration
  @{
 */
#if (SYSCTRL_SECURE_EN == 1)
#define SYSCTRL             ((SYSCTRL_T *) (SYSCTRL_BASE | PERIPH_SECURE_OFFSET))    /*!< Pointer to System control security register structure*/
#else
#define SYSCTRL             ((SYSCTRL_T *) SYSCTRL_BASE)                             /*!< Pointer to System control non-security register structure*/
#endif

#if (GPIO_SECURE_EN == 1)
#define GPIO                  ((GPIO_T *) (GPIO_BASE | PERIPH_SECURE_OFFSET))        /*!< Pointer to GPIO security register structure*/
#else
#define GPIO                  ((GPIO_T *) GPIO_BASE)                                 /*!< Pointer to GPIO non-security register structure*/
#endif

#if (RTC_SECURE_EN == 1)
#define RTC                   ((RTC_T *) (RTC_BASE | PERIPH_SECURE_OFFSET))          /*!< Pointer to RTC control security register structure*/
#else
#define RTC                   ((RTC_T *) RTC_BASE)                                   /*!< Pointer to rtc non-security register structure*/
#endif

#if (DPD_SECURE_EN == 1)
#define DPD_CTRL            ((DPD_T *) (DPD_BASE | PERIPH_SECURE_OFFSET))             /*!< Pointer to dpd security register structure*/
#else
#define DPD_CTRL            ((DPD_T *) DPD_BASE)                                       /*!< Pointer to dpd non-security register structure*/
#endif

#if (SOC_PMU_SECURE_EN == 1)
#define PMU_CTRL            ((PMU_T *) (SOC_PMU_BASE | PERIPH_SECURE_OFFSET))         /*!< Pointer to pmu security register structure*/
#else
#define PMU_CTRL            ((PMU_T *) SOC_PMU_BASE)                                  /*!< Pointer to pwm non-security register structure*/
#endif

#if (FLASHCTRL_SECURE_EN == 1)
#define FLASH                           ((FLASHCTL_T *) (FLASHCTRL_BASE | PERIPH_SECURE_OFFSET))     /*!< Pointer to flash contrl security register structure*/
#else
#define FLASH            ((FLASHCTL_T *) (FLASHCTRL_BASE))                            /*!< Pointer to flash contrl non-security register structure*/
#endif

#if (TIMER0_SECURE_EN == 1)
#define TIMER0            ((TIMERN_T *) (TIMER0_BASE | PERIPH_SECURE_OFFSET))         /*!< Pointer to timer 0 security register structure*/
#else
#define TIMER0            ((TIMERN_T *) TIMER0_BASE)                                  /*!< Pointer to timer 0 non-security register structure*/
#endif

#if (TIMER1_SECURE_EN == 1)
#define TIMER1            ((TIMERN_T *) (TIMER1_BASE | PERIPH_SECURE_OFFSET))         /*!< Pointer to timer 1 security register structure*/
#else
#define TIMER1            ((TIMERN_T *) TIMER1_BASE)                                  /*!< Pointer to timer 1 non-security register structure*/
#endif

#if (TIMER2_SECURE_EN == 1)
#define TIMER2            ((TIMERN_T *) (TIMER2_BASE | PERIPH_SECURE_OFFSET))          /*!< Pointer to timer 2 security register structure*/
#else
#define TIMER2            ((TIMERN_T *) TIMER2_BASE)                                /*!< Pointer to timer 2 non-security register structure*/
#endif

#if (TIMER32K0_SECURE_EN == 1)
#define TIMER32K0            ((TIMER32KN_T *) (TIMER32K0_BASE | PERIPH_SECURE_OFFSET)) /*!< Pointer to timer32k 0 security register structure*/
#else
#define TIMER32K0            ((TIMER32KN_T *) TIMER32K0_BASE)                           /*!< Pointer to timer32k 0 non-security register structure*/
#endif

#if (TIMER32K1_SECURE_EN == 1)
#define TIMER32K1            ((TIMER32KN_T *) (TIMER32K1_BASE | PERIPH_SECURE_OFFSET)) /*!< Pointer to timer32k 1 security register structure*/
#else
#define TIMER32K1            ((TIMER32KN_T *) TIMER32K1_BASE)                           /*!< Pointer to timer32k 1 non-security register structure*/
#endif

#if (WDT_SECURE_EN == 1)
#define WDT                   ((WDT_T *) (WDT_BASE | PERIPH_SECURE_OFFSET))            /*!< Pointer to watch dog timer security register structure*/
#else
#define WDT                   ((WDT_T *) WDT_BASE)                                       /*!< Pointer to watch dog timer non-security register structure*/
#endif

#if (UART0_SECURE_EN == 1)
#define UART0                 ((UART_T *) (UART0_BASE | PERIPH_SECURE_OFFSET))         /*!< Pointer to uart 0 security register structure*/
#else
#define UART0                 ((UART_T *) UART0_BASE)                                   /*!< Pointer to uart 0 non-security register structure*/
#endif

#if (UART1_SECURE_EN == 1)
#define UART1                 ((UART_T *) (UART1_BASE | PERIPH_SECURE_OFFSET))          /*!< Pointer to uart 1 security register structure*/
#else
#define UART1                 ((UART_T *) UART1_BASE)                                   /*!< Pointer to uart 1 non-security register structure*/
#endif

#if (UART2_SECURE_EN == 1)
#define UART2                 ((UART_T *) (UART2_BASE | PERIPH_SECURE_OFFSET))           /*!< Pointer to uart 2 security register structure*/
#else
#define UART2                 ((UART_T *) UART2_BASE)                                  /*!< Pointer to uart 2 non-security register structure*/
#endif

#if (PWM_SECURE_EN == 1)
#define PWM0                ((PWM_T *) (PWM0_BASE | PERIPH_SECURE_OFFSET))              /*!< Pointer to pwm 0 security register structure*/
#define PWM1                ((PWM_T *) (PWM1_BASE | PERIPH_SECURE_OFFSET))              /*!< Pointer to pwm 1 security register structure*/
#define PWM2                ((PWM_T *) (PWM2_BASE | PERIPH_SECURE_OFFSET))              /*!< Pointer to pwm 2 security register structure*/
#define PWM3                ((PWM_T *) (PWM3_BASE | PERIPH_SECURE_OFFSET))              /*!< Pointer to pwm 3 security register structure*/
#define PWM4                ((PWM_T *) (PWM4_BASE | PERIPH_SECURE_OFFSET))              /*!< Pointer to pwm 4 security register structure*/
#else
#define PWM0                ((PWM_T *) PWM0_BASE)                                       /*!< Pointer to pwm 0 non-security register structure*/
#define PWM1                ((PWM_T *) PWM1_BASE)                                       /*!< Pointer to pwm 1 non-security register structure*/
#define PWM2                ((PWM_T *) PWM2_BASE)                                       /*!< Pointer to pwm 2 non-security register structure*/
#define PWM3                ((PWM_T *) PWM3_BASE)                                       /*!< Pointer to pwm 3 non-security register structure*/
#define PWM4                ((PWM_T *) PWM4_BASE)                                       /*!< Pointer to pwm 4 non-security register structure*/
#endif

#if (I2C_MASTER0_SECURE_EN == 1)
#define I2C_MASTER0           ((I2C_MASTER_T *) (I2C_MASTER0_BASE | PERIPH_SECURE_OFFSET))  /*!< Pointer to i2c master 0 security register structure*/
#else
#define I2C_MASTER0           ((I2C_MASTER_T *) I2C_MASTER0_BASE)                          /*!< Pointer to i2c master 0 non-security register structure*/
#endif

#if (I2C_MASTER1_SECURE_EN == 1)
#define I2C_MASTER1           ((I2C_MASTER_T *) (I2C_MASTER1_BASE | PERIPH_SECURE_OFFSET))  /*!< Pointer to i2c master 1 security register structure*/
#else
#define I2C_MASTER1           ((I2C_MASTER_T *) I2C_MASTER1_BASE)                           /*!< Pointer to i2c master 1 non-security register structure*/
#endif

#if (I2C_SLAVE_SECURE_EN == 1)
#define I2C_SLAVE             ((I2C_SLAVE_T *) (I2C_SLAVE_BASE | PERIPH_SECURE_OFFSET))     /*!< Pointer to i2c slave security register structure*/
#else
#define I2C_SLAVE             ((I2C_SLAVE_T *) I2C_SLAVE_BASE)                              /*!< Pointer to i2c slave non-security register structure*/
#endif

#if (COMM_SUBSYSTEM_AHB_SECURE_EN == 1)
#define COMM_SUBSYSTEM_AHB             ((COMM_SUBSYSTEM_AHB_T *) (COMM_SUBSYSTEM_AHB_BASE | PERIPH_SECURE_OFFSET))    /*!< Pointer to sub system security register structure*/
#else
#define COMM_SUBSYSTEM_AHB             ((COMM_SUBSYSTEM_AHB_T *) COMM_SUBSYSTEM_AHB_BASE)                             /*!< Pointer to sub system non-security register structure*/
#endif

#if (RCO32K_CAL_SECURE_EN == 1)
#define RCO32K_CAL            ((RCO32K_CAL_T *) (RCO32K_BASE | PERIPH_SECURE_OFFSET))          /*!< Pointer to rco32k  security register structure*/
#else
#define RCO32K_CAL            ((RCO32K_CAL_T *) RCO32K_BASE)                                  /*!< Pointer to rco32k  non-security register structure*/
#endif


#if (AUX_COMP_SECURE_EN == 1)
#define AUX_COMP            ((AUX_COMP_CTL_T *) (AUX_COMP_BASE | PERIPH_SECURE_OFFSET))        /*!< Pointer to aux comparator security register structure*/
#else
#define AUX_COMP            ((AUX_COMP_CTL_T *) AUX_COMP_BASE)                                 /*!< Pointer to aux comparator non-security register structure*/
#endif

#if (BOD_COMP_SECURE_EN == 1)
#define BOD_COMP            ((BOD_COMP_CTL_T *) (BOD_COMP_BASE | PERIPH_SECURE_OFFSET))        /*!< Pointer to bod comparator security register structure*/
#else
#define BOD_COMP            ((BOD_COMP_CTL_T *) BOD_COMP_BASE)                                 /*!< Pointer to bod comparator non-security register structure*/
#endif

#if (RCO1M_CAL_SECURE_EN == 1)
#define RCO1M_CAL            ((RCO1M_CAL_T *) (RCO1M_BASE | PERIPH_SECURE_OFFSET))             /*!< Pointer to rco1m security register structure*/
#else
#define RCO1M_CAL            ((RCO1M_CAL_T *) RCO1M_BASE)                                      /*!< Pointer to rco1m non-security register structure*/
#endif

#if (QSPI0_SECURE_EN == 1)
#define QSPI0                 ((QSPI_T *) (QSPI0_BASE | PERIPH_SECURE_OFFSET))                 /*!< Pointer to qspi 0 security register structure*/
#else
#define QSPI0                 ((QSPI_T *) QSPI0_BASE)                                          /*!< Pointer to qspi 0  non-security register structure*/
#endif

#if (QSPI1_SECURE_EN == 1)
#define QSPI1                 ((QSPI_T *) (QSPI1_BASE | PERIPH_SECURE_OFFSET))                 /*!< Pointer to  qspi 1  security register structure*/
#else
#define QSPI1                 ((QSPI_T *) QSPI1_BASE)                                          /*!< Pointer to  qspi 1   non-security register structure*/
#endif

#if (IRM_SECURE_EN == 1)
#define IRM                   ((IRM_T *) (IRM_BASE | PERIPH_SECURE_OFFSET))                    /*!< Pointer to irm security register structure*/
#else
#define IRM                   ((IRM_T *) IRM_BASE)                                             /*!< Pointer to irm non-security register structure*/
#endif

#if (XDMA_SECURE_EN == 1)
#define XDMA                  ((XDMA_T *) (XDMA_BASE | PERIPH_SECURE_OFFSET))                  /*!< Pointer to xdma security register structure*/
#else
#define XDMA                  ((XDMA_T *) XDMA_BASE)                                           /*!< Pointer to xdma non-security register structure*/
#endif

#if (DMA0_SECURE_EN == 1)
#define DMA0                  ((DMA_T *) (DMA0_BASE | PERIPH_SECURE_OFFSET))                   /*!< Pointer to dma 0 security register structure*/
#else
#define DMA0                  ((DMA_T *) DMA0_BASE)                                            /*!< Pointer to dma 0 non-security register structure*/
#endif

#if (DMA1_SECURE_EN == 1)
#define DMA1                  ((DMA_T *) (DMA1_BASE | PERIPH_SECURE_OFFSET))                   /*!< Pointer to dma 1 security register structure*/
#else
#define DMA1                  ((DMA_T *) DMA1_BASE)                                            /*!< Pointer to dma 1 non-security register structure*/
#endif

#if (I2S0_SECURE_EN == 1)
#define I2S_MASTER            ((I2S_T *) (I2S0_BASE | PERIPH_SECURE_OFFSET))                   /*!< Pointer to i2s master security register structure*/
#else
#define I2S_MASTER            ((I2S_T *) I2S0_BASE)                                            /*!< Pointer to i2s master non-security register structure*/
#endif

#if (SADC_SECURE_EN == 1)
#define SADC                  ((SADC_T *) (SADC_BASE | PERIPH_SECURE_OFFSET))                  /*!< Pointer to sadc security register structure*/
#else
#define SADC                  ((SADC_T *) SADC_BASE)                                           /*!< Pointer to sadc non-security register structure*/
#endif

#if (SW_IRQ0_SECURE_EN == 1)
#define SW_INT0               ((SWI_T *) (SW_IRQ0_BASE | PERIPH_SECURE_OFFSET))                /*!< Pointer tosoftware interrupt 0 security register structure*/
#else
#define SW_INT0               ((SWI_T *) SW_IRQ0_BASE)                                         /*!< Pointer tosoftware interrupt 0 non-security register structure*/
#endif

#if (SW_IRQ1_SECURE_EN == 1)
#define SW_INT1               ((SWI_T *) (SW_IRQ1_BASE | PERIPH_SECURE_OFFSET))                /*!< Pointer to software interrupt 1 security register structure*/
#else
#define SW_INT1               ((SWI_T *) SW_IRQ1_BASE)                                         /*!< Pointer to software interrupt 1 non-security register structure*/
#endif


#define CRYPTO               ((CRYPTO_T *)(CRYPTO_BASE))                                       /*!< Pointer to crypto register structure*/

#if (PUF_OTP_SECURE_EN == 1)
#define TRNG_CTRL           ((TRNG_T *) (TRNG_BASE | PERIPH_SECURE_OFFSET))                    /*!< Pointer to trng security register structure*/
#else
#define TRNG_CTRL           ((TRNG_T *) TRNG_BASE)                                             /*!< Pointer to trng control non-security register structure*/
#endif

/*IDAU Secure controller is only used in secure mode only*/
#define SEC_CTRL            ((SEC_CTRL_T *)(SEC_CTRL_BASE))                                    /*!< Pointer to secure  register structure*/

/**@}*/ /* end of group RT584z_PERIPHERAL_DECLARATION */


/******************************************************************************/
/*      Cortex-M33 INTS Setting Define                                        */
/******************************************************************************/
/** @addtogroup RT584Z_INTS RT584Z Ints Setting Define
  RT584z Ints Setting define constans
  @{
*/

#define  GPIO_INT_NONSEC              BIT0      /*!<  (1UL << 0)    */
#define  TIMER0_INT_NONSEC            BIT1      /*!<  (1UL << 1)    */
#define  TIMER1_INT_NONSEC            BIT2      /*!<  (1UL << 2)    */
#define  TIMER2_INT_NONSEC            BIT3      /*!<  (1UL << 3)    */
#define  TIMER32K0_INT_NONSEC         BIT4      /*!<  (1UL << 4)    */
#define  TIMER32K1_INT_NONSEC         BIT5      /*!<  (1UL << 5)    */
#define  WATCHDOG_INT_NONSEC          BIT6      /*!<  (1UL << 6)    */
#define  RTC_INT_NONSEC               BIT7      /*!<  (1UL << 7)    */
#define  SOFT0_INT_NONSEC             BIT9      /*!<  (1UL << 9)    */
#define  SOFT1_INT_NONSEC             BIT10     /*!<  (1UL << 10)   */
#define  DMA_CH0_INT_NONSEC           BIT12     /*!<  (1UL << 12)   */
#define  DMA_CH1_INT_NONSEC           BIT13     /*!<  (1UL << 13)   */
#define  UART0_INT_NONSEC             BIT16     /*!<  (1UL << 16)   */
#define  UART1_INT_NONSEC             BIT17     /*!<  (1UL << 17)   */
#define  UART2_INT_NONSEC             BIT18     /*!<  (1UL << 18)   */
#define  IRM_INT_NONSEC               BIT20     /*!<  (1UL << 20)   */
#define  I2C_MASTER0_INT_NONSEC       BIT21     /*!<  (1UL << 21)   */
#define  I2C_MASTER1_INT_NONSEC       BIT22     /*!<  (1UL << 22)   */
#define  I2C_SLAVE_INT_NONSEC         BIT23     /*!<  (1UL << 23)   */
#define  QSPI0_INT_NONSEC             BIT25     /*!<  (1UL << 25)   */
#define  QSPI1_INT_NONSEC             BIT26     /*!<  (1UL << 26)   */
#define  I2S0_INT_NONSEC              BIT29     /*!<  (1UL << 29)   */

/*ITNS[1]*/
#define  PWM0_INT_NONSEC              BIT0          /*!< (1UL << 0)   */
#define  PWM1_INT_NONSEC              BIT1          /*!< (1UL << 1)   */
#define  PWM2_INT_NONSEC              BIT2          /*!< (1UL << 2)   */
#define  PWM3_INT_NONSEC              BIT3          /*!< (1UL << 3)   */
#define  PWM4_INT_NONSEC              BIT4          /*!< (1UL << 4)   */
#define  FLASH_INT_NONSEC             BIT7          /*!< (1UL << 7)   */
#define  PUF_TRNG_OTP_INT_NONSEC      BIT8          /*!< (1UL << 8)   */
#define  CRYPTO_INT_NONSEC            BIT9          /*!< (1UL << 9)   */
#define  BOD_COMP_INT_NONSEC          BIT10         /*!< (1UL << 10)  */
#define  SEC_INT_NONSEC               BIT12         /*!< (1UL << 12)  */
#define  CommSubsys_INT_NONSEC        BIT13         /*!< (1UL << 13)  */
#define  SADC_INT_NONSEC              BIT14         /*!< (1UL << 14)  */
#define  AUX_COMP_INT_NONSEC          BIT16         /*!< (1UL << 16)  */

/**@}*/ /* end of group RT584Z_INTS_SETTING_DFINE */


/******************************************************************************/
/*                I/O Routines                                            */
/******************************************************************************/
/** @addtogroup IO_ROUTINES RT584Z I/O Routines
  The Declaration of RT584z I/O routines
  @{
 */
/**
  * @brief Set a 32-bit unsigned value to specified I/O port
  * @param[in] port Port address to set 32-bit data
  * @param[in] value Value to write to I/O port
  * @return  None
  * @note The output port must be 32-bit aligned
  */
#define outp32(port,value)    *((volatile uint32_t *)(port)) = (value)

/**
  * @brief Get a 32-bit unsigned value from specified I/O port
  * @param[in] port Port address to get 32-bit data from
  * @return  32-bit unsigned value stored in specified I/O port
  * @note The input port must be 32-bit aligned
  */
#define inp32(port)           (*((volatile uint32_t *)(port)))

/**
  * @brief Set a 16-bit unsigned value to specified I/O port
  * @param[in] port Port address to set 16-bit data
  * @param[in] value Value to write to I/O port
  * @return  None
  * @note The output port must be 16-bit aligned
  */
#define outp16(port,value)    *((volatile uint16_t *)(port)) = (value)

/**
  * @brief Get a 16-bit unsigned value from specified I/O port
  * @param[in] port Port address to get 16-bit data from
  * @return  16-bit unsigned value stored in specified I/O port
  * @note The input port must be 16-bit aligned
  */
#define inp16(port)           (*((volatile uint16_t *)(port)))

/**
  * @brief Set a 8-bit unsigned value to specified I/O port
  * @param[in] port Port address to set 8-bit data
  * @param[in] value Value to write to I/O port
  * @return  None
  */
#define outp8(port,value)     *((volatile uint8_t *)(port)) = (value)

/**
  * @brief Get a 8-bit unsigned value from specified I/O port
  * @param[in] port Port address to get 8-bit data from
  * @return  8-bit unsigned value stored in specified I/O port
  */
#define inp8(port)            (*((volatile uint8_t *)(port)))


/**@}*/ /* end of group IO_ROUTINE */

/******************************************************************************/
/*                Legacy Constants                                            */
/******************************************************************************/
/** @addtogroup Legacy_Constants RT584Z Legacy Constants
  Legacy Constants
  @{
*/

#ifndef NULL
#define NULL           (0)        ///< NULL pointer
#endif

#define TRUE           (1UL)      ///< Boolean true, define to use in API parameters or return value
#define FALSE          (0UL)      ///< Boolean false, define to use in API parameters or return value

#define ENABLE         (1UL)      ///< Enable, define to use in API parameters
#define DISABLE        (0UL)      ///< Disable, define to use in API parameters

/* Define one bit mask */
#define BIT0     (0x00000001UL)       ///< Bit 0 mask of an 32 bit integer
#define BIT1     (0x00000002UL)       ///< Bit 1 mask of an 32 bit integer
#define BIT2     (0x00000004UL)       ///< Bit 2 mask of an 32 bit integer
#define BIT3     (0x00000008UL)       ///< Bit 3 mask of an 32 bit integer
#define BIT4     (0x00000010UL)       ///< Bit 4 mask of an 32 bit integer
#define BIT5     (0x00000020UL)       ///< Bit 5 mask of an 32 bit integer
#define BIT6     (0x00000040UL)       ///< Bit 6 mask of an 32 bit integer
#define BIT7     (0x00000080UL)       ///< Bit 7 mask of an 32 bit integer
#define BIT8     (0x00000100UL)       ///< Bit 8 mask of an 32 bit integer
#define BIT9     (0x00000200UL)       ///< Bit 9 mask of an 32 bit integer
#define BIT10    (0x00000400UL)       ///< Bit 10 mask of an 32 bit integer
#define BIT11    (0x00000800UL)       ///< Bit 11 mask of an 32 bit integer
#define BIT12    (0x00001000UL)       ///< Bit 12 mask of an 32 bit integer
#define BIT13    (0x00002000UL)       ///< Bit 13 mask of an 32 bit integer
#define BIT14    (0x00004000UL)       ///< Bit 14 mask of an 32 bit integer
#define BIT15    (0x00008000UL)       ///< Bit 15 mask of an 32 bit integer
#define BIT16    (0x00010000UL)       ///< Bit 16 mask of an 32 bit integer
#define BIT17    (0x00020000UL)       ///< Bit 17 mask of an 32 bit integer
#define BIT18    (0x00040000UL)       ///< Bit 18 mask of an 32 bit integer
#define BIT19    (0x00080000UL)       ///< Bit 19 mask of an 32 bit integer
#define BIT20    (0x00100000UL)       ///< Bit 20 mask of an 32 bit integer
#define BIT21    (0x00200000UL)       ///< Bit 21 mask of an 32 bit integer
#define BIT22    (0x00400000UL)       ///< Bit 22 mask of an 32 bit integer
#define BIT23    (0x00800000UL)       ///< Bit 23 mask of an 32 bit integer
#define BIT24    (0x01000000UL)       ///< Bit 24 mask of an 32 bit integer
#define BIT25    (0x02000000UL)       ///< Bit 25 mask of an 32 bit integer
#define BIT26    (0x04000000UL)       ///< Bit 26 mask of an 32 bit integer
#define BIT27    (0x08000000UL)       ///< Bit 27 mask of an 32 bit integer
#define BIT28    (0x10000000UL)       ///< Bit 28 mask of an 32 bit integer
#define BIT29    (0x20000000UL)       ///< Bit 29 mask of an 32 bit integer
#define BIT30    (0x40000000UL)       ///< Bit 30 mask of an 32 bit integer
#define BIT31    (0x80000000UL)       ///< Bit 31 mask of an 32 bit integer

/* Byte Mask Definitions */
#define BYTE0_Msk              (0x000000FFUL)         ///< Mask to get bit0~bit7 from a 32 bit integer
#define BYTE1_Msk              (0x0000FF00UL)         ///< Mask to get bit8~bit15 from a 32 bit integer
#define BYTE2_Msk              (0x00FF0000UL)         ///< Mask to get bit16~bit23 from a 32 bit integer
#define BYTE3_Msk              (0xFF000000UL)         ///< Mask to get bit24~bit31 from a 32 bit integer

#define GET_BYTE0(u32Param)    (((u32Param) & BYTE0_Msk)      )  /*!< Extract Byte 0 (Bit  0~ 7) from parameter u32Param */
#define GET_BYTE1(u32Param)    (((u32Param) & BYTE1_Msk) >>  8)  /*!< Extract Byte 1 (Bit  8~15) from parameter u32Param */
#define GET_BYTE2(u32Param)    (((u32Param) & BYTE2_Msk) >> 16)  /*!< Extract Byte 2 (Bit 16~23) from parameter u32Param */
#define GET_BYTE3(u32Param)    (((u32Param) & BYTE3_Msk) >> 24)  /*!< Extract Byte 3 (Bit 24~31) from parameter u32Param */


/**@}*/  /* end of group Legacy_Constants */


/*This is a software breakpoint, which will stop the firmware when using ICE debugging*/
#define BREAK()     do{ __asm volatile ("BKPT    #0"); while(1);} while(0)
#define RM_ASSERT()    do{ while(1);} while(0)




#ifdef __cplusplus
}
#endif

#endif      /* end of _RT584_CM33_H_ */
