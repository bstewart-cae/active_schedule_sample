// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file TestTransmitSingleCastLinkLayer_3ch.c
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

void helper_func_expect_radio_init_jp (zpal_radio_profile_t *pRfProfile, mock_t *p_mock)
{
  // Setup of test expectations. When configuration the Data Link Layer, the following calls are expected further down to Phy Layer.
  // 1) zpal_radio_init, with configuration of 3 channels for 9.6k, 40k, 100k. And a Energy Harvesting channel (still to be defined)
  mock_call_expect(TO_STR(zpal_radio_init), &p_mock);
  p_mock->expect_arg[0].pointer = pRfProfile;
  p_mock->compare_rule_arg[1]   = COMPARE_NULL;
  p_mock->compare_rule_arg[2]   = COMPARE_NULL;
  p_mock->compare_rule_arg[3]   = COMPARE_NULL;
}

/** Verification that the Data Link Layer can be initialized with an RF Profile and Data Rate.
 *
 * It verifies the behavior when radio is used in a controller that supports 3 channel @ 100k.
 *
 * When initializing the link layer for a given region (Freq. and speeds) a call to the RF driver is expected.
 */
void test_data_link_layer_3ch_all_regions_100k_speed(void)
{
  mock_t * p_mock = NULL;

  // Missing test expectation for channel configurations
  zpal_radio_region_t threeChTestVectorRegions[] = { REGION_JP, REGION_KR };

  ZW_ReturnCode_t radioInitReturnValueExpected = SUCCESS;
  ZW_ReturnCode_t radioInitReturnValueActual;

  zpal_radio_profile_t radioProfile;

  for (uint32_t i = 0; i < sizeof(threeChTestVectorRegions) / sizeof(threeChTestVectorRegions[0]); i++)
  {
    radioProfile.region = threeChTestVectorRegions[i];
    radioProfile.active_lr_channel_config = ZPAL_RADIO_LR_CH_CFG_NO_LR;

    mock_call_expect(TO_STR(zpal_radio_init), &p_mock);
    p_mock->expect_arg[0].pointer = &radioProfile;
    p_mock->compare_rule_arg[1]   = COMPARE_NULL;
    p_mock->compare_rule_arg[2]   = COMPARE_NULL;
    p_mock->compare_rule_arg[3]   = COMPARE_NULL;

    mock_call_expect(TO_STR(zpal_radio_init), &p_mock);
    p_mock->expect_arg[0].pointer = &radioProfile;
    p_mock->compare_rule_arg[1]   = COMPARE_NULL;
    p_mock->compare_rule_arg[2]   = COMPARE_NULL;
    p_mock->compare_rule_arg[3]   = COMPARE_NULL;

    mock_call_expect(TO_STR(zpal_radio_init), &p_mock);
    p_mock->expect_arg[0].pointer = &radioProfile;
    p_mock->compare_rule_arg[1]   = COMPARE_NULL;
    p_mock->compare_rule_arg[2]   = COMPARE_NULL;
    p_mock->compare_rule_arg[3]   = COMPARE_NULL;

    char errorMessage[25];
    sprintf(errorMessage, "- in test round %lu.", (long unsigned)i);


    radioProfile.wakeup = ZPAL_RADIO_WAKEUP_ALWAYS_LISTEN;
    radioInitReturnValueActual = llInit(&radioProfile);
    TEST_ASSERT_EQUAL_MESSAGE(radioInitReturnValueExpected, radioInitReturnValueActual,
                              errorMessage);


    radioProfile.wakeup = ZPAL_RADIO_WAKEUP_EVERY_250ms;
    radioInitReturnValueActual = llInit(&radioProfile);
    TEST_ASSERT_EQUAL_MESSAGE(radioInitReturnValueExpected, radioInitReturnValueActual,
                              errorMessage);

    radioProfile.wakeup = ZPAL_RADIO_WAKEUP_EVERY_1000ms;
    radioInitReturnValueActual = llInit(&radioProfile);
    TEST_ASSERT_EQUAL_MESSAGE(radioInitReturnValueExpected, radioInitReturnValueActual,
                              errorMessage);
  }
}

/** Verification that provided with node capabilities a set of communication profiles is returned.
 * When transmitting a frame with a given communication profile then the frame is passed to the phy layer with correct CRC.
 *
 * This test will set up the node to support JP @ 100k
 */
