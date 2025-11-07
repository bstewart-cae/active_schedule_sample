/**
 * @file test_CC_AssociationGroupInfo.c
 * @copyright 2022 Silicon Laboratories Inc.
 */

#include <ZW_typedefs.h>
#include <CC_AssociationGroupInfo.h>
#include <string.h>
#include <unity.h>
#include <mock_control.h>
#include <test_common.h>
#include "ZAF_CC_Invoker.h"
#include <endpoints_groups_associations_helper.h>
#include <cc_agi_config_api_mock.h>
#include <association_plus_base.h>
#include "SizeOf.h"
#include "zaf_transport_tx_mock.h"

// This AGI config can be used for tests that don't require other groups than lifeline.
const cc_agi_config_t AGI_CONFIG_LIFELINE_ONLY = {
                          .groups = NULL,
                          .group_count = 0
};

static zaf_tx_callback_t g_zaf_tx_callback = NULL;

static bool
zaf_transport_tx_callback(const uint8_t* frame, uint8_t frame_length, 
                          zaf_tx_callback_t callback, 
                          zaf_tx_options_t* zaf_tx_options, int cmock_num_calls)
{
  (void) frame;
  (void) frame_length;
  (void) zaf_tx_options;
  (void) cmock_num_calls;

  g_zaf_tx_callback = callback;

  return true;
}

void setUpSuite(void) {

}

void tearDownSuite(void) {

}

void setUp(void) {

}

void tearDown(void) {

}


#define CC_AGI_handler(a,b,c,d,e) invoke_cc_handler_v2(a,b,c,d,e)

extern bool GetApplGroupCommandList(
    uint8_t * pGroupList,
    size_t * pGroupListSize,
    uint8_t groupId,
    uint8_t endpoint);

static void association_group_name_get_frame_create(
    uint8_t * pFrame,
    uint8_t * pFrameLength,
    uint8_t groupID);

static void association_group_command_list_get_frame_create(
    command_handler_input_t * pCommandHandlerInput,
    bool allowCache,
    uint8_t groupID);

static command_handler_input_t * association_group_info_get_frame_create(
    bool refreshCache,
    bool listMode,
    uint8_t groupID);

void test_ASSOCIATION_GROUP_NAME_GET_no_group_ID(void)
{
  mock_calls_clear();

  RECEIVE_OPTIONS_TYPE_EX rxOptions;
  memset((uint8_t *)&rxOptions, 0x00, sizeof(RECEIVE_OPTIONS_TYPE_EX));
  ZW_APPLICATION_TX_BUFFER frame;
  uint8_t * pFrame = (uint8_t *)&frame;
  uint8_t commandLength = 0;

  const uint8_t UNIMPORTANT_GROUP_ID = 1;

  cc_agi_get_config_ExpectAndReturn(&AGI_CONFIG_LIFELINE_ONLY);
  ZAF_CC_init_specific(COMMAND_CLASS_ASSOCIATION_GRP_INFO);

  association_group_name_get_frame_create(pFrame, &commandLength, UNIMPORTANT_GROUP_ID);

  mock_call_use_as_stub(TO_STR(Check_not_legal_response_job));

  // Force command length to 2 to discard group ID.
  commandLength = 2;

  CC_AGI_handler(
      &rxOptions,
      &frame,
      commandLength,
      NULL,
      NULL);

  mock_calls_verify();
}


/**
 * Tests whether the name of the lifeline for the root device is the one returned when a valid request
 * with group ID 1 is made - happy path test
 */
void test_ASSOCIATION_GROUP_NAME_GET_transmit_success(void)
{
  mock_t* pMock = NULL;
  mock_calls_clear();

  RECEIVE_OPTIONS_TYPE_EX rxOptions;
  memset((uint8_t *)&rxOptions, 0x00, sizeof(RECEIVE_OPTIONS_TYPE_EX));
  ZW_APPLICATION_TX_BUFFER inframe;
  ZW_APPLICATION_TX_BUFFER outframe;
  uint8_t * pFrame = (uint8_t *)&inframe;
  memset(pFrame, 0x00, sizeof(ZW_APPLICATION_TX_BUFFER));
  uint8_t commandLength = 0;
  uint8_t frameLength = 0;
  const uint8_t GROUP_ID = 1;

  association_group_name_get_frame_create(pFrame, &commandLength, GROUP_ID);


  mock_call_expect(TO_STR(Check_not_legal_response_job), &pMock);
  pMock->expect_arg[0].p = &rxOptions;
  pMock->return_code.v = false;

  cc_agi_get_config_ExpectAndReturn(&AGI_CONFIG_LIFELINE_ONLY);
  ZAF_CC_init_specific(COMMAND_CLASS_ASSOCIATION_GRP_INFO);

  CC_AGI_handler(
      &rxOptions,
      &inframe,
      commandLength,
      &outframe,
      &frameLength);
  mock_calls_verify();

  // sizeof(ZW_ASSOCIATION_GROUP_NAME_REPORT_1BYTE_FRAME) - sizeof(uint8_t) + (uint8_t)(strlen("Lifeline"))
  const uint8_t EXPECTED_FRAME_SIZE = 12;

  const uint8_t EXPECTED_FRAME[] = {
                              COMMAND_CLASS_ASSOCIATION_GRP_INFO,
                              ASSOCIATION_GROUP_NAME_REPORT,
                              0x01, 
                              0x08, 
                              'L',
                              'i',
                              'f',
                              'e',
                              'l',
                              'i',
                              'n',
                              'e'
};

  TEST_ASSERT_EQUAL_UINT8_ARRAY(EXPECTED_FRAME, (const uint8_t *)&(outframe), sizeof(EXPECTED_FRAME));
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(EXPECTED_FRAME_SIZE, frameLength,"Wrong inputs.");

}

