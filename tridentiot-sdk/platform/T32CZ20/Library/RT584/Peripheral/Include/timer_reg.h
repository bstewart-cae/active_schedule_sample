/**
 ******************************************************************************
 * @file    timer_reg.h
 * @author
 * @brief   timer control register header file
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



#ifndef __RT584_TIMER_REG_H__
#define __RT584_TIMER_REG_H__

#if defined (__CC_ARM)
#pragma anon_unions
#endif



/**************************************************************************************************
 *    Timer
 *************************************************************************************************/
//0x08
typedef union timern_ctrl_s
{
    struct timern_ctrl_b
    {
        uint32_t UP_CONUNT                  : 1;
        uint32_t ONE_SHOT_EN                : 1;
        uint32_t PRESCALE                   : 3;
        uint32_t INT_ENABLE                 : 1;
        uint32_t MODE                       : 1;
        uint32_t EN                         : 1;

        uint32_t INT_STATUS                 : 1;
        uint32_t TIMER_ENABLE_STATUS        : 1;
        uint32_t CH0_CAPTURE_INT_STATUS     : 1;
        uint32_t CH1_CAPTURE_INT_STATUS     : 1;
        uint32_t RESERVED1                  : 4;

        uint32_t CH0_CAPTURE_EDGE           : 1;
        uint32_t CH1_CAPTURE_EDGE           : 1;
        uint32_t CH0_DEGLICH_EN             : 1;
        uint32_t CH1_DEGLICH_EN             : 1;
        uint32_t CH0_CAPTURE_INT_EN         : 1;
        uint32_t CH1_CAPTURE_INT_EN         : 1;
        uint32_t RESERVED2                  : 10;
    } bit;
    uint32_t reg;
} timern_ctrl_t;

//0x10
typedef union timern_cap_clr_s
{
    struct timern_cap_clr_b
    {
        uint32_t CH0_CAPTURE_INT_CLEAR      : 1;
        uint32_t CH1_CAPTURE_INT_CLEAR      : 1;
        uint32_t RESERVED1                  : 30;
    } bit;
    uint32_t reg;
} timern_cap_clr_t;

//0x24
typedef union timern_cap_en_s
{
    struct timern_cap_en_b
    {
        uint32_t CH0_CAPTURE_EN     : 1;
        uint32_t CH1_CAPTURE_EN     : 1;
        uint32_t TIMER_PWM_EN       : 1;
        uint32_t RESERVED1          : 29;
    } bit;
    uint32_t reg;
} timern_cap_en_t;

//0x28
typedef union timern_cap_io_sel_s
{
    struct timern_cap_io_sel_b
    {
        uint32_t CH0_CAPTURE_IO_SEL     : 5;
        uint32_t RESERVED1              : 3;
        uint32_t CH1_CAPTURE_IO_SEL     : 5;
        uint32_t RESERVED2              : 19;
    } bit;
    uint32_t reg;
} timern_cap_io_sel_t;

//0x30
typedef union timern_pha_s
{
    struct timern_pha_b
    {
        uint32_t PHA                    : 1;
        uint32_t RESERVED1              : 31;
    } bit;
    uint32_t reg;
} timern_pha_t;

/** @addtogroup REGISTER RT584Z Peripheral Control Register

  @{

*/

/**
    @addtogroup TIMER Timer Controller(TIMER)
    Memory Mapped Structure for Timer Controller
  @{
*/


typedef struct
{
    __IO uint32_t LOAD;                         /*0x00*/
    __IO uint32_t VALUE;                        /*0x04*/
    __IO timern_ctrl_t CONTROL;                 /*0x08*/
    __IO uint32_t CLEAR;                        /*0x0C*/
    __O  timern_cap_clr_t CAPTURE_CLEAR;        /*0x10*/
    __I  uint32_t CH0_CAP_VALUE;                /*0x14*/
    __I  uint32_t CH1_CAP_VALUE;                /*0x18*/
    __IO uint32_t PRESCALE;                     /*0x1C*/
    __IO uint32_t EXPRIED_VALUE;                /*0x20*/
    __IO timern_cap_en_t CAP_EN;                /*0x24*/
    __IO timern_cap_io_sel_t CAP_IO_SEL;        /*0x28*/
    __IO uint32_t THD;                          /*0x2C*/
    __IO timern_pha_t PHA;                      /*0x30*/

} TIMERN_T;

/**@}*/ /* end of TIMER Controller group */

/**************************************************************************************************
 *    Timer 32K
 *************************************************************************************************/

//0x08
typedef union timer32kn_ctrl_s
{
    struct timer32kn_ctrl_b
    {
        uint32_t UP_CONUNT              : 1;
        uint32_t ONE_SHOT_EN            : 1;
        uint32_t PRESCALE               : 3;
        uint32_t INT_ENABLE             : 1;
        uint32_t MODE                   : 1;
        uint32_t EN                     : 1;
        uint32_t INT_STATUS             : 1;
        uint32_t TIMER_ENABLE_STATUS    : 1;
        uint32_t RESERVED2              : 22;
    } bit;
    uint32_t reg;
} timer32kn_ctrl_t;

//0x10
typedef union timer32kn_repdly_s
{
    struct timer32kn_repdly_b
    {
        uint32_t INT_REPEAT_DELAY               : 16;
        uint32_t INT_REPEAT_DELAY_DISABLE       : 1;
        uint32_t RESERVED                       : 15;
    } bit;
    uint32_t reg;
} timer32kn_repdly_t;

/**
    @addtogroup TIMER32K Timer Controller(TIMER32K)
    Memory Mapped Structure for Timer32K Controller
  @{
*/


typedef struct
{
    __IO uint32_t LOAD;                         /*0x00*/
    __IO uint32_t VALUE;                        /*0x04*/
    __IO timer32kn_ctrl_t CONTROL;              /*0x08*/
    __IO uint32_t CLEAR;                        /*0x0C*/
    __O  timer32kn_repdly_t REPEAT_DELAY;       /*0x10*/
    __IO uint32_t PRESCALE;                     /*0x14*/
    __IO uint32_t EXPRIED_VALUE;                /*0x18*/

} TIMER32KN_T;

/**@}*/ /* end of TIMER32K Controller group */



/**@}*/ /* end of REGISTER group */


#if defined (__CC_ARM)
#pragma no_anon_unions
#endif

#endif      /* end of __RT584_TIMER_REG_H__ */

