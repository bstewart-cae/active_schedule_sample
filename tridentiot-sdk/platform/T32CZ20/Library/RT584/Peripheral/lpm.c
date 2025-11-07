/**
 ******************************************************************************
 * @file    lpm.c
 * @author
 * @brief   lpm driver file
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

/***********************************************************************************************************************
 *    INCLUDES
 **********************************************************************************************************************/
#include "cm33.h"
#include "lpm.h"
#include "sysctrl.h"
#include "rf_mcu.h"
#include "gpio.h"
#include "dpd.h"
/***********************************************************************************************************************
 *    GLOBAL VARIABLES
 **********************************************************************************************************************/
static volatile  uint32_t low_power_mask_status = LOW_POWER_NO_MASK;
static volatile  uint32_t comm_subsystem_wakeup_mask_status = COMM_SUBSYS_WAKEUP_NO_MASK;
static volatile  low_power_level_cfg_t low_power_level = LOW_POWER_LEVEL_NORMAL;
static volatile  low_power_wakeup_cfg_t low_power_wakeup = LOW_POWER_WAKEUP_NULL;
static volatile  uint32_t low_power_wakeup_update = 0;
static volatile  uint32_t low_power_wakeup_uart = 0;
static volatile  uint32_t low_power_wakeup_gpio = 0;
static volatile  uint32_t low_power_timer_pwm = 0;
static volatile  uint32_t low_power_aux_level = 0;
static volatile  uint32_t low_power_bod_level = 0;
/***********************************************************************************************************************
 *    CONSTANTS AND DEFINES
 **********************************************************************************************************************/
#define LPM_SRAM0_RETAIN 0x1F
/***********************************************************************************************************************
 *    GLOBAL FUNCTIONS
 **********************************************************************************************************************/
void Lpm_Init()
{
    low_power_mask_status = LOW_POWER_NO_MASK;
    comm_subsystem_wakeup_mask_status = COMM_SUBSYS_WAKEUP_NO_MASK;
    low_power_level = LOW_POWER_LEVEL_NORMAL;
    low_power_wakeup = LOW_POWER_WAKEUP_NULL;
    low_power_wakeup_update = 0;
    low_power_wakeup_uart = 0;
    low_power_wakeup_gpio = 0;
    low_power_timer_pwm = 0;
    low_power_aux_level = 0;
    low_power_bod_level = 0;
}
/**
* @brief umask low power mode mask value
*/
void Lpm_Low_Power_Mask(uint32_t mask)
{
    low_power_mask_status |= mask;
}
/**
* @brief unmask low power mode mask value
*/
void Lpm_Low_Power_Unmask(uint32_t unmask)
{
    low_power_mask_status &= (~unmask);
}
/**
* @brief get low power mode mask status
*/
uint32_t Lpm_Get_Low_Power_Mask_Status(void)
{
    return low_power_mask_status;
}
/**
* @brief mask communication system wakeup value
*/
void Lpm_Comm_Subsystem_Wakeup_Mask(uint32_t mask)
{
    comm_subsystem_wakeup_mask_status |= mask;
}
/**
* @brief unmask communication system wakeup value
*/
void Lpm_Comm_Subsystem_Wakeup_Unmask(uint32_t unmask)
{
    comm_subsystem_wakeup_mask_status &= (~unmask);
}
/**
* @brief check communication system ready
*/
void Lpm_Comm_Subsystem_Check_System_Ready(void)
{
    uint32_t status;
    do
    {
        status = COMM_SUBSYSTEM_AHB->COMM_SUBSYSTEM_INFO;

    } while (((status & 0x01) != 1));

}
/**
* @brief disable wait communication subsystem 32k done
*/
void Lpm_Comm_Subsystem_Disable_Wait_32k_Done(void)
{
    //
}

/**
* @brief get subsystem power status
*/
static uint32_t Lpm_Comm_Subsystem_Power_Status(void)
{
    return  ((COMM_SUBSYSTEM_AHB->COMM_SUBSYSTEM_INFO & 0x6) >> 1);
}
/**
* @brief check communication subsystem slee mode
*/
void Lpm_Comm_Subsystem_Check_System_DeepSleep(void)
{
    uint32_t status;

    do
    {
        status = ((COMM_SUBSYSTEM_AHB->COMM_SUBSYSTEM_INFO & 0x6) >> 1);
    } while ((status != 1UL));
}

