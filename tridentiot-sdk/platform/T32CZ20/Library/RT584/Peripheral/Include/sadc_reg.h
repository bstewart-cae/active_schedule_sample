/**
 ******************************************************************************
 * @file    sadc_reg.h
 * @author
 * @brief   successive approximation (SAR) analog  register header file
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

#ifndef __RT584_SADC_REG_H__
#define __RT584_SADC_REG_H__

#if defined (__CC_ARM)
#pragma anon_unions
#endif


typedef union sadc_ctrl0_s
{
    struct sadc_ctrl0_b
    {
        uint32_t CFG_SADC_ENA      : 1;
        uint32_t CFG_SADC_VGA_ENA  : 1;
        uint32_t CFG_SADC_LDO_ENA  : 1;
        uint32_t RESERVED1         : 5;
        uint32_t CFG_SADC_CK_FREE  : 2;
        uint32_t RESERVED2         : 22;
    } bit;
    uint32_t reg;
} sadc_ctrl0_t;

typedef union sadc_ctrl1_s
{
    struct sadc_ctrl1_b
    {
        uint32_t CFG_SADC_RST           : 1;
        uint32_t RESERVED1              : 7;
        uint32_t CFG_SADC_AFIFO_RST     : 1;
        uint32_t RESERVED2              : 23;
    } bit;
    uint32_t reg;
} sadc_ctrl1_t;

typedef union sadc_ctrl2_s
{
    struct sadc_ctrl2_b
    {
        uint32_t CFG_SADC_START    : 1;
        uint32_t RESERVED          : 31;
    } bit;
    uint32_t reg;
} sadc_ctrl2_t;

typedef union sadc_set0_s
{
    struct sadc_set0_b
    {
        uint32_t CFG_SADC_SMP_MODE      : 1;
        uint32_t CFG_SADC_TMR_CKSEL     : 1;
        uint32_t CFG_SADC_AFIFO_CKPSEL  : 1;
        uint32_t CFG_SADC_DBG_SEL       : 4;
        uint32_t RESERVED               : 9;
        uint32_t CFG_SADC_TMR_CKDIV     : 16;
    } bit;
    uint32_t reg;
} sadc_set0_t;

typedef union sadc_set1_s
{
    struct sadc_set1_b
    {
        uint32_t CFG_SADC_BIT           : 4;
        uint32_t CFG_SADC_CHX_SEL       : 4;
        uint32_t CFG_SADC_OSR           : 4;
        uint32_t CFG_SADC_TST           : 4;
        uint32_t CFG_SADC_VAL_TST       : 12;
        uint32_t RESERVED               : 4;
    } bit;
    uint32_t reg;
} sadc_set1_t;

typedef union sadc_pnsel_ch_s
{
    struct sadc_pnsel_ch_b
    {
        uint32_t CFG_SADC_PSEL_CH       : 4;
        uint32_t CFG_SADC_NSEL_CH       : 4;
        uint32_t CFG_SADC_GAIN_CH       : 6;
        uint32_t CFG_SADC_REF_IN_CH     : 2;
        uint32_t CFG_SADC_PULL_CH       : 8;
        uint32_t CFG_SADC_TACQ_CH       : 3;
        uint32_t RESERVED1              : 1;
        uint32_t CFG_SADC_EDLY_CH       : 3;
        uint32_t RESERVED2              : 1;
    } bit;
    uint32_t reg;
} sadc_pnsel_ch_t;

typedef union sadc_set_ch_s
{
    struct sadc_set_ch_b
    {
        uint32_t RESERVED               : 31;
        uint32_t CFG_SADC_BURST_CH      : 1;
    } bit;
    uint32_t reg;
} sadc_set_ch_t;

typedef union sadc_thd_ch_s
{
    struct sadc_thd_ch_b
    {
        uint32_t CFG_SADC_LTHD_CH       : 14;
        uint32_t RESERVED1              : 2;
        uint32_t CFG_SADC_HTHD_CH       : 14;
        uint32_t RESERVED2              : 2;
    } bit;
    uint32_t reg;
} sadc_thd_ch_t;

typedef union sadc_ana_set0_s
{
    struct sadc_ana_set0_b
    {
        uint32_t    AUX_ADC_DEBUG       : 1;
        uint32_t    AUX_ADC_MODE        : 1;
        uint32_t    AUX_ADC_OUTPUTSTB   : 1;
        uint32_t    AUX_ADC_OSPN        : 1;
        uint32_t    AUX_ADC_CLK_SEL     : 2;
        uint32_t    AUX_ADC_MCAP        : 2;
        uint32_t    AUX_ADC_MDLY        : 2;
        uint32_t    AUX_ADC_SEL_DUTY    : 2;
        uint32_t    AUX_ADC_OS          : 2;
        uint32_t    RESERVED1           : 2;
        uint32_t    AUX_ADC_BR          : 4;
        uint32_t    AUX_ADC_PW          : 3;
        uint32_t    RESERVED2           : 1;
        uint32_t    AUX_ADC_STB_BIT     : 3;
        uint32_t    RESERVED3           : 1;
        uint32_t    AUX_PW              : 3;
        uint32_t    RESERVED4           : 1;

    } bit;
    uint32_t reg;
} sadc_ana_set0_t;

typedef union sadc_ana_set1_s
{
    struct sadc_ana_set1_b
    {
        uint32_t    AUX_VGA_CMSEL       : 4;
        uint32_t    AUX_VGA_COMP        : 2;
        uint32_t    AUX_VGA_SIN         : 2;

        uint32_t    AUX_VGA_LOUT        : 1;
        uint32_t    AUX_VGA_SW_VDD      : 1;
        uint32_t    AUX_VGA_VLDO        : 2;
        uint32_t    AUX_VGA_ACM         : 4;

        uint32_t    AUX_VGA_PW          : 6;
        uint32_t    AUX_DC_ADJ          : 2;

        uint32_t    AUX_TEST_MODE       : 1;
        uint32_t CFG_EN_CLKAUX          : 1;
        uint32_t    AUX_VGA_TEST_AIO_EN : 1;
        uint32_t    RESERVED1           : 5;

    } bit;
    uint32_t reg;
} sadc_ana_set1_t;

typedef union sadc_wdma_ctl0_s
{
    struct sadc_wdma_ctl0_b
    {
        uint32_t CFG_SADC_WDMA_CTL0     : 1;
        uint32_t RESERVED               : 31;
    } bit;
    uint32_t reg;
} sadc_wdma_ctl0_t;

typedef union sadc_wdma_ctl1_s
{
    struct sadc_wdma_ctl1_b
    {
        uint32_t CFG_SADC_WDMA_CTL1     : 1;
        uint32_t RESERVED               : 31;
    } bit;
    uint32_t reg;
} sadc_wdma_ctl1_t;

typedef union sadc_wdma_set0_s
{
    struct sadc_wdma_set0_b
    {
        uint32_t CFG_SADC_SEG_SIZE     : 16;
        uint32_t CFG_SADC_BLK_SIZE     : 16;
    } bit;
    uint32_t reg;
} sadc_wdma_set0_t;

typedef union sadc_wdma_set2_s
{
    struct sadc_wdma_set2_b
    {
        uint32_t CFG_SADC_INIT_ADDR    : 1;
        uint32_t RESERVED1             : 3;
        uint32_t CFG_SADC_DMA_DATA_FMT : 2;
        uint32_t RESERVED2             : 26;
    } bit;
    uint32_t reg;
} sadc_wdma_set2_t;

typedef union sadc_int_s
{
    struct sadc_int_b
    {
        uint32_t WDMA            : 1;
        uint32_t WDMA_ERROR      : 1;
        uint32_t DONE            : 1;
        uint32_t VALID           : 1;
        uint32_t MODE_DONE       : 1;
        uint32_t RESERVED1       : 3;
        uint32_t MONITOR_LOW     : 10;
        uint32_t MONITOR_HIGH    : 10;
        uint32_t RESERVED2       : 4;
    } bit;
    uint32_t reg;
} sadc_int_t;

typedef union sadc_r0_s
{
    struct sadc_r0_b
    {
        uint32_t SADC_O          : 14;
        uint32_t RESERVED1       : 2;
        uint32_t SADC_O_CHX      : 4;
        uint32_t RESERVED2       : 12;
    } bit;
    uint32_t reg;
} sadc_r0_t;

typedef union sadc_r1_s
{
    struct sadc_r1_b
    {
        uint32_t SADC_I_12B      : 12;
        uint32_t RESERVED        : 4;
        uint32_t SADC_NUM_RES    : 16;
    } bit;
    uint32_t reg;
} sadc_r1_t;

typedef union sadc_r2_s
{
    struct sadc_r2_b
    {
        uint32_t SADC_I_SYN      : 12;
        uint32_t RESERVED1       : 4;
        uint32_t SADC_BUSY       : 1;
        uint32_t SADC_ANA_ENA    : 1;
        uint32_t RESERVED2       : 14;
    } bit;
    uint32_t reg;
} sadc_r2_t;

/** @addtogroup REGISTER RT584Z Peripheral Control Register

  @{

*/

