#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdarg.h>
#include <ZAF_TSE.h>
#include "ZAF_types.h"
#include <test_common.h>
#include "CC_Configuration.h"
#include <ZW_TransportEndpoint_mock.h>
#include <ZAF_CC_Invoker.h>
#include <cc_configuration_config_api_mock.h>
#include <ZAF_nvm_mock.h>
#include <SizeOf.h>
#include <ZAF_Common_interface_mock.h>
#include "zaf_transport_tx_mock.h"
#include <assert.h>

// Simplified CC handler invocation.
#define CC_Configuration_handler(b)    invoke_cc_handler_v2(&b->rxOptions,                         \
                                                         &b->frame.as_zw_application_tx_buffer, \
                                                         b->frameLength,                        \
                                                         NULL,                                  \
                                                         NULL)

#define NUMBER_OF_PARAMS_BITMASK(n)   (0x7 & (n))
#define DEFAULT_VALUE_BITMASK(d)      ((d) << 7)
#define BULK_PROPERTIES_BITMASK(n, d) (DEFAULT_VALUE_BITMASK(d) | (NUMBER_OF_PARAMS_BITMASK(n)))


// Array used for fake file system.
static uint32_t files[] = { 0, 0, 0, 0, 0, 0, 0, 0 };

/*
 * This pool of configuration parameters defines 3 pairs of configuration parameters:
 * 1. 2 x 32 bit parameters
 * 2. 2 x 16 bit parameters
 * 3. 2 x  8 bit parameters
 */
cc_config_parameter_metadata_t configuration_pool[] = {
  // 32 bit parameter A
  {
    .number            = 1,
    .next_number       = 2,
    .file_id           = 0,
    .migration_handler = NULL,
    .attributes = {
      .name                        = "32 bit A",
      .info                        = "Parameter 1 long info",
      .format                      = CC_CONFIG_PARAMETER_FORMAT_SIGNED_INTEGER,
      .size                        = CC_CONFIG_PARAMETER_SIZE_32_BIT,
      .min_value.as_int32          = -100000,
      .max_value.as_int32          = -90000,
      .default_value.as_int32      = -95000,
      .flags.read_only             = false,
      .flags.altering_capabilities = false,
      .flags.advanced              = false,
    },
  },
  // 32 bit parameter B
  {
    .number            = 2,
    .next_number       = 3,
    .file_id           = 1,
    .migration_handler = NULL,
    .attributes = {
      .name                        = "32 bit B",
      .info                        = "Dummy",
      .format                      = CC_CONFIG_PARAMETER_FORMAT_SIGNED_INTEGER,
      .size                        = CC_CONFIG_PARAMETER_SIZE_32_BIT,
      .min_value.as_int32          = -80000,
      .max_value.as_int32          = -70000,
      .default_value.as_int32      = -75000,
      .flags.read_only             = false,
      .flags.altering_capabilities = false,
      .flags.advanced              = false,
    },
  },
  // 16 bit parameter A
  {
    .number            = 3,
    .next_number       = 4,
    .file_id           = 2,
    .migration_handler = NULL,
    .attributes = {
      .name                        = "16 bit A",
      .info                        = "Dummy",
      .format                      = CC_CONFIG_PARAMETER_FORMAT_SIGNED_INTEGER,
      .size                        = CC_CONFIG_PARAMETER_SIZE_16_BIT,
      .min_value.as_int16          = -30000,
      .max_value.as_int16          = -20000,
      .default_value.as_int16      = -25000,
      .flags.read_only             = false,
      .flags.altering_capabilities = false,
      .flags.advanced              = false,
    },
  },
  // 16 bit parameter B
  {
    .number            = 4,
    .next_number       = 5,
    .file_id           = 3,
    .migration_handler = NULL,
    .attributes = {
      .name                        = "16 bit B",
      .info                        = "Dummy",
      .format                      = CC_CONFIG_PARAMETER_FORMAT_SIGNED_INTEGER,
      .size                        = CC_CONFIG_PARAMETER_SIZE_16_BIT,
      .min_value.as_int16          = -15000,
      .max_value.as_int16          = -5000,
      .default_value.as_int16      = -10000,
      .flags.read_only             = false,
      .flags.altering_capabilities = false,
      .flags.advanced              = false,
    },
  },
  // 8 bit parameter A
  {
    .number            = 5,
    .next_number       = 6,
    .file_id           = 4,
    .migration_handler = NULL,
    .attributes = {
      .name                        = "8 bit A",
      .info                        = "Dummy",
      .format                      = CC_CONFIG_PARAMETER_FORMAT_SIGNED_INTEGER,
      .size                        = CC_CONFIG_PARAMETER_SIZE_8_BIT,
      .min_value.as_int8           = -100,
      .max_value.as_int8           = -50,
      .default_value.as_int8       = -75,
      .flags.read_only             = false,
      .flags.altering_capabilities = false,
      .flags.advanced              = false,
    },
  },
  // 8 bit parameter B
  {
    .number            = 6,
    .next_number       = 0,
    .file_id           = 5,
    .migration_handler = NULL,
    .attributes = {
      .name                        = "8 bit B",
      .info                        = "Dummy",
      .format                      = CC_CONFIG_PARAMETER_FORMAT_SIGNED_INTEGER,
      .size                        = CC_CONFIG_PARAMETER_SIZE_8_BIT,
      .min_value.as_int8          = -30,
      .max_value.as_int8          = -20,
      .default_value.as_int8      = -25,
      .flags.read_only             = false,
      .flags.altering_capabilities = false,
      .flags.advanced              = false,
    },
  },
};

static cc_configuration_t default_configuration = {
  .numberOfParameters = sizeof_array(configuration_pool),
  .parameters         = &configuration_pool[0]
};

/*
 * Serves a as a fake for reading files.
 */
static zpal_status_t callback_ZAF_nvm_read(zpal_nvm_object_key_t key, void* object, size_t object_size, int call_count)
{
  *((uint32_t*)(object)) = files[key];
  return ZPAL_STATUS_OK;
}

/*
 * Serves a as a fake for writing files.
 */
static zpal_status_t callback_ZAF_nvm_write(zpal_nvm_object_key_t key, const void* object, size_t object_size, int call_count)
{
  cc_config_parameter_value_t value = *((cc_config_parameter_value_t*)(object));
  files[key] = value.as_int32;
  return ZPAL_STATUS_OK;
}

void setUpSuite(void)
{
}

void tearDownSuite(void)
{
}

void setUp(void)
{
  cc_configuration_get_configuration_ExpectAndReturn(&default_configuration);

  ZAF_nvm_read_IgnoreAndReturn(ZPAL_STATUS_OK);
  ZAF_nvm_write_IgnoreAndReturn(ZPAL_STATUS_OK);

  ZAF_CC_init_specific(COMMAND_CLASS_CONFIGURATION_V4);

  ZAF_nvm_read_StopIgnore();
  ZAF_nvm_write_StopIgnore();

  ZAF_nvm_read_Stub(callback_ZAF_nvm_read);
  ZAF_nvm_write_Stub(callback_ZAF_nvm_write);
}

void tearDown(void)
{
}

command_handler_input_t * create_configuration_set(uint8_t parameter_number, uint8_t parameter_size, cc_config_parameter_value_t new_configuration_value)
{
  command_handler_input_t * p_chi = test_common_command_handler_input_allocate();

  p_chi->frame.as_byte_array[p_chi->frameLength++] = COMMAND_CLASS_CONFIGURATION_V4;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = CONFIGURATION_SET_V4;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = parameter_number;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = 0x00              // Default flag
                                                     | parameter_size; // Size
  switch (parameter_size) {
    case CC_CONFIG_PARAMETER_SIZE_32_BIT:
      p_chi->frame.as_byte_array[p_chi->frameLength++] = new_configuration_value.as_uint8_array[3];
      p_chi->frame.as_byte_array[p_chi->frameLength++] = new_configuration_value.as_uint8_array[2];
      p_chi->frame.as_byte_array[p_chi->frameLength++] = new_configuration_value.as_uint8_array[1];
      p_chi->frame.as_byte_array[p_chi->frameLength++] = new_configuration_value.as_uint8_array[0];
      break;
    case CC_CONFIG_PARAMETER_SIZE_16_BIT:
      p_chi->frame.as_byte_array[p_chi->frameLength++] = new_configuration_value.as_uint8_array[1];
      p_chi->frame.as_byte_array[p_chi->frameLength++] = new_configuration_value.as_uint8_array[0];
      break;
    case CC_CONFIG_PARAMETER_SIZE_8_BIT:
      p_chi->frame.as_byte_array[p_chi->frameLength++] = new_configuration_value.as_uint8_array[0];
      break;
    default:
      break;
  }
  return p_chi;
}

