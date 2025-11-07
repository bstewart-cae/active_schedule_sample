// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

#include "ZAF_CmdPublisher.h"
#include "mock_control.h"


#define MOCK_FILE "ZAF_CmdPublisher_mock.c"

CP_Handle_t ZAF_CP_Init(void *pStorage, uint8_t numSubscribers)
{
  mock_t * p_mock;

  static ZAF_CP_STORAGE(zaf_cp_storage, 100);

  MOCK_CALL_RETURN_IF_USED_AS_STUB((CP_Handle_t)&zaf_cp_storage);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, NULL);
  MOCK_CALL_ACTUAL(p_mock, pStorage, numSubscribers);

  MOCK_CALL_COMPARE_INPUT_POINTER(p_mock, ARG0, pStorage);
  MOCK_CALL_COMPARE_INPUT_UINT8(p_mock, ARG1, numSubscribers);

  MOCK_CALL_RETURN_POINTER(p_mock, CP_Handle_t);
}


bool ZAF_CP_SubscribeToAll(CP_Handle_t handle,
                           void* pSubscriberContext,
                           zaf_cp_subscriberFunction_t pFunction)
{
  mock_t * p_mock;
  MOCK_CALL_RETURN_IF_USED_AS_STUB(true);

  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, false);
  MOCK_CALL_ACTUAL(p_mock, handle, pSubscriberContext, pFunction);

  MOCK_CALL_COMPARE_INPUT_POINTER(p_mock, ARG0, handle);
  MOCK_CALL_COMPARE_INPUT_POINTER(p_mock, ARG1, pSubscriberContext);
  MOCK_CALL_COMPARE_INPUT_POINTER(p_mock, ARG2, pFunction);

  MOCK_CALL_RETURN_POINTER(p_mock, bool);
}

bool ZAF_CP_SubscribeToCC(CP_Handle_t handle,
                          void* pSubscriberContext,
                          zaf_cp_subscriberFunction_t pFunction,
                          uint16_t CmdClass)
{
  mock_t * p_mock;
  MOCK_CALL_RETURN_IF_USED_AS_STUB(true);

  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, false);
  MOCK_CALL_ACTUAL(p_mock, handle, pSubscriberContext, pFunction, CmdClass);

  MOCK_CALL_COMPARE_INPUT_POINTER(p_mock, ARG0, handle);
  MOCK_CALL_COMPARE_INPUT_POINTER(p_mock, ARG1, pSubscriberContext);
  MOCK_CALL_COMPARE_INPUT_POINTER(p_mock, ARG2, pFunction);
  MOCK_CALL_COMPARE_INPUT_UINT16(p_mock, ARG3, CmdClass);

  MOCK_CALL_RETURN_POINTER(p_mock, bool);
}

bool ZAF_CP_SubscribeToCmd(CP_Handle_t handle,
                           void* pSubscriberContext,
                           zaf_cp_subscriberFunction_t pFunction,
                           uint16_t CmdClass,
                           uint8_t Cmd)
{
  mock_t * p_mock;
  MOCK_CALL_RETURN_IF_USED_AS_STUB(true);

  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, false);
  MOCK_CALL_ACTUAL(p_mock, handle, pSubscriberContext, pFunction, CmdClass, Cmd);

  MOCK_CALL_COMPARE_INPUT_POINTER(p_mock, ARG0, handle);
  MOCK_CALL_COMPARE_INPUT_POINTER(p_mock, ARG1, pSubscriberContext);
  MOCK_CALL_COMPARE_INPUT_POINTER(p_mock, ARG2, pFunction);
  MOCK_CALL_COMPARE_INPUT_UINT16(p_mock, ARG3, CmdClass);
  MOCK_CALL_COMPARE_INPUT_UINT16(p_mock, ARG3, Cmd);

  MOCK_CALL_RETURN_POINTER(p_mock, bool);
}

bool ZAF_CP_UnsubscribeToAll(CP_Handle_t handle,
                             void* pSubscriberContext,
                             zaf_cp_subscriberFunction_t pFunction)
{
  mock_t * p_mock;
  MOCK_CALL_RETURN_IF_USED_AS_STUB(true);

  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, false);
  MOCK_CALL_ACTUAL(p_mock, handle, pSubscriberContext, pFunction);

  MOCK_CALL_COMPARE_INPUT_POINTER(p_mock, ARG0, handle);
  MOCK_CALL_COMPARE_INPUT_POINTER(p_mock, ARG1, pSubscriberContext);
  MOCK_CALL_COMPARE_INPUT_POINTER(p_mock, ARG2, pFunction);

  MOCK_CALL_RETURN_POINTER(p_mock, bool);
}

bool ZAF_CP_UnsubscribeToCC(CP_Handle_t handle,
                            void* pSubscriberContext,
                            zaf_cp_subscriberFunction_t pFunction,
                            uint16_t CmdClass)
{
  mock_t * p_mock;
  MOCK_CALL_RETURN_IF_USED_AS_STUB(true);

  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, false);
  MOCK_CALL_ACTUAL(p_mock, handle, pSubscriberContext, pFunction, CmdClass);

  MOCK_CALL_COMPARE_INPUT_POINTER(p_mock, ARG0, handle);
  MOCK_CALL_COMPARE_INPUT_POINTER(p_mock, ARG1, pSubscriberContext);
  MOCK_CALL_COMPARE_INPUT_POINTER(p_mock, ARG2, pFunction);
  MOCK_CALL_COMPARE_INPUT_UINT16(p_mock, ARG3, CmdClass);

  MOCK_CALL_RETURN_POINTER(p_mock, bool);
}

bool ZAF_CP_UnsubscribeToCmd(CP_Handle_t handle,
                             void* pSubscriberContext,
                             zaf_cp_subscriberFunction_t pFunction,
                             uint16_t CmdClass,
                             uint8_t Cmd)
{
  mock_t * p_mock;
  MOCK_CALL_RETURN_IF_USED_AS_STUB(true);

  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, false);
  MOCK_CALL_ACTUAL(p_mock, handle, pSubscriberContext, pFunction, CmdClass, Cmd);

  MOCK_CALL_COMPARE_INPUT_POINTER(p_mock, ARG0, handle);
  MOCK_CALL_COMPARE_INPUT_POINTER(p_mock, ARG1, pSubscriberContext);
  MOCK_CALL_COMPARE_INPUT_POINTER(p_mock, ARG2, pFunction);
  MOCK_CALL_COMPARE_INPUT_UINT16(p_mock, ARG3, CmdClass);
  MOCK_CALL_COMPARE_INPUT_UINT16(p_mock, ARG3, Cmd);

  MOCK_CALL_RETURN_POINTER(p_mock, bool);
}

void ZAF_CP_CommandPublish(CP_Handle_t handle, void * pRxPackage)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(p_mock);

  MOCK_CALL_ACTUAL(p_mock, handle, pRxPackage);

  MOCK_CALL_COMPARE_INPUT_POINTER(p_mock, ARG0, handle);
  MOCK_CALL_COMPARE_INPUT_POINTER(p_mock, ARG1, pRxPackage);

}
