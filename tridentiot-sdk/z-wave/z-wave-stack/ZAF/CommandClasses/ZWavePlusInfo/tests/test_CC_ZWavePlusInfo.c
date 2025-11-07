/**
 * @file test_CC_ZWavePlusInfo.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include <ZW_classcmd.h>
#include <test_common.h>
#include <mock_control.h>
#include <ZW_TransportEndpoint.h>
#include <string.h>
#include <cc_zwave_plus_info_config_api.h>
#include <SizeOf.h>
#include "ZAF_CC_Invoker.h"
#include "zaf_config.h"

void setUpSuite(void) {

}

void tearDownSuite(void) {

}

void setUp(void) {

}

void tearDown(void) {

}

static const uint8_t ZWAVE_PLUS_VERSION = 2;

static void zwaveplus_info_get_frame_create(uint8_t * pFrame, uint8_t * pFrameLength);

void test_ZWAVEPLUS_INFO_GET_non_supported_command(void)
{
  RECEIVE_OPTIONS_TYPE_EX rxOptions;
  memset((uint8_t *)&rxOptions, 0x00, sizeof(RECEIVE_OPTIONS_TYPE_EX));
  ZW_APPLICATION_TX_BUFFER frame;
  uint8_t * pFrame = (uint8_t *)&frame;
  uint8_t commandLength = 0;

  zwaveplus_info_get_frame_create(pFrame, &commandLength);

  *(pFrame + 1) = 0x00; // Invalid command

  ZW_APPLICATION_TX_BUFFER frame_out;
  uint8_t frame_out_length = 0;

  invoke_cc_handler_v2(
      &rxOptions,
      &frame,
      commandLength,
      &frame_out,
      &frame_out_length);

  TEST_ASSERT_EQUAL_UINT8(0, frame_out_length);
}

typedef struct
{
  uint8_t endpoint;
  uint16_t installer_icon_type;
  uint16_t user_icon_type;
}
test_zwaveplus_info_get_t;

void test_ZWAVEPLUS_INFO_GET_root_device_and_all_endpoints(void)
{
  mock_t* pMock = NULL;
  mock_calls_clear();

  RECEIVE_OPTIONS_TYPE_EX rxOptions;
  memset((uint8_t *)&rxOptions, 0x00, sizeof(RECEIVE_OPTIONS_TYPE_EX));
  ZW_APPLICATION_TX_BUFFER frame;
  uint8_t * pFrame = (uint8_t *)&frame;
  uint8_t commandLength = 0;

  const uint8_t NODE_TYPE = ZWAVEPLUS_INFO_REPORT_NODE_TYPE_ZWAVEPLUS_NODE;
  const uint16_t INSTALLER_ICON_TYPE = ZAF_CONFIG_INSTALLER_ICON_TYPE;
  const uint16_t USER_ICON_TYPE = ZAF_CONFIG_USER_ICON_TYPE;

  zwaveplus_info_get_frame_create(pFrame, &commandLength);

  /*
   * Test root device and all endpoints. Include an extra invalid endpoint (5) to verify that the
   * CC handles this without problems.
   */
  const test_zwaveplus_info_get_t zwaveplus_info_get_expected[] = {
                                                               {0, INSTALLER_ICON_TYPE, USER_ICON_TYPE},
                                                               {1, 0x0123, 0x4567},
                                                               {2, 0x89AB, 0xCDEF},
                                                               {3, 0x1234, 0x5678},
                                                               {4, 0x9ABC, 0xDEF0},
                                                               {5, INSTALLER_ICON_TYPE, USER_ICON_TYPE}
  };

  zw_plus_info_config_icons_t expectedIcons[sizeof_array(zwaveplus_info_get_expected)];

  uint8_t i;

  for (i = 0; i < (sizeof(zwaveplus_info_get_expected) / sizeof(test_zwaveplus_info_get_t)); i++)
  {
    printf("\ni: %u\n", i);
    rxOptions.destNode.endpoint = zwaveplus_info_get_expected[i].endpoint;

    mock_call_use_as_stub(TO_STR(Check_not_legal_response_job));

    mock_call_expect(TO_STR(cc_zwave_plus_info_config_get_endpoint_count), &pMock);
    pMock->return_code.v = (sizeof(zwaveplus_info_get_expected) / sizeof(test_zwaveplus_info_get_t)) - 1;

    if (i != 0)
    {
      expectedIcons[i].installer_icon_type = zwaveplus_info_get_expected[i].installer_icon_type;
      expectedIcons[i].user_icon_type = zwaveplus_info_get_expected[i].user_icon_type;
      expectedIcons[i].endpoint = zwaveplus_info_get_expected[i].endpoint;

      mock_call_expect(TO_STR(cc_zwave_plus_info_config_get_endpoint_entry), &pMock);
      pMock->expect_arg[0].v = zwaveplus_info_get_expected[i].endpoint;
      pMock->return_code.p = &expectedIcons[i];
    }

    uint8_t expectedFrame[] = {
        0x5E, // COMMAND_CLASS_ZWAVEPLUS_INFO
        0x02, // ZWAVEPLUS_INFO_REPORT
        ZWAVE_PLUS_VERSION, // ZW_PLUS_VERSION
        ZAF_CONFIG_APP_ROLE_TYPE,
        NODE_TYPE,
        (uint8_t)(zwaveplus_info_get_expected[i].installer_icon_type >> 8),
        (uint8_t)zwaveplus_info_get_expected[i].installer_icon_type,
        (uint8_t)(zwaveplus_info_get_expected[i].user_icon_type >> 8),
        (uint8_t)zwaveplus_info_get_expected[i].user_icon_type
    };

    ZW_APPLICATION_TX_BUFFER frame_out;
    uint8_t frame_out_length = 0;

    invoke_cc_handler_v2(
        &rxOptions,
        &frame,
        commandLength,
        &frame_out,
        &frame_out_length);

    TEST_ASSERT_EQUAL_UINT8_MESSAGE(sizeof(expectedFrame), frame_out_length, "Frame length didn't match");
    TEST_ASSERT_EQUAL_UINT8_ARRAY_MESSAGE(expectedFrame, (uint8_t *)&frame_out, frame_out_length, "Frame didn't match");
  }

  mock_calls_verify();
}