command_handler_input_t * create_configuration_bulk_get(uint16_t parameter_offset, uint8_t number_of_parameters)
{
  command_handler_input_t * p_chi = test_common_command_handler_input_allocate();
  p_chi->frame.as_byte_array[p_chi->frameLength++] = COMMAND_CLASS_CONFIGURATION_V4;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = CONFIGURATION_BULK_GET_V4;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = (uint8_t)(parameter_offset >> 8); // MSB
  p_chi->frame.as_byte_array[p_chi->frameLength++] = (uint8_t)parameter_offset;        // LSB
  p_chi->frame.as_byte_array[p_chi->frameLength++] = number_of_parameters;

  return p_chi;
}

command_handler_input_t * create_configuration_bulk_set(uint16_t parameter_offset,
                                                        uint8_t number_of_parameters,
                                                        uint8_t size,
                                                        bool default_value,
                                                        bool handshake,
                                                        ...)
{
  command_handler_input_t * p_chi = test_common_command_handler_input_allocate();

  p_chi->frame.as_byte_array[p_chi->frameLength++] = COMMAND_CLASS_CONFIGURATION_V4;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = CONFIGURATION_BULK_SET_V4;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = (uint8_t)(parameter_offset >> 8); // MSB
  p_chi->frame.as_byte_array[p_chi->frameLength++] = (uint8_t)parameter_offset;        // LSB
  p_chi->frame.as_byte_array[p_chi->frameLength++] = number_of_parameters;
  uint8_t properties = BULK_PROPERTIES_BITMASK(size,default_value);
  if (true == handshake) {
    properties |= 0x40;
  }
  p_chi->frame.as_byte_array[p_chi->frameLength++] = properties;

  va_list list;

  va_start(list, handshake);

  for (uint32_t i = 0; i < number_of_parameters; i++)
  {
    cc_config_parameter_value_t new_value = va_arg(list, cc_config_parameter_value_t);
    switch (size) {
      case CC_CONFIG_PARAMETER_SIZE_32_BIT:
        p_chi->frame.as_byte_array[p_chi->frameLength++] = new_value.as_uint8_array[3];
        p_chi->frame.as_byte_array[p_chi->frameLength++] = new_value.as_uint8_array[2];
        p_chi->frame.as_byte_array[p_chi->frameLength++] = new_value.as_uint8_array[1];
        p_chi->frame.as_byte_array[p_chi->frameLength++] = new_value.as_uint8_array[0];
        break;
      case CC_CONFIG_PARAMETER_SIZE_16_BIT:
        p_chi->frame.as_byte_array[p_chi->frameLength++] = new_value.as_uint8_array[1];
        p_chi->frame.as_byte_array[p_chi->frameLength++] = new_value.as_uint8_array[0];
        break;
      case CC_CONFIG_PARAMETER_SIZE_8_BIT:
        p_chi->frame.as_byte_array[p_chi->frameLength++] = new_value.as_uint8_array[0];
        break;
      default:
        break;
    }
  }

  va_end(list);

  return p_chi;
}

command_handler_input_t * create_configuration_properties_get(uint16_t parameter_number)
{
  command_handler_input_t * p_chi = test_common_command_handler_input_allocate();
  p_chi->frame.as_byte_array[p_chi->frameLength++] = COMMAND_CLASS_CONFIGURATION_V4;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = CONFIGURATION_PROPERTIES_GET_V4;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = (uint8_t)(parameter_number >> 8); // MSB
  p_chi->frame.as_byte_array[p_chi->frameLength++] = (uint8_t)parameter_number;        // LSB

  return p_chi;
}

command_handler_input_t * create_configuration_info_get(uint16_t parameter_number)
{
  command_handler_input_t * p_chi = test_common_command_handler_input_allocate();
  p_chi->frame.as_byte_array[p_chi->frameLength++] = COMMAND_CLASS_CONFIGURATION_V4;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = CONFIGURATION_INFO_GET_V4;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = (uint8_t)(parameter_number >> 8);
  p_chi->frame.as_byte_array[p_chi->frameLength++] = (uint8_t)parameter_number;

  return p_chi;
}

/*
 * This test verifies that a 4 byte parameter can be set using a frame and read using
 * cc_configuration_get().
 */
void test_cc_configuration_handler_cmd_set_4_byte_parameters(void)
{
  const uint8_t PARAMETER_NUMBER = 1;
  cc_config_parameter_value_t parameter = {
    .as_int32 = -96000, // Random, but valid value.
  };

  command_handler_input_t * p_chi = create_configuration_set(PARAMETER_NUMBER,
                                                             CC_CONFIG_PARAMETER_SIZE_32_BIT,
                                                             parameter);

  received_frame_status_t handler_return_value = CC_Configuration_handler(p_chi);
  TEST_ASSERT_EQUAL_MESSAGE(RECEIVED_FRAME_STATUS_SUCCESS, handler_return_value, "Wrong frame status :(");

  bool transaction_result = false;
  cc_config_parameter_buffer_t parameter_buffer;
  transaction_result = cc_configuration_get(PARAMETER_NUMBER, &parameter_buffer);

  TEST_ASSERT_TRUE(transaction_result);
  TEST_ASSERT_EQUAL_INT32_MESSAGE(files[0], parameter_buffer.data_buffer.as_int32,  "int32_t value doesn't match :(");
  test_common_command_handler_input_free(p_chi);
}

/*
 * This test verifies that a 2 byte parameter can be set using a frame and read using
 * cc_configuration_get().
 */
void test_cc_configuration_handler_cmd_set_2_byte_parameters(void)
{
  const uint8_t  PARAMETER_NUMBER = 3;
  cc_config_parameter_value_t parameter = {
    .as_int16 = -27000, // Random, but valid value.
  };

  command_handler_input_t * p_chi = create_configuration_set(PARAMETER_NUMBER,
                                                             CC_CONFIG_PARAMETER_SIZE_16_BIT,
                                                             parameter);

  received_frame_status_t handler_return_value = CC_Configuration_handler(p_chi);
  TEST_ASSERT_MESSAGE(RECEIVED_FRAME_STATUS_SUCCESS == handler_return_value, "Wrong frame status :(");

  bool transaction_result = false;
  cc_config_parameter_buffer_t parameter_buffer;
  transaction_result = cc_configuration_get(PARAMETER_NUMBER, &parameter_buffer);

  TEST_ASSERT_EQUAL(true, transaction_result);
  TEST_ASSERT_EQUAL_INT16_MESSAGE(files[2], parameter_buffer.data_buffer.as_int16,  "int16_t value doesn't match :(");
  test_common_command_handler_input_free(p_chi);
}

/*
 * This test verifies that a 1 byte parameter can be set using a frame and read using
 * cc_configuration_get().
 */
void test_cc_configuration_handler_cmd_set_1_byte_parameters(void)
{
  const uint8_t  PARAMETER_NUMBER = 5;
  cc_config_parameter_value_t parameter = {
    .as_int32 = -80, // Random, but valid value.
  };

  command_handler_input_t * p_chi = create_configuration_set(PARAMETER_NUMBER,
                                                             CC_CONFIG_PARAMETER_SIZE_8_BIT,
                                                             parameter);

  received_frame_status_t handler_return_value = CC_Configuration_handler(p_chi);
  TEST_ASSERT_MESSAGE(RECEIVED_FRAME_STATUS_SUCCESS == handler_return_value, "Wrong frame status :(");

  bool transaction_result = false;
  cc_config_parameter_buffer_t parameter_buffer;
  transaction_result = cc_configuration_get(PARAMETER_NUMBER, &parameter_buffer);

  TEST_ASSERT_TRUE(transaction_result);
  TEST_ASSERT_EQUAL_INT8_MESSAGE(files[4], parameter_buffer.data_buffer.as_int8, "int8_t value doesn't match :(");
  test_common_command_handler_input_free(p_chi);
}