/**
 * Tests whether the name of the lifeline for the root device is the one returned when asking
 * for lifeline on an endpoint.
 */
void test_ASSOCIATION_GROUP_NAME_GET_endpoint_and_lifeline(void)
{
  mock_t* pMock = NULL;
  mock_calls_clear();

  RECEIVE_OPTIONS_TYPE_EX rxOptions;
  memset((uint8_t *)&rxOptions, 0x00, sizeof(RECEIVE_OPTIONS_TYPE_EX));
  ZW_APPLICATION_TX_BUFFER inframe;
  ZW_APPLICATION_TX_BUFFER outframe;
  uint8_t * pFrame = (uint8_t *)&inframe;
  memset(pFrame, 0x00, sizeof(ZW_APPLICATION_TX_BUFFER));
  uint8_t commandLength = 0;
  uint8_t frameLength = 0;
  const uint8_t GROUP_ID = 1; // Lifeline

  cc_agi_get_config_ExpectAndReturn(&AGI_CONFIG_LIFELINE_ONLY);
  ZAF_CC_init_specific(COMMAND_CLASS_ASSOCIATION_GRP_INFO);

  association_group_name_get_frame_create(pFrame, &commandLength, GROUP_ID);

  uint8_t endpoint;
  for (endpoint = 0; endpoint <= ZAF_CONFIG_NUMBER_OF_END_POINTS; endpoint++)
  {
    rxOptions.destNode.endpoint = endpoint;


    mock_call_expect(TO_STR(Check_not_legal_response_job), &pMock);
    pMock->expect_arg[0].p = &rxOptions;
    pMock->return_code.v = false;

    CC_AGI_handler(
        &rxOptions,
        &inframe,
        commandLength,
        &outframe,
        &frameLength);
    mock_calls_verify();

    //sizeof(ZW_ASSOCIATION_GROUP_NAME_REPORT_1BYTE_FRAME) - sizeof(uint8_t) + (uint8_t)(strlen("Lifeline"))
    const uint8_t EXPECTED_FRAME_SIZE = 12;
    const uint8_t EXPECTED_FRAME[] = {
                              COMMAND_CLASS_ASSOCIATION_GRP_INFO,
                              ASSOCIATION_GROUP_NAME_REPORT,
                              0x01,
                              0x08,
                              'L',
                              'i',
                              'f',
                              'e',
                              'l',
                              'i',
                              'n',
                              'e'
    };

    TEST_ASSERT_EQUAL_UINT8_ARRAY(EXPECTED_FRAME, (const uint8_t *)&(outframe), sizeof(EXPECTED_FRAME));
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(EXPECTED_FRAME_SIZE, frameLength,"Wrong inputs.");
  } 
}

/**
 * Tests whether the name of the lifeline for the root device is the one returned when asking
 * for lifeline on an endpoint.
 */
