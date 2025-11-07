/**
 ******************************************************************************
 * @file    wdt.h
 * @author
 * @brief   watch dog timer driver header file
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


#ifndef __RT584_WDT_H__
#define __RT584_WDT_H__

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
 * @brief WDT prescale definitions.
 */
#define WDT_PRESCALE_1               0
#define WDT_PRESCALE_16              15
#define WDT_PRESCALE_32              31
#define WDT_PRESCALE_128             127
#define WDT_PRESCALE_256             255
#define WDT_PRESCALE_1024            1023
#define WDT_PRESCALE_4096            4095

/**
 * @brief WDT Kick value definitions.
 */
#define WDT_KICK_VALUE         0xA5A5
/**************************************************************************************************
 *    TYPEDEFS
 *************************************************************************************************/

/**
 * @brief rt58x API
 */

/**
 * @brief WDT TIMER interrupt service routine callback for user application, it is for rt58x.
 *
 *
 */
typedef void (*wdt_isr_handler_t)();

/**@brief wdt config structure holding configuration settings for the wdt timer, it is for rt58x.
 */
typedef struct
{
    uint8_t    int_enable     : 1;
    uint8_t    reset_enable   : 1;
    uint8_t    lock_enable    : 1;
    uint8_t    prescale       : 3;
} wdt_config_mode_t;
/**
 * @brief End of rt58x API
 */

/**
 * @brief WDT TIMER interrupt service routine callback for user application.
 */
typedef void (*wdt_proc_cb)();

/**
 * @brief wdt config structure holding configuration settings for the wdt timer.
 */
typedef struct
{
    uint8_t    int_enable     : 1;
    uint8_t    reset_enable   : 1;
    uint8_t    lock_enable    : 1;
    uint8_t    prescale       : 3;
    uint16_t   sp_prescale    : 11;
} wdt_config_mode_ex_t;


/**
 * @brief wdt config structure holding configuration settings for the wdt timer.
 */
typedef struct
{
    uint32_t    wdt_ticks;
    uint32_t    int_ticks;
    uint32_t    wdt_min_ticks;
} wdt_config_tick_t;

/** @addtogroup Peripheral_Driver RT584Z Periperhal Driver Function
  @{
*/

/** @addtogroup WDT_DRIVER WDT Driver Functions
  @{
*/

/**************************************************************************************************
 *    GLOBAL PROTOTYPES
 *************************************************************************************************/
/**
 * @brief rt58x API
 */
/**
 * @brief Watch dog Start
 * @param[in] wdt_mode config the watch dog
              \arg int_enable
              \arg reset_enable
              \arg lock_enable
              \arg prescale
 * @param[in] wdt_cfg_ticks config the watch dog ticks
              \arg wdt_ticks        setting the watchdog reset ticks
              \arg int_ticks        setting the watchdog interrupt ticks
              \arg wdt_min_ticks    setting the watchdog window min ticks
 * @param[in] wdt_handler register watch dog interrupt service routine callback
 * @return
 */
uint32_t Wdt_Start(
    wdt_config_mode_t wdt_mode,
    wdt_config_tick_t wdt_cfg_ticks,
    wdt_isr_handler_t wdt_handler);
/**
 * @brief End of rt58x API
 */



/*Remark:
 *   WDT interrupt is "warning interrupt", that notify system it will reset soon.
 * so int_ticks should be less that wdt_ticks and should have enough time for
 * "reset error" handling.. int_ticks can NOT greater or equal then wdt_ticks.
 *
 *   If you set lock_enable mode, you can NOT change the watchdog setting anymore
 * before reset (after WDT lock_enable bit has been set). That is you must kick
 * the watchdog periodic to prevent reset event.
 *
 * Example:
 * Clock is 32M Hz. Set prescale is 32.. so WDT is 1M Hz.
 * If we set wdt_ticks 500000, and int_ticks 300
 * then we should kick WDT within 0.5 second, otherwise
 * the whole system will reset (if reset enable) after 0.5s
 * from the last kick and before reset. And before reset,
 * a warning reset interrupt will generate, system has 300 us
 * to save some important data or something..
 * We can ignore int_ticks and wdt_handler, if we don't set
 * int_enable mode.
 *
 */
/**
 * @brief register Watch dog interrupt callback function
 *
 * @param[in] wdt_handler
 *
 * @return None
*/
extern void Wdt_Register_Callback(wdt_proc_cb wdt_handler);

/**
 * @brief Watch dog Start
 *
 * @param[in] wdt_mode config the watch dog
              \arg int_enable
              \arg reset_enable
              \arg lock_enable
              \arg prescale
 * @param[in] wdt_cfg_ticks config the watch dog ticks
              \arg wdt_ticks        setting the watchdog reset ticks
              \arg int_ticks        setting the watchdog interrupt ticks
              \arg wdt_min_ticks    setting the watchdog window min ticks
 * @param[in] wdt_handler register watch dog interrupt service routine callback
 *
 * @retval STATUS_INVALID_REQUEST   watchdog control lock mode is true, then can't clear the watchdog enable flag
 * @retval STATUS_INVALID_PARAM     watchdog control parameters are invalid
 * @retval STATUS_SUCCESS           watchdog enable is successful
 */
uint32_t Wdt_Start_Ex(
    wdt_config_mode_ex_t wdt_mode,
    wdt_config_tick_t wdt_cfg_ticks,
    wdt_proc_cb wdt_handler);

/**
 * @brief Stop the watchdog
 *
 * @retval STATUS_INVALID_REQUEST   watchdog control lock mode is true, then can't clear the watchdog enable flag
 * @retval STATUS_SUCCESS           clear the watchdog enable flag is successful
 */
uint32_t Wdt_Stop(void);

/**
 * @brief Get watchdog reset event value
 * @return Watchdog reset event value
 */
__STATIC_INLINE uint32_t Wdt_Reset_Event_Get(void)
{
    return WDT->RST_OCCUR.bit.RESET_OCCUR;
}

/**
 * @brief Clear Watchdog Reset Event
 * @return None
 */
__STATIC_INLINE void Wdt_Reset_Event_Clear(void)
{
    WDT->RST_OCCUR.bit.RESET_OCCUR = 1;     /*Write clear.*/
}

/**
 * @brief Reload the watchdog Kick value
 * @return None
 */
__STATIC_INLINE void Wdt_Kick(void)
{
    WDT->WDT_KICK = WDT_KICK_VALUE;
}

/**
 * @brief Clear Watchdog interrupt flag
 * @return Watch dog value
 */
__STATIC_INLINE void Wdt_Interrupt_Clear(void)
{
    WDT->CLEAR = 1;
}
/**
 * @brief Clear Watchdog interrupt flag
 * @return return Watchdog value
 */
__STATIC_INLINE uint32_t Wdt_Current_Get(void)
{
    return (WDT->VALUE);
}


/**@}*/ /* end of WDT_DRIVER group */

/**@}*/ /* end of PERIPHERAL_DRIVER group */


#ifdef __cplusplus
}
#endif

#endif      /* end of __RT584_WDT_H__ */
