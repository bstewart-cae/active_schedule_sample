/**
 ******************************************************************************
 * @file    xdma_reg.h
 * @author
 * @brief   simple data memory access module control register header file
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


#ifndef __XDMA_REG_H__
#define __XDMA_REG_H__

#if defined (__CC_ARM)
#pragma anon_unions
#endif

/** @addtogroup REGISTER RT584Z Peripheral Control Register

  @{

*/

/**
    @addtogroup xDMA Data Memory Access Module Controller(xDMA)
    Memory Mapped Structure for Data Memory Access Module Controller
  @{
*/
typedef struct
{
    __IO uint32_t XDMA_CTL0;        /*offset:0x00*/
    __O  uint32_t XDMA_CTL1;        /*offset:0x04*/

} XDMA_T;




/***********************************************************************************************************************
 *    CONSTANTS AND DEFINES
 **********************************************************************************************************************/


#define XDMA_ENABLE            (1UL<<0)
#define XDMA_RESET             (1UL<<0)

/**@}*/ /* end of xDMA register group */

/**@}*/ /* end of REGISTER group */


#if defined ( __CC_ARM   )
#pragma no_anon_unions
#endif

#endif /* end of _RT584_XDMA_REG_H */


