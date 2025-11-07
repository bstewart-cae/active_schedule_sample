/**
 * @file
 *
 * @copyright 2022 Silicon Laboratories Inc.
 *
 */
#include <zaf_cc_list_generator.h>
#include <ZAF_CC_Invoker_mock.h>
#include <zaf_config_api_mock.h>
#include <SizeOf.h>
#include <stdlib.h>
#include <string.h>

static CC_handler_map_latest_t *g_handler_maps = NULL;
static size_t g_handler_maps_size = 0;

static zaf_cc_config_entry_latest_t *g_config_entries = NULL;
static size_t g_config_entries_size = 0;

static void ZAF_CC_foreach_Callback(zaf_cc_invoker_callback_t callback, zaf_cc_context_t context,
                                    __attribute__((unused)) int cmock_num_calls)
{
  uint8_t i;

  for (i = 0; i < g_handler_maps_size; i++) {
    if (true == callback(&g_handler_maps[i], context)) {
      break;
    }
  }
}

static void ZAF_CC_config_foreach_Callback(zaf_cc_config_invoker_callback_t callback, void* context,
                                           __attribute__((unused)) int cmock_num_calls)
{
  uint8_t i;

  for (i = 0; i < g_config_entries_size; i++) {
    if (true == callback(&g_config_entries[i], context)) {
      break;
    }
  }
}

static void validate_list(zaf_cc_list_t *zaf_cc_list, uint8_t *expected_zaf_cc_list, size_t expected_zaf_cc_list_size)
{
  uint8_t i, j;
  uint8_t *board;

  // Verify zaf_cc_list
  TEST_ASSERT_EQUAL_INT8_MESSAGE(expected_zaf_cc_list_size, zaf_cc_list->list_size, "Expected list and actual list size don't match.");

  board = malloc(zaf_cc_list->list_size * sizeof(uint8_t));
  memset(board, 0, zaf_cc_list->list_size * sizeof(uint8_t));
  for (i = 0; i < zaf_cc_list->list_size; i++) {
    for (j = 0; j < expected_zaf_cc_list_size; j++) {
      if (zaf_cc_list->cc_list[i] == expected_zaf_cc_list[j]) {
        board[i] = 1;
      }
    }
  }

  for (i = 0; i < zaf_cc_list->list_size; i++) {
    TEST_ASSERT_EQUAL_INT8_MESSAGE(1, board[i], "Expected list and actual list elements don't match.");
  }
  free(board);
}

void
setUpSuite(void)
{
}

void
tearDownSuite(void)
{
}

void
setUp(void)
{
  // Clear global variables before new test starts
  // These pointers will point to local variable that only exist during the
  // test execution
  g_handler_maps = NULL;
  g_handler_maps_size = 0;

  g_config_entries = NULL;
  g_config_entries_size = 0;
}

void
tearDown(void)
{
}

/**
 * Test CC List Generator With Switch On Off information
 */
void test_switch_on_off(void)
{
  zaf_cc_list_t *unsecure_included_cc;
  zaf_cc_list_t *secure_included_unsecure_cc;
  zaf_cc_list_t *secure_included_secure_cc;

  CC_handler_map_latest_t handler_maps[] = {
    {
      .CC = COMMAND_CLASS_FIRMWARE_UPDATE_MD_V5
    },
    {
      .CC = COMMAND_CLASS_ASSOCIATION_GRP_INFO
    },
    {
      .CC = COMMAND_CLASS_SWITCH_BINARY_V2
    },
    {
      .CC = COMMAND_CLASS_VERSION
    },
    {
      .CC = COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION_V2
    },
    {
      .CC = COMMAND_CLASS_SUPERVISION
    },
    {
      .CC = COMMAND_CLASS_POWERLEVEL
    },
    {
      .CC = COMMAND_CLASS_BASIC
    },
    {
      .CC = COMMAND_CLASS_DEVICE_RESET_LOCALLY
    },
    {
      .CC = COMMAND_CLASS_ASSOCIATION
    },
    {
      .CC = COMMAND_CLASS_MANUFACTURER_SPECIFIC
    },
    {
      .CC = 0xFF
    },
    {
      .CC = COMMAND_CLASS_INDICATOR
    },
    {
      .CC = COMMAND_CLASS_ZWAVEPLUS_INFO
    }
  };

  uint8_t expected_unsecure_included_cc[] =
  {
    COMMAND_CLASS_ZWAVEPLUS_INFO,
    COMMAND_CLASS_SWITCH_BINARY_V2,
    COMMAND_CLASS_ASSOCIATION,
    COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION_V2,
    COMMAND_CLASS_ASSOCIATION_GRP_INFO,
    COMMAND_CLASS_TRANSPORT_SERVICE_V2,
    COMMAND_CLASS_VERSION,
    COMMAND_CLASS_MANUFACTURER_SPECIFIC,
    COMMAND_CLASS_DEVICE_RESET_LOCALLY,
    COMMAND_CLASS_INDICATOR,
    COMMAND_CLASS_POWERLEVEL,
    COMMAND_CLASS_SECURITY,
    COMMAND_CLASS_SECURITY_2,
    COMMAND_CLASS_SUPERVISION,
    COMMAND_CLASS_FIRMWARE_UPDATE_MD_V5
  };

  uint8_t expected_secure_included_unsecure_cc[] =
  {
    COMMAND_CLASS_ZWAVEPLUS_INFO,
    COMMAND_CLASS_TRANSPORT_SERVICE_V2,
    COMMAND_CLASS_SECURITY,
    COMMAND_CLASS_SECURITY_2,
    COMMAND_CLASS_SUPERVISION
  };

  uint8_t expected_secure_included_secure_cc[] =
  {
    COMMAND_CLASS_VERSION,
    COMMAND_CLASS_SWITCH_BINARY_V2,
    COMMAND_CLASS_ASSOCIATION,
    COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION_V2,
    COMMAND_CLASS_ASSOCIATION_GRP_INFO,
    COMMAND_CLASS_MANUFACTURER_SPECIFIC,
    COMMAND_CLASS_DEVICE_RESET_LOCALLY,
    COMMAND_CLASS_INDICATOR,
    COMMAND_CLASS_POWERLEVEL,
    COMMAND_CLASS_FIRMWARE_UPDATE_MD_V5
  };

  g_handler_maps = handler_maps;
  g_handler_maps_size = sizeof_array(handler_maps);

  zaf_config_get_requested_security_keys_ExpectAndReturn(SECURITY_KEY_S0_BIT | SECURITY_KEY_S2_UNAUTHENTICATED_BIT | SECURITY_KEY_S2_AUTHENTICATED_BIT);
  ZAF_CC_handler_map_size_ExpectAndReturn(sizeof_array(handler_maps));
  zaf_config_get_number_of_endpoints_IgnoreAndReturn(0);
  ZAF_CC_foreach_StubWithCallback(ZAF_CC_foreach_Callback);
  zaf_config_get_secure_only_IgnoreAndReturn(0);

  zafi_cc_list_generator_generate();

  zaf_config_get_number_of_endpoints_IgnoreAndReturn(0);

  zafi_cc_list_generator_get_lists(0, &unsecure_included_cc, &secure_included_unsecure_cc, &secure_included_secure_cc);

  validate_list(unsecure_included_cc, expected_unsecure_included_cc, sizeof_array(expected_unsecure_included_cc));
  validate_list(secure_included_unsecure_cc, expected_secure_included_unsecure_cc, sizeof_array(expected_secure_included_unsecure_cc));
  validate_list(secure_included_secure_cc, expected_secure_included_secure_cc, sizeof_array(expected_secure_included_secure_cc));

  zafi_cc_list_generator_destroy();
}

/**
 * Test CC List Generator With LED Bulb information
 */
