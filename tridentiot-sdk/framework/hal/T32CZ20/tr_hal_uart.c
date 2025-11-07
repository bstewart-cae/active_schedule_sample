/// ****************************************************************************
/// @file tr_hal_uart.c
///
/// @brief This contains the code for the Trident HAL UART for T32CZ20
///
///
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************

#include <string.h>
#include "tr_hal_uart.h"
#include "tr_hal_power.h"

// for debug when we need to add printing
//#include "util_printf.h"

// variables to keep track of current UART running state
// we need to remember if a UART has been configured and what the user function is
bool g_uart_init_completed[TR_NUMBER_OF_UARTS] = {false, false, false};
bool g_uart_powered[TR_NUMBER_OF_UARTS] = {true, true, true};

// internal data structs to keep track of the UART settings
// these are only valid if g_uart_init_completed=true for this UART
tr_hal_uart_settings_t g_current_uart_settings[TR_NUMBER_OF_UARTS];

// for DMA receive mode:
// we keep track of what bytes we have read from the DMA RX buffer
// by keeping an index to the last byte we read
uint16_t g_curr_dma_rx_index[TR_NUMBER_OF_UARTS] = {0,0,0};

// INTERNAL function: when we have received bytes in the DMA
// this reads the NEW bytes in the buffer and calls the user
// function
static void hal_internal_handle_rx_dma_bytes(tr_hal_uart_id_t uart_id);


// *****************************************************************
// baud rates - see section 18.3.3
//
// lookup table for divisor and fractional divisor to set the baud rate
//
// the baud rate is determines by taking the 32 MHz clock and
// dividing by 8 and also by a configured number. The configured
// number is a whole number divisor and a fractional divisor
// the fractional divisor is a number from 1-8 divided by 8
//
// FOR INSTANCE: for a target of 115200 baud rate
// 32,000,000 / 8 = 4,000,000 / 115200 = 34.722
// divisor would be 34
// fractional divisor would be 6/8th which is closest 8th to 0.722
// so set divisor_latch_register = 34
// and set fractional_divisor_latch = FRAC_DIVISOR_6
//
// we have an enum so the user can specify what baud rate is desired
// that enum is used to lookup the divisor and fractional divisor
// to set the 2 registers
// *****************************************************************
static uint32_t baud_rate_lookup_table[TR_HAL_NUM_BAUD_RATES][2] =
{
    // FOR: tr_hal_clock_t clock_mode = TR_HAL_CLOCK_32M
    // for a target of 115200 baud rate
    // 32,000,000 clock / 8 = 4,000,000 / 115200 = 34.722
    // divisor = 34, fractional divisor = 6 (6/8th = 0.750)

    { 1666, 5 }, //  0 = TR_HAL_UART_BAUD_RATE_2400
    {  833, 3 }, //  1 = TR_HAL_UART_BAUD_RATE_4800
    {  416, 5 }, //  2 = TR_HAL_UART_BAUD_RATE_9600
    {  277, 6 }, //  3 = TR_HAL_UART_BAUD_RATE_14400
    {  208, 3 }, //  4 = TR_HAL_UART_BAUD_RATE_19200
    {  138, 7 }, //  5 = TR_HAL_UART_BAUD_RATE_28800
    {  104, 1 }, //  6 = TR_HAL_UART_BAUD_RATE_38400
    {   69, 4 }, //  7 = TR_HAL_UART_BAUD_RATE_57600
    {   52, 1 }, //  8 = TR_HAL_UART_BAUD_RATE_76800
    {   34, 6 }, //  9 = TR_HAL_UART_BAUD_RATE_115200
    {   17, 3 }, // 10 = TR_HAL_UART_BAUD_RATE_230400
    {    8, 0 }, // 11 = TR_HAL_UART_BAUD_RATE_500000
    {    4, 0 }, // 12 = TR_HAL_UART_BAUD_RATE_1000000
    {    2, 0 }, // 13 = TR_HAL_UART_BAUD_RATE_2000000

    // FOR: tr_hal_clock_t clock_mode = TR_HAL_CLOCK_16M
    // for a target of 115200 baud rate
    // 16,000,000 clock / 8 = 2,000,000 / 115200 = 17.36
    // divisor = 17, fractional divisor = 3 (3/8th)

    { 833, 3 }, //  0 = TR_HAL_UART_BAUD_RATE_LPM_2400
    { 416, 5 }, //  1 = TR_HAL_UART_BAUD_RATE_LPM_4800
    { 208, 3 }, //  2 = TR_HAL_UART_BAUD_RATE_LPM_9600
    { 138, 6 }, //  3 = TR_HAL_UART_BAUD_RATE_LPM_14400
    { 104, 1 }, //  4 = TR_HAL_UART_BAUD_RATE_LPM_19200
    {  69, 4 }, //  5 = TR_HAL_UART_BAUD_RATE_LPM_28800
    {  52, 1 }, //  6 = TR_HAL_UART_BAUD_RATE_LPM_38400
    {  34, 6 }, //  7 = TR_HAL_UART_BAUD_RATE_LPM_57600
    {  26, 0 }, //  8 = TR_HAL_UART_BAUD_RATE_LPM_76800
    {  17, 3 }, //  9 = TR_HAL_UART_BAUD_RATE_LPM_115200
    {   8, 5 }, // 10 = TR_HAL_UART_BAUD_RATE_LPM_230400
    {   4, 0 }, // 11 = TR_HAL_UART_BAUD_RATE_LPM_500000
    {   2, 0 }, // 12 = TR_HAL_UART_BAUD_RATE_LPM_1000000
    {   1, 0 }, // 13 = TR_HAL_UART_BAUD_RATE_LPM_2000000

    // FOR: tr_hal_clock_t clock_mode = TR_HAL_CLOCK_1M
    // for a target of 115200 baud rate
    // 921,600 clock / 4 = 230,400 / 115,200 = 2
    // divisor = 2, fractional divisor = 0

    { 96, 0 }, //  0 = TR_HAL_UART_BAUD_RATE_LOW_SPEED_2400
    { 48, 0 }, //  1 = TR_HAL_UART_BAUD_RATE_LOW_SPEED_4800
    { 24, 0 }, //  2 = TR_HAL_UART_BAUD_RATE_LOW_SPEED_9600
    { 16, 0 }, //  3 = TR_HAL_UART_BAUD_RATE_LOW_SPEED_14400
    { 12, 0 }, //  4 = TR_HAL_UART_BAUD_RATE_LOW_SPEED_19200
    {  8, 0 }, //  5 = TR_HAL_UART_BAUD_RATE_LOW_SPEED_28800
    {  6, 0 }, //  6 = TR_HAL_UART_BAUD_RATE_LOW_SPEED_38400
    {  4, 0 }, //  7 = TR_HAL_UART_BAUD_RATE_LOW_SPEED_57600
    {  3, 0 }, //  8 = TR_HAL_UART_BAUD_RATE_LOW_SPEED_76800
    {  2, 0 }, //  9 = TR_HAL_UART_BAUD_RATE_LOW_SPEED_115200

    // FOR: tr_hal_clock_t clock_mode = TR_HAL_CLOCK_32K
    // for a target of 9600 baud rate
    // 38,600 clock / 4 = 9,650 / 9600 = 1.005
    // divisor = 1, fractional divisor = 0

    { 4, 0 }, //  0 = TR_HAL_UART_BAUD_RATE_2400
    { 2, 0 }, //  1 = TR_HAL_UART_BAUD_RATE_4800
    { 1, 0 }, //  2 = TR_HAL_UART_BAUD_RATE_9600

};
// for accessing the lookup table
#define DIVISOR 0
#define FRAC_DIVISOR 1


/// ***************************************************************************
/// tr_hal_uart_get_uart_register_address
/// ***************************************************************************
UART_REGISTERS_T* tr_hal_uart_get_uart_register_address(tr_hal_uart_id_t uart_id)
{
    if      (uart_id == UART_0_ID) { return UART0_CHIP_REGISTERS;}
    else if (uart_id == UART_1_ID) { return UART1_CHIP_REGISTERS;}
    else if (uart_id == UART_2_ID) { return UART2_CHIP_REGISTERS;}

    return 0;
}

/// ***************************************************************************
/// check_uart_is_ready
///
/// checks oif UART is ready for use.
/// ready means: UART ID is valid, init has been completed, and it is powered on
/// ***************************************************************************
static tr_hal_status_t check_uart_is_ready(tr_hal_uart_id_t uart_id)
{
    // check if UART is within range
    if (uart_id >= TR_NUMBER_OF_UARTS)
    {
        return TR_HAL_ERROR_INVALID_UART_ID;
    }

    // check if UART has been initialized
    if (!(g_uart_init_completed[uart_id]))
    {
        return TR_HAL_ERROR_NOT_INITIALIZED;
    }

    // check if UART is powered on
    if (!(g_uart_powered[uart_id]))
    {
        return TR_HAL_ERROR_UART_NOT_POWERED;
    }

    return TR_HAL_SUCCESS;
}

/// ***************************************************************************
/// disable_and_clear_irq_and_disable_peripheral_clock
///
/// given a UART ID, this clears and disables interrupts and disables the peripheral clock
/// ***************************************************************************
static void disable_and_clear_irq_and_disable_peripheral_clock(tr_hal_uart_id_t uart_id)
{
    if (uart_id == UART_0_ID)
    {
        NVIC_DisableIRQ(Uart0_IRQn);
        NVIC_ClearPendingIRQ(Uart0_IRQn);
    }
    else if (uart_id == UART_1_ID)
    {
        NVIC_DisableIRQ(Uart1_IRQn);
        NVIC_ClearPendingIRQ(Uart1_IRQn);
    }
    else if (uart_id == UART_2_ID)
    {
        NVIC_DisableIRQ(Uart2_IRQn);
        NVIC_ClearPendingIRQ(Uart2_IRQn);
    }
}


/// ***************************************************************************
/// clear_uart_irq_and_enable_peripheral_clock
///
/// given a UART ID, this clears any pending interrupt and enables the peripheral clock
/// ***************************************************************************
static void clear_uart_irq_and_enable_peripheral_clock(tr_hal_uart_id_t uart_id)
{
    if (uart_id == UART_0_ID)
    {
        NVIC_ClearPendingIRQ(Uart0_IRQn);
    }
    else if (uart_id == UART_1_ID)
    {
        NVIC_ClearPendingIRQ(Uart1_IRQn);
    }
    else if (uart_id == UART_2_ID)
    {
        NVIC_ClearPendingIRQ(Uart2_IRQn);
    }
}


