// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file ZW_main_region_mock.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include <ZW_typedefs.h>
#include "mock_control.h"
#include "unity.h"
#include <ZW_main_region.h>

#define MOCK_FILE "ZW_main_region_mock.c"

bool ProtocolChangeLrChannelConfig(zpal_radio_lr_channel_config_t eLrChCfg)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(true);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, false);
  MOCK_CALL_ACTUAL(p_mock, eLrChCfg);
  MOCK_CALL_RETURN_IF_ERROR_SET(p_mock, bool);
  MOCK_CALL_COMPARE_INPUT_UINT8(p_mock, ARG0, eLrChCfg);
  MOCK_CALL_RETURN_VALUE(p_mock, bool);
}

bool
ProtocolZWaveLongRangeChannelSet(uint8_t channelId)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(true);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, false);
  MOCK_CALL_ACTUAL(p_mock, channelId);
  MOCK_CALL_RETURN_IF_ERROR_SET(p_mock, bool);
  MOCK_CALL_COMPARE_INPUT_UINT8(p_mock, ARG0, channelId);
  MOCK_CALL_RETURN_VALUE(p_mock, bool);
}

bool
ProtocolLongRangeChannelSet(zpal_radio_lr_channel_t channelId)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(true);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, false);
  MOCK_CALL_ACTUAL(p_mock, channelId);
  MOCK_CALL_RETURN_IF_ERROR_SET(p_mock, bool);
  MOCK_CALL_COMPARE_INPUT_UINT8(p_mock, ARG0, channelId);
  MOCK_CALL_RETURN_VALUE(p_mock, bool);
}

uint8_t
ProtocolLongRangeChannelGet(void)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(0x01);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(pMock, 0x00);
  MOCK_CALL_RETURN_VALUE(pMock, uint8_t);
}

#ifdef ZW_CONTROLLER
void
ProtocolLongRangeChannelModeSet(zpal_radio_lr_channel_mode_t channelmode)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(pMock);
  MOCK_CALL_ACTUAL(p_mock, channelmode);
  MOCK_CALL_COMPARE_INPUT_UINT8(p_mock, ARG0, channelmode);
}

uint8_t
ProtocolLongRangeChannelModeGet(void)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(0x01);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(pMock, 0x00);
  MOCK_CALL_RETURN_VALUE(pMock, uint8_t);
}

#endif