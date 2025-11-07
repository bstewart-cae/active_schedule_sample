// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file test_ZAF_Common_interface.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include <mock_control.h>
#include "ZAF_Common_interface.h"
#include "zaf_protocol_config_mock.h"

void setUpSuite(void) {

}

void tearDownSuite(void) {

}

void test_ZAF_setAppHandle(void)
{
  SApplicationHandles appHandles;

  ZAF_setAppHandle(&appHandles);

  TEST_ASSERT_MESSAGE(&appHandles == ZAF_getAppHandle(), "appHandle pointer does not match");
}

void test_ZAF_setPowerLock(void)
{
  zpal_pm_handle_t powerLock = (zpal_pm_handle_t)0x8;
  ZAF_setPowerLock(powerLock);
  TEST_ASSERT_MESSAGE(powerLock == ZAF_getPowerLock(), "powerLock pointer does not match");
}

void test_isFLiRS_true(void)
{
  SAppNodeInfo_t AppNodeInfo = {  .DeviceOptionsMask = APPLICATION_FREQ_LISTENING_MODE_1000ms };

  zaf_get_app_node_info_ExpectAndReturn(&AppNodeInfo);

  TEST_ASSERT_MESSAGE(isFLiRS(&AppNodeInfo), "Test failed: expected isFLiRS() to be true, returned false instead");
}

void test_isFLiRS_false(void)
{
  SAppNodeInfo_t AppNodeInfo = {  .DeviceOptionsMask = APPLICATION_NODEINFO_LISTENING };

  zaf_get_app_node_info_ExpectAndReturn(&AppNodeInfo);

  TEST_ASSERT_MESSAGE(!isFLiRS(&AppNodeInfo), "Test failed: expected isFLiRS() to be false, returned true instead");
}

void test_ZAF_GetInclusionState(void)
{
  SApplicationHandles AppHandle;
  SNetworkInfo NetworkInfo;

  EInclusionState_t inclusion_state = EINCLUSIONSTATE_SECURE_INCLUDED;
  NetworkInfo.eInclusionState = inclusion_state;
  AppHandle.pNetworkInfo = &NetworkInfo;
  ZAF_setAppHandle(&AppHandle);
  TEST_ASSERT_MESSAGE(inclusion_state == ZAF_GetInclusionState(), "Inclusion state does not match :(");
}

void test_ZAF_GetNodeID(void)
{
  SApplicationHandles AppHandle;
  SNetworkInfo NetworkInfo;
  node_id_t node_id = 5; // some dummy value

  NetworkInfo.NodeId = node_id;
  AppHandle.pNetworkInfo = &NetworkInfo;

  ZAF_setAppHandle(&AppHandle);
  TEST_ASSERT_MESSAGE(node_id == ZAF_GetNodeID(), "Node ID does not match :(");
}

void test_ZAF_GetInclusionMode(void)
{
  SApplicationHandles AppHandle;
  SNetworkInfo NetworkInfo;

  NetworkInfo.NodeId = 0; // node not included
  AppHandle.pNetworkInfo = &NetworkInfo;

  ZAF_setAppHandle(&AppHandle);
  TEST_ASSERT_MESSAGE(EINCLUSIONMODE_NOT_SET == ZAF_GetInclusionMode(), "Inclusion mode does not match - expected not included :(");

  NetworkInfo.NodeId = 2; // node included
  TEST_ASSERT_MESSAGE(EINCLUSIONMODE_ZWAVE_CLS == ZAF_GetInclusionMode(), "Inclusion mode does not match - expected included :(");
}

void test_ZAF_GetSecurityKeys(void)
{
  mock_calls_clear();

  SApplicationHandles AppHandle;
  SNetworkInfo NetworkInfo;
  const uint8_t SECURITY_KEYS = 0xAA;
  NetworkInfo.SecurityKeys = SECURITY_KEYS;
  AppHandle.pNetworkInfo = &NetworkInfo;

  mock_call_use_as_stub(TO_STR(ZW_TransportEndpoint_Init));

  ZAF_setAppHandle(&AppHandle);

  TEST_ASSERT_MESSAGE(SECURITY_KEYS == ZAF_GetSecurityKeys(), "Security keys does not match :(");

}

void test_ZAF_InitCmdPublisher(void)
{
  CP_Handle_t handle = "AA";  // whatever, non null

  ZAF_SetCPHandle(handle);

  TEST_ASSERT_MESSAGE(handle == ZAF_getCPHandle(), "CmdPublisher context not properly set :(");
}
