/// ****************************************************************************
/// @file tr_hal_adc.c
///
/// @brief This contains the code for the Trident HAL ADC for T32CM11
///
///
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************

#include "tr_hal_adc.h"

// RAM for use with ADC for DMA
// 2 byte chunks, N of them
#define DMA_NUM_SAMPLES 2
static uint16_t g_adc_dma_memory[DMA_NUM_SAMPLES];

static tr_hal_adc_event_callback_t g_adc_event_handler[TR_HAL_NUM_ADC] = {NULL};

// for doing adc-reading-to-microvolt conversion
bool     adc_microvolt_conversion_enabled[TR_HAL_NUM_ADC] = {false};
uint32_t adc_reading_low_value[TR_HAL_NUM_ADC];
uint32_t adc_reading_high_value[TR_HAL_NUM_ADC];
uint32_t microvolt_low[TR_HAL_NUM_ADC];
uint32_t microvolt_high[TR_HAL_NUM_ADC];

// we need to keep track of if an ADC channel has been initialized
// and what pins were used (so we can free them up on uninit)
bool g_adc_init_complete[TR_HAL_NUM_ADC]       = {false};
tr_hal_gpio_pin_t g_p_pin_used[TR_HAL_NUM_ADC] = {false};
tr_hal_gpio_pin_t g_n_pin_used[TR_HAL_NUM_ADC] = {false};

// keep track of the last read value for each channel so we can return this
// from the READ API call
uint32_t g_last_read_value[TR_HAL_NUM_ADC] = {0};


/// ****************************************************************************
/// check_adc_settings_valid
/// helper for tr_hal_adc_init
/// ****************************************************************************
static tr_hal_status_t check_adc_settings_valid(tr_hal_adc_channel_id_t adc_channel_id,
                                                tr_hal_adc_settings_t*  adc_settings)
{
    tr_hal_status_t status;

    // settings can't be NULL
    if (adc_settings == NULL)
    {
        return TR_HAL_ERROR_NULL_PARAMS;
    }

    // ADC channel must be in range
    if (adc_channel_id > MAX_ADC_CHANNEL_ID)
    {
        return TR_HAL_INVALID_ADC_CHANNEL_ID;
    }

    // if we have ALREADY initialized this channel we return an error
    if (g_adc_init_complete[adc_channel_id] == true)
    {
        return TR_HAL_ADC_CHANNEL_ALREADY_INIT;
    }

    // check pin in use
    uint32_t pin_num = adc_settings->adc_pin_p.pin;

    // pin P needs to be set
    if ( (pin_num != ADC_VALID_PIN_CHOICE1)
        && (pin_num != ADC_VALID_PIN_CHOICE2)
        && (pin_num != ADC_VALID_PIN_CHOICE3)
        && (pin_num != ADC_VALID_PIN_CHOICE4)
       )
    {
        return TR_HAL_ADC_INVALID_P_PIN;
    }

    // check if P pin is already configured to another peripheral
    status = tr_hal_gpio_mgr_check_gpio(adc_settings->adc_pin_p);
    if (status != TR_HAL_SUCCESS)
    {
        return status;
    }

    pin_num = adc_settings->adc_pin_n.pin;
    // pin N needs to be set or set to invalid
    if ( (pin_num != TR_HAL_PIN_NOT_SET)
        && (pin_num != ADC_VALID_PIN_CHOICE1)
        && (pin_num != ADC_VALID_PIN_CHOICE2)
        && (pin_num != ADC_VALID_PIN_CHOICE3)
        && (pin_num != ADC_VALID_PIN_CHOICE4)
       )
    {
        return TR_HAL_ADC_INVALID_N_PIN;
    }

    // if N pin is set, check if N pin is already configured to another peripheral
    if (pin_num != TR_HAL_PIN_NOT_SET)
    {
        status = tr_hal_gpio_mgr_check_gpio(adc_settings->adc_pin_n);
        if (status != TR_HAL_SUCCESS)
        {
            return status;
        }
    }

    // check gain value
    if (adc_settings->vga_gain_in_dB > ADC_CONFIG_REG_MAX_GAIN_SETTING)
    {
        return TR_HAL_ADC_GAIN_TOO_HIGH;
    }

    // check P pull mode
    if (adc_settings->pin_p_pull_mode > TR_HAL_ADC_PULL_NOT_USED)
    {
        return TR_HAL_ADC_INVALID_P_PULL_MODE;
    }

    // check N pull mode
    if (adc_settings->pin_n_pull_mode > TR_HAL_ADC_PULL_NOT_USED)
    {
        return TR_HAL_ADC_INVALID_N_PULL_MODE;
    }

    // check aquisition time
    if (adc_settings->aquisition_time > TR_HAL_ADC_TIME_16)
    {
        return TR_HAL_ADC_INVALID_AQUISITION_TIME;
    }

    // check end delay time
    if (adc_settings->end_delay_time > TR_HAL_ADC_TIME_16)
    {
        return TR_HAL_ADC_INVALID_END_TIME;
    }

    // check clock to use
    if (adc_settings->clock_to_use > TR_HAL_ADC_USE_SLOW_CLOCK)
    {
        return TR_HAL_ADC_INVALID_CLOCK_TO_USE;
    }

    // check clock divisor
    if ( (adc_settings->clock_divider > TR_HAL_ADC_MAX_CLOCK_DIVISOR)
        || (adc_settings->clock_divider < TR_HAL_ADC_MIN_CLOCK_DIVISOR))
    {
        return TR_HAL_ADC_INVALID_CLOCK_DIVISOR;
    }

    // check oversample
    tr_hal_adc_oversample_t oversample = adc_settings->oversample;
    if ( (oversample != TR_HAL_ADC_REG_SAMPLE_NO_OVERSAMPLE)
        && (oversample != TR_HAL_ADC_REG_SAMPLE_OVERSAMPLE_2)
        && (oversample != TR_HAL_ADC_REG_SAMPLE_OVERSAMPLE_4)
        && (oversample != TR_HAL_ADC_REG_SAMPLE_OVERSAMPLE_8)
        && (oversample != TR_HAL_ADC_REG_SAMPLE_OVERSAMPLE_16)
        && (oversample != TR_HAL_ADC_REG_SAMPLE_OVERSAMPLE_32)
        && (oversample != TR_HAL_ADC_REG_SAMPLE_OVERSAMPLE_64)
        && (oversample != TR_HAL_ADC_REG_SAMPLE_OVERSAMPLE_128)
        && (oversample != TR_HAL_ADC_REG_SAMPLE_OVERSAMPLE_256)
       )
    {
        return TR_HAL_ADC_INVALID_OVERSAMPLE;
    }

    // check threshholds
    if (adc_settings->threshhold_low > 0x3FFF)
    {
        return TR_HAL_ADC_INVALID_LOW_THRESHHOLD;
    }
    if (adc_settings->threshhold_high > 0x3FFF)
    {
        return TR_HAL_ADC_INVALID_HIGH_THRESHHOLD;
    }

    // everything is ok
    return TR_HAL_SUCCESS;
}


