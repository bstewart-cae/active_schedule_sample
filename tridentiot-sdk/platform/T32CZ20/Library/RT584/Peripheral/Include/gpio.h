/**
 ******************************************************************************
 * @file    gpio.h
 * @author
 * @brief   gpio driver header file
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


#ifndef __RT584_GPIO_H__
#define __RT584_GPIO_H__

#ifdef __cplusplus
extern "C"
{
#endif



/**************************************************************************************************
 *    INCLUDES
 *************************************************************************************************/
#include <stdio.h>
#include <string.h>
#include "cm33.h"


/**************************************************************************************************
 *    CONSTANTS AND DEFINES
 *************************************************************************************************/

/**
 *  @Brief GPIO Max Pin Definition
 */
#define GPIO0        0
#define GPIO1        1
#define GPIO2        2
#define GPIO3        3
#define GPIO4        4
#define GPIO5        5
#define GPIO6        6
#define GPIO7        7
#define GPIO8        8
#define GPIO9        9
#define GPIO10       10
#define GPIO11       11
#define GPIO12       12
#define GPIO13       13
#define GPIO14       14
#define GPIO15       15
#define GPIO16       16
#define GPIO17       17
#define GPIO18       18
#define GPIO19       19
#define GPIO20       20
#define GPIO21       21
#define GPIO22       22
#define GPIO23       23
#define GPIO24       24
#define GPIO25       25
#define GPIO26       26
#define GPIO27       27
#define GPIO28       28
#define GPIO29       29
#define GPIO30       30
#define GPIO31       31
#define MAX_NUMBER_OF_PINS    (32)           /*!< Specify Maximum Pins of GPIO */


/**
 *  @Brief GPIO_DEBOUNCE CLOCK Constant Definitions
 */
#define  DEBOUNCE_SLOWCLOCKS_32     (0)       /*!< setting for sampling cycle = 32 clocks */
#define  DEBOUNCE_SLOWCLOCKS_64     (1)       /*!< setting for sampling cycle = 64 clocks */
#define  DEBOUNCE_SLOWCLOCKS_128    (2)       /*!< setting for sampling cycle = 128 clocks */
#define  DEBOUNCE_SLOWCLOCKS_256    (3)       /*!< setting for sampling cycle = 256 clocks */
#define  DEBOUNCE_SLOWCLOCKS_512    (4)       /*!< setting for sampling cycle = 512 clocks */
#define  DEBOUNCE_SLOWCLOCKS_1024   (5)       /*!< setting for sampling cycle = 1024 clocks */
#define  DEBOUNCE_SLOWCLOCKS_2048   (6)       /*!< setting for sampling cycle = 2048 clocks */
#define  DEBOUNCE_SLOWCLOCKS_4096   (7)       /*!< setting for sampling cycle = 4096 clocks */


/**
 *  @Brief Let rt584 can use rt58x api
 */
#define gpio_cfg(pin_number,dir,int_mode)                           Gpio_Cfg(pin_number,dir,int_mode)
#define gpio_register_isr(pin,app_gpio_callback,param)              Gpio_Register_Callback(pin,app_gpio_callback,param)
#define gpio_cfg_output(pin_number)                                 Gpio_Cfg_Output(pin_number)
#define gpio_cfg_input(pin_number,int_mode)                         Gpio_Cfg_Input(pin_number,int_mode)
#define gpio_pin_set(pin_number)                                    Gpio_Pin_Set(pin_number)
#define gpio_pin_clear(pin_number)                                  Gpio_Pin_Clear(pin_number)
#define gpio_pin_write(pin_number,value)                            Gpio_Pin_Write(pin_number,value)
#define gpio_pin_toggle(pin_number)                                 Gpio_Pin_Toggle(pin_number)
#define gpio_pin_get(pin_number)                                    Gpio_Pin_Get(pin_number)
#define gpio_int_enable(pin)                                        Gpio_Int_Enable(pin)
#define gpio_int_disable(pin)                                       Gpio_Int_Disable(pin)
#define gpio_debounce_enable(pin)                                   Gpio_Debounce_Enable(pin)
#define gpio_debounce_disable(pin)                                  Gpio_Debounce_Disable(pin)
#define gpio_set_debounce_time(mode)                                Gpio_Set_Debounce_Time(mode)

