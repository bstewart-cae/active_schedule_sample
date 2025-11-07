// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file ZAF_AssociationDataStore.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include "ZAF_AssociationDataStore.h"
#include <Assert.h>

typedef struct
{
    uint8_t NodeID;
    s_ZAF_ADS_SecurityHighestClass_t highestSecurityClass;
    e_ZAF_ADS_CC_Capability_t supportedCCs[ZAF_ADS_MAX_CC];
} s_ZAF_ADS_NodeIDEntry_t;

typedef struct
{
  uint8_t nodeIDListLength;
  s_ZAF_ADS_NodeIDEntry_t nodeIDList[]; // Flexible array
} s_ZAF_ADS_Context_t;

/*
 * A few sanity checks to ensure the public ADS_STORAGE_SIZE macro is aligned with the private
 * struct definitions.
 */
STATIC_ASSERT(ZAF_ADS_HEADER_SIZE  == offsetof(s_ZAF_ADS_Context_t, nodeIDList),     ASSERT_ZAF_ADS_STORAGE_SIZE_macro_headersize_incorrect_1);
STATIC_ASSERT(ZAF_ADS_ELEMENT_SIZE == sizeof(s_ZAF_ADS_NodeIDEntry_t),               ASSERT_ZAF_ADS_STORAGE_SIZE_macro_elementsize_incorrect_1);
STATIC_ASSERT(ZAF_ADS_STORAGE_SIZE(1)  == offsetof(s_ZAF_ADS_Context_t, nodeIDList[1]),  ASSERT_ZAF_ADS_STORAGE_SIZE_macro_calc_incorrect_1);
STATIC_ASSERT(ZAF_ADS_STORAGE_SIZE(15) == offsetof(s_ZAF_ADS_Context_t, nodeIDList[15]), ASSERT_ZAF_ADS_STORAGE_SIZE_macro_calc_incorrect_15);

static void ClearNodeEntryData(s_ZAF_ADS_NodeIDEntry_t * pNodeEntry)
{
  pNodeEntry->NodeID = 0;
  pNodeEntry->highestSecurityClass.state = SECURITY_HIGHEST_CLASS_NOT_DISCOVERED;
  pNodeEntry->highestSecurityClass.securityKey = SECURITY_KEY_NONE;
  for (uint8_t j = 0; j < ZAF_ADS_MAX_CC; j++)
  {
    pNodeEntry->supportedCCs[j] = CC_CAPABILITY_NOT_DISCOVERED;
  }
}

ZAF_ADS_Handle_t ZAF_ADS_Init(void * pStorage, uint8_t MaxNodeCount)
{
  s_ZAF_ADS_Context_t * pContext = (s_ZAF_ADS_Context_t *)pStorage;
  pContext->nodeIDListLength = MaxNodeCount;

  for (uint8_t i = 0; i < pContext->nodeIDListLength; i++)
  {
    ClearNodeEntryData(&pContext->nodeIDList[i]);
  }
  return pContext;
}

static s_ZAF_ADS_NodeIDEntry_t * LookupEntry(ZAF_ADS_Handle_t handle, uint8_t NodeID)
{
  s_ZAF_ADS_Context_t * pContext = handle;

  for (uint8_t i = 0; i < pContext->nodeIDListLength; i++)
  {
    if (pContext->nodeIDList[i].NodeID == NodeID)
    {
      return &pContext->nodeIDList[i];
    }
  }
  return NULL;
}

bool ZAF_ADS_SetNodeHighestSecurityClass(
    ZAF_ADS_Handle_t handle,
    uint8_t NodeID,
    security_key_t HighestClass)
{
  s_ZAF_ADS_NodeIDEntry_t * pNodeEntry = LookupEntry(handle, NodeID);
  if (NULL != pNodeEntry)
  {
    pNodeEntry->highestSecurityClass.securityKey = HighestClass;
    pNodeEntry->highestSecurityClass.state = SECURITY_HIGHEST_CLASS_DISCOVERED;
    return true;
  }

  // If we did not find the given Node ID, search for an empty entry.
  pNodeEntry = LookupEntry(handle, 0);
  if (NULL != pNodeEntry)
  {
    pNodeEntry->NodeID = NodeID;
    pNodeEntry->highestSecurityClass.securityKey = HighestClass;
    pNodeEntry->highestSecurityClass.state = SECURITY_HIGHEST_CLASS_DISCOVERED;
    return true;
  }
  return false;
}

e_ZAF_ADS_SecurityHighestClassState_t ZAF_ADS_GetNodeHighestSecurityClass(
    ZAF_ADS_Handle_t handle,
    uint8_t NodeID,
    security_key_t * pHighestClass)
{
  s_ZAF_ADS_NodeIDEntry_t * pNodeEntry = LookupEntry(handle, NodeID);
  if (NULL != pNodeEntry)
  {
    *pHighestClass = pNodeEntry->highestSecurityClass.securityKey;
    return pNodeEntry->highestSecurityClass.state;
  }

  return SECURITY_HIGHEST_CLASS_NOT_DISCOVERED;
}

bool ZAF_ADS_Delete(ZAF_ADS_Handle_t handle, uint8_t NodeID)
{
  s_ZAF_ADS_NodeIDEntry_t * pNodeEntry = LookupEntry(handle, NodeID);
  if (NULL != pNodeEntry)
  {
    ClearNodeEntryData(pNodeEntry);
    return true;
  }

  return false;
}

e_ZAF_ADS_CC_Capability_t ZAF_ADS_IsNodeSupportingCC(
    ZAF_ADS_Handle_t handle,
    uint8_t NodeID,
    e_ZAF_ADS_CommandClass_t commandClass)
{
  if (ZAF_ADS_MAX_CC <= commandClass)
  {
    return CC_CAPABILITY_NOT_SUPPORTED;
  }

  s_ZAF_ADS_NodeIDEntry_t * pNodeEntry = LookupEntry(handle, NodeID);
  if (NULL != pNodeEntry)
  {
    return pNodeEntry->supportedCCs[commandClass];
  }

  return CC_CAPABILITY_NOT_DISCOVERED;
}

bool ZAF_ADS_SetNodeIsSupportingCC(
    ZAF_ADS_Handle_t handle,
    uint8_t NodeID,
    e_ZAF_ADS_CommandClass_t commandClass,
    e_ZAF_ADS_CC_Capability_t CC_Capability)
{
  if (ZAF_ADS_MAX_CC <= commandClass)
  {
    return false;
  }

  s_ZAF_ADS_NodeIDEntry_t * pNodeEntry = LookupEntry(handle, NodeID);
  if (NULL != pNodeEntry)
  {
    pNodeEntry->supportedCCs[commandClass] = CC_Capability;
    return true;
  }

  pNodeEntry = LookupEntry(handle, 0);
  if (NULL != pNodeEntry)
  {
    pNodeEntry->NodeID = NodeID;
    pNodeEntry->supportedCCs[commandClass] = CC_Capability;
    return true;
  }

  return false;
}
