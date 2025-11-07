// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file test_ZW_TransportEndpoint.c
 * @copyright 2022 Silicon Laboratories Inc.
 */

#include <ZW_TransportEndpoint.c>
#include <unity.h>
#include <mock_control.h>
#include <zaf_cc_list_generator_mock.h>
#include "zaf_config_api_mock.h"

const uint8_t NUMBER_OF_ENDPOINTS = 2;

void setUpSuite(void) {

}

void tearDownSuite(void) {

}

void setUp(void) {
  zaf_config_get_number_of_endpoints_IgnoreAndReturn(NUMBER_OF_ENDPOINTS);
}

void tearDown(void) {

}

/**
 * Test GetEndpointNif
 */

#define NUMBER_OF_INDIVIDUAL_ENDPOINTS ZAF_CONFIG_NUMBER_OF_END_POINTS
#define NUMBER_OF_AGGREGATED_ENDPOINTS 0
#define TEST_SECURITY_KEY_BITS (SECURITY_KEY_S0_BIT | SECURITY_KEY_S2_UNAUTHENTICATED_BIT)

/* All the following command class arrays were at some point defined as const.
 * That resulted in a warning when assigning to EP_NIF endpointsNIF:
 * "initialization discards 'const' qualifier from pointer target type"
 */
static uint8_t ep1_noSec_InclNonSecure[] =
{
    COMMAND_CLASS_ZWAVEPLUS_INFO,
    COMMAND_CLASS_SWITCH_BINARY,
    COMMAND_CLASS_BASIC
};

static zaf_cc_list_t ep1_noSec_InclNonSecure_list = {
  .cc_list = ep1_noSec_InclNonSecure,
  .list_size = sizeof(ep1_noSec_InclNonSecure)
};

static zaf_cc_list_t *p_ep1_noSec_InclNonSecure_list = &ep1_noSec_InclNonSecure_list;

static uint8_t ep1_noSec_InclSecure[] =
{
    COMMAND_CLASS_SECURITY_2,
    COMMAND_CLASS_ZWAVEPLUS_INFO,
    COMMAND_CLASS_SECURITY
};

static zaf_cc_list_t ep1_noSec_InclSecure_list = {
  .cc_list = ep1_noSec_InclSecure,
  .list_size = sizeof(ep1_noSec_InclSecure)
};

static zaf_cc_list_t *p_ep1_noSec_InclSecure_list = &ep1_noSec_InclSecure_list;

static uint8_t ep1_sec_InclSecure[] =
{
    COMMAND_CLASS_SWITCH_BINARY,
    COMMAND_CLASS_BASIC
};

static zaf_cc_list_t ep1_sec_InclSecure_list = {
  .cc_list = ep1_sec_InclSecure,
  .list_size = sizeof(ep1_sec_InclSecure)
};

static zaf_cc_list_t *p_ep1_sec_InclSecure_list = &ep1_sec_InclSecure_list;

static uint8_t ep2_noSec_InclNonSecure[] =
{
    COMMAND_CLASS_ZWAVEPLUS_INFO,
    COMMAND_CLASS_SWITCH_BINARY,
    COMMAND_CLASS_BASIC,
    COMMAND_CLASS_ASSOCIATION
};

static zaf_cc_list_t ep2_noSec_InclNonSecure_list = {
  .cc_list = ep2_noSec_InclNonSecure,
  .list_size = sizeof(ep2_noSec_InclNonSecure)
};

static zaf_cc_list_t *p_ep2_noSec_InclNonSecure_list = &ep2_noSec_InclNonSecure_list;

static uint8_t ep2_noSec_InclSecure[] =
{
    COMMAND_CLASS_SECURITY,
    COMMAND_CLASS_ZWAVEPLUS_INFO,
    COMMAND_CLASS_SECURITY_2
};

static zaf_cc_list_t ep2_noSec_InclSecure_list = {
  .cc_list = ep2_noSec_InclSecure,
  .list_size = sizeof(ep2_noSec_InclSecure)
};

static zaf_cc_list_t *p_ep2_noSec_InclSecure_list = &ep2_noSec_InclSecure_list;

static uint8_t ep2_sec_InclSecure[] =
{
    COMMAND_CLASS_SWITCH_BINARY,
    COMMAND_CLASS_BASIC,
    COMMAND_CLASS_ASSOCIATION
};

