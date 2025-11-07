/**
  ******************************************************************************
  * @file    pmu.h
  * @author
  * @brief   power managements unit register definition header file
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
  *
  */

#ifndef __PMU_REG_H__
#define __PMU_REG_H__

#if defined (__CC_ARM)
#pragma anon_unions
#endif


typedef union pmu_bbpll_read_s
{
    struct pmu_bbpll_read_b
    {
        uint32_t BBPLL_VTBIT      : 2;
        uint32_t BBPLL_TEMPBIT    : 2;
        uint32_t BBPLL_BANK_VCO   : 3;
        uint32_t RESERVED            : 25;
    } bit;
    uint32_t reg;
} pmu_bbpll_read_t;


typedef union pmu_xtal0_s
{
    struct pmu_xtal0_b
    {

        uint32_t EN_XBUF            : 1;
        uint32_t EN_D_XTALIN        : 1;
        uint32_t SEL_XBUFLOAD       : 1;
        uint32_t BYP_XLDO           : 1;
        uint32_t AUTO_PWD_XBUF      : 1;
        uint32_t XOSC_EN_INJ            : 1;
        uint32_t XOSC_EN_VREF1      : 1;
        uint32_t XOSC_EN_AGC            : 1;
        uint32_t XOSC_CAP_MAUN      : 1;
        uint32_t XOSC_CAP_CLK_DIV2  : 1;
        uint32_t XOSC_BIAS_HALF     : 1;
        uint32_t XOSC_IBOOST            : 1;
        uint32_t XOSC_INJ_TIME      : 2;
        uint32_t XOSC_AMPDET_TH     : 2;
        uint32_t XOSC_I_INJ         : 2;
        uint32_t XOSC_C_INJ         : 2;
        uint32_t XOSC_LPF_C         : 2;
        uint32_t XOSC_LPF_R         : 1;
        uint32_t XOSC_MODE_XTAL_BO  : 1;
        uint32_t PW_XTAL            : 3;
        uint32_t EN_CLOCKMODEM      : 1;
        uint32_t RESERVED            : 4;
    } bit;
    uint32_t reg;
} pmu_xtal0_t;


typedef union pmu_xtal1_s
{
    struct pmu_xtal1_b
    {

        uint32_t XOSC_CAP_INI           : 6;
        uint32_t RESERVED1              : 2;
        uint32_t XOSC_CAP_TARGET        : 8;
        uint32_t CFG_XTAL_SETTLE_TIME   : 8;
        uint32_t CFG_BYPASS_XTAL_SETTLE : 1;
        uint32_t CFG_EN_XTAL_FAST       : 1;
        uint32_t CFG_EN_XTAL            : 1;
        uint32_t CFG_EN_XTAL_AUTO       : 1;
        uint32_t RESERVED2              : 4;
    } bit;
    uint32_t reg;
} pmu_xtal1_t;


typedef union pmu_osc_32k_s
{
    struct pmu_osc_32k_b
    {

        uint32_t TUNE_FINE_RCO_32K      : 8;
        uint32_t TUNE_COARSE_RCO_32K    : 2;
        uint32_t PW_BUF_RCO_32K         : 2;
        uint32_t PW_RCO_32K             : 4;
        uint32_t EN_XO_32K              : 1;
        uint32_t EN_XO_32K_FAST         : 1;
        uint32_t PW_BUF_XO_32K          : 2;
        uint32_t PW_XO_32K              : 3;
        uint32_t FORCE_RCO_32K_OFF      : 1;
        uint32_t RCO_32K_SEL            : 1;
        uint32_t RESERVED           : 7;
    } bit;
    uint32_t reg;
} pmu_osc_32k_t;


typedef union pmu_rvd0_s
{
    struct pmu_rvd0_b
    {

        uint32_t CFG_XTAL_FAST_TIME     : 8;
        uint32_t CFG_DS_32K_OFF_DLY     : 5;
        uint32_t RESERVED           : 11;
        uint32_t PMU_RESERVED_0         : 8;

    } bit;
    uint32_t reg;
} pmu_rvd0_t;

