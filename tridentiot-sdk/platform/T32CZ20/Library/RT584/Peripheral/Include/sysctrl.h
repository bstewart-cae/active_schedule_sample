/**
 ******************************************************************************
 * @file    sysctrl.h
 * @author
 * @brief   system control driver header file
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


#ifndef __RT584_SYSCTRL_H__
#define __RT584_SYSCTRL_H__

#ifdef __cplusplus
extern "C"
{
#endif


/**************************************************************************************************
 *    INCLUDES
 *************************************************************************************************/
#include "cm33.h"
/**************************************************************************************************
 *    CONSTANTS AND DEFINES
 *************************************************************************************************/
/**
 *  @brief To maintain compatibility with 58x, the modification will be done by using the macro (#define) in the pre-processing stage.
*/
#define pin_get_mode(pin_number)                   Pin_Get_Mode(pin_number)
#define pin_set_mode(pin_number, mode)             Pin_Set_Mode(pin_number, mode)
#define pin_set_mode_ex(pin_number, mode)          Pin_Set_Mode_Ex(pin_number, mode)
#define enable_perclk(clock_id)                    Enable_Perclk(clock_id)
#define disable_perclk(clock)                      Disable_Perclk(clock)
#define pin_set_pullopt(pin_number, mode)          Pin_Set_Pullopt(pin_number, mode)
#define pin_set_drvopt(pin_number, mode)           Pin_Set_Drvopt(pin_number, mode)
#define enable_pin_opendrain(pin_number)           Enable_Pin_Opendrain(pin_number)
#define disable_pin_opendrain(pin_number)          Disable_Pin_Opendrain(pin_number)
#define set_slow_clock_source(mode)                Set_Slow_Clock_Source(mode)
#define set_ext32k_pin(pin_number)                 Set_Ext32k_Pin(pin_number)
#define sys_set_retention_reg(index, value)        Sys_Set_Retention_Reg(index, value)
#define sys_get_retention_reg(index, value)        Sys_Get_Retention_Reg(index, value)
#define set_lowpower_control(value)                Set_Lowpower_Control(value)
#define get_lowpower_control()                     Get_Lowpower_Control()
#define get_clk_control()                          Get_Clk_Control()
#define set_deepsleep_wakeup_pin(value)            Set_Deepsleep_Wakeup_Pin(value)
#define set_deepsleep_wakeup_invert(value)         Set_Deepsleep_Wakeup_Invert(value)
#define set_sram_shutdown_normal(value)            Set_Sram_Shutdown_Normal(value)
#define set_sram_shutdown_sleep(value)             Set_Sram_Shutdown_Sleep(value)
#define get_chip_version()                         Get_Chip_Version()

#define MAP_BASE         ((uint32_t)SYSCTRL + 0x80)
#define PULLOPT_BASE     ((uint32_t)SYSCTRL + 0x20)
#define DRV_BASE         ((uint32_t)SYSCTRL + 0x30)
#define OD_BASE          ((uint32_t)SYSCTRL + 0x38)


#define SEL_CLK_RCO1MHZ             1
#define SEL_CLK_16MHZ               2
#define SEL_CLK_32MHZ               3
#define SEL_CLK_48MHZ               4
#define SEL_CLK_64MHZ               5
#define SEL_CLK_72MHZ               6
#define SEL_CLK_36MHZ               7
#define SEL_CLK_40MHZ               8


/*Define AHB SYSTEM CLOCK mode*/
#define SYS_CLK_32MHZ               0
#define SYS_CLK_16MHZ               2
#define SYS_CLK_RCO1MHZ             3

#define SYS_PLLCLK_48MHZ            0
#define SYS_PLLCLK_64MHZ            1
#define SYS_PLLCLK_RVD1             2
#define SYS_PLLCLK_RVD2             3
#define SYS_PLLCLK_24MHZ            4
#define SYS_PLLCLK_32MHZ            5
#define SYS_PLLCLK_36MHZ            6
#define SYS_PLLCLK_40MHZ            7

#define SYS_PLL_CLK_OFFSET  0x10

/*Because compiler code optimize, we should set PLL_WAIT_PERIOD as 4N */
#define  PLL_WAIT_PERIOD     1024


/**
 *  @brief Pin mux setting.
 *         RT584 NEW PINMUX QSPI0 DOES NOT SUPPORT CSN2 CSN3
 *         RT584 EXT 32K is not set in pinmux, set it in SYC_CRTL register SYS_CLK_CTRL
 *         Use Example: RT58x use MODE_GPIO
 *                      RT584 use MODE_GPIO_EX
*/

