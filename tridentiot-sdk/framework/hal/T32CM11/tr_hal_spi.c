/// ****************************************************************************
/// @file tr_hal_spi.c
///
/// @brief This contains the code for the Trident HAL SPI for T32CM11
///
///
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************

#include "tr_hal_spi.h"
#include "string.h"

// variable to keep track of if SPI has been initialized successfully
bool g_spi_init_completed[TR_HAL_NUM_SPI] = {false, false};

// internal data structs to keep track of the SPI settings
// these are only valid if g_spi_init_completed=true for this SPI
tr_hal_spi_settings_t g_current_spi_settings[TR_HAL_NUM_SPI];

// keep track of when we are transmitting
bool g_transmitting_active[TR_HAL_NUM_SPI];

// keep track of when we are transmitting with DMA (we need to disable when done)
bool g_dma_tx_active[TR_HAL_NUM_SPI];

// counter for what byte index we have already read in the DMA RX buffer
uint16_t g_curr_spi_dma_rx_index[TR_HAL_NUM_SPI];

// we sometimes need to set the AUX CTRL register to prevent hearing our
// own sent packets. In this case it is helpful to know what the setting
// is supposed to be (since we have to keep this settings plus the one
// bit we change
uint32_t spi_remember_aux_ctrl_setting[TR_HAL_NUM_SPI];

// on raw TX commands, we can be asked to send more bytes than the TX FIFO
// can handle. In these cases, we store the bytes to be sent in the
// SPI settings raw_tx_buffer. These variables help keep track of
// what we still need to send
static uint16_t g_bytes_sent[TR_HAL_NUM_SPI] = {0,0};
static uint16_t g_bytes_remaining[TR_HAL_NUM_SPI] = {0,0};


// forward declare
static void hal_spi_internal_tx_more_bytes(tr_hal_spi_id_t spi_id);
static uint8_t hal_spi_internal_handle_rx_bytes(tr_hal_spi_id_t spi_id);
static void hal_spi_internal_handle_rx_dma_bytes(tr_hal_spi_id_t spi_id);

// we count the number of TX started and RX started for each SPI
// in testing we occasinally see a missed INT for TX_COMPLETE which
// then makes the SPI stop working since it thinks there is a TX
// in progress, if these counts are the same then TX is not in progress
static uint32_t g_count_tx_started[TR_HAL_NUM_SPI] = {0,0};
static uint32_t g_count_tx_finished[TR_HAL_NUM_SPI] = {0,0};

// bytes received since init of the SPI
static uint32_t g_count_rx_bytes[TR_HAL_NUM_SPI] = {0,0};

/// ***************************************************************************
/// tr_hal_spi_get_register_address
/// ***************************************************************************
SPI_REGISTERS_T* tr_hal_spi_get_register_address(tr_hal_spi_id_t spi_id)
{
    if      (spi_id == SPI_0_ID) { return SPI0_REGISTERS;}
    else if (spi_id == SPI_1_ID) { return SPI1_REGISTERS;}

    return NULL;
}


/// ***************************************************************************
/// check_spi_is_ready_for_tx_or_rx
///
/// spi_id must be valid
/// spi_id must be already initialized
/// spi_id must not be in the middle of another transmit
/// ***************************************************************************
static tr_hal_status_t check_spi_is_ready_for_tx_or_rx(tr_hal_spi_id_t spi_id)
{
    // error check the spi ID
    if (spi_id >= TR_HAL_NUM_SPI)
    {
        return TR_HAL_INVALID_SPI_ID;
    }

    // ready also means initialized
    if (g_spi_init_completed[spi_id] == false)
    {
        return TR_HAL_ERROR_NOT_INITIALIZED;
    }

    // we can't be in the middle of a transmit
    if (g_transmitting_active[spi_id] == true)
    {
        // if this was a false alarm, clear the flag and return SUCCESS
        if (g_count_tx_finished[spi_id] >= g_count_tx_started[spi_id])
        {
            g_transmitting_active[spi_id] = false;
            return TR_HAL_SUCCESS;
        }

        return TR_HAL_TRANSMITTER_BUSY;
    }

    return TR_HAL_SUCCESS;
}



/// ***************************************************************************
/// check_spi_chip_select_is_valid
/// ***************************************************************************
static tr_hal_status_t check_spi_chip_select_is_valid(tr_hal_spi_id_t spi_id,
                                                      uint8_t         chip_select_index_to_use)
{
    // SPI1 can only use CS 0
    if (spi_id == SPI_1_ID)
    {
        // 0 is the only valid CS for SPI1
        if (chip_select_index_to_use == 0)
        {
            return TR_HAL_SUCCESS;
        }
        else
        {
            return TR_HAL_SPI_INVALID_CS_INDEX;
        }
    }
    // SPI0 can use up to 3 CS but they have to be configured
    else if (spi_id == SPI_0_ID)
    {
        // get the SPI settings
        tr_hal_spi_settings_t* spi_settings = &(g_current_spi_settings[spi_id]);

        uint8_t num_cs_set = spi_settings->num_chip_select_pins;

        // the index of the one we want to use has to be lower than the number configured
        if (chip_select_index_to_use < num_cs_set)
        {
            return TR_HAL_SUCCESS;
        }
        else
        {
            return TR_HAL_SPI_INVALID_CS_INDEX;
        }
    }
    // don't think we can get this far with a bad SPI ID, but checking just in case
    // also don't want to return a misleading error code
    else
    {
        return TR_HAL_INVALID_SPI_ID;
    }
}


/// ***************************************************************************
/// get_chip_select_bit_from_index
/// ***************************************************************************
uint32_t get_chip_select_setting_from_index(uint8_t chip_select_index_to_use)
{
    switch (chip_select_index_to_use)
    {
        case 0: return SPI_PERIPH_SELECT_0;
        case 1: return SPI_PERIPH_SELECT_1;
        case 2: return SPI_PERIPH_SELECT_2;
        case 3: return SPI_PERIPH_SELECT_3;
    }
    // this shouldn't happen due to error checks before this is called
    return 0;
}


/// ***************************************************************************
/// enable_spi_peripheral_clock
/// ***************************************************************************
static void enable_spi_peripheral_clock(tr_hal_spi_id_t spi_id)
{
    if (spi_id == SPI_0_ID)
    {
        enter_critical_section();
        SYS_CTRL_CHIP_REGISTERS->system_clock_control |= (1 << SPI0_CLK_BIT) ;
        leave_critical_section();
    }
    else if (spi_id == SPI_1_ID)
    {
        enter_critical_section();
        SYS_CTRL_CHIP_REGISTERS->system_clock_control |= (1 << SPI1_CLK_BIT) ;
        leave_critical_section();
    }
    // else do nothing
}


/// ***************************************************************************
/// disable_peripheral_clock
/// ***************************************************************************
static void disable_spi_peripheral_clock(tr_hal_spi_id_t spi_id)
{
    if (spi_id == SPI_0_ID)
    {
        enter_critical_section();
        SYS_CTRL_CHIP_REGISTERS->system_clock_control &= (uint32_t)(~(1 << SPI0_CLK_BIT)) ;
        leave_critical_section();
    }
    else if (spi_id == SPI_1_ID)
    {
        enter_critical_section();
        SYS_CTRL_CHIP_REGISTERS->system_clock_control &= (uint32_t)(~(1 << SPI1_CLK_BIT)) ;
        leave_critical_section();
    }
    // else do nothing
}


/// ***************************************************************************
/// check_spi_settings_valid
/// helper for tr_hal_spi_init
/// ***************************************************************************
static tr_hal_status_t check_spi_settings_valid(tr_hal_spi_id_t        spi_id,
                                                tr_hal_spi_settings_t* spi_settings)
{
    // if spi_id is out of bounds then abort
    if (spi_id >= TR_HAL_NUM_SPI)
    {
        return TR_HAL_INVALID_SPI_ID;
    }

    // if SPI has already been initialized then error
    if (g_spi_init_completed[spi_id])
    {
        return TR_HAL_ERROR_ALREADY_INITIALIZED;
    }

    // don't need to check pins as these are checked as part of settings them

    // don't need to check the boolean run_as_controller since both
    // true (controller) and false (peripheral) are valid

    // check spi_mode is valid
    if (   (spi_settings->spi_mode != TR_HAL_SPI_MODE_NORMAL)
        && (spi_settings->spi_mode != TR_HAL_SPI_MODE_DUAL)
        && (spi_settings->spi_mode != TR_HAL_SPI_MODE_QUAD) )
    {
        return TR_HAL_SPI_UNSUPPORTED_MODE;
    }

    // check watermark fields valid

    if (   (spi_settings->transmit_watermark != TR_HAL_SPI_TX_WATERMARK_LEVEL_8)
        && (spi_settings->transmit_watermark != TR_HAL_SPI_TX_WATERMARK_LEVEL_16)
        && (spi_settings->transmit_watermark != TR_HAL_SPI_TX_WATERMARK_LEVEL_24) )
    {
        return TR_HAL_SPI_UNSUPPORTED_WMARK;
    }
    if (   (spi_settings->receive_watermark != TR_HAL_SPI_RX_WATERMARK_LEVEL_8)
        && (spi_settings->receive_watermark != TR_HAL_SPI_RX_WATERMARK_LEVEL_16)
        && (spi_settings->receive_watermark != TR_HAL_SPI_RX_WATERMARK_LEVEL_24) )
    {
        return TR_HAL_SPI_UNSUPPORTED_WMARK;
    }

    return TR_HAL_SUCCESS;
}


