/**
 ******************************************************************************
 * @file    sysfun.c
 * @author
 * @brief   System function implement driver file
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
#include "cm33.h"
#include "sysfun.h"
#include "wdt.h"
#include "flashctl.h"
#include "mp_sector.h"
/**
 * @brief Nest of critical section.
 */
static int critical_counter = 0;
static txpower_default_cfg_t tx_pwr_level;

void Enter_Critical_Section(void)
{
    __disable_irq();
    critical_counter++;
}

void Leave_Critical_Section(void)
{
    critical_counter--;

#ifdef  DEBUG
    if (critical_counter < 0)
    {
        /*Serious Error */
        while (1);
    }
#endif

    if (critical_counter == 0)
    {
        __enable_irq();
    }
}
/**
 * @brief check ic version
 */
uint32_t Version_Check(void)
{
#if 0
    /*not support yet*/
    uint32_t   version_info, ret = 1, chip_id, chip_rev;

    version_info =  SYSCTRL_S->CHIP_INFO ;

    chip_id =  (version_info >> 8) & 0xFF;
    chip_rev = (version_info >> 4) & 0x0F;

    if (chip_id != 0x84)
    {
        return 0;
    }

    return ret;

#else

    return 1;

#endif

}

/**
 * @brief  Reset the system software by using the watchdog timer to reset the chip.
 */
void Sys_Software_Reset(void)
{
    wdt_ctrl_t controller;


    controller.reg = 0;
    if (WDT->CONTROL.bit.LOCKOUT)
    {
        while (1);
    }

    WDT->CLEAR = 1;/*clear interrupt*/

    controller.bit.INT_EN = 0;
    controller.bit.RESET_EN = 1;
    controller.bit.WDT_EN = 1;

    WDT->WIN_MAX = 0x2;
    WDT->CONTROL.reg = controller.reg;
    while (1);
}
/**
 * @brief get otp version
 */
chip_model_t GetOtpVersion()
{
    uint32_t   otp_rd_buf_addr[64];
    uint32_t     i;
    uint8_t    buf_Tmp[4];

    otp_version_t otp_version;
    chip_model_t  chip_model;

    chip_model.type = CHIP_TYPE_UNKNOW;
    chip_model.version = CHIP_VERSION_UNKNOW;

    if (flash_read_otp_sec_page((uint32_t)otp_rd_buf_addr) != STATUS_SUCCESS)
    {
        return  chip_model;
    }

    for (i = 0; i < 8; i++)
    {
        *(uint32_t *)buf_Tmp = otp_rd_buf_addr[(i / 4)];
        otp_version.buf[i] = buf_Tmp[(i % 4)];
    }

    if (otp_version.buf[0] == 0xFF) //otp version flag
    {
        return chip_model;
    }

    /*ASCII Value*/
    otp_version.buf[5] -= 0x30; /* ascii 0~9 0x30~0x39 */
    otp_version.buf[6] -= 0x40; /* ascii A~Z 0x41~0x5A */

    /*reference chip_define.h
     CHIP_ID(TYPE,VER)                   ((TYPE << 8) | VER)
     CHIP_MODEL                           CHIP_ID(CHIP_TYPE,CHIP_VERSION)
    */
    chip_model.type = (chip_type_t)otp_version.buf[5];

    chip_model.version = (chip_version_t)otp_version.buf[6];
    return chip_model;
}

/**
 * @brief Set the System PMU Mode
 */
void Sys_Pmu_SetMode(pmu_mode_cfg_t pmu_mode)
{
    if (pmu_mode == PMU_MODE_DCDC)
    {
        PMU_CTRL->PMU_EN_CONTROL.bit.EN_LDOMV_NM    = 1;
        PMU_CTRL->PMU_EN_CONTROL.bit.EN_DCDC_NM     = 1;
        PMU_CTRL->PMU_EN_CONTROL.bit.EN_LDOMV_NM    = 0;
    }
    else if (pmu_mode == PMU_MODE_LDO)
    {
        PMU_CTRL->PMU_EN_CONTROL.bit.EN_LDOMV_NM    = 1;
        PMU_CTRL->PMU_EN_CONTROL.bit.EN_DCDC_NM     = 1;
        PMU_CTRL->PMU_EN_CONTROL.bit.EN_DCDC_NM     = 0;
    }
}

