/// ****************************************************************************
/// @file tr_mfg_tokens.h
///
/// @brief MFG token infrastructure + declarations
///
///
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************
#ifndef TR_MFG_TOKENS_H
#define TR_MFG_TOKENS_H

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "tr_platform_token_def.h"
#include "tr_platform_tokens.h"

// this is used to start the definition of manufacturing tokens
#define TR_START_MFG_TOKEN_DEFS \
        typedef struct __attribute__((packed))

// this is used to end the definition of mfg tokens
#define TR_END_MFG_TOKEN_DEFS \
        tr_mfg_token_struct_t;

// this is used to create a token which becomes an element in the
// structure with the passed in type and name
#define TR_CREATE_MFG_TOKEN(name, type) \
        type name;

/// ****************************************************************************
/// @defgroup tr_mfg_tokens MFG Tokens
/// @ingroup trident_topic_tokens
/// @{
/// ****************************************************************************

/// @brief gets the size of a MFG token
/// @param token name of MFG token (ex. TR_MFG_TOKEN_VERSION)
/// @return returns the size of a specific element in the token structure
#define tr_get_mfg_token_len(token) \
        sizeof(((tr_mfg_token_struct_t*)0)->token)

/// @brief gets the address offset to a specific MFG token
/// @param token name of MFG token (ex. TR_MFG_TOKEN_VERSION)
/// @return returns the address of the structure element referenced to 0
#define tr_get_mfg_token_offset(token) \
        ((uint32_t)(&((tr_mfg_token_struct_t*)0)->token))

/// @brief copies the token data to the location pointed to by data
/// @param data pointer to buffer to store the token data
/// @param token name of MFG token to read (ex. TR_MFG_TOKEN_VERSION)
#define tr_get_mfg_token(data, token)                                \
        tr_platform_token_read((uint8_t*)data,                       \
                               (uint8_t)tr_get_mfg_token_len(token), \
                               (uint32_t)(TR_MFG_TOKEN_BASE + tr_get_mfg_token_offset(token)))

/// @brief checks if a specific MFG token is erased
/// @param token name of MFG token (ex. TR_MFG_TOKEN_VERSION)
/// @return returns true if erased, false otherwise
#define tr_mfg_token_check_erased(token)                             \
        ({                                                           \
        bool is_erased = true;                                       \
        uint8_t count = 0;                                           \
        uint8_t data[200];                                           \
        tr_get_mfg_token(data, token);                               \
        for (size_t i = 0 ; i < tr_get_mfg_token_len(token) ; i++) { \
            if (data[i] == 0xFF) {                                   \
                count++;                                             \
            }                                                        \
        }                                                            \
        if (count != tr_get_mfg_token_len(token)) {                  \
            is_erased = false;                                       \
        }                                                            \
        is_erased;                                                   \
    })

/// @brief write token data to a specific MFG token
/// @param data pointer to buffer with the token data
/// @param data_len number of bytes to write
/// @param token name of MFG token to write (ex. TR_MFG_TOKEN_VERSION)
#define tr_set_mfg_token(data, data_len, token)                                                          \
        do {                                                                                             \
            if (tr_mfg_token_check_erased(token)) {                                                      \
                tr_platform_token_write((uint8_t*)data,                                                  \
                                        (uint8_t)data_len,                                               \
                                        (uint32_t)(TR_MFG_TOKEN_BASE + tr_get_mfg_token_offset(token))); \
            }                                                                                            \
        } while (0)

/// @} // end of trident_topic_tokens

// NOTE: tokens are written into flash as byte strings which means
// data types larger than uint8_t (not counting arrays of uint8_t's)
// are read out in big endian format. In order to fix this, anything that
// is a uint16_t or greater needs to use __attribute__((packed, scalar_storage_order("big-endian")))
// to change the endian-ness during a read.

// extra care must be taken when doing a write from runtime to ensure that
// the endian-ness is written as intended as well.

