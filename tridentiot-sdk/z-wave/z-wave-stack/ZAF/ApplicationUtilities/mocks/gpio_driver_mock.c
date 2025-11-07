// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file gpio_driver_mock.c
 * @copyright 2022 Silicon Laboratories Inc.
 */

#include <ZW_typedefs.h>
#include <stdint.h>
#include <mock_control.h>

#define MOCK_FILE "gpio_driver_mock.c"
bool
gpio_DriverInit(bool automaticPinSwap)
{
  return true;
}

void
gpio_SetPinIn(uint8_t pin, bool fPullUp)
{

}

void gpio_SetPinOut(uint8_t pin)
{

}

void gpio_SetPin(uint8_t pin, bool fValue)
{

}

bool gpio_GetPin(uint8_t pin)
{
  return true;
}

bool
gpio_GetPinBool(uint8_t pin, uint8_t * pfState)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(true);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, false);
  MOCK_CALL_ACTUAL(p_mock, pin, pfState);

  MOCK_CALL_RETURN_IF_ERROR_SET(p_mock, bool);

  MOCK_CALL_COMPARE_INPUT_UINT8(p_mock, ARG0, pin);
  MOCK_CALL_COMPARE_INPUT_POINTER(p_mock, ARG1, pfState);

  *pfState = (uint8_t)p_mock->output_arg[1].v;
  MOCK_CALL_RETURN_VALUE(p_mock, bool);
}
