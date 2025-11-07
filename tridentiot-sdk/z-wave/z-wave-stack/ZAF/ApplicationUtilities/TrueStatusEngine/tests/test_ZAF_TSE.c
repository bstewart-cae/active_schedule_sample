// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file test_ZAF_TSE.c
 * @copyright 2022 Silicon Laboratories Inc.
 */

#include <mock_control.h>
#include <stdlib.h>
#include "ZAF_TSE.h"
#include "zaf_tse_config.h"
#include "ZAF_types.h"
#include "SwTimer.h"
#include <string.h>
#include "ZW_TransportEndpoint.h"
#include "zaf_transport_tx.h"

void setUpSuite(void) {

}

void tearDownSuite(void) {

}

/* Simulating the type of object passed to the TSE*/
typedef struct _my_personalized_struct_t_{
  RECEIVE_OPTIONS_TYPE_EX rxOptions;
  void * pCommandPayload;
}my_personalized_struct_t;

/* Variables used to pass in argument*/
static my_personalized_struct_t TestData;
static my_personalized_struct_t TestData2;
static MULTICHAN_NODE_ID pList[5];

/* An array of callback function */
static zaf_tse_callback_t callback_functions[ZAF_TSE_MAXIMUM_SIMULTANEOUS_TRIGGERS+1];

static int TestCallbackCounter = 0;
/* Dummy callback function to pass to the TSE trigger */
void TestCallback(zaf_tx_options_t *tx_options, void* pData)
{
  TestCallbackCounter++;
}

/* Function to set automatically the multicast mock parameters */
void set_is_multicast_to_false(mock_t * pMock)
{
  pMock->compare_rule_arg[0] = COMPARE_NOT_NULL;
  pMock->return_code.v = false;
}

/* Function to set automatically the handleAssociationGetnodeList mock parameters */
void set_handleAssociationGetnodeList_default_mock(mock_t * pMock)
{
  /* Set up the Test data, used for rx Options and association mock verify */
  TestData.rxOptions.destNode.endpoint = 0;
  TestData.rxOptions.sourceNode.nodeId = 2;
  TestData.rxOptions.sourceNode.endpoint = 0;
  pMock->expect_arg[0].v = ZAF_TSE_GROUP_ID;
  pMock->expect_arg[1].v = 0; //Has to be Root Device Association Group
  pMock->compare_rule_arg[2] = COMPARE_NOT_NULL;
  pMock->compare_rule_arg[3] = COMPARE_NOT_NULL;
}

/* This should be done at the beginning of each test*/
static void init_tse(mock_t *pMock, SSwTimer** zaf_tse_timer,void (**pTimerCallback)(SSwTimer*))
{

  /* First init our local callback functions */
  for (uint8_t i=0;i<ZAF_TSE_MAXIMUM_SIMULTANEOUS_TRIGGERS+1; i++){
    callback_functions[i] = malloc(sizeof(zaf_tse_callback_t));
  }

  /* We expect timer configuration calls */
  mock_call_expect(TO_STR(AppTimerRegister), &pMock);
  pMock->compare_rule_arg[0] = COMPARE_NOT_NULL;
  pMock->expect_arg[1].v = false;
  pMock->compare_rule_arg[2] = COMPARE_NOT_NULL;
  pMock->return_code.v = true;

  ZAF_TSE_Init();

  /* Assign the timer and callback values */
  *zaf_tse_timer = pMock->actual_arg[0].p;
  *pTimerCallback = pMock->actual_arg[2].p;
}

static void deinit_tse(void)
{
  for (uint8_t i=0;i<ZAF_TSE_MAXIMUM_SIMULTANEOUS_TRIGGERS+1; i++){
    free(callback_functions[i]);
  }
}

/* Testing the ZAF TSE Init function */
void test_ZAF_TSE_Init(void)
{
  mock_t * pMock = NULL;
  SSwTimer* zaf_tse_timer;
  void (*pCallback)(SSwTimer*);
  mock_calls_clear();

  init_tse(pMock, &zaf_tse_timer,&pCallback);

  deinit_tse();

  mock_calls_verify();
}


/* Testing a failed ZAF TSE Init function */
void test_ZAF_TSE_Init_failed(void)
{
  mock_t * pMock;
  mock_calls_clear();

  mock_call_expect(TO_STR(AppTimerRegister), &pMock);
  pMock->compare_rule_arg[0] = COMPARE_NOT_NULL;
  pMock->expect_arg[1].v = false;
  pMock->compare_rule_arg[2] = COMPARE_NOT_NULL;
  pMock->return_code.v = false;

  ZAF_TSE_Init();

  mock_calls_verify();
}

/* Testing the ZAF TSE Trigger function when arguments are not valid */
void test_ZAF_TSE_Trigger_NULL_arguments(void)
{
  /* NULL Callback function */
  TEST_ASSERT_FALSE_MESSAGE(ZAF_TSE_Trigger(NULL, &TestData, true), "NULL pCallback was not rejected");

  /* NULL Data pointer  */
  TEST_ASSERT_FALSE_MESSAGE(ZAF_TSE_Trigger(callback_functions[0], NULL, true), "NULL pData was not rejected");
}


/* Testing the ZAF TSE Trigger function when rxOption is received via multicast */
void test_ZAF_TSE_Trigger_received_via_muticast(void)
{
  mock_t * pMock;
  SSwTimer* zaf_tse_timer;
  void (*pCallback)(SSwTimer*);
  mock_calls_clear();
  init_tse(pMock, &zaf_tse_timer, &pCallback);

  /* Test return value if received via Multicast */
  TestData.rxOptions.destNode.BitAddress = 0;
  mock_call_expect(TO_STR(is_multicast), &pMock);
  pMock->compare_rule_arg[0] = COMPARE_NOT_NULL;
  pMock->return_code.v = true;

  TEST_ASSERT_TRUE_MESSAGE(ZAF_TSE_Trigger(callback_functions[0], &TestData, true), "Broadcast/multicast trigger was rejected");

  deinit_tse();

  mock_calls_verify();
}