/**
* @brief check communication subsystem slee mode
*/
void Lpm_Comm_Subsystem_Check_Sleep_Mode(uint32_t mode)
{
    uint32_t status;

    do
    {
        status = ((COMM_SUBSYSTEM_AHB->COMM_SUBSYSTEM_INFO & 0x6) >> 1);
    } while ((status != mode));
}
/**
* @brief get communication subsystem wakeup mask status
*/
uint32_t Lpm_Get_Comm_Subsystem_Wakeup_Mask_Status(void)
{
    return comm_subsystem_wakeup_mask_status;
}
/**
* @brief set low power mode
*/
void Lpm_Set_Low_Power_Level(low_power_level_cfg_t low_power_level_cfg)
{
    low_power_level = low_power_level_cfg;
    low_power_wakeup_update = 1;
}
/**
* @brief enable low power mode wake up source
*/
void Lpm_Enable_Low_Power_Wakeup(low_power_wakeup_cfg_t low_power_wakeup_cfg)
{
    uint32_t wakeup_source = 0;
    uint32_t wakeup_source_index = 0;

    wakeup_source = (low_power_wakeup_cfg & 0xFFFF);

    if (wakeup_source == LOW_POWER_WAKEUP_GPIO)
    {
        wakeup_source_index = (low_power_wakeup_cfg >> 16);
        low_power_wakeup_gpio |= (1 << wakeup_source_index);
    }
    else if (wakeup_source == LOW_POWER_WAKEUP_UART_RX)
    {
        wakeup_source_index = (low_power_wakeup_cfg >> 16);
        low_power_wakeup_uart |= (1 << wakeup_source_index);
    }
    else if (wakeup_source == LOW_POWER_WAKEUP_UART_DATA)
    {
        wakeup_source_index = (low_power_wakeup_cfg >> 16);
        low_power_wakeup_uart |= (1 << wakeup_source_index);
    }
    else
    {
        low_power_wakeup |= wakeup_source;
    }

    low_power_wakeup_update = 1;
}
/**
* @brief disable low power mode wake up source
*/
void Lpm_Disable_Low_Power_Wakeup(low_power_wakeup_cfg_t low_power_wakeup_cfg)
{
    uint32_t wakeup_source = 0;
    uint32_t wakeup_source_index = 0;

    wakeup_source = (low_power_wakeup_cfg & 0xFFFF);

    if (wakeup_source == LOW_POWER_WAKEUP_GPIO)
    {
        wakeup_source_index = (low_power_wakeup_cfg >> 16);
        low_power_wakeup_gpio &= (~(1 << wakeup_source_index));
    }
    else if (wakeup_source == LOW_POWER_WAKEUP_UART_RX)
    {
        wakeup_source_index = (low_power_wakeup_cfg >> 16);
        low_power_wakeup_uart &= (~(1 << wakeup_source_index));
    }
    else if (wakeup_source == LOW_POWER_WAKEUP_UART_DATA)
    {
        wakeup_source_index = (low_power_wakeup_cfg >> 16);
        low_power_wakeup_uart &= (~(1 << wakeup_source_index));
    }
    else
    {
        low_power_wakeup &= ~wakeup_source;
    }

    low_power_wakeup_update = 1;
}

/**
* @brief Enable timer pwm in low power mode
*/
void Lpm_Enable_Timer_Pwm(void)
{
    low_power_timer_pwm = 1;
}

/**
* @brief Disable timer pwm in low power mode
*/
void Lpm_Disable_Timer_Pwm(void)
{
    low_power_timer_pwm = 0;
}

/**
* @brief Enable aux level wake up in low power mode
*/
void Lpm_Enable_Aux_Level(void)
{
    low_power_aux_level = 1;
}

/**
* @brief Disable aux level wake up in low power mode
*/
void Lpm_Disable_Aux_Level(void)
{
    low_power_aux_level = 0;
}

/**
* @brief Enable bod level wake up in low power mode
*/
void Lpm_Enable_Bod_Level(void)
{
    low_power_bod_level = 1;
}

/**
* @brief Disable bod level wake up in low power mode
*/
void Lpm_Disable_Bod_Level(void)
{
    low_power_bod_level = 0;
}

