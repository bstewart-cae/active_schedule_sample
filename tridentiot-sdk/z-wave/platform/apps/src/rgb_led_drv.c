/// ***************************************************************************
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2025 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************

#include <stddef.h>
#include <stdint.h>
#include <DebugPrint.h>
#include "tr_hal_gpio.h"
#include <pwm.h>
#include <sysctrl.h>
#include "Assert.h" // NOSONAR
#include <rgb_led_drv.h>


#define PWM_MAX_VALUE    1000
#define COLOR_MAX_VALUE  0xFF

static pwm_seq_para_head_t m_led_config = {
  .pwm_seq0 = {
    .pwm_rdma_addr = 0,
    .pwm_element_num = 1,
    .pwm_repeat_num = 0,
    .pwm_delay_num = 0
  },
  .pwm_seq1  = {
    .pwm_rdma_addr = 0,
    .pwm_element_num = 0,
    .pwm_repeat_num = 0,
    .pwm_delay_num = 0
  },
  .pwm_play_cnt = 0,
  .pwm_count_end_val = 0, // Ignored when using format 1.
  .pwm_seq_order = PWM_SEQ_ORDER_R,
  .pwm_triggered_src = PWM_TRIGGER_SRC_SELF,
  .pwm_seq_num = 0,
  .pwm_id = PWM_ID_0,
  .pwm_clk_div = PWM_CLK_DIV_32,
  .pwm_counter_mode = PWM_COUNTER_MODE_UP,
  .pwm_dma_smp_fmt = PWM_DMA_SMP_FMT_1,
  .pwm_seq_mode = PWM_SEQ_MODE_CONTINUOUS
};

void rgb_led_init(rgb_led_config_t * pRgb_led_config)
{
  for (pwm_id_t pwm_id = PWM_ID_0; pwm_id < pRgb_led_config->pwm_ch_count; pwm_id++)
  {
    m_led_config.pwm_id = pwm_id;
    tr_hal_gpio_pin_t led_gpio = {.pin = pRgb_led_config->pPwm_gpio_arry[pwm_id]};

    pRgb_led_config->pPwm_duty_arry[pwm_id] = PWM_FILL_SAMPLE_DATA_MODE1(1, 0, PWM_MAX_VALUE);
    m_led_config.pwm_seq0.pwm_rdma_addr = (uint32_t)&pRgb_led_config->pPwm_duty_arry[pwm_id];
    uint32_t result = Pwm_Init(&m_led_config);
    ASSERT(STATUS_SUCCESS == result);

    tr_hal_gpio_set_output(led_gpio, TR_HAL_GPIO_LEVEL_HIGH);
    tr_hal_gpio_set_mode(led_gpio, pRgb_led_config->pPwm_gpio_mode_arry[pwm_id]);
    tr_hal_gpio_set_pull_mode(led_gpio, TR_HAL_PULLOPT_PULL_NONE);
    tr_hal_gpio_set_direction(led_gpio, TR_HAL_GPIO_DIRECTION_OUTPUT);

    Pwm_Start(&m_led_config);
  }
}

void update_rgb_led(rgb_led_config_t *pRgb_led_config, uint8_t pwm_id, uint8_t pwm_value)
{
    m_led_config.pwm_id = pwm_id;
    Pwm_Stop(&m_led_config);
    tr_hal_gpio_pin_t led_gpio = {.pin = pRgb_led_config->pPwm_gpio_arry[pwm_id]};
    if (!pwm_value)
    {
      tr_hal_gpio_set_output(led_gpio, TR_HAL_GPIO_LEVEL_HIGH);
      tr_hal_gpio_set_mode(led_gpio, TR_HAL_GPIO_MODE_GPIO);
      tr_hal_gpio_set_direction(led_gpio,  TR_HAL_GPIO_DIRECTION_OUTPUT);// Set as output.
    }
    else if (0xFF == pwm_value)
    {
      tr_hal_gpio_set_output(led_gpio, TR_HAL_GPIO_LEVEL_LOW);
      tr_hal_gpio_set_mode(led_gpio, TR_HAL_GPIO_MODE_GPIO);
      tr_hal_gpio_set_direction(led_gpio,  TR_HAL_GPIO_DIRECTION_OUTPUT);// Set as output.
    }
    else
    {

      tr_hal_gpio_set_output(led_gpio, TR_HAL_GPIO_LEVEL_HIGH);
      tr_hal_gpio_set_mode(led_gpio, pRgb_led_config->pPwm_gpio_mode_arry[pwm_id]);
      tr_hal_gpio_set_pull_mode(led_gpio, TR_HAL_PULLOPT_PULL_NONE);
      tr_hal_gpio_set_direction(led_gpio,  TR_HAL_GPIO_DIRECTION_OUTPUT);// Set as output.

      uint32_t temp_color_value = ((uint32_t)pwm_value * PWM_MAX_VALUE) / COLOR_MAX_VALUE;
      pRgb_led_config->pPwm_duty_arry[pwm_id] = PWM_FILL_SAMPLE_DATA_MODE1(1, (uint16_t)(temp_color_value), PWM_MAX_VALUE);
      Pwm_Start(&m_led_config);
    }
}