/* Testing the ZAF TSE Trigger function when rxOption is received with bit address */
void test_ZAF_TSE_Trigger_received_bit_address(void)
{
  mock_t * pMock;
  SSwTimer* zaf_tse_timer;
  void (*pCallback)(SSwTimer*);
  mock_calls_clear();
  init_tse(pMock, &zaf_tse_timer, &pCallback);

  /* Test return value if received via Multicast */
  TestData.rxOptions.destNode.BitAddress = 1;
  mock_call_expect(TO_STR(is_multicast), &pMock);
  pMock->compare_rule_arg[0] = COMPARE_NOT_NULL;
  pMock->return_code.v = false;

  /* Trigger with empty destination association group */
  mock_call_expect(TO_STR(handleAssociationGetnodeList), &pMock);
  set_handleAssociationGetnodeList_default_mock(pMock);
  pMock->output_arg[3].v = 0;

  TEST_ASSERT_TRUE_MESSAGE(ZAF_TSE_Trigger(callback_functions[0], &TestData, true), "Trigger with bit address set to 1 was rejected");

  deinit_tse();

  mock_calls_verify();
}

/* Testing the ZAF TSE Trigger function when no destination is present in the target GROUP_ID */
void test_ZAF_TSE_Trigger_no_association_in_target_group(void)
{
  mock_t * pMock;
  SSwTimer* zaf_tse_timer;
  void (*pCallback)(SSwTimer*);
  mock_calls_clear();
  init_tse(pMock, &zaf_tse_timer,&pCallback);

  /* Test return value if received via Multicast */
  TestData.rxOptions.destNode.BitAddress = 1;
  mock_call_expect(TO_STR(is_multicast), &pMock);
  set_is_multicast_to_false(pMock);

  /* Trigger with empty destination association group */
  mock_call_expect(TO_STR(handleAssociationGetnodeList), &pMock);
  set_handleAssociationGetnodeList_default_mock(pMock);
  pMock->output_arg[3].v = 0;

  TEST_ASSERT_TRUE_MESSAGE(ZAF_TSE_Trigger(callback_functions[0], &TestData, true), "Trigger with no association on ZAF_TSE_GROUP_ID was rejected");

  deinit_tse();

  mock_calls_verify();
}


/* Testing the ZAF TSE Trigger function when the change was triggerd by the lifeline destination */
void test_ZAF_TSE_Trigger_changed_made_by_lifeline(void)
{
  mock_t * pMock;
  SSwTimer* zaf_tse_timer;
  void (*pCallback)(SSwTimer*);
  mock_calls_clear();
  init_tse(pMock, &zaf_tse_timer,&pCallback);

  /* Test return value if received via Multicast */
  TestData.rxOptions.destNode.BitAddress = 0;
  mock_call_expect(TO_STR(is_multicast), &pMock);
  set_is_multicast_to_false(pMock);

  /* Trigger with identical RxOptions / lifeline destination */
  mock_call_expect(TO_STR(handleAssociationGetnodeList), &pMock);
  set_handleAssociationGetnodeList_default_mock(pMock);
  pList[0].node.nodeId = TestData.rxOptions.sourceNode.nodeId;
  pList[0].node.endpoint = TestData.rxOptions.sourceNode.endpoint;
  pMock->output_arg[2].p = pList;
  pMock->output_arg[3].v = 1;

  TEST_ASSERT_TRUE_MESSAGE(ZAF_TSE_Trigger(callback_functions[0], &TestData, true), "Trigger initiated by lifeline change was rejected");

  deinit_tse();

  mock_calls_verify();
}

/* Testing the ZAF TSE Trigger function when there are more than 1 lifeline destination */
void test_ZAF_TSE_Trigger_more_than_one_lifeline_destination(void)
{
  mock_t * pMock;
  SSwTimer* zaf_tse_timer;
  void (*pCallback)(SSwTimer*);
  mock_calls_clear();
  init_tse(pMock, &zaf_tse_timer,&pCallback);

  /* Test return value if received via Multicast */
  mock_call_expect(TO_STR(is_multicast), &pMock);
  set_is_multicast_to_false(pMock);

  /* Trigger with identical RxOptions / lifeline destination */
  mock_call_expect(TO_STR(handleAssociationGetnodeList), &pMock);
  set_handleAssociationGetnodeList_default_mock(pMock);
  pList[0].node.nodeId = TestData.rxOptions.sourceNode.nodeId+1;
  pList[0].node.endpoint = TestData.rxOptions.sourceNode.endpoint;
  pList[1].node.nodeId = TestData.rxOptions.sourceNode.nodeId;
  pList[1].node.endpoint = TestData.rxOptions.sourceNode.endpoint+1;
  pList[2].node.nodeId = TestData.rxOptions.sourceNode.nodeId;
  pList[2].node.endpoint = TestData.rxOptions.sourceNode.endpoint;
  pList[3].node.nodeId = TestData.rxOptions.sourceNode.nodeId+1;
  pList[3].node.endpoint = TestData.rxOptions.sourceNode.endpoint+1;
  pMock->output_arg[2].p = pList;
  pMock->output_arg[3].v = 4;

  mock_call_expect(TO_STR(TimerIsActive), &pMock);
  pMock->expect_arg[0].p = zaf_tse_timer;
  pMock->return_code.v = false;

  mock_call_expect(TO_STR(TimerStart), &pMock);
  pMock->expect_arg[0].p = zaf_tse_timer;
  pMock->expect_arg[1].v = ZAF_TSE_DELAY_TRIGGER;

  TEST_ASSERT_TRUE_MESSAGE(ZAF_TSE_Trigger(callback_functions[0], &TestData, true), "Regular trigger was rejected");

  deinit_tse();

  mock_calls_verify();
}

