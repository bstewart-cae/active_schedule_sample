/**
 * @file test_CC_CentralScene.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include <test_common.h>
#include <string.h>
#include <mock_control.h>
#include <CC_CentralScene.h>
#include <ZAF_TSE.h>
#include "ZAF_CC_Invoker.h"
#include <ZAF_file_ids.h>
#include <CC_CentralScene_config_mock.h>
#include <cc_central_scene_io_mock.h>
#include <SizeOf.h>
#include "zaf_transport_tx_mock.h"

extern void CC_CentralScene_configuration_report_stx(
    TRANSMIT_OPTIONS_TYPE_SINGLE_EX txOptions,
    void *pData);

static cc_central_scene_t scenes_attributes [] = {
  {
     .scene_number = 1,
     .scene_attributes =
         (1 << CENTRAL_SCENE_NOTIFICATION_KEY_ATTRIBUTES_KEY_PRESSED_1_TIME_V2 ) |
         (1 << CENTRAL_SCENE_NOTIFICATION_KEY_ATTRIBUTES_KEY_RELEASED_V2 ) |
         (1 << CENTRAL_SCENE_NOTIFICATION_KEY_ATTRIBUTES_KEY_HELD_DOWN_V2)
  }
};

static central_scene_configuration_t central_scene_configuration = {
    .slowRefresh = 1
};
void setUpSuite(void) {

}

void tearDownSuite(void) {

}

void setUp(void)
{
  cc_central_scene_migrate_Ignore();
  cc_central_scene_read_IgnoreAndReturn(false);
  cc_central_scene_write_IgnoreAndReturn(true);

  mock_call_use_as_stub(TO_STR(AppTimerRegister));
  ZAF_CC_init_specific(COMMAND_CLASS_CENTRAL_SCENE);
}

void tearDown(void)
{
}

#define SUPPORTED_SCENES 3
#define IDENTICAL 1
#define NBR_BITMASK_BYTES 1
#define PROPERTIES ((NBR_BITMASK_BYTES << 1) | IDENTICAL)
#define supportedKeyAttributesForScene 7

static void supported_get_frame_create(uint8_t * pFrame, uint8_t * pFrameLength);
static void configuration_get_frame_create(uint8_t * pFrame, uint8_t * pFrameLength);
static void configuration_set_frame_create(uint8_t * pFrame, uint8_t * pFrameLength, uint8_t slowRefresh);

void test_handleCommandClassCentralScene_unknown_command(void)
{
  mock_calls_clear();
  RECEIVE_OPTIONS_TYPE_EX rxOptions;
  memset((uint8_t *)&rxOptions, 0x00, sizeof(RECEIVE_OPTIONS_TYPE_EX));
  ZW_APPLICATION_TX_BUFFER frame;
  uint8_t * pFrame = (uint8_t *)&frame;
  uint8_t commandLength = 0;

  uint8_t frameCount = 0;
  memset(pFrame, 0x00, sizeof(ZW_APPLICATION_TX_BUFFER));
  pFrame[frameCount++] = COMMAND_CLASS_CENTRAL_SCENE;
  pFrame[frameCount++] = 60; // Unknown command.
  commandLength = frameCount;

  ZW_APPLICATION_TX_BUFFER frame_out;
  uint8_t frame_out_length = 0;
  received_frame_status_t status = invoke_cc_handler_v2(&rxOptions, &frame, commandLength, &frame_out, &frame_out_length);
  TEST_ASSERT_EQUAL(RECEIVED_FRAME_STATUS_NO_SUPPORT, status);
  TEST_ASSERT_EQUAL_UINT8(0, frame_out_length);

  mock_calls_verify();
}

void test_CENTRAL_SCENE_SUPPORTED_GET_V2_legal_response_check(void)
{
  mock_t* pMock = NULL;
  mock_calls_clear();

  RECEIVE_OPTIONS_TYPE_EX rxOptions;
  memset((uint8_t *)&rxOptions, 0x00, sizeof(RECEIVE_OPTIONS_TYPE_EX));
  ZW_APPLICATION_TX_BUFFER frame;
  uint8_t * pFrame = (uint8_t *)&frame;
  uint8_t commandLength = 0;

  supported_get_frame_create(pFrame, &commandLength);

  mock_call_expect(TO_STR(Check_not_legal_response_job), &pMock);
  pMock->expect_arg[0].p = &rxOptions;
  pMock->return_code.v = true;

  ZW_APPLICATION_TX_BUFFER frame_out;
  uint8_t frame_out_length = 0;
  received_frame_status_t status = invoke_cc_handler_v2(&rxOptions,
                                                     &frame,
                                                     commandLength,
                                                     &frame_out,
                                                     &frame_out_length);
  TEST_ASSERT_EQUAL(RECEIVED_FRAME_STATUS_FAIL, status);
  TEST_ASSERT_EQUAL_UINT8(0, frame_out_length);

  mock_calls_verify();
}

/// Test CC_CENTRAL_SCENE_SUPPORTED_GET with identical scenes
void test_CENTRAL_SCENE_SUPPORTED_GET_V2_success(void)
{
  mock_calls_clear();

  RECEIVE_OPTIONS_TYPE_EX rxOptions;
  memset((uint8_t *)&rxOptions, 0x00, sizeof(RECEIVE_OPTIONS_TYPE_EX));
  ZW_APPLICATION_TX_BUFFER frame;
  uint8_t * pFrame = (uint8_t *)&frame;
  uint8_t commandLength = 0;

  const uint8_t CENTRAL_SCENE_PROPERTIES = 0x83; // Slow refresh = 1; Number of Bit Mask Bytes = 1; Identical = 1

  supported_get_frame_create(pFrame, &commandLength);

  mock_call_use_as_stub(TO_STR(Check_not_legal_response_job));

  // Expect: 4 identical scenes.
  uint8_t number_of_scenes = 4;
  cc_central_scene_config_get_number_of_scenes_ExpectAndReturn(number_of_scenes);
  cc_central_scene_config_get_identical_ExpectAndReturn(1);
  cc_central_scene_config_get_attribute_bitmask_ExpectAndReturn(1, scenes_attributes[0].scene_attributes);

  uint8_t expectedFrame[] = { // ZW_CENTRAL_SCENE_SUPPORTED_REPORT_1BYTE_V2_FRAME
      0x5B, // COMMAND_CLASS_CENTRAL_SCENE
      0x02, // CENTRAL_SCENE_SUPPORTED_REPORT_V3
      number_of_scenes,
      CENTRAL_SCENE_PROPERTIES,
      scenes_attributes[0].scene_attributes
  };

  ZW_APPLICATION_TX_BUFFER frame_out;
  uint8_t frame_out_length = 0;
  received_frame_status_t status = invoke_cc_handler_v2(&rxOptions,
                                                     &frame,
                                                     commandLength,
                                                     &frame_out,
                                                     &frame_out_length);

  TEST_ASSERT_EQUAL_MESSAGE(RECEIVED_FRAME_STATUS_SUCCESS, status, "Wrong return status :(");
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(sizeof(expectedFrame), frame_out_length, "Unexpected length of output frame :(");
  TEST_ASSERT_EQUAL_UINT8_ARRAY_MESSAGE(expectedFrame, (uint8_t *)&frame_out, frame_out_length, "Unexpected output frame :( ");

  mock_calls_verify();
}

/*
 * Test CENTRAL_SCENE_SUPPORTED_REPORT when no scenes are defined.
 */
