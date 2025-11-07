// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file TestReceiveFilterLinkLayer_3ch.c
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
static bool mTestFilter0Called = false;
static bool mTestFilter1Called = false;
static bool mTestFilter2Called = false;
static bool mTestFilter3Called = false;
static bool mTestFilter4Called = false;
static bool mTestFilter5Called = false;
static bool mTestFilter6Called = false;
static uint8_t                 receive_buffer[sizeof(zpal_radio_receive_frame_t) + 255]; // Let's allocate 255 frame content payload for received frames for testing.
static zpal_radio_receive_frame_t *  mp_zpal_received_frame = (zpal_radio_receive_frame_t*) receive_buffer;
static ZW_ReceiveFrame_t mTestFilter0ReceivedFrame;
static ZW_ReceiveFrame_t mTestFilter1ReceivedFrame;
static ZW_ReceiveFrame_t mTestFilter2ReceivedFrame;
static ZW_ReceiveFrame_t mTestFilter3ReceivedFrame;
static ZW_ReceiveFrame_t mTestFilter4ReceivedFrame;
static ZW_ReceiveFrame_t mTestFilter5ReceivedFrame;
static ZW_ReceiveFrame_t mTestFilter6ReceivedFrame;

void setUp(void)
{
  //This is run before EACH TEST
  mock_calls_clear();
  mTestFilter0Called = false;
  mTestFilter1Called = false;
  mTestFilter2Called = false;
  mTestFilter3Called = false;
  mTestFilter4Called = false;
  mTestFilter5Called = false;
  mTestFilter6Called = false;
}

void tearDown(void)
{
  mock_calls_verify();
}

void helper_func_expect_radio_init_jp(zpal_radio_profile_t *pRfProfile, mock_t *p_mock)
{
  // Setup of test expectations. When configuration the Data Link Layer, the following calls are expected further down to Phy Layer.
  // 1) zpal_radio_init, with configuration of 3 channels for 9.6k, 40k, 100k. And a Energy Harvesting channel (still to be defined)
  mock_call_expect(TO_STR(zpal_radio_init), &p_mock);
  p_mock->expect_arg[0].pointer = pRfProfile;
  p_mock->compare_rule_arg[1]   = COMPARE_NULL;
  p_mock->compare_rule_arg[2]   = COMPARE_NULL;
  p_mock->compare_rule_arg[3]   = COMPARE_NULL;
}

void testFilter0Callback(ZW_ReceiveFrame_t * pFrame)
{
  mTestFilter0Called = true;
  mTestFilter0ReceivedFrame = *pFrame;
}

void testFilter1Callback(ZW_ReceiveFrame_t * pFrame)
{
  mTestFilter1Called = true;
  mTestFilter1ReceivedFrame = *pFrame;
}

void testFilter2Callback(ZW_ReceiveFrame_t * pFrame)
{
  mTestFilter2Called = true;
  mTestFilter2ReceivedFrame = *pFrame;
}

void testFilter3Callback(ZW_ReceiveFrame_t * pFrame)
{
  mTestFilter3Called = true;
  mTestFilter3ReceivedFrame = *pFrame;
}

void testFilter4Callback(ZW_ReceiveFrame_t * pFrame)
{
  mTestFilter4Called = true;
  mTestFilter4ReceivedFrame = *pFrame;
}

void testFilter5Callback(ZW_ReceiveFrame_t * pFrame)
{
  mTestFilter5Called = true;
  mTestFilter5ReceivedFrame = *pFrame;
}

