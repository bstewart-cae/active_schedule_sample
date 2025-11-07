// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file TestReceiveFramesLinkLayer_3ch.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include <unity.h>
#include <mock_control.h>
#include <stdio.h>
#include <string.h>

#include "ZW_DataLinkLayer.h"
#include <zpal_radio.h>

void setUpSuite(void) {

}

void tearDownSuite(void) {

}

#define BACKWARD_COMPABILITY_TEST

/** Missing test overview for single cast:
 * - Speed modified, test that if a frame is sent at a lower speed than supported by the nodes, then speed modified value is set.
 * - 3 Channel tests
 * -
 * -
 */
static bool mTestFilterCalled = false;

static uint8_t                 receive_buffer[sizeof(zpal_radio_receive_frame_t) + 255]; // Let's allocate 255 frame content payload for received frames for testing.
static zpal_radio_receive_frame_t *  mp_zpal_received_frame = (zpal_radio_receive_frame_t*) receive_buffer;
static ZW_ReceiveFrame_t mTestFilterReceivedFrame;

void setUp(void)
{
  //This is run before EACH TEST
  mock_calls_clear();
  mTestFilterCalled = false;
}

void tearDown(void)
{
  mock_calls_verify();
}

void helper_func_expect_radio_init_jp(zpal_radio_profile_t *pRfProfile, mock_t *p_mock)
{
  // Setup of test expectations. When configuration the Data Link Layer, the following calls are expected further down to Phy Layer.
  // 1) ZW_RadioPhyIni, with configuration of 3 channels for 9.6k, 40k, 100k. And a Energy Harvesting channel (still to be defined)
  mock_call_expect(TO_STR(zpal_radio_init), &p_mock);
  p_mock->expect_arg[0].pointer = pRfProfile;
  p_mock->compare_rule_arg[1]   = COMPARE_NULL;
  p_mock->compare_rule_arg[2]   = COMPARE_NULL;
  p_mock->compare_rule_arg[3]   = COMPARE_NULL;
}

void receivedFramesFilterCallback(ZW_ReceiveFrame_t * pFrame)
{
  mTestFilterCalled = true;
  mTestFilterReceivedFrame = *pFrame;
}



void test_backward_compability_mode(void)
{
#ifdef BACKWARD_COMPABILITY_TEST
  TEST_IGNORE_MESSAGE("Currently full frame is pushed to upper layer for backward compability - Adjust test cases when upper layer is updated");
#else  // BACKWARD_COMPABILITY_TEST
  TEST_FAIL_MESSAGE("Backward compability mode disabled - Ensure layers above Link Layer are correctly updated");
#endif // BACKWARD_COMPABILITY_TEST

}


/** Verification that the Data Link Layer can filter frames based upon payload filter, with home id = FF FF FF FF.
 *
 */
void test_receive_singlecast_frame_with_no_ack(void)
{
  mock_t* p_mock = NULL;
  zpal_radio_rx_parameters_t rxParameters = {
    .speed = ZPAL_RADIO_SPEED_100K,
    .channel_id = 1,
    .channel_header_format = ZPAL_RADIO_HEADER_TYPE_3CH,
    .rssi = 0
  };

  uint8_t          nifFrame[] = {0xC9, 0x2F, 0x7F, 0xAD, 0x00, 0x01, 0x00, 0x21, 0x0C, 0xFF,                                                                   // HomeId, Src, Flags1, Flags2, Length, Dst
                                 0x01, 0x01, 0xD3, 0x9C, 0x01, 0x10, 0x01, 0x5E, 0x25, 0x85, 0x8E, 0x59, 0x55, 0x86, 0x72, 0x5A, 0x73, 0x98, 0x9F, 0x6C, 0x7A, // CC vers, NIF, Capabilities, Security, Properties, CC supported,
                                 0xBD, 0xFD}; // Crc.

  ZW_ReturnCode_t actualVal;
  zpal_radio_profile_t radioProfile = { REGION_JP, ZPAL_RADIO_WAKEUP_ALWAYS_LISTEN };
  /** As correct radio initialization is ensured in other tests, we just utilize the helper function in this test. */
  helper_func_expect_radio_init_jp(&radioProfile, p_mock);
  actualVal = llInit(&radioProfile);
  TEST_ASSERT_EQUAL(SUCCESS, actualVal);

  ZW_ReceiveFilter_t rxFilter = {.payloadIndex1 = 0, .payloadFilterValue1 = 0x00, // CC Vers. 1
                                 .payloadIndex2 = 0, .payloadFilterValue2 = 0x00, // NIF
                                 .headerType = 0x01,                              // Single Cast
                                 .flag = 0,
                                 .frameHandler = receivedFramesFilterCallback};

  TEST_ASSERT_FALSE(mTestFilterCalled);
  actualVal = llReceiveFilterAdd(&rxFilter);
  TEST_ASSERT_EQUAL(SUCCESS, actualVal);
  mp_zpal_received_frame->frame_content_length = sizeof(nifFrame);
  memcpy(mp_zpal_received_frame->frame_content, nifFrame, mp_zpal_received_frame->frame_content_length);

    radioProfile.receive_handler_cb(&rxParameters, mp_zpal_received_frame);

  // test tjhe recived frame information
  TEST_ASSERT_EQUAL(33, mTestFilterReceivedFrame.frameContentLength);
  TEST_ASSERT_EQUAL(0, memcmp(&mTestFilterReceivedFrame.frameContent[10], &nifFrame[10], mTestFilterReceivedFrame.frameContentLength - 10));

  TEST_ASSERT_EQUAL(0x01,mTestFilterReceivedFrame.frameOptions.frameType);

  TEST_ASSERT_EQUAL(nifFrame[0],mTestFilterReceivedFrame.frameOptions.homeId[0]);
  TEST_ASSERT_EQUAL(nifFrame[1],mTestFilterReceivedFrame.frameOptions.homeId[1]);
  TEST_ASSERT_EQUAL(nifFrame[2],mTestFilterReceivedFrame.frameOptions.homeId[2]);
  TEST_ASSERT_EQUAL(nifFrame[3],mTestFilterReceivedFrame.frameOptions.homeId[3]);

  TEST_ASSERT_EQUAL(nifFrame[4],mTestFilterReceivedFrame.frameOptions.sourceNodeId);
  TEST_ASSERT_EQUAL(nifFrame[8],mTestFilterReceivedFrame.frameOptions.sequenceNumber);
  TEST_ASSERT_EQUAL(nifFrame[9],mTestFilterReceivedFrame.frameOptions.destinationNodeId);


  TEST_ASSERT_FALSE(mTestFilterReceivedFrame.frameOptions.acknowledge);
  TEST_ASSERT_FALSE(mTestFilterReceivedFrame.frameOptions.speedModified);
  TEST_ASSERT_FALSE(mTestFilterReceivedFrame.frameOptions.lowPower);
  TEST_ASSERT_FALSE(mTestFilterReceivedFrame.frameOptions.routed);
  TEST_ASSERT_FALSE(mTestFilterReceivedFrame.frameOptions.wakeup250ms);
  TEST_ASSERT_FALSE(mTestFilterReceivedFrame.frameOptions.wakeup1000ms);
  TEST_ASSERT_FALSE(mTestFilterReceivedFrame.frameOptions.extended);
}