#define MODE_GPIO            0          /*!< set pin for GPIO mode  */
#define MODE_QSPI0           1          /*!< set pin for QSPI0 mode */
#define MODE_I2C             4          /*!< set pin for I2C mode   */
#define MODE_UART            6          /*!< set pin for UART mode  */
#define MODE_UART1          (MODE_UART)
#define MODE_UART2          (MODE_UART)

#define MODE_I2S             4          /*!< set pin for I2S mode  */
#define MODE_PWM             4          /*!< set pin for PWM mode  */
#define MODE_PWM0            1          /*!< set pin for PWM0 mode  */
#define MODE_PWM1            2          /*!< set pin for PWM1 mode  */
#define MODE_PWM2            3          /*!< set pin for PWM2 mode  */
#define MODE_PWM3            5          /*!< set pin for PWM3 mode  */
#define MODE_PWM4            7          /*!< set pin for PWM4 mode  */
#define MODE_QSPI1           5          /*!< set pin for QSPI1 mode  */

#define MODE_EXT32K          5          /*!< set pin for EXT32K mode, only GPIO0~GPIO9 available for this setting  */

/*NOTICE: The following setting only in GPIO0~GPIO3*/
#define MODE_QSPI0_CSN1       2         /*!< set pin for QSPI0 CSN1 mode, only GPIO0~GPIO3 available for this setting  */
#define MODE_QSPI0_CSN2       3         /*!< set pin for QSPI0 CSN2 mode, only GPIO0~GPIO3 available for this setting  */
#define MODE_QSPI0_CSN3       6         /*!< set pin for QSPI0 CSN3 mode, only GPIO0~GPIO3 available for this setting  */


/**
 * @brief  RT584 output pin mux register value definitions
 */
#define  MODE_GPIO_REG_VALUE              0x00
#define  MODE_UART0_TX_REG_VALUE          0x01
#define  MODE_UART1_TX_REG_VALUE          0x02
#define  MODE_UART1_RTSN_REG_VALUE        0x03
#define  MODE_UART2_TX_REG_VALUE          0x04
#define  MODE_UART2_RTSN_REG_VALUE        0x05
#define  MODE_PWM0_REG_VALUE              0x06
#define  MODE_PWM1_REG_VALUE              0x07
#define  MODE_PWM2_REG_VALUE              0x08
#define  MODE_PWM3_REG_VALUE              0x09
#define  MODE_PWM4_REG_VALUE              0x0A
#define  MODE_IRM_REG_VALUE               0x0B
#define  MODE_I2CM0_SCL_OUT_REG_VALUE     0x0C
#define  MODE_I2CM0_SDA_OUT_REG_VALUE     0x0D
#define  MODE_I2CM1_SCL_OUT_REG_VALUE     0x0E
#define  MODE_I2CM1_SDA_OUT_REG_VALUE     0x0F
#define  MODE_I2CS_SCL_OUT_REG_VALUE      0x10
#define  MODE_I2CS_SDA_OUT_REG_VALUE      0x11
#define  MODE_SPI0_SCLK_OUT_REG_VALUE     0x12
#define  MODE_SPI0_SDATA_OUT0_REG_VALUE   0x13
#define  MODE_SPI0_SDATA_OUT1_REG_VALUE   0x14
#define  MODE_SPI0_SDATA_OUT2_REG_VALUE   0x15
#define  MODE_SPI0_SDATA_OUT3_REG_VALUE   0x16
#define  MODE_SPI0_CSN0_REG_VALUE         0x17
#define  MODE_SPI0_CSN1_REG_VALUE         0x18
#define  MODE_SPI0_CSN2_REG_VALUE         0x19
#define  MODE_SPI0_CSN3_REG_VALUE         0x1A
#define  MODE_SPI1_SCLK_OUT_REG_VALUE     0x1B
#define  MODE_SPI1_SDATA_OUT0_REG_VALUE   0x1C
#define  MODE_SPI1_SDATA_OUT1_REG_VALUE   0x1D
#define  MODE_SPI1_SDATA_OUT2_REG_VALUE   0x1E
#define  MODE_SPI1_SDATA_OUT3_REG_VALUE   0x1F
#define  MODE_SPI1_CSN0_REG_VALUE         0x20
#define  MODE_SPI1_CSN1_REG_VALUE         0x21
#define  MODE_SPI1_CSN2_REG_VALUE         0x22
#define  MODE_SPI1_CSN3_REG_VALUE         0x23
#define  MODE_I2S_BCK_REG_VALUE           0x24
#define  MODE_I2S_WCK_REG_VALUE           0x25
#define  MODE_I2S_SDO_REG_VALUE           0x26
#define  MODE_I2S_MCLK_REG_VALUE          0x27
#define  MODE_SWD_REG_VALUE               0x2F
#define  MODE_DBG0_REG_VALUE              0x30
#define  MODE_DBG1_REG_VALUE              0x31
#define  MODE_DBG2_REG_VALUE              0x32
#define  MODE_DBG3_REG_VALUE              0x33
#define  MODE_DBG4_REG_VALUE              0x34
#define  MODE_DBG5_REG_VALUE              0x35
#define  MODE_DBG6_REG_VALUE              0x36
#define  MODE_DBG7_REG_VALUE              0x37
#define  MODE_DBG8_REG_VALUE              0x38
#define  MODE_DBG9_REG_VALUE              0x39
#define  MODE_DBGA_REG_VALUE              0x3A
#define  MODE_DBGB_REG_VALUE              0x3B
#define  MODE_DBGC_REG_VALUE              0x3C
#define  MODE_DBGD_REG_VALUE              0x3D
#define  MODE_DBGE_REG_VALUE              0x3E
#define  MODE_DBGF_REG_VALUE              0x3F


