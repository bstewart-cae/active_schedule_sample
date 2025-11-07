// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file TestZW_network_management_SLAVE.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include <stdbool.h>
#include "ZW_application_transport_interface.h"
#include "ZW_network_management.h"
#include "ZW_protocol.h"
#include "ZW.h"
#include "mock_control.h"
#include <ZW_explore.h>
#include <zpal_power_manager.h>
#include <ZW_slave.h>
#include <ZW_basis.h>
#include <SizeOf.h>
#include <ZW_system_startup_api.h>
#include <zpal_entropy.h>
#include <zpal_radio.h>

void setUpSuite(void) {

}

void tearDownSuite(void) {

}

void setUp(void) {

}

void tearDown(void) {

}

typedef void (*LearnStateTimerCallback_t)(SSwTimer* pTimer);

/*
 * Used for saving the callback function passed to ZwTimerRegister(). Make sure at least one
 * test is saving this function pointer.
 */
LearnStateTimerCallback_t LearnStateTimerCallback;

/*
 * Used for saving the callback function passed to InternSetLearnMode(). Make sure at least one
 * test is saving this function pointer.
 */
learn_mode_callback_t LearnModeCallback;

/*
 *
 */
ZW_SendData_Callback_t ExplorerQueueCallback;

uint32_t dummyReg = 0;

// Global variables defined by various Z-Wave modules.
uint16_t g_nodeID = 0;
uint8_t g_Dsk[16];
bool g_learnMode = false;
bool g_learnModeDsk = false;
bool g_findInProgress = false;
uint8_t bNetworkWideInclusion = NETWORK_WIDE_MODE_IDLE;
uint8_t _ss_power_lock, _lr_power_lock;
zpal_pm_handle_t ss_power_lock = (zpal_pm_handle_t)&_ss_power_lock;
zpal_pm_handle_t lr_power_lock = (zpal_pm_handle_t)&_lr_power_lock;

void setup_ZW_NetworkLearnModeStart(E_NETWORK_LEARN_MODE_ACTION bMode)
{
  mock_call_use_as_stub(TO_STR(SlaveStorageSetSmartStartState));
  mock_call_use_as_stub(TO_STR(ZW_mainDeepSleepPowerLockEnable));
  mock_call_use_as_stub(TO_STR(zpal_pm_cancel));
  mock_call_use_as_stub(TO_STR(zpal_pm_register));
  mock_call_use_as_stub(TO_STR(InternSetLearnMode));
  mock_call_use_as_stub(TO_STR(ZW_TransmitCallbackBind));
  mock_call_use_as_stub(TO_STR(TimerStart));
  mock_call_use_as_stub(TO_STR(TimerStop));
  mock_call_use_as_stub(TO_STR(zpal_pm_stay_awake));
  mock_call_use_as_stub(TO_STR(ZW_SendNodeInformation));
  mock_call_use_as_fake(TO_STR(zpal_retention_register_write));
  mock_call_use_as_fake(TO_STR(zpal_retention_register_read));
  mock_call_use_as_stub(TO_STR(zpal_radio_is_long_range_locked));
  mock_call_use_as_stub(TO_STR(zpal_radio_get_lr_channel_config));
  mock_call_use_as_stub(TO_STR(NodeSupportsBeingIncludedAsLR));
  mock_call_use_as_stub(TO_STR(zpal_radio_get_region));
  mock_call_use_as_stub(TO_STR(GetResetReason));

  g_nodeID = 0;
  ZW_NetworkLearnModeStart(bMode);
}

void test_ZW_NetworkManagementSetMaxInclusionRequestIntervals(void)
{
  uint32_t setIntervals;

  setIntervals = ZW_NetworkManagementSetMaxInclusionRequestIntervals(4);
  TEST_ASSERT_EQUAL_UINT32(0, setIntervals);

  setIntervals = ZW_NetworkManagementSetMaxInclusionRequestIntervals(5);
  TEST_ASSERT_EQUAL_UINT32(5, setIntervals);

  setIntervals = ZW_NetworkManagementSetMaxInclusionRequestIntervals(100);
  TEST_ASSERT_EQUAL_UINT32(5, setIntervals);

  setIntervals = ZW_NetworkManagementSetMaxInclusionRequestIntervals(99);
  TEST_ASSERT_EQUAL_UINT32(99, setIntervals);

  setIntervals = ZW_NetworkManagementSetMaxInclusionRequestIntervals(0);
  TEST_ASSERT_EQUAL_UINT32(0, setIntervals);
}

