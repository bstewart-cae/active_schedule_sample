/**
 * @file test_CC_UserCode.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include <stdbool.h>
#include <string.h>
#include <mock_control.h>
#include <CC_UserCode.h>
#include <test_common.h>
#include <ZAF_TSE.h>
#include "ZAF_CC_Invoker.h"
#include <cc_user_code_io_mock.h>
#include "zaf_transport_tx_mock.h"

void setUpSuite(void) {

}

void tearDownSuite(void) {

}

void setUp(void) {

}

void tearDown(void) {

}

static command_handler_input_t * user_code_get_frame_create(uint8_t userID);

/**************************************************************************************************
 *
 **************************************************************************************************
 */
void test_USER_CODE_GET_transmit_success(void)
{
  SUserCode user_code = {
    .user_id_status = USER_ID_OCCUPIED,
    .userCode = { 0x31, 0x31, 0x31, 0x31 },
    .userCodeLen = 4
  };

  mock_calls_clear();

  RECEIVE_OPTIONS_TYPE_EX rxOptions;
  memset((uint8_t *)&rxOptions, 0x00, sizeof(rxOptions));
  ZW_APPLICATION_TX_BUFFER frame;
  uint8_t * pFrame = (uint8_t *)&frame;
  uint8_t commandLength;

  const uint8_t USER_IDENTIFIER = 1;

  memset(pFrame, 0x00, sizeof(ZW_APPLICATION_TX_BUFFER));
  uint8_t frameCount = 0;
  pFrame[frameCount++] = 0x63; // COMMAND_CLASS_USER_CODE
  pFrame[frameCount++] = 0x02; // USER_CODE_GET
  pFrame[frameCount++] = USER_IDENTIFIER;
  commandLength = frameCount;

  mock_call_use_as_stub(TO_STR(Check_not_legal_response_job));

  CC_UserCode_Read_ExpectAndReturn(USER_IDENTIFIER, NULL, true);
  CC_UserCode_Read_IgnoreArg_userCodeData();
  CC_UserCode_Read_ReturnThruPtr_userCodeData(&user_code);

  CC_UserCode_Read_ExpectAndReturn(USER_IDENTIFIER, NULL, true);
  CC_UserCode_Read_IgnoreArg_userCodeData();
  CC_UserCode_Read_ReturnThruPtr_userCodeData(&user_code);

  uint8_t expectedFrame[8] = {
      0x63, // COMMAND_CLASS_USER_CODE
      0x03, // USER_CODE_REPORT
      USER_IDENTIFIER,
      1, // USER_ID_OCCUPIED
  };
  memcpy(expectedFrame + user_code.userCodeLen, user_code.userCode, user_code.userCodeLen);

  ZW_APPLICATION_TX_BUFFER frame_out;
  uint8_t frame_out_length = 0;

  received_frame_status_t status;
  status = invoke_cc_handler_v2(
      &rxOptions,
      &frame,
      commandLength,
      &frame_out,
      &frame_out_length);

  TEST_ASSERT_MESSAGE(status == RECEIVED_FRAME_STATUS_SUCCESS, "Wrong frame status :(");
  TEST_ASSERT_EQUAL_UINT8(sizeof(expectedFrame), frame_out_length);
  TEST_ASSERT_EQUAL_UINT8_ARRAY(expectedFrame, (uint8_t *)&frame_out, frame_out_length);

  mock_calls_verify();
}

/**************************************************************************************************
 *
 **************************************************************************************************
 */
void test_USER_CODE_GET_user_identifier_zero(void)
{
  mock_calls_clear();

  const uint8_t USER_IDENTIFIER = 0;

  command_handler_input_t * p_chi = user_code_get_frame_create(USER_IDENTIFIER);

  mock_call_use_as_stub(TO_STR(Check_not_legal_response_job));

  const uint8_t EXPECTED_FRAME[] = {
                                    COMMAND_CLASS_USER_CODE,
                                    USER_CODE_REPORT,
                                    USER_IDENTIFIER,
                                    (uint8_t)USER_ID_NO_STATUS
  };

  ZW_APPLICATION_TX_BUFFER frame_out;
  uint8_t frame_out_length = 0;

  received_frame_status_t status;
  status = invoke_cc_handler_v2(
      &p_chi->rxOptions,
      &p_chi->frame.as_zw_application_tx_buffer,
      p_chi->frameLength,
      &frame_out,
      &frame_out_length);

  TEST_ASSERT_TRUE_MESSAGE(status == RECEIVED_FRAME_STATUS_SUCCESS, "Wrong frame status :(");
  TEST_ASSERT_EQUAL_UINT8(sizeof(EXPECTED_FRAME), frame_out_length);
  TEST_ASSERT_EQUAL_UINT8_ARRAY(EXPECTED_FRAME, (uint8_t *)&frame_out, frame_out_length);

  test_common_command_handler_input_free(p_chi);

  mock_calls_verify();
}

/**************************************************************************************************
 *
 **************************************************************************************************
 */
void test_USER_CODE_GET_user_identifier_too_high(void)
{
  mock_calls_clear();

  mock_call_use_as_stub(TO_STR(Check_not_legal_response_job));

  for (uint8_t userId = CC_USER_CODE_MAX_IDS + 1; userId < UINT8_MAX; userId++)
  {
    command_handler_input_t * p_chi = user_code_get_frame_create(userId);

    const uint8_t EXPECTED_FRAME[] = {
                                      COMMAND_CLASS_USER_CODE,
                                      USER_CODE_REPORT,
                                      userId,
                                      (uint8_t)USER_ID_NO_STATUS
    };

    ZW_APPLICATION_TX_BUFFER frame_out;
    uint8_t frame_out_length = 0;

    received_frame_status_t status;
    status = invoke_cc_handler_v2(
        &p_chi->rxOptions,
        &p_chi->frame.as_zw_application_tx_buffer,
        p_chi->frameLength,
        &frame_out,
        &frame_out_length);

    TEST_ASSERT_TRUE_MESSAGE(status == RECEIVED_FRAME_STATUS_SUCCESS, "Wrong frame status :(");
    TEST_ASSERT_EQUAL_UINT8(sizeof(EXPECTED_FRAME), frame_out_length);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(EXPECTED_FRAME, (uint8_t *)&frame_out, frame_out_length);

    test_common_command_handler_input_free(p_chi);
  }

  mock_calls_verify();
}