void test_ASSOCIATION_GROUP_NAME_GET_lifeline_specific_name(void)
{
  mock_t* pMock = NULL;
  mock_calls_clear();

  RECEIVE_OPTIONS_TYPE_EX rxOptions;
  memset((uint8_t *)&rxOptions, 0x00, sizeof(RECEIVE_OPTIONS_TYPE_EX));
  ZW_APPLICATION_TX_BUFFER inframe;
  ZW_APPLICATION_TX_BUFFER outframe;
  uint8_t * pFrame = (uint8_t *)&inframe;
  memset(pFrame, 0x00, sizeof(ZW_APPLICATION_TX_BUFFER));
  uint8_t commandLength = 0;
  uint8_t frameLength = 0;

  const uint8_t GROUP_ID = 1; // Lifeline
  rxOptions.destNode.endpoint = 0; // Some endpoint not being zero (root device).

  association_group_name_get_frame_create(pFrame, &commandLength, GROUP_ID);


  mock_call_expect(TO_STR(Check_not_legal_response_job), &pMock);
  pMock->expect_arg[0].p = &rxOptions;
  pMock->return_code.v = false;

  cc_agi_get_config_ExpectAndReturn(myAgi);
  ZAF_CC_init_specific(COMMAND_CLASS_ASSOCIATION_GRP_INFO);

  CC_AGI_handler(
      &rxOptions,
      &inframe,
      commandLength,
      &outframe,
      &frameLength);
  mock_calls_verify();

  //sizeof(ZW_ASSOCIATION_GROUP_NAME_REPORT_1BYTE_FRAME) - sizeof(uint8_t) + (uint8_t)(strlen("Lifeline"))
  const uint8_t EXPECTED_FRAME_SIZE = 12;

  const uint8_t EXPECTED_FRAME[] = {
                              COMMAND_CLASS_ASSOCIATION_GRP_INFO,
                              ASSOCIATION_GROUP_NAME_REPORT,
                              0x01,
                              0x08,
                              'L',
                              'i',
                              'f',
                              'e',
                              'l',
                              'i',
                              'n',
                              'e'
  };

  TEST_ASSERT_EQUAL_UINT8_ARRAY(EXPECTED_FRAME, (const uint8_t *)&(outframe), sizeof(EXPECTED_FRAME));
 
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(EXPECTED_FRAME_SIZE, frameLength, "Wrong inputs.");
}

/**
 * Tests whether the name of the lifeline for the root device is the one returned when asking
 * for an unsupported group identifier 
 * Application Workgroup Specifications -  CC:0059.01.01.12.001
 */
void test_ASSOCIATION_GROUP_NAME_GET_GROUPID0(void)
{
  mock_t* pMock = NULL;
  mock_calls_clear();

  RECEIVE_OPTIONS_TYPE_EX rxOptions;
  memset((uint8_t *)&rxOptions, 0x00, sizeof(RECEIVE_OPTIONS_TYPE_EX));
  ZW_APPLICATION_TX_BUFFER inframe;
  ZW_APPLICATION_TX_BUFFER outframe;
  uint8_t * pFrame = (uint8_t *)&inframe;
  memset(pFrame, 0x00, sizeof(ZW_APPLICATION_TX_BUFFER));
  uint8_t commandLength = 0;
  uint8_t frameLength = 0;

  const uint8_t GROUP_ID = 0; // Lifeline
  rxOptions.destNode.endpoint = 0; // Some endpoint not being zero (root device).

  association_group_name_get_frame_create(pFrame, &commandLength, GROUP_ID);


  mock_call_expect(TO_STR(Check_not_legal_response_job), &pMock);
  pMock->expect_arg[0].p = &rxOptions;
  pMock->return_code.v = false;

  const ccc_pair_t CCC_PAIRS[] = {
                                  {
                                   .cmdClass = COMMAND_CLASS_BASIC,
                                   .cmd = BASIC_SET
                                  }
  };

  const cc_agi_group_t agiTableRootDeviceGroups[] = {
                                          {{0x20, 0x01}, sizeof_array(CCC_PAIRS), CCC_PAIRS, "Basic Control"},
  };

  const cc_agi_config_t customAgiTable[] =
  {
    {
      .groups = agiTableRootDeviceGroups,
      .group_count = sizeof_array(agiTableRootDeviceGroups)
    }
  };

  cc_agi_get_config_ExpectAndReturn(customAgiTable);
  ZAF_CC_init_specific(COMMAND_CLASS_ASSOCIATION_GRP_INFO);

  CC_AGI_handler(
      &rxOptions,
      (ZW_APPLICATION_TX_BUFFER*) pFrame,
      commandLength,
      &outframe,
      &frameLength);
  mock_calls_verify();

  // sizeof(ZW_ASSOCIATION_GROUP_NAME_REPORT_1BYTE_FRAME) - sizeof(uint8_t) + (uint8_t)(strlen("Lifeline"))
  const uint8_t EXPECTED_FRAME_SIZE = 12;
  const uint8_t EXPECTED_FRAME[] = {
                              COMMAND_CLASS_ASSOCIATION_GRP_INFO,
                              ASSOCIATION_GROUP_NAME_REPORT,
                              0x01,
                              0x08,
                              'L',
                              'i',
                              'f',
                              'e',
                              'l',
                              'i',
                              'n',
                              'e'
  };

  TEST_ASSERT_EQUAL_UINT8_ARRAY(EXPECTED_FRAME, (const uint8_t *)&(outframe), sizeof(EXPECTED_FRAME));
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(EXPECTED_FRAME_SIZE, frameLength,"Wrong inputs.");
}

/**
 * Tests whether the name of the requested group identifier is the one returned when asking
 * for an valid group identifier 
 */