/* Testing the ZAF TSE Trigger function when a different local endpoint calls the same callback */
void test_ZAF_TSE_Trigger_different_endpoint_triggers_same_callback(void)
{
  mock_t * pMock;
  SSwTimer* zaf_tse_timer;
  void (*pCallback)(SSwTimer*);
  mock_calls_clear();
  init_tse(pMock, &zaf_tse_timer,&pCallback);

  /* Test return value if received via Multicast */
  mock_call_expect(TO_STR(is_multicast), &pMock);
  set_is_multicast_to_false(pMock);

  /* Trigger with identical RxOptions / lifeline destination */
  mock_call_expect(TO_STR(handleAssociationGetnodeList), &pMock);
  set_handleAssociationGetnodeList_default_mock(pMock);
  pList[0].node.nodeId = TestData.rxOptions.sourceNode.nodeId+1;
  pList[0].node.endpoint = TestData.rxOptions.sourceNode.endpoint;
  pMock->output_arg[2].p = pList;
  pMock->output_arg[3].v = 1;

  mock_call_expect(TO_STR(TimerIsActive), &pMock);
  pMock->expect_arg[0].p = zaf_tse_timer;
  pMock->return_code.v = false;

  mock_call_expect(TO_STR(TimerStart), &pMock);
  pMock->expect_arg[0].p = zaf_tse_timer;
  pMock->expect_arg[1].v = ZAF_TSE_DELAY_TRIGGER;

  TEST_ASSERT_TRUE_MESSAGE(ZAF_TSE_Trigger(callback_functions[0], &TestData, true), "Regular trigger was rejected");

  /* Test return value if received via Multicast */
  mock_call_expect(TO_STR(is_multicast), &pMock);
  set_is_multicast_to_false(pMock);

  /* Trigger with identical RxOptions / lifeline destination */
  mock_call_expect(TO_STR(handleAssociationGetnodeList), &pMock);
  set_handleAssociationGetnodeList_default_mock(pMock);
  pList[0].node.nodeId = TestData.rxOptions.sourceNode.nodeId+1;
  pList[0].node.endpoint = TestData.rxOptions.sourceNode.endpoint;
  pMock->output_arg[2].p = pList;
  pMock->output_arg[3].v = 1;

  mock_call_expect(TO_STR(TimerIsActive), &pMock);
  pMock->expect_arg[0].p = zaf_tse_timer;
  pMock->return_code.v = true;

  /* Increment local endpoint and trigger again */
  TestData2.rxOptions.destNode.endpoint = TestData.rxOptions.destNode.endpoint+1;
  TestData2.rxOptions.destNode.nodeId = TestData.rxOptions.destNode.nodeId;
  TestData2.rxOptions.sourceNode.nodeId = TestData.rxOptions.sourceNode.nodeId;
  TestData2.rxOptions.sourceNode.endpoint = TestData.rxOptions.sourceNode.endpoint;
  TEST_ASSERT_TRUE_MESSAGE(ZAF_TSE_Trigger(callback_functions[0], &TestData2, true), "Regular trigger was rejected");

  deinit_tse();

  mock_calls_verify();
}


/* Testing the ZAF TSE Trigger function when the trigger is restarted */
void test_ZAF_TSE_Trigger_restart_trigger(void)
{
  mock_t * pMock;
  SSwTimer* zaf_tse_timer;
  void (*pCallback)(SSwTimer*);
  mock_calls_clear();
  init_tse(pMock, &zaf_tse_timer,&pCallback);

  /* Test return value if received via Multicast */
  mock_call_expect(TO_STR(is_multicast), &pMock);
  set_is_multicast_to_false(pMock);

  /* Trigger with identical RxOptions / lifeline destination */
  mock_call_expect(TO_STR(handleAssociationGetnodeList), &pMock);
  set_handleAssociationGetnodeList_default_mock(pMock);
  pList[0].node.nodeId = TestData.rxOptions.sourceNode.nodeId;
  pList[0].node.endpoint = TestData.rxOptions.sourceNode.endpoint+1;
  pMock->output_arg[2].p = pList;
  pMock->output_arg[3].v = 1;

  mock_call_expect(TO_STR(TimerIsActive), &pMock);
  pMock->expect_arg[0].p = zaf_tse_timer;
  pMock->return_code.v = false;

  mock_call_expect(TO_STR(TimerStart), &pMock);
  pMock->expect_arg[0].p = zaf_tse_timer;
  pMock->expect_arg[1].v = ZAF_TSE_DELAY_TRIGGER;

  // First call to assign the callback
  TEST_ASSERT_TRUE_MESSAGE(ZAF_TSE_Trigger(callback_functions[0], &TestData, true), "Trigger initiated by change from another node than Association Group ID was rejected");

  /* Test return value if received via Multicast */
  mock_call_expect(TO_STR(is_multicast), &pMock);
  set_is_multicast_to_false(pMock);

  /* Trigger with different node in RxOptions and in lifeline destination */
  mock_call_expect(TO_STR(handleAssociationGetnodeList), &pMock);
  set_handleAssociationGetnodeList_default_mock(pMock);
  pMock->output_arg[2].p = pList;
  pMock->output_arg[3].v = 1;

  mock_call_expect(TO_STR(TimerIsActive), &pMock);
  pMock->expect_arg[0].p = zaf_tse_timer; //Same timer is checked
  pMock->return_code.v = true;

  mock_call_expect(TO_STR(TimerRestart), &pMock);
  pMock->expect_arg[0].p = zaf_tse_timer;

  TEST_ASSERT_TRUE_MESSAGE(ZAF_TSE_Trigger(callback_functions[0], &TestData, true), "Restarting trigger was rejected");

  deinit_tse();

  mock_calls_verify();
}


