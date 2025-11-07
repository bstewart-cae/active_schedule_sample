/**
 * @file test_AssociationGroupMapping.c
 * @copyright 2022 Silicon Laboratories Inc.
 */

#include <ZW_typedefs.h>
#include <CC_AssociationGroupInfo.h>
#include <CC_Association.h>
#include <association_plus.h>
#include <ZAF_file_ids.h>
#include <string.h>
#include <SizeOf.h>
#include <unity.h>
#include <mock_control.h>
#include <test_common.h>
#include "ZAF_CC_Invoker.h"
#include <zpal_nvm.h>
#include <endpoints_groups_associations_helper.h>
#include <cc_agi_config_api_mock.h>


void setUpSuite(void) {

}

void tearDownSuite(void) {

}

void setUp(void) {

}

void tearDown(void) {

}

#define handleCommandClassAssociation(a,b,c,d,e) invoke_cc_handler_v2(a,b,c,d,e)

static void InitAssociationFiles( mock_t** ppMock, SAssociationInfo* pAssociationFile) {

  size_t fileSize = ZAF_FILE_SIZE_ASSOCIATIONINFO;
  mock_call_expect(TO_STR(ZAF_nvm_get_object_size), ppMock);
  (*ppMock)->expect_arg[0].v     = ZAF_FILE_ID_ASSOCIATIONINFO;
  (*ppMock)->compare_rule_arg[1] = COMPARE_NOT_NULL;
  (*ppMock)->output_arg[1].p     = &fileSize;
  (*ppMock)->return_code.v       = ZPAL_STATUS_OK;

   mock_call_expect(TO_STR(ZAF_nvm_read), ppMock);
  (*ppMock)->expect_arg[0].v     = ZAF_FILE_ID_ASSOCIATIONINFO;
  (*ppMock)->compare_rule_arg[1] = COMPARE_NOT_NULL;
  (*ppMock)->output_arg[1].p     = pAssociationFile;
  (*ppMock)->expect_arg[2].v     = ZAF_FILE_SIZE_ASSOCIATIONINFO;
  (*ppMock)->return_code.v       = ZPAL_STATUS_OK;

  mock_call_expect(TO_STR(ZAF_nvm_write), ppMock);
  (*ppMock)->expect_arg[0].v     = ZAF_FILE_ID_ASSOCIATIONINFO;
  (*ppMock)->compare_rule_arg[1] = COMPARE_NOT_NULL;
  (*ppMock)->expect_arg[1].p     = pAssociationFile;
  (*ppMock)->expect_arg[2].v     = ZAF_FILE_SIZE_ASSOCIATIONINFO;
  (*ppMock)->return_code.v       = ZPAL_STATUS_OK;

   mock_call_expect(TO_STR(ZAF_nvm_read), ppMock);
  (*ppMock)->expect_arg[0].v     = ZAF_FILE_ID_ASSOCIATIONINFO;
  (*ppMock)->compare_rule_arg[1] = COMPARE_NOT_NULL;
  (*ppMock)->output_arg[1].p     = pAssociationFile;
  (*ppMock)->expect_arg[2].v     = ZAF_FILE_SIZE_ASSOCIATIONINFO;
  (*ppMock)->return_code.v       = ZPAL_STATUS_OK;

  CC_Association_Init();
}

/*
 * This function tests the mapping between endpoint groups and root device groups for the sake of
 * backwards compatibility.
 * We have the following mapping
 * 
 * EP group counts do not include lifeline.
 * Root Device: 1 | 2 3 | 4 5 6 | 7
 * EP1 : 2 3
 * EP2 : 2 3 4
 * EP3 : 2
 */
