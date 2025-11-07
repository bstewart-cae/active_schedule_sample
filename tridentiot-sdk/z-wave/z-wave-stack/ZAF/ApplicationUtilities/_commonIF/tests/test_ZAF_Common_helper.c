// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file test_ZAF_Common_helper.c
 * @copyright 2022 Silicon Laboratories Inc.
 */

#include <mock_control.h>
#include "ZAF_Common_interface.h"
#include "ZAF_Common_helper.h"
#include "ZW_TransportSecProtocol.h"
#include "ZAF_ApplicationEvents.h"
#include "zaf_config_api_mock.h"
#include <zaf_cc_list_generator_mock.h>
#include "board_indicator.h"
#include "zaf_protocol_config_mock.h"
#include "zaf_transport_tx_mock.h"
#include "ZAF_AppName_mock.h"

SAppNodeInfo_t AppNodeInfo;

const SProtocolConfig_t appProtocolConfig = {
                     .pNodeInfo = &AppNodeInfo
};

/* We need a non-null task handle to pass assert checks */
uint32_t dummyTaskHandleObj;
TaskHandle_t pAppTaskHandle = (TaskHandle_t)&dummyTaskHandleObj;
SApplicationHandles appHandle;

static zaf_cc_list_t empty_list = {
  .list_size = 0,
  .cc_list = NULL
};

static zaf_cc_list_t *p_empty_list = &empty_list;

void setUpSuite(void) {

}

void tearDownSuite(void) {

}

void setUp(void) {

}

void tearDown(void) {

}

void test_ZAF_Init_FlirsDeviceType(void)
{
  mock_t * pMock;
  uint8_t _handle;
  zpal_pm_handle_t handle = (zpal_pm_handle_t)&_handle;

  mock_calls_clear();

  // Set device type to FLiRS
  AppNodeInfo.DeviceOptionsMask = APPLICATION_FREQ_LISTENING_MODE_1000ms;

  zaf_transport_init_Expect();

  mock_call_expect(TO_STR(AppTimerInit), &pMock);
  pMock->expect_arg[0].v = EAPPLICATIONEVENT_TIMER;
  pMock->expect_arg[1].pointer = pAppTaskHandle;

  mock_call_expect(TO_STR(Board_Init), &pMock);

  mock_call_expect(TO_STR(Board_IndicatorInit), &pMock);

  mock_call_expect(TO_STR(ZAF_nvm_app_init), &pMock);

  mock_call_expect(TO_STR(ZAF_nvm_init), &pMock);

  mock_call_expect(TO_STR(zpal_pm_register), &pMock);
  pMock->expect_arg[0].v = ZPAL_PM_TYPE_USE_RADIO;
  pMock->return_code.p = handle;

  mock_call_expect(TO_STR(ZW_TransportEndpoint_Init), &pMock);
  pMock->compare_rule_arg[1] = COMPARE_ANY;

  mock_call_use_as_stub(TO_STR(ZAF_CP_Init));
  mock_call_use_as_stub(TO_STR(ZAF_SetCPHandle));
  mock_call_use_as_stub(TO_STR(ZAF_CP_SubscribeToAll));
  mock_call_use_as_stub(TO_STR(ZW_system_startup_SetCCSet));

  mock_call_expect(TO_STR(ZAF_TSE_Init), &pMock);

  zaf_config_get_role_type_ExpectAndReturn(ZWAVEPLUS_INFO_REPORT_ROLE_TYPE_END_NODE_SLEEPING_LISTENING);

  zafi_cc_list_generator_generate_Expect();
  zafi_cc_list_generator_get_lists_Expect(0, NULL, NULL, NULL);
  zafi_cc_list_generator_get_lists_IgnoreArg_unsecure_included_ccs(); // Used as output;
  zafi_cc_list_generator_get_lists_ReturnMemThruPtr_unsecure_included_ccs(&p_empty_list, sizeof(p_empty_list));
  zafi_cc_list_generator_get_lists_IgnoreArg_secure_included_unsecure_ccs(); // Used as output;
  zafi_cc_list_generator_get_lists_ReturnMemThruPtr_secure_included_unsecure_ccs(&p_empty_list, sizeof(p_empty_list));
  zafi_cc_list_generator_get_lists_IgnoreArg_secure_included_secure_ccs(); // Used as output;
  zafi_cc_list_generator_get_lists_ReturnMemThruPtr_secure_included_secure_ccs(&p_empty_list, sizeof(p_empty_list));

  mock_call_expect(TO_STR(zaf_event_distributor_init), &pMock);

  mock_call_expect(TO_STR(Transport_OnApplicationInitSW), &pMock);

  mock_call_expect(TO_STR(ZW_TransportMulticast_init), &pMock);

  zaf_get_protocol_config_ExpectAndReturn(&appProtocolConfig);

  ZAF_AppName_Write_ExpectAndReturn(true);

  ZAF_Init(pAppTaskHandle, &appHandle);

  TEST_ASSERT_MESSAGE(pAppTaskHandle == ZAF_getAppTaskHandle(), "pAppTaskHandle failed");
  TEST_ASSERT_MESSAGE(&appHandle == ZAF_getAppHandle(), "appHandle failed");
  TEST_ASSERT_MESSAGE(handle == ZAF_getPowerLock(), "m_PowerLock failed");

  mock_calls_verify();
}

