// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file
 * Functions for calculation of CRC.
 * @copyright 2018 Silicon Laboratories Inc.
 */
#include <CRC.h>

#define POLY 0x1021          /* crc-ccitt mask */

uint16_t CRC_CheckCrc16(
  uint16_t crc,
  const uint8_t *pDataAddr,
  uint16_t bDataLen)
{
  uint8_t WorkData;
  uint8_t bitMask;
  uint8_t NewBit;

  while(bDataLen--)
  {
    WorkData = *pDataAddr;
    pDataAddr++;
    for (bitMask = 0x80; bitMask != 0; bitMask >>= 1)
    {
      /* Align test bit with next bit of the message byte, starting with msb. */
      NewBit = ((WorkData & bitMask) != 0) ^ ((crc & 0x8000) != 0);
      crc <<= 1;
      if (NewBit)
      {
        crc ^= POLY;
      }
    } /* for (bitMask = 0x80; bitMask != 0; bitMask >>= 1) */
  }
  return crc;
}
