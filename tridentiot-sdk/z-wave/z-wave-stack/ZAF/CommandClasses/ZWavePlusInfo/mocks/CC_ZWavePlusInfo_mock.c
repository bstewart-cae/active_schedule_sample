/**
 * @file CC_ZWavePlusInfo_mock.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include <cc_zwave_plus_info_config_api.h>
#include <ZW_typedefs.h>
#include <mock_control.h>

#define MOCK_FILE "CC_ZWavePlusInfo_mock.c"

zw_plus_info_config_icons_t *cc_zwave_plus_info_config_get_endpoint_entry(uint8_t endpoint)
{
  mock_t *p_mock;

  static zw_plus_info_config_icons_t icons =
  {
    .installer_icon_type = ICON_TYPE_UNASSIGNED,
    .user_icon_type = ICON_TYPE_UNASSIGNED,
    .endpoint = 0
  };

  MOCK_CALL_RETURN_IF_USED_AS_STUB(&icons);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, &icons);
  MOCK_CALL_ACTUAL(p_mock, endpoint);
  MOCK_CALL_COMPARE_INPUT_UINT8(p_mock, ARG0, endpoint);

  MOCK_CALL_RETURN_POINTER(p_mock, zw_plus_info_config_icons_t *);
}

uint8_t cc_zwave_plus_info_config_get_endpoint_count(void)
{
  mock_t *p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(0);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, 0);
  MOCK_CALL_RETURN_IF_ERROR_SET(p_mock, uint8_t);

  MOCK_CALL_RETURN_VALUE(p_mock, uint8_t);
}