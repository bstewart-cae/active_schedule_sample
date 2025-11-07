/// ****************************************************************************
/// @file tr_hal_pwm.c
///
/// @brief This contains the code for the Trident HAL PWM for T32CZ20
///
///
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************

#include "tr_hal_pwm.h"
#include <string.h>

// is the PWM initialized
static bool g_pwm_init_completed[TR_HAL_NUM_PWM] = {false, false, false, false, false};
static bool g_pwm_is_running[TR_HAL_NUM_PWM] = {false, false, false, false, false};

// we save the config info for an initialized PWM
tr_hal_pwm_settings_t g_current_pwm_settings[TR_HAL_NUM_PWM];

// RAM used by PWM to be loaded into the DMA
static uint32_t g_rseq_ram[TR_HAL_NUM_PWM] = {0,0,0,0,0};


/// ***************************************************************************
/// tr_hal_rtc_get_register_address
/// ***************************************************************************
PWM_REGISTERS_T* tr_hal_pwm_get_register_address(tr_hal_pwm_id_t pwm_id)
{
    if      (pwm_id == PWM_0_ID) { return PWM0_REGISTERS;}
    else if (pwm_id == PWM_1_ID) { return PWM1_REGISTERS;}
    else if (pwm_id == PWM_2_ID) { return PWM2_REGISTERS;}
    else if (pwm_id == PWM_3_ID) { return PWM3_REGISTERS;}
    else if (pwm_id == PWM_4_ID) { return PWM4_REGISTERS;}

    return NULL;
}


/// ***************************************************************************
/// tr_hal_pwm_init
/// ***************************************************************************
tr_hal_status_t check_pwm_settings_valid(tr_hal_pwm_id_t        pwm_id,
                                         tr_hal_pwm_settings_t* pwm_settings)

{
    tr_hal_status_t status;

    // settings can't be NULL
    if (pwm_settings == NULL)
    {
        return TR_HAL_ERROR_NULL_PARAMS;
    }

    // PWM ID must be in range
    if (pwm_id > PWM_4_ID)
    {
        return TR_HAL_INVALID_PWM_ID;
    }

    // this PWM can't be already initialized
    // it needs to be uninitialized before being used again
    if (g_pwm_init_completed[pwm_id] == true)
    {
        return TR_HAL_ERROR_ALREADY_INITIALIZED;
    }

    // GPIO must be in range
    if (! (tr_hal_gpio_is_available(pwm_settings->pin_to_use)))
    {
        return TR_HAL_ERROR_PIN_NOT_AVAILABLE;
    }

    // check if GPIO is already configured to another peripheral
    status = tr_hal_gpio_mgr_check_gpio(pwm_settings->pin_to_use);
    if (status != TR_HAL_SUCCESS)
    {
        return status;
    }

    // threshhold must be less than end_count
    if ( (pwm_settings->threshhold) >= (pwm_settings->end_count) )
    {
        return TR_HAL_PWM_TH_MUST_BE_LESS_THAN_EC;
    }

    // threshhold must be within range
    if ( (pwm_settings->threshhold < MINIMUM_THRESHHOLD_VALUE)
        || (pwm_settings->threshhold > MAXIMUM_THRESHHOLD_VALUE))
    {
        return TR_HAL_PWM_THRESHHOLD_INVALID;
    }

    // end count must be within range
    if ( (pwm_settings->end_count < MINIMUM_END_COUNT_VALUE)
        || (pwm_settings->end_count > MAXIMUM_END_COUNT_VALUE))
    {
        return TR_HAL_PWM_END_COUNT_INVALID;
    }

    // clk divider must be within range
    tr_hal_pwm_clk_div_t clk_div = pwm_settings->clock_divider;
    if (    (clk_div != TR_HAL_PWM_CLOCK_DIVIDER_1)
         && (clk_div != TR_HAL_PWM_CLOCK_DIVIDER_2)
         && (clk_div != TR_HAL_PWM_CLOCK_DIVIDER_4)
         && (clk_div != TR_HAL_PWM_CLOCK_DIVIDER_8)
         && (clk_div != TR_HAL_PWM_CLOCK_DIVIDER_16)
         && (clk_div != TR_HAL_PWM_CLOCK_DIVIDER_32)
         && (clk_div != TR_HAL_PWM_CLOCK_DIVIDER_64)
         && (clk_div != TR_HAL_PWM_CLOCK_DIVIDER_128)
         && (clk_div != TR_HAL_PWM_CLOCK_DIVIDER_256)
       )
    {
        return TR_HAL_PWM_CLK_DIV_INVALID;
    }

    // check clock select (clock source)
    tr_hal_pwm_clk_select_t clk_source = pwm_settings->clock_to_use;
    if (    (clk_source != TR_HAL_PWM_CLK_SELECT_HCLK)
         && (clk_source != TR_HAL_PWM_CLK_SELECT_PER_CLK)
         && (clk_source != TR_HAL_PWM_CLK_SELECT_RCO_1M)
         && (clk_source != TR_HAL_PWM_CLK_SELECT_SLOW_CLK)
       )
    {
        return TR_HAL_PWM_CLK_SELECT_INVALID;
    }

    return TR_HAL_SUCCESS;
}


