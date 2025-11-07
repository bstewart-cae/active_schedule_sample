/**
 ******************************************************************************
 * @file    timer.c
 * @author
 * @brief   timer driver file
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
#include "timer.h"
#include "cm33.h"

#define MAX_NUMBER_OF_TIMER           3         /*3 timer*/
#define MAX_NUMBER_OF_TIMER_32K       2         /*2 timer*/

#define TIMER_STATE_OPEN           1
#define TIMER_STATE_CLOSED         0

/**
 * @brief timer_cb save callback and the timer state
 */
typedef struct
{
    timer_proc_cb  timer_callback;           /**< user application ISR handler. */
    uint8_t        state : 2;               /**< device state. */
} timer_cb;

static timer_cb timer_cfg[MAX_NUMBER_OF_TIMER];

/**
 * @brief timer_32k_cb save callback and the timer state
 */
typedef struct
{
    timer_proc_cb  timer_callback;           /**< user application ISR handler. */
    uint8_t        state : 2;               /**< device state. */
} timer_32k_cb;

static timer_32k_cb timer_32k_cfg[MAX_NUMBER_OF_TIMER];

/**
 * @brief rt58x API
 */
void Timer_Int_Callback_Register(uint32_t timer_id, timer_isr_handler_t timer_handler)
{
    if (timer_id < MAX_NUMBER_OF_TIMER)
    {
        //timer0/1/2, down count
        Timer_Callback_Register(timer_id, (timer_proc_cb)timer_handler);
    }
    else if ( (timer_id >= MAX_NUMBER_OF_TIMER) && (timer_id < (MAX_NUMBER_OF_TIMER + MAX_NUMBER_OF_TIMER_32K)) )
    {
        //time32k0/1
        uint32_t timer32k_id;

        timer32k_id = timer_id - MAX_NUMBER_OF_TIMER;
        Timer_32k_Int_Callback_Register(timer32k_id, (timer_proc_cb)timer_handler);
    }

    return;
}

uint32_t Timer_Open(uint32_t timer_id, timer_config_mode_t cfg, timer_isr_handler_t timer_callback)
{
    uint32_t status;

    if (timer_id < MAX_NUMBER_OF_TIMER)
    {
        timer_config_mode_ex_t cfg_ex;

        //timer0/1/2, down count
        if (timer_cfg[timer_id].state != TIMER_STATE_CLOSED)
        {
            return STATUS_INVALID_REQUEST;    /*device already opened*/
        }

        cfg_ex.CountingMode = TIMER_DOWN_COUNTING;
        cfg_ex.IntEnable = cfg.int_en;
        cfg_ex.Mode = cfg.mode;
        cfg_ex.OneShotMode = TIMER_ONE_SHOT_DISABLE;
        cfg_ex.Prescale = cfg.prescale;
        cfg_ex.UserPrescale = 0;

        status = Timer_Open_Ex(timer_id, cfg_ex, (timer_proc_cb)timer_callback);

        return status;
    }
    else if ( (timer_id >= MAX_NUMBER_OF_TIMER) && (timer_id < (MAX_NUMBER_OF_TIMER + MAX_NUMBER_OF_TIMER_32K)) )
    {
        //time32k0/1
        uint32_t timer32k_id;
        timer_32k_config_mode_t cfg_32k;

        timer32k_id = timer_id - MAX_NUMBER_OF_TIMER;
        if (timer_32k_cfg[timer32k_id].state != TIMER_STATE_CLOSED)
        {
            return STATUS_INVALID_REQUEST;    /*device already opened*/
        }

        cfg_32k.CountingMode = TIMER_DOWN_COUNTING;
        cfg_32k.IntEnable = cfg.int_en;
        cfg_32k.Mode = cfg.mode;
        cfg_32k.OneShotMode = TIMER_ONE_SHOT_DISABLE;
        cfg_32k.Prescale = cfg.prescale;
        cfg_32k.UserPrescale = 0;
        cfg_32k.RepeatDelay = 0;

        status =  Timer_32k_Open(timer32k_id, cfg_32k, (timer_proc_cb)timer_callback);

        return status;
    }
    else
    {
        return STATUS_INVALID_PARAM;
    }

}


