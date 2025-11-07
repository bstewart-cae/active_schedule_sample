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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ZAF_TSE.h>
#include "ZAF_types.h"
#include <test_common.h>
#include <mock_control.h>
#include "CC_Configuration.h"
#include <ZW_TransportEndpoint.h>
#include <ZW_application_transport_interface.h>
#include "../mocks/CC_Configuration_interface_mock.h"
#include <ZAF_CC_Invoker.h>
#include <cc_configuration_config_api_mock.h>
#include "zaf_transport_tx_mock.h"

// -----------------------------------------------------------------------------
//                Macros and Typedefs
// -----------------------------------------------------------------------------
#define MAX_TX_BUFFER_PAYLOAD_SIZE_BYTES  32
 // CC | CMD | Param Number 1 | Param Number 2 | Reports to follow
#define CC_CONFIG_INFO_REPORT_HEADER  5
#define MAX_CC_CONFIG_INFO_LEN  (MAX_TX_BUFFER_PAYLOAD_SIZE_BYTES - CC_CONFIG_INFO_REPORT_HEADER)
#define ZAF_TXBUFF_GET_APPTXBUFF(x, y)    (&(x[y].appTxBuf))
#define CC_Configuration_handler(a, b)    invoke_cc_handler_v2(&b->rxOptions, \
                                                        &b->frame.as_zw_application_tx_buffer, \
                                                        b->frameLength, \
                                                        NULL, \
                                                        NULL)
// -----------------------------------------------------------------------------
//              Static Function Declarations
// -----------------------------------------------------------------------------
static uint8_t *
cc_configuration_copyToFrame(  uint8_t * pFrame,
                                  cc_config_parameter_buffer_t* parameter_buffer,
                                  uint8_t const * pField);

static uint16_t
cc_configuration_calc_reports_to_follow(size_t data_length, size_t payload_limit);
// -----------------------------------------------------------------------------
//                Global Variables
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
//                Static Variables
// -----------------------------------------------------------------------------
// -----------------------------------------------------------------------------
//              Public Function Definitions
// -----------------------------------------------------------------------------

void setUpSuite(void)
{
  cc_configuration_t const* default_configuration_pool;

  default_configuration_pool  = configuration_get_default_config_table();

  cc_configuration_get_configuration_ExpectAndReturn(default_configuration_pool);

  mock_call_use_as_fake(TO_STR(cc_configuration_io_write));
  mock_call_use_as_fake(TO_STR(cc_configuration_io_read));
  ZAF_CC_init_specific(COMMAND_CLASS_CONFIGURATION_V4);
}

void tearDownSuite(void)
{
}

void test_cc_configuration_handler_default_branch(void)
{
  received_frame_status_t handler_return_value;
  command_handler_input_t* p_chi = test_common_command_handler_input_allocate();

  p_chi->frame.as_byte_array[p_chi->frameLength++] = COMMAND_CLASS_CONFIGURATION_V4;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = 0xFF;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = 0;
  p_chi->frame.as_byte_array[p_chi->frameLength]   = 0;
  memset(&p_chi->rxOptions, 0, sizeof(p_chi->rxOptions));

  handler_return_value = CC_Configuration_handler(CC_Configuration_handler, p_chi);
  TEST_ASSERT_MESSAGE(RECEIVED_FRAME_STATUS_NO_SUPPORT == handler_return_value, "Wrong frame status :(");

  test_common_command_handler_input_free(p_chi);
}

/**
 * CONFIGURATION_INFO_GET_V4
 */
void test_cc_configuration_handler_cmd_info_get_success(void)
{
  mock_t* pMock = NULL;
  mock_calls_clear();

  received_frame_status_t handler_return_value;
  command_handler_input_t* p_chi = test_common_command_handler_input_allocate();

  const uint16_t test_parameter_number = configuration_index_dummy_limit_1;
  cc_config_parameter_buffer_t parameter_buffer;
  ZAF_TRANSPORT_TX_BUFFER* TxBuf;
  uint16_t tx_buffer_index = 0;

  cc_configuration_fill_fake_files_with_dummy_data();
  mock_call_use_as_fake(TO_STR(cc_configuration_io_read));
  cc_configuration_get(test_parameter_number, &parameter_buffer);

  uint16_t reports_to_follow_count   = 0;
  uint16_t expected_number_of_frames = 0;
  uint16_t raw_str_length            = strlen(parameter_buffer.metadata->attributes.info);
  reports_to_follow_count   = cc_configuration_calc_reports_to_follow(raw_str_length, MAX_TX_BUFFER_PAYLOAD_SIZE_BYTES);
  expected_number_of_frames = reports_to_follow_count + 1;

  TxBuf = (ZAF_TRANSPORT_TX_BUFFER*)malloc(sizeof(ZAF_TRANSPORT_TX_BUFFER) * expected_number_of_frames);

  p_chi->frame.as_byte_array[p_chi->frameLength++] = COMMAND_CLASS_CONFIGURATION_V4;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = CONFIGURATION_INFO_GET_V4;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = (uint8_t)((test_parameter_number>>8)&0xFF), //MSB;
  p_chi->frame.as_byte_array[p_chi->frameLength]   = (uint8_t) (test_parameter_number    &0xFF), //LSB;
  memset(&p_chi->rxOptions, 0, sizeof(p_chi->rxOptions));

  mock_call_expect(TO_STR(ZAF_getAppHandle), &pMock);
  SApplicationHandles AppHandles;
  SNetworkInfo NetworkInfo;
  AppHandles.pNetworkInfo    = &NetworkInfo;
  NetworkInfo.MaxPayloadSize = MAX_TX_BUFFER_PAYLOAD_SIZE_BYTES;
  pMock->return_code.p       = &AppHandles;

  zaf_transport_rx_to_tx_options_Expect(&p_chi->rxOptions, NULL);
  zaf_transport_rx_to_tx_options_IgnoreArg_tx_options();

  mock_call_use_as_stub(TO_STR(Check_not_legal_response_job));

  uint16_t str_pointer = 0;
  
  while(expected_number_of_frames--)
  {
    ZAF_TXBUFF_GET_APPTXBUFF(TxBuf,
                           tx_buffer_index)->ZW_ConfigurationInfoReport4byteV4Frame.cmdClass   = COMMAND_CLASS_CONFIGURATION_V4;
    ZAF_TXBUFF_GET_APPTXBUFF(TxBuf,
                           tx_buffer_index)->ZW_ConfigurationInfoReport4byteV4Frame.cmd        = CONFIGURATION_INFO_REPORT_V4;
    ZAF_TXBUFF_GET_APPTXBUFF(TxBuf,
                           tx_buffer_index)->ZW_ConfigurationInfoReport4byteV4Frame.parameterNumber1 = (uint8_t)((test_parameter_number>>8)&0xFF), //MSB
    ZAF_TXBUFF_GET_APPTXBUFF(TxBuf,
                           tx_buffer_index)->ZW_ConfigurationInfoReport4byteV4Frame.parameterNumber2 = (uint8_t) (test_parameter_number    &0xFF), //LSB
    ZAF_TXBUFF_GET_APPTXBUFF(TxBuf,
                           tx_buffer_index)->ZW_ConfigurationInfoReport4byteV4Frame.reportsToFollow  = reports_to_follow_count;
    reports_to_follow_count--;

    size_t remaining_byte_count = raw_str_length - str_pointer;
    size_t byte_count_to_send   = (remaining_byte_count >= MAX_CC_CONFIG_INFO_LEN) ? MAX_CC_CONFIG_INFO_LEN : remaining_byte_count;
    size_t frame_size_bytes     = 5 /*header size*/
                                  + byte_count_to_send;

    memcpy( &ZAF_TXBUFF_GET_APPTXBUFF(TxBuf, tx_buffer_index)->ZW_ConfigurationInfoReport4byteV4Frame.info1,
            &parameter_buffer.metadata->attributes.info[str_pointer],
            byte_count_to_send);

    zaf_transport_tx_ExpectAndReturn((uint8_t*)ZAF_TXBUFF_GET_APPTXBUFF(TxBuf, tx_buffer_index), frame_size_bytes, NULL, NULL, true);
    zaf_transport_tx_IgnoreArg_callback();
    zaf_transport_tx_IgnoreArg_zaf_tx_options();

    if(expected_number_of_frames == 0) {break;}

    str_pointer+=byte_count_to_send;
    tx_buffer_index++;
  }

  handler_return_value = CC_Configuration_handler(CC_Configuration_handler, p_chi);
  TEST_ASSERT_MESSAGE(RECEIVED_FRAME_STATUS_SUCCESS == handler_return_value, "Wrong frame status :(");

  test_common_command_handler_input_free(p_chi);

  mock_calls_verify();

  free(TxBuf);
}

