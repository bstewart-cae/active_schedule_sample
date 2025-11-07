// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file ZW_network_management.h
 * @copyright 2022 Silicon Laboratories Inc.
 *
 * @brief This file contains protocol specific definitions used by network management
 * module.
 */
#ifndef _NETWORK_MANAGEMENT_H_
#define _NETWORK_MANAGEMENT_H_
/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/

#include "SwTimer.h"

/****************************************************************************/
/*                     EXPORTED TYPES and DEFINITIONS                       */
/****************************************************************************/
/* ZW_SetlearnMode set Smart Start mode definition */
#define ZW_SET_LEARN_MODE_SMARTSTART          0x04


#ifdef ZW_CONTROLLER
extern void ZCB_NetworkManagementSetNWI(SSwTimer* pTimer);

extern void NetworkManagementStartSetNWI(void);

extern void NetworkManagementStopSetNWI(void);

#endif

/****************************************************************************/
/*                              EXPORTED DATA                               */
/****************************************************************************/

/****************************************************************************/
/*                           EXPORTED FUNCTIONS                             */
/****************************************************************************/

/**
 *  Return true if Smart Start functionality enabled and Smart Start INCLUDE bit is set
 *  this is used in Smart Start mode to determine if Smart Start PRIME or INCLUDE
 *  ExploreRequestinclusion Explore frame should be transmitted - if INCLUDE then the node will
 *  wait for the matching AssignID protocol command for 2 seconds before starting next wait period
 *
 *  @return    true if Smart Start functionality enabled and Smart Start INCLUDE bit is set
 */
uint8_t NetworkManagementSmartStartIsInclude(void);

bool NetworkManagementSmartStartIsLR(void);

/**
 * Reads DSK part of public key from NVM and copies it to global g_Dsk array
 * Manipulates bits in g_Dsk
 *
 * @return None
 */
void NetworkManagementGenerateDskpart(void);


/**
 *
 * Sets the maximum time interval between SmartStart Inclusion requests
 *
 * @return 0    Requested maximum intervals either 0(default) or not valid
 * @return 5-99 The requested number of intervals set
 * @param[in] bInclRequestIntervals The maximum number of 128sec ticks inbetween SmartStart inclusion requests.
              Valid range 5-99, which corresponds to 640-12672sec.
 *
 */
uint32_t
ZW_NetworkManagementSetMaxInclusionRequestIntervals(
  uint32_t bInclRequestIntervals);

/**
 * Get whether an inclusion request process has been started and is ongoing for SmartStart.
 */
bool NetworkManagement_IsSmartStartInclusionOngoing(void);
void NetworkManagement_IsSmartStartInclusionSet(void);
void NetworkManagement_IsSmartStartInclusionClear(void);

/* Delete all controller network info storage files by ID */
void DeleteCtrlNetworkInfoStorage(void);

#endif /* _NETWORK_MANAGEMENT_H_ */
