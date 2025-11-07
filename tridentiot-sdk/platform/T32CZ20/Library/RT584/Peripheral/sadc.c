/**
 ******************************************************************************
 * @file    sadc.c
 * @author
 * @brief   sadc driver file
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
#include "sadc.h"
#include "sysctrl.h"
#include "aux_comp.h"
#include "mp_sector.h"

/**************************************************************************************************
 *    MACROS
 *************************************************************************************************/


/**************************************************************************************************
 *    CONSTANTS AND DEFINES
 *************************************************************************************************/
#define SADC_TEST_MODE          0

#if (SADC_TEST_MODE == 1)
#define SADC_TEST_VALUE         0x5AF
#endif

#define SADC_GAIN_AIO       0x02
#define SADC_GAIN_TEMP      0x03
#define SADC_GAIN_VBAT      0x07
#define SADC_PULL_AIO       0x00
#define SADC_PULL_TEMP      0x00
#define SADC_PULL_VBAT      0x03

/**************************************************************************************************
 *    TYPEDEFS
 *************************************************************************************************/


/**************************************************************************************************
 *    GLOBAL VARIABLES
 *************************************************************************************************/
static sadc_proc_cb   sadc_reg_handler = NULL;
static uint32_t sadc_xdma_single_mode = DISABLE;
static sadc_convert_state_t  sadc_convert_state = SADC_CONVERT_IDLE;
static sadc_convert_state_t  sadc_ch_read_convert_state = SADC_CONVERT_IDLE;
//static int32_t sadc_compensation_offset = 0;
//static int32_t sadc_temperature_calibration_offset = 0;

static sadc_input_ch_t  sadc_convert_ch = SADC_CH_NC;
static sadc_value_t sadc_ch_value;

static sadc_config_resolution_t sadc_config_res = SADC_RES_12BIT;
static sadc_config_oversample_t sadc_config_os = SADC_OVERSAMPLE_256;
static sadc_proc_cb sadc_config_int_callback = NULL;

sadc_channel_config_t sadc_ch_init[] =
{
    {SADC_CHANNEL_0, SADC_AIN_0,       SADC_AIN_DISABLED4,        SADC_GAIN_AIO,  SADC_PULL_AIO,  SADC_TACQ_EDLY_TIME_4US, SADC_TACQ_EDLY_TIME_2US, SADC_BURST_ENABLE, SADC_MONITOR_LOW_THD_DEFAULT, SADC_MONITOR_HIGH_THD_DEFAULT},
    {SADC_CHANNEL_1, SADC_AIN_1,       SADC_AIN_DISABLED4,        SADC_GAIN_AIO,  SADC_PULL_AIO,  SADC_TACQ_EDLY_TIME_4US, SADC_TACQ_EDLY_TIME_2US, SADC_BURST_ENABLE, SADC_MONITOR_LOW_THD_DEFAULT, SADC_MONITOR_HIGH_THD_DEFAULT},
    {SADC_CHANNEL_2, SADC_AIN_2,       SADC_AIN_DISABLED4,        SADC_GAIN_AIO,  SADC_PULL_AIO,  SADC_TACQ_EDLY_TIME_4US, SADC_TACQ_EDLY_TIME_2US, SADC_BURST_ENABLE, SADC_MONITOR_LOW_THD_DEFAULT, SADC_MONITOR_HIGH_THD_DEFAULT},
    {SADC_CHANNEL_3, SADC_AIN_3,       SADC_AIN_DISABLED4,        SADC_GAIN_AIO,  SADC_PULL_AIO,  SADC_TACQ_EDLY_TIME_4US, SADC_TACQ_EDLY_TIME_2US, SADC_BURST_ENABLE, SADC_MONITOR_LOW_THD_DEFAULT, SADC_MONITOR_HIGH_THD_DEFAULT},
    {SADC_CHANNEL_4, SADC_AIN_4,       SADC_AIN_DISABLED4,        SADC_GAIN_AIO,  SADC_PULL_AIO,  SADC_TACQ_EDLY_TIME_4US, SADC_TACQ_EDLY_TIME_2US, SADC_BURST_ENABLE, SADC_MONITOR_LOW_THD_DEFAULT, SADC_MONITOR_HIGH_THD_DEFAULT},
    {SADC_CHANNEL_5, SADC_AIN_5,       SADC_AIN_DISABLED4,        SADC_GAIN_AIO,  SADC_PULL_AIO,  SADC_TACQ_EDLY_TIME_4US, SADC_TACQ_EDLY_TIME_2US, SADC_BURST_ENABLE, SADC_MONITOR_LOW_THD_DEFAULT, SADC_MONITOR_HIGH_THD_DEFAULT},
    {SADC_CHANNEL_6, SADC_AIN_6,       SADC_AIN_DISABLED4,        SADC_GAIN_AIO,  SADC_PULL_AIO,  SADC_TACQ_EDLY_TIME_4US, SADC_TACQ_EDLY_TIME_2US, SADC_BURST_ENABLE, SADC_MONITOR_LOW_THD_DEFAULT, SADC_MONITOR_HIGH_THD_DEFAULT},
    {SADC_CHANNEL_7, SADC_AIN_7,       SADC_AIN_DISABLED4,        SADC_GAIN_AIO,  SADC_PULL_AIO,  SADC_TACQ_EDLY_TIME_4US, SADC_TACQ_EDLY_TIME_2US, SADC_BURST_ENABLE, SADC_MONITOR_LOW_THD_DEFAULT, SADC_MONITOR_HIGH_THD_DEFAULT},
    {SADC_CHANNEL_8, SADC_TEMPERATURE, SADC_TEMPERATURE,          SADC_GAIN_TEMP, SADC_PULL_TEMP, SADC_TACQ_EDLY_TIME_4US, SADC_TACQ_EDLY_TIME_2US, SADC_BURST_ENABLE, SADC_MONITOR_LOW_THD_DEFAULT, SADC_MONITOR_HIGH_THD_DEFAULT},
    {SADC_CHANNEL_9, SADC_VBAT,        SADC_AIN_DISABLED4,        SADC_GAIN_VBAT, SADC_PULL_VBAT, SADC_TACQ_EDLY_TIME_4US, SADC_TACQ_EDLY_TIME_2US, SADC_BURST_ENABLE, SADC_MONITOR_LOW_THD_DEFAULT, SADC_MONITOR_HIGH_THD_DEFAULT},
};

