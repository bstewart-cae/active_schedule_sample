// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file TestTransmitSingleCastLinkLayer.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include <unity.h>
#include <mock_control.h>
//#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "ZW_DataLinkLayer.h"
#include <zpal_radio.h>
#include <ZW_home_id_hash.h>
#include <zpal_radio_utils.h>

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


void helper_func_expect_radio_init_eu(zpal_radio_profile_t *pRfProfile, mock_t * p_mock)
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
 * This test will set up the node to support EU @ 9.6k/40k/100k and the capabilities of remote node is just 9.6k / 40k.
 */
void test_speed_to_comm_profile_always_listen_and_transmit_eu(void)
{
  /***** Expectation setup *****/


  /** When transmitting a frame, it is expected that a call ZW_RadioTransmit to phy layer will occur.
   *  This test ensures that a frame can be correctly transmitted @9.6k and @40 speed as that is what the remote node supports.
   *  Self node supports all speed variants.
   *  This test receive and transmits the first part of an inclusion, i.e.
   *  1) Transmit a transfer presentation (broadcast)
   *  2) Receive a NIF (TODO)
   *  3) Transmit an assign id=0x02 and home id=0xCAFEBABF.
   *  4) Receive an ACK (TODO)
   *  5) Transmit a NOP
   *  6) Receive an ACK (TODO)
   *  Jump to higher speed (40K) and:
   *  7) Transmit find nodes in range
   */
  mock_t * p_mock = NULL;
  zpal_radio_transmit_parameter_t txParameters9k6 = {.speed = ZPAL_RADIO_SPEED_9600,// Speed 9.6K=1000, Speed 40K=0100, Speed 100K=0010.
                                      .channel_id = 2,       // Channel0=0x00, Channel1=0x01, Channel2=0x02.
                                      .crc = 0x01,          // CRC 8bit=0, CRC 16bit=1
                                      .preamble_length = 10, // 10 bytes preamble on 9.6K
                                      .repeats = 0};        // No repeats (No wakeup beam)

  zpal_radio_transmit_parameter_t txParameters40k = {.speed = ZPAL_RADIO_SPEED_40K,// Speed 9.6K=1000, Speed 40K=0100, Speed 100K=0010.
                                      .channel_id = 1,       // Channel0=0x00, Channel1=0x01, Channel2=0x02.
                                      .crc = 0x01,          // CRC 8bit=0, CRC 16bit=1
                                      .preamble_length = 20, // 20 bytes preamble on 40K
                                      .repeats = 0};        // No repeats (No wakeup beam)

  uint8_t          transferPresentationFrameHeaderLength = 9; // The Transfer presentation as it is expected to be seen on the phy layer.
  uint8_t          transferPresentationFramePayloadLength = 3; // The Transfer presentation as it is expected to be seen on the phy layer.
  uint8_t          transferPresentationFullFrame[]     = {0xCA, 0xFE, 0xBA, 0xBF, 0x01, 0x01, 0x01, 0x0D, 0xFF, // HomeId, Src, Flags1, Flags2, Length, Dst
                                                          0x01, 0x08, 0x05,
                                                          0x31}; // CRC
  uint8_t          assignIdFrameHeaderLength = 9;
  uint8_t          assignIdFramePayloadLength = 7;
  uint8_t          assignIdFullFrame[]     = {0x11, 0x22, 0x33, 0x44, 0x01, 0x41, 0x02, 0x11, 0x00, // HomeId, Src, Flags1, Flags2, Length, Dst
                                              0x01, 0x03, 0x05, 0xCA, 0xFE, 0xBA, 0xBF,
                                              0xDE}; // CRC
  uint8_t          findNodesInRangeFrameHeaderLength = 9;
  uint8_t          findNodesInRangeFramePayloadLength = 7;
  uint8_t          findNodesInRangeFullFrame[]     = {0xCA, 0xFE, 0xBA, 0xBF, 0x01, 0x41, 0x03, 0x11, 0x05, // HomeId, Src, Flags1, Flags2, Length, Dst
                                                      0x01, 0x04, 0x01, 0x01, 0x00, 0x03, 0xCF,
                                                      0x50}; // CRC

  // Broad cast@9.6k
  mock_call_expect(TO_STR(zpal_radio_transmit), &p_mock);
  p_mock->expect_arg[0].pointer = &txParameters9k6;
  p_mock->expect_arg[1].value   = transferPresentationFrameHeaderLength;
  p_mock->expect_arg[2].pointer = &transferPresentationFullFrame;
  p_mock->expect_arg[3].value   = transferPresentationFramePayloadLength;
  p_mock->expect_arg[4].pointer = &transferPresentationFullFrame[transferPresentationFrameHeaderLength];
  p_mock->expect_arg[5].value   = 1;

  // single cast@9.6k
  mock_call_expect(TO_STR(zpal_radio_transmit), &p_mock);
  p_mock->expect_arg[0].pointer = &txParameters9k6;
  p_mock->expect_arg[1].value   = assignIdFrameHeaderLength;
  p_mock->expect_arg[2].pointer = &assignIdFullFrame;
  p_mock->expect_arg[3].value   = assignIdFramePayloadLength;
  p_mock->expect_arg[4].pointer = &assignIdFullFrame[assignIdFrameHeaderLength];
  p_mock->expect_arg[5].value   = 1;

  // single cast@40k
  mock_call_expect(TO_STR(zpal_radio_transmit), &p_mock);
  p_mock->expect_arg[0].pointer = &txParameters40k;
  p_mock->expect_arg[1].value   = findNodesInRangeFrameHeaderLength;
  p_mock->expect_arg[2].pointer = &findNodesInRangeFullFrame;
  p_mock->expect_arg[3].value   = findNodesInRangeFramePayloadLength;
  p_mock->expect_arg[4].pointer = &findNodesInRangeFullFrame[findNodesInRangeFrameHeaderLength];
  p_mock->expect_arg[5].value   = 1;
  /***** Executed the test *****/

  uint8_t          transferPresentationLength = 3;
  uint8_t          transferPresentation[]     = {0x01, 0x08, 0x05};

  uint8_t          assignIdLength = 7;
  uint8_t          assignId[]     = {0x01, 0x03, 0x05, 0xCA, 0xFE, 0xBA, 0xBF};

  uint8_t          findNodesInRangeLength = 7;
  uint8_t          findNodesInRange[]     = {0x01, 0x04, 0x01, 0x01, 0x00, 0x03, 0xCF};

  uint8_t      frameBuffer[sizeof(ZW_TransmissionFrame_t) + 7 + 1] = {0, };
  ZW_TransmissionFrame_t * pTestFrame = (ZW_TransmissionFrame_t *)frameBuffer;

  ZW_ReturnCode_t radioInitReturnValueExpected = SUCCESS;
  ZW_ReturnCode_t actualVal;
  // TODO: When API to phy layer is defined, create corresponding mock for test verification.

  zpal_radio_profile_t radioProfile = { REGION_EU, ZPAL_RADIO_WAKEUP_ALWAYS_LISTEN };
  /** As correct radio initialization is ensured in other tests, we just utilize the helper function in this test. */
  helper_func_expect_radio_init_eu(&radioProfile, p_mock);
  actualVal = llInit(&radioProfile);
  TEST_ASSERT_EQUAL(radioInitReturnValueExpected, actualVal);

  CommunicationProfile_t communicationProfile = RF_PROFILE_9_6K;

  pTestFrame->useLBT=1;
  // First transmission of a transfer presentation
  pTestFrame->frameOptions.frameType = HDRTYP_SINGLECAST;
  pTestFrame->frameOptions.homeId[0] = 0xCA;
  pTestFrame->frameOptions.homeId[1] = 0xFE;
  pTestFrame->frameOptions.homeId[2] = 0xBA;
  pTestFrame->frameOptions.homeId[3] = 0xBF;
  pTestFrame->frameOptions.sourceNodeId      = 1;
  pTestFrame->frameOptions.destinationNodeId = 0xFF;
  pTestFrame->frameOptions.sequenceNumber    = 1;
  pTestFrame->headerLength = transferPresentationFrameHeaderLength;
  pTestFrame->payloadLength = transferPresentationLength;
  memcpy(pTestFrame->payload, transferPresentation, pTestFrame->payloadLength);
  actualVal = llTransmitFrame(communicationProfile, pTestFrame);
  TEST_ASSERT_EQUAL(SUCCESS, actualVal);

  // Second transmission of an assign id
  pTestFrame->frameOptions.frameType = HDRTYP_SINGLECAST;
  pTestFrame->frameOptions.homeId[0] = 0x11;
  pTestFrame->frameOptions.homeId[1] = 0x22;
  pTestFrame->frameOptions.homeId[2] = 0x33;
  pTestFrame->frameOptions.homeId[3] = 0x44;
  pTestFrame->frameOptions.sourceNodeId      = 1;
  pTestFrame->frameOptions.destinationNodeId = 0;
  pTestFrame->frameOptions.acknowledge       = 1;
  pTestFrame->frameOptions.sequenceNumber    = 2;
  pTestFrame->headerLength = assignIdFrameHeaderLength;
  pTestFrame->payloadLength = assignIdLength;
  memcpy(pTestFrame->payload, assignId, pTestFrame->payloadLength);
  actualVal = llTransmitFrame(communicationProfile, pTestFrame);
  TEST_ASSERT_EQUAL(SUCCESS, actualVal);

  communicationProfile = RF_PROFILE_40K;

  pTestFrame->frameOptions.frameType = HDRTYP_SINGLECAST;
  pTestFrame->frameOptions.homeId[0] = 0xCA;
  pTestFrame->frameOptions.homeId[1] = 0xFE;
  pTestFrame->frameOptions.homeId[2] = 0xBA;
  pTestFrame->frameOptions.homeId[3] = 0xBF;
  pTestFrame->frameOptions.sourceNodeId      = 1;
  pTestFrame->frameOptions.destinationNodeId = 5;
  pTestFrame->frameOptions.acknowledge       = 1;
  pTestFrame->frameOptions.sequenceNumber    = 3;
  pTestFrame->headerLength = findNodesInRangeFrameHeaderLength;
  pTestFrame->payloadLength = findNodesInRangeLength;
  memcpy(pTestFrame->payload, findNodesInRange, pTestFrame->payloadLength);
  actualVal = llTransmitFrame(communicationProfile, pTestFrame);
  TEST_ASSERT_EQUAL(SUCCESS, actualVal);

  actualVal = llTransmitFrame(0, NULL);
  TEST_ASSERT_EQUAL(UNSUPPORTED, actualVal);
}

