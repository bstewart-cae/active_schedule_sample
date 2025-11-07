/// ****************************************************************************
/// @file zpal_otw_hw_config.h
///
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************

#ifndef ZPAL_OTW_HW_CONFIG_H_
#define ZPAL_OTW_HW_CONFIG_H_
/**
 * @addtogroup zpal
 * @brief
 * Zwave Platform Abstraction Layer.
 * @{
 * @addtogroup zpal_otw_hw_config
 * @brief
 * Define an interface to a weak function used to provide the otw process with
 * the UART configuration used by the application.
 * @{
 */

#include <stdint.h>
#include <zpal_uart.h>
#include <zpal_uart_gpio.h>

/**
 * typedef for OTW uart configuration
 */
typedef struct
{
  zpal_uart_id_t uart_id;                 ///< UART id
  uint32_t baud_rate;                     ///< UART baud rate in bits
  uint8_t data_bits;                      ///< UART data bits number
  zpal_uart_parity_bit_t parity_bit;      ///< UART parit bit configuration
  zpal_uart_stop_bits_t stop_bits;        ///< UART stop bits number
} zpal_otw_uart_config_t;

/**
 * @brief Get the application UART configuration
 * Retreive the UART configuration used by the application
 *
 * Note: this is a weak function. If not defined then the OTW process
 *   will assume application is usibng UART0 running at 115200 buad,8 data bits,
 *   no parity, 1 start, and 1 stop bit. Tx on gpio17 and rx on gpio16.
 *
 * @param[out]  uart_config Pointer UART configuration used by the application.
 * @param[out]  uart_gpio_config  Pointer UART gpios used by the application.
 *
 */
void zpal_otw_hw_config(zpal_otw_uart_config_t  *uart_config,
                        zpal_uart_config_ext_t *uart_gpio_config);
/**
 * @}
 * @}
 */
#endif /* ZPAL_OTW_HW_CONFIG_H_ */
