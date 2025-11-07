// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file ZW_network_management_mock.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include <SwTimer.h>

static bool smartStartInclusionOngoing = false;  // Indicates that a SmartStart Related transmission is in progress

bool NetworkManagement_IsSmartStartInclusionOngoing(void)
{
  return smartStartInclusionOngoing;
}

void NetworkManagement_IsSmartStartInclusionSet(void)
{
  smartStartInclusionOngoing = true;
}

void NetworkManagement_IsSmartStartInclusionClear(void)
{
  smartStartInclusionOngoing = false;
}

void
NetworkManagementStopSetNWI(void)
{

}

void
NetworkManagementGenerateDskpart(void)
{

}

void
ZCB_NetworkManagementSetNWI(SSwTimer* pTimer)
{

}

void
NetworkManagementStartSetNWI(void)
{

}