/**
* @brief set low power mode wake up source
*/
void Lpm_Set_Platform_Low_Power_Wakeup(low_power_platform_enter_mode_cfg_t platform_low_power_mode)
{

    do
    {
        if (low_power_wakeup_update == 0)
        {
            break;
        }

        low_power_wakeup_update = 0;

        if (platform_low_power_mode == LOW_POWER_PLATFORM_ENTER_SLEEP)
        {
            Lpm_Set_Peripheral_Power_Off(MCU_PERI2_PWR_OFF_SLEEP, ENABLE);
            Lpm_Set_Peripheral_Power_Off(MCU_PERI3_PWR_OFF_SLEEP, ENABLE);

            /* UART0 sleep wake enable selection in Sleep */
            if (low_power_wakeup_uart & (1 << 0))
            {
                UART0->WAKE_SLEEP_EN = 1;
            }
            else if (low_power_wakeup_uart & (1 << 3))
            {
                Lpm_Set_Peripheral_Power_Off(MCU_PERI3_PWR_OFF_SLEEP, DISABLE);
                UART0->WAKE_SLEEP_EN = 1;
            }
            else
            {
                UART0->WAKE_SLEEP_EN = 0;
            }

            /* UART1 sleep wake enable selection in Sleep */
            if (low_power_wakeup_uart & (1 << 1))
            {
                Lpm_Set_Peripheral_Power_Off(MCU_PERI3_PWR_OFF_SLEEP, DISABLE);
                UART1->WAKE_SLEEP_EN = 1;
            }
            else if (low_power_wakeup_uart & (1 << 4))
            {
                Lpm_Set_Peripheral_Power_Off(MCU_PERI3_PWR_OFF_SLEEP, DISABLE);
                UART1->WAKE_SLEEP_EN = 1;
            }
            else
            {
                UART1->WAKE_SLEEP_EN = 0;
            }

            /* UART2 sleep wake enable selection in Sleep */
            if (low_power_wakeup_uart & (1 << 2))
            {
                Lpm_Set_Peripheral_Power_Off(MCU_PERI2_PWR_OFF_SLEEP, DISABLE);
                Lpm_Set_Peripheral_Power_Off(MCU_PERI3_PWR_OFF_SLEEP, DISABLE);
                UART2->WAKE_SLEEP_EN = 1;
            }
            else if (low_power_wakeup_uart & (1 << 5))
            {
                Lpm_Set_Peripheral_Power_Off(MCU_PERI2_PWR_OFF_SLEEP, DISABLE);
                Lpm_Set_Peripheral_Power_Off(MCU_PERI3_PWR_OFF_SLEEP, DISABLE);
                UART2->WAKE_SLEEP_EN = 1;
            }
            else
            {
                UART2->WAKE_SLEEP_EN = 0;
            }

            if ( low_power_timer_pwm == 1)
            {
                Lpm_Set_Peripheral_Power_Off(MCU_PERI3_PWR_OFF_SLEEP, DISABLE);
            }
        }
        else if (platform_low_power_mode == LOW_POWER_PLATFORM_ENTER_DEEP_SLEEP)
        {

            SYSCTRL->SRAM_LOWPOWER_3.bit.CFG_DS_RCO32K_OFF = 1;  //Disable RCO32K in Deepsleep Mode

            if (low_power_wakeup & LOW_POWER_WAKEUP_RTC_TIMER)
            {
                SYSCTRL->SRAM_LOWPOWER_3.bit.CFG_DS_RCO32K_OFF = 0;
                SYSCTRL->SYS_CLK_CTRL2.bit.EN_CK32_RTC = 1;
            }

            if (low_power_wakeup & LOW_POWER_WAKEUP_AUX_COMP)
            {
                SYSCTRL->SRAM_LOWPOWER_3.bit.CFG_DS_RCO32K_OFF = 0;
                if ( low_power_aux_level == 1 )
                {
                    SYSCTRL->SYS_CLK_CTRL2.bit.EN_CK32_AUXCOMP = 0;
                }
                else
                {
                    SYSCTRL->SYS_CLK_CTRL2.bit.EN_CK32_AUXCOMP = 1;
                }
                AUX_COMP->COMP_DIG_CTRL0.bit.COMP_EN_DS = 1;
            }

            if (low_power_wakeup & LOW_POWER_WAKEUP_BOD_COMP)
            {
                SYSCTRL->SRAM_LOWPOWER_3.bit.CFG_DS_RCO32K_OFF = 0;
                if ( low_power_aux_level == 1 )
                {
                    SYSCTRL->SYS_CLK_CTRL2.bit.EN_CK32_BODCOMP = 0;
                }
                else
                {
                    SYSCTRL->SYS_CLK_CTRL2.bit.EN_CK32_BODCOMP = 1;
                }
                BOD_COMP->COMP_DIG_CTRL0.bit.COMP_EN_DS = 1;
            }
        }
        else if (platform_low_power_mode == LOW_POWER_PLATFORM_ENTER_POWER_DOWN_MODE)
        {



        }
    } while (0);
}
/**
* @brief disable sram in normal mode
*/
void Lpm_Set_Sram_Normal_Shutdown(uint32_t value)
{
    Set_Sram_Shutdown_Normal(value);                                   /* SRAM shut-down control in Normal Mode, set the corresponding bit0~bit4 to shut-down SRAM0~SRAM4 in Normal Mode */
}
/**
* @brief disable sram in sleep and deep sleep mode
*/
void Lpm_Set_Sram_Sleep_Deepsleep_Shutdown(uint32_t value)
{
    if (low_power_level == LPM_SLEEP)
    {
        Set_Sram_Shutdown_Sleep(value);
    }
    else if (low_power_level == LPM_DEEP_SLEEP)
    {
        Set_Sram_Shutdown_DeepSleep(value);
    }
    else if (low_power_level == LPM_POWER_DOWN)
    {
        Set_Sram_Shutdown_DeepSleep(value);
    }
}
/**
* @brief enable or disable peripheral group 1/2/3
*        power off in sleep and deep sleep mode
*/
void Lpm_Set_Peripheral_Power_Off(uint32_t peripwroffctlbit, uint32_t isenabled)
{
    Set_Peripheral_Power_Off(peripwroffctlbit, isenabled);
}
/**
* @brief disable sram in sleep and deep sleep mode
*/
void Lpm_Set_GPIO_Deepsleep_Wakeup_Invert(uint32_t value)
{
    set_deepsleep_wakeup_invert(value);                                /* Set the corresponding bit0~bit31 to invert the GPIO0~GPIO31 for wakeup in DeepSleep Mode */
}