/** Verification that provided with node capabilities a set of communication profiles is returned.
 * When transmitting a frame with a given communication profile then the frame is passed to the phy layer with correct CRC.
 *
 * This test will set up the node to support EU @ 9.6k/40k/100k and the capabilities of remote node is just 9.6k / 40k.
 */
void test_speed_to_comm_profile_null_pointer_frame_eu(void)
{
  /***** Expectation setup *****/


  /** Ensure that in case a pFrame is a null pointer, then the link layer return invalid parameters as return code.
   */
  ZW_ReturnCode_t radioInitReturnValueExpected = SUCCESS;
  ZW_ReturnCode_t actualVal;

  zpal_radio_profile_t radioProfile = { REGION_EU, ZPAL_RADIO_WAKEUP_ALWAYS_LISTEN };
  /** As correct radio initialization is ensured in other tests, we just utilize the helper function in this test. */
  mock_t * p_mock = NULL;
  helper_func_expect_radio_init_eu(&radioProfile, p_mock);
  actualVal = llInit(&radioProfile);
  TEST_ASSERT_EQUAL(radioInitReturnValueExpected, actualVal);


  CommunicationProfile_t communicationProfile = RF_PROFILE_40K;

  // First transmission of a transfer presentation
  actualVal = llTransmitFrame(communicationProfile, NULL);
  TEST_ASSERT_EQUAL(INVALID_PARAMETERS, actualVal);
}