/// ***************************************************************************
/// init SPI peripheral
/// ***************************************************************************
tr_hal_status_t tr_hal_spi_init(tr_hal_spi_id_t        spi_id,
                                tr_hal_spi_settings_t* spi_settings)
{
    tr_hal_status_t status;

    // check that the values in settings are valid
    // this also checks that spi_id is valid and settings is not NULL
    status = check_spi_settings_valid(spi_id, spi_settings);
    if (status != TR_HAL_SUCCESS)
    {
        return status;
    }

    // valid settings, so get the spi register address
    SPI_REGISTERS_T* spi_register_address = tr_hal_spi_get_register_address(spi_id);


    // ********************************
    // set the SPI pins
    // ********************************

    // we set the pins based on standard/dual or quad - quad uses 2 more pins
    // yes, SPI1 does not support quad. we could check that here but it is already
    // checked in the functions below, so no need to
    if (spi_settings->spi_mode == TR_HAL_SPI_MODE_QUAD)
    {
        // *** quad SPI is not yet supported ***
        status = TR_HAL_SPI_UNSUPPORTED_MODE;
        ////status = tr_hal_spi_set_quad_pins(spi_id,
        ////                                  spi_settings->clock_pin,
        ////                                  spi_settings->chip_select_0,
        ////                                  spi_settings->io_0_pin,
        ////                                  spi_settings->io_1_pin,
        ////                                  spi_settings->io_2_pin,
        ////                                  spi_settings->io_3_pin);
        // on error we abort
        if (status != TR_HAL_SUCCESS)
        {
            return status;
        }
    }

    else if (spi_settings->spi_mode == TR_HAL_SPI_MODE_DUAL)
    {
        // *** Dual SPI is not yet supported ***
        status = TR_HAL_SPI_UNSUPPORTED_MODE;
    }

    // this else case is when:
    //     (spi_settings->spi_mode == TR_HAL_SPI_MODE_NORMAL)
    // it was checked in the check_spi_settings_valid() function that one of these 3 was set
    else
    {
        status = tr_hal_spi_set_standard_pins(spi_id,
                                          spi_settings->clock_pin,
                                          spi_settings->chip_select_0,
                                          spi_settings->io_0_pin,
                                          spi_settings->io_1_pin);
        // on error we abort
        if (status != TR_HAL_SUCCESS)
        {
            return status;
        }
    }


    // ********************************
    // set the control register
    // ********************************

    // first set a byte with the correct values
    uint32_t control_register_value = 0;

    // continuous transfer bit (bit 0)
    if ((spi_settings->run_as_controller) && (spi_settings->continuous_transfer))
    {
        control_register_value |= SPI_CONTROL_REG_CONTINUOUS_TRANSFER;
    }

    // byte swap bit (bit 1)
    if (spi_settings->byte_swap)
    {
        control_register_value |= SPI_CONTROL_REG_BYTE_SWAP;
    }

    // MSB first (bit 2)
    if (spi_settings->most_significant_bit_first)
    {
        control_register_value |= SPI_CONTROL_REG_MSB_FIRST;
    }

    // cpha_bit (bit 3)
    if (spi_settings->cpha_bit)
    {
        control_register_value |= SPI_CONTROL_REG_CPHA_HIGH;
    }

    // cpol_bit (bit 4)
    if (spi_settings->cpol_bit)
    {
        control_register_value |= SPI_CONTROL_REG_CPOL_HIGH;
    }

    // controller/peripheral (bit 5) and sdat0or1 (bit 6)
    if (spi_settings->run_as_controller)
    {
        // this sets bit 5 for controller (peripheral is clear bit)
        control_register_value |= SPI_CONTROL_REG_SET_AS_CONTROLLER;
    }
    else
    {
        // if SDO/SDI pins are NOT crossed, then set this option
        // this option should be set for setups where the pins are not crossed
        // (SDO to SDO and SDI to SDI)
        if (spi_settings->sdo_sdi_pins_crossed == false)
        {
            control_register_value |= SPI_CONTROL_REG_SDATA_FOR_CROSSED;
        }
    }

    // enable inter transfer delay (bit 11)
    // only valid for controllers
    if ((spi_settings->run_as_controller) && (spi_settings->enable_inter_transfer_delay))
    {
        control_register_value |= SPI_CONTROL_REG_ENABLE_CONTROLLER_DELAY;

        // set the controller delay setting
        spi_register_address->controller_delay_setting = spi_settings->delay_in_clock_cycles;
    }
    else
    {
        spi_register_address->controller_delay_setting = 0;
    }

    // set the watermark values
    control_register_value |= spi_settings->transmit_watermark;
    control_register_value |= spi_settings->receive_watermark;

    // write the value to the control register
    spi_register_address->spi_control = control_register_value;

    // ********************************
    // set the auxiliary control register
    // ********************************
    uint32_t aux_ctrl_register_value = 0;

    // set the SPI mode (normal, dual, quad)
    aux_ctrl_register_value |= spi_settings->spi_mode;

    // we don't set inhibitDout or inhibitDin
    // these prevent the SPI writing (inhibitDout) or reading (inhibitDin) data
    // this is when communication is 1 way: not sure if there is a use case for this

    // set bitsize 8 or 32
    aux_ctrl_register_value |= spi_settings->bit_size;

    // we do not set the contXferExtend (bit 7)
    // when this bit is set it keeps Chip Select asserted
    // this is meant for when using Dual and Quad SPI

    // remember this for later when we need to temporarily modify it
    spi_remember_aux_ctrl_setting[spi_id] = aux_ctrl_register_value;

    // write the value to the auxiliary control register
    spi_register_address->spi_aux_control = aux_ctrl_register_value;


    // ********************************
    // set the clock speed if we are the controller
    // ********************************
    if (spi_settings->run_as_controller)
    {
        spi_register_address->controller_clock_divider = spi_settings->controller_clock_rate;
    }


    // ********************************
    // setup DMA
    // ********************************
    uint32_t dma_int_enable = 0;

    // DMA TX gets enabled when we want to send, disabled otherwise
    spi_register_address->DMA_tx_enable = SPI_DMA_DISABLE;

    // start out disabled, it gets enabled below if settings have it on
    spi_register_address->DMA_rx_enable = SPI_DMA_DISABLE;

    // enable DMA TX interrupts if the settings say so
    if (spi_settings->tx_dma_enabled)
    {
        dma_int_enable = SPI_DMA_TX_INTERRUPT_ENABLE;
    }

    // enable DMA RX if the settings say so
    if (spi_settings->rx_dma_enabled)
    {
        // if we are doing DMA RX it must have a buffer
        if (spi_settings->rx_dma_buffer == NULL)
        {
            return TR_HAL_ERROR_DMA_RX_BUFFER_MISSING;
        }
        // buffer can't be less than min size
        if (spi_settings->rx_dma_buff_length < SPI_DMA_RX_BUFF_MINIMUM_SIZE)
        {
            return TR_HAL_ERROR_DMA_RX_BUFF_BAD_LEN;
        }

        // setup the DMA RX buffer from the buffer in the settings
        uint8_t* ptr_buffer = spi_settings->rx_dma_buffer;
        spi_register_address->DMA_rx_buffer_addr = (uint32_t) ptr_buffer;
        spi_register_address->DMA_rx_buffer_len = spi_settings->rx_dma_buff_length;

        // enable DMA RX
        // there was a question if the interrupts needed to be enabled BEFORE
        // the DMA is enabled - that is not the case. The DMA can be enabled
        // first and then the interrupts can be enabled.
        spi_register_address->DMA_rx_enable = SPI_DMA_ENABLE;

        // enable DMA RX interrupts
        dma_int_enable |= SPI_DMA_RX_INTERRUPT_ENABLE;
    }

    // this enables or disables DMA TX and RX interrupts, based on the code above
    if (spi_settings->enable_chip_interrupts)
    {
        spi_register_address->DMA_interrupt_enable = dma_int_enable;
    }

    // ********************************
    // clear some local variable state
    // ********************************
    g_dma_tx_active[spi_id] = false;
    g_transmitting_active[spi_id] = false;
    g_bytes_sent[spi_id] = 0;
    g_bytes_remaining[spi_id] = 0;
    g_curr_spi_dma_rx_index[spi_id] = 0;
    g_count_tx_started[spi_id] = 0;
    g_count_tx_finished[spi_id] = 0;
    g_count_rx_bytes[spi_id] = 0;

    // ********************************
    // interrupt enable and enable periperal clock
    // ********************************
    if (spi_settings->enable_chip_interrupts)
    {
        spi_register_address->interrupt_enable = SPI_INTERRUPT_ALL;
    }

    enable_spi_peripheral_clock(spi_id);

    if (spi_settings->enable_chip_interrupts)
    {
        if (spi_id == SPI_0_ID)
        {
            NVIC_SetPriority(Qspi0_IRQn, spi_settings->interrupt_priority);
            NVIC_EnableIRQ(Qspi0_IRQn);
        }
        else if (spi_id == SPI_1_ID)
        {
            NVIC_SetPriority(Qspi1_IRQn, spi_settings->interrupt_priority);
            NVIC_EnableIRQ(Qspi1_IRQn);
        }
    }


    // ********************************
    // chip select / peripheral select
    // ********************************

    // set polarity so that controller is active low
    spi_register_address->peripheral_select_polarity = SPI_PERIPH_SELECT_CONTROLLER_ACTIVE_LOW;

    // set chip select to controller
    spi_register_address->peripheral_select = SPI_PERIPH_SELECT_NONE;


    // ********************************
    // enable SPI
    // ********************************
    spi_register_address->spi_enable_disable = SPI_ENABLE;

    // init has been completed for this SPI, so copy in the settings
    memcpy(&(g_current_spi_settings[spi_id]), spi_settings, sizeof(tr_hal_spi_settings_t));
    g_spi_init_completed[spi_id] = true;

    return TR_HAL_SUCCESS;
}


/// ***************************************************************************
/// uninit SPI peripheral
/// ***************************************************************************
tr_hal_status_t tr_hal_spi_uninit(tr_hal_spi_id_t spi_id)
{
    // error check the spi ID
    if (spi_id >= TR_HAL_NUM_SPI)
    {
        return TR_HAL_INVALID_SPI_ID;
    }

    // see if it is in the correct state to uninitialize
    if (g_spi_init_completed[spi_id] == false)
    {
        return TR_HAL_ERROR_NOT_INITIALIZED;
    }

    // register address
    SPI_REGISTERS_T* spi_register_address = tr_hal_spi_get_register_address(spi_id);

    // settings for this SPI
    tr_hal_spi_settings_t* spi_settings = &(g_current_spi_settings[spi_id]);

    // disable SPI
    spi_register_address->spi_enable_disable = SPI_DISABLE;

    // set the pins back to normal
    tr_hal_gpio_set_mode(spi_settings->clock_pin, TR_HAL_GPIO_MODE_GPIO);
    tr_hal_gpio_set_mode(spi_settings->chip_select_0, TR_HAL_GPIO_MODE_GPIO);
    tr_hal_gpio_set_mode(spi_settings->io_0_pin, TR_HAL_GPIO_MODE_GPIO);
    tr_hal_gpio_set_mode(spi_settings->io_1_pin, TR_HAL_GPIO_MODE_GPIO);

    // release the GPIOs used
    tr_hal_gpio_mgr_release_gpio(spi_settings->clock_pin);
    tr_hal_gpio_mgr_release_gpio(spi_settings->chip_select_0);
    tr_hal_gpio_mgr_release_gpio(spi_settings->io_0_pin);
    tr_hal_gpio_mgr_release_gpio(spi_settings->io_1_pin);

    // if we are quad, set the extra pins back
    if (spi_settings->spi_mode == TR_HAL_SPI_MODE_QUAD)
    {
        tr_hal_gpio_set_mode(spi_settings->io_2_pin, TR_HAL_GPIO_MODE_GPIO);
        tr_hal_gpio_set_mode(spi_settings->io_3_pin, TR_HAL_GPIO_MODE_GPIO);

        tr_hal_gpio_mgr_release_gpio(spi_settings->io_2_pin);
        tr_hal_gpio_mgr_release_gpio(spi_settings->io_3_pin);
    }

    // if we have extra chip select pins, set them back
    uint8_t num_chip_select = spi_settings->num_chip_select_pins;
    if (num_chip_select > 1)
    {
        tr_hal_gpio_set_mode(spi_settings->chip_select_1, TR_HAL_GPIO_MODE_GPIO);
        tr_hal_gpio_mgr_release_gpio(spi_settings->chip_select_1);
    }
    if (num_chip_select > 2)
    {
        tr_hal_gpio_set_mode(spi_settings->chip_select_2, TR_HAL_GPIO_MODE_GPIO);
        tr_hal_gpio_mgr_release_gpio(spi_settings->chip_select_2);
    }
    if (num_chip_select > 3)
    {
        tr_hal_gpio_set_mode(spi_settings->chip_select_3, TR_HAL_GPIO_MODE_GPIO);
        tr_hal_gpio_mgr_release_gpio(spi_settings->chip_select_3);
    }

    // clear pending interrupts and disable
    if (spi_id == SPI_0_ID)
    {
        NVIC_ClearPendingIRQ(Qspi0_IRQn);
        NVIC_DisableIRQ(Qspi0_IRQn);
    }
    else if (spi_id == SPI_1_ID)
    {
        NVIC_DisableIRQ(Qspi1_IRQn);
        NVIC_ClearPendingIRQ(Qspi1_IRQn);
    }

    // peripheral clock off
    disable_spi_peripheral_clock(spi_id);

    // done
    g_spi_init_completed[spi_id] = false;

    return TR_HAL_SUCCESS;
}