/*
 * This test verifies that two 4 byte parameters can be set using CONFIGURATION_BULK_SET and
 * read using CONFIGURATION_BULK_GET.
 */
void test_cc_configuration_handler_cmd_bulk_set_4_byte_parameters_bulk_report(void)
{
  uint16_t parameter_offset_base = 1;
  const cc_config_parameter_value_t PARAMETERS[] = {
    {
      .as_int32 = -92000 // Random, but valid value.
    },
    {
      .as_int32 = -72000 // Random, but valid value.
    }
  };

  uint8_t number_of_parameters = sizeof_array(PARAMETERS);

  command_handler_input_t * p_chi = create_configuration_bulk_set(parameter_offset_base,
                                                                  number_of_parameters,
                                                                  CC_CONFIG_PARAMETER_SIZE_32_BIT,
                                                                  false,
                                                                  false,
                                                                  PARAMETERS[0],
                                                                  PARAMETERS[1]);

  received_frame_status_t handler_return_value = CC_Configuration_handler(p_chi);

  TEST_ASSERT_MESSAGE(RECEIVED_FRAME_STATUS_SUCCESS == handler_return_value, "CONFIGURATION_BULK_SET_V4 failed :(");

  // Free the previous frame and create a Configuration Bulk Get frame.
  test_common_command_handler_input_free(p_chi);
  p_chi = create_configuration_bulk_get(parameter_offset_base, number_of_parameters);

  Check_not_legal_response_job_IgnoreAndReturn(false);

  static SApplicationHandles appHandles;
  const SNetworkInfo NetworkInfo = {
    .MaxPayloadSize = 1000, // Let the max payload size be big enough for all parameters to fit
                            // into one frame.
  };
  appHandles.pNetworkInfo = &NetworkInfo;

  ZAF_getAppHandle_IgnoreAndReturn(&appHandles);

  zaf_transport_rx_to_tx_options_Ignore();

  const uint8_t EXPECTED_REPORT[] = {
                                     COMMAND_CLASS_CONFIGURATION_V4,
                                     CONFIGURATION_BULK_REPORT_V4,
                                     0, // Parameter offset MSB
                                     1, // Parameter offset LSB
                                     2, // Number of parameters
                                     0, // Reports to follow
                                     CC_CONFIG_PARAMETER_SIZE_32_BIT,
                                     0xFF, // MSB
                                     0xFE,
                                     0x98,
                                     0xA0, // LSB -92000
                                     0xFF, // MSB
                                     0xFE,
                                     0xE6,
                                     0xC0  // LSB -72000
  };

  zaf_transport_tx_ExpectAndReturn(EXPECTED_REPORT, sizeof(EXPECTED_REPORT), NULL, NULL, true);
  zaf_transport_tx_IgnoreArg_zaf_tx_options();

  handler_return_value = CC_Configuration_handler(p_chi);
  TEST_ASSERT_MESSAGE(RECEIVED_FRAME_STATUS_SUCCESS == handler_return_value, "Wrong frame status :(");

  test_common_command_handler_input_free(p_chi);
}

/*
 * This test verifies that two 4 byte parameters can be set using CONFIGURATION_BULK_SET and
 * read using CONFIGURATION_BULK_GET.
 */
void test_configuration_bulk_set_4_byte_with_handshake(void)
{
  uint16_t parameter_offset_base = 0;
  const cc_config_parameter_value_t PARAMETERS[] = {
    {
      .as_int32 = -92000 // Random, but valid value.
    },
    {
      .as_int32 = -72000 // Random, but valid value.
    }
  };

  uint8_t number_of_parameters = sizeof_array(PARAMETERS);

  command_handler_input_t * p_chi = create_configuration_bulk_set(parameter_offset_base,
                                                                  number_of_parameters,
                                                                  CC_CONFIG_PARAMETER_SIZE_32_BIT,
                                                                  false,
                                                                  true,
                                                                  PARAMETERS[0],
                                                                  PARAMETERS[1]);

  /*
   * Configuration Bulk Report expectations
   */
  static SApplicationHandles appHandles;
  const SNetworkInfo NetworkInfo = {
    .MaxPayloadSize = 1000, // Let the max payload size be big enough for all parameters to fit
                            // into one frame.
  };
  appHandles.pNetworkInfo = &NetworkInfo;

  ZAF_getAppHandle_IgnoreAndReturn(&appHandles);

  zaf_transport_rx_to_tx_options_Ignore();

  const uint8_t EXPECTED_REPORT[] = {
                                     COMMAND_CLASS_CONFIGURATION_V4,
                                     CONFIGURATION_BULK_REPORT_V4,
                                     0, // Parameter offset MSB
                                     1, // Parameter offset LSB
                                     2, // Number of parameters
                                     0, // Reports to follow
                                     0x40 | CC_CONFIG_PARAMETER_SIZE_32_BIT,
                                     0xFF, // MSB
                                     0xFE,
                                     0x98,
                                     0xA0, // LSB -92000
                                     0xFF, // MSB
                                     0xFE,
                                     0xE6,
                                     0xC0  // LSB -72000
  };

  zaf_transport_tx_ExpectAndReturn(EXPECTED_REPORT, sizeof(EXPECTED_REPORT), NULL, NULL, true);
  zaf_transport_tx_IgnoreArg_zaf_tx_options();

  received_frame_status_t handler_return_value = CC_Configuration_handler(p_chi);

  TEST_ASSERT_MESSAGE(RECEIVED_FRAME_STATUS_SUCCESS == handler_return_value, "CONFIGURATION_BULK_SET_V4 failed :(");

  // Free the previous frame and create a Configuration Bulk Get frame.
  test_common_command_handler_input_free(p_chi);
}

/*
 * This test verifies that two 2 byte parameters can be set using CONFIGURATION_BULK_SET and
 * read using CONFIGURATION_BULK_GET.
 */
void test_cc_configuration_handler_cmd_bulk_set_2_byte_parameters_bulk_report(void)
{
  uint8_t parameter_offset_base = 3;

  cc_config_parameter_value_t PARAMETERS[] = {
    {
      .as_int16 = -22000 // Random, but valid value.
    },
    {
      .as_int16 = -7000 // Random, but valid value.
    }
  };

  uint8_t number_of_parameters = sizeof_array(PARAMETERS);

  command_handler_input_t * p_chi = create_configuration_bulk_set(parameter_offset_base,
                                                                  number_of_parameters,
                                                                  CC_CONFIG_PARAMETER_SIZE_16_BIT,
                                                                  false,
                                                                  false,
                                                                  PARAMETERS[0],
                                                                  PARAMETERS[1]);

  received_frame_status_t handler_return_value = CC_Configuration_handler(p_chi);

  TEST_ASSERT_MESSAGE(RECEIVED_FRAME_STATUS_SUCCESS == handler_return_value, "Wrong frame status :(");

  // Free the previous frame and create a Configuration Bulk Get frame.
  test_common_command_handler_input_free(p_chi);
  p_chi = create_configuration_bulk_get(parameter_offset_base, number_of_parameters);

  Check_not_legal_response_job_IgnoreAndReturn(false);

  static SApplicationHandles appHandles;
  const SNetworkInfo NetworkInfo = {
    .MaxPayloadSize = 1000, // Let the max payload size be big enough for all parameters to fit
                            // into one frame.
  };
  appHandles.pNetworkInfo = &NetworkInfo;

  ZAF_getAppHandle_IgnoreAndReturn(&appHandles);

  zaf_transport_rx_to_tx_options_Ignore();

  const uint8_t EXPECTED_REPORT[] = {
                                     COMMAND_CLASS_CONFIGURATION_V4,
                                     CONFIGURATION_BULK_REPORT_V4,
                                     0, // Parameter offset MSB
                                     3, // Parameter offset LSB
                                     2, // Number of parameters
                                     0, // Reports to follow
                                     CC_CONFIG_PARAMETER_SIZE_16_BIT,
                                     0xAA, // MSB
                                     0x10, // LSB -22000
                                     0xE4, // MSB
                                     0xA8  // LSB -7000
  };

  zaf_transport_tx_ExpectAndReturn(EXPECTED_REPORT, sizeof(EXPECTED_REPORT), NULL, NULL, true);
  zaf_transport_tx_IgnoreArg_zaf_tx_options();

  handler_return_value = CC_Configuration_handler(p_chi);
  TEST_ASSERT_MESSAGE(RECEIVED_FRAME_STATUS_SUCCESS == handler_return_value, "Wrong frame status :(");

  test_common_command_handler_input_free(p_chi);
}

