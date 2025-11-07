/**
 ******************************************************************************
 * @file    sysctrl.c
 * @author
 * @brief   system control driver file
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

/**************************************************************************************************
 *    INCLUDES
 *************************************************************************************************/
#include "sysctrl.h"
#include "gpio.h"
#include "project_config.h"
#include "pin_mux_define.h"

/**************************************************************************************************
 *    GLOBAL FUNCTIONS
 *************************************************************************************************/
__attribute__((optimize("-O3"))) void __Delay_us(uint64_t us)
{
    while (us != 0)
    {
        us--;
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
        __NOP();
    }
}

/**
 * @brief delay us
 */
void Delay_us(unsigned int us)
{
    __Delay_us(us);
}
/**
* @brief delay ms
*/
void Delay_ms(unsigned int ms)
{
    __Delay_us(ms * 1000);
}

/**************************************************************************************************
 *    GLOBAL FUNCTIONS
 *************************************************************************************************/
/**
* @brief delay initinal
*/
void Delay_Init(void)
{

}

/**
* @brief pin set mode
*/
void Pin_Set_Mode(uint32_t pin_number, uint32_t mode)
{
    /*
     * avoid to set pin 10 & 11 these two is SWD/SWI
     */
    if ((pin_number >= 32) || (pin_number == 10) || (pin_number == 11) || (mode > 15))
    {
        return;     /*Invalid setting mode.*/
    }

    if ( rt58x_pin_mux_define[pin_number][mode][1] != MODE_PIN_MUX_NULL)
    {
        if ( rt58x_pin_mux_define[pin_number][mode][2] != MODE_PIN_MUX_NULL )
        {
            Pin_Set_Out_Mode_Ex(pin_number, rt58x_pin_mux_define[pin_number][mode][2]);
        }
        if ( rt58x_pin_mux_define[pin_number][mode][3] != MODE_PIN_MUX_NULL )
        {
            Pin_Set_In_Mode_Ex(pin_number, rt58x_pin_mux_define[pin_number][mode][3]);
        }
    }
    return;
}

