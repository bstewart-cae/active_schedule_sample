// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

#include <mock_control.h>
#include <ZAF_CmdPublisher.h>
#include <ZW_application_transport_interface.h>

void setUpSuite(void) {

}

void tearDownSuite(void) {

}

//#define DEBUGPRINT
#include "DebugPrint.h"
#define  SUBSCRIBERS_NUM_3 3
#define  SUBSCRIBERS_NUM_5 5
// Used to verify that pSubscriberContext argument is successfully passed to callback function
static void *psContext = "AA"; // non-null, don't care

// varibles for testing cb functions used for different types of subscription
static int subscribeAll=0, subscribeCC=0, subscribeCmd=0;

/* General purpose callback function */
static void dummyCb(void *pSubscriberContext, SZwaveReceivePackage* pRxPackage)
{
  DPRINT("$$$ dummyCb called.\n");
}

static void dummyCb_All(void *pSubscriberContext, SZwaveReceivePackage* pRxPackage)
{
  TEST_ASSERT_TRUE_MESSAGE(psContext == pSubscriberContext,
                           "Callback function: Unexpected subscribers context!");
  subscribeAll++;
  DPRINTF("$$$ dummyCb_All called %d times\n", subscribeAll);
}

static void dummyCb_CC(void *pSubscriberContext, SZwaveReceivePackage* pRxPackage)
{
  TEST_ASSERT_TRUE_MESSAGE(psContext == pSubscriberContext,
                           "Callback function: Unexpected subscribers context!");
  TEST_ASSERT_TRUE_MESSAGE(0 != pRxPackage->uReceiveParams.Rx.Payload.rxBuffer.ZW_Common.cmdClass,
                           "dummyCb_CC: Invalid value of received package!");
  subscribeCC++;
  DPRINTF("$$$ dummyCb_CC called %d times\n", subscribeCC);
}

static void dummyCb_Cmd(void *pSubscriberContext, SZwaveReceivePackage* pRxPackage)
{
  TEST_ASSERT_TRUE_MESSAGE(psContext == pSubscriberContext,
                           "Callback function: Unexpected subscribers context!");
  TEST_ASSERT_TRUE_MESSAGE((0 != pRxPackage->uReceiveParams.Rx.Payload.rxBuffer.ZW_Common.cmd) &&
                           (0 != pRxPackage->uReceiveParams.Rx.Payload.rxBuffer.ZW_Common.cmdClass),
                           "dummyCb_Cmd: Invalid value of received package!");
  subscribeCmd++;
  DPRINTF("$$$ dummyCb_Cmd called %d times\n", subscribeCmd);
}


void test_ZAF_CP_Init(void)
{
  const uint8_t numSubscribers = SUBSCRIBERS_NUM_5;
  CP_Handle_t handle;

  ZAF_CP_STORAGE(content, SUBSCRIBERS_NUM_5);

  handle = ZAF_CP_Init((void*) &content, numSubscribers);


  TEST_ASSERT_TRUE_MESSAGE(ZAF_CP_SubscribeToAll(handle, NULL, (zaf_cp_subscriberFunction_t)dummyCb),
                           "Subscription failed");
  TEST_ASSERT_TRUE_MESSAGE(ZAF_CP_SubscribeToCC(handle, NULL, (zaf_cp_subscriberFunction_t)dummyCb,
                                                COMMAND_CLASS_SWITCH_BINARY_V2),
                           "Subscription failed");
  TEST_ASSERT_TRUE_MESSAGE(ZAF_CP_SubscribeToCmd(handle, NULL, (zaf_cp_subscriberFunction_t)dummyCb,
                                                 COMMAND_CLASS_SWITCH_BINARY_V2, SWITCH_BINARY_GET),
                           "Subscription failed");
}

void test_SubscribeTo_fail_duplicateSubscriber(void)
{
  /* Try to add the same subscriber twice */
  const uint8_t numSubscribers = SUBSCRIBERS_NUM_3;
  CP_Handle_t handle;

  ZAF_CP_STORAGE(content, SUBSCRIBERS_NUM_3);
  handle = ZAF_CP_Init((void*) &content, numSubscribers);

  TEST_ASSERT_TRUE_MESSAGE(ZAF_CP_SubscribeToAll(handle, NULL, (zaf_cp_subscriberFunction_t)dummyCb),
                           "Subscription failed");

  // Try to add the same Entry.
  TEST_ASSERT_FALSE_MESSAGE(ZAF_CP_SubscribeToAll(handle, NULL, (zaf_cp_subscriberFunction_t)dummyCb),
                            "Duplicated subscription!");
  // Unsubscribe existing item
  TEST_ASSERT_TRUE_MESSAGE(ZAF_CP_UnsubscribeToAll(handle, NULL, (zaf_cp_subscriberFunction_t)dummyCb),
                           "Unsubscription failed");
  // Try to add the same entry again. Now it should pass
  TEST_ASSERT_TRUE_MESSAGE(ZAF_CP_SubscribeToAll(handle, NULL, (zaf_cp_subscriberFunction_t)dummyCb),
                           "Repeated subscription failed");
}

