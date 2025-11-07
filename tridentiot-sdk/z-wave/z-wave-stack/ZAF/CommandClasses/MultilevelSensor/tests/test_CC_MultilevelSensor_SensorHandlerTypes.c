/***************************************************************************//**
 * @file test_CC_MultilevelSensor_SensorHandlerTypes.c
 * @brief test_CC_MultilevelSensor_SensorHandlerTypes.c
 * @copyright 2020 Silicon Laboratories Inc.
 *******************************************************************************
 * # License
 * <b>Copyright 2020 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 ******************************************************************************/
// -----------------------------------------------------------------------------
//                   Includes
// -----------------------------------------------------------------------------
#include <string.h>
#include <stdbool.h>
#include <mock_control.h>
#include "CC_MultilevelSensor_SensorHandler.h"
#include "CC_MultilevelSensor_SensorHandlerTypes.h"
// -----------------------------------------------------------------------------
//                Macros and Typedefs
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
//              Static Function Declarations
// -----------------------------------------------------------------------------
static bool init_sensor_interface(void);
static bool deinit_sensor_interface(void);
static bool sensor_interface_read(sensor_read_result_t* o_result, uint8_t i_scale);

// -----------------------------------------------------------------------------
//                Global Variables
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
//                Static Variables
// -----------------------------------------------------------------------------

static sensor_interface_t test_sensor_interface_A;
// -----------------------------------------------------------------------------
//              Public Function Definitions
// -----------------------------------------------------------------------------

void setUpSuite(void)
{
  cc_multilevel_sensor_init_interface(&test_sensor_interface_A, SENSOR_NAME_AIR_TEMPERATURE);
  cc_multilevel_sensor_add_supported_scale_interface(&test_sensor_interface_A, SENSOR_SCALE_CELSIUS);
  test_sensor_interface_A.init     = init_sensor_interface;
  test_sensor_interface_A.deinit     = deinit_sensor_interface;
  test_sensor_interface_A.read_value   = sensor_interface_read;

  cc_multilevel_sensor_registration(&test_sensor_interface_A);
}

void tearDownSuite(void)
{

}

void test_cc_multilevel_sensor_init_interface_success(void)
{
  sensor_interface_return_value_t return_value;
  sensor_interface_t sensor_interface;
  sensor_name_t    sensor_name = SENSOR_NAME_AIR_TEMPERATURE;

  return_value = cc_multilevel_sensor_init_interface(&sensor_interface, sensor_name);
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(SENSOR_INTERFACE_RETURN_VALUE_OK, return_value,
    "[Sensor interface handler types] Sensor interface init failed");
}

void test_cc_multilevel_sensor_init_interface_NULL_interface(void)
{
  sensor_interface_return_value_t return_value;
  sensor_name_t    sensor_name = SENSOR_NAME_AIR_TEMPERATURE;

  return_value = cc_multilevel_sensor_init_interface(NULL, sensor_name);
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(SENSOR_INTERFACE_RETURN_VALUE_ERROR, return_value,
    "[Sensor interface handler types] Sensor interface init with wrong parameter failed ");
}

void test_cc_multilevel_sensor_init_interface_invalid_sensor_name(void)
{
  sensor_interface_return_value_t return_value;
  sensor_interface_t sensor_interface;
  sensor_name_t    sensor_name = SENSOR_NAME_MAX_COUNT;

  return_value = cc_multilevel_sensor_init_interface(&sensor_interface, sensor_name);
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(SENSOR_INTERFACE_RETURN_VALUE_ERROR, return_value,
    "[Sensor interface handler types] Sensor interface init failed");
}

void test_cc_multilevel_sensor_get_sensor_type_success(void)
{
  const sensor_type_t* sensor_type;

  sensor_type = cc_multilevel_sensor_get_sensor_type(SENSOR_NAME_AIR_TEMPERATURE);
  TEST_ASSERT_NOT_NULL_MESSAGE(sensor_type,
    "[Sensor interface handler types] Sensor type not found");
}

void test_cc_multilevel_sensor_get_sensor_type_wrong_parameter(void)
{
  const sensor_type_t* sensor_type;

  sensor_type = cc_multilevel_sensor_get_sensor_type(SENSOR_NAME_MAX_COUNT);
  TEST_ASSERT_NULL_MESSAGE(sensor_type,
    "[Sensor interface handler types] Call with wrong parameter failed");
}

void test_cc_multilevel_sensor_add_supported_scale_success(void)
{
  sensor_interface_return_value_t return_value;
  uint8_t scale_to_add = SENSOR_SCALE_FAHRENHEIT;

  return_value = cc_multilevel_sensor_add_supported_scale_interface(&test_sensor_interface_A, scale_to_add);
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(SENSOR_INTERFACE_RETURN_VALUE_OK, return_value,
    "[Sensor interface handler types] Sensor interface add scale failed");
}

void test_cc_multilevel_sensor_add_supported_scale_invalid_scale(void)
{
  sensor_interface_return_value_t return_value;
  uint8_t invalid_scale_to_add = 0xFF;

  return_value = cc_multilevel_sensor_add_supported_scale_interface(&test_sensor_interface_A, invalid_scale_to_add);
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(SENSOR_INTERFACE_RETURN_VALUE_INVALID_SCALE_VALUE, return_value,
    "[Sensor interface handler types] Sensor interface add invalid scale failed");
}

void test_cc_multilevel_sensor_add_supported_scale_already_set_scale(void)
{
  sensor_interface_return_value_t return_value;
  uint8_t scale_to_add = SENSOR_SCALE_CELSIUS;

  return_value = cc_multilevel_sensor_add_supported_scale_interface(&test_sensor_interface_A, scale_to_add);
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(SENSOR_INTERFACE_RETURN_VALUE_ALREADY_SET, return_value,
    "[Sensor interface handler types] Sensor interface add invalid scale failed");
}

void test_cc_multilevel_sensor_add_supported_scale_wrong_parameter(void)
{
  sensor_interface_return_value_t return_value;
  uint8_t scale_to_add = SENSOR_SCALE_FAHRENHEIT;

  return_value = cc_multilevel_sensor_add_supported_scale_interface(NULL, scale_to_add);
  TEST_ASSERT_NOT_EQUAL_MESSAGE(SENSOR_INTERFACE_RETURN_VALUE_OK, return_value,
    "[Sensor interface handler types] Sensor interface add scale failed");

}

// -----------------------------------------------------------------------------
//              Static Function Definitions
// -----------------------------------------------------------------------------

static bool init_sensor_interface(void)
{
  return true;
}

static bool deinit_sensor_interface(void)
{
  return true;
}

static bool sensor_interface_read(sensor_read_result_t* o_result, uint8_t i_scale)
{
  return true;
}