// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file ZW_inclusion_controller.h
 * @brief Implements inclusion controller functionality API
 * @copyright 2021 Silicon Laboratories Inc.
 */

#ifndef ZWAVE_ZW_INCLUSION_CONTROLLER_H_
#define ZWAVE_ZW_INCLUSION_CONTROLLER_H_

/*
 * Enquire if the controller is an inclusion controller
 * @return true - Controller is an inclusion controller.
 *         false - Controller is not an inclusion controller
 */
bool isNodeIDServerPresent();

/*
* Set inclusion controller capability
* @param[in] true - Controller is an inclusion controller
*            false - Controller is not an inclusion controller
*/
void SetNodeIDServerPresent(bool bPresent);

#endif /* ZWAVE_ZW_INCLUSION_CONTROLLER_H_ */
