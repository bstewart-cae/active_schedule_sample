// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file TestZW_transport.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include "ZW_transport.h"
#include "mock_control.h"
#include "ZW_security_api.h"
#include "ZW_protocol.h"
#include "ZW_explore.h"
#include "ZW_ismyframe.h"
#include "ZW_node.h"

void setUpSuite(void) {

}

void tearDownSuite(void) {

}

void setUp(void) {

}

void tearDown(void) {

}

bool      g_learnModeDsk = false;

uint8_t   g_sucNodeID = 0;
uint8_t   g_Dsk[16];
uint8_t   homeID[HOMEID_LENGTH];

exploreQueueElementStruct *pExploreQueueElement;

extern uint16_t ZW_GetMaxPayloadSize(uint8_t keys);
extern bool IsRouteValid(ZW_HeaderFormatType_t headerFormatType, const frame *pFrame);

void test_IsRouteValid_3ch(void)
{
  frame aFrame3CH[15] = {{.singlecastRouted3ch.numRepsNumHops = 0x41, .singlecastRouted3ch.repeaterList = {1, 2, 3, 1}, .singlecastRouted3ch.routeStatus = 0},
                         {.singlecastRouted3ch.numRepsNumHops = 0x41, .singlecastRouted3ch.repeaterList = {1, 2, 0, 4}, .singlecastRouted3ch.routeStatus = 0},
                         {.singlecastRouted3ch.numRepsNumHops = 0x41, .singlecastRouted3ch.repeaterList = {1, 0, 3, 4}, .singlecastRouted3ch.routeStatus = 0},
                         {.singlecastRouted3ch.numRepsNumHops = 0x41, .singlecastRouted3ch.repeaterList = {0, 2, 3, 4}, .singlecastRouted3ch.routeStatus = 0},
                         {.singlecastRouted3ch.numRepsNumHops = 0x41, .singlecastRouted3ch.repeaterList = {1, 2, 3, 0}, .singlecastRouted3ch.routeStatus = 0},
                         {.singlecastRouted3ch.numRepsNumHops = 0x41, .singlecastRouted3ch.repeaterList = {1, 2, 3, 4}, .singlecastRouted3ch.routeStatus = 0},
                         {.singlecastRouted3ch.numRepsNumHops = 0x21, .singlecastRouted3ch.repeaterList = {1, 2, 0, 0}, .singlecastRouted3ch.routeStatus = 0},
                         {.singlecastRouted3ch.numRepsNumHops = 0x21, .singlecastRouted3ch.repeaterList = {0, 2, 0, 0}, .singlecastRouted3ch.routeStatus = 0},
                         {.singlecastRouted3ch.numRepsNumHops = 0x12, .singlecastRouted3ch.repeaterList = {1, 2, 0, 0}, .singlecastRouted3ch.routeStatus = 0},
                         {.singlecastRouted3ch.numRepsNumHops = 0x12, .singlecastRouted3ch.repeaterList = {1, 2, 0, 0}, .singlecastRouted3ch.routeStatus = MASK_DIRECTION},
                         {.singlecastRouted3ch.numRepsNumHops = 0x11, .singlecastRouted3ch.repeaterList = {1, 0, 0, 0}, .singlecastRouted3ch.routeStatus = MASK_DIRECTION},
                         {.singlecastRouted3ch.numRepsNumHops = 0x1F, .singlecastRouted3ch.repeaterList = {1, 2, 0, 0}, .singlecastRouted3ch.routeStatus = MASK_DIRECTION},
                         {.singlecastRouted3ch.numRepsNumHops = 0x15, .singlecastRouted3ch.repeaterList = {1, 2, 0, 0}, .singlecastRouted3ch.routeStatus = MASK_DIRECTION},
                         {.singlecastRouted3ch.numRepsNumHops = 0x00, .singlecastRouted3ch.repeaterList = {1, 2, 0, 0}, .singlecastRouted3ch.routeStatus = MASK_DIRECTION},
                         {.singlecastRouted3ch.numRepsNumHops = 0x52, .singlecastRouted3ch.repeaterList = {1, 2, 0, 0}, .singlecastRouted3ch.routeStatus = MASK_DIRECTION}
                        };
  // 3 Channel
  uint16_t valid = IsRouteValid(HDRFORMATTYP_3CH, &aFrame3CH[0]);
  TEST_ASSERT_EQUAL_UINT(0, valid);

  valid = IsRouteValid(HDRFORMATTYP_3CH, &aFrame3CH[1]);
  TEST_ASSERT_EQUAL_UINT(0, valid);

  valid = IsRouteValid(HDRFORMATTYP_3CH, &aFrame3CH[2]);
  TEST_ASSERT_EQUAL_UINT(0, valid);

  valid = IsRouteValid(HDRFORMATTYP_3CH, &aFrame3CH[3]);
  TEST_ASSERT_EQUAL_UINT(0, valid);

  valid = IsRouteValid(HDRFORMATTYP_3CH, &aFrame3CH[4]);
  TEST_ASSERT_EQUAL_UINT(0, valid);

  valid = IsRouteValid(HDRFORMATTYP_3CH, &aFrame3CH[5]);
  TEST_ASSERT_EQUAL_UINT(1, valid);

  valid = IsRouteValid(HDRFORMATTYP_3CH, &aFrame3CH[6]);
  TEST_ASSERT_EQUAL_UINT(1, valid);

  valid = IsRouteValid(HDRFORMATTYP_3CH, &aFrame3CH[7]);
  TEST_ASSERT_EQUAL_UINT(0, valid);

  valid = IsRouteValid(HDRFORMATTYP_3CH, &aFrame3CH[8]);
  TEST_ASSERT_EQUAL_UINT(0, valid);

  valid = IsRouteValid(HDRFORMATTYP_3CH, &aFrame3CH[9]);
  TEST_ASSERT_EQUAL_UINT(0, valid);

  valid = IsRouteValid(HDRFORMATTYP_3CH, &aFrame3CH[10]);
  TEST_ASSERT_EQUAL_UINT(0, valid);

  valid = IsRouteValid(HDRFORMATTYP_3CH, &aFrame3CH[11]);
  TEST_ASSERT_EQUAL_UINT(1, valid);

  valid = IsRouteValid(HDRFORMATTYP_3CH, &aFrame3CH[12]);
  TEST_ASSERT_EQUAL_UINT(0, valid);

  valid = IsRouteValid(HDRFORMATTYP_3CH, &aFrame3CH[13]);
  TEST_ASSERT_EQUAL_UINT(0, valid);

  valid = IsRouteValid(HDRFORMATTYP_3CH, &aFrame3CH[14]);
  TEST_ASSERT_EQUAL_UINT(0, valid);
}