/*
 * This test verifies that two 1 byte parameters can be set using CONFIGURATION_BULK_SET and
 * read using CONFIGURATION_BULK_GET.
 */
void test_cc_configuration_handler_cmd_bulk_set_1_byte_parameters_bulk_report(void)
{
  uint8_t parameter_offset_base = 5;

  cc_config_parameter_value_t PARAMETERS[] = {
    {
      .as_int8 = -70 // Random, but valid value.
    },
    {
      .as_int8 = -22 // Random, but valid value.
    }
  };

  uint8_t number_of_parameters = sizeof_array(PARAMETERS);

  command_handler_input_t * p_chi = create_configuration_bulk_set(parameter_offset_base,
                                                                  number_of_parameters,
                                                                  CC_CONFIG_PARAMETER_SIZE_8_BIT,
                                                                  false,
                                                                  false,
                                                                  PARAMETERS[0],
                                                                  PARAMETERS[1]);

  received_frame_status_t handler_return_value = CC_Configuration_handler(p_chi);

  TEST_ASSERT_MESSAGE(RECEIVED_FRAME_STATUS_SUCCESS == handler_return_value, "Wrong frame status :(");

  // Free the previous frame and create a Configuration Bulk Get frame.
  test_common_command_handler_input_free(p_chi);
  p_chi = create_configuration_bulk_get(parameter_offset_base, number_of_parameters);

  Check_not_legal_response_job_IgnoreAndReturn(false);

  SApplicationHandles appHandles;
  const SNetworkInfo NetworkInfo = {
    .MaxPayloadSize = 1000,
  };
  appHandles.pNetworkInfo = &NetworkInfo;

  ZAF_getAppHandle_ExpectAndReturn(&appHandles);

  zaf_transport_rx_to_tx_options_Ignore();

  const uint8_t EXPECTED_REPORT[] = {
                                     COMMAND_CLASS_CONFIGURATION_V4,
                                     CONFIGURATION_BULK_REPORT_V4,
                                     0, // Parameter offset MSB
                                     5, // Parameter offset LSB
                                     2, // Number of parameters
                                     0, // Reports to follow
                                     CC_CONFIG_PARAMETER_SIZE_8_BIT,
                                     -70,
                                     -22
  };

  zaf_transport_tx_ExpectAndReturn(EXPECTED_REPORT, sizeof(EXPECTED_REPORT), NULL, NULL, true);
  zaf_transport_tx_IgnoreArg_zaf_tx_options();

  handler_return_value = CC_Configuration_handler(p_chi);
  TEST_ASSERT_MESSAGE(RECEIVED_FRAME_STATUS_SUCCESS == handler_return_value, "Wrong frame status :(");

  test_common_command_handler_input_free(p_chi);
}

/*
 * Verifies that two parameters (4 bytes) are sent in separate Configuration Bulk Reports if the
 * max payload size can only hold one parameter.
 */
void test_cc_configuration_handler_cmd_bulk_set_4_byte_parameters_bulk_report_multiframe(void)
{
  uint8_t parameter_offset_base = 1;

  const cc_config_parameter_value_t PARAMETERS[] = {
    {
      .as_int32 = -92000 // Random, but valid value.
    },
    {
      .as_int32 = -72000 // Random, but valid value.
    }
  };

  uint8_t number_of_parameters = sizeof_array(PARAMETERS);

  command_handler_input_t* p_chi = create_configuration_bulk_set(parameter_offset_base,
                                                                 number_of_parameters,
                                                                 CC_CONFIG_PARAMETER_SIZE_32_BIT,
                                                                 false,
                                                                 false,
                                                                 PARAMETERS[0],
                                                                 PARAMETERS[1]);

  received_frame_status_t handler_return_value = CC_Configuration_handler(p_chi);

  TEST_ASSERT_MESSAGE(RECEIVED_FRAME_STATUS_SUCCESS == handler_return_value, "CONFIGURATION_BULK_SET failed :(");

  // Free the previous frame and create a Configuration Bulk Get frame.
  test_common_command_handler_input_free(p_chi);
  p_chi = create_configuration_bulk_get(parameter_offset_base, number_of_parameters);

  Check_not_legal_response_job_IgnoreAndReturn(false);

  SApplicationHandles appHandles;
  const SNetworkInfo NetworkInfo = {
    .MaxPayloadSize = 11, // A Configuration Bulk Report with one 32 bit value takes up 11 bytes.
  };
  appHandles.pNetworkInfo = &NetworkInfo;

  ZAF_getAppHandle_IgnoreAndReturn(&appHandles);

  zaf_transport_rx_to_tx_options_Ignore();

  ZW_APPLICATION_TX_BUFFER *pTxBuf_1 = malloc(sizeof(ZW_APPLICATION_TX_BUFFER));
  pTxBuf_1->ZW_ConfigurationBulkReport4byteV4Frame.cmdClass = COMMAND_CLASS_CONFIGURATION_V4;
  pTxBuf_1->ZW_ConfigurationBulkReport4byteV4Frame.cmd      = CONFIGURATION_BULK_REPORT_V4;
  pTxBuf_1->ZW_ConfigurationBulkReport4byteV4Frame.parameterOffset1   = 0;
  pTxBuf_1->ZW_ConfigurationBulkReport4byteV4Frame.parameterOffset2   = 1;
  pTxBuf_1->ZW_ConfigurationBulkReport4byteV4Frame.numberOfParameters = 1;
  pTxBuf_1->ZW_ConfigurationBulkReport4byteV4Frame.reportsToFollow    = 1;
  pTxBuf_1->ZW_ConfigurationBulkReport4byteV4Frame.properties1 = 0x00 | CC_CONFIG_PARAMETER_SIZE_32_BIT;
  pTxBuf_1->ZW_ConfigurationBulkReport4byteV4Frame.variantgroup1.parameter1 = 0xFF;
  pTxBuf_1->ZW_ConfigurationBulkReport4byteV4Frame.variantgroup1.parameter2 = 0xFE;
  pTxBuf_1->ZW_ConfigurationBulkReport4byteV4Frame.variantgroup1.parameter3 = 0x98;
  pTxBuf_1->ZW_ConfigurationBulkReport4byteV4Frame.variantgroup1.parameter4 = 0xA0; // -92000

  zaf_transport_tx_ExpectAndReturn((uint8_t *) pTxBuf_1, sizeof(pTxBuf_1->ZW_ConfigurationBulkReport1byteV4Frame), NULL, NULL, true);
  zaf_transport_tx_IgnoreArg_zaf_tx_options();

  ZW_APPLICATION_TX_BUFFER *pTxBuf_2 = malloc(sizeof(ZW_APPLICATION_TX_BUFFER));
  pTxBuf_2->ZW_ConfigurationBulkReport4byteV4Frame.cmdClass = COMMAND_CLASS_CONFIGURATION_V4;
  pTxBuf_2->ZW_ConfigurationBulkReport4byteV4Frame.cmd      = CONFIGURATION_BULK_REPORT_V4;
  pTxBuf_2->ZW_ConfigurationBulkReport4byteV4Frame.parameterOffset1   = 0;
  pTxBuf_2->ZW_ConfigurationBulkReport4byteV4Frame.parameterOffset2   = 2;
  pTxBuf_2->ZW_ConfigurationBulkReport4byteV4Frame.numberOfParameters = 1;
  pTxBuf_2->ZW_ConfigurationBulkReport4byteV4Frame.reportsToFollow    = 0;
  pTxBuf_2->ZW_ConfigurationBulkReport4byteV4Frame.properties1 = 0x00 | CC_CONFIG_PARAMETER_SIZE_32_BIT;
  pTxBuf_2->ZW_ConfigurationBulkReport4byteV4Frame.variantgroup1.parameter1 = 0xFF;
  pTxBuf_2->ZW_ConfigurationBulkReport4byteV4Frame.variantgroup1.parameter2 = 0xFE;
  pTxBuf_2->ZW_ConfigurationBulkReport4byteV4Frame.variantgroup1.parameter3 = 0xE6;
  pTxBuf_2->ZW_ConfigurationBulkReport4byteV4Frame.variantgroup1.parameter4 = 0xC0; // -72000

  zaf_transport_tx_ExpectAndReturn((uint8_t *) pTxBuf_2, sizeof(pTxBuf_1->ZW_ConfigurationBulkReport1byteV4Frame), NULL, NULL, true);
  zaf_transport_tx_IgnoreArg_zaf_tx_options();

  handler_return_value = CC_Configuration_handler(p_chi);
  TEST_ASSERT_MESSAGE(RECEIVED_FRAME_STATUS_SUCCESS == handler_return_value, "Wrong frame status :(");

  free(pTxBuf_1);
  free(pTxBuf_2);
  test_common_command_handler_input_free(p_chi);
}

