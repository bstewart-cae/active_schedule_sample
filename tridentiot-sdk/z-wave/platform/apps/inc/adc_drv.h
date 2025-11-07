/// ***************************************************************************
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2024 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************

/**
 * @file adc_drv.h
 * @brief Platform abstraction for Z-Wave Applications
 */
#ifndef ADC_DRV_H_
#define ADC_DRV_H_

#include <stdint.h>

/**
 * Initialize ADC HW
 *
 */
void adc_init(void);

/**
 * Enable/Disable the ADC
 *
 * @param[in] state Enables the ADC if set to true and disables the ADC if set to false.
 */
void adc_enable(uint8_t state);

/**
 * Read the voltage from the battry monitor
 *
 * @param[out] pVoltage Address of variable where the read voltage will be written.
 */
void adc_get_voltage(uint32_t *pVoltage);

/**
 * Read the temprature from the built-in sesnor
 *
 * @param[out] pTemp Address of variable where the read temperature will be written.
 */
void adc_get_temp(int32_t *pTemp);

#endif /* ADC_DRV_H_ */
