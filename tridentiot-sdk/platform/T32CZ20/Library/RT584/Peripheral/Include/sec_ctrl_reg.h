/**
 ******************************************************************************
 * @file    sec_ctrl_reg.h
 * @author
 * @brief   security controller register header file
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

#ifndef __RT_SEC_CONTROL_REG_H__
#define __RT_SEC_CONTROL_REG_H__



typedef enum SEC_IADU_Type
{
    /******  CM33 Specific security attribuite Numbers *************************************************/
    //attribuite 0
    SYS_CTRL_IADU_Type          = 0,
    GPIO_IADU_Type              = 1,
    RTC_IADU_Type               = 4,
    DPD_CTRL_IADU_Type          = 5,
    SOC_PMU_IADU_Type           = 6,
    FLASH_CONTROL_IADU_Type = 9,
    TIMER0_IADU_Type            = 10,
    TIMER1_IADU_Type            = 11,
    TIMER2_IADU_Type            = 12,
    TIMER32K0_IADU_Type         = 13,
    TIMER32K1_IADU_Type         = 14,
    WDT_IADU_Type               = 16,
    UART0_IADU_Type             = 18,
    UART1_IADU_Type             = 19,
    I2C_S_IADU_Type             = 24,
    RT569_AHB_IADU_Type         = 26,
    RCO32K_CAL_IADU_Type        = 28,
    BOD_COMP_IADU_Type          = 29,
    AUX_COMP_IADU_Type          = 30,
    RCO1M_CAL_IADU_Type         = 31,
    //attribuite 1
    QSPI0_IADU_Type             = 32,
    QSPI1_IADU_Type             = 33,
    IRM_IADU_Type               = 36,
    UART2_IADU_Type             = 37,
    PWM_IADU_Type               = 38,
    XDMA_IADU_Type              = 40,
    DMA0_IADU_Type              = 41,
    DMA1_IADU_Type              = 42,
    I2C_M0_IADU_Type            = 43,
    I2C_M1_IADU_Type            = 44,
    I2S0_M_IADU_Type            = 45,
    SADC_IADU_Type              = 47,
    SW_IRQ0_IADU_Type           = 48,
    SW_IRQ1_IADU_Type           = 49,
    //attribuite 2
    CRYPTO_IADU_Type            = 64,
    PUF_OTP_IADU_Type           = 65,
} SEC_IADU_Type;


typedef union sec_int_en_ctrl_s
{
    struct sec_int_en_ctrl_b
    {
        uint32_t SEC_EN_ROM_ERR_INT                  : 1;
        uint32_t SEC_EN_FLASH_ERR_INT                : 1;
        uint32_t SEC_EN_RAM_ERR_INT                  : 1;
        uint32_t SEC_EN_PERI_ERR_INT                 : 1;
        uint32_t SEC_EN_CRYPTO_ERR_INT               : 1;
        uint32_t SEC_EN_PUF_ERR_INT                  : 1;
        uint32_t RESERVED1                           : 26;
    } bit;
    uint32_t reg;
} sec_int_en_ctrl_t;


typedef union sec_int_clr_ctrl_s
{
    struct sec_int_clr_ctrl_b
    {
        uint32_t SEC_CLR_ROM_ERR_INT                  : 1;
        uint32_t SEC_CLR_FLASH_ERR_INT                : 1;
        uint32_t SEC_CLR_RAM_ERR_INT                  : 1;
        uint32_t SEC_CLR_PERI_ERR_INT                 : 1;
        uint32_t SEC_CLR_CRYPTO_ERR_INT               : 1;
        uint32_t SEC_CLR_PUF_ERR_INT                  : 1;
        uint32_t RESERVED1                            : 26;
    } bit;
    uint32_t reg;
} sec_int_clr_ctrl_t;


typedef union sec_int_status_ctrl_s
{
    struct sec_int_status_ctrl_b
    {
        uint32_t SEC_STATUS_ROM_ERR_INT                  : 1;
        uint32_t SEC_STATUS_FLASH_ERR_INT                : 1;
        uint32_t SEC_STATUS_RAM_ERR_INT                  : 1;
        uint32_t SEC_STATUS_PERI_ERR_INT                 : 1;
        uint32_t SEC_STATUS_CRYPTO_ERR_INT               : 1;
        uint32_t SEC_STATUS_PUF_ERR_INT                  : 1;
        uint32_t RESERVED1                               : 26;
    } bit;
    uint32_t reg;
} sec_int_status_ctrl_t;


/** @addtogroup REGISTER RT584Z Peripheral Control Register

  @{

*/

/**
    @addtogroup SECURE Security Controller(SECURE)
    Memory Mapped Structure for Security Controller
  @{
*/

typedef struct
{
    __IO uint32_t SEC_FLASH_SEC_SIZE;     /*0x00*/

    __IO uint32_t SEC_FLASH_NSC_START;    /*0x04*/
    __IO uint32_t SEC_FLASH_NSC_STOP;     /*0x08*/

    __IO uint32_t SEC_FLASH_NS_STOP;      /*0x0C*/

    __IO uint32_t SEC_RAM_SEC_SIZE;       /*0x10*/

    __IO uint32_t SEC_RAM_NSC_START;      /*0x14*/
    __IO uint32_t SEC_RAM_NSC_STOP;       /*0x18*/
    __IO uint32_t SEC_RAM_NS_STOP;        /*0x1C*/

    __IO uint32_t SEC_PERI_ATTR[3];       /*0x20~28*/
    __IO uint32_t SEC_IDAU_CTRL;          /*0x2C*/

    __IO sec_int_en_ctrl_t SEC_INT_EN;             /*0x30*/

    __IO sec_int_clr_ctrl_t SEC_INT_CLR;            /*0x34*/
    __IO sec_int_status_ctrl_t SEC_INT_STATUS;         /*0x38*/
    __IO uint32_t RESV1;                  /*0x3C*/

    __IO uint32_t SEC_MCU_DEBUG;          /*0x40*/
    __IO uint32_t SEC_LOCK_MCU_CTRL;      /*0x44*/

    __IO uint32_t SEC_OTP_WRITE_KEY;      /*0x48*/
} SEC_CTRL_T;