void Pin_Set_Mode_Ex(uint32_t pin_number, uint32_t mode)
{
    /*
     * avoid to set pin 10 & 11 these two is SWD/SWI
     */
    if ((pin_number >= 32) || (pin_number == 10) || (pin_number == 11) || (mode >= MODE_MAX_EX))
    {
        return;     /*Invalid setting mode.*/
    }
    switch (mode)
    {
    case MODE_GPIO_EX:
        Pin_Set_Out_Mode_Ex(pin_number, MODE_GPIO_REG_VALUE);
        Pin_Set_Pullopt(pin_number, PULL_NONE);
        break;

    case MODE_UART0_TX_EX:
        Pin_Set_Out_Mode_Ex(pin_number, MODE_UART0_TX_REG_VALUE);
        Pin_Set_Pullopt(pin_number, PULL_NONE);
        Gpio_Cfg_Output(pin_number);
        break;

    case MODE_UART0_RX_EX:
        Pin_Set_In_Mode_Ex(pin_number, MODE_UART0_RX_REG_VALUE);
        break;

    case MODE_UART1_TX_EX:
        Pin_Set_Out_Mode_Ex(pin_number, MODE_UART1_TX_REG_VALUE);
        Pin_Set_Pullopt(pin_number, PULL_NONE);
        Gpio_Cfg_Output(pin_number);
        break;

    case MODE_UART1_RX_EX:
        Pin_Set_In_Mode_Ex(pin_number, MODE_UART1_RX_REG_VALUE);
        break;

    case MODE_UART1_RTSN_EX:
        Pin_Set_Out_Mode_Ex(pin_number, MODE_UART1_RTSN_REG_VALUE);
        Pin_Set_Pullopt(pin_number, PULL_NONE);
        Gpio_Cfg_Output(pin_number);
        break;

    case MODE_UART1_CTSN_EX:
        Pin_Set_In_Mode_Ex(pin_number, MODE_UART1_CTSN_REG_VALUE);
        break;

    case MODE_UART2_TX_EX:
        Pin_Set_Out_Mode_Ex(pin_number, MODE_UART2_TX_REG_VALUE);
        Pin_Set_Pullopt(pin_number, PULL_NONE);
        Gpio_Cfg_Output(pin_number);
        break;

    case MODE_UART2_RX_EX:
        Pin_Set_In_Mode_Ex(pin_number, MODE_UART2_RX_REG_VALUE);
        break;

    case MODE_UART2_RTSN_EX:
        Pin_Set_Out_Mode_Ex(pin_number, MODE_UART2_RTSN_REG_VALUE);
        Pin_Set_Pullopt(pin_number, PULL_NONE);
        Gpio_Cfg_Output(pin_number);
        break;

    case MODE_UART2_CTSN_EX:
        Pin_Set_In_Mode_Ex(pin_number, MODE_UART2_CTSN_REG_VALUE);
        break;

    case MODE_PWM0_EX:
        Pin_Set_Out_Mode_Ex(pin_number, MODE_PWM0_REG_VALUE);
        Pin_Set_Pullopt(pin_number, PULL_NONE);
        Gpio_Cfg_Output(pin_number);
        break;

    case MODE_PWM1_EX:
        Pin_Set_Out_Mode_Ex(pin_number, MODE_PWM1_REG_VALUE);
        Pin_Set_Pullopt(pin_number, PULL_NONE);
        Gpio_Cfg_Output(pin_number);
        break;

    case MODE_PWM2_EX:
        Pin_Set_Out_Mode_Ex(pin_number, MODE_PWM2_REG_VALUE);
        Pin_Set_Pullopt(pin_number, PULL_NONE);
        Gpio_Cfg_Output(pin_number);
        break;

    case MODE_PWM3_EX:
        Pin_Set_Out_Mode_Ex(pin_number, MODE_PWM3_REG_VALUE);
        Pin_Set_Pullopt(pin_number, PULL_NONE);
        Gpio_Cfg_Output(pin_number);
        break;

    case MODE_PWM4_EX:
        Pin_Set_Out_Mode_Ex(pin_number, MODE_PWM4_REG_VALUE);
        Pin_Set_Pullopt(pin_number, PULL_NONE);
        Gpio_Cfg_Output(pin_number);
        break;

    case MODE_IRM_EX:
        Pin_Set_Out_Mode_Ex(pin_number, MODE_IRM_REG_VALUE);
        Pin_Set_Pullopt(pin_number, PULL_NONE);
        Gpio_Cfg_Output(pin_number);
        break;

    case MODE_I2CM0_SCL_EX:
        Pin_Set_Out_Mode_Ex(pin_number, MODE_I2CM0_SCL_OUT_REG_VALUE);
        Pin_Set_In_Mode_Ex(pin_number, MODE_I2CM0_SCL_REG_VALUE);
        break;

    case MODE_I2CM0_SDA_EX:
        Pin_Set_Out_Mode_Ex(pin_number, MODE_I2CM0_SDA_OUT_REG_VALUE);
        Pin_Set_In_Mode_Ex(pin_number, MODE_I2CM0_SDA_REG_VALUE);
        break;

    case MODE_I2CM1_SCL_EX:
        Pin_Set_Out_Mode_Ex(pin_number, MODE_I2CM1_SCL_OUT_REG_VALUE);
        Pin_Set_In_Mode_Ex(pin_number, MODE_I2CM1_SCL_REG_VALUE);
        break;

    case MODE_I2CM1_SDA_EX:
        Pin_Set_Out_Mode_Ex(pin_number, MODE_I2CM1_SDA_OUT_REG_VALUE);
        Pin_Set_In_Mode_Ex(pin_number, MODE_I2CM1_SDA_REG_VALUE);
        break;

    case MODE_I2CS_SCL_EX:
        Pin_Set_Out_Mode_Ex(pin_number, MODE_I2CS_SCL_OUT_REG_VALUE);
        Pin_Set_In_Mode_Ex(pin_number, MODE_I2CS_SCL_REG_VALUE);
        break;

    case MODE_I2CS_SDA_EX:
        Pin_Set_Out_Mode_Ex(pin_number, MODE_I2CS_SDA_OUT_REG_VALUE);
        Pin_Set_In_Mode_Ex(pin_number, MODE_I2CS_SDA_REG_VALUE);
        break;


    case MODE_SPI0_MASTER_SCLK_EX:
        Pin_Set_Out_Mode_Ex(pin_number, MODE_SPI0_SCLK_OUT_REG_VALUE);
        Pin_Set_Pullopt(pin_number, PULL_NONE);
        Gpio_Cfg_Output(pin_number);
        break;

    case MODE_SPI0_MASTER_CSN0_EX:
        Pin_Set_Out_Mode_Ex(pin_number, MODE_SPI0_CSN0_REG_VALUE);
        Pin_Set_Pullopt(pin_number, PULL_NONE);
        Gpio_Cfg_Output(pin_number);
        break;

    case MODE_SPI0_MASTER_MOSI_EX:
        Pin_Set_Out_Mode_Ex(pin_number, MODE_SPI0_SDATA_OUT0_REG_VALUE);
        Pin_Set_Pullopt(pin_number, PULL_NONE);
        Gpio_Cfg_Output(pin_number);
        break;

    case MODE_SPI0_MASTER_MISO_EX:
        Pin_Set_In_Mode_Ex(pin_number, MODE_QSPI0_SDATA1_REG_VALUE);
        break;

    case MODE_SPI0_MASTER_SDATA0_EX:
        Pin_Set_Out_Mode_Ex(pin_number, MODE_SPI0_SDATA_OUT0_REG_VALUE);
        Pin_Set_In_Mode_Ex(pin_number, MODE_QSPI0_SDATA0_REG_VALUE);
        break;

    case MODE_SPI0_MASTER_SDATA1_EX:
        Pin_Set_Out_Mode_Ex(pin_number, MODE_SPI0_SDATA_OUT1_REG_VALUE);
        Pin_Set_In_Mode_Ex(pin_number, MODE_QSPI0_SDATA1_REG_VALUE);
        break;

    case MODE_SPI0_MASTER_SDATA2_EX:
        Pin_Set_Out_Mode_Ex(pin_number, MODE_SPI0_SDATA_OUT2_REG_VALUE);
        Pin_Set_In_Mode_Ex(pin_number, MODE_QSPI0_SDATA2_REG_VALUE);
        break;

    case MODE_SPI0_MASTER_SDATA3_EX:
        Pin_Set_Out_Mode_Ex(pin_number, MODE_SPI0_SDATA_OUT3_REG_VALUE);
        Pin_Set_In_Mode_Ex(pin_number, MODE_QSPI0_SDATA3_REG_VALUE);
        break;


    case MODE_SPI0_SLAVE_SCLK_EX:
        Pin_Set_In_Mode_Ex(pin_number, MODE_QSPI0_SCLK_REG_VALUE);
        break;

    case MODE_SPI0_SLAVE_CSN0_EX:
        Pin_Set_In_Mode_Ex(pin_number, MODE_QSPI0_CSN_REG_VALUE);
        break;

    case MODE_SPI0_SLAVE_MOSI_EX:
        Pin_Set_In_Mode_Ex(pin_number, MODE_QSPI0_SDATA0_REG_VALUE);
        break;

    case MODE_SPI0_SLAVE_MISO_EX:
        Pin_Set_Out_Mode_Ex(pin_number, MODE_SPI0_SDATA_OUT1_REG_VALUE);
        Pin_Set_Pullopt(pin_number, PULL_NONE);
        Gpio_Cfg_Output(pin_number);
        break;




    case MODE_SPI1_MASTER_SCLK_EX:
        Pin_Set_Out_Mode_Ex(pin_number, MODE_SPI1_SCLK_OUT_REG_VALUE);
        Pin_Set_Pullopt(pin_number, PULL_NONE);
        Gpio_Cfg_Output(pin_number);
        break;

    case MODE_SPI1_MASTER_CSN0_EX:
        Pin_Set_Out_Mode_Ex(pin_number, MODE_SPI1_CSN0_REG_VALUE);
        Pin_Set_Pullopt(pin_number, PULL_NONE);
        Gpio_Cfg_Output(pin_number);
        break;

    case MODE_SPI1_MASTER_MOSI_EX:
        Pin_Set_Out_Mode_Ex(pin_number, MODE_SPI1_SDATA_OUT0_REG_VALUE);
        Pin_Set_Pullopt(pin_number, PULL_NONE);
        Gpio_Cfg_Output(pin_number);
        break;

    case MODE_SPI1_MASTER_MISO_EX:
        Pin_Set_In_Mode_Ex(pin_number, MODE_QSPI1_SDATA1_REG_VALUE);
        break;

    case MODE_SPI1_MASTER_SDATA0_EX:
        Pin_Set_Out_Mode_Ex(pin_number, MODE_SPI0_SDATA_OUT0_REG_VALUE);
        Pin_Set_In_Mode_Ex(pin_number, MODE_QSPI0_SDATA0_REG_VALUE);
        break;

    case MODE_SPI1_MASTER_SDATA1_EX:
        Pin_Set_Out_Mode_Ex(pin_number, MODE_SPI0_SDATA_OUT1_REG_VALUE);
        Pin_Set_In_Mode_Ex(pin_number, MODE_QSPI0_SDATA1_REG_VALUE);
        break;

    case MODE_SPI1_MASTER_SDATA2_EX:
        Pin_Set_Out_Mode_Ex(pin_number, MODE_SPI0_SDATA_OUT2_REG_VALUE);
        Pin_Set_In_Mode_Ex(pin_number, MODE_QSPI0_SDATA2_REG_VALUE);
        break;

    case MODE_SPI1_MASTER_SDATA3_EX:
        Pin_Set_Out_Mode_Ex(pin_number, MODE_SPI0_SDATA_OUT3_REG_VALUE);
        Pin_Set_In_Mode_Ex(pin_number, MODE_QSPI0_SDATA3_REG_VALUE);
        break;

    case MODE_SPI1_SLAVE_SCLK_EX:
        Pin_Set_In_Mode_Ex(pin_number, MODE_QSPI1_SCLK_REG_VALUE);
        break;

    case MODE_SPI1_SLAVE_CSN0_EX:
        Pin_Set_In_Mode_Ex(pin_number, MODE_QSPI1_CSN_REG_VALUE);
        break;

    case MODE_SPI1_SLAVE_MOSI_EX:
        Pin_Set_In_Mode_Ex(pin_number, MODE_QSPI1_SDATA0_REG_VALUE);
        break;

    case MODE_SPI1_SLAVE_MISO_EX:
        Pin_Set_Out_Mode_Ex(pin_number, MODE_SPI1_SDATA_OUT1_REG_VALUE);
        Pin_Set_Pullopt(pin_number, PULL_NONE);
        Gpio_Cfg_Output(pin_number);
        break;

    case MODE_SPI0_MASTER_CSN1_EX:
        Pin_Set_Out_Mode_Ex(pin_number, MODE_SPI0_CSN1_REG_VALUE);
        Pin_Set_Pullopt(pin_number, PULL_NONE);
        Gpio_Cfg_Output(pin_number);
        break;

    case MODE_SPI0_MASTER_CSN2_EX:
        Pin_Set_Out_Mode_Ex(pin_number, MODE_SPI0_CSN2_REG_VALUE);
        Pin_Set_Pullopt(pin_number, PULL_NONE);
        Gpio_Cfg_Output(pin_number);
        break;

    case MODE_SPI0_MASTER_CSN3_EX:
        Pin_Set_Out_Mode_Ex(pin_number, MODE_SPI0_CSN3_REG_VALUE);
        Pin_Set_Pullopt(pin_number, PULL_NONE);
        Gpio_Cfg_Output(pin_number);
        break;

    case MODE_SPI1_MASTER_CSN1_EX:
        Pin_Set_Out_Mode_Ex(pin_number, MODE_SPI1_CSN1_REG_VALUE);
        Pin_Set_Pullopt(pin_number, PULL_NONE);
        Gpio_Cfg_Output(pin_number);
        break;

    case MODE_SPI1_MASTER_CSN2_EX:
        Pin_Set_Out_Mode_Ex(pin_number, MODE_SPI1_CSN2_REG_VALUE);
        Pin_Set_Pullopt(pin_number, PULL_NONE);
        Gpio_Cfg_Output(pin_number);
        break;

    case MODE_SPI1_MASTER_CSN3_EX:
        Pin_Set_Out_Mode_Ex(pin_number, MODE_SPI1_CSN3_REG_VALUE);
        Pin_Set_Pullopt(pin_number, PULL_NONE);
        Gpio_Cfg_Output(pin_number);
        break;


    case MODE_SPI0_SLAVE_CSN1_EX:
        Pin_Set_In_Mode_Ex(pin_number, MODE_QSPI0_CSN_REG_VALUE);
        break;

    case MODE_SPI0_SLAVE_CSN2_EX:
        Pin_Set_In_Mode_Ex(pin_number, MODE_QSPI0_CSN_REG_VALUE);
        break;

    case MODE_SPI0_SLAVE_CSN3_EX:
        Pin_Set_In_Mode_Ex(pin_number, MODE_QSPI0_CSN_REG_VALUE);
        break;

    case MODE_SPI1_SLAVE_CSN1_EX:
        Pin_Set_In_Mode_Ex(pin_number, MODE_QSPI1_CSN_REG_VALUE);
        break;

    case MODE_SPI1_SLAVE_CSN2_EX:
        Pin_Set_In_Mode_Ex(pin_number, MODE_QSPI1_CSN_REG_VALUE);
        break;

    case MODE_SPI1_SLAVE_CSN3_EX:
        Pin_Set_In_Mode_Ex(pin_number, MODE_QSPI1_CSN_REG_VALUE);
        break;

    case MODE_I2S_BCK_EX:
        Pin_Set_Out_Mode_Ex(pin_number, MODE_I2S_BCK_REG_VALUE);
        Pin_Set_Pullopt(pin_number, PULL_NONE);
        Gpio_Cfg_Output(pin_number);
        break;

    case MODE_I2S_WCK_EX:
        Pin_Set_Out_Mode_Ex(pin_number, MODE_I2S_WCK_REG_VALUE);
        Pin_Set_Pullopt(pin_number, PULL_NONE);
        Gpio_Cfg_Output(pin_number);
        break;

    case MODE_I2S_SDO_EX:
        Pin_Set_Out_Mode_Ex(pin_number, MODE_I2S_SDO_REG_VALUE);
        Pin_Set_Pullopt(pin_number, PULL_NONE);
        Gpio_Cfg_Output(pin_number);
        break;

    case MODE_I2S_SDI_EX:
        Pin_Set_In_Mode_Ex(pin_number, MODE_I2S_SDI_REG_VALUE);
        break;

    case MODE_I2S_MCLK_EX:
        Pin_Set_Out_Mode_Ex(pin_number, MODE_I2S_MCLK_REG_VALUE);
        Pin_Set_Pullopt(pin_number, PULL_NONE);
        Gpio_Cfg_Output(pin_number);
        break;

    case MODE_SWD_EX:
        Pin_Set_Out_Mode_Ex(pin_number, MODE_SWD_REG_VALUE);
        Pin_Set_Pullopt(pin_number, PULL_NONE);
        Gpio_Cfg_Output(pin_number);
        break;

    case MODE_DBG0_EX:
        Pin_Set_Out_Mode_Ex(pin_number, MODE_DBG0_REG_VALUE);
        Pin_Set_Pullopt(pin_number, PULL_NONE);
        Gpio_Cfg_Output(pin_number);
        break;

    case MODE_DBG1_EX:
        Pin_Set_Out_Mode_Ex(pin_number, MODE_DBG1_REG_VALUE);
        Pin_Set_Pullopt(pin_number, PULL_NONE);
        Gpio_Cfg_Output(pin_number);
        break;

    case MODE_DBG2_EX:
        Pin_Set_Out_Mode_Ex(pin_number, MODE_DBG2_REG_VALUE);
        Pin_Set_Pullopt(pin_number, PULL_NONE);
        Gpio_Cfg_Output(pin_number);
        break;

    case MODE_DBG3_EX:
        Pin_Set_Out_Mode_Ex(pin_number, MODE_DBG3_REG_VALUE);
        Pin_Set_Pullopt(pin_number, PULL_NONE);
        Gpio_Cfg_Output(pin_number);
        break;

    case MODE_DBG4_EX:
        Pin_Set_Out_Mode_Ex(pin_number, MODE_DBG4_REG_VALUE);
        Pin_Set_Pullopt(pin_number, PULL_NONE);
        Gpio_Cfg_Output(pin_number);
        break;

    case MODE_DBG5_EX:
        Pin_Set_Out_Mode_Ex(pin_number, MODE_DBG5_REG_VALUE);
        Pin_Set_Pullopt(pin_number, PULL_NONE);
        Gpio_Cfg_Output(pin_number);
        break;

    case MODE_DBG6_EX:
        Pin_Set_Out_Mode_Ex(pin_number, MODE_DBG6_REG_VALUE);
        Pin_Set_Pullopt(pin_number, PULL_NONE);
        Gpio_Cfg_Output(pin_number);
        break;

    case MODE_DBG7_EX:
        Pin_Set_Out_Mode_Ex(pin_number, MODE_DBG7_REG_VALUE);
        Pin_Set_Pullopt(pin_number, PULL_NONE);
        Gpio_Cfg_Output(pin_number);
        break;

    case MODE_DBG8_EX:
        Pin_Set_Out_Mode_Ex(pin_number, MODE_DBG8_REG_VALUE);
        Pin_Set_Pullopt(pin_number, PULL_NONE);
        Gpio_Cfg_Output(pin_number);
        break;

    case MODE_DBG9_EX:
        Pin_Set_Out_Mode_Ex(pin_number, MODE_DBG9_REG_VALUE);
        Pin_Set_Pullopt(pin_number, PULL_NONE);
        Gpio_Cfg_Output(pin_number);
        break;

    case MODE_DBGA_EX:
        Pin_Set_Out_Mode_Ex(pin_number, MODE_DBGA_REG_VALUE);
        Pin_Set_Pullopt(pin_number, PULL_NONE);
        Gpio_Cfg_Output(pin_number);
        break;

    case MODE_DBGB_EX:
        Pin_Set_Out_Mode_Ex(pin_number, MODE_DBGB_REG_VALUE);
        Pin_Set_Pullopt(pin_number, PULL_NONE);
        Gpio_Cfg_Output(pin_number);
        break;

    case MODE_DBGC_EX:
        Pin_Set_Out_Mode_Ex(pin_number, MODE_DBGC_REG_VALUE);
        Pin_Set_Pullopt(pin_number, PULL_NONE);
        Gpio_Cfg_Output(pin_number);
        break;

    case MODE_DBGD_EX:
        Pin_Set_Out_Mode_Ex(pin_number, MODE_DBGD_REG_VALUE);
        Pin_Set_Pullopt(pin_number, PULL_NONE);
        Gpio_Cfg_Output(pin_number);
        break;

    case MODE_DBGE_EX:
        Pin_Set_Out_Mode_Ex(pin_number, MODE_DBGE_REG_VALUE);
        Pin_Set_Pullopt(pin_number, PULL_NONE);
        Gpio_Cfg_Output(pin_number);
        break;

    case MODE_DBGF_EX:
        Pin_Set_Out_Mode_Ex(pin_number, MODE_DBGF_REG_VALUE);
        Pin_Set_Pullopt(pin_number, PULL_NONE);
        Gpio_Cfg_Output(pin_number);
        break;
    }

    return;
}

