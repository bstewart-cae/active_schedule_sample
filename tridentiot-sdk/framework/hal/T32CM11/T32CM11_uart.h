/// ****************************************************************************
/// @file T32CM11_uart.h
///
/// @brief This is the chip specific include file for T32CM11 UART Driver
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

#ifndef T32CM11_UART_H_
#define T32CM11_UART_H_

#include "tr_hal_platform.h"
#include "tr_hal_gpio.h"

/// ****************************************************************************
/// @defgroup tr_hal_uart_cm11 UART CM11
/// @ingroup tr_hal_T32CM11
/// @{
/// ****************************************************************************


/// ******************************************************************
/// defines used by the UART module
/// ******************************************************************

// the T32CM11 has 3 UARTs available for configuration
// this is a chip constraint - can't change it
#define TR_NUMBER_OF_UARTS 3

// UART IDs
typedef enum
{
    UART_0_ID = 0,
    UART_1_ID = 1,
    UART_2_ID = 2,

} tr_hal_uart_id_t;

// threshhold of bytes left in DMA buffer to notify user
// this is a software setting, it can be adjusted
#define LOW_BYTES_BUFFER_THRESHHOLD 16

// the maximum size buffer the raw tx API can take in to transmit
// this is a software setting, it can be adjusted
// if adjusting, note we keep this x 3 UARTs in RAM
#define MAX_RAW_TX_DATA_BUFFER_SIZE 256

// if using a DMA buffer we enforce a minimum size
// this is a software setting, and current set to equal the FIFO RX size
#define DMA_RX_BUFF_MINIMUM_SIZE 16

// size of transmit FIFO.
// this is a chip constraint - can't change it
#define TX_FIFO_SIZE 16

// default interrupt priority for UART
//#define UART_DEFAULT_INTERRUPT_PRIORITY TR_HAL_INTERRUPT_PRIORITY_5


/// ****************************************************************************
/// Pin information for the 3 UARTs of the T32CM11
///
/// UART valid pin configurations are below
///
/// UART 0
/// ------------------
/// PIN_17 = UART_0_TX
/// PIN_16 = UART_0_RX
///
/// UART 1
/// ------------------
/// PIN_4 or PIN_28 = UART_1_TX    (note PIN_10 is normally a choice but not on this chip)
/// PIN_5 or PIN_29 = UART_1_RX    (note PIN_11 is normally a choice but not on this chip)
///
/// PIN_14 or PIN_20 or PIN_18 = UART_1_RTSN
/// PIN_15 or PIN_21 or PIN_19 = UART_1_CTSN
///
/// UART 2
/// ------------------
/// PIN_6 or PIN_30 = UART_2_TX    (note PIN_12 is normally a choice but not on this chip)
/// PIN_7 or PIN_31 = UART_2_RX    (note PIN_13 is normally a choice but not on this chip)
///
/// ****************************************************************************

#define UART_INVALID_PIN 0xFF

// we define the valid choices for UART TX and RX pins so these can be used
// in other parts of the code.
// any combination of RX and TX works (these are not in PAIRS)

// UART0 has just 1 option
#define UART0_TX_PIN_OPTION1 17
#define UART0_RX_PIN_OPTION1 16

// UART1 normally has 3 options but OPTION2 is not available on this chip
// left these defines in here to prevent confusion - someone trying to figure
// out why these pins are not supported
#define UART1_TX_PIN_OPTION1 4
#define UART1_RX_PIN_OPTION1 5
//#define UART1_TX_PIN_OPTION2 10
//#define UART1_RX_PIN_OPTION2 11
#define UART1_TX_PIN_OPTION3 28
#define UART1_RX_PIN_OPTION3 29

// RTS/CTS pin options
#define UART1_RTS_PIN_OPTION1 14
#define UART1_CTS_PIN_OPTION1 15

#define UART1_RTS_PIN_OPTION2 20
#define UART1_CTS_PIN_OPTION2 21


