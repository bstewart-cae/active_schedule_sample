/*
 * SPDX-License-Identifier: BSD-3-Clause
 * SPDX-FileCopyrightText: 2025 Card Access Engineering, LLC.
 */
/**
 * @file CC_ActiveSchedule.c
 * Implementation of the Active Schedule CC handler code.
 *
 * This code is designed to be as abstract as possible, as the
 * actual implementation of the scheduling framework and even
 * which command classes are using this CC is not intended to
 * even be known by the handler.
 *
 * Because of the way that the specification is written, it
 * makes far more sense to offload most of the processing and
 * validation of targets and schedules to their respective command
 * classes.
 *
 * This handler uses a system of anonymous function registry to
 * assign handlers based on the Target CC, to which control is passed
 * after the Scheduling information is validated to
 * functions defined within that command class to validate the target
 * and process any schedule management operations.
 *
 */

/****************************************************************************
*                              INCLUDE FILES                               *
****************************************************************************/
// Comment out line below to get rid of assert errors in the open source SDK
#define OPEN_SOURCE_ASSERT

#ifdef OPEN_SOURCE_ASSERT
#include <Assert.h>
#else
#include <assert.h>
#endif

#include <string.h>
#include "CC_ActiveSchedule.h"
#include "cc_active_schedule_config_api.h"
#include "cc_active_schedule_io.h"
#include <ZW_TransportEndpoint.h>
#include <ZAF_types.h>
#include <ZAF_TSE.h>
#include <zaf_event_distributor_soc.h>
#include <zaf_transport_tx.h>

/****************************************************************************
*                      PRIVATE TYPES and DEFINITIONS                       *
****************************************************************************/

#ifndef OPEN_SOURCE_ASSERT
#define ASSERT assert
#endif

/**
 * Simple helper macro, all frames with a Target use this same structure and logic
 * but it isn't located in the same spot in the frame
 */
#define ASCC_TARGET_FROM_FRAME(in_frame_ptr)               \
  {                                                        \
    .target_cc = in_frame_ptr->targetCc,                   \
    .target_id = (uint16_t)((in_frame_ptr->targetId1 << 8) \
                            | in_frame_ptr->targetId2)     \
  }

/**
 * Helper macro to get the schedule slot ID from the frame
 */
#define ASCC_SLOT_ID_FROM_FRAME(in_frame_ptr)       \
  ( (uint16_t)((in_frame_ptr->scheduleSlotId1 << 8) \
               | in_frame_ptr->scheduleSlotId2) )

/**
 * @brief Building block for the handler map.
 *
 * Because ASCC is technically capable of 'scheduling' literally any other CC,
 * a CC can provide a block of handlers that are called by the ZAF when a
 * command is received.
 */
typedef struct _ascc_handler_collection_ {
  uint8_t cc_id;
  ascc_target_stubs_t callbacks;
} ascc_handler_collection_t;

/****************************************************************************
*                             EXTERNAL DATA                                *
****************************************************************************/

/****************************************************************************
*                   PRIVATE FUNCTION FORWARD DECLARATIONS                  *
****************************************************************************/
/* Primary application and stack interface handlers */
static void ascc_event_handler(const uint8_t event,
                               const void * p_data);
static received_frame_status_t CC_ActiveSchedule_handler(cc_handler_input_t * input,
                                                         cc_handler_output_t * output);
/* Inidividual command handlers*/
static received_frame_status_t CC_ActiveSchedule_CapabilitesGet_handler(cc_handler_output_t * output);
static received_frame_status_t CC_ActiveSchedule_EnableSet_handler(cc_handler_input_t * input);
static received_frame_status_t CC_ActiveSchedule_EnableGet_handler(cc_handler_input_t * input,
                                                                   cc_handler_output_t * output);
static received_frame_status_t CC_ActiveSchedule_YearDaySet_handler(cc_handler_input_t * input,
                                                                    cc_handler_output_t * output);
static received_frame_status_t CC_ActiveSchedule_YearDayGet_handler(cc_handler_input_t * input,
                                                                    cc_handler_output_t * output);
static received_frame_status_t CC_ActiveSchedule_DailyRepeatingSet_handler(cc_handler_input_t * input,
                                                                           cc_handler_output_t * output);
static received_frame_status_t CC_ActiveSchedule_DailyRepeatingGet_handler(cc_handler_input_t * input,
                                                                           cc_handler_output_t * output);

/* General helper Functions */
static bool get_cc_map_index(const uint8_t cc_id, uint8_t * index);
static bool get_stubs_by_cc(const uint8_t cc_id,
                            ascc_target_stubs_t **stubs);
static void pack_daily_repeating_report_frame(
  const ascc_report_type_t report_type,
  const ascc_schedule_t *schedule,
  const uint16_t next_schedule_slot,
  ZW_ACTIVE_SCHEDULE_DAILY_REPEATING_SCHEDULE_REPORT_1BYTE_FRAME *out_frame,
  uint8_t *out_frame_len);
static void pack_enable_report_frame(
  const ascc_report_type_t report_type,
  const ascc_target_t *target,
  bool enabled,
  ZW_ACTIVE_SCHEDULE_ENABLE_REPORT_FRAME * frame,
  uint8_t *out_frame_len);
static void pack_year_day_report_frame(
  const ascc_report_type_t report_type,
  const ascc_schedule_t* schedule,
  const uint16_t next_schedule_slot,
  ZW_ACTIVE_SCHEDULE_YEAR_DAY_SCHEDULE_REPORT_1BYTE_FRAME *out_frame,
  uint8_t *out_frame_len);

/* Validation and operation helper functions */
static bool validate_target(const ascc_target_t * const target,
                            ascc_target_stubs_t **out_handlers);
static ascc_op_result_t validate_and_get_schedule(ascc_schedule_t * schedule,
                                                  uint16_t * next_schedule_slot);
static received_frame_status_t validate_and_set_schedule(ascc_op_type_t operation,
                                                         const ascc_schedule_t * const schedule,
                                                         uint16_t * next_schedule_slot,
                                                         uint8_t *duration);

static void send_report_tse(zaf_tx_options_t * p_tx_options,
                            void * p_data);
static void send_report(const cc_handler_input_t * const in_report,
                        cc_handler_input_t * tse_report,
                        const bool notify_lifeline);