void test_ASSOCIATION_GROUP_NAME_GET_GROUPID2(void)
{
  mock_t* pMock = NULL;
  mock_calls_clear();

  RECEIVE_OPTIONS_TYPE_EX rxOptions;
  memset((uint8_t *)&rxOptions, 0x00, sizeof(RECEIVE_OPTIONS_TYPE_EX));
  ZW_APPLICATION_TX_BUFFER inframe;
  ZW_APPLICATION_TX_BUFFER outframe;
  uint8_t * pFrame = (uint8_t *)&inframe;
  memset(pFrame, 0x00, sizeof(ZW_APPLICATION_TX_BUFFER));
  uint8_t commandLength = 0;
  uint8_t frameLength = 0;

  const uint8_t GROUP_ID = 2; 
  rxOptions.destNode.endpoint = 0; // Some endpoint not being zero (root device).

  association_group_name_get_frame_create(pFrame, &commandLength, GROUP_ID);

  const ccc_pair_t CCC_PAIRS[] = {
                                  {
                                   .cmdClass = COMMAND_CLASS_BASIC,
                                   .cmd = BASIC_SET
                                  }
  };

  const cc_agi_group_t agiTableRootDeviceGroups[] = {
                                          {{0x20, 0x01}, sizeof_array(CCC_PAIRS), CCC_PAIRS, "Basic Control"},
                                          {{0xAA, 0xBB}, sizeof_array(CCC_PAIRS), CCC_PAIRS, "BUTTON 1"},
                                          {{0xCC, 0xDD}, sizeof_array(CCC_PAIRS), CCC_PAIRS, "BUTTON 2"},
  };

  const cc_agi_config_t customAgiTable[] =
  {
    {
      .groups = agiTableRootDeviceGroups,
      .group_count = sizeof_array(agiTableRootDeviceGroups)
    }
  };

  cc_agi_get_config_ExpectAndReturn(customAgiTable);
  ZAF_CC_init_specific(COMMAND_CLASS_ASSOCIATION_GRP_INFO);

  mock_call_expect(TO_STR(Check_not_legal_response_job), &pMock);
  pMock->expect_arg[0].p = &rxOptions;
  pMock->return_code.v = false;

  CC_AGI_handler(
      &rxOptions,
      (ZW_APPLICATION_TX_BUFFER*) pFrame,
      commandLength,
      &outframe,
      &frameLength);
  mock_calls_verify();

  //(uint8_t)sizeof(ZW_ASSOCIATION_GROUP_NAME_REPORT_1BYTE_FRAME) - sizeof(uint8_t) + (uint8_t)(strlen("Basic Control"))
  const uint8_t EXPECTED_FRAME_SIZE = 17;
  const uint8_t EXPECTED_FRAME[] = {
                              COMMAND_CLASS_ASSOCIATION_GRP_INFO,
                              ASSOCIATION_GROUP_NAME_REPORT,
                              0x02,
                              0x0D,
                              'B',
                              'a',
                              's',
                              'i',
                              'c',
                              ' ',
                              'C',
                              'o',
                              'n',
                              't',
                              'r',
                              'o',
                              'l'
  };

  TEST_ASSERT_EQUAL_UINT8_ARRAY(EXPECTED_FRAME, (const uint8_t *)&(outframe), sizeof(EXPECTED_FRAME));
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(EXPECTED_FRAME_SIZE, frameLength, "Wrong inputs.");
}

void test_ASSOCIATION_GROUP_COMMAND_LIST_GET_invalid_endpoint(void)
{
  command_handler_input_t chi_ag_command_list_get;
  test_common_clear_command_handler_input(&chi_ag_command_list_get);

  const uint8_t UNIMPORTANT_GROUP_ID = 1;

  chi_ag_command_list_get.rxOptions.destNode.endpoint = 100; // Invalid endpoint

  association_group_command_list_get_frame_create(
      &chi_ag_command_list_get,
      false,
      UNIMPORTANT_GROUP_ID);

  mock_call_use_as_stub(TO_STR(Check_not_legal_response_job));

  const ccc_pair_t CCC_PAIRS[] = {
                                  {
                                   .cmdClass = COMMAND_CLASS_BASIC,
                                   .cmd = BASIC_SET
                                  }
  };

  const cc_agi_group_t agiTableRootDeviceGroups[] = {
                                          {{0x20, 0x01}, sizeof_array(CCC_PAIRS), CCC_PAIRS, "Basic Control"},
                                          {{0xAA, 0xBB}, sizeof_array(CCC_PAIRS), CCC_PAIRS, "BUTTON 1"},
                                          {{0xCC, 0xDD}, sizeof_array(CCC_PAIRS), CCC_PAIRS, "BUTTON 2"},
  };

  const cc_agi_config_t customAgiTable[] =
  {
    {
      .groups = agiTableRootDeviceGroups,
      .group_count = sizeof_array(agiTableRootDeviceGroups)
    }
  };

  cc_agi_get_config_ExpectAndReturn(customAgiTable);
  ZAF_CC_init_specific(COMMAND_CLASS_ASSOCIATION_GRP_INFO);

  received_frame_status_t status;
  status = CC_AGI_handler(
      &chi_ag_command_list_get.rxOptions,
      &chi_ag_command_list_get.frame.as_zw_application_tx_buffer,
      chi_ag_command_list_get.frameLength,
      NULL,
      NULL);

  TEST_ASSERT_MESSAGE(RECEIVED_FRAME_STATUS_FAIL == status, "Wrong frame status :(");
}

