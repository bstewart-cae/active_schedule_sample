/// ****************************************************************************
/// @file tr_hal_i2c.c
///
/// @brief This contains the code for the Trident HAL I2C for T32CZ20
///
///
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************

// string.h is for memcpy
#include <string.h>
#include "tr_hal_i2c.h"

// keep track of what the user passed in
static tr_hal_i2c_receive_callback_t i2c_rx_handler_function[NUM_I2C_CONTROLLER];
static tr_hal_i2c_event_callback_t   i2c_event_handler_fx[NUM_I2C_CONTROLLER];

// keep track of if the I2C has been initialized and keep track of the
// settings passed in, so we can copy them out on read
static bool g_i2c_init_completed[NUM_I2C_CONTROLLER] = {false, false};
static      tr_hal_i2c_settings_t g_current_i2c_settings[NUM_I2C_CONTROLLER];

// count we use to timeout a check on the TX or RX being ready
#define I2C_TIMEOUT_COUNT 1600000

static bool g_interrupt_rx_finish_flag = false;
static bool g_interrupt_tx_empty_flag = false;
static bool g_interrupt_rx_under_flag = false;

// interrupt counters
tr_hal_i2c_int_count_t g_int_count[NUM_I2C_CONTROLLER];
static void clear_interrupt_counters(tr_hal_i2c_id_t i2c_id);


// ****************************************************************************
// tr_hal_i2c_ctrl_get_register_address
// ****************************************************************************
I2C_REGISTERS_T* tr_hal_i2c_get_controller_register_address(tr_hal_i2c_id_t i2c_id)
{
    if      (i2c_id == I2C_CTRL_0_ID) { return I2C0_CHIP_REGISTERS;}
    else if (i2c_id == I2C_CTRL_1_ID) { return I2C1_CHIP_REGISTERS;}

    return NULL;
}


// ****************************************************************************
// static set_i2c_pins
// ****************************************************************************
static tr_hal_status_t set_i2c_pins(tr_hal_i2c_id_t   i2c_id,
                                    tr_hal_gpio_pin_t sda_pin,
                                    tr_hal_gpio_pin_t scl_pin)
{
    tr_hal_status_t status;

    // check if SDA pin is already configured to another peripheral
    status = tr_hal_gpio_mgr_check_gpio(sda_pin);
    if (status != TR_HAL_SUCCESS)
    {
        return status;
    }

    // check if SCL pin is already configured to another peripheral
    status = tr_hal_gpio_mgr_check_gpio(scl_pin);
    if (status != TR_HAL_SUCCESS)
    {
        return status;
    }

    // set the pins for Controller 0
    if (i2c_id == I2C_CTRL_0_ID)
    {
        // set SDA
        status = tr_hal_gpio_set_mode(sda_pin,
                                      TR_HAL_GPIO_MODE_I2C_0_MASTER_SDA);
        if (status != TR_HAL_SUCCESS)
        {
            return status;
        }
        // set SCL
        status = tr_hal_gpio_set_mode(scl_pin,
                                      TR_HAL_GPIO_MODE_I2C_0_MASTER_SCL);
        if (status != TR_HAL_SUCCESS)
        {
            // on set SCL error, set SDA back to GPIO mode
            tr_hal_gpio_set_mode(sda_pin,
                                 TR_HAL_GPIO_MODE_GPIO);
            return status;
        }

        // tell the GPIO manager that these 2 GPIOs are in use
        tr_hal_gpio_mgr_reserve_gpio(sda_pin, TR_HAL_GPIO_SET_FOR_I2C);
        tr_hal_gpio_mgr_reserve_gpio(scl_pin, TR_HAL_GPIO_SET_FOR_I2C);

        // all done
        return TR_HAL_SUCCESS;
    }

    // set the pins for Controller 1
    else if (i2c_id == I2C_CTRL_1_ID)
    {
        // set SDA
        status = tr_hal_gpio_set_mode(sda_pin,
                                      TR_HAL_GPIO_MODE_I2C_1_MASTER_SDA);
        if (status != TR_HAL_SUCCESS)
        {
            return status;
        }
        // set SCL
        status = tr_hal_gpio_set_mode(scl_pin,
                                      TR_HAL_GPIO_MODE_I2C_1_MASTER_SCL);
        if (status != TR_HAL_SUCCESS)
        {
            // on set SCL error, set SDA back to GPIO mode
            tr_hal_gpio_set_mode(sda_pin,
                                 TR_HAL_GPIO_MODE_GPIO);
            return status;
        }

        // tell the GPIO manager that these 2 GPIOs are in use
        tr_hal_gpio_mgr_reserve_gpio(sda_pin, TR_HAL_GPIO_SET_FOR_I2C);
        tr_hal_gpio_mgr_reserve_gpio(scl_pin, TR_HAL_GPIO_SET_FOR_I2C);

        // all done
        return TR_HAL_SUCCESS;
    }

    // if we get here then this is an invalid I2C ID
    return TR_HAL_INVALID_I2C_ID;
}


