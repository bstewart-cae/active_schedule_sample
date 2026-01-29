/*
 * SPDX-License-Identifier: BSD-3-Clause
 * SPDX-FileCopyrightText: 2025 Z-Wave Alliance <https://z-wavealliance.org>
 * SPDX-FileCopyrightText: 2025 Card Access Engineering, LLC. <https://www.caengineering.com>
 */
/**
 * @file CC_ActiveSchedule.h
 * Function and data definitions for the Active Schedule CC handler code.
 *
 * @addtogroup CC
 * @{
 * @addtogroup Active Schedule
 * @{
 *
 * The Active Schedule CC is an evolution of the Schedule Entry CC that allows
 * scheduling of various constructs within the Z-Wave ecosystem.
 * While Schedule Entry Lock was limited to User Codes, Active Schedule
 * can be used by any CC to schedule some construct within its purview.
 *
 * The Active Schedule CC consists of Daily Repeating and Year-Day Schedule
 * Slots tied to Targets - a Target is identifed using the command class
 * that manages it and some optional identifier in the case that the CC
 * in question is managing multiple instances of a potential Target.
 *
 * The specifications of the Active Schedule CC and Schedule Entry Lock CC can be found in
 * https://github.com/Z-Wave-Alliance/AWG/
 *
 * @}
 * @}
 *
 *
 */

/****************************************************************************
*                              INCLUDE FILES                               *
****************************************************************************/
#include "CC_ActiveSchedule_types.h"
#include "ZW_TransportEndpoint.h"

/****************************************************************************
*                            EXPORTED TYPES                                *
****************************************************************************/

/****************************************************************************
*                     EXPORTED FUNCTION DEFINITIONS                        *
****************************************************************************/
/**
 * @brief Retrieves current frame information for caching on application layer
 *
 * @param[out] rx_opts Pointer to RX option struct into which the current RX options
 *                     are copied.
 *
 * @returns True if a command is currently is being handled
 *          False if a command is NOT currently being handled
 *
 */
bool CC_ActiveSchedule_Get_Current_Frame_Options(RECEIVE_OPTIONS_TYPE_EX * rx_opts);

/**
 * @brief Transmit a Schedule Enable Report frame back to the sending node defined in
 *        rx_opts.
 *
 * @param report_type Defines how the operation was initially triggered
 * @param target      Target described by the included state
 * @param enabled     True if target is valid and has schedules enabled, false otherwise.
 * @param rx_opts     RX options to retrieve sending node.
 */
void CC_ActiveSchedule_Enable_Report_tx(
  const ascc_report_type_t report_type,
  const ascc_target_t * target,
  const bool enabled,
  RECEIVE_OPTIONS_TYPE_EX * rx_opts);

/**
 * @brief Transmit a Year Day Schedule Report frame back to the sending node
 *        defined in rx_opts.
 *
 * Made public so asynchronous applications have access to the function.
 *
 * @param report_type        Defines how the operation was initially triggered
 * @param schedule           Pointer to schedule data to be transmitted
 * @param next_schedule_slot The next occupied schedule slot for a given type and target
 * @param rx_opts            Pointer for RX options that triggered the initial operation
 */
void CC_ActiveSchedule_YearDay_Schedule_Report_tx(
  const ascc_report_type_t report_type,
  const ascc_schedule_t * schedule,
  const uint16_t next_schedule_slot,
  RECEIVE_OPTIONS_TYPE_EX * rx_opts);

/**
 * @brief Transmit a Daily Repeating Schedule Report frame back to the sending node
 *        defined in rx_opts.
 *
 * Made public so asynchronous applications have access to the function.
 *
 * @param report_type        Defines how the operation was initially triggered
 * @param schedule           Pointer to schedule data to be transmitted
 * @param next_schedule_slot The next occupied schedule slot for a given type and target
 * @param rx_opts            Pointer for RX options that triggered the initial operation
 */
void CC_ActiveSchedule_DailyRepeating_Schedule_Report_tx(
  const ascc_report_type_t report_type,
  const ascc_schedule_t * schedule,
  const uint16_t next_schedule_slot,
  RECEIVE_OPTIONS_TYPE_EX * rx_opts);
