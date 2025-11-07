// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file ZW_qrcode.h
 * @copyright 2022 Silicon Laboratories Inc.
 */
#ifndef QR_CODE_H_
#define QR_CODE_H_

#include <stdint.h>
#include "key_generation.h" //for KEY_SIZE

/**
 * Protocols supported by the node
 */
enum _ESupportedProtocol{
  SUPPORTEDPROTOCOL_ZWAVE,   //!< SUPPORTEDPROTOCOL_ZWAVE
  SUPPORTEDPROTOCOL_ZWAVE_LR //!< SUPPORTEDPROTOCOL_ZWAVE_LR
};

// Following structure defines the QR code as stated in design document SDS13937-6
typedef struct _ZW_qrcode_t{
  uint8_t Lead_in[2];  // Must be "90", representing ASCII character 'Z'. Allows for an initial detection of Z-Wave QR codes
  uint8_t Version[2];  // MUST be set to 0 for S2-only devices
  uint8_t Checksum[5]; // This field MUST advertise a 5 digit decimal value representing the first
                       // two bytes of the SHA-1 hash calculated over all digits following this field.
                       // When calculating the SHA-1 value, the digits MUST be treated as ASCII characters
  uint8_t Requested_Keys[3];           // This field carries an 8 bit value as 3 decimal digits.The value MUST reflect the Requested Keys byte as defined  by the S2 specification
  uint8_t DSK[40];                     // S2 DSK. 16 bit decimal block representation
  uint8_t TypeCritical_ProductType[2]; // ProductType, 16 bit decimal representation.
  uint8_t Len_ProductType[2];          // Number of digits in ProductType
  uint8_t ProductType[10];             // QRProductType:Z-Wave Device Type = [Light Dimmer Switch] 0x11.0x01 = 04353
                                       // Z-Wave Installer Icon Type = LIGHT_DIMMER_SWITCH_PLUGIN = 0x0601 = 0153716
                                       // bit decimal block representation
  uint8_t TypeCritical_ProductID[2];
  uint8_t Len_ProductID[2];
  uint8_t ProductID[20];
  uint8_t TypeCritical_SupportedProtocols[2]; // SupportedProtocols Type
  uint8_t Len_SupportedProtocols[2];          // Length of SupportedProtocols field
  // bitmask that represents supported Protocols
  uint8_t SupportedProtocols[3];
  // Note: required to keep struct size aligned
  // with size TOKEN_MFG_ZW_QR_CODE_SIZE,
  // excluded from checksum calculation.
  uint8_t reserved[9];
} ZW_qrcode_t;

void compose_qr_code(bool regionLR, const uint8_t public_key[KEY_SIZE], ZW_qrcode_t *qrcode);

#endif /* QR_CODE_H_ */