void test_SubscribeTo_fail_exceedMaxAllocated(void)
{
  /* Try to add more subscribers than allocated */
  const uint8_t numSubscribers = SUBSCRIBERS_NUM_3;
  CP_Handle_t handle;

  ZAF_CP_STORAGE(content, SUBSCRIBERS_NUM_3);
  handle = ZAF_CP_Init((void*) &content, numSubscribers);

  for (int i = 0; i < numSubscribers; i++)
  {
    TEST_ASSERT_TRUE_MESSAGE(ZAF_CP_SubscribeToCC(handle, NULL, (zaf_cp_subscriberFunction_t)dummyCb, i+1),
                             "New subscription failed");
  }

  TEST_ASSERT_FALSE_MESSAGE(ZAF_CP_SubscribeToCC(handle, NULL, (zaf_cp_subscriberFunction_t)dummyCb,
                                                 numSubscribers + 1),
                            "Unexpected subscription to the fully occupied array");
}

void test_SubscribeTo_fail_invalidSubscriber(void)
{
  /* Try to add invalid subscriber (type and CmdClass/Cmd member do not match) */
  const uint8_t numSubscribers = SUBSCRIBERS_NUM_3;
  CP_Handle_t handle;

  ZAF_CP_STORAGE(content, SUBSCRIBERS_NUM_3);
  handle = ZAF_CP_Init((void*) &content, numSubscribers);

  TEST_ASSERT_FALSE_MESSAGE(ZAF_CP_SubscribeToCmd(handle, NULL, (zaf_cp_subscriberFunction_t)dummyCb, 0, 1),
                            "Invalid subscription, CC value cannot be null!");
  TEST_ASSERT_FALSE_MESSAGE(ZAF_CP_SubscribeToCmd(handle, NULL, (zaf_cp_subscriberFunction_t)dummyCb, 1, 0),
                            "Invalid subscription, Cmd value cannot be null!");
  TEST_ASSERT_FALSE_MESSAGE(ZAF_CP_SubscribeToCC(handle, NULL, (zaf_cp_subscriberFunction_t)dummyCb, 0),
                            "Invalid subscription, CC value cannot be null!");
  TEST_ASSERT_FALSE_MESSAGE(ZAF_CP_SubscribeToAll(handle, NULL, NULL),
                             "Subscriber with NULL callback function shouldn't be added!");
}

void test_SubscribeTo_multipleSubscribers(void)
{
  /* Test different combination:
   * 1. the same CC, but different Cmd
   * 2. The same Cmd, but different pFunction and/or Context
   */
  const uint8_t numSubscribers = SUBSCRIBERS_NUM_5;
  CP_Handle_t handle;

  ZAF_CP_STORAGE(content, SUBSCRIBERS_NUM_5);
  handle = ZAF_CP_Init((void*)&content, numSubscribers);

  for (int i = 0; i < numSubscribers/2; i++)
  { // Subscribe to random commands for the same random class
    TEST_ASSERT_TRUE_MESSAGE(ZAF_CP_SubscribeToCmd(handle, NULL, (zaf_cp_subscriberFunction_t)dummyCb,
                                                   COMMAND_CLASS_SWITCH_BINARY_V2, i+1),
                             "New subscription failed");
  }

  TEST_ASSERT_TRUE_MESSAGE(ZAF_CP_SubscribeToCmd(handle, psContext, (zaf_cp_subscriberFunction_t)dummyCb_All,
                                                 COMMAND_CLASS_SWITCH_BINARY_V2, numSubscribers/2),
                           "New subscription failed");

  TEST_ASSERT_FALSE_MESSAGE(ZAF_CP_SubscribeToCmd(handle, psContext, (zaf_cp_subscriberFunction_t)dummyCb_All,
                                                  COMMAND_CLASS_SWITCH_BINARY_V2, numSubscribers/2),
                            "Added already existing subscription");

  TEST_ASSERT_TRUE_MESSAGE(ZAF_CP_SubscribeToCmd(handle, psContext, (zaf_cp_subscriberFunction_t)dummyCb_Cmd,
                                                 COMMAND_CLASS_SWITCH_BINARY_V2, numSubscribers/2),
                           "New subscription failed");
}

