// SPDX-FileCopyrightText: Silicon Laboratories Inc. <https://www.silabs.com/>
//
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @file TestZW_slave.c
 * @copyright 2022 Silicon Laboratories Inc.
 */
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "ZW_slave.h"

#include <zpal_power_manager.h>
#include "ZW_explore.h"
#include "ZW_ismyframe.h"
#include "ZW_routing_all.h"
#include "ZW_receivefilter_learnmode.h"
#include "ZW_keystore.h"
#include <zpal_radio_utils.h>

#include "ZW_s2_inclusion_glue.h"
#include "ZW_Security_Scheme2.h"

#include "SwTimer.h"
#include "SwTimerLiaison.h"
#include "ZW_protocol_cmd_handler.h"
#include "ZW_protocol_interface.h"
#include "ZW_CCList.h"

#include <unity.h>
#include <mock_control.h>
#include <SizeOf.h>

void setUpSuite(void) {

}

void tearDownSuite(void) {

}

void setUp(void) {

}

void tearDown(void) {

}


static CommandHandler_arg_t pArgs;

CommandHandler_arg_t * command_handler_set_arguments(uint8_t * pCmd, uint8_t cmdLength, RECEIVE_OPTIONS_TYPE * pRxOptions)
{
  pArgs.cmd = pCmd;
  pArgs.cmdLength = cmdLength;
  pArgs.rxOpt = pRxOptions;
  return &pArgs;
}

#define COMMAND_HANDLER_RETURN_ARG_OBJECT(pFrame, pRxOptions) \
  command_handler_set_arguments((uint8_t *)pFrame, sizeof(*pFrame), pRxOptions)

/**
 * Global variables needed for build
 */
sec_learn_complete_t learnNodeFunc;  /* Node learn call back function. */
bool exploreInclusionModeRepeat;
bool bUseExploreAsRouteResolution;
uint8_t bNetworkWideInclusion;
struct S2* s2_ctx = NULL;

extern void ZW_HandleCmdNOP(uint8_t *pCmd, uint8_t cmdLength, RECEIVE_OPTIONS_TYPE *rxopt);

/*
 * Test ZW_HandleCmdNOP
 */
void test_ZW_HandleCmdNOP(void)
{
  mock_t * pMock;
  mock_calls_clear();

  RECEIVE_OPTIONS_TYPE rxopt = {0};
  /*
     NOP frame was received
   * Expected: NOP handler triggered
   */
  NOP_FRAME nopFrame = {.cmd = ZWAVE_CMD_NOP};

  mock_call_expect(TO_STR(ProtocolInterfacePassToAppNodeUpdate), &pMock);
  pMock->expect_arg[0].v = UPDATE_STATE_NOP_POWER_RECEIVED;
  pMock->expect_arg[1].v = 0; // source node
  pMock->compare_rule_arg[2] = COMPARE_NULL;
  pMock->expect_arg[3].v = 0; // length
  pMock->return_code.v = true;

  ZW_HandleCmdNOP((uint8_t*)&nopFrame, sizeof(NOP_FRAME), &rxopt);
  mock_calls_verify();
}

/*
 * Test zw_protocol_invoke_cmd_handler_lr
 */
void test_zw_protocol_invoke_cmd_handler_lr(void)
{
  mock_t * pMock;
  mock_calls_clear();

  RECEIVE_OPTIONS_TYPE rxopt = {0};

  /*
   * NOP LR frame was received
   * Expected: NOP LR handler triggered
   */
  NOP_FRAME_LR nopFrameLR = {
                             .cmd = ZWAVE_LR_CMD_NOP,
                             .cmdClass = ZWAVE_CMD_CLASS_PROTOCOL_LR };

  mock_call_expect(TO_STR(ProtocolInterfacePassToAppNodeUpdate), &pMock);
  pMock->expect_arg[0].v = UPDATE_STATE_NOP_POWER_RECEIVED;
  pMock->expect_arg[1].v = 0; // source node
  pMock->compare_rule_arg[2] = COMPARE_NULL;
  pMock->expect_arg[3].v = 0; // length
  pMock->return_code.v = true;

  zw_protocol_invoke_cmd_handler_lr((uint8_t*)&nopFrameLR, sizeof(NOP_FRAME_LR), &rxopt);
  mock_calls_verify();
}

