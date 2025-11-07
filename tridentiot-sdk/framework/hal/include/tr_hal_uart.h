/// ****************************************************************************
/// @file tr_hal_uart.h
///
/// @brief This is the common include file for the Trident HAL UART Driver
///
///
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************
/// how to RX and TX in different modes with the UART:
/// ***************************************************************************
/// --------- transmit ---------
/// 1. transmit using the raw TX APIs
///                        => set tr_hal_uart_settings_t.tx_dma_enabled=0
///                        => use API: tr_hal_uart_raw_tx_buffer
///
/// 2. transmit using DMA TX APIs
///                        => set tr_hal_uart_settings_t.tx_dma_enabled=1
///                        => use API: tr_hal_uart_dma_tx_bytes_in_buffer
///
/// --------- receive ---------
/// 1. receive automatically in the user defined receive function (recommended)
///                => set tr_hal_uart_settings_t.rx_handler_function to an app function
///                => set tr_hal_uart_settings_t.rx_dma_enabled=0
///                => handle bytes in the app function
///
/// 2. receive using DMA RX, where the app manages the DMA RX buffer
///
/// 3. receive using the raw API calls
///                => set tr_hal_uart_settings_t.rx_dma_enabled=0
///                => call tr_hal_uart_raw_rx_available_bytes
/// ***************************************************************************

#ifndef TR_HAL_UART_H_
#define TR_HAL_UART_H_

#include "tr_hal_platform.h"


// ****************************************************************************
// UART Driver API functions
// ****************************************************************************


/// ****************************************************************************
/// @defgroup tr_hal_uart UART
/// @ingroup tr_hal_api
/// @{
/// ****************************************************************************


// ***************************************************************************
// ---- UART init/uninit APIs ----
// ***************************************************************************

/// this initializes one of the 3 UARTs.
/// this takes the uart_id and a tr_hal_uart_settings_t config struct.
/// the config struct is used to set up all aspects of the UART.
/// if a configuration needs to be changed, uninit the UART, change
/// the settings in the tr_hal_uart_settings_t and then re-init the UART.
tr_hal_status_t tr_hal_uart_init(tr_hal_uart_id_t        uart_id,
                                 tr_hal_uart_settings_t* uart_settings);

/// this un-initializes one of the 3 UARTs.
/// clears all interrupts, empties FIFOs, sets pins back to GPIO mode.
tr_hal_status_t tr_hal_uart_uninit(tr_hal_uart_id_t uart_id);

/// if set to a LITE_SLEEP mode, this will change the clock and set the
/// baud rate to the values in the UART settings for sleep. If set to
/// WAKE mode then this will set the clock and baud rate to the values
/// in the UART settings for normal mode.
tr_hal_status_t tr_hal_uart_set_power_mode(tr_hal_uart_id_t    uart_id,
                                           tr_hal_power_mode_t power_mode);


// ***************************************************************************
// ---- UART receive APIs ----
// ***************************************************************************

/// This function attempts to read one byte off of the chip registers.
/// The app will not need this function if the receive_fx is enabled and used.
tr_hal_status_t tr_hal_uart_raw_rx_one_byte(tr_hal_uart_id_t uart_id,
                                            uint8_t*            byte);

/// This function attempts to read bytes off of the chip registers.
/// The app will not need this function if the receive_fx is enabled and used.
tr_hal_status_t tr_hal_uart_raw_rx_available_bytes(tr_hal_uart_id_t uart_id,
                                                   uint8_t*            bytes,
                                                   uint16_t         buffer_size,
                                                   uint16_t*        num_returned_bytes);

// ***************************************************************************
// ---- UART transmit APIs ----
// ***************************************************************************

/// This function transmits the byte in the variable byte_to_send using
/// raw (non DMA) for transmit.
tr_hal_status_t tr_hal_uart_raw_tx_one_byte(tr_hal_uart_id_t uart_id,
                                            const uint8_t       byte_to_send);

/// This function transmits the bytes in the bytes_to_send buffer using
/// raw (non DMA) for transmit.
tr_hal_status_t tr_hal_uart_raw_tx_buffer(tr_hal_uart_id_t uart_id,
                                          const uint8_t*      bytes_to_send,
                                          uint16_t         num_bytes_to_send);

