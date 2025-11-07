/**
 ******************************************************************************
 * @file    rtc_base.h
 * @author
 * @brief   real time clock register header file
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

#ifndef __RT584_RTC_REG_H__
#define __RT584_RTC_REG_H__

#if defined ( __CC_ARM   )
#pragma anon_unions
#endif

/** @addtogroup REGISTER RT584Z Peripheral Control Register

  @{

*/

/**
    @addtogroup RTC Real Time Clock Controller(RTC)
    Memory Mapped Structure for Real Time Clock Controller
  @{
*/
typedef struct
{
    __IO  uint32_t   RTC_SECOND;             //0x0
    __IO  uint32_t   RTC_MINUTE;             //0x4
    __IO  uint32_t   RTC_HOUR;               //0x8
    __IO  uint32_t   RTC_DAY;                //0xC
    __IO  uint32_t   RTC_MONTH;              //0x10
    __IO  uint32_t   RTC_YEAR;               //0x14
    __IO  uint32_t   RTC_CONTROL;            //0x18
    __IO  uint32_t   RTC_CLOCK_DIV;          //0x1C
    __IO  uint32_t   RTC_ALARM_SECOND;       //0x20
    __IO  uint32_t   RTC_ALARM_MINUTE;       //0x24
    __IO  uint32_t   RTC_ALARM_HOUR;         //0x28
    __IO  uint32_t   RTC_ALARM_DAY;          //0x2C
    __IO  uint32_t   RTC_ALARM_MONTH;        //0x30
    __IO  uint32_t   RTC_ALARM_YEAR;         //0x34
    __IO  uint32_t   RTC_INT_CONTROL;        //0x38
    __IO  uint32_t   RTC_INT_STATUS;         //0x3C
    __IO  uint32_t   RTC_INT_CLEAR;          //0x40
    __IO  uint32_t   RTC_LOAD;               //0x44
    __IO  uint32_t   RTC_MSECOND;             //0x48
    __IO  uint32_t   RTC_ALARM_MSECOND;       //0x4C

} RTC_T;

#define  RTC_INT_SEC         (1<<0)
#define  RTC_INT_MIN         (1<<1)
#define  RTC_INT_HOUR        (1<<2)
#define  RTC_INT_DAY         (1<<3)
#define  RTC_INT_MONTH       (1<<4)
#define  RTC_INT_YEAR        (1<<5)
#define  RTC_INT_EVENT       (1<<6)

#define  RTC_CTRL_CLRPLS     (1<<7)
#define  RTC_CTRL_ENABLE     (1<<8)

#define  RTC_LOAD_TIME       (1<<0)
#define  RTC_LOAD_ALARM      (1<<1)
#define  RTC_LOAD_DIVISOR    (1<<2)


/**@}*/ /* end of  RTC Controller group */

/**@}*/ /* end of REGISTER group */


#if defined ( __CC_ARM   )
#pragma no_anon_unions
#endif

#endif      /* end of __RT584_RTC_REG_H__ */