uint32_t Timer_Load(uint32_t timer_id, uint32_t timeout_ticks)
{
    uint32_t status;

    if (timer_id < MAX_NUMBER_OF_TIMER)
    {
        //timer0/1/2, down count
        status = Timer_Load_Ex(timer_id, timeout_ticks, 0);
        return status;
    }
    else if ( (timer_id >= MAX_NUMBER_OF_TIMER) && (timer_id < (MAX_NUMBER_OF_TIMER + MAX_NUMBER_OF_TIMER_32K)) )
    {
        //time32k0/1
        uint32_t timer32k_id;

        timer32k_id = timer_id - MAX_NUMBER_OF_TIMER;
        status = Timer_32k_Load(timer32k_id, timeout_ticks, 0);

        return status;
    }
    else
    {
        return STATUS_INVALID_PARAM;
    }
}

uint32_t Timer_Start(uint32_t timer_id, uint32_t timeout_ticks)
{
    uint32_t status;

    if (timer_id < MAX_NUMBER_OF_TIMER)
    {
        //timer0/1/2, down count
        status = Timer_Start_Ex(timer_id, timeout_ticks, 0);

        return status;
    }
    else if ( (timer_id >= MAX_NUMBER_OF_TIMER) && (timer_id < (MAX_NUMBER_OF_TIMER + MAX_NUMBER_OF_TIMER_32K)) )
    {
        //time32k0/1
        uint32_t timer32k_id;

        timer32k_id = timer_id - MAX_NUMBER_OF_TIMER;
        status = Timer_32k_Start(timer32k_id, timeout_ticks, 0);

        return status;
    }
    else
    {
        return STATUS_INVALID_PARAM;
    }
}

uint32_t Timer_Stop(uint32_t timer_id)
{
    uint32_t status;

    if (timer_id < MAX_NUMBER_OF_TIMER)
    {
        //timer0/1/2, down count
        status = Timer_Stop_Ex(timer_id);

        return status;
    }
    else if ( (timer_id >= MAX_NUMBER_OF_TIMER) && (timer_id < (MAX_NUMBER_OF_TIMER + MAX_NUMBER_OF_TIMER_32K)) )
    {
        //time32k0/1
        uint32_t timer32k_id;

        timer32k_id = timer_id - MAX_NUMBER_OF_TIMER;
        status = Timer_32k_Stop(timer32k_id);

        return status;
    }
    else
    {
        return STATUS_INVALID_PARAM;
    }
}

uint32_t Timer_Close(uint32_t timer_id)
{
    uint32_t status;

    if (timer_id < MAX_NUMBER_OF_TIMER)
    {
        //timer0/1/2, down count
        status = Timer_Close_Ex(timer_id);

        return status;
    }
    else if ( (timer_id >= MAX_NUMBER_OF_TIMER) && (timer_id < (MAX_NUMBER_OF_TIMER + MAX_NUMBER_OF_TIMER_32K)) )
    {
        //time32k0/1
        uint32_t timer32k_id;

        timer32k_id = timer_id - MAX_NUMBER_OF_TIMER;
        status = Timer_32k_Close(timer32k_id);

        return status;
    }
    else
    {
        return STATUS_INVALID_PARAM;
    }
}

uint32_t Timer_Status_Get(uint32_t timer_id)
{
    uint32_t status;

    if (timer_id < MAX_NUMBER_OF_TIMER)
    {
        //timer0/1/2, down count
        status = Timer_IntStatus_Get(timer_id);

        return status;
    }
    else if ( (timer_id >= MAX_NUMBER_OF_TIMER) && (timer_id < (MAX_NUMBER_OF_TIMER + MAX_NUMBER_OF_TIMER_32K)) )
    {
        //time32k0/1
        uint32_t timer32k_id;

        timer32k_id = timer_id - MAX_NUMBER_OF_TIMER;
        status = Timer_32k_IntStatus_Get(timer32k_id);

        return status;
    }
    else
    {
        return STATUS_INVALID_PARAM;
    }
}

uint32_t Timer_Current_Get(uint32_t timer_id)
{
    uint32_t status;

    if (timer_id < MAX_NUMBER_OF_TIMER)
    {
        //timer0/1/2, down count
        status = Timer_Current_Get_Ex(timer_id);

        return status;
    }
    else if ( (timer_id >= MAX_NUMBER_OF_TIMER) && (timer_id < (MAX_NUMBER_OF_TIMER + MAX_NUMBER_OF_TIMER_32K)) )
    {
        //time32k0/1
        uint32_t timer32k_id;

        timer32k_id = timer_id - MAX_NUMBER_OF_TIMER;
        status = Timer_32k_Current_Get(timer32k_id);

        return status;
    }
    else
    {
        return STATUS_INVALID_PARAM;
    }
}
/**
 * @brief end of rt58x API
 */



