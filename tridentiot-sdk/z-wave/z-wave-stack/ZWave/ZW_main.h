// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file ZW_main.h
 * @copyright 2022 Silicon Laboratories Inc.
 *
 * @brief Z-Wave Main module utilities header.
 */
#ifndef _ZW_MAIN_H_
#define _ZW_MAIN_H_

#include <ZW_typedefs.h>
#include <ZW_basis_api.h>
#include "ZW_application_transport_interface.h"
#include "ZW_protocol_interface.h"
#include <ZW_main_region.h>

/****************************************************************************/
/*                            EXPORTED FUNCTIONS                            */
/****************************************************************************/

/**@brief Function validates current Network HomeID-nodeID and updates if needed
 *
 *    Check if current homeID is 0000 or FFFF and nodeID equals 0 (Slave)
 *    if so then a new random homeID is generated and nodeId = 1 (Controller)
 */
void NetworkIdUpdateValidateNotify(void);

/*=============================   zwave_task   ===============================
**    Z-Wave protocol task
**    This function never returns
**
**--------------------------------------------------------------------------*/
void
ZwaveTask(SApplicationInterface* pAppInterface);

/**
* This function is called from runCycle if there is a change in the security0 inclusion statemachine.
* This in return will trigger calling runCycle agin until there are no chnage in the statemachine.
*/
void
SecurityRunCycleNotify(void);

/**
 * This function is called from ZW_network_management module when node enters and exits SMART START mode
 */
void
ZW_mainDeepSleepPowerLockEnable(bool powerLockEnable);

/**
 * This function returns the initial region set on startup
 * For REGION_US_LR the region configured can change runtime
 */
uint32_t ZW_mainInitialRegionGet(void);

/**
* Notify the Z-Wave protocol that a RF PHY change is needed.
*/
void ProtocolChangeRfPHYNotify(void);

/**
 * Register application functions to be called just before power down
 *
 * The provided function will be called as the last steps before the chip is
 * forced into deep sleep hibernate.
 *
 * NB: When the function is called the OS tick has been disabled and the FreeRTOS
 *     scheduler is no longer running. OS features like events, queues and timers
 *     are therefore unavailable.
 *
 * @param callback Function to call on power down. Set to NULL to unregister any
 *                 previously registered function.
 */
bool ZW_SetAppPowerDownCallback(void (*callback)(void));


/**
 * Startup the system, This function is the first call from the startup code
 */
int ZW_system_startup(zpal_reset_reason_t eResetReason);

/**
 * Function called as the last step just before the chip enters deep sleep hibernate.
 *
 * NB: When this function is called the OS tick has been disabled and the RTOS
 *     scheduler is no longer running. OS features like events, queues and timers
 *     are therefore unavailable and must not be called from the callback function.
 *
 *     The callback functions can be used to set pins and write to retention RAM.
 *     Do NOT try to write to the NVM file system.
 * 
 * @remarks This is a weakly defined function that can be overwritten from the 
 * application
 */
void ZW_AppPowerDownCallBack(void);

#endif  /* _ZW_MAIN_H_ */
