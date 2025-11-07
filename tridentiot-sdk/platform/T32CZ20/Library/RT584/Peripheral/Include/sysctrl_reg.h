/**
 ******************************************************************************
 * @file    sysctrl_reg.h
 * @author
 * @brief   system control register header file
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

#ifndef __RT584_SYSCTRL_REG_H__
#define __RT584_SYSCTRL_REG_H__

#if defined ( __CC_ARM   )
#pragma anon_unions
#endif


//0x00
typedef union sysctrl_soc_chip_info_s
{
    struct sysctrl_soc_chip_info_b
    {
        uint32_t CHIP_REV       : 8;
        uint32_t RESERVED           : 8;
        uint32_t CHIP_ID        : 16;
    } bit;
    uint32_t reg;
} sysctrl_soc_chip_info_t;

//0x04
typedef union sysctrl_clk_ctl0_s
{
    struct sysctrl_clk_ctl0_b
    {
        uint32_t HCLK_SEL           : 2;
        uint32_t PER_CLK_SEL        : 2;
        uint32_t RESERVED0          : 2;
        uint32_t SLOW_CLK_SEL       : 2;
        uint32_t CFG_BBPLL_FREQ     : 3;
        uint32_t RESERVED1          : 4;
        uint32_t CFG_BBPLL_EN       : 1;
        uint32_t RESERVED2          : 16;
    } bit;
    uint32_t reg;
} sysctrl_clk_ctl0_t;

//0x08
typedef union sysctrl_clk_ctl1_s
{
    struct sysctrl_clk_ctl1_b
    {
        uint32_t UART0_CLK_SEL      : 2;
        uint32_t UART1_CLK_SEL      : 2;
        uint32_t UART2_CLK_SEL      : 2;
        uint32_t RESERVED0          : 2;
        uint32_t EXT_SLOW_CLK_SEL   : 6;
        uint32_t RESERVED1          : 2;
        uint32_t PWM0_CLK_SEL       : 2;
        uint32_t PWM1_CLK_SEL       : 2;
        uint32_t PWM2_CLK_SEL       : 2;
        uint32_t PWM3_CLK_SEL       : 2;
        uint32_t PWM4_CLK_SEL       : 2;
        uint32_t TIMER0_CLK_SEL     : 2;
        uint32_t TIMER1_CLK_SEL     : 2;
        uint32_t TIMER2_CLK_SEL     : 2;
    } bit;
    uint32_t reg;
} sysctrl_clk_ctl1_t;

//0x0C
typedef union sysctrl_power_state_s
{
    struct sysctrl_power_state_b
    {
        uint32_t CFG_SET_LOWPOWER       : 3;
        uint32_t RESERVED0              : 29;
    } bit;
    uint32_t reg;
} sysctrl_power_state_t;

//0x44
typedef union sysctrl_soc_gpio_en_aio_s
{
    struct sysctrl_soc_gpio_en_aio_b
    {
        uint32_t GPIO_EN_AIO            : 8;
        uint32_t FLASH_SIO_PULL_EN      : 4;
        uint32_t FLASH_DRV_SEL          : 2;
        uint32_t RESERVED0              : 2;
        uint32_t RESERVED1              : 16;
    } bit;
    uint32_t reg;
} sysctrl_soc_gpio_en_aio_t;

//0x48
typedef union sysctrl_cache_ctl_s
{
    struct sysctrl_cache_ctl_b
    {
        uint32_t CACHE_EN               : 1;
        uint32_t CACHE_WAY_1_EN         : 1;
        uint32_t RESERVED0              : 6;
        uint32_t CACHE_WAY_0_CLR        : 1;
        uint32_t CACHE_WAY_1_CLR        : 1;
        uint32_t RESERVED1              : 22;
    } bit;
    uint32_t reg;
} sysctrl_cache_ctl_t;

//0x4C
typedef union sysctrl_soc_pwm_sel_s
{
    struct sysctrl_soc_pwm_sel_b
    {
        uint32_t PWM0_SRC_SEL          : 2;
        uint32_t PWM1_SRC_SEL          : 2;
        uint32_t PWM2_SRC_SEL          : 2;
        uint32_t PWM3_SRC_SEL          : 2;
        uint32_t PWM4_SRC_SEL          : 2;
        uint32_t RESERVED              : 22;
    } bit;
    uint32_t reg;
} sysctrl_soc_pwm_sel_t;

//0x50
typedef union sysctrl_sram_lowpower_0_s
{
    struct sysctrl_sram_lowpower_0_b
    {

        uint32_t  CFG_SRAM_DS_SP_RAM0       : 1;
        uint32_t  CFG_SRAM_DS_SP_RAM1       : 1;
        uint32_t  CFG_SRAM_DS_SP_RAM2       : 1;
        uint32_t  CFG_SRAM_DS_SP_RAM3       : 1;
        uint32_t  CFG_SRAM_DS_SP_RAM4       : 1;
        uint32_t  CFG_SRAM_DS_SP_RAM5       : 1;
        uint32_t  CFG_SRAM_DS_SP_RAM6       : 1;
        uint32_t  RESERVED1                 : 6;
        uint32_t  CFG_SRAM_DS_SP_ROM:       1;
        uint32_t  CFG_SRAM_DS_SP_CACHE_RAM  : 1;
        uint32_t  CFG_SRAM_DS_SP_CRYPTO_RAM : 1;
        uint32_t  CFG_SRAM_DS_DS_RAM0       : 1;
        uint32_t  CFG_SRAM_DS_DS_RAM1       : 1;
        uint32_t  CFG_SRAM_DS_DS_RAM2       : 1;
        uint32_t  CFG_SRAM_DS_DS_RAM3       : 1;
        uint32_t  CFG_SRAM_DS_DS_RAM4       : 1;
        uint32_t  CFG_SRAM_DS_DS_RAM5       : 1;
        uint32_t  CFG_SRAM_DS_SD_RAM6       : 1;
        uint32_t  RESERVED2                 : 6;
        uint32_t  CFG_SRAM_DS_DS_ROM        : 1;
        uint32_t  CFG_SRAM_DS_DS_CACHE_RAM  : 1;
        uint32_t  CFG_SRAM_DS_DS_CRYPTO_RAM : 1;
    } bit;
    uint32_t reg;
} sysctrl_sram_lowpower_0_t;

//0x54
typedef union sysctrl_sram_lowpower_1_s
{
    struct sysctrl_sram_lowpower_1_b
    {
        uint32_t  CFG_SRAM_SD_NM_RAM0       : 1;
        uint32_t  CFG_SRAM_SD_NM_RAM1       : 1;
        uint32_t  CFG_SRAM_SD_NM_RAM2       : 1;
        uint32_t  CFG_SRAM_SD_NM_RAM3       : 1;
        uint32_t  CFG_SRAM_SD_NM_RAM4       : 1;
        uint32_t  CFG_SRAM_SD_NM_RAM5       : 1;
        uint32_t  CFG_SRAM_SD_NM_RAM6       : 1;
        uint32_t  RESERVED1                 : 6;
        uint32_t  CFG_SRAM_SD_NM_ROM        : 1;
        uint32_t  CFG_SRAM_SD_NM_CACHE_RAM  : 1;
        uint32_t  CFG_SRAM_SD_NM_CRYPTO_RAM : 1;
        uint32_t  CFG_SRAM_SD_SP_RAM0       : 1;
        uint32_t  CFG_SRAM_SD_SP_RAM1       : 1;
        uint32_t  CFG_SRAM_SD_SP_RAM2       : 1;
        uint32_t  CFG_SRAM_SD_SP_RAM3       : 1;
        uint32_t  CFG_SRAM_SD_SP_RAM4       : 1;
        uint32_t  CFG_SRAM_SD_SP_RAM5       : 1;
        uint32_t  CFG_SRAM_SD_SP_RAM6       : 1;
        uint32_t  RESERVED2                 : 6;
        uint32_t  CFG_SRAM_SD_SP_ROM        : 1;
        uint32_t  CFG_SRAM_SD_SP_CACHE_RAM  : 1;
        uint32_t  CFG_SRAM_SD_SP_CRYPTO_RAM : 1;

    } bit;
    uint32_t reg;
} sysctrl_sram_lowpower_1_t;

//0x58
typedef union sysctrl_sram_lowpower_2_s
{
    struct sysctrl_sram_lowpower_2_b
    {
        uint32_t  CFG_SRAM_SD_DS_RAM0       : 1;
        uint32_t  CFG_SRAM_SD_DS_RAM1       : 1;
        uint32_t  CFG_SRAM_SD_DS_RAM2       : 1;
        uint32_t  CFG_SRAM_SD_DS_RAM3       : 1;
        uint32_t  CFG_SRAM_SD_DS_RAM4       : 1;
        uint32_t  CFG_SRAM_SD_DS_RAM5       : 1;
        uint32_t  CFG_SRAM_SD_DS_RAM6       : 1;
        uint32_t  RESERVED1                 : 6;
        uint32_t  CFG_SRAM_SD_DS_ROM        : 1;
        uint32_t  CFG_SRAM_SD_DS_CACHE_RAM  : 1;
        uint32_t  CFG_SRAM_SD_DS_CRYPTO_RAM : 1;
        uint32_t  CFG_SRAM_PD_NM_RAM0_RAM1  : 1;
        uint32_t  CFG_SRAM_PD_NM_RAM2_RAM3  : 1;
        uint32_t  CFG_SRAM_PD_NM_RAM4       : 1;
        uint32_t  CFG_SRAM_PD_NM_RAM5       : 1;
        uint32_t  CFG_SRAM_PD_NM_RAM6       : 1;
        uint32_t  RESERVED2                 : 3;
        uint32_t  CFG_SRAM_PD_SP_RAM0_RAM1  : 1;
        uint32_t  CFG_SRAM_PD_SP_RAM2_RAM3  : 1;
        uint32_t  CFG_SRAM_PD_SP_RAM4       : 1;
        uint32_t  CFG_SRAM_PD_SP_RAM5       : 1;
        uint32_t  CFG_SRAM_PD_SP_RAM6       : 1;
        uint32_t  RESERVED3                 : 3;
    } bit;
    uint32_t reg;
} sysctrl_sram_lowpower_2_t;

//0x5C
typedef union sysctrl_sram_lowpower_3_s
{
    struct sysctrl_sram_lowpower_3_b
    {
        uint32_t  CFG_SRAM_PD_DS_RAM0_RAM1      : 1;
        uint32_t  CFG_SRAM_PD_DS_RAM2_RAM3      : 1;
        uint32_t  CFG_SRAM_PD_DS_RAM4           : 1;
        uint32_t  CFG_SRAM_PD_DS_RAM5           : 1;
        uint32_t  CFG_SRAM_PD_DS_RAM6           : 1;
        uint32_t  RESERVED0                     : 3;
        uint32_t CFG_SRAM_RM                    : 4;
        uint32_t CFG_SRAM_TEST1                 : 1;
        uint32_t CFG_SRAM_TME                   : 1;
        uint32_t RESERVED1                      : 2;
        uint32_t RESERVED2                      : 3;
        uint32_t CFG_PERI1_OFF_DS               : 1;
        uint32_t CFG_PERI2_OFF_SP               : 1;
        uint32_t CFG_PERI2_OFF_DS               : 1;
        uint32_t CFG_PERI3_OFF_SP               : 1;
        uint32_t CFG_PERI3_OFF_DS               : 1;
        uint32_t CFG_CACHE_AUTO_FLUSH_SP        : 1;
        uint32_t CFG_DS_RCO32K_OFF              : 1;
        uint32_t RESERVED3                      : 6;
    } bit;
    uint32_t reg;
} sysctrl_sram_lowpower_3_t;

//0x60
typedef union sysctrl_clk_ctl2_s
{
    struct sysctrl_clk_ctl2_b
    {
        uint32_t EN_CK_UART0            : 1;
        uint32_t EN_CK_UART1            : 1;
        uint32_t EN_CK_UART2            : 1;
        uint32_t RESERVED0              : 1;
        uint32_t EN_CK_QSPI0            : 1;
        uint32_t EN_CK_QSPI1            : 1;
        uint32_t RESERVED1              : 2;

        uint32_t EN_CK_I2CM0            : 1;
        uint32_t EN_CK_I2CM1            : 1;
        uint32_t EN_CK_I2S0             : 1;
        uint32_t RESERVED2              : 1;
        uint32_t EN_CK_CRYPTO           : 1;
        uint32_t EN_CK_XDMA             : 1;
        uint32_t EN_CK_IRM              : 1;
        uint32_t RESERVED3              : 1;

        uint32_t EN_CK_TIMER0           : 1;
        uint32_t EN_CK_TIMER1           : 1;
        uint32_t EN_CK_TIMER2           : 1;
        uint32_t RESERVED4              : 1;
        uint32_t EN_CK32_TIMER3         : 1;
        uint32_t EN_CK32_timer4         : 1;
        uint32_t EN_CK32_RTC            : 1;
        uint32_t EN_CK32_GPIO           : 1;

        uint32_t RESERVED5              : 4;
        uint32_t EN_CK32_AUXCOMP        : 1;
        uint32_t EN_CK32_BODCOMP        : 1;
        uint32_t EN_CK_DIV_32K          : 1;
        uint32_t EN_RCO32K_DIV2         : 1;
    } bit;
    uint32_t reg;
} sysctrl_clk_ctl2_t;

//0x64
typedef union sysctrl_test_s
{
    struct sysctrl_test_b
    {
        uint32_t CFG_WLAN_ACTIVE_SEL        : 5;
        uint32_t RESERVED0                  : 2;
        uint32_t CFG_WLAN_ACTIVE_EN         : 1;
        uint32_t DBG_CLK_SEL                : 3;
        uint32_t RESERVED1                  : 1;
        uint32_t CFG_DPC_SPI_DIV            : 3;
        uint32_t RESERVED2                  : 1;
        uint32_t DBG_OUT_SEL                : 5;
        uint32_t RESERVED3                  : 3;
        uint32_t DBG_CLKOUT_EN              : 1;
        uint32_t CFG_ICE_WAKEUP_PMU         : 1;
        uint32_t RESERVED4                  : 2;
        uint32_t OTP_TEST_EN                : 1;
        uint32_t OTP_TEST_SEL               : 3;
    } bit;
    uint32_t reg;
} sysctrl_test_t;

/** @addtogroup REGISTER RT584Z Peripheral Control Register

  @{

*/

