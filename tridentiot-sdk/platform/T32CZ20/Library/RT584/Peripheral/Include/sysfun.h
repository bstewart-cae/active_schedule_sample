/**
 ******************************************************************************
 * @file    sysfun.h
 * @author
 * @brief   system function implement driver header file
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



#ifndef ___SYSFUN_H__
#define ___SYSFUN_H__

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
#define critical_section_init()     Critical_Section_Init()
#define enter_critical_section()    Enter_Critical_Section()
#define leave_critical_section()    Leave_Critical_Section()
#define version_check()             Version_Check()
#define GetPmuMode()                Sys_Pmu_GetMode()
#define SystemPmuSetMode(v)         Sys_Pmu_SetMode(v)
#define OTP_IC_VERSION_OFFSET       0x00
#define OTP_FT_VERSION_OFFSET       0x04

#ifndef SET_PMU_MODE
#define SET_PMU_MODE                PMU_MODE_DCDC
#endif


/**************************************************************************************************
 *    TYPEDEFS
 *************************************************************************************************/

typedef enum
{
    CHIP_TYPE_581 = 0x01,            /*!<ic type 581 */
    CHIP_TYPE_582 = 0x02,            /*!<ic type 582 */
    CHIP_TYPE_583 = 0x03,            /*!<ic type 583 */
    CHIP_TYPE_584 = 0x04,            /*!<ic type 584 */
    CHIP_TYPE_UNKNOW = 0xFF,
} chip_type_t;


typedef enum
{
    CHIP_VERSION_SHUTTLE = 0x00,                     /*!<ic type SHUTTLE */
    CHIP_VERSION_MPA = 0x01,                         /*!<ic type MPA */
    CHIP_VERSION_MPB = 0x02,                   /*!<ic type MPB */
    CHIP_VERSION_UNKNOW = 0xFF,
} chip_version_t;

typedef struct __attribute__((packed))
{
    chip_type_t     type;
    chip_version_t  version;
}
chip_model_t;



typedef struct __attribute__((packed))
{
    uint8_t  buf[8];

}
otp_version_t;
/**
 * @brief  system pmu mode
 */
typedef enum
{
    PMU_MODE_LDO = 0,               /*!< System PMU LDO Mode */
    PMU_MODE_DCDC,                  /*!< System PMU DCDC Mode */
} pmu_mode_cfg_t;
/**
 * @brief system slow clock mode
 */
typedef enum
{
    RCO20K_MODE = 0,               /*!< System slow clock 20k Mode */
    RCO32K_MODE,                   /*!< System slow clock 32k Mode */
    RCO16K_MODE,                   /*!< System slow clock 32k Mode */
    RCO1M_MODE,                    /*!< System slow clock 32k Mode */
    RCO_MODE,
} slow_clock_mode_cfg_t;

/**
 * @brief Irq priority definitions.
 */
typedef enum
{
    TX_POWER_20DBM_DEF = 0x7D,             /*!< System TX Power 20 DBM Default */
    TX_POWER_14DBM_DEF = 0x3E,             /*!< System TX Power 14 DBM Default */
    TX_POWER_0DBM_DEF = 0x3D,             /*!< System TX Power 0 DBM Default */
} txpower_default_cfg_t;

/**
 * @brief Irq priority definitions.
 */
typedef enum
{
    IRQ_PRIORITY_HIGHEST = 0,
    IRQ_PRIORITY_HIGH = 1,
    IRQ_PRIORITY_NORMAL = 3,
    IRQ_PRIORITY_LOW = 5,
    IRQ_PRIORITY_LOWEST = 7,
} irq_priority_t;

/** @addtogroup Peripheral_Driver RT584Z Periperhal Driver Function
  @{
*/



/** @addtogroup SYSFUN_DRIVER SYSFUN Driver Functions
  @{
*/


/**************************************************************************************************
 *    GLOBAL PROTOTYPES
 *************************************************************************************************/
/**
* @brief   enter critical sections
* @details This function is nest function, that is, system call this function several times.
*           This function will mask all interrupt , except non-mask interrupt.
*           So as short as possible for using this function.
*
*/
void Enter_Critical_Section(void);

/**
 * @brief   leave critical sections
 * @details Because enter critical sections is nest allowed.
 *           So it will only unmask all inerrupt when the number "enter_critical_section"
 *           equals "leave_critical_section" times.
 *           Please be careful design your code when calling enter_critical_section/leave_critical_section.
 *           One Enter_Critical_Section must have one corresponding leave_critical_section!
 *
 */
void Leave_Critical_Section(void);

/**
 * @brief   check hardware chip version and software defined version compared value.
 * @details
 *           version_check is help function to check
 *           software project setting is the same as hardware IC version.
 *           If software project define "CHIP_VERSION" is
 *           not matched with hardware IC version, this functio will return 0, otherwise 1.
 * @return
 * @retval    0 --- hardware and system defined matched.
 * @retval    1 --- hardware and system defined mis-matched.
 */
uint32_t Version_Check(void);

/**
* @brief   Set the system PMU mode
* @param[in] pmu_mode Specifies the system PMU mode
*   This parameter can be the following values:
*     @arg PMU_MODE_LDO: Specifies the system PMU LDO mode
*     @arg PMU_MODE_DCDC: Specifies the system PMU DCDC mode
* @return None
*/
void Sys_Pmu_SetMode(pmu_mode_cfg_t pmu_mode);

/**
 * @brief   Get the system PMU mode
 * @details
 *            return the system config pmu  mode
 * @return
 * @retval    0 --- LDO MODE
 * @retval    1 --- DCDC Mode
 */
pmu_mode_cfg_t Sys_Pmu_GetMode(void);

/**
 * @brief   Get the system slow clock mode
 * @details
 *            return the system slow clock mode
 * @return
 * @retval    0 --- RCO20K MODE
 * @retval    1 --- RCO32K MODE
 * @retval    2 --- RCO16K MODE
 * @retval    3 --- RCO1M MODE
 */
slow_clock_mode_cfg_t Sys_Slow_Clk_Mode(void);

/**
 * @brief   Get the TX power Default
 * @details
 *            return the system Tx power Default
 * @return
 * @retval    0 --- 20 dbm Default
 * @retval    1 --- 14 dbm Default
 * @retval    2 --- 0 dbm Default
 */
txpower_default_cfg_t Sys_TXPower_GetDefault(void);

/**
 * @brief   Set the TX power Default
 * @param[in] txpwrdefault Specifies the sub system tx power level
 *     @arg TX_POWER_20DBM_DEF: Specifies tx power 20dbm
 *     @arg TX_POWER_14DBM_DEF: Specifies tx power 14dbm
 *     @arg TX_POWER_0DBM_DEF: Specifies tx power 0dbm
 * @return NONE
 */

void Set_Sys_TXPower_Default(txpower_default_cfg_t txpwrdefault);
/**
 * @brief   System reset
 * @details Reset the system software by using the watchdog timer to reset the chip.
 */
void Sys_Software_Reset(void);


/**@}*/ /* end of SYSFUN_DRIVER group */

/**@}*/ /* end of PERIPHERAL_DRIVER group */

#ifdef __cplusplus
}
#endif

#endif      /* end of ___SYSFUN_H__ */

