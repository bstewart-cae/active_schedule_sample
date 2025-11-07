/***************************************************************************//**
 * @file test_CC_MultilevelSensor_Support.c
 * @brief test_CC_MultilevelSensor_Support.c
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
#include <ZAF_TSE.h>
#include "ZAF_types.h"
#include <test_common.h>
#include <mock_control.h>
#include <ZW_TransportEndpoint.h>
#include "CC_MultilevelSensor_Support.h"
#include "CC_MultilevelSensor_SensorHandler.h"
#include "CC_MultilevelSensor_SensorHandlerTypes.h"
#include "QueueNotifying.h"
#include "ZAF_CC_Invoker.h"

// -----------------------------------------------------------------------------
//                Macros and Typedefs
// -----------------------------------------------------------------------------
#define SLI_TEST_CC_MULTILEVELSENSOR_SUPPORT_EXPECTED_CELSIUS   0xA5
#define SLI_TEST_CC_MULTILEVELSENSOR_SUPPORT_EXPECTED_PRECISION SENSOR_READ_RESULT_PRECISION_1
#define SLI_TEST_CC_MULTILEVELSENSOR_SUPPORT_EXPECTED_SIZE      SENSOR_READ_RESULT_SIZE_1
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
// -----------------------------------------------------------------------------
//              Public Function Definitions
// -----------------------------------------------------------------------------

void setUpSuite(void)
{
}

void tearDownSuite(void)
{
}

void test_cc_multilevel_get_sensor_default_branch(void)
{
  received_frame_status_t handler_return_value;
  command_handler_input_t* p_chi = test_common_command_handler_input_allocate();
  uint8_t sensor_type_value_to_test = 0x01; /*SENSOR_NAME_AIR_TEMPERATURE*/
  uint8_t sensor_scale_to_test    = (SENSOR_SCALE_CELSIUS << 3);

  p_chi->frame.as_byte_array[p_chi->frameLength++] = COMMAND_CLASS_SENSOR_MULTILEVEL_V11;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = 0xFF;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = sensor_type_value_to_test;
  p_chi->frame.as_byte_array[p_chi->frameLength]   = sensor_scale_to_test;
  memset(&p_chi->rxOptions, 0, sizeof(p_chi->rxOptions));

  multilevel_sensor_register_interface_for_test();
  ZW_APPLICATION_TX_BUFFER frameOut;
  uint8_t frameOutLength;  
  handler_return_value = invoke_cc_handler_v2(
      &p_chi->rxOptions,
      &p_chi->frame.as_zw_application_tx_buffer,
      p_chi->frameLength,
      &frameOut,
      &frameOutLength);
  TEST_ASSERT_MESSAGE(RECEIVED_FRAME_STATUS_NO_SUPPORT == handler_return_value, "Wrong frame status :(");

  test_common_command_handler_input_free(p_chi);
}

