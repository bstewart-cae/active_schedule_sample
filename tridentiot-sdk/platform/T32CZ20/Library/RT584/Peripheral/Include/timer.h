/**
 ******************************************************************************
 * @file    timer.h
 * @author
 * @brief   timer driver header file
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



#ifndef __RT584_TIMER_H__
#define __RT584_TIMER_H__

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
 * @brief  Timer counting mode definitions.
 */
#define TIMER_DOWN_COUNTING               0
#define TIMER_UP_COUNTING                 1

/**
 * @brief Timer one-shot mode definitions.
 */
#define TIMER_ONE_SHOT_DISABLE                0
#define TIMER_ONE_SHOT_ENABLE                 1

/**
 * @brief Timer mode definitions.
 */
#define TIMER_FREERUN_MODE             0
#define TIMER_PERIODIC_MODE            1

/**
 * @brief Timer interrupt definitions.
 */
#define TIMER_INT_DISABLE                0
#define TIMER_INT_ENABLE                 1

/**
 * @brief Timer prescale definitions.
 */
#define TIMER_PRESCALE_1               0
#define TIMER_PRESCALE_2               3
#define TIMER_PRESCALE_8               4
#define TIMER_PRESCALE_16              1
#define TIMER_PRESCALE_32              5
#define TIMER_PRESCALE_128             6
#define TIMER_PRESCALE_256             2
#define TIMER_PRESCALE_1024            7

/**
 * @brief Timer capture edge definitions.
 */
#define TIMER_CAPTURE_POS_EDGE             0
#define TIMER_CAPTURE_NEG_EDGE             1

/**
 * @brief Timer capture deglich definitions.
 */
#define TIMER_CAPTURE_DEGLICH_DISABLE            0
#define TIMER_CAPTURE_DEGLICH_ENABLE             1

/**
 * @brief Timer capture interrupt definitions.
 */
#define TIMER_CAPTURE_INT_DISABLE                0
#define TIMER_CAPTURE_INT_ENABLE                 1

/**
 * @brief Timer clock source definitions.
 */
#define TIMER_CLOCK_SOURCEC_PERI            0
#define TIMER_CLOCK_SOURCEC_RCO1M           2
#define TIMER_CLOCK_SOURCEC_PMU             3

/**************************************************************************************************
 *    TYPEDEFS
 *************************************************************************************************/

/**
 * @brief rt58x API
 */
/**
 * @brief TIMER interrupt service routine callback for user application, it is rt58x API.
 *
 * @param timer_id   is timer id for the interrupt source
 *
 */
typedef void (*timer_isr_handler_t)(uint32_t timer_id);

/**@brief timer config structure holding configuration settings for the timer, it is for RT58x.
 */
typedef struct
{
    uint8_t    mode     : 1;
    uint8_t    prescale : 3;
    uint8_t    int_en   : 1;
} timer_config_mode_t;
/**
 * @brief End of rt58x API
 */

/**
 * @brief TIMER interrupt service routine callback for user application.
 *
 * @param timer_id   is timer id for the interrupt source
 *
 *
 * @details    This callback function is still running in interrupt mode, so this function
 *              should be as short as possible. It can NOT call any block function in this
 *              callback service routine.
 */
typedef void (*timer_proc_cb)(uint32_t timer_id);

/**
 * @brief timer config structure holding configuration settings for the timer.
 */
typedef struct
{
    uint8_t     CountingMode : 1;       /*!< Set counting mode */
    uint8_t     OneShotMode : 1;        /*!< Enable one shot */
    uint8_t     Mode     : 1;           /*!< Set Freerun or Periodic mode */
    uint8_t     IntEnable   : 1;        /*!< Enable Interrupt */
    uint8_t     Prescale : 3;           /*!< Set prescale */
    uint16_t    UserPrescale : 10;      /*!< Set user define prescale */
} timer_config_mode_ex_t;

/**
 * @brief timer capture config structure holding configuration settings for the timer.
 */
