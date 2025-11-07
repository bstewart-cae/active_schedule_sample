// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file TestZW_controller.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include <stdbool.h>
#include <stdint.h>
#include <ZW_controller.h>
#include <ZW_transport.h>
#include <ZW_protocol_cmd_handler.h>
#include <zpal_radio_utils.h>
#include <ZW_explore.h>
#include <string.h>
#include <mock_control.h>

void setUpSuite(void) {

}

void tearDownSuite(void) {

}

/*
 * Global variables required for the test to build.
 */
uint8_t bNetworkWideInclusion;
bool bUseExploreAsRouteResolution;
bool inclusionHomeIDActive;
bool bNetworkWideInclusionReady;
uint8_t bCurrentRoutingSpeed;
uint8_t bMultispeedArrayByteCount;
uint8_t abMultispeedMergeNeighbors[MAX_NODEMASK_LENGTH];
bool protocolLockedCachedRoute;
bool exploreRemoteInclusion;
uint8_t crH[HOMEID_LENGTH];
uint8_t inclusionHomeID[4];
uint8_t RoutingInfoState;
uint8_t abNeighbors;
bool exploreInclusionModeRepeat;

void setUp(void)
{
  printf("\nRUN BEFORE EACH TEST!");
  mock_calls_clear();
}

void tearDown(void)
{
  printf("\nRUN AFTER EACH TEST!");
  mock_calls_verify();
}

/*
 * Verifies that ProtocolInterfacePassToAppMultiFrame() is invoked with the correct arguments for
 * command ID 0x20-0xFF.
 */
void test_command_handler_pass_to_application(void)
{
  mock_t * pMock;
  mock_calls_clear();

  /*
   * The value is chosen so that when shifting the most significant bits, the value will not be
   * the same:
   * 0b1011 1010 becomes
   *   101
   * 11010
   */
  const uint8_t TEST_VALUE = 0xBA;

  uint8_t multicast_bitmask[29];
  memset(&multicast_bitmask, 0, sizeof(multicast_bitmask));
  multicast_bitmask[0] = TEST_VALUE;

  uint8_t raw_frame[100];
  memset(&raw_frame, 0, sizeof(raw_frame));

  RECEIVE_OPTIONS_TYPE rxOptions;
  memset((uint8_t *)&rxOptions, 0, sizeof(rxOptions));

  for (uint32_t command_id = 0x20; command_id <= 0xFF; command_id++)
  {
    raw_frame[0] = command_id;

    mock_call_expect(TO_STR(ProtocolInterfacePassToAppMultiFrame), &pMock);
    pMock->expect_arg[0].v = (TEST_VALUE >> 5) & 0x07; // 3 msb
    pMock->expect_arg[1].v = TEST_VALUE & 0x1F; // 5 lsb
    pMock->compare_rule_arg[2] = COMPARE_NOT_NULL;
    pMock->expect_arg[3].p = raw_frame;
    pMock->expect_arg[4].v = sizeof(raw_frame);
    pMock->expect_arg[5].p = &rxOptions;

    CommandHandler_arg_t args;
    args.multi = multicast_bitmask;
    args.cmd = raw_frame;
    args.cmdLength = sizeof(raw_frame);
    args.rxOpt = &rxOptions;
    CommandHandler(&args);
  }

  mock_calls_verify();
}


/*
 * Verifies that GetMaxNodeID_LR returns correct LR nodeIDs
 */
