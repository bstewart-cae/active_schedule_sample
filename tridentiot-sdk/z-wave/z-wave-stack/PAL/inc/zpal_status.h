// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
// SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file
 * Defines status codes returned by zpal functions.
 *
 * @copyright 2021 Silicon Laboratories Inc.
 */

#ifndef ZPAL_STATUS_H_
#define ZPAL_STATUS_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @addtogroup zpal
 * @brief
 * Z-Wave Platform Abstraction Layer.
 * @{
 * @addtogroup zpal-status
 * @brief
 * Defines status codes returned by zpal functions.
 *
 * @{
 */

/**
 * @brief ZPAL statuses to be used by ZPAL APIs.
 *
 */
typedef enum {
  ZPAL_STATUS_OK = 0,                               ///< No error.
  ZPAL_STATUS_FAIL,                                 ///< Generic error.
  ZPAL_STATUS_BUFFER_FULL,                          ///< There is no more room in the buffer.
  ZPAL_STATUS_BUSY,                                 ///< Process is busy.
  ZPAL_STATUS_INVALID_ARGUMENT,                     ///< At least one argument is invalid.
  ZPAL_STATUS_BOOTLOADER_INVALID_CHECKSUM,          ///< The firmware image checksum does not match.
  ZPAL_STATUS_BOOTLOADER_UNABLE_TO_DECRYPT,         ///< Unable to decrypt the firmware image.
  ZPAL_STATUS_BOOTLOADER_DOWNGRADE_NOT_SUPPORTED,   ///< Downgrading to previous version not supported.
} zpal_status_t;

/**
 * @} //zpal-status
 * @} //zpal
 */

#ifdef __cplusplus
}
#endif

#endif /* ZPAL_STATUS_H_ */
