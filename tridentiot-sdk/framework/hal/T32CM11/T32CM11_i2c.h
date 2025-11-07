/// ****************************************************************************
/// @file T32CM11_i2c.h
///
/// @brief This is the chip specific include file for T32CM11 I2C Driver.
///        note that there is a common include file for this HAL module that
///        contains the APIs (such as the init function) that should be used
///        by the application.
///
/// Trident HAL I2C Controller Driver:
///
/// I2C (Inter-Integrated Circuit) also known as IIC, is a 2-wire synchronous,
/// multi-device serial communication protocol. it is simple and low cost but
/// not the fastest (2 wires).
///
/// the 2 wires for I2C are:
/// SDA == serial data
/// SCL == serial clock
///
/// NXP (who created I2C as Philips in 1982 have changed the terminology of
/// I2C and now substitude "Controller" for "Master" and "Target" for "Slave".
///
/// the CM11 chip has 1 I2C Controllers.
///
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
/// ****************************************************************************


#ifndef T32CM11_I2C_H_
#define T32CM11_I2C_H_

#include "tr_hal_platform.h"
#include "tr_hal_gpio.h"


// ******************************************************************
// defines used by the I2C module
// ******************************************************************


/// On the T32CM11 there is just 1 I2C available to act as a Controller.
typedef enum
{
    I2C_CTRL_0_ID = 0,

} tr_hal_i2c_id_t;

#define TR_HAL_MAX_I2C_CONTROLLER_ID 0
#define NUM_I2C_CONTROLLER 1


// where to put the I2C pins
#define I2C_SCL_PIN_OPTION1 20
#define I2C_SDA_PIN_OPTION1 21
#define I2C_SCL_PIN_OPTION2 22
#define I2C_SDA_PIN_OPTION2 23

// defaults
#define DEFAULT_I2C_SCL_PIN I2C_SCL_PIN_OPTION2
#define DEFAULT_I2C_SDA_PIN I2C_SDA_PIN_OPTION2


// command/transmit FIFO is 32
#define I2C_COMMAND_FIFO 32

// receive FIFO is 16 bytes
#define I2C_RX_FIFO_BYTES 16


/// ******************************************************************
/// section 3.1 of the data sheet explains the Memory map
/// this gives the base address for how to write the I2C registers
/// the chip registers are how the software interacts with the I2C
/// chip peripheral. We create a struct below that addresses the
/// individual registers. This makes it so we can use this base
/// address and a struct field to read or write a chip register
/// ******************************************************************

#define CHIP_MEMORY_MAP_I2C_CONTROLLER_BASE 0xA0100000


/// ***************************************************************************
/// the struct we use so we can address I2C controller chip registers using
/// field names
/// ***************************************************************************
typedef struct
{
    // I2C status: TX/RX FIFO and error conditions
    __IO uint32_t status;              // 0x00

    // data read from target
    __IO uint32_t read_data;           // 0x04

    // set this to an I2C command
    __IO uint32_t command;             // 0x08

    // enable interrupts
    __IO uint32_t interrupt_enable;    // 0x0C

    // interrupt status and interrupt clear
    __IO uint32_t interrupt_status;    // 0x10

    // control: enable, clock source, and lower 6 bits of clock divider
    __IO uint32_t control;             // 0x14

    // for setting up the clock, contains upper 8 bits of divider
    __IO uint32_t prescale;            // 0x18

} I2C_REGISTERS_T;


// *****************************************************************
// these defines help when dealing with the STATUS register (0x00)