void test_IsRouteValid_2ch(void)
{
  frameTx aFrameTx2CH[12] = {{.singlecastRouted.numRepsNumHops = 0x41, .singlecastRouted.repeaterList = {1, 2, 3, 1}, .singlecastRouted.routeStatus = 0},
                             {.singlecastRouted.numRepsNumHops = 0x41, .singlecastRouted.repeaterList = {1, 2, 0, 4}, .singlecastRouted.routeStatus = 0},
                             {.singlecastRouted.numRepsNumHops = 0x41, .singlecastRouted.repeaterList = {1, 2, 3, 4}, .singlecastRouted.routeStatus = 0},
                             {.singlecastRouted.numRepsNumHops = 0x21, .singlecastRouted.repeaterList = {1, 2, 0, 0}, .singlecastRouted.routeStatus = 0},
                             {.singlecastRouted.numRepsNumHops = 0x21, .singlecastRouted.repeaterList = {0, 2, 0, 0}, .singlecastRouted.routeStatus = 0},
                             {.singlecastRouted.numRepsNumHops = 0x12, .singlecastRouted.repeaterList = {1, 2, 0, 0}, .singlecastRouted.routeStatus = 0},
                             {.singlecastRouted.numRepsNumHops = 0x12, .singlecastRouted.repeaterList = {1, 2, 0, 0}, .singlecastRouted.routeStatus = MASK_DIRECTION},
                             {.singlecastRouted.numRepsNumHops = 0x11, .singlecastRouted.repeaterList = {1, 0, 0, 0}, .singlecastRouted.routeStatus = MASK_DIRECTION},
                             {.singlecastRouted.numRepsNumHops = 0x1F, .singlecastRouted.repeaterList = {1, 2, 0, 0}, .singlecastRouted.routeStatus = MASK_DIRECTION},
                             {.singlecastRouted.numRepsNumHops = 0x15, .singlecastRouted.repeaterList = {1, 2, 0, 0}, .singlecastRouted.routeStatus = MASK_DIRECTION},
                             {.singlecastRouted.numRepsNumHops = 0x00, .singlecastRouted.repeaterList = {1, 2, 0, 0}, .singlecastRouted.routeStatus = MASK_DIRECTION},
                             {.singlecastRouted.numRepsNumHops = 0x52, .singlecastRouted.repeaterList = {1, 2, 0, 0}, .singlecastRouted.routeStatus = MASK_DIRECTION}
                            };
  // 3 Channel
  uint16_t valid = IsRouteValid(HDRFORMATTYP_2CH, (frame*)&aFrameTx2CH[0]);
  TEST_ASSERT_EQUAL_UINT(0, valid);

  valid = IsRouteValid(HDRFORMATTYP_2CH, (frame*)&aFrameTx2CH[1]);
  TEST_ASSERT_EQUAL_UINT(0, valid);

  valid = IsRouteValid(HDRFORMATTYP_2CH, (frame*)&aFrameTx2CH[2]);
  TEST_ASSERT_EQUAL_UINT(1, valid);

  valid = IsRouteValid(HDRFORMATTYP_2CH, (frame*)&aFrameTx2CH[3]);
  TEST_ASSERT_EQUAL_UINT(1, valid);

  valid = IsRouteValid(HDRFORMATTYP_2CH, (frame*)&aFrameTx2CH[4]);
  TEST_ASSERT_EQUAL_UINT(0, valid);

  valid = IsRouteValid(HDRFORMATTYP_2CH, (frame*)&aFrameTx2CH[5]);
  TEST_ASSERT_EQUAL_UINT(0, valid);

  valid = IsRouteValid(HDRFORMATTYP_2CH, (frame*)&aFrameTx2CH[6]);
  TEST_ASSERT_EQUAL_UINT(0, valid);

  valid = IsRouteValid(HDRFORMATTYP_2CH, (frame*)&aFrameTx2CH[7]);
  TEST_ASSERT_EQUAL_UINT(0, valid);

  valid = IsRouteValid(HDRFORMATTYP_2CH, (frame*)&aFrameTx2CH[8]);
  TEST_ASSERT_EQUAL_UINT(1, valid);

  valid = IsRouteValid(HDRFORMATTYP_2CH, (frame*)&aFrameTx2CH[9]);
  TEST_ASSERT_EQUAL_UINT(0, valid);

  valid = IsRouteValid(HDRFORMATTYP_2CH, (frame*)&aFrameTx2CH[10]);
  TEST_ASSERT_EQUAL_UINT(0, valid);

  valid = IsRouteValid(HDRFORMATTYP_2CH, (frame*)&aFrameTx2CH[11]);
  TEST_ASSERT_EQUAL_UINT(0, valid);
}