void test_cc_configuration_handler_cmd_info_get_fail(void)
{
  mock_t* pMock = NULL;
  mock_calls_clear();

  received_frame_status_t handler_return_value;
  command_handler_input_t* p_chi = test_common_command_handler_input_allocate();

  const uint16_t test_parameter_number = configuration_index_dummy_limit_1;
  cc_config_parameter_buffer_t parameter_buffer;
  ZAF_TRANSPORT_TX_BUFFER TxBuf;

  cc_configuration_fill_fake_files_with_dummy_data();
  mock_call_use_as_fake(TO_STR(cc_configuration_io_read));
  cc_configuration_get(test_parameter_number, &parameter_buffer);

  uint16_t reports_to_follow_count   = 0;
  size_t   raw_str_length            = strlen(parameter_buffer.metadata->attributes.info);
  reports_to_follow_count            = cc_configuration_calc_reports_to_follow(raw_str_length,
                                                                                  MAX_TX_BUFFER_PAYLOAD_SIZE_BYTES);

  p_chi->frame.as_byte_array[p_chi->frameLength++] = COMMAND_CLASS_CONFIGURATION_V4;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = CONFIGURATION_INFO_GET_V4;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = (uint8_t)((test_parameter_number>>8)&0xFF), //MSB;
  p_chi->frame.as_byte_array[p_chi->frameLength]   = (uint8_t) (test_parameter_number    &0xFF), //LSB;
  memset(&p_chi->rxOptions, 0, sizeof(p_chi->rxOptions));

  mock_call_expect(TO_STR(ZAF_getAppHandle), &pMock);
  SApplicationHandles AppHandles;
  SNetworkInfo NetworkInfo;
  AppHandles.pNetworkInfo    = &NetworkInfo;
  NetworkInfo.MaxPayloadSize = MAX_TX_BUFFER_PAYLOAD_SIZE_BYTES;
  pMock->return_code.p       = &AppHandles;

  zaf_transport_rx_to_tx_options_Expect(&p_chi->rxOptions, NULL);
  zaf_transport_rx_to_tx_options_IgnoreArg_tx_options();

  mock_call_use_as_stub(TO_STR(Check_not_legal_response_job));
  
  TxBuf.appTxBuf.ZW_ConfigurationInfoReport4byteV4Frame.cmdClass         = COMMAND_CLASS_CONFIGURATION_V4;
  TxBuf.appTxBuf.ZW_ConfigurationInfoReport4byteV4Frame.cmd              = CONFIGURATION_INFO_REPORT_V4;
  TxBuf.appTxBuf.ZW_ConfigurationInfoReport4byteV4Frame.parameterNumber1 = (uint8_t)((test_parameter_number>>8)&0xFF), //MSB
  TxBuf.appTxBuf.ZW_ConfigurationInfoReport4byteV4Frame.parameterNumber2 = (uint8_t) (test_parameter_number    &0xFF), //LSB
  TxBuf.appTxBuf.ZW_ConfigurationInfoReport4byteV4Frame.reportsToFollow  = (uint8_t)reports_to_follow_count;


  size_t byte_count_to_send  = (raw_str_length >= MAX_CC_CONFIG_INFO_LEN) ? MAX_CC_CONFIG_INFO_LEN : raw_str_length;
  size_t frame_size_bytes    = 5 /*header size*/
                               + byte_count_to_send;

  memcpy( (void*)&TxBuf.appTxBuf.ZW_ConfigurationInfoReport4byteV4Frame.info1,
          (void*)parameter_buffer.metadata->attributes.info,
          byte_count_to_send);

  zaf_transport_tx_ExpectAndReturn((uint8_t*)&TxBuf.appTxBuf, frame_size_bytes, NULL, NULL, false);
  zaf_transport_tx_IgnoreArg_zaf_tx_options();

  handler_return_value = CC_Configuration_handler(CC_Configuration_handler, p_chi);
  TEST_ASSERT_MESSAGE(RECEIVED_FRAME_STATUS_FAIL == handler_return_value, "Wrong frame status :(");

  test_common_command_handler_input_free(p_chi);

  mock_calls_verify();
}

void test_cc_configuration_command_not_legal_job(void)
{
  mock_t* pMock = NULL;
  mock_calls_clear();

  mock_call_use_as_stub(TO_STR(ZAF_getAppHandle));

  received_frame_status_t handler_return_value;
  command_handler_input_t* p_chi = test_common_command_handler_input_allocate();

  uint16_t test_parameter_number = 0x0001;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = COMMAND_CLASS_CONFIGURATION_V4;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = CONFIGURATION_INFO_GET_V4;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = (uint8_t)((test_parameter_number>>8)&0xFF), //MSB;
  p_chi->frame.as_byte_array[p_chi->frameLength]   = (uint8_t) (test_parameter_number    &0xFF), //LSB;
  memset(&p_chi->rxOptions, 0, sizeof(p_chi->rxOptions));

  mock_call_expect(TO_STR(Check_not_legal_response_job), &pMock);
  pMock->expect_arg[0].p = &p_chi->rxOptions;
  pMock->return_code.v   = true;

  handler_return_value = CC_Configuration_handler(CC_Configuration_handler, p_chi);
  TEST_ASSERT_MESSAGE(RECEIVED_FRAME_STATUS_FAIL == handler_return_value, "Wrong frame status :(");

  test_common_command_handler_input_free(p_chi);

  mock_calls_verify();
}

/**
 * CONFIGURATION_GET_V4
 */
void test_cc_configuration_handler_cmd_get_success(void)
{
  mock_calls_clear();

  received_frame_status_t handler_return_value;
  command_handler_input_t* p_chi = test_common_command_handler_input_allocate();
  ZAF_TRANSPORT_TX_BUFFER TxBuf;

  memset(&TxBuf, 0, sizeof(ZAF_TRANSPORT_TX_BUFFER));

  const uint8_t  test_parameter_number          = configuration_index_dummy_limit_1;
  cc_config_parameter_buffer_t parameter_buffer;

  cc_configuration_fill_fake_files_with_dummy_data();
  mock_call_use_as_fake(TO_STR(cc_configuration_io_read));
  cc_configuration_get(test_parameter_number, &parameter_buffer);

  TxBuf.appTxBuf.ZW_ConfigurationReport4byteV4Frame.cmdClass = COMMAND_CLASS_CONFIGURATION_V4;
  TxBuf.appTxBuf.ZW_ConfigurationReport4byteV4Frame.cmd      = CONFIGURATION_REPORT_V4;
  TxBuf.appTxBuf.ZW_ConfigurationReport4byteV4Frame.parameterNumber = test_parameter_number,
  TxBuf.appTxBuf.ZW_ConfigurationReport4byteV4Frame.level    = parameter_buffer.metadata->attributes.size;
 
  cc_configuration_copyToFrame(&TxBuf.appTxBuf.ZW_ConfigurationReport4byteV4Frame.configurationValue1,
                                  &parameter_buffer,
                                  parameter_buffer.data_buffer.as_uint8_array);

  p_chi->frame.as_byte_array[p_chi->frameLength++] = COMMAND_CLASS_CONFIGURATION_V4;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = CONFIGURATION_GET_V4;
  p_chi->frame.as_byte_array[p_chi->frameLength  ] = test_parameter_number;
  memset(&p_chi->rxOptions, 0, sizeof(p_chi->rxOptions));

  mock_call_use_as_stub(TO_STR(Check_not_legal_response_job));

  zaf_transport_rx_to_tx_options_Expect(&p_chi->rxOptions, NULL);
  zaf_transport_rx_to_tx_options_IgnoreArg_tx_options();

  zaf_transport_tx_ExpectAndReturn((uint8_t*)&TxBuf.appTxBuf.ZW_ConfigurationReport4byteV4Frame, 
                                    sizeof(TxBuf.appTxBuf.ZW_ConfigurationReport4byteV4Frame), NULL, NULL, true);
  zaf_transport_tx_IgnoreArg_zaf_tx_options();

  handler_return_value = CC_Configuration_handler(CC_Configuration_handler, p_chi);
  TEST_ASSERT_MESSAGE(RECEIVED_FRAME_STATUS_SUCCESS == handler_return_value, "Wrong frame status :(");

  test_common_command_handler_input_free(p_chi);

  mock_calls_verify();
}

void test_cc_configuration_handler_cmd_get_multicast(void)
{
  mock_t* pMock = NULL;
  mock_calls_clear();

  received_frame_status_t handler_return_value;
  command_handler_input_t* p_chi = test_common_command_handler_input_allocate();

  p_chi->frame.as_byte_array[p_chi->frameLength++] = COMMAND_CLASS_CONFIGURATION_V4;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = CONFIGURATION_GET_V4;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = 0, //MSB;
  p_chi->frame.as_byte_array[p_chi->frameLength]   = 0, //LSB;
  memset(&p_chi->rxOptions, 0, sizeof(p_chi->rxOptions));

  mock_call_expect(TO_STR(Check_not_legal_response_job), &pMock);
  pMock->expect_arg[0].p = &p_chi->rxOptions;
  pMock->return_code.v   = true;

  handler_return_value = CC_Configuration_handler(CC_Configuration_handler, p_chi);
  TEST_ASSERT_MESSAGE(RECEIVED_FRAME_STATUS_FAIL == handler_return_value, "Wrong frame status :(");

  test_common_command_handler_input_free(p_chi);

  mock_calls_verify();
}

/**
 * CONFIGURATION_SET_V4
 */
