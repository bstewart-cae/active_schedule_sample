/**
 * SPDX-License-Identifier: LicenseRef-TridentMSLA
 * SPDX-FileCopyrightText: 2024 Trident IoT, LLC <https://www.tridentiot.com>
 */
#include "tr_ring_buffer.h"

static bool ring_buffer_is_full(tr_ring_buffer_t *p_rb) 
{
  return (p_rb->count == p_rb->buffer_size);
}

static bool ring_buffer_is_empty(tr_ring_buffer_t *p_rb) 
{
  return (0 == p_rb->count);
}

bool tr_ring_buffer_init(tr_ring_buffer_t *p_rb) 
{
  if (NULL == p_rb)
  {
    return false;
  }

  if ((NULL == p_rb->p_buffer) || (0xFF < p_rb->buffer_size ))
  {
    return 0;
  }

  p_rb->head = 0;
  p_rb->tail = 0;
  p_rb->count = 0;
  
  return true;
}

bool tr_ring_buffer_write(tr_ring_buffer_t *p_rb, uint8_t data) 
{
  if (ring_buffer_is_full(p_rb)) {
    // Handle overflow here.
    return false;
  }
  p_rb->p_buffer[p_rb->head] = data;
  p_rb->head = (p_rb->head + 1) % p_rb->buffer_size;
  p_rb->count++;
  return true;
}

size_t tr_ring_buffer_read(tr_ring_buffer_t *p_rb, uint8_t *p_data, size_t length) 
{
  size_t count = 0;
  for (; count < length; count++)
  {
    if (ring_buffer_is_empty(p_rb)) {
     // Handle underflow here.
     break;
    }    
    p_data[count] = p_rb->p_buffer[p_rb->tail];
    p_rb->tail = (p_rb->tail + 1) % p_rb->buffer_size;
    p_rb->count--;
  }
  return count;
}

size_t tr_ring_buffer_get_available(tr_ring_buffer_t *p_rb)
{
  return p_rb->count;
}