void test_ASSOCIATION_GROUP_INFO_GET_nonlist_mode(void)
{
  mock_calls_clear();

  RECEIVE_OPTIONS_TYPE_EX rxOptions;
  memset((uint8_t *)&rxOptions, 0x00, sizeof(RECEIVE_OPTIONS_TYPE_EX));

  const uint8_t GROUP_ID = 1;

  command_handler_input_t * p_chi = association_group_info_get_frame_create(
      false,
      false, // Non-list mode
      GROUP_ID);

  mock_call_use_as_stub(TO_STR(Check_not_legal_response_job));

  zaf_transport_rx_to_tx_options_Expect(NULL, NULL);
  zaf_transport_rx_to_tx_options_IgnoreArg_rx_options();
  zaf_transport_rx_to_tx_options_IgnoreArg_tx_options();

  cc_agi_get_config_ExpectAndReturn(&AGI_CONFIG_LIFELINE_ONLY);
  ZAF_CC_init_specific(COMMAND_CLASS_ASSOCIATION_GRP_INFO);

  uint8_t expectedFrame[] = {
      0x59,
      0x04,
      0x01, // List mode = false, Dynamic Info = false & Group count = 1
      GROUP_ID,
      0, // Mode
      0x00, // Profile MSB: ASSOCIATION_GROUP_INFO_REPORT_AGI_PROFILE_GENERAL
      0x01, // Profile LSB: ASSOCIATION_GROUP_INFO_REPORT_AGI_GENERAL_LIFELINE
      0x00, // Reserved
      0x00, // Event code MSB
      0x00 // Event code LSB
  };

  zaf_transport_tx_AddCallback(zaf_transport_tx_callback);
  zaf_transport_tx_ExpectAndReturn(expectedFrame, sizeof(expectedFrame), NULL, NULL, true);
  zaf_transport_tx_IgnoreArg_zaf_tx_options();
  zaf_transport_tx_IgnoreArg_callback();

  received_frame_status_t status;

  status = CC_AGI_handler(
      &p_chi->rxOptions,
      &p_chi->frame.as_zw_application_tx_buffer,
      p_chi->frameLength,
      NULL,
      NULL);

  TEST_ASSERT(0xFF == status);

  g_zaf_tx_callback(0);

  test_common_command_handler_input_free(p_chi);

  mock_calls_verify();
}

