/**
 * @file test_CC_ColorSwitch.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include <mock_control.h>
#include "CC_ColorSwitch.h"

#include <string.h>
#include <test_common.h>
#include <ZW_TransportEndpoint.h>
#include <SizeOf.h>
#include <ZAF_TSE.h>
#include "ZAF_CC_Invoker.h"
#include <cc_color_switch_config_api_mock.h>
#include <cc_color_switch_io_mock.h>
#include "zaf_transport_tx_mock.h"

static command_handler_input_t* inputbuff_SupportedGet();
static command_handler_input_t* inputbuff_Get(EColorComponents colorID);
static command_handler_input_t* inputbuff_SetV3_1Color(uint8_t colorCount, EColorComponents colorID,
                                                       uint8_t value, uint8_t duration);
static command_handler_input_t* inputbuff_SetV3_3Colors(uint8_t colorCount,
                                                        EColorComponents colorID1, uint8_t value1,
                                                        EColorComponents colorID2, uint8_t value2,
                                                        EColorComponents colorID3, uint8_t value3,
                                                        uint8_t duration);
static command_handler_input_t* inputbuff_SetV2_3Colors(uint8_t colorCount,
                                                        EColorComponents colorID1, uint8_t value1,
                                                        EColorComponents colorID2, uint8_t value2,
                                                        EColorComponents colorID3, uint8_t value3);
static command_handler_input_t* inputbuff_StartLevelChangeV3(uint8_t properties,
                                                             EColorComponents colorID,
                                                             uint8_t startLevelChange,
                                                             uint8_t duration);
static command_handler_input_t* inputbuff_StartLevelChangeV2(uint8_t properties,
                                                             EColorComponents colorID,
                                                             uint8_t startLevelChange);
static command_handler_input_t* inputbuff_StopLevelChange(EColorComponents colorID);


static void app_color_cb(s_colorComponent *color)
{
  char color_str[20];
  switch(color->colorId)
  {
    case ECOLORCOMPONENT_AMBER:
      sprintf(color_str, "COLOR_AMBER");
      break;
    case ECOLORCOMPONENT_BLUE:
      sprintf(color_str, "COLOR_BLUE");
      break;
    case ECOLORCOMPONENT_COLD_WHITE:
      sprintf(color_str, "COLOR_COLD_WHITE");
      break;
    case ECOLORCOMPONENT_CYAN:
      sprintf(color_str, "COLOR_CYAN");
      break;
    case ECOLORCOMPONENT_GREEN:
      sprintf(color_str, "COLOR_GREEN");
      break;
    case ECOLORCOMPONENT_PURPLE:
      sprintf(color_str, "COLOR_PURPLE");
      break;
    case ECOLORCOMPONENT_RED:
      sprintf(color_str, "COLOR_RED");
      break;
    default:
      sprintf(color_str, "COLOR_UNKNOWN");
      break;
  }
  printf ("%s - %s\n", __func__, color_str);
}

static int cb_counter = 0;
void cc_color_switch_refresh_cb(void)
{
  ++cb_counter;
  //printf("%s, counter = %d\n", __func__, cb_counter);
}

void setUpSuite(void)
{
  cb_counter = 0;
}

void tearDownSuite(void) {

}

void setUp(void)
{

}

void tearDown(void)
{

}

void test_SWITCH_COLOR_handler(void)
{
  RECEIVE_OPTIONS_TYPE_EX rxOpt;
  ZW_APPLICATION_TX_BUFFER buff;
  received_frame_status_t status;

  mock_calls_clear();
  buff.ZW_Common.cmdClass = COMMAND_CLASS_SWITCH_COLOR;
  buff.ZW_Common.cmd      = 0xAA; // unsupported
  mock_call_use_as_stub(TO_STR(Check_not_legal_response_job));

  cc_handler_input_t input = {
      .rx_options = &rxOpt,
      .frame = &buff,
  };

  ZW_APPLICATION_TX_BUFFER frameOut = {0};
  cc_handler_output_t output = {
      .frame = &frameOut,
      .length = 0,
      .duration = 0
  };

  status = invoke_cc_handler(&input, &output);
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(RECEIVED_FRAME_STATUS_NO_SUPPORT, status, "Wrong status");
}

/** Helper function used to set values of Actuator structure as a beginning state before some change happens */
static void helper_setObjectValues(s_Actuator *obj, uint8_t valueCurrent, uint8_t valueTarget, uint8_t duration)
{
  obj->valueCurrent = valueCurrent;
  obj->valueTarget = valueTarget;
}

static uint8_t helper_GetCurrentValue(s_Actuator *obj)
{
  return obj->valueCurrent;
}

static void (*colorswitch_cb)(s_Actuator *);
void test_CC_ColorSwitchInit_pass(void)
{
  mock_calls_clear();
  mock_t *pMock = NULL;
  mock_t *pMock_ZAF_Actuator_Set = NULL;
  s_colorComponent colors[] = {
                                { .colorId = ECOLORCOMPONENT_RED,
                                  .cb = &app_color_cb },
                                { .colorId = ECOLORCOMPONENT_GREEN,
                                  .cb = &app_color_cb },
                                { .colorId = ECOLORCOMPONENT_BLUE,
                                  .cb = &app_color_cb }};

  cc_color_switch_get_default_duration_IgnoreAndReturn(2);
  cc_color_switch_get_length_colorComponents_IgnoreAndReturn(sizeof_array(colors));
  cc_color_switch_get_colorComponents_IgnoreAndReturn(colors);
  // test: min, max, timer, use actual callback function and not NULL
  for (int i= 0; i < sizeof_array(colors); i++)
  {
    cc_color_switch_read_ExpectAndReturn(i, NULL, false);
    cc_color_switch_read_IgnoreArg_color_component();
    cc_color_switch_write_IgnoreAndReturn(true);
    mock_call_expect(TO_STR(ZAF_Actuator_Init), &pMock);
    pMock->compare_rule_arg[0] = COMPARE_NOT_NULL;
    pMock->expect_arg[0].p = &colors[i].obj;
    pMock->expect_arg[1].v = 0; //min
    pMock->expect_arg[2].v = 255; //max
    pMock->expect_arg[3].v = 20; //stepSize
    pMock->expect_arg[4].v = 2; //duration
    pMock->compare_rule_arg[5] = COMPARE_NOT_NULL;
    mock_call_expect(TO_STR(ZAF_Actuator_Set), &pMock_ZAF_Actuator_Set);
    pMock_ZAF_Actuator_Set->compare_rule_arg[0] = COMPARE_NOT_NULL;
    pMock_ZAF_Actuator_Set->expect_arg[0].p = &colors[i].obj;
    pMock_ZAF_Actuator_Set->expect_arg[1].v = 255; //value
    pMock_ZAF_Actuator_Set->expect_arg[2].v = 0; //duration
  }
  ZAF_CC_init_specific(COMMAND_CLASS_SWITCH_COLOR);
  colorswitch_cb = pMock->actual_arg[5].p;
  mock_calls_verify();
}