/**
* @brief disable sram in sleep and deep sleep mode
*/
void Lpm_Set_GPIO_Deepsleep_Wakeup_Invert_Ex(uint8_t num, uint32_t value)    /* Set the corresponding bit0~bit31 to invert the GPIO0~GPIO31 for wakeup in DeepSleep Mode */
{
    Gpio_Setup_Deep_Sleep_Io(num, value);
}
/**
* @brief disable sram in sleep and deep sleep mode
*/
void Lpm_Set_GPIO_Powerdown_Wakeup_Invert_Ex(uint8_t num, uint32_t value)    /* Set the corresponding bit0~bit31 to invert the GPIO0~GPIO31 for wakeup in DeepSleep Mode */
{
    Gpio_Setup_Deep_Powerdown_Io(num, value);
}

/**
* @brief check communication subsystem slee mode
*/
void Lpm_Comm_Subsystem_Power_Status_Check(commumication_subsystem_pwr_mode_cfg_t mode)
{
    uint32_t status;

    do
    {
        status = ((COMM_SUBSYSTEM_AHB->COMM_SUBSYSTEM_INFO & 0x6) >> 1);

    } while ((status != mode));
}
/**
* @brief set mcu enter low power mode
*/
void Lpm_Sub_System_Low_Power_Mode(commumication_subsystem_pwr_mode_cfg_t mode)
{

    if (mode == COMMUMICATION_SUBSYSTEM_PWR_STATE_SLEEP)
    {
        /* set platform subsystem entering sleep mode*/
        COMM_SUBSYSTEM_AHB->COMM_SUBSYSTEM_HOST |= COMMUMICATION_SUBSYSTEM_HOSTMODE;
        COMM_SUBSYSTEM_AHB->COMM_SUBSYSTEM_HOST |= COMMUMICATION_SUBSYSTEM_SLEEP;
    }
    else if (mode == COMMUMICATION_SUBSYSTEM_PWR_STATE_DEEP_SLEEP)
    {
        if (Lpm_Comm_Subsystem_Power_Status() == COMMUMICATION_SUBSYSTEM_PWR_STATE_NORMAL)
        {
            RfMcu_EventQueueReset();
        }

        if (RfMcu_RxQueueCheck())
        {
            /* Reset BMU to let radio MCU entering sleep */
            RfMcu_BmuReset();
        }

        /* check subsystem enter sleep mode*/
        if (Lpm_Comm_Subsystem_Power_Status() == COMMUMICATION_SUBSYSTEM_PWR_STATE_SLEEP ||
                Lpm_Comm_Subsystem_Power_Status() == COMMUMICATION_SUBSYSTEM_PWR_STATE_DEEP_SLEEP)
        {
            //sub system deep sleep mode;
        }
        else
        {
            Lpm_Comm_Subsystem_Power_Status_Check(COMMUMICATION_SUBSYSTEM_PWR_STATE_SLEEP);
        }

        COMM_SUBSYSTEM_AHB->COMM_SUBSYSTEM_HOST |= COMMUMICATION_SUBSYSTEM_HOSTMODE;
        if (__NVIC_GetPendingIRQ(CommSubsystem_IRQn))
        {
            COMM_SUBSYSTEM_AHB->COMM_SUBSYSTEM_HOST |= COMMUMICATION_SUBSYSTEM_WAKEUP;
            COMM_SUBSYSTEM_AHB->COMM_SUBSYSTEM_HOST |= COMMUMICATION_SUBSYSTEM_RESET;
            Lpm_Comm_Subsystem_Check_System_Ready();
            RfMcu_InterruptClear(0xFFFF);
            __NVIC_ClearPendingIRQ(CommSubsystem_IRQn);
        }
        COMM_SUBSYSTEM_AHB->COMM_SUBSYSTEM_HOST |= COMMUMICATION_SUBSYSTEM_DEEPSLEEP;

    }
    else
    {
        /* set platform subsystem entering normal mode*/
        COMM_SUBSYSTEM_AHB->COMM_SUBSYSTEM_HOST |= COMMUMICATION_SUBSYSTEM_HOSTMODE;
        COMM_SUBSYSTEM_AHB->COMM_SUBSYSTEM_HOST |= COMMUMICATION_SUBSYSTEM_RESET;
        COMM_SUBSYSTEM_AHB->COMM_SUBSYSTEM_HOST |= COMMUMICATION_SUBSYSTEM_DIS_HOSTMODE;
        Lpm_Comm_Subsystem_Check_System_Ready();
    }

    Lpm_Comm_Subsystem_Power_Status_Check(mode);
}