/* Inline helpers */
static inline uint8_t * pack_two_byte(uint8_t *byteptr, const uint16_t value)
{
#if __GNUC__
#if (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
  *((uint16_t*)byteptr++) = (uint16_t)value;
  byteptr++;
#elif (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
  *byteptr++ = (uint8_t)((value >> 8) & 0xFF);
  *byteptr++ = (uint8_t)(value & 0xFF);
#endif /* __BYTE_ORDER__ */
#endif /* __GNUC__ */
  return byteptr;
}

/****************************************************************************
*                              PRIVATE DATA                                *
****************************************************************************/

/**
 * @brief One off, static struct that holds all of the external cc handler information.
 */
static struct {
  uint8_t num_ccs_registered;
  ascc_handler_collection_t callbacks[CC_ACTIVE_SCHEDULE_MAX_NUM_SUPPORTED_CCS];
} b_cc_handler_map;

/**
 * @brief Cached RX options of current frame.
 *
 * @note Automatically set to null upon handler exit.
 */
static RECEIVE_OPTIONS_TYPE_EX * b_rx_opts;

/**
 * @brief Cached TSE RX options for triggering
 */
static RECEIVE_OPTIONS_TYPE_EX tse_current_rx_opts;

/* Statically allocated output buffer for lifeline report transmission */
static ZW_APPLICATION_TX_BUFFER yd_report_buf;
static ZW_APPLICATION_TX_BUFFER dr_report_buf;
static ZW_APPLICATION_TX_BUFFER se_report_buf;

static cc_handler_input_t tse_sched_yd_report = { 0 };
static cc_handler_input_t tse_sched_dr_report = { 0 };
static cc_handler_input_t tse_sched_enable_report = { 0 };

/****************************************************************************
*                  EXPORTED HEADER FUNCTION DEFINITIONS                    *
****************************************************************************/
bool CC_ActiveSchedule_Get_Current_Frame_Options(RECEIVE_OPTIONS_TYPE_EX * rx_opts)
{
  if (b_rx_opts
      && rx_opts) {
    *rx_opts = *b_rx_opts;
    return true;
  }
  return false;
}

void CC_ActiveSchedule_Enable_Report_tx(const ascc_report_type_t report_type,
                                        const ascc_target_t * target,
                                        const bool enabled,
                                        RECEIVE_OPTIONS_TYPE_EX * rx_opts)
{
  cc_handler_input_t out_frame = {
    .rx_options = rx_opts,
    .frame = &se_report_buf,
  };

  pack_enable_report_frame(
    report_type,
    target,
    enabled,
    (ZW_ACTIVE_SCHEDULE_ENABLE_REPORT_FRAME *)&out_frame.frame->
    ZW_ActiveScheduleEnableReportFrame,
    &out_frame.length);

  /**
   * Determine whether nodes in the the Lifeline association group must be
   * notified.
   * This only applies to successful database modifications.
   */
  bool notify_lifeline =
    (report_type == ASCC_REP_TYPE_MODIFY_EXTERNAL)
    || (report_type == ASCC_REP_TYPE_MODIFY_ZWAVE);

  send_report(&out_frame,
              &tse_sched_enable_report,
              notify_lifeline);
}

void CC_ActiveSchedule_YearDay_Schedule_Report_tx(
  const ascc_report_type_t report_type,
  const ascc_schedule_t * schedule,
  const uint16_t next_schedule_slot,
  RECEIVE_OPTIONS_TYPE_EX * rx_opts)
{
  cc_handler_input_t out_frame = {
    .rx_options = rx_opts,
    .frame = &yd_report_buf
  };

  pack_year_day_report_frame(
    report_type,
    schedule,
    next_schedule_slot,
    (ZW_ACTIVE_SCHEDULE_YEAR_DAY_SCHEDULE_REPORT_1BYTE_FRAME *)&out_frame.frame->
    ZW_ActiveScheduleYearDayScheduleReport1byteFrame,
    &out_frame.length);

  /**
   * Determine whether nodes in the the Lifeline association group must be
   * notified.
   * This only applies to successful database modifications.
   */
  bool notify_lifeline =
    (report_type == ASCC_REP_TYPE_MODIFY_EXTERNAL)
    || (report_type == ASCC_REP_TYPE_MODIFY_ZWAVE);

  send_report(&out_frame,
              &tse_sched_yd_report,
              notify_lifeline);
}

void CC_ActiveSchedule_DailyRepeating_Schedule_Report_tx(
  const ascc_report_type_t report_type,
  const ascc_schedule_t * schedule,
  const uint16_t next_schedule_slot,
  RECEIVE_OPTIONS_TYPE_EX * rx_opts)
{
  cc_handler_input_t out_frame = {
    .rx_options = rx_opts,
    .frame = &dr_report_buf,
  };

  pack_daily_repeating_report_frame(
    report_type,
    schedule,
    next_schedule_slot,
    (ZW_ACTIVE_SCHEDULE_DAILY_REPEATING_SCHEDULE_REPORT_1BYTE_FRAME *)&out_frame.frame->
    ZW_ActiveScheduleDailyRepeatingScheduleReport1byteFrame,
    &out_frame.length);

  /**
   * Determine whether nodes in the the Lifeline association group must be
   * notified.
   * This only applies to successful database modifications.
   */
  bool notify_lifeline =
    (report_type == ASCC_REP_TYPE_MODIFY_EXTERNAL)
    || (report_type == ASCC_REP_TYPE_MODIFY_ZWAVE);

  send_report(&out_frame,
              &tse_sched_dr_report,
              notify_lifeline);
}

ZW_WEAK void CC_ActiveSchedule_RegisterCallbacks(uint8_t command_class_id,
                                                 const ascc_target_stubs_t * callbacks)
{
  // Error out if map is full
  ASSERT(b_cc_handler_map.num_ccs_registered < cc_active_schedule_get_num_supported_ccs());

  uint8_t index = 0;
  uint8_t cc_value = 0;
  /*
   * Find next open slot in the loop, or overwrite existing handlers
   * if the CC has already been registered
   */
  do {
    cc_value = b_cc_handler_map.callbacks[index++].cc_id;
  } while (cc_value != 0
           && cc_value != command_class_id
           && index < cc_active_schedule_get_num_supported_ccs());
  /*
   * Decrement index by one as it will always pop out of the above loop as
   * targetindex + 1.
   */
  --index;
  ASSERT(index < cc_active_schedule_get_num_supported_ccs());

  ascc_handler_collection_t * handler = &b_cc_handler_map.callbacks[index];
  memcpy(&handler->callbacks, callbacks, sizeof(ascc_target_stubs_t));
  handler->cc_id = command_class_id;
  // Increment number of command classes registered ONLY IF this is a new CC
  if (cc_value == 0) {
    b_cc_handler_map.num_ccs_registered++;
  }
}