void test_cc_multilevel_get_sensor_command_success(void)
{
  mock_calls_clear();

  received_frame_status_t handler_return_value;
  command_handler_input_t* p_chi = test_common_command_handler_input_allocate();
  ZAF_TRANSPORT_TX_BUFFER  TxBuf;
  ZW_APPLICATION_TX_BUFFER *pTxBuf = &(TxBuf.appTxBuf);
  uint8_t sensor_type_value_to_test = 0x01; /*SENSOR_NAME_AIR_TEMPERATURE*/

  memset( pTxBuf,
          0,
          sizeof(pTxBuf->ZW_SensorMultilevelReport4byteV11Frame));

  pTxBuf->ZW_SensorMultilevelReport4byteV11Frame.cmdClass   = COMMAND_CLASS_SENSOR_MULTILEVEL_V11;
  pTxBuf->ZW_SensorMultilevelReport4byteV11Frame.cmd        = SENSOR_MULTILEVEL_REPORT_V11;
  pTxBuf->ZW_SensorMultilevelReport4byteV11Frame.sensorType = sensor_type_value_to_test;
  pTxBuf->ZW_SensorMultilevelReport4byteV11Frame.level      =  SLI_TEST_CC_MULTILEVELSENSOR_SUPPORT_EXPECTED_PRECISION << 5 |
                                                               SENSOR_SCALE_CELSIUS << 3                                 |
                                                               SLI_TEST_CC_MULTILEVELSENSOR_SUPPORT_EXPECTED_SIZE;

  pTxBuf->ZW_SensorMultilevelReport4byteV11Frame.sensorValue1 = SLI_TEST_CC_MULTILEVELSENSOR_SUPPORT_EXPECTED_CELSIUS;

  p_chi->frame.as_byte_array[p_chi->frameLength++] = COMMAND_CLASS_SENSOR_MULTILEVEL_V11;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = SENSOR_MULTILEVEL_GET_V11;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = sensor_type_value_to_test;
  p_chi->frame.as_byte_array[p_chi->frameLength]   = SENSOR_SCALE_CELSIUS << 3;
  memset(&p_chi->rxOptions, 0, sizeof(p_chi->rxOptions));

  mock_call_use_as_stub(TO_STR(Check_not_legal_response_job));

  multilevel_sensor_register_interface_for_test();
  ZW_APPLICATION_TX_BUFFER frameOut;
  uint8_t frameOutLength;  
  handler_return_value = invoke_cc_handler_v2(
      &p_chi->rxOptions,
      &p_chi->frame.as_zw_application_tx_buffer,
      p_chi->frameLength,
      &frameOut,
      &frameOutLength);

  TEST_ASSERT_MESSAGE(RECEIVED_FRAME_STATUS_SUCCESS == handler_return_value, "Wrong frame status :(");
  TEST_ASSERT_EQUAL_INT8_MESSAGE(frameOutLength,
                                  sizeof(pTxBuf->ZW_SensorMultilevelReport4byteV11Frame),
                                  "Frame size does not match");
  TEST_ASSERT_EQUAL_INT8_ARRAY_MESSAGE((uint8_t*)pTxBuf, &frameOut,
                                        sizeof(pTxBuf->ZW_SensorMultilevelReport4byteV11Frame),
                                        "Frame does not match");

  test_common_command_handler_input_free(p_chi);

  mock_calls_verify();
}

void test_cc_multilevel_get_sensor_command_not_legal_job(void)
{
  mock_t* pMock = NULL;
  mock_calls_clear();

  received_frame_status_t handler_return_value;
  command_handler_input_t* p_chi = test_common_command_handler_input_allocate();
  uint8_t sensor_type_value_to_test = 0x01; /*SENSOR_NAME_AIR_TEMPERATURE*/
  uint8_t sensor_scale_to_test    = (SENSOR_SCALE_CELSIUS << 3);

  p_chi->frame.as_byte_array[p_chi->frameLength++] = COMMAND_CLASS_SENSOR_MULTILEVEL_V11;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = SENSOR_MULTILEVEL_GET_V11;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = sensor_type_value_to_test;
  p_chi->frame.as_byte_array[p_chi->frameLength]   = sensor_scale_to_test;
  memset(&p_chi->rxOptions, 0, sizeof(p_chi->rxOptions));

  mock_call_expect(TO_STR(Check_not_legal_response_job), &pMock);
  pMock->expect_arg[0].p = &p_chi->rxOptions;
  pMock->return_code.v   = true;

  multilevel_sensor_register_interface_for_test();
  ZW_APPLICATION_TX_BUFFER frameOut;
  uint8_t frameOutLength;  
  handler_return_value = invoke_cc_handler_v2(
      &p_chi->rxOptions,
      &p_chi->frame.as_zw_application_tx_buffer,
      p_chi->frameLength,
      &frameOut,
      &frameOutLength);

  TEST_ASSERT_MESSAGE(RECEIVED_FRAME_STATUS_FAIL == handler_return_value, "Wrong frame status :(");

  test_common_command_handler_input_free(p_chi);

  mock_calls_verify();
}

