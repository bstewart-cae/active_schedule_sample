/*
 * SPDX-License-Identifier: BSD-3-Clause
 * SPDX-FileCopyrightText: 2025 Card Access Engineering, LLC <https://www.caengineering.com/>
 */
/**
 * @file app_database_ascc.c
 *
 * @date 2025-11-10
 * @author bstewart (Card Access, Inc.)
 *
 * @brief
 */

/****************************************************************************
 *                              INCLUDE FILES                               *
 ****************************************************************************/
// Module includes should always go first and be listed alphabetically
#include "app_database.h"
#include "cc_active_schedule_config_api.h"
#include "cc_user_credential_config_api.h"
#include "cc_user_credential_io.h"


// Stack/SDK includes should always go second to last and be listed alphabetically

// Language includes should always go last and be listed alphabetically

/****************************************************************************
 *                     PRIVATE TYPES AND DECLARATIONS                       *
 ****************************************************************************/

/****************************************************************************
 *                               PRIVATE DATA                               *
 ****************************************************************************/

/****************************************************************************
 *                     PRIVATE FUNCTION DECLARATIONS                        *
 ****************************************************************************/
static uint16_t app_db_get_schedule_count(const ascc_type_t schedule_type);
static uint16_t app_db_get_target_count(void);
static bool app_db_validate_target(const ascc_target_t * const target);
static bool app_db_validate_schedule_slot(const uint16_t target_ID,
                                          const ascc_type_t type,
                                          const uint16_t slot);
static bool app_db_validate_schedule_data(const ascc_schedule_t * const schedule);
static ascc_op_result_t app_db_get_schedule_state(const ascc_target_t * const target,
                                                  bool * state);
static ascc_op_result_t app_db_set_schedule_state(const ascc_target_t * const target,
                                                  const bool state);
static ascc_op_result_t app_db_get_schedule_data(const ascc_type_t schedule_type,
                                                 const uint16_t slot,
                                                 const ascc_target_t * const target,
                                                 ascc_schedule_data_t * schedule,
                                                 uint16_t * next_slot);
static ascc_op_result_t app_db_set_schedule_data(const ascc_op_type_t operation,
                                                 const ascc_schedule_t * const schedule,
                                                 uint16_t * next_slot);

/****************************************************************************
 *                     EXPORTED FUNCTION DEFINITIONS                        *
 ****************************************************************************/

/****************************************************************************
 *                     PRIVATE FUNCTION DEFINITIONS                         *
 ****************************************************************************/

 /**
 * @brief Gets the number of supported schedules per target for a given schedule type.
 *
 * @param schedule_type Schedule type (Year Day/ Daily Repeating)
 * @return Number of supported schedules per target
 */
uint16_t app_db_get_schedule_count(const ascc_type_t schedule_type)
{
    if (schedule_type == ASCC_TYPE_DAILY_REPEATING) {
        return MAX_DAILY_REPEATING_SCHEDULES_PER_USER;
    } else if (schedule_type == ASCC_TYPE_YEAR_DAY) {
        return MAX_YEAR_DAY_SCHEDULES_PER_USER;
    }
    return 0;
}

/**
 * @brief Gets the number of supported targets supported by the registered CC.
 *
 * @return Number of supported targets for the registered CC handler.
 */
uint16_t app_db_get_target_count(void) {
    return cc_user_credential_get_max_user_unique_idenfitiers();
}

/**
 * @brief Verifies that given target is valid per the specification. Subset of the schedule validation
 * operation but this is more intended for use in Gets where the schedule information is unknown.
 *
 * @param target Const pointer to target to be verified
 *
 * @return true if target is valid.
 *         false if target is invalid.
 */
bool app_db_validate_target(const ascc_target_t * const target) {
    if (!target) {
        return false;
    } else if (target->target_cc != COMMAND_CLASS_USER_CREDENTIAL) {
        return false;
    } else if (target->target_id >= cc_user_credential_get_max_user_unique_idenfitiers()) {
        return false;
    } else {
        // Just return if the user exists in the database or not
        return app_db_get_user_offset_from_id(target->target_id,NULL); 
    }
}

/**
 * @brief Verifies that given schedule slot is valid for the registered CC and given type.
 *
 * @param target_ID   Target ID
 * @param type        Schedule type (Year Day/Daily Repeating)
 * @param slot        Schedule slot
 *
 * @return true if target slot is valid.
 *         false if target slot is invalid.
 */
bool app_db_validate_schedule_slot(const uint16_t target_ID,
                                   const ascc_type_t type,
                                   const uint16_t slot)
{
    if (type == ASCC_TYPE_DAILY_REPEATING) {
        return slot < MAX_DAILY_REPEATING_SCHEDULES_PER_USER;
    } else if (type == ASCC_TYPE_YEAR_DAY) {
        return slot < MAX_YEAR_DAY_SCHEDULES_PER_USER;
    }
    return false;
}

