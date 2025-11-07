// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file ZW_secure_learn_support_mock.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include <ZW_typedefs.h>
#include "mock_control.h"

#ifndef ZW_SECURITY_PROTOCOL
#define ZW_SECURITY_PROTOCOL
#endif
#include "ZW_secure_learn_support.h"

#define MOCK_FILE "ZW_secure_learn_support_mock.c"

E_SECURE_LEARN_ERRNO
secure_learn_get_errno(void)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(0);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(pMock, 0);

  MOCK_CALL_RETURN_VALUE(pMock, E_SECURE_LEARN_ERRNO);
}

void
secure_learn_set_errno_reset(void)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(pMock);
}

uint8_t
getSecureKeysRequested(void)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(0x00);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(pMock, 0x00);

  MOCK_CALL_RETURN_VALUE(pMock, uint8_t);
}


void security_reset(void)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(pMock);
}

void security_learn_begin(sec_learn_complete_t __cb)
{
  mock_t * pMock;
  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(pMock);

  MOCK_CALL_ACTUAL(pMock, __cb);
  MOCK_CALL_COMPARE_INPUT_POINTER(pMock, ARG0, __cb);

}

void secure_learn_set_errno(E_SECURE_LEARN_ERRNO error_number)
{

}


void security_learn_begin_early(sec_learn_complete_t __cb)
{

}

void security_learn_exit(void)
{

}

void SecurityCommandHandler(uint8_t *pCmd_,
                            uint8_t cmdLength,
                            RECEIVE_OPTIONS_TYPE *rxopt)
{

}


void security_init(uint8_t SecureKeysRequested)
{

}