/**************************************************************************************************
 *
 **************************************************************************************************
 */
void test_USER_CODE_SET_too_short(void)
{
  SUserCode user_code = {
    .user_id_status = USER_ID_AVAILABLE,
    .userCode = { '0', '0', '0', '0' },
    .userCodeLen = 4
  };

  mock_calls_clear();

  RECEIVE_OPTIONS_TYPE_EX rxOptions;
  memset((uint8_t *)&rxOptions, 0x00, sizeof(rxOptions));
  ZW_APPLICATION_TX_BUFFER frame;
  uint8_t * pFrame = (uint8_t *)&frame;
  uint8_t commandLength;

  const uint8_t USER_IDENTIFIER = 1;

  CC_UserCode_Read_ExpectAndReturn(USER_IDENTIFIER, NULL, true);
  CC_UserCode_Read_IgnoreArg_userCodeData();
  CC_UserCode_Read_ReturnThruPtr_userCodeData(&user_code);

  CC_UserCode_Read_ExpectAndReturn(USER_IDENTIFIER, NULL, true);
  CC_UserCode_Read_IgnoreArg_userCodeData();
  CC_UserCode_Read_ReturnThruPtr_userCodeData(&user_code);

  // Create a frame
  memset(pFrame, 0x00, sizeof(ZW_APPLICATION_TX_BUFFER));
  uint8_t frameCount = 0;
  pFrame[frameCount++] = 0x63; // COMMAND_CLASS_USER_CODE
  pFrame[frameCount++] = 0x01; // USER_CODE_SET
  pFrame[frameCount++] = USER_IDENTIFIER;
  pFrame[frameCount++] = 0x01; // USER_ID_OCCUPIED
  pFrame[frameCount++] = '3';
  pFrame[frameCount++] = '2';
  pFrame[frameCount++] = '1';
  commandLength = frameCount;

  ZW_APPLICATION_TX_BUFFER frame_out;
  uint8_t frame_out_length = 0;

  received_frame_status_t status;
  status = invoke_cc_handler_v2(
      &rxOptions,
      &frame,
      commandLength,
      &frame_out,
      &frame_out_length);

  TEST_ASSERT_MESSAGE(status == RECEIVED_FRAME_STATUS_FAIL, "Wrong frame status :(");
  TEST_ASSERT_EQUAL_UINT8(0, frame_out_length);

  /*
   * USER  GET
   */

  memset(pFrame, 0x00, sizeof(ZW_APPLICATION_TX_BUFFER));
  frameCount = 0;
  pFrame[frameCount++] = 0x63; // COMMAND_CLASS_USER_CODE
  pFrame[frameCount++] = 0x02; // USER_CODE_GET
  pFrame[frameCount++] = USER_IDENTIFIER;
  commandLength = frameCount;

  mock_call_use_as_stub(TO_STR(Check_not_legal_response_job));

  const uint8_t EXPECTED_FRAME[] = {
      0x63, // COMMAND_CLASS_USER_CODE
      0x03, // USER_CODE_REPORT
      USER_IDENTIFIER,
      0x00, // USER_ID_AVAILABLE
      '0',
      '0',
      '0',
      '0'
  };

  status = invoke_cc_handler_v2(
      &rxOptions,
      &frame,
      commandLength,
      &frame_out,
      &frame_out_length);

  TEST_ASSERT_MESSAGE(status == RECEIVED_FRAME_STATUS_SUCCESS, "Wrong frame status :(");
  TEST_ASSERT_EQUAL_UINT8(sizeof(EXPECTED_FRAME), frame_out_length);
  TEST_ASSERT_EQUAL_UINT8_ARRAY(EXPECTED_FRAME, (uint8_t *)&frame_out, frame_out_length);

  mock_calls_verify();
}

/**************************************************************************************************
 *
 **************************************************************************************************
 */