void test_cc_configuration_handler_cmd_set_success(void)
{
  received_frame_status_t handler_return_value;
  command_handler_input_t* p_chi = test_common_command_handler_input_allocate();

  const uint8_t  test_parameter_number = configuration_index_dummy_limit_1;
  cc_config_parameter_buffer_t parameter_buffer;
  cc_config_parameter_value_t new_configuration_value = {
    .as_int16 = 89 //This is a random value
  };

  cc_configuration_fill_fake_files_with_dummy_data();
  mock_call_use_as_fake(TO_STR(cc_configuration_io_read));

  cc_configuration_get(test_parameter_number, &parameter_buffer);

  p_chi->frame.as_byte_array[p_chi->frameLength++] = COMMAND_CLASS_CONFIGURATION_V4;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = CONFIGURATION_SET_V4;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = test_parameter_number;                      //Parameter number
  p_chi->frame.as_byte_array[p_chi->frameLength++] = parameter_buffer.metadata->attributes.size; //Size
  p_chi->frame.as_byte_array[p_chi->frameLength++] = new_configuration_value.as_uint8_array[1];  //Configuration value  MSB
  p_chi->frame.as_byte_array[p_chi->frameLength++] = new_configuration_value.as_uint8_array[0];  //                     LSB
  
  memset(&p_chi->rxOptions, 0, sizeof(p_chi->rxOptions));
  mock_call_use_as_fake(TO_STR(cc_configuration_io_write));

  handler_return_value = CC_Configuration_handler(CC_Configuration_handler, p_chi);
  TEST_ASSERT_MESSAGE(RECEIVED_FRAME_STATUS_SUCCESS == handler_return_value, "Wrong frame status :(");

  cc_configuration_get(test_parameter_number, &parameter_buffer);

  TEST_ASSERT_EQUAL_UINT8_ARRAY(new_configuration_value.as_uint8_array,
                                parameter_buffer.data_buffer.as_uint8_array,
                                sizeof(new_configuration_value.as_uint8_array));

  test_common_command_handler_input_free(p_chi);
}

void test_cc_configuration_handler_cmd_set_default_success(void)
{
  received_frame_status_t handler_return_value;
  command_handler_input_t* p_chi = test_common_command_handler_input_allocate();

  const uint8_t  test_parameter_number = configuration_index_dummy_limit_1;
  cc_config_parameter_buffer_t parameter_buffer;

  cc_configuration_fill_fake_files_with_dummy_data();

  p_chi->frame.as_byte_array[p_chi->frameLength++] = COMMAND_CLASS_CONFIGURATION_V4;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = CONFIGURATION_SET_V4;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = test_parameter_number;                  //Parameter number
  p_chi->frame.as_byte_array[p_chi->frameLength++] = 0x80 |                                  // Default flag
                                                     CC_CONFIG_PARAMETER_SIZE_16_BIT;        // Size
  p_chi->frame.as_byte_array[p_chi->frameLength++] = 0;                                      //Configuration value
  p_chi->frame.as_byte_array[p_chi->frameLength++] = 0;
  
  memset(&p_chi->rxOptions, 0, sizeof(p_chi->rxOptions));

  mock_call_use_as_stub(TO_STR(Check_not_legal_response_job));
  mock_call_use_as_fake(TO_STR(cc_configuration_io_write));

  handler_return_value = CC_Configuration_handler(CC_Configuration_handler, p_chi);
  TEST_ASSERT_MESSAGE(RECEIVED_FRAME_STATUS_SUCCESS == handler_return_value, "Wrong frame status :(");

  cc_configuration_get(test_parameter_number, &parameter_buffer);

  TEST_ASSERT_EQUAL_UINT8_ARRAY(parameter_buffer.metadata->attributes.default_value.as_uint8_array,
                                parameter_buffer.data_buffer.as_uint8_array,
                                sizeof(parameter_buffer.data_buffer.as_uint8_array));

  test_common_command_handler_input_free(p_chi);
}

/**
 * CONFIGURATION_NAME_GET_V4
 */
void test_cc_configuration_handler_cmd_name_get_success(void)
{
  mock_t* pMock = NULL;
  mock_calls_clear();

  received_frame_status_t handler_return_value;
  command_handler_input_t* p_chi = test_common_command_handler_input_allocate();
  const uint16_t test_parameter_number = configuration_index_dummy_limit_1;
  cc_config_parameter_buffer_t parameter_buffer;
  ZAF_TRANSPORT_TX_BUFFER* TxBuf;
  uint16_t tx_buffer_index = 0;

  cc_configuration_fill_fake_files_with_dummy_data();
  mock_call_use_as_fake(TO_STR(cc_configuration_io_read));
  cc_configuration_get(test_parameter_number, &parameter_buffer);

  uint16_t expected_number_of_frames = 0;
  size_t   raw_str_length            = strlen(parameter_buffer.metadata->attributes.name);
  uint16_t reports_to_follow_count   = 0;
  reports_to_follow_count   = cc_configuration_calc_reports_to_follow(raw_str_length, MAX_TX_BUFFER_PAYLOAD_SIZE_BYTES);
  expected_number_of_frames = reports_to_follow_count + 1;

  TxBuf = (ZAF_TRANSPORT_TX_BUFFER*)malloc(sizeof(ZAF_TRANSPORT_TX_BUFFER) * expected_number_of_frames);

  p_chi->frame.as_byte_array[p_chi->frameLength++] = COMMAND_CLASS_CONFIGURATION_V4;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = CONFIGURATION_NAME_GET_V4;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = (uint8_t)((test_parameter_number>>8)&0xFF), //MSB;
  p_chi->frame.as_byte_array[p_chi->frameLength]   = (uint8_t) (test_parameter_number    &0xFF), //LSB;
  memset(&p_chi->rxOptions, 0, sizeof(p_chi->rxOptions));

  mock_call_use_as_stub(TO_STR(Check_not_legal_response_job));

  mock_call_expect(TO_STR(ZAF_getAppHandle), &pMock);
  SApplicationHandles AppHandles;
  SNetworkInfo NetworkInfo;
  AppHandles.pNetworkInfo = &NetworkInfo;
  NetworkInfo.MaxPayloadSize = MAX_TX_BUFFER_PAYLOAD_SIZE_BYTES;
  pMock->return_code.p = &AppHandles;

  zaf_transport_rx_to_tx_options_Expect(&p_chi->rxOptions, NULL);
  zaf_transport_rx_to_tx_options_IgnoreArg_tx_options();

  uint16_t str_pointer = 0;
  
  while(expected_number_of_frames--)
  {
    ZAF_TXBUFF_GET_APPTXBUFF(TxBuf,
                           tx_buffer_index)->ZW_ConfigurationNameReport4byteV4Frame.cmdClass   = COMMAND_CLASS_CONFIGURATION_V4;
    ZAF_TXBUFF_GET_APPTXBUFF(TxBuf,
                           tx_buffer_index)->ZW_ConfigurationNameReport4byteV4Frame.cmd        = CONFIGURATION_NAME_REPORT_V4;
    ZAF_TXBUFF_GET_APPTXBUFF(TxBuf,
                           tx_buffer_index)->ZW_ConfigurationNameReport4byteV4Frame.parameterNumber1 = (uint8_t)((test_parameter_number>>8)&0xFF), //MSB
    ZAF_TXBUFF_GET_APPTXBUFF(TxBuf,
                           tx_buffer_index)->ZW_ConfigurationNameReport4byteV4Frame.parameterNumber2 = (uint8_t) (test_parameter_number    &0xFF), //LSB
    ZAF_TXBUFF_GET_APPTXBUFF(TxBuf,
                           tx_buffer_index)->ZW_ConfigurationNameReport4byteV4Frame.reportsToFollow  = (uint8_t)reports_to_follow_count;
    reports_to_follow_count--;

    size_t remaining_byte_count = raw_str_length - str_pointer;
    size_t byte_count_to_send   = (remaining_byte_count >= MAX_TX_BUFFER_PAYLOAD_SIZE_BYTES) ? MAX_TX_BUFFER_PAYLOAD_SIZE_BYTES : remaining_byte_count;
    size_t frame_size_bytes     = 5 /*header size*/
                                  + byte_count_to_send;

    memcpy( &ZAF_TXBUFF_GET_APPTXBUFF(TxBuf, tx_buffer_index)->ZW_ConfigurationNameReport4byteV4Frame.name1,
            &parameter_buffer.metadata->attributes.name[str_pointer],
            byte_count_to_send);

    zaf_transport_tx_ExpectAndReturn((uint8_t*)ZAF_TXBUFF_GET_APPTXBUFF(TxBuf, tx_buffer_index), frame_size_bytes, NULL, NULL, true);
    zaf_transport_tx_IgnoreArg_zaf_tx_options();

    if(expected_number_of_frames == 0) {break;}

    str_pointer+=byte_count_to_send;
    tx_buffer_index++;
  }

  handler_return_value = CC_Configuration_handler(CC_Configuration_handler, p_chi);
  TEST_ASSERT_MESSAGE(RECEIVED_FRAME_STATUS_SUCCESS == handler_return_value, "Wrong frame status :(");

  test_common_command_handler_input_free(p_chi);

  mock_calls_verify();

  free(TxBuf);
}