typedef struct
{
    uint8_t     CountingMode : 1;                   /*!< Set counting mode */
    uint8_t     OneShotMode : 1;                    /*!< Enable one shot */
    uint8_t     Mode     : 1;                       /*!< Set Freerun or Periodic mode */
    uint8_t     IntEnable   : 1;                    /*!< Enable Interrupt */
    uint8_t     Prescale : 3;                       /*!< Set prescale */
    uint16_t    UserPrescale : 10;                  /*!< Set user define prescale */
    uint8_t     Channel0CaptureEdge : 1;            /*!< Set Capture channel0 trigger edge */
    uint8_t     Channel0DeglichEnable : 1;          /*!< Enable Capture channel0 deglitch */
    uint8_t     Channel0IntEnable : 1;              /*!< Enable Capture channel0 interrupt */
    uint8_t     Channel0IoSel : 5;                  /*!< Set Capture channel0 gpio */
    uint8_t     Channel1CaptureEdge : 1;            /*!< Set Capture channel1 trigger edge */
    uint8_t     Channel1DeglichEnable : 1;          /*!< Enable Capture channel1 deglitch */
    uint8_t     Channel1IntEnable : 1;              /*!< Enable Capture channel1 interrupt */
    uint8_t     Channel1IoSel : 5;                  /*!< Set Capture channel1 gpio */
} timer_capture_config_mode_t;

/**
 * @brief timer pwm config structure holding configuration settings for the timer.
 */
typedef struct
{
    uint8_t     CountingMode : 1;               /*!< Set counting mode */
    uint8_t     OneShotMode : 1;                /*!< Enable one shot */
    uint8_t     Mode     : 1;                   /*!< Set Freerun or Periodic mode */
    uint8_t     IntEnable   : 1;                /*!< Enable Interrupt */
    uint8_t     Prescale : 3;                   /*!< Set prescale */
    uint16_t    UserPrescale : 10;              /*!< Set user define prescale */
    uint8_t     Pwm0Enable : 1;                 /*!< Enable Pwm channel0 */
    uint8_t     Pwm1Enable : 1;                 /*!< Enable Pwm channel1 */
    uint8_t     Pwm2Enable : 1;                 /*!< Enable Pwm channel2 */
    uint8_t     Pwm3Enable : 1;                 /*!< Enable Pwm channel3 */
    uint8_t     Pwm4Enable : 1;                 /*!< Enable Pwm channel4 */
} timer_pwm_config_mode_t;

/**
 * @brief 32k timer config structure holding configuration settings for the timer.
 */
typedef struct
{
    uint8_t     CountingMode : 1;               /*!< Set counting mode */
    uint8_t     OneShotMode : 1;                /*!< Enable one shot */
    uint8_t     Mode     : 1;                   /*!< Set Freerun or Periodic mode */
    uint8_t     IntEnable   : 1;                /*!< Enable Interrupt */
    uint8_t     Prescale : 3;                   /*!< Set prescale */
    uint16_t    UserPrescale : 10;              /*!< Set user define prescale */
    uint16_t    RepeatDelay;                    /*!< Set repeat delay count */
} timer_32k_config_mode_t;

/** @addtogroup Peripheral_Driver RT584Z Periperhal Driver Function
  @{
*/



/** @addtogroup TIMER_DRIVER TIMER Driver Functions
  @{
*/


/**************************************************************************************************
 *    GLOBAL PROTOTYPES
 *************************************************************************************************/
/**
 * @brief rt58x API
 */
/**
 * @brief Register user interrupt ISR callback function.
 *
 * @param[in]    timer_id               Specifies the timer id.
 * @param[in]    timer_callback         Specifies user callback function when the timer interrupt generated.
 *
 * @retval      none
 */
void Timer_Int_Callback_Register(uint32_t timer_id, timer_isr_handler_t timer_handler);

/**
 * @brief  set timer parameter, it is rt58x API
 *
 * @param[in]       timer_id            Specifies the timer id.
 * @param[in]       cfg                 Config parameter
 * @param[in]       timer_callback      Callback function
 *
 * @retval  0    STATUS_SUCCESS
 * @retval  1    STATUS_INVALID_PARAM
 * @retval  2    STATUS_INVALID_REQUEST
 */
uint32_t Timer_Open(uint32_t timer_id, timer_config_mode_t cfg, timer_isr_handler_t timer_callback);