/****************************************************************************
*                     EXPORTED IO FUNCTION DEFINITIONS
*
* Some of these functions require access to static, private data in this file
* to respond appropriately which is why they are defined here, instead of
* elsewhere in a separate file.
****************************************************************************/
ZW_WEAK uint8_t cc_active_schedule_get_num_supported_ccs(void)
{
  return CC_ACTIVE_SCHEDULE_MAX_NUM_SUPPORTED_CCS;
}

ZW_WEAK bool cc_active_schedule_is_cc_supported(const uint8_t cc_id)
{
  /*
   * Return true if it's in the map, we don't care
   * what the index is in this case
   */
  return get_cc_map_index(cc_id, NULL);
}

/****************************************************************************
*                     PRIVATE HELPER FUNCTION DEFINITIONS                  *
****************************************************************************/

/**
 * @brief Get the index for the handlers of a given CC in the map.
 *
 * @param cc_id      Command Class identifier
 * @param[out] index Optional pointer to the index of the CC if found
 * @return true      Command Class ID found in the map
 * @return false     Command Class ID not found in the map
 */
static bool get_cc_map_index(uint8_t cc_id, uint8_t * index)
{
  for (int i = 0; i < CC_ACTIVE_SCHEDULE_MAX_NUM_SUPPORTED_CCS; i++) {
    if (b_cc_handler_map.callbacks[i].cc_id == cc_id) {
      if (index) {
        *index = i;
      }
      return true;
    }
  }
  return false;
}

/**
 * @brief This function returns whether or not IO stubs exist for a given cc,
 *        and if so, returns a pointer to them.
 *
 * @param cc_id      Command Class identifier
 * @param[out] stubs Address to stub pointer. Set to NULL if the CC isn't in the map
 *                   or no stubs are registered for the CC.
 */
static bool get_stubs_by_cc(const uint8_t cc_id,
                            ascc_target_stubs_t **stubs)
{
  bool result = false;
  uint8_t index;
  if (get_cc_map_index(cc_id, &index)) {
    ascc_target_stubs_t * b_stubs = &b_cc_handler_map.callbacks[index].callbacks;
    if (b_stubs) {
      *stubs = b_stubs;
      result = true;
    } else {
      *stubs = NULL;
    }
  }
  return result;
}

/**
 * @brief Packs a Daily Repeating Schedule report frame with incoming data.
 *
 * @param report_type        Value to populate report type field
 * @param schedule           Schedule data for report
 * @param next_schedule_slot Next schedule slot value for report
 * @param[out] out_frame     Pointer for buffer into which to pack the report
 * @param[out] out_frame_len Pointer with which to output the length of the packed report
 */
static void pack_daily_repeating_report_frame(
  const ascc_report_type_t report_type,
  const ascc_schedule_t *schedule,
  const uint16_t next_schedule_slot,
  ZW_ACTIVE_SCHEDULE_DAILY_REPEATING_SCHEDULE_REPORT_1BYTE_FRAME *out_frame,
  uint8_t *out_frame_len)
{
  const ascc_daily_repeating_schedule_t * daily_repeating =
    &schedule->data.schedule.daily_repeating;
  ASSERT(schedule->type == ASCC_TYPE_DAILY_REPEATING);

  out_frame->cmdClass = COMMAND_CLASS_ACTIVE_SCHEDULE;
  out_frame->cmd = ACTIVE_SCHEDULE_DAILY_REPEATING_SCHEDULE_REPORT;
  out_frame->properties1 = (uint8_t)(report_type
                                     & ACTIVE_SCHEDULE_DAILY_REPEATING_SCHEDULE_REPORT_PROPERTIES1_RESERVED_MASK);
  out_frame->targetCc = schedule->target.target_cc;
  out_frame->targetId1 = (uint8_t)((schedule->target.target_id >> 8) & 0xFF);
  out_frame->targetId2 = (uint8_t)((schedule->target.target_id) & 0xFF);
  out_frame->scheduleSlotId1 = (uint8_t)((schedule->slot_id >> 8) & 0xFF);
  out_frame->scheduleSlotId2 = (uint8_t)((schedule->slot_id) & 0xFF);
  out_frame->nextScheduleSlotId1 = (uint8_t)(next_schedule_slot >> 8);   // MSB
  out_frame->nextScheduleSlotId2 = (uint8_t)next_schedule_slot;   // LSB

  /* Schedule type specific data */
  out_frame->weekDayBitmask = daily_repeating->weekday_mask;
  out_frame->startHour = daily_repeating->start_hour;
  out_frame->startMinute = daily_repeating->start_minute;
  out_frame->durationHour = daily_repeating->duration_hour;
  out_frame->durationMinute = daily_repeating->duration_minute;

  /* Metadata information */
  out_frame->properties1 = (uint8_t)(schedule->data.metadata_length
                                     & ACTIVE_SCHEDULE_DAILY_REPEATING_SCHEDULE_REPORT_PROPERTIES2_METADATA_LENGTH_MASK);
  if (schedule->data.metadata_length > 0) {
    memcpy(&out_frame->metadata1, &schedule->data.metadata[0], schedule->data.metadata_length);
  }

  *out_frame_len = sizeof(ZW_ACTIVE_SCHEDULE_YEAR_DAY_SCHEDULE_REPORT_1BYTE_FRAME)
                   + schedule->data.metadata_length - 1;
}

/**
 * @brief Packs an Enabled Report frame based on incoming information
 *
 * @param report_type        Value to populate report type field
 * @param target             Pointer to target information
 * @param enabled            Whether or not the schedules for that target are enabled
 * @param[out] out_frame     Pointer for buffer into which to pack the report
 * @param[out] out_frame_len Pointer with which to output the length of the packed report
 */
