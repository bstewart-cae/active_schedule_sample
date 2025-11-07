// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file ZW_keystore_mock.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include <ZW_typedefs.h>
#include <mock_control.h>
#include <ZW_security_api.h>

#define MOCK_FILE "ZW_keystore_mock.c"

uint8_t
keystore_get_cached_security_keys(void)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(SECURITY_KEY_NONE);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(pMock, false);

  MOCK_CALL_RETURN_VALUE(pMock, uint8_t);
}

void
ZW_SetSecurityS0NetworkKey(uint8_t * network_key)
{

}

void
keystore_public_dsk_read(uint8_t *buf)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(pMock);
  MOCK_CALL_ACTUAL(pMock, buf);

  MOCK_CALL_COMPARE_INPUT_POINTER(pMock, ARG0, buf);
}

void ZW_KeystoreInit(void)
{

}

void 
keystore_dynamic_keypair_generate(void)
{

}

void
keystore_smartstart_initiated(bool smartstartinitiated)
{

}