// ****************************************************************************
// tr_hal_i2c_init_controller
// ****************************************************************************
tr_hal_status_t tr_hal_i2c_init(tr_hal_i2c_id_t        i2c_id,
                                tr_hal_i2c_settings_t* i2c_settings)
{
    tr_hal_status_t status;

    // make sure SPI ID is within range
    if (i2c_id > TR_HAL_MAX_I2C_CONTROLLER_ID)
    {
        return TR_HAL_INVALID_I2C_ID;
    }

    // settings can't be NULL
    if ( i2c_settings == NULL)
    {
        return TR_HAL_I2C_NULL_SETTINGS;
    }

    // set the pins
    // this also checks the pins for validity
    status = set_i2c_pins(i2c_id,
                          i2c_settings->sda_pin,
                          i2c_settings->scl_pin);
    if (status != TR_HAL_SUCCESS)
    {
        return status;
    }

    // valid settings, so get the i2c register address
    I2C_REGISTERS_T* i2c_register_address = tr_hal_i2c_get_controller_register_address(i2c_id);

    // set control register for this to be disabled
    i2c_register_address->control = 0;

    // target address to send to gets set in the transmit fx
    // i2c_register_address->target = 0;

    // data to send gets set in the transmit fx
    // i2c_register_address->buffer = 0;

    // ignoring wake on interrupt field for now
    // i2c_settings->wake_on_interrupt

    // enable interrupts
    if (i2c_settings->enable_chip_interrupts)
    {
        // enable NVIC interrupts and priority based on I2C ID
        if (i2c_id == I2C_CTRL_0_ID)
        {
            NVIC_SetPriority(I2C_Master0_IRQn, i2c_settings->interrupt_priority);
            NVIC_EnableIRQ(I2C_Master0_IRQn);
        }
        else if (i2c_id == I2C_CTRL_1_ID)
        {
            NVIC_SetPriority(I2C_Master1_IRQn, i2c_settings->interrupt_priority);
            NVIC_EnableIRQ(I2C_Master1_IRQn);
        }
        // enable interrupts in the chip register
        i2c_register_address->interrupt_enable = I2C_INTERRUPT_ALL;
    }

    // set the clock
    i2c_register_address->clock_divider = i2c_settings->clock_setting;

    // save the user provided information
    i2c_rx_handler_function[i2c_id] = i2c_settings->rx_handler_function;
    i2c_event_handler_fx[i2c_id]    = i2c_settings->event_handler_fx;

    // init has been completed for this UART, so copy in the settings
    g_i2c_init_completed[i2c_id] = true;
    memcpy(&(g_current_i2c_settings[i2c_id]), i2c_settings, sizeof(tr_hal_i2c_settings_t));

    clear_interrupt_counters(i2c_id);

    return TR_HAL_SUCCESS;
}


