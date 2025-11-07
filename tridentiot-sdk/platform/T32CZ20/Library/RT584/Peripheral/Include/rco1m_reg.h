/**
 ******************************************************************************
 * @file    rco1m_reg.h
 * @author
 * @brief   1M rc oscillator register header file
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

#ifndef _RT584_RCO1M_REG_H__
#define _RT584_RCO1M_REG_H__

#if defined ( __CC_ARM   )
#pragma anon_unions
#endif


typedef union rco1m_cfg0_s
{
    struct rco1m_cfg0_b
    {
        uint32_t CFG_CAL_TARGET : 18;
        uint32_t RESERVED1      : 6;
        uint32_t CFG_CAL_EN     : 1;
        uint32_t RESERVED2      : 7;
    } bit;
    uint32_t reg;
} rco1m_cfg0_t;


typedef union rco1m_cfg1_s
{
    struct rco1m_cfg1_b
    {
        uint32_t    CFG_CAL_LOCK_ERR    : 8;
        uint32_t    CFG_CAL_AVG_COARSE  : 2;
        uint32_t    CFG_CAL_AVG_FINE    : 2;
        uint32_t    CFG_CAL_AVG_LOCK    : 2;
        uint32_t    CFG_CAL_DLY         : 2;
        uint32_t    CFG_CAL_FINE_GAIN   : 4;
        uint32_t    CFG_CAL_LOCK_GAIN   : 4;
        uint32_t    CFG_CAL_TRACK_EN    : 1;
        uint32_t    CFG_CAL_SKIP_COARSE : 1;
        uint32_t    CFG_CAL_BOUND_MODE  : 1;
        uint32_t    CFG_TUNE_RCO_SEL    : 1;
        uint32_t    EN_CK_CAL           : 1;
        uint32_t    RESERVED1           : 3;
    } bit;
    uint32_t reg;
} rco1m_cfg1_t;


typedef union rco1m_result0_s
{
    struct rco1m_result0_b
    {
        uint32_t    EST_RCO_RESULT          : 18;
        uint32_t    RESERVED1               : 5;
        uint32_t    EST_RCO_RESULT_VALID    : 1;
        uint32_t    RESERVED2               : 3;
        uint32_t    CAL_BUSY                : 1;
        uint32_t    CAL_LOCK                : 1;
        uint32_t    CAL_TIMEOUT             : 1;
        uint32_t    RESERVED3               : 1;
    } bit;
    uint32_t reg;
} rco1m_result0_t;

typedef union rco1m_result1_s
{
    struct rco1m_result1_b
    {
        uint32_t    TUNE_FINE_RCO   : 7;
        uint32_t    RESERVED1       : 9;
        uint32_t    TUNE_COARSE_RCO : 4;
        uint32_t    RESERVED2       : 12;
    } bit;
    uint32_t reg;
} rco1m_result1_t;


/** @addtogroup REGISTER RT584Z Peripheral Control Register

  @{

*/

/**
    @addtogroup RCO1M 1M RC Oscillator Controller(RCO1M)
    Memory Mapped Structure for 1M RC Oscillator Controller
  @{
*/
typedef struct
{
    __IO  rco1m_cfg0_t      CAL1M_CFG0;         /*offset:0x00*/
    __IO  rco1m_cfg1_t      CAL1M_CFG1;         /*offset:0x04*/
    __I   rco1m_result0_t  CAL1M_RESULT0;       /*offset:0x08*/
    __I   rco1m_result1_t  CAL1M_RESULT1;       /*offset:0x0C*/
} RCO1M_CAL_T;



/**@}*/ /* end of RCO1M Control group */

/**@}*/ /* end of REGISTER group */
#if defined ( __CC_ARM   )
#pragma no_anon_unions
#endif

#endif /* end of _RT584_SWI_REG_H */

