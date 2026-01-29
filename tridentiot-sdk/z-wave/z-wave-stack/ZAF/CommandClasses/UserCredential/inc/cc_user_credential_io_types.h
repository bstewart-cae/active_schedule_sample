/*
 * SPDX-FileCopyrightText: 2025 Card Access Engineering, LLC <https://www.caengineering.com/>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

/**
 * @file cc_user_credential_io_types.h
 * Data structures for the User Credential Command Class I/O.
 *
 * This allows us to add fields to the parameters and return values, or remove unused ones,
 * without changing the function signature - in theory, this makes code maintenance easier
 * and lower risk.
 *
 * @copyright 2023 Silicon Labs, Inc.
 * @copyright 2025 Card Access Engineering LLC
 */

#ifndef CC_USER_CREDENTIAL_IO_TYPES_H
#define CC_USER_CREDENTIAL_IO_TYPES_H

#include "CC_UserCredential.h"

/**
 * @addtogroup CC
 * @{
 * @addtogroup UserCredential
 * @{
 */

/****************************************************************************/
/*                      EXPORTED TYPES and DEFINITIONS                      */
/****************************************************************************/

typedef enum u3c_db_operation_result_ {
  U3C_DB_OPERATION_RESULT_SUCCESS,         ///< The operation completed succesfully
  U3C_DB_OPERATION_RESULT_ERROR,           ///< An error occurred
  U3C_DB_OPERATION_RESULT_ERROR_IO,        ///< An I/O error occurred
  U3C_DB_OPERATION_RESULT_ERROR_DUPLICATE, ///< Duplicate Entry in table
  U3C_DB_OPERATION_RESULT_FAIL_DNE,        ///< The object does not exist
  U3C_DB_OPERATION_RESULT_FAIL_FULL,       ///< There is no space left for the object
  U3C_DB_OPERATION_RESULT_FAIL_OCCUPIED,   ///< The object already exists
  U3C_DB_OPERATION_RESULT_FAIL_REASSIGN,   ///< The credential is assigned to a different user
  U3C_DB_OPERATION_RESULT_FAIL_IDENTICAL,  ///< The new data is identical to the data already stored locally
  U3C_DB_OPERATION_RESULT_WORKING = 0xFE,  ///< The operation has started and is running in parallel
} u3c_db_operation_result;

/*
 * Pack all structures in this file as tightly as possible so they can be
 * passed by value when needed.
 */
#pragma pack(push, 1)

/**
 * @brief This allows an IO operation to respond with both the result and/or
 *        working time if the the operation is running in parallel when returning
 *        to the radio context.
 */
typedef struct u3c_io_operation_status_ {
  u3c_db_operation_result result; ///< Result of the operation.
  uint8_t working_time;           ///< Set to zero or ignored if result != U3C_DB_OPERATION_RESULT_WORKING
} u3c_io_operation_status_t;

// Reset packing
#pragma pack(pop)

/* V2 Additions - Key Locker */

/**
 * @brief Input parameters for Key Locker Set
 */
typedef struct u3c_kl_set_input_ {
  RECEIVE_OPTIONS_TYPE_EX * rx_opts;  ///< RX options of the of the Z-Wave transaction that triggered the operation.
  const u3c_kl_slot_type_t slot_type; ///< Key Locker slot type.
  const uint16_t slot;                ///< Key Locker slot
  const uint8_t * const data;         ///< Key Locker entry data to store
  const uint16_t length;              ///< Key Locker entry data length
} u3c_kl_set_input_t;

/**
 * @brief Output parameters for Key Locker Set
 */
typedef struct u3c_kl_set_output_ {
  uint8_t reserved;     ///< Add outputs as needed.
} u3c_kl_set_output_t;

/**
 * @brief Input parameters for Key Locker Get operation
 */
typedef struct u3c_kl_get_input_ {
  RECEIVE_OPTIONS_TYPE_EX * rx_opts;  ///< RX options of the of the Z-Wave transaction that triggered the operation.
  const u3c_kl_slot_type_t slot_type; ///< Key Locker slot type.
  const uint16_t slot;                ///< Key Locker slot
} u3c_kl_get_input_t;

/**
 * @brief Output parameters for Key Locker Get operations
 */
typedef struct u3c_kl_get_output_ {
  u3c_kl_slot_type_t slot_type; ///< Key Locker slot type.
  uint16_t slot;                ///< Key Locker slot
  bool occupied;                ///< Whether or not the Key Locker slot contains data
} u3c_kl_get_output_t;

/**
 * @}
 * @}
 */

#endif /* CC_USER_CREDENTIAL_IO_H */
