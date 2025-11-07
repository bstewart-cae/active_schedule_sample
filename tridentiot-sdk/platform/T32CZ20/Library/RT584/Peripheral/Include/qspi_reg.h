/**
 ******************************************************************************
 * @file    qspi_reg.h
 * @author
 * @brief   queued serial Perripheral interface register definition header file
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



#ifndef _RT584_QSPI_REG_H__
#define _RT584_QSPI_REG_H__

#if defined ( __CC_ARM   )
#pragma anon_unions
#endif


typedef union
{
    struct
    {
        uint32_t cfg_short_cycle_en: 1;
        uint32_t RVD0: 7;
        uint32_t cfg_short_cycle_num: 5;
        uint32_t RVD1: 3;
        uint32_t cfg_direct_bit_en: 1;
        uint32_t RVD2: 7;
        uint32_t cfg_direct_bit_num: 5;
        uint32_t RVD3: 3;
    } bit;
    uint32_t Reg;
} EPD_FUNC_t;


/** @addtogroup REGISTER RT584Z Peripheral Control Register

  @{

*/

/**
    @addtogroup QSPI Queued Serial Perripheral Interface Controller(QSPI)
    Memory Mapped Structure for Queued Serial Perripheral Interface Controller
  @{
*/
typedef struct
{
    __IO  uint32_t   QSPI_TX_FIFO;           //0x0
    __I   uint32_t   QSPI_RX_FIFO;           //0x4
    __IO  uint32_t   QSPI_CONTROL;           //0x8
    __IO  uint32_t   QSPI_CONTROL2;          //0xC
    __IO  uint32_t   QSPI_SS_CONFIG;         //0x10
    __IO  uint32_t   QSPI_CLKDIV;            //0x14
    __IO  EPD_FUNC_t QSPI_EPD_FUNC;          //0x18
    __IO  uint32_t   QSPI_DELAYS;            //0x1C

    __IO  uint32_t   QSPI_INT_EN;            //0x20
    __I   uint32_t   QSPI_INT_STATUS;        //0x24
    __IO  int32_t    QSPI_INT_CLR;           //0x28

    __IO  uint32_t   QSPI_EN;                //0x2C


    __I   uint32_t   QSPI_STATUS;            //0x30
    __I   uint32_t   QSPI_TX_FIFO_LVL;       //0x34
    __I   uint32_t   QSPI_RX_FIFO_LVL;       //0x38
    __I   uint32_t   RESERVE_1;              //0x3C

    __IO  uint32_t   DMA_RX_ADDR;            //0x40
    __IO  uint32_t   DMA_RX_LEN;             //0x44
    __IO  uint32_t   DMA_TX_ADDR;            //0x48
    __IO  uint32_t   DMA_TX_LEN;             //0x4C

    __I   uint32_t   DMA_RX_RLEN;            //0x50
    __I   uint32_t   DMA_TX_RLEN;            //0x54
    __IO  uint32_t   DMA_IER;                //0x58
    __IO  uint32_t   DMA_INT_STATUS;         //0x5C

    __IO  uint32_t   DMA_RX_ENABLE;          //0x60
    __IO  uint32_t   DMA_TX_ENABLE;          //0x64

} QSPI_T;

#define  QSPI_TX_FIFO_OFFSET          0
#define  QSPI_RX_FIFO_OFFSET          4

/*QSPI_CONTROL */
#define  QSPI_CNTL_MASTER_SHIFT      (0)
#define  QSPI_CNTL_MASTER            (1<<QSPI_CNTL_MASTER_SHIFT)
#define  QSPI_CNTL_SLAVE             (0<<QSPI_CNTL_MASTER_SHIFT)

#define  QSPI_CNTL_CPHA_SHIFT        (1)
#define  QSPI_CNTL_CPOL_SHIFT        (2)

#define  SPI_CNTL_SLAVE_SDATA_SHIFT  (3)
#define  SPI_CNTL_SLAVE_SDATA_OUT    (1<<3)