/* Testing the ZAF TSE Trigger function when the trigger is not overwritten */
void test_ZAF_TSE_Trigger_identical_trigger_no_overwrite(void)
{
  mock_t * pMock;
  SSwTimer* zaf_tse_timer;
  void (*pCallback)(SSwTimer*);
  mock_calls_clear();
  init_tse(pMock, &zaf_tse_timer,&pCallback);

  /* Test return value if received via Multicast */
  mock_call_expect(TO_STR(is_multicast), &pMock);
  set_is_multicast_to_false(pMock);

  /* Trigger with identical RxOptions / lifeline destination */
  mock_call_expect(TO_STR(handleAssociationGetnodeList), &pMock);
  set_handleAssociationGetnodeList_default_mock(pMock);
  pList[0].node.nodeId = TestData.rxOptions.sourceNode.nodeId+1;
  pList[0].node.endpoint = TestData.rxOptions.sourceNode.endpoint;
  pMock->output_arg[2].p = pList;
  pMock->output_arg[3].v = 1;

  mock_call_expect(TO_STR(TimerIsActive), &pMock);
  pMock->expect_arg[0].p = zaf_tse_timer;
  pMock->return_code.v = false;

  mock_call_expect(TO_STR(TimerStart), &pMock);
  pMock->expect_arg[0].p = zaf_tse_timer;
  pMock->expect_arg[1].v = ZAF_TSE_DELAY_TRIGGER;

  // First call to assign the callback
  TEST_ASSERT_TRUE_MESSAGE(ZAF_TSE_Trigger(callback_functions[0], &TestData, true), "Trigger initiated by change from another node than Association Group ID was rejected");

  /* Test return value if received via Multicast */
  mock_call_expect(TO_STR(is_multicast), &pMock);
  set_is_multicast_to_false(pMock);

  /* Trigger with different node in RxOptions and in lifeline destination */
  mock_call_expect(TO_STR(handleAssociationGetnodeList), &pMock);
  set_handleAssociationGetnodeList_default_mock(pMock);
  pMock->output_arg[2].p = pList;
  pMock->output_arg[3].v = 1;

  TEST_ASSERT_TRUE_MESSAGE(ZAF_TSE_Trigger(callback_functions[0], &TestData, false), "New trigger with no overwrite was rejected");

  deinit_tse();

  mock_calls_verify();
}


/* Testing the ZAF TSE Trigger function when the maximum number of calls are already made  */
void test_ZAF_TSE_Trigger_maximum_triggers(void)
{
  mock_t * pMock;
  SSwTimer* zaf_tse_timer;
  void (*pCallback)(SSwTimer*);
  mock_calls_clear();
  init_tse(pMock, &zaf_tse_timer,&pCallback);

  for (uint8_t i=0;i<ZAF_TSE_MAXIMUM_SIMULTANEOUS_TRIGGERS; i++)
  {
    /* Test return value if received via Multicast */
    mock_call_expect(TO_STR(is_multicast), &pMock);
    set_is_multicast_to_false(pMock);

    /* Trigger with different node in RxOptions and in lifeline destination */
    mock_call_expect(TO_STR(handleAssociationGetnodeList), &pMock);
    set_handleAssociationGetnodeList_default_mock(pMock);
    pList[0].node.nodeId = TestData.rxOptions.sourceNode.nodeId+1;
    pList[0].node.endpoint = TestData.rxOptions.sourceNode.endpoint+1;
    pMock->output_arg[2].p = pList;
    pMock->output_arg[3].v = 1;

    mock_call_expect(TO_STR(TimerIsActive), &pMock);
    pMock->expect_arg[0].p = zaf_tse_timer;
    pMock->return_code.v = true; // here it does not really matter

    if (0 == i)
    {
      // Timer will be started first time only because we're already transmitting.
      mock_call_expect(TO_STR(TimerStart), &pMock);
      pMock->expect_arg[0].p = zaf_tse_timer;
      pMock->expect_arg[1].v = ZAF_TSE_DELAY_TRIGGER;
    }

    // Increment the callback function address every time to fill up the capacity
    TEST_ASSERT_TRUE_MESSAGE(ZAF_TSE_Trigger(callback_functions[i], &TestData, true), "Regular trigger was not accepted");
  }

  /* Test return value if received via Multicast */
  mock_call_expect(TO_STR(is_multicast), &pMock);
  set_is_multicast_to_false(pMock);

  /* Trigger with different node in RxOptions and in lifeline destination */
  mock_call_expect(TO_STR(handleAssociationGetnodeList), &pMock);
  set_handleAssociationGetnodeList_default_mock(pMock);
  pMock->output_arg[2].p = pList;
  pMock->output_arg[3].v = 1;

  mock_call_expect(TO_STR(TimerIsActive), &pMock);
  pMock->expect_arg[0].p = zaf_tse_timer;
  pMock->return_code.v = true;

  TEST_ASSERT_FALSE_MESSAGE(ZAF_TSE_Trigger(callback_functions[ZAF_TSE_MAXIMUM_SIMULTANEOUS_TRIGGERS],
   &TestData, true), "Trigger accepted despite maximum number of timers active");

  deinit_tse();

  mock_calls_verify();
}