/**************************************************************************************************
 *    TYPEDEFS
 *************************************************************************************************/
/**
 * @brief Pin direction definitions.
 */
typedef enum
{
    GPIO_PIN_DIR_INPUT,           /*!< GPIO Input Mode */
    GPIO_PIN_DIR_OUTPUT,          /*!< GPIO Output Mode */
    GPIO_PIN_DIR_INVALID
} gpio_pin_dir_t;

/**
 * @brief Selecting the pin to sense high or low level, edge for pin input.
 */
typedef enum
{
    GPIO_PIN_NOINT,              /*!< GPIO Interrupt mode disable */
    GPIO_PIN_INT_LEVEL_LOW,      /*!< GPIO Interrupt enable for Level-Low */
    GPIO_PIN_INT_LEVEL_HIGH,     /*!< GPIO Interrupt enable for Level-High */
    GPIO_PIN_INT_EDGE_RISING,    /*!< GPIO Interrupt enable for Rising Edge */
    GPIO_PIN_INT_EDGE_FALLING,   /*!< GPIO Interrupt enable for Falling Edge */
    GPIO_PIN_INT_BOTH_EDGE,      /*!< GPIO Interrupt enable for both Rising and Falling Edge */
} gpio_pin_int_mode_t;

/**
 * @brief Selecting the pin to wake up high or low level.
 */
typedef enum
{
    GPIO_LEVEL_LOW,      /*!< GPIO Level-Low wake up */
    GPIO_LEVEL_HIGH,     /*!< GPIO Level-High wake up */
} gpio_pin_wake_t;

/**
 * @brief GPIO interrupt service routine callback for user application.
 *
 * @param[in]   pin        Interrupt pin number.
 * @param[in]   isr_param  isr_param passed to user interrupt handler.
 *                         The parameter is set by user called in function gpio_register_isr
 *                          "*param". This register could be NULL, if user does NOT require this context
 *                         parameter.
 *
 *
 * @details    This callback function is still running in interrupt mode, so this function
 *              should be as short as possible. It can NOT call any block function in this
 *              callback service routine.
 *
 */
typedef void (* gpio_proc_cb)(uint32_t pin, void *isr_param);


/** @addtogroup Peripheral_Driver RT584Z Periperhal Driver Function
  @{
*/



/** @addtogroup GPIO_DRIVER GPIO Driver Functions
  @{
*/



/**************************************************************************************************
 *    GLOBAL PROTOTYPES
 *************************************************************************************************/
/**
 * @brief Pin configuration function.
 *
 * @param[in]   pin_number    Specifies the pin number.
 * @param[in]   dir           Pin direction.
 * @param[in]   int_mode      Pin interrupt mode. Only available when dir is GPIO_PIN_DIR_INPUT
 *
 * @retval     none
 */
void Gpio_Cfg(uint32_t pin_number, gpio_pin_dir_t dir, gpio_pin_int_mode_t int_mode);

/**
 * @brief Register user interrupt ISR callback function.
 *
 * @param[in]    pin                    Specifies the pin number.
 * @param[in]    app_gpio_callback      Specifies user callback function when the pin interrupt generated.
 * @param[in]    param                  Paramter passed to user interrupt handler "app_gpio_callback"
 *
 * @retval      none
 */
void Gpio_Register_Callback(uint32_t pin, gpio_proc_cb app_gpio_callback, void *param);

/**
 * @brief Unregister user interrupt ISR callback function.
 *
 * @param[in]    pin                    Specifies the pin number.
 *
 * @retval      none
 */
void Gpio_Cancell_Callback(uint32_t pin);

/**
 * @brief setup gpio wakeup from deep sleep with level high or low.
 *
 * @param[in]    pin    Specifies the pin number.
 * @param[in]    level  Set wakeup polarity low or high in deepsleep.
 *
 * @retval      none
 */
