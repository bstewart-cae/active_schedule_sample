// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file TestTransmitAckLinkLayer_3ch.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include <unity.h>
#include <mock_control.h>
//#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "ZW_DataLinkLayer.h"
#include <zpal_radio.h>

void setUpSuite(void) {

}

void tearDownSuite(void) {

}

/** Missing test overview for single cast:
 * - Speed modified, test that if a frame is sent at a lower speed than supported by the nodes, then speed modified value is set.
 * - 3 Channel tests
 * -
 * -
 */

void setUp(void)
{
  //This is run before EACH TEST
  mock_calls_clear();
}

void tearDown(void)
{
  mock_calls_verify();
}


void helper_func_expect_radio_init_jp(zpal_radio_profile_t *pRfProfile, mock_t * p_mock)
{
  // Setup of test expectations. When configuration the Data Link Layer, the following calls are expected further down to Phy Layer.
  // 1) ZW_RadioPhyIni, with configuration of 3 channels for 9.6k, 40k, 100k. And a Energy Harvesting channel (still to be defined)
  mock_call_expect(TO_STR(zpal_radio_init), &p_mock);
  p_mock->expect_arg[0].pointer = pRfProfile;
  p_mock->compare_rule_arg[1]   = COMPARE_NULL;
  p_mock->compare_rule_arg[2]   = COMPARE_NULL;
  p_mock->compare_rule_arg[3]   = COMPARE_NULL;
}

/** Verification that when transmitting a frame with a given communication profile then the frame is passed to the phy layer with correct CRC.
 *
 * Link Layer knows nothing about ACK frame, only frame header.
 * Thus it is expected that upper layer has encoded explorer header correctly, and link layer only prepends the real header.
 *
 * This test will set up the node to support JP @ 100k spped on all channels.
 */