// ****************************************************************************
// tr_hal_i2c_uninit_controller
// ****************************************************************************
tr_hal_status_t tr_hal_i2c_uninit(tr_hal_i2c_id_t i2c_id)
{
    // make sure SPI ID is within range
    if (i2c_id > TR_HAL_MAX_I2C_CONTROLLER_ID)
    {
        return TR_HAL_INVALID_I2C_ID;
    }

    // get the i2c register address
    I2C_REGISTERS_T* i2c_register_address = tr_hal_i2c_get_controller_register_address(i2c_id);
    tr_hal_i2c_settings_t* settings = &(g_current_i2c_settings[i2c_id]);

    // set control register for this to be disabled
    i2c_register_address->control = 0;

    // disable all interrupts
    i2c_register_address->interrupt_enable = 0;

    // set the pins back to just GPIO
    tr_hal_gpio_set_mode(settings->sda_pin,
                         TR_HAL_GPIO_MODE_GPIO);
    tr_hal_gpio_set_mode(settings->scl_pin,
                         TR_HAL_GPIO_MODE_GPIO);

    // release the GPIOs used
    tr_hal_gpio_mgr_release_gpio(settings->sda_pin);
    tr_hal_gpio_mgr_release_gpio(settings->scl_pin);

    // clear the NVIC interrupts
    if (i2c_id == I2C_CTRL_0_ID)
    {
        NVIC_ClearPendingIRQ(I2C_Master0_IRQn);
        NVIC_DisableIRQ(I2C_Master0_IRQn);
    }
    else if (i2c_id == I2C_CTRL_1_ID)
    {
        NVIC_ClearPendingIRQ(I2C_Master1_IRQn);
        NVIC_DisableIRQ(I2C_Master1_IRQn);
    }

    // this I2C is no longer valid
    g_i2c_init_completed[i2c_id] = false;

    // clear out the global saved data
    i2c_rx_handler_function[i2c_id] = NULL;
    i2c_event_handler_fx[i2c_id]    = NULL;

    return TR_HAL_SUCCESS;
}


// ****************************************************************************
// tr_hal_i2c_settings_read
// ****************************************************************************
tr_hal_status_t tr_hal_i2c_read_settings(tr_hal_i2c_id_t        i2c_id,
                                         tr_hal_i2c_settings_t* i2c_settings)
{
    // make sure SPI ID is within range
    if (i2c_id > TR_HAL_MAX_I2C_CONTROLLER_ID)
    {
        return TR_HAL_INVALID_I2C_ID;
    }

    // if the I2C in question has not been initialized then return error
    if (g_i2c_init_completed[i2c_id] == false)
    {
        return TR_HAL_ERROR_NOT_INITIALIZED;
    }

    // settings can't be NULL
    if ( i2c_settings == NULL)
    {
        return TR_HAL_I2C_NULL_SETTINGS;
    }

    // copy in the saved settings
    memcpy(i2c_settings, &(g_current_i2c_settings[i2c_id]), sizeof(tr_hal_i2c_settings_t));

    // success
    return TR_HAL_SUCCESS;
}


