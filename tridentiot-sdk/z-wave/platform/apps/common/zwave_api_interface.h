/// ***************************************************************************
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************

/**
 * @file zwave_api_interface.h
 * @brief Platform abstraction for Z-Wave Applications
 */
#ifndef ZWAVE_API_INTERFACE_H_
#define ZWAVE_API_INTERFACE_H_

#include <stdint.h>
#include <ZAF_Common_interface.h>

/**
 * This function register a callback function into the protocol.
 * The callback function is called before entering the deep sleep power mode.
 * The fucntion retruns true if the callback registed, else returns false.
 *
 * @param[in] callback Pointer tot he callback function to register
 *
 * @return True if the callback is registered else false.
 */
bool set_powerdown_callback(zaf_wake_up_callback_t callback);


#endif /* ZWAVE_API_INTERFACE_H_ */
