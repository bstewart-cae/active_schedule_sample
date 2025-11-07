// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file TestTransmitExplorerFramesLinkLayer_3ch.c
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

/** Verification that provided with node capabilities a set of communication profiles is returned.
 * When transmitting a frame with a given communication profile then the frame is passed to the phy layer with correct CRC.
 *
 * Link Layer knows nothing about eplorer frame, only frame header.
 * Thus it is expected that upper layer has encoded explorer header correctly, and link layer only prepends the real header.
 *
 * This test will set up the node to support JP @ 100k speed on all channels.
 */
void test_speed_to_comm_profile_always_listen_and_transmit_explorer_frame_jp(void)
{
  mock_t* p_mock = NULL;

  /** When transmitting a frame, it is expected that a call ZW_RadioTransmit to phy layer will occur.
   *  This test ensures that a frame can be correctly transmitted @100k speed.
   *  Self node supports all speed variants.
   *  This test receive and transmits the first part of an inclusion, i.e.
   *  1) Transmit a Cmd Set Nwi Mode (Explorer Normal)
   */
  zpal_radio_transmit_parameter_t txParameters100k = {.speed = ZPAL_RADIO_SPEED_100K,      // Speed 100K = 0010.
                                       .channel_id = 0,       // Channel_0 = 0x00, Channel1=0x01, Channel2=0x02.
                                       .crc = 0x02,          // CRC 16bit = 2
                                       .preamble_length = 24, // 24 bytes preamble on 3ck@100K
                                       .repeats = 0};        // No repeats (No wakeup beam)

  uint8_t cmdSetNwiFrameHeaderLength  = 10; // The Cmd Set Nwi header as it is expected to be seen on the phy layer.
  uint8_t cmdSetNwiFramePayloadLength = 12; // The Cmd Set Nwi payload as it is expected to be seen on the phy layer.
  uint8_t cmdSetNwiFullFrame[]        = {0xFD, 0x39, 0x34, 0xB0, 0x01, 0x05, 0x00, 0x18, 0x59, 0xFF, // General Header
                                         0x20, 0x00, 0xFA, 0x31, 0x05, 0x00, 0x00, 0x00,       // Explorer header. Version = 1, Command Type = 0 (Explorer Cmd Normal),  Session Tx Random = 0xFA, Routed, direction, stop, stop req all 0, Session TTL = 4, Repeater list empty
                                         0x01, 0x22, 0x01, 0x00,                               // Z-Wave protocol vers. = 1, Cmd set nwi (0x22), Idle = 0, Timeout Default = 0
                                         0x6A, 0x35};                                          // CRC

  // Explorer frame@40k
  mock_call_expect(TO_STR(zpal_radio_transmit), &p_mock);
  p_mock->expect_arg[0].pointer = &txParameters100k;
  p_mock->expect_arg[1].value   = cmdSetNwiFrameHeaderLength;
  p_mock->expect_arg[2].pointer = &cmdSetNwiFullFrame[0];
  p_mock->expect_arg[3].value   = cmdSetNwiFramePayloadLength;
  p_mock->expect_arg[4].pointer = &cmdSetNwiFullFrame[cmdSetNwiFrameHeaderLength];
  p_mock->expect_arg[5].value   = 1;

  /***** Execute the test *****/

  uint8_t cmdSetNwiLength = 4;
  uint8_t cmdSetNwi[]     = {0x01, 0x22, 0x01, 0x00};

  uint8_t frameBuffer[sizeof(ZW_TransmissionFrame_t) + 8 + 4 + 1] = {0,};
  ZW_TransmissionFrame_t * pTestFrame = (ZW_TransmissionFrame_t *)frameBuffer;

  ZW_ReturnCode_t radioInitReturnValueExpected = SUCCESS;
  ZW_ReturnCode_t actualVal;

  zpal_radio_profile_t radioProfile = { REGION_JP, ZPAL_RADIO_WAKEUP_ALWAYS_LISTEN };
  CommunicationProfile_t communicationProfile = RF_PROFILE_3CH_100K_CH_A;
    /** As correct radio initialization is ensured in other tests, we just utilize the helper function in this test. */
  helper_func_expect_radio_init_jp(&radioProfile, p_mock);
  actualVal = llInit(&radioProfile);
  TEST_ASSERT_EQUAL(radioInitReturnValueExpected, actualVal);

  // First transmission of a transfer presentation
  pTestFrame->useLBT=1;
  pTestFrame->frameOptions.frameType = HDRTYP_EXPLORE;
  pTestFrame->frameOptions.homeId[0] = 0xFD;
  pTestFrame->frameOptions.homeId[1] = 0x39;
  pTestFrame->frameOptions.homeId[2] = 0x34;
  pTestFrame->frameOptions.homeId[3] = 0xB0;
  pTestFrame->frameOptions.sourceNodeId      = 1;
  pTestFrame->frameOptions.destinationNodeId = 0xFF;
  pTestFrame->frameOptions.sequenceNumber    = 0x59;
  pTestFrame->headerLength = 0;
  pTestFrame->headerLength = cmdSetNwiFrameHeaderLength;
  pTestFrame->payloadLength = cmdSetNwiFramePayloadLength ;

 // Explorer module is expected to build the explorer header. Not the Link Layer.
  uint8_t explorerHeader[] = {0x20, 0x00, 0xFA, 0x40, 0x00, 0x00, 0x00, 0x00};
  memcpy(pTestFrame->payload, explorerHeader, sizeof(explorerHeader));
  memcpy(&pTestFrame->payload[sizeof(explorerHeader)], cmdSetNwi, cmdSetNwiLength);

  // test channel 0
  actualVal = llTransmitFrame(communicationProfile, pTestFrame);
  TEST_ASSERT_EQUAL(SUCCESS, actualVal);
}

/** Verification that provided with node capabilities a set of communication profiles is returned.
 * When transmitting a frame with a given communication profile then the frame is passed to the phy layer with correct CRC.
 *
 * This test will set up the node to support EU @ 9.6k/40k/100k and the capabilities of remote node is just 9.6k / 40k.
 */
void test_speed_to_comm_profile_null_pointer_explorer_frame_jp(void)
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