/*
 * Verify default flag in properties field of Bulk Report command.
 * Covers 1 byte parameter case.
 * Requirements: CC:0070.02.09.11.007, CC:0070.02.09.11.008, CC:0070.02.09.11.009
 */
void test_cc_configuration_handler_cmd_get_bulk_1_byte_parameters_properties(void)
{
  received_frame_status_t handler_return_value;

  uint16_t parameter_offset_base = 5; // Pick first parameter with size 8 bits
  uint8_t number_of_parameters = 2;
  bool is_default_value = true;

  // Set all 8 bit parameters to default and get the BULK REPORT.
  // Default flag should be true.

  // Send BULK SET with Default set to True.
  command_handler_input_t* p_chi = create_configuration_bulk_set(parameter_offset_base,
                                        number_of_parameters,
                                        CC_CONFIG_PARAMETER_SIZE_8_BIT,
                                        is_default_value,
                                        false);

  handler_return_value = CC_Configuration_handler(p_chi);
  TEST_ASSERT_MESSAGE(RECEIVED_FRAME_STATUS_SUCCESS == handler_return_value, "Wrong frame status :(");

  // Now that value is set to default, verify BULK REPORT

  test_common_command_handler_input_free(p_chi);
  p_chi = create_configuration_bulk_get(parameter_offset_base, number_of_parameters);

  Check_not_legal_response_job_IgnoreAndReturn(false);

  SApplicationHandles appHandles;
  const SNetworkInfo NetworkInfo = {
      .MaxPayloadSize = 1000,
  };
  appHandles.pNetworkInfo = &NetworkInfo;

  ZAF_getAppHandle_ExpectAndReturn(&appHandles);

  zaf_transport_rx_to_tx_options_Ignore();

  assert(parameter_offset_base >= 1);

  const uint8_t EXPECTED_REPORT[] = {
      COMMAND_CLASS_CONFIGURATION_V4,
      CONFIGURATION_BULK_REPORT_V4,
      0, // Parameter offset MSB
      parameter_offset_base, // Parameter offset LSB
      number_of_parameters,
      0, // Reports to follow
      BULK_PROPERTIES_BITMASK(CC_CONFIG_PARAMETER_SIZE_8_BIT, is_default_value),
      // The default_configuration array (in the tests) stores the configuration number from offset 1 but the array indexed by 0
      default_configuration.parameters[parameter_offset_base-1].attributes.default_value.as_int8,
      default_configuration.parameters[parameter_offset_base].attributes.default_value.as_int8
  };

  zaf_transport_tx_ExpectAndReturn(EXPECTED_REPORT, sizeof(EXPECTED_REPORT), NULL, NULL, true);
  zaf_transport_tx_IgnoreArg_zaf_tx_options();

  handler_return_value = CC_Configuration_handler(p_chi);
  TEST_ASSERT_MESSAGE(RECEIVED_FRAME_STATUS_SUCCESS == handler_return_value, "Wrong frame status :(");
  test_common_command_handler_input_free(p_chi);

  // 2. Change the value of the parameter
  // and verify that default flag in Bulk report is now set to false

  is_default_value = false;
  number_of_parameters = 1; // Only one paramter will be set with bulk in this test.
  int8_t value = -50; // Random non default value to be set.
  p_chi = create_configuration_bulk_set(parameter_offset_base,
                                        number_of_parameters, // number of parameters
                                        CC_CONFIG_PARAMETER_SIZE_8_BIT,
                                        is_default_value,
                                        false,
                                        value);

  handler_return_value = CC_Configuration_handler(p_chi);

  TEST_ASSERT_MESSAGE(RECEIVED_FRAME_STATUS_SUCCESS == handler_return_value, "Wrong frame status :(");
  test_common_command_handler_input_free(p_chi);

  // Now verify that default flag is false in BULK REPORT cmd
  p_chi = create_configuration_bulk_get(parameter_offset_base, number_of_parameters);

  Check_not_legal_response_job_IgnoreAndReturn(false);

  ZAF_getAppHandle_ExpectAndReturn(&appHandles);

  zaf_transport_rx_to_tx_options_Ignore();

  const uint8_t EXPECTED_REPORT2[] = {
      COMMAND_CLASS_CONFIGURATION_V4,
      CONFIGURATION_BULK_REPORT_V4,
      0, // Parameter offset MSB
      parameter_offset_base, // Parameter offset LSB
      number_of_parameters,
      0, // Reports to follow
      BULK_PROPERTIES_BITMASK(CC_CONFIG_PARAMETER_SIZE_8_BIT, is_default_value),
      value
  };

  zaf_transport_tx_ExpectAndReturn(EXPECTED_REPORT2, sizeof(EXPECTED_REPORT2), NULL, NULL, true);
  zaf_transport_tx_IgnoreArg_zaf_tx_options();

  handler_return_value = CC_Configuration_handler(p_chi);
  TEST_ASSERT_MESSAGE(RECEIVED_FRAME_STATUS_SUCCESS == handler_return_value, "Wrong frame status :(");

  test_common_command_handler_input_free(p_chi);
}


/*
 * Verify default flag in properties field of Bulk Report command.
 * Covers 2 byte parameter case.
 */