void test_SWITCH_COLOR_handler_SUPPORTED_GET(void)
{
  mock_calls_clear();

  received_frame_status_t status;
  s_colorComponent colors[] = {
                               { .colorId = ECOLORCOMPONENT_RED, },
                               { .colorId = ECOLORCOMPONENT_GREEN, },
                               { .colorId = ECOLORCOMPONENT_BLUE, }};

  cc_color_switch_get_default_duration_IgnoreAndReturn(2);
  cc_color_switch_get_length_colorComponents_IgnoreAndReturn(sizeof_array(colors));
  cc_color_switch_get_colorComponents_IgnoreAndReturn(colors);
  mock_call_use_as_stub(TO_STR(ZAF_Actuator_Init));
  mock_call_use_as_stub(TO_STR(ZAF_Actuator_Set));
  for (int i= 0; i < sizeof_array(colors); i++)
  {
    cc_color_switch_read_ExpectAndReturn(i, NULL, false);
    cc_color_switch_read_IgnoreArg_color_component();
    cc_color_switch_write_IgnoreAndReturn(true);
  }

  ZAF_CC_init_specific(COMMAND_CLASS_SWITCH_COLOR);

  // Input data
  command_handler_input_t *p_chi = inputbuff_SupportedGet();

  uint16_t colorComponentMask = 0 | (1 << ECOLORCOMPONENT_RED) | (1 << ECOLORCOMPONENT_GREEN) | (1 << ECOLORCOMPONENT_BLUE);
  uint8_t output_buff[] = {COMMAND_CLASS_SWITCH_COLOR_V3, SWITCH_COLOR_SUPPORTED_REPORT,
                           colorComponentMask & 0xFF, colorComponentMask >> 8};

  mock_call_use_as_stub(TO_STR(Check_not_legal_response_job));


  cc_handler_input_t input = {
      .rx_options = &p_chi->rxOptions,
      .frame = &p_chi->frame.as_zw_application_tx_buffer,
      .length = p_chi->frameLength
  };

  ZW_APPLICATION_TX_BUFFER frameOut = {0};
  cc_handler_output_t output = {
      .frame = &frameOut,
      .length = 0,
      .duration = 0
  };

  status = invoke_cc_handler(&input, &output);

  TEST_ASSERT_EQUAL_UINT16_MESSAGE(RECEIVED_FRAME_STATUS_SUCCESS, status, "Wrong status");

  TEST_ASSERT_EQUAL_INT8_MESSAGE(output.length,
                                  sizeof(output_buff),
                                  "Frame size does not match");
  TEST_ASSERT_EQUAL_INT8_ARRAY_MESSAGE(output_buff, &frameOut,
                                        sizeof(output_buff),
                                        "Frame does not match");

  test_common_command_handler_input_free(p_chi);

  mock_calls_verify();
}


void test_SWITCH_COLOR_handler_GET_pass(void)
{
  mock_calls_clear();

  mock_t *pMock = NULL;
  received_frame_status_t status;
  s_colorComponent colors[] = {
                               { .colorId = ECOLORCOMPONENT_RED },
                               { .colorId = ECOLORCOMPONENT_GREEN },
                               { .colorId = ECOLORCOMPONENT_BLUE }};

  cc_color_switch_get_default_duration_IgnoreAndReturn(2);
  cc_color_switch_get_length_colorComponents_IgnoreAndReturn(sizeof_array(colors));
  cc_color_switch_get_colorComponents_IgnoreAndReturn(colors);
  mock_call_use_as_stub(TO_STR(ZAF_Actuator_Init));
  mock_call_use_as_stub(TO_STR(ZAF_Actuator_Set));
  for (int i= 0; i < sizeof_array(colors); i++)
  {
    cc_color_switch_read_ExpectAndReturn(i, NULL, false);
    cc_color_switch_read_IgnoreArg_color_component();
    cc_color_switch_write_IgnoreAndReturn(true);
  }
  ZAF_CC_init_specific(COMMAND_CLASS_SWITCH_COLOR);

  // GET command for RED color
  command_handler_input_t *p_chi_1 = inputbuff_Get(ECOLORCOMPONENT_RED);
  uint8_t output_buff[] = {COMMAND_CLASS_SWITCH_COLOR_V3, SWITCH_COLOR_REPORT_V3, ECOLORCOMPONENT_RED, 0x0, 0x0, 0};

  mock_call_use_as_stub(TO_STR(Check_not_legal_response_job));

  mock_call_expect(TO_STR(ZAF_Actuator_GetCurrentValue), &pMock);
  pMock->expect_arg[0].p = &colors[0].obj;
  pMock->return_code.v = colors[0].obj.valueCurrent;

  mock_call_expect(TO_STR(ZAF_Actuator_GetTargetValue), &pMock);
  pMock->expect_arg[0].p = &colors[0].obj;
  pMock->return_code.v = colors[0].obj.valueTarget;

  mock_call_expect(TO_STR(ZAF_Actuator_GetDurationRemaining), &pMock);
  pMock->expect_arg[0].p = &colors[0].obj;
  pMock->return_code.v = 0;

  cc_handler_input_t input = {
      .rx_options = &p_chi_1->rxOptions,
      .frame = &p_chi_1->frame.as_zw_application_tx_buffer,
      .length = p_chi_1->frameLength
  };

  ZW_APPLICATION_TX_BUFFER frameOut = {0};
  cc_handler_output_t output = {
      .frame = &frameOut,
      .length = 0,
      .duration = 0
  };

  status = invoke_cc_handler(&input, &output);

  TEST_ASSERT_EQUAL_UINT16_MESSAGE(RECEIVED_FRAME_STATUS_SUCCESS, status, "Wrong status");
  TEST_ASSERT_EQUAL_INT8_MESSAGE(output.length,
                                  sizeof(output_buff),
                                  "Frame size does not match");
  TEST_ASSERT_EQUAL_INT8_ARRAY_MESSAGE(output_buff, &frameOut,
                                        sizeof(output_buff),
                                        "Frame does not match");
  
  mock_calls_verify();

  // CASE2. GET command when unsupported color was requested
  // Expectation: REPORT should contain first color listed in colors[].
  mock_calls_clear();
  command_handler_input_t *p_chi_2 = inputbuff_Get(ECOLORCOMPONENT_PURPLE);
  // output_buff should be the same
  mock_call_use_as_stub(TO_STR(Check_not_legal_response_job));

  mock_call_use_as_stub(TO_STR(ZAF_Actuator_GetCurrentValue));
  mock_call_use_as_stub(TO_STR(ZAF_Actuator_GetTargetValue));
  mock_call_use_as_stub(TO_STR(ZAF_Actuator_GetDurationRemaining));

  cc_handler_input_t input2 = {
       .rx_options = &p_chi_2->rxOptions,
       .frame = &p_chi_2->frame.as_zw_application_tx_buffer,
       .length = p_chi_2->frameLength
   };

  status = invoke_cc_handler(&input2, &output);

  TEST_ASSERT_EQUAL_UINT16_MESSAGE(RECEIVED_FRAME_STATUS_SUCCESS, status, "Wrong status");
  TEST_ASSERT_EQUAL_INT8_MESSAGE(output.length,
                                 sizeof(output_buff),
                                 "Frame size does not match");
  TEST_ASSERT_EQUAL_INT8_ARRAY_MESSAGE(output_buff, &frameOut,
                                       sizeof(output_buff),
                                        "Frame does not match");

  test_common_command_handler_input_free(p_chi_1);
  test_common_command_handler_input_free(p_chi_2);

  mock_calls_verify();
}

void test_SWITCH_COLOR_handler_SET_fail(void)
{
  mock_calls_clear();

  received_frame_status_t status;
  s_colorComponent colors[] = {
                               { .colorId = ECOLORCOMPONENT_RED, },
                               { .colorId = ECOLORCOMPONENT_GREEN, },
                               { .colorId = ECOLORCOMPONENT_BLUE, }};

  cc_color_switch_get_default_duration_IgnoreAndReturn(2);
  cc_color_switch_get_length_colorComponents_IgnoreAndReturn(sizeof_array(colors));
  cc_color_switch_get_colorComponents_IgnoreAndReturn(colors);
  mock_call_use_as_stub(TO_STR(ZAF_Actuator_Init));
  mock_call_use_as_stub(TO_STR(ZAF_Actuator_Set));
  for (int i= 0; i < sizeof_array(colors); i++)
  {
    cc_color_switch_read_ExpectAndReturn(i, NULL, false);
    cc_color_switch_read_IgnoreArg_color_component();
    cc_color_switch_write_IgnoreAndReturn(true);
  }
  ZAF_CC_init_specific(COMMAND_CLASS_SWITCH_COLOR);

  // input data - invalid color count
  mock_calls_clear();

  command_handler_input_t* p_chi_1 = inputbuff_SetV3_3Colors(0,
                                                          ECOLORCOMPONENT_RED, 0xFF,
                                                          ECOLORCOMPONENT_GREEN, 0xFF,
                                                          ECOLORCOMPONENT_BLUE, 0xFF,
                                                          0xA);

  cc_handler_input_t input = {
       .rx_options = &p_chi_1->rxOptions,
       .frame = &p_chi_1->frame.as_zw_application_tx_buffer,
       .length = p_chi_1->frameLength
   };

  ZW_APPLICATION_TX_BUFFER frameOut = {0};
  cc_handler_output_t output = {
      .frame = &frameOut,
      .length = 0,
      .duration = 0
  };

  status = invoke_cc_handler(&input, &output);

  TEST_ASSERT_EQUAL_UINT16_MESSAGE(RECEIVED_FRAME_STATUS_FAIL, status, "Wrong status");
  mock_calls_verify();

  // input data - invalid colorId
  mock_calls_clear();
  command_handler_input_t* p_chi_2 = inputbuff_SetV3_1Color(1, ECOLORCOMPONENT_AMBER, 0xFF, 0xA);

  cc_handler_input_t input2 = {
      .rx_options = &p_chi_2->rxOptions,
      .frame = &p_chi_2->frame.as_zw_application_tx_buffer,
      .length = p_chi_2->frameLength
  };

  status = invoke_cc_handler(&input2, &output);

  TEST_ASSERT_EQUAL_UINT16_MESSAGE(RECEIVED_FRAME_STATUS_FAIL, status, "Wrong status");

  test_common_command_handler_input_free(p_chi_1);
  test_common_command_handler_input_free(p_chi_2);

  mock_calls_verify();
}

