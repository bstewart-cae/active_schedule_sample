// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file ZW_qrcode.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zpal_misc.h>
#include "ZW_qrcode.h"

#include "ZW_lib_defines.h"
#ifdef ZWAVE_PSA_SECURE_VAULT
#include "psa/ZW_psa.h"
#else
#include <mbedtls/sha1.h>
#endif

// #define DEBUGPRINT
#include "DebugPrint.h"

#define ZWAVE_SHA1_LENGTH 20

#define ZWAVE_QRCODE_SIGNATURE "90"               // Representing character Z signifies zwave device in qr code
#define ZWAVE_QRCODE_PRODTYPE_LEN "10"            // Representing the length of the product type field
#define ZWAVE_QRCODE_PRODUCTID_LEN "20"           // Representing the length of the product id field
#define ZWAVE_QRCODE_SUPPORTED_PROTOCOLS_LEN "03" // Representing the length of the "Supported Protocols" Value
#define ZWAVE_QRCODE_TYPECRITICAL_PRODTYPE "00"
#define ZWAVE_QRCODE_TYPECRITICAL_PRODID "02"
#define ZWAVE_QRCODE_TYPECRITICAL_SUPPORTED_PROTOCOLS "08" // 08 = 0x04 << 1
#define ZWAVE_QRCODE_VERSION "01"                          // Version 1 Smart start enable
#define NO_OF_CHAR_IN_2HEXBYTE 5

/// SHA-1 hash calculated over all digits following Checksum field
/// Source: "Node Provisioning QR Code Format" doc
#define CHECKSUM_INPUT_LEN (offsetof(ZW_qrcode_t, reserved) - offsetof(ZW_qrcode_t, Requested_Keys))

extern char *itoa(int, char *, int); // calling library function Iota

/* convert_hex_decstring:  convert two hex bytes to 5 decimal digits */
static void convert_hex_decstring(char decstr[], uint8_t hexbytes[])
{
  uint8_t csumhexbyte[4];
  uint16_t checksumeqvdec;
  csumhexbyte[0] = (hexbytes[0] % 16);
  csumhexbyte[1] = (hexbytes[0] / 16);
  csumhexbyte[2] = (hexbytes[1] % 16);
  csumhexbyte[3] = (hexbytes[1] / 16);
  checksumeqvdec = (csumhexbyte[3] << 12) + (csumhexbyte[2] << 8) + (csumhexbyte[1] << 4) + csumhexbyte[0];
  itoa(checksumeqvdec, decstr, 10);
}

static void paddedcopy(char deststr[], char srcstr[], size_t maxlen)
{
  unsigned int len, i;
  len = strnlen(srcstr, maxlen);
  memset((void *)deststr, 0x30, NO_OF_CHAR_IN_2HEXBYTE);
  for (i = 0; ((i < len) && (len <= NO_OF_CHAR_IN_2HEXBYTE)); i++) {
    deststr[NO_OF_CHAR_IN_2HEXBYTE - i - 1] = srcstr[len - i - 1];
  }
}