void test_cc_configuration_handler_cmd_name_get_multicast(void)
{
  mock_t* pMock = NULL;
  mock_calls_clear();

  received_frame_status_t handler_return_value;
  command_handler_input_t* p_chi = test_common_command_handler_input_allocate();

  p_chi->frame.as_byte_array[p_chi->frameLength++] = COMMAND_CLASS_CONFIGURATION_V4;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = CONFIGURATION_NAME_GET_V4;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = 0; //MSB;
  p_chi->frame.as_byte_array[p_chi->frameLength]   = 0; //LSB;
  memset(&p_chi->rxOptions, 0, sizeof(p_chi->rxOptions));

  mock_call_expect(TO_STR(Check_not_legal_response_job), &pMock);
  pMock->expect_arg[0].p = &p_chi->rxOptions;
  pMock->return_code.v   = true;

  mock_call_expect(TO_STR(ZAF_getAppHandle), &pMock);
  SApplicationHandles AppHandles;
  SNetworkInfo NetworkInfo;
  AppHandles.pNetworkInfo = &NetworkInfo;
  NetworkInfo.MaxPayloadSize = MAX_TX_BUFFER_PAYLOAD_SIZE_BYTES;
  pMock->return_code.p = &AppHandles;

  handler_return_value = CC_Configuration_handler(CC_Configuration_handler, p_chi);
  TEST_ASSERT_MESSAGE(RECEIVED_FRAME_STATUS_FAIL == handler_return_value, "Wrong frame status :(");

  test_common_command_handler_input_free(p_chi);

  mock_calls_verify();
}

/**
 * CONFIGURATION_PROPERTIES_GET_V4
 */
void test_cc_configuration_handler_cmd_properties_get_success(void)
{
    mock_calls_clear();

  received_frame_status_t handler_return_value;
  command_handler_input_t* p_chi = test_common_command_handler_input_allocate();
  ZAF_TRANSPORT_TX_BUFFER TxBuf;

  cc_config_parameter_buffer_t parameter_buffer;
  const uint16_t test_parameter_number = configuration_index_dummy_limit_1;

  cc_configuration_fill_fake_files_with_dummy_data();
  mock_call_use_as_fake(TO_STR(cc_configuration_io_read));
  cc_configuration_get(test_parameter_number, &parameter_buffer);

  memset(&TxBuf.appTxBuf.ZW_ConfigurationPropertiesReport4byteV4Frame,
          0,
          sizeof(TxBuf.appTxBuf.ZW_ConfigurationPropertiesReport4byteV4Frame));
                                                                    
  TxBuf.appTxBuf.ZW_ConfigurationPropertiesReport4byteV4Frame.cmdClass         = COMMAND_CLASS_CONFIGURATION_V4;
  TxBuf.appTxBuf.ZW_ConfigurationPropertiesReport4byteV4Frame.cmd              = CONFIGURATION_PROPERTIES_REPORT_V4;
  TxBuf.appTxBuf.ZW_ConfigurationPropertiesReport4byteV4Frame.parameterNumber1 = (uint8_t)((test_parameter_number>>8)&0xFF);
  TxBuf.appTxBuf.ZW_ConfigurationPropertiesReport4byteV4Frame.parameterNumber2 = (uint8_t) (test_parameter_number    &0xFF);

  TxBuf.appTxBuf.ZW_ConfigurationPropertiesReport4byteV4Frame.properties1 = (parameter_buffer.metadata->attributes.format<<3) |
                                                                            (parameter_buffer.metadata->attributes.size);

  uint8_t * pFrame_min = &TxBuf.appTxBuf.ZW_ConfigurationPropertiesReport4byteV4Frame.minValue1;
  uint8_t * pFrame_max = &TxBuf.appTxBuf.ZW_ConfigurationPropertiesReport4byteV4Frame.maxValue1;
  uint8_t * pFrame_def = &TxBuf.appTxBuf.ZW_ConfigurationPropertiesReport4byteV4Frame.defaultValue1;

  switch (parameter_buffer.metadata->attributes.size) {
    case CC_CONFIG_PARAMETER_SIZE_8_BIT:
      pFrame_min = &TxBuf.appTxBuf.ZW_ConfigurationPropertiesReport1byteV4Frame.minValue1;
      pFrame_max = &TxBuf.appTxBuf.ZW_ConfigurationPropertiesReport1byteV4Frame.maxValue1;
      pFrame_def = &TxBuf.appTxBuf.ZW_ConfigurationPropertiesReport1byteV4Frame.defaultValue1;
      break;
    case CC_CONFIG_PARAMETER_SIZE_16_BIT:
      pFrame_min = &TxBuf.appTxBuf.ZW_ConfigurationPropertiesReport2byteV4Frame.minValue1;
      pFrame_max = &TxBuf.appTxBuf.ZW_ConfigurationPropertiesReport2byteV4Frame.maxValue1;
      pFrame_def = &TxBuf.appTxBuf.ZW_ConfigurationPropertiesReport2byteV4Frame.defaultValue1;
      break;
    case CC_CONFIG_PARAMETER_SIZE_32_BIT:
    default:
      break;
  }
  cc_configuration_copyToFrame(pFrame_min,
                               &parameter_buffer,
                               (uint8_t *)&parameter_buffer.metadata->attributes.min_value);
  cc_configuration_copyToFrame(pFrame_max,
                               &parameter_buffer,
                               (uint8_t *)&parameter_buffer.metadata->attributes.max_value);
  cc_configuration_copyToFrame(pFrame_def,
                               &parameter_buffer,
                               (uint8_t *)&parameter_buffer.metadata->attributes.default_value);

  TxBuf.appTxBuf.ZW_ConfigurationPropertiesReport4byteV4Frame.nextParameterNumber1 = (parameter_buffer.metadata->next_number >> 8)&0xFF;
  TxBuf.appTxBuf.ZW_ConfigurationPropertiesReport4byteV4Frame.nextParameterNumber2 = (parameter_buffer.metadata->next_number)&0xFF;
  
  p_chi->frame.as_byte_array[p_chi->frameLength++] = COMMAND_CLASS_CONFIGURATION_V4;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = CONFIGURATION_PROPERTIES_GET_V4;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = (uint8_t)((test_parameter_number>>8)&0xFF), //MSB;
  p_chi->frame.as_byte_array[p_chi->frameLength]   = (uint8_t) (test_parameter_number    &0xFF), //LSB;
  memset(&p_chi->rxOptions, 0, sizeof(p_chi->rxOptions));

  mock_call_use_as_stub(TO_STR(Check_not_legal_response_job));

  zaf_transport_rx_to_tx_options_Expect(&p_chi->rxOptions, NULL);
  zaf_transport_rx_to_tx_options_IgnoreArg_tx_options();

  zaf_transport_tx_ExpectAndReturn((uint8_t*)&TxBuf.appTxBuf.ZW_ConfigurationPropertiesReport4byteV4Frame, 
                                    sizeof(TxBuf.appTxBuf.ZW_ConfigurationPropertiesReport2byteV4Frame), NULL, NULL, true);
  zaf_transport_tx_IgnoreArg_zaf_tx_options();

  handler_return_value = CC_Configuration_handler(CC_Configuration_handler, p_chi);
  TEST_ASSERT_MESSAGE(RECEIVED_FRAME_STATUS_SUCCESS == handler_return_value, "Wrong frame status :(");

  test_common_command_handler_input_free(p_chi);

  mock_calls_verify();
}

void test_cc_configuration_handler_cmd_properties_get_invalid_parameter_number(void)
{
  mock_calls_clear();

  received_frame_status_t handler_return_value;
  command_handler_input_t* p_chi = test_common_command_handler_input_allocate();
  ZAF_TRANSPORT_TX_BUFFER TxBuf;

  cc_config_parameter_buffer_t parameter_buffer;
  const uint16_t test_parameter_number = configuration_index_count;  // invalid parameter number

  cc_configuration_fill_fake_files_with_dummy_data();
  mock_call_use_as_fake(TO_STR(cc_configuration_io_read));
  cc_configuration_get(test_parameter_number, &parameter_buffer);

  memset(&TxBuf.appTxBuf.ZW_ConfigurationPropertiesReport4byteV4Frame,
          0,
          sizeof(TxBuf.appTxBuf.ZW_ConfigurationPropertiesReport4byteV4Frame));
                                                                    
  TxBuf.appTxBuf.ZW_ConfigurationPropertiesReport4byteV4Frame.cmdClass         = COMMAND_CLASS_CONFIGURATION_V4;
  TxBuf.appTxBuf.ZW_ConfigurationPropertiesReport4byteV4Frame.cmd              = CONFIGURATION_PROPERTIES_REPORT_V4;
  TxBuf.appTxBuf.ZW_ConfigurationPropertiesReport4byteV4Frame.parameterNumber1 = (uint8_t)((test_parameter_number>>8)&0xFF);
  TxBuf.appTxBuf.ZW_ConfigurationPropertiesReport4byteV4Frame.parameterNumber2 = (uint8_t) (test_parameter_number    &0xFF);
  TxBuf.appTxBuf.ZW_ConfigurationPropertiesReport4byteV4Frame.properties1 = 0;
   
  p_chi->frame.as_byte_array[p_chi->frameLength++] = COMMAND_CLASS_CONFIGURATION_V4;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = CONFIGURATION_PROPERTIES_GET_V4;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = (uint8_t)((test_parameter_number>>8)&0xFF), //MSB;
  p_chi->frame.as_byte_array[p_chi->frameLength]   = (uint8_t) (test_parameter_number    &0xFF), //LSB;
  memset(&p_chi->rxOptions, 0, sizeof(p_chi->rxOptions));

  mock_call_use_as_stub(TO_STR(Check_not_legal_response_job));

  zaf_transport_rx_to_tx_options_Expect(&p_chi->rxOptions, NULL);
  zaf_transport_rx_to_tx_options_IgnoreArg_tx_options();

  zaf_transport_tx_ExpectAndReturn(NULL, 8, NULL, NULL, true);
  zaf_transport_tx_IgnoreArg_frame();
  zaf_transport_tx_IgnoreArg_zaf_tx_options();

  handler_return_value = CC_Configuration_handler(CC_Configuration_handler, p_chi);
  TEST_ASSERT_MESSAGE(RECEIVED_FRAME_STATUS_SUCCESS == handler_return_value, "Wrong frame status :(");

  test_common_command_handler_input_free(p_chi);

  mock_calls_verify();
}

