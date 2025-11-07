/**
 * @file test_association_plus.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include <association_plus.h>
#include <unity.h>
#include <mock_control.h>
#include <ZAF_file_ids.h>
#include <string.h>
#include <SizeOf.h>
#include <zpal_nvm.h>
#include <endpoints_groups_associations_helper.h>
#include <cc_agi_config_api_mock.h>
#include "CC_AssociationGroupInfo_mock.h"
#include "misc.h"

#define DISABLE_DEBUG_PRINT

void setUpSuite(void) {
  CC_AGI_groupCount_handler_IgnoreAndReturn(CC_ASSOCIATION_MAX_GROUPS_PER_ENDPOINT);
}

void tearDownSuite(void) {

}

void setUp(void) {

}

void tearDown(void) {

}

typedef struct
{
  node_id_t id;
  uint8_t   endpoint;
  bool      endpointDestination;
} node_t;

void PrintNodes(node_t *pNodes, uint8_t nodeCount)
{
#if !defined(DISABLE_DEBUG_PRINT)
  printf("\n --> Adding nodes:");
  for (uint8_t i = 0; i < nodeCount; i++)
  {
    printf("\n %u: Node (nodeID.endpoint): %u.%u (bit-addressed: %u)",
        i, pNodes[i].id, pNodes[i].endpoint, pNodes[i].endpointDestination);
  }
  printf("\n");
#endif
}

/**
 * Sets the ID of a given node.
 * @param pNode Node object
 * @param id Node ID
 */
static void NodeSetId(destination_info_t * pNode, uint8_t id)
{
  pNode->node.nodeId = id;
}

/**
 * Sets the endpoint of a given node.
 * @param pNode Node object
 * @param endpoint Node endpoint
 */
static void NodeSetEndpoint(destination_info_t * pNode, uint8_t endpoint)
{
  pNode->node.endpoint = endpoint;
}

/**
 * Enable bit addressing for a given node.
 * @param pNode Node object
 * @param enable true for enabling, false for disabling
 */
static void NodeEnableBitAddress(destination_info_t * pNode, bool enable)
{
  pNode->node.BitAddress = (true == enable) ? 1 : 0;
}


static void AddNodes(uint8_t endpoint, uint8_t groupId, node_t *pNodes, uint8_t nodeCount)
{
  PrintNodes(pNodes, nodeCount);

  for (uint8_t i = 0; i < nodeCount; i++)
  {
    destination_info_t node;
    NodeSetId(&node, pNodes[i].id);
    NodeSetEndpoint(&node, pNodes[i].endpoint);
    NodeEnableBitAddress(&node, pNodes[i].endpointDestination);
    AssociationAddNode(groupId, endpoint, &node.node, node.node.BitAddress == 1);
  }
}


void test_CC_Association_Init(void)
{
  mock_t* pMock = NULL;
  mock_calls_clear();

  size_t fileSize = ZAF_FILE_SIZE_ASSOCIATIONINFO;
  mock_call_expect(TO_STR(ZAF_nvm_get_object_size), &pMock);
  pMock->expect_arg[0].v     = ZAF_FILE_ID_ASSOCIATIONINFO;
  pMock->compare_rule_arg[1] = COMPARE_NOT_NULL;
  pMock->output_arg[1].p     = &fileSize;
  pMock->return_code.v       = ZPAL_STATUS_OK;


  SAssociationInfo legacyDefaultFile;
  SAssociationInfo updatedDefaultFile;
  memset(&legacyDefaultFile, 0x00, ZAF_FILE_SIZE_ASSOCIATIONINFO);
  memset(&updatedDefaultFile, 0x00, ZAF_FILE_SIZE_ASSOCIATIONINFO);

  //set unused nodeIds to 0xFF in updatedDefaultFile
  for (uint8_t endpoint = 0; endpoint < (ZAF_CONFIG_NUMBER_OF_END_POINTS + 1); endpoint++)
  {
    for (uint8_t group = 0; group < CC_ASSOCIATION_MAX_GROUPS_PER_ENDPOINT; group++)
    {
      for (uint8_t node = 0; node < CC_ASSOCIATION_MAX_NODES_IN_GROUP; node++)
      {
        updatedDefaultFile.Groups[endpoint][group].subGrp[node].node.nodeId = 0xFF; //FREE_VALUE
      }
    }
  }

  //read a file with legacy FREE_VALUE 0x00
  mock_call_expect(TO_STR(ZAF_nvm_read), &pMock);
  pMock->expect_arg[0].v     = ZAF_FILE_ID_ASSOCIATIONINFO;
  pMock->compare_rule_arg[1] = COMPARE_NOT_NULL;
  pMock->output_arg[1].p     = &legacyDefaultFile;
  pMock->expect_arg[2].v     = ZAF_FILE_SIZE_ASSOCIATIONINFO;
  pMock->return_code.v       = ZPAL_STATUS_OK;

  //verify that a file is written with current FREE_VALUE 0xFF
  mock_call_expect(TO_STR(ZAF_nvm_write), &pMock);
  pMock->expect_arg[0].v     = ZAF_FILE_ID_ASSOCIATIONINFO;
  pMock->expect_arg[1].p     = &updatedDefaultFile;
  pMock->expect_arg[2].v     = ZAF_FILE_SIZE_ASSOCIATIONINFO;
  pMock->return_code.v       = ZPAL_STATUS_OK;

  mock_call_expect(TO_STR(ZAF_nvm_read), &pMock);
  pMock->expect_arg[0].v     = ZAF_FILE_ID_ASSOCIATIONINFO;
  pMock->compare_rule_arg[1] = COMPARE_NOT_NULL;
  pMock->output_arg[1].p     = &updatedDefaultFile;
  pMock->expect_arg[2].v     = ZAF_FILE_SIZE_ASSOCIATIONINFO;
  pMock->return_code.v       = ZPAL_STATUS_OK;


  CC_Association_Init();

  mock_calls_verify();
}

void test_CC_CC_CC_Association_Reset(void)
{
  mock_t* pMock = NULL;
  mock_calls_clear();

  SAssociationInfo updatedDefaultFile;
  memset(&updatedDefaultFile, 0x00, ZAF_FILE_SIZE_ASSOCIATIONINFO);

  //set unused nodeIds to 0xFF in updatedDefaultFile
    for (uint8_t endpoint = 0; endpoint < (ZAF_CONFIG_NUMBER_OF_END_POINTS + 1); endpoint++)
    {
      for (uint8_t group = 0; group < CC_ASSOCIATION_MAX_GROUPS_PER_ENDPOINT; group++)
      {
        for (uint8_t node = 0; node < CC_ASSOCIATION_MAX_NODES_IN_GROUP; node++)
        {
          updatedDefaultFile.Groups[endpoint][group].subGrp[node].node.nodeId = 0xFF; //FREE_VALUE
        }
      }
    }

  //verify that a default file is written
  mock_call_expect(TO_STR(ZAF_nvm_write), &pMock);
  pMock->expect_arg[0].v     = ZAF_FILE_ID_ASSOCIATIONINFO;
  pMock->expect_arg[1].p     = &updatedDefaultFile;
  pMock->expect_arg[2].v     = ZAF_FILE_SIZE_ASSOCIATIONINFO;
  pMock->return_code.v       = ZPAL_STATUS_OK;

  mock_call_expect(TO_STR(ZAF_nvm_read), &pMock);
  pMock->expect_arg[0].v     = ZAF_FILE_ID_ASSOCIATIONINFO;
  pMock->compare_rule_arg[1] = COMPARE_NOT_NULL;
  pMock->expect_arg[2].v     = ZAF_FILE_SIZE_ASSOCIATIONINFO;
  pMock->return_code.v       = ZPAL_STATUS_OK;

  CC_Association_Reset();

  mock_calls_verify();
}