/// ***************************************************************************
/// set_pwm_clock
/// ***************************************************************************
static void set_pwm_clock(tr_hal_pwm_id_t         pwm_id,
                          tr_hal_pwm_clk_select_t new_clock)
{
    uint32_t clock_reg_1 = SYS_CTRL_CHIP_REGISTERS->system_clock_control_1;

    // the PWM clock is a 2 bit value that is part of this register
    // PWM0 is at 16 bits, PWM1 is at 18 bits, PWM2 is at 20 bits, etc
    uint32_t pwm_shift = (uint32_t)(16 + (pwm_id * 2));

    // find the current value
    tr_hal_pwm_clk_select_t existing_clock = (clock_reg_1 >> pwm_shift) & 0x00000003;

    if (existing_clock == new_clock)
    {
        return;
    }

    // since they are different, update the clock select

    // first clear those 2 bits
    uint32_t bitmask = 0x03 << pwm_shift;
    uint32_t inv_bitmask = ~(bitmask);
    clock_reg_1 = clock_reg_1 & inv_bitmask;

    // now add those 2 bits back in
    uint32_t add_val = new_clock << pwm_shift;
    clock_reg_1 = clock_reg_1 | add_val;

    // write the register
    SYS_CTRL_CHIP_REGISTERS->system_clock_control_1 = clock_reg_1;
}


/// ***************************************************************************
/// set_pin_to_pwm_mode
///
/// we expect this to be called AFTER check_pwm_settings_valid
/// ***************************************************************************
static tr_hal_status_t set_pin_to_pwm_mode(tr_hal_pwm_id_t        pwm_id,
                                           tr_hal_pwm_settings_t* pwm_settings)

{
    tr_hal_status_t status;

    // settings can't be NULL
    if (pwm_settings == NULL)
    {
        return TR_HAL_ERROR_NULL_PARAMS;
    }

    tr_hal_gpio_pin_t pin = pwm_settings->pin_to_use;

    // PWM 0
    if (pwm_id == PWM_0_ID)
    {
        status = tr_hal_gpio_set_mode(pin, TR_HAL_GPIO_MODE_PWM0);
    }
    // PWM 1
    else if (pwm_id == PWM_1_ID)
    {
        status = tr_hal_gpio_set_mode(pin, TR_HAL_GPIO_MODE_PWM1);
    }
    // PWM 2
    else if (pwm_id == PWM_2_ID)
    {
        status = tr_hal_gpio_set_mode(pin, TR_HAL_GPIO_MODE_PWM2);
    }
    // PWM 3
    else if (pwm_id == PWM_3_ID)
    {
        status = tr_hal_gpio_set_mode(pin, TR_HAL_GPIO_MODE_PWM3);
    }
    // PWM 4
    else if (pwm_id == PWM_4_ID)
    {
        status = tr_hal_gpio_set_mode(pin, TR_HAL_GPIO_MODE_PWM4);
    }
    // we should never get here, since we already checked for valid pin
    else
    {
        status = TR_HAL_ERROR_PIN_NOT_AVAILABLE;
    }

    // if it worked, tell the GPIO manager that this pin is in use
    if (status == TR_HAL_SUCCESS)
    {
        tr_hal_gpio_mgr_reserve_gpio(pin,
                                     TR_HAL_GPIO_SET_FOR_PWM);
    }

    return status;
}