void test_cc_configuration_handler_cmd_properties_get_multicast(void)
{
  mock_t* pMock = NULL;
  mock_calls_clear();

  received_frame_status_t handler_return_value;
  command_handler_input_t* p_chi = test_common_command_handler_input_allocate();

  p_chi->frame.as_byte_array[p_chi->frameLength++] = COMMAND_CLASS_CONFIGURATION_V4;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = CONFIGURATION_PROPERTIES_GET_V4;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = 0;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = 0;
  p_chi->frame.as_byte_array[p_chi->frameLength]   = 0;
  memset(&p_chi->rxOptions, 0, sizeof(p_chi->rxOptions));

  mock_call_expect(TO_STR(Check_not_legal_response_job), &pMock);
  pMock->expect_arg[0].p = &p_chi->rxOptions;
  pMock->return_code.v   = true;

  handler_return_value = CC_Configuration_handler(CC_Configuration_handler, p_chi);
  TEST_ASSERT_MESSAGE(RECEIVED_FRAME_STATUS_FAIL == handler_return_value, "Wrong frame status :(");

  test_common_command_handler_input_free(p_chi);

  mock_calls_verify();
}

/**
 * CONFIGURATION_BULK_GET_V4
 */
void test_cc_configuration_handler_cmd_bulk_get_success(void)
{
  TEST_IGNORE();
  mock_t* pMock = NULL;
  mock_calls_clear();

  received_frame_status_t handler_return_value;
  command_handler_input_t* p_chi = test_common_command_handler_input_allocate();

  const uint16_t parameter_offset      = 0;
  const uint8_t  number_of_parameters  = 2;
  cc_config_parameter_buffer_t parameter_buffer;
  ZAF_TRANSPORT_TX_BUFFER* TxBuf;
  uint16_t tx_buffer_index = 0;
  uint16_t sum_size_to_report      = 0;
  uint16_t continous_parater_count = 0;
  uint8_t reports_to_follow_count  = 0;
  bool io_transaction_result = true;
  uint16_t required_parameter_count = number_of_parameters - parameter_offset;
  uint16_t first_parameter_number   = parameter_offset; 

  cc_configuration_fill_fake_files_with_dummy_data();
  mock_call_use_as_fake(TO_STR(cc_configuration_io_read));
  io_transaction_result = cc_configuration_get(first_parameter_number, &parameter_buffer);

  while(io_transaction_result != false)
  {
    sum_size_to_report += parameter_buffer.metadata->attributes.size;
    continous_parater_count++;
    if(continous_parater_count == required_parameter_count)
    {
      // We are done, found all of the parameters
      break;
    }

    io_transaction_result = cc_configuration_get(parameter_buffer.metadata->next_number, &parameter_buffer);
  }

  reports_to_follow_count = cc_configuration_calc_reports_to_follow(sum_size_to_report, MAX_TX_BUFFER_PAYLOAD_SIZE_BYTES);
  
  TxBuf = (ZAF_TRANSPORT_TX_BUFFER*)malloc(sizeof(ZAF_TRANSPORT_TX_BUFFER) * (reports_to_follow_count + 1));

  p_chi->frame.as_byte_array[p_chi->frameLength++] = COMMAND_CLASS_CONFIGURATION_V4;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = CONFIGURATION_BULK_GET_V4;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = (uint8_t)((parameter_offset>>8)&0xFF); //MSB;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = (uint8_t) (parameter_offset    &0xFF); //LSB;
  p_chi->frame.as_byte_array[p_chi->frameLength]   = number_of_parameters;
  memset(&p_chi->rxOptions, 0, sizeof(p_chi->rxOptions));

  mock_call_use_as_stub(TO_STR(Check_not_legal_response_job));
  mock_call_use_as_stub(TO_STR(cc_configuration_io_read));

  mock_call_expect(TO_STR(ZAF_getAppHandle), &pMock);
  SApplicationHandles AppHandles;
  SNetworkInfo NetworkInfo;
  AppHandles.pNetworkInfo = &NetworkInfo;
  NetworkInfo.MaxPayloadSize = MAX_TX_BUFFER_PAYLOAD_SIZE_BYTES;
  pMock->return_code.p = &AppHandles;

  zaf_transport_rx_to_tx_options_Expect(&p_chi->rxOptions, NULL);
  zaf_transport_rx_to_tx_options_IgnoreArg_tx_options();

  size_t  current_payload_size = 0;
  uint8_t sent_parameter_count = 0;

  io_transaction_result = cc_configuration_get(first_parameter_number, &parameter_buffer);

  while(sent_parameter_count < continous_parater_count)
  {
    sent_parameter_count++;

    uint8_t* buffer = (uint8_t*)&ZAF_TXBUFF_GET_APPTXBUFF(TxBuf,
                                                        tx_buffer_index)->ZW_ConfigurationBulkReport4byteV4Frame.variantgroup1;

    buffer += current_payload_size;

    cc_configuration_copyToFrame(buffer,
                                    &parameter_buffer,
                                    parameter_buffer.data_buffer.as_uint8_array);
    
    ZAF_TXBUFF_GET_APPTXBUFF(TxBuf,
                tx_buffer_index)->ZW_ConfigurationBulkReport4byteV4Frame.numberOfParameters++;

    current_payload_size += parameter_buffer.metadata->attributes.size;

    if( (current_payload_size                   == MAX_TX_BUFFER_PAYLOAD_SIZE_BYTES)||
        (parameter_buffer.metadata->next_number == 0)                               ||
        (sent_parameter_count                   == continous_parater_count))
    {
      size_t frame_size_bytes = 7 /*header size*/
                                + current_payload_size;

      ZAF_TXBUFF_GET_APPTXBUFF(TxBuf,
                tx_buffer_index)->ZW_ConfigurationBulkReport4byteV4Frame.cmdClass   = COMMAND_CLASS_CONFIGURATION_V4;
      ZAF_TXBUFF_GET_APPTXBUFF(TxBuf,
                tx_buffer_index)->ZW_ConfigurationBulkReport4byteV4Frame.cmd        = CONFIGURATION_BULK_REPORT_V4;
      ZAF_TXBUFF_GET_APPTXBUFF(TxBuf,
                tx_buffer_index)->ZW_ConfigurationBulkReport4byteV4Frame.parameterOffset1 = (uint8_t)((parameter_offset>>8)&0xFF), //MSB
      ZAF_TXBUFF_GET_APPTXBUFF(TxBuf,
                tx_buffer_index)->ZW_ConfigurationBulkReport4byteV4Frame.parameterOffset2 = (uint8_t) (parameter_offset&0xFF), //LSB
      ZAF_TXBUFF_GET_APPTXBUFF(TxBuf,
                tx_buffer_index)->ZW_ConfigurationBulkReport4byteV4Frame.reportsToFollow  = reports_to_follow_count;
      ZAF_TXBUFF_GET_APPTXBUFF(TxBuf,
                tx_buffer_index)->ZW_ConfigurationBulkReport4byteV4Frame.properties1 = parameter_buffer.metadata->attributes.size;

      zaf_transport_tx_ExpectAndReturn((uint8_t*)ZAF_TXBUFF_GET_APPTXBUFF(TxBuf, tx_buffer_index), frame_size_bytes, NULL, NULL, true);
      zaf_transport_tx_IgnoreArg_callback();
      zaf_transport_tx_IgnoreArg_zaf_tx_options();

      tx_buffer_index++;
      current_payload_size = 0;
      reports_to_follow_count--;
      ZAF_TXBUFF_GET_APPTXBUFF( TxBuf,
                              tx_buffer_index)->ZW_ConfigurationBulkReport4byteV4Frame.numberOfParameters = 0;
    }

    io_transaction_result = cc_configuration_get(parameter_buffer.metadata->next_number, &parameter_buffer);
  }

  handler_return_value = CC_Configuration_handler(CC_Configuration_handler, p_chi);
  TEST_ASSERT_MESSAGE(RECEIVED_FRAME_STATUS_FAIL == handler_return_value, "Wrong frame status :(");

  test_common_command_handler_input_free(p_chi);

  mock_calls_verify();

  free(TxBuf);
}

void test_cc_configuration_handler_cmd_bulk_get_multicast(void)
{
  mock_t* pMock = NULL;
  mock_calls_clear();

  received_frame_status_t handler_return_value;
  command_handler_input_t* p_chi = test_common_command_handler_input_allocate();

  p_chi->frame.as_byte_array[p_chi->frameLength++] = COMMAND_CLASS_CONFIGURATION_V4;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = CONFIGURATION_BULK_GET_V4;

  memset(&p_chi->rxOptions, 0, sizeof(p_chi->rxOptions));

  mock_call_expect(TO_STR(Check_not_legal_response_job), &pMock);
  pMock->expect_arg[0].p = &p_chi->rxOptions;
  pMock->return_code.v   = true;

  handler_return_value = CC_Configuration_handler(CC_Configuration_handler, p_chi);
  TEST_ASSERT_MESSAGE(RECEIVED_FRAME_STATUS_FAIL == handler_return_value, "Wrong frame status :(");

  test_common_command_handler_input_free(p_chi);

  mock_calls_verify();
}