// read only: 1 = command FIFO empty
#define I2C_STATUS_COMMAND_FIFO_EMPTY        0x01
// read only: 1 = Read Data FIFO not empty
#define I2C_STATUS_READ_DATA_FIFO_NOT_EMPTY  0x02
// 1 = no ack received
// write 1 to clear this condition
#define I2C_STATUS_ERROR_NO_ACK              0x04
// 1 = lost arbitration
// write 1 to clear this condition
#define I2C_STATUS_ERROR_LOST_ARBITRATION    0x08
// 1 = Read Data underflow
#define I2C_STATUS_ERROR_READ_DATA_UNDERFLOW 0x10
// 1 = command FIFO overflow
#define I2C_STATUS_COMMAND_FIFO_OVERFLOW     0x20
// 1 = command FIFO full
#define I2C_STATUS_COMMAND_FIFO_FULL         0x40
// 1 = transfer in progress
#define I2C_STATUS_TRANSFER_IN_PROGRESS      0x80
// 1 = read data FIFO overflow
#define I2C_STATUS_READ_DATA_FIFO_OVERFLOW   0x100
// 1 = command data FIFO ubderflow
#define I2C_STATUS_COMMAND_FIFO_UNDERFLOW    0x200
// clear FIFO: write 1 initiates clear to both FIFOs
#define I2C_STATUS_CLEAR_FIFO                0x400


// *****************************************************************
// these defines help when dealing with the COMMAND register (0x08)

#define I2C_COMMAND_NULL         0x00
#define I2C_COMMAND_WRITE_DATA_0 0x10
#define I2C_COMMAND_WRITE_DATA_1 0x11
#define I2C_COMMAND_WRITE_DATA_8 0x12
#define I2C_COMMAND_READ_DATA_8  0x13
#define I2C_COMMAND_STOP         0x14
#define I2C_COMMAND_START        0x15
#define I2C_COMMAND_VACK         0x16

#define I2C_WRITE_DATA_BIT 0x00
#define I2C_READ_DATA_BIT  0x01

// *****************************************************************
// these defines help when dealing with the INTERRUPT registers (0x0C, 0x10)

#define I2C_INTERRUPT_COMMAND_FIFO_EMPTY       0x01
#define I2C_INTERRUPT_READ_DATA_FIFO_NOT_EMPTY 0x02
#define I2C_INTERRUPT_ERROR                    0x04
#define I2C_INTERRUPT_LOSS_OF_ARBITRATION      0x08
#define I2C_INTERRUPT_COMMAND_FIFO_DONE        0x10

#define I2C_INTERRUPT_ALL                      0x1F

// *****************************************************************
// these defines help when dealing with the CONTROL register (0x14)

// bit 7 = enable/disable I2C
#define I2C_CONTROL_REG_I2C_ENABLE  0x80
#define I2C_CONTROL_REG_I2C_DISABLE 0x00

// bit 6 = which clock, 1=PCK
#define I2C_CONTROL_REG_CLOCK_SOURCE_APB 0x40
#define I2C_CONTROL_REG_CLOCK_SOURCE_EXT 0x00

// bit 0-5 lower 6 bits of clock divider
#define I2C_CONTROL_REG_CLOCK_DIV_MASK 0x3F



// *****************************************************************
// helper enums for CLOCK DIVIDER REGISTER
//
// I2C system clock frequency is determined by:
// 32MHz clock / ((clock_divider register + 1) * 4)
//
// although not all I2C sensors can do 1 MHz, we allow 1 MHz,
// but use 400 KHz as a default since almost all sensors will
// support this
//
// value  0 =  0+1 =  1 --> is 32 Mhz / 1 = 32 MHz
// --------------------------------------------------
// value  7 =  7+1 =  8 --> is 32 MHz / ( 8*4) = 1000 KHz = 1MHz
// value  9 =  9+1 = 10 --> is 32 MHz / (10*4) = 800 KHz
// value 19 = 19+1 = 20 --> is 32 MHz / (20*4) = 400 KHz
// value 39 = 39+1 = 40 --> is 32 MHz / (40*4) = 200 KHz
// value 79 = 79+1 = 80 --> is 32 MHz / (80*4) = 100 KHz
typedef enum
{
    I2C_CLOCK_1_MHZ   =  7,
    I2C_CLOCK_800_KHZ =  9,
    I2C_CLOCK_400_KHZ = 19,
    I2C_CLOCK_200_KHZ = 39,
    I2C_CLOCK_100_KHZ = 79,

} tr_hal_i2c_clock_rate_t;

