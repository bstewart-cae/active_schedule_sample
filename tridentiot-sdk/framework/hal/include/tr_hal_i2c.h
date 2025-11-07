/// ****************************************************************************
/// @file tr_hal_i2c.h
///
/// @brief This is the common include file for the Trident HAL I2C Driver
///
///
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************

#ifndef TR_HAL_I2C_H_
#define TR_HAL_I2C_H_

#include "tr_hal_platform.h"


// ****************************************************************************
// I2C Driver API functions
// ****************************************************************************


// ****************************************************************************
// API for I2C Controller (what used to be called I2C Master)
// ****************************************************************************

/// this initializes one of the 2 I2C Controllers.
/// this takes the i2c_id and a tr_hal_i2c_settings_t config struct.
/// the config struct is used to set up all aspects of the I2C.
/// if a configuration needs to be changed, uninit the I2C, change
/// the settings in the tr_hal_i2c_settings_t and then re-init the I2C.
tr_hal_status_t tr_hal_i2c_init(tr_hal_i2c_id_t        i2c_id,
                                tr_hal_i2c_settings_t* i2c_settings);

/// this un-initializes one of the I2C Controllers.
/// This clears all interrupts, empties FIFOs, sets pins back to GPIO mode.
tr_hal_status_t tr_hal_i2c_uninit(tr_hal_i2c_id_t i2c_id);


/// this loads the current I2C settings into the i2c_settings passed in
tr_hal_status_t tr_hal_i2c_read_settings(tr_hal_i2c_id_t        i2c_id,
                                         tr_hal_i2c_settings_t* i2c_settings);

/// this allows the Controller to send bytes and receive bytes.
/// If the Controller wanted to send 1 byte and expect to receive 3 bytes
/// it would set num_bytes_to_send=1 and num_bytes_to_read=3.
/// bytes received come in via the rx_handler_function set in the
/// tr_hal_i2c_settings_t. If the Controller wanted to just transmit
/// bytes then set num_bytes_to_read=0, If the Controller wanted
/// to just receive bytes then set num_bytes_to_send=0
tr_hal_status_t tr_hal_i2c_tx_rx(tr_hal_i2c_id_t i2c_id,
                                 uint8_t         target_address,
                                 uint8_t*        bytes_to_send,
                                 uint16_t        num_bytes_to_send,
                                 uint16_t        num_bytes_to_read);

/// transmit only
#define tr_hal_i2c_transmit(i2c_id, target_address, bytes_to_send, num_bytes_to_send) \
        tr_hal_i2c_tx_rx(i2c_id, target_address, bytes_to_send, num_bytes_to_send, 0)

/// receive only
#define tr_hal_i2c_receive(i2c_id, target_address, num_bytes_to_read) \
        tr_hal_i2c_tx_rx(i2c_id, target_address, NULL, 0, num_bytes_to_read)

tr_hal_status_t tr_hal_i2c_read_interrupt_counters(tr_hal_i2c_id_t         i2c_id,
                                                   tr_hal_i2c_int_count_t* int_count);

#endif //TR_HAL_SPI_H_
