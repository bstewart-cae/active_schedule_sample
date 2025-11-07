/**
 * @file test_CC_Basic.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include <test_common.h>
#include <string.h>
#include <mock_control.h>
#include <CC_Common.h>
#include <CC_Basic.h>
#include "ZAF_CC_Invoker.h"

void setUpSuite(void) {

}

void tearDownSuite(void) {

}

void setUp(void) {

}

void tearDown(void) {

}

#define CC_Basic_handler(a,b,c) invoke_cc_handler_v2(a,b,c,NULL,NULL)

static void basic_set_frame_create(
    command_handler_input_t * pCommandHandlerInput,
    uint8_t value);

void test_BASIC_SET_invalid_value(void)
{
  mock_calls_clear();

  command_handler_input_t chi_basic_set;
  test_common_clear_command_handler_input(&chi_basic_set);

  received_frame_status_t status;

  for (uint8_t i = 0x64; i < 0xFF; i++)
  {
    basic_set_frame_create(&chi_basic_set, i);

    status = CC_Basic_handler(
        &chi_basic_set.rxOptions,
        &chi_basic_set.frame.as_zw_application_tx_buffer,
        chi_basic_set.frameLength);

    TEST_ASSERT_MESSAGE(status == RECEIVED_FRAME_STATUS_FAIL, "Wrong frame status :(");
  }

  mock_calls_verify();
}

static void basic_set_frame_create(
    command_handler_input_t * pCommandHandlerInput,
    uint8_t value)
{
  uint8_t frameCount = 0;
  memset(pCommandHandlerInput->frame.as_byte_array, 0x00, sizeof(ZW_APPLICATION_TX_BUFFER));
  pCommandHandlerInput->frame.as_byte_array[frameCount++] = COMMAND_CLASS_BASIC;
  pCommandHandlerInput->frame.as_byte_array[frameCount++] = BASIC_SET;
  pCommandHandlerInput->frame.as_byte_array[frameCount++] = value;
  pCommandHandlerInput->frameLength = frameCount;
}
