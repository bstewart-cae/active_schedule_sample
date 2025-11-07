/**
 ******************************************************************************
 * @file    rtc.c
 * @author
 * @brief   rtc driver file
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
#include "rtc.h"

/**
 * In this driver design, we don't consider multithread issue. So we don't
 * any context-switch protect. If your system is multi-thread OS, you should
 * add some prtotect mechanism.
 *
 */

static rtc_alarm_proc_cb  rtc_notify_cb;

__STATIC_INLINE uint32_t Bcd2Bin(uint32_t val)
{
    return ((val & 0xF) + ((val & 0xFF) >> 4) * 10);
}

__STATIC_INLINE uint32_t Bin2Bcd(uint32_t val)
{
    return (((val / 10) << 4) | (val % 10));
}

__STATIC_INLINE uint32_t Bcd2Bin_Ms(uint32_t val)
{
    return ((val & 0xF) + ((val & 0xFF) >> 4) * 10 + ((val & 0xFFF) >> 8) * 100);
}

__STATIC_INLINE uint32_t Bin2Bcd_Ms(uint32_t val)
{
    return (((val / 100) << 8) | (((val / 10) % 10) << 4) | (val % 10));
}

uint32_t Rtc_Get_Time(rtc_time_t *tm)
{
    uint32_t temp;

    if (tm == NULL)
    {
        return STATUS_INVALID_PARAM;
    }

read_again:

    tm->tm_msec = Bcd2Bin_Ms(RTC->RTC_MSECOND);
    tm->tm_sec  = Bcd2Bin(RTC->RTC_SECOND);
    tm->tm_min  = Bcd2Bin(RTC->RTC_MINUTE);
    tm->tm_hour = Bcd2Bin(RTC->RTC_HOUR);
    tm->tm_day  = Bcd2Bin(RTC->RTC_DAY);
    tm->tm_mon  = Bcd2Bin(RTC->RTC_MONTH);
    tm->tm_year = Bcd2Bin(RTC->RTC_YEAR);

    temp = Bcd2Bin(RTC->RTC_SECOND);    /*recheck second again.*/

    if (temp != tm->tm_sec)
    {
        /* maybe HH:MM:59 to become HH:(MM+1):00
         * so we read again.*/
        if (temp == 0)
        {
            goto read_again;
        }
        else
        {
            tm->tm_sec = temp;    /*just second update one second.*/
        }
    }

    return STATUS_SUCCESS;
}

uint32_t Rtc_Set_Time(rtc_time_t *tm)
{
    if (tm == NULL)
    {
        return STATUS_INVALID_PARAM;
    }

    if ( !(RTC->RTC_CONTROL & RTC_CTRL_ENABLE) )
    {
        Rtc_Enable();
    }

    /*we don't check input data is valid or not here.
      Caller should ensure it*/

    RTC->RTC_MSECOND = Bin2Bcd_Ms(tm->tm_msec);
    RTC->RTC_SECOND = Bin2Bcd(tm->tm_sec);
    RTC->RTC_MINUTE = Bin2Bcd(tm->tm_min);
    RTC->RTC_HOUR   = Bin2Bcd(tm->tm_hour);
    RTC->RTC_DAY    = Bin2Bcd(tm->tm_day);
    RTC->RTC_MONTH  = Bin2Bcd(tm->tm_mon);
    RTC->RTC_YEAR   = Bin2Bcd(tm->tm_year);

    RTC->RTC_LOAD  = RTC_LOAD_TIME;

    /*wait this take effect, wait RTC_LOAD bcome 0*/
    while (RTC->RTC_LOAD)
    {

    }

    return STATUS_SUCCESS;
}