/// ****************************************************************************
/// analog_init
///
/// this code is taken from the chip sample code
/// ****************************************************************************
static void tr_hal_adc_analog_init(void)
{
    ADC_REGISTERS->analog_settings0 = 0x7F708;

    ADC_REGISTERS->analog_settings1.bit.CFG_AUX_CMSEL = 0;
    ADC_REGISTERS->analog_settings1.bit.CFG_AUX_CMSEL = 5;
    ADC_REGISTERS->analog_settings1.bit.CFG_AUX_CMSEL = 0;
    ADC_REGISTERS->analog_settings1.bit.CFG_AUX_CMSEL = 1;

    ADC_REGISTERS->analog_settings1.bit.CFG_AUX_COMP = 3;
    ADC_REGISTERS->analog_settings1.bit.CFG_AUX_ADC_OUTPUTSTB = 0;
    ADC_REGISTERS->analog_settings1.bit.CFG_AUX_TEST_MODE = 0;
    ADC_REGISTERS->analog_settings1.bit.CFG_AUX_VLDO = 3;
    ADC_REGISTERS->analog_settings1.bit.CFG_AUX_CLK_SEL = 0;

    ADC_REGISTERS->analog_settings1.bit.CFG_AUX_PW = 0;
    ADC_REGISTERS->analog_settings1.bit.CFG_AUX_PW = 36;
    ADC_REGISTERS->analog_settings1.bit.CFG_AUX_PW = 0;
    ADC_REGISTERS->analog_settings1.bit.CFG_AUX_PW = 36;

    ADC_REGISTERS->analog_settings1.bit.CFG_EN_CLKAUX = 1; // enable
}

