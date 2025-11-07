#include <unity.h>
#include <test_common.h>
#include <ZW_classcmd.h>
#include "ZAF_CC_Invoker.h"
#include "ZAF_types.h"
#include <stdbool.h>
#include <string.h>
#include "ZAF_Common_interface_mock.h"
#include "zaf_config_api_mock.h"
#include "ZW_TransportEndpoint_mock.h"
#include "ZW_TransportSecProtocol_mock.h"
#include "multichannel_mock.h"
#include "cc_multi_channel_config_api_mock.h"
#include "SizeOf.h"
#include "zpal_radio_mock.h"
#include "zpal_radio_utils_mock.h"

void setUpSuite(void)
{
}

void tearDownSuite(void)
{
}

void setUp(void)
{
}

void tearDown(void)
{
}

// When CC_Basic is invoked, modify the Command Class of the incoming frame
static received_frame_status_t CC_Test_Handler(
  RECEIVE_OPTIONS_TYPE_EX *rxOpt, ZW_APPLICATION_TX_BUFFER *pcmd,
  uint8_t cmdLength, ZW_APPLICATION_TX_BUFFER *pFrameOut,
  uint8_t * pFrameOutLength)
{
  pcmd->ZW_Common.cmdClass = COMMAND_CLASS_SWITCH_BINARY_V2;
  return RECEIVED_FRAME_STATUS_SUCCESS;
}

REGISTER_CC_V2(COMMAND_CLASS_BASIC, BASIC_VERSION_V2, CC_Test_Handler);

static const cc_multi_channel_config_t endpoints[] = {
  {
    // Endpoint 1
    .generic_type  = GENERIC_TYPE_SWITCH_BINARY,
    .specific_type = SPECIFIC_TYPE_NOT_USED
  },
  {
    // Endpoint 2
    .generic_type  = GENERIC_TYPE_SWITCH_MULTILEVEL,
    .specific_type = SPECIFIC_TYPE_NOT_USED
  },
};

cc_multi_channel_config_t const * get_endpoint(uint8_t endpoint, __attribute__((unused)) int numCalls)
{
  ASSERT(endpoint <= sizeof_array(endpoints));
  if (endpoint == 0) {
    return &endpoints[0];
  } else {
    return &endpoints[endpoint - 1];
  }
}

/*
 * Make sure the Multi Channel handler doesn't alter the original incoming
 * frame's Command Class when it iterates through endpoints after receiving
 * a frame with a Bit Address destination
 */
void test_MULTI_CHANNEL_bit_address_basic_cc_incoming_frame_retention(void)
{
  // Assemble incoming frame
  command_handler_input_t chi;
  uint8_t endpointsBitMask = 0x01 | 0x02; // Endpoints 1 and 2

  ZW_MULTI_CHANNEL_CMD_ENCAP_V2_FRAME multiChannelFrame = {
    COMMAND_CLASS_MULTI_CHANNEL_V4,
    MULTI_CHANNEL_CMD_ENCAP_V4,
    0x00, // Source Endpoint
    MULTI_CHANNEL_CMD_ENCAP_PROPERTIES2_BIT_ADDRESS_BIT_MASK_V4
    | endpointsBitMask,
    { .ZW_BasicSetV2Frame = {
        COMMAND_CLASS_BASIC_V2,
        BASIC_SET_V2,
        0xFF // On
      } }
  };

  test_common_clear_command_handler_input(&chi);
  chi.frame.as_zw_application_tx_buffer.ZW_MultiChannelCmdEncapV2Frame
    = multiChannelFrame;

  // Set up mock calls
  zaf_cc_list_t cc_list_empty = { 0, NULL };
  ZW_APPLICATION_TX_BUFFER* pEncapFrame = (ZW_APPLICATION_TX_BUFFER*)
                                          &chi.frame.as_zw_application_tx_buffer
                                          .ZW_MultiChannelCmdEncapV2Frame
                                          .encapFrame.ZW_BasicSetV2Frame;

  for (uint8_t i = 1; i <= 2; ++i) {
    GetEndpointcmdClassList_ExpectAndReturn(false, i, &cc_list_empty);
  }
  GetEndpointcmdClassList_IgnoreAndReturn(NULL);

  for (uint8_t i = 0; i < 2; ++i) {
    ZAF_CC_MultiChannel_IsCCSupported_ExpectAndReturn(&chi.rxOptions,
                                                      pEncapFrame, true);
  }
  ZAF_CC_MultiChannel_IsCCSupported_IgnoreAndReturn(false);

  zaf_config_get_number_of_endpoints_ExpectAndReturn(2);
  Transport_ApplicationCommandHandlerEx_IgnoreAndReturn(
    RECEIVED_FRAME_STATUS_SUCCESS);

  ZW_APPLICATION_TX_BUFFER frameOut = { 0 };
  uint8_t lengthOut = 0;
  // Process Multi Channel encapsulation (without providing an outgoing frame)
  invoke_cc_handler_v2(&chi.rxOptions, &chi.frame.as_zw_application_tx_buffer,
                       chi.frameLength, &frameOut, &lengthOut);

  // Assert that the encapsulated Basic frame retained its command class
  TEST_ASSERT_MESSAGE(
    chi.frame.as_zw_application_tx_buffer.ZW_MultiChannelCmdEncapV2Frame
    .encapFrame.ZW_Common.cmdClass == COMMAND_CLASS_BASIC_V2,
    "The Command Class of the incoming frame was altered incorrectly."
    );
}

