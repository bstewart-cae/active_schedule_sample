/// ***************************************************************************
///
/// @file zpal_uart_gpio.h
///
/// @brief Defines a structure used for selecting gpio pins for uart when using zpal_uart_init()
///
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2024 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************

#ifndef ZPAL_UART_GPIO_H_
#define ZPAL_UART_GPIO_H_

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup zpal
 * @brief
 * Z-Wave Platform Abstraction Layer.
 * @{
 * @addtogroup zpal-uart-gpio
 * @brief
 * Defines a structure used for selecting gpio pins for uart when using zpal_uart_init()
 *
 * How to use the structure
 *
 * The zpal_uart_config_ext_t structure is used to specify pin locations on the chip for a UART
 * The structure should be provided to the zpla_uart_init() call in the ptr pointer in the config structure.
 *
 * If the ptr pointer is set to NULL the default pins will be used
 *
 * ZPAL_UART0
 * rx_pin = 16
 * tx_pin = 17
 *
 * ZPAL_UART1
 * rx_pin = 29
 * tx_pin = 28
 *
 * ZPAL_UART2
 * rx_pin = 31
 * tx_pin = 30
 *
 * Enabling hardware flow control is done by setting cts and rts pins to something different than zero
 * @{
 */


/**
 * @brief UART gpio and wakeup configuration.
 */
typedef struct {
  uint8_t txd_pin;        ///< Tx pin location
  uint8_t rxd_pin;        ///< Rx pin location
  uint8_t cts_pin;        ///< CTS pin location
  uint8_t rts_pin;        ///< RTS pin location
  bool uart_wakeup;       ///< Support uart wakeup
} zpal_uart_config_ext_t;

/**
 * @} //zpal-uart-gpio
 * @} //zpal
 */

#ifdef __cplusplus
}
#endif

#endif /* ZPAL_UART_GPIO_H_ */
