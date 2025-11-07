// SPDX-FileCopyrightText: 2017 Silicon Laboratories Inc. <https://www.silabs.com/>
// SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
// SPDX-License-Identifier: BSD-3-Clause
#ifndef BIGINT_H
#define BIGINT_H


#include <stdint.h>


/* Arithmetic on big integers represented as arrays of unsigned char in radix 2^256 */

/*
#define bigint_add avrnacl_bigint_add
#define bigint_sub avrnacl_bigint_sub
#define bigint_mul avrnacl_bigint_mul
#define bigint_mul32 avrnacl_bigint_mul32
#define bigint_cmov avrnacl_bigint_cmov
*/

/**
 * @brief Adds two big integers (byte arrays) with MSB placed at index 0.
 *
 * @param[out] r   Address of an array where the result will be written.
 * @param[in]  a   Address of array one.
 * @param[in]  b   Address of array two.
 * @param[in]  len Size of integer aka. length of array one and two and the result array.
 * @return Returns the carry.
 */
 uint8_t bigint_add_big_endian(uint8_t *r, const uint8_t *a, const uint8_t *b, uint16_t len);

/**
 * @brief Adds two big integers (byte arrays) with LSB placed at index 0.
 *
 * @param[out] r   Address of an array where the result will be written.
 * @param[in]  a   Address of array one.
 * @param[in]  b   Address of array two.
 * @param[in]  len Size of integer aka. length of array one and two and the result array.
 * @return Returns the carry.
 */
uint8_t bigint_add(uint8_t *r, const uint8_t *a, const uint8_t *b, uint16_t len);

uint8_t bigint_sub(uint8_t *r, const uint8_t *a, const uint8_t *b, uint16_t len);

void bigint_mul(unsigned char *r, const unsigned char *a, const unsigned char *b, uint8_t len);

void bigint_mul32(uint8_t *r, const uint8_t *a, const uint8_t *b);

void bigint_cmov(uint8_t *r, const uint8_t *x, uint8_t b, uint16_t len);

#endif