void test_cc_configuration_handler_cmd_bulk_set_valid_handler(void)
{
  received_frame_status_t handler_return_value;
  command_handler_input_t* p_chi = test_common_command_handler_input_allocate();

  const uint16_t parameter_offset               = 1;
  const uint8_t  number_of_parameters           = 2;

  p_chi->frame.as_byte_array[p_chi->frameLength++] = COMMAND_CLASS_CONFIGURATION_V4;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = CONFIGURATION_BULK_SET_V4;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = (uint8_t)((parameter_offset>>8)&0xFF); // Parameter offset MSB
  p_chi->frame.as_byte_array[p_chi->frameLength++] = (uint8_t)(parameter_offset&0xFF);      // Parameter offset LSB
  p_chi->frame.as_byte_array[p_chi->frameLength++] = number_of_parameters;                  // Number of parameters
  p_chi->frame.as_byte_array[p_chi->frameLength++] = (1<<7) | // Default flag
                                                     (0<<6) | // Handshake
                                                         2; // Size

  mock_call_use_as_stub(TO_STR(cc_configuration_io_write));
  mock_call_use_as_stub(TO_STR(cc_configuration_io_read));
  
  memset(&p_chi->rxOptions, 0, sizeof(p_chi->rxOptions));

  handler_return_value = CC_Configuration_handler(CC_Configuration_handler, p_chi);
  TEST_ASSERT_MESSAGE(RECEIVED_FRAME_STATUS_SUCCESS == handler_return_value, "Wrong frame status :(");

  test_common_command_handler_input_free(p_chi);
}

void test_cc_configuration_handler_cmd_bulk_set_default_flag_false(void)
{
  received_frame_status_t handler_return_value;
  command_handler_input_t* p_chi = test_common_command_handler_input_allocate();

  const uint16_t parameter_offset               = 1;
  const uint8_t  number_of_parameters           = 2;

  p_chi->frame.as_byte_array[p_chi->frameLength++] = COMMAND_CLASS_CONFIGURATION_V4;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = CONFIGURATION_BULK_SET_V4;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = (uint8_t)((parameter_offset>>8)&0xFF); // Parameter offset MSB
  p_chi->frame.as_byte_array[p_chi->frameLength++] = (uint8_t)(parameter_offset&0xFF);      // Parameter offset LSB
  p_chi->frame.as_byte_array[p_chi->frameLength++] = number_of_parameters;                  // Number of parameters
  p_chi->frame.as_byte_array[p_chi->frameLength++] = (~(1<<7) & // Default flag
                                                     (0<<6)) | // Handshake
                                                       2; // Size

  mock_call_use_as_stub(TO_STR(cc_configuration_io_write));
  mock_call_use_as_stub(TO_STR(cc_configuration_io_read));
  
  memset(&p_chi->rxOptions, 0, sizeof(p_chi->rxOptions));

  handler_return_value = CC_Configuration_handler(CC_Configuration_handler, p_chi);
  TEST_ASSERT_MESSAGE(RECEIVED_FRAME_STATUS_SUCCESS == handler_return_value, "Wrong frame status :(");

  test_common_command_handler_input_free(p_chi);
}

void test_cc_configuration_limit_value_unsigned_integer_size_not_specified(void)
{
  cc_config_parameter_metadata_t metadata = {
    .attributes.format              = CC_CONFIG_PARAMETER_FORMAT_UNSIGNED_INTEGER,
    .attributes.size                = CC_CONFIG_PARAMETER_SIZE_NOT_SPECIFIED,
    .attributes.min_value.as_int32  =  10,
    .attributes.max_value.as_int32  = 100,
  };
  cc_config_parameter_buffer_t parameter_buffer = {
    .metadata = &metadata
  };
  cc_config_parameter_value_t new_value = {
    .as_uint8 = 5
  };

  bool return_value = cc_configuration_limit_value(&parameter_buffer, &new_value);

  TEST_ASSERT_MESSAGE(true == return_value, "Wrong return value :(");
}

void test_cc_configuration_limit_value_unsigned_integer_uint8_min_limit(void)
{
  cc_config_parameter_metadata_t metadata = {
    .attributes.format              = CC_CONFIG_PARAMETER_FORMAT_UNSIGNED_INTEGER,
    .attributes.size                = CC_CONFIG_PARAMETER_SIZE_8_BIT,
    .attributes.min_value.as_int32  =  10,
    .attributes.max_value.as_int32  = 100,
  };
  cc_config_parameter_buffer_t parameter_buffer = {
    .metadata = &metadata
  };
  cc_config_parameter_value_t new_value = {
    .as_uint8 = 5
  };

  cc_configuration_limit_value(&parameter_buffer, &new_value);

  TEST_ASSERT_MESSAGE(new_value.as_uint8 == metadata.attributes.min_value.as_uint8, "Wrong return value :(");
}

void test_cc_configuration_limit_value_unsigned_integer_uint8_max_limit(void)
{
  cc_config_parameter_metadata_t metadata = {
    .attributes.format              = CC_CONFIG_PARAMETER_FORMAT_UNSIGNED_INTEGER,
    .attributes.size                = CC_CONFIG_PARAMETER_SIZE_8_BIT,
    .attributes.min_value.as_uint8  =  10,
    .attributes.max_value.as_uint8  = 100,
  };
  cc_config_parameter_buffer_t parameter_buffer = {
    .metadata = &metadata
  };
  cc_config_parameter_value_t new_value = {
    .as_uint8 = 150
  };

  cc_configuration_limit_value(&parameter_buffer, &new_value);

  TEST_ASSERT_MESSAGE(new_value.as_uint8 == metadata.attributes.max_value.as_uint8, "Wrong return value :(");
}

void test_cc_configuration_limit_value_unsigned_integer_uint8_in_range(void)
{
  cc_config_parameter_metadata_t metadata = {
    .attributes.format              = CC_CONFIG_PARAMETER_FORMAT_UNSIGNED_INTEGER,
    .attributes.size                = CC_CONFIG_PARAMETER_SIZE_8_BIT,
    .attributes.min_value.as_uint8  =  10,
    .attributes.max_value.as_uint8  = 100,
  };
  cc_config_parameter_buffer_t parameter_buffer = {
    .metadata = &metadata
  };
  cc_config_parameter_value_t new_value = {
    .as_uint8 = 20
  };

  cc_configuration_limit_value(&parameter_buffer, &new_value);

  TEST_ASSERT_MESSAGE(new_value.as_uint8 == 20, "Wrong return value :(");
}

void test_cc_configuration_limit_value_unsigned_integer_uint16_min_limit(void)
{
  cc_config_parameter_metadata_t metadata = {
    .attributes.format              = CC_CONFIG_PARAMETER_FORMAT_UNSIGNED_INTEGER,
    .attributes.size                = CC_CONFIG_PARAMETER_SIZE_16_BIT,
    .attributes.min_value.as_uint16  =  10,
    .attributes.max_value.as_uint16  = 100,
  };
  cc_config_parameter_buffer_t parameter_buffer = {
    .metadata = &metadata
  };
  cc_config_parameter_value_t new_value = {
    .as_uint16 = 5
  };

  cc_configuration_limit_value(&parameter_buffer, &new_value);

  TEST_ASSERT_MESSAGE(new_value.as_uint16 == metadata.attributes.min_value.as_uint16, "Wrong return value :(");
}

void test_cc_configuration_limit_value_unsigned_integer_uint16_max_limit(void)
{
  cc_config_parameter_metadata_t metadata = {
    .attributes.format              = CC_CONFIG_PARAMETER_FORMAT_UNSIGNED_INTEGER,
    .attributes.size                = CC_CONFIG_PARAMETER_SIZE_16_BIT,
    .attributes.min_value.as_uint16  =  10,
    .attributes.max_value.as_uint16  = 100,
  };
  cc_config_parameter_buffer_t parameter_buffer = {
    .metadata = &metadata
  };
  cc_config_parameter_value_t new_value = {
    .as_uint16 = 150
  };

  cc_configuration_limit_value(&parameter_buffer, &new_value);

  TEST_ASSERT_MESSAGE(new_value.as_uint16 == metadata.attributes.max_value.as_uint16, "Wrong return value :(");
}

void test_cc_configuration_limit_value_unsigned_integer_uint16_in_range(void)
{
  cc_config_parameter_metadata_t metadata = {
    .attributes.format              = CC_CONFIG_PARAMETER_FORMAT_UNSIGNED_INTEGER,
    .attributes.size                = CC_CONFIG_PARAMETER_SIZE_16_BIT,
    .attributes.min_value.as_uint16  =  10,
    .attributes.max_value.as_uint16  = 100,
  };
  cc_config_parameter_buffer_t parameter_buffer = {
    .metadata = &metadata
  };
  cc_config_parameter_value_t new_value = {
    .as_uint16 = 20
  };

  cc_configuration_limit_value(&parameter_buffer, &new_value);

  TEST_ASSERT_MESSAGE(new_value.as_uint16 == 20, "Wrong return value :(");
}

