// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file zaf_job_helper_mock.c
 * @copyright 2022 Silicon Laboratories Inc.
 */

#include "mock_control.h"

#define MOCK_FILE "zaf_job_helper_mock.c"

void ZAF_JobHelperInit(void)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(p_mock);
}

bool ZAF_JobHelperJobEnqueue(uint8_t event)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(true);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, false);
  MOCK_CALL_ACTUAL(p_mock, event);

  MOCK_CALL_COMPARE_INPUT_UINT8(p_mock, ARG0, event);

  MOCK_CALL_RETURN_VALUE(p_mock, bool);
}

bool ZAF_JobHelperJobDequeue(uint8_t * pEvent)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(true);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, false);
  MOCK_CALL_ACTUAL(p_mock, pEvent);

  MOCK_CALL_COMPARE_INPUT_POINTER(p_mock, ARG0, pEvent);

  MOCK_CALL_RETURN_VALUE(p_mock, bool);
}