/**
 * @brief  load timer ticks, it is 58x api
 *
 * @param[in]       timer_id            Specifies the timer id.
 * @param[in]       timeload_ticks      Timer reload tick
 * @param[in]       timeout_ticks       Timer timeout tick
 *
 * @retval  0    STATUS_SUCCESS
 * @retval  1    STATUS_INVALID_PARAM
 */
uint32_t Timer_Load(uint32_t timer_id, uint32_t timeout_ticks);

/**
 * @brief  start timer, it is 58x api
 *
 * @param[in]       timer_id            Specifies the timer id.
 * @param[in]       timeout_ticks       Timer timeout tick
 *
 * @retval  0    STATUS_SUCCESS
 * @retval  1    STATUS_INVALID_PARAM
 * @retval  4    STATUS_NO_INIT
 */
uint32_t Timer_Start(uint32_t timer_id, uint32_t timeout_ticks);

/**
 * @brief  stop timer, it is 58x api
 *
 * @param[in]       timer_id        Specifies the timer id.
 *
 * @retval 0    STATUS_SUCCESS
 * @retval 1    STATUS_INVALID_PARAM
 * @retval 4    STATUS_NO_INIT
 */
uint32_t Timer_Stop(uint32_t timer_id);

/**
 * @brief  clear timer setting to close, it is 58x api
 *
 * @param[in]       timer_id        Specifies the timer id.
 *
 * @retval 0    STATUS_SUCCESS
 * @retval 1    STATUS_INVALID_PARAM
 */
uint32_t Timer_Close(uint32_t timer_id);

/**
 * @brief  get the timer interrupt status, it is 58x api
 *
 * @param[in]       timer_id        Specifies the timer id.
 *
 * @retval interrupt status
 */
uint32_t Timer_Status_Get(uint32_t timer_id);

/**
 * @brief  get the timer current value, it is 58x api
 *
 * @param[in]       timer_id        Specifies the timer id.
 *
 * @retval current value
 */
uint32_t Timer_Current_Get(uint32_t timer_id);
/**
 * @brief end of rt58x API
 */
/**
 * @brief Register user interrupt ISR callback function.
 *
 * @param[in]    timer_id               Specifies the timer id.
 * @param[in]    timer_callback         Specifies user callback function when the timer interrupt generated.
 *
 * @retval      none
 */
void Timer_Callback_Register(uint32_t timer_id, timer_proc_cb timer_callback);

/**
 * @brief  get timer enable status
 *
 * @param[in]    timer_id               Specifies the timer id.
 *
 * @retval 0    enable
 * @retval 1    disable
 */
uint32_t Get_Timer_Enable_Status(uint32_t timer_id);

/**
 * @brief  set timer parameter
 *
 * @param[in]       timer_id            Specifies the timer id.
 * @param[in]       cfg                 Config parameter
 * @param[in]       timer_callback      Callback function
 *
 * @retval  0    STATUS_SUCCESS
 * @retval  1    STATUS_INVALID_PARAM
 * @retval  2    STATUS_INVALID_REQUEST
 */
uint32_t Timer_Open_Ex(uint32_t timer_id, timer_config_mode_ex_t cfg, timer_proc_cb timer_callback);

/**
 * @brief  load timer ticks
 *
 * @param[in]       timer_id            Specifies the timer id.
 * @param[in]       timeload_ticks      Timer reload tick
 * @param[in]       timeout_ticks       Timer timeout tick
 *
 * @retval  0    STATUS_SUCCESS
 * @retval  1    STATUS_INVALID_PARAM
 */
uint32_t Timer_Load_Ex(uint32_t timer_id, uint32_t timeload_ticks, uint32_t timeout_ticks);

/**
 * @brief  start timer
 *
 * @param[in]       timer_id            Specifies the timer id.
 * @param[in]       timeload_ticks      Timer reload tick
 * @param[in]       timeout_ticks       Timer timeout tick
 *
 * @retval  0    STATUS_SUCCESS
 * @retval  1    STATUS_INVALID_PARAM
 * @retval  4    STATUS_NO_INIT
 */
uint32_t Timer_Start_Ex(uint32_t timer_id, uint32_t timeload_ticks, uint32_t timeout_ticks);