void test_receive_singlecast_frame_with_ack(void)
{
  mock_t* p_mock = NULL;
  zpal_radio_rx_parameters_t rxParameters = {
    .speed = ZPAL_RADIO_SPEED_100K,
    .channel_id = 1,
    .channel_header_format = ZPAL_RADIO_HEADER_TYPE_3CH,
    .rssi = 0
  };

  uint8_t          nifFrame[] = {0xC9, 0x2F, 0x7F, 0xAD, 0x00, 0x81, 0x00, 0x21, 0x0C, 0xFF,    // HomeId, Src, Flags1, Flags2, Length, Dst
                                 0x01, 0x01, 0xD3, 0x9C, 0x01, 0x10, 0x01, 0x5E, 0x25, 0x85,
                                 0x8E, 0x59, 0x55, 0x86, 0x72, 0x5A, 0x73, 0x98, 0x9F, 0x6C, 0x7A, // CC vers, NIF, Capabilities, Security, Properties, CC supported,
                                 0xB7, 0x4E}; // Crc.

  ZW_ReturnCode_t actualVal;
  zpal_radio_profile_t radioProfile = { REGION_JP, ZPAL_RADIO_WAKEUP_ALWAYS_LISTEN };
  /** As correct radio initialization is ensured in other tests, we just utilize the helper function in this test. */
  helper_func_expect_radio_init_jp(&radioProfile, p_mock);
  actualVal = llInit(&radioProfile);
  TEST_ASSERT_EQUAL(SUCCESS, actualVal);

  ZW_ReceiveFilter_t rxFilter = {.payloadIndex1 = 0, .payloadFilterValue1 = 0x00, // CC Vers. 1
                                 .payloadIndex2 = 0, .payloadFilterValue2 = 0x00, // NIF
                                 .headerType = 0x01,                              // Single Cast
                                 .flag = 0,
                                 .frameHandler = receivedFramesFilterCallback};

  TEST_ASSERT_FALSE(mTestFilterCalled);
  actualVal = llReceiveFilterAdd(&rxFilter);
  TEST_ASSERT_EQUAL(SUCCESS, actualVal);
  mp_zpal_received_frame->frame_content_length = sizeof(nifFrame);
  memcpy(mp_zpal_received_frame->frame_content, nifFrame, mp_zpal_received_frame->frame_content_length);


    radioProfile.receive_handler_cb(&rxParameters, mp_zpal_received_frame);


  // test tjhe recived frame information
  TEST_ASSERT_EQUAL(33, mTestFilterReceivedFrame.frameContentLength);
  TEST_ASSERT_EQUAL(0, memcmp(&mTestFilterReceivedFrame.frameContent[10], &nifFrame[10], mTestFilterReceivedFrame.frameContentLength - 10));

  TEST_ASSERT_EQUAL(0x01,mTestFilterReceivedFrame.frameOptions.frameType);

  TEST_ASSERT_EQUAL(nifFrame[0],mTestFilterReceivedFrame.frameOptions.homeId[0]);
  TEST_ASSERT_EQUAL(nifFrame[1],mTestFilterReceivedFrame.frameOptions.homeId[1]);
  TEST_ASSERT_EQUAL(nifFrame[2],mTestFilterReceivedFrame.frameOptions.homeId[2]);
  TEST_ASSERT_EQUAL(nifFrame[3],mTestFilterReceivedFrame.frameOptions.homeId[3]);

  TEST_ASSERT_EQUAL(nifFrame[4],mTestFilterReceivedFrame.frameOptions.sourceNodeId);
  TEST_ASSERT_EQUAL(nifFrame[8],mTestFilterReceivedFrame.frameOptions.sequenceNumber);
  TEST_ASSERT_EQUAL(nifFrame[9],mTestFilterReceivedFrame.frameOptions.destinationNodeId);


  TEST_ASSERT_TRUE(mTestFilterReceivedFrame.frameOptions.acknowledge);
  TEST_ASSERT_FALSE(mTestFilterReceivedFrame.frameOptions.speedModified);
  TEST_ASSERT_FALSE(mTestFilterReceivedFrame.frameOptions.lowPower);
  TEST_ASSERT_FALSE(mTestFilterReceivedFrame.frameOptions.routed);
  TEST_ASSERT_FALSE(mTestFilterReceivedFrame.frameOptions.wakeup250ms);
  TEST_ASSERT_FALSE(mTestFilterReceivedFrame.frameOptions.wakeup1000ms);
  TEST_ASSERT_FALSE(mTestFilterReceivedFrame.frameOptions.extended);

  actualVal = llReceiveFilterRemove(&rxFilter);
  TEST_ASSERT_EQUAL(SUCCESS, actualVal);

}


void test_receive_singlecast_frame_with_250ms_sourcewakeup(void)
{
  mock_t* p_mock = NULL;
  zpal_radio_rx_parameters_t rxParameters = {
    .speed = ZPAL_RADIO_SPEED_100K,
    .channel_id = 1,
    .channel_header_format = ZPAL_RADIO_HEADER_TYPE_3CH,
    .rssi = 0
  };

  uint8_t          nifFrame[] = {0xC9, 0x2F, 0x7F, 0xAD, 0x00, 0x81, 0x10, 0x21, 0x0C, 0xFF,                                                                   // HomeId, Src, Flags1, Flags2, Length, Dst
                                 0x01, 0x01, 0xD3, 0x9C, 0x01, 0x10, 0x01, 0x5E, 0x25, 0x85,
                                 0x8E, 0x59, 0x55, 0x86, 0x72, 0x5A, 0x73, 0x98, 0x9F, 0x6C, 0x7A, // CC vers, NIF, Capabilities, Security, Properties, CC supported,
                                 0xAC, 0x8A}; // Crc.

  ZW_ReturnCode_t actualVal;
  zpal_radio_profile_t radioProfile = { REGION_JP, ZPAL_RADIO_WAKEUP_ALWAYS_LISTEN };
  /** As correct radio initialization is ensured in other tests, we just utilize the helper function in this test. */
  helper_func_expect_radio_init_jp(&radioProfile, p_mock);
  actualVal = llInit(&radioProfile);
  TEST_ASSERT_EQUAL(SUCCESS, actualVal);

  ZW_ReceiveFilter_t rxFilter = {.payloadIndex1 = 0, .payloadFilterValue1 = 0x00, // CC Vers. 1
                                 .payloadIndex2 = 0, .payloadFilterValue2 = 0x00, // NIF
                                 .headerType = 0x01,                              // Single Cast
                                 .flag = 0,
                                 .frameHandler = receivedFramesFilterCallback};

  TEST_ASSERT_FALSE(mTestFilterCalled);
  actualVal = llReceiveFilterAdd(&rxFilter);
  TEST_ASSERT_EQUAL(SUCCESS, actualVal);
  mp_zpal_received_frame->frame_content_length = sizeof(nifFrame);
  memcpy(mp_zpal_received_frame->frame_content, nifFrame, mp_zpal_received_frame->frame_content_length);

    radioProfile.receive_handler_cb(&rxParameters, mp_zpal_received_frame);

  // test tjhe recived frame information
  TEST_ASSERT_EQUAL(33, mTestFilterReceivedFrame.frameContentLength);
  TEST_ASSERT_EQUAL(0, memcmp(&mTestFilterReceivedFrame.frameContent[10], &nifFrame[10], mTestFilterReceivedFrame.frameContentLength - 10));

  TEST_ASSERT_EQUAL(0x01,mTestFilterReceivedFrame.frameOptions.frameType);

  TEST_ASSERT_EQUAL(nifFrame[0],mTestFilterReceivedFrame.frameOptions.homeId[0]);
  TEST_ASSERT_EQUAL(nifFrame[1],mTestFilterReceivedFrame.frameOptions.homeId[1]);
  TEST_ASSERT_EQUAL(nifFrame[2],mTestFilterReceivedFrame.frameOptions.homeId[2]);
  TEST_ASSERT_EQUAL(nifFrame[3],mTestFilterReceivedFrame.frameOptions.homeId[3]);

  TEST_ASSERT_EQUAL(nifFrame[4],mTestFilterReceivedFrame.frameOptions.sourceNodeId);
  TEST_ASSERT_EQUAL(nifFrame[8],mTestFilterReceivedFrame.frameOptions.sequenceNumber);
  TEST_ASSERT_EQUAL(nifFrame[9],mTestFilterReceivedFrame.frameOptions.destinationNodeId);


  TEST_ASSERT_TRUE(mTestFilterReceivedFrame.frameOptions.acknowledge);
  TEST_ASSERT_FALSE(mTestFilterReceivedFrame.frameOptions.speedModified);
  TEST_ASSERT_FALSE(mTestFilterReceivedFrame.frameOptions.lowPower);
  TEST_ASSERT_FALSE(mTestFilterReceivedFrame.frameOptions.routed);
  TEST_ASSERT_TRUE(mTestFilterReceivedFrame.frameOptions.wakeup250ms);
  TEST_ASSERT_FALSE(mTestFilterReceivedFrame.frameOptions.wakeup1000ms);
  TEST_ASSERT_FALSE(mTestFilterReceivedFrame.frameOptions.extended);

  actualVal = llReceiveFilterRemove(&rxFilter);
  TEST_ASSERT_EQUAL(SUCCESS, actualVal);

}