/// ****************************************************************************
/// INTERNAL FUNCTION tr_hal_analog_pin_enable
///
/// on CM11: GPIO 24 to 31 have ADC ability:
/// we don't have GPIO 24 to 27, just 28 to 31
///    GPIO28 = AIO4
///    GPIO29 = AIO5
///    GPIO30 = AIO6
///    GPIO31 = AIO7
///    not used = ADC_CONFIG_REG_x_CHANNEL_NONE
///
/// ****************************************************************************
static uint32_t tr_hal_analog_pin_enable(tr_hal_gpio_pin_t pin_struct,
                                         bool              is_p_pin)
{
    uint32_t pin_num = pin_struct.pin;

    switch (pin_num)
    {
        // pin 28 = AIO 4
        case TR_HAL_ADC_AIO4:
        {
            SYS_CTRL_CHIP_REGISTERS->analog_IO_enable |= TR_ADC_ENABLE_AIO4;
            if (is_p_pin)
            {
                return ADC_CONFIG_REG_P_CHANNEL_AIN_4;
            }
            else
            {
                return ADC_CONFIG_REG_N_CHANNEL_AIN_4;
            }
        }

        // pin 29 = AIO 5
        case TR_HAL_ADC_AIO5:
        {
            SYS_CTRL_CHIP_REGISTERS->analog_IO_enable |= TR_ADC_ENABLE_AIO5;
            if (is_p_pin)
            {
                return ADC_CONFIG_REG_P_CHANNEL_AIN_5;
            }
            else
            {
                return ADC_CONFIG_REG_N_CHANNEL_AIN_5;
            }
        }

        // pin 30 = AIO 6
        case TR_HAL_ADC_AIO6:
        {
            SYS_CTRL_CHIP_REGISTERS->analog_IO_enable |= TR_ADC_ENABLE_AIO6;
            if (is_p_pin)
            {
                return ADC_CONFIG_REG_P_CHANNEL_AIN_6;
            }
            else
            {
                return ADC_CONFIG_REG_N_CHANNEL_AIN_6;
            }
        }

        // pin 31 = AIO 7
        case TR_HAL_ADC_AIO7:
        {
            SYS_CTRL_CHIP_REGISTERS->analog_IO_enable |= TR_ADC_ENABLE_AIO7;
            if (is_p_pin)
            {
                return ADC_CONFIG_REG_P_CHANNEL_AIN_7;
            }
            else
            {
                return ADC_CONFIG_REG_N_CHANNEL_AIN_7;
            }
        }
    }

    if (is_p_pin)
    {
        return ADC_CONFIG_REG_P_CHANNEL_NONE;
    }
    // else
    return ADC_CONFIG_REG_N_CHANNEL_NONE;
}


/// ****************************************************************************
/// tr_hal_analog_pin_disable
/// ****************************************************************************
static void tr_hal_analog_pin_disable(tr_hal_gpio_pin_t pin_struct)
{
    uint32_t pin_num = pin_struct.pin;

    switch (pin_num)
    {
        // pin 28 = AIO 4
        case TR_HAL_ADC_AIO4:
        {
            SYS_CTRL_CHIP_REGISTERS->analog_IO_enable &= (uint32_t) (~TR_ADC_ENABLE_AIO4);
            return;
        }

        // pin 29 = AIO 5
        case TR_HAL_ADC_AIO5:
        {
            SYS_CTRL_CHIP_REGISTERS->analog_IO_enable &= (uint32_t) (~TR_ADC_ENABLE_AIO5);
            return;
        }

        // pin 30 = AIO 6
        case TR_HAL_ADC_AIO6:
        {
            SYS_CTRL_CHIP_REGISTERS->analog_IO_enable &= (uint32_t) (~TR_ADC_ENABLE_AIO6);
            return;
        }

        // pin 31 = AIO 7
        case TR_HAL_ADC_AIO7:
        {
            SYS_CTRL_CHIP_REGISTERS->analog_IO_enable &= (uint32_t) (~TR_ADC_ENABLE_AIO7);
            return;
        }
    }
}


/// ****************************************************************************
/// INTERNAL FUNCTION tr_hal_get_gain_reg_setting_from_value
/// return the gain register setting based on the gain value
/// ****************************************************************************
static uint32_t tr_hal_get_gain_reg_setting_from_value(uint32_t gain_value)
{
    // the VGA gain setting is 6 bits of an int32, bits 8 to 13
    //
    // upper 2 bits get multiplied by 6 dB
    //     max value of 3 -> 3 x 6 = 18
    //
    // lower 4 bits get multiplied by 3 dB
    //     max value of 15 -> 15 x 3 = 45
    //
    // so max value is 45 + 18 = 63
    //
    if (gain_value > ADC_CONFIG_REG_MAX_GAIN_SETTING)
    {
        // this should NOT happen because the gain setting is
        // checked prior to calling this. In case something
        // went terribly wrong, provide a default, which will
        // be the max value, which is 0b111111 starting at bit 8
        return 0x3F00;
    }

    // determine how many 6s are in the setting, max of 3
    uint32_t num_sixes = gain_value / 6;
    if (num_sixes > 3) { num_sixes = 3; }
    gain_value = gain_value - (num_sixes * 6);

    // we ignore any extra and so always round down if the user
    // passes it a number that is not a multiple of 3
    uint32_t num_threes = gain_value / 3;

    uint32_t value = (num_sixes << 12) + (num_threes << 8);

    return value;
}

