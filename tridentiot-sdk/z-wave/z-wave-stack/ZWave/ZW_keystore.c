// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file ZW_keystore.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include "ZW_lib_defines.h"
#include "Assert.h"
#include <platform.h>
#include <stdint.h>
#include <s2_keystore.h>
#include "ZW_keystore.h"
#include <string.h> // For memcpy functionality

#ifdef ZW_CONTROLLER
#include "ZW_controller_network_info_storage.h"
#endif

#ifdef ZW_SLAVE
#include <ZW_slave_network_info_storage.h>
#endif

#include <curve25519.h>
#include <MfgTokens.h>
#ifdef ZWAVE_PSA_SECURE_VAULT
#include "psa/ZW_psa.h"
#else
#include <zpal_entropy.h>
#endif
//#define DEBUGPRINT
#include "DebugPrint.h"
#include "SizeOf.h"
#include "S2_external.h"

SSyncEventArg1 g_KeystoreSecurityKeysChanged =    { .uFunctor.pFunction = 0 };  /* Callback activated on change to m_assignedSec_keys */


#if defined(ZW_SLAVE)

#ifdef ZWAVE_PSA_SECURE_VAULT
typedef struct zwave_s2_ecc_keypair_t s2_keypair_t;
#else
typedef struct s2_keypair_t
{
  uint8_t zwave_s2_ecc_private_key[ZW_S2_PRIVATE_KEY_SIZE];
  uint8_t zwave_s2_ecc_public_key[ZW_S2_PUBLIC_KEY_SIZE];
} s2_keypair_t;
#endif

/* Current no grant/S0/unauthenticated keypair only cached in RAM */
static s2_keypair_t m_s2_dynamic_keypair = { 0 };

/**
 * Flag indicating if current S2 bootstraping has been initiated through Smart Start.
 * if true use persistent Curve25519 keypair regardless if dynamic Curve25519 keypair is requested.
 * if false use dynamic Curve25519 keypair if dynamic Curve25519 keypair is requested.
 */
static bool m_smartstart_initiated = false;

/* Current cached "asssigned security keys" */
static uint8_t m_assignedSec_keys;

static void UpdateCachedSecurityKeys(uint8_t AssignedSec_keys);

/**
 * Function for setting if current secure bootstraping has been initiated through smartstart.
 *
 * \param[in] smartstartinitiated has secure bootstraping been initiated through smartstart
 */
void keystore_smartstart_initiated(bool smartstartinitiated)
{
  m_smartstart_initiated = smartstartinitiated;
}
#endif // #if defined(ZW_SLAVE)

/**
* Fetches DSK part of public Curve25519 key from persistent storage and copies it to buf
* \param[out] buf Public DSK
*/
void keystore_public_dsk_read(uint8_t *buf)
{
#ifdef ZW_SLAVE
  uint8_t pubk[TOKEN_MFG_ZW_PUK_SIZE];
  StorageGetZWPublicKey(pubk);
  memcpy(buf, pubk, SECURITY_KEY_S2_PUBLIC_DSK_LENGTH);
#else
  ZW_GetMfgTokenData(buf, TOKEN_MFG_ZW_PUK_ID, SECURITY_KEY_S2_PUBLIC_DSK_LENGTH);
#endif
}

#if defined(ZW_SLAVE)
/**
* Function for fetching public Curve25519 key from persistent storage and copies it to buf.

* \param[out] buf Public key
*/
void keystore_public_key_read(uint8_t *buf)
{
  StorageGetZWPublicKey(buf);
#ifdef ZWAVE_PSA_SECURE_VAULT
  zw_psa_verify_public_key(buf);
#endif
}

/**
 * Fetches public Curve25519 key from cached RAM/persistent storage and copies it to buf.
 * if secure bootstraping has been initiated through smartstart the returned public Curve25519 key will be from the persistent storage keypair.
 * if secure bootstraping has not been initiated through smartstart the returned public Curve25519 key will be from the dynamic keypair.
 *
 * \param[out] buf Public key
 */
void keystore_dynamic_public_key_read(uint8_t *buf)
{
  if (true == m_smartstart_initiated)
  {
    keystore_public_key_read(buf);
  }
  else
  {
    memcpy(buf, m_s2_dynamic_keypair.zwave_s2_ecc_public_key, TOKEN_MFG_ZW_PUK_SIZE);
  }
}

#ifdef ZWAVE_PSA_SECURE_VAULT

