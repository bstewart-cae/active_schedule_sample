#include <unity.h>
#include <test_common.h>
#include <CC_Basic.h>
#include "ZAF_CC_Invoker.h"
#include "zaf_config_api_mock.h"
#include "ZW_TransportSecProtocol_mock.h"

void setUpSuite(void) {
}

void tearDownSuite(void) {
}

void setUp(void) {
}

void tearDown(void) {
}

void test_BASIC_SET_endpoint_retention(void) {
  command_handler_input_t chi;
  zaf_cc_list_t cc_list_empty = {0, NULL};
  uint8_t value = 0xFF;

  // Create Basic Set frame addressed to Root Device
  test_common_clear_command_handler_input(&chi);
  chi.frame.as_zw_application_tx_buffer.ZW_BasicSetFrame.cmdClass = COMMAND_CLASS_BASIC;
  chi.frame.as_zw_application_tx_buffer.ZW_BasicSetFrame.cmd = BASIC_SET;
  chi.frame.as_zw_application_tx_buffer.ZW_BasicSetFrame.value = value;
  chi.rxOptions.destNode.endpoint = 0;

  // Prevent forwarding the frame to a specific Command Class
  GetCommandClassList_IgnoreAndReturn(&cc_list_empty);
  // Report 1 endpoint
  zaf_config_get_number_of_endpoints_IgnoreAndReturn(1);

  invoke_cc_handler_v2(&chi.rxOptions, &chi.frame.as_zw_application_tx_buffer, chi.frameLength, NULL, NULL);

  // Verify that the destination is still the Root Device
  TEST_ASSERT_MESSAGE(chi.rxOptions.destNode.endpoint == 0, "The destination endpoint was changed incorrectly.");
}