void Pin_Set_Out_Mode_Ex(uint32_t pin_number, uint32_t mode_value)
{
    uint32_t base, reg_shift;
    uint32_t index;

    /*RT584 mode become 4 bits.. */
    if ( pin_number > 32 )
    {
        return;     /*Invalid setting mode.*/
    }

    index = (pin_number / 4);
    base  = (pin_number % 4);
    reg_shift = (base << 3);

    SYSCTRL->SYS_GPIO_OMUX[index] = (SYSCTRL->SYS_GPIO_OMUX[index] & ~ (0x3F << reg_shift));
    SYSCTRL->SYS_GPIO_OMUX[index] =  SYSCTRL->SYS_GPIO_OMUX[index] |  (mode_value << reg_shift);

    return;
}


//0x10000010

void Pin_Set_In_Mode_Ex(uint32_t pin_number, uint32_t mode_value)
{
    uint32_t reg_shift, enable;
    uint32_t index;

    /*RT584 mode become 4 bits..*/
    if ( pin_number > 32 )
    {
        return;     /*Invalid setting mode.*/
    }


    index = 0;
    reg_shift = 0;

    index  = (mode_value >> 28 & 0x07);
    reg_shift = (mode_value & 0x18);
    enable = ((mode_value & 0x18) + 7);

    SYSCTRL->SYS_GPIO_IMUX[index] = (SYSCTRL->SYS_GPIO_IMUX[index] & ~(0x3F << reg_shift));
    SYSCTRL->SYS_GPIO_IMUX[index] = (SYSCTRL->SYS_GPIO_IMUX[index] | (pin_number << reg_shift));
    SYSCTRL->SYS_GPIO_IMUX[index] = (SYSCTRL->SYS_GPIO_IMUX[index] | (1 << enable));

    return;
}

