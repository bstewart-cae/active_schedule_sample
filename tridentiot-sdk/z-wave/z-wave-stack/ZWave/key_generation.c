// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file key_generation.c
 * @copyright 2022 Silicon Laboratories Inc.
 *
 * @brief Generate the Manufacturing Token system
 */
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "key_generation.h"
#include "ZW_qrcode.h"
#include "zpal_entropy.h"
#include <MfgTokens.h>
#include "ZW_lib_defines.h"
#if defined(ZW_SLAVE)
#include <ZW_slave_network_info_storage.h>
#endif /* defined(ZW_SLAVE) */

//#define DEBUGPRINT
#include "DebugPrint.h"

#if !defined(ZWAVE_PSA_SECURE_VAULT) || (defined(ZWAVE_PSA_SECURE_VAULT) && defined(ZW_CONTROLLER))
#define USE_LIBS2_CRYPTO 1
#else
#define USE_LIBS2_CRYPTO 0
#endif

extern void crypto_scalarmult_curve25519_base(
    uint8_t *q,
    const uint8_t *n
);

uint8_t zwave_secret_key[KEY_SIZE];

uint8_t zwave_public_key[KEY_SIZE];

#if USE_LIBS2_CRYPTO
/**@brief Function for composing the private key from a random number generator
 *
 * @param[in]     None
 * @param[in,out] None
 * @return        0 on success 1 on failure
 */
static uint32_t create_zwave_private_key(void)
{
  //call pseduo random number generator to create the private key
  zpal_get_random_data(zwave_secret_key,KEY_SIZE);
  return 0;
}

/**@brief Function for composing the public key from the private key
 *
 * @param[in]     None
 * @param[in,out] None
 * @return        0 on success 1 on failure
 */
static uint32_t create_zwave_public_key(void)
{

  //Generate public key from private key
  crypto_scalarmult_curve25519_base(zwave_public_key, zwave_secret_key);

  return 0;
}

#endif

void check_create_keys_qrcode(bool regionLR)
{
  uint8_t mfgsetdata = 0x00;
  uint8_t mfggetdata = 0xFF;

  ZW_GetMfgTokenData((void*)&mfggetdata,TOKEN_MFG_ZW_INITIALIZED_ID,TOKEN_MFG_ZW_INITIALIZED_SIZE);
#ifdef ZW_SLAVE
  bool qr_code_ok = false;
  uint8_t qrcode[TOKEN_MFG_ZW_QR_CODE_SIZE];
  //read the QR code from the flash
  ZW_GetMfgTokenData((void*)&qrcode, TOKEN_MFG_ZW_QR_CODE_ID, TOKEN_MFG_ZW_QR_CODE_SIZE);

  for(uint8_t i = 0 ; i < TOKEN_MFG_ZW_QR_CODE_SIZE; i++ )
  {
    if (qrcode[i] != 0xFF)
    {
      qr_code_ok = true;
      break;
    }
  }
  // if the qr code already written then skip generating new qr code and keys
  if(qr_code_ok)
#else
  if (mfggetdata != 0xFF)
#endif
  {
    ZW_LockMfgTokenData();
    DPRINT("manufacturing data is already initialized \n");
    return;
  }
  DPRINT("manufacturing data is uninitialized, first time\n");

// Currently libs2 is being used by 700 series and 800 series controllers
#if USE_LIBS2_CRYPTO
  // skip generating new keys and only generate new qr code
  if (0xFF == mfggetdata )
  {
    create_zwave_private_key();
    create_zwave_public_key();
  }
#ifdef ZW_SLAVE
  else
  {
    StorageGetZWPublicKey(zwave_public_key);
  }
#endif
#endif

#ifdef ZW_SLAVE
  compose_qr_code(regionLR, zwave_public_key, (ZW_qrcode_t *)qrcode);
  //Write the QR code to the flash
  ZW_SetMfgTokenData(TOKEN_MFG_ZW_QR_CODE_ID, (void*)&qrcode, TOKEN_MFG_ZW_QR_CODE_SIZE);
#else
  (void)regionLR; // fake UNUSED to avoid warnings.
#endif
  //Now writes all the keys and qr code into the flash
  if (0xFF == mfggetdata )
  {
#if USE_LIBS2_CRYPTO
#ifdef ZW_SLAVE
    StorageSetZWPrivateKey(zwave_secret_key);
    StorageSetZWPublicKey(zwave_public_key);
#else /* defined(ZW_SLAVE) */
    ZW_SetMfgTokenData(TOKEN_MFG_ZW_PRK_ID,(void*)zwave_secret_key,TOKEN_MFG_ZW_PRK_SIZE);
    //Write the public key to the flash
    ZW_SetMfgTokenData(TOKEN_MFG_ZW_PUK_ID,(void*)zwave_public_key,TOKEN_MFG_ZW_PUK_SIZE);
#endif /* defined(ZW_SLAVE) */
#endif

    ZW_SetMfgTokenData(TOKEN_MFG_ZW_INITIALIZED_ID,(void*)&mfgsetdata,TOKEN_MFG_ZW_INITIALIZED_SIZE);
  }
#if defined(ZW_SLAVE) && defined DEBUGPRINT
  DPRINT("Read QR code back for testing\n");
  ZW_qrcode_t qrcode_read;

  ZW_GetMfgTokenData((void *)&qrcode_read, TOKEN_MFG_ZW_QR_CODE_ID, TOKEN_MFG_ZW_QR_CODE_SIZE);
  // print entire QR code
  int i=0;
  for(; i< TOKEN_MFG_ZW_QR_CODE_SIZE; i++)
  {
    DPRINTF("%c", *((char *)&qrcode_read + i));
  }
  DPRINTF("\nSize of QR code  = %d\n", i);

  uint8_t res;
  res = memcmp((void *)&qrcode_read, (void *)&qrcode, TOKEN_MFG_ZW_QR_CODE_SIZE);
  res = res;
  DPRINTF("QR code %s\n", res ? "write fail :(":"successfully written :)");
#endif

  ZW_LockMfgTokenData();
  DPRINT("manufacturing data write is complete\n");
}

