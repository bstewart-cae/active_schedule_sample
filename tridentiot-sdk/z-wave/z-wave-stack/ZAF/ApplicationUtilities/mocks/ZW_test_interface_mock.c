// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file ZW_test_interface_mock.c
 * @copyright 2022 Silicon Laboratories Inc.
 */

#include <ZW_typedefs.h>
#include <stdint.h>

bool ZW_test_interface_allocate(char channel, VOID_CALLBACKFUNC(pCallback) (char channel, char * pString))
{
  return true;
}
