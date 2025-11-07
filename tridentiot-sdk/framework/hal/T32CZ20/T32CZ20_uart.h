/// ****************************************************************************
/// @file T32CZ20_uart.h
///
/// @brief This is the chip specific include file for T32CZ20 UART Driver.
///        note that there is a common include file for this HAL module that
///        contains the APIs (such as the init function) that should be used
///        by the application.
///
/// Trident HAL UART Driver:
///
/// This module contains APIs for interacting with the Trident HAL for UART.
/// This defines a struct to use for setting up a UART and APIs to transmit
/// and receive from a UART. It also defines a struct that allows interaction
/// with the chip registers.
///
/// before setting up a UART, pick how it will be used:
///
/// --- transmit ---
/// 1. transmit using the raw TX APIs
/// 2. transmit using DMA TX APIs
///
/// --- receive ---
/// 1. receive automatically in the user defined receive function
///    (this is recommended)
/// 2. receive using DMA RX, where the app manages the DMA RX buffer
/// 3. receive using the raw API calls
///
/// UART configuration is done by setting fields in an instance of the
/// tr_hal_uart_settings_t struct and then passing this instance to tr_hal_uart_init.
///
/// There is a define that can be used to initialize an instance to the default
/// values for that particular UART. for instance, to init UART1 to default
/// values set it to DEFAULT_UART1_CONFIG, like this:
///     tr_hal_uart_settings_t g_uart1_settings = DEFAULT_UART1_CONFIG;
///
/// if a user defined receive function is set (rx_handler_function)
/// then the Trident HAL will manage the chip interrupts and registers and
/// will pass received bytes to this user receive function. This is the
/// recommended way to get UART communication working. If this method is used
/// then the app will just handle bytes that come in to the user receive
/// function and can send responses using any of the TX APIs,
/// for instance:
///       tr_hal_uart_raw_tx_one_byte(UART_1_ID, byte_to_send);
///       tr_hal_uart_raw_tx_buffer(UART_1_ID, &bytes_to_send, send_length);
///
/// alternatively, the UART can be setup to use the chip DMA functions to
/// transmit or receive bytes. Set the correct fields in the tr_hal_uart_settings_t
/// to enable DMA TX (tx_dma_enabled) or DMA RX (rx_dma_enabled).
///
///
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************

#ifndef T32CZ20_UART_H_
#define T32CZ20_UART_H_

#include "tr_hal_platform.h"
#include "tr_hal_gpio.h"


/// ****************************************************************************
/// @defgroup tr_hal_uart_cz20 UART CZ20
/// @ingroup tr_hal_T32CZ20
/// @{
/// ****************************************************************************


// ******************************************************************
// defines used by the UART module
// ******************************************************************

/// the T32CZ20 has 3 UARTs available for configuration. this is a chip constraint - can't change it.
#define TR_NUMBER_OF_UARTS 3

/// this type is used to specify a UART ID. On the T32CZ20 there are 3 UARTs available.
typedef enum
{
    UART_0_ID = 0,
    UART_1_ID = 1,
    UART_2_ID = 2,

} tr_hal_uart_id_t;

/// this sets a threshhold for the number of bytes left in the DMA buffer
/// before we should notify the user via an event. This is a software
/// setting, and it can be adjusted/changed
#define LOW_BYTES_BUFFER_THRESHHOLD 16

/// This defines the maximum size buffer the raw tx API can take in to transmit.
/// this is a software setting, and it can be adjusted.
/// if adjusting, note that this uses 3x the RAM since we have 3 UARTs.
#define MAX_RAW_TX_DATA_BUFFER_SIZE 256

/// If using a DMA buffer we enforce a minimum size.
/// This is a software setting, and current set to equal the FIFO RX size
#define DMA_RX_BUFF_MINIMUM_SIZE 16

/// This is the size of the RX and TX FIFO.
/// This is a chip constraint - can't change it
#define TX_FIFO_SIZE 32

// default interrupt priority for UART
//#define UART_DEFAULT_INTERRUPT_PRIORITY TR_HAL_INTERRUPT_PRIORITY_5


// ******************************************************************
// on the CZ20 any combination of RX and TX works as long as they don't overlap.
// this defines default locations for the UART pins but these could be changed.
// ******************************************************************

/// UART0 TX default
#define UART0_TX_PIN_DEFAULT 17
/// UART0 RX default
#define UART0_RX_PIN_DEFAULT 16

/// UART1 TX default
#define UART1_TX_PIN_DEFAULT 6
/// UART1 RX default
#define UART1_RX_PIN_DEFAULT 7

/// UART2 TX default
#define UART2_TX_PIN_DEFAULT 8
/// UART2 RX default
#define UART2_RX_PIN_DEFAULT 9

/// use for an unset pin
#define TR_HAL_PIN_NOT_SET 0xFF