/// ****************************************************************************
/// INTERNAL FUNCTION tr_hal_get_pull_reg_setting_from_enum
/// return the pull mode for both pins based on the enum values
/// ****************************************************************************
static uint32_t tr_hal_get_pull_reg_setting_from_enum(tr_hal_adc_pull_mode_t pin_p_pull_mode,
                                                      tr_hal_adc_pull_mode_t pin_n_pull_mode)
{
    uint32_t result = 0;

    // handle pin P pull mode
    switch (pin_p_pull_mode)
    {
        case TR_HAL_ADC_PULL_LOW:          result |= ADC_CONFIG_REG_P_CHAN_PULL_LOW; break;
        case TR_HAL_ADC_PULL_HIGH:         result |= ADC_CONFIG_REG_P_CHAN_PULL_HIGH; break;
        case TR_HAL_ADC_PULL_SINGLE_ENDED: result |= ADC_CONFIG_REG_P_CHAN_VCM_VOLTAGE; break;
        case TR_HAL_ADC_PULL_NOT_USED:     result |= ADC_CONFIG_REG_P_CHAN_PULL_NONE; break;
        default:                           result |= ADC_CONFIG_REG_P_CHAN_PULL_NONE;
    }

    // handle pin N pull mode
    switch (pin_n_pull_mode)
    {
        case TR_HAL_ADC_PULL_LOW:          result |= ADC_CONFIG_REG_N_CHAN_PULL_LOW; break;
        case TR_HAL_ADC_PULL_HIGH:         result |= ADC_CONFIG_REG_N_CHAN_PULL_HIGH; break;
        case TR_HAL_ADC_PULL_SINGLE_ENDED: result |= ADC_CONFIG_REG_N_CHAN_VCM_VOLTAGE; break;
        case TR_HAL_ADC_PULL_NOT_USED:     result |= ADC_CONFIG_REG_N_CHAN_PULL_NONE; break;
        default:                           result |= ADC_CONFIG_REG_N_CHAN_PULL_NONE;
    }

    return result;
}


/// ****************************************************************************
/// INTERNAL FUNCTION tr_hal_get_aquisition_reg_setting_from_enum
/// return the aquisition time register setting based on the enum value
/// ****************************************************************************
static uint32_t tr_hal_get_aquisition_reg_setting_from_enum(tr_hal_time_t aqu_time_enum)
{
    switch (aqu_time_enum)
    {
        case TR_HAL_ADC_TIME_HALF: return ADC_CONFIG_REG_AQUISITION_TIME_0p3_uS;
        case TR_HAL_ADC_TIME_1:    return ADC_CONFIG_REG_AQUISITION_TIME_1_uS;
        case TR_HAL_ADC_TIME_2:    return ADC_CONFIG_REG_AQUISITION_TIME_2_uS;
        case TR_HAL_ADC_TIME_3:    return ADC_CONFIG_REG_AQUISITION_TIME_3_uS;
        case TR_HAL_ADC_TIME_4:    return ADC_CONFIG_REG_AQUISITION_TIME_4_uS;
        case TR_HAL_ADC_TIME_8:    return ADC_CONFIG_REG_AQUISITION_TIME_8_uS;
        case TR_HAL_ADC_TIME_12:   return ADC_CONFIG_REG_AQUISITION_TIME_12_uS;
        case TR_HAL_ADC_TIME_16:   return ADC_CONFIG_REG_AQUISITION_TIME_16_uS;
        default:                   return ADC_CONFIG_REG_AQUISITION_TIME_16_uS;
    }
    return ADC_CONFIG_REG_AQUISITION_TIME_16_uS;
}


/// ****************************************************************************
/// INTERNAL FUNCTION tr_hal_get_end_delay_reg_setting_from_enum
/// return the end delay time register setting based on the enum value
/// ****************************************************************************
static uint32_t tr_hal_get_end_delay_reg_setting_from_enum(tr_hal_time_t end_time_enum)
{
    switch (end_time_enum)
    {
        case TR_HAL_ADC_TIME_HALF: return ADC_CONFIG_REG_END_DELAY_TIME_0p3_uS;
        case TR_HAL_ADC_TIME_1:    return ADC_CONFIG_REG_END_DELAY_TIME_1_uS;
        case TR_HAL_ADC_TIME_2:    return ADC_CONFIG_REG_END_DELAY_TIME_2_uS;
        case TR_HAL_ADC_TIME_3:    return ADC_CONFIG_REG_END_DELAY_TIME_3_uS;
        case TR_HAL_ADC_TIME_4:    return ADC_CONFIG_REG_END_DELAY_TIME_4_uS;
        case TR_HAL_ADC_TIME_8:    return ADC_CONFIG_REG_END_DELAY_TIME_8_uS;
        case TR_HAL_ADC_TIME_12:   return ADC_CONFIG_REG_END_DELAY_TIME_12_uS;
        case TR_HAL_ADC_TIME_16:   return ADC_CONFIG_REG_END_DELAY_TIME_16_uS;
        default:                   return ADC_CONFIG_REG_END_DELAY_TIME_16_uS;
    }
    return ADC_CONFIG_REG_END_DELAY_TIME_16_uS;
}


/// ****************************************************************************
/// INTERNAL FUNCTION tr_hal_get_clock_to_use_reg_setting_from_enum
/// return the clock to use register setting based on the enum value
/// ****************************************************************************
static uint32_t tr_hal_get_clock_to_use_reg_setting_from_enum(tr_hal_adc_clock_t clock)
{
    switch (clock)
    {
        case TR_HAL_ADC_USE_SYSTEM_CLOCK: return ADC_REG_TIMER_USE_SYSTEM_CLOCK;
        case TR_HAL_ADC_USE_SLOW_CLOCK:   return ADC_REG_TIMER_USE_SLOW_CLOCK;
        default:                          return ADC_REG_TIMER_USE_SLOW_CLOCK;
    }
    return ADC_REG_TIMER_USE_SLOW_CLOCK;
}