void test_SWITCH_COLOR_handler_SET_pass(void)
{
  mock_calls_clear();

  mock_t *pMock = NULL;
  received_frame_status_t status;
  s_colorComponent colors[] = {
                               { .colorId = ECOLORCOMPONENT_RED, },
                               { .colorId = ECOLORCOMPONENT_GREEN, },
                               { .colorId = ECOLORCOMPONENT_BLUE, }};

  cc_color_switch_get_default_duration_IgnoreAndReturn(2);
  cc_color_switch_get_length_colorComponents_IgnoreAndReturn(sizeof_array(colors));
  cc_color_switch_get_colorComponents_IgnoreAndReturn(colors);
  mock_call_use_as_stub(TO_STR(ZAF_Actuator_Init));
  for (int i = 0; i < sizeof_array(colors); i++)
  {
    cc_color_switch_read_ExpectAndReturn(i, NULL, false);
    cc_color_switch_read_IgnoreArg_color_component();
    cc_color_switch_write_IgnoreAndReturn(true);
    mock_call_expect(TO_STR(ZAF_Actuator_Set), &pMock);
    pMock->compare_rule_arg[0] = COMPARE_NOT_NULL;
    pMock->expect_arg[0].p = &colors[i].obj;
    pMock->expect_arg[1].v = 255;
    pMock->expect_arg[2].v = 0;
  }
  ZAF_CC_init_specific(COMMAND_CLASS_SWITCH_COLOR);

  uint8_t colorsCount = 3;

  // input data < V3
  command_handler_input_t* p_chi_1 = inputbuff_SetV2_3Colors(colorsCount,
                                                           ECOLORCOMPONENT_RED, 0xFF,
                                                           ECOLORCOMPONENT_GREEN, 0xDD,
                                                           ECOLORCOMPONENT_BLUE, 0xBB);

  for (int i = 0; i< colorsCount; i++)
  {
    mock_call_expect(TO_STR(ZAF_Actuator_Set), &pMock);
    pMock->compare_rule_arg[0] = COMPARE_NOT_NULL;
    pMock->expect_arg[0].p = &colors[i].obj;
    pMock->expect_arg[1].v = p_chi_1->frame.as_byte_array[3 + 2*i +1];
    pMock->expect_arg[2].v = 0;
    pMock->return_code.v = 0; //Not changing
  }

  cc_handler_input_t input = {
       .rx_options = &p_chi_1->rxOptions,
       .frame = &p_chi_1->frame.as_zw_application_tx_buffer,
       .length = p_chi_1->frameLength
   };

  ZW_APPLICATION_TX_BUFFER frameOut = {0};
  cc_handler_output_t output = {
      .frame = &frameOut,
      .length = 0,
      .duration = 0
  };

  status = invoke_cc_handler(&input, &output);

  TEST_ASSERT_EQUAL_UINT16_MESSAGE(RECEIVED_FRAME_STATUS_SUCCESS, status, "Wrong status");
  mock_calls_verify();

  // input data - V3
  mock_calls_clear();
  command_handler_input_t* p_chi_2 = inputbuff_SetV3_3Colors(colorsCount,
                                  ECOLORCOMPONENT_RED, 0xFF,
                                  ECOLORCOMPONENT_GREEN, 0xDD,
                                  ECOLORCOMPONENT_BLUE, 0xEE,
                                  0xA);

  for (int i = 0; i< colorsCount; i++)
  {
    mock_call_expect(TO_STR(ZAF_Actuator_Set), &pMock);
    pMock->compare_rule_arg[0] = COMPARE_NOT_NULL;
    pMock->expect_arg[0].p = &colors[i].obj;
    pMock->expect_arg[1].v = p_chi_2->frame.as_byte_array[3 + 2*i +1];
    pMock->expect_arg[2].v = p_chi_2->frame.as_byte_array[p_chi_2->frameLength-1];
    pMock->return_code.v = 1; //Changing
  }

  cc_handler_input_t input2 = {
      .rx_options = &p_chi_2->rxOptions,
      .frame = &p_chi_2->frame.as_zw_application_tx_buffer,
      .length = p_chi_2->frameLength
  };

  status = invoke_cc_handler(&input2, &output);

  TEST_ASSERT_EQUAL_UINT16_MESSAGE(RECEIVED_FRAME_STATUS_SUCCESS, status, "Wrong status");
  mock_calls_verify();

  // input data - V3. Duration > 0, supervision enabled.
  mock_calls_clear();
  command_handler_input_t* p_chi_3 = inputbuff_SetV3_3Colors(colorsCount,
                                  ECOLORCOMPONENT_RED, 0xFF,
                                  ECOLORCOMPONENT_GREEN, 0xDD,
                                  ECOLORCOMPONENT_BLUE, 0xEE,
                                  0xA);

  p_chi_3->rxOptions.bSupervisionActive = 1;
  p_chi_3->rxOptions.sessionId = 33;
  p_chi_3->rxOptions.statusUpdate = 1;

  for (int i = 0; i< colorsCount; i++)
  {
    mock_call_expect(TO_STR(ZAF_Actuator_Set), &pMock);
    pMock->compare_rule_arg[0] = COMPARE_NOT_NULL;
    pMock->expect_arg[0].p = &colors[i].obj;
    pMock->expect_arg[1].v = p_chi_3->frame.as_byte_array[3 + 2*i +1];
    pMock->expect_arg[2].v = p_chi_3->frame.as_byte_array[p_chi_3->frameLength-1];
    pMock->return_code.v = 1; //Changing
  }

  mock_call_expect(TO_STR(ZAF_Actuator_GetDurationRemaining),&pMock);
  pMock->compare_rule_arg[0] = COMPARE_NOT_NULL;
  pMock->return_code.v = p_chi_3->frame.as_byte_array[p_chi_3->frameLength-1]; // requested duration

  mock_call_expect(TO_STR(is_multicast), &pMock);
  pMock->compare_rule_arg[0] = COMPARE_NOT_NULL;
  pMock->return_code.v = false;  // is not a multicast

  cc_handler_input_t input3 = {
      .rx_options = &p_chi_3->rxOptions,
      .frame = &p_chi_3->frame.as_zw_application_tx_buffer,
      .length = p_chi_3->frameLength
  };

  status = invoke_cc_handler(&input3, &output);

  TEST_ASSERT_EQUAL_UINT16_MESSAGE(RECEIVED_FRAME_STATUS_WORKING, status, "Wrong status");

  test_common_command_handler_input_free(p_chi_1);
  test_common_command_handler_input_free(p_chi_2);
  test_common_command_handler_input_free(p_chi_3);

  mock_calls_verify();
}


