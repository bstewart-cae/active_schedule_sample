/// ****************************************************************************
/// @file T32CM11_spi.h
///
/// @brief This is the chip specific include file for T32CM11 SPI Driver
///        note that there is a common include file for this HAL module that
///        contains the APIs (such as the init function) that should be used
///        by the application.
///
/// this chip supports 2 SPIs. It supports standard SPI, and Dual and Quad SPI.
/// see below for more information on standard SPI vs Dual SPI vs Quad SPI
///
///
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************

#ifndef T32CM11_SPI_H_
#define T32CM11_SPI_H_

#include "tr_hal_platform.h"

/// ****************************************************************************
/// @defgroup tr_hal_spi_cm11 SPI CM11
/// @ingroup tr_hal_T32CM11
/// @{
/// ****************************************************************************


#define TR_HAL_NUM_SPI 2

// SPI IDs
typedef enum
{
    SPI_0_ID = 0,
    SPI_1_ID = 1,

} tr_hal_spi_id_t;

/// *****************************************************************************
/// \brief Normal SPI vs Dual SPI vs Quad SPI modes
/// *****************************************************************************
/// Normal SPI aka Standard SPI
///
/// SPI is used for synchronous serial communication, between a Controller (device
/// that controls which device uses the bus) and a number of Peripherals, devices
/// that use the bus.
/// SPI uses at least 4 wires:
///     CS = chip select, active low, to allow a peripheral to communicate
///     SCLK  = clock signal, from Controller
///     SDO = serial data out / PICO = periperal in, controller out (data goes from Controller to Peripheral)
///     SDI = serial data in / POCI = peripheral out, controller in (data goes from Peripheral to Controller)
/// one device is the Controller, it has at least one Peripheral
/// if there is more than 1 peripheral there should be a different CS line
/// for each peripheral, but common bus for the other 3 pins. This way the
/// Controller can pull the CS line low to a particular peripheral and only
/// that peripheral will use the bus
///
/// *****************************************************************************
/// Dual SPI
///
/// Dual SPI is a mode of operation that can double the transfer rate by using
/// both the SDO and SDI data pins to send two bits per clock cycle (instead of
/// just one), which is twice as fast as normal SPI. The SDO line is now called
/// IO0 and the SDI line is called IO1.
///
/// *****************************************************************************
/// Quad SPI
///
/// Quad SPI is a mode of operation that quadruples the throughput by using
/// both SDO (now IO0) and SDI (now IO1) lines and two additional data lines.
/// The additional data lines are called IO2, and IO3.
///
/// *****************************************************************************

// this is used to set the SPI mode
// this enum uses the same values as the chip registers (section 7.4.4)
typedef enum
{
    TR_HAL_SPI_MODE_NORMAL = 0,
    TR_HAL_SPI_MODE_DUAL   = 2,
    TR_HAL_SPI_MODE_QUAD   = 3,

} tr_hal_spi_mode_t;


// used to set the data transfer amount - 8 bits or 32 bits
// this enum uses the same values as the chip registers (section 7.4.4)
typedef enum
{
    TR_HAL_SPI_BIT_SIZE_8  = 0x10,
    TR_HAL_SPI_BIT_SIZE_32 = 0x70,

} tr_hal_spi_bit_size_t;

// used to set the RX high watermark
// this enum uses the same values as the chip registers (section 7.4.3)
typedef enum
{
    TR_HAL_SPI_RX_WATERMARK_LEVEL_8  = 0x1000,
    TR_HAL_SPI_RX_WATERMARK_LEVEL_16 = 0x2000,
    TR_HAL_SPI_RX_WATERMARK_LEVEL_24 = 0x3000,

} tr_hal_spi_rx_watermark_level_t;

// used to set the TX high watermark
// this enum uses the same values as the chip registers (section 7.4.3)
typedef enum
{
    TR_HAL_SPI_TX_WATERMARK_LEVEL_8  = 0x4000,
    TR_HAL_SPI_TX_WATERMARK_LEVEL_16 = 0x8000,
    TR_HAL_SPI_TX_WATERMARK_LEVEL_24 = 0xC000,

} tr_hal_spi_tx_watermark_level_t;

// FIFO sizes - this is a chip limitation
#define TR_HAL_SPI_TX_FIFO_SIZE 32
#define TR_HAL_SPI_RX_FIFO_SIZE 32