static void pack_enable_report_frame(
  const ascc_report_type_t report_type,
  const ascc_target_t *target,
  bool enabled,
  ZW_ACTIVE_SCHEDULE_ENABLE_REPORT_FRAME * out_frame,
  uint8_t *out_frame_len)
{
  out_frame->cmdClass = COMMAND_CLASS_ACTIVE_SCHEDULE;
  out_frame->cmd = ACTIVE_SCHEDULE_ENABLE_REPORT;
  out_frame->properties1 = (((uint8_t)report_type <<
                             ACTIVE_SCHEDULE_ENABLE_REPORT_PROPERTIES1_REPORT_CODE_SHIFT)
                            & ACTIVE_SCHEDULE_ENABLE_REPORT_PROPERTIES1_REPORT_CODE_MASK)
                           | (uint8_t)(enabled & ACTIVE_SCHEDULE_ENABLE_REPORT_PROPERTIES1_ENABLED_BIT_MASK);
  out_frame->targetCc = target->target_cc;
  out_frame->targetId1 = (uint8_t)((target->target_id >> 8) & 0xFF);
  out_frame->targetId2 = (uint8_t)((target->target_id) & 0xFF);

  *out_frame_len = sizeof(ZW_ACTIVE_SCHEDULE_ENABLE_REPORT_FRAME);
}

/**
 * @brief Packs a Year Day Report frame based on incoming information
 *
 * @param report_type        Value to populate report type field
 * @param schedule           Pointer to schedule information
 * @param next_schedule_slot Next occupied slot for the given target embedded in the schedule
 * @param[out] out_frame     Pointer for buffer into which to pack the report
 * @param[out] out_frame_len Pointer with which to output the length of the packed report
 */
static void pack_year_day_report_frame(
  const ascc_report_type_t report_type,
  const ascc_schedule_t* schedule,
  const uint16_t next_schedule_slot,
  ZW_ACTIVE_SCHEDULE_YEAR_DAY_SCHEDULE_REPORT_1BYTE_FRAME *out_frame,
  uint8_t *out_frame_len)
{
  const ascc_year_day_schedule_t * year_day = &schedule->data.schedule.year_day;
  ASSERT(schedule->type == ASCC_TYPE_YEAR_DAY);

  out_frame->cmdClass = COMMAND_CLASS_ACTIVE_SCHEDULE;
  out_frame->cmd = ACTIVE_SCHEDULE_YEAR_DAY_SCHEDULE_REPORT;
  out_frame->properties1 = (uint8_t)(report_type
                                     & ACTIVE_SCHEDULE_YEAR_DAY_SCHEDULE_REPORT_PROPERTIES1_RESERVED_MASK);
  out_frame->targetCc = schedule->target.target_cc;
  out_frame->targetId1 = (uint8_t)((schedule->target.target_id >> 8) & 0xFF);
  out_frame->targetId2 = (uint8_t)((schedule->target.target_id) & 0xFF);
  out_frame->scheduleSlotId1 = (uint8_t)(schedule->slot_id >> 8);
  out_frame->scheduleSlotId2 = (uint8_t)schedule->slot_id;
  out_frame->nextScheduleSlotId1 = (uint8_t)(next_schedule_slot >> 8);   // MSB
  out_frame->nextScheduleSlotId2 = (uint8_t)next_schedule_slot;   // LSB

  /* Schedule type specific data */
  out_frame->startMinute = year_day->start_minute;
  out_frame->startHour = year_day->start_hour;
  out_frame->startDay = year_day->start_day;
  out_frame->startMonth = year_day->start_month;
  out_frame->startYear1 = (uint8_t)((year_day->start_year >> 8) & 0xFF);
  out_frame->startYear2 = (uint8_t)(year_day->start_year & 0xFF);
  out_frame->stopMinute = year_day->stop_minute;
  out_frame->stopHour = year_day->stop_hour;
  out_frame->stopDay = year_day->stop_day;
  out_frame->stopMonth = year_day->stop_month;
  out_frame->stopYear1 = (uint8_t)((year_day->stop_year >> 8) & 0xFF);
  out_frame->stopYear2 = (uint8_t)(year_day->stop_year & 0xFF);

  /* Metadata information */
  out_frame->properties2 = (uint8_t)(schedule->data.metadata_length
                                     & ACTIVE_SCHEDULE_YEAR_DAY_SCHEDULE_REPORT_PROPERTIES2_METADATA_LENGTH_MASK);
  if (schedule->data.metadata_length > 0) {
    memcpy(&out_frame->metadata1, &schedule->data.metadata[0], schedule->data.metadata_length);
  }

  *out_frame_len = sizeof(ZW_ACTIVE_SCHEDULE_YEAR_DAY_SCHEDULE_REPORT_1BYTE_FRAME)
                   + schedule->data.metadata_length - 1;
}

/**
 * @brief This helper function combines the operations verifying that
 * a particular Target CC is supported, retrieving the appropriate handler
 * collection for a CC and verifying that the target ID provided is
 * also valid.
 *
 * @param target Pointer to target information
 * @param[out] out_handlers Returns a pointer to the registered handler collection.
 *                          MUST NOT be NULL.
 * @return true  If Target CC is supported, has registerd callbacks and
 *               the Target ID is valid.
 * @return false If any of the above fails
 *
 */
static bool validate_target(const ascc_target_t * const target,
                            ascc_target_stubs_t **out_handlers)
{
  bool result = true;
  // Ensure that CC has registered callbacks with the module
  ascc_target_stubs_t * stubs = NULL;
  if (!get_stubs_by_cc(target->target_cc, &stubs)) {
    result = false;
  } else {
    // Failsafe assertions
    ASSERT(out_handlers != NULL);
    // Make sure that the target ID is valid.
    if (stubs->validate_target) {
      result = stubs->validate_target(target);
    }
    *out_handlers = stubs;
  }
  return result;
}

/**
 * @brief Validates schedule slot and retrieves schedule data if available.
 *
 * @param schedule Pointer to schedule type to validate
 * @param[out] next_schedule_slot Pointer with which to output the next occupied schedule slot
 *                                for the provided target embedded in the schedule data.
 * @return ASCC_OPERATION_INVALID_GET if operation is invalid,
 *         otherwise passes through the result of the Get operation.
 */