void test_USER_CODE_SET_too_long(void)
{
  SUserCode user_code = {
    .user_id_status = USER_ID_AVAILABLE,
    .userCode = { '0', '0', '0', '0' },
    .userCodeLen = 4
  };

  mock_calls_clear();

  RECEIVE_OPTIONS_TYPE_EX rxOptions;
  memset((uint8_t *)&rxOptions, 0x00, sizeof(rxOptions));
  ZW_APPLICATION_TX_BUFFER frame;
  uint8_t * pFrame = (uint8_t *)&frame;
  uint8_t commandLength;

  const uint8_t USER_IDENTIFIER = 1;

  CC_UserCode_Read_ExpectAndReturn(USER_IDENTIFIER, NULL, true);
  CC_UserCode_Read_IgnoreArg_userCodeData();
  CC_UserCode_Read_ReturnThruPtr_userCodeData(&user_code);

  CC_UserCode_Read_ExpectAndReturn(USER_IDENTIFIER, NULL, true);
  CC_UserCode_Read_IgnoreArg_userCodeData();
  CC_UserCode_Read_ReturnThruPtr_userCodeData(&user_code);

  // Create a frame
  memset(pFrame, 0x00, sizeof(ZW_APPLICATION_TX_BUFFER));
  uint8_t frameCount = 0;
  pFrame[frameCount++] = 0x63; // COMMAND_CLASS_USER_CODE
  pFrame[frameCount++] = 0x01; // USER_CODE_SET
  pFrame[frameCount++] = USER_IDENTIFIER;
  pFrame[frameCount++] = 0x01; // USER_ID_OCCUPIED
  pFrame[frameCount++] = '0';
  pFrame[frameCount++] = '9';
  pFrame[frameCount++] = '8';
  pFrame[frameCount++] = '7';
  pFrame[frameCount++] = '6';
  pFrame[frameCount++] = '5';
  pFrame[frameCount++] = '4';
  pFrame[frameCount++] = '3';
  pFrame[frameCount++] = '2';
  pFrame[frameCount++] = '1';
  pFrame[frameCount++] = '0';
  commandLength = frameCount;

  ZW_APPLICATION_TX_BUFFER frame_out;
  uint8_t frame_out_length = 0;

  received_frame_status_t status;
  status = invoke_cc_handler_v2(
      &rxOptions,
      &frame,
      commandLength,
      &frame_out,
      &frame_out_length);

  TEST_ASSERT_MESSAGE(status == RECEIVED_FRAME_STATUS_FAIL, "Wrong frame status :(");
  TEST_ASSERT_EQUAL_UINT8(0, frame_out_length);

  /*
   * USER  GET
   */

  memset(pFrame, 0x00, sizeof(ZW_APPLICATION_TX_BUFFER));
  frameCount = 0;
  pFrame[frameCount++] = 0x63; // COMMAND_CLASS_USER_CODE
  pFrame[frameCount++] = 0x02; // USER_CODE_GET
  pFrame[frameCount++] = USER_IDENTIFIER;
  commandLength = frameCount;

  mock_call_use_as_stub(TO_STR(Check_not_legal_response_job));

  uint8_t expectedFrame[] = {
      0x63, // COMMAND_CLASS_USER_CODE
      0x03, // USER_CODE_REPORT
      USER_IDENTIFIER,
      0x00, // USER_ID_AVAILABLE
      '0',
      '0',
      '0',
      '0'
  };

  frame_out_length = 0; // Reset output length before verifying.

  status = invoke_cc_handler_v2(
      &rxOptions,
      &frame,
      commandLength,
      &frame_out,
      &frame_out_length);

TEST_ASSERT_MESSAGE(status == RECEIVED_FRAME_STATUS_SUCCESS, "Wrong frame status :(");
TEST_ASSERT_EQUAL_UINT8(sizeof(expectedFrame), frame_out_length);
TEST_ASSERT_EQUAL_UINT8_ARRAY(expectedFrame, (uint8_t *)&frame_out, frame_out_length);

  mock_calls_verify();
}

/**************************************************************************************************
 *
 **************************************************************************************************
 */
void test_USER_CODE_SET_invalid_characters(void)
{
  mock_calls_clear();

  RECEIVE_OPTIONS_TYPE_EX rxOptions;
  memset((uint8_t *)&rxOptions, 0x00, sizeof(rxOptions));
  ZW_APPLICATION_TX_BUFFER frame;
  uint8_t * pFrame = (uint8_t *)&frame;
  uint8_t commandLength;

#define NUMBER_OF_CHARACTERS (256)

  const uint8_t USER_IDENTIFIER = 1;

  uint8_t test_user_codes[NUMBER_OF_CHARACTERS][USERCODE_MAX_LEN];

  // Fill in an array of user codes with valid/invalid characters.
  uint16_t code_index;
  uint8_t char_index;
  for (code_index = 0; code_index < NUMBER_OF_CHARACTERS; code_index++)
  {
    for (char_index = 0; char_index < USERCODE_MAX_LEN; char_index++)
    {
      test_user_codes[code_index][char_index] = code_index; // Use the code index as character.
    }
  }

  // Start from index 1 because '0' is valid.
  for (code_index = 1; code_index < NUMBER_OF_CHARACTERS; code_index++)
  {
    //printf("%x: %d%d%d%d", code_index, test_user_codes[code_index][0], test_user_codes[code_index][1], test_user_codes[code_index][2], test_user_codes[code_index][3]);
    if (code_index >= 0x30 && code_index <= 0x39)
    {
      // Skip the valid characters.
      continue;
    }

    /*
     * USER  SET
     */

    // Create a frame
    memset(pFrame, 0x00, sizeof(ZW_APPLICATION_TX_BUFFER));
    uint8_t frameCount = 0;
    pFrame[frameCount++] = 0x63; // COMMAND_CLASS_USER_CODE
    pFrame[frameCount++] = 0x01; // USER_CODE_SET
    pFrame[frameCount++] = USER_IDENTIFIER;
    pFrame[frameCount++] = USER_ID_OCCUPIED;
    pFrame[frameCount++] = test_user_codes[code_index][0];
    pFrame[frameCount++] = test_user_codes[code_index][1];
    pFrame[frameCount++] = test_user_codes[code_index][2];
    pFrame[frameCount++] = test_user_codes[code_index][3];
    commandLength = frameCount;

    ZW_APPLICATION_TX_BUFFER frame_out;
    uint8_t frame_out_length = 0;

    received_frame_status_t status;
    status = invoke_cc_handler_v2(
        &rxOptions,
        &frame,
        commandLength,
        &frame_out,
        &frame_out_length);

    TEST_ASSERT_MESSAGE(status == RECEIVED_FRAME_STATUS_FAIL, "Wrong frame status :(");
    TEST_ASSERT_EQUAL_UINT8(0, frame_out_length);
  }

  mock_calls_verify();
}

/**************************************************************************************************
 *
 **************************************************************************************************
 */