void Gpio_Setup_Deep_Sleep_Io(uint8_t num, gpio_pin_wake_t level);

/**
 * @brief disable gpio wakeup from deep sleep.
 *
 * @param[in]    pin    Specifies the pin number.
 *
 * @retval      none
 */
void Gpio_Disable_Deep_Sleep_Io(uint8_t num);

/**
 * @brief setup gpio wakeup from deep power down with level high or low.
 *
 * @param[in]    pin    Specifies the pin number.
 * @param[in]    level  Set wakeup polarity low or high in deep power down.
 *
 * @retval      none
 */
void Gpio_Setup_Deep_Powerdown_Io(uint8_t num, gpio_pin_wake_t level);

/**
 * @brief disable gpio wakeup from deep power down.
 *
 * @param[in]    pin    Specifies the pin number.
 *
 * @retval      none
 */
void Gpio_Disable_Deep_Powerdown_Io(uint8_t num);

/**
 * @brief setup gpio Schmitt.
 *
 * @param[in]    pin    Specifies the pin number.
 *
 * @retval      none
 */
void Gpio_Setup_Io_Schmitt(uint8_t num);

/**
 * @brief disable gpio Schmitt.
 *
 * @param[in]    pin    Specifies the pin number.
 *
 * @retval      none
 */
void Gpio_Disable_Io_Schmitt(uint8_t num);

/**
 * @brief setup gpio filter.
 *
 * @param[in]    pin    Specifies the pin number.
 *
 * @retval      none
 */
void Gpio_Setup_Io_Filter(uint8_t num);

/**
 * @brief disable gpio filter.
 *
 * @param[in]    pin    Specifies the pin number.
 *
 * @retval      none
 */
void Gpio_Disable_Io_Filter(uint8_t num);

/**
 * @brief  Set gpio pin to input mode and related parameter.
 *         It is recommended to use this API to complete the gpio input configuration
 *
 * @param[in]  pin_number: Gpio pin number.
 * @param[in]  int_mode: Gpio input interrupt mode.
 * @param[in]  app_gpio_callback: Gpio callback function.
 * @param[in]  param: isr_param passed to user interrupt handler.
 * @param[in]  debounce_en: Enable debounce
 *
 * @retval    none
 */

void Gpio_Cfg_Input_Parameters(uint32_t pin_number, gpio_pin_int_mode_t int_mode,
                               gpio_proc_cb app_gpio_callback, void *param, bool debounce_en);


/**
 * @brief  Set gpio pin to output mode.
 *
 * @param[in]    pin   Specifies the pin number to output mode.
 *
 * @retval      none
 */

__STATIC_INLINE void Gpio_Cfg_Output(uint32_t pin_number)
{
    Gpio_Cfg(pin_number, GPIO_PIN_DIR_OUTPUT, GPIO_PIN_NOINT);
}

/**
 * @brief  Set gpio pin to input mode.
 *
 * @param[in]  pin          Specifies the pin number to input mode.
 * @param[in]  int_mode     Specifies the pin number interrupt if this pin need to be gpio interrupt source.
 *
 * @retval    none
 */

__STATIC_INLINE void Gpio_Cfg_Input(uint32_t pin_number, gpio_pin_int_mode_t int_mode)
{
    Gpio_Cfg(pin_number, GPIO_PIN_DIR_INPUT, int_mode);
}

/**
 * @brief  Set gpio pin output high.
 *
 * @param[in]  pin      Specifies the pin number to output high.
 *
 * @retval    none
 */

__STATIC_INLINE void Gpio_Pin_Set(uint32_t pin_number)
{
    assert_param(pin_number < MAX_NUMBER_OF_PINS);

    GPIO->OUTPUT_HIGH = (1 << pin_number);
}

/**
 * @brief  Set gpio pin output low.
 *
 * @param[in]  pin      Specifies the pin number to output low.
 *
 * @retval    none
 */

__STATIC_INLINE void Gpio_Pin_Clear(uint32_t pin_number)
{
    assert_param(pin_number < MAX_NUMBER_OF_PINS);

    GPIO->OUTPUT_LOW = (1 << pin_number);
}