void test_SWITCH_COLOR_handler_Endpoints(void)
{
  mock_calls_clear();

  mock_t *pMock = NULL;
  received_frame_status_t status;
  s_colorComponent colors[] = {
                                { .colorId = ECOLORCOMPONENT_RED,
                                  .cb = &app_color_cb,
                                  .ep = 1},
                                { .colorId = ECOLORCOMPONENT_BLUE,
                                  .cb = &app_color_cb,
                                  .ep = 1},
                                { .colorId = ECOLORCOMPONENT_RED,
                                  .cb = &app_color_cb,
                                  .ep = 2},
                                { .colorId = ECOLORCOMPONENT_GREEN,
                                  .cb = &app_color_cb,
                                  .ep = 2},
                                { .colorId = ECOLORCOMPONENT_AMBER,
                                  .cb = &app_color_cb,
                                  .ep = 3},
  };

  cc_color_switch_get_default_duration_IgnoreAndReturn(2);
  cc_color_switch_get_length_colorComponents_IgnoreAndReturn(sizeof_array(colors));
  cc_color_switch_get_colorComponents_IgnoreAndReturn(colors);
  mock_call_use_as_fake(TO_STR(ZAF_Actuator_Init));
  mock_call_use_as_stub(TO_STR(ZAF_Actuator_Set));
  for (int i= 0; i < sizeof_array(colors); i++)
  {
    cc_color_switch_read_ExpectAndReturn(i, NULL, false);
    cc_color_switch_read_IgnoreArg_color_component();
    cc_color_switch_write_IgnoreAndReturn(true);
  }
  ZAF_CC_init_specific(COMMAND_CLASS_SWITCH_COLOR);


  // input data - Get supported colors for ep 1
  mock_calls_clear();
  command_handler_input_t* p_chi_1 = inputbuff_SupportedGet();
  p_chi_1->rxOptions.destNode.endpoint = 1;

  uint16_t colorComponentMask = 0 | (1 << ECOLORCOMPONENT_RED) | (1 << ECOLORCOMPONENT_BLUE);
  uint8_t output_buff_supported[] = {COMMAND_CLASS_SWITCH_COLOR_V3, SWITCH_COLOR_SUPPORTED_REPORT,
                           colorComponentMask & 0xFF, colorComponentMask >> 8};

  mock_call_use_as_stub(TO_STR(Check_not_legal_response_job));

  cc_handler_input_t input = {
      .rx_options = &p_chi_1->rxOptions,
      .frame = &p_chi_1->frame.as_zw_application_tx_buffer,
      .length = p_chi_1->frameLength
  };

  ZW_APPLICATION_TX_BUFFER frameOut = {0};
  cc_handler_output_t output = {
      .frame = &frameOut,
      .length = 0,
      .duration = 0
  };

  status = invoke_cc_handler(&input, &output);

  TEST_ASSERT_EQUAL_UINT16_MESSAGE(RECEIVED_FRAME_STATUS_SUCCESS, status, "Wrong status");
  TEST_ASSERT_EQUAL_INT8_MESSAGE(output.length,
                                  sizeof(output_buff_supported),
                                  "Frame size does not match");
  TEST_ASSERT_EQUAL_INT8_ARRAY_MESSAGE(output_buff_supported, &frameOut,
                                        sizeof(output_buff_supported),
                                        "Frame does not match");
  
  mock_calls_verify();

  // input data - SET color for endpoint 2
  mock_calls_clear();
  uint8_t colorsCount = 1;
  command_handler_input_t* p_chi_2 = inputbuff_SetV3_1Color(colorsCount, ECOLORCOMPONENT_RED, 0xFF, 0xA);
  p_chi_2->rxOptions.destNode.endpoint = 2;

  mock_call_expect(TO_STR(ZAF_Actuator_Set), &pMock);
  pMock->compare_rule_arg[0] = COMPARE_NOT_NULL;
  pMock->expect_arg[0].p = &colors[2].obj;
  pMock->expect_arg[1].v = p_chi_2->frame.as_byte_array[3 + 1];
  pMock->expect_arg[2].v = p_chi_2->frame.as_byte_array[p_chi_2->frameLength-1];
  pMock->return_code.v = 1; //Changing

  cc_handler_input_t input2 = {
      .rx_options = &p_chi_2->rxOptions,
      .frame = &p_chi_2->frame.as_zw_application_tx_buffer,
      .length = p_chi_2->frameLength
  };

  status = invoke_cc_handler(&input2, &output);

  TEST_ASSERT_EQUAL_UINT16_MESSAGE(RECEIVED_FRAME_STATUS_SUCCESS, status, "Wrong status");
  mock_calls_verify();

  // input data - SET unsupported color for endpoint 2 V3
  mock_calls_clear();
  command_handler_input_t* p_chi_3 = inputbuff_SetV3_1Color(colorsCount, ECOLORCOMPONENT_BLUE, 0xBB, 0xA);
  p_chi_3->rxOptions.destNode.endpoint = 2;

  cc_handler_input_t input3 = {
      .rx_options = &p_chi_3->rxOptions,
      .frame = &p_chi_3->frame.as_zw_application_tx_buffer,
      .length = p_chi_3->frameLength
  };


  status = invoke_cc_handler(&input3, &output);
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(RECEIVED_FRAME_STATUS_FAIL, status, "Wrong status");

  mock_calls_verify();

  // input data - SET RED color for endpoints 1 and 2 and make sure that correct value of RED is updated
  mock_calls_clear();
  uint8_t value_ep1 = 0x11;
  command_handler_input_t* p_chi_4 = inputbuff_SetV3_1Color(colorsCount, ECOLORCOMPONENT_RED, value_ep1, 0x0);
  p_chi_4->rxOptions.destNode.endpoint = 1;
  mock_call_use_as_fake(TO_STR(ZAF_Actuator_Set));
  mock_call_use_as_stub(TO_STR(ZAF_Actuator_GetDurationRemaining));
  mock_call_use_as_stub(TO_STR(ZAF_Actuator_GetTargetValue));
  mock_call_use_as_stub(TO_STR(ZAF_Actuator_GetCurrentValue));
  cc_color_switch_write_IgnoreAndReturn(true);
  mock_call_use_as_stub(TO_STR(ZAF_TSE_Trigger));

  cc_handler_input_t input4 = {
      .rx_options = &p_chi_4->rxOptions,
      .frame = &p_chi_4->frame.as_zw_application_tx_buffer,
      .length = p_chi_4->frameLength
  };
  status = invoke_cc_handler(&input4, &output);

  TEST_ASSERT_EQUAL_UINT16_MESSAGE(RECEIVED_FRAME_STATUS_SUCCESS, status, "Wrong status");

  uint8_t value_ep2 = 0x22;
  command_handler_input_t* p_chi_5 = inputbuff_SetV3_1Color(colorsCount, ECOLORCOMPONENT_RED, value_ep2, 0x0);
  p_chi_5->rxOptions.destNode.endpoint = 2;
  mock_call_use_as_fake(TO_STR(ZAF_Actuator_Set));
  mock_call_use_as_stub(TO_STR(ZAF_Actuator_GetDurationRemaining));
  mock_call_use_as_stub(TO_STR(ZAF_Actuator_GetTargetValue));
  mock_call_use_as_stub(TO_STR(ZAF_Actuator_GetCurrentValue));
  cc_color_switch_write_IgnoreAndReturn(true);
  mock_call_use_as_stub(TO_STR(ZAF_TSE_Trigger));

  cc_handler_input_t input5= {
      .rx_options = &p_chi_5->rxOptions,
      .frame = &p_chi_5->frame.as_zw_application_tx_buffer,
      .length = p_chi_5->frameLength
  };
  status = invoke_cc_handler(&input5, &output);

  TEST_ASSERT_EQUAL_UINT16_MESSAGE(RECEIVED_FRAME_STATUS_SUCCESS, status, "Wrong status");
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(value_ep1, helper_GetCurrentValue(&colors[0].obj), "Wrong value");
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(value_ep2, helper_GetCurrentValue(&colors[2].obj), "Wrong value");

  test_common_command_handler_input_free(p_chi_1);
  test_common_command_handler_input_free(p_chi_2);
  test_common_command_handler_input_free(p_chi_3);
  test_common_command_handler_input_free(p_chi_4);
  test_common_command_handler_input_free(p_chi_5);

  mock_calls_verify();
}