/// ***************************************************************************
/// enable_uart_interrupt_and_set_priority
///
/// given a UART ID, this enables the interrupts and sets the int priority
/// ***************************************************************************
static void enable_uart_interrupt_and_set_priority(tr_hal_uart_id_t uart_id,
                                                   tr_hal_int_pri_t interrupt_priority)
{
    if (uart_id == UART_0_ID)
    {
        NVIC_SetPriority(Uart0_IRQn, interrupt_priority);
        NVIC_EnableIRQ(Uart0_IRQn);
    }
    else if (uart_id == UART_1_ID)
    {
        NVIC_SetPriority(Uart1_IRQn, interrupt_priority);
        NVIC_EnableIRQ(Uart1_IRQn);
    }
    else if (uart_id == UART_2_ID)
    {
        NVIC_SetPriority(Uart2_IRQn, interrupt_priority);
        NVIC_EnableIRQ(Uart2_IRQn);
    }
}


/// ***************************************************************************
/// check_uart_settings_valid
///
/// this validates the settings to make sure trhere are no errors or conflicts
/// before tr_hal_uart_init runs
/// ***************************************************************************
static tr_hal_status_t check_uart_settings_valid(tr_hal_uart_id_t        uart_id,
                                                 tr_hal_uart_settings_t* uart_settings)
{
    tr_hal_status_t status;

    // *** check the TX and RX pins
    if (!(tr_hal_uart_check_pins_valid(uart_id,
                                       uart_settings->tx_pin,
                                       uart_settings->rx_pin)))
    {
        return TR_HAL_ERROR_INVALID_PINS;
    }

    // check if TX is already configured to another peripheral
    status = tr_hal_gpio_mgr_check_gpio(uart_settings->tx_pin);
    if (status != TR_HAL_SUCCESS)
    {
        return status;
    }

    // check if RX is already configured to another peripheral
    status = tr_hal_gpio_mgr_check_gpio(uart_settings->rx_pin);
    if (status != TR_HAL_SUCCESS)
    {
        return status;
    }

    // make sure we picked a valid clock
    tr_hal_clock_t clock_to_use = uart_settings->clock_to_use;

    if ( (clock_to_use != TR_HAL_CLOCK_32M)
        && (clock_to_use != TR_HAL_CLOCK_16M)
        && (clock_to_use != TR_HAL_CLOCK_1M)
        && (clock_to_use != TR_HAL_CLOCK_32K) )
    {
        return TR_HAL_ERROR_INVALID_CLOCK;
    }

    // *** baud rate
    uint16_t baud_rate = uart_settings->baud_rate;

    // check baud rates for 32M and 16M clock settings
    if (   (clock_to_use == TR_HAL_CLOCK_32M)
        || (clock_to_use == TR_HAL_CLOCK_16M) )
    {
        if (      (baud_rate != TR_HAL_UART_BAUD_RATE_2400)
               && (baud_rate != TR_HAL_UART_BAUD_RATE_4800)
               && (baud_rate != TR_HAL_UART_BAUD_RATE_9600)
               && (baud_rate != TR_HAL_UART_BAUD_RATE_14400)
               && (baud_rate != TR_HAL_UART_BAUD_RATE_19200)
               && (baud_rate != TR_HAL_UART_BAUD_RATE_28800)
               && (baud_rate != TR_HAL_UART_BAUD_RATE_38400)
               && (baud_rate != TR_HAL_UART_BAUD_RATE_57600)
               && (baud_rate != TR_HAL_UART_BAUD_RATE_76800)
               && (baud_rate != TR_HAL_UART_BAUD_RATE_115200)
               && (baud_rate != TR_HAL_UART_BAUD_RATE_230400)
               && (baud_rate != TR_HAL_UART_BAUD_RATE_500000)
               && (baud_rate != TR_HAL_UART_BAUD_RATE_1000000)
               && (baud_rate != TR_HAL_UART_BAUD_RATE_2000000) )
           {
               return TR_HAL_ERROR_INVALID_BAUD_RATE;
           }
    }
    // check baud rates for 1M clock settings
    else if (clock_to_use == TR_HAL_CLOCK_1M)
    {
        if (      (baud_rate != TR_HAL_UART_BAUD_RATE_2400)
               && (baud_rate != TR_HAL_UART_BAUD_RATE_4800)
               && (baud_rate != TR_HAL_UART_BAUD_RATE_9600)
               && (baud_rate != TR_HAL_UART_BAUD_RATE_14400)
               && (baud_rate != TR_HAL_UART_BAUD_RATE_19200)
               && (baud_rate != TR_HAL_UART_BAUD_RATE_28800)
               && (baud_rate != TR_HAL_UART_BAUD_RATE_38400)
               && (baud_rate != TR_HAL_UART_BAUD_RATE_57600)
               && (baud_rate != TR_HAL_UART_BAUD_RATE_76800)
               && (baud_rate != TR_HAL_UART_BAUD_RATE_115200) )
           {
               return TR_HAL_ERROR_INVALID_BAUD_RATE;
           }
    }
    // check baud rates for 32K clock
    else if (clock_to_use == TR_HAL_CLOCK_32K)
    {
        if (      (baud_rate != TR_HAL_UART_BAUD_RATE_2400)
               && (baud_rate != TR_HAL_UART_BAUD_RATE_4800)
               && (baud_rate != TR_HAL_UART_BAUD_RATE_9600) )
           {
               return TR_HAL_ERROR_INVALID_BAUD_RATE;
           }
    }

    // *** RX DMA enabled
    if (uart_settings->rx_dma_enabled)
    {
        // must supply a buffer if RX DMA is enabled
        if (uart_settings->rx_dma_buffer == NULL)
        {
            return TR_HAL_ERROR_DMA_RX_BUFFER_MISSING;
        }
        // buffer can't be less than min size
        if (uart_settings->rx_dma_buff_length < DMA_RX_BUFF_MINIMUM_SIZE)
        {
            return TR_HAL_ERROR_DMA_RX_BUFF_BAD_LEN;
        }

        // NOTE: might need a check for length being a multiple of 4 bytes
        // buffer MUST be a multiple of 4 bytes. This has to do with the
        // dma address needing to start aligned (see section 14.3)
        if (((uart_settings->rx_dma_buff_length) & 3) > 0)
        {
            return TR_HAL_DMA_BUFF_UNALIGNED_LENGTH;
        }
    }

    // *** rx_bytes_before_trigger can only be certain values
    uint8_t bytes_before_trigger = uart_settings->rx_bytes_before_trigger;

    if (      (bytes_before_trigger != FCR_TRIGGER_1_BYTE)
           && (bytes_before_trigger != FCR_TRIGGER_4_BYTES)
           && (bytes_before_trigger != FCR_TRIGGER_8_BYTES)
           && (bytes_before_trigger != FCR_TRIGGER_14_BYTES) )
    {
        return TR_HAL_ERROR_TRIGGER_BYTES_INVALID;
    }

    // if hardware flow control is enabled and this is not UART1 then that is an error
    // only UART1 supports flow control
    if ( (uart_settings->hardware_flow_control_enabled)
        && (uart_id != UART_1_ID))
    {
        return TR_HAL_ERROR_HWFC_NOT_VALID;
    }

    // no errors found, return OK
    return TR_HAL_SUCCESS;
}


