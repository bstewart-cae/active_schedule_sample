// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file ZW_slave_network_info_storage_mock.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include "mock_control.h"
#include <ZW_slave_network_info_storage.h>
#include <string.h>

#define MOCK_FILE "ZW_slave_network_info_storage_mock.c"

uint8_t
SlaveStorageGetSmartStartState(void)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(0x01);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(pMock, 0);

  MOCK_CALL_RETURN_VALUE(pMock, uint8_t);
}

void
SlaveStorageSetSmartStartState(uint8_t SystemState)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(pMock);
  MOCK_CALL_ACTUAL(pMock, SystemState);

  MOCK_CALL_COMPARE_INPUT_UINT8(pMock, ARG0, SystemState);
}

void SlaveStorageGetNetworkIds(uint8_t *pHomeID, node_id_t *pNodeID)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(pMock);
  MOCK_CALL_ACTUAL(pMock, pHomeID, pNodeID);

  MOCK_CALL_COMPARE_INPUT_POINTER(pMock, ARG0, pHomeID);
  if (NULL != pHomeID) {
    memcpy(pHomeID, pMock->output_arg[0].p, 4);
  }

  MOCK_CALL_COMPARE_INPUT_POINTER(pMock, ARG1, pNodeID);
  if (NULL != pNodeID) {
    *pNodeID = pMock->output_arg[1].v;
  }
}

void SlaveStorageGetReturnRoute(node_id_t destRouteIndex , NVM_RETURN_ROUTE_STRUCT* pReturnRoute)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(pMock);
  MOCK_CALL_ACTUAL(pMock, destRouteIndex, pReturnRoute);

  MOCK_CALL_COMPARE_INPUT_UINT16(pMock, ARG0, destRouteIndex);
  MOCK_CALL_COMPARE_INPUT_POINTER(pMock, ARG1, pReturnRoute);
}

void SlaveStorageGetReturnRouteSpeed(node_id_t destRouteIndex , NVM_RETURN_ROUTE_SPEED* pReturnRouteSpeed)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(pMock);
  MOCK_CALL_ACTUAL(pMock, destRouteIndex, pReturnRouteSpeed);

  MOCK_CALL_COMPARE_INPUT_UINT16(pMock, ARG0, destRouteIndex);
  MOCK_CALL_COMPARE_INPUT_POINTER(pMock, ARG1, pReturnRouteSpeed);
}

void StorageSetPrimaryLongRangeChannelId(zpal_radio_lr_channel_t channelId)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(p_mock);
  MOCK_CALL_ACTUAL(p_mock, channelId);

  MOCK_CALL_COMPARE_INPUT_UINT8(p_mock, ARG0, channelId);
}

zpal_radio_lr_channel_t StorageGetPrimaryLongRangeChannelId(void)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(0x01);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, 0x00);

  MOCK_CALL_RETURN_VALUE(p_mock, zpal_radio_lr_channel_t);
}

bool StorageGetS2MpanTable(void * mpan_table)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(true);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, false);
  MOCK_CALL_ACTUAL(p_mock, mpan_table);

  MOCK_CALL_COMPARE_INPUT_POINTER(p_mock, ARG0, mpan_table);
  MOCK_CALL_RETURN_VALUE(p_mock, bool);
}

bool StorageSetS2MpanTable(void * mpan_table)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(true);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, false);
  MOCK_CALL_ACTUAL(p_mock, mpan_table);

  MOCK_CALL_COMPARE_INPUT_POINTER(p_mock, ARG0, mpan_table);
  MOCK_CALL_RETURN_VALUE(p_mock, bool);
}

bool StorageGetS2SpanTable(void * span_table)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(true);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, false);
  MOCK_CALL_ACTUAL(p_mock, span_table);

  MOCK_CALL_COMPARE_INPUT_POINTER(p_mock, ARG0, span_table);
  MOCK_CALL_RETURN_VALUE(p_mock, bool);
}

bool StorageSetS2SpanTable(void * span_table)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(true);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, false);
  MOCK_CALL_ACTUAL(p_mock, span_table);

  MOCK_CALL_COMPARE_INPUT_POINTER(p_mock, ARG0, span_table);
  MOCK_CALL_RETURN_VALUE(p_mock, bool);
}
