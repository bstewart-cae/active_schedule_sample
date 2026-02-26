/*
 * SPDX-License-Identifier: BSD-3-Clause
 * SPDX-FileCopyrightText: 2025 Z-Wave Alliance <https://z-wavealliance.org>
 * SPDX-FileCopyrightText: 2025 Card Access Engineering, LLC. <https://www.caengineering.com>
 */
/**
 * @file CC_ActiveSchedule_types.h
 * Defines helper structures and data types for the Active Schedule CC handler code.
 *
 * The Active Schedule CC needs to be application agnostic and new command classes
 * should be able to use the framework without having to make changes to the handler code.
 *
 *
 * @addtogroup CC
 * @{
 * @addtogroup Active Schedule
 * @{
 *
 *
 * The specifications of the Active Schedule CC and Schedule Entry Lock CC can be found in
 * https://github.com/Z-Wave-Alliance/AWG/
 *
 * @}
 * @}
 *
 *
 */

#ifndef _COMMANDCLASSACTIVESCHEDULETYPES_H_
#define _COMMANDCLASSACTIVESCHEDULETYPES_H_

/****************************************************************************
*                              INCLUDE FILES                               *
****************************************************************************/
#include <ZAF_types.h>

/****************************************************************************
*                      PRIVATE TYPES and DEFINITIONS                       *
****************************************************************************/

/// Set length of the metadata field attached to each schedule.
#define ACTIVE_SCHEDULE_METADATA_LENGTH   7

/**
 * @brief Unless otherwise specified, all of the app events here
 * correspond to an operation completing in a time indeterminate
 * manner.
 *
 */
typedef enum _ascc_app_event_ {
  ASCC_APP_EVENT_ON_GET_SCHEDULE_CAPABILITIES_COMPLETE,
  ASCC_APP_EVENT_ON_GET_SCHEDULE_COMPLETE,
  ASCC_APP_EVENT_ON_SET_SCHEDULE_COMPLETE,
  ASCC_APP_EVENT_ON_GET_SCHEDULE_STATE_COMPLETE,
  ASCC_APP_EVENT_ON_SET_SCHEDULE_STATE_COMPLETE,
  ASCC_APP_EVENT_ALL_SCHEDULES_CLEARED_FOR_TARGET,
  ASCC_APP_EVENT_ON_SCHEDULE_STATE_CHANGE,                ///< End node signals to the stack that a
                                                          ///  schedule has updated to a new state.
                                                          ///  Currently unused, but desired.
} ascc_app_event_t;

/**
 * @brief Enumeration of potential I/O operation results to trigger reports
 * pr Supervision states, as needed.
 */
typedef enum _ascc_io_op_result {
  ASCC_OPERATION_SUCCESS,       ///< Set or Get operation succeeded
  ASCC_OPERATION_WORKING,       ///< Set or Get operation is in process,
                                ///  Report to be returned at a later time by application.
                                ///  Application assumes responsibility for command state management.
  ASCC_OPERATION_FAIL,          ///< Set or Get operation failed
  ASCC_OPERATION_INVALID_GET,   ///< Get Operation Invalid (sets have a verification step)
} ascc_io_op_result_t;

/*
 * As this packet contains only constant values and
 * is intended to be returned in its entirety rather than as a pointer,
 * This packs the struct into the footprint of a uint16_t rather than
 * the 8 bytes it would be with padding otherwise in the worst case, depending
 * on the platform.
 *
 * This ensures minimal stack usage and ease of parsing.
 */
#pragma pack(push, 2)
/**
 * @brief 2 byte return structure for IO operations.
 * Includes working time for proper supervision handling
 * as IO operations should be assumed to be time-indeterminate.
 */
typedef struct _ascc_result_ {
  ascc_io_op_result_t result;
  uint8_t working_time;
} ascc_op_result_t;
#pragma pack(pop)

/**
 * @brief Enumerated Schedule Set operations.
 */
typedef enum _ascc_op_type {
  ASCC_OP_TYPE_ERASE = 0x00,    ///< Clear a Schedule slot
  ASCC_OP_TYPE_MODIFY           ///< Modify a Schedule slot
} ascc_op_type_t;

/**
 * @brief Defines the supported types of schedule
 */
