/*
 * SPDX-FileCopyrightText: 2025 Z-Wave Alliance <https://z-wavealliance.org/>
 * SPDX-FileCopyrightText: 2025 Card Access Engineering, LLC <https://www.caengineering.com/>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 *
 */
/**
 * @file CC_Version.h
 * This file defines the public CC_Version functions. The functions declared
 * by this API are NOT stubbed by the test system and/or defined weakly,
 * and are instead defined in CC_Version.c
 *
 */

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include "cc_version_migration_types.h"

/****************************************************************************/
/*                       PUBLIC TYPES and DEFINITIONS                       */
/****************************************************************************/

/****************************************************************************/
/*                      EXPORTED FUNCTION DEFINITIONS                       */
/****************************************************************************/

/**
 * @brief Retrieves the RX options for the Version CC command that is currently
 *        being processed.
 *
 * @param[out] rx_opts Pointer to struct into which options are copied
 *
 * @returns true if command is currently being processed and RX options exist,
 *          false otherwise
 */
bool CC_Version_GetCurrentRxOpts(RECEIVE_OPTIONS_TYPE_EX *rx_opts);
