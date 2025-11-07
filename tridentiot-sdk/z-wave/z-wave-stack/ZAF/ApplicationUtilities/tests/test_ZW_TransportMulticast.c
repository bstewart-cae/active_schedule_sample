// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file test_ZW_TransportMulticast.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include <stdint.h>
#include <stdlib.h>
#include <ZW_TransportMulticast.c>
#include <unity.h>
#include <mock_control.h>
#include <SizeOf.h>

void setUpSuite(void) {

}

void tearDownSuite(void) {

}

void setUp(void)
{
  mock_t * pMock = NULL;
  
  mock_calls_clear();
  mock_call_use_as_stub(TO_STR(ZW_TransportEndpoint_Init));

  // Clear the App Handle to prevent it from leaking to another test
  ZAF_setAppHandle(NULL);

  mock_call_expect(TO_STR(zpal_pm_register), &pMock);
  pMock->expect_arg[0].v     = ZPAL_PM_TYPE_USE_RADIO;

  ZW_TransportMulticast_init();

  mock_calls_verify();
}

static SApplicationHandles * GetApplicationHandles(uint8_t grantedKeys);

static uint8_t howManyTimesWasCallbackCalled = 0;

#define DATA_SIZE (20)
static ZAF_TRANSPORT_TX_BUFFER buffer;

static uint8_t * someData = (uint8_t *)&buffer.appTxBuf;

const uint8_t MULTICAST_GROUP_ID = 2;

transmission_result_t ExpectedTXResult;

void multicast_callback(TRANSMISSION_RESULT * pTransmissionResult)
{
  howManyTimesWasCallbackCalled++;

  TEST_ASSERT_EQUAL_MESSAGE(ExpectedTXResult.isFinished, pTransmissionResult->isFinished, "Finish flag is incorrect");
  TEST_ASSERT_EQUAL_MESSAGE(ExpectedTXResult.nodeId, pTransmissionResult->nodeId, "Node ID is incorrect");
  TEST_ASSERT_EQUAL_MESSAGE(ExpectedTXResult.status, pTransmissionResult->status, "Status is incorrect");
}

void test_empty_node_list(void)
{
  TRANSMIT_OPTIONS_TYPE_EX nodeList;

  nodeList.list_length = 0;

  enum ETRANSPORT_MULTICAST_STATUS result;

  result = ZW_TransportMulticast_SendRequest(
      someData,
      DATA_SIZE,
      false,
      &nodeList,
      multicast_callback);

  TEST_ASSERT_EQUAL_UINT32(0, result); // ETRANSPORTMULTICAST_FAILED = 0
  TEST_ASSERT_EQUAL_UINT8(0, howManyTimesWasCallbackCalled);
}

void test_nodelist_null(void)
{
  enum ETRANSPORT_MULTICAST_STATUS result;
  result = ZW_TransportMulticast_SendRequest(
      someData,
      DATA_SIZE,
      false,
      NULL,
      multicast_callback);

  TEST_ASSERT_EQUAL_UINT32(0, result); // ETRANSPORTMULTICAST_FAILED = 0
  TEST_ASSERT_EQUAL_UINT8(0, howManyTimesWasCallbackCalled);
}

void test_data_null(void)
{
  TRANSMIT_OPTIONS_TYPE_EX nodeList;

  nodeList.list_length = 0;

  enum ETRANSPORT_MULTICAST_STATUS result;
  result = ZW_TransportMulticast_SendRequest(
      NULL,
      DATA_SIZE,
      false,
      &nodeList,
      multicast_callback);

  TEST_ASSERT_EQUAL_UINT32(0, result); // ETRANSPORTMULTICAST_FAILED = 0
  TEST_ASSERT_EQUAL_UINT8(0, howManyTimesWasCallbackCalled);
}

void test_data_length_zero(void)
{
  uint8_t someData[20] = {0};
  TRANSMIT_OPTIONS_TYPE_EX nodeList;

  nodeList.list_length = 0;

  enum ETRANSPORT_MULTICAST_STATUS result;
  result = ZW_TransportMulticast_SendRequest(
      someData,
      0,
      false,
      &nodeList,
      multicast_callback);

  TEST_ASSERT_EQUAL_UINT32(0, result); // ETRANSPORTMULTICAST_FAILED = 0
  TEST_ASSERT_EQUAL_UINT8(0, howManyTimesWasCallbackCalled);
}


void test_call_send_data_multicast_no_sv(void)
{
  mock_t * pMock = NULL;

  mock_call_use_as_stub(TO_STR(AssociationGetBitAdressingDestination));

#define NODE_LIST_SIZE (3)
#define TEST_SECURITY_KEY_BITS (0x81) // SECURITY_KEY_S0_BIT | SECURITY_KEY_S2_UNAUTHENTICATED_BIT

  SApplicationHandles * pAppHandles = GetApplicationHandles(TEST_SECURITY_KEY_BITS);
  ZAF_setAppHandle(pAppHandles);

  //memset(someData, 0xAA, DATA_SIZE);

  TRANSMIT_OPTIONS_TYPE_EX nodeList;
  nodeList.list_length = NODE_LIST_SIZE;
  nodeList.sourceEndpoint = 0;
  nodeList.txOptions = 0;
  nodeList.S2_groupID = MULTICAST_GROUP_ID;

  MULTICHAN_NODE_ID multichan_node_id[NODE_LIST_SIZE];
  multichan_node_id[0].node.nodeId = 1;
  multichan_node_id[0].node.endpoint = 0;
  multichan_node_id[0].nodeInfo.security = SECURITY_KEY_S2_UNAUTHENTICATED;
  multichan_node_id[1].node.nodeId = 2;
  multichan_node_id[1].node.endpoint = 0;
  multichan_node_id[1].nodeInfo.security = SECURITY_KEY_S2_UNAUTHENTICATED;
  multichan_node_id[2].node.nodeId = 3;
  multichan_node_id[2].node.endpoint = 0;
  multichan_node_id[2].nodeInfo.security = SECURITY_KEY_S2_UNAUTHENTICATED;

  nodeList.pList = multichan_node_id;

  mock_call_expect(TO_STR(AssociationGetDestinationInit), &pMock);
  pMock->expect_arg[0].p = multichan_node_id;

  mock_call_expect(TO_STR(AssociationGetSinglecastNodeCount), &pMock);
  pMock->return_code.v = nodeList.list_length;

  mock_call_expect(TO_STR(GetHighestSecureLevel), &pMock);
  pMock->expect_arg[0].v = TEST_SECURITY_KEY_BITS;
  pMock->return_code.v = SECURITY_KEY_S2_UNAUTHENTICATED;

  mock_call_expect(TO_STR(QueueNotifyingSendToBack), &pMock);
  static SZwaveTransmitPackage FramePackage;
  FramePackage.eTransmitType = EZWAVETRANSMITTYPE_MULTI_EX;
  FramePackage.uTransmitParams.SendDataMultiEx.FrameConfig.TransmitOptions = 0;
  FramePackage.uTransmitParams.SendDataMultiEx.FrameConfig.iFrameLength = DATA_SIZE;
  FramePackage.uTransmitParams.SendDataMultiEx.GroupId = nodeList.S2_groupID;
  memcpy(FramePackage.uTransmitParams.SendDataMultiEx.FrameConfig.aFrame, someData, DATA_SIZE);

  pMock->expect_arg[0].p = pAppHandles->pZwTxQueue;
  pMock->expect_arg[1].p = &FramePackage;
  pMock->expect_arg[2].v = 0;
  pMock->return_code.v = 0; // 0 = EQUEUENOTIFYING_STATUS_SUCCESS

  enum ETRANSPORT_MULTICAST_STATUS result;
  result = ZW_TransportMulticast_SendRequest(
      someData,
      DATA_SIZE,
      false,
      &nodeList,
      multicast_callback);

  free((void *) pAppHandles->pNetworkInfo);
  free(pAppHandles->pZwTxQueue);
  free(pAppHandles);

  mock_calls_verify();

  TEST_ASSERT_EQUAL_UINT32(1, result); // 1 = ETRANSPORTMULTICAST_ADDED_TO_QUEUE
  TEST_ASSERT_EQUAL_UINT8(0, howManyTimesWasCallbackCalled);
}

