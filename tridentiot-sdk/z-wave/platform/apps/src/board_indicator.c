/// ***************************************************************************
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2024 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************

/**
 * @file board_indicator.c
 */
#include "tr_platform.h"
#include <timer.h>
#include <sysfun.h>
#include <sysctrl.h>

#include "apps_hw.h"
#include "board_indicator.h"
#include "tr_hal_gpio.h"
//#define DEBUGPRINT // NOSONAR
#include "DebugPrint.h"
#include "Assert.h"

#define LED_TIMER       3

#define LED_ON     1
#define LED_OFF    0
#define RUN_FOREVER   (uint32_t)-1

static const timer_config_mode_t  led_timer_cfg = {.mode = TIMER_FREERUN_MODE, .prescale = TIMER_PRESCALE_1, .int_en = true};

static uint8_t m_indicator_gpio;

/**
 * @brief Defines the configuration of the indicator.
 *
 */
typedef struct _indicator_cfg_t
{
  bool off_time_state;
  bool indicator_initialized;
  bool indicator_active_from_cc;
  uint32_t off_time_ms;
  uint32_t on_time_ms;
  uint32_t repeat_count;
  uint32_t led_timer_freq;
/*
 * Used for deactivate the indicator LED.
 * Valid values are 0 or 1.
 * Every indicator LED blink period starts by setting the indicator GPIO
 * to this value for off_time_ms (@ref Board_IndicatorControl).
 */
  uint8_t indicator_led_off_value;
} indicator_cfg_t;

volatile static indicator_cfg_t m_indicator_cfg= {.off_time_state = true,
                                                  .indicator_initialized = false,
                                                  .indicator_active_from_cc = false,
                                                  .off_time_ms = 0,
                                                  .on_time_ms = 0,
                                                  .repeat_count = 0,
                                                  .led_timer_freq = 0,
                                                  .indicator_led_off_value = TR_HAL_GPIO_LEVEL_HIGH
                                                  };
static void Led_set(uint8_t state)
{
  tr_hal_gpio_pin_t _pin = {.pin = m_indicator_gpio};
  if ((state & ~m_indicator_cfg.indicator_led_off_value) | (~state & m_indicator_cfg.indicator_led_off_value))
  {
    tr_hal_gpio_set_output(_pin, TR_HAL_GPIO_LEVEL_HIGH);
  }
  else
  {
    tr_hal_gpio_set_output(_pin, TR_HAL_GPIO_LEVEL_LOW);
  }
}

static void led_timer_isr(uint32_t timer_id)
{
  uint32_t period;
  if (m_indicator_cfg.off_time_state)
  {
     m_indicator_cfg.off_time_state = false;
     period = m_indicator_cfg.on_time_ms;
     Led_set(LED_ON);
  }
  else
  {
     m_indicator_cfg.off_time_state = true;
     period = m_indicator_cfg.off_time_ms;
     Led_set(LED_OFF);
     if (RUN_FOREVER !=m_indicator_cfg.repeat_count)
     {
       m_indicator_cfg.repeat_count--;
     }
  }
  if (m_indicator_cfg.repeat_count)
  {
    Timer_Load(LED_TIMER, period);
  }
  else
  {
    Timer_Stop(LED_TIMER);
  }
}

/*
 * An application might need to customize the CC Indicator behavior together
 * with some other functionality. Hence, define Board_IndicateStatus() weakly.
 */
__attribute__((weak))
void Board_IndicateStatus(board_status_t status)
{
  if (BOARD_STATUS_LEARNMODE_ACTIVE == status)
  {
    /* Blink indicator LED */
    Board_IndicatorControl(100, 900, 0, false);
  }
  else
  {
    /* Turn off the indicator LED */
    Board_IndicatorControl(0, 0, 0, false);
  }
}

void Board_IndicatorInit(void)
{
  if (!m_indicator_cfg.indicator_initialized)
  {
    /* Configure LED */
    Timer_Open(LED_TIMER,
               led_timer_cfg,
               led_timer_isr);
    m_indicator_cfg.led_timer_freq = 16000; // timer clk is 16kHz
    m_indicator_cfg.indicator_led_off_value = board_indicator_led_off_gpio_state();
    m_indicator_gpio = board_indicator_gpio_get();
    tr_hal_gpio_pin_t _pin = {.pin = m_indicator_gpio};
    tr_hal_gpio_set_mode(_pin, TR_HAL_GPIO_MODE_GPIO);

    tr_hal_gpio_settings_t pin_cfg = DEFAULT_GPIO_OUTPUT_CONFIG;
    pin_cfg.output_level = m_indicator_cfg.indicator_led_off_value;
    tr_hal_gpio_init(_pin, &pin_cfg);
    m_indicator_cfg.indicator_initialized = true;
  }
}

/*
 * An application might need to customize the CC Indicator behavior together
 * with some other functionality. Hence, define Board_IndicatorControl() weakly.
 */
__attribute__((weak))
bool Board_IndicatorControl(uint32_t on_time_ms, uint32_t off_time_ms,
                            uint32_t num_blinks, bool called_from_indicator_cc)
{
  if (!m_indicator_cfg.indicator_initialized)
  {
    ASSERT(false);
    return false;
  }

  if (0 == on_time_ms)
  {
    // On time is zero -> turn off the indicator
    Timer_Stop(LED_TIMER);

    m_indicator_cfg.indicator_active_from_cc = false;
    Led_set(LED_OFF);
  }
  else
  {
    m_indicator_cfg.off_time_state = false;
    m_indicator_cfg.repeat_count = num_blinks ? num_blinks : RUN_FOREVER;
    m_indicator_cfg.off_time_ms = (off_time_ms * m_indicator_cfg.led_timer_freq) /1000;
    m_indicator_cfg.on_time_ms = (on_time_ms * m_indicator_cfg.led_timer_freq) /1000;
    Led_set(LED_ON);

    Timer_Start(LED_TIMER, m_indicator_cfg.on_time_ms);

    m_indicator_cfg.indicator_active_from_cc = called_from_indicator_cc;
  }
  return true;
}

bool Board_IsIndicatorActive(void)
{
  return m_indicator_cfg.indicator_active_from_cc;
}

/*
 * An application might need to customize the CC Indicator behavior together
 * with some other functionality. Hence, define cc_indicator_handler() weakly.
 */
void cc_indicator_handler(uint32_t on_time_ms, uint32_t off_time_ms, uint32_t num_blinks)
{
    Board_IndicatorControl(on_time_ms, off_time_ms, num_blinks, true);
}

void board_indicator_gpio_set( uint8_t gpio)
{
  m_indicator_gpio = gpio;
}