void test_Unsubscribe(void)
{
  /*
   * 1. Unsubscribe non existing item.
   * 2. Subscribe, then unsubscribe. (Additionaly: Make sure that old entry is not found anymore)
   * 3. After unsubsription, make sure that all other entries are still there.
   *    (number of non-default items was N, and after removing one, is N-1)
   */

  const uint8_t numSubscribers = SUBSCRIBERS_NUM_3;
  CP_Handle_t handle;

  ZAF_CP_STORAGE(content, SUBSCRIBERS_NUM_3);
  handle = ZAF_CP_Init((void*) &content, numSubscribers);

  TEST_ASSERT_FALSE_MESSAGE(ZAF_CP_UnsubscribeToAll(handle, NULL, (zaf_cp_subscriberFunction_t)dummyCb),
                            "Unsubscribed non existing item!");

  TEST_ASSERT_TRUE_MESSAGE(ZAF_CP_SubscribeToAll(handle, NULL, (zaf_cp_subscriberFunction_t)dummyCb),
                           "Subscription failed");

  // Unsubscribe existing item
  TEST_ASSERT_TRUE_MESSAGE(ZAF_CP_UnsubscribeToAll(handle, NULL, (zaf_cp_subscriberFunction_t)dummyCb),
                           "Unsubscription failed");
  TEST_ASSERT_FALSE_MESSAGE(ZAF_CP_UnsubscribeToAll(handle, NULL, (zaf_cp_subscriberFunction_t)dummyCb),
                            "Unsubscribed the same item twice!");

  for (int i = 0; i < numSubscribers; i++)
  {
    TEST_ASSERT_TRUE_MESSAGE(ZAF_CP_SubscribeToCC(handle, NULL, (zaf_cp_subscriberFunction_t)dummyCb, i+1),
                             "New subscription failed");
  }

  // Unsubscribe 'random' item (numSubscribers/2)
  TEST_ASSERT_TRUE_MESSAGE(ZAF_CP_UnsubscribeToCC(handle, NULL, (zaf_cp_subscriberFunction_t)dummyCb, numSubscribers/2),
                           "Unsubscription failed");
}

void test_PublishCmd(void)
{
  /*
   * 1. Correct item can be found
   * 2. Cb function is called expected number of times
   */
  const uint8_t numSubscribers = SUBSCRIBERS_NUM_5;
  CP_Handle_t handle;
  SZwaveReceivePackage pRxPackage;

  // reset number of callse
  subscribeAll = subscribeCC = subscribeCmd = 0;

  ZAF_CP_STORAGE(content, SUBSCRIBERS_NUM_5);
  handle = ZAF_CP_Init((void*) &content, numSubscribers);

  TEST_ASSERT_TRUE_MESSAGE(ZAF_CP_SubscribeToAll(handle, psContext, (zaf_cp_subscriberFunction_t)dummyCb_All),
                           "New subscription failed");
  TEST_ASSERT_TRUE_MESSAGE(ZAF_CP_SubscribeToCC(handle, psContext, (zaf_cp_subscriberFunction_t)dummyCb_CC, 0x01),
                           "New subscription failed");
  TEST_ASSERT_TRUE_MESSAGE(
      ZAF_CP_SubscribeToCmd(handle, psContext, (zaf_cp_subscriberFunction_t )dummyCb_Cmd, 0x02, 0x01),
      "New subscription failed");

  /* Test All */
  pRxPackage.uReceiveParams.Rx.Payload.rxBuffer.ZW_Common.cmdClass = 0x00;
  pRxPackage.uReceiveParams.Rx.Payload.rxBuffer.ZW_Common.cmd = 0x00;
  ZAF_CP_CommandPublish(handle, (void *) &pRxPackage); //subscribeAll++

  /* Test CC */
  pRxPackage.uReceiveParams.Rx.Payload.rxBuffer.ZW_Common.cmdClass = 0x01;
  pRxPackage.uReceiveParams.Rx.Payload.rxBuffer.ZW_Common.cmd = 0x00;
  ZAF_CP_CommandPublish(handle, (void *) &pRxPackage);  //subscribeAll++, subscribeCC++

  /* Test Cmd */
  pRxPackage.uReceiveParams.Rx.Payload.rxBuffer.ZW_Common.cmdClass = 0x02;
  pRxPackage.uReceiveParams.Rx.Payload.rxBuffer.ZW_Common.cmd = 0x01;
  ZAF_CP_CommandPublish(handle, (void *) &pRxPackage);  //subscribeAll++, subscribeCmd++

  /* Test Cmd - will notify a group subscribed to all, but not CMD or CC groups*/
  pRxPackage.uReceiveParams.Rx.Payload.rxBuffer.ZW_Common.cmdClass = 0x05;
  pRxPackage.uReceiveParams.Rx.Payload.rxBuffer.ZW_Common.cmd = 0x05;
  ZAF_CP_CommandPublish(handle, (void *) &pRxPackage); // subscribeAll++,

  char str[80];
  sprintf(str, "Subscribers to all - expected %d, but was triggered %d times", 4, subscribeAll);
  TEST_ASSERT_TRUE_MESSAGE(4 == subscribeAll, str);

  sprintf(str, "Subscribers to CC - expected %d, but was triggered %d times", 1, subscribeCC);
  TEST_ASSERT_TRUE_MESSAGE(1 == subscribeCC, str);

  sprintf(str, "Subscribers to CC - expected %d, but was triggered %d times", 1, subscribeCmd);
  TEST_ASSERT_TRUE_MESSAGE(1 == subscribeCmd, str);
}

