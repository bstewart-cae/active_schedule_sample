/**
 * @file
 * Configuration file for the ZAF 
 *
 * @copyright 2022 Silicon Laboratories Inc.
 */

#include "zaf_config_api.h"
#include "mock_control.h"

#define MOCK_FILE "zaf_config_mock.c"

uint8_t zaf_config_get_bootloader_upgradable(void)
{
    return 0;
}

uint8_t zaf_config_get_bootloader_target_id(void)
{
    return 1;
}

uint16_t zaf_config_get_build_no(void)
{
    mock_t * pMock;

    MOCK_CALL_RETURN_IF_USED_AS_STUB(0xffff);
    MOCK_CALL_FIND_RETURN_ON_FAILURE(pMock, 0xaaaa);
    MOCK_CALL_RETURN_VALUE(pMock, uint16_t); 

}

uint8_t zaf_config_get_hardware_version(void)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(1);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(pMock, 0xFF);
  MOCK_CALL_RETURN_VALUE(pMock, uint8_t);
}

uint8_t zaf_config_get_firmware_target_count(void)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(1);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(pMock, 0xFF);
  MOCK_CALL_RETURN_VALUE(pMock, uint8_t);
}

uint16_t zaf_config_get_manufacturer_id(void)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(0);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, 0x00);

  MOCK_CALL_RETURN_VALUE(p_mock, uint16_t);
}

uint16_t zaf_config_get_product_type_id(void)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(0);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, 0x00);

  MOCK_CALL_RETURN_VALUE(p_mock, uint16_t);
}

uint16_t zaf_config_get_product_id(void)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(0);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, 0x00);

  MOCK_CALL_RETURN_VALUE(p_mock, uint16_t);
}

uint8_t zaf_config_get_number_of_endpoints(void)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(0);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, 0x00);

  MOCK_CALL_RETURN_VALUE(p_mock, uint8_t);
}

uint8_t zaf_config_get_default_endpoint(void) {
  mock_t * p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(0);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, 0x00);

  MOCK_CALL_RETURN_VALUE(p_mock, uint8_t);
}

uint8_t zaf_config_get_role_type(void) {
  mock_t * p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(0);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, 0x00);

  MOCK_CALL_RETURN_VALUE(p_mock, uint8_t);
}