void test_ASSOCIATION_GROUP_INFO_GET_list_mode(void)
{
  mock_t * pMock = NULL;
  mock_t * pMockAppTimerRegister = NULL;
  mock_calls_clear();

  void (*TimerStart_callback)(void);

  RECEIVE_OPTIONS_TYPE_EX rxOptions;
  memset((uint8_t *)&rxOptions, 0x00, sizeof(RECEIVE_OPTIONS_TYPE_EX));

  const ccc_pair_t CCC_PAIRS[] = {
                                  {
                                   .cmdClass = COMMAND_CLASS_BASIC,
                                   .cmd = BASIC_SET
                                  }
  };

  const cc_agi_group_t agiTableRootDeviceGroups[] = {
                                          {{0xAA, 0xBB}, sizeof_array(CCC_PAIRS), CCC_PAIRS, "Group 2"},
                                          {{0xCC, 0xDD}, sizeof_array(CCC_PAIRS), CCC_PAIRS, "Group 3"},
                                          {{0xEE, 0xFF}, sizeof_array(CCC_PAIRS), CCC_PAIRS, "Group 4"},
                                          {{0xAB, 0xCD}, sizeof_array(CCC_PAIRS), CCC_PAIRS, "Group 5"}
  };

  const cc_agi_config_t customAgiTable[] =
  {
    {
      .groups = agiTableRootDeviceGroups,
      .group_count = sizeof_array(agiTableRootDeviceGroups)
    }
  };

  cc_agi_get_config_ExpectAndReturn(customAgiTable);
  ZAF_CC_init_specific(COMMAND_CLASS_ASSOCIATION_GRP_INFO);

  const uint8_t GROUP_ID = 213; // Random value since it MUST be ignored.

  command_handler_input_t * p_chi = association_group_info_get_frame_create(
      false,
      true, // List mode
      GROUP_ID);

#ifdef NOT_USED
  uint8_t j;
  for (j = 0; j < commandLength; j++)
  {
    printf("%#02x ", pFrame[j]);
  }
#endif

  mock_call_use_as_stub(TO_STR(ZAF_GetInclusionMode));

  mock_call_use_as_stub(TO_STR(Check_not_legal_response_job));

  agi_profile_t agiProfile[5] = {
                                 {0x00, 0x01},
                                 {0xAA, 0xBB},
                                 {0xCC, 0xDD},
                                 {0xEE, 0xFF},
                                 {0xAB, 0xCD}
  };

  uint8_t i;
  for (i = 0; i < 5; i++)
  {
    printf("Iteration: %d\n", i);

    zaf_transport_rx_to_tx_options_Expect(NULL, NULL);
    zaf_transport_rx_to_tx_options_IgnoreArg_rx_options();
    zaf_transport_rx_to_tx_options_IgnoreArg_tx_options();

    uint8_t expectedFrame[] = {
        0x59,
        0x04,
        0x81, // List mode = false, Dynamic Info = false & Group count = 1
        i + 1, // Group ID
        0, // Mode
        agiProfile[i].profile_MS,
        agiProfile[i].profile_LS,
        0x00, // Reserved
        0x00, // Event code MSB
        0x00 // Event code LSB
    };

    zaf_transport_tx_AddCallback(zaf_transport_tx_callback);
    zaf_transport_tx_ExpectAndReturn(expectedFrame, sizeof(expectedFrame), NULL, NULL, true);
    zaf_transport_tx_IgnoreArg_zaf_tx_options();
    zaf_transport_tx_IgnoreArg_callback();

    if (0 == i)
    {
      received_frame_status_t status;

      status = CC_AGI_handler(
          &p_chi->rxOptions,
          &p_chi->frame.as_zw_application_tx_buffer,
          p_chi->frameLength,
          NULL,
          NULL);

      TEST_ASSERT(0xFF == status);

      // Transmit the command once more to see that it is ignored.

      status = CC_AGI_handler(
          &p_chi->rxOptions,
          &p_chi->frame.as_zw_application_tx_buffer,
          p_chi->frameLength,
          NULL,
          NULL);

      TEST_ASSERT(0x02 == status);
    }
    else
    {
      TimerStart_callback = pMockAppTimerRegister->actual_arg[2].p;
      TimerStart_callback();
    }

    cc_agi_config_get_group_count_by_endpoint_ExpectAndReturn(p_chi->rxOptions.destNode.endpoint, customAgiTable->group_count);

    if (4 > i)
    {
      mock_call_expect(TO_STR(AppTimerRegister), &pMockAppTimerRegister);
      pMockAppTimerRegister->compare_rule_arg[0] = COMPARE_NOT_NULL;
      pMockAppTimerRegister->expect_arg[1].v = false;
      pMockAppTimerRegister->compare_rule_arg[2] = COMPARE_NOT_NULL;
      pMockAppTimerRegister->return_code.v = true; // Timer handle

      mock_call_expect(TO_STR(TimerStart), &pMock);
      pMock->compare_rule_arg[0] = COMPARE_NOT_NULL;
      pMock->expect_arg[1].v = 10; // 10 ms
    }

    g_zaf_tx_callback(0); // We don't care about the argument.
  }

  test_common_command_handler_input_free(p_chi);

  mock_calls_verify();
}

void test_AssociationGroupInfoGetReport_group_id_zero(void)
{
  mock_calls_clear();

  const uint8_t ENDPOINT = 0; // Root device

  // Group ID 0 is not valid and the info for group 1 must be returned.
  const uint8_t GROUP_ID = 0;
  const uint8_t GROUP_ID_EXPECTED = 1;


  /*
   * Set up frame
   */
  command_handler_input_t * p_chi = association_group_info_get_frame_create(false, false, GROUP_ID);
  p_chi->rxOptions.destNode.endpoint = ENDPOINT;

  mock_call_use_as_stub(TO_STR(Check_not_legal_response_job));

  cc_agi_get_config_ExpectAndReturn(&AGI_CONFIG_LIFELINE_ONLY);
  ZAF_CC_init_specific(COMMAND_CLASS_ASSOCIATION_GRP_INFO);

  const uint8_t EXPECTED_FRAME[] = {
                              COMMAND_CLASS_ASSOCIATION_GRP_INFO,
                              ASSOCIATION_GROUP_INFO_REPORT,
                              0x01, // List mode = 0, Dynamic info = 0, Group count = 1
                              GROUP_ID_EXPECTED,
                              0x00, // Mode
                              0x00, // Profile MSB
                              0x01, // Profile LSB
                              0x00, // Reserved
                              0x00, // Event Code MSB
                              0x00  // Event Code LSB
  };

  zaf_transport_rx_to_tx_options_Expect(NULL, NULL);
  zaf_transport_rx_to_tx_options_IgnoreArg_rx_options();
  zaf_transport_rx_to_tx_options_IgnoreArg_tx_options();

  zaf_transport_tx_ExpectAndReturn(EXPECTED_FRAME, sizeof(EXPECTED_FRAME), NULL, NULL, true);
  zaf_transport_tx_IgnoreArg_zaf_tx_options();
  zaf_transport_tx_IgnoreArg_callback();

  received_frame_status_t status;
  status = CC_AGI_handler(
      &p_chi->rxOptions,
      &p_chi->frame.as_zw_application_tx_buffer,
      p_chi->frameLength,
      NULL,
      NULL);

  TEST_ASSERT_MESSAGE(RECEIVED_FRAME_STATUS_SUCCESS == status, "Wrong frame status :(");

  test_common_command_handler_input_free(p_chi);

  mock_calls_verify();
}

