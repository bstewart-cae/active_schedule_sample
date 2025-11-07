/// ****************************************************************************
/// @file T32CM11_power.h
///
/// @brief This is the chip specific include file for T32CM11 Power Management
///        note that there is a common include file for this HAL module that
///        contains the APIs (such as the init function) that should be used
///        by the application.
///
///
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************

#ifndef T32CM11_POWER_H_
#define T32CM11_POWER_H_

#include "tr_hal_platform.h"

/// ****************************************************************************
/// @defgroup tr_hal_power_cm11 Power Mgmt CM11
/// @ingroup tr_hal_T32CM11
/// @{
/// ****************************************************************************


/// ****************************************************************************
/// \brief enum for the different power modes that the chip can be in
/// ****************************************************************************
typedef enum
{
    TR_HAL_POWER_MODE_0 = 10, // wake
    TR_HAL_POWER_MODE_1 = 11, // sleep0 = platform sleep + radio ON
    TR_HAL_POWER_MODE_2 = 12, // sleep1 = platform sleep + radio sleep
    TR_HAL_POWER_MODE_3 = 13, // sleep2 = platform sleep + radio DEEP sleep
    TR_HAL_POWER_MODE_4 = 14, // sleep3 = platform DEEP sleep + radio DEEP sleep

} tr_hal_power_mode_t;


/// ****************************************************************************
/// @} // end of tr_hal_T32CM11
/// ****************************************************************************


#endif // T32CM11_POWER_H_