/// ****************************************************************************
/// tr_hal_adc_init
/// ****************************************************************************
tr_hal_status_t tr_hal_adc_init(tr_hal_adc_channel_id_t adc_channel_id,
                                tr_hal_adc_settings_t*  adc_settings)
{
    tr_hal_status_t status;

    status = check_adc_settings_valid(adc_channel_id,
                                      adc_settings);
    if (status != TR_HAL_SUCCESS)
    {
        return status;
    }

    // start by disable and reset ADC
    ADC_REGISTERS->control_enable = ADC_REG_ENABLE_ADC_DISABLE;
    ADC_REGISTERS->control_reset = ADC_REG_RESET_ADC
                                    | ADC_REG_RESET_FIFO;

    // setup analog settings
    tr_hal_adc_analog_init();

    // set the pin desired to ANALOG mode and get the value
    // that should be passed to the ch_x_config
    uint32_t p_pin_setting = tr_hal_analog_pin_enable(adc_settings->adc_pin_p,
                                                      true);
    uint32_t n_pin_setting = tr_hal_analog_pin_enable(adc_settings->adc_pin_n,
                                                      false);

    // figure out the gain register setting from the gain value passed in
    uint32_t gain_setting = tr_hal_get_gain_reg_setting_from_value(adc_settings->vga_gain_in_dB);

    // One shot mode enables only one channel of SADC_PNSEL_CH0~CH9. Configure register SADC_SET0.[0]
    // sets the trigger source is from the control register. The MCU enables the controller
    // via SADC_CTL0 and triggers the mode controller via SADC_CTRL2 by single shot. It sends a start
    // request to the digital controller. Whenever the digital controller receives start request, it begins
    // sampling the SADC result according to the channel configurations.

    // we only support one-shot mode for now
    if (adc_settings->mode != TR_HAL_ADC_MODE_ONE_SHOT)
    {
        return TR_HAL_ADC_UNSUPPORTED_MODE;
    }

    // no pull on pin P
    tr_hal_gpio_pin_t aio_pin_p = { adc_settings->adc_pin_p.pin };
    tr_hal_gpio_set_pull_mode(aio_pin_p, TR_HAL_PULLOPT_PULL_NONE);

    // no pull on pin N
    if (adc_settings->adc_pin_n.pin != 255)
    {
        tr_hal_gpio_pin_t aio_pin_n = { adc_settings->adc_pin_n.pin };
        tr_hal_gpio_set_pull_mode(aio_pin_n, TR_HAL_PULLOPT_PULL_NONE);
    }

    uint32_t pull_mode_register = tr_hal_get_pull_reg_setting_from_enum(adc_settings->pin_p_pull_mode, adc_settings->pin_n_pull_mode);
    uint32_t aquisition_time_register = tr_hal_get_aquisition_reg_setting_from_enum(adc_settings->aquisition_time);
    uint32_t end_delay_time_register = tr_hal_get_end_delay_reg_setting_from_enum(adc_settings->end_delay_time);
    uint32_t clock_to_use_register = tr_hal_get_clock_to_use_reg_setting_from_enum(adc_settings->clock_to_use);
    uint32_t clock_divider = (adc_settings->clock_divider) - 1;
    clock_divider = clock_divider << ADC_REG_TIMER_CLOCK_DIV_SHIFT;
    uint32_t burst_mode_register = ADC_BURST_REG_DISABLE_BURST;
    if (adc_settings->enable_burst_mode)
    {
        burst_mode_register = ADC_BURST_REG_ENABLE_BURST;
    }

    // setup one shot mode on the ADC channel selected
    ADC_REGISTERS->ch_x_setting[adc_channel_id].ch_x_config = (p_pin_setting
                                                            | n_pin_setting
                                                            | gain_setting
                                                            | ADC_CONFIG_REG_SELECT_REF_IN
                                                            | pull_mode_register
                                                            | aquisition_time_register
                                                            | end_delay_time_register
                                                            );
    ADC_REGISTERS->ch_x_setting[adc_channel_id].ch_x_burst = burst_mode_register;
    ADC_REGISTERS->ch_x_setting[adc_channel_id].ch_x_threshholds = (uint32_t)((adc_settings->threshhold_low << 16)
                                                                 | adc_settings->threshhold_low);

    // set for SOFTWARE trigger (not timer)
    ADC_REGISTERS->clock_settings = ADC_REG_TIMER_RATE_DEPENDS_ON_SOFTWARE
                                     | clock_to_use_register
                                     | ADC_REG_TIMER_RISING_EDGE
                                     | clock_divider;
    // no oversample
    ADC_REGISTERS->oversample_settings = adc_settings->resolution
                                          | adc_settings->oversample;

    // setup DMA
    ADC_REGISTERS->dma_buffer_addr = (uint32_t) g_adc_dma_memory;
    uint16_t block_size = 2;
    uint16_t segment_size = 2;
    ADC_REGISTERS->dma_buffer_size = ( (block_size << 16) | segment_size);

    // enable/start DMA
    ADC_REGISTERS->enable_dma = ADC_ENDMA_REG_ENABLE_DMA;
    ADC_REGISTERS->reset_dma = ADC_ENDMA_REG_RESET_DMA;

    // enable ADC
    ADC_REGISTERS->control_enable = ADC_REG_ENABLE_ADC_ENABLE
                                    | ADC_REG_ENABLE_FREE_RUN_CLOCK
                                    | ADC_REG_ENABLE_FREE_RUN_ENGINE;
    // reset ADC
    ADC_REGISTERS->control_reset = ADC_REG_RESET_ADC
                                    | ADC_REG_RESET_FIFO;

    // **********************************
    // handle interrupts
    // **********************************
    if (adc_settings->interrupt_enabled)
    {
        // if we enable interrupts then the event handler needs to be defined
        if (adc_settings->event_handler_fx == NULL)
        {
            return TR_ADC_ERROR_NEED_EVENT_HANDLER;
        }

        // interrupt priority needs to be valid
        status = tr_hal_check_interrupt_priority(adc_settings->interrupt_priority);
        if (status != TR_HAL_SUCCESS)
        {
            return status;
        }

        // set the event handler
        g_adc_event_handler[adc_channel_id] = adc_settings->event_handler_fx;

        // set the interrupt priority
        NVIC_SetPriority(Sadc_IRQn, adc_settings->interrupt_priority);
        NVIC_EnableIRQ(Sadc_IRQn);

        // enable the specific interrupts, we want all interrupts. This is a
        // mask, so we set which interrupts to disable, which is none
        ADC_REGISTERS->interrupt_enable = 0;
    }

    // are we enabling conversion of an ADC reading to uV?
    // if so, the values could be pulled from NV from cal values
    // or set constant if there isn't enough flux and put in settings
    adc_microvolt_conversion_enabled[adc_channel_id] = false;
    if (adc_settings->enable_microvolt_conversion)
    {
        adc_microvolt_conversion_enabled[adc_channel_id] = true;
        adc_reading_low_value[adc_channel_id] =  adc_settings->min_expected_adc_reading;
        adc_reading_high_value[adc_channel_id] = adc_settings->max_expected_adc_reading;
        microvolt_low[adc_channel_id] = adc_settings->min_microvolt_value;
        microvolt_high[adc_channel_id] = adc_settings->max_microvolt_value;
    }

    // if the caller set the start now flag then start the ADC
    if (adc_settings->start_now)
    {
        // software start ADC
        ADC_REGISTERS->control_start = ADC_REG_START_ADC;
    }

    // remember the P and N pins used
    g_p_pin_used[adc_channel_id] = (tr_hal_gpio_pin_t) { adc_settings->adc_pin_p.pin };
    g_n_pin_used[adc_channel_id] = (tr_hal_gpio_pin_t) { adc_settings->adc_pin_n.pin };

    // init has been completed
    g_adc_init_complete[adc_channel_id] = true;

    // tell the GPIO manager that P pin is in use
    tr_hal_gpio_mgr_reserve_gpio(g_p_pin_used[adc_channel_id], TR_HAL_GPIO_SET_FOR_ADC);

    // if N pin is set, tell the GPIO manager that N pin is in use
    if (n_pin_setting != TR_HAL_PIN_NOT_SET)
    {
        tr_hal_gpio_mgr_reserve_gpio(g_n_pin_used[adc_channel_id], TR_HAL_GPIO_SET_FOR_ADC);
    }

    return TR_HAL_SUCCESS;
}


