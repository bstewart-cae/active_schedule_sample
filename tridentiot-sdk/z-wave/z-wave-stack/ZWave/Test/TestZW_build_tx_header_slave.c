// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file TestZW_build_tx_header_slave.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include <unity.h>
#include <mock_control.h>
#include <stdio.h>
#include <string.h>

#include "ZW_build_tx_header.h"
#include "ZW_node.h"
#include "ZW_explore.h"
#include "NodeMask.h"
#include "ZW_transport.h"

void setUpSuite(void) {

}

void tearDownSuite(void) {

}

//#define PRINT_TESTFRAME
#define sizeof_array(ARRAY) (sizeof(ARRAY)/sizeof(ARRAY[0]))

 /** START of Global variable needed for testing **/
bool g_learnModeClassic;
bool g_learnMode;
bool exploreInclusionModeRepeat;
uint8_t crH[HOMEID_LENGTH];
uint8_t currentSeqNo;
uint8_t sACK;
bool bNetworkWideInclusionReady = false;
uint8_t inclusionHomeID[HOMEID_LENGTH];
uint8_t inclusionHomeIDActive[HOMEID_LENGTH];
uint8_t bNetworkWideInclusion;
uint8_t mTransportRxCurrentSpeed;
uint8_t multiIDList[MAX_NODEMASK_LENGTH];
 /** END of Global variable needed for testing **/
TxQueueElement txElm;
mock_t * pMock;
/** END of Global variable needed for testing **/


void setUp(void)
{
  //This is run before EACH TEST
  mock_calls_clear();
}

void tearDown(void)
{
  mock_calls_verify();
}

typedef struct
{
  ZW_BasicFrameOptions_t   expectedFrameOption;
  ZW_HeaderFormatType_t headerType;
  uint8_t SeqNo;
  uint8_t frameType;
  uint8_t mHomeID[4];
  uint8_t mNodeID;
} ack_test_vector_t;

void test_build_ack_frames(void)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-braces"
  ack_test_vector_t test_vector[] = {
    {{0x00,.sourceNodeId = 2, .homeId = {0xF8, 0x75, 0xE2, 0x04}, .sequenceNumber = 0x50, .frameType = HDRTYP_TRANSFERACK}, HDRFORMATTYP_2CH, 0x50, FRAME_TYPE_ACK,.mHomeID = {0xF8, 0x75, 0xE2, 0x04}, .mNodeID = 2},
    {{0x00,.sourceNodeId = 1, .homeId = {0xF8, 0x75, 0xE2, 0x04}, .sequenceNumber = 8   , .frameType = HDRTYP_TRANSFERACK}, HDRFORMATTYP_3CH, 8,    FRAME_TYPE_ACK,.mHomeID = {0xF8, 0x75, 0xE2, 0x04}, .mNodeID = 1},
    {{0x00,.sourceNodeId = 1, .homeId = {0xF8, 0x75, 0xE2, 0x04}, .sequenceNumber = 8   , .frameType = HDRTYP_TRANSFERACK}, HDRFORMATTYP_LR,  8,    FRAME_TYPE_ACK,.mHomeID = {0xF8, 0x75, 0xE2, 0x04}, .mNodeID = 1}
  };
#pragma GCC diagnostic pop
  int result;
  for (uint32_t i = 0; i < sizeof_array(test_vector); i++)
  {
    printf("Test_vector %d\n",i);
    mock_call_expect(TO_STR(SetTransmitHomeID), &pMock);
    memset((uint8_t*)&txElm, 0, sizeof(TxQueueElement));  // Clear txElm
    pMock->expect_arg[0].pointer = &txElm;
    ZW_HomeIDSet(test_vector[i].mHomeID);
    g_nodeID = test_vector[i].mNodeID;
    txElm.bFrameOptions = 0;
    currentSeqNo = test_vector[i].SeqNo;
    BuildTxHeader(test_vector[i].headerType, test_vector[i].frameType, test_vector[i].SeqNo, &txElm);
    result = memcmp((uint8_t *)&test_vector[i].expectedFrameOption, (uint8_t*)&txElm.frame.frameOptions, sizeof(ZW_BasicFrameOptions_t));

    TEST_ASSERT_EQUAL_UINT8(result, 0);
  }
}