/// ***************************************************************************
/// read SPI settings
///
/// this loads the current SPI settings into the return_spi_settings passed in
/// only works when a SPI has already been initialized
/// reads the values that it can from the chip directly
/// ***************************************************************************
tr_hal_status_t tr_hal_spi_settings_read(tr_hal_spi_id_t        spi_id,
                                         tr_hal_spi_settings_t* return_spi_settings)
{
    // error check the spi ID
    if (spi_id >= TR_HAL_NUM_SPI)
    {
        return TR_HAL_INVALID_SPI_ID;
    }

    // SPI settings can't be NULL, this is what we fill in
    if (return_spi_settings == NULL)
    {
        return TR_HAL_SPI_NULL_SETTINGS;
    }

    // we can only return info for SPIs that are already initialized
    // otherwise we would not have pin info and may just be reading
    // from unset registers
    if (g_spi_init_completed[spi_id] == false)
    {
        return TR_HAL_ERROR_NOT_INITIALIZED;
    }

    // we need access to the chip register and settings for this SPI
    SPI_REGISTERS_T* spi_register_address = tr_hal_spi_get_register_address(spi_id);
    tr_hal_spi_settings_t* saved_spi_settings = &(g_current_spi_settings[spi_id]);

    // first set the pins (can't get this from the chip)
    return_spi_settings->clock_pin = saved_spi_settings->clock_pin;
    return_spi_settings->io_0_pin = saved_spi_settings->io_0_pin;
    return_spi_settings->io_1_pin = saved_spi_settings->io_1_pin;
    return_spi_settings->io_2_pin = saved_spi_settings->io_2_pin;
    return_spi_settings->io_3_pin = saved_spi_settings->io_3_pin;
    return_spi_settings->num_chip_select_pins = saved_spi_settings->num_chip_select_pins;
    return_spi_settings->chip_select_0 = saved_spi_settings->chip_select_0;
    return_spi_settings->chip_select_1 = saved_spi_settings->chip_select_1;
    return_spi_settings->chip_select_2 = saved_spi_settings->chip_select_2;
    return_spi_settings->chip_select_3 = saved_spi_settings->chip_select_3;

    // now read the control register
    uint32_t ctrl_reg = spi_register_address->spi_control;

    // controller / peripheral
    return_spi_settings->run_as_controller = false;
    if (ctrl_reg & SPI_CONTROL_REG_SET_AS_CONTROLLER)
    {
        return_spi_settings->run_as_controller = true;
    }

    // cpha_bit
    return_spi_settings->cpha_bit = false;
    if (ctrl_reg & SPI_CONTROL_REG_CPHA_HIGH)
    {
        return_spi_settings->cpha_bit = true;
    }

    // cpol_bit
    return_spi_settings->cpol_bit = false;
    if (ctrl_reg & SPI_CONTROL_REG_CPOL_HIGH)
    {
        return_spi_settings->cpol_bit = true;
    }

    // byte swap bit
    return_spi_settings->byte_swap = false;
    if (ctrl_reg & SPI_CONTROL_REG_BYTE_SWAP)
    {
        return_spi_settings->byte_swap = true;
    }

    // MSB first
    return_spi_settings->most_significant_bit_first = false;
    if (ctrl_reg & SPI_CONTROL_REG_MSB_FIRST)
    {
        return_spi_settings->most_significant_bit_first = true;
    }

    // continuous transfer bit
    return_spi_settings->continuous_transfer = false;
    if (ctrl_reg & SPI_CONTROL_REG_CONTINUOUS_TRANSFER)
    {
        return_spi_settings->continuous_transfer = true;
    }

    // pins crossed (will only show on Peripheral)
    return_spi_settings->sdo_sdi_pins_crossed = false;
    if (ctrl_reg & SPI_CONTROL_REG_SDATA_FOR_CROSSED)
    {
        return_spi_settings->sdo_sdi_pins_crossed = true;
    }

    // emable inter transfer delay
    return_spi_settings->enable_inter_transfer_delay = false;
    if (ctrl_reg & SPI_CONTROL_REG_ENABLE_CONTROLLER_DELAY)
    {
        return_spi_settings->enable_inter_transfer_delay = true;
    }

    // RX and TX watermark settings from control register
    return_spi_settings->transmit_watermark = ctrl_reg & SPI_CONTROL_REG_RX_WMARK_MASK;
    return_spi_settings->receive_watermark = ctrl_reg & SPI_CONTROL_REG_TX_WMARK_MASK;

    // now read the AUX control register
    uint32_t aux_ctrl_reg = spi_register_address->spi_aux_control;

    // SPI MODE
    return_spi_settings->spi_mode = aux_ctrl_reg & SPI_AUX_CTRL_REG_MODE_MASK;

    // bitsize
    return_spi_settings->bit_size = aux_ctrl_reg & SPI_AUX_CTRL_REG_BITSIZE_MASK;

    // we can get RX DMA settings from the chip
    bool rx_dma_enabled = spi_register_address->DMA_rx_enable;
    if (rx_dma_enabled)
    {
        return_spi_settings->rx_dma_enabled = true;
        return_spi_settings->rx_dma_buffer = (uint8_t*) spi_register_address->DMA_rx_buffer_addr;
        return_spi_settings->rx_dma_buff_length = (uint16_t)(spi_register_address->DMA_rx_buffer_len);
    }
    else
    {
        return_spi_settings->rx_dma_enabled = false;
        return_spi_settings->rx_dma_buffer = 0;
        return_spi_settings->rx_dma_buff_length = 0;
    }



    // we get the TX DMA settings from the saved SPI settings, since we can't get them from the chip
    return_spi_settings->tx_dma_enabled = saved_spi_settings->tx_dma_enabled;

    // raw TX buffer settings also are not from the chip
    return_spi_settings->raw_tx_buffer = saved_spi_settings->raw_tx_buffer;
    return_spi_settings->raw_tx_buff_length = saved_spi_settings->raw_tx_buff_length;

    // handler functions come from saved settings, not the chip
    return_spi_settings->rx_handler_function = saved_spi_settings->rx_handler_function;
    return_spi_settings->event_handler_fx = saved_spi_settings->event_handler_fx;

    // clock rate comes from the chip
    return_spi_settings->controller_clock_rate = spi_register_address->controller_clock_divider;

    // interrupt priority
    if (spi_id == SPI_0_ID)
    {
        return_spi_settings->interrupt_priority = NVIC_GetPriority(Qspi0_IRQn);
    }
    else if (spi_id == SPI_1_ID)
    {
        return_spi_settings->interrupt_priority = NVIC_GetPriority(Qspi1_IRQn);
    }

    uint32_t interrupts_enabled = spi_register_address->interrupt_enable;
    return_spi_settings->enable_chip_interrupts = false;
    if (interrupts_enabled)
    {
        return_spi_settings->enable_chip_interrupts = true;
    }

    // delay setting comes from the chip
    return_spi_settings->delay_in_clock_cycles = (uint16_t)(spi_register_address->controller_delay_setting);

    // read this from the saved settings
    return_spi_settings->wake_on_interrupt = saved_spi_settings->wake_on_interrupt;

    return TR_HAL_SUCCESS;
}


/// ***************************************************************************
/// RECEIVE API: tr_hal_spi_raw_rx_one_byte
/// ***************************************************************************
tr_hal_status_t tr_hal_spi_raw_rx_one_byte(tr_hal_spi_id_t spi_id,
                                           uint8_t*        data)
{
    // make sure the SPI is ready
    tr_hal_status_t status = check_spi_is_ready_for_tx_or_rx(spi_id);
    if (status != TR_HAL_SUCCESS)
    {
        return status;
    }

    // we need access to the chip register for SPI
    SPI_REGISTERS_T* spi_register_address = tr_hal_spi_get_register_address(spi_id);

    // we need access to the settings for this SPI
    tr_hal_spi_settings_t* spi_settings = &(g_current_spi_settings[spi_id]);

    // it is an error to call this when the SPI is not initialized
    if (g_spi_init_completed[spi_id] == false)
    {
        return TR_HAL_ERROR_NOT_INITIALIZED;
    }

    // it is an error to call this when we have defined a user receive callback
    if (spi_settings->rx_handler_function != NULL)
    {
        return TR_HAL_ERROR_RECEIVE_FX_HANDLES_RX;
    }

    // if we have DMA for received bytes then this is not allowed
    if (spi_settings->rx_dma_enabled)
    {
        return TR_HAL_ERROR_DMA_HANDLES_RX;
    }

    // *****************************
    // error checking is done
    // *****************************

    // the easiest way to see if we have RX data to read is to
    // check the RX FIFO level
    uint32_t rx_fifo_level = spi_register_address->rx_fifo_current_level;

    // if no data then return that status
    if (rx_fifo_level == 0)
    {
        return TR_HAL_STATUS_NO_DATA_AVAILABLE;
    }

    // receive data by reading the rx register
    uint32_t data_read = spi_register_address->spi_rx_data;

    // when data is available, we read 4 bytes of data since the
    // register is 4 bytes. The LSB is the data we want. If there
    // was previous data it is in the other 3 bytes. Each new
    // byte read "pushes" the other bytes over. For instance,
    // if we were reading "1234" = 0x31 32 33 34 we would first
    //     read 0x00000031 (this is reading "1" = 0x31)
    // and then 0x00003132 (this is reading "2" = 0x32)
    // and then 0x00313233 (this is reading "3" = 0x33)
    // and then 0x31323334 (this is reading "4" = 0x34)

    // return the LSB
    (*data) = (uint8_t) data_read;
    g_count_rx_bytes[spi_id]++;

    // if we have more than 1 byte to return then we let the caller
    // know that we have more bytes
    if (rx_fifo_level > 1)
    {
        return TR_HAL_STATUS_MORE_BYTES;
    }
    // else let the caller know that we have no more data
    return TR_HAL_STATUS_DONE;
}