/*
 * Test CommandHandler
 */
void test_CommandHandlerCall(void)
{
  mock_t * pMock;
  mock_calls_clear();

  RECEIVE_OPTIONS_TYPE rxopt = {0};
  uint8_t cmd[5];

  /*
   * Case1 - call function with dummy data.
   * Expected: Nothing should happen
   */
  cmd[0] = 0x19;   // set some dummy data

  // Call with non existing CMD code, just to trigger the function
  CommandHandler(COMMAND_HANDLER_RETURN_ARG_OBJECT(cmd, &rxopt));

  /*
   * Case2 - NOP frame was received
   * Expected: NOP handler triggered
   */
  NOP_FRAME nopFrame = {.cmd = ZWAVE_CMD_NOP};

  mock_call_expect(TO_STR(ProtocolInterfacePassToAppNodeUpdate), &pMock);
  pMock->expect_arg[0].v = UPDATE_STATE_NOP_POWER_RECEIVED;
  pMock->expect_arg[1].v = 0; // source node
  pMock->compare_rule_arg[2] = COMPARE_NULL;
  pMock->expect_arg[3].v = 0; // length
  pMock->return_code.v = true;

  CommandHandler(COMMAND_HANDLER_RETURN_ARG_OBJECT(&nopFrame, &rxopt));

  /*
   * Case3 - NOP LR frame was received
   * Expected: NOP LR handler triggered
   */
  NOP_FRAME_LR nopFrameLR = {
                             .cmd = ZWAVE_LR_CMD_NOP,
                             .cmdClass = ZWAVE_CMD_CLASS_PROTOCOL_LR };

  mock_call_expect(TO_STR(ProtocolInterfacePassToAppNodeUpdate), &pMock);
  pMock->expect_arg[0].v = UPDATE_STATE_NOP_POWER_RECEIVED;
  pMock->expect_arg[1].v = 0; // source node
  pMock->compare_rule_arg[2] = COMPARE_NULL;
  pMock->expect_arg[3].v = 0; // length
  pMock->return_code.v = true;

  CommandHandler(COMMAND_HANDLER_RETURN_ARG_OBJECT(&nopFrameLR, &rxopt));

  /*
   * Case5 - Assign ID frame was received
   * Expected: corresponding handler triggered
   */
  ASSIGN_IDS_FRAME assignIdFrame = { .cmd = ZWAVE_CMD_ASSIGN_IDS,
                                     .cmdClass = ZWAVE_CMD_CLASS_PROTOCOL };

  // no mocks here (yet)
  CommandHandler(COMMAND_HANDLER_RETURN_ARG_OBJECT(&assignIdFrame, &rxopt));

  /*
   * Case6 - Assign ID LR frame was received
   * Expected: corresponding handler triggered
   */
  ASSIGN_IDS_FRAME_LR assignIdFrameLR = {
                                          .cmd = ZWAVE_LR_CMD_ASSIGN_IDS,
                                          .cmdClass = ZWAVE_CMD_CLASS_PROTOCOL_LR };

  // no mocks here (yet)
  CommandHandler(COMMAND_HANDLER_RETURN_ARG_OBJECT(&assignIdFrameLR, &rxopt));

  /*
   * Case7 - Non Secure Inclusion Complete LR frame was received
   * Expected: corresponding handler triggered
   */
  NON_SECURE_INCLUSION_COMPLETE_FRAME_LR nonSecInclusionFrameLR = {
                                          .cmd = ZWAVE_LR_CMD_NON_SECURE_INCLUSION_COMPLETE,
                                          .cmdClass = ZWAVE_CMD_CLASS_PROTOCOL_LR };

  mock_call_use_as_stub(TO_STR(S2_init_ctx));
  CommandHandler(COMMAND_HANDLER_RETURN_ARG_OBJECT(&nonSecInclusionFrameLR, &rxopt));
  mock_calls_verify();
}