void test_GetMaxNodeID_LR(void)
{
  mock_t * pMock;
  mock_calls_clear();

  //Verify that HIGHEST_LONG_RANGE_NODE_ID is returned
  mock_call_expect(TO_STR(ZW_LR_NodeMaskNodeIn), &pMock);
  pMock->return_code.value = 1;
  pMock->compare_rule_arg[0] = COMPARE_NOT_NULL;
  pMock->expect_arg[1].v = ZW_MAX_NODES_LR;

  node_id_t nodeID = GetMaxNodeID_LR();

  TEST_ASSERT(HIGHEST_LONG_RANGE_NODE_ID == nodeID);

  mock_calls_verify();
  mock_calls_clear();


  //Verify that LOWEST_LONG_RANGE_NODE_ID is returned
  for(uint32_t i = 0; i<(ZW_MAX_NODES_LR-1); i++)
  {
    mock_call_expect(TO_STR(ZW_LR_NodeMaskNodeIn), &pMock);
    pMock->return_code.value = 0;
    pMock->compare_rule_arg[0] = COMPARE_NOT_NULL;
    pMock->expect_arg[1].v = ZW_MAX_NODES_LR - i;
  }

  mock_call_expect(TO_STR(ZW_LR_NodeMaskNodeIn), &pMock);
  pMock->return_code.value = 1;
  pMock->compare_rule_arg[0] = COMPARE_NOT_NULL;
  pMock->expect_arg[1].v = 1;

  nodeID = GetMaxNodeID_LR();

  TEST_ASSERT(LOWEST_LONG_RANGE_NODE_ID == nodeID);

  mock_calls_verify();
  mock_calls_clear();


  //Verify that zero indicating no LR nodes is returned
  for(uint32_t i = 0; i<ZW_MAX_NODES_LR; i++)
  {
    mock_call_expect(TO_STR(ZW_LR_NodeMaskNodeIn), &pMock);
    pMock->return_code.value = 0;
    pMock->compare_rule_arg[0] = COMPARE_NOT_NULL;
    pMock->expect_arg[1].v = ZW_MAX_NODES_LR - i;
  }

  //return true on classic node
  mock_call_expect(TO_STR(ZW_NodeMaskNodeIn), &pMock);
  pMock->return_code.value = 1;
  pMock->compare_rule_arg[0] = COMPARE_NOT_NULL;
  pMock->expect_arg[1].v = ZW_MAX_NODES;

  nodeID = GetMaxNodeID_LR();

  TEST_ASSERT(0 == nodeID);

  mock_calls_verify();
}

/*
 * Verifies that AddNodeInfo saves correct LR nodes in LR bitmask
 */
void test_AddNodeInfo(void)
{
  mock_t * pMock;
  mock_calls_clear();

  mock_call_use_as_stub(TO_STR(zpal_nvm_get_object_size));
  mock_call_use_as_fake(TO_STR(zpal_nvm_read));
  mock_call_use_as_stub(TO_STR(zpal_nvm_write));

  //Test that nothing is set when nodeID is below LOWEST_LONG_RANGE_NODE_ID
  node_id_t nodeID = LOWEST_LONG_RANGE_NODE_ID - 1;
  NODEINFO_FRAME nodeInfoFrame;
  nodeInfoFrame.capability = ZWAVE_NODEINFO_LISTENING_SUPPORT;
  nodeInfoFrame.security = ZWAVE_NODEINFO_SPECIFIC_DEVICE_TYPE;
  AddNodeInfo(nodeID, (uint8_t*)&nodeInfoFrame, false);
  AddNodeInfo(nodeID, NULL, false);


  //Test that lowest bit is set when nodeID is LOWEST_LONG_RANGE_NODE_ID
  mock_call_expect(TO_STR(ZW_LR_NodeMaskSetBit), &pMock);
  pMock->compare_rule_arg[0] = COMPARE_NOT_NULL;
  pMock->expect_arg[1].v = 1;

  nodeID = LOWEST_LONG_RANGE_NODE_ID;
  AddNodeInfo(nodeID, (uint8_t*)&nodeInfoFrame, false);


  //Test that highest bit is set when nodeID is HIGHEST_LONG_RANGE_NODE_ID
  mock_call_expect(TO_STR(ZW_LR_NodeMaskSetBit), &pMock);
  pMock->compare_rule_arg[0] = COMPARE_NOT_NULL;
  pMock->expect_arg[1].v = ZW_MAX_NODES_LR;

  nodeID = HIGHEST_LONG_RANGE_NODE_ID;
  AddNodeInfo(nodeID, (uint8_t*)&nodeInfoFrame, false);


  //Test that nothing is set when nodeID is above HIGHEST_LONG_RANGE_NODE_ID
  nodeID = HIGHEST_LONG_RANGE_NODE_ID + 1;
  AddNodeInfo(nodeID, (uint8_t*)&nodeInfoFrame, false);
  AddNodeInfo(nodeID, NULL, false);

  mock_calls_verify();
}

/*
 * Verifies that
 * - a Z-Wave command received on LR channel is not processed, and
 * - a Z-Wave LR command received on non-LR channel is not processed.
 */