void test_USER_CODE_SET_invalid_character(void)
{
  SUserCode user_code = {
    .user_id_status = USER_ID_AVAILABLE,
    .userCode = { },
    .userCodeLen = 0
  };

  mock_calls_clear();

  RECEIVE_OPTIONS_TYPE_EX rxOptions;
  memset((uint8_t *)&rxOptions, 0x00, sizeof(RECEIVE_OPTIONS_TYPE_EX));
  ZW_APPLICATION_TX_BUFFER frame;
  uint8_t * pFrame = (uint8_t *)&frame;
  uint8_t commandLength;

  const uint8_t USER_IDENTIFIER = 1;

  // Create a frame
  memset(pFrame, 0x00, sizeof(ZW_APPLICATION_TX_BUFFER));
  uint8_t frameCount = 0;
  pFrame[frameCount++] = 0x63; // COMMAND_CLASS_USER_CODE
  pFrame[frameCount++] = 0x01; // USER_CODE_SET
  pFrame[frameCount++] = USER_IDENTIFIER;
  pFrame[frameCount++] = USER_ID_OCCUPIED;
  pFrame[frameCount++] = '1';
  pFrame[frameCount++] = 'a'; // Invalid character
  pFrame[frameCount++] = '1';
  pFrame[frameCount++] = '1';
  commandLength = frameCount;

  ZW_APPLICATION_TX_BUFFER frame_out;
  uint8_t frame_out_length = 0;

  received_frame_status_t status;
  status = invoke_cc_handler_v2(
      &rxOptions,
      &frame,
      commandLength,
      &frame_out,
      &frame_out_length);

  TEST_ASSERT_MESSAGE(status == RECEIVED_FRAME_STATUS_FAIL, "Wrong frame status :(");
  TEST_ASSERT_EQUAL_UINT8(0, frame_out_length);

  /*
   * USER  GET
   */

  CC_UserCode_Read_ExpectAndReturn(USER_IDENTIFIER, NULL, true);
  CC_UserCode_Read_IgnoreArg_userCodeData();
  CC_UserCode_Read_ReturnThruPtr_userCodeData(&user_code);

  CC_UserCode_Read_ExpectAndReturn(USER_IDENTIFIER, NULL, true);
  CC_UserCode_Read_IgnoreArg_userCodeData();
  CC_UserCode_Read_ReturnThruPtr_userCodeData(&user_code);

  memset(pFrame, 0x00, sizeof(ZW_APPLICATION_TX_BUFFER));
  frameCount = 0;
  pFrame[frameCount++] = 0x63; // COMMAND_CLASS_USER_CODE
  pFrame[frameCount++] = 0x02; // USER_CODE_GET
  pFrame[frameCount++] = USER_IDENTIFIER;
  commandLength = frameCount;

  mock_call_use_as_stub(TO_STR(Check_not_legal_response_job));

  uint8_t expectedFrame[] = {
      0x63, // COMMAND_CLASS_USER_CODE
      0x03, // USER_CODE_REPORT
      USER_IDENTIFIER,
      USER_ID_AVAILABLE
  };

  frame_out_length = 0;

  status = invoke_cc_handler_v2(
      &rxOptions,
      &frame,
      commandLength,
      &frame_out,
      &frame_out_length);

  TEST_ASSERT_MESSAGE(status == RECEIVED_FRAME_STATUS_SUCCESS, "Wrong frame status :(");
  TEST_ASSERT_EQUAL_UINT8(sizeof(expectedFrame), frame_out_length);
  TEST_ASSERT_EQUAL_UINT8_ARRAY(expectedFrame, (uint8_t *)&frame_out, frame_out_length);

  mock_calls_verify();
}

/**************************************************************************************************
 *
 **************************************************************************************************
 */
void test_USER_CODE_SET_invalid_user_id_status(void)
{
  mock_calls_clear();

  RECEIVE_OPTIONS_TYPE_EX rxOptions;
  memset((uint8_t *)&rxOptions, 0x00, sizeof(rxOptions));
  ZW_APPLICATION_TX_BUFFER frame;
  uint8_t * pFrame = (uint8_t *)&frame;
  uint8_t commandLength;

  const uint8_t USER_IDENTIFIER = 1;

  /*
   * USER  SET
   */

  // Create a frame
  memset(pFrame, 0x00, sizeof(ZW_APPLICATION_TX_BUFFER));
  uint8_t frameCount = 0;
  pFrame[frameCount++] = 0x63; // COMMAND_CLASS_USER_CODE
  pFrame[frameCount++] = 0x01; // USER_CODE_SET
  pFrame[frameCount++] = USER_IDENTIFIER;
  pFrame[frameCount++] = 0x03; // Invalid user ID status
  pFrame[frameCount++] = '1';
  pFrame[frameCount++] = '1';
  pFrame[frameCount++] = '1';
  pFrame[frameCount++] = '1';
  commandLength = frameCount;

  ZW_APPLICATION_TX_BUFFER frame_out;
  uint8_t frame_out_length = 0;

  received_frame_status_t status;
  status = invoke_cc_handler_v2(
      &rxOptions,
      &frame,
      commandLength,
      &frame_out,
      &frame_out_length);

  TEST_ASSERT_MESSAGE(status == RECEIVED_FRAME_STATUS_FAIL, "Wrong frame status :(");

  mock_calls_verify();
}

/**************************************************************************************************
 *
 **************************************************************************************************
 */