/// ***************************************************************************
/// RECEIVE API: tr_hal_spi_raw_rx_available_bytes
/// ***************************************************************************
tr_hal_status_t tr_hal_spi_raw_rx_available_bytes(tr_hal_spi_id_t spi_id,
                                                  char*           bytes,
                                                  uint16_t        buffer_size,
                                                  uint16_t*       num_returned_bytes)
{
    // make sure the SPI is ready
    tr_hal_status_t status = check_spi_is_ready_for_tx_or_rx(spi_id);
    if (status != TR_HAL_SUCCESS)
    {
        return status;
    }

    // we need access to the chip register for SPI
    SPI_REGISTERS_T* spi_register_address = tr_hal_spi_get_register_address(spi_id);

    // we need access to the settings for this SPI
    tr_hal_spi_settings_t* spi_settings = &(g_current_spi_settings[spi_id]);

    // it is an error to call this when the SPI is not initialized
    if (g_spi_init_completed[spi_id] == false)
    {
        return TR_HAL_ERROR_NOT_INITIALIZED;
    }

    // it is an error to call this when we have defined a user receive callback
    if (spi_settings->rx_handler_function != NULL)
    {
        return TR_HAL_ERROR_RECEIVE_FX_HANDLES_RX;
    }

    // if we have DMA for received bytes then this is not allowed
    if (spi_settings->rx_dma_enabled)
    {
        return TR_HAL_ERROR_DMA_HANDLES_RX;
    }

    // *****************************
    // error checking is done
    // *****************************

    // the easiest way to see if we have RX data to read is to
    // check the RX FIFO level
    uint32_t rx_fifo_level = spi_register_address->rx_fifo_current_level;

    // if we have more bytes then the buffer will accommodate we need
    // to only read the bytes that the buffer will hold
    uint8_t bytes_to_read = (uint8_t) rx_fifo_level;
    uint16_t return_code = TR_HAL_STATUS_DONE;
    if (rx_fifo_level > buffer_size)
    {
        bytes_to_read = (uint8_t) buffer_size;
        return_code = TR_HAL_STATUS_MORE_BYTES;
    }

    // read all the bytes
    for (uint8_t index = 0; index < bytes_to_read; index++)
    {
        // receive data
        uint32_t data_read = spi_register_address->spi_rx_data;

        bytes[index] = (uint8_t) data_read;
        g_count_rx_bytes[spi_id]++;
    }

    // set the return buffer length
    (*num_returned_bytes) = bytes_to_read;

    // return based on if there are more bytes to read or not
    return return_code;
}


/// ***************************************************************************
/// TRANSMIT API: tr_hal_spi_raw_tx_one_byte
/// ***************************************************************************
tr_hal_status_t tr_hal_spi_raw_tx_one_byte(tr_hal_spi_id_t spi_id,
                                           uint8_t         chip_select_index_to_use,
                                           char            byte_to_send,
                                           bool            receive_bytes)
{
    // make sure the SPI is ready
    tr_hal_status_t status = check_spi_is_ready_for_tx_or_rx(spi_id);
    if (status != TR_HAL_SUCCESS)
    {
        return status;
    }

    // make sure the CS index is valid
    status = check_spi_chip_select_is_valid(spi_id,
                                            chip_select_index_to_use);
    if (status != TR_HAL_SUCCESS)
    {
        return status;
    }

    SPI_REGISTERS_T* spi_register_address = tr_hal_spi_get_register_address(spi_id);

    // we need access to the settings for this SPI
    tr_hal_spi_settings_t* spi_settings = &(g_current_spi_settings[spi_id]);

    // if we are setup for transmit using DMA then this (raw tx) is not allowed
    if (spi_settings->tx_dma_enabled)
    {
        return TR_HAL_ERROR_DMA_HANDLES_TX;
    }

    // if we do NOT want to receive bytes while we are sending, set the aux_ctrl register
    // this is only valid for Controllers
    if (!receive_bytes)
    {
        // set the AUX CTRL so we don't receive the byte we are sending
        uint32_t new_setting = spi_remember_aux_ctrl_setting[spi_id] | SPI_AUX_CTRL_REG_PREVENT_RX_BIT;
        spi_register_address->spi_aux_control = new_setting;
    }

    // if we are Controller, set the register that picks which Chip Select
    // to use, so the peripheral can get the data
    if (spi_settings->run_as_controller)
    {
        spi_register_address->peripheral_select = get_chip_select_setting_from_index(chip_select_index_to_use);
    }

    // set the transmit flag
    // NOTE: this MUST be BEFORE the code that sends data (spi_register_address->spi_tx_data = x)
    // if this code is after the send, and the send is just ONE byte then the TX_EMPTY interrupt
    // will fire BEFORE the g_transmitting_active flag is set, and then it is never cleared
    // this is why we were seeing issue #303 the SPI was thinking the TX was still active for
    // one byte values
    g_transmitting_active[spi_id] = true;
    g_count_tx_started[spi_id]++;

    // transmit data
    spi_register_address->spi_tx_data = byte_to_send;

    return TR_HAL_SUCCESS;
}


/// ***************************************************************************
/// TRANSMIT API: tr_hal_spi_raw_tx_buffer
/// ***************************************************************************
tr_hal_status_t tr_hal_spi_raw_tx_buffer(tr_hal_spi_id_t spi_id,
                                         uint8_t         chip_select_index_to_use,
                                         char*           bytes_to_send,
                                         uint16_t        num_bytes_to_send,
                                         bool            receive_bytes)
{
    uint16_t index;

    // make sure the SPI is ready
    tr_hal_status_t status = check_spi_is_ready_for_tx_or_rx(spi_id);
    if (status != TR_HAL_SUCCESS)
    {
        return status;
    }

    // make sure the CS index is valid
    status = check_spi_chip_select_is_valid(spi_id,
                                            chip_select_index_to_use);
    if (status != TR_HAL_SUCCESS)
    {
        return status;
    }

    // get the SPI chip register address
    SPI_REGISTERS_T* spi_register_address = tr_hal_spi_get_register_address(spi_id);

    // we need access to the settings for this SPI
    tr_hal_spi_settings_t* spi_settings = &(g_current_spi_settings[spi_id]);

    // if we are setup for transmit using DMA then this (raw tx) is not allowed
    if (spi_settings->tx_dma_enabled)
    {
        return TR_HAL_ERROR_DMA_HANDLES_TX;
    }

    // if we do NOT want to receive bytes while we are sending, set the aux_ctrl register
    // this is only valid for Controllers
    if (!receive_bytes)
    {
        // set the AUX CTRL so we don't receive the byte we are sending
        uint32_t new_setting = spi_remember_aux_ctrl_setting[spi_id] | SPI_AUX_CTRL_REG_PREVENT_RX_BIT;
        spi_register_address->spi_aux_control = new_setting;
    }

    // if the num_bytes_to_send is greater than fits in the TX FIFO then
    // we will need to have a transmit buffer to use
    bool use_tx_buffer = false;
    uint16_t bytes_to_tx_now = num_bytes_to_send;
    if (num_bytes_to_send > TR_HAL_SPI_TX_FIFO_SIZE)
    {
        if (spi_settings->raw_tx_buffer == NULL)
        {
            return TR_HAL_ERROR_RAW_TX_BUFFER_MISSING;
        }
        // make sure the buffer being sent fits in the buffer we have
        if (num_bytes_to_send > spi_settings->raw_tx_buff_length)
        {
            return TR_HAL_ERROR_TX_BUFFER_TOO_LONG;
        }
        // remember if we need to use the tx buffer and multiple sends
        use_tx_buffer = true;
        // we can only send what the FIFO can hold
        bytes_to_tx_now = TR_HAL_SPI_TX_FIFO_SIZE;

        // copy the buffer passed in into the transmit buffer
        uint8_t* transmit_buffer = spi_settings->raw_tx_buffer;
        memcpy(transmit_buffer, bytes_to_send, num_bytes_to_send);
    }

    // if we are Controller, set the register that picks which Chip Select
    // to use, so the peripheral can get the data
    if (spi_settings->run_as_controller)
    {
        spi_register_address->peripheral_select = get_chip_select_setting_from_index(chip_select_index_to_use);
    }

    // set the transmit flag
    // NOTE: this MUST be BEFORE the code that sends data (spi_register_address->spi_tx_data = x)
    // if this code is after the send, and the send is just ONE byte then the TX_EMPTY interrupt
    // will fire BEFORE the g_transmitting_active flag is set, and then it is never cleared
    // this is why we were seeing issue #303 the SPI was thinking the TX was still active for
    // one byte values
    g_transmitting_active[spi_id] = true;
    g_count_tx_started[spi_id]++;

    // send up to what the FIFO can handle
    for (index = 0; index < bytes_to_tx_now; index++)
    {
        // transmit data
        spi_register_address->spi_tx_data = bytes_to_send[index];
    }

    // if we need to send more than what the FIFO can hold, we setup
    // so on the INTERRUPT that the FIFO is empty we can transmit
    // the next chunk
    if (use_tx_buffer)
    {
        // setup the bytes sent and bytes remaining, this tells us how to index
        // into the spi_settings->raw_tx_buffer later when the FIFO is free
        // (g_bytes_sent > 0 triggers it to try to send more)
        g_bytes_sent[spi_id] = TR_HAL_SPI_TX_FIFO_SIZE;
        g_bytes_remaining[spi_id] = num_bytes_to_send - TR_HAL_SPI_TX_FIFO_SIZE;
    }
    else
    {
        // we are not sending more bytes, make sure state is good
        g_bytes_sent[spi_id] = 0;
        g_bytes_remaining[spi_id] = 0;
    }

    return TR_HAL_SUCCESS;
}


/// ***************************************************************************
/// tr_hal_spi_clear_tx_busy
///
/// this clears the error: TR_HAL_TRANSMITTER_BUSY
/// ***************************************************************************
tr_hal_status_t tr_hal_spi_clear_tx_busy(tr_hal_spi_id_t spi_id)
{
    // error check the spi ID
    if (spi_id >= TR_HAL_NUM_SPI)
    {
        return TR_HAL_INVALID_SPI_ID;
    }

    // clear the TX error
    g_transmitting_active[spi_id] = false;

    // correct the counting
    if (g_count_tx_finished[spi_id] < g_count_tx_started[spi_id])
    {
        g_count_tx_finished[spi_id] = g_count_tx_started[spi_id];
    }

    return TR_HAL_SUCCESS;
}