/** Verification that provided with node capabilities a set of communication profiles is returned.
 * When transmitting a frame with a given communication profile then the frame is passed to the phy layer with correct CRC.
 *
 * This test will set up the node to support EU @ 9.6k/40k/100k and the capabilities of remote node is just 9.6k / 40k.
 */
void test_speed_to_comm_profile_tx_40k_two_nodes_assign_id_transmit_eu(void)
{
  /***** Expectation setup *****/


  /** When transmitting a frame with wakeup beam, it is expected that two calls ZW_RadioTransmit to phy layer will occur.
   *  This test ensures that a wakeup beam and the frame can be correctly transmitted @40 speed as that is what the remote node supports.
   *  Self node supports all speed variants.
   *  This test transmits a wakeup beam  and the actual frame.
   *  1) Transmit an assign id=0x02 and home id=0xCAFEBABF. (TODO Adjust to real FLiRS frame)
   */

  mock_t * p_mock = NULL;

  zpal_radio_transmit_parameter_t txParameters40k = {.speed = ZPAL_RADIO_SPEED_40K,// Speed 9.6K=1000, Speed 40K=0100, Speed 100K=0010.
                                      .channel_id = 1,       // Channel0=0x00, Channel1=0x01, Channel2=0x02.
                                      .crc = 0x01,          // CRC 8bit=0, CRC 16bit=1
                                      .preamble_length = 20, // 10 bytes preamble on 40K
                                      .repeats = 0};        // Not a wakeup beam


  uint8_t          assignIdHeaderLength    = 9;
  uint8_t          assignIdPayloadLength   = 7;
  uint8_t          assignIdExpect2[] = {0xCA, 0xFE, 0xBA, 0xBF, 0x01, 0x41, 0x03, 0x11, 0x00, // HomeId, Src, Flags1, Flags2, Length, Dst
                                        0x01, 0x03, 0x02, 0x7E, 0x57, 0x1D, 0x01, 0xA9        // CC vers, AssignId, NewNodeId, NewHomeId, Crc.
                                       };
  uint8_t          assignIdExpect3[] = {0xC0, 0xB5, 0x07, 0xE6, 0x01, 0x41, 0x04, 0x11, 0x00, // HomeId, Src, Flags1, Flags2, Length, Dst
                                        0x01, 0x03, 0x03, 0x7E, 0x57, 0x1D, 0x01, 0x0A        // CC vers, AssignId, NewNodeId, NewHomeId, Crc.
                                       };
  // single cast@40k
  mock_call_expect(TO_STR(zpal_radio_transmit), &p_mock);
  p_mock->expect_arg[0].pointer = &txParameters40k;
  p_mock->expect_arg[1].value   = assignIdHeaderLength;
  p_mock->expect_arg[2].pointer = &assignIdExpect2;
  p_mock->expect_arg[3].value   = assignIdPayloadLength;
  p_mock->expect_arg[4].pointer = &assignIdExpect2[assignIdHeaderLength];
  p_mock->expect_arg[5].value   = 1;

  // single cast@40k
  mock_call_expect(TO_STR(zpal_radio_transmit), &p_mock);
  p_mock->expect_arg[0].pointer = &txParameters40k;
  p_mock->expect_arg[1].value   = assignIdHeaderLength;
  p_mock->expect_arg[2].pointer = &assignIdExpect3;
  p_mock->expect_arg[3].value   = assignIdPayloadLength;
  p_mock->expect_arg[4].pointer = &assignIdExpect3[assignIdHeaderLength];
  p_mock->expect_arg[5].value   = 1;

  ZW_ReturnCode_t actualVal;
  ZW_ReturnCode_t radioInitReturnValueExpected = SUCCESS;

  // Initialize our radio, which is always listen and all speeds.
  zpal_radio_profile_t radioProfile = { REGION_EU, ZPAL_RADIO_WAKEUP_ALWAYS_LISTEN };
  /** As correct radio initialization is ensured in other tests, we just utilize the helper function in this test. */
  helper_func_expect_radio_init_eu(&radioProfile, p_mock);
  actualVal = llInit(&radioProfile);
  TEST_ASSERT_EQUAL(radioInitReturnValueExpected, actualVal);

  CommunicationProfile_t communicationProfile = RF_PROFILE_40K;

  uint8_t      frameBuffer[sizeof(ZW_TransmissionFrame_t ) + 7 + 1] = {0, };
  ZW_TransmissionFrame_t * pFrame = (ZW_TransmissionFrame_t*) frameBuffer;
  memset(&pFrame->frameOptions, 0, sizeof(pFrame->frameOptions));
  pFrame->useLBT=1;
  pFrame->frameOptions.frameType = HDRTYP_SINGLECAST;
  pFrame->frameOptions.homeId[0] = 0xCA;
  pFrame->frameOptions.homeId[1] = 0xFE;
  pFrame->frameOptions.homeId[2] = 0xBA;
  pFrame->frameOptions.homeId[3] = 0xBF;
  pFrame->frameOptions.sourceNodeId      = 1;
  pFrame->frameOptions.destinationNodeId = 0;
  pFrame->frameOptions.acknowledge       = 1;
  pFrame->frameOptions.sequenceNumber    = 3;
  pFrame->headerLength = assignIdHeaderLength;
  pFrame->payloadLength = 7;    // Length of payload
  pFrame->payload[0]    = 0x01; // Z-Wave protocol Command Class version 1
  pFrame->payload[1]    = 0x03; // Z-Wave command: Assign ID
  pFrame->payload[2]    = 0x02; // New Node ID
  pFrame->payload[3]    = 0x7E; // New Home ID (4 bytes)
  pFrame->payload[4]    = 0x57;
  pFrame->payload[5]    = 0x1D;
  pFrame->payload[6]    = 0x01;

  actualVal = llTransmitFrame(communicationProfile, pFrame);
  TEST_ASSERT_EQUAL(SUCCESS, actualVal);

  ZW_BasicFrameOptions_t txOptionsNet2 = {.frameType = HDRTYP_SINGLECAST, .homeId = {0xC0, 0xB5, 0x07, 0xE6}, .sourceNodeId = 1, .destinationNodeId = 0, .acknowledge = 1, .sequenceNumber = 4, .speedModified = 0};
  memcpy(&pFrame->frameOptions, &txOptionsNet2, sizeof(ZW_BasicFrameOptions_t));
  pFrame->headerLength = assignIdHeaderLength;
  pFrame->payloadLength = 7;    // Length of payload
  pFrame->payload[0]    = 0x01; // Z-Wave protocol Command Class version 1
  pFrame->payload[1]    = 0x03; // Z-Wave command: Assign ID
  pFrame->payload[2]    = 0x03; // New Node ID
  pFrame->payload[3]    = 0x7E; // New Home ID (4 bytes)
  pFrame->payload[4]    = 0x57;
  pFrame->payload[5]    = 0x1D;
  pFrame->payload[6]    = 0x01;
  actualVal = llTransmitFrame(communicationProfile, pFrame);
  TEST_ASSERT_EQUAL(SUCCESS, actualVal);

//  // TODO: Expect a wakeup beam being transmitted
//  //       Thereafter the real frame
//  actualVal = llTransmitFrame(communicationProfileSet.speed_40k, 0, 0, 0);
//  TEST_ASSERT_EQUAL(SUCCESS, actualVal);
//
  // ToDo: As the communication profile is no longer returned as set, but directly provided by the caller, this tests must be adjusted accordingly.
  // 100k is unsupported, so let's verify that is actually the case.
  //actualVal = llTransmitSingleCastFrame(communicationProfileSet.speed_100k, pFrame);
  //TEST_ASSERT_EQUAL(UNSUPPORTED, actualVal);

  actualVal = llTransmitFrame(0, NULL);
  TEST_ASSERT_EQUAL(UNSUPPORTED, actualVal);
}

