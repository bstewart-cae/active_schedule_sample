/// ****************************************************************************
/// @file zpal_uart_private.h
///
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************

#ifndef ZPAL_UART_PRIVATE_H_
#define ZPAL_UART_PRIVATE_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup zpal
 * @brief
 * Zwave Platform Abstraction Layer.
 * @{
 * @addtogroup zpal-uart
 * @brief
 * Platform specific uart API
 * @{
 */

/**
  * @brief  Uninitialize a running UART port.
  * This function powers off the running UART port and prepares it to be configured again with alternative settings.
  *
  * @param[in]  uart_id The ID of the UART port to be uninitialized.
  * @return  ZPAL_STATUS_OK if the UART port is uninitialized successfully; otherwise, returns ZPAL_STATUS_FAIL.
 *
 */
zpal_status_t zpal_uart_uninit(zpal_uart_id_t uart_id);

/**
 * @}
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* ZPAL_UART_PRIVATE_H_ */