/**
 * @brief  RT584 input pin mux register value definitions
 */
//A0
#define  MODE_UART1_RX_REG_VALUE           0x00000000
#define  MODE_UART1_CTSN_REG_VALUE         0x00000008
#define  MODE_UART2_RX_REG_VALUE           0x00000010
#define  MODE_UART2_CTSN_REG_VALUE         0x00000018
//A4
#define  MODE_UART0_RX_REG_VALUE           0x10000000
#define  MODE_I2S_SDI_REG_VALUE            0x10000008
#define  MODE_I2CS_SCL_REG_VALUE           0x10000010
#define  MODE_I2CS_SDA_REG_VALUE           0x10000018
//A8
#define  MODE_I2CM0_SCL_REG_VALUE          0x20000000
#define  MODE_I2CM0_SDA_REG_VALUE          0x20000008
#define  MODE_I2CM1_SCL_REG_VALUE          0x20000010
#define  MODE_I2CM1_SDA_REG_VALUE          0x20000018
//AC
#define  MODE_QSPI0_CSN_REG_VALUE          0x40000000
#define  MODE_QSPI0_SCLK_REG_VALUE         0x40000008
#define  MODE_QSPI0_SDATA0_REG_VALUE       0x40000010
#define  MODE_QSPI0_SDATA1_REG_VALUE       0x40000018
//B0
#define  MODE_QSPI0_SDATA2_REG_VALUE       0x50000000
#define  MODE_QSPI0_SDATA3_REG_VALUE       0x50000008
//B4
#define  MODE_QSPI1_CSN_REG_VALUE          0x60000000
#define  MODE_QSPI1_SCLK_REG_VALUE         0x60000008
#define  MODE_QSPI1_SDATA0_REG_VALUE       0x60000010
#define  MODE_QSPI1_SDATA1_REG_VALUE       0x60000018
//B8
#define  MODE_QSPI1_SDATA2_REG_VALUE       0x70000000
#define  MODE_QSPI1_SDATA3_REG_VALUE       0x70000008

#define  MODE_PIN_MUX_NULL                 0xFFFFFFFF



/**
 * @brief  RT584 pin mux definitions
 */
#define  MODE_GPIO_EX              0x00
#define  MODE_UART0_TX_EX          0x01
#define  MODE_UART0_RX_EX          0x02
#define  MODE_UART1_TX_EX          0x03
#define  MODE_UART1_RX_EX          0x04
#define  MODE_UART1_RTSN_EX        0x05
#define  MODE_UART1_CTSN_EX        0x06
#define  MODE_UART2_TX_EX          0x07
#define  MODE_UART2_RX_EX          0x08
#define  MODE_UART2_RTSN_EX        0x09
#define  MODE_UART2_CTSN_EX        0x0A
#define  MODE_PWM0_EX              0x0B
#define  MODE_PWM1_EX              0x0C
#define  MODE_PWM2_EX              0x0D
#define  MODE_PWM3_EX              0x0E
#define  MODE_PWM4_EX              0x0F
#define  MODE_IRM_EX               0x10
#define  MODE_I2CM0_SCL_EX         0x11
#define  MODE_I2CM0_SDA_EX         0x12
#define  MODE_I2CM1_SCL_EX         0x13
#define  MODE_I2CM1_SDA_EX         0x14
#define  MODE_I2CS_SCL_EX          0x15
#define  MODE_I2CS_SDA_EX          0x16


