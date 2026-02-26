/*
 * SPDX-FileCopyrightText: 2025 Z-Wave Alliance <https://z-wavealliance.org>
 * SPDX-FileCopyrightText: 2026 Card Access Engineering, LLC <http://www.caengineering.com>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file app_schedules.c
 * @author bstewart-cae
 * @brief This module defines the stubs for User targets for the Active Schedule CC.
 *        This also interfaces with the NVM database to track and store this information.
 *
 * @copyright 2026 Card Access Engineering, LLC on behalf of the Z-Wave Alliance
 */

#ifdef __cplusplus
extern "C" {
#endif

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
// Module includes should always go first and be listed alphabetically
#include "app_schedules.h"
#include "database_common.h"
#include "cc_user_credential_nvm.h"
#include "CC_ActiveSchedule.h"
#include "cc_active_schedule_config_api.h"
#include "cc_active_schedule_io.h"
#include "cc_user_credential_config_api.h"
#include "cc_user_credential_io.h"
#include "events.h"
// Stack/SDK includes should always go second to last and be listed alphabetically
#include <Assert.h>
//define DEBUGPRINT
#include "DebugPrint.h"
#include "zaf_event_distributor_soc.h"
// Language includes should always go last and be listed alphabetically
#include <string.h>

/****************************************************************************/
/*                      PRIVATE TYPES and DEFINITIONS                       */
/****************************************************************************/

/*
 * Verify that a non-zero number of schedules are supported if scheduling is
 * reported as supported or no schedules are supported if scheduling is not supported
 */
STATIC_ASSERT(
    !(((CC_USER_CREDENTIAL_YEAR_DAY_SCHEDULES_PER_USER + CC_USER_CREDENTIAL_DAILY_REPEATING_SCHEDULES_PER_USER) > 0) ^
    (CC_USER_CREDENTIAL_USER_SCHEDULING_SUPPORTED == 1)),
    STATIC_ASSERT_FAILED_Schedule_support_bit_and_number_of_supported_user_schedules_does_not_match
);



#define MAX_HOUR_COUNTER   (23)   ///< Hours will never be more than this value
#define MAX_MINUTE_COUNTER (59)   ///< Minutes will never be more than this value
#define MAX_MONTH_COUNTER  (12)   ///< Month value will never be more than this value
#define FEB_LEAP_YEAR_DAYS (29)   ///< Number of days in February during leap years
#define FEB_INDEX          (2)    ///< February month index
#define LEAP_YEAR_CADENCE  (4)    ///< Number of days in February during leap years
#define MAX_WEEKDAY_MASK   (0x7F) ///< Maximum value for weekday mask
#define LAST_OCCUPIED_SLOT (0xFF) ///< Signifier that no more occupied slots exist
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

/****************************************************************************/
/*                              PRIVATE DATA                                */
/****************************************************************************/

/**
 * @brief Simple map (month - 1) containing the value of the number of days in the month
 */
static const uint8_t m_day_count[] = {
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

static RECEIVE_OPTIONS_TYPE_EX m_rx_opts = { 0 };

/****************************************************************************/
/*                       PRIVATE FUNCTION DECLARATIONS                      */
/****************************************************************************/

static uint16_t app_sch_get_schedule_count(const ascc_type_t schedule_type);
static uint16_t app_sch_get_target_count(void);
static bool app_sch_validate_target(const ascc_target_t * const target);
static bool app_sch_validate_schedule_slot(const uint16_t target_ID,
                                          const ascc_type_t type,
                                          const uint16_t slot);
static bool app_sch_validate_schedule_data(const ascc_schedule_t * const schedule);
static ascc_op_result_t app_sch_get_schedule_state(const ascc_target_t * const target,
                                                  bool * state);
static ascc_op_result_t app_sch_set_schedule_state(const ascc_target_t * const target,
                                                  const bool state);
static ascc_op_result_t app_sch_get_schedule_data(const ascc_type_t schedule_type,
                                                 const uint16_t slot,
                                                 const ascc_target_t * const target,
                                                 ascc_schedule_data_t * schedule,
                                                 uint16_t * next_slot);
static ascc_op_result_t app_sch_set_schedule_data(const ascc_op_type_t operation,
                                                 const ascc_schedule_t * const schedule,
                                                 uint16_t * next_slot);
static void clear_all_schedules_for_user(uint16_t uuid, bool send_reports);
static void clear_all_schedules_for_user_by_type(
    schedule_metadata_nvm_t * schedule_data,
    const ascc_type_t schedule_type);
static uint16_t get_first_schedule_slot(
    const schedule_metadata_nvm_t * schedule_data,
    const ascc_type_t type);
static uint16_t get_next_schedule_slot(
    const schedule_metadata_nvm_t * schedule_data,
    const ascc_type_t type,
    const uint16_t current_slot);
static bool is_time_stamp_valid(const ascc_time_stamp_t * const timestamp);
static bool is_time_fence_valid(const ascc_time_stamp_t * const start,
                                const ascc_time_stamp_t * const end);
static void user_changed(
  const uint16_t uuid,
  const u3c_operation_type_t operation
);

/****************************************************************************/
/*                       EXPORTED FUNCTION DEFINITIONS                      */
/****************************************************************************/

void app_sch_initialize_handlers(void)
{
    // At least one schedule needs to be supported per user, otherwise do not register. 
    if (cc_user_credential_get_num_year_day_per_user() > 0 ||
        cc_user_credential_get_num_daily_repeating_per_user() > 0) {
        
        const ascc_target_stubs_t stubs = {
            .get_schedule_count = app_sch_get_schedule_count,
            .get_schedule_data = app_sch_get_schedule_data,
            .get_schedule_state = app_sch_get_schedule_state,
            .get_target_count = app_sch_get_target_count,
            .set_schedule_data = app_sch_set_schedule_data,
            .set_schedule_state = app_sch_set_schedule_state,
            .validate_schedule_data = app_sch_validate_schedule_data,
            .validate_schedule_slot = app_sch_validate_schedule_slot,
            .validate_target = app_sch_validate_target,
        };
        u3c_nvm_cbs_t callbacks = {
            .user_changed = user_changed
        };
        u3c_nvm_register_cbs(&callbacks);
        CC_ActiveSchedule_RegisterCallbacks(COMMAND_CLASS_USER_CREDENTIAL_V2, &stubs);
    }
}

void app_sch_init_schedule_db(void)
{
    // Intended to be called by initialization logic if DB is empty
    ascc_schedule_t tmp_schedule = {
        .slot_id = 1,
        .target = {
            .target_cc = COMMAND_CLASS_USER_CREDENTIAL,
            .target_id = 1,
        }
    };
    // Stick a YD Schedule on user 1
    if (cc_user_credential_get_num_year_day_per_user() > 0) {
        tmp_schedule.type = ASCC_TYPE_YEAR_DAY;
        ascc_year_day_schedule_t * sch_data = &tmp_schedule.data.schedule.year_day;
        memset(sch_data, 0, sizeof(ascc_year_day_schedule_t));
        sch_data->start_year = 1923;
        sch_data->start_month = 3;
        sch_data->start_day = 4;
        sch_data->start_hour = 12;
        sch_data->start_minute = 25;
        sch_data->stop_year = 2023;
        sch_data->stop_month = 5;
        sch_data->stop_day = 4; 
        sch_data->stop_hour = 3;
        sch_data->stop_minute = 23;
        app_sch_set_schedule_data(ASCC_OP_TYPE_MODIFY, &tmp_schedule, NULL);
    }

    // Stick a DR schedule on user 1
    if (cc_user_credential_get_num_daily_repeating_per_user > 0) {
        tmp_schedule.type = ASCC_TYPE_DAILY_REPEATING;
        ascc_daily_repeating_schedule_t * sch_data = &tmp_schedule.data.schedule.daily_repeating;
        memset(sch_data, 0, sizeof(ascc_daily_repeating_schedule_t));
        sch_data->duration_hour = 4; 
        sch_data->duration_minute = 56;
        sch_data->start_hour = 12;
        sch_data->start_minute = 34;
        sch_data->weekday_mask = (uint8_t)0b00111110; // Weekdays only
        app_sch_set_schedule_data(ASCC_OP_TYPE_MODIFY, &tmp_schedule, NULL);
    }
}

void app_sch_reset_schedules(void)
{
    schedule_metadata_nvm_t data = { 0 };
    uint16_t max = u3c_nvm_get_max_users();
    for (uint16_t i = 0; i < max; i++) {
        app_nvm(U3C_WRITE, APP_NVM_AREA_SCHEDULE_DATA, i ,&data, sizeof(schedule_metadata_nvm_t));
    }
}

bool app_sch_local_delete_for_user(const uint16_t p_uuid, const ascc_type_t p_schedule_type)
{
    uint16_t uuid;
    // Special case for uuid of 0
    if (p_uuid == 0 &&
        !u3c_nvm_get_first_uuid(&uuid)) {
        return false;
    } else if (p_uuid != 0) {
        uuid = p_uuid;
    }
    schedule_metadata_nvm_t schedule_data = { 0 };
    if (u3c_nvm_get_user_offset_from_id(uuid, NULL) &&
            app_nvm(U3C_READ, APP_NVM_AREA_SCHEDULE_DATA, uuid-1, &schedule_data, sizeof(schedule_metadata_nvm_t))) {
        clear_all_schedules_for_user_by_type(&schedule_data, p_schedule_type);
    }
    // Back up updated mirror to NVM
    if (app_nvm(U3C_WRITE, APP_NVM_AREA_SCHEDULE_DATA, uuid-1, &schedule_data, sizeof(schedule_metadata_nvm_t))) {
        // Send unsolicited report to lifeline
        static ascc_sched_event_data_t schedule_event_data; 
        memset(&schedule_event_data, 0, sizeof(ascc_sched_event_data_t));
        // Fill in required information for batch delete report
        schedule_event_data.schedule.type = p_schedule_type;
        schedule_event_data.report_type = p_schedule_type == ASCC_TYPE_DAILY_REPEATING ? 
            ACTIVE_SCHEDULE_DAILY_REPEATING_SCHEDULE_REPORT_REPORT_CODE_SCHEDULE_MODIFIED_EXTERNAL :
            ACTIVE_SCHEDULE_YEAR_DAY_SCHEDULE_REPORT_REPORT_CODE_SCHEDULE_MODIFIED_EXTERNAL;
        schedule_event_data.schedule.target.target_cc = COMMAND_CLASS_USER_CREDENTIAL_V2; 
        schedule_event_data.schedule.target.target_id = uuid;
        zaf_event_distributor_enqueue_cc_event(COMMAND_CLASS_ACTIVE_SCHEDULE, ASCC_APP_EVENT_ON_SET_SCHEDULE_COMPLETE, 
            (void*)(&schedule_event_data));
        return true;
    }
    return false; 
}

bool app_sch_toggle_enable(bool * state)
{
    uint16_t uuid;
    if (!state) {
        return false; 
    }
    // Special case for uuid of 0
    if (u3c_nvm_get_first_uuid(&uuid)) {
        schedule_metadata_nvm_t schedule_data = { 0 };
        if (app_nvm(U3C_READ, APP_NVM_AREA_SCHEDULE_DATA, uuid-1, &schedule_data, sizeof(schedule_metadata_nvm_t))) {
            //Flip bit
            schedule_data.scheduling_active = !schedule_data.scheduling_active;
            if (app_nvm(U3C_WRITE, APP_NVM_AREA_SCHEDULE_DATA, uuid-1, &schedule_data, sizeof(schedule_metadata_nvm_t))) {
                *state = schedule_data.scheduling_active;
                static ascc_sched_enable_event_data_t event_data = { 0 };
                event_data.enabled = schedule_data.scheduling_active;
                event_data.report_type = ACTIVE_SCHEDULE_ENABLE_REPORT_REPORT_CODE_SCHEDULE_MODIFIED_EXTERNAL;
                event_data.target.target_cc = COMMAND_CLASS_USER_CREDENTIAL_V2;
                event_data.target.target_id = uuid;
                event_data.rx_opts = NULL; // < This ensures lifeline transmission
                zaf_event_distributor_enqueue_cc_event_from_isr(COMMAND_CLASS_ACTIVE_SCHEDULE, ASCC_APP_EVENT_ON_SET_SCHEDULE_STATE_COMPLETE,
                    (void*)&event_data); 
                return true;
            }
        }
    }
    return false;
}

/****************************************************************************/
/*                       PRIVATE FUNCTION DEFINITIONS                       */
/****************************************************************************/

 /**
 * @brief Gets the number of supported schedules per target for a given schedule type.
 *
 * @param schedule_type Schedule type (Year Day/ Daily Repeating)
 * @return Number of supported schedules per target
 */
uint16_t app_sch_get_schedule_count(const ascc_type_t schedule_type)
{
    if (schedule_type == ASCC_TYPE_DAILY_REPEATING) {
        return cc_user_credential_get_num_daily_repeating_per_user();
    } else if (schedule_type == ASCC_TYPE_YEAR_DAY) {
        return cc_user_credential_get_num_year_day_per_user();
    }
    return 0;
}

/**
 * @brief Gets the number of supported targets supported by the registered CC.
 *
 * @return Number of supported targets for the registered CC handler.
 */
uint16_t app_sch_get_target_count(void) {
    return cc_user_credential_get_max_user_unique_identifiers();
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
bool app_sch_validate_target(const ascc_target_t * const target) {
    if (!target) {
        return false;
    } else if (target->target_cc != COMMAND_CLASS_USER_CREDENTIAL) {
        return false;
    } else if (target->target_id >= cc_user_credential_get_max_user_unique_identifiers()) {
        return false;
    } else if (target != 0) {
        // Just return if the user exists in the database or not
        return u3c_nvm_get_user_offset_from_id(target->target_id,NULL);
    } else {
        return true;
    }
}

/**
 * @brief Verifies that given Target ID and schedule slot is valid
 *        for the registered CC and given type.
 *
 * @param target_ID   Target ID
 * @param type        Schedule type (Year Day/Daily Repeating)
 * @param slot        Schedule slot
 *
 * @note Slots are 1-indexed so we ned to account for that here.
 * @return true if target slot is valid.
 *         false if target slot is invalid.
 */
bool app_sch_validate_schedule_slot(const uint16_t target_ID,
                                    const ascc_type_t type,
                                    const uint16_t slot)
{
    const ascc_target_t user = {
        .target_cc = COMMAND_CLASS_USER_CREDENTIAL_V2,
        .target_id = target_ID,
    };
    if (app_sch_validate_target(&user)) {
        if (type == ASCC_TYPE_DAILY_REPEATING) {
            return slot <= cc_user_credential_get_num_daily_repeating_per_user();
        } else if (type == ASCC_TYPE_YEAR_DAY) {
            return slot <= cc_user_credential_get_num_year_day_per_user();
        }
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
bool app_sch_validate_schedule_data(const ascc_schedule_t * const schedule)
{
    // Requirement CC:0083.02.00.21.003
    // Schedule data is unused, but we allow it anyway
    if (schedule->data.metadata_length > 0) {
        DPRINTF("%s: Metadata of length %d will be ignored",
            __func__,
            schedule->data.metadata_length);
    }
    bool response = true;
    // Check schedule time values here
    if (schedule->type == ASCC_TYPE_DAILY_REPEATING) {
        const ascc_daily_repeating_schedule_t * dr_schedule =
            &schedule->data.schedule.daily_repeating;
        /* Make sure that all of the values are in appropriate ranges */
        if (dr_schedule->weekday_mask == 0 || // At least one day must be selected
              dr_schedule->duration_hour > MAX_HOUR_COUNTER ||
              dr_schedule->duration_minute > MAX_MINUTE_COUNTER ||
              dr_schedule->start_hour > MAX_HOUR_COUNTER ||
              dr_schedule->start_minute > MAX_MINUTE_COUNTER ||
              dr_schedule->weekday_mask > MAX_WEEKDAY_MASK ||
              (dr_schedule->duration_hour == 0 && // Users don't support zero duration
              dr_schedule->duration_minute == 0)) {
            response = false;
        }
    } else if (schedule->type == ASCC_TYPE_YEAR_DAY) {
        const ascc_year_day_schedule_t * yd_schedule = &schedule->data.schedule.year_day;
        const ascc_time_stamp_t * start = (ascc_time_stamp_t*)&yd_schedule->start_year;
        const ascc_time_stamp_t * end = (ascc_time_stamp_t*)&yd_schedule->stop_year;
        // Verifying the time fence also verifies the individual time stamps
        response = is_time_fence_valid(start, end);
    } else {
        // Don't want rogue enumerator values in here
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
 *          ASCC_OPERATION_RESULT_FAILURE for unsuccessful get operation. Consult supporting node documentation.
 */
ascc_op_result_t app_sch_get_schedule_state(const ascc_target_t * const target,
                                           bool * state)
{
    ascc_op_result_t result = {
        .result = ASCC_OPERATION_FAIL
    };
    if (state &&
        target &&
        u3c_nvm_get_user_offset_from_id(target->target_id, NULL)) {
        schedule_metadata_nvm_t schedule_data = { 0 };
        if (app_nvm(U3C_READ, APP_NVM_AREA_SCHEDULE_DATA, target->target_id-1, (void*)&schedule_data, sizeof(schedule_metadata_nvm_t))) {
            *state = schedule_data.scheduling_active;
            result.result = ASCC_OPERATION_SUCCESS;
        }
    }
    return result;
}

/**
 * @brief Sets the state of the schedules attached to a target.
 *
 * @note  It is assumed that the inputs have been cleaned and the values provided are valid
 *        for the given operation.
 *
 * @param target   Const pointer to target
 * @param state    Boolean input - True to enable all schedules attached to a target,
 *                                 False to disable all schedules attached to a target.
 *
 * @return ascc_io_op_result_t as follows:
 *         ASCC_OPERATION_RESULT_SUCCESS for successful set
 *         ASCC_OPERATION_RESULT_WORKING for successful initiation of set process.
 *             Application assumes responsibility for appropriate report handling.
 *         ASCC_OPERATION_RESULT_FAILURE for unsuccessful set operation. Consult supporting node documentation.
 */
ascc_op_result_t app_sch_set_schedule_state(const ascc_target_t * const target,
                                           const bool state)
{
    ascc_op_result_t result = {
        .result = ASCC_OPERATION_FAIL
    };
    if (target && u3c_nvm_get_user_offset_from_id(target->target_id, NULL)) {
        schedule_metadata_nvm_t schedule_data = { 0 };
        if (app_nvm(U3C_READ, APP_NVM_AREA_SCHEDULE_DATA, target->target_id-1, &schedule_data, sizeof(schedule_metadata_nvm_t))) {
            // If 'enabled' value is not equal, then update and back up
            if (schedule_data.scheduling_active != state) {
                schedule_data.scheduling_active = state;
                if(!app_nvm(U3C_WRITE, APP_NVM_AREA_SCHEDULE_DATA, target->target_id-1, &schedule_data, sizeof(schedule_metadata_nvm_t))) {
                    return result;
                }
            }
            result.result = ASCC_OPERATION_SUCCESS;
        }
    }
    return result;
}

/**
 * @brief Gets the schedule data for a given type and slot number.
 *
 * @note  It is assumed that the inputs have been cleaned and the values provided are valid
 *        for the given operation.
 *
 * @param schedule_type  Year Day or Daily Repeating
 * @param slot           Slot Index
 * @param target         Pointer to Target
 * @param[out] schedule  Pointer to schedule structure in which schedule data is returned
 * @param[out] next_slot Pointer with which to populate the next occupied schedule slot
 *                       for that target, or 0 if there are none.
 *
 * @note Remember that slots are 1-indexed from a Z-Wave perspective.
 *
 * @return ascc_io_op_result_t as follows:
 *         ASCC_OPERATION_RESULT_SUCCESS for successful get
 *         ASCC_OPERATION_RESULT_WORKING for successful initiation of get process.
 * `            Application assumes responsibility for appropriate report handling.
 *         ASCC_OPERATION_RESULT_FAILURE for unsuccessful get operation. Consult supporting node documentation.
 */
ascc_op_result_t app_sch_get_schedule_data(const ascc_type_t schedule_type,
                                          const uint16_t slot,
                                          const ascc_target_t * const target,
                                          ascc_schedule_data_t * schedule,
                                          uint16_t * next_slot)
{
    ascc_op_result_t result = {
        .result = ASCC_OPERATION_FAIL
    };
    if (target
        /* Overflow protection */
        && slot <= app_sch_get_schedule_count(schedule_type)
        && schedule
        && u3c_nvm_get_user_offset_from_id(target->target_id, NULL)) {
        schedule_metadata_nvm_t schedule_data = { 0 };
        if (app_nvm(U3C_READ, APP_NVM_AREA_SCHEDULE_DATA, target->target_id-1, (void*)&schedule_data, sizeof(schedule_metadata_nvm_t))) {
            uint16_t slot_tmp = slot == 0 ? get_first_schedule_slot(&schedule_data, schedule_type) :
                                           slot;
            if (schedule_type == ASCC_TYPE_DAILY_REPEATING) {
                memcpy(&schedule->schedule.daily_repeating, (void*)&schedule_data.daily_repeating_schedules[slot_tmp-1].schedule, sizeof(ascc_daily_repeating_schedule_t));
            } else if (schedule_type == ASCC_TYPE_YEAR_DAY) {
                memcpy(&schedule->schedule.year_day, (void*)&schedule_data.year_day_schedules[slot_tmp-1].schedule, sizeof(ascc_year_day_schedule_t));
            } else {
                result.result = ASCC_OPERATION_INVALID_GET;
                return result;
            }
            // Populate next slot, if provided.
            if (next_slot) {
                *next_slot = get_next_schedule_slot(&schedule_data, schedule_type, slot_tmp);
            }
            result.result = ASCC_OPERATION_SUCCESS;
        }
    } else {
        result.result = ASCC_OPERATION_INVALID_GET;
    }

    return result;
}

/**
 * @brief Sets schedule slot information, either to modify and attach to a target or
 *        clear the slot.
 *
 * @note  It is assumed that the inputs have been cleaned and the values provided are valid
 *        for the given operation.
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
 *         ASCC_OPERATION_RESULT_FAILURE for unsuccessful set operation. Consult supporting node documentation.
 */
ascc_op_result_t app_sch_set_schedule_data(const ascc_op_type_t operation,
                                          const ascc_schedule_t * const schedule,
                                          uint16_t * next_slot)
{
    ascc_op_result_t result = {
        .result = ASCC_OPERATION_FAIL
    };

    if (!next_slot || !schedule) {
        return result;
    }
    if (operation == ASCC_OP_TYPE_ERASE) {
        // Erase all schedules
        if (schedule->slot_id == 0) {
            if (schedule->target.target_id == 0) { // Erase all schedules for all targets
                if (CC_ActiveSchedule_Get_Current_Frame_Options(&m_rx_opts)) {
                    result.result = ASCC_OPERATION_WORKING;
                    result.working_time = 10; // 10 seconds as an example
                }
                zaf_event_distributor_enqueue_app_event_from_isr(schedule->type == ASCC_TYPE_YEAR_DAY ? EVENT_APP_DELETE_ALL_YD_SCHEDULES_START :
                                                                                                EVENT_APP_DELETE_ALL_DR_SCHEDULES_START);
            } else { // Erase all schedules for this target specifically
                schedule_metadata_nvm_t schedule_data = { 0 };
                if (u3c_nvm_get_user_offset_from_id(schedule->target.target_id, NULL) &&
                      app_nvm(U3C_READ, APP_NVM_AREA_SCHEDULE_DATA, schedule->target.target_id-1, &schedule_data, sizeof(schedule_metadata_nvm_t))) {
                    clear_all_schedules_for_user_by_type(&schedule_data, schedule->type);
                }
                // Back up updated mirror to NVM
                if (app_nvm(U3C_WRITE, APP_NVM_AREA_SCHEDULE_DATA, schedule->target.target_id-1, &schedule_data, sizeof(schedule_metadata_nvm_t))) {
                    result.result = ASCC_OPERATION_SUCCESS;
                    *next_slot = 0;
                }
            }
        // Erase a specific schedule, 0 isn't allowed as a user
        } else if (schedule->slot_id != 0 && schedule->target.target_id != 0){
            schedule_metadata_nvm_t schedule_data = { 0 };
            if (u3c_nvm_get_user_offset_from_id(schedule->target.target_id, NULL) &&
                  app_nvm(U3C_READ, APP_NVM_AREA_SCHEDULE_DATA, schedule->target.target_id-1, &schedule_data, sizeof(schedule_metadata_nvm_t))) {
                // Update information in local flash mirror
                // By clearing out the entire struct, we also clear the available bit
                void * ptr = NULL;
                size_t len = 0;
                if (schedule->type == ASCC_TYPE_DAILY_REPEATING) {
                    ptr = (void*)&schedule_data.daily_repeating_schedules[schedule->slot_id-1];
                    len = sizeof(daily_repeating_nvm_t);
                } else if (schedule->type == ASCC_TYPE_YEAR_DAY) {
                    ptr = (void*)&schedule_data.year_day_schedules[schedule->slot_id-1];
                    len = sizeof(year_day_nvm_t);
                }
                if (ptr) {
                    memset(ptr, 0x00, len);
                    // Back up updated mirror to NVM
                    if (app_nvm(U3C_WRITE, APP_NVM_AREA_SCHEDULE_DATA, schedule->target.target_id-1, &schedule_data, sizeof(schedule_metadata_nvm_t))) {
                        result.result = ASCC_OPERATION_SUCCESS;
                        *next_slot = get_next_schedule_slot(&schedule_data, schedule->type, schedule->slot_id);
                    }
                }
            }
        }
    } else if (operation == ASCC_OP_TYPE_MODIFY &&
               schedule->slot_id != 0 && schedule->target.target_id != 0) {
        schedule_metadata_nvm_t schedule_data = { 0 };
        if (app_nvm(U3C_READ, APP_NVM_AREA_SCHEDULE_DATA, schedule->target.target_id-1, &schedule_data, sizeof(schedule_metadata_nvm_t))) {
            // Update information in local flash mirror
            if (schedule->type == ASCC_TYPE_DAILY_REPEATING) {
                daily_repeating_nvm_t * tmp = &schedule_data.daily_repeating_schedules[schedule->slot_id-1];
                memcpy(&tmp->schedule,
                        &schedule->data.schedule.daily_repeating,
                        sizeof(ascc_daily_repeating_schedule_t));
                tmp->occupied = true;
            } else if (schedule->type == ASCC_TYPE_YEAR_DAY) {
                year_day_nvm_t * tmp = &schedule_data.year_day_schedules[schedule->slot_id-1];
                memcpy(&tmp->schedule,
                        &schedule->data.schedule.year_day,
                        sizeof(ascc_year_day_schedule_t));
                tmp->occupied = true;
            }
            // Enable scheduling for the target by default
            schedule_data.scheduling_active = true;
            // Back up updated mirror to NVM
            if (app_nvm(U3C_WRITE, APP_NVM_AREA_SCHEDULE_DATA, schedule->target.target_id-1, &schedule_data, sizeof(schedule_metadata_nvm_t))) {
                result.result = ASCC_OPERATION_SUCCESS;
                *next_slot = get_next_schedule_slot(&schedule_data, schedule->type, schedule->slot_id);
            }
        }
    }
    return result;
}

static void clear_all_schedules_for_user(uint16_t uuid, bool send_local_reports)
{
    schedule_metadata_nvm_t schedule_data = { 0 };
    if(app_nvm(U3C_READ, APP_NVM_AREA_SCHEDULE_DATA, uuid-1, &schedule_data, sizeof(schedule_metadata_nvm_t))) {
        schedule_data.scheduling_active = false;
        schedule_data.uuid = 0;
        clear_all_schedules_for_user_by_type(&schedule_data, ASCC_TYPE_YEAR_DAY);
        clear_all_schedules_for_user_by_type(&schedule_data, ASCC_TYPE_DAILY_REPEATING);
        // Back up updated mirror to NVM
        app_nvm(U3C_WRITE, APP_NVM_AREA_SCHEDULE_DATA, uuid-1, &schedule_data, sizeof(schedule_metadata_nvm_t));
        if (send_local_reports) {
            /*
             * This function, when called from the radio context, is handled as part of a
             * larger transaction and Z-Wave Modified reports will be sent later. 
             * 
             * We only need to handle report TX when the schedules are modified locally.
             */
            static ascc_sched_clear_event_data_t event_data = { 0 };
            event_data.report_type = ASCC_REP_TYPE_MODIFY_EXTERNAL;
            event_data.target.target_cc = COMMAND_CLASS_USER_CREDENTIAL_V2;
            event_data.target.target_id = uuid;
            event_data.send_dr = cc_user_credential_get_num_daily_repeating_per_user();
            event_data.send_yd = cc_user_credential_get_num_year_day_per_user();
            event_data.rx_opts = NULL;
            // Use events to ensure that TX transmits are called within the ZAF context.
            zaf_event_distributor_enqueue_cc_event_from_isr(
                COMMAND_CLASS_ACTIVE_SCHEDULE,
                ASCC_APP_EVENT_ALL_SCHEDULES_CLEARED_FOR_TARGET,
                &event_data
            );
        }
    }
}

/**
 * @brief Clears all schedules for a given block of metadata and schedule type
 *
 * @param schedule_data Schedule metadata for a user
 * @param schedule_type Which type of schedule to clear
 */
static void clear_all_schedules_for_user_by_type(
    schedule_metadata_nvm_t * schedule_data,
    const ascc_type_t type)
{
    void* start = NULL;
    size_t size = 0;
    if (type == ASCC_TYPE_DAILY_REPEATING) {
        start = (void*)&schedule_data->daily_repeating_schedules;
        size = sizeof(daily_repeating_nvm_t) * cc_user_credential_get_num_daily_repeating_per_user();
    } else if (type == ASCC_TYPE_YEAR_DAY) {
        start = (void*)&schedule_data->year_day_schedules;
        size = sizeof(year_day_nvm_t) * cc_user_credential_get_num_year_day_per_user();
    }
    if (start != NULL) {
        memset(start, 0x00, size);
    }
}

/**
 * @brief Retrieves the first occupied schedule slot (1-indexed) from a schedule information entry.
 *
 * @returns First occupied slot, or 0 if there are no occupied indices.
 */
static uint16_t get_first_schedule_slot(const schedule_metadata_nvm_t * schedule_data, const ascc_type_t type)
{
    return get_next_schedule_slot(schedule_data, type, 0);
}

/**
 * @brief Retrieves the next occupied 1-indexed schedule slot, starting from a known offset.
 *        If
 * @param schedule_data Schedule information for a user from NVM
 * @param type          Which schedule type to parse
 * @param current_slot  Current schedule slot (1-index). Passing in 0 in this parameter will
 *                      return the first occupied slot for the given user and schedule type.
 * @returns Next occupied slot, or 0 if there are no more occupied slots.
 */
static uint16_t get_next_schedule_slot(
    const schedule_metadata_nvm_t * schedule_data,
    const ascc_type_t type,
    const uint16_t current_slot)
{
    ASSERT(schedule_data != NULL);
    uint16_t index = current_slot;
    bool occupied = false;
    uint16_t max_slot = app_sch_get_schedule_count(type);
    if (type == ASCC_TYPE_DAILY_REPEATING) {
        while (!occupied && index < max_slot) {
            if (schedule_data->daily_repeating_schedules[index++].occupied) {
                occupied = true;
            }
        }
    } else if (type == ASCC_TYPE_YEAR_DAY) {
        while (!occupied && index < max_slot) {
            if (schedule_data->year_day_schedules[index++].occupied) {
                occupied = true;
            }
        }
    }
    // Index has increased by one, so it matches the slot number
    return occupied ? index : 0;
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
    uint8_t days = m_day_count[timestamp->month];
    // Make sure this isn't february during a leap year
    if (timestamp->month == FEB_INDEX &&
          ((timestamp->year % LEAP_YEAR_CADENCE) == 0)) {
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
                    result = true;
                } else if (start->day == end->day) {
                    if (start->hour < end->hour) {
                        result = true;
                    } else if (start-> hour == end->hour) {
                        if (start->minute < end->minute) {
                            result = true;
                        } // If time stamps are equal, then this check fails
                    }
                }
            }
        }
    }
    return result;
}

/**
 * @brief When a user is deleted, the schedules attached to that user also need
 *        to be deleted.
 * 
 * @param uuid User in question
 * @param operation The operation that the user underwent
 */
static void user_changed(
  const uint16_t uuid,
  const u3c_operation_type_t operation
)
{
    if (operation == U3C_OPERATION_TYPE_DELETE) {
        clear_all_schedules_for_user(uuid,false);
    } else if (operation == U3C_OPERATION_TYPE_MODIFY) {
        u3c_user_t user = { 0 };
        CC_UserCredential_get_user(uuid, &user, NULL);
        if (user.type == USER_TYPE_EXPIRING) {
            clear_all_schedules_for_user(uuid, true);
        }
    }
}

#ifdef __cplusplus
}
#endif
