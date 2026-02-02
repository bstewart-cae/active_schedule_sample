/*
 * SPDX-FileCopyrightText: 2024 Silicon Laboratories Inc. <https://www.silabs.com/>
 * SPDX-FileCopyrightText: 2026 Z-Wave Alliance <https://www.z-wavealliance.org/>
 * SPDX-FileCopyrightText: 2026 Card Access Engineering, LLC <https://www.caengineering.com/>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

/**
 * @file
 * Non-volatile memory implementation for Command Class User Credential I/O
 * 
 * @note Some application-level NVM behavior was introduced alongside U3Cv2.
 *       This file now defines the callback structure for those hooks, and 
 *       provides a framework for adding more.
 *
 * @copyright 2024 Silicon Laboratories Inc.
 */

#ifndef CC_USER_CREDENTIAL_NVM
#define CC_USER_CREDENTIAL_NVM

#include "CC_UserCredential.h"
#include "cc_user_credential_config.h"

/****************************************************************************/
/*                          CONSTANTS and TYPEDEFS                          */
/****************************************************************************/

// Maximum number of User and User Name objects that can be stored in the NVM
#define MAX_USER_OBJECTS                   \
  ZAF_FILE_ID_CC_USER_CREDENTIAL_USER_LAST \
  - ZAF_FILE_ID_CC_USER_CREDENTIAL_USER_BASE
/**
 * Maximum number of Credential and Credential Data objects that can be stored
 * in the NVM
 */
#define MAX_CREDENTIAL_OBJECTS                   \
  ZAF_FILE_ID_CC_USER_CREDENTIAL_CREDENTIAL_LAST \
  - ZAF_FILE_ID_CC_USER_CREDENTIAL_CREDENTIAL_BASE

/**
 * Credential metadata object for storage in NVM.
 */
typedef struct credential_metadata_nvm_t_ {
  uint16_t uuid;
  uint16_t modifier_node_id;
  uint8_t length;
  u3c_modifier_type modifier_type;
} credential_metadata_nvm_t;

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

typedef enum u3c_nvm_operation_ {
  U3C_READ,
  U3C_WRITE
} u3c_nvm_operation;

typedef enum u3c_nvm_area_ {
  AREA_NUMBER_OF_USERS,
  AREA_NUMBER_OF_CREDENTIALS,
  AREA_USER_DESCRIPTORS,
  AREA_USERS,
  AREA_USER_NAMES,
  AREA_CREDENTIAL_DESCRIPTORS,
  AREA_CREDENTIAL_METADATA,
  AREA_CREDENTIAL_DATA,
  AREA_ADMIN_PIN_CODE_DATA,
} u3c_nvm_area;

/*
 * Some database manipulation functions need to have exposure to multiple modules
 * depending on what particular features of the U3C are supported by the end device.
 */

 /* Function definitions for app layer hooks */

/**
 * @brief This callback is used when a User entry is manipulated in the database.
 *        The application can then attach logic to run when a User entry is changed.
 * 
 * @note  The reason for this approach over a ZAF event is to ensure that all NVM operations
 *        will run in the same context.
 * 
 * @param uuid User that was changed
 * @param operation The operation performed on the user
 */
typedef void (*u3c_nvm_user_added_or_deleted_cb_t)(
  const uint16_t uuid,
  const u3c_operation_type_t operation
);

/**
 * @brief These callbacks are available so that an application developer can attach
 *        hooks to various database operations.
 * 
 * @note  The only reason that function pointers are used rather than weak, overrideable functions is because
 *        they are intended to be implemented by the application, and that is not always clear to
 *        users of the SDK. This also allows greater flexibility by changing callbacks at runtime as needed.
 */
typedef struct u3c_nvm_callbacks_ {
  u3c_nvm_user_added_or_deleted_cb_t user_changed;
} u3c_nvm_cbs_t;

/****************************************************************************/
/*                               API FUNCTIONS                              */
/****************************************************************************/

/**
 * @brief  Execute an NVM read or write operation for application device.
 *
 * @return true if the operation has been executed successfully and more than 0
 *         bytes were transferred
 */
bool u3c_nvm(
  const u3c_nvm_operation operation, const u3c_nvm_area area, uint16_t offset, void* pData,
  uint16_t size);

/**
 * @brief  Get the file ID offset of a given User Unique ID.
 *
 * @param      uuid User Unique ID to find
 * @param[out] offset Optional pointer in which to store the page offset of the given user 
 *                    in the database region
 * @return     true if the user ID exists in the database.
 */
bool u3c_nvm_get_user_offset_from_id(const uint16_t uuid, uint16_t * offset);

/** 
 * @brief Get current number of user entries in the database
 * 
 * @return Number of existing User entries in the database
 */
uint16_t u3c_nvm_get_num_users(void);

/** 
 * @brief Get current number of credential entries in the database
 * 
 * @return Number of existing credential entries in the database
 */
uint16_t u3c_nvm_get_num_creds(void);

/**
 * @brief Get the maximum number of users allowed in the database
 * 
 * @return Maximum number of User Slots allowed in the database
 */
uint16_t u3c_nvm_get_max_users(void);

/**
 * @brief Called by application to register app-level callbacks for different events
 *        within the u3c_nvm module.
 */
void u3c_nvm_register_cbs(const u3c_nvm_cbs_t * const callbacks);

/* To add: Key Locker, Time, User Code, Migration, etc. */

#endif /* CC_USER_CREDENTIAL_NVM */