typedef union pmu_rco1m_s
{
    struct pmu_rco1m_b
    {

        uint32_t TUNE_FINE_RCO_1M       : 7;
        uint32_t RESERVED1              : 1;
        uint32_t TUNE_COARSE_RCO_1M     : 4;
        uint32_t PW_RCO_1M              : 2;
        uint32_t TEST_RCO_1M            : 2;
        uint32_t EN_RCO_1M              : 1;
        uint32_t RESERVED2           : 15;

    } bit;
    uint32_t reg;
} pmu_rco1m_t;

typedef union pmu_dcdc_vosel_s
{
    struct pmu_dcdc_vosel_b
    {

        uint32_t DCDC_VOSEL_NORMAL  : 6;
        uint32_t RESERVED1          : 2;
        uint32_t DCDC_VOSEL_HEAVY   : 6;
        uint32_t RESERVED2          : 2;
        uint32_t DCDC_VOSEL_LIGHT   : 6;
        uint32_t RESERVED3          : 2;
        uint32_t DCDC_RUP_RATE       : 8;

    } bit;
    uint32_t reg;
} pmu_dcdc_vosel_t;

typedef union pmu_ldomv_vosel_s
{
    struct pmu_ldomv_vosel_b
    {

        uint32_t LDOMV_VOSEL_NORMAL : 6;
        uint32_t RESERVED1          : 2;
        uint32_t LDOMV_VOSEL_HEAVY  : 6;
        uint32_t RESERVED2          : 2;
        uint32_t LDOMV_VOSEL_LIGHT  : 6;
        uint32_t RESERVED3          : 2;
        uint32_t DCDC_RDN_RATE        : 8;

    } bit;
    uint32_t reg;
} pmu_ldomv_vosel_t;

typedef union pmu_core_vosel_s
{
    struct pmu_core_vosel_b
    {

        uint32_t SLDO_VOSEL_NM        : 6;
        uint32_t RESERVED1            : 2;
        uint32_t SLDO_VOSEL_SP        : 6;
        uint32_t RESERVED2            : 2;
        uint32_t LDODIG_VOSEL         : 4;
        uint32_t LDOFLASH_VOSEL       : 4;
        uint32_t LDOFLASH_SIN       : 2;
        uint32_t LDOFLASH_IOC_WK      : 1;
        uint32_t LDOFLASH_IOC_NM      : 1;
        uint32_t POR_DLY              : 2;
        uint32_t POR_VTH              : 2;

    } bit;
    uint32_t reg;
} pmu_core_vosel_t;

typedef union pmu_dcdc_normal_s
{
    struct pmu_dcdc_normal_b
    {

        uint32_t DCDC_PPOWER_NORMAL : 3;
        uint32_t DCDC_EN_COMP_NORMAL: 1;
        uint32_t DCDC_NPOWER_NORMAL : 3;
        uint32_t DCDC_EN_ZCD_NORMAL : 1;
        uint32_t DCDC_PDRIVE_NORMAL : 3;
        uint32_t DCDC_MG_NORMAL     : 1;
        uint32_t DCDC_NDRIVE_NORMAL : 3;
        uint32_t DCDC_EN_CM_NORMAL  : 1;
        uint32_t DCDC_PW_NORMAL     : 3;
        uint32_t DCDC_C_HG_NORMAL   : 1;
        uint32_t DCDC_PWMF_NORMAL   : 4;
        uint32_t DCDC_C_SC_NORMAL   : 1;
        uint32_t DCDC_OS_PN_NORMAL  : 1;
        uint32_t DCDC_OS_NORMAL     : 2;
        uint32_t DCDC_HG_NORMAL     : 2;
        uint32_t DCDC_DLY_NORMAL    : 2;
    } bit;
    uint32_t reg;
} pmu_dcdc_normal_t;


