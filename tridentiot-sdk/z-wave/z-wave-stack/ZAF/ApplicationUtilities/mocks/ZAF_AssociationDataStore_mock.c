// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file ZAF_AssociationDataStore_mock.c
 * @copyright 2022 Silicon Laboratories Inc.
 */

#include <ZAF_AssociationDataStore.h>
#include <mock_control.h>

#define MOCK_FILE "ZAF_AssociationDataStore_mock.c"

ZAF_ADS_Handle_t ZAF_ADS_Init (void* pStorage, uint8_t MaxNodeCount)
{
  mock_t * p_mock;

  static ZAF_ADS_STORAGE(zaf_ads_storage, 100);

  MOCK_CALL_RETURN_IF_USED_AS_STUB((ZAF_ADS_Handle_t)&zaf_ads_storage);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, NULL);
  MOCK_CALL_ACTUAL(p_mock, pStorage, MaxNodeCount);

  MOCK_CALL_COMPARE_INPUT_POINTER(p_mock, ARG0, pStorage);
  MOCK_CALL_COMPARE_INPUT_UINT8(p_mock, ARG1, MaxNodeCount);

  MOCK_CALL_RETURN_POINTER(p_mock, ZAF_ADS_Handle_t);
}

bool ZAF_ADS_Delete(ZAF_ADS_Handle_t handle, uint8_t NodeID)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(true);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, false);
  MOCK_CALL_ACTUAL(p_mock, handle, NodeID);

  MOCK_CALL_COMPARE_INPUT_POINTER(p_mock, ARG0, handle);
  MOCK_CALL_COMPARE_INPUT_UINT8(p_mock, ARG1, NodeID);

  MOCK_CALL_RETURN_VALUE(p_mock, bool);
}

e_ZAF_ADS_SecurityHighestClassState_t ZAF_ADS_GetNodeHighestSecurityClass(
    ZAF_ADS_Handle_t handle,
    uint8_t NodeID,
    security_key_t * pHighestClass)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(SECURITY_HIGHEST_CLASS_DISCOVERED);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, SECURITY_HIGHEST_CLASS_NOT_DISCOVERED);
  MOCK_CALL_ACTUAL(p_mock, handle, NodeID, pHighestClass);

  MOCK_CALL_COMPARE_INPUT_POINTER(p_mock, ARG0, handle);
  MOCK_CALL_COMPARE_INPUT_UINT8(p_mock, ARG1, NodeID);
  MOCK_CALL_COMPARE_INPUT_POINTER(p_mock, ARG2, pHighestClass);

  MOCK_CALL_RETURN_VALUE(p_mock, e_ZAF_ADS_SecurityHighestClassState_t);
}

bool ZAF_ADS_SetNodeHighestSecurityClass(
    ZAF_ADS_Handle_t handle,
    uint8_t NodeID,
    security_key_t HighestClass)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(SECURITY_HIGHEST_CLASS_DISCOVERED);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, SECURITY_HIGHEST_CLASS_NOT_DISCOVERED);
  MOCK_CALL_ACTUAL(p_mock, handle, NodeID, HighestClass);

  MOCK_CALL_COMPARE_INPUT_POINTER(p_mock, ARG0, handle);
  MOCK_CALL_COMPARE_INPUT_UINT8(p_mock, ARG1, NodeID);
  MOCK_CALL_COMPARE_INPUT_UINT8(p_mock, ARG2, HighestClass);

  MOCK_CALL_RETURN_VALUE(p_mock, bool);
}

e_ZAF_ADS_CC_Capability_t ZAF_ADS_IsNodeSupportingCC(
    ZAF_ADS_Handle_t handle,
    uint8_t NodeID,
    e_ZAF_ADS_CommandClass_t CC)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(CC_CAPABILITY_SUPPORTED);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, CC_CAPABILITY_NOT_DISCOVERED);
  MOCK_CALL_ACTUAL(p_mock, handle, NodeID, CC);

  MOCK_CALL_COMPARE_INPUT_POINTER(p_mock, ARG0, handle);
  MOCK_CALL_COMPARE_INPUT_UINT8(p_mock, ARG1, NodeID);
  MOCK_CALL_COMPARE_INPUT_UINT8(p_mock, ARG2, CC);

  MOCK_CALL_RETURN_VALUE(p_mock, e_ZAF_ADS_CC_Capability_t);
}

bool ZAF_ADS_SetNodeIsSupportingCC(
    ZAF_ADS_Handle_t handle,
    uint8_t NodeID,
    e_ZAF_ADS_CommandClass_t CC,
    e_ZAF_ADS_CC_Capability_t CC_Capability)
{
  mock_t * p_mock;

  MOCK_CALL_RETURN_IF_USED_AS_STUB(true);
  MOCK_CALL_FIND_RETURN_ON_FAILURE(p_mock, false);
  MOCK_CALL_ACTUAL(p_mock, handle, NodeID, CC, CC_Capability);

  MOCK_CALL_COMPARE_INPUT_POINTER(p_mock, ARG0, handle);
  MOCK_CALL_COMPARE_INPUT_UINT8(p_mock, ARG1, NodeID);
  MOCK_CALL_COMPARE_INPUT_UINT8(p_mock, ARG2, CC);
  MOCK_CALL_COMPARE_INPUT_UINT8(p_mock, ARG3, CC_Capability);

  MOCK_CALL_RETURN_VALUE(p_mock, bool);
}