uint32_t Rtc_Get_Alarm(rtc_time_t *tm)
{
    if (tm == NULL)
    {
        return STATUS_INVALID_PARAM;
    }

    tm->tm_msec = Bcd2Bin_Ms(RTC->RTC_ALARM_MSECOND);
    tm->tm_sec  = Bcd2Bin(RTC->RTC_ALARM_SECOND);
    tm->tm_min  = Bcd2Bin(RTC->RTC_ALARM_MINUTE);
    tm->tm_hour = Bcd2Bin(RTC->RTC_ALARM_HOUR);
    tm->tm_day  = Bcd2Bin(RTC->RTC_ALARM_DAY);
    tm->tm_mon  = Bcd2Bin(RTC->RTC_ALARM_MONTH);
    tm->tm_year = Bcd2Bin(RTC->RTC_ALARM_YEAR);

    return STATUS_SUCCESS;
}

uint32_t Rtc_Set_Alarm(rtc_time_t *tm, uint32_t rtc_int_mode, rtc_alarm_proc_cb rtc_usr_isr)
{
    uint32_t  temp_int_reg;

    /*We don't check rtc_int_mode is correct or not...
      caller should use correct setting.*/
    if (tm == NULL)
    {
        return STATUS_INVALID_PARAM;
    }

    if (rtc_usr_isr == NULL)
    {
        return STATUS_INVALID_PARAM;
    }

    if ( !(RTC->RTC_CONTROL & RTC_CTRL_ENABLE) )
    {
        Rtc_Enable();
    }

    /*
     * IRQ Event and IRQ Event Repeat bit in hardware register is bit8 and bit9
     * But In our software flag, each event flag in different position, so we need
     * adjust the shift number depends on the flag type.
     */

    RTC->RTC_ALARM_MSECOND = Bin2Bcd_Ms(tm->tm_msec) | ((rtc_int_mode & RTC_MSEC_MASK) << RTC_IRQEVENT_MSEC_SHIFT);

    RTC->RTC_ALARM_SECOND = Bin2Bcd(tm->tm_sec) | ((rtc_int_mode & RTC_SEC_MASK) << RTC_IRQEVENT_SEC_SHIFT);
    RTC->RTC_ALARM_MINUTE = Bin2Bcd(tm->tm_min) | ((rtc_int_mode & RTC_MIN_MASK) << RTC_IRQEVENT_MIN_SHIFT);
    RTC->RTC_ALARM_HOUR   = Bin2Bcd(tm->tm_hour) | ((rtc_int_mode & RTC_HOUR_MASK) << RTC_IRQEVENT_HOUR_SHIFT);

    RTC->RTC_ALARM_DAY    = Bin2Bcd(tm->tm_day) | ((rtc_int_mode & RTC_DAY_MASK) << RTC_IRQEVENT_DAY_SHIFT);

    RTC->RTC_ALARM_MONTH  = Bin2Bcd(tm->tm_mon) | ((rtc_int_mode & RTC_MONTH_MASK) << RTC_IRQEVENT_MONTH_SHIFT);

    RTC->RTC_ALARM_YEAR   = Bin2Bcd(tm->tm_year) | ((rtc_int_mode & RTC_YEAR_MASK) >> RTC_IRQEVENT_YEAR_RSHIFT);

    temp_int_reg = (rtc_int_mode >> RTC_INTERRUPT_MASK_SHIFT) & RTC_INTERRUPT_MASK;

    /*clear all interrupt source first*/
    RTC->RTC_INT_CLEAR = RTC_INTERRUPT_MASK;

    RTC->RTC_INT_CONTROL = temp_int_reg;

    if (temp_int_reg)
    {
        /*Enable Cortex-M3 interrupt*/
        NVIC_EnableIRQ(Rtc_IRQn);
    }

    /*remember user callback function*/
    rtc_notify_cb = rtc_usr_isr;

    RTC->RTC_LOAD  = RTC_LOAD_ALARM;

    while (RTC->RTC_LOAD)
    {

    }

    return STATUS_SUCCESS;
}


