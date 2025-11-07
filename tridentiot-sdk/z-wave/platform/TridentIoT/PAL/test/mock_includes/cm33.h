/// ****************************************************************************
/// @file cm33.h
///
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************

#ifndef _CM33_H
#define _CM33_H
#include <stdint.h>
#include <stdbool.h>
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


/* IO definitions (access restrictions to peripheral registers) */
/**
    \defgroup CMSIS_glob_defs CMSIS Global Defines

    <strong>IO Type Qualifiers</strong> are used
    \li to specify the access to peripheral variables.
    \li for automatic generation of peripheral register debug information.
*/
#ifdef __cplusplus
  #define   __I     volatile             /*!< Defines 'read only' permissions */
#else
  #define   __I     volatile const       /*!< Defines 'read only' permissions */
#endif
#define     __O     volatile             /*!< Defines 'write only' permissions */
#define     __IO    volatile             /*!< Defines 'read / write' permissions */

/* following defines should be used for structure members */
#define     __IM     volatile const      /*! Defines 'read only' structure member permissions */
#define     __OM     volatile            /*! Defines 'write only' structure member permissions */
#define     __IOM    volatile            /*! Defines 'read / write' structure member permissions */


#endif //_CM33_H_