void test_USER_CODE_SET_all_users(void)
{
  mock_t* pMock = NULL;
  mock_calls_clear();

  RECEIVE_OPTIONS_TYPE_EX rxOptions;
  memset((uint8_t *)&rxOptions, 0x00, sizeof(rxOptions));
  ZW_APPLICATION_TX_BUFFER frame;
  uint8_t * pFrame = (uint8_t *)&frame;
  uint8_t commandLength;

  uint8_t byteCount;
  SUserCode user_code;

  ZW_APPLICATION_TX_BUFFER frame_out;
  uint8_t frame_out_length = 0;

  /*
   * USER  SET - OCCUPY ALL AVAILABLE USER CODES.
   */

  uint8_t userCount;
  for (userCount = 1; userCount <= CC_USER_CODE_MAX_IDS; userCount++)
  {
    memset(&user_code, 0, sizeof(user_code));
    user_code.user_id_status = USER_ID_OCCUPIED;
    memset(user_code.userCode, 0x30 + userCount, 4);
    user_code.userCodeLen = 4;

    // Create a frame
    memset(pFrame, 0x00, sizeof(ZW_APPLICATION_TX_BUFFER));
    byteCount = 0;
    pFrame[byteCount++] = 0x63; // COMMAND_CLASS_USER_CODE
    pFrame[byteCount++] = 0x01; // USER_CODE_SET
    pFrame[byteCount++] = userCount;
    pFrame[byteCount++] = USER_ID_OCCUPIED;
    pFrame[byteCount++] = 0x30 + userCount;
    pFrame[byteCount++] = 0x30 + userCount;
    pFrame[byteCount++] = 0x30 + userCount;
    pFrame[byteCount++] = 0x30 + userCount;
    commandLength = byteCount;

    CC_UserCode_Write_ExpectAndReturn(userCount, &user_code, true);
    CC_UserCode_Write_ReturnThruPtr_userCodeData(&user_code);

    mock_call_expect(TO_STR(ZAF_TSE_Trigger), &pMock);
    pMock->compare_rule_arg[0] = COMPARE_NOT_NULL;
    pMock->compare_rule_arg[1] = COMPARE_NOT_NULL;
    pMock->expect_arg[2].v = false;       //Overwrite_previous_trigger
    pMock->return_code.v = true;

    frame_out_length = 0;

    received_frame_status_t status;
    status = invoke_cc_handler_v2(
        &rxOptions,
        &frame,
        commandLength,
        &frame_out,
        &frame_out_length);

    TEST_ASSERT_MESSAGE(status == RECEIVED_FRAME_STATUS_SUCCESS, "Wrong SET ALL USERS frame status :(");;
    TEST_ASSERT_EQUAL_UINT8(0, frame_out_length);
  }

  /*
   * USER  GET ALL
   */
  for (userCount = 1; userCount <= CC_USER_CODE_MAX_IDS; userCount++)
  {
    //printf("\nUser count: %d\n", userCount);
    user_code.user_id_status = USER_ID_OCCUPIED;
    memset(user_code.userCode, 0x30 + userCount, 4);
    user_code.userCodeLen = 4;

    memset(pFrame, 0x00, sizeof(ZW_APPLICATION_TX_BUFFER));
    byteCount = 0;
    pFrame[byteCount++] = 0x63; // COMMAND_CLASS_USER_CODE
    pFrame[byteCount++] = 0x02; // USER_CODE_GET
    pFrame[byteCount++] = userCount;
    commandLength = byteCount;

    CC_UserCode_Read_ExpectAndReturn(userCount, NULL, true);
    CC_UserCode_Read_IgnoreArg_userCodeData();
    CC_UserCode_Read_ReturnThruPtr_userCodeData(&user_code);

    CC_UserCode_Read_ExpectAndReturn(userCount, NULL, true);
    CC_UserCode_Read_IgnoreArg_userCodeData();
    CC_UserCode_Read_ReturnThruPtr_userCodeData(&user_code);

    mock_call_use_as_stub(TO_STR(Check_not_legal_response_job));

    uint8_t expectedFrame[] = {
            0x63, // COMMAND_CLASS_USER_CODE
            0x03, // USER_CODE_REPORT
            userCount,
            USER_ID_OCCUPIED,
            0x30 + userCount,
            0x30 + userCount,
            0x30 + userCount,
            0x30 + userCount
    };

    mock_call_expect(TO_STR(ZAF_TSE_Trigger), &pMock);
    pMock->compare_rule_arg[0] = COMPARE_NOT_NULL;
    pMock->compare_rule_arg[1] = COMPARE_NOT_NULL;
    pMock->expect_arg[2].v = false;       //Overwrite_previous_trigger
    pMock->return_code.v = true;

    frame_out_length = 0;

    received_frame_status_t status;
    status = invoke_cc_handler_v2(
        &rxOptions,
        &frame,
        commandLength,
        &frame_out,
        &frame_out_length);

    TEST_ASSERT_MESSAGE(status == RECEIVED_FRAME_STATUS_SUCCESS, "Wrong GET ALL USERS frame status :(");;
    TEST_ASSERT_EQUAL_UINT8(sizeof(expectedFrame), frame_out_length);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expectedFrame, (uint8_t *)&frame_out, frame_out_length);
  }

  /*
   * USER  SET ALL USERS BY USER IDENTIFIER ZERO
   */

  const uint8_t USER_IDENTIFIER = 0;
  memset(&user_code, 0, sizeof(user_code));
  user_code.user_id_status = USER_ID_AVAILABLE;
  memset(user_code.userCode, 0, 4);
  user_code.userCodeLen = 4;

  // Create a frame
  memset(pFrame, 0x00, sizeof(ZW_APPLICATION_TX_BUFFER));
  uint8_t frameCount = 0;
  pFrame[frameCount++] = 0x63; // COMMAND_CLASS_USER_CODE
  pFrame[frameCount++] = 0x01; // USER_CODE_SET
  pFrame[frameCount++] = USER_IDENTIFIER;
  pFrame[frameCount++] = USER_ID_AVAILABLE;
  pFrame[frameCount++] = 0;
  pFrame[frameCount++] = 0;
  pFrame[frameCount++] = 0;
  pFrame[frameCount++] = 0;
  commandLength = frameCount;

  for (userCount = 1; userCount <= CC_USER_CODE_MAX_IDS; userCount++) {
    CC_UserCode_Write_ExpectAndReturn(userCount, &user_code, true);
    CC_UserCode_Write_ReturnThruPtr_userCodeData(&user_code);
  }

  received_frame_status_t status;
  status = invoke_cc_handler_v2(
      &rxOptions,
      &frame,
      commandLength,
      &frame_out,
      &frame_out_length);

  TEST_ASSERT_MESSAGE(status == RECEIVED_FRAME_STATUS_SUCCESS, "Wrong SET USER ZERO frame status :(");

  /*
   * USER  GET
   */
  for (userCount = 1; userCount <= CC_USER_CODE_MAX_IDS; userCount++)
  {
    user_code.user_id_status = USER_ID_AVAILABLE;
    memset(user_code.userCode, 0, 4);
    user_code.userCodeLen = 4;

    memset(pFrame, 0x00, sizeof(ZW_APPLICATION_TX_BUFFER));
    byteCount = 0;
    pFrame[byteCount++] = 0x63; // COMMAND_CLASS_USER_CODE
    pFrame[byteCount++] = 0x02; // USER_CODE_GET
    pFrame[byteCount++] = userCount;
    commandLength = byteCount;

    CC_UserCode_Read_ExpectAndReturn(userCount, NULL, true);
    CC_UserCode_Read_IgnoreArg_userCodeData();
    CC_UserCode_Read_ReturnThruPtr_userCodeData(&user_code);

    CC_UserCode_Read_ExpectAndReturn(userCount, NULL, true);
    CC_UserCode_Read_IgnoreArg_userCodeData();
    CC_UserCode_Read_ReturnThruPtr_userCodeData(&user_code);

    mock_call_use_as_stub(TO_STR(Check_not_legal_response_job));

    uint8_t expectedFrame[] = {
        0x63, // COMMAND_CLASS_USER_CODE
        0x03, // USER_CODE_REPORT
        userCount,
        USER_ID_AVAILABLE,
        0,
        0,
        0,
        0
    };

    received_frame_status_t status;
    status = invoke_cc_handler_v2(
        &rxOptions,
        &frame,
        commandLength,
        &frame_out,
        &frame_out_length);

    char error_msg[100];
    sprintf(error_msg, "Wrong USER GET %d frame status :(", userCount);
    TEST_ASSERT_MESSAGE(status == RECEIVED_FRAME_STATUS_SUCCESS, error_msg);
    TEST_ASSERT_EQUAL_UINT8(sizeof(expectedFrame), frame_out_length);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expectedFrame, (uint8_t *)&frame_out, frame_out_length);
  }

  mock_calls_verify();
}

