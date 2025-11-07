/// ****************************************************************************
/// @file tr_hal_adc.h
///
/// @brief This is the common include file for the Trident HAL ADC driver
///
///
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************

#ifndef TR_HAL_ADC_H_
#define TR_HAL_ADC_H_

#include "tr_hal_platform.h"

/// ****************************************************************************
/// ---- ADC API functions ---- 
/// ****************************************************************************

// note: when using more than one ADC channel, it is required to use interrupts
// in order to get readings from all initialized ADC channels. This is because 
// a START command starts ALL the ADCs and an interrupt fires for each one as
// they are ready and there is only one RESULT register so it needs to be read
// when the interrupt fires. The READ APIs below return the LAST value read 
// at the last START command. When using a single ADC channel interrupts are
// not required because the RESULT register only has results for the one 
// channel


// ADC register values are cleared with a power cycle but not on a software reset
// this clears the ADC registers. Safe to call this when the HAL comes up before
// any other peripherals are configured
tr_hal_status_t tr_hal_adc_set_for_initial_state_all_disabled(void);

// error check and then load the settings into the registers
tr_hal_status_t tr_hal_adc_init(tr_hal_adc_channel_id_t adc_channel_id,
                                tr_hal_adc_settings_t*  adc_settings);

// sets GPIO back to general GPIO and turns off interrupts, etc
tr_hal_status_t tr_hal_adc_uninit(tr_hal_adc_channel_id_t adc_channel_id);

// starts the ADC reading. This starts ALL ADCs that are initialized to take
// a measurement. The value read can be received either in the interrupt
// handler or by calling the adc_read API
tr_hal_status_t tr_hal_adc_start(void);

// returns the value last read by the ADC for the channel selected
tr_hal_status_t tr_hal_adc_read(tr_hal_adc_channel_id_t adc_channel_id,
                                uint32_t*               result);

// returns the value last read by the ADC for the channel selected in 
// microvolts if this channel is setup to convert
tr_hal_status_t tr_hal_adc_read_voltage(tr_hal_adc_channel_id_t adc_channel_id,
                                        uint32_t*               microVolts);


#endif //TR_HAL_ADC_H_
