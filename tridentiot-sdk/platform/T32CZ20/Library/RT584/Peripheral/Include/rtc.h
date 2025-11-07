/**
 ******************************************************************************
 * @file    ringbuffer.h
 * @author
 * @brief   ring buffer header file
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
  *  RTC_MODE_MATCH_SEC_INTERRUPT mode will only trigger rtc "second interrupt"
* "current second" = "alarm second" once per minute, if RTC_MODE_EN_SEC_INTERRUPT
*  mode set.
*
*  RTC_MODE_SECOND_EVENT_INTERRUPT will generate "event interupt" every second,
*  if RTC_MODE_EVENT_INTERRUPT should be enable.
*
*  RTC_MODE_EVERY_MIN_INTERRUPT mode will trigger rtc "min interrupt" for every
* "minute" if RTC_MODE_EN_MIN_INTERRUPT mode set.
*
*  RTC_MODE_MATCH_MIN_INTERRUPT mode will only trigger rtc "minute interrupt"
* "current minute" = "alarm minute" once per hour, if RTC_MODE_EN_MIN_INTERRUPT
*  mode set.
*
*  RTC_MODE_MINUTE_EVENT_INTERRUPT will generate "event interupt" for matched SS matched.
*  (RTC_MODE_SECOND_EVENT_INTERRUPT should NOT set for this option. And
*  RTC_MODE_EVENT_INTERRUPT should be enable.)
*
*  RTC_MODE_EVERY_HOUR_INTERRUPT mode will trigger rtc "hour interrupt" for every
* "HOUR" if RTC_MODE_EN_HOUR_INTERRUPT mode set.
*
*  RTC_MODE_MATCH_HOUR_INTERRUPT mode will only trigger rtc "hour interrupt"
* "current hour" = "alarm hour" once per day, if RTC_MODE_EN_HOUR_INTERRUPT mode set.
*
*  RTC_MODE_HOUR_EVENT_INTERRUPT will generate "event interupt"for MM:SS matched.
*  (RTC_MODE_MIN_EVENT_INTERRUPT and  RTC_MODE_SEC_EVENT_INTERRUPT should
*   NOT set for this option.  And RTC_MODE_EVENT_INTERRUPT should be enable.)
*
*  RTC_MODE_EVERY_DAY_INTERRUPT mode will trigger rtc "day interrupt" for every
* "DAY" if RTC_MODE_EN_DAY_INTERRUPT mode set.
*
*  RTC_MODE_MATCH_DAY_INTERRUPT mode will only trigger rtc "day interrupt"
* "current day" = "alarm day" once per month, if RTC_MODE_EN_DAY_INTERRUPT mode set.
*
*  RTC_MODE_DAY_EVENT_INTERRUPT will generate "event interupt" for HH:MM:SS matched
*  (RTC_MODE_HOUR_EVENT_INTERRUPT, RTC_MODE_MIN_EVENT_INTERRUPT and
*   RTC_MODE_SEC_EVENT_INTERRUPT should NOT set for this option.
*   And RTC_MODE_EVENT_INTERRUPT should be enable.)
*
*  RTC_MODE_EVERY_MONTH_INTERRUPT mode will trigger rtc "month interrupt" for every
* "month" if RTC_MODE_EN_MONTH_INTERRUPT mode set.
*
*  RTC_MODE_MATCH_MONTH_INTERRUPT mode will only trigger rtc "month interrupt"
* "current month" = "alarm month" once per year, if RTC_MODE_EN_MONTH_INTERRUPT mode set.
*
*  RTC_MODE_MONTH_EVENT_INTERRUPT will generate "event interupt" for DAY HH:MM:SS matched.
*  (RTC_MODE_DAY_EVENT_INTERRUPT, RTC_MODE_HOUR_EVENT_INTERRUPT,
*   RTC_MODE_MIN_EVENT_INTERRUPT and RTC_MODE_SEC_EVENT_INTERRUPT should
*   NOT set for this option. And RTC_MODE_EVENT_INTERRUPT should be enable.)
*
*  RTC_MODE_EVERY_YEAR_INTERRUPT mode will trigger rtc "year interrupt" for every
* "year" if RTC_MODE_EN_YEAR_INTERRUPT mode set.
*
*  RTC_MODE_MATCH_YEAR_INTERRUPT mode will only trigger rtc "year interrupt"
* "current year" = "alarm year" once per century, if RTC_MODE_EN_YEAR_INTERRUPT mode set.
*
*  RTC_MODE_YEAR_EVENT_INTERRUPT will generate "event interupt" for MON:DAY HH:MM:SS matched.
*  (RTC_MODE_MONTH_EVENT_INTERRUPT, RTC_MODE_DAY_EVENT_INTERRUPT, RTC_MODE_HOUR_EVENT_INTERRUPT,
*   RTC_MODE_MIN_EVENT_INTERRUPT and RTC_MODE_SEC_EVENT_INTERRUPT should
*   NOT set for this option.)
 */


