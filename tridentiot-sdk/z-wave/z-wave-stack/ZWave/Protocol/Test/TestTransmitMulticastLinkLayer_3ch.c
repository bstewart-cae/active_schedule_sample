// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file TestTransmitMulticastLinkLayer_3ch.c
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

/** Verification that when transmitting a frame with a given communication profile then the frame is passed to the phy layer with correct CRC.
 *
 * Link Layer knows nothing about multicast frame frame, only frame header.
 * Thus it is expected that upper layer has encoded explorer header correctly, and link layer only prepends the real header.
 *
 * This test will set up the node to support JP @ 100k.
 */
void test_profile_always_listen_and_transmit_multicast_frame_jp(void)
{
  /***** Expectation setup *****/


  /** When transmitting a frame, it is expected that a call ZW_RadioTransmit to phy layer will occur.
   *  This test ensures that a frame can be correctly transmitted @100k .
   *  This test receive and transmits the first device reset locally
   */
  mock_t * p_mock = NULL;

  zpal_radio_transmit_parameter_t txParameters100k = {.speed = ZPAL_RADIO_SPEED_100K,// Speed 9.6K=1000, Speed 40K=0100, Speed 100K=0010.
                                       .channel_id = 0,        // Channel0=0x00, Channel1=0x01, Channel2=0x02.
                                       .crc = 0x02,           // CRC 8bit=1, CRC 16bit=2
                                       .preamble_length = 24,  // 24 bytes preamble on 100K
                                       .repeats = 0};         // No repeats (No wakeup beam)


  uint8_t          cmdMulticastFrameHeaderLength_100k = 39;
  uint8_t          cmdMulticastFramePayloadLength_100k = 2;
  uint8_t          cmdMulticastFrame_100k[]     = {0xE3, 0x72, 0x83, 0xAF, 0x02, 0x02, 0x00, 0x2B, 0x03, // General Header.
                                                   0x1D, 0x0D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                                   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                                   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                                   0x00, 0x00, 0x00, // Multicast frame, Address offset + Mask bytes, Mask byte. (Node: 1, 3, 4)
                                                   0x5A, 0x01, // Payload, Device Reset Locally
                                                   0x7E, 0xB2};// CRC-CCITT

  uint8_t          cmdMulticastExtendedFrameHeaderLength_100k = 46;
  uint8_t          cmdMulticastExtendedFramePayloadLength_100k = 2;
  uint8_t          cmdMulticastExtendedFrame_100k[]     = {0xE3, 0x72, 0x83, 0xAF, 0x02, 0x02, 0x80, 0x32, 0x03, // General Header.
                                                           0x1D, 0x0D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                                           0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                                           0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                                           0x00, 0x00, 0x00,             // Multicast frame, Address offset + Mask bytes, Mask byte. (Node: 1, 3, 4)
                                                           0x07, 0xde, 0xad, 0xbe, 0xef, 0x42, 0x24,  // extended header
                                                           0x5A, 0x01,                   // Payload, Device Reset Locally
                                                           0x7C, 0x05};                  // CRC-CCITT


  // Ack frame@100k
  mock_call_expect(TO_STR(zpal_radio_transmit), &p_mock);
  p_mock->expect_arg[0].pointer = &txParameters100k;
  p_mock->expect_arg[1].value   = cmdMulticastFrameHeaderLength_100k;
  p_mock->expect_arg[2].pointer = &cmdMulticastFrame_100k[0];
  p_mock->expect_arg[3].value   = cmdMulticastFramePayloadLength_100k;
  p_mock->expect_arg[4].pointer = &cmdMulticastFrame_100k[cmdMulticastFrameHeaderLength_100k];
  p_mock->expect_arg[5].value   = 1;


  mock_call_expect(TO_STR(zpal_radio_transmit), &p_mock);
  p_mock->expect_arg[0].pointer = &txParameters100k;
  p_mock->expect_arg[1].value   = cmdMulticastExtendedFrameHeaderLength_100k;
  p_mock->expect_arg[2].pointer = &cmdMulticastExtendedFrame_100k[0];
  p_mock->expect_arg[3].value   = cmdMulticastExtendedFramePayloadLength_100k;
  p_mock->expect_arg[4].pointer = &cmdMulticastExtendedFrame_100k[cmdMulticastExtendedFrameHeaderLength_100k];
  p_mock->expect_arg[5].value   = 1;

  /***** Execute the test *****/

  uint8_t          multicastFrameLength = 2;
  uint8_t          multicastFrame[]     = {0x5A, 0x01};

  uint8_t      frameBuffer[sizeof(ZW_TransmissionFrame_t) + 2 + 2] = {0, };
  ZW_TransmissionFrame_t * pTestFrame = (ZW_TransmissionFrame_t *)frameBuffer;

  ZW_ReturnCode_t radioInitReturnValueExpected = SUCCESS;
  ZW_ReturnCode_t actualVal;

  zpal_radio_profile_t radioProfile = { .region = REGION_JP,
                                        .wakeup = ZPAL_RADIO_WAKEUP_ALWAYS_LISTEN,
  };
  /** As correct radio initialization is ensured in other tests, we just utilize the helper function in this test. */
  helper_func_expect_radio_init_jp(&radioProfile, p_mock);
  actualVal = llInit(&radioProfile);
  TEST_ASSERT_EQUAL(radioInitReturnValueExpected, actualVal);

  CommunicationProfile_t communicationProfile;
  pTestFrame->useLBT=1;
  // First transmission of a transfer presentation on 40k
  pTestFrame->frameOptions.frameType = HDRTYP_MULTICAST;
  pTestFrame->frameOptions.homeId[0] = 0xE3;
  pTestFrame->frameOptions.homeId[1] = 0x72;
  pTestFrame->frameOptions.homeId[2] = 0x83;
  pTestFrame->frameOptions.homeId[3] = 0xAF;
  pTestFrame->frameOptions.sourceNodeId      = 2;
  pTestFrame->frameOptions.sequenceNumber    = 0x3;

  // Multicast module is expected to build the multicast header. Not the Link Layer.
  uint8_t multicastHeader[] = {0x1D, 0x0D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Address offset (0) + Number of mask bytes (0x1D = 29), Thereaftter the mask bytes for node: 1, 3, 4
                               0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                               0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

  pTestFrame->headerLength = cmdMulticastFrameHeaderLength_100k;
  memcpy((uint8_t*)&pTestFrame->header, multicastHeader, sizeof(multicastHeader));
  pTestFrame->payloadLength = multicastFrameLength;
  memcpy(pTestFrame->payload, multicastFrame, pTestFrame->payloadLength);
  // Transmit using 40k.
  communicationProfile = RF_PROFILE_3CH_100K_CH_A;
  actualVal = llTransmitFrame(communicationProfile, pTestFrame);
  TEST_ASSERT_EQUAL(SUCCESS, actualVal);

  // Second transmission of a transfer presentation on 100k (sequence number incremented)
  pTestFrame->frameOptions.frameType = HDRTYP_MULTICAST;
  pTestFrame->frameOptions.homeId[0] = 0xE3;
  pTestFrame->frameOptions.homeId[1] = 0x72;
  pTestFrame->frameOptions.homeId[2] = 0x83;
  pTestFrame->frameOptions.homeId[3] = 0xAF;
  pTestFrame->frameOptions.sourceNodeId      = 2;
  pTestFrame->frameOptions.extended = 1;
  pTestFrame->frameOptions.sequenceNumber    = 3;

  // Multicast module is expected to build the multicast header. Not the Link Layer.
  uint8_t multicastExtendedHeader[] = {0x1D, 0x0D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Address offset (0) + Number of mask bytes (0x1D = 29), Thereaftter the mask bytes for node: 1, 3, 4
                                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                       0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                       0x07, 0xde, 0xad, 0xbe, 0xef, 0x42, 0x24};

  pTestFrame->headerLength = cmdMulticastExtendedFrameHeaderLength_100k;
  memcpy((uint8_t*)&pTestFrame->header, multicastExtendedHeader, sizeof(multicastExtendedHeader));
  pTestFrame->payloadLength = multicastFrameLength;
  memcpy(pTestFrame->payload, multicastFrame, pTestFrame->payloadLength);

  // Transmit using 100k.
  communicationProfile = RF_PROFILE_3CH_100K;
  actualVal = llTransmitFrame(communicationProfile, pTestFrame);
  TEST_ASSERT_EQUAL(SUCCESS, actualVal);

}

/**
 * This test will set up the node to support EU @ 9.6k/40k/100k and the capabilities of remote node is just 9.6k / 40k.
 */
void test_speed_to_comm_profile_null_pointer_ack_frame_jp(void)
{
  /***** Expectation setup *****/
  mock_t* p_mock = NULL;

  /** Ensure that in case a pFrame is a null pointer, then the link layer return invalid parameters as return code.
   */

  ZW_ReturnCode_t radioInitReturnValueExpected = SUCCESS;
  ZW_ReturnCode_t actualVal;

  zpal_radio_profile_t radioProfile = { .region = REGION_JP,
                                        .wakeup = ZPAL_RADIO_WAKEUP_ALWAYS_LISTEN,
  };
  /** As correct radio initialization is ensured in other tests, we just utilize the helper function in this test. */
  helper_func_expect_radio_init_jp(&radioProfile, p_mock);
  actualVal = llInit(&radioProfile);
  TEST_ASSERT_EQUAL(radioInitReturnValueExpected, actualVal);

  CommunicationProfile_t communicationProfile = RF_PROFILE_3CH_100K;

  actualVal = llTransmitFrame(0, NULL);
  TEST_ASSERT_EQUAL(UNSUPPORTED, actualVal);

  actualVal = llTransmitFrame(communicationProfile, NULL);
  TEST_ASSERT_EQUAL(INVALID_PARAMETERS, actualVal);
}