void test_speed_to_comm_profile_always_listen_and_transmit_jp(void)
{
  mock_t * p_mock = NULL;

  /** When transmitting a frame, it is expected that a call ZW_RadioTransmit to phy layer will occur.
   *  This test ensures that a frame can be correctly transmitted @100 speed as that is what the remote node supports.
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
  zpal_radio_transmit_parameter_t txParameters100k = {.speed = ZPAL_RADIO_SPEED_100K,// Speed 100K=0010.
                                       .channel_id = 0,       // Channel0=0x00, Channel1=0x01, Channel2=0x02.
                                       .crc = 0x02,          // CRC 16bit=2
                                       .preamble_length = 24, // 24 bytes preamble on 3ck@100K
                                       .repeats = 0};        // No repeats (No wakeup beam)

  uint8_t          transferPresentationFrameHeaderLength = 10; // The Transfer presentation as it is expected to be seen on the phy layer.
  uint8_t          transferPresentationFramePayloadLength = 3;
  uint8_t          transferPresentationFullFrame[]     = {0xDC, 0x67, 0xCD, 0x37, 0x01, 0x01, 0x00, 0x0F, 0x25, 0xFF, // HomeId, Src, Flags1, Flags2, Length, Dst
                                                          0x01, 0x08, 0x05,
                                                          0xB2, 0x4A}; // CRC

  uint8_t          assignIdFrameHeaderLength = 10;
  uint8_t          assignIdFramePayloadLength = 7;
  uint8_t          assignIdFullFrame[]     = {0xC9, 0x2F, 0x7F, 0xAD, 0x01, 0x81, 0x00, 0x13, 0x26, 0x00, // HomeId, Src, Flags1, Flags2, Length, Dst
                                              0x01, 0x03, 0x02, 0xDC, 0x67, 0xCD, 0x37,
                                              0x4B, 0x24}; // CRC

  uint8_t          findNodesInRangeFrameHeaderLength = 10;
  uint8_t          findNodesInRangeFramePayloadLength = 6;
  uint8_t          findNodesInRangeFullFrame[]     = {0xDC, 0x67, 0xCD, 0x37, 0x01, 0x81, 0x00, 0x12, 0x28, 0x02, // HomeId, Src, Flags1, Flags2, Length, Dst
                                                      0x01, 0x04, 0x01, 0x01, 0x00, 0x03,
                                                      0x23, 0x80}; // CRC

  // Broad cast@9.6k
  mock_call_expect(TO_STR(zpal_radio_transmit), &p_mock);
  p_mock->expect_arg[0].pointer = &txParameters100k;
  p_mock->expect_arg[1].value   = transferPresentationFrameHeaderLength;
  p_mock->expect_arg[2].pointer = &transferPresentationFullFrame;
  p_mock->expect_arg[3].value   = transferPresentationFramePayloadLength;
  p_mock->expect_arg[4].pointer = &transferPresentationFullFrame[transferPresentationFrameHeaderLength];
  p_mock->expect_arg[5].value   = 1;

  // single cast@9.6k
  mock_call_expect(TO_STR(zpal_radio_transmit), &p_mock);
  p_mock->expect_arg[0].pointer = &txParameters100k;
  p_mock->expect_arg[1].value   = assignIdFrameHeaderLength;
  p_mock->expect_arg[2].pointer = &assignIdFullFrame;
  p_mock->expect_arg[3].value   = assignIdFramePayloadLength;
  p_mock->expect_arg[4].pointer = &assignIdFullFrame[assignIdFrameHeaderLength];
  p_mock->expect_arg[5].value   = 1;

  // single cast@40k
  mock_call_expect(TO_STR(zpal_radio_transmit), &p_mock);
  p_mock->expect_arg[0].pointer = &txParameters100k;
  p_mock->expect_arg[1].value   = findNodesInRangeFrameHeaderLength;
  p_mock->expect_arg[2].pointer = &findNodesInRangeFullFrame;
  p_mock->expect_arg[3].value   = findNodesInRangeFramePayloadLength;
  p_mock->expect_arg[4].pointer = &findNodesInRangeFullFrame[findNodesInRangeFrameHeaderLength];
  p_mock->expect_arg[5].value   = 1;

  /***** Execute the test *****/

  uint8_t          transferPresentationLength = 3;
  uint8_t          transferPresentation[]     = {0x01, 0x08, 0x05};

  uint8_t          assignIdLength = 7;
  uint8_t          assignId[]     = {0x01, 0x03, 0x02, 0xDC, 0x67, 0xCD, 0x37};

  uint8_t          findNodesInRangeLength = 6;
  uint8_t          findNodesInRange[]     = {0x01, 0x04, 0x01, 0x01, 0x00, 0x03};

  uint8_t      frameBuffer[sizeof(ZW_TransmissionFrame_t) + 7 + 1] = {0, };
  ZW_TransmissionFrame_t * pTestFrame = (ZW_TransmissionFrame_t *)frameBuffer;

  ZW_ReturnCode_t radioInitReturnValueExpected = SUCCESS;
  ZW_ReturnCode_t actualVal;
//  uint8_t speed = 0b00000010; // 9.6k and 40k supported
//  uint8_t nodewakeup = 0x00;  // Always listen
  // TODO: When API to phy layer is defined, create corresponding mock for test verification.

  zpal_radio_profile_t radioProfile = { REGION_JP, ZPAL_RADIO_WAKEUP_ALWAYS_LISTEN };
/** As correct radio initialization is ensured in other tests, we just utilize the helper function in this test. */
  helper_func_expect_radio_init_jp(&radioProfile, p_mock);
  actualVal = llInit(&radioProfile);
  TEST_ASSERT_EQUAL(radioInitReturnValueExpected, actualVal);

  CommunicationProfile_t communicationProfile = RF_PROFILE_3CH_100K_CH_A;
  pTestFrame->useLBT=1;
  // First transmission of a transfer presentation
  pTestFrame->frameOptions.frameType = HDRTYP_SINGLECAST;
  pTestFrame->frameOptions.homeId[0] = 0xDC;
  pTestFrame->frameOptions.homeId[1] = 0x67;
  pTestFrame->frameOptions.homeId[2] = 0xCD;
  pTestFrame->frameOptions.homeId[3] = 0x37;
  pTestFrame->frameOptions.sourceNodeId      = 1;
  pTestFrame->frameOptions.destinationNodeId = 0xFF;
  pTestFrame->frameOptions.sequenceNumber    = 37;
  pTestFrame->headerLength = transferPresentationFrameHeaderLength;
  pTestFrame->payloadLength = transferPresentationLength;
  memcpy(pTestFrame->payload, transferPresentation, pTestFrame->payloadLength);
  actualVal = llTransmitFrame(communicationProfile, pTestFrame);
  TEST_ASSERT_EQUAL(SUCCESS, actualVal);

  // Second transmission of an assign id
  pTestFrame->frameOptions.frameType = HDRTYP_SINGLECAST;
  pTestFrame->frameOptions.homeId[0] = 0xC9;
  pTestFrame->frameOptions.homeId[1] = 0x2F;
  pTestFrame->frameOptions.homeId[2] = 0x7F;
  pTestFrame->frameOptions.homeId[3] = 0xAD;
  pTestFrame->frameOptions.sourceNodeId      = 1;
  pTestFrame->frameOptions.destinationNodeId = 0;
  pTestFrame->frameOptions.acknowledge       = 1;
  pTestFrame->frameOptions.sequenceNumber    = 38;
  pTestFrame->headerLength = assignIdFrameHeaderLength;
  pTestFrame->payloadLength = assignIdLength;
  memcpy(pTestFrame->payload, assignId, pTestFrame->payloadLength);
  actualVal = llTransmitFrame(communicationProfile, pTestFrame);
  TEST_ASSERT_EQUAL(SUCCESS, actualVal);

  pTestFrame->frameOptions.frameType = HDRTYP_SINGLECAST;
  pTestFrame->frameOptions.homeId[0] = 0xDC;
  pTestFrame->frameOptions.homeId[1] = 0x67;
  pTestFrame->frameOptions.homeId[2] = 0xCD;
  pTestFrame->frameOptions.homeId[3] = 0x37;
  pTestFrame->frameOptions.sourceNodeId      = 1;
  pTestFrame->frameOptions.destinationNodeId = 2;
  pTestFrame->frameOptions.acknowledge       = 1;
  pTestFrame->frameOptions.sequenceNumber    = 40;
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
  mock_t* p_mock = NULL;

  /** Ensure that in case a pFrame is a null pointer, then the link layer return invalid parameters as return code.
   */
  ZW_ReturnCode_t actualVal;

  zpal_radio_profile_t radioProfile = { REGION_JP, ZPAL_RADIO_WAKEUP_ALWAYS_LISTEN };
/** As correct radio initialization is ensured in other tests, we just utilize the helper function in this test. */
  helper_func_expect_radio_init_jp(&radioProfile, p_mock);
  actualVal = llInit(&radioProfile);
  TEST_ASSERT_EQUAL(SUCCESS, actualVal);

  CommunicationProfile_t communicationProfile = RF_PROFILE_3CH_100K_CH_A;

  // First transmission of a transfer presentation
  actualVal = llTransmitFrame(communicationProfile, NULL);
  TEST_ASSERT_EQUAL(INVALID_PARAMETERS, actualVal);
}