/**
* @brief set mcu deep sleep ski isp fas boot
*/
void Lpm_Deep_Sleep_Wakeup_Fastboot()
{
    DPD_Set_DeepSleep_Wakeup_Fast_Boot();
}

/**
* @brief set slow clock setting time
*/
void Lpm_Deep_Sleep_Set_RCO_Settle_Time()
{
    if (Sys_Slow_Clk_Mode() != RCO32K_MODE)
    {

        
        PMU_CTRL->PMU_SOC_PMU_TIMING.bit.CFG_MV_SETTLE_TIME = 12;   //MV  settle time = 400us (400us for 1.8V, 3.3V 200us)
        PMU_CTRL->PMU_SOC_PMU_TIMING.bit.CFG_LV_SETTLE_TIME = 5;    //LV  settle time = 150us
    
        PMU_CTRL->PMU_SOC_PMU_XTAL1.bit.CFG_XTAL_SETTLE_TIME = 31;  //1ms
       
        PMU_CTRL->PMU_RVD0.bit.CFG_XTAL_FAST_TIME = 15;             //0.5ms
        
        PMU_CTRL->PMU_SOC_PMU_TIMING.bit.CFG_PWRX_SETTLE_TIME = 2;
    }
}
/**
* @brief clear interrupt
*/
static void Lpm_Deep_Sleep_Clr_Interrupt(void)
{
    uint32_t i = 0;

    for (i = 0; i < 48; i++)
    {

        if (__NVIC_GetPendingIRQ((IRQn_Type)(i)))
        {
            __NVIC_ClearPendingIRQ((IRQn_Type)i);
        }
    }

    GPIO->EDGE_INT_CLR = 0xFFFFFFFF;

    FLASH->FLASH_INT |= 0x00010000;

    UART0->ISR = 0x000000FF;
    UART1->ISR = 0x000000FF;
    UART2->ISR = 0x000000FF;

    DMA0->DMA_INT = 0x00030000;
    DMA1->DMA_INT = 0x00030000;

    IRM->INT_CLR.reg = 0x07;

    PWM0->PWM_INT_CLEAR = 0x000000FF;
    PWM1->PWM_INT_CLEAR = 0x000000FF;
    PWM2->PWM_INT_CLEAR = 0x000000FF;
    PWM3->PWM_INT_CLEAR = 0x000000FF;
    PWM4->PWM_INT_CLEAR = 0x000000FF;

    TIMER0->CLEAR = 1;
    TIMER1->CLEAR = 1;
    TIMER2->CLEAR = 1;

    TIMER32K0->CLEAR = 1;
    TIMER32K1->CLEAR = 1;
    TIMER32K1->CLEAR = 1;

    RTC->RTC_INT_CLEAR = 0x000000FF;

    SW_INT0->CLEAR_IRQ = 0xFFFFFFFF;
    SW_INT1->CLEAR_IRQ = 0xFFFFFFFF;

    BOD_COMP->COMP_DIG_CTRL1.bit.CLR_INTR_COUNTER = BOD_COMP->COMP_DIG_CTRL2.bit.STA_INTR_COUNTER;
    BOD_COMP->COMP_DIG_CTRL1.bit.CLR_INTR_FALLING = BOD_COMP->COMP_DIG_CTRL2.bit.STA_INTR_FALLING;
    BOD_COMP->COMP_DIG_CTRL1.bit.CLR_INTR_RISING = BOD_COMP->COMP_DIG_CTRL2.bit.STA_INTR_RISING;

    AUX_COMP->COMP_DIG_CTRL1.bit.CLR_INTR_COUNTER = AUX_COMP->COMP_DIG_CTRL2.bit.STA_INTR_COUNTER;
    AUX_COMP->COMP_DIG_CTRL1.bit.CLR_INTR_FALLING = AUX_COMP->COMP_DIG_CTRL2.bit.STA_INTR_FALLING;
    AUX_COMP->COMP_DIG_CTRL1.bit.CLR_INTR_RISING = AUX_COMP->COMP_DIG_CTRL2.bit.STA_INTR_RISING;

    CRYPTO->CRYPTO_CFG.bit.clr_crypto_int = 1;

    I2C_MASTER0->INT_CLEAR = 0x000001FF;
    I2C_MASTER1->INT_CLEAR = 0x000001FF;

    I2C_SLAVE->I2C_INT_STATUS = 0x0000001F;

    I2S_MASTER->I2S_INT_CLEAR = 0x0000000F;

    QSPI0->QSPI_INT_CLR = 0x0000003F;
    QSPI1->QSPI_INT_CLR = 0x0000003F;

    SADC->SADC_INT_CLEAR.reg = 0x0FFFFFFF;

    COMM_SUBSYSTEM_AHB->COMM_SUBSYSTEM_INTR_CLR = 0x000001FF;

    WDT->CLEAR = 1;
    
    SCB->ICSR |= SCB_ICSR_PENDSVCLR_Msk;
}