/**
    @addtogroup SYSTEMCTRL System Controller(SYSTEMCTRL)
    Memory Mapped Structure for System Controller
  @{
*/

typedef struct
{
    __I  sysctrl_soc_chip_info_t    SOC_CHIP_INFO;                  /*0x00*/
    __IO sysctrl_clk_ctl0_t         SYS_CLK_CTRL;                   /*0x04*/
    __IO sysctrl_clk_ctl1_t         SYS_CLK_CTRL1;                  /*0x08*/
    __IO sysctrl_power_state_t      SYS_POWER_STATE;                /*0x0C*/
    __IO uint32_t                   RESERVED_1;                      /*0x10*/
    __IO uint32_t                   RESERVED_2;                      /*0x14*/
    __IO uint32_t                   RESERVED_3;                      /*0x18*/
    __IO uint32_t                   RESERVED_4;                      /*0x1C*/
    __IO uint32_t                   GPIO_PULL_CTRL[4];              /*0x20 ~ 0x2C*/
    __IO uint32_t                   GPIO_DRV_CTRL[2];               /*0x30 ~ 0x34*/
    __IO uint32_t                   GPIO_OD_CTRL;                   /*0x38*/
    __IO uint32_t                   GPIO_EN_SCHMITT;                /*0x3C*/
    __IO uint32_t                   GPIO_EN_FILTER;                 /*0x40*/
    __IO sysctrl_soc_gpio_en_aio_t  GPIO_AIO_CTRL;                  /*0x44*/
    __IO sysctrl_cache_ctl_t        CACHE_CTRL;                     /*0x48*/
    __IO sysctrl_soc_pwm_sel_t      SOC_PWM_SEL;                    /*0x4C*/
    __IO sysctrl_sram_lowpower_0_t  SRAM_LOWPOWER_0;                /*0x50*/
    __IO sysctrl_sram_lowpower_1_t  SRAM_LOWPOWER_1;                /*0x54*/
    __IO sysctrl_sram_lowpower_2_t  SRAM_LOWPOWER_2;                /*0x58*/
    __IO sysctrl_sram_lowpower_3_t  SRAM_LOWPOWER_3;                /*0x5C*/
    __IO sysctrl_clk_ctl2_t         SYS_CLK_CTRL2;                  /*0x60*/
    __IO sysctrl_test_t             SYS_TEST;                       /*0x64*/


    __IO uint32_t                     RESERVED_5;                     /*0x68*/
    __IO uint32_t                     RESERVED_6;                     /*0x6C*/
    __IO uint32_t                     RESERVED_7;                     /*0x70*/
    __IO uint32_t                     RESERVED_8;                     /*0x74*/
    __IO uint32_t                     RESERVED_9;                     /*0x78*/
    __IO uint32_t                     RESERVED_10;                    /*0x7C*/
    __IO uint32_t                     SYS_GPIO_OMUX[8];               /*0x80~0x9C*/
    //__IO uint32_t                   SYS_GPIO_OMUX0;                 /*0x80*/
    //__IO uint32_t                   SYS_GPIO_OMUX1;                 /*0x84*/
    //__IO uint32_t                   SYS_GPIO_OMUX2;                 /*0x88*/
    //__IO uint32_t                   SYS_GPIO_OMUX3;                 /*0x8C*/
    //__IO uint32_t                   SYS_GPIO_OMUX4;                 /*0x90*/
    //__IO uint32_t                   SYS_GPIO_OMUX5;                 /*0x94*/
    //__IO uint32_t                   SYS_GPIO_OMUX6;                 /*0x98*/
    //__IO uint32_t                   SYS_GPIO_OMUX7;                 /*0x9C*/

    __IO uint32_t                     SYS_GPIO_IMUX[8];               /*0xA0*/
    //__IO uint32_t                   SYS_GPIO_IMUX0;                 /*0xA0*/
    //__IO uint32_t                   SYS_GPIO_IMUX1;                 /*0xA4*/
    //__IO uint32_t                   SYS_GPIO_IMUX2;                 /*0xA8*/
    //__IO uint32_t                   SYS_GPIO_IMUX3;                 /*0xAC*/
    //__IO uint32_t                   SYS_GPIO_IMUX4;                 /*0xB0*/
    //__IO uint32_t                   SYS_GPIO_IMUX5;                 /*0xB4*/
    //__IO uint32_t                   SYS_GPIO_IMUX6;                 /*0xB8*/
    //__IO uint32_t                   SYS_GPIO_IMUX7;                 /*0xBC*/



} SYSCTRL_T;


