 /***************************************************************************//**
 * @file CC_Configuration_mock.c
 * @brief CC_Configuration_mock.c
 * @copyright 2020 Silicon Laboratories Inc.
 *******************************************************************************
 * # License
 * <b>Copyright 2020 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * The licensor of this software is Silicon Laboratories Inc. Your use of this
 * software is governed by the terms of Silicon Labs Master Software License
 * Agreement (MSLA) available at
 * www.silabs.com/about-us/legal/master-software-license-agreement. This
 * software is distributed to you in Source Code format and is governed by the
 * sections of the MSLA applicable to Source Code.
 *
 ******************************************************************************/
// -----------------------------------------------------------------------------
//                   Includes
// -----------------------------------------------------------------------------
#include <string.h>
#include <stdbool.h>
#include <CC_Configuration.h>
#include "CC_Configuration_interface_mock.h"
#include <mock_control.h>
// -----------------------------------------------------------------------------
//                Macros and Typedefs
// -----------------------------------------------------------------------------
#define MOCK_FILE "CC_Configuration_mock.c"

// -----------------------------------------------------------------------------
//              Static Function Declarations
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
//                Global Variables
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
//                Static Variables
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
//              Public Function Definitions
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
//              Static Function Definitions
// -----------------------------------------------------------------------------

bool
cc_configuration_get_FAKE(uint16_t parameter_number, cc_config_parameter_buffer_t* parameter_buffer)
{
  TEST_ASSERT_NOT_NULL(parameter_buffer);
  memset(parameter_buffer, 0, sizeof(cc_config_parameter_buffer_t));
  const cc_configuration_t *config_table;
  config_table = configuration_get_default_config_table();
  for(uint8_t index = 0; index < config_table->numberOfParameters; index++)
  {
     if(parameter_number ==config_table->parameters->number)
     {
       parameter_buffer->metadata = &config_table->parameters[index];
       return true;
     } 
  }
  return false;
}

bool
cc_configuration_get(uint16_t parameter_number, cc_config_parameter_buffer_t* parameter_buffer)
{
  mock_t * p_mock;
  MOCK_CALL_RETURN_IF_USED_AS_STUB(true);
  MOCK_CALL_RETURN_IF_USED_AS_FAKE(cc_configuration_get_FAKE, parameter_number, parameter_buffer);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, false);
  MOCK_CALL_ACTUAL(p_mock, parameter_number, parameter_buffer);
  MOCK_CALL_RETURN_VALUE(p_mock, bool);
}

received_frame_status_t CC_Configuration_handler( RECEIVE_OPTIONS_TYPE_EX *pRxOpt,
                                                  ZW_APPLICATION_TX_BUFFER *pCmd,
                                                  uint8_t cmdLength)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(RECEIVED_FRAME_STATUS_SUCCESS);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(pMock, RECEIVED_FRAME_STATUS_FAIL);
  MOCK_CALL_ACTUAL(pMock, pRxOpt, pCmd, cmdLength);

  MOCK_CALL_COMPARE_INPUT_POINTER(pMock, ARG0, pRxOpt);
  MOCK_CALL_COMPARE_INPUT_POINTER(pMock, ARG1, pCmd);
  MOCK_CALL_COMPARE_INPUT_UINT8(pMock, ARG2, cmdLength);

  MOCK_CALL_RETURN_VALUE(pMock, received_frame_status_t);
}