/// ******************************************************************
/// section 2.2 of the data sheet explains the Memory map
/// this gives the base address for how to write the UART registers
/// the UART registers are how the software interacts with the UART
/// peripheral. We create a struct below that addresses the individual
/// registers. This makes it so we can use this base address and a
/// struct field to read or write a chip register
/// ******************************************************************
#ifdef UART0_SECURE_EN
    #define CHIP_MEMORY_MAP_UART0_BASE     (0x50012000UL)
#else
    #define CHIP_MEMORY_MAP_UART0_BASE     (0x40012000UL)
#endif // UART0_SECURE_EN

#ifdef UART1_SECURE_EN
    #define CHIP_MEMORY_MAP_UART1_BASE     (0x50013000UL)
#else
    #define CHIP_MEMORY_MAP_UART1_BASE     (0x40013000UL)
#endif // UART1_SECURE_EN

#ifdef UART2_SECURE_EN
    #define CHIP_MEMORY_MAP_UART2_BASE     (0x50025000UL)
#else
    #define CHIP_MEMORY_MAP_UART2_BASE     (0x40025000UL)
#endif // UART2_SECURE_EN


/// values we can trigger on for receive FIFO
typedef enum
{
    FCR_TRIGGER_1_BYTE   = 0x00,
    FCR_TRIGGER_4_BYTES  = 0x40,
    FCR_TRIGGER_8_BYTES  = 0x80,
    FCR_TRIGGER_14_BYTES = 0xC0,
    FCR_NO_TRIGGER       = 0xFF,

} tr_hal_fifo_trigger_t;


/// bits 0, 1 set the data-bits: 0b00=5 data-bits, 0b01=6 data-bits, 0b10=7 data-bits, 0b11=8 data-bits
/// these values come from the chip register and cannot be changed
typedef enum
{
    LCR_DATA_BITS_5_VALUE       = 0x00,
    LCR_DATA_BITS_6_VALUE       = 0x01,
    LCR_DATA_BITS_7_VALUE       = 0x02,
    LCR_DATA_BITS_8_VALUE       = 0x03,
    LCR_DATA_BITS_INVALID_VALUE = 0xFF,

} tr_hal_data_bits_t;


/// bit 2 sets the stop bits: 0b0=1 stop bit, 0b1=2 stop bits
/// these values come from the chip register and cannot be changed
typedef enum
{
    LCR_STOP_BITS_ONE_VALUE     = 0x00,
    LCR_STOP_BITS_TWO_VALUE     = 0x04,
    LCR_STOP_BITS_INVALID_VALUE = 0xFF,

} tr_hal_stop_bits_t;


/// bit 3, 4 set the parity: 0b00=no parity 0b10=no parity, 0b01=odd parity, 0b11=even parity,
/// these values come from the chip register and cannot be changed
typedef enum
{
    LCR_PARITY_NONE_VALUE    = 0x00,
    LCR_PARITY_ODD_VALUE     = 0x08,
    LCR_PARITY_EVEN_VALUE    = 0x16,
    LCR_PARITY_INVALID_VALUE = 0xFF,

} tr_hal_parity_t;

/// for setting up hardware flow control
typedef enum
{
    MCR_NO_FLOW_CONTROL_VALUE = 0x00,
    MCR_SET_RTS_READY         = 0x02,
    MCR_SET_CTS_ENABLED       = 0x20,

} tr_hal_hw_fc_t;


/// ***************************************************************************
/// \cond
/// starting here we tell doxygen to ignore the code
/// ***************************************************************************


/// ***************************************************************************
/// the struct we use so we can address UART chip registers using field names
/// ***************************************************************************
typedef struct
{
    // received data
    __IO uint32_t receive_buffer_register;   // 0x00 = RBR

    // enable interrupts
    __IO uint32_t interrupt_enable_register; // 0x04 = IER

    // FIFO trigger levels
    __IO uint32_t FIFO_control_register;     // 0x08 = FCR

    // line control: parity, stop bits, word length
    __IO uint32_t line_control_register;     // 0x0C = LCR

    // configure CTS, RTS, loopback
    __IO uint32_t modem_control_register;    // 0x10 = MCR

    // transmitter empty, data ready, overrun error, framing error, etc
    __IO  uint32_t line_status_register;     // 0x14 = LSR

    // CTS and DCTS
    __I  uint32_t modem_status_register;     // 0x18 = MSR

    // used to tell which interrupt fired
    __IO uint32_t interrupt_identification_register;   // 0x1C = IIR

    // these control the baud rate by dividing the clock
    // baud rate = clock / (divisor latch + fac divisor) x 8
    __IO uint32_t divisor_latch_register;    // 0x20 = DLX
    __IO uint32_t fractional_divisor_latch;  // 0x24 = FDL

    // this changes the baud rate divisor and latch by 1/2
    // changes division multipler from 8x to 4x. need to use
    // different set of baud rates
    __IO uint32_t low_speed_mode;            // 0x28

    __IO uint32_t wake_enable;               // 0x2C
    __IO uint32_t enable;                    // 0x30


    // setup for DMA RX
    __IO uint32_t DMA_rx_buffer_addr;  // 0x34
    __IO uint32_t DMA_rx_buffer_len;   // 0x38

    // setup for DMA TX
    __IO uint32_t DMA_tx_buffer_addr;  // 0x3C
    __IO uint32_t DMA_tx_buffer_len;   // 0x40

    // using DMA
    __I  uint32_t DMA_rx_xfer_len_remaining;  // 0x44
    __I  uint32_t DMA_tx_xfer_len_remaining;  // 0x48
    __IO uint32_t DMA_rx_enable;              // 0x4C
    __IO uint32_t DMA_tx_enable;              // 0x50

} UART_REGISTERS_T;