/*
 * Init some data needed by SlaveInit()
 */
static void prepare_slave(void)
{
  static  SApplicationInterface appInterface;
  static const zpal_radio_network_stats_t networkStatistics;
  static const SProtocolInfo protocolInfo;
  static const SNetworkInfo networkInfo;
  static const SLongRangeInfo longRangeInfo;
  static const SRadioStatus radioStatus;
  static const SAppNodeInfo_t nodeInfo;
  static const SProtocolConfig_t protocolConfig = {
                                                   .pNodeInfo = &nodeInfo };

  appInterface.AppHandles.pNetworkStatistics = &networkStatistics;
  appInterface.AppHandles.pProtocolInfo = &protocolInfo;
  appInterface.AppHandles.pNetworkInfo = &networkInfo;
  appInterface.AppHandles.pLongRangeInfo= &longRangeInfo;
  appInterface.AppHandles.pRadioStatus = &radioStatus;
  appInterface.pProtocolConfig = &protocolConfig;

  CCListSetNetworkInfo(appInterface.AppHandles.pNetworkInfo);


}

static void helper_func_mocks_setup(void)
{
  mock_t * pMock;
  mock_calls_clear();
  mock_call_use_as_stub(TO_STR(zpal_pm_register));
  mock_call_use_as_stub(TO_STR(llIsHeaderFormat3ch));
  mock_call_use_as_stub(TO_STR(ZwTimerRegister));
  mock_call_use_as_stub(TO_STR(zpal_radio_enable_rx_broadcast_beam));
  mock_call_use_as_stub(TO_STR(TimerStop));
  mock_call_use_as_stub(TO_STR(ZW_LockRoute));
  mock_call_use_as_stub(TO_STR(ZW_system_startup_GetCCSet));
  mock_call_expect(TO_STR(zpal_radio_get_protocol_mode), &pMock);
  pMock->return_code.value = zpal_radio_region_get_protocol_mode(REGION_US_LR, ZPAL_RADIO_LR_CH_CFG1);
}

/*
 * Test GenerateNodeInformation
 */