// ****************************************************************************
// tr_hal_i2c_tx_rx
//
// @param target_address - the addres of the Target device to send to
// @param bytes_to_send - the byte array of bytes to be written
// @param num_bytes_to_send - the number of bytes to write to the Target
// @param num_bytes_to_read - the number of bytes to read from the Target
// ****************************************************************************
tr_hal_status_t tr_hal_i2c_tx_rx(tr_hal_i2c_id_t i2c_id,
                                 uint8_t         target_address,
                                 uint8_t*        bytes_to_send,
                                 uint16_t        num_bytes_to_send,
                                 uint16_t        num_bytes_to_read)
{
    uint8_t index;

    // for making sure we get out of the loops that check if we can send and receive
    // for instance, if the addr is bad then these will hang forever unless we have
    // a way out
    uint32_t timeout_counter = 0;

    // check ID
    if (i2c_id > TR_HAL_MAX_I2C_CONTROLLER_ID)
    {
        return TR_HAL_INVALID_I2C_ID;
    }

    // make sure bytes to send is not too large
    if (num_bytes_to_send > I2C_TX_FIFO_BYTES)
    {
        return TR_HAL_I2C_WRITE_BYTES_TOO_LARGE;
    }

    // make sure bytes to read is not too large
    if (num_bytes_to_read > I2C_RX_FIFO_BYTES)
    {
        return TR_HAL_I2C_READ_BYTES_TOO_LARGE;
    }

    // reset any INT flags
    g_interrupt_rx_finish_flag = false;
    g_interrupt_tx_empty_flag = false;
    g_interrupt_rx_under_flag = false;

    // valid id, so get the i2c register address
    I2C_REGISTERS_T* i2c_register_address = tr_hal_i2c_get_controller_register_address(i2c_id);
    tr_hal_i2c_settings_t* i2c_settings = &(g_current_i2c_settings[i2c_id]);

    // disable the I2C using the control register
    i2c_register_address->control = I2C_CONTROL_DISABLE_CONTROLLER;

    // set the clock divider
    i2c_register_address->clock_divider = i2c_settings->clock_setting;

    // target address to send to gets set in the transmit fx
    i2c_register_address->target = target_address;

    // we DON'T enable I2C here, we wait until later

    // write the bytes we need to write
    for (index = 0; index < num_bytes_to_send; index++)
    {
        i2c_register_address->buffer = I2C_BUFFER_SET_FOR_WRITE | bytes_to_send[index];
    }

    // send the bytes that allow us to read the bytes we need to read
    for (index = 0; index < num_bytes_to_read; index++)
    {
        i2c_register_address->buffer = I2C_BUFFER_SET_FOR_READ;
    }

    // send STOP as we are done writing
    // NOTE: this does NOT work correctly, the stop is NOT sent
    // in order to send the STOP there needs to be a small delay and then set
    // the control register to ENABLE|STOP|RESTART
    i2c_register_address->control = I2C_CONTROL_ENABLE_CONTROLLER | I2C_CONTROL_STOP_TRANSACTION;

    // this is the delay
    uint32_t counter = 0;
    for (uint32_t i=0; i<500; i++)
    {
        counter++;
    }

    // set control to ENABLE|STOP|RESTART
    // WITHOUT THIS STEP THE I2C DOES NOT SEND A STOP and commands do not work properly
    i2c_register_address->control = I2C_CONTROL_ENABLE_CONTROLLER | I2C_CONTROL_STOP_TRANSACTION | I2C_CONTROL_ENABLE_RESTART;

    // wait for bytes to be ready to read - this means TX done
    if (num_bytes_to_read > 0)
    {
        // wait until the FIFO is in READ FINISH
        timeout_counter = 0;
        while ((i2c_register_address->interrupt_raw_status & I2C_INTERRUPT_TX_EMPTY) == 0)
        {
            timeout_counter++;
            if (timeout_counter > 32000)
            {
                break;
            }
        }
    }

    // read data - this should be the same number of bytes that we wrote
    for (index = 0; index < num_bytes_to_read; index++)
    {
        uint32_t data = i2c_register_address->buffer;
        uint8_t data8b = data & 0xFF;
        // make sure we didn't RX UNDER - which means there was no byte to read
        // if ((i2c_register_address->interrupt_raw_status & I2C_INTERRUPT_RX_UNDER) > 0)
        // otherwise return the byte to the app
        if (i2c_rx_handler_function[i2c_id] != NULL)
        {
            i2c_rx_handler_function[i2c_id](data8b);
        }
    }

    return TR_HAL_SUCCESS;
}


