/**
  ******************************************************************************
  * @file    i2c_master.h
  * @author
  * @brief    inter integrated circuit master register definition header file
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




#ifndef __RT584_I2C_MATER_REG_H__
#define __RT584_I2C_MATER_REG_H__

#if defined ( __CC_ARM   )
#pragma anon_unions
#endif

/** @addtogroup REGISTER RT584Z Peripheral Control Register

  @{

*/

/**
    @addtogroup I2C_Master Inter Integrated Circuit Master Controller(I2C_Master)
    Memory Mapped Structure for Inter Integrated Circuit Master Controller
  @{
*/

typedef struct
{
    __IO  uint32_t  CONTROL;            /*0x00*/
    __IO  uint32_t  TAR;                /*0x04*/
    __IO  uint32_t  BUF;                /*0x08*/
    __I   uint32_t  INT_STATUS;         /*0x0C*/
    __IO  uint32_t  INT_ENABLE;         /*0x10*/
    __I   uint32_t  INT_RAW_STATUS;     /*0x14*/
    __IO  uint32_t  INT_CLEAR;          /*0x18*/
    __IO  uint32_t  SLCK_GEN;           /*0x1C*/
} I2C_MASTER_T;


/**@}*/ /* end of I2C Master Controller group */

/**@}*/ /* end of REGISTER  group */

#if defined ( __CC_ARM   )
#pragma no_anon_unions
#endif

#endif      /* end of __RT584_I2C_MATER_REG_H__ */