void test_handleAssociationGetnodeList_invalid_endpoint(void)
{
  const uint8_t GROUP_ID = 1;
  const uint8_t ENDPOINT = ZAF_CONFIG_NUMBER_OF_END_POINTS + 2; // +2 because root device counts as an endpoint.
  destination_info_t * pList;
  uint8_t list_length;
  NODE_LIST_STATUS status = handleAssociationGetnodeList(GROUP_ID,
                                                           ENDPOINT,
                                                           &pList,
                                                           &list_length);

  TEST_ASSERT_EQUAL_UINT32_MESSAGE(NODE_LIST_STATUS_ERR_ENDPOINT_OUT_OF_RANGE, status, "Node list status does not match :(");
}

void test_handleAssociationGetnodeList_list_null(void)
{
  const uint8_t GROUP_ID = 1;
  const uint8_t ENDPOINT = 0; // Root device
  uint8_t list_length;

  NODE_LIST_STATUS status = handleAssociationGetnodeList(GROUP_ID,
                                                           ENDPOINT,
                                                           NULL,
                                                           &list_length);

  TEST_ASSERT_EQUAL_UINT32_MESSAGE(NODE_LIST_STATUS_ERROR_LIST, status, "Node list status does not match :(");
}

void test_handleAssociationGetnodeList_list_length_null(void)
{
  const uint8_t GROUP_ID = 1;
  const uint8_t ENDPOINT = 0; // Root device
  destination_info_t * pList;
  NODE_LIST_STATUS status = handleAssociationGetnodeList(GROUP_ID,
                                                           ENDPOINT,
                                                           &pList,
                                                           NULL);

  TEST_ASSERT_EQUAL_UINT32_MESSAGE(NODE_LIST_STATUS_ERROR_LIST, status, "Node list status does not match :(");
}

void test_handleAssociationGetnodeList_group_id_zero(void)
{
  const uint8_t GROUP_ID = 0;
  const uint8_t ENDPOINT = 0; // Root device
  destination_info_t * pList;
  uint8_t list_length;
  NODE_LIST_STATUS status = handleAssociationGetnodeList(GROUP_ID,
                                                           ENDPOINT,
                                                           &pList,
                                                           &list_length);

  TEST_ASSERT_EQUAL_UINT32_MESSAGE(NODE_LIST_STATUS_ERR_GROUP_NBR_NOT_LEGAL, status, "Node list status does not match :(");
}

void test_handleAssociationGetnodeList_group_id_too_large(void)
{
  const uint8_t GROUP_ID = CC_ASSOCIATION_MAX_GROUPS_PER_ENDPOINT + 1;
  const uint8_t ENDPOINT = 0; // Root device
  destination_info_t * pList;
  uint8_t list_length;

  NODE_LIST_STATUS status = handleAssociationGetnodeList(GROUP_ID,
                                                           ENDPOINT,
                                                           &pList,
                                                           &list_length);

  TEST_ASSERT_EQUAL_UINT32_MESSAGE(NODE_LIST_STATUS_ERR_GROUP_NBR_NOT_LEGAL, status, "Node list status does not match :(");
}


void FetchAndPrintNodes(uint8_t endpoint,
                        uint8_t groupId,
                        destination_info_t ** ppList,
                        uint8_t* pListLen)
{
  destination_info_t * pList;
  uint8_t list_length;

  handleAssociationGetnodeList(groupId, endpoint, &pList, &list_length);

  if (NULL != ppList) *ppList = pList;
  if (NULL != pListLen) *pListLen = list_length;
#if !defined(DISABLE_DEBUG_PRINT)
  printf("\nAdded nodes: ----------------------------------------------------------------------------");
  for (uint8_t i = 0; i < CC_ASSOCIATION_MAX_NODES_IN_GROUP; i++)
  {
    destination_info_t * pNode = (pList + i);

    if (pNode->node.nodeId != FREE_VALUE)  // Don't print Nodes that are FREE!
    {
      printf("\n%u: Node (nodeID.endpoint): %u.%u", i, pNode->node.nodeId, pNode->node.endpoint);
    }
  }
  printf("\n");
#endif
}

void test_handleAssociationGetnodeList_max_nodes(void)
{
//  TEST_IGNORE();

  mock_t * pMock;
  mock_calls_clear();

  const uint8_t GROUP_ID = 1;
  const uint8_t ENDPOINT = 0;

  const ccc_pair_t CCC_PAIRS[] = {
                                  {
                                   .cmdClass = COMMAND_CLASS_BASIC,
                                   .cmd = BASIC_SET
                                  }
  };

  const cc_agi_group_t agiTableRootDeviceGroups[] = {
                                          {{0xAA, 0xBB}, sizeof_array(CCC_PAIRS), CCC_PAIRS, "Group 2"},
                                          {{0xCC, 0xDD}, sizeof_array(CCC_PAIRS), CCC_PAIRS, "Group 3"}
  };

  const cc_agi_config_t customAgiTable[] = {
                                            {
                                             .groups = agiTableRootDeviceGroups,
                                             .group_count = sizeof_array(agiTableRootDeviceGroups)
                                            }
  };
  cc_agi_get_config_IgnoreAndReturn(customAgiTable);

  mock_call_use_as_stub(TO_STR(ZAF_GetInclusionMode));
  mock_call_use_as_stub(TO_STR(ZAF_GetSecurityKeys));
  mock_call_use_as_stub(TO_STR(GetHighestSecureLevel));

  mock_call_use_as_stub(TO_STR(ZAF_nvm_write));

  mock_call_expect(TO_STR(ZAF_nvm_read), &pMock);
  pMock->expect_arg[0].v = ZAF_FILE_ID_ASSOCIATIONINFO;
  pMock->compare_rule_arg[1] = COMPARE_NOT_NULL;
  pMock->expect_arg[2].v = 10395; // 3 bytes * (4 endpoints + root device) * 3 groups * 231 nodes

  //IMPORTANT: provide an SAssociationInfo struct full of zeros that can be read by AssociationInit()
  SAssociationInfo tAssociationInfo;
  memset(&tAssociationInfo, 0, sizeof(SAssociationInfo));

  for (uint8_t endpoint = 0; endpoint < ZAF_CONFIG_NUMBER_OF_END_POINTS; endpoint++)
  {
    for (uint8_t group = 0; group < CC_ASSOCIATION_MAX_GROUPS_PER_ENDPOINT; group++)
    {
      for (uint8_t node = 0; node < CC_ASSOCIATION_MAX_NODES_IN_GROUP; node++)
      {
        tAssociationInfo.Groups[endpoint][group].subGrp[node].node.nodeId = 0xFF; // Free
      }
    }
  }

  pMock->output_arg[2].p = &tAssociationInfo;

  mock_call_use_as_stub(TO_STR(ZAF_nvm_get_object_size));

  CC_Association_Init();

  MULTICHAN_DEST_NODE_ID nodes[CC_ASSOCIATION_MAX_NODES_IN_GROUP];

  for (int i = 0; i < CC_ASSOCIATION_MAX_NODES_IN_GROUP; i++)
  {
    nodes[i].nodeId = i + 1;
    nodes[i].endpoint = 0;
    bool add_status = AssociationAddNode(GROUP_ID, ENDPOINT, &nodes[i], true);
    char str[100];
    sprintf(str, "Cannot add node %u (index %u)", nodes[i].nodeId, i);
    TEST_ASSERT_TRUE_MESSAGE(add_status, str);
  }

  FetchAndPrintNodes(ENDPOINT, GROUP_ID, NULL, NULL);

  destination_info_t * pList;
  uint8_t list_length;

  NODE_LIST_STATUS status = handleAssociationGetnodeList(GROUP_ID,
                                                         ENDPOINT,
                                                         &pList,
                                                         &list_length);

  TEST_ASSERT_EQUAL_UINT32_MESSAGE(NODE_LIST_STATUS_SUCCESS, status, "Node list status does not match :(");
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(CC_ASSOCIATION_MAX_NODES_IN_GROUP, list_length, "List length differs");

  for (uint8_t i = 0; i < list_length; i++)
  {
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(nodes[i].nodeId, (pList + i)->node.nodeId, "Node ID does not match");

    //printf("Node %d ID: %d\n", i, (pList + i)->node.nodeId);
  }

  //Test that singlecast node count can be updated at max node count
  AssociationGetDestinationInit(pList);
  uint32_t node_count = AssociationGetSinglecastNodeCount();
  TEST_ASSERT_EQUAL_UINT32_MESSAGE(CC_ASSOCIATION_MAX_NODES_IN_GROUP, node_count, "Node count does not match");

  mock_calls_verify();
}

