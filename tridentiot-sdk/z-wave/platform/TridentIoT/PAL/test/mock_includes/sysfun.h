/// ****************************************************************************
/// @file cm3_mcu.h
///
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************

#ifndef  _SYSFUN_H_
#define  _SYSFUN_H_

#define enter_critical_section()    Enter_Critical_Section()
#define leave_critical_section()    Leave_Critical_Section()
/*----------------------------------------------------------------------------
  Define PMU Mode
 *----------------------------------------------------------------------------*/
/*Define PMU mode type*/
#define PMU_LDO_MODE     0
#define PMU_DCDC_MODE    1

#ifndef SET_PMU_MODE
#define SET_PMU_MODE    PMU_DCDC_MODE
#endif

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

typedef enum
{
    IRQ_PRIORITY_HIGHEST = 0,
    IRQ_PRIORITY_HIGH = 1,
    IRQ_PRIORITY_NORMAL = 3,
    IRQ_PRIORITY_LOW = 5,
    IRQ_PRIORITY_LOWEST = 7,
} irq_priority_t;

void Enter_Critical_Section(void);
void Leave_Critical_Section(void);

#endif /** _SYSFUN_H_ */