#ifndef __RT584_RTC_H__
#define __RT584_RTC_H__

#ifdef __cplusplus
extern "C"
{
#endif




/**************************************************************************************************
 *    INCLUDES
 *************************************************************************************************/
#include "cm33.h"

/**************************************************************************************************
 *    CONSTANTS AND DEFINES
 *************************************************************************************************/
/**
 *  @Brief Interrupt configuration Definitions
 */
#define RTC_MODE_EVERY_SEC_INTERRUPT         (1<<0)         /*!< Geneate second interrupt for every new seconds   */
#define RTC_MODE_MATCH_SEC_INTERRUPT         (0<<0)         /*!< Geneate second interrupt only for matched second of every minutes   */
#define RTC_MODE_SECOND_EVENT_INTERRUPT      (1<<1)         /*!< Geneate event interrupt for every second  */

#define RTC_MODE_EVERY_MIN_INTERRUPT         (1<<2)         /*!< Geneate minute interrupt for every new minutes  (**:00)   */
#define RTC_MODE_MATCH_MIN_INTERRUPT         (0<<2)         /*!< Geneate minute interrupt only for matched minutes  (mm:00) of every hours, mm is the value set in alarm minute  */
#define RTC_MODE_MINUTE_EVENT_INTERRUPT      (1<<3)         /*!< Geneate event interrupt for matched (ss) of every minutes  */

#define RTC_MODE_EVERY_HOUR_INTERRUPT        (1<<4)         /*!< Geneate hour interrupt for every new hours (**:00:00)  */
#define RTC_MODE_MATCH_HOUR_INTERRUPT        (0<<4)         /*!< Geneate hour interrupt only for matched hours (hh:00:00) of every days, HH is the value set in alarm hour */
#define RTC_MODE_HOUR_EVENT_INTERRUPT        (1<<5)         /*!< Geneate event interrupt only for matched (mm:ss) of every hours  */

#define RTC_MODE_EVERY_DAY_INTERRUPT         (1<<6)         /*!< Geneate day interrupt for every new day (00:00:00)   */
#define RTC_MODE_MATCH_DAY_INTERRUPT         (0<<6)         /*!< Geneate day interrupt only for match day of months ( dd 00:00:00)   */
#define RTC_MODE_DAY_EVENT_INTERRUPT         (1<<7)         /*!< Geneate event interrupt for matched (hh:mm:ss) of every days  */

#define RTC_MODE_EVERY_MONTH_INTERRUPT       (1<<8)         /*!< Geneate month interrupt for every new month ( **:00 00:00:00)   */
#define RTC_MODE_MATCH_MONTH_INTERRUPT       (0<<8)         /*!< Geneate month interrupt only for matched month of years ( mm:00 00:00:00)   */
#define RTC_MODE_MONTH_EVENT_INTERRUPT       (1<<9)         /*!< Geneate event interrupt for matched (dd hh:mm:ss) of every months  */

#define RTC_MODE_EVERY_YEAR_INTERRUPT        (1<<10)        /*!< Geneate year interrupt for every new year ( **:00:00 00:00:00)   */
#define RTC_MODE_MATCH_YEAR_INTERRUPT        (0<<10)        /*!< Geneate year interrupt only for matched year of century ( YY:00:00 00:00:00)  */
#define RTC_MODE_YEAR_EVENT_INTERRUPT        (1<<11)        /*!< Geneate event interrupt for matched (mm:dd hh:mm:ss) of every years  */

#define RTC_MODE_EVERY_MSEC_INTERRUPT         (1<<12)         /*!< Geneate second interrupt for every new milliseconds   */
#define RTC_MODE_MATCH_MSEC_INTERRUPT         (0<<12)         /*!< Geneate second interrupt only for matched millisecond of every minutes   */
#define RTC_MODE_MSECOND_EVENT_INTERRUPT      (1<<13)         /*!< Geneate event interrupt for every millisecond  */

#define RTC_INTERRUPT_MASK_SHIFT             (16)
#define RTC_INTERRUPT_MASK                   (0xFF)

#define RTC_MODE_EN_SEC_INTERRUPT            (1<< RTC_INTERRUPT_MASK_SHIFT)             /*!< Geneate Second interrupt  */
#define RTC_MODE_EN_MIN_INTERRUPT            (1<<(RTC_INTERRUPT_MASK_SHIFT+1))          /*!< Geneate Minute interrupt  */
#define RTC_MODE_EN_HOUR_INTERRUPT           (1<<(RTC_INTERRUPT_MASK_SHIFT+2))          /*!< Geneate Hour   interrupt  */
#define RTC_MODE_EN_DAY_INTERRUPT            (1<<(RTC_INTERRUPT_MASK_SHIFT+3))          /*!< Geneate Day    interrupt  */
#define RTC_MODE_EN_MONTH_INTERRUPT          (1<<(RTC_INTERRUPT_MASK_SHIFT+4))          /*!< Geneate Month  interrupt  */
#define RTC_MODE_EN_YEAR_INTERRUPT           (1<<(RTC_INTERRUPT_MASK_SHIFT+5))          /*!< Geneate Year   interrupt  */
#define RTC_MODE_EVENT_INTERRUPT             (1<<(RTC_INTERRUPT_MASK_SHIFT+6))          /*!< Geneate Event  interrupt  */
#define RTC_MODE_EN_MSEC_INTERRUPT           (1<<(RTC_INTERRUPT_MASK_SHIFT+7))          /*!< Geneate MilliSecond  interrupt  */
#define RTC_MODE_EN_MSEC_INTERRUPT           (1<<(RTC_INTERRUPT_MASK_SHIFT+7))          /*!< Geneate MilliSecond  interrupt  */

#define RTC_IRQEVENT_SEC_SHIFT               (8)                        /*!< rtc Second shift bit. */
#define RTC_IRQEVENT_MIN_SHIFT               (6)                        /*!< rtc Minute shift bit. */
#define RTC_IRQEVENT_HOUR_SHIFT              (4)                        /*!< rtc Hour shift bit.   */
#define RTC_IRQEVENT_DAY_SHIFT               (2)                        /*!< rtc Day shift bit.    */
#define RTC_IRQEVENT_MONTH_SHIFT             (0)                        /*!< rtc Month shift bit.  */
#define RTC_IRQEVENT_YEAR_RSHIFT             (2)                        /*!< rtc Year shift bit.   */
#define RTC_IRQEVENT_MSEC_SHIFT              (0)

#define RTC_SEC_MASK                         (0x3<<0)           /*!< rtc Second mask.      */
#define RTC_MIN_MASK                         (0x3<<2)           /*!< rtc Minute mask.      */
#define RTC_HOUR_MASK                        (0x3<<4)           /*!< rtc Hour mask.        */
#define RTC_DAY_MASK                         (0x3<<6)           /*!< rtc Day mask.         */
#define RTC_MONTH_MASK                       (0x3<<8)           /*!< rtc Month mask.       */
#define RTC_YEAR_MASK                        (0x3<<10)          /*!< rtc Year mask.        */
#define RTC_MSEC_MASK                        (0x3<<12)

/**
 *  @Brief Interrupt status Definitions
 */
#define RTC_INT_SECOND_BIT                   (1<<0)             /*!< Indicate second interrupt flag  */
#define RTC_INT_MINUTE_BIT                   (1<<1)             /*!< Indicate minute interrupt flag  */
#define RTC_INT_HOUR_BIT                     (1<<2)             /*!< Indicate hour interrupt flag    */
#define RTC_INT_DAY_BIT                      (1<<3)             /*!< Indicate day interrupt flag  */
#define RTC_INT_MONTH_BIT                    (1<<4)             /*!< Indicate month interrupt flag  */
#define RTC_INT_YEAR_BIT                     (1<<5)             /*!< Indicate year interrupt flag  */
#define RTC_INT_EVENT_BIT                    (1<<6)             /*!< Indicate event interrupt flag  */
#define RTC_INT_MSECOND_BIT                  (1<<7)             /*!< Indicate millisecond interrupt flag  */

/**
 *  @Brief Let rt584 can use rt58x api
 */
#define rtc_get_time(tm)                                Rtc_Get_Time(tm)
#define rtc_set_time(tm)                                Rtc_Set_Time(tm)
#define rtc_get_alarm(tm)                               Rtc_Get_Alarm(tm)
#define rtc_set_alarm(tm,rtc_int_mode,rtc_usr_isr)      Rtc_Set_Alarm(tm,rtc_int_mode,rtc_usr_isr)
#define rtc_disable_alarm()                             Rtc_Disable_Alarm()
#define rtc_set_clk(clk)                                Rtc_Set_Clk(clk)
#define rtc_reset()                                     Rtc_Reset()


/**************************************************************************************************
 *    TYPEDEFS
 *************************************************************************************************/
/**
 * @brief RTC alarm routine callback to notify user that alarm timing event happen.
 *
 * @param rtc_status  rtc_status passed to user rtc alarm routine for the reason
 *                     RTC alarm routine called.
 *
 */
typedef void (*rtc_alarm_proc_cb)(uint32_t rtc_status);


/**
 * @brief RTC timer structure for RTC setting
 *
 *        Please Note each member of rtc_time_t is binary value.
 *        Invalid input parameter will make rtc to enter unknown/undetermine state.
 *        So please input valid date and time number.
 */
typedef struct
{
    uint32_t   tm_msec;              /*!< rtc second in binary  */
    uint32_t   tm_sec;              /*!< rtc second in binary  */
    uint32_t   tm_min;              /*!< rtc minute in binary  */
    uint32_t   tm_hour;             /*!< rtc hour in binary  */
    uint32_t   tm_day;              /*!< rtc day  in binary */
    uint32_t   tm_mon;              /*!< rtc month in binary  */
    uint32_t   tm_year;             /*!< rtc year in binary  */
} rtc_time_t;

/** @addtogroup Peripheral_Driver RT584Z Periperhal Driver Function
  @{
*/



/** @addtogroup RTC_DRIVER RTC Driver Functions
  @{
*/

/**************************************************************************************************
 *    GLOBAL PROTOTYPES
 *************************************************************************************************/
/**
 * @brief rtc_get_time. Return the current time read from RTC.
 *
 * @param tm:     specify pointer of rtc_time_t to get current rtc value.
 *
 * @return
 * @retval    STATUS_SUCCESS
 * @retval    STATUS_INVALID_PARAM --- tm is null pointer.
 *
 *
 */
uint32_t Rtc_Get_Time(rtc_time_t *tm);

/**
 * @brief rtc_set_time. Set the time in the RTC.
 *
 * @param tm:   specify pointer to rtc_time_t to set current rtc time.
 *
 * @retval    STATUS_SUCCESS
 * @retval    STATUS_INVALID_PARAM --- tm is null pointer.
 *
 * @details     Because RTC timing domain is running in 32K clock domain, calling this function may
 *             block maxium 1ms for take the value effect.
 *
 */
uint32_t Rtc_Set_Time(rtc_time_t *tm);

/**
 * @brief Use to get rtc alarm time.
 *
 * @param tm: Get Alarm time information.
 *
 * @retval    STATUS_SUCCESS
 * @retval    STATUS_INVALID_PARAM --- tm is null pointer.
 *
 *
 */
uint32_t Rtc_Get_Alarm(rtc_time_t *tm);

/**
 * @brief Use to set rtc alarm time.
 *
 * @param    tm:             set Alarm time information.
 * @param    rtc_int_mode:   set mode for interrupt generated.
 * @param    rtc_usr_isr:    when rtc interrupt happen it will call rtc_usr_isr to
 *            notify the interrupt happen.
 *
 * @retval    STATUS_SUCCESS
 * @retval    STATUS_INVALID_PARAM --- tm is NULL pointer or rtc_usr_isr is NULL.
 *
 * @details    Because RTC timing domain is running in 32K clock, calling this function may
 *            block maximum 1ms  for take the value effect.
 *            Calling this function will also enable RTC interrupt of cortex-m3.
 *
 */
uint32_t Rtc_Set_Alarm(
    rtc_time_t *tm,
    uint32_t rtc_int_mode,
    rtc_alarm_proc_cb rtc_usr_isr
);

/**
 * @brief Use to disable rtc alarm time.
 *
 *
 */
void Rtc_Disable_Alarm(void);

/**
 * @brief Use to set rtc ticks for second .
 *
 * @param clk: Set ticks for one second used for RTC counter.
 *
 * @details: Set rtc ticks will only take effect in next second start. It will not
 *         change current ticks in counting rtc. Adjust this value to match rtc clock.
 *
 */
void Rtc_Set_Clk(uint32_t clk);

/**
 * @brief Use to reset the RTC to default setting.
 *
 */
void Rtc_Reset(void);

/**
 * @brief Set the RTC to wakeup from deep sleep.
 *
 */
void Setup_RTC_Wakeup_From_Deep_Sleep(void);

/**
 * @brief  Get rtc status
 *
 * @retval    RTC Status
 *
 */
uint32_t Get_RTC_Status(void);

/**
 * @brief  Enable RTC.
 *
 * @retval      none
 */

void Rtc_Enable(void);


/**
 * @brief  Disable RTC.
 *
 * @retval      none
 */

void Rtc_Disable(void);


uint32_t Setup_RTC_Ms_Counter_Alarm(uint32_t unit_time, uint32_t prescale, rtc_alarm_proc_cb rtc_usr_isr);

uint32_t Get_RTC_Ms_Counter_Time(void);

/**@}*/ /* end of RTC_DRIVER group */

/**@}*/ /* end of PERIPHERAL_DRIVER group */


#ifdef __cplusplus
}
#endif

#endif /* end of __RT584_RTC_H__ */