/*
 * Verifies that when having nodes
 * 3.1
 * 3.2
 * 4.3
 * 4.4
 * 5
 * AssociationGetBitAdressingDestination() will return true once and output two bit addressing
 * destinations.
 */
void test_AssociationGetBitAdressingDestination(void)
{
//  TEST_IGNORE();

  mock_t * pMock;
  mock_calls_clear();

  const uint8_t ENDPOINT = 0;
  const uint8_t GROUP_ID = 1;

  mock_call_use_as_stub(TO_STR(ZAF_GetInclusionMode));
  mock_call_use_as_stub(TO_STR(ZAF_GetSecurityKeys));
  mock_call_use_as_stub(TO_STR(GetHighestSecureLevel));

  mock_call_use_as_stub(TO_STR(ZAF_nvm_write));

  mock_call_expect(TO_STR(ZAF_nvm_read), &pMock);
  pMock->expect_arg[0].v = ZAF_FILE_ID_ASSOCIATIONINFO;
  pMock->compare_rule_arg[1] = COMPARE_NOT_NULL;
  pMock->expect_arg[2].v = 10395; // 3 bytes * (4 endpoints + root device) * 3 groups * 231 nodes

  cc_agi_get_config_IgnoreAndReturn(myAgi);

  //IMPORTANT: provide an SAssociationInfo struct full of zeros that can be read by AssociationInit()
  SAssociationInfo tAssociationInfo;
  memset(&tAssociationInfo, 0, sizeof(SAssociationInfo));

  for (uint8_t endpoint = 0; endpoint < ZAF_CONFIG_NUMBER_OF_END_POINTS; endpoint++)
  {
    for (uint8_t group = 0; group < CC_ASSOCIATION_MAX_GROUPS_PER_ENDPOINT; group++)
    {
      for (uint8_t node = 0; node < CC_ASSOCIATION_MAX_NODES_IN_GROUP; node++)
      {
        tAssociationInfo.Groups[endpoint][group].subGrp[node].node.nodeId = 0xFF; // Free
      }
    }
  }

  pMock->output_arg[2].p = &tAssociationInfo;

  mock_call_use_as_stub(TO_STR(ZAF_nvm_get_object_size));
  CC_Association_Init();

  node_t nodes[] = {
                    {3, 1, true},
                    {3, 2, true},
                    {4, 3, true},
                    {4, 4, true},
                    {5, 0, false}
  };

  AddNodes(ENDPOINT, GROUP_ID, nodes, sizeof_array(nodes));

  destination_info_t * pList;
  uint8_t list_length;

  FetchAndPrintNodes( ENDPOINT, GROUP_ID, &pList, &list_length);

  destination_info_t * const P_LIST = pList;
  const uint8_t LIST_LENGTH = sizeof_array(nodes);

  destination_info_t nodeOutput;
  bool moreNodes;

  // Repeat the sequence a number of times to verify that the function reset itself after each run.
  for (uint8_t i = 0; i < 10; i++)
  {
    /*
     * Reset the input list for each run.
     * This is required when the function is used in Multicast e.g.
     */
    pList = P_LIST;
    list_length = LIST_LENGTH;

    /*
     * Invoke the first time and expect true as return value and the first two associations as one
     * bit addressing node.
     */
    moreNodes = AssociationGetBitAdressingDestination(&pList, &list_length, &nodeOutput);

    TEST_ASSERT_TRUE_MESSAGE(moreNodes, "No more nodes! :(");

    TEST_ASSERT_NOT_NULL_MESSAGE(pList, "pList has zero nodes :(");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(4, pList->node.nodeId, "pList points to the wrong node");

    TEST_ASSERT_EQUAL_UINT8_MESSAGE(3, list_length, "Remaining node count wrong (first)");

    TEST_ASSERT_EQUAL_UINT8_MESSAGE(3, nodeOutput.node.nodeId, "Incorrect Node ID (first)");
    // Endpoint 1 & 2 as bit addressing:
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(0x03, nodeOutput.node.endpoint, "Incorrect bit addressing endpoints (first)");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(1, nodeOutput.node.BitAddress, "Incorrect bit address value (first)");

    /*
     * Invoke the second time and expect false as return value and the third and fourth associations
     * as one bit addressing node.
     */
    moreNodes = AssociationGetBitAdressingDestination(&pList, &list_length, &nodeOutput);

    TEST_ASSERT_FALSE_MESSAGE(moreNodes, "More nodes are left in the list! :(");

    TEST_ASSERT_NOT_NULL_MESSAGE(pList, "pList has zero nodes :(");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(5, pList->node.nodeId, "pList points to the wrong node");

    TEST_ASSERT_EQUAL_UINT8_MESSAGE(1, list_length, "Remaining node count wrong (second)");

    TEST_ASSERT_EQUAL_UINT8_MESSAGE(4, nodeOutput.node.nodeId, "Incorrect Node ID (second)");
    // Endpoint 3 & 4 as bit addressing:
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(0x0C, nodeOutput.node.endpoint, "Incorrect bit addressing endpoints (second)");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(1, nodeOutput.node.BitAddress, "Incorrect bit address value (first)");
  }

  mock_calls_verify();
}

/*
 * Verifies that when having nodes
 * 3
 * 4
 * AssociationGetBitAdressingDestination() will return false and output no bit addressing
 * destination.
 */
