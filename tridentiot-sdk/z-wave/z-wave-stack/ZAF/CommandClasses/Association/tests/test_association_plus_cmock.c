#include "association_plus_base.h"
#include "misc.h"
#include "unity.h"
#include "ZAF_nvm_mock.h"
#include "CC_AssociationGroupInfo_mock.h"
#include "ZAF_Common_interface_mock.h"
#include "ZW_TransportSecProtocol_mock.h"
#include "association_plus_file.h"
#include <string.h>

SAssociationInfo file;

zpal_status_t ZAF_nvm_write_Callback(__attribute__((unused)) zpal_nvm_object_key_t key, const void* object, size_t object_size, int cmock_num_calls)
{
  memcpy(&file, object, sizeof(SAssociationInfo));
  return ZPAL_STATUS_OK;
}

zpal_status_t ZAF_nvm_read_Callback(__attribute__((unused)) zpal_nvm_object_key_t key, void* object, size_t object_size, int cmock_num_calls)
{
  memcpy(object, &file, sizeof(SAssociationInfo));
  return ZPAL_STATUS_OK;
}

void setUpSuite(void)
{
  ZAF_nvm_get_object_size_IgnoreAndReturn(ZPAL_STATUS_OK);
  ZAF_nvm_write_Stub(ZAF_nvm_write_Callback);
  ZAF_nvm_read_Stub(ZAF_nvm_read_Callback);
}

void tearDownSuite(void)
{
}

void setUp(void)
{
  CC_Association_Init();
  CC_Association_Reset();
  printf("\n ZAF_CONFIG_NUMBER_OF_END_POINTS: %u", ZAF_CONFIG_NUMBER_OF_END_POINTS);
  printf("\n CC_ASSOCIATION_MAX_GROUPS_PER_ENDPOINT: %u", CC_ASSOCIATION_MAX_GROUPS_PER_ENDPOINT);
  printf("\n");
}

void tearDown(void)
{
}

void test_ReqNodeList_lifeline_zero_nodes(void)
{
  agi_profile_t profile = {
    .profile_MS = ASSOCIATION_GROUP_INFO_REPORT_AGI_PROFILE_GENERAL,
    .profile_LS = ASSOCIATION_GROUP_INFO_REPORT_AGI_GENERAL_LIFELINE
  };

  TRANSMIT_OPTIONS_TYPE_EX * tx_options = NULL;

  CC_AGI_groupCount_handler_IgnoreAndReturn(1);

  ZAF_GetInclusionMode_IgnoreAndReturn(EINCLUSIONMODE_ZWAVE_CLS);

  tx_options = ReqNodeList(&profile,
                           NULL, // Don't care when the profile is Lifeline.
                           0);

  TEST_ASSERT_NULL(tx_options);
}

void test_ReqNodeList_lifeline(void)
{
  ZAF_GetSecurityKeys_ExpectAndReturn(0);
  GetHighestSecureLevel_ExpectAndReturn(0, 0);

  MULTICHAN_DEST_NODE_ID node = {
    .nodeId = 1,
    .endpoint = 0,
    .BitAddress = 0
  };
  AssociationAddNode(1, 0, &node, false);

  agi_profile_t profile = {
    .profile_MS = ASSOCIATION_GROUP_INFO_REPORT_AGI_PROFILE_GENERAL,
    .profile_LS = ASSOCIATION_GROUP_INFO_REPORT_AGI_GENERAL_LIFELINE
  };

  TRANSMIT_OPTIONS_TYPE_EX * tx_options = NULL;

  CC_AGI_groupCount_handler_IgnoreAndReturn(1);

  ZAF_GetInclusionMode_IgnoreAndReturn(EINCLUSIONMODE_ZWAVE_CLS);

  tx_options = ReqNodeList(&profile,
                           NULL, // Don't care when the profile is Lifeline.
                           0);

  TRANSMIT_OPTIONS_TYPE_EX expected_tx_options = {
    .S2_groupID = 1,
    .txOptions = ZWAVE_PLUS_TX_OPTIONS,
    .sourceEndpoint = 0,
    .pList = NULL,
    .list_length = 1
  };

  TEST_ASSERT_NOT_NULL(tx_options);
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(expected_tx_options.S2_groupID, tx_options->S2_groupID, "S2 group ID");
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(expected_tx_options.txOptions, tx_options->txOptions, "TX options");
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(expected_tx_options.sourceEndpoint, tx_options->sourceEndpoint, "Source endpoint");
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(1, tx_options->pList->node.nodeId, "Node ID");
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(0, tx_options->pList->node.endpoint, "Endpoint");
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(0, tx_options->pList->node.BitAddress, "Bit addressing");
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(0, tx_options->pList->nodeInfo.BitMultiChannelEncap, "BitMultiChannelEncap");
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(0, tx_options->pList->nodeInfo.security, "security");
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(expected_tx_options.list_length, tx_options->list_length, "List length");
}

void test_ReqNodeList_group_2(void)
{
  const uint8_t ENDPOINT = 0;

  ZAF_GetSecurityKeys_ExpectAndReturn(0);
  GetHighestSecureLevel_ExpectAndReturn(0, 0);

  MULTICHAN_DEST_NODE_ID node = {
    .nodeId = 1,
    .endpoint = 0,
    .BitAddress = 0
  };
  AssociationAddNode(2, ENDPOINT, &node, false);

  agi_profile_t profile = {
    .profile_MS = 0xAA,
    .profile_LS = 0xBB
  };

  ccc_pair_t ccc_pair = {
    .cmdClass = COMMAND_CLASS_NOTIFICATION_V9,
    .cmd = NOTIFICATION_REPORT_V9
  };

  TRANSMIT_OPTIONS_TYPE_EX * tx_options = NULL;

  cc_agi_get_group_id_ExpectAndReturn(&profile, &ccc_pair, ENDPOINT, 2);

  CC_AGI_groupCount_handler_IgnoreAndReturn(2);

  ZAF_GetInclusionMode_IgnoreAndReturn(EINCLUSIONMODE_ZWAVE_CLS);

  tx_options = ReqNodeList(&profile,
                           &ccc_pair, // Don't care when the profile is Lifeline.
                           0);

  TRANSMIT_OPTIONS_TYPE_EX expected_tx_options = {
    .S2_groupID = 2,
    .txOptions = ZWAVE_PLUS_TX_OPTIONS,
    .sourceEndpoint = 0,
    .pList = NULL,
    .list_length = 1
  };

  TEST_ASSERT_NOT_NULL(tx_options);
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(expected_tx_options.S2_groupID, tx_options->S2_groupID, "S2 group ID");
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(expected_tx_options.txOptions, tx_options->txOptions, "TX options");
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(expected_tx_options.sourceEndpoint, tx_options->sourceEndpoint, "Source endpoint");
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(1, tx_options->pList->node.nodeId, "Node ID");
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(0, tx_options->pList->node.endpoint, "Endpoint");
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(0, tx_options->pList->node.BitAddress, "Bit addressing");
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(0, tx_options->pList->nodeInfo.BitMultiChannelEncap, "BitMultiChannelEncap");
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(0, tx_options->pList->nodeInfo.security, "security");
  TEST_ASSERT_EQUAL_UINT8_MESSAGE(expected_tx_options.list_length, tx_options->list_length, "List length");
}