void test_led_bulb(void)
{
  zaf_cc_list_t *unsecure_included_cc;
  zaf_cc_list_t *secure_included_unsecure_cc;
  zaf_cc_list_t *secure_included_secure_cc;

  CC_handler_map_latest_t handler_maps[] = {
    {
      .CC = COMMAND_CLASS_ASSOCIATION
    },
    {
      .CC = COMMAND_CLASS_ASSOCIATION_GRP_INFO
    },
    {
      .CC = COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION_V2
    },
    {
      .CC = COMMAND_CLASS_DEVICE_RESET_LOCALLY
    },
    {
      .CC = COMMAND_CLASS_FIRMWARE_UPDATE_MD_V2
    },
    {
      .CC = COMMAND_CLASS_INDICATOR
    },
    {
      .CC = COMMAND_CLASS_MANUFACTURER_SPECIFIC
    },
    {
      .CC = COMMAND_CLASS_POWERLEVEL
    },
    {
      .CC = COMMAND_CLASS_SUPERVISION
    },
    {
      .CC = COMMAND_CLASS_VERSION
    },
    {
      .CC = COMMAND_CLASS_ZWAVEPLUS_INFO_V2
    },
    {
      .CC = 0xFF
    },
    {
      .CC = COMMAND_CLASS_BASIC
    },
    {
      .CC = COMMAND_CLASS_SWITCH_COLOR
    },
    {
      .CC = COMMAND_CLASS_SWITCH_MULTILEVEL
    }
  };

  uint8_t expected_unsecure_included_cc[] =
  {
    COMMAND_CLASS_ZWAVEPLUS_INFO,
    COMMAND_CLASS_SWITCH_COLOR_V3,
    COMMAND_CLASS_SWITCH_MULTILEVEL_V4,
    COMMAND_CLASS_ASSOCIATION,
    COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION_V2,
    COMMAND_CLASS_ASSOCIATION_GRP_INFO,
    COMMAND_CLASS_TRANSPORT_SERVICE_V2,
    COMMAND_CLASS_VERSION,
    COMMAND_CLASS_MANUFACTURER_SPECIFIC,
    COMMAND_CLASS_DEVICE_RESET_LOCALLY,
    COMMAND_CLASS_INDICATOR,
    COMMAND_CLASS_POWERLEVEL,
    COMMAND_CLASS_SECURITY,
    COMMAND_CLASS_SECURITY_2,
    COMMAND_CLASS_SUPERVISION,
    COMMAND_CLASS_FIRMWARE_UPDATE_MD_V5
  };

  uint8_t expected_secure_included_unsecure_cc[] =
  {
    COMMAND_CLASS_ZWAVEPLUS_INFO,
    COMMAND_CLASS_TRANSPORT_SERVICE_V2,
    COMMAND_CLASS_SECURITY,
    COMMAND_CLASS_SECURITY_2,
    COMMAND_CLASS_SUPERVISION
  };

  uint8_t expected_secure_included_secure_cc[] =
  {
    COMMAND_CLASS_VERSION,
    COMMAND_CLASS_SWITCH_COLOR_V3,
    COMMAND_CLASS_SWITCH_MULTILEVEL_V4,
    COMMAND_CLASS_ASSOCIATION,
    COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION_V2,
    COMMAND_CLASS_ASSOCIATION_GRP_INFO,
    COMMAND_CLASS_MANUFACTURER_SPECIFIC,
    COMMAND_CLASS_DEVICE_RESET_LOCALLY,
    COMMAND_CLASS_INDICATOR,
    COMMAND_CLASS_POWERLEVEL,
    COMMAND_CLASS_FIRMWARE_UPDATE_MD_V5
  };

  g_handler_maps = handler_maps;
  g_handler_maps_size = sizeof_array(handler_maps);

  zaf_config_get_requested_security_keys_ExpectAndReturn(SECURITY_KEY_S0_BIT | SECURITY_KEY_S2_UNAUTHENTICATED_BIT | SECURITY_KEY_S2_AUTHENTICATED_BIT);
  ZAF_CC_handler_map_size_ExpectAndReturn(sizeof_array(handler_maps));
  zaf_config_get_number_of_endpoints_IgnoreAndReturn(0);
  ZAF_CC_foreach_StubWithCallback(ZAF_CC_foreach_Callback);
  zaf_config_get_secure_only_IgnoreAndReturn(0);

  zafi_cc_list_generator_generate();

  zaf_config_get_number_of_endpoints_IgnoreAndReturn(0);

  zafi_cc_list_generator_get_lists(0, &unsecure_included_cc, &secure_included_unsecure_cc, &secure_included_secure_cc);

  validate_list(unsecure_included_cc, expected_unsecure_included_cc, sizeof_array(expected_unsecure_included_cc));
  validate_list(secure_included_unsecure_cc, expected_secure_included_unsecure_cc, sizeof_array(expected_secure_included_unsecure_cc));
  validate_list(secure_included_secure_cc, expected_secure_included_secure_cc, sizeof_array(expected_secure_included_secure_cc));

  zafi_cc_list_generator_destroy();
}

/**
 * Test CC List Generator With DoorLockKeyPad information
 */
void test_door_lock_keypad(void)
{
  zaf_cc_list_t *unsecure_included_cc;
  zaf_cc_list_t *secure_included_unsecure_cc;
  zaf_cc_list_t *secure_included_secure_cc;

  CC_handler_map_latest_t handler_maps[] = {
    {
      .CC = COMMAND_CLASS_ASSOCIATION
    },
    {
      .CC = COMMAND_CLASS_ASSOCIATION_GRP_INFO
    },
    {
      .CC = COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION_V2
    },
    {
      .CC = COMMAND_CLASS_DEVICE_RESET_LOCALLY
    },
    {
      .CC = COMMAND_CLASS_FIRMWARE_UPDATE_MD_V2
    },
    {
      .CC = COMMAND_CLASS_INDICATOR
    },
    {
      .CC = COMMAND_CLASS_MANUFACTURER_SPECIFIC
    },
    {
      .CC = COMMAND_CLASS_POWERLEVEL
    },
    {
      .CC = COMMAND_CLASS_SUPERVISION
    },
    {
      .CC = COMMAND_CLASS_VERSION
    },
    {
      .CC = COMMAND_CLASS_ZWAVEPLUS_INFO_V2
    },
    {
      .CC = 0xFF
    },
    {
      .CC = COMMAND_CLASS_BASIC
    },
    {
      .CC = COMMAND_CLASS_BATTERY
    },
    {
      .CC = COMMAND_CLASS_DOOR_LOCK
    },
    {
      .CC = COMMAND_CLASS_USER_CODE
    }
  };

  uint8_t expected_unsecure_included_cc[] =
  {
    COMMAND_CLASS_ZWAVEPLUS_INFO,
    COMMAND_CLASS_TRANSPORT_SERVICE_V2,
    COMMAND_CLASS_SECURITY,
    COMMAND_CLASS_SECURITY_2,
    COMMAND_CLASS_SUPERVISION
  };

  uint8_t expected_secure_included_unsecure_cc[] =
  {
    COMMAND_CLASS_ZWAVEPLUS_INFO,
    COMMAND_CLASS_TRANSPORT_SERVICE_V2,
    COMMAND_CLASS_SECURITY,
    COMMAND_CLASS_SECURITY_2,
    COMMAND_CLASS_SUPERVISION
  };

  uint8_t expected_secure_included_secure_cc[] =
  {
    COMMAND_CLASS_VERSION,
    COMMAND_CLASS_MANUFACTURER_SPECIFIC,
    COMMAND_CLASS_DEVICE_RESET_LOCALLY,
    COMMAND_CLASS_INDICATOR,
    COMMAND_CLASS_POWERLEVEL,
    COMMAND_CLASS_BATTERY,
    COMMAND_CLASS_DOOR_LOCK,
    COMMAND_CLASS_USER_CODE,
    COMMAND_CLASS_ASSOCIATION_V2,
    COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION_V2,
    COMMAND_CLASS_ASSOCIATION_GRP_INFO,
    COMMAND_CLASS_FIRMWARE_UPDATE_MD_V5
  };

  g_handler_maps = handler_maps;
  g_handler_maps_size = sizeof_array(handler_maps);

  zaf_config_get_requested_security_keys_ExpectAndReturn(SECURITY_KEY_S0_BIT | SECURITY_KEY_S2_ACCESS_BIT);
  ZAF_CC_handler_map_size_ExpectAndReturn(sizeof_array(handler_maps));
  zaf_config_get_number_of_endpoints_IgnoreAndReturn(0);
  ZAF_CC_foreach_StubWithCallback(ZAF_CC_foreach_Callback);
  zaf_config_get_secure_only_IgnoreAndReturn(1);

  zafi_cc_list_generator_generate();

  zaf_config_get_number_of_endpoints_IgnoreAndReturn(0);

  zafi_cc_list_generator_get_lists(0, &unsecure_included_cc, &secure_included_unsecure_cc, &secure_included_secure_cc);

  validate_list(unsecure_included_cc, expected_unsecure_included_cc, sizeof_array(expected_unsecure_included_cc));
  validate_list(secure_included_unsecure_cc, expected_secure_included_unsecure_cc, sizeof_array(expected_secure_included_unsecure_cc));
  validate_list(secure_included_secure_cc, expected_secure_included_secure_cc, sizeof_array(expected_secure_included_secure_cc));

  zafi_cc_list_generator_destroy();
}