/// ****************************************************************************
/// check if all ADC channels are disabled
/// this is helpful in uninit since we should not disable interrupts if any
/// channel is still enabled
/// ****************************************************************************
static bool are_all_adc_channels_uninitialized(void)
{
    for (uint16_t i=0; i<TR_HAL_NUM_ADC; i++)
    {
        if (g_adc_init_complete[i] == true)
        {
            return false;
        }
    }
    return true;
}


/// ****************************************************************************
/// tr_hal_adc_uninit
/// ****************************************************************************
tr_hal_status_t tr_hal_adc_uninit(tr_hal_adc_channel_id_t adc_channel_id)
{
    // ADC channel must be in range
    if (adc_channel_id > MAX_ADC_CHANNEL_ID)
    {
        return TR_HAL_INVALID_ADC_CHANNEL_ID;
    }

    // ADC channel must be initialized in order to uninit
    if (g_adc_init_complete[adc_channel_id] == false)
    {
        return TR_HAL_ADC_CHANNEL_NOT_INITIALIZED;
    }

    // we set this a bit early because it needs to be set before calling are_all_adc_channels_disabled
    g_adc_init_complete[adc_channel_id] = false;

    // if all channels are uninit then disabled interrupts
    if (are_all_adc_channels_uninitialized() == true)
    {
        // turn off interrupts
        NVIC_ClearPendingIRQ(Sadc_IRQn);
        NVIC_DisableIRQ(Sadc_IRQn);
        ADC_REGISTERS->interrupt_enable = 0;

        // turn OFF ADC
        ADC_REGISTERS->control_enable = ADC_REG_ENABLE_ADC_DISABLE;
    }

    // disable the channel by setting the config back to default
    ADC_REGISTERS->ch_x_setting[adc_channel_id].ch_x_config = ADC_CONFIG_REG_CLEAR_VALUE;

    // disable the one analog input
    tr_hal_analog_pin_disable(g_p_pin_used[adc_channel_id]);
    // (it is ok to do this even if N pin is not used, the fx just does nothing in this case)
    tr_hal_analog_pin_disable(g_n_pin_used[adc_channel_id]);

    // clear global state
    g_adc_event_handler[adc_channel_id] = NULL;

    // release the GPIOs used
    tr_hal_gpio_mgr_release_gpio(g_p_pin_used[adc_channel_id]);
    tr_hal_gpio_mgr_release_gpio(g_n_pin_used[adc_channel_id]);

    return TR_HAL_SUCCESS;
}


