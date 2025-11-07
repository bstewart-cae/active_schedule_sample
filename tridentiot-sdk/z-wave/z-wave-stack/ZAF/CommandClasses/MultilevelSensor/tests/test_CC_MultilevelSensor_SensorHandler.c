/***************************************************************************//**
 * @file test_CC_MultilevelSensor_SensorHandler.c
 * @brief test_CC_MultilevelSensor_SensorHandler.c
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
static void multilevel_sensor_register_interface_for_test(void);
// -----------------------------------------------------------------------------
//                Global Variables
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
//                Static Variables
// -----------------------------------------------------------------------------
static sensor_interface_t test_sensor_interface_A;
static sensor_interface_t test_sensor_interface_B;
static sensor_interface_t test_sensor_interface_C;
static sensor_interface_t test_sensor_interface_D; // SENSOR_NAME_ACCELERATION_X
static sensor_interface_t test_sensor_interface_E; // SENSOR_NAME_ACCELERATION_Y
static sensor_interface_t test_sensor_interface_F; // SENSOR_NAME_ACCELERATION_Z
// -----------------------------------------------------------------------------
//              Public Function Definitions
// -----------------------------------------------------------------------------

void setUpSuite(void)
{
}

void tearDownSuite(void)
{

}

void test_cc_multilevel_sensor_registration_success(void)
{
  uint8_t number_of_registered_sensors = 0;

  multilevel_sensor_register_interface_for_test();
  number_of_registered_sensors = cc_multilevel_sensor_get_number_of_registered_sensors();
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(6, number_of_registered_sensors,
    "[Sensor interface registration] Interface registration failed, second interface could not be registered");
}


void test_cc_multilevel_sensor_registration_already_registered(void)
{
  cc_multilevel_sensor_return_value return_value;

  cc_multilevel_sensor_reset_administration(); 

  cc_multilevel_sensor_init_interface(&test_sensor_interface_A, SENSOR_NAME_AIR_TEMPERATURE);
  cc_multilevel_sensor_add_supported_scale_interface(&test_sensor_interface_A, SENSOR_SCALE_CELSIUS);
  cc_multilevel_sensor_add_supported_scale_interface(&test_sensor_interface_A, SENSOR_SCALE_FAHRENHEIT);
  test_sensor_interface_A.init         = init_sensor_interface;
  test_sensor_interface_A.deinit       = deinit_sensor_interface;
  test_sensor_interface_A.read_value   = sensor_interface_read;

  cc_multilevel_sensor_registration(&test_sensor_interface_A);
  return_value = cc_multilevel_sensor_registration(&test_sensor_interface_A);

  TEST_ASSERT_EQUAL_UINT8(CC_MULTILEVEL_SENSOR_RETURN_VALUE_ALREADY_REGISTRATED, return_value);
}

void test_cc_multilevel_sensor_registration_NULL_pointer(void)
{
  cc_multilevel_sensor_return_value return_value;

  multilevel_sensor_register_interface_for_test();
  return_value = cc_multilevel_sensor_registration(NULL);
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(CC_MULTILEVEL_SENSOR_RETURN_VALUE_ERROR, return_value,
    "[Sensor interface registration] Interface registration NULL pointer test failed");
}

void test_cc_multilevel_sensor_registration_registration_limit_reached(void)
{
  cc_multilevel_sensor_return_value return_value;

  multilevel_sensor_register_interface_for_test();
  return_value = cc_multilevel_sensor_registration(&test_sensor_interface_A);
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(CC_MULTILEVEL_SENSOR_RETURN_VALUE_REGISTRATION_LIMIT_REACHED, return_value,
    "[Sensor interface registration] Interface registration registration limit reached test failed");
}

void test_cc_multilevel_sensor_check_type_registered_success(void)
{
  cc_multilevel_sensor_return_value is_sensor_type_registered;

  multilevel_sensor_register_interface_for_test();
  is_sensor_type_registered = cc_multilevel_sensor_check_sensor_type_registered(test_sensor_interface_A.sensor_type->value);
  TEST_ASSERT_EQUAL_UINT8_MESSAGE((uint8_t)CC_MULTILEVEL_SENSOR_RETURN_VALUE_OK, is_sensor_type_registered,
    "[Sensor type registration check] Sensor type is not registered");
}

void test_cc_multilevel_sensor_check_scale_success(void)
{
  uint8_t default_scale = 0;
  
  multilevel_sensor_register_interface_for_test();
  default_scale = cc_multilevel_sensor_check_scale(&test_sensor_interface_A, SENSOR_SCALE_CELSIUS);
  TEST_ASSERT_EQUAL_UINT8_MESSAGE((uint8_t)SENSOR_SCALE_CELSIUS, default_scale,
    "[Sensor check scale] Valid scale check failed");
}

void test_cc_multilevel_sensor_check_scale_invalid_scale(void)
{
  uint8_t default_scale = 0;
  uint8_t not_existing_dummy_scale = 0xFF;

  multilevel_sensor_register_interface_for_test();
  default_scale = cc_multilevel_sensor_check_scale(&test_sensor_interface_A, not_existing_dummy_scale);
  TEST_ASSERT_EQUAL_UINT8_MESSAGE((uint8_t)SENSOR_SCALE_CELSIUS, default_scale,
    "[Sensor check scale] Invalid dummy scale test failed");
}

void test_cc_multilevel_sensor_check_scale_NULL_pointer(void)
{
  uint8_t default_scale = 0;

  multilevel_sensor_register_interface_for_test();
  default_scale = cc_multilevel_sensor_check_scale(NULL, SENSOR_SCALE_CELSIUS);
  TEST_ASSERT_EQUAL_UINT8_MESSAGE((uint8_t)SENSOR_SCALE_CELSIUS, default_scale,
    "[Sensor check scale] NULL pointer input test failed");
}

void test_cc_multilevel_sensor_get_interface_success(void)
{
  cc_multilevel_sensor_return_value getter_result;
  sensor_interface_t* test_get_sensor_interface;

  multilevel_sensor_register_interface_for_test();
  getter_result = cc_multilevel_sensor_get_interface(test_sensor_interface_A.sensor_type->value , &test_get_sensor_interface);
  TEST_ASSERT_EQUAL_UINT8_MESSAGE((uint8_t)CC_MULTILEVEL_SENSOR_RETURN_VALUE_OK, getter_result,
    "[Sensor get interface] Get interface failed");
}

void test_cc_multilevel_sensor_get_interface_NULL_pointer(void)
{
  cc_multilevel_sensor_return_value getter_result;

  multilevel_sensor_register_interface_for_test();
  getter_result = cc_multilevel_sensor_get_interface(test_sensor_interface_A.sensor_type->value , NULL);
  TEST_ASSERT_EQUAL_UINT8_MESSAGE((uint8_t)CC_MULTILEVEL_SENSOR_RETURN_VALUE_ERROR, getter_result,
    "[Sensor get interface] Get interface NULL pointer test failed");
}

void test_cc_multilevel_sensor_get_interface_invalid_sensor_value_type(void)
{
  cc_multilevel_sensor_return_value getter_result;
  sensor_interface_t* test_get_sensor_interface;
  uint8_t invalid_sensor_value_type = 0xFF;

  multilevel_sensor_register_interface_for_test();
  getter_result = cc_multilevel_sensor_get_interface(invalid_sensor_value_type , &test_get_sensor_interface);
  TEST_ASSERT_EQUAL_UINT8_MESSAGE((uint8_t)CC_MULTILEVEL_SENSOR_RETURN_VALUE_NOT_FOUND, getter_result,
    "[Sensor get interface] Get interface invalid sensor type value test failed");
}

void test_cc_multilevel_sensor_get_supported_sensors_success(void)
{
  uint8_t supported_sensors[11];
  uint8_t supported_sensors_expected[11];
  uint8_t result      = 0;
  uint8_t result_expected = 0;

  memset(supported_sensors, 0, sizeof(supported_sensors));
  memset(supported_sensors_expected, 0, sizeof(supported_sensors_expected));

  // Values from CC_MulitlevelSensor_SensorHandlerTypes.c
  const uint8_t byte_offset_temperature = 0x01;
  const uint8_t byte_offset_humidity    = 0x01;
  const uint8_t byte_offset_illuminance = 0x01;
  const uint8_t byte_offset_IMU         = 0x07;

  supported_sensors_expected[byte_offset_temperature - 1] |= 1<<0; //SENSOR_NAME_AIR_TEMPERATURE
  supported_sensors_expected[byte_offset_humidity    - 1] |= 1<<4; //SENSOR_NAME_HUMIDITY
  supported_sensors_expected[byte_offset_illuminance - 1] |= 1<<2; //SENSOR_NAME_ILLUMINANCE
  supported_sensors_expected[byte_offset_IMU - 1]         |= 1<<3; //SENSOR_NAME_ACCELERATION_X
  supported_sensors_expected[byte_offset_IMU - 1]         |= 1<<4; //SENSOR_NAME_ACCELERATION_Y
  supported_sensors_expected[byte_offset_IMU - 1]         |= 1<<5; //SENSOR_NAME_ACCELERATION_Z

  multilevel_sensor_register_interface_for_test();
  cc_multilevel_sensor_get_supported_sensors(supported_sensors);

  result = memcmp(supported_sensors, supported_sensors_expected, sizeof(supported_sensors));

  TEST_ASSERT_EQUAL_UINT8_MESSAGE(result_expected, result,
    "[Sensor get supported sensors] Get supported sensors failed");
}

void test_cc_multilevel_sensor_get_supported_sensors_NULL_pointer(void)
{
  sensor_interface_return_value_t return_value;

  multilevel_sensor_register_interface_for_test();
  return_value = cc_multilevel_sensor_get_supported_sensors(NULL);

  TEST_ASSERT_EQUAL_UINT8_MESSAGE(CC_MULTILEVEL_SENSOR_RETURN_VALUE_ERROR, return_value,
    "[Sensor get supported sensors] Get supported sensors failed");
}

void test_cc_multilevel_sensor_get_default_sensor_type_success(void)
{
  sensor_interface_return_value_t return_value;
  uint8_t default_sensor_type = 0;
  uint8_t expected_default_sensor_type = 0x01; //SENSOR_NAME_AIR_TEMPERATURE

  multilevel_sensor_register_interface_for_test();
  return_value = cc_multilevel_sensor_get_default_sensor_type(&default_sensor_type);
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(CC_MULTILEVEL_SENSOR_RETURN_VALUE_OK, return_value,
    "[Sensor get default sensor type] Sensor get default sensor type failed");
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(expected_default_sensor_type, default_sensor_type,
    "[Sensor get default sensor type] Sensor get default sensor type failed");
}

void test_cc_multilevel_sensor_get_default_sensor_type_no_sensor_registered(void)
{
  sensor_interface_return_value_t return_value;
  uint8_t default_sensor_type = 0;

  cc_multilevel_sensor_reset_administration();
  return_value = cc_multilevel_sensor_get_default_sensor_type(&default_sensor_type);
  TEST_ASSERT_EQUAL_UINT8(CC_MULTILEVEL_SENSOR_RETURN_VALUE_ERROR, return_value);
}

void test_cc_multilevel_sensor_get_default_sensor_type_NULL_pointer(void)
{
  sensor_interface_return_value_t return_value;

  multilevel_sensor_register_interface_for_test();
  return_value = cc_multilevel_sensor_get_default_sensor_type(NULL);
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(CC_MULTILEVEL_SENSOR_RETURN_VALUE_ERROR, return_value,
    "[Sensor get default sensor type] Sensor get default sensor type failed");
}

void test_cc_multilevel_sensor_get_default_sensor_type_no_sensor_registered_NULL_pointer(void)
{
  sensor_interface_return_value_t return_value;

  cc_multilevel_sensor_reset_administration();
  return_value = cc_multilevel_sensor_get_default_sensor_type(NULL);
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(CC_MULTILEVEL_SENSOR_RETURN_VALUE_ERROR, return_value,
    "[Sensor get default sensor type] Sensor get default sensor type failed");
}

void test_cc_multilevel_sensor_get_supported_scale_success(void)
{
  uint8_t supported_scale   = 0;
  uint8_t air_temperature_sensor_type_value = 0x01; // SENSOR_NAME_AIR_TEMPERATURE
  sensor_interface_return_value_t return_value;

  multilevel_sensor_register_interface_for_test();
  return_value = cc_multilevel_sensor_get_supported_scale(air_temperature_sensor_type_value, &supported_scale);
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(CC_MULTILEVEL_SENSOR_RETURN_VALUE_OK, return_value,
    "[Sensor get supported scale] Failed");
}

void test_cc_multilevel_sensor_get_supported_scale_NULL_pointer(void)
{
  uint8_t air_temperature_sensor_type_value = 0x01; // SENSOR_NAME_AIR_TEMPERATURE
  sensor_interface_return_value_t return_value;

  multilevel_sensor_register_interface_for_test();
  return_value = cc_multilevel_sensor_get_supported_scale(air_temperature_sensor_type_value, NULL);
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(CC_MULTILEVEL_SENSOR_RETURN_VALUE_ERROR, return_value,
    "[Sensor get supported scale] Failed");
}

void test_cc_multilevel_sensor_get_supported_scale_invalid_sensor_type_value(void)
{
  uint8_t supported_scale   = 0;
  uint8_t invalid_sensor_type_value = 0xFF;
  sensor_interface_return_value_t return_value;

  multilevel_sensor_register_interface_for_test();
  return_value = cc_multilevel_sensor_get_supported_scale(invalid_sensor_type_value, &supported_scale);
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(CC_MULTILEVEL_SENSOR_RETURN_VALUE_NOT_FOUND, return_value,
    "[Sensor get supported scale] Failed");
}

void test_cc_multilevel_sensor_next_iterator_success(void)
{
  sensor_interface_iterator_t* sensor_interface_iterator;
  multilevel_sensor_register_interface_for_test();

  cc_multilevel_sensor_init_iterator(&sensor_interface_iterator);
  TEST_ASSERT_EQUAL_PTR(sensor_interface_iterator, &test_sensor_interface_A);

  cc_multilevel_sensor_next_iterator(&sensor_interface_iterator);
  TEST_ASSERT_EQUAL_PTR(sensor_interface_iterator, &test_sensor_interface_B);

  cc_multilevel_sensor_next_iterator(&sensor_interface_iterator);
  TEST_ASSERT_EQUAL_PTR(sensor_interface_iterator, &test_sensor_interface_C);

  cc_multilevel_sensor_next_iterator(&sensor_interface_iterator);
  TEST_ASSERT_EQUAL_PTR(sensor_interface_iterator, &test_sensor_interface_D);

    cc_multilevel_sensor_next_iterator(&sensor_interface_iterator);
  TEST_ASSERT_EQUAL_PTR(sensor_interface_iterator, &test_sensor_interface_E);

    cc_multilevel_sensor_next_iterator(&sensor_interface_iterator);
  TEST_ASSERT_EQUAL_PTR(sensor_interface_iterator, &test_sensor_interface_F);

  cc_multilevel_sensor_next_iterator(&sensor_interface_iterator);
  TEST_ASSERT_EQUAL_PTR(sensor_interface_iterator, NULL);
}

void test_cc_multilevel_sensor_init_all_sensor(void)
{
  cc_multilevel_sensor_init_all_sensor();
}

// -----------------------------------------------------------------------------
//              Static Function Definitions
// -----------------------------------------------------------------------------

static void multilevel_sensor_register_interface_for_test(void)
{
  cc_multilevel_sensor_reset_administration();

  cc_multilevel_sensor_init_interface(&test_sensor_interface_A, SENSOR_NAME_AIR_TEMPERATURE);
  cc_multilevel_sensor_add_supported_scale_interface(&test_sensor_interface_A, SENSOR_SCALE_CELSIUS);
  cc_multilevel_sensor_add_supported_scale_interface(&test_sensor_interface_A, SENSOR_SCALE_FAHRENHEIT);
  test_sensor_interface_A.init         = init_sensor_interface;
  test_sensor_interface_A.deinit       = deinit_sensor_interface;
  test_sensor_interface_A.read_value   = sensor_interface_read;

  cc_multilevel_sensor_registration(&test_sensor_interface_A);

  cc_multilevel_sensor_init_interface(&test_sensor_interface_B, SENSOR_NAME_HUMIDITY);
  cc_multilevel_sensor_add_supported_scale_interface(&test_sensor_interface_B, SENSOR_SCALE_ABSOLUTE_HUMIDITY);
  test_sensor_interface_B.init         = init_sensor_interface;
  test_sensor_interface_B.deinit       = deinit_sensor_interface;
  test_sensor_interface_B.read_value   = sensor_interface_read;

  cc_multilevel_sensor_registration(&test_sensor_interface_B);

  cc_multilevel_sensor_init_interface(&test_sensor_interface_C, SENSOR_NAME_ILLUMINANCE);
  cc_multilevel_sensor_add_supported_scale_interface(&test_sensor_interface_C, SENSOR_SCALE_LUX);
  test_sensor_interface_C.init         = init_sensor_interface;
  test_sensor_interface_C.deinit       = deinit_sensor_interface;
  test_sensor_interface_C.read_value   = sensor_interface_read;

  cc_multilevel_sensor_registration(&test_sensor_interface_C);

  cc_multilevel_sensor_init_interface(&test_sensor_interface_D, SENSOR_NAME_ACCELERATION_X);
  cc_multilevel_sensor_add_supported_scale_interface(&test_sensor_interface_D, SENSOR_SCALE_METER_SQUARE_SECOND);
  test_sensor_interface_D.init         = init_sensor_interface;
  test_sensor_interface_D.deinit       = deinit_sensor_interface;
  test_sensor_interface_D.read_value   = sensor_interface_read;

  cc_multilevel_sensor_registration(&test_sensor_interface_D);

  cc_multilevel_sensor_init_interface(&test_sensor_interface_E, SENSOR_NAME_ACCELERATION_Y);
  cc_multilevel_sensor_add_supported_scale_interface(&test_sensor_interface_E, SENSOR_SCALE_METER_SQUARE_SECOND);
  test_sensor_interface_E.init         = init_sensor_interface;
  test_sensor_interface_E.deinit       = deinit_sensor_interface;
  test_sensor_interface_E.read_value   = sensor_interface_read;

  cc_multilevel_sensor_registration(&test_sensor_interface_E);

    cc_multilevel_sensor_init_interface(&test_sensor_interface_F, SENSOR_NAME_ACCELERATION_Z);
  cc_multilevel_sensor_add_supported_scale_interface(&test_sensor_interface_F, SENSOR_SCALE_METER_SQUARE_SECOND);
  test_sensor_interface_F.init         = init_sensor_interface;
  test_sensor_interface_F.deinit       = deinit_sensor_interface;
  test_sensor_interface_F.read_value   = sensor_interface_read;

  cc_multilevel_sensor_registration(&test_sensor_interface_F);
}

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