/**
 * Test CC List Generator With PowerStrip information
 * COMMAND_CLASS_SWITCH_BINARY...COMMAND_CLASS_SWITCH_MULTILEVEL
 */
void test_power_strip_1(void)
{
  zaf_cc_list_t *unsecure_included_cc;
  zaf_cc_list_t *secure_included_unsecure_cc;
  zaf_cc_list_t *secure_included_secure_cc;

  CC_handler_map_latest_t handler_maps[] = {
    {
      .CC = COMMAND_CLASS_ASSOCIATION
    },
    {
      .CC = COMMAND_CLASS_ASSOCIATION_GRP_INFO
    },
    {
      .CC = COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION_V2
    },
    {
      .CC = COMMAND_CLASS_DEVICE_RESET_LOCALLY
    },
    {
      .CC = COMMAND_CLASS_FIRMWARE_UPDATE_MD_V2
    },
    {
      .CC = COMMAND_CLASS_INDICATOR
    },
    {
      .CC = COMMAND_CLASS_MANUFACTURER_SPECIFIC
    },
    {
      .CC = COMMAND_CLASS_POWERLEVEL
    },
    {
      .CC = COMMAND_CLASS_SUPERVISION
    },
    {
      .CC = COMMAND_CLASS_VERSION
    },
    {
      .CC = COMMAND_CLASS_ZWAVEPLUS_INFO_V2
    },
    {
      .CC = 0xFF
    },
    {
      .CC = COMMAND_CLASS_BASIC
    },
    {
      .CC = COMMAND_CLASS_SWITCH_BINARY
    },
    {
      .CC = COMMAND_CLASS_MULTI_CHANNEL_V3
    },
    {
      .CC = COMMAND_CLASS_SWITCH_MULTILEVEL
    },
    {
      .CC = COMMAND_CLASS_NOTIFICATION_V8
    },
    {
      .CC = COMMAND_CLASS_SECURITY_2
    },
    {
      .CC = COMMAND_CLASS_SECURITY
    }
  };

  zaf_cc_config_t switch_binary = {
    .endpoint = 1
  };

  zaf_cc_config_t switch_multilevel = {
    .endpoint = 2
  };

  zaf_cc_config_t notification_1 = {
    .endpoint = 2
  };

  zaf_cc_config_t notification_2 =  {
    .endpoint = 1
  };

  zaf_cc_config_entry_latest_t config_entries[] = {
    {
      .command_class = COMMAND_CLASS_SWITCH_MULTILEVEL,
      .p_cc_configuration = &switch_multilevel,
    },
    {
      .command_class = COMMAND_CLASS_SWITCH_BINARY,
      .p_cc_configuration = &switch_binary,
    },
    {
      .command_class = COMMAND_CLASS_NOTIFICATION_V8,
      .p_cc_configuration = &notification_1,
    },
    {
      .command_class = COMMAND_CLASS_NOTIFICATION_V8,
      .p_cc_configuration = &notification_2,
    }
  };

  uint8_t expected_unsecure_included_cc[] =
  {
    COMMAND_CLASS_ZWAVEPLUS_INFO,
    COMMAND_CLASS_SWITCH_BINARY_V2,
    COMMAND_CLASS_ASSOCIATION,
    COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION_V2,
    COMMAND_CLASS_ASSOCIATION_GRP_INFO,
    COMMAND_CLASS_NOTIFICATION_V3,
    COMMAND_CLASS_TRANSPORT_SERVICE_V2,
    COMMAND_CLASS_VERSION,
    COMMAND_CLASS_MANUFACTURER_SPECIFIC,
    COMMAND_CLASS_DEVICE_RESET_LOCALLY,
    COMMAND_CLASS_INDICATOR,
    COMMAND_CLASS_POWERLEVEL,
    COMMAND_CLASS_SECURITY,
    COMMAND_CLASS_SECURITY_2,
    COMMAND_CLASS_MULTI_CHANNEL_V4,
    COMMAND_CLASS_SUPERVISION,
    COMMAND_CLASS_FIRMWARE_UPDATE_MD_V5
  };

  uint8_t expected_secure_included_unsecure_cc[] =
  {
    COMMAND_CLASS_ZWAVEPLUS_INFO,
    COMMAND_CLASS_TRANSPORT_SERVICE_V2,
    COMMAND_CLASS_SECURITY,
    COMMAND_CLASS_SECURITY_2,
    COMMAND_CLASS_SUPERVISION
  };

  uint8_t expected_secure_included_secure_cc[] =
  {
    COMMAND_CLASS_SWITCH_BINARY_V2,
    COMMAND_CLASS_ASSOCIATION,
    COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION_V2,
    COMMAND_CLASS_ASSOCIATION_GRP_INFO,
    COMMAND_CLASS_NOTIFICATION_V3,
    COMMAND_CLASS_VERSION,
    COMMAND_CLASS_MANUFACTURER_SPECIFIC,
    COMMAND_CLASS_DEVICE_RESET_LOCALLY,
    COMMAND_CLASS_INDICATOR,
    COMMAND_CLASS_POWERLEVEL,
    COMMAND_CLASS_MULTI_CHANNEL_V4,
    COMMAND_CLASS_FIRMWARE_UPDATE_MD_V5
  };

  g_handler_maps = handler_maps;
  g_handler_maps_size = sizeof_array(handler_maps);
  g_config_entries = config_entries;
  g_config_entries_size = sizeof_array(config_entries);

  zaf_config_get_requested_security_keys_ExpectAndReturn(SECURITY_KEY_S0_BIT | SECURITY_KEY_S2_UNAUTHENTICATED_BIT | SECURITY_KEY_S2_AUTHENTICATED_BIT);
  ZAF_CC_handler_map_size_ExpectAndReturn(sizeof_array(handler_maps));
  zaf_config_get_number_of_endpoints_IgnoreAndReturn(2);
  ZAF_CC_config_foreach_StubWithCallback(ZAF_CC_config_foreach_Callback);
  ZAF_CC_foreach_StubWithCallback(ZAF_CC_foreach_Callback);
  zaf_config_get_secure_only_IgnoreAndReturn(0);

  zafi_cc_list_generator_generate();

  zaf_config_get_number_of_endpoints_IgnoreAndReturn(2);

  zafi_cc_list_generator_get_lists(0, &unsecure_included_cc, &secure_included_unsecure_cc, &secure_included_secure_cc);

  validate_list(unsecure_included_cc, expected_unsecure_included_cc, sizeof_array(expected_unsecure_included_cc));
  validate_list(secure_included_unsecure_cc, expected_secure_included_unsecure_cc, sizeof_array(expected_secure_included_unsecure_cc));
  validate_list(secure_included_secure_cc, expected_secure_included_secure_cc, sizeof_array(expected_secure_included_secure_cc));

  uint8_t expected_ep1_unsecure_included_cc[] =
  {
    COMMAND_CLASS_ZWAVEPLUS_INFO,
    COMMAND_CLASS_SWITCH_BINARY_V2,
    COMMAND_CLASS_ASSOCIATION,
    COMMAND_CLASS_ASSOCIATION_GRP_INFO,
    COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION_V2,
    COMMAND_CLASS_NOTIFICATION_V3,
    COMMAND_CLASS_SUPERVISION,
    COMMAND_CLASS_SECURITY,
    COMMAND_CLASS_SECURITY_2
  };

  uint8_t expected_ep1_secure_included_unsecure_cc[] =
  {
    COMMAND_CLASS_ZWAVEPLUS_INFO,
    COMMAND_CLASS_SUPERVISION,
    COMMAND_CLASS_SECURITY,
    COMMAND_CLASS_SECURITY_2
  };

  uint8_t expected_ep1_secure_included_secure_cc[] =
  {
    COMMAND_CLASS_SWITCH_BINARY_V2,
    COMMAND_CLASS_ASSOCIATION,
    COMMAND_CLASS_ASSOCIATION_GRP_INFO,
    COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION_V2,
    COMMAND_CLASS_NOTIFICATION_V3
  };

  zafi_cc_list_generator_get_lists(1, &unsecure_included_cc, &secure_included_unsecure_cc, &secure_included_secure_cc);

  validate_list(unsecure_included_cc, expected_ep1_unsecure_included_cc, sizeof_array(expected_ep1_unsecure_included_cc));
  validate_list(secure_included_unsecure_cc, expected_ep1_secure_included_unsecure_cc, sizeof_array(expected_ep1_secure_included_unsecure_cc));
  validate_list(secure_included_secure_cc, expected_ep1_secure_included_secure_cc, sizeof_array(expected_ep1_secure_included_secure_cc));

  uint8_t expected_ep2_unsecure_included_cc[] =
  {
    COMMAND_CLASS_ZWAVEPLUS_INFO,
    COMMAND_CLASS_SWITCH_MULTILEVEL,
    COMMAND_CLASS_ASSOCIATION,
    COMMAND_CLASS_ASSOCIATION_GRP_INFO,
    COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION_V2,
    COMMAND_CLASS_NOTIFICATION_V3,
    COMMAND_CLASS_SUPERVISION,
    COMMAND_CLASS_SECURITY,
    COMMAND_CLASS_SECURITY_2
  };

  uint8_t expected_ep2_secure_included_unsecure_cc[] =
  {
    COMMAND_CLASS_ZWAVEPLUS_INFO,
    COMMAND_CLASS_SUPERVISION,
    COMMAND_CLASS_SECURITY,
    COMMAND_CLASS_SECURITY_2
  };

  uint8_t expected_ep2_secure_included_secure_cc[] =
  {
    COMMAND_CLASS_SWITCH_MULTILEVEL,
    COMMAND_CLASS_ASSOCIATION,
    COMMAND_CLASS_ASSOCIATION_GRP_INFO,
    COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION_V2,
    COMMAND_CLASS_NOTIFICATION_V3
  };

  zafi_cc_list_generator_get_lists(2, &unsecure_included_cc, &secure_included_unsecure_cc, &secure_included_secure_cc);

  validate_list(unsecure_included_cc, expected_ep2_unsecure_included_cc, sizeof_array(expected_ep2_unsecure_included_cc));
  validate_list(secure_included_unsecure_cc, expected_ep2_secure_included_unsecure_cc, sizeof_array(expected_ep2_secure_included_unsecure_cc));
  validate_list(secure_included_secure_cc, expected_ep2_secure_included_secure_cc, sizeof_array(expected_ep2_secure_included_secure_cc));

  zafi_cc_list_generator_get_lists(3, &unsecure_included_cc, &secure_included_unsecure_cc, &secure_included_secure_cc);

  TEST_ASSERT_NULL_MESSAGE(unsecure_included_cc, "unsecure_included_cc is not null");
  TEST_ASSERT_NULL_MESSAGE(secure_included_unsecure_cc, "secure_included_unsecure_cc is not null");
  TEST_ASSERT_NULL_MESSAGE(secure_included_secure_cc, "secure_included_secure_cc is not null");

  zafi_cc_list_generator_destroy();
}

