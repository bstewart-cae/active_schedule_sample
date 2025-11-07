
#include <CC_WakeUp.h>
#include <ZW_basis_api.h>
#include <CC_Common.h>
#include <ZAF_types.h>
#include <mock_control.h>

#define MOCK_FILE "CC_WakeUp_config_mock.c"

// Exactly as in test_CC_WakeUp.c
#define WAKEUP_PAR_MIN_SLEEP_TIME_VALUE      60
#define WAKEUP_PAR_MAX_SLEEP_TIME_VALUE     120
#define WAKEUP_PAR_DEFAULT_SLEEP_TIME_VALUE  90
#define WAKEUP_PAR_SLEEP_STEP_VALUE          10

uint32_t cc_wake_up_config_get_default_sleep_time_sec(void)
{
 return WAKEUP_PAR_DEFAULT_SLEEP_TIME_VALUE;
}

uint32_t cc_wake_up_config_get_minimum_sleep_time_sec(void)
{
  return WAKEUP_PAR_MIN_SLEEP_TIME_VALUE;
}

uint32_t cc_wake_up_config_get_maximum_sleep_time_sec(void)
{
  return WAKEUP_PAR_MAX_SLEEP_TIME_VALUE;
}

uint32_t cc_wake_up_config_get_sleep_step_time_sec(void)
{
  return WAKEUP_PAR_SLEEP_STEP_VALUE;
}