static ascc_op_result_t validate_and_get_schedule(ascc_schedule_t * schedule,
                                                  uint16_t * next_schedule_slot)
{
  ascc_op_result_t result = {
    .result = ASCC_OPERATION_INVALID_GET
  };
  bool valid = true;
  ascc_target_stubs_t *stubs = NULL;
  if (!validate_target(&schedule->target, &stubs)
      || !stubs) {
    return result;
  }

  if (stubs->validate_schedule_slot) {
    valid &= stubs->validate_schedule_slot(schedule->target.target_id,
                                           schedule->type,
                                           schedule->slot_id);
  }

  if (valid
      && stubs->get_schedule_data) {
    result = stubs->get_schedule_data(schedule->type,
                                      schedule->slot_id,
                                      &schedule->target,
                                      &schedule->data,
                                      next_schedule_slot);
  }
  /* Return empty struct otherwise */
  return result;
}

/**
 * @brief Run and interpret the schedule set operation and its result.
 *
 * @param operation Which database operation needs to be run
 * @param schedule  Schedule data for the operation. May be blank on clear, but must not be NULL.
 * @param[out] next_schedule_slot Pointer with which to output the next occupied schedule slot
 *                                for the provided target embedded in the schedule data.
 * @param[out] duration           Pointer provided to the application that can be used to output
 *                                the expected time duration of the IO operation in seconds.
 * @return received_frame_status_t Return value to trigger appropriate Supervision statuses
 */
static received_frame_status_t validate_and_set_schedule(ascc_op_type_t operation,
                                                         const ascc_schedule_t * const schedule,
                                                         uint16_t * next_schedule_slot,
                                                         uint8_t * duration)
{
  received_frame_status_t status = RECEIVED_FRAME_STATUS_FAIL;
  bool valid = true;
  ascc_target_stubs_t *stubs = NULL;

  /*
   * Some of the following validation steps may be a little redundant,
   * but the implementer of the CC is free to mix and match as needed for
   * their application as these all validate something slightly different
   */
  if (!validate_target(&schedule->target, &stubs)
      || !stubs) {
    return status;
  }

  if (stubs->validate_schedule_slot) {
    valid &= stubs->validate_schedule_slot(schedule->target.target_id,
                                           schedule->type,
                                           schedule->slot_id);
  }

  if (operation != ASCC_OP_TYPE_ERASE &&
      stubs->validate_schedule_data) {
    valid &= stubs->validate_schedule_data(schedule);
  }

  /* Once request is validated, run operation. */
  if (stubs->set_schedule_data
      && valid) {
    ascc_op_result_t result = stubs->set_schedule_data(operation,
                                                       schedule,
                                                       next_schedule_slot);
    switch (result.result) {
      case ASCC_OPERATION_SUCCESS: {
        status = RECEIVED_FRAME_STATUS_SUCCESS;
        break;
      }
      case ASCC_OPERATION_WORKING: {
        status = RECEIVED_FRAME_STATUS_WORKING;
        *duration = result.working_time;
        break;
      }
      default:
        break;
    }
  }
  return status;
}

/****************************************************************************
*                     PRIVATE HANDLER FUNCTION DEFINITIONS                 *
****************************************************************************/

/**
 * @brief This function synchronously handles a Capabilities Get and returns a Capabilities Report
 *        to the sending node.
 *
 * @param input  Input Frame
 * @param output Output Frame
 * @return received_frame_status_t RECEIVED_FRAME_STATUS_NO_SUPPORT if num of supported CCs is == 0
 *                                 RECEIVED_FRAME_STATUS_SUCCESS otherwise
 */
static received_frame_status_t CC_ActiveSchedule_CapabilitesGet_handler(cc_handler_output_t * output)
{
  received_frame_status_t status = RECEIVED_FRAME_STATUS_NO_SUPPORT;
  uint8_t count = b_cc_handler_map.num_ccs_registered;
  if (count > 0) {
    ZW_ACTIVE_SCHEDULE_CAPABILITIES_REPORT_1BYTE_FRAME *p_report = &output->frame->ZW_ActiveScheduleCapabilitiesReport1byteFrame;
    p_report->cmdClass = COMMAND_CLASS_ACTIVE_SCHEDULE;
    p_report->cmd = ACTIVE_SCHEDULE_CAPABILITIES_REPORT;
    p_report->numberOfSupportedTargetCcs = count;
    uint8_t *ptr = (uint8_t*)&p_report->supportedTargetCc1;
    /* List all supported CCs */
    for (int i = 0; i < count; i++) {
      *ptr++ = b_cc_handler_map.callbacks[i].cc_id;
    }
    /* Get and list supported number of targets per CC */
    for (int i = 0; i < count; i++) {
      if (b_cc_handler_map.callbacks[i].callbacks.get_target_count) {
        ptr = pack_two_byte(ptr, b_cc_handler_map.callbacks[i].callbacks.get_target_count());
      } else {
        ptr = pack_two_byte(ptr, 0x00);
      }
    }
    /* Get and list supported number of Year Day Schedules per target */
    for (int i = 0; i < count; i++) {
      if (b_cc_handler_map.callbacks[i].callbacks.get_schedule_count) {
        ptr = pack_two_byte(ptr, b_cc_handler_map.callbacks[i].callbacks.get_schedule_count(ASCC_TYPE_YEAR_DAY));
      } else {
        ptr = pack_two_byte(ptr, 0x00);
      }
    }
    /* Get and list supported number of Daily Repeating Schedules per target */
    for (int i = 0; i < count; i++) {
      if (b_cc_handler_map.callbacks[i].callbacks.get_schedule_count) {
        ptr = pack_two_byte(ptr, b_cc_handler_map.callbacks[i].callbacks.get_schedule_count(ASCC_TYPE_DAILY_REPEATING));
      } else {
        ptr = pack_two_byte(ptr, 0x00);
      }
    }

    output->length = (uint8_t *)ptr - (uint8_t *)p_report;

    status = RECEIVED_FRAME_STATUS_SUCCESS;
  }
  return status;
}

/**
 * @brief Processes an Enable Set command, and returns relevant status information
 *        for Supervision if necessary.
 *
 * @param input Input Frame
 * @return received_frame_status_t RECIEVED_FRAME_STATUS_SUCCESS if IO function returns ASCC_OPERATION_SUCCESS
 *                                 RECEIVED_FRAME_STATUS_WORKING if IO function returns ASCC_OPERATION_WORKING
 *                                 RECEIVED_FRAME_STATUS_FAIL otherwise
 *
 */