/// ****************************************************************************
/// tr_hal_adc_set_for_initial_state_all_disabled
/// ****************************************************************************
tr_hal_status_t tr_hal_adc_set_for_initial_state_all_disabled(void)
{
    // this can only be called if all channels are currently uninitialized
    if (are_all_adc_channels_uninitialized() == false)
    {
        return TR_HAL_ADC_CHANNEL_ALREADY_INIT;
    }

    // turn OFF ADC
    ADC_REGISTERS->control_enable = ADC_REG_ENABLE_ADC_DISABLE;

    // disable all GPIO as analog
    SYS_CTRL_CHIP_REGISTERS->analog_IO_enable = 0;
    
    // disable every channel by setting the channel config back to default
    for (uint16_t i=0; i<TR_HAL_NUM_ADC; i++)
    {
        ADC_REGISTERS->ch_x_setting[i].ch_x_config = ADC_CONFIG_REG_CLEAR_VALUE;
    }

    return TR_HAL_SUCCESS;
}


/// ****************************************************************************
/// tr_hal_adc_start
/// when we start the ADC, we start it for all channels, which is why we don't
/// need a tr_hal_adc_channel_id_t argument
/// ****************************************************************************
tr_hal_status_t tr_hal_adc_start(void)
{
    // check that at least one ADC channel has been initialized
    if (are_all_adc_channels_uninitialized() == true)
    {
        return TR_HAL_ADC_CHANNEL_NOT_INITIALIZED;
    }

    // software start ADC
    ADC_REGISTERS->control_start = ADC_REG_START_ADC;

    return TR_HAL_SUCCESS;
}


/// ***************************************************************************
/// convert_ADC_reading_to_voltage
/// ***************************************************************************
static uint32_t convert_ADC_reading_to_voltage(uint32_t                adc_reading,
                                               tr_hal_adc_channel_id_t adc_channel_id)
{
    // we expect a min of adc_reading_low_value
    if (adc_reading <= adc_reading_low_value[adc_channel_id])
    {
        return microvolt_low[adc_channel_id];
    }

    // we expect a max of adc_reading_high_value
    if (adc_reading >= adc_reading_high_value[adc_channel_id])
    {
        return microvolt_high[adc_channel_id];
    }

    // at this point we are between adc_reading_low_value and adc_reading_high_value
    adc_reading = adc_reading - adc_reading_low_value[adc_channel_id];

    // round to nearest 100
    adc_reading = (adc_reading / 100) * 100;

    // uV range
    uint32_t uv_range = microvolt_high[adc_channel_id] - microvolt_low[adc_channel_id];

    // adc reading range
    uint32_t adc_reading_range = adc_reading_high_value[adc_channel_id] - adc_reading_low_value[adc_channel_id];

    // what percent of range is the reading
    uint32_t percent = adc_reading / (adc_reading_range / 100);

    // multiply uV range by percent and add in low value
    uint32_t result = ((uv_range * percent) / 100) + microvolt_low[adc_channel_id];

    // we have uV
    return result;
}


/// ****************************************************************************
/// tr_hal_adc_read_voltage
/// ****************************************************************************
tr_hal_status_t tr_hal_adc_read_voltage(tr_hal_adc_channel_id_t adc_channel_id,
                                        uint32_t*               microVolts)
{
    // check for valid ADC channel ID
    if (adc_channel_id > MAX_ADC_CHANNEL_ID)
    {
        return TR_HAL_INVALID_ADC_CHANNEL_ID;
    }

    // result location can't be NULL
    if (microVolts == NULL)
    {
        return TR_HAL_ERROR_NULL_PARAMS;
    }

    // check that this ADC channel has been initialized
    if (g_adc_init_complete[adc_channel_id] == false)
    {
        (*microVolts) = 0;
        return TR_HAL_ADC_CHANNEL_NOT_INITIALIZED;
    }

    // if conversion is NOT enabled (meaning we don't have any info on how to
    // convert) then just return zero
    if (adc_microvolt_conversion_enabled[adc_channel_id] == false)
    {
        (*microVolts) = 0;
        return TR_HAL_ADC_CONV_DISABLED;
    }

    // convert and return the value
    uint32_t raw_result = g_last_read_value[adc_channel_id];

    (*microVolts) = convert_ADC_reading_to_voltage(raw_result, 
                                                   adc_channel_id);
    return TR_HAL_SUCCESS;
}


