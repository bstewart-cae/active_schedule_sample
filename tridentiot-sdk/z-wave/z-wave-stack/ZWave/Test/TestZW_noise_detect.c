// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file TestZW_noise_detect.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include "mock_control.h"
#include "ZW_noise_detect.h"
#include "ZW_basis.h"
#include "ZW_basis_api.h"
#include <zpal_radio.h>
#include <zpal_radio_utils.h>

void setUpSuite(void) {

}

void tearDownSuite(void) {

}

void setUp(void) {

}

void tearDown(void) {

}

#define NOISE_LEVEL_SAMPLES   8

node_id_t g_nodeID;

typedef struct
{
  zpal_radio_region_t region;
  zpal_radio_lr_channel_config_t chCfg;
  uint8_t             numChannels;
  uint16_t            nodeId;
}
test_vector_t;
#define US_LR_END_DEVICE_CH_NO  2
#define US_LR_CH_NO   4
void test_noise_detect(void)
{
  test_vector_t test_vectors[] = {
                                 {REGION_US_LR, ZPAL_RADIO_LR_CH_CFG3, US_LR_END_DEVICE_CH_NO, 256}, //US_LR node included as US_LR_END_DEVICE
                                 {REGION_US_LR, ZPAL_RADIO_LR_CH_CFG1, US_LR_CH_NO,            0}  //US_LR node not included yet
  };

  for (uint32_t k=0; k<2; k++)
  {
    g_nodeID = test_vectors[k].nodeId;
    mock_calls_clear();
    mock_t * pMock;

    /*****************************************************************************************/
    /**Test that calling NoiseDetectInit() will set all noise levels to RAIL_RSSI_INVALID_DBM*/
    /*****************************************************************************************/
    NoiseDetectInit();

    mock_call_expect(TO_STR(zpal_radio_get_lr_channel_config), &pMock);
    pMock->return_code.value = test_vectors[k].chCfg; //node is LR
    if ( ZPAL_RADIO_LR_CH_CFG3 != test_vectors[k].chCfg )
    {
      mock_call_expect(TO_STR(zpal_radio_get_protocol_mode), &pMock);
      pMock->return_code.value = zpal_radio_region_get_protocol_mode(REGION_US_LR, ZPAL_RADIO_LR_CH_CFG1);
    }
    RSSI_LEVELS noise_levels;
    ZW_GetBackgroundRSSI(&noise_levels);

    //Verify that noise levels for all channels are set to RAIL_RSSI_INVALID_DBM
    //after calling NoiseDetectInit()
    for (uint32_t i = 0; i < test_vectors[k].numChannels; i++)
    {
      TEST_ASSERT_EQUAL(ZPAL_RADIO_INVALID_RSSI_DBM, noise_levels.rssi_dBm[i]);
    }
    /*******************************************************************/
    /**Test calling SampleNoiseLevel() once after NoiseDetectInit(). ***/
    /**ZW_GetBackgroundRSSI() must return the values sampled.        ***/
    /*******************************************************************/
    mock_call_expect(TO_STR(zpal_radio_get_lr_channel_config), &pMock);
    pMock->return_code.value = test_vectors[k].chCfg; //node is LR
    if ( ZPAL_RADIO_LR_CH_CFG3 != test_vectors[k].chCfg )
    {
      mock_call_expect(TO_STR(zpal_radio_get_protocol_mode), &pMock);
      pMock->return_code.value = zpal_radio_region_get_protocol_mode(REGION_US_LR, ZPAL_RADIO_LR_CH_CFG1);
    }

    int8_t rssi[32];

    for (uint32_t i = 0; i < test_vectors[k].numChannels; i++)
    {
      rssi[i] = -100 + i; //Return different values on different channels
      mock_call_expect(TO_STR(zpal_radio_get_background_rssi), &pMock);
      pMock->expect_arg[0].v = i;
      pMock->output_arg[1].p = &rssi[i];
      pMock->compare_rule_arg[1] = COMPARE_NOT_NULL;
      pMock->return_code.value = ZPAL_STATUS_OK;
    }

    SampleNoiseLevel();

    mock_call_expect(TO_STR(zpal_radio_get_lr_channel_config), &pMock);
    pMock->return_code.value = test_vectors[k].chCfg; //node is LR
    if ( ZPAL_RADIO_LR_CH_CFG3 != test_vectors[k].chCfg )
    {
      mock_call_expect(TO_STR(zpal_radio_get_protocol_mode), &pMock);
      pMock->return_code.value = zpal_radio_region_get_protocol_mode(REGION_US_LR, ZPAL_RADIO_LR_CH_CFG1);
    }
    ZW_GetBackgroundRSSI(&noise_levels);

    //Verify that noise levels set in SampleNoiseLevel() are read in ZW_GetBackgroundRSSI()
    for (uint32_t i = 0; i < test_vectors[k].numChannels; i++)
    {
      TEST_ASSERT_EQUAL(-100 + i, noise_levels.rssi_dBm[i]);
    }

    /*******************************************************************************************/
    /**Test calling SampleNoiseLevel() 8 times. It will overwrite internal array of samples. ***/
    /**ZW_GetBackgroundRSSI() must return an average of the values sampled.                  ***/
    /*******************************************************************************************/
    for(uint32_t j = 0; j < NOISE_LEVEL_SAMPLES; j++)
    {
      mock_call_expect(TO_STR(zpal_radio_get_lr_channel_config), &pMock);
      pMock->return_code.value = test_vectors[k].chCfg; //node is LR
      if ( ZPAL_RADIO_LR_CH_CFG3 != test_vectors[k].chCfg )
      {
        mock_call_expect(TO_STR(zpal_radio_get_protocol_mode), &pMock);
        pMock->return_code.value = zpal_radio_region_get_protocol_mode(REGION_US_LR, ZPAL_RADIO_LR_CH_CFG1);
      }

      int8_t rssi[32];
      for (uint32_t i = 0; i < test_vectors[k].numChannels; i++)
      {
        rssi[i] = - 50 + i + j; //Return different values on different channels
        mock_call_expect(TO_STR(zpal_radio_get_background_rssi), &pMock);
        pMock->expect_arg[0].v = i;
        pMock->output_arg[1].p = &rssi[i];
        pMock->compare_rule_arg[1] = COMPARE_NOT_NULL;
        pMock->return_code.value = ZPAL_STATUS_OK;
      }

      SampleNoiseLevel();
    }

    int8_t averages[US_LR_CH_NO];

    //Calculate the averages of values set in SampleNoiseLevel()
    for (uint32_t i = 0; i < test_vectors[k].numChannels; i++)
    {
      int16_t sum = 0;
      for(uint32_t j = 0; j < NOISE_LEVEL_SAMPLES; j++)
      {
        sum += -50 + i + j;
      }
      averages[i] = sum/NOISE_LEVEL_SAMPLES;
    }

    mock_call_expect(TO_STR(zpal_radio_get_lr_channel_config), &pMock);
    pMock->return_code.value = test_vectors[k].chCfg; //node is LR
    if ( ZPAL_RADIO_LR_CH_CFG3 != test_vectors[k].chCfg )
    {
      mock_call_expect(TO_STR(zpal_radio_get_protocol_mode), &pMock);
      pMock->return_code.value = zpal_radio_region_get_protocol_mode(REGION_US_LR, ZPAL_RADIO_LR_CH_CFG1);
    }

    ZW_GetBackgroundRSSI(&noise_levels);

    //Verify that the average of noise levels set is read in ZW_GetBackgroundRSSI()
    for (uint32_t i = 0; i < test_vectors[k].numChannels; i++)
    {
      TEST_ASSERT_EQUAL(averages[i], noise_levels.rssi_dBm[i]);
    }

    mock_calls_verify();
  }
}
