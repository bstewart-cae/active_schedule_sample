/**
 * @file test_CC_MultiChan.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include "ZW_TransportEndpoint.h"
#include <test_common.h>
#include "mock_control.h"
#include <string.h>
#include "ZAF_CC_Invoker.h"
#include "cc_multi_channel_config_api_mock.h"
#include "zaf_config_api_mock.h"
#include "SizeOf.h"
#include "ZW_TransportEndpoint_mock.h"
#include "zpal_radio_mock.h"
#include "zpal_radio_utils_mock.h"

void setUpSuite(void) {

}

void tearDownSuite(void) {

}

void setUp(void) {

}

void tearDown(void) {

}

#define MultiChanCommandHandler(a,b,c) invoke_cc_handler_v2(a,b,c,NULL,NULL)

void test_MULTI_CHANNEL_CAPABILITY_GET_V4(void)
{
  mock_t * pMock;
  mock_calls_clear();

  RECEIVE_OPTIONS_TYPE_EX rxOpt;
  ZW_APPLICATION_TX_BUFFER frame;

  const uint8_t ENDPOINT = 0x01;
  const uint8_t GENERIC_DEVICE_CLASS  = 0xAA;
  const uint8_t SPECIFIC_DEVICE_CLASS = 0xBB;

  cc_multi_channel_config_t config[] = {
                                        {
                                         .generic_type = GENERIC_DEVICE_CLASS,
                                         .specific_type = SPECIFIC_DEVICE_CLASS
                                        }
  };

  Check_not_legal_response_job_ExpectAndReturn(&rxOpt, false);

  zaf_config_get_number_of_endpoints_ExpectAndReturn(sizeof_array(config));

  ZW_MULTI_CHANNEL_CAPABILITY_GET_V4_FRAME * pCmd = (ZW_MULTI_CHANNEL_CAPABILITY_GET_V4_FRAME *)&frame;
  pCmd->cmdClass    = COMMAND_CLASS_MULTI_CHANNEL_V4;
  pCmd->cmd         = MULTI_CHANNEL_CAPABILITY_GET_V4;
  pCmd->properties1 = ENDPOINT;

  cc_multi_channel_get_config_endpoint_ExpectAndReturn(ENDPOINT, config);

  mock_call_expect(TO_STR(ZAF_GetNodeID), &pMock);
  pMock->return_code.v = 1;

  mock_call_expect(TO_STR(GetCommandClassList), &pMock);
  pMock->expect_arg[0].v = false;
  pMock->expect_arg[1].v = SECURITY_KEY_NONE;
  pMock->expect_arg[1].v = ENDPOINT;

  const uint8_t LIST_SIZE              = 0x04;
#define CC1 (0x11)
#define CC2 (0x22)
#define CC3 (0x33)
#define CC4 (0x44)
  uint8_t CC_LIST[] = {CC1, CC2, CC3, CC4};
  zaf_cc_list_t cmd_class_list = {
    .cc_list = &CC_LIST[0],
    .list_size = LIST_SIZE
  };
  pMock->return_code.pointer = &cmd_class_list;

  uint8_t expected_frame[] = {
      0x60,
      0x0A,
      0x01,
      GENERIC_DEVICE_CLASS,
      SPECIFIC_DEVICE_CLASS,
      CC1,
      CC2,
      CC3,
      CC4
  };

  mock_call_expect(TO_STR(ZAF_GetNodeID), &pMock);
  pMock->return_code.v = 1;

  received_frame_status_t status;
  ZW_APPLICATION_TX_BUFFER frame_out;
  uint8_t frame_out_length = 0;

  status = invoke_cc_handler_v2(&rxOpt, (ZW_APPLICATION_TX_BUFFER*)&frame, sizeof(frame), &frame_out, &frame_out_length);

  TEST_ASSERT_EQUAL_MESSAGE(RECEIVED_FRAME_STATUS_SUCCESS, status, "");
  TEST_ASSERT_EQUAL_UINT8(sizeof(expected_frame), frame_out_length);
  TEST_ASSERT_EQUAL_UINT8_ARRAY(&expected_frame, (uint8_t *)&frame_out, frame_out_length);

  mock_calls_verify();
}

/*
 * Verifies that MULTI_CHANNEL_CAPABILITY_GET is ignored if an invalid endpoint is received.
 */