void Timer_Callback_Register(uint32_t timer_id, timer_proc_cb timer_callback)
{
    timer_cfg[timer_id].timer_callback = timer_callback;

    return;
}

uint32_t Get_Timer_Enable_Status(uint32_t timer_id)
{
    TIMERN_T *timer;
    TIMERN_T *base[MAX_NUMBER_OF_TIMER] = {TIMER0, TIMER1, TIMER2};

    if (timer_id > MAX_NUMBER_OF_TIMER)
    {
        return STATUS_INVALID_PARAM;
    }

    timer = base[timer_id];

    return (timer->CONTROL.bit.TIMER_ENABLE_STATUS);
}

uint32_t Timer_Open_Ex(uint32_t timer_id, timer_config_mode_ex_t cfg, timer_proc_cb timer_callback)
{
    TIMERN_T *timer;
    TIMERN_T *base[MAX_NUMBER_OF_TIMER] = {TIMER0, TIMER1, TIMER2};


    if (timer_id > MAX_NUMBER_OF_TIMER)
    {
        return STATUS_INVALID_PARAM;
    }

    if (timer_cfg[timer_id].state != TIMER_STATE_CLOSED)
    {
        return STATUS_INVALID_REQUEST;    /*device already opened*/
    }

    timer = base[timer_id];

    timer->CONTROL.reg = 0;
    while ( Get_Timer_Enable_Status(timer_id) );
    timer->CLEAR = 1;

    if (cfg.UserPrescale)
    {
        timer->PRESCALE = cfg.UserPrescale;
    }
    else
    {
        timer->PRESCALE = 0;
        timer->CONTROL.bit.PRESCALE = cfg.Prescale;
    }

    timer->CONTROL.bit.UP_CONUNT = cfg.CountingMode;
    timer->CONTROL.bit.ONE_SHOT_EN = cfg.OneShotMode;
    timer->CONTROL.bit.MODE = cfg.Mode;
    timer->CONTROL.bit.INT_ENABLE = cfg.IntEnable;

    /*
     * Because of watchdog reset, it is possible that "last time" timer interrupt pending
     * in system, so we should set callback before enable interrupt, otherwise when
     * we call NVIC_EnableIRQ(...), it will trigger interrupt at the same time and jump
     * an invalid address such that hardware fault generated.
     */
    Timer_Callback_Register(timer_id, timer_callback);
    NVIC_EnableIRQ((IRQn_Type)(Timer0_IRQn + timer_id));

    timer_cfg[timer_id].state = TIMER_STATE_OPEN;

    return STATUS_SUCCESS;
}

uint32_t Timer_Load_Ex(uint32_t timer_id, uint32_t timeload_ticks, uint32_t timeout_ticks)
{
    TIMERN_T *timer;
    TIMERN_T *base[MAX_NUMBER_OF_TIMER] = {TIMER0, TIMER1, TIMER2};

    if (timer_id > MAX_NUMBER_OF_TIMER)
    {
        return STATUS_INVALID_PARAM;
    }

    timer = base[timer_id];
    timer->LOAD = (timeload_ticks - 1);
    timer->EXPRIED_VALUE = timeout_ticks;
    while ( !((timer->LOAD == timer->VALUE) && (timer->LOAD == (timeload_ticks - 1))) );

    return STATUS_SUCCESS;
}

uint32_t Timer_Start_Ex(uint32_t timer_id, uint32_t timeload_ticks, uint32_t timeout_ticks)
{
    TIMERN_T *timer;
    TIMERN_T *base[MAX_NUMBER_OF_TIMER] = {TIMER0, TIMER1, TIMER2};

    if (timer_id > MAX_NUMBER_OF_TIMER)
    {
        return STATUS_INVALID_PARAM;
    }

    if (timer_cfg[timer_id].state != TIMER_STATE_OPEN)
    {
        return STATUS_NO_INIT;    /*DEVIC SHOULD BE OPEN FIRST.*/
    }

    timer = base[timer_id];

    Timer_Load_Ex(timer_id, timeload_ticks, timeout_ticks);
    timer->CONTROL.bit.EN = 1;

    return STATUS_SUCCESS;
}