/**
 * @brief  stop timer
 *
 * @param[in]       timer_id        Specifies the timer id.
 *
 * @retval 0    STATUS_SUCCESS
 * @retval 1    STATUS_INVALID_PARAM
 * @retval 4    STATUS_NO_INIT
 */
uint32_t Timer_Stop_Ex(uint32_t timer_id);

/**
 * @brief  clear timer setting to close
 *
 * @param[in]       timer_id        Specifies the timer id.
 *
 * @retval 0    STATUS_SUCCESS
 * @retval 1    STATUS_INVALID_PARAM
 */
uint32_t Timer_Close_Ex(uint32_t timer_id);

/**
 * @brief  get the timer interrupt status
 *
 * @param[in]       timer_id        Specifies the timer id.
 *
 * @retval interrupt status
 */
uint32_t Timer_IntStatus_Get(uint32_t timer_id);

/**
 * @brief  get the timer current value
 *
 * @param[in]       timer_id        Specifies the timer id.
 *
 * @retval current value
 */
uint32_t Timer_Current_Get_Ex(uint32_t timer_id);

/**
 * @brief  set timer capture parameter
 *
 * @param[in]       timer_id            Specifies the timer id.
 * @param[in]       cfg                 Capture config parameter
 * @param[in]       timer_callback      Callback function
 *
 * @retval  0    STATUS_SUCCESS
 * @retval  1    STATUS_INVALID_PARAM
 * @retval  2    STATUS_INVALID_REQUEST
 */
uint32_t Timer_Capture_Open(uint32_t timer_id,
                            timer_capture_config_mode_t cfg,
                            timer_proc_cb timer_callback);

/**
 * @brief  start timer capture
 *
 * @param[in]       timer_id            Specifies the timer id.
 * @param[in]       timeload_ticks      Timer reload tick
 * @param[in]       timeout_ticks       Timer timeout tick
 * @param[in]       chanel0_enable      Enable channel0
 * @param[in]       chanel1_enable      Enable channel1
 *
 * @retval 0    STATUS_SUCCESS
 * @retval 1    STATUS_INVALID_PARAM
 * @retval 4    STATUS_NO_INIT
 */
uint32_t Timer_Capture_Start(uint32_t timer_id, uint32_t timeload_ticks, uint32_t timeout_ticks, bool chanel0_enable, bool chanel1_enable);

/**
 * @brief  get the timer capture channel0 current value
 *
 * @param[in]       timer_id        Specifies the timer id.
 *
 * @retval The timer capture current value
 */
uint32_t Timer_Ch0_Capture_Value_Get(uint32_t timer_id);

/**
 * @brief  get the timer capture channel0 interrupt status
 *
 * @param[in]       timer_id        Specifies the timer id.
 *
 * @retval The timer capture interrupt status
 */
uint32_t Timer_Ch0_Capture_Int_Status(uint32_t timer_id);

/**
 * @brief  get the timer capture channel1 current value
 *
 * @param[in]       timer_id        Specifies the timer id.
 *
 * @retval The timer capture current value
 */
uint32_t Timer_Ch1_Capture_Value_Get(uint32_t timer_id);

/**
 * @brief  get the timer capture channel1 interrupt status
 *
 * @param[in]       timer_id        Specifies the timer id.
 *
 * @retval The timer capture interrupt status
 */
uint32_t Timer_Ch1_Capture_Int_Status(uint32_t timer_id);

/**
 * @brief  set timer pwm parameter
 *
 * @param[in]       timer_id        Specifies the timer id.
 * @param[in]       cfg             Pwm config parameter
 *
 * @retval  0    STATUS_SUCCESS
 * @retval  1    STATUS_INVALID_PARAM
 * @retval  2    STATUS_INVALID_REQUEST
 */
uint32_t Timer_Pwm_Open(uint32_t timer_id, timer_pwm_config_mode_t cfg);

/**
 * @brief  start pwm timer
 *
 * @param[in]       timer_id            Specifies the timer id.
 * @param[in]       timeload_ticks      Timer reload tick
 * @param[in]       timeout_ticks       Timer timeout tick
 * @param[in]       threshold           The pwm change phase threshold
 * @param[in]       phase               The pwm start phase
 *
 * @retval 0    STATUS_SUCCESS
 * @retval 1    STATUS_INVALID_PARAM
 * @retval 4    STATUS_NO_INIT
 */
