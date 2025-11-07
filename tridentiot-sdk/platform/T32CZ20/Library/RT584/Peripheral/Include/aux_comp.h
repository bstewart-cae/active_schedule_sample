/**
 ******************************************************************************
 * @file    aux_comp.c
 * @author
 * @brief   aux comparator driver header file
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

#ifndef __RT584_AUX_COMP_H__
#define __RT584_AUX_COMP_H__

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

/**************************************************************************************************
 *    TYPEDEFS
 *************************************************************************************************/
/**
 * @brief Aux comparator interrupt service routine callback for user application.
 *
 * @param[in]   status  status passed to user aux routine for the reason
 *
 *
 * @details    This callback function is still running in interrupt mode, so this function
 *              should be as short as possible. It can NOT call any block function in this
 *              callback service routine.
 *
 */
typedef void (* aux_comp_proc_cb)(uint32_t status);

/**
 * @brief Debounce time definitions.
 */
typedef enum
{
    AUX_SLOW_CLOKC_2,          /*!< 2 slow clock debounce time */
    AUX_SLOW_CLOKC_4,          /*!< 4 slow clock debounce time */
    AUX_SLOW_CLOKC_8           /*!< 8 slow clock debounce time */
} aux_comp_debounce_time_t;

/**
 * @brief Counter mode edge definitions.
 */
typedef enum
{
    AUX_RISING_EDGE,          /*!< counter trigger when rising edge */
    AUX_FALLING_EDGE,         /*!< counter trigger when falling edge */
    AUX_BOTH_EDGE             /*!< counter trigger when both edge */
} aux_comp_counter_mode_edge_t;

/**
 * @brief Counter mode edge definitions.
 */
typedef enum
{
    AUX_LOW_LEVEL,          /*!< aux comparator without clock to wakeup at low level when deepsleep */
    AUX_HIGH_LEVEL          /*!< aux comparator without clock to wakeup at high level when deepsleep */
} aux_comp_wakeup_deepslepp_level_t;


/**
 * @brief aux comparator config structure.
 */
typedef struct
{
    uint8_t debounce_en             : 1;    /*!< set debounce enable */
    uint8_t debounce_sel            : 2;    /*!< debounce time select */
    uint8_t counter_mode_en         : 1;    /*!< set counter mode enable */
    uint8_t counter_mode_edge       : 2;    /*!< set the trigger polarity of counter mode */
    uint8_t counter_mode_int_en     : 1;    /*!< counter mode interrupt enable */
    uint8_t rising_edge_int_en      : 1;    /*!< set rising edge interrupt enable */
    uint8_t falling_edge_int_en     : 1;    /*!< set falling edge interrupt enable */
    uint16_t counter_mode_threshold;        /*!< set the trigger threshold of the counter mode. When COUNTER_CNT > counter_mode_threshold , Interrupt will be triggered. */
} aux_comp_config_t;


/** @addtogroup Peripheral_Driver RT584Z Driver Function
  @{
*/

/** @addtogroup AUX_DRIVER AUX Driver Function
  @{
*/

/**************************************************************************************************
 *    GLOBAL PROTOTYPES
 *************************************************************************************************/
/**
 * @brief Register user interrupt ISR callback function.
 *
 * @param[in]    aux_comp_callback  Specifies user callback function when the aux comparator interrupt generated.
 *
 * @retval     none
 */
void Aux_Comp_Register_Callback(aux_comp_proc_cb aux_comp_callback);

/**
 * @brief     Init Aux comparator Aanlog setting.
 *
 * @retval     none
 */
void Aux_Comp_Ana_Init(void);

/**
 * @brief Setting Aux comparator configuration.
 *
 * @param[in]    aux_cfg            Aux comparator configuration.
 * @param[in]    aux_comp_callback  Specifies user callback function when the aux comparator interrupt generated.
 *
 * @retval     none
 */
void Aux_Comp_Open(aux_comp_config_t aux_cfg, aux_comp_proc_cb aux_comp_callback);

/**
 * @brief     Enable Aux comparator .
 *
 * @retval     none
 */
void Aux_Comp_Normal_Start(void);

/**
 * @brief     Disable Aux comparator .
 *
 * @retval     none
 */
void Aux_Comp_Normal_Stop(void);

/**
 * @brief    Enable Aux comparator in sleep mode.
 *
 * @retval     none
 */
void Aux_Comp_Sleep_Start(void);

/**
 * @brief    Disable Aux comparator in sleep mode.
 *
 * @retval     none
 */
void Aux_Comp_Sleep_Stop(void);

/**
 * @brief    Enable Aux comparator in deep sleep mode.
 *
 * @retval     none
 */
void Aux_Comp_Deep_Sleep_Start(void);

/**
 * @brief    Disable Aux comparator in deep sleep mode.
 *
 * @retval     none
 */
void Aux_Comp_Deep_Sleep_Stop(void);

/**
 * @brief    Enable clock source when Aux comparator in deep sleep mode.
 *
 * @retval     none
 */
void Aux_Comp_Setup_Deep_Sleep_Enable_Clock(void);

/**
 * @brief    Disable clock source when Aux comparator in deep sleep mode.
 *
 * @param[in]    wakeup_level   Set wakeup polarity low or high in deepsleep without clock source.
 *
 * @retval     none
 */
void Aux_Comp_Setup_Deep_Sleep_Disable_Clock(uint8_t wakeup_level);

/**
 * @brief    Get Aux comparator count.
 *
 * @retval     Counter count.
 */
uint32_t Get_Aux_Comp_Counter_Count(void);

/**
 * @brief    Clear Aux comparator count.
 *
 * @retval     none
 */
void Clear_Aux_Comp_Counter_Count(void);


/**@}*/ /* end of AUX_DRIVER group */

/**@}*/ /* end of PERIPHERAL_DRIVER group */


#ifdef __cplusplus
}
#endif

#endif      /* end of __RT584_AUX_COMP_H__ */


