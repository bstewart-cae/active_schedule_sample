/// ****************************************************************************
/// @file tr_mfg_tokens.c
///
/// @brief
///
///
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************
#include "tr_mfg_tokens.h"
#include "flashctl.h"
#if defined(TR_PLATFORM_T32CZ20) || defined(TR_PLATFORM_T32CM11C)
#include "tr_tlv_parser.h"
#endif

#define XTAL_TRIM_TAG  0x01
#define XTAL_TRIM_SIZE 0x02

/*
 * Read the xtal_trim value from the security page
 * The xtal_trim value is stored at address 0x1200
 */
static void xtal_trim_read(uint8_t *xtal_trim)
{
#if defined(TR_PLATFORM_T32CZ20) || defined(TR_PLATFORM_T32CM11C)
    uint8_t rd_buf[256];
    uint8_t xtal_trim_len;

    flash_read_sec_register((uint32_t)&rd_buf, FLASH_SECREG_R1_P2);

    if (!find_tlv_by_tag(rd_buf,
                         sizeof(rd_buf),
                         XTAL_TRIM_TAG,
                         xtal_trim,
                         &xtal_trim_len) || (xtal_trim_len != XTAL_TRIM_SIZE))
    {
        xtal_trim[0] = 0xFF;
        xtal_trim[1] = 0xFF;
    }
#else  /* if defined(TR_PLATFORM_T32CZ20) || defined(TR_PLATFORM_T32CM11C) */
    xtal_trim[0] = 0xFF;
    xtal_trim[1] = 0xFF;
#endif /* if defined(TR_PLATFORM_T32CZ20) || defined(TR_PLATFORM_T32CM11C) */
}

// intended to be called on boot or after writing one of the tokens that gets checked in here
void tr_mfg_tokens_process(void)
{
    tr_mfg_tok_type_xtal_trim xtal_trim;
    tr_get_mfg_token(&xtal_trim, TR_MFG_TOKEN_XTAL_TRIM);

    // if xal trim token is empty, read from the security page
    if (xtal_trim.value == 0xFFFF)
    {
        xtal_trim_read((uint8_t*)&xtal_trim);
    }

    // if xtal trim not empty
    if (xtal_trim.value != 0xFFFF)
    {
        tr_platform_token_process_xtal_trim(xtal_trim.value);
    }

    // TODO: determine which other tokens we care to process on boot
}