/**
 * Test that emulates transfer of a frame with wakeup beam requested.
 */
void test_speed_40k_wakeup_beam_250_ms_transmit_eu(void)
{
  /***** Expectation setup *****/


  /** When transmitting a frame with wakeup beam, it is expected that two calls ZW_RadioTransmit to phy layer will occur.
   *  This test ensures that a wakeup beam and the frame can be correctly transmitted @40 speed as that is what the remote node supports.
   *  Self node supports all speed variants.
   *  This test transmits a wakeup beam  and the actual frame.
   *  1) Transmit an assign id=0x02 and home id=0xCAFEBABF. (TODO Adjust to real FLiRS frame)
   */

  mock_t * p_mock = NULL;
  zpal_radio_transmit_parameter_t txParameters40kWakeup = {.speed = ZPAL_RADIO_SPEED_40K,// Speed 9.6K=1000, Speed 40K=0100, Speed 100K=0010.
                                            .channel_id = 1,       // Channel0=0x00, Channel1=0x01, Channel2=0x02.
                                            .crc = 0x00,          // CRC 8bit=0, CRC 16bit=1
                                            .preamble_length = 20, // 20 bytes preamble on 40K wakeup beam
                                            .repeats = 58};       // Wakeup beam should be repeated for 100 ms, equivilent to 58 repeats

  uint8_t          wakeupFrameLength    = 3;
  uint8_t          wakeupFrame[]        = {0x55, 0x02, 0xCE};
  // We expect HomeIdHashCalculate to be called
  mock_call_expect(TO_STR(HomeIdHashCalculate), &p_mock);
  p_mock->expect_arg[0].value   = 0xBFBAFECA; //uint32_t homeId
  p_mock->expect_arg[1].value   = 2; // destinationNodeId
  p_mock->expect_arg[2].value   = zpal_radio_region_get_protocol_mode(REGION_EU, ZPAL_RADIO_LR_CH_CFG_NO_LR);
  // wakeup@40k
  mock_call_expect(TO_STR(zpal_radio_transmit_beam), &p_mock);
  p_mock->expect_arg[0].pointer = &txParameters40kWakeup;
  p_mock->expect_arg[1].value   = wakeupFrameLength;
  p_mock->expect_arg[2].pointer = &wakeupFrame;

  mock_call_expect(TO_STR(zpal_radio_get_protocol_mode), &p_mock);
  p_mock->return_code.value = zpal_radio_region_get_protocol_mode(REGION_EU, ZPAL_RADIO_LR_CH_CFG_NO_LR);

  ZW_ReturnCode_t actualVal;
  ZW_ReturnCode_t radioInitReturnValueExpected = SUCCESS;

  // Initialize our radio, which is always listen and all speeds.
  zpal_radio_profile_t radioProfile = { REGION_EU, ZPAL_RADIO_WAKEUP_ALWAYS_LISTEN };
  /** As correct radio initialization is ensured in other tests, we just utilize the helper function in this test. */
  helper_func_expect_radio_init_eu(&radioProfile, p_mock);
  actualVal = llInit(&radioProfile);
  TEST_ASSERT_EQUAL(radioInitReturnValueExpected, actualVal);

  CommunicationProfile_t communicationProfile = RF_PROFILE_40K_WAKEUP_250;

  uint8_t      frameBuffer[sizeof(ZW_TransmissionFrame_t ) + 2 + 1] = {0, };
  ZW_TransmissionFrame_t * pFrame = (ZW_TransmissionFrame_t*) frameBuffer;
  memset(&pFrame->frameOptions, 0, sizeof(ZW_BasicFrameOptions_t));
  pFrame->frameOptions.frameType = HDRTYP_SINGLECAST;
  pFrame->frameOptions.homeId[0] = 0xCA;
  pFrame->frameOptions.homeId[1] = 0xFE;
  pFrame->frameOptions.homeId[2] = 0xBA;
  pFrame->frameOptions.homeId[3] = 0xBF;
  pFrame->frameOptions.sourceNodeId      = 1;
  pFrame->frameOptions.destinationNodeId = 2;
  pFrame->frameOptions.acknowledge       = 1;
  pFrame->frameOptions.sequenceNumber    = 10;
  pFrame->payloadLength = 2;    // Length of payload
  pFrame->payload[0]    = 0x5e; // Z-Wave protocol Command Class version 1, Z-Wave Plus
  pFrame->payload[1]    = 0x01; // Z-Wave command: Node Info get

  actualVal = llTransmitFrame(communicationProfile, pFrame);
  TEST_ASSERT_EQUAL(SUCCESS, actualVal);
}