/**
 * Test CC List Generator With PowerStrip information
 * COMMAND_CLASS_SWITCH_MULTILEVEL...COMMAND_CLASS_SWITCH_BINARY
 */
void test_power_strip_2(void)
{
  zaf_cc_list_t *unsecure_included_cc;
  zaf_cc_list_t *secure_included_unsecure_cc;
  zaf_cc_list_t *secure_included_secure_cc;

  CC_handler_map_latest_t handler_maps[] = {
    {
      .CC = COMMAND_CLASS_ASSOCIATION
    },
    {
      .CC = COMMAND_CLASS_ASSOCIATION_GRP_INFO
    },
    {
      .CC = COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION_V2
    },
    {
      .CC = COMMAND_CLASS_DEVICE_RESET_LOCALLY
    },
    {
      .CC = COMMAND_CLASS_FIRMWARE_UPDATE_MD_V2
    },
    {
      .CC = COMMAND_CLASS_INDICATOR
    },
    {
      .CC = COMMAND_CLASS_MANUFACTURER_SPECIFIC
    },
    {
      .CC = COMMAND_CLASS_POWERLEVEL
    },
    {
      .CC = COMMAND_CLASS_SUPERVISION
    },
    {
      .CC = COMMAND_CLASS_VERSION
    },
    {
      .CC = COMMAND_CLASS_ZWAVEPLUS_INFO_V2
    },
    {
      .CC = 0xFF
    },
    {
      .CC = COMMAND_CLASS_BASIC
    },
    {
      .CC = COMMAND_CLASS_SWITCH_MULTILEVEL
    },
    {
      .CC = COMMAND_CLASS_MULTI_CHANNEL_V3
    },
    {
      .CC = COMMAND_CLASS_SWITCH_BINARY
    },
    {
      .CC = COMMAND_CLASS_NOTIFICATION_V8
    },
    {
      .CC = COMMAND_CLASS_SECURITY_2
    },
    {
      .CC = COMMAND_CLASS_SECURITY
    }
  };

  zaf_cc_config_t switch_binary = {
    .endpoint = 1
  };

  zaf_cc_config_t switch_multilevel = {
    .endpoint = 2
  };

  zaf_cc_config_t notification_1 = {
    .endpoint = 2
  };

  zaf_cc_config_t notification_2 =  {
    .endpoint = 1
  };

  zaf_cc_config_entry_latest_t config_entries[] = {
    {
      .command_class = COMMAND_CLASS_SWITCH_MULTILEVEL,
      .p_cc_configuration = &switch_multilevel,
    },
    {
      .command_class = COMMAND_CLASS_SWITCH_BINARY,
      .p_cc_configuration = &switch_binary,
    },
    {
      .command_class = COMMAND_CLASS_NOTIFICATION_V8,
      .p_cc_configuration = &notification_1,
    },
    {
      .command_class = COMMAND_CLASS_NOTIFICATION_V8,
      .p_cc_configuration = &notification_2,
    }
  };

  uint8_t expected_unsecure_included_cc[] =
  {
    COMMAND_CLASS_ZWAVEPLUS_INFO,
    COMMAND_CLASS_SWITCH_BINARY_V2,
    COMMAND_CLASS_ASSOCIATION,
    COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION_V2,
    COMMAND_CLASS_ASSOCIATION_GRP_INFO,
    COMMAND_CLASS_NOTIFICATION_V3,
    COMMAND_CLASS_TRANSPORT_SERVICE_V2,
    COMMAND_CLASS_VERSION,
    COMMAND_CLASS_MANUFACTURER_SPECIFIC,
    COMMAND_CLASS_DEVICE_RESET_LOCALLY,
    COMMAND_CLASS_INDICATOR,
    COMMAND_CLASS_POWERLEVEL,
    COMMAND_CLASS_SECURITY,
    COMMAND_CLASS_SECURITY_2,
    COMMAND_CLASS_MULTI_CHANNEL_V4,
    COMMAND_CLASS_SUPERVISION,
    COMMAND_CLASS_FIRMWARE_UPDATE_MD_V5
  };

  uint8_t expected_secure_included_unsecure_cc[] =
  {
    COMMAND_CLASS_ZWAVEPLUS_INFO,
    COMMAND_CLASS_TRANSPORT_SERVICE_V2,
    COMMAND_CLASS_SECURITY,
    COMMAND_CLASS_SECURITY_2,
    COMMAND_CLASS_SUPERVISION
  };

  uint8_t expected_secure_included_secure_cc[] =
  {
    COMMAND_CLASS_SWITCH_BINARY_V2,
    COMMAND_CLASS_ASSOCIATION,
    COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION_V2,
    COMMAND_CLASS_ASSOCIATION_GRP_INFO,
    COMMAND_CLASS_NOTIFICATION_V3,
    COMMAND_CLASS_VERSION,
    COMMAND_CLASS_MANUFACTURER_SPECIFIC,
    COMMAND_CLASS_DEVICE_RESET_LOCALLY,
    COMMAND_CLASS_INDICATOR,
    COMMAND_CLASS_POWERLEVEL,
    COMMAND_CLASS_MULTI_CHANNEL_V4,
    COMMAND_CLASS_FIRMWARE_UPDATE_MD_V5
  };

  g_handler_maps = handler_maps;
  g_handler_maps_size = sizeof_array(handler_maps);
  g_config_entries = config_entries;
  g_config_entries_size = sizeof_array(config_entries);

  zaf_config_get_requested_security_keys_ExpectAndReturn(SECURITY_KEY_S0_BIT | SECURITY_KEY_S2_UNAUTHENTICATED_BIT | SECURITY_KEY_S2_AUTHENTICATED_BIT);
  ZAF_CC_handler_map_size_ExpectAndReturn(sizeof_array(handler_maps));
  zaf_config_get_number_of_endpoints_IgnoreAndReturn(2);
  ZAF_CC_config_foreach_StubWithCallback(ZAF_CC_config_foreach_Callback);
  ZAF_CC_foreach_StubWithCallback(ZAF_CC_foreach_Callback);
  zaf_config_get_secure_only_IgnoreAndReturn(0);

  zafi_cc_list_generator_generate();

  zafi_cc_list_generator_get_lists(0, &unsecure_included_cc, &secure_included_unsecure_cc, &secure_included_secure_cc);

  validate_list(unsecure_included_cc, expected_unsecure_included_cc, sizeof_array(expected_unsecure_included_cc));
  validate_list(secure_included_unsecure_cc, expected_secure_included_unsecure_cc, sizeof_array(expected_secure_included_unsecure_cc));
  validate_list(secure_included_secure_cc, expected_secure_included_secure_cc, sizeof_array(expected_secure_included_secure_cc));

  uint8_t expected_ep1_unsecure_included_cc[] =
  {
    COMMAND_CLASS_ZWAVEPLUS_INFO,
    COMMAND_CLASS_SWITCH_BINARY_V2,
    COMMAND_CLASS_ASSOCIATION,
    COMMAND_CLASS_ASSOCIATION_GRP_INFO,
    COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION_V2,
    COMMAND_CLASS_NOTIFICATION_V3,
    COMMAND_CLASS_SUPERVISION,
    COMMAND_CLASS_SECURITY,
    COMMAND_CLASS_SECURITY_2
  };

  uint8_t expected_ep1_secure_included_unsecure_cc[] =
  {
    COMMAND_CLASS_ZWAVEPLUS_INFO,
    COMMAND_CLASS_SUPERVISION,
    COMMAND_CLASS_SECURITY,
    COMMAND_CLASS_SECURITY_2
  };

  uint8_t expected_ep1_secure_included_secure_cc[] =
  {
    COMMAND_CLASS_SWITCH_BINARY_V2,
    COMMAND_CLASS_ASSOCIATION,
    COMMAND_CLASS_ASSOCIATION_GRP_INFO,
    COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION_V2,
    COMMAND_CLASS_NOTIFICATION_V3
  };

  zafi_cc_list_generator_get_lists(1, &unsecure_included_cc, &secure_included_unsecure_cc, &secure_included_secure_cc);

  validate_list(unsecure_included_cc, expected_ep1_unsecure_included_cc, sizeof_array(expected_ep1_unsecure_included_cc));
  validate_list(secure_included_unsecure_cc, expected_ep1_secure_included_unsecure_cc, sizeof_array(expected_ep1_secure_included_unsecure_cc));
  validate_list(secure_included_secure_cc, expected_ep1_secure_included_secure_cc, sizeof_array(expected_ep1_secure_included_secure_cc));

  uint8_t expected_ep2_unsecure_included_cc[] =
  {
    COMMAND_CLASS_ZWAVEPLUS_INFO,
    COMMAND_CLASS_SWITCH_MULTILEVEL,
    COMMAND_CLASS_ASSOCIATION,
    COMMAND_CLASS_ASSOCIATION_GRP_INFO,
    COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION_V2,
    COMMAND_CLASS_NOTIFICATION_V3,
    COMMAND_CLASS_SUPERVISION,
    COMMAND_CLASS_SECURITY,
    COMMAND_CLASS_SECURITY_2
  };

  uint8_t expected_ep2_secure_included_unsecure_cc[] =
  {
    COMMAND_CLASS_ZWAVEPLUS_INFO,
    COMMAND_CLASS_SUPERVISION,
    COMMAND_CLASS_SECURITY,
    COMMAND_CLASS_SECURITY_2
  };

  uint8_t expected_ep2_secure_included_secure_cc[] =
  {
    COMMAND_CLASS_SWITCH_MULTILEVEL,
    COMMAND_CLASS_ASSOCIATION,
    COMMAND_CLASS_ASSOCIATION_GRP_INFO,
    COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION_V2,
    COMMAND_CLASS_NOTIFICATION_V3
  };

  zafi_cc_list_generator_get_lists(2, &unsecure_included_cc, &secure_included_unsecure_cc, &secure_included_secure_cc);

  validate_list(unsecure_included_cc, expected_ep2_unsecure_included_cc, sizeof_array(expected_ep2_unsecure_included_cc));
  validate_list(secure_included_unsecure_cc, expected_ep2_secure_included_unsecure_cc, sizeof_array(expected_ep2_secure_included_unsecure_cc));
  validate_list(secure_included_secure_cc, expected_ep2_secure_included_secure_cc, sizeof_array(expected_ep2_secure_included_secure_cc));

  zafi_cc_list_generator_get_lists(3, &unsecure_included_cc, &secure_included_unsecure_cc, &secure_included_secure_cc);

  TEST_ASSERT_NULL_MESSAGE(unsecure_included_cc, "unsecure_included_cc is not null");
  TEST_ASSERT_NULL_MESSAGE(secure_included_unsecure_cc, "secure_included_unsecure_cc is not null");
  TEST_ASSERT_NULL_MESSAGE(secure_included_secure_cc, "secure_included_secure_cc is not null");

  zafi_cc_list_generator_destroy();
}