void test_ZW_GetMaxPayloadSize(void)
{
  mock_calls_clear();
  mock_t * pMock;

  uint16_t max_payload = 0;

  //Max payload of Long Range slave
  mock_call_expect(TO_STR(zpal_radio_is_long_range_locked), &pMock);
  pMock->return_code.value = true;

  max_payload = ZW_GetMaxPayloadSize(SECURITY_KEY_S2_AUTHENTICATED_BIT);
  TEST_ASSERT_EQUAL_INT16(MAX_SINGLECAST_PAYLOAD_LR - 12, max_payload); //S2_ENCAPSULATION_LENGTH 12

  mock_calls_verify();

  //Max payload of 3-channel Classic frames
  mock_calls_clear();
  mock_call_expect(TO_STR(zpal_radio_is_long_range_locked), &pMock);
  pMock->return_code.value = false;

  mock_call_expect(TO_STR(llIsHeaderFormat3ch), &pMock);
  pMock->return_code.value = true;

  max_payload = ZW_GetMaxPayloadSize(SECURITY_KEY_S2_AUTHENTICATED_BIT);
  TEST_ASSERT_EQUAL_INT16(MAX_EXPLORE_PAYLOAD_3CH - 12, max_payload); //S2_ENCAPSULATION_LENGTH 12

  mock_calls_verify();

  //Max payload of 2-channel Classic frames
  mock_calls_clear();
  mock_call_expect(TO_STR(zpal_radio_is_long_range_locked), &pMock);
  pMock->return_code.value = false;

  mock_call_expect(TO_STR(llIsHeaderFormat3ch), &pMock);
  pMock->return_code.value = false;

  max_payload = ZW_GetMaxPayloadSize(SECURITY_KEY_S2_AUTHENTICATED_BIT);
  TEST_ASSERT_EQUAL_INT16(MAX_EXPLORE_PAYLOAD_LEGACY - 12, max_payload); //S2_ENCAPSULATION_LENGTH 12

  mock_calls_verify();
}