void test_AssociationGetBitAdressingDestination_no_endpoints(void)
{
//  TEST_IGNORE();

  mock_t * pMock;
  mock_calls_clear();

  const uint8_t ENDPOINT = 0;
  const uint8_t GROUP_ID = 1;

  mock_call_use_as_stub(TO_STR(ZAF_GetInclusionMode));
  mock_call_use_as_stub(TO_STR(ZAF_GetSecurityKeys));
  mock_call_use_as_stub(TO_STR(GetHighestSecureLevel));

  mock_call_use_as_stub(TO_STR(ZAF_nvm_write));

  mock_call_expect(TO_STR(ZAF_nvm_read), &pMock);
  pMock->expect_arg[0].v = ZAF_FILE_ID_ASSOCIATIONINFO;
  pMock->compare_rule_arg[1] = COMPARE_NOT_NULL;
  pMock->expect_arg[2].v = 10395; // 3 bytes * (4 endpoints + root device) * 3 groups * 231 nodes

  //IMPORTANT: provide an SAssociationInfo struct full of zeros that can be read by AssociationInit()
  SAssociationInfo tAssociationInfo;
  memset(&tAssociationInfo, 0, sizeof(SAssociationInfo));

  for (uint8_t endpoint = 0; endpoint < ZAF_CONFIG_NUMBER_OF_END_POINTS; endpoint++)
  {
    for (uint8_t group = 0; group < CC_ASSOCIATION_MAX_GROUPS_PER_ENDPOINT; group++)
    {
      for (uint8_t node = 0; node < CC_ASSOCIATION_MAX_NODES_IN_GROUP; node++)
      {
        tAssociationInfo.Groups[endpoint][group].subGrp[node].node.nodeId = 0xFF; // Free
      }
    }
  }

  pMock->output_arg[2].p = &tAssociationInfo;

  mock_call_use_as_stub(TO_STR(ZAF_nvm_get_object_size));
  CC_Association_Init();

  node_t nodes[] = {
                    {3, 0, false},
                    {4, 0, false}
  };

  AddNodes(ENDPOINT, GROUP_ID, nodes, sizeof_array(nodes));

  destination_info_t * pList;
  uint8_t list_length;

  FetchAndPrintNodes( ENDPOINT, GROUP_ID, &pList, &list_length);

  destination_info_t * const P_LIST = pList;
  const uint8_t LIST_LENGTH = sizeof_array(nodes);

  destination_info_t nodeOutput;
  bool moreNodes;

  // Repeat the sequence a number of times to verify that the function reset itself after each run.
  for (uint8_t i = 0; i < 10; i++)
  {
    /*
     * Reset the input list for each run.
     * This is required when the function is used in Multicast e.g.
     */
    pList = P_LIST;
    list_length = LIST_LENGTH;

    /*
     * Invoke the first time and expect true as return value and the first two associations as one
     * bit addressing node.
     */
    moreNodes = AssociationGetBitAdressingDestination(&pList, &list_length, &nodeOutput);

    TEST_ASSERT_FALSE_MESSAGE(moreNodes, "No more nodes! :(");

    TEST_ASSERT_EQUAL_UINT8_MESSAGE(2, list_length, "Remaining node count wrong (first)");

    TEST_ASSERT_EQUAL_UINT8_MESSAGE(0, nodeOutput.node.BitAddress, "Incorrect bit address value (first)");
  }

  mock_calls_verify();
}

/*
 * Verifies that when having nodes
 * 3.1
 * 3.2
 * 4.3
 * 4.4
 * AssociationGetBitAdressingDestination() will return true once and output two bit addressing
 * destinations.
 */
void test_AssociationGetBitAdressingDestination_only_endpoints(void)
{
//  TEST_IGNORE();

  mock_t * pMock;
  mock_calls_clear();

  const uint8_t ENDPOINT = 0;
  const uint8_t GROUP_ID = 1;

  mock_call_use_as_stub(TO_STR(ZAF_GetInclusionMode));
  mock_call_use_as_stub(TO_STR(ZAF_GetSecurityKeys));
  mock_call_use_as_stub(TO_STR(GetHighestSecureLevel));

  mock_call_use_as_stub(TO_STR(ZAF_nvm_write));

  mock_call_expect(TO_STR(ZAF_nvm_read), &pMock);
  pMock->expect_arg[0].v = ZAF_FILE_ID_ASSOCIATIONINFO;
  pMock->compare_rule_arg[1] = COMPARE_NOT_NULL;
  pMock->expect_arg[2].v = 10395; // 3 bytes * (4 endpoints + root device) * 3 groups * 231 nodes

  cc_agi_get_config_IgnoreAndReturn(myAgi);
  //IMPORTANT: provide an SAssociationInfo struct full of zeros that can be read by AssociationInit()
  SAssociationInfo tAssociationInfo;
  memset(&tAssociationInfo, 0, sizeof(SAssociationInfo));

  for (uint8_t endpoint = 0; endpoint < ZAF_CONFIG_NUMBER_OF_END_POINTS; endpoint++)
  {
    for (uint8_t group = 0; group < CC_ASSOCIATION_MAX_GROUPS_PER_ENDPOINT; group++)
    {
      for (uint8_t node = 0; node < CC_ASSOCIATION_MAX_NODES_IN_GROUP; node++)
      {
        tAssociationInfo.Groups[endpoint][group].subGrp[node].node.nodeId = 0xFF; // Free
      }
    }
  }

  pMock->output_arg[2].p = &tAssociationInfo;

  mock_call_use_as_stub(TO_STR(ZAF_nvm_get_object_size));
  CC_Association_Init();

  node_t nodes[] = {
                    {3, 1, true},
                    {3, 2, true},
                    {4, 3, true},
                    {4, 4, true}
  };

  AddNodes(ENDPOINT, GROUP_ID, nodes, sizeof_array(nodes));

  destination_info_t * pList;
  uint8_t list_length;

  FetchAndPrintNodes( ENDPOINT, GROUP_ID, &pList, &list_length);

  destination_info_t * const P_LIST = pList;
  const uint8_t LIST_LENGTH = sizeof_array(nodes);

  destination_info_t nodeOutput;
  bool moreNodes;

  // Repeat the sequence a number of times to verify that the function reset itself after each run.
  for (uint8_t i = 0; i < 10; i++)
  {
    /*
     * Reset the input list for each run.
     * This is required when the function is used in Multicast e.g.
     */
    pList = P_LIST;
    list_length = LIST_LENGTH;

    /*
     * Invoke the first time and expect true as return value and the first two associations as one
     * bit addressing node.
     */
    moreNodes = AssociationGetBitAdressingDestination(&pList, &list_length, &nodeOutput);

    TEST_ASSERT_TRUE_MESSAGE(moreNodes, "No more nodes! :(");

    TEST_ASSERT_NOT_NULL_MESSAGE(pList, "pList has zero nodes :(");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(4, pList->node.nodeId, "pList points to the wrong node");

    TEST_ASSERT_EQUAL_UINT8_MESSAGE(2, list_length, "Remaining node count wrong (first)");

    TEST_ASSERT_EQUAL_UINT8_MESSAGE(3, nodeOutput.node.nodeId, "Incorrect Node ID (first)");
    // Endpoint 1 & 2 as bit addressing:
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(0x03, nodeOutput.node.endpoint, "Incorrect bit addressing endpoints (first)");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(1, nodeOutput.node.BitAddress, "Incorrect bit address value (first)");

    /*
     * Invoke the second time and expect false as return value and the third and fourth associations
     * as one bit addressing node.
     */
    moreNodes = AssociationGetBitAdressingDestination(&pList, &list_length, &nodeOutput);

    TEST_ASSERT_FALSE_MESSAGE(moreNodes, "More nodes are left in the list! :(");

    TEST_ASSERT_NULL_MESSAGE(pList, "pList has more nodes :(");

    TEST_ASSERT_EQUAL_UINT8_MESSAGE(0, list_length, "Remaining node count wrong (second)");

    TEST_ASSERT_EQUAL_UINT8_MESSAGE(4, nodeOutput.node.nodeId, "Incorrect Node ID (second)");
    // Endpoint 3 & 4 as bit addressing:
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(0x0C, nodeOutput.node.endpoint, "Incorrect bit addressing endpoints (second)");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(1, nodeOutput.node.BitAddress, "Incorrect bit address value (first)");
  }

  mock_calls_verify();
}