uint32_t Timer_Stop_Ex(uint32_t timer_id)
{
    TIMERN_T *timer;
    TIMERN_T *base[MAX_NUMBER_OF_TIMER] = {TIMER0, TIMER1, TIMER2};

    if (timer_id > MAX_NUMBER_OF_TIMER)
    {
        return STATUS_INVALID_PARAM;
    }

    if (timer_cfg[timer_id].state != TIMER_STATE_OPEN)
    {
        return STATUS_NO_INIT;    /*DEVIC SHOULD BE OPEN FIRST.*/
    }

    timer = base[timer_id];
    timer->CONTROL.bit.EN = 0;

    return STATUS_SUCCESS;
}

uint32_t Timer_Close_Ex(uint32_t timer_id)
{
    TIMERN_T *timer;
    TIMERN_T *base[MAX_NUMBER_OF_TIMER] = {TIMER0, TIMER1, TIMER2};

    if (timer_id > MAX_NUMBER_OF_TIMER)
    {
        return STATUS_INVALID_PARAM;
    }

    timer = base[timer_id];
    timer->CONTROL.reg = 0;

    NVIC_DisableIRQ((IRQn_Type)(Timer0_IRQn + timer_id));

    timer_cfg[timer_id].timer_callback = NULL;
    timer_cfg[timer_id].state = TIMER_STATE_CLOSED;

    return STATUS_SUCCESS;
}

uint32_t Timer_IntStatus_Get(uint32_t timer_id)
{
    TIMERN_T *timer;
    TIMERN_T *base[MAX_NUMBER_OF_TIMER] = {TIMER0, TIMER1, TIMER2};

    timer = base[timer_id];
    return (timer->CONTROL.bit.INT_STATUS);
}

uint32_t Timer_Current_Get_Ex(uint32_t timer_id)
{
    TIMERN_T *timer;
    TIMERN_T *base[MAX_NUMBER_OF_TIMER] = {TIMER0, TIMER1, TIMER2};

    timer = base[timer_id];

    return (timer->VALUE);
}

uint32_t Timer_Capture_Open(uint32_t timer_id,
                            timer_capture_config_mode_t cfg,
                            timer_proc_cb timer_callback)
{
    TIMERN_T *timer;
    TIMERN_T *base[MAX_NUMBER_OF_TIMER] = {TIMER0, TIMER1, TIMER2};


    if (timer_id > MAX_NUMBER_OF_TIMER)
    {
        return STATUS_INVALID_PARAM;
    }

    if (timer_cfg[timer_id].state != TIMER_STATE_CLOSED)
    {
        return STATUS_INVALID_REQUEST;    /*device already opened*/
    }

    timer = base[timer_id];

    timer->CONTROL.reg = 0;
    while ( Get_Timer_Enable_Status(timer_id) );
    timer->CLEAR = 1;
    timer->CAPTURE_CLEAR.bit.CH0_CAPTURE_INT_CLEAR = 1;
    timer->CAPTURE_CLEAR.bit.CH1_CAPTURE_INT_CLEAR = 1;

    if (cfg.UserPrescale)
    {
        timer->PRESCALE = cfg.UserPrescale;
    }
    else
    {
        timer->PRESCALE = 0;
        timer->CONTROL.bit.PRESCALE = cfg.Prescale;
    }

    timer->CONTROL.bit.UP_CONUNT = cfg.CountingMode;
    timer->CONTROL.bit.ONE_SHOT_EN = cfg.OneShotMode;
    timer->CONTROL.bit.MODE = cfg.Mode;
    timer->CONTROL.bit.INT_ENABLE = cfg.IntEnable;

    timer->CONTROL.bit.CH0_CAPTURE_EDGE = cfg.Channel0CaptureEdge;
    timer->CONTROL.bit.CH0_CAPTURE_INT_EN = cfg.Channel0IntEnable;
    timer->CONTROL.bit.CH0_DEGLICH_EN = cfg.Channel0DeglichEnable;
    timer->CAP_IO_SEL.bit.CH0_CAPTURE_IO_SEL = cfg.Channel0IoSel;

    timer->CONTROL.bit.CH1_CAPTURE_EDGE = cfg.Channel1CaptureEdge;
    timer->CONTROL.bit.CH1_CAPTURE_INT_EN = cfg.Channel1IntEnable;
    timer->CONTROL.bit.CH1_DEGLICH_EN = cfg.Channel1DeglichEnable;
    timer->CAP_IO_SEL.bit.CH1_CAPTURE_IO_SEL = cfg.Channel1IoSel;

    /*
     * Because of watchdog reset, it is possible that "last time" timer interrupt pending
     * in system, so we should set callback before enable interrupt, otherwise when
     * we call NVIC_EnableIRQ(...), it will trigger interrupt at the same time and jump
     * an invalid address such that hardware fault generated.
     */
    Timer_Callback_Register(timer_id, timer_callback);
    NVIC_EnableIRQ((IRQn_Type)(Timer0_IRQn + timer_id));

    timer_cfg[timer_id].state = TIMER_STATE_OPEN;

    return STATUS_SUCCESS;
}


