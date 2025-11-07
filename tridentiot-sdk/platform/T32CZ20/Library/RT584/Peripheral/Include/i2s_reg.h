/**
  ******************************************************************************
  * @file    i2s.h
  * @author
  * @brief   inter ic sound register definition header file
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


#ifndef __RT584_I2S_REG_H__
#define __RT584_I2S_REG_H__

#if defined ( __CC_ARM   )
#pragma anon_unions
#endif



//0x00
typedef union i2s_ms_ctl0_s
{
    struct i2s_ms_ctl0_b
    {
        uint32_t CFG_I2S_ENA            : 1;
        uint32_t CFG_MCK_ENA            : 1;
        uint32_t CFG_PDM_TX_EN_MUX      : 1;
        uint32_t RESERVED0              : 5;
        uint32_t CFG_I2S_CK_FREE        : 2;
        uint32_t RESERVED1              : 22;
    } bit;
    uint32_t reg;
} i2s_ms_ctl0_t;

//0x04
typedef union i2s_ms_ctl1_s
{
    struct i2s_ms_ctl1_b
    {
        uint32_t CFG_I2S_RST    : 1;
        uint32_t RESERVED       : 31;
    } bit;
    uint32_t reg;
} i2s_ms_ctl1_t;

//0x08
typedef union i2s_mclk_set0_s
{
    struct i2s_mclk_set0_b
    {
        uint32_t CFG_MCK_ISEL   : 3;
        uint32_t RESERVED       : 29;
    } bit;
    uint32_t reg;
} i2s_mclk_set0_t;

//0x0C
typedef union i2s_mclk_set1_s
{
    struct i2s_mclk_set1_b
    {
        uint32_t CFG_MCK_DIV    : 4;
        uint32_t RESERVED       : 28;
    } bit;
    uint32_t reg;
} i2s_mclk_set1_t;

//0x10
typedef union i2s_mclk_set2_s
{
    struct i2s_mclk_set2_b
    {
        uint32_t CFG_MCK_FRA    : 16;
        uint32_t CFG_MCK_INT    : 8;
        uint32_t RESERVED       : 8;
    } bit;
    uint32_t reg;
} i2s_mclk_set2_t;

//0x14
typedef union i2s_ms_set0_s
{
    struct i2s_ms_set0_b
    {
        uint32_t CFG_BCK_OSR    : 2;
        uint32_t CFG_I2S_MOD    : 2;
        uint32_t CFG_I2S_FMT    : 2;
        uint32_t CFG_BCK_LEN    : 2;
        uint32_t CFG_TXD_WID    : 2;
        uint32_t CFG_RXD_WID    : 2;
        uint32_t CFG_TXD_CHN    : 2;
        uint32_t CFG_RXD_CHN    : 2;
        uint32_t CFG_I2S_TST    : 8;
        uint32_t RESERVED       : 4;
        uint32_t CFG_DBG_SEL    : 4;
    } bit;
    uint32_t reg;
} i2s_ms_set0_t;

//0x40
typedef union i2s_rdma_ctl0_s
{
    struct i2s_rdma_ctl0_b
    {
        uint32_t CFG_I2S_RDMA_CTL0      : 4;
        uint32_t RESERVED               : 28;
    } bit;
    uint32_t reg;
} i2s_rdma_ctl0_t;

//0x44
typedef union i2s_rdma_ctl1_s
{
    struct i2s_rdma_ctl1_b
    {
        uint32_t CFG_I2S_RDMA_CTL1      : 1;
        uint32_t RESERVED               : 31;
    } bit;
    uint32_t reg;
} i2s_rdma_ctl1_t;

//0x50
typedef union i2s_rdma_set2_s
{
    struct i2s_rdma_set2_b
    {
        uint32_t CFG_I2S_RDMA_SET2      : 8;
        uint32_t RESERVED               : 24;
    } bit;
    uint32_t reg;
} i2s_rdma_set2_t;

//0x60
typedef union i2s_wdma_ctl0_s
{
    struct i2s_wdma_ctl0_b
    {
        uint32_t CFG_I2S_WDMA_CTL0      : 4;
        uint32_t RESERVED               : 28;
    } bit;
    uint32_t reg;
} i2s_wdma_ctl0_t;

//0x64
typedef union i2s_wdma_ctl1_s
{
    struct i2s_wdma_ctl1_b
    {
        uint32_t CFG_I2S_WDMA_CTL1      : 1;
        uint32_t RESERVED               : 31;
    } bit;
    uint32_t reg;
} i2s_wdma_ctl1_t;

//0x70
typedef union i2s_wdma_set2_s
{
    struct i2s_wdma_set2_b
    {
        uint32_t CFG_I2S_WDMA_SET2      : 8;
        uint32_t RESERVED               : 24;
    } bit;
    uint32_t reg;
} i2s_wdma_set2_t;

/** @addtogroup REGISTER RT584Z Peripheral Control Register

  @{

*/