void test_ZAF_Init_ListeningDeviceType(void)
{
  mock_t * pMock;
  mock_calls_clear();

  mock_call_use_as_stub(TO_STR(AppTimerInit));
  mock_call_use_as_stub(TO_STR(Board_Init));
  mock_call_use_as_stub(TO_STR(Board_IndicatorInit));
  mock_call_use_as_stub(TO_STR(ZAF_nvm_app_init));
  mock_call_use_as_stub(TO_STR(ZAF_nvm_init));
  mock_call_use_as_stub(TO_STR(AppTimerSetReceiverTask));
  mock_call_use_as_stub(TO_STR(ZW_system_startup_SetCCSet));

  zaf_transport_init_Expect();

  // Set device type to LISTENING
  AppNodeInfo.DeviceOptionsMask = APPLICATION_NODEINFO_LISTENING;

  //ZW_TransportEndpoint_Init(pAppHandle->pZwTxQueue, updateStayAwakePeriodFunc);
  mock_call_expect(TO_STR(ZW_TransportEndpoint_Init), &pMock);
  pMock->compare_rule_arg[0] = COMPARE_ANY;
  pMock->compare_rule_arg[1] = COMPARE_ANY;

  mock_call_expect(TO_STR(ZAF_TSE_Init), &pMock);

  mock_call_use_as_stub(TO_STR(ZAF_CP_Init));
  mock_call_use_as_stub(TO_STR(ZAF_SetCPHandle));
  mock_call_use_as_stub(TO_STR(ZAF_CP_SubscribeToAll));

  zaf_config_get_role_type_ExpectAndReturn(ZWAVEPLUS_INFO_REPORT_ROLE_TYPE_END_NODE_ALWAYS_ON);

  zafi_cc_list_generator_generate_Expect();
  zafi_cc_list_generator_get_lists_Expect(0, NULL, NULL, NULL);
  zafi_cc_list_generator_get_lists_IgnoreArg_unsecure_included_ccs(); // Used as output;
  zafi_cc_list_generator_get_lists_ReturnMemThruPtr_unsecure_included_ccs(&p_empty_list, sizeof(p_empty_list));
  zafi_cc_list_generator_get_lists_IgnoreArg_secure_included_unsecure_ccs(); // Used as output;
  zafi_cc_list_generator_get_lists_ReturnMemThruPtr_secure_included_unsecure_ccs(&p_empty_list, sizeof(p_empty_list));
  zafi_cc_list_generator_get_lists_IgnoreArg_secure_included_secure_ccs(); // Used as output;
  zafi_cc_list_generator_get_lists_ReturnMemThruPtr_secure_included_secure_ccs(&p_empty_list, sizeof(p_empty_list));

  mock_call_expect(TO_STR(zaf_event_distributor_init), &pMock);

  mock_call_expect(TO_STR(Transport_OnApplicationInitSW), &pMock);

  mock_call_expect(TO_STR(ZW_TransportMulticast_init), &pMock);

  zaf_get_protocol_config_ExpectAndReturn(&appProtocolConfig);

  ZAF_AppName_Write_ExpectAndReturn(true);

  ZAF_Init(pAppTaskHandle, &appHandle);

  TEST_ASSERT_MESSAGE(pAppTaskHandle == ZAF_getAppTaskHandle(), "pAppTaskHandle failed");
  TEST_ASSERT_MESSAGE(&appHandle == ZAF_getAppHandle(), "appHandle failed");

  mock_calls_verify();
}


