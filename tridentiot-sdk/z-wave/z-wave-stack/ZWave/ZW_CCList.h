// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file ZW_CCList.h
 * @copyright 2022 Silicon Laboratories Inc.
 *
 * @brief CCList - CommandClassList module.
 * The module can determine the currently relevant commandclass list based
 * on inclusion state, security state etc.
 */
#ifndef _ZW_CCList_H_
#define _ZW_CCList_H_

#include "stdbool.h"
#include "stdint.h"
#include "ZW_application_transport_interface.h"

/**
* Set the Network Info in CCList.
*
* Provides module with needed network information of inclusions status
* and obtained security keys.
*
* @remarks Must be called prior to calling any other CCList methods.
*
* @param[in]     pNetworkInfo       Pointer to network information. Information must be kept
*                                   up to date.
*/
void CCListSetNetworkInfo(const SNetworkInfo * pNetworkInfo);

/**
* Returns pointer to currently relevant CommandClass List.
*
* Returns relevant CommandClass List based on reqyest key and current
* inclusion state e.g. included, secure included etc.
*
* @param[in]     eKey               Security key type that requests the CommandClass List.
*                                   items to queue.
* @return                           Pointer to CommandClass List.
*/
const SCommandClassList_t* CCListGet(security_key_t eKey);


/**
 * Returns true if a given command class is in the list
 *
 * @param cclist Command class list
 * @param cmdClass Class to search for
 */
bool CCListHasClass(const SCommandClassList_t* cclist , uint8_t cmdClass);

#endif /* _ZW_CCList_H_ */