void test_ZWAVEPLUS_INFO_GET_no_endpoints(void)
{
  mock_t* pMock;
  mock_calls_clear();

  RECEIVE_OPTIONS_TYPE_EX rxOptions;
  memset((uint8_t *)&rxOptions, 0x00, sizeof(RECEIVE_OPTIONS_TYPE_EX));
  ZW_APPLICATION_TX_BUFFER frame;
  uint8_t * pFrame = (uint8_t *)&frame;
  uint8_t commandLength = 0;

  const uint8_t NODE_TYPE = ZWAVEPLUS_INFO_REPORT_NODE_TYPE_ZWAVEPLUS_NODE;
  const uint16_t INSTALLER_ICON_TYPE = ZAF_CONFIG_INSTALLER_ICON_TYPE;
  const uint16_t USER_ICON_TYPE = ZAF_CONFIG_USER_ICON_TYPE;

  zwaveplus_info_get_frame_create(pFrame, &commandLength);

  rxOptions.destNode.endpoint = 1; // Valid endpoint
  
  mock_call_use_as_stub(TO_STR(Check_not_legal_response_job));

  mock_call_expect(TO_STR(cc_zwave_plus_info_config_get_endpoint_count), &pMock);
  pMock->return_code.v = 0;

  uint8_t expectedFrame[] = {
      0x5E, // COMMAND_CLASS_ZWAVEPLUS_INFO
      0x02, // ZWAVEPLUS_INFO_REPORT
      ZWAVE_PLUS_VERSION, // ZW_PLUS_VERSION
      ZAF_CONFIG_APP_ROLE_TYPE,
      NODE_TYPE,
      (uint8_t)(INSTALLER_ICON_TYPE >> 8),
      (uint8_t)INSTALLER_ICON_TYPE,
      (uint8_t)(USER_ICON_TYPE >> 8),
      (uint8_t)USER_ICON_TYPE
  };

  ZW_APPLICATION_TX_BUFFER frame_out;
  uint8_t frame_out_length = 0;

  invoke_cc_handler_v2(
      &rxOptions,
      &frame,
      commandLength,
      &frame_out,
      &frame_out_length);

  TEST_ASSERT_EQUAL_UINT8(sizeof(expectedFrame), frame_out_length);
  TEST_ASSERT_EQUAL_UINT8_ARRAY(expectedFrame, (uint8_t *)&frame_out, frame_out_length);

  mock_calls_verify();
}



/**
 * Creates a ZWAVEPLUS_INFO_GET frame.
 * @param pFrame Pointer to frame.
 * @param pFrameLength Pointer to length of frame.
 */
static void zwaveplus_info_get_frame_create(uint8_t * pFrame, uint8_t * pFrameLength)
{
  uint8_t frameCount = 0;
  memset(pFrame, 0x00, sizeof(ZW_APPLICATION_TX_BUFFER));
  pFrame[frameCount++] = COMMAND_CLASS_ZWAVEPLUS_INFO;
  pFrame[frameCount++] = ZWAVEPLUS_INFO_GET;
  *pFrameLength = frameCount;
}