/// ***************************************************************************
/// set_uart_clock_mode
///
/// this sets the:
///     SYS_CTRL_CHIP_REGISTERS->system_clock_control_1
///     fields uart0_clk_sel, uart1_clk_sel, uart2_clk_sel
/// based on clock setting
///
/// it also sets low_power_mode in the UART register based on clock setting
///     uart_register_address->low_speed_mode
///
/// ***************************************************************************
static uint32_t set_uart_clock_mode(tr_hal_uart_id_t   uart_id,
                                    tr_hal_clock_t     clock_to_use,
                                    tr_hal_baud_rate_t baud_rate)
{
    // NOTE: on error checking
    // no checking is needed for: UART ID, clock speed, or baud rate
    // this is meant to be called from tr_hal_uart_init which has already
    // called check_uart_settings_valid whcih checks all of these

    // this is the clock setting to be written to SYS CONTROL system clock register
    uint32_t new_setting = 0;

    // this is the return value, which is an index into the baud rate table
    uint32_t baud_rate_index = 0;

    // UART registers for this UART ID
    UART_REGISTERS_T* uart_register_address = tr_hal_uart_get_uart_register_address(uart_id);

    // based on clock setting we determine register new setting and baud rate index
    switch (clock_to_use)
    {
        // *********************************************
        // 32 MHz per clk
        // *********************************************
        case TR_HAL_CLOCK_32M:
        {
            // baud rate is no offset
            baud_rate_index = baud_rate;

            // use per_clk
            new_setting = SYS_CTRL_UART_CLOCK_SELECT_PER_CLOCK;

            // set low speed mode DISABLED (normal speed)
            uart_register_address->low_speed_mode = NORMAL_SPEED_MODE_ENABLED;

            // **** we need to make sure the per_clk is set to 32M

            // read the existing system_clock_control_0
            uint32_t sysclock_reg_0 = SYS_CTRL_CHIP_REGISTERS->system_clock_control_0;

            // per clk select is bits 2 and 3
            uint32_t sysclock_reg_0_clear_mask = SYS_CTRL_PER_CLK_SELECT_MASK;

            // erase the current setting
            sysclock_reg_0 &= ~(sysclock_reg_0_clear_mask);

            // add in the new setting
            sysclock_reg_0 |= SYS_CTRL_PER_CLK_SELECT_XTAL_CLK;

            // write the register
            SYS_CTRL_CHIP_REGISTERS->system_clock_control_0 = sysclock_reg_0;
        }
        break;

        // *********************************************
        // 16 MHz per clk
        // *********************************************
        case TR_HAL_CLOCK_16M:
        {
            // baud rate is offset by the full speed baud rates
            baud_rate_index = baud_rate + TR_HAL_NUM_FULL_SPEED_BAUD_RATES;

            // use per_clk
            new_setting = SYS_CTRL_UART_CLOCK_SELECT_PER_CLOCK;

            // set low speed mode DISABLED (normal speed)
            uart_register_address->low_speed_mode = NORMAL_SPEED_MODE_ENABLED;

            // **** we need to set the per_clk to 16M

            // read the existing system_clock_control_0
            uint32_t sysclock_reg_0 = SYS_CTRL_CHIP_REGISTERS->system_clock_control_0;

            // per clk select is bits 2 and 3
            uint32_t sysclock_reg_0_clear_mask = SYS_CTRL_PER_CLK_SELECT_MASK;

            // erase the current setting
            sysclock_reg_0 &= ~(sysclock_reg_0_clear_mask);

            // add in the new setting
            sysclock_reg_0 |= SYS_CTRL_PER_CLK_SELECT_XTAL_CLK_DIV2;

            // write the register
            SYS_CTRL_CHIP_REGISTERS->system_clock_control_0 = sysclock_reg_0;
        }
        break;

        // *********************************************
        // ~1 MHz RC oscillator (921.6 KHz)
        // *********************************************
        case TR_HAL_CLOCK_1M:
        {
            // baud rate offset by the full speed and LPM baud rates
            baud_rate_index = baud_rate + TR_HAL_NUM_FULL_SPEED_BAUD_RATES
                                        + TR_HAL_NUM_LPM_BAUD_RATES;

            // use rco1m
            new_setting = SYS_CTRL_UART_CLOCK_SELECT_RCO_1M;

            // set low speed mode ENABLED
            uart_register_address->low_speed_mode = LOW_SPEED_MODE_ENABLED;
        }
        break;

        // *********************************************
        // ~32 KHz RC oscillator (38.4 KHz)
        // *********************************************
        case TR_HAL_CLOCK_32K:
        {
            // baud rate offset by the full speed, LPM, and low speed baud rates
            baud_rate_index = baud_rate + TR_HAL_NUM_FULL_SPEED_BAUD_RATES
                                        + TR_HAL_NUM_LPM_BAUD_RATES
                                        + TR_HAL_NUM_LOW_SPEED_BAUD_RATES;
            // use rco32k
            new_setting = SYS_CTRL_UART_CLOCK_SELECT_RCO_32K;

            // set low speed mode ENABLED
            uart_register_address->low_speed_mode = LOW_SPEED_MODE_ENABLED;
        }
        break;

        // we will never get here (error checked to make sure we use a valud clock)
        // but this helps some of the code review tools to not complain
        default:

    }

    // read the existing clock register that contains the UART clock select
    uint32_t sysclock_reg_1 = SYS_CTRL_CHIP_REGISTERS->system_clock_control_1;

    // *****************************************************
    // need to clear any existing setting of the register.
    // each setting is 2 bits starting with UART0_CHIP_REGISTERS
    // *****************************************************

    // UART0 is LSB 2 bits
    uint32_t clear_mask = 0x03;

    // UART0 is first 2 bits
    if (uart_id == UART_0_ID)
    {
        clear_mask = clear_mask << SYS_CTRL_UART0_CLOCK_SELECT_BIT_SHIFT;
        new_setting = new_setting << SYS_CTRL_UART0_CLOCK_SELECT_BIT_SHIFT;
    }
    // UART1 is next 2 bits
    else if (uart_id == UART_1_ID)
    {
        clear_mask = clear_mask << SYS_CTRL_UART1_CLOCK_SELECT_BIT_SHIFT;
        new_setting = new_setting << SYS_CTRL_UART1_CLOCK_SELECT_BIT_SHIFT;
    }
    // UART2 is next 2 bits
    else if (uart_id == UART_2_ID)
    {
        clear_mask = clear_mask << SYS_CTRL_UART2_CLOCK_SELECT_BIT_SHIFT;
        new_setting = new_setting << SYS_CTRL_UART2_CLOCK_SELECT_BIT_SHIFT;
    }

    // invert so we have 0s where we want to clear
    clear_mask = ~(clear_mask);

    // *****************************************************
    // now we clear, set, and write back to the register
    // *****************************************************

    // clear the setting
    sysclock_reg_1 &= clear_mask;

    // add in the new setting
    sysclock_reg_1 |= new_setting;

    // write the register
    SYS_CTRL_CHIP_REGISTERS->system_clock_control_1 = sysclock_reg_1;

    return baud_rate_index;
}


/// ***************************************************************************
/// tr_hal_uart_init
///
/// this initializes one of the 3 UARTs
///
/// this takes the uart_id and a tr_hal_uart_settings_t config struct
/// the config struct is used to set up all aspects of the UART.
/// if a configuration needs to be changed, uninit the UART, change
/// the settings in the tr_hal_uart_settings_t and then re-init the UART
///
/// ***************************************************************************
tr_hal_status_t tr_hal_uart_init(tr_hal_uart_id_t        uart_id,
                                 tr_hal_uart_settings_t* uart_settings)
{
    uint32_t temp __attribute__((unused));

    // variable for chip register address
    UART_REGISTERS_T* uart_register_address;

    // must pass in settings
    if (uart_settings == NULL)
    {
        return TR_HAL_ERROR_NULL_PARAMS;
    }

    // if uart_id is out of bounds then abort
    if (uart_id >= TR_NUMBER_OF_UARTS)
    {
        return TR_HAL_ERROR_INVALID_UART_ID;
    }

    // settings must be valid
    tr_hal_status_t status = check_uart_settings_valid(uart_id, uart_settings);
    if (status != TR_HAL_SUCCESS)
    {
        return status;
    }

    // check the UART ID
    if      (uart_id == UART_0_ID) { uart_register_address = UART0_CHIP_REGISTERS;}
    else if (uart_id == UART_1_ID) { uart_register_address = UART1_CHIP_REGISTERS;}
    else if (uart_id == UART_2_ID) { uart_register_address = UART2_CHIP_REGISTERS;}
    else    { return TR_HAL_ERROR_INVALID_UART_ID;}


    // set the TX and RX pins for UART mode
    status = tr_hal_uart_set_tx_rx_pins(uart_id,
                                        uart_settings->tx_pin,
                                        uart_settings->rx_pin);

    // if the settings pins failed then abort with error
    if (status != TR_HAL_SUCCESS)
    {
        return status;
    }

    // if this is UART1 and HW flow control is enabled, set the RTS/CTS pins
    if ( (uart_id == UART_1_ID)
        && (uart_settings->hardware_flow_control_enabled) )
    {
        status = tr_hal_uart_set_rts_cts_pins(uart_id,
                                              uart_settings->rts_pin,
                                              uart_settings->cts_pin);
        // abort on error
        if (status != TR_HAL_SUCCESS)
        {
            return status;
        }
    }

    // make sure we are powered on
    if (g_uart_powered[uart_id] == false)
    {
        // the current function does all the work that happens in tr_hal_uart_power_on(),
        // so we can just say we are on after this function completes
        g_uart_powered[uart_id] = true;
    }

    // clear any pending interrupts and turn on the peripheral clock
    clear_uart_irq_and_enable_peripheral_clock(uart_id);

    // clear FIFO by disabling and clearing both RX and TX -- see section 10.4.6
    uart_register_address->FIFO_control_register = 0;
    uart_register_address->FIFO_control_register = (FCR_CLEAR_RECEIVER | FCR_CLEAR_TRANSMIT);

    // Disable UART interrupts by setting enable to 0 for all
    uart_register_address->interrupt_enable_register = 0;

    // read the clock mode and baud rate value from the config
    tr_hal_clock_t clock_to_use  = uart_settings->clock_to_use;
    tr_hal_baud_rate_t baud_rate = uart_settings->baud_rate;

    // set the clock mode and return an index into the baud rate settings table
    uint32_t baud_rate_index = set_uart_clock_mode(uart_id,
                                                   clock_to_use,
                                                   baud_rate);

    // set the baud rate, see section 18.3.3
    uart_register_address->divisor_latch_register = baud_rate_lookup_table[baud_rate_index][DIVISOR];
    uart_register_address->fractional_divisor_latch = baud_rate_lookup_table[baud_rate_index][FRAC_DIVISOR];

    // set the line control register by setting the desired
    // data_bits, stop_bits, and parity at the same time
    uint8_t combined_value = uart_settings->data_bits | uart_settings->stop_bits | uart_settings->parity;
    uart_register_address->line_control_register = combined_value;

    // read RBR to clear out any noise
    temp = uart_register_address->receive_buffer_register;

    // initially set MCR to 0, this controls flow control
    uart_register_address->modem_control_register = 0;

    // turn DMA off

    // disable all DMA interrupts
    //uart_register_address->DMA_interrupt_enable_register = 0;
    // disable TX and RX DMA
    uart_register_address->DMA_rx_enable = UART_DMA_DISABLE;
    uart_register_address->DMA_tx_enable = UART_DMA_DISABLE;
    // set length to 0 so as to ensure no confusion when we add a buffer later
    uart_register_address->DMA_rx_buffer_len = 0;
    uart_register_address->DMA_tx_buffer_len = 0;

    // if we are using DMA TX or DMA RX, enable the DMA interrupts
    // and enable DMA in the FIFO
    if (uart_settings->tx_dma_enabled)
    {
        uart_register_address->interrupt_enable_register |= IER_ENABLE_DMA_TX_INT;
        uart_register_address->FIFO_control_register = FCR_DMA_SELECT;
    }
    if (uart_settings->rx_dma_enabled)
    {
        uart_register_address->interrupt_enable_register |= IER_ENABLE_DMA_RX_INT;
        uart_register_address->FIFO_control_register = FCR_DMA_SELECT;
        // for RX DMA we also need to set the RX BUFFER
        uint8_t* ptr_buffer = uart_settings->rx_dma_buffer;
        uart_register_address->DMA_rx_buffer_addr = (uint32_t) ptr_buffer;
        uart_register_address->DMA_rx_buffer_len = uart_settings->rx_dma_buff_length;

        // enable DMA receive
        uart_register_address->DMA_rx_enable = 1;
    }

    // if we are using HW flow control, enable that
    if (uart_settings->hardware_flow_control_enabled)
    {
        // HWFC is only valid on UART1
        if (uart_id != UART_1_ID)
        {
            return TR_HAL_ERROR_HWFC_NOT_VALID;
        }

        // enable the HW FC asked for
        uart_register_address->modem_control_register = (MCR_SET_RTS_READY | MCR_SET_CTS_ENABLED);

        // enable the corresponding interrupts
        uart_register_address->interrupt_enable_register |= IER_ENABLE_FRAMING_PARITY_OVERRUN_ERROR_INT;
        uart_register_address->interrupt_enable_register |= IER_ENABLE_MODEM_STATUS_INT;
    }

    // if we are enabling interrupts...
    if (uart_settings->enable_chip_interrupts)
    {
        // ...enable the line status interrupt which fires when we have parity error, framing or overrun error
        uart_register_address->interrupt_enable_register |= IER_ENABLE_FRAMING_PARITY_OVERRUN_ERROR_INT;
        uart_register_address->interrupt_enable_register |= IER_ENABLE_READY_TO_TRANSMIT_INT;
        uart_register_address->interrupt_enable_register |= IER_ENABLE_RECEIVE_DATA_AVAIL_INT;
    }

    // NOTE: this was checked for a valid value before
    tr_hal_fifo_trigger_t byte_trigger = uart_settings->rx_bytes_before_trigger;
    // respect what is there as there is a DMA enable bit that is needed when doing DMA
    uart_register_address->FIFO_control_register |= byte_trigger;


    // if we are enabling interrupts...
    if (uart_settings->enable_chip_interrupts)
    {
        // set the UART interrupt priority and enable
        enable_uart_interrupt_and_set_priority(uart_id, uart_settings->interrupt_priority);
    }

    // init has been completed for this UART, so copy in the settings
    memcpy(&(g_current_uart_settings[uart_id]), uart_settings, sizeof(tr_hal_uart_settings_t));
    g_uart_init_completed[uart_id] = true;

    // clear the dma rx index and data buffer
    g_curr_dma_rx_index[uart_id] = 0;
    if (g_current_uart_settings[uart_id].rx_dma_buffer != NULL)
    {
        uint8_t* rx_buff = g_current_uart_settings[uart_id].rx_dma_buffer;
        uint16_t buff_len = g_current_uart_settings[uart_id].rx_dma_buff_length;
        memset(rx_buff, 0, buff_len);
    }

    // enable UART
    uart_register_address->enable = UART_ENABLE;

    // enable WAKE from sleep
    uart_register_address->wake_enable = UART_WAKE_ENABLE;

    return 0;
}