void test_ZW_NetworkLearnModeStart_SMARTSTART(void)
{
  mock_t * pMock;
  mock_calls_clear();

  mock_t * pMockLearnStateTimer;
  mock_t * pMockInternSetLearnMode;

//BEGIN TEST 1
  //test SmartStart ZW_NetworkLearnModeStart() on not included node
  g_nodeID = 0;

  // mocks in ZW_NetworkLearnModeStart()

  // 1. -> NetworkManagementRegisterLearnStateTimer()
  mock_call_expect(TO_STR(ZwTimerRegister), &pMockLearnStateTimer);  //LearnStateTimer,
  pMockLearnStateTimer->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMockLearnStateTimer->expect_arg[ARG1].value = false;
  pMockLearnStateTimer->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;

  // 2. (node not in network)
  mock_call_expect(TO_STR(SlaveStorageSetSmartStartState), &pMock);
  pMock->expect_arg[ARG0].value = NVM_SYSTEM_STATE_IDLE;

  // 3. -> ResetLrChannelConfig()
  mock_call_expect(TO_STR(zpal_radio_get_lr_channel_config), &pMock);
  pMock->return_code.v = ZPAL_RADIO_LR_CH_CFG1;

  // 4. -> NetworkManagementEnterSmartStart()
  mock_call_use_as_stub(TO_STR(GetResetReason));

  // -> NetworkManagementEnterSmartStart()
  mock_call_expect(TO_STR(zpal_pm_register), &pMock);   //smart_start_power_lock
  pMock->expect_arg[ARG0].value = ZPAL_PM_TYPE_USE_RADIO;
  pMock->return_code.p = ss_power_lock;

  // -> NetworkManagementEnterSmartStart()
  mock_call_expect(TO_STR(ZW_mainDeepSleepPowerLockEnable), &pMock);
  pMock->expect_arg[ARG0].value = true;

  // -> NetworkManagementEnterSmartStart() -> NetworkManagementSmartStartIsEnabled()
  mock_call_use_as_fake(TO_STR(zpal_retention_register_read));

  //  -> NetworkManagementEnterSmartStart() -> NetworkManagementSmartStartEnable()
  mock_call_expect(TO_STR(NodeSupportsBeingIncludedAsLR), &pMock);
  pMock->return_code.v = true;
  mock_call_use_as_fake(TO_STR(zpal_retention_register_write));

  // -> NetworkManagementEnterSmartStart()
  mock_call_expect(TO_STR(SlaveStorageSetSmartStartState), &pMock);
  pMock->expect_arg[ARG0].value = NVM_SYSTEM_STATE_SMART_START;

  g_learnMode = true;

  // 5. -> NetworkManagementLearnStateProcess()
  mock_call_expect(TO_STR(InternSetLearnMode), &pMockInternSetLearnMode);
  pMockInternSetLearnMode->expect_arg[ARG0].value = ZW_SET_LEARN_MODE_SMARTSTART;
  pMockInternSetLearnMode->compare_rule_arg[ARG1] = COMPARE_NOT_NULL;

  // -> NetworkManagementLearnStateProcess() -> ZCB_NetworkManagementLearnNodeStateTimeout()
  mock_call_expect(TO_STR(TimerStop), &pMock);  //LearnStateTimer
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;

  // -> NetworkManagementLearnStateProcess() -> ZCB_NetworkManagementLearnNodeStateTimeout() -> NetworkManagementSmartStartIsLR()
  mock_call_expect(TO_STR(NodeSupportsBeingIncludedAsLR), &pMock);
  pMock->return_code.v = true;
  mock_call_use_as_fake(TO_STR(zpal_retention_register_read));

  // -> NetworkManagementLearnStateProcess() -> ZCB_NetworkManagementLearnNodeStateTimeout()
  mock_call_expect(TO_STR(zpal_radio_get_lr_channel_config), &pMock);
  pMock->return_code.v = ZPAL_RADIO_LR_CH_CFG1;

  // -> NetworkManagementLearnStateProcess() -> ZCB_NetworkManagementLearnNodeStateTimeout()
  mock_call_expect(TO_STR(llChangeRfPHYToLrChannelConfig), &pMock);
  pMock->expect_arg[0].v = ZPAL_RADIO_LR_CH_CFG3;
  pMock->return_code.v = true;

  // -> NetworkManagementLearnStateProcess() -> ZCB_NetworkManagementLearnNodeStateTimeout()
  mock_call_expect(TO_STR(ZW_TransmitCallbackBind), &pMock);
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->compare_rule_arg[ARG1] = COMPARE_NOT_NULL;
  pMock->compare_rule_arg[ARG2] = COMPARE_NULL;

  // -> NetworkManagementLearnStateProcess() -> ZCB_NetworkManagementLearnNodeStateTimeout() -> ZW_ExploreRequestInclusion()
  mock_call_expect(TO_STR(SlaveStorageGetNetworkIds), &pMock);
  pMock->compare_rule_arg[0] = COMPARE_NULL;
  pMock->compare_rule_arg[1] = COMPARE_NOT_NULL;
  pMock->output_arg[1].v = g_nodeID;

  // -> NetworkManagementLearnStateProcess() -> ZCB_NetworkManagementLearnNodeStateTimeout() -> zpal_pm_stay_awake
  mock_call_expect(TO_STR(zpal_pm_stay_awake), &pMock);  //smart_start_power_lock
  pMock->expect_arg[ARG0].p = ss_power_lock;
  pMock->expect_arg[ARG1].value = 5000; //LEARN_MODE_SMARTSTART_EXPLORE_TIMEOUT

  // Finally call function
  ZW_NetworkLearnModeStart(E_NETWORK_LEARN_MODE_INCLUSION_SMARTSTART);

  // Save the callback function passed to ZwTimerRegister().
  LearnStateTimerCallback = (LearnStateTimerCallback_t)pMockLearnStateTimer->actual_arg[2].p;
  LearnModeCallback       = (learn_mode_callback_t)pMockInternSetLearnMode->actual_arg[1].p;

  mock_calls_verify();

  //Disable exclusion so it won't interfere with next test
  setup_ZW_NetworkLearnModeStart(E_NETWORK_LEARN_MODE_DISABLE);
  mock_calls_clear();

//BEGIN TEST 2
  //test SmartStart ZW_NetworkLearnModeStart() on node with g_nodeID where SmartStartState is set.
  g_nodeID = 5;

  mock_call_use_as_stub(TO_STR(GetResetReason));

  //Return NVM_SYSTEM_STATE_SMART_START to indicate SmartStartState is set in NVM3
  mock_call_expect(TO_STR(SlaveStorageGetSmartStartState), &pMock);
  pMock->return_code.value = NVM_SYSTEM_STATE_SMART_START;

  //The protocol must pass ELEARNSTATUS_LEARN_MODE_COMPLETED_FAILED to app
  mock_call_expect(TO_STR(ProtocolInterfacePassToAppLearnStatus), &pMock);
  pMock->expect_arg[ARG0].value = ELEARNSTATUS_LEARN_MODE_COMPLETED_FAILED;

  // This test will only test as classic node - mock call always returns false on stub call
  mock_call_use_as_stub(TO_STR(zpal_radio_is_long_range_locked));
  ZW_NetworkLearnModeStart(E_NETWORK_LEARN_MODE_INCLUSION_SMARTSTART);

  mock_calls_verify();

  mock_calls_clear();

//BEGIN TEST 3
  mock_call_use_as_stub(TO_STR(GetResetReason));

  mock_call_use_as_fake(TO_STR(zpal_retention_register_write));
  mock_call_use_as_fake(TO_STR(zpal_retention_register_read));

  //test SmartStart ZW_NetworkLearnModeStart() on node with g_nodeID where SmartStartState is not set.
  g_nodeID = 5;

  //Return NVM_SYSTEM_STATE_IDLE to indicate SmartStartState is not set in NVM3
  mock_call_expect(TO_STR(SlaveStorageGetSmartStartState), &pMock);
  pMock->return_code.value = NVM_SYSTEM_STATE_IDLE;

  mock_call_expect(TO_STR(SlaveStorageSetSmartStartState), &pMock);
  pMock->expect_arg[ARG0].value = NVM_SYSTEM_STATE_IDLE;

  // -> NetworkManagementLearnStateProcess() -> ZCB_NetworkManagementLearnNodeStateTimeout()
  mock_call_expect(TO_STR(ZW_TransmitCallbackBind), &pMock);
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->compare_rule_arg[ARG1] = COMPARE_NULL;
  pMock->compare_rule_arg[ARG2] = COMPARE_NULL;

  // -> NetworkManagementLearnStateProcess() ? -> InternSetLearnMode() -> NetworkManagementGenerateDskpart()
  mock_call_use_as_stub(TO_STR(keystore_public_dsk_read));

  mock_call_expect(TO_STR(ExploreQueueFrame), &pMock);
  pMock->compare_rule_arg[0] = COMPARE_ANY;
  pMock->compare_rule_arg[1] = COMPARE_ANY;
  pMock->compare_rule_arg[2] = COMPARE_ANY;
  pMock->compare_rule_arg[3] = COMPARE_ANY;
  pMock->compare_rule_arg[4] = COMPARE_ANY;
  pMock->compare_rule_arg[5] = COMPARE_ANY;
  pMock->return_code.v = 1; // != 0 means frame was enqueued

  mock_call_expect(TO_STR(ZW_mainDeepSleepPowerLockEnable), &pMock);
  pMock->expect_arg[ARG0].value = false;

  // This test will only test as classic node - mock call always returns false on stub call
  mock_call_use_as_stub(TO_STR(zpal_radio_is_long_range_locked));
  ZW_NetworkLearnModeStart(E_NETWORK_LEARN_MODE_INCLUSION_SMARTSTART);

  mock_calls_verify();
}

const uint32_t ret_reg_smart_start = 0xF | 0x40; //SYSTEM_TYPE_SMART_START_COUNT_MASK | INCLUSION_STATE_RETENTION_SMART_START_ACTIVE

static void expect_NetworkManagementSmartStartClear(mock_t *p_mock)
{
  //From calling: NetworkManagementSmartStartClear();
  mock_call_expect(TO_STR(zpal_retention_register_write), &p_mock);
  p_mock->expect_arg[ARG0].value = 0;  //ZPAL_RETENTION_REGISTER_SMARTSTART
  p_mock->expect_arg[ARG1].value = 0;  //clear retention RAM
}

void test_ZW_NetworkLearnModeStart_CLASSIC(void)
{
  mock_t * pMock;
  mock_calls_clear();

  //test ZW_NetworkLearnModeStart() on not included node
  g_nodeID = 0;

  mock_call_expect(TO_STR(SlaveStorageSetSmartStartState), &pMock);
  pMock->expect_arg[ARG0].value = NVM_SYSTEM_STATE_IDLE;

  // -> ResetLrChannelConfig()
  mock_call_expect(TO_STR(zpal_radio_get_lr_channel_config), &pMock);
  pMock->return_code.v = ZPAL_RADIO_LR_CH_CFG1;

  expect_NetworkManagementSmartStartClear(pMock);

  // -> NetworkManagementSmartStartClear()
  mock_call_expect(TO_STR(ZW_mainDeepSleepPowerLockEnable), &pMock);
  pMock->expect_arg[ARG0].value = false;

  // -> NetworkManagementLearnStateProcess()
  mock_call_expect(TO_STR(InternSetLearnMode), &pMock);
  pMock->expect_arg[ARG0].value = ZW_SET_LEARN_MODE_CLASSIC;
  pMock->compare_rule_arg[ARG1] = COMPARE_NOT_NULL;

  g_learnMode = true;

  // -> NetworkManagementLearnStateProcess()
  mock_call_expect(TO_STR(zpal_radio_is_long_range_locked), &pMock);

  // -> NetworkManagementLearnStateProcess()
  mock_call_expect(TO_STR(ZW_SendNodeInformation), &pMock);
  pMock->expect_arg[ARG0].value = NODE_BROADCAST;
  pMock->expect_arg[ARG1].value = 0;
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;

  // -> NetworkManagementLearnStateProcess()
  mock_call_expect(TO_STR(TimerStart), &pMock);  //LearnStateTimer
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 2000; //LEARN_MODE_CLASSIC_TIMEOUT

  ZW_NetworkLearnModeStart(E_NETWORK_LEARN_MODE_INCLUSION);

  //test callback to LearnStateTimer

  mock_call_expect(TO_STR(TimerStop), &pMock);  //LearnStateTimer
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;

  //Switch to LEARN_MODE_NWI
  mock_call_expect(TO_STR(InternSetLearnMode), &pMock);
  pMock->expect_arg[ARG0].value = ZW_SET_LEARN_MODE_DISABLE;
  pMock->compare_rule_arg[ARG1] = COMPARE_NULL;

  mock_call_expect(TO_STR(InternSetLearnMode), &pMock);
  pMock->expect_arg[ARG0].value = ZW_SET_LEARN_MODE_NWI;
  pMock->compare_rule_arg[ARG1] = COMPARE_NOT_NULL;

  mock_call_expect(TO_STR(TimerStop), &pMock);  //LearnStateTimer
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;

  mock_call_expect(TO_STR(ZW_TransmitCallbackBind), &pMock);
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->compare_rule_arg[ARG1] = COMPARE_NULL;
  pMock->compare_rule_arg[ARG2] = COMPARE_NULL;

  mock_call_expect(TO_STR(SlaveStorageGetNetworkIds), &pMock);
  pMock->compare_rule_arg[0] = COMPARE_NULL;
  pMock->compare_rule_arg[1] = COMPARE_NOT_NULL;
  pMock->output_arg[1].v = g_nodeID;

  mock_call_expect(TO_STR(TimerStart), &pMock);  //LearnStateTimer
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 8800; //From random generator seeded with 0x01

  const uint8_t seed = 0x01;
  mock_call_expect(TO_STR(zpal_get_pseudo_random), &pMock);
  pMock->return_code.v = (seed ^ 0xAA) + 0x11;

  LearnStateTimerCallback(NULL);

  mock_calls_verify();

  //Disable inclusion so it won't interfere with next test
  setup_ZW_NetworkLearnModeStart(E_NETWORK_LEARN_MODE_DISABLE);
}