void test_ZAF_Init_NotListeningDeviceType(void)
{
  mock_t * pMock;
  mock_calls_clear();

  mock_call_use_as_stub(TO_STR(AppTimerInit));
  mock_call_use_as_stub(TO_STR(Board_Init));
  mock_call_use_as_stub(TO_STR(Board_IndicatorInit));
  mock_call_use_as_stub(TO_STR(ZAF_nvm_app_init));
  mock_call_use_as_stub(TO_STR(ZAF_nvm_init));
  mock_call_use_as_stub(TO_STR(AppTimerSetReceiverTask));
  mock_call_use_as_stub(TO_STR(ZW_system_startup_SetCCSet));

  zaf_transport_init_Expect();

  // Set device type to NOT LISTENING
  AppNodeInfo.DeviceOptionsMask = APPLICATION_NODEINFO_NOT_LISTENING;

  //ZW_TransportEndpoint_Init(pAppHandle->pZwTxQueue, updateStayAwakePeriodFunc);
  mock_call_expect(TO_STR(ZW_TransportEndpoint_Init), &pMock);
  pMock->compare_rule_arg[0] = COMPARE_ANY;
  pMock->compare_rule_arg[1] = COMPARE_ANY;

  mock_call_expect(TO_STR(ZAF_TSE_Init), &pMock);

  mock_call_use_as_stub(TO_STR(ZAF_CP_Init));
  mock_call_use_as_stub(TO_STR(ZAF_SetCPHandle));
  mock_call_use_as_stub(TO_STR(ZAF_CP_SubscribeToAll));

  zaf_config_get_role_type_ExpectAndReturn(ZWAVEPLUS_INFO_REPORT_ROLE_TYPE_END_NODE_SLEEPING_REPORTING);

  zafi_cc_list_generator_generate_Expect();
  zafi_cc_list_generator_get_lists_Expect(0, NULL, NULL, NULL);
  zafi_cc_list_generator_get_lists_IgnoreArg_unsecure_included_ccs(); // Used as output;
  zafi_cc_list_generator_get_lists_ReturnMemThruPtr_unsecure_included_ccs(&p_empty_list, sizeof(p_empty_list));
  zafi_cc_list_generator_get_lists_IgnoreArg_secure_included_unsecure_ccs(); // Used as output;
  zafi_cc_list_generator_get_lists_ReturnMemThruPtr_secure_included_unsecure_ccs(&p_empty_list, sizeof(p_empty_list));
  zafi_cc_list_generator_get_lists_IgnoreArg_secure_included_secure_ccs(); // Used as output;
  zafi_cc_list_generator_get_lists_ReturnMemThruPtr_secure_included_secure_ccs(&p_empty_list, sizeof(p_empty_list));

  mock_call_expect(TO_STR(zaf_event_distributor_init), &pMock);

  mock_call_expect(TO_STR(Transport_OnApplicationInitSW), &pMock);

  mock_call_expect(TO_STR(ZW_TransportMulticast_init), &pMock);

  zaf_get_protocol_config_ExpectAndReturn(&appProtocolConfig);

  ZAF_AppName_Write_ExpectAndReturn(true);

  ZAF_Init(pAppTaskHandle, &appHandle);

  TEST_ASSERT_MESSAGE(pAppTaskHandle == ZAF_getAppTaskHandle(), "pAppTaskHandle failed");
  TEST_ASSERT_MESSAGE(&appHandle == ZAF_getAppHandle(), "appHandle failed");

  mock_calls_verify();
}