void test_cc_configuration_handler_cmd_get_bulk_2_byte_parameters_properties(void)
{
  received_frame_status_t handler_return_value;

  uint16_t parameter_offset_base = 3; // Pick first parameter with size 16 bits
  uint8_t number_of_parameters = 1; // get one parameter
  bool is_default_value = true;

  // Set value to default and get the BULK REPORT. Default flag should be true.
  // For that, send command BULK SET with Default set to True.
  command_handler_input_t* p_chi = create_configuration_bulk_set(parameter_offset_base,
                                        number_of_parameters,
                                        CC_CONFIG_PARAMETER_SIZE_16_BIT,
                                        is_default_value,
                                        false);

  handler_return_value = CC_Configuration_handler(p_chi);
  TEST_ASSERT_MESSAGE(RECEIVED_FRAME_STATUS_SUCCESS == handler_return_value, "Wrong frame status :(");

  // Now that value is set to default, verify BULK REPORT
  test_common_command_handler_input_free(p_chi);
  p_chi = create_configuration_bulk_get(parameter_offset_base, number_of_parameters);

  Check_not_legal_response_job_IgnoreAndReturn(false);

  SApplicationHandles appHandles;
  const SNetworkInfo NetworkInfo = {
      .MaxPayloadSize = 1000,
  };
  appHandles.pNetworkInfo = &NetworkInfo;

  ZAF_getAppHandle_ExpectAndReturn(&appHandles);

  zaf_transport_rx_to_tx_options_Ignore();

  const uint8_t EXPECTED_REPORT[] = {
      COMMAND_CLASS_CONFIGURATION_V4,
      CONFIGURATION_BULK_REPORT_V4,
      0, // Parameter offset MSB
      parameter_offset_base, // Parameter offset LSB
      number_of_parameters,
      0, // Reports to follow
      BULK_PROPERTIES_BITMASK(CC_CONFIG_PARAMETER_SIZE_16_BIT, is_default_value),
      // WARNING: The default_configuration array stores the configuration number from offset 1 but the array indexed by 0
      default_configuration.parameters[parameter_offset_base-1].attributes.default_value.as_uint8_array[1],
      default_configuration.parameters[parameter_offset_base-1].attributes.default_value.as_uint8_array[0]
  };

  zaf_transport_tx_ExpectAndReturn(EXPECTED_REPORT, sizeof(EXPECTED_REPORT), NULL, NULL, true);
  zaf_transport_tx_IgnoreArg_zaf_tx_options();

  handler_return_value = CC_Configuration_handler(p_chi);
  TEST_ASSERT_MESSAGE(RECEIVED_FRAME_STATUS_SUCCESS == handler_return_value, "Wrong frame status :(");
  test_common_command_handler_input_free(p_chi);

  // 2. Change the value of the parameter
  // and verify that default flag in Bulk report is set to false

  is_default_value = false;
  int16_t value = -22000; // Random non default value to be set.
  p_chi = create_configuration_bulk_set(parameter_offset_base,
                                        number_of_parameters,
                                        CC_CONFIG_PARAMETER_SIZE_16_BIT,
                                        is_default_value,
                                        false,
                                        value);

  handler_return_value = CC_Configuration_handler(p_chi);

  TEST_ASSERT_MESSAGE(RECEIVED_FRAME_STATUS_SUCCESS == handler_return_value, "Wrong frame status :(");

  // Now verify that default flag is false in BULK REPORT cmd
  test_common_command_handler_input_free(p_chi);
  p_chi = create_configuration_bulk_get(parameter_offset_base, number_of_parameters);

  Check_not_legal_response_job_IgnoreAndReturn(false);

  ZAF_getAppHandle_ExpectAndReturn(&appHandles);

  zaf_transport_rx_to_tx_options_Ignore();

  const uint8_t EXPECTED_REPORT2[] = {
      COMMAND_CLASS_CONFIGURATION_V4,
      CONFIGURATION_BULK_REPORT_V4,
      0, // Parameter offset MSB
      parameter_offset_base, // Parameter offset LSB
      number_of_parameters,
      0, // Reports to follow
      BULK_PROPERTIES_BITMASK(CC_CONFIG_PARAMETER_SIZE_16_BIT, is_default_value),
      value >> 8, // MSB
      value & 0xFF // LSB
  };

  zaf_transport_tx_ExpectAndReturn(EXPECTED_REPORT2, sizeof(EXPECTED_REPORT2), NULL, NULL, true);
  zaf_transport_tx_IgnoreArg_zaf_tx_options();

  handler_return_value = CC_Configuration_handler(p_chi);
  TEST_ASSERT_MESSAGE(RECEIVED_FRAME_STATUS_SUCCESS == handler_return_value, "Wrong frame status :(");

  test_common_command_handler_input_free(p_chi);
}

/*
 * Verify default flag in properties field of Bulk Report command.
 * Covers 4 byte parameter case.
 */
void test_cc_configuration_handler_cmd_get_bulk_4_byte_parameters_properties(void)
{
  received_frame_status_t handler_return_value;

  uint16_t parameter_offset_base = 1; // Pick first parameter with size 32 bits
  uint8_t number_of_parameters = 1; // get one parameter
  bool is_default_value = true;

  // Set value to default and get the BULK REPORT. Default flag should be true.
  // For that, send command BULK SET with Default set to True.
  command_handler_input_t* p_chi = create_configuration_bulk_set(parameter_offset_base,
                                        number_of_parameters,
                                        CC_CONFIG_PARAMETER_SIZE_32_BIT,
                                        is_default_value,
                                        false);

  handler_return_value = CC_Configuration_handler(p_chi);
  TEST_ASSERT_MESSAGE(RECEIVED_FRAME_STATUS_SUCCESS == handler_return_value, "Wrong frame status :(");

  // Now that value is set to default, verify BULK REPORT
  test_common_command_handler_input_free(p_chi);
  p_chi = create_configuration_bulk_get(parameter_offset_base, number_of_parameters);

  Check_not_legal_response_job_IgnoreAndReturn(false);

  SApplicationHandles appHandles;
  const SNetworkInfo NetworkInfo = {
      .MaxPayloadSize = 1000,
  };
  appHandles.pNetworkInfo = &NetworkInfo;

  ZAF_getAppHandle_ExpectAndReturn(&appHandles);

  zaf_transport_rx_to_tx_options_Ignore();

  assert(parameter_offset_base >= 1);

  const uint8_t EXPECTED_REPORT[] = {
      COMMAND_CLASS_CONFIGURATION_V4,
      CONFIGURATION_BULK_REPORT_V4,
      0, // Parameter offset MSB
      parameter_offset_base, // Parameter offset LSB
      number_of_parameters,
      0, // Reports to follow
      BULK_PROPERTIES_BITMASK(CC_CONFIG_PARAMETER_SIZE_32_BIT, is_default_value),
      // The default_configuration array (in the tests) stores the configuration number from offset 1 but the array indexed by 0
      default_configuration.parameters[parameter_offset_base-1].attributes.default_value.as_uint8_array[3],
      default_configuration.parameters[parameter_offset_base-1].attributes.default_value.as_uint8_array[2],
      default_configuration.parameters[parameter_offset_base-1].attributes.default_value.as_uint8_array[1],
      default_configuration.parameters[parameter_offset_base-1].attributes.default_value.as_uint8_array[0]
  };


  zaf_transport_tx_ExpectAndReturn(EXPECTED_REPORT, sizeof(EXPECTED_REPORT), NULL, NULL, true);
  zaf_transport_tx_IgnoreArg_zaf_tx_options();

  handler_return_value = CC_Configuration_handler(p_chi);
  TEST_ASSERT_MESSAGE(RECEIVED_FRAME_STATUS_SUCCESS == handler_return_value, "Wrong frame status :(");
  test_common_command_handler_input_free(p_chi);

  // 2. Change the value of the parameter
  // and verify that default flag in Bulk report is set to false

  is_default_value = false;
  int32_t value = -92000 ; // Random non default value to be set.
  p_chi = create_configuration_bulk_set(parameter_offset_base,
                                        number_of_parameters,
                                        CC_CONFIG_PARAMETER_SIZE_32_BIT,
                                        is_default_value,
                                        false,
                                        value);

  handler_return_value = CC_Configuration_handler(p_chi);

  TEST_ASSERT_MESSAGE(RECEIVED_FRAME_STATUS_SUCCESS == handler_return_value, "Wrong frame status :(");

  // Now verify that default flag is false in BULK REPORT cmd
  test_common_command_handler_input_free(p_chi);
  p_chi = create_configuration_bulk_get(parameter_offset_base, number_of_parameters);

  Check_not_legal_response_job_IgnoreAndReturn(false);

  ZAF_getAppHandle_ExpectAndReturn(&appHandles);

  zaf_transport_rx_to_tx_options_Ignore();

  const uint8_t EXPECTED_REPORT2[] = {
      COMMAND_CLASS_CONFIGURATION_V4,
      CONFIGURATION_BULK_REPORT_V4,
      0, // Parameter offset MSB
      parameter_offset_base, // Parameter offset LSB
      number_of_parameters,
      0, // Reports to follow
      BULK_PROPERTIES_BITMASK(CC_CONFIG_PARAMETER_SIZE_32_BIT, is_default_value),
      value >> 24, // MSB
      value >> 16, // MSB
      value >> 8, // MSB
      value & 0xFF // LSB
  };

  zaf_transport_tx_ExpectAndReturn(EXPECTED_REPORT2, sizeof(EXPECTED_REPORT2), NULL, NULL, true);
  zaf_transport_tx_IgnoreArg_zaf_tx_options();

  handler_return_value = CC_Configuration_handler(p_chi);
  TEST_ASSERT_MESSAGE(RECEIVED_FRAME_STATUS_SUCCESS == handler_return_value, "Wrong frame status :(");

  test_common_command_handler_input_free(p_chi);
}

/*
 * Verifies that two parameters (2 byte) are sent in separate Configuration Bulk Reports if the max
 * payload size can only hold one parameter.
 */