/**
 * Test CC List Generator With Sensor PIR information
 */
void test_sensor_pir(void)
{
  zaf_cc_list_t *unsecure_included_cc;
  zaf_cc_list_t *secure_included_unsecure_cc;
  zaf_cc_list_t *secure_included_secure_cc;

  CC_handler_map_latest_t handler_maps[] = {
    {
      .CC = COMMAND_CLASS_ASSOCIATION
    },
    {
      .CC = COMMAND_CLASS_ASSOCIATION_GRP_INFO
    },
    {
      .CC = COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION_V2
    },
    {
      .CC = COMMAND_CLASS_DEVICE_RESET_LOCALLY
    },
    {
      .CC = COMMAND_CLASS_FIRMWARE_UPDATE_MD_V2
    },
    {
      .CC = COMMAND_CLASS_INDICATOR
    },
    {
      .CC = COMMAND_CLASS_MANUFACTURER_SPECIFIC
    },
    {
      .CC = COMMAND_CLASS_POWERLEVEL
    },
    {
      .CC = COMMAND_CLASS_SUPERVISION
    },
    {
      .CC = COMMAND_CLASS_VERSION
    },
    {
      .CC = COMMAND_CLASS_ZWAVEPLUS_INFO_V2
    },
    {
      .CC = 0xFF
    },
    {
      .CC = COMMAND_CLASS_BATTERY
    },
    {
      .CC = COMMAND_CLASS_NOTIFICATION_V8
    },
    {
      .CC = COMMAND_CLASS_WAKE_UP
    }
  };

  uint8_t expected_unsecure_included_cc[] =
  {
    COMMAND_CLASS_ZWAVEPLUS_INFO,
    COMMAND_CLASS_ASSOCIATION,
    COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION_V2,
    COMMAND_CLASS_ASSOCIATION_GRP_INFO,
    COMMAND_CLASS_TRANSPORT_SERVICE_V2,
    COMMAND_CLASS_VERSION,
    COMMAND_CLASS_MANUFACTURER_SPECIFIC,
    COMMAND_CLASS_DEVICE_RESET_LOCALLY,
    COMMAND_CLASS_INDICATOR,
    COMMAND_CLASS_POWERLEVEL,
    COMMAND_CLASS_BATTERY,
    COMMAND_CLASS_SECURITY_2,
    COMMAND_CLASS_NOTIFICATION_V3,
    COMMAND_CLASS_WAKE_UP,
    COMMAND_CLASS_SUPERVISION,
    COMMAND_CLASS_FIRMWARE_UPDATE_MD_V5
  };

  uint8_t expected_secure_included_unsecure_cc[] =
  {
    COMMAND_CLASS_ZWAVEPLUS_INFO,
    COMMAND_CLASS_TRANSPORT_SERVICE_V2,
    COMMAND_CLASS_SECURITY_2,
    COMMAND_CLASS_SUPERVISION
  };

  uint8_t expected_secure_included_secure_cc[] =
  {
    COMMAND_CLASS_VERSION,
    COMMAND_CLASS_ASSOCIATION,
    COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION_V2,
    COMMAND_CLASS_ASSOCIATION_GRP_INFO,
    COMMAND_CLASS_MANUFACTURER_SPECIFIC,
    COMMAND_CLASS_DEVICE_RESET_LOCALLY,
    COMMAND_CLASS_INDICATOR,
    COMMAND_CLASS_POWERLEVEL,
    COMMAND_CLASS_BATTERY,
    COMMAND_CLASS_NOTIFICATION_V3,
    COMMAND_CLASS_WAKE_UP,
    COMMAND_CLASS_FIRMWARE_UPDATE_MD_V5
  };

  g_handler_maps = handler_maps;
  g_handler_maps_size = sizeof_array(handler_maps);

  zaf_config_get_requested_security_keys_ExpectAndReturn(SECURITY_KEY_S2_UNAUTHENTICATED_BIT | SECURITY_KEY_S2_AUTHENTICATED_BIT);
  ZAF_CC_handler_map_size_ExpectAndReturn(sizeof_array(handler_maps));
  zaf_config_get_number_of_endpoints_IgnoreAndReturn(0);
  ZAF_CC_foreach_StubWithCallback(ZAF_CC_foreach_Callback);
  zaf_config_get_secure_only_IgnoreAndReturn(0);

  zafi_cc_list_generator_generate();

  zaf_config_get_number_of_endpoints_IgnoreAndReturn(0);

  zafi_cc_list_generator_get_lists(0, &unsecure_included_cc, &secure_included_unsecure_cc, &secure_included_secure_cc);

  validate_list(unsecure_included_cc, expected_unsecure_included_cc, sizeof_array(expected_unsecure_included_cc));
  validate_list(secure_included_unsecure_cc, expected_secure_included_unsecure_cc, sizeof_array(expected_secure_included_unsecure_cc));
  validate_list(secure_included_secure_cc, expected_secure_included_secure_cc, sizeof_array(expected_secure_included_secure_cc));

  zafi_cc_list_generator_destroy();
}