/// ***************************************************************************
/// uninit the UART specified
///
/// clears all interrupts, empties FIFOs, sets pins back to GPIO mode
/// ***************************************************************************
tr_hal_status_t tr_hal_uart_uninit(tr_hal_uart_id_t uart_id)
{
    // the power off API does all the actions we need to uninit except put the pins back
    tr_hal_status_t status = tr_hal_uart_power_off(uart_id);

    tr_hal_uart_settings_t* uart_settings = &(g_current_uart_settings[uart_id]);

    // set the pins back to GPIO
    // we ignore status here. nothing to do if one of these fails
    tr_hal_gpio_set_mode(uart_settings->tx_pin, TR_HAL_GPIO_MODE_GPIO);
    tr_hal_gpio_set_mode(uart_settings->rx_pin, TR_HAL_GPIO_MODE_GPIO);

    // release the GPIOs used for TX and RX
    tr_hal_gpio_mgr_release_gpio(uart_settings->tx_pin);
    tr_hal_gpio_mgr_release_gpio(uart_settings->rx_pin);

    // if we set RTS/CTS release those pins also
    if ( (uart_id == UART_1_ID) && (uart_settings->hardware_flow_control_enabled) )
    {
        tr_hal_gpio_mgr_release_gpio(uart_settings->rts_pin);
        tr_hal_gpio_mgr_release_gpio(uart_settings->cts_pin);
    }

    g_uart_init_completed[uart_id] = false;

    return status;
}


/// ***************************************************************************
/// tr_hal_uart_set_power_mode
///
/// ***************************************************************************
tr_hal_status_t tr_hal_uart_set_power_mode(tr_hal_uart_id_t    uart_id,
                                           tr_hal_power_mode_t power_mode)
{
    // check if UART is within range
    if (uart_id >= TR_NUMBER_OF_UARTS)
    {
        return TR_HAL_ERROR_INVALID_UART_ID;
    }

    // check if UART has been initialized
    if (!(g_uart_init_completed[uart_id]))
    {
        return TR_HAL_ERROR_NOT_INITIALIZED;
    }

    uint32_t baud_rate_index = 0;
    UART_REGISTERS_T* uart_register_address = tr_hal_uart_get_uart_register_address(uart_id);

    // on WAKE set the clock and baud rate back to normal
    if (power_mode == TR_HAL_POWER_MODE_0)
    {
        baud_rate_index = set_uart_clock_mode(uart_id,
                                              g_current_uart_settings[uart_id].clock_to_use,
                                              g_current_uart_settings[uart_id].baud_rate);
        // set baud rate
        uart_register_address->divisor_latch_register = baud_rate_lookup_table[baud_rate_index][DIVISOR];
        uart_register_address->fractional_divisor_latch = baud_rate_lookup_table[baud_rate_index][FRAC_DIVISOR];
    }

    // on LITE SLEEP set the clock and baud rate to sleep if we are on during sleep
    else if (power_mode == TR_HAL_POWER_MODE_1)
    {
        // are we on in when sleeping?
        if (g_current_uart_settings[uart_id].run_when_sleeping)
        {
            // make sure the clock selected is enabled
            if ( (g_current_uart_settings[uart_id].sleep_clock_to_use) == TR_HAL_CLOCK_1M)
            {
                // make sure the 1M is enabled
                tr_hal_power_enable_clock(TR_HAL_CLOCK_1M);
            }
            else if ( (g_current_uart_settings[uart_id].sleep_clock_to_use) == TR_HAL_CLOCK_32K)
            {
                // make sure the 32K RC is enabled
                tr_hal_power_enable_clock(TR_HAL_CLOCK_32K);
            }
            else
            {
                // this is an error, unexpected clock
                return TR_HAL_UNSUPPORTED_CLOCK;
            }

            // set the UART to the clock specified
            baud_rate_index = set_uart_clock_mode(uart_id,
                                                  g_current_uart_settings[uart_id].sleep_clock_to_use,
                                                  g_current_uart_settings[uart_id].sleep_baud_rate);
        }
        // set baud rate
        uart_register_address->divisor_latch_register = baud_rate_lookup_table[baud_rate_index][DIVISOR];
        uart_register_address->fractional_divisor_latch = baud_rate_lookup_table[baud_rate_index][FRAC_DIVISOR];
    }

    // NOTE: for MODE_DEEP_SLEEP and MODE_DEEP_POWER_DOWN
    // the UART is effectively OFF, so that is handled in other code
    // (The UART does not need to know it is being turned off)

    // unknown power mode
    else
    {
        return TR_HAL_UNSUPPORTED_POWER_MODE;
    }

    return TR_HAL_SUCCESS;
}


/// ***************************************************************************
/// tr_hal_uart_power_off
///
/// disables and clears interrupts
/// ***************************************************************************
tr_hal_status_t tr_hal_uart_power_off(tr_hal_uart_id_t uart_id)
{
    // check if UART is within range
    if (uart_id >= TR_NUMBER_OF_UARTS)
    {
        return TR_HAL_ERROR_INVALID_UART_ID;
    }

    // check if UART has been initialized
    if (!(g_uart_init_completed[uart_id]))
    {
        return TR_HAL_ERROR_NOT_INITIALIZED;
    }

    // check if UART is already powered off
    // if we think the UART is already powered off then the code does nothing
    // and just returns with SUCCESS status. NOTE: if the user/app has changed
    // the register settings OUTSIDE of the Trident HAL APIs, the user/app may
    // need to fix the register settings directly since this call will not do
    // it (since it thinks UART is already in the desired state)
    if (!(g_uart_powered[uart_id]))
    {
        return TR_HAL_SUCCESS;
    }

    // *** checks are done, now power off the UART ***

    // get the chip register address
    UART_REGISTERS_T* uart_register_address = tr_hal_uart_get_uart_register_address(uart_id);

    // disable and clear interrupt and turn off clock
    disable_and_clear_irq_and_disable_peripheral_clock(uart_id);

    // disable interrupts for DMA and turn DMA off
    uart_register_address->DMA_rx_enable = UART_DMA_DISABLE;
    uart_register_address->DMA_tx_enable = UART_DMA_DISABLE;

    // disable and clear FIFO by setting both RX and TX -- see section 10.4.6
    uart_register_address->FIFO_control_register = 0;
    uart_register_address->FIFO_control_register = (FCR_CLEAR_RECEIVER | FCR_CLEAR_TRANSMIT);

    // disable interrupts if that flag is set
    if (g_current_uart_settings[uart_id].wake_on_interrupt == false)
    {
        // Disable UART interrupts by setting enable to 0 for all
        uart_register_address->interrupt_enable_register = 0;
    }

    // we are now powered off
    g_uart_powered[uart_id] = false;

    return TR_HAL_SUCCESS;
}


/// ***************************************************************************
/// tr_hal_uart_power_on
///
/// DO NOT call this before initializing a UART. tr_hal_uart_init will do the same
/// things that are done in this function. This function is here so if a UART
/// is initialized and running and it needs to SLEEP, then tr_hal_uart_power_off
/// can be called. To recover from this condition, call tr_hal_uart_power_on
///
/// ***************************************************************************
tr_hal_status_t tr_hal_uart_power_on(tr_hal_uart_id_t uart_id)
{
    // check if UART is within range
    if (uart_id >= TR_NUMBER_OF_UARTS)
    {
        return TR_HAL_ERROR_INVALID_UART_ID;
    }

    // check if UART has been initialized
    if (!(g_uart_init_completed[uart_id]))
    {
        return TR_HAL_ERROR_NOT_INITIALIZED;
    }

    // check if UART is already powered on
    // if we think the UART is already powered off then the code does nothing
    // and just returns with SUCCESS status. NOTE: if the user/app has changed
    // the register settings OUTSIDE of the Trident HAL APIs, the user/app may
    // need to fix the register settings directly since this call will not do
    // it (since it thinks UART is already in the desired state)
    if (g_uart_powered[uart_id])
    {
        return TR_HAL_SUCCESS;
    }

    // *** checks are done, now power ON the UART ***

    // get the chip register address
    UART_REGISTERS_T* uart_register_address = tr_hal_uart_get_uart_register_address(uart_id);
    tr_hal_uart_settings_t* uart_settings = &(g_current_uart_settings[uart_id]);

    // clear any pending interrupts and turn on the peripheral clock
    clear_uart_irq_and_enable_peripheral_clock(uart_id);

    // clear FIFO by disabling and clearing both RX and TX -- see section 10.4.6
    uart_register_address->FIFO_control_register = 0;
    uart_register_address->FIFO_control_register = (FCR_CLEAR_RECEIVER | FCR_CLEAR_TRANSMIT);

    // *** enable DMA
    // if we are using DMA TX or DMA RX, enable the DMA interrupts
    // and enable DMA in the FIFO
    if (uart_settings->tx_dma_enabled)
    {
        uart_register_address->interrupt_enable_register |= IER_ENABLE_DMA_TX_INT;
        uart_register_address->FIFO_control_register = FCR_DMA_SELECT;
    }
    if (uart_settings->rx_dma_enabled)
    {
        uart_register_address->interrupt_enable_register |= IER_ENABLE_DMA_RX_INT;
        uart_register_address->FIFO_control_register = FCR_DMA_SELECT;
        // for RX DMA we also need to set the RX BUFFER
        uint8_t* ptr_buffer = uart_settings->rx_dma_buffer;
        uart_register_address->DMA_rx_buffer_addr = (uint32_t) ptr_buffer;
        uart_register_address->DMA_rx_buffer_len = uart_settings->rx_dma_buff_length;

        // enable DMA receive
        uart_register_address->DMA_rx_enable = 1;
    }

    // *** enable interrupts

    // if we are using HW flow control, enable the interrupts needed
    if (uart_settings->hardware_flow_control_enabled)
    {
        // enable the corresponding interrupts
        uart_register_address->interrupt_enable_register |= IER_ENABLE_FRAMING_PARITY_OVERRUN_ERROR_INT;
        uart_register_address->interrupt_enable_register |= IER_ENABLE_MODEM_STATUS_INT;
    }

    // if we are enabling interrupts...
    if (uart_settings->enable_chip_interrupts)
    {
        // ...enable parity error, transmit free, data received
        uart_register_address->interrupt_enable_register |= IER_ENABLE_FRAMING_PARITY_OVERRUN_ERROR_INT;
        uart_register_address->interrupt_enable_register |= IER_ENABLE_READY_TO_TRANSMIT_INT;
        uart_register_address->interrupt_enable_register |= IER_ENABLE_RECEIVE_DATA_AVAIL_INT;
    }

    // *** enable FIFO

    // enable FIFO with the correct byte trigger
    tr_hal_fifo_trigger_t byte_trigger = uart_settings->rx_bytes_before_trigger;
    // respect what is there as there is a DMA enable bit that is needed when doing DMA
    uart_register_address->FIFO_control_register |= byte_trigger;

    // *** set interrupt enable and priority

    // if we are enabling interrupts...
    if (uart_settings->enable_chip_interrupts)
    {
        // set the UART interrupt priority and enable
        enable_uart_interrupt_and_set_priority(uart_id, uart_settings->interrupt_priority);
    }

    // we are now powered on
    g_uart_powered[uart_id] = true;

    return TR_HAL_SUCCESS;
}