void test_SendFragmentedBeamACK(void)
{
  mock_calls_clear();
  mock_t * pMock;

  const int8_t txPower = 10;
  const int8_t rssi    = -60;

  TxQueueElement ACKFrame;

  //Test for classic nodes
  mock_call_use_as_stub(TO_STR(ZwTimerRegister));
  mock_call_expect(TO_STR(TxQueueGetFreeElement), &pMock);
  pMock->return_code.pointer = &ACKFrame;
  pMock->expect_arg[ARG0].value = TX_QUEUE_PRIORITY_HIGH;

  mock_call_expect(TO_STR(BuildTxHeader), &pMock);
  pMock->expect_arg[ARG0].value = HDRFORMATTYP_3CH;
  pMock->expect_arg[ARG1].value = FRAME_TYPE_ACK;
  pMock->expect_arg[ARG2].value = 0;  //currentSeqNo
  pMock->expect_arg[ARG3].pointer = &ACKFrame;

  mock_call_expect(TO_STR(TxQueueQueueElement), &pMock);
  pMock->expect_arg[ARG0].pointer = &ACKFrame;

  g_nodeID = 2; //Classic node
  SendFragmentedBeamACK();

  TEST_ASSERT_EQUAL(NODE_BROADCAST, ACKFrame.frame.frameOptions.destinationNodeId);


  //Test for Long Range nodes
  mock_call_use_as_stub(TO_STR(ZwTimerRegister));
  mock_call_expect(TO_STR(TxQueueGetFreeElement), &pMock);
  pMock->return_code.pointer = &ACKFrame;
  pMock->expect_arg[ARG0].value = TX_QUEUE_PRIORITY_HIGH;

  mock_call_expect(TO_STR(BuildTxHeader), &pMock);
  pMock->expect_arg[ARG0].value = HDRFORMATTYP_LR;
  pMock->expect_arg[ARG1].value = FRAME_TYPE_ACK;
  pMock->expect_arg[ARG2].value = 0;  //currentSeqNo
  pMock->expect_arg[ARG3].pointer = &ACKFrame;

  mock_call_expect(TO_STR(zpal_radio_get_flirs_beam_tx_power), &pMock);
  pMock->return_code.value = txPower;

  mock_call_expect(TO_STR(zpal_radio_get_last_beam_rssi), &pMock);
  pMock->return_code.value = rssi;

  mock_call_expect(TO_STR(zpal_radio_get_flirs_beam_tx_power), &pMock);
  pMock->return_code.value = txPower;

  mock_call_expect(TO_STR(zpal_radio_get_last_beam_rssi), &pMock);
  pMock->return_code.value = rssi;

  mock_call_expect(TO_STR(SaveTxPowerAndRSSI), &pMock);
  pMock->expect_arg[ARG0].value = txPower;
  pMock->expect_arg[ARG1].value = rssi;

  mock_call_expect(TO_STR(TxQueueQueueElement), &pMock);
  pMock->expect_arg[ARG0].pointer = &ACKFrame;


  g_nodeID = 256; //Long Range node
  SendFragmentedBeamACK();

  TEST_ASSERT_EQUAL(NODE_BROADCAST_LR, ACKFrame.frame.frameOptions.destinationNodeId);

  mock_calls_verify();
}

extern void EnQueueCommon(uint8_t rfSpeed,
                          uint8_t frameType,
                          uint8_t const * const pData,
                          uint8_t  dataLength,
                          const STransmitCallback*,
                          TxQueueElement *pFreeTxElement);

