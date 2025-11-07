// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file ZW_Security_Scheme0_mock.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include <ZW_security_api.h>
#include <string.h>
#include <ZW_transport_transmit_cb.h>


uint8_t sec0_send_data(uint16_t snode,
                       uint16_t dnode,
                       const void* pData,
                       uint8_t len,
                       uint8_t txoptions,
                       const STransmitCallback* pCallback)
{
  return 0;
}