/// ***************************************************************************
/// tr_hal_uart_raw_rx_one_byte
///
/// does nothing and returns error if the user has defined a user callback for
/// received bytes or if UART is in DMA mode for receive
///
/// receive related (non-error) return status:
/// ------------------------------------------
/// TR_HAL_STATUS_MORE_BYTES .......= more bytes available now
/// TR_HAL_STATUS_DONE .............= no more bytes available now
/// TR_HAL_STATUS_NO_DATA_AVAILABLE = no bytes available now
///
/// ***************************************************************************
tr_hal_status_t tr_hal_uart_raw_rx_one_byte(tr_hal_uart_id_t uart_id,
                                            uint8_t*            byte)
{
    // check if UART is ready
    tr_hal_status_t status = check_uart_is_ready(uart_id);
    if (status != TR_HAL_SUCCESS)
    {
        return status;
    }

    // get the chip register address and settings
    UART_REGISTERS_T* uart_register_address = tr_hal_uart_get_uart_register_address(uart_id);
    tr_hal_uart_settings_t* uart_settings = &(g_current_uart_settings[uart_id]);

    // if we have a user defined callback for received bytes then this is not allowed
    if (uart_settings->rx_handler_function != NULL)
    {
        return TR_HAL_ERROR_RECEIVE_FX_HANDLES_RX;
    }

    // if we have DMA for received bytes then this is not allowed
    if (uart_settings->rx_dma_enabled)
    {
        return TR_HAL_ERROR_DMA_HANDLES_RX;
    }

    // read the line status register
    uint8_t line_status_register = (uint8_t)uart_register_address->line_status_register;

    // if the line status register has data ready, then we can read it
    if (line_status_register & LSR_DATA_READY)
    {
        // when data is available it shows up here
        *byte = (uint8_t)uart_register_address->receive_buffer_register;

        // check for more bytes to read
        line_status_register = (uint8_t)uart_register_address->line_status_register;
        if (line_status_register & LSR_DATA_READY)
        {
            return TR_HAL_STATUS_MORE_BYTES;
        }
        else
        {
            return TR_HAL_STATUS_DONE;
        }
    }
    return TR_HAL_STATUS_NO_DATA_AVAILABLE;
}


/// ***************************************************************************
/// tr_hal_uart_raw_rx_available_bytes
///
/// does nothing and returns error if the user has defined a user callback for
/// received bytes or if UART is in DMA mode for receive
///
/// receive related (non-error) return status:
/// ------------------------------------------
/// TR_HAL_STATUS_MORE_BYTES .......= more bytes available now
/// TR_HAL_STATUS_DONE .............= no more bytes available now
/// TR_HAL_STATUS_NO_DATA_AVAILABLE = no bytes available now
///
/// ***************************************************************************
tr_hal_status_t tr_hal_uart_raw_rx_available_bytes(tr_hal_uart_id_t uart_id,
                                                   uint8_t*            bytes,
                                                   uint16_t         buffer_size,
                                                   uint16_t*        num_returned_bytes)
{
    // check if UART is ready
    tr_hal_status_t status = check_uart_is_ready(uart_id);
    if (status != TR_HAL_SUCCESS)
    {
        return status;
    }

    // get the chip register address and settings
    UART_REGISTERS_T* uart_register_address = tr_hal_uart_get_uart_register_address(uart_id);
    tr_hal_uart_settings_t* uart_settings = &(g_current_uart_settings[uart_id]);

    // if we have a user defined callback for received bytes then this is not allowed
    if (uart_settings->rx_handler_function != NULL)
    {
        return TR_HAL_ERROR_RECEIVE_FX_HANDLES_RX;
    }

    // if we have DMA for received bytes then this is not allowed
    if (uart_settings->rx_dma_enabled)
    {
        return TR_HAL_ERROR_DMA_HANDLES_RX;
    }

    // read the line status register
    uint8_t line_status_register = (uint8_t)uart_register_address->line_status_register;

    uint16_t received_bytes = 0;

    // see if there is NO data
    if (!(line_status_register & LSR_DATA_READY))
    {
        *num_returned_bytes = 0;
        return TR_HAL_STATUS_NO_DATA_AVAILABLE;
    }

    // if the line status register has data ready, then we can read it
    while ( (line_status_register & LSR_DATA_READY) && (received_bytes <= buffer_size) )
    {
        // when data is available it shows up here
        bytes[received_bytes] = (uint8_t)uart_register_address->receive_buffer_register;
        received_bytes++;

        line_status_register = (uint8_t)uart_register_address->line_status_register;
    }
    *num_returned_bytes = received_bytes;

    // we return based on if there is still any data waiting
    line_status_register = (uint8_t)uart_register_address->line_status_register;
    if (line_status_register & LSR_DATA_READY)
    {
        return TR_HAL_STATUS_MORE_BYTES;
    }
    else
    {
        return TR_HAL_STATUS_DONE;
    }
}


/// ***************************************************************************
/// tr_hal_uart_raw_tx_one_byte
///
/// does nothing and returns error if UART is in DMA mode for transmit
///
/// ***************************************************************************
tr_hal_status_t tr_hal_uart_raw_tx_one_byte(tr_hal_uart_id_t uart_id,
                                            const uint8_t       byte_to_send)
{
    // check if UART is ready
    uint32_t status = check_uart_is_ready(uart_id);
    if (status != TR_HAL_SUCCESS)
    {
        return status;
    }

    // get the chip register address and settings
    UART_REGISTERS_T* uart_register_address = tr_hal_uart_get_uart_register_address(uart_id);
    tr_hal_uart_settings_t* uart_settings = &(g_current_uart_settings[uart_id]);

    // if we are setup for transmit using DMA then this is not allowed
    if (uart_settings->tx_dma_enabled)
    {
        return TR_HAL_ERROR_DMA_HANDLES_TX;
    }

    // see if the transmitter is ready
    while ((uart_register_address->line_status_register & LSR_TRANSMITTER_EMPTY) == 0);

    // send it by writing to the transmitter holding register
    uart_register_address->transmitter_holding_register = (uint8_t) byte_to_send;
    return TR_HAL_SUCCESS;
}


/// ***************************************************************************
/// tr_hal_uart_raw_tx_buffer
///
/// does nothing and returns error if UART is in DMA mode for transmit
///
/// attempts to send the bytes in the buffer passed in.
///
/// ***************************************************************************
tr_hal_status_t tr_hal_uart_raw_tx_buffer(tr_hal_uart_id_t uart_id,
                                          const uint8_t*      bytes_to_send,
                                          uint16_t         num_bytes_to_send)
{
    // check if UART is ready
    tr_hal_status_t status = check_uart_is_ready(uart_id);
    if (status != TR_HAL_SUCCESS)
    {
        return status;
    }

    // get the chip register address and settings
    UART_REGISTERS_T* uart_register_address = tr_hal_uart_get_uart_register_address(uart_id);
    tr_hal_uart_settings_t* uart_settings = &(g_current_uart_settings[uart_id]);

    // if we are setup for transmit using DMA then this (raw tx) is not allowed
    if (uart_settings->tx_dma_enabled)
    {
        return TR_HAL_ERROR_DMA_HANDLES_TX;
    }

    for (uint16_t index = 0; index < num_bytes_to_send; index++)
    {
        // we wait for the transmitter to be free, and then send
        while ((uart_register_address->line_status_register & LSR_TRANSMITTER_EMPTY) == 0);

        uart_register_address->transmitter_holding_register = (uint8_t) (bytes_to_send[index]);
    }

    return TR_HAL_SUCCESS;
}


/// ***************************************************************************
/// tr_hal_uart_raw_tx_buffer_no_checks
///
/// shortcut to send a buffer with no checks if the caller is sure the UART
/// is working. This ONLY works for buffers that are < TX_FIFO_SIZE
/// (which is 32 for CZ20)
/// ***************************************************************************
tr_hal_status_t tr_hal_uart_raw_tx_buffer_no_checks(tr_hal_uart_id_t uart_id,
                                                    const uint8_t*            bytes_to_send,
                                                    uint16_t         num_bytes_to_send)
{
    // we can only send up to the FIFO size number of bytes
    if (num_bytes_to_send < TX_FIFO_SIZE)
    {
        return TR_HAL_ERROR_TX_BUFFER_TOO_LONG;
    }

    // get the chip register address and settings
    UART_REGISTERS_T* uart_register_address = tr_hal_uart_get_uart_register_address(uart_id);

    // once the  transmitter is free, we can send as many bytes as fill up the FIFO
    while ((uart_register_address->line_status_register & LSR_TRANSMITTER_EMPTY) == 0);

    // tx bytes to fill up the FIFO
    for (uint16_t index = 0; index < num_bytes_to_send; index++)
    {
        uart_register_address->transmitter_holding_register = bytes_to_send[index];
    }

    return TR_HAL_SUCCESS;
}