uint32_t Timer_Pwm_Start(uint32_t timer_id, uint32_t timeload_ticks, uint32_t timeout_ticks, uint32_t threshold, bool phase);

/**
 * @brief  stop pwm timer
 *
 * @param[in]       timer_id        Specifies the timer id.
 *
 * @retval 0    STATUS_SUCCESS
 * @retval 1    STATUS_INVALID_PARAM
 * @retval 4    STATUS_NO_INIT
 */
uint32_t Timer_Pwm_Stop(uint32_t timer_id);

/**
 * @brief Register user interrupt ISR callback function.
 *
 * @param[in]    timer_id               Specifies the 32k timer id.
 * @param[in]    timer_callback         Specifies user callback function when the timer interrupt generated.
 *
 * @retval      none
 */
void Timer_32k_Int_Callback_Register(uint32_t timer_id, timer_proc_cb timer_callback);

/**
 * @brief  get 32k timer enable status
 *
 * @param[in]    timer_id               Specifies the 32k timer id.
 *
 * @retval 0    enable
 * @retval 1    disable
 */
uint32_t Get_Timer32k_Enable_Status(uint32_t timer_id);

/**
 * @brief  set 32k timer parameter
 *
 * @param[in]       timer_id            Specifies the 32k timer id.
 * @param[in]       cfg                 Config parameter
 * @param[in]       timer_callback      Callback function
 *
 * @retval  0    STATUS_SUCCESS
 * @retval  1    STATUS_INVALID_PARAM
 * @retval  2    STATUS_INVALID_REQUEST
 */
uint32_t Timer_32k_Open(uint32_t timer_id,
                        timer_32k_config_mode_t mode,
                        timer_proc_cb timer_callback);

/**
 * @brief  load 32k timer ticks
 *
 * @param[in]       timer_id            Specifies the 32k timer id.
 * @param[in]       timeload_ticks      Timer reload tick
 * @param[in]       timeout_ticks       Timer timeout tick
 *
 * @retval  0    STATUS_SUCCESS
 * @retval  1    STATUS_INVALID_PARAM
 */
uint32_t Timer_32k_Load(uint32_t timer_id, uint32_t timeload_ticks, uint32_t timeout_ticks);

/**
 * @brief  start 32k timer
 *
 * @param[in]       timer_id            Specifies the 32k timer id.
 * @param[in]       timeload_ticks      Timer reload tick
 * @param[in]       timeout_ticks       Timer timeout tick
 *
 * @retval  0    STATUS_SUCCESS
 * @retval  1    STATUS_INVALID_PARAM
 * @retval  4    STATUS_NO_INIT
 */
uint32_t Timer_32k_Start(uint32_t timer_id, uint32_t timeload_ticks, uint32_t timeout_ticks);

/**
 * @brief  stop 32k timer
 *
 * @param[in]       timer_id        Specifies the 32k timer id.
 *
 * @retval 0    STATUS_SUCCESS
 * @retval 1    STATUS_INVALID_PARAM
 * @retval 4    STATUS_NO_INIT
 */
uint32_t Timer_32k_Stop(uint32_t timer_id);

/**
 * @brief  clear 32k timer setting to close
 *
 * @param[in]       timer_id        Specifies the 32k timer id.
 *
 * @retval 0    STATUS_SUCCESS
 * @retval 1    STATUS_INVALID_PARAM
 */
uint32_t Timer_32k_Close(uint32_t timer_id);

/**
 * @brief  get the 32k timer interrupt status
 *
 * @param[in]       timer_id        Specifies the 32k timer id.
 *
 * @retval interrupt status
 */
uint32_t Timer_32k_IntStatus_Get(uint32_t timer_id);

/**
 * @brief  get the 32k timer current value
 *
 * @param[in]       timer_id        Specifies the 32k timer id.
 *
 * @retval current value
 */
uint32_t Timer_32k_Current_Get(uint32_t timer_id);


/**@}*/ /* end of TIMER_DRIVER group */

/**@}*/ /* end of PERIPHERAL_DRIVER group */

#ifdef __cplusplus
}
#endif

#endif      /* end of __RT584_TIMER_H__ */

