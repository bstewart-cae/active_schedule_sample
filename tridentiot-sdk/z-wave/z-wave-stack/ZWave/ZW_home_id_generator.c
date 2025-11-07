// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file ZW_home_id_generator.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include "ZW_home_id_generator.h"
//#include "TickTime.h"
#include <zpal_entropy.h>

/**
* Generate pseudo random HomeID in range 0xC0000000-0xFFFFFFFE
* \return New HomeID as 32bit unsigned integer
* \param[out] pHomeID HomeID pointer to HomeID byte array where new HomeID is placed
*
*/
uint32_t
HomeIdGeneratorGetNewId(uint8_t * pHomeID)
{
  /* New HomeID must be inside homeID range 0xC0000000-0xFFFFFFFE */

  uint32_t uHomeId = 0;
  if (pHomeID)
  {
    do
    {
      zpal_get_random_data((uint8_t *)&uHomeId, sizeof(uHomeId));
      if (0xC0000000 > uHomeId)
      {
        uHomeId |= 0xC0000000;
      }
    } while (0xFFFFFFFF == uHomeId);

    // Convert the newly generated HomeId to Big-endian.
    pHomeID[0] = (uHomeId >> 24) & 0xff;
    pHomeID[1] = (uHomeId >> 16) & 0xff;
    pHomeID[2] = (uHomeId >> 8) & 0xff;
    pHomeID[3] = (uHomeId >> 0) & 0xff;
  }
  return uHomeId;  // The HomeID-array is used in big-endian form.
}
