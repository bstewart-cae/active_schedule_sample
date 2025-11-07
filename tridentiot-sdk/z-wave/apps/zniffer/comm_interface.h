/**
 * @file comm_interface.h
 * @copyright 2022 Silicon Laboratories Inc.
 */
/// ***************************************************************************
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************


#ifndef __COMM_INTERFACE__
#define __COMM_INTERFACE__

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <zpal_uart.h>

/**
 * @addtogroup Apps
 * @{
 * @addtogroup Zniffer
 * @{
 */

/**
 * @brief Receive buffer size
 *
 */
#define RECEIVE_BUFFER_SIZE     32

/**
 * @brief Transmit buffer size
 *
 */
#define TRANSMIT_BUFFER_SIZE    180

/**
 * @brief Parsing result enum
 *
 */
typedef enum
{
  PARSE_IDLE,             ///< returned if nothing special has happened
  PARSE_FRAME_RECEIVED,   ///< returned when a valid frame has been received
  PARSE_FRAME_SENT,       ///< returned if frame was ACKed by the other end
  PARSE_FRAME_ERROR,      ///< returned if frame has error in Checksum
  PARSE_RX_TIMEOUT,       ///< returned if Rx timeout has happened
  PARSE_TX_TIMEOUT        ///< returned if Tx timeout (waiting for ACK) ahs happened
} comm_interface_parse_result_t;

/**
 * @brief Transport handle type
 *
 */
typedef void * transport_handle_t;

/**
 * @brief Transport
 *
 */
typedef struct _transport_t
{
  transport_handle_t handle; ///< Transport handle
} transport_t;

/**
 * @brief Transmit done callback type
 *
 */
typedef void (*transmit_done_cb_t)(transport_handle_t transport);

/**
 * Structure for zniffer commands on the UART
 */
typedef struct __attribute__((packed))
{
  uint8_t sof;                            ///< Start of frame
  uint8_t cmd;                            ///< Command
  uint8_t len;                            ///< Length
  uint8_t payload[TRANSMIT_BUFFER_SIZE];  ///< Payload
} comm_interface_command_t ;

/**
 * Structure for zniffer Rx frames on the UART
 */
typedef struct __attribute__((packed))
{
  uint8_t   sof_frame;                      ///< Start of frame
  uint8_t   type;                           ///< Type of frame
  uint16_t  timestamp;                      ///< Timestamp
  uint8_t   channel_and_speed;              ///< Channel and speed
  uint8_t   region;                         ///< Region
  uint8_t   rssi;                           ///< RSSI value
  uint16_t  start_of_data;                  ///< Start of data
  uint8_t   len;                            ///< Length
  uint8_t   payload[TRANSMIT_BUFFER_SIZE];  ///< Payload
} comm_interface_frame_t ;

/**
 * @brief Beam start frame type
 */
typedef struct __attribute__((packed))
{
  uint8_t sof;                ///< Start of frame
  uint8_t type;               ///< Type of frame
  uint16_t timestamp;         ///< Timestamp
  uint8_t channel_and_speed;  ///< Channel and Speed
  uint8_t region;             ///< Region
  uint8_t rssi;               ///< RSSI value
  uint8_t payload[4];         ///< Payload
} comm_interface_beam_start_frame_t;

/**
 * @brief Beam stop frame type
 */
typedef struct __attribute__((packed))
{
  uint8_t sof;                ///< Start of frame
  uint8_t type;               ///< Type of frame
  uint16_t timestamp;         ///< Timestamp
  uint8_t rssi;               ///< RSSI value
  uint16_t counter;           ///< Counter value
} comm_interface_beam_stop_frame_t;

/**
 * @brief Transmit a command to the host
 *
 * @param cmd
 * @param payload
 * @param len
 * @param cb
 */
void comm_interface_transmit_command(uint8_t cmd, const uint8_t *payload, uint8_t len, transmit_done_cb_t cb);

/**
 * @brief Transmit a frame to the host
 *
 * @param timestamp
 * @param ch_and_speed
 * @param region
 * @param rssi
 * @param payload
 * @param length
 * @param cb
 */
void comm_interface_transmit_frame(uint16_t timestamp, uint8_t ch_and_speed, uint8_t region, int8_t rssi, const uint8_t *payload, uint8_t length, transmit_done_cb_t cb);

/**
 * @brief Transmit a beam start frame to the host
 *
 * @param timestamp
 * @param ch_and_speed
 * @param region
 * @param rssi
 * @param payload
 * @param length
 * @param cb
 */
void comm_interface_transmit_beam_start(uint16_t timestamp, uint8_t ch_and_speed, uint8_t region, int8_t rssi, const uint8_t *payload, uint8_t length, transmit_done_cb_t cb);

/**
 * @brief Transmit a beam stop frame to the host
 *
 * @param timestamp
 * @param rssi
 * @param counter
 * @param cb
 */
void comm_interface_transmit_beam_stop(uint16_t timestamp, int8_t rssi, uint16_t counter, transmit_done_cb_t cb);

/**
 * Wait for a transmission to finish (blocking)
 */
void comm_interface_wait_transmit_done(void);

/**
 * @brief Initialize the communication interface
 *
 * @param uart_config             UART configuration
 * @param uart_rx_event_handler   UART event handler function
*/
void comm_interface_init(zpal_uart_config_t *uart_config, void (*uart_rx_event_handler)());

/**
 * @brief Parse the incomming data
 *
 * @return comm_interface_parse_result_t
 */
comm_interface_parse_result_t comm_interface_parse_data(void);

/**
 * @brief Get the received command
 *
 * @return comm_interface_command_t
 */
comm_interface_command_t* comm_interface_get_command(void);

/**
 * @brief Get the transmision status
 *
 * @return True if transmision is done, else false
 */

bool comm_interface_was_transmit_done(void);

/**
 * @brief Get the number of bytes in the receive buffer
 *
 * @return Number of received bytes
 */

size_t comm_interface_data_available(void);
/**
 * @}
 * @}
 */

#endif /* __COMM_INTERFACE__ */
