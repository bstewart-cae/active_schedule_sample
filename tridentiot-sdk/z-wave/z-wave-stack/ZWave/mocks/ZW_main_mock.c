// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file ZW_main_mock.c
 * @copyright 2022 Silicon Laboratories Inc.
 */

#include <ZW_typedefs.h>
#include "mock_control.h"
#include "unity.h"
#include <ZW_protocol_interface.h>

#define MOCK_FILE "ZW_main_mock.c"

bool ZW_SetSleepMode(
    uint8_t Mode,
    uint8_t IntEnable,
    uint8_t bBeamCount)
{
  return true;
}

void
ZW_SetWutTimeout(
  uint32_t wutTimeout)
{

}

uint8_t ZW_Type_Library(void)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(0x00);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(pMock, 0xFF);
  MOCK_CALL_RETURN_VALUE(pMock, uint8_t);
}

uint8_t ZW_SetRFReceiveMode(uint8_t mode)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(0x00);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, 0xFF);
  MOCK_CALL_ACTUAL(p_mock, mode);
  MOCK_CALL_RETURN_IF_ERROR_SET(p_mock, uint8_t);
  MOCK_CALL_COMPARE_INPUT_UINT8(p_mock, ARG0, mode);
  MOCK_CALL_RETURN_VALUE(p_mock, uint8_t);
}

void NewIDCreate(void)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(pMock);
}

void
ZW_mainDeepSleepPowerLockEnable(bool powerLockEnable)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(pMock);
  MOCK_CALL_ACTUAL(pMock, powerLockEnable);
  MOCK_CALL_COMPARE_INPUT_BOOL(pMock, ARG0, powerLockEnable);
}

void
NetworkIdUpdateValidateNotify(void)
{

}

void ProtocolChangeRfPHYNotify(void)
{

}

void
ZwaveTask(SApplicationInterface* pAppInterface)
{

}

/// @sa https://github.com/Z-Wave-Alliance/z-wave-stack/issues/177
uint16_t ZW_GetProtocolManufacturerId(void)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(0);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, 0);
  MOCK_CALL_RETURN_IF_ERROR_SET(p_mock, uint16_t);

  MOCK_CALL_RETURN_VALUE(p_mock, uint16_t);
}