void keystore_keyid_read(uint32_t *keyid)
{
  *keyid = (uint32_t)ZWAVE_PSA_ECDH_KEY_ID;
}

void keystore_dynamic_keyid_read(uint32_t *keyid)
{
  if (true == m_smartstart_initiated)
  {
    keystore_keyid_read(keyid);
  }
  else
  {
    *keyid = (uint32_t) m_s2_dynamic_keypair.zwave_ecc_key_handle;
  }
}

#else

/**
* Fetches private Curve25519 key from NVM and copies it to buf
* \param[out] buf Private key
*/
void keystore_private_key_read(uint8_t *buf)
{
#if defined(ZW_SLAVE)
  StorageGetZWPrivateKey(buf);
#else /* defined(ZW_SLAVE) */
  ZW_GetMfgTokenData(buf, TOKEN_MFG_ZW_PRK_ID, TOKEN_MFG_ZW_PRK_SIZE);
#endif /* defined(ZW_SLAVE) */
}

/**
 * Fetches private Curve25519 key from cached RAM/persistent and copies it to buf.
 *
 * if secure bootstraping has been initiated through smartstart the returned public Curve25519 key will be from the persistent storage keypair.
 * if secure bootstraping has not been initiated through smartstart the returned public Curve25519 key will be from the dynamic keypair.
 *
 * \param[out] buf Private key
 */
void keystore_dynamic_private_key_read(uint8_t *buf)
{
  if (true == m_smartstart_initiated)
  {
    keystore_private_key_read(buf);
  }
  else
  {
    memcpy(buf, m_s2_dynamic_keypair.zwave_s2_ecc_private_key, TOKEN_MFG_ZW_PRK_SIZE);
  }
}
#endif

/**
 * Generates the dynamic Curve25519 key-pair into cached RAM
 *
 * The dynamic Curve25519 key-pair is used for unauthenticated key exchanges, where the
 * full public key (without blanking the initial 16 bits) is sent over the radio,
 * in order to protect the primary public key from a MITM attack as described in
 * PSIRT-33 and SWPROT-3265.
 */
void keystore_dynamic_keypair_generate(void)
{
  if (false == m_smartstart_initiated)
  {
#ifdef ZWAVE_PSA_SECURE_VAULT
    zw_psa_gen_dynamic_ecc_keypair(&m_s2_dynamic_keypair);
#else
    zpal_get_random_data(m_s2_dynamic_keypair.zwave_s2_ecc_private_key, sizeof_structmember(s2_keypair_t, zwave_s2_ecc_private_key));
    crypto_scalarmult_curve25519_base(m_s2_dynamic_keypair.zwave_s2_ecc_public_key, m_s2_dynamic_keypair.zwave_s2_ecc_private_key);
#endif
  }
}

