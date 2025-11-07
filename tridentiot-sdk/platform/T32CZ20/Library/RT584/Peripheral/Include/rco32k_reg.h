/**
 ******************************************************************************
 * @file    rco32k_reg.h
 * @author
 * @brief   32k rc oscillator register header file
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

#ifndef _RT584_RCO32K_REG_H__
#define _RT584_RCO32K_REG_H__

#if defined ( __CC_ARM   )
#pragma anon_unions
#endif


typedef union rco32k_cfg0_s
{
    struct rco32k_cfg0_b
    {
        uint32_t CFG_CAL32K_TARGET      : 18;
        uint32_t RESERVED1              : 6;
        uint32_t CFG_CAL32K_EN          : 1;
        uint32_t RESERVED2              : 7;
    } bit;
    uint32_t reg;
} rco32k_cfg0_t;


typedef union rco32k_cfg1_s
{
    struct rco32k_cfg1_b
    {
        uint32_t CFG_CAL32K_LOCK_ERR        : 8;
        uint32_t CFG_CAL32K_AVG_COARSE  : 2;
        uint32_t CFG_CAL32K_AVG_FINE        : 2;
        uint32_t CFG_CAL32K_AVG_LOCK        : 2;
        uint32_t CFG_CAL32K_DLY         : 2;
        uint32_t CFG_CAL32K_FINE_GAIN   : 4;
        uint32_t CFG_CAL32K_LOCK_GAIN   : 4;
        uint32_t CFG_CAL32K_TRACK_EN        : 1;
        uint32_t CFG_CAL32K_SKIP_COARSE : 1;
        uint32_t CFG_CAL32K_BOUND_MODE  : 1;
        uint32_t CFG_32K_RC_SEL         : 1;
        uint32_t EN_CK_CAL32K           : 1;
        uint32_t RESERVED1              : 3;
    } bit;
    uint32_t reg;
} rco32k_cfg1_t;


typedef union rco32k_result0_s
{
    struct rco32k_result0_b
    {
        uint32_t EST_32K_RESULT             : 20;
        uint32_t RESERVED1                  : 4;
        uint32_t EST_32K_RESULT_VALID       : 1;
        uint32_t RESERVED2                  : 3;
        uint32_t CAL32K_BUSY                : 1;
        uint32_t CAL32K_LOCK                : 1;
        uint32_t CAL32K_TIMEOUT             : 1;
        uint32_t RESERVED3                  : 1;
    } bit;
    uint32_t reg;
} rco32k_result0_t;

typedef union rco32k_result1_s
{
    struct rco32k_result1_b
    {
        uint32_t  TUNE_FINE_CAL32K        : 8;
        uint32_t  RESERVED1               : 8;
        uint32_t  TUNE_COARSE_CAL32K      : 2;
        uint32_t  RESERVED2               : 14;
    } bit;
    uint32_t reg;
} rco32k_result1_t;

/** @addtogroup REGISTER RT584Z Peripheral Control Register

  @{

*/

/**
    @addtogroup RCO32K 32K RC Oscillator Controller(RCO32K)
    Memory Mapped Structure for 32K RC Oscillator Controller
  @{
*/
typedef struct
{
    __IO  rco32k_cfg0_t     CAL32K_CFG0;        /*offset:0x00*/
    __IO  rco32k_cfg1_t     CAL32K_CFG1;        /*offset:0x04*/
    __I   rco32k_result0_t  CAL32K_RESULT0;     /*offset:0x08*/
    __I   rco32k_result1_t  CAL32K_RESULT1;     /*offset:0x0C*/
} RCO32K_CAL_T;


/**@}*/ /* end of RCO32K Control group */

/**@}*/ /* end of REGISTER group */
#if defined ( __CC_ARM   )
#pragma no_anon_unions
#endif

#endif /* end of _RT584_SWI_REG_H */

