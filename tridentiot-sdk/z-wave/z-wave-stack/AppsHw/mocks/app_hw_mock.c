/*
 * app_hw_mock.c
 *
 */

#include <mock_control.h>
#include <app_hw.h>

#define MOCK_FILE "app_hw_mock.c"

void app_hw_init(void)
{
  mock_t * p_mock;
  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(p_mock);
}

void app_hw_deep_sleep_wakeup_handler(void)
{
  mock_t * pMock;

  MOCK_CALL_RETURN_VOID_IF_USED_AS_STUB();
  MOCK_CALL_FIND_RETURN_VOID_ON_FAILURE(pMock);
}
