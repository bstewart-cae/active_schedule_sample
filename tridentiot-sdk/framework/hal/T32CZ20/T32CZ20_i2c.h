/// ****************************************************************************
/// @file T32CZ20_i2c.h
///
/// @brief This is the chip specific include file for T32CZ20 I2C Driver.
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
/// the CZ20 chip has 2 I2C Controllers and 1 I2C Target. Controllers and
/// Target use a different register mapping.
///
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
/// ****************************************************************************


#ifndef T32CZ20_I2C_H_
#define T32CZ20_I2C_H_

#include "tr_hal_platform.h"
#include "tr_hal_gpio.h"


// ******************************************************************
// defines used by the I2C module
// ******************************************************************


/// On the T32CZ20 there are 2 I2C available to act as a Controller.
typedef enum
{
    I2C_CTRL_0_ID = 0,
    I2C_CTRL_1_ID = 1,

} tr_hal_i2c_id_t;

#define TR_HAL_MAX_I2C_CONTROLLER_ID 1
#define NUM_I2C_CONTROLLER 2


// default I2C pins - these can be any of the available pins
// these have been picked since they are the pins used for I2C
// on the CM11 (which has a much stricter pin usage)
#define DEFAULT_I2C_SCL0_PIN 22
#define DEFAULT_I2C_SDA0_PIN 23

#define DEFAULT_I2C_SCL1_PIN 20
#define DEFAULT_I2C_SDA1_PIN 21


// transmit FIFO is 9x16
#define I2C_TX_FIFO_BYTES 18

// receive FIFO is 8x16
#define I2C_RX_FIFO_BYTES 16


/// ******************************************************************
/// section 2.2 of the data sheet explains the Memory map
/// this gives the base address for how to write the I2C registers
/// the chip registers are how the software interacts with the I2C
/// chip peripheral. We create a struct below that addresses the
/// individual registers. This makes it so we can use this base
/// address and a struct field to read or write a chip register
/// ******************************************************************
#ifdef I2C_MASTER0_SECURE_EN
    #define CHIP_MEMORY_MAP_I2C_CONTROLLER0_BASE (0x5002B000UL)
#else
    #define CHIP_MEMORY_MAP_I2C_CONTROLLER0_BASE (0x4002B000UL)
#endif // I2C_MASTER0_SECURE_EN

#ifdef I2C_MASTER1_SECURE_EN
    #define CHIP_MEMORY_MAP_I2C_CONTROLLER1_BASE (0x5002C000UL)
#else
    #define CHIP_MEMORY_MAP_I2C_CONTROLLER1_BASE (0x4002C000UL)
#endif // I2C_MASTER1_SECURE_EN



/// ***************************************************************************
/// the struct we use so we can address I2C controller chip registers using
/// field names
/// ***************************************************************************
typedef struct
{
    // control: enable, restart, stop, bus clear, FIFO clear
    __IO uint32_t control;              // 0x00

    // target address
    __IO uint32_t target;               // 0x04

    // receive buffer or transmit buffer
    __IO uint32_t buffer;               // 0x08

    // interrupts
    __IO uint32_t interrupt_status;     // 0x0C
    __IO uint32_t interrupt_enable;     // 0x10
    __IO uint32_t interrupt_raw_status; // 0x14
    __IO uint32_t interrupt_clear;      // 0x18

    // setting up the clock
    __IO uint32_t clock_divider;        // 0x1C

} I2C_REGISTERS_T;


// *****************************************************************
// these defines help when dealing with the CONTROL register (0x00)

// bit 0 enables or disables the device for I2C
#define I2C_CONTROL_ENABLE_CONTROLLER 0x01
#define I2C_CONTROL_DISABLE_CONTROLLER 0x00
// bit 1 enables restart
#define I2C_CONTROL_ENABLE_RESTART    0x02
// bit 2 stops the current transaction
#define I2C_CONTROL_STOP_TRANSACTION  0x04
// bit 3 clears the bus by sending 9 clock pulses
#define I2C_CONTROL_BUS_CLEAR         0x08
// bit 4 clears the TX and RX FIFOs
#define I2C_CONTROL_FIFO_CLEAR        0x10


// *****************************************************************
// these defines help when dealing with the BUFFER register (0x04)

// when receiving data set the register to this
#define I2C_BUFFER_SET_FOR_READ  0x100
// when sending data, the bit gets set to 0, plus the data
#define I2C_BUFFER_SET_FOR_WRITE 0x000

// *****************************************************************
// these defines help when dealing with the INTERRUPT registers (0x0C, 0x10, 0x14, 0x18)