typedef struct
{
  ZW_BasicFrameOptions_t   expectedFrameOption;
  ZW_HeaderFormatType_t headerType;
  uint8_t SeqNo;
  uint8_t frameType;
  uint8_t mHomeID[4];
  uint8_t mNodeID;
  uint8_t setAck;
} single_test_vector_t;

void test_build_single_frames(void)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-braces"
  single_test_vector_t test_vector[] = {
    {{0x00,.sequenceNumber = 5 ,.frameType = HDRTYP_SINGLECAST} ,HDRFORMATTYP_2CH, 0x50, (0x50|FRAME_TYPE_SINGLE),.mHomeID = {0xF8, 0x75, 0xE2, 0x04}, .mNodeID = 2, .setAck = 0},
    {{0x00,.sequenceNumber = 5 ,.frameType = HDRTYP_SINGLECAST,.acknowledge = 1} ,HDRFORMATTYP_2CH, 0x50, (0x50|FRAME_TYPE_SINGLE),.mHomeID = {0xF8, 0x75, 0xE2, 0x04}, .mNodeID = 2, .setAck = 1},

    {{0x00,.sequenceNumber = 8 ,.frameType = HDRTYP_SINGLECAST} ,HDRFORMATTYP_3CH, 8, FRAME_TYPE_SINGLE,.mHomeID = {0xF8, 0x75, 0xE2, 0x04}, .mNodeID = 1, .mNodeID = 2, .setAck = 0},
    {{0x00,.sequenceNumber = 8 ,.frameType = HDRTYP_SINGLECAST,.acknowledge = 1} ,HDRFORMATTYP_3CH, 8, FRAME_TYPE_SINGLE,.mHomeID = {0xF8, 0x75, 0xE2, 0x04}, .mNodeID = 1, .mNodeID = 2, .setAck = 1},

    {{0x00,.sequenceNumber = 8 ,.frameType = HDRTYP_SINGLECAST} ,HDRFORMATTYP_LR, 8, FRAME_TYPE_SINGLE,.mHomeID = {0xF8, 0x75, 0xE2, 0x04}, .mNodeID = 1, .mNodeID = 2, .setAck = 0},
    {{0x00,.sequenceNumber = 8 ,.frameType = HDRTYP_SINGLECAST,.acknowledge = 1} ,HDRFORMATTYP_LR, 8, FRAME_TYPE_SINGLE,.mHomeID = {0xF8, 0x75, 0xE2, 0x04}, .mNodeID = 1, .mNodeID = 2, .setAck = 1},

  };
 #pragma GCC diagnostic pop
  int result;
  ZW_nodeDeviceOptionsSet(0);
  for (uint32_t i = 0; i < sizeof_array(test_vector); i++)
  {
    printf("Test_vector %d\n",i);
    mock_call_expect(TO_STR(SetTransmitHomeID), &pMock);
    memset((uint8_t*)&txElm, 0, sizeof(TxQueueElement));
    pMock->expect_arg[0].pointer = &txElm;
    ZW_HomeIDSet(test_vector[i].mHomeID);
    g_nodeID = test_vector[i].mNodeID;
    txElm.bFrameOptions = 0;
    currentSeqNo = test_vector[i].SeqNo ;
    uint8_t ackCount;
    if (test_vector[i].setAck)
    {
      txElm.bFrameOptions |= TRANSMIT_OPTION_ACK;
      ackCount = 3;
    }
    else
    {
      ackCount = 1;
    }
    BuildTxHeader(test_vector[i].headerType, test_vector[i].frameType, test_vector[i].SeqNo, &txElm);
    result = memcmp((uint8_t *)&test_vector[i].expectedFrameOption, (uint8_t*)&txElm.frame.frameOptions, sizeof(ZW_BasicFrameOptions_t));
    TEST_ASSERT_EQUAL_UINT8(result, 0);
    TEST_ASSERT_EQUAL_UINT8(ackCount, txElm.bCallersStatus);
  }
}
typedef struct
{
  ZW_BasicFrameOptions_t   expectedFrameOption;
  ZW_HeaderFormatType_t headerType;
  uint8_t SeqNo;
  uint8_t frameType;
  uint8_t setAck;
  uint8_t routedHeader[sizeof(frameHeaderSinglecastRouted3ch)];
  uint8_t expectedHeaderLen;
  uint8_t destIsSensor;
  uint8_t addExHeader;
} routed_test_vector_t;