void test_cc_configuration_handler_cmd_bulk_set_2_byte_parameters_bulk_report_multiframe(void)
{
  uint8_t parameter_offset_base = 3;

  const cc_config_parameter_value_t PARAMETERS[] = {
    {
      .as_int16 = -22000 // Random, but valid value.
    },
    {
      .as_int16 = -7000 // Random, but valid value.
    }
  };

  uint8_t number_of_parameters = sizeof_array(PARAMETERS);

  command_handler_input_t* p_chi = create_configuration_bulk_set(parameter_offset_base,
                                                                 number_of_parameters,
                                                                 CC_CONFIG_PARAMETER_SIZE_16_BIT,
                                                                 false,
                                                                 false,
                                                                 PARAMETERS[0],
                                                                 PARAMETERS[1]);

  received_frame_status_t handler_return_value = CC_Configuration_handler(p_chi);

  TEST_ASSERT_MESSAGE(RECEIVED_FRAME_STATUS_SUCCESS == handler_return_value, "CONFIGURATION_BULK_SET failed :(");

  // Free the previous frame and create a Configuration Bulk Get frame.
  test_common_command_handler_input_free(p_chi);
  p_chi = create_configuration_bulk_get(parameter_offset_base, number_of_parameters);

  Check_not_legal_response_job_IgnoreAndReturn(false);

  SApplicationHandles appHandles;
  const SNetworkInfo NetworkInfo = {
    .MaxPayloadSize = 9, // A Configuration Bulk Report with one 16 bit value takes up 9 bytes.
  };
  appHandles.pNetworkInfo = &NetworkInfo;

  ZAF_getAppHandle_IgnoreAndReturn(&appHandles);

  zaf_transport_rx_to_tx_options_Ignore();

  const uint8_t EXPECTED_REPORT[] = {
                                     COMMAND_CLASS_CONFIGURATION_V4,
                                     CONFIGURATION_BULK_REPORT_V4,
                                     0, // Parameter offset MSB
                                     3, // Parameter offset LSB
                                     1, // Number of parameters
                                     1, // Reports to follow
                                     CC_CONFIG_PARAMETER_SIZE_16_BIT,
                                     0xAA, // MSB
                                     0x10, // LSB -22000
  };

  zaf_transport_tx_ExpectAndReturn(EXPECTED_REPORT, sizeof(EXPECTED_REPORT), NULL, NULL, true);
  zaf_transport_tx_IgnoreArg_zaf_tx_options();

  const uint8_t EXPECTED_REPORT_2[] = {
                                     COMMAND_CLASS_CONFIGURATION_V4,
                                     CONFIGURATION_BULK_REPORT_V4,
                                     0, // Parameter offset MSB
                                     4, // Parameter offset LSB
                                     1, // Number of parameters
                                     0, // Reports to follow
                                     CC_CONFIG_PARAMETER_SIZE_16_BIT,
                                     0xE4, // MSB
                                     0xA8  // LSB -7000
  };

  zaf_transport_tx_ExpectAndReturn(EXPECTED_REPORT_2, sizeof(EXPECTED_REPORT_2), NULL, NULL, true);
  zaf_transport_tx_IgnoreArg_zaf_tx_options();

  handler_return_value = CC_Configuration_handler(p_chi);
  TEST_ASSERT_MESSAGE(RECEIVED_FRAME_STATUS_SUCCESS == handler_return_value, "Wrong frame status :(");
  test_common_command_handler_input_free(p_chi);
}

/*
 * Verifies Configuration Properties Get/Report for a 4 byte configuration parameter.
 */
void test_configuration_properties_get_report_4_bytes(void)
{
  uint16_t PARAMETER_NUMBER = 1;

  command_handler_input_t* p_chi = create_configuration_properties_get(PARAMETER_NUMBER);

  const uint8_t EXPECTED_REPORT[] = {
                                     COMMAND_CLASS_CONFIGURATION_V4,
                                     CONFIGURATION_PROPERTIES_REPORT_V4,
                                     (uint8_t)(PARAMETER_NUMBER >> 8),  // MSB
                                     (uint8_t)PARAMETER_NUMBER,         // LSB
                                     0 | 0 | 0 | 4,                     // Altering cap, Read Only, Format & Size
                                     configuration_pool[0].attributes.min_value.as_uint8_array[3],
                                     configuration_pool[0].attributes.min_value.as_uint8_array[2],
                                     configuration_pool[0].attributes.min_value.as_uint8_array[1],
                                     configuration_pool[0].attributes.min_value.as_uint8_array[0],
                                     configuration_pool[0].attributes.max_value.as_uint8_array[3],
                                     configuration_pool[0].attributes.max_value.as_uint8_array[2],
                                     configuration_pool[0].attributes.max_value.as_uint8_array[1],
                                     configuration_pool[0].attributes.max_value.as_uint8_array[0],
                                     configuration_pool[0].attributes.default_value.as_uint8_array[3],
                                     configuration_pool[0].attributes.default_value.as_uint8_array[2],
                                     configuration_pool[0].attributes.default_value.as_uint8_array[1],
                                     configuration_pool[0].attributes.default_value.as_uint8_array[0],
                                     0,    // Next parameter number MSB
                                     2,    // Next parameter number LSB
                                     0 | 0 // No Bulk Support & Advanced
  };

  zaf_transport_tx_ExpectAndReturn(EXPECTED_REPORT, sizeof(EXPECTED_REPORT), NULL, NULL, true);
  zaf_transport_tx_IgnoreArg_zaf_tx_options();

  received_frame_status_t handler_return_value = CC_Configuration_handler(p_chi);
  TEST_ASSERT_MESSAGE(RECEIVED_FRAME_STATUS_SUCCESS == handler_return_value, "Wrong frame status :(");

  test_common_command_handler_input_free(p_chi);
}

/*
 * Verifies Configuration Properties Get/Report for a 2 byte configuration parameter.
 */
void test_configuration_properties_get_report_2_bytes(void)
{
  uint16_t PARAMETER_NUMBER = 3;

  command_handler_input_t* p_chi = create_configuration_properties_get(PARAMETER_NUMBER);

  const uint8_t EXPECTED_REPORT[] = {
                                     COMMAND_CLASS_CONFIGURATION_V4,
                                     CONFIGURATION_PROPERTIES_REPORT_V4,
                                     (uint8_t)(PARAMETER_NUMBER >> 8),  // MSB
                                     (uint8_t)PARAMETER_NUMBER,         // LSB
                                     0 | 0 | 0 | 2,                     // Altering cap, Read Only, Format & Size
                                     configuration_pool[2].attributes.min_value.as_uint8_array[1],
                                     configuration_pool[2].attributes.min_value.as_uint8_array[0],
                                     configuration_pool[2].attributes.max_value.as_uint8_array[1],
                                     configuration_pool[2].attributes.max_value.as_uint8_array[0],
                                     configuration_pool[2].attributes.default_value.as_uint8_array[1],
                                     configuration_pool[2].attributes.default_value.as_uint8_array[0],
                                     0,    // Next parameter number MSB
                                     4,    // Next parameter number LSB
                                     0 | 0 // No Bulk Support & Advanced
  };

  zaf_transport_tx_ExpectAndReturn(EXPECTED_REPORT, sizeof(EXPECTED_REPORT), NULL, NULL, true);
  zaf_transport_tx_IgnoreArg_zaf_tx_options();

  received_frame_status_t handler_return_value = CC_Configuration_handler(p_chi);
  TEST_ASSERT_MESSAGE(RECEIVED_FRAME_STATUS_SUCCESS == handler_return_value, "Wrong frame status :(");

  test_common_command_handler_input_free(p_chi);
}

/*
 * Verifies Configuration Properties Get/Report for a 1 byte configuration parameter.
 */
void test_configuration_properties_get_report_1_byte(void)
{
  uint16_t PARAMETER_NUMBER = 5;

  command_handler_input_t* p_chi = create_configuration_properties_get(PARAMETER_NUMBER);

  const uint8_t EXPECTED_REPORT[] = {
                                     COMMAND_CLASS_CONFIGURATION_V4,
                                     CONFIGURATION_PROPERTIES_REPORT_V4,
                                     (uint8_t)(PARAMETER_NUMBER >> 8),  // MSB
                                     (uint8_t)PARAMETER_NUMBER,         // LSB
                                     0 | 0 | 0 | 1,                     // Altering cap, Read Only, Format & Size
                                     configuration_pool[4].attributes.min_value.as_uint8_array[0],
                                     configuration_pool[4].attributes.max_value.as_uint8_array[0],
                                     configuration_pool[4].attributes.default_value.as_uint8_array[0],
                                     0,    // Next parameter number MSB
                                     6,    // Next parameter number LSB
                                     0 | 0 // No Bulk Support & Advanced
  };

  zaf_transport_tx_ExpectAndReturn(EXPECTED_REPORT, sizeof(EXPECTED_REPORT), NULL, NULL, true);
  zaf_transport_tx_IgnoreArg_zaf_tx_options();

  received_frame_status_t handler_return_value = CC_Configuration_handler(p_chi);
  TEST_ASSERT_MESSAGE(RECEIVED_FRAME_STATUS_SUCCESS == handler_return_value, "Wrong frame status :(");

  test_common_command_handler_input_free(p_chi);
}

