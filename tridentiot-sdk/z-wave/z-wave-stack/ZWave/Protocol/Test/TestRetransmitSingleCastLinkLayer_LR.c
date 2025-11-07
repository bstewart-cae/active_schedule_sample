// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file TestRetransmitSingleCastLinkLayer_LR.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include <unity.h>
#include <mock_control.h>
//#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "ZW_DataLinkLayer.h"
#include "ZW_basis_api.h"
#include <zpal_retention_register.h>
#include "ZW_dynamic_tx_power.h"

void setUpSuite(void) {

}

void tearDownSuite(void) {

}

void setUp(void) {

}

void tearDown(void) {

}

node_id_t g_nodeID = 256;

void test_tx_power_of_long_range_retransmit_frame(void)
{
  mock_calls_clear();

  mock_t * p_mock = NULL;

  mock_call_use_as_stub(TO_STR(zpal_radio_get_maximum_lr_tx_power));
  mock_call_use_as_stub(TO_STR(zpal_radio_get_minimum_lr_tx_power));
  mock_call_use_as_stub(TO_STR(zpal_radio_rf_channel_statistic_tx_retries));

  mock_call_use_as_fake(TO_STR(zpal_retention_register_write));
  mock_call_use_as_fake(TO_STR(zpal_retention_register_read));

  ZW_ReturnCode_t  actualVal;

  uint8_t      frameBuffer[sizeof(ZW_TransmissionFrame_t) + 1] = {0, };
  ZW_TransmissionFrame_t * pTestFrame = (ZW_TransmissionFrame_t *)frameBuffer;

  //Setup retransmit frame
  pTestFrame->status = 1;   //RETRANSMIT_FRAME
  pTestFrame->headerLength  = 12;
  pTestFrame->payloadLength = 2;
  pTestFrame->frameOptions.frameType = HDRTYP_SINGLECAST;
  pTestFrame->frameOptions.homeId[0] = 0xCA;
  pTestFrame->frameOptions.homeId[1] = 0xFE;
  pTestFrame->frameOptions.homeId[2] = 0xBA;
  pTestFrame->frameOptions.homeId[3] = 0xBF;
  pTestFrame->frameOptions.sourceNodeId      = g_nodeID;
  pTestFrame->frameOptions.destinationNodeId = 1;
  pTestFrame->frameOptions.sequenceNumber    = 3;


  int8_t rssi = -70;

  //Tx power input of last frame. (dBm)
  int8_t lastTxPower[] =  {-6, -2, 2, 6, 10, 13};
  //Tx power of retransmit frame. (dBm)
  int8_t retrTxPower[] =  {-3,  1, 2, 6, 10, 13};

  for (uint32_t i = 0; i<sizeof(lastTxPower); i++)
  {
    //Tx power and rssi of previous frame is stored to a retention register.
    SaveTxPowerAndRSSI(lastTxPower[i], rssi);

    //zpal_radio_get_protocol_mode is called twice
    mock_call_expect(TO_STR(zpal_radio_get_protocol_mode), &p_mock);
    p_mock->return_code.value = ZPAL_RADIO_PROTOCOL_MODE_4;
    mock_call_expect(TO_STR(zpal_radio_get_protocol_mode), &p_mock);
    p_mock->return_code.value = ZPAL_RADIO_PROTOCOL_MODE_4;


    //Expected LR frame
    mock_call_expect(TO_STR(zpal_radio_transmit), &p_mock);
    p_mock->compare_rule_arg[0]   = COMPARE_NOT_NULL;
    p_mock->expect_arg[1].value   = 12;               //Frame HeaderLength
    p_mock->compare_rule_arg[2]   = COMPARE_NOT_NULL;
    p_mock->expect_arg[3].value   = 2;                //Frame PayloadLength
    p_mock->compare_rule_arg[4]   = COMPARE_NOT_NULL;
    p_mock->expect_arg[5].value   = 0; //useLBT
    p_mock->expect_arg[6].value   = retrTxPower[i];   //Expected retransmit Tx power


    // Transmit retransmit frame using 100k_LR_A.
    actualVal = llTransmitFrame(RF_PROFILE_100K_LR_A, pTestFrame);
    TEST_ASSERT_EQUAL(SUCCESS, actualVal);
  }
  mock_calls_verify();
}


