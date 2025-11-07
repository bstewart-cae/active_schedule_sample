/// ****************************************************************************
/// @file tr_app_tokens.h
///
/// @brief APP token infrastructure
///
///
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************
#ifndef TR_APP_TOKENS_H
#define TR_APP_TOKENS_H

#include <stdint.h>
#include <stdbool.h>
#include "tr_platform_token_def.h"
#include "tr_platform_tokens.h"

/// ****************************************************************************
/// @defgroup tr_app_tokens APP Tokens
/// @details The TR_START_APP_TOKEN_DEFS and TR_END_APP_TOKEN_DEFS macros create a
/// strucutre of all application tokens with defined element names and data types. <br>
/// For example: <br>
/// TR_START_APP_TOKEN_DEFS <br>
/// { <br>
/// <pre> TR_CREATE_APP_TOKEN(MY_APP_TOKEN, uint8_t) </pre>
/// } <br>
/// TR_END_APP_TOKEN_DEFS <br>
///
/// @ingroup trident_topic_tokens
/// @{
/// ****************************************************************************

/// @brief used to start the definition of the application tokens structure
#define TR_START_APP_TOKEN_DEFS \
        typedef struct __attribute__((packed))

/// @brief used to end the definition of the application tokens structure
#define TR_END_APP_TOKEN_DEFS \
        tr_app_token_struct_t;

/// @brief used to create a token which becomes an element in the application
/// tokens structure with the passed in type and name
/// @param token name of token to be created (ex. MY_APP_TOKEN)
/// @param type data tyoe of token being created (ex. uint8_t)
#define TR_CREATE_APP_TOKEN(token, type) \
        type token;

/// @brief gets the size of an APP token
/// @param token name of APP token (ex. MY_APP_TOKEN)
/// @return returns the size of a specific element in the token structure
#define tr_get_app_token_len(token) \
        sizeof(((tr_app_token_struct_t*)0)->token)

/// @brief gets the address offset to a specific APP token
/// @param token name of APP token (ex. MY_APP_TOKEN)
/// @return returns the address of the structure element referenced to 0
#define tr_get_app_token_offset(token) \
        ((uint32_t)(&((tr_app_token_struct_t*)0)->token))

/// @brief copies the token data to the location pointed to by data
/// @param data pointer to buffer to store the token data
/// @param token name of APP token to read (ex. MY_APP_TOKEN)
#define tr_get_app_token(data, token)                                \
        tr_platform_token_read(data,                                 \
                               (uint8_t)tr_get_app_token_len(token), \
                               (uint32_t)(TR_APP_TOKEN_BASE + tr_get_app_token_offset(token)))

/// @brief checks if a specific APP token is erased
/// @param token name of APP token (ex. MY_APP_TOKEN)
/// @return returns true if erased, false otherwise
#define tr_app_token_check_erased(token)                             \
        ({                                                           \
        bool is_erased = true;                                       \
        uint8_t count = 0;                                           \
        uint8_t data[200];                                           \
        tr_get_app_token(data, token);                               \
        for (size_t i = 0 ; i < tr_get_app_token_len(token) ; i++) { \
            if (data[i] == 0xFF) {                                   \
                count++;                                             \
            }                                                        \
        }                                                            \
        if (count != tr_get_app_token_len(token)) {                  \
            is_erased = false;                                       \
        }                                                            \
        is_erased;                                                   \
    })

/// @brief write token data to a specific APP token
/// @param data pointer to buffer with the token data
/// @param data_len number of bytes to write
/// @param token name of APP token to write (ex. MY_APP_TOKEN)
#define tr_set_app_token(data, data_len, token)                                                        \
        do {                                                                                           \
            tr_platform_token_write((uint8_t*)data,                                                    \
                                    (uint8_t)data_len,                                                 \
                                    (uint32_t)((TR_APP_TOKEN_BASE + tr_get_app_token_offset(token)))); \
        } while (0)

/// @} // end of trident_topic_tokens

#endif // TR_APP_TOKENS_H
