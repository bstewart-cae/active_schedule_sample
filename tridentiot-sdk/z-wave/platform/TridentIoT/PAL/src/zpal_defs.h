/// ***************************************************************************
///
/// @file zpal_defs.h
///
/// @brief  Platform specific definitions
///
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2024 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************

#ifndef ZPAL_DEFS_H_
#define ZPAL_DEFS_H_
#include "tr_platform.h"
#define RET_REG_ENTROPY_SEED_INDEX   1
#define RET_REG_RESET_REASON_INDEX   0

#define REGISTER_RETENTION_ARRAY(arr_type,arr_name,arr_length)\
  arr_type arr_name[arr_length] __attribute__((section(".ret_sram"), __used__ ))

#define REGISTER_RETENTION_VAR(var_type, var_name)\
  var_type var_name __attribute__((section(".ret_sram"), __used__ ))

#define MAGIC_STR_LEN     5
#define MAGIC_WORD_STR    "M_425"         // in hex 0x4D5F343235

typedef struct  __attribute__((packed))
{
  uint8_t app_version_major;
  uint8_t app_version_minor;
  uint8_t app_version_patch;
} app_version_t;

typedef struct  __attribute__((packed))
{
    uint8_t        magic_word[5];
    app_version_t  app_version;
    uint16_t       manufacturer_id;
    uint16_t       product_type_id;
    uint16_t       product_id;
    uint8_t        padding[2];       // This structure MUST be 16 bytes
} app_version_info_t;

#endif /* ZPAL_DEFS_H_ */
