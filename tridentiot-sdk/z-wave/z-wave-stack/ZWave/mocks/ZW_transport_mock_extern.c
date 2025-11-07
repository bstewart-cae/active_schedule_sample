// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file ZW_transport_mock_extern.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include "mock_control.h"
#include "ZW_transport.h"

#define MOCK_FILE "ZW_transport_mock_extern.c"

void CommandHandler(CommandHandler_arg_t * pArgs)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(p_mock);
  MOCK_CALL_ACTUAL(p_mock, pArgs);

  MOCK_CALL_COMPARE_STRUCT_ARRAY_UINT8(p_mock, ARG0, pArgs, CommandHandler_arg_t, cmd, cmdLength);
  MOCK_CALL_COMPARE_STRUCT_MEMBER_UINT8(p_mock, ARG0, pArgs, CommandHandler_arg_t, cmdLength);

  TEST_FAIL_MESSAGE("This mock is not finished");
}