void test_build_routed_frames(void)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-braces"
   routed_test_vector_t test_vector[] = {
    // 2CH testr vector
    {{0x00,.sequenceNumber = 5 ,.frameType = HDRTYP_SINGLECAST, .routed = 1},
     HDRFORMATTYP_2CH, 0x50, (0x50|FRAME_TYPE_ROUTED), .setAck =  TRANSMIT_OPTION_ACK,
     .routedHeader = {0xD9, 0xAD, 0x0F, 0x82, 0x01, 0x91, 0x01, 0x0F, 0x03, 0x00, 0x10, 0x02},
     .expectedHeaderLen = 12},

    {{0x00,.sequenceNumber = 5 ,.frameType = HDRTYP_SINGLECAST, .routed = 1},
     HDRFORMATTYP_2CH, 0x50, (0x50|FRAME_TYPE_ROUTED), .setAck =  TRANSMIT_OPTION_ACK,
     .routedHeader = {0xD9, 0xAD, 0x0F, 0x82, 0x01, 0x91, 0x01, 0x0F, 0x05, 0x08, 0x20, 0x02, 0x03, 0x10, APPLICATION_FREQ_LISTENING_MODE_250ms},
     .expectedHeaderLen = 15, .destIsSensor = APPLICATION_FREQ_LISTENING_MODE_250ms, .addExHeader = 1},

    {{0x00,.sequenceNumber = 5 ,.frameType = HDRTYP_SINGLECAST, .routed = 1},
     HDRFORMATTYP_2CH, 0x50, (0x50|FRAME_TYPE_ROUTED), .setAck =  TRANSMIT_OPTION_ACK,
     .routedHeader = {0xD9, 0xAD, 0x0F, 0x82, 0x01, 0x91, 0x01, 0x0F, 0x05, 0x00, 0x30, 0x02, 0x03, 0x04},
     .expectedHeaderLen = 14, .destIsSensor = APPLICATION_FREQ_LISTENING_MODE_250ms, .addExHeader = 0},

    // 3CH testr vector
    {{0x00,.sequenceNumber = 8 ,.frameType = HDRTYP_ROUTED,.routed = 1},
     HDRFORMATTYP_3CH, 8, FRAME_TYPE_ROUTED, .setAck =  TRANSMIT_OPTION_ACK,
     .routedHeader = {0xD9, 0xAD, 0x0F, 0x82, 0x01, 0x88, 0x00, 0x0F, 0x05, 0x05, 0x00, 0x10, 0x02, 0x00},
     .expectedHeaderLen = 14},

    {{0x00,.sequenceNumber = 8 ,.frameType = HDRTYP_ROUTED,.routed = 1},
     HDRFORMATTYP_3CH, 8, FRAME_TYPE_ROUTED, .setAck =  TRANSMIT_OPTION_ACK,
     .routedHeader = {0xD9, 0xAD, 0x0F, 0x82, 0x01, 0x88, 0x00, 0x0E, 0x05, 0x05, 0x00, 0x20, 0x02, 0x03, APPLICATION_FREQ_LISTENING_MODE_250ms},
     .expectedHeaderLen = 15, .destIsSensor = APPLICATION_FREQ_LISTENING_MODE_250ms, .addExHeader = 1},

    {{0x00,.sequenceNumber = 8 ,.frameType = HDRTYP_ROUTED,.routed = 1},
     HDRFORMATTYP_3CH, 8, FRAME_TYPE_ROUTED, .setAck =  TRANSMIT_OPTION_ACK,
     .routedHeader = {0xD9, 0xAD, 0x0F, 0x82, 0x01, 0x88, 0x00, 0x0E, 0x05, 0x05, 0x00, 0x10, 0x02, 0x00},
     .expectedHeaderLen = 14, .destIsSensor = APPLICATION_FREQ_LISTENING_MODE_250ms, .addExHeader = 0},
  };
