// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file TestTransmitAckLinkLayer_LR.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include <unity.h>
#include <mock_control.h>
#include <stdio.h>
#include <string.h>

#include "ZW_DataLinkLayer.h"
#include "ZW_basis_api.h"

void setUpSuite(void) {

}

void tearDownSuite(void) {

}

void setUp(void) {

}

void tearDown(void) {

}

node_id_t g_nodeID = 256;

void test_tx_power_of_long_range_ack_frame(void)
{
  mock_calls_clear();

  mock_t * p_mock = NULL;

  mock_call_use_as_stub(TO_STR(zpal_radio_get_maximum_lr_tx_power));
  mock_call_use_as_stub(TO_STR(zpal_radio_get_minimum_lr_tx_power));
  mock_call_use_as_stub(TO_STR(zpal_radio_rf_channel_statistic_background_rssi_average_update));

  mock_call_use_as_fake(TO_STR(zpal_retention_register_write));
  mock_call_use_as_fake(TO_STR(zpal_retention_register_read));

  ZW_ReturnCode_t  actualVal;

  uint8_t      frameBuffer[sizeof(ZW_TransmissionFrame_t) + 1] = {0, };
  ZW_TransmissionFrame_t * pTestFrame = (ZW_TransmissionFrame_t *)frameBuffer;

  //Setup Ack frame
  pTestFrame->status = 0;
  pTestFrame->frameOptions.frameType = HDRTYP_TRANSFERACK;
  pTestFrame->frameOptions.homeId[0] = 0xCA;
  pTestFrame->frameOptions.homeId[1] = 0xFE;
  pTestFrame->frameOptions.homeId[2] = 0xBA;
  pTestFrame->frameOptions.homeId[3] = 0xBF;
  pTestFrame->frameOptions.sourceNodeId      = g_nodeID;
  pTestFrame->frameOptions.destinationNodeId = NODE_BROADCAST_LR;
  pTestFrame->frameOptions.sequenceNumber    = 3;
  pTestFrame->headerLength = 13;
  pTestFrame->payloadLength = 0;


  RSSI_LEVELS noise_levels = {{-97, -97, -97, -97}};

  //Tx power input of wakeup beams. (dBm)
  int8_t beamTxPower[] = {-6, -2,  2, 6, 10, 13, 16};

  //Verify that Acks are transmitted with the tx power of the frames they Ack.
  for (uint32_t i = 0; i<sizeof(beamTxPower); i++)
  {
    //Noise level is sampled, read and adjusted.
    mock_call_expect(TO_STR(SampleNoiseLevel), &p_mock);
    p_mock->expect_arg[0].value = true;

    mock_call_expect(TO_STR(ZW_GetBackgroundRSSI), &p_mock);
    p_mock->compare_rule_arg[0]   = COMPARE_NOT_NULL;
    p_mock->output_arg[0].p = &noise_levels;

    mock_call_expect(TO_STR(ZW_GetChannelIndexLR), &p_mock);
    p_mock->expect_arg[0].value = RF_PROFILE_100K_LR_A;
    p_mock->expect_arg[1].value = ZPAL_RADIO_LR_CH_CFG1;
    p_mock->return_code.value   = 0;

    mock_call_expect(TO_STR(zpal_radio_get_lr_channel_config), &p_mock);
    p_mock->return_code.value = ZPAL_RADIO_LR_CH_CFG1;

    mock_call_expect(TO_STR(zpal_radio_get_lr_channel_config), &p_mock);
    p_mock->return_code.value = ZPAL_RADIO_LR_CH_CFG1;

    //Expected LR Ack frame
    mock_call_expect(TO_STR(zpal_radio_transmit), &p_mock);
    p_mock->compare_rule_arg[0]   = COMPARE_NOT_NULL;
    p_mock->expect_arg[1].value   = 13;               //LR AckFrame HeaderLength
    p_mock->compare_rule_arg[2]   = COMPARE_NOT_NULL;
    p_mock->expect_arg[3].value   = 0;                //LR AckFrame PayloadLength
    p_mock->compare_rule_arg[4]   = COMPARE_NOT_NULL;
    p_mock->expect_arg[5].value   = 0; //useLBT
    p_mock->expect_arg[6].value   = beamTxPower[i];   //Tx power of Ack. Expected to be the txPower of pTestFrame below.


    // Transmit Ack frame using 100k_LR_A.
    pTestFrame->txPower = beamTxPower[i];
    actualVal = llTransmitFrame(RF_PROFILE_100K_LR_A, pTestFrame);
    TEST_ASSERT_EQUAL(SUCCESS, actualVal);
  }

  mock_calls_verify();
}
