/**
 ******************************************************************************
 * @file    swi_reg.h
 * @author
 * @brief   software interrupt register header file
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

#ifndef _RT584_SWI_REG_H__
#define _RT584_SWI_REG_H__

#if defined ( __CC_ARM   )
#pragma anon_unions
#endif

/** @addtogroup REGISTER RT584Z Peripheral Control Register

  @{

*/

/**
    @addtogroup SWI Software Interrupt Controller(SWI)
    Memory Mapped Structure for Software Interrupt Controller
  @{
*/
typedef struct
{
    __OM  uint32_t  ENABLE_IRQ;     /*offset:0x00*/
    __OM  uint32_t  CLEAR_IRQ;      /*offset:0x04*/
    __IM  uint32_t  IRQ_STATE;      /*offset:0x08*/
    __IO  uint32_t  DATA;           /*offset:0x0C*/
} SWI_T;

#define ENABLE_SOFT_IRQ            (1<<0)
#define CLEAR_SOFT_IRQ             (1<<0)


/**@}*/ /* end of SWI Controller group */

/**@}*/ /* end of REGISTER group */


#if defined ( __CC_ARM   )
#pragma no_anon_unions
#endif

#endif /* end of _RT584_SWI_REG_H */