/// ****************************************************************************
/// tr_hal_adc_read
/// ****************************************************************************
tr_hal_status_t tr_hal_adc_read(tr_hal_adc_channel_id_t adc_channel_id,
                                uint32_t*               result)
{
    // check for valid ADC channel ID
    if (adc_channel_id > MAX_ADC_CHANNEL_ID)
    {
        return TR_HAL_INVALID_ADC_CHANNEL_ID;
    }

    // result ptr can't be NULL
    if (result == NULL)
    {
        return TR_HAL_ERROR_NULL_PARAMS;
    }

    // check that this ADC channel has been initialized
    if (g_adc_init_complete[adc_channel_id] == false)
    {
        (*result) = 0;
        return TR_HAL_ADC_CHANNEL_NOT_INITIALIZED;
    }

    (*result) = g_last_read_value[adc_channel_id];

    return TR_HAL_SUCCESS;
}


/// ****************************************************************************
/// INTERRUPT HANDLER
/// ****************************************************************************
void Sadc_Handler(void)
{
    // bitmask to send to event_handler_fx
    uint32_t event_bitmask = 0;

    // for reading the ADC result
    uint32_t raw_result       = 0;
    uint32_t converted_result = 0;

    // figure out what interrupts have occurred in the INT STATUS register
    uint32_t int_status = ADC_REGISTERS->interrupt_status;

    // for keeping track of the ADC channel
    tr_hal_adc_channel_id_t adc_channel_id = TR_HAL_ADC_CHANNEL_NONE;

    // clear interrupts
    ADC_REGISTERS->interrupt_clear = TR_HAL_ADC_INTERRUPT_ALL;

    // ****************************************
    // interrupt: done and valid
    // ****************************************
    if ( (int_status & TR_HAL_ADC_INTERRUPT_DONE)
        && (int_status & TR_HAL_ADC_INTERRUPT_VALID) )
    {
        // get the result
        raw_result = ADC_REGISTERS->result_oversample & TR_HAL_ADC_R0_RESULT_MASK;

        // channel 0 result
        if (int_status & TR_HAL_ADC_INTERRUPT_CHAN_0)
        {
            event_bitmask |= TR_HAL_ADC_EVENT_CH_0_RESULT;
            adc_channel_id = ADC_CHANNEL_0_ID;
            converted_result = convert_ADC_reading_to_voltage(raw_result, ADC_CHANNEL_0_ID);
            g_last_read_value[ADC_CHANNEL_0_ID] = raw_result;
        }
        // channel 1 result
        else if (int_status & TR_HAL_ADC_INTERRUPT_CHAN_1)
        {
            event_bitmask |= TR_HAL_ADC_EVENT_CH_1_RESULT;
            adc_channel_id = ADC_CHANNEL_1_ID;
            converted_result = convert_ADC_reading_to_voltage(raw_result, ADC_CHANNEL_1_ID);
            g_last_read_value[ADC_CHANNEL_1_ID] = raw_result;
        }
        // channel 2 result
        else if (int_status & TR_HAL_ADC_INTERRUPT_CHAN_2)
        {
            event_bitmask |= TR_HAL_ADC_EVENT_CH_2_RESULT;
            adc_channel_id = ADC_CHANNEL_2_ID;
            converted_result = convert_ADC_reading_to_voltage(raw_result, ADC_CHANNEL_2_ID);
            g_last_read_value[ADC_CHANNEL_2_ID] = raw_result;
        }
        // channel 3 result
        else if (int_status & TR_HAL_ADC_INTERRUPT_CHAN_3)
        {
            event_bitmask |= TR_HAL_ADC_EVENT_CH_3_RESULT;
            adc_channel_id = ADC_CHANNEL_3_ID;
            converted_result = convert_ADC_reading_to_voltage(raw_result, ADC_CHANNEL_3_ID);
            g_last_read_value[ADC_CHANNEL_3_ID] = raw_result;
        }
    }

    // ****************************************
    // interrupt: this means done with ALL channels
    // ****************************************
    if (int_status & TR_HAL_ADC_INTERRUPT_MODE_DONE)
    {
        event_bitmask |= TR_HAL_ADC_EVENT_ALL_CH_DONE;
    }

    // ****************************************
    // interrupt: DMA
    // ****************************************
    if (int_status & TR_HAL_ADC_INTERRUPT_DMA)
    {
        event_bitmask |= TR_HAL_ADC_EVENT_DMA;
    }

    // ****************************************
    // if we got a channel, call the user event function, passing the event bitmask and result
    // ****************************************
    if (adc_channel_id != TR_HAL_ADC_CHANNEL_NONE)
    {
        if (g_adc_event_handler[adc_channel_id] != NULL)
        {
            (g_adc_event_handler[adc_channel_id])(raw_result,
                                                  converted_result,
                                                  event_bitmask,
                                                  int_status);
        }
    }
}