void test_GenerateNodeInformation(void)
{
  // Random values that can be verified during the test.
  const uint8_t GENERIC_DEVICE_CLASS  = 0x45;
  const uint8_t SPECIFIC_DEVICE_CLASS = 0x54;

  SAppNodeInfo_t appNodeInfo;
  NODEINFO_SLAVE_FRAME pnodeInfo;
  NODEINFO_SLAVE_FRAME pnodeInfo_expected;

  /*
   * Test 1 - GenerateNodeInformation output for LR included 1000ms FLiRS slave node on REGION_US_LR
   *
   * Expected:
   * - Speed bits should be set to 100kbps LR only (40 and 9.6kbps should not be set)
   * - Routing support (ZWAVE_NODEINFO_ROUTING_SUPPORT) should not be set
   * - Optional functionality should not be set (ZWAVE_NODEINFO_OPTIONAL_FUNC)
   * - Only 1000ms frequently listening mode should be allowed
   *
   */
  helper_func_mocks_setup();

  g_nodeID = LOWEST_LONG_RANGE_NODE_ID;
  pnodeInfo_expected.cmdClass = ZWAVE_CMD_CLASS_PROTOCOL_LR;
  pnodeInfo_expected.cmd = ZWAVE_LR_CMD_NODE_INFO;
  pnodeInfo_expected.capability = 0;
  pnodeInfo_expected.security = ZWAVE_NODEINFO_SENSOR_MODE_WAKEUP_1000;
  pnodeInfo_expected.reserved = ZWAVE_NODEINFO_BAUD_100KLR;
  pnodeInfo_expected.nodeType.generic = GENERIC_DEVICE_CLASS;
  pnodeInfo_expected.nodeType.specific = SPECIFIC_DEVICE_CLASS;
  pnodeInfo_expected.nodeInfo[0] = 0; // Number of command classes in the list.
  prepare_slave();
  appNodeInfo.DeviceOptionsMask = APPLICATION_FREQ_LISTENING_MODE_1000ms;
  appNodeInfo.NodeType.generic = GENERIC_DEVICE_CLASS;
  appNodeInfo.NodeType.specific = SPECIFIC_DEVICE_CLASS;
  SlaveInit(&appNodeInfo);
  uint8_t len = GenerateNodeInformation(&pnodeInfo, ZWAVE_CMD_CLASS_PROTOCOL_LR);

  TEST_ASSERT_EQUAL_UINT8_MESSAGE(8, len, "Nodeinfo length wrong (test 1)");
  TEST_ASSERT_EQUAL_UINT8_ARRAY_MESSAGE(&pnodeInfo_expected, &pnodeInfo, len, "Returned Nodeinfo wrong for LR included REGION_US_LR FLiRS node");

  mock_calls_verify();

  /*
   * Test 2 - GenerateNodeInformation output for none included 1000ms FLiRS slave node on REGION_US_LR
   *
   * Expected:
   * - Speed bits 100kbps LR, 100kbps and 40kbps should be set
   * - Routing support (ZWAVE_NODEINFO_ROUTING_SUPPORT) should be set
   * - Optional functionality (ZWAVE_NODEINFO_OPTIONAL_FUNC) should be set
   * - 1000ms frequently listening mode should be set
   *
   */
  helper_func_mocks_setup();

  g_nodeID = 0;
  pnodeInfo_expected.cmdClass = ZWAVE_CMD_CLASS_PROTOCOL;
  pnodeInfo_expected.cmd = ZWAVE_CMD_NODE_INFO;
  pnodeInfo_expected.capability = ZWAVE_NODEINFO_VERSION_4 | ZWAVE_NODEINFO_BAUD_40000 | ZWAVE_NODEINFO_ROUTING_SUPPORT;
  pnodeInfo_expected.security = ZWAVE_NODEINFO_BEAM_CAPABILITY | ZWAVE_NODEINFO_SLAVE_ROUTING |
                                ZWAVE_NODEINFO_SPECIFIC_DEVICE_TYPE | ZWAVE_NODEINFO_OPTIONAL_FUNC |
                                ZWAVE_NODEINFO_SENSOR_MODE_WAKEUP_1000;
  pnodeInfo_expected.reserved = ZWAVE_NODEINFO_BAUD_100KLR | ZWAVE_NODEINFO_BAUD_100K;
  pnodeInfo_expected.nodeType.generic = GENERIC_DEVICE_CLASS;
  pnodeInfo_expected.nodeType.specific = SPECIFIC_DEVICE_CLASS;
  prepare_slave();
  appNodeInfo.DeviceOptionsMask = APPLICATION_FREQ_LISTENING_MODE_1000ms;
  appNodeInfo.NodeType.generic = GENERIC_DEVICE_CLASS;
  appNodeInfo.NodeType.specific = SPECIFIC_DEVICE_CLASS;
  SlaveInit(&appNodeInfo);
  len = GenerateNodeInformation(&pnodeInfo, ZWAVE_CMD_CLASS_PROTOCOL);

  TEST_ASSERT_EQUAL_UINT8_MESSAGE(7, len, "Nodeinfo length wrong (test 2)");
  TEST_ASSERT_EQUAL_UINT8_ARRAY_MESSAGE(&pnodeInfo_expected, &pnodeInfo, len, "Returned Nodeinfo wrong for not included REGION_US_LR FLiRS node");

  mock_calls_verify();

  /*
   * Test 3 - GenerateNodeInformation output for Classic included 250ms FLiRS slave node on REGION_US_LR
   *
   * Expected:
   * - Speed bits 100kbps LR, 100kbps and 40kbps should be set
   * - Routing support (ZWAVE_NODEINFO_ROUTING_SUPPORT) should be set
   * - Optional functionality (ZWAVE_NODEINFO_OPTIONAL_FUNC) should be set
   * - 250ms frequently listening mode should be set
   *
   */
  helper_func_mocks_setup();

  g_nodeID = 6;
  pnodeInfo_expected.cmdClass = ZWAVE_CMD_CLASS_PROTOCOL;
  pnodeInfo_expected.cmd = ZWAVE_CMD_NODE_INFO;
  pnodeInfo_expected.capability = ZWAVE_NODEINFO_VERSION_4 | ZWAVE_NODEINFO_BAUD_40000 | ZWAVE_NODEINFO_ROUTING_SUPPORT;
  pnodeInfo_expected.security = ZWAVE_NODEINFO_BEAM_CAPABILITY | ZWAVE_NODEINFO_SLAVE_ROUTING |
                                ZWAVE_NODEINFO_SPECIFIC_DEVICE_TYPE | ZWAVE_NODEINFO_OPTIONAL_FUNC |
                                ZWAVE_NODEINFO_SENSOR_MODE_WAKEUP_250;
  pnodeInfo_expected.reserved = ZWAVE_NODEINFO_BAUD_100KLR | ZWAVE_NODEINFO_BAUD_100K;
  pnodeInfo_expected.nodeType.generic = GENERIC_DEVICE_CLASS;
  pnodeInfo_expected.nodeType.specific = SPECIFIC_DEVICE_CLASS;
  prepare_slave();
  appNodeInfo.DeviceOptionsMask = APPLICATION_FREQ_LISTENING_MODE_250ms;
  appNodeInfo.NodeType.generic = GENERIC_DEVICE_CLASS;
  appNodeInfo.NodeType.specific = SPECIFIC_DEVICE_CLASS;
  SlaveInit(&appNodeInfo);
  len = GenerateNodeInformation(&pnodeInfo, ZWAVE_CMD_CLASS_PROTOCOL);

  TEST_ASSERT_EQUAL_UINT8_MESSAGE(7, len, "Nodeinfo length wrong (test 3)");
  TEST_ASSERT_EQUAL_UINT8_ARRAY_MESSAGE(&pnodeInfo_expected, &pnodeInfo, len, "Returned Nodeinfo wrong for classic included REGION_US_LR FLiRS node");

  mock_calls_verify();
}