/**************************************************************************************************
 *    LOCAL FUNCTIONS
 *************************************************************************************************/
/**
* @brief Sadc analog initinal
*/
void Sadc_Analog_Aio_Init(void)
{
    SADC->SADC_ANA_SET0.bit.AUX_ADC_DEBUG = 0;
    SADC->SADC_ANA_SET0.bit.AUX_ADC_MODE = 0;
    SADC->SADC_ANA_SET0.bit.AUX_ADC_OUTPUTSTB = 0;
    SADC->SADC_ANA_SET0.bit.AUX_ADC_OSPN = 0;
    SADC->SADC_ANA_SET0.bit.AUX_ADC_CLK_SEL = 0;
    SADC->SADC_ANA_SET0.bit.AUX_ADC_MCAP = 3;
    SADC->SADC_ANA_SET0.bit.AUX_ADC_MDLY = 2;
    SADC->SADC_ANA_SET0.bit.AUX_ADC_SEL_DUTY = 2;
    SADC->SADC_ANA_SET0.bit.AUX_ADC_OS = 0;
    SADC->SADC_ANA_SET0.bit.AUX_ADC_BR = 15;
    SADC->SADC_ANA_SET0.bit.AUX_ADC_PW = 0;
    SADC->SADC_ANA_SET0.bit.AUX_ADC_STB_BIT = 0;
    SADC->SADC_ANA_SET0.bit.AUX_PW = 2;

    SADC->SADC_ANA_SET1.bit.AUX_VGA_CMSEL = 0;
    SADC->SADC_ANA_SET1.bit.AUX_VGA_COMP = 1;
    SADC->SADC_ANA_SET1.bit.AUX_VGA_SIN = 0;
    SADC->SADC_ANA_SET1.bit.AUX_VGA_LOUT = 0;
    SADC->SADC_ANA_SET1.bit.AUX_VGA_SW_VDD = 0;
    SADC->SADC_ANA_SET1.bit.AUX_VGA_VLDO = 2;
    SADC->SADC_ANA_SET1.bit.AUX_VGA_ACM = 15;
    SADC->SADC_ANA_SET1.bit.AUX_VGA_PW = 31;
    SADC->SADC_ANA_SET1.bit.AUX_DC_ADJ = 3;
    SADC->SADC_ANA_SET1.bit.AUX_TEST_MODE = 0;
    SADC->SADC_ANA_SET1.bit.CFG_EN_CLKAUX = 1;
    SADC->SADC_ANA_SET1.bit.AUX_VGA_TEST_AIO_EN = 0;
    AUX_COMP->COMP_ANA_CTRL.bit.COMP_EN_START = 1;
}

void Sadc_Analog_Vbat_Init(void)
{
    SADC->SADC_ANA_SET0.bit.AUX_ADC_DEBUG = 0;
    SADC->SADC_ANA_SET0.bit.AUX_ADC_MODE = 0;
    SADC->SADC_ANA_SET0.bit.AUX_ADC_OUTPUTSTB = 0;
    SADC->SADC_ANA_SET0.bit.AUX_ADC_OSPN = 0;
    SADC->SADC_ANA_SET0.bit.AUX_ADC_CLK_SEL = 0;
    SADC->SADC_ANA_SET0.bit.AUX_ADC_MCAP = 3;
    SADC->SADC_ANA_SET0.bit.AUX_ADC_MDLY = 2;
    SADC->SADC_ANA_SET0.bit.AUX_ADC_SEL_DUTY = 2;
    SADC->SADC_ANA_SET0.bit.AUX_ADC_OS = 0;
    SADC->SADC_ANA_SET0.bit.AUX_ADC_BR = 15;
    SADC->SADC_ANA_SET0.bit.AUX_ADC_PW = 0;
    SADC->SADC_ANA_SET0.bit.AUX_ADC_STB_BIT = 0;
    SADC->SADC_ANA_SET0.bit.AUX_PW = 2;

    SADC->SADC_ANA_SET1.bit.AUX_VGA_CMSEL = 0;
    SADC->SADC_ANA_SET1.bit.AUX_VGA_COMP = 1;
    SADC->SADC_ANA_SET1.bit.AUX_VGA_SIN = 0;
    SADC->SADC_ANA_SET1.bit.AUX_VGA_LOUT = 0;
    SADC->SADC_ANA_SET1.bit.AUX_VGA_SW_VDD = 0;
    SADC->SADC_ANA_SET1.bit.AUX_VGA_VLDO = 2;
    SADC->SADC_ANA_SET1.bit.AUX_VGA_ACM = 15;
    SADC->SADC_ANA_SET1.bit.AUX_VGA_PW = 28;
    SADC->SADC_ANA_SET1.bit.AUX_DC_ADJ = 0;
    SADC->SADC_ANA_SET1.bit.AUX_TEST_MODE = 0;
    SADC->SADC_ANA_SET1.bit.CFG_EN_CLKAUX = 1;
    SADC->SADC_ANA_SET1.bit.AUX_VGA_TEST_AIO_EN = 0;
    AUX_COMP->COMP_ANA_CTRL.bit.COMP_EN_START = 1;
}