#define  MODE_SPI0_MASTER_SCLK_EX  0x17
#define  MODE_SPI0_MASTER_CSN0_EX  0x18
#define  MODE_SPI0_MASTER_MOSI_EX  0x19
#define  MODE_SPI0_MASTER_MISO_EX  0x1A
#define  MODE_SPI0_SLAVE_SCLK_EX   0x1B
#define  MODE_SPI0_SLAVE_CSN0_EX   0x1C
#define  MODE_SPI0_SLAVE_MOSI_EX   0x1D
#define  MODE_SPI0_SLAVE_MISO_EX   0x1E
#define  MODE_SPI1_MASTER_SCLK_EX  0x1F
#define  MODE_SPI1_MASTER_CSN0_EX  0x20
#define  MODE_SPI1_MASTER_MOSI_EX  0x21
#define  MODE_SPI1_MASTER_MISO_EX  0x22
#define  MODE_SPI1_SLAVE_SCLK_EX   0x23
#define  MODE_SPI1_SLAVE_CSN0_EX   0x24
#define  MODE_SPI1_SLAVE_MOSI_EX   0x25
#define  MODE_SPI1_SLAVE_MISO_EX   0x26
#define  MODE_SPI0_MASTER_SDATA0_EX 0x27
#define  MODE_SPI0_MASTER_SDATA1_EX 0x28
#define  MODE_SPI0_MASTER_SDATA2_EX 0x29
#define  MODE_SPI0_MASTER_SDATA3_EX 0x2A
#define  MODE_SPI1_MASTER_SDATA0_EX 0x2B
#define  MODE_SPI1_MASTER_SDATA1_EX 0x2C
#define  MODE_SPI1_MASTER_SDATA2_EX 0x2D
#define  MODE_SPI1_MASTER_SDATA3_EX 0x2E
#define  MODE_SPI0_MASTER_CSN1_EX  0x2F
#define  MODE_SPI0_MASTER_CSN2_EX  0x30
#define  MODE_SPI0_MASTER_CSN3_EX  0x31
#define  MODE_SPI0_SLAVE_CSN1_EX   0x32
#define  MODE_SPI0_SLAVE_CSN2_EX   0x33
#define  MODE_SPI0_SLAVE_CSN3_EX   0x34
#define  MODE_SPI1_MASTER_CSN1_EX  0x35
#define  MODE_SPI1_MASTER_CSN2_EX  0x36
#define  MODE_SPI1_MASTER_CSN3_EX  0x37
#define  MODE_SPI1_SLAVE_CSN1_EX   0x38
#define  MODE_SPI1_SLAVE_CSN2_EX   0x39
#define  MODE_SPI1_SLAVE_CSN3_EX   0x3A
#define  MODE_I2S_BCK_EX           0x3B
#define  MODE_I2S_WCK_EX           0x3C
#define  MODE_I2S_SDO_EX           0x3D
#define  MODE_I2S_SDI_EX           0x3E
#define  MODE_I2S_MCLK_EX          0x3F
#define  MODE_SWD_EX               0x40
#define  MODE_DBG0_EX              0x41
#define  MODE_DBG1_EX              0x42
#define  MODE_DBG2_EX              0x43
#define  MODE_DBG3_EX              0x44
#define  MODE_DBG4_EX              0x45
#define  MODE_DBG5_EX              0x46
#define  MODE_DBG6_EX              0x47
#define  MODE_DBG7_EX              0x48
#define  MODE_DBG8_EX              0x49
#define  MODE_DBG9_EX              0x4A
#define  MODE_DBGA_EX              0x4B
#define  MODE_DBGB_EX              0x4C
#define  MODE_DBGC_EX              0x4D
#define  MODE_DBGD_EX              0x4E
#define  MODE_DBGE_EX              0x4F
#define  MODE_DBGF_EX              0x50
#define  MODE_MAX_EX               0x51

/*Driving through setting mode*/
#define PULL_NONE         0          /*!< set pin for no pull, if you set pin to GPIO output mode, system will set this option for output pin */
#define PULLDOWN_10K      1          /*!< set pin for 10K pull down  */
#define PULLDOWN_100K     2          /*!< set pin for 100K pull down  */
#define PULLDOWN_1M       3          /*!< set pin for 1M pull down  */
#define PULLUP_10K        5          /*!< set pin for 10K pull up  */
#define PULLUP_100K       6          /*!< set pin for 100K pull up, this is default pin mode */
#define PULLUP_1M         7          /*!< set pin for 1M pull up  */


/**
 * @brief  Define pin driver option
 */
#define DRV_4MA             0               /*!< set pin for 4mA driver   */
#define DRV_10MA            1               /*!< set pin for 10mA driver  */
#define DRV_14MA            2               /*!< set pin for 14mA driver  */
#define DRV_20MA            3               /*!< set pin for 20mA driver  */