/*
 * Verifies that when having nodes
 * 40
 * 70.100
 * 70.110
 *
 * TODO change the below
 * AssociationGetBitAdressingDestination() will return true once and output two bit addressing destinations.
 */
void test_AssociationGetBitAdressingDestination_no_endpoints_and_endpoint_mixed(void)
{
//  TEST_IGNORE();
  mock_t * pMock;
  mock_calls_clear();

  const uint8_t ENDPOINT = 0;
  const uint8_t GROUP_ID = 2;

  mock_call_use_as_stub(TO_STR(ZAF_GetInclusionMode));
  mock_call_use_as_stub(TO_STR(ZAF_GetSecurityKeys));
  mock_call_use_as_stub(TO_STR(GetHighestSecureLevel));

  mock_call_use_as_stub(TO_STR(ZAF_nvm_write));
  
  cc_agi_get_config_IgnoreAndReturn(myAgi);


  mock_call_expect(TO_STR(ZAF_nvm_read), &pMock);
  pMock->expect_arg[0].v = ZAF_FILE_ID_ASSOCIATIONINFO;
  pMock->compare_rule_arg[1] = COMPARE_NOT_NULL;
  pMock->expect_arg[2].v = 10395; // 3 bytes * (4 endpoints + root device) * 3 groups * 231 nodes

  //IMPORTANT: provide an SAssociationInfo struct full of zeros that can be read by AssociationInit()
  SAssociationInfo tAssociationInfo;
  memset(&tAssociationInfo, 0, sizeof(SAssociationInfo));

  for (uint8_t endpoint = 0; endpoint < ZAF_CONFIG_NUMBER_OF_END_POINTS; endpoint++)
  {
    for (uint8_t group = 0; group < CC_ASSOCIATION_MAX_GROUPS_PER_ENDPOINT; group++)
    {
      for (uint8_t node = 0; node < CC_ASSOCIATION_MAX_NODES_IN_GROUP; node++)
      {
        tAssociationInfo.Groups[endpoint][group].subGrp[node].node.nodeId = 0xFF; // Free
      }
    }
  }

  pMock->output_arg[2].p = &tAssociationInfo;

  mock_call_use_as_stub(TO_STR(ZAF_nvm_get_object_size));
  CC_Association_Init();

  /* This array is sorted as needed by AssociationAddNode() and is retrieved in the sorted way by
   * AssociationGetBitAdressingDestination() and its associated functions. */
  node_t nodes[] = {
                    {35, 2, true},
                    {40, 0, false},
                    {35, 1, true},
                    {36, 7, true},
                    {70, 100, true},
                    {70, 110, true},
                    {36, 3, true},
  };

  AddNodes(ENDPOINT, GROUP_ID, nodes, sizeof_array(nodes));

  destination_info_t * pList;
  uint8_t list_length;

  FetchAndPrintNodes( ENDPOINT, GROUP_ID, &pList, &list_length);

  destination_info_t * const P_LIST = pList;
  const uint8_t LIST_LENGTH = sizeof_array(nodes);

  destination_info_t nodeOutput;
  bool moreNodes;

  // Repeat the sequence a number of times to verify that the function reset itself after each run.
  for (uint8_t i = 0; i < 10; i++)
  {
    /*
     * Reset the input list for each run.
     * This is required when the function is used in Multicast e.g.
     */
    pList = P_LIST;
    list_length = LIST_LENGTH;

    /*
     * Invoke the first time and expect true as return value and the first two associations as one
     * bit addressing node.
     */
    moreNodes = AssociationGetBitAdressingDestination(&pList, &list_length, &nodeOutput);
#if !defined(DISABLE_DEBUG_PRINT)
    printf("\n");
#endif
    TEST_ASSERT_TRUE_MESSAGE(moreNodes, "No more nodes! :(");

    // Check remaining list:

    TEST_ASSERT_NOT_NULL_MESSAGE(pList, "pList has zero nodes :(");
    // Using the addition of the nodeID and endpoint determines precisely the node it is being pointed to.
    TEST_ASSERT_EQUAL_UINT16_MESSAGE(39, pList->node.nodeId + pList->node.endpoint, "pList points to the wrong node");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(5, list_length, "Remaining node count wrong (first)");

    // Check output node:

    // We cannot add the nodeID and endpoint here, because the endpoint-field is used for bit-addressing (are flags).
    TEST_ASSERT_EQUAL_UINT16_MESSAGE(35, nodeOutput.node.nodeId, "Incorrect Node ID (first)");
    // Endpoint 1 & 2 as bit addressing:
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(0x01 | 0x02, nodeOutput.node.endpoint, "Incorrect bit addressing endpoints (first)");
    // Is the bit-addressing flag set so that the TX can be made with multi-channel?
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(1, nodeOutput.node.BitAddress, "Incorrect bit address value (first)");

    /*
     * Invoke the second time and assess the output.
     */
    moreNodes = AssociationGetBitAdressingDestination(&pList, &list_length, &nodeOutput);
#if !defined(DISABLE_DEBUG_PRINT)
    printf("\n");
#endif

    TEST_ASSERT_FALSE_MESSAGE(moreNodes, "More nodes are left in the list! :(");

    // Check remaining list:

    TEST_ASSERT_NOT_NULL_MESSAGE(pList, "pList has zero nodes :(");
    // Using the addition of the nodeID and endpoint determines precisely the node it is being pointed to.
    TEST_ASSERT_EQUAL_UINT16_MESSAGE(40, pList->node.nodeId + pList->node.endpoint, "pList points to the wrong node");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(3, list_length, "Remaining node count wrong (second)");

    // Check output node:

    // We cannot add the nodeID and endpoint here, because the endpoint-field is used for bit-addressing (are flags).
    TEST_ASSERT_EQUAL_UINT16_MESSAGE(36, nodeOutput.node.nodeId, "Incorrect Node ID (second)");
    // Endpoint 3 & 4 as bit addressing:
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(0x40 | 0x04, nodeOutput.node.endpoint, "Incorrect bit addressing endpoints (second)");
    // Is the bit-addressing flag set so that the TX can be made with multi-channel?
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(1, nodeOutput.node.BitAddress, "Incorrect bit address value (second)");

    /*
     * Invoke the third time and assess the output.
     */
    moreNodes = AssociationGetBitAdressingDestination(&pList, &list_length, &nodeOutput);
#if !defined(DISABLE_DEBUG_PRINT)
    printf("\n");
#endif

    TEST_ASSERT_FALSE_MESSAGE(moreNodes, "More nodes are left in the list! :(");

    // Check remaining list:

    TEST_ASSERT_NOT_NULL_MESSAGE(pList, "pList has zero nodes :(");
    // Using the addition of the nodeID and endpoint determines precisely the node it is being pointed to.
    TEST_ASSERT_EQUAL_UINT16_MESSAGE(40, pList->node.nodeId + pList->node.endpoint, "pList points to the wrong node");

    // The list length is not changing! (pList->node.nodeId keeps pointing at nodeID = 40)
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(3, list_length, "Remaining node count wrong (third)");

    // Check output node:

    // We cannot add the nodeID and endpoint here, because the endpoint-field is used for bit-addressing (are flags).
    TEST_ASSERT_EQUAL_UINT16_MESSAGE(36, nodeOutput.node.nodeId, "Incorrect Node ID (third)");
    // Endpoint 3 & 4 as bit addressing:
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(0x40 | 0x04, nodeOutput.node.endpoint, "Incorrect bit addressing endpoints (third)");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(0, nodeOutput.node.BitAddress, "Incorrect bit address value (third)");

    /*
     * Invoke the forth time and assess the output.
     */
    moreNodes = AssociationGetBitAdressingDestination(&pList, &list_length, &nodeOutput);
#if !defined(DISABLE_DEBUG_PRINT)
    printf("\n");
#endif
    TEST_ASSERT_FALSE_MESSAGE(moreNodes, "More nodes are left in the list! :(");
    TEST_ASSERT_NOT_NULL_MESSAGE(pList, "pList has zero nodes :(");

    // The list length is not changing! (pList->node.nodeId keeps pointing at nodeID = 40)
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(3, list_length, "Remaining node count wrong (forth)");

    // We cannot add the nodeID and endpoint here, because the endpoint-field is used for bit-addressing (are flags).
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(36, nodeOutput.node.nodeId, "Incorrect Node ID (forth)");
    // Endpoint 3 & 4 as bit addressing:
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(0x40 | 0x04, nodeOutput.node.endpoint, "Incorrect bit addressing endpoints (forth)");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(0, nodeOutput.node.BitAddress, "Incorrect bit address value (forth)");
  }

  mock_calls_verify();
}