void test_tx_power_of_long_range_singlecast_frame(void)
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
  pTestFrame->frameOptions.frameType = HDRTYP_SINGLECAST;
  pTestFrame->frameOptions.homeId[0] = 0xCA;
  pTestFrame->frameOptions.homeId[1] = 0xFE;
  pTestFrame->frameOptions.homeId[2] = 0xBA;
  pTestFrame->frameOptions.homeId[3] = 0xBF;
  pTestFrame->frameOptions.sourceNodeId      = g_nodeID;
  pTestFrame->frameOptions.destinationNodeId = NODE_BROADCAST_LR;
  pTestFrame->frameOptions.sequenceNumber    = 3;
  pTestFrame->headerLength = 12;
  pTestFrame->payloadLength = 0;


  RSSI_LEVELS noise_levels = {{-97, -97, -97, -97}};

  //Tx power input of wakeup beam. (dBm)
  int8_t txPower  = 2;
  //Measured rssi of input of wakeup beams. (dBm)
  int8_t beamRssi[]      = {-110, -100, -90, -80, -70, -60, -50, -40, -30, -20};
  //Expected Tx power of Acks to wakeup beams. (dBm)
  int8_t ackTxPower[]  = {   5,    5,   2,   -1,  -1,  -1,  -1,  -1,  -1,  -1};



  //Test response to beamTxPower
  for (uint32_t i = 0; i<sizeof(ackTxPower); i++)
  {
    //Tx power and rssi of an incoming beam is stored to a retention register.
    SaveTxPowerAndRSSI(ackTxPower[i], beamRssi[0]);

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
    p_mock->expect_arg[1].value   = 12;               //LR singlecast Frame HeaderLength
    p_mock->compare_rule_arg[2]   = COMPARE_NOT_NULL;
    p_mock->expect_arg[3].value   = 0;                //LR singlecast frame PayloadLength
    p_mock->compare_rule_arg[4]   = COMPARE_NOT_NULL;
    p_mock->expect_arg[5].value   = 0; //useLBT
    p_mock->expect_arg[6].value   = 14;    //Expected Tx power of Ack


    // Transmit Ack frame using 100k_LR_A.
    actualVal = llTransmitFrame(RF_PROFILE_100K_LR_A, pTestFrame);
    TEST_ASSERT_EQUAL(SUCCESS, actualVal);
  }

  pTestFrame->frameOptions.destinationNodeId = 1;

  //Test response to beam rssi
  for (uint32_t i = 0; i<sizeof(beamRssi); i++)
  {
    //Tx power and rssi of an incoming beam is stored to a retention register.
    SaveTxPowerAndRSSI(txPower, beamRssi[i]);

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
    p_mock->expect_arg[1].value   = 12;               //LR singlecast Frame HeaderLength
    p_mock->compare_rule_arg[2]   = COMPARE_NOT_NULL;
    p_mock->expect_arg[3].value   = 0;                //LR singlecast frame PayloadLength
    p_mock->compare_rule_arg[4]   = COMPARE_NOT_NULL;
    p_mock->expect_arg[5].value   = 0; //useLBT

    p_mock->expect_arg[6].value   = ackTxPower[i];    //Expected Tx power of Ack


    // Transmit Ack frame using 100k_LR_A.
    actualVal = llTransmitFrame(RF_PROFILE_100K_LR_A, pTestFrame);
    TEST_ASSERT_EQUAL(SUCCESS, actualVal);
  }

  mock_calls_verify();
}
