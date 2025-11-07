// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file test_ZAF_AssociationDataStore.c
 * @copyright 2022 Silicon Laboratories Inc.
 */

#include <ZAF_AssociationDataStore.h>
#include <unity.h>

void setUpSuite(void) {

}

void tearDownSuite(void) {

}

void test_init_and_adding_removing_one_node(void)
{
  const uint8_t ADS_ELEMENTS  = 10;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wvla"  
  ZAF_ADS_STORAGE(ADSObject, ADS_ELEMENTS);
#pragma GCC diagnostic pop
  ZAF_ADS_Handle_t handle;

  handle = ZAF_ADS_Init(&ADSObject, ADS_ELEMENTS);

  const uint8_t NODE_ID = 2;

  TEST_ASSERT_TRUE_MESSAGE(ZAF_ADS_SetNodeHighestSecurityClass(handle, NODE_ID, SECURITY_KEY_S2_ACCESS), "");

  security_key_t key;
  e_ZAF_ADS_SecurityHighestClassState_t result;

  result = ZAF_ADS_GetNodeHighestSecurityClass(handle, NODE_ID, &key);
  TEST_ASSERT_TRUE_MESSAGE(SECURITY_HIGHEST_CLASS_DISCOVERED == result, "");
  TEST_ASSERT_TRUE_MESSAGE(SECURITY_KEY_S2_ACCESS == key, "");

  TEST_ASSERT_TRUE_MESSAGE(ZAF_ADS_Delete(handle, NODE_ID), "");

  result = ZAF_ADS_GetNodeHighestSecurityClass(handle, NODE_ID, &key);
  TEST_ASSERT_TRUE_MESSAGE(SECURITY_HIGHEST_CLASS_NOT_DISCOVERED == result, "");
}

/*
 * Verifies that the node located on the last location in the array can be found when nodes
 * at a lower index are deleted.
 */
void test_overwrite_existing(void)
{
  const uint8_t ADS_ELEMENTS  = 3;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wvla"
  ZAF_ADS_STORAGE(ADSObject, ADS_ELEMENTS);
#pragma GCC diagnostic pop
  ZAF_ADS_Handle_t handle;

  handle = ZAF_ADS_Init(&ADSObject, ADS_ELEMENTS);

  const uint8_t NODE_ID = 3;
  const uint8_t ANOTHER_NODE_ID = 10;
  security_key_t key;
  e_ZAF_ADS_SecurityHighestClassState_t result;

  // Fill all the spots
  for (uint8_t i = 1; i <= ADS_ELEMENTS; i++)
  {
    TEST_ASSERT_TRUE_MESSAGE(ZAF_ADS_SetNodeHighestSecurityClass(handle, i, SECURITY_KEY_S2_ACCESS), "");
  }

  // Delete the middle node
  TEST_ASSERT_TRUE_MESSAGE(ZAF_ADS_Delete(handle, 2), "");

  // Check that a specific node is found and changed
  TEST_ASSERT_TRUE_MESSAGE(ZAF_ADS_SetNodeHighestSecurityClass(handle, NODE_ID, SECURITY_KEY_S2_AUTHENTICATED), "");

  result = ZAF_ADS_GetNodeHighestSecurityClass(handle, NODE_ID, &key);
  TEST_ASSERT_TRUE_MESSAGE(SECURITY_HIGHEST_CLASS_DISCOVERED == result, "");
  TEST_ASSERT_TRUE_MESSAGE(SECURITY_KEY_S2_AUTHENTICATED == key, "");

  // Check that one new node can be added to make sure that node 3 was not added again
  TEST_ASSERT_TRUE_MESSAGE(ZAF_ADS_SetNodeHighestSecurityClass(handle, ANOTHER_NODE_ID, SECURITY_KEY_S2_AUTHENTICATED), "");
}

