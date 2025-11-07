// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file ZW_Channels_mock.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include "mock_control.h"
#include <ZW_Channels.h>


#define MOCK_FILE "ZW_Channels_mock.c"

zpal_radio_lr_channel_t ZW_RadioConvertChannelIdToLongRangeChannel(uint8_t channelId)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(ZPAL_RADIO_LR_CHANNEL_A);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(pMock, ZPAL_RADIO_LR_CHANNEL_UNKNOWN);

  MOCK_CALL_ACTUAL(pMock, channelId);
  MOCK_CALL_COMPARE_INPUT_UINT8(pMock, ARG0, channelId);
  MOCK_CALL_RETURN_VALUE(pMock, zpal_radio_lr_channel_t);
}

uint8_t ZW_GetChannelIndexLR(CommunicationProfile_t communicationProfile, zpal_radio_lr_channel_config_t eLrChCfg)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(3);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(pMock, 3);

  MOCK_CALL_ACTUAL(pMock, communicationProfile, eLrChCfg);
  MOCK_CALL_COMPARE_INPUT_UINT32(pMock, ARG0, communicationProfile);
  MOCK_CALL_COMPARE_INPUT_UINT32(pMock, ARG1, eLrChCfg);
  MOCK_CALL_RETURN_VALUE(pMock, uint8_t);
}
