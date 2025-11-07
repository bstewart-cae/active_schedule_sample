/// ***************************************************************************
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2024 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************

/**
 * @file MultilevelSensor_common_hw.c
 */

#include <string.h>
#include <DebugPrint.h>
#include "SizeOf.h"
#include "adc_drv.h"
#include <CC_Battery.h>
#include <CC_MultilevelSensor_SensorHandlerTypes.h>

#define MY_BATTERY_SPEC_LEVEL_FULL         3000  // My battery's 100% level (millivolts)
#define MY_BATTERY_SPEC_LEVEL_EMPTY        2400  // My battery's 0% level (millivolts)

uint8_t
CC_Battery_BatteryGet_handler(uint8_t endpoint)
{
  uint32_t VBattery;

  uint8_t  accurateLevel;
  uint8_t  roundedLevel;
  uint8_t reporting_decrements;

  (void)endpoint;
   adc_init();
  /*
   * Simple example how to use the ADC to measure the battery voltage
   * and convert to a percentage battery level on a linear scale.
   */

  adc_get_voltage(&VBattery);
  /* Turn off the ADC when the conversion is finished to save power. */
  adc_enable(false);
  DPRINTF("\r\nBattery voltage: %dmV", VBattery);
  if (MY_BATTERY_SPEC_LEVEL_FULL <= VBattery)
  {
    // Level is full
    return (uint8_t)CMD_CLASS_BATTERY_LEVEL_FULL;
  }
  else if (MY_BATTERY_SPEC_LEVEL_EMPTY > VBattery)
  {
    // Level is empty (<0%)
    return (uint8_t)CMD_CLASS_BATTERY_LEVEL_WARNING;
  }
  else
  {
    reporting_decrements = cc_battery_config_get_reporting_decrements();
    // Calculate the percentage level from 0 to 100
    accurateLevel = (uint8_t)((100 * (VBattery - MY_BATTERY_SPEC_LEVEL_EMPTY)) / (MY_BATTERY_SPEC_LEVEL_FULL - MY_BATTERY_SPEC_LEVEL_EMPTY));

    // And round off to the nearest "reporting_decrements" level
    roundedLevel = (accurateLevel / reporting_decrements) * reporting_decrements; // Rounded down
    if ((accurateLevel % reporting_decrements) >= (reporting_decrements / 2))
    {
      roundedLevel += reporting_decrements; // Round up
    }
  }
  return roundedLevel;
}

void app_hw_deep_sleep_wakeup_handler(void)
{
  /* Nothing here, but offers the option to perform something after wake up
     from deep sleep. */
}

bool cc_multilevel_sensor_air_temperature_interface_read_value(sensor_read_result_t* o_result, uint8_t i_scale)
{
  static int32_t  temperature_celsius = 3220;
     adc_init();
  /*
   * Simple example how to use the ADC to measure the temperature
   */

  adc_get_temp(&temperature_celsius);
  /* Turn off the ADC when the conversion is finished to save power. */
  adc_enable(false);


  if(o_result != NULL)
  {
    memset(o_result, 0, sizeof(sensor_read_result_t));
    o_result->precision  = SENSOR_READ_RESULT_PRECISION_3;
    o_result->size_bytes = SENSOR_READ_RESULT_SIZE_4;

    if(i_scale == SENSOR_SCALE_FAHRENHEIT)
    {
      float temperature_celsius_divided = (float)temperature_celsius/(float)1000;
      int32_t temperature_fahrenheit = (int32_t)((temperature_celsius_divided * ((float)9/(float)5) + (float)32)*1000);

      o_result->raw_result[3] = (uint8_t)(temperature_fahrenheit&0xFF);
      o_result->raw_result[2] = (uint8_t)((temperature_fahrenheit>>8 )&0xFF);
      o_result->raw_result[1] = (uint8_t)((temperature_fahrenheit>>16)&0xFF);
      o_result->raw_result[0] = (uint8_t)((temperature_fahrenheit>>24)&0xFF);
    }
    else
    {
      o_result->raw_result[3] = (uint8_t)(temperature_celsius&0xFF);
      o_result->raw_result[2] = (uint8_t)((temperature_celsius>>8 )&0xFF);
      o_result->raw_result[1] = (uint8_t)((temperature_celsius>>16)&0xFF);
      o_result->raw_result[0] = (uint8_t)((temperature_celsius>>24)&0xFF);
    }
  }

  return true;
}

bool cc_multilevel_sensor_humidity_interface_read_value(sensor_read_result_t* o_result, __attribute__((unused)) uint8_t i_scale)
{

  // There is no humidity peripheral so we will return a static value.
  // The precision is set to 2, so "humidity" value would be read as 30.00%
  // This example only implements "scale = 0" used for percentage measurements.
  static uint32_t humidity = 3000;

  if(o_result != NULL)
  {
    memset(o_result, 0, sizeof(sensor_read_result_t));
    o_result->precision  = SENSOR_READ_RESULT_PRECISION_2;
    o_result->size_bytes = SENSOR_READ_RESULT_SIZE_4;

    o_result->raw_result[3] = (uint8_t)(humidity&0xFF);
    o_result->raw_result[2] = (uint8_t)((humidity>>8 )&0xFF);
    o_result->raw_result[1] = (uint8_t)((humidity>>16)&0xFF);
    o_result->raw_result[0] = (uint8_t)((humidity>>24)&0xFF);
  }

  return true;
}

bool cc_multilevel_sensor_ambient_light_interface_read_value(sensor_read_result_t* o_result,  __attribute__((unused)) uint8_t i_scale)
{
  // There is no ambient light peripheral so we will return a static value.
  // The precision is set to 2, so "ambient_light" would be read as 50.00 [lx]
  // which equals to 50.00 [lm/m^2] (lumens per square meter).

  // This example only implements "scale = 1" used for percentage measurements.
  static uint32_t ambient_light = 5000;

  if(o_result != NULL)
  {
    memset(o_result, 0, sizeof(sensor_read_result_t));
    o_result->precision  = SENSOR_READ_RESULT_PRECISION_2;
    o_result->size_bytes = SENSOR_READ_RESULT_SIZE_4;

    o_result->raw_result[3] = (uint8_t)(ambient_light&0xFF);
    o_result->raw_result[2] = (uint8_t)((ambient_light>>8 )&0xFF);
    o_result->raw_result[1] = (uint8_t)((ambient_light>>16)&0xFF);
    o_result->raw_result[0] = (uint8_t)((ambient_light>>24)&0xFF);
  }

  return true;
}