#define MCU_HCLK_SEL_SHFT                   0
#define MCU_HCLK_SEL_MASK                   (3<<MCU_HCLK_SEL_SHFT)
#define MCU_HCLK_SEL_32M                    (0<<MCU_HCLK_SEL_SHFT)
#define MCU_HCLK_SEL_PLL                    (1<<MCU_HCLK_SEL_SHFT)
#define MCU_HCLK_SEL_16M                    (2<<MCU_HCLK_SEL_SHFT)
#define MCU_HCLK_SEL_RCO1M                  (3<<MCU_HCLK_SEL_SHFT)

#define MCU_PERCLK_SEL_SHFT                 2
#define MCU_PERCLK_SEL_MASK                 (3<<MCU_PERCLK_SEL_SHFT)
#define MCU_PERCLK_SEL_32M                  (0<<MCU_PERCLK_SEL_SHFT)
#define MCU_PERCLK_SEL_16M                  (2<<MCU_PERCLK_SEL_SHFT)
#define MCU_PERCLK_SEL_RCO1M                (3<<MCU_PERCLK_SEL_SHFT)

#define MCU_BBPLL_ENABLE_SHIFT              15
#define MCU_BBPLL_MASK                      (1<<MCU_BBPLL_ENABLE_SHIFT)
#define MCU_BBPLL_ENABLE                    (1<<MCU_BBPLL_ENABLE_SHIFT)
#define MCU_BBPLL_DISABLE                   (0<<MCU_BBPLL_ENABLE_SHIFT)