/**
* @brief set mcu enter low power mode
*/
void Lpm_Enter_Low_Power_Mode(void)
{

    if (low_power_mask_status == LOW_POWER_NO_MASK)
    {
        if (low_power_level == LPM_SLEEP)
        {
            Lpm_Set_Platform_Low_Power_Wakeup(LOW_POWER_PLATFORM_ENTER_SLEEP);                 /* set platform system wakeup source when entering sleep mode*/

            SYSCTRL->SYS_POWER_STATE.bit.CFG_SET_LOWPOWER = LOW_POWER_PLATFORM_ENTER_SLEEP;                          /* platform system enter sleep mode */
            __WFI();
        }
        else if (low_power_level == LPM_DEEP_SLEEP)
        {
            Lpm_Deep_Sleep_Wakeup_Fastboot();
            Lpm_Deep_Sleep_Set_RCO_Settle_Time();
            Lpm_Set_Platform_Low_Power_Wakeup(LOW_POWER_PLATFORM_ENTER_DEEP_SLEEP);        /* set platform system wakeup source when entering deep sleep mode*/
            while (1)
            {
                Lpm_Deep_Sleep_Clr_Interrupt();
                SYSCTRL->SYS_POWER_STATE.bit.CFG_SET_LOWPOWER = LOW_POWER_PLATFORM_ENTER_DEEP_SLEEP; /* platform system enter deep sleep mode */
                __WFI();
            }
        }
        else if (low_power_level == LPM_POWER_DOWN)
        {
            Lpm_Set_Platform_Low_Power_Wakeup(LOW_POWER_PLATFORM_ENTER_POWER_DOWN_MODE);        /* set platform system wakeup source when entering power down mode*/
            SYSCTRL->SYS_POWER_STATE.bit.CFG_SET_LOWPOWER = LOW_POWER_PLATFORM_ENTER_POWER_DOWN_MODE;               /* platform system enter power down mode */
            __WFI();


        }

        SYSCTRL->SYS_POWER_STATE.bit.CFG_SET_LOWPOWER = LOW_POWER_PLATFORM_NORMAL;
    }
}