/**************************************************************************************************
 *
 **************************************************************************************************
 */
void test_USERS_NUMBER_GET_tx_failure(void)
{
  mock_calls_clear();

  RECEIVE_OPTIONS_TYPE_EX rxOptions;
  memset((uint8_t *)&rxOptions, 0x00, sizeof(rxOptions));
  ZW_APPLICATION_TX_BUFFER frame;
  uint8_t * pFrame = (uint8_t *)&frame;
  uint8_t commandLength;
  uint8_t byteCount;

  memset(pFrame, 0x00, sizeof(ZW_APPLICATION_TX_BUFFER));
  byteCount = 0;
  pFrame[byteCount++] = 0x63; // COMMAND_CLASS_USER_CODE
  pFrame[byteCount++] = 0x04; // USERS_NUMBER_GET
  commandLength = byteCount;

  mock_call_use_as_stub(TO_STR(Check_not_legal_response_job));

  uint8_t expectedFrame[] = {
      0x63, // COMMAND_CLASS_USER_CODE
      0x05, // USERS_NUMBER_REPORT
      CC_USER_CODE_MAX_IDS
  };

  ZW_APPLICATION_TX_BUFFER frame_out;
  uint8_t frame_out_length = 0;

  received_frame_status_t status;
  status = invoke_cc_handler_v2(
      &rxOptions,
      &frame,
      commandLength,
      &frame_out,
      &frame_out_length);

  TEST_ASSERT_MESSAGE(status == RECEIVED_FRAME_STATUS_SUCCESS, "Wrong frame status :(");
  TEST_ASSERT_EQUAL_UINT8(sizeof(expectedFrame), frame_out_length);
  TEST_ASSERT_EQUAL_UINT8_ARRAY(expectedFrame, (uint8_t *)&frame_out, frame_out_length);

  mock_calls_verify();
}

/**************************************************************************************************
 *
 **************************************************************************************************
 */
void test_USERS_NUMBER_GET_tx_success(void)
{
  mock_calls_clear();

  RECEIVE_OPTIONS_TYPE_EX rxOptions;
  memset((uint8_t *)&rxOptions, 0x00, sizeof(rxOptions));
  ZW_APPLICATION_TX_BUFFER frame;
  uint8_t * pFrame = (uint8_t *)&frame;
  uint8_t commandLength;
  uint8_t byteCount;

  memset(pFrame, 0x00, sizeof(ZW_APPLICATION_TX_BUFFER));
  byteCount = 0;
  pFrame[byteCount++] = 0x63; // COMMAND_CLASS_USER_CODE
  pFrame[byteCount++] = 0x04; // USERS_NUMBER_GET
  commandLength = byteCount;

  mock_call_use_as_stub(TO_STR(Check_not_legal_response_job));

  uint8_t expectedFrame[] = {
      0x63, // COMMAND_CLASS_USER_CODE
      0x05, // USER_CODE_REPORT
      CC_USER_CODE_MAX_IDS
  };

  ZW_APPLICATION_TX_BUFFER frame_out;
  uint8_t frame_out_length = 0;

  received_frame_status_t status;
  status = invoke_cc_handler_v2(
      &rxOptions,
      &frame,
      commandLength,
      &frame_out,
      &frame_out_length);

  TEST_ASSERT_MESSAGE(status == RECEIVED_FRAME_STATUS_SUCCESS, "Wrong frame status :(");
  TEST_ASSERT_EQUAL_UINT8(sizeof(expectedFrame), frame_out_length);
  TEST_ASSERT_EQUAL_UINT8_ARRAY(expectedFrame, (uint8_t *)&frame_out, frame_out_length);

  mock_calls_verify();
}

