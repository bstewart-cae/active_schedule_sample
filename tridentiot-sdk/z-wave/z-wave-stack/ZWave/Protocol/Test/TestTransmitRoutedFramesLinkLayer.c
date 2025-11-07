// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file TestTransmitRoutedFramesLinkLayer.c
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


void helper_func_expect_radio_init_eu (zpal_radio_profile_t *pRfProfile, mock_t * p_mock)
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
 * This test will set up the node to support EU @ 9.6k/40k/100k and the capabilities of remote node is just 9.6k / 40k.
 */
void test_speed_to_comm_profile_always_listen_and_transmit_routed_frame_eu(void)
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
                                      .crc = 0x01,          // CRC 8bit=0, CRC 16bit=1
                                      .preamble_length = 20, // 10 bytes preamble on 40K
                                      .repeats = 0};        // No repeats (No wakeup beam)

  uint8_t nopFrameHeaderLength = 12;
  uint8_t nopFullFrame[] = {0x00, 0x54, 0xA5, 0x00, 0x01, 0x81, 0x05, 0x0E, 0x08, 0x00, 0x10, 0x04, 0x00, 0x99};
  // Explorer frame@40k
  mock_call_expect(TO_STR(zpal_radio_transmit), &p_mock);
  p_mock->expect_arg[0].pointer = &txParameters40k;
  p_mock->expect_arg[1].value   = nopFrameHeaderLength;
  p_mock->expect_arg[2].pointer = &nopFullFrame[0];
  p_mock->expect_arg[3].value   = 1;
  p_mock->expect_arg[4].pointer = &nopFullFrame[nopFrameHeaderLength];
  p_mock->expect_arg[5].value   = 1;
  /***** Executed the test *****/


  uint8_t      frameBuffer[sizeof(ZW_TransmissionFrame_t) + 4 + 1] = {0, };
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

  CommunicationProfile_t communicationProfile = RF_PROFILE_40K;

  // First transmission of a transfer presentation
  pTestFrame->useLBT = 1;
  pTestFrame->frameOptions.frameType = HDRTYP_SINGLECAST;
  pTestFrame->frameOptions.homeId[0] = 0x00;
  pTestFrame->frameOptions.homeId[1] = 0x54;
  pTestFrame->frameOptions.homeId[2] = 0xA5;
  pTestFrame->frameOptions.homeId[3] = 0x00;
  pTestFrame->frameOptions.sourceNodeId      = 1;
  pTestFrame->frameOptions.destinationNodeId = 0x08;
  pTestFrame->frameOptions.sequenceNumber    = 5;
  pTestFrame->frameOptions.routed    = 1;

  pTestFrame->headerLength = nopFrameHeaderLength;
  memcpy((uint8_t*)&pTestFrame->header, nopFullFrame, nopFrameHeaderLength);

  pTestFrame->payloadLength = 1;
  pTestFrame->payload[0] = 0x00;



  actualVal = llTransmitFrame(communicationProfile, pTestFrame);
  TEST_ASSERT_EQUAL(SUCCESS, actualVal);

}

/** Verification that provided with node capabilities a set of communication profiles is returned.
 * When transmitting a frame with a given communication profile then the frame is passed to the phy layer with correct CRC.
 *
 * This test will set up the node to support EU @ 9.6k/40k/100k and the capabilities of remote node is just 9.6k / 40k.
 */
void test_speed_to_comm_profile_null_pointer_routed_frame_eu(void)
{
  mock_t * p_mock = NULL;
  /***** Expectation setup *****/
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

