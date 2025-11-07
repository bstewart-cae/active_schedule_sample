/**
  ******************************************************************************
 * @file     gpio_reg.h
  * @author
  * @brief   general purpose input/output register definition header file
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
  *
  */


#ifndef __RT584_GPIO_REG_H__
#define __RT584_GPIO_REG_H__

#if defined ( __CC_ARM   )
#pragma anon_unions
#endif

/** @addtogroup REGISTER RT584Z Peripheral Control Register

  @{

*/

/**
    @addtogroup GPIO General Purpose Input/Output Controller(GPIO)
    Memory Mapped Structure for General Purpose Input/Output Controller
  @{
*/
typedef struct
{

    union                           /*0x00*/
    {
        __IM uint32_t STATE;
        __OM uint32_t OUTPUT_HIGH;
    };

    union                           /*0x04*/
    {
        __IM uint32_t INT_STATUS;
        __OM uint32_t OUTPUT_LOW;
    };

    __IOM uint32_t OUTPUT_EN;       /*0x08, read for output state*/

    union                           /*0x0C*/
    {
        __IM uint32_t OUTPUT_STATE;
        __OM uint32_t INPUT_EN;
    };

    union                           /*0x10*/
    {
        __IO  uint32_t INT_ENABLE_STATUS;
        __OM  uint32_t ENABLE_INT;
    };

    __OM  uint32_t DISABLE_INT;     /*0x14*/
    __IOM uint32_t EDGE_TRIG;       /*0x18, read for the state of edge trigger enable*/
    __IOM uint32_t LEVEL_TRIG;      /*0x1C, read for the state of level enable*/
    __IOM uint32_t POSTITIVE;       /*0x20*/
    __IOM uint32_t NEGATIVE;        /*0x24*/
    __IOM uint32_t BOTH_EDGE_EN;    /*0x28*/

    __OM  uint32_t BOTH_EDGE_CLR;   /*0x2C*/
    __OM  uint32_t EDGE_INT_CLR;    /*0x30*/
    __IOM uint32_t LOOPBACK;        /*0x34 Enable Loopback*/
    __IOM uint32_t ENABLE_INPUT;    /*0x38 Enable register(0x00) to read input value */
    __OM  uint32_t DISABLE_INPUT;   /*0x3C Disable register(0x00) to read input value */

    __IOM uint32_t DEBOUCE_EN;      /*0x40 Enable Debounce*/
    __OM  uint32_t DEBOUCE_DIS;     /*0x44 Disable Debounce*/
    __IOM uint32_t DEBOUNCE_TIME;   /*0x48 De-Bounce time selection*/

    __IOM uint32_t Reserve1;        /*0x4C */

    __IOM uint32_t SET_DS_EN;       /*0x50*/
    __OM  uint32_t DIS_DS_EN;       /*0x54*/
    __IOM uint32_t SET_DS_INV;      /*0x58, write 1 to set corresponding GPIO wakeup polarity of high in DeepSleep*/
    __OM  uint32_t DIS_DS_INV;      /*0x5C, write 1 to set corresponding GPIO wakeup polarity of low in DeepSleep*/

} GPIO_T;

/**@}*/ /* end of PERIPHERAL_BASE group */

/**@}*/ /* end of PERIPHERAL_BASE group */

#if defined ( __CC_ARM   )
#pragma no_anon_unions
#endif

#endif      /* end of __RT584_GPIO_REG_H__ */