/// ***************************************************************************
/// tr_hal_uart_tx_active
///
/// returns true if a transmit is in progress
/// returns false if all transmits have been completed
/// ***************************************************************************
tr_hal_status_t tr_hal_uart_tx_active(tr_hal_uart_id_t uart_id,
                                      bool*            tx_active)
{
    // the return bool can't be NULL
    if (tx_active == NULL)
    {
        return TR_HAL_ERROR_NULL_PARAMS;
    }

    // check if UART is within range
    if (uart_id >= TR_NUMBER_OF_UARTS)
    {
        return TR_HAL_ERROR_INVALID_UART_ID;
    }

    UART_REGISTERS_T* uart_register_address = tr_hal_uart_get_uart_register_address(uart_id);

    if ((uart_register_address->line_status_register & LSR_TRANSMITTER_EMPTY) == 0)
    {
        // if bit is NOT set then the transmit is still in progress
        (*tx_active) = true;
        return TR_HAL_SUCCESS;
    }

    // else if bit is SET then transmit is finished
    (*tx_active) = false;
    return TR_HAL_SUCCESS;
}


/// ***************************************************************************
/// tr_hal_uart_dma_tx_bytes_in_buffer
///
/// does nothing and returns error if UART is in non-DMA mode for transmit
///
/// ***************************************************************************
tr_hal_status_t tr_hal_uart_dma_tx_bytes_in_buffer(tr_hal_uart_id_t uart_id,
                                                   const uint8_t*   bytes_to_send,
                                                   uint16_t         num_bytes_to_send)
{
    // check if UART is ready
    tr_hal_status_t status = check_uart_is_ready(uart_id);
    if (status != TR_HAL_SUCCESS)
    {
        return status;
    }

    // get the chip register address and settings
    UART_REGISTERS_T* uart_register_address = tr_hal_uart_get_uart_register_address(uart_id);
    tr_hal_uart_settings_t* uart_settings = &(g_current_uart_settings[uart_id]);

    // DMA TX must be enabled to call this function
    if (!(uart_settings->tx_dma_enabled))
    {
        return TR_HAL_ERROR_DMA_NOT_ENABLED;
    }

    // set the new buffer and buffer length
    uart_register_address->DMA_tx_buffer_addr = (uint32_t) bytes_to_send;
    uart_register_address->DMA_tx_buffer_len = num_bytes_to_send;

    // enable the transmission
    uart_register_address->DMA_tx_enable = UART_DMA_ENABLE;

    return TR_HAL_SUCCESS;
}


/// ***************************************************************************
/// tr_hal_uart_dma_change_rx_buffer
///
/// for DMA mode, this updates the receive buffer to the new one
/// ***************************************************************************
tr_hal_status_t tr_hal_uart_dma_change_rx_buffer(tr_hal_uart_id_t uart_id,
                                                 uint8_t*         new_receive_buffer,
                                                 uint16_t         new_buffer_length)
{
    // check if UART is ready
    tr_hal_status_t status = check_uart_is_ready(uart_id);
    if (status != TR_HAL_SUCCESS)
    {
        return status;
    }

    // get the chip register address and settings
    UART_REGISTERS_T* uart_register_address = tr_hal_uart_get_uart_register_address(uart_id);
    tr_hal_uart_settings_t* uart_settings = &(g_current_uart_settings[uart_id]);

    // DMA RX must be enabled to call this function
    if (!(uart_settings->rx_dma_enabled))
    {
        return TR_HAL_ERROR_DMA_NOT_ENABLED;
    }

    // make sure the new DMA buffer is a minimum size
    if (new_buffer_length < DMA_RX_BUFF_MINIMUM_SIZE)
    {
        return TR_HAL_ERROR_DMA_BUFFER_TOO_SMALL;
    }

    // FIRST check the DMA buffer and empty out any bytes that have been
    // received since we last checked (if we have a user handler function)
    if (uart_settings->rx_handler_function != NULL)
    {
        hal_internal_handle_rx_dma_bytes(uart_id);
    }
    // else: we expect the app to have read the buffer before changing
    // if a user_rx function is not defined

    // disable DMA RX
    uart_register_address->DMA_rx_enable = UART_DMA_DISABLE;

    // set the new buffer and buffer length
    uart_register_address->DMA_rx_buffer_addr = (uint32_t) new_receive_buffer;
    uart_register_address->DMA_rx_buffer_len = new_buffer_length;

    // enable DMA RX
    uart_register_address->DMA_rx_enable = UART_DMA_ENABLE;

    return TR_HAL_SUCCESS;
}


/// ***************************************************************************
/// tr_hal_uart_dma_receive_buffer_num_bytes_left
///
/// gives the number of free bytes available for future received data
/// ***************************************************************************
tr_hal_status_t tr_hal_uart_dma_receive_buffer_num_bytes_left(tr_hal_uart_id_t uart_id,
                                                              uint32_t*        bytes_left)
{
    // check if UART is ready
    tr_hal_status_t status = check_uart_is_ready(uart_id);
    if (status != TR_HAL_SUCCESS)
    {
        return status;
    }

    // get the chip register address and settings
    UART_REGISTERS_T* uart_register_address = tr_hal_uart_get_uart_register_address(uart_id);
    tr_hal_uart_settings_t* uart_settings = &(g_current_uart_settings[uart_id]);

    // DMA RX must be enabled to call this function
    if (!(uart_settings->rx_dma_enabled))
    {
        return TR_HAL_ERROR_DMA_NOT_ENABLED;
    }

    *bytes_left = uart_register_address->DMA_rx_xfer_len_remaining;

    return TR_HAL_SUCCESS;
}


/// ***************************************************************************
/// tr_hal_uart_internal_interrupt_handler
///
/// called from the code that gets the int from the assembly code
///
/// this TRANSLATES from chip interrupt to applicaton EVENT
///
/// this function figures out what events happened and which ones the user
/// needs to handle. These events are set as bits in a bitmask and then passed
/// to the user handler function
/// ***************************************************************************
void tr_hal_uart_internal_interrupt_handler(tr_hal_uart_id_t uart_id)
{

    // since we are getting an interrupt for a UART we know that it has been
    // initialized and is powered. but we should still check
    uint32_t status = check_uart_is_ready(uart_id);
    if (status != TR_HAL_SUCCESS)
    {
        return;
    }

    // get the chip register address and settings
    UART_REGISTERS_T* uart_register_address = tr_hal_uart_get_uart_register_address(uart_id);
    tr_hal_uart_settings_t* uart_settings = &(g_current_uart_settings[uart_id]);
    uint32_t event_bitmask = 0;

    // *****************************************************
    // if DMA TX is enabled, handle the DMA TX done INT
    // *****************************************************
    if ( (uart_settings->tx_dma_enabled)
        && (uart_register_address->interrupt_identification_register & IIR_DMA_TX_INTERRUPT) )
    {
        // clear transmit interrupt
        uart_register_address->interrupt_identification_register = IIR_DMA_TX_INTERRUPT;

        // turn off DMA transmit (the chip handles transmitting the whole buffer)
        uart_register_address->DMA_tx_enable = UART_DMA_DISABLE;

        // set the event
        event_bitmask |= TR_HAL_UART_EVENT_DMA_TX_COMPLETE;
    }

    // *****************************************************
    // if DMA RX is enabled, handle the DMA RX done INT
    // *****************************************************
    if ( (uart_settings->rx_dma_enabled)
        && (uart_register_address->interrupt_identification_register & IIR_DMA_RX_INTERRUPT) )
    {
        // clear receive interrupt
        uart_register_address->interrupt_identification_register = IIR_DMA_RX_INTERRUPT;

        // the example has the code turning off DMA receive.. but we want to KEEP receiving
        //uart_register_address->DMA_rx_enable = 0;

        // check if the DMA buffer is getting low
        uint32_t bytes_remaining = uart_register_address->DMA_rx_xfer_len_remaining;
        if (bytes_remaining < LOW_BYTES_BUFFER_THRESHHOLD)
        {
            event_bitmask |= TR_HAL_UART_EVENT_DMA_RX_BUFFER_LOW;
        }

        // if we have a user handler function then handle the received bytes
        if (uart_settings->rx_handler_function != NULL)
        {
            hal_internal_handle_rx_dma_bytes(uart_id);
            event_bitmask |= TR_HAL_UART_EVENT_DMA_RX_TO_USER_FX;
        }
        else
        {
            // set the event
            event_bitmask |= TR_HAL_UART_EVENT_DMA_RX_READY;
        }
    }

    // read interrupt part of interrupt_identification_register (IIR)
    uint32_t interrupt_register = uart_register_address->interrupt_identification_register;
    // only part of the register is the interrupts, need to mask that out
    interrupt_register = interrupt_register & IIR_INTERRUPT_MASK;

    // *******************************
    // INTERRUPT: received data
    // this means, in non-DMA RX mode, time to read data from the RBR (receive_buffer_register)
    // clear this by reading from the RBR (receive_buffer_register)
    // *******************************
    if  ((interrupt_register & IIR_RX_DATA_AVAIL_INTERRUPT) > 0)
    {
        // clear the interrupt
        uart_register_address->interrupt_identification_register = IIR_RX_DATA_AVAIL_INTERRUPT;

        // if the user wants the rx bytes sent to a function
        if (uart_settings->rx_handler_function != NULL)
        {
          if ((uart_register_address->line_status_register) & LSR_DATA_READY)
          {
            event_bitmask |= TR_HAL_UART_EVENT_RX_TO_USER_FX;
          }
          // read bytes and send to the user until no more to read
          while ((uart_register_address->line_status_register) & LSR_DATA_READY)
          {
            uint32_t rx_data = uart_register_address->receive_buffer_register;
            uint8_t rx_byte = (rx_data & 0x000000FF);
            uart_settings->rx_handler_function(rx_byte);
          }
        }
        else
        {
          // let the user know to read bytes
          event_bitmask |= TR_HAL_UART_EVENT_RX_READY;
        }
    }

    // *******************************
    // INTERRUPT: transmitter empty
    // this means, in non-DMA TX mode, that more data can be sent to the THR
    // clear this by reading IIR (already done)
    // *******************************
    if ((interrupt_register & IIR_THR_EMPTY_INTERRUPT) > 0)
    {
        // clear the interrupt
        uart_register_address->interrupt_identification_register = IIR_THR_EMPTY_INTERRUPT;

        // let the user know ok to TX
        event_bitmask |= TR_HAL_UART_EVENT_TX_COMPLETE;
    }

    // *******************************
    // INTERRUPT: receiver error
    // this means framing, parity, overrun, or break error
    // clear this by reading the LSR (line status register)
    // *******************************
    if ((interrupt_register & IIR_RECEIVER_ERROR_INTERRUPT) > 0)
    {
        // read what error it is from the LSR
        uint32_t lsr_reg_value = uart_register_address->line_status_register;

        // NOTE: we do separate if stmts here in case more than one error is set

        // error is overrun
        if ((lsr_reg_value & LSR_OVERRUN_ERROR) > 0)
        {
            event_bitmask |= TR_HAL_UART_EVENT_RX_ERR_OVERRUN;
        }

        // error is parity
        if ((lsr_reg_value & LSR_PARITY_ERROR) > 0)
        {
            event_bitmask |= TR_HAL_UART_EVENT_RX_ERR_PARITY;
        }

        // error is framing
        if ((lsr_reg_value & LSR_FRAMING_ERROR) > 0)
        {
            event_bitmask |= TR_HAL_UART_EVENT_RX_ERR_FRAMING;
        }

        // error is break
        if ((lsr_reg_value & LSR_BREAK_INDICATOR) > 0)
        {
            event_bitmask |= TR_HAL_UART_EVENT_RX_ERR_BREAK;
        }

        // clear the interrupt
        uart_register_address->interrupt_identification_register = IIR_RECEIVER_ERROR_INTERRUPT;

        // in order to clear the interrupt, need to clear all 4 error bits of the LSR
        uart_register_address->line_status_register = CLEAR_RX_ERROR_BY_CLEAR_LSR_BITS;
    }

    // *******************************
    // INTERRUPT: relates to hardware flow control
    // set when dCTS, dDSR, TERI, or dDCD is set in the LSR (line status)
    // clear this by reading the MSR (modem status register)
    // *******************************
    if ((interrupt_register & IIR_MODEM_STATUS_INTERRUPT) > 0)
    {
        // clear the interrupt
        uart_register_address->interrupt_identification_register = IIR_MODEM_STATUS_INTERRUPT;

        event_bitmask |= TR_HAL_UART_EVENT_HW_FLOW_CONTROL;
    }

    // *******************************
    // INTERRUPT: character timeout
    // when no characters have been written for a time exceeding the timeout
    // this usually means a transmission is done, so time to read bytes
    // clear this by reading from the RBR (receive_buffer_register)
    // *******************************
    if ((interrupt_register & IIR_RX_DATA_TIMEOUT_INTERRUPT) > 0)
    {
        // clear the interrupt
        uart_register_address->interrupt_identification_register = IIR_RX_DATA_TIMEOUT_INTERRUPT;

        // if the user wants the rx bytes sent to a function
        if (uart_settings->rx_handler_function != NULL)
        {
            // we can't just read a byte since we don't know if any more data is left

            // keep track of if we get any data
            uint32_t rx_byte_count = 0;

            // if any data is ready, read bytes and send until no more to read
            while ((uart_register_address->line_status_register) & LSR_DATA_READY)
            {
                uint32_t rx_data = uart_register_address->receive_buffer_register;
                uint8_t rx_byte = (rx_data & 0x000000FF);
                uart_settings->rx_handler_function(rx_byte);
                rx_byte_count++;
            }

            // send back an event based on if we found data or not
            if (rx_byte_count == 0)
            {
                event_bitmask |= TR_HAL_UART_EVENT_RX_ENDED_NO_DATA;
            }
            else
            {
                event_bitmask |= TR_HAL_UART_EVENT_RX_ENDED_TO_USER_FX;
            }
        }
        // this is when there is no user function
        else
        {
            // let the user know to read bytes
            event_bitmask |= TR_HAL_UART_EVENT_RX_MAYBE_READY;
        }
    }

    // *******************************
    // UNEXPECTED
    // *******************************
    if (event_bitmask == 0)
    {
        event_bitmask |= TR_HAL_UART_EVENT_UNEXPECTED;
    }

    // call the user event function, passing the event bitmask
    if (uart_settings->event_handler_fx != NULL)
    {
        uart_settings->event_handler_fx(event_bitmask);
    }
}