#define MCU_BBPLL_CLK_SHIFT                 8
#define MCU_BBPLL_CLK_MASK                  (7<<MCU_BBPLL_CLK_SHIFT)
#define MCU_BBPLL_48M                       (0<<MCU_BBPLL_CLK_SHIFT)
#define MCU_BBPLL_64M                       (1<<MCU_BBPLL_CLK_SHIFT)
#define MCU_BBPLL_RVD1                       (2<<MCU_BBPLL_CLK_SHIFT)
#define MCU_BBPLL_RVD2                       (3<<MCU_BBPLL_CLK_SHIFT)
#define MCU_BBPLL_24M                       (4<<MCU_BBPLL_CLK_SHIFT)
#define MCU_BBPLL_32M                       (5<<MCU_BBPLL_CLK_SHIFT)
#define MCU_BBPLL_36M                       (6<<MCU_BBPLL_CLK_SHIFT)
#define MCU_BBPLL_40M                       (7<<MCU_BBPLL_CLK_SHIFT)

#define MCU_PERI1_PWR_OFF_DEEP_SLEEP        (1<<19)
#define MCU_PERI2_PWR_OFF_SLEEP             (1<<20)
#define MCU_PERI2_PWR_OFF_DEEP_SLEEP        (1<<21)
#define MCU_PERI3_PWR_OFF_SLEEP             (1<<22)
#define MCU_PERI3_PWR_OFF_DEEP_SLEEP        (1<<23)


/**@}*/ /* end of SYSTEM Control group */

/**@}*/ /* end of REGISTER group */


#if defined ( __CC_ARM   )
#pragma no_anon_unions
#endif

#endif      /* end of __RT584_SYSCTRL_REG_H_ */