/**
 * @brief Verifies whether provided *incoming* schedule data is valid.
 *
 * Only true/false is needed as a response, as any command with an invalid schedule and/or target
 * are simply ignored.
 *
 * @param schedule Const pointer to schedule to be verified
 *
 * @return true if schedule, target and metadata is valid.
 *         false otherwise.
 */
bool app_db_validate_schedule_data(const ascc_schedule_t * const schedule)
{
    (void*)schedule;
    return false;
}

/**
 * @brief Gets the current state of the schedules attached to the target.
 *
 * @param      target   Const pointer to target
 * @param[out] state    Output pointer to be populated with the state of the schedule -
 *                      true for enabled, false for disabled.
 *
 * @returns ascc_io_op_result_t as follows:
 *          ASCC_OPERATION_RESULT_SUCCESS for successful get
 *          ASCC_OPERATION_RESULT_WORKING for successful initiation of get process.
 *              Application assumes responsibility for appropriate report handling.
 *          ASCC_OPERATION_RESULT_FAILURE for unsuccessful get operation. Consult end node documentation.
 */
ascc_op_result_t app_db_get_schedule_state(const ascc_target_t * const target,
                                           bool * state)
{
    ascc_op_result_t result = {
        .result = ASCC_OPERATION_FAIL
    };
    uint16_t page = 0;
    if (app_db_get_user_offset_from_id(target->target_id, &page)) {
        
    }

    return result;
}

/**
 * @brief Sets the state of the schedules attached to a target.
 *
 * @param target   Const pointer to target
 * @param state    Boolean input - True to enable all schedules attached to a target,
 *                                 False to disable all schedules attached to a target.
 *
 * @return ascc_io_op_result_t as follows:
 *         ASCC_OPERATION_RESULT_SUCCESS for successful set
 *         ASCC_OPERATION_RESULT_WORKING for successful initiation of set process.
 *             Application assumes responsibility for appropriate report handling.
 *         ASCC_OPERATION_RESULT_FAILURE for unsuccessful set operation. Consult end node documentation.
 */
ascc_op_result_t app_db_set_schedule_state(__attribute__((unused))const ascc_target_t * const target,
                                           __attribute__((unused))const bool state)
{
    ascc_op_result_t result = {
        .result = ASCC_OPERATION_FAIL
    };

    return result;
}

/**
 * @brief Gets the schedule data for a given type and slot number.
 *
 * @param schedule_type  Year Day or Daily Repeating
 * @param slot           Slot Index
 * @param target         Pointer to Target
 * @param[out] schedule  Pointer to schedule structure in which schedule data is returned
 * @param[out] next_slot Pointer with which to populate the next occupied schedule slot
 *                       for that target, or 0 if there are none.
 *
 * @return ascc_io_op_result_t as follows:
 *         ASCC_OPERATION_RESULT_SUCCESS for successful get
 *         ASCC_OPERATION_RESULT_WORKING for successful initiation of get process.
 * `            Application assumes responsibility for appropriate report handling.
 *         ASCC_OPERATION_RESULT_FAILURE for unsuccessful get operation. Consult end node documentation.
 */
ascc_op_result_t app_db_get_schedule_data(__attribute__((unused))const ascc_type_t schedule_type,
                                          __attribute__((unused))const uint16_t slot,
                                          __attribute__((unused))const ascc_target_t * const target,
                                          __attribute__((unused))ascc_schedule_data_t * schedule,
                                          __attribute__((unused))uint16_t * next_slot)
{
   ascc_op_result_t result = {
        .result = ASCC_OPERATION_FAIL
    };

    return result;
}

/**
 * @brief Sets schedule slot information, either to modify and attach to a target or
 *        clear the slot.
 *
 * @param[in]  operation Which operation to perform on the schedule slot
 * @param[in]  schedule  Constant pointer to schedule data
 * @param[out] next_slot Pointer with which to populate the next occupied schedule slot
 *                       for that target, or 0 if there are none.
 *
 * @return ascc_io_op_result_t as follows:
 *         ASCC_OPERATION_RESULT_SUCCESS for successful set
 *         ASCC_OPERATION_RESULT_WORKING for successful initiation of set process.
 * `            Application assumes responsibility for appropriate report handling.
 *         ASCC_OPERATION_RESULT_FAILURE for unsuccessful set operation. Consult end node documentation.
 */
ascc_op_result_t app_db_set_schedule_data(__attribute__((unused))const ascc_op_type_t operation,
                                          __attribute__((unused))const ascc_schedule_t * const schedule,
                                          __attribute__((unused))uint16_t * next_slot)
{
   ascc_op_result_t result = {
        .result = ASCC_OPERATION_FAIL
    };

    return result;
}