void test_CENTRAL_SCENE_SUPPORTED_GET_V2_zero_scenes(void)
{
  mock_calls_clear();

  RECEIVE_OPTIONS_TYPE_EX rxOptions;
  memset((uint8_t *)&rxOptions, 0x00, sizeof(RECEIVE_OPTIONS_TYPE_EX));
  ZW_APPLICATION_TX_BUFFER frame;
  uint8_t * pFrame = (uint8_t *)&frame;
  uint8_t commandLength = 0;

  supported_get_frame_create(pFrame, &commandLength);

  mock_call_use_as_stub(TO_STR(Check_not_legal_response_job));

  cc_central_scene_config_get_number_of_scenes_IgnoreAndReturn(0);
  cc_central_scene_config_get_identical_IgnoreAndReturn(1);
  cc_central_scene_config_get_attribute_bitmask_IgnoreAndReturn(0);

  const uint8_t CENTRAL_SCENE_PROPERTIES = 0x83; // Slow refresh = 1; Number of Bit Mask Bytes = 1; Identical = 1
  uint8_t expectedFrame[] = { // ZW_CENTRAL_SCENE_SUPPORTED_REPORT_1BYTE_V2_FRAME
       0x5B, // COMMAND_CLASS_CENTRAL_SCENE
       0x02, // CENTRAL_SCENE_SUPPORTED_REPORT_V3
       0, // no scenes
       CENTRAL_SCENE_PROPERTIES,
       0
   };

  ZW_APPLICATION_TX_BUFFER frame_out;
  uint8_t frame_out_length = 0;
  received_frame_status_t status = invoke_cc_handler_v2(&rxOptions,
                                                     &frame,
                                                     commandLength,
                                                     &frame_out,
                                                     &frame_out_length);

  TEST_ASSERT_EQUAL_MESSAGE(RECEIVED_FRAME_STATUS_SUCCESS, status, "Wrong return status :(");
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(sizeof(expectedFrame), frame_out_length, "Unexpected length of output frame :(");
  TEST_ASSERT_EQUAL_UINT8_ARRAY_MESSAGE(expectedFrame, (uint8_t *)&frame_out, frame_out_length, "Unexpected output frame :( ");


  mock_calls_verify();
}

