/// ***************************************************************************
///
/// @file fal_cfg.h
///
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2024 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************

#ifndef _FAL_CFG_H_
#define _FAL_CFG_H_
#include <stdint.h>

//#define FAL_DEBUG 1
#define FAL_PART_HAS_TABLE_CFG
#define NOR_FLASH_DEV_NAME             "t32cz20_flash"

#define FDB_KV_NAME_MAX               6
#define FDB_KV_CACHE_TABLE_SIZE       64
#define FDB_SECTOR_CACHE_TABLE_SIZE   8
extern uint32_t __nvm_storage_start__;
/* ===================== Flash device Configuration ========================= */
extern const struct fal_flash_dev t32cz20_onchip_flash;

/* flash device table */
#define FAL_FLASH_DEV_TABLE                                          \
{                                                                    \
    &t32cz20_onchip_flash,                                                     \
}
/* ====================== Partition Configuration ========================== */
#ifdef FAL_PART_HAS_TABLE_CFG
/* partition table */
#define FAL_PART_TABLE                                 \
{                                                      \
  {FAL_PART_MAGIC_WORD, "dummy1",  "dummy1",  0, 0, 0},\
  {FAL_PART_MAGIC_WORD, "dummy2",  "dummy1",  0, 0, 0},\
  {FAL_PART_MAGIC_WORD, "dummy3",  "dummy1",  0, 0, 0},\
}
#endif /* FAL_PART_HAS_TABLE_CFG */

#endif /* _FAL_CFG_H_ */