void test_add_too_many(void)
{
  const uint8_t ADS_ELEMENTS  = 10;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wvla"  
  ZAF_ADS_STORAGE(ADSObject, ADS_ELEMENTS);
#pragma GCC diagnostic pop
  ZAF_ADS_Handle_t handle;

  handle = ZAF_ADS_Init(&ADSObject, ADS_ELEMENTS);

  uint8_t i;
  for (i = 1; i <= ADS_ELEMENTS; i++)
  {
    TEST_ASSERT_TRUE_MESSAGE(ZAF_ADS_SetNodeHighestSecurityClass(handle, i, SECURITY_KEY_S2_ACCESS), "");
  }

  /*
   * Adding an 11th node must fail.
   * Notice that the node ID is just "i" because the for loop increase it before stopping.
   */
  TEST_ASSERT_FALSE_MESSAGE(ZAF_ADS_SetNodeHighestSecurityClass(handle, i, SECURITY_KEY_S2_ACCESS), "");
}

void test_delete_non_existing(void)
{
  const uint8_t ADS_ELEMENTS  = 1;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wvla"  
  ZAF_ADS_STORAGE(ADSObject, ADS_ELEMENTS);
#pragma GCC diagnostic pop
  ZAF_ADS_Handle_t handle;

  handle = ZAF_ADS_Init(&ADSObject, ADS_ELEMENTS);

  TEST_ASSERT_FALSE_MESSAGE(ZAF_ADS_Delete(handle, 1), "");
}

void test_supported_CCs(void)
{
  const uint8_t ADS_ELEMENTS  = 1;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wvla"  
  ZAF_ADS_STORAGE(ADSObject, ADS_ELEMENTS);
#pragma GCC diagnostic pop
  ZAF_ADS_Handle_t handle;

  handle = ZAF_ADS_Init(&ADSObject, ADS_ELEMENTS);

  const uint8_t NODE_ID = 3;

  e_ZAF_ADS_CC_Capability_t CapabilityResult;

  uint8_t i;
  for (i = 0; i < ZAF_ADS_MAX_CC; i++)
  {
    CapabilityResult = ZAF_ADS_IsNodeSupportingCC(handle, NODE_ID, i);
    TEST_ASSERT_TRUE_MESSAGE(CC_CAPABILITY_NOT_DISCOVERED == CapabilityResult, "");

    TEST_ASSERT_TRUE_MESSAGE(ZAF_ADS_SetNodeIsSupportingCC(handle, NODE_ID, i, CC_CAPABILITY_SUPPORTED), "");

    CapabilityResult = ZAF_ADS_IsNodeSupportingCC(handle, NODE_ID, i);
    TEST_ASSERT_TRUE_MESSAGE(CC_CAPABILITY_SUPPORTED == CapabilityResult, "");
  }

  // Check invalid command class
  CapabilityResult = ZAF_ADS_IsNodeSupportingCC(handle, NODE_ID, ZAF_ADS_MAX_CC);
  TEST_ASSERT_TRUE_MESSAGE(CC_CAPABILITY_NOT_SUPPORTED == CapabilityResult, "");

  TEST_ASSERT_FALSE_MESSAGE(ZAF_ADS_SetNodeIsSupportingCC(handle, NODE_ID, ZAF_ADS_MAX_CC, CC_CAPABILITY_SUPPORTED), "");
}

void test_supported_CCs_too_many(void)
{
  const uint8_t ADS_ELEMENTS  = 1;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wvla"  
  ZAF_ADS_STORAGE(ADSObject, ADS_ELEMENTS);
#pragma GCC diagnostic pop
  ZAF_ADS_Handle_t handle;

  handle = ZAF_ADS_Init(&ADSObject, ADS_ELEMENTS);

  const uint8_t NODE_ID = 3;

  TEST_ASSERT_TRUE_MESSAGE(ZAF_ADS_SetNodeIsSupportingCC(handle, NODE_ID, ZAF_ADS_CC_SUPERVISION, CC_CAPABILITY_SUPPORTED), "");

  TEST_ASSERT_FALSE_MESSAGE(ZAF_ADS_SetNodeIsSupportingCC(handle, NODE_ID + 1, ZAF_ADS_CC_SUPERVISION, CC_CAPABILITY_SUPPORTED), "");
}