/// Test Supported GET frame with non identical scenes
void test_CENTRAL_SCENE_SUPPORTED_GET_non_identical(void)
{
  mock_calls_clear();

  RECEIVE_OPTIONS_TYPE_EX rxOptions;
  memset((uint8_t *)&rxOptions, 0x00, sizeof(RECEIVE_OPTIONS_TYPE_EX));
  ZW_APPLICATION_TX_BUFFER frame;
  uint8_t * pFrame = (uint8_t *)&frame;
  uint8_t commandLength = 0;

  const uint8_t CENTRAL_SCENE_PROPERTIES = 0x82; // Slow refresh = 1; Number of Bit Mask Bytes = 1; Identical = 0

  supported_get_frame_create(pFrame, &commandLength);

  mock_call_use_as_stub(TO_STR(Check_not_legal_response_job));

  // Configuration
  cc_central_scene_t scenes[] = {
      {
          .scene_number = 1,
          .scene_attributes =
              (1 << CENTRAL_SCENE_NOTIFICATION_KEY_ATTRIBUTES_KEY_PRESSED_1_TIME_V2 )
      },
      {
          .scene_number = 2,
          .scene_attributes =
              (1 << CENTRAL_SCENE_NOTIFICATION_KEY_ATTRIBUTES_KEY_RELEASED_V2 ) |
              (1 << CENTRAL_SCENE_NOTIFICATION_KEY_ATTRIBUTES_KEY_HELD_DOWN_V2)
      },
      {
          .scene_number = 3,
          .scene_attributes =
              (1 << CENTRAL_SCENE_NOTIFICATION_KEY_ATTRIBUTES_KEY_PRESSED_1_TIME_V2 )
      },
      {
          .scene_number = 4,
          .scene_attributes =
              (1 << CENTRAL_SCENE_NOTIFICATION_KEY_ATTRIBUTES_KEY_PRESSED_1_TIME_V2 ) |
              (1 << CENTRAL_SCENE_NOTIFICATION_KEY_ATTRIBUTES_KEY_RELEASED_V2 ) |
              (1 << CENTRAL_SCENE_NOTIFICATION_KEY_ATTRIBUTES_KEY_HELD_DOWN_V2)
      }
  };

  uint8_t number_of_scenes = sizeof_array(scenes);
  cc_central_scene_config_get_number_of_scenes_ExpectAndReturn(number_of_scenes);
  cc_central_scene_config_get_identical_ExpectAndReturn(0); // not identical
  cc_central_scene_config_get_attribute_bitmask_ExpectAndReturn(1, scenes[0].scene_attributes);

  uint8_t payload[4] = {0}; // 4 = number_of_scenes * CC_CENTRAL_SCENE_SIZE_OF_SUPPORTED_SCENES_ATTRIBUTES
  payload[0] = scenes[0].scene_attributes;
  for (uint8_t scene_no = 2; scene_no <= number_of_scenes; scene_no++)
  {
    cc_central_scene_config_get_attribute_bitmask_ExpectAndReturn(scene_no, scenes[scene_no-1].scene_attributes);
    payload[scene_no-1] = scenes[scene_no-1].scene_attributes;
  }

  uint8_t expectedFrame[] = { // ZW_CENTRAL_SCENE_SUPPORTED_REPORT_1BYTE_V2_FRAME
      0x5B, // COMMAND_CLASS_CENTRAL_SCENE
      0x02, // CENTRAL_SCENE_SUPPORTED_REPORT_V3
      number_of_scenes, // number_of_scenes * CC_CENTRAL_SCENE_SIZE_OF_SUPPORTED_SCENES_ATTRIBUTES
      CENTRAL_SCENE_PROPERTIES,
      payload[0],
      payload[1],
      payload[2],
      payload[3]
  };

  ZW_APPLICATION_TX_BUFFER frame_out;
  uint8_t frame_out_length = 0;
  received_frame_status_t status = invoke_cc_handler_v2(&rxOptions,
                                                     &frame,
                                                     commandLength,
                                                     &frame_out,
                                                     &frame_out_length);
  TEST_ASSERT_EQUAL_MESSAGE(RECEIVED_FRAME_STATUS_SUCCESS, status, "Wrong return status :(");
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(sizeof(expectedFrame), frame_out_length, "Unexpected length of output frame :(");
  TEST_ASSERT_EQUAL_UINT8_ARRAY_MESSAGE(expectedFrame, (uint8_t *)&frame_out, frame_out_length, "Unexpected output frame :( ");

  mock_calls_verify();
}

