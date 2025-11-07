/**
  ******************************************************************************
  * @file    i2c_slave.h
  * @author
  * @brief   inter integrated circuit slave register definition header file
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


#ifndef __RT584_I2C_SLAVE_REG_H__
#define __RT584_I2C_SLAVE_REG_H__

#if defined ( __CC_ARM   )
#pragma anon_unions
#endif

/** @addtogroup REGISTER RT584Z Peripheral Control Register

  @{

*/

/**
    @addtogroup I2C_Slave Inter Integrated Circuit Slave Controller(I2C_Slave)
    Memory Mapped Structure for Inter Integrated Circuit Slave Controller
  @{
*/
typedef struct
{
    __IO  uint32_t  RX_DATA;             /*0x00*/
    __IO  uint32_t  I2C_SLAVE_ADDR;      /*0x04*/
    __IO  uint32_t  I2C_INT_ENABLE;      /*0x08*/
    __IO  uint32_t  I2C_INT_STATUS;      /*0x0C*/
    __IO  uint32_t  I2C_TIMEOUT;         /*0x10*/
    __IO  uint32_t  I2C_SLAVE_ENABLE;    /*0x14*/
    __I   uint32_t  I2C_SLAVE_STATUS;    /*0x18*/
} I2C_SLAVE_T;


/**
 *  @Brief I2C Slave interrupt register definitions
 */
#define  WR_DATA     RX_DATA

/**@}*/ /* end of I2C Slave Control group */

/**@}*/ /* end of REGISTER group */

#if defined ( __CC_ARM   )
#pragma no_anon_unions
#endif

#endif      /* end of __RT584_I2C_SLAVE_REG_H__ */