/**
    @addtogroup I2S Inter Ic Sound Controller(I2S)
    Memory Mapped Structure for Inter Ic Sound Controller
  @{
*/
typedef struct
{
    __IO  i2s_ms_ctl0_t         I2S_MS_CTL0;        /*0x00 */
    __IO  i2s_ms_ctl1_t         I2S_MS_CTL1;        /*0x04 */
    __IO  i2s_mclk_set0_t       I2S_MCLK_SET0;      /*0x08 */
    __IO  i2s_mclk_set1_t       I2S_MCLK_SET1;      /*0x0C */
    __IO  i2s_mclk_set2_t       I2S_MCLK_SET2;      /*0x10 */
    __IO  i2s_ms_set0_t         I2S_MS_SET0;        /*0x14 */
    __IO  uint32_t              I2S_RESVD_1;        /*0x17 */
    __IO  uint32_t              I2S_RESVD_2;        /*0x1C */
    __IO  uint32_t              I2S_RESVD_3;        /*0x20 */
    __IO  uint32_t              I2S_RESVD_4;        /*0x24 */
    __IO  uint32_t              I2S_RESVD_5;        /*0x28 */
    __IO  uint32_t              I2S_RESVD_6;        /*0x2C */
    __IO  uint32_t              I2S_RESVD_7;        /*0x30 */
    __IO  uint32_t              I2S_RESVD_8;        /*0x34 */
    __IO  uint32_t              I2S_RESVD_9;        /*0x38 */
    __IO  uint32_t              I2S_RESVD_10;       /*0x3C */
    __IO  i2s_rdma_ctl0_t       I2S_RDMA_CTL0;      /*0x40 */
    __IO  i2s_rdma_ctl1_t       I2S_RDMA_CTL1;      /*0x44 */
    __IO  uint32_t              I2S_RDMA_SET0;      /*0x48 */
    __IO  uint32_t              I2S_RDMA_SET1;      /*0x4C */
    __IO  i2s_rdma_set2_t       I2S_RDMA_SET2;      /*0x50 */
    __IO  uint32_t              I2S_RESVD_11;     /*0x54 */
    __I   uint32_t              I2S_RDMA_R0;      /*0x58 */
    __I   uint32_t              I2S_RDMA_R1;      /*0x5C */
    __IO  i2s_wdma_ctl0_t       I2S_WDMA_CTL0;    /*0x60 */
    __IO  i2s_wdma_ctl1_t       I2S_WDMA_CTL1;    /*0x64 */
    __IO  uint32_t              I2S_WDMA_SET0;    /*0x68 */
    __IO  uint32_t              I2S_WDMA_SET1;    /*0x6C */
    __IO  i2s_wdma_set2_t       I2S_WDMA_SET2;     /*0x70 */
    __IO  uint32_t              I2S_RESVD_12;     /*0x74 */
    __I   uint32_t              I2S_WDMA_R0;      /*0x78 */
    __I   uint32_t              I2S_WDMA_R1;      /*0x7C */
    __IO  uint32_t              I2S_RESVD_13;     /*0x80 */
    __IO  uint32_t              I2S_RESVD_14;     /*0x84 */
    __IO  uint32_t              I2S_RESVD_15;     /*0x88 */
    __IO  uint32_t              I2S_RESVD_16;     /*0x8C */
    __IO  uint32_t              I2S_RESVD_17;     /*0x90 */
    __IO  uint32_t              I2S_RESVD_18;     /*0x94 */
    __IO  uint32_t              I2S_RESVD_19;     /*0x98 */
    __IO  uint32_t              I2S_RESVD_20;     /*0x9C */
    __IO  uint32_t              I2S_INT_CLEAR;    /*0xA0 */
    __IO  uint32_t              I2S_INT_MASK;     /*0xA4 */
    __I   uint32_t              I2S_INT_STATUS;   /*0xA8 */
} I2S_T;

/* offset 0x40 */
#define I2S_RDMA_ENABLE_SHFT          0
#define I2S_RDMA_ENABLE_MASK          (0x01UL << I2S_RDMA_ENABLE_SHFT)

