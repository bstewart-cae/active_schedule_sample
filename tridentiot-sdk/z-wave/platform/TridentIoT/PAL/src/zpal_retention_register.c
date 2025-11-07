/// ****************************************************************************
/// SPDX-License-Identifier: LicenseRef-TridentMSLA
/// SPDX-FileCopyrightText: 2023 Trident IoT, LLC <https://www.tridentiot.com>
/// ***************************************************************************

#include "zpal_retention_register.h"
#include "zpal_defs.h"
#include "tr_platform.h"
#include "sysfun.h"

#define ZPAL_RETENTION_REGISTER_COUNT 32
//static uint32_t reg_file[ZPAL_RETENTION_REGISTER_COUNT]  __attribute__((section(".ret_sram"))) __attribute__((used));
static REGISTER_RETENTION_ARRAY(uint32_t, reg_file, ZPAL_RETENTION_REGISTER_COUNT);

void zpal_retention_register_clear(void)
{
  for (size_t i = 0; i < ZPAL_RETENTION_REGISTER_COUNT; i++)
    zpal_retention_register_write(i, 0);
}

zpal_status_t zpal_retention_register_read(uint32_t index, uint32_t *data)
{
  if (index >= ZPAL_RETENTION_REGISTER_COUNT)
  {
    return ZPAL_STATUS_INVALID_ARGUMENT;
  }

  *data = reg_file[index];
  return ZPAL_STATUS_OK;
}

zpal_status_t zpal_retention_register_write(uint32_t index, uint32_t value)
{
  if (index >= ZPAL_RETENTION_REGISTER_COUNT)
  {
    return ZPAL_STATUS_INVALID_ARGUMENT;
  }

  enter_critical_section();
  reg_file[index] = value;
  leave_critical_section();
  return ZPAL_STATUS_OK;
}

size_t zpal_retention_register_count(void)
{
  return ZPAL_RETENTION_REGISTER_COUNT;
}