/**
 * Test CC List Generator With Wall Controller information
 */
void test_wall_controller(void)
{
  zaf_cc_list_t *unsecure_included_cc;
  zaf_cc_list_t *secure_included_unsecure_cc;
  zaf_cc_list_t *secure_included_secure_cc;

  CC_handler_map_latest_t handler_maps[] = {
    {
      .CC = COMMAND_CLASS_ASSOCIATION
    },
    {
      .CC = COMMAND_CLASS_ASSOCIATION_GRP_INFO
    },
    {
      .CC = COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION_V2
    },
    {
      .CC = COMMAND_CLASS_DEVICE_RESET_LOCALLY
    },
    {
      .CC = COMMAND_CLASS_FIRMWARE_UPDATE_MD_V2
    },
    {
      .CC = COMMAND_CLASS_INDICATOR
    },
    {
      .CC = COMMAND_CLASS_MANUFACTURER_SPECIFIC
    },
    {
      .CC = COMMAND_CLASS_POWERLEVEL
    },
    {
      .CC = COMMAND_CLASS_SUPERVISION
    },
    {
      .CC = COMMAND_CLASS_VERSION
    },
    {
      .CC = COMMAND_CLASS_ZWAVEPLUS_INFO_V2
    },
    {
      .CC = 0xFF
    },
    {
      .CC = COMMAND_CLASS_CENTRAL_SCENE
    },
    {
      .CC = COMMAND_CLASS_SECURITY_2
    },
    {
      .CC = COMMAND_CLASS_SECURITY
    }
  };

  uint8_t expected_unsecure_included_cc[] =
  {
    COMMAND_CLASS_ZWAVEPLUS_INFO,
    COMMAND_CLASS_ASSOCIATION_V2,
    COMMAND_CLASS_ASSOCIATION_GRP_INFO,
    COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION_V2,
    COMMAND_CLASS_TRANSPORT_SERVICE_V2,
    COMMAND_CLASS_VERSION,
    COMMAND_CLASS_MANUFACTURER_SPECIFIC,
    COMMAND_CLASS_DEVICE_RESET_LOCALLY,
    COMMAND_CLASS_INDICATOR,
    COMMAND_CLASS_POWERLEVEL,
    COMMAND_CLASS_SECURITY,
    COMMAND_CLASS_SECURITY_2,
    COMMAND_CLASS_SUPERVISION,
    COMMAND_CLASS_CENTRAL_SCENE_V2,
    COMMAND_CLASS_FIRMWARE_UPDATE_MD_V5,
  };

  uint8_t expected_secure_included_unsecure_cc[] =
  {
    COMMAND_CLASS_ZWAVEPLUS_INFO,
    COMMAND_CLASS_TRANSPORT_SERVICE_V2,
    COMMAND_CLASS_SECURITY,
    COMMAND_CLASS_SECURITY_2,
    COMMAND_CLASS_SUPERVISION
  };

  uint8_t expected_secure_included_secure_cc[] =
  {
    COMMAND_CLASS_VERSION,
    COMMAND_CLASS_ASSOCIATION_V2,
    COMMAND_CLASS_ASSOCIATION_GRP_INFO,
    COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION_V2,
    COMMAND_CLASS_MANUFACTURER_SPECIFIC,
    COMMAND_CLASS_DEVICE_RESET_LOCALLY,
    COMMAND_CLASS_INDICATOR,
    COMMAND_CLASS_POWERLEVEL,
    COMMAND_CLASS_CENTRAL_SCENE_V2,
    COMMAND_CLASS_FIRMWARE_UPDATE_MD_V5
  };

  g_handler_maps = handler_maps;
  g_handler_maps_size = sizeof_array(handler_maps);

  zaf_config_get_requested_security_keys_ExpectAndReturn(SECURITY_KEY_S0_BIT | SECURITY_KEY_S2_UNAUTHENTICATED_BIT | SECURITY_KEY_S2_AUTHENTICATED_BIT);
  ZAF_CC_handler_map_size_ExpectAndReturn(sizeof_array(handler_maps));
  zaf_config_get_number_of_endpoints_IgnoreAndReturn(0);
  ZAF_CC_foreach_StubWithCallback(ZAF_CC_foreach_Callback);
  zaf_config_get_secure_only_IgnoreAndReturn(0);

  zafi_cc_list_generator_generate();

  zaf_config_get_number_of_endpoints_IgnoreAndReturn(0);

  zafi_cc_list_generator_get_lists(0, &unsecure_included_cc, &secure_included_unsecure_cc, &secure_included_secure_cc);

  validate_list(unsecure_included_cc, expected_unsecure_included_cc, sizeof_array(expected_unsecure_included_cc));
  validate_list(secure_included_unsecure_cc, expected_secure_included_unsecure_cc, sizeof_array(expected_secure_included_unsecure_cc));
  validate_list(secure_included_secure_cc, expected_secure_included_secure_cc, sizeof_array(expected_secure_included_secure_cc));

  zafi_cc_list_generator_destroy();
}