typedef enum _ascc_type {
  ASCC_TYPE_YEAR_DAY        = 0x00,   ///< Time Fence defined by a start date and time,
                                      ///  and a stop date and time
  ASCC_TYPE_DAILY_REPEATING = 0x01    ///< Time Fence defined by a start time, a duration,
                                      ///  and which days throughout the week this schedule
                                      ///  is active (M-F, T/Th, etc.)
} ascc_type_t;

/**
 * @brief Defines a Target data structure. This
 * is a construct to which a schedule is attached.
 */
typedef struct _ascc_target {
  uint8_t target_cc;        ///< Command Class ID that supports scheduling.
  uint16_t target_id;       ///< Depending on the Command Class, this identifies what specifically
                            ///  in that command class is scheduled.
} ascc_target_t;

/**
 * @brief Defines the report type.
 */
typedef enum _ascc_report_type_t_ {
  ASCC_REP_TYPE_RESPONSE_TO_GET = ACTIVE_SCHEDULE_YEAR_DAY_SCHEDULE_REPORT_REPORT_CODE_RESPONSE_TO_GET,
  ASCC_REP_TYPE_MODIFY_EXTERNAL = ACTIVE_SCHEDULE_YEAR_DAY_SCHEDULE_REPORT_REPORT_CODE_SCHEDULE_MODIFIED_EXTERNAL,
  ASCC_REP_TYPE_MODIFY_ZWAVE    = ACTIVE_SCHEDULE_YEAR_DAY_SCHEDULE_REPORT_REPORT_CODE_SCHEDULE_MODIFIED_Z_WAVE
} ascc_report_type_t;

/**
 * @brief Defines the time fence for a year day schedule.
 *
 * @note Unlike SEL, ASCC uses the Gregorian year rather than an epoch.
 *       For example, a year value for 2026 would be
 *       saved directly as an integer value 0x07EA (2026).
 */
typedef struct _ascc_year_day_schedule {
  uint16_t start_year;
  uint8_t start_month;
  uint8_t start_day;
  uint8_t start_hour;
  uint8_t start_minute;
  uint16_t stop_year;
  uint8_t stop_month;
  uint8_t stop_day;
  uint8_t stop_hour;
  uint8_t stop_minute;
} ascc_year_day_schedule_t;

/**
 * @brief Defines the time fence for a daily repeating schedule.
 */
typedef struct _ascc_daily_repeating_schedule {
  uint8_t weekday_mask;
  uint8_t start_hour;
  uint8_t start_minute;
  uint8_t duration_hour;
  uint8_t duration_minute;
} ascc_daily_repeating_schedule_t;

/** 
 * @brief TSE requires a pointer to a RECEIVE_OPTIONS_TYPE_EX at the initial memory location.
 *        Passing a pointer to cc_handler_input_t into TSE_Trigger results in a double pointer
 *        and undefined behavior.
 * 
 * @note  These are functionally equivalent to cc_handler_input_t but instead uses a full nested
 *        struct for the RX options rather than a pointer to it.
 */
typedef struct ascc_report_blob {
  RECEIVE_OPTIONS_TYPE_EX rx_options;
  ZW_APPLICATION_TX_BUFFER * frame;
  uint8_t length;
} ascc_report_blob_t;

/**
 * @brief Defines a common schedule structure for easier handling and manipulation
 */
typedef struct _ascc_schedule_data {
  union {
    ascc_year_day_schedule_t year_day;
    ascc_daily_repeating_schedule_t daily_repeating;
  } schedule;
  uint8_t metadata_length;
  uint8_t metadata[ACTIVE_SCHEDULE_METADATA_LENGTH];
} ascc_schedule_data_t;

/**
 * @brief Defines a helper struct to include schedule data with the index and type.
 */
typedef struct _ascc_schedule {
  ascc_target_t target;
  uint16_t slot_id;
  ascc_type_t type;
  ascc_schedule_data_t data;
} ascc_schedule_t;

/**
 * @brief Defines a helper struct to include schedule event data.
 */
typedef struct _ascc_sched_event_data {
  RECEIVE_OPTIONS_TYPE_EX rx_opts;
  ascc_report_type_t report_type;
  ascc_schedule_t schedule;
  uint16_t next_schedule_slot;
} ascc_sched_event_data_t;

/**
 * @brief Defines a helper struct to include schedule clear event data.
 *        We only need the target information here. When all schedules are cleared, 
 *        virtually everything is zeroed out.
 */
