// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file ZW_controller_mock.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include <stdint.h>
#include <string.h>
#include <ZW_transport.h>
#include <ZW_controller.h>
#include <ZW_basis_api.h>
#include <ZW_transport_api.h>
#include <mock_control.h>

#define MOCK_FILE "ZW_controller_mock.c"

bool                 /*RET true if virtual slave node, false if not */
ZW_IsVirtualNode(
  node_id_t bNodeID)      /* IN Node ID on node to test for if Virtual */
{
  mock_t * pMock;
  MOCK_CALL_RETURN_IF_USED_AS_STUB(false);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(pMock, 0xFF);
  MOCK_CALL_ACTUAL(pMock, bNodeID);
  MOCK_CALL_COMPARE_INPUT_UINT16(pMock, ARG0, bNodeID);
  MOCK_CALL_RETURN_VALUE(pMock, bool);

}

uint8_t              /* RET  Type of node, or 0 if not registred */
ZCB_GetNodeType(
  node_id_t bNodeID)   /* IN   Node id */
{
  mock_t * pMock;
  MOCK_CALL_RETURN_IF_USED_AS_STUB(0x00);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(pMock, 0xFF);
  MOCK_CALL_ACTUAL(pMock, bNodeID);
  MOCK_CALL_COMPARE_INPUT_UINT16(pMock, ARG0, bNodeID);
  MOCK_CALL_RETURN_VALUE(pMock, uint8_t);

}

uint8_t                  /*RET Node capability */
GetNodeCapabilities(
  node_id_t bNodeID)       /* IN Node id */
{
  mock_t * pMock;
  MOCK_CALL_RETURN_IF_USED_AS_STUB(0x00);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(pMock, 0xFF);
  MOCK_CALL_ACTUAL(pMock, bNodeID);
  MOCK_CALL_COMPARE_INPUT_UINT16(pMock, ARG0, bNodeID);
  MOCK_CALL_RETURN_VALUE(pMock, uint8_t);

}

uint8_t
IsNodeSensor(
  node_id_t bSensorNodeID,
  bool bRetSensorMode,
  bool bCheckAssignState)
{
  mock_t * pMock;
  MOCK_CALL_RETURN_IF_USED_AS_STUB(0x00);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(pMock, 0xFF);
  MOCK_CALL_ACTUAL(pMock, bSensorNodeID,bRetSensorMode,bCheckAssignState);
  MOCK_CALL_COMPARE_INPUT_UINT16(pMock, ARG0, bSensorNodeID);
  MOCK_CALL_COMPARE_INPUT_UINT8(pMock, ARG1, bRetSensorMode);
  MOCK_CALL_COMPARE_INPUT_UINT8(pMock, ARG2, bCheckAssignState);
  MOCK_CALL_RETURN_VALUE(pMock, uint8_t);

}

uint8_t
GenerateNodeInformation(
  NODEINFO_FRAME *pnodeInfo,
  uint8_t cmdClass)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(10); // Random length
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, 0);
  MOCK_CALL_ACTUAL(p_mock, pnodeInfo);

  MOCK_CALL_COMPARE_INPUT_POINTER(p_mock, ARG0, pnodeInfo);
  MOCK_CALL_COMPARE_INPUT_UINT8(p_mock, ARG1, cmdClass);

  if (NULL != pnodeInfo) {
    memcpy(pnodeInfo, p_mock->output_arg[0].p, sizeof(NODEINFO_FRAME));
  }

  MOCK_CALL_RETURN_VALUE(p_mock, uint8_t);
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