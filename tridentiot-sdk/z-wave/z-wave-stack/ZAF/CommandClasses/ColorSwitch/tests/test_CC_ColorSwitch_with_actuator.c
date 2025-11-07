
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
#include "AppTimer_mock.h"

#define CC_ColorSwitch_handler(a) invoke_cc_handler(a,NULL)

static command_handler_input_t* inputbuff_SetV3_3Colors(uint8_t colorCount,
                                                        EColorComponents colorID1, uint8_t value1,
                                                        EColorComponents colorID2, uint8_t value2,
                                                        EColorComponents colorID3, uint8_t value3,
                                                        uint8_t duration);

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

void setUpSuite(void)
{

}

void tearDownSuite(void) {

}

void setUp(void)
{

}

void tearDown(void)
{

}

/*
 * This test verifies the flow of Color Switch Set V2 with True Status and Supervision.
 *
 * As the duration is set to zero the expected behavior is:
 * 1. Trigger True Status
 * 2. Don't transmit a Supervision Report, but return success so that CC Supervision will take
 *    care of transmitting the Supervision Report.
 */
void test_SWITCH_COLOR_handler_SET_pass_V3_TSE(void)
{
  mock_t *pMock = NULL;
  command_handler_input_t* p_chi = NULL;

  mock_calls_clear();

  received_frame_status_t status;

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

  for (int i = 0; i < sizeof_array(colors); i++)
  {
    AppTimerRegister_IgnoreAndReturn(true); // Invoked by ZAF_Actuator_Init().
    cc_color_switch_read_ExpectAndReturn(i, NULL, false);
    cc_color_switch_read_IgnoreArg_color_component();
    cc_color_switch_write_IgnoreAndReturn(true);
  }
  ZAF_CC_init_specific(COMMAND_CLASS_SWITCH_COLOR);

  // CASE 2 - change 3 colors, supervision enabled, status update on, duration 0
  uint8_t colorsCount = 3;
  uint8_t value = 0xCC;
  uint8_t duration = 0x00;
  p_chi = inputbuff_SetV3_3Colors(colorsCount,
                                  ECOLORCOMPONENT_RED, value,
                                  ECOLORCOMPONENT_GREEN, value,
                                  ECOLORCOMPONENT_BLUE, value,
                                  duration);
  p_chi->rxOptions.bSupervisionActive = 1;
  p_chi->rxOptions.sessionId = 33;
  p_chi->rxOptions.statusUpdate = 1;

  for (int i = 0; i< colorsCount; i++)
  {
    mock_call_expect(TO_STR(ZAF_TSE_Trigger), &pMock);
    pMock->compare_rule_arg[0] = COMPARE_NOT_NULL; // TSE trigger
    pMock->compare_rule_arg[1] = COMPARE_NOT_NULL; //colorSwitchData;
    pMock->expect_arg[1].p = &colors[i];
    pMock->expect_arg[2].v = false;           //Overwrite_previous_trigger
    pMock->return_code.v = true;
  }

  cc_handler_input_t input = {
      .rx_options = &p_chi->rxOptions,
      .frame = &p_chi->frame.as_zw_application_tx_buffer,
      .length = p_chi->frameLength
  };
  status = CC_ColorSwitch_handler(&input);
  TEST_ASSERT_EQUAL_UINT16_MESSAGE(RECEIVED_FRAME_STATUS_SUCCESS, status, "Wrong status");

  test_common_command_handler_input_free(p_chi);

  mock_calls_verify();
}

/* helper functions */
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