uint32_t Timer_Capture_Start(uint32_t timer_id, uint32_t timeload_ticks, uint32_t timeout_ticks, bool chanel0_enable, bool chanel1_enable)
{
    TIMERN_T *timer;
    TIMERN_T *base[MAX_NUMBER_OF_TIMER] = {TIMER0, TIMER1, TIMER2};

    if (timer_id > MAX_NUMBER_OF_TIMER)
    {
        return STATUS_INVALID_PARAM;
    }

    if (timer_cfg[timer_id].state != TIMER_STATE_OPEN)
    {
        return STATUS_NO_INIT;    /*DEVIC SHOULD BE OPEN FIRST.*/
    }

    timer = base[timer_id];

    Timer_Load_Ex(timer_id, timeload_ticks, timeout_ticks);
    timer->CONTROL.bit.EN = 1;
    timer->CAP_EN.bit.CH0_CAPTURE_EN = chanel0_enable;
    timer->CAP_EN.bit.CH1_CAPTURE_EN = chanel1_enable;

    return STATUS_SUCCESS;
}

uint32_t Timer_Ch0_Capture_Value_Get(uint32_t timer_id)
{
    uint32_t value;
    TIMERN_T *timer;
    TIMERN_T *base[MAX_NUMBER_OF_TIMER] = {TIMER0, TIMER1, TIMER2};

    timer = base[timer_id];

    value = timer->CH0_CAP_VALUE;
    timer->CAPTURE_CLEAR.bit.CH0_CAPTURE_INT_CLEAR = 1;

    return (value);
}

uint32_t Timer_Ch0_Capture_Int_Status(uint32_t timer_id)
{
    TIMERN_T *timer;
    TIMERN_T *base[MAX_NUMBER_OF_TIMER] = {TIMER0, TIMER1, TIMER2};

    timer = base[timer_id];

    return (timer->CONTROL.bit.CH0_CAPTURE_INT_STATUS);
}

uint32_t Timer_Ch1_Capture_Value_Get(uint32_t timer_id)
{
    uint32_t value;
    TIMERN_T *timer;
    TIMERN_T *base[MAX_NUMBER_OF_TIMER] = {TIMER0, TIMER1, TIMER2};

    timer = base[timer_id];

    value = timer->CH1_CAP_VALUE;
    timer->CAPTURE_CLEAR.bit.CH1_CAPTURE_INT_CLEAR = 1;

    return (value);
}

uint32_t Timer_Ch1_Capture_Int_Status(uint32_t timer_id)
{

    TIMERN_T *timer;
    TIMERN_T *base[MAX_NUMBER_OF_TIMER] = {TIMER0, TIMER1, TIMER2};

    timer = base[timer_id];

    return (timer->CONTROL.bit.CH1_CAPTURE_INT_STATUS);
}