/* Testing the ZAF_TSE_TimerCallback function with unknown SwTimer  */
void test_ZAF_TSE_TimerCallback_unknown_sw_timer(void)
{
  mock_t * pMock;
  SSwTimer* zaf_tse_timer;
  void (*pCallback)(SSwTimer*);
  mock_calls_clear();
  init_tse(pMock, &zaf_tse_timer,&pCallback);

  /* Test return value if received via Multicast */
  mock_call_expect(TO_STR(is_multicast), &pMock);
  set_is_multicast_to_false(pMock);

  /* Trigger with different node in RxOptions and in lifeline destination */
  mock_call_expect(TO_STR(handleAssociationGetnodeList), &pMock);
  set_handleAssociationGetnodeList_default_mock(pMock);
  pList[0].node.nodeId = TestData.rxOptions.sourceNode.nodeId;
  pList[0].node.endpoint = TestData.rxOptions.sourceNode.endpoint+1;
  pMock->output_arg[2].p = pList;
  pMock->output_arg[3].v = 1;

  mock_call_expect(TO_STR(TimerIsActive), &pMock);
  pMock->compare_rule_arg[0] = COMPARE_NOT_NULL;
  pMock->return_code.v = false;

  mock_call_expect(TO_STR(TimerStart), &pMock);
  pMock->expect_arg[0].p = zaf_tse_timer;
  pMock->expect_arg[1].v = ZAF_TSE_DELAY_TRIGGER;

  TEST_ASSERT_TRUE_MESSAGE(ZAF_TSE_Trigger(TestCallback, &TestData, true), "Trigger initiated by change from another node than Association Group ID was rejected");

  /* Call manually the callback function with unknown timer */
  TestCallbackCounter = 0;
  pCallback(zaf_tse_timer+1);
  TEST_ASSERT_FALSE_MESSAGE(1==TestCallbackCounter, "TestCallback was wrongfully called back by the TSE trigger");

  deinit_tse();

  mock_calls_verify();
}

static destination_info_t *Mock_handleAssociationGetnodeList(mock_t * pMock, uint8_t nodeCount)
{
  mock_call_expect(TO_STR(handleAssociationGetnodeList), &pMock);
  destination_info_t * pNodelist = malloc(sizeof(destination_info_t) * nodeCount);
  memset((uint8_t *)pNodelist, 0, nodeCount * sizeof(destination_info_t));
  for (uint32_t i = 0; i < nodeCount; i++)
  {
    (pNodelist + i)->node.nodeId = i + 1;
  }
  uint8_t nodeListLength = nodeCount;
  pMock->compare_rule_arg[0] = COMPARE_ANY;
  pMock->compare_rule_arg[1] = COMPARE_ANY;
  pMock->compare_rule_arg[2] = COMPARE_ANY;
  pMock->compare_rule_arg[3] = COMPARE_ANY;
  pMock->output_arg[2].p = pNodelist;
  pMock->output_arg[3].v = nodeListLength;

  return pNodelist;
}

static uint32_t cb_wait_for_tx_callback_count;
static uint32_t expected_destination_node_id;

void cb_ZAF_TSE_TimerCallback_normal_case_low_power_rx_options(zaf_tx_options_t *tx_options, void* pData)
{
  cb_wait_for_tx_callback_count++;
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(RECEIVE_STATUS_LOW_POWER, tx_options->tx_options & RECEIVE_STATUS_LOW_POWER, "Tx options is wrong :(");
}

/* Testing the ZAF_TSE_TimerCallback normal case with low power rx Option */
void test_ZAF_TSE_TimerCallback_normal_case_low_power_rx_options(void)
{
  mock_t * pMock = NULL;
  SSwTimer* zaf_tse_timer;
  void (*pTimerCallback)(SSwTimer*);
  mock_calls_clear();

  const uint8_t NODE_COUNT = 1;

  my_personalized_struct_t CCData;
  memset((uint8_t *)&CCData, 0, sizeof(my_personalized_struct_t));

  CCData.rxOptions.rxStatus = RECEIVE_STATUS_LOW_POWER;

  init_tse(pMock, &zaf_tse_timer, &pTimerCallback);

  mock_call_use_as_stub(TO_STR(is_multicast));
  mock_call_use_as_stub(TO_STR(TimerIsActive));
  mock_call_use_as_stub(TO_STR(TimerStart));

  destination_info_t *pNodelist = Mock_handleAssociationGetnodeList(pMock, NODE_COUNT);

  cb_wait_for_tx_callback_count = 0;

  ZAF_TSE_Trigger(cb_ZAF_TSE_TimerCallback_normal_case_low_power_rx_options, &CCData, true);

  // Trigger the 1. invocation of cb_wait_for_tx_callback
  expected_destination_node_id = 1;
  pTimerCallback(zaf_tse_timer);
  TEST_ASSERT_EQUAL_UINT32_MESSAGE(1, cb_wait_for_tx_callback_count, "Callback count (1) did not match :(");

  free(pNodelist);

  deinit_tse();

  mock_calls_verify();
}

void cb_wait_for_tx_callback(zaf_tx_options_t *tx_options, __attribute__((unused)) void* pData)
{
  cb_wait_for_tx_callback_count++;
  char str[100];
  sprintf(str, "Node ID did not match :( EN%u, AN%u, CB%u", expected_destination_node_id, tx_options->dest_node_id, cb_wait_for_tx_callback_count);
  TEST_ASSERT_EQUAL_UINT32_MESSAGE(expected_destination_node_id, tx_options->dest_node_id, str);
}