void Sadc_Analog_Temp_Init(void)
{
    SADC->SADC_ANA_SET0.bit.AUX_ADC_DEBUG = 0;
    SADC->SADC_ANA_SET0.bit.AUX_ADC_MODE = 0;
    SADC->SADC_ANA_SET0.bit.AUX_ADC_OUTPUTSTB = 0;
    SADC->SADC_ANA_SET0.bit.AUX_ADC_OSPN = 0;
    SADC->SADC_ANA_SET0.bit.AUX_ADC_CLK_SEL = 0;
    SADC->SADC_ANA_SET0.bit.AUX_ADC_MCAP = 3;
    SADC->SADC_ANA_SET0.bit.AUX_ADC_MDLY = 2;
    SADC->SADC_ANA_SET0.bit.AUX_ADC_SEL_DUTY = 2;
    SADC->SADC_ANA_SET0.bit.AUX_ADC_OS = 0;
    SADC->SADC_ANA_SET0.bit.AUX_ADC_BR = 15;
    SADC->SADC_ANA_SET0.bit.AUX_ADC_PW = 0;
    SADC->SADC_ANA_SET0.bit.AUX_ADC_STB_BIT = 0;
    SADC->SADC_ANA_SET0.bit.AUX_PW = 2;

    SADC->SADC_ANA_SET1.bit.AUX_VGA_CMSEL = 0;
    SADC->SADC_ANA_SET1.bit.AUX_VGA_COMP = 1;
    SADC->SADC_ANA_SET1.bit.AUX_VGA_SIN = 0;
    SADC->SADC_ANA_SET1.bit.AUX_VGA_LOUT = 0;
    SADC->SADC_ANA_SET1.bit.AUX_VGA_SW_VDD = 0;
    SADC->SADC_ANA_SET1.bit.AUX_VGA_VLDO = 2;
    SADC->SADC_ANA_SET1.bit.AUX_VGA_ACM = 15;
    SADC->SADC_ANA_SET1.bit.AUX_VGA_PW = 31;
    SADC->SADC_ANA_SET1.bit.AUX_DC_ADJ = 3;
    SADC->SADC_ANA_SET1.bit.AUX_TEST_MODE = 0;
    SADC->SADC_ANA_SET1.bit.CFG_EN_CLKAUX = 1;
    SADC->SADC_ANA_SET1.bit.AUX_VGA_TEST_AIO_EN = 0;
    AUX_COMP->COMP_ANA_CTRL.bit.COMP_EN_START = 1;


    PMU_CTRL->SOC_TS.bit.TS_VX = 5;
    PMU_CTRL->SOC_TS.bit.TS_S = 4;
    PMU_CTRL->SOC_TS.bit.TS_EN = 1;
    PMU_CTRL->SOC_TS.bit.TS_RST = 0;
    PMU_CTRL->SOC_TS.bit.TS_RST = 1;
    PMU_CTRL->SOC_TS.bit.TS_RST = 0;
    PMU_CTRL->SOC_TS.bit.TS_CLK_EN = 1;
    PMU_CTRL->SOC_TS.bit.TS_CLK_SEL = 1;
}


/**************************************************************************************************
 *    GLOBAL FUNCTIONS
 *************************************************************************************************/
/**
* @brief Sadc  register interrupt callback
*/
void Sadc_Register_Int_Callback(sadc_proc_cb sadc_int_callback)
{
    sadc_reg_handler = sadc_int_callback;

    return;
}
/**
* @brief Sadc interrupt enable
*/
void Sadc_Int_Enable(uint32_t int_mask)
{
    SADC->SADC_INT_CLEAR.reg = SADC_INT_CLEAR_ALL;
    SADC->SADC_INT_MASK.reg = int_mask;
    NVIC_EnableIRQ((IRQn_Type)(Sadc_IRQn));
    return;
}
/**
* @brief Sadc interrupt disable
*/
void Sadc_Int_Disable(void)
{
    NVIC_DisableIRQ((IRQn_Type)(Sadc_IRQn));
    SADC->SADC_INT_MASK.reg = SADC_INT_DISABLE_ALL;
    SADC->SADC_INT_CLEAR.reg = SADC_INT_CLEAR_ALL;
    return;
}

void Sadc_Aio_Disable(uint8_t aio_num)
{
    SYSCTRL->GPIO_AIO_CTRL.bit.GPIO_EN_AIO &= ~(0x01 << aio_num);

    return;
}

void Sadc_Aio_Enable(uint8_t aio_num)
{
    SYSCTRL->GPIO_AIO_CTRL.bit.GPIO_EN_AIO |= (0x01 << aio_num);
    return;
}

/**
* @brief Sadc xdma config function
*/
void Sadc_Xdma_Config(uint32_t xdma_start_addr,
                      uint16_t xdma_seg_size,
                      uint16_t xdma_blk_size)
{
    /*Reset XDMA*/
    SADC->SADC_WDMA_CTL1.bit.CFG_SADC_WDMA_CTL1 = ENABLE;

    /*Clear XDMA IRQ*/
    SADC->SADC_INT_CLEAR.bit.WDMA = ENABLE;
    SADC->SADC_INT_CLEAR.bit.WDMA_ERROR = ENABLE;

    /*Enable XDMA IRQ*/
    SADC->SADC_INT_MASK.bit.WDMA = 0;
    SADC->SADC_INT_MASK.bit.WDMA_ERROR = 0;


    /*Set XDMA buffer address*/
    SADC->SADC_WDMA_SET1 = xdma_start_addr;

    /*Set XDMA buffer size*/
    SADC->SADC_WDMA_SET0.bit.CFG_SADC_SEG_SIZE = xdma_seg_size;
    SADC->SADC_WDMA_SET0.bit.CFG_SADC_BLK_SIZE = xdma_blk_size;

    /*Start XDMA for memory access*/
    SADC_SET_XDMA_START();

    return;
}
/**
* @brief Sadc resolution compensation
*/
uint32_t Sadc_Resolution_Compensation(sadc_value_t *p_data)
{
    uint32_t compensation_bit = 0;

    if (p_data == NULL)
    {
        return STATUS_INVALID_PARAM;
    }

    switch (SADC_GET_RES_BIT())
    {
    case SADC_RES_8BIT:
        compensation_bit = 6;
        break;

    case SADC_RES_10BIT:
        compensation_bit = 4;
        break;

    case SADC_RES_12BIT:
        compensation_bit = 2;
        break;

    case SADC_RES_14BIT:
        break;

    default:
        break;
    }

    (*p_data) >>= compensation_bit;

    return STATUS_SUCCESS;
}

/**
* @brief Sadc resolution compensation
*/
uint32_t Sadc_Resolution_Compensation_Temp(sadc_value_t *p_data)
{
    if (p_data == NULL)
    {
        return STATUS_INVALID_PARAM;
    }

    (*p_data) >>= 2;

    return STATUS_SUCCESS;
}

void Sadc_Calibration_Init(void)
{
    MpCalAdcInit();
}