typedef struct _ascc_sched_clear_event_data {
  RECEIVE_OPTIONS_TYPE_EX * rx_opts; ///< NULL if locally or externally triggered
  ascc_report_type_t report_type;
  ascc_target_t target;
  bool send_yd;                      ///< Mark true if year day report needs to be sent.
  bool send_dr;                      ///< Mark true if daily repeating report needs to be sent.
} ascc_sched_clear_event_data_t;

/**
 * @brief Defines a helper struct to include schedule enable event data.
 */
typedef struct _ascc_sched_enable_event_data {
  RECEIVE_OPTIONS_TYPE_EX * rx_opts;
  ascc_report_type_t report_type;
  ascc_target_t target;
  bool enabled;
} ascc_sched_enable_event_data_t;

/*************************************
 * Stub Function Definitions
 **************************************/

/**
 * @brief Gets the number of supported schedules per target for a given schedule type.
 *
 * @param schedule_type Schedule type (Year Day/ Daily Repeating)
 * @return Number of supported schedules per target
 */
typedef uint16_t (*ascc_get_schedule_count_stub_t)(const ascc_type_t schedule_type);

/**
 * @brief Gets the number of supported targets supported by the registered CC.
 *
 * @return Number of supported targets for the registered CC handler.
 */
typedef uint16_t (*ascc_get_target_count_stub_t)(void);

/**
 * @brief Verifies that given target is valid per the specification. Subset of the schedule validation
 * operation but this is more intended for use in Gets where the schedule information is unknown.
 *
 * @param target Const pointer to target to be verified
 *
 * @return true if target is valid.
 *         false if target is invalid.
 */
typedef bool (*ascc_target_validation_stub_t)(const ascc_target_t * const target);

/**
 * @brief Verifies that given schedule slot is valid for the registered CC and given type.
 *
 * @param target_ID   Target ID
 * @param type        Schedule type (Year Day/Daily Repeating)
 * @param slot        Schedule slot
 *
 * @return true if target is valid.
 *         false if target is invalid.
 */
typedef bool (*ascc_schedule_slot_validation_stub_t)(const uint16_t target_ID,
                                                     const ascc_type_t type,
                                                     const uint16_t slot);

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
typedef bool (*ascc_schedule_data_validation_stub_t)(const ascc_schedule_t * const schedule);

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
typedef ascc_op_result_t (*ascc_get_schedule_state_stub_t)(const ascc_target_t * const target,
                                                           bool * state);

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
typedef ascc_op_result_t (*ascc_set_schedule_state_stub_t)(const ascc_target_t * const target,
                                                           const bool state);

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
typedef ascc_op_result_t (*ascc_get_schedule_data_stub_t)(const ascc_type_t schedule_type,
                                                          const uint16_t slot,
                                                          const ascc_target_t * const target,
                                                          ascc_schedule_data_t * schedule,
                                                          uint16_t * next_slot);
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
typedef ascc_op_result_t (*ascc_set_schedule_data_stub_t)(const ascc_op_type_t operation,
                                                          const ascc_schedule_t * const schedule,
                                                          uint16_t * next_slot);

/**
 * @brief By design, this command class has zero visibility into any other command classes that
 * use it. Therefore, it's a lot easier for it to just have an array of stubs that
 * get registered by the 'scheduled' ccs - each CC *may* have slight variations on
 * how operations are processed, so baking in a single definition at compile time is
 * not an ideal solution for ensuring maximum compatibility.
 */
typedef struct _ascc_target_stubs {
  ascc_get_schedule_count_stub_t get_schedule_count;
  ascc_get_target_count_stub_t get_target_count;
  ascc_get_schedule_data_stub_t get_schedule_data;
  ascc_get_schedule_state_stub_t get_schedule_state;
  ascc_set_schedule_data_stub_t set_schedule_data;
  ascc_set_schedule_state_stub_t set_schedule_state;
  ascc_target_validation_stub_t validate_target;
  ascc_schedule_slot_validation_stub_t validate_schedule_slot;
  ascc_schedule_data_validation_stub_t validate_schedule_data;
} ascc_target_stubs_t;

/****************************************************************************
*                             EXTERNAL DATA                                *
****************************************************************************/

/****************************************************************************
*                     EXPORTED FUNCTION DEFINITIONS                        *
****************************************************************************/

#endif /* _COMMANDCLASSACTIVESCHEDULETYPES_H_ */
