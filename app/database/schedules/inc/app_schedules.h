/*
 * SPDX-FileCopyrightText: 2025 Z-Wave Alliance <https://z-wavealliance.org>
 * SPDX-FileCopyrightText: 2026 Card Access Engineering, LLC <http://www.caengineering.com>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file app_schedules.h
 * @author bstewart-cae
 * @brief This file contains the application level logic for the scheduling
 *        implementation of User Credential v2.
 *
 * @copyright 2026 Card Access Engineering, LLC on behalf of the Z-Wave Alliance
 */

#ifndef _APP_SCHEDULES_H_
#define _APP_SCHEDULES_H_

/* CPP type safety */
#ifdef __cplusplus
extern "C" {
#endif

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include "database_types_ascc.h"
/****************************************************************************/
/*                      EXPORTED TYPES AND DEFINITIONS                      */
/****************************************************************************/

/****************************************************************************/
/*                      EXPORTED FUNCTION DECLARATIONS                      */
/****************************************************************************/

/**
 * @brief Active Schedule CC requires that separate function stubs be provided
 *        to the stack so for each CC that is scheduled.
 */
void app_sch_initialize_handlers(void);

/**
 * @brief Clear all stored schedule information
 */
void app_sch_reset_schedules(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* _APP_SCHEDULES_H_ */