/// ***************************************************************************
/// get_clk_div_register_val_from_enum
/// ***************************************************************************
static uint32_t get_clk_div_register_val_from_enum(tr_hal_pwm_clk_div_t clk_div)
{
    switch (clk_div)
    {
        case TR_HAL_PWM_CLOCK_DIVIDER_1:   return PWM_CLK_DIV1_1;
        case TR_HAL_PWM_CLOCK_DIVIDER_2:   return PWM_CLK_DIV1_2;
        case TR_HAL_PWM_CLOCK_DIVIDER_4:   return PWM_CLK_DIV1_4;
        case TR_HAL_PWM_CLOCK_DIVIDER_8:   return PWM_CLK_DIV1_8;
        case TR_HAL_PWM_CLOCK_DIVIDER_16:  return PWM_CLK_DIV1_16;
        case TR_HAL_PWM_CLOCK_DIVIDER_32:  return PWM_CLK_DIV1_32;
        case TR_HAL_PWM_CLOCK_DIVIDER_64:  return PWM_CLK_DIV1_64;
        case TR_HAL_PWM_CLOCK_DIVIDER_128: return PWM_CLK_DIV1_128;
        case TR_HAL_PWM_CLOCK_DIVIDER_256: return PWM_CLK_DIV1_256;
    }
    // should never get here since the clk_div was checked for validity
    // before calling this function
    return PWM_CLK_DIV1_1;
}