static zaf_cc_list_t ep2_sec_InclSecure_list = {
  .cc_list = ep2_sec_InclSecure,
  .list_size = sizeof(ep2_sec_InclSecure)
};

static zaf_cc_list_t *p_ep2_sec_InclSecure_list = &ep2_sec_InclSecure_list;

void ZCB_timeout_kick(void)
{

}

void test_get_sec_endpoint_1_CCL_secure_incl(void)
{
  zaf_cc_list_t* pEpList = NULL;
  mock_t* pMock = NULL;
  mock_calls_clear();

  mock_call_expect(TO_STR(ZAF_GetSecurityKeys), &pMock);
  pMock->return_code.v = TEST_SECURITY_KEY_BITS;

  mock_call_expect(TO_STR(GetHighestSecureLevel), &pMock);
  pMock->expect_arg[0].v = TEST_SECURITY_KEY_BITS;
  pMock->return_code.v = SECURITY_KEY_S2_UNAUTHENTICATED;

  zafi_cc_list_generator_get_lists_Expect(1, NULL, NULL, NULL);
  zafi_cc_list_generator_get_lists_IgnoreArg_unsecure_included_ccs(); // Used as output;
  zafi_cc_list_generator_get_lists_ReturnMemThruPtr_unsecure_included_ccs(&p_ep1_noSec_InclNonSecure_list, sizeof(p_ep1_noSec_InclNonSecure_list));
  zafi_cc_list_generator_get_lists_IgnoreArg_secure_included_unsecure_ccs(); // Used as output;
  zafi_cc_list_generator_get_lists_ReturnMemThruPtr_secure_included_unsecure_ccs(&p_ep1_noSec_InclSecure_list, sizeof(p_ep1_noSec_InclSecure_list));
  zafi_cc_list_generator_get_lists_IgnoreArg_secure_included_secure_ccs(); // Used as output;
  zafi_cc_list_generator_get_lists_ReturnMemThruPtr_secure_included_secure_ccs(&p_ep1_sec_InclSecure_list, sizeof(p_ep1_sec_InclSecure_list));

  pEpList = GetEndpointcmdClassList( true, 1);

  TEST_ASSERT_NOT_NULL(pEpList);

  TEST_ASSERT_EQUAL_UINT8_ARRAY(p_ep1_sec_InclSecure_list->cc_list, pEpList->cc_list, p_ep1_noSec_InclNonSecure_list->list_size);

  mock_call_expect(TO_STR(ZAF_GetSecurityKeys), &pMock);
  pMock->return_code.v = TEST_SECURITY_KEY_BITS;

  mock_call_expect(TO_STR(GetHighestSecureLevel), &pMock);
  pMock->expect_arg[0].v = TEST_SECURITY_KEY_BITS;
  pMock->return_code.v = SECURITY_KEY_S2_UNAUTHENTICATED;

  zafi_cc_list_generator_get_lists_Expect(1, NULL, NULL, NULL);
  zafi_cc_list_generator_get_lists_IgnoreArg_unsecure_included_ccs(); // Used as output;
  zafi_cc_list_generator_get_lists_ReturnMemThruPtr_unsecure_included_ccs(&p_ep1_noSec_InclNonSecure_list, sizeof(p_ep1_noSec_InclNonSecure_list));
  zafi_cc_list_generator_get_lists_IgnoreArg_secure_included_unsecure_ccs(); // Used as output;
  zafi_cc_list_generator_get_lists_ReturnMemThruPtr_secure_included_unsecure_ccs(&p_ep1_noSec_InclSecure_list, sizeof(p_ep1_noSec_InclSecure_list));
  zafi_cc_list_generator_get_lists_IgnoreArg_secure_included_secure_ccs(); // Used as output;
  zafi_cc_list_generator_get_lists_ReturnMemThruPtr_secure_included_secure_ccs(&p_ep1_sec_InclSecure_list, sizeof(p_ep1_sec_InclSecure_list));

  pEpList = GetEndpointcmdClassList( false, 1);

  TEST_ASSERT_NOT_NULL(pEpList);

  TEST_ASSERT_EQUAL_UINT8_ARRAY(p_ep1_noSec_InclSecure_list->cc_list, pEpList->cc_list, p_ep1_noSec_InclSecure_list->list_size);
  mock_calls_verify();
}