/**
    @addtogroup SADC Successive Approximation (SAR) Analog Controller(SADC)
    Memory Mapped Structure for Successive Approximation (SAR) Analog Controller
  @{
*/

typedef struct
{
    __IO sadc_ctrl0_t SADC_CTL0;               /*0x00*/
    __IO sadc_ctrl1_t SADC_CTL1;               /*0x04*/
    __IO sadc_ctrl2_t SADC_CTL2;               /*0x08*/
    __IO sadc_set0_t SADC_SET0;                /*0x0C*/
    __IO sadc_set1_t SADC_SET1;                /*0x10*/
    __IO uint32_t SADC_RESERVED1;              /*0x14*/
    __IO uint32_t SADC_RESERVED2;              /*0x18*/
    __IO uint32_t SADC_RESERVED3;              /*0x1C*/
    __IO sadc_pnsel_ch_t SADC_PNSEL_CH0;       /*0x20*/
    __IO sadc_set_ch_t SADC_SET_CH0;           /*0x24*/
    __IO sadc_thd_ch_t SADC_THD_CH0;           /*0x28*/
    __IO uint32_t SADC_RESERVED4;              /*0x2C*/
    __IO sadc_pnsel_ch_t SADC_PNSEL_CH1;       /*0x30*/
    __IO sadc_set_ch_t SADC_SET_CH1;           /*0x34*/
    __IO sadc_thd_ch_t SADC_THD_CH1;           /*0x38*/
    __IO uint32_t SADC_RESERVED5;              /*0x3C*/
    __IO sadc_pnsel_ch_t SADC_PNSEL_CH2;       /*0x40*/
    __IO sadc_set_ch_t SADC_SET_CH2;           /*0x44*/
    __IO sadc_thd_ch_t SADC_THD_CH2;           /*0x48*/
    __IO uint32_t SADC_RESERVED6;              /*0x4C*/
    __IO sadc_pnsel_ch_t SADC_PNSEL_CH3;       /*0x50*/
    __IO sadc_set_ch_t SADC_SET_CH3;           /*0x54*/
    __IO sadc_thd_ch_t SADC_THD_CH3;           /*0x58*/
    __IO uint32_t SADC_RESERVED7;              /*0x5C*/
    __IO sadc_pnsel_ch_t SADC_PNSEL_CH4;       /*0x60*/
    __IO sadc_set_ch_t SADC_SET_CH4;           /*0x64*/
    __IO sadc_thd_ch_t SADC_THD_CH4;           /*0x68*/
    __IO uint32_t SADC_RESERVED8;              /*0x6C*/
    __IO sadc_pnsel_ch_t SADC_PNSEL_CH5;       /*0x70*/
    __IO sadc_set_ch_t SADC_SET_CH5;           /*0x74*/
    __IO sadc_thd_ch_t SADC_THD_CH5;           /*0x78*/
    __IO uint32_t SADC_RESERVED9;              /*0x7C*/
    __IO sadc_pnsel_ch_t SADC_PNSEL_CH6;       /*0x80*/
    __IO sadc_set_ch_t SADC_SET_CH6;           /*0x84*/
    __IO sadc_thd_ch_t SADC_THD_CH6;           /*0x88*/
    __IO uint32_t SADC_RESERVED10;             /*0x8C*/
    __IO sadc_pnsel_ch_t SADC_PNSEL_CH7;       /*0x90*/
    __IO sadc_set_ch_t SADC_SET_CH7;           /*0x94*/
    __IO sadc_thd_ch_t SADC_THD_CH7;           /*0x98*/
    __IO uint32_t SADC_RESERVED11;             /*0x9C*/
    __IO sadc_pnsel_ch_t SADC_PNSEL_CH8;       /*0xA0*/
    __IO sadc_set_ch_t SADC_SET_CH8;           /*0xA4*/
    __IO sadc_thd_ch_t SADC_THD_CH8;           /*0xA8*/
    __IO uint32_t SADC_RESERVED12;             /*0xAC*/
    __IO sadc_pnsel_ch_t SADC_PNSEL_CH9;       /*0xB0*/
    __IO sadc_set_ch_t SADC_SET_CH9;           /*0xB4*/
    __IO sadc_thd_ch_t SADC_THD_CH9;           /*0xB8*/
    __IO sadc_ana_set0_t SADC_ANA_SET0;        /*0xBC*/
    __IO sadc_ana_set1_t SADC_ANA_SET1;        /*0xC0*/
    __IO uint32_t SADC_RESERVED13;             /*0xC4*/
    __IO uint32_t SADC_RESERVED14;             /*0xC8*/
    __IO uint32_t SADC_RESERVED15;             /*0xCC*/
    __IO uint32_t SADC_RESERVED16;             /*0xD0*/
    __IO uint32_t SADC_RESERVED17;             /*0xD4*/
    __IO uint32_t SADC_RESERVED18;             /*0xD8*/
    __IO uint32_t SADC_RESERVED19;             /*0xDC*/
    __IO uint32_t SADC_RESERVED20;             /*0xE0*/
    __IO uint32_t SADC_RESERVED21;             /*0xE4*/
    __IO uint32_t SADC_RESERVED22;             /*0xE8*/
    __IO uint32_t SADC_RESERVED23;             /*0xEC*/
    __IO uint32_t SADC_RESERVED24;             /*0xF0*/
    __IO uint32_t SADC_RESERVED25;             /*0xF4*/
    __IO uint32_t SADC_RESERVED26;             /*0xF8*/
    __IO uint32_t SADC_RESERVED27;             /*0xFC*/
    __IO sadc_wdma_ctl0_t SADC_WDMA_CTL0;      /*0x100*/
    __IO sadc_wdma_ctl1_t SADC_WDMA_CTL1;      /*0x104*/
    __IO sadc_wdma_set0_t SADC_WDMA_SET0;      /*0x108*/
    __IO uint32_t SADC_WDMA_SET1;              /*0x10C*/
    __IO sadc_wdma_set2_t SADC_WDMA_SET2;      /*0x110*/
    __IO uint32_t SADC_WDMA_R0;                /*0x114*/
    __IO uint32_t SADC_WDMA_R1;                /*0x118*/
    __IO uint32_t SADC_RESERVED29;             /*0x11C*/
    __IO sadc_int_t SADC_INT_CLEAR;            /*0x120*/
    __IO sadc_int_t SADC_INT_MASK;             /*0x124*/
    __IO sadc_int_t SADC_INT_STATUS;           /*0x128*/
    __IO sadc_r0_t SADC_R0;                    /*0x12C*/
    __IO sadc_r1_t SADC_R1;                    /*0x130*/
    __IO sadc_r2_t SADC_R2;                    /*0x134*/
} SADC_T;


/**@}*/ /* end of SADC Control group */

/**@}*/ /* end of REGISTER group */


#if defined ( __CC_ARM   )
#pragma no_anon_unions
#endif

#endif      /* end of __RT584_SADC_REG_H__ */

