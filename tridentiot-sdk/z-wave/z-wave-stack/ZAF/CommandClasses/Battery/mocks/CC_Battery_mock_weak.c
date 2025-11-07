/**
 * @file CC_Battery_mock_extern.c
 * @copyright 2022 Silicon Laboratories Inc.
 */

#include <mock_control.h>
#include <ZW_typedefs.h>
#include <ZW_classcmd.h>
#include <stdint.h>
#include <CC_Common.h>
#include <CC_Battery.h>

#define MOCK_FILE "CC_Battery_mock_weak.c"

uint8_t CC_Battery_BatteryGet_handler(uint8_t endpoint)
{
  mock_t * p_mock;
  MOCK_CALL_RETURN_IF_USED_AS_STUB(0xFF);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, 0);
  MOCK_CALL_ACTUAL(p_mock, endpoint);
  MOCK_CALL_COMPARE_INPUT_UINT8(p_mock, ARG0, endpoint);
  MOCK_CALL_RETURN_VALUE(p_mock, uint8_t);
}