typedef union pmu_dcdc_heavy_s
{
    struct pmu_dcdc_heavy_b
    {

        uint32_t DCDC_PPOWER_HEAVY  : 3;
        uint32_t DCDC_EN_COMP_HEAVY : 1;
        uint32_t DCDC_NPOWER_HEAVY  : 3;
        uint32_t DCDC_EN_ZCD_HEAVY  : 1;
        uint32_t DCDC_PDRIVE_HEAVY  : 3;
        uint32_t DCDC_MG_HEAVY      : 1;
        uint32_t DCDC_NDRIVE_HEAVY  : 3;
        uint32_t DCDC_EN_CM_HEAVY   : 1;
        uint32_t DCDC_PW_HEAVY      : 3;
        uint32_t DCDC_C_HG_HEAVY    : 1;
        uint32_t DCDC_PWMF_HEAVY    : 4;
        uint32_t DCDC_C_SC_HEAVY    : 1;
        uint32_t DCDC_OS_PN_HEAVY   : 1;
        uint32_t DCDC_OS_HEAVY      : 2;
        uint32_t DCDC_HG_HEAVY      : 2;
        uint32_t DCDC_DLY_HEAVY     : 2;

    } bit;
    uint32_t reg;
} pmu_dcdc_heavy_t;

typedef union pmu_dcdc_light_s
{
    struct pmu_dcdc_light_b
    {

        uint32_t DCDC_PPOWER_LIGHT  : 3;
        uint32_t DCDC_EN_COMP_LIGHT : 1;
        uint32_t DCDC_NPOWER_LIGHT  : 3;
        uint32_t DCDC_EN_ZCD_LIGHT  : 1;
        uint32_t DCDC_PDRIVE_LIGHT  : 3;
        uint32_t DCDC_MG_LIGHT      : 1;
        uint32_t DCDC_NDRIVE_LIGHT  : 3;
        uint32_t DCDC_EN_CM_LIGHT   : 1;
        uint32_t DCDC_PW_LIGHT      : 3;
        uint32_t DCDC_C_HG_LIGHT    : 1;
        uint32_t DCDC_PWMF_LIGHT    : 4;
        uint32_t DCDC_C_SC_LIGHT    : 1;
        uint32_t DCDC_OS_PN_LIGHT   : 1;
        uint32_t DCDC_OS_LIGHT      : 2;
        uint32_t DCDC_HG_LIGHT      : 2;
        uint32_t DCDC_DLY_LIGHT     : 2;

    } bit;
    uint32_t reg;
} pmu_dcdc_light_t;

typedef union pmu_dcdc_reserved_s
{
    struct pmu_dcdc_reserved_b
    {

        uint32_t DCDC_PW_DIG_NORMAL         : 2;
        uint32_t DCDC_PW_DIG_HEAVY          : 2;
        uint32_t DCDC_PW_DIG_LIGHT          : 2;
        uint32_t DCDC_EALV_AUTO             : 1;
        uint32_t DCDC_EN_EALV               : 1;
        uint32_t DCDC_RESERVED_NORMAL       : 8;
        uint32_t DCDC_RESERVED_HEAVY        : 8;
        uint32_t DCDC_RESERVED_LIGHT        : 8;
    } bit;
    uint32_t reg;
} pmu_dcdc_reserved_t;

typedef union pmu_ldo_ctrl_s
{
    struct pmu_ldo_ctrl_b
    {

        uint32_t LDODIG_SIN                 : 2;
        uint32_t LDODIG_LOUT                : 1;
        uint32_t RESERVED1                  : 1;
        uint32_t LDODIG_IOC_NM              : 3;
        uint32_t RESERVED2                  : 1;
        uint32_t LDOMV_SIN                  : 2;
        uint32_t LDOMV_LOUT                 : 1;
        uint32_t LDOFLASH_VOSEL_INIT_SEL    : 1;
        uint32_t LDOMV_IOC_NM               : 3;
        uint32_t RESERVED3                  : 1;
        uint32_t DCDC_IOC                   : 3;
        uint32_t DCDC_EN_OCP                : 1;
        uint32_t DCDC_RUP_EN                : 1;
        uint32_t DCDC_RDN_EN                : 1;
        uint32_t DCDC_MANUAL_MODE           : 2;
        uint32_t LDODIG_IOC_WK              : 3;
        uint32_t RESERVED4                  : 1;
        uint32_t LDOMV_IOC_WK               : 3;
        uint32_t RESERVED5                  : 1;

    } bit;
    uint32_t reg;
} pmu_ldo_ctrl_t;