// REMOVED
//    __IO uint32_t DMA_interrupt_enable_register; //0x38
//    __IO uint32_t DMA_interrupt_status;          //0x3C

// *****************************************************************
// *** some registers are multi-purpose:

// at register address 0x00, a read pulls data from RBR (read-buffer),
// while a write puts a byte in the THR (transmitter-holder)
#define transmitter_holding_register receive_buffer_register


// *****************************************************************
// this orients the 3 structs (for 3 UARTs) with the correct addresses
// so referencing a field will now read/write the correct chip address
// *****************************************************************
#define UART0_CHIP_REGISTERS  ((UART_REGISTERS_T *) CHIP_MEMORY_MAP_UART0_BASE)
#define UART1_CHIP_REGISTERS  ((UART_REGISTERS_T *) CHIP_MEMORY_MAP_UART1_BASE)
#define UART2_CHIP_REGISTERS  ((UART_REGISTERS_T *) CHIP_MEMORY_MAP_UART2_BASE)

// *****************************************************************
// these defines help when dealing with the INTERRUPT ENABLE REGISTER (0x04)
#define IER_ENABLE_RECEIVE_DATA_AVAIL_INT           0x01
#define IER_ENABLE_READY_TO_TRANSMIT_INT            0x02
#define IER_ENABLE_FRAMING_PARITY_OVERRUN_ERROR_INT 0x04
#define IER_ENABLE_MODEM_STATUS_INT                 0x08
#define IER_ENABLE_RECEIVED_DATA_TIMEOUT            0x10
#define IER_ENABLE_DMA_RX_INT                       0x20
#define IER_ENABLE_DMA_TX_INT                       0x40

// *****************************************************************
// these defines help when dealing with the FIFO CONTROL REGISTER (0x08)
// this uses tr_hal_fifo_trigger_t enum
#define FCR_CLEAR_RECEIVER  0x02
#define FCR_CLEAR_TRANSMIT  0x04
#define FCR_DMA_SELECT      0x08

#define FCR_TRIGGER_MASK     0xC0
#define FCR_RTS_TRIGGER_MASK 0x300


// *****************************************************************
// these defines to help when dealing with the LINE CONTROL REGISTER (0x0C)
// bits 0, 1 set the data bits: 00=5, 01=6, 10=7, 11=8, use tr_hal_data_bits_t
// bit 2 sets the stop bits: 0=1 stop bit, 1=2 stop bits, use tr_hal_stop_bits_t
// bit 3, 4 set the parity: 00=no parity 10=no parity, 01=odd parity, 11=even parity,
// bit 5 sets the sticky parity -> for debug
// bit 6 sets break -> for debug
// bit 7 sets the divisior latch access - when set to 1 this changes
//       the RBR and IER registers to be the divisor latch bytes which
//       sets the baud rate
#define LCR_DATA_BITS_MASK         0x03
#define LCR_DATA_BITS_INV_MASK     0xFC
#define LCR_STOP_BITS_MASK         0x04
#define LCR_STOP_BITS_INV_MASK     0xFB
#define LCR_PARITY_BITS_MASK       0x18
#define LCR_PARITY_BITS_INV_MASK   0xE7
#define LCR_BAUD_RATE_SETTING_MASK 0x80



// *****************************************************************
// helper enum for MODEM CONTROL REGISTER (0x10)
// the modem control register controls the HW flow control, use tr_hal_hw_fc_t enum


// *****************************************************************
// helper defines for LINE STATUS REGISTER (0x14)
// bit 0 set means data ready
#define LSR_DATA_READY    0x01
// bit 1 is set when an incoming character over writes a waiting character
#define LSR_OVERRUN_ERROR 0x02
// bit 2 is the parity error
#define LSR_PARITY_ERROR  0x04
// bit 3 is framing error
#define LSR_FRAMING_ERROR 0x08
// bit 4 is the break interrupt
#define LSR_BREAK_INDICATOR 0x10
// bit 5 is THR empty. behavior is different if FIFOs are enabled - see 10.3.8
// if FIFO is enabled, then this is CLEAR (0) when FIFO is empty
// if FIFO is disabled, then this is CLEAR (0) when THR is empty (so it can be used again)
#define LSR_TRANSMITTER_HOLDING_REG_EMPTY 0x20
// bit 6 is SET(1) when THR is empty and ready to accept characters (in both FIFO and non-FIFO mode)
#define LSR_TRANSMITTER_EMPTY 0x40
// this defines the 4 bits for the 4 error bits that need to be cleared on interrupt
#define CLEAR_RX_ERROR_BY_CLEAR_LSR_BITS 0x1E