/**
* Write key_class security key to NVM keystore.
* \return true if keyclass valid -> key written
*         false if keyclass not supported and nothing written
* \param[in] key class to write to. Key class uses the BITMASK format, but exactly one
*            bit must be set.
* \param[in] keybuf points at key in SRAM which is to be written at key_class position in NVM keystore.
*
* updates m_assignedSec_keys used in ZW_GetsecurityKeys
*/
static bool
keystore_network_key_write_impl(
  uint8_t key_class,
  const uint8_t *keybuf)
{
  if (!IS_KEY_VALID(key_class))
  {
    DPRINTF("ERROR: Unknown keystore write requested. %02X\r\n", key_class);
    return false;
  }

  DPRINTF("Network key write: %02X\r\n", key_class);
#if defined(ZWAVE_PSA_SECURE_VAULT) && defined(ZWAVE_PSA_AES)
  uint32_t net_key_id;
#endif
#if  !(defined(ZWAVE_PSA_SECURE_VAULT) && defined(ZWAVE_PSA_AES) && defined(ZW_CONTROLLER))
  uint8_t key_index = TO_NVM_INDEX(key_class);
  Ss2_keys tSs2keys = { 0 };
  if (false == StorageGetS2Keys(&tSs2keys))
  {
    DPRINT("ERROR: ZW_keystore unable to read S2_Keys from NVM.\r\n");
    return false;
  }
#if defined(ZWAVE_PSA_SECURE_VAULT) && defined(ZWAVE_PSA_AES)
  if (key_class == KEY_CLASS_S0)
  {
    memcpy(tSs2keys.s2_network_keys[key_index], keybuf, ZW_S2_NETWORK_KEY_SIZE);
  }
  else
  {
    /** The S2 network keys are stored in SE (Secure Element) in wrapped form.
     * Do not let key material linger around in memory. Therefore,
     * Write dummy-zero values on end-devices when SE is used.
     **/
    memset(tSs2keys.s2_network_keys[key_index], 0, ZW_S2_NETWORK_KEY_SIZE);
  }
#else
    memcpy(tSs2keys.s2_network_keys[key_index], keybuf, ZW_S2_NETWORK_KEY_SIZE);
#endif

  if (false == StorageSetS2Keys(&tSs2keys))
  {
    DPRINT("ERROR: ZW_keystore unable to write S2_Keys to NVM.\r\n");
    return false;
  }
#endif

  Ss2_keyclassesAssigned tSs2_keyclassesAssigned = { 0 };
  if (false == StorageGetS2KeyClassesAssigned(&tSs2_keyclassesAssigned))
  {
    DPRINT("ERROR: ZW_keystore unable to read S2_KeyClassesAssigned from NVM.\r\n");
    return false;
  }

  UpdateCachedSecurityKeys(key_class | tSs2_keyclassesAssigned.keyclasses_assigned);

  tSs2_keyclassesAssigned.keyclasses_assigned = m_assignedSec_keys;

  if (false == StorageSetS2KeyClassesAssigned(&tSs2_keyclassesAssigned))
  {
    DPRINT("ERROR: ZW_keystore unable to write S2_KeyClassesAssigned to NVM.\r\n");
    return false;
  }

#if defined(ZWAVE_PSA_SECURE_VAULT) && defined(ZWAVE_PSA_AES)
  net_key_id = convert_key_class_to_psa_key_id(key_class);
#ifdef ZW_SLAVE
  zw_wrap_aes_key_secure_vault(&net_key_id, keybuf, ZW_PSA_ALG_CMAC);
#else //ZW_SLAVE
  zw_status_t status = zw_import_aes_key_secure_vault(&net_key_id, keybuf, ZW_PSA_ALG_CMAC);
  if (PSA_SUCCESS != status)
  {
    DPRINT("ERROR: ZW_keystore unable to write S2_Keys to secure vault.\r\n");
    return false;
  }
#endif
#endif

#ifdef ZW_DEBUG
#if defined(ZWAVE_PSA_SECURE_VAULT) && defined(ZWAVE_PSA_AES) && defined(ZW_CONTROLLER)
  status = zw_read_vault_network_key_aes_cmac(net_key_id, tmp_key, ZW_S2_NETWORK_KEY_SIZE, &key_len);
  DPRINTF("%02X%02X%02X\r\n", tmp_key[0], tmp_key[1], tmp_key[2]);

#else // defined(ZWAVE_PSA_SECURE_VAULT) && defined(ZWAVE_PSA_AES) && defined(ZW_CONTROLLER)
  /* Verify what was actually written */
  {
    StorageGetS2Keys(&tSs2keys);
    const uint8_t *pDebug = &tSs2keys.s2_network_keys[key_index][0];
    DPRINTF("%02X%02X%02X\r\n", pDebug[0], pDebug[1], pDebug[2]);
  }
#endif
#endif

  return true;
}

static bool
keystore_network_key_read_impl(
  uint8_t key_class,
  uint8_t *buf)
{
  /* Is key_class assigned */
  Ss2_keyclassesAssigned tSs2_keyclassesAssigned = {0};
  StorageGetS2KeyClassesAssigned(&tSs2_keyclassesAssigned);
  if (0 == (tSs2_keyclassesAssigned.keyclasses_assigned & key_class))
  {
    return false;
  }
  if (!IS_KEY_VALID(key_class))
  {
    DPRINT("Invalid network key read requested. Aborting.\r\n");
    return false;
  }
#if defined(ZWAVE_PSA_SECURE_VAULT) && defined(ZWAVE_PSA_AES) && defined(ZW_CONTROLLER)
  uint32_t net_key_id = convert_key_class_to_psa_key_id(key_class);
  size_t key_len;
  zw_status_t status = zw_read_vault_network_key_aes_cmac(net_key_id, buf, ZW_S2_NETWORK_KEY_SIZE, &key_len);
  if ((ZW_S2_NETWORK_KEY_SIZE != key_len) || (PSA_SUCCESS != status))
  {
    DPRINT("ERROR: ZW_keystore unable to read S2_Keys from secure vault.\r\n");
    return false;
  }

#else
  uint8_t key_index = TO_NVM_INDEX(key_class);
  Ss2_keys tSs2_keys = {0};
  StorageGetS2Keys(&tSs2_keys);
  DPRINTF("Network key read: %02X\r\n", key_class);
  memcpy(buf, &tSs2_keys.s2_network_keys[key_index], ZW_S2_NETWORK_KEY_SIZE);
#endif // defined(ZWAVE_PSA_SECURE_VAULT) && defined(ZWAVE_PSA_AES) && defined(ZWAVE_CONTROLLER)
  DPRINTF("%02X%02X%02X\r\n", buf[0], buf[1], buf[2]);
  return true;
}