uint32_t Sadc_Calibration(sadc_cal_type_t cal_type, sadc_value_t adc_val)
{
    uint32_t read_status;
    mp_cal_adc_t mp_cal_adc;
    mp_cal_temp_adc_t mp_cal_temp_adc;
    int32_t cal_vol = 0;
    if (cal_type == SADC_CALIBRATION_VBAT)
    {
        read_status = MpCalVbatAdcRead(&mp_cal_adc);
        if (read_status == STATUS_SUCCESS)
        {
            float m, k;
            uint32_t adc_1, adc_2;

            adc_1 = mp_cal_adc.adc_1 << 2;
            adc_2 = mp_cal_adc.adc_2 << 2;

            m = (float)(3600 - 1800) / (float)(adc_2 - adc_1);
            k = (float)1800 - (float)(m * adc_1);

            cal_vol = adc_val * m + k;

        }
        else
        {
            cal_vol = adc_val;
        }
    }
    else if (cal_type == SADC_CALIBRATION_AIO)
    {
        read_status = MpCalAioAdcRead(&mp_cal_adc);
        if (read_status == STATUS_SUCCESS)
        {
            float m, k;
            uint32_t adc_1, adc_2;

            adc_1 = mp_cal_adc.adc_1 << 2;
            adc_2 = mp_cal_adc.adc_2 << 2;


            m = (float)3600 / (float)(adc_2 - adc_1);
            k = - (float)(m * adc_1);

            cal_vol = adc_val * m + k;
        }
        else
        {
            cal_vol = adc_val;
        }
    }
    else if (cal_type == SADC_CALIBRATION_TEMP)
    {
        read_status = MpCalTempAdcRead(&mp_cal_temp_adc);
        if (read_status == STATUS_SUCCESS)
        {
            cal_vol = (float)(adc_val - mp_cal_temp_adc.adc_1) / 9.3 + 25;
        }
        else
        {
            cal_vol = adc_val;
        }
    }
    else
    {
        read_status = STATUS_INVALID_PARAM;
    }

    return cal_vol;
}

/**
* @brief Sadc channel enable
*/
void Sadc_Channel_Enable(sadc_channel_config_t *config_channel)
{
    volatile sadc_pnsel_ch_t *sadc_pnsel_ch;
    volatile sadc_set_ch_t *sadc_set_ch;
    volatile sadc_thd_ch_t *sadc_thd_ch;


    sadc_pnsel_ch = &(SADC->SADC_PNSEL_CH0);
    sadc_set_ch = &(SADC->SADC_SET_CH0);
    sadc_thd_ch = &(SADC->SADC_THD_CH0);


    sadc_pnsel_ch->bit.CFG_SADC_PSEL_CH = config_channel->pi_sel;
    sadc_pnsel_ch->bit.CFG_SADC_NSEL_CH = config_channel->ni_sel;
    sadc_pnsel_ch->bit.CFG_SADC_GAIN_CH = config_channel->gain;
    sadc_pnsel_ch->bit.CFG_SADC_PULL_CH = config_channel->pull;
    sadc_pnsel_ch->bit.CFG_SADC_REF_IN_CH  = 1;

    sadc_pnsel_ch->bit.CFG_SADC_TACQ_CH = config_channel->tacq;
    sadc_pnsel_ch->bit.CFG_SADC_EDLY_CH = config_channel->edly;

    sadc_set_ch->bit.CFG_SADC_BURST_CH = config_channel->burst;


    sadc_thd_ch->bit.CFG_SADC_LTHD_CH = config_channel->low_thd;
    sadc_thd_ch->bit.CFG_SADC_HTHD_CH = config_channel->high_thd;

    /*
    sadc_pnsel_ch->bit.CFG_SADC_PSEL_CH = 7;
    sadc_pnsel_ch->bit.CFG_SADC_NSEL_CH = 15;
    sadc_pnsel_ch->bit.CFG_SADC_GAIN_CH = 2;
    sadc_pnsel_ch->bit.CFG_SADC_REF_IN_CH  = 1;
    sadc_pnsel_ch->bit.CFG_SADC_PULL_CH = 0;
    */

    return;
}


/**
* @brief Sadc channel disable
*/
void Sadc_Channel_Disable(sadc_config_channel_t ch_sel)
{
    volatile sadc_pnsel_ch_t *sadc_pnsel_ch;
    volatile sadc_set_ch_t *sadc_set_ch;
    volatile sadc_thd_ch_t *sadc_thd_ch;

    sadc_pnsel_ch = &(SADC->SADC_PNSEL_CH0) + (ch_sel * SADC_CH_REG_OFFSET);
    sadc_set_ch = &(SADC->SADC_SET_CH0) + (ch_sel * SADC_CH_REG_OFFSET);
    sadc_thd_ch = &(SADC->SADC_THD_CH0) + (ch_sel * SADC_CH_REG_OFFSET);

    sadc_pnsel_ch->reg = SADC_PNSEL_CH_REG_DEFAULT;
    sadc_set_ch->reg = SADC_SET_CH_REG_DEFAULT;
    sadc_thd_ch->reg = SADC_THD_CH_REG_DEFAULT;

    return;
}

void Sadc_Vbat_Enable(sadc_channel_config_t *config_channel)
{
    volatile sadc_pnsel_ch_t *sadc_pnsel_ch;
    volatile sadc_set_ch_t *sadc_set_ch;
    volatile sadc_thd_ch_t *sadc_thd_ch;

    sadc_pnsel_ch = &(SADC->SADC_PNSEL_CH0);
    sadc_set_ch = &(SADC->SADC_SET_CH0);
    sadc_thd_ch = &(SADC->SADC_THD_CH0);

    sadc_pnsel_ch->bit.CFG_SADC_PSEL_CH = config_channel->pi_sel;
    sadc_pnsel_ch->bit.CFG_SADC_NSEL_CH = config_channel->ni_sel;
    sadc_pnsel_ch->bit.CFG_SADC_GAIN_CH = config_channel->gain;
    sadc_pnsel_ch->bit.CFG_SADC_PULL_CH = config_channel->pull;
    sadc_pnsel_ch->bit.CFG_SADC_REF_IN_CH  = 1;

    sadc_pnsel_ch->bit.CFG_SADC_TACQ_CH = config_channel->tacq;
    sadc_pnsel_ch->bit.CFG_SADC_EDLY_CH = config_channel->edly;

    sadc_set_ch->bit.CFG_SADC_BURST_CH = config_channel->burst;

    sadc_thd_ch->bit.CFG_SADC_LTHD_CH = config_channel->low_thd;
    sadc_thd_ch->bit.CFG_SADC_HTHD_CH = config_channel->high_thd;

    /*
    sadc_pnsel_ch->bit.CFG_SADC_PSEL_CH = 10;
    sadc_pnsel_ch->bit.CFG_SADC_NSEL_CH = 15;
    sadc_pnsel_ch->bit.CFG_SADC_GAIN_CH = 7;
    sadc_pnsel_ch->bit.CFG_SADC_REF_IN_CH  = 1;
    sadc_pnsel_ch->bit.CFG_SADC_PULL_CH = 3;
    */

    return;
}