/**
 * Test that emulates transfer of a frame with wakeup beam requested.
 */
void test_speed_40k_wakeup_beam_1000_ms_transmit_eu(void)
{
  /***** Expectation setup *****/


  /** When transmitting a frame with wakeup beam, it is expected that two calls ZW_RadioTransmit to phy layer will occur.
   *  This test ensures that a wakeup beam and the frame can be correctly transmitted @40 speed as that is what the remote node supports.
   *  Self node supports all speed variants.
   *  This test transmits a wakeup beam  and the actual frame.
   *  1) Transmit an assign id=0x02 and home id=0xCAFEBABF. (TODO Adjust to real FLiRS frame)
   */

  mock_t * p_mock = NULL;
  zpal_radio_transmit_parameter_t txParameters40kWakeup = {.speed = ZPAL_RADIO_SPEED_40K,// Speed 9.6K=1000, Speed 40K=0100, Speed 100K=0010.
                                            .channel_id = 1,       // Channel0=0x00, Channel1=0x01, Channel2=0x02.
                                            .crc = 0x00,          // CRC 8bit=0, CRC 16bit=1
                                            .preamble_length = 20, // 20 bytes preamble on 40K wakeup beam
                                            .repeats = 230};       // Wakeup beam should be repeated for 100 ms, equivilent to 58 repeats

  uint8_t          wakeupFrameLength    = 3;
  uint8_t          wakeupFrame[]        = {0x55, 0x02, 0xCE};

  // We expect HomeIdHashCalculate to be called
  mock_call_expect(TO_STR(HomeIdHashCalculate), &p_mock);
  p_mock->expect_arg[0].value   = 0xBFBAFECA; //uint32_t homeId
  p_mock->expect_arg[1].value   = 2; // destinationNodeId
  p_mock->expect_arg[2].value   = zpal_radio_region_get_protocol_mode(REGION_EU, ZPAL_RADIO_LR_CH_CFG_NO_LR);
  // wakeup@40k - We expect zpal_radio_transmit_beam to be called
  mock_call_expect(TO_STR(zpal_radio_transmit_beam), &p_mock);
  p_mock->expect_arg[0].pointer = &txParameters40kWakeup;
  p_mock->expect_arg[1].value   = wakeupFrameLength;
  p_mock->expect_arg[2].pointer = &wakeupFrame;

  mock_call_expect(TO_STR(zpal_radio_get_protocol_mode), &p_mock);
  p_mock->return_code.value = zpal_radio_region_get_protocol_mode(REGION_EU, ZPAL_RADIO_LR_CH_CFG_NO_LR);

  ZW_ReturnCode_t actualVal;
  ZW_ReturnCode_t radioInitReturnValueExpected = SUCCESS;

  // Initialize our radio, which is always listen and all speeds.
  zpal_radio_profile_t radioProfile = { REGION_EU, ZPAL_RADIO_WAKEUP_ALWAYS_LISTEN };
  /** As correct radio initialization is ensured in other tests, we just utilize the helper function in this test. */
  helper_func_expect_radio_init_eu(&radioProfile, p_mock);
  actualVal = llInit(&radioProfile);
  TEST_ASSERT_EQUAL(radioInitReturnValueExpected, actualVal);

  CommunicationProfile_t communicationProfile = RF_PROFILE_40K_WAKEUP_1000;

  uint8_t      frameBuffer[sizeof(ZW_TransmissionFrame_t ) + 2 + 1] = {0, };
  ZW_TransmissionFrame_t * pFrame = (ZW_TransmissionFrame_t*) frameBuffer;
  memset(&pFrame->frameOptions, 0, sizeof(ZW_BasicFrameOptions_t));
  pFrame->frameOptions.frameType = HDRTYP_SINGLECAST;
  pFrame->frameOptions.homeId[0] = 0xCA;
  pFrame->frameOptions.homeId[1] = 0xFE;
  pFrame->frameOptions.homeId[2] = 0xBA;
  pFrame->frameOptions.homeId[3] = 0xBF;
  pFrame->frameOptions.sourceNodeId      = 1;
  pFrame->frameOptions.destinationNodeId = 2;
  pFrame->frameOptions.acknowledge       = 1;
  pFrame->frameOptions.sequenceNumber    = 10;
  pFrame->payloadLength = 2;    // Length of payload
  pFrame->payload[0]    = 0x5e; // Z-Wave protocol Command Class version 1, Z-Wave Plus
  pFrame->payload[1]    = 0x01; // Z-Wave command: Node Info get

  actualVal = llTransmitFrame(communicationProfile, pFrame);
  TEST_ASSERT_EQUAL(SUCCESS, actualVal);
}