/**
 * @brief  Define IC chip id  and chip revision information
 */


#define IC_CHIP_ID_MASK_SHIFT               16
#define IC_CHIP_ID_MASK                             (0xFF<<IC_CHIP_ID_MASK_SHIFT)
#define IC_RT58X                            (0x70<<IC_CHIP_ID_MASK_SHIFT)           /*!< RT584 IC Chip ID  */
#define IC_CHIP_REVISION_MASK_SHIFT         0
#define IC_CHIP_REVISION_MASK               (0xF<<IC_CHIP_REVISION_MASK_SHIFT)
#define IC_CHIP_REVISION_MPA                (1<<IC_CHIP_REVISION_MASK_SHIFT)       /*!< RT584 IC Chip Revision ID For MPA  */
#define IC_CHIP_REVISION_MPB                (2<<IC_CHIP_REVISION_MASK_SHIFT)       /*!< RT584 IC Chip Revision ID For MPB  */


#define DPD_RETENTION_BASE          0x10

/**************************************************************************************************
 *    TYPEDEFS
 *************************************************************************************************/
typedef enum
{
    SYS_32MHZ_CLK = SYS_CLK_32MHZ,
    SYS_16MHZ_CLK = SYS_CLK_16MHZ,
    SYS_RCO1MHZ_CLK = SYS_CLK_RCO1MHZ,
    SYS_48MHZ_PLLCLK = SYS_PLL_CLK_OFFSET + SYS_PLLCLK_48MHZ,
    SYS_64MHZ_PLLCLK = SYS_PLL_CLK_OFFSET + SYS_PLLCLK_64MHZ,
    SYS_RVD1_PLLCLK  = SYS_PLL_CLK_OFFSET + SYS_PLLCLK_RVD1,
    SYS_RVD2_PLLCLK  = SYS_PLL_CLK_OFFSET + SYS_PLLCLK_RVD2,
    SYS_24MHZ_PLLCLK = SYS_PLL_CLK_OFFSET + SYS_PLLCLK_24MHZ,
    SYS_32MHZ_PLLCLK = SYS_PLL_CLK_OFFSET + SYS_PLLCLK_32MHZ,
    SYS_36MHZ_PLLCLK = SYS_PLL_CLK_OFFSET + SYS_PLLCLK_36MHZ,
    SYS_40MHZ_PLLCLK = SYS_PLL_CLK_OFFSET + SYS_PLLCLK_40MHZ,
    SYS_CLK_MAX,
} sys_clk_sel_t;

/**
 * @brief Selecting the hclk source.
 */
typedef enum
{
    HCLK_SEL_32M,
    HCLK_SEL_PLL,
    HCLK_SEL_16M,
    HCLK_SEL_RCO1M,
} hclk_clk_sel_t;

/**
 * @brief Selecting the peripheral source.
 */
typedef enum
{
    PERCLK_SEL_32M,
    PERCLK_SEL_16M = 2,
    PERCLK_SEL_RCO1M,
} perclk_clk_sel_t;

/**
 * @brief Enable peripheral interface clock.
 */
typedef enum
{
    UART0_CLK,
    UART1_CLK,
    UART2_CLK,
    QSPI0_CLK = 4,
    QSPI1_CLK,
    I2CM0_CLK = 8,
    I2CM1_CLK,
    I2CS0_CLK,
    CRYPTO_CLK = 12,
    XDMA_CLK,
    IRM_CLK,
    TIMER0_CLK = 16,
    TIMER1_CLK,
    TIMER2_CLK,
    TIMER3_32K_CLK = 20,
    TIMER4_32K_CLK,
    RTC_32K_CLK,
    GPIO_32K_CLK,
    RTC_PCLK,
    GPIO_PCLK,
    AUX_PCLK,
    BOD_PCLK,
    AUX_32K_CLK,
    BOD_32K_CLK,
    CLK_32K_DIV,
    RCO32K_DIV2
} per_interface_clk_en_t;

/** @addtogroup Peripheral_Driver RT584Z Periperhal Driver Function
  @{
*/

typedef enum
{
    RCO32K_SELECT = 1,               /*!< System slow clock 32k Mode */
    RCO16K_SELECT,                   /*!< System slow clock 16k Mode */
    RCO1M_SELECT,                    /*!< System slow clock 1M Mode */
    RCO_NULL,
} slow_clock_select_t;

/** @addtogroup SYSCTRL_DRIVER System Control Driver Functions
  @{
*/
/**************************************************************************************************
 *    GLOBAL PROTOTYPES
 *************************************************************************************************/