void test_cc_multilevel_get_sensor_command_unregistered_sensor_type(void)
{
  mock_calls_clear();

  received_frame_status_t handler_return_value;
  command_handler_input_t* p_chi = test_common_command_handler_input_allocate();
  ZAF_TRANSPORT_TX_BUFFER  TxBuf;
  ZW_APPLICATION_TX_BUFFER *pTxBuf = &(TxBuf.appTxBuf);
  uint8_t sensor_type_value_to_test   = 0x02; /*SENSOR_NAME_HUMIDITY*/
  uint8_t sensor_type_value_to_expect = 0x01; /*SENSOR_NAME_AIR_TEMPERATURE*/

  memset( pTxBuf,
        0,
        sizeof(pTxBuf->ZW_SensorMultilevelReport4byteV11Frame));

  pTxBuf->ZW_SensorMultilevelReport4byteV11Frame.cmdClass   = COMMAND_CLASS_SENSOR_MULTILEVEL_V11;
  pTxBuf->ZW_SensorMultilevelReport4byteV11Frame.cmd        = SENSOR_MULTILEVEL_REPORT_V11;
  pTxBuf->ZW_SensorMultilevelReport4byteV11Frame.sensorType = sensor_type_value_to_expect;
  pTxBuf->ZW_SensorMultilevelReport4byteV11Frame.level      =  SLI_TEST_CC_MULTILEVELSENSOR_SUPPORT_EXPECTED_PRECISION << 5 |
                                                               SENSOR_SCALE_CELSIUS << 3                                 |
                                                               SLI_TEST_CC_MULTILEVELSENSOR_SUPPORT_EXPECTED_SIZE;

  pTxBuf->ZW_SensorMultilevelReport4byteV11Frame.sensorValue1 = SLI_TEST_CC_MULTILEVELSENSOR_SUPPORT_EXPECTED_CELSIUS;

  p_chi->frame.as_byte_array[p_chi->frameLength++] = COMMAND_CLASS_SENSOR_MULTILEVEL_V11;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = SENSOR_MULTILEVEL_GET_V11;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = sensor_type_value_to_test;
  p_chi->frame.as_byte_array[p_chi->frameLength]   = SENSOR_SCALE_CELSIUS << 3;
  memset(&p_chi->rxOptions, 0, sizeof(p_chi->rxOptions));

  mock_call_use_as_stub(TO_STR(Check_not_legal_response_job));

  multilevel_sensor_register_interface_for_test();
  ZW_APPLICATION_TX_BUFFER frameOut;
  uint8_t frameOutLength;  
  handler_return_value = invoke_cc_handler_v2(
      &p_chi->rxOptions,
      &p_chi->frame.as_zw_application_tx_buffer,
      p_chi->frameLength,
      &frameOut,
      &frameOutLength);

  TEST_ASSERT_MESSAGE(RECEIVED_FRAME_STATUS_SUCCESS == handler_return_value, "Wrong frame status :(");
  TEST_ASSERT_EQUAL_INT8_MESSAGE(frameOutLength,
                                  sizeof(pTxBuf->ZW_SensorMultilevelReport4byteV11Frame),
                                  "Frame size does not match");
  TEST_ASSERT_EQUAL_INT8_ARRAY_MESSAGE((uint8_t*)pTxBuf, &frameOut,
                                        sizeof(pTxBuf->ZW_SensorMultilevelReport4byteV11Frame),
                                        "Frame does not match");

  test_common_command_handler_input_free(p_chi);

  mock_calls_verify();
}

