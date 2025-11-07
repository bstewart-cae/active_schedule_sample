/**
 * @file firmware_properties.c
 *
 * This file defines information used by the bootloader and by ZPAL.
 *
 * The file must be compiled together with the application because
 * it uses defines that must be set for each application.
 *
 * SPDX-License-Identifier: LicenseRef-TridentMSLA
 * SPDX-FileCopyrightText: 2024 Trident IoT, LLC <https://www.tridentiot.com>
 */

#include "zw_version_config.h"
#include "zpal_defs.h"
#include "zaf_config.h"
#include "zaf_config_security.h"
#include "ZW_classcmd.h"

// This information is used by the bootloader.
const __attribute__((__used__, section(".fw_properties"))) app_version_info_t
   app_version_info = {.magic_word = MAGIC_WORD_STR,
                       .app_version =  {.app_version_major = APP_VERSION,
                       .app_version_minor = APP_REVISION,
                       .app_version_patch = APP_PATCH },
                       .manufacturer_id = ZAF_CONFIG_MANUFACTURER_ID,
                       .product_type_id = ZAF_CONFIG_PRODUCT_TYPE_ID,
                       .product_id      = ZAF_CONFIG_PRODUCT_ID
                      };

// This information is used by zpal_get_product_id().
const uint16_t ZPAL_PRODUCT_ID_INSTALLER_ICON_TYPE = ZAF_CONFIG_INSTALLER_ICON_TYPE;
const uint8_t ZPAL_PRODUCT_ID_GENERIC_TYPE = ZAF_CONFIG_GENERIC_TYPE;
const uint8_t ZPAL_PRODUCT_ID_SPECIFIC_TYPE = ZAF_CONFIG_SPECIFIC_TYPE;
const uint8_t ZPAL_PRODUCT_ID_REQUESTED_SECURITY_KEYS = ZAF_CONFIG_REQUESTED_SECURITY_KEYS;