// for setting the peripheral clock
#define SPI0_CLK_BIT 20
#define SPI1_CLK_BIT 21
#define SPI0_CLK_ENABLE_VALUE 0x100000
#define SPI1_CLK_ENABLE_VALUE 0x200000

/// *****************************************************************************
/// valid pins for SPI0 - this chip has 2 SPI peripherals
/// note that pins 10-13, 18-19, and 24-27 are not available and are not included below
/// *****************************************************************************
/// SPI0
///
/// Chip select can be multiple (for multiple peripheral devices)
///    Chip Select 0 - GPIO 7 or GPIO 29 => set to GPIO MODE 1
///    Chip Select 1 - GPIO 0, or GPIO 1, or GPIO 2, or GPIO 3 => set to GPIO MODE 2
///    Chip Select 2 - GPIO 0, or GPIO 1, or GPIO 2, or GPIO 3 => set to GPIO MODE 3
///    Chip Select 3 - GPIO 0, or GPIO 1, or GPIO 2, or GPIO 3 => set to GPIO MODE 6
///
/// SCLK - GPIO 6 or GPIO 28 => set to GPIO MODE 1
///
/// SDO == IO0 - GPIO 8 or GPIO 30 => set to GPIO MODE 1
/// SDI == IO1 - GPIO 9 or GPIO 31 => set to GPIO MODE 1
///        IO2 - GPIO 4 or GPIO 14 => set to GPIO MODE 1
///        IO4 - GPIO 5 or GPIO 15 => set to GPIO MODE 1
/// *****************************************************************************

#define SPI_INVALID_PIN 0xFF

// SPI0 clock
#define SPI0_CLK_PIN_OPTION1 6
#define SPI0_CLK_PIN_OPTION2 28
// SPI0 SDO == IO0
#define SPI0_IO0_PIN_OPTION1 8
#define SPI0_IO0_PIN_OPTION2 30
// SPI0 SDI == IO1
#define SPI0_IO1_PIN_OPTION1 9
#define SPI0_IO1_PIN_OPTION2 31
// SPI0 IO2
#define SPI0_IO2_PIN_OPTION1 4
#define SPI0_IO2_PIN_OPTION2 14
// SPI0 IO3
#define SPI0_IO3_PIN_OPTION1 5
#define SPI0_IO3_PIN_OPTION2 15
// SPI0 chip select0
#define SPI0_MAX_CHIP_SELECT_PINS 4
#define SPI0_CS0_PIN_OPTION1 7
#define SPI0_CS0_PIN_OPTION2 29
// SPI0 chip select1
#define SPI0_CS1_PIN_OPTION1 1
#define SPI0_CS1_PIN_OPTION2 2
#define SPI0_CS1_PIN_OPTION3 3
#define SPI0_CS1_PIN_OPTION4 4
// SPI0 chip select2
#define SPI0_CS2_PIN_OPTION1 1
#define SPI0_CS2_PIN_OPTION2 2
#define SPI0_CS2_PIN_OPTION3 3
#define SPI0_CS2_PIN_OPTION4 4
// SPI0 chip select3
#define SPI0_CS3_PIN_OPTION1 1
#define SPI0_CS3_PIN_OPTION2 2
#define SPI0_CS3_PIN_OPTION3 3
#define SPI0_CS3_PIN_OPTION4 4


/// *****************************************************************************
/// valid pins for SPI1 - this chip has 2 SPI peripherals
/// note that pins 10-13, 18-19, and 24-27 are not available and are not included below
/// *****************************************************************************
/// SPI1
///
/// only one Chip select
/// CS         - GPIO 29 => set to GPIO MODE 5
/// SCLK       - GPIO 28 => set to GPIO MODE 5
/// SDO == IO0 - GPIO 30 => set to GPIO MODE 5
/// SDI == IO1 - GPIO 31 => set to GPIO MODE 5
/// *****************************************************************************

// SPI1 clock
#define SPI1_CLK_PIN_OPTION1 28
// SPI1 SDO == IO0
#define SPI1_IO0_PIN_OPTION1 30
// SPI1 SDI == IO1
#define SPI1_IO1_PIN_OPTION1 31
// SPI1 chip select
#define SPI1_MAX_CHIP_SELECT_PINS 1
#define SPI1_CS0_PIN_OPTION1 29