void test_ZW_NetworkLearnModeStart_EXCLUSION(void)
{
  mock_t * pMock;
  mock_calls_clear();

  //test ZW_NetworkLearnModeStart() on included node
  g_nodeID = 3;

  //Return NVM_SYSTEM_STATE_IDLE to indicate SmartStartState is not set in NVM3
  mock_call_expect(TO_STR(SlaveStorageGetSmartStartState), &pMock);
  pMock->return_code.value = NVM_SYSTEM_STATE_IDLE;

  mock_call_expect(TO_STR(SlaveStorageSetSmartStartState), &pMock);
  pMock->expect_arg[ARG0].value = NVM_SYSTEM_STATE_IDLE;

  expect_NetworkManagementSmartStartClear(pMock);

  mock_call_expect(TO_STR(ZW_mainDeepSleepPowerLockEnable), &pMock);
  pMock->expect_arg[ARG0].value = false;

  mock_call_expect(TO_STR(InternSetLearnMode), &pMock);
  pMock->expect_arg[ARG0].value = ZW_SET_LEARN_MODE_CLASSIC;
  pMock->compare_rule_arg[ARG1] = COMPARE_NOT_NULL;

  mock_call_expect(TO_STR(ZW_SendNodeInformation), &pMock);
  pMock->expect_arg[ARG0].value = NODE_BROADCAST;
  pMock->expect_arg[ARG1].value = 0;
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;

  mock_call_expect(TO_STR(zpal_radio_is_long_range_locked), &pMock);

  mock_call_expect(TO_STR(TimerStart), &pMock);
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 2000; //LEARN_MODE_CLASSIC_TIMEOUT

  ZW_NetworkLearnModeStart(E_NETWORK_LEARN_MODE_EXCLUSION);

  //test callback to LearnStateTimer

  mock_call_expect(TO_STR(TimerStop), &pMock);  //LearnStateTimer
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;

  mock_call_expect(TO_STR(InternSetLearnMode), &pMock);
  pMock->expect_arg[ARG0].value = ZW_SET_LEARN_MODE_DISABLE;
  pMock->compare_rule_arg[ARG1] = COMPARE_NULL;

  //Please note that ZW_SET_LEARN_MODE_NWI is set and not ZW_SET_LEARN_MODE_NWE
  //Not sure if this is intended or if it matters.
  mock_call_expect(TO_STR(InternSetLearnMode), &pMock);
  pMock->expect_arg[ARG0].value = ZW_SET_LEARN_MODE_NWI;
  pMock->compare_rule_arg[ARG1] = COMPARE_NOT_NULL;

  mock_call_expect(TO_STR(TimerStop), &pMock);  //LearnStateTimer
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;

  mock_call_expect(TO_STR(InternSetLearnMode), &pMock);
  pMock->expect_arg[ARG0].value = ZW_SET_LEARN_MODE_DISABLE;
  pMock->compare_rule_arg[ARG1] = COMPARE_NULL;

  mock_call_expect(TO_STR(TimerStop), &pMock);  //LearnStateTimer
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;

  expect_NetworkManagementSmartStartClear(pMock);

  mock_call_expect(TO_STR(ZW_mainDeepSleepPowerLockEnable), &pMock);
  pMock->expect_arg[ARG0].value = false;

  mock_call_expect(TO_STR(ProtocolInterfacePassToAppLearnStatus), &pMock);
  pMock->expect_arg[ARG0].value = ELEARNSTATUS_LEARN_MODE_COMPLETED_TIMEOUT;

  LearnStateTimerCallback(NULL);

  mock_calls_verify();

  //Disable exclusion so it won't interfere with next test
  setup_ZW_NetworkLearnModeStart(E_NETWORK_LEARN_MODE_DISABLE);
}

void test_ZW_NetworkLearnModeStart_EXCLUSION_NWE(void)
{
  mock_t * pMock;
  mock_calls_clear();

  //test ZW_NetworkLearnModeStart() on included node
  g_nodeID = 3;

  //Return NVM_SYSTEM_STATE_IDLE to indicate SmartStartState is not set in NVM3
  mock_call_expect(TO_STR(SlaveStorageGetSmartStartState), &pMock);
  pMock->return_code.value = NVM_SYSTEM_STATE_IDLE;

  mock_call_expect(TO_STR(SlaveStorageSetSmartStartState), &pMock);
  pMock->expect_arg[ARG0].value = NVM_SYSTEM_STATE_IDLE;

  expect_NetworkManagementSmartStartClear(pMock);

  mock_call_expect(TO_STR(ZW_mainDeepSleepPowerLockEnable), &pMock);
  pMock->expect_arg[ARG0].value = false;

  mock_call_expect(TO_STR(InternSetLearnMode), &pMock);
  pMock->expect_arg[ARG0].value = ZW_SET_LEARN_MODE_CLASSIC;
  pMock->compare_rule_arg[ARG1] = COMPARE_NOT_NULL;

  g_learnMode = true;

  mock_call_expect(TO_STR(zpal_radio_is_long_range_locked), &pMock);

  mock_call_expect(TO_STR(ZW_SendNodeInformation), &pMock);
  pMock->expect_arg[ARG0].value = NODE_BROADCAST;
  pMock->expect_arg[ARG1].value = 0;
  pMock->compare_rule_arg[ARG2] = COMPARE_NOT_NULL;

  //Please note that the timeout value below is not LEARN_MODE_NWI_TIMEOUT
  //Is this correct?
  mock_call_expect(TO_STR(TimerStart), &pMock);
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 2000; //LEARN_MODE_CLASSIC_TIMEOUT

  ZW_NetworkLearnModeStart(E_NETWORK_LEARN_MODE_EXCLUSION_NWE);

  //test callback to LearnStateTimer

  mock_call_expect(TO_STR(TimerStop), &pMock);  //LearnStateTimer
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;

  mock_call_expect(TO_STR(InternSetLearnMode), &pMock);
  pMock->expect_arg[ARG0].value = ZW_SET_LEARN_MODE_DISABLE;
  pMock->compare_rule_arg[ARG1] = COMPARE_NULL;

  mock_call_expect(TO_STR(InternSetLearnMode), &pMock);
  pMock->expect_arg[ARG0].value = ZW_SET_LEARN_MODE_NWE;
  pMock->compare_rule_arg[ARG1] = COMPARE_NOT_NULL;

  mock_call_expect(TO_STR(TimerStop), &pMock);  //LearnStateTimer
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;

  mock_call_expect(TO_STR(ZW_TransmitCallbackBind), &pMock);
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->compare_rule_arg[ARG1] = COMPARE_NULL;
  pMock->compare_rule_arg[ARG2] = COMPARE_NULL;

  mock_call_expect(TO_STR(ZW_ExploreRequestExclusion), &pMock);
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;

  mock_call_expect(TO_STR(TimerStart), &pMock);  //LearnStateTimer
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
  pMock->expect_arg[ARG1].value = 8800; //From random generator seeded with 0x01

  const uint8_t seed = 0x01;
  mock_call_expect(TO_STR(zpal_get_pseudo_random), &pMock);
  pMock->return_code.v = (seed ^ 0xAA) + 0x11;

  LearnStateTimerCallback(NULL);

  mock_calls_verify();

  //Disable exclusion so it won't interfere with next test
  setup_ZW_NetworkLearnModeStart(E_NETWORK_LEARN_MODE_DISABLE);
}