#define  QSPI_CNTL_EDIAN_SHIFT       (4)
#define  QSPI_CNTL_LITTLE_ENDIAN     (1<<QSPI_CNTL_EDIAN_SHIFT)

#define  QSPI_CNTL_MSB_SHIFT         (5)
#define  QSPI_CNTL_MSB_FIRST         (1<<QSPI_CNTL_MSB_SHIFT)
#define  QSPI_CNTL_LSB_FIRST         (0<<QSPI_CNTL_MSB_SHIFT)

#define  QSPI_CNTL_contXfer_SHIFT    (6)
#define  QSPI_CNTL_contXfer_En       (1<<QSPI_CNTL_contXfer_SHIFT)


#define  QSPI_CNTL_PREDELAY_SHIFT        (8)
#define  QSPI_CNTL_PREDELAY_ENABLE       (1<<8)
#define  QSPI_CNTL_PREDELAY_DISABLE      (0<<8)

#define  QSPI_CNTL_INTER_DELAY_SHIFT     (9)
#define  QSPI_CNTL_INTER_DELAY_ENEABLE   (1<<9)
#define  QSPI_CNTL_INTER_DELAY_DISABLE   (0<<9)

#define  QSPI_CNTL_POSTDELAY_SHIFT       (10)
#define  QSPI_CNTL_POSTDELAY_ENABLE      (1<<10)
#define  QSPI_CNTL_POSTDELAY_DISABLE     (0<<10)


#define  QSPI_CNTL_rxWmark_SHIFT        (12)
#define  QSPI_CNTL_txWmark_SHIFT        (14)

#define  QSPI_CNTL_TX_1_8_WATERMARK     (0<<QSPI_CNTL_txWmark_SHIFT)
#define  QSPI_CNTL_TX_1_4_WATERMARK     (1<<QSPI_CNTL_txWmark_SHIFT)
#define  QSPI_CNTL_TX_HALF_WATERMARK    (2<<QSPI_CNTL_txWmark_SHIFT)
#define  QSPI_CNTL_TX_3_4_WATERMARK     (3<<QSPI_CNTL_txWmark_SHIFT)

#define  QSPI_CNTL_RX_1_8_WATERMARK     (0<<QSPI_CNTL_rxWmark_SHIFT)
#define  QSPI_CNTL_RX_1_4_WATERMARK     (1<<QSPI_CNTL_rxWmark_SHIFT)
#define  QSPI_CNTL_RX_HALF_WATERMARK    (2<<QSPI_CNTL_rxWmark_SHIFT)
#define  QSPI_CNTL_RX_3_4_WATERMARK     (3<<QSPI_CNTL_rxWmark_SHIFT)


#define  QSPI_CNTL_TX_4BYTE_WATERMARK       (0<<QSPI_CNTL_txWmark_SHIFT)
#define  QSPI_CNTL_TX_8BYTE_WATERMARK       (1<<QSPI_CNTL_txWmark_SHIFT)
#define  QSPI_CNTL_TX_16BYTE_WATERMARK      (2<<QSPI_CNTL_txWmark_SHIFT)
#define  QSPI_CNTL_TX_20BYTE_WATERMARK      (3<<QSPI_CNTL_txWmark_SHIFT)

#define  QSPI_CNTL_RX_4BYTE_WATERMARK       (0<<QSPI_CNTL_rxWmark_SHIFT)
#define  QSPI_CNTL_RX_8BYTE_WATERMARK       (1<<QSPI_CNTL_rxWmark_SHIFT)
#define  QSPI_CNTL_RX_16BYTE_WATERMARK      (2<<QSPI_CNTL_rxWmark_SHIFT)
#define  QSPI_CNTL_RX_20BYTE_WATERMARK      (3<<QSPI_CNTL_rxWmark_SHIFT)

#define  QSPI_WIRE_MODE_SHIFT      (0)

#define  QSPI_BITSIZE_8            (1<<4)
#define  QSPI_BITSIZE_16           (3<<4)
#define  QSPI_BITSIZE_32           (7<<4)