void test_ASSOCIATION_GET_endpoint_group_config1(void)
{
  mock_t* pMock = NULL;
  mock_calls_clear();

  mock_call_use_as_stub(TO_STR(ZAF_GetInclusionMode));

  RECEIVE_OPTIONS_TYPE_EX rxOptions;
  ZW_APPLICATION_TX_BUFFER frame;
  uint8_t commandLength = 0;
  const uint8_t MAX_NODES_SUPPORTED = 5;
  const uint8_t ENDPOINT = 0;

  rxOptions.destNode.endpoint = ENDPOINT;

  frame.ZW_AssociationGetV2Frame.cmdClass = COMMAND_CLASS_ASSOCIATION;
  frame.ZW_AssociationGetV2Frame.cmd = ASSOCIATION_GET;

  typedef struct _ep_gr_config{
    uint8_t lastRootGr;
    uint8_t epNo;
    uint8_t grCount;
    uint8_t grIDs[CC_ASSOCIATION_MAX_NODES_IN_GROUP];
  } ep_gr_config;

  const ep_gr_config ROOT_DEVICE_GROUP_MAPPING [3]= {{.epNo = 1, .grCount = 2, {1, 2, 3, 4, 5}},       //EP1
                                                     {.epNo = 2, .grCount = 3, {21, 22, 23, 4, 25}},   //EP2
                                                     {.epNo = 3, .grCount = 1, {31, 32, 33, 34, 35}}   //EP3
  };
  const uint8_t ROOT_GR_TO_EP[] = {0, 0, 1, 1, 1, 2};               // map the root device groip number of the corresponding endpoiny numer
  /*
   * Set up AGI
   */
  const ccc_pair_t CCC_PAIRS[] = {
                                  {
                                   .cmdClass = COMMAND_CLASS_BASIC,
                                   .cmd = BASIC_SET
                                  }
  };

  const cc_agi_group_t agiTableRootDeviceGroups[] = {
                                          {{0xAA, 0xBB}, sizeof_array(CCC_PAIRS), CCC_PAIRS, "Group 2"}, //endpoint 1
                                          {{0xCC, 0xDD}, sizeof_array(CCC_PAIRS), CCC_PAIRS, "Group 3"}, //endpoint 1
                                          {{0xEE, 0xFF}, sizeof_array(CCC_PAIRS), CCC_PAIRS, "Group 4"}, //endpoint 2
                                          {{0xAB, 0xCD}, sizeof_array(CCC_PAIRS), CCC_PAIRS, "Group 5"}, //endpoint 2
                                          {{0xAB, 0xCD}, sizeof_array(CCC_PAIRS), CCC_PAIRS, "Group 6"}, //endpoint 2
                                          {{0xAB, 0xCD}, sizeof_array(CCC_PAIRS), CCC_PAIRS, "Group 7"} //endpoint3
  };

 const cc_agi_group_t agiTableEndpoint1Groups[] =  { 
                                          {{0xAA, 0xBB}, sizeof_array(CCC_PAIRS), CCC_PAIRS, "EP 1 Group 1"},
                                          {{0xCC, 0xDD}, sizeof_array(CCC_PAIRS), CCC_PAIRS, "EP 1 Group 2"},
                                        };

 const cc_agi_group_t agiTableEndpoint2Groups[] =  { 
                                          {{0xEE, 0xFF}, sizeof_array(CCC_PAIRS), CCC_PAIRS, "EP 2 Group 1"},
                                          {{0xAA, 0xBB}, sizeof_array(CCC_PAIRS), CCC_PAIRS, "EP 2 Group 2"},
                                          {{0xCC, 0xDD}, sizeof_array(CCC_PAIRS), CCC_PAIRS, "EP 2 Group 3"},
                                        };

 const cc_agi_group_t agiTableEndpoint3Groups[] = { {{0xEE, 0xFF}, sizeof_array(CCC_PAIRS), CCC_PAIRS, "EP 3 Group 1"}, };

 const cc_agi_config_t customAgiTable[] = {
    {
      .groups = agiTableRootDeviceGroups,
      .group_count = sizeof_array(agiTableRootDeviceGroups)
    },
    {
      .groups = agiTableEndpoint1Groups,
      .group_count = sizeof_array(agiTableEndpoint1Groups)
    },
    {
      .groups = agiTableEndpoint2Groups,
      .group_count = sizeof_array(agiTableEndpoint2Groups)
    },
    {
      .groups = agiTableEndpoint3Groups,
      .group_count = sizeof_array(agiTableEndpoint3Groups)
    }
  };

  cc_agi_get_config_ExpectAndReturn(customAgiTable);

  ZAF_CC_init_specific(COMMAND_CLASS_ASSOCIATION_GRP_INFO);

  SAssociationInfo AssInfo;
  memset((uint8_t*)&AssInfo, 0x00, sizeof(SAssociationInfo));

  for (uint8_t x =0; x < sizeof_array(ROOT_DEVICE_GROUP_MAPPING); x++)
  {
    uint8_t curEp = ROOT_DEVICE_GROUP_MAPPING[x].epNo;
    uint8_t curGrCount = ROOT_DEVICE_GROUP_MAPPING[x].grCount;

    // Fill the root endpoint asscociation structure
    for (uint8_t grIndx = 0; grIndx < curGrCount; grIndx++) {
      for (uint8_t i = 0; i < CC_ASSOCIATION_MAX_NODES_IN_GROUP; i++) {
        AssInfo.Groups[curEp][grIndx + 1].subGrp[i].node.nodeId = ROOT_DEVICE_GROUP_MAPPING[x].grIDs[i];
      }
    }
  }

  InitAssociationFiles(&pMock, &AssInfo);
  uint8_t expectedFrame[] = {
      0x85, // COMMAND_CLASS_ASSOCIATION
      0x03, // ASSOCIATION_REPORT
      0,
      MAX_NODES_SUPPORTED,
      0, // Reports to follow
      0, // First node ID
      0,
      0,
      0,
      0 // Last node ID
  };  
  for (uint8_t groupID = 0; groupID < 6; groupID++) {
    printf("\nGroup ID: %u", groupID);
    frame.ZW_AssociationGetV2Frame.groupingIdentifier = groupID + 2; // Lifeline      
    expectedFrame[2] = groupID + 2;
    expectedFrame[5] = ROOT_DEVICE_GROUP_MAPPING[ROOT_GR_TO_EP[groupID]].grIDs[0];
    expectedFrame[6] = ROOT_DEVICE_GROUP_MAPPING[ROOT_GR_TO_EP[groupID]].grIDs[1];
    expectedFrame[7] = ROOT_DEVICE_GROUP_MAPPING[ROOT_GR_TO_EP[groupID]].grIDs[2];
    expectedFrame[8] = ROOT_DEVICE_GROUP_MAPPING[ROOT_GR_TO_EP[groupID]].grIDs[3];
    expectedFrame[9] = ROOT_DEVICE_GROUP_MAPPING[ROOT_GR_TO_EP[groupID]].grIDs[4];

    mock_call_use_as_stub(TO_STR(Check_not_legal_response_job));

    cc_agi_config_get_group_count_by_endpoint_ExpectAndReturn(ENDPOINT, customAgiTable[ENDPOINT].group_count);

    cc_agi_config_get_group_count_by_endpoint_ExpectAndReturn(1, customAgiTable[1].group_count);

    switch (groupID) {
      case 0:
        cc_agi_config_get_group_count_by_endpoint_ExpectAndReturn(1, customAgiTable[1].group_count);
        break;

      case 1:
        cc_agi_config_get_group_count_by_endpoint_ExpectAndReturn(1, customAgiTable[1].group_count);
        break;

      case 2:
        cc_agi_config_get_group_count_by_endpoint_ExpectAndReturn(1, customAgiTable[1].group_count);
        cc_agi_config_get_group_count_by_endpoint_ExpectAndReturn(2, customAgiTable[2].group_count);
        break;

      case 3:
        cc_agi_config_get_group_count_by_endpoint_ExpectAndReturn(1, customAgiTable[1].group_count);
        cc_agi_config_get_group_count_by_endpoint_ExpectAndReturn(2, customAgiTable[2].group_count);
        break;

      case 4:
        cc_agi_config_get_group_count_by_endpoint_ExpectAndReturn(1, customAgiTable[1].group_count);
        cc_agi_config_get_group_count_by_endpoint_ExpectAndReturn(2, customAgiTable[2].group_count);
        break;

      case 5:
        cc_agi_config_get_group_count_by_endpoint_ExpectAndReturn(1, customAgiTable[1].group_count);
        cc_agi_config_get_group_count_by_endpoint_ExpectAndReturn(2, customAgiTable[2].group_count);
        cc_agi_config_get_group_count_by_endpoint_ExpectAndReturn(3, customAgiTable[3].group_count);
        break;
    }
    received_frame_status_t status;
    char str[200];
    sprintf(str, "Wrong frame status :( group id: %d", groupID +2);
    
    ZW_APPLICATION_TX_BUFFER frameOut;
    uint8_t frameOutLength;
    status = handleCommandClassAssociation(
        &rxOptions,
        &frame,
        commandLength,
        &frameOut,
        &frameOutLength);

    TEST_ASSERT_EQUAL_INT8_MESSAGE(frameOutLength,
                                  sizeof(expectedFrame),
                                  "Frame size does not match");
    TEST_ASSERT_EQUAL_INT8_ARRAY_MESSAGE(expectedFrame, &frameOut,
                                        sizeof(expectedFrame),
                                        "Frame does not match");

    TEST_ASSERT_MESSAGE(status == RECEIVED_FRAME_STATUS_SUCCESS, str);
  }

  mock_calls_verify();
}