void test_get_sec_endpoint_2_CCL_secure_incl(void)
{
  zaf_cc_list_t* pEpList = NULL;
  mock_t* pMock = NULL;
  mock_calls_clear();
  mock_call_expect(TO_STR(ZAF_GetSecurityKeys), &pMock);
  pMock->return_code.v = TEST_SECURITY_KEY_BITS;

  mock_call_expect(TO_STR(GetHighestSecureLevel), &pMock);
  pMock->expect_arg[0].v = TEST_SECURITY_KEY_BITS;
  pMock->return_code.v = SECURITY_KEY_S2_UNAUTHENTICATED;

  zafi_cc_list_generator_get_lists_Expect(2, NULL, NULL, NULL);
  zafi_cc_list_generator_get_lists_IgnoreArg_unsecure_included_ccs(); // Used as output;
  zafi_cc_list_generator_get_lists_ReturnMemThruPtr_unsecure_included_ccs(&p_ep2_noSec_InclNonSecure_list, sizeof(p_ep2_noSec_InclNonSecure_list));
  zafi_cc_list_generator_get_lists_IgnoreArg_secure_included_unsecure_ccs(); // Used as output;
  zafi_cc_list_generator_get_lists_ReturnMemThruPtr_secure_included_unsecure_ccs(&p_ep2_noSec_InclSecure_list, sizeof(p_ep2_noSec_InclSecure_list));
  zafi_cc_list_generator_get_lists_IgnoreArg_secure_included_secure_ccs(); // Used as output;
  zafi_cc_list_generator_get_lists_ReturnMemThruPtr_secure_included_secure_ccs(&p_ep2_sec_InclSecure_list, sizeof(p_ep2_sec_InclSecure_list));

  pEpList = GetEndpointcmdClassList( true, 2);

  TEST_ASSERT_NOT_NULL(pEpList);

  TEST_ASSERT_EQUAL_UINT8_ARRAY(p_ep2_sec_InclSecure_list->cc_list, pEpList->cc_list, p_ep2_noSec_InclNonSecure_list->list_size);

  mock_call_expect(TO_STR(ZAF_GetSecurityKeys), &pMock);
  pMock->return_code.v = TEST_SECURITY_KEY_BITS;

  mock_call_expect(TO_STR(GetHighestSecureLevel), &pMock);
  pMock->expect_arg[0].v = TEST_SECURITY_KEY_BITS;
  pMock->return_code.v = SECURITY_KEY_S2_UNAUTHENTICATED;

  zafi_cc_list_generator_get_lists_Expect(2, NULL, NULL, NULL);
  zafi_cc_list_generator_get_lists_IgnoreArg_unsecure_included_ccs(); // Used as output;
  zafi_cc_list_generator_get_lists_ReturnMemThruPtr_unsecure_included_ccs(&p_ep2_noSec_InclNonSecure_list, sizeof(p_ep2_noSec_InclNonSecure_list));
  zafi_cc_list_generator_get_lists_IgnoreArg_secure_included_unsecure_ccs(); // Used as output;
  zafi_cc_list_generator_get_lists_ReturnMemThruPtr_secure_included_unsecure_ccs(&p_ep2_noSec_InclSecure_list, sizeof(p_ep2_noSec_InclSecure_list));
  zafi_cc_list_generator_get_lists_IgnoreArg_secure_included_secure_ccs(); // Used as output;
  zafi_cc_list_generator_get_lists_ReturnMemThruPtr_secure_included_secure_ccs(&p_ep2_sec_InclSecure_list, sizeof(p_ep2_sec_InclSecure_list));

  pEpList = GetEndpointcmdClassList( false, 2);

  TEST_ASSERT_NOT_NULL(pEpList);

  TEST_ASSERT_EQUAL_UINT8_ARRAY(p_ep2_noSec_InclSecure_list->cc_list, pEpList->cc_list, p_ep2_noSec_InclSecure_list->list_size);
  mock_calls_verify();
}

void test_get_sec_endpoint_3_CCL_secure_incl(void)
{
  mock_calls_clear();
  zaf_cc_list_t* pEpList = NULL;

  pEpList = GetEndpointcmdClassList( true, 3);

  TEST_ASSERT_NULL(pEpList);
  mock_calls_verify();
}