/// ***************************************************************************
/// common_event_handler
///
/// DEFAULT user function for handling the UART events
/// we expect some users to override this function call
/// ***************************************************************************
void common_event_handler(uint32_t event_bitmask)
{

    UART_REGISTERS_T* uart_register_address = tr_hal_uart_get_uart_register_address(0);

    if ((event_bitmask & TR_HAL_UART_EVENT_DMA_TX_COMPLETE) != 0)
    {
        // DMA transmit complete, the app is now clear to send again.
        // The interrupt has already been cleared
    }

    if ((event_bitmask & TR_HAL_UART_EVENT_DMA_RX_BUFFER_LOW) != 0)
    {
        // this means there are less than LOW_BYTES_BUFFER_THRESHHOLD
        // remaining in the DMA RX buffer. App should refresh the
        // buffer by calling: tr_hal_uart_dma_change_rx_buffer
        // (this function clears any remaining bytes out before
        // switching buffers so no worries of dropping bytes)
        //
        //status = tr_hal_uart_dma_change_rx_buffer( uart_id, new_receive_buffer, strlen(new_receive_buffer);
    }

    if ((event_bitmask & TR_HAL_UART_EVENT_DMA_RX_TO_USER_FX) != 0)
    {
        // DMA received bytes. These bytes have already been sent
        // back to the app in the user app callback (if there
        // is no callback then the event_bitmask would be
        // TR_HAL_UART_EVENT_DMA_RX_READY
        // nothing the app needs to do, this is inform only
    }

    if ((event_bitmask & TR_HAL_UART_EVENT_DMA_RX_READY) != 0)
    {
        // DMA receive got bytes. These bytes have already been sent
        // back to the app in the user app callback (if defined)
        // otherwise, this is a trigger for the app to read the DMA
        // RX buffer
        //
        // while ((uart_register_address->line_status_register) & LSR_DATA_READY)
        // {
        //    uint8_t rx_data = uart_register_address->receive_buffer_register;
        // }
    }

    if ((event_bitmask & TR_HAL_UART_EVENT_TX_COMPLETE) != 0)
    {
        // The transmit asked for by the user has been completed.
        // this lets the app know that it can transmit again
    }

    if ((event_bitmask & TR_HAL_UART_EVENT_TX_STILL_GOING) != 0)
    {
        // the FIFO is clear and the trident HAL code transmitted more saved
        // bytes that it was holding on to. Inform only.
    }

    if ((event_bitmask & TR_HAL_UART_EVENT_RX_TO_USER_FX) != 0)
    {
        // received bytes. These bytes have already been sent
        // back to the app in the user app callback (if there
        // is no callback then the event_bitmask would be TR_HAL_UART_EVENT_RX_READY
        // nothing the app needs to do, this is inform only
    }

    if ((event_bitmask & TR_HAL_UART_EVENT_RX_READY) != 0)
    {
        // received data: this means, in non-DMA RX mode, time to read data
        // from the RBR. even if one is not ready we need to do this to
        // clear the interrupt (note the added "unused" piece is to prevent
        // a compiler warning)
        uint32_t rx_data __attribute__((unused));
        rx_data = uart_register_address->receive_buffer_register;

        // read bytes and send until no more to read
        while ((uart_register_address->line_status_register) & LSR_DATA_READY)
        {
            rx_data = uart_register_address->receive_buffer_register;
        }
    }

    if ((event_bitmask & TR_HAL_UART_EVENT_RX_ENDED_NO_DATA) != 0)
    {
        // this means that due to a char timeout (probable end of transmission)
        // the RBR was read and no bytes were found waiting.
    }

    if ((event_bitmask & TR_HAL_UART_EVENT_RX_ENDED_TO_USER_FX) != 0)
    {
        // this means that due to a char timeout (probable end of transmission)
        // the RBR was read for any bytes waiting. These were passed to the
        // user receive function. inform only.
    }

    if ((event_bitmask & TR_HAL_UART_EVENT_RX_MAYBE_READY) != 0)
    {
        // this means that due to a char timeout (possible end of transmission)
        // the RBR should be read for any bytes waiting.
        // (note the added "unused" piece is to prevent a compiler warning)
        uint32_t rx_data  __attribute__((unused));
        rx_data = uart_register_address->receive_buffer_register;

        // read bytes and send until no more to read
        while ((uart_register_address->line_status_register) & LSR_DATA_READY)
        {
            rx_data = uart_register_address->receive_buffer_register;
        }
    }

    if ((event_bitmask & TR_HAL_UART_EVENT_RX_ERR_OVERRUN) != 0)
    {
        // overrun error
    }

    if ((event_bitmask & TR_HAL_UART_EVENT_RX_ERR_PARITY) != 0)
    {
        // parity error
    }

    if ((event_bitmask & TR_HAL_UART_EVENT_RX_ERR_FRAMING) != 0)
    {
        // framing error
    }

    if ((event_bitmask & TR_HAL_UART_EVENT_RX_ERR_BREAK) != 0)
    {
        // break error
    }

    if ((event_bitmask & TR_HAL_UART_EVENT_HW_FLOW_CONTROL) != 0)
    {
        // need to read MSR to clear the interrupt
        // (note the added "unused" piece is to prevent a compiler warning)
        uint32_t modem_status __attribute__((unused));
        modem_status = uart_register_address->modem_status_register;

        //if (modem_status & MSR_DCTS) {;}
        //if (modem_status & MSR_DDSR) {;}
        //if (modem_status & MSR_TERI) {;}
        //if (modem_status & MSR_DDCD) {;}
        //if (modem_status & MSR_CTS)  {;}
        //if (modem_status & MSR_DSR)  {;}
        //if (modem_status & MSR_RI)   {;}
        //if (modem_status & MSR_DCD)  {;}
    }

    if ((event_bitmask & TR_HAL_UART_EVENT_UNEXPECTED) != 0)
    {
        // we should read the interrupt_identification_register and figure out what happened
        uint32_t interrupt_register = uart_register_address->interrupt_identification_register;
        interrupt_register = interrupt_register & IIR_INTERRUPT_MASK;
    }
}