/// ***************************************************************************
/// DMA TRANSMIT API: tr_hal_spi_dma_tx_bytes_in_buffer
/// ***************************************************************************
tr_hal_status_t tr_hal_spi_dma_tx_bytes_in_buffer(tr_hal_spi_id_t spi_id,
                                                  uint8_t         chip_select_index_to_use,
                                                  char*           bytes_to_send,
                                                  uint16_t        num_bytes_to_send,
                                                  bool            receive_bytes)
{
    // make sure the SPI is ready
    tr_hal_status_t status = check_spi_is_ready_for_tx_or_rx(spi_id);
    if (status != TR_HAL_SUCCESS)
    {
        return status;
    }

    // make sure the CS index is valid
    status = check_spi_chip_select_is_valid(spi_id,
                                            chip_select_index_to_use);
    if (status != TR_HAL_SUCCESS)
    {
        return status;
    }

    // get the chip register address and settings
    SPI_REGISTERS_T* spi_register_address = tr_hal_spi_get_register_address(spi_id);
    tr_hal_spi_settings_t* spi_settings = &(g_current_spi_settings[spi_id]);

    // DMA TX must be enabled to call this function
    if (!(spi_settings->tx_dma_enabled))
    {
        return TR_HAL_ERROR_DMA_NOT_ENABLED;
    }

    // if we do NOT want to receive bytes while we are sending, set the aux_ctrl register
    // this is only valid for Controllers
    if (!receive_bytes)
    {
        // set the AUX CTRL so we don't receive the byte we are sending
        uint32_t new_setting = spi_remember_aux_ctrl_setting[spi_id] | SPI_AUX_CTRL_REG_PREVENT_RX_BIT;
        spi_register_address->spi_aux_control = new_setting;
    }

    // if we are Controller, set the register that picks which Chip Select
    // to use, so the peripheral can get the data
    if (spi_settings->run_as_controller)
    {
        spi_register_address->peripheral_select = get_chip_select_setting_from_index(chip_select_index_to_use);
    }

    // set the transmit flag
    // NOTE: this MUST be BEFORE the code that sends data (spi_register_address->DMA_tx_enable = SPI_DMA_ENABLE)
    // if this code is after the send, and the send is just ONE byte then the TX_EMPTY interrupt
    // will fire BEFORE the g_transmitting_active flag is set, and then it is never cleared
    // this is why we were seeing issue #303 the SPI was thinking the TX was still active for
    // one byte values
    g_transmitting_active[spi_id] = true;
    g_dma_tx_active[spi_id] = true;
    g_count_tx_started[spi_id]++;

    // set the new buffer and buffer length
    spi_register_address->DMA_tx_buffer_addr = (uint32_t) bytes_to_send;
    spi_register_address->DMA_tx_buffer_len = num_bytes_to_send;

    // enable the transmission
    spi_register_address->DMA_tx_enable = SPI_DMA_ENABLE;

    return TR_HAL_SUCCESS;
}


/// ***************************************************************************
/// tr_hal_spi_dma_receive_buffer_num_bytes_left
///
/// checks the number of bytes available in the DMA RX buffer
/// ***************************************************************************
tr_hal_status_t tr_hal_spi_dma_receive_buffer_num_bytes_left(tr_hal_spi_id_t spi_id,
                                                             uint32_t*       bytes_left)
{
    // error check the spi ID
    if (spi_id >= TR_HAL_NUM_SPI)
    {
        return TR_HAL_INVALID_SPI_ID;
    }

    // get the chip register address and settings
    SPI_REGISTERS_T* spi_register_address = tr_hal_spi_get_register_address(spi_id);
    tr_hal_spi_settings_t* spi_settings = &(g_current_spi_settings[spi_id]);

    // DMA RX must be enabled to call this function
    if (!(spi_settings->rx_dma_enabled))
    {
        return TR_HAL_ERROR_DMA_NOT_ENABLED;
    }

    *bytes_left = spi_register_address->DMA_rx_xfer_len_remaining;

    return TR_HAL_SUCCESS;
}


/// ***************************************************************************
/// tr_hal_spi_dma_change_rx_buffer
///
/// change the DMA RX buffer if it is getting low
/// ***************************************************************************
tr_hal_status_t tr_hal_spi_dma_change_rx_buffer(tr_hal_spi_id_t spi_id,
                                                uint8_t*        new_receive_buffer,
                                                uint16_t        new_buffer_length)
{
    // make sure the SPI is ready
    tr_hal_status_t status = check_spi_is_ready_for_tx_or_rx(spi_id);
    if (status != TR_HAL_SUCCESS)
    {
        return status;
    }

    // get the chip register address and settings
    SPI_REGISTERS_T* spi_register_address = tr_hal_spi_get_register_address(spi_id);
    tr_hal_spi_settings_t* spi_settings = &(g_current_spi_settings[spi_id]);

    // DMA RX must be enabled to call this function
    if (!(spi_settings->rx_dma_enabled))
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
    if (spi_settings->rx_handler_function != NULL)
    {
        hal_spi_internal_handle_rx_dma_bytes(spi_id);
    }
    // else: we expect the app to have read the buffer before changing
    // if a user_rx function is not defined

    // disable DMA RX
    spi_register_address->DMA_rx_enable = SPI_DMA_DISABLE;

    // set the new buffer and buffer length
    spi_register_address->DMA_rx_buffer_addr = (uint32_t) new_receive_buffer;
    spi_register_address->DMA_rx_buffer_len = new_buffer_length;

    // note: we do NOT need to enable DMA interrupts again here. The chip
    // does remember that they are enabled

    // enable DMA RX
    spi_register_address->DMA_rx_enable = SPI_DMA_ENABLE;

    return TR_HAL_SUCCESS;
}


/// ***************************************************************************
/// tr_hal_spi_power_off
///
/// SPI power off - this is called when it goes to sleep - the app should not
/// need to call these
/// ***************************************************************************
tr_hal_status_t tr_hal_spi_power_off(tr_hal_spi_id_t spi_id)
{
    // error check the spi ID
    if (spi_id >= TR_HAL_NUM_SPI)
    {
        return TR_HAL_INVALID_SPI_ID;
    }

    // must be initialized
    if (g_spi_init_completed[spi_id] == false)
    {
        return TR_HAL_ERROR_NOT_INITIALIZED;
    }

    // we need access to the chip register and settings for this SPI
    SPI_REGISTERS_T* spi_register_address = tr_hal_spi_get_register_address(spi_id);
    tr_hal_spi_settings_t* spi_settings = &(g_current_spi_settings[spi_id]);

    // if we disable interrupts on power off
    if (spi_settings->wake_on_interrupt == false)
    {
        // all DMA int off
        spi_register_address->DMA_interrupt_enable = SPI_DMA_INTERRUPTS_DISABLE;
        // all SPI int off
        spi_register_address->interrupt_enable = SPI_INTERRUPT_ALL;

        // clear pending interrupts and disable
        if (spi_id == SPI_0_ID)
        {
            NVIC_ClearPendingIRQ(Qspi0_IRQn);
            NVIC_DisableIRQ(Qspi0_IRQn);
        }
        else if (spi_id == SPI_1_ID)
        {
            NVIC_DisableIRQ(Qspi1_IRQn);
            NVIC_ClearPendingIRQ(Qspi1_IRQn);
        }

        // peripheral clock off
        disable_spi_peripheral_clock(spi_id);
    }

    return TR_HAL_SUCCESS;
}


/// ***************************************************************************
/// tr_hal_spi_power_on
///
/// SPI power on - this is called after it wakes from sleep - the app should
/// not need to call these
/// ***************************************************************************
tr_hal_status_t tr_hal_spi_power_on(tr_hal_spi_id_t spi_id)
{
    // error check the spi ID
    if (spi_id >= TR_HAL_NUM_SPI)
    {
        return TR_HAL_INVALID_SPI_ID;
    }

    // must be initialized
    if (g_spi_init_completed[spi_id] == false)
    {
        return TR_HAL_ERROR_NOT_INITIALIZED;
    }

    // we need access to the chip register and settings for this SPI
    SPI_REGISTERS_T* spi_register_address = tr_hal_spi_get_register_address(spi_id);
    tr_hal_spi_settings_t* spi_settings = &(g_current_spi_settings[spi_id]);

    // if we disable interrupts on power off, and we want chip interrupts on,
    // we need to turn INTs back on again in power on
    if ( (spi_settings->wake_on_interrupt == false)
        && (spi_settings->enable_chip_interrupts) )
    {
        // all SPI interrupts ON
        spi_register_address->interrupt_enable = SPI_INTERRUPT_NONE;

        // check if we need to enable DMA TX interrupts
        uint32_t dma_int_enable = 0;
        if (spi_settings->tx_dma_enabled)
        {
            dma_int_enable = SPI_DMA_TX_INTERRUPT_ENABLE;
        }

        // check if we need to enable DMA RX interrupts
        if (spi_settings->rx_dma_enabled)
        {
            dma_int_enable |= SPI_DMA_RX_INTERRUPT_ENABLE;
        }
        // enable DMA interrupts
        spi_register_address->DMA_interrupt_enable = dma_int_enable;

        // peripheral clock on
        enable_spi_peripheral_clock(spi_id);

        // make sure INT priority is correct and enable INT
        if (spi_id == SPI_0_ID)
        {
            NVIC_SetPriority(Qspi0_IRQn, spi_settings->interrupt_priority);
            NVIC_EnableIRQ(Qspi0_IRQn);
        }
        else if (spi_id == SPI_1_ID)
        {
            NVIC_SetPriority(Qspi1_IRQn, spi_settings->interrupt_priority);
            NVIC_EnableIRQ(Qspi1_IRQn);
        }
    }

    return TR_HAL_SUCCESS;
}


/// ***************************************************************************
/// hal_spi_0_check_std_pins_valid
/// helper function for tr_hal_spi_set_standard_pins and tr_hal_spi_set_quad_pins
/// ***************************************************************************
static tr_hal_status_t hal_spi_0_check_std_pins_valid(tr_hal_gpio_pin_t clk_pin,
                                                      tr_hal_gpio_pin_t chip_select_pin,
                                                      tr_hal_gpio_pin_t sdo_pin,
                                                      tr_hal_gpio_pin_t sdi_pin)
{
    tr_hal_status_t status;

    // for SPI1, the pins only have 1 options, due to some pins not
    // being available on the chip

    // clock
    if (   (clk_pin.pin != SPI0_CLK_PIN_OPTION1)
        && (clk_pin.pin != SPI0_CLK_PIN_OPTION2) )
    {
        return TR_HAL_SPI_INVALID_CLK_PIN;
    }

    // chip select
    if (   (chip_select_pin.pin != SPI0_CS0_PIN_OPTION1)
        && (chip_select_pin.pin != SPI0_CS0_PIN_OPTION2) )
    {
        return TR_HAL_SPI_INVALID_CS0_PIN;
    }

    // SDO = IO0
    if (   (sdo_pin.pin != SPI0_IO0_PIN_OPTION1)
        && (sdo_pin.pin != SPI0_IO0_PIN_OPTION2) )
    {
        return TR_HAL_SPI_INVALID_IO0_PIN;
    }

    // SDI = IO1
    if (   (sdi_pin.pin != SPI0_IO1_PIN_OPTION1)
        && (sdi_pin.pin != SPI0_IO1_PIN_OPTION2) )
    {
        return TR_HAL_SPI_INVALID_IO1_PIN;
    }

    // check if clock is already configured to another peripheral
    status = tr_hal_gpio_mgr_check_gpio(clk_pin);
    if (status != TR_HAL_SUCCESS)
    {
        return status;
    }

    // check if chip select is already configured to another peripheral
    status = tr_hal_gpio_mgr_check_gpio(chip_select_pin);
    if (status != TR_HAL_SUCCESS)
    {
        return status;
    }

    // check if SDO is already configured to another peripheral
    status = tr_hal_gpio_mgr_check_gpio(sdo_pin);
    if (status != TR_HAL_SUCCESS)
    {
        return status;
    }

    // check if SDI is already configured to another peripheral
    status = tr_hal_gpio_mgr_check_gpio(sdi_pin);
    if (status != TR_HAL_SUCCESS)
    {
        return status;
    }

    return TR_HAL_SUCCESS;
}