void test_cc_multilevel_sensor_get_command_supported_sensor_success(void)
{
  mock_calls_clear();

  received_frame_status_t handler_return_value;
  command_handler_input_t* p_chi = test_common_command_handler_input_allocate();

  uint8_t output_buff[13];
  output_buff[0] = COMMAND_CLASS_SENSOR_MULTILEVEL_V11;
  output_buff[1] = SENSOR_MULTILEVEL_SUPPORTED_SENSOR_REPORT_V11;
  cc_multilevel_sensor_get_supported_sensors(&output_buff[2]);
  size_t buffer_length_bytes = 3; // Header + 1 byte of sensor indication

  p_chi->frame.as_byte_array[p_chi->frameLength++] = COMMAND_CLASS_SENSOR_MULTILEVEL_V11;
  p_chi->frame.as_byte_array[p_chi->frameLength]   = SENSOR_MULTILEVEL_SUPPORTED_GET_SENSOR_V11;
  memset(&p_chi->rxOptions, 0, sizeof(p_chi->rxOptions));

  mock_call_use_as_stub(TO_STR(Check_not_legal_response_job));

  multilevel_sensor_register_interface_for_test();
  ZW_APPLICATION_TX_BUFFER frameOut;
  uint8_t frameOutLength;  
  handler_return_value = invoke_cc_handler_v2(
      &p_chi->rxOptions,
      &p_chi->frame.as_zw_application_tx_buffer,
      p_chi->frameLength,
      &frameOut,
      &frameOutLength);

  TEST_ASSERT_MESSAGE(RECEIVED_FRAME_STATUS_SUCCESS == handler_return_value, "Wrong frame status :(");
  TEST_ASSERT_EQUAL_INT8_MESSAGE(frameOutLength,
                                  buffer_length_bytes,
                                  "Frame size does not match");
  TEST_ASSERT_EQUAL_INT8_ARRAY_MESSAGE(output_buff, &frameOut,
                                        buffer_length_bytes,
                                        "Frame does not match");

  test_common_command_handler_input_free(p_chi);

  mock_calls_verify();
}

void test_cc_multilevel_sensor_get_command_supported_sensor_not_legal_job(void)
{
  mock_t* pMock = NULL;
  mock_calls_clear();

  received_frame_status_t handler_return_value;
  command_handler_input_t* p_chi = test_common_command_handler_input_allocate();

  p_chi->frame.as_byte_array[p_chi->frameLength++] = COMMAND_CLASS_SENSOR_MULTILEVEL_V11;
  p_chi->frame.as_byte_array[p_chi->frameLength]   = SENSOR_MULTILEVEL_SUPPORTED_GET_SENSOR_V11;
  memset(&p_chi->rxOptions, 0, sizeof(p_chi->rxOptions));

  mock_call_expect(TO_STR(Check_not_legal_response_job), &pMock);
  pMock->expect_arg[0].p = &p_chi->rxOptions;
  pMock->return_code.v   = true;

  multilevel_sensor_register_interface_for_test();
  ZW_APPLICATION_TX_BUFFER frameOut;
  uint8_t frameOutLength;  
  handler_return_value = invoke_cc_handler_v2(
      &p_chi->rxOptions,
      &p_chi->frame.as_zw_application_tx_buffer,
      p_chi->frameLength,
      &frameOut,
      &frameOutLength);

  TEST_ASSERT_MESSAGE(RECEIVED_FRAME_STATUS_FAIL == handler_return_value, "Wrong frame status :(");

  test_common_command_handler_input_free(p_chi);

  mock_calls_verify();
}

void test_cc_multilevel_sensor_get_command_supported_scale_success(void)
{
  mock_calls_clear();

  received_frame_status_t handler_return_value;
  command_handler_input_t* p_chi = test_common_command_handler_input_allocate();
  uint8_t sensor_type_value_to_test = 0x01; /*SENSOR_NAME_AIR_TEMPERATURE*/
              
  uint8_t output_buff[4];
  output_buff[0] = COMMAND_CLASS_SENSOR_MULTILEVEL_V11;
  output_buff[1] = SENSOR_MULTILEVEL_SUPPORTED_SCALE_REPORT_V11;
  output_buff[2] = sensor_type_value_to_test;
  cc_multilevel_sensor_get_supported_scale(sensor_type_value_to_test, &output_buff[3]);

  p_chi->frame.as_byte_array[p_chi->frameLength++] = COMMAND_CLASS_SENSOR_MULTILEVEL_V11;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = SENSOR_MULTILEVEL_SUPPORTED_GET_SCALE_V11;
  p_chi->frame.as_byte_array[p_chi->frameLength]   = sensor_type_value_to_test;
  memset(&p_chi->rxOptions, 0, sizeof(p_chi->rxOptions));

  mock_call_use_as_stub(TO_STR(Check_not_legal_response_job));

  multilevel_sensor_register_interface_for_test();
  ZW_APPLICATION_TX_BUFFER frameOut;
  uint8_t frameOutLength;  
  handler_return_value = invoke_cc_handler_v2(
      &p_chi->rxOptions,
      &p_chi->frame.as_zw_application_tx_buffer,
      p_chi->frameLength,
      &frameOut,
      &frameOutLength);

  TEST_ASSERT_MESSAGE(RECEIVED_FRAME_STATUS_SUCCESS == handler_return_value, "Wrong frame status :(");
  TEST_ASSERT_EQUAL_INT8_MESSAGE(frameOutLength,
                                  sizeof(output_buff),
                                  "Frame size does not match");
  TEST_ASSERT_EQUAL_INT8_ARRAY_MESSAGE(output_buff, &frameOut,
                                        sizeof(output_buff),
                                        "Frame does not match");

  test_common_command_handler_input_free(p_chi);

  mock_calls_verify();
}