void Rtc_Disable_Alarm(void)
{
    /*Disable Cortex-M3 interrupt*/
    NVIC_DisableIRQ(Rtc_IRQn);

    RTC->RTC_INT_CONTROL = 0;   /*set control register to disable all interrpt*/

    /**clear all interrupt source first**/
    RTC->RTC_INT_CLEAR = RTC_INTERRUPT_MASK;
}

uint32_t calculate_frequency_integer(uint32_t *freq)
{
    uint32_t i = 0;
    uint32_t caldiv;

    do
    {
        if (RCO32K_CAL->CAL32K_RESULT0.bit.CAL32K_LOCK == 1)
        {
            break;
        }
    } while (1);

    for (i = 0; i < 5; i++)
    {

        if ((RCO32K_CAL->CAL32K_RESULT0.bit.EST_32K_RESULT_VALID == 1) && (RCO32K_CAL->CAL32K_RESULT0.bit.CAL32K_LOCK == 1))
        {
            uint32_t R1 = RCO32K_CAL->CAL32K_RESULT0.bit.EST_32K_RESULT;  // Mask 20 bits

            if (R1 == 0)
            {
                return STATUS_ERROR;
            }

            uint64_t numerator = ((uint64_t)512000000ULL << 16);
            uint64_t denominator = (uint64_t)R1 * 125ULL;
            uint32_t F_Q16 = (uint32_t)(numerator / denominator);

            uint8_t int_part = (F_Q16 >> 16) & 0xFF;  // ? 8 bit ???
            uint16_t frac_part = (uint16_t)(F_Q16 & 0xFFFF);
            caldiv = (((uint32_t)int_part << 16) | frac_part);

#if RCO32K_ENABLE
            // For 32KHz use the 32K calibration directly
            *freq = caldiv;
#elif RCO16K_ENABLE
            // For 16KHz divide 32K value by 2
            *freq = (caldiv >> 1);
#elif RCO20K_ENABLE
            // For 20KHz take 5/8 of the 32K value
            uint64_t temp_freq = caldiv * 5;
            *freq = (uint32_t)((temp_freq >> 3) & 0xFFFFFFFF);
#else
            *freq = caldiv;
#endif
            //LOG_DEBUG_PERI(" %d caldiv=%8x\r\n",i,caldiv);
        }
    }
    return STATUS_SUCCESS;
}

void Rtc_Set_Clk(uint32_t clk)
{
    uint32_t status = 0, cal_clk = clk;

    status = calculate_frequency_integer(&cal_clk);

    if ( status == STATUS_SUCCESS)
    {
        clk = cal_clk;
    }

    RTC->RTC_CLOCK_DIV = (clk) & 0xFFFFFF; /*only 24bits.*/
    RTC->RTC_LOAD = RTC_LOAD_DIVISOR;
    if ( !(RTC->RTC_CONTROL & RTC_CTRL_ENABLE) )
    {
        Rtc_Enable();
    }
    /*wait this take effect, wait RTC_LOAD bcome 0*/
    while (RTC->RTC_LOAD)
    {

    }

}

void Rtc_Reset(void)
{

    RTC->RTC_CONTROL = RTC_CTRL_CLRPLS;

    while (RTC->RTC_CONTROL & RTC_CTRL_CLRPLS);            /*clear in progress*/

}

uint32_t Get_RTC_Status(void)
{
    return RTC->RTC_INT_STATUS;
}

void Setup_RTC_Wakeup_From_Deep_Sleep(void)
{
    SYSCTRL->SRAM_LOWPOWER_3.bit.CFG_DS_RCO32K_OFF = 0;
}

void Rtc_Enable(void)
{
    RTC->RTC_CONTROL |= RTC_CTRL_ENABLE;
}

void Rtc_Disable(void)
{
    RTC->RTC_CONTROL &= ~RTC_CTRL_ENABLE;
}

uint32_t Setup_RTC_Ms_Counter_Alarm(uint32_t unit_time, uint32_t __attribute__((__unused__)) prescale, rtc_alarm_proc_cb rtc_usr_isr)

