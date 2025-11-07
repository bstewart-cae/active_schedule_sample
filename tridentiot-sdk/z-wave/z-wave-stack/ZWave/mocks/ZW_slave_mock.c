// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file ZW_slave_mock.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include <ZW_transport_api.h>
#include <ZW_application_transport_interface.h>
#include <mock_control.h>
#include <ZW_transport.h>
#include <string.h>

#define MOCK_FILE "ZW_slave_mock.c"

uint8_t
ZW_SendTestFrame(
  uint8_t bNodeID,
  uint8_t powerLevel,
  void (*func)(uint8_t txStatus, TX_STATUS_TYPE *txStatusReport))
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(0x01);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, 0x01);
  MOCK_CALL_ACTUAL(p_mock, bNodeID, powerLevel, func);
  MOCK_CALL_RETURN_IF_ERROR_SET(p_mock, uint8_t);

  MOCK_CALL_COMPARE_INPUT_UINT8(p_mock, ARG0, bNodeID);
  MOCK_CALL_COMPARE_INPUT_UINT8(p_mock, ARG1, powerLevel);
  MOCK_CALL_COMPARE_INPUT_POINTER(p_mock, ARG2, func);

  MOCK_CALL_RETURN_VALUE(p_mock, uint8_t);
}


void ZW_SetLearnMode(
    uint8_t mode,
    void (*learnFunc)(uint8_t bStatus, uint8_t nodeID))
{

}

uint8_t ZW_SendNodeInformation(
  uint8_t destNode,
  uint8_t txOptions,
  void (*completedFunc)(uint8_t, TX_STATUS_TYPE *))
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(1);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, 1);
  MOCK_CALL_ACTUAL(p_mock, destNode, txOptions, completedFunc);

  MOCK_CALL_COMPARE_INPUT_UINT8(p_mock, ARG0, destNode);
  MOCK_CALL_COMPARE_INPUT_UINT8(p_mock, ARG1, txOptions);
  MOCK_CALL_COMPARE_INPUT_POINTER(p_mock, ARG2, completedFunc);

  MOCK_CALL_RETURN_VALUE(p_mock, uint8_t);
}

bool ZW_SendExcludeRequestLR(
  void (*completedFunc)(uint8_t, TX_STATUS_TYPE *))
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(1);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, 1);
  MOCK_CALL_ACTUAL(p_mock, completedFunc);

  MOCK_CALL_COMPARE_INPUT_POINTER(p_mock, ARG0, completedFunc);

  MOCK_CALL_RETURN_VALUE(p_mock, bool);
}

void
InternSetLearnMode(
  uint8_t mode,
  void (*learnFunc)(ELearnStatus bStatus, node_id_t nodeID))
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(p_mock);
  MOCK_CALL_ACTUAL(p_mock, mode, learnFunc);

  MOCK_CALL_COMPARE_INPUT_UINT8(p_mock, ARG0, mode);
  MOCK_CALL_COMPARE_INPUT_POINTER(p_mock, ARG1, learnFunc);
}

uint8_t
GenerateNodeInformation(
  NODEINFO_SLAVE_FRAME *pnodeInfo,
  uint8_t cmdClass)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(10); // Random length
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, 0);
  MOCK_CALL_ACTUAL(p_mock, pnodeInfo);

  MOCK_CALL_COMPARE_INPUT_POINTER(p_mock, ARG0, pnodeInfo);
  MOCK_CALL_COMPARE_INPUT_UINT8(p_mock, ARG1, cmdClass);

  if (NULL != pnodeInfo) {
    memcpy(pnodeInfo, p_mock->output_arg[0].p, sizeof(NODEINFO_SLAVE_FRAME));
  }

  MOCK_CALL_RETURN_VALUE(p_mock, uint8_t);
}

/*
 * Temp definition of NodeSupportsLR()
 */
bool NodeSupportsBeingIncludedAsLR(void)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(false);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, false);

  MOCK_CALL_RETURN_VALUE(p_mock, bool);
}

void UpdateResponseRouteLastReturnRoute(uint8_t bResponseRouteIndex)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(p_mock);
  MOCK_CALL_ACTUAL(p_mock, bResponseRouteIndex);

  MOCK_CALL_COMPARE_INPUT_UINT8(p_mock, ARG0, bResponseRouteIndex);
}

void SlaveSetSucNodeId(uint8_t SucNodeId)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(p_mock);
  MOCK_CALL_ACTUAL(p_mock, SucNodeId);

  MOCK_CALL_COMPARE_INPUT_UINT8(p_mock, ARG0, SucNodeId);
}

zpal_radio_lr_channel_config_t ZW_LrChannelConfigToUse(const zpal_radio_profile_t * pRfProfile)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(1);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, 0);
  MOCK_CALL_ACTUAL(p_mock, pRfProfile);

  MOCK_CALL_COMPARE_INPUT_POINTER(p_mock, ARG0, pRfProfile);

  MOCK_CALL_RETURN_VALUE(p_mock, zpal_radio_lr_channel_config_t);
}