static uint8_t callback_call_send_data_no_sv_one_node_count = 0;

void callback_call_send_data_no_sv_one_node(TRANSMISSION_RESULT * pTransmissionResult)
{
  TEST_ASSERT_EQUAL_UINT8(1, pTransmissionResult->nodeId);
  TEST_ASSERT_EQUAL_UINT8(0, pTransmissionResult->status); // 0 = TRANSMIT_COMPLETE_OK
  TEST_ASSERT_EQUAL_UINT8(1, pTransmissionResult->isFinished); // 1 = finished

  callback_call_send_data_no_sv_one_node_count++;
}

void test_call_send_data_no_sv_one_node(void)
{
  mock_t * pMock = NULL;

  mock_call_use_as_stub(TO_STR(AssociationGetBitAdressingDestination));

#undef NODE_LIST_SIZE
#define NODE_LIST_SIZE (1)
#undef TEST_SECURITY_KEY_BITS
#define TEST_SECURITY_KEY_BITS (0x81) // SECURITY_KEY_S0_BIT | SECURITY_KEY_S2_UNAUTHENTICATED_BIT

  SApplicationHandles * pAppHandles = GetApplicationHandles(TEST_SECURITY_KEY_BITS);
  ZAF_setAppHandle(pAppHandles);

  TRANSMIT_OPTIONS_TYPE_EX nodeList;
  nodeList.list_length = NODE_LIST_SIZE;
  nodeList.sourceEndpoint = 0;
  nodeList.txOptions = 0;

  MULTICHAN_NODE_ID multichan_node_id[NODE_LIST_SIZE];
  multichan_node_id[0].node.nodeId = 1;
  multichan_node_id[0].node.endpoint = 0;

  // Destination node uses S2 Unauthenticated.
  multichan_node_id[0].nodeInfo.security = SECURITY_KEY_S2_UNAUTHENTICATED;

  nodeList.pList = multichan_node_id;

  mock_call_use_as_stub(TO_STR(AssociationGetDestinationInit));

  mock_call_expect(TO_STR(AssociationGetSinglecastNodeCount), &pMock);
  pMock->return_code.v = nodeList.list_length;

  mock_call_expect(TO_STR(AssociationGetNextSinglecastDestination), &pMock);
  pMock->return_code.p = &multichan_node_id[0];

  mock_call_expect(TO_STR(ZAF_Transmit), &pMock);
  pMock->compare_rule_arg[0] = COMPARE_NOT_NULL;
  pMock->expect_arg[1].v = DATA_SIZE;
  pMock->compare_rule_arg[2] = COMPARE_NOT_NULL;
  pMock->compare_rule_arg[3] = COMPARE_NOT_NULL;
  pMock->return_code.v = 0; // 0 = EQUEUENOTIFYING_STATUS_SUCCESS

  enum ETRANSPORT_MULTICAST_STATUS result;
  result = ZW_TransportMulticast_SendRequest(
      someData,
      DATA_SIZE,
      false,
      &nodeList,
      callback_call_send_data_no_sv_one_node);

  TRANSMISSION_RESULT transmissionResult;

  transmissionResult.nodeId = 1;
  transmissionResult.status = TRANSMIT_COMPLETE_OK;

  ZCB_multicast_callback(&transmissionResult);

  free((void *) pAppHandles->pNetworkInfo);
  free(pAppHandles->pZwTxQueue);
  free(pAppHandles);

  mock_calls_verify();

  TEST_ASSERT_EQUAL_UINT32(1, result); // 1 = ETRANSPORTMULTICAST_ADDED_TO_QUEUE
  TEST_ASSERT_EQUAL_UINT8(1, callback_call_send_data_no_sv_one_node_count);
}

/******************************************************************************
 * Next test
 ******************************************************************************
 */

static uint8_t callback_call_send_data_no_sv_two_nodes_count = 0;

void callback_call_send_data_no_sv_two_nodes(TRANSMISSION_RESULT * pTransmissionResult)
{
  switch (callback_call_send_data_no_sv_two_nodes_count)
  {
  case 0:
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(1, pTransmissionResult->nodeId, "(1) Wrong node ID.");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(0, pTransmissionResult->status, "(1) Wrong status."); // 0 = TRANSMIT_COMPLETE_OK
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(0, pTransmissionResult->isFinished, "(1) Wrong finish value."); // 1 = finished
    break;
  case 1:
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(2, pTransmissionResult->nodeId, "(2) Wrong node ID.");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(0, pTransmissionResult->status, "(2) Wrong status."); // 0 = TRANSMIT_COMPLETE_OK
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(1, pTransmissionResult->isFinished, "(2) Wrong finish value."); // 1 = finished
    break;
  default:
    // Do nothing.
    break;
  }

  callback_call_send_data_no_sv_two_nodes_count++;
}