void test_ZW_NetworkLearnModeStart_DISABLE(void)
{
  mock_t * pMock;

  //Test disabling Classic Inclusion

  setup_ZW_NetworkLearnModeStart(E_NETWORK_LEARN_MODE_INCLUSION);
  g_nodeID = 0;

  mock_calls_clear();

  mock_call_use_as_fake(TO_STR(zpal_retention_register_write));
  mock_call_use_as_fake(TO_STR(zpal_retention_register_read));

  mock_call_expect(TO_STR(SlaveStorageSetSmartStartState), &pMock);
  pMock->expect_arg[ARG0].value = NVM_SYSTEM_STATE_IDLE;

  // -> ResetLrChannelConfig()
  mock_call_expect(TO_STR(zpal_radio_get_lr_channel_config), &pMock);
  pMock->return_code.v = ZPAL_RADIO_LR_CH_CFG1;

  // -> NetworkManagementLearnStateProcess()
  mock_call_expect(TO_STR(InternSetLearnMode), &pMock);
  pMock->expect_arg[ARG0].value = ZW_SET_LEARN_MODE_DISABLE;
  pMock->expect_arg[ARG1].value = 0;

  // -> NetworkManagementLearnStateProcess()
  mock_call_expect(TO_STR(TimerStop), &pMock);
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;

  // -> NetworkManagementLearnStateProcess() -> NetworkManagementSmartStartClear()
  mock_call_expect(TO_STR(ZW_mainDeepSleepPowerLockEnable), &pMock);
  pMock->expect_arg[ARG0].value = false;

  ZW_NetworkLearnModeStart(E_NETWORK_LEARN_MODE_DISABLE);

  mock_calls_verify();

  //Test disabling Classic Exclusion
  setup_ZW_NetworkLearnModeStart(E_NETWORK_LEARN_MODE_EXCLUSION);
  g_nodeID = 3;

  mock_calls_clear();

  mock_call_use_as_fake(TO_STR(zpal_retention_register_write));
  mock_call_use_as_fake(TO_STR(zpal_retention_register_read));

  //Return NVM_SYSTEM_STATE_IDLE to indicate SmartStartState is not set in NVM3
  mock_call_expect(TO_STR(SlaveStorageGetSmartStartState), &pMock);
  pMock->return_code.value = NVM_SYSTEM_STATE_IDLE;

  mock_call_expect(TO_STR(SlaveStorageSetSmartStartState), &pMock);
  pMock->expect_arg[ARG0].value = NVM_SYSTEM_STATE_IDLE;

  mock_call_expect(TO_STR(InternSetLearnMode), &pMock);
  pMock->expect_arg[ARG0].value = ZW_SET_LEARN_MODE_DISABLE;
  pMock->expect_arg[ARG1].value = 0;

  mock_call_expect(TO_STR(TimerStop), &pMock);
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;

  mock_call_expect(TO_STR(ZW_mainDeepSleepPowerLockEnable), &pMock);
  pMock->expect_arg[ARG0].value = false;

  ZW_NetworkLearnModeStart(E_NETWORK_LEARN_MODE_DISABLE);

  mock_calls_verify();

  //Test disabling NWE Exclusion
  setup_ZW_NetworkLearnModeStart(E_NETWORK_LEARN_MODE_EXCLUSION_NWE);
  g_nodeID = 3;

  mock_calls_clear();

  mock_call_use_as_fake(TO_STR(zpal_retention_register_write));
  mock_call_use_as_fake(TO_STR(zpal_retention_register_read));

  //Return NVM_SYSTEM_STATE_IDLE to indicate SmartStartState is not set in NVM3
  mock_call_expect(TO_STR(SlaveStorageGetSmartStartState), &pMock);
  pMock->return_code.value = NVM_SYSTEM_STATE_IDLE;

  mock_call_expect(TO_STR(SlaveStorageSetSmartStartState), &pMock);
  pMock->expect_arg[ARG0].value = NVM_SYSTEM_STATE_IDLE;

  mock_call_expect(TO_STR(InternSetLearnMode), &pMock);
  pMock->expect_arg[ARG0].value = ZW_SET_LEARN_MODE_DISABLE;
  pMock->expect_arg[ARG1].value = 0;

  mock_call_expect(TO_STR(TimerStop), &pMock);
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;

  mock_call_expect(TO_STR(ZW_mainDeepSleepPowerLockEnable), &pMock);
  pMock->expect_arg[ARG0].value = false;

  ZW_NetworkLearnModeStart(E_NETWORK_LEARN_MODE_DISABLE);

  mock_calls_verify();
}

/*
 * Verifies that starting Smart Start and then disabling learn mode while being in Smart Start
 * works correctly.
 */
void test_smartstart_enable_disable(void)
{
  mock_t * pMock;
  mock_calls_clear();

  // Node is not included.
  g_nodeID = 0;

  // Setup mocks for starting learn mode using Smart Start.
  // Because we did that in an earlier test, we will use most of the functions as stubs.

  mock_call_use_as_stub(TO_STR(SlaveStorageSetSmartStartState));

  // -> ResetLrChannelConfig()
  mock_call_expect(TO_STR(zpal_radio_get_lr_channel_config), &pMock);
  pMock->return_code.v = ZPAL_RADIO_LR_CH_CFG1;

  mock_call_use_as_stub(TO_STR(GetResetReason));

  mock_call_use_as_fake(TO_STR(zpal_retention_register_write));
  mock_call_use_as_fake(TO_STR(zpal_retention_register_read));

  mock_call_expect(TO_STR(ZW_mainDeepSleepPowerLockEnable), &pMock);
  pMock->expect_arg[ARG0].value = true;

  mock_call_use_as_stub(TO_STR(zpal_pm_cancel));

  mock_call_use_as_stub(TO_STR(InternSetLearnMode));

  mock_call_use_as_stub(TO_STR(TimerStop));

  mock_call_expect(TO_STR(NodeSupportsBeingIncludedAsLR), &pMock);
  pMock->return_code.v = true;

  mock_call_use_as_stub(TO_STR(ZW_TransmitCallbackBind));

  mock_call_expect(TO_STR(SlaveStorageGetNetworkIds), &pMock);
  pMock->compare_rule_arg[0] = COMPARE_NULL;
  pMock->compare_rule_arg[1] = COMPARE_NOT_NULL;
  pMock->output_arg[1].v = g_nodeID;

  mock_call_expect(TO_STR(NodeSupportsBeingIncludedAsLR), &pMock);
  pMock->return_code.v = true;

  mock_call_use_as_stub(TO_STR(zpal_pm_stay_awake));

  ZW_NetworkLearnModeStart(E_NETWORK_LEARN_MODE_INCLUSION_SMARTSTART);

  // Setup mocks for disabling learn mode.
  // Be aware that some functions are already setup above to be used as stubs.

  mock_call_expect(TO_STR(ZW_mainDeepSleepPowerLockEnable), &pMock);
  pMock->expect_arg[ARG0].value = false;
  // -> ResetLrChannelConfig()
  mock_call_expect(TO_STR(zpal_radio_get_lr_channel_config), &pMock);
  pMock->return_code.v = ZPAL_RADIO_LR_CH_CFG1;


  ZW_NetworkLearnModeStart(E_NETWORK_LEARN_MODE_DISABLE);

  mock_calls_verify();
}

void test_ZCB_NetworkManagementLearnModeCompleted_SMARTSTART(void)
{
  mock_t * pMock;

  mock_calls_clear();

  mock_call_expect(TO_STR(SlaveStorageGetNetworkIds), &pMock);
  pMock->compare_rule_arg[0] = COMPARE_NULL;
  pMock->compare_rule_arg[1] = COMPARE_NOT_NULL;
  pMock->output_arg[1].v = g_nodeID;

  mock_call_use_as_stub(TO_STR(llChangeRfPHYToLrChannelConfig));

  setup_ZW_NetworkLearnModeStart(E_NETWORK_LEARN_MODE_INCLUSION_SMARTSTART);

  mock_calls_clear();

  //Test ZCB_NetworkManagementLearnModeCompleted(ELEARNSTATUS_ASSIGN_NODEID_DONE, g_nodeID)

  mock_call_expect(TO_STR(TimerStop), &pMock);  //pLearnStateTimer
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;

  expect_NetworkManagementSmartStartClear(pMock);

  mock_call_expect(TO_STR(ZW_mainDeepSleepPowerLockEnable), &pMock);
  pMock->expect_arg[ARG0].value = false;

  mock_call_expect(TO_STR(ProtocolInterfacePassToAppLearnStatus), &pMock);
  pMock->expect_arg[ARG0].value = ELEARNSTATUS_SMART_START_IN_PROGRESS;

  mock_call_expect(TO_STR(zpal_pm_stay_awake), &pMock);  //smart_start_power_lock
  pMock->expect_arg[ARG0].p = ss_power_lock;
  pMock->expect_arg[ARG1].value = 250000; //LEARN_MODE_SMARTSTART_LEARN_TIMEOUT

  g_nodeID = 5;
  LearnModeCallback(ELEARNSTATUS_ASSIGN_NODEID_DONE, g_nodeID);

  //Test ZCB_NetworkManagementLearnModeCompleted(ELEARNSTATUS_ASSIGN_COMPLETE, g_nodeID)

  mock_call_expect(TO_STR(TimerStop), &pMock);  //pLearnStateTimer
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;

  mock_call_expect(TO_STR(secure_learn_get_errno), &pMock);
  pMock->return_code.value = 0; //E_SECURE_LEARN_ERRNO_COMPLETE

  mock_call_expect(TO_STR(getSecureKeysRequested), &pMock);
  pMock->return_code.value = SECURITY_KEY_S2_UNAUTHENTICATED_BIT;

  mock_call_expect(TO_STR(secure_learn_get_errno), &pMock);
  pMock->return_code.value = 0; //E_SECURE_LEARN_ERRNO_COMPLETE

  mock_call_expect(TO_STR(SlaveStorageSetSmartStartState), &pMock);
  pMock->expect_arg[ARG0].value = NVM_SYSTEM_STATE_IDLE;

  mock_call_expect(TO_STR(InternSetLearnMode), &pMock);
  pMock->expect_arg[ARG0].value = ZW_SET_LEARN_MODE_DISABLE;
  pMock->compare_rule_arg[ARG1] = COMPARE_NULL;

  mock_call_expect(TO_STR(TimerStop), &pMock);
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL; //LearnStateTimer

  expect_NetworkManagementSmartStartClear(pMock);

  mock_call_expect(TO_STR(ZW_mainDeepSleepPowerLockEnable), &pMock);
  pMock->expect_arg[ARG0].value = false;

  mock_call_expect(TO_STR(ProtocolInterfacePassToAppLearnStatus), &pMock);
  pMock->expect_arg[ARG0].value = ELEARNSTATUS_ASSIGN_COMPLETE;

  mock_call_expect(TO_STR(zpal_pm_cancel), &pMock); //smart_start_power_lock
  pMock->expect_arg[ARG0].p = ss_power_lock;

  LearnModeCallback(ELEARNSTATUS_ASSIGN_COMPLETE, g_nodeID);

  mock_calls_verify();
}