void Delay_Init(void);
/**
 * @brief Delay_us
 *
 * @param[in] us
 *
 * @retval  none
 *
 * @details
 *
 *
 */
void Delay_us(unsigned int us);
/**
 * @brief Delay_ms
 *
 * @param[in] ms
 *
 * @retval none
 *
 * @details
 *
 *
 */
void Delay_ms(unsigned int ms);

/**
 * @brief get pin function mode
 *
 * @param[in] pin_number    Specifies the pin number.
 *                                                  GPIO0~GPIO30
 * @return
 *         get the pin function mode UART/I2S/PWM/SADC/I2C/SPI....
 * @details
 *         each pin has different function pin setting, please read RT58x datasheet to know each pin
 *   function usage setting.
 */
uint32_t Pin_Get_Mode(uint32_t pin_number);
/**
 * @brief set pin function mode
 *
 * @param[in] pin_number    Specifies the pin number.
 *                                                  GPIO0~GPIO30
 * @param[in] mode          The specail function mode for the pin_number
 *                                              Config GPIO To --> UART/I2S/PWM/SADC/I2C/SPI...
 * @return
 *         NONE
 * @details
 *         each pin has different function pin setting, please read RT58x datasheet to know each pin
 *   function usage setting.
 */
void Pin_Set_Mode(uint32_t pin_number, uint32_t mode);

/**
 * @brief set pin function mode
 *
 * @param[in] pin_number    Specifies the pin number.
 *                                                  GPIO0~GPIO30
 * @param[in] mode          The specail function mode for the pin_number
 *                                              Config GPIO To --> UART/I2S/PWM/SADC/I2C/SPI...
 * @return
 *         NONE
 * @details
 *         each pin has different function pin setting, please read RT58x datasheet to know each pin
 *   function usage setting.
 */
void Pin_Set_Mode_Ex(uint32_t pin_number, uint32_t mode);

/**
 * @brief set pin function mode extend for RT584
 *
 * @param[in] pin_number    Specifies the pin number.
 *                                                  GPIO0~GPIO31
 * @param[in] mode          The specail function mode for the pin_number
 *                                              Config GPIO To --> UART/I2S/PWM/SADC/I2C/SPI...
 * @return
 *         NONE
 * @details
 *         each pin has different function pin setting, please read RT584 datasheet to know each pin
 *   function usage setting.
 */
void Pin_Set_Out_Mode_Ex(uint32_t pin_number, uint32_t mode);

/**
 * @brief set pin function mode extend for RT584
 *
 * @param[in] pin_number    Specifies the pin number.
 *                                                  GPIO0~GPIO31
 * @param[in] mode          The specail function mode for the pin_number
 *                                              Config GPIO To --> UART/I2S/PWM/SADC/I2C/SPI...
 * @return
 *         NONE
 * @details
 *         each pin has different function pin setting, please read RT584 datasheet to know each pin
 *   function usage setting.
 */
void Pin_Set_In_Mode_Ex(uint32_t pin_number, uint32_t mode);

/**
 * @brief get pin function mode
 *
 * @param[in] pin_number    Specifies the pin number.
 *                                                  GPIO0~GPIO31
 * @return
 *         Pin mode
 * @details
 *         each pin has different function pin setting, please read RT584 datasheet to know each pin
 *   function usage setting.
 */
uint32_t Pin_Get_Mode(uint32_t pin_number);

/**
 * @brief enable peripherial interface clock
 *
 * @param[in] clock_id    enable the specifies peripheral "clock_id" interface clock.
 *                                              UART0_CLK
 *                                              UART1_CLK
 *                                              UART2_CLK
 *                                              I2CM_CLK
 *                                              QSPI0_CLK
 *                                              QSPI1_CLK
 *                                              TIMER1_CLK
 *                                              TIMER2_CLK
 *                                              I2S_CLK
 * @return
 *         NONE
 */
void Enable_Perclk(uint32_t clock);
/**
 * @brief disable peripherial interface clock
 *
 * @param[in] clock_id  disable the specifies peripheral "clock_id" interface clock.
 *                                          UART0_CLK
 *                                          UART1_CLK
 *                                          UART2_CLK
 *                                          I2CM_CLK
 *                                          QSPI0_CLK
 *                                          QSPI1_CLK
 *                                          TIMER1_CLK
 *                                          TIMER2_CLK
 *                                          I2S_CLK
 * @return
 *         NONE
 */