void Sadc_Vbat_Disable(sadc_config_channel_t __attribute__((__unused__)) ch_sel)
{
    volatile sadc_pnsel_ch_t *sadc_pnsel_ch;
    volatile sadc_set_ch_t *sadc_set_ch;
    volatile sadc_thd_ch_t *sadc_thd_ch;

    sadc_pnsel_ch = &(SADC->SADC_PNSEL_CH0);
    sadc_set_ch = &(SADC->SADC_SET_CH0);
    sadc_thd_ch = &(SADC->SADC_THD_CH0);

    sadc_pnsel_ch->reg = SADC_PNSEL_CH_REG_DEFAULT;
    sadc_set_ch->reg = SADC_SET_CH_REG_DEFAULT;
    sadc_thd_ch->reg = SADC_THD_CH_REG_DEFAULT;

    return;
}

void Sadc_Temp_Enable(sadc_channel_config_t *config_channel)
{
    volatile sadc_pnsel_ch_t *sadc_pnsel_ch;
    volatile sadc_set_ch_t *sadc_set_ch;
    volatile sadc_thd_ch_t *sadc_thd_ch;

    sadc_pnsel_ch = &(SADC->SADC_PNSEL_CH0);
    sadc_set_ch = &(SADC->SADC_SET_CH0);
    sadc_thd_ch = &(SADC->SADC_THD_CH0);

    sadc_pnsel_ch->bit.CFG_SADC_PSEL_CH = config_channel->pi_sel;
    sadc_pnsel_ch->bit.CFG_SADC_NSEL_CH = config_channel->ni_sel;
    sadc_pnsel_ch->bit.CFG_SADC_GAIN_CH = config_channel->gain;
    sadc_pnsel_ch->bit.CFG_SADC_PULL_CH = config_channel->pull;
    sadc_pnsel_ch->bit.CFG_SADC_REF_IN_CH  = 0;

    sadc_pnsel_ch->bit.CFG_SADC_TACQ_CH = config_channel->tacq;
    sadc_pnsel_ch->bit.CFG_SADC_EDLY_CH = config_channel->edly;

    sadc_set_ch->bit.CFG_SADC_BURST_CH = config_channel->burst;


    sadc_thd_ch->bit.CFG_SADC_LTHD_CH = config_channel->low_thd;
    sadc_thd_ch->bit.CFG_SADC_HTHD_CH = config_channel->high_thd;

    /*
    sadc_pnsel_ch->bit.CFG_SADC_PSEL_CH = 8;
    sadc_pnsel_ch->bit.CFG_SADC_NSEL_CH = 8;
    sadc_pnsel_ch->bit.CFG_SADC_GAIN_CH = 3;
    sadc_pnsel_ch->bit.CFG_SADC_PULL_CH = 0;
    sadc_pnsel_ch->bit.CFG_SADC_REF_IN_CH  = 0;
    */

    return;
}

void Sadc_Temp_Disable(sadc_config_channel_t __attribute__((__unused__)) ch_sel)
{
    volatile sadc_pnsel_ch_t *sadc_pnsel_ch;
    volatile sadc_set_ch_t *sadc_set_ch;
    volatile sadc_thd_ch_t *sadc_thd_ch;

    sadc_pnsel_ch = &(SADC->SADC_PNSEL_CH0);
    sadc_set_ch = &(SADC->SADC_SET_CH0);
    sadc_thd_ch = &(SADC->SADC_THD_CH0);

    sadc_pnsel_ch->reg = SADC_PNSEL_CH_REG_DEFAULT;
    sadc_set_ch->reg = SADC_SET_CH_REG_DEFAULT;
    sadc_thd_ch->reg = SADC_THD_CH_REG_DEFAULT;

    return;
}