void test_MULTI_CHANNEL_CAPABILITY_GET_invalid_endpoint(void)
{
  RECEIVE_OPTIONS_TYPE_EX rxOpt = {0};
  ZW_APPLICATION_TX_BUFFER frame;

  const uint8_t ENDPOINT = 10;

  ZW_MULTI_CHANNEL_CAPABILITY_GET_V4_FRAME * pCmd = (ZW_MULTI_CHANNEL_CAPABILITY_GET_V4_FRAME *)&frame;
  pCmd->cmdClass    = COMMAND_CLASS_MULTI_CHANNEL_V4;
  pCmd->cmd         = MULTI_CHANNEL_CAPABILITY_GET_V4;
  pCmd->properties1 = ENDPOINT;

  Check_not_legal_response_job_ExpectAndReturn(&rxOpt, false);

  zaf_config_get_number_of_endpoints_ExpectAndReturn(9);

  received_frame_status_t status;
  uint8_t frame_out_length = 0;

  status = invoke_cc_handler_v2(&rxOpt, &frame, 10, NULL, &frame_out_length);

  TEST_ASSERT_EQUAL_MESSAGE(RECEIVED_FRAME_STATUS_FAIL, status, "");

  TEST_ASSERT_EQUAL_UINT8(0, frame_out_length);
}

/*
 * Verifies that every possible value of "Aggregated End Point" will result in a response with
 * "Number of bit masks" set to zero as aggregated endpoints are deprecated.
 */
void test_MULTI_CHANNEL_AGGREGATED_MEMBERS_GET_V4(void)
{
  RECEIVE_OPTIONS_TYPE_EX rxOpt = {0};

  ZW_MULTI_CHANNEL_AGGREGATED_MEMBERS_REPORT_1BYTE_V4_FRAME expected_frame;
  expected_frame.cmdClass         = COMMAND_CLASS_MULTI_CHANNEL_V4;
  expected_frame.cmd              = MULTI_CHANNEL_AGGREGATED_MEMBERS_REPORT_V4;
  expected_frame.properties1      = 0; // This field will be updated for every iteration.
  expected_frame.numberOfBitMasks = 0; // Expected 0 in every iteration.

  ZW_MULTI_CHANNEL_AGGREGATED_MEMBERS_GET_V4_FRAME frame;
  frame.cmdClass    = COMMAND_CLASS_MULTI_CHANNEL_V4;
  frame.cmd         = MULTI_CHANNEL_AGGREGATED_MEMBERS_GET_V4;

  ZW_APPLICATION_TX_BUFFER frame_out;
  uint8_t frame_out_length = 0;
  const uint8_t EXPECTED_FRAME_LENGTH = sizeof(expected_frame) - 1; // No bitmask

  // Loop through all possible values of "Aggregated End Point".
  for (uint32_t i = 0; i < 0x7F; i++)
  {
    Check_not_legal_response_job_ExpectAndReturn(&rxOpt, false);

    frame.properties1          = i;
    expected_frame.properties1 = i;

    invoke_cc_handler_v2(&rxOpt, (ZW_APPLICATION_TX_BUFFER*)&frame, sizeof(frame), &frame_out, &frame_out_length);

    TEST_ASSERT_EQUAL_UINT8(EXPECTED_FRAME_LENGTH, frame_out_length);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(&expected_frame, (uint8_t *)&frame_out, frame_out_length);
  }
}

/*
 * Verifies that all endpoints are returned if 0xFF is given generic and specific types.
 */
