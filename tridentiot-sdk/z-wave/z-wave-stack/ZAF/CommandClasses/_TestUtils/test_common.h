/**
 * @file test_common.h
 * @copyright 2022 Silicon Laboratories Inc.
 */
#ifndef PRODUCTPLUS_APPLICATIONCOMMANDHANDLERS_TESTS_TEST_COMMON_H_
#define PRODUCTPLUS_APPLICATIONCOMMANDHANDLERS_TESTS_TEST_COMMON_H_

#include <stdint.h>
#include <ZW_typedefs.h>
#include <ZW_classcmd.h>
#include <ZAF_types.h>

#define INVOKE_CC_HANDLER(handler, testObject) handler( &testObject->rxOptions, \
                                                        &testObject->frame.as_zw_application_tx_buffer, \
                                                        testObject->frameLength)

typedef union
{
  ZW_APPLICATION_TX_BUFFER as_zw_application_tx_buffer;
  uint8_t as_byte_array[sizeof(ZW_APPLICATION_TX_BUFFER)];
}
zaf_frame_t;

typedef struct
{
  RECEIVE_OPTIONS_TYPE_EX rxOptions;
  zaf_frame_t frame;
  uint8_t frameLength;
}
command_handler_input_t;

/**
 * Clears everything in a given Command Handler Input struct.
 * @param pCommandHandlerInput Pointer to Command Handler Input struct.
 */
void test_common_clear_command_handler_input(command_handler_input_t * pCommandHandlerInput);

/**
 * Clears everything in a given array of Command Handler Input structs.
 * @param pCommandHandlerInput Pointer to an array of Command Handler Input structs.
 * @param count Number of structs in the given array.
 */
void test_common_clear_command_handler_input_array(command_handler_input_t * pCommandHandlerInput, uint8_t count);

/**
 * Allocates a CHI and writes zeroes to it.
 * @return Pointer to allocated CHI.
 */
command_handler_input_t * test_common_command_handler_input_allocate(void);

/**
 * Frees a CHI
 * 
 * @param p_chi Pointer to Command Handler Input struct. 
 */
void test_common_command_handler_input_free(command_handler_input_t *p_chi);

/**
 * Helper function for reusing old version of invoke_cc_handler function in unit tests
 * @param rxOpt
 * @param pFrameIn
 * @param cmdLength
 * @param pFrameOut
 * @param pLengthOut
 * @return
 */
received_frame_status_t
invoke_cc_handler_v2 (
    RECEIVE_OPTIONS_TYPE_EX *rxOpt,
    ZW_APPLICATION_TX_BUFFER *pFrameIn,
    uint8_t cmdLength,
    ZW_APPLICATION_TX_BUFFER *pFrameOut,
    uint8_t *pLengthOut);

#endif /* PRODUCTPLUS_APPLICATIONCOMMANDHANDLERS_TESTS_TEST_COMMON_H_ */