/*
 * This function tests the mapping between endpoint groups and root device groups for the sake of
 * backwards compatibility.
 * We have the following mapping
 * 
 * EP group counts do not include lifeline.
 * Root Device: 1 | 2 3 4| 5 | 6 7
 * EP1 : 2 3 4
 * EP2 : 2
 * EP3 : 2 3
 */
void test_ASSOCIATION_GET_endpoint_group_config2(void)
{
  mock_t* pMock = NULL;
  mock_calls_clear();

  mock_call_use_as_stub(TO_STR(ZAF_GetInclusionMode));

  RECEIVE_OPTIONS_TYPE_EX rxOptions;
  ZW_APPLICATION_TX_BUFFER frame;
  uint8_t commandLength = 0;
  const uint8_t MAX_NODES_SUPPORTED = 5;
  const uint8_t ENDPOINT = 0;

  rxOptions.destNode.endpoint = ENDPOINT;

  frame.ZW_AssociationGetV2Frame.cmdClass = COMMAND_CLASS_ASSOCIATION;
  frame.ZW_AssociationGetV2Frame.cmd = ASSOCIATION_GET;

  typedef struct _ep_gr_config{
    uint8_t lastRootGr;
    uint8_t epNo;
    uint8_t grCount;
    uint8_t grIDs[CC_ASSOCIATION_MAX_NODES_IN_GROUP];
  } ep_gr_config;

  const ep_gr_config ROOT_DEVICE_GROUP_MAPPING [3]= {{.epNo = 1, .grCount = 3, {1, 2, 3, 4, 5}},       //EP1
                                                     {.epNo = 2, .grCount = 1, {21, 22, 23, 4, 25}},   //EP2
                                                     {.epNo = 3, .grCount = 2, {31, 32, 33, 34, 35}}   //EP3
  };
  const uint8_t ROOT_GR_TO_EP[] = {0, 0, 0, 1, 2, 2};               // map the root device groip number of the corresponding endpoiny numer
  /*
   * Set up AGI
   */
  const ccc_pair_t CCC_PAIRS[] = {
                                  {
                                   .cmdClass = COMMAND_CLASS_BASIC,
                                   .cmd = BASIC_SET
                                  }
  };

  const cc_agi_group_t agiTableRootDeviceGroups[] = {
                                          {{0xAA, 0xBB}, sizeof_array(CCC_PAIRS), CCC_PAIRS, "Group 2"}, //endpoint 1
                                          {{0xCC, 0xDD}, sizeof_array(CCC_PAIRS), CCC_PAIRS, "Group 3"}, //endpoint 1
                                          {{0xEE, 0xFF}, sizeof_array(CCC_PAIRS), CCC_PAIRS, "Group 4"}, //endpoint 2
                                          {{0xAB, 0xCD}, sizeof_array(CCC_PAIRS), CCC_PAIRS, "Group 5"}, //endpoint 2
                                          {{0xAB, 0xCD}, sizeof_array(CCC_PAIRS), CCC_PAIRS, "Group 6"}, //endpoint 2
                                          {{0xAB, 0xCD}, sizeof_array(CCC_PAIRS), CCC_PAIRS, "Group 7"} //endpoint3
  };

 const cc_agi_group_t agiTableEndpoint1Groups[] =  { 
                                          {{0xAA, 0xBB}, sizeof_array(CCC_PAIRS), CCC_PAIRS, "EP 1 Group 1"},
                                          {{0xCC, 0xDD}, sizeof_array(CCC_PAIRS), CCC_PAIRS, "EP 1 Group 2"},
                                          {{0xEE, 0xFF}, sizeof_array(CCC_PAIRS), CCC_PAIRS, "EP 2 Group 1"},
                                        };

 const cc_agi_group_t agiTableEndpoint2Groups[] =  { 
                                          {{0xAA, 0xBB}, sizeof_array(CCC_PAIRS), CCC_PAIRS, "EP 2 Group 2"},
                                        };

 const cc_agi_group_t agiTableEndpoint3Groups[] =  { 
                                          {{0xEE, 0xFF}, sizeof_array(CCC_PAIRS), CCC_PAIRS, "EP 3 Group 1"},
                                          {{0xCC, 0xDD}, sizeof_array(CCC_PAIRS), CCC_PAIRS, "EP 2 Group 3"},
                                              };

 const cc_agi_config_t customAgiTable[] = {
    {
      .groups = agiTableRootDeviceGroups,
      .group_count = sizeof_array(agiTableRootDeviceGroups)
    },
    {
      .groups = agiTableEndpoint1Groups,
      .group_count = sizeof_array(agiTableEndpoint1Groups)
    },
    {
      .groups = agiTableEndpoint2Groups,
      .group_count = sizeof_array(agiTableEndpoint2Groups)
    },
    {
      .groups = agiTableEndpoint3Groups,
      .group_count = sizeof_array(agiTableEndpoint3Groups)
    }
  };

  cc_agi_get_config_ExpectAndReturn(customAgiTable);


  SAssociationInfo AssInfo;
  memset((uint8_t*)&AssInfo, 0x00, sizeof(SAssociationInfo));

  for (uint8_t x =0; x < sizeof_array(ROOT_DEVICE_GROUP_MAPPING); x++)
  {
    uint8_t curEp = ROOT_DEVICE_GROUP_MAPPING[x].epNo;
    uint8_t curGrCount = ROOT_DEVICE_GROUP_MAPPING[x].grCount;

    // Fill the root endpoint asscociation structure
    for (uint8_t grIndx = 0; grIndx < curGrCount; grIndx++) {
      for (uint8_t i = 0; i < CC_ASSOCIATION_MAX_NODES_IN_GROUP; i++) {
        AssInfo.Groups[curEp][grIndx + 1].subGrp[i].node.nodeId = ROOT_DEVICE_GROUP_MAPPING[x].grIDs[i];
      }
    }
  }

  InitAssociationFiles(&pMock, &AssInfo);
  uint8_t expectedFrame[] = {
      0x85, // COMMAND_CLASS_ASSOCIATION
      0x03, // ASSOCIATION_REPORT
      0,
      MAX_NODES_SUPPORTED,
      0, // Reports to follow
      0, // First node ID
      0,
      0,
      0,
      0 // Last node ID
  };  
  for (uint8_t groupID = 0; groupID < 6; groupID++) {
    frame.ZW_AssociationGetV2Frame.groupingIdentifier = groupID + 2; // Lifeline      
    expectedFrame[2] = groupID + 2;
    expectedFrame[5] = ROOT_DEVICE_GROUP_MAPPING[ROOT_GR_TO_EP[groupID]].grIDs[0];
    expectedFrame[6] = ROOT_DEVICE_GROUP_MAPPING[ROOT_GR_TO_EP[groupID]].grIDs[1];
    expectedFrame[7] = ROOT_DEVICE_GROUP_MAPPING[ROOT_GR_TO_EP[groupID]].grIDs[2];
    expectedFrame[8] = ROOT_DEVICE_GROUP_MAPPING[ROOT_GR_TO_EP[groupID]].grIDs[3];
    expectedFrame[9] = ROOT_DEVICE_GROUP_MAPPING[ROOT_GR_TO_EP[groupID]].grIDs[4];

    mock_call_use_as_stub(TO_STR(Check_not_legal_response_job));

    cc_agi_config_get_group_count_by_endpoint_ExpectAndReturn(ENDPOINT, customAgiTable[ENDPOINT].group_count);

    cc_agi_config_get_group_count_by_endpoint_ExpectAndReturn(1, customAgiTable[1].group_count);

    switch (groupID) {
      case 0:
        cc_agi_config_get_group_count_by_endpoint_ExpectAndReturn(1, customAgiTable[1].group_count);
        break;

      case 1:
        cc_agi_config_get_group_count_by_endpoint_ExpectAndReturn(1, customAgiTable[1].group_count);
        break;

      case 2:
        cc_agi_config_get_group_count_by_endpoint_ExpectAndReturn(1, customAgiTable[1].group_count);
        break;

      case 3:
        cc_agi_config_get_group_count_by_endpoint_ExpectAndReturn(1, customAgiTable[1].group_count);
        cc_agi_config_get_group_count_by_endpoint_ExpectAndReturn(2, customAgiTable[2].group_count);
        break;

      case 4:
        cc_agi_config_get_group_count_by_endpoint_ExpectAndReturn(1, customAgiTable[1].group_count);
        cc_agi_config_get_group_count_by_endpoint_ExpectAndReturn(2, customAgiTable[2].group_count);
        cc_agi_config_get_group_count_by_endpoint_ExpectAndReturn(3, customAgiTable[3].group_count);
        break;

      case 5:
        cc_agi_config_get_group_count_by_endpoint_ExpectAndReturn(1, customAgiTable[1].group_count);
        cc_agi_config_get_group_count_by_endpoint_ExpectAndReturn(2, customAgiTable[2].group_count);
        cc_agi_config_get_group_count_by_endpoint_ExpectAndReturn(3, customAgiTable[3].group_count);
        break;
    }

    received_frame_status_t status;
    ZW_APPLICATION_TX_BUFFER frameOut;
    uint8_t frameOutLength;
    char str[200];
    sprintf(str, "Wrong frame status :( group id: %d", groupID +2);
    status = handleCommandClassAssociation(
        &rxOptions,
        &frame,
        commandLength,
        &frameOut,
        &frameOutLength);

    TEST_ASSERT_EQUAL_INT8_MESSAGE(frameOutLength,
                                  sizeof(expectedFrame),
                                  "Frame size does not match");
    TEST_ASSERT_EQUAL_INT8_ARRAY_MESSAGE(expectedFrame, &frameOut,
                                        sizeof(expectedFrame),
                                        "Frame does not match");

    TEST_ASSERT_MESSAGE(status == RECEIVED_FRAME_STATUS_SUCCESS, str);
  }

  mock_calls_verify();
}



