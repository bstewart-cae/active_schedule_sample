/**
 ******************************************************************************
 * @file    trng_reg.h
 * @author
 * @brief   timer control register header file
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

#ifndef _RT584_TRNG_REG_H_
#define _RT584_TRNG_REG_H_

#if defined ( __CC_ARM   )
#pragma anon_unions
#endif

/** @addtogroup REGISTER RT584Z Peripheral Control Register

  @{

*/

/**
    @addtogroup TRNG Trng Controller(TRNG)
    Memory Mapped Structure for Trng Controller
  @{
*/
typedef struct trng_ctrl_struct
{

    __IO uint32_t       TRNG0;          /*offset:0x00*/
    __IO uint32_t       TRNG1;          /*offset:0x04*/
    __I  uint32_t       TRNG2;          /*offset:0x08*/
    __IO uint32_t       TRNG3;          /*offset:0x0C*/

} TRNG_T;

/**@}*/ /* end of TRNG Control group */

/**@}*/ /* end of REGISTER group */

#define  TRNG_ENABLE          (1UL << 0)
#define  TRNG_INTR_CLEAR      (1UL << 1)

#define  TRNG_SEL             (1UL << 0)
#define  TRNG_INTR_ENABLE     (1UL << 1)

#define  TRNG_BUSY            (1UL << 0)
#define  TRNG_INTR_STATUS     (1UL << 1)






#if defined ( __CC_ARM   )
#pragma no_anon_unions
#endif

#endif /* end of _RT584_TRNG_REG_H */