void test_CENTRAL_SCENE_CONFIGURATION_GET_V3_legal_response_check(void)
{
  mock_t* pMock = NULL;
  mock_calls_clear();

  RECEIVE_OPTIONS_TYPE_EX rxOptions;
  memset((uint8_t *)&rxOptions, 0x00, sizeof(RECEIVE_OPTIONS_TYPE_EX));
  ZW_APPLICATION_TX_BUFFER frame;
  uint8_t * pFrame = (uint8_t *)&frame;
  uint8_t commandLength = 0;

  configuration_get_frame_create(pFrame, &commandLength);

  mock_call_expect(TO_STR(Check_not_legal_response_job), &pMock);
  pMock->expect_arg[0].p = &rxOptions;
  pMock->return_code.v = true;

  ZW_APPLICATION_TX_BUFFER frame_out;
  uint8_t frame_out_length = 0;
  received_frame_status_t status = invoke_cc_handler_v2(&rxOptions,
                                                     &frame,
                                                     commandLength,
                                                     &frame_out,
                                                     &frame_out_length);
  TEST_ASSERT_EQUAL_MESSAGE(RECEIVED_FRAME_STATUS_FAIL, status, "Wrong return status :(");
  TEST_ASSERT_EQUAL_UINT8(0, frame_out_length);

  mock_calls_verify();
}

void test_CENTRAL_SCENE_CONFIGURATION_GET_V3_success(void)
{
  mock_calls_clear();

  RECEIVE_OPTIONS_TYPE_EX rxOptions;
  memset((uint8_t *)&rxOptions, 0x00, sizeof(RECEIVE_OPTIONS_TYPE_EX));
  ZW_APPLICATION_TX_BUFFER frame;
  uint8_t * pFrame = (uint8_t *)&frame;
  uint8_t commandLength = 0;

  configuration_get_frame_create(pFrame, &commandLength);

  mock_call_use_as_stub(TO_STR(Check_not_legal_response_job));

  uint8_t expectedFrame[] = { // ZW_CENTRAL_SCENE_SUPPORTED_REPORT_1BYTE_V2_FRAME
      0x5B, // COMMAND_CLASS_CENTRAL_SCENE
      0x06, // CENTRAL_SCENE_CONFIGURATION_REPORT_V3
      central_scene_configuration.slowRefresh << 7
  };

  ZW_APPLICATION_TX_BUFFER frame_out;
  uint8_t frame_out_length = 0;
  received_frame_status_t status = invoke_cc_handler_v2(&rxOptions,
                                                     &frame,
                                                     commandLength,
                                                     &frame_out,
                                                     &frame_out_length);
  TEST_ASSERT_EQUAL_MESSAGE(RECEIVED_FRAME_STATUS_SUCCESS, status, "Wrong return status :(");
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(sizeof(expectedFrame), frame_out_length, "Unexpected length of output frame :(");
  TEST_ASSERT_EQUAL_UINT8_ARRAY_MESSAGE(expectedFrame, (uint8_t *)&frame_out, frame_out_length, "Unexpected output frame :( ");

  mock_calls_verify();
}