void test_SWITCH_COLOR_handler_SET_pass_V3_WITH_TSE(void)
{
  mock_calls_clear();

  mock_t *pMock = NULL;
  received_frame_status_t status;
  zaf_tx_options_t txOptions;

  s_colorComponent colors[] = {
                                { .colorId = ECOLORCOMPONENT_RED,
                                  .cb = &app_color_cb },
                                { .colorId = ECOLORCOMPONENT_GREEN,
                                  .cb = &app_color_cb },
                                { .colorId = ECOLORCOMPONENT_BLUE,
                                  .cb = &app_color_cb }};

  cc_color_switch_get_default_duration_IgnoreAndReturn(2);
  cc_color_switch_get_length_colorComponents_IgnoreAndReturn(sizeof_array(colors));
  cc_color_switch_get_colorComponents_IgnoreAndReturn(colors);
  mock_call_use_as_fake(TO_STR(ZAF_Actuator_Init));
  for (int i = 0; i < sizeof_array(colors); i++)
  {
    cc_color_switch_read_ExpectAndReturn(i, NULL, false);
    cc_color_switch_read_IgnoreArg_color_component();
    cc_color_switch_write_IgnoreAndReturn(true);
    mock_call_expect(TO_STR(ZAF_Actuator_Set), &pMock);
    pMock->compare_rule_arg[0] = COMPARE_NOT_NULL;
    pMock->expect_arg[0].p = &colors[i].obj;
    pMock->expect_arg[1].v = 255;
    pMock->expect_arg[2].v = 0;
  }
  ZAF_CC_init_specific(COMMAND_CLASS_SWITCH_COLOR);

  // input data - one color, no supervision. Duration > 0.
  uint8_t colorsCount = 1;
  command_handler_input_t* p_chi_1 = inputbuff_SetV3_1Color(colorsCount,
                                                          ECOLORCOMPONENT_GREEN, 0xDD,
                                                          0xA);

  mock_call_use_as_fake(TO_STR(ZAF_Actuator_Set));
  mock_call_use_as_stub(TO_STR(ZAF_Actuator_GetDurationRemaining));
  mock_call_use_as_stub(TO_STR(ZAF_Actuator_GetTargetValue));
  mock_call_use_as_stub(TO_STR(ZAF_Actuator_GetCurrentValue));
  cc_color_switch_write_IgnoreAndReturn(true);

  mock_call_expect(TO_STR(ZAF_TSE_Trigger), &pMock);
  pMock->compare_rule_arg[0] = COMPARE_NOT_NULL; // TSE trigger
  pMock->compare_rule_arg[1] = COMPARE_NOT_NULL; //colorSwitchData;
  pMock->expect_arg[1].p = &colors[1];
  pMock->expect_arg[2].v = false;           //Overwrite_previous_trigger
  pMock->return_code.v = true;

  cc_handler_input_t input = {
      .rx_options = &p_chi_1->rxOptions,
      .frame = &p_chi_1->frame.as_zw_application_tx_buffer,
      .length = p_chi_1->frameLength
  };

  ZW_APPLICATION_TX_BUFFER frameOut = {0};
  cc_handler_output_t output = {
      .frame = &frameOut,
      .length = 0,
      .duration = 0
  };

  status = invoke_cc_handler(&input, &output);

  TEST_ASSERT_EQUAL_UINT16_MESSAGE(RECEIVED_FRAME_STATUS_SUCCESS, status, "Wrong status");

  // Fetch the callback passed to ZAF_TSE_Trigger().
  zaf_tse_callback_t cb = (zaf_tse_callback_t)pMock->actual_arg[0].p;

  uint8_t output_buff[] = {COMMAND_CLASS_SWITCH_COLOR_V3, SWITCH_COLOR_REPORT_V3, ECOLORCOMPONENT_GREEN, 0x0, 0x0, 0};

  memset((uint8_t*)&txOptions, 0x00, sizeof(zaf_tx_options_t));
  zaf_transport_tx_ExpectAndReturn(output_buff, sizeof(output_buff), ZAF_TSE_TXCallback, &txOptions, true);
  cb(&txOptions, &colors[1]);

  mock_calls_verify();

  // CASE 2 - change 3 colors, supervision enabled, status update on, duration 0
  mock_calls_clear();

  colorsCount = 3;
  uint8_t value = 0xCC;
  uint8_t duration = 0x00;
  command_handler_input_t* p_chi_2 = inputbuff_SetV3_3Colors(colorsCount,
                                  ECOLORCOMPONENT_RED, value,
                                  ECOLORCOMPONENT_GREEN, value,
                                  ECOLORCOMPONENT_BLUE, value,
                                  duration);
  p_chi_2->rxOptions.bSupervisionActive = 1;
  p_chi_2->rxOptions.sessionId = 33;
  p_chi_2->rxOptions.statusUpdate = 1;

  for (int i = 0; i< colorsCount; i++)
  {
    mock_call_expect(TO_STR(ZAF_Actuator_Set), &pMock);
    pMock->compare_rule_arg[0] = COMPARE_NOT_NULL;
    pMock->expect_arg[0].p = &colors[i].obj;
    pMock->expect_arg[1].v = value;
    pMock->expect_arg[2].v = duration;
    pMock->return_code.v = EACTUATOR_NOT_CHANGING;
  }

  cc_handler_input_t input2 = {
      .rx_options = &p_chi_2->rxOptions,
      .frame = &p_chi_2->frame.as_zw_application_tx_buffer,
      .length = p_chi_2->frameLength
  };
  status = invoke_cc_handler(&input2, &output);

  TEST_ASSERT_EQUAL_UINT16_MESSAGE(RECEIVED_FRAME_STATUS_SUCCESS, status, "Wrong status");
  mock_calls_verify();

  // CASE 3 - change 3 colors, supervision enabled, status update on, duration > 0
  mock_calls_clear();
  for (int i = 0; i < sizeof_array(colors); i++)
  {
    helper_setObjectValues(&colors[i].obj, 0, 0, 0);
  }


  colorsCount = 3;
  duration = 0xB;
  command_handler_input_t* p_chi_3 = inputbuff_SetV3_3Colors(colorsCount,
                                  ECOLORCOMPONENT_RED, value,
                                  ECOLORCOMPONENT_GREEN, value,
                                  ECOLORCOMPONENT_BLUE, value,
                                  duration);
  p_chi_3->rxOptions.bSupervisionActive = 1;
  p_chi_3->rxOptions.sessionId = 7;
  p_chi_3->rxOptions.statusUpdate = 1;

  for (int i = 0; i< colorsCount; i++)
  {
    mock_call_expect(TO_STR(ZAF_Actuator_Set), &pMock);
    pMock->compare_rule_arg[0] = COMPARE_NOT_NULL;
    pMock->expect_arg[0].p = &colors[i].obj;
    pMock->expect_arg[1].v = value;
    pMock->expect_arg[2].v = duration;
    pMock->return_code.v = EACTUATOR_CHANGING;
  }

  mock_call_expect(TO_STR(ZAF_Actuator_GetDurationRemaining), &pMock);
  pMock->compare_rule_arg[0] = COMPARE_NOT_NULL;
  pMock->return_code.v =  p_chi_3->frame.as_byte_array[p_chi_3->frameLength-1]; // requested duration;

  mock_call_expect(TO_STR(is_multicast), &pMock);
  pMock->compare_rule_arg[0] = COMPARE_NOT_NULL;
  pMock->return_code.v = false;  // is not a multicast

  cc_handler_input_t input3 = {
      .rx_options = &p_chi_3->rxOptions,
      .frame = &p_chi_3->frame.as_zw_application_tx_buffer,
      .length = p_chi_3->frameLength
  };

  status = invoke_cc_handler(&input3, &output);

  TEST_ASSERT_EQUAL_UINT16_MESSAGE(RECEIVED_FRAME_STATUS_WORKING, status, "Wrong status");
  mock_calls_verify();

  mock_call_expect(TO_STR(is_multicast), &pMock);
  pMock->compare_rule_arg[0] = COMPARE_NOT_NULL;
  pMock->return_code.v = false;  // is not a multicast

  for (int i = 0; i < colorsCount; i++)
  {
    mock_call_expect(TO_STR(ZAF_Actuator_GetTargetValue), &pMock);
    pMock->expect_arg[0].p = &colors[i].obj;
    pMock->return_code.v = value;

    mock_call_expect(TO_STR(ZAF_Actuator_GetCurrentValue), &pMock);
    pMock->expect_arg[0].p = &colors[i].obj;
    pMock->return_code.v = value/2;

    // call callback function in the middle of change
    colorswitch_cb(&colors[i].obj);
  }

  for (int i = 0; i < colorsCount; i++)
  {
    mock_call_expect(TO_STR(ZAF_Actuator_GetTargetValue), &pMock);
    pMock->expect_arg[0].p = &colors[i].obj;
    pMock->return_code.v = value;

    mock_call_expect(TO_STR(ZAF_Actuator_GetCurrentValue), &pMock);
    pMock->expect_arg[0].p = &colors[i].obj;
    pMock->return_code.v = value;

    cc_color_switch_write_IgnoreAndReturn(true);

    mock_call_expect(TO_STR(ZAF_TSE_Trigger), &pMock);
    pMock->compare_rule_arg[0] = COMPARE_NOT_NULL; // TSE trigger
    pMock->compare_rule_arg[1] = COMPARE_NOT_NULL; //colorSwitchData;
    pMock->expect_arg[1].p = &colors[i];
    pMock->expect_arg[2].v = false;           //Overwrite_previous_trigger
    pMock->return_code.v = true;

    if (i == (colorsCount - 1))
    {
      // supervision status update is set, send supervision report when all colors are done
      zaf_transport_rx_to_tx_options_Expect(&colors[colorsCount-1].rxOpt, NULL);
      zaf_transport_rx_to_tx_options_IgnoreArg_tx_options();

      mock_call_expect(TO_STR(CmdClassSupervisionReportSend), &pMock);
      pMock->compare_rule_arg[0] = COMPARE_NOT_NULL;
      pMock->expect_arg[1].v = p_chi_3->rxOptions.sessionId; // Status update - this is last
      pMock->expect_arg[2].v = 0xFF; //CC_SUPERVISION_STATUS_SUCCESS;
      pMock->expect_arg[3].v = 0; // duration is 0 now
      pMock->return_code.v = JOB_STATUS_SUCCESS;

    }
    // call callback function when change completed
    colorswitch_cb(&colors[i].obj);
  }

  test_common_command_handler_input_free(p_chi_1);
  test_common_command_handler_input_free(p_chi_2);
  test_common_command_handler_input_free(p_chi_3);

  mock_calls_verify();
}