/*
 * Verifies that a list of associations all being endpoint destinations to a unique node ID, will
 * output no bit addressing destinations.
 */
void test_AssociationGetBitAdressingDestination_single_endpoint_destinations_only(void)
{
//  TEST_IGNORE();

  mock_t * pMock;
  mock_calls_clear();

  const uint8_t ENDPOINT = 0;
  const uint8_t GROUP_ID = 1;

  mock_call_use_as_stub(TO_STR(ZAF_GetInclusionMode));
  mock_call_use_as_stub(TO_STR(ZAF_GetSecurityKeys));
  mock_call_use_as_stub(TO_STR(GetHighestSecureLevel));

  mock_call_use_as_stub(TO_STR(ZAF_nvm_write));

  mock_call_expect(TO_STR(ZAF_nvm_read), &pMock);
  pMock->expect_arg[0].v = ZAF_FILE_ID_ASSOCIATIONINFO;
  pMock->compare_rule_arg[1] = COMPARE_NOT_NULL;
  pMock->expect_arg[2].v = 10395; // 3 bytes * (4 endpoints + root device) * 3 groups * 231 nodes

  //IMPORTANT: provide an SAssociationInfo struct full of zeros that can be read by AssociationInit()
  SAssociationInfo tAssociationInfo;
  memset(&tAssociationInfo, 0, sizeof(SAssociationInfo));

  for (uint8_t endpoint = 0; endpoint < ZAF_CONFIG_NUMBER_OF_END_POINTS; endpoint++)
  {
    for (uint8_t group = 0; group < CC_ASSOCIATION_MAX_GROUPS_PER_ENDPOINT; group++)
    {
      for (uint8_t node = 0; node < CC_ASSOCIATION_MAX_NODES_IN_GROUP; node++)
      {
        tAssociationInfo.Groups[endpoint][group].subGrp[node].node.nodeId = 0xFF; // Free
      }
    }
  }

  pMock->output_arg[2].p = &tAssociationInfo;

  mock_call_use_as_stub(TO_STR(ZAF_nvm_get_object_size));
  CC_Association_Init();

  node_t nodes[] = {
                    {3, 1, true},
                    {4, 2, true},
                    {5, 3, true}
  };

  AddNodes(ENDPOINT, GROUP_ID, nodes, sizeof_array(nodes));

  destination_info_t * pList;
  uint8_t list_length;

  FetchAndPrintNodes( ENDPOINT, GROUP_ID, &pList, &list_length);

  destination_info_t * const P_LIST = pList;
  const uint8_t LIST_LENGTH = sizeof_array(nodes);

  destination_info_t nodeOutput;
  bool moreNodes;

  // Repeat the sequence a number of times to verify that the function reset itself after each run.
  for (uint8_t i = 0; i < 10; i++)
  {
    /*
     * Reset the input list for each run.
     * This is required when the function is used in Multicast e.g.
     */
    pList = P_LIST;
    list_length = LIST_LENGTH;

    /*
     * Invoke the first time.
     */
    moreNodes = AssociationGetBitAdressingDestination(&pList, &list_length, &nodeOutput);

    TEST_ASSERT_TRUE_MESSAGE(moreNodes, "No more nodes! :(");
    TEST_ASSERT_NOT_NULL_MESSAGE(pList, "pList has zero nodes :(");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(nodes[1].id, pList->node.nodeId, "pList points to the wrong node");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(2, list_length, "Remaining node count wrong (first)");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(0, nodeOutput.node.BitAddress, "Incorrect bit address value (first)");

    /*
     * Invoke the second time.
     */
    moreNodes = AssociationGetBitAdressingDestination(&pList, &list_length, &nodeOutput);

    TEST_ASSERT_TRUE_MESSAGE(moreNodes, "No more nodes! :(");
    TEST_ASSERT_NOT_NULL_MESSAGE(pList, "pList has zero nodes :(");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(nodes[2].id, pList->node.nodeId, "pList points to the wrong node");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(1, list_length, "Remaining node count wrong (second)");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(0, nodeOutput.node.BitAddress, "Incorrect bit address value (first)");

    /*
     * Invoke the third time.
     */
    moreNodes = AssociationGetBitAdressingDestination(&pList, &list_length, &nodeOutput);

    TEST_ASSERT_FALSE_MESSAGE(moreNodes, "More nodes are left in the list! :(");
    TEST_ASSERT_NULL_MESSAGE(pList, "pList has more nodes :(");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(0, list_length, "Remaining node count wrong (second)");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(0, nodeOutput.node.BitAddress, "Incorrect bit address value (first)");
  }

  mock_calls_verify();
}

/*
 * Given that associations are sorted with endpoint destinations first and then root device
 * destinations, this test verifies that when having nodes
 * 2.1  <- singlecast destination
 * 3.1
 * 3.2
 * 4.0  <- singlecast destination
 * AssociationGetBitAdressingDestination() will
 * - return false once, and output 1-bit addressing destination, and
 * - leave the remaining singlecast destinations to be fetched.
 */
