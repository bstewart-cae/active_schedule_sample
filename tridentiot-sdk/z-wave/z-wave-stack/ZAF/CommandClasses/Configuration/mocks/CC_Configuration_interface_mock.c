 /***************************************************************************//**
 * @file CC_Configuration_interface_mock.c
 * @brief CC_Configuration_interface_mock.c
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
#define MOCK_FILE "CC_Configuration_interface_mock.c"

// -----------------------------------------------------------------------------
//              Static Function Declarations
// -----------------------------------------------------------------------------
static bool 
migration_handler_mock(cc_config_parameter_buffer_t* parameter_buffer);

// -----------------------------------------------------------------------------
//                Global Variables
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
//                Static Variables
// -----------------------------------------------------------------------------
const cc_config_parameter_metadata_t node_configuration_pool_default[] = {
  {
    .number            = configuration_index_dummy_limit_0,
    .next_number       = configuration_index_dummy_limit_1,
    .file_id           = 0,
    .migration_handler = migration_handler_mock,

    .attributes = {
      .name                        = "configuration_index_dummy_limit_0",
      .info                        = "configuration_index_dummy_limit_0 info",
      .format                      = CC_CONFIG_PARAMETER_FORMAT_SIGNED_INTEGER,
      .size                        = CC_CONFIG_PARAMETER_SIZE_16_BIT,
      .min_value.as_int16          =  -100,
      .max_value.as_int16          =    50,

      .default_value.as_int16 = 10,

      .flags.read_only             = false,
      .flags.altering_capabilities = false,
      .flags.advanced              = false,
    },
  },
  {
    .number            = configuration_index_dummy_limit_1,
    .next_number       = 0,
    .file_id           = 1,
    .migration_handler = migration_handler_mock,

    .attributes = {
      .name                        = "configuration_index_dummy_limit_1",
      .info                        = "configuration_index_dummy_limit_1 info",
      .format                      = CC_CONFIG_PARAMETER_FORMAT_SIGNED_INTEGER,
      .size                        = CC_CONFIG_PARAMETER_SIZE_16_BIT,
      .min_value.as_int16          =  -100,
      .max_value.as_int16          =   100,

      .default_value.as_int16      = 20,

      .flags.read_only             = false,
      .flags.altering_capabilities = false,
      .flags.advanced              = false,
    },
  }
};

#define SLI_CC_CONFIGURATION_MOCK_FAKE_FILE_EMPTY 0xFF

static cc_config_parameter_value_t nvm_fake[] = {
  {.as_uint8_array = {0xFF, 0xFF, 0xFF, 0xFF}},
  {.as_uint8_array = {0xFF, 0xFF, 0xFF, 0xFF}}
};

static const cc_configuration_t default_configuration = {
  .numberOfParameters = 2,
  .parameters         = &node_configuration_pool_default[0]
};

// -----------------------------------------------------------------------------
//              Public Function Definitions
// -----------------------------------------------------------------------------

cc_configuration_t const*
configuration_get_default_config_table(void)
{
  return &default_configuration;
} 

void
configuration_mock_empty_fake_files(void)
{
  memset(nvm_fake, SLI_CC_CONFIGURATION_MOCK_FAKE_FILE_EMPTY, sizeof(nvm_fake));
}

void
cc_configuration_fill_fake_files_with_dummy_data(void)
{
  uint16_t number_of_parameters = sizeof(node_configuration_pool_default)/sizeof(node_configuration_pool_default[0]);
  for(uint16_t loop_ix = 0; loop_ix < number_of_parameters; loop_ix++)
  {
    uint8_t file_id = (uint8_t)node_configuration_pool_default[loop_ix].file_id;
    memcpy((void*)&nvm_fake[file_id],
           (void*)&node_configuration_pool_default[loop_ix].attributes.max_value,
           sizeof(cc_config_parameter_value_t));
  }
}
// -----------------------------------------------------------------------------
//              Static Function Definitions
// -----------------------------------------------------------------------------
bool
cc_configuration_io_read_FAKE(zpal_nvm_object_key_t file_id, uint8_t *data_buffer, size_t size)
{
  bool retval = false;
  TEST_ASSERT_NOT_NULL(data_buffer);
  uint8_t empty_file_pattern[] = {0xFF, 0xFF, 0xFF, 0xFF};

  if(0 == memcmp(empty_file_pattern, nvm_fake[file_id].as_uint8_array, sizeof(nvm_fake[file_id].as_uint8_array)))
  {
    retval = false;
  }
  else
  {
    memcpy(data_buffer, &nvm_fake[file_id], sizeof(cc_config_parameter_value_t));
    retval = true;
  }

  return retval;
}

bool
cc_configuration_io_write_FAKE(zpal_nvm_object_key_t file_id, uint8_t const* data, size_t size)
{
  bool retval = true;
  TEST_ASSERT_NOT_NULL(data);

  memcpy(&nvm_fake[file_id], data, sizeof(cc_config_parameter_value_t));

  return retval;
}

bool
cc_configuration_io_write(zpal_nvm_object_key_t file_id, uint8_t const* data_to_write, size_t size)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_FAKE(cc_configuration_io_write_FAKE, file_id, data_to_write, size);

  MOCK_CALL_RETURN_IF_USED_AS_STUB(true);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, false);
  MOCK_CALL_ACTUAL(p_mock, file_id, data_to_write, size);

  return true;
}

bool
cc_configuration_io_read(zpal_nvm_object_key_t file_id, uint8_t *data_buffer, size_t size)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_FAKE(cc_configuration_io_read_FAKE, file_id, data_buffer, size);

  MOCK_CALL_RETURN_IF_USED_AS_STUB(true);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, false);
  MOCK_CALL_ACTUAL(p_mock, file_id, data_buffer, size);

  return true;
}

static bool
migration_handler_mock(cc_config_parameter_buffer_t* parameter_buffer)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(true);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, false);
  MOCK_CALL_ACTUAL(p_mock, parameter_buffer);

  return true;
}