/**
 * @file CC_DeviceResetLocally_mock.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include <ZW_typedefs.h>
#include <CC_DeviceResetLocally.h>
#include <mock_control.h>

#define MOCK_FILE "CC_DeviceResetLocally_mock.c"

void CC_DeviceResetLocally_notification_tx(void)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(p_mock);
}