void test_receive_singlecast_frame_with_1000ms_sourcewakeup(void)
{
  mock_t* p_mock = NULL;
  zpal_radio_rx_parameters_t rxParameters = {
    .speed = ZPAL_RADIO_SPEED_100K,
    .channel_id = 1,
    .channel_header_format = ZPAL_RADIO_HEADER_TYPE_3CH,
    .rssi = 0
  };

  uint8_t          nifFrame[] = {0xC9, 0x2F, 0x7F, 0xAD, 0x00, 0x81, 0x20, 0x21, 0x0C, 0xFF,                                                                   // HomeId, Src, Flags1, Flags2, Length, Dst
                                 0x01, 0x01, 0xD3, 0x9C, 0x01, 0x10, 0x01, 0x5E, 0x25, 0x85,
                                 0x8E, 0x59, 0x55, 0x86, 0x72, 0x5A, 0x73, 0x98, 0x9F, 0x6C, 0x7A, // CC vers, NIF, Capabilities, Security, Properties, CC supported,
                                 0x80, 0xC6}; // Crc.

  ZW_ReturnCode_t actualVal;
  zpal_radio_profile_t radioProfile = { REGION_JP, ZPAL_RADIO_WAKEUP_ALWAYS_LISTEN };
  /** As correct radio initialization is ensured in other tests, we just utilize the helper function in this test. */
  helper_func_expect_radio_init_jp(&radioProfile, p_mock);
  actualVal = llInit(&radioProfile);
  TEST_ASSERT_EQUAL(SUCCESS, actualVal);

  ZW_ReceiveFilter_t rxFilter = {.payloadIndex1 = 0, .payloadFilterValue1 = 0x00, // CC Vers. 1
                                 .payloadIndex2 = 0, .payloadFilterValue2 = 0x00, // NIF
                                 .headerType = 0x01,                              // Single Cast
                                 .flag = 0,
                                 .frameHandler = receivedFramesFilterCallback};

  TEST_ASSERT_FALSE(mTestFilterCalled);
  actualVal = llReceiveFilterAdd(&rxFilter);
  TEST_ASSERT_EQUAL(SUCCESS, actualVal);
  mp_zpal_received_frame->frame_content_length = sizeof(nifFrame);
  memcpy(mp_zpal_received_frame->frame_content, nifFrame, mp_zpal_received_frame->frame_content_length);

    radioProfile.receive_handler_cb(&rxParameters, mp_zpal_received_frame);


  // test tjhe recived frame information
  TEST_ASSERT_EQUAL(33, mTestFilterReceivedFrame.frameContentLength);
  TEST_ASSERT_EQUAL(0, memcmp(&mTestFilterReceivedFrame.frameContent[10], &nifFrame[10], mTestFilterReceivedFrame.frameContentLength - 10));

  TEST_ASSERT_EQUAL(0x01,mTestFilterReceivedFrame.frameOptions.frameType);

  TEST_ASSERT_EQUAL(nifFrame[0],mTestFilterReceivedFrame.frameOptions.homeId[0]);
  TEST_ASSERT_EQUAL(nifFrame[1],mTestFilterReceivedFrame.frameOptions.homeId[1]);
  TEST_ASSERT_EQUAL(nifFrame[2],mTestFilterReceivedFrame.frameOptions.homeId[2]);
  TEST_ASSERT_EQUAL(nifFrame[3],mTestFilterReceivedFrame.frameOptions.homeId[3]);

  TEST_ASSERT_EQUAL(nifFrame[4],mTestFilterReceivedFrame.frameOptions.sourceNodeId);
  TEST_ASSERT_EQUAL(nifFrame[8],mTestFilterReceivedFrame.frameOptions.sequenceNumber);
  TEST_ASSERT_EQUAL(nifFrame[9],mTestFilterReceivedFrame.frameOptions.destinationNodeId);


  TEST_ASSERT_TRUE(mTestFilterReceivedFrame.frameOptions.acknowledge);
  TEST_ASSERT_FALSE(mTestFilterReceivedFrame.frameOptions.speedModified);
  TEST_ASSERT_FALSE(mTestFilterReceivedFrame.frameOptions.lowPower);
  TEST_ASSERT_FALSE(mTestFilterReceivedFrame.frameOptions.routed);
  TEST_ASSERT_FALSE(mTestFilterReceivedFrame.frameOptions.wakeup250ms);
  TEST_ASSERT_TRUE(mTestFilterReceivedFrame.frameOptions.wakeup1000ms);
  TEST_ASSERT_FALSE(mTestFilterReceivedFrame.frameOptions.extended);

  actualVal = llReceiveFilterRemove(&rxFilter);
  TEST_ASSERT_EQUAL(SUCCESS, actualVal);

}


void test_receive_singlecast_frame_with_extended(void)
{
  mock_t* p_mock = NULL;
  zpal_radio_rx_parameters_t rxParameters = {
    .speed = ZPAL_RADIO_SPEED_100K,
    .channel_id = 1,
    .channel_header_format = ZPAL_RADIO_HEADER_TYPE_3CH,
    .rssi = 0
  };

  uint8_t          nifFrame[] = {0xC9, 0x2F, 0x7F, 0xAD, 0x00, 0x81, 0x80, 0x22, 0x22, 0xFF,                                                                   // HomeId, Src, Flags1, Flags2, Length, Dst
                                 0x08, 0x01, 0x01, 0xD3, 0x9C, 0x01, 0x10, 0x01, 0x5E, 0x25, 0x85,
                                 0x8E, 0x59, 0x55, 0x86, 0x72, 0x5A, 0x73, 0x98, 0x9F, 0x6C, 0x7A, // CC vers, NIF, Capabilities, Security, Properties, CC supported,
                                 0xC2, 0xAB}; // Crc.
  ZW_ReturnCode_t actualVal;
  zpal_radio_profile_t radioProfile = { REGION_JP, ZPAL_RADIO_WAKEUP_ALWAYS_LISTEN };
   /** As correct radio initialization is ensured in other tests, we just utilize the helper function in this test. */
  helper_func_expect_radio_init_jp(&radioProfile, p_mock);
  actualVal = llInit(&radioProfile);
  TEST_ASSERT_EQUAL(SUCCESS, actualVal);

  ZW_ReceiveFilter_t rxFilter = {.payloadIndex1 = 0, .payloadFilterValue1 = 0x00, // CC Vers. 1
                                 .payloadIndex2 = 0, .payloadFilterValue2 = 0x00, // NIF
                                 .headerType = HDRTYP_SINGLECAST,                              // Single Cast
                                 .flag = 0,
                                 .frameHandler = receivedFramesFilterCallback};

  TEST_ASSERT_FALSE(mTestFilterCalled);
  actualVal = llReceiveFilterAdd(&rxFilter);
  TEST_ASSERT_EQUAL(SUCCESS, actualVal);
  mp_zpal_received_frame->frame_content_length = sizeof(nifFrame);
  memcpy(mp_zpal_received_frame->frame_content, nifFrame, mp_zpal_received_frame->frame_content_length);

    radioProfile.receive_handler_cb(&rxParameters, mp_zpal_received_frame);


  // test tjhe recived frame information
  TEST_ASSERT_EQUAL(nifFrame[7], mTestFilterReceivedFrame.frameContentLength);
  TEST_ASSERT_EQUAL(0, memcmp(&mTestFilterReceivedFrame.frameContent[10], &nifFrame[10], mTestFilterReceivedFrame.frameContentLength - 10));

  TEST_ASSERT_EQUAL(HDRTYP_SINGLECAST, mTestFilterReceivedFrame.frameOptions.frameType);

  TEST_ASSERT_EQUAL(nifFrame[0],mTestFilterReceivedFrame.frameOptions.homeId[0]);
  TEST_ASSERT_EQUAL(nifFrame[1],mTestFilterReceivedFrame.frameOptions.homeId[1]);
  TEST_ASSERT_EQUAL(nifFrame[2],mTestFilterReceivedFrame.frameOptions.homeId[2]);
  TEST_ASSERT_EQUAL(nifFrame[3],mTestFilterReceivedFrame.frameOptions.homeId[3]);

  TEST_ASSERT_EQUAL(nifFrame[4],mTestFilterReceivedFrame.frameOptions.sourceNodeId);
  TEST_ASSERT_EQUAL(nifFrame[8],mTestFilterReceivedFrame.frameOptions.sequenceNumber);
  TEST_ASSERT_EQUAL(nifFrame[9],mTestFilterReceivedFrame.frameOptions.destinationNodeId);


  TEST_ASSERT_TRUE(mTestFilterReceivedFrame.frameOptions.acknowledge);
  TEST_ASSERT_FALSE(mTestFilterReceivedFrame.frameOptions.speedModified);
  TEST_ASSERT_FALSE(mTestFilterReceivedFrame.frameOptions.lowPower);
  TEST_ASSERT_FALSE(mTestFilterReceivedFrame.frameOptions.routed);
  TEST_ASSERT_FALSE(mTestFilterReceivedFrame.frameOptions.wakeup250ms);
  TEST_ASSERT_FALSE(mTestFilterReceivedFrame.frameOptions.wakeup1000ms);
  TEST_ASSERT_TRUE(mTestFilterReceivedFrame.frameOptions.extended);

  actualVal = llReceiveFilterRemove(&rxFilter);
  TEST_ASSERT_EQUAL(SUCCESS, actualVal);

}