void test_cc_multilevel_sensor_get_command_supported_scale_not_legal_job(void)
{
  mock_t* pMock = NULL;
  mock_calls_clear();

  received_frame_status_t handler_return_value;
  command_handler_input_t* p_chi = test_common_command_handler_input_allocate();

  p_chi->frame.as_byte_array[p_chi->frameLength++] = COMMAND_CLASS_SENSOR_MULTILEVEL_V11;
  p_chi->frame.as_byte_array[p_chi->frameLength]   = SENSOR_MULTILEVEL_SUPPORTED_GET_SCALE_V11;
  memset(&p_chi->rxOptions, 0, sizeof(p_chi->rxOptions));

  mock_call_expect(TO_STR(Check_not_legal_response_job), &pMock);
  pMock->expect_arg[0].p = &p_chi->rxOptions;
  pMock->return_code.v   = true;

  multilevel_sensor_register_interface_for_test();
  ZW_APPLICATION_TX_BUFFER frameOut;
  uint8_t frameOutLength;  
  handler_return_value = invoke_cc_handler_v2(
      &p_chi->rxOptions,
      &p_chi->frame.as_zw_application_tx_buffer,
      p_chi->frameLength,
      &frameOut,
      &frameOutLength);

  TEST_ASSERT_MESSAGE(RECEIVED_FRAME_STATUS_FAIL == handler_return_value, "Wrong frame status :(");

  test_common_command_handler_input_free(p_chi);

  mock_calls_verify();
}

void test_cc_multilevel_sensor_send_sensor_data(void)
{
  mock_call_use_as_stub(TO_STR(cc_multilevel_sensor_init_iterator)); 
  mock_call_use_as_stub(TO_STR(ZAF_TSE_Trigger));

  cc_multilevel_sensor_send_sensor_data();
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
  memset(o_result, 0, sizeof(sensor_read_result_t));
  o_result->precision  = SLI_TEST_CC_MULTILEVELSENSOR_SUPPORT_EXPECTED_PRECISION;
  o_result->size_bytes = SLI_TEST_CC_MULTILEVELSENSOR_SUPPORT_EXPECTED_SIZE;

  o_result->raw_result[0] = SLI_TEST_CC_MULTILEVELSENSOR_SUPPORT_EXPECTED_CELSIUS;

  return true;
}

static void multilevel_sensor_register_interface_for_test(void)
{
  cc_multilevel_sensor_reset_administration();
  cc_multilevel_sensor_init_interface(&test_sensor_interface_A, SENSOR_NAME_AIR_TEMPERATURE);
  cc_multilevel_sensor_add_supported_scale_interface(&test_sensor_interface_A, SENSOR_SCALE_CELSIUS);
  test_sensor_interface_A.init     = init_sensor_interface;
  test_sensor_interface_A.deinit     = deinit_sensor_interface;
  test_sensor_interface_A.read_value   = sensor_interface_read;

  cc_multilevel_sensor_registration(&test_sensor_interface_A);
}
