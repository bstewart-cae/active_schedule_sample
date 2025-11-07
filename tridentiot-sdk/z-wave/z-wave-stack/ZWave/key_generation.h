// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/*
 * @file
 * @copyright 2022 Silicon Laboratories Inc.
 *
 * @brief definations of fucntions for geerating tokend and keys tokens used
 * by ZWave protocols
 */


#ifndef KEY_GENERATION_H
#define KEY_GENERATION_H

#ifdef __cplusplus
extern "C" {
#endif

#define KEY_SIZE (32) // In bytes

/**
 * Checks if manufacturing key exist in the flash or not.
 * If not, generates and writes the public key, private key and QR code.
 *
 * @param[in] regionLR True if device uses Z-Wave Long Range region
 *                     False otherwise.
 */
void check_create_keys_qrcode(bool regionLR);


#ifdef __cplusplus
}
#endif

#endif /* KEY_GENERATION */