void test_AssociationGetBitAdressingDestination_one_ep_two_ep_non_ep(void)
{
//  TEST_IGNORE();

  mock_t * pMock;
  mock_calls_clear();

  const uint8_t ENDPOINT = 0;
  const uint8_t GROUP_ID = 1;

  mock_call_use_as_stub(TO_STR(ZAF_GetInclusionMode));
  mock_call_use_as_stub(TO_STR(ZAF_GetSecurityKeys));
  mock_call_use_as_stub(TO_STR(GetHighestSecureLevel));

  mock_call_use_as_stub(TO_STR(ZAF_nvm_write));

  mock_call_expect(TO_STR(ZAF_nvm_read), &pMock);
  pMock->expect_arg[0].v = ZAF_FILE_ID_ASSOCIATIONINFO;
  pMock->compare_rule_arg[1] = COMPARE_NOT_NULL;
  pMock->expect_arg[2].v = 10395; // 3 bytes * (4 endpoints + root device) * 3 groups * 231 nodes

  //IMPORTANT: provide an SAssociationInfo struct full of zeros that can be read by AssociationInit()
  SAssociationInfo tAssociationInfo;
  memset(&tAssociationInfo, 0, sizeof(SAssociationInfo));

  for (uint8_t endpoint = 0; endpoint < ZAF_CONFIG_NUMBER_OF_END_POINTS; endpoint++)
  {
    for (uint8_t group = 0; group < CC_ASSOCIATION_MAX_GROUPS_PER_ENDPOINT; group++)
    {
      for (uint8_t node = 0; node < CC_ASSOCIATION_MAX_NODES_IN_GROUP; node++)
      {
        tAssociationInfo.Groups[endpoint][group].subGrp[node].node.nodeId = 0xFF; // Free
      }
    }
  }

  pMock->output_arg[2].p = &tAssociationInfo;

  mock_call_use_as_stub(TO_STR(ZAF_nvm_get_object_size));
  CC_Association_Init();

  node_t nodes[] = {
                    {2, 1, true},
                    {3, 1, true},
                    {3, 2, true},
                    {4, 0, false},
  };

  AddNodes(ENDPOINT, GROUP_ID, nodes, sizeof_array(nodes));

  destination_info_t * pList;
  uint8_t list_length;

  FetchAndPrintNodes( ENDPOINT, GROUP_ID, &pList, &list_length);

  destination_info_t * const P_LIST = pList;
  const uint8_t LIST_LENGTH = sizeof_array(nodes);

  destination_info_t nodeOutput;
  bool moreNodes;

  // Repeat the sequence a number of times to verify that the function reset itself after each run.
  for (uint8_t i = 0; i < 10; i++)
  {
    /*
     * Reset the input list for each run.
     * This is required when the function is used in Multicast e.g.
     */
    pList = P_LIST;
    list_length = LIST_LENGTH;

    AssociationGetDestinationInit(pList);

    /*
     * Invoke the first time.
     */
    moreNodes = AssociationGetBitAdressingDestination(&pList, &list_length, &nodeOutput);
    TEST_ASSERT_TRUE_MESSAGE(moreNodes, "No more nodes! :(");
    TEST_ASSERT_NOT_NULL_MESSAGE(pList, "pList has zero nodes :(");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(nodes[1].id, pList->node.nodeId, "pList points to the wrong node");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(3, list_length, "Remaining node count wrong (second)");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(0, nodeOutput.node.BitAddress, "Incorrect bit address value (first)");

    /*
     * Invoke the second time.
     */
    moreNodes = AssociationGetBitAdressingDestination(&pList, &list_length, &nodeOutput);

    TEST_ASSERT_FALSE_MESSAGE(moreNodes, "More nodes are left in the list! :(");
    TEST_ASSERT_NOT_NULL_MESSAGE(pList, "pList has zero nodes :(");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(nodes[3].id, pList->node.nodeId, "pList points to the wrong node");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(1, list_length, "Remaining node count wrong (second)");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(1, nodeOutput.node.BitAddress, "Incorrect bit address value (second)");

    TEST_ASSERT_EQUAL_UINT32_MESSAGE(2, AssociationGetSinglecastNodeCount(), "Singlecast count incorrect");

    destination_info_t * pNode;
    pNode = AssociationGetNextSinglecastDestination();
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(nodes[3].id, pNode->node.nodeId, "First singlecast incorrect (first)");
    pNode = AssociationGetNextSinglecastDestination();
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(nodes[0].id, pNode->node.nodeId, "Second singlecast incorrect (first)");
    pNode = AssociationGetNextSinglecastDestination();
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(nodes[3].id, pNode->node.nodeId, "First singlecast incorrect (second)");
    pNode = AssociationGetNextSinglecastDestination();
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(nodes[0].id, pNode->node.nodeId, "Second singlecast incorrect (second)");
  }

  mock_calls_verify();
}

/*
 * Given that associations are sorted with endpoint destinations first and then root device
 * destinations, this test verifies that when having nodes
 * 2.1
 * 3.1
 * 3.2
 * AssociationGetBitAdressingDestination() will
 * - return false once and output one bit addressing destination, and
 * - leave the remaining singlecast destinations to be fetched.
 */
void test_AssociationGetBitAdressingDestination_one_ep_two_ep(void)
{
//  TEST_IGNORE();

  mock_t * pMock;
  mock_calls_clear();

  const uint8_t ENDPOINT = 0;
  const uint8_t GROUP_ID = 1;

  mock_call_use_as_stub(TO_STR(ZAF_GetInclusionMode));
  mock_call_use_as_stub(TO_STR(ZAF_GetSecurityKeys));
  mock_call_use_as_stub(TO_STR(GetHighestSecureLevel));

  mock_call_use_as_stub(TO_STR(ZAF_nvm_write));

  mock_call_expect(TO_STR(ZAF_nvm_read), &pMock);
  pMock->expect_arg[0].v = ZAF_FILE_ID_ASSOCIATIONINFO;
  pMock->compare_rule_arg[1] = COMPARE_NOT_NULL;
  pMock->expect_arg[2].v = 10395; // 3 bytes * (4 endpoints + root device) * 3 groups * 231 nodes

  //IMPORTANT: provide an SAssociationInfo struct full of zeros that can be read by AssociationInit()
  SAssociationInfo tAssociationInfo;
  memset(&tAssociationInfo, 0, sizeof(SAssociationInfo));

  for (uint8_t endpoint = 0; endpoint < ZAF_CONFIG_NUMBER_OF_END_POINTS; endpoint++)
  {
    for (uint8_t group = 0; group < CC_ASSOCIATION_MAX_GROUPS_PER_ENDPOINT; group++)
    {
      for (uint8_t node = 0; node < CC_ASSOCIATION_MAX_NODES_IN_GROUP; node++)
      {
        tAssociationInfo.Groups[endpoint][group].subGrp[node].node.nodeId = 0xFF; // Free
      }
    }
  }

  pMock->output_arg[2].p = &tAssociationInfo;

  mock_call_use_as_stub(TO_STR(ZAF_nvm_get_object_size));
  CC_Association_Init();

  node_t nodes[] = {
                    {2, 1, true},
                    {3, 1, true},
                    {3, 2, true}
  };

  AddNodes(ENDPOINT, GROUP_ID, nodes, sizeof_array(nodes));

  destination_info_t * pList;
  uint8_t list_length;

  FetchAndPrintNodes( ENDPOINT, GROUP_ID, &pList, &list_length);

  destination_info_t * const P_LIST = pList;
  const uint8_t LIST_LENGTH = sizeof_array(nodes);

  destination_info_t nodeOutput;
  bool moreNodes;

  // Repeat the sequence a number of times to verify that the function reset itself after each run.
  for (uint8_t i = 0; i < 10; i++)
  {
    /*
     * Reset the input list for each run.
     * This is required when the function is used in Multicast e.g.
     */
    pList = P_LIST;
    list_length = LIST_LENGTH;

    AssociationGetDestinationInit(pList);

    /*
     * Invoke the first time.
     */
    moreNodes = AssociationGetBitAdressingDestination(&pList, &list_length, &nodeOutput);

    TEST_ASSERT_TRUE_MESSAGE(moreNodes, "No more nodes! :(");
    TEST_ASSERT_NOT_NULL_MESSAGE(pList, "pList has zero nodes :(");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(nodes[1].id, pList->node.nodeId, "pList points to the wrong node");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(2, list_length, "Remaining node count wrong (second)");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(0, nodeOutput.node.BitAddress, "Incorrect bit address value (first)");

    /*
     * Invoke the second time.
     */
    moreNodes = AssociationGetBitAdressingDestination(&pList, &list_length, &nodeOutput);

    TEST_ASSERT_FALSE_MESSAGE(moreNodes, "More nodes are left in the list! :(");
    TEST_ASSERT_NULL_MESSAGE(pList, "pList has more nodes :(");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(0, list_length, "Remaining node count wrong (second)");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(1, nodeOutput.node.BitAddress, "Incorrect bit address value (second)");

    TEST_ASSERT_EQUAL_UINT32_MESSAGE(1, AssociationGetSinglecastNodeCount(), "Singlecast count incorrect");

    destination_info_t * pNode;
    pNode = AssociationGetNextSinglecastDestination();
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(nodes[0].id, pNode->node.nodeId, "1");
    pNode = AssociationGetNextSinglecastDestination();
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(nodes[0].id, pNode->node.nodeId, "2");
    pNode = AssociationGetNextSinglecastDestination();
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(nodes[0].id, pNode->node.nodeId, "3");
    pNode = AssociationGetNextSinglecastDestination();
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(nodes[0].id, pNode->node.nodeId, "4");
  }

  mock_calls_verify();
}