/**
 * Test that emulates transfer of a frame with wakeup beam requested.
 */
void test_speed_100k_fragmented_wakeup_beam_transmit_jp(void)
{
  TEST_IGNORE_MESSAGE("Disabled until fragmented beam is ready on 700 series.");
  mock_t * p_mock = NULL;

  /** When transmitting a frame with wakeup beam, it is expected that two calls ZW_RadioTransmit to phy layer will occur.
   *  This test ensures that a wakeup beam and the frame can be correctly transmitted @40 speed as that is what the remote node supports.
   *  Self node supports all speed variants.
   *  This test transmits a wakeup beam  and the actual frame.
   *  1) Transmit an assign id=0x02 and home id=0xCAFEBABF. (TODO Adjust to real FLiRS frame)
   */
  zpal_radio_transmit_parameter_t txParameters100kWakeup = {.speed = ZPAL_RADIO_SPEED_100K,// Speed 9.6K=1000, Speed 40K=0100, Speed 100K=0010.
                                             .channel_id = 0,       // Channel0=0x00, Channel1=0x01, Channel2=0x02.
                                             .crc = 0x00,          // CRC 8bit=0, CRC 16bit=1
                                             .preamble_length = 8, // 20 bytes preamble on 40K wakeup beam
                                             .repeats = 58};       // Wakeup beam should be repeated for 100 ms, equivilent to 58 repeats

  uint8_t          wakeupFrameLength    = 3;
  uint8_t          wakeupFrame[]        = {0x55, 0x02, 0xCE};

  // Fragmented wakeup@100k
  // This is currently copied from 2ch test case, adopt to fragmented beams.
  mock_call_expect(TO_STR(zpal_radio_transmit), &p_mock);
  p_mock->expect_arg[0].pointer = &txParameters100kWakeup;
  p_mock->expect_arg[1].value   = wakeupFrameLength;
  p_mock->expect_arg[2].pointer = &wakeupFrame;
  p_mock->expect_arg[3].value   = 1;


  ZW_ReturnCode_t actualVal;
  ZW_ReturnCode_t radioInitReturnValueExpected = SUCCESS;

  // Initialize our radio, which is always listen and all speeds.
  zpal_radio_profile_t radioProfile = { REGION_JP, ZPAL_RADIO_WAKEUP_ALWAYS_LISTEN };
/** As correct radio initialization is ensured in other tests, we just utilize the helper function in this test. */
  helper_func_expect_radio_init_jp(&radioProfile, p_mock);
  actualVal = llInit(&radioProfile);
  TEST_ASSERT_EQUAL(radioInitReturnValueExpected, actualVal);

  CommunicationProfile_t communicationProfile = RF_PROFILE_3CH_100K_WAKEUP_FRAGMENTED;

  uint8_t      frameBuffer[sizeof(ZW_TransmissionFrame_t ) + 2 + 2] = {0, };
  ZW_TransmissionFrame_t * pFrame = (ZW_TransmissionFrame_t*) frameBuffer;
  memset(&pFrame->frameOptions, 0, sizeof(ZW_BasicFrameOptions_t));
  pFrame->frameOptions.frameType = HDRTYP_SINGLECAST;
  pFrame->frameOptions.homeId[0] = 0xDC;
  pFrame->frameOptions.homeId[1] = 0x67;
  pFrame->frameOptions.homeId[2] = 0xCD;
  pFrame->frameOptions.homeId[3] = 0x37;
  pFrame->frameOptions.sourceNodeId      = 1;
  pFrame->frameOptions.destinationNodeId = 2;
  pFrame->frameOptions.acknowledge       = 1;
  pFrame->frameOptions.sequenceNumber    = 43;
  pFrame->payloadLength = 2;    // Length of payload
  pFrame->payload[0]    = 0x5e; // Z-Wave protocol Command Class version 1, Z-Wave Plus
  pFrame->payload[1]    = 0x01; // Z-Wave command: Node Info get

  actualVal = llTransmitFrame(communicationProfile, pFrame);
  TEST_ASSERT_EQUAL(SUCCESS, actualVal);
}

