// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file TestTransmitMulticastLinkLayer.c
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


void helper_func_expect_radio_init_eu (zpal_radio_profile_t *pRfProfile, mock_t *p_mock)
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
 * This test will set up the node to support EU @ 9.6k/40k/100k and the capabilities of remote node is just 9.6k / 40k.
 */
void test_speed_to_comm_profile_always_listen_and_transmit_multicast_frame_eu(void)
{

  /***** Expectation setup *****/


  /** When transmitting a frame, it is expected that a call ZW_RadioTransmit to phy layer will occur.
   *  This test ensures that a frame can be correctly transmitted @9.6k and @40 speed as that is what the remote node supports.
   *  Self node supports all speed variants.
   *  This test receive and transmits the first part of an inclusion, i.e.
   *  1) Transmit a Cmd Set Nwi Mode (Explorer Normal)
   */
  mock_t * p_mock = NULL;
  zpal_radio_transmit_parameter_t txParameters40k = {.speed = ZPAL_RADIO_SPEED_40K,// Speed 9.6K=1000, Speed 40K=0100, Speed 100K=0010.
                                      .channel_id = 1,       // Channel0=0x00, Channel1=0x01, Channel2=0x02.
                                      .crc = 0x01,          // CRC 8bit=1, CRC 16bit=2
                                      .preamble_length = 20, // 20 bytes preamble on 40K
                                      .repeats = 0};        // No repeats (No wakeup beam)

  zpal_radio_transmit_parameter_t txParameters100k = {.speed = ZPAL_RADIO_SPEED_100K,// Speed 9.6K=1000, Speed 40K=0100, Speed 100K=0010.
                                       .channel_id = 0,        // Channel0=0x00, Channel1=0x01, Channel2=0x02.
                                       .crc = 0x02,           // CRC 8bit=1, CRC 16bit=2
                                       .preamble_length = 40,  // 40 bytes preamble on 100K
                                       .repeats = 0};         // No repeats (No wakeup beam)

  uint8_t          cmdMulticastFrameHeaderLength_40k = 38; // The Multicast Frame as it is expected to be seen on the phy layer.
  uint8_t          cmdMulticastFramePayloadLength_40k = 2;
  uint8_t          cmdMulticastFrame_40k[]     = {0xE3, 0x72, 0x83, 0xAF, 0x02, 0x02, 0x01, 0x29, // General Header
                                                  0x1D, 0x0D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Multicast frame, Address offset + Mask bytes, Mask byte. (Node: 1, 3, 4)
                                                  0x5A, 0x01, // Payload, Device Reset Locally
                                                  0x21};      // CRC

  uint8_t          cmdMulticastFrameHeaderLength_100k = 38; // The Multicast Frame as it is expected to be seen on the phy layer.
  uint8_t          cmdMulticastFramePayloadLength_100k = 2;
  uint8_t          cmdMulticastFrame_100k[]     = {0xE3, 0x72, 0x83, 0xAF, 0x02, 0x02, 0x03, 0x2A, // General Header
                                                   0x1D, 0x0D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // Multicast frame, Address offset + Mask bytes, Mask byte. (Node: 1, 3, 4)
                                                   0x5A, 0x01, // Payload, Device Reset Locally
                                                   0x20, 0x39};// CRC-CCITT

  // Ack frame@40k
  mock_call_expect(TO_STR(zpal_radio_transmit), &p_mock);
  p_mock->expect_arg[0].pointer = &txParameters40k;
  p_mock->expect_arg[1].value   = cmdMulticastFrameHeaderLength_40k;
  p_mock->expect_arg[2].pointer = &cmdMulticastFrame_40k[0];
  p_mock->expect_arg[3].value   = cmdMulticastFramePayloadLength_40k;
  p_mock->expect_arg[4].pointer = &cmdMulticastFrame_40k[cmdMulticastFrameHeaderLength_40k];
  p_mock->expect_arg[5].value   = 1;

  // Ack frame@100k
  mock_call_expect(TO_STR(zpal_radio_transmit), &p_mock);
  p_mock->expect_arg[0].pointer = &txParameters100k;
  p_mock->expect_arg[1].value   = cmdMulticastFrameHeaderLength_100k;
  p_mock->expect_arg[2].pointer = &cmdMulticastFrame_100k[0];
  p_mock->expect_arg[3].value   = cmdMulticastFramePayloadLength_100k;
  p_mock->expect_arg[4].pointer = &cmdMulticastFrame_100k[cmdMulticastFrameHeaderLength_100k];
  p_mock->expect_arg[5].value   = 1;

  /***** Executed the test *****/

  uint8_t          multicastFrameLength = 2;
  uint8_t          multicastFrame[]     = {0x5A, 0x01};

// Multicast module is expected to build the multicast header. Not the Link Layer.

  uint8_t      frameBuffer[sizeof(ZW_TransmissionFrame_t) + 2 + 2] = {0, };
  ZW_TransmissionFrame_t * pTestFrame = (ZW_TransmissionFrame_t *)frameBuffer;

  ZW_ReturnCode_t radioInitReturnValueExpected = SUCCESS;
  ZW_ReturnCode_t actualVal;

  zpal_radio_profile_t radioProfile = { .region = REGION_EU,
                                        .wakeup = ZPAL_RADIO_WAKEUP_ALWAYS_LISTEN,
  };
  /** As correct radio initialization is ensured in other tests, we just utilize the helper function in this test. */
  helper_func_expect_radio_init_eu(&radioProfile, p_mock);
    actualVal = llInit(&radioProfile);
  TEST_ASSERT_EQUAL(radioInitReturnValueExpected, actualVal);

  CommunicationProfile_t communicationProfile;

  // First transmission of a transfer presentation on 40k
  pTestFrame->useLBT=1;
  pTestFrame->frameOptions.frameType = HDRTYP_MULTICAST;
  pTestFrame->frameOptions.homeId[0] = 0xE3;
  pTestFrame->frameOptions.homeId[1] = 0x72;
  pTestFrame->frameOptions.homeId[2] = 0x83;
  pTestFrame->frameOptions.homeId[3] = 0xAF;
  pTestFrame->frameOptions.sourceNodeId      = 2;
  pTestFrame->frameOptions.sequenceNumber    = 1;

  // Multicast module is expected to build the multicast header. Not the Link Layer.
  // So copy in the multicast header.
  pTestFrame->headerLength = cmdMulticastFrameHeaderLength_40k;
  memcpy((uint8_t*)&pTestFrame->header, cmdMulticastFrame_40k, cmdMulticastFrameHeaderLength_40k);

  pTestFrame->payloadLength = multicastFrameLength;
  memcpy(pTestFrame->payload, multicastFrame, pTestFrame->payloadLength);

  // Transmit using 40k.
  communicationProfile = RF_PROFILE_40K;
  actualVal = llTransmitFrame(communicationProfile, pTestFrame);
  TEST_ASSERT_EQUAL(SUCCESS, actualVal);

  // Second transmission of a transfer presentation on 100k (sequence number incremented)
  pTestFrame->frameOptions.frameType = HDRTYP_MULTICAST;
  pTestFrame->frameOptions.homeId[0] = 0xE3;
  pTestFrame->frameOptions.homeId[1] = 0x72;
  pTestFrame->frameOptions.homeId[2] = 0x83;
  pTestFrame->frameOptions.homeId[3] = 0xAF;
  pTestFrame->frameOptions.sourceNodeId      = 2;
  pTestFrame->frameOptions.sequenceNumber    = 3;

  // Multicast module is expected to build the multicast header. Not the Link Layer.
  // So copy in the multicast header.
  pTestFrame->headerLength = cmdMulticastFrameHeaderLength_100k;
  memcpy((uint8_t*)&pTestFrame->header, cmdMulticastFrame_100k, cmdMulticastFrameHeaderLength_100k);

  pTestFrame->payloadLength = multicastFrameLength;
  memcpy(pTestFrame->payload, multicastFrame, pTestFrame->payloadLength);

  // Transmit using 100k.
  communicationProfile = RF_PROFILE_100K;
  actualVal = llTransmitFrame(communicationProfile, pTestFrame);
  TEST_ASSERT_EQUAL(SUCCESS, actualVal);
}