/*
 * Verifies that only one frame is sent at a time.
 *
 * Node ID 1 sends something to "this" node and with 5 nodes, including node ID 1, in Lifeline,
 * the following scenario with 4 destinations is verified.
 *
 * Triggering TSE twice is important to show that the session cleans up and is ready to be
 * triggered again.
 * 1.  Init TSE
 * 2.  Trigger TSE
 * 3.  Invoke timer callback => TX of 1. frame to node ID 2
 * 4.  Invoke tx callback    => TX of 2. frame to node ID 3
 * 5.  Invoke tx callback    => TX of 3. frame to node ID 4
 * 5.  Invoke tx callback    => TX of 4. frame to node ID 5
 * 6.  Invoke tx callback    => Do nothing
 * 7.  Trigger TSE
 * 8.  Invoke timer callback => TX of 1. frame to node ID 2
 * etc.
 */
void test_ZAF_TSE_wait_for_tx_callback_one_trigger(void)
{
  mock_t * pMock = NULL;
  SSwTimer* zaf_tse_timer;
  void (*pTimerCallback)(SSwTimer*);
  mock_calls_clear();

  const uint8_t NODE_COUNT = 5;

  my_personalized_struct_t CCData;
  memset((uint8_t *)&CCData, 0, sizeof(my_personalized_struct_t));

  // TODO: If setting the source node to X (higher than zero), node X must not receive a frame.
  CCData.rxOptions.sourceNode.nodeId = 0;

  init_tse(pMock, &zaf_tse_timer, &pTimerCallback);

  mock_call_use_as_stub(TO_STR(is_multicast));
  mock_call_use_as_stub(TO_STR(TimerIsActive));
  mock_call_use_as_stub(TO_STR(TimerStart));

  destination_info_t *pNodelist_1 = Mock_handleAssociationGetnodeList(pMock, NODE_COUNT);

  cb_wait_for_tx_callback_count = 0;

  ZAF_TSE_Trigger(cb_wait_for_tx_callback, &CCData, true);

  // Trigger the 1. invocation of cb_wait_for_tx_callback
  expected_destination_node_id = 1;
  pTimerCallback(zaf_tse_timer);
  TEST_ASSERT_EQUAL_UINT32_MESSAGE(1, cb_wait_for_tx_callback_count, "Callback count (1) did not match :(");

  for (uint32_t i = 2; i <= NODE_COUNT; i++)
  {
    // Trigger the i-th invocation of cb_wait_for_tx_callback
    expected_destination_node_id = i;
    ZAF_TSE_TXCallback(NULL);
    char str[100];
    sprintf(str, "Callback count (%u) did not match :(", i);
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(i, cb_wait_for_tx_callback_count, str);
  }

  // Trigger callback from last transmission.
  ZAF_TSE_TXCallback(NULL);
  /*
   * Expect the number of callback counts to be the same because the last TX callback will not
   * trigger a new transmission.
   */
  TEST_ASSERT_EQUAL_UINT32_MESSAGE(5, cb_wait_for_tx_callback_count, "Callback count unexpectedly increased :(");

  /*********************************************************************************************
   * At this point we consider the True Status session to be done. Now let's trigger the same one
   * again to verify that it works.
   *********************************************************************************************
   */

  destination_info_t *pNodelist_2 = Mock_handleAssociationGetnodeList(pMock, NODE_COUNT);

  cb_wait_for_tx_callback_count = 0;

  ZAF_TSE_Trigger(cb_wait_for_tx_callback, &CCData, true);

  // Trigger the 1. invocation of cb_wait_for_tx_callback
  expected_destination_node_id = 1;
  pTimerCallback(zaf_tse_timer);
  TEST_ASSERT_EQUAL_UINT32_MESSAGE(1, cb_wait_for_tx_callback_count, "Callback count did not match :(");

  free(pNodelist_1);
  free(pNodelist_2);
  deinit_tse();

  mock_calls_verify();}

/**
 * Verifies that 1 frame will be transmitted at a time even though 3 triggers were triggered.
 *
 * With 3 nodes in Lifeline and source nodes for each trigger being 1, 2 and 3, respectively,
 * We must expect 6 frames transmitted in total:
 * 1. trigger transmits frames to 2 and 3
 * 2. trigger transmits frames to 1 and 3
 * 3. trigger transmits frames to 1 and 2
 */