void Disable_Perclk(uint32_t clock);
/**
 * @brief Set pin pull option.
 *
 * @param[in] clock   Specifies the pin number.
 *                                      PULL_NONE        0
 *                                      PULL_DOWN_10K    1
 *                                      PULL_DOWN_100K   2
 *                                      PULL_DOWN_1M     3
 *                                      PULL_UP_10K      5
 *                                      PULL_UP_100K     6
 *                                      PULL_UP_1M       7
 * @return
 *         NONE
 * @details
 *      Pin default pull option is 100K pull up, User can use this function to change the pull up/down setting.
 *      If user set the pin  set to gpio output mode, then the pin will auto to be set as no pull option.
 *
 */
void Pin_Set_Pullopt(uint32_t pin_number, uint32_t mode);
/**
 * @brief Set pin driving option
 *
 * @param[in] pin_number    Specifies the pin number.
 * @param[in] mode              pin driving option
 *                                              DRV_4MA      0
 *                                              DRV_10MA     1
 *                                              DRV_14MA     2
 *                                              DRV_20MA     3
 * @return
 *         NONE
 *
 * @details
 *      Pin default driving option is 20mA, User can use this function to change the pin driving setting.
 *
 */
void Pin_Set_Drvopt(uint32_t pin_number, uint32_t mode);
/**
 * @brief Set pin to opendrain option
 *
 * @param[in] pin_number  Specifies the pin number.
 *                                              GPIO0~GPIO31
 * @return
 *         NONE
 *
 * @details
 *        Set the pin to opendrain mode.
 */
void Enable_Pin_Opendrain(uint32_t pin_number);
/**
 * @brief Disable pin to opendrain option
 *
 * @param[in] pin_number   Specifies the pin number
 *                                               GPIO0~GPIO31
 * @return
 *         NONE
 *
 * @details
 *        Reset the pin to non-opendrain mode.
 */
void Disable_Pin_Opendrain(uint32_t pin_number);

/*
 * Change CPU AHB CLOCK,
 *      return STATUS_SUCCESS(0) for change success.
 *      return STATUS_ERROR      for change fail.
 *
 */
uint32_t Change_Ahb_System_Clk(sys_clk_sel_t sys_clk_mode);

/*
 * Get CPU AHB CLOCK,
 *      return SYS_CLK_32MHZ      for CPU AHB 32MHz clock.
 *      return SYS_CLK_48MHZ      for CPU AHB 48MHz clock.
 *      return SYS_CLK_64MHZ      for CPU AHB 64MHz clock.
 *
 */
uint32_t Get_Ahb_System_Clk(void);

/**
 * @brief enable peripherial interface clock
 *
 * @param[in] perclk_clk_sel_t    enable the specifies peripheral "clock_id" interface clock.
 *                                  PERCLK_SEL_32M
 *                                  PERCLK_SEL_16M
 *                                  PERCLK_SEL_RCO1M
 *
 * @return
 *         NONE
 */
uint32_t Change_Peri_Clk(perclk_clk_sel_t sys_clk_mode);

/**
 * @brief enable peripherial interface clock
 *
 * @return perclk_clk_sel_t    enable the specifies peripheral "clock_id" interface clock.
 *                                  PERCLK_SEL_32M
 *                                  PERCLK_SEL_16M
 *                                  PERCLK_SEL_RCO1M
 *
 */
uint32_t Get_Peri_Clk(void);


/*
 * Select Slow clock source.
 * Available mode:
 *         SLOW_CLOCK_INTERNAL   --- default value.
 *                  If system don't call this function, then slow clock source is from internal RCO by default.
 *
 *          SLOW_CLOCK_FROM_GPIO ---
 *                 If system set this mode, system should use an external 32K source from GPIO.
 *
 *
 */
void Set_Slow_Clock_Source(uint32_t mode);

/**
 * @brief Check IC version
 *
 * @param  None
 *
 * @retval    IC version
 *
 * @details   Return IC version information
 *             Bit7 ~ Bit4 is chip_revision
 *             Bit15 ~ Bit8 is chip_id
 */
__STATIC_INLINE uint32_t Get_Chip_Version(void)
{
    return ((uint32_t)(SYSCTRL->SOC_CHIP_INFO.reg));
}


/** for security world
 * @brief sys_set_retention_reg. Use to save some retention value.
 *
 * @param index: The index for which scratchpad register to save
 *                It should be 0~3.
 * @param value: register value
 *
 * @details   Please notice when system power-reset (cold boot), all system retention scratchpad register (0~3)
 *
 *
 *
 */

__STATIC_INLINE void sys_set_retention_reg(uint32_t index, uint32_t value)
{
    if (index < 3)
    {
        outp32(((uint32_t)DPD_CTRL + DPD_RETENTION_BASE + (index << 2)), value);
    }
}

/** for security world using
 * @brief sys_get_retention_reg. Use to get retention value.
 *
 * @param[in]   index:     The index for which scratchpad register to get
 *                         It should be 0~3.
 * @param[out]  *value     the address for return value.
 *
 *
 */