void test_verify_protocol_and_channel(void)
{
  /*
   * ZW command on LR channel
   */
  uint8_t raw_frame[100];
  memset(&raw_frame, 0, sizeof(raw_frame));

  uint8_t i = 0;
  raw_frame[i++] = ZWAVE_CMD_CLASS_PROTOCOL;
  raw_frame[i++] = ZWAVE_CMD_REQUEST_NODE_INFO;

  RECEIVE_OPTIONS_TYPE rxOptions;
  memset((uint8_t *)&rxOptions, 0, sizeof(rxOptions));
  rxOptions.isLongRangeChannel = true;

  CommandHandler_arg_t args;
  args.multi = NULL;
  args.cmd = raw_frame;
  args.cmdLength = i;
  args.rxOpt = &rxOptions;
  CommandHandler(&args);

  /*
   * LR command on ZW channel
   */
  memset(&raw_frame, 0, sizeof(raw_frame));

  i = 0;
  raw_frame[i++] = ZWAVE_CMD_CLASS_PROTOCOL_LR;
  raw_frame[i++] = ZWAVE_LR_CMD_REQUEST_NODE_INFO;

  memset((uint8_t *)&rxOptions, 0, sizeof(rxOptions));
  rxOptions.isLongRangeChannel = false;

  args.multi = NULL;
  args.cmd = raw_frame;
  args.cmdLength = i;
  args.rxOpt = &rxOptions;
  CommandHandler(&args);
}

static void helper_func_mocks_setup(void)
{
  mock_t * pMock;
  mock_calls_clear();
  mock_call_use_as_stub(TO_STR(ZW_NodeMaskNodeIn));
  mock_call_use_as_stub(TO_STR(ZW_NodeMaskSetBit));
  mock_call_use_as_stub(TO_STR(ZW_NodeMaskClearBit));
  mock_call_use_as_stub(TO_STR(ZW_NodeMaskClear));
  mock_call_use_as_stub(TO_STR(IsNodeRepeater));
  mock_call_use_as_stub(TO_STR(InitRoutingValues));
  mock_call_use_as_stub(TO_STR(zpal_nvm_write));
  mock_call_use_as_fake(TO_STR(zpal_nvm_read));
  mock_call_use_as_stub(TO_STR(ZW_LockRoute));
  mock_call_use_as_stub(TO_STR(ZW_system_startup_GetCCSet));
  mock_call_expect(TO_STR(zpal_radio_get_protocol_mode), &pMock);
  pMock->return_code.value = zpal_radio_region_get_protocol_mode(REGION_US_LR, ZPAL_RADIO_LR_CH_CFG1);
  mock_call_expect(TO_STR(zpal_radio_get_protocol_mode), &pMock);
  pMock->return_code.value = zpal_radio_region_get_protocol_mode(REGION_US_LR, ZPAL_RADIO_LR_CH_CFG1);
}

/*
 * Verifies that
 * - Node information generated for LR channel transmission follows Spec
 * - Node information generated for Classic channel transmission follows Spec.
 */
