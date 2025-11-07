/**
 ******************************************************************************
 * @file     dma_reg.h
 * @author
 * @brief   director access memory register definition header file
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


#ifndef _RT584_DMA_REG_H_
#define _RT584_DMA_REG_H_


#if defined ( __CC_ARM   )
#pragma anon_unions
#endif


/** @addtogroup REGISTER RT584Z Peripheral Control Register

  @{

*/

/**
    @addtogroup DMA Data Memory Access Controller(DMA)
    Memory Mapped Structure for Data Memory Access Controller
  @{
*/
typedef struct
{
    __IO  uint32_t   DMA_SRC_ADR;            /*offset:0x00*/
    __IO  uint32_t   DMA_DEST_ADR;           /*offset:0x04*/
    __IO  uint32_t   DMA_BYTES_NUM;          /*offset:0x08*/
    __IO  uint32_t   DMA_CONTROL;            /*offset:0x0C*/
    __IO  uint32_t   DMA_INT;                /*offset:0x10*/
    __IO  uint32_t   DMA_PORT;               /*offset:0x14*/
} DMA_T;



#define  DMA_START_ENABLE                (1<<0)
#define  DMA_BUSY                        (1<<8)

#define  DMA_CHL_INT_STATUS              (1<<0)
#define  DMA_CHL_WAKEUP_STATUS           (1<<1)

#define  DMA_CHL_INT_ENABLE              (1<<8)
#define  DMA_CHL_INT_WAKEUP              (1<<9)

#define  DMA_CHL_INT_CLEAR_CLR           (1<<16)
#define  DMA_CHL_INT_WAKEUP_CLR          (1<<17)


/**@}*/ /* end of DMA Controlle group */

/**@}*/ /* end of REGISTER group */

#if defined ( __CC_ARM   )
#pragma no_anon_unions
#endif

#endif      /* end of __RT584_DMA_REG_H__ */