// mfg token types
typedef struct __attribute__((packed, scalar_storage_order("big-endian")))
{
    uint16_t value;
}
tr_mfg_tok_type_version;
typedef uint8_t tr_mfg_tok_type_custom_eui[8];
typedef uint8_t tr_mfg_tok_type_mfg_name[32];
typedef uint8_t tr_mfg_tok_type_model_name[32];
typedef uint8_t tr_mfg_tok_type_hw_version;
typedef struct __attribute__((packed, scalar_storage_order("big-endian")))
{
    uint16_t value;
}
tr_mfg_tok_type_manuf_id;
typedef uint8_t tr_mfg_tok_type_serial_num[8];
typedef struct __attribute__((packed, scalar_storage_order("big-endian")))
{
    uint16_t value;
}
tr_mfg_tok_type_xtal_trim;
typedef struct __attribute__((packed, scalar_storage_order("big-endian")))
{
    uint16_t value;
}
tr_mfg_tok_type_phy_config;
typedef struct __attribute__((packed, scalar_storage_order("big-endian")))
{
    uint16_t value;
}
tr_mfg_tok_type_cca_threshold;
typedef struct
{
    uint8_t certificate[48];
    uint8_t ca_public_key[22];
    uint8_t private_key[21];
    // The lsb indicates the token has been initialzed.
    // Other flag bits are undefined.
    uint8_t flags;
} tr_mfg_tok_type_cbke_data;
typedef struct __attribute__((packed, scalar_storage_order("big-endian")))
{
    // The lsb indicates the token has been initialzed.
    // Bits 1 through 4 give the size of the value. Valid options are:
    // 6 (0x6) bytes, 8 (0x8) bytes, 12 (0xc) bytes, and 16 (0xf) bytes
    // Other flag bits are undefined.
    uint16_t flags;
    uint8_t  value[16];
    uint16_t crc;
}
tr_mfg_tok_type_installation_code;
typedef uint8_t tr_mfg_tok_type_distributed_key[16];
typedef struct __attribute__((packed, scalar_storage_order("big-endian")))
{
    uint16_t value;
}
tr_mfg_tok_type_security_config;
typedef struct
{
    uint8_t certificate[74];
    uint8_t ca_public_key[37];
    uint8_t private_key[36];
    // The lsb indicates the token has been initialzed.
    // Other flag bits are undefined.
    uint8_t flags;
} tr_mfg_tok_type_cbke_283k1_data;
typedef uint8_t tr_mfg_tok_type_nvm_crypto_key[16];
typedef uint8_t tr_mfg_tok_type_bootload_aes_key[16];
typedef uint8_t tr_mfg_tok_type_secure_bootloader_key[16];
typedef uint8_t tr_mfg_tok_type_signed_bootloader_key_x[32];
typedef uint8_t tr_mfg_tok_type_signed_bootloader_key_y[32];
typedef uint8_t tr_mfg_tok_type_serial_boot_delay_sec;
typedef struct __attribute__((packed, scalar_storage_order("big-endian")))
{
    uint8_t  join_key[32];
    uint16_t join_key_length;
}
tr_mfg_tok_type_thread_join_key;
typedef uint8_t tr_mfg_tok_type_zwave_region;
typedef uint8_t tr_mfg_tok_type_zwave_initialized;
typedef uint8_t tr_mfg_tok_type_zwave_qr_code[106];
typedef uint8_t tr_mfg_tok_type_zwave_puk[32];
typedef uint8_t tr_mfg_tok_type_zwave_prk[32];

