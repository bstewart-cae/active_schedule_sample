/**
 ******************************************************************************
 * @file    dpd_reg.h
 * @author
 * @brief   dpd  deep power down register definition header file
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


#ifndef __RT584_DPD_REG_H__
#define __RT584_DPD_REG_H__

#if defined (__CC_ARM)
#pragma anon_unions
#endif






//0x00
typedef union dpd_rst_cause_s
{
    struct dpd_rst_cause_b
    {
        uint32_t RST_CAUSE_POR          : 1;
        uint32_t RST_CAUSE_EXT          : 1;
        uint32_t RST_CAUSE_DPD          : 1;
        uint32_t RST_CAUSE_DS           : 1;
        uint32_t RST_CAUSE_WDT          : 1;
        uint32_t RST_CAUSE_SOFT         : 1;
        uint32_t RST_CAUSE_LOCK         : 1;
        uint32_t RESERVED               : 1;
        uint32_t BOOT_STATUS            : 3;
        uint32_t RESERVE2               : 21;
    } bit;
    uint32_t reg;
} dpd_rst_cause_t;

//0x04
typedef union dpd_cmd_s
{
    struct dpd_cmd_b
    {
        uint32_t CLR_RST_CAUSE          : 1;
        uint32_t RESERVED               : 15;
        uint32_t DPD_FLASH_DPD_EN       : 1;
        uint32_t RESERVED2              : 7;
        uint32_t EN_UVL                 : 1;
        uint32_t UVL_OUT_VALID          : 1;
        uint32_t UVL_VTH                : 2;
        uint32_t RESERVED3              : 3;
        uint32_t UVH_DISABLE            : 1;
    } bit;
    uint32_t reg;
} dpd_cmd_t;

/** @addtogroup REGISTER RT584Z Peripheral Control Register

  @{

*/

/**
    @addtogroup DPD Deep Power Down Controller(DPD)
    Memory Mapped Structure for Deep Power Down Controller
  @{
*/

typedef struct
{
    __IO dpd_rst_cause_t        DPD_RST_CAUSE;      /*<! High indicates the reset cause reason. */                      /*offset 0x00*/
    __IO dpd_cmd_t              DPD_CMD;            /*<! command. */                                                                                     /*offset 0x04*/
    __IO uint32_t               DPD_GPIO_EN;        /*<! Set which GPIO can wake up in Deep Power Down mode. */ /*offset 0x08*/
    __IO uint32_t               DPD_GPIO_INV;       /*<! Set the wake up polarity of the corresponding GPIO. */ /*offset 0x0C*/
    __IO uint32_t               DPD_RET0_REG;       /*<! Retention Register 0. *//*offset 0x10*/
    __IO uint32_t               DPD_RET1_REG;       /*<! Retention Register 1. *//*offset 0x14*/
    __IO uint32_t               DPD_RET2_REG;       /*<! Retention Register 2. *//*offset 0x18*/
    __IO uint32_t               DPD_RET3_REG;       /*<! Retention Register 3. *//*offset 0x1C*/
} DPD_T;


/**@}*/ /* end of  DPD Controller group */

/**@}*/ /* end of REGISTER group */

#if defined (__CC_ARM)
#pragma no_anon_unions
#endif

#endif      /* end of __RT584_DPD_REG_H__ */