extern uint8_t currentSeqNo;
extern bool currentSeqNoUseTX;
#define DATA_LENGTH   10
void test_EnQueueCommon(void)
{
  mock_calls_clear();
  mock_t * pMock;

  STransmitCallback CompletedFunc;

  const node_id_t destNodeID = 5;
  const uint8_t forceLR = 1;

  TxQueueElement TestTxQueueElement;
  TestTxQueueElement.frame.frameOptions.destinationNodeId = destNodeID;
  TestTxQueueElement.frame.header.header.headerInfo = 0;

  TestTxQueueElement.forceLR = forceLR;
  TestTxQueueElement.bFrameOptions = 0;
  TestTxQueueElement.bFrameOptions1 = 0;

  const uint8_t dataLength = DATA_LENGTH;
  uint8_t data[DATA_LENGTH];
  const uint8_t * pData = (const uint8_t *)&data;

  //Test that Sequence number is counted up from 0 to 255 for LR frames
  currentSeqNoUseTX = true;
  for(uint32_t i = 0; i<256; i++)
  {
    mock_call_expect(TO_STR(llGetCurrentHeaderFormat), &pMock);
    pMock->expect_arg[ARG0].value = destNodeID;
    pMock->expect_arg[ARG1].value = forceLR;
    pMock->return_code.value = HDRFORMATTYP_LR;

    mock_call_expect(TO_STR(TxQueueClearOptionFlags), &pMock);
    pMock->expect_arg[ARG0].pointer = &TestTxQueueElement;
    pMock->expect_arg[ARG1].value = TRANSMIT_OPTION_AUTO_ROUTE;

    mock_call_expect(TO_STR(TxQueueSetOptionFlags), &pMock);
    pMock->expect_arg[ARG0].pointer = &TestTxQueueElement;
    pMock->expect_arg[ARG1].value = TRANSMIT_OPTION_NO_ROUTE;

    mock_call_expect(TO_STR(TxQueueGetOptions), &pMock);
    pMock->expect_arg[ARG0].pointer = &TestTxQueueElement;
    pMock->return_code.value = 0;

    mock_call_expect(TO_STR(BuildTxHeader), &pMock);
    pMock->expect_arg[ARG0].value = HDRFORMATTYP_LR;
    pMock->expect_arg[ARG1].value = FRAME_TYPE_SINGLE;
    pMock->expect_arg[ARG2].value = i;  //Sequence Number
    pMock->expect_arg[ARG3].pointer = &TestTxQueueElement;

    mock_call_expect(TO_STR(TxQueueQueueElement), &pMock);
    pMock->expect_arg[ARG0].pointer = &TestTxQueueElement;

    EnQueueCommon( RF_SPEED_LR_100K,
                   FRAME_TYPE_SINGLE,
                   pData,
                   dataLength,
                   &CompletedFunc,
                   &TestTxQueueElement);
  }

  //Test that RX sequence number is used if currentSeqNoUseTX is false
  currentSeqNo = 55;
  currentSeqNoUseTX = false;

  mock_call_expect(TO_STR(llGetCurrentHeaderFormat), &pMock);
  pMock->expect_arg[ARG0].value = destNodeID;
  pMock->expect_arg[ARG1].value = forceLR;
  pMock->return_code.value = HDRFORMATTYP_LR;

  mock_call_expect(TO_STR(TxQueueClearOptionFlags), &pMock);
  pMock->expect_arg[ARG0].pointer = &TestTxQueueElement;
  pMock->expect_arg[ARG1].value = TRANSMIT_OPTION_AUTO_ROUTE;

  mock_call_expect(TO_STR(TxQueueSetOptionFlags), &pMock);
  pMock->expect_arg[ARG0].pointer = &TestTxQueueElement;
  pMock->expect_arg[ARG1].value = TRANSMIT_OPTION_NO_ROUTE;

  mock_call_expect(TO_STR(TxQueueGetOptions), &pMock);
  pMock->expect_arg[ARG0].pointer = &TestTxQueueElement;
  pMock->return_code.value = 0;

  mock_call_expect(TO_STR(BuildTxHeader), &pMock);
  pMock->expect_arg[ARG0].value = HDRFORMATTYP_LR;
  pMock->expect_arg[ARG1].value = FRAME_TYPE_SINGLE;
  pMock->expect_arg[ARG2].value = currentSeqNo;
  pMock->expect_arg[ARG3].pointer = &TestTxQueueElement;

  mock_call_expect(TO_STR(TxQueueQueueElement), &pMock);
  pMock->expect_arg[ARG0].pointer = &TestTxQueueElement;

  EnQueueCommon( RF_SPEED_LR_100K,
                 FRAME_TYPE_SINGLE,
                 pData,
                 dataLength,
                 &CompletedFunc,
                 &TestTxQueueElement);


  //Test that Sequence number is counted up from 1 to 15 for
  //2CH frames with RF_SPEED_40K
  currentSeqNoUseTX = true;
  uint8_t counter = 0;

  for (uint32_t i=0; i<=20; i++)
  {
    mock_call_expect(TO_STR(llGetCurrentHeaderFormat), &pMock);
    pMock->expect_arg[ARG0].value = destNodeID;
    pMock->expect_arg[ARG1].value = forceLR;
    pMock->return_code.value = HDRFORMATTYP_2CH;

    mock_call_expect(TO_STR(TxQueueGetOptions), &pMock);
    pMock->expect_arg[ARG0].pointer = &TestTxQueueElement;
    pMock->return_code.value = 0;

    mock_call_expect(TO_STR(TxQueueIsEmpty), &pMock);
    pMock->return_code.value = 0;

    //The counter value must be in the range 1-15
    counter++;
    if (16 == counter)
    {
      counter = 1;
    }
    //Set highest 4 bits of frameType to counter
    uint8_t frameType = (counter<<4) | FRAME_TYPE_SINGLE;

    mock_call_expect(TO_STR(BuildTxHeader), &pMock);
    pMock->expect_arg[ARG0].value = HDRFORMATTYP_2CH;
    pMock->expect_arg[ARG1].value = frameType;
    pMock->expect_arg[ARG2].value = 0;
    pMock->expect_arg[ARG3].pointer = &TestTxQueueElement;

    mock_call_expect(TO_STR(TxQueueQueueElement), &pMock);
    pMock->expect_arg[ARG0].pointer = &TestTxQueueElement;

    EnQueueCommon( RF_SPEED_40K,
                   FRAME_TYPE_SINGLE,
                   pData,
                   dataLength,
                   &CompletedFunc,
                   &TestTxQueueElement);
  }

  //Test that Sequence number is counted up from 1 to 15 for
  //2CH frames with RF_SPEED_9_6K
  currentSeqNoUseTX = true;
  counter = 0;

  for (uint32_t i=0; i<=20; i++)
  {
    mock_call_expect(TO_STR(llGetCurrentHeaderFormat), &pMock);
    pMock->expect_arg[ARG0].value = destNodeID;
    pMock->expect_arg[ARG1].value = forceLR;
    pMock->return_code.value = HDRFORMATTYP_2CH;

    mock_call_expect(TO_STR(TxQueueGetOptions), &pMock);
    pMock->expect_arg[ARG0].pointer = &TestTxQueueElement;
    pMock->return_code.value = 0;

    mock_call_expect(TO_STR(TxQueueIsEmpty), &pMock);
    pMock->return_code.value = 0;

    //The counter value must be in the range 1-15
    counter++;
    if (16 == counter)
    {
      counter = 1;
    }
    //Set highest 4 bits of frameType to counter
    uint8_t frameType = (counter<<4) | FRAME_TYPE_SINGLE;

    mock_call_expect(TO_STR(BuildTxHeader), &pMock);
    pMock->expect_arg[ARG0].value = HDRFORMATTYP_2CH;
    pMock->expect_arg[ARG1].value = frameType;
    pMock->expect_arg[ARG2].value = 0;
    pMock->expect_arg[ARG3].pointer = &TestTxQueueElement;

    mock_call_expect(TO_STR(TxQueueQueueElement), &pMock);
    pMock->expect_arg[ARG0].pointer = &TestTxQueueElement;

    EnQueueCommon( RF_SPEED_9_6K,
                   FRAME_TYPE_SINGLE,
                   pData,
                   dataLength,
                   &CompletedFunc,
                   &TestTxQueueElement);
  }

  //Test that Sequence number is counted up from 1 to 15 for
  //2CH frames with RF_SPEED_9_6K and FRAME_TYPE_ROUTED
  currentSeqNoUseTX = true;
  counter = 0;

  for (uint32_t i=0; i<=20; i++)
  {
    mock_call_expect(TO_STR(llGetCurrentHeaderFormat), &pMock);
    pMock->expect_arg[ARG0].value = destNodeID;
    pMock->expect_arg[ARG1].value = forceLR;
    pMock->return_code.value = HDRFORMATTYP_2CH;

    mock_call_expect(TO_STR(TxQueueGetOptions), &pMock);
    pMock->expect_arg[ARG0].pointer = &TestTxQueueElement;
    pMock->return_code.value = 0;

    mock_call_expect(TO_STR(TxQueueIsEmpty), &pMock);
    pMock->return_code.value = 0;

    //The counter value must be in the range 1-15
    counter++;
    if (16 == counter)
    {
      counter = 1;
    }
    //Set highest 4 bits of frameType to counter
    uint8_t frameType = (counter<<4) | FRAME_TYPE_ROUTED;

    mock_call_expect(TO_STR(BuildTxHeader), &pMock);
    pMock->expect_arg[ARG0].value = HDRFORMATTYP_2CH;
    pMock->expect_arg[ARG1].value = frameType;
    pMock->expect_arg[ARG2].value = 0;
    pMock->expect_arg[ARG3].pointer = &TestTxQueueElement;

    mock_call_expect(TO_STR(TxQueueQueueElement), &pMock);
    pMock->expect_arg[ARG0].pointer = &TestTxQueueElement;

    EnQueueCommon( RF_SPEED_9_6K,
                   FRAME_TYPE_ROUTED,
                   pData,
                   dataLength,
                   &CompletedFunc,
                   &TestTxQueueElement);
  }

  mock_calls_verify();
}