void test_call_send_data_no_sv_two_nodes(void)
{
  mock_t * pMock = NULL;

  mock_call_use_as_stub(TO_STR(AssociationGetBitAdressingDestination));

#undef NODE_LIST_SIZE
#define NODE_LIST_SIZE (2)
#undef TEST_SECURITY_KEY_BITS
#define TEST_SECURITY_KEY_BITS (0x81) // SECURITY_KEY_S0_BIT | SECURITY_KEY_S2_UNAUTHENTICATED_BIT

  SApplicationHandles * pAppHandles = GetApplicationHandles(TEST_SECURITY_KEY_BITS);
  ZAF_setAppHandle(pAppHandles);

  TRANSMIT_OPTIONS_TYPE_EX nodeList;
  nodeList.list_length = NODE_LIST_SIZE;
  nodeList.sourceEndpoint = 0;
  nodeList.txOptions = 0;
  nodeList.S2_groupID = MULTICAST_GROUP_ID;

  MULTICHAN_NODE_ID multichan_node_id[NODE_LIST_SIZE];
  multichan_node_id[0].node.nodeId = 1;
  multichan_node_id[0].node.endpoint = 0;
  multichan_node_id[0].nodeInfo.security = SECURITY_KEY_S2_UNAUTHENTICATED;
  multichan_node_id[1].node.nodeId = 2;
  multichan_node_id[1].node.endpoint = 0;
  multichan_node_id[1].nodeInfo.security = SECURITY_KEY_S2_UNAUTHENTICATED;

  nodeList.pList = multichan_node_id;

  mock_call_use_as_stub(TO_STR(AssociationGetDestinationInit));

  mock_call_expect(TO_STR(AssociationGetSinglecastNodeCount), &pMock);
  pMock->return_code.v = nodeList.list_length;

  mock_call_expect(TO_STR(GetHighestSecureLevel), &pMock);
  pMock->expect_arg[0].v = TEST_SECURITY_KEY_BITS;
  pMock->return_code.v = SECURITY_KEY_S2_UNAUTHENTICATED;

  mock_call_expect(TO_STR(QueueNotifyingSendToBack), &pMock);
  static SZwaveTransmitPackage FramePackage;
  FramePackage.eTransmitType = EZWAVETRANSMITTYPE_MULTI_EX;
  FramePackage.uTransmitParams.SendDataMultiEx.FrameConfig.TransmitOptions = 0;
  FramePackage.uTransmitParams.SendDataMultiEx.FrameConfig.iFrameLength = DATA_SIZE;
  FramePackage.uTransmitParams.SendDataMultiEx.GroupId = nodeList.S2_groupID;
  memcpy(FramePackage.uTransmitParams.SendDataMultiEx.FrameConfig.aFrame, someData, DATA_SIZE);

  pMock->expect_arg[0].p = pAppHandles->pZwTxQueue;
  pMock->expect_arg[1].p = &FramePackage;
  pMock->expect_arg[2].v = 0;
  pMock->return_code.v = 0; // 0 = EQUEUENOTIFYING_STATUS_SUCCESS


  enum ETRANSPORT_MULTICAST_STATUS result;
  result = ZW_TransportMulticast_SendRequest(
      someData,
      DATA_SIZE,
      false,
      &nodeList,
      callback_call_send_data_no_sv_two_nodes);

  TEST_ASSERT_EQUAL_UINT32_MESSAGE(1, result, "Wrong return value from ZW_TransportMulticast_SendRequest"); // 1 = ETRANSPORTMULTICAST_ADDED_TO_QUEUE

  mock_call_expect(TO_STR(AssociationGetNextSinglecastDestination), &pMock);
  pMock->return_code.p = &multichan_node_id[0];

  mock_call_expect(TO_STR(ZAF_Transmit), &pMock);
  pMock->compare_rule_arg[0] = COMPARE_NOT_NULL;
  pMock->expect_arg[1].v = DATA_SIZE;
  pMock->compare_rule_arg[2] = COMPARE_NOT_NULL;
  pMock->compare_rule_arg[3] = COMPARE_NOT_NULL;
  pMock->return_code.v = 0; // 0 = EQUEUENOTIFYING_STATUS_SUCCESS

  ZCB_multicast_callback(NULL);

  mock_call_expect(TO_STR(AssociationGetNextSinglecastDestination), &pMock);
  pMock->return_code.p = &multichan_node_id[1];

  mock_call_expect(TO_STR(ZAF_Transmit), &pMock);
  pMock->compare_rule_arg[0] = COMPARE_NOT_NULL;
  pMock->expect_arg[1].v = DATA_SIZE;
  pMock->compare_rule_arg[2] = COMPARE_NOT_NULL;
  pMock->compare_rule_arg[3] = COMPARE_NOT_NULL;
  pMock->return_code.v = 0; // 0 = EQUEUENOTIFYING_STATUS_SUCCESS

  TRANSMISSION_RESULT transmissionResult;
  transmissionResult.nodeId = 1;
  transmissionResult.status = TRANSMIT_COMPLETE_OK;
  ZCB_multicast_callback(&transmissionResult);

  transmissionResult.nodeId = 2;
  transmissionResult.status = TRANSMIT_COMPLETE_OK;
  ZCB_multicast_callback(&transmissionResult);

  free((void *) pAppHandles->pNetworkInfo);
  free(pAppHandles->pZwTxQueue);
  free(pAppHandles);

  mock_calls_verify();

  TEST_ASSERT_EQUAL_UINT8_MESSAGE(2, callback_call_send_data_no_sv_two_nodes_count, "Number of callbacks doesn't match.");
}

/*
 * In this test case we have two destinations in association group.
 * The first destination is without multi channel endpoint (destination 1.0)
 * The second destination is with a multi channel endpoint (destination 2.1)
 *
 * Expect no multicast transmission since one of the destinations is an
 * endpoint destination.
 * Instead it should just do two individual singlecast transmissions.
 */
void test_call_send_data_no_multicast(void)
{
  mock_t * pMock = NULL;

  mock_call_use_as_stub(TO_STR(AssociationGetBitAdressingDestination));

#undef NODE_LIST_SIZE
#define NODE_LIST_SIZE (2)
#undef TEST_SECURITY_KEY_BITS
#define TEST_SECURITY_KEY_BITS (0x81) // SECURITY_KEY_S0_BIT | SECURITY_KEY_S2_UNAUTHENTICATED_BIT

  SApplicationHandles * pAppHandles = GetApplicationHandles(TEST_SECURITY_KEY_BITS);
  ZAF_setAppHandle(pAppHandles);

  TRANSMIT_OPTIONS_TYPE_EX nodeList;
  nodeList.list_length = NODE_LIST_SIZE;
  nodeList.sourceEndpoint = 0;
  nodeList.txOptions = 0;
  nodeList.S2_groupID = MULTICAST_GROUP_ID;

  MULTICHAN_NODE_ID multichan_node_id[NODE_LIST_SIZE];
  multichan_node_id[0].node.nodeId = 1;
  multichan_node_id[0].node.endpoint = 0;
  multichan_node_id[0].nodeInfo.security = SECURITY_KEY_S2_UNAUTHENTICATED;
  multichan_node_id[1].node.nodeId = 2;
  multichan_node_id[1].node.endpoint = 1;
  multichan_node_id[1].nodeInfo.security = SECURITY_KEY_S2_UNAUTHENTICATED;

  nodeList.pList = multichan_node_id;

  mock_call_use_as_stub(TO_STR(AssociationGetDestinationInit));

  mock_call_expect(TO_STR(AssociationGetSinglecastNodeCount), &pMock);
  pMock->return_code.v = nodeList.list_length;

  mock_call_expect(TO_STR(GetHighestSecureLevel), &pMock);
  pMock->expect_arg[0].v = TEST_SECURITY_KEY_BITS;
  pMock->return_code.v = SECURITY_KEY_NONE;

  mock_call_expect(TO_STR(QueueNotifyingSendToBack), &pMock);
  pMock->compare_rule_arg[0] = COMPARE_NOT_NULL;
  pMock->compare_rule_arg[1] = COMPARE_NOT_NULL;
  pMock->expect_arg[2].v = 0;

  mock_call_expect(TO_STR(AssociationGetNextSinglecastDestination), &pMock);
  pMock->return_code.p = &multichan_node_id[0];

  mock_call_expect(TO_STR(ZAF_Transmit), &pMock);
  pMock->compare_rule_arg[0] = COMPARE_NOT_NULL;
  pMock->expect_arg[1].v = DATA_SIZE;
  pMock->compare_rule_arg[2] = COMPARE_NOT_NULL;
  pMock->compare_rule_arg[3] = COMPARE_NOT_NULL;
  pMock->return_code.v = 0; // 0 = EQUEUENOTIFYING_STATUS_SUCCESS

  enum ETRANSPORT_MULTICAST_STATUS result;
  result = ZW_TransportMulticast_SendRequest(
      someData,
      DATA_SIZE,
      false,
      &nodeList,
      callback_call_send_data_no_sv_two_nodes);

  /****************************************************************************************
   * Set expectations for 1st singlecast callback
   */
  mock_call_expect(TO_STR(AssociationGetNextSinglecastDestination), &pMock);
  pMock->return_code.p = &multichan_node_id[1];

  mock_call_expect(TO_STR(ZAF_Transmit), &pMock);
  pMock->compare_rule_arg[0] = COMPARE_NOT_NULL;
  pMock->expect_arg[1].v = DATA_SIZE;
  pMock->compare_rule_arg[2] = COMPARE_NOT_NULL;
  pMock->compare_rule_arg[3] = COMPARE_NOT_NULL;
  pMock->return_code.v = 0; // 0 = EQUEUENOTIFYING_STATUS_SUCCESS

  TRANSMISSION_RESULT transmissionResult;

  transmissionResult.nodeId = 1;
  transmissionResult.status = TRANSMIT_COMPLETE_OK;

  ZCB_multicast_callback(&transmissionResult); // Do the callback.

  /****************************************************************************************
   * Set expectations for 2nd (and last) singlecast callback
   */

  transmissionResult.nodeId = 2;
  transmissionResult.status = TRANSMIT_COMPLETE_OK;

  ZCB_multicast_callback(&transmissionResult);  // Last callback. Expect no further mock calls

  free((void *) pAppHandles->pNetworkInfo);
  free(pAppHandles->pZwTxQueue);
  free(pAppHandles);

  mock_calls_verify();

  TEST_ASSERT_EQUAL_UINT32_MESSAGE(1, result, "Wrong return value from ZW_TransportMulticast_SendRequest"); // 1 = ETRANSPORTMULTICAST_ADDED_TO_QUEUE
}


