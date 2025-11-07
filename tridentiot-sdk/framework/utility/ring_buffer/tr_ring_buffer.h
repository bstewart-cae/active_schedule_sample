/**
 * @file
 *
 * A simple ring buffer for handling transfer of bytes.
 *
 * The ring buffer supports a buffer of up to 256 bytes.
 *
 * SPDX-License-Identifier: LicenseRef-TridentMSLA
 * SPDX-FileCopyrightText: 2024 Trident IoT, LLC <https://www.tridentiot.com>
 */
#ifndef TR_RING_BUFFER_H
#define TR_RING_BUFFER_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

/**
 * @addtogroup tr-utility
 * @{
 * @addtogroup tr-utility-ring-buffer Ring Buffer
 * @brief
 * A simple ring buffer for handling transfer of bytes.
 *
 * @{
 */

/**
 * @brief Ring buffer object definition.
 *
 */
typedef struct
{
  uint8_t * p_buffer; ///< Address of the array allocated to the ring buffer.
  size_t head;        ///< Head of the ring buffer.
  size_t tail;        ///< Tail of the ring buffer.
  size_t buffer_size; ///< Size of the array pointed to by p_buffer.
  size_t count;       ///< Current number of items in the ring buffer.
}
tr_ring_buffer_t;

/**
 * Initializes the ring buffer.
 *
 * @param[in] p_rb Ring buffer object where buffer and buffer size is set.
 *
 * @return Returns `true` if the ring buffer was successfully initialized, and `false` otherwise.
 */
bool tr_ring_buffer_init(tr_ring_buffer_t *p_rb);

/**
 * Writes a byte to the ring buffer.
 *
 * @param[in] p_rb Address of a ring buffer object that has been initialized by tr_ring_buffer_init().
 * @param[in] data A byte of data to write to the ringer buffer.
 *
 * @return Returns `true` if the byte was written to the ring buffer, and `false` otherwise.
 */
bool tr_ring_buffer_write(tr_ring_buffer_t *p_rb, uint8_t data);

/**
 * Reads a given number of bytes from the ring buffer.
 *
 * @param[in]  p_rb   Address of a ring buffer object that has been initialized by tr_ring_buffer_init().
 * @param[out] p_data Address of buffer where read data must be written to.
 * @param[in]  length Number of bytes to read.
 *
 * @return Returns the number of bytes that was read from the given ring buffer.
 */
size_t tr_ring_buffer_read(tr_ring_buffer_t *p_rb, uint8_t *p_data, size_t length);

/**
 * Returns the number of occupied bytes in the ring buffer.
 *
 * @param[in] p_rb Address of a ring buffer object that has been initialized by tr_ring_buffer_init().
 *
 * @return Returns the number of occupied bytes in the ring buffer.
 */
size_t tr_ring_buffer_get_available(tr_ring_buffer_t *p_rb);

/**
 * @} //tr-utility-ring-buffer
 * @} //tr-utility
 */

#endif // TR_RING_BUFFER_H