/**
 * Test CC List Generator With Multilevel Sensor information
 */
void test_multilevel_sensor(void)
{
  zaf_cc_list_t *unsecure_included_cc;
  zaf_cc_list_t *secure_included_unsecure_cc;
  zaf_cc_list_t *secure_included_secure_cc;

  CC_handler_map_latest_t handler_maps[] = {
    {
      .CC = COMMAND_CLASS_ASSOCIATION
    },
    {
      .CC = COMMAND_CLASS_ASSOCIATION_GRP_INFO
    },
    {
      .CC = COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION_V2
    },
    {
      .CC = COMMAND_CLASS_DEVICE_RESET_LOCALLY
    },
    {
      .CC = COMMAND_CLASS_FIRMWARE_UPDATE_MD_V2
    },
    {
      .CC = COMMAND_CLASS_INDICATOR
    },
    {
      .CC = COMMAND_CLASS_MANUFACTURER_SPECIFIC
    },
    {
      .CC = COMMAND_CLASS_POWERLEVEL
    },
    {
      .CC = COMMAND_CLASS_SUPERVISION
    },
    {
      .CC = COMMAND_CLASS_VERSION
    },
    {
      .CC = COMMAND_CLASS_ZWAVEPLUS_INFO_V2
    },
    {
      .CC = 0xFF
    },
    {
      .CC = COMMAND_CLASS_BATTERY
    },
    {
      .CC = COMMAND_CLASS_SENSOR_MULTILEVEL
    },
    {
      .CC = COMMAND_CLASS_CONFIGURATION
    },
    {
      .CC = COMMAND_CLASS_WAKE_UP
    }
  };

  uint8_t expected_unsecure_included_cc[] =
  {
    COMMAND_CLASS_ZWAVEPLUS_INFO,
    COMMAND_CLASS_ASSOCIATION,
    COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION_V2,
    COMMAND_CLASS_ASSOCIATION_GRP_INFO,
    COMMAND_CLASS_TRANSPORT_SERVICE_V2,
    COMMAND_CLASS_VERSION,
    COMMAND_CLASS_MANUFACTURER_SPECIFIC,
    COMMAND_CLASS_DEVICE_RESET_LOCALLY,
    COMMAND_CLASS_INDICATOR,
    COMMAND_CLASS_POWERLEVEL,
    COMMAND_CLASS_SECURITY_2,
    COMMAND_CLASS_WAKE_UP,
    COMMAND_CLASS_SUPERVISION,
    COMMAND_CLASS_FIRMWARE_UPDATE_MD_V5,
    COMMAND_CLASS_CONFIGURATION_V4,
    COMMAND_CLASS_SENSOR_MULTILEVEL_V11,
    COMMAND_CLASS_BATTERY
  };

  uint8_t expected_secure_included_unsecure_cc[] =
  {
    COMMAND_CLASS_ZWAVEPLUS_INFO,
    COMMAND_CLASS_TRANSPORT_SERVICE_V2,
    COMMAND_CLASS_SECURITY_2,
    COMMAND_CLASS_SUPERVISION
  };

  uint8_t expected_secure_included_secure_cc[] =
  {
    COMMAND_CLASS_VERSION,
    COMMAND_CLASS_ASSOCIATION,
    COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION_V2,
    COMMAND_CLASS_ASSOCIATION_GRP_INFO,
    COMMAND_CLASS_MANUFACTURER_SPECIFIC,
    COMMAND_CLASS_DEVICE_RESET_LOCALLY,
    COMMAND_CLASS_INDICATOR,
    COMMAND_CLASS_POWERLEVEL,
    COMMAND_CLASS_WAKE_UP,
    COMMAND_CLASS_FIRMWARE_UPDATE_MD_V5,
    COMMAND_CLASS_CONFIGURATION_V4,
    COMMAND_CLASS_SENSOR_MULTILEVEL_V11,
    COMMAND_CLASS_BATTERY
  };

  g_handler_maps = handler_maps;
  g_handler_maps_size = sizeof_array(handler_maps);

  zaf_config_get_requested_security_keys_ExpectAndReturn(SECURITY_KEY_S2_UNAUTHENTICATED_BIT | SECURITY_KEY_S2_AUTHENTICATED_BIT);
  ZAF_CC_handler_map_size_ExpectAndReturn(sizeof_array(handler_maps));
  zaf_config_get_number_of_endpoints_IgnoreAndReturn(0);
  ZAF_CC_foreach_StubWithCallback(ZAF_CC_foreach_Callback);
  zaf_config_get_secure_only_IgnoreAndReturn(0);

  zafi_cc_list_generator_generate();

  zaf_config_get_number_of_endpoints_IgnoreAndReturn(0);

  zafi_cc_list_generator_get_lists(0, &unsecure_included_cc, &secure_included_unsecure_cc, &secure_included_secure_cc);

  validate_list(unsecure_included_cc, expected_unsecure_included_cc, sizeof_array(expected_unsecure_included_cc));
  validate_list(secure_included_unsecure_cc, expected_secure_included_unsecure_cc, sizeof_array(expected_secure_included_unsecure_cc));
  validate_list(secure_included_secure_cc, expected_secure_included_secure_cc, sizeof_array(expected_secure_included_secure_cc));

  zafi_cc_list_generator_destroy();
}

/**
 * Test CC List Generator With Key Fob information
 */