void test_ZCB_NetworkManagementLearnModeCompleted_CLASSIC(void)
{
  setup_ZW_NetworkLearnModeStart(E_NETWORK_LEARN_MODE_INCLUSION);

  mock_t * pMock;
  mock_calls_clear();

  //Test ZCB_NetworkManagementLearnModeCompleted(ELEARNSTATUS_ASSIGN_NODEID_DONE, g_nodeID)

  mock_call_expect(TO_STR(TimerStop), &pMock);  //pLearnStateTimer
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;

  expect_NetworkManagementSmartStartClear(pMock);

  mock_call_expect(TO_STR(ZW_mainDeepSleepPowerLockEnable), &pMock);
  pMock->expect_arg[ARG0].value = false;

  mock_call_expect(TO_STR(ProtocolInterfacePassToAppLearnStatus), &pMock);
  pMock->expect_arg[ARG0].value = ELEARNSTATUS_LEARN_IN_PROGRESS;

  g_nodeID = 5;
  LearnModeCallback(ELEARNSTATUS_ASSIGN_NODEID_DONE, g_nodeID);

  //Test ZCB_NetworkManagementLearnModeCompleted(ELEARNSTATUS_ASSIGN_COMPLETE, g_nodeID)

  mock_call_expect(TO_STR(TimerStop), &pMock);  //pLearnStateTimer
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;

  mock_call_expect(TO_STR(secure_learn_get_errno), &pMock);
  pMock->return_code.value = 0; //E_SECURE_LEARN_ERRNO_COMPLETE

  mock_call_expect(TO_STR(SlaveStorageSetSmartStartState), &pMock);
  pMock->expect_arg[ARG0].value = NVM_SYSTEM_STATE_IDLE;

  mock_call_expect(TO_STR(InternSetLearnMode), &pMock);
  pMock->expect_arg[ARG0].value = ZW_SET_LEARN_MODE_DISABLE;
  pMock->compare_rule_arg[ARG1] = COMPARE_NULL;

  mock_call_expect(TO_STR(TimerStop), &pMock);
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL; //LearnStateTimer

  expect_NetworkManagementSmartStartClear(pMock);

  mock_call_expect(TO_STR(ZW_mainDeepSleepPowerLockEnable), &pMock);
  pMock->expect_arg[ARG0].value = false;

  mock_call_expect(TO_STR(ProtocolInterfacePassToAppLearnStatus), &pMock);
  pMock->expect_arg[ARG0].value = ELEARNSTATUS_ASSIGN_COMPLETE;

  mock_call_expect(TO_STR(zpal_pm_cancel), &pMock); //smart_start_power_lock
  pMock->expect_arg[ARG0].p = ss_power_lock;

  LearnModeCallback(ELEARNSTATUS_ASSIGN_COMPLETE,  g_nodeID);

  mock_calls_verify();
}

typedef struct
{
  uint8_t command;
  uint8_t transmit_options;
  uint32_t time;
}
test_t;

/*
 * Verifies the flow from starting learn mode until the full time period in between prime/include
 * transmission is reached.
 *
 * Prerequisites:
 * - Always on device
 *
 * Variables for table of combinations:
 * - Time between transmission of prime/include and the next transmission of both
 * - Command (Prime / Include)
 * - Explore transmit options
 */