// UART2 normally has 3 options but OPTION2 is not available on this chip
// left these defines in here to prevent confusion - someone trying to figure
// out why these pins are not supported
#define UART2_TX_PIN_OPTION1 6
#define UART2_RX_PIN_OPTION1 7
//#define UART2_TX_PIN_OPTION2 12
//#define UART2_RX_PIN_OPTION2 13
#define UART2_TX_PIN_OPTION3 30
#define UART2_RX_PIN_OPTION3 31

#define TR_HAL_PIN_NOT_SET 255


/// ******************************************************************
/// section 3.1 of the data sheet explains the Memory map
/// this gives the base address for how to write the UART registers
/// the UART registers are how the software interacts with the UART
/// peripheral. We create a struct below that addresses the individual
/// registers. This makes it so we can use this base address and a
/// struct field to read or write a chip register
/// ******************************************************************
#define CHIP_MEMORY_MAP_UART0_BASE   (0xA0000000UL)
#define CHIP_MEMORY_MAP_UART1_BASE   (0xA0500000UL)
#define CHIP_MEMORY_MAP_UART2_BASE   (0xA0600000UL)


/// ***************************************************************************
/// the struct we use so we can address UART chip registers using field names
/// ***************************************************************************
typedef struct
{
    __IO uint32_t receive_buffer_register;   // 0x00 = RBR
    __IO uint32_t interrupt_enable_register; // 0x04 = IER
    __IO uint32_t FIFO_control_register;     // 0x08 = FCR
    __IO uint32_t line_control_register;     // 0x0C = LCR
    __IO uint32_t modem_control_register;    // 0x10 = MCR
    __I  uint32_t line_status_register;      // 0x14 = LSR
    __I  uint32_t modem_status_register;     // 0x18 = MSR
    __IO uint32_t scratch_register;          // 0x1C = SCR (not used)

    // setup for DMA RX
    __IO uint32_t DMA_rx_buffer_addr;  //0x20
    __IO uint32_t DMA_rx_buffer_len;   //0x24

    // setup for DMA TX
    __IO uint32_t DMA_tx_buffer_addr;  //0x28
    __IO uint32_t DMA_tx_buffer_len;   //0x2C

    // using DMA
    __I  uint32_t DMA_rx_xfer_len_remaining;      //0x30
    __I  uint32_t DMA_tx_xfer_len_remaining;      //0x34
    __IO uint32_t DMA_interrupt_enable_register; //0x38
    __IO uint32_t DMA_interrupt_status;          //0x3C
    __IO uint32_t DMA_rx_enable;                 //0x40
    __IO uint32_t DMA_tx_enable;                 //0x44

} UART_REGISTERS_T;


// *****************************************************************
// *** some registers are multi-purpose:

// at register address 0x00, a read pulls data from RBR (read-buffer),
// while a write puts a byte in the THR (transmitter-holder)
#define transmitter_holding_register receive_buffer_register

// at register address 0x08, a read pulls data from IIR(interrupt-identification),
// while a write puts a byte in the FCR (FIFO control)
#define interrupt_identification_register FIFO_control_register

// the divisor latch low byte is set in register 0x00, same as RBR and THR
// to write this the LCR must have the divisor latch access bit set
// if that is not set, then we are setting a transmit byte
#define divisor_latch_LSB receive_buffer_register

// the divisor latch low byte is set in register 0x04, same as IER
// to write this the LCR must have the divisor latch access bit set
// if that is not set, then we are setting the interrupt enable register
#define divisor_latch_MSB interrupt_enable_register


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

// *****************************************************************
// these defines help when dealing with the FIFO CONTROL REGISTER (0x08)
#define FCR_FIFO_ENABLE     0x01
#define FCR_CLEAR_RECEIVER  0x02
#define FCR_CLEAR_TRANSMIT  0x04
#define FCR_DMA_SELECT      0x08

#define FCR_TRIGGER_MASK     0xC0

// values we can trigger on
typedef enum
{
    FCR_TRIGGER_1_BYTE   = 0x00,
    FCR_TRIGGER_4_BYTES  = 0x40,
    FCR_TRIGGER_8_BYTES  = 0x80,
    FCR_TRIGGER_14_BYTES = 0xC0,
    FCR_NO_TRIGGER       = 0xFF,

} tr_hal_fifo_trigger_t;