void test_TransportEnQueueExploreFrame(void)
{
  mock_calls_clear();
  mock_t * pMock;
  frameExploreStruct Frame;
  Frame.baseFrame.payloadLength = 50;
  Frame.baseFrame.frameOptions.sequenceNumber = 3;
  Frame.baseFrame.header.header.headerInfo = 0;

  TxOptions_t exploreTxOptions = 0x12345678; //dummy test value

  ZW_SendData_Callback_t  completedFunc = NULL;


  //Test failure to get free TxQueue element
  mock_call_expect(TO_STR(llIsHeaderFormat3ch), &pMock);
  pMock->return_code.value = 1;

  mock_call_expect(TO_STR(TxQueueGetFreeElement), &pMock);
  pMock->expect_arg[ARG0].value = TX_QUEUE_PRIORITY_LOW;
  pMock->expect_arg[ARG1].value = false;
  pMock->return_code.pointer = NULL;

  uint8_t retval = TransportEnQueueExploreFrame(&Frame, exploreTxOptions, completedFunc);

  TEST_ASSERT_EQUAL(false, retval);


  //Test that Sequence Number is counted up from 0 to 255 for 3CH frames
  TxQueueElement FreeTxQueueElement;
  for (uint32_t i=0; i<=260; i++)
  {
    mock_call_expect(TO_STR(llIsHeaderFormat3ch), &pMock);
    pMock->return_code.value = 1;  //3CH format

    mock_call_expect(TO_STR(TxQueueGetFreeElement), &pMock);
    pMock->expect_arg[ARG0].value = TX_QUEUE_PRIORITY_LOW;
    pMock->expect_arg[ARG1].value = false;
    pMock->return_code.pointer = &FreeTxQueueElement;

    mock_call_expect(TO_STR(TxQueueSetOptionFlags), &pMock);
    pMock->expect_arg[ARG0].pointer = &FreeTxQueueElement;
    pMock->expect_arg[ARG1].value = exploreTxOptions;

    mock_call_expect(TO_STR(BuildTxHeader), &pMock);
    pMock->expect_arg[ARG0].value = HDRFORMATTYP_3CH;
    pMock->expect_arg[ARG1].value = FRAME_TYPE_EXPLORE;
    pMock->expect_arg[ARG2].value = (i % 256);  //Sequence Number
    pMock->expect_arg[ARG3].pointer = &FreeTxQueueElement;

    mock_call_expect(TO_STR(TxQueueQueueElement), &pMock);
    pMock->expect_arg[ARG0].pointer = &FreeTxQueueElement;

    retval = TransportEnQueueExploreFrame(&Frame, exploreTxOptions, completedFunc);

    TEST_ASSERT_EQUAL(true, retval);
  }

  //Test that 4 highest bits of frameType is counted up from 1 to 15 for 2CH frames
  uint8_t counter = 0;

  for (uint32_t i=0; i<=20; i++)
  {
    mock_call_expect(TO_STR(llIsHeaderFormat3ch), &pMock);
    pMock->return_code.value = 0;  //2CH format

    mock_call_expect(TO_STR(TxQueueGetFreeElement), &pMock);
    pMock->expect_arg[ARG0].value = TX_QUEUE_PRIORITY_LOW;
    pMock->expect_arg[ARG1].value = false;
    pMock->return_code.pointer = &FreeTxQueueElement;

    mock_call_expect(TO_STR(TxQueueSetOptionFlags), &pMock);
    pMock->expect_arg[ARG0].pointer = &FreeTxQueueElement;
    pMock->expect_arg[ARG1].value = exploreTxOptions;

    //The counter value must be in the range 1-15
    counter++;
    if (16 == counter)
    {
      counter = 1;
    }
    //Set highest 4 bits of frameType to counter
    uint8_t frameType = (counter<<4) | FRAME_TYPE_EXPLORE;

    mock_call_expect(TO_STR(BuildTxHeader), &pMock);
    pMock->expect_arg[ARG0].value = HDRFORMATTYP_2CH;
    pMock->expect_arg[ARG1].value = frameType;
    pMock->expect_arg[ARG2].value = 0; //Not used for 2CH frames
    pMock->expect_arg[ARG3].pointer = &FreeTxQueueElement;

    mock_call_expect(TO_STR(TxQueueQueueElement), &pMock);
    pMock->expect_arg[ARG0].pointer = &FreeTxQueueElement;

    retval = TransportEnQueueExploreFrame(&Frame, exploreTxOptions, completedFunc);

    TEST_ASSERT_EQUAL(true, retval);
  }

  mock_calls_verify();
}
/*
Test that the extened bit in 3ch routed frame is sat correclty
*/
extern void SendRouteAck(RX_FRAME *pRx);