uint32_t Timer_Pwm_Open(uint32_t timer_id,
                        timer_pwm_config_mode_t cfg)
{
    TIMERN_T *timer;
    TIMERN_T *base[MAX_NUMBER_OF_TIMER] = {TIMER0, TIMER1, TIMER2};


    if (timer_id > MAX_NUMBER_OF_TIMER)
    {
        return STATUS_INVALID_PARAM;
    }

    if (timer_cfg[timer_id].state != TIMER_STATE_CLOSED)
    {
        return STATUS_INVALID_REQUEST;    /*device already opened*/
    }

    timer = base[timer_id];

    timer->CONTROL.reg = 0;
    while ( Get_Timer_Enable_Status(timer_id) );
    timer->CLEAR = 1;

    if (cfg.UserPrescale)
    {
        timer->PRESCALE = cfg.UserPrescale;
    }
    else
    {
        timer->PRESCALE = 0;
        timer->CONTROL.bit.PRESCALE = cfg.Prescale;
    }

    timer->CONTROL.bit.UP_CONUNT = cfg.CountingMode;
    timer->CONTROL.bit.ONE_SHOT_EN = cfg.OneShotMode;
    timer->CONTROL.bit.MODE = cfg.Mode;
    timer->CONTROL.bit.INT_ENABLE = cfg.IntEnable;

    //select PMU_CLK
    if (timer_id == 0)
    {
        SYSCTRL->SYS_CLK_CTRL1.bit.TIMER0_CLK_SEL = TIMER_CLOCK_SOURCEC_PMU;
    }
    else if (timer_id == 1)
    {
        SYSCTRL->SYS_CLK_CTRL1.bit.TIMER1_CLK_SEL = TIMER_CLOCK_SOURCEC_PMU;
    }
    else if (timer_id == 2)
    {
        SYSCTRL->SYS_CLK_CTRL1.bit.TIMER2_CLK_SEL = TIMER_CLOCK_SOURCEC_PMU;
    }


    if (cfg.Pwm0Enable)
    {
        SYSCTRL->SOC_PWM_SEL.bit.PWM0_SRC_SEL = timer_id + 1;
    }
    if (cfg.Pwm1Enable)
    {
        SYSCTRL->SOC_PWM_SEL.bit.PWM1_SRC_SEL = timer_id + 1;
    }
    if (cfg.Pwm2Enable)
    {
        SYSCTRL->SOC_PWM_SEL.bit.PWM2_SRC_SEL = timer_id + 1;
    }
    if (cfg.Pwm3Enable)
    {
        SYSCTRL->SOC_PWM_SEL.bit.PWM3_SRC_SEL = timer_id + 1;
    }
    if (cfg.Pwm4Enable)
    {
        SYSCTRL->SOC_PWM_SEL.bit.PWM4_SRC_SEL = timer_id + 1;
    }

    timer_cfg[timer_id].state = TIMER_STATE_OPEN;

    return STATUS_SUCCESS;
}

uint32_t Timer_Pwm_Start(uint32_t timer_id, uint32_t timeload_ticks, uint32_t timeout_ticks, uint32_t threshold, bool phase)
{
    TIMERN_T *timer;
    TIMERN_T *base[MAX_NUMBER_OF_TIMER] = {TIMER0, TIMER1, TIMER2};

    if (timer_id > MAX_NUMBER_OF_TIMER)
    {
        return STATUS_INVALID_PARAM;
    }

    if (timer_cfg[timer_id].state != TIMER_STATE_OPEN)
    {
        return STATUS_NO_INIT;    /*DEVIC SHOULD BE OPEN FIRST.*/
    }

    timer = base[timer_id];

    Timer_Load_Ex(timer_id, timeload_ticks, timeout_ticks);
    timer->CAP_EN.bit.TIMER_PWM_EN = 1;
    timer->CONTROL.bit.EN = 1;
    timer->THD = threshold;
    timer->PHA.bit.PHA = phase;

    return STATUS_SUCCESS;
}

uint32_t Timer_Pwm_Stop(uint32_t timer_id)
{
    TIMERN_T *timer;
    TIMERN_T *base[MAX_NUMBER_OF_TIMER] = {TIMER0, TIMER1, TIMER2};

    if (timer_id > MAX_NUMBER_OF_TIMER)
    {
        return STATUS_INVALID_PARAM;
    }

    if (timer_cfg[timer_id].state != TIMER_STATE_OPEN)
    {
        return STATUS_NO_INIT;    /*DEVIC SHOULD BE OPEN FIRST.*/
    }

    timer = base[timer_id];

    timer->CAP_EN.bit.TIMER_PWM_EN = 0;
    timer->CONTROL.bit.EN = 0;

    return STATUS_SUCCESS;
}

void Timer_32k_Int_Callback_Register(uint32_t timer_id, timer_proc_cb timer_callback)
{
    timer_32k_cfg[timer_id].timer_callback = timer_callback;
    return;
}

