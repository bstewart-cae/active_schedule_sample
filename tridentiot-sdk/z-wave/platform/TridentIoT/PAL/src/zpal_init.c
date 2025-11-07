/// ****************************************************************************
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2024 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************

#include "zpal_init.h"
#include "zpal_watchdog.h"
#include "zpal_status.h"
#include "zpal_defs.h"
#include "sysctrl.h"
#include "dpd.h"
#include "rtc.h"

static zpal_library_type_t m_library_type = ZPAL_LIBRARY_TYPE_UNDEFINED;

bool zpal_init_is_valid(uint8_t generic_type, uint8_t specific_type)
{
  (void)generic_type;
  (void)specific_type;

  return true;
}

void zpal_init_invalidate(void)
{
  zpal_enable_watchdog(false);
  while(1);
}

zpal_reset_reason_t zpal_get_reset_reason (void)
{
  zpal_reset_reason_t reason = ZPAL_RESET_REASON_OTHER;
  static zpal_reset_reason_t deep_sleep_wakeup = ZPAL_RESET_REASON_OTHER;
  dpd_rst_cause_t reset_reason;
  sys_get_retention_reg(RET_REG_RESET_REASON_INDEX,(uint32_t*)&reset_reason);


  if      (reset_reason.bit.RST_CAUSE_POR == true)  reason = ZPAL_RESET_REASON_POWER_ON;  //(Reset_By_Power_On()          == true)
  else if (reset_reason.bit.RST_CAUSE_EXT == true)  reason = ZPAL_RESET_REASON_PIN;       //(Reset_By_External()          == true)
  else if (reset_reason.bit.RST_CAUSE_DPD == true)  reason = ZPAL_RESET_REASON_OTHER;     //(Reset_By_Deep_Power_Down()   == treu)
  else if (reset_reason.bit.RST_CAUSE_DS  == true)                                        //(Reset_By_Deep_Sleep()        == true)
  {
    /*
      To determine whether waking from deep sleep was caused by a timer event or an external GPIO event,
       we need to check the RTC status and then clear it.
       However, if we call this function again and find that reset_reason.bit.RST_CAUSE_DS is true,
       we will receive an incorrect reset reason since the RTC status has already been cleared.
       To address this issue, we use a static local variable (deep_sleep_wakeup), to hold the ZPAL reset reason.
       We only check the RTC status when `deep_sleep_wakeup` is equal to ZPAL_RESET_REASON_OTHER.

    */
    if (ZPAL_RESET_REASON_OTHER == deep_sleep_wakeup )
    {
      uint32_t status = Get_RTC_Status();
      Rtc_Disable_Alarm();
      if (status & 0x40)
      {
        // reset trigger by RTC timeout
        deep_sleep_wakeup = ZPAL_RESET_REASON_DEEP_SLEEP_WUT;
      }
      else
      {
        // reset triggered by external gpio int
        deep_sleep_wakeup = ZPAL_RESET_REASON_DEEP_SLEEP_EXT_INT;
      }
    }
    reason = deep_sleep_wakeup;
  }
  else if (reset_reason.bit.RST_CAUSE_WDT  == true)  reason = ZPAL_RESET_REASON_WATCHDOG;  //(Reset_By_WDT()             == true)
  else if (reset_reason.bit.RST_CAUSE_SOFT == true)  reason = ZPAL_RESET_REASON_SOFTWARE;  //(Reset_By_Software()        == true)
  else if (reset_reason.bit.RST_CAUSE_LOCK == true)  reason = ZPAL_RESET_REASON_OTHER;     //(Reset_By_Lock()            == true)
  else
  {
    // Do nothing.
  }

  return reason;
}

zpal_library_type_t zpal_get_library_type(void)
{
  return m_library_type;
}

zpal_status_t zpal_init_set_library_type(const zpal_library_type_t library_type)
{
  if ((ZPAL_LIBRARY_TYPE_UNDEFINED == m_library_type) && (ZPAL_LIBRARY_TYPE_UNDEFINED != library_type))
  {
    m_library_type = library_type;
    return ZPAL_STATUS_OK;
  }
  return ZPAL_STATUS_FAIL;
}