void test_key_fob(void)
{
  zaf_cc_list_t *unsecure_included_cc;
  zaf_cc_list_t *secure_included_unsecure_cc;
  zaf_cc_list_t *secure_included_secure_cc;

  CC_handler_map_latest_t handler_maps[] = {
    {
      .CC = 0xFF
    },
    {
      .CC = COMMAND_CLASS_ASSOCIATION
    },
    {
      .CC = COMMAND_CLASS_ASSOCIATION_GRP_INFO
    },
    {
      .CC = COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION_V2
    },
    {
      .CC = COMMAND_CLASS_BATTERY
    },
    {
      .CC = COMMAND_CLASS_DEVICE_RESET_LOCALLY
    },
    {
      .CC = COMMAND_CLASS_MANUFACTURER_SPECIFIC
    },
    {
      .CC = COMMAND_CLASS_SUPERVISION
    },
    {
      .CC = COMMAND_CLASS_VERSION
    },
    {
      .CC = COMMAND_CLASS_ZWAVEPLUS_INFO_V2
    }
  };

  uint8_t expected_unsecure_included_cc[] =
  {
    COMMAND_CLASS_ZWAVEPLUS_INFO_V2,
    COMMAND_CLASS_ASSOCIATION,
    COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION_V2,
    COMMAND_CLASS_ASSOCIATION_GRP_INFO,
    COMMAND_CLASS_TRANSPORT_SERVICE_V2,
    COMMAND_CLASS_BATTERY,
    COMMAND_CLASS_MANUFACTURER_SPECIFIC,
    COMMAND_CLASS_SECURITY,
    COMMAND_CLASS_SECURITY_2,
    COMMAND_CLASS_DEVICE_RESET_LOCALLY,
    COMMAND_CLASS_SUPERVISION,
    COMMAND_CLASS_VERSION
  };

  uint8_t expected_secure_included_unsecure_cc[] =
  {
    COMMAND_CLASS_ZWAVEPLUS_INFO,
    COMMAND_CLASS_TRANSPORT_SERVICE_V2,
    COMMAND_CLASS_SECURITY,
    COMMAND_CLASS_SECURITY_2,
    COMMAND_CLASS_SUPERVISION
  };

  uint8_t expected_secure_included_secure_cc[] =
  {
    COMMAND_CLASS_ASSOCIATION,
    COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION_V2,
    COMMAND_CLASS_ASSOCIATION_GRP_INFO,
    COMMAND_CLASS_BATTERY,
    COMMAND_CLASS_VERSION,
    COMMAND_CLASS_MANUFACTURER_SPECIFIC,
    COMMAND_CLASS_DEVICE_RESET_LOCALLY
  };

  g_handler_maps = handler_maps;
  g_handler_maps_size = sizeof_array(handler_maps);

  zaf_config_get_requested_security_keys_ExpectAndReturn(SECURITY_KEY_S0_BIT | SECURITY_KEY_S2_UNAUTHENTICATED_BIT | SECURITY_KEY_S2_AUTHENTICATED_BIT | SECURITY_KEY_S2_ACCESS_BIT);
  ZAF_CC_handler_map_size_ExpectAndReturn(sizeof_array(handler_maps));
  zaf_config_get_number_of_endpoints_IgnoreAndReturn(0);
  ZAF_CC_foreach_StubWithCallback(ZAF_CC_foreach_Callback);
  zaf_config_get_secure_only_IgnoreAndReturn(0);

  zafi_cc_list_generator_generate();

  zaf_config_get_number_of_endpoints_IgnoreAndReturn(0);

  zafi_cc_list_generator_get_lists(0, &unsecure_included_cc, &secure_included_unsecure_cc, &secure_included_secure_cc);

  validate_list(unsecure_included_cc, expected_unsecure_included_cc, sizeof_array(expected_unsecure_included_cc));
  validate_list(secure_included_unsecure_cc, expected_secure_included_unsecure_cc, sizeof_array(expected_secure_included_unsecure_cc));
  validate_list(secure_included_secure_cc, expected_secure_included_secure_cc, sizeof_array(expected_secure_included_secure_cc));

  zafi_cc_list_generator_destroy();
}

/**
 * Verifies behavior for command classes that are not used by existing command classes as of
 * 2023-03-09:
 * - COMMAND_CLASS_APPLICATION_STATUS
 * - COMMAND_CLASS_INCLUSION_CONTROLLER
 * - COMMAND_CLASS_MULTI_CMD
 * - COMMAND_CLASS_TIME
 */
void test_unused_command_classes(void)
{
  zaf_cc_list_t *unsecure_included_cc;
  zaf_cc_list_t *secure_included_unsecure_cc;
  zaf_cc_list_t *secure_included_secure_cc;

  CC_handler_map_latest_t handler_maps[] = {
    {
      .CC = 0xFF
    },
    {
      .CC = COMMAND_CLASS_ASSOCIATION
    },
    {
      .CC = COMMAND_CLASS_ASSOCIATION_GRP_INFO
    },
    {
      .CC = COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION_V2
    },
    {
      .CC = COMMAND_CLASS_BATTERY
    },
    {
      .CC = COMMAND_CLASS_DEVICE_RESET_LOCALLY
    },
    {
      .CC = COMMAND_CLASS_MANUFACTURER_SPECIFIC
    },
    {
      .CC = COMMAND_CLASS_SUPERVISION
    },
    {
      .CC = COMMAND_CLASS_VERSION
    },
    {
      .CC = COMMAND_CLASS_ZWAVEPLUS_INFO_V2
    },
    {
      .CC = COMMAND_CLASS_APPLICATION_STATUS
    },
    {
      .CC = COMMAND_CLASS_INCLUSION_CONTROLLER
    },
    {
      .CC = COMMAND_CLASS_MULTI_CMD
    },
    {
      .CC = COMMAND_CLASS_TIME
    }
  };

  uint8_t expected_unsecure_included_cc[] =
  {
    COMMAND_CLASS_ZWAVEPLUS_INFO_V2,
    COMMAND_CLASS_ASSOCIATION,
    COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION_V2,
    COMMAND_CLASS_ASSOCIATION_GRP_INFO,
    COMMAND_CLASS_TRANSPORT_SERVICE_V2,
    COMMAND_CLASS_BATTERY,
    COMMAND_CLASS_MANUFACTURER_SPECIFIC,
    COMMAND_CLASS_SECURITY,
    COMMAND_CLASS_SECURITY_2,
    COMMAND_CLASS_DEVICE_RESET_LOCALLY,
    COMMAND_CLASS_SUPERVISION,
    COMMAND_CLASS_VERSION,
    COMMAND_CLASS_APPLICATION_STATUS,
    COMMAND_CLASS_INCLUSION_CONTROLLER,
    COMMAND_CLASS_MULTI_CMD,
    COMMAND_CLASS_TIME
  };

  uint8_t expected_secure_included_unsecure_cc[] =
  {
    COMMAND_CLASS_ZWAVEPLUS_INFO,
    COMMAND_CLASS_TRANSPORT_SERVICE_V2,
    COMMAND_CLASS_SECURITY,
    COMMAND_CLASS_SECURITY_2,
    COMMAND_CLASS_SUPERVISION,
    COMMAND_CLASS_APPLICATION_STATUS,
    COMMAND_CLASS_INCLUSION_CONTROLLER,
    COMMAND_CLASS_MULTI_CMD,
    COMMAND_CLASS_TIME
  };

  uint8_t expected_secure_included_secure_cc[] =
  {
    COMMAND_CLASS_ASSOCIATION,
    COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION_V2,
    COMMAND_CLASS_ASSOCIATION_GRP_INFO,
    COMMAND_CLASS_BATTERY,
    COMMAND_CLASS_VERSION,
    COMMAND_CLASS_MANUFACTURER_SPECIFIC,
    COMMAND_CLASS_DEVICE_RESET_LOCALLY
  };

  g_handler_maps = handler_maps;
  g_handler_maps_size = sizeof_array(handler_maps);

  zaf_config_get_requested_security_keys_ExpectAndReturn(SECURITY_KEY_S0_BIT | SECURITY_KEY_S2_UNAUTHENTICATED_BIT | SECURITY_KEY_S2_AUTHENTICATED_BIT | SECURITY_KEY_S2_ACCESS_BIT);
  ZAF_CC_handler_map_size_ExpectAndReturn(sizeof_array(handler_maps));
  zaf_config_get_number_of_endpoints_IgnoreAndReturn(0);
  ZAF_CC_foreach_StubWithCallback(ZAF_CC_foreach_Callback);
  zaf_config_get_secure_only_IgnoreAndReturn(0);

  zafi_cc_list_generator_generate();

  zaf_config_get_number_of_endpoints_IgnoreAndReturn(0);

  zafi_cc_list_generator_get_lists(0, &unsecure_included_cc, &secure_included_unsecure_cc, &secure_included_secure_cc);

  validate_list(unsecure_included_cc, expected_unsecure_included_cc, sizeof_array(expected_unsecure_included_cc));
  validate_list(secure_included_unsecure_cc, expected_secure_included_unsecure_cc, sizeof_array(expected_secure_included_unsecure_cc));
  validate_list(secure_included_secure_cc, expected_secure_included_secure_cc, sizeof_array(expected_secure_included_secure_cc));

  zafi_cc_list_generator_destroy();
}

/******************************************************************************/
/* Mocking the pvPortMalloc and vPortFree is an impossible mission            */
/* Hacky mock below                                                           */
/******************************************************************************/
void * pvPortMalloc(size_t xWantedSize)
{
  return malloc(xWantedSize);
}

void vPortFree(void * pv)
{
  free(pv);
}