bool
keystore_network_key_clear(
  uint8_t keyclass)
{
  Ss2_keyclassesAssigned tSs2_keyclassesAssigned = {0};
  if(false == StorageGetS2KeyClassesAssigned(&tSs2_keyclassesAssigned))
  {
    DPRINT("ERROR: ZW_keystore unable to get S2_KeyClassesAssigned from NVM.\r\n");
    return false;
  }

  if (KEY_CLASS_ALL != keyclass)
  {
    /* If specified keyclass not supported do not update keyclasses_assigned or m_assignedSec_keys */
    if ((keyclass && tSs2_keyclassesAssigned.keyclasses_assigned) == 0)
    {
      return false;
    }
  }
  UpdateCachedSecurityKeys(tSs2_keyclassesAssigned.keyclasses_assigned & ~keyclass);

#if defined(ZWAVE_PSA_SECURE_VAULT) && defined(ZWAVE_PSA_AES)
  if (keyclass == KEY_CLASS_ALL)
  {
    zw_psa_clear_all_network_keys();
  }
  else
  {
    zw_psa_clear_network_key(keyclass);
  }
#endif

  /* Clear the flags for the requested key classes */
  tSs2_keyclassesAssigned.keyclasses_assigned &= ~keyclass;

  if (false == StorageSetS2KeyClassesAssigned(&tSs2_keyclassesAssigned))
  {
    DPRINT("ERROR: ZW_keystore unable to clear S2_KeyClassesAssigned in NVM.\r\n");
    return false;
  }

  return true;
}


/**
* Fetches an s2 key from NVM to SRAM.
* \return true if keyclass assigned and read
*         false if keyclass not supported and therefor not read
* \param[out] buf The memory area to copy to.
* \param[in] key_class The key class to fetch. Key class uses the BITMASK format, but exactly one
*           bit must be set.
* updates m_assignedSec_keys used in ZW_GetsecurityKeys
*/
bool
keystore_network_key_read(
  uint8_t key_class,
  uint8_t *buf)
{
  return keystore_network_key_read_impl(key_class, buf);
}

/**
* Write key_class security key to NVM keystore.
* \return true if keyclass valid -> key written
*         false if keyclass not supported and nothing written
* \param[in] key class to write to. Key class uses the BITMASK format, but exactly one
*            bit must be set.
* \param[in] keybuf points at key in SRAM which is to be written at key_class position in NVM keystore.
*
* updates m_assignedSec_keys used in ZW_GetsecurityKeys
*/
bool
keystore_network_key_write(
  uint8_t key_class,
  const uint8_t *keybuf)
{
  return keystore_network_key_write_impl(key_class, keybuf);
}

void
ZW_KeystoreInit(void)
{
  Ss2_keyclassesAssigned tSs2_keyclassesAssigned = {0};
  if(false == StorageGetS2KeyClassesAssigned(&tSs2_keyclassesAssigned))
  {
    DPRINT("ERROR: ZW_keystore unable to read S2_KeyClassesAssigned from NVM.\r\n");
  }

  UpdateCachedSecurityKeys(tSs2_keyclassesAssigned.keyclasses_assigned);
}

/*
* Get Assigned Security keys bitmask.
*
*/
uint8_t
keystore_get_cached_security_keys(void)
{
  DPRINTF("Get m_assignedSec_keys: 0x%X\n", m_assignedSec_keys);
  return m_assignedSec_keys;
}


static void UpdateCachedSecurityKeys(uint8_t AssignedSec_keys)
{
  DPRINTF("Set m_assignedSec_keys: 0x%X\n", AssignedSec_keys);
  m_assignedSec_keys = AssignedSec_keys;

#if defined(ZW_SECURITY_PROTOCOL)
  SyncEventArg1Invoke(&g_KeystoreSecurityKeysChanged, m_assignedSec_keys);
#endif
}
#endif // #if defined(ZW_SLAVE)
