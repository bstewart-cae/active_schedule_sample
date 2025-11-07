// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file ZW_inclusion_controller.c
 * @brief Implements inclusion controller functionality, most functionality is still
 * in zw_controller.c and zw_replication.c but should be moved here when possible
 * @copyright 2021 Silicon Laboratories Inc.
 */

#include "ZW_typedefs.h"
#include "ZW_lib_defines.h"

bool nodeIdserverPresent; /* true - a NodeID server is persent in the network. */

bool isNodeIDServerPresent(void)
{
  return nodeIdserverPresent; /* true - a NodeID server is persent in the network. */
}

void SetNodeIDServerPresent(bool bPresent)
{
  nodeIdserverPresent = bPresent;
}
