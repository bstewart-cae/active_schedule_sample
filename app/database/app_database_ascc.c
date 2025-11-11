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
 * @brief This defines the stubs for User targets for the Active Schedule CC.
 * This also interfaces with the NVM database to track and store this information.
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

#define MAX_HOUR_COUNTER   (23)   ///< Hours will never be more than this value
#define MAX_MINUTE_COUNTER (59)   ///< Minutes will never be more than this value
#define MAX_MONTH_COUNTER  (12)   ///< Month value will never be more than this value
#define FEB_LEAP_YEAR_DAYS (29)   ///< Number of days in February during leap years
#define FEB_INDEX          (2)    ///< February month index
#define LEAP_YEAR_CADENCE  (4)    ///< Number of days in February during leap years
#define MAX_WEEKDAY_MASK   (0x7F) ///< Maximum value for weekday mask

/* 
 * This is packed to fill the same format as the YD schedules.
 * The schedules are defined as loose values still because there's
 * no standard way to represent time or time stamps, and we don't want
 * to add unnecessary steps to any other application
 */
#pragma pack(push, 1)
typedef struct ascc_time_stamp_ {
    uint16_t year;   ///< Gregorian Year
    uint8_t  month;  ///< January - 1, Feb. - 2, etc. 0 is unused and considered erroneous
    uint8_t  day;    ///< Calendar day, 1-31
    uint8_t  hour;   ///< Hour in 24h time (0-23)
    uint8_t  minute; ///< Minute (0-59)
} ascc_time_stamp_t;
#pragma pack(pop)

/****************************************************************************
 *                               PRIVATE DATA                               *
 ****************************************************************************/

/**
 * @brief Simple map (month - 1) containing the value of the number of days in the month
 */
static const uint8_t day_count[] = {
    0,  ///< Unused
    31, ///< January
    28, ///< February (Check for leap year)
    31, ///< March
    30, ///< April
    31, ///< May
    30, ///< June
    31, ///< July
    31, ///< August
    30, ///< September
    31, ///< October
    30, ///< November
    31 ///< December
};



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
static bool is_time_stamp_valid(const ascc_time_stamp_t * const timestamp);
static bool is_time_fence_valid(const ascc_time_stamp_t * const start, 
                                const ascc_time_stamp_t * const end);
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
    // Schedule data is unused
    if (schedule->data.metadata_length > 0) {
        return false;
    }
    bool response = true;
    // Check schedule time values here
    if (schedule->type == ASCC_TYPE_DAILY_REPEATING) {
        ascc_daily_repeating_schedule_t * dr_schedule = 
            &schedule->data.schedule.daily_repeating;
        /* Make sure that all of the values are in appropriate ranges */
        if (dr_schedule->duration_hour > MAX_HOUR_COUNTER ||
              dr_schedule->duration_minute > MAX_MINUTE_COUNTER ||
              dr_schedule->start_hour > MAX_HOUR_COUNTER ||
              dr_schedule->start_minute > MAX_MINUTE_COUNTER ||
              dr_schedule->weekday_mask > MAX_WEEKDAY_MASK) {
            response = false;
        }
    } else if (schedule->type == ASCC_TYPE_YEAR_DAY) {
        ascc_year_day_schedule_t * yd_schedule = &schedule->data.schedule;
        const ascc_time_stamp_t * start = (ascc_time_stamp_t*)&yd_schedule->start_year;
        const ascc_time_stamp_t * end = (ascc_time_stamp_t*)&yd_schedule->stop_year;
        // Verifying the time fence also verifies the individual time stamps
        response = is_time_fence_valid(start, end);
    } else {
        // Don't want roque enumerator values in here
        return false;
    }
    return response;
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

/**
 * @brief Checks if a given time stamp is valid
 * 
 * @param timestamp Timestamp to check for validity
 * @return True if valid, false if not
 */
static bool is_time_stamp_valid(const ascc_time_stamp_t * const timestamp)
{
    bool result = true;
    if (!timestamp) {
        return false;
    }
    // Year is always valid, so we skip that
    if (timestamp->month == 0 || timestamp->month > MAX_MONTH_COUNTER) {
        return false;
    }
    // Get target day count based on month
    uint8_t days = day_count[timestamp->month];
    // Make sure this isn't february during a leap year
    if (timestamp->month == FEB_INDEX && 
          timestamp->year % LEAP_YEAR_CADENCE) {
        days = FEB_LEAP_YEAR_DAYS;
    }
    // Check remaining values
    if (timestamp->day == 0 
          || timestamp->day > days 
          || timestamp->hour > MAX_HOUR_COUNTER
          || timestamp->minute > MAX_MINUTE_COUNTER) {
        result = false;
    }
    return result;
}

/**
 * @brief Checks if a given time fence is valid. Both time stamps must be 
 *        valid time stamps and the end must be after the start.
 * 
 * @note Daylight savings is not taken into account here - this is just for 
 *       reference. You're on your own with that one in your particular application.
 * 
 * @param timestamp Timestamp to check for validity
 * @return True if valid, false if not
 */
static bool is_time_fence_valid(const ascc_time_stamp_t * const start,
                                const ascc_time_stamp_t * const end)
{
    bool result = false;
    if (is_time_stamp_valid(start) && is_time_stamp_valid(end)) {
        if (start->year < end->year) {
            result = true;
        } else if (start->year == end->year) {
            if (start->month < end->month) {
                result = true;
            } else if (start->month == end->month) {
                if (start->day < end->day) {
                    result == true;
                } else if (start->day == end->day) {
                    if (start->hour < end->hour) {
                        result == true;
                    } else if (start-> hour == end->hour) {
                        if (start->minute < end->minute) {
                            result == true;
                        } // If time stamps are equal, then this check fails
                    }
                }
            }
        }
    }
    return result;
}