/*
 * This function tests the mapping between endpoint groups and root device groups for the sake of
 * backwards compatibility.
 * We have the following mapping
 * 
 * EP group counts do not include lifeline.
 * Root Device: 1 | 2 | 3 4 5 | 6 7
 * EP1 : 2
 * EP2 : 2 3 4
 * EP3 : 2 3
 */
void test_ASSOCIATION_GET_endpoint_group_config3(void)
{
  mock_t* pMock = NULL;
  mock_calls_clear();

  mock_call_use_as_stub(TO_STR(ZAF_GetInclusionMode));

  RECEIVE_OPTIONS_TYPE_EX rxOptions;
  ZW_APPLICATION_TX_BUFFER frame;
  uint8_t commandLength = 0;
  const uint8_t MAX_NODES_SUPPORTED = 5;
  const uint8_t ENDPOINT = 0;

  rxOptions.destNode.endpoint = ENDPOINT;

  frame.ZW_AssociationGetV2Frame.cmdClass = COMMAND_CLASS_ASSOCIATION;
  frame.ZW_AssociationGetV2Frame.cmd = ASSOCIATION_GET;

  typedef struct _ep_gr_config{
    uint8_t lastRootGr;
    uint8_t epNo;
    uint8_t grCount;
    uint8_t grIDs[CC_ASSOCIATION_MAX_NODES_IN_GROUP];
  } ep_gr_config;

  const ep_gr_config ROOT_DEVICE_GROUP_MAPPING [3]= {{.epNo = 1, .grCount = 1, {1, 2, 3, 4, 5}},       //EP1
                                                     {.epNo = 2, .grCount = 3, {21, 22, 23, 4, 25}},   //EP2
                                                     {.epNo = 3, .grCount = 2, {31, 32, 33, 34, 35}}   //EP3
  };
  const uint8_t ROOT_GR_TO_EP[] = {0, 1, 1, 1, 2, 2};               // map the root device groip number of the corresponding endpoiny numer
  /*
   * Set up AGI
   */
  const ccc_pair_t CCC_PAIRS[] = {
                                  {
                                   .cmdClass = COMMAND_CLASS_BASIC,
                                   .cmd = BASIC_SET
                                  }
  };

  const cc_agi_group_t agiTableRootDeviceGroups[] = {
                                          {{0xAA, 0xBB}, sizeof_array(CCC_PAIRS), CCC_PAIRS, "Group 2"}, //endpoint 1
                                          {{0xCC, 0xDD}, sizeof_array(CCC_PAIRS), CCC_PAIRS, "Group 3"}, //endpoint 1
                                          {{0xEE, 0xFF}, sizeof_array(CCC_PAIRS), CCC_PAIRS, "Group 4"}, //endpoint 2
                                          {{0xAB, 0xCD}, sizeof_array(CCC_PAIRS), CCC_PAIRS, "Group 5"}, //endpoint 2
                                          {{0xAB, 0xCD}, sizeof_array(CCC_PAIRS), CCC_PAIRS, "Group 6"}, //endpoint 2
                                          {{0xAB, 0xCD}, sizeof_array(CCC_PAIRS), CCC_PAIRS, "Group 7"} //endpoint3
  };

 const cc_agi_group_t agiTableEndpoint1Groups[] =  { 
                                          {{0xAA, 0xBB}, sizeof_array(CCC_PAIRS), CCC_PAIRS, "EP 1 Group 1"},
                                        };

 const cc_agi_group_t agiTableEndpoint2Groups[] =  { 
                                          {{0xCC, 0xDD}, sizeof_array(CCC_PAIRS), CCC_PAIRS, "EP 1 Group 2"},
                                          {{0xEE, 0xFF}, sizeof_array(CCC_PAIRS), CCC_PAIRS, "EP 2 Group 1"},
                                          {{0xAA, 0xBB}, sizeof_array(CCC_PAIRS), CCC_PAIRS, "EP 2 Group 2"},
                                        };

 const cc_agi_group_t agiTableEndpoint3Groups[] =  { 
                                          {{0xEE, 0xFF}, sizeof_array(CCC_PAIRS), CCC_PAIRS, "EP 3 Group 1"},
                                          {{0xCC, 0xDD}, sizeof_array(CCC_PAIRS), CCC_PAIRS, "EP 2 Group 3"},
                                              };

 const cc_agi_config_t customAgiTable[] = {
    {
      .groups = agiTableRootDeviceGroups,
      .group_count = sizeof_array(agiTableRootDeviceGroups)
    },
    {
      .groups = agiTableEndpoint1Groups,
      .group_count = sizeof_array(agiTableEndpoint1Groups)
    },
    {
      .groups = agiTableEndpoint2Groups,
      .group_count = sizeof_array(agiTableEndpoint2Groups)
    },
    {
      .groups = agiTableEndpoint3Groups,
      .group_count = sizeof_array(agiTableEndpoint3Groups)
    }
  };

  cc_agi_get_config_ExpectAndReturn(customAgiTable);

  ZAF_CC_init_specific(COMMAND_CLASS_ASSOCIATION_GRP_INFO);

  SAssociationInfo AssInfo;
  memset((uint8_t*)&AssInfo, 0x00, sizeof(SAssociationInfo));

  for (uint8_t x =0; x < sizeof_array(ROOT_DEVICE_GROUP_MAPPING); x++)
  {
    uint8_t curEp = ROOT_DEVICE_GROUP_MAPPING[x].epNo;
    uint8_t curGrCount = ROOT_DEVICE_GROUP_MAPPING[x].grCount;

    // Fill the root endpoint asscociation structure
    for (uint8_t grIndx = 0; grIndx < curGrCount; grIndx++) {
      for (uint8_t i = 0; i < CC_ASSOCIATION_MAX_NODES_IN_GROUP; i++) {
        AssInfo.Groups[curEp][grIndx + 1].subGrp[i].node.nodeId = ROOT_DEVICE_GROUP_MAPPING[x].grIDs[i];
      }
    }
  }
 

  InitAssociationFiles(&pMock, &AssInfo);
  uint8_t expectedFrame[] = {
      0x85, // COMMAND_CLASS_ASSOCIATION
      0x03, // ASSOCIATION_REPORT
      0,
      MAX_NODES_SUPPORTED,
      0, // Reports to follow
      0, // First node ID
      0,
      0,
      0,
      0 // Last node ID
  };  
  for (uint8_t groupID = 0; groupID < 6; groupID++) {
    frame.ZW_AssociationGetV2Frame.groupingIdentifier = groupID + 2; // Lifeline      
    expectedFrame[2] = groupID + 2;
    expectedFrame[5] = ROOT_DEVICE_GROUP_MAPPING[ROOT_GR_TO_EP[groupID]].grIDs[0];
    expectedFrame[6] = ROOT_DEVICE_GROUP_MAPPING[ROOT_GR_TO_EP[groupID]].grIDs[1];
    expectedFrame[7] = ROOT_DEVICE_GROUP_MAPPING[ROOT_GR_TO_EP[groupID]].grIDs[2];
    expectedFrame[8] = ROOT_DEVICE_GROUP_MAPPING[ROOT_GR_TO_EP[groupID]].grIDs[3];
    expectedFrame[9] = ROOT_DEVICE_GROUP_MAPPING[ROOT_GR_TO_EP[groupID]].grIDs[4];

    mock_call_use_as_stub(TO_STR(Check_not_legal_response_job));

    cc_agi_config_get_group_count_by_endpoint_ExpectAndReturn(ENDPOINT, customAgiTable[ENDPOINT].group_count);

    cc_agi_config_get_group_count_by_endpoint_ExpectAndReturn(1, customAgiTable[1].group_count);

    switch (groupID) {
      case 0:
        cc_agi_config_get_group_count_by_endpoint_ExpectAndReturn(1, customAgiTable[1].group_count);
        break;

      case 1:
        cc_agi_config_get_group_count_by_endpoint_ExpectAndReturn(1, customAgiTable[1].group_count);
        cc_agi_config_get_group_count_by_endpoint_ExpectAndReturn(2, customAgiTable[2].group_count);
        break;

      case 2:
        cc_agi_config_get_group_count_by_endpoint_ExpectAndReturn(1, customAgiTable[1].group_count);
        cc_agi_config_get_group_count_by_endpoint_ExpectAndReturn(2, customAgiTable[2].group_count);
        break;

      case 3:
        cc_agi_config_get_group_count_by_endpoint_ExpectAndReturn(1, customAgiTable[1].group_count);
        cc_agi_config_get_group_count_by_endpoint_ExpectAndReturn(2, customAgiTable[2].group_count);
        break;

      case 4:
        cc_agi_config_get_group_count_by_endpoint_ExpectAndReturn(1, customAgiTable[1].group_count);
        cc_agi_config_get_group_count_by_endpoint_ExpectAndReturn(2, customAgiTable[2].group_count);
        cc_agi_config_get_group_count_by_endpoint_ExpectAndReturn(3, customAgiTable[3].group_count);
        break;

      case 5:
        cc_agi_config_get_group_count_by_endpoint_ExpectAndReturn(1, customAgiTable[1].group_count);
        cc_agi_config_get_group_count_by_endpoint_ExpectAndReturn(2, customAgiTable[2].group_count);
        cc_agi_config_get_group_count_by_endpoint_ExpectAndReturn(3, customAgiTable[3].group_count);
        break;
    }

    received_frame_status_t status;
    char str[200];
    sprintf(str, "Wrong frame status :( group id: %d", groupID +2);

    ZW_APPLICATION_TX_BUFFER frameOut;
    uint8_t frameOutLength;
    status = handleCommandClassAssociation(
        &rxOptions,
        &frame,
        commandLength,
        &frameOut,
        &frameOutLength);

    TEST_ASSERT_EQUAL_INT8_MESSAGE(frameOutLength,
                                  sizeof(expectedFrame),
                                  "Frame size does not match");
    TEST_ASSERT_EQUAL_INT8_ARRAY_MESSAGE(expectedFrame, &frameOut,
                                        sizeof(expectedFrame),
                                        "Frame does not match");

    TEST_ASSERT_MESSAGE(status == RECEIVED_FRAME_STATUS_SUCCESS, str);
  }

  mock_calls_verify();
}