/**
 * Test that verifies that a frame sent a lower spped will have the speed modified bit set.
 */
void test_speed_9_6k_retransmit_low_speed_eu(void)
{
  /***** Expectation setup *****/

  mock_t * p_mock = NULL;

  zpal_radio_transmit_parameter_t txParameters9_6k = {.speed = ZPAL_RADIO_SPEED_9600,// Speed 9.6K=1000, Speed 40K=0100, Speed 100K=0010.
                                       .channel_id = 2,       // Channel0=0x00, Channel1=0x01, Channel2=0x02.
                                       .crc = 0x01,          // CRC 8bit=0, CRC 16bit=1
                                       .preamble_length = 10, // 10 bytes preamble on 9.6K
                                       .repeats = 0};        // Not a wakeup beam

  uint8_t          nodeInforGetHeaderLength = 9;
  uint8_t          nodeInforGetPayloadLength = 2;
  uint8_t          nodeInforGet[]     = {0xca, 0xfe, 0xba, 0xbf, 0x01, 0x51, 0x0a, 0x0c, 0x02, // HomeId, Src, Flags1, Flags2, Length, Dst
                                         0x5e, 0x01,                                           // CC Plus vers, InfoGet,
                                         0xC5};                                                // Crc


  // single cast@40k
  mock_call_expect(TO_STR(zpal_radio_transmit), &p_mock);
  p_mock->expect_arg[0].pointer = &txParameters9_6k;
  p_mock->expect_arg[1].value   = nodeInforGetHeaderLength;
  p_mock->expect_arg[2].pointer = &nodeInforGet;
  p_mock->expect_arg[3].value   = nodeInforGetPayloadLength;
  p_mock->expect_arg[4].pointer = &nodeInforGet[nodeInforGetHeaderLength];
  p_mock->expect_arg[5].value   = 1;


  ZW_ReturnCode_t actualVal;
  ZW_ReturnCode_t radioInitReturnValueExpected = SUCCESS;

  // Initialize our radio, which is always listen and all speeds.
  zpal_radio_profile_t radioProfile = { .region = REGION_EU,
                                        .wakeup = ZPAL_RADIO_WAKEUP_ALWAYS_LISTEN,
  };
  /** As correct radio initialization is ensured in other tests, we just utilize the helper function in this test. */
  helper_func_expect_radio_init_eu(&radioProfile, p_mock);
  actualVal = llInit(&radioProfile);
  TEST_ASSERT_EQUAL(radioInitReturnValueExpected, actualVal);

  CommunicationProfile_t communicationProfile = RF_PROFILE_9_6K;

  uint8_t      frameBuffer[sizeof(ZW_TransmissionFrame_t ) + 2 + 1] = {0, };
  ZW_TransmissionFrame_t * pFrame = (ZW_TransmissionFrame_t*) frameBuffer;
  pFrame->useLBT=1;
  pFrame->frameOptions.frameType = HDRTYP_SINGLECAST;
  pFrame->frameOptions.homeId[0] = 0xCA;
  pFrame->frameOptions.homeId[1] = 0xFE;
  pFrame->frameOptions.homeId[2] = 0xBA;
  pFrame->frameOptions.homeId[3] = 0xBF;
  pFrame->frameOptions.sourceNodeId      = 1;
  pFrame->frameOptions.destinationNodeId = 2;
  pFrame->frameOptions.acknowledge       = 1;
  pFrame->frameOptions.speedModified     = 1;
  pFrame->frameOptions.sequenceNumber    = 10;
  pFrame->headerLength = nodeInforGetHeaderLength;
  pFrame->payloadLength = 2;    // Length of payload
  pFrame->payload[0]    = 0x5e; // Z-Wave protocol Command Class version 1, Z-Wave Plus
  pFrame->payload[1]    = 0x01; // Z-Wave command: Node Info get

  actualVal = llTransmitFrame(communicationProfile, pFrame);
  TEST_ASSERT_EQUAL(SUCCESS, actualVal);
}

