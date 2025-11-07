/// ***************************************************************************
///
/// @file flashdb_low_lvl.h
///
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2023 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************

#ifndef FLASHDB_LOW_LVL_H
#define FLASHDB_LOW_LVL_H

#define NVM_PAGE_SIZE    256
#define NVM_ERASE_SIZE   4096

void nvm_init(void);

uint32_t nvm_get_start_address(void);

void nvm_mfg_token_read(uint32_t nvmAddress, uint32_t Len, uint8_t *pDestBuffer);
void nvm_mfg_token_write(uint32_t nvmAddress, uint32_t Len, uint8_t *pSrcBuffer);
void nvm_write(uint32_t nvmAddress, uint32_t Len, uint8_t *pSrcBuffer);
void nvm_read(uint32_t nvmAddress, uint32_t Len, uint8_t *pDestBuffer);

#endif