#define ENABLE_IDAU_SEC_CTRL               1
#define DISABLE_IDAU_SEC_CTRL              0


#define SEC_ROM_ERR_INT                 (1<<0)
#define SEC_FLASH_ERR_INT               (1<<1)
#define SEC_RAM_ERR_INT                 (1<<2)
#define SEC_PERI_ERR_INT                (1<<3)
#define SEC_CRYPTO_ERR_INT              (1<<4)


#define SEC_DBGEN                       (1)
#define SEC_SPIDEN                      (2)
#define SEC_NIDEN                       (4)
#define SEC_SPNIDEN                     (8)


#define SEC_LOCK_SVTAIRCR               (1<<0)
#define SEC_LOCK_NSVTOR                 (1<<1)
#define SEC_LOCK_SMPU                   (1<<1)
#define SEC_LOCK_NSMPU                  (1<<3)
#define SEC_LOCK_SAU                    (1<<4)


/*Attribute setting*/

/*For REG SEC_PERI_ATTR0*/
#define SEC_IDAU_SYS_CTRL                   BIT0            /*  (1UL << 0)   */
#define SEC_IDAU_GPIO                       BIT1            /*  (1UL << 1)   */
#define SEC_IDAU_SEC_CTRL                   BIT3            /*  (1UL << 3)    */
#define SEC_IDAU_RTC                        BIT4            /*  (1UL << 4)    */
#define SEC_IDAU_DPD_CTRL                   BIT5            /*  (1UL << 5)    */
#define SEC_IDAU_SOC_PMU                    BIT6            /*  (1UL << 6)    */
#define SEC_IDAU_FLASH_CTRL                 BIT9            /*  (1UL << 9)    */
#define SEC_IDAU_TIMER0                     BIT10           /*  (1UL << 10)   */
#define SEC_IDAU_TIMER1                     BIT11           /*  (1UL << 11)   */
#define SEC_IDAU_TIMER2                     BIT12           /*  (1UL << 12)   */
#define SEC_IDAU_TIMER32K0                  BIT13           /*  (1UL << 13)   */
#define SEC_IDAU_TIMER32K1                  BIT14           /*  (1UL << 14)   */
#define SEC_IDAU_WDT                        BIT16           /*  (1UL << 16)   */
#define SEC_IDAU_UART0                      BIT18           /*  (1UL << 18)   */
#define SEC_IDAU_UART1                      BIT19           /*  (1UL << 19)   */
#define SEC_IDAU_I2C_SLAVE                  BIT24           /*  (1UL << 24)   */
#define SEC_IDAU_COMM_AHB                   BIT26           /*  (1UL << 26)   */
#define SEC_IDAU_RCO32_CAL                  BIT28           /*  (1UL << 28)   */
#define SEC_IDAU_BOC_CMP                    BIT29           /*  (1UL << 29)   */
#define SEC_IDAU_AUX_CMP                    BIT30           /*  (1UL << 30)   */
#define SEC_IDAU_RCO1M_CAL                  BIT31           /*  (1UL << 31)   */
/*For REG SEC_PERI_ATTR1*/
#define SEC_IDAU_QSPI0                      BIT0            /*  (1UL << 0)    */
#define SEC_IDAU_QSPI1                      BIT1            /*  (1UL << 1)    */
#define SEC_IDAU_IRM                        BIT4            /*  (1UL << 4)    */
#define SEC_IDAU_UART2                      BIT5            /*  (1UL << 5)    */
#define SEC_IDAU_PWM                        BIT6            /*  (1UL << 6)    */
#define SEC_IDAU_XDMA                       BIT8            /*  (1UL << 8)    */
#define SEC_IDAU_DMA0                       BIT9            /*  (1UL << 9)    */
#define SEC_IDAU_DMA1                       BIT10           /*  (1UL << 10)   */
#define SEC_IDAU_I2C_MASTER0                BIT11           /*  (1UL << 11)   */
#define SEC_IDAU_I2C_MASTER1                BIT12           /*  (1UL << 12)   */
#define SEC_IDAU_I2S0_MASTER                BIT13           /*  (1UL << 13)   */
#define SEC_IDAU_SADC                       BIT16           /*  (1UL << 15)   */
#define SEC_IDAU_SW_IRQ0                    BIT16           /*  (1UL << 16)   */
#define SEC_IDAU_SW_IRQ1                    BIT17           /*  (1UL << 17)   */
#define SEC_PERI_ATTRI1                                     0
/*For REG SEC_PERI_ATTR2*/
#define SEC_IDAU_CRYPTO                     BIT0            /*  (1UL << 0)    */
#define SEC_IDAU_OTP                        BIT1            /*  (1UL << 1)    */

#define SEC_WRITE_OTP_MAGIC_KEY         (0x28514260)
/**@}*/ /* end of  SEC Controller group */

/**@}*/ /* end of REGISTER group */


#endif

