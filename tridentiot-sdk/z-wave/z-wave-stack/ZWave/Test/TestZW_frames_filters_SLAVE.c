// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file TestZW_frames_filters_SLAVE.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include "ZW_protocol.h"
#include "ZW_slave.h"
#include "ZW_explore.h"
#include "ZW_Frame.h"
#include "ZW.h"
#include "unity.h"
#include "mock_control.h"
#include "ZW_frames_filters.h"
#include "string.h"

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

/*
 * Test that IsFrameIlLegal filter the received singlecast frames correctly
 */
void test_IsFrameIlLegal_singlecast_2ch(void)
{
  frame rxFrame;
  bool dropFrame;
  uint8_t cmdClass;
  uint8_t cmdClassCmd;
  /* Test that frame is dropped if  sourceID == g_nodeID and the frame is COMAND_CLASS_SECUROTY*/
  mock_calls_clear();
  memset((uint8_t*)&rxFrame, 0, sizeof(frame));
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
  g_learnMode = 0;
  rxFrame.header.sourceID = 5;
  rxFrame.singlecast.destinationID = 0xFF;
  SET_HEADERTYPE(rxFrame, HDRTYP_SINGLECAST);
  cmdClass = COMMAND_CLASS_SECURITY;
  cmdClassCmd = 0x35; // dummy value
  dropFrame = IsFrameIlLegal(HDRFORMATTYP_2CH, &rxFrame, &cmdClass, &cmdClassCmd);
  TEST_ASSERT_TRUE(dropFrame);
  mock_calls_verify();

  /* Test that frame is dropped if  sourceID == node */
  mock_calls_clear();
  memset((uint8_t*)&rxFrame, 0, sizeof(frame));
  g_learnMode = 0;
  rxFrame.header.sourceID = 0x01;
  rxFrame.singlecast.destinationID = 0x06;
  SET_HEADERTYPE(rxFrame, HDRTYP_SINGLECAST);
  cmdClass = 2;// dummy command class
  cmdClassCmd = 0x35; // dummy value
  dropFrame = IsFrameIlLegal(HDRFORMATTYP_2CH, &rxFrame, &cmdClass, &cmdClassCmd);
  TEST_ASSERT_TRUE(dropFrame);
  mock_calls_verify();

  /* Test that frame is not dropped if  sourceID == node and we re in learn mode*/
  mock_calls_clear();
  memset((uint8_t*)&rxFrame, 0, sizeof(frame));
  g_learnMode = 1;
  rxFrame.header.sourceID = 0x01;
  rxFrame.singlecast.destinationID = 0x06;
  SET_HEADERTYPE(rxFrame, HDRTYP_SINGLECAST);
  cmdClass = 2;// dummy command class
  cmdClassCmd = 0x35; // dummy value
  dropFrame = IsFrameIlLegal(HDRFORMATTYP_2CH, &rxFrame, &cmdClass, &cmdClassCmd);
  TEST_ASSERT_FALSE(dropFrame);
  mock_calls_verify();

}



/*
 * Test that IsFrameIlLegal filter the received singlecast frames correctly
 */