void test_SWITCH_COLOR_HANDLER_SET_cb_func(void)
{
  mock_calls_clear();

  mock_t *pMock = NULL;
  received_frame_status_t status;
  s_colorComponent colors[] = {
                               { .colorId = ECOLORCOMPONENT_RED, },
                               { .colorId = ECOLORCOMPONENT_GREEN, },
                               { .colorId = ECOLORCOMPONENT_BLUE, }};

  cc_color_switch_get_default_duration_IgnoreAndReturn(2);
  cc_color_switch_get_length_colorComponents_IgnoreAndReturn(sizeof_array(colors));
  cc_color_switch_get_colorComponents_IgnoreAndReturn(colors);
  mock_call_use_as_fake(TO_STR(ZAF_Actuator_Init));
  for (int i = 0; i < sizeof_array(colors); i++)
  {
    cc_color_switch_read_ExpectAndReturn(i, NULL, false);
    cc_color_switch_read_IgnoreArg_color_component();
    cc_color_switch_write_IgnoreAndReturn(true);
    mock_call_expect(TO_STR(ZAF_Actuator_Set), &pMock);
    pMock->compare_rule_arg[0] = COMPARE_NOT_NULL;
    pMock->expect_arg[0].p = &colors[i].obj;
    pMock->expect_arg[1].v = 255;
    pMock->expect_arg[2].v = 0;
  }
  ZAF_CC_init_specific(COMMAND_CLASS_SWITCH_COLOR);

  // change 3 colors, supervision disabled, duration > 0
  int colorsCount = 3;
  command_handler_input_t *p_chi = inputbuff_SetV3_3Colors(colorsCount,
                                  ECOLORCOMPONENT_RED, 0xCC,
                                  ECOLORCOMPONENT_GREEN, 0xCC,
                                  ECOLORCOMPONENT_BLUE, 0xCC,
                                  0x0A);

  mock_call_use_as_fake(TO_STR(ZAF_Actuator_Set));
  mock_call_use_as_stub(TO_STR(ZAF_Actuator_GetDurationRemaining));
  mock_call_use_as_stub(TO_STR(ZAF_Actuator_GetTargetValue));
  mock_call_use_as_stub(TO_STR(ZAF_Actuator_GetCurrentValue));
  for (int i = 0; i< colorsCount; i++)
  {
    mock_call_expect(TO_STR(ZAF_TSE_Trigger), &pMock);
    pMock->compare_rule_arg[0] = COMPARE_NOT_NULL; // TSE trigger
    pMock->compare_rule_arg[1] = COMPARE_NOT_NULL; //colorSwitchData;
    pMock->expect_arg[1].p = &colors[i];
    pMock->expect_arg[2].v = false;           //Overwrite_previous_trigger
    pMock->return_code.v = true;
  }

  cc_handler_input_t input= {
      .rx_options = &p_chi->rxOptions,
      .frame = &p_chi->frame.as_zw_application_tx_buffer,
      .length = p_chi->frameLength
  };

  ZW_APPLICATION_TX_BUFFER frameOut = {0};
  cc_handler_output_t output = {
      .frame = &frameOut,
      .length = 0,
      .duration = 0
  };

  status = invoke_cc_handler(&input, &output);

  TEST_ASSERT_EQUAL_UINT16_MESSAGE(RECEIVED_FRAME_STATUS_SUCCESS, status, "Wrong status");
  // test_colorSwitch_cb was called at least colorsCount*2 times if duration > 0 (simulating refresh rate - there was at least one step)
  TEST_ASSERT_TRUE_MESSAGE(cb_counter >= 2*colorsCount, "Callback function triggered unexpected number of times");

  test_common_command_handler_input_free(p_chi);

  mock_calls_verify();  
}