void test_MULTI_CHANNEL_END_POINT_FIND_V4_wildcard(void)
{
  Check_not_legal_response_job_IgnoreAndReturn(false);

  const cc_multi_channel_config_t config[] = {
                                        {
                                         .generic_type = 0xAA,
                                         .specific_type = 0xBB
                                        },
                                        {
                                         .generic_type = 0xCC,
                                         .specific_type = 0xDD
                                        },
  };

  zaf_config_get_number_of_endpoints_ExpectAndReturn(sizeof_array(config));

  RECEIVE_OPTIONS_TYPE_EX rxOpt = {0};

  ZW_MULTI_CHANNEL_END_POINT_FIND_V4_FRAME frame;
  frame.cmdClass              = COMMAND_CLASS_MULTI_CHANNEL_V4;
  frame.cmd                   = MULTI_CHANNEL_END_POINT_FIND_V4;
  frame.genericDeviceClass    = 0xFF;
  frame.specificDeviceClass   = 0xFF;

  ZW_APPLICATION_TX_BUFFER frame_out;
  uint8_t frame_out_length = 0;

  ZW_MULTI_CHANNEL_END_POINT_FIND_REPORT_2BYTE_V4_FRAME expected_frame;
  expected_frame.cmdClass             = COMMAND_CLASS_MULTI_CHANNEL_V4;
  expected_frame.cmd                  = MULTI_CHANNEL_END_POINT_FIND_REPORT_V4;
  expected_frame.reportsToFollow      = 0;
  expected_frame.genericDeviceClass   = 0xFF;
  expected_frame.specificDeviceClass  = 0xFF;
  expected_frame.variantgroup1.properties1 = 1; // Endpoint 1
  expected_frame.variantgroup2.properties1 = 2; // Endpoint 2

  cc_multi_channel_get_config_endpoint_ExpectAndReturn(1, &config[0]);
  cc_multi_channel_get_config_endpoint_ExpectAndReturn(2, &config[1]);

  invoke_cc_handler_v2(&rxOpt, (ZW_APPLICATION_TX_BUFFER*)&frame, sizeof(frame), &frame_out, &frame_out_length);

  TEST_ASSERT_EQUAL_UINT8(sizeof(expected_frame), frame_out_length);
  TEST_ASSERT_EQUAL_UINT8_ARRAY(&expected_frame, (uint8_t *)&frame_out, frame_out_length);
}

/*
 * Verifies that one "endpoint" of the value 0 is returned if there is no match to generic and
 * specific type.
 */
void test_MULTI_CHANNEL_END_POINT_FIND_V4_no_match(void)
{
  Check_not_legal_response_job_IgnoreAndReturn(false);

  const cc_multi_channel_config_t config[] = {
                                        {
                                         .generic_type = 0xAA,
                                         .specific_type = 0xBB
                                        },
                                        {
                                         .generic_type = 0xCC,
                                         .specific_type = 0xDD
                                        },
  };

  zaf_config_get_number_of_endpoints_ExpectAndReturn(sizeof_array(config));

  RECEIVE_OPTIONS_TYPE_EX rxOpt = {0};

  ZW_MULTI_CHANNEL_END_POINT_FIND_V4_FRAME frame;
  frame.cmdClass              = COMMAND_CLASS_MULTI_CHANNEL_V4;
  frame.cmd                   = MULTI_CHANNEL_END_POINT_FIND_V4;
  frame.genericDeviceClass    = 0xAB;
  frame.specificDeviceClass   = 0xCD;

  ZW_APPLICATION_TX_BUFFER frame_out;
  uint8_t frame_out_length = 0;

  ZW_MULTI_CHANNEL_END_POINT_FIND_REPORT_1BYTE_V4_FRAME expected_frame;
  expected_frame.cmdClass             = COMMAND_CLASS_MULTI_CHANNEL_V4;
  expected_frame.cmd                  = MULTI_CHANNEL_END_POINT_FIND_REPORT_V4;
  expected_frame.reportsToFollow      = 0;
  expected_frame.genericDeviceClass   = 0xAB;
  expected_frame.specificDeviceClass  = 0xCD;
  expected_frame.variantgroup1.properties1 = 0;

  cc_multi_channel_get_config_endpoint_ExpectAndReturn(1, &config[0]);
  cc_multi_channel_get_config_endpoint_ExpectAndReturn(2, &config[1]);

  invoke_cc_handler_v2(&rxOpt, (ZW_APPLICATION_TX_BUFFER*)&frame, sizeof(frame), &frame_out, &frame_out_length);

  TEST_ASSERT_EQUAL_UINT8_MESSAGE(sizeof(expected_frame), frame_out_length, "Frame length didn't match");
  TEST_ASSERT_EQUAL_UINT8_ARRAY(&expected_frame, (uint8_t *)&frame_out, frame_out_length);
}

/*
 * Verifies that Transport_ApplicationCommandHandlerEx() is invoked once with the right parameters
 * for a non-bit addressing destination endpoint.
 */
