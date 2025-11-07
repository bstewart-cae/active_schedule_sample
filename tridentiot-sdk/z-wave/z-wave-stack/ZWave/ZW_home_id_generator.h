// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file ZW_home_id_generator.h
 * @copyright 2022 Silicon Laboratories Inc.
 *
 * @brief Z-Wave Home ID Generator.
 * Generates HOME ID for ZWave controllers.
 */
#ifndef _ZW_HOME_ID_GENERATOR_H_
#define _ZW_HOME_ID_GENERATOR_H_

#include <stdint.h>

/**
* Return new random HomeId
*
* New Home will be outside reserved HOME ID range. E.g. it will be
* within 0xC0000000-0xFFFFFFFE.
*
* @return                   Randomly generated HomeId.
*/
uint32_t HomeIdGeneratorGetNewId(uint8_t * pHomeID);


#endif // _ZW_HOME_ID_GENERATOR_H_