void DummyCallback (void)
{
}
void test_ZAF_Init_CmdPublisher(void)
{
  mock_t * pMock;
  mock_calls_clear();

  mock_call_use_as_stub(TO_STR(ZW_TransportEndpoint_Init));
  mock_call_use_as_stub(TO_STR(ZAF_TSE_Init));
  mock_call_use_as_stub(TO_STR(AppTimerInit));
  mock_call_use_as_stub(TO_STR(Board_Init));
  mock_call_use_as_stub(TO_STR(Board_IndicatorInit));
  mock_call_use_as_stub(TO_STR(ZAF_nvm_app_init));
  mock_call_use_as_stub(TO_STR(ZAF_nvm_init));
  mock_call_use_as_stub(TO_STR(AppTimerSetReceiverTask));
  zaf_config_get_role_type_IgnoreAndReturn(0x00);
  mock_call_use_as_stub(TO_STR(ZW_system_startup_SetCCSet));

  zaf_transport_init_Expect();

  CP_Handle_t cp_handle = DummyCallback;
  mock_call_expect(TO_STR(ZAF_CP_Init), &pMock);
  pMock->compare_rule_arg[0] = COMPARE_NOT_NULL;
  pMock->expect_arg[1].v = 3; //CP_MAX_SUBSCRIBERS
  pMock->return_code.p = cp_handle;

  mock_call_expect(TO_STR(ZAF_CP_SubscribeToAll), &pMock);
  pMock->expect_arg[0].p = cp_handle;
  pMock->compare_rule_arg[1] = COMPARE_ANY;
  pMock->expect_arg[2].p = (zaf_cp_subscriberFunction_t)ApplicationCommandHandler;
  pMock->return_code.v = true;

  zafi_cc_list_generator_generate_Expect();
  zafi_cc_list_generator_get_lists_Expect(0, NULL, NULL, NULL);
  zafi_cc_list_generator_get_lists_IgnoreArg_unsecure_included_ccs(); // Used as output;
  zafi_cc_list_generator_get_lists_ReturnMemThruPtr_unsecure_included_ccs(&p_empty_list, sizeof(p_empty_list));
  zafi_cc_list_generator_get_lists_IgnoreArg_secure_included_unsecure_ccs(); // Used as output;
  zafi_cc_list_generator_get_lists_ReturnMemThruPtr_secure_included_unsecure_ccs(&p_empty_list, sizeof(p_empty_list));
  zafi_cc_list_generator_get_lists_IgnoreArg_secure_included_secure_ccs(); // Used as output;
  zafi_cc_list_generator_get_lists_ReturnMemThruPtr_secure_included_secure_ccs(&p_empty_list, sizeof(p_empty_list));

  mock_call_expect(TO_STR(zaf_event_distributor_init), &pMock);

  mock_call_expect(TO_STR(Transport_OnApplicationInitSW), &pMock);

  mock_call_expect(TO_STR(ZW_TransportMulticast_init), &pMock);

  zaf_get_protocol_config_ExpectAndReturn(&appProtocolConfig);

  ZAF_AppName_Write_ExpectAndReturn(true);

  ZAF_Init(pAppTaskHandle, &appHandle);

  mock_calls_verify();
}
