// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file TestZW_frames_filters_CONTROLLER.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include "ZW_protocol.h"
#include "ZW_controller.h"
#include "ZW_replication.h"
#include "ZW_explore.h"
#include "ZW_controller_api.h"
#include "ZW_Frame.h"
#include "ZW.h"
#include "string.h"
#include "unity.h"
#include "mock_control.h"
#include "ZW_frames_filters.h"

void setUpSuite(void) {

}

void tearDownSuite(void) {

}

void setUp(void) {

}

void tearDown(void) {

}

uint16_t g_nodeID = 0x0001;
bool g_learnMode;
uint8_t g_learnNodeState;
uint8_t replicationStatus;
uint8_t staticControllerNodeID;
bool nodeIdserverPresent;
/*
 * Test that IsFrameIlLegal filter the received singlecast frames correctly
 */
void test_IsFrameIlLegal_singlecast_2ch(void)
{
  frame rxFrame;
  bool dropFrame;
  uint8_t cmdClass;
  uint8_t cmdClassCmd;
  mock_t * pMock;

  /* Test that frame is dropped if  sourceID == g_nodeID and the frame is COMAND_CLASS_SECUROTY*/
  mock_calls_clear();
  memset((uint8_t*)&rxFrame, 0, sizeof(frame));
  replicationStatus = 0;
  g_learnNodeState = LEARN_NODE_STATE_OFF;
  g_learnMode = 0;
  rxFrame.header.sourceID = g_nodeID;
  rxFrame.singlecast.destinationID = g_nodeID;
  SET_HEADERTYPE(rxFrame, HDRTYP_SINGLECAST);
  cmdClass = COMMAND_CLASS_SECURITY;
  cmdClassCmd = 0x35; // dummy value
  dropFrame = IsFrameIlLegal(HDRFORMATTYP_2CH, &rxFrame, &cmdClass, &cmdClassCmd);
  TEST_ASSERT_TRUE(dropFrame);
  mock_calls_verify();

  /* Test that frame is dropped if  sourceID == g_nodeID and the frame is COMAND_CLASS_SECUROTY even if we are in learmMode*/
  
  mock_calls_clear();
  memset((uint8_t*)&rxFrame, 0, sizeof(frame));
  replicationStatus = 0;
  g_learnNodeState = LEARN_NODE_STATE_OFF;
  g_learnMode = 1;
  rxFrame.header.sourceID = g_nodeID;
  rxFrame.singlecast.destinationID = g_nodeID;
  SET_HEADERTYPE(rxFrame, HDRTYP_SINGLECAST);
  cmdClass = COMMAND_CLASS_SECURITY;
  cmdClassCmd = 0x35; // dummy value
  dropFrame = IsFrameIlLegal(HDRFORMATTYP_2CH, &rxFrame, &cmdClass, &cmdClassCmd);
  TEST_ASSERT_TRUE(dropFrame);
  mock_calls_verify();

/* Test that frame is dropped if  sourceID == g_nodeID and the frame is COMAND_CLASS_SECUROTY_2*/
  mock_calls_clear();
  memset((uint8_t*)&rxFrame, 0, sizeof(frame));
  replicationStatus = 0;
  g_learnNodeState = LEARN_NODE_STATE_OFF;
  g_learnMode = 0;
  rxFrame.header.sourceID = g_nodeID;
  rxFrame.singlecast.destinationID = g_nodeID;
  SET_HEADERTYPE(rxFrame, HDRTYP_SINGLECAST);
  cmdClass = COMMAND_CLASS_SECURITY_2;
  cmdClassCmd = 0x35; // dummy value
  dropFrame = IsFrameIlLegal(HDRFORMATTYP_2CH, &rxFrame, &cmdClass, &cmdClassCmd);
  TEST_ASSERT_TRUE(dropFrame);
  mock_calls_verify();

  /* Test that frame is dropped if  sourceID == g_nodeID and the frame is COMAND_CLASS_SECUROTY_2 even if we are in learmMode*/
  mock_calls_clear();
  memset((uint8_t*)&rxFrame, 0, sizeof(frame));
  replicationStatus = 0;
  g_learnNodeState = LEARN_NODE_STATE_OFF;
  g_learnMode = 1;
  rxFrame.header.sourceID = g_nodeID;
  rxFrame.singlecast.destinationID = g_nodeID;
  SET_HEADERTYPE(rxFrame, HDRTYP_SINGLECAST);
  cmdClass = COMMAND_CLASS_SECURITY_2;
  cmdClassCmd = 0x35; // dummy value
  dropFrame = IsFrameIlLegal(HDRFORMATTYP_2CH, &rxFrame, &cmdClass, &cmdClassCmd);
  TEST_ASSERT_TRUE(dropFrame);
  mock_calls_verify();

  /* Test that frame is dropped if  destinationID == 0xFF and the frame is COMAND_CLASS_SECUROTY*/
  mock_calls_clear();
  memset((uint8_t*)&rxFrame, 0, sizeof(frame));
  replicationStatus = 0;
  g_learnNodeState = LEARN_NODE_STATE_OFF;
  g_learnMode = 0;
  rxFrame.header.sourceID = 5;
  rxFrame.singlecast.destinationID = 0xFF;
  SET_HEADERTYPE(rxFrame, HDRTYP_SINGLECAST);
  cmdClass = COMMAND_CLASS_SECURITY;
  cmdClassCmd = 0x35; // dummy value
  dropFrame = IsFrameIlLegal(HDRFORMATTYP_2CH, &rxFrame, &cmdClass, &cmdClassCmd);
  TEST_ASSERT_TRUE(dropFrame);
  mock_calls_verify();

  /* Test that frame is dropped if  sourceID != node but it is unknown*/
  mock_calls_clear();
  memset((uint8_t*)&rxFrame, 0, sizeof(frame));
  mock_call_expect(TO_STR(ZW_IsVirtualNode),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = false;

  mock_call_expect(TO_STR(ZCB_GetNodeType),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = false;

  replicationStatus = 0;
  g_learnNodeState = LEARN_NODE_STATE_OFF;
  g_learnMode = 0;
  rxFrame.header.sourceID = 5;
  rxFrame.singlecast.destinationID = 0x01;
  SET_HEADERTYPE(rxFrame, HDRTYP_SINGLECAST);
  cmdClass = 2;// dummy command class
  cmdClassCmd = 0x35; // dummy value
  dropFrame = IsFrameIlLegal(HDRFORMATTYP_2CH, &rxFrame, &cmdClass, &cmdClassCmd);
  TEST_ASSERT_TRUE(dropFrame);
  mock_calls_verify();

  /* Test that frame is dropped if  sourceID == node */
  mock_calls_clear();
  memset((uint8_t*)&rxFrame, 0, sizeof(frame));
  mock_call_expect(TO_STR(ZW_IsVirtualNode),&pMock);
  pMock->expect_arg[ARG0].value = 0x01;
  pMock->return_code.value = false;

  replicationStatus = 0;
  g_learnNodeState = LEARN_NODE_STATE_OFF;
  g_learnMode = 0;
  rxFrame.header.sourceID = 0x01;
  rxFrame.singlecast.destinationID = 0x06;
  SET_HEADERTYPE(rxFrame, HDRTYP_SINGLECAST);
  cmdClass = 2;// dummy command class
  cmdClassCmd = 0x35; // dummy value
  dropFrame = IsFrameIlLegal(HDRFORMATTYP_2CH, &rxFrame, &cmdClass, &cmdClassCmd);
  TEST_ASSERT_TRUE(dropFrame);
  mock_calls_verify();
  
  /* Test that ack frames are not dropped if  sourceID != node even it is known node*/
  mock_calls_clear();
  memset((uint8_t*)&rxFrame, 0, sizeof(frame));
  mock_call_expect(TO_STR(ZW_IsVirtualNode),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = false;

  mock_call_expect(TO_STR(ZCB_GetNodeType),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = false;

  replicationStatus = 0;
  g_learnNodeState = LEARN_NODE_STATE_OFF;
  g_learnMode = 0;
  rxFrame.header.sourceID = 5;
  rxFrame.singlecast.destinationID = 0x01;
  SET_HEADERTYPE(rxFrame, HDRTYP_TRANSFERACK);
  cmdClass = 2;// dummy command class
  cmdClassCmd = 0x35; // dummy value
  dropFrame = IsFrameIlLegal(HDRFORMATTYP_2CH, &rxFrame, &cmdClass, &cmdClassCmd);
  TEST_ASSERT_FALSE(dropFrame);
  mock_calls_verify();

  /* Test that frames are not dropped if we re in learnMode */
  mock_calls_clear();
  memset((uint8_t*)&rxFrame, 0, sizeof(frame));
  mock_call_expect(TO_STR(ZW_IsVirtualNode),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = false;

  mock_call_expect(TO_STR(ZCB_GetNodeType),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = true;

  replicationStatus = 0;
  g_learnNodeState = LEARN_NODE_STATE_OFF;
  g_learnMode = 1;
  rxFrame.header.sourceID = 5;
  rxFrame.singlecast.destinationID = 0x01;
  SET_HEADERTYPE(rxFrame, HDRTYP_SINGLECAST);
  cmdClass = 2;// dummy command class
  cmdClassCmd = 0x35; // dummy value
  dropFrame = IsFrameIlLegal(HDRFORMATTYP_2CH, &rxFrame, &cmdClass, &cmdClassCmd);
  TEST_ASSERT_FALSE(dropFrame);
  mock_calls_verify();

  /* Test that frames are not dropped if we are in receving replication */
  mock_calls_clear();
  memset((uint8_t*)&rxFrame, 0, sizeof(frame));
  mock_call_expect(TO_STR(ZW_IsVirtualNode),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = false;

  mock_call_expect(TO_STR(ZCB_GetNodeType),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = true;

  replicationStatus |= RECEIVER_CTRL_BIT;
  g_learnNodeState = LEARN_NODE_STATE_OFF;
  g_learnMode = 0;
  rxFrame.header.sourceID = 5;
  rxFrame.singlecast.destinationID = 0x01;
  SET_HEADERTYPE(rxFrame, HDRTYP_SINGLECAST);
  cmdClass = 2;// dummy command class
  cmdClassCmd = 0x35; // dummy value
  dropFrame = IsFrameIlLegal(HDRFORMATTYP_2CH, &rxFrame, &cmdClass, &cmdClassCmd);
  TEST_ASSERT_FALSE(dropFrame);
  mock_calls_verify();

  /* Test that frames are not dropped if we are adding nodes  and nframe from unkwon node*/
  mock_calls_clear();
  memset((uint8_t*)&rxFrame, 0, sizeof(frame));
  mock_call_expect(TO_STR(ZW_IsVirtualNode),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = false;

  mock_call_expect(TO_STR(ZCB_GetNodeType),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = false;

  replicationStatus = 0;
  g_learnNodeState = LEARN_NODE_STATE_NEW;
  g_learnMode = 0;
  rxFrame.header.sourceID = 5;
  rxFrame.singlecast.destinationID = 0x01;
  SET_HEADERTYPE(rxFrame, HDRTYP_SINGLECAST);
  cmdClass = 2;// dummy command class
  cmdClassCmd = 0x35; // dummy value
  dropFrame = IsFrameIlLegal(HDRFORMATTYP_2CH, &rxFrame, &cmdClass, &cmdClassCmd);
  TEST_ASSERT_FALSE(dropFrame);
  mock_calls_verify();

  /* Test that frames are not dropped if we are removing nodes  and nframe from unkwon node*/
  mock_calls_clear();
  memset((uint8_t*)&rxFrame, 0, sizeof(frame));
  mock_call_expect(TO_STR(ZW_IsVirtualNode),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = false;

  mock_call_expect(TO_STR(ZCB_GetNodeType),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = false;

  replicationStatus = 0;
  g_learnNodeState = LEARN_NODE_STATE_DELETE;
  g_learnMode = 0;
  rxFrame.header.sourceID = 5;
  rxFrame.singlecast.destinationID = 0x01;
  SET_HEADERTYPE(rxFrame, HDRTYP_SINGLECAST);
  cmdClass = 2;// dummy command class
  cmdClassCmd = 0x35; // dummy value
  dropFrame = IsFrameIlLegal(HDRFORMATTYP_2CH, &rxFrame, &cmdClass, &cmdClassCmd);
  TEST_ASSERT_FALSE(dropFrame);
  mock_calls_verify();

  /* Test that frames are not dropped if we are updating nodes  and nframe from unkwon node*/
  mock_calls_clear();
  memset((uint8_t*)&rxFrame, 0, sizeof(frame));
  mock_call_expect(TO_STR(ZW_IsVirtualNode),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = false;

  mock_call_expect(TO_STR(ZCB_GetNodeType),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = false;

  replicationStatus = 0;
  g_learnNodeState = LEARN_NODE_STATE_UPDATE;
  g_learnMode = 0;
  rxFrame.header.sourceID = 5;
  rxFrame.singlecast.destinationID = 0x01;
  SET_HEADERTYPE(rxFrame, HDRTYP_SINGLECAST);
  cmdClass = 2;// dummy command class
  cmdClassCmd = 0x35; // dummy value
  dropFrame = IsFrameIlLegal(HDRFORMATTYP_2CH, &rxFrame, &cmdClass, &cmdClassCmd);
  TEST_ASSERT_FALSE(dropFrame);
  mock_calls_verify();

  /* Test that frames are dropped if we are in smartstart  and frame from unkwon node*/
  mock_calls_clear();
  memset((uint8_t*)&rxFrame, 0, sizeof(frame));
  mock_call_expect(TO_STR(ZW_IsVirtualNode),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = false;

  mock_call_expect(TO_STR(ZCB_GetNodeType),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = false;

  replicationStatus = 0;
  g_learnNodeState = LEARN_NODE_STATE_SMART_START;
  g_learnMode = 0;
  rxFrame.header.sourceID = 5;
  rxFrame.singlecast.destinationID = 0x01;
  SET_HEADERTYPE(rxFrame, HDRTYP_SINGLECAST);
  cmdClass = 2;// dummy command class
  cmdClassCmd = 0x35; // dummy value
  dropFrame = IsFrameIlLegal(HDRFORMATTYP_2CH, &rxFrame, &cmdClass, &cmdClassCmd);
  TEST_ASSERT_TRUE(dropFrame);
  mock_calls_verify();

  /* Test that frames are not dropped if we are in smartstart  and frame from known node*/
  mock_calls_clear();
  memset((uint8_t*)&rxFrame, 0, sizeof(frame));
  mock_call_expect(TO_STR(ZW_IsVirtualNode),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = false;
  mock_call_expect(TO_STR(ZCB_GetNodeType),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = true;

  replicationStatus = 0;
  g_learnNodeState = LEARN_NODE_STATE_SMART_START;
  g_learnMode = 0;
  rxFrame.header.sourceID = 5;
  rxFrame.singlecast.destinationID = 0x01;
  SET_HEADERTYPE(rxFrame, HDRTYP_SINGLECAST);
  cmdClass = 2;// dummy command class
  cmdClassCmd = 0x35; // dummy value
  dropFrame = IsFrameIlLegal(HDRFORMATTYP_2CH, &rxFrame, &cmdClass, &cmdClassCmd);
  TEST_ASSERT_FALSE(dropFrame);
  mock_calls_verify();

  /* Test that NOP POWER framer are not dropped */
  mock_calls_clear();
  memset((uint8_t*)&rxFrame, 0, sizeof(frame));
  mock_call_expect(TO_STR(ZW_IsVirtualNode),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = false;

  mock_call_expect(TO_STR(ZCB_GetNodeType),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = false;

  replicationStatus = 0;
  g_learnNodeState = LEARN_NODE_STATE_SMART_START;
  g_learnMode = 0;
  rxFrame.header.sourceID = 5;
  rxFrame.singlecast.destinationID = 0x01;
  SET_HEADERTYPE(rxFrame, HDRTYP_SINGLECAST);
  cmdClass = 1;
  cmdClassCmd = ZWAVE_CMD_NOP_POWER;
  dropFrame = IsFrameIlLegal(HDRFORMATTYP_2CH, &rxFrame, &cmdClass, &cmdClassCmd);
  TEST_ASSERT_FALSE(dropFrame);
  mock_calls_verify();

  mock_calls_clear();
  memset((uint8_t*)&rxFrame, 0, sizeof(frame));
  mock_call_expect(TO_STR(ZW_IsVirtualNode),&pMock);
  pMock->expect_arg[ARG0].value = 1;
  pMock->return_code.value = false;
#if 0
  mock_call_expect(TO_STR(ZCB_GetNodeType),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = false;
#endif
  replicationStatus = 0;
  g_learnNodeState = LEARN_NODE_STATE_SMART_START;
  g_learnMode = 0;
  rxFrame.header.sourceID = 1;
  rxFrame.singlecast.destinationID = 0x01;
  SET_HEADERTYPE(rxFrame, HDRTYP_SINGLECAST);
  cmdClass = 1;
  cmdClassCmd = ZWAVE_CMD_NOP_POWER;
  dropFrame = IsFrameIlLegal(HDRFORMATTYP_2CH, &rxFrame, &cmdClass, &cmdClassCmd);
  TEST_ASSERT_FALSE(dropFrame);
  mock_calls_verify();

  /* Test that ZWAVE_CMD_TRANSFER_PRESENTATION framer are not dropped */
  mock_calls_clear();
  memset((uint8_t*)&rxFrame, 0, sizeof(frame));
  mock_call_expect(TO_STR(ZW_IsVirtualNode),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = false;
  mock_call_expect(TO_STR(ZCB_GetNodeType),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = false;


  replicationStatus = 0;
  g_learnNodeState = LEARN_NODE_STATE_SMART_START;
  g_learnMode = 0;
  rxFrame.header.sourceID = 5;
  rxFrame.singlecast.destinationID = 0x01;
  SET_HEADERTYPE(rxFrame, HDRTYP_SINGLECAST);
  cmdClass = 1;
  cmdClassCmd = ZWAVE_CMD_TRANSFER_PRESENTATION;
  dropFrame = IsFrameIlLegal(HDRFORMATTYP_2CH, &rxFrame, &cmdClass, &cmdClassCmd);
  TEST_ASSERT_FALSE(dropFrame);
  mock_calls_verify();

  mock_calls_clear();
  memset((uint8_t*)&rxFrame, 0, sizeof(frame));
  mock_call_expect(TO_STR(ZW_IsVirtualNode),&pMock);
  pMock->expect_arg[ARG0].value = 1;
  pMock->return_code.value = false;

  replicationStatus = 0;
  g_learnNodeState = LEARN_NODE_STATE_SMART_START;
  g_learnMode = 0;
  rxFrame.header.sourceID = 1;
  rxFrame.singlecast.destinationID = 0x01;
  SET_HEADERTYPE(rxFrame, HDRTYP_SINGLECAST);
  cmdClass = 1;
  cmdClassCmd = ZWAVE_CMD_TRANSFER_PRESENTATION;
  dropFrame = IsFrameIlLegal(HDRFORMATTYP_2CH, &rxFrame, &cmdClass, &cmdClassCmd);
  TEST_ASSERT_FALSE(dropFrame);
  mock_calls_verify();  
}

/*
 * Test that IsFrameIlLegal filter the received singlecast frames correctly 3ch system
 */
void test_IsFrameIlLegal_singlecast_3ch(void)
{
  frame rxFrame;
  bool dropFrame;
  uint8_t cmdClass;
  uint8_t cmdClassCmd;
  mock_t * pMock;

  /* Test that frame is dropped if  sourceID == g_nodeID and the frame is COMAND_CLASS_SECUROTY*/
  mock_calls_clear();
  memset((uint8_t*)&rxFrame, 0, sizeof(frame));

  replicationStatus = 0;
  g_learnNodeState = LEARN_NODE_STATE_OFF;
  g_learnMode = 0;
  rxFrame.header.sourceID = g_nodeID;
  rxFrame.singlecast3ch.destinationID = g_nodeID;
  SET_HEADERTYPE(rxFrame, HDRTYP_SINGLECAST);
  cmdClass = COMMAND_CLASS_SECURITY;
  cmdClassCmd = 0x35; // dummy value
  dropFrame = IsFrameIlLegal(HDRFORMATTYP_3CH, &rxFrame, &cmdClass, &cmdClassCmd);
  TEST_ASSERT_TRUE(dropFrame);
  mock_calls_verify();
  
  /* Test that frame is dropped if  sourceID == g_nodeID and the frame is COMAND_CLASS_SECUROTY even if we are in learmMode*/
  mock_calls_clear();
  memset((uint8_t*)&rxFrame, 0, sizeof(frame));
  replicationStatus = 0;
  g_learnNodeState = LEARN_NODE_STATE_OFF;
  g_learnMode = 1;
  rxFrame.header.sourceID = g_nodeID;
  rxFrame.singlecast3ch.destinationID = g_nodeID;
  SET_HEADERTYPE(rxFrame, HDRTYP_SINGLECAST);
  cmdClass = COMMAND_CLASS_SECURITY;
  cmdClassCmd = 0x35; // dummy value
  dropFrame = IsFrameIlLegal(HDRFORMATTYP_3CH, &rxFrame, &cmdClass, &cmdClassCmd);
  TEST_ASSERT_TRUE(dropFrame);
  mock_calls_verify();

/* Test that frame is dropped if  sourceID == g_nodeID and the frame is COMAND_CLASS_SECUROTY_2*/
  mock_calls_clear();
  memset((uint8_t*)&rxFrame, 0, sizeof(frame));

  replicationStatus = 0;
  g_learnNodeState = LEARN_NODE_STATE_OFF;
  g_learnMode = 0;
  rxFrame.header.sourceID = g_nodeID;
  rxFrame.singlecast3ch.destinationID = g_nodeID;
  SET_HEADERTYPE(rxFrame, HDRTYP_SINGLECAST);
  cmdClass = COMMAND_CLASS_SECURITY_2;
  cmdClassCmd = 0x35; // dummy value
  dropFrame = IsFrameIlLegal(HDRFORMATTYP_3CH, &rxFrame, &cmdClass, &cmdClassCmd);
  TEST_ASSERT_TRUE(dropFrame);
  mock_calls_verify();

  /* Test that frame is dropped if  sourceID == g_nodeID and the frame is COMAND_CLASS_SECUROTY_2 even if we are in learmMode*/
  mock_calls_clear();
  memset((uint8_t*)&rxFrame, 0, sizeof(frame));

  replicationStatus = 0;
  g_learnNodeState = LEARN_NODE_STATE_OFF;
  g_learnMode = 1;
  rxFrame.header.sourceID = g_nodeID;
  rxFrame.singlecast3ch.destinationID = g_nodeID;
  SET_HEADERTYPE(rxFrame, HDRTYP_SINGLECAST);
  cmdClass = COMMAND_CLASS_SECURITY_2;
  cmdClassCmd = 0x35; // dummy value
  dropFrame = IsFrameIlLegal(HDRFORMATTYP_3CH, &rxFrame, &cmdClass, &cmdClassCmd);
  TEST_ASSERT_TRUE(dropFrame);
  mock_calls_verify();

  /* Test that frame is dropped if  destinationID == 0xFF and the frame is COMAND_CLASS_SECUROTY*/
  mock_calls_clear();
  memset((uint8_t*)&rxFrame, 0, sizeof(frame));
  replicationStatus = 0;
  g_learnNodeState = LEARN_NODE_STATE_OFF;
  g_learnMode = 0;
  rxFrame.header.sourceID = 5;
  rxFrame.singlecast3ch.destinationID = 0xFF;
  SET_HEADERTYPE(rxFrame, HDRTYP_SINGLECAST);
  cmdClass = COMMAND_CLASS_SECURITY;
  cmdClassCmd = 0x35; // dummy value
  dropFrame = IsFrameIlLegal(HDRFORMATTYP_3CH, &rxFrame, &cmdClass, &cmdClassCmd);
  TEST_ASSERT_TRUE(dropFrame);
  mock_calls_verify();

  /* Test that frame is dropped if  sourceID != node but it is unknown*/
  mock_calls_clear();
  memset((uint8_t*)&rxFrame, 0, sizeof(frame));
  mock_call_expect(TO_STR(ZW_IsVirtualNode),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = false;
  mock_call_expect(TO_STR(ZCB_GetNodeType),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = false;
  
  replicationStatus = 0;
  g_learnNodeState = LEARN_NODE_STATE_OFF;
  g_learnMode = 0;
  rxFrame.header.sourceID = 5;
  rxFrame.singlecast3ch.destinationID = 0x01;
  SET_HEADERTYPE(rxFrame, HDRTYP_SINGLECAST);
  cmdClass = 2;// dummy command class
  cmdClassCmd = 0x35; // dummy value
  dropFrame = IsFrameIlLegal(HDRFORMATTYP_3CH, &rxFrame, &cmdClass, &cmdClassCmd);
  TEST_ASSERT_TRUE(dropFrame);
  mock_calls_verify();

  /* Test that ack frames are not dropped if  sourceID != node even it is known node*/
  mock_calls_clear();
  memset((uint8_t*)&rxFrame, 0, sizeof(frame));
  mock_call_expect(TO_STR(ZW_IsVirtualNode),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = false;

  mock_call_expect(TO_STR(ZCB_GetNodeType),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = false;

  replicationStatus = 0;
  g_learnNodeState = LEARN_NODE_STATE_OFF;
  g_learnMode = 0;
  rxFrame.header.sourceID = 5;
  rxFrame.singlecast3ch.destinationID = 0x01;
  SET_HEADERTYPE(rxFrame, HDRTYP_TRANSFERACK);
  cmdClass = 2;// dummy command class
  cmdClassCmd = 0x35; // dummy value
  dropFrame = IsFrameIlLegal(HDRFORMATTYP_3CH, &rxFrame, &cmdClass, &cmdClassCmd);
  TEST_ASSERT_FALSE(dropFrame);
  mock_calls_verify();

  /* Test that frame is dropped if  sourceID == node */
  mock_calls_clear();
  memset((uint8_t*)&rxFrame, 0, sizeof(frame));
  mock_call_expect(TO_STR(ZW_IsVirtualNode),&pMock);
  pMock->expect_arg[ARG0].value = 0x01;
  pMock->return_code.value = false;

  replicationStatus = 0;
  g_learnNodeState = LEARN_NODE_STATE_OFF;
  g_learnMode = 0;
  rxFrame.header.sourceID = 0x01;
  rxFrame.singlecast3ch.destinationID = 0x06;
  SET_HEADERTYPE(rxFrame, HDRTYP_SINGLECAST);
  cmdClass = 2;// dummy command class
  cmdClassCmd = 0x35; // dummy value
  dropFrame = IsFrameIlLegal(HDRFORMATTYP_3CH, &rxFrame, &cmdClass, &cmdClassCmd);
  TEST_ASSERT_TRUE(dropFrame);
  mock_calls_verify();


  /* Test that frames are not dropped if we re in learnMode */
  mock_calls_clear();
  memset((uint8_t*)&rxFrame, 0, sizeof(frame));
  mock_call_expect(TO_STR(ZW_IsVirtualNode),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = false;

  mock_call_expect(TO_STR(ZCB_GetNodeType),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = true;

  replicationStatus = 0;
  g_learnNodeState = LEARN_NODE_STATE_OFF;
  g_learnMode = 1;
  rxFrame.header.sourceID = 5;
  rxFrame.singlecast3ch.destinationID = 0x01;
  SET_HEADERTYPE(rxFrame, HDRTYP_SINGLECAST);
  cmdClass = 2;// dummy command class
  cmdClassCmd = 0x35; // dummy value
  dropFrame = IsFrameIlLegal(HDRFORMATTYP_3CH, &rxFrame, &cmdClass, &cmdClassCmd);
  TEST_ASSERT_FALSE(dropFrame);
  mock_calls_verify();

  /* Test that frames are not dropped if we are in receving replication */
  mock_calls_clear();
  memset((uint8_t*)&rxFrame, 0, sizeof(frame));
  mock_call_expect(TO_STR(ZW_IsVirtualNode),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = false;

  mock_call_expect(TO_STR(ZCB_GetNodeType),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = true;

  replicationStatus |= RECEIVER_CTRL_BIT;
  g_learnNodeState = LEARN_NODE_STATE_OFF;
  g_learnMode = 0;
  rxFrame.header.sourceID = 5;
  rxFrame.singlecast3ch.destinationID = 0x01;
  SET_HEADERTYPE(rxFrame, HDRTYP_SINGLECAST);
  cmdClass = 2;// dummy command class
  cmdClassCmd = 0x35; // dummy value
  dropFrame = IsFrameIlLegal(HDRFORMATTYP_3CH, &rxFrame, &cmdClass, &cmdClassCmd);
  TEST_ASSERT_FALSE(dropFrame);
  mock_calls_verify();


  /* Test that frames are not dropped if we are adding nodes  and nframe from unkwon node*/
  mock_calls_clear();
  memset((uint8_t*)&rxFrame, 0, sizeof(frame));
  mock_call_expect(TO_STR(ZW_IsVirtualNode),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = false;

  mock_call_expect(TO_STR(ZCB_GetNodeType),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = false;

  replicationStatus = 0;
  g_learnNodeState = LEARN_NODE_STATE_NEW;
  g_learnMode = 0;
  rxFrame.header.sourceID = 5;
  rxFrame.singlecast3ch.destinationID = 0x01;
  SET_HEADERTYPE(rxFrame, HDRTYP_SINGLECAST);
  cmdClass = 2;// dummy command class
  cmdClassCmd = 0x35; // dummy value
  dropFrame = IsFrameIlLegal(HDRFORMATTYP_3CH, &rxFrame, &cmdClass, &cmdClassCmd);
  TEST_ASSERT_FALSE(dropFrame);
  mock_calls_verify();

  /* Test that frames are not dropped if we are removing nodes  and frame from unkwon node*/
  mock_calls_clear();
  memset((uint8_t*)&rxFrame, 0, sizeof(frame));
  mock_call_expect(TO_STR(ZW_IsVirtualNode),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = false;

  mock_call_expect(TO_STR(ZCB_GetNodeType),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = false;

  replicationStatus = 0;
  g_learnNodeState = LEARN_NODE_STATE_DELETE;
  g_learnMode = 0;
  rxFrame.header.sourceID = 5;
  rxFrame.singlecast3ch.destinationID = 0x01;
  SET_HEADERTYPE(rxFrame, HDRTYP_SINGLECAST);
  cmdClass = 2;// dummy command class
  cmdClassCmd = 0x35; // dummy value
  dropFrame = IsFrameIlLegal(HDRFORMATTYP_3CH, &rxFrame, &cmdClass, &cmdClassCmd);
  TEST_ASSERT_FALSE(dropFrame);
  mock_calls_verify();

  /* Test that frames are not dropped if we are updating nodes  and nframe from unkwon node*/
  mock_calls_clear();
  memset((uint8_t*)&rxFrame, 0, sizeof(frame));
  mock_call_expect(TO_STR(ZW_IsVirtualNode),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = false;

  mock_call_expect(TO_STR(ZCB_GetNodeType),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = false;

  replicationStatus = 0;
  g_learnNodeState = LEARN_NODE_STATE_UPDATE;
  g_learnMode = 0;
  rxFrame.header.sourceID = 5;
  rxFrame.singlecast3ch.destinationID = 0x01;
  SET_HEADERTYPE(rxFrame, HDRTYP_SINGLECAST);
  cmdClass = 2;// dummy command class
  cmdClassCmd = 0x35; // dummy value
  dropFrame = IsFrameIlLegal(HDRFORMATTYP_3CH, &rxFrame, &cmdClass, &cmdClassCmd);
  TEST_ASSERT_FALSE(dropFrame);
  mock_calls_verify();

  /* Test that frames are dropped if we are in smartstart  and frame from unkwon node*/
  mock_calls_clear();
  memset((uint8_t*)&rxFrame, 0, sizeof(frame));
  mock_call_expect(TO_STR(ZW_IsVirtualNode),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = false;

  mock_call_expect(TO_STR(ZCB_GetNodeType),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = false;

  replicationStatus = 0;
  g_learnNodeState = LEARN_NODE_STATE_SMART_START;
  g_learnMode = 0;
  rxFrame.header.sourceID = 5;
  rxFrame.singlecast3ch.destinationID = 0x01;
  SET_HEADERTYPE(rxFrame, HDRTYP_SINGLECAST);
  cmdClass = 2;// dummy command class
  cmdClassCmd = 0x35; // dummy value
  dropFrame = IsFrameIlLegal(HDRFORMATTYP_3CH, &rxFrame, &cmdClass, &cmdClassCmd);
  TEST_ASSERT_TRUE(dropFrame);
  mock_calls_verify();

  /* Test that frames are not dropped if we are in smartstart  and frame from known node*/
  mock_calls_clear();
  memset((uint8_t*)&rxFrame, 0, sizeof(frame));
  mock_call_expect(TO_STR(ZW_IsVirtualNode),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = false;

  mock_call_expect(TO_STR(ZCB_GetNodeType),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = true;

  replicationStatus = 0;
  g_learnNodeState = LEARN_NODE_STATE_SMART_START;
  g_learnMode = 0;
  rxFrame.header.sourceID = 5;
  rxFrame.singlecast3ch.destinationID = 0x01;
  SET_HEADERTYPE(rxFrame, HDRTYP_SINGLECAST);
  cmdClass = 2;// dummy command class
  cmdClassCmd = 0x35; // dummy value
  dropFrame = IsFrameIlLegal(HDRFORMATTYP_3CH, &rxFrame, &cmdClass, &cmdClassCmd);
  TEST_ASSERT_FALSE(dropFrame);
  mock_calls_verify();

  /* Test that NOP POWER framer are not dropped */
  mock_calls_clear();
  memset((uint8_t*)&rxFrame, 0, sizeof(frame));
  mock_call_expect(TO_STR(ZW_IsVirtualNode),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = false;

  mock_call_expect(TO_STR(ZCB_GetNodeType),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = false;

  replicationStatus = 0;
  g_learnNodeState = LEARN_NODE_STATE_SMART_START;
  g_learnMode = 0;
  rxFrame.header.sourceID = 5;
  rxFrame.singlecast3ch.destinationID = 0x01;
  SET_HEADERTYPE(rxFrame, HDRTYP_SINGLECAST);
  cmdClass = 1;
  cmdClassCmd = ZWAVE_CMD_NOP_POWER;
  dropFrame = IsFrameIlLegal(HDRFORMATTYP_3CH, &rxFrame, &cmdClass, &cmdClassCmd);
  TEST_ASSERT_FALSE(dropFrame);
  mock_calls_verify();

  mock_calls_clear();
  memset((uint8_t*)&rxFrame, 0, sizeof(frame));
  mock_call_expect(TO_STR(ZW_IsVirtualNode),&pMock);
  pMock->expect_arg[ARG0].value = 1;
  pMock->return_code.value = false;

  replicationStatus = 0;
  g_learnNodeState = LEARN_NODE_STATE_SMART_START;
  g_learnMode = 0;
  rxFrame.header.sourceID = 1;
  rxFrame.singlecast3ch.destinationID = 0x01;
  SET_HEADERTYPE(rxFrame, HDRTYP_SINGLECAST);
  cmdClass = 1;
  cmdClassCmd = ZWAVE_CMD_NOP_POWER;
  dropFrame = IsFrameIlLegal(HDRFORMATTYP_3CH, &rxFrame, &cmdClass, &cmdClassCmd);
  TEST_ASSERT_FALSE(dropFrame);
  mock_calls_verify();

  /* Test that ZWAVE_CMD_TRANSFER_PRESENTATION framer are not dropped */
  mock_calls_clear();
  memset((uint8_t*)&rxFrame, 0, sizeof(frame));
  mock_call_expect(TO_STR(ZW_IsVirtualNode),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = false;

  mock_call_expect(TO_STR(ZCB_GetNodeType),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = false;

  replicationStatus = 0;
  g_learnNodeState = LEARN_NODE_STATE_SMART_START;
  g_learnMode = 0;
  rxFrame.header.sourceID = 5;
  rxFrame.singlecast3ch.destinationID = 0x01;
  SET_HEADERTYPE(rxFrame, HDRTYP_SINGLECAST);
  cmdClass = 1;
  cmdClassCmd = ZWAVE_CMD_TRANSFER_PRESENTATION;
  dropFrame = IsFrameIlLegal(HDRFORMATTYP_3CH, &rxFrame, &cmdClass, &cmdClassCmd);
  TEST_ASSERT_FALSE(dropFrame);
  mock_calls_verify();

  mock_calls_clear();
  memset((uint8_t*)&rxFrame, 0, sizeof(frame));
  mock_call_expect(TO_STR(ZW_IsVirtualNode),&pMock);
  pMock->expect_arg[ARG0].value = 1;
  pMock->return_code.value = false;

  replicationStatus = 0;
  g_learnNodeState = LEARN_NODE_STATE_SMART_START;
  g_learnMode = 0;
  rxFrame.header.sourceID = 1;
  rxFrame.singlecast3ch.destinationID = 0x01;
  SET_HEADERTYPE(rxFrame, HDRTYP_SINGLECAST);
  cmdClass = 1;
  cmdClassCmd = ZWAVE_CMD_TRANSFER_PRESENTATION;
  dropFrame = IsFrameIlLegal(HDRFORMATTYP_3CH, &rxFrame, &cmdClass, &cmdClassCmd);
  TEST_ASSERT_FALSE(dropFrame);
  mock_calls_verify();
}


/*
 * Test that IsFrameIlLegal filter the received routed  frames correctly
 */
void test_IsFrameIlLegal_routed_2ch(void)
{
  frame rxFrame;
  bool dropFrame;
  uint8_t cmdClass;
  uint8_t cmdClassCmd;
  mock_t * pMock;

  /* Test that frame is dropped if  sourceID == g_nodeID and the frame is COMAND_CLASS_SECUROTY*/
  mock_calls_clear();
  memset((uint8_t*)&rxFrame, 0, sizeof(frame));

  replicationStatus = 0;
  g_learnNodeState = LEARN_NODE_STATE_OFF;
  g_learnMode = 0;
  rxFrame.header.sourceID = g_nodeID;
  rxFrame.singlecastRouted.destinationID = g_nodeID;
  SET_HEADERTYPE(rxFrame, HDRTYP_SINGLECAST);
  SET_ROUTED(rxFrame);
  cmdClass = COMMAND_CLASS_SECURITY;
  cmdClassCmd = 0x35; // dummy value
  dropFrame = IsFrameIlLegal(HDRFORMATTYP_2CH, &rxFrame, &cmdClass, &cmdClassCmd);
  TEST_ASSERT_TRUE(dropFrame);
  mock_calls_verify();

  /* Test that frame is dropped if  sourceID == g_nodeID and the frame is COMAND_CLASS_SECUROTY even if we are in learmMode*/
  mock_calls_clear();
  memset((uint8_t*)&rxFrame, 0, sizeof(frame));

  replicationStatus = 0;
  g_learnNodeState = LEARN_NODE_STATE_OFF;
  g_learnMode = 1;
  rxFrame.header.sourceID = g_nodeID;
  rxFrame.singlecastRouted.destinationID = g_nodeID;
  SET_HEADERTYPE(rxFrame, HDRTYP_SINGLECAST);
  SET_ROUTED(rxFrame);
  cmdClass = COMMAND_CLASS_SECURITY;
  cmdClassCmd = 0x35; // dummy value
  dropFrame = IsFrameIlLegal(HDRFORMATTYP_2CH, &rxFrame, &cmdClass, &cmdClassCmd);
  TEST_ASSERT_TRUE(dropFrame);
  mock_calls_verify();

/* Test that frame is dropped if  sourceID == g_nodeID and the frame is COMAND_CLASS_SECUROTY_2*/
  mock_calls_clear();
  memset((uint8_t*)&rxFrame, 0, sizeof(frame));

  replicationStatus = 0;
  g_learnNodeState = LEARN_NODE_STATE_OFF;
  g_learnMode = 0;
  rxFrame.header.sourceID = g_nodeID;
  rxFrame.singlecastRouted.destinationID = g_nodeID;
  SET_HEADERTYPE(rxFrame, HDRTYP_SINGLECAST);
  SET_ROUTED(rxFrame);
  cmdClass = COMMAND_CLASS_SECURITY_2;
  cmdClassCmd = 0x35; // dummy value
  dropFrame = IsFrameIlLegal(HDRFORMATTYP_2CH, &rxFrame, &cmdClass, &cmdClassCmd);
  TEST_ASSERT_TRUE(dropFrame);
  mock_calls_verify();

  /* Test that frame is dropped if  sourceID == g_nodeID and the frame is COMAND_CLASS_SECUROTY_2 even if we are in learmMode*/
  mock_calls_clear();
  memset((uint8_t*)&rxFrame, 0, sizeof(frame));

  replicationStatus = 0;
  g_learnNodeState = LEARN_NODE_STATE_OFF;
  g_learnMode = 1;
  rxFrame.header.sourceID = g_nodeID;
  rxFrame.singlecastRouted.destinationID = g_nodeID;
  SET_HEADERTYPE(rxFrame, HDRTYP_SINGLECAST);
  SET_ROUTED(rxFrame);
  cmdClass = COMMAND_CLASS_SECURITY_2;
  cmdClassCmd = 0x35; // dummy value
  dropFrame = IsFrameIlLegal(HDRFORMATTYP_2CH, &rxFrame, &cmdClass, &cmdClassCmd);
  TEST_ASSERT_TRUE(dropFrame);
  mock_calls_verify();

  /* Test that frame is dropped if  destinationID == 0xFF and the frame is COMAND_CLASS_SECUROTY*/
  mock_calls_clear();
  memset((uint8_t*)&rxFrame, 0, sizeof(frame));

  replicationStatus = 0;
  g_learnNodeState = LEARN_NODE_STATE_OFF;
  g_learnMode = 0;
  rxFrame.header.sourceID = 5;
  rxFrame.singlecastRouted.destinationID = 0xFF;
  SET_HEADERTYPE(rxFrame, HDRTYP_SINGLECAST);
  SET_ROUTED(rxFrame);
  cmdClass = COMMAND_CLASS_SECURITY;
  cmdClassCmd = 0x35; // dummy value
  dropFrame = IsFrameIlLegal(HDRFORMATTYP_2CH, &rxFrame, &cmdClass, &cmdClassCmd);
  TEST_ASSERT_TRUE(dropFrame);
  mock_calls_verify();

  /* Test that frame is not dropped if sourceID != node but it is unknown and the frame is routed */
  mock_calls_clear();
  memset((uint8_t*)&rxFrame, 0, sizeof(frame));

  mock_call_expect(TO_STR(ZW_IsVirtualNode),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = false;

  mock_call_expect(TO_STR(ZCB_GetNodeType),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = false;

  replicationStatus = 0;
  g_learnNodeState = LEARN_NODE_STATE_OFF;
  g_learnMode = 0;
  rxFrame.header.sourceID = 5;
  rxFrame.singlecastRouted.destinationID = 0x01;
  SET_HEADERTYPE(rxFrame, HDRTYP_SINGLECAST);
  SET_ROUTED(rxFrame);
  cmdClass = 2;// dummy command class
  cmdClassCmd = 0x35; // dummy value
  dropFrame = IsFrameIlLegal(HDRFORMATTYP_2CH, &rxFrame, &cmdClass, &cmdClassCmd);
  TEST_ASSERT_FALSE(dropFrame);
  mock_calls_verify();

  /* Test that ack frames are not dropped if  sourceID != node even it is known node*/
  mock_calls_clear();
  memset((uint8_t*)&rxFrame, 0, sizeof(frame));

  mock_call_expect(TO_STR(ZW_IsVirtualNode),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = false;

  mock_call_expect(TO_STR(ZCB_GetNodeType),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = false;

  replicationStatus = 0;
  g_learnNodeState = LEARN_NODE_STATE_OFF;
  g_learnMode = 0;
  rxFrame.header.sourceID = 5;
  rxFrame.singlecastRouted.destinationID = 0x01;
  SET_HEADERTYPE(rxFrame, HDRTYP_TRANSFERACK);
  SET_ROUTED(rxFrame);
  cmdClass = 2;// dummy command class
  cmdClassCmd = 0x35; // dummy value
  dropFrame = IsFrameIlLegal(HDRFORMATTYP_2CH, &rxFrame, &cmdClass, &cmdClassCmd);
  TEST_ASSERT_FALSE(dropFrame);
  mock_calls_verify();

  /* Test that frame is dropped if  sourceID == node and destination = g_nodeID*/
  mock_calls_clear();
  memset((uint8_t*)&rxFrame, 0, sizeof(frame));

  mock_call_expect(TO_STR(ZW_IsVirtualNode),&pMock);
  pMock->expect_arg[ARG0].value = 0x01;
  pMock->return_code.value = false;

  replicationStatus = 0;
  g_learnNodeState = LEARN_NODE_STATE_OFF;
  g_learnMode = 0;
  rxFrame.header.sourceID = 0x01;
  rxFrame.singlecastRouted.destinationID = 0x01;
  SET_ROUTED(rxFrame);
  SET_HEADERTYPE(rxFrame, HDRTYP_SINGLECAST);
  cmdClass = 2;// dummy command class
  cmdClassCmd = 0x35; // dummy value
  dropFrame = IsFrameIlLegal(HDRFORMATTYP_2CH, &rxFrame, &cmdClass, &cmdClassCmd);
  TEST_ASSERT_TRUE(dropFrame);
  mock_calls_verify();

  /* Test that frame is not dropped if  sourceID == node and destination != g_nodeID*/
  mock_calls_clear();
  memset((uint8_t*)&rxFrame, 0, sizeof(frame));

  mock_call_expect(TO_STR(ZW_IsVirtualNode),&pMock);
  pMock->expect_arg[ARG0].value = 0x01;
  pMock->return_code.value = false;

  mock_call_expect(TO_STR(ZCB_GetNodeType),&pMock);
  pMock->expect_arg[ARG0].value = 0x01;
  pMock->return_code.value = true;

  replicationStatus = 0;
  g_learnNodeState = LEARN_NODE_STATE_OFF;
  g_learnMode = 0;
  rxFrame.header.sourceID = 0x01;
  rxFrame.singlecastRouted.destinationID = 0x05;
  SET_HEADERTYPE(rxFrame, HDRTYP_SINGLECAST);
  SET_ROUTED(rxFrame);
  cmdClass = 2;// dummy command class
  cmdClassCmd = 0x35; // dummy value
  dropFrame = IsFrameIlLegal(HDRFORMATTYP_2CH, &rxFrame, &cmdClass, &cmdClassCmd);
  TEST_ASSERT_FALSE(dropFrame);
  mock_calls_verify();

  /* Test that frames are not dropped if we re in learnMode */
  mock_calls_clear();
  memset((uint8_t*)&rxFrame, 0, sizeof(frame));

  mock_call_expect(TO_STR(ZW_IsVirtualNode),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = false;

  mock_call_expect(TO_STR(ZCB_GetNodeType),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = true;

  replicationStatus = 0;
  g_learnNodeState = LEARN_NODE_STATE_OFF;
  g_learnMode = 1;
  rxFrame.header.sourceID = 5;
  rxFrame.singlecastRouted.destinationID = 0x01;
  SET_HEADERTYPE(rxFrame, HDRTYP_SINGLECAST);
  SET_ROUTED(rxFrame);
  cmdClass = 2;// dummy command class
  cmdClassCmd = 0x35; // dummy value
  dropFrame = IsFrameIlLegal(HDRFORMATTYP_2CH, &rxFrame, &cmdClass, &cmdClassCmd);
  TEST_ASSERT_FALSE(dropFrame);
  mock_calls_verify();

  /* Test that frames are not dropped if we are in receving replication */
  mock_calls_clear();
  memset((uint8_t*)&rxFrame, 0, sizeof(frame));

  mock_call_expect(TO_STR(ZW_IsVirtualNode),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = false;

  mock_call_expect(TO_STR(ZCB_GetNodeType),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = true;

  replicationStatus |= RECEIVER_CTRL_BIT;
  g_learnNodeState = LEARN_NODE_STATE_OFF;
  g_learnMode = 0;
  rxFrame.header.sourceID = 5;
  rxFrame.singlecastRouted.destinationID = 0x01;
  SET_HEADERTYPE(rxFrame, HDRTYP_SINGLECAST);
  SET_ROUTED(rxFrame);
  cmdClass = 2;// dummy command class
  cmdClassCmd = 0x35; // dummy value
  dropFrame = IsFrameIlLegal(HDRFORMATTYP_2CH, &rxFrame, &cmdClass, &cmdClassCmd);
  TEST_ASSERT_FALSE(dropFrame);
  mock_calls_verify();

  /* Test that frames are not dropped if we are adding nodes  and frame from unkwon node*/
  mock_calls_clear();
  memset((uint8_t*)&rxFrame, 0, sizeof(frame));

  mock_call_expect(TO_STR(ZW_IsVirtualNode),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = false;

  mock_call_expect(TO_STR(ZCB_GetNodeType),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = false;

  replicationStatus = 0;
  g_learnNodeState = LEARN_NODE_STATE_NEW;
  g_learnMode = 0;
  rxFrame.header.sourceID = 5;
  rxFrame.singlecastRouted.destinationID = 0x01;
  SET_HEADERTYPE(rxFrame, HDRTYP_SINGLECAST);
  SET_ROUTED(rxFrame);
  cmdClass = 2;// dummy command class
  cmdClassCmd = 0x35; // dummy value
  dropFrame = IsFrameIlLegal(HDRFORMATTYP_2CH, &rxFrame, &cmdClass, &cmdClassCmd);
  TEST_ASSERT_FALSE(dropFrame);
  mock_calls_verify();

  /* Test that frames are not dropped if we are removing nodes  and nframe from unkwon node*/
  mock_calls_clear();
  memset((uint8_t*)&rxFrame, 0, sizeof(frame));

  mock_call_expect(TO_STR(ZW_IsVirtualNode),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = false;

  mock_call_expect(TO_STR(ZCB_GetNodeType),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = false;

  replicationStatus = 0;
  g_learnNodeState = LEARN_NODE_STATE_DELETE;
  g_learnMode = 0;
  rxFrame.header.sourceID = 5;
  rxFrame.singlecastRouted.destinationID = 0x01;
  SET_HEADERTYPE(rxFrame, HDRTYP_SINGLECAST);
  SET_ROUTED(rxFrame);
  cmdClass = 2;// dummy command class
  cmdClassCmd = 0x35; // dummy value
  dropFrame = IsFrameIlLegal(HDRFORMATTYP_2CH, &rxFrame, &cmdClass, &cmdClassCmd);
  TEST_ASSERT_FALSE(dropFrame);
  mock_calls_verify();

  /* Test that frames are not dropped if we are updating nodes  and nframe from unkwon node*/
  mock_calls_clear();
  memset((uint8_t*)&rxFrame, 0, sizeof(frame));

  mock_call_expect(TO_STR(ZW_IsVirtualNode),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = false;

  mock_call_expect(TO_STR(ZCB_GetNodeType),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = false;

  replicationStatus = 0;
  g_learnNodeState = LEARN_NODE_STATE_UPDATE;
  g_learnMode = 0;
  rxFrame.header.sourceID = 5;
  rxFrame.singlecastRouted.destinationID = 0x01;
  SET_HEADERTYPE(rxFrame, HDRTYP_SINGLECAST);
  SET_ROUTED(rxFrame);
  cmdClass = 2;// dummy command class
  cmdClassCmd = 0x35; // dummy value
  dropFrame = IsFrameIlLegal(HDRFORMATTYP_2CH, &rxFrame, &cmdClass, &cmdClassCmd);
  TEST_ASSERT_FALSE(dropFrame);
  mock_calls_verify();

  /* Test that frames are not dropped if we are in smartstart and frame from unkwon node and frame is routed */
  mock_calls_clear();
  memset((uint8_t*)&rxFrame, 0, sizeof(frame));

  mock_call_expect(TO_STR(ZW_IsVirtualNode),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = false;

  mock_call_expect(TO_STR(ZCB_GetNodeType),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = false;

  replicationStatus = 0;
  g_learnNodeState = LEARN_NODE_STATE_SMART_START;
  g_learnMode = 0;
  rxFrame.header.sourceID = 5;
  rxFrame.singlecastRouted.destinationID = 0x01;
  SET_HEADERTYPE(rxFrame, HDRTYP_SINGLECAST);
  SET_ROUTED(rxFrame);
  cmdClass = 2;// dummy command class
  cmdClassCmd = 0x35; // dummy value
  dropFrame = IsFrameIlLegal(HDRFORMATTYP_2CH, &rxFrame, &cmdClass, &cmdClassCmd);
  TEST_ASSERT_FALSE(dropFrame);
  mock_calls_verify();

  /* Test that frames are not dropped if we are in smartstart  and frame from known node*/
  mock_calls_clear();
  memset((uint8_t*)&rxFrame, 0, sizeof(frame));

  mock_call_expect(TO_STR(ZW_IsVirtualNode),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = false;

  mock_call_expect(TO_STR(ZCB_GetNodeType),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = true;

  replicationStatus = 0;
  g_learnNodeState = LEARN_NODE_STATE_SMART_START;
  g_learnMode = 0;
  rxFrame.header.sourceID = 5;
  rxFrame.singlecastRouted.destinationID = 0x01;
  SET_HEADERTYPE(rxFrame, HDRTYP_SINGLECAST);
  SET_ROUTED(rxFrame);
  cmdClass = 2;// dummy command class
  cmdClassCmd = 0x35; // dummy value
  dropFrame = IsFrameIlLegal(HDRFORMATTYP_2CH, &rxFrame, &cmdClass, &cmdClassCmd);
  TEST_ASSERT_FALSE(dropFrame);
  mock_calls_verify();

  /* Test that NOP POWER framer are not dropped */
  mock_calls_clear();
  memset((uint8_t*)&rxFrame, 0, sizeof(frame));

  mock_call_expect(TO_STR(ZW_IsVirtualNode),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = false;

  mock_call_expect(TO_STR(ZCB_GetNodeType),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = false;

  replicationStatus = 0;
  g_learnNodeState = LEARN_NODE_STATE_SMART_START;
  g_learnMode = 0;
  rxFrame.header.sourceID = 5;
  rxFrame.singlecastRouted.destinationID = 0x01;
  SET_HEADERTYPE(rxFrame, HDRTYP_SINGLECAST);
  SET_ROUTED(rxFrame);
  cmdClass = 1;
  cmdClassCmd = ZWAVE_CMD_NOP_POWER;
  dropFrame = IsFrameIlLegal(HDRFORMATTYP_2CH, &rxFrame, &cmdClass, &cmdClassCmd);
  TEST_ASSERT_FALSE(dropFrame);
  mock_calls_verify();

  mock_calls_clear();
  memset((uint8_t*)&rxFrame, 0, sizeof(frame));

  mock_call_expect(TO_STR(ZW_IsVirtualNode),&pMock);
  pMock->expect_arg[ARG0].value = 1;
  pMock->return_code.value = false;

  replicationStatus = 0;
  g_learnNodeState = LEARN_NODE_STATE_SMART_START;
  g_learnMode = 0;
  rxFrame.header.sourceID = 1;
  rxFrame.singlecastRouted.destinationID = 0x01;
  SET_HEADERTYPE(rxFrame, HDRTYP_SINGLECAST);
  SET_ROUTED(rxFrame);
  cmdClass = 1;
  cmdClassCmd = ZWAVE_CMD_NOP_POWER;
  dropFrame = IsFrameIlLegal(HDRFORMATTYP_2CH, &rxFrame, &cmdClass, &cmdClassCmd);
  TEST_ASSERT_FALSE(dropFrame);
  mock_calls_verify();

  /* Test that ZWAVE_CMD_TRANSFER_PRESENTATION framer are not dropped */
  mock_calls_clear();
  memset((uint8_t*)&rxFrame, 0, sizeof(frame));

  mock_call_expect(TO_STR(ZW_IsVirtualNode),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = false;

  mock_call_expect(TO_STR(ZCB_GetNodeType),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = false;

  replicationStatus = 0;
  g_learnNodeState = LEARN_NODE_STATE_SMART_START;
  g_learnMode = 0;
  rxFrame.header.sourceID = 5;
  rxFrame.singlecastRouted.destinationID = 0x01;
  SET_HEADERTYPE(rxFrame, HDRTYP_SINGLECAST);
  SET_ROUTED(rxFrame);
  cmdClass = 1;
  cmdClassCmd = ZWAVE_CMD_TRANSFER_PRESENTATION;
  dropFrame = IsFrameIlLegal(HDRFORMATTYP_2CH, &rxFrame, &cmdClass, &cmdClassCmd);
  TEST_ASSERT_FALSE(dropFrame);
  mock_calls_verify();

  mock_calls_clear();
  memset((uint8_t*)&rxFrame, 0, sizeof(frame));

  mock_call_expect(TO_STR(ZW_IsVirtualNode),&pMock);
  pMock->expect_arg[ARG0].value = 1;
  pMock->return_code.value = false;

  replicationStatus = 0;
  g_learnNodeState = LEARN_NODE_STATE_SMART_START;
  g_learnMode = 0;
  rxFrame.header.sourceID = 1;
  rxFrame.singlecastRouted.destinationID = 0x01;
  SET_HEADERTYPE(rxFrame, HDRTYP_SINGLECAST);
  SET_ROUTED(rxFrame);
  cmdClass = 1;
  cmdClassCmd = ZWAVE_CMD_TRANSFER_PRESENTATION;
  dropFrame = IsFrameIlLegal(HDRFORMATTYP_2CH, &rxFrame, &cmdClass, &cmdClassCmd);
  TEST_ASSERT_FALSE(dropFrame);
  mock_calls_verify();
}


/*
 * Test that IsFrameIlLegal filter the received routed  frames correctly in 3ch system
 */
void test_IsFrameIlLegal_routed_3ch(void)
{
  frame rxFrame;
  bool dropFrame;
  uint8_t cmdClass;
  uint8_t cmdClassCmd;
  mock_t * pMock;

  /* Test that frame is dropped if  sourceID == g_nodeID and the frame is COMAND_CLASS_SECUROTY*/
  mock_calls_clear();
  memset((uint8_t*)&rxFrame, 0, sizeof(frame));

  replicationStatus = 0;
  g_learnNodeState = LEARN_NODE_STATE_OFF;
  g_learnMode = 0;
  rxFrame.header.sourceID = g_nodeID;
  rxFrame.singlecastRouted3ch.destinationID = g_nodeID;
  SET_HEADERTYPE(rxFrame, HDRTYP_ROUTED);

  cmdClass = COMMAND_CLASS_SECURITY;
  cmdClassCmd = 0x35; // dummy value
  dropFrame = IsFrameIlLegal(HDRFORMATTYP_3CH, &rxFrame, &cmdClass, &cmdClassCmd);
  TEST_ASSERT_TRUE(dropFrame);
  mock_calls_verify();

  /* Test that frame is dropped if  sourceID == g_nodeID and the frame is COMAND_CLASS_SECUROTY even if we are in learmMode*/
  mock_calls_clear();
  memset((uint8_t*)&rxFrame, 0, sizeof(frame));

  replicationStatus = 0;
  g_learnNodeState = LEARN_NODE_STATE_OFF;
  g_learnMode = 1;
  rxFrame.header.sourceID = g_nodeID;
  rxFrame.singlecastRouted3ch.destinationID = g_nodeID;
  SET_HEADERTYPE(rxFrame, HDRTYP_ROUTED);
  cmdClass = COMMAND_CLASS_SECURITY;
  cmdClassCmd = 0x35; // dummy value
  dropFrame = IsFrameIlLegal(HDRFORMATTYP_3CH, &rxFrame, &cmdClass, &cmdClassCmd);
  TEST_ASSERT_TRUE(dropFrame);
  mock_calls_verify();

/* Test that frame is dropped if  sourceID == g_nodeID and the frame is COMAND_CLASS_SECUROTY_2*/
  mock_calls_clear();
  memset((uint8_t*)&rxFrame, 0, sizeof(frame));

  replicationStatus = 0;
  g_learnNodeState = LEARN_NODE_STATE_OFF;
  g_learnMode = 0;
  rxFrame.header.sourceID = g_nodeID;
  rxFrame.singlecastRouted3ch.destinationID = g_nodeID;
  SET_HEADERTYPE(rxFrame, HDRTYP_ROUTED);
  cmdClass = COMMAND_CLASS_SECURITY_2;
  cmdClassCmd = 0x35; // dummy value
  dropFrame = IsFrameIlLegal(HDRFORMATTYP_3CH, &rxFrame, &cmdClass, &cmdClassCmd);
  TEST_ASSERT_TRUE(dropFrame);
  mock_calls_verify();

  /* Test that frame is dropped if  sourceID == g_nodeID and the frame is COMAND_CLASS_SECUROTY_2 even if we are in learmMode*/
  mock_calls_clear();
  memset((uint8_t*)&rxFrame, 0, sizeof(frame));

  replicationStatus = 0;
  g_learnNodeState = LEARN_NODE_STATE_OFF;
  g_learnMode = 1;
  rxFrame.header.sourceID = g_nodeID;
  rxFrame.singlecastRouted3ch.destinationID = g_nodeID;

  SET_HEADERTYPE(rxFrame, HDRTYP_ROUTED);
  cmdClass = COMMAND_CLASS_SECURITY_2;
  cmdClassCmd = 0x35; // dummy value
  dropFrame = IsFrameIlLegal(HDRFORMATTYP_3CH, &rxFrame, &cmdClass, &cmdClassCmd);
  TEST_ASSERT_TRUE(dropFrame);
  mock_calls_verify();

  /* Test that frame is dropped if  destinationID == 0xFF and the frame is COMAND_CLASS_SECUROTY*/
  mock_calls_clear();
  memset((uint8_t*)&rxFrame, 0, sizeof(frame));

  replicationStatus = 0;
  g_learnNodeState = LEARN_NODE_STATE_OFF;
  g_learnMode = 0;
  rxFrame.header.sourceID = 5;
  rxFrame.singlecastRouted3ch.destinationID = 0xFF;
  SET_HEADERTYPE(rxFrame, HDRTYP_ROUTED);
  cmdClass = COMMAND_CLASS_SECURITY;
  cmdClassCmd = 0x35; // dummy value
  dropFrame = IsFrameIlLegal(HDRFORMATTYP_3CH, &rxFrame, &cmdClass, &cmdClassCmd);
  TEST_ASSERT_TRUE(dropFrame);
  mock_calls_verify();

  /* Test that frame is not dropped if sourceID != node but it is unknown and frame is routed */
  mock_calls_clear();
  memset((uint8_t*)&rxFrame, 0, sizeof(frame));

  mock_call_expect(TO_STR(ZW_IsVirtualNode),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = false;

  mock_call_expect(TO_STR(ZCB_GetNodeType),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = false;

  replicationStatus = 0;
  g_learnNodeState = LEARN_NODE_STATE_OFF;
  g_learnMode = 0;
  rxFrame.header.sourceID = 5;
  rxFrame.singlecastRouted3ch.destinationID = 0x01;
  SET_HEADERTYPE(rxFrame, HDRTYP_ROUTED);
  cmdClass = 2;// dummy command class
  cmdClassCmd = 0x35; // dummy value
  dropFrame = IsFrameIlLegal(HDRFORMATTYP_3CH, &rxFrame, &cmdClass, &cmdClassCmd);
  TEST_ASSERT_FALSE(dropFrame);
  mock_calls_verify();

  /* Test that frame is dropped if  sourceID == node and destination = g_nodeID*/
  mock_calls_clear();
  memset((uint8_t*)&rxFrame, 0, sizeof(frame));

  mock_call_expect(TO_STR(ZW_IsVirtualNode),&pMock);
  pMock->expect_arg[ARG0].value = 0x01;
  pMock->return_code.value = false;

  replicationStatus = 0;
  g_learnNodeState = LEARN_NODE_STATE_OFF;
  g_learnMode = 0;
  rxFrame.header.sourceID = 0x01;
  rxFrame.singlecastRouted3ch.destinationID = 0x01;
  SET_HEADERTYPE(rxFrame, HDRTYP_SINGLECAST);
  cmdClass = 2;// dummy command class
  cmdClassCmd = 0x35; // dummy value
  dropFrame = IsFrameIlLegal(HDRFORMATTYP_3CH, &rxFrame, &cmdClass, &cmdClassCmd);
  TEST_ASSERT_TRUE(dropFrame);
  mock_calls_verify();

  /* Test that frame is node dropped if  sourceID == node and destination != g_nodeID*/
  mock_calls_clear();
  memset((uint8_t*)&rxFrame, 0, sizeof(frame));

  mock_call_expect(TO_STR(ZW_IsVirtualNode),&pMock);
  pMock->expect_arg[ARG0].value = 0x01;
  pMock->return_code.value = false;

  mock_call_expect(TO_STR(ZCB_GetNodeType),&pMock);
  pMock->expect_arg[ARG0].value = 0x01;
  pMock->return_code.value = true;

  replicationStatus = 0;
  g_learnNodeState = LEARN_NODE_STATE_OFF;
  g_learnMode = 0;
  rxFrame.header.sourceID = 0x01;
  rxFrame.singlecastRouted3ch.destinationID = 0x05;
  SET_HEADERTYPE(rxFrame, HDRTYP_ROUTED);
  cmdClass = 2;// dummy command class
  cmdClassCmd = 0x35; // dummy value
  dropFrame = IsFrameIlLegal(HDRFORMATTYP_3CH, &rxFrame, &cmdClass, &cmdClassCmd);
  TEST_ASSERT_FALSE(dropFrame);
  mock_calls_verify();

  /* Test that frames are not dropped if we re in learnMode */
  mock_calls_clear();
  memset((uint8_t*)&rxFrame, 0, sizeof(frame));

  mock_call_expect(TO_STR(ZW_IsVirtualNode),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = false;

  mock_call_expect(TO_STR(ZCB_GetNodeType),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = true;

  replicationStatus = 0;
  g_learnNodeState = LEARN_NODE_STATE_OFF;
  g_learnMode = 1;
  rxFrame.header.sourceID = 5;
  rxFrame.singlecastRouted3ch.destinationID = 0x01;

  SET_HEADERTYPE(rxFrame, HDRTYP_ROUTED);
  cmdClass = 2;// dummy command class
  cmdClassCmd = 0x35; // dummy value
  dropFrame = IsFrameIlLegal(HDRFORMATTYP_3CH, &rxFrame, &cmdClass, &cmdClassCmd);
  TEST_ASSERT_FALSE(dropFrame);
  mock_calls_verify();

  /* Test that frames are not dropped if we are in receving replication */
  mock_calls_clear();
  memset((uint8_t*)&rxFrame, 0, sizeof(frame));

  mock_call_expect(TO_STR(ZW_IsVirtualNode),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = false;

  mock_call_expect(TO_STR(ZCB_GetNodeType),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = true;

  replicationStatus |= RECEIVER_CTRL_BIT;
  g_learnNodeState = LEARN_NODE_STATE_OFF;
  g_learnMode = 0;
  rxFrame.header.sourceID = 5;
  rxFrame.singlecastRouted3ch.destinationID = 0x01;

  SET_HEADERTYPE(rxFrame, HDRTYP_ROUTED);
  cmdClass = 2;// dummy command class
  cmdClassCmd = 0x35; // dummy value
  dropFrame = IsFrameIlLegal(HDRFORMATTYP_3CH, &rxFrame, &cmdClass, &cmdClassCmd);
  TEST_ASSERT_FALSE(dropFrame);
  mock_calls_verify();


  /* Test that frames are not dropped if we are adding nodes  and frame from unkwon node*/
  mock_calls_clear();
  memset((uint8_t*)&rxFrame, 0, sizeof(frame));

  mock_call_expect(TO_STR(ZW_IsVirtualNode),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = false;

  mock_call_expect(TO_STR(ZCB_GetNodeType),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = false;

  replicationStatus = 0;
  g_learnNodeState = LEARN_NODE_STATE_NEW;
  g_learnMode = 0;
  rxFrame.header.sourceID = 5;
  rxFrame.singlecastRouted3ch.destinationID = 0x01;

  SET_HEADERTYPE(rxFrame, HDRTYP_ROUTED);
  cmdClass = 2;// dummy command class
  cmdClassCmd = 0x35; // dummy value
  dropFrame = IsFrameIlLegal(HDRFORMATTYP_3CH, &rxFrame, &cmdClass, &cmdClassCmd);
  TEST_ASSERT_FALSE(dropFrame);
  mock_calls_verify();

  /* Test that frames are not dropped if we are removing nodes  and nframe from unkwon node*/
  mock_calls_clear();
  memset((uint8_t*)&rxFrame, 0, sizeof(frame));

  mock_call_expect(TO_STR(ZW_IsVirtualNode),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = false;

  mock_call_expect(TO_STR(ZCB_GetNodeType),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = false;

  replicationStatus = 0;
  g_learnNodeState = LEARN_NODE_STATE_DELETE;
  g_learnMode = 0;
  rxFrame.header.sourceID = 5;
  rxFrame.singlecastRouted3ch.destinationID = 0x01;
  SET_HEADERTYPE(rxFrame, HDRTYP_ROUTED);

  cmdClass = 2;// dummy command class
  cmdClassCmd = 0x35; // dummy value
  dropFrame = IsFrameIlLegal(HDRFORMATTYP_3CH, &rxFrame, &cmdClass, &cmdClassCmd);
  TEST_ASSERT_FALSE(dropFrame);
  mock_calls_verify();

  /* Test that frames are not dropped if we are updating nodes  and nframe from unkwon node*/
  mock_calls_clear();
  memset((uint8_t*)&rxFrame, 0, sizeof(frame));

  mock_call_expect(TO_STR(ZW_IsVirtualNode),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = false;

  mock_call_expect(TO_STR(ZCB_GetNodeType),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = false;

  replicationStatus = 0;
  g_learnNodeState = LEARN_NODE_STATE_UPDATE;
  g_learnMode = 0;
  rxFrame.header.sourceID = 5;
  rxFrame.singlecastRouted3ch.destinationID = 0x01;
  SET_HEADERTYPE(rxFrame, HDRTYP_ROUTED);

  cmdClass = 2;// dummy command class
  cmdClassCmd = 0x35; // dummy value
  dropFrame = IsFrameIlLegal(HDRFORMATTYP_3CH, &rxFrame, &cmdClass, &cmdClassCmd);
  TEST_ASSERT_FALSE(dropFrame);
  mock_calls_verify();

  /* Test that frames are not dropped if we are in smartstart and frame from unkwon node and frame is routed */
  mock_calls_clear();
  memset((uint8_t*)&rxFrame, 0, sizeof(frame));

  mock_call_expect(TO_STR(ZW_IsVirtualNode),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = false;

  mock_call_expect(TO_STR(ZCB_GetNodeType),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = false;

  replicationStatus = 0;
  g_learnNodeState = LEARN_NODE_STATE_SMART_START;
  g_learnMode = 0;
  rxFrame.header.sourceID = 5;
  rxFrame.singlecastRouted3ch.destinationID = 0x01;
  SET_HEADERTYPE(rxFrame, HDRTYP_ROUTED);

  cmdClass = 2;// dummy command class
  cmdClassCmd = 0x35; // dummy value
  dropFrame = IsFrameIlLegal(HDRFORMATTYP_3CH, &rxFrame, &cmdClass, &cmdClassCmd);
  TEST_ASSERT_FALSE(dropFrame);
  mock_calls_verify();

  /* Test that frames are not dropped if we are in smartstart  and frame from known node*/
  mock_calls_clear();
  memset((uint8_t*)&rxFrame, 0, sizeof(frame));

  mock_call_expect(TO_STR(ZW_IsVirtualNode),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = false;

  mock_call_expect(TO_STR(ZCB_GetNodeType),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = true;

  replicationStatus = 0;
  g_learnNodeState = LEARN_NODE_STATE_SMART_START;
  g_learnMode = 0;
  rxFrame.header.sourceID = 5;
  rxFrame.singlecastRouted3ch.destinationID = 0x01;
  SET_HEADERTYPE(rxFrame, HDRTYP_ROUTED);

  cmdClass = 2;// dummy command class
  cmdClassCmd = 0x35; // dummy value
  dropFrame = IsFrameIlLegal(HDRFORMATTYP_3CH, &rxFrame, &cmdClass, &cmdClassCmd);
  TEST_ASSERT_FALSE(dropFrame);
  mock_calls_verify();

  /* Test that NOP POWER framer are not dropped */
  mock_calls_clear();
  memset((uint8_t*)&rxFrame, 0, sizeof(frame));

  mock_call_expect(TO_STR(ZW_IsVirtualNode),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = false;

  mock_call_expect(TO_STR(ZCB_GetNodeType),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = false;

  replicationStatus = 0;
  g_learnNodeState = LEARN_NODE_STATE_SMART_START;
  g_learnMode = 0;
  rxFrame.header.sourceID = 5;
  rxFrame.singlecastRouted3ch.destinationID = 0x01;
  SET_HEADERTYPE(rxFrame, HDRTYP_ROUTED);

  cmdClass = 1;
  cmdClassCmd = ZWAVE_CMD_NOP_POWER;
  dropFrame = IsFrameIlLegal(HDRFORMATTYP_3CH, &rxFrame, &cmdClass, &cmdClassCmd);
  TEST_ASSERT_FALSE(dropFrame);
  mock_calls_verify();

  mock_calls_clear();
  memset((uint8_t*)&rxFrame, 0, sizeof(frame));
  
  mock_call_expect(TO_STR(ZW_IsVirtualNode),&pMock);
  pMock->expect_arg[ARG0].value = 1;
  pMock->return_code.value = false;

  replicationStatus = 0;
  g_learnNodeState = LEARN_NODE_STATE_SMART_START;
  g_learnMode = 0;
  rxFrame.header.sourceID = 1;
  rxFrame.singlecastRouted3ch.destinationID = 0x01;
  SET_HEADERTYPE(rxFrame, HDRTYP_ROUTED);
  cmdClass = 1;
  cmdClassCmd = ZWAVE_CMD_NOP_POWER;
  dropFrame = IsFrameIlLegal(HDRFORMATTYP_3CH, &rxFrame, &cmdClass, &cmdClassCmd);
  TEST_ASSERT_FALSE(dropFrame);
  mock_calls_verify();

  /* Test that ZWAVE_CMD_TRANSFER_PRESENTATION framer are not dropped */
  mock_calls_clear();
  memset((uint8_t*)&rxFrame, 0, sizeof(frame));

  mock_call_expect(TO_STR(ZW_IsVirtualNode),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = false;

  mock_call_expect(TO_STR(ZCB_GetNodeType),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = false;

  replicationStatus = 0;
  g_learnNodeState = LEARN_NODE_STATE_SMART_START;
  g_learnMode = 0;
  rxFrame.header.sourceID = 5;
  rxFrame.singlecastRouted3ch.destinationID = 0x01;
  SET_HEADERTYPE(rxFrame, HDRTYP_ROUTED);

  cmdClass = 1;
  cmdClassCmd = ZWAVE_CMD_TRANSFER_PRESENTATION;
  dropFrame = IsFrameIlLegal(HDRFORMATTYP_3CH, &rxFrame, &cmdClass, &cmdClassCmd);
  TEST_ASSERT_FALSE(dropFrame);
  mock_calls_verify();

  mock_calls_clear();
  memset((uint8_t*)&rxFrame, 0, sizeof(frame));

  mock_call_expect(TO_STR(ZW_IsVirtualNode),&pMock);
  pMock->expect_arg[ARG0].value = 1;
  pMock->return_code.value = false;

  replicationStatus = 0;
  g_learnNodeState = LEARN_NODE_STATE_SMART_START;
  g_learnMode = 0;
  rxFrame.header.sourceID = 1;
  rxFrame.singlecastRouted3ch.destinationID = 0x01;
  SET_HEADERTYPE(rxFrame, HDRTYP_ROUTED);
  cmdClass = 1;
  cmdClassCmd = ZWAVE_CMD_TRANSFER_PRESENTATION;
  dropFrame = IsFrameIlLegal(HDRFORMATTYP_3CH, &rxFrame, &cmdClass, &cmdClassCmd);
  TEST_ASSERT_FALSE(dropFrame);
  mock_calls_verify();

}

/*
 * Test that IsFrameIlLegal filter the received routed  frames correctly
 */
void test_IsFrameIlLegal_explore_2ch(void)
{
  frame rxFrame;
  bool dropFrame;
  uint8_t cmdClass;
  uint8_t cmdClassCmd;
  mock_t * pMock;

  /* Test that frame is dropped if  sourceID != node but it is unknown*/
  mock_calls_clear();
  memset((uint8_t*)&rxFrame, 0, sizeof(frame));

  mock_call_expect(TO_STR(ZW_IsVirtualNode),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = false;

  mock_call_expect(TO_STR(ZCB_GetNodeType),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = false;

  replicationStatus = 0;
  g_learnNodeState = LEARN_NODE_STATE_OFF;
  g_learnMode = 0;
  rxFrame.header.sourceID = 5;
  SET_HEADERTYPE(rxFrame, HDRTYP_EXPLORE);
  cmdClass = 2;// dummy command class
  cmdClassCmd = 0x35; // dummy value
  dropFrame = IsFrameIlLegal(HDRFORMATTYP_2CH, &rxFrame, &cmdClass, &cmdClassCmd);
  TEST_ASSERT_TRUE(dropFrame);
  mock_calls_verify();  

  /* Test that frames are not dropped if we re in learnMode */
  mock_calls_clear();
  memset((uint8_t*)&rxFrame, 0, sizeof(frame));

  mock_call_expect(TO_STR(ZW_IsVirtualNode),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = false;

  mock_call_expect(TO_STR(ZCB_GetNodeType),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = true;

  replicationStatus = 0;
  g_learnNodeState = LEARN_NODE_STATE_OFF;
  g_learnMode = 1;
  rxFrame.header.sourceID = 5;
  SET_HEADERTYPE(rxFrame, HDRTYP_EXPLORE);
  cmdClass = 2;// dummy command class
  cmdClassCmd = 0x35; // dummy value
  dropFrame = IsFrameIlLegal(HDRFORMATTYP_2CH, &rxFrame, &cmdClass, &cmdClassCmd);
  TEST_ASSERT_FALSE(dropFrame);
  mock_calls_verify();

  /* Test that frames are not dropped if we are in receving replication */
  mock_calls_clear();
  memset((uint8_t*)&rxFrame, 0, sizeof(frame));

  mock_call_expect(TO_STR(ZW_IsVirtualNode),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = false;

  mock_call_expect(TO_STR(ZCB_GetNodeType),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = true;

  replicationStatus |= RECEIVER_CTRL_BIT;
  g_learnNodeState = LEARN_NODE_STATE_OFF;
  g_learnMode = 0;
  rxFrame.header.sourceID = 5;
  SET_HEADERTYPE(rxFrame, HDRTYP_EXPLORE);
  cmdClass = 2;// dummy command class
  cmdClassCmd = 0x35; // dummy value
  dropFrame = IsFrameIlLegal(HDRFORMATTYP_2CH, &rxFrame, &cmdClass, &cmdClassCmd);
  TEST_ASSERT_FALSE(dropFrame);
  mock_calls_verify();


  /* Test that frames are not dropped if we are adding nodes  and frame from unkwon node*/
  mock_calls_clear();
  memset((uint8_t*)&rxFrame, 0, sizeof(frame));

  mock_call_expect(TO_STR(ZW_IsVirtualNode),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = false;

  mock_call_expect(TO_STR(ZCB_GetNodeType),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = false;

  replicationStatus = 0;
  g_learnNodeState = LEARN_NODE_STATE_NEW;
  g_learnMode = 0;
  rxFrame.header.sourceID = 5;
  SET_HEADERTYPE(rxFrame, HDRTYP_EXPLORE);
  cmdClass = 2;// dummy command class
  cmdClassCmd = 0x35; // dummy value
  dropFrame = IsFrameIlLegal(HDRFORMATTYP_2CH, &rxFrame, &cmdClass, &cmdClassCmd);
  TEST_ASSERT_FALSE(dropFrame);
  mock_calls_verify();

  /* Test that frames are not dropped if we are removing nodes  and nframe from unkwon node*/
  mock_calls_clear();
  memset((uint8_t*)&rxFrame, 0, sizeof(frame));

  mock_call_expect(TO_STR(ZW_IsVirtualNode),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = false;

  mock_call_expect(TO_STR(ZCB_GetNodeType),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = false;

  replicationStatus = 0;
  g_learnNodeState = LEARN_NODE_STATE_DELETE;
  g_learnMode = 0;
  rxFrame.header.sourceID = 5;
  SET_HEADERTYPE(rxFrame, HDRTYP_EXPLORE);
  cmdClass = 2;// dummy command class
  cmdClassCmd = 0x35; // dummy value
  dropFrame = IsFrameIlLegal(HDRFORMATTYP_2CH, &rxFrame, &cmdClass, &cmdClassCmd);
  TEST_ASSERT_FALSE(dropFrame);
  mock_calls_verify();

  /* Test that frames are not dropped if we are updating nodes  and nframe from unkwon node*/
  mock_calls_clear();
  memset((uint8_t*)&rxFrame, 0, sizeof(frame));

  mock_call_expect(TO_STR(ZW_IsVirtualNode),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = false;

  mock_call_expect(TO_STR(ZCB_GetNodeType),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = false;

  replicationStatus = 0;
  g_learnNodeState = LEARN_NODE_STATE_UPDATE;
  g_learnMode = 0;
  rxFrame.header.sourceID = 5;
  SET_HEADERTYPE(rxFrame, HDRTYP_EXPLORE);
  cmdClass = 2;// dummy command class
  cmdClassCmd = 0x35; // dummy value
  dropFrame = IsFrameIlLegal(HDRFORMATTYP_2CH, &rxFrame, &cmdClass, &cmdClassCmd);
  TEST_ASSERT_FALSE(dropFrame);
  mock_calls_verify();

  /* Test that frames are dropped if we are in smartstart  and node is SIS and frame is execlude requeest*/
  mock_calls_clear();
  memset((uint8_t*)&rxFrame, 0, sizeof(frame));

  staticControllerNodeID = g_nodeID;
  nodeIdserverPresent = true;
  
  mock_call_expect(TO_STR(ZW_IsVirtualNode),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = false;
  mock_call_expect(TO_STR(ZCB_GetNodeType),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = false;

  replicationStatus = 0;
  g_learnNodeState = LEARN_NODE_STATE_SMART_START;
  g_learnMode = 0;
  rxFrame.header.sourceID = 5;
  rxFrame.explore.ver_Cmd = EXPLORE_CMD_AUTOINCLUSION;
  ((EXPLORE_REMOTE_INCLUSION_REQUEST_FRAME*)EXPLORE_PAYLOAD(rxFrame))->header.cmd = ZWAVE_CMD_EXCLUDE_REQUEST;
  SET_HEADERTYPE(rxFrame, HDRTYP_EXPLORE);

  cmdClass = 2;// dummy command class
  cmdClassCmd = 0x35; // dummy value
  dropFrame = IsFrameIlLegal(HDRFORMATTYP_2CH, &rxFrame, &cmdClass, &cmdClassCmd);
  TEST_ASSERT_TRUE(dropFrame);
  mock_calls_verify();

  /* Test that frames are not dropped if we are in smartstart  and node is SIS and frame is include requeest*/
  mock_calls_clear();
  memset((uint8_t*)&rxFrame, 0, sizeof(frame));

  staticControllerNodeID = g_nodeID;
  nodeIdserverPresent = true;
  
  mock_call_expect(TO_STR(ZW_IsVirtualNode),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = false;

  mock_call_expect(TO_STR(ZCB_GetNodeType),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = false;

  replicationStatus = 0;
  g_learnNodeState = LEARN_NODE_STATE_SMART_START;
  g_learnMode = 0;
  rxFrame.header.sourceID = 5;
  rxFrame.explore.ver_Cmd = EXPLORE_CMD_AUTOINCLUSION;
  ((EXPLORE_REMOTE_INCLUSION_REQUEST_FRAME*)EXPLORE_PAYLOAD(rxFrame))->header.cmd = ZWAVE_CMD_INCLUDED_NODE_INFO;
  SET_HEADERTYPE(rxFrame, HDRTYP_EXPLORE);

  cmdClass = 2;// dummy command class
  cmdClassCmd = 0x35; // dummy value
  dropFrame = IsFrameIlLegal(HDRFORMATTYP_2CH, &rxFrame, &cmdClass, &cmdClassCmd);
  TEST_ASSERT_FALSE(dropFrame);
  mock_calls_verify();

  /* Test that frames are not dropped if we are in smartstart  and node is not SIS and frame is include requeest*/
  mock_calls_clear();
  memset((uint8_t*)&rxFrame, 0, sizeof(frame));

  staticControllerNodeID = g_nodeID;
  nodeIdserverPresent = false;
  mock_call_expect(TO_STR(ZW_IsVirtualNode),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = false;

  mock_call_expect(TO_STR(ZCB_GetNodeType),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = false;

  replicationStatus = 0;
  g_learnNodeState = LEARN_NODE_STATE_SMART_START;
  g_learnMode = 0;
  rxFrame.header.sourceID = 5;
  rxFrame.explore.ver_Cmd = EXPLORE_CMD_AUTOINCLUSION;
  ((EXPLORE_REMOTE_INCLUSION_REQUEST_FRAME*)EXPLORE_PAYLOAD(rxFrame))->header.cmd = ZWAVE_CMD_INCLUDED_NODE_INFO;
  SET_HEADERTYPE(rxFrame, HDRTYP_EXPLORE);

  cmdClass = 2;// dummy command class
  cmdClassCmd = 0x35; // dummy value
  dropFrame = IsFrameIlLegal(HDRFORMATTYP_2CH, &rxFrame, &cmdClass, &cmdClassCmd);
  TEST_ASSERT_FALSE(dropFrame);
  mock_calls_verify();


  /* Test that frames are not dropped if we are in smartstart  and frame from known node*/
  mock_calls_clear();
  memset((uint8_t*)&rxFrame, 0, sizeof(frame));

  staticControllerNodeID = g_nodeID;
  nodeIdserverPresent = false;
 
  mock_call_expect(TO_STR(ZW_IsVirtualNode),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = false;

  mock_call_expect(TO_STR(ZCB_GetNodeType),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = true;

  replicationStatus = 0;
  g_learnNodeState = LEARN_NODE_STATE_SMART_START;
  g_learnMode = 0;
  rxFrame.header.sourceID = 5;
  rxFrame.explore.ver_Cmd = EXPLORE_CMD_NORMAL;
  ((EXPLORE_REMOTE_INCLUSION_REQUEST_FRAME*)EXPLORE_PAYLOAD(rxFrame))->header.cmd = 10; //dummy
  SET_HEADERTYPE(rxFrame, HDRTYP_EXPLORE);

  cmdClass = 2;// dummy command class
  cmdClassCmd = 0x35; // dummy value
  dropFrame = IsFrameIlLegal(HDRFORMATTYP_2CH, &rxFrame, &cmdClass, &cmdClassCmd);
  TEST_ASSERT_FALSE(dropFrame);
  mock_calls_verify();

  /* Test that NOP POWER framer are not dropped */
  mock_calls_clear();
  memset((uint8_t*)&rxFrame, 0, sizeof(frame));
  
  mock_call_expect(TO_STR(ZW_IsVirtualNode),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = false;

  mock_call_expect(TO_STR(ZCB_GetNodeType),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = false;

  replicationStatus = 0;
  g_learnNodeState = LEARN_NODE_STATE_SMART_START;
  g_learnMode = 0;
  rxFrame.header.sourceID = 5;
  rxFrame.explore.ver_Cmd = EXPLORE_CMD_NORMAL;
  ((EXPLORE_REMOTE_INCLUSION_REQUEST_FRAME*)EXPLORE_PAYLOAD(rxFrame))->header.cmd = 10; //dummy
  SET_HEADERTYPE(rxFrame, HDRTYP_EXPLORE);
  cmdClass = 1;
  cmdClassCmd = ZWAVE_CMD_NOP_POWER;
  dropFrame = IsFrameIlLegal(HDRFORMATTYP_2CH, &rxFrame, &cmdClass, &cmdClassCmd);
  TEST_ASSERT_FALSE(dropFrame);
  mock_calls_verify();

  /* Test that ZWAVE_CMD_TRANSFER_PRESENTATION framer are not dropped */
  mock_calls_clear();
  memset((uint8_t*)&rxFrame, 0, sizeof(frame));

  mock_call_expect(TO_STR(ZW_IsVirtualNode),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = false;

  mock_call_expect(TO_STR(ZCB_GetNodeType),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = false;

  replicationStatus = 0;
  g_learnNodeState = LEARN_NODE_STATE_SMART_START;
  g_learnMode = 0;
  rxFrame.header.sourceID = 5;
  rxFrame.explore.ver_Cmd = EXPLORE_CMD_NORMAL;
  ((EXPLORE_REMOTE_INCLUSION_REQUEST_FRAME*)EXPLORE_PAYLOAD(rxFrame))->header.cmd = 10; //dummy
  SET_HEADERTYPE(rxFrame, HDRTYP_EXPLORE);
  cmdClass = 1;
  cmdClassCmd = ZWAVE_CMD_TRANSFER_PRESENTATION;
  dropFrame = IsFrameIlLegal(HDRFORMATTYP_2CH, &rxFrame, &cmdClass, &cmdClassCmd);
  TEST_ASSERT_FALSE(dropFrame);
  mock_calls_verify();
}

/*
 * Test that IsFrameIlLegal filter the received routed  frames correctly
 */
void test_IsFrameIlLegal_explore_3ch(void)
{
  frame rxFrame;
  bool dropFrame;
  uint8_t cmdClass;
  uint8_t cmdClassCmd;
  mock_t * pMock;

  /* Test that frame is dropped if  sourceID != node but it is unknown*/
  mock_calls_clear();
  memset((uint8_t*)&rxFrame, 0, sizeof(frame));

  mock_call_expect(TO_STR(ZW_IsVirtualNode),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = false;

  mock_call_expect(TO_STR(ZCB_GetNodeType),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = false;

  replicationStatus = 0;
  g_learnNodeState = LEARN_NODE_STATE_OFF;
  g_learnMode = 0;
  rxFrame.header.sourceID = 5;
  SET_HEADERTYPE(rxFrame, HDRTYP_EXPLORE);
  cmdClass = 2;// dummy command class
  cmdClassCmd = 0x35; // dummy value
  dropFrame = IsFrameIlLegal(HDRFORMATTYP_3CH, &rxFrame, &cmdClass, &cmdClassCmd);
  TEST_ASSERT_TRUE(dropFrame);
  mock_calls_verify();

  /* Test that frames are not dropped if we re in learnMode */
  mock_calls_clear();
  memset((uint8_t*)&rxFrame, 0, sizeof(frame));
  
  mock_call_expect(TO_STR(ZW_IsVirtualNode),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = false;

  mock_call_expect(TO_STR(ZCB_GetNodeType),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = true;

  replicationStatus = 0;
  g_learnNodeState = LEARN_NODE_STATE_OFF;
  g_learnMode = 1;
  rxFrame.header.sourceID = 5;
  SET_HEADERTYPE(rxFrame, HDRTYP_EXPLORE);
  cmdClass = 2;// dummy command class
  cmdClassCmd = 0x35; // dummy value
  dropFrame = IsFrameIlLegal(HDRFORMATTYP_3CH, &rxFrame, &cmdClass, &cmdClassCmd);
  TEST_ASSERT_FALSE(dropFrame);
  mock_calls_verify();

  /* Test that frames are not dropped if we are in receving replication */
  mock_calls_clear();
  memset((uint8_t*)&rxFrame, 0, sizeof(frame));

  mock_call_expect(TO_STR(ZW_IsVirtualNode),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = false;

  mock_call_expect(TO_STR(ZCB_GetNodeType),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = true;

  replicationStatus |= RECEIVER_CTRL_BIT;
  g_learnNodeState = LEARN_NODE_STATE_OFF;
  g_learnMode = 0;
  rxFrame.header.sourceID = 5;
  SET_HEADERTYPE(rxFrame, HDRTYP_EXPLORE);
  cmdClass = 2;// dummy command class
  cmdClassCmd = 0x35; // dummy value
  dropFrame = IsFrameIlLegal(HDRFORMATTYP_3CH, &rxFrame, &cmdClass, &cmdClassCmd);
  TEST_ASSERT_FALSE(dropFrame);
  mock_calls_verify();

  /* Test that frames are not dropped if we are adding nodes  and frame from unkwon node*/
  mock_calls_clear();
  memset((uint8_t*)&rxFrame, 0, sizeof(frame));

  mock_call_expect(TO_STR(ZW_IsVirtualNode),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = false;

  mock_call_expect(TO_STR(ZCB_GetNodeType),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = false;

  replicationStatus = 0;
  g_learnNodeState = LEARN_NODE_STATE_NEW;
  g_learnMode = 0;
  rxFrame.header.sourceID = 5;
  SET_HEADERTYPE(rxFrame, HDRTYP_EXPLORE);
  cmdClass = 2;// dummy command class
  cmdClassCmd = 0x35; // dummy value
  dropFrame = IsFrameIlLegal(HDRFORMATTYP_3CH, &rxFrame, &cmdClass, &cmdClassCmd);
  TEST_ASSERT_FALSE(dropFrame);
  mock_calls_verify();

  /* Test that frames are not dropped if we are removing nodes  and nframe from unkwon node*/
  mock_calls_clear();
  memset((uint8_t*)&rxFrame, 0, sizeof(frame));

  mock_call_expect(TO_STR(ZW_IsVirtualNode),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = false;

  mock_call_expect(TO_STR(ZCB_GetNodeType),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = false;

  replicationStatus = 0;
  g_learnNodeState = LEARN_NODE_STATE_DELETE;
  g_learnMode = 0;
  rxFrame.header.sourceID = 5;
  SET_HEADERTYPE(rxFrame, HDRTYP_EXPLORE);
  cmdClass = 2;// dummy command class
  cmdClassCmd = 0x35; // dummy value
  dropFrame = IsFrameIlLegal(HDRFORMATTYP_3CH, &rxFrame, &cmdClass, &cmdClassCmd);
  TEST_ASSERT_FALSE(dropFrame);
  mock_calls_verify();

  /* Test that frames are not dropped if we are updating nodes  and nframe from unkwon node*/
  mock_calls_clear();
  memset((uint8_t*)&rxFrame, 0, sizeof(frame));

  mock_call_expect(TO_STR(ZW_IsVirtualNode),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = false;

  mock_call_expect(TO_STR(ZCB_GetNodeType),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = false;

  replicationStatus = 0;
  g_learnNodeState = LEARN_NODE_STATE_UPDATE;
  g_learnMode = 0;
  rxFrame.header.sourceID = 5;
  SET_HEADERTYPE(rxFrame, HDRTYP_EXPLORE);
  cmdClass = 2;// dummy command class
  cmdClassCmd = 0x35; // dummy value
  dropFrame = IsFrameIlLegal(HDRFORMATTYP_3CH, &rxFrame, &cmdClass, &cmdClassCmd);
  TEST_ASSERT_FALSE(dropFrame);
  mock_calls_verify();

  /* Test that frames are dropped if we are in smartstart  and node is SIS and frame is execlude requeest*/
  mock_calls_clear();
  memset((uint8_t*)&rxFrame, 0, sizeof(frame));

  staticControllerNodeID = g_nodeID;
  nodeIdserverPresent = true;
  
  mock_call_expect(TO_STR(ZW_IsVirtualNode),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = false;
  mock_call_expect(TO_STR(ZCB_GetNodeType),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = false;

  replicationStatus = 0;
  g_learnNodeState = LEARN_NODE_STATE_SMART_START;
  g_learnMode = 0;
  rxFrame.header.sourceID = 5;
  rxFrame.explore3ch.ver_Cmd = EXPLORE_CMD_AUTOINCLUSION;
  ((EXPLORE_REMOTE_INCLUSION_REQUEST_FRAME*)EXPLORE_PAYLOAD_3CH(rxFrame))->header.cmd = ZWAVE_CMD_EXCLUDE_REQUEST;
  SET_HEADERTYPE(rxFrame, HDRTYP_EXPLORE);

  cmdClass = 2;// dummy command class
  cmdClassCmd = 0x35; // dummy value
  dropFrame = IsFrameIlLegal(HDRFORMATTYP_3CH, &rxFrame, &cmdClass, &cmdClassCmd);
  TEST_ASSERT_TRUE(dropFrame);
  mock_calls_verify();
  /* Test that frames are not dropped if we are in smartstart  and node is SIS and frame is include requeest*/
  mock_calls_clear();
  memset((uint8_t*)&rxFrame, 0, sizeof(frame));

  staticControllerNodeID = g_nodeID;
  nodeIdserverPresent = true;
  
  mock_call_expect(TO_STR(ZW_IsVirtualNode),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = false;

  mock_call_expect(TO_STR(ZCB_GetNodeType),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = false;

  replicationStatus = 0;
  g_learnNodeState = LEARN_NODE_STATE_SMART_START;
  g_learnMode = 0;
  rxFrame.header.sourceID = 5;
  rxFrame.explore3ch.ver_Cmd = EXPLORE_CMD_AUTOINCLUSION;
  ((EXPLORE_REMOTE_INCLUSION_REQUEST_FRAME*)EXPLORE_PAYLOAD_3CH(rxFrame))->header.cmd = ZWAVE_CMD_INCLUDED_NODE_INFO;
  SET_HEADERTYPE(rxFrame, HDRTYP_EXPLORE);

  cmdClass = 2;// dummy command class
  cmdClassCmd = 0x35; // dummy value
  dropFrame = IsFrameIlLegal(HDRFORMATTYP_3CH, &rxFrame, &cmdClass, &cmdClassCmd);
  TEST_ASSERT_FALSE(dropFrame);
  mock_calls_verify();

  /* Test that frames are not dropped if we are in smartstart  and node is not SIS and frame is include requeest*/
  mock_calls_clear();
  memset((uint8_t*)&rxFrame, 0, sizeof(frame));

  staticControllerNodeID = g_nodeID;
  nodeIdserverPresent = false;
  
  mock_call_expect(TO_STR(ZW_IsVirtualNode),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = false;

  mock_call_expect(TO_STR(ZCB_GetNodeType),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = false;

  replicationStatus = 0;
  g_learnNodeState = LEARN_NODE_STATE_SMART_START;
  g_learnMode = 0;
  rxFrame.header.sourceID = 5;
  rxFrame.explore3ch.ver_Cmd = EXPLORE_CMD_AUTOINCLUSION;
  ((EXPLORE_REMOTE_INCLUSION_REQUEST_FRAME*)EXPLORE_PAYLOAD_3CH(rxFrame))->header.cmd = ZWAVE_CMD_INCLUDED_NODE_INFO;
  SET_HEADERTYPE(rxFrame, HDRTYP_EXPLORE);

  cmdClass = 2;// dummy command class
  cmdClassCmd = 0x35; // dummy value
  dropFrame = IsFrameIlLegal(HDRFORMATTYP_3CH, &rxFrame, &cmdClass, &cmdClassCmd);
  TEST_ASSERT_FALSE(dropFrame);
  mock_calls_verify();

  /* Test that frames are not dropped if we are in smartstart  and frame from known node*/
  mock_calls_clear();
  memset((uint8_t*)&rxFrame, 0, sizeof(frame));

  staticControllerNodeID = g_nodeID;
  nodeIdserverPresent = false;

  mock_call_expect(TO_STR(ZW_IsVirtualNode),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = false;

  mock_call_expect(TO_STR(ZCB_GetNodeType),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = true;

  replicationStatus = 0;
  g_learnNodeState = LEARN_NODE_STATE_SMART_START;
  g_learnMode = 0;
  rxFrame.header.sourceID = 5;
  rxFrame.explore3ch.ver_Cmd = EXPLORE_CMD_NORMAL;
  ((EXPLORE_REMOTE_INCLUSION_REQUEST_FRAME*)EXPLORE_PAYLOAD_3CH(rxFrame))->header.cmd = 10; //dummy
  SET_HEADERTYPE(rxFrame, HDRTYP_EXPLORE);

  cmdClass = 2;// dummy command class
  cmdClassCmd = 0x35; // dummy value
  dropFrame = IsFrameIlLegal(HDRFORMATTYP_3CH, &rxFrame, &cmdClass, &cmdClassCmd);
  TEST_ASSERT_FALSE(dropFrame);
  mock_calls_verify();

  /* Test that NOP POWER framer are not dropped */
  mock_calls_clear();
  memset((uint8_t*)&rxFrame, 0, sizeof(frame));

  mock_call_expect(TO_STR(ZW_IsVirtualNode),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = false;

  mock_call_expect(TO_STR(ZCB_GetNodeType),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = false;

  replicationStatus = 0;
  g_learnNodeState = LEARN_NODE_STATE_SMART_START;
  g_learnMode = 0;
  rxFrame.header.sourceID = 5;
  rxFrame.explore3ch.ver_Cmd = EXPLORE_CMD_NORMAL;
  ((EXPLORE_REMOTE_INCLUSION_REQUEST_FRAME*)EXPLORE_PAYLOAD_3CH(rxFrame))->header.cmd = 10; //dummy
  SET_HEADERTYPE(rxFrame, HDRTYP_EXPLORE);
  cmdClass = 1;
  cmdClassCmd = ZWAVE_CMD_NOP_POWER;
  dropFrame = IsFrameIlLegal(HDRFORMATTYP_3CH, &rxFrame, &cmdClass, &cmdClassCmd);
  TEST_ASSERT_FALSE(dropFrame);
  mock_calls_verify();

  /* Test that ZWAVE_CMD_TRANSFER_PRESENTATION framer are not dropped */
  mock_calls_clear();
  memset((uint8_t*)&rxFrame, 0, sizeof(frame));

  mock_call_expect(TO_STR(ZW_IsVirtualNode),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = false;

  mock_call_expect(TO_STR(ZCB_GetNodeType),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = false;

  replicationStatus = 0;
  g_learnNodeState = LEARN_NODE_STATE_SMART_START;
  g_learnMode = 0;
  rxFrame.header.sourceID = 5;
  rxFrame.explore3ch.ver_Cmd = EXPLORE_CMD_NORMAL;
  ((EXPLORE_REMOTE_INCLUSION_REQUEST_FRAME*)EXPLORE_PAYLOAD_3CH(rxFrame))->header.cmd = 10; //dummy
  SET_HEADERTYPE(rxFrame, HDRTYP_EXPLORE);
  cmdClass = 1;
  cmdClassCmd = ZWAVE_CMD_TRANSFER_PRESENTATION;
  dropFrame = IsFrameIlLegal(HDRFORMATTYP_3CH, &rxFrame, &cmdClass, &cmdClassCmd);
  TEST_ASSERT_FALSE(dropFrame);
  mock_calls_verify();

}

void test_IsFrameIlLegal_explore_LR(void)
{
  frame rxFrame;
  bool dropFrame;
  uint8_t cmdClass;
  uint8_t cmdClassCmd;
  mock_t * pMock;

  const node_id_t sourceNodeID = 0x0105;

  mock_calls_clear();

  memset((uint8_t *)&rxFrame, 0, sizeof(rxFrame));
  g_learnMode = 0;
  SET_SINGLECAST_SOURCE_NODEID_LR(rxFrame, sourceNodeID);
  SET_SINGLECAST_DESTINATION_NODEID_LR(rxFrame, 0x0FFF);

  rxFrame.headerLR.length = 20;
  SET_HEADERTYPE_LR(rxFrame, HDRTYP_SINGLECAST);


  /* Test that ZWAVE_LR_CMD_INCLUDED_NODE_INFO frames are not dropped */
  cmdClass = ZWAVE_CMD_CLASS_PROTOCOL_LR;
  cmdClassCmd = ZWAVE_LR_CMD_INCLUDED_NODE_INFO;

  mock_call_expect(TO_STR(ZW_IsVirtualNode), &pMock);
  pMock->expect_arg[ARG0].value = sourceNodeID;
  pMock->return_code.value = false;

  mock_call_expect(TO_STR(ZCB_GetNodeType), &pMock);
  pMock->expect_arg[ARG0].value = sourceNodeID;
  pMock->return_code.value = 0;

  dropFrame = IsFrameIlLegal(HDRFORMATTYP_LR, &rxFrame, &cmdClass, &cmdClassCmd);
  TEST_ASSERT_FALSE(dropFrame);


  /* Test that ZWAVE_LR_CMD_NODE_INFO frames are dropped */
  cmdClass = ZWAVE_CMD_CLASS_PROTOCOL_LR;
  cmdClassCmd = ZWAVE_LR_CMD_NODE_INFO;

  mock_call_expect(TO_STR(ZW_IsVirtualNode), &pMock);
  pMock->expect_arg[ARG0].value = sourceNodeID;
  pMock->return_code.value = false;

  mock_call_expect(TO_STR(ZCB_GetNodeType), &pMock);
  pMock->expect_arg[ARG0].value = sourceNodeID;
  pMock->return_code.value = 0;

  dropFrame = IsFrameIlLegal(HDRFORMATTYP_LR, &rxFrame, &cmdClass, &cmdClassCmd);
  TEST_ASSERT_TRUE(dropFrame);


  /* Test that frames without CMD CLASS are dropped */
  cmdClassCmd = ZWAVE_LR_CMD_INCLUDED_NODE_INFO;

  mock_call_expect(TO_STR(ZW_IsVirtualNode), &pMock);
  pMock->expect_arg[ARG0].value = sourceNodeID;
  pMock->return_code.value = false;

  mock_call_expect(TO_STR(ZCB_GetNodeType), &pMock);
  pMock->expect_arg[ARG0].value = sourceNodeID;
  pMock->return_code.value = 0;

  dropFrame = IsFrameIlLegal(HDRFORMATTYP_LR, &rxFrame, NULL, &cmdClassCmd);
  TEST_ASSERT_TRUE(dropFrame);

  mock_calls_verify();
}

/*
 * Test that IsFindNodeInRangeAllowed
 */
void test_IsFindNodeInRangeAllowed(void)
{
  bool frameAllowed;
  mock_t * pMock;
  g_nodeID = 0x01;

  /* Test that findNodeInRange is not allowed from when node is SIS (source node is known)*/
  mock_calls_clear();
  g_learnMode = 0;
  replicationStatus = 0;
  staticControllerNodeID = g_nodeID;
  nodeIdserverPresent = true;
  frameAllowed = IsFindNodeInRangeAllowed(5);
  TEST_ASSERT_FALSE(frameAllowed);
  mock_calls_verify();

  /* Test that findNodeInRange is not allowed from when node is SIS (source node is unknown)*/
  mock_calls_clear();
  g_learnMode = 0;
  replicationStatus = 0;
  staticControllerNodeID = g_nodeID;
  nodeIdserverPresent = true;
  frameAllowed = IsFindNodeInRangeAllowed(5);
  TEST_ASSERT_FALSE(frameAllowed);
  mock_calls_verify();

  /* Test that findNodeInRange is not allowed from uknown node*/
  mock_calls_clear();  
  g_learnMode = 0;
  replicationStatus = 0;
  staticControllerNodeID = 0;
  nodeIdserverPresent = false;
  
  mock_call_expect(TO_STR(ZCB_GetNodeType),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = false;
  frameAllowed = IsFindNodeInRangeAllowed(5);
  TEST_ASSERT_FALSE(frameAllowed);
  mock_calls_verify();

  /* Test that findNodeInRange is not allowed from uknown node when node is inclusion controller*/
  mock_calls_clear();  
  g_learnMode = 0;
  replicationStatus = 0;
  staticControllerNodeID = 2;
  nodeIdserverPresent = true;
  
  mock_call_expect(TO_STR(ZCB_GetNodeType),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = false;
  frameAllowed = IsFindNodeInRangeAllowed(5);
  TEST_ASSERT_FALSE(frameAllowed);
  mock_calls_verify();

  /* Test that findNodeInRange is allowed from known node when node is inclusion controller*/
  mock_calls_clear();  
  g_learnMode = 0;
  replicationStatus = 0;
  staticControllerNodeID = 2;
  nodeIdserverPresent = true;
  
  mock_call_expect(TO_STR(ZCB_GetNodeType),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = true;
  mock_call_expect(TO_STR(ZCB_GetNodeType),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = true;

  frameAllowed = IsFindNodeInRangeAllowed(5);
  TEST_ASSERT_TRUE(frameAllowed);
  mock_calls_verify();
  /* Test that findNodeInRange is allowed from unknown node when we are in learnMode*/
  mock_calls_clear();  
  g_learnMode = 1;
  replicationStatus = 0;
  staticControllerNodeID = 2;
  nodeIdserverPresent = false;
  frameAllowed = IsFindNodeInRangeAllowed(5);
  TEST_ASSERT_TRUE(frameAllowed);
  mock_calls_verify();
  /* Test that findNodeInRange is allowed from unknown node when we are in replicating receive mode*/
  mock_calls_clear();  
  g_learnMode = 0;
  replicationStatus = RECEIVER_CTRL_BIT;
  staticControllerNodeID = 2;
  nodeIdserverPresent = false;
  
  frameAllowed = IsFindNodeInRangeAllowed(5);
  TEST_ASSERT_TRUE(frameAllowed);
  mock_calls_verify();
}

/*
 * Test DropPingTest
 */
void test_DropPingTest(void)
{
  bool dropPing;
  mock_t * pMock;

  /* Test that we dont ping known non listening nodes*/
  mock_calls_clear();
  mock_call_expect(TO_STR(ZCB_GetNodeType),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = true;
  mock_call_expect(TO_STR(GetNodeCapabilities),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = 0;

  dropPing = DropPingTest(5);
  TEST_ASSERT_TRUE(dropPing);
  mock_calls_verify();


  /* Test that we ping known listening nodes*/
  mock_calls_clear();
  mock_call_expect(TO_STR(ZCB_GetNodeType),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = true;
  mock_call_expect(TO_STR(GetNodeCapabilities),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = NODEINFO_LISTENING_SUPPORT;

  dropPing = DropPingTest(5);
  TEST_ASSERT_FALSE(dropPing);
  mock_calls_verify();

  /* Test that we ping unknown  nodes*/
  mock_calls_clear();  
  mock_call_expect(TO_STR(ZCB_GetNodeType),&pMock);
  pMock->expect_arg[ARG0].value = 0x5;
  pMock->return_code.value = false;

  dropPing = DropPingTest(5);
  TEST_ASSERT_FALSE(dropPing);
  mock_calls_verify();

}
