/*
 * SPDX-FileCopyrightText: 2024 Silicon Laboratories Inc. <https://www.silabs.com/>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */

/**
 * @file
 * User Credential Command Class configuration API.
 *
 * @copyright 2024 Silicon Laboratories Inc.
 */

#ifndef CC_USER_CREDENTIAL_CONFIG_API_H
#define CC_USER_CREDENTIAL_CONFIG_API_H

#include "CC_UserCredential.h"

/**
 * Checks whether the application is configured to support a specific user type.
 *
 * @param[in] user_type The requested user type
 * @return true if this user type is supported
 */
bool cc_user_credential_is_user_type_supported(u3c_user_type user_type);

/**
 * Gets the configured maximum number of supported user unique identifiers.
 *
 * @return The maximum number of supported UUIDs
 */
uint16_t cc_user_credential_get_max_user_unique_identifiers(void);

/**
 * Gets the configured maximum user name length.
 *
 * @return The maximum length of a user name (in bytes)
 */
uint8_t cc_user_credential_get_max_length_of_user_name(void);

/**
 * Checks whether the application is configured to support a specific credential type.
 *
 * @param[in] credential_type The specific credential type
 * @return true if this credential type is supported
 */
bool cc_user_credential_is_credential_type_supported(u3c_credential_type credential_type);

/**
 * Checks whether the Credential Learn functionality is supported for a specific
 * credential type by the application.
 *
 * @param[in] credential_type The specific credential type
 * @return true if Credential Learn is supported for this credential type
 */
bool cc_user_credential_is_credential_learn_supported(u3c_credential_type credential_type);

/**
 * Checks whether a specific credential rule is supported by the application.
 *
 * @param[in] credential_rule The specific credential rule
 * @return true if this credential rule is supported
 */
bool cc_user_credential_is_credential_rule_supported(u3c_credential_rule credential_rule);

/**
 * Gets the number of credential types configured as supported by the application.
 *
 * @return The number of supported credential types
 */
uint8_t cc_user_credential_get_number_of_supported_credential_types(void);

/**
 * Gets the configured maximum number of slots for a specific credential type.
 *
 * @param[in] credential_type The specific credential type
 * @return The maximum number of credential slots for this credential type
 */
uint16_t cc_user_credential_get_max_credential_slots(u3c_credential_type credential_type);

/**
 * Gets the configured minimum length of the credential data for a specific credential type.
 *
 * @param[in] credential_type The specific credential type
 * @return The minimum allowed data length for this credential type (in bytes)
 */
uint8_t cc_user_credential_get_min_length_of_data(u3c_credential_type credential_type);

/**
 * Gets the configured maximum length of the credential data for a specific credential type.
 *
 * @param[in] credential_type The specific credential type
 * @return The maximum allowed data length for this credential type (in bytes)
 */
uint8_t cc_user_credential_get_max_length_of_data(u3c_credential_type credential_type);

/**
 * Gets the configured maximum hash length for a specific credential type.
 * If the credential type is supported by the application, a maximum hash length
 * of 0 implies that credentials of this type must always be read back when sent
 * in a report.
 *
 * @param[in] type The specific credential type
 * @return The maximum allowed hash length for this credential type (in bytes),
 *         if supported
 */
uint8_t cc_user_credential_get_max_hash_length(u3c_credential_type type);

/**
 * Gets the configured recommended Credential Learn timeout for a specific credential type.
 *
 * @param[in] credential_type The specific credential type
 * @return The recommended Credential Learn timeout (in seconds)
 */
uint8_t cc_user_credential_get_cl_recommended_timeout(u3c_credential_type credential_type);

/**
 * Gets the number of steps required to complete the Credential Learn process
 * for a specific credential type.
 *
 * @param[in] credential_type The specific credential type
 * @return The number of steps required to complete the Credential Learn process
 */
uint8_t cc_user_credential_get_cl_number_of_steps(u3c_credential_type credential_type);

/**
 * Returns whether User Scheduling is supported via Active Schedule CC.
 *
 * @return true if scheduling is supported, false otherwise.
 */
bool cc_user_credential_is_user_scheduling_supported(void);

/**
 * Returns whether All Users Checksum is supported.
 *
 * @return true if All Users Checksum is supported, false otherwise.
 */
bool cc_user_credential_is_all_users_checksum_supported(void);

/**
 * Checks whether the User Checksum functionality is supported by the application.
 *
 * @return true if User Checksum is supported
 */
bool cc_user_credential_is_user_checksum_supported(void);

/**
 * Checks whether the Credential Checksum functionality is supported by the application.
 *
 * @return true if Credential Checksum is supported
 */
bool cc_user_credential_is_credential_checksum_supported(void);

/**
 * Checks whether the Admin Code functionality is supported by the application.
 *
 * @return true if Admin Code is supported
 */
bool cc_user_credential_get_admin_code_supported(void);

/**
 * Checks whether the Admin Code Deactivate functionality is supported by the application.
 *
 * @return true if Admin Code Deactivate is supported
 */
bool cc_user_credential_get_admin_code_deactivate_supported(void);

/**
 * @brief Retrieves the number of Year Day schedules each user supports.
 * This value is the same for all users.
 *
 * @return Number of Year Day schedules per User slot
 */
uint16_t cc_user_credential_get_num_year_day_per_user(void);

/**
 * @brief Retrieves the number of Daily Repeating schedules each user supports.
 * This value is the same for all users.
 *
 * @return Number of Daily Repeating schedules per User slot
 */
uint16_t cc_user_credential_get_num_daily_repeating_per_user(void);

/**
 * @brief  Returns the configured number of key locker slots for a given slot type
 *
 * @return Returns number of slots for the slot type,
 *         0 if the slot type or Key Locker functionality are not supported.
 */
uint16_t cc_user_credential_get_key_locker_slot_count(const u3c_kl_slot_type_t slot_type);

/**
 * @brief  Returns the maximum allowed data length for a given slot type
 *
 * @return Returns maximum allowed data length for the slot type,
 *         0 if the slot type or Key Locker functionality are not supported.
 */
uint16_t cc_user_credential_get_key_locker_min_data_length(const u3c_kl_slot_type_t slot_type);

/**
 * @brief  Returns the minimum allowed data length for a given slot type
 *
 * @return Returns minimum allowed data length for the slot type,
 *         0 if the slot type or Key Locker functionality are not supported.
 */
uint16_t cc_user_credential_get_key_locker_max_data_length(const u3c_kl_slot_type_t slot_type);


#endif /* CC_USER_CREDENTIAL_CONFIG_API_H */