void test_call_send_data_nosecure_two_nodes(void)
{
  mock_t * pMock = NULL;

  mock_call_use_as_stub(TO_STR(AssociationGetBitAdressingDestination));

#undef NODE_LIST_SIZE
#define NODE_LIST_SIZE (2)
#undef TEST_SECURITY_KEY_BITS
#define TEST_SECURITY_KEY_BITS (SECURITY_KEY_S2_UNAUTHENTICATED_BIT)

  SApplicationHandles * pAppHandles = GetApplicationHandles(TEST_SECURITY_KEY_BITS);
  ZAF_setAppHandle(pAppHandles);

  callback_call_send_data_no_sv_two_nodes_count = 0;

  TRANSMIT_OPTIONS_TYPE_EX nodeList;
  nodeList.list_length = NODE_LIST_SIZE;
  nodeList.sourceEndpoint = 0;
  nodeList.txOptions = 0;
  nodeList.S2_groupID = MULTICAST_GROUP_ID;

  MULTICHAN_NODE_ID multichan_node_id[NODE_LIST_SIZE];
  multichan_node_id[0].node.nodeId = 1;
  multichan_node_id[0].node.endpoint = 0;
  multichan_node_id[0].nodeInfo.security = SECURITY_KEY_S2_UNAUTHENTICATED;
  multichan_node_id[1].node.nodeId = 2;
  multichan_node_id[1].node.endpoint = 0;
  multichan_node_id[1].nodeInfo.security = SECURITY_KEY_S2_UNAUTHENTICATED;

  nodeList.pList = multichan_node_id;

  mock_call_use_as_stub(TO_STR(AssociationGetDestinationInit));

  mock_call_expect(TO_STR(AssociationGetSinglecastNodeCount), &pMock);
  pMock->return_code.v = nodeList.list_length;

  /****************************************************************************************
   * Multicast flow
   */

  mock_call_expect(TO_STR(GetHighestSecureLevel), &pMock);
  pMock->expect_arg[0].v = TEST_SECURITY_KEY_BITS;
  pMock->return_code.v = SECURITY_KEY_NONE;

  mock_call_expect(TO_STR(QueueNotifyingSendToBack), &pMock);
  static SZwaveTransmitPackage FramePackage;
  FramePackage.eTransmitType = EZWAVETRANSMITTYPE_MULTI_EX;
  FramePackage.uTransmitParams.SendDataMultiEx.FrameConfig.TransmitOptions = 0;
  FramePackage.uTransmitParams.SendDataMultiEx.FrameConfig.iFrameLength = DATA_SIZE;
  FramePackage.uTransmitParams.SendDataMultiEx.GroupId = nodeList.S2_groupID;
  memcpy(FramePackage.uTransmitParams.SendDataMultiEx.FrameConfig.aFrame, someData, DATA_SIZE);

  pMock->expect_arg[0].p = pAppHandles->pZwTxQueue;
  pMock->expect_arg[1].p = &FramePackage;
  pMock->expect_arg[2].v = 0;
  pMock->return_code.v = 0; // 0 = EQUEUENOTIFYING_STATUS_SUCCESS

  enum ETRANSPORT_MULTICAST_STATUS result;
  result = ZW_TransportMulticast_SendRequest(
      someData,
      DATA_SIZE,
      false,
      &nodeList,
      callback_call_send_data_no_sv_two_nodes);

  TEST_ASSERT_EQUAL_UINT32_MESSAGE(1, result, "Wrong return value from ZW_TransportMulticast_SendRequest"); // 1 = ETRANSPORTMULTICAST_ADDED_TO_QUEUE

  /****************************************************************************************
   * Set expectations for multicast callback
   */

  mock_call_expect(TO_STR(AssociationGetNextSinglecastDestination), &pMock);
  pMock->return_code.p = &multichan_node_id[0];

  mock_call_expect(TO_STR(ZAF_Transmit), &pMock);
  pMock->compare_rule_arg[0] = COMPARE_NOT_NULL;
  pMock->expect_arg[1].v = DATA_SIZE;
  pMock->compare_rule_arg[2] = COMPARE_NOT_NULL;
  pMock->compare_rule_arg[3] = COMPARE_NOT_NULL;
  pMock->return_code.v = 0; // 0 = EQUEUENOTIFYING_STATUS_SUCCESS

  ZCB_multicast_callback(NULL);

  mock_call_expect(TO_STR(AssociationGetNextSinglecastDestination), &pMock);
  pMock->return_code.p = &multichan_node_id[1];

  mock_call_expect(TO_STR(ZAF_Transmit), &pMock);
  pMock->compare_rule_arg[0] = COMPARE_NOT_NULL;
  pMock->expect_arg[1].v = DATA_SIZE;
  pMock->compare_rule_arg[2] = COMPARE_NOT_NULL;
  pMock->compare_rule_arg[3] = COMPARE_NOT_NULL;
  pMock->return_code.v = 0; // 0 = EQUEUENOTIFYING_STATUS_SUCCESS

  TRANSMISSION_RESULT transmissionResult;
  transmissionResult.nodeId = 1;
  transmissionResult.status = TRANSMIT_COMPLETE_OK;
  ZCB_multicast_callback(&transmissionResult);

  transmissionResult.nodeId = 2;
  transmissionResult.status = TRANSMIT_COMPLETE_OK;
  ZCB_multicast_callback(&transmissionResult);

  free((void *) pAppHandles->pNetworkInfo);
  free(pAppHandles->pZwTxQueue);
  free(pAppHandles);

  mock_calls_verify();

  TEST_ASSERT_EQUAL_UINT8_MESSAGE(2, callback_call_send_data_no_sv_two_nodes_count, "Number of callbacks doesn't match.");
}