/// ***************************************************************************
/// hal_internal_handle_rx_dma_bytes
///
/// INTERNAL
/// we got an interrupt that DMA bytes are available
/// AND we have a user defined function
/// pass these to the user defined receive function
/// ***************************************************************************
static void hal_internal_handle_rx_dma_bytes(tr_hal_uart_id_t uart_id)
{
    // pass bytes from the DMA RX buffer to the user function
    tr_hal_uart_settings_t* uart_settings = &(g_current_uart_settings[uart_id]);

    // we expect that we have a RX data buffer AND a user defined function
    // if that is not true then we need to eject
    if (   (uart_settings->rx_dma_buffer == NULL)
        || (uart_settings->rx_handler_function == NULL))
    {
        return;
    }

    // get the buffer and length and current index
    uint8_t* rx_buffer = uart_settings->rx_dma_buffer;
    uint16_t buff_len = uart_settings->rx_dma_buff_length;
    uint16_t curr_index = g_curr_dma_rx_index[uart_id];

    for (uint16_t i = curr_index; i < buff_len; i++)
    {
        if (rx_buffer[i] == 0)
        {
            break;
        }
        else
        {
            uart_settings->rx_handler_function(rx_buffer[i]);
            g_curr_dma_rx_index[uart_id]++;
        }
    }
}


/// ***************************************************************************
/// tr_hal_get_baud_rate_enum_from_value
///
/// takes a baud rate as an int (like 115200) and returns an enum like
/// TR_HAL_UART_BAUD_RATE_xxx
/// ***************************************************************************
tr_hal_baud_rate_t tr_hal_get_baud_rate_enum_from_value(uint32_t baud_rate_value)
{
    switch (baud_rate_value)
    {
        case 2400:
            return TR_HAL_UART_BAUD_RATE_2400;
        case 4800:
            return TR_HAL_UART_BAUD_RATE_4800;
        case 9600:
            return TR_HAL_UART_BAUD_RATE_9600;
        case 14400:
            return TR_HAL_UART_BAUD_RATE_14400;
        case 19200:
            return TR_HAL_UART_BAUD_RATE_19200;
        case 28800:
            return TR_HAL_UART_BAUD_RATE_28800;
        case 38400:
            return TR_HAL_UART_BAUD_RATE_38400;
        case 57600:
            return TR_HAL_UART_BAUD_RATE_57600;
        case 76800:
            return TR_HAL_UART_BAUD_RATE_76800;
        case 115200:
            return TR_HAL_UART_BAUD_RATE_115200;
        case 230400:
            return TR_HAL_UART_BAUD_RATE_230400;
        case 500000:
            return TR_HAL_UART_BAUD_RATE_500000;
        case 1000000:
            return TR_HAL_UART_BAUD_RATE_1000000;
        case 2000000:
            return TR_HAL_UART_BAUD_RATE_2000000;
    }
    return TR_HAL_UART_BAUD_RATE_ERROR;
}


/// ***************************************************************************
/// tr_hal_get_data_bits_enum_from_value
///
/// takes a data bits value as an int (like 8) and returns an enum like
/// LCR_DATA_BITS_xxx_VALUE
/// ***************************************************************************
tr_hal_data_bits_t tr_hal_get_data_bits_enum_from_value(uint8_t data_bits_value)
{
    switch (data_bits_value)
    {
        case 5:
            return LCR_DATA_BITS_5_VALUE;
        case 6:
            return LCR_DATA_BITS_6_VALUE;
        case 7:
            return LCR_DATA_BITS_7_VALUE;
        case 8:
            return LCR_DATA_BITS_8_VALUE;
    }
    return LCR_DATA_BITS_INVALID_VALUE;
}


/// ***************************************************************************
/// tr_hal_get_stop_bits_enum_from_value
///
/// takes a stop bits value as an int (like 1) and returns an enum like
/// LCR_STOP_BITS_xxx_VALUE
/// ***************************************************************************
tr_hal_stop_bits_t tr_hal_get_stop_bits_enum_from_value(uint8_t stop_bits_value)
{
    switch (stop_bits_value)
    {
        case 1:
            return LCR_STOP_BITS_ONE_VALUE;
        case 2:
            return LCR_STOP_BITS_TWO_VALUE;
    }
    return LCR_STOP_BITS_INVALID_VALUE;
}


/// ****************************************************************************
/// tr_hal_uart_set_tx_rx_pins
/// ****************************************************************************
tr_hal_status_t tr_hal_uart_set_tx_rx_pins(tr_hal_uart_id_t  uart_id,
                                           tr_hal_gpio_pin_t tx_pin,
                                           tr_hal_gpio_pin_t rx_pin)
{
    tr_hal_status_t status;

    // make sure pins are valid
    if (!(tr_hal_uart_check_pins_valid(uart_id, tx_pin, rx_pin)))
    {
        return TR_HAL_ERROR_INVALID_PINS;
    }

    // ***************
    // UART 0
    // ***************
    if (uart_id == UART_0_ID)
    {
        status = tr_hal_gpio_set_mode(tx_pin, TR_HAL_GPIO_MODE_UART_0_TX);
        if (status != TR_HAL_SUCCESS)
        {
            return status;
        }

        status = tr_hal_gpio_set_mode(rx_pin, TR_HAL_GPIO_MODE_UART_0_RX);
        if (status != TR_HAL_SUCCESS)
        {
            // on failure of RX, set the TX pin back
            tr_hal_gpio_set_mode(tx_pin, TR_HAL_GPIO_MODE_GPIO);
            return status;
        }
    }

    // ***************
    // UART 1
    // ***************
    else if (uart_id == UART_1_ID)
    {
        status = tr_hal_gpio_set_mode(tx_pin, TR_HAL_GPIO_MODE_UART_1_TX);
        if (status != TR_HAL_SUCCESS)
        {
            return status;
        }

        status = tr_hal_gpio_set_mode(rx_pin, TR_HAL_GPIO_MODE_UART_1_RX);
        if (status != TR_HAL_SUCCESS)
        {
            // on failure of RX, set the TX pin back
            tr_hal_gpio_set_mode(tx_pin, TR_HAL_GPIO_MODE_GPIO);
            return status;
        }
    }

    // ***************
    // UART 2
    // ***************
    else if (uart_id == UART_2_ID)
    {
        status = tr_hal_gpio_set_mode(tx_pin, TR_HAL_GPIO_MODE_UART_2_TX);
        if (status != TR_HAL_SUCCESS)
        {
            return status;
        }

        status = tr_hal_gpio_set_mode(rx_pin, TR_HAL_GPIO_MODE_UART_2_RX);
        if (status != TR_HAL_SUCCESS)
        {
            // on failure of RX, set the TX pin back
            tr_hal_gpio_set_mode(tx_pin, TR_HAL_GPIO_MODE_GPIO);
            return status;
        }
    }

    // reserve these GPIOs with the GPIO Manager
    tr_hal_gpio_mgr_reserve_gpio(tx_pin,
                                 TR_HAL_GPIO_SET_FOR_UART);
    tr_hal_gpio_mgr_reserve_gpio(rx_pin,
                                 TR_HAL_GPIO_SET_FOR_UART);

    return TR_HAL_SUCCESS;
}


/// ****************************************************************************
/// tr_hal_uart_set_rts_cts_pins
/// ****************************************************************************
tr_hal_status_t tr_hal_uart_set_rts_cts_pins(tr_hal_uart_id_t  uart_id,
                                             tr_hal_gpio_pin_t rts_pin,
                                             tr_hal_gpio_pin_t cts_pin)
{
    tr_hal_status_t status;

    // UART 0 can't do RTS CTS
    if (uart_id == UART_0_ID)
    {
        return TR_HAL_ERROR_HWFC_NOT_VALID;
    }

    // make sure pins are valid
    if (tr_hal_uart_check_pins_valid(uart_id, rts_pin, cts_pin))
    {
        return TR_HAL_ERROR_INVALID_PINS;
    }

    // check if RTS is already configured to another peripheral
    status = tr_hal_gpio_mgr_check_gpio(rts_pin);
    if (status != TR_HAL_SUCCESS)
    {
        return status;
    }

    // check if CTS is already configured to another peripheral
    status = tr_hal_gpio_mgr_check_gpio(cts_pin);
    if (status != TR_HAL_SUCCESS)
    {
        return status;
    }

    // ***************
    // UART 1
    // ***************
    if (uart_id == UART_1_ID)
    {
        status = tr_hal_gpio_set_mode(rts_pin, TR_HAL_GPIO_MODE_UART_1_RTSN);
        if (status != TR_HAL_SUCCESS)
        {
            return status;
        }

        status = tr_hal_gpio_set_mode(cts_pin, TR_HAL_GPIO_MODE_UART_1_CTS);
        if (status != TR_HAL_SUCCESS)
        {
            // on failure of CTS, set the RTS pin back
            tr_hal_gpio_set_mode(rts_pin, TR_HAL_GPIO_MODE_GPIO);
            return status;
        }
    }

    // ***************
    // UART 2
    // ***************
    else if (uart_id == UART_2_ID)
    {
        status = tr_hal_gpio_set_mode(rts_pin, TR_HAL_GPIO_MODE_UART_2_RTSN);
        if (status != TR_HAL_SUCCESS)
        {
            return status;
        }

        status = tr_hal_gpio_set_mode(cts_pin, TR_HAL_GPIO_MODE_UART_2_CTS);
        if (status != TR_HAL_SUCCESS)
        {
            // on failure of CTS, set the RTS pin back
            tr_hal_gpio_set_mode(rts_pin, TR_HAL_GPIO_MODE_GPIO);
            return status;
        }
    }

    // reserve these GPIOs with the GPIO Manager
    tr_hal_gpio_mgr_reserve_gpio(rts_pin,
                                 TR_HAL_GPIO_SET_FOR_UART);
    tr_hal_gpio_mgr_reserve_gpio(cts_pin,
                                 TR_HAL_GPIO_SET_FOR_UART);

    return TR_HAL_SUCCESS;
}



/// ****************************************************************************
/// tr_hal_uart_check_pins_valid
///
/// checks if a TX, RX combination of pins is valid
/// ****************************************************************************
bool tr_hal_uart_check_pins_valid(tr_hal_uart_id_t  uart_id,
                                  tr_hal_gpio_pin_t tx_pin,
                                  tr_hal_gpio_pin_t rx_pin)
{
    (void)uart_id;

    // check that TX pin is valid
    if (tr_hal_gpio_is_available(tx_pin) == false)
    {
        return false;
    }

    // check that RX pin is valid
    if (tr_hal_gpio_is_available(rx_pin) == false)
    {
        return false;
    }

    // TX and RX can't be the same pin
    if (tx_pin.pin == rx_pin.pin)
    {
        return false;
    }

    // otherwise the pins are OK
    return true;
}

void UART0_Handler(void)
{
    tr_hal_uart_internal_interrupt_handler(UART_0_ID);
}

void UART1_Handler(void)
{
    tr_hal_uart_internal_interrupt_handler(UART_1_ID);
}

void UART2_Handler(void)
{
    tr_hal_uart_internal_interrupt_handler(UART_2_ID);
}