/**
 * This test will set up the node to support EU @ 9.6k/40k/100k and the capabilities of remote node is just 9.6k / 40k.
 */
void test_speed_to_comm_profile_null_pointer_ack_frame_eu(void)
{
  /***** Expectation setup *****/
  mock_t* p_mock = NULL;
  ZW_ReturnCode_t radioInitReturnValueExpected = SUCCESS;
  ZW_ReturnCode_t actualVal;

  zpal_radio_profile_t radioProfile = { .region = REGION_EU,
                                        .wakeup = ZPAL_RADIO_WAKEUP_ALWAYS_LISTEN,
  };
  /** As correct radio initialization is ensured in other tests, we just utilize the helper function in this test. */
  helper_func_expect_radio_init_eu(&radioProfile, p_mock);
  actualVal = llInit(&radioProfile);
  TEST_ASSERT_EQUAL(radioInitReturnValueExpected, actualVal);

  CommunicationProfile_t communicationProfile = RF_PROFILE_40K;

  actualVal = llTransmitFrame(0, NULL);
  TEST_ASSERT_EQUAL(UNSUPPORTED, actualVal);

  actualVal = llTransmitFrame(communicationProfile, NULL);
  TEST_ASSERT_EQUAL(INVALID_PARAMETERS, actualVal);
}