/*
 * Verifies that the NIF doesn't contain CC S0 when the node is included as a LR node.
 */
void test_GenerateNodeInformation_no_S0_when_included_as_LR(void)
{
  mock_t * pMock;
  mock_calls_clear();

  mock_call_use_as_stub(TO_STR(zpal_pm_register));
  mock_call_use_as_stub(TO_STR(llIsHeaderFormat3ch));
  mock_call_use_as_stub(TO_STR(ZwTimerRegister));
  mock_call_use_as_stub(TO_STR(zpal_radio_enable_rx_broadcast_beam));
  mock_call_use_as_stub(TO_STR(TimerStop));
  mock_call_use_as_stub(TO_STR(ZW_LockRoute));
  mock_call_use_as_stub(TO_STR(ChooseSpeedForDestination_slave));
  mock_call_expect(TO_STR(zpal_radio_get_protocol_mode), &pMock);
  pMock->return_code.value = zpal_radio_region_get_protocol_mode(REGION_US_LR, ZPAL_RADIO_LR_CH_CFG1);

  /*
   * Set up all values referenced in relation to NIF processing.
   */
  g_nodeID = LOWEST_LONG_RANGE_NODE_ID;

  /*
   * Adding more CCs than allowed due to the value of NODEPARM_MAX to verify that it's being
   * limited correctly. The max number will be 34 (and not 35) because we use one byte for
   * the number of CCs.
   */
  uint8_t CCs[] = {
                          COMMAND_CLASS_TRANSPORT_SERVICE_V2,
                          COMMAND_CLASS_SECURITY,
                          COMMAND_CLASS_SECURITY_2,
                          3,
                          4, 5,
                          6, 7,
                          8, 9,
                          10, 11,
                          12, 13,
                          14, 15,
                          16, 17,
                          18, 19,
                          20, 21,
                          22, 23,
                          24, 25,
                          26, 27,
                          28, 29,
                          30, 31,
                          32, 33,
                          34 // Should be removed.
  };

  SCommandClassSet_t CCSet = {
    .SecureIncludedUnSecureCC = {
      .iListLength = sizeof_array(CCs),
      .pCommandClasses = CCs
    }
  };

  const SAppNodeInfo_t appNodeInfo = {
                                      .DeviceOptionsMask = APPLICATION_NODEINFO_LISTENING,
                                      .NodeType = {
                                                   .generic = GENERIC_TYPE_SWITCH_BINARY,
                                                   .specific = SPECIFIC_TYPE_NOT_USED
                                      },
  };
  SlaveInit(&appNodeInfo);

  const SNetworkInfo networkInfo = {
                                    .eInclusionState = EINCLUSIONSTATE_SECURE_INCLUDED
  };

  CCListSetNetworkInfo(&networkInfo);

  mock_call_expect(TO_STR(ZW_system_startup_GetCCSet), &pMock);
  pMock->return_code.p = &CCSet;

  /*
   * Setup expectations and transmit a "Node Info Request" to the slave.
   */
  const uint8_t nif_expected[] = {
                                  ZWAVE_CMD_CLASS_PROTOCOL_LR,        // cmdClass
                                  ZWAVE_LR_CMD_NODE_INFO,             // cmd
                                  ZWAVE_NODEINFO_LISTENING_MASK,      // capability
                                  0,                                  // security
                                  ZWAVE_NODEINFO_BAUD_100KLR,         // reserved
                                  GENERIC_TYPE_SWITCH_BINARY,         // nodeType generic
                                  SPECIFIC_TYPE_NOT_USED,             // nodeType specific
                                  33,                                  // CC count
                                  COMMAND_CLASS_TRANSPORT_SERVICE_V2, // CC index 0
                                  COMMAND_CLASS_SECURITY_2,           // CC index 1
                                  3,
                                  4, 5,
                                  6, 7,
                                  8, 9,
                                  10, 11,
                                  12, 13,
                                  14, 15,
                                  16, 17,
                                  18, 19,
                                  20, 21,
                                  22, 23,
                                  24, 25,
                                  26, 27,
                                  28, 29,
                                  30, 31,
                                  32, 33
  };

  const uint8_t request_nif[] = {
                                 ZWAVE_CMD_CLASS_PROTOCOL_LR,
                                 ZWAVE_LR_CMD_REQUEST_NODE_INFO
  };

  const node_id_t SOURCE_NODE_ID = 1;

  const RECEIVE_OPTIONS_TYPE rxOptions = {
                                          .sourceNode = SOURCE_NODE_ID,
                                          .isLongRangeChannel = true
  };

  mock_call_expect(TO_STR(EnQueueSingleData), &pMock);
  pMock->expect_arg[0].v = RF_SPEED_LR_100K;
  pMock->compare_rule_arg[1] = COMPARE_ANY;
  pMock->expect_arg[2].v = SOURCE_NODE_ID;
  pMock->expect_arg[3].p = (uint8_t *)&nif_expected;
  pMock->expect_arg[4].v = sizeof(nif_expected);
  pMock->expect_arg[5].v = TRANSMIT_OPTION_ACK | TRANSMIT_OPTION_AUTO_ROUTE;
  pMock->expect_arg[6].v = 0; // Delay
  pMock->expect_arg[7].v = ZPAL_RADIO_TX_POWER_DEFAULT;
  pMock->compare_rule_arg[8] = COMPARE_NOT_NULL;

  mock_call_expect(TO_STR(ProtocolInterfacePassToAppNodeUpdate), &pMock);
  pMock->expect_arg[0].v = UPDATE_STATE_REQUEST_NODE_INFO_RECEIVED;
  pMock->expect_arg[1].v = SOURCE_NODE_ID;
  pMock->compare_rule_arg[2] = COMPARE_NULL;
  pMock->expect_arg[3].v = 0;

  CommandHandler(COMMAND_HANDLER_RETURN_ARG_OBJECT(request_nif, (RECEIVE_OPTIONS_TYPE *)&rxOptions));

  mock_calls_verify();
}