void testFilter6Callback(ZW_ReceiveFrame_t * pFrame)
{
  mTestFilter6Called = true;
  mTestFilter6ReceivedFrame = *pFrame;
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
void test_receive_payload_filter_frames(void)
{
  mock_t* p_mock = NULL;
  zpal_radio_profile_t radioProfile = { REGION_JP, ZPAL_RADIO_WAKEUP_ALWAYS_LISTEN };
  /** As correct radio initialization is ensured in other tests, we just utilize the helper function in this test. */
  helper_func_expect_radio_init_jp(&radioProfile, p_mock);
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
  actualVal = llInit(&radioProfile);
  TEST_ASSERT_EQUAL(SUCCESS, actualVal);

  ZW_ReceiveFilter_t rxFilter = {.payloadIndex1 = 0, .payloadFilterValue1 = 0x01, // CC Vers. 1
                                 .payloadIndex2 = 1, .payloadFilterValue2 = 0x01, // NIF
                                 .headerType = 0x01,                              // Single Cast
                                 .flag = PAYLOAD_INDEX_1_FILTER_FLAG | PAYLOAD_INDEX_1_FILTER_FLAG,
                                 .frameHandler = testFilter1Callback};

  TEST_ASSERT_FALSE(mTestFilter1Called);
  mp_zpal_received_frame->frame_content_length = sizeof(nifFrame);
  memcpy(mp_zpal_received_frame->frame_content, nifFrame, mp_zpal_received_frame->frame_content_length);
  mock_calls_clear();
    radioProfile.receive_handler_cb(&rxParameters, mp_zpal_received_frame);
  mock_calls_verify();
  // No filter defined, thus frame should be discarded.
  TEST_ASSERT_FALSE(mTestFilter1Called);

  actualVal = llReceiveFilterAdd(&rxFilter);
  TEST_ASSERT_EQUAL(SUCCESS, actualVal);

  mock_calls_clear();
    radioProfile.receive_handler_cb(&rxParameters, mp_zpal_received_frame);
  TEST_ASSERT_TRUE(mTestFilter1Called);
  mock_calls_verify();

  TEST_ASSERT_EQUAL(33, mTestFilter1ReceivedFrame.frameContentLength);
  TEST_ASSERT_EQUAL(0, memcmp(&mTestFilter1ReceivedFrame.frameContent[10], &nifFrame[10], mTestFilter1ReceivedFrame.frameContentLength - 10));
}

/** Verification that the Data Link Layer can filter frames based upon HOMEID, empty Payload filter.
 *
 */
void test_receive_homeid_filter_frames(void)
{
  mock_t* p_mock = NULL;
  zpal_radio_profile_t radioProfile = { REGION_JP, ZPAL_RADIO_WAKEUP_ALWAYS_LISTEN };
  /** As correct radio initialization is ensured in other tests, we just utilize the helper function in this test. */
  helper_func_expect_radio_init_jp(&radioProfile, p_mock);

  zpal_radio_rx_parameters_t rxParameters = {
    .speed = ZPAL_RADIO_SPEED_100K,
    .channel_id = 1,
    .channel_header_format = ZPAL_RADIO_HEADER_TYPE_3CH,
    .rssi = 0
  };

  uint8_t          infoGetFrameMatching[] = {0xca, 0xfe, 0xba, 0xbf, 0x01, 0x51, 0x0a, 0x0c, 0x02,
                                             0x5e, 0x01,
                                             0xc5};

  uint8_t          infoGetFrameNotMatching[] = {0xca, 0xfe, 0xba, 0xbe, 0x01, 0x51, 0x0a, 0x0c, 0x02,
                                                0x5e, 0x01,
                                                0xc5};

  ZW_ReturnCode_t actualVal;

  actualVal = llInit(&radioProfile);
  TEST_ASSERT_EQUAL(SUCCESS, actualVal);

  ZW_ReceiveFilter_t rxFilter = {.homeId = {{0xCA, 0xFE, 0xBA, 0xBF}},
                                 .payloadIndex1 = 0xFF, .payloadFilterValue1 = 0,
                                 .payloadIndex2 = 0xFF, .payloadFilterValue2 = 0,
                                 .headerType = 0x01,                                 // Single Cast
                                 .flag = HOMEID_FILTER_FLAG,
                                 .frameHandler = testFilter1Callback};

  actualVal = llReceiveFilterAdd(&rxFilter);
  TEST_ASSERT_EQUAL(SUCCESS, actualVal);

  mock_calls_clear();
    radioProfile.receive_handler_cb(&rxParameters, mp_zpal_received_frame);
  mock_calls_verify();

  memcpy(mp_zpal_received_frame->frame_content, infoGetFrameNotMatching, mp_zpal_received_frame->frame_content_length);

  mock_calls_clear();
    radioProfile.receive_handler_cb(&rxParameters, mp_zpal_received_frame);
  // Frame does not match the home id, thus it is expected to be discarded.
  TEST_ASSERT_FALSE(mTestFilter1Called);
  mock_calls_verify();

  mp_zpal_received_frame->frame_content_length = sizeof(infoGetFrameMatching);
  memcpy(mp_zpal_received_frame->frame_content, infoGetFrameMatching, mp_zpal_received_frame->frame_content_length);
  mock_calls_clear();
//  mock_call_expect(TO_STR(zpal_radio_get_protocol_mode),&pMock);
//  pMock->return_code.value = ZPAL_RADIO_PROTOCOL_MODE_2;
    radioProfile.receive_handler_cb(&rxParameters, mp_zpal_received_frame);
  mock_calls_verify();
  TEST_ASSERT_TRUE(mTestFilter1Called);
  TEST_ASSERT_EQUAL(12, mTestFilter1ReceivedFrame.frameContentLength);
  TEST_ASSERT_EQUAL(0, memcmp(&mTestFilter1ReceivedFrame.frameContent[9], &infoGetFrameMatching[9], mTestFilter1ReceivedFrame.frameContentLength - 9));
}

/** Verification that the Data Link Layer can filter frames based upon HOMEID, empty Payload filter.
 *
 */
void test_receive_add_remove_filters(void)
{
  mock_t* p_mock = NULL;
  zpal_radio_profile_t radioProfile = { REGION_JP, ZPAL_RADIO_WAKEUP_ALWAYS_LISTEN };
  /** As correct radio initialization is ensured in other tests, we just utilize the helper function in this test. */
  helper_func_expect_radio_init_jp(&radioProfile, p_mock);

  zpal_radio_rx_parameters_t rxParameters = {
    .speed = ZPAL_RADIO_SPEED_100K,
    .channel_id = 1,
    .channel_header_format = ZPAL_RADIO_HEADER_TYPE_3CH,
    .rssi = 0
  };

  uint8_t          infoGetFrameMatching[] = {0xDC, 0x67, 0xCD, 0x37, 0x01, 0x81, 0x00, 0x0E, 0x2B, 0x02,
                                             0x5e, 0x01,
                                             0x82, 0x4A};

  ZW_ReturnCode_t actualVal;

  actualVal = llInit(&radioProfile);
  TEST_ASSERT_EQUAL(SUCCESS, actualVal);

  ZW_ReceiveFilter_t rxFilter0 = {.homeId = {{0xDC, 0x67, 0xCD, 0x37}},
                                  .payloadIndex1 = 0x00, .payloadFilterValue1 = 0x5e,
                                  .payloadIndex2 = 0x01, .payloadFilterValue2 = 0x01,
                                  .headerType = 0x01,                                 // Single Cast
                                  .flag = HOMEID_FILTER_FLAG | PAYLOAD_INDEX_1_FILTER_FLAG | PAYLOAD_INDEX_2_FILTER_FLAG,
                                  .frameHandler = testFilter0Callback};

  ZW_ReceiveFilter_t rxFilter1 = {.homeId = {{0xDC, 0x67, 0xCD, 0x37}},
                                  .destinationNodeId = 2,
                                  .headerType = 0x01,                                 // Single Cast
                                  .flag = HOMEID_FILTER_FLAG | DESTINATION_NODE_ID_FILTER_FLAG,
                                  .frameHandler = testFilter1Callback};

  ZW_ReceiveFilter_t rxFilter2 = {.homeId = {{0xDC, 0x67, 0xCD, 0x37}},
                                  .payloadIndex1 = 0x00, .payloadFilterValue1 = 5,
                                  .payloadIndex2 = 0x01, .payloadFilterValue2 = 6,
                                  .headerType = 0x01,                                 // Single Cast
                                  .flag = HOMEID_FILTER_FLAG | PAYLOAD_INDEX_1_FILTER_FLAG | PAYLOAD_INDEX_2_FILTER_FLAG,
                                  .frameHandler = testFilter2Callback};

  ZW_ReceiveFilter_t rxFilter3 = {.homeId = {{0xDC, 0x67, 0xCD, 0x37}},
                                  .payloadIndex1 = 0x00, .payloadFilterValue1 = 7,
                                  .payloadIndex2 = 0x01, .payloadFilterValue2 = 8,
                                  .headerType = 0x01,                                 // Single Cast
                                  .flag = HOMEID_FILTER_FLAG | PAYLOAD_INDEX_1_FILTER_FLAG | PAYLOAD_INDEX_2_FILTER_FLAG,
                                  .frameHandler = testFilter3Callback};

  ZW_ReceiveFilter_t rxFilter4 = {.homeId = {{0xDC, 0x67, 0xCD, 0x37}},
                                  .payloadIndex1 = 0x00, .payloadFilterValue1 = 9,
                                  .payloadIndex2 = 0x01, .payloadFilterValue2 = 10,
                                  .headerType = 0x01,                                 // Single Cast
                                  .flag = HOMEID_FILTER_FLAG | PAYLOAD_INDEX_1_FILTER_FLAG | PAYLOAD_INDEX_2_FILTER_FLAG,
                                  .frameHandler = testFilter4Callback};

  ZW_ReceiveFilter_t rxFilter5 = {.homeId = {{0xDC, 0x67, 0xCD, 0x37}},
                                  .payloadIndex1 = 0x00, .payloadFilterValue1 = 11,
                                  .payloadIndex2 = 0x01, .payloadFilterValue2 = 12,
                                  .headerType = 0x01,                                 // Single Cast
                                  .flag = HOMEID_FILTER_FLAG | PAYLOAD_INDEX_1_FILTER_FLAG | PAYLOAD_INDEX_2_FILTER_FLAG,
                                  .frameHandler = testFilter5Callback};

  mock_calls_clear();
  mp_zpal_received_frame->frame_content_length = sizeof(infoGetFrameMatching);
  memcpy(mp_zpal_received_frame->frame_content, infoGetFrameMatching, mp_zpal_received_frame->frame_content_length);
    radioProfile.receive_handler_cb(&rxParameters, mp_zpal_received_frame);
  TEST_ASSERT_FALSE(mTestFilter1Called);
  mock_calls_verify();

  actualVal = llReceiveFilterAdd(&rxFilter4);
  TEST_ASSERT_EQUAL(SUCCESS, actualVal);

  actualVal = llReceiveFilterAdd(&rxFilter2);
  TEST_ASSERT_EQUAL(SUCCESS, actualVal);

  actualVal = llReceiveFilterAdd(&rxFilter0);
  TEST_ASSERT_EQUAL(SUCCESS, actualVal);

  actualVal = llReceiveFilterAdd(&rxFilter3);
  TEST_ASSERT_EQUAL(SUCCESS, actualVal);

  actualVal = llReceiveFilterAdd(&rxFilter1);
  TEST_ASSERT_EQUAL(SUCCESS, actualVal);

  actualVal = llReceiveFilterAdd(&rxFilter5);
  TEST_ASSERT_EQUAL(NO_MEMORY, actualVal);

  mock_calls_clear();
    radioProfile.receive_handler_cb(&rxParameters, mp_zpal_received_frame);
  mock_calls_verify();

  TEST_ASSERT_TRUE(mTestFilter1Called);
  TEST_ASSERT_EQUAL(14, mTestFilter1ReceivedFrame.frameContentLength);
  TEST_ASSERT_EQUAL(0, memcmp(&mTestFilter1ReceivedFrame.frameContent[10], &infoGetFrameMatching[10], mTestFilter1ReceivedFrame.frameContentLength - 10));

  mTestFilter1Called = false;

  // Remove two filters to ensure room for adding filters again.
  actualVal = llReceiveFilterRemove(&rxFilter3);
  TEST_ASSERT_EQUAL(SUCCESS, actualVal);

  actualVal = llReceiveFilterRemove(&rxFilter1);
  TEST_ASSERT_EQUAL(SUCCESS, actualVal);

  actualVal = llReceiveFilterAdd(&rxFilter5);
  TEST_ASSERT_EQUAL(SUCCESS, actualVal);

  mock_calls_clear();
    radioProfile.receive_handler_cb(&rxParameters, mp_zpal_received_frame);
  TEST_ASSERT_FALSE(mTestFilter1Called);
  mock_calls_verify();
  // Re-add the main matching filter and check that frames are correctly received.
  actualVal = llReceiveFilterAdd(&rxFilter1);
  TEST_ASSERT_EQUAL(SUCCESS, actualVal);

  mock_calls_clear();
    radioProfile.receive_handler_cb(&rxParameters, mp_zpal_received_frame);
  TEST_ASSERT_TRUE(mTestFilter1Called);
  TEST_ASSERT_EQUAL(14, mTestFilter1ReceivedFrame.frameContentLength);
  TEST_ASSERT_EQUAL(0, memcmp(&mTestFilter1ReceivedFrame.frameContent[10], &infoGetFrameMatching[10], mTestFilter1ReceivedFrame.frameContentLength - 10));
  mock_calls_verify();
}

/** Verification that the Data Link Layer can filter a frame and discard based on each flag type.
 *
 */
void test_receive_singlecast_filtering_flags(void)
{
  mock_t* p_mock = NULL;
  zpal_radio_profile_t radioProfile = { REGION_JP, ZPAL_RADIO_WAKEUP_ALWAYS_LISTEN };
  /** As correct radio initialization is ensured in other tests, we just utilize the helper function in this test. */
  helper_func_expect_radio_init_jp(&radioProfile, p_mock);

  zpal_radio_rx_parameters_t rxParameters = {
    .speed = ZPAL_RADIO_SPEED_100K,
    .channel_id = 1,
    .channel_header_format = ZPAL_RADIO_HEADER_TYPE_3CH,
    .rssi = 0
  };

  uint8_t          infoGetFrameMatching[] = {0xDC, 0x67, 0xCD, 0x37, 0x01, 0x81, 0x00, 0x0E, 0x2B, 0x02,
                                             0x5e, 0x01,
                                             0x82, 0x4A};
  ZW_ReturnCode_t actualVal;

  actualVal = llInit(&radioProfile);
  TEST_ASSERT_EQUAL(SUCCESS, actualVal);

  ZW_ReceiveFilter_t rxFilter5 = {.headerType = 0x01,                                 // Single Cast
                                  .flag = 0,
                                  .frameHandler = testFilter5Callback};

  ZW_ReceiveFilter_t rxFilter0 = {.homeId = {{0xCA, 0xFE, 0xBA, 0xB3}},
                                  .headerType = 0x01,                                 // Single Cast
                                  .flag = HOMEID_FILTER_FLAG,
                                  .frameHandler = testFilter0Callback};

  ZW_ReceiveFilter_t rxFilter1 = {.homeId = {{0xDC, 0x67, 0xCD, 0x37}},
                                  .destinationNodeId = 5,
                                  .headerType = 0x01,                                 // Single Cast
                                  .flag = HOMEID_FILTER_FLAG | DESTINATION_NODE_ID_FILTER_FLAG | SOURCE_NODE_ID_FILTER_FLAG,
                                  .frameHandler = testFilter1Callback};

  ZW_ReceiveFilter_t rxFilter2 = {.homeId = {{0xDC, 0x67, 0xCD, 0x37}},
                                  .destinationNodeId = 2,
                                  .sourceNodeId = 5,
                                  .headerType = 0x01,                                 // Single Cast
                                  .flag = HOMEID_FILTER_FLAG | DESTINATION_NODE_ID_FILTER_FLAG | SOURCE_NODE_ID_FILTER_FLAG,
                                  .frameHandler = testFilter2Callback};

  ZW_ReceiveFilter_t rxFilter3 = {.homeId = {{0xDC, 0x67, 0xCD, 0x37}},
                                  .destinationNodeId = 2,
                                  .sourceNodeId = 1,
                                  .payloadIndex1 = 0, .payloadFilterValue1 = 7,
                                  .headerType = 0x01,                                 // Single Cast
                                  .flag = HOMEID_FILTER_FLAG | DESTINATION_NODE_ID_FILTER_FLAG | SOURCE_NODE_ID_FILTER_FLAG | PAYLOAD_INDEX_1_FILTER_FLAG,
                                  .frameHandler = testFilter3Callback};

  ZW_ReceiveFilter_t rxFilter4 = {.homeId = {{0xDC, 0x67, 0xCD, 0x37}},
                                  .destinationNodeId = 2,
                                  .sourceNodeId = 1,
                                  .payloadIndex1 = 0, .payloadFilterValue1 = 0x5e,
                                  .payloadIndex2 = 1, .payloadFilterValue2 = 8,
                                  .headerType = 0x01,                                 // Single Cast
                                  .flag = HOMEID_FILTER_FLAG | DESTINATION_NODE_ID_FILTER_FLAG | SOURCE_NODE_ID_FILTER_FLAG | PAYLOAD_INDEX_1_FILTER_FLAG | PAYLOAD_INDEX_2_FILTER_FLAG,
                                  .frameHandler = testFilter4Callback};


  mp_zpal_received_frame->frame_content_length = sizeof(infoGetFrameMatching);
  memcpy(mp_zpal_received_frame->frame_content, infoGetFrameMatching, mp_zpal_received_frame->frame_content_length);
  mock_calls_clear();
    radioProfile.receive_handler_cb(&rxParameters, mp_zpal_received_frame);
  TEST_ASSERT_FALSE(mTestFilter0Called | mTestFilter1Called | mTestFilter2Called | mTestFilter3Called | mTestFilter4Called | mTestFilter5Called);
  mock_calls_verify();
  // Start adding the gloabal explorer filter
  actualVal = llReceiveFilterAdd(&rxFilter5);
  TEST_ASSERT_EQUAL(SUCCESS, actualVal);

  // Home ID only (No match)
  actualVal = llReceiveFilterAdd(&rxFilter0);
  TEST_ASSERT_EQUAL(SUCCESS, actualVal);

  mock_calls_clear();
    radioProfile.receive_handler_cb(&rxParameters, mp_zpal_received_frame);
  TEST_ASSERT_FALSE(mTestFilter0Called | mTestFilter1Called | mTestFilter2Called | mTestFilter3Called | mTestFilter4Called);
  TEST_ASSERT_TRUE(mTestFilter5Called);
  mTestFilter5Called = false;
  mock_calls_verify();

  // Home ID and dest (No match)
  actualVal = llReceiveFilterAdd(&rxFilter1);
  TEST_ASSERT_EQUAL(SUCCESS, actualVal);

  mock_calls_clear();
    radioProfile.receive_handler_cb(&rxParameters, mp_zpal_received_frame);
  TEST_ASSERT_FALSE(mTestFilter0Called | mTestFilter1Called | mTestFilter2Called | mTestFilter3Called | mTestFilter4Called);
  TEST_ASSERT_TRUE(mTestFilter5Called);
  mTestFilter5Called = false;
  mock_calls_verify();

  // Home ID, source, and dest (No match)
  actualVal = llReceiveFilterAdd(&rxFilter2);
  TEST_ASSERT_EQUAL(SUCCESS, actualVal);

  mock_calls_clear();
    radioProfile.receive_handler_cb(&rxParameters, mp_zpal_received_frame);
  TEST_ASSERT_FALSE(mTestFilter0Called | mTestFilter1Called | mTestFilter2Called | mTestFilter3Called | mTestFilter4Called);
  TEST_ASSERT_TRUE(mTestFilter5Called);
  mTestFilter5Called = false;
  mock_calls_verify();

  // Home ID, source, dest, payload 1 (No match)
  actualVal = llReceiveFilterAdd(&rxFilter3);
  TEST_ASSERT_EQUAL(SUCCESS, actualVal);

  mock_calls_clear();
    radioProfile.receive_handler_cb(&rxParameters, mp_zpal_received_frame);
  TEST_ASSERT_FALSE(mTestFilter0Called | mTestFilter1Called | mTestFilter2Called | mTestFilter3Called | mTestFilter4Called);
  TEST_ASSERT_TRUE(mTestFilter5Called);
  mTestFilter5Called = false;
  mock_calls_verify();

  // Remove a single filter to make room for next in list.
  actualVal = llReceiveFilterRemove(&rxFilter0);
  TEST_ASSERT_EQUAL(SUCCESS, actualVal);

  // Home ID, source, dest, payload 1, payload 2 (No match)
  actualVal = llReceiveFilterAdd(&rxFilter4);
  TEST_ASSERT_EQUAL(SUCCESS, actualVal);

  mock_calls_clear();
    radioProfile.receive_handler_cb(&rxParameters, mp_zpal_received_frame);
  TEST_ASSERT_FALSE(mTestFilter0Called | mTestFilter1Called | mTestFilter2Called | mTestFilter3Called | mTestFilter4Called);
  TEST_ASSERT_TRUE(mTestFilter5Called);
  mTestFilter5Called = false;
  mock_calls_verify();

  TEST_ASSERT_EQUAL(14, mTestFilter5ReceivedFrame.frameContentLength);
  TEST_ASSERT_EQUAL(0, memcmp(&mTestFilter5ReceivedFrame.frameContent[9], &infoGetFrameMatching[9], mTestFilter5ReceivedFrame.frameContentLength - 9));
}

/*
 * This test verifies that a filter with payload matching on a high index, e.g. 50, will not trigger a match if payload is <50.
 */
void test_receive_singlecast_out_of_bound_filters(void)
{
  mock_t* p_mock = NULL;
  zpal_radio_profile_t radioProfile = { REGION_JP, ZPAL_RADIO_WAKEUP_ALWAYS_LISTEN };
  /** As correct radio initialization is ensured in other tests, we just utilize the helper function in this test. */
  helper_func_expect_radio_init_jp(&radioProfile, p_mock);

  zpal_radio_rx_parameters_t rxParameters = {
    .speed = ZPAL_RADIO_SPEED_100K,
    .channel_id = 1,
    .channel_header_format = ZPAL_RADIO_HEADER_TYPE_3CH,
    .rssi = 0
  };

  uint8_t          infoGetFrameMatching[] = {0xDC, 0x67, 0xCD, 0x37, 0x01, 0x81, 0x00, 0x0E, 0x2B, 0x02,
                                             0x5e, 0x01,
                                             0x82, 0x4A};

  ZW_ReturnCode_t actualVal;
  actualVal = llInit(&radioProfile);
  TEST_ASSERT_EQUAL(SUCCESS, actualVal);

  ZW_ReceiveFilter_t rxFilter0 = {.homeId = {{0xDC, 0x67, 0xCD, 0x37}},
                                  .payloadIndex1 = 50, .payloadFilterValue1 = 0x55,
                                  .payloadIndex2 = 51, .payloadFilterValue2 = 0x55,
                                  .headerType = 0x01,                                 // Single Cast
                                  .flag = HOMEID_FILTER_FLAG | PAYLOAD_INDEX_1_FILTER_FLAG | PAYLOAD_INDEX_2_FILTER_FLAG,
                                  .frameHandler = testFilter0Callback};

  memset(receive_buffer, 0x55, sizeof(receive_buffer)); // Set all buffer content to 0x55, to trigger payload filter match if compared at out of bounds.
  mp_zpal_received_frame->frame_content_length = sizeof(infoGetFrameMatching);
  memcpy(mp_zpal_received_frame->frame_content, infoGetFrameMatching, mp_zpal_received_frame->frame_content_length);
  mock_calls_clear();
    radioProfile.receive_handler_cb(&rxParameters, mp_zpal_received_frame);
  TEST_ASSERT_FALSE(mTestFilter0Called);
  mock_calls_verify();
  // Start adding the explorer filter with payload filter
  actualVal = llReceiveFilterAdd(&rxFilter0);
  TEST_ASSERT_EQUAL(SUCCESS, actualVal);

  mock_calls_clear();
  // Receive the frame, the filter should not be triggered, as the payload filter index is out of bounds.
  TEST_ASSERT_EQUAL(0x55, mp_zpal_received_frame->frame_content[59]); // Check the frame, does indeed contain the filtered data in the out of bounds memory section. (9 is added to index internally, as that is basic header length)
    radioProfile.receive_handler_cb(&rxParameters, mp_zpal_received_frame);
  TEST_ASSERT_FALSE(mTestFilter0Called);
  mock_calls_verify();
}

///** Verification that the Data Link Layer can filter frames based upon HOMEID, and not self destination id (probably a frame to repeat)
// *
// */
//void test_receive_homeid_other_nodeid_filter_frames(void)
//{
//  /** As correct radio initialization is ensured in other tests, we just utilize the helper function in this test. */
//  helper_func_expect_radio_init_jp();
//
//  zpal_radio_rx_parameters_t rxParameters = {.speed = ZPAL_RADIO_SPEED_100K,// Speed 9.6K=1000, Speed 40K=0100, Speed 100K=0010.
//                                   .channelId = 1,
//                                   .rssi = 0};
//
//  uint8_t explorerVersionGetMatching[] = {0xca, 0xfe, 0xba, 0xbf, 0x01, 0x45, 0x03, 0x15, 0x03, // HomeId, Src, Flags1, Flags2, Length, Dst,
//                                          0xd1, 0x50, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00,       // Explorer Header: Explorer Command, TxRandom, TTL, Repeaters, Network Home Id,
//                                          0x86, 0x13,                                           // CC vers, NIF, Capabilities, Security, Properties, CC supported,
//                                          0x20, 0x8b};
//
//  uint8_t explorerVersionGetRepeatedMatching[] = {0xca, 0xfe, 0xba, 0xbf, 0x01, 0x45, 0x00, 0x15, 0x03, // HomeId, Src, Flags1, Flags2, Length, Dst,
//                                                  0xd1, 0x50, 0x00, 0x11, 0x02, 0x00, 0x00, 0x00,       // Explorer Header: Explorer Command, TxRandom, TTL, Repeaters, Network Home Id,
//                                                  0x86, 0x13,                                           // CC vers, NIF, Capabilities, Security, Properties, CC supported,
//                                                  0x20, 0xbb};
//
//  ZW_ReturnCode_t radioInitReturnValueExpected = SUCCESS;
//  ZW_ReturnCode_t actualVal;
//
//  zpal_radio_profile_t radioProfile = { REGION_JP, ZPAL_RADIO_WAKEUP_ALWAYS_LISTEN };
//  actualVal = llInit(&radioProfile);
//  TEST_ASSERT_EQUAL(radioInitReturnValueExpected, actualVal);
//
//  ZW_ReceiveFilter_t rxNodeId02Filter = {.homeId = {{0xCA, 0xFE, 0xBA, 0xBF}},
//                                         .destinationNodeId = 0x02,
//                                         .headerType = 0x05,                                 // Explorer frame
//                                         .flag = HOMEID_FILTER_FLAG | DESTINATION_NODE_ID_FILTER_FLAG,
//                                         .frameHandler = testFilter1Callback};
//
//  ZW_ReceiveFilter_t rxNodeId03Filter = {.homeId = {{0xCA, 0xFE, 0xBA, 0xBF}},
//                                         .destinationNodeId = 0x03,
//                                         .headerType = 0x05,                                 // Explorer frame
//                                         .flag = HOMEID_FILTER_FLAG | DESTINATION_NODE_ID_FILTER_FLAG,
//                                         .frameHandler = testFilter1Callback};
//
//  ZW_ReceiveFilter_t rxNoNodeIdFilter = {.homeId = {{0xCA, 0xFE, 0xBA, 0xBF}},
//                                         .headerType = 0x05,                                 // Explorer frame
//                                         .flag = HOMEID_FILTER_FLAG,
//                                         .frameHandler = testFilter2Callback};
//
//  ZW_ReceiveFilter_t rxOtherHomeIdFilter = {.homeId = {{0xBA, 0xBB, 0xBC, 0xBD}},
//                                         .headerType = 0x05,                                 // Explorer frame
//                                         .flag = HOMEID_FILTER_FLAG,
//                                         .frameHandler = testFilter1Callback};
//
//  mp_zpal_received_frame->frameContentLength = sizeof(explorerVersionGetMatching);
//  memcpy(mp_zpal_received_frame->frameContent, explorerVersionGetMatching, mp_zpal_received_frame->frameContentLength);
//    radioProfile.receive_handler_cb(&rxParameters, mp_zpal_received_frame);
//  // No filter is set, thus frame is expected to be discarded.
//  TEST_ASSERT_FALSE(mTestFilter1Called);
//
//  // Add home id and my node id destination filter.
//  actualVal = llReceiveFilterAdd(&rxNodeId03Filter);
//  TEST_ASSERT_EQUAL(SUCCESS, actualVal);
//
//  mp_zpal_received_frame->frameContentLength = sizeof(explorerVersionGetMatching);
//  memcpy(mp_zpal_received_frame->frameContent, explorerVersionGetMatching, mp_zpal_received_frame->frameContentLength);
//      radioProfile.receive_handler_cb(&rxParameters, mp_zpal_received_frame);
//  // Frame match the home id and node, thus it is expected to be processed by filter1.
//  TEST_ASSERT_TRUE(mTestFilter1Called);
//  TEST_ASSERT_FALSE(mTestFilter2Called);
//  mTestFilter1Called = false;
//
//#ifdef BACKWARD_COMPABILITY_TEST
//  TEST_ASSERT_EQUAL(21, mTestFilter1ReceivedFrame.frameContentLength);
//  TEST_ASSERT_EQUAL(0, memcmp(&mTestFilter1ReceivedFrame.frameContent[9], &explorerVersionGetMatching[9], mTestFilter1ReceivedFrame.frameContentLength - 9));
//#else  // BACKWARD_COMPABILITY_TEST
//  TEST_ASSERT_EQUAL(11, mTestFilter1ReceivedFrame.frameContentLength);
//  TEST_ASSERT_EQUAL(0, memcmp(&mTestFilter1ReceivedFrame.frameContent, &explorerVersionGetMatching[9], mTestFilter1ReceivedFrame.frameContentLength));
//#endif // BACKWARD_COMPABILITY_TEST
//
//  // Add home id only filter - The filter with home and node id should be matched by frame explorerVersionGetMatching should still be called.
//  actualVal = llReceiveFilterAdd(&rxNoNodeIdFilter);
//  TEST_ASSERT_EQUAL(SUCCESS, actualVal);
//
//  mp_zpal_received_frame->frameContentLength = sizeof(explorerVersionGetMatching);
//  memcpy(mp_zpal_received_frame->frameContent, explorerVersionGetMatching, mp_zpal_received_frame->frameContentLength);
//      radioProfile.receive_handler_cb(&rxParameters, mp_zpal_received_frame);
//  TEST_ASSERT_TRUE(mTestFilter1Called);
//  TEST_ASSERT_FALSE(mTestFilter2Called);
//  mTestFilter1Called = false;
//
//  // Remove the node id filter. (This should result in the general home id filter being matched)
//  actualVal = llReceiveFilterRemove(&rxNodeId03Filter);
//  TEST_ASSERT_EQUAL(SUCCESS, actualVal);
//
//  mp_zpal_received_frame->frameContentLength = sizeof(explorerVersionGetMatching);
//  memcpy(mp_zpal_received_frame->frameContent, explorerVersionGetMatching, mp_zpal_received_frame->frameContentLength);
//      radioProfile.receive_handler_cb(&rxParameters, mp_zpal_received_frame);
//  TEST_ASSERT_FALSE(mTestFilter1Called);
//  TEST_ASSERT_TRUE(mTestFilter2Called);
//  mTestFilter2Called = false;
//
//#ifdef BACKWARD_COMPABILITY_TEST
//  TEST_ASSERT_EQUAL(21, mTestFilter2ReceivedFrame.frameContentLength);
//  TEST_ASSERT_EQUAL(0, memcmp(&mTestFilter2ReceivedFrame.frameContent[9], &explorerVersionGetMatching[9], mTestFilter2ReceivedFrame.frameContentLength - 9));
//#else  // BACKWARD_COMPABILITY_TEST
//  TEST_ASSERT_EQUAL(11, mTestFilter2ReceivedFrame.frameContentLength);
//  TEST_ASSERT_EQUAL(0, memcmp(&mTestFilter2ReceivedFrame.frameContent, &explorerVersionGetMatching[9], mTestFilter2ReceivedFrame.frameContentLength));
//#endif // BACKWARD_COMPABILITY_TEST
//
//  // Re-Add home id and my node id destination filter.
//  actualVal = llReceiveFilterAdd(&rxNodeId03Filter);
//  TEST_ASSERT_EQUAL(SUCCESS, actualVal);
//
//  mp_zpal_received_frame->frameContentLength = sizeof(explorerVersionGetMatching);
//  memcpy(mp_zpal_received_frame->frameContent, explorerVersionGetMatching, mp_zpal_received_frame->frameContentLength);
//      radioProfile.receive_handler_cb(&rxParameters, mp_zpal_received_frame);
//  // Frame match the home id and node, thus it is expected to be processed by filter1.
//  TEST_ASSERT_TRUE(mTestFilter1Called);
//  TEST_ASSERT_FALSE(mTestFilter2Called);
//  mTestFilter1Called = false;
//
//#ifdef BACKWARD_COMPABILITY_TEST
//  TEST_ASSERT_EQUAL(21, mTestFilter1ReceivedFrame.frameContentLength);
//  TEST_ASSERT_EQUAL(0, memcmp(&mTestFilter1ReceivedFrame.frameContent[9], &explorerVersionGetMatching[9], mTestFilter1ReceivedFrame.frameContentLength - 9));
//#else  // BACKWARD_COMPABILITY_TEST
//  TEST_ASSERT_EQUAL(11, mTestFilter1ReceivedFrame.frameContentLength);
//  TEST_ASSERT_EQUAL(0, memcmp(&mTestFilter1ReceivedFrame.frameContent, &explorerVersionGetMatching[9], mTestFilter1ReceivedFrame.frameContentLength));
//#endif // BACKWARD_COMPABILITY_TEST
//
//}
//
///** Verification that the Data Link Layer can filter frames based upon filter, with home id = FF FF FF FF, Explorer Command AutoInclusion.
// *
// */
//void test_receive_explorer_auto_inclusion_frames(void)
//{
//  /** As correct radio initialization is ensured in other tests, we just utilize the helper function in this test. */
//  helper_func_expect_radio_init_jp();
//  zpal_radio_rx_parameters_t rxParameters = {.speed = ZPAL_RADIO_SPEED_100K,// Speed 9.6K=1000, Speed 40K=0100, Speed 100K=0010.
//                                   .channelId = 1,
//                                   .rssi = 0};
//
//  uint8_t          explorerNifFrame[] = {0xFB, 0x68, 0x48, 0x1E, 0x00, 0x45, 0x08, 0x2B, 0xFF,                                                                   // HomeId, Src, Flags1, Flags2, Length, Dst,
//                                         0x21, 0x00, 0xFA, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,                                                 // Explorer Header: Explorer Command, TxRandom, TTL, Repeaters, Network Home Id,
//                                         0x01, 0x01, 0xD3, 0x9C, 0x01, 0x10, 0x01, 0x5E, 0x25, 0x85, 0x8E, 0x59, 0x55, 0x86, 0x72, 0x5A, 0x73, 0x98, 0x9F, 0x6C, // CC vers, NIF, Capabilities, Security, Properties, CC supported,
//                                         0x7A, 0xD7}; // Crc
//
//  uint8_t          explorerNifRepeatFrame[] = {0xFB, 0x68, 0x48, 0x1E, 0x00, 0x45, 0x08, 0x2B, 0xFF,
//                                               0x21, 0x00, 0xFA, 0x31, 0x04, 0x00, 0x00, 0x00, 0x00, 0x54, 0xA5, 0x00,
//                                               0x01, 0x01, 0xD3, 0x9C, 0x01, 0x10, 0x01, 0x5E, 0x25, 0x85, 0x8E, 0x59, 0x55, 0x86, 0x72, 0x5A, 0x73, 0x98, 0x9F, 0x6C,
//                                               0x7A, 0x53};
//
//  ZW_ReturnCode_t radioInitReturnValueExpected = SUCCESS;
//  ZW_ReturnCode_t actualVal;
//
//  zpal_radio_profile_t radioProfile = { REGION_JP, ZPAL_RADIO_WAKEUP_ALWAYS_LISTEN };
//  actualVal = llInit(&radioProfile);
//  TEST_ASSERT_EQUAL(radioInitReturnValueExpected, actualVal);
//
//  // General filter for the home id explorer frames.
//  ZW_ReceiveFilter_t rxFilter1 = {.homeId = {{0x00, 0x54, 0x45, 0x00}},
//                                  .flag = HOMEID_FILTER_FLAG,
//                                  .headerType = 0x05,                                 // Explorer Frame
//                                  .frameHandler = testFilter1Callback};
//
//  ZW_ReceiveFilter_t rxFilter2 = {.payloadIndex1 = 0,    .payloadFilterValue1 = 0x21, // Explorer Frame Version 1 (0x20), Command: Auto Inclusion: 0x01
//                                  .flag = PAYLOAD_INDEX_1_FILTER_FLAG,
//                                  .headerType = 0x05,                                 // Explorer Frame
//                                  .frameHandler = testFilter2Callback};
//
//  TEST_ASSERT_FALSE(mTestFilter1Called);
//  actualVal = llReceiveFilterAdd(&rxFilter1);
//  TEST_ASSERT_EQUAL(SUCCESS, actualVal);
//
//  mp_zpal_received_frame->frameContentLength = sizeof(explorerNifFrame);
//  memcpy(mp_zpal_received_frame->frameContent, explorerNifFrame, mp_zpal_received_frame->frameContentLength);
//      radioProfile.receive_handler_cb(&rxParameters, mp_zpal_received_frame);
//  // Explorer auto inclusion from foreign homeid, frame should be discarded.
//  TEST_ASSERT_FALSE(mTestFilter1Called);
//
//  // Add explorer autoinclusion filter (NWI mode enabled)
//  actualVal = llReceiveFilterAdd(&rxFilter2);
//  TEST_ASSERT_EQUAL(SUCCESS, actualVal);
//
//      radioProfile.receive_handler_cb(&rxParameters, mp_zpal_received_frame);
//  // Explorer auto inclusion from foreign homeid, frame should not be discarded as we have a filter.
//  // but should not reach the home id callback filter.
//  TEST_ASSERT_FALSE(mTestFilter1Called);
//  TEST_ASSERT_TRUE(mTestFilter2Called);
//
//  mTestFilter2Called = false;
//
//  /* Let's test a repeated frame. */
//  mp_zpal_received_frame->frameContentLength = sizeof(explorerNifRepeatFrame);
//  memcpy(mp_zpal_received_frame->frameContent, explorerNifRepeatFrame, mp_zpal_received_frame->frameContentLength);
//      radioProfile.receive_handler_cb(&rxParameters, mp_zpal_received_frame);
//  // Explorer auto inclusion from foreign homeid, frame should not be discarded as we have a filter.
//  // but should not reach the home id callback filter.
//  TEST_ASSERT_FALSE(mTestFilter1Called);
//  TEST_ASSERT_TRUE(mTestFilter2Called);
//
//#ifdef BACKWARD_COMPABILITY_TEST
//  TEST_ASSERT_EQUAL(43, mTestFilter2ReceivedFrame.frameContentLength);
//  TEST_ASSERT_EQUAL(0, memcmp(&mTestFilter2ReceivedFrame.frameContent[9], &explorerNifRepeatFrame[9], mTestFilter2ReceivedFrame.frameContentLength - 9));
//#else  // BACKWARD_COMPABILITY_TEST
//  TEST_ASSERT_EQUAL(33, mTestFilter1ReceivedFrame.frameContentLength);
//  TEST_ASSERT_EQUAL(0, memcmp(mTestFilter1ReceivedFrame.frameContent, &explorerNifFrame[9], mTestFilter1ReceivedFrame.frameContentLength));
//#endif // BACKWARD_COMPABILITY_TEST
//}

/** Verification that the Data Link Layer return the proper error if a filter not in the list is being removed.
 *
 */
void test_receive_remove_invalid_filter(void)
{
  mock_t* p_mock = NULL;
  zpal_radio_profile_t radioProfile = { REGION_JP, ZPAL_RADIO_WAKEUP_ALWAYS_LISTEN };
  /** As correct radio initialization is ensured in other tests, we just utilize the helper function in this test. */
  helper_func_expect_radio_init_jp(&radioProfile, p_mock);

  ZW_ReturnCode_t actualVal;

  actualVal = llInit(&radioProfile);
  TEST_ASSERT_EQUAL(SUCCESS, actualVal);

  ZW_ReceiveFilter_t rxFilterInvalid = {.homeId = {{0xCA, 0xFE, 0xBA, 0xBF}},
                                         .headerType = 0x01,                                 // Single Cast
                                         .flag = HOMEID_FILTER_FLAG,
                                         .frameHandler = testFilter0Callback};

  // Try to remove a filter that doesn't exists.
  actualVal = llReceiveFilterRemove(&rxFilterInvalid);
  TEST_ASSERT_EQUAL(INVALID_PARAMETERS, actualVal);
}

///** Verification that the Data Link Layer can filter frames based upon HOMEID, empty Payload filter.
// *
// */
//void test_receive_add_explorer_filters(void)
//{
//  /** As correct radio initialization is ensured in other tests, we just utilize the helper function in this test. */
//  helper_func_expect_radio_init_eu(0x06);
//
//  zpal_radio_rx_parameters_t rxParameters = {.speed = ZPAL_RADIO_SPEED_40K,// Speed 9.6K=1000, Speed 40K=0100, Speed 100K=0010.
//                                   .channel_id = 1,
//                                   .rssi = 0};
//
//  uint8_t          explorerNifFrame[] = {0xFB, 0x68, 0x48, 0x1E, 0x00, 0x45, 0x08, 0x2B, 0xFF,                                                                   // HomeId, Src, Flags1, Flags2, Length, Dst,
//                                         0x21, 0x00, 0xFA, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,                                                 // Explorer Header: Explorer Command, TxRandom, TTL, Repeaters, Network Home Id,
//                                         0x01, 0x01, 0xD3, 0x9C, 0x01, 0x10, 0x01, 0x5E, 0x25, 0x85, 0x8E, 0x59, 0x55, 0x86, 0x72, 0x5A, 0x73, 0x98, 0x9F, 0x6C, // CC vers, NIF, Capabilities, Security, Properties, CC supported,
//                                         0x7A, 0xD7}; // Crc
//
//  uint8_t          explorerFrameForeign[] = {0xE3, 0x26, 0xE5, 0x51, 0x01, 0x45, 0x03, 0x15, 0x02,       // HomeId, Src, Flags1, Flags2, Length, Dst,
//                                             0x20, 0x00, 0xFA, 0x40, 0x00, 0x00, 0x00, 0x00, 0x86, 0x13, // CC vers,
//                                             0x7A, 0xAB}; // Crc
//
//  ZW_ReturnCode_t radioInitReturnValueExpected = SUCCESS;
//  ZW_ReturnCode_t actualVal;
//
//  zpal_radio_profile_t radioProfile = { REGION_EU, ZPAL_RADIO_WAKEUP_ALWAYS_LISTEN };
//  actualVal = llInit(&radioProfile);
//  TEST_ASSERT_EQUAL(radioInitReturnValueExpected, actualVal);
//
//  ZW_ReceiveFilter_t rxFilter6 = {.headerType = 0x05,                                 // Explorer
//                                  .flag = 0,
//                                  .frameHandler = testFilter6Callback};
//
//  ZW_ReceiveFilter_t rxFilter0 = {.homeId = {{0xFB, 0x68, 0x48, 0x1E}},
//                                  .headerType = 0x05,                                 // Explorer
//                                  .flag = HOMEID_FILTER_FLAG,
//                                  .frameHandler = testFilter0Callback};
//
//  ZW_ReceiveFilter_t rxFilter1 = {.homeId = {{0xFB, 0x68, 0x48, 0x1E}},
//                                  .destinationNodeId = 2,
//                                  .headerType = 0x05,                                 // Explorer
//                                  .flag = HOMEID_FILTER_FLAG | DESTINATION_NODE_ID_FILTER_FLAG,
//                                  .frameHandler = testFilter1Callback};
//
//  ZW_ReceiveFilter_t rxFilter2 = {.homeId = {{0xFB, 0x68, 0x48, 0x1E}},
//                                  .sourceNodeId      = 0,
//                                  .destinationNodeId = 0xFF,
//                                  .headerType = 0x05,                                 // Explorer
//                                  .flag = HOMEID_FILTER_FLAG | SOURCE_NODE_ID_FILTER_FLAG | DESTINATION_NODE_ID_FILTER_FLAG,
//                                  .frameHandler = testFilter2Callback};
//
//  ZW_ReceiveFilter_t rxFilter3 = {.homeId = {{0xFB, 0x68, 0x48, 0x1E}},
//                                  .sourceNodeId      = 1,
//                                  .destinationNodeId = 2,
//                                  .payloadIndex1 = 0x00, .payloadFilterValue1 = 7,
//                                  .payloadIndex2 = 0x01, .payloadFilterValue2 = 8,
//                                  .headerType = 0x05,                                 // Explorer
//                                  .flag = HOMEID_FILTER_FLAG | SOURCE_NODE_ID_FILTER_FLAG | DESTINATION_NODE_ID_FILTER_FLAG | PAYLOAD_INDEX_1_FILTER_FLAG | PAYLOAD_INDEX_2_FILTER_FLAG,
//                                  .frameHandler = testFilter3Callback};
//
//  ZW_ReceiveFilter_t rxFilter4 = {.homeId = {{0xFB, 0x68, 0x48, 0x1E}},
//                                  .payloadIndex1 = 0x01, .payloadFilterValue1 = 12,
//                                  .headerType = 0x05,                                 // Explorer
//                                  .flag = HOMEID_FILTER_FLAG | PAYLOAD_INDEX_1_FILTER_FLAG,
//                                  .frameHandler = testFilter4Callback};
//
//  ZW_ReceiveFilter_t rxFilter5 = {.homeId = {{0xFB, 0x68, 0x48, 0x1E}},
//                                  .sourceNodeId      = 0,
//                                  .destinationNodeId = 0xFF,
//                                  .payloadIndex1 = 0x00, .payloadFilterValue1 = 1,
//                                  .payloadIndex2 = 0x01, .payloadFilterValue2 = 10,
//                                  .headerType = 0x05,                                 // Explorer
//                                  .flag = HOMEID_FILTER_FLAG | SOURCE_NODE_ID_FILTER_FLAG | DESTINATION_NODE_ID_FILTER_FLAG | PAYLOAD_INDEX_1_FILTER_FLAG | PAYLOAD_INDEX_2_FILTER_FLAG,
//                                  .frameHandler = testFilter5Callback};
//
//  mp_zpal_received_frame->frameContentLength = sizeof(explorerNifFrame);
//  memcpy(mp_zpal_received_frame->frameContent, explorerNifFrame, mp_zpal_received_frame->frameContentLength);
//      radioProfile.receive_handler_cb(&rxParameters, mp_zpal_received_frame);
//  TEST_ASSERT_FALSE(mTestFilter1Called);
//
//  // Start adding the gloabal explorer filter
//  actualVal = llReceiveFilterAdd(&rxFilter6);
//  TEST_ASSERT_EQUAL(SUCCESS, actualVal);
//
//  // Home ID only
//  actualVal = llReceiveFilterAdd(&rxFilter0);
//  TEST_ASSERT_EQUAL(SUCCESS, actualVal);
//
//  // Home ID and dest
//  actualVal = llReceiveFilterAdd(&rxFilter1);
//  TEST_ASSERT_EQUAL(SUCCESS, actualVal);
//
//  // Home ID, source, and dest
//  actualVal = llReceiveFilterAdd(&rxFilter2);
//  TEST_ASSERT_EQUAL(SUCCESS, actualVal);
//
//  // Home ID, source, dest, payload 1 and 2
//  actualVal = llReceiveFilterAdd(&rxFilter3);
//  TEST_ASSERT_EQUAL(SUCCESS, actualVal);
//
//  // Home ID and payload 1
//  actualVal = llReceiveFilterAdd(&rxFilter4);
//  TEST_ASSERT_EQUAL(NO_MEMORY, actualVal);
//
//  // Home ID, payload 1, payload 2
//  actualVal = llReceiveFilterAdd(&rxFilter5);
//  TEST_ASSERT_EQUAL(NO_MEMORY, actualVal);
//
//
//  // Pass the foreign frame. Only global explorer filter should match.
//  mp_zpal_received_frame->frameContentLength = sizeof(explorerFrameForeign);
//  memcpy(mp_zpal_received_frame->frameContent, explorerFrameForeign, mp_zpal_received_frame->frameContentLength);
//      radioProfile.receive_handler_cb(&rxParameters, mp_zpal_received_frame);
//  TEST_ASSERT_TRUE(mTestFilter6Called);
//  mTestFilter6Called = false;
//  mpTestFilter6ReceivedFrame = NULL;
//
//
//  mp_zpal_received_frame->frameContentLength = sizeof(explorerNifFrame);
//  memcpy(mp_zpal_received_frame->frameContent, explorerNifFrame, mp_zpal_received_frame->frameContentLength);
//      radioProfile.receive_handler_cb(&rxParameters, mp_zpal_received_frame);
//  TEST_ASSERT_TRUE(mTestFilter2Called);
//#ifdef BACKWARD_COMPABILITY_TEST
//  TEST_ASSERT_EQUAL(43, mTestFilter2ReceivedFrame.frameContentLength);
//  TEST_ASSERT_EQUAL(0, memcmp(&mTestFilter2ReceivedFrame.frameContent[9], &explorerNifFrame[9], mTestFilter2ReceivedFrame.frameContentLength - 9));
//#else  // BACKWARD_COMPABILITY_TEST
//  TEST_ASSERT_EQUAL(34, mTestFilter2ReceivedFrame.frameContentLength);
//  TEST_ASSERT_EQUAL(0, memcmp(mTestFilter2ReceivedFrame.frameContent, &explorerNifFrame[9], mTestFilter2ReceivedFrame.frameContentLength));
//#endif // BACKWARD_COMPABILITY_TEST
//
//  mTestFilter2Called = false;
//  mpTestFilter2ReceivedFrame = NULL;
//
//  // Remove two filters to ensure room for adding filters again.
//  actualVal = llReceiveFilterRemove(&rxFilter3);
//  TEST_ASSERT_EQUAL(SUCCESS, actualVal);
//
//  actualVal = llReceiveFilterRemove(&rxFilter2);
//  TEST_ASSERT_EQUAL(SUCCESS, actualVal);
//
//  actualVal = llReceiveFilterAdd(&rxFilter5);
//  TEST_ASSERT_EQUAL(SUCCESS, actualVal);
//
//  // Filter 2 is removed, thus filter 0 should match
//      radioProfile.receive_handler_cb(&rxParameters, mp_zpal_received_frame);
//  TEST_ASSERT_FALSE(mTestFilter2Called);
//  TEST_ASSERT_TRUE(mTestFilter0Called);
//  mTestFilter0Called = false;
//
//  // Re-add the main matching filter and check that frames are correctly received.
//  actualVal = llReceiveFilterAdd(&rxFilter2);
//  TEST_ASSERT_EQUAL(SUCCESS, actualVal);
//
//      radioProfile.receive_handler_cb(&rxParameters, mp_zpal_received_frame);
//  TEST_ASSERT_TRUE(mTestFilter2Called);
//#ifdef BACKWARD_COMPABILITY_TEST
//  TEST_ASSERT_EQUAL(43, mTestFilter2ReceivedFrame.frameContentLength);
//  TEST_ASSERT_EQUAL(0, memcmp(&mTestFilter2ReceivedFrame.frameContent[9], &explorerNifFrame[9], mTestFilter2ReceivedFrame.frameContentLength - 9));
//#else  // BACKWARD_COMPABILITY_TEST
//  TEST_ASSERT_EQUAL(34, mTestFilter2ReceivedFrame.frameContentLength);
//  TEST_ASSERT_EQUAL(0, memcmp(mTestFilter2ReceivedFrame.frameContent, &explorerNifFrame[9], mTestFilter2ReceivedFrame.frameContentLength));
//#endif // BACKWARD_COMPABILITY_TEST
//
//  mTestFilter2Called = false;
//  mpTestFilter2ReceivedFrame = NULL;
//  mp_zpal_received_frame->frameContentLength = sizeof(explorerFrameForeign);
//  memcpy(mp_zpal_received_frame->frameContent, explorerFrameForeign, mp_zpal_received_frame->frameContentLength);
//
//      radioProfile.receive_handler_cb(&rxParameters, mp_zpal_received_frame);
//  TEST_ASSERT_FALSE(mTestFilter0Called);
//  TEST_ASSERT_FALSE(mTestFilter1Called);
//  TEST_ASSERT_FALSE(mTestFilter2Called);
//  TEST_ASSERT_FALSE(mTestFilter3Called);
//  TEST_ASSERT_FALSE(mTestFilter4Called);
//  TEST_ASSERT_FALSE(mTestFilter5Called);
//}
//
///*
// * This test verifies that a filter with payload matching on a high index, e.g. 50, will not trigger a match if payload is <50.
// */
//void test_receive_explorer_out_of_bound_filters(void)
//{
//  /** As correct radio initialization is ensured in other tests, we just utilize the helper function in this test. */
//  helper_func_expect_radio_init_eu(0x06);
//
//  zpal_radio_rx_parameters_t rxParameters = {.speed = ZPAL_RADIO_SPEED_40K,// Speed 9.6K=1000, Speed 40K=0100, Speed 100K=0010.
//                                   .channelId = 1,
//                                   .rssi = 0};
//
//  uint8_t          explorerNifFrame[] = {0xFB, 0x68, 0x48, 0x1E, 0x00, 0x45, 0x08, 0x2B, 0xFF,                                                                   // HomeId, Src, Flags1, Flags2, Length, Dst,
//                                         0x21, 0x00, 0xFA, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,                                                 // Explorer Header: Explorer Command, TxRandom, TTL, Repeaters, Network Home Id,
//                                         0x01, 0x01, 0xD3, 0x9C, 0x01, 0x10, 0x01, 0x5E, 0x25, 0x85, 0x8E, 0x59, 0x55, 0x86, 0x72, 0x5A, 0x73, 0x98, 0x9F, 0x6C, // CC vers, NIF, Capabilities, Security, Properties, CC supported,
//                                         0x7A, 0xD7}; // Crc
//
//  ZW_ReturnCode_t radioInitReturnValueExpected = SUCCESS;
//  ZW_ReturnCode_t actualVal;
//
//  zpal_radio_profile_t radioProfile = { REGION_EU, ZPAL_RADIO_WAKEUP_ALWAYS_LISTEN };
//  actualVal = llInit(&radioProfile);
//  TEST_ASSERT_EQUAL(radioInitReturnValueExpected, actualVal);
//
//  ZW_ReceiveFilter_t rxFilter0 = {.homeId = {{0xFB, 0x68, 0x48, 0x1E}},
//                                  .payloadIndex1 = 50, .payloadFilterValue1 = 0x55,
//                                  .payloadIndex2 = 51, .payloadFilterValue2 = 0x55,
//                                  .headerType = 0x05,                                 // Explorer
//                                  .flag = HOMEID_FILTER_FLAG | PAYLOAD_INDEX_1_FILTER_FLAG | PAYLOAD_INDEX_2_FILTER_FLAG,
//                                  .frameHandler = testFilter0Callback};
//
//  memset(receive_buffer, 0x55, sizeof(receive_buffer)); // Set all buffer content to 0x55, to trigger payload filter match if compared at out of bounds.
//  mp_zpal_received_frame->frameContentLength = sizeof(explorerNifFrame);
//  memcpy(mp_zpal_received_frame->frameContent, explorerNifFrame, mp_zpal_received_frame->frameContentLength);
//      radioProfile.receive_handler_cb(&rxParameters, mp_zpal_received_frame);
//  TEST_ASSERT_FALSE(mTestFilter0Called);
//
//  // Start adding the explorer filter with payload filter
//  actualVal = llReceiveFilterAdd(&rxFilter0);
//  TEST_ASSERT_EQUAL(SUCCESS, actualVal);
//
//  // Receive the frame, the filter should not be triggered, as the payload filter index is out of bounds.
//  TEST_ASSERT_EQUAL(0x55, mp_zpal_received_frame->frameContent[59]); // Check the frame, does indeed contain the filtered data in the out of bounds memory section. (9 is added to index internally, as that is basic header length)
//      radioProfile.receive_handler_cb(&rxParameters, mp_zpal_received_frame);
//  TEST_ASSERT_FALSE(mTestFilter0Called);
//}
//
///** Verification that the Data Link Layer can filter a frame and discard based on each flag type.
// *
// */
//void test_receive_explorer_filtering_flags(void)
//{
//  /** As correct radio initialization is ensured in other tests, we just utilize the helper function in this test. */
//  helper_func_expect_radio_init_eu(0x06);
//
//  zpal_radio_rx_parameters_t rxParameters = {.speed = ZPAL_RADIO_SPEED_40K,// Speed 9.6K=1000, Speed 40K=0100, Speed 100K=0010.
//                                   .channelId = 1,
//                                   .rssi = 0};
//
//  uint8_t          explorerNifFrame[] = {0xFB, 0x68, 0x48, 0x1E, 0x00, 0x45, 0x08, 0x2B, 0xFF,                                                                   // HomeId, Src, Flags1, Flags2, Length, Dst,
//                                         0x21, 0x00, 0xFA, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,                                                 // Explorer Header: Explorer Command, TxRandom, TTL, Repeaters, Network Home Id,
//                                         0x01, 0x01, 0xD3, 0x9C, 0x01, 0x10, 0x01, 0x5E, 0x25, 0x85, 0x8E, 0x59, 0x55, 0x86, 0x72, 0x5A, 0x73, 0x98, 0x9F, 0x6C, // CC vers, NIF, Capabilities, Security, Properties, CC supported,
//                                         0x7A, 0xD7}; // Crc
//
//  ZW_ReturnCode_t radioInitReturnValueExpected = SUCCESS;
//  ZW_ReturnCode_t actualVal;
//
//  zpal_radio_profile_t radioProfile = { REGION_EU, ZPAL_RADIO_WAKEUP_ALWAYS_LISTEN };
//  actualVal = llInit(&radioProfile);
//  TEST_ASSERT_EQUAL(radioInitReturnValueExpected, actualVal);
//
//  ZW_ReceiveFilter_t rxFilter5 = {.headerType = 0x05,                                 // Explorer
//                                  .flag = 0,
//                                  .frameHandler = testFilter5Callback};
//
//  ZW_ReceiveFilter_t rxFilter0 = {.homeId = {{0xFB, 0x68, 0x48, 0x12}},
//                                  .headerType = 0x05,                                 // Explorer
//                                  .flag = HOMEID_FILTER_FLAG,
//                                  .frameHandler = testFilter0Callback};
//
//  ZW_ReceiveFilter_t rxFilter1 = {.homeId = {{0xFB, 0x68, 0x48, 0x1E}},
//                                  .destinationNodeId = 2,
//                                  .headerType = 0x05,                                 // Explorer
//                                  .flag = HOMEID_FILTER_FLAG | DESTINATION_NODE_ID_FILTER_FLAG,
//                                  .frameHandler = testFilter1Callback};
//
//  ZW_ReceiveFilter_t rxFilter2 = {.homeId = {{0xFB, 0x68, 0x48, 0x1E}},
//                                  .sourceNodeId      = 1,
//                                  .destinationNodeId = 0xFF,
//                                  .headerType = 0x05,                                 // Explorer
//                                  .flag = HOMEID_FILTER_FLAG | SOURCE_NODE_ID_FILTER_FLAG | DESTINATION_NODE_ID_FILTER_FLAG,
//                                  .frameHandler = testFilter2Callback};
//
//  ZW_ReceiveFilter_t rxFilter3 = {.homeId = {{0xFB, 0x68, 0x48, 0x1E}},
//                                  .sourceNodeId      = 0,
//                                  .destinationNodeId = 0xFF,
//                                  .payloadIndex1 = 12, .payloadFilterValue1 = 7,  // First index in explorer payload
//                                  .headerType = 0x05,                                 // Explorer
//                                  .flag = HOMEID_FILTER_FLAG | SOURCE_NODE_ID_FILTER_FLAG | DESTINATION_NODE_ID_FILTER_FLAG | PAYLOAD_INDEX_1_FILTER_FLAG,
//                                  .frameHandler = testFilter3Callback};
//
//  ZW_ReceiveFilter_t rxFilter4 = {.homeId = {{0xFB, 0x68, 0x48, 0x1E}},
//                                  .sourceNodeId      = 0,
//                                  .destinationNodeId = 0xFF,
//                                  .payloadIndex1 = 12, .payloadFilterValue1 = 1,  // First index in explorer payload
//                                  .payloadIndex2 = 13, .payloadFilterValue2 = 10, // second index in explorer payload
//                                  .headerType = 0x05,                                 // Explorer
//                                  .flag = HOMEID_FILTER_FLAG | SOURCE_NODE_ID_FILTER_FLAG | DESTINATION_NODE_ID_FILTER_FLAG | PAYLOAD_INDEX_1_FILTER_FLAG | PAYLOAD_INDEX_2_FILTER_FLAG,
//                                  .frameHandler = testFilter4Callback};
//
//  mp_zpal_received_frame->frameContentLength = sizeof(explorerNifFrame);
//  memcpy(mp_zpal_received_frame->frameContent, explorerNifFrame, mp_zpal_received_frame->frameContentLength);
//      radioProfile.receive_handler_cb(&rxParameters, mp_zpal_received_frame);
//  TEST_ASSERT_FALSE(mTestFilter0Called | mTestFilter1Called | mTestFilter2Called | mTestFilter3Called | mTestFilter4Called | mTestFilter5Called);
//
//  // Start adding the gloabal explorer filter
//  actualVal = llReceiveFilterAdd(&rxFilter5);
//  TEST_ASSERT_EQUAL(SUCCESS, actualVal);
//
//  // Home ID only (No match)
//  actualVal = llReceiveFilterAdd(&rxFilter0);
//  TEST_ASSERT_EQUAL(SUCCESS, actualVal);
//
//      radioProfile.receive_handler_cb(&rxParameters, mp_zpal_received_frame);
//  TEST_ASSERT_FALSE(mTestFilter0Called | mTestFilter1Called | mTestFilter2Called | mTestFilter3Called | mTestFilter4Called);
//  TEST_ASSERT_TRUE(mTestFilter5Called)
//  mTestFilter5Called = false;
//
//  // Home ID and dest (No match)
//  actualVal = llReceiveFilterAdd(&rxFilter1);
//  TEST_ASSERT_EQUAL(SUCCESS, actualVal);
//
//      radioProfile.receive_handler_cb(&rxParameters, mp_zpal_received_frame);
//  TEST_ASSERT_FALSE(mTestFilter0Called | mTestFilter1Called | mTestFilter2Called | mTestFilter3Called | mTestFilter4Called);
//  TEST_ASSERT_TRUE(mTestFilter5Called)
//  mTestFilter5Called = false;
//
//  // Home ID, source, and dest (No match)
//  actualVal = llReceiveFilterAdd(&rxFilter2);
//  TEST_ASSERT_EQUAL(SUCCESS, actualVal);
//
//      radioProfile.receive_handler_cb(&rxParameters, mp_zpal_received_frame);
//  TEST_ASSERT_FALSE(mTestFilter0Called | mTestFilter1Called | mTestFilter2Called | mTestFilter3Called | mTestFilter4Called);
//  TEST_ASSERT_TRUE(mTestFilter5Called)
//  mTestFilter5Called = false;
//
//  // Home ID, source, dest, payload 1 (No match)
//  actualVal = llReceiveFilterAdd(&rxFilter3);
//  TEST_ASSERT_EQUAL(SUCCESS, actualVal);
//
//      radioProfile.receive_handler_cb(&rxParameters, mp_zpal_received_frame);
//  TEST_ASSERT_FALSE(mTestFilter0Called | mTestFilter1Called | mTestFilter2Called | mTestFilter3Called | mTestFilter4Called);
//  TEST_ASSERT_TRUE(mTestFilter5Called)
//  mTestFilter5Called = false;
//
//  // Remove a single filter to make room for next in list.
//  actualVal = llReceiveFilterRemove(&rxFilter0);
//  TEST_ASSERT_EQUAL(SUCCESS, actualVal);
//
//  // Home ID, source, dest, payload 1, payload 2 (No match)
//  actualVal = llReceiveFilterAdd(&rxFilter4);
//  TEST_ASSERT_EQUAL(SUCCESS, actualVal);
//
//      radioProfile.receive_handler_cb(&rxParameters, mp_zpal_received_frame);
//  TEST_ASSERT_FALSE(mTestFilter0Called | mTestFilter1Called | mTestFilter2Called | mTestFilter3Called | mTestFilter4Called);
//  TEST_ASSERT_TRUE(mTestFilter5Called)
//  mTestFilter5Called = false;
//
//#ifdef BACKWARD_COMPABILITY_TEST
//  TEST_ASSERT_EQUAL(43, mTestFilter5ReceivedFrame.frameContentLength);
//  TEST_ASSERT_EQUAL(0, memcmp(&mTestFilter5ReceivedFrame.frameContent[9], &explorerNifFrame[9], mTestFilter5ReceivedFrame.frameContentLength - 9));
//#else  // BACKWARD_COMPABILITY_TEST
//  TEST_ASSERT_EQUAL(34, mTestFilter0ReceivedFrame.frameContentLength);
//  TEST_ASSERT_EQUAL(0, memcmp(mTestFilter5ReceivedFrame.frameContent, &explorerNifFrame[9], mTestFilter5ReceivedFrame.frameContentLength));
//#endif // BACKWARD_COMPABILITY_TEST
//}
//
///** Verification that the Data Link Layer can filter frames based upon HOMEID, empty Payload filter on ACK frames.
// *
// */
//void test_receive_add_ack_filters(void)
//{
//  /** As correct radio initialization is ensured in other tests, we just utilize the helper function in this test. */
//  helper_func_expect_radio_init_eu(0x06);
//
//  zpal_radio_rx_parameters_t rxParameters = {.speed = ZPAL_RADIO_SPEED_40K,// Speed 9.6K=1000, Speed 40K=0100, Speed 100K=0010.
//                                   .channelId = 1,
//                                   .rssi = 0};
//
//  uint8_t          ackFrame9600[] = {0xca, 0xfe, 0xba, 0xbf, 0x02, 0x03, 0x02, 0x0a, 0x01, // HomeId, Src, Flags1, Flags2, Length, Dst,
//                                     0xC6}; // Crc
//  uint8_t          ackFrame100k[] = {0xca, 0xfe, 0xba, 0xbf, 0x02, 0x03, 0x03, 0x0b, 0x01, // HomeId, Src, Flags1, Flags2, Length, Dst,
//                                     0x7d, 0x21}; // Crc
//
//  uint8_t          ackFrame100kForeign[] = {0xE3, 0x26, 0xE5, 0x51, 0x02, 0x03, 0x04, 0x0B, 0x01,
//                                            0xDE, 0x4B};
//
//  ZW_ReturnCode_t radioInitReturnValueExpected = SUCCESS;
//  ZW_ReturnCode_t actualVal;
//
//  zpal_radio_profile_t radioProfile = { REGION_EU, ZPAL_RADIO_WAKEUP_ALWAYS_LISTEN };
//  actualVal = llInit(&radioProfile);
//  TEST_ASSERT_EQUAL(radioInitReturnValueExpected, actualVal);
//
//  ZW_ReceiveFilter_t rxFilter0 = {.homeId = {{0xca, 0xfe, 0xba, 0xbf}},
//                                  .headerType = 0x03,                                 // Ack
//                                  .flag = HOMEID_FILTER_FLAG,
//                                  .frameHandler = testFilter0Callback};
//
//  ZW_ReceiveFilter_t rxFilter1 = {.homeId = {{0xca, 0xfe, 0xba, 0xbf}},
//                                  .destinationNodeId = 1,
//                                  .headerType = 0x03,                                 // Ack
//                                  .flag = HOMEID_FILTER_FLAG | DESTINATION_NODE_ID_FILTER_FLAG,
//                                  .frameHandler = testFilter1Callback};
//
//  ZW_ReceiveFilter_t rxFilter2 = {.homeId = {{0xca, 0xfe, 0xba, 0xbf}},
//                                  .sourceNodeId      = 2,
//                                  .destinationNodeId = 1,
//                                  .headerType = 0x03,                                 // Ack
//                                  .flag = HOMEID_FILTER_FLAG | SOURCE_NODE_ID_FILTER_FLAG | DESTINATION_NODE_ID_FILTER_FLAG,
//                                  .frameHandler = testFilter2Callback};
//
//  ZW_ReceiveFilter_t rxFilter3 = {.homeId = {{0xca, 0xfe, 0xba, 0xbf}},
//                                  .sourceNodeId      = 0,
//                                  .destinationNodeId = 1,
//                                  .headerType = 0x03,                                 // Ack
//                                  .flag = HOMEID_FILTER_FLAG | SOURCE_NODE_ID_FILTER_FLAG | DESTINATION_NODE_ID_FILTER_FLAG,
//                                  .frameHandler = testFilter3Callback};
//
//  ZW_ReceiveFilter_t rxFilter4 = {.headerType = 0x03,                                 // Ack
//                                  .flag = 0,
//                                  .frameHandler = testFilter4Callback};
//
//  ZW_ReceiveFilter_t rxFilter5 = {.homeId = {{0x12, 0x34, 0x56, 0x78}},
//                                  .headerType = 0x03,                                 // Ack
//                                  .flag = HOMEID_FILTER_FLAG,
//                                  .frameHandler = testFilter5Callback};
//
//  mp_zpal_received_frame->frameContentLength = sizeof(ackFrame9600);
//  memcpy(mp_zpal_received_frame->frameContent, ackFrame9600, mp_zpal_received_frame->frameContentLength);
//      radioProfile.receive_handler_cb(&rxParameters, mp_zpal_received_frame);
//  TEST_ASSERT_FALSE(mTestFilter1Called);
//
//  actualVal = llReceiveFilterAdd(&rxFilter0);
//  TEST_ASSERT_EQUAL(SUCCESS, actualVal);
//
//  actualVal = llReceiveFilterAdd(&rxFilter1);
//  TEST_ASSERT_EQUAL(SUCCESS, actualVal);
//
//  actualVal = llReceiveFilterAdd(&rxFilter2);
//  TEST_ASSERT_EQUAL(SUCCESS, actualVal);
//
//  actualVal = llReceiveFilterAdd(&rxFilter3);
//  TEST_ASSERT_EQUAL(SUCCESS, actualVal);
//
//  actualVal = llReceiveFilterAdd(&rxFilter4);
//  TEST_ASSERT_EQUAL(SUCCESS, actualVal);
//
//  actualVal = llReceiveFilterAdd(&rxFilter5);
//  TEST_ASSERT_EQUAL(NO_MEMORY, actualVal);
//
//      radioProfile.receive_handler_cb(&rxParameters, mp_zpal_received_frame);
//  TEST_ASSERT_TRUE(mTestFilter2Called);
//#ifdef BACKWARD_COMPABILITY_TEST
//  TEST_ASSERT_EQUAL(10, mTestFilter2ReceivedFrame.frameContentLength);
//  TEST_ASSERT_EQUAL(0, memcmp(&mTestFilter2ReceivedFrame.frameContent[9], &ackFrame9600[9], mTestFilter2ReceivedFrame.frameContentLength - 9));
//#else  // BACKWARD_COMPABILITY_TEST
//  TEST_ASSERT_EQUAL(1, mTestFilter2ReceivedFrame.frameContentLength);
//  TEST_ASSERT_EQUAL(0, memcmp(mTestFilter2ReceivedFrame.frameContent, &ackFrame9600[9], mTestFilter2ReceivedFrame.frameContentLength));
//#endif // BACKWARD_COMPABILITY_TEST
//
//  mTestFilter2Called = false;
//  mpTestFilter2ReceivedFrame = NULL;
//  mp_zpal_received_frame->frameContentLength = sizeof(ackFrame100k);
//  memcpy(mp_zpal_received_frame->frameContent, ackFrame100k, mp_zpal_received_frame->frameContentLength);
//
//  // Remove two filters to ensure room for adding filters again.
//  actualVal = llReceiveFilterRemove(&rxFilter3);
//  TEST_ASSERT_EQUAL(SUCCESS, actualVal);
//
//  actualVal = llReceiveFilterRemove(&rxFilter2);
//  TEST_ASSERT_EQUAL(SUCCESS, actualVal);
//
//  actualVal = llReceiveFilterAdd(&rxFilter5);
//  TEST_ASSERT_EQUAL(SUCCESS, actualVal);
//
//  // Filter 2 is removed, thus filter 1 should match
//      radioProfile.receive_handler_cb(&rxParameters, mp_zpal_received_frame);
//  TEST_ASSERT_FALSE(mTestFilter2Called);
//  TEST_ASSERT_TRUE(mTestFilter1Called);
//  mTestFilter1Called = false;
//
//  // Re-add the main matching filter and check that frames are correctly received.
//  actualVal = llReceiveFilterAdd(&rxFilter2);
//  TEST_ASSERT_EQUAL(SUCCESS, actualVal);
//
//      radioProfile.receive_handler_cb(&rxParameters, mp_zpal_received_frame);
//  TEST_ASSERT_TRUE(mTestFilter2Called);
//#ifdef BACKWARD_COMPABILITY_TEST
//  TEST_ASSERT_EQUAL(11, mTestFilter2ReceivedFrame.frameContentLength);
//  TEST_ASSERT_EQUAL(0, memcmp(&mTestFilter2ReceivedFrame.frameContent[9], &ackFrame100k[9], mTestFilter2ReceivedFrame.frameContentLength - 9));
//#else  // BACKWARD_COMPABILITY_TEST
//  TEST_ASSERT_EQUAL(2, mTestFilter2ReceivedFrame.frameContentLength);
//  TEST_ASSERT_EQUAL(0, memcmp(mTestFilter2ReceivedFrame.frameContent, &ackFrame100k[9], mTestFilter2ReceivedFrame.frameContentLength));
//#endif // BACKWARD_COMPABILITY_TEST
//
//  mTestFilter2Called = false;
//  mpTestFilter2ReceivedFrame = NULL;
//  mp_zpal_received_frame->frameContentLength = sizeof(ackFrame100kForeign);
//  memcpy(mp_zpal_received_frame->frameContent, ackFrame100kForeign, mp_zpal_received_frame->frameContentLength);
//
//      radioProfile.receive_handler_cb(&rxParameters, mp_zpal_received_frame);
//  TEST_ASSERT_FALSE(mTestFilter0Called);
//  TEST_ASSERT_FALSE(mTestFilter1Called);
//  TEST_ASSERT_FALSE(mTestFilter2Called);
//  TEST_ASSERT_FALSE(mTestFilter3Called);
//  TEST_ASSERT_TRUE(mTestFilter4Called);
//  TEST_ASSERT_FALSE(mTestFilter5Called);
//}
//
///** Verification that ACK frame receive handler does not support filters with payload matching.
// *
// */
//void test_receive_add_ack_payload_filter_fail(void)
//{
//  /** As correct radio initialization is ensured in other tests, we just utilize the helper function in this test. */
//  helper_func_expect_radio_init_eu(0x06);
//
//  zpal_radio_rx_parameters_t rxParameters = {.speed = ZPAL_RADIO_SPEED_40K,// Speed 9.6K=1000, Speed 40K=0100, Speed 100K=0010.
//                                   .channelId = 1,
//                                   .rssi = 0};
//
//  ZW_ReturnCode_t radioInitReturnValueExpected = SUCCESS;
//  ZW_ReturnCode_t actualVal;
//
//  zpal_radio_profile_t radioProfile = { REGION_EU, ZPAL_RADIO_WAKEUP_ALWAYS_LISTEN };
//  actualVal = llInit(&radioProfile);
//  TEST_ASSERT_EQUAL(radioInitReturnValueExpected, actualVal);
//
//  ZW_ReceiveFilter_t rxFilter0 = {.homeId = {{0xca, 0xfe, 0xba, 0xbf}},
//                                  .payloadIndex1 = 0, .payloadFilterValue1 = 0x11,
//                                  .payloadIndex2 = 1, .payloadFilterValue2 = 0x11,
//                                  .headerType = 0x03,                                 // Ack
//                                  .flag = HOMEID_FILTER_FLAG | PAYLOAD_INDEX_1_FILTER_FLAG | PAYLOAD_INDEX_2_FILTER_FLAG,
//                                  .frameHandler = testFilter0Callback};
//
//
//  actualVal = llReceiveFilterAdd(&rxFilter0);
//  TEST_ASSERT_EQUAL(INVALID_PARAMETERS, actualVal);
//}
//
///** Verification that the Data Link Layer can filter ACK frames and discard based on each flag type.
// *
// */
//void test_receive_ack_filtering_flags(void)
//{
//  /** As correct radio initialization is ensured in other tests, we just utilize the helper function in this test. */
//  helper_func_expect_radio_init_eu(0x06);
//
//  zpal_radio_rx_parameters_t rxParameters = {.speed = ZPAL_RADIO_SPEED_40K,// Speed 9.6K=1000, Speed 40K=0100, Speed 100K=0010.
//                                   .channelId = 1,
//                                   .rssi = 0};
//
//  uint8_t          ackFrame9600[] = {0xca, 0xfe, 0xba, 0xbf, 0x02, 0x03, 0x02, 0x0a, 0x01, // HomeId, Src, Flags1, Flags2, Length, Dst,
//                                     0xC6}; // Crc
//  uint8_t          ackFrame100k[] = {0xca, 0xfe, 0xba, 0xbf, 0x02, 0x03, 0x03, 0x0b, 0x01, // HomeId, Src, Flags1, Flags2, Length, Dst,
//                                     0x7d, 0x21}; // Crc
//
//  uint8_t          ackFrame100kForeign[] = {0xE3, 0x26, 0xE5, 0x51, 0x02, 0x03, 0x04, 0x0B, 0x01,
//                                            0xDE, 0x4B};
//
//  ZW_ReturnCode_t radioInitReturnValueExpected = SUCCESS;
//  ZW_ReturnCode_t actualVal;
//
//  zpal_radio_profile_t radioProfile = { REGION_EU, ZPAL_RADIO_WAKEUP_ALWAYS_LISTEN };
//  actualVal = llInit(&radioProfile);
//  TEST_ASSERT_EQUAL(radioInitReturnValueExpected, actualVal);
//
//  ZW_ReceiveFilter_t rxFilter5 = {.headerType = 0x03,                                 // ACK
//                                  .flag = 0,
//                                  .frameHandler = testFilter5Callback};
//
//  ZW_ReceiveFilter_t rxFilter0 = {.homeId = {{0xca, 0xfe, 0xba, 0xb3}},
//                                  .headerType = 0x03,                                 // ACK
//                                  .flag = HOMEID_FILTER_FLAG,
//                                  .frameHandler = testFilter0Callback};
//
//  ZW_ReceiveFilter_t rxFilter1 = {.homeId = {{0xca, 0xfe, 0xba, 0xbf}},
//                                  .destinationNodeId = 11,
//                                  .headerType = 0x03,                                 // ACK
//                                  .flag = HOMEID_FILTER_FLAG | DESTINATION_NODE_ID_FILTER_FLAG,
//                                  .frameHandler = testFilter1Callback};
//
//  ZW_ReceiveFilter_t rxFilter2 = {.homeId = {{0xca, 0xfe, 0xba, 0xbf}},
//                                  .sourceNodeId      = 12,
//                                  .destinationNodeId = 1,
//                                  .headerType = 0x03,                                 // ACK
//                                  .flag = HOMEID_FILTER_FLAG | SOURCE_NODE_ID_FILTER_FLAG | DESTINATION_NODE_ID_FILTER_FLAG,
//                                  .frameHandler = testFilter2Callback};
//
//  mp_zpal_received_frame->frameContentLength = sizeof(ackFrame100k);
//  memcpy(mp_zpal_received_frame->frameContent, ackFrame100k, mp_zpal_received_frame->frameContentLength);
//      radioProfile.receive_handler_cb(&rxParameters, mp_zpal_received_frame);
//  TEST_ASSERT_FALSE(mTestFilter0Called | mTestFilter1Called | mTestFilter2Called | mTestFilter5Called);
//
//  // Start adding the gloabal explorer filter
//  actualVal = llReceiveFilterAdd(&rxFilter5);
//  TEST_ASSERT_EQUAL(SUCCESS, actualVal);
//
//  // Home ID only (No match)
//  actualVal = llReceiveFilterAdd(&rxFilter0);
//  TEST_ASSERT_EQUAL(SUCCESS, actualVal);
//
//      radioProfile.receive_handler_cb(&rxParameters, mp_zpal_received_frame);
//  TEST_ASSERT_FALSE(mTestFilter0Called | mTestFilter1Called | mTestFilter2Called);
//  TEST_ASSERT_TRUE(mTestFilter5Called)
//  mTestFilter5Called = false;
//
//  // Home ID and dest (No match)
//  actualVal = llReceiveFilterAdd(&rxFilter1);
//  TEST_ASSERT_EQUAL(SUCCESS, actualVal);
//
//      radioProfile.receive_handler_cb(&rxParameters, mp_zpal_received_frame);
//  TEST_ASSERT_FALSE(mTestFilter0Called | mTestFilter1Called | mTestFilter2Called);
//  TEST_ASSERT_TRUE(mTestFilter5Called)
//  mTestFilter5Called = false;
//
//  // Home ID, source, and dest (No match)
//  actualVal = llReceiveFilterAdd(&rxFilter2);
//  TEST_ASSERT_EQUAL(SUCCESS, actualVal);
//
//      radioProfile.receive_handler_cb(&rxParameters, mp_zpal_received_frame);
//  TEST_ASSERT_FALSE(mTestFilter1Called);
//
//  // Start adding the gloabal explorer filter
//  actualVal = llReceiveFilterAdd(&rxFilter6);
//  TEST_ASSERT_EQUAL(SUCCESS, actualVal);
//
//  // Home ID only
//  actualVal = llReceiveFilterAdd(&rxFilter0);
//  TEST_ASSERT_EQUAL(SUCCESS, actualVal);
//
//  // Home ID and dest
//  actualVal = llReceiveFilterAdd(&rxFilter1);
//  TEST_ASSERT_EQUAL(SUCCESS, actualVal);
//
//  // Home ID, source, and dest
//  actualVal = llReceiveFilterAdd(&rxFilter2);
//  TEST_ASSERT_EQUAL(SUCCESS, actualVal);
//
//  // Home ID, source, dest, payload 1 and 2
//  actualVal = llReceiveFilterAdd(&rxFilter3);
//  TEST_ASSERT_EQUAL(SUCCESS, actualVal);
//
//  // Home ID and payload 1
//  actualVal = llReceiveFilterAdd(&rxFilter4);
//  TEST_ASSERT_EQUAL(NO_MEMORY, actualVal);
//
//  // Home ID, payload 1, payload 2
//  actualVal = llReceiveFilterAdd(&rxFilter5);
//  TEST_ASSERT_EQUAL(NO_MEMORY, actualVal);
//
//
//  // Pass the foreign frame. Only global explorer filter should match.
//  mp_zpal_received_frame->frameContentLength = sizeof(cmdMulticastFrameForeign);
//  memcpy(mp_zpal_received_frame->frameContent, cmdMulticastFrameForeign, mp_zpal_received_frame->frameContentLength);
//      radioProfile.receive_handler_cb(&rxParameters, mp_zpal_received_frame);
//  TEST_ASSERT_TRUE(mTestFilter6Called);
//  mTestFilter6Called = false;
//  mpTestFilter6ReceivedFrame = NULL;
//
//
//  mp_zpal_received_frame->frameContentLength = sizeof(cmdMulticastFrame);
//  memcpy(mp_zpal_received_frame->frameContent, cmdMulticastFrame, mp_zpal_received_frame->frameContentLength);
//      radioProfile.receive_handler_cb(&rxParameters, mp_zpal_received_frame);
//  TEST_ASSERT_TRUE(mTestFilter2Called);
//#ifdef BACKWARD_COMPABILITY_TEST
//  TEST_ASSERT_EQUAL(41, mTestFilter2ReceivedFrame.frameContentLength);
//  TEST_ASSERT_EQUAL(0, memcmp(&mTestFilter2ReceivedFrame.frameContent[9], &cmdMulticastFrame[9], mTestFilter2ReceivedFrame.frameContentLength - 9));
//#else  // BACKWARD_COMPABILITY_TEST
//  TEST_ASSERT_EQUAL(32, mTestFilter2ReceivedFrame.frameContentLength);
//  TEST_ASSERT_EQUAL(0, memcmp(mTestFilter2ReceivedFrame.frameContent, &cmdMulticastFrame[9], mTestFilter2ReceivedFrame.frameContentLength));
//#endif // BACKWARD_COMPABILITY_TEST
//
//  mTestFilter2Called = false;
//  mpTestFilter2ReceivedFrame = NULL;
//
//  // Remove two filters to ensure room for adding filters again.
//  actualVal = llReceiveFilterRemove(&rxFilter3);
//  TEST_ASSERT_EQUAL(SUCCESS, actualVal);
//
//  actualVal = llReceiveFilterRemove(&rxFilter2);
//  TEST_ASSERT_EQUAL(SUCCESS, actualVal);
//
//  actualVal = llReceiveFilterAdd(&rxFilter5);
//  TEST_ASSERT_EQUAL(SUCCESS, actualVal);
//
//  // Filter 2 is removed, thus filter 0 should match
//      radioProfile.receive_handler_cb(&rxParameters, mp_zpal_received_frame);
//  TEST_ASSERT_FALSE(mTestFilter2Called);
//  TEST_ASSERT_TRUE(mTestFilter0Called);
//  mTestFilter0Called = false;
//
//  // Re-add the main matching filter and check that frames are correctly received.
//  actualVal = llReceiveFilterAdd(&rxFilter2);
//  TEST_ASSERT_EQUAL(SUCCESS, actualVal);
//
//      radioProfile.receive_handler_cb(&rxParameters, mp_zpal_received_frame);
//  TEST_ASSERT_TRUE(mTestFilter2Called);
//#ifdef BACKWARD_COMPABILITY_TEST
//  TEST_ASSERT_EQUAL(41, mTestFilter2ReceivedFrame.frameContentLength);
//  TEST_ASSERT_EQUAL(0, memcmp(&mTestFilter2ReceivedFrame.frameContent[9], &cmdMulticastFrame[9], mTestFilter2ReceivedFrame.frameContentLength - 9));
//#else  // BACKWARD_COMPABILITY_TEST
//  TEST_ASSERT_EQUAL(32, mTestFilter2ReceivedFrame.frameContentLength);
//  TEST_ASSERT_EQUAL(0, memcmp(mTestFilter2ReceivedFrame.frameContent, &cmdMulticastFrame[9], mTestFilter2ReceivedFrame.frameContentLength));
//#endif // BACKWARD_COMPABILITY_TEST
//
//  mTestFilter2Called = false;
//  mpTestFilter2ReceivedFrame = NULL;
//  mp_zpal_received_frame->frameContentLength = sizeof(cmdMulticastFrameForeign);
//  memcpy(mp_zpal_received_frame->frameContent, cmdMulticastFrameForeign, mp_zpal_received_frame->frameContentLength);
//
//      radioProfile.receive_handler_cb(&rxParameters, mp_zpal_received_frame);
//  TEST_ASSERT_FALSE(mTestFilter0Called);
//  TEST_ASSERT_FALSE(mTestFilter1Called);
//  TEST_ASSERT_FALSE(mTestFilter2Called);
//  TEST_ASSERT_FALSE(mTestFilter3Called);
//  TEST_ASSERT_FALSE(mTestFilter4Called);
//  TEST_ASSERT_FALSE(mTestFilter5Called);
//}
//
///*
// * This test verifies that a filter with payload matching on a high index, e.g. 50, will not trigger a match if payload is <50.
// */
//void test_receive_multicast_out_of_bound_filters(void)
//{
//  /** As correct radio initialization is ensured in other tests, we just utilize the helper function in this test. */
//  helper_func_expect_radio_init_eu(0x06);
//
//  zpal_radio_rx_parameters_t rxParameters = {.speed = ZPAL_RADIO_SPEED_40K,// Speed 9.6K=1000, Speed 40K=0100, Speed 100K=0010.
//                                   .channelId = 1,
//                                   .rssi = 0};
//
//  uint8_t          cmdMulticastFrame[] = {0xE3, 0x72, 0x83, 0xAF, 0x02, 0x02, 0x01, 0x29, // General Header
//                                          0x1D, 0x0D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Multicast frame, Address offset + Mask bytes, Mask byte. (Node: 1, 3, 4)
//                                          0x5A, 0x01, // Payload, Device Reset Locally
//                                          0x21};      // CRC
//
//  ZW_ReturnCode_t radioInitReturnValueExpected = SUCCESS;
//  ZW_ReturnCode_t actualVal;
//
//  zpal_radio_profile_t radioProfile = { REGION_EU, BAUD_RATE_9_6k_40k | BAUD_RATE_100k, ZPAL_RADIO_WAKEUP_ALWAYS_LISTEN };
//  actualVal = llInit(&radioProfile);
//  TEST_ASSERT_EQUAL(radioInitReturnValueExpected, actualVal);
//
//  ZW_ReceiveFilter_t rxFilter0 = {.homeId = {{0xFB, 0x68, 0x48, 0x1E}},
//                                  .payloadIndex1 = 50, .payloadFilterValue1 = 0x55,
//                                  .payloadIndex2 = 51, .payloadFilterValue2 = 0x55,
//                                  .headerType = 0x02,                                 // Multicast
//                                  .flag = HOMEID_FILTER_FLAG | PAYLOAD_INDEX_1_FILTER_FLAG | PAYLOAD_INDEX_2_FILTER_FLAG,
//                                  .frameHandler = testFilter0Callback};
//
//  memset(receive_buffer, 0x55, sizeof(receive_buffer)); // Set all buffer content to 0x55, to trigger payload filter match if compared at out of bounds.
//  mp_zpal_received_frame->frameContentLength = sizeof(cmdMulticastFrame);
//  memcpy(mp_zpal_received_frame->frameContent, cmdMulticastFrame, mp_zpal_received_frame->frameContentLength);
//      radioProfile.receive_handler_cb(&rxParameters, mp_zpal_received_frame);
//  TEST_ASSERT_FALSE(mTestFilter0Called);
//
//  // Start adding the explorer filter with payload filter
//  actualVal = llReceiveFilterAdd(&rxFilter0);
//  TEST_ASSERT_EQUAL(SUCCESS, actualVal);
//
//  // Receive the frame, the filter should not be triggered, as the payload filter index is out of bounds.
//  TEST_ASSERT_EQUAL(0x55, mp_zpal_received_frame->frameContent[59]); // Check the frame, does indeed contain the filtered data in the out of bounds memory section. (9 is added to index internally, as that is basic header length)
//      radioProfile.receive_handler_cb(&rxParameters, mp_zpal_received_frame);
//  TEST_ASSERT_FALSE(mTestFilter0Called);
//}
//
///** Verification that the Data Link Layer can filter a frame and discard based on each flag type.
// *
// */
//void test_receive_multicast_filtering_flags(void)
//{
//  /** As correct radio initialization is ensured in other tests, we just utilize the helper function in this test. */
//  helper_func_expect_radio_init_eu(0x06);
//
//  zpal_radio_rx_parameters_t rxParameters = {.speed = ZPAL_RADIO_SPEED_40K,// Speed 9.6K=1000, Speed 40K=0100, Speed 100K=0010.
//                                   .channelId = 1,
//                                   .rssi = 0};
//
//  uint8_t          cmdMulticastFrame[] = {0xE3, 0x72, 0x83, 0xAF, 0x02, 0x02, 0x01, 0x29, // General Header
//                                          0x1D, 0x0D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Multicast frame, Address offset + Mask bytes, Mask byte. (Node: 1, 3, 4)
//                                          0x5A, 0x01, // Payload, Device Reset Locally
//                                          0x21};      // CRC
//
//  ZW_ReturnCode_t radioInitReturnValueExpected = SUCCESS;
//  ZW_ReturnCode_t actualVal;
//
//  zpal_radio_profile_t radioProfile = { REGION_EU, BAUD_RATE_9_6k_40k | BAUD_RATE_100k, ZPAL_RADIO_WAKEUP_ALWAYS_LISTEN };
//  actualVal = llInit(&radioProfile);
//  TEST_ASSERT_EQUAL(radioInitReturnValueExpected, actualVal);
//
//  ZW_ReceiveFilter_t rxFilter0 = {.headerType = 0x02,                                 // Multicast
//                                  .flag = 0,
//                                  .frameHandler = testFilter0Callback};
//
//  ZW_ReceiveFilter_t rxFilter1 = {.homeId = {{0x11, 0x22, 0x33, 0x44}},
//                                  .headerType = 0x02,                                 // Multicast
//                                  .flag = HOMEID_FILTER_FLAG,
//                                  .frameHandler = testFilter1Callback};
//
//  ZW_ReceiveFilter_t rxFilter2 = {.homeId = {{0xE3, 0x72, 0x83, 0xAF}},
//                                  .headerType = 0x02,                                 // Multicast
//                                  .flag = HOMEID_FILTER_FLAG,
//                                  .frameHandler = testFilter1Callback};
//
//  ZW_ReceiveFilter_t rxFilter3 = {.homeId = {{0xE3, 0x72, 0x83, 0xAF}},
//                                  .sourceNodeId = 1,
//                                  .headerType   = 0x02,                               // Multicast
//                                  .flag = HOMEID_FILTER_FLAG | SOURCE_NODE_ID_FILTER_FLAG,
//                                  .frameHandler = testFilter2Callback};
//
//  ZW_ReceiveFilter_t rxFilter4 = {.homeId = {{0xE3, 0x72, 0x83, 0xAF}},
//                                  .sourceNodeId      = 0,
//                                  .payloadIndex1 = 30, .payloadFilterValue1 = 0xAA, // First index in explorer payload
//                                  .headerType = 0x02,                               // Multicast
//                                  .flag = HOMEID_FILTER_FLAG | SOURCE_NODE_ID_FILTER_FLAG | PAYLOAD_INDEX_1_FILTER_FLAG,
//                                  .frameHandler = testFilter3Callback};
//
//  ZW_ReceiveFilter_t rxFilter5 = {.homeId = {{0xE3, 0x72, 0x83, 0xAF}},
//                                  .sourceNodeId  = 0,
//                                  .payloadIndex1 = 30, .payloadFilterValue1 = 0x5A, // First index in explorer payload
//                                  .payloadIndex2 = 31, .payloadFilterValue2 = 0x01, // second index in explorer payload
//                                  .headerType = 0x02,                               // Multicast
//                                  .flag = HOMEID_FILTER_FLAG | SOURCE_NODE_ID_FILTER_FLAG | PAYLOAD_INDEX_1_FILTER_FLAG | PAYLOAD_INDEX_2_FILTER_FLAG,
//                                  .frameHandler = testFilter4Callback};
//
//  mp_zpal_received_frame->frameContentLength = sizeof(cmdMulticastFrame);
//  memcpy(mp_zpal_received_frame->frameContent, cmdMulticastFrame, mp_zpal_received_frame->frameContentLength);
//      radioProfile.receive_handler_cb(&rxParameters, mp_zpal_received_frame);
//  TEST_ASSERT_FALSE(mTestFilter0Called | mTestFilter1Called | mTestFilter2Called | mTestFilter3Called | mTestFilter4Called);
//
//  // Start adding the gloabal multicast filter
//  actualVal = llReceiveFilterAdd(&rxFilter0);
//  TEST_ASSERT_EQUAL(SUCCESS, actualVal);
//
//  // Home ID only (No match)
//  actualVal = llReceiveFilterAdd(&rxFilter1);
//  TEST_ASSERT_EQUAL(SUCCESS, actualVal);
//
//      radioProfile.receive_handler_cb(&rxParameters, mp_zpal_received_frame);
//  TEST_ASSERT_FALSE(mTestFilter1Called | mTestFilter2Called | mTestFilter3Called | mTestFilter4Called | mTestFilter5Called);
//  TEST_ASSERT_TRUE(mTestFilter0Called)
//  mTestFilter0Called = false;
//
//  // Home ID and source (No match)
//  actualVal = llReceiveFilterAdd(&rxFilter3);
//  TEST_ASSERT_EQUAL(SUCCESS, actualVal);
//
//      radioProfile.receive_handler_cb(&rxParameters, mp_zpal_received_frame);
//  TEST_ASSERT_FALSE(mTestFilter1Called | mTestFilter2Called | mTestFilter3Called | mTestFilter4Called | mTestFilter5Called);
//  TEST_ASSERT_TRUE(mTestFilter0Called)
//  mTestFilter0Called = false;
//
//  // Home ID, source, and payload (No match)
//  actualVal = llReceiveFilterAdd(&rxFilter4);
//  TEST_ASSERT_EQUAL(SUCCESS, actualVal);
//
//      radioProfile.receive_handler_cb(&rxParameters, mp_zpal_received_frame);
//  TEST_ASSERT_FALSE(mTestFilter1Called | mTestFilter2Called | mTestFilter3Called | mTestFilter4Called | mTestFilter5Called);
//  TEST_ASSERT_TRUE(mTestFilter0Called)
//  mTestFilter0Called = false;
//
//  // Home ID, source, payload 1 anbd 2 (No match)
//  actualVal = llReceiveFilterAdd(&rxFilter5);
//  TEST_ASSERT_EQUAL(SUCCESS, actualVal);
//
//      radioProfile.receive_handler_cb(&rxParameters, mp_zpal_received_frame);
//  TEST_ASSERT_FALSE(mTestFilter1Called | mTestFilter2Called | mTestFilter3Called | mTestFilter4Called | mTestFilter5Called);
//  TEST_ASSERT_TRUE(mTestFilter0Called)
//  mTestFilter0Called = false;
//
//#ifdef BACKWARD_COMPABILITY_TEST
//  TEST_ASSERT_EQUAL(41, mTestFilter0ReceivedFrame.frameContentLength);
//  TEST_ASSERT_EQUAL(0, memcmp(&mTestFilter0ReceivedFrame.frameContent[9], &cmdMulticastFrame[9], mTestFilter0ReceivedFrame.frameContentLength - 9));
//#else  // BACKWARD_COMPABILITY_TEST
//  TEST_ASSERT_EQUAL(32, mTestFilter0ReceivedFrame.frameContentLength);
//  TEST_ASSERT_EQUAL(0, memcmp(mTestFilter0ReceivedFrame.frameContent, &cmdMulticastFrame[9], mTestFilter0ReceivedFrame.frameContentLength));
//#endif // BACKWARD_COMPABILITY_TEST
//}
//
///** Verification that multicast frame receive handler does not support filters with destination matching.
// *
// */
//void test_receive_add_multicast_destination_filter_fail(void)
//{
//  /** As correct radio initialization is ensured in other tests, we just utilize the helper function in this test. */
//  helper_func_expect_radio_init_eu(0x06);
//
//  zpal_radio_rx_parameters_t rxParameters = {.speed = ZPAL_RADIO_SPEED_40K,// Speed 9.6K=1000, Speed 40K=0100, Speed 100K=0010.
//                                   .channelId = 1,
//                                   .rssi = 0};
//
//  ZW_ReturnCode_t radioInitReturnValueExpected = SUCCESS;
//  ZW_ReturnCode_t actualVal;
//
//  zpal_radio_profile_t radioProfile = { REGION_EU, BAUD_RATE_9_6k_40k | BAUD_RATE_100k, ZPAL_RADIO_WAKEUP_ALWAYS_LISTEN };
//  actualVal = llInit(&radioProfile);
//  TEST_ASSERT_EQUAL(radioInitReturnValueExpected, actualVal);
//
//  ZW_ReceiveFilter_t rxFilter0 = {.homeId = {{0xca, 0xfe, 0xba, 0xbf}},
//                                  .destinationNodeId = 5,
//                                  .headerType = 0x02,                                 // Multicast
//                                  .flag = HOMEID_FILTER_FLAG | DESTINATION_NODE_ID_FILTER_FLAG,
//                                  .frameHandler = testFilter0Callback};
//
//  actualVal = llReceiveFilterAdd(&rxFilter0);
//  TEST_ASSERT_EQUAL(INVALID_PARAMETERS, actualVal);
//}
//
///** Verification that the Data Link Layer can filter a frame and discard based on each flag type.
// *
// */
//void test_filter_compare(void)
//{
//  /** As correct radio initialization is ensured in other tests, we just utilize the helper function in this test. */
//  helper_func_expect_radio_init_eu(0x06);
//
//  zpal_radio_rx_parameters_t rxParameters = {.speed = ZPAL_RADIO_SPEED_40K,// Speed 9.6K=1000, Speed 40K=0100, Speed 100K=0010.
//                                   .channelId = 1,
//                                   .rssi = 0};
//
//  ZW_ReturnCode_t radioInitReturnValueExpected = SUCCESS;
//  ZW_ReturnCode_t actualVal;
//
//  zpal_radio_profile_t radioProfile = { REGION_EU, ZPAL_RADIO_WAKEUP_ALWAYS_LISTEN };
//  actualVal = llInit(&radioProfile);
//  TEST_ASSERT_EQUAL(radioInitReturnValueExpected, actualVal);
//
//  // Let's ensure all padding are different, so unused/padded fields will fail comparison, if they are compared by mistake.
//  ZW_ReceiveFilter_t rxFilter0;
//  memset(&rxFilter0, 0x55, sizeof(rxFilter0));
//
//  ZW_ReceiveFilter_t rxFilter1;
//  memset(&rxFilter1, 0xAA, sizeof(rxFilter1));
//
//  // Set home id and add/remove the filter to trigger comparison.
//  rxFilter0.headerType = 0x01;
//  rxFilter0.flag       = 0;
//  rxFilter0.frameHandler = NULL;
//
//  // Start adding the filter
//  actualVal = llReceiveFilterAdd(&rxFilter0);
//  TEST_ASSERT_EQUAL(SUCCESS, actualVal);
//
//  // Try removing using another struct not valid.
//  actualVal = llReceiveFilterRemove(&rxFilter1);
//  TEST_ASSERT_EQUAL(INVALID_PARAMETERS, actualVal);
//
//  rxFilter1.headerType = 0x05; // Valid but wrong header
//  actualVal = llReceiveFilterRemove(&rxFilter1);
//  TEST_ASSERT_EQUAL(INVALID_PARAMETERS, actualVal);
//
//  // Match the other filter.
//  rxFilter1.headerType = 0x01;
//  rxFilter1.flag       = 0;
//  rxFilter1.frameHandler = NULL;
//
//  actualVal = llReceiveFilterRemove(&rxFilter1);
//  TEST_ASSERT_EQUAL(SUCCESS, actualVal);
//
//  // Update with home id
//  rxFilter0.homeId.word = 0xCAFEBABF;
//  rxFilter0.flag        = HOMEID_FILTER_FLAG;
//  rxFilter1.flag        = HOMEID_FILTER_FLAG;
//
//  actualVal = llReceiveFilterAdd(&rxFilter0);
//  TEST_ASSERT_EQUAL(SUCCESS, actualVal);
//
//  // Try removing using another struct not valid.
//  actualVal = llReceiveFilterRemove(&rxFilter1);
//  TEST_ASSERT_EQUAL(INVALID_PARAMETERS, actualVal);
//
//  // Update the other filter.
//  rxFilter1.homeId.word = 0xCAFEBABF;
//
//  actualVal = llReceiveFilterRemove(&rxFilter1);
//  TEST_ASSERT_EQUAL(SUCCESS, actualVal);
//
//  // Update with Destination id
//  rxFilter0.destinationNodeId  = 1;
//  rxFilter0.flag              |= DESTINATION_NODE_ID_FILTER_FLAG;
//  rxFilter1.flag              |= DESTINATION_NODE_ID_FILTER_FLAG;
//
//  actualVal = llReceiveFilterAdd(&rxFilter0);
//  TEST_ASSERT_EQUAL(SUCCESS, actualVal);
//
//  // Try removing using another struct not valid.
//  actualVal = llReceiveFilterRemove(&rxFilter1);
//  TEST_ASSERT_EQUAL(INVALID_PARAMETERS, actualVal);
//
//  // Update the other filter.
//  rxFilter1.destinationNodeId  = 1;
//
//  actualVal = llReceiveFilterRemove(&rxFilter1);
//  TEST_ASSERT_EQUAL(SUCCESS, actualVal);
//
//  // Update with Source id
//  rxFilter0.sourceNodeId  = 1;
//  rxFilter0.flag         |= SOURCE_NODE_ID_FILTER_FLAG;
//  rxFilter1.flag         |= SOURCE_NODE_ID_FILTER_FLAG;
//
//  actualVal = llReceiveFilterAdd(&rxFilter0);
//  TEST_ASSERT_EQUAL(SUCCESS, actualVal);
//
//  // Try removing using another struct not valid.
//  actualVal = llReceiveFilterRemove(&rxFilter1);
//  TEST_ASSERT_EQUAL(INVALID_PARAMETERS, actualVal);
//
//  // Update the other filter.
//  rxFilter1.sourceNodeId  = 1;
//
//  actualVal = llReceiveFilterRemove(&rxFilter1);
//  TEST_ASSERT_EQUAL(SUCCESS, actualVal);
//
//  // Update with payload 1
//  rxFilter0.payloadIndex1       = 1;
//  rxFilter0.payloadFilterValue1 = 0x45;
//  rxFilter0.flag               |= PAYLOAD_INDEX_1_FILTER_FLAG;
//  rxFilter1.flag               |= PAYLOAD_INDEX_1_FILTER_FLAG;
//
//  actualVal = llReceiveFilterAdd(&rxFilter0);
//  TEST_ASSERT_EQUAL(SUCCESS, actualVal);
//
//  // Try removing using another struct not valid.
//  actualVal = llReceiveFilterRemove(&rxFilter1);
//  TEST_ASSERT_EQUAL(INVALID_PARAMETERS, actualVal);
//
//  // Update the other filter on payload index 1.
//  rxFilter1.payloadIndex1       = 1;
//
//  actualVal = llReceiveFilterRemove(&rxFilter1);
//  TEST_ASSERT_EQUAL(INVALID_PARAMETERS, actualVal);
//
//  // Update the other filter on payload value 1.
//  rxFilter1.payloadFilterValue1 = 0x45;
//
//  actualVal = llReceiveFilterRemove(&rxFilter1);
//  TEST_ASSERT_EQUAL(SUCCESS, actualVal);
//
//  // Update with payload 2
//  rxFilter0.payloadIndex2       = 5;
//  rxFilter0.payloadFilterValue2 = 0x23;
//  rxFilter0.flag               |= PAYLOAD_INDEX_2_FILTER_FLAG;
//  rxFilter1.flag               |= PAYLOAD_INDEX_2_FILTER_FLAG;
//
//  actualVal = llReceiveFilterAdd(&rxFilter0);
//  TEST_ASSERT_EQUAL(SUCCESS, actualVal);
//
//  // Try removing using another struct not valid.
//  actualVal = llReceiveFilterRemove(&rxFilter1);
//  TEST_ASSERT_EQUAL(INVALID_PARAMETERS, actualVal);
//
//  // Update the other filter on payload index 2.
//  rxFilter1.payloadIndex2       = 5;
//
//  actualVal = llReceiveFilterRemove(&rxFilter1);
//  TEST_ASSERT_EQUAL(INVALID_PARAMETERS, actualVal);
//
//  // Update the other filter on payload value 2.
//  rxFilter1.payloadFilterValue2 = 0x23;
//
//  actualVal = llReceiveFilterRemove(&rxFilter1);
//  TEST_ASSERT_EQUAL(SUCCESS, actualVal);
//
//  // Update with frame handler
//  rxFilter0.frameHandler = testFilter0Callback;
//
//  actualVal = llReceiveFilterAdd(&rxFilter0);
//  TEST_ASSERT_EQUAL(SUCCESS, actualVal);
//
//  // Try removing using another struct not valid.
//  actualVal = llReceiveFilterRemove(&rxFilter1);
//  TEST_ASSERT_EQUAL(INVALID_PARAMETERS, actualVal);
//
//  // Update the other filter.
//  rxFilter1.frameHandler = testFilter0Callback;
//
//  actualVal = llReceiveFilterRemove(&rxFilter1);
//  TEST_ASSERT_EQUAL(SUCCESS, actualVal);
//}
//
///** Verification that unsupported frame types returns INVALID_PARAMETERS when filter is added.
// *  Currently unsupported types:
// *  - HDRTYP_MULTICAST
// *  - HDRTYP_FLOODED
// *  - HDRTYP_ROUTED
// *
// */
//void test_filter_add_invalid_frame_types(void)
//{
//  /** As correct radio initialization is ensured in other tests, we just utilize the helper function in this test. */
//  helper_func_expect_radio_init_eu(0x06);
//
//  zpal_radio_rx_parameters_t rxParameters = {.speed = ZPAL_RADIO_SPEED_40K,// Speed 9.6K=1000, Speed 40K=0100, Speed 100K=0010.
//                                   .channelId = 1,
//                                   .rssi = 0};
//
//  ZW_ReturnCode_t radioInitReturnValueExpected = SUCCESS;
//  ZW_ReturnCode_t actualVal;
//
//  zpal_radio_profile_t radioProfile = { REGION_EU, ZPAL_RADIO_WAKEUP_ALWAYS_LISTEN };
//  actualVal = llInit(&radioProfile);
//  TEST_ASSERT_EQUAL(radioInitReturnValueExpected, actualVal);
//
//  ZW_ReceiveFilter_t rxFilter1 = {.headerType = 4, // Flooded
//                                  .flag = 0,
//                                  .frameHandler = testFilter1Callback};
//
//  ZW_ReceiveFilter_t rxFilter2 = {.headerType = 8, // Routed
//                                  .flag = 0,
//                                  .frameHandler = testFilter2Callback};
//
//  ZW_ReceiveFilter_t rxFilter3 = {.headerType = 10, // Dummy
//                                  .flag = 0,
//                                  .frameHandler = testFilter3Callback};
//
//  actualVal = llReceiveFilterAdd(&rxFilter1);
//  TEST_ASSERT_EQUAL(INVALID_PARAMETERS, actualVal);
//
//  actualVal = llReceiveFilterAdd(&rxFilter2);
//  TEST_ASSERT_EQUAL(INVALID_PARAMETERS, actualVal);
//
//  actualVal = llReceiveFilterAdd(&rxFilter3);
//  TEST_ASSERT_EQUAL(INVALID_PARAMETERS, actualVal);
//}