{
    rtc_time_t alarm_tm;
    uint32_t rtc_int_mode;
    uint32_t  temp_int_reg;
    uint32_t msec, sec, day, temp_four_year;
    int32_t temp_day;

    /*2000 to start*/
    RTC->RTC_CONTROL |= (0x01 << 6);

    alarm_tm.tm_year = 0;
    alarm_tm.tm_mon = 1;
    alarm_tm.tm_day = 1;
    alarm_tm.tm_hour = 0;
    alarm_tm.tm_min = 0;
    alarm_tm.tm_sec = 0;

    msec = unit_time % 1000;
    unit_time = (unit_time - msec) / 1000;

    alarm_tm.tm_msec = msec;

    //86400 = 1(day) * 24(hour) * 60(min) * 60(sec)
    sec = unit_time % 86400;
    day = unit_time / 86400;

    //set sec
    alarm_tm.tm_sec = (sec % 60);
    sec = sec - (sec % 60);

    //change to min, and set
    if ( unit_time != 0)
    {
        sec = sec / 60;
        alarm_tm.tm_min = (sec % 60);
        sec = sec - (sec % 60);
        //LOG_DEBUG_PERI("alarm_tm.tm_min %d\r\n",alarm_tm.tm_min);
    }

    //change to hour, and set
    if ( sec != 0)
    {
        sec = sec / 60;
        alarm_tm.tm_hour = (sec % 24);
        sec = sec - (sec % 24);
        //LOG_DEBUG_PERI("alarm_tm.tm_hour %d\r\n",alarm_tm.tm_hour);
    }
    //LOG_DEBUG_PERI("sec %d, day %d\r\n",sec,day);

    //change to day, and set
    if ( day != 0)
    {
        //1641 = 366 + 365*3;
        temp_four_year = day / 1641;
        temp_day = day % 1641;

        if ( temp_day < 366 )
        {
            uint8_t month = 1;
            uint32_t temp_day_old;

            do
            {
                temp_day_old = temp_day;
                if ( month == 1 || month == 3 || month == 5 || month == 7
                        || month == 8 || month == 10 || month == 12)
                {
                    temp_day = temp_day - 31;
                }
                else if ( month == 2 )
                {
                    temp_day = temp_day - 29;
                }
                else if ( month == 4 || month == 6
                          || month == 9 || month == 11)
                {
                    temp_day = temp_day - 30;
                }

                if ( temp_day <= 0 )
                {
                    alarm_tm.tm_year = temp_four_year * 4;
                    alarm_tm.tm_mon = month;

                    if ( temp_day == 0)
                    {
                        alarm_tm.tm_mon = alarm_tm.tm_mon + 1;
                        alarm_tm.tm_day = 1;
                    }
                    else
                    {
                        alarm_tm.tm_day = temp_day_old + 1;
                    }
                    //LOG_DEBUG_PERI("alarm_tm.tm_year %d, alarm_tm.tm_mon %d, alarm_tm.tm_day %d\r\n",alarm_tm.tm_year ,alarm_tm.tm_mon, alarm_tm.tm_day);
                }
                else
                {
                    month++;
                    //LOG_DEBUG_PERI("month %d\r\n",month);
                }
            } while (month <= 12 && temp_day > 0);
        }
        else
        {
            uint8_t month = 1;
            uint32_t temp_year_old;
            int32_t temp_day_old;

            temp_day = temp_day - 366;
            temp_year_old = (temp_day / 365) + 1;
            temp_day = temp_day % 365;

            do
            {
                temp_day_old = temp_day;
                if ( month == 1 || month == 3 || month == 5 || month == 7
                        || month == 8 || month == 10 || month == 12)
                {
                    temp_day = temp_day - 31;
                }
                else if ( month == 2 )
                {
                    temp_day = temp_day - 28;
                }
                else if ( month == 4 || month == 6
                          || month == 9 || month == 11)
                {
                    temp_day = temp_day - 30;
                }

                if ( temp_day <= 0 )
                {
                    alarm_tm.tm_year = temp_four_year * 4 + temp_year_old;
                    alarm_tm.tm_mon = month;

                    if ( temp_day == 0)
                    {
                        alarm_tm.tm_mon = alarm_tm.tm_mon + 1;
                        alarm_tm.tm_day = 1;
                    }
                    else
                    {
                        alarm_tm.tm_day = temp_day_old + 1;
                    }
                    //LOG_DEBUG_PERI("alarm_tm.tm_year %d, alarm_tm.tm_mon %d, alarm_tm.tm_day %d\r\n",alarm_tm.tm_year ,alarm_tm.tm_mon, alarm_tm.tm_day);
                }
                else
                {
                    month++;
                }
            } while (month <= 12 && temp_day > 0);
        }
    }

    /*printf("Alarm time is %2d-%2d-%2d %2d:%2d:%2d.%3d \n",
               alarm_tm.tm_year, alarm_tm.tm_mon, alarm_tm.tm_day,
               alarm_tm.tm_hour, alarm_tm.tm_min, alarm_tm.tm_sec, alarm_tm.tm_msec);
    */

    rtc_int_mode = RTC_MODE_EVENT_INTERRUPT;

    RTC->RTC_ALARM_MSECOND = Bin2Bcd_Ms(alarm_tm.tm_msec) | ((rtc_int_mode & RTC_MSEC_MASK) << RTC_IRQEVENT_MSEC_SHIFT);
    RTC->RTC_ALARM_SECOND = Bin2Bcd(alarm_tm.tm_sec) | ((rtc_int_mode & RTC_SEC_MASK) << RTC_IRQEVENT_SEC_SHIFT);
    RTC->RTC_ALARM_MINUTE = Bin2Bcd(alarm_tm.tm_min) | ((rtc_int_mode & RTC_MIN_MASK) << RTC_IRQEVENT_MIN_SHIFT);
    RTC->RTC_ALARM_HOUR   = Bin2Bcd(alarm_tm.tm_hour) | ((rtc_int_mode & RTC_HOUR_MASK) << RTC_IRQEVENT_HOUR_SHIFT);
    RTC->RTC_ALARM_DAY    = Bin2Bcd(alarm_tm.tm_day) | ((rtc_int_mode & RTC_DAY_MASK) << RTC_IRQEVENT_DAY_SHIFT);
    RTC->RTC_ALARM_MONTH  = Bin2Bcd(alarm_tm.tm_mon) | ((rtc_int_mode & RTC_MONTH_MASK) << RTC_IRQEVENT_MONTH_SHIFT);
    RTC->RTC_ALARM_YEAR   = Bin2Bcd(alarm_tm.tm_year) | ((rtc_int_mode & RTC_YEAR_MASK) >> RTC_IRQEVENT_YEAR_RSHIFT);

    temp_int_reg = (rtc_int_mode >> RTC_INTERRUPT_MASK_SHIFT) & RTC_INTERRUPT_MASK;

    /*clear all interrupt source first*/
    RTC->RTC_INT_CLEAR = RTC_INTERRUPT_MASK;
    RTC->RTC_INT_CONTROL = temp_int_reg;

    /*Enable Cortex-M3 interrupt*/
    NVIC_EnableIRQ(Rtc_IRQn);

    /*remember user callback function*/
    rtc_notify_cb = rtc_usr_isr;

    if ( !(RTC->RTC_CONTROL & RTC_CTRL_ENABLE) )
    {
        Rtc_Enable();
    }

    RTC->RTC_LOAD  = RTC_LOAD_ALARM;

    while (RTC->RTC_LOAD)
    {
    }

    alarm_tm.tm_year = 0;
    alarm_tm.tm_mon = 1;
    alarm_tm.tm_day = 1;
    alarm_tm.tm_hour = 0;
    alarm_tm.tm_min = 0;
    alarm_tm.tm_sec = 0;
    alarm_tm.tm_msec = 0;

    Rtc_Set_Time(&alarm_tm);

#if RCO16K_ENABLE
    Rtc_Set_Clk(0x100000);
#elif RCO20K_ENABLE
    Rtc_Set_Clk(0x0A0000);
#else
    Rtc_Set_Clk(0x200000);
#endif

    return STATUS_SUCCESS;
}