static received_frame_status_t CC_ActiveSchedule_EnableSet_handler(cc_handler_input_t * input)
{
  received_frame_status_t status = RECEIVED_FRAME_STATUS_FAIL;
  /* Validate Target */
  ZW_ACTIVE_SCHEDULE_ENABLE_SET_FRAME *in_frame = &input->frame->ZW_ActiveScheduleEnableSetFrame;

  const ascc_target_t target = ASCC_TARGET_FROM_FRAME(in_frame);
  const bool new_state = (bool)(in_frame->properties1 & ACTIVE_SCHEDULE_ENABLE_SET_PROPERTIES1_ENABLED_BIT_MASK);
  ascc_target_stubs_t *stubs = NULL;

  if (validate_target(&target, &stubs)
      && stubs->set_schedule_state) {
    ascc_op_result_t result = stubs->set_schedule_state(&target, new_state);
    switch (result.result) {
      case ASCC_OPERATION_SUCCESS: {
        status = RECEIVED_FRAME_STATUS_SUCCESS;
        break;
      }
      case ASCC_OPERATION_WORKING: {
        status = RECEIVED_FRAME_STATUS_WORKING;
        break;
      }
      default:
        break;
    }
  }

  if (status == RECEIVED_FRAME_STATUS_SUCCESS && input->rx_options) {
    CC_ActiveSchedule_Enable_Report_tx(ASCC_REP_TYPE_MODIFY_ZWAVE,
                                       &target,
                                       new_state,
                                       input->rx_options);
  } else if (!input->rx_options) {
    status = RECEIVED_FRAME_STATUS_FAIL;
  }

  return status;
}

/**
 * @brief Processes an Enable Get command, and returns relevant status information
 *        for Supervision if necessary.
 *
 *
 * @param input Input Frame
 * @return received_frame_status_t RECIEVED_FRAME_STATUS_SUCCESS if IO function returns ASCC_OPERATION_SUCCESS
 *                                 RECEIVED_FRAME_STATUS_WORKING if IO function returns ASCC_OPERATION_WORKING
 *                                 RECEIVED_FRAME_STATUS_FAIL otherwise
 *
 * @note  Will automatically populate Report frame information if status is RECEIVED_FRAME_STATUS_SUCCESS
 */
static received_frame_status_t CC_ActiveSchedule_EnableGet_handler(cc_handler_input_t * input,
                                                                   cc_handler_output_t * output)
{
  received_frame_status_t status = RECEIVED_FRAME_STATUS_FAIL;
  /* Validate Target */
  ZW_ACTIVE_SCHEDULE_ENABLE_SET_FRAME *in_frame = &input->frame->ZW_ActiveScheduleEnableSetFrame;

  const ascc_target_t target = ASCC_TARGET_FROM_FRAME(in_frame);
  ascc_target_stubs_t *stubs = NULL;
  bool state = false;

  if (validate_target(&target, &stubs)
      && stubs->get_schedule_state) {
    ascc_op_result_t result = stubs->get_schedule_state(&target, &state);
    switch (result.result) {
      case ASCC_OPERATION_SUCCESS: {
        ZW_ACTIVE_SCHEDULE_ENABLE_REPORT_FRAME *out_frame =
          &output->frame->ZW_ActiveScheduleEnableReportFrame;
        pack_enable_report_frame(ASCC_REP_TYPE_RESPONSE_TO_GET,
                                 &target,
                                 state,
                                 out_frame,
                                 &output->length);
        output->duration = 0;
        status = RECEIVED_FRAME_STATUS_SUCCESS;
        break;
      }
      case ASCC_OPERATION_WORKING: {
        status = RECEIVED_FRAME_STATUS_WORKING;
        output->duration = result.working_time;
        break;
      }
      default:
        break;
    }
  }
  return status;
}

/**
 * @brief Processes a Year Day Schedule Set command, and returns relevant status information
 *        for Supervision if necessary.
 *
 * @param input Input Frame
 * @return received_frame_status_t RECIEVED_FRAME_STATUS_SUCCESS if IO function returns ASCC_OPERATION_SUCCESS
 *                                 RECEIVED_FRAME_STATUS_WORKING if IO function returns ASCC_OPERATION_WORKING
 *                                 RECEIVED_FRAME_STATUS_FAIL otherwise
 *
 */
static received_frame_status_t CC_ActiveSchedule_YearDaySet_handler(cc_handler_input_t * input,
                                                                    cc_handler_output_t * output)
{
  received_frame_status_t status = RECEIVED_FRAME_STATUS_FAIL;
  uint16_t next_schedule_slot = 0;
  ZW_ACTIVE_SCHEDULE_YEAR_DAY_SCHEDULE_SET_1BYTE_FRAME *in_frame = &input->frame->ZW_ActiveScheduleYearDayScheduleSet1byteFrame;

  ascc_op_type_t operation = (in_frame->properties1 & ACTIVE_SCHEDULE_YEAR_DAY_SCHEDULE_SET_PROPERTIES1_SET_ACTION_MASK);
  ascc_schedule_t schedule = {
    .target = ASCC_TARGET_FROM_FRAME(in_frame),
    .slot_id = ASCC_SLOT_ID_FROM_FRAME(in_frame),
    .type = ASCC_TYPE_YEAR_DAY,
    .data = {
      .schedule.year_day = {
        .start_day = in_frame->startDay,
        .start_month = in_frame->startMonth,
        .start_year = (uint16_t)((in_frame->startYear1 << 8) | in_frame->startYear2),
        .start_hour = in_frame->startHour,
        .start_minute = in_frame->startMinute,
        .stop_day = in_frame->stopDay,
        .stop_month = in_frame->stopMonth,
        .stop_year = (uint16_t)((in_frame->stopYear1 << 8) | in_frame->stopYear2),
        .stop_hour = in_frame->stopHour,
        .stop_minute = in_frame->stopMinute,
      },
      .metadata_length = (in_frame->properties2 ? (in_frame->properties2 & ACTIVE_SCHEDULE_YEAR_DAY_SCHEDULE_SET_PROPERTIES2_METADATA_LENGTH_MASK) : 0x00)
    }
  };

  if (schedule.data.metadata_length > 0) {
    memcpy(&schedule.data.metadata[0],
           &in_frame->metadata1,
           schedule.data.metadata_length);
  }

  /* Validate and handle the set operation. This logic is shared between all schedule types. */
  status = validate_and_set_schedule(operation, &schedule, &next_schedule_slot, &output->duration);

  if (status == RECEIVED_FRAME_STATUS_SUCCESS && input->rx_options) {
    CC_ActiveSchedule_YearDay_Schedule_Report_tx(ASCC_REP_TYPE_MODIFY_ZWAVE,
                                                 &schedule,
                                                 next_schedule_slot,
                                                 input->rx_options);
  } else if (!input->rx_options) {
    status = RECEIVED_FRAME_STATUS_FAIL;
  }

  return status;
}

