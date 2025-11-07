/// ***************************************************************************
///
/// @file partition.h
///
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************

#ifndef _PARTITION_RT584_CM33_H_
#define _PARTITION_RT584_CM33_H_

#include "cm33.h"
#include "sysctrl.h"
#include "flashctl.h"

/*
// Flash Size Information Address
//
//
//
*/
#define   FLASH_SIZE_INFO 0x50009038
///*
//// <e>Setup behaviour of Sleep and Exception Handling
//*/
#define SCB_CSR_AIRCR_INIT          1

/*
//   <o> Deep Sleep can be enabled by
//     <0=>Secure and Non-Secure state
//     <1=>Secure state only
//   <i> Value for SCB->CSR register bit DEEPSLEEPS
*/
#define SCB_CSR_DEEPSLEEPS_VAL      0

/*
//   <o>System reset request accessible from
//     <0=> Secure and Non-Secure state
//     <1=> Secure state only
//   <i> Value for SCB->AIRCR register bit SYSRESETREQS
*/
#define SCB_AIRCR_SYSRESETREQS_VAL  0

/*
//   <o>Priority of Non-Secure exceptions is
//     <0=> Not altered
//     <1=> Lowered to 0x80-0xFF
//   <i> Value for SCB->AIRCR register bit PRIS
*/
#define SCB_AIRCR_PRIS_VAL          1

/*
//   <o>BusFault, HardFault, and NMI target
//     <0=> Secure state
//     <1=> Non-Secure state
//   <i> Value for SCB->AIRCR register bit BFHFNMINS
*/
#define SCB_AIRCR_BFHFNMINS_VAL     0

///*
//// </e>
//*/

/*
// <e>Setup behaviour of Floating Point Unit
*/
#define TZ_FPU_NS_USAGE             1

/*
// <o>Floating Point Unit usage
//     <0=> Secure state only
//     <3=> Secure and Non-Secure state
//   <i> Value for SCB->NSACR register bits CP10, CP11
*/
#define SCB_NSACR_CP10_11_VAL       3

/*
// <o>Treat floating-point registers as Secure
//     <0=> Disabled
//     <1=> Enabled
//   <i> Value for FPU->FPCCR register bit TS
*/
#define FPU_FPCCR_TS_VAL            0

/*
// <o>Clear on return (CLRONRET) accessibility
//     <0=> Secure and Non-Secure state
//     <1=> Secure state only
//   <i> Value for FPU->FPCCR register bit CLRONRETS
*/
#define FPU_FPCCR_CLRONRETS_VAL     0

/*
// <o>Clear floating-point caller saved registers on exception return
//     <0=> Disabled
//     <1=> Enabled
//   <i> Value for FPU->FPCCR register bit CLRONRET
*/
#define FPU_FPCCR_CLRONRET_VAL      1

#if defined (__ARM_FEATURE_CMSE) && (__ARM_FEATURE_CMSE == 3U)
/**
  \brief   Get IADU Target State
  \details Read the peripheral attribute field in the IADU and returns the peripheral attribute bit for the device specific attribute.
  \param [in]      IADU_Type  Device specific number.
  \return             0  if secure peripheral attribute is assigned to Secure
                      1  if secure peripheral attribute is assigned to Non Secure
  \note
 */
__STATIC_INLINE uint32_t SEC_GetIADUState(SEC_IADU_Type SecIADUn)
{
    if ((int32_t)(SecIADUn) >= 0)
    {
        return ((uint32_t)(((SEC_CTRL->SEC_PERI_ATTR[(((uint32_t)SecIADUn) >> 5UL)] & (1UL << (((uint32_t)SecIADUn) & 0x1FUL))) != 0UL) ? 1UL : 0UL));
    }
    else
    {
        return (0U);
    }
}


