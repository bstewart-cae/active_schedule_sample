// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file
 * @brief Generate the Manufacturing Token system
 *
 * @copyright 2018 Silicon Laboratories Inc.
 */
#include <stdint.h>
#include <stdio.h>
#include "MfgTokens.h"
#include <string.h>
#include "Assert.h"
#include <zpal_nvm.h>

//#define DEBUGPRINT
#include "DebugPrint.h"

static zpal_nvm_handle_t handle;

void ZW_MfgTokenModuleInit(void)
{
  handle = zpal_nvm_init(ZPAL_NVM_AREA_MANUFACTURER_TOKENS);
}

void ZW_SetMfgTokenData(uint16_t token, void *data, uint8_t len)
{
  ASSERT(handle);
  zpal_nvm_write(handle, token, data, len);
}

void ZW_GetMfgTokenData(void *data, uint16_t token, uint8_t len)
{
  ASSERT(handle);
  zpal_nvm_read(handle, token, data, len);
}

void ZW_GetMfgTokenDataCountryFreq(void *data)
{
  ASSERT(handle);
  zpal_nvm_read(handle, TOKEN_MFG_ZWAVE_COUNTRY_FREQ_ID, data, TOKEN_MFG_ZWAVE_COUNTRY_FREQ_SIZE);
}

void ZW_SetMfgTokenDataCountryRegion(void* region)
{
  ZW_SetMfgTokenData(TOKEN_MFG_ZWAVE_COUNTRY_FREQ_ID,
                     region,
                     TOKEN_MFG_ZWAVE_COUNTRY_FREQ_SIZE);
}

void ZW_LockMfgTokenData(void)
{
  zpal_nvm_lock(handle);
}

