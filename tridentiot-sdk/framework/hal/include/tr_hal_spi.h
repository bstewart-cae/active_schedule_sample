/// ****************************************************************************
/// @file tr_hal_spi.h
///
/// @brief This is the common include file for the Trident HAL SPI Driver
///
///
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************

#ifndef TR_HAL_SPI_H_
#define TR_HAL_SPI_H_

#include "tr_hal_platform.h"


/// ****************************************************************************
/// \brief SPI Driver API functions
/// ****************************************************************************


/// ****************************************************************************
/// @defgroup tr_hal_spi SPI
/// @ingroup tr_hal_api
/// @{
/// ****************************************************************************


/// ****************************************************************************
/// ---- init settings / uninit settings / read settings ----
/// ****************************************************************************

// error check and then load the settings into the registers
tr_hal_status_t tr_hal_spi_init(tr_hal_spi_id_t        spi_id,
                                tr_hal_spi_settings_t* spi_settings);

// sets pins back to general GPIO and turns off interrupts, etc
tr_hal_status_t tr_hal_spi_uninit(tr_hal_spi_id_t spi_id);


// this loads the current SPI settings into the spi_settings passed in
tr_hal_status_t tr_hal_spi_settings_read(tr_hal_spi_id_t        spi_id,
                                         tr_hal_spi_settings_t* spi_settings);


/// ****************************************************************************
/// ---- SPI receive APIs ----
/// ****************************************************************************
tr_hal_status_t tr_hal_spi_raw_rx_one_byte(tr_hal_spi_id_t spi_id,
                                           uint8_t*        byte);

tr_hal_status_t tr_hal_spi_raw_rx_available_bytes(tr_hal_spi_id_t spi_id,
                                                  char*           bytes,
                                                  uint16_t        buffer_size,
                                                  uint16_t*       num_returned_bytes);


/// ****************************************************************************
/// ---- SPI transmit APIs ----
/// ****************************************************************************
tr_hal_status_t tr_hal_spi_raw_tx_one_byte(tr_hal_spi_id_t spi_id,
                                           uint8_t         chip_select_index_to_use,
                                           char            byte_to_send,
                                           bool            receive_bytes);

tr_hal_status_t tr_hal_spi_raw_tx_buffer(tr_hal_spi_id_t spi_id,
                                         uint8_t         chip_select_index_to_use,
                                         char*           bytes_to_send,
                                         uint16_t        num_bytes_to_send,
                                         bool            receive_bytes);


/// ****************************************************************************
/// ---- SPI DMA transmit ----
/// ****************************************************************************
tr_hal_status_t tr_hal_spi_dma_tx_bytes_in_buffer(tr_hal_spi_id_t spi_id,
                                                  uint8_t         chip_select_index_to_use,
                                                  char*           bytes_to_send,
                                                  uint16_t        num_bytes_to_send,
                                                  bool            receive_bytes);

/// ****************************************************************************
/// ---- SPI DMA receive ----
/// to receive bytes with SPI, the app should check the receive buffer passed in
/// ****************************************************************************

// checks the number of bytes available in the DMA RX buffer
// there is also an event: SPI_EVENT_DMA_RX_BUFFER_LOW
tr_hal_status_t tr_hal_spi_dma_receive_buffer_num_bytes_left(tr_hal_spi_id_t spi_id,
                                                             uint32_t*       bytes_left);

// change the buffer if it is getting low
tr_hal_status_t tr_hal_spi_dma_change_rx_buffer(tr_hal_spi_id_t spi_id,
                                                uint8_t*        new_receive_buffer,
                                                uint16_t        new_buffer_length);


/// ****************************************************************************
/// @} // end of tr_hal_api
/// ****************************************************************************


#endif //TR_HAL_SPI_H_