// *****************************************************************
// these defines help when dealing with the INTERRUPT IDENTIFICATION REGISTER (0x08)
// we have an interrupt
// need to mask out the top 4 bits as only the bottom 4 bits are the interrupt
#define IIR_INTERRUPT_MASK     0x0F
// all of these assume that the MASK has already been applied.
// we can only get one of these at a time so we can compare by VALUE and not BIT
#define IIR_NO_INTERRUPT_PENDING     0x01
#define IIR_MODEM_STATUS_INTERRUPT   0x00
#define IIR_THR_EMPTY_INTERRUPT      0x02
#define IIR_RECEIVER_ERROR_INTERRUPT 0x06
#define IIR_RX_DATA_AVAIL_INTERRUPT  0x04
#define IIR_CHAR_TIMEOUT_INTERRUPT   0x0C

// *****************************************************************
// these defines to help when dealing with the LINE CONTROL REGISTER (0x0C)
// bits 0, 1 set the data bits: 00=5, 01=6, 10=7, 11=8
// bit 2 sets the stop bits: 0=1 stop bit, 1=2 stop bits
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

// bits 0, 1 set the data-bits: 0b00=5 data-bits, 0b01=6 data-bits, 0b10=7 data-bits, 0b11=8 data-bits
// these values come from the chip register and cannot be changed
typedef enum
{
    LCR_DATA_BITS_5_VALUE       = 0x00,
    LCR_DATA_BITS_6_VALUE       = 0x01,
    LCR_DATA_BITS_7_VALUE       = 0x02,
    LCR_DATA_BITS_8_VALUE       = 0x03,
    LCR_DATA_BITS_INVALID_VALUE = 0xFF,

} tr_hal_data_bits_t;


// bit 2 sets the stop bits: 0b0=1 stop bit, 0b1=2 stop bits
// these values come from the chip register and cannot be changed
typedef enum
{
    LCR_STOP_BITS_ONE_VALUE     = 0x00,
    LCR_STOP_BITS_TWO_VALUE     = 0x04,
    LCR_STOP_BITS_INVALID_VALUE = 0xFF,

} tr_hal_stop_bits_t;


// bit 3, 4 set the parity: 0b00=no parity 0b10=no parity, 0b01=odd parity, 0b11=even parity,
// these values come from the chip register and cannot be changed
typedef enum
{
    LCR_PARITY_NONE_VALUE    = 0x00,
    LCR_PARITY_ODD_VALUE     = 0x08,
    LCR_PARITY_EVEN_VALUE    = 0x16,
    LCR_PARITY_INVALID_VALUE = 0xFF,

} tr_hal_parity_t;


// *****************************************************************
// helper enum for MODEM CONTROL REGISTER (0x10)
// the modem control register controls the HW flow control
typedef enum
{
    MCR_NO_FLOW_CONTROL_VALUE = 0x00,
    MCR_SET_DTR_READY         = 0x01,
    MCR_SET_RTS_READY         = 0x02,
    MCR_SET_CTS_ENABLED       = 0x20,

} tr_hal_hw_fc_t;


// *****************************************************************
// helper defines for LINE STATUS REGISTER (0x14)
// see section 10.3.8
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
// bit 7 is FIFO error
#define LSR_FIFO_ERROR 0x80

// *****************************************************************
// helper defines for MODEM STATUS REGISTER (0x18)
#define MSR_DCTS 0x01
#define MSR_DDSR 0x02
#define MSR_TERI 0x04
#define MSR_DDCD 0x08
#define MSR_CTS  0x10
#define MSR_DSR  0x20
#define MSR_RI   0x40
#define MSR_DCD  0x80

// *****************************************************************
// helper defines for DMA INTERRUPT ENABLE REGISTER (0x38)
#define DMA_IER_ENABLE_RECEIVE_INT  0x01
#define DMA_IER_ENABLE_TRANSMIT_INT 0x02