/// ******************************************************************
/// \brief chip register addresses
/// section 3.1 of the data sheet explains the Memory map.
/// this gives the base address for how to write the chip registers
/// the chip registers are how the software interacts and configures
/// the SPI peripherals. We create a struct below that addresses the
/// individual registers. This makes it so we can use this base address
/// and a struct field to read or write a chip register
/// ******************************************************************
#define CHIP_MEMORY_MAP_SPI0_BASE     (0xB0000000UL)
#define CHIP_MEMORY_MAP_SPI1_BASE     (0x80000000UL)


/// ***************************************************************************
/// \brief the struct we use so we can address registers using field names
/// ***************************************************************************
typedef struct
{
    // when sending data, write it here
    __IO  uint32_t   spi_tx_data;                // 0x00

    // received data comes in from this register
    __I   uint32_t   spi_rx_data;                // 0x04
    __I   uint32_t   reserved1;                  // 0x08

    // configuration of SPI peripheral, incl Controller/Peripheral setting
    __IO  uint32_t   spi_control;                // 0x0C
    __IO  uint32_t   spi_aux_control;            // 0x10

    // status of SPI peripheral
    __I   uint32_t   spi_status;                 // 0x14

    // peripheral select and polarity (only when running as SPI Controller)
    __IO  uint32_t   peripheral_select;          // 0x18
    __IO  uint32_t   peripheral_select_polarity; // 0x1C

    // interrupt enable/status/clear
    __IO  uint32_t   interrupt_enable;           // 0x20
    __I   uint32_t   interrupt_status;           // 0x24
    __IO  int32_t    interrupt_clear;            // 0x28

    // can read the current levels of the TX and RX FIFOs
    __I   uint32_t   tx_fifo_current_level;      // 0x2C
    __I   uint32_t   rx_fifo_current_level;      // 0x30
    __I   uint32_t   reserved2;                  // 0x34

    // if running as SPI Controller: this sets the inter-transfer delay
    __IO  uint32_t   controller_delay_setting;   // 0x38

    // enable and disable the SPI peripheral
    __IO  uint32_t   spi_enable_disable;         // 0x3C
    __IO  uint32_t   reserved3[4];               // 0x40, 0x44, 0x48, 0x4C

    // this is used to change the Controller's clock to a different frequency
    // normally it is 32 MHz, but this can bring it down to 16 Mhz, 8, 5.33, 4, etc
    __IO  uint32_t   controller_clock_divider;   // 0x50
    __IO  uint32_t   reserved4[3];               // 0x54, 0x58, 0x5C

    // setup for DMA receive
    __IO  uint32_t   DMA_rx_buffer_addr;         // 0x60
    __IO  uint32_t   DMA_rx_buffer_len;          // 0x64

    // setup for DMA transmit
    __IO  uint32_t   DMA_tx_buffer_addr;         // 0x68
    __IO  uint32_t   DMA_tx_buffer_len;          // 0x6C

    __I   uint32_t   DMA_rx_xfer_len_remaining;  // 0x70
    __I   uint32_t   DMA_tx_xfer_len_remaining;  // 0x74

    // using DMA - interrupt enable, status, and DMA RX start and DMA TX start
    __IO  uint32_t   DMA_interrupt_enable;       // 0x78
    __IO  uint32_t   DMA_interrupt_status;       // 0x7C
    __IO  uint32_t   DMA_rx_enable;              // 0x80
    __IO  uint32_t   DMA_tx_enable;              // 0x84

} SPI_REGISTERS_T;


// *****************************************************************
// this orients the SPIx_REGISTERS struct with the correct addresses
// so referencing a field will now read/write the correct SPI
// register chip address
#define SPI0_REGISTERS  ((SPI_REGISTERS_T *) CHIP_MEMORY_MAP_SPI0_BASE)
#define SPI1_REGISTERS  ((SPI_REGISTERS_T *) CHIP_MEMORY_MAP_SPI1_BASE)