/**
* @brief Sadc initinal
*/
uint32_t Sadc_Init(sadc_config_t *p_config, sadc_proc_cb sadc_int_callback)
{
    if (p_config == NULL)
    {
        return STATUS_INVALID_PARAM;
    }

    sadc_xdma_single_mode = DISABLE;
    sadc_convert_state = SADC_CONVERT_IDLE;

    SADC_RESET();                                       /*Reset SADC*/

    Sadc_Analog_Aio_Init();

    SADC_RES_BIT(p_config->sadc_resolution);            /*Set SADC resolution bit*/

    SADC_OVERSAMPLE_RATE(p_config->sadc_oversample);    /*Set SADC oversample rate*/

    if (p_config->sadc_xdma.enable == ENABLE)
    {
        Sadc_Xdma_Config(p_config->sadc_xdma.start_addr, p_config->sadc_xdma.seg_size, p_config->sadc_xdma.blk_size);

        if (p_config->sadc_xdma.blk_size == 0)
        {
            sadc_xdma_single_mode = ENABLE;
        }
    }

    if (sadc_int_callback != NULL)
    {
        Sadc_Register_Int_Callback(sadc_int_callback);
    }
    Sadc_Int_Enable(p_config->sadc_int_mask.reg);

    SADC_SAMPLE_MODE(p_config->sadc_sample_mode);                    /*Sample rate depends on timer rate*/
    if (p_config->sadc_sample_mode == SADC_SAMPLE_TIMER)
    {
        SADC_TIMER_CLOCK(p_config->sadc_timer.timer_clk_src);        /*Timer clock source = system clock*/
        SADC_TIMER_CLOCK_DIV(p_config->sadc_timer.timer_clk_div);    /*Timer rate configuration*/
    }

    //for analog test
    SADC->SADC_SET1.bit.CFG_SADC_TST = 12;

    SADC->SADC_SET1.bit.CFG_SADC_CHX_SEL = 0;
    SADC->SADC_CTL0.bit.CFG_SADC_CK_FREE = 1;

#if (SADC_TEST_MODE == 1)
    SADC_TEST_ENABLE();
    SADC_TEST_ADJUST_VALUE(SADC_TEST_VALUE);
#elif (SADC_CALIBRATION_VALUE != 0)
    SADC_TEST_ADJUST_VALUE((uint32_t)SADC_CALIBRATION_VALUE);
#endif

    return STATUS_SUCCESS;
}
/**
* @brief Sadc config enable
*/
void Sadc_Config_Enable(sadc_config_resolution_t res, sadc_config_oversample_t os, sadc_proc_cb sadc_int_callback)
{
    sadc_config_t p_sadc_config;

    Sadc_Calibration_Init();

    //=== Sadc config backup ===
    sadc_config_res = res;
    sadc_config_os = os;
    sadc_config_int_callback = sadc_int_callback;

    //=== Sadc_Config(&p_sadc_config); start ===
    p_sadc_config.sadc_int_mask.bit.DONE = 1;                         /*Set SADC interrupt mask*/
    p_sadc_config.sadc_int_mask.bit.MODE_DONE = 1;
    p_sadc_config.sadc_int_mask.bit.MONITOR_HIGH = 0x3FF;
    p_sadc_config.sadc_int_mask.bit.MONITOR_LOW = 0x3FF;
    p_sadc_config.sadc_int_mask.bit.VALID = 0;
    p_sadc_config.sadc_int_mask.bit.WDMA = 1;
    p_sadc_config.sadc_int_mask.bit.WDMA_ERROR = 1;

    p_sadc_config.sadc_resolution = res;                              /*Set SADC resolution bit*/

    p_sadc_config.sadc_oversample = os;                               /*Set SADC oversample rate*/

    p_sadc_config.sadc_xdma.enable = DISABLE;
    p_sadc_config.sadc_xdma.start_addr = (uint32_t)&sadc_ch_value;
    p_sadc_config.sadc_xdma.seg_size = 1;
    p_sadc_config.sadc_xdma.blk_size = 0;

    p_sadc_config.sadc_sample_mode = SADC_SAMPLE_START;               /*Sample rate depends on start trigger*/
    //p_sadc_config.sadc_sample_mode = SADC_SAMPLE_TIMER;
    //=== Sadc_Config(&p_sadc_config); end ===

    Sadc_Init(&p_sadc_config, sadc_int_callback);

    Sadc_Enable();       /*Enable SADC*/
}

uint32_t Sadc_Vbat_Init(sadc_config_t *p_config, sadc_proc_cb sadc_int_callback)
{
    if (p_config == NULL)
    {
        return STATUS_INVALID_PARAM;
    }

    sadc_xdma_single_mode = DISABLE;
    sadc_convert_state = SADC_CONVERT_IDLE;

    SADC_RESET();                                       /*Reset SADC*/

    Sadc_Analog_Vbat_Init();

    SADC_RES_BIT(p_config->sadc_resolution);            /*Set SADC resolution bit*/

    SADC_OVERSAMPLE_RATE(p_config->sadc_oversample);    /*Set SADC oversample rate*/

    if (p_config->sadc_xdma.enable == ENABLE)
    {
        Sadc_Xdma_Config(p_config->sadc_xdma.start_addr, p_config->sadc_xdma.seg_size, p_config->sadc_xdma.blk_size);

        if (p_config->sadc_xdma.blk_size == 0)
        {
            sadc_xdma_single_mode = ENABLE;
        }
    }

    if (sadc_int_callback != NULL)
    {
        Sadc_Register_Int_Callback(sadc_int_callback);
    }
    Sadc_Int_Enable(p_config->sadc_int_mask.reg);

    SADC_SAMPLE_MODE(p_config->sadc_sample_mode);                    /*Sample rate depends on timer rate*/
    if (p_config->sadc_sample_mode == SADC_SAMPLE_TIMER)
    {
        SADC_TIMER_CLOCK(p_config->sadc_timer.timer_clk_src);        /*Timer clock source = system clock*/
        SADC_TIMER_CLOCK_DIV(p_config->sadc_timer.timer_clk_div);    /*Timer rate configuration*/
    }

    //for analog test
    //SADC->SADC_SET1.bit.CFG_SADC_TST = 12;
    SADC->SADC_SET1.bit.CFG_SADC_TST = 8;
    SADC->SADC_SET1.bit.CFG_SADC_CHX_SEL = 0;
    SADC->SADC_CTL0.bit.CFG_SADC_CK_FREE = 1;

#if (SADC_TEST_MODE == 1)
    SADC_TEST_ENABLE();
    SADC_TEST_ADJUST_VALUE(SADC_TEST_VALUE);
#elif (SADC_CALIBRATION_VALUE != 0)
    SADC_TEST_ADJUST_VALUE((uint32_t)SADC_CALIBRATION_VALUE);
#endif

    return STATUS_SUCCESS;
}

void Sadc_Vbat_Config_Enable(sadc_config_resolution_t res, sadc_config_oversample_t os, sadc_proc_cb sadc_int_callback)
{
    sadc_config_t p_sadc_config;

    Sadc_Calibration_Init();

    //=== Sadc config backup ===
    sadc_config_res = res;
    sadc_config_os = os;
    sadc_config_int_callback = sadc_int_callback;

    //=== Sadc_Config(&p_sadc_config); start ===
    p_sadc_config.sadc_int_mask.bit.DONE = 1;                         /*Set SADC interrupt mask*/
    p_sadc_config.sadc_int_mask.bit.MODE_DONE = 1;
    p_sadc_config.sadc_int_mask.bit.MONITOR_HIGH = 0x3FF;
    p_sadc_config.sadc_int_mask.bit.MONITOR_LOW = 0x3FF;
    p_sadc_config.sadc_int_mask.bit.VALID = 0;
    p_sadc_config.sadc_int_mask.bit.WDMA = 1;
    p_sadc_config.sadc_int_mask.bit.WDMA_ERROR = 1;

    p_sadc_config.sadc_resolution = res;                              /*Set SADC resolution bit*/

    p_sadc_config.sadc_oversample = os;                               /*Set SADC oversample rate*/

    p_sadc_config.sadc_xdma.enable = DISABLE;
    p_sadc_config.sadc_xdma.start_addr = (uint32_t)&sadc_ch_value;
    p_sadc_config.sadc_xdma.seg_size = 1;
    p_sadc_config.sadc_xdma.blk_size = 0;

    p_sadc_config.sadc_sample_mode = SADC_SAMPLE_START;               /*Sample rate depends on start trigger*/
    //p_sadc_config.sadc_sample_mode = SADC_SAMPLE_TIMER;
    //=== Sadc_Config(&p_sadc_config); end ===

    Sadc_Vbat_Init(&p_sadc_config, sadc_int_callback);

    Sadc_Enable();       /*Enable SADC*/
}


