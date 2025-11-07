/// ***************************************************************************
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2024 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************

/**
 * @file zniffer_app.h
 * @brief Zniffer application
 */
#ifndef _ZNIFFER_APP_H_
#define _ZNIFFER_APP_H_

#include <zpal_power_manager.h>

/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/

/**
 * @brief Size of allocated memory for the application task stack.
 *
 */
#define APPLICATION_TASK_STACK              ((16 * 1024) / (sizeof(StackType_t)))
#define APPLICATION_TASK_PRIORITY_STACK     ( TASK_PRIORITY_MAX - 10 )  ///< High, due to time critical protocol activity.

/****************************************************************************/
/*                            EXPORTED FUNCTIONS                            */
/****************************************************************************/

/**
 * @brief Zniffer Task.
 *
 * @param unused_prt
 */
void
ZwaveZnifferTask(void* unused_prt);


#endif  /* _ZNIFFER_APP_H_ */