// *****************************************************************
// helper defines for MODEM STATUS REGISTER (0x18)
#define MSR_DELTA_CTS 0x01
#define MSR_CTS  0x10

// *****************************************************************
// these defines help when dealing with the INTERRUPT IDENTIFICATION REGISTER (0x1C)
#define IIR_RX_DATA_AVAIL_INTERRUPT   0x01
#define IIR_THR_EMPTY_INTERRUPT       0x02
#define IIR_RECEIVER_ERROR_INTERRUPT  0x04
#define IIR_MODEM_STATUS_INTERRUPT    0x08
#define IIR_RX_DATA_TIMEOUT_INTERRUPT 0x10
#define IIR_DMA_RX_INTERRUPT          0x20
#define IIR_DMA_TX_INTERRUPT          0x40
#define IIR_INTERRUPT_MASK            0x7F

// *****************************************************************
// helper defines for LOW SPEED MODE (0x28)
#define LOW_SPEED_MODE_ENABLED    0x01
#define NORMAL_SPEED_MODE_ENABLED 0x00

// *****************************************************************
// helper defines for UART WAKE ENABLE (0x2C)
#define UART_WAKE_ENABLE     0x01

// *****************************************************************
// helper defines for UART ENABLE (0x30)
#define UART_ENABLE     0x01


// *****************************************************************
// helper defines for DMA_rx_enable (0x4C) and DMA_tx_enable(0x50) REGISTERs
#define UART_DMA_ENABLE  0x01
#define UART_DMA_DISABLE 0x00


/// ***************************************************************************
/// \endcond
/// done with doxygen ignoring the code
/// ***************************************************************************


/// *****************************************************************
/// baud rates - see section 18.3.3
/// the baud rate is determines by taking the 32 MHz clock and
/// dividing by 8 and also by a configured number. The configured
/// number is a whole number divisor and a fractional divisor
/// the fractional divisor is a number from 1-8 divided by 8.
///
/// FOR INSTANCE: for a target of 115200 baud rate
/// 32,000,000 / 8 = 4,000,000 / 115200 = 34.722
/// divisor would be 34
/// fractional divisor would be 6/8th which is closest 8th to 0.722
/// so set divisor_latch_register = 34
/// and set fractional_divisor_latch = FRAC_DIVISOR_6
///
/// we have an enum so the user can specify what baud rate is desired
/// that enum is used to lookup the divisor and fractional divisor
/// to set the 2 registers
///
/// DO NOT CHANGE THESE NUMBERS AS THEY ARE USED AS INDICES INTO
/// THE LOOKUP TABLE FOR THE DIVISOR AND FRAC DIVISOR
/// *****************************************************************
#define TR_HAL_NUM_FULL_SPEED_BAUD_RATES     14
#define TR_HAL_NUM_LPM_BAUD_RATES            14
#define TR_HAL_NUM_LOW_SPEED_BAUD_RATES      10
#define TR_HAL_NUM_SLOW_CLOCK_BAUD_RATES      3
#define TR_HAL_NUM_BAUD_RATES (TR_HAL_NUM_FULL_SPEED_BAUD_RATES + \
                               TR_HAL_NUM_LPM_BAUD_RATES + \
                               TR_HAL_NUM_LOW_SPEED_BAUD_RATES + \
                               TR_HAL_NUM_SLOW_CLOCK_BAUD_RATES)

