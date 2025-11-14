/*
 * SPDX-License-Identifier: BSD-3-Clause
 * SPDX-FileCopyrightText: 2024 Silicon Laboratories Inc. <https://www.silabs.com/>
 * SPDX-FileCopyrightText: 2025 Card Access Engineering, LLC <https://www.caengineering.com/>
 */
/**
 * @file app_database.h
 * @brief Non-volatile memory implementation for Command Class User Credential I/O
 *        Modified from original cc_user_credential_nvm implementation to also include
 *        schedule information.
 *
 * @copyright 2024 Silicon Laboratories Inc.
 * @copyright 2025 Card Access Engineering, LLC
 */

/*
 * Verify that we're only pulling in one implementation of the
 * nvm database.
 */
#ifdef CC_USER_CREDENTIAL_NVM
#error "User Credential database already defined, check includes
#endif

#ifndef CC_USER_CREDENTIAL_NVM
#define CC_USER_CREDENTIAL_NVM

#include "database_properties.h" 
#include "CC_ActiveSchedule.h"
#include "cc_active_schedule_io.h"
#include "cc_active_schedule_config.h"
#include "CC_UserCredential.h"
#include "cc_user_credential_config.h"

/****************************************************************************/
/*                          CONSTANTS and TYPEDEFS                          */
/****************************************************************************/

/**
 * Credential metadata object for storage in NVM.
 */
typedef struct credential_metadata_nvm_t_ {
  uint16_t uuid;
  uint16_t modifier_node_id;
  uint8_t length;
  u3c_modifier_type modifier_type;
} credential_metadata_nvm_t;

#pragma pack(push, 1)
/**
 * @brief Packs relevant Year Day schedule information into single struct 
 */
typedef struct year_day_nvm_ {
  bool occupied;
  ascc_year_day_schedule_t schedule;
} year_day_nvm_t;

/**
 * @brief Packs relevant Daily Repeating schedule information into single struct
 */
typedef struct daily_repeating_nvm_ {
  bool occupied;
  ascc_daily_repeating_schedule_t schedule;
} daily_repeating_nvm_t;
#pragma pack(pop)

/**
 * Schedule metadata object for storage in NVM.
 * 
 * This contains all of the schedule information for a given User.
 * 
 * @note The schedules are stored in zero-indexed arrays - a schedule 
 * 'slot' will correspond to its array index + 1 as slots are 1-indexed.
 */
typedef struct schedule_metadata_nvm_t_ {
  uint16_t uuid;
  bool scheduling_active;
  year_day_nvm_t year_day_schedules[MAX_YEAR_DAY_SCHEDULES_PER_USER]; 
  daily_repeating_nvm_t daily_repeating_schedules[MAX_DAILY_REPEATING_SCHEDULES_PER_USER];
} schedule_metadata_nvm_t;

/**
 * The User descriptor table is an array of associations between User Unique IDs
 * and file IDs of User objects.
 * Only the entries from ZAF_FILE_ID_CC_USER_CREDENTIAL_CREDENTIAL_BASE to
 * ZAF_FILE_ID_CC_USER_CREDENTIAL_CREDENTIAL_BASE + n_users - 1 are to be
 * considered valid.
 */
typedef struct user_descriptor_t_ {
  uint16_t unique_identifier;
  uint16_t object_offset;
} user_descriptor_t;

/**
 * The Credential descriptor table is an array of associations between unique
 * Credentials and file IDs of Credential metadata objects. A Credential is
 * identified by its owner's User Unique ID and the Credential's type and slot.
 * Only the entries from ZAF_FILE_ID_CC_USER_CREDENTIAL_USER_BASE to
 * ZAF_FILE_ID_CC_USER_CREDENTIAL_USER_BASE + n_credentials - 1 are to be
 * considered valid.
 */
typedef struct credential_descriptor_t_ {
  uint16_t user_unique_identifier;
  uint16_t credential_slot;
  uint16_t object_offset;
  u3c_credential_type credential_type;
} credential_descriptor_t;

/**
 * @brief Metadata to track current admin code information
 */
typedef struct admin_pin_code_metadata_nvm_t_ {
  uint8_t code_length; // Admin Code functionality disabled if 0
  uint8_t code[CC_USER_CREDENTIAL_MAX_DATA_LENGTH_PIN_CODE];
} admin_pin_code_metadata_nvm_t;

typedef enum app_nvm_operation_ {
  U3C_READ,
  U3C_WRITE
} app_nvm_operation;

typedef enum app_nvm_area_ {
  AREA_NUMBER_OF_USERS,
  AREA_NUMBER_OF_CREDENTIALS,
  AREA_USER_DESCRIPTORS,
  AREA_USERS,
  AREA_USER_NAMES,
  AREA_CREDENTIAL_DESCRIPTORS,
  AREA_CREDENTIAL_METADATA,
  AREA_CREDENTIAL_DATA,
  AREA_ADMIN_PIN_CODE_DATA,
  AREA_KEY_LOCKER_DATA,
  AREA_SCHEDULE_DATA,
} app_nvm_area;

/**
 * @brief  Execute an NVM read or write operation for application device.
 *
 * @return true if the operation has been executed successfully and more than 0
 *         bytes were transferred
 */
bool app_nvm(
  app_nvm_operation operation, app_nvm_area, uint16_t offset, void* pData,
  uint16_t size);

/**
 * @brief  Get the file ID offset of a given User Unique ID.
 *
 * @param      uuid User Unique ID to find
 * @param[out] offset Page offset of the given user in the database region
 * @return     true if the user ID exists in the database.
 */
bool app_db_get_user_offset_from_id(const uint16_t uuid, uint16_t * offset);

/** 
 * @brief Get current number of user entries in the database
 */
uint16_t app_db_get_num_users(void);

/** 
 * @brief Get current number of credential entries in the database
 */
uint16_t app_db_get_num_creds(void);

/**
 * @brief Active Schedule CC requires that ;
 */
void app_db_initialize_handlers(void);

/**
 * @brief Clear all stored schedule information
 */
void app_db_reset_schedules(void);

/**
 * @brief Reset 
 */

#endif /* CC_USER_CREDENTIAL_NVM */