/*
 * This function tests the mapping between endpoint groups and root device groups for the sake of
 * backwards compatibility.
 * We have the following mapping
 * 
 * EP group counts do not include lifeline.
 * Root Device: 1 | 2 | 3 | 4 5 6 7
 * EP1 : 2
 * EP2 : 2
 * EP3 : 2 3 4 5
 */
void test_ASSOCIATION_GET_endpoint_group_config4(void)
{
  mock_t* pMock = NULL;
  mock_calls_clear();

  mock_call_use_as_stub(TO_STR(ZAF_GetInclusionMode));

  RECEIVE_OPTIONS_TYPE_EX rxOptions;
  ZW_APPLICATION_TX_BUFFER frame;
  uint8_t commandLength = 0;
  const uint8_t MAX_NODES_SUPPORTED = 5;
  const uint8_t ENDPOINT = 0;

  rxOptions.destNode.endpoint = ENDPOINT;

  frame.ZW_AssociationGetV2Frame.cmdClass = COMMAND_CLASS_ASSOCIATION;
  frame.ZW_AssociationGetV2Frame.cmd = ASSOCIATION_GET;

  typedef struct _ep_gr_config{
    uint8_t lastRootGr;
    uint8_t epNo;
    uint8_t grCount;
    uint8_t grIDs[CC_ASSOCIATION_MAX_NODES_IN_GROUP];
  } ep_gr_config;

  const ep_gr_config ROOT_DEVICE_GROUP_MAPPING [3]= {{.epNo = 1, .grCount = 1, {1, 2, 3, 4, 5}},       //EP1
                                                     {.epNo = 2, .grCount = 1, {21, 22, 23, 4, 25}},   //EP2
                                                     {.epNo = 3, .grCount = 4, {31, 32, 33, 34, 35}}   //EP3
  };
  const uint8_t ROOT_GR_TO_EP[] = {0, 1, 2, 2, 2, 2};               // map the root device groip number of the corresponding endpoiny numer

  /*
   * Set up AGI
   */
  const ccc_pair_t CCC_PAIRS[] = {
                                  {
                                   .cmdClass = COMMAND_CLASS_BASIC,
                                   .cmd = BASIC_SET
                                  }
  };

  const cc_agi_group_t agiTableRootDeviceGroups[] = {
                                          {{0xAA, 0xBB}, sizeof_array(CCC_PAIRS), CCC_PAIRS, "Group 2"}, //endpoint 1
                                          {{0xCC, 0xDD}, sizeof_array(CCC_PAIRS), CCC_PAIRS, "Group 3"}, //endpoint 1
                                          {{0xEE, 0xFF}, sizeof_array(CCC_PAIRS), CCC_PAIRS, "Group 4"}, //endpoint 2
                                          {{0xAB, 0xCD}, sizeof_array(CCC_PAIRS), CCC_PAIRS, "Group 5"}, //endpoint 2
                                          {{0xAB, 0xCD}, sizeof_array(CCC_PAIRS), CCC_PAIRS, "Group 6"}, //endpoint 2
                                          {{0xAB, 0xCD}, sizeof_array(CCC_PAIRS), CCC_PAIRS, "Group 7"} //endpoint3
  };

 const cc_agi_group_t agiTableEndpoint1Groups[] =  { 
                                          {{0xAA, 0xBB}, sizeof_array(CCC_PAIRS), CCC_PAIRS, "EP 1 Group 1"},
                                        };

 const cc_agi_group_t agiTableEndpoint2Groups[] =  { 
                                          {{0xCC, 0xDD}, sizeof_array(CCC_PAIRS), CCC_PAIRS, "EP 1 Group 2"},
                                        };

 const cc_agi_group_t agiTableEndpoint3Groups[] =  { 
                                          {{0xEE, 0xFF}, sizeof_array(CCC_PAIRS), CCC_PAIRS, "EP 2 Group 1"},
                                          {{0xAA, 0xBB}, sizeof_array(CCC_PAIRS), CCC_PAIRS, "EP 2 Group 2"},
                                          {{0xEE, 0xFF}, sizeof_array(CCC_PAIRS), CCC_PAIRS, "EP 3 Group 1"},
                                          {{0xCC, 0xDD}, sizeof_array(CCC_PAIRS), CCC_PAIRS, "EP 2 Group 3"},
                                              };

  const cc_agi_config_t customAgiTable[] =
  {
   {
    .groups = agiTableRootDeviceGroups,
    .group_count = sizeof_array(agiTableRootDeviceGroups)
   },
   {
    .groups = agiTableEndpoint1Groups,
    .group_count = sizeof_array(agiTableEndpoint1Groups)
   },
   {
    .groups = agiTableEndpoint2Groups,
    .group_count = sizeof_array(agiTableEndpoint2Groups)
   },
   {
    .groups = agiTableEndpoint3Groups,
    .group_count = sizeof_array(agiTableEndpoint3Groups)
   }
  };

  cc_agi_get_config_ExpectAndReturn(customAgiTable);

  ZAF_CC_init_specific(COMMAND_CLASS_ASSOCIATION_GRP_INFO);

  SAssociationInfo AssInfo;
  memset((uint8_t*)&AssInfo, 0x00, sizeof(SAssociationInfo));


  for (uint8_t x =0; x < sizeof_array(ROOT_DEVICE_GROUP_MAPPING); x++)
  {
    uint8_t curEp = ROOT_DEVICE_GROUP_MAPPING[x].epNo;
    uint8_t curGrCount = ROOT_DEVICE_GROUP_MAPPING[x].grCount;

    // Fill the root endpoint asscociation structure
    for (uint8_t grIndx = 0; grIndx < curGrCount; grIndx++) {
      for (uint8_t i = 0; i < CC_ASSOCIATION_MAX_NODES_IN_GROUP; i++) {
        AssInfo.Groups[curEp][grIndx + 1].subGrp[i].node.nodeId = ROOT_DEVICE_GROUP_MAPPING[x].grIDs[i];
      }
    }
  }
  InitAssociationFiles(&pMock, &AssInfo);
  uint8_t expectedFrame[] = {
      0x85, // COMMAND_CLASS_ASSOCIATION
      0x03, // ASSOCIATION_REPORT
      0,
      MAX_NODES_SUPPORTED,
      0, // Reports to follow
      0, // First node ID
      0,
      0,
      0,
      0 // Last node ID
  };  
  for (uint8_t groupID = 0; groupID < 6; groupID++) {
    frame.ZW_AssociationGetV2Frame.groupingIdentifier = groupID + 2; // Lifeline      
    expectedFrame[2] = groupID + 2;
    expectedFrame[5] = ROOT_DEVICE_GROUP_MAPPING[ROOT_GR_TO_EP[groupID]].grIDs[0];
    expectedFrame[6] = ROOT_DEVICE_GROUP_MAPPING[ROOT_GR_TO_EP[groupID]].grIDs[1];
    expectedFrame[7] = ROOT_DEVICE_GROUP_MAPPING[ROOT_GR_TO_EP[groupID]].grIDs[2];
    expectedFrame[8] = ROOT_DEVICE_GROUP_MAPPING[ROOT_GR_TO_EP[groupID]].grIDs[3];
    expectedFrame[9] = ROOT_DEVICE_GROUP_MAPPING[ROOT_GR_TO_EP[groupID]].grIDs[4];

    mock_call_use_as_stub(TO_STR(Check_not_legal_response_job));

    cc_agi_config_get_group_count_by_endpoint_ExpectAndReturn(ENDPOINT, customAgiTable[ENDPOINT].group_count);

    cc_agi_config_get_group_count_by_endpoint_ExpectAndReturn(1, customAgiTable[1].group_count);

    switch (groupID) {
      case 0:
        cc_agi_config_get_group_count_by_endpoint_ExpectAndReturn(1, customAgiTable[1].group_count);
        break;

      case 1:
        cc_agi_config_get_group_count_by_endpoint_ExpectAndReturn(1, customAgiTable[1].group_count);
        cc_agi_config_get_group_count_by_endpoint_ExpectAndReturn(2, customAgiTable[2].group_count);
        break;

      case 2:
        cc_agi_config_get_group_count_by_endpoint_ExpectAndReturn(1, customAgiTable[1].group_count);
        cc_agi_config_get_group_count_by_endpoint_ExpectAndReturn(2, customAgiTable[2].group_count);
        cc_agi_config_get_group_count_by_endpoint_ExpectAndReturn(3, customAgiTable[3].group_count);
        break;

      case 3:
        cc_agi_config_get_group_count_by_endpoint_ExpectAndReturn(1, customAgiTable[1].group_count);
        cc_agi_config_get_group_count_by_endpoint_ExpectAndReturn(2, customAgiTable[2].group_count);
        cc_agi_config_get_group_count_by_endpoint_ExpectAndReturn(3, customAgiTable[3].group_count);
        break;

      case 4:
        cc_agi_config_get_group_count_by_endpoint_ExpectAndReturn(1, customAgiTable[1].group_count);
        cc_agi_config_get_group_count_by_endpoint_ExpectAndReturn(2, customAgiTable[2].group_count);
        cc_agi_config_get_group_count_by_endpoint_ExpectAndReturn(3, customAgiTable[3].group_count);
        break;

      case 5:
        cc_agi_config_get_group_count_by_endpoint_ExpectAndReturn(1, customAgiTable[1].group_count);
        cc_agi_config_get_group_count_by_endpoint_ExpectAndReturn(2, customAgiTable[2].group_count);
        cc_agi_config_get_group_count_by_endpoint_ExpectAndReturn(3, customAgiTable[3].group_count);
        break;
    }

    received_frame_status_t status;
    char str[200];
    sprintf(str, "Wrong frame status :( group id: %d", groupID +2);

    ZW_APPLICATION_TX_BUFFER frameOut;
    uint8_t frameOutLength;
    status = handleCommandClassAssociation(
        &rxOptions,
        &frame,
        commandLength,
        &frameOut,
        &frameOutLength);

  TEST_ASSERT_EQUAL_INT8_MESSAGE(frameOutLength,
                                  sizeof(expectedFrame),
                                  "Frame size does not match");
  TEST_ASSERT_EQUAL_INT8_ARRAY_MESSAGE(expectedFrame, &frameOut,
                                        sizeof(expectedFrame),
                                        "Frame does not match");

    TEST_ASSERT_MESSAGE(status == RECEIVED_FRAME_STATUS_SUCCESS, str);
  }

  mock_calls_verify();
}