void test_SWITCH_COLOR_handler_START_LEVEL_CHANGE_pass(void)
{
  mock_calls_clear();

  mock_t *pMock = NULL;
  received_frame_status_t status;
  s_colorComponent colors[] = {
                               { .colorId = ECOLORCOMPONENT_RED, },
                               { .colorId = ECOLORCOMPONENT_GREEN, },
                               { .colorId = ECOLORCOMPONENT_BLUE, }};

  cc_color_switch_get_default_duration_IgnoreAndReturn(2);
  cc_color_switch_get_length_colorComponents_IgnoreAndReturn(sizeof_array(colors));
  cc_color_switch_get_colorComponents_IgnoreAndReturn(colors);
  mock_call_use_as_stub(TO_STR(ZAF_Actuator_Init));
  mock_call_use_as_stub(TO_STR(ZAF_Actuator_Set));
  for (int i= 0; i < sizeof_array(colors); i++)
  {
    cc_color_switch_read_ExpectAndReturn(i, NULL, false);
    cc_color_switch_read_IgnoreArg_color_component();
    cc_color_switch_write_IgnoreAndReturn(true);
  }
  ZAF_CC_init_specific(COMMAND_CLASS_SWITCH_COLOR);

  // CASE 1 - V2 (duration = 0)
  mock_calls_clear();
  uint8_t upDownBitMask = 1 << 6;
  uint8_t ignoreStartLevelBitMask = 1 << 5;
  uint8_t properties = 0;
  command_handler_input_t *p_chi_1 = inputbuff_StartLevelChangeV2(properties, // properties - don't care
                                                                ECOLORCOMPONENT_RED,
                                                                0); // start level - don't care

  mock_call_expect(TO_STR(ZAF_Actuator_StartChange), &pMock);
  pMock->compare_rule_arg[0] = COMPARE_NOT_NULL;
  pMock->expect_arg[0].p = &colors[0].obj;
  pMock->expect_arg[1].v = 0; // UpDown
  pMock->expect_arg[2].v = 0; // Ignore StartLevel
  pMock->expect_arg[3].v = 0; // Start Level Change
  pMock->expect_arg[4].v = 0; // duration
  pMock->return_code.v = 0; // no change in progress - instant change

  cc_handler_input_t input= {
      .rx_options = &p_chi_1->rxOptions,
      .frame = &p_chi_1->frame.as_zw_application_tx_buffer,
      .length = p_chi_1->frameLength
  };

  ZW_APPLICATION_TX_BUFFER frameOut = {0};
  cc_handler_output_t output = {
      .frame = &frameOut,
      .length = 0,
      .duration = 0
  };

  status = invoke_cc_handler(&input, &output);


  TEST_ASSERT_EQUAL_UINT16_MESSAGE(RECEIVED_FRAME_STATUS_SUCCESS, status, "Wrong status");
  mock_calls_verify();


  // CASE 2 - V3 (duration != 0, no supervision)
  mock_calls_clear();
  properties = 0;
  uint8_t duration = 0xAA;
  command_handler_input_t *p_chi_2 = inputbuff_StartLevelChangeV3(properties, // properties - don't care
                          ECOLORCOMPONENT_RED,
                          0, // start level - don't care
                          duration);

  mock_call_expect(TO_STR(ZAF_Actuator_StartChange), &pMock);
  pMock->compare_rule_arg[0] = COMPARE_NOT_NULL;
  pMock->expect_arg[0].p = &colors[0].obj;
  pMock->expect_arg[1].v = 0; // UpDown
  pMock->expect_arg[2].v = 0; // Ignore StartLevel
  pMock->expect_arg[3].v = 0; // Start Level Change
  pMock->expect_arg[4].v = duration; // duration
  pMock->return_code.v = 1; // change in progress

  cc_handler_input_t input2 = {
      .rx_options = &p_chi_2->rxOptions,
      .frame = &p_chi_2->frame.as_zw_application_tx_buffer,
      .length = p_chi_2->frameLength
  };

  status = invoke_cc_handler(&input2, &output);

  TEST_ASSERT_EQUAL_UINT16_MESSAGE(RECEIVED_FRAME_STATUS_SUCCESS, status, "Wrong status");
  mock_calls_verify();


  // CASE 3 - V3 (duration != 0, supervision, decreasing, ignore Start Level)
  mock_calls_clear();

  properties = 0 | upDownBitMask | ignoreStartLevelBitMask;
  uint8_t startLevel = 0x0F;
  command_handler_input_t *p_chi_3 = inputbuff_StartLevelChangeV3(properties,
                                       ECOLORCOMPONENT_RED,
                                       startLevel,
                                       duration);

  p_chi_3->rxOptions.bSupervisionActive = 1;
  p_chi_3->rxOptions.sessionId = 33;
  p_chi_3->rxOptions.statusUpdate = 1;

  mock_call_expect(TO_STR(ZAF_Actuator_StartChange), &pMock);
  pMock->compare_rule_arg[0] = COMPARE_NOT_NULL;
  pMock->expect_arg[0].p = &colors[0].obj;
  pMock->expect_arg[1].v = 1; // UpDown
  pMock->expect_arg[2].v = 1; // Ignore StartLevel
  pMock->expect_arg[3].v = startLevel; // Start Level Change
  pMock->expect_arg[4].v = duration; // duration
  pMock->return_code.v = 1; // change in progress

  cc_handler_input_t input3 = {
      .rx_options = &p_chi_3->rxOptions,
      .frame = &p_chi_3->frame.as_zw_application_tx_buffer,
      .length = p_chi_3->frameLength
  };

  status = invoke_cc_handler(&input3, &output);

  TEST_ASSERT_EQUAL_UINT16_MESSAGE(RECEIVED_FRAME_STATUS_SUCCESS, status, "Wrong status");

  test_common_command_handler_input_free(p_chi_1);
  test_common_command_handler_input_free(p_chi_2);
  test_common_command_handler_input_free(p_chi_3);

  mock_calls_verify();
}

void test_SWITCH_COLOR_handler_START_LEVEL_CHANGE_fail(void)
{
  mock_calls_clear();

  received_frame_status_t status;
  s_colorComponent colors[] = {
                               { .colorId = ECOLORCOMPONENT_RED, },
                               { .colorId = ECOLORCOMPONENT_GREEN, },
                               { .colorId = ECOLORCOMPONENT_BLUE, }};

  cc_color_switch_get_default_duration_IgnoreAndReturn(2);
  cc_color_switch_get_length_colorComponents_IgnoreAndReturn(sizeof_array(colors));
  cc_color_switch_get_colorComponents_IgnoreAndReturn(colors);
  mock_call_use_as_stub(TO_STR(ZAF_Actuator_Init));
  mock_call_use_as_stub(TO_STR(ZAF_Actuator_Set));
  for (int i= 0; i < sizeof_array(colors); i++)
  {
    cc_color_switch_read_ExpectAndReturn(i, NULL, false);
    cc_color_switch_read_IgnoreArg_color_component();
    cc_color_switch_write_IgnoreAndReturn(true);
  }
  ZAF_CC_init_specific(COMMAND_CLASS_SWITCH_COLOR);

  // Invalid color
  command_handler_input_t *p_chi = inputbuff_StartLevelChangeV3(0, // properties - don't care
                                                                ECOLORCOMPONENT_CYAN,
                                                                0, // start level - don't care
                                                                0);// duration - don't care

  cc_handler_input_t input= {
      .rx_options = &p_chi->rxOptions,
      .frame = &p_chi->frame.as_zw_application_tx_buffer,
      .length = p_chi->frameLength
  };

  ZW_APPLICATION_TX_BUFFER frameOut = {0};
  cc_handler_output_t output = {
      .frame = &frameOut,
      .length = 0,
      .duration = 0
  };

  status = invoke_cc_handler(&input, &output);

  TEST_ASSERT_EQUAL_UINT16_MESSAGE(RECEIVED_FRAME_STATUS_FAIL, status, "Wrong status");

  test_common_command_handler_input_free(p_chi);

  mock_calls_verify();
}

void test_SWITCH_COLOR_handler_STOP_LEVEL_CHANGE_pass(void)
{
  mock_calls_clear();

  mock_t *pMock = NULL;
  received_frame_status_t status;
  s_colorComponent colors[] = {
                               { .colorId = ECOLORCOMPONENT_RED, },
                               { .colorId = ECOLORCOMPONENT_GREEN, },
                               { .colorId = ECOLORCOMPONENT_BLUE, }};

  cc_color_switch_get_default_duration_IgnoreAndReturn(2);
  cc_color_switch_get_length_colorComponents_IgnoreAndReturn(sizeof_array(colors));
  cc_color_switch_get_colorComponents_IgnoreAndReturn(colors);
  mock_call_use_as_stub(TO_STR(ZAF_Actuator_Init));
  mock_call_use_as_stub(TO_STR(ZAF_Actuator_Set));
  for (int i= 0; i < sizeof_array(colors); i++)
  {
    cc_color_switch_read_ExpectAndReturn(i, NULL, false);
    cc_color_switch_read_IgnoreArg_color_component();
    cc_color_switch_write_IgnoreAndReturn(true);
  }
  ZAF_CC_init_specific(COMMAND_CLASS_SWITCH_COLOR);

  // CASE1 - No change in progress
  command_handler_input_t *p_chi = inputbuff_StopLevelChange(ECOLORCOMPONENT_RED);

  mock_call_expect(TO_STR(ZAF_Actuator_StopChange), &pMock);
  pMock->compare_rule_arg[0] = COMPARE_NOT_NULL;
  pMock->return_code.v = 0;

  cc_handler_input_t input= {
      .rx_options = &p_chi->rxOptions,
      .frame = &p_chi->frame.as_zw_application_tx_buffer,
      .length = p_chi->frameLength
  };

  ZW_APPLICATION_TX_BUFFER frameOut = {0};
  cc_handler_output_t output = {
      .frame = &frameOut,
      .length = 0,
      .duration = 0
  };

  status = invoke_cc_handler(&input, &output);

  TEST_ASSERT_EQUAL_UINT16_MESSAGE(RECEIVED_FRAME_STATUS_SUCCESS, status, "Wrong status");
  mock_calls_verify();

  // CASE2 - Change is in progress
  mock_calls_clear();
  // set some values to simulate ongoing change
  helper_setObjectValues(&colors->obj, 0x0A, 0x0F, 9);

  mock_call_expect(TO_STR(ZAF_Actuator_StopChange), &pMock);
  pMock->compare_rule_arg[0] = COMPARE_NOT_NULL;
  pMock->return_code.v = 1;

  mock_call_expect(TO_STR(ZAF_TSE_Trigger), &pMock);
  pMock->compare_rule_arg[0] = COMPARE_NOT_NULL; // TSE trigger
  pMock->compare_rule_arg[1] = COMPARE_NOT_NULL; //colorSwitchData;
  pMock->expect_arg[1].p = &colors[0];
  pMock->expect_arg[2].v = false;           //Overwrite_previous_trigger
  pMock->return_code.v = true;

  status = invoke_cc_handler(&input, &output);

  TEST_ASSERT_EQUAL_UINT16_MESSAGE(RECEIVED_FRAME_STATUS_SUCCESS, status, "Wrong status");

  test_common_command_handler_input_free(p_chi);

  mock_calls_verify();
}