uint32_t Get_Timer32k_Enable_Status(uint32_t timer_id)
{
    TIMER32KN_T *timer;
    TIMER32KN_T *base[MAX_NUMBER_OF_TIMER_32K] = {TIMER32K0, TIMER32K1};

    timer = base[timer_id];

    return (timer->CONTROL.bit.TIMER_ENABLE_STATUS);
}

uint32_t Timer_32k_Open(uint32_t timer_id,
                        timer_32k_config_mode_t cfg,
                        timer_proc_cb timer_callback)
{
    TIMER32KN_T *timer;
    TIMER32KN_T *base[MAX_NUMBER_OF_TIMER_32K] = {TIMER32K0, TIMER32K1};

    if (timer_id > MAX_NUMBER_OF_TIMER_32K)
    {
        return STATUS_INVALID_PARAM;
    }

    if (timer_32k_cfg[timer_id].state != TIMER_STATE_CLOSED)
    {
        return STATUS_INVALID_REQUEST;    /*device already opened*/
    }

    timer = base[timer_id];

    timer->CONTROL.reg = 0;
    while ( Get_Timer32k_Enable_Status(timer_id) );
    timer->CLEAR = 1;

    if (cfg.UserPrescale)
    {
        timer->PRESCALE = cfg.UserPrescale;
    }
    else
    {
        timer->PRESCALE = 0;
        timer->CONTROL.bit.PRESCALE = cfg.Prescale;
    }

    timer->CONTROL.bit.UP_CONUNT = cfg.CountingMode;
    timer->CONTROL.bit.ONE_SHOT_EN = cfg.OneShotMode;
    timer->CONTROL.bit.MODE = cfg.Mode;
    timer->CONTROL.bit.INT_ENABLE = cfg.IntEnable;
    if (cfg.RepeatDelay)
    {
        timer->REPEAT_DELAY.bit.INT_REPEAT_DELAY_DISABLE = 0;
        timer->REPEAT_DELAY.bit.INT_REPEAT_DELAY = cfg.RepeatDelay;
    }
    else
    {
        timer->REPEAT_DELAY.bit.INT_REPEAT_DELAY_DISABLE = 1;
    }

    timer_32k_cfg[timer_id].state = TIMER_STATE_OPEN;

    /*
     * Because of watchdog reset, it is possible that "last time" timer interrupt pending
     * in system, so we should set callback before enable interrupt, otherwise when
     * we call NVIC_EnableIRQ(...), it will trigger interrupt at the same time and jump
     * an invalid address such that hardware fault generated.
     */
    Timer_32k_Int_Callback_Register(timer_id, timer_callback);
    NVIC_EnableIRQ((IRQn_Type)(Timer32K0_IRQn + timer_id));

    return STATUS_SUCCESS;
}

uint32_t Timer_32k_Load(uint32_t timer_id, uint32_t timeload_ticks, uint32_t timeout_ticks)
{
    TIMER32KN_T *timer;
    TIMER32KN_T *base[MAX_NUMBER_OF_TIMER_32K] = {TIMER32K0, TIMER32K1};

    if (timer_id > MAX_NUMBER_OF_TIMER_32K)
    {
        return STATUS_INVALID_PARAM;
    }

    timer = base[timer_id];
    timer->LOAD = (timeload_ticks - 1);
    timer->EXPRIED_VALUE = timeout_ticks;
    while ( !((timer->LOAD == timer->VALUE) && (timer->LOAD == (timeload_ticks - 1))) );

    return STATUS_SUCCESS;
}

uint32_t Timer_32k_Start(uint32_t timer_id, uint32_t timeload_ticks, uint32_t timeout_ticks)
{
    TIMER32KN_T *timer;
    TIMER32KN_T *base[MAX_NUMBER_OF_TIMER_32K] = {TIMER32K0, TIMER32K1};

    if (timer_id > MAX_NUMBER_OF_TIMER_32K)
    {
        return STATUS_INVALID_PARAM;
    }

    if (timer_32k_cfg[timer_id].state != TIMER_STATE_OPEN)
    {
        return STATUS_NO_INIT;    /*DEVIC SHOULD BE OPEN FIRST.*/
    }

    timer = base[timer_id];

    Timer_32k_Load(timer_id, timeload_ticks, timeout_ticks);
    timer->CONTROL.bit.EN = 1;

    return STATUS_SUCCESS;
}

