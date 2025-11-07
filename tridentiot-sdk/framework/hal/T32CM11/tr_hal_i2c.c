/// ****************************************************************************
/// @file tr_hal_i2c.c
///
/// @brief This contains the code for the Trident HAL I2C for T32CM11
///
///
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************

// string.h is for memcpy
#include <string.h>
#include "tr_hal_i2c.h"

// remember the settings from the user
tr_hal_i2c_settings_t g_current_i2c_settings;
bool g_i2c_init_completed = false;

// interrupt counters
tr_hal_i2c_int_count_t g_int_count;

// count we use to timeout a check on the TX or RX being ready
#define I2C_TIMEOUT_COUNT 1600000

#define NUM_CLOCK_PULSES_TO_RESET_BUS 9


// ****************************************************************************
// tr_hal_i2c_ctrl_get_register_address
// ****************************************************************************
I2C_REGISTERS_T* tr_hal_i2c_get_controller_register_address(tr_hal_i2c_id_t i2c_id __attribute__((unused)))
{
    return I2C_CHIP_REGISTERS;
}


// ****************************************************************************
// set_i2c_pins
// ****************************************************************************
static tr_hal_status_t set_i2c_pins(tr_hal_gpio_pin_t sda_pin,
                                    tr_hal_gpio_pin_t scl_pin)
{
    tr_hal_status_t status;

    // SDA = Data
    if (   (sda_pin.pin != I2C_SDA_PIN_OPTION1)
        && (sda_pin.pin != I2C_SDA_PIN_OPTION2) )
    {
        return TR_HAL_I2C_INVALID_SDA_PIN;
    }

    // SCL = clock
    if (   (scl_pin.pin != I2C_SCL_PIN_OPTION1)
        && (scl_pin.pin != I2C_SCL_PIN_OPTION2) )
    {
        return TR_HAL_I2C_INVALID_SCL_PIN;
    }


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

    tr_hal_gpio_set_mode(sda_pin,
                         TR_HAL_GPIO_MODE_I2C);
    tr_hal_gpio_set_mode(scl_pin,
                         TR_HAL_GPIO_MODE_I2C);

    // tell the GPIO manager that these 2 GPIOs are in use
    tr_hal_gpio_mgr_reserve_gpio(sda_pin, TR_HAL_GPIO_SET_FOR_I2C);
    tr_hal_gpio_mgr_reserve_gpio(scl_pin, TR_HAL_GPIO_SET_FOR_I2C);

    return TR_HAL_SUCCESS;
}


