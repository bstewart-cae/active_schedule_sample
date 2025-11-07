/**
 * @file test_common.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include <test_common.h>
#include <string.h>
#include <stdlib.h>
#include <ZAF_CC_Invoker.h>

void test_common_clear_command_handler_input(command_handler_input_t * pCommandHandlerInput)
{
  test_common_clear_command_handler_input_array(pCommandHandlerInput, 1);
}

void test_common_clear_command_handler_input_array(command_handler_input_t * pCommandHandlerInput, uint8_t count)
{
  memset((uint8_t *)pCommandHandlerInput, 0x00, sizeof(command_handler_input_t) * count);
}

command_handler_input_t * test_common_command_handler_input_allocate(void)
{
  command_handler_input_t * p_chi = (command_handler_input_t *)malloc(sizeof(command_handler_input_t));
  memset((uint8_t *)p_chi, 0x00, sizeof(command_handler_input_t));
  return p_chi;
}

void test_common_command_handler_input_free(command_handler_input_t *p_chi)
{
  free(p_chi);
}

received_frame_status_t
invoke_cc_handler_v2 (
    RECEIVE_OPTIONS_TYPE_EX *rxOpt,
    ZW_APPLICATION_TX_BUFFER *pFrameIn,
    uint8_t cmdLength,
    ZW_APPLICATION_TX_BUFFER *pFrameOut,
    uint8_t *pLengthOut)
{
  cc_handler_input_t i = {
      .rx_options = rxOpt,
      .frame = pFrameIn,
      .length = cmdLength };

  cc_handler_output_t o = {
      .frame = pFrameOut,
      .length = 0,
      .duration = 0
  };
  received_frame_status_t status = invoke_cc_handler (&i, &o);
  if(pLengthOut != NULL) {
    *pLengthOut = o.length;
  }
  return status;
}