/// ***************************************************************************
/// tr_hal_pwm_init
/// ***************************************************************************
tr_hal_status_t tr_hal_pwm_init(tr_hal_pwm_id_t        pwm_id,
                                tr_hal_pwm_settings_t* pwm_settings)
{
    tr_hal_status_t status;

    // check that the values in settings are valid
    // this also checks that pwm_id is valid and settings is not NULL
    status = check_pwm_settings_valid(pwm_id, pwm_settings);
    if (status != TR_HAL_SUCCESS)
    {
        return status;
    }

    // set the GPIO pin to the PWM mode
    status = set_pin_to_pwm_mode(pwm_id, pwm_settings);
    if (status != TR_HAL_SUCCESS)
    {
        return status;
    }

    // valid settings, so get the PWM register address
    PWM_REGISTERS_T* pwm_register_address = tr_hal_pwm_get_register_address(pwm_id);

    // set the PWM clock
    set_pwm_clock(pwm_id, pwm_settings->clock_to_use);

    // disable the PWM and reset it
    pwm_register_address->enable = PWM_CTRL_REG_DISABLE_PWM;
    pwm_register_address->reset = PWM_CTRL_REG_RESET;

    // just 1 segment in RSEQ
    pwm_register_address->rseq_num_elements = 1;
    pwm_register_address->rseq_num_repeats = 100;
    pwm_register_address->rseq_delay = 0; // no gaps

    // ******************************
    // set up the RSEQ DMA segment
    // 0:14 threshhold
    // 15 polarity
    // 16:30 end_count
    // 31 reserved
    // ******************************

    // 1 MHz, 50% duty cycle
    // end_count = 0x1F + 1 = 0x20 = 32 --> 32Mhz/32 = 1 MHz
    // polarity = 1 (the 8)
    // threshhold = 0x010 = 16 --> (32-16)/32 = 50%
    //uint32_t record1 = (0x001F8010);

    // 500 KHz, 75% duty cycle
    // end_count = 0x3F + 1 = 0x40 = 64 --> 32Mhz/64 = 500 KHz
    // polarity = 1 (the 8)
    // threshhold = 0x010 = 16 --> (64-16)/64 = 75%
    //uint32_t record1 = (0x003F8010);

    // 500 KHz, 25% duty cycle
    // end_count = 0x3F + 1 = 0x40 = 64 --> 32Mhz/64 = 500 KHz
    // polarity = 1 (the 8)
    // threshhold = 0x030 = 48 --> (64-48)/64 = 75%
    //uint32_t record1 = (0x003F8030);

    // 500 KHz, 75% duty cycle
    // end_count = 0x3F + 1 = 0x40 = 64 --> 32Mhz/64 = 500 KHz
    // polarity = 0
    // threshhold = 0x030 = 48 --> 48/64 = 75%
    //uint32_t record1 = (0x003F0030);

    // we store end_count at the full value, so we need to subtract one here
    uint32_t record1 = (pwm_settings->end_count) - 1;
    record1 = record1 << 16;
    record1 = record1 | pwm_settings->threshhold;

    // set the RSEQ first record
    g_rseq_ram[pwm_id] = record1;

    // DMA setup for RSEQ data
    pwm_register_address->dma0_segment_size = 1;
    pwm_register_address->dma0_start_addr = (uint32_t) &(g_rseq_ram[pwm_id]);
    pwm_register_address->dma0_settings = 1; // load addr on reset

    // get the clock divisor register value from the enum
    uint32_t clk_div_reg = get_clk_div_register_val_from_enum(pwm_settings->clock_divider);

    // setup the settings for the PWM
    pwm_register_address->settings = PWM_CTRL_REG_RSEQ_FIRST
                                    | PWM_CTRL_REG_ONE_SEQUENCE
                                    | PWM_CTRL_REG_CONTINUOUS
                                    | PWM_CTRL_REG_DMA_FORMAT_1
                                    | PWM_CTRL_REG_UP_COUNTER
                                    | PWM_CTRL_REG_TRIGGER_ON_FIFO
                                    | PWM_CTRL_REG_NO_AUTO_TRIGGER
                                    | PWM_CTRL_REG_MODE_DMA
                                    | clk_div_reg
                                    | PWM_CTRL_REG_SELF_TRIGGER
                                    | PWM_CTRL_REG_DATA_PLAY_1
                                    | PWM_CLK_DIV2_NO_VALUE;

    // these registers aren't used when using PWM_CTRL_REG_DMA_FORMAT_1
    //pwm_register_address->counter_end = 100;
    pwm_register_address->sequence_repeat = 65535;

    // init has been completed for this PWM, so copy in the settings
    memcpy(&(g_current_pwm_settings[pwm_id]),
           pwm_settings,
           sizeof(tr_hal_pwm_settings_t));

    // set init flag
    g_pwm_init_completed[pwm_id] = true;
    g_pwm_is_running[pwm_id] = false;

    return TR_HAL_SUCCESS;
}


/// ***************************************************************************
/// tr_hal_pwm_uninit
/// ***************************************************************************
tr_hal_status_t tr_hal_pwm_uninit(tr_hal_pwm_id_t pwm_id)
{
    tr_hal_status_t status = TR_HAL_SUCCESS;

    // make sure we are initialized
    if (!(g_pwm_init_completed[pwm_id]))
    {
        return TR_HAL_ERROR_NOT_INITIALIZED;
    }

    // get the pin used for this PWM
    tr_hal_gpio_pin_t pin = (g_current_pwm_settings[pwm_id]).pin_to_use;

    // set the GPIO pin back to a GPIO
    status = tr_hal_gpio_set_mode(pin, TR_HAL_GPIO_MODE_GPIO);

    // release the GPIOs used
    tr_hal_gpio_mgr_release_gpio(pin);

    // disable the PWM
    PWM_REGISTERS_T* pwm_register_address = tr_hal_pwm_get_register_address(pwm_id);
    pwm_register_address->enable = PWM_CTRL_REG_DISABLE_PWM;

    // disable the 2 RDMAs
    pwm_register_address->dma0_enable = PWM_DMA_DISABLE;
    pwm_register_address->dma1_enable = PWM_DMA_DISABLE;

    g_pwm_init_completed[pwm_id] = false;
    g_pwm_is_running[pwm_id] = false;
    return status;
}