#define  QSPI_DISABLE_OUT          (1<<2)
#define  QSPI_DISABLE_IN           (1<<3)

#define  QSPI_Xfer_Extend          (1<<7)


#define  QSPI_PRE_DELAY_SHIFT            (0)
#define  QSPI_INTER_DELAY_SHIFT          (8)
#define  QSPI_POST_DELAY_SHIFT           (16)

#define  QSPI_PRE_DELAY_MASK             (0xFF << QSPI_PRE_DELAY_SHIFT)
#define  QSPI_INTER_DELAY_MASK           (0xFF << QSPI_INTER_DELAY_SHIFT)
#define  QSPI_POST_DELAY_MASK            (0xFF << QSPI_POST_DELAY_SHIFT)

#define  QSPI_SSOUT_SHIFT                (0)

#define  QSPI_SSOUT_MASK                 (0xF<<QSPI_SSOUT_SHIFT)

#define  QSPI_CFG_SSPOL_SHIFT            (8)

#define  QSPI_CFG_SSPOL_MASK             (0xF<<QSPI_CFG_SSPOL_SHIFT)

#define  QSPI_CFG_SS_MANUAL_SHIFT            (16)
#define  QSPI_CFG_SS_MANUUAL_ENABLE_SHIFT    (24)


#define  QSPI_MST_CLKDIV_EN           (1<<8)

/*the following setting base 32MHz OSC*/
#define  QSPI_MST_CLKDIV_16MHZ        (0)
#define  QSPI_MST_CLKDIV_8MHZ         (1)
#define  QSPI_MST_CLKDIV_4MHZ         (3)
#define  QSPI_MST_CLKDIV_2MHZ         (7)
#define  QSPI_MST_CLKDIV_1MHZ         (15)


#define  QSPI_INT_txEmpty             (1<<0)
#define  QSPI_INT_under_txWmark       (1<<1)
#define  QSPI_INT_over_rxWmark        (1<<2)
#define  QSPI_INT_rxFull              (1<<3)
#define  QSPI_INT_xferDone            (1<<4)
#define  QSPI_INT_rxNotEmpty          (1<<5)

#define  QSPI_STATUS_xferIP           (1<<0)            /*transfer in progress*/
#define  QSPI_STATUS_AllCmdDone       (1<<1)
#define  QSPI_STATUS_txEmpty          (1<<2)
#define  QSPI_STATUS_txWmark          (1<<3)
#define  QSPI_STATUS_txFull           (1<<4)
#define  QSPI_STATUS_rxEmpty          (1<<5)
#define  QSPI_STATUS_rxWmark          (1<<6)
#define  QSPI_STATUS_rxFull           (1<<7)


#define  QSPI_MST_CLKDIV_EN           (1<<8)

#define  QSPI_MST_CLKDIV_16MHZ        (0)
#define  QSPI_MST_CLKDIV_8MHZ         (1)
#define  QSPI_MST_CLKDIV_4MHZ         (3)
#define  QSPI_MST_CLKDIV_2MHZ         (7)
#define  QSPI_MST_CLKDIV_1MHZ         (15)


#define  QSPI_DMA_ISR_TX              (1<<1)
#define  QSPI_DMA_ISR_RX              (1<<0)

#define  QSPI_DMA_ISR_CLEARALL        (QSPI_DMA_ISR_TX|QSPI_DMA_ISR_RX)


#define  QSPI_DMA_IER_TX              QSPI_DMA_ISR_TX
#define  QSPI_DMA_IER_RX              QSPI_DMA_ISR_RX

#define  QSPI_DMA_ENABLE              (1<<0)
#define  QSPI_DMA_Dummy_ENABLE        (1<<1)


#define  SPI0_CLK_INDEX               (20)


/**@}*/ /* end of QSPI Control group */

/**@}*/ /* end of REGISTER group */
#if defined ( __CC_ARM   )
#pragma no_anon_unions
#endif

#endif /* end of _RT584_QSPI_REG_H */
