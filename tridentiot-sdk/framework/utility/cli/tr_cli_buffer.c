/// ****************************************************************************
/// @file tr_cli_buffer.c
///
/// @brief code for buffering bytes to be sent to the CLI utility
///
///
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************
#include "tr_cli.h"
#include "tr_ring_buffer.h"
// for critical section functions
#include "sysfun.h"

/// ***************************************************************************
/// VARIABLES
/// ***************************************************************************
// buffer for the uart bytes coming in
static uint8_t buffer[128];

// ring buffer structure
static tr_ring_buffer_t cli_ring_buffer;

// high watermark to make sure the buffer is sized properly
uint8_t g_buffer_high_watermark;

// number of errors due to drops
uint16_t g_byte_drops;

/// ****************************************************************************
/// tr_cli_buffer_init
///
/// @brief this clears out the buffer by setting the number of bytes to 0
/// the buffer contents are not actually cleared - not necessary
/// ****************************************************************************
void tr_cli_buffer_init(void)
{
    cli_ring_buffer.p_buffer    = buffer;
    cli_ring_buffer.buffer_size = sizeof(buffer);
    tr_ring_buffer_init(&cli_ring_buffer);
}

/// ****************************************************************************
/// tr_cli_buffer_clear_stats
///
/// @brief this clears out the driver stats
/// ****************************************************************************
void tr_cli_buffer_clear_stats(void)
{
    g_buffer_high_watermark = 0;
    g_byte_drops            = 0;
}

/// ****************************************************************************
/// tr_cli_buffer_byte
///
/// @brief adds byte to the buffer waiting for the CLI
/// ****************************************************************************
void tr_cli_buffer_byte(uint8_t new_input_char)
{
    enter_critical_section();

    if (tr_ring_buffer_write(&cli_ring_buffer, new_input_char))
    {
        // check high water mark
        if (cli_ring_buffer.count > g_buffer_high_watermark)
        {
            g_buffer_high_watermark = (uint8_t)cli_ring_buffer.count;
        }
    }
    else
    {
        // byte was dropped
        g_byte_drops++;
    }
    leave_critical_section();
}

/// ****************************************************************************
/// tr_cli_buffer_process_bytes
///
/// @brief this pulls bytes out of the buffer and hands them to the CLI
/// ****************************************************************************
void tr_cli_buffer_process_bytes(void)
{
    uint8_t data;
    uint8_t num_bytes;

    do
    {
        // enter critical section while dealing with the ring buffer
        enter_critical_section();
        num_bytes = tr_ring_buffer_read(&cli_ring_buffer, &data, 1);
        leave_critical_section();

        if (num_bytes != 0)
        {
            tr_cli_char_received(data);
        }
    }
    while (num_bytes != 0);
}

/// ****************************************************************************
/// tr_cli_buffer_get_stats
///
/// @brief this returns the driver stats
/// ****************************************************************************
tr_cli_buffer_stats_t tr_cli_buffer_get_stats(void)
{
    tr_cli_buffer_stats_t stats;

    stats.buffer_high_watermark = g_buffer_high_watermark;
    stats.byte_drops            = g_byte_drops;
    stats.buffer_size           = sizeof(buffer);

    return stats;
}