// Validate the success of two consecutive multicast transmissions with different GroupIds
void test_multicast_s2_nodes_different_groupIds(void)
{
  mock_t * pMock = NULL;

#undef TEST_SECURITY_KEY_BITS
#define TEST_SECURITY_KEY_BITS (SECURITY_KEY_S2_UNAUTHENTICATED_BIT)

  mock_call_use_as_stub(TO_STR(AssociationGetDestinationInit));
  mock_call_use_as_stub(TO_STR(AssociationGetBitAdressingDestination));

  SApplicationHandles * pAppHandles = GetApplicationHandles(TEST_SECURITY_KEY_BITS);
  ZAF_setAppHandle(pAppHandles);

  callback_call_send_data_no_sv_two_nodes_count = 0;

  /*
   * PREPARE INPUTS
   */
  MULTICHAN_NODE_ID multichan_node_id[2];
  multichan_node_id[0].node.nodeId = 1;
  multichan_node_id[0].node.endpoint = 0;
  multichan_node_id[0].nodeInfo.security = SECURITY_KEY_S2_ACCESS;
  multichan_node_id[1].node.nodeId = 2;
  multichan_node_id[1].node.endpoint = 0;
  multichan_node_id[1].nodeInfo.security = SECURITY_KEY_S2_AUTHENTICATED;

  TRANSMIT_OPTIONS_TYPE_EX nodeList;
  nodeList.pList = multichan_node_id;
  nodeList.list_length = sizeof_array(multichan_node_id);
  nodeList.sourceEndpoint = 0;
  nodeList.txOptions = 0;

  // Send two multicast frames with incrementing GroupId
  for (uint8_t i=0; i<2; i++)
  {
    callback_call_send_data_no_sv_two_nodes_count = 0;

    nodeList.S2_groupID = MULTICAST_GROUP_ID + i; // Increment the groupID

    /*
     * SETUP MOCKS
     */
    mock_call_use_as_stub(TO_STR(AssociationGetDestinationInit));    
    mock_call_use_as_stub(TO_STR(AssociationGetBitAdressingDestination));

    mock_call_expect(TO_STR(AssociationGetSinglecastNodeCount), &pMock);
    pMock->return_code.v = nodeList.list_length;

    mock_call_expect(TO_STR(GetHighestSecureLevel), &pMock);
    pMock->expect_arg[0].v = TEST_SECURITY_KEY_BITS;
    pMock->return_code.v = SECURITY_KEY_NONE;

    mock_call_expect(TO_STR(QueueNotifyingSendToBack), &pMock);
    static SZwaveTransmitPackage FramePackage;
    FramePackage.eTransmitType = EZWAVETRANSMITTYPE_MULTI_EX;
    FramePackage.uTransmitParams.SendDataMultiEx.FrameConfig.TransmitOptions = 0;
    FramePackage.uTransmitParams.SendDataMultiEx.FrameConfig.iFrameLength = DATA_SIZE;
    FramePackage.uTransmitParams.SendDataMultiEx.GroupId = nodeList.S2_groupID; // Multicast GroupId is expected to equal nodeList.S2_groupID
    memcpy(FramePackage.uTransmitParams.SendDataMultiEx.FrameConfig.aFrame, someData, DATA_SIZE);

    pMock->expect_arg[0].p = pAppHandles->pZwTxQueue;
    pMock->expect_arg[1].p = &FramePackage;
    pMock->expect_arg[2].v = 0;
    pMock->return_code.v = 0; // 0 = EQUEUENOTIFYING_STATUS_SUCCESS

    /*
     * CALL THE FUNCTION UNDER TEST
     */
    enum ETRANSPORT_MULTICAST_STATUS result;
    result = ZW_TransportMulticast_SendRequest(
        someData,
        DATA_SIZE,
        false,
        &nodeList,
        callback_call_send_data_no_sv_two_nodes);

    TEST_ASSERT_EQUAL_UINT32_MESSAGE(1, result, "Wrong return value from ZW_TransportMulticast_SendRequest"); // 1 = ETRANSPORTMULTICAST_ADDED_TO_QUEUE

    mock_call_expect(TO_STR(AssociationGetNextSinglecastDestination), &pMock);
    pMock->return_code.p = &multichan_node_id[0];

    mock_call_expect(TO_STR(ZAF_Transmit), &pMock);
    pMock->compare_rule_arg[0] = COMPARE_NOT_NULL;
    pMock->expect_arg[1].v = DATA_SIZE;
    pMock->compare_rule_arg[2] = COMPARE_NOT_NULL;
    pMock->compare_rule_arg[3] = COMPARE_NOT_NULL;
    pMock->return_code.v = 0; // 0 = EQUEUENOTIFYING_STATUS_SUCCESS

    ZCB_multicast_callback(NULL);

    mock_call_expect(TO_STR(AssociationGetNextSinglecastDestination), &pMock);
    pMock->return_code.p = &multichan_node_id[1];

    mock_call_expect(TO_STR(ZAF_Transmit), &pMock);
    pMock->compare_rule_arg[0] = COMPARE_NOT_NULL;
    pMock->expect_arg[1].v = DATA_SIZE;
    pMock->compare_rule_arg[2] = COMPARE_NOT_NULL;
    pMock->compare_rule_arg[3] = COMPARE_NOT_NULL;
    pMock->return_code.v = 0; // 0 = EQUEUENOTIFYING_STATUS_SUCCESS

    TRANSMISSION_RESULT transmissionResult;
    transmissionResult.nodeId = 1;
    transmissionResult.status = TRANSMIT_COMPLETE_OK;
    ZCB_multicast_callback(&transmissionResult);

    transmissionResult.nodeId = 2;
    transmissionResult.status = TRANSMIT_COMPLETE_OK;
    ZCB_multicast_callback(&transmissionResult);

    TEST_ASSERT_EQUAL_UINT8_MESSAGE(2, callback_call_send_data_no_sv_two_nodes_count, "Number of callbacks doesn't match.");
  }

  free((void *) pAppHandles->pNetworkInfo);
  free(pAppHandles->pZwTxQueue);
  free(pAppHandles);

  mock_calls_verify();
}

void test_TO6684_multicast_to_s2_nodes_only(void)
{
  mock_t * pMock = NULL;

  mock_call_use_as_stub(TO_STR(AssociationGetDestinationInit));
  mock_call_use_as_stub(TO_STR(AssociationGetBitAdressingDestination));

#undef TEST_SECURITY_KEY_BITS
#define TEST_SECURITY_KEY_BITS (SECURITY_KEY_S2_UNAUTHENTICATED_BIT)

  SApplicationHandles * pAppHandles = GetApplicationHandles(TEST_SECURITY_KEY_BITS);
  ZAF_setAppHandle(pAppHandles);

  callback_call_send_data_no_sv_two_nodes_count = 0;

  /*
   * PREPARE INPUTS
   */
  MULTICHAN_NODE_ID multichan_node_id[4];
  multichan_node_id[0].node.nodeId = 3;
  multichan_node_id[0].node.endpoint = 0;
  multichan_node_id[0].nodeInfo.security = SECURITY_KEY_S2_AUTHENTICATED;
  multichan_node_id[1].node.nodeId = 4;
  multichan_node_id[1].node.endpoint = 0;
  multichan_node_id[1].nodeInfo.security = SECURITY_KEY_S2_AUTHENTICATED;
  multichan_node_id[2].node.nodeId = 5;
  multichan_node_id[2].node.endpoint = 0;
  multichan_node_id[2].nodeInfo.security = SECURITY_KEY_S2_AUTHENTICATED;
  multichan_node_id[3].node.nodeId = 6;
  multichan_node_id[3].node.endpoint = 0;
  multichan_node_id[3].nodeInfo.security = SECURITY_KEY_S2_AUTHENTICATED;

  TRANSMIT_OPTIONS_TYPE_EX nodeList;
  nodeList.pList = multichan_node_id;
  nodeList.list_length = sizeof_array(multichan_node_id);
  nodeList.sourceEndpoint = 0;
  nodeList.txOptions = 0;
  nodeList.S2_groupID = MULTICAST_GROUP_ID;

  /*
   * SETUP MOCKS
   */
  mock_call_expect(TO_STR(AssociationGetSinglecastNodeCount), &pMock);
  pMock->return_code.v = nodeList.list_length;

  mock_call_expect(TO_STR(GetHighestSecureLevel), &pMock);
  pMock->expect_arg[0].v = TEST_SECURITY_KEY_BITS;
  pMock->return_code.v = SECURITY_KEY_NONE;

  mock_call_expect(TO_STR(QueueNotifyingSendToBack), &pMock);
  static SZwaveTransmitPackage FramePackage;
  FramePackage.eTransmitType = EZWAVETRANSMITTYPE_MULTI_EX;
  FramePackage.uTransmitParams.SendDataMultiEx.FrameConfig.TransmitOptions = 0;
  FramePackage.uTransmitParams.SendDataMultiEx.FrameConfig.iFrameLength = DATA_SIZE;
  FramePackage.uTransmitParams.SendDataMultiEx.GroupId = nodeList.S2_groupID;
  memcpy(FramePackage.uTransmitParams.SendDataMultiEx.FrameConfig.aFrame, someData, DATA_SIZE);

  pMock->expect_arg[0].p = pAppHandles->pZwTxQueue;
  pMock->expect_arg[1].p = &FramePackage;
  pMock->expect_arg[2].v = 0;
  pMock->return_code.v = 0; // 0 = EQUEUENOTIFYING_STATUS_SUCCESS

  /*
   * CALL THE FUNCTION UNDER TEST
   */
  enum ETRANSPORT_MULTICAST_STATUS result;
  result = ZW_TransportMulticast_SendRequest(
      someData,
      DATA_SIZE,
      false,
      &nodeList,
      NULL);

  TEST_ASSERT_EQUAL_UINT32_MESSAGE(1, result, "Wrong return value from ZW_TransportMulticast_SendRequest"); // 1 = ETRANSPORTMULTICAST_ADDED_TO_QUEUE

  free((void *) pAppHandles->pNetworkInfo);
  free(pAppHandles->pZwTxQueue);
  free(pAppHandles);

  mock_calls_verify();
}