uint32_t Get_RTC_Ms_Counter_Time(void)
{
    rtc_time_t current_tm;
    uint32_t unit_time = 0, temp_day = 0, four_year, temp_year;
    uint32_t month = 1;

    Rtc_Get_Time(&current_tm);

    unit_time = current_tm.tm_sec;

    if (current_tm.tm_mon == 0)
    {
        if ( current_tm.tm_min )
        {
            unit_time = unit_time + (current_tm.tm_min * 60);
        }

        if ( current_tm.tm_hour )
        {
            unit_time = unit_time + (current_tm.tm_hour * 60 * 60);
        }

        if ( current_tm.tm_day )
        {
            unit_time = unit_time + (current_tm.tm_day * 60 * 60 * 24);
        }
    }
    else
    {
        if ( current_tm.tm_min )
        {
            unit_time = unit_time + (current_tm.tm_min * 60);
        }

        if ( current_tm.tm_hour )
        {
            unit_time = unit_time + (current_tm.tm_hour * 60 * 60);
        }

        //calculate how many days
        four_year = current_tm.tm_year / 4;
        temp_year = current_tm.tm_year % 4;

        //passed whole year change day
        temp_day = four_year * (365 * 3 + 366);
        if ( temp_year > 0)
        {
            temp_day = temp_day + 365 * (temp_year - 1) + 366;
        }
        //LOG_DEBUG_PERI("year temp_day :%d\n", temp_day);

        //passed whole month change day
        for ( month = 1; month <= current_tm.tm_mon; month++)
        {

            if ( month == 1 || month == 3 || month == 5 || month == 7
                    || month == 8 || month == 10 || month == 12)
            {
                if ( month == current_tm.tm_mon )
                {
                    temp_day = temp_day + (current_tm.tm_day - 1);
                }
                else
                {
                    temp_day = temp_day + 31;
                }

            }
            else if ( month == 2 )
            {
                if (temp_year)
                {
                    if ( month == current_tm.tm_mon )
                    {
                        temp_day = temp_day + (current_tm.tm_day - 1);
                    }
                    else
                    {
                        temp_day = temp_day + 28;
                    }
                }
                else
                {
                    if ( month == current_tm.tm_mon )
                    {
                        temp_day = temp_day + (current_tm.tm_day - 1);
                    }
                    else
                    {
                        temp_day = temp_day + 29;
                    }
                }
            }
            else if ( month == 4 || month == 6
                      || month == 9 || month == 11)
            {
                if ( month == current_tm.tm_mon )
                {
                    temp_day = temp_day + (current_tm.tm_day - 1);
                }
                else
                {
                    temp_day = temp_day + 30;
                }
            }
        }
        //LOG_DEBUG_PERI("month :%d, temp_day %d\n", month, temp_day);

        if ( temp_day )
        {
            unit_time = unit_time + temp_day * 24 * 60 * 60;
        }
    }

    //LOG_DEBUG_PERI("unit_time :%d\n", unit_time);
    //ms
    unit_time = (unit_time * 1000) + current_tm.tm_msec;
    return unit_time;
}

/**
 * @ingroup RTC_Driver
 * @brief RTC interrupt
 * @details
 * @return
 */
void RTC_Handler(void)
{
    uint32_t status;

    status = RTC->RTC_INT_STATUS;
    RTC->RTC_INT_CLEAR = status;     /*clear interrupt status.*/

    if (rtc_notify_cb != NULL)
    {
        /*call RTC user isr*/
        rtc_notify_cb(status);
    }

}