/**
 * Creates a ASSOCIATION_GROUP_NAME_GET frame.
 * @param pFrame Pointer to frame.
 * @param pFrameLength Pointer to length of frame.
 * @param groupID ID of the association group.
 */
static void association_group_name_get_frame_create(
    uint8_t * pFrame,
    uint8_t * pFrameLength,
    uint8_t groupID)
{
  uint8_t frameCount = 0;
  memset(pFrame, 0x00, sizeof(ZW_APPLICATION_TX_BUFFER));
  pFrame[frameCount++] = 0x59; // COMMAND_CLASS_ASSOCIATION_GRP_INFO;
  pFrame[frameCount++] = 0x01; // ASSOCIATION_GROUP_NAME_GET
  pFrame[frameCount++] = groupID;
  *pFrameLength = frameCount;
}

/**
 * Creates a ASSOCIATION_GROUP_COMMAND_LIST_GET frame.
 * @param pFrame Pointer to frame.
 * @param pFrameLength Pointer to length of frame.
 * @param allowCache Allow cache field.
 * @param groupID ID of the association group.
 */
static void association_group_command_list_get_frame_create(
    command_handler_input_t * pCommandHandlerInput,
    bool allowCache,
    uint8_t groupID)
{
  uint8_t frameCount = 0;
  pCommandHandlerInput->frame.as_byte_array[frameCount++] = COMMAND_CLASS_ASSOCIATION_GRP_INFO;
  pCommandHandlerInput->frame.as_byte_array[frameCount++] = ASSOCIATION_GROUP_COMMAND_LIST_GET;
  pCommandHandlerInput->frame.as_byte_array[frameCount++] = ((true == allowCache) ? 1 : 0) & 0x80;
  pCommandHandlerInput->frame.as_byte_array[frameCount++] = groupID;
  pCommandHandlerInput->frameLength = frameCount;
}

/**
 * Creates a ASSOCIATION_GROUP_INFO_GET frame.
 * @param allowCache Allow cache field.
 * @param groupID ID of the association group.
 */
static command_handler_input_t * association_group_info_get_frame_create(
    bool refreshCache,
    bool listMode,
    uint8_t groupID)
{
  command_handler_input_t * p_chi = test_common_command_handler_input_allocate();
  p_chi->frame.as_byte_array[p_chi->frameLength++] = COMMAND_CLASS_ASSOCIATION_GRP_INFO;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = ASSOCIATION_GROUP_INFO_GET;
  p_chi->frame.as_byte_array[p_chi->frameLength]   = 0;
  p_chi->frame.as_byte_array[p_chi->frameLength]   = (true == refreshCache) ? 0x80 : 0;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = (true == listMode) ? 0x40 : 0;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = groupID;
  return p_chi;
}


