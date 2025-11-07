// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file Testreceivefilter_transport.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include <unity.h>
#include <mock_control.h>
#include <stdio.h>
#include <string.h>

#include "ZW_DataLinkLayer.h"
#include <zpal_radio.h>
#include "ZW_receivefilter_transport.h"

void setUpSuite(void) {

}

void tearDownSuite(void) {

}

static bool mReceiveHandlerCalled = false;
static uint8_t                 receive_buffer[sizeof(zpal_radio_receive_frame_t) + 255]; // Let's allocate 255 frame content payload for received frames for testing.
static zpal_radio_receive_frame_t *  mp_zpal_received_frame = (zpal_radio_receive_frame_t*) receive_buffer;
static ZW_ReceiveFrame_t    m_received_frame;
static ZW_ReceiveFrame_t *  mp_received_frame;
static ZW_ReceiveFrame_t *  mpReceiveHandlerReceivedFrame;

void setUp(void)
{
  //This is run before EACH TEST
  mock_calls_clear();
  mpReceiveHandlerReceivedFrame = NULL;
  mReceiveHandlerCalled = false;
  m_received_frame.frameContent = mp_zpal_received_frame->frame_content;
  mp_received_frame = &m_received_frame;
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

void ReceiveHandler(ZW_ReceiveFrame_t * pFrame)
{
  mReceiveHandlerCalled = true;
  mpReceiveHandlerReceivedFrame = pFrame;
}

/** Verification that the Data Link Layer can filter frames based upon payload filter, with home id = FF FF FF FF.
 *
 */
void test_transport_filter_frames(void)
{
  mock_t* p_mock = NULL;
  zpal_radio_profile_t radioProfile = { REGION_EU, ZPAL_RADIO_WAKEUP_ALWAYS_LISTEN };
  /** As correct radio initialization is ensured in other tests, we just utilize the helper function in this test. */
  helper_func_expect_radio_init_eu(&radioProfile, p_mock);
  zpal_radio_rx_parameters_t rxParameters = {.speed = ZPAL_RADIO_SPEED_40K,// Speed 9.6K=1000, Speed 40K=0100, Speed 100K=0010.
                                   .channel_id = 1,
                                   .rssi = 0};

  uint8_t          nifFrame[] = {0xF3, 0x8B, 0xE3, 0x4C, 0x00, 0x01, 0x02, 0x17, 0xFF,                         // HomeId, Src, Flags1, Flags2, Length, Dst
                                 0x01, 0x01, 0xD3, 0x9C, 0x01, 0x10, 0x01, 0x5E, 0x86, 0x72, 0x5A, 0x73, 0x98, // CC vers, NIF, Capabilities, Security, Properties, CC supported,
                                 0x87}; // Crc.
  ZW_ReturnCode_t actualVal;

  actualVal = llInit(&radioProfile);
  TEST_ASSERT_EQUAL(SUCCESS, actualVal);
  TEST_ASSERT_FALSE(mReceiveHandlerCalled);

  mp_received_frame->frameContentLength = sizeof(nifFrame);
  mp_received_frame->channelHeaderFormat = HDRFORMATTYP_2CH;
  memcpy(mp_received_frame->frameContent, nifFrame, mp_received_frame->frameContentLength);
    radioProfile.receive_handler_cb(&rxParameters, mp_zpal_received_frame);
  // No filter defined, thus frame should be discarded.
  TEST_ASSERT_FALSE(mReceiveHandlerCalled);

  actualVal = rfTransportFilter_Set();
  TEST_ASSERT_EQUAL(SUCCESS, actualVal);

    radioProfile.receive_handler_cb(&rxParameters, mp_zpal_received_frame);
  TEST_ASSERT_TRUE(mReceiveHandlerCalled);

  // Pause all active Filters
  llReceiveFilterPause(true);

  mReceiveHandlerCalled = false;
  TEST_ASSERT_FALSE(mReceiveHandlerCalled);

    radioProfile.receive_handler_cb(&rxParameters, mp_zpal_received_frame);
  // All active Filters Paused - no frames gets through
  TEST_ASSERT_FALSE(mReceiveHandlerCalled);

  // Re enable all active Filters
  llReceiveFilterPause(false);

    radioProfile.receive_handler_cb(&rxParameters, mp_zpal_received_frame);
  TEST_ASSERT_TRUE(mReceiveHandlerCalled);
}