void test_MULTI_CHANNEL_CMD_ENCAP_V4(void)
{
  mock_calls_clear();

  const uint8_t SOURCE_ENDPOINT = 1;
  const uint8_t DESTINATION_ENDPOINT = 1;

  RECEIVE_OPTIONS_TYPE_EX rxOpt = {0};
  memset(&rxOpt, 0, sizeof(RECEIVE_OPTIONS_TYPE_EX));
  rxOpt.rxStatus = 0;
  rxOpt.securityKey = 0;
  rxOpt.sourceNode.nodeId = 1;
  rxOpt.sourceNode.endpoint = 0;
  rxOpt.sourceNode.res = 0;
  rxOpt.destNode.nodeId = 2;
  rxOpt.destNode.endpoint = 0;
  rxOpt.destNode.BitAddress = 0;
  rxOpt.bSupervisionActive = 0;
  rxOpt.sessionId = 0;
  rxOpt.statusUpdate = 0;

  uint8_t frame[255];
  memset(frame, 0, sizeof(frame));
  uint8_t frame_length = 0;
  frame[frame_length++] = COMMAND_CLASS_MULTI_CHANNEL_V4;
  frame[frame_length++] = MULTI_CHANNEL_CMD_ENCAP_V4;
  frame[frame_length++] = SOURCE_ENDPOINT;
  frame[frame_length++] = DESTINATION_ENDPOINT;
  frame[frame_length++] = COMMAND_CLASS_BASIC;
  frame[frame_length++] = BASIC_SET;
  frame[frame_length++] = 0xFF;

  uint8_t * p_basic_set_frame = &frame[4];

  zaf_config_get_number_of_endpoints_ExpectAndReturn(2);

  uint8_t command_classes[] = {
                               COMMAND_CLASS_BASIC
  };

  zaf_cc_list_t cc_list = {
                           .cc_list = command_classes,
                           .list_size = sizeof_array(command_classes)
  };

  GetEndpointcmdClassList_ExpectAndReturn(false, DESTINATION_ENDPOINT, &cc_list);
  GetEndpointcmdClassList_ExpectAndReturn(true, DESTINATION_ENDPOINT, &cc_list);

  RECEIVE_OPTIONS_TYPE_EX expected_receive_options;
  memset(&expected_receive_options, 0, sizeof(RECEIVE_OPTIONS_TYPE_EX));

  expected_receive_options = rxOpt;
  expected_receive_options.sourceNode.endpoint = SOURCE_ENDPOINT;
  expected_receive_options.destNode.endpoint = DESTINATION_ENDPOINT;

  /*
   * CC Basic is not linked so Transport_ApplicationCommandHandlerEx() will be invoked.
   */
  Transport_ApplicationCommandHandlerEx_ExpectWithArrayAndReturn((RECEIVE_OPTIONS_TYPE_EX *)&expected_receive_options,
                                                                 1, // Compare one element of RECEIVE_OPTIONS_TYPE_EX
                                                                 (ZW_APPLICATION_TX_BUFFER *)p_basic_set_frame,
                                                                 1,
                                                                 frame_length - 4,
                                                                 RECEIVED_FRAME_STATUS_SUCCESS);

  ZW_APPLICATION_TX_BUFFER frame_out = {0};
  uint8_t frame_out_length = 0;

  invoke_cc_handler_v2((RECEIVE_OPTIONS_TYPE_EX *)&rxOpt, (ZW_APPLICATION_TX_BUFFER *)&frame, frame_length, &frame_out, &frame_out_length);

  TEST_ASSERT_EQUAL_UINT8_MESSAGE(0, frame_out_length, "Expecting no response, but output length is higher than zero.");

  mock_calls_verify();
}

/*
 * Verifies that Transport_ApplicationCommandHandlerEx() is invoked the right number of times with
 * the right parameters for a bit addressing destination endpoint.
 *
 * It is also verified that no call is made when a bit is not set.
 */
