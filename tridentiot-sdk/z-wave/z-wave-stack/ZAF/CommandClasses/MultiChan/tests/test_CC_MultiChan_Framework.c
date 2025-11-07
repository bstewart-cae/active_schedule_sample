/**
 * @file test_CC_MultiChan_Framework.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include <test_common.h>
#include <mock_control.h>
#include <ZW_TransportEndpoint.h>
#include <ZW_TransportSecProtocol.c>
#include <string.h>
#include <misc.h>
#include "ZAF_CC_Invoker.h"
#include <zaf_cc_list_generator_mock.h>
#include "cc_multi_channel_config_api_mock.h"
#include "zaf_config_api_mock.h"
#include "SizeOf.h"
#include "zpal_radio_mock.h"
#include "zpal_radio_utils_mock.h"

#define MultiChanCommandHandler(a,b,c) invoke_cc_handler_v2(a,b,c,NULL,NULL)

static uint8_t ep12_noSec_InclNonSecure[] =
{
  COMMAND_CLASS_SWITCH_BINARY,
  COMMAND_CLASS_ZWAVEPLUS_INFO,
  COMMAND_CLASS_ASSOCIATION,
  COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION_V2,
  COMMAND_CLASS_NOTIFICATION_V3,
  COMMAND_CLASS_SUPERVISION
};

static zaf_cc_list_t ep12_noSec_InclNonSecure_list = {
  .cc_list = ep12_noSec_InclNonSecure,
  .list_size = sizeof(ep12_noSec_InclNonSecure)
};

static zaf_cc_list_t *p_ep12_noSec_InclNonSecure_list = &ep12_noSec_InclNonSecure_list;

static uint8_t ep12_noSec_InclSecure[] =
{
    COMMAND_CLASS_ZWAVEPLUS_INFO,
    COMMAND_CLASS_SECURITY,
    COMMAND_CLASS_SECURITY_2
};


static zaf_cc_list_t ep12_noSec_InclSecure_list = {
  .cc_list = ep12_noSec_InclSecure,
  .list_size = sizeof(ep12_noSec_InclSecure)
};

static zaf_cc_list_t *p_ep12_noSec_InclSecure_list = &ep12_noSec_InclSecure_list;

static uint8_t ep12_sec_InclSecure[] =
{
  COMMAND_CLASS_SWITCH_BINARY,
  COMMAND_CLASS_ASSOCIATION,
  COMMAND_CLASS_ASSOCIATION_GRP_INFO,
  COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION_V2,
  COMMAND_CLASS_NOTIFICATION_V3,
  COMMAND_CLASS_SUPERVISION
};

static zaf_cc_list_t ep12_sec_InclSecure_list = {
  .cc_list = ep12_sec_InclSecure,
  .list_size = sizeof(ep12_sec_InclSecure)
};

static zaf_cc_list_t *p_ep12_sec_InclSecure_list = &ep12_sec_InclSecure_list;

static uint8_t ep3_noSec_InclNonSecure[] =
{
  COMMAND_CLASS_ZWAVEPLUS_INFO,
  COMMAND_CLASS_SWITCH_MULTILEVEL,
  COMMAND_CLASS_ASSOCIATION,
  COMMAND_CLASS_ASSOCIATION_GRP_INFO,
  COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION_V2,
  COMMAND_CLASS_NOTIFICATION_V3,
  COMMAND_CLASS_SUPERVISION,
  COMMAND_CLASS_SUPERVISION

};

static zaf_cc_list_t ep3_noSec_InclNonSecure_list = {
  .cc_list = ep3_noSec_InclNonSecure,
  .list_size = sizeof(ep3_noSec_InclNonSecure)
};

static zaf_cc_list_t *p_ep3_noSec_InclNonSecure_list = &ep3_noSec_InclNonSecure_list;

static uint8_t ep3_noSec_InclSecure[] =
{
    COMMAND_CLASS_SECURITY,
    COMMAND_CLASS_SECURITY_2
};

static zaf_cc_list_t ep3_noSec_InclSecure_list = {
  .cc_list = ep3_noSec_InclSecure,
  .list_size = sizeof(ep3_noSec_InclSecure)
};

static zaf_cc_list_t *p_ep3_noSec_InclSecure_list = &ep3_noSec_InclSecure_list;

static uint8_t ep3_sec_InclSecure[] =
{
    COMMAND_CLASS_SWITCH_MULTILEVEL,
    COMMAND_CLASS_ASSOCIATION,
    COMMAND_CLASS_ASSOCIATION_GRP_INFO,
    COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION_V2,
    COMMAND_CLASS_NOTIFICATION_V3,
    COMMAND_CLASS_SUPERVISION
};

static zaf_cc_list_t ep3_sec_InclSecure_list = {
  .cc_list = ep3_sec_InclSecure,
  .list_size = sizeof(ep3_sec_InclSecure)
};

static zaf_cc_list_t *p_ep3_sec_InclSecure_list = &ep3_sec_InclSecure_list;

cc_multi_channel_config_t cc_multi_channel_config[] = {
                                                       {
                                                        .generic_type = GENERIC_TYPE_SWITCH_BINARY,
                                                        .specific_type = SPECIFIC_TYPE_POWER_SWITCH_BINARY
                                                       },
                                                       {
                                                        .generic_type = GENERIC_TYPE_SWITCH_BINARY,
                                                        .specific_type = SPECIFIC_TYPE_POWER_SWITCH_BINARY
                                                       },
                                                       {
                                                        .generic_type = GENERIC_TYPE_SWITCH_MULTILEVEL,
                                                        .specific_type = SPECIFIC_TYPE_POWER_SWITCH_MULTILEVEL
                                                       }
};

void setUpSuite(void) {

}

void tearDownSuite(void) {

}

void setUp(void) {
  zaf_config_get_number_of_endpoints_IgnoreAndReturn(sizeof_array(cc_multi_channel_config));
}

void tearDown(void) {

}

static void PowerStrip_softwareInit(mock_t ** ppMock, uint8_t * pNnodeId)
{
  ZW_TransportEndpoint_Init();
}
void test_MULTI_CHANNEL_CAPABILITY_GET_V4_endpoint_1_incl(void)
{
  mock_t* pMock = NULL;
  mock_calls_clear();

  const uint8_t NODE_ID  = 2;
  const uint8_t ENDPOINT = 1;

  uint8_t requestedSecurityKeysBits = 0;
  RECEIVE_OPTIONS_TYPE_EX rxOpt;
  rxOpt.rxStatus = 0;
  rxOpt.securityKey = 0;
  rxOpt.sourceNode.nodeId = 1;
  rxOpt.sourceNode.endpoint = 0;
  rxOpt.sourceNode.res = 0;
  rxOpt.destNode.nodeId = NODE_ID;
  rxOpt.destNode.endpoint = 0;
  rxOpt.destNode.BitAddress = 0;

  mock_call_expect(TO_STR(ZAF_GetSecurityKeys), &pMock);
  pMock->return_code.v = requestedSecurityKeysBits;

  PowerStrip_softwareInit(&pMock, NULL);

  ZW_MULTI_CHANNEL_CAPABILITY_GET_V4_FRAME frame;
  frame.cmdClass    = COMMAND_CLASS_MULTI_CHANNEL_V4;
  frame.cmd         = MULTI_CHANNEL_CAPABILITY_GET_V4;
  frame.properties1 = ENDPOINT;

  zaf_config_get_number_of_endpoints_ExpectAndReturn(sizeof_array(cc_multi_channel_config));

  cc_multi_channel_get_config_endpoint_ExpectAndReturn(ENDPOINT, &cc_multi_channel_config[ENDPOINT-1]);

  static uint8_t expected_frame[] = {
      COMMAND_CLASS_MULTI_CHANNEL_V4,
      MULTI_CHANNEL_CAPABILITY_REPORT_V4,
      ENDPOINT,
      GENERIC_TYPE_SWITCH_BINARY,
      SPECIFIC_TYPE_POWER_SWITCH_BINARY,
      COMMAND_CLASS_SWITCH_BINARY,
      COMMAND_CLASS_ZWAVEPLUS_INFO,
      COMMAND_CLASS_ASSOCIATION,
      COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION_V2,
      COMMAND_CLASS_NOTIFICATION_V3,
      COMMAND_CLASS_SUPERVISION
  };

  mock_call_expect(TO_STR(ZAF_GetNodeID), &pMock);
  pMock->return_code.v = NODE_ID;

  mock_call_expect(TO_STR(ZAF_GetNodeID), &pMock);
  pMock->return_code.v = 1;

  zafi_cc_list_generator_get_lists_Expect(ENDPOINT, NULL, NULL, NULL);
  zafi_cc_list_generator_get_lists_IgnoreArg_unsecure_included_ccs(); // Used as output;
  zafi_cc_list_generator_get_lists_ReturnMemThruPtr_unsecure_included_ccs(&p_ep12_noSec_InclNonSecure_list, sizeof(p_ep12_noSec_InclNonSecure_list));
  zafi_cc_list_generator_get_lists_IgnoreArg_secure_included_unsecure_ccs(); // Used as output;
  zafi_cc_list_generator_get_lists_ReturnMemThruPtr_secure_included_unsecure_ccs(&p_ep12_noSec_InclSecure_list, sizeof(p_ep12_noSec_InclSecure_list));
  zafi_cc_list_generator_get_lists_IgnoreArg_secure_included_secure_ccs(); // Used as output;
  zafi_cc_list_generator_get_lists_ReturnMemThruPtr_secure_included_secure_ccs(&p_ep12_sec_InclSecure_list, sizeof(p_ep12_sec_InclSecure_list));

  mock_call_expect(TO_STR(ZAF_GetSecurityKeys), &pMock);
  pMock->return_code.v = requestedSecurityKeysBits;

  received_frame_status_t status;
  ZW_APPLICATION_TX_BUFFER frame_out;
  uint8_t frame_out_length = 0;

  status = invoke_cc_handler_v2(&rxOpt, (ZW_APPLICATION_TX_BUFFER*)&frame, sizeof(frame), &frame_out, &frame_out_length);

  TEST_ASSERT_EQUAL_MESSAGE(RECEIVED_FRAME_STATUS_SUCCESS, status, "");
  TEST_ASSERT_EQUAL_UINT8(sizeof(expected_frame), frame_out_length);
  TEST_ASSERT_EQUAL_UINT8_ARRAY(&expected_frame, (uint8_t *)&frame_out, frame_out_length);

  mock_calls_verify();
}


void test_MULTI_CHANNEL_CAPABILITY_GET_V4_endpoint_3_incl_SECURITY_KEY_S0_BIT(void)
{
  mock_t* pMock = NULL;
  mock_calls_clear();

  const uint8_t NODE_ID                       = 2;
  const uint8_t ENDPOINT                      = 3;
  const uint8_t REQUESTED_SECURITY_KEYS_BITS  = SECURITY_KEY_S0_BIT;

  RECEIVE_OPTIONS_TYPE_EX rxOpt;
  rxOpt.rxStatus = 0;
  rxOpt.securityKey = 0;
  rxOpt.sourceNode.nodeId = 1;
  rxOpt.sourceNode.endpoint = 0;
  rxOpt.sourceNode.res = 0;
  rxOpt.destNode.nodeId = NODE_ID;
  rxOpt.destNode.endpoint = 0;
  rxOpt.destNode.BitAddress = 0;
  
  mock_call_expect(TO_STR(ZAF_GetSecurityKeys), &pMock);
  pMock->return_code.v = REQUESTED_SECURITY_KEYS_BITS;

  PowerStrip_softwareInit(&pMock, NULL);

  ZW_MULTI_CHANNEL_CAPABILITY_GET_V4_FRAME frame;
  frame.cmdClass    = COMMAND_CLASS_MULTI_CHANNEL_V4;
  frame.cmd         = MULTI_CHANNEL_CAPABILITY_GET_V4;
  frame.properties1 = ENDPOINT;

  zaf_config_get_number_of_endpoints_ExpectAndReturn(sizeof_array(cc_multi_channel_config));

  cc_multi_channel_get_config_endpoint_ExpectAndReturn(ENDPOINT, &cc_multi_channel_config[ENDPOINT-1]);

  uint8_t expected_frame[] = {
      COMMAND_CLASS_MULTI_CHANNEL_V4,
      MULTI_CHANNEL_CAPABILITY_REPORT_V4,
      ENDPOINT,
      cc_multi_channel_config[ENDPOINT - 1].generic_type,
      cc_multi_channel_config[ENDPOINT - 1].specific_type,
      COMMAND_CLASS_SECURITY,
      COMMAND_CLASS_SECURITY_2
  };

  mock_call_expect(TO_STR(ZAF_GetNodeID), &pMock);
  pMock->return_code.v = NODE_ID;


  mock_call_expect(TO_STR(ZAF_GetNodeID), &pMock);
  pMock->return_code.v = 1;

  zafi_cc_list_generator_get_lists_Expect(ENDPOINT, NULL, NULL, NULL);
  zafi_cc_list_generator_get_lists_IgnoreArg_unsecure_included_ccs(); // Used as output;
  zafi_cc_list_generator_get_lists_ReturnMemThruPtr_unsecure_included_ccs(&p_ep3_noSec_InclNonSecure_list, sizeof(p_ep3_noSec_InclNonSecure_list));
  zafi_cc_list_generator_get_lists_IgnoreArg_secure_included_unsecure_ccs(); // Used as output;
  zafi_cc_list_generator_get_lists_ReturnMemThruPtr_secure_included_unsecure_ccs(&p_ep3_noSec_InclSecure_list, sizeof(p_ep3_noSec_InclSecure_list));
  zafi_cc_list_generator_get_lists_IgnoreArg_secure_included_secure_ccs(); // Used as output;
  zafi_cc_list_generator_get_lists_ReturnMemThruPtr_secure_included_secure_ccs(&p_ep3_sec_InclSecure_list, sizeof(p_ep3_sec_InclSecure_list));

  mock_call_expect(TO_STR(ZAF_GetSecurityKeys), &pMock);
  pMock->return_code.v = REQUESTED_SECURITY_KEYS_BITS;

  received_frame_status_t status;
  ZW_APPLICATION_TX_BUFFER frame_out;
  uint8_t frame_out_length = 0;

  status = invoke_cc_handler_v2(&rxOpt, (ZW_APPLICATION_TX_BUFFER*)&frame, sizeof(frame), &frame_out, &frame_out_length);

  TEST_ASSERT_EQUAL_MESSAGE(RECEIVED_FRAME_STATUS_SUCCESS, status, "");
  TEST_ASSERT_EQUAL_UINT8(sizeof(expected_frame), frame_out_length);
  TEST_ASSERT_EQUAL_UINT8_ARRAY_MESSAGE(&expected_frame, (uint8_t *)&frame_out, frame_out_length, "Frame didn't match");

  mock_calls_verify();
}


void test_MULTI_CHANNEL_END_POINT_GET_V4_endpoint_3_incl(void)
{
  mock_t* pMock = NULL;
  mock_calls_clear();

  RECEIVE_OPTIONS_TYPE_EX rxOpt = {0};

  PowerStrip_softwareInit(&pMock, NULL);

  ZW_MULTI_CHANNEL_END_POINT_GET_V4_FRAME frame;
  frame.cmdClass    = COMMAND_CLASS_MULTI_CHANNEL_V4;
  frame.cmd         = MULTI_CHANNEL_END_POINT_GET_V4;

  uint8_t expected_frame[] = {
      COMMAND_CLASS_MULTI_CHANNEL_V4,
      MULTI_CHANNEL_END_POINT_REPORT_V4,
      0, // No dynamic endpoints and endpoints are NOT identical.
      ZAF_CONFIG_NUMBER_OF_END_POINTS, // Number of individual endpoints according to the define in CMakeLists.txt.
      0 // Zero aggregated endpoints as they are deprecated.
  };

  cc_multi_channel_are_endpoints_identical_ExpectAndReturn(false);

  zaf_config_get_number_of_endpoints_ExpectAndReturn(sizeof_array(cc_multi_channel_config));

  received_frame_status_t status;
  ZW_APPLICATION_TX_BUFFER frame_out;
  uint8_t frame_out_length = 0;

  status = invoke_cc_handler_v2(&rxOpt, (ZW_APPLICATION_TX_BUFFER*)&frame, sizeof(frame), &frame_out, &frame_out_length);

  TEST_ASSERT_EQUAL_MESSAGE(RECEIVED_FRAME_STATUS_SUCCESS, status, "Frame status didn't match");
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(sizeof(expected_frame), frame_out_length, "Frame length didn't match");
  TEST_ASSERT_EQUAL_UINT8_ARRAY_MESSAGE(&expected_frame, (uint8_t *)&frame_out, frame_out_length, "Frame didn't match");

  mock_calls_verify();
}

void test_MULTI_CHANNEL_END_POINT_FIND_V4_GENERIC_FF_SPECIFIC_FF(void)
{
  mock_t* pMock = NULL;
  mock_calls_clear();

  RECEIVE_OPTIONS_TYPE_EX rxOpt;
  rxOpt.rxStatus = 0;
  rxOpt.securityKey = 0;
  rxOpt.sourceNode.nodeId = 1;
  rxOpt.sourceNode.endpoint = 0;
  rxOpt.sourceNode.res = 0;
  rxOpt.destNode.nodeId = 2;
  rxOpt.destNode.endpoint = 0;
  rxOpt.destNode.BitAddress = 0;
  
  uint8_t node_id = 1;
  PowerStrip_softwareInit(&pMock, &node_id);

  ZW_MULTI_CHANNEL_END_POINT_FIND_V4_FRAME frame;
  frame.cmdClass    = COMMAND_CLASS_MULTI_CHANNEL_V4;
  frame.cmd         = MULTI_CHANNEL_END_POINT_FIND_V4;
  frame.genericDeviceClass = 0xff;
  frame.specificDeviceClass = 0xff;

  uint8_t expected_frame[] = {
      COMMAND_CLASS_MULTI_CHANNEL_V4,
      MULTI_CHANNEL_END_POINT_FIND_REPORT_V4,
      0,
      frame.genericDeviceClass,
      frame.specificDeviceClass,
      1, 2, 3
  };

  zaf_config_get_number_of_endpoints_ExpectAndReturn(sizeof_array(cc_multi_channel_config));

  cc_multi_channel_get_config_endpoint_ExpectAndReturn(1, &cc_multi_channel_config[0]);
  cc_multi_channel_get_config_endpoint_ExpectAndReturn(2, &cc_multi_channel_config[1]);
  cc_multi_channel_get_config_endpoint_ExpectAndReturn(3, &cc_multi_channel_config[2]);

  received_frame_status_t status;
  ZW_APPLICATION_TX_BUFFER frame_out;
  uint8_t frame_out_length = 0;

  status = invoke_cc_handler_v2(&rxOpt, (ZW_APPLICATION_TX_BUFFER*)&frame, sizeof(frame), &frame_out, &frame_out_length);

  TEST_ASSERT_EQUAL_MESSAGE(RECEIVED_FRAME_STATUS_SUCCESS, status, "Frame status didn't match");
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(sizeof(expected_frame), frame_out_length, "Frame length didn't match");
  TEST_ASSERT_EQUAL_UINT8_ARRAY_MESSAGE(&expected_frame, (uint8_t *)&frame_out, frame_out_length, "Frame didn't match");

  mock_calls_verify();
}