uint32_t Timer_32k_Stop(uint32_t timer_id)
{
    TIMER32KN_T *timer;
    TIMER32KN_T *base[MAX_NUMBER_OF_TIMER_32K] = {TIMER32K0, TIMER32K1};

    if (timer_id > MAX_NUMBER_OF_TIMER_32K)
    {
        return STATUS_INVALID_PARAM;
    }

    if (timer_32k_cfg[timer_id].state != TIMER_STATE_OPEN)
    {
        return STATUS_NO_INIT;    /*DEVIC SHOULD BE OPEN FIRST.*/
    }

    timer = base[timer_id];
    timer->CONTROL.bit.EN = 0;         /*Disable timer*/

    return STATUS_SUCCESS;
}

uint32_t Timer_32k_Close(uint32_t timer_id)
{
    TIMER32KN_T *timer;
    TIMER32KN_T *base[MAX_NUMBER_OF_TIMER_32K] = {TIMER32K0, TIMER32K1};

    if (timer_id > MAX_NUMBER_OF_TIMER_32K)
    {
        return STATUS_INVALID_PARAM;
    }

    timer = base[timer_id];
    timer->CONTROL.reg = 0;         /*Disable timer*/

    /*disable interrupt.*/
    NVIC_DisableIRQ((IRQn_Type)(Timer32K0_IRQn + timer_id));

    timer_32k_cfg[timer_id].timer_callback = NULL;
    timer_32k_cfg[timer_id].state = TIMER_STATE_CLOSED;

    return STATUS_SUCCESS;
}

uint32_t Timer_32k_IntStatus_Get(uint32_t timer_id)
{
    TIMER32KN_T *timer;
    TIMER32KN_T *base[MAX_NUMBER_OF_TIMER_32K] = {TIMER32K0, TIMER32K1};

    timer = base[timer_id];
    return (timer->CONTROL.bit.INT_STATUS);
}

uint32_t Timer_32k_Current_Get(uint32_t timer_id)
{
    TIMER32KN_T *timer;
    TIMER32KN_T *base[MAX_NUMBER_OF_TIMER_32K] = {TIMER32K0, TIMER32K1};

    timer = base[timer_id];

    return (timer->VALUE);
}


/**
 * @ingroup Timer_Driver
 * @brief Timer0 interrupt
 * @details
 * @return
 */
void Timer0_Handler(void)
{
    TIMER0->CLEAR = 1;       /*clear interrupt*/

    if (timer_cfg[0].timer_callback != NULL)
    {
        timer_cfg[0].timer_callback(0);
    }
    return;
}

/**
 * @ingroup Timer_Driver
 * @brief Timer1 interrupt
 * @details
 * @return
 */
void Timer1_Handler(void)
{
    TIMER1->CLEAR = 1;       /*clear interrupt*/

    if (timer_cfg[1].timer_callback != NULL)
    {
        timer_cfg[1].timer_callback(1);
    }
    return;
}

/**
 * @ingroup Timer_Driver
 * @brief Timer2 interrupt
 * @details
 * @return
 */
void Timer2_Handler(void)
{
    TIMER2->CLEAR = 1;       /*clear interrupt*/

    if (timer_cfg[2].timer_callback != NULL)
    {
        timer_cfg[2].timer_callback(2);
    }
    return;
}

/**
 * @ingroup Timer_Driver
 * @brief 32k Timer0 interrupt
 * @details
 * @return
 */
void Timer32K0_Handler(void)
{
    if (TIMER32K0->CONTROL.bit.ONE_SHOT_EN)
    {
        //TIMER32K0->CONTROL.bit.EN = 0;
    }

    TIMER32K0->CLEAR = 1;       /*clear interrupt*/

    if (timer_32k_cfg[0].timer_callback != NULL)
    {
        timer_32k_cfg[0].timer_callback(0);
    }
    return;
}


/**
 * @ingroup Timer_Driver
 * @brief 32k Timer1 interrupt
 * @details
 * @return
 */
void Timer32K1_Handler(void)
{
    if (TIMER32K1->CONTROL.bit.ONE_SHOT_EN)
    {
        //TIMER32K1->CONTROL.bit.EN = 0;
    }

    TIMER32K1->CLEAR = 1;       /*clear interrupt*/

    if (timer_32k_cfg[1].timer_callback != NULL)
    {
        timer_32k_cfg[1].timer_callback(1);
    }
    return;
}