// *****************************************************************
// helper defines for SPI STATUS REGISTER (0x14)
#define SPI_STATUS_TX_IN_PROGRESS 0x01
#define SPI_STATUS_TX_FIFO_EMPTY  0x04
#define SPI_STATUS_TX_FIFO_WMARK  0x08
#define SPI_STATUS_TX_FIFO_FULL   0x10
#define SPI_STATUS_RX_FIFO_EMPTY  0x20
#define SPI_STATUS_RX_FIFO_WMARK  0x40
#define SPI_STATUS_RX_FIFO_FULL   0x80


// *****************************************************************
// helper defines for SPI CONTROL REGISTER (0x0C)

// bit 0 = continuous transfer
#define SPI_CONTROL_REG_CONTINUOUS_TRANSFER 0x01
// bit 1 = byte swap
#define SPI_CONTROL_REG_BYTE_SWAP           0x02
// bit 2 = MSB first
#define SPI_CONTROL_REG_MSB_FIRST           0x04
// bit 3 = cpha (clock phase)
#define SPI_CONTROL_REG_CPHA_HIGH           0x08
#define SPI_CONTROL_REG_CPHA_LOW            0x00
// bit 4 = cpol (clock polarity)
#define SPI_CONTROL_REG_CPOL_HIGH           0x10
#define SPI_CONTROL_REG_CPOL_LOW            0x00
// bit 5 = set=controller, clear=peripheral
#define SPI_CONTROL_REG_SET_AS_CONTROLLER   0x20
#define SPI_CONTROL_REG_SET_AS_PERIPHERAL   0x00
// bit 6 = sdata0or1, set to 0 for controller and 1 for peripheral
#define SPI_CONTROL_REG_SDATA_FOR_CROSSED   0x40
// bit 11 = enable controller delay
#define SPI_CONTROL_REG_ENABLE_CONTROLLER_DELAY 0x800
// bit 12-13 = receive watermark
#define SPI_CONTROL_REG_RX_WMARK_MASK 0x3000
// bit 14-15 = transmit watermark
#define SPI_CONTROL_REG_TX_WMARK_MASK 0xC000

// *****************************************************************
// helper defines for SPI AUX CONTROL REGISTER (0x10)

// bits 0,1 are SPI MODE
#define SPI_AUX_CTRL_REG_MODE_MASK 0x03

// bit 2 = setting this bit prevents transmitting
#define SPI_AUX_CTRL_REG_PREVENT_TX_BIT 0x04

// bit 3 = setting this bit prevents receiving
// this can be useful to set when transmitting, so you don't receive
// the bytes you just sent
#define SPI_AUX_CTRL_REG_PREVENT_RX_BIT 0x08

// bits 4,5,6 = bitsize
#define SPI_AUX_CTRL_REG_BITSIZE_MASK 0x70

// used to extend the transfer, like if using dual SPI mode and
// wanting the Controller to be able to receive
#define SPI_AUX_CTRL_REG_TRANSFER_EXTEND 0x80


// *****************************************************************
// helper defines for SPI INTERRUPT ENABLE REGISTER (0x20)
// and SPI INTERRUPT STATUS REGISTER (0x24)
// and SPI INTERRUPT CLEAR REGISTER (0x28)
#define SPI_INTERRUPT_TX_EMPTY      0x01
#define SPI_INTERRUPT_TX_WATERMARK  0x02
#define SPI_INTERRUPT_RX_WATERMARK  0x04
#define SPI_INTERRUPT_RX_FULL       0x08
#define SPI_INTERRUPT_TRANSFER_DONE 0x10
#define SPI_INTERRUPT_RX_NOT_EMPTY  0x20
// all and none
#define SPI_INTERRUPT_ALL           0x3F
#define SPI_INTERRUPT_NONE          0x00

// *****************************************************************
// helper defines for SPI PERIPHERAL SELECT REGISTER (0x18)
#define SPI_PERIPH_SELECT_NONE 0x00
#define SPI_PERIPH_SELECT_0    0x01
#define SPI_PERIPH_SELECT_1    0x02
#define SPI_PERIPH_SELECT_2    0x04
#define SPI_PERIPH_SELECT_3    0x08

// *****************************************************************
// helper defines for SPI PERIPHERAL POLARITY REGISTER (0x1C)
#define SPI_PERIPH_SELECT_CONTROLLER_ACTIVE_LOW  0x00
#define SPI_PERIPH_SELECT_CONTROLLER_ACTIVE_HIGH 0x0F