static received_frame_status_t CC_ActiveSchedule_YearDayGet_handler(cc_handler_input_t * input,
                                                                    cc_handler_output_t * output)
{
  uint16_t next_schedule_slot = 0;
  received_frame_status_t status = RECEIVED_FRAME_STATUS_FAIL;
  ZW_ACTIVE_SCHEDULE_YEAR_DAY_SCHEDULE_GET_FRAME *in_frame = &input->frame->ZW_ActiveScheduleYearDayScheduleGetFrame;

  ascc_schedule_t schedule = {
    .target = ASCC_TARGET_FROM_FRAME(in_frame),
    .slot_id = ASCC_SLOT_ID_FROM_FRAME(in_frame),
    .type = ASCC_TYPE_YEAR_DAY
  };

  ascc_op_result_t result = validate_and_get_schedule(&schedule,
                                                      &next_schedule_slot);

  switch (result.result) {
    case ASCC_OPERATION_SUCCESS: {
      ZW_ACTIVE_SCHEDULE_YEAR_DAY_SCHEDULE_REPORT_1BYTE_FRAME * out_frame =
        &output->frame->ZW_ActiveScheduleYearDayScheduleReport1byteFrame;
      pack_year_day_report_frame(ASCC_REP_TYPE_RESPONSE_TO_GET,
                                 &schedule,
                                 next_schedule_slot,
                                 out_frame,
                                 &output->length);
      status = RECEIVED_FRAME_STATUS_SUCCESS;
      output->duration = 0;
      break;
    }
    case ASCC_OPERATION_WORKING: {
      status = RECEIVED_FRAME_STATUS_WORKING;
      output->duration = result.working_time;
      break;
    }
    default:
      break;
  }

  return status;
}

static received_frame_status_t CC_ActiveSchedule_DailyRepeatingSet_handler(cc_handler_input_t * input,
                                                                           cc_handler_output_t * output)
{
  received_frame_status_t status = RECEIVED_FRAME_STATUS_FAIL;
  uint16_t next_schedule_slot = 0;
  ZW_ACTIVE_SCHEDULE_DAILY_REPEATING_SCHEDULE_SET_1BYTE_FRAME
  *in_frame = &input->frame->ZW_ActiveScheduleDailyRepeatingScheduleSet1byteFrame;

  /* Extract schedule and operation type */
  ascc_op_type_t operation = (in_frame->properties1 & ACTIVE_SCHEDULE_DAILY_REPEATING_SCHEDULE_SET_PROPERTIES1_SET_ACTION_MASK);
  ascc_schedule_t schedule = {
    .target = ASCC_TARGET_FROM_FRAME(in_frame),
    .slot_id = ASCC_SLOT_ID_FROM_FRAME(in_frame),
    .type = ASCC_TYPE_DAILY_REPEATING,
    .data = {
      .schedule.daily_repeating = {
        .weekday_mask = in_frame->weekDayBitmask,
        .start_hour = in_frame->startHour,
        .start_minute = in_frame->startMinute,
        .duration_hour = in_frame->durationHour,
        .duration_minute = in_frame->durationMinute
      },
      .metadata_length = (in_frame->properties2 ? (in_frame->properties2 & ACTIVE_SCHEDULE_DAILY_REPEATING_SCHEDULE_SET_PROPERTIES2_METADATA_LENGTH_MASK) : 0x00)
    }
  };

  if (schedule.data.metadata_length > 0) {
    memcpy(&schedule.data.metadata[0],
           &in_frame->metadata1,
           schedule.data.metadata_length);
  }

  /* Validate and handle the set operation. This logic is shared between all schedule types. */
  status = validate_and_set_schedule(operation, &schedule, &next_schedule_slot, &output->duration);

  if (status == RECEIVED_FRAME_STATUS_SUCCESS && input->rx_options) {
    CC_ActiveSchedule_DailyRepeating_Schedule_Report_tx(ASCC_REP_TYPE_MODIFY_ZWAVE,
                                                        &schedule,
                                                        next_schedule_slot,
                                                        input->rx_options);
  } else if (!input->rx_options) {
    status = RECEIVED_FRAME_STATUS_FAIL;
  }

  return status;
}

static received_frame_status_t CC_ActiveSchedule_DailyRepeatingGet_handler(cc_handler_input_t * input,
                                                                           cc_handler_output_t * output)
{
  uint16_t next_schedule_slot = 0;
  ZW_ACTIVE_SCHEDULE_YEAR_DAY_SCHEDULE_GET_FRAME *in_frame = &input->frame->ZW_ActiveScheduleYearDayScheduleGetFrame;
  received_frame_status_t status = RECEIVED_FRAME_STATUS_FAIL;
  ascc_schedule_t schedule = {
    .target = ASCC_TARGET_FROM_FRAME(in_frame),
    .slot_id = ASCC_SLOT_ID_FROM_FRAME(in_frame),
    .type = ASCC_TYPE_DAILY_REPEATING
  };

  ascc_op_result_t result = validate_and_get_schedule(&schedule,
                                                      &next_schedule_slot);

  switch (result.result) {
    case ASCC_OPERATION_SUCCESS: {
      ZW_ACTIVE_SCHEDULE_DAILY_REPEATING_SCHEDULE_REPORT_1BYTE_FRAME * out_frame =
        &output->frame->ZW_ActiveScheduleDailyRepeatingScheduleReport1byteFrame;
      pack_daily_repeating_report_frame(ASCC_REP_TYPE_RESPONSE_TO_GET,
                                        &schedule,
                                        next_schedule_slot,
                                        out_frame,
                                        &output->length);
      status = RECEIVED_FRAME_STATUS_SUCCESS;
      break;
    }
    case ASCC_OPERATION_WORKING: {
      status = RECEIVED_FRAME_STATUS_WORKING;
      output->duration = result.working_time;
      // TODO: handle supervision
      break;
    }
    default:
      break;
  }

  return status;
}