/// ***************************************************************************
/// tr_hal_pwm_settings_read
/// ***************************************************************************
tr_hal_status_t tr_hal_pwm_settings_read(tr_hal_pwm_id_t        pwm_id,
                                         tr_hal_pwm_settings_t* pwm_settings)
{
    // make sure we are initialized
    if (!(g_pwm_init_completed[pwm_id]))
    {
        return TR_HAL_ERROR_NOT_INITIALIZED;
    }

    // saved settings
    tr_hal_pwm_settings_t* saved = &(g_current_pwm_settings[pwm_id]);

    // copy over the saved info
    pwm_settings->pin_to_use    = saved->pin_to_use;
    pwm_settings->clock_divider = saved->clock_divider;
    pwm_settings->clock_to_use  = saved->clock_to_use;
    pwm_settings->end_count     = saved->end_count;
    pwm_settings->threshhold    = saved->threshhold;

    return TR_HAL_SUCCESS;
}


/// ***************************************************************************
/// tr_hal_pwm_start
/// ***************************************************************************
tr_hal_status_t tr_hal_pwm_start(tr_hal_pwm_id_t pwm_id)
{
    // make sure we are initialized
    if (!(g_pwm_init_completed[pwm_id]))
    {
        return TR_HAL_ERROR_NOT_INITIALIZED;
    }

    // if we are already running, do nothing
    if (g_pwm_is_running[pwm_id] == true)
    {
        return TR_HAL_SUCCESS;
    }

    // get the PWM register address
    PWM_REGISTERS_T* pwm_register_address = tr_hal_pwm_get_register_address(pwm_id);

    // enable the PWM and the PWM clock
    pwm_register_address->enable = ( PWM_CTRL_REG_ENABLE_PWM | PWM_CTRL_REG_ENABLE_CLK);

    // enable and reset the RDMA
    pwm_register_address->dma0_enable = PWM_DMA_ENABLE;
    pwm_register_address->dma0_reset = PWM_DMA_RESET;

    g_pwm_is_running[pwm_id] = true;

    return TR_HAL_SUCCESS;
}


/// ***************************************************************************
/// tr_hal_pwm_stop
/// ***************************************************************************
tr_hal_status_t tr_hal_pwm_stop(tr_hal_pwm_id_t pwm_id)
{
    // make sure we are initialized
    if (!(g_pwm_init_completed[pwm_id]))
    {
        return TR_HAL_ERROR_NOT_INITIALIZED;
    }

    // if we are already stopped, do nothing
    if (g_pwm_is_running[pwm_id] == false)
    {
        return TR_HAL_SUCCESS;
    }

    // get the PWM register address
    PWM_REGISTERS_T* pwm_register_address = tr_hal_pwm_get_register_address(pwm_id);

    // disable the PWM
    pwm_register_address->enable = PWM_CTRL_REG_DISABLE_PWM;

    // disable the RDMA
    pwm_register_address->dma0_enable = PWM_DMA_DISABLE;

    g_pwm_is_running[pwm_id] = false;

    return TR_HAL_SUCCESS;
}


/// ***************************************************************************
/// tr_hal_pwm_is_running
/// ***************************************************************************
tr_hal_status_t tr_hal_pwm_is_running(tr_hal_pwm_id_t pwm_id,
                                      bool*           is_running)
{
    (*is_running) = g_pwm_is_running[pwm_id];

    return TR_HAL_SUCCESS;
}