/*
 * Verifies Configuration Properties Get/Report with parameter number set to zero.
 */
void test_configuration_properties_get_report_parameter_number_0(void)
{
  uint16_t PARAMETER_NUMBER = 0;

  command_handler_input_t* p_chi = create_configuration_properties_get(PARAMETER_NUMBER);

  const uint8_t EXPECTED_REPORT[] = {
                                     COMMAND_CLASS_CONFIGURATION_V4,
                                     CONFIGURATION_PROPERTIES_REPORT_V4,
                                     (uint8_t)(PARAMETER_NUMBER >> 8),  // MSB
                                     (uint8_t)PARAMETER_NUMBER,         // LSB
                                     0 | 0 | 0 | 0,                     // Altering cap, Read Only, Format & Size
                                     0,    // Next parameter number MSB
                                     1,    // Next parameter number LSB
                                     0 | 0 // No Bulk Support & Advanced
  };

  zaf_transport_tx_ExpectAndReturn(EXPECTED_REPORT, sizeof(EXPECTED_REPORT), NULL, NULL, true);
  zaf_transport_tx_IgnoreArg_zaf_tx_options();

  received_frame_status_t handler_return_value = CC_Configuration_handler(p_chi);
  TEST_ASSERT_MESSAGE(RECEIVED_FRAME_STATUS_SUCCESS == handler_return_value, "Wrong frame status :(");

  test_common_command_handler_input_free(p_chi);
}

/*
 * Verifies Info Report frame in case when entire info can fit into a single frame
 */
void test_cc_configuration_handler_info_report(void)
{
  uint8_t param_number = 2;
  uint8_t param_index = param_number-1;
  // Info Report for this param should fit into a single frame.
  uint8_t reports_to_follow = 0;
  uint8_t info_len = strlen(configuration_pool[param_index].attributes.info);
  uint8_t header_len = offsetof(ZW_CONFIGURATION_INFO_REPORT_4BYTE_V3_FRAME, info1);

  command_handler_input_t* p_chi = create_configuration_info_get(param_number);

  SApplicationHandles appHandles;
  const SNetworkInfo NetworkInfo = {
    .MaxPayloadSize = 30, // Payload size big enough to report entire Info property in a single frame.
  };
  appHandles.pNetworkInfo = &NetworkInfo;

  ZAF_getAppHandle_IgnoreAndReturn(&appHandles);
  zaf_transport_rx_to_tx_options_Ignore();

  uint8_t frame_header[] = {
      COMMAND_CLASS_CONFIGURATION_V4,
      CONFIGURATION_INFO_REPORT_V4,
      (uint8_t)param_number >> 8,
      (uint8_t)param_number & 0xFF,
      reports_to_follow,
  };

  uint8_t EXPECTED_REPORT[10] = {0}; //header_len + info_len

  // Form the expected frame. In consists of header followed by info.
  memcpy(EXPECTED_REPORT, frame_header, header_len);
  memcpy(&EXPECTED_REPORT[header_len], configuration_pool[param_index].attributes.info, info_len);

  zaf_transport_tx_ExpectAndReturn(EXPECTED_REPORT, sizeof(EXPECTED_REPORT), NULL, NULL, true);
  zaf_transport_tx_IgnoreArg_zaf_tx_options();

  received_frame_status_t handler_return_value = CC_Configuration_handler(p_chi);
  TEST_ASSERT_MESSAGE(RECEIVED_FRAME_STATUS_SUCCESS == handler_return_value, "Wrong frame status :(");

  test_common_command_handler_input_free(p_chi);
}

/*
 * Verifies that Info Report frame can be split into multiple frames
 * if "Info" is too long for a single frame
 */
void test_cc_configuration_handler_info_report_multiframe(void)
{
  uint8_t param_number = 1;
  uint8_t param_index = 1-1;
  uint8_t info_len = strlen(configuration_pool[param_index].attributes.info);
  uint8_t header_len = offsetof(ZW_CONFIGURATION_INFO_REPORT_4BYTE_V3_FRAME, info1);

  command_handler_input_t* p_chi = create_configuration_info_get(param_number);

  SApplicationHandles appHandles;
  const SNetworkInfo NetworkInfo = {
      .MaxPayloadSize = 15, // Set payload size small enough to force multiple frames.
  };
  appHandles.pNetworkInfo = &NetworkInfo;
  // Maximum possible length of info attribute in a current frame
  uint8_t max_info_len = NetworkInfo.MaxPayloadSize - header_len;

  ZAF_getAppHandle_IgnoreAndReturn(&appHandles);
  zaf_transport_rx_to_tx_options_Ignore();

  // Assign 0 initially. And then calculate this value for each frame.
  uint8_t reports_to_follow = 0;

  // The same header is used for all frames.
  uint8_t frame_header[] = {
      COMMAND_CLASS_CONFIGURATION_V4,
      CONFIGURATION_INFO_REPORT_V4,
      (uint8_t)param_number >> 8,
      (uint8_t)param_number & 0xFF,
      reports_to_follow,
  };

  //  Calculate how many reports should be in total
  uint8_t reports_total = 3; // = info_len / max_info_len + ((info_len % max_info_len) ? 1:0);

  // First frame.
  uint8_t EXPECTED_REPORT[15] = {0}; // NetworkInfo.MaxPayloadSize
  int report_no = reports_total-1;

  // length of info attribute part in a current frame
  uint8_t info_current_len =  max_info_len ;
  frame_header[4] = report_no; // Reports to follow
  memcpy(EXPECTED_REPORT, frame_header, header_len);

  // Pointer to the beginning of next info segment
  uint8_t info_index = 0;

  // Form the expected frame. In consists of header followed by info.
  memcpy(&EXPECTED_REPORT[header_len], &configuration_pool[param_index].attributes.info[info_index], info_current_len);

  zaf_transport_tx_ExpectAndReturn(EXPECTED_REPORT, sizeof(EXPECTED_REPORT), NULL, NULL, true);
  zaf_transport_tx_IgnoreArg_zaf_tx_options();

  // Second frame.
  uint8_t EXPECTED_REPORT1[15] = {0};

  report_no--;
  frame_header[4] = report_no; // Reports to follow
  memcpy(EXPECTED_REPORT1, frame_header, header_len);

  info_index += max_info_len;
  // Form the expected frame. In consists of header followed by info.
  memcpy(&EXPECTED_REPORT1[header_len], &configuration_pool[param_index].attributes.info[info_index], info_current_len);

  zaf_transport_tx_ExpectAndReturn(EXPECTED_REPORT1, sizeof(EXPECTED_REPORT1), NULL, NULL, true);
  zaf_transport_tx_IgnoreArg_zaf_tx_options();

  // Third and last frame
  uint8_t EXPECTED_REPORT2[6] = {0};
  report_no--;

  // length of info attribute part in a current frame
  info_current_len = info_len % max_info_len;
  frame_header[4] = report_no;
  memcpy(EXPECTED_REPORT2, frame_header, header_len);

  info_index += max_info_len;
  memcpy(&EXPECTED_REPORT2[header_len], &configuration_pool[param_index].attributes.info[info_index], info_current_len);


  zaf_transport_tx_ExpectAndReturn(EXPECTED_REPORT2, sizeof(EXPECTED_REPORT2), NULL, NULL, true);
  zaf_transport_tx_IgnoreArg_zaf_tx_options();

  received_frame_status_t handler_return_value = CC_Configuration_handler(p_chi);
  TEST_ASSERT_MESSAGE(RECEIVED_FRAME_STATUS_SUCCESS == handler_return_value, "Wrong frame status :(");

  test_common_command_handler_input_free(p_chi);
}

