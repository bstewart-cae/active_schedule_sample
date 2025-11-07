/// ***************************************************************************
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2024 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************

#include <stdio.h>
#include <string.h>
#include "tr_platform.h"
#include <sadc.h>
#include <FreeRTOS.h>
#include <semphr.h>
#include <task.h>
#include <zpal_power_manager.h>

static bool m_adc_init = false;
static volatile sadc_convert_state_t sadc_convert_status = SADC_CONVERT_IDLE;
static volatile sadc_value_t         sadc_convert_value;

static SemaphoreHandle_t adc_mutex = NULL;
static StaticSemaphore_t adc_mutex_buffer;
static zpal_pm_handle_t  adc_power_lock;

static void Sadc_Int_Callback_Handler(sadc_cb_t *p_cb)
{
  if (p_cb->type == SADC_CB_SAMPLE)
  {
    if (p_cb->data.sample.channel == SADC_CH_VBAT)
    {
      sadc_convert_value = p_cb->data.sample.value;
      sadc_convert_status = SADC_CONVERT_DONE;
    }
    else if (p_cb->data.sample.channel == SADC_CH_TEMPERATURE)
    {
      sadc_convert_value = p_cb->data.sample.value;
      sadc_convert_status = SADC_CONVERT_DONE;
    }
  }
}

void adc_enable(uint8_t state)
{
  if (state)
  {
    /* register callback, now aio/vbat/temp use same callback */
    Sadc_Config_Enable(SADC_RES_12BIT, SADC_OVERSAMPLE_0, Sadc_Int_Callback_Handler);
  }
  else
  {
    Sadc_Disable();
  }
}

void adc_init(void)
{
  if (!m_adc_init)
  {
    m_adc_init = true;
    adc_power_lock = zpal_pm_register(ZPAL_PM_TYPE_USE_RADIO);
    adc_mutex = xSemaphoreCreateMutexStatic( &adc_mutex_buffer );
    /* The adc_mutex_buffer was not NULL, so it is expected that the handle will not be NULL. */
    configASSERT( adc_mutex );
    NVIC_SetPriority(Sadc_IRQn, 4);
  }
  adc_enable(true);
}

static int32_t get_adc_reading (sadc_input_ch_t ch)
{
  sadc_convert_status = SADC_CONVERT_START;
  zpal_pm_stay_awake(adc_power_lock,  0);
  if (SADC_CH_VBAT == ch)
  {
    Sadc_Vbat_Read();
  }
  else if (SADC_CH_TEMPERATURE == ch)
  {
    Sadc_Temp_Read();
  }
  while (SADC_CONVERT_DONE != sadc_convert_status)
  {
    vTaskDelay(1);
  }
  zpal_pm_cancel(adc_power_lock);
  return sadc_convert_value;
}

void adc_get_voltage(uint32_t *pVoltage)
{
  if (NULL ==pVoltage)
  {
    return;
  }
  xSemaphoreTake(adc_mutex, portMAX_DELAY);
  // Measure AVDD (battery supply voltage level)
  *pVoltage = (uint32_t)get_adc_reading(SADC_CH_VBAT);
  xSemaphoreGive(adc_mutex);
}

/*This is not working yet*/
void adc_get_temp(int32_t *pTemp)
{
  if (NULL == pTemp)
  {
    return;
  }
  xSemaphoreTake(adc_mutex, portMAX_DELAY);
  // Measure the tempretuare
  *pTemp = get_adc_reading(SADC_CH_TEMPERATURE);
  xSemaphoreGive(adc_mutex);
}