#pragma GCC diagnostic pop

  int result;
  uint16_t dest_nodeid  = 5;
  for (uint32_t i = 0; i < sizeof_array(test_vector); i++)
  {
    printf("Test_vector %d\n",i);
    memset((uint8_t*)&txElm, 0, sizeof(TxQueueElement));
    mock_call_expect(TO_STR(SetTransmitHomeID), &pMock);
    pMock->expect_arg[0].pointer = &txElm;
    g_nodeID = 0x02;
    txElm.frame.frameOptions.sourceNodeId = 0x03;
    txElm.frame.frameOptions.destinationNodeId = dest_nodeid;

    test_vector[i].expectedFrameOption.destinationNodeId = dest_nodeid;
    test_vector[i].expectedFrameOption.sourceNodeId = 0x03;

    currentSeqNo = test_vector[i].SeqNo ;
    uint8_t ackCount;
    txElm.bFrameOptions |= test_vector[i].setAck;
    if (test_vector[i].setAck)
    {
      ackCount = 3;
    }
    else
    {
      ackCount = 1;
    }
    if (test_vector[i].destIsSensor)
    {
      txElm.wRFoptions = RF_OPTION_BEAM_MASK;
      if (test_vector[i].addExHeader)
      {
        txElm.frame.frameOptions.sourceNodeId = g_nodeID;
        test_vector[i].expectedFrameOption.sourceNodeId = g_nodeID;
      }
     }

    memcpy((uint8_t*)&txElm.frame.header, test_vector[i].routedHeader, sizeof(frameHeaderSinglecastRouted3ch));
    BuildTxHeader(test_vector[i].headerType, test_vector[i].frameType, test_vector[i].SeqNo, &txElm);
    result = memcmp((uint8_t *)&test_vector[i].expectedFrameOption, (uint8_t*)&txElm.frame.frameOptions, sizeof(ZW_BasicFrameOptions_t));
    TEST_ASSERT_EQUAL_UINT8(result, 0);
    TEST_ASSERT_EQUAL_UINT8(ackCount, txElm.bCallersStatus);
    TEST_ASSERT_EQUAL_UINT8(test_vector[i].expectedHeaderLen, txElm.frame.headerLength);
  }
}