/*
 * Verifies that the Root End Point does not respond to Multi Channel Capability Get.
 */
void test_MULTI_CHANNEL_capability_report_root_end_point(void)
{
  // Create incoming frame
  command_handler_input_t input;
  test_common_clear_command_handler_input(&input);
  ZW_MULTI_CHANNEL_CAPABILITY_GET_V4_FRAME incomingFrame = {
    COMMAND_CLASS_MULTI_CHANNEL_V4,
    MULTI_CHANNEL_CAPABILITY_GET_V4,
    0x00 // Root End Point
  };
  input.frame.as_zw_application_tx_buffer.ZW_MultiChannelCapabilityGetV4Frame
    = incomingFrame;

  // Set up mock calls
  Check_not_legal_response_job_IgnoreAndReturn(false);
  zaf_config_get_number_of_endpoints_ExpectAndReturn(2);

  // Process Multi Channel Capability Get
  ZW_APPLICATION_TX_BUFFER output;
  uint8_t lengthOut = 0;
  received_frame_status_t status =
    invoke_cc_handler_v2(&input.rxOptions,
                         &input.frame.as_zw_application_tx_buffer,
                         input.frameLength, &output, &lengthOut);

  // Verify outgoing frame
  TEST_ASSERT_EQUAL_MESSAGE(
    RECEIVED_FRAME_STATUS_FAIL, status,
    "The Multi Channel Capability Get was answered for EP0."
    );
}

/*
 * Verifies that the EP1 responds to Multi Channel Capability Get.
 */
void test_MULTI_CHANNEL_capability_report_EP1(void)
{
  // Configure End Point
  const cc_multi_channel_config_t deviceType = {
    .generic_type = GENERIC_TYPE_SWITCH_BINARY,
    .specific_type = SPECIFIC_TYPE_NOT_USED
  };
  uint8_t commandClasses = COMMAND_CLASS_BASIC_V2;
  zaf_cc_list_t ccList = { .cc_list = &commandClasses, .list_size = 1 };

  // Create incoming frame
  command_handler_input_t input;
  test_common_clear_command_handler_input(&input);
  ZW_MULTI_CHANNEL_CAPABILITY_GET_V4_FRAME incomingFrame = {
    COMMAND_CLASS_MULTI_CHANNEL_V4,
    MULTI_CHANNEL_CAPABILITY_GET_V4,
    0x01 // EP1
  };
  input.frame.as_zw_application_tx_buffer.ZW_MultiChannelCapabilityGetV4Frame
    = incomingFrame;

  // Create expected outgoing frame
  ZW_APPLICATION_TX_BUFFER expectedOutput = {
    .ZW_MultiChannelCapabilityReport1byteV4Frame = {
      .cmdClass = COMMAND_CLASS_MULTI_CHANNEL_V4,
      .cmd = MULTI_CHANNEL_CAPABILITY_REPORT_V4,
      .genericDeviceClass = deviceType.generic_type,
      .specificDeviceClass = deviceType.specific_type,
      .properties1 = 0x01, //EP1
      .commandClass1 = COMMAND_CLASS_BASIC_V2
    }
  };

  // Set up mock calls
  Check_not_legal_response_job_IgnoreAndReturn(false);
  zaf_config_get_number_of_endpoints_ExpectAndReturn(2);
  cc_multi_channel_get_config_endpoint_ExpectAndReturn(1, &deviceType);
  ZAF_GetNodeID_ExpectAndReturn(1);
  GetCommandClassList_IgnoreAndReturn(&ccList);
  ZAF_GetNodeID_ExpectAndReturn(1);

  // Process Multi Channel Capability Get
  ZW_APPLICATION_TX_BUFFER output;
  uint8_t lengthOut = 0;
  received_frame_status_t status =
    invoke_cc_handler_v2(&input.rxOptions,
                         &input.frame.as_zw_application_tx_buffer,
                         input.frameLength, &output, &lengthOut);

  // Verify outgoing frame
  TEST_ASSERT_EQUAL_MESSAGE(
    RECEIVED_FRAME_STATUS_SUCCESS, status,
    "The Multi Channel Capability Get was not answered."
    );
  TEST_ASSERT_EQUAL_MEMORY_MESSAGE(
    &expectedOutput, &output,
    sizeof(ZW_MULTI_CHANNEL_CAPABILITY_REPORT_1BYTE_V4_FRAME),
    "The outgoing frame had unexpected contents."
    );
}