void test_CommandClassCentralSceneNotificationTransmit_success(void)
{
  mock_t* pMock = NULL;
  mock_calls_clear();

  AGI_PROFILE AGI_PROFILE_VAL = {
                                  ASSOCIATION_GROUP_INFO_REPORT_AGI_PROFILE_GENERAL,
                                  ASSOCIATION_GROUP_INFO_REPORT_AGI_GENERAL_LIFELINE
  };
  const uint8_t SOURCE_ENDPOINT = 0;
  const uint8_t KEY_ATTRIBUTE = CENTRAL_SCENE_NOTIFICATION_PROPERTIES1_KEY_ATTRIBUTES_MASK_V3;
  const uint8_t SCENE_NUMBER = 1;
  ccc_pair_t ccc_pair = {COMMAND_CLASS_CENTRAL_SCENE_V3, CENTRAL_SCENE_NOTIFICATION_V3};

  mock_call_expect(TO_STR(ZAF_nvm_read), &pMock);
  pMock->expect_arg[0].v     = ZAF_FILE_ID_CENTRAL_SCENE_CONFIG;
  pMock->compare_rule_arg[1] = COMPARE_NOT_NULL;
  pMock->expect_arg[2].v     = 1; //sizeof(uint8_t)
  central_scene_configuration_t configurationReturn = {.slowRefresh = 1};
  pMock->output_arg[1].p = &configurationReturn;
  pMock->return_code.value   = ZPAL_STATUS_OK;

  uint8_t payload[] = {
    0,
    (uint8_t)((1 << 7) | (KEY_ATTRIBUTE & 0x87)),
    SCENE_NUMBER
  };

  mock_call_expect(TO_STR(cc_engine_multicast_request), &pMock);
  pMock->expect_arg[0].p = &AGI_PROFILE_VAL;
  pMock->expect_arg[1].v = SOURCE_ENDPOINT;
  pMock->expect_arg[2].p = &ccc_pair;
  pMock->expect_arg[3].p = payload;
  pMock->expect_arg[4].v = sizeof(payload);
  pMock->expect_arg[5].v = true;
  pMock->return_code.v = JOB_STATUS_SUCCESS;

  JOB_STATUS status;
  status = cc_central_scene_notification_tx((AGI_PROFILE *)&AGI_PROFILE_VAL,
                                            KEY_ATTRIBUTE,
                                            SCENE_NUMBER,
                                            NULL);

  TEST_ASSERT_EQUAL_UINT8_MESSAGE(0, status, "Job status incorrect.");
}

void test_CommandClassCentralSceneNotificationTransmit_failure(void)
{
  mock_t* pMock = NULL;
  mock_calls_clear();

  AGI_PROFILE AGI_PROFILE_VAL = {
                                  ASSOCIATION_GROUP_INFO_REPORT_AGI_PROFILE_GENERAL,
                                  ASSOCIATION_GROUP_INFO_REPORT_AGI_GENERAL_LIFELINE
  };
  const uint8_t SOURCE_ENDPOINT = 0;
  const uint8_t KEY_ATTRIBUTE = CENTRAL_SCENE_NOTIFICATION_PROPERTIES1_KEY_ATTRIBUTES_MASK_V3;
  const uint8_t SCENE_NUMBER = 1;
  ccc_pair_t ccc_pair = {COMMAND_CLASS_CENTRAL_SCENE_V3, CENTRAL_SCENE_NOTIFICATION_V3};

  mock_call_expect(TO_STR(ZAF_nvm_read), &pMock);
  pMock->expect_arg[0].v     = ZAF_FILE_ID_CENTRAL_SCENE_CONFIG;
  pMock->compare_rule_arg[1] = COMPARE_NOT_NULL;
  pMock->expect_arg[2].v     = 1; //sizeof(uint8_t)
  pMock->output_arg[1].p = &central_scene_configuration;
  pMock->return_code.value   = ZPAL_STATUS_OK;

  uint8_t payload[] = {
    1,
    (uint8_t)((1 << 7) | (KEY_ATTRIBUTE & 0x87)),
    SCENE_NUMBER
  };

  mock_call_expect(TO_STR(cc_engine_multicast_request), &pMock);
  pMock->expect_arg[0].p = &AGI_PROFILE_VAL;
  pMock->expect_arg[1].v = SOURCE_ENDPOINT;
  pMock->expect_arg[2].p = &ccc_pair;
  pMock->expect_arg[3].p = payload;
  pMock->expect_arg[4].v = sizeof(payload);
  pMock->expect_arg[5].v = true;
  pMock->return_code.v = JOB_STATUS_BUSY;

  JOB_STATUS status;
  status = cc_central_scene_notification_tx((AGI_PROFILE *)&AGI_PROFILE_VAL,
                                            KEY_ATTRIBUTE,
                                            SCENE_NUMBER,
                                            NULL);

  TEST_ASSERT_EQUAL_UINT8_MESSAGE(1, status, "Job status incorrect.");
}