typedef union pmu_en_ctrl_s
{
    struct pmu_en_ctrl_b
    {

        uint32_t    EN_DCDC_NM      : 1;
        uint32_t    EN_LDOMV_NM     : 1;
        uint32_t    EN_LDODIG_NM    : 1;
        uint32_t    EN_BG_NM        : 1;
        uint32_t    EN_DCDC_SP      : 1;
        uint32_t    EN_LDOMV_SP     : 1;
        uint32_t    EN_LDODIG_SP    : 1;
        uint32_t    EN_BG_SP        : 1;
        uint32_t    EN_DCDC_DS      : 1;
        uint32_t    EN_LDOMV_DS     : 1;
        uint32_t    EN_LDODIG_DS    : 1;
        uint32_t    EN_BG_DS        : 1;
        uint32_t    UVLO_DIS_NM     : 1;
        uint32_t    UVLO_DIS_SP     : 1;
        uint32_t    UVLO_DIS_DS     : 1;
        uint32_t    RESERVED1       : 1;
        uint32_t    EN_LDOANA_NM    : 1;
        uint32_t    EN_LDOANA_SP    : 1;
        uint32_t    EN_LDOANA_DS    : 1;
        uint32_t    RESERVED2       : 1;
        uint32_t    EN_LDOFLASH_NM  : 1;
        uint32_t    EN_LDOFLASH_SP  : 1;
        uint32_t    EN_LDOFLASH_DS  : 1;
        uint32_t    RESERVED3       : 1;
        uint32_t    LDOANA_BYPASS   : 1;
        uint32_t    RESERVED4       : 3;
        uint32_t    EN_LDOANA_BG_NM : 1;
        uint32_t    EN_LDOANA_BG_SP : 1;
        uint32_t    EN_LDOANA_BG_DS : 1;
        uint32_t    RESERVED5       : 1;
    } bit;
    uint32_t reg;
} pmu_en_ctrl_t;

typedef union pmu_bg_ctrl_s
{
    struct pmu_bg_ctrl_b
    {

        uint32_t RESERVED1                  : 16;
        uint32_t PMU_BG_OS                  : 2;
        uint32_t PMU_BG_OS_DIR              : 1;
        uint32_t PMU_TC_BIAS                : 1;
        uint32_t PMU_MBULK                  : 1;
        uint32_t PMU_BULK_MANU              : 1;
        uint32_t PMU_PSRR                   : 1;
        uint32_t RESERVED2                  : 1;
        uint32_t PMU_RET_SEL                : 1;
        uint32_t PMU_RES_DIS                : 1;
        uint32_t RESERVED3                  : 6;
    } bit;
    uint32_t reg;
} pmu_bg_ctrl_t;

typedef union pum_rfldo_s
{
    struct pum_rfldo_b
    {
        uint32_t LDOANA_VTUNE_NORMAL        : 4;
        uint32_t LDOANA_SIN_M               : 2;
        uint32_t LDOANA_BG_OS               : 2;
        uint32_t LDOANA_BG_OS_DIR           : 1;
        uint32_t LDOANA_LOUT                : 1;
        uint32_t LDOANA_BG_PN_SYNC          : 1;
        uint32_t LDOANA_SEL                 : 1;
        uint32_t LDOANA_IOC_NM              : 3;
        uint32_t RESERVED1                  : 1;
        uint32_t LDOANA_IOC_WK              : 3;
        uint32_t RESERVED2                  : 1;
        uint32_t LDOANA_VTUNE_HEAVY         : 4;
        uint32_t RESERVED3                  : 6;
        uint32_t APMU_TEST                  : 2;

    } bit;
    uint32_t reg;
} pmu_rfldo_t;