void test_build_repeated_explore_frames(void)
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmissing-braces"
   routed_test_vector_t test_vector[] = {
    // 2CH testr vector
    {{0x00,.sequenceNumber = 5 ,.frameType = HDRTYP_SINGLECAST, .routed = 1},
     HDRFORMATTYP_2CH, 0x50, (0x50|FRAME_TYPE_ROUTED), .setAck =  TRANSMIT_OPTION_ACK,
     .routedHeader = {0xD9, 0xAD, 0x0F, 0x82, 0x01, 0x91, 0x01, 0x0F, 0x03, 0x00, 0x10, 0x02},
     .expectedHeaderLen = 12},

    {{0x00,.sequenceNumber = 5 ,.frameType = HDRTYP_SINGLECAST, .routed = 1},
     HDRFORMATTYP_2CH, 0x50, (0x50|FRAME_TYPE_ROUTED), .setAck =  TRANSMIT_OPTION_ACK,
     .routedHeader = {0xD9, 0xAD, 0x0F, 0x82, 0x01, 0x91, 0x01, 0x0F, 0x05, 0x08, 0x20, 0x02, 0x03, 0x10, APPLICATION_FREQ_LISTENING_MODE_250ms},
     .expectedHeaderLen = 15, .destIsSensor = APPLICATION_FREQ_LISTENING_MODE_250ms, .addExHeader = 1},

    {{0x00,.sequenceNumber = 5 ,.frameType = HDRTYP_SINGLECAST, .routed = 1},
     HDRFORMATTYP_2CH, 0x50, (0x50|FRAME_TYPE_ROUTED), .setAck =  TRANSMIT_OPTION_ACK,
     .routedHeader = {0xD9, 0xAD, 0x0F, 0x82, 0x01, 0x91, 0x01, 0x0F, 0x05, 0x00, 0x30, 0x02, 0x03, 0x04},
     .expectedHeaderLen = 14, .destIsSensor = APPLICATION_FREQ_LISTENING_MODE_250ms, .addExHeader = 0},

    // 3CH testr vector
    {{0x00,.sequenceNumber = 8 ,.frameType = HDRTYP_ROUTED,.routed = 1},
     HDRFORMATTYP_3CH, 8, FRAME_TYPE_ROUTED, .setAck =  TRANSMIT_OPTION_ACK,
     .routedHeader = {0xD9, 0xAD, 0x0F, 0x82, 0x01, 0x88, 0x00, 0x0F, 0x05, 0x05, 0x00, 0x10, 0x02, 0x00},
     .expectedHeaderLen = 14},

    {{0x00,.sequenceNumber = 8 ,.frameType = HDRTYP_ROUTED,.routed = 1},
     HDRFORMATTYP_3CH, 8, FRAME_TYPE_ROUTED, .setAck =  TRANSMIT_OPTION_ACK,
     .routedHeader = {0xD9, 0xAD, 0x0F, 0x82, 0x01, 0x88, 0x00, 0x0E, 0x05, 0x05, 0x00, 0x20, 0x02, 0x03, APPLICATION_FREQ_LISTENING_MODE_250ms},
     .expectedHeaderLen = 15, .destIsSensor = APPLICATION_FREQ_LISTENING_MODE_250ms, .addExHeader = 1},

    {{0x00,.sequenceNumber = 8 ,.frameType = HDRTYP_ROUTED,.routed = 1},
     HDRFORMATTYP_3CH, 8, FRAME_TYPE_ROUTED, .setAck =  TRANSMIT_OPTION_ACK,
     .routedHeader = {0xD9, 0xAD, 0x0F, 0x82, 0x01, 0x88, 0x00, 0x0E, 0x05, 0x05, 0x00, 0x10, 0x02, 0x00},
     .expectedHeaderLen = 14, .destIsSensor = APPLICATION_FREQ_LISTENING_MODE_250ms, .addExHeader = 0},
  };
 #pragma GCC diagnostic pop

  int result;
  uint16_t dest_nodeid  = 5;
  for (uint32_t i = 0; i < sizeof_array(test_vector); i++)
  {
    printf("Test_vector %d\n",i);
    memset((uint8_t*)&txElm, 0, sizeof(TxQueueElement));

    txElm.bFrameOptions = TRANSMIT_OPTION_EXPLORE_REPEAT;
    txElm.frame.payloadLength = 0;
    uint8_t *ptr = &test_vector[i].routedHeader[(HDRFORMATTYP_3CH == test_vector[i].headerType)?10:9];
    memcpy(txElm.frame.payload, ptr, 8);

    mock_call_expect(TO_STR(SetTransmitHomeID), &pMock);
    pMock->expect_arg[0].pointer = &txElm;
    g_nodeID = 0x02;
    txElm.frame.frameOptions.sourceNodeId = 0x03;
    txElm.frame.frameOptions.destinationNodeId = dest_nodeid;

    test_vector[i].expectedFrameOption.destinationNodeId = dest_nodeid;
    test_vector[i].expectedFrameOption.sourceNodeId = 0x03;

    currentSeqNo = test_vector[i].SeqNo ;
    uint8_t ackCount;
    txElm.bFrameOptions |= test_vector[i].setAck;
    if (test_vector[i].setAck)
    {
      ackCount = 3;
    }
    else
    {
      ackCount = 1;
    }
    if (test_vector[i].destIsSensor)
    {
      txElm.wRFoptions = RF_OPTION_BEAM_MASK;
      if (test_vector[i].addExHeader)
      {
        txElm.frame.frameOptions.sourceNodeId = g_nodeID;
        test_vector[i].expectedFrameOption.sourceNodeId = g_nodeID;
      }
     }
    memcpy((uint8_t*)&txElm.frame.header, test_vector[i].routedHeader, sizeof(frameHeaderSinglecastRouted3ch));
    BuildTxHeader(test_vector[i].headerType, test_vector[i].frameType, test_vector[i].SeqNo, &txElm);
    result = memcmp((uint8_t *)&test_vector[i].expectedFrameOption, (uint8_t*)&txElm.frame.frameOptions, sizeof(ZW_BasicFrameOptions_t));
    TEST_ASSERT_EQUAL_UINT8(result, 0);
    TEST_ASSERT_EQUAL_UINT8(ackCount, txElm.bCallersStatus);
    TEST_ASSERT_EQUAL_UINT8(test_vector[i].expectedHeaderLen, txElm.frame.headerLength);
  }
}