void test_CENTRAL_SCENE_CONFIGURATION_SET_V3(void)
{
  mock_t* pMock = NULL;
  mock_calls_clear();

  RECEIVE_OPTIONS_TYPE_EX rxOptions;
  memset((uint8_t *)&rxOptions, 0x00, sizeof(RECEIVE_OPTIONS_TYPE_EX));
  ZW_APPLICATION_TX_BUFFER frame;
  uint8_t * pFrame = (uint8_t *)&frame;
  uint8_t commandLength = 0;

  uint8_t slow_refresh_value = 1;
  configuration_set_frame_create(pFrame, &commandLength, slow_refresh_value);

  cc_central_scene_write_IgnoreAndReturn(false);

  mock_call_expect(TO_STR(ZAF_TSE_Trigger), &pMock);
  pMock->compare_rule_arg[0] = COMPARE_NOT_NULL;  //CC_centralScene_report_stx;
  pMock->compare_rule_arg[1] = COMPARE_NOT_NULL;  //centralSceneData;
  pMock->expect_arg[2].v = true;           //Overwrite_previous_trigger
  pMock->return_code.v = true;

  ZW_APPLICATION_TX_BUFFER frame_out;
  uint8_t frame_out_length = 0;
  received_frame_status_t status = invoke_cc_handler_v2(&rxOptions,
                                                     &frame,
                                                     commandLength,
                                                     &frame_out,
                                                     &frame_out_length);
  TEST_ASSERT_EQUAL_MESSAGE(RECEIVED_FRAME_STATUS_SUCCESS, status, "Wrong return status :(");

  mock_calls_verify();
}

