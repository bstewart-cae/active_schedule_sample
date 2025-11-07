/// ****************************************************************************
/// @file tr_hal_platform.h
///
/// @brief This file contains the CHIP SPECIFIC types and defines for the T32CM11
///
///
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************

#ifndef TR_HAL_PLATFORM_H_
#define TR_HAL_PLATFORM_H_

#include "tr_hal_common.h"
#include "cm3_mcu.h"



/// ****************************************************************************
/// \brief pin type
/// ****************************************************************************
typedef struct
{
    uint32_t pin;

} tr_hal_gpio_pin_t;


/// ****************************************************************************
/// \brief values and a range checking function for setting the interrupt
///        priority in the Trident HAL APIs
/// ****************************************************************************
typedef enum
{
    // DO NOT use pri 0, this will cause problems
    // this is what the INT system uses
    //TR_HAL_INTERRUPT_PRIORITY_0   = 0,

    TR_HAL_INTERRUPT_PRIORITY_HIGHEST = 1,
    TR_HAL_INTERRUPT_PRIORITY_1       = 1,
    TR_HAL_INTERRUPT_PRIORITY_2       = 2,
    TR_HAL_INTERRUPT_PRIORITY_3       = 3,
    TR_HAL_INTERRUPT_PRIORITY_4       = 4,
    TR_HAL_INTERRUPT_PRIORITY_5       = 5,
    TR_HAL_INTERRUPT_PRIORITY_6       = 6,
    TR_HAL_INTERRUPT_PRIORITY_7       = 7,
    TR_HAL_INTERRUPT_PRIORITY_LOWEST  = 7,
} tr_hal_int_pri_t;

/// check interrupt priority is in the right range
tr_hal_status_t tr_hal_check_interrupt_priority(tr_hal_int_pri_t interrupt_priority);


/// ****************************************************************************
/// include Trident HAL platform specific headers
///
/// this is done so the platform-inspecific headers don't include platform specific headers
///
/// NOTE these need to be included AFTER the definition of tr_hal_gpio_pin_t
///
/// TODO: we will want a way to control which of these drivers get included in a
///       project so we dont force every project to have every driver. Maybe we
///       can add ifdefs around the includes and use the generated code to determine
///       which peripheral is included.
/// ****************************************************************************
#include "T32CM11_gpio.h"
#include "T32CM11_power.h"
#include "T32CM11_uart.h"
#include "T32CM11_timers.h"
#include "T32CM11_spi.h"
#include "T32CM11_rtc.h"
#include "T32CM11_wdog.h"
#include "T32CM11_adc.h"
#include "T32CM11_trng.h"
#include "T32CM11_pwm.h"
#include "T32CM11_i2c.h"


#endif //TR_HAL_PLATFORM_H_