void test_MULTI_CHANNEL_CMD_ENCAP_V4_bit_addressing(void)
{
  mock_calls_clear();

#define NUMBER_OF_ENDPOINTS_DEFINE 7
  const uint8_t NUMBER_OF_ENDPOINTS = NUMBER_OF_ENDPOINTS_DEFINE;

  const uint8_t SOURCE_ENDPOINT = 1;

  RECEIVE_OPTIONS_TYPE_EX rxOpt = {0};
  rxOpt.rxStatus = 0;
  rxOpt.securityKey = 0;
  rxOpt.sourceNode.nodeId = 1;
  rxOpt.sourceNode.endpoint = 0;
  rxOpt.sourceNode.res = 0;
  rxOpt.destNode.nodeId = 2;
  rxOpt.destNode.endpoint = 0;
  rxOpt.destNode.BitAddress = 0;
  rxOpt.bSupervisionActive = 0;
  rxOpt.sessionId = 0;
  rxOpt.statusUpdate = 0;

  uint8_t frame[255];
  memset(frame, 0, sizeof(frame));
  uint8_t frame_length = 0;
  frame[frame_length++] = COMMAND_CLASS_MULTI_CHANNEL_V4;
  frame[frame_length++] = MULTI_CHANNEL_CMD_ENCAP_V4;
  frame[frame_length++] = SOURCE_ENDPOINT;
  /*
   * Enable bit addressing by setting 0x80.
   *
   * Set all bits except 0x02 = endpoint 2
   */
  frame[frame_length++] = 0x80 | 0x40 | 0x20 | 0x10 | 0x08 | 0x04 | 0x01;
  frame[frame_length++] = COMMAND_CLASS_BASIC;
  frame[frame_length++] = BASIC_SET;
  frame[frame_length++] = 0xFF;

  uint8_t * p_basic_set_frame = &frame[4];

  zaf_config_get_number_of_endpoints_ExpectAndReturn(NUMBER_OF_ENDPOINTS);

  uint8_t command_classes[] = {
                               COMMAND_CLASS_BASIC
  };

  zaf_cc_list_t cc_list = {
                           .cc_list = command_classes,
                           .list_size = sizeof_array(command_classes)
  };

  RECEIVE_OPTIONS_TYPE_EX expected_receive_options[NUMBER_OF_ENDPOINTS_DEFINE];
  memset(expected_receive_options, 0, sizeof(expected_receive_options));

  for (uint8_t endpoint = 1; endpoint <= NUMBER_OF_ENDPOINTS; endpoint++)
  {
    // Skip expectations for endpoint 2 as it was not set in the bitmask.
    if (2 == endpoint) {
      continue;
    }
    uint8_t endpoint_index = endpoint - 1;
    GetEndpointcmdClassList_ExpectAndReturn(false, endpoint, &cc_list);
    GetEndpointcmdClassList_ExpectAndReturn(true, endpoint, &cc_list);

    expected_receive_options[endpoint_index] = rxOpt;
    expected_receive_options[endpoint_index].sourceNode.endpoint = SOURCE_ENDPOINT;
    expected_receive_options[endpoint_index].destNode.endpoint = endpoint;
    expected_receive_options[endpoint_index].destNode.BitAddress = 1;

    /*
     * CC Basic is not linked so Transport_ApplicationCommandHandlerEx() will be invoked.
     */
    Transport_ApplicationCommandHandlerEx_ExpectWithArrayAndReturn((RECEIVE_OPTIONS_TYPE_EX *)&expected_receive_options[endpoint_index],
                                                                   1, // Compare one element of RECEIVE_OPTIONS_TYPE_EX
                                                                   (ZW_APPLICATION_TX_BUFFER *)p_basic_set_frame,
                                                                   1,
                                                                   frame_length - 4,
                                                                   RECEIVED_FRAME_STATUS_SUCCESS);
  }

  ZW_APPLICATION_TX_BUFFER frame_out = {0};
  uint8_t frame_out_length = 0;

  invoke_cc_handler_v2((RECEIVE_OPTIONS_TYPE_EX *)&rxOpt, (ZW_APPLICATION_TX_BUFFER *)&frame, frame_length, &frame_out, &frame_out_length);

  TEST_ASSERT_EQUAL_UINT8_MESSAGE(0, frame_out_length, "Expecting no response, but output length is higher than zero.");

  mock_calls_verify();
}

/*
 * Verifies the limit of configured endpoints.
 */