/// ***************************************************************************
/// hal_spi_1_check_std_pins_valid
/// helper function for tr_hal_spi_set_standard_pins
/// ***************************************************************************
static tr_hal_status_t hal_spi_1_check_std_pins_valid(tr_hal_gpio_pin_t clk_pin,
                                                      tr_hal_gpio_pin_t chip_select_pin,
                                                      tr_hal_gpio_pin_t sdo_pin,
                                                      tr_hal_gpio_pin_t sdi_pin)
{
    tr_hal_status_t status;

    // for SPI1, the pins only have 1 options, due to some pins not
    // being available on the chip

    // clock
    if (clk_pin.pin != SPI1_CLK_PIN_OPTION1)
    {
        return TR_HAL_SPI_INVALID_CLK_PIN;
    }

    // chip select
    if (chip_select_pin.pin != SPI1_CS0_PIN_OPTION1)
    {
        return TR_HAL_SPI_INVALID_CS0_PIN;
    }

    // SDO = IO0
    if (sdo_pin.pin != SPI1_IO0_PIN_OPTION1)
    {
        return TR_HAL_SPI_INVALID_IO0_PIN;
    }

    // SDI = IO1
    if (sdi_pin.pin != SPI1_IO1_PIN_OPTION1)
    {
        return TR_HAL_SPI_INVALID_IO1_PIN;
    }

    // check if clock is already configured to another peripheral
    status = tr_hal_gpio_mgr_check_gpio(clk_pin);
    if (status != TR_HAL_SUCCESS)
    {
        return status;
    }

    // check if chip select is already configured to another peripheral
    status = tr_hal_gpio_mgr_check_gpio(chip_select_pin);
    if (status != TR_HAL_SUCCESS)
    {
        return status;
    }

    // check if SDO is already configured to another peripheral
    status = tr_hal_gpio_mgr_check_gpio(sdo_pin);
    if (status != TR_HAL_SUCCESS)
    {
        return status;
    }

    // check if SDI is already configured to another peripheral
    status = tr_hal_gpio_mgr_check_gpio(sdi_pin);
    if (status != TR_HAL_SUCCESS)
    {
        return status;
    }

    return TR_HAL_SUCCESS;
}


/// ***************************************************************************
/// function for setting the pins for a standard SPI
/// this also checks that the pin choices are VALID for that particular SPI
/// ***************************************************************************
tr_hal_status_t tr_hal_spi_set_standard_pins(tr_hal_spi_id_t   spi_id,
                                             tr_hal_gpio_pin_t clk_pin,
                                             tr_hal_gpio_pin_t chip_select_0_pin,
                                             tr_hal_gpio_pin_t sdo_pin,
                                             tr_hal_gpio_pin_t sdi_pin)
{
    tr_hal_status_t status;

    // SPI0
    if (spi_id == SPI_0_ID)
    {
        // check if pins are valid
        status = hal_spi_0_check_std_pins_valid(clk_pin,
                                                chip_select_0_pin,
                                                sdo_pin,
                                                sdi_pin);
        // if error, return it
        if (status != TR_HAL_SUCCESS)
        {
            return status;
        }

        // one example explained that: the clock pin drive strength needs
        // to be reduced. But in testing it works with or without this
        // change. Leaving this code in case it is needed later.
        //tr_hal_gpio_set_drive_strength(clk_pin,
        //                               TR_HAL_DRIVE_STRENGTH_14_MA);

        // set the pins
        tr_hal_gpio_set_mode(clk_pin,
                             TR_HAL_GPIO_MODE_SPI0);
        tr_hal_gpio_set_mode(chip_select_0_pin,
                             TR_HAL_GPIO_MODE_SPI0);
        tr_hal_gpio_set_mode(sdo_pin,
                             TR_HAL_GPIO_MODE_SPI0);
        tr_hal_gpio_set_mode(sdi_pin,
                             TR_HAL_GPIO_MODE_SPI0);

    }
    // SPI1
    else if (spi_id == SPI_1_ID)
    {
        // check if pins are valid
        status = hal_spi_1_check_std_pins_valid(clk_pin,
                                                chip_select_0_pin,
                                                sdo_pin,
                                                sdi_pin);
        // if error, return it
        if (status != TR_HAL_SUCCESS)
        {
            return status;
        }

        // one example explained that: the clock pin drive strength needs
        // to be reduced. But in testing it works with or without this
        // change. Leaving this code in case it is needed later.
        //tr_hal_gpio_set_drive_strength(clk_pin,
        //                               TR_HAL_DRIVE_STRENGTH_14_MA);

        // set the pins
        tr_hal_gpio_set_mode(clk_pin,
                             TR_HAL_GPIO_MODE_SPI1);
        tr_hal_gpio_set_mode(chip_select_0_pin,
                             TR_HAL_GPIO_MODE_SPI1);
        tr_hal_gpio_set_mode(sdo_pin,
                             TR_HAL_GPIO_MODE_SPI1);
        tr_hal_gpio_set_mode(sdi_pin,
                             TR_HAL_GPIO_MODE_SPI1);
    }

    // invalid SPI ID
    else
    {
        return TR_HAL_INVALID_SPI_ID;
    }

    // tell the GPIO manager that these pins are in use
    tr_hal_gpio_mgr_reserve_gpio(clk_pin,
                                 TR_HAL_GPIO_SET_FOR_SPI);
    tr_hal_gpio_mgr_reserve_gpio(chip_select_0_pin,
                                 TR_HAL_GPIO_SET_FOR_SPI);
    tr_hal_gpio_mgr_reserve_gpio(sdo_pin,
                                 TR_HAL_GPIO_SET_FOR_SPI);
    tr_hal_gpio_mgr_reserve_gpio(sdi_pin,
                                 TR_HAL_GPIO_SET_FOR_SPI);

    return TR_HAL_SUCCESS;
}


/// ***************************************************************************
/// function for setting the pins for a quad SPI which requires 2 more pins
/// this also checks that the pin choices are VALID for that particular SPI
/// ***************************************************************************
////tr_hal_status_t tr_hal_spi_set_quad_pins(tr_hal_spi_id_t   spi_id,
////                                         tr_hal_gpio_pin_t clk_pin,
////                                         tr_hal_gpio_pin_t chip_select_pin,
////                                         tr_hal_gpio_pin_t io_0_pin,
////                                         tr_hal_gpio_pin_t io_1_pin,
////                                         tr_hal_gpio_pin_t io_2_pin,
////                                         tr_hal_gpio_pin_t io_3_pin)
////{
////    tr_hal_status_t status;
////
////    // SPI 1 can't be quad mode
////    if (spi_id == SPI_1_ID)
////    {
////        return TR_HAL_SPI_UNSUPPORTED_MODE;
////    }
////
////    // we MUST be SPI0, otherwise error
////    if (spi_id != SPI_0_ID)
////    {
////        return TR_HAL_INVALID_SPI_ID;
////    }
////
////    // check if the first four pins are valid
////    status = hal_spi_0_check_std_pins_valid(clk_pin,
////                                            chip_select_pin,
////                                            io_0_pin,
////                                            io_1_pin);
////    // if error, return it
////    if (status != TR_HAL_SUCCESS)
////    {
////        return status;
////    }
////
////    // check IO2
////    if (   (io_2_pin.pin != SPI0_IO2_PIN_OPTION1)
////        && (io_2_pin.pin != SPI0_IO2_PIN_OPTION2) )
////    {
////        return TR_HAL_SPI_INVALID_IO2_PIN;
////    }
////
////    // check IO3
////    if (   (io_3_pin.pin != SPI0_IO3_PIN_OPTION1)
////        && (io_3_pin.pin != SPI0_IO3_PIN_OPTION2) )
////    {
////        return TR_HAL_SPI_INVALID_IO3_PIN;
////    }
////
////    tr_hal_gpio_set_mode(clk_pin,
////                         TR_HAL_GPIO_MODE_SPI0);
////    tr_hal_gpio_set_mode(chip_select_pin,
////                         TR_HAL_GPIO_MODE_SPI0);
////    tr_hal_gpio_set_mode(io_0_pin,
////                         TR_HAL_GPIO_MODE_SPI0);
////    tr_hal_gpio_set_mode(io_1_pin,
////                         TR_HAL_GPIO_MODE_SPI0);
////    tr_hal_gpio_set_mode(io_2_pin,
////                         TR_HAL_GPIO_MODE_SPI0);
////    tr_hal_gpio_set_mode(io_3_pin,
////                         TR_HAL_GPIO_MODE_SPI0);
////
////    return TR_HAL_SUCCESS;
////}


