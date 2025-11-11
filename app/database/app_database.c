/*
 * SPDX-License-Identifier: BSD-3-Clause
 * SPDX-FileCopyrightText: 2024 Silicon Laboratories Inc. <https://www.silabs.com/>
 * SPDX-FileCopyrightText: 2025 Card Access Engineering, LLC <https://www.caengineering.com/>
 */
/**
 * @file app_database.c
 * @brief Non-volatile memory User and User Schedule database implementation
 * 
 *        This file was created by extracting the database file setup and operation
 *        functions from the cc_user_credential_nvm file, then the CC_UserCredential
 *        functions were then placed into a separate module for better code sharing
 *        between this database module and the ASCC handling functions.
 *        It's better that these functions exist in the application layer and not the
 *        SDK.
 *
 * @copyright 2023 Silicon Laboratories Inc.
 * @copyright 2025 Card Access Engineering, LLC
 */

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/

#include "app_database.h"
#include "cc_user_credential_io.h"
#include "cc_user_credential_io_config.h"
#include "ZAF_file_ids.h"
#include "ZAF_nvm.h"
#include "cc_user_credential_config_api.h"
#include "assert.h"
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

/****************************************************************************/
/*                             STATIC VARIABLES                             */
/****************************************************************************/

/****************************************************************************/
/*                           GENERAL API FUNCTIONS                          */
/****************************************************************************/

bool app_nvm(
  app_nvm_operation operation, app_nvm_area area, uint16_t offset, void* pData,
  uint16_t size)
{
  // Set parameters depending on the NVM area
  zpal_nvm_object_key_t file_base;
  switch (area) {
    /**********************/
    /* Known size objects */
    /**********************/
    case AREA_NUMBER_OF_USERS:
      file_base = ZAF_FILE_ID_CC_USER_CREDENTIAL_NUMBER_OF_USERS;
      size = sizeof(uint16_t);
      offset = 0;
      break;

    case AREA_NUMBER_OF_CREDENTIALS:
      file_base = ZAF_FILE_ID_CC_USER_CREDENTIAL_NUMBER_OF_CREDENTIALS;
      size = sizeof(uint16_t);
      offset = 0;
      break;

    case AREA_USER_DESCRIPTORS:
      file_base = ZAF_FILE_ID_CC_USER_CREDENTIAL_USER_DESCRIPTOR_TABLE;
      size = sizeof(user_descriptor_t) * app_db_get_num_users();
      offset = 0;
      break;

    case AREA_CREDENTIAL_DESCRIPTORS:
      file_base = ZAF_FILE_ID_CC_USER_CREDENTIAL_CREDENTIAL_DESCRIPTOR_TABLE;
      size = sizeof(credential_descriptor_t) * app_db_get_num_creds();
      offset = 0;
      break;

    case AREA_USERS:
      file_base = ZAF_FILE_ID_CC_USER_CREDENTIAL_USER_BASE;
      size = sizeof(u3c_user_t);
      break;

    case AREA_CREDENTIAL_METADATA:
      file_base = ZAF_FILE_ID_CC_USER_CREDENTIAL_CREDENTIAL_BASE;
      size = sizeof(credential_metadata_nvm_t);
      break;

    case AREA_ADMIN_PIN_CODE_DATA:
      file_base = ZAF_FILE_ID_ADMIN_PIN_CODE;
      size = sizeof(admin_pin_code_metadata_nvm_t);
      break;
    
    case AREA_SCHEDULE_DATA:
      file_base = ZAF_FILE_ID_CC_USER_CREDENTIAL_SCHEDULE_BASE;
      size = sizeof(schedule_metadata_nvm_t);
      break;

    /************************/
    /* Dynamic size objects */
    /************************/
    case AREA_CREDENTIAL_DATA:
      file_base = ZAF_FILE_ID_CC_USER_CREDENTIAL_CREDENTIAL_DATA_BASE;
      break;

    case AREA_USER_NAMES:
      file_base = ZAF_FILE_ID_CC_USER_CREDENTIAL_USER_NAME_BASE;
      break;

    default:
      return false;
  }

  if (size == 0) {
    return true;
  }

  zpal_status_t nvm_result = ZPAL_STATUS_FAIL;
  switch (operation) {
    case U3C_READ:
      nvm_result = ZAF_nvm_read(file_base + offset, pData, (size_t)size);
      break;
    case U3C_WRITE:
      nvm_result = ZAF_nvm_write(file_base + offset, pData, (size_t)size);
      break;
    default:
      break;
  }
  return nvm_result == ZPAL_STATUS_OK;
}