void test_GenerateNodeInformation(void)
{
  SAppNodeInfo_t appNodeInfo;
  SVirtualSlaveNodeInfoTable_t virtualSlaveNodeInfoTable;
  NODEINFO_FRAME pnodeInfo;
  NODEINFO_FRAME pnodeInfo_expected;

  /*
   * Test 1 - Controller node on REGION_US_LR GenerateNodeInformation output for LR channel
   *
   * Expected:
   * - Speed bits should be set to 100kbps LR only
   * - Routing support (ZWAVE_NODEINFO_ROUTING_SUPPORT) should NOT be set
   * - Optional functionality should NOT be set (ZWAVE_NODEINFO_OPTIONAL_FUNC)
   * - ZWAVE_NODEINFO_BEAM_CAPABILITY is NOT used as all controllers are capable of doing BEAMs
   * - Listening flag should be set
   * - No version bit like ZWAVE_NODEINFO_VERSION_4 are defined in Long Range Nodeinformation
   *
   */
  helper_func_mocks_setup();

  g_nodeID = 1;
  pnodeInfo_expected.cmdClass = ZWAVE_CMD_CLASS_PROTOCOL_LR;
  pnodeInfo_expected.cmd = ZWAVE_LR_CMD_NODE_INFO;
  pnodeInfo_expected.capability = ZWAVE_NODEINFO_LISTENING_MASK;
  pnodeInfo_expected.security = ZWAVE_NODEINFO_CONTROLLER_NODE;
  pnodeInfo_expected.reserved = ZWAVE_NODEINFO_BAUD_100KLR;
  pnodeInfo_expected.nodeType.basic = BASIC_TYPE_STATIC_CONTROLLER;
  pnodeInfo_expected.nodeType.generic = 0x36;
  pnodeInfo_expected.nodeType.specific = 0x63;
  appNodeInfo.DeviceOptionsMask = APPLICATION_NODEINFO_LISTENING;
  appNodeInfo.NodeType.generic = 0x36;
  appNodeInfo.NodeType.specific = 0x63;
  ControllerInit(&virtualSlaveNodeInfoTable, &appNodeInfo, false);
  uint8_t len = GenerateNodeInformation(&pnodeInfo, ZWAVE_CMD_CLASS_PROTOCOL_LR);

  TEST_ASSERT_EQUAL_UINT8_MESSAGE(8, len, "Nodeinfo length wrong");
  TEST_ASSERT_EQUAL_UINT8_ARRAY_MESSAGE(&pnodeInfo_expected, &pnodeInfo, len, "Returned Nodeinfo wrong for Controller LR channel on REGION_US_LR");

  mock_calls_verify();

  /*
   * Test 2 - Controller node on REGION_US_LR GenerateNodeInformation output for Classic channel
   *
   * Expected:
   * - Speed bits should be set to 100kbps LR, 100kbps and 40kbps on RGION_US_LR
   * - Routing support (ZWAVE_NODEINFO_ROUTING_SUPPORT) must be set
   * - Optional functionality must be set (ZWAVE_NODEINFO_OPTIONAL_FUNC)
   * - Specific device type bit should be set
   * - ZWAVE_NODEINFO_BEAM_CAPABILITY is must be set
   * - Listening flag should be set
   * - Version bits should be set
   * - Controller bit MUST be set
   *
   */
  helper_func_mocks_setup();

  g_nodeID = 1;
  pnodeInfo_expected.cmdClass = ZWAVE_CMD_CLASS_PROTOCOL;
  pnodeInfo_expected.cmd = ZWAVE_CMD_NODE_INFO;
  pnodeInfo_expected.capability = ZWAVE_NODEINFO_VERSION_4 | ZWAVE_NODEINFO_BAUD_40000 | ZWAVE_NODEINFO_ROUTING_SUPPORT | ZWAVE_NODEINFO_LISTENING_MASK;
  pnodeInfo_expected.security = ZWAVE_NODEINFO_CONTROLLER_NODE | ZWAVE_NODEINFO_BEAM_CAPABILITY | ZWAVE_NODEINFO_SPECIFIC_DEVICE_TYPE | ZWAVE_NODEINFO_OPTIONAL_FUNC;
  pnodeInfo_expected.reserved = ZWAVE_NODEINFO_BAUD_100KLR | ZWAVE_NODEINFO_BAUD_100K;
  pnodeInfo_expected.nodeType.generic = 0x45;
  pnodeInfo_expected.nodeType.specific = 0x54;
  appNodeInfo.DeviceOptionsMask = APPLICATION_NODEINFO_LISTENING;
  appNodeInfo.NodeType.generic = 0x45;
  appNodeInfo.NodeType.specific = 0x54;
  ControllerInit(&virtualSlaveNodeInfoTable, &appNodeInfo, false);
  len = GenerateNodeInformation(&pnodeInfo, ZWAVE_CMD_CLASS_PROTOCOL);

  TEST_ASSERT_EQUAL_UINT8_MESSAGE(8, len, "Nodeinfo length wrong");
  TEST_ASSERT_EQUAL_UINT8_ARRAY_MESSAGE(&pnodeInfo_expected, &pnodeInfo, len, "Returned Nodeinfo wrong for Controller classic channel on REGION_US_LR");

  mock_calls_verify();
}

/*
 * Verifies that ZW_HandleCmdIncludedNodeInfo() sends UPDATE_STATE_INCLUDED_NODE_INFO_RECEIVED to application
 * when conditions are correct.
 */