void test_TO6768_multicast_nonsecure(void)
{
  mock_t * pMock = NULL;

  SApplicationHandles * pAppHandles = GetApplicationHandles(0);
  ZAF_setAppHandle(pAppHandles);

  /*
   * PREPARE INPUTS
   */
  uint8_t dataToSend[20];

  TRANSMIT_OPTIONS_TYPE_EX nodeList;

  MULTICHAN_NODE_ID multichan_node_id[4];
  memset((uint8_t *)multichan_node_id, 0x00, sizeof(multichan_node_id));
  multichan_node_id[0].node.nodeId = 3;
  multichan_node_id[0].node.endpoint = 0;
  multichan_node_id[0].nodeInfo.security = SECURITY_KEY_NONE;
  multichan_node_id[1].node.nodeId = 4;
  multichan_node_id[1].node.endpoint = 0;
  multichan_node_id[1].nodeInfo.security = SECURITY_KEY_NONE;
  multichan_node_id[2].node.nodeId = 5;
  multichan_node_id[2].node.endpoint = 0;
  multichan_node_id[2].nodeInfo.security = SECURITY_KEY_NONE;
  multichan_node_id[3].node.nodeId = 6;
  multichan_node_id[3].node.endpoint = 0;
  multichan_node_id[3].nodeInfo.security = SECURITY_KEY_NONE;

  nodeList.pList = multichan_node_id;
  nodeList.list_length = sizeof_array(multichan_node_id);
  nodeList.sourceEndpoint = 0;
  nodeList.txOptions = 0;

  printf("\nList length: %u", nodeList.list_length);

  /*
   * SETUP MOCKS
   */
  mock_call_use_as_stub(TO_STR(AssociationGetBitAdressingDestination));
  mock_call_use_as_stub(TO_STR(GetHighestSecureLevel));
  mock_call_use_as_stub(TO_STR(AssociationGetDestinationInit));

  mock_call_expect(TO_STR(AssociationGetSinglecastNodeCount), &pMock);
  pMock->return_code.v = nodeList.list_length;

  for (uint32_t i = 0; i < nodeList.list_length; i++)
  {
    mock_call_expect(TO_STR(AssociationGetNextSinglecastDestination), &pMock);
    pMock->return_code.p = &multichan_node_id[i];
  }

  mock_call_expect(TO_STR(QueueNotifyingSendToBack), &pMock);
  static SZwaveTransmitPackage FramePackage;
  FramePackage.eTransmitType = EZWAVETRANSMITTYPE_MULTI;
  FramePackage.uTransmitParams.SendDataMultiEx.FrameConfig.iFrameLength = sizeof(dataToSend);
  memcpy(FramePackage.uTransmitParams.SendDataMultiEx.FrameConfig.aFrame, dataToSend, sizeof(dataToSend));

  pMock->expect_arg[0].p = pAppHandles->pZwTxQueue;
  pMock->expect_arg[1].p = &FramePackage;
  pMock->expect_arg[2].v = 0;
  pMock->return_code.v = 0; // 0 = EQUEUENOTIFYING_STATUS_SUCCESS


  /*
   * CALL THE FUNCTION UNDER TEST
   */
  enum ETRANSPORT_MULTICAST_STATUS result;
  result = ZW_TransportMulticast_SendRequest(
      dataToSend,
      sizeof(dataToSend),
      false,
      &nodeList,
      NULL);

  TEST_ASSERT_EQUAL_UINT32_MESSAGE(1, result, "Wrong return value from ZW_TransportMulticast_SendRequest"); // 1 = ETRANSPORTMULTICAST_ADDED_TO_QUEUE

  free((void *) pAppHandles->pNetworkInfo);
  free(pAppHandles->pZwTxQueue);
  free(pAppHandles);

  mock_calls_verify();
}

static SApplicationHandles * GetApplicationHandles(uint8_t grantedKeys)
{
  SApplicationHandles * pHandles = (SApplicationHandles *)malloc(sizeof(SApplicationHandles));

  SNetworkInfo * pNetworkInfo = (SNetworkInfo *)malloc(sizeof(SNetworkInfo));
  pNetworkInfo->SecurityKeys = grantedKeys;

  pHandles->pNetworkInfo = pNetworkInfo;

  SQueueNotifying * pTxQueue = (SQueueNotifying *)malloc(sizeof(SQueueNotifying));
  pHandles->pZwTxQueue = pTxQueue;

  return pHandles;
}

/*
 * Verifies that ZAF_Transmit is invoked with TX single options containing bit addressing
 * to two endpoints.
 */
