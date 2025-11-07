/// ****************************************************************************
/// @file tr_platform_tokens.h
///
/// @brief platform specific token API declarations
///
///
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************
#ifndef TR_PLATFORM_TOKENS_H
#define TR_PLATFORM_TOKENS_H

#include <stdint.h>

void tr_platform_token_write(uint8_t  *buffer,
                             uint8_t  buf_size,
                             uint32_t token_addr);

void tr_platform_token_read(void     *buffer,
                            uint8_t  buf_size,
                            uint32_t token_addr);

void tr_platform_token_process_xtal_trim(uint16_t xtal_trim);

#endif // TR_PLATFORM_TOKENS_H
