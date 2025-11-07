/**
 * @file test_CC_Battery.c
 * @copyright 2022 Silicon Laboratories Inc.
 */

#include <test_common.h>
#include <mock_control.h>
#include <CC_Battery.h>
#include <string.h>
#include <cc_battery_io_mock.h>
#include "ZAF_CC_Invoker.h"
#include <ZW_TransportEndpoint.h>

void setUpSuite(void) {

}

void tearDownSuite(void) {

}

#define CC_Battery_handler(a,b,c,d,e) invoke_cc_handler_v2(a,b,c,d,e)

#define COMMAND_CLASS_BATTERY_NUMBER  (0x80)
#define BATTERY_GET_CMD_NUM           (0x02)
#define BATTERY_REPORT_CMD_NUM        (0x03)
#define BATTERY_DATA_UNASSIGNED_VALUE (CMD_CLASS_BATTERY_LEVEL_FULL + 1)  // Just some value not defined in cc_battery_level_t

void test_battery_reset(void) {
  SBatteryData battery_data;
  battery_data.lastReportedBatteryLevel = BATTERY_DATA_UNASSIGNED_VALUE;
  cc_battery_write_ExpectAndReturn(&battery_data, true);
  ZAF_CC_reset_specific(COMMAND_CLASS_BATTERY_V3);
}

void test_battery_init(void) {
  SBatteryData battery_data;

  battery_data.lastReportedBatteryLevel = BATTERY_DATA_UNASSIGNED_VALUE;
  cc_battery_read_ExpectAndReturn(&battery_data, true);
  ZAF_CC_init_specific(COMMAND_CLASS_BATTERY_V3);

  cc_battery_read_ExpectAndReturn(&battery_data, false);
  cc_battery_write_ExpectAndReturn(&battery_data, true);
  ZAF_CC_init_specific(COMMAND_CLASS_BATTERY_V3);
}


void test_BATTERY_GET_transmit_success(void)
{

  mock_t* pMock = NULL;
  mock_calls_clear();

  /* ---- Setup mock functions ---- */

  mock_call_use_as_stub(TO_STR(Check_not_legal_response_job));

  static RECEIVE_OPTIONS_TYPE_EX rxOptions;
  rxOptions.destNode.endpoint = 1;

  mock_call_expect(TO_STR(CC_Battery_BatteryGet_handler), &pMock);
  pMock->expect_arg[0].v = 1;
  pMock->return_code.v = CMD_CLASS_BATTERY_LEVEL_WARNING;

  uint8_t expectedFrame[] = {
    COMMAND_CLASS_BATTERY_NUMBER,
    BATTERY_REPORT_CMD_NUM,
    CMD_CLASS_BATTERY_LEVEL_WARNING  // expect low-battery warning
  };

  /* ---- Call command handler with a GET command frame ---- */
  uint8_t cmdFrame[] = {
    COMMAND_CLASS_BATTERY_NUMBER,
    BATTERY_GET_CMD_NUM
  };

  ZW_APPLICATION_TX_BUFFER frameOut;
  uint8_t frameOutLength;

  received_frame_status_t res = CC_Battery_handler(
                                       &rxOptions,
                                       (ZW_APPLICATION_TX_BUFFER *) cmdFrame,
                                       sizeof(cmdFrame),
                                       &frameOut,
                                       &frameOutLength);

  TEST_ASSERT_EQUAL_UINT_MESSAGE(RECEIVED_FRAME_STATUS_SUCCESS,
                                 res,
                                 "return code from CC_Battery_handler(...)");

  TEST_ASSERT_EQUAL_INT8_MESSAGE(frameOutLength,
                                  sizeof(expectedFrame),
                                  "Frame size does not match");
  TEST_ASSERT_EQUAL_INT8_ARRAY_MESSAGE(expectedFrame, &frameOut,
                                        sizeof(expectedFrame),
                                        "Frame does not match");
  mock_calls_verify();
}

void test_BATTERY_GET_invalid_frame(void)
{
  mock_calls_clear();

  /* ---- Setup mock functions ---- */

      /* [EMPTY] - none of the mocks should be called in this scenario */

  /* ---- Call command handler with a GET command frame ---- */

  static RECEIVE_OPTIONS_TYPE_EX rxOptions;
  rxOptions.destNode.endpoint = 1;

  uint8_t cmdFrame[] = {
    COMMAND_CLASS_BATTERY_NUMBER,
    0x88  // Unknown command
  };

  ZW_APPLICATION_TX_BUFFER frameOut;
  uint8_t frameOutLength;

  received_frame_status_t res = CC_Battery_handler(
                                       &rxOptions,
                                       (ZW_APPLICATION_TX_BUFFER *) cmdFrame,
                                       sizeof(cmdFrame),
                                       &frameOut,
                                       &frameOutLength);

  TEST_ASSERT_EQUAL_UINT_MESSAGE(RECEIVED_FRAME_STATUS_NO_SUPPORT,
                                 res,
                                 "return code from handleCommandClassBinarySwitch(...)");

  mock_calls_verify();
}