/**
 * @brief  Set gpio pin output value.
 *
 * @param[in]  pin      Specifies the pin number.
 * @param[in]  value    value 0 for output low, value 1 for output high.
 *
 * @retval    none
 */

__STATIC_INLINE void Gpio_Pin_Write(uint32_t pin_number, uint32_t value)
{
    if (value == 0)
    {
        Gpio_Pin_Clear(pin_number);
    }
    else
    {
        Gpio_Pin_Set(pin_number);
    }
}


/**
 * @brief  Toggle gpio pin output value.
 *
 * @param[in]  pin      Specifies the pin number.
 *
 * @retval    none
 */

__STATIC_INLINE void Gpio_Pin_Toggle(uint32_t pin_number)
{
    uint32_t state, MASK;

    assert_param(pin_number < MAX_NUMBER_OF_PINS);

    MASK = (1 << pin_number);
    state = GPIO->OUTPUT_STATE & MASK;

    if (state)
    {
        GPIO->OUTPUT_LOW = MASK;
    }
    else
    {
        GPIO->OUTPUT_HIGH = MASK;
    }

}

/**
 * @brief  Get gpio pin input value.
 *
 * @param[in]  pin      Specifies the pin number.
 *
 * @retval    1 for input pin is high, 0 for input is low.
 */

__STATIC_INLINE uint32_t Gpio_Pin_Get(uint32_t pin_number)
{
    assert_param(pin_number < MAX_NUMBER_OF_PINS);

    return ((GPIO->STATE & (1 << pin_number)) ? 1 : 0);
}


/**
 * @brief  Enable gpio pin interrupt
 *
 * @param[in]  pin      Specifies the pin number that enable interrupt.
 *
 * @retval    None
 */

__STATIC_INLINE void Gpio_Int_Enable(uint32_t pin)
{
    assert_param(pin < MAX_NUMBER_OF_PINS);
    GPIO->ENABLE_INT = (1 << pin);
}

/**
 * @brief  Disable gpio pin interrupt
 *
 * @param[in]  pin      Specifies the pin number that disable interrupt.
 *
 * @retval    None
 */

__STATIC_INLINE void Gpio_Int_Disable(uint32_t pin)
{
    assert_param(pin < MAX_NUMBER_OF_PINS);
    GPIO->DISABLE_INT = (1 << pin);
}

/**
 * @brief  Enable gpio pin debounce function.
 *         The debounce enable Api needs to be done after setting Gpio_Cfg_Input
 *
 * @param[in]  pin      Specifies the pin number that enable gpio debounce when interrupt happened.
 *
 * @retval    None
 */

__STATIC_INLINE void Gpio_Debounce_Enable(uint32_t pin)
{
    assert_param(pin < MAX_NUMBER_OF_PINS);
    GPIO->DEBOUCE_EN = (1 << pin);
}

/**
 * @brief  Disable gpio pin debounce function.
 *
 * @param[in]  pin      Specifies the pin number that disable gpio debounce when interrupt happened.
 *
 * @retval    None
 */

__STATIC_INLINE void Gpio_Debounce_Disable(uint32_t pin)
{
    assert_param(pin < MAX_NUMBER_OF_PINS);
    GPIO->DEBOUCE_DIS = (1 << pin);
}

/**
 * @brief  Set debounce clock for all GPIO.
 *
 * @param[in]  mode   Specifies the sampling clock of debounce function.
 *
 * @retval    None
 */

__STATIC_INLINE void Gpio_Set_Debounce_Time(uint32_t mode)
{
    assert_param(mode < DEBOUNCE_SLOWCLOCKS_4096);

    if (mode > DEBOUNCE_SLOWCLOCKS_4096)
    {
        mode = DEBOUNCE_SLOWCLOCKS_4096;
    }

    GPIO->DEBOUNCE_TIME = mode;
}

/**@}*/ /* end of GPIO_DRIVER group */

/**@}*/ /* end of PERIPHERAL_DRIVER group */


#ifdef __cplusplus
}
#endif

#endif      /* end of __RT584_GPIO_H__ */


