/******************************************************************************
 * @file     pin_mux_define.h
 * @version
 * @brief    rt58x pin mux define header file
 *
 * @copyright
 ******************************************************************************/
/** @defgroup pin mux define
 *  @ingroup  peripheral_group
 *  @{
 *  @brief  Pin Mux Define header information
*/

#ifndef _PIN_MUX_DEFINE_H_
#define _PIN_MUX_DEFINE_H_

#include "sysctrl.h"

#ifdef __cplusplus
extern "C" {
#endif

/* {Pin, Old_Pin_Mux, New_Pin_Mux_Out, New_Pin_Mux_In}*/
uint32_t rt58x_pin_mux_define[32][8][4] =
{
    //GPIO0
    {
        {0, MODE_GPIO,          MODE_GPIO_REG_VALUE,       MODE_PIN_MUX_NULL},
        {0, MODE_PIN_MUX_NULL,  MODE_PIN_MUX_NULL,         MODE_PIN_MUX_NULL},
        {0, MODE_QSPI0_CSN1,    MODE_SPI0_CSN1_REG_VALUE,  MODE_PIN_MUX_NULL},
        {0, MODE_QSPI0_CSN2,    MODE_SPI0_CSN2_REG_VALUE,  MODE_PIN_MUX_NULL},
        {0, MODE_I2S,           MODE_I2S_BCK_REG_VALUE,    MODE_PIN_MUX_NULL},
        {0, MODE_EXT32K,        MODE_GPIO_REG_VALUE,       MODE_PIN_MUX_NULL},
        {0, MODE_QSPI0_CSN3,    MODE_SPI0_CSN3_REG_VALUE,  MODE_PIN_MUX_NULL},
        {0, MODE_PIN_MUX_NULL,  MODE_PIN_MUX_NULL,         MODE_PIN_MUX_NULL},
    },
    //GPIO1
    {
        {1, MODE_GPIO,          MODE_GPIO_REG_VALUE,       MODE_PIN_MUX_NULL},
        {1, MODE_PIN_MUX_NULL,  MODE_PIN_MUX_NULL,         MODE_PIN_MUX_NULL},
        {1, MODE_QSPI0_CSN1,    MODE_SPI0_CSN1_REG_VALUE,  MODE_PIN_MUX_NULL},
        {1, MODE_QSPI0_CSN2,    MODE_SPI0_CSN2_REG_VALUE,  MODE_PIN_MUX_NULL},
        {1, MODE_I2S,           MODE_I2S_WCK_REG_VALUE,    MODE_PIN_MUX_NULL},
        {1, MODE_EXT32K,        MODE_GPIO_REG_VALUE,       MODE_PIN_MUX_NULL},
        {1, MODE_QSPI0_CSN3,    MODE_SPI0_CSN3_REG_VALUE,  MODE_PIN_MUX_NULL},
        {1, MODE_PIN_MUX_NULL,  MODE_PIN_MUX_NULL,         MODE_PIN_MUX_NULL},
    },
    //GPIO2
    {
        {2, MODE_GPIO,          MODE_GPIO_REG_VALUE,       MODE_PIN_MUX_NULL},
        {2, MODE_PIN_MUX_NULL,  MODE_PIN_MUX_NULL,         MODE_PIN_MUX_NULL},
        {2, MODE_QSPI0_CSN1,    MODE_SPI0_CSN1_REG_VALUE,  MODE_PIN_MUX_NULL},
        {2, MODE_QSPI0_CSN2,    MODE_SPI0_CSN2_REG_VALUE,  MODE_PIN_MUX_NULL},
        {2, MODE_I2S,           MODE_I2S_SDO_REG_VALUE,    MODE_PIN_MUX_NULL},
        {2, MODE_EXT32K,        MODE_GPIO_REG_VALUE,       MODE_PIN_MUX_NULL},
        {2, MODE_QSPI0_CSN3,    MODE_SPI0_CSN3_REG_VALUE,  MODE_PIN_MUX_NULL},
        {2, MODE_PIN_MUX_NULL,  MODE_PIN_MUX_NULL,         MODE_PIN_MUX_NULL},
    },
    //GPIO3
    {
        {3, MODE_GPIO,          MODE_GPIO_REG_VALUE,       MODE_PIN_MUX_NULL},
        {3, MODE_PIN_MUX_NULL,  MODE_PIN_MUX_NULL,         MODE_PIN_MUX_NULL},
        {3, MODE_QSPI0_CSN1,    MODE_SPI0_CSN1_REG_VALUE,  MODE_PIN_MUX_NULL},
        {3, MODE_QSPI0_CSN2,    MODE_SPI0_CSN2_REG_VALUE,  MODE_PIN_MUX_NULL},
        {3, MODE_I2S,           MODE_PIN_MUX_NULL,         MODE_I2S_SDI_REG_VALUE},
        {3, MODE_EXT32K,        MODE_GPIO_REG_VALUE,       MODE_PIN_MUX_NULL},
        {3, MODE_QSPI0_CSN3,    MODE_SPI0_CSN3_REG_VALUE,  MODE_PIN_MUX_NULL},
        {3, MODE_PIN_MUX_NULL,  MODE_PIN_MUX_NULL,         MODE_PIN_MUX_NULL},
    },
    //GPIO4
    {
        {4, MODE_GPIO,          MODE_GPIO_REG_VALUE,               MODE_PIN_MUX_NULL},
        {4, MODE_QSPI0,         MODE_SPI0_SDATA_OUT2_REG_VALUE,    MODE_QSPI0_SDATA2_REG_VALUE},
        {4, MODE_PIN_MUX_NULL,  MODE_PIN_MUX_NULL,                 MODE_PIN_MUX_NULL},
        {4, MODE_PIN_MUX_NULL,  MODE_PIN_MUX_NULL,                 MODE_PIN_MUX_NULL},
        {4, MODE_I2S,           MODE_I2S_MCLK_REG_VALUE,           MODE_PIN_MUX_NULL},
        {4, MODE_EXT32K,        MODE_GPIO_REG_VALUE,               MODE_PIN_MUX_NULL},
        {4, MODE_UART,          MODE_UART1_TX_REG_VALUE,           MODE_PIN_MUX_NULL},
        {4, MODE_PIN_MUX_NULL,  MODE_PIN_MUX_NULL,                 MODE_PIN_MUX_NULL},
    },
    //GPIO5
    {
        {5, MODE_GPIO,          MODE_GPIO_REG_VALUE,               MODE_PIN_MUX_NULL},
        {5, MODE_QSPI0,         MODE_SPI0_SDATA_OUT3_REG_VALUE,    MODE_QSPI0_SDATA3_REG_VALUE},
        {5, MODE_PIN_MUX_NULL,  MODE_PIN_MUX_NULL,                 MODE_PIN_MUX_NULL},
        {5, MODE_PIN_MUX_NULL,  MODE_PIN_MUX_NULL,                 MODE_PIN_MUX_NULL},
        {5, MODE_I2S,           MODE_I2S_MCLK_REG_VALUE,           MODE_PIN_MUX_NULL},
        {5, MODE_EXT32K,        MODE_GPIO_REG_VALUE,               MODE_PIN_MUX_NULL},
        {5, MODE_UART,          MODE_PIN_MUX_NULL,                 MODE_UART1_RX_REG_VALUE},
        {5, MODE_PIN_MUX_NULL,  MODE_PIN_MUX_NULL,                 MODE_PIN_MUX_NULL},
    },
    //GPIO6
    {
        {6, MODE_GPIO,          MODE_GPIO_REG_VALUE,               MODE_PIN_MUX_NULL},
        {6, MODE_QSPI0,         MODE_SPI0_SCLK_OUT_REG_VALUE,      MODE_QSPI0_SCLK_REG_VALUE},
        {6, MODE_PIN_MUX_NULL,  MODE_PIN_MUX_NULL,                 MODE_PIN_MUX_NULL},
        {6, MODE_PIN_MUX_NULL,  MODE_PIN_MUX_NULL,                 MODE_PIN_MUX_NULL},
        {6, MODE_PIN_MUX_NULL,  MODE_PIN_MUX_NULL,                 MODE_PIN_MUX_NULL},
        {6, MODE_EXT32K,        MODE_GPIO_REG_VALUE,               MODE_PIN_MUX_NULL},
        {6, MODE_UART,          MODE_UART2_TX_REG_VALUE,           MODE_PIN_MUX_NULL},
        {6, MODE_PIN_MUX_NULL,  MODE_PIN_MUX_NULL,                 MODE_PIN_MUX_NULL},
    },
    //GPIO7
    {
        {7, MODE_GPIO,          MODE_GPIO_REG_VALUE,               MODE_PIN_MUX_NULL},
        {7, MODE_QSPI0,         MODE_SPI0_CSN0_REG_VALUE,          MODE_QSPI0_CSN_REG_VALUE},
        {7, MODE_PIN_MUX_NULL,  MODE_PIN_MUX_NULL,                 MODE_PIN_MUX_NULL},
        {7, MODE_PIN_MUX_NULL,  MODE_PIN_MUX_NULL,                 MODE_PIN_MUX_NULL},
        {7, MODE_PIN_MUX_NULL,  MODE_PIN_MUX_NULL,                 MODE_PIN_MUX_NULL},
        {7, MODE_EXT32K,        MODE_GPIO_REG_VALUE,               MODE_PIN_MUX_NULL},
        {7, MODE_UART,          MODE_PIN_MUX_NULL,                 MODE_UART2_RX_REG_VALUE},
        {7, MODE_PIN_MUX_NULL,  MODE_PIN_MUX_NULL,                 MODE_PIN_MUX_NULL},
    },
    //GPIO8
    {
        {8, MODE_GPIO,          MODE_GPIO_REG_VALUE,               MODE_PIN_MUX_NULL},
        {8, MODE_QSPI0,         MODE_SPI0_SDATA_OUT0_REG_VALUE,    MODE_QSPI0_SDATA0_REG_VALUE},
        {8, MODE_PIN_MUX_NULL,  MODE_PIN_MUX_NULL,                 MODE_PIN_MUX_NULL},
        {8, MODE_PIN_MUX_NULL,  MODE_PIN_MUX_NULL,                 MODE_PIN_MUX_NULL},
        {8, MODE_PIN_MUX_NULL,  MODE_PIN_MUX_NULL,                 MODE_PIN_MUX_NULL},
        {8, MODE_EXT32K,        MODE_GPIO_REG_VALUE,               MODE_PIN_MUX_NULL},
        {8, MODE_PIN_MUX_NULL,  MODE_PIN_MUX_NULL,                 MODE_PIN_MUX_NULL},
        {8, MODE_PIN_MUX_NULL,  MODE_PIN_MUX_NULL,                 MODE_PIN_MUX_NULL},
    },
    //GPIO9
    {
        {9, MODE_GPIO,          MODE_GPIO_REG_VALUE,               MODE_PIN_MUX_NULL},
        {9, MODE_QSPI0,         MODE_SPI0_SDATA_OUT1_REG_VALUE,    MODE_QSPI0_SDATA1_REG_VALUE},
        {9, MODE_PIN_MUX_NULL,  MODE_PIN_MUX_NULL,                 MODE_PIN_MUX_NULL},
        {9, MODE_PIN_MUX_NULL,  MODE_PIN_MUX_NULL,                 MODE_PIN_MUX_NULL},
        {9, MODE_PIN_MUX_NULL,  MODE_PIN_MUX_NULL,                 MODE_PIN_MUX_NULL},
        {9, MODE_EXT32K,        MODE_GPIO_REG_VALUE,               MODE_PIN_MUX_NULL},
        {9, MODE_PIN_MUX_NULL,  MODE_PIN_MUX_NULL,                 MODE_PIN_MUX_NULL},
        {9, MODE_PIN_MUX_NULL,  MODE_PIN_MUX_NULL,                 MODE_PIN_MUX_NULL},
    },
    //GPIO10
    {
        {10, MODE_GPIO,         MODE_GPIO_REG_VALUE,               MODE_PIN_MUX_NULL},
        {10, MODE_PWM0,         MODE_PWM0_REG_VALUE,               MODE_PIN_MUX_NULL},
        {10, MODE_PWM1,         MODE_PWM1_REG_VALUE,               MODE_PIN_MUX_NULL},
        {10, MODE_PWM2,         MODE_PWM2_REG_VALUE,               MODE_PIN_MUX_NULL},
        {10, MODE_I2S,          MODE_I2S_BCK_REG_VALUE,            MODE_PIN_MUX_NULL},
        {10, MODE_PWM3,         MODE_PWM3_REG_VALUE,               MODE_PIN_MUX_NULL},
        {10, MODE_UART,         MODE_UART1_TX_REG_VALUE,           MODE_PIN_MUX_NULL},
        {10, MODE_PWM4,         MODE_PWM4_REG_VALUE,               MODE_PIN_MUX_NULL},
    },
    //GPIO11
    {
        {11, MODE_GPIO,         MODE_GPIO_REG_VALUE,               MODE_PIN_MUX_NULL},
        {11, MODE_PWM0,         MODE_PWM0_REG_VALUE,               MODE_PIN_MUX_NULL},
        {11, MODE_PWM1,         MODE_PWM1_REG_VALUE,               MODE_PIN_MUX_NULL},
        {11, MODE_PWM2,         MODE_PWM2_REG_VALUE,               MODE_PIN_MUX_NULL},
        {11, MODE_I2S,          MODE_I2S_WCK_REG_VALUE,            MODE_PIN_MUX_NULL},
        {11, MODE_PWM3,         MODE_PWM3_REG_VALUE,               MODE_PIN_MUX_NULL},
        {11, MODE_UART,         MODE_PIN_MUX_NULL,                 MODE_UART1_RX_REG_VALUE},
        {11, MODE_PWM4,         MODE_PWM4_REG_VALUE,               MODE_PIN_MUX_NULL},
    },
    //GPIO12
    {
        {12, MODE_GPIO,         MODE_GPIO_REG_VALUE,               MODE_PIN_MUX_NULL},
        {12, MODE_PWM0,         MODE_PWM0_REG_VALUE,               MODE_PIN_MUX_NULL},
        {12, MODE_PWM1,         MODE_PWM1_REG_VALUE,               MODE_PIN_MUX_NULL},
        {12, MODE_PWM2,         MODE_PWM2_REG_VALUE,               MODE_PIN_MUX_NULL},
        {12, MODE_I2S,          MODE_I2S_SDO_REG_VALUE,            MODE_PIN_MUX_NULL},
        {12, MODE_PWM3,         MODE_PWM3_REG_VALUE,               MODE_PIN_MUX_NULL},
        {12, MODE_UART,         MODE_UART2_TX_REG_VALUE,           MODE_PIN_MUX_NULL},
        {12, MODE_PWM4,         MODE_PWM4_REG_VALUE,               MODE_PIN_MUX_NULL},
    },
    //GPIO13
    {
        {13, MODE_GPIO,         MODE_GPIO_REG_VALUE,               MODE_PIN_MUX_NULL},
        {13, MODE_PWM0,         MODE_PWM0_REG_VALUE,               MODE_PIN_MUX_NULL},
        {13, MODE_PWM1,         MODE_PWM1_REG_VALUE,               MODE_PIN_MUX_NULL},
        {13, MODE_PWM2,         MODE_PWM2_REG_VALUE,               MODE_PIN_MUX_NULL},
        {13, MODE_I2S,          MODE_PIN_MUX_NULL,                 MODE_I2S_SDI_REG_VALUE},
        {13, MODE_PWM3,         MODE_PWM3_REG_VALUE,               MODE_PIN_MUX_NULL},
        {13, MODE_UART,         MODE_PIN_MUX_NULL,                 MODE_UART2_RX_REG_VALUE},
        {13, MODE_PWM4,         MODE_PWM4_REG_VALUE,               MODE_PIN_MUX_NULL},
    },
    //GPIO14
    {
        {14, MODE_GPIO,          MODE_GPIO_REG_VALUE,               MODE_PIN_MUX_NULL},
        {14, MODE_QSPI0,         MODE_SPI0_SDATA_OUT2_REG_VALUE,    MODE_QSPI0_SDATA2_REG_VALUE},
        {14, MODE_PIN_MUX_NULL,  MODE_PIN_MUX_NULL,                 MODE_PIN_MUX_NULL},
        {14, MODE_PIN_MUX_NULL,  MODE_PIN_MUX_NULL,                 MODE_PIN_MUX_NULL},
        {14, MODE_PIN_MUX_NULL,  MODE_PIN_MUX_NULL,                 MODE_PIN_MUX_NULL},
        {14, MODE_QSPI1,         MODE_SPI1_SDATA_OUT2_REG_VALUE,    MODE_QSPI1_SDATA2_REG_VALUE},
        {14, MODE_UART,          MODE_UART1_RTSN_REG_VALUE,         MODE_PIN_MUX_NULL},
        {14, MODE_PIN_MUX_NULL,  MODE_PIN_MUX_NULL,                 MODE_PIN_MUX_NULL},
    },
    //GPIO15
    {
        {15, MODE_GPIO,          MODE_GPIO_REG_VALUE,               MODE_PIN_MUX_NULL},
        {15, MODE_QSPI0,         MODE_SPI0_SDATA_OUT3_REG_VALUE,    MODE_QSPI0_SDATA3_REG_VALUE},
        {15, MODE_PIN_MUX_NULL,  MODE_PIN_MUX_NULL,                 MODE_PIN_MUX_NULL},
        {15, MODE_PIN_MUX_NULL,  MODE_PIN_MUX_NULL,                 MODE_PIN_MUX_NULL},
        {15, MODE_PIN_MUX_NULL,  MODE_PIN_MUX_NULL,                 MODE_PIN_MUX_NULL},
        {15, MODE_PIN_MUX_NULL,  MODE_PIN_MUX_NULL,                 MODE_PIN_MUX_NULL},
        {15, MODE_UART,          MODE_PIN_MUX_NULL,                 MODE_UART1_CTSN_REG_VALUE},
        {15, MODE_PIN_MUX_NULL,  MODE_PIN_MUX_NULL,                 MODE_PIN_MUX_NULL},
    },
    //GPIO16
    {
        {16, MODE_GPIO,         MODE_GPIO_REG_VALUE,               MODE_PIN_MUX_NULL},
        {16, MODE_PIN_MUX_NULL, MODE_PIN_MUX_NULL,                 MODE_PIN_MUX_NULL},
        {16, MODE_PIN_MUX_NULL, MODE_PIN_MUX_NULL,                 MODE_PIN_MUX_NULL},
        {16, MODE_PIN_MUX_NULL, MODE_PIN_MUX_NULL,                 MODE_PIN_MUX_NULL},
        {16, MODE_PIN_MUX_NULL, MODE_PIN_MUX_NULL,                 MODE_PIN_MUX_NULL},
        {16, MODE_PIN_MUX_NULL, MODE_PIN_MUX_NULL,                 MODE_PIN_MUX_NULL},
        {16, MODE_UART,         MODE_PIN_MUX_NULL,                 MODE_UART0_RX_REG_VALUE},
        {16, MODE_PIN_MUX_NULL, MODE_PIN_MUX_NULL,                 MODE_PIN_MUX_NULL},
    },
    //GPIO17
    {
        {17, MODE_GPIO,         MODE_GPIO_REG_VALUE,               MODE_PIN_MUX_NULL},
        {17, MODE_PIN_MUX_NULL, MODE_PIN_MUX_NULL,                 MODE_PIN_MUX_NULL},
        {17, MODE_PIN_MUX_NULL, MODE_PIN_MUX_NULL,                 MODE_PIN_MUX_NULL},
        {17, MODE_PIN_MUX_NULL, MODE_PIN_MUX_NULL,                 MODE_PIN_MUX_NULL},
        {17, MODE_PIN_MUX_NULL, MODE_PIN_MUX_NULL,                 MODE_PIN_MUX_NULL},
        {17, MODE_PIN_MUX_NULL, MODE_PIN_MUX_NULL,                 MODE_PIN_MUX_NULL},
        {17, MODE_UART,         MODE_UART0_TX_REG_VALUE,           MODE_PIN_MUX_NULL},
        {17, MODE_PIN_MUX_NULL, MODE_PIN_MUX_NULL,                 MODE_PIN_MUX_NULL},
    },
    //GPIO18
    {
        {18, MODE_GPIO,         MODE_GPIO_REG_VALUE,               MODE_PIN_MUX_NULL},
        {18, MODE_PWM0,         MODE_PWM0_REG_VALUE,               MODE_PIN_MUX_NULL},
        {18, MODE_PWM1,         MODE_PWM1_REG_VALUE,               MODE_PIN_MUX_NULL},
        {18, MODE_PWM2,         MODE_PWM2_REG_VALUE,               MODE_PIN_MUX_NULL},
        {18, MODE_I2C,          MODE_I2CM0_SDA_OUT_REG_VALUE,      MODE_I2CM0_SDA_REG_VALUE},
        {18, MODE_PWM3,         MODE_PWM3_REG_VALUE,               MODE_PIN_MUX_NULL},
        {18, MODE_UART,         MODE_PIN_MUX_NULL,                 MODE_UART1_CTSN_REG_VALUE},
        {18, MODE_PWM4,         MODE_PWM4_REG_VALUE,               MODE_PIN_MUX_NULL},
    },
    //GPIO19
    {
        {19, MODE_GPIO,         MODE_GPIO_REG_VALUE,               MODE_PIN_MUX_NULL},
        {19, MODE_PWM0,         MODE_PWM0_REG_VALUE,               MODE_PIN_MUX_NULL},
        {19, MODE_PWM1,         MODE_PWM1_REG_VALUE,               MODE_PIN_MUX_NULL},
        {19, MODE_PWM2,         MODE_PWM2_REG_VALUE,               MODE_PIN_MUX_NULL},
        {19, MODE_I2C,          MODE_I2CM0_SDA_OUT_REG_VALUE,      MODE_I2CM0_SDA_REG_VALUE},
        {19, MODE_PWM3,         MODE_PWM3_REG_VALUE,               MODE_PIN_MUX_NULL},
        {19, MODE_UART,         MODE_PIN_MUX_NULL,                 MODE_UART1_CTSN_REG_VALUE},
        {19, MODE_PWM4,         MODE_PWM4_REG_VALUE,               MODE_PIN_MUX_NULL},
    },
    //GPIO20
    {
        {20, MODE_GPIO,         MODE_GPIO_REG_VALUE,               MODE_PIN_MUX_NULL},
        {20, MODE_PWM0,         MODE_PWM0_REG_VALUE,               MODE_PIN_MUX_NULL},
        {20, MODE_PWM1,         MODE_PWM1_REG_VALUE,               MODE_PIN_MUX_NULL},
        {20, MODE_PWM2,         MODE_PWM2_REG_VALUE,               MODE_PIN_MUX_NULL},
        {20, MODE_I2C,          MODE_I2CM0_SCL_OUT_REG_VALUE,      MODE_I2CM0_SCL_REG_VALUE},
        {20, MODE_PWM3,         MODE_PWM3_REG_VALUE,               MODE_PIN_MUX_NULL},
        {20, MODE_UART,         MODE_UART1_RTSN_REG_VALUE,         MODE_PIN_MUX_NULL},
        {20, MODE_PWM4,         MODE_PWM4_REG_VALUE,               MODE_PIN_MUX_NULL},
    },
    //GPIO21
    {
        {21, MODE_GPIO,         MODE_GPIO_REG_VALUE,               MODE_PIN_MUX_NULL},
        {21, MODE_PWM0,         MODE_PWM0_REG_VALUE,               MODE_PIN_MUX_NULL},
        {21, MODE_PWM1,         MODE_PWM1_REG_VALUE,               MODE_PIN_MUX_NULL},
        {21, MODE_PWM2,         MODE_PWM2_REG_VALUE,               MODE_PIN_MUX_NULL},
        {21, MODE_I2C,          MODE_I2CM0_SDA_OUT_REG_VALUE,      MODE_I2CM0_SDA_REG_VALUE},
        {21, MODE_PWM3,         MODE_PWM3_REG_VALUE,               MODE_PIN_MUX_NULL},
        {21, MODE_UART,         MODE_PIN_MUX_NULL,                 MODE_UART1_CTSN_REG_VALUE},
        {21, MODE_PWM4,         MODE_PWM4_REG_VALUE,               MODE_PIN_MUX_NULL},
    },
    //GPIO22
    {
        {22, MODE_GPIO,         MODE_GPIO_REG_VALUE,               MODE_PIN_MUX_NULL},
        {22, MODE_PWM0,         MODE_PWM0_REG_VALUE,               MODE_PIN_MUX_NULL},
        {22, MODE_PWM1,         MODE_PWM1_REG_VALUE,               MODE_PIN_MUX_NULL},
        {22, MODE_PWM2,         MODE_PWM2_REG_VALUE,               MODE_PIN_MUX_NULL},
        {22, MODE_I2C,          MODE_I2CM0_SCL_OUT_REG_VALUE,      MODE_I2CM0_SCL_REG_VALUE},
        {22, MODE_PWM3,         MODE_PWM3_REG_VALUE,               MODE_PIN_MUX_NULL},
        {22, MODE_PIN_MUX_NULL, MODE_PIN_MUX_NULL,                 MODE_PIN_MUX_NULL},
        {22, MODE_PWM4,         MODE_PWM4_REG_VALUE,               MODE_PIN_MUX_NULL},
    },
    //GPIO23
    {
        {23, MODE_GPIO,         MODE_GPIO_REG_VALUE,               MODE_PIN_MUX_NULL},
        {23, MODE_PWM0,         MODE_PWM0_REG_VALUE,               MODE_PIN_MUX_NULL},
        {23, MODE_PWM1,         MODE_PWM1_REG_VALUE,               MODE_PIN_MUX_NULL},
        {23, MODE_PWM2,         MODE_PWM2_REG_VALUE,               MODE_PIN_MUX_NULL},
        {23, MODE_I2C,          MODE_I2CM0_SDA_OUT_REG_VALUE,      MODE_I2CM0_SDA_REG_VALUE},
        {23, MODE_PWM3,         MODE_PWM3_REG_VALUE,               MODE_PIN_MUX_NULL},
        {23, MODE_PIN_MUX_NULL, MODE_PIN_MUX_NULL,                 MODE_PIN_MUX_NULL},
        {23, MODE_PWM4,         MODE_PWM4_REG_VALUE,               MODE_PIN_MUX_NULL},
    },
    //GPIO24
    {
        {24, MODE_GPIO,         MODE_GPIO_REG_VALUE,               MODE_PIN_MUX_NULL},
        {24, MODE_QSPI0,        MODE_SPI0_SCLK_OUT_REG_VALUE,      MODE_QSPI0_SCLK_REG_VALUE},
        {24, MODE_QSPI0_CSN1,   MODE_SPI0_CSN1_REG_VALUE,          MODE_PIN_MUX_NULL},
        {24, MODE_QSPI0_CSN2,   MODE_SPI0_CSN2_REG_VALUE,          MODE_PIN_MUX_NULL},
        {24, MODE_PIN_MUX_NULL, MODE_PIN_MUX_NULL,                 MODE_PIN_MUX_NULL},
        {24, MODE_QSPI1,        MODE_SPI1_SCLK_OUT_REG_VALUE,      MODE_QSPI1_SCLK_REG_VALUE},
        {24, MODE_QSPI0_CSN3,   MODE_SPI0_CSN3_REG_VALUE,          MODE_PIN_MUX_NULL},
        {24, MODE_PIN_MUX_NULL, MODE_PIN_MUX_NULL,                 MODE_PIN_MUX_NULL},
    },
    //GPIO25
    {
        {25, MODE_GPIO,         MODE_GPIO_REG_VALUE,               MODE_PIN_MUX_NULL},
        {25, MODE_QSPI0,        MODE_SPI0_CSN0_REG_VALUE,          MODE_QSPI0_CSN_REG_VALUE},
        {25, MODE_QSPI0_CSN1,   MODE_SPI0_CSN1_REG_VALUE,          MODE_PIN_MUX_NULL},
        {25, MODE_QSPI0_CSN2,   MODE_SPI0_CSN2_REG_VALUE,          MODE_PIN_MUX_NULL},
        {25, MODE_PIN_MUX_NULL, MODE_PIN_MUX_NULL,                 MODE_PIN_MUX_NULL},
        {25, MODE_QSPI1,        MODE_SPI1_CSN0_REG_VALUE,          MODE_QSPI1_CSN_REG_VALUE},
        {25, MODE_QSPI0_CSN3,   MODE_SPI0_CSN3_REG_VALUE,          MODE_PIN_MUX_NULL},
        {25, MODE_PIN_MUX_NULL, MODE_PIN_MUX_NULL,                 MODE_PIN_MUX_NULL},
    },
    //GPIO26
    {
        {26, MODE_GPIO,         MODE_GPIO_REG_VALUE,               MODE_PIN_MUX_NULL},
        {26, MODE_QSPI0,        MODE_SPI0_SDATA_OUT0_REG_VALUE,    MODE_QSPI0_SDATA0_REG_VALUE},
        {26, MODE_QSPI0_CSN1,   MODE_SPI0_CSN1_REG_VALUE,          MODE_PIN_MUX_NULL},
        {26, MODE_QSPI0_CSN2,   MODE_SPI0_CSN2_REG_VALUE,          MODE_PIN_MUX_NULL},
        {26, MODE_PIN_MUX_NULL, MODE_PIN_MUX_NULL,                 MODE_PIN_MUX_NULL},
        {26, MODE_QSPI1,        MODE_SPI1_SDATA_OUT0_REG_VALUE,    MODE_QSPI1_SDATA0_REG_VALUE},
        {26, MODE_QSPI0_CSN3,   MODE_SPI0_CSN3_REG_VALUE,          MODE_PIN_MUX_NULL},
        {26, MODE_PIN_MUX_NULL, MODE_PIN_MUX_NULL,                 MODE_PIN_MUX_NULL},
    },
    //GPIO27
    {
        {27, MODE_GPIO,         MODE_GPIO_REG_VALUE,               MODE_PIN_MUX_NULL},
        {27, MODE_QSPI0,        MODE_SPI0_SDATA_OUT1_REG_VALUE,    MODE_QSPI0_SDATA1_REG_VALUE},
        {27, MODE_QSPI0_CSN1,   MODE_SPI0_CSN1_REG_VALUE,          MODE_PIN_MUX_NULL},
        {27, MODE_QSPI0_CSN2,   MODE_SPI0_CSN2_REG_VALUE,          MODE_PIN_MUX_NULL},
        {27, MODE_PIN_MUX_NULL, MODE_PIN_MUX_NULL,                 MODE_PIN_MUX_NULL},
        {27, MODE_QSPI1,        MODE_SPI1_SDATA_OUT1_REG_VALUE,    MODE_QSPI1_SDATA1_REG_VALUE},
        {27, MODE_QSPI0_CSN3,   MODE_SPI0_CSN3_REG_VALUE,          MODE_PIN_MUX_NULL},
        {27, MODE_PIN_MUX_NULL, MODE_PIN_MUX_NULL,                 MODE_PIN_MUX_NULL},
    },
    //GPIO28
    {
        {28, MODE_GPIO,         MODE_GPIO_REG_VALUE,               MODE_PIN_MUX_NULL},
        {28, MODE_QSPI0,        MODE_SPI0_SCLK_OUT_REG_VALUE,      MODE_QSPI0_SCLK_REG_VALUE},
        {28, MODE_PIN_MUX_NULL, MODE_PIN_MUX_NULL,                 MODE_PIN_MUX_NULL},
        {28, MODE_PIN_MUX_NULL, MODE_PIN_MUX_NULL,                 MODE_PIN_MUX_NULL},
        {28, MODE_PIN_MUX_NULL, MODE_PIN_MUX_NULL,                 MODE_PIN_MUX_NULL},
        {28, MODE_QSPI1,        MODE_SPI1_SCLK_OUT_REG_VALUE,      MODE_QSPI1_SCLK_REG_VALUE},
        {28, MODE_UART,         MODE_UART1_TX_REG_VALUE,           MODE_PIN_MUX_NULL},
        {28, MODE_PIN_MUX_NULL, MODE_PIN_MUX_NULL,                 MODE_PIN_MUX_NULL},
    },
    //GPIO29
    {
        {29, MODE_GPIO,         MODE_GPIO_REG_VALUE,               MODE_PIN_MUX_NULL},
        {29, MODE_QSPI0,        MODE_SPI0_CSN0_REG_VALUE,          MODE_QSPI0_CSN_REG_VALUE},
        {29, MODE_PIN_MUX_NULL, MODE_PIN_MUX_NULL,                 MODE_PIN_MUX_NULL},
        {29, MODE_PIN_MUX_NULL, MODE_PIN_MUX_NULL,                 MODE_PIN_MUX_NULL},
        {29, MODE_PIN_MUX_NULL, MODE_PIN_MUX_NULL,                 MODE_PIN_MUX_NULL},
        {29, MODE_QSPI1,        MODE_SPI1_CSN0_REG_VALUE,          MODE_QSPI1_CSN_REG_VALUE},
        {29, MODE_UART,         MODE_PIN_MUX_NULL,                 MODE_UART1_RX_REG_VALUE},
        {29, MODE_PIN_MUX_NULL, MODE_PIN_MUX_NULL,                 MODE_PIN_MUX_NULL},
    },
    //GPIO30
    {
        {30, MODE_GPIO,         MODE_GPIO_REG_VALUE,               MODE_PIN_MUX_NULL},
        {30, MODE_QSPI0,        MODE_SPI0_SDATA_OUT0_REG_VALUE,    MODE_QSPI0_SDATA0_REG_VALUE},
        {30, MODE_PIN_MUX_NULL, MODE_PIN_MUX_NULL,                 MODE_PIN_MUX_NULL},
        {30, MODE_PIN_MUX_NULL, MODE_PIN_MUX_NULL,                 MODE_PIN_MUX_NULL},
        {30, MODE_PIN_MUX_NULL, MODE_PIN_MUX_NULL,                 MODE_PIN_MUX_NULL},
        {30, MODE_QSPI1,        MODE_SPI1_SDATA_OUT0_REG_VALUE,    MODE_QSPI1_SDATA0_REG_VALUE},
        {30, MODE_UART,         MODE_UART2_TX_REG_VALUE,           MODE_PIN_MUX_NULL},
        {30, MODE_PIN_MUX_NULL, MODE_PIN_MUX_NULL,                 MODE_PIN_MUX_NULL},
    },
    //GPIO31
    {
        {31, MODE_GPIO,         MODE_GPIO_REG_VALUE,               MODE_PIN_MUX_NULL},
        {31, MODE_QSPI0,        MODE_SPI0_SDATA_OUT1_REG_VALUE,    MODE_QSPI0_SDATA1_REG_VALUE},
        {31, MODE_PIN_MUX_NULL, MODE_PIN_MUX_NULL,                 MODE_PIN_MUX_NULL},
        {31, MODE_PIN_MUX_NULL, MODE_PIN_MUX_NULL,                 MODE_PIN_MUX_NULL},
        {31, MODE_PIN_MUX_NULL, MODE_PIN_MUX_NULL,                 MODE_PIN_MUX_NULL},
        {31, MODE_QSPI1,        MODE_SPI1_SDATA_OUT1_REG_VALUE,    MODE_QSPI1_SDATA1_REG_VALUE},
        {31, MODE_UART,         MODE_PIN_MUX_NULL,                 MODE_UART2_RX_REG_VALUE},
        {31, MODE_PIN_MUX_NULL, MODE_PIN_MUX_NULL,                 MODE_PIN_MUX_NULL},
    },
};

#ifdef __cplusplus
}
#endif

#endif      /* end of _PIN_MUX_DEFINE_H_ */