typedef enum
{
    // *******************************************
    // NOTE: users should ONLY use the first set of values
    // these are mapped to the correct baud rate
    // this way the users don't need to map specific baud rate enums to the clock type
    // *******************************************

    // these are the baud rate values for when using the 32MHz system clock
    // tr_hal_clock_t clock_mode = TR_HAL_CLOCK_32M
    TR_HAL_UART_BAUD_RATE_2400     = 0,
    TR_HAL_UART_BAUD_RATE_4800     = 1,
    TR_HAL_UART_BAUD_RATE_9600     = 2,
    TR_HAL_UART_BAUD_RATE_14400    = 3,
    TR_HAL_UART_BAUD_RATE_19200    = 4,
    TR_HAL_UART_BAUD_RATE_28800    = 5,
    TR_HAL_UART_BAUD_RATE_38400    = 6,
    TR_HAL_UART_BAUD_RATE_57600    = 7,
    TR_HAL_UART_BAUD_RATE_76800    = 8,
    TR_HAL_UART_BAUD_RATE_115200   = 9,
    TR_HAL_UART_BAUD_RATE_230400   = 10,
    TR_HAL_UART_BAUD_RATE_500000   = 11,
    TR_HAL_UART_BAUD_RATE_1000000  = 12,
    TR_HAL_UART_BAUD_RATE_2000000  = 13,

    // these are the baud rate values for when using the 32MHz system clock
    // and in low_speed_mode enabled
    // (uart_register_address->low_speed_mode = LOW_SPEED_MODE_ENABLED)
    // tr_hal_clock_t clock_mode = TR_HAL_CLOCK_16M
    TR_HAL_UART_BAUD_RATE_LPM_2400     =  0 + TR_HAL_NUM_FULL_SPEED_BAUD_RATES,
    TR_HAL_UART_BAUD_RATE_LPM_4800     =  1 + TR_HAL_NUM_FULL_SPEED_BAUD_RATES,
    TR_HAL_UART_BAUD_RATE_LPM_9600     =  2 + TR_HAL_NUM_FULL_SPEED_BAUD_RATES,
    TR_HAL_UART_BAUD_RATE_LPM_14400    =  3 + TR_HAL_NUM_FULL_SPEED_BAUD_RATES,
    TR_HAL_UART_BAUD_RATE_LPM_19200    =  4 + TR_HAL_NUM_FULL_SPEED_BAUD_RATES,
    TR_HAL_UART_BAUD_RATE_LPM_28800    =  5 + TR_HAL_NUM_FULL_SPEED_BAUD_RATES,
    TR_HAL_UART_BAUD_RATE_LPM_38400    =  6 + TR_HAL_NUM_FULL_SPEED_BAUD_RATES,
    TR_HAL_UART_BAUD_RATE_LPM_57600    =  7 + TR_HAL_NUM_FULL_SPEED_BAUD_RATES,
    TR_HAL_UART_BAUD_RATE_LPM_76800    =  8 + TR_HAL_NUM_FULL_SPEED_BAUD_RATES,
    TR_HAL_UART_BAUD_RATE_LPM_115200   =  9 + TR_HAL_NUM_FULL_SPEED_BAUD_RATES,
    TR_HAL_UART_BAUD_RATE_LPM_230400   = 10 + TR_HAL_NUM_FULL_SPEED_BAUD_RATES,
    TR_HAL_UART_BAUD_RATE_LPM_500000   = 11 + TR_HAL_NUM_FULL_SPEED_BAUD_RATES,
    TR_HAL_UART_BAUD_RATE_LPM_1000000  = 12 + TR_HAL_NUM_FULL_SPEED_BAUD_RATES,
    TR_HAL_UART_BAUD_RATE_LPM_2000000  = 13 + TR_HAL_NUM_FULL_SPEED_BAUD_RATES,

    // these are the baud rate values for when using the rco1m clock
    // tr_hal_clock_t clock_mode = TR_HAL_CLOCK_1M
    TR_HAL_UART_BAUD_RATE_LOW_SPEED_2400     = 0 + TR_HAL_NUM_FULL_SPEED_BAUD_RATES + TR_HAL_NUM_LPM_BAUD_RATES,
    TR_HAL_UART_BAUD_RATE_LOW_SPEED_4800     = 1 + TR_HAL_NUM_FULL_SPEED_BAUD_RATES + TR_HAL_NUM_LPM_BAUD_RATES,
    TR_HAL_UART_BAUD_RATE_LOW_SPEED_9600     = 2 + TR_HAL_NUM_FULL_SPEED_BAUD_RATES + TR_HAL_NUM_LPM_BAUD_RATES,
    TR_HAL_UART_BAUD_RATE_LOW_SPEED_14400    = 3 + TR_HAL_NUM_FULL_SPEED_BAUD_RATES + TR_HAL_NUM_LPM_BAUD_RATES,
    TR_HAL_UART_BAUD_RATE_LOW_SPEED_19200    = 4 + TR_HAL_NUM_FULL_SPEED_BAUD_RATES + TR_HAL_NUM_LPM_BAUD_RATES,
    TR_HAL_UART_BAUD_RATE_LOW_SPEED_28800    = 5 + TR_HAL_NUM_FULL_SPEED_BAUD_RATES + TR_HAL_NUM_LPM_BAUD_RATES,
    TR_HAL_UART_BAUD_RATE_LOW_SPEED_38400    = 6 + TR_HAL_NUM_FULL_SPEED_BAUD_RATES + TR_HAL_NUM_LPM_BAUD_RATES,
    TR_HAL_UART_BAUD_RATE_LOW_SPEED_57600    = 7 + TR_HAL_NUM_FULL_SPEED_BAUD_RATES + TR_HAL_NUM_LPM_BAUD_RATES,
    TR_HAL_UART_BAUD_RATE_LOW_SPEED_76800    = 8 + TR_HAL_NUM_FULL_SPEED_BAUD_RATES + TR_HAL_NUM_LPM_BAUD_RATES,
    TR_HAL_UART_BAUD_RATE_LOW_SPEED_115200   = 9 + TR_HAL_NUM_FULL_SPEED_BAUD_RATES + TR_HAL_NUM_LPM_BAUD_RATES,

    // these are the baud rate values for when using the rco32k clock
    // tr_hal_clock_t clock_mode = TR_HAL_CLOCK_32K
    TR_HAL_UART_BAUD_RATE_SLOW_CLOCK_2400     = 0 + TR_HAL_NUM_FULL_SPEED_BAUD_RATES + TR_HAL_NUM_LPM_BAUD_RATES + TR_HAL_NUM_LOW_SPEED_BAUD_RATES,
    TR_HAL_UART_BAUD_RATE_SLOW_CLOCK_4800     = 1 + TR_HAL_NUM_FULL_SPEED_BAUD_RATES + TR_HAL_NUM_LPM_BAUD_RATES + TR_HAL_NUM_LOW_SPEED_BAUD_RATES,
    TR_HAL_UART_BAUD_RATE_SLOW_CLOCK_9600     = 2 + TR_HAL_NUM_FULL_SPEED_BAUD_RATES + TR_HAL_NUM_LPM_BAUD_RATES + TR_HAL_NUM_LOW_SPEED_BAUD_RATES,

    // error
    TR_HAL_UART_BAUD_RATE_ERROR = TR_HAL_NUM_BAUD_RATES,

} tr_hal_baud_rate_t;


