// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file ZW_frames_filters_mock.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include <stdint.h>
#include <ZW_Frame.h>

uint8_t IsFrameIlLegal (ZW_HeaderFormatType_t curHeader, frame* pRxFrame, uint8_t* pCmdClass, uint8_t *pCmd)
{
  return 0;
}