/**
 * @brief  Get the System PMU Mode
 */
pmu_mode_cfg_t Sys_Pmu_GetMode(void)
{
    pmu_mode_cfg_t Mode;

    Mode = PMU_MODE_DCDC;

    if ( (PMU_CTRL->PMU_EN_CONTROL.bit.EN_LDOMV_NM == 0) && (PMU_CTRL->PMU_EN_CONTROL.bit.EN_DCDC_NM == 1))
    {
        Mode = PMU_MODE_DCDC;
    }
    else if ( (PMU_CTRL->PMU_EN_CONTROL.bit.EN_LDOMV_NM == 1) && (PMU_CTRL->PMU_EN_CONTROL.bit.EN_DCDC_NM == 0))
    {
        Mode = PMU_MODE_LDO;
    }

    return Mode;
}

/**
 * @brief  Get the System slow clock mode
 */
slow_clock_mode_cfg_t Sys_Slow_Clk_Mode(void)
{
    slow_clock_mode_cfg_t mode;

    if ((RCO32K_CAL->CAL32K_CFG0.bit.CFG_CAL32K_TARGET == 204800) ||
            (RCO32K_CAL->CAL32K_CFG0.bit.CFG_CAL32K_TARGET == 102400))   //per_clk = 32MHz, target ~= 20KHz is 204800'd
    {
        //per_clk = 16MHz, target ~= 20KHz is 102400'd
        mode = RCO20K_MODE;

    }
    else if ((RCO32K_CAL->CAL32K_CFG0.bit.CFG_CAL32K_TARGET == 128000) ||
             (RCO32K_CAL->CAL32K_CFG0.bit.CFG_CAL32K_TARGET == 64000))   //per_clk = 32MHz, target ~= 20KHz is 128000'd
    {
        //per_clk = 16MHz, target ~= 20KHz is 64000'd
        if (SYSCTRL->SYS_CLK_CTRL2.bit.EN_RCO32K_DIV2)
        {
            mode = RCO16K_MODE;
        }
        else
        {
            mode = RCO32K_MODE;
        }
    }
    else
    {
        mode = RCO_MODE;        //unkonw slow clock mode
    }

    return mode;
}

/**
 * @brief  Get the System TX Power Default
 */
txpower_default_cfg_t Sys_TXPower_GetDefault(void)
{
#if (SUPPORT_SUBG_14DBM==1) || (SUPPORT_SUBG_0DBM==1) || (SUPPORT_SUBG_20DBM==1)

    txpower_default_cfg_t    PowerDefault;

#if (SUPPORT_SUBG_20DBM == 1)
    PowerDefault = TX_POWER_20DBM_DEF;
#elif (SUPPORT_SUBG_14DBM == 1)
    PowerDefault = TX_POWER_14DBM_DEF;
#else
    PowerDefault = TX_POWER_0DBM_DEF;
#endif

    return PowerDefault;

#else

    uint32_t read_addr, i;
    uint8_t txpwrlevel;
    if (Flash_Size() == FLASH_1024K)
    {
#if FLASHCTRL_SECURE_EN==1
        read_addr = 0x100FFFD8;
#else
        read_addr = 0x000FFFD8;
#endif
    }
    else if (Flash_Size() == FLASH_2048K)
    {
#if FLASHCTRL_SECURE_EN==1
        read_addr = 0x101FFFD8;
#else
        read_addr = 0x001FFFD8;
#endif
    }

    i = (read_addr + 7); //FD8~FDF, 8bytes

    for (read_addr = read_addr; read_addr < i; read_addr++)
    {

        txpwrlevel = (*(uint8_t *)(read_addr));

        if ((txpwrlevel == TX_POWER_20DBM_DEF) || (txpwrlevel == TX_POWER_14DBM_DEF) || (txpwrlevel == TX_POWER_0DBM_DEF))
        {

            break;
        }
        else
        {

            txpwrlevel = TX_POWER_14DBM_DEF;
        }
    }

    return (txpower_default_cfg_t)txpwrlevel;

#endif

}

/**
 * @brief  Get the System TX Power Default
 */
void Set_Sys_TXPower_Default(txpower_default_cfg_t txpwrdefault)
{
    tx_pwr_level = txpwrdefault;
}

