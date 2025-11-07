// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file ZW_crc_mock.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include <stdint.h>

uint16_t
ZW_CheckCrc16(
  uint16_t crc,
  uint8_t *pDataAddr,
  uint16_t bDataLen)
{
  return 0;
}