/**
  \brief   Set IADU Target State
  \details Sets the peripheral attribute field in the IADU and returns the peripheral attribute bit for the device specific attribute.
  \param [in]      IADU_Type  Device specific number.
  \return             0  if secure peripheral attribute is assigned to Secure
                      1  if secure peripheral attribute is assigned to Non Secure
  \note
 */
__STATIC_INLINE uint32_t SEC_SetIADUState(SEC_IADU_Type SecIADUn)
{
    if ((int32_t)(SecIADUn) >= 0)
    {
        SEC_CTRL->SEC_PERI_ATTR[(((uint32_t)SecIADUn) >> 5UL)] |=  ((uint32_t)(1UL << (((uint32_t)SecIADUn) & 0x1FUL)));
        return ((uint32_t)(((SEC_CTRL->SEC_PERI_ATTR[(((uint32_t)SecIADUn) >> 5UL)] & (1UL << (((uint32_t)SecIADUn) & 0x1FUL))) != 0UL) ? 1UL : 0UL));
    }
    else
    {
        return (0U);
    }
}


/**
  \brief   Clear IADU Target State
  \details Clears peripheral attribute field in the IADU and returns the peripheral attribute bit for the device specific attribute.
  \param [in]      IADU_Type  Device specific number.
  \return             0  if secure peripheral attribute is assigned to Secure
                      1  if secure peripheral attribute is assigned to Non Secure
  \note
 */
__STATIC_INLINE uint32_t SEC_ClearIADUState(SEC_IADU_Type SecIADUn)
{

    if ((int32_t)(SecIADUn) >= 0)
    {
        SEC_CTRL->SEC_PERI_ATTR[(((uint32_t)SecIADUn) >> 5UL)] &= ~((uint32_t)(1UL << (((uint32_t)SecIADUn) & 0x1FUL)));
        return ((uint32_t)(((SEC_CTRL->SEC_PERI_ATTR[(((uint32_t)SecIADUn) >> 5UL)] & (1UL << (((uint32_t)SecIADUn) & 0x1FUL))) != 0UL) ? 1UL : 0UL));
    }
    else
    {
        return (0U);
    }
}

#endif /* defined (__ARM_FEATURE_CMSE) && (__ARM_FEATURE_CMSE == 3U) */