void test_receive_ack_frame(void)
{
  mock_t* p_mock = NULL;
  zpal_radio_rx_parameters_t rxParameters = {
    .speed = ZPAL_RADIO_SPEED_100K,
    .channel_id = 1,
    .channel_header_format = ZPAL_RADIO_HEADER_TYPE_3CH,
    .rssi = 0
  };

  uint8_t        cmdAckFrame[]     = {0xFD, 0x39, 0x34, 0xB0, 0x06, 0x03, 0x00, 0x0C, 0x6A, 0x01, // General Header
                                      0x58, 0xFE};                                          // CRC

  ZW_ReturnCode_t radioInitReturnValueExpected = SUCCESS;
  ZW_ReturnCode_t actualVal;

  zpal_radio_profile_t radioProfile = { REGION_JP, ZPAL_RADIO_WAKEUP_ALWAYS_LISTEN };
  /** As correct radio initialization is ensured in other tests, we just utilize the helper function in this test. */
  helper_func_expect_radio_init_jp(&radioProfile, p_mock);
  actualVal = llInit(&radioProfile);
  TEST_ASSERT_EQUAL(radioInitReturnValueExpected, actualVal);

  ZW_ReceiveFilter_t rxFilter = {.payloadIndex1 = 0, .payloadFilterValue1 = 0x00, // CC Vers. 1
                                 .payloadIndex2 = 0, .payloadFilterValue2 = 0x00, // NIF
                                 .headerType = HDRTYP_TRANSFERACK,                              // Single Cast
                                 .flag = 0,
                                 .frameHandler = receivedFramesFilterCallback};

  TEST_ASSERT_FALSE(mTestFilterCalled);
  actualVal = llReceiveFilterAdd(&rxFilter);
  TEST_ASSERT_EQUAL(SUCCESS, actualVal);
  mp_zpal_received_frame->frame_content_length = sizeof(cmdAckFrame);
  memcpy(mp_zpal_received_frame->frame_content, cmdAckFrame, mp_zpal_received_frame->frame_content_length);

    radioProfile.receive_handler_cb(&rxParameters, mp_zpal_received_frame);
  TEST_ASSERT_TRUE(mTestFilterCalled);
  // test tjhe recived frame information
  TEST_ASSERT_EQUAL(cmdAckFrame[7], mTestFilterReceivedFrame.frameContentLength);
  TEST_ASSERT_EQUAL(0, memcmp(&mTestFilterReceivedFrame.frameContent[10], &cmdAckFrame[10], mTestFilterReceivedFrame.frameContentLength - 10));

  TEST_ASSERT_EQUAL(HDRTYP_TRANSFERACK, mTestFilterReceivedFrame.frameOptions.frameType);

  TEST_ASSERT_EQUAL(cmdAckFrame[0],mTestFilterReceivedFrame.frameOptions.homeId[0]);
  TEST_ASSERT_EQUAL(cmdAckFrame[1],mTestFilterReceivedFrame.frameOptions.homeId[1]);
  TEST_ASSERT_EQUAL(cmdAckFrame[2],mTestFilterReceivedFrame.frameOptions.homeId[2]);
  TEST_ASSERT_EQUAL(cmdAckFrame[3],mTestFilterReceivedFrame.frameOptions.homeId[3]);

  TEST_ASSERT_EQUAL(cmdAckFrame[4],mTestFilterReceivedFrame.frameOptions.sourceNodeId);
  TEST_ASSERT_EQUAL(cmdAckFrame[8],mTestFilterReceivedFrame.frameOptions.sequenceNumber);
  TEST_ASSERT_EQUAL(cmdAckFrame[9],mTestFilterReceivedFrame.frameOptions.destinationNodeId);


  TEST_ASSERT_FALSE(mTestFilterReceivedFrame.frameOptions.acknowledge);
  TEST_ASSERT_FALSE(mTestFilterReceivedFrame.frameOptions.speedModified);
  TEST_ASSERT_FALSE(mTestFilterReceivedFrame.frameOptions.lowPower);
  TEST_ASSERT_FALSE(mTestFilterReceivedFrame.frameOptions.routed);
  TEST_ASSERT_FALSE(mTestFilterReceivedFrame.frameOptions.wakeup250ms);
  TEST_ASSERT_FALSE(mTestFilterReceivedFrame.frameOptions.wakeup1000ms);
  TEST_ASSERT_FALSE(mTestFilterReceivedFrame.frameOptions.extended);

  mTestFilterCalled = false;

  // test receive filters
  // test home id filter
  actualVal = llReceiveFilterRemove(&rxFilter);
  TEST_ASSERT_EQUAL(SUCCESS, actualVal);

  rxFilter.flag = HOMEID_FILTER_FLAG;
  actualVal = llReceiveFilterAdd(&rxFilter);
  TEST_ASSERT_EQUAL(SUCCESS, actualVal);

    radioProfile.receive_handler_cb(&rxParameters, mp_zpal_received_frame);
  TEST_ASSERT_FALSE(mTestFilterCalled);

  actualVal = llReceiveFilterRemove(&rxFilter);
  TEST_ASSERT_EQUAL(SUCCESS, actualVal);

  rxFilter.homeId.array[0] = cmdAckFrame[0];
  rxFilter.homeId.array[1] = cmdAckFrame[1];
  rxFilter.homeId.array[2] = cmdAckFrame[2];
  rxFilter.homeId.array[3] = cmdAckFrame[3];

  mTestFilterCalled = false;
  actualVal = llReceiveFilterAdd(&rxFilter);
  TEST_ASSERT_EQUAL(SUCCESS, actualVal);

    radioProfile.receive_handler_cb(&rxParameters, mp_zpal_received_frame);
  TEST_ASSERT_TRUE(mTestFilterCalled);

  TEST_ASSERT_EQUAL(cmdAckFrame[0],mTestFilterReceivedFrame.frameOptions.homeId[0]);
  TEST_ASSERT_EQUAL(cmdAckFrame[1],mTestFilterReceivedFrame.frameOptions.homeId[1]);
  TEST_ASSERT_EQUAL(cmdAckFrame[2],mTestFilterReceivedFrame.frameOptions.homeId[2]);
  TEST_ASSERT_EQUAL(cmdAckFrame[3],mTestFilterReceivedFrame.frameOptions.homeId[3]);

  TEST_ASSERT_EQUAL(cmdAckFrame[4],mTestFilterReceivedFrame.frameOptions.sourceNodeId);
  TEST_ASSERT_EQUAL(cmdAckFrame[8],mTestFilterReceivedFrame.frameOptions.sequenceNumber);
  TEST_ASSERT_EQUAL(cmdAckFrame[9],mTestFilterReceivedFrame.frameOptions.destinationNodeId);


  TEST_ASSERT_FALSE(mTestFilterReceivedFrame.frameOptions.acknowledge);
  TEST_ASSERT_FALSE(mTestFilterReceivedFrame.frameOptions.speedModified);
  TEST_ASSERT_FALSE(mTestFilterReceivedFrame.frameOptions.lowPower);
  TEST_ASSERT_FALSE(mTestFilterReceivedFrame.frameOptions.routed);
  TEST_ASSERT_FALSE(mTestFilterReceivedFrame.frameOptions.wakeup250ms);
  TEST_ASSERT_FALSE(mTestFilterReceivedFrame.frameOptions.wakeup1000ms);
  TEST_ASSERT_FALSE(mTestFilterReceivedFrame.frameOptions.extended);

  mTestFilterCalled = false;
  actualVal = llReceiveFilterRemove(&rxFilter);
  TEST_ASSERT_EQUAL(SUCCESS, actualVal);
  // test source and destination filter
  rxFilter.flag = (SOURCE_NODE_ID_FILTER_FLAG | DESTINATION_NODE_ID_FILTER_FLAG);
  rxFilter.sourceNodeId = 0xFF;
  rxFilter.destinationNodeId = 0xFF;

  actualVal = llReceiveFilterAdd(&rxFilter);
  TEST_ASSERT_EQUAL(SUCCESS, actualVal);

    radioProfile.receive_handler_cb(&rxParameters, mp_zpal_received_frame);
  TEST_ASSERT_FALSE(mTestFilterCalled);

  actualVal = llReceiveFilterRemove(&rxFilter);
  TEST_ASSERT_EQUAL(SUCCESS, actualVal);
  rxFilter.sourceNodeId = 0x06;

  actualVal = llReceiveFilterAdd(&rxFilter);
  TEST_ASSERT_EQUAL(SUCCESS, actualVal);

    radioProfile.receive_handler_cb(&rxParameters, mp_zpal_received_frame);
  TEST_ASSERT_FALSE(mTestFilterCalled);

  actualVal = llReceiveFilterRemove(&rxFilter);
  TEST_ASSERT_EQUAL(SUCCESS, actualVal);

  rxFilter.destinationNodeId = 0x01;
  actualVal = llReceiveFilterAdd(&rxFilter);
  TEST_ASSERT_EQUAL(SUCCESS, actualVal);
    radioProfile.receive_handler_cb(&rxParameters, mp_zpal_received_frame);
  TEST_ASSERT_TRUE(mTestFilterCalled);

  // payload filter
  actualVal = llReceiveFilterRemove(&rxFilter);
  TEST_ASSERT_EQUAL(SUCCESS, actualVal);

  mTestFilterCalled = false;

  rxFilter.flag = PAYLOAD_INDEX_1_FILTER_FLAG;
  actualVal = llReceiveFilterAdd(&rxFilter);
  TEST_ASSERT_EQUAL(INVALID_PARAMETERS, actualVal);


  rxFilter.flag = PAYLOAD_INDEX_2_FILTER_FLAG;
  actualVal = llReceiveFilterAdd(&rxFilter);
  TEST_ASSERT_EQUAL(INVALID_PARAMETERS, actualVal);



}