// ****************************************************************************
// reset_i2c_bus
//
// This function resets the I2C bus. The easiest way to cause a bus lockup
// is to reset the I2C Controller part way through a read transfer. The I2C
// target is driving data onto the bus and won't let go until the data is
// clocked out. So all I2C controller chips should start by resetting the bus.
//
// In the I2C guide from NXP section 3.1.16 Bus Clear: it says to use device
// reset pins in the event that the bus is stuck. Since the whole point of
// I2C is to minimize the number of bus pins, many devices don't have reset
// pins. Alternatively, If the data line (SDA) is stuck LOW, the Controller
// should send nine clock pulses. The device that held the bus LOW should
// release it sometime within those nine clocks
// https://www.nxp.com/docs/en/user-guide/UM10204.pdf
//
// this function does the 9 clock pulses
// ****************************************************************************
tr_hal_status_t reset_i2c_bus(void)
{
    // we are not checking if pins are valid since we have already
    // done that at init, we just make sure we are initialized
    if (g_i2c_init_completed == false)
    {
        return TR_HAL_ERROR_NOT_INITIALIZED;
    }

    // get the register address
    I2C_REGISTERS_T* i2c_register_address = tr_hal_i2c_get_controller_register_address(I2C_CTRL_0_ID);

    // get the settings
    tr_hal_i2c_settings_t* settings = &(g_current_i2c_settings);

    // read the pins
    tr_hal_gpio_pin_t sda_pin = settings->sda_pin;
    tr_hal_gpio_pin_t scl_pin = settings->scl_pin;

    // variable for counting pulses sent
    uint32_t pulse = 0;

    // clear I2C FIFOs
    i2c_register_address->status = I2C_STATUS_CLEAR_FIFO;

    // wait for the FIFO to clear
    uint32_t timeout_counter = 0;
    while ((i2c_register_address->status & I2C_STATUS_CLEAR_FIFO) == 0)
    {
        timeout_counter++;
        if (timeout_counter > I2C_TIMEOUT_COUNT)
        {
            return TR_HAL_I2C_WRITE_TIMEOUT_ERROR;
        }
    }

    // disable I2C so we don't use the pins
    i2c_register_address->control = I2C_CONTROL_REG_I2C_DISABLE;

    // turn off I2C interrupts
    NVIC_ClearPendingIRQ(I2cm_IRQn);
    NVIC_DisableIRQ(I2cm_IRQn);

    // set the pins back to GPIO so we can bit bang the 9 pulses
    tr_hal_gpio_set_mode(sda_pin,
                         TR_HAL_GPIO_MODE_GPIO);
    tr_hal_gpio_set_mode(scl_pin,
                         TR_HAL_GPIO_MODE_GPIO);

    // set pins for output
    tr_hal_gpio_set_direction(sda_pin,
                              TR_HAL_GPIO_DIRECTION_OUTPUT);
    tr_hal_gpio_set_direction(scl_pin,
                              TR_HAL_GPIO_DIRECTION_OUTPUT);
    // set pins high
    tr_hal_gpio_set_output(sda_pin,
                           TR_HAL_GPIO_LEVEL_HIGH);
    tr_hal_gpio_set_output(scl_pin,
                           TR_HAL_GPIO_LEVEL_HIGH);

    // run the delay
    __NOP();
    __NOP();
    __NOP();
    __NOP();

    // run the 9 pulses
    for (pulse = 0; pulse < NUM_CLOCK_PULSES_TO_RESET_BUS; pulse++)
    {
        tr_hal_gpio_set_output(scl_pin,
                               TR_HAL_GPIO_LEVEL_LOW);

        // run the delay
        __NOP();
        __NOP();
        __NOP();
        __NOP();

        tr_hal_gpio_set_output(scl_pin,
                               TR_HAL_GPIO_LEVEL_HIGH);

        // run the delay
        __NOP();
        __NOP();
        __NOP();
        __NOP();
    }

    // set the pins for I2C
    tr_hal_gpio_set_mode(sda_pin,
                         TR_HAL_GPIO_MODE_I2C);
    tr_hal_gpio_set_mode(scl_pin,
                         TR_HAL_GPIO_MODE_I2C);

    // do we need this?
    //enable_pin_opendrain(SCL_pin);
    //enable_pin_opendrain(SDA_pin);

    return TR_HAL_SUCCESS;
}