void test_ZAF_TSE_max_number_of_triggers(void)
{
  destination_info_t *pNodelists[ZAF_TSE_MAXIMUM_SIMULTANEOUS_TRIGGERS];
  mock_t * pMock = NULL;
  SSwTimer* zaf_tse_timer;
  void (*pTimerCallback)(SSwTimer*);
  mock_calls_clear();

  // Number of nodes associated in Lifeline.
  const uint8_t NODE_COUNT = 3;

  my_personalized_struct_t * pCCData = calloc(ZAF_TSE_MAXIMUM_SIMULTANEOUS_TRIGGERS, sizeof(my_personalized_struct_t));

  /*
   * Make sure that each of the triggers are "received" from a unique node so that we can
   * distinguish them in the callback.
   */
  for (uint32_t i = 0; i < ZAF_TSE_MAXIMUM_SIMULTANEOUS_TRIGGERS; i++)
  {
    (pCCData + i)->rxOptions.sourceNode.nodeId = i + 1;
  }

  init_tse(pMock, &zaf_tse_timer, &pTimerCallback);

  mock_call_use_as_stub(TO_STR(is_multicast));
  mock_call_use_as_stub(TO_STR(TimerIsActive));
  mock_call_use_as_stub(TO_STR(TimerStart));

  cb_wait_for_tx_callback_count = 0;

  for (uint32_t i = 0; i < ZAF_TSE_MAXIMUM_SIMULTANEOUS_TRIGGERS; i++)
  {
    pNodelists[i] = Mock_handleAssociationGetnodeList(pMock, NODE_COUNT);
    ZAF_TSE_Trigger(cb_wait_for_tx_callback, pCCData + i, false);
  }

  /*
   * There is only one timer and this will trigger once starting transmission of the nodes in the
   * 1. trigger.
   */
  expected_destination_node_id = 2;
  pTimerCallback(zaf_tse_timer);
  TEST_ASSERT_EQUAL_UINT32_MESSAGE(1, cb_wait_for_tx_callback_count, "Callback count did not match :(");

  expected_destination_node_id = 3;
  ZAF_TSE_TXCallback(NULL);
  TEST_ASSERT_EQUAL_UINT32_MESSAGE(2, cb_wait_for_tx_callback_count, "Callback count did not match :(");

  // Trigger 2
  expected_destination_node_id = 1;
  ZAF_TSE_TXCallback(NULL);
  TEST_ASSERT_EQUAL_UINT32_MESSAGE(3, cb_wait_for_tx_callback_count, "Callback count did not match :(");

  expected_destination_node_id = 3;
  ZAF_TSE_TXCallback(NULL);
  TEST_ASSERT_EQUAL_UINT32_MESSAGE(4, cb_wait_for_tx_callback_count, "Callback count did not match :(");

  // Trigger 3
  expected_destination_node_id = 1;
  ZAF_TSE_TXCallback(NULL);
  TEST_ASSERT_EQUAL_UINT32_MESSAGE(5, cb_wait_for_tx_callback_count, "Callback count did not match :(");

  expected_destination_node_id = 2;
  ZAF_TSE_TXCallback(NULL);
  TEST_ASSERT_EQUAL_UINT32_MESSAGE(6, cb_wait_for_tx_callback_count, "Callback count did not match :(");

  // Trigger callback from last transmission.
  ZAF_TSE_TXCallback(NULL);
  /*
   * Expect the number of callback counts to be the same because the last TX callback will not
   * trigger a new transmission.
   */
  TEST_ASSERT_EQUAL_UINT32_MESSAGE(6, cb_wait_for_tx_callback_count, "Callback count unexpectedly increased :(");

  for (uint32_t i = 0; i < ZAF_TSE_MAXIMUM_SIMULTANEOUS_TRIGGERS; i++)
  {
    free(pNodelists[i]);
  }
  free(pCCData);
  deinit_tse();

  mock_calls_verify();
}

/**
 * Verifies that while transmitting frames, the frames of a new trigger, will be transmitted after
 * the first trigger is done.
 *
 * With 3 nodes in Lifeline, the following applies:
 * 1. Init
 * 2. Trigger number 1.
 * 3. Invoke timer callback that transmits 1. frame (from trigger 1)
 * 4. Trigger number 2.
 * 5. Invoke TX callback that transmits 2. frame (from trigger 1)
 * 6. Trigger number 3.
 * 7. Invoke TX callback that transmits 1. frame (from trigger 2)
 * 8. Invoke TX callback that transmits 2. frame (from trigger 2)
 * 9. Invoke TX callback that transmits 1. frame (from trigger 3)
 * 10. Invoke TX callback that transmits 2. frame (from trigger 3)
 * 11. Invoke TX callback that does nothing.
 */
void test_ZAF_TSE_trigger_while_transmitting(void)
{
  mock_t * pMock;
  SSwTimer* zaf_tse_timer;
  void (*pTimerCallback)(SSwTimer*);
  mock_calls_clear();

  // Number of nodes associated in Lifeline.
  const uint8_t NODE_COUNT = 3;

  my_personalized_struct_t * pCCData = calloc(ZAF_TSE_MAXIMUM_SIMULTANEOUS_TRIGGERS, sizeof(my_personalized_struct_t));

  /*
   * Make sure that each of the triggers are "received" from a unique node so that we can
   * distinguish them in the callback.
   */
  for (uint32_t i = 0; i < ZAF_TSE_MAXIMUM_SIMULTANEOUS_TRIGGERS; i++)
  {
    (pCCData + i)->rxOptions.sourceNode.nodeId = i + 1;
  }

  init_tse(pMock, &zaf_tse_timer, &pTimerCallback);

  mock_call_use_as_stub(TO_STR(is_multicast));
  //mock_call_use_as_stub(TO_STR(TimerIsActive));
  //mock_call_use_as_stub(TO_STR(TimerStart));

  cb_wait_for_tx_callback_count = 0;

  destination_info_t *pNodelist_1 = Mock_handleAssociationGetnodeList(pMock, NODE_COUNT);
  mock_call_expect(TO_STR(TimerStart), &pMock);
  pMock->compare_rule_arg[0] = COMPARE_ANY;
  pMock->compare_rule_arg[1] = COMPARE_ANY;
  ZAF_TSE_Trigger(cb_wait_for_tx_callback, pCCData, false);

  /*
   * There is only one timer and this will trigger once starting transmission of the nodes in the
   * 1. trigger.
   */
  expected_destination_node_id = 2;
  pTimerCallback(zaf_tse_timer);
  TEST_ASSERT_EQUAL_UINT32_MESSAGE(1, cb_wait_for_tx_callback_count, "Callback count did not match :(");

  destination_info_t *pNodelist_2 = Mock_handleAssociationGetnodeList(pMock, NODE_COUNT);
  ZAF_TSE_Trigger(cb_wait_for_tx_callback, pCCData + 1, false);

  expected_destination_node_id = 3;
  ZAF_TSE_TXCallback(NULL);
  TEST_ASSERT_EQUAL_UINT32_MESSAGE(2, cb_wait_for_tx_callback_count, "Callback count did not match :(");

  destination_info_t *pNodelist_3 = Mock_handleAssociationGetnodeList(pMock, NODE_COUNT);
  ZAF_TSE_Trigger(cb_wait_for_tx_callback, pCCData + 2, false);

  // Trigger 2
  expected_destination_node_id = 1;
  ZAF_TSE_TXCallback(NULL);
  TEST_ASSERT_EQUAL_UINT32_MESSAGE(3, cb_wait_for_tx_callback_count, "Callback count did not match :(");

  expected_destination_node_id = 3;
  ZAF_TSE_TXCallback(NULL);
  TEST_ASSERT_EQUAL_UINT32_MESSAGE(4, cb_wait_for_tx_callback_count, "Callback count did not match :(");

  // Trigger 3
  expected_destination_node_id = 1;
  ZAF_TSE_TXCallback(NULL);
  TEST_ASSERT_EQUAL_UINT32_MESSAGE(5, cb_wait_for_tx_callback_count, "Callback count did not match :(");

  expected_destination_node_id = 2;
  ZAF_TSE_TXCallback(NULL);
  TEST_ASSERT_EQUAL_UINT32_MESSAGE(6, cb_wait_for_tx_callback_count, "Callback count did not match :(");

  // Trigger callback from last transmission.
  ZAF_TSE_TXCallback(NULL);
  /*
   * Expect the number of callback counts to be the same because the last TX callback will not
   * trigger a new transmission.
   */
  TEST_ASSERT_EQUAL_UINT32_MESSAGE(6, cb_wait_for_tx_callback_count, "Callback count unexpectedly increased :(");

  free(pNodelist_1);
  free(pNodelist_2);
  free(pNodelist_3);
  free(pCCData);
  deinit_tse();

  mock_calls_verify();
}