void test_cc_configuration_limit_value_unsigned_integer_uint32_min_limit(void)
{
  cc_config_parameter_metadata_t metadata = {
    .attributes.format              = CC_CONFIG_PARAMETER_FORMAT_UNSIGNED_INTEGER,
    .attributes.size                = CC_CONFIG_PARAMETER_SIZE_32_BIT,
    .attributes.min_value.as_int32  =  10,
    .attributes.max_value.as_int32  = 100,
  };
  cc_config_parameter_buffer_t parameter_buffer = {
    .metadata = &metadata
  };
  cc_config_parameter_value_t new_value = {
    .as_uint32 = 5
  };

  cc_configuration_limit_value(&parameter_buffer, &new_value);

  TEST_ASSERT_MESSAGE(new_value.as_uint32 == metadata.attributes.min_value.as_uint32, "Wrong return value :(");
}

void test_cc_configuration_limit_value_unsigned_integer_uint32_max_limit(void)
{
  cc_config_parameter_metadata_t metadata = {
    .attributes.format              = CC_CONFIG_PARAMETER_FORMAT_UNSIGNED_INTEGER,
    .attributes.size                = CC_CONFIG_PARAMETER_SIZE_32_BIT,
    .attributes.min_value.as_uint32  =  10,
    .attributes.max_value.as_uint32  = 100,
  };
  cc_config_parameter_buffer_t parameter_buffer = {
    .metadata = &metadata
  };
  cc_config_parameter_value_t new_value = {
    .as_uint32 = 150
  };

  cc_configuration_limit_value(&parameter_buffer, &new_value);

  TEST_ASSERT_MESSAGE(new_value.as_uint32 == metadata.attributes.max_value.as_uint32, "Wrong return value :(");
}

void test_cc_configuration_limit_value_unsigned_integer_uint32_in_range(void)
{
  cc_config_parameter_metadata_t metadata = {
    .attributes.format              = CC_CONFIG_PARAMETER_FORMAT_UNSIGNED_INTEGER,
    .attributes.size                = CC_CONFIG_PARAMETER_SIZE_32_BIT,
    .attributes.min_value.as_uint32  =  10,
    .attributes.max_value.as_uint32  = 100,
  };
  cc_config_parameter_buffer_t parameter_buffer = {
    .metadata = &metadata
  };
  cc_config_parameter_value_t new_value = {
    .as_uint32 = 20
  };

  cc_configuration_limit_value(&parameter_buffer, &new_value);

  TEST_ASSERT_MESSAGE(new_value.as_uint32 == 20, "Wrong return value :(");
}

void test_cc_configuration_limit_value_signed_integer_int8_min_limit(void)
{
  cc_config_parameter_metadata_t metadata = {
    .attributes.format              = CC_CONFIG_PARAMETER_FORMAT_SIGNED_INTEGER,
    .attributes.size                = CC_CONFIG_PARAMETER_SIZE_8_BIT,
    .attributes.min_value.as_int8  =  -15,
    .attributes.max_value.as_int8  =   15,
  };
  cc_config_parameter_buffer_t parameter_buffer = {
    .metadata = &metadata
  };
  cc_config_parameter_value_t new_value = {
    .as_int8 = -20
  };

  cc_configuration_limit_value(&parameter_buffer, &new_value);

  TEST_ASSERT_MESSAGE(new_value.as_int8 == metadata.attributes.min_value.as_int8, "Wrong return value :(");
}

void test_cc_configuration_limit_value_signed_integer_int8_max_limit(void)
{
  cc_config_parameter_metadata_t metadata = {
    .attributes.format              = CC_CONFIG_PARAMETER_FORMAT_SIGNED_INTEGER,
    .attributes.size                = CC_CONFIG_PARAMETER_SIZE_8_BIT,
    .attributes.min_value.as_int8  =  -15,
    .attributes.max_value.as_int8  =   15,
  };
  cc_config_parameter_buffer_t parameter_buffer = {
    .metadata = &metadata
  };
  cc_config_parameter_value_t new_value = {
    .as_int8 = 20
  };

  cc_configuration_limit_value(&parameter_buffer, &new_value);

  TEST_ASSERT_MESSAGE(new_value.as_int8 == metadata.attributes.max_value.as_int8, "Wrong return value :(");
}

void test_cc_configuration_limit_value_signed_integer_int8_in_range(void)
{
  cc_config_parameter_metadata_t metadata = {
    .attributes.format              = CC_CONFIG_PARAMETER_FORMAT_SIGNED_INTEGER,
    .attributes.size                = CC_CONFIG_PARAMETER_SIZE_8_BIT,
    .attributes.min_value.as_int8  =  -15,
    .attributes.max_value.as_int8  =   15,
  };
  cc_config_parameter_buffer_t parameter_buffer = {
    .metadata = &metadata
  };
  cc_config_parameter_value_t new_value = {
    .as_int8 = 5
  };

  cc_configuration_limit_value(&parameter_buffer, &new_value);

  TEST_ASSERT_MESSAGE(new_value.as_int8 == 5, "Wrong return value :(");
}

void test_cc_configuration_limit_value_signed_integer_int16_min_limit(void)
{
  cc_config_parameter_metadata_t metadata = {
    .attributes.format              = CC_CONFIG_PARAMETER_FORMAT_SIGNED_INTEGER,
    .attributes.size                = CC_CONFIG_PARAMETER_SIZE_16_BIT,
    .attributes.min_value.as_int16  =  -100,
    .attributes.max_value.as_int16  =   100,
  };
  cc_config_parameter_buffer_t parameter_buffer = {
    .metadata = &metadata
  };
  cc_config_parameter_value_t new_value = {
    .as_int16 = -150
  };

  cc_configuration_limit_value(&parameter_buffer, &new_value);

  TEST_ASSERT_MESSAGE(new_value.as_int16 ==  metadata.attributes.min_value.as_int16, "Wrong return value :(");
}

void test_cc_configuration_limit_value_signed_integer_int16_max_limit(void)
{
  cc_config_parameter_metadata_t metadata = {
    .attributes.format              = CC_CONFIG_PARAMETER_FORMAT_SIGNED_INTEGER,
    .attributes.size                = CC_CONFIG_PARAMETER_SIZE_16_BIT,
    .attributes.min_value.as_int16  =  -100,
    .attributes.max_value.as_int16  =   100,
  };
  cc_config_parameter_buffer_t parameter_buffer = {
    .metadata = &metadata
  };
  cc_config_parameter_value_t new_value = {
    .as_int16 = 150
  };

  cc_configuration_limit_value(&parameter_buffer, &new_value);

  TEST_ASSERT_MESSAGE(new_value.as_int16 == metadata.attributes.max_value.as_int16, "Wrong return value :(");
}

void test_cc_configuration_limit_value_signed_integer_int16_in_range(void)
{
  cc_config_parameter_metadata_t metadata = {
    .attributes.format              = CC_CONFIG_PARAMETER_FORMAT_SIGNED_INTEGER,
    .attributes.size                = CC_CONFIG_PARAMETER_SIZE_16_BIT,
    .attributes.min_value.as_int16  =  -100,
    .attributes.max_value.as_int16  =   100,
  };
  cc_config_parameter_buffer_t parameter_buffer = {
    .metadata = &metadata
  };
  cc_config_parameter_value_t new_value = {
    .as_int16 = -50
  };

  cc_configuration_limit_value(&parameter_buffer, &new_value);

  TEST_ASSERT_MESSAGE(new_value.as_int16 == -50, "Wrong return value :(");
}

void test_cc_configuration_limit_value_signed_integer_int32_min_limit(void)
{
  cc_config_parameter_metadata_t metadata = {
    .attributes.format              = CC_CONFIG_PARAMETER_FORMAT_SIGNED_INTEGER,
    .attributes.size                = CC_CONFIG_PARAMETER_SIZE_32_BIT,
    .attributes.min_value.as_int32  =  -100,
    .attributes.max_value.as_int32  =   100,
  };
  cc_config_parameter_buffer_t parameter_buffer = {
    .metadata = &metadata
  };
  cc_config_parameter_value_t new_value = {
    .as_int32 = -150
  };

  cc_configuration_limit_value(&parameter_buffer, &new_value);

  TEST_ASSERT_MESSAGE(new_value.as_int32 == metadata.attributes.min_value.as_int32, "Wrong return value :(");
}

void test_cc_configuration_limit_value_signed_integer_int32_max_limit(void)
{
  cc_config_parameter_metadata_t metadata = {
    .attributes.format              = CC_CONFIG_PARAMETER_FORMAT_SIGNED_INTEGER,
    .attributes.size                = CC_CONFIG_PARAMETER_SIZE_32_BIT,
    .attributes.min_value.as_int32  =  -100,
    .attributes.max_value.as_int32  =   100,
  };
  cc_config_parameter_buffer_t parameter_buffer = {
    .metadata = &metadata
  };
  cc_config_parameter_value_t new_value = {
    .as_int32 = 150
  };

  cc_configuration_limit_value(&parameter_buffer, &new_value);

  TEST_ASSERT_MESSAGE(new_value.as_int32 == metadata.attributes.max_value.as_int32, "Wrong return value :(");
}

