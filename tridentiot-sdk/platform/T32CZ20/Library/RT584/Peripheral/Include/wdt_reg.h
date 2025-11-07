/**
 ******************************************************************************
 * @file    wdt_reg.h
 * @author
 * @brief   watch dog timer control register header file
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



#ifndef __RT584_WDT_REG_H__
#define __RT584_WDT_REG_H__

#if defined (__CC_ARM)
#pragma anon_unions
#endif


typedef union wdt_ctrl_s
{
    struct wdt_ctrl_b
    {
        uint32_t LOCKOUT   : 1;
        uint32_t RESERVED1 : 4;
        uint32_t RESET_EN  : 1;
        uint32_t INT_EN    : 1;
        uint32_t WDT_EN    : 1;
        uint32_t RESERVED2 : 24;
    } bit;
    uint32_t reg;
} wdt_ctrl_t;


typedef union wdt_reset_occur_s
{
    struct wdt_reset_occur_b
    {
        uint32_t RESET_OCCUR    : 8;
        uint32_t RESERVED       : 24;
    } bit;
    uint32_t reg;
} wdt_reset_occur_t;


/** @addtogroup REGISTER RT584Z Peripheral Control Register

  @{

*/

/**
    @addtogroup WDT Watch Dog Timer Controller(WDT)
    Memory Mapped Structure for Watch Dog Timer Controller
  @{
*/
typedef struct
{
    __IO uint32_t WIN_MAX;                  /*0x00*/
    __I  uint32_t VALUE;                    /*0x04*/
    __IO wdt_ctrl_t CONTROL;                /*0x08*/
    __IO uint32_t WDT_KICK;                 /*0x0C*/
    __IO wdt_reset_occur_t RST_OCCUR;       /*0x10*/
    __O  uint32_t CLEAR;                    /*0x14*/
    __IO uint32_t INT_VALUE;                /*0x18*/
    __IO uint32_t WIN_MIN;                  /*0x1C*/
    __IO uint32_t PRESCALE;                 /*0x20*/

} WDT_T;


/**@}*/ /* end of WDT Controller group */

/**@}*/ /* end of REGISTER group */


#if defined ( __CC_ARM   )
#pragma no_anon_unions
#endif

#endif      /* end of __RT84_WDT_REG_H__ */