// returns true if a transmit is in progress
// returns false if all transmits have been completed (no tx in progress)
tr_hal_status_t tr_hal_uart_tx_active(tr_hal_uart_id_t uart_id,
                                      bool*            tx_active);

// ***************************************************************************
// ---- UART DMA transmit ----
// ***************************************************************************

/// This function transmits the bytes in the bytes_to_send buffer using
/// DMA for transmit.
tr_hal_status_t tr_hal_uart_dma_tx_bytes_in_buffer(tr_hal_uart_id_t uart_id,
                                                   const uint8_t*   bytes_to_send,
                                                   uint16_t         num_bytes_to_send);

// ***************************************************************************
// ---- UART DMA receive ----
// to receive bytes with UART, the app should check the receive buffer passed in
// ***************************************************************************

/// This function checks the number of bytes available in the DMA RX buffer.
/// There is an event that notifies the user of a low condition: UART_EVENT_DMA_RX_BUFFER_LOW
tr_hal_status_t tr_hal_uart_dma_receive_buffer_num_bytes_left(tr_hal_uart_id_t uart_id,
                                                              uint32_t*        bytes_left);

/// This functions allows the caller to change the DMA buffer used for receive.
/// This should be done when the buffer is getting full (low on bytes remaining)
tr_hal_status_t tr_hal_uart_dma_change_rx_buffer(tr_hal_uart_id_t uart_id,
                                                 uint8_t*         new_receive_buffer,
                                                 uint16_t         new_buffer_length);


// ***************************************************************************
// ---- UART functions to convert int value to enum ----
// these can be used by the app to convert user input (like from CLI) to enum values
// ***************************************************************************

/// takes a baud rate as an int (like 115200) and returns an enum of
/// like TR_HAL_UART_BAUD_RATE_xxx
tr_hal_baud_rate_t tr_hal_get_baud_rate_enum_from_value(uint32_t baud_rate_value);

/// takes a data bits value as an int (like 8) and returns an enum
/// like LCR_DATA_BITS_xxx_VALUE
tr_hal_data_bits_t tr_hal_get_data_bits_enum_from_value(uint8_t data_bits_value);

/// takes a stop bits value as an int (like 1) and returns an enum like
/// LCR_STOP_BITS_xxx_VALUE
tr_hal_stop_bits_t tr_hal_get_stop_bits_enum_from_value(uint8_t stop_bits_value);


// ***************************************************************************
// ---- UART power on/off - these are called from init and uninit - the app should not need to call these
// ***************************************************************************

/// called from init. the app should not need to call this.
/// disables and clears interrupts
tr_hal_status_t tr_hal_uart_power_off(tr_hal_uart_id_t uart_id);

/// called from uninit. the app should not need to call this.
/// enables and interrupts
tr_hal_status_t tr_hal_uart_power_on(tr_hal_uart_id_t uart_id);


// ***************************************************************************
// ---- setting pin mode and checking valid pin settings ----
// ***************************************************************************

/// function for setting the TX and RX pins for a particular UART.
/// this also checks that the pin choices are VALID for that particular UART.
tr_hal_status_t tr_hal_uart_set_tx_rx_pins(tr_hal_uart_id_t  uart_id,
                                           tr_hal_gpio_pin_t tx_pin,
                                           tr_hal_gpio_pin_t rx_pin);

/// function for setting the RTS and CTS pins for a particular UART.
/// this also checks that the pin choices are VALID for that particular UART.
tr_hal_status_t tr_hal_uart_set_rts_cts_pins(tr_hal_uart_id_t  uart_id,
                                             tr_hal_gpio_pin_t rts_pin,
                                             tr_hal_gpio_pin_t cts_pin);

/// checking if pins are valid on any of the 3 UARTs, passing
/// in the uart_id
bool tr_hal_uart_check_pins_valid(tr_hal_uart_id_t  uart_id,
                                  tr_hal_gpio_pin_t new_tx_pin,
                                  tr_hal_gpio_pin_t new_rx_pin);


/// ****************************************************************************
/// @} // end of tr_hal_api
/// ****************************************************************************


#endif //TR_HAL_UART_H_
