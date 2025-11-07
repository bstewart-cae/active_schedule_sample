/**
 ******************************************************************************
 * @file    Crypto_reg.h
 * @author
 * @brief   crypto acceleator register definition header file
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
#ifndef _RT584_CRYPTO_REG_H__
#define _RT584_CRYPTO_REG_H__




typedef union crypto_ctrl_struct
{

    struct crypto_ctrl_b
    {
        __IO uint32_t vlw_op_num     : 8;
        __IO uint32_t vlw_sb_num     : 5;
        __IO uint32_t reserved1      : 3;
        __IO uint32_t en_crypto      : 1;
        __IO uint32_t enable_sha     : 1;
        __IO uint32_t reserved2      : 6;
        __IO uint32_t crypto_done    : 1;
        __IO uint32_t sha_done       : 1;
        __IO uint32_t crypto_busy    : 1;
        __IO uint32_t sha_busy       : 1;
        __IO uint32_t reserved3      : 3;
        __IO uint32_t clr_crypto_int : 1;
    } bit;

    uint32_t reg;

} crypto_ctrl_t;

/** @addtogroup REGISTER RT584Z Peripheral Control Register

  @{

*/

/**
    @addtogroup CRYPTO Crypto Acceleator Controller(CRYPTO)
    Memory Mapped Structure for Crypto Acceleator Controller
  @{
*/
typedef struct
{
    __IO crypto_ctrl_t  CRYPTO_CFG;
    __IO uint32_t       SHA_DIGEST_BASE;
    __IO uint32_t       SHA_K_BASE;
    __IO uint32_t       SHA_DMA_BASE;
    __IO uint32_t       SHA_DMA_LENGTH;
    __IO uint32_t       SHA_MISC_CFG;
} CRYPTO_T;


/**@}*/ /* end of CRYPTO Controller group */

/**@}*/ /* end of REGISTER group */



#endif
