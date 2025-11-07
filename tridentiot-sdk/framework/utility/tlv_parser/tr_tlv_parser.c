/// ****************************************************************************
/// @file tr_tlv_parser.c
///
/// @brief A utility to parse TridentIot TLV data.
///
///
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************
#include "tr_tlv_parser.h"

#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#define TLV_HEADER_LENGTH 2      ///< 1 byte tag + 1 byte length
#define TLV_LENGTH_OFFSET 1      ///< Offset of the length field in a TLV entry


bool find_tlv_by_tag(const uint8_t *buffer,
                     size_t buffer_len,
                     uint8_t tag,
                     uint8_t *out_value,
                     uint8_t *out_value_len) {
    size_t index = 0;

    while ((index + TLV_HEADER_LENGTH) <= buffer_len) {
        uint8_t current_tag = buffer[index];
        uint8_t length = buffer[index + TLV_LENGTH_OFFSET];

        if ((index + TLV_HEADER_LENGTH + length) > buffer_len) {
            return false;  // Malformed TLV: length field extends beyond buffer
        }

        if ((current_tag == tag) && length) {
            if ((out_value == NULL) || (out_value_len == NULL)) {
                return false; // Output buffer invalid or out_value_len is invalid
            }

            memcpy(out_value, &buffer[index + TLV_HEADER_LENGTH], length);
            *out_value_len = length;
            return true;
        }

        index += (TLV_HEADER_LENGTH + length);
    }

    return false; // Tag not found or length mismatch
}


bool update_tlv_tag(uint8_t *buffer,
                    size_t buffer_size,
                    uint8_t tag,
                    const uint8_t *value,
                    uint8_t length) {
    if (!buffer || !buffer_size || !value) {
      return false;
    }

    size_t index = 0;

    while (index + TLV_HEADER_LENGTH <= buffer_size) {
        uint8_t current_tag = buffer[index];

        // Stop at first 0xFF — marks start of unused space
        if (current_tag == 0xFF) {
            break;
        }

        uint8_t len = buffer[index + TLV_LENGTH_OFFSET];

        // Check if the TLV entry fits within the buffer
        if (index + TLV_HEADER_LENGTH + len > buffer_size) {
            return false;
        }

        // If tag matches and length matches, update value
        if (current_tag == tag && len == length) {
            memcpy(&buffer[index + TLV_HEADER_LENGTH], value, length);
            return true;
        }

        index += TLV_HEADER_LENGTH + len;
    }

    // Append new TLV entry if space is available
    if (index + TLV_HEADER_LENGTH + length > buffer_size) {
        return false; // not enough space
    }

    buffer[index] = tag;
    buffer[index + TLV_LENGTH_OFFSET] = length;
    memcpy(&buffer[index + TLV_HEADER_LENGTH], value, length);

    return true;
}