// *****************************************************************
// helper defines for SPI ENABLE DISABLE REGISTER (0x3C)
#define SPI_ENABLE  0x01
#define SPI_DISABLE 0x00

// *****************************************************************
// helper enums for SPI CONTROLLER CLOCK REGISTER (0x50)
// note: this register has two fields that determine clock
// the first field can be OFF (32 MHZ) or ON (bit 8 = 0x100 when set)
// If ON then the 2nd field determines the clock by creating a divider
// for the 32 MHz clock. It uses bits 0 to 7 (a full byte) to determine
// the divider like this: ((setting +1) *2)
// meaning 0  is 0  +1=1  *2 = 2 --> 32 MHz / 2 = 16 MHz
// meaning 1  is 1  +1=2  *2 = 4 --> 32 MHz / 4 =  8 MHz
// meaning 3  is 3  +1=4  *2 = 8 --> 32 MHz / 8 =  4 MHz
// meaning 7  is 7  +1=8  *2 =16 --> 32 MHz / 16=  2 MHz
// meaning 15 is 15 +1=16 *2 =32 --> 32 MHz / 32=  1 MHz
// meaning 31 is 31 +1=32 *2 =64 --> 32 MHz / 64=  500 KHz
// meaning 63 is 63 +1=64 *2 =128--> 32 MHz / 128= 250 KHz
// meaning 127is 127+1=128*2 =256--> 32 MHz / 256= 125 KHz
// etc
// up to 0xFF = 255+1=256*2 = 512 --> 32 MHz / 512 =  1/16th MHz = 62.5 KHz
//
// these enums COMBINES the 2 fields so there is only one enum needed
// to set each clock rate desired in the controller_clock_divider register.
// This set of enums is NOT exhaustive (there would 256)
typedef enum
{
    SPI_CTRL_CLOCK_32_MHZ  = 0x000,
    SPI_CTRL_CLOCK_16_MHZ  = 0x100,
    SPI_CTRL_CLOCK_8_MHZ   = 0x101,
    SPI_CTRL_CLOCK_4_MHZ   = 0x103,
    SPI_CTRL_CLOCK_2_MHZ   = 0x107,
    SPI_CTRL_CLOCK_1_MHZ   = 0x10F,
    SPI_CTRL_CLOCK_500_KHZ = 0x11F,
    SPI_CTRL_CLOCK_250_KHZ = 0x13F,
    SPI_CTRL_CLOCK_125_KHZ = 0x17F,

} tr_hal_spi_clock_rate_t;


// *****************************************************************
// helper defines for SPI DMA INTERRUPT ENABLE REGISTER (0x78)
#define SPI_DMA_INTERRUPTS_DISABLE  0x00
#define SPI_DMA_RX_INTERRUPT_ENABLE 0x01
#define SPI_DMA_TX_INTERRUPT_ENABLE 0x02

// *****************************************************************
// helper defines for SPI DMA INTERRUPT STATUS REGISTER (0x78)
#define SPI_DMA_RX_INTERRUPT_ACTIVE 0x01
#define SPI_DMA_TX_INTERRUPT_ACTIVE 0x02

// *****************************************************************
// helper defines for DMA_rx_enable (0x80) and DMA_tx_enable(0x84) REGISTERs
#define SPI_DMA_ENABLE  0x01
#define SPI_DMA_DISABLE 0x00

// if using DMA for RX we require a minimum for the buffer
#define SPI_DMA_RX_BUFF_MINIMUM_SIZE 16


/// ***************************************************************************
/// if the app wants to directly interface with the chip registers, this is a
/// convenience function for getting the address/struct of a particular SPI
/// so the chip registers can be accessed.
/// ***************************************************************************
SPI_REGISTERS_T* tr_hal_spi_get_register_address(tr_hal_spi_id_t spi_id);