/**
 * Test that verifies that a frame sent a lower spped will have the speed modified bit set.
 */
void test_speed_100k_transmit_get_info_eu(void)
{
  /***** Expectation setup *****/

  mock_t * p_mock = NULL;

  zpal_radio_transmit_parameter_t txParameters100k = {.speed = ZPAL_RADIO_SPEED_100K,// Speed 9.6K=1000, Speed 40K=0100, Speed 100K=0010.
                                       .channel_id = 0,       // Channel0=0x00, Channel1=0x01, Channel2=0x02.
                                       .crc = 0x02,          // CRC 8bit=1, CRC 16bit=2
                                       .preamble_length = 40, // 40 bytes preamble on 100K
                                       .repeats = 0};        // Not a wakeup beam

  uint8_t          nodeInforGetHeaderLength = 9;
  uint8_t          nodeInforGetPayloadLength = 2;
  uint8_t          nodeInforGet[]     = {0xca, 0xfe, 0xba, 0xbf, 0x01, 0x51, 0x0a, 0x0d, 0x02, // HomeId, Src, Flags1, Flags2, Length, Dst
                                         0x5e, 0x01,                                           // CC Plus vers, InfoGet,
                                         0x82, 0x34};                                          // Crc - ccitt


  // single cast@40k
  mock_call_expect(TO_STR(zpal_radio_transmit), &p_mock);
  p_mock->expect_arg[0].pointer = &txParameters100k;
  p_mock->expect_arg[1].value   = nodeInforGetHeaderLength;
  p_mock->expect_arg[2].pointer = &nodeInforGet;
  p_mock->expect_arg[3].value   = nodeInforGetPayloadLength;
  p_mock->expect_arg[4].pointer = &nodeInforGet[nodeInforGetHeaderLength];
  p_mock->expect_arg[5].value   = 1;


  ZW_ReturnCode_t actualVal;
  ZW_ReturnCode_t radioInitReturnValueExpected = SUCCESS;
  // Initialize our radio, which is always listen and all speeds.
  zpal_radio_profile_t radioProfile = { REGION_EU, ZPAL_RADIO_WAKEUP_ALWAYS_LISTEN };
  /** As correct radio initialization is ensured in other tests, we just utilize the helper function in this test. */
  helper_func_expect_radio_init_eu(&radioProfile, p_mock);
  actualVal = llInit(&radioProfile);
  TEST_ASSERT_EQUAL(radioInitReturnValueExpected, actualVal);

  CommunicationProfile_t communicationProfile = RF_PROFILE_100K;

  uint8_t      frameBuffer[sizeof(ZW_TransmissionFrame_t ) + 2 + 2] = {0,};
  ZW_TransmissionFrame_t * pFrame = (ZW_TransmissionFrame_t*) frameBuffer;
  pFrame->useLBT=1;
  pFrame->frameOptions.frameType = HDRTYP_SINGLECAST;
  pFrame->frameOptions.homeId[0] = 0xCA;
  pFrame->frameOptions.homeId[1] = 0xFE;
  pFrame->frameOptions.homeId[2] = 0xBA;
  pFrame->frameOptions.homeId[3] = 0xBF;
  pFrame->frameOptions.sourceNodeId      = 1;
  pFrame->frameOptions.destinationNodeId = 2;
  pFrame->frameOptions.acknowledge       = 1;
  pFrame->frameOptions.speedModified     = 1;
  pFrame->frameOptions.sequenceNumber    = 10;
  pFrame->headerLength = nodeInforGetHeaderLength;
  pFrame->payloadLength = 2;    // Length of payload
  pFrame->payload[0]    = 0x5e; // Z-Wave protocol Command Class version 1, Z-Wave Plus
  pFrame->payload[1]    = 0x01; // Z-Wave command: Node Info get

  actualVal = llTransmitFrame(communicationProfile, pFrame);
  TEST_ASSERT_EQUAL(SUCCESS, actualVal);
}

