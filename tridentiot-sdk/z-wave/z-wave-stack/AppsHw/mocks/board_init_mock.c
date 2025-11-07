// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file board_init_mock.c
 * @copyright 2022 Silicon Laboratories Inc.
 */

#include <mock_control.h>
#include <board_init.h>

#define MOCK_FILE "board_init_mock.c"

void Board_Init(void)
{
  mock_t * p_mock;
  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(p_mock);
}