/*
 * Verifies that associations with endpoints higher than 7 are not going to be set for Multi-Channel
 * transmission using Bit-Addressing. The implementation must treat these association as those without
 * an endpoint. This is done during sorting, where the endpoints higher than 7 are places last in the
 * association list. The reason is that bit addressing does not support more than 7 endpoints.
 *
 * These are already sorted, but do not need to be, since AddNodes() will sort the nodes during addition.
 * 70.1
 * 70.2
 * 80.1
 * 80.2
 * 40
 * 60
 * 70.8       endpoint higher than 7.
 */
void test_AssociationGetBitAdressingDestination_endpoints_larger_than_7(void)
{
//  TEST_IGNORE();

  mock_t * pMock;
  mock_calls_clear();

  const uint8_t ENDPOINT = 0;
  const uint8_t GROUP_ID = 1;

  mock_call_use_as_stub(TO_STR(ZAF_GetInclusionMode));
  mock_call_use_as_stub(TO_STR(ZAF_GetSecurityKeys));
  mock_call_use_as_stub(TO_STR(GetHighestSecureLevel));

  mock_call_use_as_stub(TO_STR(ZAF_nvm_write));

  mock_call_expect(TO_STR(ZAF_nvm_read), &pMock);
  pMock->expect_arg[0].v = ZAF_FILE_ID_ASSOCIATIONINFO;
  pMock->compare_rule_arg[1] = COMPARE_NOT_NULL;
  pMock->expect_arg[2].v = 10395; // 3 bytes * (4 endpoints + root device) * 3 groups * 231 nodes

  //IMPORTANT: provide an SAssociationInfo struct full of zeros that can be read by AssociationInit()
  SAssociationInfo tAssociationInfo;
  memset(&tAssociationInfo, 0, sizeof(SAssociationInfo));

  for (uint8_t endpoint = 0; endpoint < ZAF_CONFIG_NUMBER_OF_END_POINTS; endpoint++)
  {
    for (uint8_t group = 0; group < CC_ASSOCIATION_MAX_GROUPS_PER_ENDPOINT; group++)
    {
      for (uint8_t node = 0; node < CC_ASSOCIATION_MAX_NODES_IN_GROUP; node++)
      {
        tAssociationInfo.Groups[endpoint][group].subGrp[node].node.nodeId = 0xFF; // Free
      }
    }
  }

  pMock->output_arg[2].p = &tAssociationInfo;

  mock_call_use_as_stub(TO_STR(ZAF_nvm_get_object_size));
  CC_Association_Init();

  // Already sorted array, but does not need to be, since AddNodes() will sort them during addition.
  node_t nodes[] = {
                    {70, 1, false},
                    {70, 2, false},
                    {80, 1, false},
                    {80, 2, false},
                    {40, 0, false},
                    {60, 0, false},
                    {70, 8, false}
  };

  AddNodes(ENDPOINT, GROUP_ID, nodes, sizeof_array(nodes));

  destination_info_t * pList;
  uint8_t list_length;

  FetchAndPrintNodes( ENDPOINT, GROUP_ID, &pList, &list_length);

  destination_info_t * const P_LIST = pList;
  const uint8_t LIST_LENGTH = sizeof_array(nodes);

  destination_info_t nodeOutput;
  bool moreNodes;

  // Repeat the sequence a number of times to verify that the function reset itself after each run.
  for (uint8_t i = 0; i < 10; i++)
  {
    /*
     * Reset the input list for each run.
     * This is required when the function is used in Multicast e.g.
     */
    pList = P_LIST;
    list_length = LIST_LENGTH;

    AssociationGetDestinationInit(pList);

    /*
     * Invoke the first time.
     */
    moreNodes = AssociationGetBitAdressingDestination(&pList, &list_length, &nodeOutput);
    TEST_ASSERT_TRUE_MESSAGE(moreNodes, "No more nodes! :(");
    TEST_ASSERT_NOT_NULL_MESSAGE(pList, "pList has zero nodes :(");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(nodes[2].id, pList->node.nodeId, "pList points to the wrong node");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(5, list_length, "Remaining node count wrong (first)");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(70, nodeOutput.node.nodeId, "Incorrect output node ID (first)");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(3, nodeOutput.node.endpoint, "Incorrect endpoint (first)");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(1, nodeOutput.node.BitAddress, "Incorrect bit address value (first)");

    /*
     * Invoke the second time.
     */
    moreNodes = AssociationGetBitAdressingDestination(&pList, &list_length, &nodeOutput);

    TEST_ASSERT_FALSE_MESSAGE(moreNodes, "More nodes are left in the list! :(");
    TEST_ASSERT_NOT_NULL_MESSAGE(pList, "pList has zero nodes :(");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(3, list_length, "Remaining node count wrong (second)");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(80, nodeOutput.node.nodeId, "Incorrect output node ID (first)");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(3, nodeOutput.node.endpoint, "Incorrect endpoint (first)");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(1, nodeOutput.node.BitAddress, "Incorrect bit address value (first)");

    TEST_ASSERT_EQUAL_UINT32_MESSAGE(3, AssociationGetSinglecastNodeCount(), "Singlecast count incorrect");

    destination_info_t * pNode;
    pNode = AssociationGetNextSinglecastDestination();
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(nodes[4].id, pNode->node.nodeId, "1.1");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(nodes[4].endpoint, pNode->node.endpoint, "1.2");
    pNode = AssociationGetNextSinglecastDestination();
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(nodes[5].id, pNode->node.nodeId, "2.1");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(nodes[5].endpoint, pNode->node.endpoint, "2.2");
    pNode = AssociationGetNextSinglecastDestination();
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(nodes[6].id, pNode->node.nodeId, "3.1");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(nodes[6].endpoint, pNode->node.endpoint, "3.2");
    pNode = AssociationGetNextSinglecastDestination();
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(nodes[4].id, pNode->node.nodeId, "4.1");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(nodes[4].endpoint, pNode->node.endpoint, "4.2");
    pNode = AssociationGetNextSinglecastDestination();
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(nodes[5].id, pNode->node.nodeId, "5.1");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(nodes[5].endpoint, pNode->node.endpoint, "5.2");
    pNode = AssociationGetNextSinglecastDestination();
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(nodes[6].id, pNode->node.nodeId, "6.1");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(nodes[6].endpoint, pNode->node.endpoint, "6.2");
  }

  mock_calls_verify();
}