__STATIC_INLINE void sys_get_retention_reg(uint32_t index, uint32_t *value)
{
    if (index < 3)
    {
        *value =  inp32(((uint32_t)DPD_CTRL + DPD_RETENTION_BASE + (index << 2)));
    }
    else
    {
        *value = 0;
    }
}
/** for security world using
 * @brief sys_get_retention_reg. Use to get retention value.
 *
 * @param[in]   index:     The index for which scratchpad register to get
 *                         It should be 0~3.
 * @param[out]  *value     the address for return value.
 *
 *
 */
__STATIC_INLINE void set_deepsleep_wakeup_pin(uint32_t value)
{
    GPIO->SET_DS_EN |= value;
}
/** for security world using
 * @brief sys_get_retention_reg. Use to get retention value.
 *
 * @param[in]   index:     The index for which scratchpad register to get
 *                         It should be 0~3.
 * @param[out]  *value     the address for return value.
 *
 *
 */
__STATIC_INLINE void set_deepsleep_wakeup_invert(uint32_t value)
{
    GPIO->SET_DS_INV |= value;
}

/**
 * @brief sys_get_retention_reg. Use to get retention value.
 *
 * @param[in]   index:     The index for which scratchpad register to get
 *                         It should be 0~3.
 * @param[out]  *value     the address for return value.
 *
 *
 */
__STATIC_INLINE void Set_Sram_Shutdown_Normal(uint32_t value)
{
    SYSCTRL->SRAM_LOWPOWER_1.reg = (SYSCTRL->SRAM_LOWPOWER_1.reg & 0xFF00) | value;
}
/**
 * @brief sys_get_retention_reg. Use to get retention value.
 *
 * @param[in]   index:     The index for which scratchpad register to get
 *                         It should be 0~3.
 * @param[out]  *value     the address for return value.
 *
 *
 */
__STATIC_INLINE void Set_Sram_Shutdown_Sleep(uint32_t value)
{
    SYSCTRL->SRAM_LOWPOWER_1.reg = (SYSCTRL->SRAM_LOWPOWER_1.reg & 0x2000) | (value << 16);
}
/**
 * @brief sys_get_retention_reg. Use to get retention value.
 *
 * @param[in]   value:     The index for which scratchpad register to get
 *                         It should be 0~3.
 *
 *
 */
__STATIC_INLINE void Set_Sram_Shutdown_DeepSleep(uint32_t value)
{
    SYSCTRL->SRAM_LOWPOWER_2.reg |= value;
}
/**
 * @brief Set Peripheral group power off
 *
 * @param[in] isenabled
 *            value: peripheral group 1/peripheral group 2 /peripheral group 3
 *
 */
__STATIC_INLINE void Set_Peripheral_Power_Off (uint32_t peripwroffctlbit, uint32_t isenabled)
{
    if (isenabled == 1)
    {
        SYSCTRL->SRAM_LOWPOWER_3.reg |= peripwroffctlbit;
    }
    else
    {
        SYSCTRL->SRAM_LOWPOWER_3.reg &= ~peripwroffctlbit;
    }

}


/**
 * @brief if postmasking flag set... write_otp_key
 * is no use. write_otp_key is only use when postmasking is
 * not setting.
 */
__STATIC_INLINE void Enable_Secure_Write_Protect(void)
{
    SEC_CTRL->SEC_OTP_WRITE_KEY = SEC_WRITE_OTP_MAGIC_KEY;

}
/**
 * @brief disable secure write protect
 *
 *
 *
 */
__STATIC_INLINE void Disable_Secure_Write_Protect(void)
{
    SEC_CTRL->SEC_OTP_WRITE_KEY = 0;
}
/**
 * @brief enable pin schmitt
 *
 *
 *
 */
void pin_enable_schmitt(uint32_t pin_number);
/**
 * @brief disable pin schmitt
 *
 *
 *
 */
void pin_disable_schmitt(uint32_t pin_number);
/**
 * @brief enable pin filter
 *
 *
 *
 */
void pin_enable_filter(uint32_t pin_number);
/**
 * @brief disable pin filter
 *
 *
 *
 */
void pin_disable_filter(uint32_t pin_number);

/**
 * @brief slow clock calibration
 */
void Slow_Clock_Calibration(slow_clock_select_t rco_cal);
/**@}*/ /* end of SYSCTRL_DRIVER group */

/**@}*/ /* end of PERIPHERAL_DRIVER group */

#ifdef __cplusplus
}
#endif

#endif      /* end of __RT584_SYSCTRL_H__ */
