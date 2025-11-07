// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file ZW_receivefilter_learnmode_mock.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include "ZW_DataLinkLayer.h"

#define MOCK_FILE "ZW_receivefilter_learnmode_mock.c"

ZW_ReturnCode_t rfLearnModeFilter_Set(uint8_t bMode,
                                      node_id_t nodeId,
                                      uint8_t pHomeId[HOME_ID_LENGTH])
{
  return SUCCESS;
}