void test_IsFrameIlLegal_singlecast_3ch(void)
{
  frame rxFrame;
  bool dropFrame;
  uint8_t cmdClass;
  uint8_t cmdClassCmd;
  
  /* Test that frame is dropped if  sourceID == g_nodeID and the frame is COMAND_CLASS_SECUROTY*/
  mock_calls_clear();
  memset((uint8_t*)&rxFrame, 0, sizeof(frame));
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
  g_learnMode = 0;
  rxFrame.header.sourceID = 5;
  rxFrame.singlecast3ch.destinationID = 0xFF;
  SET_HEADERTYPE(rxFrame, HDRTYP_SINGLECAST);
  cmdClass = COMMAND_CLASS_SECURITY;
  cmdClassCmd = 0x35; // dummy value
  dropFrame = IsFrameIlLegal(HDRFORMATTYP_3CH, &rxFrame, &cmdClass, &cmdClassCmd);
  TEST_ASSERT_TRUE(dropFrame);
  mock_calls_verify();

  /* Test that frame is dropped if  sourceID == node */
  mock_calls_clear();
  memset((uint8_t*)&rxFrame, 0, sizeof(frame));
  g_learnMode = 0;
  rxFrame.header.sourceID = 0x01;
  rxFrame.singlecast3ch.destinationID = 0x06;
  SET_HEADERTYPE(rxFrame, HDRTYP_SINGLECAST);
  cmdClass = 2;// dummy command class
  cmdClassCmd = 0x35; // dummy value
  dropFrame = IsFrameIlLegal(HDRFORMATTYP_3CH, &rxFrame, &cmdClass, &cmdClassCmd);
  TEST_ASSERT_TRUE(dropFrame);
  mock_calls_verify();
  /* Test that frame is not dropped if  sourceID == node and we re in learn mode*/
  mock_calls_clear();
  memset((uint8_t*)&rxFrame, 0, sizeof(frame));
  g_learnMode = 1;
  rxFrame.header.sourceID = 0x01;
  rxFrame.singlecast.destinationID = 0x06;
  SET_HEADERTYPE(rxFrame, HDRTYP_SINGLECAST);
  cmdClass = 2;// dummy command class
  cmdClassCmd = 0x35; // dummy value
  dropFrame = IsFrameIlLegal(HDRFORMATTYP_2CH, &rxFrame, &cmdClass, &cmdClassCmd);
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

  /* Test that frame is dropped if  sourceID == g_nodeID and the frame is COMAND_CLASS_SECUROTY*/
  mock_calls_clear();

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

  /* Test that frame is dropped if  sourceID == node and destination = g_nodeID*/
  mock_calls_clear();

  g_learnMode = 0;
  rxFrame.header.sourceID = 0x01;
  rxFrame.singlecastRouted.destinationID = 0x01;
  SET_HEADERTYPE(rxFrame, HDRTYP_SINGLECAST);
  SET_ROUTED(rxFrame);
  cmdClass = 2;// dummy command class
  cmdClassCmd = 0x35; // dummy value
  dropFrame = IsFrameIlLegal(HDRFORMATTYP_2CH, &rxFrame, &cmdClass, &cmdClassCmd);
  TEST_ASSERT_TRUE(dropFrame);
  mock_calls_verify();

  /* Test that frame is not dropped if  sourceID == node and destination != g_nodeID*/
  mock_calls_clear();

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

  g_learnMode = 1;
  rxFrame.header.sourceID = 0x01;
  rxFrame.singlecastRouted.destinationID = 0x01;
  SET_HEADERTYPE(rxFrame, HDRTYP_SINGLECAST);
  SET_ROUTED(rxFrame);
  cmdClass = 2;// dummy command class
  cmdClassCmd = 0x35; // dummy value
  dropFrame = IsFrameIlLegal(HDRFORMATTYP_2CH, &rxFrame, &cmdClass, &cmdClassCmd);
  TEST_ASSERT_FALSE(dropFrame);
  mock_calls_verify();

/* Test that frames are not dropped if we re in learnMode and sourceid = destinationid = g_nodeID*/
  mock_calls_clear();

  g_learnMode = 1;
  rxFrame.header.sourceID = 0x01;
  rxFrame.singlecastRouted.destinationID = 0x01;
  SET_HEADERTYPE(rxFrame, HDRTYP_SINGLECAST);
  SET_ROUTED(rxFrame);
  cmdClass = 2;// dummy command class
  cmdClassCmd = 0x35; // dummy value
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

  /* Test that frame is dropped if  sourceID == g_nodeID and the frame is COMAND_CLASS_SECUROTY*/
  mock_calls_clear();

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
  g_learnMode = 0;
  rxFrame.header.sourceID = 5;
  rxFrame.singlecastRouted3ch.destinationID = 0xFF;
  SET_HEADERTYPE(rxFrame, HDRTYP_ROUTED);
  cmdClass = COMMAND_CLASS_SECURITY;
  cmdClassCmd = 0x35; // dummy value
  dropFrame = IsFrameIlLegal(HDRFORMATTYP_3CH, &rxFrame, &cmdClass, &cmdClassCmd);
  TEST_ASSERT_TRUE(dropFrame);
  mock_calls_verify();

  /* Test that frame is dropped if  sourceID == node and destination = g_nodeID*/
  mock_calls_clear();

  g_learnMode = 0;
  rxFrame.header.sourceID = 0x01;
  rxFrame.singlecastRouted3ch.destinationID = 0x01;
  SET_HEADERTYPE(rxFrame, HDRTYP_ROUTED);
  cmdClass = 2;// dummy command class
  cmdClassCmd = 0x35; // dummy value
  dropFrame = IsFrameIlLegal(HDRFORMATTYP_3CH, &rxFrame, &cmdClass, &cmdClassCmd);
  TEST_ASSERT_TRUE(dropFrame);
  mock_calls_verify();

  /* Test that frame is not dropped if  sourceID == node and destination != g_nodeID*/
  mock_calls_clear();

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

  g_learnMode = 1;
  rxFrame.header.sourceID = 0x01;
  rxFrame.singlecastRouted3ch.destinationID = 0x01;
  SET_HEADERTYPE(rxFrame, HDRTYP_ROUTED);
  cmdClass = 2;// dummy command class
  cmdClassCmd = 0x35; // dummy value
  dropFrame = IsFrameIlLegal(HDRFORMATTYP_3CH, &rxFrame, &cmdClass, &cmdClassCmd);
  TEST_ASSERT_FALSE(dropFrame);
  mock_calls_verify();

/* Test that frames are not dropped if we re in learnMode and sourceid = destinationid = g_nodeID*/
  mock_calls_clear();

  g_learnMode = 1;
  rxFrame.header.sourceID = 0x01;
  rxFrame.singlecastRouted3ch.destinationID = 0x01;
  SET_HEADERTYPE(rxFrame, HDRTYP_ROUTED);
  cmdClass = 2;// dummy command class
  cmdClassCmd = 0x35; // dummy value
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

  /* Test that frame is dropped if  sourceID == node */
  mock_calls_clear();
  g_learnMode = 0;
  rxFrame.header.sourceID = 1;
  SET_HEADERTYPE(rxFrame, HDRTYP_EXPLORE);
  cmdClass = 2;// dummy command class
  cmdClassCmd = 0x35; // dummy value
  dropFrame = IsFrameIlLegal(HDRFORMATTYP_2CH, &rxFrame, &cmdClass, &cmdClassCmd);
  TEST_ASSERT_TRUE(dropFrame);
  mock_calls_verify();

  /* Test that frames are not dropped if sourceID != g_nodeID and we are not in learnMode */
  mock_calls_clear();

  g_learnMode = 0;
  rxFrame.header.sourceID = 5;
  SET_HEADERTYPE(rxFrame, HDRTYP_EXPLORE);
  cmdClass = 2;// dummy command class
  cmdClassCmd = 0x35; // dummy value
  dropFrame = IsFrameIlLegal(HDRFORMATTYP_2CH, &rxFrame, &cmdClass, &cmdClassCmd);
  TEST_ASSERT_FALSE(dropFrame);
  mock_calls_verify();

  /* Test that frames are not dropped if sourceID = g_nodeID and we are in learnMode */
  mock_calls_clear();

  g_learnMode = 1;
  rxFrame.header.sourceID = 1;
  SET_HEADERTYPE(rxFrame, HDRTYP_EXPLORE);
  cmdClass = 2;// dummy command class
  cmdClassCmd = 0x35; // dummy value
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

  /* Test that frame is dropped if  sourceID == node*/
  mock_calls_clear();
  g_learnMode = 0;
  rxFrame.header.sourceID = 1;
  SET_HEADERTYPE(rxFrame, HDRTYP_EXPLORE);
  cmdClass = 2;// dummy command class
  cmdClassCmd = 0x35; // dummy value
  dropFrame = IsFrameIlLegal(HDRFORMATTYP_3CH, &rxFrame, &cmdClass, &cmdClassCmd);
  TEST_ASSERT_TRUE(dropFrame);
  mock_calls_verify();

  /* Test that frame is not  dropped if  sourceID != node and we are not in learn mode*/
  mock_calls_clear();

  g_learnMode = 0;
  rxFrame.header.sourceID = 5;
  SET_HEADERTYPE(rxFrame, HDRTYP_EXPLORE);
  cmdClass = 2;// dummy command class
  cmdClassCmd = 0x35; // dummy value
  dropFrame = IsFrameIlLegal(HDRFORMATTYP_3CH, &rxFrame, &cmdClass, &cmdClassCmd);
  TEST_ASSERT_FALSE(dropFrame);
  mock_calls_verify();

  /* Test that frames are not dropped if we re in learnMode */
  mock_calls_clear();

  g_learnMode = 1;
  rxFrame.header.sourceID = 5;
  SET_HEADERTYPE(rxFrame, HDRTYP_EXPLORE);
  cmdClass = 2;// dummy command class
  cmdClassCmd = 0x35; // dummy value
  dropFrame = IsFrameIlLegal(HDRFORMATTYP_3CH, &rxFrame, &cmdClass, &cmdClassCmd);
  TEST_ASSERT_FALSE(dropFrame);
  mock_calls_verify();

}