/**
 * @brief Main stack callback for the Active Schedule command class. This function
 * is called by the ZAF when an Active Schedule command is recieved by another node.
 *
 */
static received_frame_status_t CC_ActiveSchedule_handler(cc_handler_input_t * input,
                                                         cc_handler_output_t * output)
{
  received_frame_status_t status = RECEIVED_FRAME_STATUS_FAIL;
  b_rx_opts = input->rx_options;
  // Check all target-independent function(s) first.
  if (input->frame->ZW_Common.cmd == ACTIVE_SCHEDULE_CAPABILITIES_GET) {
    status = CC_ActiveSchedule_CapabilitesGet_handler(output);
  } else {
    switch (input->frame->ZW_Common.cmd) {
      case ACTIVE_SCHEDULE_ENABLE_SET: {
        status = CC_ActiveSchedule_EnableSet_handler(input);
        break;
      }
      case ACTIVE_SCHEDULE_ENABLE_GET: {
        status = CC_ActiveSchedule_EnableGet_handler(input, output);
        break;
      }
      case ACTIVE_SCHEDULE_DAILY_REPEATING_SCHEDULE_SET: {
        status = CC_ActiveSchedule_DailyRepeatingSet_handler(input, output);
        break;
      }
      case ACTIVE_SCHEDULE_DAILY_REPEATING_SCHEDULE_GET: {
        status = CC_ActiveSchedule_DailyRepeatingGet_handler(input, output);
        break;
      }
      case ACTIVE_SCHEDULE_YEAR_DAY_SCHEDULE_SET: {
        status = CC_ActiveSchedule_YearDaySet_handler(input, output);
        break;
      }
      case ACTIVE_SCHEDULE_YEAR_DAY_SCHEDULE_GET: {
        status = CC_ActiveSchedule_YearDayGet_handler(input, output);
        break;
      }
      default: {
        status = RECEIVED_FRAME_STATUS_NO_SUPPORT;
        break;
      }
    }
  }

  // Reset current RX options
  b_rx_opts = NULL;
  return status;
}

/**
 * Callback function for ZAF TSE to send Active Schedule Reports to multiple
 * destinations
 */
static void send_report_tse(zaf_tx_options_t * p_tx_options,
                            void * p_data)
{
  cc_handler_input_t * report = (cc_handler_input_t *)p_data;
  zaf_transport_tx(
    (uint8_t*)report->frame,
    report->length,
    ZAF_TSE_TXCallback,
    p_tx_options
    );
}

/**
 * Sends an Active Schedule report.
 *
 * @param[in] report          Pointer to the report frame to be sent
 * @param[in] tse_report      Pointer to where a frame can be stored to be sent
 *                            later by the TSE, if needed
 * @param[in] notify_lifeline true if the nodes in the Lifeline association
 *                            group should receive the report
 */
static void send_report(const cc_handler_input_t * const in_report,
                        cc_handler_input_t * tse_report,
                        const bool notify_lifeline)
{
  zaf_tx_options_t tx_options;
  RECEIVE_OPTIONS_TYPE_EX *p_rx_options = in_report->rx_options;
  if (p_rx_options) {
    zaf_transport_rx_to_tx_options(p_rx_options, &tx_options);
  } else {
    return;
  }

  if (p_rx_options->destNode.nodeId == 0) {
    /**
     * If this is an unsolicited Schedule Report, there are no RX options to ensure that
     * the reports can be returned.
     *
     * We need to use association to ensure that this makes it back to the controlling node -
     * recommended that AGI be set to NULL and dest node to 0, which the stack interprets as
     * using Lifeline by default.
     */
    tx_options.agi_profile = NULL;
    tx_options.dest_node_id = 0;
  } else {
    /*
     * In the current TSE implementation the operation source node is never
     * notified via lifeline.
     */
    zaf_transport_tx(
      (uint8_t*)in_report->frame, in_report->length, NULL, &tx_options);
  }

  if (notify_lifeline && tse_report != NULL) {
    // Fill TSE report to be sent later on.
    *tse_report = *in_report;
    ZAF_TSE_Trigger(send_report_tse, tse_report, false);
  }
}

/* No automatic Lifeline reporting to speak of */
REGISTER_CC_V5(COMMAND_CLASS_ACTIVE_SCHEDULE, ACTIVE_SCHEDULE_VERSION,
               CC_ActiveSchedule_handler, NULL, NULL, NULL, 0,
               NULL, NULL);

static void ascc_event_handler(const uint8_t event,
                               const void * p_data)
{
  switch (event) {
    case ASCC_APP_EVENT_ON_SET_SCHEDULE_COMPLETE: {
      const ascc_sched_event_data_t* data = (ascc_sched_event_data_t *)p_data;
      // Avoid const warning
      tse_current_rx_opts = data->rx_opts;
      if (data->schedule.type == ASCC_TYPE_YEAR_DAY) {
        CC_ActiveSchedule_YearDay_Schedule_Report_tx(
          data->report_type,
          &data->schedule,
          data->next_schedule_slot,
          &tse_current_rx_opts);
      } else if (data->schedule.type == ASCC_TYPE_DAILY_REPEATING) {
        CC_ActiveSchedule_DailyRepeating_Schedule_Report_tx(
          data->report_type,
          &data->schedule,
          data->next_schedule_slot,
          &tse_current_rx_opts);
      }

      break;
    }
    case ASCC_APP_EVENT_ON_SET_SCHEDULE_STATE_COMPLETE: {
      RECEIVE_OPTIONS_TYPE_EX rx_opts = { 0 };
      const ascc_sched_enable_event_data_t* data =
        (ascc_sched_enable_event_data_t *)p_data;

      CC_ActiveSchedule_Enable_Report_tx(data->report_type,
                                         &data->target,
                                         data->enabled,
                                         &rx_opts);

      break;
    }
    case ASCC_APP_EVENT_ON_GET_SCHEDULE_CAPABILITIES_COMPLETE:
    case ASCC_APP_EVENT_ON_GET_SCHEDULE_COMPLETE:
    case ASCC_APP_EVENT_ON_GET_SCHEDULE_STATE_COMPLETE:
    case ASCC_APP_EVENT_ON_SCHEDULE_STATE_CHANGE:
    default:
      break;
  }
}

ZAF_EVENT_DISTRIBUTOR_REGISTER_CC_EVENT_HANDLER(COMMAND_CLASS_ACTIVE_SCHEDULE, ascc_event_handler);
