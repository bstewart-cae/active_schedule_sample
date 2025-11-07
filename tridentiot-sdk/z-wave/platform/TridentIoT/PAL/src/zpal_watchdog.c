/// ****************************************************************************
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2023 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************

#include "zpal_watchdog.h"
#include "tr_hal_wdog.h"
#include "sysctrl.h"
#include "chip_define.h"

static volatile bool enabled;

static tr_hal_wdog_settings_t wdog_setting = DEFAULT_WDOG_CONFIG;

#define ms_to_wdt_tick(ms) ((TR_HAL_WDOG_1_SECOND_TIMER_VALUE/1000) * ms)

void zpal_watchdog_init(void)
{
  wdog_setting.initial_value = ms_to_wdt_tick(2000);
  tr_hal_wdog_init(&wdog_setting);
}

bool zpal_is_watchdog_enabled(void)
{
  return enabled;
}

void zpal_enable_watchdog(bool enable)
{
  enabled = enable;
  if (enable)
  {
    tr_hal_wdog_enable();
    zpal_feed_watchdog();
  }
  else
  {
    tr_hal_wdog_disable();
  }
}

void zpal_feed_watchdog(void)
{
  if (enabled)
  {
    tr_hal_wdog_reset();
  }
}