void test_MultiChannelWithBitAddressing(void)
{
  mock_t * pMock;

  howManyTimesWasCallbackCalled = 0;

  const uint8_t SOURCE_ENDPOINT = 1;

  const uint8_t frame[] = {0xAA, 0xBB, 0xCC};

  const destination_info_t NODES[] = {
                                      {{10, 1, 0}, {1, 0}},
                                      {{10, 2, 0}, {1, 0}}
  };

  TRANSMIT_OPTIONS_TYPE_EX txOptions = {
                                        .S2_groupID = 0,
                                        .txOptions = 0,
                                        .sourceEndpoint = SOURCE_ENDPOINT,
                                        .pList = (destination_info_t *)NODES,
                                        .list_length = sizeof_array(NODES)
  };

  mock_call_use_as_stub(TO_STR(AssociationGetDestinationInit));

  SApplicationHandles * pAppHandles = GetApplicationHandles(0);
  ZAF_setAppHandle(pAppHandles);

  const destination_info_t DESTINATION_WITH_BIT_ADDRESSING = {
                                                              {10, 3, 1}, {0, 0}
  };

  mock_call_expect(TO_STR(AssociationGetBitAdressingDestination), &pMock);
  pMock->expect_arg[0].p = (void *)&(txOptions.pList);
  pMock->output_arg[0].p = NULL;
  pMock->compare_rule_arg[1] = COMPARE_NOT_NULL;
  uint8_t remainingNodes = 0;
  pMock->output_arg[1].p = &remainingNodes;
  pMock->compare_rule_arg[2] = COMPARE_NOT_NULL;
  pMock->output_arg[2].p = (void *)&DESTINATION_WITH_BIT_ADDRESSING;
  pMock->return_code.v = false;

  mock_call_expect(TO_STR(ZAF_Transmit), &pMock);
  pMock->expect_arg[0].p = (uint8_t *)frame;
  pMock->expect_arg[1].v = sizeof(frame);
  const TRANSMIT_OPTIONS_TYPE_SINGLE_EX txOptionsSingle = {
                                                     .txOptions = 0,
                                                     .txSecOptions = 0,
                                                     .sourceEndpoint = SOURCE_ENDPOINT,
                                                     .pDestNode = (destination_info_t *)&DESTINATION_WITH_BIT_ADDRESSING
  };
  pMock->expect_arg[2].p = (void *)&txOptionsSingle;
  pMock->compare_rule_arg[2] = COMPARE_NOT_NULL;
  pMock->compare_rule_arg[3] = COMPARE_NOT_NULL;
  pMock->return_code.v = EQUEUENOTIFYING_STATUS_SUCCESS;

  enum ETRANSPORT_MULTICAST_STATUS status;

  status = ZW_TransportMulticast_SendRequest(frame, sizeof(frame), false, &txOptions, multicast_callback);

  TEST_ASSERT(ETRANSPORTMULTICAST_ADDED_TO_QUEUE == status);

  ZAF_TX_Callback_t callback;
  callback = pMock->actual_arg[3].p;

  mock_call_expect(TO_STR(AssociationGetSinglecastNodeCount), &pMock);
  pMock->return_code.v = 0;

  ExpectedTXResult.isFinished = TRANSMISSION_RESULT_FINISHED;
  ExpectedTXResult.nodeId = DESTINATION_WITH_BIT_ADDRESSING.node.nodeId;
  ExpectedTXResult.status = TRANSMIT_COMPLETE_OK;

  // Invoke callback
  transmission_result_t result;
  result.isFinished = TRANSMISSION_RESULT_FINISHED;
  result.nodeId = 0;
  result.status = TRANSMIT_COMPLETE_OK;
  callback(&result);

  TEST_ASSERT_EQUAL_UINT8(1, howManyTimesWasCallbackCalled);

  free((void *) pAppHandles->pNetworkInfo);
  free(pAppHandles->pZwTxQueue);
  free(pAppHandles);

  mock_calls_verify();
}

/*
 * Verifies that ZAF_Transmit is invoked with TX single options containing bit addressing
 * to two endpoints.
 */
void test_MultiChannelWithBitAddressing_no_ack(void)
{
  mock_t *pMock;

  howManyTimesWasCallbackCalled = 0;

  const uint8_t SOURCE_ENDPOINT = 1;

  const uint8_t frame[] = {0xAA, 0xBB, 0xCC};

  const destination_info_t NODES[] = {
                                      {{10, 1, 0}, {1, 0}},
                                      {{10, 2, 0}, {1, 0}}
  };

  TRANSMIT_OPTIONS_TYPE_EX txOptions = {
                                        .S2_groupID = 0,
                                        .txOptions = 0,
                                        .sourceEndpoint = SOURCE_ENDPOINT,
                                        .pList = (destination_info_t *)NODES,
                                        .list_length = sizeof_array(NODES)
  };

  mock_call_use_as_stub(TO_STR(AssociationGetDestinationInit));

  // Notice the security keys because the flow depends on those.
  SApplicationHandles * pAppHandles = GetApplicationHandles(0);
  ZAF_setAppHandle(pAppHandles);

  // The 3 here is a bit-addressed endpoint for endpoint 2 and 1 above in the NODES-array.
  const destination_info_t DESTINATION_WITH_BIT_ADDRESSING = {
                                                              {10, 3, 1}, {0, 0}
  };

  mock_call_expect(TO_STR(AssociationGetBitAdressingDestination), &pMock);
  pMock->expect_arg[0].p = (void *)&(txOptions.pList);
  pMock->compare_rule_arg[1] = COMPARE_NOT_NULL;
  pMock->compare_rule_arg[2] = COMPARE_NOT_NULL;
  pMock->output_arg[0].p = NULL;
  uint8_t remainingNodes = 0;
  pMock->output_arg[1].p = &remainingNodes;
  pMock->output_arg[2].p = (void *)&DESTINATION_WITH_BIT_ADDRESSING;
  pMock->return_code.v = false;

  mock_call_expect(TO_STR(ZAF_Transmit), &pMock);
  pMock->expect_arg[0].p = (uint8_t *)frame;
  pMock->expect_arg[1].v = sizeof(frame);

  const TRANSMIT_OPTIONS_TYPE_SINGLE_EX txOptionsSingle = {
                                                     .txOptions = 0,
                                                     .txSecOptions = 0,
                                                     .sourceEndpoint = SOURCE_ENDPOINT,
                                                     .pDestNode = (destination_info_t *)&DESTINATION_WITH_BIT_ADDRESSING
  };

  pMock->expect_arg[2].p = (void *)&txOptionsSingle;
  pMock->compare_rule_arg[2] = COMPARE_NOT_NULL;
  pMock->compare_rule_arg[3] = COMPARE_NOT_NULL;
  pMock->return_code.v = EQUEUENOTIFYING_STATUS_SUCCESS;

  enum ETRANSPORT_MULTICAST_STATUS status;

  status = ZW_TransportMulticast_SendRequest(frame, sizeof(frame), false, &txOptions, multicast_callback);

  TEST_ASSERT(ETRANSPORTMULTICAST_ADDED_TO_QUEUE == status);

  ExpectedTXResult.isFinished = TRANSMISSION_RESULT_FINISHED;
  ExpectedTXResult.nodeId = DESTINATION_WITH_BIT_ADDRESSING.node.nodeId;
  ExpectedTXResult.status = TRANSMIT_COMPLETE_NO_ACK;

  // Invoke callback (multicast_callback)
  ZAF_TX_Callback_t callback;
  callback = pMock->actual_arg[3].p;  // Get the actual callback pointer received by the ZAF_Transmit() function.
  transmission_result_t result;
  result.isFinished = TRANSMISSION_RESULT_FINISHED;
  result.nodeId = 0;
  result.status = TRANSMIT_COMPLETE_NO_ACK;

  mock_call_expect(TO_STR(AssociationGetSinglecastNodeCount), &pMock);
  pMock->return_code.v = 0;  // Make AssociationGetSinglecastNodeCount() return 0.

  mock_call_expect(TO_STR(AssociationGetSinglecastNodeCount), &pMock);
  pMock->return_code.v = 0;  // Make AssociationGetSinglecastNodeCount() return 0.

  callback(&result);

  TEST_ASSERT_EQUAL_UINT8_MESSAGE(2, howManyTimesWasCallbackCalled, "Callback invoke count incorrect");

  free((void *) pAppHandles->pNetworkInfo);
  free(pAppHandles->pZwTxQueue);
  free(pAppHandles);

  mock_calls_verify();
}

/**
 * Verifies the successful flow of
 * 1. A Multi Channel Encapsulated frame to bit addressing destination 10.3
 * 2. A Multi Channel Encapsulated frame to bit addressing destination 11.3
 * 3. An S2 multicast frame
 * 4. A singlecast follow-up frame to destination 12
 * 5. A singlecast follow-up frame to destination 13
 */
