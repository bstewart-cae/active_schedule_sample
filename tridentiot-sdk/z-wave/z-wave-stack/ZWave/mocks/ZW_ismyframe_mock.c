// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file ZW_ismyframe_mock.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include <stdint.h>
#include <ZW_Frame.h>
#include <ZW_transport.h>

bool IsMyFrame(
    ZW_HeaderFormatType_t curHeader,
    RX_FRAME *pRxFrame,
    uint8_t * pReceiveStatus)
{
  return false;
}