/// ***************************************************************************
/// these are the EVENTS that can be received into the SPI event handler
/// functions. These are BITMASKs since we can have more than 1 in an event
/// these are what the APP needs to handle in its event_handler_fx
/// ***************************************************************************
#define TR_HAL_SPI_EVENT_TX_EMPTY          0x00000001
#define TR_HAL_SPI_EVENT_TX_WMARK          0x00000002
#define TR_HAL_SPI_EVENT_RX_WMARK          0x00000004
#define TR_HAL_SPI_EVENT_RX_FULL           0x00000008
#define TR_HAL_SPI_EVENT_RX_HAS_MORE_DATA  0x00000010
#define TR_HAL_SPI_EVENT_TRANSFER_DONE     0x00000020
#define TR_HAL_SPI_EVENT_RX_TO_USER_FX     0x00000040
#define TR_HAL_SPI_EVENT_RX_READY          0x00000080
#define TR_HAL_SPI_EVENT_DMA_RX_TO_USER_FX 0x00000100
#define TR_HAL_SPI_EVENT_DMA_RX_READY      0x00000200
#define TR_HAL_SPI_EVENT_DMA_TX_COMPLETE   0x00000400


/// ***************************************************************************
/// callbacks from the Trident HAL to the App
/// ***************************************************************************

// prototype for callback from the Trident HAL to the app when a byte is received
typedef void (*tr_hal_spi_receive_callback_t) (uint8_t num_received_bytes, uint8_t* byte_buffer);

// prototype for callback from the Trident HAL to the app when an event happens
typedef void (*tr_hal_spi_event_callback_t) (tr_hal_spi_id_t spi_id, uint32_t event_bitmask);


/// ***************************************************************************
/// SPI settings struct - this is passed to tr_hal_spi_init
///
/// this contains SPI settings
///
/// ***************************************************************************
typedef struct
{
    // **** high level settings ****

    // true=SPI Controller, false=SPI Peripheral
    // (maps to bit 5 in CONTROL register, and also controls bit 6)
    bool run_as_controller;

    // Normal SPI, Dual SPI, or Quad SPI
    tr_hal_spi_mode_t spi_mode;


    // **** pins used ****

    // pins to use for the SPI
    tr_hal_gpio_pin_t clock_pin;
    // this is also known as SDO
    tr_hal_gpio_pin_t io_0_pin;
    // this is also known as SDI
    tr_hal_gpio_pin_t io_1_pin;
    // ony valid if spi_mode=TR_HAL_SPI_MODE_QUAD
    tr_hal_gpio_pin_t io_2_pin;
    tr_hal_gpio_pin_t io_3_pin;

    // chip select - SPI0 can have up to 4
    uint8_t num_chip_select_pins;
    tr_hal_gpio_pin_t chip_select_0;
    tr_hal_gpio_pin_t chip_select_1;
    tr_hal_gpio_pin_t chip_select_2;
    tr_hal_gpio_pin_t chip_select_3;

    // C = SPI Controller, P = SPI Peripheral
    // normally, SDO on C is wired to SDO on P, and SDI on C is wired to SDI on P
    // but sometimes these pins are crossed. if the pins are crossed (SDO wired to SDI)
    // then set this option to TRUE. This controls how the value of sdat0or1 in the
    // control register is set. If sdo_sdi_pins_crossed=T and the device is a Peripheral
    // then the sdat0or1 gets set to 1
    bool sdo_sdi_pins_crossed;


    // **** SPI Clock Settings ****

    // NOTE: SPI mode 0 is most common, this is:
    // CPOL = 0
    // CPHA = 0

    // (CPOL = CLOCK POLARITY) SPI clock rests high? (maps to bit 4 in CONTROL register)
    // true=clock rests high, false=clock rests low
    bool cpol_bit;

    // (CPHA = CLOCK PHASE) first bit on SS (maps to bit 3 in CONTROL register)
    // true=first TX bit as SS is asserted, false=first TX bit on first clk edge after SS
    bool cpha_bit;

    // for the Controller only: set the clock speed
    tr_hal_spi_clock_rate_t controller_clock_rate;


    // **** TX/RX settings ****

    // bit size of TX and RX FIFO
    tr_hal_spi_bit_size_t bit_size;

    // if this is set to false (0) then chip select deasserts after each byte
    // if this is set to true(1) then chip select STAYS asserted until all bytes are done
    // most SPI devices getting multiple bytes will expect CS to remain asserted
    // only matters when run_as_controller=true (maps to bit 0 in CONTROL register)
    bool continuous_transfer;

    // Byte swap in bitsize = 16/32 bit for TX/RX FIFO
    bool byte_swap;

    // (MSB/LSB) true=MSB, false=LSB (maps to bit 2 in CONTROL register)
    bool most_significant_bit_first;

    // do we enable inter transfer delays
    // only matters when run_as_controller=true (maps to bit 11 in CONTROL register)
    bool enable_inter_transfer_delay;

    // if enable_inter_transfer_delay=T, this sets the delay
    uint16_t delay_in_clock_cycles;


    // **** DMA settings ****

    bool     rx_dma_enabled;
    bool     tx_dma_enabled;
    // note: RX buffer must STAY allocated
    uint8_t* rx_dma_buffer;
    uint16_t rx_dma_buff_length;
    // note: TX buffers are passed when transmitting


    // **** non-DMA transmit ****

    // if transmit is not done with DMA, then the app needs to allocate a
    // transmit buffer and set a pointer to that transmit buffer here
    uint8_t* raw_tx_buffer;
    uint16_t raw_tx_buff_length;


    // **** receive and event handler functions ****

    // callback from HAL to App when a byte is received
    // if the app doesn't want this, then set it to NULL
    tr_hal_spi_receive_callback_t rx_handler_function;

    // callback from HAL to App when an event happens
    // if the app doesn't want this, then set it to NULL
    tr_hal_spi_event_callback_t  event_handler_fx;


    // **** chip behavior settings ****


    // are the chip interrupts enabled?
    bool enable_chip_interrupts;

    // set the INT priority
    tr_hal_int_pri_t interrupt_priority;

    // when the device is sleeping, we can choose to DISABLE interrupts,
    // or leave them enabled which would allow the device to wake on
    // an interrupt from this peripheral
    bool wake_on_interrupt;

    // watermark
    tr_hal_spi_tx_watermark_level_t transmit_watermark;
    tr_hal_spi_rx_watermark_level_t receive_watermark;

} tr_hal_spi_settings_t;