void test_get_sec_endpoint_1_CCL_non_secure_incl(void)
{
  zaf_cc_list_t* pEpList = NULL;
  mock_t* pMock = NULL;
  mock_calls_clear();

  mock_call_expect(TO_STR(ZAF_GetSecurityKeys), &pMock);
  pMock->return_code.v = TEST_SECURITY_KEY_BITS;

  mock_call_expect(TO_STR(GetHighestSecureLevel), &pMock);
  pMock->expect_arg[0].v = TEST_SECURITY_KEY_BITS;
  pMock->return_code.v = SECURITY_KEY_NONE;

  zafi_cc_list_generator_get_lists_Expect(1, NULL, NULL, NULL);
  zafi_cc_list_generator_get_lists_IgnoreArg_unsecure_included_ccs(); // Used as output;
  zafi_cc_list_generator_get_lists_ReturnMemThruPtr_unsecure_included_ccs(&p_ep1_noSec_InclNonSecure_list, sizeof(p_ep1_noSec_InclNonSecure_list));
  zafi_cc_list_generator_get_lists_IgnoreArg_secure_included_unsecure_ccs(); // Used as output;
  zafi_cc_list_generator_get_lists_ReturnMemThruPtr_secure_included_unsecure_ccs(&p_ep1_noSec_InclSecure_list, sizeof(p_ep1_noSec_InclSecure_list));
  zafi_cc_list_generator_get_lists_IgnoreArg_secure_included_secure_ccs(); // Used as output;
  zafi_cc_list_generator_get_lists_ReturnMemThruPtr_secure_included_secure_ccs(&p_ep1_sec_InclSecure_list, sizeof(p_ep1_sec_InclSecure_list));

  pEpList = GetEndpointcmdClassList( false , 1);

  TEST_ASSERT_NOT_NULL(pEpList);

  TEST_ASSERT_EQUAL_UINT8_ARRAY(p_ep1_noSec_InclNonSecure_list->cc_list, pEpList->cc_list, p_ep1_noSec_InclNonSecure_list->list_size);

  mock_call_expect(TO_STR(ZAF_GetSecurityKeys), &pMock);
  pMock->return_code.v = TEST_SECURITY_KEY_BITS;

  mock_call_expect(TO_STR(GetHighestSecureLevel), &pMock);
  pMock->expect_arg[0].v = TEST_SECURITY_KEY_BITS;
  pMock->return_code.v = SECURITY_KEY_NONE;

  zafi_cc_list_generator_get_lists_Expect(1, NULL, NULL, NULL);
  zafi_cc_list_generator_get_lists_IgnoreArg_unsecure_included_ccs(); // Used as output;
  zafi_cc_list_generator_get_lists_ReturnMemThruPtr_unsecure_included_ccs(&p_ep1_noSec_InclNonSecure_list, sizeof(p_ep1_noSec_InclNonSecure_list));
  zafi_cc_list_generator_get_lists_IgnoreArg_secure_included_unsecure_ccs(); // Used as output;
  zafi_cc_list_generator_get_lists_ReturnMemThruPtr_secure_included_unsecure_ccs(&p_ep1_noSec_InclSecure_list, sizeof(p_ep1_noSec_InclSecure_list));
  zafi_cc_list_generator_get_lists_IgnoreArg_secure_included_secure_ccs(); // Used as output;
  zafi_cc_list_generator_get_lists_ReturnMemThruPtr_secure_included_secure_ccs(&p_ep1_sec_InclSecure_list, sizeof(p_ep1_sec_InclSecure_list));

  pEpList = GetEndpointcmdClassList( true , 1);

  TEST_ASSERT_NULL(pEpList->cc_list);
  TEST_ASSERT_EQUAL_UINT8(0, pEpList->list_size);
  mock_calls_verify();
}

void test_Check_not_legal_response_job(void)
{
  mock_calls_clear();
  RECEIVE_OPTIONS_TYPE_EX rxOpt;

  memset(&rxOpt, 0,sizeof(rxOpt));

  /* default singlecast Set job: false*/
  TEST_ASSERT_FALSE( Check_not_legal_response_job( &rxOpt ));

  /*Test bit adr*/
  rxOpt.destNode.BitAddress = true;
  TEST_ASSERT_TRUE( Check_not_legal_response_job( &rxOpt ));

  /*test broadcast*/
  rxOpt.destNode.BitAddress = false;
  rxOpt.rxStatus = RECEIVE_STATUS_TYPE_BROAD ;
  TEST_ASSERT_TRUE( Check_not_legal_response_job( &rxOpt ));

  /*test multicast*/
  rxOpt.rxStatus = 0;
  rxOpt.rxStatus =  RECEIVE_STATUS_TYPE_MULTI;
  TEST_ASSERT_TRUE( Check_not_legal_response_job( &rxOpt ));

  /*test broadcast and multicast*/
  rxOpt.rxStatus = 0;
  rxOpt.rxStatus = (RECEIVE_STATUS_TYPE_BROAD | RECEIVE_STATUS_TYPE_MULTI);
  TEST_ASSERT_TRUE( Check_not_legal_response_job( &rxOpt ));

  /*test supervision encapulated*/
  rxOpt.rxStatus = 0;
  SetFlagSupervisionEncap(true);
  TEST_ASSERT_TRUE( Check_not_legal_response_job( &rxOpt ));
  /*test supervision encapulated flag is cleared*/
  TEST_ASSERT_FALSE( Check_not_legal_response_job( &rxOpt ));

  mock_calls_verify();
}