/// ***************************************************************************
/// function for setting additional chip select pins beyond the 1 already set
/// either tr_hal_spi_set_standard_pins or ..set_quad_pins needs to have been called
/// this also checks that the pin choices are VALID for that particular SPI
/// ***************************************************************************
tr_hal_status_t tr_hal_spi_set_addl_cs_pins(tr_hal_spi_id_t   spi_id,
                                            uint8_t           num_chip_select,
                                            tr_hal_gpio_pin_t chip_select_1_pin,
                                            tr_hal_gpio_pin_t chip_select_2_pin,
                                            tr_hal_gpio_pin_t chip_select_3_pin)
{
    // SPI 1 can't set more than 1 CS
    if (spi_id == SPI_1_ID)
    {
        return TR_HAL_SPI_INVALID_CS1_PIN;
    }

    // we MUST be SPI0, otherwise error
    if (spi_id != SPI_1_ID)
    {
        return TR_HAL_INVALID_SPI_ID;
    }

    // can't have too many CS
    if (num_chip_select > SPI0_MAX_CHIP_SELECT_PINS)
    {
        return TR_HAL_ERROR_INVALID_PARAM;
    }

    // must have at least 1 more than the 1 default = 2
    if (num_chip_select < 2)
    {
        return TR_HAL_ERROR_INVALID_PARAM;
    }

    // check CS1
    if (   (chip_select_1_pin.pin != SPI0_CS1_PIN_OPTION1)
        && (chip_select_1_pin.pin != SPI0_CS1_PIN_OPTION2)
        && (chip_select_1_pin.pin != SPI0_CS1_PIN_OPTION3)
        && (chip_select_1_pin.pin != SPI0_CS1_PIN_OPTION4) )
    {
        return TR_HAL_SPI_INVALID_CS1_PIN;
    }

    // check CS2 if we are setting it
    if (num_chip_select > 2)
    {
        if (   (chip_select_2_pin.pin != SPI0_CS2_PIN_OPTION1)
            && (chip_select_2_pin.pin != SPI0_CS2_PIN_OPTION2)
            && (chip_select_2_pin.pin != SPI0_CS2_PIN_OPTION3)
            && (chip_select_2_pin.pin != SPI0_CS2_PIN_OPTION4) )
        {
            return TR_HAL_SPI_INVALID_CS2_PIN;
        }
    }

    // check CS3 if we are setting it
    if (num_chip_select > 3)
    {
        if (   (chip_select_3_pin.pin != SPI0_CS3_PIN_OPTION1)
            && (chip_select_3_pin.pin != SPI0_CS3_PIN_OPTION2)
            && (chip_select_3_pin.pin != SPI0_CS3_PIN_OPTION3)
            && (chip_select_3_pin.pin != SPI0_CS3_PIN_OPTION4) )
        {
            return TR_HAL_SPI_INVALID_CS3_PIN;
        }
    }

    // at this point we have valid CS, so set the GPIO mode

    // chip select 1
    tr_hal_gpio_set_mode(chip_select_1_pin,
                         TR_HAL_GPIO_MODE_SPI0_CS1);

    // chip select 2 - if specified
    if (num_chip_select > 2)
    {
        tr_hal_gpio_set_mode(chip_select_2_pin,
                             TR_HAL_GPIO_MODE_SPI0_CS2);
    }

    // chip select 3 - if specified
    if (num_chip_select > 3)
    {
        tr_hal_gpio_set_mode(chip_select_3_pin,
                             TR_HAL_GPIO_MODE_SPI0_CS3);
    }

    return TR_HAL_SUCCESS;
}

/// ***************************************************************************
/// tr_hal_spi_internal_interrupt_handler
/// ***************************************************************************
void tr_hal_spi_internal_interrupt_handler(tr_hal_spi_id_t spi_id)
{
    // get the chip register address and settings
    SPI_REGISTERS_T* spi_register_address = tr_hal_spi_get_register_address(spi_id);

    // get the current settings for this SPI (including event_handler_fx)
    tr_hal_spi_settings_t* spi_settings = &(g_current_spi_settings[spi_id]);

    // bitmask to send to event_handler_fx
    uint32_t event_bitmask = 0;

    // figure out what interrupts have occurred in the DMA INT STATUS register
    uint32_t dma_int_status = spi_register_address->DMA_interrupt_status;

    // figure out what interrupts have occurred in the INT STATUS register
    uint32_t int_status = spi_register_address->interrupt_status;

    // ****************************************
    // transmitter empty interrupt
    // this happens on the TRANSMITTING DEVICE when the whole transmission is completed
    // at this point we can release CS if we are the Controller
    // if we are using DMA TX then we need to turn it off
    // ****************************************
    if (int_status & SPI_INTERRUPT_TX_EMPTY)
    {
        // if we are transmitting and transmitter is empty...
        if (g_transmitting_active[spi_id])
        {
            // we may still have more to send
            if (g_bytes_remaining[spi_id] > 0)
            {
                hal_spi_internal_tx_more_bytes(spi_id);
            }
            // if we don't have more to send...
            else
            {
                // we are dont with this transmit
                g_transmitting_active[spi_id] = false;
                g_count_tx_finished[spi_id]++;

                // set the AUX CTRL back to normal so we can receive
                spi_register_address->spi_aux_control = spi_remember_aux_ctrl_setting[spi_id];

                // if this was using DMA TX then turn it off - we are done for now
                if (g_dma_tx_active[spi_id])
                {
                    spi_register_address->DMA_tx_enable = SPI_DMA_DISABLE;
                    g_dma_tx_active[spi_id] = false;
                }

                // we do NOT need to deassert CS0, the chip does it for us
                // if (spi_settings->run_as_controller)
                // {
                //     spi_register_address->peripheral_select = SPI_PERIPH_SELECT_NONE;
                // }
            }
        }
        // clear the interrupt
        spi_register_address->interrupt_clear = SPI_INTERRUPT_TX_EMPTY;

        // set the event
        event_bitmask |= TR_HAL_SPI_EVENT_TX_EMPTY;
    }

    // ****************************************
    // transmitter hit watermark interrupt
    // this means we have hit the watermark defined by the user (in spi_settings->transmit_watermark)
    // just send the message back to the user
    // ****************************************
    if (int_status & SPI_INTERRUPT_TX_WATERMARK)
    {
        spi_register_address->interrupt_clear = SPI_INTERRUPT_TX_WATERMARK;
        event_bitmask |= TR_HAL_SPI_EVENT_TX_WMARK;
    }

    // ****************************************
    // receiver hit watermark
    // this means we have hit the watermark defined by the user (in spi_settings->receive_watermark)
    // when this happens we try to read bytes and send them back to
    // the user defined rx_handler_function via a call to hal_spi_internal_handle_rx_bytes
    // if no rx_handler_function then we send an event for the app to read bytes
    // ****************************************
    if (int_status & SPI_INTERRUPT_RX_WATERMARK)
    {
        // clear the interrupt
        spi_register_address->interrupt_clear = SPI_INTERRUPT_RX_WATERMARK;

        // setup the event
        event_bitmask |= TR_HAL_SPI_EVENT_RX_WMARK;

        // if we have a user-defined receive callback function, we need to
        // read the received bytes and send them to that function
        if (spi_settings->rx_handler_function != NULL)
        {
            uint8_t bytes_found = hal_spi_internal_handle_rx_bytes(spi_id);

            // if there were bytes to return, set the event
            if (bytes_found > 0)
            {
                event_bitmask |= TR_HAL_SPI_EVENT_RX_TO_USER_FX;
            }
        }
        // this is when there is no user function
        else
        {
            // let the user know to read bytes
            event_bitmask |= TR_HAL_SPI_EVENT_RX_READY;
        }

    }

    // ****************************************
    // transfer done
    // this happens on the receiver when bytes are received and the
    // transmission completes (chip select de-asserts)
    // when this happens we try to read bytes and send them back to
    // the user defined rx_handler_function via a call to hal_spi_internal_handle_rx_bytes
    // if no rx_handler_function then we send an event for the app to read bytes
    // ****************************************
    if (int_status & SPI_INTERRUPT_TRANSFER_DONE)
    {
        // clear the interrupt
        spi_register_address->interrupt_clear = SPI_INTERRUPT_TRANSFER_DONE;

        // setup the event
        event_bitmask |= TR_HAL_SPI_EVENT_TRANSFER_DONE;

        // if we have RX DMA enabled, check the DMA buffer
        if (spi_settings->rx_dma_enabled)
        {
            // if we have a user handler function then handle the received bytes
            if (spi_settings->rx_handler_function != NULL)
            {
                hal_spi_internal_handle_rx_dma_bytes(spi_id);
                event_bitmask |= TR_HAL_SPI_EVENT_DMA_RX_TO_USER_FX;
            }
            // otherwise just send back an event
            else
            {
                // event is that DMA RX bytes are ready
                event_bitmask |= TR_HAL_SPI_EVENT_DMA_RX_READY;
            }
        }
        // if we do NOT have DMA RX defined, then handle it raw
        else
        {
            // if we have a user-defined receive callback function, we need to
            // read the received bytes and send them to that function
            if (spi_settings->rx_handler_function != NULL)
            {
                uint8_t bytes_found = hal_spi_internal_handle_rx_bytes(spi_id);

                // if there were bytes to return, set the event
                if (bytes_found > 0)
                {
                    event_bitmask |= TR_HAL_SPI_EVENT_RX_TO_USER_FX;
                }
            }
            // this is when there is no user function
            else
            {
                // let the user know to read bytes
                event_bitmask |= TR_HAL_SPI_EVENT_RX_READY;
            }
        }
    }

    // ****************************************
    // receiver full
    // if a transmission sends more bytes than the FIFO can
    // hold then we will get this interrupt BEFORE the TRANSFER_DONE
    // interrupt. hopefully we get the RX_WATERMARK interrupt first
    // when this happens we try to read bytes and send them back to
    // the user defined rx_handler_function via a call to hal_spi_internal_handle_rx_bytes
    // if no rx_handler_function then we send an event for the app to read bytes
    // ****************************************
    if (int_status & SPI_INTERRUPT_RX_FULL)
    {
        // clear the interrupt
        spi_register_address->interrupt_clear = SPI_INTERRUPT_RX_FULL;

        // setup the event
        event_bitmask |= TR_HAL_SPI_EVENT_RX_FULL;

        // if we have a user-defined receive callback function, we need to
        // read the received bytes and send them to that function
        if (spi_settings->rx_handler_function != NULL)
        {
            uint8_t bytes_found = hal_spi_internal_handle_rx_bytes(spi_id);

            // if there were bytes to return, set the event
            if (bytes_found > 0)
            {
                event_bitmask |= TR_HAL_SPI_EVENT_RX_TO_USER_FX;
            }
        }
        // this is when there is no user function
        else
        {
            // let the user know to read bytes
            event_bitmask |= TR_HAL_SPI_EVENT_RX_READY;
        }
    }

    // ****************************************
    // receiver still sending
    // just send the message back to the user
    // ****************************************
    if (int_status & SPI_INTERRUPT_RX_NOT_EMPTY)
    {
        spi_register_address->interrupt_clear = SPI_INTERRUPT_RX_NOT_EMPTY;
        event_bitmask |= TR_HAL_SPI_EVENT_RX_HAS_MORE_DATA;
    }

    // ****************************************
    // now we check the DMA interrupts, which could also be the source of the interrupt
    // ****************************************


    // ****************************************
    // DMA RX interrupt
    // this interrupt only fires when the DMA RX buffer has filled up, so we
    // normally use the TRANSFER_DONE interrupt to check for DMA RX bytes, since
    // it will happen before this interrupt (we don't want to wait for the DMA RX
    // buffer to be full before processing the received bytes. BUT if a transfer
    // is LARGER than our DMA RX buffer, we will see this interrupt first.
    // ****************************************
    if (dma_int_status & SPI_DMA_RX_INTERRUPT_ACTIVE)
    {
        // clear the interrupt
        spi_register_address->DMA_interrupt_status = SPI_DMA_RX_INTERRUPT_ACTIVE;

        // if we have a user handler function then handle the received bytes
        if (spi_settings->rx_handler_function != NULL)
        {
            hal_spi_internal_handle_rx_dma_bytes(spi_id);
            event_bitmask |= TR_HAL_SPI_EVENT_DMA_RX_TO_USER_FX;
        }
        // otherwise just send back an event
        else
        {
            // event is that DMA RX bytes are ready
            event_bitmask |= TR_HAL_SPI_EVENT_DMA_RX_READY;
        }
    }

    // ****************************************
    // DMA TX interrupt
    // ****************************************
    if (dma_int_status & SPI_DMA_TX_INTERRUPT_ACTIVE)
    {
        // clear the interrupt
        spi_register_address->DMA_interrupt_status = SPI_DMA_TX_INTERRUPT_ACTIVE;

        // set the event
        event_bitmask |= TR_HAL_SPI_EVENT_DMA_TX_COMPLETE;

        // this is one place we could disable DMA TX, since the transmit is done
        // but we already do this in the SPI_INTERRUPT_TX_EMPTY regular interrupt
        //spi_register_address->DMA_tx_enable = SPI_DMA_DISABLE;
    }

    // ****************************************
    // call the user event function, passing the event bitmask
    // ****************************************
    if (spi_settings->event_handler_fx != NULL)
    {
        spi_settings->event_handler_fx(spi_id, event_bitmask);
    }
}