/**
 * Verify that TSE can recover from triggering with overwrite while transmitting the same
 * trigger.
 */
void test_ZAF_TSE_trigger_while_transmitting_overwrite(void)
{
  mock_t * pMock;
  SSwTimer* zaf_tse_timer;
  void (*pTimerCallback)(SSwTimer*);
  mock_calls_clear();

  // Number of nodes associated in Lifeline.
  const uint8_t NODE_COUNT = 3;

  my_personalized_struct_t * pCCData = calloc(ZAF_TSE_MAXIMUM_SIMULTANEOUS_TRIGGERS, sizeof(my_personalized_struct_t));

  /*
   * Make sure that each of the triggers are "received" from a unique node so that we can
   * distinguish them in the callback.
   */
  for (uint32_t i = 0; i < ZAF_TSE_MAXIMUM_SIMULTANEOUS_TRIGGERS; i++)
  {
    (pCCData + i)->rxOptions.sourceNode.nodeId = i + 1;
  }

  init_tse(pMock, &zaf_tse_timer, &pTimerCallback);

  mock_call_use_as_stub(TO_STR(is_multicast));

  cb_wait_for_tx_callback_count = 0;

  destination_info_t *pNodelist_1 = Mock_handleAssociationGetnodeList(pMock, NODE_COUNT);

  mock_call_expect(TO_STR(TimerIsActive), &pMock);
  pMock->compare_rule_arg[0] = COMPARE_ANY;
  pMock->return_code.v = false;

  mock_call_expect(TO_STR(TimerStart), &pMock);
  pMock->compare_rule_arg[0] = COMPARE_ANY;
  pMock->compare_rule_arg[1] = COMPARE_ANY;
  ZAF_TSE_Trigger(cb_wait_for_tx_callback, pCCData, true);

  /*
   * There is only one timer and this will trigger once starting transmission of the nodes in the
   * 1. trigger.
   */
  expected_destination_node_id = 2;
  pTimerCallback(zaf_tse_timer);
  TEST_ASSERT_EQUAL_UINT32_MESSAGE(1, cb_wait_for_tx_callback_count, "Callback count did not match :(");

  // Trigger with overwrite while transmitting.
  destination_info_t *pNodelist_2 = Mock_handleAssociationGetnodeList(pMock, NODE_COUNT);
  mock_call_expect(TO_STR(TimerIsActive), &pMock);
  pMock->compare_rule_arg[0] = COMPARE_ANY;
  pMock->return_code.v = false;
  ZAF_TSE_Trigger(cb_wait_for_tx_callback, pCCData, true);

  expected_destination_node_id = 3;
  ZAF_TSE_TXCallback(NULL);
  TEST_ASSERT_EQUAL_UINT32_MESSAGE(2, cb_wait_for_tx_callback_count, "Callback count did not match :(");

  expected_destination_node_id = 2;
  ZAF_TSE_TXCallback(NULL);
  TEST_ASSERT_EQUAL_UINT32_MESSAGE(3, cb_wait_for_tx_callback_count, "Callback count did not match :(");

  expected_destination_node_id = 3;
  ZAF_TSE_TXCallback(NULL);
  TEST_ASSERT_EQUAL_UINT32_MESSAGE(4, cb_wait_for_tx_callback_count, "Callback count did not match :(");

  // Trigger callback from last transmission.
  ZAF_TSE_TXCallback(NULL);
  /*
   * Expect the number of callback counts to be the same because the last TX callback will not
   * trigger a new transmission.
   */
  TEST_ASSERT_EQUAL_UINT32_MESSAGE(4, cb_wait_for_tx_callback_count, "Callback count unexpectedly increased :(");

  free(pNodelist_1);
  free(pNodelist_2);
  free(pCCData);
  deinit_tse();

  mock_calls_verify();
}
