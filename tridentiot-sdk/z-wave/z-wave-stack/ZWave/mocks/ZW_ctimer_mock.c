// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file ZW_ctimer_mock.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include <ZW_ctimer.h>
#include "mock_control.h"

#define MOCK_FILE "ZW_ctimer_mock.c"

void ctimer_init(void)
{
  mock_t * pMock;
  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(pMock);

}


void ctimer_set(struct ctimer *c, clock_time_t t,
                void(*f)(void *), void *ptr)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(pMock);

  MOCK_CALL_ACTUAL(pMock, c, t, f, ptr);

  MOCK_CALL_COMPARE_INPUT_POINTER(pMock, ARG0, c);
  MOCK_CALL_COMPARE_INPUT_UINT32(pMock, ARG1, t);
  MOCK_CALL_COMPARE_INPUT_POINTER(pMock, ARG2, f);
  MOCK_CALL_COMPARE_INPUT_POINTER(pMock, ARG3, ptr);  

}

void
ctimer_stop(struct ctimer *c)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(pMock);

  MOCK_CALL_ACTUAL(pMock, c);
  MOCK_CALL_COMPARE_INPUT_POINTER(pMock, ARG0, c);  
}

int
ctimer_expired(struct ctimer *c)
{
  mock_t * pMock;
  MOCK_CALL_RETURN_IF_USED_AS_STUB(true);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(pMock, false);

  MOCK_CALL_ACTUAL(pMock, c);
  MOCK_CALL_COMPARE_INPUT_POINTER(pMock, ARG0, c);
  MOCK_CALL_RETURN_VALUE(pMock, int);
}
