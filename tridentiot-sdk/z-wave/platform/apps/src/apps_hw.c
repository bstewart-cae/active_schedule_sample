/// ***************************************************************************
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2024 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************

/**
 * @file apps_hw.c
 */
#include <tr_platform.h>
#include <tr_hal_gpio.h>
#include <app_hw.h>
#include <apps_hw.h>

#include <sysfun.h>
#include <sysctrl.h>

//#define DEBUGPRINT // NOSONAR
#include <DebugPrint.h>

#include <zaf_event_distributor_soc.h>
#include <AppTimer.h>

#include <ZW_system_startup_api.h>
#include <zpal_gpio_private.h>
#include <zpal_power_manager.h>

#define PULL_DOWN_100K PULLDOWN_100K
#define PULL_UP_100K PULLUP_100K

/* Button event durations in milliseconds */
#define BUTTON_SHORT_PRESS_TIME   20
#define BUTTON_HOLD_TIME          300
#define BUTTON_LONG_PRESS_TIME    5000
#define MAX_HOLD_TIME             20000
#define DEBOUNCE_FILTER_TIME      10

static const gpio_config_t * m_gpio_config = NULL;
static gpio_info_t * m_gpio_info = NULL;
static uint8_t m_gpio_size = 0;
// De-bounce Timer. Timer is only running while one or more buttons are pressed.
static SSwTimer m_button_timer;
static volatile bool button_timer_is_running;

// Time duration of current button timer.
static volatile uint32_t m_current_button_time;
static zpal_pm_handle_t gpio_power_lock;

static void PushButtonEventHandler(uint8_t event,bool cancel_pwr_lock, bool from_isr)
{
  if (cancel_pwr_lock)
  {
    zpal_pm_cancel(gpio_power_lock);
  }
  if (from_isr)
  {
    zaf_event_distributor_enqueue_app_event_from_isr(event);
  }
  else
  {
    zaf_event_distributor_enqueue_app_event(event);
  }
}

gpio_state_t GetGpioState(uint8_t button)
{

  tr_hal_level_t gpio_level = 0;
  tr_hal_gpio_pin_t _pin = {.pin = m_gpio_config[button].gpio_no};
  tr_hal_gpio_read_input(_pin, &gpio_level);
  if (m_gpio_config[button].on_value == gpio_level)
  {
      return GPIO_DOWN;
  }
  return GPIO_UP;
}

/**
 * Update push button State.
 *
 * Returns Time in milliseconds the button was pressed
 */
static uint32_t PushbuttonUpdateState(uint8_t gpio_no, gpio_state_t new_state, bool from_isr)
{
  if (GPIO_UP == m_gpio_info[gpio_no].gpio_state)
  {
    // Button was up
    if (GPIO_DOWN == new_state)
    {
      // and now it is down
      if(!button_timer_is_running)
      {
        // Start debounce timer
        m_current_button_time = 0;
        button_timer_is_running = true;
        if (from_isr)
        {
          TimerStartFromISR(&m_button_timer, DEBOUNCE_FILTER_TIME);
        }
        else
        {
          TimerStart(&m_button_timer, DEBOUNCE_FILTER_TIME);
        }
      }
      /* Record the state and the time button was pressed */
      m_gpio_info[gpio_no].gpio_state = GPIO_DOWN;
      m_gpio_info[gpio_no].gpio_down_count = m_current_button_time;
    }
    return 0;
  }
  // button was down
  if (GPIO_DOWN != new_state)
  {
    // and now it changed
    m_gpio_info[gpio_no].gpio_state = GPIO_UP;
    /* Retun the times the button was down */
    return (m_current_button_time - m_gpio_info[gpio_no].gpio_down_count);
  }

  return 0;
}

void GpioEventHandler(uint8_t button, bool from_isr)
{
  if (GPIO_DOWN != GetGpioState(button)) // Button pressed down
  {
    uint32_t button_down_duration = PushbuttonUpdateState(button, GPIO_UP, from_isr);

    if (BUTTON_LONG_PRESS_TIME <= button_down_duration)
    {
      // Long press
      PushButtonEventHandler(m_gpio_config[button].long_event, true, from_isr);
    }
    else if ((BUTTON_SHORT_PRESS_TIME <= button_down_duration) && (BUTTON_HOLD_TIME >= button_down_duration))
    {
      // Short press (not wakeup)
      PushButtonEventHandler(m_gpio_config[button].short_event, true, from_isr);
    }

    if (BUTTON_HOLD_TIME <= button_down_duration)
    {
      // released
      PushButtonEventHandler(m_gpio_config[button].release_event, true, from_isr);
    }
    else if (BUTTON_SHORT_PRESS_TIME > button_down_duration)
    {
      zpal_pm_cancel(gpio_power_lock);
    }
  }
  else
  {
    // Button is down
    PushbuttonUpdateState(button, GPIO_DOWN, from_isr);
  }
}

static void gpio_event_handler (tr_hal_gpio_pin_t pin, tr_hal_gpio_event_t event)
{
  uint32_t pin_num = pin.pin;
  for (uint8_t i = 0; i < m_gpio_size; i ++)
  {
    if (m_gpio_config[i].gpio_no == pin_num)
    {
      zpal_pm_stay_awake(gpio_power_lock, 0);
      GpioEventHandler(i, true);
      break;
    }
  }
}


/**
 * Checks if a button has been DOWN long enough to change state to HOLD
 *
 * NB: inline since called from timer call back every 10 ms.

 * @param button          Button identifier.
 * @param event           system event
 *
 */