/// ***************************************************************************
/// initializer macros for default SPI settings
///
/// it is common to send most significant bit first, so these are set for
/// most_significant_bit_first = true
///
/// as far as clock polarity and phase SPI mode 0 is most common, which is
/// CPOL = 0 and CPHA = 0, which is why cpol_bit and cpha_bit are set to 0
/// in the defaults
///
/// ***************************************************************************

/// SPI settings for normal mode (4 wire) and as a SPI Controller, single CS
/// this represents the most common settings for SPI
/// set run_as_controller=false to run as a SPI Peripheral
/// set num_chip_select_pins and chip_select_X to use more than one CS
/// can adjust controller_clock_rate to be faster if the other SPI device can handle that
#define SPI_CONFIG_CONTROLLER_NORMAL_MODE                          \
    {                                                              \
        .run_as_controller = true,                                 \
        .spi_mode = TR_HAL_SPI_MODE_NORMAL,                        \
        .clock_pin = (tr_hal_gpio_pin_t) { SPI0_CLK_PIN_OPTION1 }, \
        .io_0_pin = (tr_hal_gpio_pin_t) { SPI0_IO0_PIN_OPTION1 },  \
        .io_1_pin = (tr_hal_gpio_pin_t) { SPI0_IO1_PIN_OPTION1 },  \
        .io_2_pin = (tr_hal_gpio_pin_t) { SPI_INVALID_PIN },       \
        .io_3_pin = (tr_hal_gpio_pin_t) { SPI_INVALID_PIN },       \
        .num_chip_select_pins = 1,                                 \
        .chip_select_0 =  (tr_hal_gpio_pin_t) { SPI0_CS0_PIN_OPTION1 },\
        .chip_select_1 = (tr_hal_gpio_pin_t) { SPI_INVALID_PIN },  \
        .chip_select_2 = (tr_hal_gpio_pin_t) { SPI_INVALID_PIN },  \
        .chip_select_3 = (tr_hal_gpio_pin_t) { SPI_INVALID_PIN },  \
        .sdo_sdi_pins_crossed = false,                             \
        .cpol_bit = false,                                         \
        .cpha_bit = false,                                         \
        .controller_clock_rate = SPI_CTRL_CLOCK_1_MHZ,             \
        .bit_size = TR_HAL_SPI_BIT_SIZE_8,                         \
        .continuous_transfer = true,                               \
        .byte_swap = false,                                        \
        .most_significant_bit_first = true,                        \
        .enable_inter_transfer_delay = false,                      \
        .delay_in_clock_cycles = 0,                                \
        .rx_dma_enabled = false,                                   \
        .tx_dma_enabled = false,                                   \
        .rx_dma_buffer = NULL,                                     \
        .rx_dma_buff_length = 0,                                   \
        .raw_tx_buffer = NULL,                                     \
        .raw_tx_buff_length = 0,                                   \
        .rx_handler_function = NULL,                               \
        .event_handler_fx = NULL,                                  \
        .enable_chip_interrupts = true,                            \
        .interrupt_priority = TR_HAL_INTERRUPT_PRIORITY_5,         \
        .wake_on_interrupt = false,                                \
        .transmit_watermark = TR_HAL_SPI_TX_WATERMARK_LEVEL_8,     \
        .receive_watermark = TR_HAL_SPI_RX_WATERMARK_LEVEL_8,      \
    }