/// ***************************************************************************
/// the UART can be set to various clock modes
/// The clock mode is chosen to save power or to use a clock that is still
/// running when the chip is sleeping.
///
/// the clock to use is set in the SYS_CTRL_CHIP_REGISTERS->system_clock_control_1
/// register. in the fields: uartO_clk_sel, uart1_clk_sel, uart2_clk_sel
///
///There are 3 options:
/// 1. per_clk - this clock can be set to various real clocks. the default and
///    most common is to set this to xtal_clk which is 32MH
/// 2. per_clk running at 16 MHz
///    this is when the SYS_CTRL_CHIP_REGISTERS->system_clock_control_0 has
///    field per_clk_sel set to 0b10 = xtal_clk/2
/// 3. rco1m - RC oscillator at 921.6 KHz ~= 1MHz
/// 4. rco32k - RC oscillator for slow clock timers, runs at 38.4 KHz
///
/// the UART clocks are the same as the ones defines in the POWER module
/// as tr_hal_clock_t
/// ***************************************************************************



/// ***************************************************************************
/// if the app wants to directly interface with the chip registers, this is a
/// convenience function for getting the address/struct of a particular UART
/// so the chip registers can be accessed.
///
/// EXAMPLE: check LSR and if ready, read a byte from RBR
///
///     UART_REGISTERS_T* uart_register_address = tr_hal_uart_get_uart_register_address(2);
///     if ((uart_register_address->line_status_register) & LSR_DATA_READY)
///     {
///        uint8_t rx_data = uart_register_address->receive_buffer_register;
///     }
/// ***************************************************************************
UART_REGISTERS_T* tr_hal_uart_get_uart_register_address(tr_hal_uart_id_t uart_id);


// prototype for callback from the Trident HAL to the app when a byte is received
typedef void (*tr_hal_uart_receive_callback_t) (uint8_t received_byte);

// prototype for callback from the Trident HAL to the app when an event happens
typedef void (*tr_hal_uart_event_callback_t) (uint32_t event_bitmask);