static inline void ButtonCheckHold(uint8_t button)
{
  if (GPIO_DOWN == m_gpio_info[button].gpio_state)
  {
    /* If the button has been down for more than BUTTON_HOLD_TIME
     * Change its state to HOLD and report it to the application
     */
    if (BUTTON_HOLD_TIME <= (m_current_button_time - m_gpio_info[button].gpio_down_count))
    {
      m_gpio_info[button].gpio_state = GPIO_HOLD;
      PushButtonEventHandler(m_gpio_config[button].hold_event, false, false);
    }
  }
}

/**
 * Callback function for button timer
 *
 * Iterates all push buttons to check if their state should be changed to HOLD.
 *
 */
static void ButtonTimerCallback(__attribute__((unused)) SSwTimer *p_dont_care)
{
  enter_critical_section();
  m_current_button_time += DEBOUNCE_FILTER_TIME;
  bool stop_button_timer = true;

  for (uint8_t button = 0; button < m_gpio_size; button++)
  {
    ButtonCheckHold(button);
  }

  for (uint8_t button = 0; button < m_gpio_size; button++)
  {
    if (GPIO_UP != m_gpio_info[button].gpio_state)
    {
      stop_button_timer = false;
      break;
    }
  }

  // Stop timer if all buttons are up or has exceeded max hold time
  if ((m_current_button_time > MAX_HOLD_TIME) || stop_button_timer)
  {
    TimerStop(&m_button_timer);
    button_timer_is_running = false;
  }
  else
  {
    TimerStart(&m_button_timer, DEBOUNCE_FILTER_TIME);
  }
  leave_critical_section();
}

void apps_hw_init(const gpio_config_t *p_gpio_config,
                  gpio_info_t *p_gpio_info,
                  uint8_t gpio_size)
{
  m_gpio_config = p_gpio_config;
  m_gpio_info = p_gpio_info;
  m_gpio_size = gpio_size;

  tr_hal_gpio_set_interrupt_priority(TR_HAL_INTERRUPT_PRIORITY_5);
  tr_hal_gpio_set_debounce_time(TR_HAL_DEBOUNCE_TIME_128_CLOCKS);
  gpio_power_lock = zpal_pm_register(ZPAL_PM_TYPE_USE_RADIO);
  enter_critical_section();
  for (uint8_t i = 0; i < gpio_size; i++)
  {
    tr_hal_gpio_settings_t   gpio_setting = {
      .direction = TR_HAL_GPIO_DIRECTION_INPUT,
      .interrupt_trigger = TR_HAL_GPIO_TRIGGER_EITHER_EDGE,
      .event_handler_fx = gpio_event_handler,
      .pull_mode = TR_HAL_PULLOPT_PULL_NONE,
      .output_level = TR_HAL_GPIO_LEVEL_HIGH,
      .enable_debounce = true
    };

    tr_hal_gpio_pin_t _pin = {.pin = m_gpio_config[i].gpio_no};

     tr_hal_gpio_set_mode(_pin, TR_HAL_GPIO_MODE_GPIO);
    /*gpio_cfg_input must always be called before gpio_register_isr otherwise the isr callback pointer will be erased.*/
    if (m_gpio_config[i].low_power)
    {
      tr_hal_gpio_set_wake_mode(_pin, m_gpio_config[i].on_value? TR_HAL_WAKE_MODE_INPUT_HIGH:TR_HAL_WAKE_MODE_INPUT_LOW);
      Lpm_Enable_Low_Power_Wakeup(m_gpio_config[i].low_power);
    }

    if (m_gpio_config[i].on_value)
    {
      gpio_setting.pull_mode = TR_HAL_PULLOPT_PULL_DOWN_100K;
      gpio_setting.wake_mode = TR_HAL_WAKE_MODE_INPUT_HIGH;
    }
    else
    {
      gpio_setting.pull_mode = TR_HAL_PULLOPT_PULL_UP_100K;
      gpio_setting.wake_mode = TR_HAL_WAKE_MODE_INPUT_LOW;
    }

    tr_hal_gpio_init(_pin, &gpio_setting);
  }
  AppTimerRegister(&m_button_timer, false, ButtonTimerCallback);
  uint32_t gpio_status = zpal_gpio_status_get();

  /* if reset reason is wakeup from deep sleep by external gpio then handle the trigger gpio, else skip handling gpio*/
  uint8_t gpio_num = (ZPAL_RESET_REASON_DEEP_SLEEP_EXT_INT ==  GetResetReason())? 0: 0xFF;
   /*
   * We were awakened from deep sleep by a GPIO event. The initialization process takes too long, preventing us from detecting the GPIO down event accurately.
   * Instead, we rely on the stored value of the GPIO interrupt status to identify which GPIO triggered the wake-up.
   *
   * If the GPIO is in an "up" state, this indicates that the GPIO went down and then back up again, prompting us to trigger a short press.
   * Conversely, if the GPIO remains in a "down" state, we simply update the push button (PB) state
  */

  for  (;gpio_num < gpio_size; gpio_num++)
  {
    if (gpio_status & (1 << m_gpio_config[gpio_num].gpio_no))
    {
      if (GPIO_UP == GetGpioState(gpio_num))
      {
        // Button toggle
        //The went down and then back up again before the gpio driver was ready, we trigger short press event
        PushButtonEventHandler(m_gpio_config[gpio_num].short_event, true, false);
      }
      else
      {
        // The button is still down
        zpal_pm_stay_awake(gpio_power_lock, 0);
        PushbuttonUpdateState(gpio_num, GPIO_DOWN, false);
      }
      break;
    }
  }
  leave_critical_section();
}