// ****************************************************************************
// INTERNAL I2C interrupt handler
// ****************************************************************************
static void i2c_internal_controller_interrupt_handler(tr_hal_i2c_id_t i2c_id)
{
    uint32_t event_bitmask = 0;
    // valid id, so get the i2c register address
    I2C_REGISTERS_T* i2c_register_address = tr_hal_i2c_get_controller_register_address(i2c_id);

    uint32_t int_status = i2c_register_address->interrupt_status;

    i2c_register_address->interrupt_clear = (int_status & I2C_INTERRUPT_ALL);

    // rx under
    if ((int_status & I2C_INTERRUPT_RX_UNDER) > 0)
    {
        event_bitmask |= I2C_EVENT_RX_UNDER;
        g_interrupt_rx_under_flag = true;
        g_int_count[i2c_id].count_rx_under++;
    }

    // rx over
    if ((int_status & I2C_INTERRUPT_RX_OVER) > 0)
    {
        event_bitmask |= I2C_EVENT_RX_OVER;
        g_int_count[i2c_id].count_rx_over++;
    }

    // rx full
    if ((int_status & I2C_INTERRUPT_RX_FULL) > 0)
    {
        event_bitmask |= I2C_EVENT_RX_FULL;
        g_int_count[i2c_id].count_rx_full++;
    }

    // rx finish
    if ((int_status & I2C_INTERRUPT_RX_FINISH) > 0)
    {
        event_bitmask |= I2C_EVENT_RX_FINISH;
        g_interrupt_rx_finish_flag = true;
        g_int_count[i2c_id].count_rx_finish++;
    }

    // tx over
    if ((int_status & I2C_INTERRUPT_TX_OVER) > 0)
    {
        event_bitmask |= I2C_EVENT_TX_OVER;
        g_int_count[i2c_id].count_tx_over++;
    }

    // tx empty
    if ((int_status & I2C_INTERRUPT_TX_EMPTY) > 0)
    {
        event_bitmask |= I2C_EVENT_TX_EMPTY;
        g_interrupt_tx_empty_flag = true;
        g_int_count[i2c_id].count_tx_empty++;
    }

    // abort A nack
    if ((int_status & I2C_INTERRUPT_ABORT_A_NACK) > 0)
    {
        event_bitmask |= I2C_EVENT_ABORT_A_NACK;
        g_int_count[i2c_id].count_abort_a_nack++;
    }

    // abort W nack
    if ((int_status & I2C_INTERRUPT_ABORT_W_NACK) > 0)
    {
        event_bitmask |= I2C_EVENT_ABORT_W_NACK;
        g_int_count[i2c_id].count_abort_w_nack++;
    }

    // abort lost arbitration
    if ((int_status & I2C_INTERRUPT_ABORT_LOST_ARB) > 0)
    {
        event_bitmask |= I2C_EVENT_ABORT_LOST_ARB;
        g_int_count[i2c_id].count_abort_lost_arb++;
    }

    // idle
    if ((int_status & I2C_INTERRUPT_IDLE_STATE) > 0)
    {
        //event_bitmask |= ???;
        g_int_count[i2c_id].count_idle++;
    }

    // ****************************************
    // call the user event function, passing the event bitmask
    // ****************************************
    if (i2c_event_handler_fx[i2c_id] != NULL)
    {
        i2c_event_handler_fx[i2c_id](i2c_id, event_bitmask);
    }

}

// ****************************************************************************
// clear interrupt counters
// ****************************************************************************
static void clear_interrupt_counters(tr_hal_i2c_id_t i2c_id)
{
    g_int_count[i2c_id].count_rx_under = 0;
    g_int_count[i2c_id].count_rx_over = 0;
    g_int_count[i2c_id].count_rx_full = 0;
    g_int_count[i2c_id].count_rx_finish = 0;
    g_int_count[i2c_id].count_tx_over = 0;
    g_int_count[i2c_id].count_tx_empty = 0;
    g_int_count[i2c_id].count_abort_a_nack = 0;
    g_int_count[i2c_id].count_abort_w_nack = 0;
    g_int_count[i2c_id].count_abort_lost_arb = 0;
    g_int_count[i2c_id].count_idle = 0;
}

// ****************************************************************************
// tr_hal_i2c_read_interrupt_counters
// ****************************************************************************
tr_hal_status_t tr_hal_i2c_read_interrupt_counters(tr_hal_i2c_id_t         i2c_id,
                                                   tr_hal_i2c_int_count_t* int_count)
{
    // check ID
    if (i2c_id > TR_HAL_MAX_I2C_CONTROLLER_ID)
    {
        return TR_HAL_INVALID_I2C_ID;
    }

    // counter struct can't be NULL
    if (int_count == NULL)
    {
        return TR_HAL_I2C_NULL_SETTINGS;
    }

    // copy in the current counters to the result
    memcpy(int_count, &(g_int_count[i2c_id]), sizeof(tr_hal_i2c_int_count_t) );

    return TR_HAL_SUCCESS;
}


// ****************************************************************************
// NVIC interrupt call I2C0
// ****************************************************************************
void I2C_Master0_Handler(void)
{
    i2c_internal_controller_interrupt_handler(I2C_CTRL_0_ID);
}

// ****************************************************************************
// NVIC interrupt call I2C1
// ****************************************************************************
void I2C_Master1_Handler(void)
{
    i2c_internal_controller_interrupt_handler(I2C_CTRL_1_ID);
}


