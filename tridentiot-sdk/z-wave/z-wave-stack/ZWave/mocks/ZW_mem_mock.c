// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file ZW_mem_mock.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include <string.h>
#include <ZW_typedefs.h>
#include <mock_control.h>
#include <string.h>

#define MOCK_FILE "ZW_mem_mock.c"

void
MemoryGetID(
  uint8_t *pHomeID,
  uint8_t *pNodeID)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(p_mock);
  MOCK_CALL_ACTUAL(p_mock, pHomeID, pNodeID);

  if(NULL != pHomeID)
  {
    uint8_t i = 0;
    for(i = 0; i < 4; i++)
    {
      pHomeID[i] = *(((uint8_t*)p_mock->output_arg[0].p) + i);
    }
  }
  *pNodeID =  p_mock->output_arg[1].v;
}

uint8_t MemoryPutID(uint8_t *homeID, uint8_t  bNodeID)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(0xFF);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, 0x00);

  MOCK_CALL_COMPARE_INPUT_UINT8_ARRAY(p_mock, ARG0, 4, homeID, 4);
  MOCK_CALL_COMPARE_INPUT_UINT8(p_mock, ARG1, bNodeID);

  MOCK_CALL_RETURN_VALUE(p_mock, uint8_t);
}

uint8_t
MemoryGetByte(uint16_t offset)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(0xFF);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, 0x00);

  MOCK_CALL_COMPARE_INPUT_UINT16(p_mock, ARG0, offset);

  MOCK_CALL_RETURN_VALUE(p_mock, uint8_t);
}

uint8_t
MemoryPutByte(uint16_t offset, uint8_t bData)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(0xFF);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, 0x00);

  MOCK_CALL_COMPARE_INPUT_UINT16(p_mock, ARG0, offset);

  MOCK_CALL_RETURN_VALUE(p_mock, uint8_t);
}

void
MemoryGetBuffer(uint16_t offset, uint8_t *buffer, uint8_t length)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(p_mock);
  MOCK_CALL_ACTUAL(p_mock, offset, buffer, length);

  MOCK_CALL_COMPARE_INPUT_UINT16(p_mock, ARG0, offset);
  MOCK_CALL_COMPARE_INPUT_POINTER(p_mock, ARG1, buffer);
  MOCK_CALL_COMPARE_INPUT_UINT8(p_mock, ARG2, length);

  memcpy(buffer, p_mock->output_arg[1].p, length);
}

uint8_t
MemoryPutBuffer(uint16_t offset, uint8_t *buffer, uint16_t length)
{
  return 0;
}

uint8_t
ZW_MemoryPutBuffer(
 uint16_t  offset,
 uint8_t  *buffer,
 uint16_t  length)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB((uint8_t)true);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, (uint8_t)false);

  MOCK_CALL_COMPARE_INPUT_UINT16(p_mock, ARG0, offset);

  MOCK_CALL_COMPARE_INPUT_UINT8_ARRAY(
          p_mock,
          ARG1,
          p_mock->expect_arg[2].v,
          buffer,
          length);

  MOCK_CALL_COMPARE_INPUT_UINT16(p_mock, ARG2, length);

  MOCK_CALL_RETURN_VALUE(p_mock, uint8_t);
}