/// ***************************************************************************
/// uart settings strut - this is passed to tr_hal_uart_init
///
/// this contains protocol settings:
///     - baud rate
///     - data bits
///     - stop bits
///     - parity
///     - flow control
///
/// this contains DMA settings for the UART:
///     - DMA enabled for receive
///     - DMA enabled for transmit
///     - initial DMA receive buffer (if DMA is enabled for receive)
///     - initial DMA receive buffer length (if DMA is enabled for receive)
///
/// this contains settings for the user handler function for the UART:
///     - do we have a user handler (callback) for when bytes are received
///     - if we have a user handler, give a pointer to the function
/// ***************************************************************************
typedef struct
{
    // **** GPIO pins for UART ****

    // TX and RX pin
    tr_hal_gpio_pin_t tx_pin;
    tr_hal_gpio_pin_t rx_pin;

    // for hardware flow control - note that ONLY UART1 can do this
    bool hardware_flow_control_enabled;
    tr_hal_gpio_pin_t rts_pin;
    tr_hal_gpio_pin_t cts_pin;


    // **** protocol settings ****

    // use the TR_HAL_UART_BAUD_RATE_xxx defines to set this
    tr_hal_baud_rate_t baud_rate;
    tr_hal_clock_t     clock_to_use;

    // these use the same bits as LCR - use the LCR defines to set these
    tr_hal_data_bits_t data_bits;
    tr_hal_stop_bits_t stop_bits;
    tr_hal_parity_t    parity;

    // **** DMA settings ****

    bool     rx_dma_enabled;
    bool     tx_dma_enabled;
    // note: buffer must STAY allocated
    uint8_t* rx_dma_buffer;
    uint16_t rx_dma_buff_length;


    // **** non-DMA transmit ****

    // if transmit is not done with DMA, then the app needs to allocate a
    // transmit buffer and set a pointer to that transmit buffer here
    uint8_t* raw_tx_buffer;
    uint16_t raw_tx_buff_length;


    // **** receive and event handler functions ****

    // callback from HAL to App when a byte is received
    // if the app doesn't want this, then set it to NULL
    tr_hal_uart_receive_callback_t rx_handler_function;

    // callback from HAL to App when an event happens
    // if the app doesn't want this, then set it to NULL
    tr_hal_uart_event_callback_t  event_handler_fx;


    // **** chip behavior settings ****

    // how many bytes before chip triggers the user function: 1, 4, 8, 12, or 0 (never)
    // default and suggested value is 1
    tr_hal_fifo_trigger_t rx_bytes_before_trigger;

    // are the chip interrupts enabled?
    bool enable_chip_interrupts;

    // set the INT priority
    tr_hal_int_pri_t interrupt_priority;

    // **** sleep behavior ****

    // when we go to deep sleep, the UART is off
    // when we go to lite sleep, the UART can still run, but needs to use
    // a different clock and baud rate settings. If we want the UART to run
    // in lite sleep mode, then set this to true and set valid clock and
    // baud rate for this UART
    bool               run_when_sleeping;
    tr_hal_baud_rate_t sleep_baud_rate;
    tr_hal_clock_t     sleep_clock_to_use;

    // when the UART is powered off we can choose to DISABLE interrupts, meaning
    // we will STAY powered off even when events are happening, or we can choose
    // to KEEP interrupts enabled when powered off. This means we would wake on
    // interrupt and power the UART back on
    bool wake_on_interrupt;

    // don't include UART power setting in the config
    // uart_init puts the UART in the powered on state
    // uart_power will change the power setting

} tr_hal_uart_settings_t;


// ************************************************************
// default values so an app can quickly load a reasonable set of values
// and then make any changes necessary
//
// this sets up for 115200 8-n-1, no DMA, and a default interrupt and
// receive function that can be overridden in the users application
//
// ************************************************************
#define DEFAULT_UART0_CONFIG                                    \
    {                                                           \
        .tx_pin = (tr_hal_gpio_pin_t) { UART0_TX_PIN_DEFAULT }, \
        .rx_pin = (tr_hal_gpio_pin_t) { UART0_RX_PIN_DEFAULT }, \
        .hardware_flow_control_enabled = false,                 \
        .rts_pin = (tr_hal_gpio_pin_t) { TR_HAL_PIN_NOT_SET },  \
        .cts_pin = (tr_hal_gpio_pin_t) { TR_HAL_PIN_NOT_SET },  \
        .baud_rate = TR_HAL_UART_BAUD_RATE_115200,              \
        .clock_to_use = TR_HAL_CLOCK_32M,                       \
        .data_bits = LCR_DATA_BITS_8_VALUE,                     \
        .stop_bits = LCR_STOP_BITS_ONE_VALUE,                   \
        .parity = LCR_PARITY_NONE_VALUE,                        \
        .rx_dma_enabled = false,                                \
        .tx_dma_enabled = false,                                \
        .rx_dma_buffer = NULL,                                  \
        .rx_dma_buff_length = 0,                                \
        .raw_tx_buffer = NULL,                                  \
        .raw_tx_buff_length = 0,                                \
        .rx_handler_function = NULL,                            \
        .rx_bytes_before_trigger = FCR_TRIGGER_1_BYTE,          \
        .enable_chip_interrupts = true,                         \
        .interrupt_priority = TR_HAL_INTERRUPT_PRIORITY_5,      \
        .wake_on_interrupt = false,                             \
        .event_handler_fx = NULL,                               \
        .run_when_sleeping = true,                              \
        .sleep_baud_rate = TR_HAL_UART_BAUD_RATE_115200,        \
        .sleep_clock_to_use = TR_HAL_CLOCK_1M,                  \
    }