TR_START_MFG_TOKEN_DEFS
{
    TR_CREATE_MFG_TOKEN(TR_MFG_TOKEN_VERSION, tr_mfg_tok_type_version)
    TR_CREATE_MFG_TOKEN(TR_MFG_TOKEN_CUSTOM_EUI, tr_mfg_tok_type_custom_eui)
    TR_CREATE_MFG_TOKEN(TR_MFG_TOKEN_MFG_NAME, tr_mfg_tok_type_mfg_name)
    TR_CREATE_MFG_TOKEN(TR_MFG_TOKEN_MODEL_NAME, tr_mfg_tok_type_model_name)
    TR_CREATE_MFG_TOKEN(TR_MFG_TOKEN_HW_VERSION, tr_mfg_tok_type_hw_version)
    TR_CREATE_MFG_TOKEN(TR_MFG_TOKEN_MANUF_ID, tr_mfg_tok_type_manuf_id)
    TR_CREATE_MFG_TOKEN(TR_MFG_TOKEN_SERIAL_NUM, tr_mfg_tok_type_serial_num)
    TR_CREATE_MFG_TOKEN(TR_MFG_TOKEN_XTAL_TRIM, tr_mfg_tok_type_xtal_trim)
    TR_CREATE_MFG_TOKEN(TR_MFG_TOKEN_PHY_CONFIG, tr_mfg_tok_type_phy_config)
    TR_CREATE_MFG_TOKEN(TR_MFG_TOKEN_CCA_THRESHOLD, tr_mfg_tok_type_cca_threshold)
    TR_CREATE_MFG_TOKEN(TR_MFG_TOKEN_CBKE_DATA, tr_mfg_tok_type_cbke_data)
    TR_CREATE_MFG_TOKEN(TR_MFG_TOKEN_INSTALLATION_CODE, tr_mfg_tok_type_installation_code)
    TR_CREATE_MFG_TOKEN(TR_MFG_TOKEN_DISTRIBUTED_KEY, tr_mfg_tok_type_distributed_key)
    TR_CREATE_MFG_TOKEN(TR_MFG_TOKEN_SECURITY_CONFIG, tr_mfg_tok_type_security_config)
    TR_CREATE_MFG_TOKEN(TR_MFG_TOKEN_CBKE_283K1_DATA, tr_mfg_tok_type_cbke_283k1_data)
    TR_CREATE_MFG_TOKEN(TR_MFG_TOKEN_NVM_CRYPTO_KEY, tr_mfg_tok_type_nvm_crypto_key)
    TR_CREATE_MFG_TOKEN(TR_MFG_TOKEN_BOOTLOAD_AES_KEY, tr_mfg_tok_type_bootload_aes_key)
    TR_CREATE_MFG_TOKEN(TR_MFG_TOKEN_SECURE_BOOTLOADER_KEY, tr_mfg_tok_type_secure_bootloader_key)
    TR_CREATE_MFG_TOKEN(TR_MFG_TOKEN_SIGNED_BOOTLOADER_KEY_X, tr_mfg_tok_type_signed_bootloader_key_x)
    TR_CREATE_MFG_TOKEN(TR_MFG_TOKEN_SIGNED_BOOTLOADER_KEY_Y, tr_mfg_tok_type_signed_bootloader_key_y)
    TR_CREATE_MFG_TOKEN(TR_MFG_TOKEN_SERIAL_BOOT_DELAY_SEC, tr_mfg_tok_type_serial_boot_delay_sec)
    TR_CREATE_MFG_TOKEN(TR_MFG_TOKEN_THREAD_JOIN_KEY, tr_mfg_tok_type_thread_join_key)
    TR_CREATE_MFG_TOKEN(TR_MFG_TOKEN_ZWAVE_COUNTRY_FREQ, tr_mfg_tok_type_zwave_region)
    TR_CREATE_MFG_TOKEN(TR_MFG_TOKEN_ZWAVE_INITIALIZED, tr_mfg_tok_type_zwave_initialized)
    TR_CREATE_MFG_TOKEN(TR_MFG_TOKEN_ZWAVE_QR_CODE, tr_mfg_tok_type_zwave_qr_code)
    TR_CREATE_MFG_TOKEN(TR_MFG_TOKEN_ZWAVE_PUK, tr_mfg_tok_type_zwave_puk)
    TR_CREATE_MFG_TOKEN(TR_MFG_TOKEN_ZWAVE_PRK, tr_mfg_tok_type_zwave_prk)
}
TR_END_MFG_TOKEN_DEFS

void tr_mfg_tokens_process(void);

#endif // TR_MFG_TOKENS_H