__STATIC_INLINE void TZ_SAU_Setup (void)
{

    uint32_t  flash_model_info, flash_size;

#if defined (SCB_CSR_AIRCR_INIT) && (SCB_CSR_AIRCR_INIT == 1U)
    SCB->SCR   = (SCB->SCR   & ~(SCB_SCR_SLEEPDEEPS_Msk    )) |
                 ((SCB_CSR_DEEPSLEEPS_VAL     << SCB_SCR_SLEEPDEEPS_Pos)     & SCB_SCR_SLEEPDEEPS_Msk);

    SCB->AIRCR = (SCB->AIRCR & ~(SCB_AIRCR_VECTKEY_Msk   | SCB_AIRCR_SYSRESETREQS_Msk |
                                 SCB_AIRCR_BFHFNMINS_Msk | SCB_AIRCR_PRIS_Msk          ))                    |
                 ((0x05FAU                    << SCB_AIRCR_VECTKEY_Pos)      & SCB_AIRCR_VECTKEY_Msk)      |
                 ((SCB_AIRCR_SYSRESETREQS_VAL << SCB_AIRCR_SYSRESETREQS_Pos) & SCB_AIRCR_SYSRESETREQS_Msk) |
                 ((SCB_AIRCR_PRIS_VAL         << SCB_AIRCR_PRIS_Pos)         & SCB_AIRCR_PRIS_Msk)         |
                 ((SCB_AIRCR_BFHFNMINS_VAL    << SCB_AIRCR_BFHFNMINS_Pos)    & SCB_AIRCR_BFHFNMINS_Msk);
#endif /* defined (SCB_CSR_AIRCR_INIT) && (SCB_CSR_AIRCR_INIT == 1U) */

#if defined (__FPU_USED) && (__FPU_USED == 1U) && \
      defined (TZ_FPU_NS_USAGE) && (TZ_FPU_NS_USAGE == 1U)

    SCB->NSACR = (SCB->NSACR & ~(SCB_NSACR_CP10_Msk | SCB_NSACR_CP11_Msk)) |
                 ((SCB_NSACR_CP10_11_VAL << SCB_NSACR_CP10_Pos) & (SCB_NSACR_CP10_Msk | SCB_NSACR_CP11_Msk));

    FPU->FPCCR = (FPU->FPCCR & ~(FPU_FPCCR_TS_Msk | FPU_FPCCR_CLRONRETS_Msk | FPU_FPCCR_CLRONRET_Msk)) |
                 ((FPU_FPCCR_TS_VAL        << FPU_FPCCR_TS_Pos       ) & FPU_FPCCR_TS_Msk       ) |
                 ((FPU_FPCCR_CLRONRETS_VAL << FPU_FPCCR_CLRONRETS_Pos) & FPU_FPCCR_CLRONRETS_Msk) |
                 ((FPU_FPCCR_CLRONRET_VAL  << FPU_FPCCR_CLRONRET_Pos ) & FPU_FPCCR_CLRONRET_Msk );
#endif



    TZ_SAU_Disable();

    flash_model_info =  inp32(FLASH_SIZE_INFO);
    flash_size = (1 << ((flash_model_info >> 16) & 0xFF));
    flash_size = (flash_size >> 5);     /*32 bytes uints*/

    /* IDAU Start default Setting
     * set all peripheral attribuite to seccure
     * 1 is non-secure, 0 is secure.
     *
     */
    SEC_CTRL->SEC_PERI_ATTR[0] = 1;
    SEC_CTRL->SEC_PERI_ATTR[1] = 1;
    SEC_CTRL->SEC_PERI_ATTR[2] = 1;
    /*set the whole flash as non-secure first in bootrom time*/
    /*set the sram to be secure during bootrom time*/
    SEC_CTRL->SEC_FLASH_SEC_SIZE = 0x8000;			
    SEC_CTRL->SEC_FLASH_NSC_START = 0x8000;
    SEC_CTRL->SEC_FLASH_NSC_STOP = 0x8000;
    SEC_CTRL->SEC_FLASH_NS_STOP = flash_size;     /* 0x00010000 0x00000000+ (sec_flash_ns_stop *32)-1*/

    /*
     *  Here assume 64KB secure... NSC in 62KB. 0x400 bytes.
     *  Sec     address                           Non Sec address
     *  RAM0    0x30000000  0x30007FFF  32KB      RAM0  0x20000000  0x20007FFF  32KB
     *  RAM1    0x30008000  0x3000FFFF  32KB      RAM1  0x20008000  0x2000FFFF  32KB
     *  RAM2    0x30010000  0x30017FFF  32KB      RAM2  0x20010000  0x20017FFF  32KB
     *  RAM3    0x30018000  0x3001FFFF  32KB      RAM3  0x20018000  0x2001FFFF  32KB
     *  RAM4    0x30020000  0x30027FFF  32KB      RAM4  0x20020000  0x20027FFF  32KB
     *  RAM5    0x30028000  0x3002BFFF  16KB      RAM5  0x20028000  0x2002BFFF  16KB
     *  RAM6    0x3002C000  0x3002FFFF  16KB      RAM6  0x2002C000  0x2002FFFF  16KB
     *
     *  set the sram to be secure during bootrom time
     *  For bootrom we set all SRAM (64KB as secure).
     * NO NSC in SRAM.  so SEC_RAM_NSC_START = SEC_RAM_NSC_STOP
     */
    SEC_CTRL->SEC_RAM_SEC_SIZE = 0x1800;       /* 0x30000000~0x30030000*/
    SEC_CTRL->SEC_RAM_NSC_START = 0x1800;      /* */
    SEC_CTRL->SEC_RAM_NSC_STOP = 0x1800;       /* */
    //SEC_CTRL->SEC_RAM_NS_STOP = 0xFFF;      /*  */
    //SEC_CTRL->SEC_RAM_NS_STOP =

    /******  CM33 Specific Sec attribuite Numbers *************************************************/
    //SEC_ClearIADUState(SEC_IADU_Type);    secure
    //SEC_SetIADUState(SEC_IADU_Type);      non-secure,
    //* 1 is non-secure, 0 is secure.
    //SEC_ClearIADUState function config interrupt to sec world
    //Attribuite 0
#if (SYSCTRL_SECURE_EN == 1)
    SEC_ClearIADUState(SYS_CTRL_IADU_Type);
#else
    SEC_SetIADUState(SYS_CTRL_IADU_Type);
#endif

#if (GPIO_SECURE_EN == 1)
    SEC_ClearIADUState(GPIO_IADU_Type);
#else
    SEC_SetIADUState(GPIO_IADU_Type);
#endif

#if (RTC_SECURE_EN == 1)
    SEC_ClearIADUState(RTC_IADU_Type);
#else
    SEC_SetIADUState(RTC_IADU_Type);
#endif

#if (DPD_SECURE_EN == 1)
    SEC_ClearIADUState(DPD_CTRL_IADU_Type);
#else
    SEC_SetIADUState(DPD_CTRL_IADU_Type);
#endif

#if (SOC_PMU_SECURE_EN == 1)
    SEC_ClearIADUState(SOC_PMU_IADU_Type);
#else
    SEC_SetIADUState(SOC_PMU_IADU_Type);
#endif

#if (FLASHCTRL_SECURE_EN == 1)
    SEC_ClearIADUState(FLASH_CONTROL_IADU_Type);
#else
    SEC_SetIADUState(FLASH_CONTROL_IADU_Type);
#endif

#if (TIMER0_SECURE_EN == 1)
    SEC_ClearIADUState(TIMER0_IADU_Type);
#else
    SEC_SetIADUState(TIMER0_IADU_Type);
#endif

#if (TIMER1_SECURE_EN == 1)
    SEC_ClearIADUState(TIMER1_IADU_Type);
#else
    SEC_SetIADUState(TIMER1_IADU_Type);
#endif

#if (TIMER2_SECURE_EN == 1)
    SEC_ClearIADUState(TIMER2_IADU_Type);
#else
    SEC_SetIADUState(TIMER2_IADU_Type);
#endif

#if (TIMER32K0_SECURE_EN == 1)
    SEC_ClearIADUState(TIMER32K0_IADU_Type);
#else
    SEC_SetIADUState(TIMER32K0_IADU_Type);
#endif

#if (TIMER32K1_SECURE_EN == 1)
    SEC_ClearIADUState(TIMER32K1_IADU_Type);
#else
    SEC_SetIADUState(TIMER32K1_IADU_Type);
#endif

#if (WDT_SECURE_EN == 1)
    SEC_ClearIADUState(WDT_IADU_Type);
#else
    SEC_SetIADUState(WDT_IADU_Type);
#endif

#if (UART0_SECURE_EN == 1)
    SEC_ClearIADUState(UART0_IADU_Type);
#else
    SEC_SetIADUState(UART0_IADU_Type);
#endif

#if (UART1_SECURE_EN == 1)
    SEC_ClearIADUState(UART1_IADU_Type);
#else
    SEC_SetIADUState(UART1_IADU_Type);
#endif

#if (I2C_SLAVE_SECURE_EN == 1)
    SEC_ClearIADUState(I2C_S_IADU_Type);
#else
    SEC_SetIADUState(I2C_S_IADU_Type);
#endif

#if (COMM_SUBSYSTEM_AHB_SECURE_EN == 1)
    SEC_ClearIADUState(RT569_AHB_IADU_Type);
#else
    SEC_SetIADUState(RT569_AHB_IADU_Type);
#endif

#if (RCO32K_CAL_SECURE_EN == 1)
    SEC_ClearIADUState(RCO32K_CAL_IADU_Type);
#else
    SEC_SetIADUState(RCO32K_CAL_IADU_Type);
#endif

#if (BOD_COMP_SECURE_EN == 1)
    SEC_ClearIADUState(BOD_COMP_IADU_Type);
#else
    SEC_SetIADUState(BOD_COMP_IADU_Type);
#endif

#if (AUX_COMP_SECURE_EN == 1)
    SEC_ClearIADUState(AUX_COMP_IADU_Type);
#else
    SEC_SetIADUState(AUX_COMP_IADU_Type);
#endif

#if (RCO1M_CAL_SECURE_EN == 1)
    SEC_ClearIADUState(RCO1M_CAL_IADU_Type);
#else
    SEC_SetIADUState(RCO1M_CAL_IADU_Type);
#endif


    //Attribuite 1
#if (QSPI0_SECURE_EN == 1)
    SEC_ClearIADUState(QSPI0_IADU_Type);
#else
    SEC_SetIADUState(QSPI0_IADU_Type);
#endif

#if (QSPI1_SECURE_EN == 1)
    SEC_ClearIADUState(QSPI1_IADU_Type);
#else
    SEC_SetIADUState(QSPI1_IADU_Type);
#endif

#if (IRM_SECURE_EN == 1)
    SEC_ClearIADUState(IRM_IADU_Type);
#else
    SEC_SetIADUState(IRM_IADU_Type);
#endif

#if (UART2_SECURE_EN == 1)
    SEC_ClearIADUState(UART2_IADU_Type);
#else
    SEC_SetIADUState(UART2_IADU_Type);
#endif

#if (PWM_SECURE_EN == 1)
    SEC_ClearIADUState(PWM_IADU_Type);
#else
    SEC_SetIADUState(PWM_IADU_Type);
#endif

#if (XDMA_SECURE_EN == 1)
    SEC_ClearIADUState(XDMA_IADU_Type);
#else
    SEC_SetIADUState(XDMA_IADU_Type);
#endif

#if (DMA0_SECURE_EN == 1)
    SEC_ClearIADUState(DMA0_IADU_Type);
#else
    SEC_SetIADUState(DMA0_IADU_Type);
#endif

#if (DMA1_SECURE_EN == 1)
    SEC_ClearIADUState(DMA1_IADU_Type);
#else
    SEC_SetIADUState(DMA1_IADU_Type);
#endif

#if (I2C_MASTER0_SECURE_EN == 1)
    SEC_ClearIADUState(I2C_M0_IADU_Type);
#else
    SEC_SetIADUState(I2C_M0_IADU_Type);
#endif

#if (I2C_MASTER1_SECURE_EN == 1)
    SEC_ClearIADUState(I2C_M1_IADU_Type);
#else
    SEC_SetIADUState(I2C_M1_IADU_Type);
#endif

#if (I2S0_SECURE_EN == 1)
    SEC_ClearIADUState(I2S0_M_IADU_Type);
#else
    SEC_SetIADUState(I2S0_M_IADU_Type);
#endif

#if (SADC_SECURE_EN == 1)
    SEC_ClearIADUState(SADC_IADU_Type);
#else
    SEC_SetIADUState(SADC_IADU_Type);
#endif

#if (SW_IRQ0_SECURE_EN == 1)
    SEC_ClearIADUState(SW_IRQ0_IADU_Type);
#else
    SEC_SetIADUState(SW_IRQ0_IADU_Type);
#endif

#if (SW_IRQ1_SECURE_EN == 1)
    SEC_ClearIADUState(SW_IRQ1_IADU_Type);
#else
    SEC_SetIADUState(SW_IRQ1_IADU_Type);
#endif


    //Attribuite 2
#if (CRYPTO_SECURE_EN == 1)
    SEC_ClearIADUState(CRYPTO_IADU_Type);
#else
    SEC_SetIADUState(CRYPTO_IADU_Type);
#endif

#if (PUF_OTP_SECURE_EN == 1)
    SEC_ClearIADUState(PUF_OTP_IADU_Type);
#else
    SEC_SetIADUState(PUF_OTP_IADU_Type);
#endif
    //SEC_SetIADUState function config interrupt to non sec world


    /******  rt584 cm33S pecific Interrupt Numbers *************************************************/
    //NVIC_ClearTargetState(IQRn_Type);     secure
    //NVIC_SetTargetState(IQRn_Type);       no secure
    //Nvic ClearTargeStat function config interrupt to sec world
    NVIC_ClearTargetState(Sec_Ctrl_IQRn);
    //Attribuite 0
#if (GPIO_SECURE_EN == 1)
    NVIC_ClearTargetState(Gpio_IRQn);
#else
    NVIC_SetTargetState(Gpio_IRQn);
#endif

#if (RTC_SECURE_EN == 1)
    NVIC_ClearTargetState(Rtc_IRQn);
#else
    NVIC_SetTargetState(Rtc_IRQn);
#endif

#if (FLASHCTRL_SECURE_EN == 1)
    NVIC_ClearTargetState(FlashCtl_IRQn);
#else
    NVIC_SetTargetState(FlashCtl_IRQn);
#endif

#if (TIMER0_SECURE_EN == 1)
    NVIC_ClearTargetState(Timer0_IRQn);
#else
    NVIC_SetTargetState(Timer0_IRQn);
#endif

#if (TIMER1_SECURE_EN == 1)
    NVIC_ClearTargetState(Timer1_IRQn);
#else
    NVIC_SetTargetState(Timer1_IRQn);
#endif

#if (TIMER2_SECURE_EN == 1)
    NVIC_ClearTargetState(Timer2_IRQn);
#else
    NVIC_SetTargetState(Timer2_IRQn);
#endif

#if (TIMER32K0_SECURE_EN == 1)
    NVIC_ClearTargetState(Timer32K0_IRQn);
#else
    NVIC_SetTargetState(Timer32K0_IRQn);
#endif

#if (TIMER32K1_SECURE_EN == 1)
    NVIC_ClearTargetState(Timer32K1_IRQn);
#else
    NVIC_SetTargetState(Timer32K1_IRQn);
#endif

#if (WDT_SECURE_EN == 1)
    NVIC_ClearTargetState(Wdt_IRQn);
#else
    NVIC_SetTargetState(Wdt_IRQn);
#endif

#if (UART0_SECURE_EN == 1)
    NVIC_ClearTargetState(Uart0_IRQn);
#else
    NVIC_SetTargetState(Uart0_IRQn);
#endif

#if (UART1_SECURE_EN == 1)
    NVIC_ClearTargetState(Uart1_IRQn);
#else
    NVIC_SetTargetState(Uart1_IRQn);
#endif

#if (I2C_SLAVE_SECURE_EN == 1)
    NVIC_ClearTargetState(I2C_Slave_IRQn);
#else
    NVIC_SetTargetState(I2C_Slave_IRQn);
#endif

#if (COMM_SUBSYSTEM_AHB_SECURE_EN == 1)
    NVIC_ClearTargetState(CommSubsystem_IRQn);
#else
    NVIC_SetTargetState(CommSubsystem_IRQn);
#endif

#if (BOD_COMP_SECURE_EN == 1)
    NVIC_ClearTargetState(Bod_Comp_IRQn);
#else
    NVIC_SetTargetState(Bod_Comp_IRQn);
#endif

#if (AUX_COMP_SECURE_EN == 1)
    NVIC_ClearTargetState(Aux_Comp_IRQn);
#else
    NVIC_SetTargetState(Aux_Comp_IRQn);
#endif

    //Attribuite 1
#if (QSPI0_SECURE_EN == 1)
    NVIC_ClearTargetState(Qspi0_IRQn);
#else
    NVIC_SetTargetState(Qspi0_IRQn);
#endif

#if (QSPI1_SECURE_EN == 1)
    NVIC_ClearTargetState(Qspi1_IRQn);
#else
    NVIC_SetTargetState(Qspi1_IRQn);
#endif

#if (IRM_SECURE_EN == 1)
    NVIC_ClearTargetState(Irm_IRQn);
#else
    NVIC_SetTargetState(Irm_IRQn);
#endif

#if (UART2_SECURE_EN == 1)
    NVIC_ClearTargetState(Uart2_IRQn);
#else
    NVIC_SetTargetState(Uart2_IRQn);
#endif

#if (PWM_SECURE_EN == 1)
    NVIC_ClearTargetState(Pwm0_IRQn);
    NVIC_ClearTargetState(Pwm1_IRQn);
    NVIC_ClearTargetState(Pwm2_IRQn);
    NVIC_ClearTargetState(Pwm3_IRQn);
    NVIC_ClearTargetState(Pwm4_IRQn);
#else
    NVIC_SetTargetState(Pwm0_IRQn);
    NVIC_SetTargetState(Pwm1_IRQn);
    NVIC_SetTargetState(Pwm2_IRQn);
    NVIC_SetTargetState(Pwm3_IRQn);
    NVIC_SetTargetState(Pwm4_IRQn);
#endif

#if (DMA0_SECURE_EN == 1)
    NVIC_ClearTargetState(Dma_Ch0_IRQn);
#else
    NVIC_SetTargetState(Dma_Ch0_IRQn);
#endif

#if (DMA1_SECURE_EN == 1)
    NVIC_ClearTargetState(Dma_Ch1_IRQn);
#else
    NVIC_SetTargetState(Dma_Ch1_IRQn);
#endif

#if (I2C_MASTER0_SECURE_EN == 1)
    NVIC_ClearTargetState(I2C_Master0_IRQn);
#else
    NVIC_SetTargetState(I2C_Master0_IRQn);
#endif

#if (I2C_MASTER1_SECURE_EN == 1)
    NVIC_ClearTargetState(I2C_Master1_IRQn);
#else
    NVIC_SetTargetState(I2C_Master1_IRQn);
#endif

#if (I2S0_SECURE_EN == 1)
    NVIC_ClearTargetState(I2s0_IRQn);
#else
    NVIC_SetTargetState(I2s0_IRQn);
#endif

#if (SADC_SECURE_EN == 1)
    NVIC_ClearTargetState(Sadc_IRQn);
#else
    NVIC_SetTargetState(Sadc_IRQn);
#endif

#if (SW_IRQ0_SECURE_EN == 1)
    NVIC_ClearTargetState(Soft0_IRQn);
#else
    NVIC_SetTargetState(Soft0_IRQn);
#endif

#if (SW_IRQ1_SECURE_EN == 1)
    NVIC_ClearTargetState(Soft1_IRQn);
#else
    NVIC_SetTargetState(Soft1_IRQn);
#endif


    //Attribuite 2
#if (CRYPTO_SECURE_EN == 1)
    NVIC_ClearTargetState(Crypto_IRQn);
#else
    NVIC_SetTargetState(Crypto_IRQn);
#endif

#if (PUF_OTP_SECURE_EN == 1)
    NVIC_ClearTargetState(OTP_IRQn);
#else
    NVIC_SetTargetState(OTP_IRQn);
#endif



    //Nvic SetTargeStat function config interrupt to non sec wrold


    /*Enable IDAU*/
    SEC_CTRL->SEC_IDAU_CTRL =  1;

    /*
     * 2022/12/21: set SAU to be all non-secure
     *
     */
    SAU->CTRL |= (SAU_CTRL_ALLNS_Msk);
    /*TODO: depends on system setting.*/

}


#endif