void test_smartstart_flow(void)
{
  mock_t * pMock;
  mock_t * pMockTransmitCallbackBind;
  mock_calls_clear();

  mock_call_use_as_stub(TO_STR(NodeSupportsBeingIncludedAsLR));
  mock_call_use_as_stub(TO_STR(GetResetReason));

  g_nodeID = 0;

  /*
   * The timing is calculated using GetRandom
   *
   * bRequestNo = 0
   * ---------------
   * 0x01 ^ 0xAA + 0x11 = 0xBC (188)
   * 188 % 120 = 68
   * 68 * 100 = 6800
   *
   * bRequestNo = 1
   * ---------------
   * 0xBC ^ 0xAA + 0x11 = 0x27 (39)
   * 39 % 120 = 39
   * 39 * 100 = 3900
   * 3900 + 16000 = 19900
   *
   * bRequestNo = 2
   * ---------------
   * 0x27 ^ 0xAA + 0x11 = 0x9E (158)
   * 158 % 120 = 38
   * 158 << (2-1) = 76
   * 76 * 100 = 7600
   * 7600 + 32000 = 39600
   *
   * bRequestNo = 3
   * ---------------
   * 0x9E ^ 0xAA + 0x11 = 0x45 (69)
   * 69 % 120 = 69
   * 69 << (3-1) = 276
   * 276 * 100 = 27600
   * 27600 + 64000 = 91600
   *
   * bRequestNo = 4
   * ---------------
   * 0x45 ^ 0xAA + 0x11 = 0
   * 0 % 120 = 0
   * 0 << (4-1) = 0
   * 0 * 100 = 0
   * 0 + 128000 = 128000
   *
   * bRequestNo = 5
   * ---------------
   * 0x00 ^ 0xAA + 0x11 = 0xBB (187)
   * 187 % 120 = 67
   * 67 << (5-1) = 1072
   * 1072 * 100 = 107200
   * 107200 + 256000 = 363200
   */

  const uint8_t random_sequence[] = {0xBC, 0x27, 0x9E, 0x45, 0x00, 0xBB};
  for (size_t i = 0; i < sizeof_array(random_sequence); ++i) {
    mock_call_expect(TO_STR(zpal_get_pseudo_random), &pMock);
    pMock->return_code.v = random_sequence[i];
  }

  const test_t test_combinations[] = {
    // 0
    {ZWAVE_CMD_SMARTSTART_PRIME_NODE_INFO,   QUEUE_EXPLORE_CMD_AUTOINCLUSION, 4000},
    {ZWAVE_CMD_SMARTSTART_INCLUDE_NODE_INFO, QUEUE_EXPLORE_CMD_AUTOINCLUSION | TRANSMIT_OPTION_EXPLORE_DO_ACK_TIMEOUT, 6800},

    // 1
    {ZWAVE_CMD_SMARTSTART_PRIME_NODE_INFO,   QUEUE_EXPLORE_CMD_AUTOINCLUSION, 4000},
    {ZWAVE_CMD_SMARTSTART_INCLUDE_NODE_INFO, QUEUE_EXPLORE_CMD_AUTOINCLUSION | TRANSMIT_OPTION_EXPLORE_DO_ACK_TIMEOUT, 19900},

    // 2
    {ZWAVE_CMD_SMARTSTART_PRIME_NODE_INFO,   QUEUE_EXPLORE_CMD_AUTOINCLUSION, 4000},
    {ZWAVE_CMD_SMARTSTART_INCLUDE_NODE_INFO, QUEUE_EXPLORE_CMD_AUTOINCLUSION | TRANSMIT_OPTION_EXPLORE_DO_ACK_TIMEOUT, 39600},

    // 3
    {ZWAVE_CMD_SMARTSTART_PRIME_NODE_INFO,   QUEUE_EXPLORE_CMD_AUTOINCLUSION, 4000},
    {ZWAVE_CMD_SMARTSTART_INCLUDE_NODE_INFO, QUEUE_EXPLORE_CMD_AUTOINCLUSION | TRANSMIT_OPTION_EXPLORE_DO_ACK_TIMEOUT, 91600},

    // 4
    {ZWAVE_CMD_SMARTSTART_PRIME_NODE_INFO,   QUEUE_EXPLORE_CMD_AUTOINCLUSION, 4000},
    {ZWAVE_CMD_SMARTSTART_INCLUDE_NODE_INFO, QUEUE_EXPLORE_CMD_AUTOINCLUSION | TRANSMIT_OPTION_EXPLORE_DO_ACK_TIMEOUT, 128000},

    // 5
    {ZWAVE_CMD_SMARTSTART_PRIME_NODE_INFO,   QUEUE_EXPLORE_CMD_AUTOINCLUSION, 4000},
    {ZWAVE_CMD_SMARTSTART_INCLUDE_NODE_INFO, QUEUE_EXPLORE_CMD_AUTOINCLUSION | TRANSMIT_OPTION_EXPLORE_DO_ACK_TIMEOUT, 363200},

    // 6
    {ZWAVE_CMD_SMARTSTART_PRIME_NODE_INFO,   QUEUE_EXPLORE_CMD_AUTOINCLUSION, 4000},
    {ZWAVE_CMD_SMARTSTART_INCLUDE_NODE_INFO, QUEUE_EXPLORE_CMD_AUTOINCLUSION | TRANSMIT_OPTION_EXPLORE_DO_ACK_TIMEOUT, 512000},

    // 7
    {ZWAVE_CMD_SMARTSTART_PRIME_NODE_INFO,   QUEUE_EXPLORE_CMD_AUTOINCLUSION, 4000},
    {ZWAVE_CMD_SMARTSTART_INCLUDE_NODE_INFO, QUEUE_EXPLORE_CMD_AUTOINCLUSION | TRANSMIT_OPTION_EXPLORE_DO_ACK_TIMEOUT, 512000},
  };

  mock_call_use_as_fake(TO_STR(zpal_retention_register_write));
  mock_call_use_as_fake(TO_STR(zpal_retention_register_read));

  mock_call_expect(TO_STR(SlaveStorageSetSmartStartState), &pMock);
  pMock->expect_arg[ARG0].value = NVM_SYSTEM_STATE_IDLE;

  mock_call_expect(TO_STR(ZW_mainDeepSleepPowerLockEnable), &pMock);
  pMock->expect_arg[ARG0].value = true;

  mock_call_expect(TO_STR(zpal_pm_cancel), &pMock); //smart_start_power_lock
  pMock->expect_arg[ARG0].p = ss_power_lock;

  mock_call_expect(TO_STR(SlaveStorageSetSmartStartState), &pMock);
  pMock->expect_arg[ARG0].value = NVM_SYSTEM_STATE_SMART_START;

  mock_call_expect(TO_STR(InternSetLearnMode), &pMock);
  pMock->expect_arg[ARG0].value = ZW_SET_LEARN_MODE_SMARTSTART;
  pMock->compare_rule_arg[ARG1] = COMPARE_NOT_NULL;

  for (uint32_t i = 0; i < sizeof_array(test_combinations); i++)
  {
    mock_call_expect(TO_STR(TimerStop), &pMock);
    pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL; //LearnStateTimer

    mock_call_expect(TO_STR(ZW_TransmitCallbackBind), &pMockTransmitCallbackBind);
    pMockTransmitCallbackBind->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
    pMockTransmitCallbackBind->compare_rule_arg[ARG1] = COMPARE_NOT_NULL;
    pMockTransmitCallbackBind->compare_rule_arg[ARG2] = COMPARE_NULL;

    // The following mocks is set up for functions invoked in ZW_ExploreRequestInclusion().

    mock_call_expect(TO_STR(SlaveStorageGetNetworkIds), &pMock);
    pMock->compare_rule_arg[0] = COMPARE_NULL;
    pMock->compare_rule_arg[1] = COMPARE_NOT_NULL;
    pMock->output_arg[1].v = g_nodeID;

    bNetworkWideInclusion = NETWORK_WIDE_MODE_JOIN; // Set by ZW_slave
    g_learnModeDsk = true; // Set by ZW_slave

    EXPLORE_REMOTE_INCLUSION_REQUEST_FRAME frame;
    for (uint32_t i = 0; i < sizeof(EXPLORE_REMOTE_INCLUSION_REQUEST_FRAME); i++)
    {
      *((uint8_t *)&frame + i) = i;
    }
    frame.networkHomeID[0] = 0;
    frame.networkHomeID[1] = 0;
    frame.networkHomeID[2] = 0;
    frame.networkHomeID[3] = 0;

    frame.header.cmd = test_combinations[i].command;
#if FOR_DEBUGGING
    printf("Prod: ");
    for (uint32_t i = 0; i < sizeof(EXPLORE_REMOTE_INCLUSION_REQUEST_FRAME); i++)
    {
      printf("%02x ", *((uint8_t *)&frame + i));
    }
    printf("\n");
#endif
    mock_call_expect(TO_STR(GenerateNodeInformation), &pMock);
    pMock->compare_rule_arg[0] = COMPARE_NOT_NULL;
    pMock->output_arg[0].p = (uint8_t *)&frame + 4;
    pMock->expect_arg[1].v = 0x01; // ZWAVE_CMD_CLASS_PROTOCOL
    pMock->return_code.v = sizeof(NODEINFO_SLAVE_FRAME); // Use max size as we do not care about content

    mock_call_expect(TO_STR(ExploreQueueFrame), &pMock);
    pMock->expect_arg[0].v = g_nodeID;
    pMock->expect_arg[1].v = NODE_BROADCAST;
    pMock->expect_arg[2].p = &frame;
    pMock->expect_arg[3].v = sizeof(EXPLORE_REMOTE_INCLUSION_REQUEST_FRAME);
    pMock->expect_arg[4].v = test_combinations[i].transmit_options;
    pMock->compare_rule_arg[5] = COMPARE_NOT_NULL;

    // End of mocks for ZW_ExploreRequestInclusion().

    mock_call_expect(TO_STR(zpal_pm_stay_awake), &pMock);  //smart_start_power_lock
    pMock->expect_arg[ARG0].p = ss_power_lock;
    pMock->expect_arg[ARG1].value = 5000; //LEARN_MODE_SMARTSTART_EXPLORE_TIMEOUT

    if (0 == i) {
      // First time we invoke initiating function.
      mock_call_use_as_stub(TO_STR(zpal_radio_get_lr_channel_config));
      ZW_NetworkLearnModeStart(E_NETWORK_LEARN_MODE_INCLUSION_SMARTSTART);
    } else {
      // Invoke timer callback
        LearnStateTimerCallback(NULL); // Timer object is not used
    }

    /*
     * Fetch callback function passed to ZW_TransmitCallbackBind() bound to callback object used in
     * ExploreQueueFrame().
     */
    ExplorerQueueCallback = (ZW_SendData_Callback_t)pMockTransmitCallbackBind->actual_arg[1].p;

    /* *********************************************************************************************
     * Setup mocks before invoking 1. Explore callback (Explore Prime)
     */

    mock_call_expect(TO_STR(TimerStart), &pMock);
    pMock->compare_rule_arg[0] = COMPARE_NOT_NULL;
    pMock->expect_arg[1].v = test_combinations[i].time; // Time between Prime and Include frames
    pMock->return_code.v = ESWTIMER_STATUS_SUCCESS;

    mock_call_expect(TO_STR(zpal_pm_cancel), &pMock); //smart_start_power_lock
    pMock->expect_arg[ARG0].p = ss_power_lock;

    ExplorerQueueCallback(NULL, 0, NULL); // None of the arguments are used.
  }

  // Stop learn mode
  mock_call_expect(TO_STR(SlaveStorageSetSmartStartState), &pMock);
  pMock->expect_arg[ARG0].value = NVM_SYSTEM_STATE_IDLE;

  mock_call_expect(TO_STR(InternSetLearnMode), &pMock);
  pMock->expect_arg[ARG0].value = ZW_SET_LEARN_MODE_DISABLE;
  pMock->compare_rule_arg[ARG1] = COMPARE_NULL;

  mock_call_expect(TO_STR(TimerStop), &pMock);
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL; //LearnStateTimer

  mock_call_expect(TO_STR(ZW_mainDeepSleepPowerLockEnable), &pMock);
  pMock->expect_arg[ARG0].value = false;

  if (!g_nodeID) {
    mock_call_use_as_stub(TO_STR(zpal_radio_get_lr_channel_config));
  }

  ZW_NetworkLearnModeStart(E_NETWORK_LEARN_MODE_DISABLE);

  mock_calls_verify();
}

typedef struct
{
  bool isLR;
  uint8_t command;
  uint8_t transmit_options;
  uint32_t time;
}
test_lr_t;

/*
 * Verifies the flow from starting learn mode until the full time period in between prime/include
 * transmission is reached.
 *
 * Prerequisites:
 * - Always on device
 *
 * Variables for table of combinations:
 * - Time between transmission of prime/include and the next transmission of both
 * - Command (Prime / Include)
 * - Explore transmit options
 */
