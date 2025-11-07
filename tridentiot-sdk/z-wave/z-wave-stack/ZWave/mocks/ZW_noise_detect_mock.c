// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file ZW_noise_detect_mock.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include <ZW_typedefs.h>
#include "mock_control.h"
#include "unity.h"
#include "ZW_basis_api.h"
#include <string.h>

#define MOCK_FILE "ZW_noise_detect_mock.c"

void NoiseDetectSetDefault(void)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(pMock);
}

void NoiseDetectPowerOnInit(void)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(pMock);
}

void
ZW_GetBackgroundRSSI(RSSI_LEVELS *noise_levels)
{
     mock_t * pMock;

     MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
     MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(pMock);

     MOCK_CALL_COMPARE_INPUT_POINTER(
               pMock,
               ARG0,
               noise_levels);

     if(NULL != pMock->output_arg[ARG0].p)
     {
       memcpy(noise_levels, pMock->output_arg[ARG0].p, sizeof(RSSI_LEVELS));
     }

}

void SampleNoiseLevel(void)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(pMock);
  MOCK_CALL_ACTUAL(pMock,NULL);
}

void
NoiseDetectInit(void)
{

}
