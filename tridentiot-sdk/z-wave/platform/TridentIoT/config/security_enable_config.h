/// ***************************************************************************
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************

/**************************************************************************//**
 * @file     rt584_security_enable_config.h
 * @version
 * @brief   define project config
 ******************************************************************************/
#ifndef ___RT584_SECURITY_ENABLE_CONFIG_H__
#define ___RT584_SECURITY_ENABLE_CONFIG_H__

/* ================================================================================ */
/* ================        Peripheral Secure Enable define         ================ */
/* ================================================================================ */

/**
 * @brief Attribuite 0.
 */

#define SYSCTRL_SECURE_EN                    1
#define GPIO_SECURE_EN                       1
#define RTC_SECURE_EN                        1
#define DPD_SECURE_EN                        1
#define SOC_PMU_SECURE_EN                    1
#define FLASHCTRL_SECURE_EN                  1
#define TIMER0_SECURE_EN                     1
#define TIMER1_SECURE_EN                     1
#define TIMER2_SECURE_EN                     1
#define TIMER32K0_SECURE_EN                  1
#define TIMER32K1_SECURE_EN                  1
#define WDT_SECURE_EN                        1
#define UART0_SECURE_EN                      1
#define UART1_SECURE_EN                      1
#define I2C_SLAVE_SECURE_EN                  1
#define COMM_SUBSYSTEM_AHB_SECURE_EN         1
#define RCO32K_CAL_SECURE_EN                 1
#define BOD_COMP_SECURE_EN                   1
#define AUX_COMP_SECURE_EN                   1
#define RCO1M_CAL_SECURE_EN                  1

/**
 * @brief Attribuite 1.
 */
#define QSPI0_SECURE_EN                      1
#define QSPI1_SECURE_EN                      1
#define IRM_SECURE_EN                        1
#define UART2_SECURE_EN                      1
#define PWM_SECURE_EN                        1
#define XDMA_SECURE_EN                       1
#define DMA0_SECURE_EN                       1
#define DMA1_SECURE_EN                       1
#define I2C_MASTER0_SECURE_EN                1
#define I2C_MASTER1_SECURE_EN                1
#define I2S0_SECURE_EN                       1
#define SADC_SECURE_EN                       1
#define SW_IRQ0_SECURE_EN                    1
#define SW_IRQ1_SECURE_EN                    1

/**
 * @brief Attribuite 2.
 */
#define CRYPTO_SECURE_EN                     1
#define PUF_OTP_SECURE_EN                    1

#endif
