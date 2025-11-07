/**
 ******************************************************************************
 * @file    reg_base.h
 * @author
 * @brief   register base address header file
 *
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

#ifndef ___REG_BASE_H__
#define ___REG_BASE_H__

#if defined ( __CC_ARM   )
#pragma anon_unions
#endif
/******************************************************************************/
/*                         Peripheral memory map                              */
/******************************************************************************/

/** @addtogroup PERIPHERAL_BASE RT584Z Peripheral Memory Base

  Memory Mapped Structure for Series Peripheral
  @{
 */

/*!<Peripheral and SRAM base address */
#define SRAM_BASE                       (0x20000000UL)  /*!< (SRAM      ) Base Address */
#define PERIPH_BASE                     (0x40000000UL)  /*!< (Peripheral) Base Address */
#define PERIPH_SECURE_OFFSET            (0x10000000UL)  /*!< (Secure Peripheral) Offset */

/*!<Peripheral memory map */
#define SYSCTRL_BASE                                        (PERIPH_BASE)                                  /*!<system control register base address*/
#define GPIO_BASE                       (PERIPH_BASE+0x1000UL)                         /*!<gpio register base address*/
#define SEC_CTRL_BASE                   (PERIPH_BASE+PERIPH_SECURE_OFFSET+0x3000UL)    /*!<security register base address*/
#define RTC_BASE                        (PERIPH_BASE+0x4000UL)                         /*!<rtc register base address*/
#define DPD_BASE                        (PERIPH_BASE+0x5000UL)                         /*!<dpd register base address*/
#define SOC_PMU_BASE                    (PERIPH_BASE+0x6000UL)                         /*!<pmu register base address*/
#define FLASHCTRL_BASE                  (PERIPH_BASE+0x9000UL)                         /*!<flash control  register base address*/
#define TIMER0_BASE                     (PERIPH_BASE+0xA000UL)                         /*!<timer 0 register base address*/
#define TIMER1_BASE                     (PERIPH_BASE+0xB000UL)                         /*!<timer 1 register base address*/
#define TIMER2_BASE                     (PERIPH_BASE+0xC000UL)                         /*!<timer 2 register base address*/
#define TIMER32K0_BASE                  (PERIPH_BASE+0xD000UL)                         /*!<timer32k0 register base address*/
#define TIMER32K1_BASE                  (PERIPH_BASE+0xE000UL)                         /*!<timer32k1 register base address*/
#define WDT_BASE                        (PERIPH_BASE+0x10000UL)                        /*!<wdt register base address*/
#define UART0_BASE                      (PERIPH_BASE+0x12000UL)                        /*!<uart 0 register base address*/
#define UART1_BASE                      (PERIPH_BASE+0x13000UL)                        /*!<uart 1 register base address*/
#define PWM0_BASE                       (PERIPH_BASE+0x26000UL)                        /*!<pwm 0 register base address*/
#define PWM1_BASE                       (PERIPH_BASE+0x26100UL)                        /*!<pwm 1 register base address*/
#define PWM2_BASE                       (PERIPH_BASE+0x26200UL)                        /*!<pwm 2 register base address*/
#define PWM3_BASE                       (PERIPH_BASE+0x26300UL)                        /*!<pwm 3 register base address*/
#define PWM4_BASE                       (PERIPH_BASE+0x26400UL)                        /*!<pwm 4 register base address*/
#define I2C_SLAVE_BASE                  (PERIPH_BASE+0x18000UL)                        /*!<i2s slave register base address*/
#define COMM_SUBSYSTEM_AHB_BASE         (PERIPH_BASE+0x1A000UL)                        /*!<communcation subsystem ahb register base address*/
#define RCO32K_BASE                     (PERIPH_BASE+0x1C000UL)                        /*!<rco32k register base address*/
#define BOD_COMP_BASE                   (PERIPH_BASE+0x1D000UL)                        /*!<bod comparator register base address*/
#define AUX_COMP_BASE                   (PERIPH_BASE+0x1E000UL)                        /*!<aux comparator register base address*/
#define RCO1M_BASE                      (PERIPH_BASE+0x1F000UL)                        /*!<rco1m register base address*/
#define QSPI0_BASE                      (PERIPH_BASE+0x20000UL)                        /*!<qspi 0 register base address*/
#define QSPI1_BASE                      (PERIPH_BASE+0x21000UL)                        /*!<qspi 1 register base address*/
#define TRNG_BASE                       (PERIPH_BASE+0x23000UL)                        /*!<trng register base address*/
#define IRM_BASE                        (PERIPH_BASE+0x24000UL)                        /*!<irm register base address*/
#define UART2_BASE                      (PERIPH_BASE+0x25000UL)                        /*!<uart 2 register base address*/
#define XDMA_BASE                       (PERIPH_BASE+0x28000UL)                        /*!<xdma register base address*/
#define DMA0_BASE                       (PERIPH_BASE+0x29000UL)                        /*!<dma 0 register base address*/
#define DMA1_BASE                       (PERIPH_BASE+0x2A000UL)                        /*!<dma 1 register base address*/
#define I2C_MASTER0_BASE                (PERIPH_BASE+0x2B000UL)                        /*!<i2c master 0 register base address*/
#define I2C_MASTER1_BASE                (PERIPH_BASE+0x2C000UL)                        /*!<i2c master 1 register base address*/
#define I2S0_BASE                       (PERIPH_BASE+0x2D000UL)                        /*!<i2s0 register base address*/
#define SADC_BASE                       (PERIPH_BASE+0x2F000UL)                        /*!<sadc register base address*/

#define SW_IRQ0_BASE                    (PERIPH_BASE+0x30000UL)                        /*!<software interrupt 0 register base address*/
#define SW_IRQ1_BASE                    (PERIPH_BASE+0x31000UL)                        /*!<software interrupt 1 register base address*/

#define CRYPTO_BASE                     (0x70000000UL)                                 /*!crytpot register base address*/


/**@}*/ /* end of Peripheral Memory Base group */


#if defined ( __CC_ARM   )
#pragma no_anon_unions
#endif

#endif
