// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file board_indicator_mock.c
 * @copyright 2022 Silicon Laboratories Inc.
 */

#include <mock_control.h>
#include <board_indicator.h>

#define MOCK_FILE "board_indicator_mock.c"

void
Board_IndicateStatus(board_status_t status)
{
  ;
}

bool Board_IndicatorControl(uint32_t on_time_ms,
                            uint32_t off_time_ms,
                            uint32_t num_cycles,
                            bool called_from_indicator_cc)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(true);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, false);

  MOCK_CALL_ACTUAL(p_mock, on_time_ms, off_time_ms, num_cycles, called_from_indicator_cc);

  MOCK_CALL_COMPARE_INPUT_UINT32(p_mock, ARG0, on_time_ms);
  MOCK_CALL_COMPARE_INPUT_UINT32(p_mock, ARG1, off_time_ms);
  MOCK_CALL_COMPARE_INPUT_UINT32(p_mock, ARG2, num_cycles);
  MOCK_CALL_COMPARE_INPUT_BOOL(p_mock, ARG3, called_from_indicator_cc);

  MOCK_CALL_RETURN_VALUE(p_mock, bool);
}

void Board_IndicatorInit(void)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(p_mock);
}

bool Board_IsIndicatorActive(void)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(false);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, false);

  MOCK_CALL_RETURN_VALUE(p_mock, bool);
}