void test_GetApplGroupCommandList_ResourceGroup(void)
{
  bool result;

  const uint8_t END_OF_BUFFER_MARKER = 0xFF;
  uint8_t GroupList[] = {0x00, 0x00, 0x00, 0x00, END_OF_BUFFER_MARKER}; // Room for 2 groups plus overflow detect byte

  const uint8_t ENDPOINT_ROOT = 0; // Root device
  const uint8_t GROUP_ID_RESOURCEGROUP = 2;

  /*
   * Set up AGI
   */
  const ccc_pair_t CCC_PAIRS[] = {
                                    {
                                     .cmdClass = COMMAND_CLASS_SWITCH_MULTILEVEL_V4,
                                     .cmd = SWITCH_MULTILEVEL_START_LEVEL_CHANGE_V4
                                    },
                                    {
                                     .cmdClass = COMMAND_CLASS_SWITCH_MULTILEVEL_V4,
                                     .cmd = SWITCH_MULTILEVEL_STOP_LEVEL_CHANGE_V4
                                    },
  };
  const cc_agi_group_t agiTableRootDeviceGroups[] = {
    {{0xAA, 0xBB},  // Don't care
     sizeof_array(CCC_PAIRS),
     CCC_PAIRS,
      "Group 1"}
  };

  const cc_agi_config_t customAgiTable[] =
  {
    {
      .groups = agiTableRootDeviceGroups,
      .group_count = sizeof_array(agiTableRootDeviceGroups)
    }
  };

  cc_agi_get_config_ExpectAndReturn(customAgiTable);
  ZAF_CC_init_specific(COMMAND_CLASS_ASSOCIATION_GRP_INFO);

  const uint8_t EXPECTED_GROUPLIST_BUFFER[] = {
                                               COMMAND_CLASS_SWITCH_MULTILEVEL_V4,
                                               SWITCH_MULTILEVEL_START_LEVEL_CHANGE_V4,
                                               COMMAND_CLASS_SWITCH_MULTILEVEL_V4,
                                               SWITCH_MULTILEVEL_STOP_LEVEL_CHANGE_V4,
                                               END_OF_BUFFER_MARKER
  };


  /*
   * Get the group list for resource group 1
   */
  size_t groupListSize;
  result = GetApplGroupCommandList(GroupList,
                                   &groupListSize,
                                   GROUP_ID_RESOURCEGROUP,
                                   ENDPOINT_ROOT);

  TEST_ASSERT_TRUE_MESSAGE(result, "Unexpected return value :(");
  TEST_ASSERT_EQUAL_UINT8_ARRAY_MESSAGE(EXPECTED_GROUPLIST_BUFFER, GroupList, 5, "Returned GroupList buffer content does not match :(");
}

void test_GetApplGroupCommandList_InvalidEndpoint(void)
{
  bool result;

  uint8_t GroupList[] = {0xde, 0xad, 0xbe, 0xef};

  const uint8_t ENDPOINT = ZAF_CONFIG_NUMBER_OF_END_POINTS + 1; // Invalid endpoint
  const uint8_t GROUP_ID = 1;

  /*
   * Test with an invalid endpoint
   */
  size_t groupListSize;
  result = GetApplGroupCommandList(GroupList,
                                   &groupListSize,
                                   GROUP_ID,
                                   ENDPOINT);

  TEST_ASSERT_FALSE_MESSAGE(result, "Unexpected return value :(");
}

void test_GetApplGroupCommandListSize_ResourceGroup(void)
{
  const uint8_t ENDPOINT_ROOT = 0; // Root device
  const uint8_t GROUP_ID_RESOURCEGROUP = 2;

  /*
   * Set up AGI
   */
  const ccc_pair_t CCC_PAIRS[] = {
                                  {
                                   .cmdClass = COMMAND_CLASS_SWITCH_MULTILEVEL_V4,
                                   .cmd = SWITCH_MULTILEVEL_START_LEVEL_CHANGE_V4
                                  },
                                  {
                                   .cmdClass = COMMAND_CLASS_SWITCH_MULTILEVEL_V4,
                                   .cmd = SWITCH_MULTILEVEL_STOP_LEVEL_CHANGE_V4
                                  },
  };
  const cc_agi_group_t agiTableRootDeviceGroups[] = {
    {{0xAA, 0xBB},  // Don't care
     sizeof_array(CCC_PAIRS),
     CCC_PAIRS,
      "Group 1"}
  };

  const cc_agi_config_t customAgiTable[] =
  {
    {
      .groups = agiTableRootDeviceGroups,
      .group_count = sizeof_array(agiTableRootDeviceGroups)
    }
  };
  
  cc_agi_get_config_ExpectAndReturn(customAgiTable);
  ZAF_CC_init_specific(COMMAND_CLASS_ASSOCIATION_GRP_INFO);

  const uint8_t EXPECTED_GROUPLIST_SIZE = sizeof(CMD_CLASS_GRP) * 2;

  /*
   * Get the group list size for lifeline
   */
  uint8_t GroupList[100];
  size_t returnedSize;
  GetApplGroupCommandList(GroupList,
                          &returnedSize,
                          GROUP_ID_RESOURCEGROUP,
                          ENDPOINT_ROOT);

  TEST_ASSERT_EQUAL_UINT32_MESSAGE(EXPECTED_GROUPLIST_SIZE, returnedSize, "Returned size does not match :(");
}

MULTICHAN_NODE_ID root_association[5] =
{
    {.node =  {1, 0}, .nodeInfo = {0, 0}},
};

AGI_PROFILE profile_lifeline = {
                                ASSOCIATION_GROUP_INFO_REPORT_AGI_PROFILE_GENERAL,
                                ASSOCIATION_GROUP_INFO_REPORT_AGI_GENERAL_LIFELINE
};