void test_to_comm_profile_always_listen_and_transmit_ack_frame_jp(void)
{
  mock_t* p_mock = NULL;

  /** When transmitting a frame, it is expected that a call ZW_RadioTransmit to phy layer will occur.
   *  This test ensures that a frame can be correctly transmitted @100k speed on all channels as that is what the remote node supports.
   *  Self node supports all speed variants.
   *  This test receive and transmits the first part of an inclusion, i.e.
   *  1) Transmit ack frame
   */

  zpal_radio_transmit_parameter_t txParameters100k_ch0 = {.speed = ZPAL_RADIO_SPEED_100K,// Speed 9.6K=1000, Speed 40K=0100, Speed 100K=0010.
                                           .channel_id = 0,        // Channel0=0x00, Channel1=0x01, Channel2=0x02.
                                           .crc = 0x02,           // CRC 8bit=1, CRC 16bit=2
                                           .preamble_length = 24,  // 40 bytes preamble on 100K
                                           .repeats = 0};         // No repeats (No wakeup beam)

// linker layer don't support multi channel yet
//  zpal_radio_transmit_parameter_t txParameters100k_ch1 = {.speed = ZPAL_RADIO_SPEED_100K,// Speed 9.6K=1000, Speed 40K=0100, Speed 100K=0010.
//                                           .channelId = 1,        // Channel0=0x00, Channel1=0x01, Channel2=0x02.
//                                           .crc = 0x02,           // CRC 8bit=1, CRC 16bit=2
//                                           .preamble_length = 24,  // 40 bytes preamble on 100K
//                                           .repeats = 0};         // No repeats (No wakeup beam)
//
//  zpal_radio_transmit_parameter_t txParameters100k_ch2 = {.speed = ZPAL_RADIO_SPEED_100K,// Speed 9.6K=1000, Speed 40K=0100, Speed 100K=0010.
//                                           .channelId = 2,        // Channel0=0x00, Channel1=0x01, Channel2=0x02.
//                                           .crc = 0x02,           // CRC 8bit=1, CRC 16bit=2
//                                           .preamble_length = 24,  // 40 bytes preamble on 100K
//                                           .repeats = 0};         // No repeats (No wakeup beam)


  uint8_t          cmdAckFrameHeaderLength_100k = 10; // The Ack Frame as it is expected to be seen on the phy layer.
  uint8_t          cmdAckFramePayloadLength_100k = 0;
  uint8_t          cmdAckFullFrame_100k[]     = {0xFD, 0x39, 0x34, 0xB0, 0x06, 0x03, 0x00, 0x0C, 0x6A, 0x01, // General Header
                                                 0x58, 0xFE};                                                // CRC
  // Ack frame@100k
  // test channel 0
  mock_call_expect(TO_STR(zpal_radio_transmit), &p_mock);
  p_mock->expect_arg[0].pointer = &txParameters100k_ch0;
  p_mock->expect_arg[1].value   = cmdAckFrameHeaderLength_100k;
  p_mock->expect_arg[2].pointer = &cmdAckFullFrame_100k[0];
  p_mock->expect_arg[3].value   = cmdAckFramePayloadLength_100k;
  p_mock->expect_arg[4].pointer = &cmdAckFullFrame_100k[cmdAckFrameHeaderLength_100k];
  p_mock->expect_arg[5].value   = 0;

  /***** Execute the test *****/

  uint8_t          ackFrameLength = 0;
  uint8_t          ackFrame[]     = {};

  uint8_t      frameBuffer[sizeof(ZW_TransmissionFrame_t) + 1] = {0, };
  ZW_TransmissionFrame_t * pTestFrame = (ZW_TransmissionFrame_t *)frameBuffer;

  ZW_ReturnCode_t radioInitReturnValueExpected = SUCCESS;
  ZW_ReturnCode_t actualVal;

  zpal_radio_profile_t radioProfile = { REGION_JP, ZPAL_RADIO_WAKEUP_ALWAYS_LISTEN };
  /** As correct radio initialization is ensured in other tests, we just utilize the helper function in this test. */
  helper_func_expect_radio_init_jp(&radioProfile, p_mock);
  actualVal = llInit(&radioProfile);
  TEST_ASSERT_EQUAL(radioInitReturnValueExpected, actualVal);

  CommunicationProfile_t communicationProfile;

  // First transmission of a transfer presentation on 100k
  pTestFrame->frameOptions.frameType = HDRTYP_TRANSFERACK;
  pTestFrame->frameOptions.homeId[0] = 0xFD;
  pTestFrame->frameOptions.homeId[1] = 0x39;
  pTestFrame->frameOptions.homeId[2] = 0x34;
  pTestFrame->frameOptions.homeId[3] = 0xB0;
  pTestFrame->frameOptions.sourceNodeId      = 0x06;
  pTestFrame->frameOptions.destinationNodeId = 1;
  pTestFrame->frameOptions.sequenceNumber    = 0x6A;
  pTestFrame->headerLength = cmdAckFrameHeaderLength_100k;
  pTestFrame->payloadLength = ackFrameLength;
  memcpy(pTestFrame->payload, ackFrame, pTestFrame->payloadLength);

  // Transmit using 100k.
  communicationProfile = RF_PROFILE_3CH_100K_CH_A;

  // test channel 0
  actualVal = llTransmitFrame(communicationProfile, pTestFrame);
  TEST_ASSERT_EQUAL(SUCCESS, actualVal);

//  actualVal = llTransmitFrame(communicationProfile, pTestFrame);
//  TEST_ASSERT_EQUAL(SUCCESS, actualVal);
//
//  // test channel 2
//  actualVal = llTransmitFrame(communicationProfile, pTestFrame);
//  TEST_ASSERT_EQUAL(SUCCESS, actualVal);

}

/**
 * This test will set up the node to support JP @ 100kon all channels.
 */
void test_to_comm_profile_null_pointer_ack_frame_jp(void)
{
  mock_t* p_mock = NULL;

  /** Ensure that in case a pFrame is a null pointer, then the link layer return invalid parameters as return code.
   */

  ZW_ReturnCode_t radioInitReturnValueExpected = SUCCESS;
  ZW_ReturnCode_t actualVal;

  zpal_radio_profile_t radioProfile = { REGION_JP, ZPAL_RADIO_WAKEUP_ALWAYS_LISTEN };
  /** As correct radio initialization is ensured in other tests, we just utilize the helper function in this test. */
  helper_func_expect_radio_init_jp(&radioProfile, p_mock);
  actualVal = llInit(&radioProfile);
  TEST_ASSERT_EQUAL(radioInitReturnValueExpected, actualVal);

  CommunicationProfile_t communicationProfile = RF_PROFILE_100K;

  actualVal = llTransmitFrame(0, NULL);
  TEST_ASSERT_EQUAL(UNSUPPORTED, actualVal);

  actualVal = llTransmitFrame(communicationProfile, NULL);
  TEST_ASSERT_EQUAL(INVALID_PARAMETERS, actualVal);
}

