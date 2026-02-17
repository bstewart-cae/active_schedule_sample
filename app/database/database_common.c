/*
 * SPDX-FileCopyrightText: 2026 Z-Wave Alliance <http://z-wavealliance.org>
 * SPDX-FileCopyrightText: 2026 Card Access Engineering, LLC <http://www.caengineering.com>
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
/**
 * @file database_common.c
 * @author bstewart-cae
 * @brief This contains definitions for common database operations shared
 *        by multiple application level modules.
 *
 * @copyright 2026 Card Access Engineering, LLC on behalf of the Z-Wave Alliance
 */

#ifdef __cplusplus
extern "C" {
#endif

/****************************************************************************/
/*                              INCLUDE FILES                               */
/****************************************************************************/
#include "database_common.h"
#include "app_schedules.h"
#include "ZAF_nvm_app.h"

/****************************************************************************/
/*                      PRIVATE TYPES and DEFINITIONS                       */
/****************************************************************************/

/****************************************************************************/
/*                              PRIVATE DATA                                */
/****************************************************************************/

/****************************************************************************/
/*                       PRIVATE FUNCTION DECLARATIONS                      */
/****************************************************************************/

/****************************************************************************/
/*                       EXPORTED FUNCTION DEFINITIONS                      */
/****************************************************************************/

bool app_nvm_init(void)
{
  ZAF_nvm_app_init();
  app_sch_initialize_handlers();
  return true;
}

bool app_nvm(
  app_nvm_operation_t operation, app_nvm_area_t area, uint16_t offset, void* pData,
  uint16_t size)
{
  // Set parameters depending on the NVM area
  zpal_nvm_object_key_t file_base;
  switch (area) {
  /**********************/ 
  /* Known size objects */
  /**********************/
    case APP_NVM_AREA_MIGRATION_TABLE:
      file_base = APP_NVM_FILE_MIGRATION_TABLE;
      size = 0; // FIXME: Store migration mapping information in here.
      offset = 0;
      break;

    /************************/
    /* Dynamic size objects */
    /************************/
    case APP_NVM_AREA_SCHEDULE_DATA:
      file_base = APP_NVM_FILE_SCHEDULE_DATA_BASE;
      break;
      
    case APP_NVM_AREA_MIGRATION_DATA:
      file_base = APP_NVM_FILE_MIGRATION_DATA_BASE;
      break;

    default:
      return false;
  }

  if (size == 0) {
    return true;
  }

  zpal_status_t nvm_result = ZPAL_STATUS_FAIL;
  switch (operation) {
    case U3C_READ:
      nvm_result = ZAF_nvm_app_read(file_base + offset, pData, (size_t)size);
      break;
    case U3C_WRITE:
      nvm_result = ZAF_nvm_app_write(file_base + offset, pData, (size_t)size);
      break;
    default:
      break;
  }
  return nvm_result == ZPAL_STATUS_OK;
}
/****************************************************************************/
/*                       PRIVATE FUNCTION DEFINITIONS                       */
/****************************************************************************/

#ifdef __cplusplus
}
#endif