/// ***************************************************************************
/// these functions are used by the init/uninit function and should not be
/// needed if the init and uninit functions are used to setup the SPI peripherals
/// ***************************************************************************

// SPI power on/off - these are called from init and uninit - the app should not need to call these
tr_hal_status_t tr_hal_spi_power_off(tr_hal_spi_id_t spi_id);

tr_hal_status_t tr_hal_spi_power_on(tr_hal_spi_id_t spi_id);


/// ***************************************************************************
/// these pin functions are used by the init function and should not be
/// needed if the init functions is used to setup the SPI peripherals
/// ***************************************************************************

// function for setting the pins for a standard SPI
// this also checks that the pin choices are VALID for that particular SPI
tr_hal_status_t tr_hal_spi_set_standard_pins(tr_hal_spi_id_t   spi_id,
                                             tr_hal_gpio_pin_t clk_pin,
                                             tr_hal_gpio_pin_t chip_select_0_pin,
                                             tr_hal_gpio_pin_t sdo_pin,
                                             tr_hal_gpio_pin_t sdi_pin);

// *** Quad SPI mode is not currently supported ***
// function for setting the pins for a quad SPI which requires 2 more pins
// this also checks that the pin choices are VALID for that particular SPI
////tr_hal_status_t tr_hal_spi_set_quad_pins(tr_hal_spi_id_t   spi_id,
////                                         tr_hal_gpio_pin_t clk_pin,
////                                         tr_hal_gpio_pin_t chip_select_pin,
////                                         tr_hal_gpio_pin_t io_0_pin,
////                                         tr_hal_gpio_pin_t io_1_pin,
////                                         tr_hal_gpio_pin_t io_2_pin,
////                                         tr_hal_gpio_pin_t io_3_pin);

// function for setting additional chip select pins beyond the 1 already set
// either tr_hal_spi_set_standard_pins or ..set_quad_pins needs to have been called
// this also checks that the pin choices are VALID for that particular SPI
tr_hal_status_t tr_hal_spi_set_addl_cs_pins(tr_hal_spi_id_t   spi_id,
                                            uint8_t           num_chip_select,
                                            tr_hal_gpio_pin_t chip_select_1_pin,
                                            tr_hal_gpio_pin_t chip_select_2_pin,
                                            tr_hal_gpio_pin_t chip_select_3_pin);


/// ***************************************************************************
/// these are development tools
/// they were used to diagnose and fix an issue. They remain in case they
/// are useful in the future
/// ***************************************************************************
tr_hal_status_t tr_hal_spi_read_stats(tr_hal_spi_id_t spi_id,
                                      uint32_t* transmit_started,
                                      uint32_t* transmit_completed,
                                      uint32_t* bytes_received);

tr_hal_status_t tr_hal_spi_clear_tx_busy(tr_hal_spi_id_t spi_id);


/// ***************************************************************************
// *** these APIs are experimental and have not been proven to work ***
// for sending using DUAL mode. start enables contXfer and sets it for dual mode
// stop puts mode and contXfer back. These modifications happen to the AUX CTRL
// register
/// ***************************************************************************
//// tr_hal_status_t tr_hal_spi_start_dual_spi_receive(tr_hal_spi_id_t spi_id);
//// tr_hal_status_t tr_hal_spi_stop_dual_spi_receive(tr_hal_spi_id_t spi_id);


/// ****************************************************************************
/// @} // end of tr_hal_T32CM11
/// ****************************************************************************


#endif // T32CM11_SPI_H_