/**
* @brief pin get mode
*/
uint32_t Pin_Get_Mode(uint32_t __attribute__((__unused__)) pin_number)
{
    return 0;
}

/* Notic
 * For multithread OS, and dynamic enable/disable peripheral environment,
 * Set this register has race condition issue.
 * so we need critical section protect.
 *
 */
void Enable_Perclk(uint32_t clock)
{
    if ((clock < UART0_CLK) || (clock > GPIO_32K_CLK))
    {
        return;     /*Invalid setting mode.*/
    }

    SYSCTRL->SYS_CLK_CTRL.reg |= (1 << clock) ;
}

/*
 * For multithread OS, and dynamic enable/disable peripheral environment,
 * Set this register has race condition issue,
 *  so we need critical section protect.
 *
 */
void Disable_Perclk(uint32_t clock)
{
    if ((clock < UART0_CLK) || (clock > GPIO_32K_CLK))
    {
        return;     /*Invalid setting mode.*/
    }

    SYSCTRL->SYS_CLK_CTRL.reg  &= ~(1 << clock);
}

/*
 * For multithread OS, and dynamic enable/disable peripheral environment,
 * Set this register has race condition issue, (in fact, almost impossible)
 *  so we need critical section protect.
 *
 */
void Pin_Set_Pullopt(uint32_t pin_number, uint32_t mode)
{
    uint32_t reg, base, mask_offset, mask;

    if ((pin_number >= 32) || (mode > 7))
    {
        return;     /*Invalid setting mode.*/
    }

    base = PULLOPT_BASE + (pin_number >> 3) * 4;
    mask_offset = (pin_number & 0x7) << 2;
    mask = 0xF << mask_offset;


    /*pin mux setting is share resource.*/
    reg = *((volatile unsigned int *) base);
    reg = reg & ~mask;
    reg = reg | (mode << mask_offset);

    *((volatile unsigned int *)base) =  reg;

    return;
}

