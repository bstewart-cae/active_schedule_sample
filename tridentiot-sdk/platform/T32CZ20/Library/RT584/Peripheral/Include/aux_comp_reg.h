/**
  ******************************************************************************
 * @file     aux_comp_reg.h
  * @author
 * @brief    Aux Comparator register definition header file
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

#ifndef __RT584_AUX_COMP_REG_H__
#define __RT584_AUX_COMP_REG_H__

#if defined ( __CC_ARM   )
#pragma anon_unions
#endif




//0x00
typedef union aux_comp_ana_ctl_s
{
    struct aux_comp_ana_ctl_b
    {
        uint32_t COMP_SELREF         : 1;
        uint32_t COMP_SELINPUT       : 1;
        uint32_t COMP_PW             : 2;
        uint32_t COMP_SELHYS         : 2;
        uint32_t COMP_SWDIV          : 1;
        uint32_t COMP_PSRR           : 1;
        uint32_t COMP_VSEL           : 4;
        uint32_t COMP_REFSEL         : 4;
        uint32_t COMP_CHSEL          : 4;
        uint32_t COMP_TC             : 1;
        uint32_t RESERVED1           : 3;
        uint32_t COMP_EN_START       : 2;
        uint32_t RESERVED2           : 6;
    } bit;
    uint32_t reg;
} aux_comp_ana_ctl_t;

//0x04
typedef union aux_comp_dig_ctl0_s
{
    struct aux_comp_dig_ctl0_b
    {
        uint32_t COMP_EN_NM         : 1;
        uint32_t COMP_EN_SP         : 1;
        uint32_t COMP_EN_DS         : 1;
        uint32_t RESERVED1          : 1;
        uint32_t DEBOUNCE_EN        : 1;
        uint32_t RESERVED2          : 1;
        uint32_t DEBOUNCE_SEL       : 2;
        uint32_t COUNTER_MODE_EN    : 1;
        uint32_t RESERVED3          : 1;
        uint32_t COUNTER_MODE_EDGE  : 2;
        uint32_t DS_WAKEUP_EN       : 1;
        uint32_t DS_WAKEUP_POL      : 1;
        uint32_t RESERVED4          : 2;
        uint32_t COUNTER_TRIGGER_TH : 16;
    } bit;
    uint32_t reg;
} aux_comp_dig_ctl0_t;

//0x08
typedef union aux_comp_dig_ctl1_s
{
    struct aux_comp_dig_ctl1_b
    {
        uint32_t EN_INTR_RISING         : 1;
        uint32_t EN_INTR_FALLING        : 1;
        uint32_t EN_INTR_COUNTER        : 1;
        uint32_t RESERVED1              : 5;
        uint32_t CLR_INTR_RISING        : 1;
        uint32_t CLR_INTR_FALLING       : 1;
        uint32_t CLR_INTR_COUNTER       : 1;
        uint32_t CLR_COUNTER            : 1;
        uint32_t RESERVED2              : 4;
        uint32_t COMP_SETTLE_TIME       : 4;
        uint32_t RESERVED3              : 12;
    } bit;
    uint32_t reg;
} aux_comp_dig_ctl1_t;

//0x0c
typedef union aux_comp_dig_ctl2_s
{
    struct aux_comp_dig_ctl2_b
    {
        uint32_t STA_INTR_RISING        : 1;
        uint32_t STA_INTR_FALLING       : 1;
        uint32_t STA_INTR_COUNTER       : 1;
        uint32_t RESERVED1              : 5;
        uint32_t COMP_OUT               : 1;
        uint32_t RESERVED2              : 7;
        uint32_t COUNTER_CNT            : 16;
    } bit;
    uint32_t reg;
} aux_comp_dig_ctl2_t;


/** @addtogroup REGISTER RT584Z Peripheral Control Register

  @{

*/

/**
    @addtogroup ACMP Analog Comparator Controller(ACMP)
    Memory Mapped Structure for ACMP Controller
  @{
*/

typedef struct
{
    __IO aux_comp_ana_ctl_t       COMP_ANA_CTRL;       /*0x00*/
    __IO aux_comp_dig_ctl0_t      COMP_DIG_CTRL0;      /*0x04*/
    __IO aux_comp_dig_ctl1_t      COMP_DIG_CTRL1;      /*0x08*/
    __IO aux_comp_dig_ctl2_t      COMP_DIG_CTRL2 ;     /*0x0C*/
} AUX_COMP_CTL_T;


/**@}*/ /* end of ACMP register group */

/**@}*/ /* end of REGISTER group */


#if defined ( __CC_ARM   )
#pragma no_anon_unions
#endif

#endif      /* end of __RT584_AUX_COMP_REG_H__ */