extern void SetTransmitPause(void);
extern uint8_t sACK;
extern TxQueueElement *pFrameWaitingForACK;
// test if the flag RECEIVE_DO_DELAY is set the transmit delay will depend on the frame speed
// 9.6 frames have a delay of MAC_TRANSMIT_DELAY_MS
// 40K, and 100K frames have a delay of MAC_TRANSMIT_DELAY_MS/4
void test_SetTransmitPause(void)
{
  mock_calls_clear();
  mock_t * pMock;

  TxQueueElement txElm = {0};
  pFrameWaitingForACK = &txElm;
  sACK = RECEIVE_DO_DELAY;

  // test 9.6k speed
  txElm.wRFoptions = RF_OPTION_SPEED_9600;

  mock_call_expect(TO_STR(llIsHeaderFormat3ch), &pMock);
  pMock->return_code.value = 0;  //2CH format

  mock_call_expect(TO_STR(TxQueueStartTransmissionPause), &pMock);
  pMock->expect_arg[ARG0].v = MAC_TRANSMIT_DELAY_MS;
  SetTransmitPause();


  // test 40k speed
  txElm.wRFoptions = RF_OPTION_SPEED_40K;

  mock_call_expect(TO_STR(llIsHeaderFormat3ch), &pMock);
  pMock->return_code.value = 0;  //2CH format

  mock_call_expect(TO_STR(TxQueueStartTransmissionPause), &pMock);
  pMock->expect_arg[ARG0].v = MAC_TRANSMIT_DELAY_MS/4;
  SetTransmitPause();

  // test 100k speed
  mock_call_expect(TO_STR(llIsHeaderFormat3ch), &pMock);
  pMock->return_code.value = 0;  //2CH format

  txElm.wRFoptions = RF_OPTION_SPEED_100K;

  mock_call_expect(TO_STR(TxQueueStartTransmissionPause), &pMock);
  pMock->expect_arg[ARG0].v = MAC_TRANSMIT_DELAY_MS/4;
  SetTransmitPause();

  // test 3CH speed
  mock_call_expect(TO_STR(llIsHeaderFormat3ch), &pMock);
  pMock->return_code.value = 1;  //3CH format

  mock_call_expect(TO_STR(TxQueueStartTransmissionPause), &pMock);
  pMock->expect_arg[ARG0].v = MAC_TRANSMIT_DELAY_MS/4;
  SetTransmitPause();

  // test if RECEIVE_DO_DELAY not set no delay used
  sACK = 0;
  SetTransmitPause();

  mock_calls_verify();
}