void test_MULTI_CHANNEL_CMD_ENCAP_V4_bit_addressing_invalid_endpoint(void)
{
  mock_calls_clear();

#undef NUMBER_OF_ENDPOINTS_DEFINE
#define NUMBER_OF_ENDPOINTS_DEFINE 2
  const uint8_t NUMBER_OF_ENDPOINTS = NUMBER_OF_ENDPOINTS_DEFINE;

  const uint8_t SOURCE_ENDPOINT = 1;

  RECEIVE_OPTIONS_TYPE_EX rxOpt = {0};
  rxOpt.rxStatus = 0;
  rxOpt.securityKey = 0;
  rxOpt.sourceNode.nodeId = 1;
  rxOpt.sourceNode.endpoint = 0;
  rxOpt.sourceNode.res = 0;
  rxOpt.destNode.nodeId = 2;
  rxOpt.destNode.endpoint = 0;
  rxOpt.destNode.BitAddress = 0;
  rxOpt.bSupervisionActive = 0;
  rxOpt.sessionId = 0;
  rxOpt.statusUpdate = 0;

  uint8_t frame[255];
  memset(frame, 0, sizeof(frame));
  uint8_t frame_length = 0;
  frame[frame_length++] = COMMAND_CLASS_MULTI_CHANNEL_V4;
  frame[frame_length++] = MULTI_CHANNEL_CMD_ENCAP_V4;
  /*
   * Enable bit addressing by setting 0x80.
   *
   * Set bit 0-2 => endpoint 1-3.
   */
  frame[frame_length++] = SOURCE_ENDPOINT;
  frame[frame_length++] = 0x80 | 0x04 | 0x02 | 0x01;
  frame[frame_length++] = COMMAND_CLASS_BASIC;
  frame[frame_length++] = BASIC_SET;
  frame[frame_length++] = 0xFF;

  uint8_t * p_basic_set_frame = &frame[4];

  zaf_config_get_number_of_endpoints_ExpectAndReturn(NUMBER_OF_ENDPOINTS);

  uint8_t command_classes[] = {
                               COMMAND_CLASS_BASIC
  };

  zaf_cc_list_t cc_list = {
                           .cc_list = command_classes,
                           .list_size = sizeof_array(command_classes)
  };

  RECEIVE_OPTIONS_TYPE_EX expected_receive_options[NUMBER_OF_ENDPOINTS_DEFINE];
  memset(expected_receive_options, 0, sizeof(expected_receive_options));

  for (uint8_t endpoint = 1; endpoint <= NUMBER_OF_ENDPOINTS; endpoint++)
  {
    uint8_t endpoint_index = endpoint - 1;
    GetEndpointcmdClassList_ExpectAndReturn(false, endpoint, &cc_list);
    GetEndpointcmdClassList_ExpectAndReturn(true, endpoint, &cc_list);

    expected_receive_options[endpoint_index] = rxOpt;
    expected_receive_options[endpoint_index].sourceNode.endpoint = SOURCE_ENDPOINT;
    expected_receive_options[endpoint_index].destNode.endpoint = endpoint;
    expected_receive_options[endpoint_index].destNode.BitAddress = 1;

    /*
     * CC Basic is not linked so Transport_ApplicationCommandHandlerEx() will be invoked.
     */
    Transport_ApplicationCommandHandlerEx_ExpectWithArrayAndReturn((RECEIVE_OPTIONS_TYPE_EX *)&expected_receive_options[endpoint_index],
                                                                   1, // Compare one element of RECEIVE_OPTIONS_TYPE_EX
                                                                   (ZW_APPLICATION_TX_BUFFER *)p_basic_set_frame,
                                                                   1,
                                                                   frame_length - 4,
                                                                   RECEIVED_FRAME_STATUS_SUCCESS);
  }

  ZW_APPLICATION_TX_BUFFER frame_out = {0};
  uint8_t frame_out_length = 0;

  invoke_cc_handler_v2((RECEIVE_OPTIONS_TYPE_EX *)&rxOpt, (ZW_APPLICATION_TX_BUFFER *)&frame, frame_length, &frame_out, &frame_out_length);

  TEST_ASSERT_EQUAL_UINT8_MESSAGE(0, frame_out_length, "Expecting no response, but output length is higher than zero.");

  mock_calls_verify();
}