void test_receive_routed_frame(void)
{
  mock_t* p_mock = NULL;
  zpal_radio_rx_parameters_t rxParameters = {
    .speed = ZPAL_RADIO_SPEED_100K,
    .channel_id = 1,
    .channel_header_format = ZPAL_RADIO_HEADER_TYPE_3CH,
    .rssi = 0
  };

  uint8_t        cmdRoutedFrame[]     = {0xFD, 0x39, 0x34, 0xB0, 0x06, 0x88, 0x00, 0x13, 0x6A, 0x01, // General Header
                                         0x00, 0x10, 0x05, 0x00,    // routing header
                                         0x3C, 0x02, 0xAF,         //payload (dummy)
                                         0xB5, 0xE4};                                          // CRC

  ZW_ReturnCode_t radioInitReturnValueExpected = SUCCESS;
  ZW_ReturnCode_t actualVal;

  zpal_radio_profile_t radioProfile = { REGION_JP, ZPAL_RADIO_WAKEUP_ALWAYS_LISTEN };
  /** As correct radio initialization is ensured in other tests, we just utilize the helper function in this test. */
  helper_func_expect_radio_init_jp(&radioProfile, p_mock);
  actualVal = llInit(&radioProfile);
  TEST_ASSERT_EQUAL(radioInitReturnValueExpected, actualVal);

  ZW_ReceiveFilter_t rxFilter = {.payloadIndex1 = 0, .payloadFilterValue1 = 0x00, // CC Vers. 1
                                 .payloadIndex2 = 0, .payloadFilterValue2 = 0x00, // NIF
                                 .headerType = HDRTYP_ROUTED,                              // routed frame
                                 .flag = 0,
                                 .frameHandler = receivedFramesFilterCallback};

  TEST_ASSERT_FALSE(mTestFilterCalled);
  actualVal = llReceiveFilterAdd(&rxFilter);
  TEST_ASSERT_EQUAL(SUCCESS, actualVal);
  mp_zpal_received_frame->frame_content_length = sizeof(cmdRoutedFrame);
  memcpy(mp_zpal_received_frame->frame_content, cmdRoutedFrame, mp_zpal_received_frame->frame_content_length);

  radioProfile.receive_handler_cb(&rxParameters, mp_zpal_received_frame);
  TEST_ASSERT_TRUE(mTestFilterCalled);
  // test tjhe recived frame information
  TEST_ASSERT_EQUAL(cmdRoutedFrame[7], mTestFilterReceivedFrame.frameContentLength);
  TEST_ASSERT_EQUAL(0, memcmp(&mTestFilterReceivedFrame.frameContent[10], &cmdRoutedFrame[10], mTestFilterReceivedFrame.frameContentLength - 10));

  TEST_ASSERT_EQUAL(HDRTYP_ROUTED, mTestFilterReceivedFrame.frameOptions.frameType);

  TEST_ASSERT_EQUAL(cmdRoutedFrame[0],mTestFilterReceivedFrame.frameOptions.homeId[0]);
  TEST_ASSERT_EQUAL(cmdRoutedFrame[1],mTestFilterReceivedFrame.frameOptions.homeId[1]);
  TEST_ASSERT_EQUAL(cmdRoutedFrame[2],mTestFilterReceivedFrame.frameOptions.homeId[2]);
  TEST_ASSERT_EQUAL(cmdRoutedFrame[3],mTestFilterReceivedFrame.frameOptions.homeId[3]);

  TEST_ASSERT_EQUAL(cmdRoutedFrame[4],mTestFilterReceivedFrame.frameOptions.sourceNodeId);
  TEST_ASSERT_EQUAL(cmdRoutedFrame[8],mTestFilterReceivedFrame.frameOptions.sequenceNumber);
  TEST_ASSERT_EQUAL(cmdRoutedFrame[9],mTestFilterReceivedFrame.frameOptions.destinationNodeId);


  TEST_ASSERT_TRUE(mTestFilterReceivedFrame.frameOptions.acknowledge);
  TEST_ASSERT_FALSE(mTestFilterReceivedFrame.frameOptions.speedModified);
  TEST_ASSERT_FALSE(mTestFilterReceivedFrame.frameOptions.lowPower);
  TEST_ASSERT_TRUE(mTestFilterReceivedFrame.frameOptions.routed);
  TEST_ASSERT_FALSE(mTestFilterReceivedFrame.frameOptions.wakeup250ms);
  TEST_ASSERT_FALSE(mTestFilterReceivedFrame.frameOptions.wakeup1000ms);
  TEST_ASSERT_FALSE(mTestFilterReceivedFrame.frameOptions.extended);

  mTestFilterCalled = false;

  // test receive filters
  // test home id filter
  actualVal = llReceiveFilterRemove(&rxFilter);
  TEST_ASSERT_EQUAL(SUCCESS, actualVal);

  rxFilter.flag = HOMEID_FILTER_FLAG;
  actualVal = llReceiveFilterAdd(&rxFilter);
  TEST_ASSERT_EQUAL(SUCCESS, actualVal);

  radioProfile.receive_handler_cb(&rxParameters, mp_zpal_received_frame);
  TEST_ASSERT_FALSE(mTestFilterCalled);

  actualVal = llReceiveFilterRemove(&rxFilter);
  TEST_ASSERT_EQUAL(SUCCESS, actualVal);

  rxFilter.homeId.array[0] = cmdRoutedFrame[0];
  rxFilter.homeId.array[1] = cmdRoutedFrame[1];
  rxFilter.homeId.array[2] = cmdRoutedFrame[2];
  rxFilter.homeId.array[3] = cmdRoutedFrame[3];

  mTestFilterCalled = false;
  actualVal = llReceiveFilterAdd(&rxFilter);
  TEST_ASSERT_EQUAL(SUCCESS, actualVal);

  radioProfile.receive_handler_cb(&rxParameters, mp_zpal_received_frame);
  TEST_ASSERT_TRUE(mTestFilterCalled);


  mTestFilterCalled = false;
  actualVal = llReceiveFilterRemove(&rxFilter);
  TEST_ASSERT_EQUAL(SUCCESS, actualVal);
  // test source and destination filter
  rxFilter.flag = (SOURCE_NODE_ID_FILTER_FLAG | DESTINATION_NODE_ID_FILTER_FLAG);
  rxFilter.sourceNodeId = 0xFF;
  rxFilter.destinationNodeId = 0xFF;

  actualVal = llReceiveFilterAdd(&rxFilter);
  TEST_ASSERT_EQUAL(SUCCESS, actualVal);

  radioProfile.receive_handler_cb(&rxParameters, mp_zpal_received_frame);
  TEST_ASSERT_FALSE(mTestFilterCalled);

  actualVal = llReceiveFilterRemove(&rxFilter);
  TEST_ASSERT_EQUAL(SUCCESS, actualVal);
  rxFilter.sourceNodeId = 0x06;

  actualVal = llReceiveFilterAdd(&rxFilter);
  TEST_ASSERT_EQUAL(SUCCESS, actualVal);

  radioProfile.receive_handler_cb(&rxParameters, mp_zpal_received_frame);
  TEST_ASSERT_FALSE(mTestFilterCalled);

  actualVal = llReceiveFilterRemove(&rxFilter);
  TEST_ASSERT_EQUAL(SUCCESS, actualVal);

  rxFilter.destinationNodeId = 0x01;
  actualVal = llReceiveFilterAdd(&rxFilter);
  TEST_ASSERT_EQUAL(SUCCESS, actualVal);
  radioProfile.receive_handler_cb(&rxParameters, mp_zpal_received_frame);
  TEST_ASSERT_TRUE(mTestFilterCalled);

  // payload filter
  actualVal = llReceiveFilterRemove(&rxFilter);
  TEST_ASSERT_EQUAL(SUCCESS, actualVal);

  mTestFilterCalled = false;

  rxFilter.flag =PAYLOAD_INDEX_1_FILTER_FLAG;
  rxFilter.payloadIndex1 = 0;
  rxFilter.payloadFilterValue1 = 0xFF;
  actualVal = llReceiveFilterAdd(&rxFilter);
  TEST_ASSERT_EQUAL(SUCCESS, actualVal);

  radioProfile.receive_handler_cb(&rxParameters, mp_zpal_received_frame);
  TEST_ASSERT_FALSE(mTestFilterCalled);


  actualVal = llReceiveFilterRemove(&rxFilter);
  TEST_ASSERT_EQUAL(SUCCESS, actualVal);

  mTestFilterCalled = false;

  rxFilter.flag =PAYLOAD_INDEX_1_FILTER_FLAG;
  rxFilter.payloadIndex1 = 0;
  rxFilter.payloadFilterValue1 = 0x3C;
  actualVal = llReceiveFilterAdd(&rxFilter);
  TEST_ASSERT_EQUAL(SUCCESS, actualVal);

  radioProfile.receive_handler_cb(&rxParameters, mp_zpal_received_frame);
  TEST_ASSERT_TRUE(mTestFilterCalled);

///////////////////////////////////////
  actualVal = llReceiveFilterRemove(&rxFilter);
  TEST_ASSERT_EQUAL(SUCCESS, actualVal);

  mTestFilterCalled = false;

  rxFilter.flag =PAYLOAD_INDEX_2_FILTER_FLAG;
  rxFilter.payloadIndex2 = 1;
  rxFilter.payloadFilterValue2 = 0xFF;
  actualVal = llReceiveFilterAdd(&rxFilter);
  TEST_ASSERT_EQUAL(SUCCESS, actualVal);

  radioProfile.receive_handler_cb(&rxParameters, mp_zpal_received_frame);
  TEST_ASSERT_FALSE(mTestFilterCalled);


  actualVal = llReceiveFilterRemove(&rxFilter);
  TEST_ASSERT_EQUAL(SUCCESS, actualVal);

  rxFilter.payloadIndex2 = 1;
  rxFilter.payloadFilterValue2 = 0x02;
  actualVal = llReceiveFilterAdd(&rxFilter);
  TEST_ASSERT_EQUAL(SUCCESS, actualVal);

  radioProfile.receive_handler_cb(&rxParameters, mp_zpal_received_frame);
  TEST_ASSERT_TRUE(mTestFilterCalled);

//////////////////////////////
  actualVal = llReceiveFilterRemove(&rxFilter);
  TEST_ASSERT_EQUAL(SUCCESS, actualVal);

  mTestFilterCalled = false;

  rxFilter.flag =(PAYLOAD_INDEX_2_FILTER_FLAG | PAYLOAD_INDEX_2_FILTER_FLAG);
  rxFilter.payloadIndex1 = 0;
  rxFilter.payloadFilterValue1 = 0x3C;
  rxFilter.payloadIndex2 = 2;
  rxFilter.payloadFilterValue2 = 0xAF;
  actualVal = llReceiveFilterAdd(&rxFilter);
  TEST_ASSERT_EQUAL(SUCCESS, actualVal);

  radioProfile.receive_handler_cb(&rxParameters, mp_zpal_received_frame);
  TEST_ASSERT_TRUE(mTestFilterCalled);

  actualVal = llReceiveFilterRemove(&rxFilter);
  TEST_ASSERT_EQUAL(SUCCESS, actualVal);


}



