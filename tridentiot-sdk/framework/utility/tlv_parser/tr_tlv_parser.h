/// ****************************************************************************
/// @file tr_tlv_parser.h
///
/// @brief A utility to parse TridentIot TLV data.
///
///
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************

#ifndef TR_TLV_PARSER_H
#define TR_TLV_PARSER_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Search for a TLV entry by tag and validate its length.
 *
 * If a matching tag is found and its value length is not zero,
 * the value is copied into the provided output buffer.
 *
 * @param buffer         Pointer to the TLV data buffer.
 * @param buffer_len     Length of the TLV data buffer in bytes.
 * @param tag            Tag value to search for.
 * @param out_value      Pointer to a buffer where the value will be copied.
 * @param out_value_len  Pointer to variable holding the length of the copied data.
 *
 * @return true if the tag is found, the length matches, and value was copied; false otherwise.
 */
bool find_tlv_by_tag(const uint8_t *buffer,
                     size_t buffer_len,
                     uint8_t tag,
                     uint8_t *out_value,
                     uint8_t *out_value_len);

/**
 * @brief Update an existing TLV entry if found, or append a new one.
 *        Assumes unused buffer space is filled with 0xFF.
 *
 * @param buffer       Pointer to the TLV buffer.
 * @param buffer_size  Total size of the buffer in bytes.
 * @param tag          Tag to search or insert.
 * @param value        Pointer to the value data to insert/update.
 * @param length       Length of the value in bytes.
 * @return true if the update or append was successful, false otherwise.
 */
bool update_tlv_tag(uint8_t *buffer,
                    size_t buffer_size,
                    uint8_t tag,
                    const uint8_t *value,
                    uint8_t length);

#ifdef __cplusplus
}
#endif

#endif // TR_TLV_PARSER_H
