// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file ZAF_Common_interface_mock.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include <ZAF_Common_interface.h>
#include <ZAF_Common_helper.h>
#include "mock_control.h"

#define MOCK_FILE "ZAF_Common_interface_mock.c"


void ZAF_setApplicationData(TaskHandle_t pAppTaskHandle,
                            SApplicationHandles* pAppHandle,
                            const SProtocolConfig_t * pAppProtocolConfig
                            )
{
  mock_t * pMock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(pMock);

  MOCK_CALL_ACTUAL(pMock, pAppTaskHandle, pAppHandle, pAppProtocolConfig);

  MOCK_CALL_COMPARE_INPUT_POINTER(pMock, ARG0, pAppTaskHandle);
  MOCK_CALL_COMPARE_INPUT_POINTER(pMock, ARG1, pAppHandle);
  MOCK_CALL_COMPARE_INPUT_POINTER(pMock, ARG2, pAppProtocolConfig);
}

void ZAF_setAppHandle(SApplicationHandles* pAppHandle)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(pMock);

  MOCK_CALL_ACTUAL(pMock, pAppHandle);

  MOCK_CALL_COMPARE_INPUT_POINTER(pMock, ARG0, pAppHandle);
}

void ZAF_setPowerLock(zpal_pm_handle_t handle)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(pMock);

  MOCK_CALL_ACTUAL(pMock, handle);

  MOCK_CALL_COMPARE_INPUT_POINTER(pMock, ARG0, handle);
}

void ZAF_SetCPHandle(CP_Handle_t handle)
{
  mock_t * pMock;
  MOCK_CALL_RETURN_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(pMock);
  MOCK_CALL_ACTUAL(pMock, handle);

  MOCK_CALL_COMPARE_INPUT_POINTER(pMock, ARG0, handle);
}

SApplicationHandles* ZAF_getAppHandle(void)
{
  mock_t * pMock;

  static SApplicationHandles appHandles;
  static SQueueNotifying ZwTxQueue;
  appHandles.pZwTxQueue = &ZwTxQueue;
  static SQueueNotifying ZwCommandQueue;
  appHandles.pZwCommandQueue = &ZwCommandQueue;
  static zpal_radio_network_stats_t NetworkStatistics;
  appHandles.pNetworkStatistics = &NetworkStatistics;
  static SProtocolInfo ProtocolInfo;
  appHandles.pProtocolInfo = &ProtocolInfo;
  static SNetworkInfo NetworkInfo;
  appHandles.pNetworkInfo = &NetworkInfo;
  static SRadioStatus RadioStatus;
  appHandles.pRadioStatus = &RadioStatus;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(&appHandles);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(pMock, NULL);

  MOCK_CALL_RETURN_POINTER(pMock, SApplicationHandles*);

}

SQueueNotifying* ZAF_getZwTxQueue(void)
{
  mock_t * pMock;

  MOCK_CALL_FIND_RETURN_ON_FAILURE(pMock, NULL);

  MOCK_CALL_RETURN_POINTER(pMock, SQueueNotifying*);
}

SQueueNotifying* ZAF_getZwCommandQueue(void)
{
  mock_t * pMock;

  MOCK_CALL_FIND_RETURN_ON_FAILURE(pMock, NULL);

  MOCK_CALL_RETURN_POINTER(pMock, SQueueNotifying*);
}

zpal_pm_handle_t ZAF_getPowerLock(void)
{
  mock_t * pMock;

  MOCK_CALL_FIND_RETURN_ON_FAILURE(pMock, NULL);

  MOCK_CALL_RETURN_POINTER(pMock, zpal_pm_handle_t);
}

CP_Handle_t ZAF_getCPHandle(void)
{
  mock_t * pMock;

  MOCK_CALL_FIND_RETURN_ON_FAILURE(pMock, NULL);
  MOCK_CALL_RETURN_POINTER(pMock, CP_Handle_t);
}

bool isFLiRS(const SAppNodeInfo_t * pAppNodeInfo)
{
  mock_t * pMock;
  MOCK_CALL_RETURN_IF_USED_AS_STUB(true);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(pMock, false);

  MOCK_CALL_ACTUAL(pMock, pAppNodeInfo);
  MOCK_CALL_RETURN_IF_ERROR_SET(pMock, bool);

  MOCK_CALL_COMPARE_INPUT_POINTER(pMock, ARG0, pAppNodeInfo);

  MOCK_CALL_RETURN_VALUE(pMock, bool);
}

uint8_t ZAF_GetSecurityKeys(void)
{
  mock_t * pMock;
  MOCK_CALL_RETURN_IF_USED_AS_STUB(0);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(pMock, 0xFF);
  MOCK_CALL_RETURN_IF_ERROR_SET(pMock, uint8_t);

  MOCK_CALL_RETURN_VALUE(pMock, uint8_t);
}

EInclusionState_t ZAF_GetInclusionState(void)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(EINCLUSIONSTATE_SECURE_INCLUDED);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, EINCLUSIONSTATE_EXCLUDED);

  MOCK_CALL_RETURN_VALUE(p_mock, EInclusionState_t);
}

node_id_t ZAF_GetNodeID(void)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(0x0001); // Valid node ID
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, 0x0000); // Invalid node ID

  MOCK_CALL_RETURN_VALUE(p_mock, node_id_t);
}

EInclusionMode_t ZAF_GetInclusionMode(void)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(EINCLUSIONMODE_ZWAVE_CLS); // Valid node ID
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, EINCLUSIONMODE_NOT_SET); // Invalid node ID

  MOCK_CALL_RETURN_VALUE(p_mock, EInclusionMode_t);
}

bool isRfRegionValid(zpal_radio_region_t region)
{
  mock_t * pMock;
  MOCK_CALL_RETURN_IF_USED_AS_STUB(true);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(pMock, false);

  MOCK_CALL_ACTUAL(pMock, region);
  MOCK_CALL_RETURN_IF_ERROR_SET(pMock, bool);

  MOCK_CALL_COMPARE_INPUT_UINT8(pMock, ARG0, region);

  MOCK_CALL_RETURN_VALUE(pMock, bool);
}

void zaf_set_stay_awake_callback(zaf_wake_up_callback_t callback) {
  mock_t * pMock;
  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(pMock);

  MOCK_CALL_ACTUAL(pMock, callback);

  MOCK_CALL_COMPARE_INPUT_POINTER(pMock, ARG0, callback);
}

zaf_wake_up_callback_t zaf_get_stay_awake_callback(void) {
  mock_t * pMock;
  MOCK_CALL_RETURN_IF_USED_AS_STUB(NULL);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(pMock, NULL);

  MOCK_CALL_RETURN_POINTER(pMock, zaf_wake_up_callback_t);
}
