// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file ZW_keystore.h
 * @copyright 2022 Silicon Laboratories Inc.
 *
 * @brief Key handling in keystore nvm.
 */
#ifndef _ZW_KEYSTORE_H_
#define _ZW_KEYSTORE_H_

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include <stdint.h>
#include "SyncEvent.h"

/****************************************************************************/
/*                              DEFINES                                     */
/****************************************************************************/
#define SCRAMBLING_KEY_IDX 0x3FFC

#define ZW_S2_PUBLIC_KEY_SIZE   (32)
#define ZW_S2_PRIVATE_KEY_SIZE  (32)
#define ZW_S2_PUBLIC_DSK_SIZE   (16)
#define ZW_S0_NETWORK_KEY_SIZE  (16)
#define ZW_S2_NETWORK_KEY_SIZE  ZW_S0_NETWORK_KEY_SIZE

#define S2_NUM_KEY_CLASSES 4 /* Includes the S0 key */

/* Macro for converting the KEY to the NVM index.
 * (KEY >> 1) ensures that key 0x01, 0x02, and 0x04 are indexed as 0, 1, 2.
 * (KEY - 1) >> 5 ensures 0x80 are converted to index 3. 0x03 cleares high bits for key 0x80. */
#define TO_NVM_INDEX(KEY)  (((KEY >> 1) | ((KEY - 1) >> 5)) & 0x03)

/*  Macro for evaluating if the key is valid.
 *  Only a single bit is allowed to identify a key, which is ensured by (KEY & (KEY - 1)).
 *  Bit 0, 1, 2, 7 are only bits used for keys, and is ensured by 0x87. */
#define IS_KEY_VALID(KEY) (!(KEY & (KEY - 1)) && (KEY & 0x87))

/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/

extern SSyncEventArg1 g_KeystoreSecurityKeysChanged;  // Callback activated on change to SecurityKeys


// TODO: Following enum is copied from SDK670 ZW_nvm_rearrange.h and may not be needed for 700 series.
// TODO: but definitely this enum and/or API using it needs a refactoring
typedef enum _NVM_LAYOUT_
{
  NVM_LAYOUT_UNKNOWN    = 0, /** Layout of nvm has not/cannot be determined. */
  NVM_LAYOUT_VALID      = 1, /** Layout of nvm is valid for the current firmware. */
  NVM_LAYOUT_REARRANGED = 2  /** Layout of nvm has been rearra to match the firmware. */
} NVM_LAYOUT;

typedef struct Ss2_keys
{
  // Consider making network keys a struct as well, containing a number of keys, each an array
  // instead of two dimensional array. Makes addressing code more readable.
  uint8_t s2_network_keys[S2_NUM_KEY_CLASSES][ZW_S2_NETWORK_KEY_SIZE];
} Ss2_keys;

typedef struct Ss2_keyclassesAssigned
{
  uint8_t keyclasses_assigned;
} Ss2_keyclassesAssigned;

/*===========================   ZW_KeystoreInit   =========================
**
**    Initialize the keystore
**
**--------------------------------------------------------------------------*/
void ZW_KeystoreInit(void);

/**
 * Fetches DSK part of public Curve25519 key from NVM and copies it to buf
 * \param[out] buf Public DSK
 */
void keystore_public_dsk_read(uint8_t *buf);

/**
 * Generates the dynamic Curve25519 key-pair into cached RAM
 *
 * The dynamic Curve25519 key-pair is used for no grant and unauthenticated key exchanges, where the
 * full public key (without blanking the initial 16 bits) is sent over the radio,
 */
void keystore_dynamic_keypair_generate(void);

/**
 * Returns a bitmask of security keys the application can request
 * ZW_SendDataEX() to use. When the node is excluded, no security keys
 * will be reported (0 is returned).
 */
uint8_t keystore_get_cached_security_keys(void);

#if defined(ZW_SLAVE)
/**
 * Function for setting if current secure bootstraping has been initiated through smartstart.
 * 
 * \param[in] smartstartinitiated has secure bootstraping been initiated through smartstart
 */
void keystore_smartstart_initiated(bool smartstartinitiated);
#endif

bool keystore_network_key_clear(uint8_t keyclass);

#endif /* _ZW_KEYSTORE_H_ */