typedef union pmu_timing_s
{
    struct pmu_timing_b
    {
        uint32_t CFG_LV_SETTLE_TIME         : 7;
        uint32_t CFG_BYPASS_LV_SETTLE       : 1;
        uint32_t CFG_MV_SETTLE_TIME         : 7;
        uint32_t CFG_BYPASS_MV_SETTLE       : 1;
        uint32_t CFG_PWRX_SETTLE_TIME       : 3;
        uint32_t RESERVED1                  : 9;
        uint32_t FORCE_DCDC_SOC_PMU         : 1;
        uint32_t FORCE_DCDC_SOC_Heavy_Tx    : 1;
        uint32_t FORCE_DCDC_SOC_Light_Rx    : 1;
        uint32_t RESERVED2                  : 1;
    } bit;
    uint32_t reg;
} pmu_timing_t;


typedef union pmu_bbpll0_s
{
    struct pmu_bbpll0_b
    {

        uint32_t BBPLL_SETTING_AUTO         : 1;
        uint32_t BBPLL_SELBBREF_MAN         : 1;
        uint32_t BBPLL_BANK1_MAN            : 2;
        uint32_t BBPLL_SELBBCLK_MAN         : 4;
        uint32_t BBPLL_EN_DIV_MAN           : 2;
        uint32_t BBPLL_BYP_LDO              : 1;
        uint32_t BBPLL_MANUBANK             : 1;
        uint32_t BBPLL_TRIGGER_BG           : 1;
        uint32_t BBPLL_BYP                  : 1;
        uint32_t BBPLL_SEL_DLY              : 1;
        uint32_t BBPLL_SEL_IB               : 1;
        uint32_t BBPLL_SEL_TC               : 1;
        uint32_t BBPLL_EN_VT_TEMPCOMP       : 1;
        uint32_t BBPLL_SEL_VTH              : 2;
        uint32_t BBPLL_SEL_VTL              : 2;
        uint32_t BBPLL_TUNE_TEMP            : 2;
        uint32_t BBPLL_BG_OS                : 2;
        uint32_t BBPLL_HI                   : 2;
        uint32_t BBPLL_SEL_ICP              : 2;
        uint32_t BBPLL_PW                   : 2;
    } bit;
    uint32_t reg;
} pmu_bbpll0_t;

typedef union pmu_bbpll1_s
{
    struct pmu_bbpll1_b
    {

        uint32_t BBPLL_RESETN_MAN           : 1;
        uint32_t BBPLL_RESETN_AUTO          : 1;
        uint32_t BBPLL_BG_OS_DIR            : 1;
        uint32_t BBPLL_OUT2_EN              : 1;
        uint32_t BBPLL_INI_BANK             : 3;
        uint32_t RESERVED1                  : 1;
        uint32_t CFG_BBPLL_SETTLE_TIME      : 4;
        uint32_t BBPLL_BYP_LDO2             : 1;
        uint32_t BBPLL_PN_SYNC              : 1;
        uint32_t PAD_BBPLL_OUT_EN           : 1;
        uint32_t PAD_XO32M_OUT_EN           : 1;
        uint32_t RESERVED2                  : 6;
    } bit;
    uint32_t reg;
} pmu_bbpll1_t;

//0xF0
typedef union pmu_soc_ts_s
{
    struct pmu_soc_ts_b
    {
        uint32_t RESERVED0                  : 16;
        uint32_t TS_VX                      : 3;
        uint32_t RESERVED1                  : 1;
        uint32_t TS_S                       : 3;
        uint32_t RESERVED2                  : 1;
        uint32_t TS_EN                      : 1;
        uint32_t TS_RST                     : 1;
        uint32_t TS_CLK_EN                  : 1;
        uint32_t RESERVED3                  : 1;
        uint32_t TS_CLK_SEL                 : 2;
        uint32_t RESERVED4                  : 2;
    } bit;
    uint32_t reg;
} pmu_soc_ts_t;