uint32_t Sadc_Temp_Init(sadc_config_t *p_config, sadc_proc_cb sadc_int_callback)
{
    if (p_config == NULL)
    {
        return STATUS_INVALID_PARAM;
    }

    sadc_xdma_single_mode = DISABLE;
    sadc_convert_state = SADC_CONVERT_IDLE;

    SADC_RESET();                                       /*Reset SADC*/

    Sadc_Analog_Temp_Init();

    SADC_RES_BIT(p_config->sadc_resolution);            /*Set SADC resolution bit*/

    SADC_OVERSAMPLE_RATE(p_config->sadc_oversample);    /*Set SADC oversample rate*/

    if (p_config->sadc_xdma.enable == ENABLE)
    {
        Sadc_Xdma_Config(p_config->sadc_xdma.start_addr, p_config->sadc_xdma.seg_size, p_config->sadc_xdma.blk_size);

        if (p_config->sadc_xdma.blk_size == 0)
        {
            sadc_xdma_single_mode = ENABLE;
        }
    }

    if (sadc_int_callback != NULL)
    {
        Sadc_Register_Int_Callback(sadc_int_callback);
    }
    Sadc_Int_Enable(p_config->sadc_int_mask.reg);

    SADC_SAMPLE_MODE(p_config->sadc_sample_mode);                    /*Sample rate depends on timer rate*/
    if (p_config->sadc_sample_mode == SADC_SAMPLE_TIMER)
    {
        SADC_TIMER_CLOCK(p_config->sadc_timer.timer_clk_src);        /*Timer clock source = system clock*/
        SADC_TIMER_CLOCK_DIV(p_config->sadc_timer.timer_clk_div);    /*Timer rate configuration*/
    }

    //for analog test
    //SADC->SADC_SET1.bit.CFG_SADC_TST = 12;
    SADC->SADC_SET1.bit.CFG_SADC_TST = 8;
    SADC->SADC_SET1.bit.CFG_SADC_CHX_SEL = 0;
    SADC->SADC_CTL0.bit.CFG_SADC_CK_FREE = 1;

#if (SADC_TEST_MODE == 1)
    SADC_TEST_ENABLE();
    SADC_TEST_ADJUST_VALUE(SADC_TEST_VALUE);
#elif (SADC_CALIBRATION_VALUE != 0)
    SADC_TEST_ADJUST_VALUE((uint32_t)SADC_CALIBRATION_VALUE);
#endif

    return STATUS_SUCCESS;
}

void Sadc_Temp_Config_Enable(sadc_config_resolution_t res, sadc_config_oversample_t os, sadc_proc_cb sadc_int_callback)
{
    sadc_config_t p_sadc_config;

    Sadc_Calibration_Init();

    //=== Sadc config backup ===
    sadc_config_res = res;
    sadc_config_os = os;
    sadc_config_int_callback = sadc_int_callback;

    //=== Sadc_Config(&p_sadc_config); start ===
    p_sadc_config.sadc_int_mask.bit.DONE = 1;                         /*Set SADC interrupt mask*/
    p_sadc_config.sadc_int_mask.bit.MODE_DONE = 1;
    p_sadc_config.sadc_int_mask.bit.MONITOR_HIGH = 0x3FF;
    p_sadc_config.sadc_int_mask.bit.MONITOR_LOW = 0x3FF;
    p_sadc_config.sadc_int_mask.bit.VALID = 0;
    p_sadc_config.sadc_int_mask.bit.WDMA = 1;
    p_sadc_config.sadc_int_mask.bit.WDMA_ERROR = 1;

    p_sadc_config.sadc_resolution = res;                              /*Set SADC resolution bit*/

    p_sadc_config.sadc_oversample = os;                               /*Set SADC oversample rate*/

    p_sadc_config.sadc_xdma.enable = DISABLE;
    p_sadc_config.sadc_xdma.start_addr = (uint32_t)&sadc_ch_value;
    p_sadc_config.sadc_xdma.seg_size = 1;
    p_sadc_config.sadc_xdma.blk_size = 0;

    p_sadc_config.sadc_sample_mode = SADC_SAMPLE_START;               /*Sample rate depends on start trigger*/
    //p_sadc_config.sadc_sample_mode = SADC_SAMPLE_TIMER;
    //=== Sadc_Config(&p_sadc_config); end ===

    Sadc_Temp_Init(&p_sadc_config, sadc_int_callback);

    Sadc_Enable();       /*Enable SADC*/
}

void Sadc_Disable(void)
{

    Sadc_Int_Disable();
    Sadc_Register_Int_Callback(NULL);

    SADC_DISABLE();       /*Disable SADC*/
    SADC_LDO_DISABLE();   /*Disable the SADC LDO*/
    SADC_VGA_DISABLE();   /*Disable the SADC VGA*/

    return;
}
/**
* @brief Sadc enable
*/
void Sadc_Enable(void)
{
    SADC_ENABLE();       /*Enable SADC*/
    SADC_LDO_ENABLE();
    SADC_VGA_ENABLE();
    return;
}


/**
* @brief Sadc start
*/
void Sadc_Start(void)
{
    if (sadc_xdma_single_mode == ENABLE)
    {
        SADC_SET_XDMA_START();
    }

    sadc_convert_state = SADC_CONVERT_START;

    SADC_START();        /*Start to trigger SADC*/

    return;
}