void test_S2_MultiChannel_Multicast_Singlecast(void)
{
  mock_t * pMock;
  mock_t * pMockSendRequest;

  howManyTimesWasCallbackCalled = 0;

  mock_call_use_as_stub(TO_STR(GetHighestSecureLevel));
  mock_call_use_as_stub(TO_STR(AssociationGetDestinationInit));

  // Notice the security keys because the flow depends on those.
  SApplicationHandles * pAppHandles = GetApplicationHandles(SECURITY_KEY_S2_UNAUTHENTICATED_BIT);
  ZAF_setAppHandle(pAppHandles);

  ZAF_TX_Callback_t callback;

  const uint8_t SOURCE_ENDPOINT = 0;

  const uint8_t frame[] = {0xAA, 0xBB, 0xCC};

  const destination_info_t NODES[] = {
                                      {{10, 1, 0}, {1, 0}},
                                      {{10, 2, 0}, {1, 0}},
                                      {{11, 1, 0}, {1, 0}},
                                      {{11, 2, 0}, {1, 0}},
                                      {{12, 0, 0}, {0, 1}}, // S2
                                      {{13, 0, 0}, {0, 1}}  // S2
  };
  uint8_t remainingNodeCount = sizeof_array(NODES);

  TRANSMIT_OPTIONS_TYPE_EX txOptions = {
                                        .S2_groupID = 0,
                                        .txOptions = 0,
                                        .sourceEndpoint = SOURCE_ENDPOINT,
                                        .pList = (destination_info_t *)NODES,
                                        .list_length = sizeof_array(NODES)
  };

  const destination_info_t DESTINATION_WITH_BIT_ADDRESSING[] = {
                                                                {{10, 3, 1}, {0, 0}},
                                                                {{11, 3, 1}, {0, 0}}
  };

  mock_call_expect(TO_STR(AssociationGetBitAdressingDestination), &pMock);
  pMock->expect_arg[0].p = (void *)&(txOptions.pList);
  pMock->output_arg[0].p = (void *)&NODES[2];
  pMock->compare_rule_arg[1] = COMPARE_NOT_NULL;
  // Subtract the 2 associations that become a bit addressing destination.
  remainingNodeCount -= 2;
  pMock->output_arg[1].p = &remainingNodeCount;
  pMock->compare_rule_arg[2] = COMPARE_NOT_NULL;
  pMock->output_arg[2].p = (void *)&DESTINATION_WITH_BIT_ADDRESSING[0];
  pMock->return_code.v = true;

  mock_call_expect(TO_STR(ZAF_Transmit), &pMockSendRequest);
  pMockSendRequest->expect_arg[0].p = (uint8_t *)frame;
  pMockSendRequest->expect_arg[1].v = sizeof(frame);
  TRANSMIT_OPTIONS_TYPE_SINGLE_EX txOptionsSingle;
  txOptionsSingle.txOptions = 0;
  txOptionsSingle.txSecOptions = 0;
  txOptionsSingle.sourceEndpoint = SOURCE_ENDPOINT;
  txOptionsSingle.pDestNode = (destination_info_t *)&DESTINATION_WITH_BIT_ADDRESSING[0];
  pMockSendRequest->expect_arg[2].p = (void *)&txOptionsSingle;
  pMockSendRequest->compare_rule_arg[2] = COMPARE_NOT_NULL;
  pMockSendRequest->compare_rule_arg[3] = COMPARE_NOT_NULL;
  pMockSendRequest->return_code.v = EQUEUENOTIFYING_STATUS_SUCCESS;

  enum ETRANSPORT_MULTICAST_STATUS status;

  status = ZW_TransportMulticast_SendRequest(frame, sizeof(frame), false, &txOptions, multicast_callback);

  TEST_ASSERT(ETRANSPORTMULTICAST_ADDED_TO_QUEUE == status);

  /************************************************************************************************
   * Set mock expectations and invoke first callback
   *
   * Since we have two multi channel bit addressing destinations, we expect another call to
   * - AssociationGetBitAdressingDestination()
   * - ZAF_Transmit()
   ************************************************************************************************
   */

  callback = pMockSendRequest->actual_arg[3].p;

  mock_call_expect(TO_STR(AssociationGetBitAdressingDestination), &pMock);
  pMock->expect_arg[0].p = (void *)&(txOptions.pList);
  pMock->output_arg[0].p = (void *)&NODES[4];
  pMock->compare_rule_arg[1] = COMPARE_NOT_NULL;
  // Subtract another 2 that becomes a bit addressing destination.
  remainingNodeCount -= 2;
  pMock->output_arg[1].p = &remainingNodeCount;
  pMock->compare_rule_arg[2] = COMPARE_NOT_NULL;
  pMock->output_arg[2].p = (void *)&DESTINATION_WITH_BIT_ADDRESSING[1];
  pMock->return_code.v = false;

  mock_call_expect(TO_STR(ZAF_Transmit), &pMockSendRequest);
  pMockSendRequest->expect_arg[0].p = (uint8_t *)frame;
  pMockSendRequest->expect_arg[1].v = sizeof(frame);
  txOptionsSingle.txOptions = 0;
  txOptionsSingle.txSecOptions = 0;
  txOptionsSingle.sourceEndpoint = SOURCE_ENDPOINT;
  txOptionsSingle.pDestNode = (destination_info_t *)&DESTINATION_WITH_BIT_ADDRESSING[1];
  pMockSendRequest->expect_arg[2].p = (void *)&txOptionsSingle;
  pMockSendRequest->compare_rule_arg[2] = COMPARE_NOT_NULL;
  pMockSendRequest->compare_rule_arg[3] = COMPARE_NOT_NULL;
  pMockSendRequest->return_code.v = EQUEUENOTIFYING_STATUS_SUCCESS;

  ExpectedTXResult.isFinished = TRANSMISSION_RESULT_NOT_FINISHED;
  ExpectedTXResult.nodeId = 0;
  ExpectedTXResult.status = TRANSMIT_COMPLETE_OK;

  transmission_result_t result;
  result.isFinished = TRANSMISSION_RESULT_NOT_FINISHED;
  result.nodeId = 0;
  result.status = TRANSMIT_COMPLETE_OK;
  callback(&result);

  // Expect no callbacks because we're only invoking the callback on the singlecast follow up.
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(0, howManyTimesWasCallbackCalled, "Callback invoke count incorrect");

  /************************************************************************************************
   * Set mock expectations and invoke second callback
   *
   * We expect the multicast frame to be enqueued.
   ************************************************************************************************
   */

  callback = pMockSendRequest->actual_arg[3].p;

  mock_call_expect(TO_STR(AssociationGetSinglecastNodeCount), &pMock);
  pMock->return_code.v = 2; // Remaining nodes after the bit addressing nodes are processed.

  mock_call_expect(TO_STR(AssociationGetSinglecastNodeCount), &pMock);
  pMock->return_code.v = 2; // Remaining nodes after the bit addressing nodes are processed.

  mock_call_expect(TO_STR(QueueNotifyingSendToBack), &pMock);
  pMock->compare_rule_arg[0] = COMPARE_ANY;
  pMock->compare_rule_arg[1] = COMPARE_ANY;
  pMock->compare_rule_arg[2] = COMPARE_ANY;

  ExpectedTXResult.isFinished = TRANSMISSION_RESULT_NOT_FINISHED;
  ExpectedTXResult.nodeId = 0;
  ExpectedTXResult.status = TRANSMIT_COMPLETE_OK;

  result.isFinished = TRANSMISSION_RESULT_NOT_FINISHED;
  result.nodeId = 0;
  result.status = TRANSMIT_COMPLETE_OK;
  callback(&result);

  // Expect no callbacks because we're only invoking the callback on the singlecast follow up.
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(0, howManyTimesWasCallbackCalled, "Callback invoke count incorrect");

  free((void *) pAppHandles->pNetworkInfo);
  free(pAppHandles->pZwTxQueue);
  free(pAppHandles);

  mock_calls_verify();
}