void test_is_multicast(void)
{
  mock_calls_clear();
  RECEIVE_OPTIONS_TYPE_EX rxOpt;

  memset(&rxOpt, 0,sizeof(rxOpt));

  /* default singlecast Set job: false*/
  TEST_ASSERT_FALSE( is_multicast( &rxOpt ));

  /*test broadcast*/
  rxOpt.destNode.BitAddress = false;
  rxOpt.rxStatus = RECEIVE_STATUS_TYPE_BROAD ;
  TEST_ASSERT_TRUE( is_multicast( &rxOpt ));

  /*test multicast*/
  rxOpt.rxStatus = 0;
  rxOpt.rxStatus =  RECEIVE_STATUS_TYPE_MULTI;
  TEST_ASSERT_TRUE( is_multicast( &rxOpt ));

  /*test broadcast and multicast*/
  rxOpt.rxStatus = 0;
  rxOpt.rxStatus = (RECEIVE_STATUS_TYPE_BROAD | RECEIVE_STATUS_TYPE_MULTI);
  TEST_ASSERT_TRUE( is_multicast( &rxOpt ));

  mock_calls_verify();
}


void test_ZAF_Transmit(void)
{
  mock_t* pMock = NULL;
  mock_calls_clear();

  // Test: If node is not included, ZAF_Transmit should immediately return with success, as there is nothing to do.
  mock_call_expect(TO_STR(ZAF_GetInclusionState), &pMock);
  pMock->return_code.v = EINCLUSIONMODE_NOT_SET;

  EZAF_EnqueueStatus_t status = ZAF_Transmit(NULL, 0, NULL, NULL);
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(ZAF_ENQUEUE_STATUS_SUCCESS, status, "ZAF_ENQUEUE_STATUS doesn't match for not included node :(");


  // Test sending some frame if node is included.
  mock_call_expect(TO_STR(ZAF_GetInclusionState), &pMock);
  pMock->return_code.v = EINCLUSIONMODE_ZWAVE_CLS;

  // some random values to send
  ZAF_TRANSPORT_TX_BUFFER  TxBuf;
  uint8_t frame_len = sizeof(ZW_ALARM_REPORT_FRAME);

  TRANSMIT_OPTIONS_TYPE_SINGLE_EX txOptions;
  txOptions.txOptions = ZWAVE_PLUS_TX_OPTIONS;
  txOptions.sourceEndpoint = 1; //source endpoint;
  MULTICHAN_NODE_ID destNode;
  destNode.nodeInfo.BitMultiChannelEncap = false;
  txOptions.pDestNode = &destNode;

  SQueueNotifying  txQueueNotifying_mock;
  mock_call_expect(TO_STR(ZAF_getZwTxQueue), &pMock);
  pMock->return_code.p = &txQueueNotifying_mock;

  SZwaveTransmitPackage FramePackage;
  mock_call_expect(TO_STR(QueueNotifyingSendToBack), &pMock);
  pMock->compare_rule_arg[0] = COMPARE_NOT_NULL;
  pMock->expect_arg[0].p = &txQueueNotifying_mock;
  pMock->compare_rule_arg[1] = COMPARE_NOT_NULL;
  pMock->expect_arg[1].p = &FramePackage;
  pMock->expect_arg[2].v = QUEUE_NOTIFYING_SEND_MAX_WAIT;
  pMock->return_code.v = 0; // 0 = ZAF_ENQUEUE_STATUS_SUCCESS

  status = ZAF_Transmit( (uint8_t *)&TxBuf, frame_len, &txOptions, NULL);
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(ZAF_ENQUEUE_STATUS_SUCCESS, status, "ZAF_ENQUEUE_STATUS doesn't match for included node :(");

  mock_calls_verify();

}