void test_SendRoutedAck_3ch(void)
{

  mock_calls_clear();
  mock_t * pMock;

  uint8_t rawFrame[] ={0xFB, 0x0A, 0x40, 0x96, 0x01, 0x08, 0x00, 0x11, 0x38, 0x03, 0x00, 0x11, 0x02, 0x00, 0x00, 0x90, 0x69};
  ZW_ReceiveFrame_t receivedFrame = {.channelHeaderFormat = HDRFORMATTYP_3CH,
                                     .channelId = 1,
                                     .frameContent = rawFrame,
                                     .frameContentLength = 17,
                                     .pPayloadStart = &rawFrame[14],
                                     .profile = RF_PROFILE_3CH_100K_CH_A,
                                     .frameOptions.frameType = HDRTYP_ROUTED,
                                     .frameOptions.homeId = {0xFB, 0x0A, 0x40, 0x96},
                                     .frameOptions.sourceNodeId = 0x01,
                                     .frameOptions.destinationNodeId = 3};
  RX_FRAME rxFrame;
  rxFrame.pReceiveFrame = &receivedFrame;
  rxFrame.bTotalLength = 15;
  rxFrame.payloadLength = 1;
  rxFrame.pFrame = (frame *)rawFrame;
  TxQueueElement elm;
  elm.forceLR = false;


  mock_call_expect(TO_STR(TxQueueGetFreeElement), &pMock);
  pMock->expect_arg[ARG0].value = TX_QUEUE_PRIORITY_LOW;
  pMock->expect_arg[ARG1].value = false;
  pMock->return_code.pointer = &elm;

  mock_call_expect(TO_STR(SlaveStorageGetNetworkIds), &pMock);
  pMock->expect_arg[ARG0].pointer = ZW_HomeIDGet();
  pMock->expect_arg[ARG1].pointer = NULL;
  pMock->output_arg[ARG0].pointer = rawFrame;

  mock_call_expect(TO_STR(TxQueueInitOptions), &pMock);
  pMock->expect_arg[ARG0].pointer = &elm;
  pMock->expect_arg[ARG1].value = (TRANSMIT_OPTION_ROUTED | TRANSMIT_OPTION_ACK);

  mock_call_expect(TO_STR(llGetCurrentHeaderFormat), &pMock);
  pMock->expect_arg[ARG0].value = 1;
  pMock->expect_arg[ARG1].value= false;
  pMock->return_code.value = HDRFORMATTYP_3CH;


  mock_call_expect(TO_STR(TxQueueGetOptions), &pMock);
  pMock->expect_arg[ARG0].pointer = &elm;
  pMock->return_code.value = (TRANSMIT_OPTION_ROUTED | TRANSMIT_OPTION_ACK);

  mock_call_expect(TO_STR(TxQueueGetOptions), &pMock);
  pMock->expect_arg[ARG0].pointer = &elm;
  pMock->return_code.value = (TRANSMIT_OPTION_ROUTED | TRANSMIT_OPTION_ACK);

  mock_call_expect(TO_STR(TxQueueGetOptions), &pMock);
  pMock->expect_arg[ARG0].pointer = &elm;
  pMock->return_code.value = (TRANSMIT_OPTION_ROUTED | TRANSMIT_OPTION_ACK);

  mock_call_expect(TO_STR(TxQueueGetOptions), &pMock);
  pMock->expect_arg[ARG0].pointer = &elm;
  pMock->return_code.value = (TRANSMIT_OPTION_ROUTED | TRANSMIT_OPTION_ACK);

  mock_call_expect(TO_STR(TxQueueGetOptions), &pMock);
  pMock->expect_arg[ARG0].pointer = &elm;
  pMock->return_code.value = (TRANSMIT_OPTION_ROUTED | TRANSMIT_OPTION_ACK);

  mock_call_expect(TO_STR(TxQueueGetOptions), &pMock);
  pMock->expect_arg[ARG0].pointer = &elm;
  pMock->return_code.value = (TRANSMIT_OPTION_ROUTED | TRANSMIT_OPTION_ACK);

  mock_call_expect(TO_STR(TxQueueGetOptions), &pMock);
  pMock->expect_arg[ARG0].pointer = &elm;
  pMock->return_code.value = (TRANSMIT_OPTION_ROUTED | TRANSMIT_OPTION_ACK);

  mock_call_expect(TO_STR(llGetCurrentHeaderFormat), &pMock);
  pMock->expect_arg[ARG0].value = 1;
  pMock->expect_arg[ARG1].value= false;
  pMock->return_code.value = HDRFORMATTYP_3CH;

  mock_call_expect(TO_STR(TxQueueGetOptions), &pMock);
  pMock->expect_arg[ARG0].pointer = &elm;
  pMock->return_code.value = (TRANSMIT_OPTION_ROUTED | TRANSMIT_OPTION_ACK);

  mock_call_expect(TO_STR(TxQueueQueueElement), &pMock);
  pMock->expect_arg[ARG0].pointer = &elm;

  //Test that the legacy and specification extension bits are set correctly
  SendRouteAck(&rxFrame);
  TEST_ASSERT_EQUAL(MASK_ROUT_EXTEND, elm.frame.header.singlecastRouted3ch.routeStatus & MASK_ROUT_EXTEND);
  TEST_ASSERT_EQUAL(MASK_HEADER_EXTENDED_3CH, elm.frame.header.singlecastRouted3ch.header.headerInfo2 & MASK_HEADER_EXTENDED_3CH);

  mock_calls_verify();
}