/** @addtogroup REGISTER RT584Z Peripheral Control Register

  @{

*/

/**
    @addtogroup PMU Power Managements Unit Controller(PMU)
    Memory Mapped Structure for Power Managements Unit Controller
  @{
*/
typedef struct
{
    __IO  uint32_t              RESERVED0[2];                       //offset: 0x00~0x04
    __I   pmu_bbpll_read_t      SOC_BBPLL_READ;              //offset: 0x08
    __IO  uint32_t              RESERVED0C;                         //offset: 0x0C
    __IO  uint32_t              RESERVED1[4];                       //offset: 0x10~0x1C
    __IO  pmu_xtal0_t           PMU_SOC_PMU_XTAL0;                  //offset: 0x20
    __IO  pmu_xtal1_t           PMU_SOC_PMU_XTAL1;                    //offset: 0x24
    __IO  uint32_t              RESERVED2[2];                       //offset: 0x28~0x2C
    __IO  uint32_t              RESERVED30;                         //offset: 0x30
    __IO  pmu_osc_32k_t         PMU_OSC32K;                         //offset: 0x34
    __IO  uint32_t              RESERVED3[2];                       //offset: 0x38~0x3C
    __IO  pmu_rvd0_t  PMU_RVD0;                             //offset: 0x40
    __IO  uint32_t              RESERVED4[3];                       //offset: 0x48~0x4C
    __IO  uint32_t              RESERVED5[4];                       //offset: 0x50~0x5C
    __IO  uint32_t              RESERVED6[4];                       //offset: 0x60~0x6C
    __IO  uint32_t              RESERVED7[4];                       //offset: 0x70~0x7C
    __IO  uint32_t              RESERVED8[3];                       //offset: 0x80~0x88
    __IO  pmu_rco1m_t           SOC_PMU_RCO1M;                    //offset: 0x8C
    __IO  pmu_dcdc_vosel_t  PMU_DCDC_VOSEL;                 //offset: 0x90
    __IO  pmu_ldomv_vosel_t  PMU_LDOMV_VOSEL;               //offset: 0x94
    __IO  pmu_core_vosel_t  PMU_CORE_VOSEL;                 //offset: 0x98
    __IO  pmu_dcdc_normal_t     PMU_DCDC_NORMAL;        //offset: 0x9C
    __IO  pmu_dcdc_heavy_t      PMU_DCDC_HEAVY;          //offset: 0xA0
    __IO  pmu_dcdc_light_t      PMU_DCDC_LIGHT;          //offset: 0xA4
    __IO  pmu_dcdc_reserved_t   PMU_DCDC_RESERVED;    //offset: 0xA8
    __IO  pmu_ldo_ctrl_t        PMU_LDO_CTRL;           //offset: 0xAC
    __IO  pmu_en_ctrl_t         PMU_EN_CONTROL;             //offset: 0xB0
    __IO  pmu_bg_ctrl_t         PMU_BG_CONTROL;             //offset: 0xB4
    __IO  pmu_rfldo_t           PMU_RFLDO;                  //offset: 0xB8
    __IO  pmu_timing_t          PMU_SOC_PMU_TIMING;             //offset: 0xBC
    __IO  pmu_bbpll0_t          SOC_BBPLL0;                     //offset: 0xC0
    __IO  pmu_bbpll1_t          SOC_BBPLL1;                       //offset: 0xC4
    __IO  uint32_t              RESERVED9;                      //offset: 0xC8
    __IO  uint32_t              RESERVED10;                     //offset: 0xCC
    __IO  uint32_t              RESERVED11[4];                  //offset: 0xD0~0xDC
    __IO  uint32_t              RESERVED12[4];                  //offset: 0xE0~0xEC
    __IO  pmu_soc_ts_t          SOC_TS;                         //offset: 0xF0
} PMU_T;

/**@}*/ /* end of  PMU Controller group */

/**@}*/ /* end of REGISTER group */

#if defined (__CC_ARM)
#pragma no_anon_unions
#endif

#endif