/// ***************************************************************************
/// chip interrupt handlers
/// ***************************************************************************
void qspi0_handler(void)
{
    tr_hal_spi_internal_interrupt_handler(SPI_0_ID);
    return;
}

void qspi1_handler(void)
{
    tr_hal_spi_internal_interrupt_handler(SPI_1_ID);
    return;
}

/// ***************************************************************************
/// hal_spi_internal_tx_more_bytes
///
/// on raw TX commands, we can be asked to send more bytes than the TX FIFO
/// can handle. In these cases, we store the bytes to be sent in the
/// SPI settings raw_tx_buffer. These variables help keep track of
/// what we still need to send
///
/// this is EXPECTED TO BE CALLED WHEN THE TX FIFO IS CLEAR
/// this is also expected to be called when we are in the middle of a
/// transmission, and this does NOT assert or deassert chip select,
/// since it expects it to be already asserted
///
/// ***************************************************************************
static void hal_spi_internal_tx_more_bytes(tr_hal_spi_id_t spi_id)
{
    // if nothing to do then eject
    if (g_bytes_remaining[spi_id] == 0)
    {
        g_transmitting_active[spi_id] = false;
        return;
    }

    // get the chip register address and settings
    SPI_REGISTERS_T* spi_register_address = tr_hal_spi_get_register_address(spi_id);

    // get the current settings for this SPI (including event_handler_fx)
    tr_hal_spi_settings_t* spi_settings = &(g_current_spi_settings[spi_id]);


    // figure out how many bytes to send
    // we send as much as we can up to the FIFO size
    uint16_t bytes_to_send_this_time = g_bytes_remaining[spi_id];
    if (g_bytes_remaining[spi_id] > TR_HAL_SPI_TX_FIFO_SIZE)
    {
        bytes_to_send_this_time = TR_HAL_SPI_TX_FIFO_SIZE;
    }

    // get the index to start at and end at
    uint16_t send_start_index = g_bytes_sent[spi_id];
    uint16_t send_finish_index = g_bytes_sent[spi_id] + bytes_to_send_this_time;

    // send up to what the FIFO can handle.
    // note: we do not change chip select, we expect it to still be asserted
    uint8_t* tx_buffer = spi_settings->raw_tx_buffer;
    for (uint8_t index = (uint8_t)send_start_index; index < send_finish_index; index++)
    {
        // transmit data
        spi_register_address->spi_tx_data = tx_buffer[index];
    }

    // note: we do not de-assert chip select, once these bytes are transmitted
    // there will be a TX_EMPTY interrupt and we will either come back here
    // (if there is more to send) or de-assert CS at that point

    // adjust the pointers
    g_bytes_remaining[spi_id] = g_bytes_remaining[spi_id] - bytes_to_send_this_time;
    g_bytes_sent[spi_id] = g_bytes_sent[spi_id] + bytes_to_send_this_time;
}



/// ***************************************************************************
/// hal_spi_internal_handle_rx_bytes
///
/// when we think we have bytes to receive, and the app/user has defined a
/// rx_handler_function, this gets called. This is meant to read bytes out
/// of the chip register FIFOs and send them to the rx_handler_function
///
/// when we think there are bytes and there is NO user function, the app
/// gets an event into the event handler
/// ***************************************************************************
static uint8_t hal_spi_internal_handle_rx_bytes(tr_hal_spi_id_t spi_id)
{
    uint32_t rx_fifo_level = 0;

    // get the chip register address and settings
    SPI_REGISTERS_T* spi_register_address = tr_hal_spi_get_register_address(spi_id);
    tr_hal_spi_settings_t* spi_settings = &(g_current_spi_settings[spi_id]);

    // if we have a user-defined receive callback function, we need to
    // read the received bytes and send them to that function
    if (spi_settings->rx_handler_function != NULL)
    {
        // the easiest way to see if we have RX data to read is to
        // check the RX FIFO level
        rx_fifo_level = spi_register_address->rx_fifo_current_level;

        // if we have bytes then read them and return them
        if (rx_fifo_level > 0)
        {
            uint8_t rx_buffer[TR_HAL_SPI_RX_FIFO_SIZE];

            // read in all the bytes that we can
            for (uint8_t i=0; i<rx_fifo_level; i++)
            {
                rx_buffer[i] = (uint8_t) spi_register_address->spi_rx_data;
                g_count_rx_bytes[spi_id]++;
            }

            // call the rx_handler_function, passing back the data
            spi_settings->rx_handler_function((uint8_t)rx_fifo_level, rx_buffer);
        }
    }
    return (uint8_t) rx_fifo_level;
}

/// ***************************************************************************
/// hal_spi_internal_handle_rx_dma_bytes
///
/// INTERNAL
/// we got an interrupt that DMA bytes are available
/// AND we have a user defined function
/// pass these to the user defined receive function
/// ***************************************************************************
static void hal_spi_internal_handle_rx_dma_bytes(tr_hal_spi_id_t spi_id)
{
    // get the SPI settings
    tr_hal_spi_settings_t* spi_settings = &(g_current_spi_settings[spi_id]);

    // we expect that we have a RX data buffer AND a user defined function
    // if that is not true then we need to eject
    if (   (spi_settings->rx_dma_buffer == NULL)
        || (spi_settings->rx_handler_function == NULL))
    {
        return;
    }

    // get the buffer and length and current index
    uint8_t* rx_buffer = spi_settings->rx_dma_buffer;
    uint16_t buff_len = spi_settings->rx_dma_buff_length;
    uint16_t curr_index = g_curr_spi_dma_rx_index[spi_id];
    uint8_t found_bytes = 0;

    for (uint16_t i = curr_index; i < buff_len; i++)
    {
        found_bytes++;
        // we don't just return one byte at a time
        //spi_settings->rx_handler_function(rx_buffer[i]);
        g_curr_spi_dma_rx_index[spi_id]++;
        g_count_rx_bytes[spi_id]++;
    }
    // we return the full buffer of bytes, not just one byte at a time
    spi_settings->rx_handler_function(found_bytes, &(rx_buffer[curr_index]));
}


/// ***************************************************************************
/// tr_hal_spi_read_stats
///
/// read the TX and RX stats of the SPI since init
/// ***************************************************************************
tr_hal_status_t tr_hal_spi_read_stats(tr_hal_spi_id_t spi_id,
                                      uint32_t* transmit_started,
                                      uint32_t* transmit_completed,
                                      uint32_t* bytes_received)
{
    // error check the spi ID
    if (spi_id >= TR_HAL_NUM_SPI)
    {
        return TR_HAL_INVALID_SPI_ID;
    }

    // ready also means initialized
    if (g_spi_init_completed[spi_id] == false)
    {
        return TR_HAL_ERROR_NOT_INITIALIZED;
    }

    (*transmit_started) = g_count_tx_started[spi_id];
    (*transmit_completed) = g_count_tx_finished[spi_id];
    (*bytes_received) = g_count_rx_bytes[spi_id];

    return TR_HAL_SUCCESS;
}


/// ***************************************************************************
///
/// *** note this API has not yet been proven to work ***
///
/// tr_hal_spi_start_dual_spi_receive
///
/// this is meant to be able to place a SPI into DUAL mode, since some devices
/// expect to start in NORMAL SPI mode and then switch to DUAL mode after sending
/// an instructions/command-byte. So the cmd can be sent and then this can be
/// called. This switches mode to DUAL and also sets the SPI_AUX_CTRL_REG_TRANSFER_EXTEND
/// bit which causes Chip Select to STAY asserted until this register is changed
/// (which can be done using tr_hal_spi_stop_dual_spi_receive)
/// ***************************************************************************
////tr_hal_status_t tr_hal_spi_start_dual_spi_receive(tr_hal_spi_id_t spi_id)
////{
////    SPI_REGISTERS_T* spi_register_address = tr_hal_spi_get_register_address(spi_id);
////    tr_hal_spi_settings_t* spi_settings = &(g_current_spi_settings[spi_id]);
////
////    uint32_t aux_ctrl_register_value = 0;
////
////    // set the SPI mode to dual
////    aux_ctrl_register_value |= TR_HAL_SPI_MODE_DUAL;
////
////    // we don't set inhibitDout or inhibitDin
////    // these prevent the SPI writing (inhibitDout) or reading (inhibitDin) data
////    // this is when communication is 1 way: not sure if there is a use case for this
////
////    // set bitsize 8 or 32
////    aux_ctrl_register_value |= spi_settings->bit_size;
////
////    // set contXferExtend
////    aux_ctrl_register_value |= SPI_AUX_CTRL_REG_TRANSFER_EXTEND;
////
////    // write the value to the auxiliary control register
////    spi_register_address->spi_aux_control = aux_ctrl_register_value;
////
////    return TR_HAL_SUCCESS;
////}


/// ***************************************************************************
///
/// *** note this API has not yet been proven to work ***
///
/// tr_hal_spi_stop_dual_spi_receive
///
/// meant to be called after tr_hal_spi_start_dual_spi_receive is called and
/// then DUAL SPI command it done, call this which will change the SPI MODE
/// back to NORMAL (from DUAL) and unset the SPI_AUX_CTRL_REG_TRANSFER_EXTEND
/// bit in AUX CTRL register so the Chip Select can deassert
/// ***************************************************************************
////tr_hal_status_t tr_hal_spi_stop_dual_spi_receive(tr_hal_spi_id_t spi_id)
////{
////    SPI_REGISTERS_T* spi_register_address = tr_hal_spi_get_register_address(spi_id);
////
////    // set the AUX CTRL so we end the CS and end the transfer
////    uint32_t new_setting = spi_remember_aux_ctrl_setting[spi_id];
////    spi_register_address->spi_aux_control = new_setting;
////
////    return TR_HAL_SUCCESS;
////}