// *****************************************************************
// helper defines for DMA INTERRUPT STATUS (0x3C)
#define DMA_RECEIVE_INTERRUPT  0x01
#define DMA_TRANSMIT_INTERRUPT 0x02

// *****************************************************************
// helper defines for DMA_rx_enable (0x40) and DMA_tx_enable(0x44) REGISTERs
#define UART_DMA_ENABLE  0x01
#define UART_DMA_DISABLE 0x00


// *****************************************************************
// baud rates - these are chip specific and cannot be changed
typedef enum
{
    TR_HAL_UART_BAUD_RATE_2400  = 1667,
    TR_HAL_UART_BAUD_RATE_4800  = 833,
    TR_HAL_UART_BAUD_RATE_9600  = 417,
    TR_HAL_UART_BAUD_RATE_14400  = 278,
    TR_HAL_UART_BAUD_RATE_19200  = 208,
    TR_HAL_UART_BAUD_RATE_28800  = 139,
    TR_HAL_UART_BAUD_RATE_38400  = 104,
    TR_HAL_UART_BAUD_RATE_57600  = 69,
    TR_HAL_UART_BAUD_RATE_76800  = 52,
    TR_HAL_UART_BAUD_RATE_115200  = 35,
    TR_HAL_UART_BAUD_RATE_500000  = 8,
    TR_HAL_UART_BAUD_RATE_1000000  = 4,
    TR_HAL_UART_BAUD_RATE_2000000  = 2,
    TR_HAL_UART_BAUD_RATE_ERROR  = 1,

} tr_hal_baud_rate_t;


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
///     baud rate
///     data bits
///     stop bits
///     parity
///     flow control
/// this contains DMA settings for the UART:
///     DMA enabled for receive
///     DMA enabled for transmit
///     initial DMA receive buffer (if DMA is enabled for receive)
///     initial DMA receive buffer length (if DMA is enabled for receive)
/// this contains settings for the user handler function for the UART:
///     do we have a user handler (callback) for when bytes are received
///     if we have a user handler, give a pointer to the function
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
        .tx_pin = (tr_hal_gpio_pin_t) { UART0_TX_PIN_OPTION1 }, \
        .rx_pin = (tr_hal_gpio_pin_t) { UART0_RX_PIN_OPTION1 }, \
        .hardware_flow_control_enabled = false,                 \
        .rts_pin = (tr_hal_gpio_pin_t) { TR_HAL_PIN_NOT_SET },  \
        .cts_pin = (tr_hal_gpio_pin_t) { TR_HAL_PIN_NOT_SET },  \
        .baud_rate = TR_HAL_UART_BAUD_RATE_115200,              \
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
    }

#define DEFAULT_UART1_CONFIG                                    \
    {                                                           \
        .tx_pin = (tr_hal_gpio_pin_t) { UART1_TX_PIN_OPTION1 }, \
        .rx_pin = (tr_hal_gpio_pin_t) { UART1_RX_PIN_OPTION1 }, \
        .hardware_flow_control_enabled = false,                 \
        .rts_pin = (tr_hal_gpio_pin_t) { TR_HAL_PIN_NOT_SET },  \
        .cts_pin = (tr_hal_gpio_pin_t) { TR_HAL_PIN_NOT_SET },  \
        .baud_rate = TR_HAL_UART_BAUD_RATE_115200,              \
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
    }

#define DEFAULT_UART2_CONFIG                                    \
    {                                                           \
        .tx_pin = (tr_hal_gpio_pin_t) { UART2_TX_PIN_OPTION1 }, \
        .rx_pin = (tr_hal_gpio_pin_t) { UART2_RX_PIN_OPTION1 }, \
        .hardware_flow_control_enabled = false,                 \
        .rts_pin = (tr_hal_gpio_pin_t) { TR_HAL_PIN_NOT_SET },  \
        .cts_pin = (tr_hal_gpio_pin_t) { TR_HAL_PIN_NOT_SET },  \
        .baud_rate = TR_HAL_UART_BAUD_RATE_115200,              \
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
/// @} // end of tr_hal_T32CM11
/// ****************************************************************************


#endif // T32CM11_UART_H_