/* offset 0x44 */
#define I2S_RDMA_RESET_SHFT           0
#define I2S_RDMA_RESET_MASK           (0x01UL << I2S_CFG_I2S_TST_SHFT)

/* offset 0x48 */
#define I2S_RDMA_SEG_SIZE_SHFT        0
#define I2S_RDMA_SEG_SIZE_MASK        (0x0000FFFFUL << I2S_RDMA_SEG_SIZE_SHFT)
#define I2S_RDMA_BLK_SIZE_SHFT        16
#define I2S_RDMA_BLK_SIZE_MASK        (0x0000FFFFUL << I2S_RDMA_BLK_SIZE_SHFT)

/* offset 0x4C */
#define I2S_WDMA_ENABLE_SHFT          0
#define I2S_WDMA_ENABLE_MASK          (0x01UL << I2S_WDMA_ENABLE_SHFT)

/* offset 0x64 */
#define I2S_WDMA_RESET_SHFT           0
#define I2S_WDMA_RESET_MASK           (0x01UL << I2S_WDMA_RESET_SHFT)

/* offset 0x68 */
#define I2S_WDMA_SEG_SIZE_SHFT        0
#define I2S_WDMA_SEG_SIZE_MASK        (0x0000FFFFUL << I2S_WDMA_SEG_SIZE_SHFT)
#define I2S_WDMA_BLK_SIZE_SHFT        16
#define I2S_WDMA_BLK_SIZE_MASK        (0x0000FFFFUL << I2S_WDMA_BLK_SIZE_SHFT)

/* offset 0xA0 */
#define I2S_RDMA_IRQ_CLR_SHFT         0
#define I2S_RDMA_IRQ_CLR_MASK         (0x01UL << I2S_RDMA_IRQ_CLR_SHFT)
#define I2S_RDMA_ERR_IRQ_CLR_SHFT     1
#define I2S_RDMA_ERR_IRQ_CLR_MASK     (0x01UL << I2S_RDMA_ERR_IRQ_CLR_SHFT)
#define I2S_WDMA_IRQ_CLR_SHFT         2
#define I2S_WDMA_IRQ_CLR_MASK         (0x01UL << I2S_WDMA_IRQ_CLR_SHFT)
#define I2S_WDMA_ERR_IRQ_CLR_SHFT     3
#define I2S_WDMA_ERR_IRQ_CLR_MASK     (0x01UL << I2S_WDMA_ERR_IRQ_CLR_SHFT)

/* offset 0xA4 */
#define I2S_RDMA_IRQ_MASK_SHFT        0
#define I2S_RDMA_IRQ_MASK_MASK        (0x01UL << I2S_RDMA_IRQ_MASK_SHFT)
#define I2S_RDMA_ERR_IRQ_MASK_SHFT    1
#define I2S_RDMA_ERR_IRQ_MASK_MASK    (0x01UL << I2S_RDMA_ERR_IRQ_MASK_SHFT)
#define I2S_WDMA_IRQ_MASK_SHFT        2
#define I2S_WDMA_IRQ_MASK_MASK        (0x01UL << I2S_WDMA_IRQ_MASK_SHFT)
#define I2S_WDMA_ERR_IRQ_MASK_SHFT    3
#define I2S_WDMA_ERR_IRQ_MASK_MASK    (0x01UL << I2S_WDMA_ERR_IRQ_MASK_SHFT)

/* offset 0xA8 */
#define I2S_RDMA_IRQ_STATUS_SHFT      0
#define I2S_RDMA_IRQ_STATUS_MASK      (0x01UL << I2S_RDMA_IRQ_STATUS_SHFT)
#define I2S_RDMA_ERR_IRQ_STATUS_SHFT  1
#define I2S_RDMA_ERR_IRQ_STATUS_MASK  (0x01UL << I2S_RDMA_ERR_IRQ_STATUS_SHFT)
#define I2S_WDMA_IRQ_STATUS_SHFT      0
#define I2S_WDMA_IRQ_STATUS_MASK      (0x01UL << I2S_WDMA_IRQ_STATUS_SHFT)
#define I2S_WDMA_ERR_IRQ_STATUS_SHFT  1
#define I2S_WDMA_ERR_IRQ_STATUS_MASK  (0x01UL << I2S_WDMA_ERR_IRQ_STATUS_SHFT)

/**@}*/ /* end of I2S Control group */

/**@}*/ /* end of REGISTER group */
#if defined ( __CC_ARM   )
#pragma no_anon_unions
#endif

#endif      /* end of __RT584_I2S_REG_H__ */