void test_smartstart_flow_with_LR(void)
{
  mock_t * pMock;
  mock_t * pMockTransmitCallbackBind;
  bool lr_lock_registered = false;

  mock_calls_clear();

  g_nodeID = 0;

  /*
   * The timing is calculated using GetRandom
   *
   * bRequestNo = 0
   * ---------------
   * 0x01 ^ 0xAA + 0x11 = 0xBC (188)
   * 188 % 40 = 28
   * 28 * 100 = 2800
   *
   * bRequestNo = 1
   * ---------------
   * 0xBC ^ 0xAA + 0x11 = 0x27 (39)
   * 39 % 40 = 39
   * 39 * 100 = 3900
   * 3900 + 16000 = 19900
   *
   * bRequestNo = 2
   * ---------------
   * 0x27 ^ 0xAA + 0x11 = 0x9E (158)
   * 158 % 40 = 38
   * 158 << (2-1) = 76
   * 76 * 100 = 7600
   * 7600 + 32000 = 39600
   *
   * bRequestNo = 3
   * ---------------
   * 0x9E ^ 0xAA + 0x11 = 0x45 (69)
   * 69 % 40 = 29
   * 29 << (3-1) = 116
   * 116 * 100 = 11600
   * 11600 + 64000 = 75600
   *
   * bRequestNo = 4
   * ---------------
   * 0x45 ^ 0xAA + 0x11 = 0
   * 0 % 40 = 0
   * 0 << (4-1) = 0
   * 0 * 100 = 0
   * 0 + 128000 = 128000
   *
   * bRequestNo = 5
   * ---------------
   * 0x00 ^ 0xAA + 0x11 = 0xBB (187)
   * 187 % 40 = 27
   * 27 << (5-1) = 432
   * 432 * 100 = 43200
   * 43200 + 256000 = 299200
   */

  const uint8_t random_sequence[] = {0xBC, 0x27, 0x9E, 0x45, 0x00, 0xBB};
  for (size_t i = 0; i < sizeof_array(random_sequence); ++i) {
    mock_call_expect(TO_STR(zpal_get_pseudo_random), &pMock);
    pMock->return_code.v = random_sequence[i];
  }

  const test_lr_t test_combinations[] = {
    // 0 (0-3)
    {true, ZWAVE_CMD_SMARTSTART_PRIME_NODE_INFO,   0, 4000},
    {true, ZWAVE_CMD_SMARTSTART_INCLUDE_NODE_INFO, 0, 2000},
    {false, ZWAVE_CMD_SMARTSTART_PRIME_NODE_INFO,   QUEUE_EXPLORE_CMD_AUTOINCLUSION, 4000},
    {false, ZWAVE_CMD_SMARTSTART_INCLUDE_NODE_INFO, QUEUE_EXPLORE_CMD_AUTOINCLUSION | TRANSMIT_OPTION_EXPLORE_DO_ACK_TIMEOUT, 2800},

    // 1 (4-7)
    {true, ZWAVE_CMD_SMARTSTART_PRIME_NODE_INFO,    0, 4000},
    {true, ZWAVE_CMD_SMARTSTART_INCLUDE_NODE_INFO,  0, 2000},
    {false, ZWAVE_CMD_SMARTSTART_PRIME_NODE_INFO,   QUEUE_EXPLORE_CMD_AUTOINCLUSION, 4000},
    {false, ZWAVE_CMD_SMARTSTART_INCLUDE_NODE_INFO, QUEUE_EXPLORE_CMD_AUTOINCLUSION | TRANSMIT_OPTION_EXPLORE_DO_ACK_TIMEOUT, 19900},

    // 2 (8-11)
    {true, ZWAVE_CMD_SMARTSTART_PRIME_NODE_INFO,    0, 4000},
    {true, ZWAVE_CMD_SMARTSTART_INCLUDE_NODE_INFO,  0, 2000},
    {false, ZWAVE_CMD_SMARTSTART_PRIME_NODE_INFO,   QUEUE_EXPLORE_CMD_AUTOINCLUSION, 4000},
    {false, ZWAVE_CMD_SMARTSTART_INCLUDE_NODE_INFO, QUEUE_EXPLORE_CMD_AUTOINCLUSION | TRANSMIT_OPTION_EXPLORE_DO_ACK_TIMEOUT, 39600},

    // 3 (12-15)
    {true, ZWAVE_CMD_SMARTSTART_PRIME_NODE_INFO,    0, 4000},
    {true, ZWAVE_CMD_SMARTSTART_INCLUDE_NODE_INFO,  0, 2000},
    {false, ZWAVE_CMD_SMARTSTART_PRIME_NODE_INFO,   QUEUE_EXPLORE_CMD_AUTOINCLUSION, 4000},
    {false, ZWAVE_CMD_SMARTSTART_INCLUDE_NODE_INFO, QUEUE_EXPLORE_CMD_AUTOINCLUSION | TRANSMIT_OPTION_EXPLORE_DO_ACK_TIMEOUT, 75600},

    // 4 (16-19)
    {true, ZWAVE_CMD_SMARTSTART_PRIME_NODE_INFO,    0, 4000},
    {true, ZWAVE_CMD_SMARTSTART_INCLUDE_NODE_INFO,  0, 2000},
    {false, ZWAVE_CMD_SMARTSTART_PRIME_NODE_INFO,   QUEUE_EXPLORE_CMD_AUTOINCLUSION, 4000},
    {false, ZWAVE_CMD_SMARTSTART_INCLUDE_NODE_INFO, QUEUE_EXPLORE_CMD_AUTOINCLUSION | TRANSMIT_OPTION_EXPLORE_DO_ACK_TIMEOUT, 128000},

    // 5 (20-23)
    {true, ZWAVE_CMD_SMARTSTART_PRIME_NODE_INFO,    0, 4000},
    {true, ZWAVE_CMD_SMARTSTART_INCLUDE_NODE_INFO,  0, 2000},
    {false, ZWAVE_CMD_SMARTSTART_PRIME_NODE_INFO,   QUEUE_EXPLORE_CMD_AUTOINCLUSION, 4000},
    {false, ZWAVE_CMD_SMARTSTART_INCLUDE_NODE_INFO, QUEUE_EXPLORE_CMD_AUTOINCLUSION | TRANSMIT_OPTION_EXPLORE_DO_ACK_TIMEOUT, 299200},

    // 6 (24-27)
    {true, ZWAVE_CMD_SMARTSTART_PRIME_NODE_INFO,    0, 4000},
    {true, ZWAVE_CMD_SMARTSTART_INCLUDE_NODE_INFO,  0, 2000},
    {false, ZWAVE_CMD_SMARTSTART_PRIME_NODE_INFO,   QUEUE_EXPLORE_CMD_AUTOINCLUSION, 4000},
    {false, ZWAVE_CMD_SMARTSTART_INCLUDE_NODE_INFO, QUEUE_EXPLORE_CMD_AUTOINCLUSION | TRANSMIT_OPTION_EXPLORE_DO_ACK_TIMEOUT, 512000},

    // 7 (28-31)
    {true, ZWAVE_CMD_SMARTSTART_PRIME_NODE_INFO,    0, 4000},
    {true, ZWAVE_CMD_SMARTSTART_INCLUDE_NODE_INFO,  0, 2000},
    {false, ZWAVE_CMD_SMARTSTART_PRIME_NODE_INFO,   QUEUE_EXPLORE_CMD_AUTOINCLUSION, 4000},
    {false, ZWAVE_CMD_SMARTSTART_INCLUDE_NODE_INFO, QUEUE_EXPLORE_CMD_AUTOINCLUSION | TRANSMIT_OPTION_EXPLORE_DO_ACK_TIMEOUT, 512000},

    // 8 (32-35)
    {true, ZWAVE_CMD_SMARTSTART_PRIME_NODE_INFO,    0, 4000},
    {true, ZWAVE_CMD_SMARTSTART_INCLUDE_NODE_INFO,  0, 2000},
    {false, ZWAVE_CMD_SMARTSTART_PRIME_NODE_INFO,   QUEUE_EXPLORE_CMD_AUTOINCLUSION, 4000},
    {false, ZWAVE_CMD_SMARTSTART_INCLUDE_NODE_INFO, QUEUE_EXPLORE_CMD_AUTOINCLUSION | TRANSMIT_OPTION_EXPLORE_DO_ACK_TIMEOUT, 512000},

    // 9 (36-39)
    {true, ZWAVE_CMD_SMARTSTART_PRIME_NODE_INFO,    0, 4000},
    {true, ZWAVE_CMD_SMARTSTART_INCLUDE_NODE_INFO,  0, 2000},
    {false, ZWAVE_CMD_SMARTSTART_PRIME_NODE_INFO,   QUEUE_EXPLORE_CMD_AUTOINCLUSION, 4000},
    {false, ZWAVE_CMD_SMARTSTART_INCLUDE_NODE_INFO, QUEUE_EXPLORE_CMD_AUTOINCLUSION | TRANSMIT_OPTION_EXPLORE_DO_ACK_TIMEOUT, 512000},
  };

  mock_call_use_as_stub(TO_STR(GetResetReason));

  mock_call_use_as_fake(TO_STR(zpal_retention_register_write));
  mock_call_use_as_fake(TO_STR(zpal_retention_register_read));

  // NetworkManagementEnterSmartStart()
  mock_call_expect(TO_STR(SlaveStorageSetSmartStartState), &pMock);
  pMock->expect_arg[ARG0].value = NVM_SYSTEM_STATE_IDLE;

  mock_call_expect(TO_STR(ZW_mainDeepSleepPowerLockEnable), &pMock);
  pMock->expect_arg[ARG0].value = true;

  // from NetworkManagementSmartStartEnable()
  mock_call_expect(TO_STR(NodeSupportsBeingIncludedAsLR), &pMock);
  pMock->return_code.v = true;

  mock_call_expect(TO_STR(zpal_pm_cancel), &pMock); //smart_start_power_lock
  pMock->expect_arg[ARG0].p = ss_power_lock;

  mock_call_expect(TO_STR(SlaveStorageSetSmartStartState), &pMock);
  pMock->expect_arg[ARG0].value = NVM_SYSTEM_STATE_SMART_START;

  // NetworkManagementLearnStateProcess
  mock_call_expect(TO_STR(InternSetLearnMode), &pMock);
  pMock->expect_arg[ARG0].value = ZW_SET_LEARN_MODE_SMARTSTART;
  pMock->compare_rule_arg[ARG1] = COMPARE_NOT_NULL;

  bool stateSwitchRegion = false;

  // ZCB_NetworkManagementLearnNodeStateTimeout
  for (uint32_t i = 0; i < sizeof_array(test_combinations); i++)
  {
    mock_call_expect(TO_STR(TimerStop), &pMock);
    pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL; //LearnStateTimer

    mock_call_expect(TO_STR(ZW_TransmitCallbackBind), &pMockTransmitCallbackBind);
    pMockTransmitCallbackBind->compare_rule_arg[ARG0] = COMPARE_NOT_NULL;
    pMockTransmitCallbackBind->compare_rule_arg[ARG1] = COMPARE_NOT_NULL;
    pMockTransmitCallbackBind->compare_rule_arg[ARG2] = COMPARE_NULL;

    // The following mocks is set up for functions invoked in ZW_ExploreRequestInclusion().

    mock_call_expect(TO_STR(SlaveStorageGetNetworkIds), &pMock);
    pMock->compare_rule_arg[0] = COMPARE_NULL;
    pMock->compare_rule_arg[1] = COMPARE_NOT_NULL;
    pMock->output_arg[1].v = g_nodeID;

    bNetworkWideInclusion = NETWORK_WIDE_MODE_JOIN; // Set by ZW_slave
    g_learnModeDsk = true; // Set by ZW_slave

    EXPLORE_REMOTE_INCLUSION_REQUEST_FRAME frame;
    for (uint32_t i = 0; i < sizeof(EXPLORE_REMOTE_INCLUSION_REQUEST_FRAME); i++)
    {
      *((uint8_t *)&frame + i) = i;
    }
    frame.networkHomeID[0] = 0;
    frame.networkHomeID[1] = 0;
    frame.networkHomeID[2] = 0;
    frame.networkHomeID[3] = 0;

    frame.header.cmd = test_combinations[i].command;
#if 0 // Extra debugging
    printf("Prod: ");
    for (uint32_t i = 0; i < sizeof(EXPLORE_REMOTE_INCLUSION_REQUEST_FRAME); i++)
    {
      printf("%02x ", *((uint8_t *)&frame + i));
    }
    printf("\n");
#endif
    mock_call_expect(TO_STR(GenerateNodeInformation), &pMock);
    pMock->compare_rule_arg[0] = COMPARE_NOT_NULL;
    pMock->output_arg[0].p = (uint8_t *)&frame + 4;
    if (true == test_combinations[i].isLR)
    {
      pMock->expect_arg[1].v = 0x04; // ZWAVE_CMD_CLASS_PROTOCOL_LR
    }
    else
    {
      pMock->expect_arg[1].v = 0x01; // ZWAVE_CMD_CLASS_PROTOCOL
    }
    pMock->return_code.v = sizeof(NODEINFO_SLAVE_FRAME); // Use max size as we do not care about content

    mock_call_expect(TO_STR(NodeSupportsBeingIncludedAsLR), &pMock);
    pMock->return_code.v = true;

    // if isLR is true, we expect a normal transmission and not Explore.
    if (true == test_combinations[i].isLR)
    {
      if ( !stateSwitchRegion ) {
        mock_call_expect(TO_STR(NodeSupportsBeingIncludedAsLR), &pMock);
        pMock->return_code.v = true;

        mock_call_expect(TO_STR(zpal_radio_get_lr_channel_config), &pMock);
        pMock->return_code.v = ZPAL_RADIO_LR_CH_CFG1;

        mock_call_expect(TO_STR(llChangeRfPHYToLrChannelConfig), &pMock);
        pMock->expect_arg[0].v = ZPAL_RADIO_LR_CH_CFG3;
        pMock->return_code.v = true;

        stateSwitchRegion = true;
      }
      else
      {
        mock_call_expect(TO_STR(NodeSupportsBeingIncludedAsLR), &pMock);
        pMock->return_code.v = true;
      }

      mock_call_expect(TO_STR(EnQueueSingleDataOnLRChannels), &pMock);
      pMock->expect_arg[0].v = RF_SPEED_LR_100K;
      pMock->expect_arg[1].v = 0; // Source node ID
      pMock->expect_arg[2].v = NODE_BROADCAST_LR; // Destination node ID
      pMock->expect_arg[3].p = (uint8_t *)&frame + 4; // Do not expect the first four home ID bytes
      pMock->expect_arg[4].v = sizeof(EXPLORE_REMOTE_INCLUSION_REQUEST_FRAME) - 4;
      pMock->expect_arg[5].v = 0;  // No tx delays needed.
      pMock->expect_arg[6].v = ZPAL_RADIO_TX_POWER_REDUCED;
      pMock->compare_rule_arg[7] = COMPARE_NOT_NULL; // Callback
      pMock->return_code.v = true;
    }
    else
    {
      if ( stateSwitchRegion ) {
        mock_call_expect(TO_STR(NodeSupportsBeingIncludedAsLR), &pMock);
        pMock->return_code.v = true;

        mock_call_expect(TO_STR(llChangeRfPHYToLrChannelConfig), &pMock);
        pMock->expect_arg[0].v = ZPAL_RADIO_LR_CH_CFG1;
        pMock->return_code.v = true;

        stateSwitchRegion = false;
      }
      else
      {
        mock_call_expect(TO_STR(NodeSupportsBeingIncludedAsLR), &pMock);
        pMock->return_code.v = true;

      }

      mock_call_expect(TO_STR(ExploreQueueFrame), &pMock);
      pMock->expect_arg[0].v = g_nodeID;
      pMock->expect_arg[1].v = NODE_BROADCAST;
      pMock->expect_arg[2].p = &frame;
      pMock->expect_arg[3].v = sizeof(EXPLORE_REMOTE_INCLUSION_REQUEST_FRAME);
      pMock->expect_arg[4].v = test_combinations[i].transmit_options;
      pMock->compare_rule_arg[5] = COMPARE_NOT_NULL;
    }

    // End of mocks for ZW_ExploreRequestInclusion().

    mock_call_expect(TO_STR(zpal_pm_stay_awake), &pMock);  //smart_start_power_lock
    pMock->expect_arg[ARG0].p = ss_power_lock;
    pMock->expect_arg[ARG1].value = 5000; //LEARN_MODE_SMARTSTART_EXPLORE_TIMEOUT

    if (0 == i) {
      // First time we invoke initiating function.
      if (!g_nodeID) {
        mock_call_expect(TO_STR(zpal_radio_get_lr_channel_config), &pMock);
        pMock->return_code.v = ZPAL_RADIO_LR_CH_CFG1;  // we don't try to change region since we already using the default region
      }
      ZW_NetworkLearnModeStart(E_NETWORK_LEARN_MODE_INCLUSION_SMARTSTART);
    } else {
      // Invoke timer callback
        LearnStateTimerCallback(NULL); // Timer object is not used
    }


    /*
     * Fetch callback function passed to ZW_TransmitCallbackBind() bound to callback object used in
     * ExploreQueueFrame().
     */
    ExplorerQueueCallback = (ZW_SendData_Callback_t)pMockTransmitCallbackBind->actual_arg[1].p;

    /* *********************************************************************************************
     * Setup mocks before invoking 1. Explore callback (LR Prime)
     */

    // Mocks for ZCB_NetworkManagementExploreRequestComplete, called by ExplorerQueueCallback
    mock_call_expect(TO_STR(NodeSupportsBeingIncludedAsLR), &pMock);
    pMock->return_code.v = true;

    mock_call_expect(TO_STR(NodeSupportsBeingIncludedAsLR), &pMock);
    pMock->return_code.v = true;

    // Expect these mocks only after a LR include
    if (i % 4 == 1)
    {
      if (!lr_lock_registered)
      {
        mock_call_expect(TO_STR(zpal_pm_register), &pMock); //long_range_power_lock
        pMock->expect_arg[0].v = ZPAL_PM_TYPE_USE_RADIO;
        pMock->return_code.p = lr_power_lock;
        lr_lock_registered = true;
      }

      mock_call_expect(TO_STR(zpal_pm_stay_awake), &pMock); //long_range_power_lock
      pMock->expect_arg[0].p = lr_power_lock;
      pMock->expect_arg[1].v = 2000;
    }

    mock_call_expect(TO_STR(TimerStart), &pMock);
    pMock->compare_rule_arg[0] = COMPARE_NOT_NULL;
    pMock->expect_arg[1].v = test_combinations[i].time; // Time between Prime and Include frames
    pMock->return_code.v = ESWTIMER_STATUS_SUCCESS;

    mock_call_expect(TO_STR(zpal_pm_cancel), &pMock); //smart_start_power_lock
    pMock->expect_arg[ARG0].p = ss_power_lock;

    // Trigger ZCB_NetworkManagementExploreRequestComplete()
    ExplorerQueueCallback(NULL, 0, NULL); // None of the arguments are used.
  }

  // Stop learn mode
  mock_call_expect(TO_STR(SlaveStorageSetSmartStartState), &pMock);
  pMock->expect_arg[ARG0].value = NVM_SYSTEM_STATE_IDLE;

  mock_call_expect(TO_STR(InternSetLearnMode), &pMock);
  pMock->expect_arg[ARG0].value = ZW_SET_LEARN_MODE_DISABLE;
  pMock->compare_rule_arg[ARG1] = COMPARE_NULL;

  mock_call_expect(TO_STR(TimerStop), &pMock);
  pMock->compare_rule_arg[ARG0] = COMPARE_NOT_NULL; //LearnStateTimer

  mock_call_expect(TO_STR(ZW_mainDeepSleepPowerLockEnable), &pMock);
  pMock->expect_arg[ARG0].value = false;
  if (!g_nodeID) {
    mock_call_expect(TO_STR(zpal_radio_get_lr_channel_config), &pMock);
    pMock->return_code.v = ZPAL_RADIO_LR_CH_CFG1;  // we don't try to change region since we already using the default region
  }
  ZW_NetworkLearnModeStart(E_NETWORK_LEARN_MODE_DISABLE);

  mock_calls_verify();
}