/**
* @brief Sadc convert state
*/
sadc_convert_state_t Sadc_Convert_State_Get(void)
{
    return sadc_convert_state;
}
/**
* @brief Sadc channel read
*/
uint32_t Sadc_Channel_Read(sadc_input_ch_t ch)
{
    uint32_t read_status;

    Enter_Critical_Section();

    if (sadc_ch_read_convert_state != SADC_CONVERT_START)
    {
        sadc_ch_read_convert_state = SADC_CONVERT_START;

        Leave_Critical_Section();

        sadc_convert_ch = ch;
        Sadc_Config_Enable(sadc_config_res, sadc_config_os, sadc_config_int_callback);
        Sadc_Channel_Enable(&sadc_ch_init[ch]);
        Delay_ms(10);
        Sadc_Start();        /*Start to trigger SADC*/

        read_status = STATUS_SUCCESS;
    }
    else
    {
        Leave_Critical_Section();

        read_status = STATUS_EBUSY;
    }

    return read_status;
}

uint32_t Sadc_Vbat_Read(void)
{
    uint32_t read_status;

    Enter_Critical_Section();

    if (sadc_ch_read_convert_state != SADC_CONVERT_START)
    {
        sadc_ch_read_convert_state = SADC_CONVERT_START;

        Leave_Critical_Section();

        sadc_convert_ch = SADC_CH_VBAT;
        //callback may change
        Sadc_Vbat_Config_Enable(sadc_config_res, sadc_config_os, sadc_config_int_callback);
        Sadc_Vbat_Enable(&sadc_ch_init[SADC_CH_VBAT]);
        Delay_ms(10);
        Sadc_Start();        /*Start to trigger SADC*/

        read_status = STATUS_SUCCESS;
    }
    else
    {
        Leave_Critical_Section();

        read_status = STATUS_EBUSY;
    }

    return read_status;
}

uint32_t Sadc_Temp_Read(void)
{
    uint32_t read_status;

    Enter_Critical_Section();

    if (sadc_ch_read_convert_state != SADC_CONVERT_START)
    {
        sadc_ch_read_convert_state = SADC_CONVERT_START;

        Leave_Critical_Section();

        sadc_convert_ch = SADC_CH_TEMPERATURE;
        //callback may change
        Sadc_Temp_Config_Enable(sadc_config_res, sadc_config_os, sadc_config_int_callback);
        Sadc_Temp_Enable(&sadc_ch_init[SADC_CH_TEMPERATURE]);
        Delay_ms(10);
        Sadc_Start();        /*Start to trigger SADC*/

        read_status = STATUS_SUCCESS;
    }
    else
    {
        Leave_Critical_Section();

        read_status = STATUS_EBUSY;
    }

    return read_status;
}

int Sadc_Voltage_Result(sadc_value_t sadc_value)
{
    int value, thousund_val, hundred_val;
    int ten_val __attribute__((unused));
    int unit_val __attribute__((unused));
    int value_max, value_min;

    thousund_val = 0;
    hundred_val = 0;
    value = 0;


    thousund_val = (sadc_value / 1000) % 10;
    hundred_val = (sadc_value / 100) % 10;

    value_max = (thousund_val * 1000) + (hundred_val * 100) + 51;
    value_min = (thousund_val * 1000) + (hundred_val * 100) - 49;

    if (sadc_value <= 0)
    {
        value = 0;
    }
    else if ((sadc_value < value_max) && (sadc_value >= value_min))
    {
        value = (thousund_val * 1000) + (hundred_val * 100) ; // + (ten_val * 10);
    }
    else if ( sadc_value >= value_max)
    {
        value = (thousund_val * 1000) + ((hundred_val + 1) * 100);
    }
    else if ( sadc_value < value_min)
    {
        value = (thousund_val * 1000) + ((hundred_val - 1) * 100);
    }

    return value;
}

/**
 * @brief SADC interrupt handler
 */
__attribute__((weak)) void Sadc_Handler(void)
{
    sadc_cb_t cb;
    sadc_int_t reg_sadc_int_status;
    sadc_value_t  sadc_value;
    sadc_cal_type_t cal_type = (sadc_cal_type_t)0xFF;

    reg_sadc_int_status.reg = SADC->SADC_INT_STATUS.reg;
    SADC->SADC_INT_CLEAR.reg = reg_sadc_int_status.reg;

    if (reg_sadc_int_status.reg != 0)
    {
        if (reg_sadc_int_status.bit.DONE == 1)
        {
        }

        if (reg_sadc_int_status.bit.MODE_DONE == 1)
        {
            if (SADC_GET_SAMPLE_MODE() == SADC_SAMPLE_START)
            {
                sadc_convert_state = SADC_CONVERT_DONE;
            }
        }

        if (reg_sadc_int_status.bit.VALID == 1)
        {
            cb.type = SADC_CB_SAMPLE;
            sadc_value = SADC_GET_ADC_VALUE();

            /* Need to compensation and calibration */
            if (sadc_convert_ch <= SADC_CH_AIN7)
            {
                cal_type = SADC_CALIBRATION_AIO;
            }
            else if (sadc_convert_ch == SADC_CH_VBAT)
            {
                cal_type = SADC_CALIBRATION_VBAT;
            }
            else if (sadc_convert_ch == SADC_CH_TEMPERATURE)
            {
                cal_type = SADC_CALIBRATION_TEMP;
                Sadc_Resolution_Compensation_Temp(&sadc_value);
                cb.raw.conversion_value = sadc_value;
            }

            sadc_value = Sadc_Calibration(cal_type, sadc_value);
            cb.raw.calibration_value = sadc_value;

            if (sadc_convert_ch <= SADC_CH_AIN7)
            {
                if ( (int)sadc_value < 0 )
                {
                    sadc_value = 0;
                }
            }
            else if (sadc_convert_ch == SADC_CH_VBAT)
            {
                if ( (int)sadc_value < 0 )
                {
                    sadc_value = 0;
                }
            }

            cb.data.sample.value = sadc_value;
            cb.data.sample.channel = sadc_convert_ch;
            sadc_reg_handler(&cb);

            sadc_ch_read_convert_state = SADC_CONVERT_DONE;
        }

        if (reg_sadc_int_status.bit.WDMA == 1)
        {
        }

        if (reg_sadc_int_status.bit.WDMA_ERROR == 1)
        {
        }
    }

}