#define I2C_INTERRUPT_RX_UNDER       0x01
#define I2C_INTERRUPT_RX_OVER        0x02
#define I2C_INTERRUPT_RX_FULL        0x04
#define I2C_INTERRUPT_RX_FINISH      0x08
#define I2C_INTERRUPT_TX_OVER        0x10
#define I2C_INTERRUPT_TX_EMPTY       0x20
#define I2C_INTERRUPT_ABORT_A_NACK   0x40
#define I2C_INTERRUPT_ABORT_W_NACK   0x80
#define I2C_INTERRUPT_ABORT_LOST_ARB 0x100
#define I2C_INTERRUPT_IDLE_STATE     0x200

#define I2C_INTERRUPT_ALL            0x1FF


// *****************************************************************
// helper enums for CLOCK DIVIDER REGISTER (0x1C)
//
// I2C system clock frequency is determined by:
// 32MHz clock / (clock_divider register + 1)
//
// although not all I2C sensors can do 1 MHz, we allow 1 MHz,
// but use 400 KHz as a default since almost all sensors will
// support this
//
// value  0 =  0+1 =  1 --> is 32 Mhz / 1 = 32 MHz
// --------------------------------------------------
// value   31 =    31+1 =    32 --> is 32 Mhz / 32 = 1000 KHz = 1 MHz
// value   79 =    79+1 =    80 --> is 32 Mhz /  80 = 400 KHz
// value  159 =   159+1 =   160 --> is 32 Mhz / 160 = 200 KHz
// value  319 =   319+1 =   320 --> is 32 Mhz / 320 = 100 KHz
// value  639 =   639+1 =   640 --> is 32 Mhz / 640 =  50 KHz
// value 3199 =  3199+1 =  3200 --> is 32 Mhz / 3200 = 10 KHz
typedef enum
{
    I2C_CLOCK_1_MHZ   = 31,
    I2C_CLOCK_400_KHZ = 79,
    I2C_CLOCK_200_KHZ = 159,
    I2C_CLOCK_100_KHZ = 319,
    I2C_CLOCK_50_KHZ  = 639,
    I2C_CLOCK_10_KHZ  = 3199,

} tr_hal_i2c_clock_rate_t;


// *****************************************************************
// this orients the 2 structs (for 2 I2C controllers) with the
// correct addresses, so referencing a field will now read/write
// the correct chip address
// *****************************************************************
#define I2C0_CHIP_REGISTERS  ((I2C_REGISTERS_T *) CHIP_MEMORY_MAP_I2C_CONTROLLER0_BASE)
#define I2C1_CHIP_REGISTERS  ((I2C_REGISTERS_T *) CHIP_MEMORY_MAP_I2C_CONTROLLER1_BASE)


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
#define I2C_EVENT_RX_UNDER       0x01
#define I2C_EVENT_RX_OVER        0x02
#define I2C_EVENT_RX_FULL        0x04
#define I2C_EVENT_RX_FINISH      0x08
#define I2C_EVENT_TX_OVER        0x10
#define I2C_EVENT_TX_EMPTY       0x20
#define I2C_EVENT_ABORT_A_NACK   0x40
#define I2C_EVENT_ABORT_W_NACK   0x80
#define I2C_EVENT_ABORT_LOST_ARB 0x100


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
        .sda_pin = (tr_hal_gpio_pin_t) { DEFAULT_I2C_SDA0_PIN },\
        .scl_pin = (tr_hal_gpio_pin_t) { DEFAULT_I2C_SCL0_PIN },\
        .clock_setting = I2C_CLOCK_100_KHZ,                     \
        .rx_handler_function = NULL,                            \
        .event_handler_fx = NULL,                               \
        .enable_chip_interrupts = true,                         \
        .interrupt_priority = TR_HAL_INTERRUPT_PRIORITY_5,      \
        .wake_on_interrupt = false,                             \
    }

#define I2C_CONFIG_CONTROLLER1                                  \
    {                                                           \
        .sda_pin = (tr_hal_gpio_pin_t) { DEFAULT_I2C_SDA1_PIN },\
        .scl_pin = (tr_hal_gpio_pin_t) { DEFAULT_I2C_SCL1_PIN },\
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
    uint32_t count_rx_under;
    uint32_t count_rx_over;
    uint32_t count_rx_full;
    uint32_t count_rx_finish;
    uint32_t count_tx_over;
    uint32_t count_tx_empty;
    uint32_t count_abort_a_nack;
    uint32_t count_abort_w_nack;
    uint32_t count_abort_lost_arb;
    uint32_t count_idle;

    // debug
    uint32_t write_exit_on_int_status;
    uint32_t write_exit_on_flag;
    uint32_t write_exit_on_crazy;
    uint32_t read_exit_on_int_status;
    uint32_t read_exit_on_flag;
    uint32_t read_exit_on_crazy;

} tr_hal_i2c_int_count_t;


#endif // T32CZ20_I2C_H_