void test_receive_explorer_frame(void)
{
  mock_t* p_mock = NULL;
  zpal_radio_rx_parameters_t rxParameters = {
    .speed = ZPAL_RADIO_SPEED_100K,
    .channel_id = 1,
    .channel_header_format = ZPAL_RADIO_HEADER_TYPE_3CH,
    .rssi = 0
  };

  uint8_t        cmdExplorerFrame[]     = {0xFD, 0x39, 0x34, 0xB0, 0x01, 0x05, 0x00, 0x18, 0x59, 0xFF, // General Header
                                           0x20, 0x00, 0xFA, 0x31, 0x05, 0x00, 0x00, 0x00,       // Explorer header. Version = 1, Command Type = 0 (Explorer Cmd Normal),  Session Tx Random = 0xFA, Routed, direction, stop, stop req all 0, Session TTL = 4, Repeater list empty
                                           0x01, 0x22, 0x01, 0x00,                               // Z-Wave protocol vers. = 1, Cmd set nwi (0x22), Idle = 0, Timeout Default = 0
                                           0x6A, 0x35};                                                // CRC

  ZW_ReturnCode_t radioInitReturnValueExpected = SUCCESS;
  ZW_ReturnCode_t actualVal;

  zpal_radio_profile_t radioProfile = { REGION_JP, ZPAL_RADIO_WAKEUP_ALWAYS_LISTEN };
  /** As correct radio initialization is ensured in other tests, we just utilize the helper function in this test. */
  helper_func_expect_radio_init_jp(&radioProfile, p_mock);
  actualVal = llInit(&radioProfile);
  TEST_ASSERT_EQUAL(radioInitReturnValueExpected, actualVal);

  ZW_ReceiveFilter_t rxFilter = {.payloadIndex1 = 0, .payloadFilterValue1 = 0x00, // CC Vers. 1
                                 .payloadIndex2 = 0, .payloadFilterValue2 = 0x00, // NIF
                                 .headerType = HDRTYP_EXPLORE,                              // routed frame
                                 .flag = 0,
                                 .frameHandler = receivedFramesFilterCallback};

  TEST_ASSERT_FALSE(mTestFilterCalled);
  actualVal = llReceiveFilterAdd(&rxFilter);
  TEST_ASSERT_EQUAL(SUCCESS, actualVal);
  mp_zpal_received_frame->frame_content_length = sizeof(cmdExplorerFrame);
  memcpy(mp_zpal_received_frame->frame_content, cmdExplorerFrame, mp_zpal_received_frame->frame_content_length);

  radioProfile.receive_handler_cb(&rxParameters, mp_zpal_received_frame);
  TEST_ASSERT_TRUE(mTestFilterCalled);
  // test tjhe recived frame information
  TEST_ASSERT_EQUAL(cmdExplorerFrame[7], mTestFilterReceivedFrame.frameContentLength);
  TEST_ASSERT_EQUAL(0, memcmp(&mTestFilterReceivedFrame.frameContent[10], &cmdExplorerFrame[10], mTestFilterReceivedFrame.frameContentLength - 10));

  TEST_ASSERT_EQUAL(HDRTYP_EXPLORE, mTestFilterReceivedFrame.frameOptions.frameType);

  TEST_ASSERT_EQUAL(cmdExplorerFrame[0],mTestFilterReceivedFrame.frameOptions.homeId[0]);
  TEST_ASSERT_EQUAL(cmdExplorerFrame[1],mTestFilterReceivedFrame.frameOptions.homeId[1]);
  TEST_ASSERT_EQUAL(cmdExplorerFrame[2],mTestFilterReceivedFrame.frameOptions.homeId[2]);
  TEST_ASSERT_EQUAL(cmdExplorerFrame[3],mTestFilterReceivedFrame.frameOptions.homeId[3]);

  TEST_ASSERT_EQUAL(cmdExplorerFrame[4],mTestFilterReceivedFrame.frameOptions.sourceNodeId);
  TEST_ASSERT_EQUAL(cmdExplorerFrame[8],mTestFilterReceivedFrame.frameOptions.sequenceNumber);
  TEST_ASSERT_EQUAL(cmdExplorerFrame[9],mTestFilterReceivedFrame.frameOptions.destinationNodeId);


  TEST_ASSERT_FALSE(mTestFilterReceivedFrame.frameOptions.acknowledge);
  TEST_ASSERT_FALSE(mTestFilterReceivedFrame.frameOptions.speedModified);
  TEST_ASSERT_FALSE(mTestFilterReceivedFrame.frameOptions.lowPower);
  TEST_ASSERT_FALSE(mTestFilterReceivedFrame.frameOptions.routed);
  TEST_ASSERT_FALSE(mTestFilterReceivedFrame.frameOptions.wakeup250ms);
  TEST_ASSERT_FALSE(mTestFilterReceivedFrame.frameOptions.wakeup1000ms);
  TEST_ASSERT_FALSE(mTestFilterReceivedFrame.frameOptions.extended);

  mTestFilterCalled = false;

  // test receive filters
  // test home id filter
  actualVal = llReceiveFilterRemove(&rxFilter);
  TEST_ASSERT_EQUAL(SUCCESS, actualVal);

  rxFilter.flag = HOMEID_FILTER_FLAG;
  actualVal = llReceiveFilterAdd(&rxFilter);
  TEST_ASSERT_EQUAL(SUCCESS, actualVal);

  radioProfile.receive_handler_cb(&rxParameters, mp_zpal_received_frame);
  TEST_ASSERT_FALSE(mTestFilterCalled);

  actualVal = llReceiveFilterRemove(&rxFilter);
  TEST_ASSERT_EQUAL(SUCCESS, actualVal);

  rxFilter.homeId.array[0] = cmdExplorerFrame[0];
  rxFilter.homeId.array[1] = cmdExplorerFrame[1];
  rxFilter.homeId.array[2] = cmdExplorerFrame[2];
  rxFilter.homeId.array[3] = cmdExplorerFrame[3];

  mTestFilterCalled = false;
  actualVal = llReceiveFilterAdd(&rxFilter);
  TEST_ASSERT_EQUAL(SUCCESS, actualVal);

  radioProfile.receive_handler_cb(&rxParameters, mp_zpal_received_frame);
  TEST_ASSERT_TRUE(mTestFilterCalled);


  mTestFilterCalled = false;
  actualVal = llReceiveFilterRemove(&rxFilter);
  TEST_ASSERT_EQUAL(SUCCESS, actualVal);
  // test source and destination filter
  rxFilter.flag = (SOURCE_NODE_ID_FILTER_FLAG | DESTINATION_NODE_ID_FILTER_FLAG);
  rxFilter.sourceNodeId = 0xFF;
  rxFilter.destinationNodeId = 0xFE;

  actualVal = llReceiveFilterAdd(&rxFilter);
  TEST_ASSERT_EQUAL(SUCCESS, actualVal);

  radioProfile.receive_handler_cb(&rxParameters, mp_zpal_received_frame);
  TEST_ASSERT_FALSE(mTestFilterCalled);

  actualVal = llReceiveFilterRemove(&rxFilter);
  TEST_ASSERT_EQUAL(SUCCESS, actualVal);
  rxFilter.sourceNodeId = 0x01;

  actualVal = llReceiveFilterAdd(&rxFilter);
  TEST_ASSERT_EQUAL(SUCCESS, actualVal);

  radioProfile.receive_handler_cb(&rxParameters, mp_zpal_received_frame);
  TEST_ASSERT_FALSE(mTestFilterCalled);

  actualVal = llReceiveFilterRemove(&rxFilter);
  TEST_ASSERT_EQUAL(SUCCESS, actualVal);

  rxFilter.destinationNodeId = 0xFF;
  actualVal = llReceiveFilterAdd(&rxFilter);
  TEST_ASSERT_EQUAL(SUCCESS, actualVal);
    radioProfile.receive_handler_cb(&rxParameters, mp_zpal_received_frame);
  TEST_ASSERT_TRUE(mTestFilterCalled);

  // payload filter
  actualVal = llReceiveFilterRemove(&rxFilter);
  TEST_ASSERT_EQUAL(SUCCESS, actualVal);

  mTestFilterCalled = false;

//   TODO right now this filter is not relevant to exploer frames
//  rxFilter.flag = PAYLOAD_INDEX_1_FILTER_FLAG;
//  rxFilter.payloadIndex1 = 0;
//  rxFilter.payloadFilterValue1 = 0xFF;
//  actualVal = llReceiveFilterAdd(&rxFilter);
//  TEST_ASSERT_EQUAL(SUCCESS, actualVal);
//
//    radioProfile.receive_handler_cb(&rxParameters, mp_zpal_received_frame);
//  TEST_ASSERT_FALSE(mTestFilterCalled);
//
//  actualVal = llReceiveFilterRemove(&rxFilter);
//  TEST_ASSERT_EQUAL(SUCCESS, actualVal);
//
//  mTestFilterCalled = false;
//
//  rxFilter.flag =PAYLOAD_INDEX_1_FILTER_FLAG;
//  rxFilter.payloadIndex1 = 2;
//  rxFilter.payloadFilterValue1 = 0xFA;
//  actualVal = llReceiveFilterAdd(&rxFilter);
//  TEST_ASSERT_EQUAL(SUCCESS, actualVal);
//
//    radioProfile.receive_handler_cb(&rxParameters, mp_zpal_received_frame);
//  TEST_ASSERT_TRUE(mTestFilterCalled);
//
/////////////////////////////////////////
//  actualVal = llReceiveFilterRemove(&rxFilter);
//  TEST_ASSERT_EQUAL(SUCCESS, actualVal);
//
//  mTestFilterCalled = false;
//
//  rxFilter.flag =PAYLOAD_INDEX_2_FILTER_FLAG;
//  rxFilter.payloadIndex2 = 0;
//  rxFilter.payloadFilterValue2 = 0xFF;
//  actualVal = llReceiveFilterAdd(&rxFilter);
//  TEST_ASSERT_EQUAL(SUCCESS, actualVal);
//
//    radioProfile.receive_handler_cb(&rxParameters, mp_zpal_received_frame);
//  TEST_ASSERT_FALSE(mTestFilterCalled);
//
//
//  actualVal = llReceiveFilterRemove(&rxFilter);
//  TEST_ASSERT_EQUAL(SUCCESS, actualVal);
//
//  rxFilter.payloadIndex2 = 3;
//  rxFilter.payloadFilterValue2 = 0x31;
//  actualVal = llReceiveFilterAdd(&rxFilter);
//  TEST_ASSERT_EQUAL(SUCCESS, actualVal);
//
//    radioProfile.receive_handler_cb(&rxParameters, mp_zpal_received_frame);
//  TEST_ASSERT_TRUE(mTestFilterCalled);
//
////////////////////////////////
//  actualVal = llReceiveFilterRemove(&rxFilter);
//  TEST_ASSERT_EQUAL(SUCCESS, actualVal);
//
//  mTestFilterCalled = false;
//
//  rxFilter.flag =(PAYLOAD_INDEX_2_FILTER_FLAG | PAYLOAD_INDEX_2_FILTER_FLAG);
//  rxFilter.payloadIndex1 = 0;
//  rxFilter.payloadFilterValue1 = 0x20;
//  rxFilter.payloadIndex2 = 4;
//  rxFilter.payloadFilterValue2 = 0x05;
//  actualVal = llReceiveFilterAdd(&rxFilter);
//  TEST_ASSERT_EQUAL(SUCCESS, actualVal);
//
//    radioProfile.receive_handler_cb(&rxParameters, mp_zpal_received_frame);
//  TEST_ASSERT_TRUE(mTestFilterCalled);
//
//  actualVal = llReceiveFilterRemove(&rxFilter);
//  TEST_ASSERT_EQUAL(SUCCESS, actualVal);

}