/// Set configuration value with Configuration Set
/// And verify it with Configuration Get/Report
void test_CENTRAL_SCENE_CONFIGURATION_SET_GET_REPORT_WITH_TSE(void)
{
  mock_t* pMock = NULL;
  zaf_tx_options_t txOptions;

  mock_calls_clear();
  mock_call_use_as_stub(TO_STR(Check_not_legal_response_job));

  RECEIVE_OPTIONS_TYPE_EX rxOptions;
  memset((uint8_t *)&rxOptions, 0x00, sizeof(RECEIVE_OPTIONS_TYPE_EX));
  ZW_APPLICATION_TX_BUFFER frame;
  uint8_t * pFrame = (uint8_t *)&frame;
  uint8_t commandLength = 0;

  central_scene_configuration_t new_configuration = {.slowRefresh = 0};
  configuration_set_frame_create(pFrame, &commandLength, new_configuration.slowRefresh);

  cc_central_scene_write_ExpectAndReturn(&new_configuration, true);

  mock_call_expect(TO_STR(ZAF_TSE_Trigger), &pMock);
  pMock->compare_rule_arg[0] = COMPARE_NOT_NULL;  //CC_centralScene_report_stx;
  pMock->compare_rule_arg[1] = COMPARE_NOT_NULL;  //centralSceneData;
  pMock->expect_arg[2].v = true;           //Overwrite_previous_trigger
  pMock->return_code.v = true;

  // CC Central Scene Configuration Set
  received_frame_status_t status = invoke_cc_handler_v2(&rxOptions,
                                                     &frame,
                                                     commandLength,
                                                     NULL,
                                                     NULL);
  TEST_ASSERT_EQUAL_MESSAGE(RECEIVED_FRAME_STATUS_SUCCESS, status, "Wrong return status :(");

  // Fetch the callback passed to ZAF_TSE_Trigger().
  zaf_tse_callback_t cb = (zaf_tse_callback_t)pMock->actual_arg[0].p;

  uint8_t expectedFrame[] = { // ZW_CENTRAL_SCENE_SUPPORTED_REPORT_1BYTE_V2_FRAME
      0x5B, // COMMAND_CLASS_CENTRAL_SCENE
      0x06, // CENTRAL_SCENE_CONFIGURATION_REPORT_V3
      new_configuration.slowRefresh << 7
  };

  memset((uint8_t*)&txOptions, 0x00, sizeof(zaf_tx_options_t));

  zaf_transport_tx_ExpectAndReturn(expectedFrame, sizeof(expectedFrame), ZAF_TSE_TXCallback, &txOptions, true);

  cb(&txOptions, NULL);
  /* ************************************************************************/

  // Verify configuration value with CC Central Scene Get
  memset(pFrame, 0x00, sizeof(ZW_APPLICATION_TX_BUFFER));

  configuration_get_frame_create(pFrame, &commandLength);

  ZW_APPLICATION_TX_BUFFER frame_out;
  uint8_t frame_out_length = 0;
  status = invoke_cc_handler_v2(&rxOptions,
                             &frame,
                             commandLength,
                             &frame_out,
                             &frame_out_length);
  TEST_ASSERT_EQUAL_MESSAGE(RECEIVED_FRAME_STATUS_SUCCESS, status, "Wrong return status :(");
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(sizeof(expectedFrame), frame_out_length, "Unexpected length of output frame :(");
  TEST_ASSERT_EQUAL_UINT8_ARRAY_MESSAGE(expectedFrame, (uint8_t *)&frame_out, frame_out_length, "Unexpected output frame :( ");

  mock_calls_verify();
}

/**
 * Creates a CENTRAL_SCENE_SUPPORTED_GET_V3 frame.
 * @param pFrame Pointer to frame.
 * @param pFrameLength Pointer to length of frame.
 */
static void supported_get_frame_create(uint8_t * pFrame, uint8_t * pFrameLength)
{
  uint8_t frameCount = 0;
  memset(pFrame, 0x00, sizeof(ZW_APPLICATION_TX_BUFFER));
  pFrame[frameCount++] = COMMAND_CLASS_CENTRAL_SCENE;
  pFrame[frameCount++] = CENTRAL_SCENE_SUPPORTED_GET_V3;
  *pFrameLength = frameCount;
}

/**
 * Creates a CENTRAL_SCENE_CONFIGURATION_GET_V3 frame.
 * @param pFrame Pointer to frame.
 * @param pFrameLength Pointer to length of frame.
 */
static void configuration_get_frame_create(uint8_t * pFrame, uint8_t * pFrameLength)
{
  uint8_t frameCount = 0;
  memset(pFrame, 0x00, sizeof(ZW_APPLICATION_TX_BUFFER));
  pFrame[frameCount++] = COMMAND_CLASS_CENTRAL_SCENE;
  pFrame[frameCount++] = CENTRAL_SCENE_CONFIGURATION_GET_V3;
  *pFrameLength = frameCount;
}

/**
 * Creates a CENTRAL_SCENE_CONFIGURATION_SET_V3 frame.
 * @param pFrame Pointer to frame.
 * @param pFrameLength Pointer to length of frame.
 * @param slowRefresh - 0 or 1 to disable or enable.
 */
static void configuration_set_frame_create(uint8_t * pFrame, uint8_t * pFrameLength, uint8_t slowRefresh)
{
  uint8_t frameCount = 0;
  memset(pFrame, 0x00, sizeof(ZW_APPLICATION_TX_BUFFER));
  pFrame[frameCount++] = COMMAND_CLASS_CENTRAL_SCENE;
  pFrame[frameCount++] = CENTRAL_SCENE_CONFIGURATION_SET_V3;
  pFrame[frameCount++] = (slowRefresh << 7);
  *pFrameLength = frameCount;
}