// TODO use these unit-tests or remove
///**
// * Test that emulates transfer of a frame with wakeup beam requested.
// */
//void test_speed_40k_wakeup_beam_1000_ms_transmit_eu(void)
//{
//  /** As correct radio initialization is ensured in other tests, we just utilize the helper function in this test. */
//  helper_func_expect_radio_init_eu(0x06);
//
//  /***** Expectation setup *****/
//
//
//  /** When transmitting a frame with wakeup beam, it is expected that two calls ZW_RadioTransmit to phy layer will occur.
//   *  This test ensures that a wakeup beam and the frame can be correctly transmitted @40 speed as that is what the remote node supports.
//   *  Self node supports all speed variants.
//   *  This test transmits a wakeup beam  and the actual frame.
//   *  1) Transmit an assign id=0x02 and home id=0xCAFEBABF. (TODO Adjust to real FLiRS frame)
//   */
//
//  mock_t * p_mock = NULL;
//  zpal_radio_transmit_parameter_t txParameters40kWakeup = {.speed = ZPAL_RADIO_SPEED_40K,// Speed 9.6K=1000, Speed 40K=0100, Speed 100K=0010.
//                                            .channelId = 0,       // Channel0=0x00, Channel1=0x01, Channel2=0x02.
//                                            .crc = 0x00,          // CRC 8bit=0, CRC 16bit=1
//                                            .preamble_length = 20, // 20 bytes preamble on 40K wakeup beam
//                                            .repeats = 230};       // Wakeup beam should be repeated for 100 ms, equivilent to 58 repeats
//
//  zpal_radio_transmit_parameter_t txParameters40k = {.speed = ZPAL_RADIO_SPEED_40K,// Speed 9.6K=1000, Speed 40K=0100, Speed 100K=0010.
//                                      .channelId = 0,       // Channel0=0x00, Channel1=0x01, Channel2=0x02.
//                                      .crc = 0x01,          // CRC 8bit=0, CRC 16bit=1
//                                      .preamble_length = 20, // 10 bytes preamble on 40K
//                                      .repeats = 0};        // Not a wakeup beam
//
//  uint8_t          wakeupFrameLength    = 3;
//  uint8_t          wakeupFrame[]        = {0x55, 0x02, 0xCE};
//
//  uint8_t          nodeInforGetLength = 12;
//  uint8_t          nodeInforGet[]     = {0xca, 0xfe, 0xba, 0xbf, 0x01, 0x41, 0x0a, 0x0c, 0x02, // HomeId, Src, Flags1, Flags2, Length, Dst
//                                         0x5e, 0x01,                                           // CC Plus vers, InfoGet,
//                                         0xD5};                                                // Crc.
//
//  // wakeup@40k
//  mock_call_expect(TO_STR(zpal_radio_transmit), &p_mock);
//  p_mock->expect_arg[0].pointer = &txParameters40kWakeup;
//  p_mock->expect_arg[1].value   = wakeupFrameLength;
//  p_mock->expect_arg[2].pointer = &wakeupFrame;
//p_mock->expect_arg[3].value   = 1;
//
//  // single cast@40k
//  mock_call_expect(TO_STR(zpal_radio_transmit), &p_mock);
//  p_mock->expect_arg[0].pointer = &txParameters40k;
//  p_mock->expect_arg[1].value   = nodeInforGetLength;
//  p_mock->expect_arg[2].pointer = &nodeInforGet;
//p_mock->expect_arg[3].value   = 1;
//
//
//  ZW_ReturnCode_t actualVal;
//  ZW_ReturnCode_t radioInitReturnValueExpected = SUCCESS;
//  uint8_t speed = 0b00000010; // 9.6k and 40k supported
//  uint8_t nodewakeup = 0x02;  // Node wakeup 250 ms
//
//  // Initialize our radio, which is always listen and all speeds.
//  zpal_radio_profile_t radioProfile = { REGION_EU, ZPAL_RADIO_WAKEUP_ALWAYS_LISTEN };
//  actualVal = llInit(&radioProfile);
//  TEST_ASSERT_EQUAL(radioInitReturnValueExpected, actualVal);
//
//  CommunicationProfile_t communicationProfile = RF_PROFILE_40K_WAKEUP_1000;
//
//  uint8_t      frameBuffer[sizeof(ZW_TransmissionFrame_t ) + 2 + 1] = {0, };
//  ZW_TransmissionFrame_t * pFrame = (ZW_TransmissionFrame_t*) frameBuffer;
//  memset(&pFrame->frameOptions, 0, sizeof(ZW_BasicFrameOptions_t));
//  pFrame->frameOptions.frameType = HDRTYP_SINGLECAST;
//  pFrame->frameOptions.homeId[0] = 0xCA;
//  pFrame->frameOptions.homeId[1] = 0xFE;
//  pFrame->frameOptions.homeId[2] = 0xBA;
//  pFrame->frameOptions.homeId[3] = 0xBF;
//  pFrame->frameOptions.sourceNodeId      = 1;
//  pFrame->frameOptions.destinationNodeId = 2;
//  pFrame->frameOptions.acknowledge       = 1;
//  pFrame->frameOptions.sequenceNumber    = 10;
//  pFrame->payloadLength = 2;    // Length of payload
//  pFrame->payload[0]    = 0x5e; // Z-Wave protocol Command Class version 1, Z-Wave Plus
//  pFrame->payload[1]    = 0x01; // Z-Wave command: Node Info get
//
//  actualVal = llTransmitFrame(communicationProfile, pFrame);
//  TEST_ASSERT_EQUAL(SUCCESS, actualVal);
//}
//
///**
// * Test that verifies that a frame sent a lower spped will have the speed modified bit set.
// */
//void test_speed_9_6k_retransmit_low_speed_eu(void)
//{
//  /** As correct radio initialization is ensured in other tests, we just utilize the helper function in this test. */
//  helper_func_expect_radio_init_eu(0x06);
//
//  /***** Expectation setup *****/
//
//  mock_t * p_mock = NULL;
//
//  zpal_radio_transmit_parameter_t txParameters9_6k = {.speed = ZPAL_RADIO_SPEED_9600,// Speed 9.6K=1000, Speed 40K=0100, Speed 100K=0010.
//                                       .channelId = 0,       // Channel0=0x00, Channel1=0x01, Channel2=0x02.
//                                       .crc = 0x01,          // CRC 8bit=0, CRC 16bit=1
//                                       .preamble_length = 10, // 10 bytes preamble on 9.6K
//                                       .repeats = 0};        // Not a wakeup beam
//
//  uint8_t          nodeInforGetLength = 12;
//  uint8_t          nodeInforGet[]     = {0xca, 0xfe, 0xba, 0xbf, 0x01, 0x51, 0x0a, 0x0c, 0x02, // HomeId, Src, Flags1, Flags2, Length, Dst
//                                         0x5e, 0x01,                                           // CC Plus vers, InfoGet,
//                                         0xC5};                                                // Crc
//
//
//  // single cast@40k
//  mock_call_expect(TO_STR(zpal_radio_transmit), &p_mock);
//  p_mock->expect_arg[0].pointer = &txParameters9_6k;
//  p_mock->expect_arg[1].value   = nodeInforGetLength;
//  p_mock->expect_arg[2].pointer = &nodeInforGet;
//p_mock->expect_arg[3].value   = 1;
//
//
//  ZW_ReturnCode_t actualVal;
//  ZW_ReturnCode_t radioInitReturnValueExpected = SUCCESS;
//  uint8_t speed = 0b00000010; // 9.6k and 40k supported
//  uint8_t nodewakeup = 0x00;  // Always listen
//
//  // Initialize our radio, which is always listen and all speeds.
//  zpal_radio_profile_t radioProfile = { REGION_EU, BAUD_RATE_9_6k_40k | BAUD_RATE_100k, ZPAL_RADIO_WAKEUP_ALWAYS_LISTEN };
//  actualVal = llInit(&radioProfile);
//  TEST_ASSERT_EQUAL(radioInitReturnValueExpected, actualVal);
//
//  CommunicationProfile_t communicationProfile = RF_PROFILE_9_6K;
//
//  uint8_t      frameBuffer[sizeof(ZW_TransmissionFrame_t ) + 2 + 1] = {0, };
//  ZW_TransmissionFrame_t * pFrame = (ZW_TransmissionFrame_t*) frameBuffer;
//  pFrame->frameOptions.frameType = HDRTYP_SINGLECAST;
//  pFrame->frameOptions.homeId[0] = 0xCA;
//  pFrame->frameOptions.homeId[1] = 0xFE;
//  pFrame->frameOptions.homeId[2] = 0xBA;
//  pFrame->frameOptions.homeId[3] = 0xBF;
//  pFrame->frameOptions.sourceNodeId      = 1;
//  pFrame->frameOptions.destinationNodeId = 2;
//  pFrame->frameOptions.acknowledge       = 1;
//  pFrame->frameOptions.speedModified     = 1;
//  pFrame->frameOptions.sequenceNumber    = 10;
//  pFrame->payloadLength = 2;    // Length of payload
//  pFrame->payload[0]    = 0x5e; // Z-Wave protocol Command Class version 1, Z-Wave Plus
//  pFrame->payload[1]    = 0x01; // Z-Wave command: Node Info get
//
//  actualVal = llTransmitFrame(communicationProfile, pFrame);
//  TEST_ASSERT_EQUAL(SUCCESS, actualVal);
//}
//
///**
// * Test that verifies that a frame sent a lower spped will have the speed modified bit set.
// */
//void test_speed_100k_transmit_get_info_eu(void)
//{
//  /** As correct radio initialization is ensured in other tests, we just utilize the helper function in this test. */
//  helper_func_expect_radio_init_eu(0x06);
//
//  /***** Expectation setup *****/
//
//  mock_t * p_mock = NULL;
//
//  zpal_radio_transmit_parameter_t txParameters100k = {.speed = ZPAL_RADIO_SPEED_100K,// Speed 9.6K=1000, Speed 40K=0100, Speed 100K=0010.
//                                       .channelId = 1,       // Channel0=0x00, Channel1=0x01, Channel2=0x02.
//                                       .crc = 0x02,          // CRC 8bit=0, CRC 16bit=2
//                                       .preamble_length = 40, // 40 bytes preamble on 100K
//                                       .repeats = 0};        // Not a wakeup beam
//
//  uint8_t          nodeInforGetLength = 13;
//  uint8_t          nodeInforGet[]     = {0xca, 0xfe, 0xba, 0xbf, 0x01, 0x51, 0x0a, 0x0d, 0x02, // HomeId, Src, Flags1, Flags2, Length, Dst
//                                         0x5e, 0x01,                                           // CC Plus vers, InfoGet,
//                                         0x82, 0x34};                                          // Crc - ccitt
//
//
//  // single cast@40k
//  mock_call_expect(TO_STR(zpal_radio_transmit), &p_mock);
//  p_mock->expect_arg[0].pointer = &txParameters100k;
//  p_mock->expect_arg[1].value   = nodeInforGetLength;
//  p_mock->expect_arg[2].pointer = &nodeInforGet;
//p_mock->expect_arg[3].value   = 1;
//
//
//  ZW_ReturnCode_t actualVal;
//  ZW_ReturnCode_t radioInitReturnValueExpected = SUCCESS;
//  uint8_t speed = 0b00000110; // 9.6k, 40k, and 100k supported
//  uint8_t nodewakeup = 0x00;  // Always listen
//
//  // Initialize our radio, which is always listen and all speeds.
//  zpal_radio_profile_t radioProfile = { REGION_EU, ZPAL_RADIO_WAKEUP_ALWAYS_LISTEN };
//  actualVal = llInit(&radioProfile);
//  TEST_ASSERT_EQUAL(radioInitReturnValueExpected, actualVal);
//
//  CommunicationProfile_t communicationProfile = RF_PROFILE_100K;
//
//  uint8_t      frameBuffer[sizeof(ZW_TransmissionFrame_t ) + 2 + 2] = {0,};
//  ZW_TransmissionFrame_t * pFrame = (ZW_TransmissionFrame_t*) frameBuffer;
//  pFrame->frameOptions.frameType = HDRTYP_SINGLECAST;
//  pFrame->frameOptions.homeId[0] = 0xCA;
//  pFrame->frameOptions.homeId[1] = 0xFE;
//  pFrame->frameOptions.homeId[2] = 0xBA;
//  pFrame->frameOptions.homeId[3] = 0xBF;
//  pFrame->frameOptions.sourceNodeId      = 1;
//  pFrame->frameOptions.destinationNodeId = 2;
//  pFrame->frameOptions.acknowledge       = 1;
//  pFrame->frameOptions.speedModified     = 1;
//  pFrame->frameOptions.sequenceNumber    = 10;
//  pFrame->payloadLength = 2;    // Length of payload
//  pFrame->payload[0]    = 0x5e; // Z-Wave protocol Command Class version 1, Z-Wave Plus
//  pFrame->payload[1]    = 0x01; // Z-Wave command: Node Info get
//
//  actualVal = llTransmitFrame(communicationProfile, pFrame);
//  TEST_ASSERT_EQUAL(SUCCESS, actualVal);
//}
//
///**
// * Test that verifies that a frame sent a lower spped will have the speed modified bit set.
// */
//void test_speed_100k_Retransmit_get_info_eu(void)
//{
//  /** As correct radio initialization is ensured in other tests, we just utilize the helper function in this test. */
//  helper_func_expect_radio_init_eu(0x06);
//
//  /***** Expectation setup *****/
//
//  mock_t * p_mock = NULL;
//
//  zpal_radio_transmit_parameter_t txParameters100k = {.speed = ZPAL_RADIO_SPEED_100K,// Speed 9.6K=1000, Speed 40K=0100, Speed 100K=0010.
//                                       .channelId = 1,       // Channel0=0x00, Channel1=0x01, Channel2=0x02.
//                                       .crc = 0x02,          // CRC 8bit=0, CRC 16bit=2
//                                       .preamble_length = 40, // 40 bytes preamble on 100K
//                                       .repeats = 0};        // Not a wakeup beam
//
//  uint8_t          nodeInforGetLength = 13;
//  uint8_t          nodeInforGet[]     = {0xca, 0xfe, 0xba, 0xbf, 0x01, 0x51, 0x0a, 0x0d, 0x02, // HomeId, Src, Flags1, Flags2, Length, Dst
//                                         0x5e, 0x01,                                           // CC Plus vers, InfoGet,
//                                         0x82, 0x34};                                          // Crc - ccitt
//
//
//  // single cast@40k
//  mock_call_expect(TO_STR(zpal_radio_transmit), &p_mock);
//  p_mock->expect_arg[0].pointer = &txParameters100k;
//  p_mock->expect_arg[1].value   = nodeInforGetLength;
//  p_mock->expect_arg[2].pointer = &nodeInforGet;
//p_mock->expect_arg[3].value   = 1;
//
//  mock_call_expect(TO_STR(zpal_radio_transmit), &p_mock);
//  p_mock->expect_arg[0].pointer = &txParameters100k;
//  p_mock->expect_arg[1].value   = nodeInforGetLength;
//  p_mock->expect_arg[2].pointer = &nodeInforGet;
//p_mock->expect_arg[3].value   = 1;
//
//
//  ZW_ReturnCode_t actualVal;
//  ZW_ReturnCode_t radioInitReturnValueExpected = SUCCESS;
//  uint8_t speed = 0b00000110; // 9.6k, 40k, and 100k supported
//  uint8_t nodewakeup = 0x00;  // Always listen
//
//  // Initialize our radio, which is always listen and all speeds.
//  zpal_radio_profile_t radioProfile = { REGION_EU, ZPAL_RADIO_WAKEUP_ALWAYS_LISTEN };
//  actualVal = llInit(&radioProfile);
//  TEST_ASSERT_EQUAL(radioInitReturnValueExpected, actualVal);
//
//  CommunicationProfile_t communicationProfile = RF_PROFILE_100K;
//
//  uint8_t      frameBuffer[sizeof(ZW_TransmissionFrame_t ) + 2 + 2] = {0,};
//  ZW_TransmissionFrame_t * pFrame = (ZW_TransmissionFrame_t*) frameBuffer;
//  pFrame->frameOptions.frameType = HDRTYP_SINGLECAST;
//  pFrame->frameOptions.homeId[0] = 0xCA;
//  pFrame->frameOptions.homeId[1] = 0xFE;
//  pFrame->frameOptions.homeId[2] = 0xBA;
//  pFrame->frameOptions.homeId[3] = 0xBF;
//  pFrame->frameOptions.sourceNodeId      = 1;
//  pFrame->frameOptions.destinationNodeId = 2;
//  pFrame->frameOptions.acknowledge       = 1;
//  pFrame->frameOptions.speedModified     = 1;
//  pFrame->frameOptions.sequenceNumber    = 10;
//  pFrame->payloadLength = 2;    // Length of payload
//  pFrame->payload[0]    = 0x5e; // Z-Wave protocol Command Class version 1, Z-Wave Plus
//  pFrame->payload[1]    = 0x01; // Z-Wave command: Node Info get
//
//  actualVal = llTransmitFrame(communicationProfile, pFrame);
//  TEST_ASSERT_EQUAL(SUCCESS, actualVal);
//
//  llReTransmitStart(pFrame);
//  actualVal = llTransmitFrame(communicationProfile, pFrame);
//  TEST_ASSERT_EQUAL(SUCCESS, actualVal);
//
//}
//