void test_receive_multicast_frame(void)
{
  mock_t* p_mock = NULL;
  zpal_radio_rx_parameters_t rxParameters = {
    .speed = ZPAL_RADIO_SPEED_100K,
    .channel_id = 1,
    .channel_header_format = ZPAL_RADIO_HEADER_TYPE_3CH,
    .rssi = 0
  };

  uint8_t        cmdMultiCastFrame[]     = {0xE3, 0x72, 0x83, 0xAF, 0x02, 0x02, 0x00, 0x2B, 0x03, // General Header.
                                            0x1D, 0x0D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                            0x00, 0x00, 0x00, // Multicast frame, Address offset + Mask bytes, Mask byte. (Node: 1, 3, 4)
                                            0x5A, 0x01, // Payload, Device Reset Locally
                                            0x7E, 0xB2};// CRC-CCITT

  ZW_ReturnCode_t radioInitReturnValueExpected = SUCCESS;
  ZW_ReturnCode_t actualVal;

  zpal_radio_profile_t radioProfile = { REGION_JP, ZPAL_RADIO_WAKEUP_ALWAYS_LISTEN };
   /** As correct radio initialization is ensured in other tests, we just utilize the helper function in this test. */
  helper_func_expect_radio_init_jp(&radioProfile, p_mock);
  actualVal = llInit(&radioProfile);
  TEST_ASSERT_EQUAL(radioInitReturnValueExpected, actualVal);

  ZW_ReceiveFilter_t rxFilter = {.payloadIndex1 = 0, .payloadFilterValue1 = 0x00, // CC Vers. 1
                                 .payloadIndex2 = 0, .payloadFilterValue2 = 0x00, // NIF
                                 .headerType = HDRTYP_MULTICAST,                              // routed frame
                                 .flag = 0,
                                 .frameHandler = receivedFramesFilterCallback};

  TEST_ASSERT_FALSE(mTestFilterCalled);
  actualVal = llReceiveFilterAdd(&rxFilter);
  TEST_ASSERT_EQUAL(SUCCESS, actualVal);
  mp_zpal_received_frame->frame_content_length = sizeof(cmdMultiCastFrame);
  memcpy(mp_zpal_received_frame->frame_content, cmdMultiCastFrame, mp_zpal_received_frame->frame_content_length);

  radioProfile.receive_handler_cb(&rxParameters, mp_zpal_received_frame);
  TEST_ASSERT_TRUE(mTestFilterCalled);
  // test tjhe recived frame information
  TEST_ASSERT_EQUAL(cmdMultiCastFrame[7], mTestFilterReceivedFrame.frameContentLength);
  TEST_ASSERT_EQUAL(0, memcmp(&mTestFilterReceivedFrame.frameContent[10], &cmdMultiCastFrame[10], mTestFilterReceivedFrame.frameContentLength - 10));

  TEST_ASSERT_EQUAL(HDRTYP_MULTICAST, mTestFilterReceivedFrame.frameOptions.frameType);

  TEST_ASSERT_EQUAL(cmdMultiCastFrame[0],mTestFilterReceivedFrame.frameOptions.homeId[0]);
  TEST_ASSERT_EQUAL(cmdMultiCastFrame[1],mTestFilterReceivedFrame.frameOptions.homeId[1]);
  TEST_ASSERT_EQUAL(cmdMultiCastFrame[2],mTestFilterReceivedFrame.frameOptions.homeId[2]);
  TEST_ASSERT_EQUAL(cmdMultiCastFrame[3],mTestFilterReceivedFrame.frameOptions.homeId[3]);

  TEST_ASSERT_EQUAL(cmdMultiCastFrame[4],mTestFilterReceivedFrame.frameOptions.sourceNodeId);
  TEST_ASSERT_EQUAL(cmdMultiCastFrame[8],mTestFilterReceivedFrame.frameOptions.sequenceNumber);

  TEST_ASSERT_FALSE(mTestFilterReceivedFrame.frameOptions.acknowledge);
  TEST_ASSERT_FALSE(mTestFilterReceivedFrame.frameOptions.speedModified);
  TEST_ASSERT_FALSE(mTestFilterReceivedFrame.frameOptions.lowPower);
  TEST_ASSERT_FALSE(mTestFilterReceivedFrame.frameOptions.routed);
  TEST_ASSERT_FALSE(mTestFilterReceivedFrame.frameOptions.wakeup250ms);
  TEST_ASSERT_FALSE(mTestFilterReceivedFrame.frameOptions.wakeup1000ms);
  TEST_ASSERT_FALSE(mTestFilterReceivedFrame.frameOptions.extended);

  mTestFilterCalled = false;
  actualVal = llReceiveFilterRemove(&rxFilter);
  TEST_ASSERT_EQUAL(SUCCESS, actualVal);

  // test receive filters
  // test home id filter

  rxFilter.flag = HOMEID_FILTER_FLAG;
  actualVal = llReceiveFilterAdd(&rxFilter);
  TEST_ASSERT_EQUAL(SUCCESS, actualVal);

  radioProfile.receive_handler_cb(&rxParameters, mp_zpal_received_frame);
  TEST_ASSERT_FALSE(mTestFilterCalled);

  actualVal = llReceiveFilterRemove(&rxFilter);
  TEST_ASSERT_EQUAL(SUCCESS, actualVal);

  rxFilter.homeId.array[0] = cmdMultiCastFrame[0];
  rxFilter.homeId.array[1] = cmdMultiCastFrame[1];
  rxFilter.homeId.array[2] = cmdMultiCastFrame[2];
  rxFilter.homeId.array[3] = cmdMultiCastFrame[3];

  mTestFilterCalled = false;
  actualVal = llReceiveFilterAdd(&rxFilter);
  TEST_ASSERT_EQUAL(SUCCESS, actualVal);

  radioProfile.receive_handler_cb(&rxParameters, mp_zpal_received_frame);
  TEST_ASSERT_TRUE(mTestFilterCalled);


  mTestFilterCalled = false;
  actualVal = llReceiveFilterRemove(&rxFilter);
  TEST_ASSERT_EQUAL(SUCCESS, actualVal);
  // test source and destination filter
  rxFilter.flag = SOURCE_NODE_ID_FILTER_FLAG;
  rxFilter.sourceNodeId = 0xFF;

  actualVal = llReceiveFilterAdd(&rxFilter);
  TEST_ASSERT_EQUAL(SUCCESS, actualVal);

  radioProfile.receive_handler_cb(&rxParameters, mp_zpal_received_frame);
  TEST_ASSERT_FALSE(mTestFilterCalled);

  actualVal = llReceiveFilterRemove(&rxFilter);
  TEST_ASSERT_EQUAL(SUCCESS, actualVal);
  rxFilter.sourceNodeId = 0x02;

  actualVal = llReceiveFilterAdd(&rxFilter);
  TEST_ASSERT_EQUAL(SUCCESS, actualVal);

  radioProfile.receive_handler_cb(&rxParameters, mp_zpal_received_frame);
  TEST_ASSERT_TRUE(mTestFilterCalled);


  // payload filter
  actualVal = llReceiveFilterRemove(&rxFilter);
  TEST_ASSERT_EQUAL(SUCCESS, actualVal);

  mTestFilterCalled = false;

  rxFilter.flag =PAYLOAD_INDEX_1_FILTER_FLAG;
  rxFilter.payloadIndex1 = 0;
  rxFilter.payloadFilterValue1 = 0xFF;
  actualVal = llReceiveFilterAdd(&rxFilter);
  TEST_ASSERT_EQUAL(SUCCESS, actualVal);

  radioProfile.receive_handler_cb(&rxParameters, mp_zpal_received_frame);
  TEST_ASSERT_FALSE(mTestFilterCalled);


  actualVal = llReceiveFilterRemove(&rxFilter);
  TEST_ASSERT_EQUAL(SUCCESS, actualVal);

  mTestFilterCalled = false;

  rxFilter.flag =PAYLOAD_INDEX_1_FILTER_FLAG;
  rxFilter.payloadIndex1 = 0;
  rxFilter.payloadFilterValue1 = 0x1D;
  actualVal = llReceiveFilterAdd(&rxFilter);
  TEST_ASSERT_EQUAL(SUCCESS, actualVal);

  radioProfile.receive_handler_cb(&rxParameters, mp_zpal_received_frame);
  TEST_ASSERT_TRUE(mTestFilterCalled);

///////////////////////////////////////
  actualVal = llReceiveFilterRemove(&rxFilter);
  TEST_ASSERT_EQUAL(SUCCESS, actualVal);

  mTestFilterCalled = false;

  rxFilter.flag =PAYLOAD_INDEX_2_FILTER_FLAG;
  rxFilter.payloadIndex2 = 0;
  rxFilter.payloadFilterValue2 = 0xFF;
  actualVal = llReceiveFilterAdd(&rxFilter);
  TEST_ASSERT_EQUAL(SUCCESS, actualVal);

  radioProfile.receive_handler_cb(&rxParameters, mp_zpal_received_frame);
  TEST_ASSERT_FALSE(mTestFilterCalled);


  actualVal = llReceiveFilterRemove(&rxFilter);
  TEST_ASSERT_EQUAL(SUCCESS, actualVal);

  rxFilter.payloadIndex2 = 1;
  rxFilter.payloadFilterValue2 = 0x0D;
  actualVal = llReceiveFilterAdd(&rxFilter);
  TEST_ASSERT_EQUAL(SUCCESS, actualVal);

  radioProfile.receive_handler_cb(&rxParameters, mp_zpal_received_frame);
  TEST_ASSERT_TRUE(mTestFilterCalled);

//////////////////////////////
  actualVal = llReceiveFilterRemove(&rxFilter);
  TEST_ASSERT_EQUAL(SUCCESS, actualVal);

  mTestFilterCalled = false;

  rxFilter.flag =(PAYLOAD_INDEX_2_FILTER_FLAG | PAYLOAD_INDEX_2_FILTER_FLAG);
  rxFilter.payloadIndex1 = 0;
  rxFilter.payloadFilterValue1 = 0x1D;
  rxFilter.payloadIndex2 = 1;
  rxFilter.payloadFilterValue2 = 0x0D;
  actualVal = llReceiveFilterAdd(&rxFilter);
  TEST_ASSERT_EQUAL(SUCCESS, actualVal);

  radioProfile.receive_handler_cb(&rxParameters, mp_zpal_received_frame);
  TEST_ASSERT_TRUE(mTestFilterCalled);

  actualVal = llReceiveFilterRemove(&rxFilter);
  TEST_ASSERT_EQUAL(SUCCESS, actualVal);


}