// ****************************************************************************
// for testing
// ****************************************************************************
tr_hal_status_t tr_hal_test_i2c_bus_reset(void)
{
    tr_hal_status_t status = reset_i2c_bus();

    return status;
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
    status = set_i2c_pins(i2c_settings->sda_pin,
                          i2c_settings->scl_pin);
    if (status != TR_HAL_SUCCESS)
    {
        return status;
    }

    // get the register address
    I2C_REGISTERS_T* i2c_register_address = tr_hal_i2c_get_controller_register_address(i2c_id);

    // start with I2C disabled, while we set it up
    i2c_register_address->control = I2C_CONTROL_REG_I2C_DISABLE;

    // write to the status register to clear the FIFOs
    i2c_register_address->status = I2C_STATUS_CLEAR_FIFO;

    // target address to send to gets set in the transmit fx (i2c_register_address->target)
    // data to send gets set in the transmit fx (i2c_register_address->buffer)
    // ignoring wake on interrupt field for now (i2c_settings->wake_on_interrupt)

    // enable interrupts
    if (i2c_settings->enable_chip_interrupts)
    {
        NVIC_SetPriority(I2cm_IRQn, i2c_settings->interrupt_priority);
        NVIC_EnableIRQ(I2cm_IRQn);
        // enable interrupts in the chip register
        i2c_register_address->interrupt_enable = I2C_INTERRUPT_ALL;
    }

    // the clock setting is split across 2 registers, lower 6 bits
    // in control reg and upper 8 bits in prescale reg
    uint32_t clock_setting = i2c_settings->clock_setting;
    uint32_t clock_lower_six_bits = clock_setting & CLOCK_LOWER_SIX_BITS;
    uint32_t clock_upper_eight_bits = clock_setting >> 6;

    // set the control and prescale registers which includes setting the CLOCK
    // the CLOCK SOURCE and the I2C ENABLE. we do not enable I2C here, we
    // wait until we are going to send to enable it
    i2c_register_address->control = clock_lower_six_bits | I2C_CONTROL_REG_CLOCK_SOURCE_EXT;
    i2c_register_address->prescale = clock_upper_eight_bits;

    // init has been completed for the I2C, so copy in the settings
    g_i2c_init_completed = true;
    memcpy(&(g_current_i2c_settings), i2c_settings, sizeof(tr_hal_i2c_settings_t));

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
    tr_hal_i2c_settings_t* settings = &(g_current_i2c_settings);

    // set control register for this to be disabled
    i2c_register_address->control = I2C_CONTROL_REG_I2C_DISABLE;

    // write to the status register to clear the FIFOs
    i2c_register_address->status = I2C_STATUS_CLEAR_FIFO;

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
    NVIC_ClearPendingIRQ(I2cm_IRQn);
    NVIC_DisableIRQ(I2cm_IRQn);

    // this I2C is no longer valid
    g_i2c_init_completed = false;

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
    if (g_i2c_init_completed == false)
    {
        return TR_HAL_ERROR_NOT_INITIALIZED;
    }

    // settings can't be NULL
    if ( i2c_settings == NULL)
    {
        return TR_HAL_I2C_NULL_SETTINGS;
    }

    // copy in the saved settings
    memcpy(i2c_settings, &(g_current_i2c_settings), sizeof(tr_hal_i2c_settings_t));

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
    (void)i2c_id;

    uint32_t index;
    uint32_t timeout_counter = 0;

    // if the I2C in question has not been initialized then return error
    if (g_i2c_init_completed == false)
    {
        return TR_HAL_ERROR_NOT_INITIALIZED;
    }

    // get the i2c register address
    I2C_REGISTERS_T* i2c_register_address = tr_hal_i2c_get_controller_register_address(I2C_CTRL_0_ID);

    // ENABLE the I2C
    i2c_register_address->control |= I2C_CONTROL_REG_I2C_ENABLE;

    // are we transmitting
    if (num_bytes_to_send > 0)
    {
        // send START condition
        i2c_register_address->command = I2C_COMMAND_START;

        // write the target address and the write bit
        i2c_register_address->command = I2C_COMMAND_WRITE_DATA_8;
        i2c_register_address->command = (target_address << 1) | I2C_WRITE_DATA_BIT;

        // verify Target Ack
        i2c_register_address->command = I2C_COMMAND_VACK;

        // send the bytes
        for (index = 0; index < num_bytes_to_send; index++)
        {
            i2c_register_address->command = I2C_COMMAND_WRITE_DATA_8;
            i2c_register_address->command = bytes_to_send[index];
            i2c_register_address->command = I2C_COMMAND_VACK;
        }
        // send STOP at the end
        i2c_register_address->command = I2C_COMMAND_STOP;

        // wait until TX is empty
        timeout_counter = 0;
        while ((i2c_register_address->status & I2C_STATUS_COMMAND_FIFO_EMPTY) == 0)
        {
            timeout_counter++;
            if (timeout_counter > I2C_TIMEOUT_COUNT)
            {
                // timeout, kill the transaction with a CLEAR FIFO
                i2c_register_address->status = I2C_STATUS_CLEAR_FIFO;
                reset_i2c_bus();
                return TR_HAL_I2C_WRITE_TIMEOUT_ERROR;
            }
        }
    }

    // are we reading bytes
    if (num_bytes_to_read > 0)
    {
        // send START condition
        i2c_register_address->command = I2C_COMMAND_START;

        // write the target address and the read bit
        i2c_register_address->command = I2C_COMMAND_WRITE_DATA_8;
        i2c_register_address->command = (target_address << 1) | I2C_READ_DATA_BIT;
        i2c_register_address->command = I2C_COMMAND_VACK;

        // read the bytes
        for (index = 0; index < num_bytes_to_read; index++)
        {
            // read the byte and send an ack
            i2c_register_address->command = I2C_COMMAND_READ_DATA_8;
            i2c_register_address->command = I2C_COMMAND_WRITE_DATA_1;

            // wait for read to be ready
            timeout_counter = 0;
            while ((i2c_register_address->status & I2C_STATUS_READ_DATA_FIFO_NOT_EMPTY) == 0)
            {
                timeout_counter++;
                if (timeout_counter > I2C_TIMEOUT_COUNT)
                {
                    // timeout, kill the transaction with a CLEAR FIFO
                    i2c_register_address->status = I2C_STATUS_CLEAR_FIFO;
                    reset_i2c_bus();
                    return TR_HAL_I2C_READ_TIMEOUT_ERROR;
                }
            }
        }
        // send STOP at the end
        i2c_register_address->command = I2C_COMMAND_STOP;

        // hand the read bytes to the app
        while ((i2c_register_address->status & I2C_STATUS_READ_DATA_FIFO_NOT_EMPTY) > 0)
        {
            uint32_t data = i2c_register_address->read_data;
            uint8_t data8b = data & 0xFF;
            if (g_current_i2c_settings.rx_handler_function != NULL)
            {
                g_current_i2c_settings.rx_handler_function(data8b);
            }
        }
    }

    return TR_HAL_SUCCESS;
}


