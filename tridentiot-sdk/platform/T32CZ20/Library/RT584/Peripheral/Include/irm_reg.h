/**
  ******************************************************************************
  * @file    irm.h
  * @author
  * @brief   infrared receiver module register definition header file
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

#ifndef __RT584_IRM_REG_H__
#define __RT584_IRM_REG_H__

#if defined ( __CC_ARM   )
#pragma anon_unions
#endif



//0x00
typedef union irm_conf_s
{
    struct irm_conf_b
    {
        uint32_t RESERVED     : 1;
        uint32_t OP_MODE      : 1;
        uint32_t OUT_MODE     : 2;
        uint32_t NO_CAR       : 1;
        uint32_t CAR_INI      : 1;
        uint32_t RESERVED2    : 26;
    } bit;
    uint32_t reg;
} irm_conf_t;

//0x04
typedef union irm_carrier_s
{
    struct irm_carrier_b
    {
        uint32_t CAR_BASE_CNT     : 16;
        uint32_t CAR_LOW_CNT      : 4;
        uint32_t CAR_HIGH_CNT     : 4;
        uint32_t RESERVED         : 8;
    } bit;
    uint32_t reg;
} irm_carrier_t;

//0x08
typedef union irm_fifo_in_s
{
    struct irm_fifo_in_b
    {
        uint32_t ENV_CNT      : 16;
        uint32_t ENV_MARK     : 1;
        uint32_t ENV_INIT     : 1;
        uint32_t ENV_LAST     : 1;
        uint32_t RESERVED     : 13;
    } bit;
    uint32_t reg;
} irm_fifo_in_t;

//0x0c
typedef union irm_status_s
{
    struct irm_status_b
    {
        uint32_t FIFO_LVL       : 5;
        uint32_t FIFO_FULL      : 1;
        uint32_t FIFO_EMPTY     : 1;
        uint32_t RESERVED       : 25;
    } bit;
    uint32_t reg;
} irm_status_t;

//0x10
typedef union irm_cmd_s
{
    struct irm_cmd_b
    {
        uint32_t IR_ENA       : 1;
        uint32_t IR_DIS       : 1;
        uint32_t IR_START     : 1;
        uint32_t IR_RST       : 1;
        uint32_t RESERVED     : 28;
    } bit;
    uint32_t reg;
} irm_cmd_t;

//0x14
typedef union irm_int_status_s
{
    struct irm_int_status_b
    {
        uint32_t ENV_START_INT      : 1;
        uint32_t ENV_LAST_INT       : 1;
        uint32_t ENV_UFL_INT        : 1;
        uint32_t RESERVED           : 29;
    } bit;
    uint32_t reg;
} irm_int_status_t;

//0x18
typedef union irm_int_ena_s
{
    struct irm_int_ena_b
    {
        uint32_t ENV_START_ENA      : 1;
        uint32_t ENV_LAST_ENA       : 1;
        uint32_t ENV_UFL_ENA        : 1;
        uint32_t RESERVED           : 29;
    } bit;
    uint32_t reg;
} irm_int_ena_t;

//0x1C
typedef union irm_int_clr_s
{
    struct irm_int_clr_b
    {
        uint32_t ENV_START_CLR      : 1;
        uint32_t ENV_LAST_CLR       : 1;
        uint32_t ENV_UFL_CLR        : 1;
        uint32_t RESERVED           : 29;
    } bit;
    uint32_t reg;
} irm_int_clr_t;

/** @addtogroup REGISTER RT584Z Peripheral Control Register

  @{

*/

/**
    @addtogroup IRM Infrared Receiver Module Controller(IRM)
    Memory Mapped Structure for Infrared Receiver Module Controller
  @{
*/
typedef struct
{
    __IO irm_conf_t         IR_CONF;        /*0x00*/
    __IO irm_carrier_t      CARRIER ;       /*0x04*/
    __IO irm_fifo_in_t      FIFO_IN;        /*0x08*/
    __I irm_status_t        STATUS ;        /*0x0C*/
    __IO irm_cmd_t          IR_CMD;         /*0x10*/
    __I irm_int_status_t    INT_STATUS;     /*0x14*/
    __IO irm_int_ena_t      INT_ENA;        /*0x18*/
    __IO irm_int_clr_t      INT_CLR;        /*0x1C*/

} IRM_T;

/**@}*/ /* end of IRM Control group */

/**@}*/ /* end of REGISTER group */

#if defined ( __CC_ARM   )
#pragma no_anon_unions
#endif

#endif      /* end of __RT584_IRM_REG_H__ */