#define DEFAULT_UART1_CONFIG                                    \
    {                                                           \
        .tx_pin = (tr_hal_gpio_pin_t) { UART1_TX_PIN_DEFAULT }, \
        .rx_pin = (tr_hal_gpio_pin_t) { UART1_RX_PIN_DEFAULT }, \
        .hardware_flow_control_enabled = false,                 \
        .rts_pin = (tr_hal_gpio_pin_t) { TR_HAL_PIN_NOT_SET },  \
        .cts_pin = (tr_hal_gpio_pin_t) { TR_HAL_PIN_NOT_SET },  \
        .baud_rate = TR_HAL_UART_BAUD_RATE_115200,              \
        .clock_to_use = TR_HAL_CLOCK_32M,                       \
        .data_bits = LCR_DATA_BITS_8_VALUE,                     \
        .stop_bits = LCR_STOP_BITS_ONE_VALUE,                   \
        .parity = LCR_PARITY_NONE_VALUE,                        \
        .rx_dma_enabled = false,                                \
        .tx_dma_enabled = false,                                \
        .rx_dma_buffer = NULL,                                  \
        .rx_dma_buff_length = 0,                                \
        .raw_tx_buffer = NULL,                                  \
        .raw_tx_buff_length = 0,                                \
        .rx_handler_function = NULL,                            \
        .rx_bytes_before_trigger = FCR_TRIGGER_1_BYTE,          \
        .enable_chip_interrupts = true,                         \
        .interrupt_priority = TR_HAL_INTERRUPT_PRIORITY_5,      \
        .wake_on_interrupt = false,                             \
        .event_handler_fx = NULL,                               \
        .run_when_sleeping = false,                             \
    }

#define DEFAULT_UART2_CONFIG                                    \
    {                                                           \
        .tx_pin = (tr_hal_gpio_pin_t) { UART2_TX_PIN_DEFAULT }, \
        .rx_pin = (tr_hal_gpio_pin_t) { UART2_RX_PIN_DEFAULT }, \
        .hardware_flow_control_enabled = false,                 \
        .rts_pin = (tr_hal_gpio_pin_t) { TR_HAL_PIN_NOT_SET },  \
        .cts_pin = (tr_hal_gpio_pin_t) { TR_HAL_PIN_NOT_SET },  \
        .baud_rate = TR_HAL_UART_BAUD_RATE_115200,              \
        .clock_to_use = TR_HAL_CLOCK_32M,                       \
        .data_bits = LCR_DATA_BITS_8_VALUE,                     \
        .stop_bits = LCR_STOP_BITS_ONE_VALUE,                   \
        .parity = LCR_PARITY_NONE_VALUE,                        \
        .rx_dma_enabled = false,                                \
        .tx_dma_enabled = false,                                \
        .rx_dma_buffer = NULL,                                  \
        .rx_dma_buff_length = 0,                                \
        .raw_tx_buffer = NULL,                                  \
        .raw_tx_buff_length = 0,                                \
        .rx_handler_function = NULL,                            \
        .rx_bytes_before_trigger = FCR_TRIGGER_1_BYTE,          \
        .enable_chip_interrupts = true,                         \
        .interrupt_priority = TR_HAL_INTERRUPT_PRIORITY_5,      \
        .wake_on_interrupt = false,                             \
        .event_handler_fx = NULL,                               \
        .run_when_sleeping = false,                             \
    }


/// ***************************************************************************
/// these are the EVENTS that can be received into the UART event handler
/// functions. These are BITMASKs since we can have more than 1 in an event
/// these are what the APP needs to handle in its event_handler_fx
/// ***************************************************************************
#define TR_HAL_UART_EVENT_DMA_TX_COMPLETE       0x00000001
#define TR_HAL_UART_EVENT_DMA_RX_BUFFER_LOW     0x00000002
#define TR_HAL_UART_EVENT_DMA_RX_TO_USER_FX     0x00000004
#define TR_HAL_UART_EVENT_DMA_RX_READY          0x00000008
#define TR_HAL_UART_EVENT_TX_COMPLETE           0x00000010
#define TR_HAL_UART_EVENT_TX_STILL_GOING        0x00000020
#define TR_HAL_UART_EVENT_RX_TO_USER_FX         0x00000040
#define TR_HAL_UART_EVENT_RX_READY              0x00000080
#define TR_HAL_UART_EVENT_RX_ENDED_TO_USER_FX   0x00000100
#define TR_HAL_UART_EVENT_RX_ENDED_NO_DATA      0x00000200
#define TR_HAL_UART_EVENT_RX_MAYBE_READY        0x00000400
#define TR_HAL_UART_EVENT_RX_ERR_OVERRUN        0x00000800
#define TR_HAL_UART_EVENT_RX_ERR_PARITY         0x00001000
#define TR_HAL_UART_EVENT_RX_ERR_FRAMING        0x00002000
#define TR_HAL_UART_EVENT_RX_ERR_BREAK          0x00004000
#define TR_HAL_UART_EVENT_HW_FLOW_CONTROL       0x00008000
#define TR_HAL_UART_EVENT_UNEXPECTED            0x00010000


/// ****************************************************************************
/// @} // end of tr_hal_T32CZ20
/// ****************************************************************************


#endif // T32CZ20_UART_H_