#define CLOCK_LOWER_SIX_BITS 0x2F


// *****************************************************************
// this orients the struct (for the I2C controllers) with the
// correct addresses, so referencing a field will now read/write
// the correct chip address
// *****************************************************************
#define I2C_CHIP_REGISTERS ((I2C_REGISTERS_T *) CHIP_MEMORY_MAP_I2C_CONTROLLER_BASE)


/// ***************************************************************************
/// if the app wants to directly interface with the chip registers, this is a
/// convenience function for getting the address/struct of a particular I2C
/// Controller so the chip registers can be accessed.
/// ***************************************************************************
I2C_REGISTERS_T* tr_hal_i2c_get_controller_register_address(tr_hal_i2c_id_t i2c_id);

// prototype for callback from the Trident HAL to the app when a byte is received
typedef void (*tr_hal_i2c_receive_callback_t) (uint8_t received_byte);

// prototype for callback from the Trident HAL to the app when an event happens
typedef void (*tr_hal_i2c_event_callback_t) (tr_hal_i2c_id_t i2c_id, uint32_t event_bitmask);


/// ***************************************************************************
/// I2C events
/// ***************************************************************************
#define I2C_EVENT_TX_EMPTY       0x01
#define I2C_EVENT_RX_FINISH      0x02
#define I2C_EVENT_ERROR          0x04
#define I2C_EVENT_ABORT_LOST_ARB 0x08
#define I2C_EVENT_COMMAND_DONE   0x10


/// ***************************************************************************
/// I2C Controller settings strut - this is passed to tr_hal_i2c_init_controller
///
/// ***************************************************************************
typedef struct
{
    // this struct is for I2C Controllers ONLY

    // SDA and SCL pin
    tr_hal_gpio_pin_t sda_pin;
    tr_hal_gpio_pin_t scl_pin;

    // clock setting
    tr_hal_i2c_clock_rate_t clock_setting;

    // callback from HAL to App when a byte is received
    // if the app doesn't want this, then set it to NULL
    tr_hal_i2c_receive_callback_t rx_handler_function;

    // callback from HAL to App when an event happens
    // if the app doesn't want this, then set it to NULL
    tr_hal_i2c_event_callback_t  event_handler_fx;

    // are the chip interrupts enabled
    bool enable_chip_interrupts;

    // set the interrupt priority
    tr_hal_int_pri_t interrupt_priority;

    // when the I2C is powered off we can choose to DISABLE interrupts, meaning
    // we will STAY powered off even when events are happening, or we can choose
    // to KEEP interrupts enabled when powered off. This means we would wake on
    // interrupt and power the UART back on
    bool wake_on_interrupt;

} tr_hal_i2c_settings_t;


/// ***************************************************************************
/// initializer macros for default I2C CONTROLLER setting
///
/// ***************************************************************************
#define I2C_CONFIG_CONTROLLER0                                  \
    {                                                           \
        .sda_pin = (tr_hal_gpio_pin_t) { DEFAULT_I2C_SDA_PIN }, \
        .scl_pin = (tr_hal_gpio_pin_t) { DEFAULT_I2C_SCL_PIN }, \
        .clock_setting = I2C_CLOCK_100_KHZ,                     \
        .rx_handler_function = NULL,                            \
        .event_handler_fx = NULL,                               \
        .enable_chip_interrupts = true,                         \
        .interrupt_priority = TR_HAL_INTERRUPT_PRIORITY_5,      \
        .wake_on_interrupt = false,                             \
    }


/// ***************************************************************************
/// interrupt counts
/// ***************************************************************************
typedef struct
{
    // to be determined
    uint32_t count_rx_finish;
    uint32_t count_tx_empty;
    uint32_t count_abort_lost_arb;
    uint32_t count_error;
    uint32_t count_command_done;

} tr_hal_i2c_int_count_t;


#endif // T32CM11_I2C_H_


