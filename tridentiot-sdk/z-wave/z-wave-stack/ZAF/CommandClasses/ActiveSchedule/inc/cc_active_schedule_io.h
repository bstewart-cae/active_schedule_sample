/*
 * SPDX-License-Identifier: BSD-3-Clause
 * SPDX-FileCopyrightText: 2025 Z-Wave Alliance <https://z-wavealliance.org>
 * SPDX-FileCopyrightText: 2025 Card Access Engineering, LLC. <https://www.caengineering.com>
 */
/**
 * @file cc_active_schedule_io.h
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
 * This file defines the IO related functions of Active Schedule, broken out for ctest reasons.
 *
 * The specifications of the Active Schedule CC and Schedule Entry Lock CC can be found in any AWG spec release
 * from 2025B onward.
 *
 * Link to source: https://github.com/Z-Wave-Alliance/AWG
 *
 * @}
 * @}
 *
 *
 *
 */

 #include "CC_ActiveSchedule_types.h"

/**
 * @brief This registers a block of callback pointers for ASCC to use to schedule a particular
 * command class when those commands are received by the controller.
 *
 * These callbacks are copied into an internal buffer for use, so the struct passed in need
 * not be retained.
 *
 * @param command_class_id The ID of the command class that uses this particular set of callbacks.
 * @param callbacks        Pointer to a collection of callback pointers to be copied into ASCC's internal
 *                         buffer
 */
void CC_ActiveSchedule_RegisterCallbacks(uint8_t command_class_id,
                                         const ascc_target_stubs_t * callbacks);