/*
 * For multithread OS, and dynamic enable/disable peripheral environment,
 * Set this register has race condition issue, (in fact, almost impossible)
 *  so we need critical section protect.
 *
 */
void Pin_Set_Drvopt(uint32_t pin_number, uint32_t mode)
{
    uint32_t reg, base, mask_offset, mask;

    if ((pin_number >= 32) || (mode > 3))
    {
        return;     /*Invalid setting mode.*/
    }

    base = DRV_BASE + (pin_number >> 4) * 4;
    mask_offset = (pin_number & 0xF) << 1;
    mask = 0x3 << mask_offset;

    /*pin mux setting is share resource.*/
    reg = *((volatile unsigned int *) base);
    reg = reg & ~mask;
    reg = reg | (mode << mask_offset);

    *((volatile unsigned int *)base) =  reg;

    return;
}

/*
 * For multithread OS, and dynamic enable/disable peripheral environment,
 * Set this register has race condition issue, so we need critical section protect.
 * In fact, it is almost impossible to dynamic change open drain.
 *
 */
void Enable_Pin_Opendrain(uint32_t pin_number)
{
    uint32_t base, mask, reg;

    if (pin_number >= 32)
    {
        return;     /*Invalid setting mode.*/
    }

    base = OD_BASE ;
    mask = 1 << pin_number ;

    /*pin mux setting is share resource.*/
    reg = *((volatile unsigned int *) base);
    reg = reg | mask;
    *((volatile unsigned int *)base) =  reg;

    return;
}