/**************************************************************************************************
 *
 **************************************************************************************************
 */
void test_USER_CODE_GET_no_user_code(void)
{
  const uint8_t USER_IDENTIFIER = 1; // Just a random valid user ID.

  SUserCode user_code;
  memset(&user_code, 0, sizeof(user_code));
  user_code.user_id_status = USER_ID_AVAILABLE;

  mock_calls_clear();

  RECEIVE_OPTIONS_TYPE_EX rxOptions;
  memset((uint8_t *)&rxOptions, 0x00, sizeof(rxOptions));
  ZW_APPLICATION_TX_BUFFER frame;
  uint8_t * pFrame = (uint8_t *)&frame;
  uint8_t commandLength;
  uint8_t byteCount;

  memset(pFrame, 0x00, sizeof(ZW_APPLICATION_TX_BUFFER));
  byteCount = 0;
  pFrame[byteCount++] = 0x63; // COMMAND_CLASS_USER_CODE
  pFrame[byteCount++] = 0x02; // USER_CODE_GET
  pFrame[byteCount++] = USER_IDENTIFIER;
  commandLength = byteCount;

  CC_UserCode_Read_ExpectAndReturn(1, NULL, true);
  CC_UserCode_Read_IgnoreArg_userCodeData();
  CC_UserCode_Read_ReturnThruPtr_userCodeData(&user_code);

  CC_UserCode_Read_ExpectAndReturn(1, NULL, true);
  CC_UserCode_Read_IgnoreArg_userCodeData();
  CC_UserCode_Read_ReturnThruPtr_userCodeData(&user_code);

  mock_call_use_as_stub(TO_STR(Check_not_legal_response_job));
  uint8_t expectedFrame[4] = {
      0x63, // COMMAND_CLASS_USER_CODE
      0x03, // USER_CODE_REPORT
      USER_IDENTIFIER,
      USER_ID_AVAILABLE,
  };

  ZW_APPLICATION_TX_BUFFER frame_out;
  uint8_t frame_out_length = 0;

  received_frame_status_t status;
  status = invoke_cc_handler_v2(
      &rxOptions,
      &frame,
      commandLength,
      &frame_out,
      &frame_out_length);

  TEST_ASSERT_MESSAGE(status == RECEIVED_FRAME_STATUS_SUCCESS, "Wrong frame status :(");
  TEST_ASSERT_EQUAL_UINT8(sizeof(expectedFrame), frame_out_length);
  TEST_ASSERT_EQUAL_UINT8_ARRAY(expectedFrame, (uint8_t *)&frame_out, frame_out_length);

  mock_calls_verify();
}

/**************************************************************************************************
 *
 **************************************************************************************************
 */
void test_USER_CODE_SET_user_identifier_too_high(void)
{
  mock_calls_clear();

  RECEIVE_OPTIONS_TYPE_EX rxOptions;
  memset((uint8_t *)&rxOptions, 0x00, sizeof(rxOptions));
  ZW_APPLICATION_TX_BUFFER frame;
  uint8_t * pFrame = (uint8_t *)&frame;
  uint8_t commandLength;

  const uint8_t USER_IDENTIFIER = 10;

  // Create a frame
  memset(pFrame, 0x00, sizeof(ZW_APPLICATION_TX_BUFFER));
  uint8_t frameCount = 0;
  pFrame[frameCount++] = 0x63; // COMMAND_CLASS_USER_CODE
  pFrame[frameCount++] = 0x01; // USER_CODE_SET
  pFrame[frameCount++] = USER_IDENTIFIER;
  pFrame[frameCount++] = 0x01; // USER_ID_OCCUPIED
  pFrame[frameCount++] = '1'; // Random valid character
  pFrame[frameCount++] = '1'; // Random valid character
  pFrame[frameCount++] = '1'; // Random valid character
  pFrame[frameCount++] = '1'; // Random valid character
  commandLength = frameCount;

  ZW_APPLICATION_TX_BUFFER frame_out;
  uint8_t frame_out_length = 0;

  received_frame_status_t status;
  status = invoke_cc_handler_v2(
      &rxOptions,
      &frame,
      commandLength,
      &frame_out,
      &frame_out_length);

  TEST_ASSERT_MESSAGE(status == RECEIVED_FRAME_STATUS_FAIL, "Wrong frame status :(");

  mock_calls_verify();
}

/**************************************************************************************************
 *
 **************************************************************************************************
 */