/*
 * Verifies that Multi Channel End Point Find Report contains the correct amount
 * of variant groups.
 */
void test_MULTI_CHANNEL_end_point_find_report_length(void)
{
  // Create incoming frame
  command_handler_input_t input;
  test_common_clear_command_handler_input(&input);
  ZW_MULTI_CHANNEL_END_POINT_FIND_V4_FRAME incomingFrame = {
    COMMAND_CLASS_MULTI_CHANNEL_V4,
    MULTI_CHANNEL_END_POINT_FIND_V4,
    GENERIC_TYPE_SWITCH_MULTILEVEL,
    SPECIFIC_TYPE_NOT_USED
  };
  input.frame.as_zw_application_tx_buffer.ZW_MultiChannelEndPointFindV4Frame
    = incomingFrame;

  // Create expected outgoing frame
  ZW_APPLICATION_TX_BUFFER expectedOutput = {
    .ZW_MultiChannelEndPointFindReport4byteV2Frame = {
      .cmdClass = COMMAND_CLASS_MULTI_CHANNEL_V4,
      .cmd = MULTI_CHANNEL_END_POINT_FIND_REPORT_V4,
      .genericDeviceClass = GENERIC_TYPE_SWITCH_MULTILEVEL,
      .specificDeviceClass = SPECIFIC_TYPE_NOT_USED,
      .reportsToFollow = 0x00,
      .variantgroup1 = { .properties1 = 0x02 },
      .variantgroup2 = { .properties1 = 0x00 },
      .variantgroup3 = { .properties1 = 0x00 },
      .variantgroup4 = { .properties1 = 0x00 }
    }
  };

  // Set up mock calls
  Check_not_legal_response_job_IgnoreAndReturn(false);
  zaf_config_get_number_of_endpoints_ExpectAndReturn(2);
  cc_multi_channel_get_config_endpoint_Stub(&get_endpoint);

  // Process Multi Channel End Point Find
  ZW_APPLICATION_TX_BUFFER output;
  memset(&output, 0x00,
         sizeof(ZW_MULTI_CHANNEL_END_POINT_FIND_REPORT_4BYTE_V4_FRAME));
  uint8_t lengthOut = 0;
  invoke_cc_handler_v2(&input.rxOptions,
                       &input.frame.as_zw_application_tx_buffer,
                       input.frameLength, &output, &lengthOut);

  // Verify outgoing frame
  TEST_ASSERT_EQUAL_MESSAGE(
    sizeof(ZW_MULTI_CHANNEL_END_POINT_FIND_REPORT_1BYTE_V4_FRAME), lengthOut,
    "The outgoing frame's size was not correct."
    );
  TEST_ASSERT_EQUAL_MEMORY_MESSAGE(
    &expectedOutput, &output,
    sizeof(ZW_MULTI_CHANNEL_END_POINT_FIND_REPORT_4BYTE_V4_FRAME),
    "The outgoing frame had unexpected contents."
    );
}