/// Function for composing the qr code for z-wave devices
void compose_qr_code(bool regionLR, const uint8_t public_key[KEY_SIZE], ZW_qrcode_t *qrcode)
{
  uint8_t sha1output[ZWAVE_SHA1_LENGTH] = { 0 };
  char dskindec[6] = { '\0' };
  uint8_t temphexdata[2];
  char tempstr[6] = { '\0' };
  uint8_t i;
  size_t j;
  zpal_product_id_t productId = { 0 };

  zpal_get_product_id(&productId);

  // Make sure that qrcode is always zero-aligned, regardless of the actual length.
  memset((void *)qrcode, '\0', sizeof(ZW_qrcode_t));
  memcpy((void *)(qrcode->Lead_in), ZWAVE_QRCODE_SIGNATURE, 2);
  memcpy((void *)(qrcode->Version), ZWAVE_QRCODE_VERSION, 2);

  // Composing the DSK from the public key first 16 bytes
  for (i = 0; i < 8; i++) {
    temphexdata[0] = public_key[(i * 2) + 1];
    temphexdata[1] = public_key[(i * 2)];
    convert_hex_decstring(dskindec, temphexdata);
    paddedcopy(tempstr, dskindec, sizeof(dskindec));
    memcpy((void *)(&qrcode->DSK[i * 5]), tempstr, NO_OF_CHAR_IN_2HEXBYTE);
    // DPRINTF("the DSK pair %d     %s\n",i+1,dskindec);
  }

  // Get The requested Key from application properties
  temphexdata[0] = productId.requested_security_key;
  temphexdata[1] = 0x00;
  convert_hex_decstring(dskindec, temphexdata);
  j = strnlen(dskindec, sizeof(dskindec));
  /* The value of the requested key could be less than three digits.
   * Make sure the unused leading digits are filled with '0' (instead of '\0')  */
  memset(qrcode->Requested_Keys, '0', 3);
  for (i = 0; i < j; i++) {
    qrcode->Requested_Keys[3 - i - 1] = dskindec[j - i - 1];
  }
  // DPRINTF("the requested key is   %s\n",dskindec);

  // Composing the product type fields from application attributes
  memcpy((void *)(qrcode->TypeCritical_ProductType), ZWAVE_QRCODE_TYPECRITICAL_PRODTYPE, 2);
  memcpy((void *)(qrcode->Len_ProductType), ZWAVE_QRCODE_PRODTYPE_LEN, 2);

  // compose fields device type
  temphexdata[0] = productId.specyfic_type; // Device specific type tested with 0xA5
  temphexdata[1] = productId.generic_type;  // Device generic type  tested with 0xA5;
  convert_hex_decstring(dskindec, temphexdata);
  paddedcopy(tempstr, dskindec, sizeof(dskindec));
  // copying the generic type.specific type in this structure msb generic and lsb is specific type
  memcpy((void *)(&qrcode->ProductType[0]), tempstr, NO_OF_CHAR_IN_2HEXBYTE);
  // DPRINTF("the device type is   %x,%x\n",temphexdata[0],temphexdata[1]);

  // compose fields icon type
  temphexdata[0] = productId.app_icon_type & 0xFF;        // app icon type lsb  tested with 0xFF
  temphexdata[1] = (productId.app_icon_type >> 8) & 0xFF; // app icon type msb  tested with 0xFF
  convert_hex_decstring(dskindec, temphexdata);
  paddedcopy(tempstr, dskindec, sizeof(dskindec));
  // copying the app icon type.app icon type in this structure
  memcpy((void *)(&qrcode->ProductType[5]), tempstr, NO_OF_CHAR_IN_2HEXBYTE);
  // DPRINTF("the icon type is   %x,%x\n",temphexdata[0],temphexdata[1]);

  // Composing the product id fields from application attributes
  memcpy((void *)(qrcode->TypeCritical_ProductID), ZWAVE_QRCODE_TYPECRITICAL_PRODID, 2);
  memcpy((void *)(qrcode->Len_ProductID), ZWAVE_QRCODE_PRODUCTID_LEN, 2);

  // compose various fields of the product id
  // composing the manufactuerer ID
  temphexdata[0] = productId.app_manufacturer_id & 0xFF;        // Manufactuer ID lsb
  temphexdata[1] = (productId.app_manufacturer_id >> 8) & 0xFF; // Manufactuer ID msb
  convert_hex_decstring(dskindec, temphexdata);
  paddedcopy(tempstr, dskindec, sizeof(dskindec));
  memcpy((void *)(&qrcode->ProductID[0]), tempstr, NO_OF_CHAR_IN_2HEXBYTE); // Copying manufacturer ID

  // composing the product type
  temphexdata[0] = productId.app_product_type & 0xFF;        // product type lsb
  temphexdata[1] = (productId.app_product_type >> 8) & 0xFF; // product type msb
  convert_hex_decstring(dskindec, temphexdata);
  paddedcopy(tempstr, dskindec, sizeof(dskindec));
  memcpy((void *)(&qrcode->ProductID[5]), tempstr, NO_OF_CHAR_IN_2HEXBYTE); // Copying product type

  // composing the product id
  temphexdata[0] = productId.app_product_id & 0xFF;        // product id lsb
  temphexdata[1] = (productId.app_product_id >> 8) & 0xFF; // product id msb
  convert_hex_decstring(dskindec, temphexdata);
  paddedcopy(tempstr, dskindec, sizeof(dskindec));
  memcpy((void *)(&qrcode->ProductID[10]), tempstr, NO_OF_CHAR_IN_2HEXBYTE); // Copying product id

  // composing the Application Version,  Application Revision
  temphexdata[0] = (zpal_get_app_version() & 0x0000FF00) >> 8;  // application minor version is lsb
  temphexdata[1] = (zpal_get_app_version() & 0x00FF0000) >> 16; // application major version is msb
  convert_hex_decstring(dskindec, temphexdata);
  paddedcopy(tempstr, dskindec, sizeof(dskindec));
  memcpy((void *)(&qrcode->ProductID[15]), tempstr, NO_OF_CHAR_IN_2HEXBYTE); // Copying product id
  DPRINTF("App version is %x, revison is %x\n", temphexdata[1], temphexdata[0]);

  // compose supported Protocols fields
  memcpy((void *)(qrcode->TypeCritical_SupportedProtocols), ZWAVE_QRCODE_TYPECRITICAL_SUPPORTED_PROTOCOLS, 2);
  memcpy((void *)(qrcode->Len_SupportedProtocols), ZWAVE_QRCODE_SUPPORTED_PROTOCOLS_LEN, 2);

  uint8_t supportedProtocols = 0;
  supportedProtocols |= 1 << SUPPORTEDPROTOCOL_ZWAVE;
  // if LR freq
  if (regionLR) {
    supportedProtocols |= 1 << SUPPORTEDPROTOCOL_ZWAVE_LR;
  }
  // SupportedProtocols Value fits into single (decimal) digit
  // just save it in least significant digit of SupportedProtocols as character.
  memset(qrcode->SupportedProtocols, '0', 2);
  qrcode->SupportedProtocols[2] = supportedProtocols + '0';

  DPRINTF("KEY: SupportedProtocols TLV: [%s, %s, %s]\n",
          qrcode->TypeCritical_SupportedProtocols,
          qrcode->Len_SupportedProtocols,
          qrcode->SupportedProtocols);

  // Finally, calculate the checksum using sha1 algorithm
  const uint8_t *checksum_input = qrcode->Requested_Keys;

#ifdef ZWAVE_PSA_SECURE_VAULT
  size_t hash_length = sizeof(sha1output);
  zw_psa_compute_sha1((checksum_input), CHECKSUM_INPUT_LEN, sha1output, &hash_length);
#else
  mbedtls_sha1((checksum_input), CHECKSUM_INPUT_LEN, sha1output);
#endif

  // convert the sha output to the hex digits
  temphexdata[0] = sha1output[1]; // as sha-1 giving ouput in network byte order
  temphexdata[1] = sha1output[0]; // as sha-1 giving ouput in network byte order
  convert_hex_decstring(dskindec, temphexdata);

  paddedcopy(tempstr, dskindec, sizeof(dskindec));
  memcpy((void *)(qrcode->Checksum), tempstr, NO_OF_CHAR_IN_2HEXBYTE);
}