/**
 * Test that verifies that a frame sent a lower spped will have the speed modified bit set.
 */
void test_speed_100k_Retransmit_get_info_eu(void)
{
  /***** Expectation setup *****/

  mock_t * p_mock = NULL;

  zpal_radio_transmit_parameter_t txParameters100k = {.speed = ZPAL_RADIO_SPEED_100K,// Speed 9.6K=1000, Speed 40K=0100, Speed 100K=0010.
                                       .channel_id = 0,       // Channel0=0x00, Channel1=0x01, Channel2=0x02.
                                       .crc = 0x02,          // CRC 8bit=0, CRC 16bit=2
                                       .preamble_length = 40, // 40 bytes preamble on 100K
                                       .repeats = 0};        // Not a wakeup beam

  uint8_t          nodeInforGetHeaderLength = 9;
  uint8_t          nodeInforGetPayloadLength = 2;
  uint8_t          nodeInforGet[]     = {0xca, 0xfe, 0xba, 0xbf, 0x01, 0x51, 0x0a, 0x0d, 0x02, // HomeId, Src, Flags1, Flags2, Length, Dst
                                         0x5e, 0x01,                                           // CC Plus vers, InfoGet,
                                         0x82, 0x34};                                          // Crc - ccitt

  ZW_ReturnCode_t actualVal;
  ZW_ReturnCode_t radioInitReturnValueExpected = SUCCESS;

  // Initialize our radio, which is always listen and all speeds.
  zpal_radio_profile_t radioProfile = { REGION_EU, ZPAL_RADIO_WAKEUP_ALWAYS_LISTEN };
  /** As correct radio initialization is ensured in other tests, we just utilize the helper function in this test. */
  helper_func_expect_radio_init_eu(&radioProfile, p_mock);
  actualVal = llInit(&radioProfile);
  TEST_ASSERT_EQUAL(radioInitReturnValueExpected, actualVal);

  CommunicationProfile_t communicationProfile = RF_PROFILE_100K;

  uint8_t      frameBuffer[sizeof(ZW_TransmissionFrame_t ) + 2 + 2] = {0,};
  ZW_TransmissionFrame_t * pFrame = (ZW_TransmissionFrame_t*) frameBuffer;
  pFrame->useLBT=1;
  pFrame->frameOptions.frameType = HDRTYP_SINGLECAST;
  pFrame->frameOptions.homeId[0] = 0xCA;
  pFrame->frameOptions.homeId[1] = 0xFE;
  pFrame->frameOptions.homeId[2] = 0xBA;
  pFrame->frameOptions.homeId[3] = 0xBF;
  pFrame->frameOptions.sourceNodeId      = 1;
  pFrame->frameOptions.destinationNodeId = 2;
  pFrame->frameOptions.acknowledge       = 1;
  pFrame->frameOptions.speedModified     = 1;
  pFrame->frameOptions.sequenceNumber    = 10;
  pFrame->headerLength = nodeInforGetHeaderLength;
  pFrame->payloadLength = 2;    // Length of payload
  pFrame->payload[0]    = 0x5e; // Z-Wave protocol Command Class version 1, Z-Wave Plus
  pFrame->payload[1]    = 0x01; // Z-Wave command: Node Info get

  mock_calls_clear();
  mock_call_expect(TO_STR(zpal_radio_transmit), &p_mock);
  p_mock->expect_arg[0].pointer = &txParameters100k;
  p_mock->expect_arg[1].value   = nodeInforGetHeaderLength;
  p_mock->expect_arg[2].pointer = &nodeInforGet;
  p_mock->expect_arg[3].value   = nodeInforGetPayloadLength;
  p_mock->expect_arg[4].pointer = &nodeInforGet[nodeInforGetHeaderLength];
  p_mock->expect_arg[5].value   = 1;

  actualVal = llTransmitFrame(communicationProfile, pFrame);
  TEST_ASSERT_EQUAL(SUCCESS, actualVal);
  mock_calls_verify();

  llReTransmitStart(pFrame);

  mock_calls_clear();
  mock_call_use_as_stub(TO_STR(zpal_radio_rf_channel_statistic_tx_retries));
  mock_call_expect(TO_STR(zpal_radio_transmit), &p_mock);
  p_mock->expect_arg[0].pointer = &txParameters100k;
  p_mock->expect_arg[1].value   = nodeInforGetHeaderLength;
  p_mock->expect_arg[2].pointer = &nodeInforGet;
  p_mock->expect_arg[3].value   = nodeInforGetPayloadLength;
  p_mock->expect_arg[4].pointer = &nodeInforGet[nodeInforGetHeaderLength];
  p_mock->expect_arg[5].value   = 1;

  mock_call_expect(TO_STR(zpal_radio_get_protocol_mode),&p_mock);
  p_mock->return_code.value = ZPAL_RADIO_PROTOCOL_MODE_2;
  mock_call_expect(TO_STR(zpal_radio_get_protocol_mode),&p_mock);
  p_mock->return_code.value = ZPAL_RADIO_PROTOCOL_MODE_2;
  actualVal = llTransmitFrame(communicationProfile, pFrame);
  TEST_ASSERT_EQUAL(SUCCESS, actualVal);
  mock_calls_verify();

}