/*
 * Test SlaveInit()
 */
void test_SlaveInit(void)
{
  mock_t * pMock;
  mock_calls_clear();

  SAppNodeInfo_t appNodeInfo;
  SSwTimer NeighborCompleteDelayTimer;
  SSwTimer SUC_UpdateTimer;
  SSwTimer assignTimer;

  prepare_slave();

  mock_call_use_as_stub(TO_STR(zpal_pm_register));

  mock_call_expect(TO_STR(llIsHeaderFormat3ch), &pMock);
  pMock->return_code.v = true;

  mock_call_expect(TO_STR(ZwTimerRegister), &pMock);
  pMock->compare_rule_arg[0] = COMPARE_NOT_NULL;
  pMock->expect_arg[0].p = &NeighborCompleteDelayTimer;
  pMock->expect_arg[1].v = true;
  pMock->compare_rule_arg[2] = COMPARE_NOT_NULL; //ZCB_NeighborCompleteFailCall
  pMock->return_code.v = ESWTIMERLIAISON_STATUS_SUCCESS;

  mock_call_expect(TO_STR(ZwTimerRegister), &pMock);
  pMock->compare_rule_arg[0] = COMPARE_NOT_NULL;
  pMock->expect_arg[0].p = &SUC_UpdateTimer;
  pMock->expect_arg[1].v = true;
  pMock->compare_rule_arg[2] = COMPARE_NULL;
  pMock->return_code.v = ESWTIMERLIAISON_STATUS_SUCCESS;

  mock_call_expect(TO_STR(zpal_radio_enable_rx_broadcast_beam), &pMock);
  pMock->expect_arg[0].v = false;

  // AssignTimerInit
  mock_call_expect(TO_STR(ZwTimerRegister), &pMock);
  pMock->compare_rule_arg[0] = COMPARE_NOT_NULL;
  pMock->expect_arg[0].p = &assignTimer;
  pMock->expect_arg[1].v = true;
  pMock->compare_rule_arg[2] = COMPARE_NOT_NULL; //ZCB_AssignTimeout
  pMock->return_code.v = ESWTIMERLIAISON_STATUS_SUCCESS;

  // AssignTimerStop
  mock_call_expect(TO_STR(TimerStop), &pMock);
  pMock->compare_rule_arg[0] = COMPARE_NOT_NULL;
  pMock->expect_arg[0].p = &assignTimer;

  mock_call_expect(TO_STR(ZW_LockRoute), &pMock);
  pMock->expect_arg[0].v = false;

  SlaveInit(&appNodeInfo);

  mock_calls_verify();
}

/*
 * Test NodeSupportsLR
 */
void test_NodeSupportsLR(void)
{
  mock_t * pMock;
  bool status;
  mock_calls_clear();

  mock_call_expect(TO_STR(zpal_radio_get_protocol_mode), &pMock);
  pMock->return_code.value = zpal_radio_region_get_protocol_mode(REGION_US_LR, ZPAL_RADIO_LR_CH_CFG1);

  status = NodeSupportsBeingIncludedAsLR();
  TEST_ASSERT_EQUAL_MESSAGE(true, status, "Unexpected return value 0!");

  mock_calls_verify();
}
