// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file ZAF_command_class_utils_mock.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include <ZW_security_api.h>

bool
CmdClassSupported(security_key_t eKey,
                  uint8_t commandClass,
                  uint8_t command,
                  uint8_t* pSecurelist,
                  uint8_t securelistLen,
                  uint8_t* pNonSecurelist,
                  uint8_t nonSecurelistLen)
{
  return true;
}