void test_ZW_HandleCmdIncludedNodeInfo(void)
{
  mock_t * pMock;
  mock_calls_clear();

  uint8_t homeID[] = {1, 2, 3, 4};
  ZW_HomeIDSet(homeID);

  uint8_t  pCmd[] = {ZWAVE_CMD_CLASS_PROTOCOL, ZWAVE_CMD_INCLUDED_NODE_INFO, 0,0,0,0};
  uint8_t  cmdLength = 6;

  RECEIVE_OPTIONS_TYPE rxopt;
  rxopt.sourceNode = 11;
  rxopt.rxStatus = 0;
  //SmartStart mode and foreign homeID -> must send to application.
  rxopt.homeId.word = 0xAABBCCDD; //foreign homeID
  bNetworkWideInclusion = NETWORK_WIDE_SMART_START;

  mock_call_expect(TO_STR(ProtocolInterfacePassToAppNodeUpdate), &pMock);
  pMock->expect_arg[0].v = UPDATE_STATE_INCLUDED_NODE_INFO_RECEIVED;
  pMock->expect_arg[1].v = rxopt.sourceNode;
  pMock->expect_arg[2].p = &pCmd[1];
  pMock->expect_arg[3].v = sizeof(CONTROLLER_UPDATE_INCLUDED_NODE_INFORMATION_FRAME);

  //ZW_HandleCmdIncludedNodeInfo() is a static function so it is called via zw_protocol_invoke_cmd_handler()
  zw_protocol_invoke_cmd_handler(pCmd, cmdLength, &rxopt);


  //SmartStartNWI mode and foreign homeID -> must send to application.
  rxopt.homeId.word = 0xAABBCCDD; //foreign homeID
  bNetworkWideInclusion = NETWORK_WIDE_SMART_START_NWI;

  mock_call_expect(TO_STR(ProtocolInterfacePassToAppNodeUpdate), &pMock);
  pMock->expect_arg[0].v = UPDATE_STATE_INCLUDED_NODE_INFO_RECEIVED;
  pMock->expect_arg[1].v = rxopt.sourceNode;
  pMock->expect_arg[2].p = &pCmd[1];
  pMock->expect_arg[3].v = sizeof(CONTROLLER_UPDATE_INCLUDED_NODE_INFORMATION_FRAME);

  pCmd[1] = ZWAVE_CMD_INCLUDED_NODE_INFO; //pCmd[1] was overwritten by ZW_HandleCmdIncludedNodeInfo()

  //ZW_HandleCmdIncludedNodeInfo() is a static function so it is called via zw_protocol_invoke_cmd_handler()
  zw_protocol_invoke_cmd_handler(pCmd, cmdLength, &rxopt);


  //NOT SmartStart mode and foreign homeID -> must NOT send to application.
  rxopt.homeId.word = 0xAABBCCDD; //foreign homeID
  bNetworkWideInclusion = NETWORK_WIDE_MODE_IDLE;

  pCmd[1] = ZWAVE_CMD_INCLUDED_NODE_INFO; //pCmd[1] was overwritten by ZW_HandleCmdIncludedNodeInfo()

  //ZW_HandleCmdIncludedNodeInfo() is a static function so it is called via zw_protocol_invoke_cmd_handler()
  zw_protocol_invoke_cmd_handler(pCmd, cmdLength, &rxopt);


  //SmartStart mode and NOT foreign homeID -> must NOT send to application.
  memcpy(rxopt.homeId.array, homeID, sizeof(homeID));
  bNetworkWideInclusion = NETWORK_WIDE_SMART_START;

  pCmd[1] = ZWAVE_CMD_INCLUDED_NODE_INFO; //pCmd[1] was overwritten by ZW_HandleCmdIncludedNodeInfo()

  //ZW_HandleCmdIncludedNodeInfo() is a static function so it is called via zw_protocol_invoke_cmd_handler()
  zw_protocol_invoke_cmd_handler(pCmd, cmdLength, &rxopt);


  //cmdLength too short -> must NOTsend to application.
  rxopt.homeId.word = 0xAABBCCDD; //foreign homeID
  bNetworkWideInclusion = NETWORK_WIDE_SMART_START;
  cmdLength = 5;

  pCmd[1] = ZWAVE_CMD_INCLUDED_NODE_INFO; //pCmd[1] was overwritten by ZW_HandleCmdIncludedNodeInfo()

  //ZW_HandleCmdIncludedNodeInfo() is a static function so it is called via zw_protocol_invoke_cmd_handler()
  zw_protocol_invoke_cmd_handler(pCmd, cmdLength, &rxopt);

  mock_calls_verify();
}
