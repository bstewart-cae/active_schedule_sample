/// ****************************************************************************
/// @file zpal_gpio_private.h
///
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************

#ifndef ZPAL_GPIO_PRIVATE_H_
#define ZPAL_GPIO_PRIVATE_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup zpal
 * @brief
 * Zwave Platform Abstraction Layer.
 * @{
 * @addtogroup zpal-gpio
 * @brief
 * Platform specific gpio API
 * @{
 */

/**
 * @brief Store the GPIO interrupt status register value.
 * This function stores a local copy of the GPIO interrupt status register.
 * If bit n (where n is between 0 and 31) is set to 1, it indicates that gpio(n) has triggered a GPIO interrupt.
 *
 * This API is useful for determining which GPIO caused a wakeup during deep sleep mode.
 * @param[in] gpio_status: The value of the GPIO interrupt status register.
 *
 */
void zpal_gpio_status_store(uint32_t gpio_status);


/**
 * @brief Retrieve the value of the GPIO interrupt status register.
 * This API is useful for determining which GPIO triggered a wake-up during deep sleep mode.
 *
 * @return The value of the register.
 */

uint32_t zpal_gpio_status_get(void);

/**
 * @}
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZPAL_GPIO_PRIVATE_H_ */