void test_SWITCH_COLOR_handler_STOP_LEVEL_CHANGE_fail(void)
{
  mock_calls_clear();

  received_frame_status_t status;
  s_colorComponent colors[] = {
                               { .colorId = ECOLORCOMPONENT_RED, },
                               { .colorId = ECOLORCOMPONENT_GREEN, },
                               { .colorId = ECOLORCOMPONENT_BLUE, }};

  cc_color_switch_get_default_duration_IgnoreAndReturn(2);
  cc_color_switch_get_length_colorComponents_IgnoreAndReturn(sizeof_array(colors));
  cc_color_switch_get_colorComponents_IgnoreAndReturn(colors);
  mock_call_use_as_stub(TO_STR(ZAF_Actuator_Init));
  mock_call_use_as_stub(TO_STR(ZAF_Actuator_Set));
  for (int i= 0; i < sizeof_array(colors); i++)
  {
    cc_color_switch_read_ExpectAndReturn(i, NULL, false);
    cc_color_switch_read_IgnoreArg_color_component();
    cc_color_switch_write_IgnoreAndReturn(true);
  }
  ZAF_CC_init_specific(COMMAND_CLASS_SWITCH_COLOR);

  // Invalid color
  command_handler_input_t *p_chi = inputbuff_StopLevelChange(ECOLORCOMPONENT_CYAN);

  cc_handler_input_t input= {
      .rx_options = &p_chi->rxOptions,
      .frame = &p_chi->frame.as_zw_application_tx_buffer,
      .length = p_chi->frameLength
  };

  ZW_APPLICATION_TX_BUFFER frameOut = {0};
  cc_handler_output_t output = {
      .frame = &frameOut,
      .length = 0,
      .duration = 0
  };

  status = invoke_cc_handler(&input, &output);

  TEST_ASSERT_EQUAL_UINT16_MESSAGE(RECEIVED_FRAME_STATUS_FAIL, status, "Wrong status");
  mock_calls_verify();

  // invalid frame length
  mock_calls_clear();
  uint8_t input_buff2[] = {COMMAND_CLASS_SWITCH_COLOR_V3, SWITCH_COLOR_STOP_LEVEL_CHANGE_V3};
  RECEIVE_OPTIONS_TYPE_EX rxOpt;

  input.rx_options = &rxOpt;
  input.frame = (ZW_APPLICATION_TX_BUFFER*)input_buff2;
  input.length = sizeof_array(input_buff2);
  memset(&frameOut, 0, sizeof(ZW_APPLICATION_TX_BUFFER));

  status = invoke_cc_handler(&input, &output);
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(RECEIVED_FRAME_STATUS_FAIL, status, "Invalid frame length");

  test_common_command_handler_input_free(p_chi);

  mock_calls_verify();
}

/* helper functions */
static command_handler_input_t* inputbuff_SupportedGet(void)
{
  command_handler_input_t *p_chi = test_common_command_handler_input_allocate();
  p_chi->frame.as_byte_array[p_chi->frameLength++] = COMMAND_CLASS_SWITCH_COLOR_V3;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = SWITCH_COLOR_SUPPORTED_GET;
  return p_chi;
}

static command_handler_input_t* inputbuff_Get(EColorComponents colorID)
{
  command_handler_input_t *p_chi = test_common_command_handler_input_allocate();
  p_chi->frame.as_byte_array[p_chi->frameLength++] = COMMAND_CLASS_SWITCH_COLOR_V3;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = SWITCH_COLOR_GET;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = colorID;
  return p_chi;
}

static command_handler_input_t* inputbuff_SetV3_1Color(uint8_t colorCount, EColorComponents colorID,
                                                       uint8_t value, uint8_t duration)
{
  command_handler_input_t *p_chi = test_common_command_handler_input_allocate();
  p_chi->frame.as_byte_array[p_chi->frameLength++] = COMMAND_CLASS_SWITCH_COLOR_V3;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = SWITCH_COLOR_SET;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = colorCount;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = colorID;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = value;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = duration;
  return p_chi;
}

static command_handler_input_t* inputbuff_SetV3_3Colors(uint8_t colorCount,
                                                        EColorComponents colorID1, uint8_t value1,
                                                        EColorComponents colorID2, uint8_t value2,
                                                        EColorComponents colorID3, uint8_t value3,
                                                        uint8_t duration)
{
  command_handler_input_t *p_chi = test_common_command_handler_input_allocate();
  p_chi->frame.as_byte_array[p_chi->frameLength++] = COMMAND_CLASS_SWITCH_COLOR_V3;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = SWITCH_COLOR_SET;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = colorCount,
  p_chi->frame.as_byte_array[p_chi->frameLength++] = colorID1;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = value1;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = colorID2;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = value2;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = colorID3;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = value3;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = duration;
  return p_chi;
}

static command_handler_input_t* inputbuff_SetV2_3Colors(uint8_t colorCount,
                                                        EColorComponents colorID1, uint8_t value1,
                                                        EColorComponents colorID2, uint8_t value2,
                                                        EColorComponents colorID3, uint8_t value3)
{
  return inputbuff_SetV3_3Colors(colorCount, colorID1, value1, colorID2, value2, colorID3, value3, 0);
}

static command_handler_input_t* inputbuff_StartLevelChangeV3(uint8_t properties,
                                                             EColorComponents colorID,
                                                             uint8_t startLevelChange,
                                                             uint8_t duration)
{
  command_handler_input_t *p_chi = test_common_command_handler_input_allocate();
  p_chi->frame.as_byte_array[p_chi->frameLength++] = COMMAND_CLASS_SWITCH_COLOR_V3;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = SWITCH_COLOR_START_LEVEL_CHANGE_V3;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = properties;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = colorID;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = startLevelChange;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = duration;
  return p_chi;
}

static command_handler_input_t* inputbuff_StartLevelChangeV2(uint8_t properties,
                                                             EColorComponents colorID,
                                                             uint8_t startLevelChange)
{
  return inputbuff_StartLevelChangeV3(properties, colorID, startLevelChange, 0);
}

static command_handler_input_t* inputbuff_StopLevelChange(EColorComponents colorID)
{
  command_handler_input_t *p_chi = test_common_command_handler_input_allocate();
  p_chi->frame.as_byte_array[p_chi->frameLength++] = COMMAND_CLASS_SWITCH_COLOR_V3;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = SWITCH_COLOR_STOP_LEVEL_CHANGE_V3;
  p_chi->frame.as_byte_array[p_chi->frameLength++] = colorID;
  return p_chi;
}
