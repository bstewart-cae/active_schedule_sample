/**
 * @file
 * Defines weak functions for overriding statically configured data with runtime
 * data.
 * 
 * SPDX-License-Identifier: BSD-3-Clause
 * SPDX-FileCopyrightText: 2023 Z-Wave-Alliance <https://z-wavealliance.org/>
 */

#ifndef _CC_MANUFACTURERSPECIFIC_H_
#define _CC_MANUFACTURERSPECIFIC_H_

#include <stdint.h>

/**
 * Command class manufacturer specific device Id type
 */
typedef enum
{
  DEVICE_ID_TYPE_OEM = 0,
  DEVICE_ID_TYPE_SERIAL_NUMBER,
  DEVICE_ID_TYPE_PSEUDO_RANDOM,
  NUMBER_OF_DEVICE_ID_TYPES
}
device_id_type_t;

/**
 * Command class manufacturer specific device Id format
 */
typedef enum
{
  DEVICE_ID_FORMAT_UTF_8 = 0,
  DEVICE_ID_FORMAT_BINARY,
  NUMBER_OF_DEVICE_ID_FORMATS
}
device_id_format_t;

/**
 * Writes the manufacturer ID, product type ID and product ID into the given variables.
 *
 * This function is weakly defined within the command class and can be defined by the
 * application if desired.
 * 
 * @param[out] pManufacturerID Pointer to manufacturer ID.
 * @param[out] pProductTypeID Pointer to product type ID.
 * @param[out] pProductID Pointer to product ID.
 */
void CC_ManufacturerSpecific_ManufacturerSpecificGet_handler(
  uint16_t * pManufacturerID,
  uint16_t * pProductTypeID,
  uint16_t * pProductID);

/**
 * Writes the device ID and related values into the given variables.
 *
 * This function is weakly defined within the command class and can be defined by the
 * application if desired.
 * 
 * @param[in,out] pDeviceIDType Pointer to device ID type.
 * @param[out]    pDeviceIDDataFormat Pointer to device ID data format.
 * @param[out]    pDeviceIDDataLength Pointer to device ID data length.
 * @param[out]    pDeviceIDData Pointer to device ID data.
 */
void CC_ManufacturerSpecific_DeviceSpecificGet_handler(
  device_id_type_t * pDeviceIDType,
  device_id_format_t * pDeviceIDDataFormat,
  uint8_t * pDeviceIDDataLength,
  uint8_t * pDeviceIDData);

#endif /* _CC_MANUFACTURERSPECIFIC_H_ */