/*
 * For multithread OS, and dynamic enable/disable peripheral environment,
 * Set this register has race condition issue, so we need critical section protect.
 * In fact, it is almost impossible to dynamic change open drain.
 *
 */
void Disable_Pin_Opendrain(uint32_t pin_number)
{
    uint32_t base, mask, reg;

    if (pin_number >= 32)
    {
        return;     /*Invalid setting mode.*/
    }

    base = OD_BASE ;
    mask = ~(1 << pin_number);

    /*pin mux setting is share resource.*/
    reg = *((volatile unsigned int *) base);
    reg = reg & mask;
    *((volatile unsigned int *)base) =  reg;

    return;
}
/**
* @brief change ahb system clock
*/
uint32_t Change_Ahb_System_Clk(sys_clk_sel_t sys_clk_mode)
{
    uint32_t i = 0;
    uint32_t pll_clk = 0;

    if (sys_clk_mode > SYS_CLK_MAX)                                         /*Invalid parameter*/
    {
        return STATUS_ERROR;
    }

    SYSCTRL->SYS_CLK_CTRL.reg = 0;                                      /*Set MCU clock source to default 32MHZ*/

    if ( sys_clk_mode <= SYS_RCO1MHZ_CLK)
    {
        SYSCTRL->SYS_CLK_CTRL.bit.CFG_BBPLL_EN = 0;

        if ( (sys_clk_mode) == SYS_RCO1MHZ_CLK )
        {
            PMU_CTRL->SOC_PMU_RCO1M.bit.EN_RCO_1M = 1;
        }

        SYSCTRL->SYS_CLK_CTRL.reg = (SYSCTRL->SYS_CLK_CTRL.reg & ~MCU_HCLK_SEL_MASK);
        SYSCTRL->SYS_CLK_CTRL.reg = (SYSCTRL->SYS_CLK_CTRL.reg & ~MCU_HCLK_SEL_MASK) | (sys_clk_mode << MCU_HCLK_SEL_SHFT);
    }
    else
    {
        pll_clk = (sys_clk_mode & (~SYS_PLL_CLK_OFFSET));

        SYSCTRL->SYS_CLK_CTRL.reg = (SYSCTRL->SYS_CLK_CTRL.reg & ~MCU_BBPLL_CLK_MASK) | (pll_clk << MCU_BBPLL_CLK_SHIFT);
        SYSCTRL->SYS_CLK_CTRL.reg = (SYSCTRL->SYS_CLK_CTRL.reg | MCU_BBPLL_ENABLE);      /*config BASEBAND_PLL_ENABLE*/

        /*baseband pll enable wait delay time 200us*/
        for (i = 0; i < PLL_WAIT_PERIOD; i++)
        {
            __NOP();
        }

        SYSCTRL->SYS_CLK_CTRL.reg = (SYSCTRL->SYS_CLK_CTRL.reg & ~MCU_HCLK_SEL_MASK) | HCLK_SEL_PLL;
    }

    return STATUS_SUCCESS;
}
/**
* @brief get ahb system clock
*/
uint32_t Get_Ahb_System_Clk(void)
{
    uint32_t clk_mode = 0;

    clk_mode = ((SYSCTRL->SYS_CLK_CTRL.reg & MCU_HCLK_SEL_MASK) >> MCU_HCLK_SEL_SHFT);

    if (clk_mode == HCLK_SEL_PLL)
    {
        clk_mode = ((SYSCTRL->SYS_CLK_CTRL.reg & MCU_BBPLL_CLK_MASK) >> MCU_BBPLL_CLK_SHIFT);
        clk_mode = clk_mode + SYS_PLL_CLK_OFFSET;
    }
    else if (clk_mode > 3)
    {
        clk_mode = SYS_CLK_MAX;
    }

    return clk_mode;
}
/**
* @brief set peripheral system clock
*/
uint32_t Change_Peri_Clk(perclk_clk_sel_t sys_clk_mode)
{
    if ( sys_clk_mode == PERCLK_SEL_RCO1M )
    {
        PMU_CTRL->SOC_PMU_RCO1M.bit.EN_RCO_1M = 1;
    }

    SYSCTRL->SYS_CLK_CTRL.bit.PER_CLK_SEL = sys_clk_mode;

    return STATUS_SUCCESS;
}
/**
* @brief get peripheral system clock
*/
uint32_t Get_Peri_Clk(void)
{
    uint32_t clk_mode;

    clk_mode = SYSCTRL->SYS_CLK_CTRL.bit.PER_CLK_SEL;

    return clk_mode;
}
/**
* @brief set sloc clock source
*/
void Set_Slow_Clock_Source(uint32_t mode)
{
    /*Slow clock selection.*/

    if (mode > 3)
    {
        return;    /*Invalid mode*/
    }

    SYSCTRL->SYS_CLK_CTRL.bit.SLOW_CLK_SEL = mode;
}
/**
* @brief set sloc clock source
*/
void Slow_Clock_Calibration(slow_clock_select_t rco_select)
{
    if (rco_select == RCO32K_SELECT)
    {

        PMU_CTRL->PMU_OSC32K.bit.TUNE_FINE_RCO_32K = 88;
        PMU_CTRL->PMU_OSC32K.bit.TUNE_COARSE_RCO_32K = 3;
        PMU_CTRL->PMU_OSC32K.bit.PW_BUF_RCO_32K = 3;
        PMU_CTRL->PMU_OSC32K.bit.PW_RCO_32K = 15;
        PMU_CTRL->PMU_OSC32K.bit.RCO_32K_SEL = 1;
        SYSCTRL->SYS_CLK_CTRL2.bit.EN_RCO32K_DIV2 = 0;

        //RCO
        if (Get_Peri_Clk() == PERCLK_SEL_16M)
        {
            RCO32K_CAL->CAL32K_CFG0.bit.CFG_CAL32K_TARGET = 64000; ////per_clk = 16MHz is  64000'd
        }
        else
        {
            RCO32K_CAL->CAL32K_CFG0.bit.CFG_CAL32K_TARGET = 128000; //per_clk = 32MHz is  128000'd
        }

        RCO32K_CAL->CAL32K_CFG1.bit.CFG_CAL32K_LOCK_ERR = 0x20;
        RCO32K_CAL->CAL32K_CFG1.bit.CFG_CAL32K_AVG_COARSE = 1;
        RCO32K_CAL->CAL32K_CFG1.bit.CFG_CAL32K_AVG_FINE = 2;
        RCO32K_CAL->CAL32K_CFG1.bit.CFG_CAL32K_AVG_LOCK = 2;
        RCO32K_CAL->CAL32K_CFG1.bit.CFG_CAL32K_DLY = 0;
        RCO32K_CAL->CAL32K_CFG1.bit.CFG_CAL32K_FINE_GAIN = 10;
        RCO32K_CAL->CAL32K_CFG1.bit.CFG_CAL32K_LOCK_GAIN = 10;
        RCO32K_CAL->CAL32K_CFG1.bit.CFG_CAL32K_TRACK_EN = 1;
        RCO32K_CAL->CAL32K_CFG1.bit.CFG_CAL32K_SKIP_COARSE = 1;
        RCO32K_CAL->CAL32K_CFG1.bit.CFG_CAL32K_BOUND_MODE = 0;
        RCO32K_CAL->CAL32K_CFG1.bit.CFG_32K_RC_SEL = 1;
        RCO32K_CAL->CAL32K_CFG1.bit.EN_CK_CAL32K = 1;
        RCO32K_CAL->CAL32K_CFG0.bit.CFG_CAL32K_EN = 1;

        //offset 0x500060BC
        PMU_CTRL->PMU_SOC_PMU_TIMING.bit.CFG_MV_SETTLE_TIME = 12;   //MV  settle time = 400us (400us for 1.8V, 3.3V 200us)
        PMU_CTRL->PMU_SOC_PMU_TIMING.bit.CFG_LV_SETTLE_TIME = 5;    //LV  settle time = 62.5us
        //offset 0x50006024
        PMU_CTRL->PMU_SOC_PMU_XTAL1.bit.CFG_XTAL_SETTLE_TIME = 31;  //1ms
        //offset 0x50006040
        PMU_CTRL->PMU_RVD0.bit.CFG_XTAL_FAST_TIME = 15;             //0.5ms
        //offset 0x500060BC
        PMU_CTRL->PMU_SOC_PMU_TIMING.bit.CFG_PWRX_SETTLE_TIME = 2;

    }
    else if (rco_select == RCO16K_SELECT)
    {
        PMU_CTRL->PMU_OSC32K.bit.TUNE_FINE_RCO_32K = 88;
        PMU_CTRL->PMU_OSC32K.bit.TUNE_COARSE_RCO_32K = 3;
        PMU_CTRL->PMU_OSC32K.bit.PW_BUF_RCO_32K = 3;
        PMU_CTRL->PMU_OSC32K.bit.PW_RCO_32K = 0;
        PMU_CTRL->PMU_OSC32K.bit.RCO_32K_SEL = 1;
        SYSCTRL->SYS_CLK_CTRL2.bit.EN_CK_DIV_32K = 1;
        SYSCTRL->SYS_CLK_CTRL2.bit.EN_RCO32K_DIV2 = 1;

        if (Get_Peri_Clk() == PERCLK_SEL_16M)
        {
            RCO32K_CAL->CAL32K_CFG0.bit.CFG_CAL32K_TARGET = 64000; //per_clk = 16MHz is  64000'd
        }
        else
        {
            RCO32K_CAL->CAL32K_CFG0.bit.CFG_CAL32K_TARGET = 128000; //per_clk = 32MHz is  128000'd
        }


        RCO32K_CAL->CAL32K_CFG1.bit.CFG_CAL32K_LOCK_ERR = 0x20;
        RCO32K_CAL->CAL32K_CFG1.bit.CFG_CAL32K_AVG_COARSE = 1;
        RCO32K_CAL->CAL32K_CFG1.bit.CFG_CAL32K_AVG_FINE = 2;
        RCO32K_CAL->CAL32K_CFG1.bit.CFG_CAL32K_AVG_LOCK = 2;
        RCO32K_CAL->CAL32K_CFG1.bit.CFG_CAL32K_DLY = 0;
        RCO32K_CAL->CAL32K_CFG1.bit.CFG_CAL32K_FINE_GAIN = 10;
        RCO32K_CAL->CAL32K_CFG1.bit.CFG_CAL32K_LOCK_GAIN = 10;
        RCO32K_CAL->CAL32K_CFG1.bit.CFG_CAL32K_TRACK_EN = 1;
        RCO32K_CAL->CAL32K_CFG1.bit.CFG_CAL32K_SKIP_COARSE = 1;
        RCO32K_CAL->CAL32K_CFG1.bit.CFG_CAL32K_BOUND_MODE = 0;
        RCO32K_CAL->CAL32K_CFG1.bit.CFG_32K_RC_SEL = 1;
        RCO32K_CAL->CAL32K_CFG1.bit.EN_CK_CAL32K = 1;
        RCO32K_CAL->CAL32K_CFG0.bit.CFG_CAL32K_EN = 1;

        //offset 0x500060BC
        PMU_CTRL->PMU_SOC_PMU_TIMING.bit.CFG_MV_SETTLE_TIME = 6;    //MV  settle time = 400us (400us for 1.8V, 3.3V 200us)
        PMU_CTRL->PMU_SOC_PMU_TIMING.bit.CFG_LV_SETTLE_TIME = 2;    //LV  settle time = 150us
        //offset 0x50006024
        PMU_CTRL->PMU_SOC_PMU_XTAL1.bit.CFG_XTAL_SETTLE_TIME = 15;   //slow clk 10k, 1ms
        //offset 0x50006040
        PMU_CTRL->PMU_RVD0.bit.CFG_XTAL_FAST_TIME = 7;              //slow clk 10k, 0.5ms
        //offset 0x500060BC
        PMU_CTRL->PMU_SOC_PMU_TIMING.bit.CFG_PWRX_SETTLE_TIME = 1;
    }
    else if (rco_select == RCO1M_SELECT)
    {
        PMU_CTRL->SOC_PMU_RCO1M.bit.TUNE_FINE_RCO_1M = 70;
        PMU_CTRL->SOC_PMU_RCO1M.bit.TUNE_COARSE_RCO_1M = 11;
        PMU_CTRL->SOC_PMU_RCO1M.bit.PW_RCO_1M = 1;
        PMU_CTRL->SOC_PMU_RCO1M.bit.TEST_RCO_1M = 0;
        PMU_CTRL->SOC_PMU_RCO1M.bit.EN_RCO_1M = 1;      //1:rco1m enable, 0 : rco1m disable
        //RCO1M
        if (Get_Peri_Clk() == PERCLK_SEL_16M)
        {
            RCO1M_CAL->CAL1M_CFG0.bit.CFG_CAL_TARGET  = 0x115C7; //hper_clk = 16MHz is 115C7'h"
        }
        else
        {
            RCO1M_CAL->CAL1M_CFG0.bit.CFG_CAL_TARGET = 0x22B8E; //per_clk = 32MHz is 0x22B8E'
        }

        //per_clk = 16MHz is 115C7'h
        RCO1M_CAL->CAL1M_CFG1.bit.CFG_CAL_LOCK_ERR = 0x20;
        RCO1M_CAL->CAL1M_CFG1.bit.CFG_CAL_AVG_COARSE = 1;
        RCO1M_CAL->CAL1M_CFG1.bit.CFG_CAL_AVG_FINE = 2;
        RCO1M_CAL->CAL1M_CFG1.bit.CFG_CAL_AVG_LOCK = 2;
        RCO1M_CAL->CAL1M_CFG1.bit.CFG_CAL_DLY = 0;
        RCO1M_CAL->CAL1M_CFG1.bit.CFG_CAL_FINE_GAIN = 10;
        RCO1M_CAL->CAL1M_CFG1.bit.CFG_CAL_LOCK_GAIN = 10;
        RCO1M_CAL->CAL1M_CFG1.bit.CFG_CAL_TRACK_EN = 1;
        RCO1M_CAL->CAL1M_CFG1.bit.CFG_CAL_SKIP_COARSE = 1;
        RCO1M_CAL->CAL1M_CFG1.bit.CFG_CAL_BOUND_MODE = 0;
        RCO1M_CAL->CAL1M_CFG1.bit.CFG_TUNE_RCO_SEL = 1;
        RCO1M_CAL->CAL1M_CFG1.bit.EN_CK_CAL = 1;
        RCO1M_CAL->CAL1M_CFG0.bit.CFG_CAL_EN = 1;
    }

    Delay_ms(2);
}

/**
* @brief gpio enable schmitt
*/
void pin_enable_schmitt(uint32_t pin_number)
{

    SYSCTRL->GPIO_EN_SCHMITT |= 0x1 << pin_number;
}
/**
* @brief gpio disable schmitt
*/
void pin_disable_schmitt(uint32_t pin_number)
{

    SYSCTRL->GPIO_EN_SCHMITT &= ~(0x1 << pin_number);
}
/**
* @brief gpio enable filter
*/
void pin_enable_filter(uint32_t pin_number)
{

    SYSCTRL->GPIO_EN_FILTER |= 0x1 << pin_number;
}
/**
* @brief gpio disable filter
*/
void pin_disable_filter(uint32_t pin_number)
{

    SYSCTRL->GPIO_EN_FILTER &= ~(0x1 << pin_number);
}