void test_USER_CODE_SET_unset_user_code(void)
{
  mock_t* pMock = NULL;
  SUserCode user_code;
  mock_calls_clear();

  RECEIVE_OPTIONS_TYPE_EX rxOptions;
  memset((uint8_t *)&rxOptions, 0x00, sizeof(rxOptions));
  ZW_APPLICATION_TX_BUFFER frame;
  uint8_t * pFrame = (uint8_t *)&frame;
  uint8_t commandLength;

  const uint8_t USER_IDENTIFIER = 1;

  /*
   * USER  SET
   */
  memset(&user_code, 0, sizeof(user_code));
  user_code.user_id_status = USER_ID_AVAILABLE;
  memset(user_code.userCode, 0, 4);
  user_code.userCodeLen = 4;

  // Create a frame
  memset(pFrame, 0x00, sizeof(ZW_APPLICATION_TX_BUFFER));
  uint8_t frameCount = 0;
  pFrame[frameCount++] = 0x63; // COMMAND_CLASS_USER_CODE
  pFrame[frameCount++] = USER_CODE_SET,
  pFrame[frameCount++] = USER_IDENTIFIER;
  pFrame[frameCount++] = USER_ID_AVAILABLE,
  pFrame[frameCount++] = 'a'; // Random invalid character
  pFrame[frameCount++] = 'b'; // Random invalid character
  pFrame[frameCount++] = 'c'; // Random invalid character
  pFrame[frameCount++] = 'd'; // Random invalid character
  commandLength = frameCount;

  CC_UserCode_Write_ExpectAndReturn(USER_IDENTIFIER, &user_code, true);
  CC_UserCode_Write_ReturnThruPtr_userCodeData(&user_code);

  mock_call_expect(TO_STR(ZAF_TSE_Trigger), &pMock);
  pMock->compare_rule_arg[0] = COMPARE_NOT_NULL;
  pMock->compare_rule_arg[1] = COMPARE_NOT_NULL;
  pMock->expect_arg[2].v = false;       //Overwrite_previous_trigger
  pMock->return_code.v = true;

  ZW_APPLICATION_TX_BUFFER frame_out;
  uint8_t frame_out_length = 0;

  received_frame_status_t status;
  status = invoke_cc_handler_v2(
      &rxOptions,
      &frame,
      commandLength,
      &frame_out,
      &frame_out_length);

  TEST_ASSERT_MESSAGE(status = RECEIVED_FRAME_STATUS_SUCCESS, "Wrong frame status :(");

  // Fetch the callback passed to ZAF_TSE_Trigger().
  zaf_tse_callback_t cb = (zaf_tse_callback_t)pMock->actual_arg[0].p;

  zaf_tx_options_t txOptions;
  s_CC_userCode_data_t pData;
  pData.userIdentifier = USER_IDENTIFIER;

  CC_UserCode_Read_ExpectAndReturn(pData.userIdentifier, NULL, true);
  CC_UserCode_Read_IgnoreArg_userCodeData();
  CC_UserCode_Read_ReturnThruPtr_userCodeData(&user_code);

  CC_UserCode_Read_ExpectAndReturn(pData.userIdentifier, NULL, true);
  CC_UserCode_Read_IgnoreArg_userCodeData();
  CC_UserCode_Read_ReturnThruPtr_userCodeData(&user_code);

  uint8_t expectedData[] = {
    COMMAND_CLASS_USER_CODE,
    USER_CODE_REPORT,
    pData.userIdentifier,
    USER_ID_AVAILABLE,
    0,
    0,
    0,
    0
  };

  memset((uint8_t*)&txOptions, 0x00, sizeof(zaf_tx_options_t));
  zaf_transport_tx_ExpectAndReturn(expectedData, sizeof(expectedData),
                                    ZAF_TSE_TXCallback, &txOptions, true);
  cb(&txOptions, &pData);

  mock_calls_verify();
}

/**************************************************************************************************
 *
 **************************************************************************************************
 */
void test_CmdClassUserCodeSupportReport(void)
{
  mock_t* pMock = NULL;
  mock_calls_clear();

  const uint8_t USER_CODE[] = { '3', '4', '9', '4' };

  JOB_STATUS s;

  // Test user ID
  s = CC_UserCode_SupportReport(NULL, 0, 0, 0, (uint8_t *)USER_CODE, sizeof(USER_CODE), NULL);
  TEST_ASSERT_EQUAL_MESSAGE(1, s, "Damn!");

  // Test user ID status
  s = CC_UserCode_SupportReport(NULL, 0, 1, 3, (uint8_t *)USER_CODE, sizeof(USER_CODE), NULL);
  TEST_ASSERT_EQUAL_MESSAGE(1, s, "Damn!");

  // Test user code
  s = CC_UserCode_SupportReport(NULL, 0, 1, 0, NULL, sizeof(USER_CODE), NULL);
  TEST_ASSERT_EQUAL_MESSAGE(1, s, "Damn!");

  // Test user code length max
  s = CC_UserCode_SupportReport(NULL, 0, 1, 0, (uint8_t *)USER_CODE, 11, NULL);
  TEST_ASSERT_EQUAL_MESSAGE(1, s, "Damn!");

  // Test user code length min
  s = CC_UserCode_SupportReport(NULL, 0, 1, 0, (uint8_t *)USER_CODE, 3, NULL);
  TEST_ASSERT_EQUAL_MESSAGE(1, s, "Damn!");

  // Test that cc_engine_multicast_request is called with the right arguments.
  const agi_profile_t AGI_PROFILE = {0xAA, 0xBB};
  const uint8_t SOURCE_ENDPOINT = 5; // Random value
  const uint8_t USER_ID = 3; // Random value
  const uint8_t USER_ID_STATUS = 2; // Random value

  cc_group_t cmdGrpExpected = {
                                  0x63,
                                  0x03
  };

  uint8_t expectedPayload[] = { USER_ID, USER_ID_STATUS, '3', '4', '9', '4' };

  mock_call_expect(TO_STR(cc_engine_multicast_request), &pMock);
  pMock->expect_arg[0].p = (agi_profile_t *)&AGI_PROFILE;
  pMock->expect_arg[1].v = SOURCE_ENDPOINT; // ENDPOINT_ROOT
  pMock->expect_arg[2].p = &cmdGrpExpected;
  pMock->expect_arg[3].p = expectedPayload; // COMPARE_ANY?
  pMock->expect_arg[4].v = 6;
  pMock->expect_arg[5].v = 0;
  pMock->expect_arg[6].p = NULL;
  pMock->return_code.v = 1; // JOB_STATUS_BUSY

  CC_UserCode_SupportReport(
      (agi_profile_t *)&AGI_PROFILE,
      SOURCE_ENDPOINT,
      USER_ID,
      USER_ID_STATUS,
      (uint8_t *)USER_CODE,
      sizeof(USER_CODE),
      NULL);

  mock_calls_verify();
}

static command_handler_input_t * user_code_get_frame_create(uint8_t userID)
{
  command_handler_input_t * p_chi = test_common_command_handler_input_allocate();
  p_chi->frame.as_byte_array[p_chi->frameLength++] = COMMAND_CLASS_USER_CODE;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = USER_CODE_GET;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = userID;
  return p_chi;
}
