// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file Testreceivefilter_learnmode.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include <unity.h>
#include <mock_control.h>
#include <stdio.h>
#include <string.h>
#include <zpal_radio.h>
#include "ZW_receivefilter_learnmode.h"

void setUpSuite(void) {

}

void tearDownSuite(void) {

}

static bool mTestFilterReceivedHandlerCalled = false;
static uint8_t                 receive_buffer[sizeof(zpal_radio_receive_frame_t) + 255]; // Let's allocate 255 frame content payload for received frames for testing.
static zpal_radio_receive_frame_t *  mp_zpal_received_frame = (zpal_radio_receive_frame_t*) receive_buffer;
static ZW_ReceiveFrame_t mTestFilterReceivedHandlerReceivedFrame;

void setUp(void)
{
  //This is run before EACH TEST
  mock_calls_clear();
  mTestFilterReceivedHandlerCalled = false;
}

void tearDown(void)
{
  mock_calls_verify();
}

void helper_func_expect_radio_init_eu(zpal_radio_profile_t *pRfProfile, mock_t *p_mock)
{
  // Setup of test expectations. When configuration the Data Link Layer, the following calls are expected further down to Phy Layer.
  // 1) zpal_radio_init, with configuration of 3 channels for 9.6k, 40k, 100k. And a Energy Harvesting channel (still to be defined)
  mock_call_expect(TO_STR(zpal_radio_init), &p_mock);
  p_mock->expect_arg[0].pointer = pRfProfile;
  p_mock->compare_rule_arg[1]   = COMPARE_NULL;
  p_mock->compare_rule_arg[2]   = COMPARE_NULL;
  p_mock->compare_rule_arg[3]   = COMPARE_NULL;
}

/**
 * ZW_receivefilter_learnmode will setup learnmode filters to call ReceiveHandler when frame matching a learnmode filter
 */
void ReceiveHandler(ZW_ReceiveFrame_t *pFrame)
{
  mTestFilterReceivedHandlerCalled = true;
  mTestFilterReceivedHandlerReceivedFrame = *pFrame;
}


/** Verification that the Data Link Layer can filter frames based upon payload filter, with home id = FF FF FF FF.
 *
 */
void test_learnmode_filter_frames(void)
{
  mock_t* p_mock = NULL;
  /** As correct radio initialization is ensured in other tests, we just utilize the helper function in this test. */
  zpal_radio_profile_t radioProfile = { REGION_EU, ZPAL_RADIO_WAKEUP_ALWAYS_LISTEN };
  helper_func_expect_radio_init_eu(&radioProfile, p_mock);
  zpal_radio_rx_parameters_t rxParameters = {
    .speed = ZPAL_RADIO_SPEED_40K,
    .channel_id = 1,
    .channel_header_format = ZPAL_RADIO_HEADER_TYPE_2CH,
    .rssi = 0
  };

  uint8_t          lHomeID[HOME_ID_LENGTH] = {0xF3, 0x8B, 0xE3, 0x4C};
  uint8_t          assignIDFrame[] = {0xCA, 0xD8, 0x6A, 0xE0, 0x01, 0x41, 0x0C, 0x11, 0x00, // CAD86AE0 01 41 0C 11 00 - HomeId, Src, Flags1, Flags2, Length, Dst
                                      0x01, 0x03, 0x02, 0xF4, 0x07, 0xB1, 0x8D,             // Protocol, AssignID, NodeID 02, homeID F407B18D
                                      0xF5};                                                // LRC F5
  uint8_t          transferPresentationFrame[] = {0xF4, 0x07, 0xB1, 0x8D, 0x01, 0x01, 0x04, 0x0D, 0xFF, // F407B18D 01 01 04 0D FF - HomeId, Src, Flags1, Flags2, Length, Dst
                                                  0x01, 0x08, 0x05,                         // Protocol, TransferPresentation, UNIQHOMEID+NOT_EXCLUSION
                                                  0xCA};

  uint8_t          nifFrame[] = {0xF3, 0x8B, 0xE3, 0x4C, 0x00, 0x01, 0x02, 0x17, 0xFF,                         // HomeId, Src, Flags1, Flags2, Length, Dst
                                 0x01, 0x01, 0xD3, 0x9C, 0x01, 0x10, 0x01, 0x5E, 0x86, 0x72, 0x5A, 0x73, 0x98, // CC vers, NIF, Capabilities, Security, Properties, CC supported,
                                 0x87}; // Crc.

  ZW_ReturnCode_t radioInitReturnValueExpected = SUCCESS;
  ZW_ReturnCode_t actualVal;

  mock_call_use_as_stub(TO_STR(zpal_radio_network_id_filter_set));

  actualVal = llInit(&radioProfile);
  TEST_ASSERT_EQUAL(radioInitReturnValueExpected, actualVal);

  TEST_ASSERT_FALSE(mTestFilterReceivedHandlerCalled);
  mp_zpal_received_frame->frame_content_length = sizeof(nifFrame);
  memcpy(mp_zpal_received_frame->frame_content, nifFrame, mp_zpal_received_frame->frame_content_length);
    radioProfile.receive_handler_cb(&rxParameters, mp_zpal_received_frame);
  // No filter defined, thus frame should be discarded.
  TEST_ASSERT_FALSE(mTestFilterReceivedHandlerCalled);

  actualVal = rfLearnModeFilter_Set(ZW_SET_LEARN_MODE_CLASSIC, 0, lHomeID);
  TEST_ASSERT_EQUAL(SUCCESS, actualVal);

    radioProfile.receive_handler_cb(&rxParameters, mp_zpal_received_frame);
  // Classic learnmode receivefilter defined - nif do not match receivefilter
  TEST_ASSERT_FALSE(mTestFilterReceivedHandlerCalled);

  mp_zpal_received_frame->frame_content_length = sizeof(assignIDFrame);
  memcpy(mp_zpal_received_frame->frame_content, assignIDFrame, mp_zpal_received_frame->frame_content_length);
    radioProfile.receive_handler_cb(&rxParameters, mp_zpal_received_frame);
  TEST_ASSERT_TRUE(mTestFilterReceivedHandlerCalled);

  mTestFilterReceivedHandlerCalled = false;
  TEST_ASSERT_FALSE(mTestFilterReceivedHandlerCalled);

  mp_zpal_received_frame->frame_content_length = sizeof(transferPresentationFrame);
  memcpy(mp_zpal_received_frame->frame_content, transferPresentationFrame, mp_zpal_received_frame->frame_content_length);
  radioProfile.receive_handler_cb(&rxParameters, mp_zpal_received_frame);
  TEST_ASSERT_TRUE(mTestFilterReceivedHandlerCalled);
}