void test_cc_configuration_limit_value_signed_integer_int32_in_range(void)
{
  cc_config_parameter_metadata_t metadata = {
    .attributes.format              = CC_CONFIG_PARAMETER_FORMAT_SIGNED_INTEGER,
    .attributes.size                = CC_CONFIG_PARAMETER_SIZE_32_BIT,
    .attributes.min_value.as_int32  =  -100,
    .attributes.max_value.as_int32  =   100,
  };
  cc_config_parameter_buffer_t parameter_buffer = {
    .metadata = &metadata
  };
  cc_config_parameter_value_t new_value = {
    .as_int32 = 50
  };

  cc_configuration_limit_value(&parameter_buffer, &new_value);

  TEST_ASSERT_MESSAGE(new_value.as_int32 == 50, "Wrong return value :(");
}

void test_cc_configuration_limit_value_signed_integer_int32_invalid_size(void)
{
  cc_config_parameter_metadata_t metadata = {
    .attributes.format              = CC_CONFIG_PARAMETER_FORMAT_SIGNED_INTEGER,
    .attributes.size                = CC_CONFIG_PARAMETER_SIZE_NOT_SPECIFIED,
    .attributes.min_value.as_int32  =  -100,
    .attributes.max_value.as_int32  =   100,
  };
  cc_config_parameter_buffer_t parameter_buffer = {
    .metadata = &metadata
  };
  cc_config_parameter_value_t new_value = {
    .as_int32 = 50
  };

  cc_configuration_limit_value(&parameter_buffer, &new_value);

  TEST_ASSERT_MESSAGE(new_value.as_int32 == 50, "Wrong return value :(");
}

void test_cc_configuration_limit_value_signed_integer_invalid_format(void)
{
  cc_config_parameter_metadata_t metadata = {
    .attributes.format = 0xFF, /*Intentional wrong value*/
  };
  cc_config_parameter_buffer_t parameter_buffer = {
    .metadata = &metadata
  };
  cc_config_parameter_value_t new_value = {
    .as_int32 = 50
  };

  cc_configuration_limit_value(&parameter_buffer, &new_value);

  TEST_ASSERT_MESSAGE(new_value.as_int32 == 50, "Wrong return value :(");
}

void test_cc_configuration_is_valid_size_not_specified(void)
{
  received_frame_status_t handler_return_value;
  command_handler_input_t* p_chi = test_common_command_handler_input_allocate();
  uint8_t invalid_size_parameter = CC_CONFIG_PARAMETER_SIZE_NOT_SPECIFIED; //invalid size

  cc_configuration_fill_fake_files_with_dummy_data();

  p_chi->frame.as_byte_array[p_chi->frameLength++] = COMMAND_CLASS_CONFIGURATION_V4;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = CONFIGURATION_SET_V4;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = 1;                             //Parameter number
  p_chi->frame.as_byte_array[p_chi->frameLength++] = 0x80 |                         // Default flag
                                                     invalid_size_parameter;        // Size
  p_chi->frame.as_byte_array[p_chi->frameLength++] = 0;                             //Configuration value
  p_chi->frame.as_byte_array[p_chi->frameLength++] = 0;
  
  handler_return_value = CC_Configuration_handler(CC_Configuration_handler, p_chi);
  TEST_ASSERT_MESSAGE(RECEIVED_FRAME_STATUS_FAIL == handler_return_value, "Wrong frame status :(");

  test_common_command_handler_input_free(p_chi);
}

void test_cc_configuration_is_valid_size_32_bit_size(void)
{
  received_frame_status_t handler_return_value;
  command_handler_input_t* p_chi = test_common_command_handler_input_allocate();
  uint8_t valid_size_parameter = CC_CONFIG_PARAMETER_SIZE_32_BIT; //invalid size



  cc_configuration_fill_fake_files_with_dummy_data();

  p_chi->frame.as_byte_array[p_chi->frameLength++] = COMMAND_CLASS_CONFIGURATION_V4;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = CONFIGURATION_SET_V4;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = 1;                           //Parameter number
  p_chi->frame.as_byte_array[p_chi->frameLength++] = 0x80 |                       // Default flag
                                                     valid_size_parameter;        // Size
  p_chi->frame.as_byte_array[p_chi->frameLength++] = 0;                           //Configuration value
  p_chi->frame.as_byte_array[p_chi->frameLength++] = 0;

  mock_call_use_as_fake(TO_STR(cc_configuration_io_write));
  mock_call_use_as_fake(TO_STR(cc_configuration_io_read));

  handler_return_value = CC_Configuration_handler(CC_Configuration_handler, p_chi);
  TEST_ASSERT_MESSAGE(RECEIVED_FRAME_STATUS_SUCCESS == handler_return_value, "Wrong frame status :(");

  test_common_command_handler_input_free(p_chi);
}

void test_cc_configuration_is_valid_size_8_bit_size(void)
{
  received_frame_status_t handler_return_value;
  command_handler_input_t* p_chi = test_common_command_handler_input_allocate();
  uint8_t valid_size_parameter = CC_CONFIG_PARAMETER_SIZE_8_BIT; //invalid size

  cc_configuration_fill_fake_files_with_dummy_data();

  p_chi->frame.as_byte_array[p_chi->frameLength++] = COMMAND_CLASS_CONFIGURATION_V4;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = CONFIGURATION_SET_V4;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = 1;                           //Parameter number
  p_chi->frame.as_byte_array[p_chi->frameLength++] = 0x80 |                       // Default flag
                                                     valid_size_parameter;        // Size
  p_chi->frame.as_byte_array[p_chi->frameLength++] = 0;                           //Configuration value
  p_chi->frame.as_byte_array[p_chi->frameLength++] = 0;
  
  mock_call_use_as_fake(TO_STR(cc_configuration_io_write));

  handler_return_value = CC_Configuration_handler(CC_Configuration_handler, p_chi);
  TEST_ASSERT_MESSAGE(RECEIVED_FRAME_STATUS_SUCCESS == handler_return_value, "Wrong frame status :(");

  test_common_command_handler_input_free(p_chi);
}

void test_cc_configuration_is_valid_size_invalid_size_default_branch(void)
{
  received_frame_status_t handler_return_value;
  command_handler_input_t* p_chi = test_common_command_handler_input_allocate();
  uint8_t valid_size_parameter = 7; //invalid size

  cc_configuration_fill_fake_files_with_dummy_data();

  p_chi->frame.as_byte_array[p_chi->frameLength++] = COMMAND_CLASS_CONFIGURATION_V4;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = CONFIGURATION_SET_V4;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = 1;                          //Parameter number
  p_chi->frame.as_byte_array[p_chi->frameLength++] = 0x80 |                      // Default flag
                                                     valid_size_parameter;       // Size
  p_chi->frame.as_byte_array[p_chi->frameLength++] = 0;                          //Configuration value
  p_chi->frame.as_byte_array[p_chi->frameLength++] = 0;
  
  mock_call_use_as_fake(TO_STR(cc_configuration_io_write));

  handler_return_value = CC_Configuration_handler(CC_Configuration_handler, p_chi);
  TEST_ASSERT_MESSAGE(RECEIVED_FRAME_STATUS_FAIL == handler_return_value, "Wrong frame status :(");

  test_common_command_handler_input_free(p_chi);
}

void test_cc_configuration_command_default_reset_branch(void)
{
  received_frame_status_t handler_return_value;
  command_handler_input_t* p_chi = test_common_command_handler_input_allocate();

  p_chi->frame.as_byte_array[p_chi->frameLength++] = COMMAND_CLASS_CONFIGURATION_V4;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = CONFIGURATION_DEFAULT_RESET_V4;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = 1;            //Parameter number
  p_chi->frame.as_byte_array[p_chi->frameLength++] = 0x80 | 0x01;  // Default flag + size
  p_chi->frame.as_byte_array[p_chi->frameLength++] = 0;            //Configuration value
  p_chi->frame.as_byte_array[p_chi->frameLength++] = 0;

  mock_call_use_as_stub(TO_STR(cc_configuration_io_write));
  
  handler_return_value = CC_Configuration_handler(CC_Configuration_handler, p_chi);
  TEST_ASSERT_MESSAGE(RECEIVED_FRAME_STATUS_SUCCESS == handler_return_value, "Wrong frame status :(");

  test_common_command_handler_input_free(p_chi);
}

// -----------------------------------------------------------------------------
//              Static Function Definitions
// -----------------------------------------------------------------------------
static uint8_t *
cc_configuration_copyToFrame(  uint8_t * pFrame,
                                  cc_config_parameter_buffer_t* parameter_buffer,
                                  uint8_t const * pField)
{
  uint8_t i;
  for (i = 0; i < (parameter_buffer->metadata->attributes.size); i++)
  {
    *(pFrame + i) = *(pField + parameter_buffer->metadata->attributes.size - i - 1);
  }
  return pFrame + i;
}

static uint16_t
cc_configuration_calc_reports_to_follow(size_t data_length, size_t payload_limit)
{
  uint16_t reports_to_follow_count  = data_length / payload_limit;
  reports_to_follow_count += ((data_length % payload_limit) != 0)?1:0; /*Round up*/
  reports_to_follow_count-=1; /*If the data fits in payload it should be zero*/

  return reports_to_follow_count;
}