// ****************************************************************************
// INTERNAL I2C interrupt handler
// ****************************************************************************
static void i2c_internal_controller_interrupt_handler()
{
    uint32_t event_bitmask = 0;

    // get the i2c register address
    I2C_REGISTERS_T* i2c_register_address = tr_hal_i2c_get_controller_register_address(I2C_CTRL_0_ID);

    // read interrupt status
    uint32_t int_status = i2c_register_address->interrupt_status;

    // clear interrupts
    i2c_register_address->interrupt_status = (int_status & I2C_INTERRUPT_ALL);

    // TX empty
    if ((int_status & I2C_INTERRUPT_COMMAND_FIFO_EMPTY) > 0)
    {
        event_bitmask |= I2C_EVENT_TX_EMPTY;
        g_int_count.count_tx_empty++;
    }

    // RX finish
    if ((int_status & I2C_INTERRUPT_READ_DATA_FIFO_NOT_EMPTY) > 0)
    {
        event_bitmask |= I2C_EVENT_RX_FINISH;
        g_int_count.count_rx_finish++;
   }

    // error
    if ((int_status & I2C_INTERRUPT_ERROR) > 0)
    {
        event_bitmask |= I2C_EVENT_ERROR;
        g_int_count.count_error++;
    }

    // loss of arbitration
    if ((int_status & I2C_INTERRUPT_LOSS_OF_ARBITRATION) > 0)
    {
        event_bitmask |= I2C_EVENT_ABORT_LOST_ARB;
        g_int_count.count_abort_lost_arb++;
    }

    // command done
    if ((int_status & I2C_INTERRUPT_COMMAND_FIFO_DONE) > 0)
    {
        event_bitmask |= I2C_EVENT_COMMAND_DONE;
        g_int_count.count_command_done++;
    }

    // ****************************************
    // call the user event function, passing the event bitmask
    // ****************************************
    if (g_current_i2c_settings.event_handler_fx != NULL)
    {
        g_current_i2c_settings.event_handler_fx(I2C_CTRL_0_ID,
                                                event_bitmask);
    }
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
    memcpy(int_count, &(g_int_count), sizeof(tr_hal_i2c_int_count_t) );

    return TR_HAL_SUCCESS;
}


// ****************************************************************************
// NVIC interrupt call I2C
// ****************************************************************************
void i2cm_handler(void)
{
    i2c_internal_controller_interrupt_handler();
}

